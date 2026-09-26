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
//! Linear predictive coding (LPC) filter used in TCX and ACELP

use super::{common::LpdMode, lpc_tables::*};
use crate::{aac_dec::lpd::constants::*, common::bitstream::Bitstream};
use itertools::{izip, Itertools};

const PI_DOUBLE: f64 = std::f64::consts::PI;
const NC: usize = M_LP_FILTER_ORDER / 2;
const FREQ_MAX: f32 = 6400.0;
const FREQ_DIV: f32 = 400.0;
const LSF_GAP: f32 = 50.0;
const NQ_MAX: u8 = 36;

// Public functions

/// Applies absolute weighting.
///
/// # Parameters
///
/// - `w_a`: Weighted LPC coefficient vector output. The first coeffcient is implicitly 1.0.
/// - `a`: LPC coefficient vector. The first coeffcient is implicitly 1.0.
///
/// # Examples
///
/// ```
/// use aac::aac_dec::lpd::constants::M_LP_FILTER_ORDER;
/// use aac::aac_dec::lpd::lpc::apply_abs_weighting;
///
/// let mut output_buff = [0.0_f32; M_LP_FILTER_ORDER];
/// let input_buff = [0.0_f32; M_LP_FILTER_ORDER];
///
/// apply_abs_weighting(&mut output_buff, &input_buff);
/// ```
pub fn apply_abs_weighting(w_a: &mut [f32], a: &[f32]) {
    let mut f = 0.92_f32;
    w_a.iter_mut()
        .zip(a.iter())
        .for_each(|(w_a_element, a_element)| {
            *w_a_element = *a_element * f;
            f *= 0.92_f32
        });
}

/// Does concealment.
///
/// # Parameters
///
/// - `lsp`: Buffer where the decoded LSP coefficients will be stored.
/// - `lpc4_lsf`: Buffer where the decoded LPC4 LSF coefficients will be stored (persistent).
/// - `lsf_adaptive_mean`: The 7 bit binary word from the bitstream representing the gain.
/// - `first_lpd_flag`: Indicates the previous LSF4 coefficients `lpc4_lsf` are not valid.
///
/// # Examples
///
/// ```
/// use aac::aac_dec::lpd::constants::M_LP_FILTER_ORDER;
/// use aac::aac_dec::lpd::lpc::conceal;
///
/// let mut lsp = vec![[0.0_f32; M_LP_FILTER_ORDER]; 5];
/// // Precondition: should have valid data in `lpc4_lsf`, if `first_lpd_flag` is false.
/// let mut lpc4_lsf = vec![0.0_f32; M_LP_FILTER_ORDER];
/// let lsf_adaptive_mean = vec![0.0_f32; M_LP_FILTER_ORDER];
/// let first_lpd_flag = true;
///
/// conceal(&mut lsp, &mut lpc4_lsf, &lsf_adaptive_mean, first_lpd_flag);
/// ```
pub fn conceal(
    lsp: &mut [[f32; M_LP_FILTER_ORDER]],
    lpc4_lsf: &mut [f32],
    lsf_adaptive_mean: &[f32],
    first_lpd_flag: bool,
) {
    const BETA_FL: f32 = 0.25;
    const ONE_BETA_FL: f32 = 0.75;
    const BFI_FAC_FL: f32 = 0.90;
    const ONE_BFI_FAC_FL: f32 = 0.10;

    if first_lpd_flag {
        // Reset past LSF values.
        lpc4_lsf[..M_LP_FILTER_ORDER].copy_from_slice(&LSF_INIT[..M_LP_FILTER_ORDER]);
        lsp[0][..M_LP_FILTER_ORDER].copy_from_slice(&lpc4_lsf[..M_LP_FILTER_ORDER]);
    } else {
        // Old LPC4 is new LPC0.
        lsp[0][..M_LP_FILTER_ORDER].copy_from_slice(&lpc4_lsf[..M_LP_FILTER_ORDER]);
    }

    // LPC1
    for (lsp_data, lsf_init, lsf_mean, lpc4_lsf_data) in izip!(
        lsp[1].iter_mut(),
        LSF_INIT.iter(),
        lsf_adaptive_mean.iter(),
        lpc4_lsf.iter()
    )
    .take(M_LP_FILTER_ORDER)
    {
        let tmp_lsf_mean = BETA_FL * *lsf_init + ONE_BETA_FL * *lsf_mean;
        *lsp_data = BFI_FAC_FL * *lpc4_lsf_data + ONE_BFI_FAC_FL * tmp_lsf_mean;
    }

    // LPC2 - LPC4
    for index in 2..=4 {
        for ((len, lsf_init), lsf_mean) in
            izip!(LSF_INIT.iter().enumerate(), lsf_adaptive_mean.iter()).take(M_LP_FILTER_ORDER)
        {
            let tmp_lsf_mean = (BETA_FL + index as f32 * 0.1_f32) * *lsf_init
                + (ONE_BETA_FL - index as f32 * 0.1_f32) * *lsf_mean;
            lsp[index][len] = BFI_FAC_FL * lsp[index - 1][len] + ONE_BFI_FAC_FL * tmp_lsf_mean;
        }
    }

    // Update past values for the future.
    lpc4_lsf[..M_LP_FILTER_ORDER].copy_from_slice(&lsp[4][..M_LP_FILTER_ORDER]);

    // Convert into LSP domain.
    for lsp_data in lsp.iter_mut().take(5) {
        for data in lsp_data.iter_mut().take(M_LP_FILTER_ORDER) {
            *data = f64::cos(f64::from(*data) * (PI_DOUBLE / (FREQ_MAX as f64))) as f32;
        }
    }
}

