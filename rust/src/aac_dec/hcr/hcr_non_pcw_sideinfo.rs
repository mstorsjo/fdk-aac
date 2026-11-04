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
//! HCR side information for non-priority keywords

use super::hcr_constants as hcr_const;
use super::hcr_utils::{self, TEST_BIT_10};
use crate::aac_dec::constants;
use crate::aac_dec::hcr::hcr_info::{HcrSectionInfo, HcrSegmentInfo};
use crate::common::bitstream::Bitstream;
use itertools::izip;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub(super) struct HcrNonPcwSideinfo {
    node: [u32; constants::MAX_FRAMESIZE / 4],
    result_pointer: [u16; constants::MAX_FRAMESIZE / 4],
    escape_sequence_info: [u32; constants::MAX_FRAMESIZE / 4],
    codeword_offset: u32,
    state: i8,
    codebook: [u8; constants::MAX_FRAMESIZE / 4],
    cnt_sign: [u8; constants::MAX_FRAMESIZE / 4],
    sta: [i8; constants::MAX_FRAMESIZE / 4],
}
impl Default for HcrNonPcwSideinfo {
    fn default() -> Self {
        Self {
            node: [0_u32; constants::MAX_FRAMESIZE / 4],
            result_pointer: [0_u16; constants::MAX_FRAMESIZE / 4],
            escape_sequence_info: [0_u32; constants::MAX_FRAMESIZE / 4],
            codeword_offset: 0_u32,
            state: 0_i8,
            codebook: [0_u8; constants::MAX_FRAMESIZE / 4],
            cnt_sign: [0_u8; constants::MAX_FRAMESIZE / 4],
            sta: [0_i8; constants::MAX_FRAMESIZE / 4],
        }
    }
}
impl HcrNonPcwSideinfo {
    /// Decodes all non-priority codewords (non-PCWs) by using a state-machine.
    ///
    /// # Parameters
    ///
    /// - `hcr_segment_info`:     Mut reference to a HcrSegmentInfo struct
    /// - `hcr_section_info`:     Mut reference to a HcrSectionInfo struct
    /// - `bs`:                   Bitstream instance with valid internal data
    /// - `bs_anchor`:            Bitstream anchor value at start of HCR decoding
    /// - `spec_coeff`:           Slice of spectral coefficients
    /// - `quant_spec_coeff_idx`: Index value for spectral coeffiecients
    /// - `error_word`:           Logged error type, updated on return
    ///
    /// # Return
    ///
    ///  0 : Success, 1 : Error
    #[expect(clippy::too_many_arguments)]
    pub(super) fn decode_non_pcws(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        hcr_section_info: &mut HcrSectionInfo,
        bs: &mut Bitstream,
        bs_anchor: isize,
        spec_coeff: &mut [i16],
        quant_spec_coeff_idx: &mut usize,
        error_word: &mut u32,
    ) -> u32 {
        let frame_length = spec_coeff.len();
        let num_valid_segment = hcr_segment_info.init_segment_bitfield();

        if num_valid_segment != 0 {
            let mut num_codeword = hcr_section_info.num_codeword;
            let num_set = (((num_codeword - 1) / hcr_segment_info.num_segment) + 1) as usize;

            hcr_segment_info.read_direction = hcr_const::FROM_RIGHT_TO_LEFT;

            // process sets subsequently
            for current_set in 1..num_set {
                if current_set >= hcr_const::MAX_HCR_SETS {
                    return 1;
                }

                // step 1
                let mut codeword_in_set;
                // number of remaining non PCWs [for all sets]
                num_codeword -= hcr_segment_info.num_segment;
                if num_codeword < hcr_segment_info.num_segment {
                    // for last set
                    codeword_in_set = num_codeword;
                } else {
                    // for all sets except last set
                    codeword_in_set = hcr_segment_info.num_segment;
                }

                // step 2
                // prepare array 'codeword_bitfield'; as much ones are written from left in
                // all words, as much decoded_codeword_in_set_counter nonPCWs exist in this set
                let mut temp_word = 0xFFFFFFFF;
                let num_word_for_bit_field = usize::from(hcr_segment_info.num_word_for_bit_field);

                // loop over all used words
                for cbf in hcr_segment_info
                    .codeword_bit_field
                    .iter_mut()
                    .take(num_word_for_bit_field)
                {
                    // more codewords than number of bits => fill ones
                    if codeword_in_set > hcr_const::NUMBER_OF_BIT_IN_WORD {
                        // fill a whole word with ones
                        *cbf = temp_word;
                        codeword_in_set -= hcr_const::NUMBER_OF_BIT_IN_WORD;
                    // subtract number of bits
                    } else {
                        // prepare last temp_word
                        for remaining_codewords_in_set in
                            codeword_in_set..(hcr_const::NUMBER_OF_BIT_IN_WORD)
                        {
                            // set a zero at bit number (NUMBER_OF_BIT_IN_WORD-1-i) in `temp_word`
                            temp_word &= !(1
                                << (hcr_const::NUMBER_OF_BIT_IN_WORD
                                    - 1
                                    - remaining_codewords_in_set));
                        }
                        *cbf = temp_word;
                        temp_word = 0x00000000;
                    }
                }

                // step 3
                // build non-PCW sideinfo for each non-PCW of the current set
                if self.init_non_pcw_side_information_for_current_set(
                    hcr_section_info,
                    frame_length,
                    quant_spec_coeff_idx,
                ) != 0
                {
                    return 1;
                }

                // step 4
                // decode all non-PCWs belonging to this set

                // loop over trials
                let mut codeword_offset_base = 0;
                for _trial in (0..hcr_segment_info.num_segment).rev() {
                    // loop over number of words in bitfields
                    hcr_segment_info.segment_offset = 0; // start at zero in every segment
                    self.codeword_offset = codeword_offset_base; // store in structure for states

                    for bfw in 0..num_word_for_bit_field {
                        // derive  `temp_word` with bitwise and
                        temp_word = hcr_segment_info.segment_bit_field[bfw]
                            & hcr_segment_info.codeword_bit_field[bfw];

                        // if tempWord is not zero, decode something
                        if temp_word != 0 {
                            // loop over all bits in tempWord;
                            for bit_in_word in (1..hcr_const::NUMBER_OF_BIT_IN_WORD + 1).rev() {
                                let intermediate_word = 1_u32 << (bit_in_word - 1);
                                if temp_word & intermediate_word == intermediate_word {
                                    // get state and start state machine
                                    self.state = self.sta[self.codeword_offset as usize];
                                    let seg_offset = hcr_segment_info.segment_offset as usize;
                                    while self.state != hcr_const::STOP_THIS_STATE {
                                        let start_of_segment = if hcr_segment_info.read_direction
                                            == hcr_const::FROM_LEFT_TO_RIGHT
                                        {
                                            hcr_segment_info.left_start_of_segment[seg_offset]
                                        } else {
                                            hcr_segment_info.right_start_of_segment[seg_offset]
                                        };

                                        let num_bits =
                                            bs.valid_bits() - bs_anchor + start_of_segment as isize;
                                        bs.push(num_bits);
                                        let valid_bits = bs.valid_bits();

                                        let error_status = STATE_CONST_2_STATE[self.state as usize](
                                            self,
                                            hcr_segment_info,
                                            bs,
                                            spec_coeff,
                                        );
                                        if error_status != 0 {
                                            *error_word |= error_status;
                                            return 1;
                                        }

                                        if hcr_segment_info.read_direction
                                            == hcr_const::FROM_LEFT_TO_RIGHT
                                        {
                                            hcr_segment_info.left_start_of_segment[seg_offset] +=
                                                i32::try_from(valid_bits - bs.valid_bits()).unwrap()
                                        } else {
                                            hcr_segment_info.right_start_of_segment[seg_offset] +=
                                                i32::try_from(valid_bits - bs.valid_bits()).unwrap()
                                        };
                                    }
                                }

                                // update both offsets
                                hcr_segment_info.segment_offset += 1;
                                self.codeword_offset = u32::try_from(Self::modulo_value(
                                    i32::try_from(self.codeword_offset).unwrap() + 1,
                                    hcr_segment_info.num_segment,
                                ))
                                .unwrap();
                            }
                        } else {
                            hcr_segment_info.segment_offset += hcr_const::NUMBER_OF_BIT_IN_WORD;
                            self.codeword_offset = u32::try_from(Self::modulo_value(
                                i32::try_from(
                                    self.codeword_offset + hcr_const::NUMBER_OF_BIT_IN_WORD,
                                )
                                .unwrap(),
                                hcr_segment_info.num_segment,
                            ))
                            .unwrap();
                        }
                    } // end of bitfield word loop

                    // index of the current codeword base lies within modulo range
                    codeword_offset_base = u32::try_from(Self::modulo_value(
                        i32::try_from(codeword_offset_base).unwrap() - 1,
                        hcr_segment_info.num_segment,
                    ))
                    .unwrap();

                    // rotate numSegment bits in codewordBitfield
                    // rotation of *numSegment bits in bitfield of codewords (circle-rotation)
                    // get last valid bit
                    let mut temp_bit = hcr_segment_info.codeword_bit_field
                        [num_word_for_bit_field - 1]
                        & (1_u32
                            << (hcr_const::NUMBER_OF_BIT_IN_WORD as u16
                                - hcr_segment_info.num_bit_valid_in_last_word));
                    temp_bit >>= (hcr_const::NUMBER_OF_BIT_IN_WORD as u16)
                        - hcr_segment_info.num_bit_valid_in_last_word;

                    // write zero into place where `temp_bit` was fetched from
                    hcr_segment_info.codeword_bit_field[num_word_for_bit_field - 1] &= !(1_u32
                        << (hcr_const::NUMBER_OF_BIT_IN_WORD as u16
                            - hcr_segment_info.num_bit_valid_in_last_word));

                    // rotate last valid word
                    hcr_segment_info.codeword_bit_field[num_word_for_bit_field - 1] >>= 1;

                    // transfer carry bit 0 from current word into bitposition 31 from next word and
                    // rotate current word.
                    if i16::try_from(num_word_for_bit_field).unwrap() - 2 >= 0 {
                        for bfw in (0..num_word_for_bit_field - 2 + 1).rev() {
                            // get carry (=bit at position 0) from current word
                            let carry = hcr_segment_info.codeword_bit_field[bfw] & 1;

                            // put the carry bit at position 31 into word right from current word
                            hcr_segment_info.codeword_bit_field[bfw + 1] |=
                                carry << (hcr_const::NUMBER_OF_BIT_IN_WORD - 1);

                            // shift current word
                            hcr_segment_info.codeword_bit_field[bfw] >>= 1;
                        }
                    }

                    // put `temp_bit` into free bit-position 31 from first word
                    hcr_segment_info.codeword_bit_field[0] |=
                        temp_bit << (hcr_const::NUMBER_OF_BIT_IN_WORD - 1);
                } // end of trial loop

                // toggle read direction
                hcr_segment_info.read_direction =
                    if hcr_segment_info.read_direction == hcr_const::FROM_LEFT_TO_RIGHT {
                        hcr_const::FROM_RIGHT_TO_LEFT
                    } else {
                        hcr_const::FROM_LEFT_TO_RIGHT
                    };
            } // end pf set loop
              // all non-PCWs of this spectrum are decoded
        }

        0 // all PCWs and all non PCWs are decoded. They are unbacksorted in output buffer.
    }

