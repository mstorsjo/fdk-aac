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
//! MPEG-D DRC's uniDRC configuration and its extension.

use super::super::{
    common::{comp_assign, DrcBitstreamLocation, EffectBit},
    constants::*,
    drc_error::DrcError,
};
use super::{DownmixInstructions, DrcCoefficientsUniDrc, DrcInstructionsUniDrc};
use crate::common::bitstream::Bitstream;

/// Maximum number of DRC downmix coefficients.
const DRCDEC_MAX_DOWNMIX_COEFFICIENTS: u8 = 2;
/// uniDRC configuration extension type for termination tag.
const UNIDRCCONFEXT_TERM: u8 = 0;
/// uniDRC configuration extension type for parametric DRC.
const UNIDRCCONFEXT_PARAM_DRC: u8 = 1;
/// uniDRC configuration extension type for efficient multi-band DRC coding, dynamic EQ and
/// loudness EQ.
const UNIDRCCONFEXT_V1: u8 = 2;

#[repr(C)]
#[derive(Debug)]
/// Types of EQ subband gain format.
enum EqSubbandGainFormat {
    Qmf32 = 0x1,
    QmfHybrid39 = 0x2,
    Qmf64 = 0x3,
    QmfHybrid71 = 0x4,
    Qmf128 = 0x5,
    QmfHybrid135 = 0x6,
    Uniform = 0x7,
    DefaultBsRead, // Just to indicate default case.
}

/// `From<u8>` trait for `EqSubbandGainFormat`.
impl From<u8> for EqSubbandGainFormat {
    fn from(value: u8) -> Self {
        match value {
            1 => EqSubbandGainFormat::Qmf32,
            2 => EqSubbandGainFormat::QmfHybrid39,
            3 => EqSubbandGainFormat::Qmf64,
            4 => EqSubbandGainFormat::QmfHybrid71,
            5 => EqSubbandGainFormat::Qmf128,
            6 => EqSubbandGainFormat::QmfHybrid135,
            7 => EqSubbandGainFormat::Uniform,
            _ => EqSubbandGainFormat::DefaultBsRead,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, PartialEq)]
/// `ChannelLayout` structure that holds metadata for base layout of audio signals / contents.
pub(in super::super) struct ChannelLayout {
    /// Total number of base channels.
    base_channel_count: u8,
    /// Flag to indicate presence of base layout.
    is_layout_signaling_present: bool,
    /// Channel layout configuration.
    defined_layout: u8,
    /// Output channel position.
    speaker_position: [u8; DRCDEC_MAX_CHANNELS],
}

impl ChannelLayout {
    /// Returns number of channels in the base layout / original audio input.
    pub(in super::super) fn base_channel_count(&self) -> u8 {
        self.base_channel_count
    }

    /// Reads `ChannelLayout` metadata from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        self.base_channel_count = bs.read(7) as u8;

        if self.base_channel_count > DRCDEC_MAX_CHANNELS as u8 {
            return Err(DrcError::NotOk);
        }

        self.is_layout_signaling_present = bs.read_bit() != 0;

        if self.is_layout_signaling_present {
            self.defined_layout = bs.read(8) as u8;

            if self.defined_layout == 0 {
                for sp in self
                    .speaker_position
                    .iter_mut()
                    .take(usize::from(self.base_channel_count).min(DRCDEC_MAX_CHANNELS))
                {
                    *sp = bs.read(7) as u8;
                }

                if self.base_channel_count > DRCDEC_MAX_CHANNELS as u8 {
                    for _i in DRCDEC_MAX_CHANNELS as u8..self.base_channel_count {
                        bs.push(7);
                    }
                }
            }
        }