/// Algebraic Vector Quantization (AVQ) refinement decoding.
///
/// # Parameters
///
/// - `bs`: Bitstream reader with valid internal data.
/// - `lsfq`: AVQ decoded output buffer.
/// - `nk_mode`: Coding mode for codebook number `n_k`.
/// - `nqn`: Number of split/interleaved `RE8` vectors.
/// - `length`: Total amount of individual data values to decode.
///
/// Returns 0 on success, -1 on error.
///
/// # Examples
///
/// ```
/// use aac::aac_dec::lpd::constants::M_LP_FILTER_ORDER;
/// use aac::aac_dec::lpd::lpc::decode_avq;
/// use aac::common::bitstream::{Bitstream, Mode};
///
/// // Precondition: should have a valid Bitstream instance in reader mode.
/// let buffer = vec![0; 64];
/// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
/// bitstream_reader.init(&buffer, 250);
///
/// // Precondition: should have valid data in `lpc4_lsf`, if `first_lpd_flag` is false.
/// let mut lsfq = vec![0_f32; M_LP_FILTER_ORDER];
/// let mode = 1;
/// let nqn = 2; // num_re8_vectors
/// let length = M_LP_FILTER_ORDER;
///
/// decode_avq(&mut bitstream_reader, &mut lsfq, mode, nqn, length);
/// ```
pub fn decode_avq(
    bs: &mut Bitstream,
    lsfq: &mut [f32],
    nk_mode: u8,
    nqn: usize,
    length: usize,
) -> i32 {
    debug_assert!(nqn <= 2 && 8 * nqn <= lsfq.len()); // min lsfq buffer length

    let mut len = 0;
    while len < length {
        let mut qn = [0_u8; 2];
        let mut kv = [0_i32; 8];

        decode_qn(bs, nk_mode, nqn, &mut qn);
        for l in 0..nqn {
            if qn[l] == 0 {
                lsfq[len + (l * 8)..8 + len + (l * 8)].fill(0.0);
            }

            // Voronoi extension order ( nk )
            let mut nk = 0;
            let mut n = qn[l];
            if qn[l] > 4 {
                nk = (qn[l] - 3) >> 1;
                n = qn[l] - nk * 2;
            }
            // Base codebook index, in reverse bit group order (!).
            let cb_index = bs.read(4 * n) as u16;
            if nk > 0 {
                for item in kv.iter_mut().take(8) {
                    *item = bs.read(nk) as i32;
                }
            }
            if re8_dec(
                qn[l],
                cb_index,
                &kv,
                &mut lsfq[len + (l * 8)..8 + len + (l * 8)],
            ) != 0
            {
                return -1;
            }
        }
        len += 8 * nqn;
    }
    0
}

/// Converts LSP coefficients into LP domain.
///
/// # Parameters
///
/// - `lsp`: LSP coefficients.
/// - `a`: Output coefficients after conversion.
///
/// # Examples
///
/// ```
/// use aac::aac_dec::lpd::constants::M_LP_FILTER_ORDER;
/// use aac::aac_dec::lpd::lpc::lsp_to_lpc;
///
/// let mut output_buff = [0.0_f32; M_LP_FILTER_ORDER];
/// let input_buff = [0.0_f32; M_LP_FILTER_ORDER];
///
/// lsp_to_lpc(&input_buff, &mut output_buff);
/// ```
pub fn lsp_to_lpc(lsp: &[f32], a: &mut [f32]) {
    let mut f1 = [0.0_f32; NC + 1];
    let mut f2 = [0.0_f32; NC + 1];

    //  Find the polynomials F1(z) and F2(z).
    get_lsppol(lsp, &mut f1, 0);
    get_lsppol(lsp, &mut f2, 1);

    //  Multiply F1(z) by (1+z^-1) and F2(z) by (1-z^-1).
    for i in (1..=NC).rev() {
        f1[i] += f1[i - 1];
        f2[i] -= f2[i - 1];
    }

    let mut a_iter = a.iter_mut();
    for (f1_element, f2_element) in izip!(f1.iter().skip(1), f2.iter().skip(1)) {
        *(a_iter.next().unwrap()) = 0.5_f32 * (*f1_element + *f2_element);
        *(a_iter.next_back().unwrap()) = 0.5_f32 * (*f1_element - *f2_element);
    }
}

