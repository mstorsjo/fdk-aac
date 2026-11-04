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
//! MPEG-D DRC's uniDRC instructions and downmix instructions.

use super::super::{
    common::{self, DrcBitstreamLocation, DuckingModification, EffectBit, GainModification},
    constants::*,
    drc_error::DrcError,
};
use super::DrcCoefficientsUniDrc;
use crate::common::bitstream::Bitstream;
use itertools::Itertools;

#[rustfmt::skip]
/// Downmix coefficients used in case of syntax version 0.
static  DOWNMIX_COEFF: [f32 ; 16] = [
    1.0000000000, 0.9440608763, 0.8912509381, 0.8413951416,
    0.7943282347, 0.7498942093, 0.7079457844, 0.6683439176,
    0.6309573445, 0.5956621435, 0.5623413252, 0.5308844442,
    0.5011872336, 0.4216965034, 0.3548133892, 0.0000000000];

#[rustfmt::skip]
/// Downmix coefficients used in case of syntax version 1.
static  DOWNMIX_COEFF_V1: [f32 ; 32] = [
    3.1622776602, 1.9952623150, 1.6788040181, 1.4125375446, 1.1885022274,
    1.0000000000, 0.9440608763, 0.8912509381, 0.8413951416, 0.7943282347,
    0.7498942093, 0.7079457844, 0.6683439176, 0.6309573445, 0.5956621435,
    0.5623413252, 0.5308844442, 0.5011872336, 0.4731512590, 0.4466835922,
    0.4216965034, 0.3981071706, 0.3548133892, 0.3162277660, 0.2818382931,
    0.2511886432, 0.1778279410, 0.1000000000, 0.0562341325, 0.0316227766,
    0.0100000000, 0.0000000000];

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// `DependsOnDrcSet` structure that holds `dependent DRC set identifier` or flag to indicate that
/// the DRC set can only be used in combination with another DRC set.
/// A DRC set is defined as a set of DRC sequences that produce a desired effect if applied to the
/// audio signal.
pub(in super::super) struct DependsOnDrcSet {
    /// `drc_set_id` of the DRC set this DRC depends on.
    depends_on_drc_set: i8,
    /// Flag indicates that the DRC set can only be used in combination with another DRC set as
    /// indicated by the `depends_on_drc_set` field.
    is_no_independent_use: bool,
}

impl DependsOnDrcSet {
    /// Creates a new instance.
    pub fn _new() -> Self {
        Default::default()
    }

    pub(in super::super) fn depends_on_drc_set(&self) -> i8 {
        self.depends_on_drc_set
    }

