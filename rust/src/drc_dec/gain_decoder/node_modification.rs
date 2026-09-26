/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung
e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */
//! MPEG-D DRC module for gain interpolation modification.

use super::super::{
    common::{self, DuckingModification, EffectBit, GainModification},
    drc_error::DrcError,
    reader::{
        CharacteristicFormat, CharacteristicSide, CustomDrcCharRef, DrcCharacteristic,
        DrcCoefficientsUniDrc,
    },
};
use super::{
    common::{
        CICP_DRC_CHAR_NODES_LEFT, CICP_DRC_CHAR_NODES_RIGHT, CICP_DRC_CHAR_SIGMOID_LEFT,
        CICP_DRC_CHAR_SIGMOID_RIGHT,
    },
    constants::SLOPE_FACTOR_DB_TO_LINEAR,
};

/// Number of DRC compressor characteristics for a Left/Right channel pair. See also
/// `CharacteristicSide` enum.
const NUM_CHARACTERISTIC_SIDES: usize = 2;

#[repr(C)]
#[derive(Debug, Default)]
pub(super) struct NodeModification<'s> {
    /// Effect type of particular DRC set.
    drc_set_effect: u16,
    is_drc_characteristic_present: bool,
    characteristic_format_source: [CharacteristicFormat; NUM_CHARACTERISTIC_SIDES],
    cchar_source: [CustomDrcCharRef<'s>; NUM_CHARACTERISTIC_SIDES],
    characteristic_format_target: [CharacteristicFormat; NUM_CHARACTERISTIC_SIDES],
    cchar_target: [CustomDrcCharRef<'s>; NUM_CHARACTERISTIC_SIDES],
    is_slope_negative: bool,
    is_limiter_peak_target_present: bool,
    limiter_peak_target: f32,
    loudness_normalization_gain_db: f32,
    compress: f32,
    boost: f32,
}

impl<'s> NodeModification<'s> {
    /// Gets effect type of particular DRC set.
    pub(super) fn drc_set_effect(&self) -> u16 {
        self.drc_set_effect
    }

    /// Creates new instance.
    pub fn new() -> Self {
        Default::default()
    }

    /// Gets CICP compression characteristics, based on `CharacteristicFormat`.
    fn get_cicp_characteristic(
        cicp_characteristic: usize,
        drc_char_format: &mut [CharacteristicFormat; NUM_CHARACTERISTIC_SIDES],
        cust_drc_char_ref_src: &mut [CustomDrcCharRef; NUM_CHARACTERISTIC_SIDES],
    ) -> Result<(), DrcError> {
        if !(1..=11).contains(&cicp_characteristic) {
            return Err(DrcError::NotOk);
        }

        let left = usize::from(CharacteristicSide::Left);
        let right = usize::from(CharacteristicSide::Right);

        if cicp_characteristic < 7 {
            // Sigmoid characteristic.
            drc_char_format[left] = CharacteristicFormat::Sigmoid;
            cust_drc_char_ref_src[left].sigmoid =
                Some(&CICP_DRC_CHAR_SIGMOID_LEFT[cicp_characteristic - 1]);
            drc_char_format[right] = CharacteristicFormat::Sigmoid;
            cust_drc_char_ref_src[right].sigmoid =
                Some(&CICP_DRC_CHAR_SIGMOID_RIGHT[cicp_characteristic - 1]);
        } else {
            // Nodes characteristic.
            drc_char_format[left] = CharacteristicFormat::Nodes;
            cust_drc_char_ref_src[left].nodes =
                Some(&CICP_DRC_CHAR_NODES_LEFT[cicp_characteristic - 7]);
            drc_char_format[right] = CharacteristicFormat::Nodes;
            cust_drc_char_ref_src[right].nodes =
                Some(&CICP_DRC_CHAR_NODES_RIGHT[cicp_characteristic - 7]);
        }

        Ok(())
    }

    /// Maps gain output value (dB) based on input `gain_db` and its source and destination DRC
    /// compression characteristics.
    fn map_gain(
        drc_char_format_src: CharacteristicFormat,
        cust_drc_char_ref_src: &CustomDrcCharRef,
        drc_char_format_dst: CharacteristicFormat,
        cust_drc_cchar_ref_dst: &CustomDrcCharRef,
        gain_db: f32,
    ) -> Result<f32, DrcError> {
        let mut in_level = 0.0;
        match drc_char_format_src {
            CharacteristicFormat::Sigmoid => {
                if let Some(sigmoid) = cust_drc_char_ref_src.sigmoid {
                    in_level = sigmoid.compressor_io_sigmoid_inverse(gain_db)?
                }
            }
            CharacteristicFormat::Nodes => {
                if let Some(nodes) = cust_drc_char_ref_src.nodes {
                    in_level = nodes.compressor_io_nodes_inverse(gain_db)?
                }
            }
        };

        let mut gain_out_db = 0.0;
        match drc_char_format_dst {
            CharacteristicFormat::Sigmoid => {
                if let Some(sigmoid) = cust_drc_cchar_ref_dst.sigmoid {
                    gain_out_db = sigmoid.compressor_io_sigmoid(in_level)?
                }
            }
            CharacteristicFormat::Nodes => {
                if let Some(nodes) = cust_drc_cchar_ref_dst.nodes {
                    gain_out_db = nodes.compressor_io_nodes(in_level)?
                }
            }
        };

        Ok(gain_out_db)
    }

    fn is_slope_negative(
        drc_char_format: &[CharacteristicFormat; NUM_CHARACTERISTIC_SIDES],
        cust_drc_char_ref: &[CustomDrcCharRef; NUM_CHARACTERISTIC_SIDES],
    ) -> Result<bool, DrcError> {
        let mut slope_sign = [0; NUM_CHARACTERISTIC_SIDES];
        let left = usize::from(CharacteristicSide::Left);
        let right = usize::from(CharacteristicSide::Right);
        slope_sign[left] = cust_drc_char_ref[left].get_slope_sign(drc_char_format[left])?;
        slope_sign[right] = cust_drc_char_ref[right].get_slope_sign(drc_char_format[right])?;

        if (slope_sign[left] != 0 || slope_sign[right] != 0)
            && (slope_sign[left] == -slope_sign[right])
        {
            // DRC characteristic is not invertible.
            return Err(DrcError::NotOk);
        }

        Ok(slope_sign[left] < 0)
    }

    /// This function stores references to DRC characteristics provided by `drc_coeff`. Therefore,
    /// the lifetime of `drc_coeff` reference must be larger (`'l`) than of the lifetime of `self`.
    /// The optional reference to `GainModification` instructs `self` wheter only the source DRC
    /// characteristic is to be stored (i.e. None) or both source and target DRC characteristics.
    ///
    /// # Parameters
    ///
    /// - `drc_char`: DRC characteristics.
    /// - `drc_coeff`: `DrcCoefficientsUniDrc` instance that holds payloads for all available DRC
    ///   gain sequences in one location.
    /// - `gain_modif`: Optional reference to a `GainModification` instance.
    ///
    /// # Return
    ///
    /// - `Ok(())`:
    /// - `Err(DrcError)`
    pub(super) fn prepare_drc_characteristic<'l>(
        &mut self,
        drc_char: &DrcCharacteristic,
        drc_coeff: &'l DrcCoefficientsUniDrc,
        gain_modif: Option<&GainModification>,
    ) -> Result<(), DrcError>
    where
        'l: 's,
    {
        let left = usize::from(CharacteristicSide::Left);
        let right = usize::from(CharacteristicSide::Right);

        self.is_drc_characteristic_present = drc_char.is_present();
        if self.is_drc_characteristic_present {
            if drc_char.is_cicp() {
                Self::get_cicp_characteristic(
                    usize::from(drc_char.cicp_index()),
                    &mut self.characteristic_format_source,
                    &mut self.cchar_source,
                )?;
            } else {
                let cust_index_left = usize::from(drc_char.custom_index(CharacteristicSide::Left));
                let cust_index_right =
                    usize::from(drc_char.custom_index(CharacteristicSide::Right));

                self.characteristic_format_source[left] =
                    drc_coeff.characteristic_format(CharacteristicSide::Left, cust_index_left);
                self.cchar_source[left].sigmoid = Some(
                    &(drc_coeff
                        .custom_characteristic(CharacteristicSide::Left, cust_index_left)
                        .sigmoid),
                );
                self.cchar_source[left].nodes = Some(
                    &(drc_coeff
                        .custom_characteristic(CharacteristicSide::Left, cust_index_left)
                        .nodes),
                );

                self.characteristic_format_source[right] =
                    drc_coeff.characteristic_format(CharacteristicSide::Right, cust_index_right);
                self.cchar_source[right].sigmoid = Some(
                    &(drc_coeff
                        .custom_characteristic(CharacteristicSide::Right, cust_index_right)
                        .sigmoid),
                );
                self.cchar_source[right].nodes = Some(
                    &(drc_coeff
                        .custom_characteristic(CharacteristicSide::Right, cust_index_right)
                        .nodes),
                );
            }

            self.is_slope_negative =
                Self::is_slope_negative(&self.characteristic_format_source, &self.cchar_source)?;

            if let Some(gm) = gain_modif {
                let target_char_left_idx = usize::from(gm.target_characteristic_left_index());
                let target_char_right_idx = usize::from(gm.target_characteristic_right_index());

                if gm.is_target_characteristic_left_present() {
                    self.characteristic_format_target[left] = drc_coeff
                        .characteristic_format(CharacteristicSide::Left, target_char_left_idx);
                    self.cchar_target[left].sigmoid = Some(
                        &(drc_coeff
                            .custom_characteristic(CharacteristicSide::Left, target_char_left_idx)
                            .sigmoid),
                    );
                    self.cchar_target[left].nodes = Some(
                        &(drc_coeff
                            .custom_characteristic(CharacteristicSide::Left, target_char_left_idx)
                            .nodes),
                    );
                }
                if gm.is_target_characteristic_right_present() {
                    self.characteristic_format_target[right] = drc_coeff
                        .characteristic_format(CharacteristicSide::Right, target_char_right_idx);
                    self.cchar_target[right].sigmoid = Some(
                        &(drc_coeff
                            .custom_characteristic(CharacteristicSide::Right, target_char_right_idx)
                            .sigmoid),
                    );
                    self.cchar_target[right].nodes = Some(
                        &(drc_coeff
                            .custom_characteristic(CharacteristicSide::Right, target_char_right_idx)
                            .nodes),
                    );
                }
            }
        }
        Ok(())
    }

    /// Converts a gain and a slope DRC sequence value (expressed in dB) to linear values. The
    /// output is returned as a tuple. The optional input references are used to steer the
    /// result of the linear values, considering the DRC compression characterstics.
    ///
    /// # Parameters
    ///
    /// - `gain_modif`: Optional reference to a `GainModification` instance.
    /// - `ducking_modif`: Optional reference to a `DuckingModification` instance.
    /// - `gain_db`: Input gain value (expressed in dB) to be converted to a linear value.
    /// - `slope_db`: Input slope value (expressed in dB/delta_t_min) to be converted to a linear
    ///   value.
    ///
    /// # Return
    ///
    /// - `Ok((gain_lin, slope_lin))`: Tuple of linear gain and linear slope values.
    /// - `Err(DrcError)`
    pub(super) fn to_linear(
        &self,
        gain_modif: Option<&GainModification>,
        ducking_modif: Option<&DuckingModification>,
        gain_db: f32,
        slope_db: f32,
    ) -> Result<(f32, f32), DrcError> {
        let mut gain_lin;
        let mut slope_lin;

        let mut gain_ratio = 1.0;
        let mut gain_db_offset = 0.0;
        let drc_set_effect = self.drc_set_effect;

        if (drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf) == 0)
            && drc_set_effect != u16::from(EffectBit::Fade)
            && drc_set_effect != u16::from(EffectBit::Clipping)
        {
            if let Some(gm) = gain_modif {
                let gain_db_mapped;

                if self.is_drc_characteristic_present {
                    let left = usize::from(CharacteristicSide::Left);
                    let right = usize::from(CharacteristicSide::Right);

                    if (gain_db > 0.0 && self.is_slope_negative)
                        || (gain_db < 0.0 && !self.is_slope_negative)
                    {
                        // Left side.
                        if gm.is_target_characteristic_left_present() {
                            gain_db_mapped = Self::map_gain(
                                self.characteristic_format_source[left],
                                &self.cchar_source[left],
                                self.characteristic_format_target[left],
                                &self.cchar_target[left],
                                gain_db,
                            )?;

                            // Target characteristic in payload.
                            gain_ratio = gain_db_mapped / gain_db;
                        }
                    } else if (gain_db < 0.0 && self.is_slope_negative)
                        || (gain_db > 0.0 && !self.is_slope_negative)
                    {
                        // Right side.
                        if gm.is_target_characteristic_right_present() {
                            gain_db_mapped = Self::map_gain(
                                self.characteristic_format_source[right],
                                &self.cchar_source[right],
                                self.characteristic_format_target[right],
                                &self.cchar_target[right],
                                gain_db,
                            )?;
                            // Target characteristic in payload.
                            gain_ratio = gain_db_mapped / gain_db;
                        }
                    }
                }
            }
            if gain_db < 0.0 {
                gain_ratio *= self.compress;
            } else {
                gain_ratio *= self.boost;
            }
        }

        if let Some(gm) = gain_modif {
            if gm.is_gain_scaling_present() {
                if gain_db < 0.0 {
                    gain_ratio *= gm.attenuation_scaling();
                } else {
                    gain_ratio *= gm.amplification_scaling();
                }
            }
            if gm.is_gain_offset_present() {
                // lin_gain *= pow(2.0, f64::from((gm.gain_offset()/6.0)));
                gain_db_offset += gm.gain_offset();
            }
        }

        if let Some(dm) = ducking_modif {
            if (drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf) != 0)
                && dm.is_scaling_present()
            {
                gain_ratio *= dm.ducking_scaling();
            }
        }

        let gain_db_modified = gain_db * gain_ratio;

        if self.is_limiter_peak_target_present && drc_set_effect == u16::from(EffectBit::Clipping) {
            // The only drc_set_effect is "clipping prevention".
            // `loudness_normalization_gain_modification_db` is included in
            // `loudness_normalization_gain_db`.
            //
            // gain_lin *= pow(2.0,
            //        max(0.0, -self.limiter_peak_target-self.loudness_normalization_gain_db)/6.0);
            //
            gain_db_offset +=
                (-self.limiter_peak_target - self.loudness_normalization_gain_db).max(0.0);
        }

        let gain_db_out = gain_db_modified + gain_db_offset;

        // gain_lin = pow(2.0, (gain_db_modified / 6.0f));
        gain_lin = common::approx_db_to_linear(gain_db_out);

        // slope_lin = SLOPE_FACTOR_DB_TO_LINEAR * gain_ratio * gain_lin * slope_db;
        if slope_db == 0.0 {
            slope_lin = 0.0;
        } else {
            let tmp_dbl = SLOPE_FACTOR_DB_TO_LINEAR * gain_ratio * slope_db;
            // Recalculate gain_lin from gain_db that wasn't modified by `gain_offset` and
            // `limiter_peak_target`.
            let gain_lin_modified = common::approx_db_to_linear(gain_db_modified);
            slope_lin = tmp_dbl * gain_lin_modified;
        }

        if self.is_limiter_peak_target_present
            && drc_set_effect == u16::from(EffectBit::Clipping)
            && gain_lin >= 1.0
        {
            gain_lin = 1.0;
            slope_lin = 0.0;
        }

        Ok((gain_lin, slope_lin))
    }

    /// Sets boost gain.
    pub(super) fn set_boost(&mut self, val: f32) {
        self.boost = val;
    }

    /// Sets compression gain.
    pub(super) fn set_compress(&mut self, val: f32) {
        self.compress = val;
    }

    /// Sets effect type of particular DRC set.
    pub(super) fn set_drc_set_effect(&mut self, val: u16) {
        self.drc_set_effect = val;
    }

    /// Sets whether the limiter peak target is present.
    pub(super) fn set_limiter_peak_target_present(&mut self, is_present: bool) {
        self.is_limiter_peak_target_present = is_present;
    }

    /// Sets the limiter peak target value.
    pub(super) fn set_limiter_peak_target(&mut self, val: f32) {
        self.limiter_peak_target = val;
    }

    /// Sets the loudness normalization gain value (expressed in dB).
    pub(super) fn set_loudness_normalization_gain_db(&mut self, val: f32) {
        self.loudness_normalization_gain_db = val;
    }
}
