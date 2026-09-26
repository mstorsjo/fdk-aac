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
//! Program configuration element (PCE) module
//!
//! PCE is a syntactic element that contains configuration data of a program.

use itertools::izip;

use crate::common::{
    audio_channel_type::AudioChannelType, bitstream::Bitstream, bs_element_id::ChannelElementId,
    crc::CrcInfo,
};

// ISO/IEC 14496-3 4.4.1.1 Table 4.2 Program config element
const PC_FSB_CHANNELS_MAX: usize = 16; // Front/Side/Back channels
const PC_LFE_CHANNELS_MAX: usize = 4;
const PC_ASSOCDATA_MAX: usize = 8;
const PC_CCEL_MAX: usize = 16; // CC elements
const PC_NUM_HEIGHT_LAYER: usize = 3;
const PC_NUM_CHANNEL_TYPE: usize = 3;
// front, side, back
const PCE_HEIGHT_EXT_SYNC: u32 = 0xAC;

const FRONT: usize = 0;
const SIDE: usize = 1;
const BACK: usize = 2;

static CHANNEL_TYPE_TABLE: [[AudioChannelType; PC_NUM_CHANNEL_TYPE]; PC_NUM_HEIGHT_LAYER] = [
    [
        AudioChannelType::Front,
        AudioChannelType::Side,
        AudioChannelType::Back,
    ],
    [
        AudioChannelType::FrontTop,
        AudioChannelType::SideTop,
        AudioChannelType::BackTop,
    ],
    [
        AudioChannelType::FrontBottom,
        AudioChannelType::SideBottom,
        AudioChannelType::BackBottom,
    ],
];

/// PCE comparison results
#[derive(PartialEq, Debug)]
pub enum PceCompareResult {
    Equal = 0,
    CompletelyDifferent,
    DifferentButSameChannelConfig,
    DifferentChannelConfigSameNumberOfChannels,
}

#[derive(Default, Copy, Clone, PartialEq, PartialOrd, Debug)]
#[repr(u8)]
enum HeightInfo {
    #[default]
    NormalHeight = 0,
    TopSpeaker,
    BottomSpeaker,
    Reserved,
}

// Converts u8 to HeightInfo.
impl From<u8> for HeightInfo {
    fn from(value: u8) -> Self {
        match value {
            0 => HeightInfo::NormalHeight,
            1 => HeightInfo::TopSpeaker,
            2 => HeightInfo::BottomSpeaker,
            3 => HeightInfo::Reserved,
            _ => panic!("invalid value: {value}"),
        }
    }
}

// Converts HeightInfo to usize.
impl From<HeightInfo> for usize {
    fn from(value: HeightInfo) -> Self {
        match value {
            HeightInfo::NormalHeight => 0,
            HeightInfo::TopSpeaker => 1,
            HeightInfo::BottomSpeaker => 2,
            HeightInfo::Reserved => 3,
        }
    }
}

/// Program Configuration Element (PCE) structure
#[repr(C)]
#[derive(PartialEq, Debug, Copy, Clone)]
pub struct ProgramConfig {
    // PCE bitstream elements
    element_instance_tag: u8,
    profile: u8,
    sampling_frequency_index: u8,
    num_channel_elements: [u8; PC_NUM_CHANNEL_TYPE],
    num_lfe_channel_elements: u8,
    num_assoc_data_elements: u8,
    num_valid_cc_elements: u8,
    is_mono_mixdown_present: bool,
    mono_mixdown_element_number: u8,
    is_stereo_mixdown_present: bool,
    stereo_mixdown_element_number: u8,
    is_matrix_mixdown_index_present: bool,
    matrix_mixdown_index: u8,
    is_pseudo_surround_enabled: bool,
    // element_is_cpe : true - ChannelElementId::Cpe ; false - ChannelElementId::Sce
    element_is_cpe: [[bool; PC_FSB_CHANNELS_MAX]; PC_NUM_CHANNEL_TYPE],
    element_tag_select: [[u8; PC_FSB_CHANNELS_MAX]; PC_NUM_CHANNEL_TYPE],
    element_height_info: [[HeightInfo; PC_FSB_CHANNELS_MAX]; PC_NUM_CHANNEL_TYPE],
    lfe_element_tag_select: [u8; PC_LFE_CHANNELS_MAX],
    assoc_data_element_tag_select: [u8; PC_ASSOCDATA_MAX],
    cc_element_is_ind_sw: [bool; PC_CCEL_MAX],
    valid_cc_element_tag_select: [u8; PC_CCEL_MAX],
    comment_field_bytes: u8,
    // Helper variables for administration:
    // States whether PCE has been read successfully
    is_valid: bool,
    // Amount of audio channels summing all channel elements including LFEs
    num_channels: u8,
    // Amount of audio channels summing only SCEs and CPEs
    num_effective_channels: u8,
}

impl Default for ProgramConfig {
    fn default() -> Self {
        ProgramConfig {
            element_instance_tag: 0,
            profile: 0,
            sampling_frequency_index: 0,
            num_channel_elements: [0; PC_NUM_CHANNEL_TYPE],
            num_lfe_channel_elements: 0,
            num_assoc_data_elements: 0,
            num_valid_cc_elements: 0,
            is_mono_mixdown_present: false,
            mono_mixdown_element_number: 0,
            is_stereo_mixdown_present: false,
            stereo_mixdown_element_number: 0,
            is_matrix_mixdown_index_present: false,
            matrix_mixdown_index: 0,
            is_pseudo_surround_enabled: false,
            element_is_cpe: [[false; PC_FSB_CHANNELS_MAX]; PC_NUM_CHANNEL_TYPE],
            element_tag_select: [[0; PC_FSB_CHANNELS_MAX]; PC_NUM_CHANNEL_TYPE],
            element_height_info: [[HeightInfo::NormalHeight; PC_FSB_CHANNELS_MAX];
                PC_NUM_CHANNEL_TYPE],
            lfe_element_tag_select: [0; PC_LFE_CHANNELS_MAX],
            assoc_data_element_tag_select: [0; PC_ASSOCDATA_MAX],
            cc_element_is_ind_sw: [false; PC_CCEL_MAX],
            valid_cc_element_tag_select: [0; PC_CCEL_MAX],
            comment_field_bytes: 0,
            is_valid: false,
            num_channels: 0,
            num_effective_channels: 0,
        }
    }
}

impl ProgramConfig {
    /// Creates a new `ProgramConfig` instance (PCE).
    pub fn new() -> Self {
        ProgramConfig::default()
    }

    /// Initializes program config element (PCE).
    pub fn init(&mut self) {
        self.reset();
        self.sampling_frequency_index = 0xf;
        self.element_instance_tag = 255;
    }