/// Reads and decodes LPC coefficient sets (First stage approximation + AVQ decode).
///
/// # Parameters
///
/// - `bs`: Bitstream reader with valid internal data
/// - `lsp`: Decoded LSP coefficients output
/// - `lpc4_lsf`: Decoded LCP4 LSF coefficients output
/// - `lsf_adaptive_mean_cand`: LSF adaptive mean output vector
/// - `stability`: Stability output values
/// - `lpd_modes`: LPD modes used in the current superframe
/// - `first_lpd_flag`: Flag indicating presence of LPC0
/// - `last_lpc_lost`: Flag indicating LPC4 of previous frame was lost
/// - `last_frame_ok`: Flag indicating previous LSF4 coefficients `lpc4_lsf` are valid
///
/// Returns 0 on success, -1 on error
///
/// # Examples
///
/// ```
/// use aac::aac_dec::lpd::common::LpdMode;
/// use aac::aac_dec::lpd::constants::{M_LP_FILTER_ORDER, NB_DIV};
/// use aac::aac_dec::lpd::lpc::read;
/// use aac::common::bitstream::{Bitstream, Mode};
///
/// // Precondition: should have a valid Bitstream instance in reader mode.
/// let buffer = vec![0; 64];
/// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
/// bitstream_reader.init(&buffer, 500);
///
/// // Precondition: should have valid data in lpc4_lsf if first_lpd_flag is false.
/// let mut lsp = [[0.0_f32; M_LP_FILTER_ORDER]; 5];
/// let mut lpc4_lsf = [0.0_f32; M_LP_FILTER_ORDER];
/// let mut stability = [0.0_f32; M_LP_FILTER_ORDER];
/// let mut lsf_adaptive_mean_cand = vec![0.0_f32; M_LP_FILTER_ORDER];
/// let lpd_modes = [LpdMode::Acelp; NB_DIV];
/// let length = M_LP_FILTER_ORDER;
/// let first_lpd_flag = true;
/// let last_lpc_lost = false;
/// let last_frame_ok = true;
///
/// read(
///     &mut bitstream_reader,
///     &mut lsp,
///     &mut lpc4_lsf,
///     &mut lsf_adaptive_mean_cand,
///     &mut stability,
///     &lpd_modes,
///     first_lpd_flag,
///     last_lpc_lost,
///     last_frame_ok,
/// );
/// ```
#[expect(clippy::too_many_arguments)]
pub fn read(
    bs: &mut Bitstream,
    lsp: &mut [[f32; M_LP_FILTER_ORDER]],
    lpc4_lsf: &mut [f32],
    lsf_adaptive_mean_cand: &mut [f32],
    stability: &mut [f32],
    lpd_modes: &[LpdMode],
    first_lpd_flag: bool,
    last_lpc_lost: bool,
    last_frame_ok: bool,
) -> i32 {
    let mut lpc_present = [false; 5];

    // Decode LPC4.
    lpc_present[4] = true;
    vlpc_first_stage_dec(bs, &mut lsp[4][..]);
    let mut err = vlpc_second_stage_dec(bs, &mut lsp[4][..], 0); // nk_mode = 0
    if err != 0 {
        return err;
    }

    let mut mode_lpc_bin;
    let mut nk_mode;
    let mut lpc0_available = true;

    // Decode LPC0.
    lpc_present[0] = true;
    if !first_lpd_flag {
        lpc0_available = !last_lpc_lost;
        // old LPC4 is new LPC0
        lsp[0][..M_LP_FILTER_ORDER].copy_from_slice(&lpc4_lsf[..M_LP_FILTER_ORDER]);
        // Skip LPC0 and continue with LPC2.
    } else {
        nk_mode = 0;

        mode_lpc_bin = bs.read_bit() as u8;
        if mode_lpc_bin == 0 {
            // LPC0/LPC2: Abs
            vlpc_first_stage_dec(bs, &mut lsp[0][..]);
        } else {
            // LPC0/LPC2: RelR
            let (lsp_offset_0, lsp_offset_4) = lsp.split_at_mut(4);
            lsp_offset_0[0][..M_LP_FILTER_ORDER]
                .copy_from_slice(&lsp_offset_4[0][..M_LP_FILTER_ORDER]);
            nk_mode = 3;
        }

        err = vlpc_second_stage_dec(bs, &mut lsp[0][..], nk_mode);
        if err != 0 {
            return err;
        }
    }

    // Decode LPC2.
    if lpd_modes[0].num_subframes() != 4 {
        nk_mode = 0;
        lpc_present[2] = true;

        mode_lpc_bin = bs.read_bit() as u8;
        if mode_lpc_bin == 0 {
            // LPC0/LPC2: Abs
            vlpc_first_stage_dec(bs, &mut lsp[2][..]);
        } else {
            // LPC0/LPC2: RelR
            let (lsp_offset_0, lsp_offset_4) = lsp.split_at_mut(4);
            lsp_offset_0[2][..M_LP_FILTER_ORDER]
                .copy_from_slice(&lsp_offset_4[0][..M_LP_FILTER_ORDER]);
            nk_mode = 3;
        }

        err = vlpc_second_stage_dec(bs, &mut lsp[2][..], nk_mode);
        if err != 0 {
            return err;
        }
    }

    // Decode LPC1.
    if lpd_modes[0].num_subframes() == 1 {
        // else: skip LPC1
        lpc_present[1] = true;
        mode_lpc_bin = get_vlclbf_n(bs, 2);

        match mode_lpc_bin {
            0 => {
                // LPC1: RelR
                let (lpc_offset_0, lpc_offset_2) = lsp.split_at_mut(2);
                lpc_offset_0[1][..M_LP_FILTER_ORDER]
                    .copy_from_slice(&lpc_offset_2[0][..M_LP_FILTER_ORDER]);
                let err = vlpc_second_stage_dec(bs, &mut lsp[1][..], 2);
                if err != 0 {
                    return err;
                }
            }
            1 => {
                // LPC1: abs
                vlpc_first_stage_dec(bs, &mut lsp[1][..]);
                let err = vlpc_second_stage_dec(bs, &mut lsp[1][..], 0);
                if err != 0 {
                    return err;
                }
            }
            2 => {
                // LPC1: mid0 (no second stage AVQ quantizer in this case)
                if lpc0_available {
                    // LPC0/lsf[0] might be zero some times
                    let (lsp_offset_0, lsp_hi) = lsp.split_at_mut(1);
                    let (lsp_offset_1, lsp_offset_2) = lsp_hi.split_at_mut(1);
                    izip!(
                        lsp_offset_1[0].iter_mut(),
                        lsp_offset_0[0].iter(),
                        lsp_offset_2[0].iter()
                    )
                    .for_each(|(lsp1, lsp0, lsp2)| {
                        *lsp1 = (*lsp0 / 2.0_f32) + (*lsp2 / 2.0_f32);
                    });
                } else {
                    let (lsp_offset_0, lsp_offset_2) = lsp.split_at_mut(2);
                    lsp_offset_0[1][..M_LP_FILTER_ORDER]
                        .copy_from_slice(&lsp_offset_2[0][..M_LP_FILTER_ORDER]);
                }
            }
            _ => unreachable!(),
        }
    }

    // Decode LPC3.
    if lpd_modes[2].num_subframes() == 1 {
        // else: skip LPC3
        nk_mode = 0;
        lpc_present[3] = true;

        mode_lpc_bin = get_vlclbf_n(bs, 3);

        match mode_lpc_bin {
            0 => {
                // LPC3: mid
                let (lsp_offset_0, lsp_hi) = lsp.split_at_mut(3);
                let (lsp_offset_3, lsp_offset_4) = lsp_hi.split_at_mut(1);
                izip!(
                    lsp_offset_3[0].iter_mut(),
                    lsp_offset_0[2].iter(),
                    lsp_offset_4[0].iter()
                )
                .for_each(|(lsp3, lsp2, lsp4)| {
                    *lsp3 = (*lsp2 / 2.0_f32) + (*lsp4 / 2.0_f32);
                });
                nk_mode = 1;
            }
            1 => {
                // LPC3: abs
                vlpc_first_stage_dec(bs, &mut lsp[3][..]);
            }

            2 => {
                // LPC3: relL
                let (lsp_offset_0, lsp_offset_3) = lsp.split_at_mut(3);
                lsp_offset_3[0][..M_LP_FILTER_ORDER]
                    .copy_from_slice(&lsp_offset_0[2][..M_LP_FILTER_ORDER]);
                nk_mode = 2;
            }
            3 => {
                // LPC3: relR
                let (lsp_offset_0, lsp_offset_4) = lsp.split_at_mut(4);
                lsp_offset_0[3][..M_LP_FILTER_ORDER]
                    .copy_from_slice(&lsp_offset_4[0][..M_LP_FILTER_ORDER]);
                nk_mode = 2;
            }
            _ => unreachable!(),
        }
        err = vlpc_second_stage_dec(bs, &mut lsp[3][..], nk_mode);
        if err != 0 {
            return err;
        }
    }

    if !lpc0_available && !last_frame_ok {
        const LSF_INIT_TILT: f32 = 0.25_f32;
        // LPC(0) was lost. Use next available LPC(k) instead.

        for k in 1..(NB_DIV + 1) {
            if lpc_present[k] {
                for (i, lsf_init) in LSF_INIT.iter().enumerate().take(M_LP_FILTER_ORDER) {
                    if lpd_modes[0].is_tcx() {
                        lsp[0][i] =
                            (lsp[k][i] * (1.0_f32 - LSF_INIT_TILT)) + (*lsf_init * LSF_INIT_TILT);
                    } else {
                        lsp[0][i] = lsp[k][i];
                    }
                }
                break;
            }
        }
    }

    lpc4_lsf[..M_LP_FILTER_ORDER].copy_from_slice(&lsp[4][..M_LP_FILTER_ORDER]);

    let mut idx = NB_DIV;
    let mut num_lpc = 0;
    loop {
        let i = idx;
        idx -= 1;
        if lpc_present[i] {
            num_lpc += 1
        };
        if idx == 0 || num_lpc >= 3 {
            break;
        }
    }

    let div_fac = match num_lpc {
        2 => 0.5_f32,
        3 => 1.0_f32 / 3.0_f32,
        _ => 1.0_f32,
    };

    // Get the adaptive mean for the next (bad) frame.
    for (k, lsf_mean) in lsf_adaptive_mean_cand
        .iter_mut()
        .enumerate()
        .take(M_LP_FILTER_ORDER)
    {
        let mut tmp = 0.0_f32;
        for i in (idx + 1..=NB_DIV).rev() {
            if lpc_present[i] {
                tmp += lsp[i][k] * div_fac;
            }
        }
        *lsf_mean = tmp;
    }

    // Calculate stability factor Theta. Needed for ACELP decoder and concealment.
    {
        let mut k = 0;
        let mut lsf_prev = 0;
        for i in 1..NB_DIV + 1 {
            if lpc_present[i] {
                let mut lsf_dist = 0.0_f32;
                let lsf_curr = i;

                for (prev, curr) in lsp[lsf_prev]
                    .iter()
                    .zip(lsp[lsf_curr].iter().take(M_LP_FILTER_ORDER))
                {
                    lsf_dist += (*curr - *prev) * (*curr - *prev);
                }

                // Stability = 1.25-LSFDist / 400000 , 0 <= Stability <= 1
                stability[k] = (1.25_f32 - lsf_dist / 400000.0_f32).clamp(0.0_f32, 1.0f32);
                lsf_prev = lsf_curr;
                k = i;
            } else {
                // Mark stability value as undefined.
                stability[i] = -1.0_f32;
            }
        }
    }
    // convert into LSP domain
    for (lpc_flag, lsp_data) in lpc_present.iter().zip(lsp[..].iter_mut().take(NB_DIV + 1)) {
        if *lpc_flag {
            for data in lsp_data.iter_mut().take(M_LP_FILTER_ORDER) {
                *data = f64::cos(f64::from(*data) * (PI_DOUBLE / (FREQ_MAX as f64))) as f32;
            }
        }
    }

    err
}

