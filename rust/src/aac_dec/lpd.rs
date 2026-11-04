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
//! USAC linear predictive domain coding (LPD)

pub mod acelp;
pub mod bass_postfilter;
pub mod common;
pub mod constants;
pub mod fac;
pub mod lpc;
mod lpc_tables;
pub mod tcx;

use crate::aac_dec::channel_info::IcsInfo;
use crate::aac_dec::error_codes::AacDecoderError;
use crate::aac_dec::lpd::acelp::*;
use crate::aac_dec::lpd::common::LpdMode;
use crate::aac_dec::lpd::constants::*;
use crate::aac_dec::lpd::fac::FacData;
use crate::aac_dec::lpd::lpc::*;
use crate::aac_dec::lpd::lpc_tables::LSF_INIT;
use crate::aac_dec::lpd::tcx::*;
use crate::aac_dec::utils::get_gain;
use crate::arith_coding::arith_dec::ArithDecoderData;
use crate::common::bitstream::Bitstream;
use crate::common::enums::WindowShape;
use crate::common::flags::ACFlags;
use crate::common::mdct::Mdct;
use crate::common::tables::window_tables;

/// Defines the LPD mode.
#[repr(C)]
#[derive(PartialEq, Eq, Clone, Copy, Debug, Default)]
pub enum UsacCoremode {
    #[default]
    FdLong = 0,
    FdShort = 1,
    Lpd = 2,
}

#[rustfmt::skip]
static LPD_MODES_MAP : [[LpdMode; 4]; 26] = [
    [LpdMode::Acelp, LpdMode::Acelp, LpdMode::Acelp, LpdMode::Acelp], // lpd_modes_bs = 0
    [LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Acelp, LpdMode::Acelp], // lpd_modes_bs = 1
    [LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Acelp], // lpd_modes_bs = 2
    [LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Acelp], // lpd_modes_bs = 3
    [LpdMode::Acelp, LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Acelp], // lpd_modes_bs = 4
    [LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Acelp], // lpd_modes_bs = 5
    [LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Acelp], // lpd_modes_bs = 6
    [LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Acelp], // lpd_modes_bs = 7
    [LpdMode::Acelp, LpdMode::Acelp, LpdMode::Acelp, LpdMode::Tcx20], // lpd_modes_bs = 8
    [LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Acelp, LpdMode::Tcx20], // lpd_modes_bs = 9
    [LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Tcx20], // lpd_modes_bs = 10
    [LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Tcx20], // lpd_modes_bs = 11
    [LpdMode::Acelp, LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Tcx20], // lpd_modes_bs = 12
    [LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Tcx20], // lpd_modes_bs = 13
    [LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Tcx20], // lpd_modes_bs = 14
    [LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Tcx20], // lpd_modes_bs = 15
    [LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Acelp, LpdMode::Acelp], // lpd_modes_bs = 16
    [LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Tcx20, LpdMode::Acelp], // lpd_modes_bs = 17
    [LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Acelp, LpdMode::Tcx20], // lpd_modes_bs = 18
    [LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Tcx20, LpdMode::Tcx20], // lpd_modes_bs = 19
    [LpdMode::Acelp, LpdMode::Acelp, LpdMode::Tcx40, LpdMode::Tcx40], // lpd_modes_bs = 20
    [LpdMode::Tcx20, LpdMode::Acelp, LpdMode::Tcx40, LpdMode::Tcx40], // lpd_modes_bs = 21
    [LpdMode::Acelp, LpdMode::Tcx20, LpdMode::Tcx40, LpdMode::Tcx40], // lpd_modes_bs = 22
    [LpdMode::Tcx20, LpdMode::Tcx20, LpdMode::Tcx40, LpdMode::Tcx40], // lpd_modes_bs = 23
    [LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Tcx40, LpdMode::Tcx40], // lpd_modes_bs = 24
    [LpdMode::Tcx80, LpdMode::Tcx80, LpdMode::Tcx80, LpdMode::Tcx80], // lpd_modes_bs = 25
];

#[derive(Default, Debug)]
#[repr(C)]
/// LPD info structure
pub struct LpdInfo {
    /// Current core mode (FD or LPD).
    pub core_mode: u8,
    /// Previous core mode, signalled in the bitstream.
    /// Can be FD long, FD short or LPD.
    /// Not handled by the decoder - see `LpdData::last_core_mode`.
    pub core_mode_last: UsacCoremode,
    /// Previous LPD mode, signalled in the bitstream.
    /// Not handled by the decoder - see `LpdData::last_lpd_mode`.
    pub lpd_mode_last: LpdMode,
    mode: [LpdMode; 4],
    /// BPF enabled for current superframe?
    bpf_control_info: bool,
    /// Linear prediction coefficients in LSP domain.
    pub lsp_coeffs: [[f32; constants::M_LP_FILTER_ORDER]; 5],
    /// Linear prediction coefficients in LP domain.
    pub lp_coeffs: [[f32; constants::M_LP_FILTER_ORDER]; 5],
    /// Copied to `LpdData::lsf_adaptive_mean` once frame is assumed to be correct.
    //// Used for concealment.
    lsf_adaptive_mean_cand: [f32; constants::M_LP_FILTER_ORDER],
    /// LPC coeff stability values required for ACELP and TCX concealment.
    a_stability: [f32; 4],
}