    /// Resets program config element (PCE).
    pub fn reset(&mut self) {
        self.element_instance_tag = 0;
        self.profile = 0;
        self.sampling_frequency_index = 0;
        self.num_channel_elements.fill_with(Default::default);
        self.num_lfe_channel_elements = 0;
        self.num_assoc_data_elements = 0;
        self.num_valid_cc_elements = 0;
        self.is_mono_mixdown_present = false;
        self.mono_mixdown_element_number = 0;
        self.is_stereo_mixdown_present = false;
        self.stereo_mixdown_element_number = 0;
        self.is_matrix_mixdown_index_present = false;
        self.matrix_mixdown_index = 0;
        self.is_pseudo_surround_enabled = false;
        self.element_is_cpe.fill_with(Default::default);
        self.element_tag_select.fill_with(Default::default);
        self.element_height_info.fill_with(Default::default);
        self.lfe_element_tag_select.fill_with(Default::default);
        self.assoc_data_element_tag_select
            .fill_with(Default::default);
        self.cc_element_is_ind_sw.fill_with(Default::default);
        self.valid_cc_element_tag_select.fill_with(Default::default);
        self.comment_field_bytes = 0;
        self.is_valid = false;
        self.num_channels = 0;
        self.num_effective_channels = 0;
    }

    /// Inquires state of present program config element (PCE).
    ///
    /// Returns true if the PCE structure is filled correct,
    ///         false if no valid PCE present.
    pub fn is_valid(&self) -> bool {
        self.is_valid
    }

    /// Returns `true` if `matrix mixdown index` is present, otherwise `false`.
    pub(crate) fn is_matrix_mixdown_index_present(&self) -> bool {
        self.is_matrix_mixdown_index_present
    }

    /// Returns the value of `matrix mixdown index`.
    pub(crate) fn matrix_mixdown_index(&self) -> u8 {
        self.matrix_mixdown_index
    }

    /// Returns `true` if `pseudo surround` is enabled, otherwise `false`.
    pub(crate) fn is_pseudo_surround_enabled(&self) -> bool {
        self.is_pseudo_surround_enabled
    }

    /// Sets `matrix mixdown index present` flag.
    pub(crate) fn set_matrix_mixdown_index_present(&mut self, value: bool) {
        self.is_matrix_mixdown_index_present = value;
    }

    /// Sets the value of `matrix mixdown index`.
    pub(crate) fn set_matrix_mixdown_index(&mut self, index_value: u8) {
        self.matrix_mixdown_index = index_value;
    }

    /// Sets the `pseudo surround enabled` flag.
    pub(crate) fn set_pseudo_surround_enabled(&mut self, value: bool) {
        self.is_pseudo_surround_enabled = value;
    }

    /// Returns the value of `element instance tag`.
    pub(crate) fn element_instance_tag(&self) -> u8 {
        self.element_instance_tag
    }

    /// Compares two (PCE) program configurations.
    ///
    /// # Parameters:
    ///
    /// - `other_pce`: Second PCEs instance
    ///
    /// Returns `PceCompareResult` - the result of the comparison,
    ///
    ///  - `PceCompareResult::CompletelyDifferent` - if PCEs are completely different
    ///  - `PceCompareResult::Equal` - if PCEs are completely equal
    ///  - `PceCompareResult::DifferentButSameChannelConfig` - if PCEs are different but have the
    ///    same channel config
    ///  - `PceCompareResult::DifferentChannelConfigSameNumberOfChannels` - if PCEs have different
    ///    channel config but same number of channels
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::tp_dec::{PceCompareResult, ProgramConfig};
    /// let mut first_pce = ProgramConfig::new();
    /// first_pce.init();
    /// let second_pce = ProgramConfig::new();
    /// let result = first_pce.compare(&second_pce);
    /// assert!(result == PceCompareResult::DifferentButSameChannelConfig);
    /// ```
    pub fn compare(&self, other_pce: &ProgramConfig) -> PceCompareResult {
        let mut result = PceCompareResult::Equal; // Innocent until proven false.

        if *self != *other_pce {
            // Configurations are not completely equal.
            // So look into details and analyse the channel configurations.
            result = PceCompareResult::CompletelyDifferent;

            if self.num_channels == other_pce.num_channels {
                // Now the logic changes. We first assume to have
                // the same channel configuration and then prove
                // if this assumption is true.
                result = PceCompareResult::DifferentButSameChannelConfig;

                for (fsb1, num_ch_ele1, cpe_ele1, fsb2, num_ch_ele2, cpe_ele2) in izip!(
                    self.element_height_info.iter(),
                    self.num_channel_elements.iter(),
                    self.element_is_cpe.iter(),
                    other_pce.element_height_info.iter(),
                    other_pce.num_channel_elements.iter(),
                    other_pce.element_is_cpe.iter(),
                )
                .take(PC_NUM_CHANNEL_TYPE)
                {
                    if *num_ch_ele1 != *num_ch_ele2 {
                        // different number of channel elements
                        result = PceCompareResult::DifferentChannelConfigSameNumberOfChannels;
                    } else {
                        let (mut num_ch1, mut num_ch2) = (0, 0);
                        'height_info: for (hi1, cpe1, hi2, cpe2) in
                            izip!(fsb1.iter(), cpe_ele1.iter(), fsb2.iter(), cpe_ele2.iter())
                        {
                            if *hi1 != *hi2 {
                                // different height info
                                result =
                                    PceCompareResult::DifferentChannelConfigSameNumberOfChannels;
                                break 'height_info;
                            }
                            num_ch1 += if *cpe1 { 2 } else { 1 };
                            num_ch2 += if *cpe2 { 2 } else { 1 };
                        }

                        if num_ch1 != num_ch2 {
                            // different number of channels
                            result = PceCompareResult::DifferentChannelConfigSameNumberOfChannels;
                        }
                    }
                }

                // LFE channels
                if self.num_lfe_channel_elements != other_pce.num_lfe_channel_elements {
                    // different number of lfe channels
                    result = PceCompareResult::DifferentChannelConfigSameNumberOfChannels;
                }
                // LFEs are always SCEs so we don't need to count the channels.
            }
        }
        result
    }

    /// Gets a program config element (PCE) that matches the predefined
    /// MPEG-4 channel configurations 1-14.
    ///
    /// # Parameters:
    ///
    /// - `channel_config`: MPEG-4 channel configuration
    ///
    /// Returns true if program config is a valid
    /// MPEG-4 channel configuration (1-14), else false
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::tp_dec::ProgramConfig;
    /// let mut pce = ProgramConfig::new();
    /// pce.init();
    /// let channel_config = 12;
    /// let result = pce.get_default_mpeg_config(channel_config);
    /// assert!(result);
    /// ```
    pub fn get_default_mpeg_config(&mut self, channel_config: u32) -> bool {
        if channel_config > 14 {
            false
        } else {
            self.get_default_config(channel_config);
            self.is_valid
        }
    }

