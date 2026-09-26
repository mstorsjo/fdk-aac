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
//! HCR info for once audio channel

use itertools::{izip, Itertools};
use std::cmp::Ordering;

use super::hcr_constants as hcr_const;
use super::hcr_utils::{self, TEST_BIT_10};
use crate::aac_dec::{channel_info::IcsInfo, constants, utils};
use crate::common::{bitstream::Bitstream, bs_element_id::ChannelElementId};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub(super) struct HcrInfo {
    pub(super) num_lines_in_section: [u16; hcr_const::MAX_SFB_HCR],
    pub(super) code_book: [u8; hcr_const::MAX_SFB_HCR],
    pub(super) len_of_reordered_spectral_data: u16,
    pub(super) len_of_longest_codeword: u8,
    pub(super) number_section: usize,
}
impl Default for HcrInfo {
    fn default() -> Self {
        Self {
            num_lines_in_section: [0_u16; hcr_const::MAX_SFB_HCR],
            code_book: [0_u8; hcr_const::MAX_SFB_HCR],
            len_of_reordered_spectral_data: 0_u16,
            len_of_longest_codeword: 0_u8,
            number_section: 0_usize,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub(super) struct HcrSegmentInfo {
    pub(super) num_segment: u32,
    pub(super) segment_bit_field:
        [u32; (constants::MAX_FRAMESIZE / 2) / (hcr_const::NUMBER_OF_BIT_IN_WORD as usize) + 1],
    pub(super) codeword_bit_field:
        [u32; (constants::MAX_FRAMESIZE / 2) / (hcr_const::NUMBER_OF_BIT_IN_WORD as usize) + 1],
    pub(super) segment_offset: u32,
    pub(super) left_start_of_segment: [i32; constants::MAX_FRAMESIZE / 2],
    pub(super) right_start_of_segment: [i32; constants::MAX_FRAMESIZE / 2],
    pub(super) remaining_bits_in_segment: [i8; constants::MAX_FRAMESIZE / 2],
    pub(super) read_direction: u8,
    pub(super) num_word_for_bit_field: u8,
    pub(super) num_bit_valid_in_last_word: u16,
}
impl Default for HcrSegmentInfo {
    fn default() -> Self {
        Self {
            num_segment: 0_u32,
            segment_bit_field: [0_u32;
                (constants::MAX_FRAMESIZE / 2) / (hcr_const::NUMBER_OF_BIT_IN_WORD as usize) + 1],
            codeword_bit_field: [0_u32;
                (constants::MAX_FRAMESIZE / 2) / (hcr_const::NUMBER_OF_BIT_IN_WORD as usize) + 1],
            segment_offset: 0_u32,
            left_start_of_segment: [0_i32; constants::MAX_FRAMESIZE / 2],
            right_start_of_segment: [0_i32; constants::MAX_FRAMESIZE / 2],
            remaining_bits_in_segment: [0_i8; constants::MAX_FRAMESIZE / 2],
            read_direction: 0_u8,
            num_word_for_bit_field: 0_u8,
            num_bit_valid_in_last_word: 0_u16,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub(super) struct HcrSectionInfo {
    pub(super) num_codeword: u32,
    pub(super) num_sorted_section: u32,
    pub(super) num_codeword_in_section: [u16; hcr_const::MAX_SFB_HCR],
    pub(super) num_sorted_codeword_in_section: [u16; hcr_const::MAX_SFB_HCR],
    pub(super) num_extended_sorted_codeword_in_section:
        [u16; hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
    pub(super) num_extended_sorted_codeword_in_section_index: u32,
    pub(super) num_extended_sorted_sections_in_sets: [u16; hcr_const::MAX_HCR_SETS],
    pub(super) num_extended_sorted_sections_in_sets_index: u32,
    pub(super) reorder_offeset: [u16; hcr_const::MAX_SFB_HCR],
    pub(super) sorted_codebook: [u8; hcr_const::MAX_SFB_HCR],
    pub(super) extended_sorted_codebook: [u8; hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
    pub(super) extended_sorted_codebook_index: u32,
    pub(super) max_len_of_cb_in_ext_srt_sec: [u8; hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
    pub(super) max_len_of_cb_in_ext_srt_sec_index: u32,
    pub(super) codebook_switch: [u8; hcr_const::MAX_SFB_HCR],
}
impl Default for HcrSectionInfo {
    fn default() -> Self {
        Self {
            num_codeword: 0_u32,
            num_sorted_section: 0_u32,
            num_codeword_in_section: [0_u16; hcr_const::MAX_SFB_HCR],
            num_sorted_codeword_in_section: [0_u16; hcr_const::MAX_SFB_HCR],
            num_extended_sorted_codeword_in_section: [0_u16;
                hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
            num_extended_sorted_codeword_in_section_index: 0,
            num_extended_sorted_sections_in_sets: [0_u16; hcr_const::MAX_HCR_SETS],
            num_extended_sorted_sections_in_sets_index: 0,
            reorder_offeset: [0_u16; hcr_const::MAX_SFB_HCR],
            sorted_codebook: [0_u8; hcr_const::MAX_SFB_HCR],
            extended_sorted_codebook: [0_u8; hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
            extended_sorted_codebook_index: 0,
            max_len_of_cb_in_ext_srt_sec: [0_u8; hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS],
            max_len_of_cb_in_ext_srt_sec_index: 0,
            codebook_switch: [0_u8; hcr_const::MAX_SFB_HCR],
        }
    }
}

impl HcrInfo {
    /// Returns `HcrInfo` instance
    pub(super) fn new() -> Self {
        HcrInfo::default()
    }

    /// Check both HCR lengths
    ///
    /// # Parameters
    ///
    /// - `error_word`: To log error type, updated on return
    fn err_detector_in_hcr_lengths(&self, error_word: &mut u32) {
        if self.len_of_reordered_spectral_data < u16::from(self.len_of_longest_codeword) {
            *error_word |= hcr_const::HCR_SI_LENGTHS_FAILURE;
        }
    }

    /// Check if codebook and number of sections are within allowed range (short only)
    ///
    /// # Parameters
    ///
    /// - `cb`: Code book for huffman decode reordering
    /// - `num_line`: Number of lines
    /// - `frame_length`: Frame length of spectral coefficients
    /// - `error_word`: To log error type, updated on return
    fn err_detector_in_hcr_sideinfo_short(
        &self,
        cb: u8,
        num_line: u16,
        frame_length: usize,
        error_word: &mut u32,
    ) {
        if !(utils::CBTYPE_ZERO_HCB..hcr_const::MAX_CB_CHECK).contains(&cb)
            || cb == utils::CBTYPE_BOOKSCL
        {
            *error_word |= hcr_const::CB_OUT_OF_RANGE_SHORT_BLOCK;
        }
        if num_line > frame_length as u16 {
            *error_word |= hcr_const::LINE_IN_SECT_OUT_OF_RANGE_SHORT_BLOCK;
        }
    }

    /// For short blocks a sorting algorithm is applied to get the side info,
    /// in the order that HCR could assemble the quantized spectral coefficients as if it is a long
    /// block.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Code book for huffman decode reordering
    /// - `frame_length`: Frame length of spectral coefficients
    /// - `error_word`: To log error type, updated on return
    pub(super) fn resort_side_info(
        &mut self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        frame_length: usize,
        error_word: &mut u32,
    ) {
        // short block
        if !ics_info.is_long_block() {
            let band_offsets = ics_info.scale_factor_bands();
            let mut num_line = 0;
            let mut cb = code_book[0];
            let mut cb_prev = code_book[0];
            let mut num_section = 0;

            // convert HCR-sideinfo into a unitwise manner: When the cb changes, a new section
            // starts
            self.code_book[0] = code_book[0];

            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
            let num_window_groups = ics_info.n_window_groups();
            for (band, (curr_band, next_band)) in band_offsets
                .iter()
                .tuple_windows()
                .take(ics_info.max_sf_bands())
                .enumerate()
            {
                let num_unit_in_band = (next_band - curr_band) >> hcr_const::FOUR_LOG_DIV_TWO_LOG;
                for _cu in 0..num_unit_in_band {
                    for group in 0..num_window_groups {
                        for _win_group_len in 0..ics_info.window_group_length(group) {
                            cb = code_book[group * bands_per_window + band];
                            if cb != cb_prev {
                                self.err_detector_in_hcr_sideinfo_short(
                                    cb,
                                    num_line,
                                    frame_length,
                                    error_word,
                                );
                                if *error_word != 0 {
                                    return;
                                }
                                self.num_lines_in_section[num_section] = num_line;
                                num_section += 1;
                                self.code_book[num_section] = cb;

                                cb_prev = cb;
                                num_line = hcr_const::LINES_PER_UNIT as u16;
                            } else {
                                num_line += hcr_const::LINES_PER_UNIT as u16;
                            }
                        }
                    }
                }
            }
            num_section += 1;
            self.err_detector_in_hcr_sideinfo_short(cb, num_line, frame_length, error_word);
            if num_section == 0 || num_section > hcr_const::MAX_SFB_HCR {
                *error_word |= hcr_const::NUM_SECT_OUT_OF_RANGE_SHORT_BLOCK;
            }

            self.err_detector_in_hcr_lengths(error_word);
            if *error_word != 0 {
                return;
            }
            self.num_lines_in_section[num_section - 1] = num_line;
            self.code_book[num_section] = cb;
            self.number_section = num_section;
        } else {
            // long block
            if self.number_section == 0 || self.number_section > constants::MAX_SFB_LONG {
                *error_word |= hcr_const::NUM_SECT_OUT_OF_RANGE_LONG_BLOCK;
                self.number_section = 0;
            }
            self.err_detector_in_hcr_lengths(error_word);

            for section in 0..self.number_section {
                let cb = self.code_book[section];

                if !(utils::CBTYPE_ZERO_HCB..hcr_const::MAX_CB_CHECK).contains(&cb)
                    || cb == utils::CBTYPE_BOOKSCL
                {
                    *error_word |= hcr_const::CB_OUT_OF_RANGE_LONG_BLOCK;
                }

                let num_line = self.num_lines_in_section[section];

                if (num_line == 0) || (num_line > frame_length as u16) {
                    *error_word |= hcr_const::LINE_IN_SECT_OUT_OF_RANGE_LONG_BLOCK;
                }
            }
            if *error_word != 0 {
                return;
            }
        }

        for section in 0..self.number_section {
            if (self.code_book[section] == utils::CBTYPE_NOISE_HCB)
                || (self.code_book[section] == utils::CBTYPE_INTENSITY_HCB2)
                || (self.code_book[section] == utils::CBTYPE_INTENSITY_HCB)
            {
                self.code_book[section] = 0;
            }
        }
        // HCR sideinfo input is complete and seems to be valid.
    }

    /// Decode (and adapt if necessary) the two HCR sideinfo components
    /// such as `reordered_spectral_data_length` and `longest_codeword_length`
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `global_hcr_type`: MP4 Channel element type
    pub(super) fn read(&mut self, bs: &mut Bitstream, global_hcr_type: ChannelElementId) {
        self.len_of_reordered_spectral_data = 0;
        self.len_of_longest_codeword = 0;

        // ------- SI-Value No 1 -------
        let length_ro_spec_data = bs.read(14) as u16 + hcr_const::ERROR_LORSD;

        if global_hcr_type == ChannelElementId::Cpe {
            if (0..=hcr_const::CPE_TOP_LENGTH).contains(&length_ro_spec_data) {
                // the decoded value is within range
                self.len_of_reordered_spectral_data = length_ro_spec_data;
            } else if length_ro_spec_data > hcr_const::CPE_TOP_LENGTH {
                // use valid maximum
                self.len_of_reordered_spectral_data = hcr_const::CPE_TOP_LENGTH;
            }
        } else if global_hcr_type == ChannelElementId::Sce
            || global_hcr_type == ChannelElementId::Lfe
            || global_hcr_type == ChannelElementId::Cce
        {
            if (0..=hcr_const::SCE_TOP_LENGTH).contains(&length_ro_spec_data) {
                // the decoded value is within range
                self.len_of_reordered_spectral_data = length_ro_spec_data;
            } else if length_ro_spec_data > hcr_const::SCE_TOP_LENGTH {
                self.len_of_reordered_spectral_data = hcr_const::SCE_TOP_LENGTH;
                // use valid maximum
            }
        }

        // ------- SI-Value No 2 -------
        let length_of_longest_codeword = bs.read(6) as u8 + hcr_const::ERROR_LOLC as u8;
        if (0..=hcr_const::LEN_OF_LONGEST_CW_TOP_LENGTH).contains(&length_of_longest_codeword) {
            // the decoded value is within range
            self.len_of_longest_codeword = length_of_longest_codeword;
        } else if length_of_longest_codeword > hcr_const::LEN_OF_LONGEST_CW_TOP_LENGTH {
            // use valid maximum
            self.len_of_longest_codeword = hcr_const::LEN_OF_LONGEST_CW_TOP_LENGTH;
        }
    }
}

impl HcrSegmentInfo {
    /// This function prepares the bitfield used for the segments.
    /// The list is set up once, to be used in all following sets.
    /// If a segment is decoded empty, the according bit from the bitfield is removed.
    ///
    /// # Return
    /// num_valid_segment: the number of valid segments
    pub(super) fn init_segment_bitfield(&mut self) -> u32 {
        let mut num_valid_segment: u32 = 0;
        let mut temp_word: u32;
        let mut r: usize;

        self.num_word_for_bit_field = u8::try_from(
            (self.num_segment.saturating_sub(1) >> hcr_const::THIRTYTWO_LOG_DIV_TWO_LOG) + 1,
        )
        .unwrap();
        self.num_bit_valid_in_last_word = u16::try_from(self.num_segment).unwrap();
        let last_index = usize::from(self.num_word_for_bit_field.saturating_sub(1));

        // loop over all words, which are completely used or only partially.
        // bit in pSegmentBitfield is zero if segment is empty.
        // bit in pSegmentBitfield is one if segment is not empty.
        for (index, bitfieldword) in self
            .segment_bit_field
            .iter_mut()
            .enumerate()
            .take(last_index)
        {
            temp_word = 0xFFFFFFFF; // set ones
            r = index << hcr_const::THIRTYTWO_LOG_DIV_TWO_LOG;
            for i in 0..hcr_const::NUMBER_OF_BIT_IN_WORD {
                if self.remaining_bits_in_segment[r + i as usize] == 0 {
                    // set a zero at bit number (NUMBER_OF_BIT_IN_WORD-1-i) in tempWord.
                    temp_word &= !(1 << (hcr_const::NUMBER_OF_BIT_IN_WORD - 1 - i));
                } else {
                    // count segments which are not empty.
                    num_valid_segment += 1;
                }
            }
            *bitfieldword = temp_word; // store result

            // calculate number of zeros on LSB side in the last word
            self.num_bit_valid_in_last_word -= hcr_const::NUMBER_OF_BIT_IN_WORD as u16;
        }

        // calculate last word: prepare special temp_word
        temp_word = 0xFFFFFFFF; // set ones
        for i in 0..(hcr_const::NUMBER_OF_BIT_IN_WORD - u32::from(self.num_bit_valid_in_last_word))
        {
            // clear bit i in temp_word
            temp_word &= !(1 << i);
        }

        // calculate last word
        r = last_index << hcr_const::THIRTYTWO_LOG_DIV_TWO_LOG;
        for i in 0..u32::from(self.num_bit_valid_in_last_word) {
            if self.remaining_bits_in_segment[r + i as usize] == 0 {
                // set a zero at bit number (NUMBER_OF_BIT_IN_WORD-1-i) in tempWord.
                temp_word &= !(1 << (hcr_const::NUMBER_OF_BIT_IN_WORD - 1 - i));
            } else {
                // count segments which are not empty.
                num_valid_segment += 1;
            }
        }
        self.segment_bit_field[last_index] = temp_word; // store result

        num_valid_segment
    }

    /// This function checks if all segments are empty after
    /// decoding. No lines are marked_ as invalid because it cannot be traced
    /// back where from the remaining bits are from.
    ///
    /// # Parameters
    /// - `error_word`: Logged error type, updated on return.
    pub(super) fn err_detect_within_segmentation_final(&self, error_word: &mut u32) {
        for data in self
            .remaining_bits_in_segment
            .iter()
            .take(self.num_segment as usize)
        {
            if *data != 0 {
                *error_word |= hcr_const::BIT_IN_SEGMENTATION_ERROR;
            }
        }
    }

    /// This function calculates the segmentation, which includes num_segment,
    /// left_start_of_segment, right_start_of_segment and remaining_bits_in_segment.
    /// The segmentation could be visualized as a kind of 'overlay-grid' for
    /// the bitstream-block holding the HCR-encoded quantized-spectral-coefficients.
    ///
    /// # Parameters
    /// - `section_info`: Section info for HCR data
    /// - `length_of_longest_codeword`: length of longest codewod for current element channel
    /// - `length_of_reordered_spectral_data`: length of reordered spectral data for current element
    ///   channel
    pub(super) fn prepare_segmentation_grid(
        &mut self,
        section_info: &HcrSectionInfo,
        length_of_longest_codeword: u8,
        length_of_reordered_spectral_data: u16,
    ) {
        let mut num_segment = 0;
        let mut segment_start = 0;
        let mut end_flag = false;
        let len_ro_spec_data = i32::from(length_of_reordered_spectral_data);

        for (sorted_cb, num_scw_in_sec) in section_info
            .sorted_codebook
            .iter()
            .zip(section_info.num_sorted_codeword_in_section.iter())
            .take(section_info.num_sorted_section as usize)
        {
            let segment_width = i32::from(
                hcr_const::MAX_CW_LEN[usize::from(*sorted_cb)].min(length_of_longest_codeword),
            );

            for _n in 0..*num_scw_in_sec {
                // width allows a new segment
                if (segment_start + segment_width) <= len_ro_spec_data {
                    // store segment start, segment length and increment the number of segments
                    self.left_start_of_segment[num_segment] = segment_start;
                    self.right_start_of_segment[num_segment] =
                        segment_start + segment_width - 1_i32;
                    self.remaining_bits_in_segment[num_segment] =
                        i8::try_from(segment_width).unwrap();
                    segment_start += segment_width;
                    num_segment += 1;
                }
                //   width does not allow a new segment
                else {
                    // correct the last segment length
                    self.remaining_bits_in_segment[num_segment - 1] = i8::try_from(
                        len_ro_spec_data - self.left_start_of_segment[num_segment - 1],
                    )
                    .unwrap();
                    self.right_start_of_segment[num_segment - 1] = len_ro_spec_data - 1;

                    end_flag = true;
                    break;
                }
            }
            if end_flag {
                break;
            }
        }
        self.num_segment = num_segment as u32;
    }
}

impl HcrSectionInfo {
    /// This function reorders the quantized spectral coefficients section wise for long and
    /// short-blocks and compares to the LAV (Largest Absolute Value of the current codebook)
    /// a counter is incremented if there is an error detected.
    /// Additional for short-blocks a unit-based-deinterleaving is applied.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `num_section`: Number of sections
    /// - `spectral_coefficient`: Spectral coefficients
    /// - `error_word`: To log error type, updated on return
    pub(super) fn reorder_quantized_spectral_coefficients(
        &mut self,
        ics_info: &IcsInfo,
        num_section: usize,
        spectral_coefficient: &mut [i16],
        error_word: &mut u32,
    ) {
        let frame_length = spectral_coefficient.len();
        let mut lav_error_cnt = 0;
        let mut temp_values = [0_i16; constants::MAX_BINS_LONG];

        // long and short: check if decoded huffman-values (quantized spectral
        // coefficients) are within range
        let mut num_qsc = 0;
        for (scb, num_scbw_in_section, ro_offset) in izip!(
            self.sorted_codebook.iter(),
            self.num_sorted_codeword_in_section.iter(),
            self.reorder_offeset.iter()
        )
        .take(num_section)
        {
            let num_spectral_values_in_section =
                usize::from(num_scbw_in_section << hcr_const::DIM_CB_SHIFT[usize::from(*scb)]);
            for tmp_val in temp_values[usize::from(*ro_offset)..]
                .iter_mut()
                .take(num_spectral_values_in_section)
            {
                let qsc = spectral_coefficient[num_qsc];
                num_qsc += 1;
                let abs_qsc = qsc.unsigned_abs();
                if abs_qsc <= hcr_const::LARGEST_ABSOLUTE_VALUE[usize::from(*scb)] {
                    // the qsc value is within range
                    *tmp_val = qsc;
                } else {
                    // line is too high ..
                    if abs_qsc == hcr_const::Q_VALUE_INVALID as u16 {
                        // .. because of previous marking --> dont set LAV flag (would be
                        // confusing), just copy out the already marked
                        // value
                        *tmp_val = qsc;
                    } else {
                        // .. because a too high value was decoded for this cb --> set LAV flag
                        *tmp_val = hcr_const::Q_VALUE_INVALID;
                        lav_error_cnt += 1;
                    }
                }
            }
        }

        // short block
        if !ics_info.is_long_block() {
            // deinterleave unitwise for short blocks
            let bands_per_window = frame_length / ics_info.windows_per_frame();
            for (window, spec_coeff) in spectral_coefficient
                .chunks_exact_mut(bands_per_window)
                .take(constants::MAX_WINDOWS)
                .enumerate()
            {
                let num_unit_in_frame =
                    frame_length / (hcr_const::LINES_PER_UNIT * constants::MAX_WINDOWS);
                let temp_values_offset = window << hcr_const::FOUR_LOG_DIV_TWO_LOG;
                for (unit, spec_out) in spec_coeff
                    .chunks_exact_mut(hcr_const::LINES_PER_UNIT)
                    .take(num_unit_in_frame)
                    .enumerate()
                {
                    // distance of lines between unit groups has to be constant for every
                    // framelength (32)!
                    spec_out.copy_from_slice(
                        &temp_values[temp_values_offset + unit * 32
                            ..temp_values_offset + unit * 32 + hcr_const::LINES_PER_UNIT],
                    );
                }
            }
        } else {
            // Long block
            // copy straight for long-blocks
            spectral_coefficient.copy_from_slice(&temp_values[..frame_length]);
        }

        if lav_error_cnt != 0 {
            *error_word |= hcr_const::LAV_VIOLATION;
        }
    }

    /// This function calculates the number of extended sorted sections which belong to the sets.
    /// Each set from set 0 (one and only set for the PCWs) to the last set gets a entry
    /// in the array which `num_extended_sorted_codeword_in_section` points to.
    ///
    /// Calculation: The entries in `num_extended_sorted_codeword_in_section` are added until
    /// the value num_segment is reached. Then the sum_variable is cleared and the calculation
    /// starts from the beginning. As much extended sorted sections are summed up to reach the value
    /// `num_segment`, as much is the current entry in `num_extended_sorted_codeword_in_section`.
    ///
    /// # Parameters
    ///
    /// - `num_segment`: Number of segments
    ///
    /// # Return
    ///  0 : Success, 1 : Error
    pub(super) fn derive_number_of_extended_sorted_sections_in_sets(
        &mut self,
        num_segment: u32,
    ) -> u32 {
        let mut counter: u16 = 0;
        let mut cw_sum = 0;
        let mut num_extended_sorted_codeword_in_section_idx = 0;
        let mut num_extended_sorted_sections_in_sets_idx = 0;

        while self.num_extended_sorted_codeword_in_section
            [num_extended_sorted_codeword_in_section_idx]
            != 0
        {
            cw_sum += u32::from(
                self.num_extended_sorted_codeword_in_section
                    [num_extended_sorted_codeword_in_section_idx],
            );
            num_extended_sorted_codeword_in_section_idx += 1;
            if num_extended_sorted_codeword_in_section_idx
                >= (hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS)
            {
                return 1;
            }
            if cw_sum > num_segment {
                return 1;
            }
            counter += 1;
            if usize::from(counter) > hcr_const::MAX_SFB_HCR {
                return 1;
            }
            if cw_sum == num_segment {
                self.num_extended_sorted_sections_in_sets
                    [num_extended_sorted_sections_in_sets_idx] = counter;
                num_extended_sorted_sections_in_sets_idx += 1;
                if num_extended_sorted_sections_in_sets_idx >= hcr_const::MAX_HCR_SETS {
                    return 1;
                }
                counter = 0;
                cw_sum = 0;
            }
        }
        self.num_extended_sorted_sections_in_sets[num_extended_sorted_sections_in_sets_idx] =
            counter; // save last entry for the last - probably shorter - set

        0 // return
    }

    /// This function calculates the number of codewords for each section
    /// (`num_codeword_in_section`) and the number of codewords for all sections
    /// (`num_codeword`). For zero and intensity codebooks a entry is also done in the variable
    /// `num_codeword_in_section`. It is assumed that the codebook is a two tuples codebook.
    /// This is needed later for the calculation of the base addresses for the reordering of the
    /// quantize spectral coefficients at the end of the hcr tool. The variable `num_codeword`
    /// contain the number of codewords which are really in the bitstream. Zero or intensity
    /// codebooks does not increase the variable numCodewords
    ///
    /// # Parameters
    ///
    /// - `code_book`: Code book for huffman decode reorderings
    /// - `num_line_in_section`: Number of lines in section for current channel HcrInfo
    pub(super) fn calc_num_codeword(&mut self, code_book: &[u8], num_line_in_section: &[u16]) {
        let mut num_codeword: u32 = 0;

        for (num_cw_in_sec, num_line_in_sec, cb) in izip!(
            self.num_codeword_in_section.iter_mut(),
            num_line_in_section.iter(),
            code_book.iter()
        ) {
            *num_cw_in_sec = *num_line_in_sec >> hcr_const::DIM_CB_SHIFT[usize::from(*cb)];
            if *cb != 0 {
                num_codeword += u32::from(*num_cw_in_sec);
            }
        }
        self.num_codeword = num_codeword;
    }

    /// This function adapts the sorted section boundaries to the boundaries of segmentation.
    /// If the section lengths do not fit completely into the current segment, the section
    /// is split into two so-called 'extended sections'. The extended section info
    /// (`num_extended_sorted_codeword_in_section` and `extended_sorted_codebook`) is updated in
    /// this case.
    ///
    /// # Parameters
    ///
    /// - `length_of_longest_codeword`: Length of longest codeword
    /// - `num_segment`: Number of segments
    /// - `error_word`: Logged error type, updated on return.
    pub(super) fn extended_section_info(
        &mut self,
        length_of_longest_codeword: u8,
        num_segment: u16,
        error_word: &mut u32,
    ) {
        // counter for sorted sections
        let mut srt_sec_cnt = 0;
        // counter for extended sorted sections
        let mut x_srt_sc_cnt = 0;
        let mut remain_num_cw_in_sort_sec = self.num_sorted_codeword_in_section[srt_sec_cnt];
        let mut in_segment_remain_num_cw = num_segment;

        while srt_sec_cnt < self.num_sorted_section as usize {
            self.extended_sorted_codebook[x_srt_sc_cnt] = self.sorted_codebook[srt_sec_cnt];

            match in_segment_remain_num_cw.cmp(&remain_num_cw_in_sort_sec) {
                Ordering::Less => {
                    self.num_extended_sorted_codeword_in_section[x_srt_sc_cnt] =
                        in_segment_remain_num_cw;
                    remain_num_cw_in_sort_sec -= in_segment_remain_num_cw;
                    // data of a sorted section was not integrated in extended sorted section
                    in_segment_remain_num_cw = num_segment;
                }
                Ordering::Equal => {
                    self.num_extended_sorted_codeword_in_section[x_srt_sc_cnt] =
                        in_segment_remain_num_cw;
                    srt_sec_cnt += 1;
                    remain_num_cw_in_sort_sec = self.num_sorted_codeword_in_section[srt_sec_cnt];
                    // data of a sorted section was integrated in extended sorted section
                    in_segment_remain_num_cw = num_segment;
                }
                Ordering::Greater => {
                    // inSegmentRemainNumCW > remainNumCwInSortSec
                    self.num_extended_sorted_codeword_in_section[x_srt_sc_cnt] =
                        remain_num_cw_in_sort_sec;
                    in_segment_remain_num_cw -= remain_num_cw_in_sort_sec;
                    srt_sec_cnt += 1;
                    // data of a sorted section was integrated in extended sorted section
                    remain_num_cw_in_sort_sec = self.num_sorted_codeword_in_section[srt_sec_cnt];
                }
            }
            self.max_len_of_cb_in_ext_srt_sec[x_srt_sc_cnt] = hcr_const::MAX_CW_LEN
                [usize::from(self.extended_sorted_codebook[x_srt_sc_cnt])]
            .min(length_of_longest_codeword);

            x_srt_sc_cnt += 1;

            if x_srt_sc_cnt >= (hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS) {
                *error_word |= hcr_const::EXTENDED_SORTED_COUNTER_OVERFLOW;
                return;
            }
        }
        self.num_extended_sorted_codeword_in_section[x_srt_sc_cnt] = 0;
    }

    /// Reset length and index information
    pub(super) fn reset_length_and_index_info(&mut self) {
        self.num_extended_sorted_codeword_in_section_index = 0;
        self.extended_sorted_codebook_index = 0;
        self.num_extended_sorted_sections_in_sets_index = 0;
        self.max_len_of_cb_in_ext_srt_sec_index = 0;
    }

    /// This function calculates the number of sorted codebooks and sorts the codebooks and the
    /// `num_codeword_in_section` according to the priority.
    ///
    /// # Parameters
    ///
    /// - `code_book`: Code book for huffman decode reorderings
    /// - `num_section`: Number of sections
    pub(super) fn sort_codebook_and_num_codeword_in_section(
        &mut self,
        code_book: &[u8],
        num_section: usize,
    ) {
        // calculate num_sorted_section and store the priorities in array
        let mut num_zero_section = 0;
        for (cb, sorted_cb) in code_book
            .iter()
            .zip(self.sorted_codebook.iter_mut())
            .take(num_section)
        {
            if hcr_const::CB_PRIORITY[usize::from(*cb)] == 0 {
                num_zero_section += 1;
            }
            *sorted_cb = hcr_const::CB_PRIORITY[usize::from(*cb)];
        }

        // num_sorted_section contains no zero or intensity section
        self.num_sorted_section = (num_section - num_zero_section) as u32;

        // sort priorities of the codebooks in array self.sorted_codebook[]
        self.sorted_codebook[0..num_section].sort_by(|a, b| b.cmp(a)); // (reverse sort)

        //   clear self.codebook_switch array
        self.codebook_switch[..num_section].fill(0);

        // sort section_codebooks and num_codwords_in_section and calculate
        // self.reorder_offeset[j]
        let mut search_start = 0;
        for (sorted_cb, num_scw_in_sec, ro_offset) in izip!(
            self.sorted_codebook.iter_mut(),
            self.num_sorted_codeword_in_section.iter_mut(),
            self.reorder_offeset.iter_mut()
        )
        .take(num_section)
        {
            if search_start < num_section {
                let cnt_start = search_start;
                'inner: for i in cnt_start..num_section {
                    if self.codebook_switch[i] == 0
                        && (hcr_const::MIN_OF_CB_PAIR[usize::from(*sorted_cb)] == code_book[i]
                            || hcr_const::MAX_OF_CB_PAIR[usize::from(*sorted_cb)] == code_book[i])
                    {
                        self.codebook_switch[i] = 1;
                        *sorted_cb = code_book[i];
                        *num_scw_in_sec = self.num_codeword_in_section[i];

                        let mut start_offset = 0;
                        for (k, cb) in code_book.iter().enumerate().take(i) {
                            start_offset += self.num_codeword_in_section[k]
                                << hcr_const::DIM_CB_SHIFT[usize::from(*cb)];
                        }
                        // offset for reordering the codewords
                        *ro_offset = start_offset;

                        if i == search_start {
                            let mut k = i;
                            while self.codebook_switch[k] == 1 {
                                search_start += 1;
                                k += 1;
                            }
                        }
                        break 'inner;
                    }
                }
            }
        }
    }

    /// # Description
    /// This function decodes all priority codewords (PCWs) in a spectrum (within set 0).
    /// The calculation of the PCWs is managed in two loops.
    /// The loop counter of the outer loop is set to the first value pointer
    /// num_extended_sorted_sections_in_sets points to. This value represents the number
    /// of extended sorted sections within set 0. The loop counter of the inner loop is set to
    /// the first value pointer num_extended_sorted_codeword_in_section points to. The value
    /// represents the number of extended sorted codewords in sections (the original
    /// sections have been splitted to go along with the borders of the sets). Each time
    /// the number of the extended sorted codewords in sections are de- coded, the
    /// pointer 'num_extended_sorted_codeword_in_section' is incremented by one.
    ///
    /// # Parameters
    ///
    /// - `segment_info`: HCR segment info
    /// - `bs`: Bitstream instance with valid internal data
    /// - `bs_anchor`: Bitstream anchor value at start of HCR decoding
    /// - `spectral_coefficient`: spectral coeffiecients
    /// - `quantized_spectral_coefficients_idx`: Index value for spectral coeffiecients
    /// - `error_word`: To log error type, updated on return
    ///
    /// # Return
    ///  0 : Success, 1 : Error
    pub(super) fn decode_pcws(
        &mut self,
        segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        bs_anchor: isize,
        spectral_coefficient: &mut [i16],
        quantized_spectral_coefficients_idx: &mut usize,
        error_word: &mut u32,
    ) -> u32 {
        let frame_length = spectral_coefficient.len();
        let num_sections = u32::from(
            self.num_extended_sorted_sections_in_sets
                [self.num_extended_sorted_sections_in_sets_index as usize],
        );

        let max_hcr_sfb_plus_sets = (hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS) as u32;
        if (self.num_extended_sorted_codeword_in_section_index + num_sections)
            > max_hcr_sfb_plus_sets
            || (self.extended_sorted_codebook_index + num_sections) > max_hcr_sfb_plus_sets
            || (self.max_len_of_cb_in_ext_srt_sec_index + num_sections) > max_hcr_sfb_plus_sets
            || (self.num_extended_sorted_sections_in_sets_index + 1)
                > hcr_const::MAX_HCR_SETS as u32
        {
            return 1;
        }

        // Get slices of buffer based on corresponding buffer index values
        let max_len_of_cb_in_ext_srt_sec =
            &self.max_len_of_cb_in_ext_srt_sec[self.max_len_of_cb_in_ext_srt_sec_index as usize..];
        let extended_sorted_codebook =
            &self.extended_sorted_codebook[self.extended_sorted_codebook_index as usize..];
        let num_extended_sorted_codeword_in_section = &self.num_extended_sorted_codeword_in_section
            [self.num_extended_sorted_codeword_in_section_index as usize..];

        let mut quantized_spec_coeff_idx = *quantized_spectral_coefficients_idx;
        let mut cw_offset = 0;

        // decode all PCWs in the extended sorted section(s) belonging to set 0
        for i in 0..num_sections as usize {
            let codebook = usize::from(extended_sorted_codebook[i]);
            let dimension = usize::from(hcr_const::DIM_CB[codebook]);
            let current_tree = hcr_const::HUFF_TABLE[codebook];
            let quant_val_base = hcr_const::QUANT_TABLE[codebook];
            let num_codewords = usize::from(num_extended_sorted_codeword_in_section[i]);
            let left_start_of_segment = &mut segment_info.left_start_of_segment[cw_offset..];
            let remaining_bits_in_segment =
                &mut segment_info.remaining_bits_in_segment[cw_offset..];
            let max_allowed_cw_len = max_len_of_cb_in_ext_srt_sec[i];

            // switch for decoding with different codebooks
            if hcr_const::SIGN_CB[codebook] == 0 {
                // no sign bits follow after the codeword-body
                // PCW_BodyONLY
                for (left_sos, remaining_bits) in left_start_of_segment
                    .iter_mut()
                    .zip(remaining_bits_in_segment.iter_mut())
                    .take(num_codewords)
                {
                    let push_bits = bs.valid_bits() - bs_anchor + *left_sos as isize;
                    bs.push(push_bits);

                    let valid_bits = bs.valid_bits();

                    // decode PCW_BODY
                    let branch_value = Self::decode_pcw_body(bs, current_tree);

                    if (quantized_spec_coeff_idx + dimension) >= frame_length {
                        return 1;
                    }

                    // result is written out here because NO sign bits follow the body
                    for (spec_coeff, q_val_base) in spectral_coefficient[quantized_spec_coeff_idx..]
                        .iter_mut()
                        .zip(quant_val_base[branch_value..].iter())
                        .take(dimension)
                    {
                        // write quant spec coeff into spectrum; sign is already valid
                        *spec_coeff = i16::from(*q_val_base);
                    }

                    let num_decoded_bits = i32::try_from(valid_bits - bs.valid_bits()).unwrap();
                    *left_sos += num_decoded_bits;
                    *remaining_bits -= i8::try_from(num_decoded_bits).unwrap();

                    // one more PCW should be decoded
                    if max_allowed_cw_len
                        < (u8::try_from(num_decoded_bits).unwrap()
                            + hcr_const::ERROR_PCW_BODY_ONLY_TOO_LONG)
                    {
                        *error_word |= hcr_const::TOO_MANY_PCW_BODY_BITS_DECODED;
                    }

                    if Self::err_detect_pcw_segmentation(
                        *remaining_bits - hcr_const::ERROR_PCW_BODY as i8,
                        hcr_const::PcwType::Body,
                        &mut spectral_coefficient
                            [quantized_spec_coeff_idx..quantized_spec_coeff_idx + dimension],
                        error_word,
                    ) {
                        return 1;
                    }
                    quantized_spec_coeff_idx += dimension;
                }
            } else if hcr_const::SIGN_CB[codebook] == 1 && codebook < 11 {
                // possibly there follow 1,2,3 or 4 sign bits after the codeword-body
                // PCW_Body and PCW_Sign

                for (left_sos, remaining_bits) in left_start_of_segment
                    .iter_mut()
                    .zip(remaining_bits_in_segment.iter_mut())
                    .take(num_codewords)
                {
                    let mut is_error = false;
                    let push_bits = bs.valid_bits() - bs_anchor + *left_sos as isize;
                    bs.push(push_bits);

                    let valid_bits = bs.valid_bits();

                    // decode PCW_BODY
                    let branch_value = Self::decode_pcw_body(bs, current_tree);

                    if (quantized_spec_coeff_idx + dimension) >= frame_length {
                        is_error = true;
                    }

                    // Decode PCW sign
                    Self::decode_pcw_sign(
                        bs,
                        &quant_val_base[branch_value..],
                        &mut spectral_coefficient
                            [quantized_spec_coeff_idx..quantized_spec_coeff_idx + dimension],
                    );

                    let num_decoded_bits = i32::try_from(valid_bits - bs.valid_bits()).unwrap();
                    if num_decoded_bits > i32::from(*remaining_bits) {
                        is_error = true;
                    }

                    if is_error {
                        return 1;
                    }

                    *left_sos += num_decoded_bits;
                    *remaining_bits -= i8::try_from(num_decoded_bits).unwrap();

                    // one more PCW should be decoded
                    if max_allowed_cw_len
                        < (u8::try_from(num_decoded_bits).unwrap()
                            + hcr_const::ERROR_PCW_BODY_SIGN_TOO_LONG)
                    {
                        *error_word |= hcr_const::TOO_MANY_PCW_BODY_SIGN_BITS_DECODED;
                    }

                    if Self::err_detect_pcw_segmentation(
                        *remaining_bits - hcr_const::ERROR_PCW_BODY_SIGN as i8,
                        hcr_const::PcwType::BodySign,
                        &mut spectral_coefficient
                            [quantized_spec_coeff_idx..quantized_spec_coeff_idx + dimension],
                        error_word,
                    ) {
                        return 1;
                    }
                    quantized_spec_coeff_idx += dimension;
                }
            } else if hcr_const::SIGN_CB[codebook] == 1 && codebook >= 11 {
                // possibly there follow some sign bits and maybe one or two
                // escape sequences after the cw-body
                // PCW_Body, PCW_Sign and maybe PCW_Escape

                for (left_sos, remaining_bits) in left_start_of_segment
                    .iter_mut()
                    .zip(remaining_bits_in_segment.iter_mut())
                    .take(num_codewords)
                {
                    let mut is_error = false;
                    let push_bits = bs.valid_bits() - bs_anchor + *left_sos as isize;
                    bs.push(push_bits);

                    let valid_bits = bs.valid_bits();

                    // decode PCW_BODY
                    let branch_value = Self::decode_pcw_body(bs, current_tree);

                    if (quantized_spec_coeff_idx + dimension) >= frame_length {
                        is_error = true;
                    }

                    // Decode PCW sign
                    Self::decode_pcw_sign(
                        bs,
                        &quant_val_base[branch_value..],
                        &mut spectral_coefficient
                            [quantized_spec_coeff_idx..quantized_spec_coeff_idx + dimension],
                    );

                    let mut num_decoded_bits = i32::try_from(valid_bits - bs.valid_bits()).unwrap();
                    if num_decoded_bits > i32::from(*remaining_bits) {
                        is_error = true;
                    }

                    if is_error {
                        return 1;
                    }

                    // decode PCW_ESCAPE if present
                    let quant_spec_pcw_escape_idx = quantized_spec_coeff_idx + dimension
                        - hcr_const::DIMENSION_OF_ESCAPE_CODEBOOK;
                    if spectral_coefficient[quant_spec_pcw_escape_idx].unsigned_abs()
                        == hcr_const::ESCAPE_VALUE
                    {
                        spectral_coefficient[quant_spec_pcw_escape_idx] =
                            Self::decode_escape_sequence(
                                bs,
                                spectral_coefficient[quant_spec_pcw_escape_idx],
                                *remaining_bits - i8::try_from(num_decoded_bits).unwrap(),
                                error_word,
                            );
                        num_decoded_bits = i32::try_from(valid_bits - bs.valid_bits()).unwrap();
                    }

                    if spectral_coefficient[quant_spec_pcw_escape_idx + 1].unsigned_abs()
                        == hcr_const::ESCAPE_VALUE
                    {
                        spectral_coefficient[quant_spec_pcw_escape_idx + 1] =
                            Self::decode_escape_sequence(
                                bs,
                                spectral_coefficient[quant_spec_pcw_escape_idx + 1],
                                *remaining_bits - i8::try_from(num_decoded_bits).unwrap(),
                                error_word,
                            );
                        num_decoded_bits = i32::try_from(valid_bits - bs.valid_bits()).unwrap();
                    }

                    *left_sos += num_decoded_bits;
                    *remaining_bits -= i8::try_from(num_decoded_bits).unwrap();

                    // one more PCW should be decoded
                    if max_allowed_cw_len
                        < (u8::try_from(num_decoded_bits).unwrap()
                            + hcr_const::ERROR_PCW_BODY_SIGN_ESC_TOO_LONG)
                    {
                        *error_word |= hcr_const::TOO_MANY_PCW_BODY_SIGN_ESC_BITS_DECODED;
                    }

                    if Self::err_detect_pcw_segmentation(
                        *remaining_bits - hcr_const::ERROR_PCW_BODY_SIGN_ESC as i8,
                        hcr_const::PcwType::BodySignEsc,
                        &mut spectral_coefficient[quant_spec_pcw_escape_idx
                            ..quant_spec_pcw_escape_idx + hcr_const::DIMENSION_OF_ESCAPE_CODEBOOK],
                        error_word,
                    ) {
                        return 1;
                    }
                    quantized_spec_coeff_idx += dimension;
                } // num_codewords iter
            } // if else case
            cw_offset += num_codewords;
        } // num_section loop

        // Write back indexes into structure
        self.num_extended_sorted_sections_in_sets_index += 1;
        self.num_extended_sorted_codeword_in_section_index += num_sections;
        self.extended_sorted_codebook_index += num_sections;
        self.max_len_of_cb_in_ext_srt_sec_index += num_sections;
        *quantized_spectral_coefficients_idx = quantized_spec_coeff_idx;

        0 // success
    }

    /// Decodes the body of a priority codeword (PCW)
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `current_tree`: Current tree belonging to current codebook
    ///
    /// # Return
    ///   Returns pointer offset to first of two or four quantized spectral coefficients
    fn decode_pcw_body(bs: &mut Bitstream, current_tree: &[u32]) -> usize {
        let mut branch_value: u32 = 0;
        let mut branch_node: u32 = 0;

        // get first node of current tree belonging to current codebook
        let mut tree_node = current_tree[0];

        // decode whole PCW-codeword-body
        loop {
            let carry_bit = bs.read_bit() != 0;

            hcr_utils::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );

            if branch_node & TEST_BIT_10 == TEST_BIT_10 {
                // test bit 10 ; if set --> codeword-body is complete
                // end of branch in tree reached  i.e. a whole PCW-Body is decoded
                break;
            } else {
                // update treeNode for further step in decoding tree
                tree_node = current_tree[branch_value as usize];
            }
        }

        //  pointer offset to valid first of 2 or 4 quantized values
        branch_value as usize
    }

    /// # Description:
    ///
    /// Decodes the sign bits of a priority codeword (PCW) and writes
    /// out the resulting quantized spectral values into unsorted sections.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `quant_val`: Quandtized spectral values
    /// - `quant_spec_coeff`: Quandtized spectral coefficients with sign
    ///
    /// # Output:
    ///
    /// Two or four lines at position in corresponding section
    /// (which are not located at the desired position, i.e. they must be reordered).
    fn decode_pcw_sign(bs: &mut Bitstream, quant_val: &[i8], quant_spec_coeff: &mut [i16]) {
        for (q_val, q_spec_coeff) in quant_val.iter().zip(quant_spec_coeff.iter_mut()) {
            *q_spec_coeff = if *q_val != 0 {
                // adapt sign of values according to the decoded sign bit
                if bs.read_bit() != 0 {
                    -i16::from(*q_val)
                } else {
                    i16::from(*q_val)
                }
            } else {
                0
            };
        }
    }

    /// This function checks immediately after every decoded PCW,
    /// whether out of the current segment too many bits have been read or not. If an
    /// error occurs, probably the sideinfo or the HCR-bitstream block holding the
    /// huffman encoded quantized spectral coefficients is distorted. In this case the
    /// two or four quantized spectral coefficients belonging to the current codeword
    /// are marked (for being detected by concealment later).
    ///
    /// # Parameters
    ///
    /// - `remaining_bits_in_segment`: Number of remaining bits in segment
    /// - `pcw_type`: Type of priority codeword
    /// - `qsc_base_of_cw`: quantized spectral coefficient buffer
    /// - `error_word`: To log error type, updated on return
    ///
    /// # Return
    ///   Returns bool, true if error is detected, otherwise false
    fn err_detect_pcw_segmentation(
        remaining_bits_in_segment: i8,
        pcw_type: hcr_const::PcwType,
        qsc_base_of_cw: &mut [i16],
        error_word: &mut u32,
    ) -> bool {
        let mut is_error_detected = false;
        if remaining_bits_in_segment < 0 {
            // log the error
            *error_word |= match pcw_type {
                hcr_const::PcwType::Body => hcr_const::SEGMENT_OVERRIDE_ERR_PCW_BODY,
                hcr_const::PcwType::BodySign => hcr_const::SEGMENT_OVERRIDE_ERR_PCW_BODY_SIGN,
                hcr_const::PcwType::BodySignEsc => {
                    hcr_const::SEGMENT_OVERRIDE_ERR_PCW_BODY_SIGN_ESC
                }
            };
            // mark the erred lines
            for cw in qsc_base_of_cw.iter_mut() {
                *cw = hcr_const::Q_VALUE_INVALID;
            }
            is_error_detected = true;
        }
        is_error_detected
    }

    /// This function decodes one escape sequence. In case of a escape codebook
    /// and in case of the absolute value of the quantized spectral value == 16,
    /// a escapeSequence is decoded in two steps:
    ///     1. escape prefix
    ///     2. escape word
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `quant_spec_coeff`: quantized spectral coefficient value
    /// - `remaining_bits`: Number of remaining bits
    /// - `error_word`: To log error type, updated on return
    ///
    /// # Return
    ///   Returns decoded value of escape word
    fn decode_escape_sequence(
        bs: &mut Bitstream,
        quant_spec_coeff: i16,
        mut remaining_bits: i8,
        error_word: &mut u32,
    ) -> i16 {
        let mut escape_ones_counter = 0;
        let mut escape_word = 0;

        // decode escape prefix
        loop {
            remaining_bits -= 1;
            if remaining_bits < 0 {
                return hcr_const::Q_VALUE_INVALID;
            }

            if bs.read_bit() != 0 {
                escape_ones_counter += 1;
            } else {
                escape_ones_counter += 4;
                break;
            }
        }

        //   decode escape word
        for _i in 0..escape_ones_counter {
            let carry_bit = bs.read_bit();
            remaining_bits -= 1;
            if remaining_bits < 0 {
                return hcr_const::Q_VALUE_INVALID;
            }

            escape_word <<= 1;
            escape_word |= carry_bit as i32;
        }

        if escape_ones_counter < 13 {
            // sign = (quant_spec_coef >= 0) ? 1 : -1;
            if quant_spec_coeff >= 0 {
                (1_i16 << escape_ones_counter) + escape_word as i16
            } else {
                -((1_i16 << escape_ones_counter) + escape_word as i16)
            }
        } else {
            *error_word |= hcr_const::TOO_MANY_PCW_BODY_SIGN_ESC_BITS_DECODED;
            hcr_const::Q_VALUE_INVALID
        }
    }
}
