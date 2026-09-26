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
//! RVLC interface

use super::rvlc_concealment;
use super::rvlc_info::{RvlcErrorStatus, RvlcInfo};
use crate::aac_dec::{channel_info::IcsInfo, pns::PnsData};
use crate::common::bitstream::Bitstream;

/// ER RVLC data
#[derive(Default, Debug, Copy, Clone)]
#[repr(C)]
pub struct RvlcData {
    is_current_scale_factor_ok: bool,
    is_intensity_stereo_used: bool,
    is_current_block_long: bool, // BlockType::Long: True, BlockType::{Short, Start, Stop}: False
    concealment_info: rvlc_concealment::RvlcConcealment,
    rvlc_info: RvlcInfo,
}

impl RvlcData {
    /// The function reads both the escape sequences and the scalefactors
    /// in forward and backward direction. If an error has occurred during
    /// decoding process, which can not be concealed with the rvlc concealment frame,
    /// concealment will be initiated. Then the element "is_current_scale_factor_ok"
    /// is set to 'false', otherwise it is set to 'true'.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `pns_data`: Perceptual noise substitution related data updated on return
    /// - `scale_factors`: Scalefactors for each band in each window
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::channel_info::IcsInfo;
    /// use aac::aac_dec::{constants::MAX_WINS_X_SFBS, pns::PnsData, rvlc::RvlcData};
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// // Precondition: should have a valid Bitstream and IcsInfo instance.
    /// // Refer respective components for instance creation, read/write methods.
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 40);
    ///
    /// let ics_info = IcsInfo::new();
    /// let mut pns_data: PnsData = Default::default();
    /// let mut rvlc_data = RvlcData::new();
    /// let mut code_book = vec![0_u8; MAX_WINS_X_SFBS];
    /// let mut scale_factors = vec![0_i16; MAX_WINS_X_SFBS];
    /// let global_gain = 50;
    ///
    /// rvlc_data.decode(
    ///     &mut bitstream_reader,
    ///     &ics_info,
    ///     &code_book,
    ///     global_gain,
    ///     &mut pns_data,
    ///     &mut scale_factors,
    /// );
    /// ```
    pub fn decode(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        pns_data: &mut PnsData,
        scale_factors: &mut [i16],
    ) {
        self.rvlc_info
            .decode(bs, ics_info, code_book, global_gain, pns_data);

        self.is_intensity_stereo_used = self.rvlc_info.is_intensity_stereo_used();
        self.is_current_block_long = ics_info.n_window_groups() <= 1;

        let mut er_status: RvlcErrorStatus = Default::default();
        self.rvlc_info
            .final_error_detection(ics_info, global_gain, &mut er_status);
        self.is_current_scale_factor_ok = self.rvlc_info.apply_concealment(
            ics_info,
            code_book,
            global_gain,
            &er_status,
            &mut self.concealment_info,
            scale_factors,
        );
    }

    /// Creates new RvlcData instance.
    pub fn new() -> RvlcData {
        RvlcData::default()
    }

    /// Resets RvlcData instance.
    pub fn reset(&mut self) {
        self.concealment_info.set_previous_scale_factor_ok(false);
    }

    /// Reads RVLC ESC1 data (side info) from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{channel_info::IcsInfo, constants::MAX_WINS_X_SFBS, rvlc::RvlcData};
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// // Precondition: should have a valid Bitstream and IcsInfo instance.
    /// // Refer respective components for instance creation, read/write methods.
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 40);
    ///
    /// let ics_info = IcsInfo::new();
    /// let mut rvlc_data = RvlcData::new();
    /// let mut code_book = vec![0_u8; MAX_WINS_X_SFBS];
    ///
    /// rvlc_data.read(&mut bitstream_reader, &ics_info, &code_book);
    /// ```
    pub fn read(&mut self, bs: &mut Bitstream, ics_info: &IcsInfo, code_book: &[u8]) {
        // Reset rvlc channel data
        self.is_current_scale_factor_ok = true;
        self.is_intensity_stereo_used = false;
        self.is_current_block_long = false;

        // Read rvlc info from bitstream
        self.rvlc_info.read(bs, ics_info, code_book);
    }

    /// Performs sanity checks on the channel data corresponding to one channel element.
    ///
    /// # Parameters
    ///
    /// - `rvlc_data_right`: RVLC right channel element
    /// - `is_ms_mask_present`: Flag indicates MS stereo mask presence
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::rvlc::RvlcData;
    ///
    /// let mut rvlc_data_left = RvlcData::new();
    /// let mut rvlc_data_right = RvlcData::new();
    /// let is_ms_mask_present = true;
    ///
    /// rvlc_data_left.element_check(Some(&mut rvlc_data_right), is_ms_mask_present);
    /// ```
    pub fn element_check(
        &mut self,
        rvlc_data_right: Option<&mut RvlcData>,
        is_ms_mask_present: bool,
    ) {
        // RVLC specific sanity checks
        if let Some(rvlc_right) = rvlc_data_right {
            if (!self.is_current_scale_factor_ok || !rvlc_right.is_current_scale_factor_ok)
                && is_ms_mask_present
            {
                self.is_current_scale_factor_ok = false;
                rvlc_right.is_current_scale_factor_ok = false;
            }

            if !self.is_current_scale_factor_ok
                && rvlc_right.is_current_scale_factor_ok
                && rvlc_right.is_intensity_stereo_used
            {
                rvlc_right.is_current_scale_factor_ok = false;
            }

            // Right RVLC concealment data
            rvlc_right
                .concealment_info
                .set_previous_block(rvlc_right.is_current_block_long);
            self.concealment_info
                .set_previous_scale_factor_ok(rvlc_right.is_current_scale_factor_ok);
        }

        // Left RVLC concealment data
        self.concealment_info
            .set_previous_block(self.is_current_block_long);
        self.concealment_info
            .set_previous_scale_factor_ok(self.is_current_scale_factor_ok);
    }
}
