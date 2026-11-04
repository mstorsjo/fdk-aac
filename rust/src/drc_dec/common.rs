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
//! Enums and functions used commonly by DRC's components.

use super::{constants::*, drc_error::DrcError};
use crate::common::bitstream::Bitstream;
use itertools::izip;
use std::ops::{BitAnd, BitOr};

#[expect(non_camel_case_types)]
#[repr(C)]
#[derive(Copy, Clone, Default, Debug, PartialEq)]
pub enum CodecMode {
    #[default]
    Undefined = -1,
    Mpeg4_Aac,
    MpegD_Usac,
}

#[repr(C)]
#[derive(Debug)]
/// Types of DRC effects.
pub(super) enum EffectBit {
    _Night = 0x0001,
    _Noisy = 0x0002,
    _Limited = 0x0004,
    _Lowlevel = 0x0008,
    _Dialog = 0x0010,
    GeneralCompr = 0x0020,
    _Expand = 0x0040,
    _Artistic = 0x0080,
    Clipping = 0x0100,
    Fade = 0x0200,
    DuckOther = 0x0400,
    DuckSelf = 0x0800,
}

/// `From<EffectBit>` trait for `u16`.
impl From<EffectBit> for u16 {
    fn from(value: EffectBit) -> Self {
        match value {
            EffectBit::_Night => 0x0001,
            EffectBit::_Noisy => 0x0002,
            EffectBit::_Limited => 0x0004,
            EffectBit::_Lowlevel => 0x0008,
            EffectBit::_Dialog => 0x0010,
            EffectBit::GeneralCompr => 0x0020,
            EffectBit::_Expand => 0x0040,
            EffectBit::_Artistic => 0x0080,
            EffectBit::Clipping => 0x0100,
            EffectBit::Fade => 0x0200,
            EffectBit::DuckOther => 0x0400,
            EffectBit::DuckSelf => 0x0800,
        }
    }
}

/// `BitOr` trait for `EffectBit`.
impl BitOr for EffectBit {
    type Output = u16;

    fn bitor(self, rhs: Self) -> Self::Output {
        u16::from(self) | u16::from(rhs)
    }
}

/// `BitAnd` trait for `EffectBit`.
impl BitAnd for EffectBit {
    type Output = u16;

    fn bitand(self, rhs: Self) -> Self::Output {
        u16::from(self) & u16::from(rhs)
    }
}

#[repr(C)]
#[derive(Debug, Default, PartialEq)]
/// Used for look-ahead.
pub(super) enum DelayMode {
    #[default]
    /// Applies the DRC gain with a delay of one DRC frame.
    Regular = 0,
    /// Immediately applies the decoded DRC gain.
    _Low = 1,
}