#[repr(C)]
#[derive(Debug)]
/// LPD data structure
pub struct LpdData {
    is_lpd_possible: bool,
    pub old_synth: [f32; constants::PIT_MAX_MAX - constants::BPF_DELAY],
    pub old_t_pf: [u16; constants::SYN_SFD],
    pub old_gain_pf: [f32; constants::SYN_SFD],
    pub mem_bpf: [f32; constants::L_FILT + constants::L_SUBFR],
    pub old_exc_mem: [f32; constants::PIT_MAX_MAX + constants::L_INTERPOL],
    /// Length of FAC and FAC_ZIR data stored in old_exc_mem. Used in FAC and MDCT module.
    pub fac_len: usize,
    /// Offset to FAC and FAC_ZIR data stored in old_exc_mem. Used in FAC and MDCT module.
    pub fac_offset: usize,
    /// BPF enabled for past superframe?
    old_bpf_control_info: bool,
    /// Core mode used by the decoder in previous frame.
    /// Not signalled in the bitstream - see `LpdInfo::core_mode_last`.
    pub last_core_mode: UsacCoremode,
    /// LPD mode used by the decoder in last LPD subframe.
    /// Not signalled in the bitstream - see `LpdInfo::lpd_mode_last`.
    pub last_lpd_mode: LpdMode,
    /// LPD mode used in second last LPD subframe.
    /// Not signalled in the bitstream.
    last_last_lpd_mode: LpdMode,
    /// Flag indicating that the previous LPC was lost.
    pub was_last_lpc_lost: bool,
    was_last_frame_ok: bool,
    was_last_last_frame_ok: bool,
    /// Last LPC4 coefficients in LSF domain.
    pub lpc4_lsf: [f32; constants::M_LP_FILTER_ORDER],
    /// Adaptive mean of LPC coefficients in LSF domain for concealment.
    lsf_adaptive_mean: [f32; constants::M_LP_FILTER_ORDER],
    /// Last LPC coefficients in LP domain.
    /// `lp_coeffs_old[0]` is lpc4, i.e. coeffs for right folding point of last tcx frame.
    /// `lp_coeffs_old[1]` are coeffs for left folding point of last tcx frame.
    lp_coeffs_old: [[f32; constants::M_LP_FILTER_ORDER]; 2],
    /// LPC coeff stability value from last frame (required for TCX concealment).
    old_stability: f32,
    /// Number of consecutive lost subframes.
    num_lost_lpd_frames: i32,
    tcx_data: TcxData,

    /// ACELP data instance.
    pub acelp_data: AcelpData,
    /// FAC data instance.
    pub fac_data: FacData,
    /// LPD info instance.
    pub lpd_info: LpdInfo,
}

impl Default for LpdData {
    fn default() -> Self {
        Self {
            is_lpd_possible: true,
            old_synth: [0.0; constants::PIT_MAX_MAX - constants::BPF_DELAY],
            old_t_pf: Default::default(),
            old_gain_pf: Default::default(),
            mem_bpf: [0.0; constants::L_FILT + constants::L_SUBFR],
            old_exc_mem: [0.0; constants::PIT_MAX_MAX + constants::L_INTERPOL],
            fac_len: Default::default(),
            fac_offset: Default::default(),
            old_bpf_control_info: Default::default(),
            last_core_mode: UsacCoremode::FdLong,
            last_lpd_mode: LpdMode::NotLpd,
            last_last_lpd_mode: Default::default(),
            was_last_lpc_lost: Default::default(),
            was_last_frame_ok: true,
            was_last_last_frame_ok: true,
            lpc4_lsf: Default::default(),
            lsf_adaptive_mean: Default::default(),
            lp_coeffs_old: Default::default(),
            old_stability: Default::default(),
            num_lost_lpd_frames: Default::default(),
            tcx_data: Default::default(),
            acelp_data: Default::default(),
            fac_data: Default::default(),
            lpd_info: Default::default(),
        }
    }
}

impl LpdData {
    /// Creates an `LpdData` instance.
    pub fn new(sampling_rate: u32) -> Self {
        let mut lpd_data: Self = Default::default();
        lpd_data.init(sampling_rate);
        lpd_data
    }

    /// Initializes the `LpdData` instance.
    pub fn init(&mut self, sampling_rate: u32) {
        /// Minimum allowed frequency scale for ACELP decoder.
        const FAC_FSCALE_MIN: u32 = 6000;
        /// Default value from ref soft.
        const LPD_MAX_CORE_SR: u32 = 24000;
        /// Maximum allowed frequency scale for ACELP decoder.
        const FAC_FSCALE_MAX: u32 = LPD_MAX_CORE_SR;

        *self = Default::default();

        self.is_lpd_possible = (FAC_FSCALE_MIN..=FAC_FSCALE_MAX).contains(&sampling_rate);

        self.tcx_data.init();
        self.acelp_data.init(sampling_rate);
    }

    /// Resets the `LpdData` instance.
    pub fn reset(&mut self) {
        if self.last_core_mode != UsacCoremode::Lpd {
            // Reset TCX / ACELP common memory.
            self.old_synth.fill(0.0f32);

            // Initialize the LSFs.
            for (lpc4_lsf_element, lsf_init_element) in
                self.lpc4_lsf.iter_mut().zip(LSF_INIT.iter())
            {
                *lpc4_lsf_element = *lsf_init_element;
            }

            // Reset memory needed by bass post-filter.
            self.mem_bpf.fill(0.0f32);

            self.old_bpf_control_info = false;

            self.old_t_pf.fill(64);
            self.old_gain_pf.fill(0.0f32);

            // Reset ACELP memory.
            self.acelp_data.reset();

            self.was_last_lpc_lost = false;
            self.num_lost_lpd_frames = 0;
            self.tcx_data.reset();
        }
        self.fac_data.reset();
    }

    /// Reads an LPD channel stream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `arco_data`: Arithmetic decoder instance.
    /// - `spectral_data`: Spectral data to read from bitstream.
    /// - `scratch_buffer`: Scratch buffer used for decoding spectral data. Must be >= spec.len().
    /// - `flags`: Audio coding flags (see common/flags.rs).
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        arco_data: &mut ArithDecoderData,
        spectral_data: &mut [f32],
        scratch_buffer: &mut [i16],
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        let aac_dec_error: AacDecoderError = 'block: {
            let mode = &mut self.lpd_info.mode;
            let frame_length = spectral_data.len();
            let mut last_lpc_lost = self.was_last_lpc_lost;