// Private functions

/// Decodes `nqn` amount of code book numbers. These values determine the
/// amount of following bits for `nqn` AVQ RE8 vectors.
///
/// # Parameters
/// - `bs`: Bitstream instance with valid internal data.
/// - `nk_mode`: quantization mode.
/// - `nqn`: code book number amount to read.
/// - `qn`: output buffer to hold decoded code book numbers.
fn decode_qn(bs: &mut Bitstream, nk_mode: u8, nqn: usize, qn: &mut [u8]) {
    if nk_mode == 1 {
        // Unary code for mid LPC1/LPC3.
        // Q0=0, Q2=10, Q3=110, ...
        for item in qn.iter_mut().take(nqn) {
            *item = get_vlclbf(bs);
            if *item > 0 {
                *item += 1;
            }
        }
    } else {
        // nk_mode 0, 3 and 2
        // 2 bits to specify Q2,Q3,Q4,ext
        for item in qn.iter_mut().take(nqn) {
            *item = 2 + (bs.read(2) as u8);
        }
        if nk_mode == 2 {
            // Unary code for rel LPC1/LPC3.
            // Q0 = 0, Q5=10, Q6=110, ...
            for item in qn.iter_mut().take(nqn) {
                if *item > 4 {
                    *item = get_vlclbf(bs);
                    if *item > 0 {
                        *item += 4;
                    }
                }
            }
        } else {
            // nk_mode == (0 and 3)
            // Unary code for abs and rel LPC0/LPC2.
            // Q5 = 0, Q6=10, Q0=110, Q7=1110, ...
            for item in qn.iter_mut().take(nqn) {
                if *item > 4 {
                    *item = get_vlclbf(bs);

                    match *item {
                        0 => {
                            *item = 5;
                        }
                        1 => {
                            *item = 6;
                        }
                        2 => {
                            *item = 0;
                        }
                        _ => {
                            *item += 4;
                        }
                    }
                }
            }
        }
        // The maximum value that the output values from qn[] can have is ((NQ_MAX + 1) + 4).
        debug_assert!(qn.iter().max() <= Some(&((NQ_MAX + 1) + 4)));
    }
}