    /// Gets a program config element (PCE) that matches the predefined
    /// MPEG-4 channel configurations 1-14 and helper configs like
    /// 16, 21, 30, 32.
    ///
    /// # Parameters:
    ///
    /// - `channel_config`: MPEG-4 channel configuration
    pub fn get_default_config(&mut self, channel_config: u32) {
        // Init PCE
        self.init();
        // Set AAC LC because it is the only supported object type
        self.profile = 1;

        match channel_config {
            // Helper configs used by PCE only, not exposed to outside PCE
            // - - - - - - - - - - - Helper Default configs - Starts- - - - - - - - - - - //
            32 => {
                // 7.1 side channel configuration
                self.num_channel_elements[FRONT] = 2;
                self.element_is_cpe[FRONT][0] = false;
                self.element_is_cpe[FRONT][1] = true;
                self.num_channel_elements[SIDE] = 1;
                self.element_is_cpe[SIDE][0] = true;
                self.num_channel_elements[BACK] = 1;
                self.element_is_cpe[BACK][0] = true;
                self.num_lfe_channel_elements = 1;
                self.num_channels = 8;
                self.num_effective_channels = 7;
                self.is_valid = true;
            }
            30 => {
                // 2/1 ARIB
                self.num_channel_elements[FRONT] = 1;
                self.element_is_cpe[FRONT][0] = true;
                self.num_channel_elements[SIDE] = 0;
                self.num_channel_elements[BACK] = 1;
                self.element_is_cpe[BACK][0] = false;
                self.num_channels = 3;
                self.num_effective_channels = 3;
                self.is_valid = true;
            }
            21 => {
                // 2/2 ARIB
                self.num_channel_elements[FRONT] = 1;
                self.element_is_cpe[FRONT][0] = true;
                self.num_channel_elements[SIDE] = 0;
                self.num_channel_elements[BACK] = 1;
                self.element_is_cpe[BACK][0] = true;
                self.num_channels = 4;
                self.num_effective_channels = 4;
                self.is_valid = true;
            }
            16 => {
                // dual-mono
                self.num_channel_elements[FRONT] = 2;
                self.element_is_cpe[FRONT][0] = false;
                self.element_is_cpe[FRONT][1] = false;
                self.num_channel_elements[SIDE] = 0;
                self.num_channel_elements[BACK] = 0;
                self.num_channels = 2;
                self.num_effective_channels = 2;
                self.is_valid = true;
            }
            // - - - - - - - - - - - Helper Default configs - Ends - - - - - - - - - - //
            11 | 12 => {
                if channel_config == 12 {
                    // 3/0/4.1ch surround back
                    self.element_is_cpe[BACK][1] = true;
                    self.num_channels += 1;
                    self.num_effective_channels += 1;
                }

                // 3/0/3.1ch
                self.num_channel_elements[FRONT] += 2;
                self.element_is_cpe[FRONT][0] = false;
                self.element_is_cpe[FRONT][1] = true;
                self.num_channel_elements[BACK] += 2;
                self.element_is_cpe[BACK][0] = true;
                // self.element_is_cpe[BACK][1] += 0;
                self.num_lfe_channel_elements += 1;
                self.num_channels += 7;
                self.num_effective_channels += 6;
                self.is_valid = true;
            }
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - -  //
            14 | 7 | 6 | 5 | 4 | 3 | 1 => {
                let mut fall_through_flag = false;
                if channel_config == 14 {
                    // 2/0/0-3/0/2-0.1ch front height
                    self.element_height_info[FRONT][2] = HeightInfo::TopSpeaker; // Top speaker
                    fall_through_flag = true;
                }
                if channel_config == 7 || fall_through_flag {
                    // 5/0/2.1ch front
                    self.num_channel_elements[FRONT] += 1;
                    self.element_is_cpe[FRONT][2] = true;
                    self.num_channels += 2;
                    self.num_effective_channels += 2;
                    fall_through_flag = true;
                }
                if channel_config == 6 || fall_through_flag {
                    // 3/0/2.1ch
                    self.num_lfe_channel_elements += 1;
                    self.num_channels += 1;
                    fall_through_flag = true;
                }
                if channel_config == 5 || channel_config == 4 || fall_through_flag {
                    // 3/0/1.0ch (channel = 4) 3/0/2.0ch (channel = 5)
                    let num_ch = if channel_config > 4 { 2 } else { 1 };
                    self.num_channel_elements[BACK] += 1;
                    self.element_is_cpe[BACK][0] = (num_ch - 1) != 0;
                    self.num_channels += num_ch;
                    self.num_effective_channels += num_ch;
                    fall_through_flag = true;
                }
                if channel_config == 3 || fall_through_flag {
                    // 3/0/0.0ch
                    self.num_channel_elements[FRONT] += 1;
                    self.element_is_cpe[FRONT][1] = true;
                    self.num_channels += 2;
                    self.num_effective_channels += 2;
                    fall_through_flag = true;
                }
                if channel_config == 1 || fall_through_flag {
                    // 1/0/0.0ch
                    self.num_channel_elements[FRONT] += 1;
                    self.element_is_cpe[FRONT][0] = false;
                    self.num_channels += 1;
                    self.num_effective_channels += 1;
                    self.is_valid = true;
                }
            }
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
            2 => {
                // 2/0/0.ch
                self.num_channel_elements[FRONT] = 1;
                self.element_is_cpe[FRONT][0] = true;
                self.num_channels += 2;
                self.num_effective_channels += 2;
                self.is_valid = true;
            }
            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
            _ => {
                // To be explicit!
                self.is_valid = false;
            }
        }

        if self.is_valid {
            // Create valid element instance tags
            let (mut el_tag_sce, mut el_tag_cpe, mut el_tag_lfe) = (0, 0, 0);

            for (tag_select, num_channel_ele, cpe_ele) in izip!(
                self.element_tag_select.iter_mut(),
                self.num_channel_elements.iter(),
                self.element_is_cpe.iter()
            )
            .take(PC_NUM_CHANNEL_TYPE)
            {
                izip!(tag_select.iter_mut(), cpe_ele.iter())
                    .take(usize::from(*num_channel_ele))
                    .for_each(|(ts, cpe)| {
                        if *cpe {
                            *ts = el_tag_cpe;
                            el_tag_cpe += 1;
                        } else {
                            *ts = el_tag_sce;
                            el_tag_sce += 1;
                        }
                    })
            }

            for lfe in self
                .lfe_element_tag_select
                .iter_mut()
                .take(usize::from(self.num_lfe_channel_elements))
            {
                *lfe = el_tag_lfe;
                el_tag_lfe += 1;
            }
        }
    }

    /// Reads the extension for height info.
    ///
    /// # Parameters:
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `bytes_available`: Number of comment bytes available for PCE
    /// - `alignment_anchor`: A bit position to which byte alignment gets applied
    ///
    /// Returns 0 if successful,
    ///        -1 if the CRC failed,
    ///        -2 if invalid height info.
    fn read_height_extension(
        &mut self,
        bs: &mut Bitstream,
        bytes_available: &mut i32,
        alignment_anchor: isize,
    ) -> i32 {
        let mut error = 0;
        // CRC state info
        let mut crc_info = CrcInfo::new();
        crc_info.init(0x07, 0xFF, 8);

        let crc_reg = crc_info.start_region(bs, 0);

        let start_anchor = bs.valid_bits();

        if (start_anchor >= 24) && (*bytes_available >= 3) && (bs.read(8) == PCE_HEIGHT_EXT_SYNC) {
            for (fsb, num_ele) in self
                .element_height_info
                .iter_mut()
                .zip(self.num_channel_elements.iter())
                .take(PC_NUM_CHANNEL_TYPE)
            {
                for ehi in fsb.iter_mut().take(usize::from(*num_ele)) {
                    *ehi = HeightInfo::from(bs.read(2) as u8);
                    // if *ehi >= PC_NUM_HEIGHT_LAYER as u8 {
                    if *ehi > HeightInfo::BottomSpeaker {
                        // Height information is out of the valid range.
                        error = -2;
                    }
                }
            }

            bs.align(alignment_anchor);
            crc_info.end_region(bs, crc_reg);

            if bs.read(8) != crc_info.value() {
                // CRC failed
                error = -1;
            }
            if error != 0 {
                // Reset whole height information in case an error occurred during parsing.
                // The return value ensures that "self.is_valid" is set to false and implicit
                // channel mapping is used.
                self.element_height_info
                    .iter_mut()
                    .for_each(|c| c.fill(HeightInfo::NormalHeight));
            }
        } else {
            // No valid extension data found -> restore the initial bitbuffer state
            let valid_bits = bs.valid_bits();
            bs.push(start_anchor - valid_bits);
        }

        // Always report the bytes read
        *bytes_available -= ((start_anchor - bs.valid_bits()) >> 3) as i32;

        error
    }