    ///  Sets up sideinfo for the non-PCW decoder (for the current set).
    fn init_non_pcw_side_information_for_current_set(
        &mut self,
        hcr_section_info: &mut HcrSectionInfo,
        frame_length: usize,
        quantized_spectral_coefficients_idx: &mut usize,
    ) -> u32 {
        let num_sections = u32::from(
            hcr_section_info.num_extended_sorted_sections_in_sets
                [hcr_section_info.num_extended_sorted_sections_in_sets_index as usize],
        );
        if hcr_section_info.num_extended_sorted_codeword_in_section_index + num_sections
            > (hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS) as u32
            || hcr_section_info.extended_sorted_codebook_index + num_sections
                > (hcr_const::MAX_SFB_HCR + hcr_const::MAX_HCR_SETS) as u32
            || hcr_section_info.num_extended_sorted_sections_in_sets_index + 1
                > hcr_const::MAX_HCR_SETS as u32
        {
            return 1;
        }

        let mut quant_spec_coeffs_idx =
            u16::try_from(*quantized_spectral_coefficients_idx).unwrap();
        let mut cw_offset = 0;

        // loop over number of extended sorted sections in the current set,
        // so all codewords sideinfo variables within this set can be prepared for decoding
        for (ext_cb, num_cw) in izip!(
            hcr_section_info.extended_sorted_codebook
                [(hcr_section_info.extended_sorted_codebook_index as usize)..]
                .iter(),
            hcr_section_info.num_extended_sorted_codeword_in_section
                [(hcr_section_info.num_extended_sorted_codeword_in_section_index as usize)..]
                .iter()
        )
        .take(num_sections as usize)
        {
            let code_book = usize::from(*ext_cb);
            let num_codwords = *num_cw;

            let state = hcr_const::CODEBOOK_2_START_INT[code_book];
            let i_node = hcr_const::HUFF_TABLE[code_book][0];
            let codebook_dim = u16::from(hcr_const::DIM_CB[code_book]);

            if (cw_offset + usize::from(num_codwords)) > (frame_length >> 2) {
                return 1;
            }

            if (quant_spec_coeffs_idx + codebook_dim * num_codwords)
                > u16::try_from(frame_length).unwrap()
            {
                return 1;
            }

            let num_codwords = usize::from(*num_cw);
            for (cbook, sta, inode, cnt_sign, iresult_ptr, escape_seq_info) in izip!(
                self.codebook[cw_offset..cw_offset + num_codwords].iter_mut(),
                self.sta[cw_offset..cw_offset + num_codwords].iter_mut(),
                self.node[cw_offset..cw_offset + num_codwords].iter_mut(),
                self.cnt_sign[cw_offset..cw_offset + num_codwords].iter_mut(),
                self.result_pointer[cw_offset..cw_offset + num_codwords].iter_mut(),
                self.escape_sequence_info[cw_offset..cw_offset + num_codwords].iter_mut()
            )
            .take(num_codwords)
            {
                *cbook = *ext_cb;
                *sta = state;
                *inode = i_node;
                *cnt_sign = 0;
                *iresult_ptr = quant_spec_coeffs_idx;
                *escape_seq_info = 0;

                // update pointer by codebookDim --> point to next starting value for writing out.
                quant_spec_coeffs_idx += codebook_dim;
            }
            cw_offset += num_codwords;
        }

        // Write back indices
        hcr_section_info.num_extended_sorted_sections_in_sets_index += 1;
        hcr_section_info.num_extended_sorted_codeword_in_section_index += num_sections;
        hcr_section_info.extended_sorted_codebook_index += num_sections;
        *quantized_spectral_coefficients_idx = usize::from(quant_spec_coeffs_idx);

        0 // return
    }