        Ok(())
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// `UniDrcConfigExtension` structure that holds extension type and bit size.
struct UniDrcConfigExtension {
    /// Extension type.
    uni_drc_config_ext_type: [u8; MAX_EXTENSIONS],
    /// Extension bit size.
    ext_bit_size: [u32; MAX_EXTENSIONS - 1],
}

#[repr(C)]
#[derive(Default, Debug)]
/// `UniDrcConfig` structure that holds DRC configuration information.
pub struct UniDrcConfig {
    /// Flag to indicate presence of audio sample rate in bitstream.
    is_sample_rate_present: bool,
    /// Audio sample rate in Hz.
    sample_rate: u32,
    /// Number of `DownmixInstructions` blocks in case V0.
    downmix_instructions_count_v0: u8,
    /// Number of `DownmixInstructions` blocks in case V1.
    downmix_instructions_count_v1: u8,
    /// Total number of `DownmixInstructions` blocks.
    downmix_instructions_count: u8,
    /// Number of `DrcCoefficientsUniDrc` blocks in case V0.
    drc_coefficients_uni_drc_count_v0: u8,
    /// Number of `DrcCoefficientsUniDrc` blocks in case V1.
    drc_coefficients_uni_drc_count_v1: u8,
    /// Total number of `DrcCoefficientsUniDrc` blocks.
    drc_coefficients_uni_drc_count: u8,
    /// Number of `DrcInstructionsUniDrc` blocks in case V0.
    drc_instructions_uni_drc_count_v0: u8,
    /// Number of `DrcInstructionsUniDrc` blocks in case V1.
    drc_instructions_uni_drc_count_v1: u8,
    /// Total number of `DrcInstructionsUniDrc` blocks.
    drc_instructions_uni_drc_count: u8,
    /// Audio channels base layout.
    channel_layout: ChannelLayout,
    /// DRC downmix instructions.
    downmix_instructions: [DownmixInstructions; MAX_DOWNMIX_INSTRUCTIONS as usize],
    /// DRC coefficients.
    drc_coefficients_uni_drc: [DrcCoefficientsUniDrc; MAX_DRC_COEFFICIENTS],
    /// DRC instructions.
    drc_instructions_uni_drc:
        [DrcInstructionsUniDrc; (MAX_DRC_INSTRUCTIONS + 1 + MAX_DOWNMIX_INSTRUCTIONS) as usize],
    /// Flag to indicate presence of DRC config extension in bitstream.
    is_uni_drc_config_ext_present: bool,
    /// uniDRC config extension payload.
    uni_drc_config_ext: UniDrcConfigExtension,
    /// Total number of DRC instructions including virtual DRC sets.
    drc_instructions_count_incl_virtual: u8,
    is_diff: bool,
}

impl UniDrcConfig {
    /// Returns immutable reference to `ChannelLayout`
    pub(in super::super) fn channel_layout(&self) -> &ChannelLayout {
        &self.channel_layout
    }

    /// Returns `true` if downmix coefficients are present for given `downmix_id`, Otherwise
    ///  `false`.
    ///
    /// # Parameters
    /// - `downmix_id`: Downmix ID value.
    /// - `index`: To store index value of downmix instructions with downmix coefficients for given
    ///   `downmix_id`.
    pub(in super::super) fn downmix_coefficients_are_present(
        &self,
        downmix_id: u8,
        index: &mut usize,
    ) -> bool {
        let mut is_dmx_coeff_present = false;

        *index = 0;
        for (i, dmx_instr) in self
            .downmix_instructions
            .iter()
            .enumerate()
            .take(usize::from(self.downmix_instructions_count))
        {
            if (*dmx_instr).downmix_id() == downmix_id
                && (*dmx_instr).is_downmix_coefficients_present()
            {
                if (*dmx_instr).target_channel_count() > DRCDEC_MAX_CHANNELS as u8 {
                    break;
                }
                *index = i;
                is_dmx_coeff_present = true;
                break;
            }
        }
        is_dmx_coeff_present
    }

    /// Returns downmix instructions count.
    pub(in super::super) fn downmix_instructions_count(&self) -> u8 {
        self.downmix_instructions_count
    }

    /// Returns total number of `DrcInstructionsUniDrc` blocks.
    pub(in super::super) fn drc_instructions_uni_drc_count(&self) -> u8 {
        self.drc_instructions_uni_drc_count
    }

    /// Returns total number of DRC instructions including virtual DRC sets.
    pub(in super::super) fn drc_instructions_count_incl_virtual(&self) -> u8 {
        self.drc_instructions_count_incl_virtual
    }

    /// Returns `self.drc_instructions_uni_drc[]`.
    pub(in super::super) fn drc_instructions_uni_drc(&self) -> &[DrcInstructionsUniDrc] {
        &self.drc_instructions_uni_drc[..]
    }

    /// Returns total number of `DrcCoefficientsUniDrc` blocks.
    pub(in super::super) fn drc_coefficients_uni_drc_count(&self) -> u8 {
        self.drc_coefficients_uni_drc_count
    }
    /// Returns `self.downmix_instructions[]`.
    pub(in super::super) fn downmix_instructions(&self) -> &[DownmixInstructions] {
        &self.downmix_instructions[..]
    }