/// Returns cardinality offset (index) of leaders (signed/absolute) from look-up table:
/// - i.e.: find `index` where `value >= table[index] && value < table[index+1]`.
/// - note: `table.len() >= 2 && value >= table[0]`.
fn find_cardinality_index(table: &[u16], value: u16) -> usize {
    debug_assert!(table.len() >= 2 && value >= table[0]);

    let mut tab_index = 4;
    let range = table.len();

    loop {
        if tab_index >= range || value < table[tab_index] {
            break;
        }
        tab_index += 4;
    }
    if tab_index > range {
        tab_index = range;
    }

    if value < table[tab_index - 2] {
        tab_index -= 2;
    }
    if value < table[tab_index - 1] {
        tab_index -= 1;
    }
    tab_index -= 1;

    tab_index
}

/// # Parameters:
/// - `lsp`: Input LSP array
/// - `f`:   Output LP filter coefficient array.
/// - `offset`: Offset to LSP coefficients to read.
fn get_lsppol(lsp: &[f32], f: &mut [f32], offset: usize) {
    let mut index = offset;

    f[0] = 1.0_f32;
    let mut b = -2.0_f32 * lsp[index];
    f[1] = b;

    for i in 2..=(f.len() - 1) {
        index += 2;
        b = -2.0f32 * lsp[index];
        f[i] = (b * f[i - 1]) + 2.0_f32 * f[i - 2];

        for j in (2..i).rev() {
            f[j] = f[j] + (b * f[j - 1]) + f[j - 2];
        }
        f[1] += b;
    }
}

/// Reads unary code.
///
/// # Parameters
/// - `bs`: Bitstream instance with valid internal data.
///
///  # Return:
/// - decoded value.
fn get_vlclbf(bs: &mut Bitstream) -> u8 {
    let mut result = 0;
    while bs.read_bit() != 0 && (result <= NQ_MAX) {
        result += 1;
    }
    result
}

/// Reads bit count limited unary code.
///
/// # Parameters
/// - `bs`: Bitstream instance with valid internal data.
/// - `n`: Max amount of bits to be read.
///
/// # Return:
/// - decoded value.
fn get_vlclbf_n(bs: &mut Bitstream, mut n: u8) -> u8 {
    let mut result = 0;
    if n != 0 {
        while bs.read_bit() != 0 {
            n -= 1;
            result += 1_u8;
            if n == 0 {
                break;
            }
        }
    }
    result
}

