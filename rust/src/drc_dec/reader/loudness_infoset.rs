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
//! MPEG-D DRC's Loudness info set.

use super::super::{
    common::comp_assign,
    constants::{
        DOWNMIX_ID_ANY_DOWNMIX, MAX_EXTENSIONS, MAX_LOUDNESS_INFOS, MAX_LOUDNESS_MEASUREMENTS,
    },
    drc_error::DrcError,
};
use super::{
    loudness_error::LoudnessProcessError,
    loudness_measurement::{LoudnessMeasurement, MethodValueOrder},
    loudness_measurement_system::*,
    loudness_method_definition::{MethodDefinition, MethodDefinitionRequest},
};
use crate::common::bitstream::Bitstream;

/// Loudness info set extension type for termination tag.
const UNIDRCLOUDEXT_TERM: u8 = 0x0;
/// Loudness info set extension type for equalization.
const UNIDRCLOUDEXT_EQ: u8 = 0x1;

#[repr(C)]
#[derive(Default, Debug, PartialEq)]
pub(in super::super) struct LoudnessInfo {
    /// An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// A value of 0x3F indicates that the loudness information can be applied to any DRC including
    /// no DRC.
    /// A DRC set is defined as a set of DRC sequences that produce a desired effect if applied to
    /// the audio signal.
    drc_set_id: i8,
    /// EQ set identifier for EQ instructions.
    eq_set_id: u8,
    /// A unique non-zero downmix identifier.
    downmix_id: u8,
    /// Flag to indicate presence of sample peak level in bitstream.
    is_sample_peak_level_present: bool,
    /// Sample peak level value.
    sample_peak_level: f32,
    /// Flag to indicate presence of true peak level in bitstream.
    is_true_peak_level_present: bool,
    /// True peak level value
    true_peak_level: f32,
    /// Measurement system value.
    true_peak_level_measurement_system: u8,
    /// Reliability value.
    true_peak_level_reliability: u8,
    /// Number of loudness measurements.
    measurement_count: u8,
    /// Buffer to store `LoudnessMeasurement`.
    loudness_measurement: [LoudnessMeasurement; MAX_LOUDNESS_MEASUREMENTS as usize],
}

impl LoudnessInfo {
    #[cfg(test)]
    /// Creates a new `LoudnessInfo` instance.
    pub(in super::super) fn new() -> Self {
        Default::default()
    }