#[repr(C)]
#[derive(Debug, Default, PartialEq)]
/// The concept of sub-band domain DRC means that the existing sub-band signals of the audio
/// decoder are subject to the DRC gain application.
pub(super) enum SubbandDomainMode {
    #[default]
    Off,
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// The types of DRC gain interpolation. The decoder applies interpolation to achieve a smooth gain
/// transition between the samples.
pub(super) enum GainInterpolationType {
    #[default]
    Spline = 0,
    Linear = 1,
}

/// `From<bool>` trait for GainInterpolationType.
impl From<bool> for GainInterpolationType {
    fn from(value: bool) -> Self {
        match value {
            false => GainInterpolationType::Spline,
            true => GainInterpolationType::Linear,
        }
    }
}

/// Codec-specific encoding of drcLocation for MPEG-4 Audio. Otherwise said, it describes the
/// location (bitstream syntax element) where the uniDRC gain sequences can be found in the
/// bitstream.
#[repr(u8)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
pub(super) enum DrcBitstreamLocation {
    #[default]
    Undefined = 0x0,
    Mp4InstreamUniDrc = 0x1,
    _Mp4DynRangeInfo = 0x2,
    _Mp4CompressionValue = 0x3,
    _Reserved4 = 0x4,
    _Reserved5 = 0x5,
    _Reserved6 = 0x6,
    _Reserved7 = 0x7,
    _Reserved8 = 0x8,
    _Reserved9 = 0x9,
    _ReservedA = 0xA,
    _ReservedB = 0xB,
    _ReservedC = 0xC,
    _ReservedD = 0xD,
    _ReservedE = 0xE,
    _ReservedF = 0xF,
}

impl From<u8> for DrcBitstreamLocation {
    fn from(value: u8) -> Self {
        match value {
            0 => DrcBitstreamLocation::Undefined,
            1 => DrcBitstreamLocation::Mp4InstreamUniDrc,
            2 => DrcBitstreamLocation::_Mp4DynRangeInfo,
            3 => DrcBitstreamLocation::_Mp4CompressionValue,
            4 => DrcBitstreamLocation::_Reserved4,
            5 => DrcBitstreamLocation::_Reserved5,
            6 => DrcBitstreamLocation::_Reserved6,
            7 => DrcBitstreamLocation::_Reserved7,
            8 => DrcBitstreamLocation::_Reserved8,
            9 => DrcBitstreamLocation::_Reserved9,
            10 => DrcBitstreamLocation::_ReservedA,
            11 => DrcBitstreamLocation::_ReservedB,
            12 => DrcBitstreamLocation::_ReservedC,
            13 => DrcBitstreamLocation::_ReservedD,
            14 => DrcBitstreamLocation::_ReservedE,
            15 => DrcBitstreamLocation::_ReservedF,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<DrcBitstreamLocation> for u8 {
    fn from(value: DrcBitstreamLocation) -> Self {
        match value {
            DrcBitstreamLocation::Undefined => 0,
            DrcBitstreamLocation::Mp4InstreamUniDrc => 1,
            DrcBitstreamLocation::_Mp4DynRangeInfo => 2,
            DrcBitstreamLocation::_Mp4CompressionValue => 3,
            DrcBitstreamLocation::_Reserved4 => 4,
            DrcBitstreamLocation::_Reserved5 => 5,
            DrcBitstreamLocation::_Reserved6 => 6,
            DrcBitstreamLocation::_Reserved7 => 7,
            DrcBitstreamLocation::_Reserved8 => 8,
            DrcBitstreamLocation::_Reserved9 => 9,
            DrcBitstreamLocation::_ReservedA => 10,
            DrcBitstreamLocation::_ReservedB => 11,
            DrcBitstreamLocation::_ReservedC => 12,
            DrcBitstreamLocation::_ReservedD => 13,
            DrcBitstreamLocation::_ReservedE => 14,
            DrcBitstreamLocation::_ReservedF => 15,
        }
    }
}

impl From<i32> for DrcBitstreamLocation {
    fn from(value: i32) -> Self {
        match value {
            0 => DrcBitstreamLocation::Undefined,
            1 => DrcBitstreamLocation::Mp4InstreamUniDrc,
            2 => DrcBitstreamLocation::_Mp4DynRangeInfo,
            3 => DrcBitstreamLocation::_Mp4CompressionValue,
            4 => DrcBitstreamLocation::_Reserved4,
            5 => DrcBitstreamLocation::_Reserved5,
            6 => DrcBitstreamLocation::_Reserved6,
            7 => DrcBitstreamLocation::_Reserved7,
            8 => DrcBitstreamLocation::_Reserved8,
            9 => DrcBitstreamLocation::_Reserved9,
            10 => DrcBitstreamLocation::_ReservedA,
            11 => DrcBitstreamLocation::_ReservedB,
            12 => DrcBitstreamLocation::_ReservedC,
            13 => DrcBitstreamLocation::_ReservedD,
            14 => DrcBitstreamLocation::_ReservedE,
            15 => DrcBitstreamLocation::_ReservedF,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<DrcBitstreamLocation> for i32 {
    fn from(value: DrcBitstreamLocation) -> Self {
        match value {
            DrcBitstreamLocation::Undefined => 0,
            DrcBitstreamLocation::Mp4InstreamUniDrc => 1,
            DrcBitstreamLocation::_Mp4DynRangeInfo => 2,
            DrcBitstreamLocation::_Mp4CompressionValue => 3,
            DrcBitstreamLocation::_Reserved4 => 4,
            DrcBitstreamLocation::_Reserved5 => 5,
            DrcBitstreamLocation::_Reserved6 => 6,
            DrcBitstreamLocation::_Reserved7 => 7,
            DrcBitstreamLocation::_Reserved8 => 8,
            DrcBitstreamLocation::_Reserved9 => 9,
            DrcBitstreamLocation::_ReservedA => 10,
            DrcBitstreamLocation::_ReservedB => 11,
            DrcBitstreamLocation::_ReservedC => 12,
            DrcBitstreamLocation::_ReservedD => 13,
            DrcBitstreamLocation::_ReservedE => 14,
            DrcBitstreamLocation::_ReservedF => 15,
        }
    }
}

/// Apply only DRC sets dedicated to processing location.
#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq)]
pub enum ProcessingLocation {
    /// Before downmix.
    Drc1 = 0,
    /// After downmix.
    Drc2Drc3 = 1,
}

impl From<usize> for ProcessingLocation {
    fn from(value: usize) -> Self {
        match value {
            0 => ProcessingLocation::Drc1,
            1 => ProcessingLocation::Drc2Drc3,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<ProcessingLocation> for usize {
    fn from(value: ProcessingLocation) -> Self {
        match value {
            ProcessingLocation::Drc1 => 0,
            ProcessingLocation::Drc2Drc3 => 1,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, PartialEq, Clone, Copy)]
/// Ducking modification structure.
pub(super) struct DuckingModification {
    /// Flag to indicate presence of scaling values.
    is_scaling_present: bool,
    /// Ducking scale value.
    ducking_scaling: f32,
}

impl DuckingModification {
    /// Decodes `ducking modification `data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    pub(super) fn decode_ducking_modification(&mut self, bs: &mut Bitstream) {
        self.is_scaling_present = bs.read_bit() != 0;

        if self.is_scaling_present {
            let bs_ducking_scaling = bs.read(4);
            let sigma = bs_ducking_scaling >> 3;
            let mu = bs_ducking_scaling & 0x7;

            self.ducking_scaling = if sigma != 0 {
                1.0_f32 - 0.125_f32 * ((1 + mu) as f32)
            } else {
                1.0_f32 + 0.125_f32 * ((1 + mu) as f32)
            }
        } else {
            self.ducking_scaling = 1.0_f32;
        }
    }

    pub(super) fn ducking_scaling(&self) -> f32 {
        self.ducking_scaling
    }

    pub(super) fn is_scaling_present(&self) -> bool {
        self.is_scaling_present
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// `GainModification` structure that holds gain modification for channel group.
pub(super) struct GainModification {
    /// Flag to indicate presence of left target characteristic.
    is_target_characteristic_left_present: bool,
    /// Index of left target characteristic.
    target_characteristic_left_index: u8,
    /// Flag to indicate presence of right target characteristic.
    is_target_characteristic_right_present: bool,
    /// Index of right target characteristic.
    target_characteristic_right_index: u8,
    /// Flag to indicate presence of gain scaling factors. Has to be applied for channel group.
    is_gain_scaling_present: bool,
    /// Attenutaion scale factor.
    attenuation_scaling: f32,
    /// Amplification scale factor.
    amplification_scaling: f32,
    /// Flag to indicate presence of gain offset. Has to be applied for channel group.
    is_gain_offset_present: bool,
    /// Gain offset value for channel group.
    gain_offset: f32,
}

impl GainModification {
    pub(super) fn amplification_scaling(&self) -> f32 {
        self.amplification_scaling
    }

    pub(super) fn attenuation_scaling(&self) -> f32 {
        self.attenuation_scaling
    }

    pub(super) fn gain_offset(&self) -> f32 {
        self.gain_offset
    }

    pub(super) fn is_gain_offset_present(&self) -> bool {
        self.is_gain_offset_present
    }

    pub(super) fn is_gain_scaling_present(&self) -> bool {
        self.is_gain_scaling_present
    }

    pub(super) fn is_target_characteristic_left_present(&self) -> bool {
        self.is_target_characteristic_left_present
    }

    /// Sets value of `self.is_target_characteristic_left_present`.
    pub(super) fn set_is_target_characteristic_left_present(&mut self, is_present: bool) {
        self.is_target_characteristic_left_present = is_present;
    }

    /// Sets value of `self.target_characteristic_left_index`.
    pub(super) fn _set_target_characteristic_left_index(&mut self, index: u8) {
        self.target_characteristic_left_index = index;
    }

    pub(super) fn target_characteristic_left_index(&self) -> u8 {
        self.target_characteristic_left_index
    }

    /// Sets value of `self.target_characteristic_right_index`.
    pub(super) fn _set_target_characteristic_right_index(&mut self, index: u8) {
        self.target_characteristic_right_index = index;
    }

    pub(super) fn target_characteristic_right_index(&self) -> u8 {
        self.target_characteristic_right_index
    }

    /// Sets value of `self.is_target_characteristic_right_present`.
    pub(super) fn set_is_target_characteristic_right_present(&mut self, is_present: bool) {
        self.is_target_characteristic_right_present = is_present;
    }

    pub(super) fn is_target_characteristic_right_present(&self) -> bool {
        self.is_target_characteristic_right_present
    }

    /// Sets value of `self.is_gain_scaling_present`.
    pub(super) fn set_is_gain_scaling_present(&mut self, is_present: bool) {
        self.is_gain_scaling_present = is_present;
    }

    /// Sets value of `self.is_gain_offset_present`.
    pub(super) fn set_is_gain_offset_present(&mut self, is_present: bool) {
        self.is_gain_offset_present = is_present;
    }

    /// Sets value of `self.attenuation_scaling`.
    pub(super) fn set_attenuation_scaling(&mut self, scale: f32) {
        self.attenuation_scaling = scale;
    }

    /// Sets value of `self.amplification_scaling`.
    pub(super) fn set_amplification_scaling(&mut self, scale: f32) {
        self.amplification_scaling = scale;
    }

    /// Sets value of `self.gain_offset`.
    pub(super) fn set_gain_offset(&mut self, offset: f32) {
        self.gain_offset = offset;
    }

    /// Reads gain modification data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    pub(super) fn decode(&mut self, bs: &mut Bitstream) {
        self.is_target_characteristic_left_present = bs.read_bit() != 0;
        if self.is_target_characteristic_left_present {
            self.target_characteristic_left_index = bs.read(4) as u8;
        }

        self.is_target_characteristic_right_present = bs.read_bit() != 0;
        if self.is_target_characteristic_right_present {
            self.target_characteristic_right_index = bs.read(4) as u8;
        }

        self.is_gain_scaling_present = bs.read_bit() != 0;
        if self.is_gain_scaling_present {
            let bs_attenuation_scaling = bs.read(4);
            self.attenuation_scaling = bs_attenuation_scaling as f32 * 0.125_f32;
            let bs_amplification_scaling = bs.read(4);
            self.amplification_scaling = bs_amplification_scaling as f32 * 0.125_f32;
        }

        self.is_gain_offset_present = bs.read_bit() != 0;
        if self.is_gain_offset_present {
            let is_sign = bs.read_bit() != 0;
            let bs_gain_offset = bs.read(5);
            self.gain_offset = (1 + bs_gain_offset) as f32 * 0.25_f32;
            if is_sign {
                self.gain_offset = -self.gain_offset;
            }
        }
    }
}

/// Get linear value from approximate dB.
pub(super) fn approx_db_to_linear(db_val: f32) -> f32 {
    // powf(2.0f, dB_val / 6.0f).
    // 0.11552453009332421824 => log(2) / 6.
    f32::exp(0.11552453009332421824_f32 * db_val)
}

/// Get linear value from dB.
pub fn db_to_linear(db_val: f32) -> f32 {
    // powf(10.0f, dB_val / 20.0f).
    // 0.1151292546497022842 => log(10)/20.
    f32::exp(0.1151292546497022842_f32 * db_val)
}

/// Get dB value from linear value.
pub(super) fn linear_to_db(lin_val: f32) -> f32 {
    20.0 * lin_val.max(f32::MIN_POSITIVE).log10()
}

/// Returns rounded float value.
fn round_to_int(input: f32) -> f32 {
    let output = if input >= 0.0 {
        (input + 0.5) as i32
    } else {
        (input - 0.5) as i32
    };
    output as f32
}

/// Calculates `downmix offset` value.
///
/// # Parameters
///
/// - `base_channel_count`: Number of base channels in channel layout.
/// - `bs_downmix_offset`: downmix offset value from bitstream.
/// - `target_channel_count`: Number of output channels after downmixing.
///
///  Returns `downmix offset value`.
pub(super) fn get_downmix_offset(
    base_channel_count: u8,
    bs_downmix_offset: u8,
    target_channel_count: u8,
) -> f32 {
    let mut downmix_offset = 1.0_f32;

    if bs_downmix_offset == 1 || bs_downmix_offset == 2 {
        if base_channel_count <= target_channel_count {
            return downmix_offset;
        }

        let q = target_channel_count as f32 / base_channel_count as f32;
        let mut a = linear_to_db(q);

        a *= bs_downmix_offset as f32;

        a = 0.5_f32 * round_to_int(a);

        downmix_offset = db_to_linear(a);
    }

    downmix_offset
}

/// Returns `delta time min` value for given `sample rate`.
pub(super) fn get_delta_tmin(sample_rate: u32) -> u16 {
    // half_ms = round (0.0005 * sampleRate)
    let half_ms = (sample_rate + 1000) / 2000;
    // delta_tmin value
    (half_ms + 1).next_power_of_two() as u16
}

/// Derives DRC channel groups from ducking modification and gain set indices based on DRC effect
/// types.
///
/// # Parameters
///
/// - `dm_for_channel`: Ducking modification for channels.
/// - `dm_for_channel_group`: Ducking modification for channel groups.
/// - `gain_set_index`: A unique number which specifies which DRC gain set should be assigned to
///   which channel of the configuration.
/// - `gain_set_index_for_channel_group`: Stores unique `gain_set_index` values for channel groups.
/// - `group_for_channel`: Buffer to store channel group for channels.
/// - `num_channel_groups`: Number of channel groups, gets updated.
/// - `drc_channel_count`: Number of DRC channels.
/// - `drc_set_effect`: Effect type of the DRC.
///
///  Returns `DrcError`.
#[expect(clippy::too_many_arguments)]
pub(super) fn derive_drc_channel_groups(
    dm_for_channel: Option<&[DuckingModification; DRCDEC_MAX_CHANNELS]>,
    dm_for_channel_group: Option<&mut [DuckingModification; DRCDEC_MAX_CHANNELS]>,
    gain_set_index: &[i8; DRCDEC_MAX_CHANNELS],
    gain_set_index_for_channel_group: &mut [i8; DRCDEC_MAX_CHANNELS],
    group_for_channel: &mut [i8; DRCDEC_MAX_CHANNELS],
    num_channel_groups: &mut u8,
    drc_channel_count: usize,
    drc_set_effect: u16,
) -> Result<(), DrcError> {
    let mut unique_scaling = [-1.0_f32; DRCDEC_MAX_CHANNELS];
    gain_set_index_for_channel_group.fill(-10_i8);

    let mut ducking_sequence = -1;
    let mut group_count = 0_usize;
    let channel_count = drc_channel_count.min(DRCDEC_MAX_CHANNELS);

    if drc_set_effect & u16::from(EffectBit::DuckOther) != 0 {
        if let Some(ducking_modification) = dm_for_channel {
            for (c, grp_ch) in group_for_channel.iter_mut().enumerate().take(channel_count) {
                let mut is_match = false;
                let idx = gain_set_index[c];
                let factor = ducking_modification[c].ducking_scaling;

                if idx < 0 {
                    for (n, uni_scale) in unique_scaling.iter().enumerate().take(group_count) {
                        if *uni_scale == factor {
                            is_match = true;
                            *grp_ch = n as i8;
                            break;
                        }
                    }

                    if !is_match {
                        if group_count >= DRCDEC_MAX_CHANNELS {
                            return Err(DrcError::MemoryError);
                        }
                        gain_set_index_for_channel_group[group_count] = idx;
                        unique_scaling[group_count] = factor;
                        *grp_ch = group_count as i8;
                        group_count += 1;
                    }
                } else {
                    if (ducking_sequence > 0) && (ducking_sequence != idx) {
                        return Err(DrcError::NotOk);
                    }
                    ducking_sequence = idx;
                    *grp_ch = -1;
                }
            }

            if drc_channel_count > DRCDEC_MAX_CHANNELS {
                return Err(DrcError::MemoryError);
            }
            if ducking_sequence == -1 {
                return Err(DrcError::NotOk);
            }
        }
    } else if drc_set_effect & u16::from(EffectBit::DuckSelf) != 0 {
        if let Some(ducking_modification) = dm_for_channel {
            for (c, grp_ch) in group_for_channel.iter_mut().enumerate().take(channel_count) {
                let mut is_match: bool = false;
                let idx = gain_set_index[c];
                let factor = ducking_modification[c].ducking_scaling;

                if idx >= 0 {
                    for (n, uni_scale) in unique_scaling.iter().enumerate().take(group_count) {
                        if gain_set_index_for_channel_group[n] == idx && *uni_scale == factor {
                            is_match = true;
                            *grp_ch = n as i8;
                            break;
                        }
                    }

                    if !is_match {
                        if group_count >= DRCDEC_MAX_CHANNELS {
                            return Err(DrcError::MemoryError);
                        }
                        gain_set_index_for_channel_group[group_count] = idx;
                        unique_scaling[group_count] = factor;
                        *grp_ch = group_count as i8;
                        group_count += 1;
                    }
                } else {
                    *grp_ch = -1;
                }
            }
        }

        if drc_channel_count > DRCDEC_MAX_CHANNELS {
            return Err(DrcError::MemoryError);
        }
    } else {
        // No ducking.
        for (c, grp_ch) in group_for_channel.iter_mut().enumerate().take(channel_count) {
            let mut is_match = false;
            let idx = gain_set_index[c];

            if idx >= 0 {
                for (n, uni_idx) in gain_set_index_for_channel_group
                    .iter()
                    .enumerate()
                    .take(group_count)
                {
                    if *uni_idx == idx {
                        is_match = true;
                        *grp_ch = n as i8;
                        break;
                    }
                }

                if !is_match {
                    if group_count >= DRCDEC_MAX_CHANNELS {
                        return Err(DrcError::MemoryError);
                    }
                    gain_set_index_for_channel_group[group_count] = idx;
                    *grp_ch = group_count as i8;
                    group_count += 1;
                }
            } else {
                *grp_ch = -1;
            }
        }

        if drc_channel_count > DRCDEC_MAX_CHANNELS {
            return Err(DrcError::MemoryError);
        }
    }

    *num_channel_groups = group_count as u8;

    if drc_set_effect & (EffectBit::DuckOther | EffectBit::DuckSelf) != 0 {
        if let Some(dm_for_ch_grp) = dm_for_channel_group {
            for (dm_for_cg, unique_scale) in
                izip!(dm_for_ch_grp.iter_mut(), unique_scaling.iter()).take(group_count)
            {
                dm_for_cg.ducking_scaling = *unique_scale;
                dm_for_cg.is_scaling_present = *unique_scale != 1.0_f32;
            }
        }

        if drc_set_effect & u16::from(EffectBit::DuckOther) != 0 {
            for uni_index in gain_set_index_for_channel_group
                .iter_mut()
                .take(group_count)
            {
                *uni_index = ducking_sequence;
            }
        }
    }

    // Sanity check.
    for uni_index in gain_set_index_for_channel_group.iter().take(group_count) {
        if *uni_index < 0 {
            return Err(DrcError::NotOk);
        }
    }

    Ok(())
}

/// Compare and copy. Returns `true` if `dest` and `src` differs. Otherwise `false`.
pub(super) fn comp_assign<T>(dest: &mut T, src: T) -> bool
where
    T: PartialEq,
{
    let is_diff = *dest != src;
    *dest = src;
    is_diff
}