    /// Sanity checks for program config element.
    /// Checks order of elements according to ISO/IEC 13818-7:2003(E),
    /// chapter 8.5.1
    ///
    /// Returns true if successful, otherwise false.
    fn check(&mut self) -> bool {
        let mut num_channels = [[0_i32; PC_NUM_CHANNEL_TYPE]; PC_NUM_HEIGHT_LAYER];

        let mut is_cpe_idx = [0_usize; PC_NUM_CHANNEL_TYPE];

        for (ch_idx, (fsb, num_ch_ele, cpe_ele)) in izip!(
            self.element_height_info.iter(),
            self.num_channel_elements.iter(),
            self.element_is_cpe.iter()
        )
        .take(PC_NUM_CHANNEL_TYPE)
        .enumerate()
        {
            izip!(fsb.iter(), cpe_ele.iter())
                .take(usize::from(*num_ch_ele))
                .for_each(|(hi, cpe)| {
                    num_channels[usize::from(*hi)][ch_idx] += if *cpe { 2 } else { 1 };
                })
        }

        // 0 = normal height channels, 1 = top height channels,
        // 2 = bottom height channels

        for (hl, num_chs) in num_channels
            .iter_mut()
            .take(PC_NUM_HEIGHT_LAYER)
            .enumerate()
        {
            // If number of channels is odd => first element must be an SCE
            // (front center channel).
            if num_chs[FRONT] & 1 != 0 {
                is_cpe_idx[FRONT] += 1;
                if self.element_is_cpe[FRONT][is_cpe_idx[FRONT] - 1] {
                    return false;
                }

                num_chs[FRONT] -= 1;
            }
            while num_chs[FRONT] > 0 {
                // must be CPE or paired SCE
                is_cpe_idx[FRONT] += 1;
                if !self.element_is_cpe[FRONT][is_cpe_idx[FRONT] - 1] {
                    is_cpe_idx[FRONT] += 1;
                    if self.element_is_cpe[FRONT][is_cpe_idx[FRONT] - 1] {
                        return false;
                    }
                }
                num_chs[FRONT] -= 2;
            }

            // In case that a top center surround channel (Ts) is transmitted the number
            // of channels can be odd.
            if hl != 1 {
                // number of channels must be even
                if num_chs[SIDE] & 1 != 0 {
                    return false;
                }
                while num_chs[SIDE] > 0 {
                    // must be CPE or paired SCE
                    is_cpe_idx[SIDE] += 1;
                    if !self.element_is_cpe[SIDE][is_cpe_idx[SIDE] - 1] {
                        is_cpe_idx[SIDE] += 1;
                        if self.element_is_cpe[SIDE][is_cpe_idx[SIDE] - 1] {
                            return false;
                        }
                    }
                    num_chs[SIDE] -= 2;
                }
            }

            while num_chs[BACK] > 1 {
                // must be CPE or paired SCE
                is_cpe_idx[BACK] += 1;
                if !self.element_is_cpe[BACK][is_cpe_idx[BACK] - 1] {
                    is_cpe_idx[BACK] += 1;
                    if self.element_is_cpe[BACK][is_cpe_idx[BACK] - 1] {
                        return false;
                    }
                }
                num_chs[BACK] -= 2;
            }
            // If number of channels is odd => last element must be an SCE
            // (back center channel).
            if num_chs[BACK] > 0 {
                is_cpe_idx[BACK] += 1;
                if self.element_is_cpe[BACK][is_cpe_idx[BACK] - 1] {
                    return false;
                }
            }
        }
        true
    }

    /// Gets list of elements in canonical order from a
    /// given program config field.
    ///
    /// # Parameters:
    ///
    /// - `element_list`: Output buffer where the element IDs are stored, buffer length should be
    ///   constants::MAX_ELEMENTS_IDX
    ///
    /// Returns valid number of elements count in output buffer.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::{common::bs_element_id::ChannelElementId, tp_dec::ProgramConfig};
    /// let mut pce = ProgramConfig::new();
    /// pce.init();
    /// let channel_config = 12;
    /// let result = pce.get_default_mpeg_config(channel_config);
    /// let mut element_list = [ChannelElementId::None; 8];
    /// let count = pce.get_element_list(&mut element_list[..]);
    /// assert!(result);
    /// assert!(count == 5); // Sce, Cpe, Cpe, Cpe, Lfe
    /// ```
    pub fn get_element_list(&self, element_list: &mut [ChannelElementId]) -> usize {
        let mut el_count = 0;
        let el_list_size = element_list.len();

        if (el_list_size
            >= usize::from(
                self.num_channel_elements[FRONT]
                    + self.num_channel_elements[SIDE]
                    + self.num_channel_elements[BACK]
                    + self.num_lfe_channel_elements,
            ))
            && (self.num_channels > 0)
        {
            for hl in 0..PC_NUM_HEIGHT_LAYER as u8 {
                for (fsb, num_channel_ele, cpe_ele) in izip!(
                    self.element_height_info.iter(),
                    self.num_channel_elements.iter(),
                    self.element_is_cpe.iter()
                )
                .take(PC_NUM_CHANNEL_TYPE)
                {
                    for (hi, cpe) in
                        izip!(fsb.iter(), cpe_ele.iter()).take(usize::from(*num_channel_ele))
                    {
                        if *hi == HeightInfo::from(hl) {
                            element_list[el_count] = if *cpe {
                                ChannelElementId::Cpe
                            } else {
                                ChannelElementId::Sce
                            };
                            el_count += 1;
                        }
                    }
                }
                if hl == 0 {
                    element_list[el_count..el_count + self.num_lfe_channel_elements as usize]
                        .fill(ChannelElementId::Lfe);
                    el_count += self.num_lfe_channel_elements as usize;
                }
            }
        }

        element_list[el_count..].fill(ChannelElementId::None);

        el_count
    }