/// Calculates inverse weighting factor and adds non-weighted residual
/// LSF array to the first stage of LSF approximation.
///
/// # Parameters
///
/// - `lsfq`: First stage LSF output approximation values.
/// - `xq`: Weighted residual LSF vector
/// - `nk_mode`: Code book number coding mode
fn lsf_weight_second_stage(lsfq: &mut [f32], xq: &[f32], nk_mode: u8) {
    let mut dist = [0.0f32; M_LP_FILTER_ORDER + 1];
    // lsf distance
    dist[0] = lsfq[0];
    dist[M_LP_FILTER_ORDER] = FREQ_MAX - lsfq[M_LP_FILTER_ORDER - 1];
    for (dist_element, (lsfq_element_prev, lsfq_element_curr)) in
        dist.iter_mut().skip(1).zip(lsfq.iter().tuple_windows())
    {
        *dist_element = *lsfq_element_curr - *lsfq_element_prev;
    }

    let factor = match nk_mode {
        0 => 60.0_f32, // abs
        1 => 65.0_f32, // mid
        2 => 64.0_f32, // rel1
        _ => 63.0_f32, // rel2
    };

    // Add non-weighted residual LSF vector to LSF1st.
    for (lsfq_element, xq_element, (dist_element_curr, dist_element_next)) in
        izip!(lsfq.iter_mut(), xq.iter(), dist.iter().tuple_windows())
    {
        let w_inv = factor / FREQ_DIV * f32::sqrt(*dist_element_curr * *dist_element_next);
        *lsfq_element += w_inv * (*xq_element);
    }
}

/// Nearest neighbor search in infinite lattice 2D8.
///
/// # Algorithm:
/// See `Conway and Sloane's` paper in:
/// - `IT-28 IEEE TRANSACTIONS ON INFORMATION THEORY, VOL. 28, NO. 2, MARCH 1982`
/// - `nn_2D8(x) = 2*nn_D8(x/2)`
/// - `nn_D8 = decoding of Z^8 with Wagner rule`
///
/// # Parameters
/// - `x (i)`: point in R^8
/// - `y (o)`: point in 2D8 (8-dimensional integer vector)
fn nearest_neighbor_2d8(x: &[f32; 8], y: &mut [i32; 8]) {
    let mut sum = 0_i32;
    let mut e = [0.0_f32; 8];

    // Round x into 2Z^8 i.e.:
    // Compute y=(y1,...,y8) such that yi = 2[xi/2] where [.] is the nearest integer operator.
    // In the meantime, compute sum = y1+...+y8.

    for (x_element, y_element) in x.iter().zip(y.iter_mut()) {
        // round to ..., -2, 0, 2, ... ([-1..1[ --> 0)
        if *x_element < 0.0_f32 {
            let tmp = 1.0_f32 - x_element;
            *y_element = -2 * ((tmp as i32) >> 1);
        } else {
            let tmp = 1.0_f32 + x_element;
            *y_element = 2 * ((tmp as i32) >> 1);
        }
        sum += *y_element;
    }

    // Check if `y1+...+y8` is a multiple of 4.
    // If not, `y` is not round `xj` in the wrong way, where `j` is defined by:
    // `j = arg max_i | xi -yi|`
    // (this is called the Wagner rule)
    if sum % 4 != 0 {
        // Find j = arg max_i | xi -yi|.
        let mut em = 0.0_f32;
        let mut j = 0_usize;

        izip!(e.iter_mut(), x.iter(), y.iter()).for_each(|(e_element, x_element, y_element)| {
            *e_element = *x_element - (*y_element as f32)
        });

        for (i, e_element) in e.iter_mut().enumerate() {
            // Compute |ei| = | xi-yi |
            let s: f32 = (*e_element).abs();

            // Check if |ei| is maximal, if so, set j=i.
            if em < s {
                em = s;
                j = i;
            }
        }

        // Round xj in the "wrong way".
        if e[j] < 0.0_f32 {
            y[j] -= 2;
        } else {
            y[j] += 2;
        }
    }
}

/// Multi-rate indexing of a point `y` in the lattice RE8 (index decoding).
///
/// # Parameters
/// - `n (i)`: codebook number (n is an integer defined in {0,2,3,4,..,n_max}). n_max = 36.
/// - `index (i)`: base codebook index
/// - `k (i)`: index of v (8-dimensional vector of binary indices) = Voronoi index
/// - `y (o)`: point in RE8 (8-dimensional vector)
///
///  # Return:
/// - 0 on success, -1 on error.
fn re8_dec(n: u8, index: u16, k: &[i32; 8], y: &mut [f32]) -> i32 {
    let mut v = [0_i32; 8];
    let mut c = [0_i16; 8];

    // Check bound of codebook qn
    if n > NQ_MAX {
        return -1;
    }

    // Decode the sub-indices `i` and `kv[]` according to the codebook number `n`:
    // If `n=0,2,3,4`, decode `i` (no Voronoi extension).
    // If `n>4`, Voronoi extension is used, decode `i` and `kv[]`.
    if n <= 4 {
        re8_decode_base_index(n, index, &mut c);
        for idx in 0..8 {
            y[idx] = f32::from(c[idx]);
        }
    } else {
        // Compute the Voronoi modulo `m = 2^r`, where `r` is extension order.
        let mut cb_n = n;
        let r = (cb_n - 3) >> 1;

        while cb_n > 4 {
            cb_n -= 2;
        }
        // Decode base codebook index `i` into c (c is an element of Q3 or Q4).
        // [here c is stored in y to save memory]
        re8_decode_base_index(cb_n, index, &mut c);
        // Decode Voronoi index `k[]` into `v`.
        re8_k2y(k, r.into(), &mut v);
        // Reconstruct y as `y = m c + v` (with m=2^r, r integer >=1).
        for idx in 0..8 {
            y[idx] = ((i32::from(c[idx]) << r) + v[idx]) as f32;
        }
    }
    0
}