    pub(in super::super) fn is_no_independent_use(&self) -> bool {
        self.is_no_independent_use
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Modifications for channel group.
pub(in super::super) struct ModificationForChannel {
    /// Gain modification for channel group.
    pub(in super::super) gain_modification:
        [[GainModification; DRC_MAX_BANDS]; DRCDEC_MAX_CHANNELS],
    /// Ducking modification for channels.
    pub(in super::super) ducking_modification: [DuckingModification; DRCDEC_MAX_CHANNELS],
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Structure that holds payload information about one specific DRC set that can be applied to
/// achieve a desired effect.
pub(in super::super) struct DrcInstructionsUniDrc {
    /// An unique non-zero identifier for this DRC set. 0x0 and 0x3F are reserved. A DRC set is
    /// defined as a set of DRC sequences that produce a desired effect if applied to
    /// the audio signal.
    drc_set_id: i8,
    /// Value that indicates the computational complexity level of the associated DRC set per audio
    /// channel. The complexity level is used to determine if a decoder is capable of applying
    /// the DRC set.
    drc_set_complexity_level: u8,
    /// The DRC location describes where DRC gain sequences can be found in the bitstream.
    drc_location: DrcBitstreamLocation,
    /// Flag to indicate whether DRC set is applied to the downmix. If `false`, it is applied to
    /// the base layout.
    is_drc_apply_to_downmix: bool,
    /// Number of downmix IDs.
    downmix_id_count: u8,
    /// Identifier to identify a downmix that is associated with this DRC set.
    downmix_id: [u8; MAX_DOWNMIX_IDS as usize],
    /// Effect type of the DRC (16 bit value read from bitstream).
    drc_set_effect: u16,
    /// Flag to indicate presence of limiter peak target level.
    is_limiter_peak_target_present: bool,
    /// Limiter peak target level used by the encoder-side DRC (dBFS).
    limiter_peak_target: f32,
    /// Flag to inidcate presence of target loudness for DRC set.
    is_drc_set_target_loudness_present: bool,
    /// Upper limit of the target loudness of a DRC set. The default value is
    /// `DRC_TARGET_LOUDNESS_MAX_VALUE` dB.
    drc_set_target_loudness_value_upper: i8,
    /// Lower limit of the target loudness of a DRC set. The default value is
    /// `DRC_TARGET_LOUDNESS_MIN_VALUE` dB.
    drc_set_target_loudness_value_lower: i8,
    /// Flag to indicate presence of `depends_on_drc_set` field.
    depends_on_drc_set_present: bool,
    /// Indicates the `drc_set_id` of the DRC set this DRC depends on.
    depends_on_drc_set: DependsOnDrcSet,
    /// Flag to indicate DRC set can only be used in combination with an associated EQ set.
    is_eq_required: bool,
    /// Modifications for channel & channel group. The Ducking modification is only used on the
    /// channel, but the gain modifications is used on the channel group. A collection of channels
    /// assigned to the same DRC gain sequence is called `channel group`.
    pub(in super::super) modification_for_channel_group: ModificationForChannel,
    /// A unique number which specifies which DRC gain set should be assigned to which channel of
    /// the configuration specified in `downmix_id`.
    gain_set_index: [i8; DRCDEC_MAX_CHANNELS],
    /// Number of channels.
    drc_channel_count: u8,
    /// Number of channel groups. A channel group is a collection of channels assigned to the same
    /// DRC gain set. Maximum: `DRCDEC_MAX_CHANNELS`.
    num_drc_channel_groups: u8,
    /// Gain set indices for channel groups.
    gain_set_index_for_channel_group: [i8; DRCDEC_MAX_CHANNELS],
}

impl DrcInstructionsUniDrc {
    /// Creates a new instance of `DrcInstructionsUniDrc`.
    pub(in super::super) fn _new() -> Self {
        Default::default()
    }

    /// Clears the `DrcInstructionsUniDrc` instance.
    pub(in super::super) fn clear(&mut self) {
        self.drc_set_id = 0;
        self.drc_set_complexity_level = 0;
        self.drc_location = DrcBitstreamLocation::Undefined;
        self.is_drc_apply_to_downmix = false;
        self.downmix_id_count = 0;
        self.downmix_id = [0_u8; 8];
        self.drc_set_effect = 0;
        self.is_limiter_peak_target_present = false;
        self.limiter_peak_target = 0.0;
        self.is_drc_set_target_loudness_present = false;
        self.drc_set_target_loudness_value_upper = DRC_TARGET_LOUDNESS_MAX_VALUE;
        self.drc_set_target_loudness_value_lower = DRC_TARGET_LOUDNESS_MAX_VALUE;
        self.depends_on_drc_set_present = false;
        self.depends_on_drc_set = Default::default();
        self.is_eq_required = false;
        self.modification_for_channel_group = Default::default();
        self.gain_set_index = [0_i8; DRCDEC_MAX_CHANNELS];
        self.drc_channel_count = 0;
        self.num_drc_channel_groups = 0;
        self.gain_set_index_for_channel_group = [0_i8; DRCDEC_MAX_CHANNELS];
    }

    pub(in super::super) fn drc_channel_count(&self) -> u8 {
        self.drc_channel_count
    }

    /// Returns `DRC set identifier`. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    pub(in super::super) fn drc_set_id(&self) -> i8 {
        self.drc_set_id
    }

    /// Returns the target loudness upper value for a DRC set.
    pub(in super::super) fn drc_set_target_loudness_value_upper(&self) -> i8 {
        self.drc_set_target_loudness_value_upper
    }

    /// Returns the target loudness upper value for a DRC set.
    pub(in super::super) fn drc_set_target_loudness_value_lower(&self) -> i8 {
        self.drc_set_target_loudness_value_lower
    }

    /// Number of downmix IDs.
    pub(in super::super) fn downmix_id_count(&self) -> u8 {
        self.downmix_id_count
    }

    /// Returns `DRC number of channel groups`.
    pub(in super::super) fn num_drc_channel_groups(&self) -> u8 {
        self.num_drc_channel_groups
    }

    pub(in super::super) fn gain_set_index(&self) -> &[i8; DRCDEC_MAX_CHANNELS] {
        &self.gain_set_index
    }

    /// Returns `DRC gain set indices for channel groups`.
    pub(in super::super) fn gain_set_index_for_channel_group(&self) -> &[i8] {
        &self.gain_set_index_for_channel_group
    }

    /// Returns the DRC effect value belonging to a particular DRC set.
    pub(in super::super) fn drc_set_effect(&self) -> u16 {
        self.drc_set_effect
    }

    /// Returns `DRC location`.
    pub(in super::super) fn drc_location(&self) -> DrcBitstreamLocation {
        self.drc_location
    }

    /// Returns status of flag `self.is_drc_apply_to_downmix`.
    pub(in super::super) fn is_drc_apply_to_downmix(&self) -> bool {
        self.is_drc_apply_to_downmix
    }

    /// Returns status of flag `self.is_drc_set_target_loudness_present`.
    pub(in super::super) fn is_drc_set_target_loudness_present(&self) -> bool {
        self.is_drc_set_target_loudness_present
    }

    /// Returns status of flag `self.depends_on_drc_set_present`.
    pub(in super::super) fn depends_on_drc_set_present(&self) -> bool {
        self.depends_on_drc_set_present
    }

    /// Returns dependent DRC set identifier.
    pub(in super::super) fn depends_on_drc_set(&self) -> DependsOnDrcSet {
        self.depends_on_drc_set
    }

    /// Returns status of flag `self.is_required_eq`.
    pub(in super::super) fn is_eq_required(&self) -> bool {
        self.is_eq_required
    }

    pub(in super::super) fn is_limiter_peak_target_present(&self) -> bool {
        self.is_limiter_peak_target_present
    }

    /// Returns `self.downmix_id[]`. The length of slice is limited to `self.downmix_id_count`.
    pub(in super::super) fn downmix_ids(&self) -> &[u8] {
        &self.downmix_id[..self.downmix_id_count as usize]
    }

    /// Returns the Downmix ID at the `index` position.
    pub(in super::super) fn downmix_id_at(&self, index: usize) -> u8 {
        self.downmix_id[index]
    }

    /// Generates a virtual DRC set. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    /// # Parameters
    /// - `downmix_id_0`:
    /// - `index_virtual`: `DRC set identifier` for the virtual DRC set.
    pub(in super::super) fn generate_virtual_drc_set(
        &mut self,
        downmix_id_0: u8,
        index_virtual: i8,
    ) {
        self.clear();
        self.drc_set_id = index_virtual;
        self.downmix_id[0] = downmix_id_0;
        self.downmix_id_count = 1;
    }

    /// Returns `true` if `target_loudness` in range. Otherwise, `false`.
    ///
    /// # Parameters
    ///
    /// - `target_loudness`: Target loudness value.
    pub(in super::super) fn is_target_loudness_in_range(&self, target_loudness: f32) -> bool {
        let upper = f32::from(self.drc_set_target_loudness_value_upper);
        let lower = f32::from(self.drc_set_target_loudness_value_lower);

        self.is_drc_set_target_loudness_present
            && upper >= target_loudness
            && lower < target_loudness
    }

    /// Reads `gain modification` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    /// - `band_count`: Number of bands.
    /// - `channel_group`: Channel group index.
    fn decode_gain_modification_for_channel_group(
        &mut self,
        bs: &mut Bitstream,
        version: u8,
        band_count: usize,
        channel_group: usize,
    ) {
        let gain_modification =
            &mut self.modification_for_channel_group.gain_modification[channel_group];

        if version > 0 {
            for gain_mod in gain_modification.iter_mut().take(band_count) {
                gain_mod.decode(bs);
            }
            if band_count == 1 {
                let is_shape_filter_present = bs.read_bit() != 0;
                if is_shape_filter_present {
                    // Push 4 bits forward in bitstream - shape_filter_index.
                    bs.push(4);
                }
            }
        } else {
            let mut attenuation_scaling = 1.0_f32;
            let mut amplification_scaling = 1.0_f32;
            let mut gain_offset = 0.0_f32;
            let is_gain_scaling_present = bs.read_bit() != 0;

            if is_gain_scaling_present {
                let bs_attenuation_scaling = bs.read(4);
                attenuation_scaling = bs_attenuation_scaling as f32 * 0.125_f32;
                let bs_amplification_scaling = bs.read(4);
                amplification_scaling = bs_amplification_scaling as f32 * 0.125_f32;
            }

            let is_gain_offset_present = bs.read_bit() != 0;

            if is_gain_offset_present {
                let is_sign = bs.read_bit() != 0;
                let bs_gain_offset = bs.read(5);
                gain_offset = (1 + bs_gain_offset) as f32 * 0.25_f32;
                if is_sign {
                    gain_offset = -gain_offset;
                }
            }

            for gain_mod in gain_modification.iter_mut().take(DRC_MAX_BANDS) {
                gain_mod.set_is_target_characteristic_left_present(false);
                gain_mod.set_is_target_characteristic_right_present(false);
                gain_mod.set_is_gain_scaling_present(is_gain_scaling_present);
                gain_mod.set_attenuation_scaling(attenuation_scaling);
                gain_mod.set_amplification_scaling(amplification_scaling);
                gain_mod.set_is_gain_offset_present(is_gain_offset_present);
                gain_mod.set_gain_offset(gain_offset);
            }
        }
    }

    pub(in super::super) fn gain_modification_for_channel_group(
        &self,
    ) -> &[[GainModification; DRC_MAX_BANDS]; DRCDEC_MAX_CHANNELS] {
        &self.modification_for_channel_group.gain_modification
    }

    /// Gets limiter peak target level value, depending on provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `downmix_id`: An unique non-zero downmix identifier.
    /// - `limiter_peak_target`: Output value, if `downmix_id` is a match.
    ///
    /// # Return
    ///
    /// - `is_limiter_peak_target_found`
    pub(in super::super) fn get_limiter_peak_target(
        &self,
        downmix_id: u8,
        limiter_peak_target: &mut f32,
    ) -> bool {
        if self.is_limiter_peak_target_present {
            let mut dmx_iter = self
                .downmix_id
                .iter()
                .take(usize::from(self.downmix_id_count));
            let dmx_id0 = dmx_iter.next().unwrap();

            if *dmx_id0 == downmix_id || *dmx_id0 == 0x7F {
                *limiter_peak_target = self.limiter_peak_target;
                // is_limiter_peak_target_found
                return true;
            }

            if dmx_iter.contains(&downmix_id) {
                *limiter_peak_target = self.limiter_peak_target;
                // is_limiter_peak_target_found
                return true;
            }
        }
        // !is_limiter_peak_target_found
        false
    }

    pub(in super::super) fn limiter_peak_target(&self) -> f32 {
        self.limiter_peak_target
    }

    /// Reads `DrcInstructionsUniDrc` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `downmix_instructions`: Immutable reference to buffer of `DownmixInstructions` instance.
    /// - `drc_coefficients`: Immutable reference to buffer of `DrcCoefficientsUniDrc` instance.
    /// - `downmix_instructions_count`: Number of downmix instructions in uniDRC config.
    /// - `drc_coefficients_count`: Number of valid DRC coefficients in uniDRC config.
    /// - `base_channel_count`: Number of base channels in channel layout.
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    ///
    ///  Returns `DrcError`.
    #[expect(clippy::too_many_arguments)]
    pub(in super::super) fn read(
        &mut self,
        bs: &mut Bitstream,
        downmix_instructions: &[DownmixInstructions; MAX_DOWNMIX_INSTRUCTIONS as usize],
        drc_coefficients: &[DrcCoefficientsUniDrc; MAX_DRC_COEFFICIENTS],
        downmix_instructions_count: u8,
        drc_coefficients_count: u8,
        base_channel_count: u8,
        version: u8,
    ) -> Result<(), DrcError> {
        let mut channel_group_for_channel = [0_i8; DRCDEC_MAX_CHANNELS];
        let mut ducking_modification_for_channel_group =
            [DuckingModification::default(); DRCDEC_MAX_CHANNELS];

        self.drc_set_id = bs.read(6) as i8;
        self.drc_set_complexity_level = if version == 0 {
            // Assume all v0 DRC sets to be manageable in terms of complexity.
            2
        } else {
            bs.read(4) as u8
        };

        self.drc_location = DrcBitstreamLocation::from(bs.read(4) as u8);

        let is_downmix_id_present = if version == 0 {
            true
        } else {
            bs.read_bit() != 0
        };

        if is_downmix_id_present {
            self.downmix_id[0] = bs.read(7) as u8;
            self.is_drc_apply_to_downmix = if version == 0 {
                self.downmix_id[0] != 0
            } else {
                bs.read_bit() != 0
            };

            let is_additional_downmix_id_present = bs.read_bit() != 0;
            if is_additional_downmix_id_present {
                let additional_downmix_id_count = bs.read(3) as u8;
                for downmix_id in self
                    .downmix_id
                    .iter_mut()
                    .skip(1)
                    .take(usize::from(additional_downmix_id_count))
                {
                    *downmix_id = bs.read(7) as u8;
                }

                self.downmix_id_count = 1 + additional_downmix_id_count;
            } else {
                self.downmix_id_count = 1;
            }
        } else {
            self.downmix_id[0] = 0;
            self.downmix_id_count = 1;
        }

        let drc_set_effect = bs.read(16) as u16;
        self.drc_set_effect = drc_set_effect;

        if drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf) == 0 {
            self.is_limiter_peak_target_present = bs.read_bit() != 0;

            if self.is_limiter_peak_target_present {
                let bs_limiter_peak_target = bs.read(8) as i32;
                self.limiter_peak_target = (-bs_limiter_peak_target) as f32 * 0.125_f32;
            }
        }

        self.is_drc_set_target_loudness_present = bs.read_bit() != 0;

        // Set default values.
        self.drc_set_target_loudness_value_upper = DRC_TARGET_LOUDNESS_MAX_VALUE;
        self.drc_set_target_loudness_value_lower = DRC_TARGET_LOUDNESS_MIN_VALUE;

        if self.is_drc_set_target_loudness_present {
            let bs_drc_set_target_loudness_value_upper = bs.read(6) as i8;
            self.drc_set_target_loudness_value_upper = bs_drc_set_target_loudness_value_upper - 63;

            let is_lower_loudness_present = bs.read_bit() != 0;
            if is_lower_loudness_present {
                let bs_drc_set_target_loudness_value_lower = bs.read(6) as i8;
                self.drc_set_target_loudness_value_lower =
                    bs_drc_set_target_loudness_value_lower - 63;
            }
        }

        self.depends_on_drc_set_present = bs.read_bit() != 0;

        self.depends_on_drc_set.is_no_independent_use = false;
        if self.depends_on_drc_set_present {
            self.depends_on_drc_set.depends_on_drc_set = bs.read(6) as i8;
        } else {
            self.depends_on_drc_set.is_no_independent_use = bs.read_bit() != 0;
        }

        self.is_eq_required = if version == 0 {
            false
        } else {
            bs.read_bit() != 0
        };

        let coeff_uni_drc = drc_coefficients
            .iter()
            .take(usize::from(drc_coefficients_count))
            .find(|&coeff_uni_drc| coeff_uni_drc.drc_location() == self.drc_location);

        self.drc_channel_count = base_channel_count;
        let mut channel_count = usize::from(self.drc_channel_count);

        if drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf) != 0 {
            let mut c = 0;
            let ducking_modification =
                &mut self.modification_for_channel_group.ducking_modification;
            while c < channel_count {
                let bs_gain_set_index = bs.read(6) as i8;

                if c >= DRCDEC_MAX_CHANNELS {
                    return Err(DrcError::MemoryError);
                }

                self.gain_set_index[c] = bs_gain_set_index - 1;

                ducking_modification[c].decode_ducking_modification(bs);

                c += 1;

                let is_repeat_parameters = bs.read_bit() != 0;
                if is_repeat_parameters {
                    let bs_repeat_parameters_count = bs.read(5) + 1;
                    for _i in 0..bs_repeat_parameters_count {
                        if c >= DRCDEC_MAX_CHANNELS {
                            return Err(DrcError::MemoryError);
                        }
                        self.gain_set_index[c] = self.gain_set_index[c - 1];
                        ducking_modification[c] = ducking_modification[c - 1];
                        c += 1;
                    }
                }
            }

            if c > channel_count {
                return Err(DrcError::NotOk);
            }

            common::derive_drc_channel_groups(
                Some(&self.modification_for_channel_group.ducking_modification),
                Some(&mut ducking_modification_for_channel_group),
                &self.gain_set_index,
                &mut self.gain_set_index_for_channel_group,
                &mut channel_group_for_channel,
                &mut self.num_drc_channel_groups,
                usize::from(self.drc_channel_count),
                self.drc_set_effect,
            )?;
        } else {
            let mut is_derive_channel_count = false;

            if (version == 0 || self.is_drc_apply_to_downmix)
                && self.downmix_id[0] != DOWNMIX_ID_BASE_LAYOUT
                && self.downmix_id[0] != DOWNMIX_ID_ANY_DOWNMIX
                && self.downmix_id_count == 1
            {
                if downmix_instructions_count != 0 {
                    let down_inst = downmix_instructions
                        .iter()
                        .take(usize::from(downmix_instructions_count))
                        .find(|&downmix_instru| downmix_instru.downmix_id() == self.downmix_id[0]);

                    if let Some(down) = down_inst {
                        // Target channel count from downmix_id.
                        self.drc_channel_count = down.target_channel_count();
                        channel_count = usize::from(self.drc_channel_count);
                    } else {
                        return Err(DrcError::NotOk);
                    }
                } else {
                    is_derive_channel_count = true;
                    channel_count = 1;
                }
            } else if (version == 0 || self.is_drc_apply_to_downmix)
                && (self.downmix_id[0] == DOWNMIX_ID_ANY_DOWNMIX || self.downmix_id_count > 1)
            {
                // Set maximum channel count as upper border. The effective channel count
                // is set at the process function.
                self.drc_channel_count = DRCDEC_MAX_CHANNELS as u8;
                channel_count = 1;
            }

            let mut c = 0;
            while c < channel_count {
                let bs_gain_set_index = bs.read(6) as i8;
                if c >= DRCDEC_MAX_CHANNELS {
                    return Err(DrcError::MemoryError);
                }
                self.gain_set_index[c] = bs_gain_set_index - 1;
                c += 1;

                let is_repeat_sequence_index = bs.read_bit() != 0;

                if is_repeat_sequence_index {
                    let bs_repeat_sequence_count = bs.read(5) as usize + 1;
                    if is_derive_channel_count {
                        channel_count = 1 + bs_repeat_sequence_count;
                    }

                    for _i in 0..bs_repeat_sequence_count {
                        if c >= DRCDEC_MAX_CHANNELS {
                            return Err(DrcError::MemoryError);
                        }
                        self.gain_set_index[c] = bs_gain_set_index - 1;
                        c += 1;
                    }
                }
            }
            if c > channel_count {
                return Err(DrcError::NotOk);
            }

            if is_derive_channel_count {
                self.drc_channel_count = channel_count as u8;
            }

            // DOWNMIX_ID_ANY_DOWNMIX: channelCount is > 1. Distribute gain_set_index to all
            // channels.
            if self.downmix_id[0] == DOWNMIX_ID_ANY_DOWNMIX || self.downmix_id_count > 1 {
                let value = self.gain_set_index[0];
                self.gain_set_index[..usize::from(self.drc_channel_count)].fill(value);
            }

            common::derive_drc_channel_groups(
                None,
                None,
                &self.gain_set_index,
                &mut self.gain_set_index_for_channel_group,
                &mut channel_group_for_channel,
                &mut self.num_drc_channel_groups,
                usize::from(self.drc_channel_count),
                self.drc_set_effect,
            )?;

            for channel_group in 0..usize::from(self.num_drc_channel_groups) {
                let set_index = self.gain_set_index_for_channel_group[channel_group] as usize;
                // Get band count.
                let band_count = if let Some(coeff) = coeff_uni_drc {
                    if set_index < usize::from(coeff.gain_set_count()) {
                        coeff.gain_set(set_index).band_count()
                    } else {
                        1
                    }
                } else {
                    1
                };

                self.decode_gain_modification_for_channel_group(
                    bs,
                    version,
                    usize::from(band_count),
                    channel_group,
                );
            }
        }

        Ok(())
    }