    /// Validates presence of non-audio element instance tag in PCE.
    ///
    /// # Parameters:
    ///
    /// - `tag`: Element instance tag value
    /// - `element_type`: MPEG-4 audio channel type (Element ID)
    ///
    /// Returns true if tag presence in PCE, else false.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::{common::bs_element_id::ChannelElementId, tp_dec::ProgramConfig};
    /// let mut pce = ProgramConfig::new();
    /// let element_type = ChannelElementId::Cce;
    /// let tag = 2;
    /// let result = pce.validate_non_audio_element_instance_tag(tag, element_type);
    /// assert!(!result);
    /// ```
    pub fn validate_non_audio_element_instance_tag(
        &self,
        tag: u8,
        element_type: ChannelElementId,
    ) -> bool {
        if self.is_valid {
            match element_type {
                ChannelElementId::Cce => {
                    // search in cce channels
                    for cce in self
                        .valid_cc_element_tag_select
                        .iter()
                        .take(self.num_valid_cc_elements as usize)
                    {
                        if *cce == tag {
                            return true;
                        }
                    }
                }
                ChannelElementId::Dse => {
                    // exception for pce without dse entry
                    if self.num_assoc_data_elements == 0 {
                        return true;
                    }
                    // search associated data elements
                    for assoc in self
                        .assoc_data_element_tag_select
                        .iter()
                        .take(self.num_assoc_data_elements as usize)
                    {
                        if *assoc == tag {
                            return true;
                        }
                    }
                }
                _ => {}
            }
            false // not found in any list
        } else {
            false // no valid pce
        }
    }

    /// Get element associated instance tag based on given element index and element type.
    ///
    /// # Parameters:
    ///
    /// - `element_index`: Index value of element in list
    /// - `element_type`: MPEG-4 audio channel type (Element ID)
    ///
    /// Returns element tag if found, else 255 (default value).
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::{common::bs_element_id::ChannelElementId, tp_dec::ProgramConfig};
    /// let mut pce = ProgramConfig::new();
    /// let channel_config = 5;
    /// let result = pce.get_default_mpeg_config(channel_config);
    /// let mut element_list = [ChannelElementId::None; 8];
    /// let count = pce.get_element_list(&mut element_list[..]); // [Sce, Cpe, Cpe, None, None...]
    /// let element_type = ChannelElementId::Cpe;
    /// let element_index = 2;
    /// let assoc_tag = pce.get_element_associated_instance_tag(element_index, element_type);
    /// assert!(result);
    /// assert!(assoc_tag == 1);
    /// ```
    pub fn get_element_associated_instance_tag(
        &self,
        element_index: usize,
        element_type: ChannelElementId,
    ) -> u8 {
        if self.is_valid {
            let mut ele_count = 0;
            for hl in 0..PC_NUM_HEIGHT_LAYER as u8 {
                for (fsb_index, (fsb, num_channel_ele, cpe_ele)) in izip!(
                    self.element_height_info.iter(),
                    self.num_channel_elements.iter(),
                    self.element_is_cpe.iter()
                )
                .take(PC_NUM_CHANNEL_TYPE)
                .enumerate()
                {
                    for (ch_index, (hi, cpe)) in izip!(fsb.iter(), cpe_ele.iter())
                        .take(usize::from(*num_channel_ele))
                        .enumerate()
                    {
                        if *hi == HeightInfo::from(hl) {
                            if (ele_count == element_index)
                                && (element_type
                                    == if *cpe {
                                        ChannelElementId::Cpe
                                    } else {
                                        ChannelElementId::Sce
                                    })
                            {
                                return self.element_tag_select[fsb_index][ch_index];
                            }
                            ele_count += 1;
                        }
                    }
                }

                // Normal height
                if hl == 0 {
                    for ch_index in 0..self.num_lfe_channel_elements {
                        if ele_count == element_index && element_type == ChannelElementId::Lfe {
                            return self.lfe_element_tag_select[ch_index as usize];
                        }
                        ele_count += 1;
                    }
                }
            }
        }
        255
    }

    /// Returns the corresponding implicit channel configuration
    /// index of the PCE. If none can be found it returns the value 0.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::tp_dec::ProgramConfig;
    /// let mut pce = ProgramConfig::new();
    /// let index = pce.get_channel_map_index();
    /// assert!(index == 0);
    /// ```
    pub fn get_channel_map_index(&self) -> u8 {
        let mut ch_map_idx = 0;
        let mut tmp_pce = ProgramConfig::new();

        // Find a corresponding channel configuration if possible
        match self.num_channels {
            1 | 2 => {
                // One and two channels have no alternatives.
                ch_map_idx = self.num_channels;
            }
            3 => {
                for cfg in [30, 3] {
                    // Create a PCE for the config to test ...
                    tmp_pce.get_default_config(u32::from(cfg));
                    // ... and compare it with the given one.
                    let cmp_result = self.compare(&tmp_pce);
                    if cmp_result == PceCompareResult::Equal
                        || cmp_result == PceCompareResult::DifferentButSameChannelConfig
                    {
                        // Explicit mapping of ARIB 2/1 to CICP 9.
                        ch_map_idx = if cfg == 30 { 9 } else { cfg };
                    }
                }
            }
            4 => {
                for cfg in [21, 4] {
                    // Create a PCE for the config to test ...
                    tmp_pce.get_default_config(u32::from(cfg));
                    // ... and compare it with the given one.
                    let cmp_result = self.compare(&tmp_pce);
                    if cmp_result == PceCompareResult::Equal
                        || cmp_result == PceCompareResult::DifferentButSameChannelConfig
                    {
                        // Explicit mapping of ARIB 2/2 to CICP 10.
                        ch_map_idx = if cfg == 21 { 10 } else { cfg };
                    }
                }
            }
            5 | 6 => {
                // Test if the number of channels can be used as channel config:
                // Create a PCE for the config to test ...
                tmp_pce.get_default_config(self.num_channels.into());
                // ... and compare it with the given one.
                let cmp_result = self.compare(&tmp_pce);

                if cmp_result == PceCompareResult::Equal
                    || cmp_result == PceCompareResult::DifferentButSameChannelConfig
                {
                    ch_map_idx = self.num_channels;
                }
            }
            7 => {
                // Create a PCE for the config to test ...
                tmp_pce.get_default_config(11);
                // ... and compare it with the given one.
                let cmp_result = self.compare(&tmp_pce);

                if cmp_result == PceCompareResult::Equal
                    || cmp_result == PceCompareResult::DifferentButSameChannelConfig
                {
                    ch_map_idx = 11;
                }
                // If compare result is Equal or DifferentButSameChannelConfig
                // we can be sure that it is channel config 11.
            }
            8 => {
                // Try the four possible 7.1ch configurations. One after the other.
                for cfg in [32, 14, 12, 7] {
                    // Create a PCE for the config to test ...
                    tmp_pce.get_default_config(u32::from(cfg));
                    // ... and compare it with the given one.
                    let cmp_result = self.compare(&tmp_pce);
                    if cmp_result == PceCompareResult::Equal
                        || cmp_result == PceCompareResult::DifferentButSameChannelConfig
                    {
                        // Explicit mapping of 7.1 side channel configuration to 7.1 rear
                        // channel mapping.
                        ch_map_idx = if cfg == 32 { 12 } else { cfg };
                    }
                }
            }
            _ => {
                // The PCE does not match any predefined channel configuration.
            }
        }
        ch_map_idx
    }