/// Decodes an index in Qn (n=0,2,3 or 4).
///
/// # Parameters
/// - `n     (i)`: codebook number (n is an integer defined in {0,2,3,4})
/// - `index (i)`: base codebook index
/// - `y     (o)`: point in RE8 (8-dimensional integer vector)
fn re8_decode_base_index(n: u8, index: u16, y: &mut [i16; 8]) {
    let ka;
    if n < 2 {
        y.fill(0);
    } else {
        // Search for the identifier `ka` of the absolute leader (table-lookup).
        // Q2 is a subset of Q3 - the two cases are considered in the same branch.
        match n {
            2 | 3 => {
                let tab_index = find_cardinality_index(&I3, index);
                ka = usize::from(A3[tab_index]);
            }
            4 => {
                let tab_index = find_cardinality_index(&I4, index);
                ka = usize::from(A4[tab_index]);
            }
            _ => {
                unreachable!()
            }
        }

        // Reconstruct the absolute leader.
        let mut leader = [0_i16; 8];

        for (i, item) in leader.iter_mut().enumerate() {
            *item = i16::from(DA[ka][i]);
        }

        // Search for the identifier `ks` of the signed leader (table look-up).
        // This search is focused based on the identifier `ka` of the absolute leader.
        let t = IA[ka] as usize;

        let ks = find_cardinality_index(&IS[t..t + NS[ka] as usize], index);

        // Reconstruct the signed leader from its sign code.
        let sign_code = DS[t + ks];
        for i in (0..8).rev() {
            if sign_code & (1 << i) != 0 {
                leader[7 - i] = -leader[7 - i];
            }
        }

        // Compute and decode the rank of the permutation.
        // `rank = index - cardinality offset`
        let rank = index - IS[t + ks];

        re8_decode_rank_of_permutation(rank, &leader, y);
    }
}

/// Decodes the rank of the permutation of xs[].
///
/// # Parameters
///  - `rank (i)`: index (rank) of a permutation
///  - `xs   (o)`: signed leader in RE8 (8-dimensional integer vector)
///  - `x    (o)`: point in RE8 (8-dimensional integer vector)
fn re8_decode_rank_of_permutation(rank: u16, xs: &[i16; 8], x: &mut [i16; 8]) {
    let mut a = [0_i16; 8];
    let mut w = [0_i32; 8];

    // --- Pre-processing based on the signed leader xs ---
    // - Compute the alphabet a=[a[0] ... a[q-1]] of x (q elements), such that a[0]!=...!=a[q-1]. It
    //   is assumed that xs is sorted in the form of a signed leader, which can be summarized in 2
    //   requirements: a) |xs[0]| >= |xs[1]| >= |xs[2]| >= ... >= |xs[7]| b) if |xs[i]|=|xs[i-1]|,
    //   xs[i]>=xs[i+1] where |.| indicates the absolute value operator.
    // - Compute q (the number of symbols in the alphabet).
    // - Compute w[0..q-1] where w[j] counts the number of occurences of the symbol a[j] in xs.
    // - Compute B = prod_j=0..q-1 (w[j]!) where .! is the factorial
    // xs[i], xs[i-1] and ptr_w/a
    let mut index = 0;
    w[index] = 1;
    a[index] = xs[0];
    let mut b_val = 1;

    for (xs_prev, xs_curr) in xs.iter().tuple_windows() {
        if *xs_curr != *xs_prev {
            index += 1;
            w[index] = 1;
            a[index] = *xs_curr;
        } else {
            w[index] += 1;
            b_val *= w[index];
        }
    }

    // --- Actual rank decoding ---
    // The rank of x (where x is a permutation of xs) is based on Schalkwijk's formula.
    // It is given by rank=sum_{k=0..7} (A_k * fac_k/B_k).
    // The decoding of this rank is sequential and reconstructs x[0..7]
    // element by element from x[0] to x[7].
    // The tricky part is the inference of A_k for each k...
    if w[0] == 8 {
        x[..8].fill(a[0]); // Avoid fac of 40320.
    } else {
        let mut target = i32::from(rank) * b_val;
        let mut fac_b_val = 1;
        // Decode x element by element.
        for (tab_fact, x_val) in FACTORIALS.iter().zip(x.iter_mut()).take(8) {
            let fac = fac_b_val * *tab_fact; // fac = 1..5040

            index = 0;
            loop {
                target -= w[index] * fac as i32;
                if target < 0 {
                    break;
                }
                index += 1;
            }

            *x_val = a[index];
            // Update rank, denominator B (B_k) and counter w[j]
            // target = fac_B*B*rank
            target += w[index] * fac as i32;
            fac_b_val *= w[index] as u32;
            w[index] -= 1;
        }
    }
}