    /// Sets the ID used to identify a downmix that is associated with this DRC set.
    /// # Parameters
    /// - `index`: DRC set index (Up to 8).
    /// - `dmx_id`: ID value
    pub(in super::super) fn _set_downmix_id(&mut self, index: usize, dmx_id: u8) {
        self.downmix_id[index] = dmx_id;
    }

    /// Sets a `DRC set identifier`. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    pub(in super::super) fn set_drc_set_id(&mut self, drc_set_id: i8) {
        self.drc_set_id = drc_set_id;
    }

    /// Sets the downmix ID count.
    pub(in super::super) fn _set_downmix_id_count(&mut self, dmx_id_count: u8) {
        self.downmix_id_count = dmx_id_count;
    }

    /// Sets whether DRC is applied to downmix.
    /// The value can be evaluated by calling `is_drc_apply_to_downmix()`.
    pub(in super::super) fn set_drc_apply_to_downmix(&mut self, drc_apply_to_dmx_flag: bool) {
        self.is_drc_apply_to_downmix = drc_apply_to_dmx_flag;
    }
}

#[repr(C)]
#[derive(Debug, PartialEq)]
/// `DownmixInstructions` structure that holds metadata and coefficients required for downmixing
/// audio channels to target layout.
pub(in super::super) struct DownmixInstructions {
    /// A unique non-zero downmix identifier.
    pub(super) downmix_id: u8,
    /// Number of output channels after downmixing.
    target_channel_count: u8,
    /// Target layout of output channels.
    target_layout: u8,
    /// Flag to indicate presence of downmix coefficients.
    is_downmix_coefficients_present: bool,
    /// Offset value.
    bs_downmix_offset: u8,
    /// Indices to read coefficients from `DOWNMIX_COEFF[]` / `DOWNMIX_COEFF_V1[]` table.
    downmix_coefficient: [f32; DRCDEC_MAX_CHANNELS * DRCDEC_MAX_CHANNELS],
}

