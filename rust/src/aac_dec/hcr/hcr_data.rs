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
//! HCR interface

use crate::{
    aac_dec::{channel_info::IcsInfo, error_codes::AacDecoderError},
    common::{bitstream::Bitstream, bs_element_id::ChannelElementId},
};

use super::{
    hcr_constants as hcr_const,
    hcr_info::{HcrInfo, HcrSectionInfo, HcrSegmentInfo},
    hcr_non_pcw_sideinfo::HcrNonPcwSideinfo,
};

#[derive(Default, Debug, Clone)]
#[repr(C)]
pub struct HcrData {
    segment_info: HcrSegmentInfo,
    section_info: HcrSectionInfo,
    non_pcw_side_info: HcrNonPcwSideinfo,
    hcr_info: Vec<HcrInfo>,
}

impl HcrData {
    /// Decodes the codewords of the spectral coefficients from the bitstream
    /// according to the HCR algorithm and stores the quantized spectral coefficients
    /// in correct order in the output buffer.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Code book for huffman decode reordering
    /// - `channel`: Element channel
    /// - `spectral_coefficient`: spectral coeffiecients
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{channel_info::IcsInfo, constants::MAX_WINS_X_SFBS, hcr::HcrData};
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// // Precondition, should have a valid Bitstream and IcsInfo instance
    /// // Refer respective components for instance creation, read/write methods
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 8);
    ///
    /// let ics_info = IcsInfo::new();
    ///
    /// let num_elements_channels = 2;
    /// let mut hcr_data = HcrData::new(num_elements_channels);
    /// let channel = 0;
    /// let code_book = vec![0_u8; MAX_WINS_X_SFBS];
    /// let mut spectral_coefficient = vec![0_i16; 1024];
    /// hcr_data.decode(
    ///     &mut bitstream_reader,
    ///     &ics_info,
    ///     &code_book,
    ///     channel,
    ///     &mut spectral_coefficient,
    /// );
    /// ```
    pub fn decode(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        code_book: &[u8],
        channel: usize,
        spectral_coefficient: &mut [i16],
    ) -> u32 {
        // advanced Huffman decoding starts here (HCR decoding :)
        let hcr_info = &mut self.hcr_info[channel];

        // clear result array
        spectral_coefficient[..].fill(0);

        if hcr_info.len_of_reordered_spectral_data != 0 {
            let mut error_log: u32 = 0;
            let bs_anchor = bs.valid_bits();

            hcr_info.resort_side_info(
                ics_info,
                code_book,
                spectral_coefficient.len(),
                &mut error_log,
            );

            if error_log & hcr_const::HCR_FATAL_PCW_ERROR_MASK != 0 {
                return 1; // core concealment
            }

            self.section_info.reset_length_and_index_info();
            self.section_info.calc_num_codeword(
                &hcr_info.code_book[..hcr_info.number_section],
                &hcr_info.num_lines_in_section[..hcr_info.number_section],
            );

            self.section_info.sort_codebook_and_num_codeword_in_section(
                &hcr_info.code_book[..hcr_info.number_section],
                hcr_info.number_section,
            );

            self.segment_info.prepare_segmentation_grid(
                &self.section_info,
                hcr_info.len_of_longest_codeword,
                hcr_info.len_of_reordered_spectral_data,
            );

            self.section_info.extended_section_info(
                hcr_info.len_of_longest_codeword,
                self.segment_info.num_segment as u16,
                &mut error_log,
            );

            if error_log & hcr_const::HCR_FATAL_PCW_ERROR_MASK != 0 {
                return 1; // core concealment
            }

            if self
                .section_info
                .derive_number_of_extended_sorted_sections_in_sets(self.segment_info.num_segment)
                != 0
            {
                return 1; // core concealment
            }

            // ------- decode meaningful PCWs ------
            let mut quantized_spectral_coefficients_idx = 0;
            let error_status = self.section_info.decode_pcws(
                &mut self.segment_info,
                bs,
                bs_anchor,
                spectral_coefficient,
                &mut quantized_spectral_coefficients_idx,
                &mut error_log,
            );

            if error_status != 0 || error_log & hcr_const::HCR_FATAL_PCW_ERROR_MASK != 0 {
                return 1; // core concealment
            }

            // ------ decode the non-PCWs --------
            let error_status = self.non_pcw_side_info.decode_non_pcws(
                &mut self.segment_info,
                &mut self.section_info,
                bs,
                bs_anchor,
                spectral_coefficient,
                &mut quantized_spectral_coefficients_idx,
                &mut error_log,
            );

            if error_status != 0 || error_log & hcr_const::HCR_FATAL_PCW_ERROR_MASK != 0 {
                return 1; // core concealment
            }

            self.segment_info
                .err_detect_within_segmentation_final(&mut error_log);
            self.section_info.reorder_quantized_spectral_coefficients(
                ics_info,
                hcr_info.number_section,
                spectral_coefficient,
                &mut error_log,
            );

            if error_log != 0 {
                Self::mute_erroneous_lines(spectral_coefficient);
            }

            let num_bits =
                bs.valid_bits() - bs_anchor + hcr_info.len_of_reordered_spectral_data as isize;
            bs.push(num_bits);
        }

        0 // success
    }

    /// Create a `HcrData` instance
    ///
    /// # Parameters
    ///
    /// - `num_element_channels`: Indicates number of element channels to be used
    pub fn new(num_element_channels: usize) -> Self {
        HcrData {
            hcr_info: vec![HcrInfo::new(); num_element_channels],
            ..Default::default()
        }
    }

    /// Reads HCR sideinfo components from bitstream
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `global_hcr_type`: MP4 Channel element type
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{constants, hcr::HcrData};
    /// use aac::common::{
    ///     bitstream::{Bitstream, Mode},
    ///     bs_element_id::ChannelElementId,
    /// };
    ///
    /// // Precondition, should have a valid Bitstream instance in reader mode
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 64);
    /// let num_elements_channels = 2;
    /// let mut hcr_data = HcrData::new(num_elements_channels);
    /// let channel = 0;
    /// let hcr_type = ChannelElementId::Cpe;
    /// hcr_data.read(&mut bitstream_reader, channel, hcr_type);
    /// ```
    pub fn read(&mut self, bs: &mut Bitstream, channel: u32, global_hcr_type: ChannelElementId) {
        self.hcr_info[channel as usize].read(bs, global_hcr_type);
    }

    /// Get HCR `number_section` for a particular `channel` (see `HcrData::new()`).
    pub fn get_number_section(&self, channel: usize) -> usize {
        self.hcr_info[channel].number_section
    }

    /// Set HCR `number_section` for a particular `channel` (see `HcrData::new()`).
    pub fn set_number_section(&mut self, channel: usize, val: usize) {
        self.hcr_info[channel].number_section = val;
    }

    /// Set the codebook and the corresponding number of lines in a section,
    /// for each "number_section".
    ///
    /// # Parameters
    ///
    /// - `channel`: Element channel
    /// - `code_book`: Code book for huffman decode reordering
    /// - `num_lines_in_section`:  number of lines corresponding to a section
    ///
    /// # Return:
    ///
    /// - `AacDecoderError`
    pub fn set_codebook_and_num_lines_in_section(
        &mut self,
        channel: usize,
        codebook: u8,
        num_lines_in_section: u16,
    ) -> AacDecoderError {
        let number_section = self.hcr_info[channel].number_section;
        if number_section >= hcr_const::MAX_SFB_HCR {
            return AacDecoderError::ParseError;
        }
        self.hcr_info[channel].num_lines_in_section[number_section] = num_lines_in_section;
        self.hcr_info[channel].code_book[number_section] = codebook;

        AacDecoderError::Ok
    }

    /// Mutes spectral lines which have been marked as erroneous (Q_VALUE_INVALID)
    fn mute_erroneous_lines(spectral_coefficient: &mut [i16]) {
        //   if there is a line with value Q_VALUE_INVALID mute it
        for spec_coeff in spectral_coefficient.iter_mut() {
            if *spec_coeff == hcr_const::Q_VALUE_INVALID {
                *spec_coeff = 0;
            }
        }
    }
}