    /// Returns downmix IDs of `DownmixInstructions` in `UniDrcConfig` that has matching target
    /// layout.
    ///
    /// # Parameters
    /// - `target_layout_requested`: Requested target layout.
    ///
    /// # Return
    /// - `(Option<[u8; MAX_DOWNMIX_INSTRUCTIONS]>, usize)`. Where `usize` indicates number of valid
    ///   downmix IDs in output buffer.
    pub(in super::super) fn downmix_ids_based_on_target_layout(
        &self,
        target_layout_requested: u8,
    ) -> (Option<[u8; MAX_DOWNMIX_INSTRUCTIONS as usize]>, usize) {
        let mut downmix_ids = [0_u8; MAX_DOWNMIX_INSTRUCTIONS as usize];
        let mut length = 0;

        for dmx_instr in self
            .downmix_instructions
            .iter()
            .take(usize::from(self.downmix_instructions_count))
        {
            if dmx_instr.target_layout() == target_layout_requested {
                downmix_ids[length] = dmx_instr.downmix_id();
                length += 1;
            }
        }

        if length == 0 {
            (None, 0)
        } else {
            (Some(downmix_ids), length)
        }
    }

    /// Returns downmix IDs of `DownmixInstructions` in `UniDrcConfig` that has matching target
    /// channel count.
    ///
    /// # Parameters
    /// - `target_channel_count`: Target channel count.
    ///
    /// # Return
    /// - `(Option<[u8; MAX_DOWNMIX_INSTRUCTIONS]>, usize)`. Where `usize` indicates number of valid
    ///   downmix IDs in output buffer.
    pub(in super::super) fn downmix_ids_based_on_target_channel_count(
        &self,
        target_channel_count: u8,
    ) -> (Option<[u8; MAX_DOWNMIX_INSTRUCTIONS as usize]>, usize) {
        let mut downmix_ids = [0_u8; MAX_DOWNMIX_INSTRUCTIONS as usize];
        let mut length = 0;

        for dmx_instr in self
            .downmix_instructions
            .iter()
            .take(usize::from(self.downmix_instructions_count))
        {
            if dmx_instr.target_channel_count() == target_channel_count {
                downmix_ids[length] = dmx_instr.downmix_id();
                length += 1;
            }
        }

        if length == 0 {
            (None, 0)
        } else {
            (Some(downmix_ids), length)
        }
    }

    /// Generates virtual DRC sets.
    /// # Return
    /// - `is_ok`: true, if the DRC sets were generated successful.
    pub(in super::super) fn generate_virtual_drc_sets(&mut self) -> bool {
        let mut is_ok = true;
        let num_mixes = self.downmix_instructions_count + 1;
        let mut index_virtual = -1;

        if self.drc_instructions_uni_drc_count + num_mixes
            > MAX_DRC_INSTRUCTIONS + 1 + MAX_DOWNMIX_INSTRUCTIONS
        {
            is_ok = false;
            return is_ok;
        }

        let mut drc_instr_iter = self
            .drc_instructions_uni_drc
            .iter_mut()
            .skip(usize::from(self.drc_instructions_uni_drc_count));

        drc_instr_iter
            .next()
            .unwrap()
            .generate_virtual_drc_set(DOWNMIX_ID_BASE_LAYOUT, index_virtual);
        index_virtual -= 1;

        for dmx_instr_iter in self
            .downmix_instructions
            .iter()
            .take(usize::from(num_mixes - 1))
        {
            drc_instr_iter
                .next()
                .unwrap()
                .generate_virtual_drc_set(dmx_instr_iter.downmix_id(), index_virtual);
            index_virtual -= 1;
        }

        self.drc_instructions_count_incl_virtual = self.drc_instructions_uni_drc_count + num_mixes;

        is_ok
    }

    /// Returns `Some(&DrcInstructionsUniDrc)` if dependent drc instruction found. Otherwise `None`.
    ///
    /// # Parameters
    /// - `depends_on_drc_set_id`: The DRC set ID that the output might depend on.
    ///
    ///  Returns `Option<&DrcInstructionsUniDrc>`.
    pub(in super::super) fn get_dependent_drc_instruction(
        &self,
        depends_on_drc_set_id: i8,
    ) -> Option<&DrcInstructionsUniDrc> {
        self.drc_instructions_uni_drc
            .iter()
            .take(self.drc_instructions_uni_drc_count as usize)
            .find(|inst| {
                inst.drc_set_id() == depends_on_drc_set_id && !inst.depends_on_drc_set_present()
            })
    }

    /// Returns whether a difference is detected in the `UniDrcConfig` instance.
    pub(in super::super) fn is_diff(&self) -> bool {
        self.is_diff
    }

    /// Returns `Some(&DrcCoefficientsUniDrc)` if `location` matches. Otherwise `None`.
    ///
    /// # Parameters
    /// - `location`: DRC location where to search.
    ///
    ///  Returns `Option<&DrcCoefficientsUniDrc>`.
    pub(in super::super) fn select_drc_coefficients(
        &self,
        location: DrcBitstreamLocation,
    ) -> Option<&DrcCoefficientsUniDrc> {
        self.drc_coefficients_uni_drc
            .iter()
            .take(usize::from(self.drc_coefficients_uni_drc_count))
            .find(|&coeff_uni_drc| coeff_uni_drc.drc_location() == location)
    }