            if !self.is_lpd_possible {
                break 'block AacDecoderError::ParseError;
            }

            let acelp_core_mode = bs.read(3) as u8;

            // lpd_mode
            let error = Self::lpd_mode_to_mod_array(mode, bs.read(5) as u8);
            if error != AacDecoderError::Ok {
                break 'block error;
            }

            // bpf_control_info
            self.lpd_info.bpf_control_info = bs.read_bit() == 1;
            // last_core_mode
            let prev_frame_was_lpd = bs.read(1) != 0;
            // FAC data present?
            let is_fac_data_present = bs.read(1) != 0;

            // Set valid values from `self.{last_core_mode,last_lpd_mode}`.
            self.lpd_info.core_mode_last = self.last_core_mode;
            self.lpd_info.lpd_mode_last = self.last_lpd_mode;
            let mut lpd_mode_last = self.last_lpd_mode;

            if !prev_frame_was_lpd {
                // Last frame was FD.
                self.lpd_info.core_mode_last = UsacCoremode::FdLong;
                self.lpd_info.lpd_mode_last = LpdMode::NotLpd;
            } else {
                // Last frame was LPD.
                self.lpd_info.core_mode_last = UsacCoremode::Lpd;
                if (mode[0].is_acelp() && is_fac_data_present)
                    || (!mode[0].is_acelp() && !is_fac_data_present)
                {
                    // Current LPD mode is ACELP, FAC data present -> TCX.
                    // Current LPD mode is TCX, no FAC data -> TCX.
                    if lpd_mode_last.is_acelp() {
                        // Bit stream interruption detected. Assume last TCX mode as TCX20.
                        self.lpd_info.lpd_mode_last = LpdMode::Tcx20;
                    }
                    // Else assume that remembered TCX mode is correct.
                } else {
                    self.lpd_info.lpd_mode_last = LpdMode::Acelp;
                }
            }

            // `is_first_lpd_flag` depends on bitstream configuration.
            let is_first_lpd_flag = self.lpd_info.core_mode_last != UsacCoremode::Lpd;
            let mut first_tcx_flag = true;

            if (self.last_core_mode != UsacCoremode::Lpd) && !self.was_last_frame_ok {
                // If last rendered frame was not LPD and first lpd flag was not set,
                // this must be an error - set last_lpc_lost flag.
                if !is_first_lpd_flag && !last_lpc_lost {
                    last_lpc_lost = true;
                }
            }

            let mut core_mode_last = self.lpd_info.core_mode_last;
            lpd_mode_last = self.lpd_info.lpd_mode_last;

            let nb_div = constants::NB_DIV;

            // k is the frame index. If a frame is of size 40MS or 80MS,
            // this frame index is incremented 2 or 4 instead of 1 respectively.
            let mut k = 0;
            let mut subframe_pos: usize = 0;
            while k < nb_div {
                if (k == 0 && core_mode_last == UsacCoremode::Lpd && is_fac_data_present)
                    || (lpd_mode_last == LpdMode::Acelp && !mode[k].is_acelp())
                    || ((lpd_mode_last != LpdMode::NotLpd)
                        && !lpd_mode_last.is_acelp()
                        && mode[k].is_acelp())
                {
                    // FAC for (ACELP -> TCX) or (TCX -> ACELP).
                    let error = self
                        .fac_data
                        .read(bs, spectral_data, false, Some(mode), false, k);
                    if error != 0 {
                        break 'block AacDecoderError::ParseError;
                    }
                }

                let granule_length: usize = mode[k].subframe_length(frame_length);
                let current_spec = &mut spectral_data[subframe_pos..subframe_pos + granule_length];
                subframe_pos += granule_length;

                debug_assert!(mode[k].is_acelp() || mode[k].is_tcx());

                if mode[k].is_acelp() {
                    let error = self
                        .acelp_data
                        .read(bs, acelp_core_mode, spectral_data.len(), k);
                    if error != AacDecoderError::Ok {
                        break 'block AacDecoderError::ParseError;
                    }

                    lpd_mode_last = LpdMode::Acelp;
                    k += 1;
                } else {
                    // mode != 0  =>  TCX
                    let error = self.tcx_data.read(
                        k,
                        bs,
                        current_spec,
                        arco_data,
                        scratch_buffer,
                        first_tcx_flag,
                        ac_flags,
                    );

                    lpd_mode_last = mode[k];
                    first_tcx_flag = false;
                    k += 1 << (mode[k] as u8 - 1);
                    if error != AacDecoderError::Ok {
                        break 'block AacDecoderError::ParseError;
                    }
                }
            }

            {
                // Read LPC coefficients.
                let err = lpc::read(
                    bs,
                    &mut self.lpd_info.lsp_coeffs,
                    &mut self.lpc4_lsf,
                    &mut self.lpd_info.lsf_adaptive_mean_cand,
                    &mut self.lpd_info.a_stability,
                    mode,
                    is_first_lpd_flag,
                    // If last lpc4 is available from concealment don't extrapolate lpc0 from lpc2.
                    if mode[0].is_tcx() {
                        false
                    } else {
                        last_lpc_lost && self.last_core_mode != UsacCoremode::Lpd
                    },
                    self.was_last_frame_ok,
                );
                if err != 0 {
                    break 'block AacDecoderError::ParseError;
                }
            }

            // Adjust old lsp[] following a bad frame (to avoid overshoot).
            if last_lpc_lost && !self.was_last_frame_ok {
                let mut k_next;
                k = 0;
                while k < nb_div {
                    k_next = k + mode[k].num_subframes();

                    for i in 0..constants::M_LP_FILTER_ORDER {
                        if self.lpd_info.lsp_coeffs[k_next][i] < self.lpd_info.lsp_coeffs[k][i] {
                            self.lpd_info.lsp_coeffs[k][i] = self.lpd_info.lsp_coeffs[k_next][i];
                        }
                    }
                    k = k_next;
                }
            }