/// Default trait for `DownmixInstructions`.
impl Default for DownmixInstructions {
    fn default() -> Self {
        DownmixInstructions {
            downmix_id: 0,
            target_channel_count: 0,
            target_layout: 0,
            is_downmix_coefficients_present: false,
            bs_downmix_offset: 0,
            downmix_coefficient: [0.0_f32; DRCDEC_MAX_CHANNELS * DRCDEC_MAX_CHANNELS],
        }
    }
}

impl DownmixInstructions {
    /// Returns a unique downmix identifier.
    pub(in super::super) fn downmix_id(&self) -> u8 {
        self.downmix_id
    }

    /// Returns if downmix coefficients is present or not.
    pub(in super::super) fn is_downmix_coefficients_present(&self) -> bool {
        self.is_downmix_coefficients_present
    }

    /// Returns number of target output channels.
    pub(in super::super) fn target_channel_count(&self) -> u8 {
        self.target_channel_count
    }

    /// Returns target layout.
    pub(in super::super) fn target_layout(&self) -> u8 {
        self.target_layout
    }

    /// Returns `bs_downmix_offset` value.
    pub(in super::super) fn bs_downmix_offset(&self) -> u8 {
        self.bs_downmix_offset
    }

    /// Returns `downmix_coefficient[]` value.
    pub(in super::super) fn downmix_coefficient(
        &self,
    ) -> &[f32; DRCDEC_MAX_CHANNELS * DRCDEC_MAX_CHANNELS] {
        &self.downmix_coefficient
    }