    /// Returns `DRC set identifier`. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    pub(in super::super) fn drc_set_id(&self) -> i8 {
        self.drc_set_id
    }

    /// Finds the index for the the corresponding `LoudnessMeasurement` with a matching
    /// 'MethodDefinition'. The method iterates through inner `LoudnessMeasurement` struct members
    /// starting from `start_idx` and returns only the first index that matches the search
    /// criteria. In case that there is no valid index found, the method returns with
    /// `LoudnessProcessingError`.
    ///
    /// # Parameters
    ///
    /// - `method_def`: Search criteria.
    /// - `start_idx`: Where to start the search from.
    ///
    /// # Return
    ///
    /// - `Result<usize, LoudnessProcessingError`: valid index or `LoudnessProcessingError`.
    fn find_method_definition_index(
        &self,
        method_def: MethodDefinition,
        start_idx: usize,
    ) -> Result<usize, LoudnessProcessError> {
        for (i, lm) in self
            .loudness_measurement
            .iter()
            .enumerate()
            .take(usize::from(self.measurement_count))
            .skip(start_idx)
        {
            if lm.method_definition == method_def {
                return Ok(i);
            }
        }

        Err(LoudnessProcessError::NotOk)
    }

    /// Searches for matching `method_def` instances and returns a `MethodValueOrder` instance
    /// with updated `loudness_value` and `order`.
    ///
    /// # Parameters
    ///
    /// - `method_def`: Method definition indices for the loudness normalization.
    /// - `msrb`: Loudness measurement system indices for the loudness normalization settings.
    ///
    /// # Return
    ///
    /// - `MethodValueOrder` instance.
    fn update_method_value(
        &self,
        method_def: MethodDefinition,
        msrb: MeasurementSystemRequestBonus,
    ) -> MethodValueOrder {
        let mut mvo_out = MethodValueOrder::new();

        // Find all lm instances that are matching the `method_def` criteria.
        let filtered_lm = self
            .loudness_measurement
            .iter()
            .take(usize::from(self.measurement_count))
            .filter(|lm| (lm.method_definition) == method_def);

        // Get the output MethodValueOrder.
        for lm in filtered_lm {
            mvo_out = lm.method_value(msrb, mvo_out);
        }

        mvo_out
    }

    /// Reads `LoudnessInfo` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    ///
    /// # Return
    ///
    /// `DrcError`
    fn read(&mut self, bs: &mut Bitstream, version: u8) -> Result<(), DrcError> {
        self.drc_set_id = bs.read(6) as i8;

        self.eq_set_id = if version >= 1 { bs.read(6) as u8 } else { 0 };
        self.downmix_id = bs.read(7) as u8;

        self.is_sample_peak_level_present = bs.read_bit() != 0;
        if self.is_sample_peak_level_present {
            let bs_sample_peak_level = bs.read(12);
            if bs_sample_peak_level == 0 {
                self.is_sample_peak_level_present = false;
                self.sample_peak_level = 0.0_f32;
            } else {
                self.sample_peak_level = 20.0_f32 - bs_sample_peak_level as f32 * 0.03125_f32;
            }
        }

        self.is_true_peak_level_present = bs.read_bit() != 0;
        if self.is_true_peak_level_present {
            let bs_true_peak_level = bs.read(12);

            if bs_true_peak_level == 0 {
                self.is_true_peak_level_present = false;
                self.true_peak_level = 0.0_f32;
            } else {
                self.true_peak_level = 20.0_f32 - bs_true_peak_level as f32 * 0.03125_f32;
            }

            self.true_peak_level_measurement_system = bs.read(4) as u8;
            self.true_peak_level_reliability = bs.read(2) as u8;
        }

        let measurement_count = bs.read(4) as u8;
        self.measurement_count = measurement_count.min(MAX_LOUDNESS_MEASUREMENTS);

        // CASE: (measurement_count <= MAX_LOUDNESS_MEASUREMENTS).
        for loud_mest in self
            .loudness_measurement
            .iter_mut()
            .take(usize::from(self.measurement_count))
        {
            *loud_mest = LoudnessMeasurement::default();
            loud_mest.read(bs)?;
        }

        // CASE: (measurement_count > MAX_LOUDNESS_MEASUREMENTS).
        if measurement_count > MAX_LOUDNESS_MEASUREMENTS {
            // Read rest of the LoudnessMeasurement from Bitsteam and discard them.
            let mut loud_mes_instance = LoudnessMeasurement::default();
            for _c in MAX_LOUDNESS_MEASUREMENTS..measurement_count {
                loud_mes_instance.read(bs)?;
            }
        }

        Ok(())
    }

    /// Returns current true peak level value expressed in LUFS.
    fn true_peak_level(&self) -> f32 {
        self.true_peak_level
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// `LoudnessInfoSetExtension` structure that holds set extension types and bit sizes.
struct LoudnessInfoSetExtension {
    /// Extension type.
    set_ext_type: [u8; MAX_EXTENSIONS],
    /// Extension bit size.
    ext_bit_size: [u32; MAX_EXTENSIONS - 1],
}

#[repr(C)]
#[derive(Default, Debug)]
/// `LoudnessInfoSet` structure that holds loudness info albums and set extension information.
pub struct LoudnessInfoSet {
    /// Number of `LoudnessInfo` blocks for albums in case of V0.
    li_album_count_v0: u8,
    /// Number of `LoudnessInfo` blocks for albums in case of V1.
    li_album_count_v1: u8,
    /// Total number of `LoudnessInfo` blocks for albums.
    li_album_count: u8,
    /// Number of `LoudnessInfo` blocks in case of V0.
    li_count_v0: u8,
    /// Number of `LoudnessInfo` blocks in case of V1.
    li_count_v1: u8,
    /// Total number of `LoudnessInfo` blocks.
    li_count: u8,
    /// Buffer to store `LoudnessInfo` for album.
    li_album: [LoudnessInfo; MAX_LOUDNESS_INFOS as usize],
    /// Buffer to store `LoudnessInfo`.
    loudness_info: [LoudnessInfo; MAX_LOUDNESS_INFOS as usize],
    /// Flag to indicate presence of set extension in bitstream.
    is_li_set_ext_present: bool,
    /// Loudness info set extension.
    li_set_ext: LoudnessInfoSetExtension,
    /// Flag to indicate reconfigure or not. Each new loudness info data is compared with old one.
    /// In case of difference, flag is updated and set to true.
    is_diff: bool,
}

impl LoudnessInfoSet {
    /// Returns an immutable reference to a correspondent
    /// `LoudnessInfo` instance, based on provided input parameters.
    /// If no valid `LoudnessInfo` instance is found, then the search
    /// goes further with some fallback `drc_set_id` and  `downmix_id_requested` values.
    ///
    /// # Parameters
    ///
    /// - `drc_set_id`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `downmix_id_requested`: An unique non-zero downmix identifier.
    /// - `is_album_mode`: is DRC album mode?
    ///
    /// # Return
    ///
    /// - `Option<&LoudnessInfo>`
    fn find_applicable_loudness_info_instance(
        &self,
        drc_set_id: i8,
        downmix_id_requested: u8,
        is_album_mode: bool,
    ) -> Option<&LoudnessInfo> {
        // Default value
        let mut loudness_info =
            self.find_loudness_info_instance(drc_set_id, downmix_id_requested, is_album_mode);

        // Fallback values
        if loudness_info.is_none() {
            let fallback_values = [
                (drc_set_id, 0x7F),
                (0x3F, downmix_id_requested),
                (0, downmix_id_requested),
                (0x3F, 0x7F),
                (0, 0x7F),
                (drc_set_id, 0),
                (0x3F, 0),
                (0, 0),
            ];

            for find_fallback in fallback_values.iter() {
                let (drc_set_id_val, dmx_id_val) = *find_fallback;
                loudness_info =
                    self.find_loudness_info_instance(drc_set_id_val, dmx_id_val, is_album_mode);
                if loudness_info.is_some() {
                    break;
                }
            }
        }

        loudness_info
    }

    /// Returns an immutable reference to a correspondent
    /// `LoudnessInfo` instance, based on provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `drc_set_id`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `downmix_id`: An unique non-zero downmix identifier.
    /// - `is_album_mode`: is DRC album mode?
    ///
    /// # Return
    ///
    /// - `Option<&LoudnessInfo>`
    fn find_loudness_info_instance(
        &self,
        drc_set_id: i8,
        downmix_id: u8,
        is_album_mode: bool,
    ) -> Option<&LoudnessInfo> {
        let (count, loud_info) = if is_album_mode {
            (self.li_album_count, &self.li_album[..])
        } else {
            (self.li_count, &self.loudness_info[..])
        };

        for (i, li) in loud_info.iter().enumerate().take(usize::from(count)) {
            let count = usize::from(li.measurement_count);
            if li.drc_set_id == drc_set_id && li.downmix_id == downmix_id {
                for lm in li.loudness_measurement.iter().take(count) {
                    match lm.method_definition {
                        MethodDefinition::ProgramLoudness | MethodDefinition::AnchorLoudness => {
                            return Some(&loud_info[i]);
                        }
                        _ => {
                            continue;
                        }
                    }
                }
            }
        }
        None
    }

    /// Gets loudness value (expressed in LUFS), depending on provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `is_album_mode`: is DRC album mode?
    /// - `measurement_method_requested`: Requested measurement method.
    /// - `measurement_system_requested`: Requested measurement system.
    /// - `drc_set_id`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `downmix_id_requested`: An unique non-zero downmix identifier.
    ///
    /// # Return
    ///
    /// - `Result<f32, LoudnessProcessError>`
    ///    - `Ok(loudness_val)`
    ///    - `Err(LoudnessProcessError)`
    pub(in super::super) fn get_loudness(
        &self,
        is_album_mode: bool,
        measurement_method_requested: MethodDefinitionRequest,
        measurement_system_requested: MeasurementSystemRequest,
        drc_set_id: i8,
        downmix_id_requested: u8,
    ) -> Result<f32, LoudnessProcessError> {
        let loudness_val;

        // Map MethodDefinitionRequest::Default to MethodDefinitionRequest::ProgramLoudness.
        let requested_method_definition =
            if measurement_method_requested == MethodDefinitionRequest::Default {
                MethodDefinitionRequest::ProgramLoudness
            } else {
                measurement_method_requested
            };
        let msrb = MeasurementSystemRequestBonus::from(measurement_system_requested);

        let drc_set_id = if drc_set_id < 0 { 0 } else { drc_set_id };
        let mut loudness_info = self.find_applicable_loudness_info_instance(
            drc_set_id,
            downmix_id_requested,
            is_album_mode,
        );

        if is_album_mode && loudness_info.is_none() {
            loudness_info = self.find_applicable_loudness_info_instance(
                drc_set_id,
                downmix_id_requested,
                false,
            );
        }

        match loudness_info {
            Some(li) => {
                let method_def = MethodDefinition::from(requested_method_definition);
                let mut value_order = li.update_method_value(method_def, msrb);

                // Repeat with other method definition.
                if value_order.order == -1 {
                    let method_def_req = if requested_method_definition
                        == MethodDefinitionRequest::ProgramLoudness
                    {
                        MethodDefinitionRequest::AnchorLoudness
                    } else {
                        MethodDefinitionRequest::ProgramLoudness
                    };
                    let method_def = MethodDefinition::from(method_def_req);

                    value_order = li.update_method_value(method_def, msrb);
                }

                if value_order.order == -1 {
                    return Err(LoudnessProcessError::NotOk);
                } else {
                    loudness_val = value_order.loudness_val;
                }
            }
            None => {
                return Err(LoudnessProcessError::UndefinedValue);
            }
        }
        Ok(loudness_val)
    }

    /// Calculates loudness normalization gain, based on desired `target_loudness` and loudness
    /// value of the content.
    ///
    /// # Parameters
    ///
    /// - `target_loudness`: Desired target loudness value.
    /// - `content_loudness`: Loudness value of the content.
    ///
    /// # Return
    ///
    /// - `target_loudness - content_loudness`
    pub(in super::super) fn calc_loudness_normalization_gain(
        target_loudness: f32,
        content_loudness: f32,
    ) -> f32 {
        target_loudness - content_loudness
    }

    /// Returns internal `LoudnessInfo` for album instance, given the provided `index`.
    ///
    /// # Parameters
    ///
    /// - `index`: The index value of the `LoudnessInfo` for album instance to be returned. Up to
    ///   MAX_LOUDNESS_INFOS.
    pub(in super::super) fn li_album(&self) -> &[LoudnessInfo; MAX_LOUDNESS_INFOS as usize] {
        &self.li_album
    }

    /// Returns total number of `LoudnessInfo` blocks for albums.
    pub(in super::super) fn li_album_count(&self) -> u8 {
        self.li_album_count
    }

    /// Returns the total number of `LoudnessInfo` blocks.
    pub(in super::super) fn li_count(&self) -> u8 {
        self.li_count
    }

    /// Gets correspondent production mixing level.
    ///
    /// # Parameters
    ///
    /// - `downmix_id_requested`: An unique non-zero downmix identifier.
    /// - `drc_set_id_requested`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `is_album_mode`: is DRC album mode?
    ///
    /// # Return
    ///
    /// - `Result<f32, LoudnessProcessError>`
    ///    - `Ok(mixing_level)`
    ///    - `Err(LoudnessProcessError)`
    pub(in super::super) fn get_mixing_level(
        &self,
        downmix_id_requested: u8,
        drc_set_id_requested: i8,
        is_album_mode: bool,
    ) -> Result<f32, LoudnessProcessError> {
        const MIXING_LEVEL_DFLT: f32 = 85.0;
        let mut mix_level = MIXING_LEVEL_DFLT;

        let drc_set_id_req = if drc_set_id_requested < 0 {
            0
        } else {
            drc_set_id_requested
        };

        let (count, loud_info) = if is_album_mode {
            (self.li_album_count, &self.li_album[..])
        } else {
            (self.li_count, &self.loudness_info[..])
        };

        for li in loud_info.iter().take(usize::from(count)) {
            if (li.drc_set_id == drc_set_id_req)
                && (li.downmix_id == downmix_id_requested
                    || li.downmix_id == DOWNMIX_ID_ANY_DOWNMIX)
            {
                let index_res = li.find_method_definition_index(MethodDefinition::MixingLevel, 0);

                match index_res {
                    Ok(i) => {
                        mix_level = li.loudness_measurement[i].method_value;
                        break;
                    }
                    Err(_e) => {
                        // Index not ok. Go to next `LoudnessInfo` instance.
                        continue;
                    }
                }
            }
        }
        Ok(mix_level)
    }

    /// Returns whether a difference is detected in the `LoudnessInfoSet` instance.
    pub(in super::super) fn is_diff(&self) -> bool {
        self.is_diff
    }

    /// Reads loudness set extension for equalization (`UNIDRCLOUDEXT_EQ`) from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `DrcError`.
    fn read_ext_eq(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut diff = self.is_diff;
        diff |= comp_assign(&mut self.li_album_count_v1, bs.read(6) as u8);
        diff |= comp_assign(&mut self.li_count_v1, bs.read(6) as u8);

        let offset = self.li_album_count_v0;
        self.li_album_count = (offset + self.li_album_count_v1).min(MAX_LOUDNESS_INFOS);

        let li_count_iter = MAX_LOUDNESS_INFOS.saturating_sub(offset);

        for li_alb in self
            .li_album
            .iter_mut()
            .skip(usize::from(offset))
            .take(usize::from(self.li_album_count_v1.min(li_count_iter)))
        {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 1)?;

            diff |= comp_assign(li_alb, tmp_loud);
        }

        for _i in self.li_album_count_v1.min(li_count_iter)..self.li_album_count_v1 {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 1)?;
        }

        let offset = self.li_count_v0;
        self.li_count = (offset + self.li_count_v1).min(MAX_LOUDNESS_INFOS);

        let li_count_iters = MAX_LOUDNESS_INFOS.saturating_sub(offset);

        for loud_info in self
            .loudness_info
            .iter_mut()
            .skip(usize::from(offset))
            .take(usize::from(self.li_count_v1.min(li_count_iters)))
        {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 1)?;

            diff |= comp_assign(loud_info, tmp_loud);
        }

        for _i in self.li_count_v1.min(li_count_iters)..self.li_count_v1 {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 1)?;
        }

        self.is_diff = diff;

        Ok(())
    }

    /// Reads loudness info set extension data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn read_extension(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut is_early_break = false;
        // (i.e self.set_ext_type.len());
        let iter_length = MAX_EXTENSIONS;
        'read_loop: for i in 0..iter_length - 1 {
            let ext_type = bs.read(4) as u8;
            self.li_set_ext.set_ext_type[i] = ext_type;

            if ext_type == UNIDRCLOUDEXT_TERM {
                is_early_break = true;
                break 'read_loop;
            }
            let bit_size_len = bs.read(4) as u8;
            let ext_size_bits = bit_size_len + 4;

            let bit_size = bs.read(ext_size_bits);
            let ext_bit_size = bit_size + 1;
            self.li_set_ext.ext_bit_size[i] = ext_bit_size;

            let bits_remaining = bs.valid_bits();

            match ext_type {
                UNIDRCLOUDEXT_EQ => {
                    self.read_ext_eq(bs)?;

                    if bits_remaining != (ext_bit_size as isize + bs.valid_bits()) {
                        return Err(DrcError::NotOk);
                    }
                }
                // Add future extensions here.
                _ => bs.push(ext_bit_size as isize),
            }
        }

        if !is_early_break {
            // Read last item of uni_drc_gain_ext_type (i.e [MAX_EXTENSIONS-1]) from
            // bitstream.
            self.li_set_ext.set_ext_type[MAX_EXTENSIONS - 1] = bs.read(4) as u8;
            if self.li_set_ext.set_ext_type[MAX_EXTENSIONS - 1] != UNIDRCLOUDEXT_TERM {
                return Err(DrcError::MemoryError);
            }
        }

        Ok(())
    }

    /// Reads and parses `loudnessInfoSet` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut diff = comp_assign(&mut self.li_album_count_v0, bs.read(6) as u8);
        diff |= comp_assign(&mut self.li_count_v0, bs.read(6) as u8);

        self.li_album_count = (self.li_album_count_v0).min(MAX_LOUDNESS_INFOS);

        for li_alb in self
            .li_album
            .iter_mut()
            .take(usize::from(self.li_album_count))
        {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 0)?;

            diff |= comp_assign(li_alb, tmp_loud);
        }

        for _i in self.li_album_count..self.li_album_count_v0 {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 0)?;
        }

        self.li_count = self.li_count_v0.min(MAX_LOUDNESS_INFOS);

        for loud_info in self
            .loudness_info
            .iter_mut()
            .take(usize::from(self.li_count))
        {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 0)?;

            diff |= comp_assign(loud_info, tmp_loud);
        }

        for _i in self.li_count..self.li_count_v0 {
            let mut tmp_loud = LoudnessInfo::default();
            tmp_loud.read(bs, 0)?;
        }

        diff |= comp_assign(&mut self.is_li_set_ext_present, bs.read_bit() != 0);
        self.is_diff = self.is_diff || diff;

        if self.is_li_set_ext_present {
            self.read_extension(bs)?;
        }

        Ok(())
    }

    /// Gets true peak level value of a `LoudnessInfo`, depending on provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `drc_set_id`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `downmix_id`: An unique non-zero downmix identifier.
    /// - `is_album_mode`: is DRC album mode?
    /// - `true_peak_level`: Output value, if info is found in matching `LoudnessInfo` struct.
    ///
    /// # Return
    ///
    /// - `true` if true peak is found, otherwise `false`.
    pub(in super::super) fn true_peak_level(
        &self,
        drc_set_id: i8,
        downmix_id: u8,
        is_album_mode: bool,
        true_peak_level: &mut f32,
    ) -> bool {
        let mut is_true_peak_level_found = false;

        let (count, loud_info) = if is_album_mode {
            (self.li_album_count, &self.li_album[..])
        } else {
            (self.li_count, &self.loudness_info[..])
        };

        for li in loud_info.iter().take(usize::from(count)) {
            if li.drc_set_id == drc_set_id
                && li.downmix_id == downmix_id
                && li.is_true_peak_level_present
            {
                *true_peak_level = li.true_peak_level();
                is_true_peak_level_found = true;
                break;
            }
        }

        is_true_peak_level_found
    }

    /// Gets sample peak level value of a `LoudnessInfo`, depending on provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `drc_set_id`: An unique non-zero identifier for this DRC set. 0x0 is reserved.
    /// - `downmix_id`: An unique non-zero downmix identifier.
    /// - `is_album_mode`: is DRC album mode?
    /// - `sample_peak_level`: Output value, if info is found in matching `LoudnessInfo` struct.
    ///
    /// # Return
    ///
    /// - `true` if sample peak level is found, otherwise `false`.
    pub(in super::super) fn sample_peak_level(
        &self,
        drc_set_id: i8,
        downmix_id: u8,
        is_album_mode: bool,
        sample_peak_level: &mut f32,
    ) -> bool {
        let mut is_sample_peak_level_found = false;

        let (count, loud_info) = if is_album_mode {
            (self.li_album_count, &self.li_album[..])
        } else {
            (self.li_count, &self.loudness_info[..])
        };

        for li in loud_info.iter().take(usize::from(count)) {
            if li.drc_set_id == drc_set_id
                && li.downmix_id == downmix_id
                && li.is_sample_peak_level_present
            {
                *sample_peak_level = li.sample_peak_level;
                is_sample_peak_level_found = true;
                break;
            }
        }

        is_sample_peak_level_found
    }

    pub(in super::super) fn set_diff(&mut self, is_diff: bool) {
        self.is_diff = is_diff;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // --------------------------------------------------------------------- //
    // Test Rust implementation of LoudnessInfo::update_method_value(), compared with initial
    // version ported from C code. See drcDec_selectionProcess::_getLloudnes().
    fn update_method_value_c_ref(
        li: &mut LoudnessInfo,
        method_def: MethodDefinition,
        msrb: MeasurementSystemRequestBonus,
    ) -> MethodValueOrder {
        let mut value_order = MethodValueOrder::new();
        let mut start_idx = 0;

        // This loop is a Rust version of the C code originally written in
        // drcDec_selectionProcess::_getLloudnes().
        loop {
            let index_res = li.find_method_definition_index(method_def, start_idx);
            match index_res {
                Ok(i) => {
                    value_order = li.loudness_measurement[i].method_value(msrb, value_order);
                    start_idx = i + 1;
                }
                Err(_e) => {
                    break;
                }
            }
        }
        value_order
    }

    fn update_method_value_dut(
        li: &mut LoudnessInfo,
        method_def: MethodDefinition,
        msrb: MeasurementSystemRequestBonus,
    ) -> MethodValueOrder {
        li.update_method_value(method_def, msrb)
    }

    #[test]
    fn test_use_method_value() {
        // Hardcoded initialization. This serves only as proof of concept.
        // The selected definitions used for initialization are arbitrary.
        // An extended test should follow, by testing multiple values/combinations.
        let mut li = LoudnessInfo::new();
        const N: usize = 3;
        li.loudness_measurement[N].method_definition = MethodDefinition::AnchorLoudness;
        li.loudness_measurement[N].method_value = -24.0;
        li.loudness_measurement[N].measurement_system = MeasurementSystem::ExpertPanel;
        li.measurement_count = MAX_LOUDNESS_MEASUREMENTS;

        let find_method_def = MethodDefinition::AnchorLoudness;
        let msrb = MeasurementSystemRequestBonus::ExpertPanel;

        let mvo_dut = update_method_value_dut(&mut li, find_method_def, msrb);
        let mvo_c_ref = update_method_value_c_ref(&mut li, find_method_def, msrb);

        assert!(mvo_dut == mvo_c_ref);
    }
    // --------------------------------------------------------------------- //
}