    /// Gets channel description (type and index) in MPEG canonical order.
    ///
    /// # Parameters:
    ///
    /// - `channel_type`: Output buffer to store the audio channel type
    /// - `channel_index`: Output buffer to store the individual audio channel type index
    ///
    /// Length of buffers should be constants::MAX_CHANNELS.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::{
    ///     aac_dec::constants::MAX_CHANNELS, common::audio_channel_type::AudioChannelType,
    ///     tp_dec::ProgramConfig,
    /// };
    /// let mut pce = ProgramConfig::new();
    /// pce.init();
    /// let channel_config = 12;
    /// let result = pce.get_default_mpeg_config(channel_config);
    /// let mut channel_type = [AudioChannelType::None; MAX_CHANNELS];
    /// let mut channel_index = [0_u8; MAX_CHANNELS];
    ///
    /// pce.get_channel_description(&mut channel_type, &mut channel_index);
    /// assert!(
    ///     [
    ///         AudioChannelType::Front,
    ///         AudioChannelType::Front,
    ///         AudioChannelType::Front,
    ///         AudioChannelType::Back,
    ///         AudioChannelType::Back,
    ///         AudioChannelType::Back,
    ///         AudioChannelType::Back,
    ///         AudioChannelType::Lfe
    ///     ] == channel_type
    /// );
    /// assert!([0, 1, 2, 0, 1, 2, 3, 0] == channel_index);
    /// ```
    pub fn get_channel_description(
        &self,
        channel_type: &mut [AudioChannelType],
        channel_index: &mut [u8],
    ) {
        if self.is_valid
            && (usize::from(self.num_channels) <= channel_type.len())
            && (usize::from(self.num_channels) <= channel_index.len())
        {
            let mut ch = 0;
            for (hl, ch_type_table) in CHANNEL_TYPE_TABLE
                .iter()
                .take(PC_NUM_HEIGHT_LAYER)
                .enumerate()
            {
                for (ch_index, (fsb, num_channel_ele, cpe_ele)) in izip!(
                    self.element_height_info.iter(),
                    self.num_channel_elements.iter(),
                    self.element_is_cpe.iter()
                )
                .take(PC_NUM_CHANNEL_TYPE)
                .enumerate()
                {
                    let mut grp_ch_index = 0;
                    for (hi, cpe) in
                        izip!(fsb.iter(), cpe_ele.iter()).take(usize::from(*num_channel_ele))
                    {
                        if *hi == HeightInfo::from(hl as u8) {
                            channel_type[ch] = ch_type_table[ch_index];
                            channel_index[ch] = grp_ch_index;
                            ch += 1;
                            grp_ch_index += 1;

                            if *cpe {
                                channel_type[ch] = ch_type_table[ch_index];
                                channel_index[ch] = grp_ch_index;
                                ch += 1;
                                grp_ch_index += 1;
                            }
                        }
                    }
                }

                if hl == 0 {
                    izip!(
                        channel_type[ch..ch + self.num_lfe_channel_elements as usize].iter_mut(),
                        channel_index[ch..ch + self.num_lfe_channel_elements as usize].iter_mut(),
                    )
                    .take(self.num_lfe_channel_elements as usize)
                    .enumerate()
                    .for_each(|(grp_index, (ch_type, ch_index))| {
                        *ch_type = AudioChannelType::Lfe;
                        *ch_index = grp_index as u8;
                    });
                    ch += self.num_lfe_channel_elements as usize;
                }
            }
        } else {
            channel_type.fill(AudioChannelType::None);
            channel_index.fill(0);
        }
    }

    /// Returns the profile object type index.
    pub fn profile(&self) -> u8 {
        self.profile
    }

    /// Returns the sampling frequency index.
    pub fn sampling_frequency_index(&self) -> u8 {
        self.sampling_frequency_index
    }

    /// Returns the number of audio channels.
    pub fn num_channels(&self) -> u8 {
        self.num_channels
    }