            if !self.was_last_frame_ok {
                lsp_to_lpc(
                    &self.lpd_info.lsp_coeffs[0],
                    &mut self.lpd_info.lp_coeffs[0],
                );
            } else if !self.last_lpd_mode.is_acelp() {
                if self.last_lpd_mode == LpdMode::NotLpd {
                    // We need it for TCX decoding or ACELP excitation update.
                    lsp_to_lpc(
                        &self.lpd_info.lsp_coeffs[0],
                        &mut self.lpd_info.lp_coeffs[0],
                    );
                } else {
                    // last_lpd_mode was TCX.
                    // Copy old LPC4 LP domain coefficients to LPC0 LP domain buffer (to avoid
                    // converting LSP coefficients again).
                    self.lpd_info.lp_coeffs[0].copy_from_slice(&self.lp_coeffs_old[0][..]);
                }
            } // The case "last_lpd_mode was ACELP" is handled by tcx.decode().

            if is_fac_data_present && (core_mode_last != UsacCoremode::Lpd) {
                let prev_frame_was_short = bs.read_bit() != 0;
                if prev_frame_was_short {
                    core_mode_last = UsacCoremode::FdShort;
                    self.lpd_info.core_mode_last = UsacCoremode::FdShort;
                    self.lpd_info.lpd_mode_last = LpdMode::NotLpd;

                    if (self.last_core_mode != UsacCoremode::FdShort) && self.was_last_frame_ok {
                        // `last_core_mode` must be `FdShort` if the window sequence of the
                        // previous frame was short frames.
                        // Otherwise `last_core_mode` would be encoded as FdLong.
                        break 'block AacDecoderError::ParseError;
                    }
                }

                {
                    // FAC for FD -> ACELP
                    let err = self.fac_data.read(
                        bs,
                        spectral_data,
                        core_mode_last == UsacCoremode::FdShort,
                        Some(mode),
                        true,
                        0,
                    );
                    if err != 0 {
                        break 'block AacDecoderError::ParseError;
                    }
                }
            }
            AacDecoderError::Ok
        }; // 'block (used for bail)

        aac_dec_error
    }

    /// Translates lpd_modes as read from bitstream into the lpd_mode array which
    /// describes the mode of each LPD frame.
    ///
    /// See ISO/IEC 23003-3, table 94 — Mapping of coding modes for lpd_channel_stream()
    /// for details.
    ///
    /// # Parameters
    ///
    /// - `lpd_modes`: Array that will be filled with the mode indexes of the inidividual frames.
    /// - `lpd_modes_bs`: Field (5 bits) read from the lpd_channel_stream.
    fn lpd_mode_to_mod_array(lpd_modes: &mut [LpdMode; 4], lpd_modes_bs: u8) -> AacDecoderError {
        if lpd_modes_bs >= LPD_MODES_MAP.len() as u8 {
            return AacDecoderError::ParseError;
        }

        *lpd_modes = LPD_MODES_MAP[lpd_modes_bs as usize];
        AacDecoderError::Ok
    }

    /// Decodes one LPD channel stream.
    ///
    /// # Parameters
    ///
    /// - `spectral_data`: Spectral data read from bitstream in `read()`.
    /// - `nf_random_seed`: Random generation seed.
    pub fn decode(&mut self, spectral_data: &mut [f32], nf_random_seed: &mut u32) {
        let spectral_data_len = spectral_data.len();

        // k is the frame index. If a frame is of size 40MS or 80MS,
        // this frame index is incremented 2 or 4 instead of 1 respectively.
        let mut k = 0;

        // Could be different to what has been rendered:
        let mut last_lpd_mode = self.lpd_info.lpd_mode_last;

        let mut subframe_pos: usize = 0;
        while k < NB_DIV {
            let granule_length: usize = self.lpd_info.mode[k].subframe_length(spectral_data_len);
            let current_spec = &mut spectral_data[subframe_pos..subframe_pos + granule_length];
            subframe_pos += granule_length;
            if self.lpd_info.mode[k].is_acelp() {
                // If previous frame was TCX, apply TCX gains to FAC data.
                if last_lpd_mode.is_tcx() {
                    self.fac_data.apply_gains(
                        spectral_data,
                        self.tcx_data.last_gain,
                        &self.tcx_data.last_alfd_gains,
                        last_lpd_mode,
                        k,
                    );
                }
            } else {
                // TCX
                self.tcx_data.decode(
                    current_spec,
                    &self.lpd_info.lsp_coeffs,
                    &mut self.lpd_info.lp_coeffs,
                    nf_random_seed,
                    last_lpd_mode,
                    spectral_data_len / 16,
                    k + self.lpd_info.mode[k].num_subframes(),
                    k,
                );

                // Store TCX gain scale for next possible FAC transition.
                self.tcx_data.last_gain = self.tcx_data.tcx_info[k].gain;

                // Apply gains.
                if last_lpd_mode.is_acelp() {
                    self.fac_data.apply_gains(
                        spectral_data,
                        self.tcx_data.tcx_info[k].gain,
                        &self.tcx_data.last_alfd_gains,
                        self.lpd_info.mode[k],
                        k,
                    );
                }
            }

            // Remember previous mode.
            last_lpd_mode = self.lpd_info.mode[k];

            // Increase k to next frame.
            k += self.lpd_info.mode[k].num_subframes();
        }
    }

    /// Generates time domain output signal for LPD channel streams.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data.
    /// - `imdct`: MDCT instance.
    /// - `spectral_data`: Spectral input data.
    /// - `synth_buf`: Synthesis buffer used by sub-functions to render time domain signal.
    /// - `time_data`: Time domain output signal.
    /// - `is_frame_ok`: Indicates a valid frame.
    pub fn render(
        &mut self,
        ics_info: &IcsInfo,
        imdct: &mut Mdct,
        spectral_data: &mut [f32],
        synth_buf: &mut [f32],
        time_data: &mut [f32],
        is_frame_ok: bool,
    ) -> AacDecoderError {
        let frame_len = spectral_data.len();
        let mode = &mut self.lpd_info.mode;

        let mut num_samples: usize = 0;

        let nb_div = NB_DIV;
        // Length of division (ACELP or TCX20 frame).
        let l_div = frame_len / nb_div;
        let l_fac = l_div / 2;
        // Number of subframes per division.
        let nb_subfr = frame_len / (nb_div * L_SUBFR);

        let nb_subfr_superfr = nb_div * nb_subfr;
        // Synthesis buffer delay in subframes.
        let syn_sfd = (nb_subfr_superfr / 2) - BPF_SFD;
        // Synthesis buffer delay in samples.
        let syn_delay = syn_sfd * L_SUBFR;
        let aac_delay = frame_len / 2;

        let mut pitch = [0u16; NB_SUBFR_SUPERFR + SYN_SFD];
        let mut pit_gain: [f32; NB_SUBFR_SUPERFR + SYN_SFD] = [0.0; NB_SUBFR_SUPERFR + SYN_SFD];

        let mut last_frame_lost = !self.was_last_last_frame_ok;

        //  Maintain LPD mode from previous frame
        if self.last_core_mode == UsacCoremode::FdLong
            || self.last_core_mode == UsacCoremode::FdShort
        {
            self.last_lpd_mode = LpdMode::NotLpd;
        }

        if !is_frame_ok {
            let last_lpd_mode = self.last_lpd_mode;
            let old_tcx_gain = self.tcx_data.last_gain;
            let old_stab = self.old_stability;

            // Patch the last LPD mode.
            self.lpd_info.lpd_mode_last = last_lpd_mode;

            // Do mode extrapolation and repeat the previous mode:
            //   if previous mode = ACELP        -> ACELP
            //   if previous mode = TCX-20/40    -> TCX-20
            //   if previous mode = TCX-80       -> TCX-80
            //   notes:
            //   - ACELP is not allowed after TCX (no pitch information to reuse)
            //   - TCX-40 is not allowed in the mode repetition to keep the logic simple
            match last_lpd_mode {
                LpdMode::Acelp => mode.fill(LpdMode::Acelp), // -> ACELP concealment
                LpdMode::Tcx80 => mode.fill(LpdMode::Tcx80), // -> TCX FD concealment
                LpdMode::Tcx40 => mode.fill(LpdMode::Tcx40), // -> TCX FD concealment
                _ => mode.fill(LpdMode::TcxTdConceal),       // -> TCX TD concealment
            }

            // LPC extrapolation
            lpc::conceal(
                &mut self.lpd_info.lsp_coeffs,
                &mut self.lpc4_lsf,
                &self.lsf_adaptive_mean,
                last_lpd_mode == LpdMode::NotLpd,
            );

            if last_lpd_mode.is_tcx() || last_lpd_mode == LpdMode::TcxTdConceal {
                // Copy old LPC4 LP domain coefficients to LPC0 LP domain buffer,
                // to avoid converting LSP coefficients again.
                self.lpd_info.lp_coeffs[0][..M_LP_FILTER_ORDER]
                    .copy_from_slice(&self.lp_coeffs_old[0][..M_LP_FILTER_ORDER]);
            } // The case "last_lpd_mode was ACELP" is handled by decode function.

            // The case "last_lpd_mode was time domain TCX concealment" is handled
            // after this "if !is_frame_ok" block.

            // k is the frame index. If a frame is of size 40MS or 80MS, this frame index
            // is incremented 2 or 4 instead of 1 respectively.

            let mut k = 0;
            while k < nb_div {
                self.tcx_data.tcx_info[k].gain = old_tcx_gain;

                // Restore stability value from last frame.
                self.lpd_info.a_stability[k] = old_stab;

                // Increase k to next frame.
                k = k + mode[k].num_subframes();
            }
        } else {
            // Frame OK
            if (self.last_lpd_mode == LpdMode::TcxTdConceal) && (!mode[0].is_acelp()) {
                // Copy old LPC4 LP domain coefficients to LPC0 LP domain buffer,
                // to avoid converting LSP coefficients again.
                self.lpd_info.lp_coeffs[0][..M_LP_FILTER_ORDER]
                    .copy_from_slice(&self.lp_coeffs_old[0][..M_LP_FILTER_ORDER]);
            }
        }

        AcelpData::pre_processing(
            &mut synth_buf[..PIT_MAX_MAX - BPF_DELAY],
            &self.old_synth,
            &mut pitch[..nb_subfr_superfr + syn_sfd],
            &self.old_t_pf[..syn_sfd],
            &mut pit_gain[..nb_subfr_superfr + syn_sfd],
            &self.old_gain_pf[..syn_sfd],
        );

        // k is the frame index. If a frame is of size 40MS or 80MS,
        // this frame index is incremented 2 or 4 instead of 1 respectively.
        let mut k = 0;

        let mut last_lpd_mode = self.last_lpd_mode;
        let mut last_last_lpd_mode = self.last_last_lpd_mode;
        let mut last_lpc_lost = self.was_last_lpc_lost | last_frame_lost;

        let mut last_k = -1; // Mark as invalid.
        let mut subframe_pos: usize = 0;

        // Preprocessing
        while k < NB_DIV {
            let granule_length: usize = mode[k].subframe_length(frame_len);

            // Concealment handling
            if !is_frame_ok {
                self.num_lost_lpd_frames += 1;
            } else {
                last_frame_lost |= self.num_lost_lpd_frames > 0;
                self.num_lost_lpd_frames = 0;
            }

            if mode[k].is_tcx() {
                // TCX

                let current_synth =
                    &mut synth_buf[PIT_MAX_MAX - BPF_DELAY..PIT_MAX_MAX - BPF_DELAY + frame_len];

                // Handle transitions
                if last_lpd_mode.is_time_domain() {
                    // ACELP (or TCX concealment) --> TCX

                    // Get FAC (incl. FAC ZIR, hence the '*2') length.
                    self.fac_len = FacData::fac_len(frame_len, false) * 2;

                    self.fac_offset = 0;
                    let fac_w_zir_opt =
                        { Some(&mut self.old_exc_mem[self.fac_offset..self.fac_len]) };

                    // FAC management
                    let (n_samples_written, n_samples_read_fac) = self.fac_data.acelp2mdct(
                        spectral_data,
                        current_synth,
                        num_samples,
                        fac_w_zir_opt,
                        &self.lpd_info.lp_coeffs[k],
                        imdct,
                        &mut self.acelp_data,
                        window_tables::get_table(l_div as u16, ics_info.window_shape()).unwrap(),
                        self.tcx_data.tcx_info[k].gain,
                        k,
                        granule_length,
                        last_lpd_mode,
                        false,
                        last_frame_lost || !is_frame_ok,
                        false,
                    );

                    num_samples += n_samples_written;

                    self.fac_offset += n_samples_read_fac;
                    if self.fac_offset >= self.fac_len {
                        self.fac_len = 0;
                        self.fac_offset = 0;
                    }

                    let sf_offset = (k * nb_subfr) + syn_sfd;
                    pitch[sf_offset + 1] = pitch[sf_offset - 1];
                    pitch[sf_offset] = pitch[sf_offset - 1];
                    pit_gain[sf_offset + 1] = pit_gain[sf_offset - 1];
                    pit_gain[sf_offset] = pit_gain[sf_offset - 1];
                } else {
                    // TCX/FD --> TCX
                    let fl = l_div as u16;
                    let fr = l_div as u16;

                    let fac_w_zir_opt = if self.fac_len > self.fac_offset {
                        Some(&self.old_exc_mem[self.fac_offset..self.fac_len])
                    } else {
                        None
                    };

                    let current_spec =
                        &mut spectral_data[subframe_pos..subframe_pos + granule_length];
                    let current_synth = &mut synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples
                        ..PIT_MAX_MAX - BPF_DELAY + frame_len];

                    let (n_samples_written, n_elements_read_fac) = imdct.imlt(
                        current_synth,
                        current_spec,
                        1,
                        &fac_w_zir_opt,
                        window_tables::get_table(fl, ics_info.window_shape()).unwrap(),
                        window_tables::get_table(fr, ics_info.window_shape()).unwrap(),
                        self.tcx_data.tcx_info[k].gain,
                    );

                    num_samples += n_samples_written;

                    // FAC buffer must have been drained by now.
                    debug_assert!(self.fac_offset + n_elements_read_fac == self.fac_len);
                    self.fac_len = 0;
                    self.fac_offset = 0;
                }
            } else {
                // ACELP/TCX TD conceal

                let fac_w_zir_opt = if self.fac_len > self.fac_offset {
                    Some(&self.old_exc_mem[self.fac_offset..self.fac_len])
                } else {
                    None
                };

                // Handle transitions
                {
                    if last_lpd_mode.is_time_domain() {
                        // ACELP --> ACELP
                        if k == 0 && imdct.ov_offset != frame_len / 2 {
                            imdct.ov_offset = frame_len / 2;
                        }
                        num_samples += imdct.drain(
                            &mut synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples
                                ..PIT_MAX_MAX - BPF_DELAY + frame_len],
                        );
                    } else {
                        // FD/TCX --> ACELP
                        // FAC management

                        let (n_samples_written, n_samples_read_fac) = self.fac_data.mdct2acelp(
                            spectral_data,
                            &mut synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples
                                ..PIT_MAX_MAX - BPF_DELAY + frame_len],
                            &fac_w_zir_opt,
                            &self.lpd_info.lp_coeffs[k],
                            imdct,
                            WindowShape::Sine,
                            k,
                            if k == 0 {
                                self.last_core_mode == UsacCoremode::FdShort
                            } else {
                                false
                            },
                            last_frame_lost || !is_frame_ok,
                            if k == 0 {
                                self.last_core_mode != UsacCoremode::Lpd
                            } else {
                                false
                            },
                        );

                        num_samples += n_samples_written;
                        self.fac_offset += n_samples_read_fac;
                        synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples
                            ..PIT_MAX_MAX - BPF_DELAY + num_samples + imdct.ov_offset]
                            .copy_from_slice(&imdct.overlap[..imdct.ov_offset]);
                    }

                    if self.fac_offset >= self.fac_len {
                        self.fac_len = 0;
                        self.fac_offset = 0;
                    };
                }

                if !last_lpd_mode.is_time_domain() {
                    // Prepare ACELP memory in case of TCX/FD --> ACELP transition.

                    let current_synth = &mut synth_buf[aac_delay + k * l_div
                        - BPF_DELAY
                        - 1
                        - L_INTERPOL
                        - M_LP_FILTER_ORDER
                        ..aac_delay + k * l_div - BPF_DELAY + PIT_MAX_MAX];

                    let lpd_modes = [mode[k], last_lpd_mode, last_last_lpd_mode];
                    let lp_prev = {
                        last_k = if last_lpd_mode.is_tcx() {
                            k as isize - last_lpd_mode.num_subframes() as isize
                        } else {
                            // Not LPD
                            0
                        };
                        if last_k < 0 {
                            self.lp_coeffs_old[1]
                        } else {
                            self.lpd_info.lp_coeffs[last_k as usize]
                        }
                    };

                    self.acelp_data.prepare_internal_mem(
                        &mut self.old_exc_mem,
                        current_synth,
                        &lpd_modes,
                        &self.lpd_info.lp_coeffs[k],
                        &lp_prev,
                        frame_len,
                        last_frame_lost && k < 2,
                    );
                }

                let acelp_out: &mut [f32];
                if num_samples >= frame_len {
                    // Write ACELP time domain samples into IMDCT overlap buffer.
                    acelp_out =
                        &mut imdct.overlap[imdct.ov_offset..imdct.ov_offset + granule_length];
                    // Account ACELP time domain output samples to overlap buffer.
                    imdct.ov_offset += l_div;
                } else {
                    // Write ACELP time domain samples into output buffer.
                    acelp_out = &mut synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples
                        ..PIT_MAX_MAX - BPF_DELAY + num_samples + granule_length];
                    // Account ACELP time domain output samples to output buffer.
                    num_samples += l_div;
                };

                if mode[k] == LpdMode::TcxTdConceal {
                    // TCX time domain concealment.
                    self.acelp_data.wsyn_rms = self.tcx_data.tcx_info[k].gain;

                    self.acelp_data.tcx_td_conceal(
                        &mut self.old_exc_mem,
                        self.tcx_data.last_pitch,
                        &self.lpd_info.lsp_coeffs[k],
                        &self.lpd_info.lsp_coeffs[k + 1],
                        self.num_lost_lpd_frames,
                        acelp_out,
                        frame_len,
                    );
                } else {
                    debug_assert!(self.lpd_info.a_stability[k] >= 0.0);

                    self.acelp_data.decode(
                        &self.lpd_info.lsp_coeffs[k],
                        &self.lpd_info.lsp_coeffs[k + 1],
                        &mut self.old_exc_mem,
                        self.lpd_info.a_stability[k],
                        self.num_lost_lpd_frames,
                        last_lpc_lost,
                        k,
                        acelp_out,
                        &mut pitch[(k * nb_subfr) + syn_sfd..((k + 1) * nb_subfr) + syn_sfd],
                        &mut pit_gain[(k * nb_subfr) + syn_sfd..((k + 1) * nb_subfr) + syn_sfd],
                    );
                }

                // Non-concealment case
                if mode[k] != LpdMode::TcxTdConceal
                    && !last_lpd_mode.is_acelp()
                    && self.lpd_info.bpf_control_info
                {
                    // FD/TCX -> ACELP transition.
                    // Bass post-filter past FAC area (past two (one for FD short) subframes).
                    let current_sf = syn_sfd + k * nb_subfr;

                    if (k > 0) || (self.last_core_mode != UsacCoremode::FdShort) {
                        // TCX or FD long -> ACELP
                        pitch[current_sf - 2] = pitch[current_sf];
                        pitch[current_sf - 1] = pitch[current_sf];
                        pit_gain[current_sf - 2] = pit_gain[current_sf];
                        pit_gain[current_sf - 1] = pit_gain[current_sf];
                    } else {
                        // FD short -> ACELP
                        pitch[current_sf - 1] = pitch[current_sf];
                        pit_gain[current_sf - 1] = pit_gain[current_sf];
                    }
                }
            }

            last_last_lpd_mode = last_lpd_mode;
            last_lpd_mode = mode[k];
            last_lpc_lost = !is_frame_ok;

            subframe_pos += granule_length;

            // Increase k to next frame.
            last_k = k as isize;
            k += mode[k].num_subframes();
        }
        if is_frame_ok {
            // Assume data was ok => store for concealment.
            debug_assert!(self.lpd_info.a_stability[last_k as usize] >= 0.0);
            self.old_stability = self.lpd_info.a_stability[last_k as usize];

            self.lsf_adaptive_mean
                .copy_from_slice(&self.lpd_info.lsf_adaptive_mean_cand[..M_LP_FILTER_ORDER]);
        }

        // Store past lp coeffs for next superframe.
        // They are only valid and needed if last_lpd_mode was tcx.
        if last_lpd_mode.is_tcx() {
            self.lp_coeffs_old[0]
                .copy_from_slice(&self.lpd_info.lp_coeffs[nb_div][..M_LP_FILTER_ORDER]);
            self.lp_coeffs_old[1]
                .copy_from_slice(&(self.lpd_info.lp_coeffs[last_k as usize][..M_LP_FILTER_ORDER]));
        }

        debug_assert!(num_samples == frame_len);

        // Check whether usage of bass postfilter was deactivated in the bitstream.
        // If yes, set pitch gain to 0.
        if !(self.lpd_info.bpf_control_info) {
            if !mode[0].is_acelp() && self.old_bpf_control_info {
                pit_gain[syn_sfd + 2..syn_sfd + nb_subfr_superfr].fill(0.0);
            } else {
                pit_gain[syn_sfd..syn_sfd + nb_subfr_superfr].fill(0.0);
            }
        }

        // For bass postfilter:
        self.old_t_pf[..syn_sfd]
            .copy_from_slice(&pitch[nb_subfr_superfr..nb_subfr_superfr + syn_sfd]);
        self.old_gain_pf[..syn_sfd]
            .copy_from_slice(&pit_gain[nb_subfr_superfr..nb_subfr_superfr + syn_sfd]);

        self.old_bpf_control_info = self.lpd_info.bpf_control_info;

        {
            let current_synth = &mut synth_buf[PIT_MAX_MAX - BPF_DELAY + num_samples..];
            let mut lookahead = -(constants::BPF_DELAY as isize);
            let mut copy_samp = aac_delay;
            if !mode[nb_div - 1].is_acelp() {
                copy_samp -= l_fac
            };

            // Copy enough time domain samples from MDCT to synthesis buffer as needed
            // by the bass postfilter.
            lookahead += imdct.get_ov_and_nr(&mut current_synth[..copy_samp]) as isize;

            debug_assert!(lookahead == copy_samp as isize - constants::BPF_DELAY as isize);
        }
        {
            // Recalculate pitch gain to allow postfilering on FAC area.
            for i in 0..nb_subfr_superfr {
                let t = pitch[i] as usize;
                let mut gain = pit_gain[i];
                if gain > 0.0 {
                    gain = get_gain(
                        &synth_buf[PIT_MAX_MAX + i * L_SUBFR..PIT_MAX_MAX + (i + 1) * L_SUBFR],
                        &synth_buf
                            [PIT_MAX_MAX + (i * L_SUBFR) - t..PIT_MAX_MAX + (i + 1) * L_SUBFR - t],
                        L_SUBFR,
                    );
                    pit_gain[i] = gain;
                }
            }

            {
                bass_postfilter::filter_1sf_delay(
                    synth_buf,
                    time_data,
                    &pitch,
                    &pit_gain,
                    frame_len,
                    if mode[nb_div - 1].is_tcx() {
                        syn_delay - (l_div / 2)
                    } else {
                        syn_delay
                    },
                    &mut self.mem_bpf,
                );
            }
        }

        AcelpData::post_processing(
            &synth_buf[frame_len..frame_len + PIT_MAX_MAX - BPF_DELAY],
            &mut self.old_synth,
            &pitch[nb_subfr_superfr..nb_subfr_superfr + syn_sfd],
            &mut self.old_t_pf[..syn_sfd],
        );

        // Store last mode for next super frame.
        self.last_core_mode = UsacCoremode::Lpd;
        self.last_lpd_mode = last_lpd_mode;
        self.last_last_lpd_mode = last_last_lpd_mode;
        self.was_last_lpc_lost = last_lpc_lost;

        AacDecoderError::Ok
    }

    pub fn lpc4_lsf(&self) -> &[f32] {
        &self.lpc4_lsf
    }

    pub fn lpc4_lsf_mut(&mut self) -> &mut [f32] {
        &mut self.lpc4_lsf
    }

    pub fn lsp_coeffs_mut(&mut self) -> &mut [[f32; constants::M_LP_FILTER_ORDER]] {
        &mut self.lpd_info.lsp_coeffs
    }

    pub fn lsf_adaptive_mean(&self) -> &[f32] {
        &self.lsf_adaptive_mean
    }

    pub fn was_last_last_frame_ok(&self) -> bool {
        self.was_last_last_frame_ok
    }

    pub fn was_last_frame_ok(&self) -> bool {
        self.was_last_frame_ok
    }

    pub fn set_was_last_last_frame_ok(&mut self, value: bool) {
        self.was_last_last_frame_ok = value;
    }

    pub fn set_was_last_frame_ok(&mut self, value: bool) {
        self.was_last_frame_ok = value;
    }

    pub fn lpd_frame_mode(&self, index: usize) -> LpdMode {
        if index < 4 {
            self.lpd_info.mode[index]
        } else {
            panic!("invalid index for mode buffer")
        }
    }

    pub fn last_tcx_gain(&self) -> f32 {
        self.tcx_data.last_gain()
    }

    pub fn set_last_tcx_gain(&mut self, gain: f32) {
        self.tcx_data.set_last_gain(gain);
    }

    /// LPC extrapolation.
    pub fn lpc_extrapolation(&mut self, first_lpd_flag: bool) {
        lpc::conceal(
            &mut self.lpd_info.lsp_coeffs,
            &mut self.lpc4_lsf,
            &self.lsf_adaptive_mean,
            first_lpd_flag,
        );
    }
}