/// Voronoi indexing (index decoding) k -> y.
///
/// # Parameters
/// - `k (i)`: Voronoi index k[0..7]
/// - `r (i)`: Voronoi order  (`m = 2^r = 1<<r`, where `r` is integer `>=2`)
/// - `y (o)`: 8-dimensional point `y[0..7]` in RE8
fn re8_k2y(k: &[i32; 8], r: i32, y: &mut [i32; 8]) {
    let mut v: [i32; 8] = [0_i32; 8];
    let mut zf: [f32; 8] = [0.0_f32; 8];

    // compute `y = k M` and `z=(y-a)/m`, where:
    // M = [4        ]
    //     [2 2      ]
    //     [|   \    ]
    //     [2     2  ]
    //     [1 1 _ 1 1]
    //     a=(2,0,...,0)
    //     m = 1<<r

    y.iter_mut().for_each(|y_element| *y_element = k[7]);
    zf[7] = (y[7] as f32) / ((1_i32 << r) as f32);
    let mut sum = 0_i32;
    for i in (1..=6).rev() {
        let tmp = 2 * k[i];
        sum += tmp;
        y[i] += tmp;
        zf[i] = (y[i] as f32) / ((1_i32 << r) as f32);
    }
    y[0] += 4 * k[0] + sum;
    zf[0] = ((y[0] - 2) as f32) / ((1_i32 << r) as f32);

    // Find nearest neighbor v of z in infinite RE8.
    re8_ppv(&zf, &mut v);

    // Compute `y -= m v`.
    y.iter_mut()
        .zip(v.iter())
        .for_each(|(y_element, v_element)| *y_element -= v_element << r);
}

/// Nearest neighbor search in infinite lattice RE8.
/// The algorithm is based on the definition of RE8 as
/// `RE8 = (2D8) U (2D8+[1,1,1,1,1,1,1,1])`.
/// It applies the coset decoding of Sloane and Conway.
///
/// # Parameters
/// - `x (i)`: point in R^8
/// - `y (o)`: point in RE8 (8-dimensional integer vector)
fn re8_ppv(x: &[f32; 8], y: &mut [i32; 8]) {
    let mut y0: [i32; 8] = [0; 8];
    let mut y1: [i32; 8] = [0; 8];
    let mut x1: [f32; 8] = [0.0f32; 8];

    // Find the nearest neighbor y0 of x in 2D8.
    nearest_neighbor_2d8(x, &mut y0);
    // Find the nearest neighbor y1 of x in 2D8+(1,...,1) (by coset decoding).
    x1.iter_mut()
        .zip(x.iter())
        .for_each(|(x1_element, x_element)| *x1_element = *x_element - 1.0_f32);
    nearest_neighbor_2d8(&x1, &mut y1);
    y1.iter_mut().for_each(|y1_element| *y1_element += 1);

    // Compute `e0=||x-y0||^2` and `e1=||x-y1||^2`.
    let mut e = 0.0_f32;
    izip!(x.iter(), y0.iter(), y1.iter()).for_each(|(x_element, y0_element, y1_element)| {
        let mut tmp = *x_element - *y0_element as f32;
        e += tmp * tmp;
        tmp = *x_element - *y1_element as f32;
        e -= tmp * tmp;
    });

    // Select best candidate `y0` or `y1` to minimize distortion.
    if e < 0.0_f32 {
        y.copy_from_slice(&y0[..]);
    } else {
        y.copy_from_slice(&y1[..]);
    }
}

/// Reorders LSF coefficients to minimum distance.
///
/// # Parameters
/// - `lsf`: Input: LSF coefficients. Output: reordered LSF coeffs.
/// - `min_dist`: Minimum distance
fn reorder_lsf(lsf: &mut [f32], min_dist: f32) {
    let mut lsf_min = min_dist;
    lsf.iter_mut()
        .take(M_LP_FILTER_ORDER)
        .for_each(|lsf_element| {
            if *lsf_element < lsf_min {
                *lsf_element = lsf_min;
            }
            lsf_min = *lsf_element + min_dist;
        });
    // reverse
    lsf_min = FREQ_MAX - min_dist;
    lsf.iter_mut()
        .take(M_LP_FILTER_ORDER)
        .rev()
        .for_each(|lsf_element| {
            if *lsf_element > lsf_min {
                *lsf_element = lsf_min;
            }
            lsf_min = *lsf_element - min_dist;
        });
}

/// First stage approximation.
///
/// # Parameters
/// - `bs`: Bitstream instance with valid internal data.
/// - `lsfq`: output buffer to hold LPC coefficients.
fn vlpc_first_stage_dec(bs: &mut Bitstream, lsfq: &mut [f32]) {
    let index = bs.read(8) as usize;
    lsfq[0..M_LP_FILTER_ORDER].copy_from_slice(
        &DICO_LSF_ABS_8B[index * M_LP_FILTER_ORDER..(index + 1) * M_LP_FILTER_ORDER],
    );
}

/// Does first stage approximation weighting and multiplies with AVQ refinement.
///
/// # Parameters
/// - `bs`: Bitstream instance with valid internal data.
/// - `lsfq`: `Input`: buffer holding 1st stage approx. `Output`: 1st + 2nd stage approx.
/// - `nk_mode`: quantization mode (0=abs, >0=rel)
///
/// # Return:
/// - 0 on success, -1 on error.
fn vlpc_second_stage_dec(bs: &mut Bitstream, lsfq: &mut [f32], nk_mode: u8) -> i32 {
    // weighted residual LSF vector
    let mut xq = [0_f32; M_LP_FILTER_ORDER];
    // Decode AVQ refinement.
    let err = decode_avq(bs, &mut xq, nk_mode, 2, 8);
    if err != 0 {
        return -1;
    }
    // Add non-weighted residual LSF vector to 1st LSF.
    lsf_weight_second_stage(lsfq, &xq, nk_mode);
    // reorder
    reorder_lsf(lsfq, LSF_GAP);
    0
}