    /// Reads program config element (PCE) from bitstream.
    ///
    /// # Parameters:
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `alignment_anchor`: A bit position to which byte alignment gets applied
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::{
    ///     common::bitstream::{Bitstream, Mode},
    ///     tp_dec::ProgramConfig,
    /// };
    ///
    /// // Precondition: There has to be a valid Bitstream instance.
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 50);
    /// let mut pce = ProgramConfig::new();
    /// pce.init();
    /// let align_anchor = bitstream_reader.valid_bits();
    /// pce.read(&mut bitstream_reader, align_anchor);
    /// ```
    pub fn read(&mut self, bs: &mut Bitstream, alignment_anchor: isize) {
        let mut check_element_tag_select = [[0_u8; PC_FSB_CHANNELS_MAX]; 3];

        self.is_valid = true;
        self.num_effective_channels = 0;
        self.num_channels = 0;
        self.element_instance_tag = bs.read(4) as u8;
        self.profile = bs.read(2) as u8;
        self.sampling_frequency_index = bs.read(4) as u8;
        self.num_channel_elements[FRONT] = bs.read(4) as u8;
        self.num_channel_elements[SIDE] = bs.read(4) as u8;
        self.num_channel_elements[BACK] = bs.read(4) as u8;
        self.num_lfe_channel_elements = bs.read(2) as u8;
        self.num_assoc_data_elements = bs.read(3) as u8;
        self.num_valid_cc_elements = bs.read(4) as u8;

        self.is_mono_mixdown_present = bs.read_bit() != 0;
        if self.is_mono_mixdown_present {
            self.mono_mixdown_element_number = bs.read(4) as u8;
        }

        self.is_stereo_mixdown_present = bs.read_bit() != 0;
        if self.is_stereo_mixdown_present {
            self.stereo_mixdown_element_number = bs.read(4) as u8;
        }

        self.is_matrix_mixdown_index_present = bs.read_bit() != 0;
        if self.is_matrix_mixdown_index_present {
            self.matrix_mixdown_index = bs.read(2) as u8;
            self.is_pseudo_surround_enabled = bs.read_bit() != 0;
        }

        for (tag_select, num_ch_ele, cpe_ele) in izip!(
            self.element_tag_select.iter_mut(),
            self.num_channel_elements.iter(),
            self.element_is_cpe.iter_mut()
        )
        .take(PC_NUM_CHANNEL_TYPE)
        {
            for (tag, cpe) in
                izip!(tag_select.iter_mut(), cpe_ele.iter_mut()).take(usize::from(*num_ch_ele))
            {
                *cpe = bs.read_bit() != 0;
                *tag = bs.read(4) as u8;

                self.num_channels += if *cpe { 2 } else { 1 };

                // Check element instance tag according to ISO/IEC 13818-7:2003(E),
                // chapter 8.2.1.1.
                if check_element_tag_select[*cpe as usize][*tag as usize] == 0 {
                    check_element_tag_select[*cpe as usize][*tag as usize] = 1;
                } else {
                    self.is_valid = false;
                }
            }
        }

        self.num_effective_channels = self.num_channels;

        for lfe_tag in self
            .lfe_element_tag_select
            .iter_mut()
            .take(self.num_lfe_channel_elements as usize)
        {
            *lfe_tag = bs.read(4) as u8;
            self.num_channels += 1;

            // Check element instance tag according to ISO/IEC 13818-7:2003(E),
            // chapter 8.2.1.1
            if check_element_tag_select[2][*lfe_tag as usize] == 0 {
                check_element_tag_select[2][*lfe_tag as usize] = 1;
            } else {
                self.is_valid = false;
            }
        }

        for assoc_data in self
            .assoc_data_element_tag_select
            .iter_mut()
            .take(self.num_assoc_data_elements as usize)
        {
            *assoc_data = bs.read(4) as u8;
        }

        for (is_ind, cc_tag) in izip!(
            self.cc_element_is_ind_sw.iter_mut(),
            self.valid_cc_element_tag_select.iter_mut(),
        )
        .take(self.num_valid_cc_elements as usize)
        {
            *is_ind = bs.read_bit() != 0;
            *cc_tag = bs.read(4) as u8;
        }

        bs.align(alignment_anchor);

        self.comment_field_bytes = bs.read(8) as u8;
        let mut comment_bytes = self.comment_field_bytes as i32;

        // Search for height info extension and read it if available.
        if self.read_height_extension(bs, &mut comment_bytes, alignment_anchor) != 0 {
            self.is_valid = false; // Invalid
        }

        // Only support AAC LC object type
        if self.profile != 1 {
            self.is_valid = false;
        }

        // Skip comment_bytes
        bs.push((8 * comment_bytes) as isize);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{aac_dec::constants::MAX_CHANNELS, common::bitstream::Mode};

    #[test]
    fn new() {
        let pce = ProgramConfig::new();
        assert!(!pce.is_valid());
    }

    #[test]
    fn init() {
        let mut pce = ProgramConfig::new();

        assert_eq!(pce.sampling_frequency_index, 0);
        assert_eq!(pce.element_instance_tag, 0);
        pce.init();

        assert_eq!(pce.sampling_frequency_index, 0xf);
        assert_eq!(pce.element_instance_tag, 255);
    }

    #[test]
    fn is_valid() {
        let mut pce = ProgramConfig::new();
        assert!(!pce.is_valid());

        // set some valid values 1, 2
        pce.is_valid = true;
        assert!(pce.is_valid());
    }

    #[test]
    fn compare() {
        let mut pce1 = ProgramConfig::new();
        let mut pce2 = ProgramConfig::new();

        // completely equal
        assert_eq!(pce1.compare(&pce2), PceCompareResult::Equal);

        // not equal, but same num_channels
        pce2.init();
        assert_eq!(
            pce1.compare(&pce2),
            PceCompareResult::DifferentButSameChannelConfig
        );

        // not equal and different number of channels
        pce2.num_channels = 2;
        assert_eq!(pce1.compare(&pce2), PceCompareResult::CompletelyDifferent);

        // not equal and same number of channels,
        // but different number of elements (Lfe case)
        pce1.num_channels = 2;
        pce2.num_lfe_channel_elements = 1;
        assert_eq!(
            pce1.compare(&pce2),
            PceCompareResult::DifferentChannelConfigSameNumberOfChannels
        );
    }

    #[test]
    fn get_default_mpeg_config() {
        let mut pce = ProgramConfig::new();

        // Invalid cases
        let mut channel_config = 30;
        assert!(!pce.get_default_mpeg_config(channel_config));

        channel_config = 10;
        assert!(!pce.get_default_mpeg_config(channel_config));

        // Valid case
        let channel_config = 2;
        assert!(pce.get_default_mpeg_config(channel_config));
        assert_eq!(pce.num_channel_elements[FRONT], 1);
        assert!(pce.element_is_cpe[FRONT][0]);
        assert_eq!(pce.num_channels, 2);
        assert_eq!(pce.num_effective_channels, 2);
        assert!(pce.is_valid);
    }

    #[test]
    fn get_default_config() {
        let mut pce = ProgramConfig::new();

        // Valid case of MPEG config 1-14
        let channel_config = 14;
        pce.get_default_config(channel_config);
        assert_eq!(pce.element_height_info[FRONT][2], HeightInfo::TopSpeaker);
        assert_eq!(pce.num_channels, 8);
        assert_eq!(pce.num_effective_channels, 7);

        // Valid case of helper config like 32, 30, 21, 16
        let channel_config = 32;
        pce.get_default_config(channel_config);
        assert_eq!(pce.num_channel_elements[FRONT], 2);
        assert!(!pce.element_is_cpe[FRONT][0]);
        assert!(pce.element_is_cpe[FRONT][1]);
        assert_eq!(pce.num_channel_elements[SIDE], 1);
        assert!(pce.element_is_cpe[SIDE][0]);
        assert_eq!(pce.num_channel_elements[BACK], 1);
        assert!(pce.element_is_cpe[BACK][0]);
        assert_eq!(pce.num_lfe_channel_elements, 1);
        assert_eq!(pce.num_channels, 8);
        assert_eq!(pce.num_effective_channels, 7);
        assert!(pce.is_valid);
    }

    #[test]
    fn get_element_list() {
        let mut pce = ProgramConfig::new();

        // Valid case
        let channel_config = 14;
        let result = pce.get_default_mpeg_config(channel_config);
        let mut element_list = [ChannelElementId::None; 8];
        let count = pce.get_element_list(&mut element_list[..]);
        assert!(result);
        assert_eq!(count, 5); // Sce, Cpe, Cpe, Lfe, Cpe
        assert!(
            [
                ChannelElementId::Sce,
                ChannelElementId::Cpe,
                ChannelElementId::Cpe,
                ChannelElementId::Lfe,
                ChannelElementId::Cpe,
                ChannelElementId::None,
                ChannelElementId::None,
                ChannelElementId::None
            ] == element_list[..]
        );

        // Invalid case where is no valid elements in PCE
        pce.init();
        let mut element_list = [ChannelElementId::None; 8];
        let count = pce.get_element_list(&mut element_list[..]);
        assert_eq!(count, 0); // all are None
        assert!([ChannelElementId::None; 8] == element_list[..]);
    }

    #[test]
    fn validate_non_audio_element_instance_tag() {
        let mut pce = ProgramConfig::new();

        // Invalid case
        let tag = 0xC;
        let element_type = ChannelElementId::Dse;
        assert!(!pce.validate_non_audio_element_instance_tag(tag, element_type));

        // Valid case - PCE without DSE
        let element_type = ChannelElementId::Dse;
        let channel_config = 2;
        let _result = pce.get_default_mpeg_config(channel_config);
        assert!(pce.validate_non_audio_element_instance_tag(tag, element_type));

        // PCE with DSE element
        pce.reset();
        pce.is_valid = true; // Assume PCE is valid
        pce.num_assoc_data_elements = 3;
        pce.assoc_data_element_tag_select[0..3].copy_from_slice(&[0xA, 0xD, 0xC]);
        assert!(pce.validate_non_audio_element_instance_tag(tag, element_type));

        // PCE with CCE element
        pce.reset();
        pce.is_valid = true; // Assume PCE is valid
        pce.num_valid_cc_elements = 3;
        let element_type = ChannelElementId::Cce;
        let tag = 0xB;
        pce.valid_cc_element_tag_select[0..3].copy_from_slice(&[0xA, 0xB, 0xC]);
        assert!(pce.validate_non_audio_element_instance_tag(tag, element_type));

        // Invalid Element type ( tags except DSE, CCE)
        let element_type = ChannelElementId::Cpe;
        assert!(!pce.validate_non_audio_element_instance_tag(tag, element_type));
    }

    #[test]
    fn get_element_associated_instance_tag() {
        let mut pce = ProgramConfig::new();
        let channel_config = 7;
        let result = pce.get_default_mpeg_config(channel_config);
        assert!(result);
        let mut element_list = [ChannelElementId::None; 8];
        let count = pce.get_element_list(&mut element_list[..]); // [Sce, Cpe, Cpe, None, None...]
        assert_eq!(count, 5);
        let element_type = ChannelElementId::Cpe;
        let element_index = 2;
        // Matching case: height_layer = 0 , element_type = ChannelElementId::Cpe
        let assoc_tag = pce.get_element_associated_instance_tag(element_index, element_type);
        assert_eq!(assoc_tag, 1);

        // Matching case: height_layer = 0 , element_type = ChannelElementId::Lfe
        let element_type = ChannelElementId::Lfe;
        let element_index = 4;
        let assoc_tag = pce.get_element_associated_instance_tag(element_index, element_type);
        assert_eq!(assoc_tag, 0);

        // Not matching case: height_layer = 0 , element_type = ChannelElementId::Sce
        let element_type = ChannelElementId::Sce;
        let element_index = 2;
        let assoc_tag = pce.get_element_associated_instance_tag(element_index, element_type);
        assert_eq!(assoc_tag, 255);
    }

    #[test]
    fn get_channel_map_index() {
        let mut pce = ProgramConfig::new();
        let channel_config = 14;
        let result = pce.get_default_mpeg_config(channel_config);
        assert!(result);
        assert_eq!(pce.num_channels, 8);
        let index = pce.get_channel_map_index();
        assert_eq!(index, 14);

        // With Helper channel config 30
        pce.reset();
        let channel_config = 30;
        pce.get_default_config(channel_config);
        assert_eq!(pce.num_channels, 3);
        let index = pce.get_channel_map_index();
        assert_eq!(index, 9);

        // With Helper channel config 21
        pce.reset();
        let channel_config = 21;
        pce.get_default_config(channel_config);
        assert_eq!(pce.num_channels, 4);
        let index = pce.get_channel_map_index();
        assert_eq!(index, 10);

        // Invalid channels
        pce.reset();
        assert_eq!(pce.num_channels, 0);
        pce.num_channels = 16; // set invalid value
        let index = pce.get_channel_map_index();
        assert_eq!(index, 0);
    }

    #[test]
    fn get_channel_description() {
        let mut pce = ProgramConfig::new();
        pce.init();
        let channel_config = 11;
        let result = pce.get_default_mpeg_config(channel_config);
        assert!(result);
        let mut channel_type = [AudioChannelType::None; MAX_CHANNELS];
        let mut channel_index = [0_u8; MAX_CHANNELS];

        pce.get_channel_description(&mut channel_type, &mut channel_index);

        assert!(pce.is_valid());
        assert!(
            [
                AudioChannelType::Front,
                AudioChannelType::Front,
                AudioChannelType::Front,
                AudioChannelType::Back,
                AudioChannelType::Back,
                AudioChannelType::Back,
                AudioChannelType::Lfe,
                AudioChannelType::None
            ] == channel_type[..]
        );
        assert_eq!([0, 1, 2, 0, 1, 2, 0, 0], channel_index);

        // Invalid case
        pce.reset();
        assert!(!pce.is_valid());
        pce.get_channel_description(&mut channel_type, &mut channel_index);
        assert!(
            [
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None,
                AudioChannelType::None
            ] == channel_type[..]
        );
        assert_eq!([0, 0, 0, 0, 0, 0, 0, 0], channel_index);
    }

    #[test]
    fn read() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(16, Mode::Writer);
        {
            // PCE data from bitstream
            // Byte blob meaning:
            // 0b0111      - element_instance_tag
            // 0b01        - profile
            // 0b0011      - sampling_frequency_index
            // 0b0010      - num_channel_elements (front)
            // 0b0010      - num_channel_elements (side)
            // 0b0010      - num_channel_elements (back)
            // 0b01        - num_lfe_channel_elements
            // 0b011       - num_assoc_data_elements
            // 0b0010      - num_valid_cc_elements
            bitstream_writer.write(0b0111010011001000100010010110010, 31);

            // Byte blob meaning:
            // 0b1         - is_mono_mixdown_present
            // 0b0011      - mono_mixdown_element_number
            // 0b1         - is_stereo_mixdown_present
            // 0b0011      - stereo_mixdown_element_number
            // 0b1         - is_matrix_mixdown_index_present
            // 0b00        - matrix_mixdown_index
            // 0b0         - is_pseudo_surround_enabled
            bitstream_writer.write(0b10011100111000, 14);
            // 0b10011 0b00011     - element_is_cpe + element_tag_select
            // 0b10101 0b00101     - element_is_cpe + element_tag_select
            // 0b10111 0b00111     - element_is_cpe + element_tag_select
            bitstream_writer.write(0b100110001110101001011011100111, 30);
            // 0b1111              - lfe_element_tag_select
            // 0b0001 0011 0111    - assoc_data_element_tag_select
            bitstream_writer.write(0b1111000100110111, 16);

            // 0b10011 0b00011     - cc_element_is_ind_sw + valid_cc_element_tag_select
            bitstream_writer.write(0b1001100011, 10);
            // 0b00000000          - comment_field_bytes
            // 0b01001111          - comment byte 1
            // 0b11110000          - comment byte 2
            bitstream_writer.write(0b000000000100111111110000, 24);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 125);

        let mut pce = ProgramConfig::new();
        pce.init();
        let alignment_anchor = bitstream_reader.valid_bits();
        pce.read(&mut bitstream_reader, alignment_anchor);

        assert_eq!(pce.element_instance_tag, 7);
        assert_eq!(pce.profile, 1);
        assert_eq!(pce.sampling_frequency_index, 3);
        assert_eq!(pce.num_channel_elements[FRONT], 2);
        assert_eq!(pce.num_channel_elements[SIDE], 2);
        assert_eq!(pce.num_channel_elements[BACK], 2);
        assert_eq!(pce.num_lfe_channel_elements, 1);
        assert_eq!(pce.num_assoc_data_elements, 3);
        assert_eq!(pce.num_valid_cc_elements, 2);
        assert!(pce.is_mono_mixdown_present);
        assert_eq!(pce.mono_mixdown_element_number, 3);
        assert!(pce.is_stereo_mixdown_present);
        assert_eq!(pce.stereo_mixdown_element_number, 3);
        assert!(pce.is_matrix_mixdown_index_present);
        assert_eq!(pce.matrix_mixdown_index, 0);
        assert!(!pce.is_pseudo_surround_enabled);
        assert_eq!(pce.element_is_cpe[0][..2], [true, false]);
        assert_eq!(pce.element_is_cpe[1][..2], [true, false]);
        assert_eq!(pce.element_is_cpe[2][..2], [true, false]);

        assert_eq!(pce.element_tag_select[0][..2], [3, 3]);
        assert_eq!(pce.element_tag_select[1][..2], [5, 5]);
        assert_eq!(pce.element_tag_select[2][..2], [7, 7]);

        assert_eq!(pce.lfe_element_tag_select[0], 15);
        assert_eq!(pce.assoc_data_element_tag_select[..3], [1, 3, 7]);
        assert_eq!(pce.cc_element_is_ind_sw[..2], [true, false]);
        assert_eq!(pce.valid_cc_element_tag_select[..2], [3, 3]);
        assert_eq!(pce.comment_field_bytes, 2);

        assert_eq!(pce.num_channels, 10);
        assert_eq!(pce.num_effective_channels, 9);
        assert!(!pce.is_valid);
    }
}