    /// Reads `DownmixInstructions` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `base_channel_count`: Number of base channels in channel layout.
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read(
        &mut self,
        bs: &mut Bitstream,
        base_channel_count: u8,
        version: u8,
    ) -> Result<(), DrcError> {
        self.downmix_id = bs.read(7) as u8;
        self.target_channel_count = bs.read(7) as u8;
        self.target_layout = bs.read(8) as u8;
        self.is_downmix_coefficients_present = bs.read_bit() != 0;

        if self.is_downmix_coefficients_present {
            let coeff_count =
                usize::from(self.target_channel_count) * usize::from(base_channel_count);

            if coeff_count > DRCDEC_MAX_CHANNELS * DRCDEC_MAX_CHANNELS {
                return Err(DrcError::NotOk);
            }

            if version == 0 {
                self.bs_downmix_offset = 0;

                for dc in self.downmix_coefficient.iter_mut().take(coeff_count) {
                    // LFE downmix coefficients are not supported.
                    *dc = DOWNMIX_COEFF[bs.read(4) as usize];
                }
            } else {
                self.bs_downmix_offset = bs.read(4) as u8;

                for dc in self.downmix_coefficient.iter_mut().take(coeff_count) {
                    // LFE downmix coefficients are not supported.
                    *dc = DOWNMIX_COEFF_V1[bs.read(5) as usize];
                }
            }
        }

        Ok(())
    }
}
