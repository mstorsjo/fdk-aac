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
//! RVLC module-internal info

use itertools::izip;

use crate::aac_dec::{channel_info::IcsInfo, constants, hcr, pns, utils};
use crate::common::bitstream::Bitstream;

use super::rvlc_concealment;
use super::rvlc_constants as rvlc_const;

/// ER RVLC info
#[rustfmt::skip]
#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub(super) struct RvlcInfo {
    // Error Sensitivity Class 1 Data   // Order of RVLC-bitstream components in bitstream
                                        // (RVLC-initialization).
                                        // Every component appears only once in bitstream.
    is_scf_concealment_present: bool,   // 1
    rev_global_gain: i16,               // 2
    length_of_rvlc_scf: i16,            // 3           // Original value, gets modified
                                                       // (subtract 9) in case of noise (PNS);
                                                       // is kept for later use
    dpcm_noise_nrg: i16,                // 4 optional
    is_scf_escapes_present: bool,       // 5
    length_of_rvlc_escapes: i16,        // 6 optional

    dpcm_noise_last_pos: i16,           // 7 optional
    dpcm_intensity_stereo_last_pos: i16,

    length_of_rvlc_scf_fwd: i16,        // length_of_rvlc_scf used for forward decoding
    length_of_rvlc_scf_bwd: i16,        // length_of_rvlc_scf used for backward decoding

    first_noise_band: u8,
    direction: u8,

    // bitstream indices
    bs_anchor: isize,                   // hcr bit buffer reference index
    bs_index_rvl_fwd: i16,              // base address of RVL-coded-scalefactor data (ESC 2)
                                        // for forward  decoding
    bs_index_rvl_bwd: i16,              // base address of RVL-coded-scalefactor data (ESC 2)
                                        // for backward decoding
    bs_index_esc: i16,                  // base address where RVLC-escapes start (ESC 2)

    scf_esc: [i16; rvlc_const::RVLC_MAX_SFB],
    scf_fwd: [i16; rvlc_const::RVLC_MAX_SFB],
    scf_bwd: [i16; rvlc_const::RVLC_MAX_SFB],

    // escape counters
    num_decoded_escape_words_fwd: u8,   // when decoding RVL-codes forward
    num_decoded_escape_words_bwd: u8,   // when decoding RVL-codes backward
    num_decoded_escape_words_esc: u8,   // when decoding the escape-words

    is_noise_used: bool,
    is_intensity_stereo_used: bool,
    is_scf_used: bool,

    first_scf: i16,
    last_scf: i16,
    first_nrg: i16,
    last_nrg: i16,
    first_intensity_stereo: i16,
    last_intensity_stereo: i16,

    // ------ RVLC error detection ------
    error_log_rvlc: u32,                // store RVLC errors
    conceal_min: i16,                   // is set at backward decoding
    conceal_max: i16,                   // is set at forward  decoding
    conceal_min_esc: i16,               // is set at backward decoding
    conceal_max_esc: i16,               // is set at forward  decoding

    conceal_first_band: u8,
    conceal_last_band: u8,
    conceal_first_group: u8,
    conceal_last_group: u8,
}

/// List of error status flags.
/// Defaults to false.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
pub(super) struct RvlcErrorStatus {
    last_scf: bool,
    first_scf: bool,
    last_nrg: bool,
    first_nrg: bool,
    last_intensity_stereo: bool,
    first_intensity_stereo: bool,
    forbidden_cw_fwd: bool,
    forbidden_cw_bwd: bool,
    length_fwd: bool,
    length_bwd: bool,
    length_escapes: bool,
    num_escapes_fwd: bool,
    num_escapes_bwd: bool,
}

/// Default method for RvlcInfo.
impl Default for RvlcInfo {
    fn default() -> Self {
        Self {
            is_scf_concealment_present: false,
            rev_global_gain: 0,
            length_of_rvlc_scf: 0,
            dpcm_noise_nrg: 0,
            is_scf_escapes_present: false,
            length_of_rvlc_escapes: 0,
            dpcm_noise_last_pos: 0,
            dpcm_intensity_stereo_last_pos: 0,
            length_of_rvlc_scf_fwd: 0,
            length_of_rvlc_scf_bwd: 0,
            first_noise_band: 0,
            direction: 0,
            bs_anchor: 0,
            bs_index_rvl_fwd: 0,
            bs_index_rvl_bwd: 0,
            bs_index_esc: 0,
            scf_esc: [0_i16; rvlc_const::RVLC_MAX_SFB],
            scf_fwd: [0_i16; rvlc_const::RVLC_MAX_SFB],
            scf_bwd: [0_i16; rvlc_const::RVLC_MAX_SFB],
            num_decoded_escape_words_fwd: 0,
            num_decoded_escape_words_bwd: 0,
            num_decoded_escape_words_esc: 0,
            is_noise_used: false,
            is_intensity_stereo_used: false,
            is_scf_used: false,
            first_scf: 0,
            last_scf: 0,
            first_nrg: 0,
            last_nrg: 0,
            first_intensity_stereo: 0,
            last_intensity_stereo: 0,
            error_log_rvlc: 0,
            conceal_min: 0,
            conceal_max: 0,
            conceal_min_esc: 0,
            conceal_max_esc: 0,
            conceal_first_band: 0,
            conceal_last_band: 0,
            conceal_first_group: 0,
            conceal_last_group: 0,
        }
    }
}

impl RvlcInfo {
    /// Applies concealment to scale factors if any RVLC error is detected.
    /// Returns status of current scale factors. true - ok, false - not ok.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `er_status:` Type of RVLC error detected
    /// - `rvlc_conceal`: RVLC concealment info
    /// - `scale_factors`: Scalefactors for each band in each window
    pub(super) fn apply_concealment(
        &mut self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        er_status: &RvlcErrorStatus,
        rvlc_conceal: &mut rvlc_concealment::RvlcConcealment,
        scale_factors: &mut [i16],
    ) -> bool {
        let mut is_curr_scf_ok = true;
        let mut conceal_status: bool = true;

        scale_factors[0..rvlc_const::RVLC_MAX_SFB].fill(0);

        if !er_status.is_error_status_complete() {
            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

            izip!(
                scale_factors.chunks_exact_mut(bands_per_window),
                rvlc_conceal
                    .previous_scale_factors_mut()
                    .chunks_exact_mut(bands_per_window),
                self.scf_fwd.chunks_exact(bands_per_window)
            )
            .take(ics_info.n_window_groups())
            .for_each(|(scf_win, prev_scf_win, scf_fwd_win)| {
                izip!(
                    scf_win.iter_mut(),
                    prev_scf_win.iter_mut(),
                    scf_fwd_win.iter()
                )
                .take(ics_info.max_sf_bands())
                .for_each(|(scf, prev_scf, scf_fwd)| {
                    *scf = *scf_fwd;
                    *prev_scf = *scf_fwd;
                })
            });

            izip!(
                code_book.chunks_exact(bands_per_window),
                rvlc_conceal
                    .previous_codebooks_mut()
                    .chunks_exact_mut(bands_per_window),
            )
            .take(ics_info.n_window_groups())
            .for_each(|(cbook_win, prev_cbook_win)| {
                izip!(cbook_win.iter(), prev_cbook_win.iter_mut())
                    .take(ics_info.max_sf_bands())
                    .for_each(|(cb, prev_cb)| {
                        *prev_cb = *cb;
                    });
                debug_assert!(ics_info.max_sf_bands() < rvlc_const::RVLC_MAX_SFB);
                prev_cbook_win[ics_info.max_sf_bands()..].fill(utils::CBTYPE_ZERO_HCB);
            });
        } else {
            let is_rvlc_decoded = self.conceal_min != rvlc_const::CONCEAL_MIN_INIT
                || self.conceal_max != rvlc_const::CONCEAL_MAX_INIT;
            // If an error was detected just in forward or backward direction, set the
            // corresponding border for concealment to a appropriate scalefactor band.
            // The border is set to first or last sfb respectively, because the error
            // will possibly not follow directly after the corrupt bit but just after
            // decoding some more (wrong) scalefactors.
            if self.conceal_min == rvlc_const::CONCEAL_MIN_INIT {
                self.conceal_min = 0;
            }
            if self.conceal_max == rvlc_const::CONCEAL_MAX_INIT {
                self.conceal_max = i16::try_from(
                    ics_info.n_window_groups().saturating_sub(1) * constants::MAX_SFB_SHORT
                        + ics_info.max_sf_bands().saturating_sub(1),
                )
                .unwrap();
            }

            let is_current_block_long = ics_info.n_window_groups() <= 1;

            let max_scale_factor_bands = if !is_current_block_long {
                constants::MAX_SFB_SHORT
            } else {
                constants::MAX_SFB_LONG
            } as i16;

            self.conceal_first_band =
                u8::try_from(self.conceal_min % max_scale_factor_bands).unwrap();
            self.conceal_last_band =
                u8::try_from(self.conceal_max % max_scale_factor_bands).unwrap();
            self.conceal_first_group =
                u8::try_from(self.conceal_min / max_scale_factor_bands).unwrap();
            self.conceal_last_group =
                u8::try_from(self.conceal_max / max_scale_factor_bands).unwrap();

            // A single bit error was detected in decoding of dpcm values. It also could
            // be an error with more bits in decoding of escapes and dpcm values, whereby
            // an illegal codeword followed not directly after the corrupted bits, but
            // just after decoding some more (wrong) scalefactors.
            // Use the smaller scalefactor from forward decoding, backward decoding and
            // previous frame.
            if is_rvlc_decoded
                && (self.conceal_min <= self.conceal_max)
                && (rvlc_conceal.is_previous_block_long() == is_current_block_long)
                && rvlc_conceal.is_previous_scale_factor_ok()
                && self.is_scf_concealment_present
                && conceal_status
            {
                self.bidirectional_estimation_use_scf_of_prev_frame_as_reference(
                    ics_info,
                    code_book,
                    rvlc_conceal.previous_codebooks(),
                    rvlc_conceal.previous_scale_factors(),
                    scale_factors,
                );
                conceal_status = false;
            }

            // A single bit error was detected in decoding of dpcm values. It also could
            // be an error with more bits in decoding of escapes and dpcm values, whereby
            // an illegal codeword followed not directly after the corrupted bits, but
            // just after decoding some more (wrong) scalefactors.
            // Use the smaller scalefactor from forward and backward decoding.
            if is_rvlc_decoded
                && (self.conceal_min <= self.conceal_max)
                && !((rvlc_conceal.is_previous_block_long() == is_current_block_long)
                    && rvlc_conceal.is_previous_scale_factor_ok()
                    && self.is_scf_concealment_present)
                && conceal_status
            {
                self.bidirectional_estimation_use_lower_scf_of_current_frame(
                    ics_info,
                    code_book,
                    global_gain,
                    scale_factors,
                );
                conceal_status = false;
            }

            // No errors were detected in decoding of escapes and dpcm values.
            // However, the first and last value of a group (is,nrg,sf) is incorrect.
            if (self.conceal_min <= self.conceal_max)
                && ((er_status.last_scf && er_status.first_scf)
                    || (er_status.last_nrg && er_status.first_nrg)
                    || (er_status.last_intensity_stereo && er_status.first_intensity_stereo))
                && !(er_status.forbidden_cw_fwd
                    || er_status.forbidden_cw_bwd
                    || er_status.length_escapes)
                && conceal_status
            {
                self.statistical_estimation(ics_info, code_book, scale_factors);
                conceal_status = false;
            }

            // An error with more bits in decoding of escapes and dpcm values was
            // detected. Use the smaller scalefactor from forward decoding, backward
            // decoding and previous frame.
            if (self.conceal_min <= self.conceal_max)
                && (rvlc_conceal.is_previous_block_long() == is_current_block_long)
                && rvlc_conceal.is_previous_scale_factor_ok()
                && self.is_scf_concealment_present
                && conceal_status
            {
                self.predictive_interpolation(
                    ics_info,
                    code_book,
                    rvlc_conceal.previous_codebooks(),
                    rvlc_conceal.previous_scale_factors(),
                    scale_factors,
                );
                conceal_status = false;
            }

            // Call frame concealment, because no better strategy was found.
            if conceal_status {
                scale_factors.fill(0);
                is_curr_scf_ok = false;
            }
        }

        is_curr_scf_ok
    }