    /// Returns `Some(&DownmixInstructions)` if `downmix_id` matches. Otherwise `None`.
    ///
    /// # Parameters
    /// - `downmix_id`: DRC downmix ID.
    ///
    ///  Returns `Option<&DownmixInstructions>`.
    fn select_downmix_instructions(&self, downmix_id: u8) -> Option<&DownmixInstructions> {
        self.downmix_instructions
            .iter()
            .take(usize::from(self.downmix_instructions_count()))
            .find(|&downmix_instru| downmix_instru.downmix_id() == downmix_id)
    }

    /// Returns `Some(&DrcInstructionsUniDrc)` if `drc_set_id` matches. Otherwise `None`.
    ///
    /// # Parameters
    /// - `drc_set_id`: Selected `DRC set identifier`.
    ///
    ///  Returns `Option<&DrcInstructionsUniDrc>`.
    pub(in super::super) fn select_drc_instructions(
        &self,
        drc_set_id: i8,
    ) -> Option<&DrcInstructionsUniDrc> {
        self.drc_instructions_uni_drc
            .iter()
            .take(usize::from(self.drc_instructions_count_incl_virtual))
            .find(|&drc_instru| drc_instru.drc_set_id() == drc_set_id)
    }

    /// Reads (skips) `DRC coefficients basic` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    fn skip_drc_coefficients_basic(bs: &mut Bitstream) {
        bs.push(4); // DRC location.
        bs.push(7); // DRC characteristic.
    }

    /// Reads (skips) `DRC instructions basic` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    fn skip_drc_instructions_basic(bs: &mut Bitstream) {
        bs.push(6); // DRC set ID.
        bs.push(4); // DRC location.
        bs.push(7); // Downmix ID.

        let is_additional_downmix_id_present = bs.read_bit() != 0;
        if is_additional_downmix_id_present {
            let additional_downmix_id_count = bs.read(3) as isize;
            bs.push(7 * additional_downmix_id_count); // Additional downmix ID.
        }

        let drc_set_effect = bs.read(16) as u16;
        if !(drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf)) != 0 {
            let is_limiter_peak_target_present = bs.read_bit() != 0;
            if is_limiter_peak_target_present {
                bs.push(8); // bs_limiter_peak_target.
            }
        }