    // =========================================================================================
    //                          the states of the state machine
    // =========================================================================================

    ///  # Description:
    ///
    /// Decodes the body of a codeword. This state is used for codebooks 1,2,5 and 6.
    /// No sign bits are decoded, because the table of the quantized spectral values
    /// has got a valid sign at the quantized spectral lines.
    ///
    /// # Output:
    ///
    /// Two or four quantizes spectral values written at position where `self.result_pointer` points
    /// to.
    fn hcr_state_body_only(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let cb_index = usize::from(self.codebook[cw_offset]);
        let mut tree_node = self.node[cw_offset];
        let current_tree = hcr_const::HUFF_TABLE[cb_index];
        let cb_dimension = usize::from(hcr_const::DIM_CB[cb_index]);
        let quant_val_base = hcr_const::QUANT_TABLE[cb_index];
        let result = &mut result_base[usize::from(self.result_pointer[cw_offset])..];
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        loop {
            let mut branch_node: u32 = 0;
            let mut branch_value: u32 = 0;
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // make a step in decoding tree
            hcr_utils::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );

            // if end of branch reached write out lines and count bits needed for sign,
            // otherwise store node in codeword sideinfo
            if branch_node & TEST_BIT_10 == TEST_BIT_10 {
                // test bit 10; ==> body is complete
                for (res_item, quant_val_base_item) in izip!(
                    result.iter_mut(),
                    quant_val_base[(branch_value as usize)..].iter()
                )
                .take(cb_dimension)
                {
                    *res_item = i16::from(*quant_val_base_item);
                }

                // clear a bit in bitfield and switch off state machine
                Self::clear_bit_from_bitfield(
                    &mut self.state,
                    seg_offset,
                    &mut hcr_segment_info.codeword_bit_field,
                );

                remaining_bits -= 1;
                break; // end of branch in tree reached i.e. a whole nonPCW-Body is decoded
            } else {
                // body is not decoded completely
                // update treeNode for further step in decoding tree
                tree_node = current_tree[branch_value as usize];
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.node[cw_offset] = tree_node;
        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_ONLY;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// Decodes the codeword body, writes out result and counts
    /// the number of quantized spectral values which are different from zero.
    /// For those values sign bits are needed.
    ///
    /// If sign bit counter `cnt_sign` is different from zero,
    /// switch to next state to decode sign bits there.
    /// If sign bit counter `cnt_sign` is zero, no
    /// sign bits are needed and codeword is decoded.
    ///
    /// # output:
    ///
    /// Two or four written quantizes spectral values written at position where
    /// `self.result_pointer` points to. The signs of those lines may be wrong.
    /// If the signs (or just one single sign) is wrong, the next state will correct it.
    fn hcr_state_body_sign_body(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let cb_index = usize::from(self.codebook[cw_offset]);
        let current_tree = hcr_const::HUFF_TABLE[cb_index];
        let mut tree_node = self.node[cw_offset];
        let cb_dimension = hcr_const::DIM_CB[cb_index];
        let quant_val_base = hcr_const::QUANT_TABLE[cb_index];
        let result = &mut result_base[usize::from(self.result_pointer[cw_offset])..];
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        loop {
            let mut branch_node: u32 = 0;
            let mut branch_value: u32 = 0;
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // make a step in decoding tree
            hcr_utils::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );

            // if end of branch reached write out lines and count bits needed for sign,
            // otherwise store node in codeword sideinfo
            if branch_node & TEST_BIT_10 == TEST_BIT_10 {
                // test bit 10; if set body complete
                // wrong sign and count number of values which are different from zero for
                // sign bit decoding [which happens in next state]
                let mut cnt_sign = 0;
                for (res_item, quant_val_base_item) in izip!(
                    result.iter_mut(),
                    quant_val_base[(branch_value as usize)..].iter()
                )
                .take(usize::from(cb_dimension))
                {
                    *res_item = i16::from(*quant_val_base_item);
                    if *quant_val_base_item != 0 {
                        cnt_sign += 1;
                    }
                }

                if cnt_sign == 0 {
                    // clear a bit in bitfield and switch off state machine
                    Self::clear_bit_from_bitfield(
                        &mut self.state,
                        seg_offset,
                        &mut hcr_segment_info.codeword_bit_field,
                    );
                } else {
                    // write sign count result into codewordsideinfo of current codeword
                    self.cnt_sign[cw_offset] = cnt_sign;
                    // change state
                    self.sta[cw_offset] = hcr_const::BODY_SIGN__SIGN;
                    // get state from separate array of cw-sideinfo
                    self.state = self.sta[cw_offset];
                }

                remaining_bits -= 1;

                // end of branch in tree reached i.e. a whole nonPCW-Body is decoded
                break;
            } else {
                // body is not decoded completely
                // update `tree_node` for further step in decoding tree
                tree_node = current_tree[branch_value as usize];
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.node[cw_offset] = tree_node;
        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_BODY;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// This state decodes the sign bits belonging to a codeword.
    /// The state is called as often in different "trials" until `cnt_sign[codewordOffset]` is zero.
    ///
    /// # Output:
    ///
    /// The two or four quantizes spectral values (written in previous state) have now the correct
    /// sign.
    fn hcr_state_body_sign_sign(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let frame_length = result_base.len();
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let mut i_qsc = usize::from(self.result_pointer[cw_offset]);
        let mut cnt_sign = i16::from(self.cnt_sign[cw_offset]);
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        // loop for sign bit decoding
        loop {
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // Search for a line (which was decoded in previous state) which is not zero.
            // This value will get a sign.
            while result_base[i_qsc] == 0 {
                i_qsc += 1;
                if i_qsc >= frame_length {
                    // points to current value different from zero
                    return hcr_const::STATE_ERROR_BODY_SIGN_SIGN;
                }
            }

            // put sign together with line; if `carry_bit` is zero, the sign is ok already;
            // no write operation necessary in this case
            if carry_bit {
                if let Some(negated_val) = result_base[i_qsc].checked_neg() {
                    // carry_bit = 1 --> minus
                    result_base[i_qsc] = negated_val;
                } else {
                    result_base[i_qsc] = hcr_const::Q_VALUE_INVALID;
                }
            }

            i_qsc += 1; // update to next (maybe valid) value

            cnt_sign -= 1;
            if cnt_sign == 0 {
                // if (cntSign==0)  ==>  set state CODEWORD_DECODED
                // clear a bit in bitfield and switch off state machine
                Self::clear_bit_from_bitfield(
                    &mut self.state,
                    seg_offset,
                    &mut hcr_segment_info.codeword_bit_field,
                );
                remaining_bits -= 1;
                break; // whole nonPCW-Body and according sign bits are decoded
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.cnt_sign[cw_offset] = u8::try_from(cnt_sign).unwrap();
        self.result_pointer[cw_offset] = u16::try_from(i_qsc).unwrap();

        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_SIGN;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// Decodes the codeword body in case of codebook is 11.
    /// Writes out resulting two or four lines [with probably wrong sign] and
    /// counts the number of lines, which are different from zero.
    /// This information is needed in next state where sign bits will be decoded, if necessary.
    /// If sign bit counter `cnt_sign` is zero, no sign bits are needed and codeword is decoded
    /// completely.
    ///
    /// # Output:
    ///
    /// Two lines (quantizes spectral coefficients) which are probably wrong.
    /// The sign may be wrong and if one or two values is/are 16, the following
    /// states will decode the escape sequence to correct the values which are written here.
    fn hcr_state_body_sign_esc_body(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let current_tree = hcr_const::HUFF_TABLE[hcr_const::ESCAPE_CODEBOOK];
        let mut tree_node = self.node[cw_offset];
        let quant_val_base = hcr_const::QUANT_TABLE[hcr_const::ESCAPE_CODEBOOK];
        let result = &mut result_base[usize::from(self.result_pointer[cw_offset])..];
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        loop {
            let mut branch_node: u32 = 0;
            let mut branch_value: u32 = 0;
            let carry_bit = bs.read_bit() > 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // make a step in decoding tree
            hcr_utils::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );

            // if end of branch reached write out lines and count bits needed for sign,
            // otherwise store node in codeword sideinfo
            if branch_node & TEST_BIT_10 == TEST_BIT_10 {
                // test bit 10; if set body complete
                tree_node = u32::from(self.result_pointer[cw_offset]);
                // wrong sign and count number of values which are different from zero for
                // sign bit decoding [which happens in next state]
                let mut cnt_sign = 0;
                for (res_item, quant_val_base_item) in izip!(
                    result.iter_mut(),
                    quant_val_base[(branch_value as usize)..].iter()
                )
                .take(hcr_const::DIMENSION_OF_ESCAPE_CODEBOOK)
                {
                    *res_item = i16::from(*quant_val_base_item);
                    if *quant_val_base_item != 0 {
                        cnt_sign += 1;
                    }
                }

                if cnt_sign == 0 {
                    // clear a bit in bitfield and switch off state machine
                    Self::clear_bit_from_bitfield(
                        &mut self.state,
                        seg_offset,
                        &mut hcr_segment_info.codeword_bit_field,
                    );
                } else {
                    // write sign count result into codewordsideinfo of current codeword
                    self.cnt_sign[cw_offset] = cnt_sign;
                    self.sta[cw_offset] = hcr_const::BODY_SIGN_ESC__SIGN; // change state
                    self.state = self.sta[cw_offset]; // get state from separate array of
                                                      // cw-sideinfo
                }

                remaining_bits -= 1;
                break; // end of branch in tree reached i.e. a whole nonPCW-Body is decoded
            } else {
                // body is not decoded completely
                // update treeNode for further step in decoding tree
                // and store updated treeNode because maybe no more bits left in segment.
                tree_node = current_tree[branch_value as usize];
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.node[cw_offset] = tree_node;
        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_ESC_BODY;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// This state decodes the sign bits, if a codeword of codebook 11 needs some.
    /// A flag named `flag_b` in codeword sideinfo is set, if the second line
    /// of quantized spectral values is 16. The `flag_b` is used in case of decoding of a
    /// escape sequence is necessary as far as the second line is concerned.
    ///
    /// If only the first line needs an escape sequence, the `flag_b` is cleared.
    /// If only the second line needs an escape sequence, the `flag_b` is not used.
    ///
    /// For storing sideinfo in case of escape sequence decoding one
    /// single word can be used for both escape sequences because they are decoded not
    /// at the same time:
    //                    bit 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5
    //  4  3  2  1  0
    //                        ===== == == =========== ===========
    //  =================================== ^      ^  ^         ^            ^
    //  ^ |      |  |         |            |                    | res. flag_a  flag_b
    //  escape_prefix_up  escape_prefix_down  escape_word
    /// # Output:
    ///
    /// Two lines with correct sign. If one or two values is/are 16,
    /// the lines are not valid, otherwise they are.
    fn hcr_state_body_sign_esc_sign(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let frame_length = result_base.len();
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let mut i_qsc = usize::from(self.result_pointer[cw_offset]);
        let mut cnt_sign = i16::from(self.cnt_sign[cw_offset]);
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        // loop for sign bit decoding
        loop {
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // Get a quantized spectral value (which was decoded in previous state)
            // which is not zero. This value will get a sign.
            while result_base[i_qsc] == 0 {
                i_qsc += 1;
                if i_qsc >= frame_length {
                    // points to current value different from zero
                    return hcr_const::STATE_ERROR_BODY_SIGN_ESC_SIGN;
                }
            }

            // put negative sign together with quantized spectral value;
            // if carry_bit is zero, the sign is ok already; no write operation necessary in this
            // case
            if carry_bit {
                if let Some(negated_val) = result_base[i_qsc].checked_neg() {
                    // carry_bit = 1 --> minus
                    result_base[i_qsc] = negated_val;
                } else {
                    result_base[i_qsc] = hcr_const::Q_VALUE_INVALID;
                }
            }

            i_qsc += 1; // update to next (maybe valid) value

            cnt_sign -= 1;
            if cnt_sign == 0 {
                let mut flags = 0;

                // all sign bits are decoded now

                // last reinitialzation of for loop counter (see above) is done here
                remaining_bits -= 1;

                // check decoded values if codeword is decoded: Check if one or two escape sequences
                // 16 follow

                // step 0
                // restore pointer to first decoded quantized value [ = original pResultPointr]
                // from index iNode prepared in State_BODY_SIGN_ESC__BODY
                let tree_node = self.node[cw_offset] as usize;

                // step 1
                // test first value if escape sequence follows
                if result_base[tree_node].unsigned_abs() == hcr_const::ESCAPE_VALUE {
                    flags |= hcr_const::MASK_FLAG_A;
                }

                // step 2
                // test second value if escape sequence follows
                if result_base[tree_node + 1].unsigned_abs() == hcr_const::ESCAPE_VALUE {
                    flags |= hcr_const::MASK_FLAG_B;
                }

                // step 3
                // evaluate flag result and go on if necessary
                if flags == 0 {
                    // clear a bit in bitfield and switch off state machine
                    Self::clear_bit_from_bitfield(
                        &mut self.state,
                        seg_offset,
                        &mut hcr_segment_info.codeword_bit_field,
                    );
                } else {
                    // at least one of two lines is 16
                    self.escape_sequence_info[cw_offset] = flags;

                    // set next state
                    self.sta[cw_offset] = hcr_const::BODY_SIGN_ESC__ESC_PREFIX;
                    // get state from separate array of cw-sideinfo
                    self.state = self.sta[cw_offset];
                    // set result pointer to the first line of the two decoded lines
                    i_qsc = self.node[cw_offset] as usize;

                    if (flags & hcr_const::MASK_FLAG_A == 0) && (flags & hcr_const::MASK_FLAG_B > 0)
                    {
                        // update pResultPointr ==> state Stat_BODY_SIGN_ESC__ESC_WORD writes
                        // to correct position. Second value is the one and only escape value
                        i_qsc += 1;
                    }
                }
                break; // nonPCW-Body at cb 11 and according sign bits are decoded
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.cnt_sign[cw_offset] = u8::try_from(cnt_sign).unwrap();
        self.result_pointer[cw_offset] = u16::try_from(i_qsc).unwrap();

        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_ESC_SIGN;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// Decode escape prefix of first or second escape sequence.
    /// The escape prefix consists of ones. The following zero is also decoded here.
    ///
    /// # Output:
    ///
    /// If the single separator-zero which follows the escape-prefix-ones is not yet decoded:
    /// The value `escape_prefix_up` in word `escape_sequence_info[codeword_offset]` is updated.
    ///
    /// If the single separator-zero which follows the escape-prefix-ones is decoded:
    /// Two updated values 'escape_prefix_up' and 'escape_prefix_down' in word
    /// `escape_sequence_info[codeword_offset]`. This State is finished. Switch to next state.
    fn hcr_state_body_sign_esc_esc_prefix(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        _result_base: &mut [i16],
    ) -> u32 {
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let mut escape_seq_info = self.escape_sequence_info[cw_offset];
        let mut escape_prefix_up =
            (escape_seq_info & hcr_const::MASK_ESCAPE_PREFIX_UP) >> hcr_const::LSB_ESCAPE_PREFIX_UP;
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        // decode escape prefix
        loop {
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // ount ones and store sum in escape_prefix_up
            if carry_bit {
                escape_prefix_up += 1; // update counter for ones
                if escape_prefix_up > 8 {
                    return hcr_const::STATE_ERROR_BODY_SIGN_ESC_ESC_PREFIX;
                }
                // store updated counter in sideinfo of current codeword
                // delete old escapePrefixUp
                escape_seq_info &= !hcr_const::MASK_ESCAPE_PREFIX_UP;
                // shift to correct position
                escape_prefix_up <<= hcr_const::LSB_ESCAPE_PREFIX_UP;
                // insert new escapePrefixUp
                escape_seq_info |= escape_prefix_up;
                // shift back down
                escape_prefix_up >>= hcr_const::LSB_ESCAPE_PREFIX_UP;
            } else {
                // last reinitialization of for loop counter (see above) is done here
                remaining_bits -= 1;
                // if escape_separator '0' appears, add 4 and ==> break
                escape_prefix_up += 4;

                // store escape_prefix_up in escape_seq_info[codeword_offset] at bit position
                // escape_prefix_up
                // delete old escape_prefix_up
                escape_seq_info &= !hcr_const::MASK_ESCAPE_PREFIX_UP;
                // shift to correct position
                escape_prefix_up <<= hcr_const::LSB_ESCAPE_PREFIX_UP;
                // insert new escapePrefixUp
                escape_seq_info |= escape_prefix_up;
                // shift back down
                escape_prefix_up >>= hcr_const::LSB_ESCAPE_PREFIX_UP;

                // store escape_prefix_up in escape_seq_info[codeword_offset] at bit position
                // escape_prefix_down
                // delete old escape_prefix_down
                escape_seq_info &= !hcr_const::MASK_ESCAPE_PREFIX_DOWN;
                // shift to correct position
                escape_prefix_up <<= hcr_const::LSB_ESCAPE_PREFIX_DOWN;
                // insert new escape_prefix_down
                escape_seq_info |= escape_prefix_up;

                // set next state
                self.sta[cw_offset] = hcr_const::BODY_SIGN_ESC__ESC_WORD;
                // get state from separate array of cw_sideinfo
                self.state = self.sta[cw_offset];

                break;
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.escape_sequence_info[cw_offset] = escape_seq_info;
        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_ESC_ESC_PREFIX;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// Decode `escape_word` of escape sequence. If the escape sequence is decoded completely,
    /// assemble quantized-spectral-escape-coefficient and
    /// replace the previous decoded 16 by the new value. Test `flag_b`. If `flag_b` is set,
    /// the second escape sequence must be decoded. If `flag_b` is not set, the codeword is
    /// decoded and the state machine is switched off.
    ///
    /// # Output:
    ///
    /// Two lines with valid sign. At least one of both lines has got the correct value.
    fn hcr_state_body_sign_esc_esc_word(
        self: &mut HcrNonPcwSideinfo,
        hcr_segment_info: &mut HcrSegmentInfo,
        bs: &mut Bitstream,
        result_base: &mut [i16],
    ) -> u32 {
        let cw_offset = self.codeword_offset as usize;
        let seg_offset = hcr_segment_info.segment_offset as usize;
        let mut escape_seq_info = self.escape_sequence_info[cw_offset];
        let mut escape_word = escape_seq_info & hcr_const::MASK_ESCAPE_WORD;
        let mut escape_prefix_down = (escape_seq_info & hcr_const::MASK_ESCAPE_PREFIX_DOWN)
            >> hcr_const::LSB_ESCAPE_PREFIX_DOWN;
        let mut i_qsc = usize::from(self.result_pointer[cw_offset]);
        let mut remaining_bits = hcr_segment_info.remaining_bits_in_segment[seg_offset];

        // decode escape word
        loop {
            let carry_bit = bs.read_bit() != 0;
            if hcr_segment_info.read_direction == hcr_const::FROM_RIGHT_TO_LEFT {
                bs.push(-2);
            }

            // build escape word
            // left shift previous decoded part of escapeWord by on bit
            escape_word <<= 1;
            // assemble escape word by bitwise or
            escape_word |= u32::from(carry_bit);

            // decrement counter for length of escape word because one more bit was decoded
            escape_prefix_down -= 1;

            // store updated escape_prefix_down
            // delete old escape_prefix_down
            escape_seq_info &= !hcr_const::MASK_ESCAPE_PREFIX_DOWN;
            // shift to correct position
            escape_prefix_down <<= hcr_const::LSB_ESCAPE_PREFIX_DOWN;
            // insert new escapePrefixDown
            escape_seq_info |= escape_prefix_down;
            // shift back
            escape_prefix_down >>= hcr_const::LSB_ESCAPE_PREFIX_DOWN;

            // store updated escape_word
            // delete old escapeWord
            escape_seq_info &= !hcr_const::MASK_ESCAPE_WORD;
            // insert new escape_word
            escape_seq_info |= escape_word;

            if escape_prefix_down == 0 {
                // last reinitialzation of for loop counter (see above) is done here
                remaining_bits -= 1;

                // escape sequence decoded. Assemble escape-line and replace original line.

                // step 0
                // derive sign
                // get sign of escape value 16
                let sign: i32 = if result_base[i_qsc] >= 0 { 1 } else { -1 };

                // step 1
                // get escape_prefix_up
                let escape_prefix_up = (escape_seq_info & hcr_const::MASK_ESCAPE_PREFIX_UP)
                    >> hcr_const::LSB_ESCAPE_PREFIX_UP;

                // step 2
                // calculate escape value
                result_base[i_qsc] = i16::try_from(
                    sign * ((1_i32 << escape_prefix_up) + i32::try_from(escape_word).unwrap()),
                )
                .unwrap();

                // step 3
                // clear the whole escape sideinfo word
                if escape_seq_info & hcr_const::MASK_FLAG_A != 0 {
                    if escape_seq_info & hcr_const::MASK_FLAG_B == 0 {
                        // clear a bit in bitfield and switch off state machine
                        Self::clear_bit_from_bitfield(
                            &mut self.state,
                            seg_offset,
                            &mut hcr_segment_info.codeword_bit_field,
                        );
                    } else {
                        i_qsc += 1; // update to the next and last 16

                        // change state
                        self.sta[cw_offset] = hcr_const::BODY_SIGN_ESC__ESC_PREFIX;
                        // get state from separate array of cw-sideinfo
                        self.state = self.sta[cw_offset];
                    }
                } else {
                    // clear a bit in bitfield and switch off state machine
                    Self::clear_bit_from_bitfield(
                        &mut self.state,
                        seg_offset,
                        &mut hcr_segment_info.codeword_bit_field,
                    );
                }

                escape_seq_info = 0;
                break;
            }

            remaining_bits -= 1;
            if remaining_bits <= 0 {
                break;
            }
        }

        self.escape_sequence_info[cw_offset] = escape_seq_info;
        self.result_pointer[cw_offset] = u16::try_from(i_qsc).unwrap();
        hcr_segment_info.remaining_bits_in_segment[seg_offset] = remaining_bits;

        if remaining_bits <= 0 {
            // clear a bit in bitfield and switch off statemachine
            Self::clear_bit_from_bitfield(
                &mut self.state,
                seg_offset,
                &mut hcr_segment_info.segment_bit_field,
            );

            if remaining_bits < 0 {
                return hcr_const::STATE_ERROR_BODY_SIGN_ESC_ESC_WORD;
            }
        }

        0 // return
    }

    /// # Description:
    ///
    /// It does nothing. It is passed as the first member of `STATE_CONST_2_STATE[]`.
    fn hcr_state_dummy(
        self: &mut HcrNonPcwSideinfo,
        _hcr_segment_info: &mut HcrSegmentInfo,
        _bs: &mut Bitstream,
        _result_base: &mut [i16],
    ) -> u32 {
        0 // return
    }

    /// Clears a bit from current bitfield and switches off the state machine.
    /// A bit is cleared in two cases:
    ///     a) a codeword is decoded, then a bit is cleared in codeword bitfield
    ///     b) a segment is decoded empty, then a bit is cleared in segment bitfield
    fn clear_bit_from_bitfield(state: &mut i8, offset: usize, bitfield: &mut [u32]) {
        let num_bitfield_word = offset >> hcr_const::THIRTYTWO_LOG_DIV_TWO_LOG; // int   = wordNr
        let num_bitfield_bit =
            u32::try_from(offset - (num_bitfield_word << hcr_const::THIRTYTWO_LOG_DIV_TWO_LOG))
                .unwrap(); // fract = bitNr

        // clear a bit in bitfield
        bitfield[num_bitfield_word] &=
            !(1 << (hcr_const::NUMBER_OF_BIT_IN_WORD - 1 - num_bitfield_bit));

        // switch off state machine because codeword is decoded and/or because segment is empty
        *state = hcr_const::STOP_THIS_STATE;
    }

    fn modulo_value(input: i32, bufferlength: u32) -> i32 {
        let buff_len = i32::try_from(bufferlength).unwrap();
        if input > (buff_len - 1) {
            input - buff_len
        } else if input < 0 {
            input + buff_len
        } else {
            input
        }
    }
}

/// Function pointer. It calls the states of the `HcrNonPcwSideinfo` state machine.
type Statefunc = fn(
    hcr_non_pcw_sideinfo: &mut HcrNonPcwSideinfo,
    hcr_segment_info: &mut HcrSegmentInfo,
    bs: &mut Bitstream,
    result_base: &mut [i16],
) -> u32;

static STATE_CONST_2_STATE: [Statefunc; 8] = [
    HcrNonPcwSideinfo::hcr_state_dummy,
    HcrNonPcwSideinfo::hcr_state_body_only,
    HcrNonPcwSideinfo::hcr_state_body_sign_body,
    HcrNonPcwSideinfo::hcr_state_body_sign_sign,
    HcrNonPcwSideinfo::hcr_state_body_sign_esc_body,
    HcrNonPcwSideinfo::hcr_state_body_sign_esc_sign,
    HcrNonPcwSideinfo::hcr_state_body_sign_esc_esc_prefix,
    HcrNonPcwSideinfo::hcr_state_body_sign_esc_esc_word,
];

#[cfg(test)]
mod tests {
    use super::*;

    // Variables are improperly initialized. But it should suffice for a proof of concept,
    // for calling the functions from STATE_CONST_2_STATE[].
    #[should_panic]
    #[test]
    fn run_fn_ptr() {
        let mut hcr_non_pcw_sideinfo = HcrNonPcwSideinfo::default();
        let mut hcr_segment_info = HcrSegmentInfo::default();
        let mut bs = Bitstream::default();
        let mut result_base: [i16; 4] = Default::default();

        let fn_ptr = STATE_CONST_2_STATE[1];
        fn_ptr(
            &mut hcr_non_pcw_sideinfo,
            &mut hcr_segment_info,
            &mut bs,
            &mut result_base[..],
        );
    }
}