    /// Determines the scalefactor which is closed to the scalefactor band `conceal_min`.
    /// The same is done for intensity data and noise energies.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `ref_is_fwd`: Reference value intensity data
    /// - `ref_nrg_fwd`: Reference value noise energy
    /// - `ref_scf_fwd`: Reference value scf
    fn calc_refval_fwd(
        &self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        ref_is_fwd: &mut i16,
        ref_nrg_fwd: &mut i16,
        ref_scf_fwd: &mut i16,
    ) {
        // Calculate first reference value for approach in forward direction.
        let (mut id_is, mut id_nrg, mut id_scf) = (true, true, true);

        // Set reference values.
        *ref_is_fwd = -rvlc_const::SF_OFFSET;
        *ref_nrg_fwd = i16::from(global_gain) - pns::NOISE_OFFSET - 256;
        *ref_scf_fwd = i16::from(global_gain) - rvlc_const::SF_OFFSET;

        let mut start_band = usize::from(self.conceal_first_band).saturating_sub(1);
        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

        for (rvlc_scf_fwd, cb) in izip!(
            self.scf_fwd.chunks_exact(bands_per_window),
            code_book.chunks_exact(bands_per_window)
        )
        .take(usize::from(self.conceal_first_group + 1))
        .rev()
        {
            for (fwd_band, cb_band) in rvlc_scf_fwd[..=start_band]
                .iter()
                .zip(cb[..=start_band].iter())
                .rev()
            {
                match *cb_band {
                    utils::CBTYPE_ZERO_HCB => {}
                    utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                        if id_is {
                            *ref_is_fwd = *fwd_band;
                            id_is = false; // Reference value has been set.
                        }
                    }
                    utils::CBTYPE_NOISE_HCB => {
                        if id_nrg {
                            *ref_nrg_fwd = *fwd_band;
                            id_nrg = false; // Reference value has been set.
                        }
                    }
                    _ => {
                        if id_scf {
                            *ref_scf_fwd = *fwd_band;
                            id_scf = false; // Reference value has been set.
                        }
                    }
                }
            }
            start_band = ics_info.max_sf_bands().saturating_sub(1);
        }
    }

    /// Determines the scalefactor which is closed to the scalefactor band `conceal_max`.
    /// The same is done for intensity data and noise energies.
    ///
    /// # Parameters
    ///  - `code_book`:   code book.
    ///  - `ref_is_bwd`:  reference value intensity data.
    ///  - `ref_nrg_bwd`: reference value noise energy.
    ///  - `ref_scf_bwd`: reference value scf.
    fn calc_refval_bwd(
        &self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        ref_is_bwd: &mut i16,
        ref_nrg_bwd: &mut i16,
        ref_scf_bwd: &mut i16,
    ) {
        let (mut id_is, mut id_nrg, mut id_scf) = (true, true, true);

        *ref_is_bwd = self.dpcm_intensity_stereo_last_pos - rvlc_const::SF_OFFSET;
        *ref_nrg_bwd = self.rev_global_gain + self.dpcm_noise_last_pos - pns::NOISE_OFFSET - 256
            + self.dpcm_noise_nrg;
        *ref_scf_bwd = self.rev_global_gain - rvlc_const::SF_OFFSET;

        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        let mut start_band = usize::from(self.conceal_last_band + 1);

        for (code_book_chunk, scf_chunk) in izip!(
            code_book[usize::from(self.conceal_last_group) * bands_per_window..]
                .chunks_exact(bands_per_window),
            self.scf_bwd[usize::from(self.conceal_last_group) * bands_per_window..]
                .chunks_exact(bands_per_window)
        )
        .take(ics_info.n_window_groups())
        {
            for (cb_band, scf) in izip!(
                code_book_chunk[start_band..].iter(),
                scf_chunk[start_band..].iter()
            )
            .take(ics_info.max_sf_bands())
            {
                match *cb_band {
                    utils::CBTYPE_ZERO_HCB => {}
                    utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                        if id_is {
                            *ref_is_bwd = *scf;
                            id_is = false;
                        }
                    }
                    utils::CBTYPE_NOISE_HCB => {
                        if id_nrg {
                            *ref_nrg_bwd = *scf;
                            id_nrg = false;
                        }
                    }
                    _ => {
                        if id_scf {
                            *ref_scf_bwd = *scf;
                            id_scf = false;
                        }
                    }
                }
            }
            start_band = 0;
        }
    }

    /// Checks if given codebook types are used in the current channel.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `cb_types`: Types of codebook to check presence in code_book
    fn check_codebook(&mut self, ics_info: &IcsInfo, code_book: &[u8], cb_types: &[u8]) -> bool {
        let mut is_codebook_used = false;

        if ics_info.max_sf_bands() != 0 {
            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

            'windows: for cb_win in code_book
                .chunks_exact(bands_per_window)
                .take(ics_info.n_window_groups())
            {
                '_bands: for cb_band in cb_win.iter().take(ics_info.max_sf_bands()) {
                    if cb_types.contains(cb_band) {
                        is_codebook_used = true;
                        break 'windows;
                    }
                }
            }
        }

        is_codebook_used
    }

    /// The function reads RVLC data from bitstream and decodes
    /// both the escape sequences and the scalefactors in forward and backward direction.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `pns_data`: Perceptual noise substitution related data updated on return
    #[inline(never)]
    pub(super) fn decode(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        pns_data: &mut pns::PnsData,
    ) {
        self.init(ics_info, code_book);

        let rvlc_length = self.length_of_rvlc_scf
            + if self.is_scf_escapes_present {
                self.length_of_rvlc_escapes
            } else {
                0
            };

        // Store bitstream anchor before decoding.
        self.bs_anchor = bs.valid_bits();

        // decode escapes scale factor
        if self.is_scf_escapes_present {
            bs.push(isize::from(self.bs_index_esc));
            self.decode_escapes(bs);
        }

        // Decode forward scale factor.
        let mut valid_bits = bs.valid_bits();
        bs.push(valid_bits - self.bs_anchor + isize::from(self.bs_index_rvl_fwd));
        self.decode_forward(bs, ics_info, code_book, global_gain, pns_data);

        // Decode backward scale factor.
        valid_bits = bs.valid_bits();
        bs.push(valid_bits - self.bs_anchor + isize::from(self.bs_index_rvl_bwd));
        self.decode_backward(bs, ics_info, code_book, global_gain);

        valid_bits = bs.valid_bits();
        bs.push(valid_bits - self.bs_anchor + isize::from(rvlc_length));

        pns_data.set_is_pns_active(self.is_noise_used());
    }

    ///  Decodes all huffman coded RVLC escape words.
    ///  Here, a difference to the pseudo-code-implementation from standard can be found.
    ///  A loop (and not two nested for loops) is used for two reasons:
    ///  1. The plain huffman encoded escapes are decoded before the RVL-coded scalefactors.
    ///     Therefore the escapes are present in the second step when decoding the RVL-coded
    ///     scalefactor values in forward and backward direction. When the RVL-coded scalefactors
    ///     are decoded and there an escape is needed, then it is just taken out of the array in
    ///     ascending order.
    ///  2. It's faster.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    fn decode_escapes(&mut self, bs: &mut Bitstream) {
        // Decode all RVLC-Escape words with a plain Huffman-Decoder.
        let mut esc_cnt = 0_u8;
        loop {
            if self.length_of_rvlc_escapes <= 0 {
                break;
            }
            let esc_word = self.decode_escape_word(bs);
            if esc_word >= 0 {
                // Update escape scalefactors buffer.
                self.scf_esc[usize::from(esc_cnt)] = esc_word;
                esc_cnt += 1;
            } else {
                self.error_log_rvlc |= rvlc_const::RVLC_ERROR_ALL_ESCAPE_WORDS_INVALID;
                break;
            }
        } // All RVLC escapes decoded.
        self.num_decoded_escape_words_esc = esc_cnt;
    }

    /// Decodes a huffman coded RVLC escape-word. This value is part of a DPCM coded scalefactor.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    ///
    ///  Returns a single RVLC-Escape value, which had to be applied to a DPCM value
    ///  (which has a absolute value of 7).
    fn decode_escape_word(&mut self, bs: &mut Bitstream) -> i16 {
        let mut escape_value: i16 = -1;
        // Esc_tree is always pointing to start of HUFF_TREE_RVLC_ESCAPE[] table.
        // Init at starting node
        let mut tree_node = rvlc_const::HUFF_TREE_RVLC_ESCAPE[0];
        let mut branch_value: u32 = 0;
        let mut branch_node: u32 = 0;
        for length in (0..rvlc_const::MAX_LEN_RVLC_ESCAPE_WORD).rev() {
            let carry_bit = bs.read_bit() != 0;
            hcr::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );

            if branch_node & hcr::TEST_BIT_10 != 0 {
                //  test bit 10 ; If set --> an RVLC-escape-word is completely decoded.
                escape_value = (branch_node & hcr::CLR_ABOVE_BIT_10) as i16;
                self.length_of_rvlc_escapes -= rvlc_const::MAX_LEN_RVLC_ESCAPE_WORD - length;
                if self.length_of_rvlc_escapes < 0 {
                    self.error_log_rvlc |= rvlc_const::RVLC_ERROR_ALL_ESCAPE_WORDS_INVALID;
                    escape_value = -1;
                }
                return escape_value;
            } else {
                // Update tree_node for further step in decoding tree.
                tree_node = rvlc_const::HUFF_TREE_RVLC_ESCAPE[branch_value as usize];
            }
        }
        self.error_log_rvlc |= rvlc_const::RVLC_ERROR_ALL_ESCAPE_WORDS_INVALID;

        escape_value // Should not be reached.
    }

    /// Decodes an RVL-coded DPCM-word.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `direction`: Indicates direction of decoding (forward/backward)
    /// - `rvl_bitcount`: length of RVL scale factor code word in number of bits
    /// - `error_log`: Captures RVLC error types in decoding
    ///
    /// # Return
    ///
    ///  DPCM value within range [0,1,..,14] in case of no errors.
    ///  Otherwise, -1 in case of errors (or) a forbidden codeword is detected.
    ///  The offset of 7 must be subtracted to get a valid DPCM scalefactor value
    ///  (in caller function).
    fn decode_rvl_codeword(
        bs: &mut Bitstream,
        direction: u8,
        rvl_bitcount: &mut i16,
        error_log: &mut u32,
    ) -> i16 {
        let mut dpcm_value: i16 = -1;
        let mut tree_node = rvlc_const::HUFF_TREE_RVL_CODEWDS[0];
        let mut branch_value: u32 = 0;
        let mut branch_node: u32 = 0;
        for length in (0..rvlc_const::MAX_LEN_RVLC_CODE_WORD).rev() {
            let carry_bit = bs.read_bit() != 0;
            if direction == rvlc_const::BWD {
                bs.push(-2);
            }

            hcr::carry_bit_to_branch_value(
                carry_bit,
                tree_node,
                &mut branch_value,
                &mut branch_node,
            );
            if branch_node & hcr::TEST_BIT_10 != 0 {
                //  test bit 10 ; if set --> an RVLC-escape-word is completely decoded.
                dpcm_value = (branch_node & hcr::CLR_ABOVE_BIT_10) as i16;
                *rvl_bitcount -= rvlc_const::MAX_LEN_RVLC_CODE_WORD - length;

                // check available bits for decoding
                if *rvl_bitcount < 0 {
                    *error_log |= if direction == rvlc_const::FWD {
                        rvlc_const::RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_FWD
                    } else {
                        rvlc_const::RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_BWD
                    };
                    // Signalize an error in return value, because too many bits were decoded.
                    dpcm_value = -1;
                }
                // Check max value of dpcm value.
                if dpcm_value > rvlc_const::MAX_ALLOWED_DPCM_INDEX {
                    *error_log |= if direction == rvlc_const::FWD {
                        rvlc_const::RVLC_ERROR_FORBIDDEN_CW_DETECTED_FWD
                    } else {
                        rvlc_const::RVLC_ERROR_FORBIDDEN_CW_DETECTED_BWD
                    };
                    // Signalize an error in return value. A forbidden codeword was detected.
                    dpcm_value = -1;
                }
                return dpcm_value; // Return a dpcm value with offset +7 or an error status.
            } else {
                tree_node = rvlc_const::HUFF_TREE_RVL_CODEWDS[branch_value as usize];
                // Update tree_node for further step in decoding tree.
            }
        }

        dpcm_value // return value
    }

    /// Decodes an RVL-coded DPCM-word codewords in forward direction.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `pns_data`: Perceptual noise substitution related data updated on return
    #[inline(never)]
    fn decode_forward(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        pns_data: &mut pns::PnsData,
    ) {
        let mut dpcm = 0;
        let mut factor = i16::from(global_gain) - rvlc_const::SF_OFFSET;
        let mut position = 0;
        let mut noise_nrg = i16::from(global_gain) - pns::NOISE_OFFSET - 256;

        let mut curr_band = 0;
        self.num_decoded_escape_words_fwd = 0;
        self.direction = rvlc_const::FWD;
        self.is_noise_used = false;
        self.is_scf_used = false;
        self.last_scf = 0;
        self.last_nrg = 0;
        self.last_intensity_stereo = 0;
        let mut scf_esc_index = 0;
        let pns_used = pns_data.is_pns_used_mut();

        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

        // Main loop fwd long.
        for (group, (cb_win, scf_fwd_win)) in code_book
            .chunks_exact(bands_per_window)
            .zip(self.scf_fwd.chunks_exact_mut(bands_per_window))
            .enumerate()
            .take(ics_info.n_window_groups())
        {
            for (band, (cb_band, fwd_band)) in cb_win
                .iter()
                .zip(scf_fwd_win.iter_mut())
                .enumerate()
                .take(ics_info.max_sf_bands())
            {
                curr_band = bands_per_window * group + band;
                match *cb_band {
                    utils::CBTYPE_ZERO_HCB => {
                        *fwd_band = 0;
                    }
                    utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                        // Store dpcm intensity stereo position.
                        let is_dpcm_in_expected_range = Self::decode_forward_estimate_dpcm(
                            bs,
                            self.direction,
                            curr_band,
                            self.length_of_rvlc_escapes,
                            self.scf_esc[scf_esc_index],
                            &mut self.conceal_max_esc,
                            &mut self.num_decoded_escape_words_fwd,
                            &mut scf_esc_index,
                            &mut self.length_of_rvlc_scf_fwd,
                            &mut dpcm,
                            &mut self.error_log_rvlc,
                        );
                        if !is_dpcm_in_expected_range {
                            self.conceal_max = curr_band as i16;
                            return;
                        }

                        position += dpcm;
                        if !(-287..=287).contains(&position) {
                            self.conceal_max = curr_band as i16;
                            return;
                        }
                        *fwd_band = position;
                        self.last_intensity_stereo = position;
                    }
                    utils::CBTYPE_NOISE_HCB => {
                        if !self.is_noise_used {
                            self.is_noise_used = true;
                            self.first_noise_band = curr_band as u8;
                            noise_nrg += self.dpcm_noise_nrg;
                            noise_nrg = pns::clamp_noise_range(noise_nrg);
                            *fwd_band = noise_nrg;
                            self.last_nrg = noise_nrg;
                        } else {
                            let is_dpcm_in_expected_range = Self::decode_forward_estimate_dpcm(
                                bs,
                                self.direction,
                                curr_band,
                                self.length_of_rvlc_escapes,
                                self.scf_esc[scf_esc_index],
                                &mut self.conceal_max_esc,
                                &mut self.num_decoded_escape_words_fwd,
                                &mut scf_esc_index,
                                &mut self.length_of_rvlc_scf_fwd,
                                &mut dpcm,
                                &mut self.error_log_rvlc,
                            );
                            if !is_dpcm_in_expected_range {
                                self.conceal_max = curr_band as i16;
                                return;
                            }
                            noise_nrg += dpcm;
                            noise_nrg = pns::clamp_noise_range(noise_nrg);
                            *fwd_band = noise_nrg;
                            self.last_nrg = noise_nrg;
                        }
                        pns_used[curr_band] = true; // Set pns data used for current band.
                    }
                    _ => {
                        self.is_scf_used = true;
                        let is_dpcm_in_expected_range = Self::decode_forward_estimate_dpcm(
                            bs,
                            self.direction,
                            curr_band,
                            self.length_of_rvlc_escapes,
                            self.scf_esc[scf_esc_index],
                            &mut self.conceal_max_esc,
                            &mut self.num_decoded_escape_words_fwd,
                            &mut scf_esc_index,
                            &mut self.length_of_rvlc_scf_fwd,
                            &mut dpcm,
                            &mut self.error_log_rvlc,
                        );
                        if !is_dpcm_in_expected_range {
                            self.conceal_max = curr_band as i16;
                            return;
                        }
                        factor += dpcm;
                        if !((0 - rvlc_const::SF_OFFSET)..=(255 - rvlc_const::SF_OFFSET))
                            .contains(&factor)
                        {
                            self.conceal_max = curr_band as i16;
                            return;
                        }
                        *fwd_band = factor;
                        self.last_scf = factor;
                    }
                } // match
            } // band - iterator
        } // group - iterator

        // Post-fetch fwd long.
        if self.is_intensity_stereo_used {
            // dpcm_intensity_stereo_last_pos
            let is_dpcm_in_expected_range = Self::decode_forward_estimate_dpcm(
                bs,
                self.direction,
                curr_band,
                self.length_of_rvlc_escapes,
                self.scf_esc[scf_esc_index],
                &mut self.conceal_max_esc,
                &mut self.num_decoded_escape_words_fwd,
                &mut scf_esc_index,
                &mut self.length_of_rvlc_scf_fwd,
                &mut dpcm,
                &mut self.error_log_rvlc,
            );
            if !is_dpcm_in_expected_range {
                self.conceal_max = curr_band as i16;
                return;
            }
            self.dpcm_intensity_stereo_last_pos = dpcm;
        }
    }

    /// Decodes an RVL-coded DPCM-word codewords in backward direction.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    #[inline(never)]
    fn decode_backward(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
    ) {
        let nth_band = ics_info.max_sf_bands().saturating_sub(1);
        let mut factor = self.rev_global_gain - rvlc_const::SF_OFFSET;
        let mut position = self.dpcm_intensity_stereo_last_pos;
        let mut noise_nrg =
            self.rev_global_gain + self.dpcm_noise_last_pos - pns::NOISE_OFFSET - 256;

        // Set scf_esc_index to last entry of self.scf_esc[].
        let mut scf_esc_index = usize::from(self.num_decoded_escape_words_esc).saturating_sub(1);

        self.num_decoded_escape_words_bwd = 0;
        self.direction = rvlc_const::BWD;
        self.first_scf = 0;
        self.first_nrg = 0;
        self.first_intensity_stereo = 0;

        let mut dpcm;

        // Prefetch long BWD.
        if self.is_intensity_stereo_used {
            dpcm = Self::decode_rvl_codeword(
                bs,
                self.direction,
                &mut self.length_of_rvlc_scf_bwd,
                &mut self.error_log_rvlc,
            );

            if dpcm < 0 {
                self.dpcm_intensity_stereo_last_pos = 0;
                self.conceal_min = nth_band as i16;
                return;
            }

            dpcm -= rvlc_const::TABLE_OFFSET;
            if dpcm == rvlc_const::MIN_RVL || dpcm == rvlc_const::MAX_RVL {
                if self.length_of_rvlc_escapes > 0
                    || self.num_decoded_escape_words_bwd >= self.num_decoded_escape_words_esc
                {
                    self.conceal_min = nth_band as i16;
                    return;
                } else {
                    if dpcm == rvlc_const::MIN_RVL {
                        dpcm -= self.scf_esc[scf_esc_index];
                    } else {
                        dpcm += self.scf_esc[scf_esc_index];
                    }
                    scf_esc_index = scf_esc_index.saturating_sub(1);
                    self.num_decoded_escape_words_bwd += 1;

                    if self.conceal_min_esc == rvlc_const::CONCEAL_MIN_INIT {
                        self.conceal_min_esc = nth_band as i16;
                    }
                }
            }
            self.dpcm_intensity_stereo_last_pos = dpcm;
        }

        let mut curr_band;
        let mut offset;

        // Main loop long BWD.
        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        for (group, (cb_win, scf_bwd_win)) in code_book
            .chunks_exact(bands_per_window)
            .zip(self.scf_bwd.chunks_exact_mut(bands_per_window))
            .enumerate()
            .take(ics_info.n_window_groups())
            .rev()
        {
            for (band, (cb_band, bwd_band)) in cb_win
                .iter()
                .zip(scf_bwd_win.iter_mut())
                .enumerate()
                .take(ics_info.max_sf_bands())
                .rev()
            {
                curr_band = bands_per_window * group + band;
                offset = if band == 0 && ics_info.n_window_groups() != 1 {
                    bands_per_window - ics_info.max_sf_bands() + 1
                } else {
                    1
                };

                match *cb_band {
                    utils::CBTYPE_ZERO_HCB => {
                        *bwd_band = 0;
                    }
                    utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                        // Store dpcm intensity stereo position.
                        dpcm = Self::decode_rvl_codeword(
                            bs,
                            self.direction,
                            &mut self.length_of_rvlc_scf_bwd,
                            &mut self.error_log_rvlc,
                        );

                        if dpcm < 0 {
                            *bwd_band = position;
                            self.conceal_min = curr_band.saturating_sub(offset) as i16;
                            return;
                        }

                        dpcm -= rvlc_const::TABLE_OFFSET;
                        if dpcm == rvlc_const::MIN_RVL || dpcm == rvlc_const::MAX_RVL {
                            if self.length_of_rvlc_escapes > 0
                                || self.num_decoded_escape_words_bwd
                                    >= self.num_decoded_escape_words_esc
                            {
                                *bwd_band = position;
                                self.conceal_min = curr_band.saturating_sub(offset) as i16;
                                return;
                            } else {
                                if dpcm == rvlc_const::MIN_RVL {
                                    dpcm -= self.scf_esc[scf_esc_index];
                                } else {
                                    dpcm += self.scf_esc[scf_esc_index];
                                }
                                scf_esc_index = scf_esc_index.saturating_sub(1);
                                self.num_decoded_escape_words_bwd += 1;

                                if self.conceal_min_esc == rvlc_const::CONCEAL_MIN_INIT {
                                    self.conceal_min_esc = curr_band.saturating_sub(offset) as i16;
                                }
                            }
                        }
                        if !(-287..=287).contains(&position) {
                            self.conceal_min = curr_band.saturating_sub(offset) as i16;
                            return;
                        }
                        *bwd_band = position;
                        position -= dpcm;
                        self.first_intensity_stereo = position;
                    }
                    utils::CBTYPE_NOISE_HCB => {
                        if curr_band == usize::from(self.first_noise_band) {
                            *bwd_band = self.dpcm_noise_nrg + i16::from(global_gain)
                                - pns::NOISE_OFFSET
                                - 256;
                            *bwd_band = pns::clamp_noise_range(*bwd_band);
                            self.first_nrg = *bwd_band;
                        } else {
                            dpcm = Self::decode_rvl_codeword(
                                bs,
                                self.direction,
                                &mut self.length_of_rvlc_scf_bwd,
                                &mut self.error_log_rvlc,
                            );

                            if dpcm < 0 {
                                noise_nrg = pns::clamp_noise_range(noise_nrg);
                                *bwd_band = noise_nrg;
                                self.conceal_min = curr_band.saturating_sub(offset) as i16;

                                return;
                            }

                            dpcm -= rvlc_const::TABLE_OFFSET;
                            if dpcm == rvlc_const::MIN_RVL || dpcm == rvlc_const::MAX_RVL {
                                if self.length_of_rvlc_escapes > 0
                                    || self.num_decoded_escape_words_bwd
                                        >= self.num_decoded_escape_words_esc
                                {
                                    noise_nrg = pns::clamp_noise_range(noise_nrg);
                                    *bwd_band = noise_nrg;
                                    self.conceal_min = curr_band.saturating_sub(offset) as i16;
                                    return;
                                } else {
                                    if dpcm == rvlc_const::MIN_RVL {
                                        dpcm -= self.scf_esc[scf_esc_index];
                                    } else {
                                        dpcm += self.scf_esc[scf_esc_index];
                                    }
                                    scf_esc_index = scf_esc_index.saturating_sub(1);
                                    self.num_decoded_escape_words_bwd += 1;

                                    if self.conceal_min_esc == rvlc_const::CONCEAL_MIN_INIT {
                                        self.conceal_min_esc =
                                            curr_band.saturating_sub(offset) as i16;
                                    }
                                }
                            }

                            noise_nrg = pns::clamp_noise_range(noise_nrg);

                            *bwd_band = noise_nrg;
                            noise_nrg -= dpcm;
                            self.first_nrg = noise_nrg;
                        }
                    }
                    _ => {
                        dpcm = Self::decode_rvl_codeword(
                            bs,
                            self.direction,
                            &mut self.length_of_rvlc_scf_bwd,
                            &mut self.error_log_rvlc,
                        );

                        if dpcm < 0 {
                            *bwd_band = factor;
                            self.conceal_min = curr_band.saturating_sub(offset) as i16;
                            return;
                        }

                        dpcm -= rvlc_const::TABLE_OFFSET;
                        if dpcm == rvlc_const::MIN_RVL || dpcm == rvlc_const::MAX_RVL {
                            if self.length_of_rvlc_escapes > 0
                                || self.num_decoded_escape_words_bwd
                                    >= self.num_decoded_escape_words_esc
                            {
                                *bwd_band = factor;
                                self.conceal_min = curr_band.saturating_sub(offset) as i16;
                                return;
                            } else {
                                if dpcm == rvlc_const::MIN_RVL {
                                    dpcm -= self.scf_esc[scf_esc_index];
                                } else {
                                    dpcm += self.scf_esc[scf_esc_index];
                                }
                                scf_esc_index = scf_esc_index.saturating_sub(1);
                                self.num_decoded_escape_words_bwd += 1;

                                if self.conceal_min_esc == rvlc_const::CONCEAL_MIN_INIT {
                                    self.conceal_min_esc = curr_band.saturating_sub(offset) as i16;
                                }
                            }
                        }

                        if !((0 - rvlc_const::SF_OFFSET)..=(255 - rvlc_const::SF_OFFSET))
                            .contains(&factor)
                        {
                            self.conceal_min = curr_band.saturating_sub(offset) as i16;
                            return;
                        }
                        *bwd_band = factor;
                        factor -= dpcm;
                        self.first_scf = factor;
                    }
                } // match
            } // band
        } // group
    }

    /// Decodes and estimates valid DPCM word for forward decoding case.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `direction`: Indicates direction of decoding (forward/backward)
    /// - `curr_band`: Position of current scale factor band in decoding frame
    /// - `escapes_length`: Length of RVLC escapes
    /// - `scf_esc_val`: Value of current scale factor band(RVLC escape word)
    /// - `conceal_max_esc`: Updated with value of curr_band if conceal_max_esc == CONCEAL_MAX_INIT
    /// - `num_decoded_escape_words_fwd`: Number of decoded escape words in forward direction
    /// - `scf_esc_index`: Counter for RVLC scale factor escapes buffer index
    /// - `sf_fwd_length`: Counter which is used for forward decoding
    /// - `dpcm`: Calculated dpcm value after decoding
    /// - `error_log`: Captures RVLC error types in decoding process
    ///
    ///  Returns bool which indicates decoding error and updates dpcm value
    ///     true - Error ; false - No error
    #[inline(always)]
    #[expect(clippy::too_many_arguments)]
    fn decode_forward_estimate_dpcm(
        bs: &mut Bitstream,
        direction: u8,
        curr_band: usize,
        escapes_length: i16,
        scf_esc_val: i16,
        conceal_max_esc: &mut i16,
        num_decoded_escape_words_fwd: &mut u8,
        scf_esc_index: &mut usize,
        sf_fwd_length: &mut i16,
        dpcm: &mut i16,
        error_log: &mut u32,
    ) -> bool {
        *dpcm = Self::decode_rvl_codeword(bs, direction, sf_fwd_length, error_log);

        if *dpcm < 0 {
            // Decoding error - dpcm value is not in the expected range.
            return false;
        }

        *dpcm -= rvlc_const::TABLE_OFFSET;
        if *dpcm == rvlc_const::MIN_RVL || *dpcm == rvlc_const::MAX_RVL {
            if escapes_length > 0 {
                // Decoding error - dpcm value is not in the expected range.
                return false;
            } else {
                if *dpcm == rvlc_const::MIN_RVL {
                    *dpcm -= scf_esc_val;
                } else {
                    *dpcm += scf_esc_val;
                }
                *scf_esc_index += 1;
                *num_decoded_escape_words_fwd += 1;

                if *conceal_max_esc == rvlc_const::CONCEAL_MAX_INIT {
                    *conceal_max_esc = i16::try_from(curr_band).unwrap();
                }
            }
        }
        true // Decoding success - dpcm value is in the expected range.
    }

    /// Detects RVLC decoding errors and updates them via `&mut RvlcErrorStatus`.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `er_status`: type of RVLC error detected.
    pub(super) fn final_error_detection(
        &mut self,
        ics_info: &IcsInfo,
        global_gain: u8,
        er_status: &mut RvlcErrorStatus,
    ) {
        // Invalid escape words, bit counter unequal zero, forbidden codeword detected.
        if self.error_log_rvlc & rvlc_const::RVLC_ERROR_FORBIDDEN_CW_DETECTED_FWD != 0 {
            er_status.forbidden_cw_fwd = true;
        }

        if self.error_log_rvlc & rvlc_const::RVLC_ERROR_FORBIDDEN_CW_DETECTED_BWD != 0 {
            er_status.forbidden_cw_bwd = true;
        }

        // Bit counter forward unequal zero.
        if self.length_of_rvlc_scf_fwd != 0 {
            er_status.length_fwd = true;
        }

        // Bit counter backward unequal zero.
        if self.length_of_rvlc_scf_bwd != 0 {
            er_status.length_bwd = true;
        }

        // Bit counter escape sequences unequal zero.
        if self.is_scf_escapes_present && self.length_of_rvlc_escapes != 0 {
            er_status.length_escapes = true;
        }

        if self.is_scf_used {
            // First decoded scf does not match to global gain in backward direction.
            if self.first_scf != (i16::from(global_gain) - rvlc_const::SF_OFFSET) {
                er_status.first_scf = true;
            }

            // Last decoded scf does not match to rev global gain in forward direction.
            if self.last_scf != (self.rev_global_gain - rvlc_const::SF_OFFSET) {
                er_status.last_scf = true;
            }
        }

        if self.is_noise_used {
            // First decoded nrg does not match to dpcm_noise_nrg in backward direction.

            if self.first_nrg
                != (i16::from(global_gain) + self.dpcm_noise_nrg - pns::NOISE_OFFSET - 256)
            {
                er_status.first_nrg = true;
            }

            // Last decoded nrg does not match to dpcm_noise_last_pos in forward direction.
            if self.last_nrg
                != (self.rev_global_gain + self.dpcm_noise_last_pos - pns::NOISE_OFFSET - 256)
            {
                er_status.last_nrg = true;
            }
        }

        if self.is_intensity_stereo_used {
            // First decoded is position does not match in backward direction.
            if self.first_intensity_stereo != -rvlc_const::SF_OFFSET {
                er_status.first_intensity_stereo = true;
            }

            // Last decoded is position does not match in forward direction.
            if self.last_intensity_stereo
                != (self.dpcm_intensity_stereo_last_pos - rvlc_const::SF_OFFSET)
            {
                er_status.last_intensity_stereo = true;
            }
        }

        // Decoded escapes and used escapes in forward direction do not fit.
        if self.num_decoded_escape_words_fwd != self.num_decoded_escape_words_esc
            && self.conceal_max == rvlc_const::CONCEAL_MAX_INIT
        {
            er_status.num_escapes_fwd = true;
        }

        // Decoded escapes and used escapes in backward direction do not fit.
        if self.num_decoded_escape_words_bwd != self.num_decoded_escape_words_esc
            && self.conceal_min == rvlc_const::CONCEAL_MIN_INIT
        {
            er_status.num_escapes_bwd = true;
        }

        if er_status.length_escapes
            || ((er_status.num_escapes_fwd
                && (er_status.last_scf || er_status.last_nrg || er_status.last_intensity_stereo))
                && (er_status.num_escapes_bwd
                    && (er_status.first_scf
                        || er_status.first_nrg
                        || er_status.first_intensity_stereo)))
            || ((self.conceal_max == rvlc_const::CONCEAL_MAX_INIT)
                && ((self.rev_global_gain - rvlc_const::SF_OFFSET - self.last_scf) < -15))
            || ((self.conceal_min == rvlc_const::CONCEAL_MIN_INIT)
                && ((i32::from(global_gain)
                    - i32::from(rvlc_const::SF_OFFSET)
                    - i32::from(self.first_scf))
                    < -15))
        {
            if (self.conceal_max == rvlc_const::CONCEAL_MAX_INIT)
                || (self.conceal_min == rvlc_const::CONCEAL_MIN_INIT)
            {
                self.conceal_max = 0;
                self.conceal_min = i16::try_from(
                    ics_info.n_window_groups().saturating_sub(1) * constants::MAX_SFB_SHORT
                        + ics_info.max_sf_bands().saturating_sub(1),
                )
                .unwrap();
            } else {
                self.conceal_max = self.conceal_max.min(self.conceal_max_esc);
                self.conceal_min = self.conceal_min.max(self.conceal_min_esc);
            }
        }
    }

    /// Initializes the RVLC info struct with pre-defined values, before decoding each frame.
    fn init(&mut self, ics_info: &IcsInfo, code_book: &[u8]) {
        // RVLC common initialization (init part 2 of 2).
        self.num_decoded_escape_words_esc = 0;
        self.num_decoded_escape_words_fwd = 0;
        self.num_decoded_escape_words_bwd = 0;

        self.is_intensity_stereo_used = self.check_codebook(
            ics_info,
            code_book,
            &[utils::CBTYPE_INTENSITY_HCB, utils::CBTYPE_INTENSITY_HCB2],
        );
        self.error_log_rvlc = 0;

        self.conceal_max = rvlc_const::CONCEAL_MAX_INIT;
        self.conceal_min = rvlc_const::CONCEAL_MIN_INIT;

        self.conceal_max_esc = rvlc_const::CONCEAL_MAX_INIT;
        self.conceal_min_esc = rvlc_const::CONCEAL_MIN_INIT;

        // Init scf arrays (for safety (in case of there are only zero codebooks)).
        self.scf_esc.fill(0);
        self.scf_fwd.fill(0);
        self.scf_bwd.fill(0);

        // First bit within RVL coded block as start address for forward decoding.
        self.bs_index_rvl_fwd = 0;
        // Last bit within RVL coded block as start address for backward decoding.
        self.bs_index_rvl_bwd = self.length_of_rvlc_scf - 1;

        if self.is_scf_escapes_present {
            // Locate internal bitstream ptr at escapes (which is the second part).
            self.bs_index_esc = self.length_of_rvlc_scf;
        }
    }

    /// Returns flag which indicates whether intensity stereo codebook is used in RVLC.
    pub(super) fn is_intensity_stereo_used(&self) -> bool {
        self.is_intensity_stereo_used
    }

    /// Returns flag which indicates whether noise codebook is used in RVLC.
    pub(super) fn is_noise_used(&self) -> bool {
        self.is_noise_used
    }

    /// Creates a new RvlcInfo instance.
    pub(super) fn _new() -> RvlcInfo {
        RvlcInfo::default()
    }

    /// Reads RVLC ESC1 data (side info) from bitstream and update RVLC info struct.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    pub(super) fn read(&mut self, bs: &mut Bitstream, ics_info: &IcsInfo, code_book: &[u8]) {
        // RVLC long specific initialization (init part 1 of 2).
        self.is_noise_used = self.check_codebook(ics_info, code_book, &[utils::CBTYPE_NOISE_HCB]);
        self.dpcm_noise_nrg = 0;
        self.dpcm_noise_last_pos = 0;

        // Default value is used for error detection and concealment.
        self.length_of_rvlc_escapes = -1;

        // Read only error sensitivity class 1 data (ESC 1 - data).
        self.is_scf_concealment_present = bs.read_bit() != 0; // #1
        self.rev_global_gain = bs.read(8) as i16; // #2

        self.length_of_rvlc_scf = if !ics_info.is_long_block() {
            bs.read(11) // #3
        } else {
            bs.read(9) // #3
        } as i16;

        if self.is_noise_used {
            self.dpcm_noise_nrg = bs.read(9) as i16; // #4  PNS
        }

        self.is_scf_escapes_present = bs.read_bit() != 0; // #5

        if self.is_scf_escapes_present {
            self.length_of_rvlc_escapes = bs.read(8) as i16; // #6
        }

        if self.is_noise_used {
            self.dpcm_noise_last_pos = bs.read(9) as i16; // #7  PNS
            self.length_of_rvlc_scf -= 9;
        }

        self.length_of_rvlc_scf_fwd = self.length_of_rvlc_scf;
        self.length_of_rvlc_scf_bwd = self.length_of_rvlc_scf;
    }

    /// This approach, by means of bidirectional estimation is generally performed
    /// when a single bit error has been detected, the bit error can be isolated between
    /// `conceal_min` and `conceal_max` and the `is_scf_concealment_present` flag is not set.
    /// The sets of scalefactors, decoded in forward and backward direction,
    /// are compared with each other.
    /// The smaller scalefactor will be considered as the correct one respectively.
    /// The reconstruction of the scalefactors with this approach achieves good results
    /// in audio quality.
    /// The strategy must be applied to scalefactors, intensity data and noise energy separately.
    ///
    /// output:  concealed scalefactor, noise energy and intensity data, between
    ///          `conceal_min` and `conceal_max`.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `global_gain`: Global gain for RVLC scale factor calculation
    /// - `scale_factors`: Scalefactors for each band in each window
    fn bidirectional_estimation_use_lower_scf_of_current_frame(
        &mut self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        global_gain: u8,
        scale_factors: &mut [i16],
    ) {
        let mut start_band: usize;
        let mut end_band: usize;
        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        if self.conceal_min == self.conceal_max {
            let mut ref_is_fwd = 0;
            let mut ref_nrg_fwd = 0;
            let mut ref_scf_fwd = 0;
            let mut ref_is_bwd = 0;
            let mut ref_nrg_bwd = 0;
            let mut ref_scf_bwd = 0;

            self.calc_refval_fwd(
                ics_info,
                code_book,
                global_gain,
                &mut ref_is_fwd,
                &mut ref_nrg_fwd,
                &mut ref_scf_fwd,
            );
            self.calc_refval_bwd(
                ics_info,
                code_book,
                &mut ref_is_bwd,
                &mut ref_nrg_bwd,
                &mut ref_scf_bwd,
            );

            let bands = usize::try_from(self.conceal_min).unwrap();
            match code_book[bands] {
                utils::CBTYPE_ZERO_HCB => {}
                utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                    scale_factors[bands] = if ref_is_fwd < ref_is_bwd {
                        ref_is_fwd
                    } else {
                        ref_is_bwd
                    };
                }
                utils::CBTYPE_NOISE_HCB => {
                    scale_factors[bands] = if ref_nrg_fwd < ref_nrg_bwd {
                        ref_nrg_fwd
                    } else {
                        ref_nrg_bwd
                    };
                }
                _ => {
                    scale_factors[bands] = if ref_scf_fwd < ref_scf_bwd {
                        ref_scf_fwd
                    } else {
                        ref_scf_bwd
                    };
                }
            }
        } else {
            self.scf_fwd[usize::try_from(self.conceal_max).unwrap()] =
                self.scf_bwd[usize::try_from(self.conceal_max).unwrap()];
            self.scf_bwd[usize::try_from(self.conceal_min).unwrap()] =
                self.scf_fwd[usize::try_from(self.conceal_min).unwrap()];

            // Consider the smaller of the forward and backward decoded value as the correct one.
            start_band = usize::from(self.conceal_first_band);
            end_band = if self.conceal_first_group == self.conceal_last_group {
                usize::from(self.conceal_last_band)
            } else {
                ics_info.max_sf_bands().saturating_sub(1)
            };

            let conceal_group_offset = usize::from(self.conceal_first_group) * bands_per_window;
            for (group, (scf_chunk, scf_fwd_chunk, scf_bwd_chunk)) in izip!(
                scale_factors[conceal_group_offset..].chunks_exact_mut(bands_per_window),
                self.scf_fwd[conceal_group_offset..].chunks_exact(bands_per_window),
                self.scf_bwd[conceal_group_offset..].chunks_exact(bands_per_window)
            )
            .take(usize::from(
                self.conceal_last_group - self.conceal_first_group + 1,
            ))
            .enumerate()
            {
                izip!(
                    scf_chunk[start_band..=end_band].iter_mut(),
                    scf_fwd_chunk[start_band..=end_band].iter(),
                    scf_bwd_chunk[start_band..=end_band].iter()
                )
                .for_each(|(scf, scf_fwd, scf_bwd)| {
                    *scf = if *scf_fwd < *scf_bwd {
                        *scf_fwd
                    } else {
                        *scf_bwd
                    };
                });

                start_band = 0;
                if (group + 1) == usize::from(self.conceal_last_group - self.conceal_first_group) {
                    end_band = usize::from(self.conceal_last_band);
                }
            }
        }

        // Copy all data which doesn't need to be concealed to the output buffer.
        end_band = if self.conceal_first_group == 0 {
            usize::from(self.conceal_first_band)
        } else {
            ics_info.max_sf_bands()
        };

        for (group, (scf_chunk, scf_fwd_chunk)) in izip!(
            scale_factors.chunks_exact_mut(bands_per_window),
            self.scf_fwd.chunks_exact(bands_per_window),
        )
        .take(usize::from(self.conceal_first_group + 1))
        .enumerate()
        {
            izip!(
                scf_chunk[..end_band].iter_mut(),
                scf_fwd_chunk[..end_band].iter(),
            )
            .for_each(|(scf, scf_fwd)| *scf = *scf_fwd);

            if (group + 1) == usize::from(self.conceal_first_group) {
                end_band = usize::from(self.conceal_first_band);
            }
        }

        start_band = usize::from(self.conceal_last_band + 1);

        for (scf_chunk, scf_bwd_chunk) in izip!(
            scale_factors[usize::from(self.conceal_last_group) * bands_per_window..]
                .chunks_exact_mut(bands_per_window),
            self.scf_bwd[usize::from(self.conceal_last_group) * bands_per_window..]
                .chunks_exact(bands_per_window),
        )
        .take(ics_info.n_window_groups() - usize::from(self.conceal_last_group))
        {
            if start_band < ics_info.max_sf_bands() {
                izip!(
                    scf_chunk[start_band..ics_info.max_sf_bands()].iter_mut(),
                    scf_bwd_chunk[start_band..ics_info.max_sf_bands()].iter(),
                )
                .for_each(|(scf, scf_bwd)| *scf = *scf_bwd);
            }
            start_band = 0;
        }
    }

    /// This approach by means of bidirectional estimation is generally
    /// performed when a single bit error has been detected, the bit error can be
    /// isolated between `conceal_min` and `conceal_max`, the `is_scf_concealment_present` flag is
    /// set and the previous frame has the same block type as the current frame.
    /// The scalefactor, decoded in forward and backward direction,
    /// and the scalefactor of the previous frame are compared with each other.
    /// The smaller scalefactor will be considered as the correct one.
    /// The codebook of the previous and current frame must be of the same set (scf, nrg, is)
    /// in each scalefactorband.
    /// Otherwise the scalefactor of the prev. frame is not considered in the minimum calculation.
    /// The reconstruction of the scalefactors with this approach achieves
    /// good results in audio quality.
    /// The strategy must be applied to scalefactors, intensity data and noise energy separately.
    ///
    /// output: concealed scalefactor, noise energy and intensity data, between
    ///         `conceal_min` and `conceal_max`.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `previous_codebooks`: Previous codebook from RVLC concealment info
    /// - `previous_scale_factors`: Previous scale factors from RVLC concealment info
    /// - `scale_factors`: Scalefactors for each band in each window
    fn bidirectional_estimation_use_scf_of_prev_frame_as_reference(
        &mut self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        previous_codebooks: &[u8],
        previous_scale_factors: &[i16],
        scale_factors: &mut [i16],
    ) {
        let mut common_min;

        self.scf_fwd[usize::try_from(self.conceal_max).unwrap()] =
            self.scf_bwd[usize::try_from(self.conceal_max).unwrap()];
        self.scf_bwd[usize::try_from(self.conceal_min).unwrap()] =
            self.scf_fwd[usize::try_from(self.conceal_min).unwrap()];

        // Consider the smaller of the forward and backward decoded value as the correct one.
        let mut end_band;
        let mut start_band = usize::from(self.conceal_first_band);

        if self.conceal_first_group == self.conceal_last_group {
            end_band = usize::from(self.conceal_last_band);
        } else {
            end_band = ics_info.max_sf_bands().saturating_sub(1);
        }

        let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
        let group_offset = usize::from(self.conceal_first_group) * bands_per_window;

        for (
            (group, code_book_chunk),
            prev_code_book_chunk,
            scf_chunk,
            prev_scf_chunk,
            scf_bwd_chunk,
            scf_fwd_chunk,
        ) in izip!(
            code_book[group_offset..]
                .chunks_exact(bands_per_window)
                .enumerate(),
            previous_codebooks[group_offset..].chunks_exact(bands_per_window),
            scale_factors[group_offset..].chunks_exact_mut(bands_per_window),
            previous_scale_factors[group_offset..].chunks_exact(bands_per_window),
            self.scf_bwd[group_offset..].chunks_exact(bands_per_window),
            self.scf_fwd[group_offset..].chunks_exact(bands_per_window),
        )
        .take(usize::from(
            self.conceal_last_group - self.conceal_first_group + 1,
        )) {
            for (cb_band, prev_cb_band, scf, prev_scf, scf_bwd, scf_fwd) in izip!(
                code_book_chunk[start_band..].iter(),
                prev_code_book_chunk[start_band..].iter(),
                scf_chunk[start_band..].iter_mut(),
                prev_scf_chunk[start_band..].iter(),
                scf_bwd_chunk[start_band..].iter(),
                scf_fwd_chunk[start_band..].iter()
            )
            .take(end_band - start_band + 1)
            {
                match *cb_band {
                    utils::CBTYPE_ZERO_HCB => {
                        *scf = 0;
                    }
                    utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                        if *prev_cb_band == utils::CBTYPE_INTENSITY_HCB
                            || *prev_cb_band == utils::CBTYPE_INTENSITY_HCB2
                        {
                            common_min = *scf_fwd.min(scf_bwd);
                            *scf = *prev_scf.min(&common_min);
                        } else {
                            *scf = *scf_fwd.min(scf_bwd);
                        }
                    }
                    utils::CBTYPE_NOISE_HCB => {
                        if *prev_cb_band == utils::CBTYPE_NOISE_HCB {
                            common_min = *scf_fwd.min(scf_bwd);
                            *scf = *prev_scf.min(&common_min);
                        } else {
                            *scf = *scf_fwd.min(scf_bwd);
                        }
                    }
                    _ => {
                        if *prev_cb_band != utils::CBTYPE_ZERO_HCB
                            && *prev_cb_band != utils::CBTYPE_NOISE_HCB
                            && *prev_cb_band != utils::CBTYPE_INTENSITY_HCB
                            && *prev_cb_band != utils::CBTYPE_INTENSITY_HCB2
                        {
                            common_min = *scf_fwd.min(scf_bwd);
                            *scf = *prev_scf.min(&common_min);
                        } else {
                            *scf = *scf_fwd.min(scf_bwd);
                        }
                    }
                }
            }
            start_band = 0;
            if (group + 1) == usize::from(self.conceal_last_group - self.conceal_first_group) {
                end_band = usize::from(self.conceal_last_band);
            }
        }

        // Copy all data which doesn't need to be concealed to the output buffer.
        if self.conceal_first_group == 0 {
            end_band = usize::from(self.conceal_first_band);
        } else {
            end_band = ics_info.max_sf_bands();
        }

        for ((group, scf_chunk), scf_fw_chunk) in izip!(
            scale_factors.chunks_exact_mut(bands_per_window).enumerate(),
            self.scf_fwd.chunks_exact(bands_per_window)
        )
        .take(usize::from(self.conceal_first_group + 1))
        {
            for (scf, scf_fw) in izip!(scf_chunk.iter_mut(), scf_fw_chunk.iter()).take(end_band) {
                *scf = *scf_fw;
            }
            if (group + 1) == usize::from(self.conceal_first_group) {
                end_band = usize::from(self.conceal_first_band);
            }
        }

        start_band = usize::from(self.conceal_last_band) + 1;
        let group_offset = usize::from(self.conceal_last_group) * bands_per_window;
        for (scf_chunk, scf_bw_chunk) in izip!(
            scale_factors[group_offset..].chunks_exact_mut(bands_per_window),
            self.scf_bwd[group_offset..].chunks_exact(bands_per_window)
        )
        .take(ics_info.n_window_groups() - usize::from(self.conceal_last_group))
        {
            if ics_info.max_sf_bands() > start_band {
                for (scf, scf_bw) in izip!(
                    scf_chunk[start_band..].iter_mut(),
                    scf_bw_chunk[start_band..].iter()
                )
                .take(ics_info.max_sf_bands() - start_band)
                {
                    *scf = *scf_bw;
                }
            }
            start_band = 0;
        }
    }

    /// This approach, by means of predictive estimation, is generally performed
    /// when the error cannot be isolated between `conceal_min` and `conceal_max`,
    /// the `is_scf_concealment_present` flag is set and the previous frame has
    /// the same block type as the current frame.
    /// Check for each scalefactorband,
    /// if the same type of data (scalefactor, internsity data, noise energies) is transmitted.
    /// If so, use the scalefactor (intensity data, noise energy) in the current frame.
    /// Otherwise, set the scalefactor (intensity data, noise energy) for this scalefactorband to 0.
    ///
    /// output:    concealed scalefactor, noise energy and intensity data
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `previous_codebooks`: Previous codebook from RVLC concealment info
    /// - `previous_scale_factors`: Previous scalefactors from RVLC concealment info
    /// - `scale_factors`: Scalefactors for each band in each window
    fn predictive_interpolation(
        &self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        previous_codebooks: &[u8],
        previous_scale_factors: &[i16],
        scale_factors: &mut [i16],
    ) {
        if ics_info.max_sf_bands() != 0 {
            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
            for (
                cb_chunk,
                prev_cb_chunk,
                scf_chunk,
                prev_scf_chunk,
                scf_fwd_chunk,
                scf_bwd_chunk,
            ) in izip!(
                code_book.chunks_exact(bands_per_window),
                previous_codebooks.chunks_exact(bands_per_window),
                scale_factors.chunks_exact_mut(bands_per_window),
                previous_scale_factors.chunks_exact(bands_per_window),
                self.scf_fwd.chunks_exact(bands_per_window),
                self.scf_bwd.chunks_exact(bands_per_window),
            )
            .take(ics_info.n_window_groups())
            {
                for (cb_band, prev_cb_band, scf, prev_scf, scf_fwd, scf_bwd) in izip!(
                    cb_chunk.iter(),
                    prev_cb_chunk.iter(),
                    scf_chunk.iter_mut(),
                    prev_scf_chunk.iter(),
                    scf_fwd_chunk.iter(),
                    scf_bwd_chunk.iter()
                )
                .take(ics_info.max_sf_bands())
                {
                    *scf = match *cb_band {
                        utils::CBTYPE_ZERO_HCB => 0,
                        utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                            if *prev_cb_band == utils::CBTYPE_INTENSITY_HCB
                                || *prev_cb_band == utils::CBTYPE_INTENSITY_HCB2
                            {
                                *scf_fwd.min(scf_bwd).min(prev_scf)
                            } else {
                                -110
                            }
                        }
                        utils::CBTYPE_NOISE_HCB => {
                            if *prev_cb_band == utils::CBTYPE_NOISE_HCB {
                                *scf_fwd.min(scf_bwd).min(prev_scf)
                            } else {
                                -110
                            }
                        }
                        _ => {
                            if *prev_cb_band != utils::CBTYPE_ZERO_HCB
                                && *prev_cb_band != utils::CBTYPE_NOISE_HCB
                                && *prev_cb_band != utils::CBTYPE_INTENSITY_HCB
                                && *prev_cb_band != utils::CBTYPE_INTENSITY_HCB2
                            {
                                *scf_fwd.min(scf_bwd).min(prev_scf)
                            } else {
                                0
                            }
                        }
                    };
                }
            }
        }
    }

    /// This approach, by means of statistical estimation, is generally performed
    /// when both the start value and the end value are different and
    /// no further errors have been detected.
    /// Considering the forward and backward decoded scalefactors,
    /// the set with the lower scalefactors in sum will be considered as the correct one.
    /// The scalefactors are differentially encoded.
    /// Normally, it would be enough to compare one pair of the forward and backward decoded
    /// scalefactors to specify the lower set. But having detected no further errors does not
    /// necessarily mean the absence of errors.
    /// Therefore, all scalefactors decoded in forward and backward direction are
    /// summed up separately.
    /// The set with the lower sum will be used.
    /// The strategy must be applied to scalefactors, intensity data and noise energy separately.
    ///
    ///  output:    concealed scalefactor, noise energy and intensity data
    ///
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `code_book`: Codebook for each window and scale factor band
    /// - `scale_factors`: Scalefactors for each band in each window
    fn statistical_estimation(
        &self,
        ics_info: &IcsInfo,
        code_book: &[u8],
        scale_factors: &mut [i16],
    ) {
        // Sum of intensity data forward/backward.
        let (mut sum_is_fwd, mut sum_is_bwd) = (0, 0);
        // Sum of noise energy data forward/backward.
        let (mut sum_nrg_fwd, mut sum_nrg_bwd) = (0, 0);
        // Sum of scalefactor data forward/backward.
        let (mut sum_scf_fwd, mut sum_scf_bwd) = (0, 0);

        // Calculate sum of each group (scf,nrg,is) of forward and backward direction.
        if ics_info.max_sf_bands() != 0 {
            let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();
            for (cb_chunk, scf_fwd_chunk, scf_bwd_chunk) in izip!(
                code_book.chunks_exact(bands_per_window),
                self.scf_fwd.chunks_exact(bands_per_window),
                self.scf_bwd.chunks_exact(bands_per_window)
            )
            .take(ics_info.n_window_groups())
            {
                for (cb_band, scf_fwd, scf_bwd) in
                    izip!(cb_chunk.iter(), scf_fwd_chunk.iter(), scf_bwd_chunk.iter())
                        .take(ics_info.max_sf_bands())
                {
                    match *cb_band {
                        utils::CBTYPE_ZERO_HCB => {}
                        utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                            sum_is_fwd += *scf_fwd;
                            sum_is_bwd += *scf_bwd;
                        }
                        utils::CBTYPE_NOISE_HCB => {
                            sum_nrg_fwd += *scf_fwd;
                            sum_nrg_bwd += *scf_bwd;
                        }
                        _ => {
                            sum_scf_fwd += *scf_fwd;
                            sum_scf_bwd += *scf_bwd;
                        }
                    }
                }
            }

            // Find for each group (scf,nrg,is) the correct direction.
            // The flags signals the elements which are used for the final result.
            let use_is_fwd = sum_is_fwd < sum_is_bwd;
            let use_nrg_fwd = sum_nrg_fwd < sum_nrg_bwd;
            let use_scf_fwd = sum_scf_fwd < sum_scf_bwd;

            // Conceal each group (scf,nrg,is).
            for (cb_chunk, scf_chunk, scf_fwd_chunk, scf_bwd_chunk) in izip!(
                code_book.chunks_exact(bands_per_window),
                scale_factors.chunks_exact_mut(bands_per_window),
                self.scf_fwd.chunks_exact(bands_per_window),
                self.scf_bwd.chunks_exact(bands_per_window)
            )
            .take(ics_info.n_window_groups())
            {
                for (cb_band, scf, scf_fwd, scf_bwd) in izip!(
                    cb_chunk.iter(),
                    scf_chunk.iter_mut(),
                    scf_fwd_chunk.iter(),
                    scf_bwd_chunk.iter()
                )
                .take(ics_info.max_sf_bands())
                {
                    *scf = match *cb_band {
                        utils::CBTYPE_ZERO_HCB => *scf, // Assign same value, ideally do nothing.
                        utils::CBTYPE_INTENSITY_HCB | utils::CBTYPE_INTENSITY_HCB2 => {
                            if use_is_fwd {
                                *scf_fwd
                            } else {
                                *scf_bwd
                            }
                        }
                        utils::CBTYPE_NOISE_HCB => {
                            if use_nrg_fwd {
                                *scf_fwd
                            } else {
                                *scf_bwd
                            }
                        }
                        _ => {
                            if use_scf_fwd {
                                *scf_fwd
                            } else {
                                *scf_bwd
                            }
                        }
                    };
                }
            }
        }
    }
}