        let is_drc_set_target_loudness_present = bs.read_bit() != 0;
        if is_drc_set_target_loudness_present {
            bs.push(6); // bs_drc_set_target_loudness_value_upper.
            let is_drc_set_target_loudness_value_lower_present = bs.read_bit() != 0;
            if is_drc_set_target_loudness_value_lower_present {
                bs.push(6); // bs_drc_set_target_loudness_value_lower.
            }
        }
    }

    /// Reads (skips) `loudness EQ instructions` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    fn skip_loud_eq_instructions(bs: &mut Bitstream) {
        bs.push(4); // Loud eq set ID.
        bs.push(4); // DRC location.
        let is_downmix_id_present = bs.read_bit() != 0;
        if is_downmix_id_present {
            bs.push(7); // Down mix ID.
            let is_additional_downmix_id_present = bs.read_bit() != 0;
            if is_additional_downmix_id_present {
                let additional_downmix_id_count = bs.read(7);
                // Additional downmix IDs.
                for _i in 0..additional_downmix_id_count {
                    bs.push(7);
                }
            }
        }

        let is_drc_set_id_present = bs.read_bit() != 0;
        if is_drc_set_id_present {
            bs.push(6); // DRC set ID.
            let is_additional_drc_set_id_present = bs.read_bit() != 0;
            if is_additional_drc_set_id_present {
                let additional_drc_set_id_count = bs.read(6);
                // Additional DRC set IDs.
                for _i in 0..additional_drc_set_id_count {
                    bs.push(6);
                }
            }
        }

        let is_eq_set_id_present = bs.read_bit() != 0;
        if is_eq_set_id_present {
            bs.push(6); // Eq set ID.
            let is_additional_eq_set_id_present = bs.read_bit() != 0;
            if is_additional_eq_set_id_present {
                let additional_eq_set_id_count = bs.read(6);
                // Additional eq set IDs.
                for _i in 0..additional_eq_set_id_count {
                    bs.push(6);
                }
            }
        }

        bs.push(1); // loudness after DRC.
        bs.push(1); // loudness after Eq.

        let loud_eq_gain_sequence_count = bs.read(6);
        for _i in 0..loud_eq_gain_sequence_count {
            bs.push(6); // Gain sequence index.
            let is_drc_characteristic_format_cicp = bs.read_bit() != 0;
            if is_drc_characteristic_format_cicp {
                bs.push(7); // DRC characteristic.
            } else {
                bs.push(4); // DRC characteristic left index.
                bs.push(4); // DRC characteristic right index.
            }
            bs.push(6); // Frequency range index.
            bs.push(3); // bs_loud_eq_scaling.
            bs.push(5); // bs_loud_eq_offset.
        }
    }

    /// Reads (skips) `EQ subband gain spline` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    fn skip_eq_subband_gain_spline(bs: &mut Bitstream) {
        let num_eq_nodes = bs.read(5) + 2;

        for _k in 0..num_eq_nodes {
            let is_bits = bs.read_bit() != 0;
            if !is_bits {
                bs.push(4);
            }
        }

        bs.push(4 * (num_eq_nodes - 1) as isize);
        let bits = bs.read(2) as u8;
        match bits {
            0 => {
                bs.push(5);
            }
            1 | 2 => {
                bs.push(4);
            }
            3 => {
                bs.push(3);
            }
            _ => {}
        }

        bs.push(5 * (num_eq_nodes - 1) as isize);
    }

    /// Reads (skips) `EQ coefficients` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    fn skip_eq_coefficients(bs: &mut Bitstream) {
        let is_eq_delay_max_present = bs.read_bit() != 0;
        if is_eq_delay_max_present {
            bs.push(8); // bs_eq_delay_max.
        }

        let unique_filter_block_count = bs.read(6);
        for _j in 0..unique_filter_block_count {
            let filter_element_count = bs.read(6);
            for _k in 0..filter_element_count {
                bs.push(6); // Filter element index.
                let is_filter_element_gain_present = bs.read_bit() != 0;
                if is_filter_element_gain_present {
                    bs.push(10); // bs_filter_element_gain.
                }
            }
        }

        let unique_td_filter_element_count = bs.read(6);
        for _j in 0..unique_td_filter_element_count {
            let is_eq_filter_format = bs.read_bit() != 0;
            if !is_eq_filter_format {
                // Pole / zero.
                let bs_real_zero_radius_one_count = bs.read(3);
                let real_zero_count = bs.read(6);
                let generic_zero_count = bs.read(6);
                let real_pole_count = bs.read(4);
                let complex_pole_count = bs.read(4);
                bs.push(2 * bs_real_zero_radius_one_count as isize);
                bs.push(8 * real_zero_count as isize);
                bs.push(14 * generic_zero_count as isize);
                bs.push(8 * real_pole_count as isize);
                bs.push(14 * complex_pole_count as isize);
            } else {
                // FIR coefficients.
                let fir_filter_order = bs.read(7) as isize;
                bs.push(1);
                bs.push((fir_filter_order / 2 + 1) * 11);
            }
        }

        let unique_eq_subband_gains_count = bs.read(6);

        if unique_eq_subband_gains_count > 0 {
            let is_eq_subband_gain_representation = bs.read_bit() != 0;
            let eq_subband_gain_format = EqSubbandGainFormat::from(bs.read(4) as u8);

            let eq_subband_gain_count = match eq_subband_gain_format {
                EqSubbandGainFormat::Qmf32 => 32,
                EqSubbandGainFormat::QmfHybrid39 => 39,
                EqSubbandGainFormat::Qmf64 => 64,
                EqSubbandGainFormat::QmfHybrid71 => 71,
                EqSubbandGainFormat::Qmf128 => 128,
                EqSubbandGainFormat::QmfHybrid135 => 135,
                EqSubbandGainFormat::Uniform | EqSubbandGainFormat::DefaultBsRead => bs.read(8) + 1,
            };

            for _k in 0..unique_eq_subband_gains_count {
                if is_eq_subband_gain_representation {
                    Self::skip_eq_subband_gain_spline(bs);
                } else {
                    bs.push(9 * eq_subband_gain_count as isize);
                }
            }
        }
    }

    /// Reads (skips) `TD filter` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `eq_channel_group_count`: Number of EQ channel groups
    fn skip_td_filter_cascade(bs: &mut Bitstream, eq_channel_group_count: isize) {
        for _i in 0..eq_channel_group_count {
            let is_eq_cascade_gain_present = bs.read_bit() != 0;

            if is_eq_cascade_gain_present {
                bs.push(10); // bs_eq_cascade_gain.
            }
            let filter_block_count = bs.read(4);
            bs.push(7 * filter_block_count as isize); // filter_block_index.
        }

        let is_eq_phase_alignment_present = bs.read_bit() != 0;
        if is_eq_phase_alignment_present {
            for i in 0..eq_channel_group_count {
                bs.push(eq_channel_group_count - i - 1);
            }
        }
    }

    /// Reads (skips) `EQ instructions` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn skip_eq_instructions(&self, bs: &mut Bitstream) -> DrcError {
        bs.push(6); // eq_set_id.
        bs.push(4); // eq_set_complexity_level.

        let mut additional_drc_set_id_count = 0;
        let is_downmix_id_present: bool = bs.read_bit() != 0;
        let (downmix_id, is_eq_apply_to_downmix) = if is_downmix_id_present {
            let down_mix_id = bs.read(7) as u8;
            let is_eq_apply_to_downmix = bs.read_bit() != 0;
            let is_additional_downmix_id_present = bs.read_bit() != 0;
            if is_additional_downmix_id_present {
                let additional_downmix_id_count = bs.read(7) as isize;
                bs.push(7 * additional_downmix_id_count); // additional_downmix_ids.
            }
            (down_mix_id, is_eq_apply_to_downmix)
        } else {
            (0, false)
        };

        bs.read(6); // DRC set ID.
        let is_additional_drc_set_id_present = bs.read_bit() != 0;

        if is_additional_drc_set_id_present {
            additional_drc_set_id_count = bs.read(6) as isize;
            // additional_drc_set_ids.
            for _i in 0..additional_drc_set_id_count {
                bs.push(6);
            }
        }

        bs.push(16); // eq_set_purpose.

        let is_depends_on_eq_set_present = bs.read_bit() != 0;

        if is_depends_on_eq_set_present {
            bs.push(6); // depends_on_eq_set.
        } else {
            bs.push(1); // no_independent_eq_use.
        }

        let mut channel_count = self.channel_layout().base_channel_count();

        if is_downmix_id_present
            && is_eq_apply_to_downmix
            && downmix_id != 0
            && downmix_id != DOWNMIX_ID_ANY_DOWNMIX
            && additional_drc_set_id_count == 0
        {
            let down_instr = self.select_downmix_instructions(downmix_id);
            if let Some(down) = down_instr {
                // Target channel count from downmix id.
                channel_count = down.target_channel_count();
            } else {
                return DrcError::NotOk;
            }
        } else if downmix_id == DOWNMIX_ID_ANY_DOWNMIX || additional_drc_set_id_count > 1 {
            channel_count = 1;
        }

        let mut eq_channel_group_count = 0;
        let mut eq_channel_group_for_channel = [0_u8; DRCDEC_MAX_CHANNELS];
        for c in 0..usize::from(channel_count) {
            let mut is_new_group = true;
            if c >= DRCDEC_MAX_CHANNELS {
                return DrcError::MemoryError;
            }
            eq_channel_group_for_channel[c] = bs.read(7) as u8;

            for k in 0..c {
                if eq_channel_group_for_channel[c] == eq_channel_group_for_channel[k] {
                    is_new_group = false;
                }
            }

            if is_new_group {
                eq_channel_group_count += 1;
            }
        }

        let is_td_filter_cascade_present = bs.read_bit() != 0;
        if is_td_filter_cascade_present {
            Self::skip_td_filter_cascade(bs, eq_channel_group_count);
        }

        let is_subband_gains_present = bs.read_bit() != 0;
        if is_subband_gains_present {
            bs.push(6 * eq_channel_group_count); // subband_gains_index.
        }

        let is_eq_transition_duration_present = bs.read_bit() != 0;
        if is_eq_transition_duration_present {
            bs.push(5); // bs_eq_transition_duration.
        }

        DrcError::Ok
    }

    /// Reads `UniDrcConfig extension` data from bitstream for V1 case.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn read_drc_extension_v1(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut diff = self.is_diff;
        let is_downmix_instructions_v1_present = bs.read_bit() != 0;
        if is_downmix_instructions_v1_present {
            diff |= comp_assign(&mut self.downmix_instructions_count_v1, bs.read(7) as u8);

            let offset = self.downmix_instructions_count_v0;
            self.downmix_instructions_count =
                (offset + self.downmix_instructions_count_v1).min(MAX_DOWNMIX_INSTRUCTIONS);

            let base_channel_count = self.channel_layout().base_channel_count();
            for i in 0..self.downmix_instructions_count_v1 {
                let mut tmp_down = DownmixInstructions::default();
                tmp_down.read(bs, base_channel_count, 1)?;

                if offset + i >= MAX_DOWNMIX_INSTRUCTIONS {
                    continue;
                }

                diff |= comp_assign(
                    &mut self.downmix_instructions[usize::from(offset + i)],
                    tmp_down,
                );
            }
        } else {
            diff |= comp_assign(&mut self.downmix_instructions_count_v1, 0);
        }

        let is_drc_coeffs_and_instructions_uni_drc_v1_present = bs.read_bit() != 0;
        if is_drc_coeffs_and_instructions_uni_drc_v1_present {
            diff |= comp_assign(
                &mut self.drc_coefficients_uni_drc_count_v1,
                bs.read(3) as u8,
            );

            let mut offset = self.drc_coefficients_uni_drc_count_v0;
            self.drc_coefficients_uni_drc_count = (offset + self.drc_coefficients_uni_drc_count_v1)
                .min(DRCDEC_MAX_DOWNMIX_COEFFICIENTS);

            for i in 0..self.drc_coefficients_uni_drc_count_v1 {
                let mut tmp_coeff = DrcCoefficientsUniDrc::default();
                tmp_coeff.read(bs, 1)?;

                if offset + i >= DRCDEC_MAX_DOWNMIX_COEFFICIENTS {
                    continue;
                }

                diff |= comp_assign(
                    &mut self.drc_coefficients_uni_drc[usize::from(offset + i)],
                    tmp_coeff,
                );
            }

            diff |= comp_assign(
                &mut self.drc_instructions_uni_drc_count_v1,
                bs.read(6) as u8,
            );

            offset = self.drc_instructions_uni_drc_count;
            self.drc_instructions_uni_drc_count =
                (offset + self.drc_instructions_uni_drc_count_v1).min(MAX_DRC_INSTRUCTIONS);

            let base_channel_count = self.channel_layout().base_channel_count();
            for i in 0..self
                .drc_instructions_uni_drc_count
                .min(MAX_DRC_INSTRUCTIONS - offset)
            {
                let mut tmp_inst = DrcInstructionsUniDrc::default();
                tmp_inst.read(
                    bs,
                    &self.downmix_instructions,
                    &self.drc_coefficients_uni_drc,
                    self.downmix_instructions_count,
                    self.drc_coefficients_uni_drc_count,
                    base_channel_count,
                    1,
                )?;

                diff |= comp_assign(
                    &mut self.drc_instructions_uni_drc[usize::from(offset + i)],
                    tmp_inst,
                );
            }

            for _i in self
                .drc_instructions_uni_drc_count
                .min(MAX_DRC_INSTRUCTIONS - offset)
                ..self.drc_instructions_uni_drc_count
            {
                let mut tmp_inst = DrcInstructionsUniDrc::default();
                tmp_inst.read(
                    bs,
                    &self.downmix_instructions,
                    &self.drc_coefficients_uni_drc,
                    self.downmix_instructions_count,
                    self.drc_coefficients_uni_drc_count,
                    base_channel_count,
                    1,
                )?;
            }
        } else {
            diff |= comp_assign(&mut self.drc_coefficients_uni_drc_count_v1, 0);
            diff |= comp_assign(&mut self.drc_instructions_uni_drc_count_v1, 0);
        }

        let is_loud_eq_instructions_present = bs.read_bit() != 0;
        if is_loud_eq_instructions_present {
            let loud_eq_instructions_count = bs.read(4);
            for _i in 0..loud_eq_instructions_count {
                Self::skip_loud_eq_instructions(bs);
            }
        }

        let is_eq_present = bs.read_bit() != 0;
        if is_eq_present {
            Self::skip_eq_coefficients(bs);
            let eq_instructions_count = bs.read(4);
            for _i in 0..eq_instructions_count {
                self.skip_eq_instructions(bs);
            }
        }

        self.is_diff = diff;

        Ok(())
    }

    /// Reads `UniDrcConfig extension` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn read_uni_drc_config_extension(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut k = 0;
        self.uni_drc_config_ext.uni_drc_config_ext_type[k] = bs.read(4) as u8;

        while self.uni_drc_config_ext.uni_drc_config_ext_type[k] != UNIDRCCONFEXT_TERM {
            if k >= (MAX_EXTENSIONS - 1) {
                return Err(DrcError::MemoryError);
            }
            let bit_size_len = bs.read(4) as u8;
            let ext_size_bits = bit_size_len + 4;

            let bit_size = bs.read(ext_size_bits);
            self.uni_drc_config_ext.ext_bit_size[k] = bit_size + 1;
            let n_bits_remaining = bs.valid_bits();

            match self.uni_drc_config_ext.uni_drc_config_ext_type[k] {
                UNIDRCCONFEXT_V1 => {
                    self.read_drc_extension_v1(bs)?;
                    let bs_valid_bits = bs.valid_bits();
                    if n_bits_remaining
                        != (self.uni_drc_config_ext.ext_bit_size[k] as isize + bs_valid_bits)
                    {
                        return Err(DrcError::NotOk);
                    }
                }

                UNIDRCCONFEXT_PARAM_DRC => {
                    bs.push(self.uni_drc_config_ext.ext_bit_size[k] as isize);
                }
                // Add future extensions here.
                _ => {
                    bs.push(self.uni_drc_config_ext.ext_bit_size[k] as isize);
                }
            }

            k += 1;
            self.uni_drc_config_ext.uni_drc_config_ext_type[k] = bs.read(4) as u8;
        }

        Ok(())
    }

    /// Reads `UniDrcConfig` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut tmp_chan = ChannelLayout::default();

        let mut diff = comp_assign(&mut self.is_sample_rate_present, bs.read_bit() != 0);
        if self.is_sample_rate_present {
            diff |= comp_assign(&mut self.sample_rate, bs.read(18) + 1000);
        }

        diff |= comp_assign(&mut self.downmix_instructions_count_v0, bs.read(7) as u8);

        let is_drc_description_basic_present = bs.read_bit() != 0;
        let (coefficients_basic_count, instructions_basic_count) =
            if is_drc_description_basic_present {
                (bs.read(3), bs.read(4))
            } else {
                (0, 0)
            };

        diff |= comp_assign(
            &mut self.drc_coefficients_uni_drc_count_v0,
            bs.read(3) as u8,
        );

        diff |= comp_assign(
            &mut self.drc_instructions_uni_drc_count_v0,
            bs.read(6) as u8,
        );

        tmp_chan.read(bs)?;

        diff |= comp_assign(&mut self.channel_layout, tmp_chan);

        self.downmix_instructions_count = self
            .downmix_instructions_count_v0
            .min(MAX_DOWNMIX_INSTRUCTIONS);

        let base_channel_count = self.channel_layout().base_channel_count();
        for downmix_inst in self
            .downmix_instructions
            .iter_mut()
            .take(usize::from(self.downmix_instructions_count))
        {
            let mut tmp_down = DownmixInstructions::default();
            tmp_down.read(bs, base_channel_count, 0)?;

            diff |= comp_assign(downmix_inst, tmp_down);
        }

        for _i in self.downmix_instructions_count..self.downmix_instructions_count_v0 {
            let mut tmp_down = DownmixInstructions::default();
            tmp_down.read(bs, base_channel_count, 0)?;
        }

        for _count in 0..coefficients_basic_count {
            Self::skip_drc_coefficients_basic(bs);
        }

        for _count in 0..instructions_basic_count {
            Self::skip_drc_instructions_basic(bs);
        }

        self.drc_coefficients_uni_drc_count = self
            .drc_coefficients_uni_drc_count_v0
            .min(DRCDEC_MAX_DOWNMIX_COEFFICIENTS);
        for i in 0..usize::from(self.drc_coefficients_uni_drc_count_v0) {
            let mut tmp_coeff = DrcCoefficientsUniDrc::default();
            tmp_coeff.read(bs, 0)?;

            if i >= DRCDEC_MAX_DOWNMIX_COEFFICIENTS as usize {
                continue;
            }

            diff |= comp_assign(&mut self.drc_coefficients_uni_drc[i], tmp_coeff);
        }

        self.drc_instructions_uni_drc_count = self
            .drc_instructions_uni_drc_count_v0
            .min(MAX_DRC_INSTRUCTIONS);

        for i in 0..usize::from(
            self.drc_instructions_uni_drc_count_v0
                .min(MAX_DRC_INSTRUCTIONS),
        ) {
            let mut tmp_instr = DrcInstructionsUniDrc::default();
            {
                tmp_instr.read(
                    bs,
                    &self.downmix_instructions,
                    &self.drc_coefficients_uni_drc,
                    self.downmix_instructions_count,
                    self.drc_coefficients_uni_drc_count,
                    base_channel_count,
                    0,
                )?;
            }

            diff |= comp_assign(&mut self.drc_instructions_uni_drc[i], tmp_instr);
        }

        for _i in self
            .drc_instructions_uni_drc_count_v0
            .min(MAX_DRC_INSTRUCTIONS)..self.drc_instructions_uni_drc_count_v0
        {
            let mut tmp_instr = DrcInstructionsUniDrc::default();
            tmp_instr.read(
                bs,
                &self.downmix_instructions,
                &self.drc_coefficients_uni_drc,
                self.downmix_instructions_count,
                self.drc_coefficients_uni_drc_count,
                base_channel_count,
                0,
            )?;
        }

        diff |= comp_assign(&mut self.is_uni_drc_config_ext_present, bs.read_bit() != 0);

        self.is_diff = self.is_diff || diff;

        if self.is_uni_drc_config_ext_present {
            self.read_uni_drc_config_extension(bs)?;
        }

        Ok(())
    }

    /// Sets whether a difference should be assigned in the `UniDrcConfig` instance.
    pub(in super::super) fn set_diff(&mut self, is_diff: bool) {
        self.is_diff = is_diff;
    }
}