#[cfg(test)]
mod tests {
    use super::{common::LpdMode, LpdData};
    use crate::aac_dec::{error_codes::AacDecoderError, lpd::constants::*};

    #[test]
    fn lpd_mode_to_mod_array() {
        let mut lpd_modes: [LpdMode; NB_DIV] = Default::default();

        assert!(
            LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b11111) == AacDecoderError::ParseError,
            "Value 31 accepted by lpd_mode_to_mod_array()."
        );

        assert!(
            LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b11010) == AacDecoderError::ParseError,
            "Value 26 accepted by lpd_mode_to_mod_array()."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b00000);
        assert!(
            lpd_modes
                == [
                    LpdMode::Acelp,
                    LpdMode::Acelp,
                    LpdMode::Acelp,
                    LpdMode::Acelp
                ],
            "Value 0b00000 yielded wrong results."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b01111);
        assert!(
            lpd_modes
                == [
                    LpdMode::Tcx20,
                    LpdMode::Tcx20,
                    LpdMode::Tcx20,
                    LpdMode::Tcx20,
                ],
            "Value 0b01111 yielded wrong results."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b10011);
        assert!(
            lpd_modes
                == [
                    LpdMode::Tcx40,
                    LpdMode::Tcx40,
                    LpdMode::Tcx20,
                    LpdMode::Tcx20,
                ],
            "Value 0b10011 yielded wrong results."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b10101);
        assert!(
            lpd_modes
                == [
                    LpdMode::Tcx20,
                    LpdMode::Acelp,
                    LpdMode::Tcx40,
                    LpdMode::Tcx40,
                ],
            "Value 0b10101 yielded wrong results."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b11000);
        assert!(
            lpd_modes
                == [
                    LpdMode::Tcx40,
                    LpdMode::Tcx40,
                    LpdMode::Tcx40,
                    LpdMode::Tcx40,
                ],
            "Value 0b11000 yielded wrong results."
        );

        LpdData::lpd_mode_to_mod_array(&mut lpd_modes, 0b11001);
        assert!(
            lpd_modes
                == [
                    LpdMode::Tcx80,
                    LpdMode::Tcx80,
                    LpdMode::Tcx80,
                    LpdMode::Tcx80,
                ],
            "Value 0b11001 yielded wrong results."
        );
    }
}