impl RvlcErrorStatus {
    fn is_error_status_complete(&self) -> bool {
        self.last_scf
            || self.first_scf
            || self.last_nrg
            || self.first_nrg
            || self.last_intensity_stereo
            || self.first_intensity_stereo
            || self.forbidden_cw_fwd
            || self.forbidden_cw_bwd
            || self.length_fwd
            || self.length_bwd
            || self.length_escapes
            || self.num_escapes_fwd
            || self.num_escapes_bwd
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Test whether an initialized RvlcInfo structure will pass
    // the corresponding `Default` values to the variables:
    // let (mut ref_is, mut ref_nrg, mut ref_scf) = (random, random, random);
    #[test]
    fn t_calc_refval_bwd() {
        let rvlc_info: RvlcInfo = Default::default();
        // --------------- Create ICSInfo ---------------
        let mut ics_info = IcsInfo::new();
        // init with hardcoded ics_info data.
        ics_info_unit_test_demo::init(&mut ics_info);

        // -- Various codebooks (will match every branch of the loop). --
        let mut cbook: [u8; constants::MAX_GROUPS_X_SFBS] =
            [utils::CBTYPE_ZERO_HCB; constants::MAX_GROUPS_X_SFBS];
        cbook[0..constants::MAX_SFB_SHORT].fill(utils::CBTYPE_INTENSITY_HCB);
        cbook[constants::MAX_SFB_SHORT..(2 * constants::MAX_SFB_SHORT)]
            .fill(utils::CBTYPE_NOISE_HCB);
        cbook[(2 * constants::MAX_SFB_SHORT)..(3 * constants::MAX_SFB_SHORT)]
            .fill(utils::CBTYPE_ESCBOOK);

        // -- in/out --
        let (mut ref_is, mut ref_nrg, mut ref_scf) = (-379, 4200, 501); // Random values.

        // -- DUT ----
        rvlc_info.calc_refval_bwd(&ics_info, &cbook, &mut ref_is, &mut ref_nrg, &mut ref_scf);

        // Test condition: values must be passed from RvlcInfo::scf_fwd[].
        assert_eq!((ref_is, ref_nrg, ref_scf), (0, 0, 0));
    }

    // Test whether an initialized RvlcInfo structure will pass
    // the corresponding `Default` values to the variables:
    // let (mut ref_is, mut ref_nrg, mut ref_scf) = (random, random, random);
    #[test]
    fn t_calc_ref_val_fwd() {
        let rvlc_info: RvlcInfo = RvlcInfo {
            conceal_first_group: 2,
            conceal_first_band: 1,
            // scf_fwd: 0 initialized.
            ..Default::default()
        };

        // --------------- Create ICSInfo ---------------
        let mut ics_info = IcsInfo::new();
        // init with hardcoded ics_info data.
        ics_info_unit_test_demo::init(&mut ics_info);

        // -- Various codebooks (will match every branch of the loop). --
        let mut cbook: [u8; constants::MAX_GROUPS_X_SFBS] =
            [utils::CBTYPE_ZERO_HCB; constants::MAX_GROUPS_X_SFBS];
        cbook[0..constants::MAX_SFB_SHORT].fill(utils::CBTYPE_INTENSITY_HCB);
        cbook[(constants::MAX_SFB_SHORT)..(2 * constants::MAX_SFB_SHORT)]
            .fill(utils::CBTYPE_NOISE_HCB);
        cbook[(2 * constants::MAX_SFB_SHORT)..(3 * constants::MAX_SFB_SHORT)]
            .fill(utils::CBTYPE_ESCBOOK);

        // -- in/out --
        let (mut ref_is, mut ref_nrg, mut ref_scf) = (-379, 4200, 501); // Random values

        rvlc_info.calc_refval_fwd(
            &ics_info,
            &cbook,
            0,
            &mut ref_is,
            &mut ref_nrg,
            &mut ref_scf,
        );

        // Test condition: values must be passed from RvlcInfo::scf_fwd[].
        assert_eq!((ref_is, ref_nrg, ref_scf), (0, 0, 0));
    }
}

#[cfg(test)]
// This module acts as a helper, for initializing an instance of IcsInfo with particular data.
mod ics_info_unit_test_demo {
    use crate::{
        aac_dec::{channel_info::IcsInfo, sr_info::SamplingRateInfo},
        common::{
            bitstream::{Bitstream, Mode},
            flags::ACFlags,
        },
    };

    // This function acts as a helper, for initializing an instance of IcsInfo with particular data.
    // IcsInfo instances are used in multiple places, which makes their initialization quite
    // tideous.
    pub fn init(ics_info: &mut IcsInfo) {
        const BS_BUFF_LEN: usize = 8;
        const NUM_SAMPLES: u16 = 1024;

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(NUM_SAMPLES, 3, 48000);

        // Byte blob meaning:
        // 0b0      : reserved
        // 0b10     : BlockType::Short
        // 0b0      : SineWindow
        // 0b1100   : max_sf_bands = 12
        // 0b1101101: scale factor grouping: 3|3|2
        let mut bitstream_writer = Bitstream::new(BS_BUFF_LEN, Mode::Writer);
        bitstream_writer.write(0b010011001101101, 15);
        bitstream_writer.sync();

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 15);

        // let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());
    }
}
