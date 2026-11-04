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
//! Transform Coding eXtended (TCX)

use crate::aac_dec::error_codes::AacDecoderError;
use crate::aac_dec::lpd::common as lpd_common;
use crate::aac_dec::lpd::common::LpdMode;
use crate::aac_dec::lpd::constants::*;
use crate::aac_dec::lpd::lpc as lpd_lpc;
use crate::aac_dec::utils::generate_random_sign;
use crate::arith_coding::arith_dec::ArithDecoderData;
use crate::common::arith_ops::calc_energy;
use crate::common::bitstream::Bitstream;
use crate::common::fft::fft;
use crate::common::flags::ACFlags;
use crate::common::tables::sine_tables::{SINE_TABLE_48, SINE_TABLE_64};
use itertools::izip;
use itertools::Itertools;
use num_complex::Complex;

/// FD noise shaping resolution (64=100Hz/point)
const FDNS_NPTS_1024: usize = 64;
const FDNS_NPTS: usize = FDNS_NPTS_1024;

#[derive(Default, Debug)]
#[repr(C)]
/// TCX Info. Holds information for one TCX frame in the LPD superframe.
pub struct TcxInfo {
    pub gain: f32,
    noise_factor: u8,
    global_gain: u8,
}

#[repr(C)]
#[derive(Debug)]
/// TCX Data. Holds TCX information of all TCX frames.
///
/// Therefore, it contains four instances of `TcxInfo`. Additionally, it keeps
/// information of the last decoded frame, needed for concealment and the
/// forward aliasing cancellation (FAC).
pub struct TcxData {
    pub last_gain: f32,
    pub last_alfd_gains: [f32; 32],
    pub last_pitch: u16,
    pub tcx_info: [TcxInfo; 4],
}

impl TcxInfo {
    /// Reads the TCX bitstream information.
    ///
    /// Arithmetic decoding of the spectral parameters is done on the fly inside.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream handle to read from.
    /// - `spec`: Decoded spectrum.
    /// - `arco`: Arithmetic decoder instance.
    /// - `scratch_buffer`: Scratch buffer used for decoding spectral data. Must be >= spec.len().
    /// - `is_1st_frame`: Flag indicating that this is the first TCX frame.
    /// - `flags`: Audio coding flags (see common/flags.rs).
    fn read(
        &mut self,
        bs: &mut Bitstream,
        spec: &mut [f32],
        arco: &mut ArithDecoderData,
        scratch_buffer: &mut [i16],
        is_1st_frame: bool,
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        // TCX noise level.
        self.noise_factor = bs.read(3) as u8;

        // TCX global gain.
        self.global_gain = bs.read(7) as u8;

        // Arithmetic coded residual/spectrum.
        let arith_reset_flag = if is_1st_frame {
            if ac_flags.contains(ACFlags::INDEP) {
                true
            } else {
                bs.read_bit() != 0
            }
        } else {
            false
        };

        // Decode spectral data.
        let arith_dec_pass =
            arco.decode(bs, scratch_buffer, spec.len(), spec.len(), arith_reset_flag);

        // Convert spectral data to float.
        izip!(spec.iter_mut(), scratch_buffer.iter())
            .for_each(|(spec_element, buff_element)| *spec_element = *buff_element as f32);

        if !arith_dec_pass {
            AacDecoderError::Unknown
        } else {
            AacDecoderError::Ok
        }
    }
}

impl Default for TcxData {
    fn default() -> Self {
        Self {
            last_gain: Default::default(),
            last_alfd_gains: Default::default(),
            last_pitch: L_DIV as u16,
            tcx_info: Default::default(),
        }
    }
}

impl TcxData {
    // Public functions

    /// Decodes and renders one MDCT-TCX frame.
    ///
    /// First, comfort noise is applied where there is no signal at all.
    /// Then, frequency-domain noise shaping (FDNS) and an inverse
    /// MDCT is applied to get the time domain synthesis signal.
    ///
    /// # Parameters
    ///
    /// - `coeffs`: TCX coefficients.
    /// - `lsp_coeffs`: LSP coefficients.
    /// - `lp_coeffs`: LP coefficients.
    /// - `nf_random_seed`: Random generation seed.
    /// - `last_mod`: LPD mode of last frame.
    /// - `fdns_npts`: Number of lines (FDNS_NPTS).
    /// - `next_frame`: Index of next ACELP frame.
    /// - `frame`: Index of this ACELP frame.
    #[expect(clippy::too_many_arguments)]
    pub fn decode(
        &mut self,
        coeffs: &mut [f32],
        lsp_coeffs: &[[f32; M_LP_FILTER_ORDER]],
        lp_coeffs: &mut [[f32; M_LP_FILTER_ORDER]; 5],
        nf_random_seed: &mut u32,
        last_mod: LpdMode,
        fdns_npts: usize,
        next_frame: usize,
        frame: usize,
    ) {
        // Find pitch - used for concealment.
        self.find_mpitch(coeffs);

        // Create comfort noise in silent parts of the signal.
        self.create_comfort_noise(coeffs, nf_random_seed, frame);

        // Convert LPC parameters to LP domain.
        if last_mod.is_acelp() {
            // Note: The case where last_mod == NotLpd is handled by other means in
            // lpd::read().
            lpd_lpc::lsp_to_lpc(&lsp_coeffs[frame], &mut lp_coeffs[frame]);
        }
        lpd_lpc::lsp_to_lpc(&lsp_coeffs[next_frame], &mut lp_coeffs[next_frame]);

        // FDNS decoding.
        self.fdns_decode(frame, next_frame, coeffs, lp_coeffs, fdns_npts);
    }

    /// Initializes `TcxData` instance.
    pub fn init(&mut self) {
        *self = Default::default();
    }

    /// Creates a new `TcxData` instance with default values.
    pub fn new() -> Self {
        Default::default()
    }

    /// Reads the TCX bitstream information for the current TCX frame.
    ///
    /// Arithmetic decoding of the spectral parameters is done on the fly inside.
    ///
    /// # Parameters
    ///
    /// - `k`: Number of the current TCX frame.
    /// - `bs`: Bitstream handle to read from.
    /// - `spec`: Decoded spectrum.
    /// - `arco`: Arithmetic decoder instance.
    /// - `scratch_buffer`: Scratch buffer used for decoding spectral data. Must be >= spec.len().
    /// - `is_1st_frame`: Flag indicating that this is the first TCX frame.
    /// - `flags`: Audio coding flags (see common/flags.rs).
    #[expect(clippy::too_many_arguments)]
    pub fn read(
        &mut self,
        k: usize,
        bs: &mut Bitstream,
        spec: &mut [f32],
        arco: &mut ArithDecoderData,
        scratch_buffer: &mut [i16],
        is_1st_frame: bool,
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        self.tcx_info[k].read(bs, spec, arco, scratch_buffer, is_1st_frame, ac_flags)
    }

    /// Resets `TcxData` instance.
    pub fn reset(&mut self) {
        self.last_pitch = L_DIV as u16;
    }

    // Private functions

    /// Adaptive Low Frequencies De-emphasis of spectral coefficients.
    /// Ensures quantization of low frequencies in cases where the
    /// signal's dynamic range is higher than the `LPC` noise shaping.
    ///
    /// # Parameters
    ///
    /// - `x`: Spectral coefficients.
    fn adapt_low_freq_deemph(&mut self, x: &mut [f32]) {
        let i_max = x.len() / 4; // ALFD range = 1600Hz (lg = 6400Hz)

        // Note: This stack array saves temporary accumulation results to be used in a second run.
        // The size is limited to (1024/4)/8=32.
        let mut tmp_pow2 = [0.0f32; 32];

        // Low frequency part of input.
        let x_lf = &mut x[..i_max];
        for (i, this_chunk) in x_lf.chunks_exact(8).enumerate() {
            tmp_pow2[i] = 0.01_f32 + this_chunk.iter().map(|a| a * a).sum::<f32>();
        }
        // Get peak value.
        let max = *tmp_pow2.iter().max_by(|a, b| a.total_cmp(b)).unwrap();

        // Deemphasis of all blocks below the peak.
        let mut fac = 0.1f32;

        // Store gains for FAC.
        for (dst, src) in izip!(&mut self.last_alfd_gains, &tmp_pow2).take(i_max / 8) {
            fac = fac.max((max / *src).sqrt().recip());
            *dst = fac;
        }

        // Multiply with gains.
        for (dst, fac) in izip!(
            x_lf.chunks_exact_mut(8),
            self.last_alfd_gains[..i_max / 8].iter()
        ) {
            dst.iter_mut().for_each(|x| *x *= *fac);
        }
    }

    /// Calculates the TCX gain.
    ///
    /// # Parameters
    ///
    /// - `tcx_global_gain`: TCX global gain value read from bitstream.
    /// - `rms`: Energy value.
    /// - `lg`: Frame length in audio samples.
    fn calc_tcx_gain(tcx_global_gain: u8, rms: f32, lg: usize) -> f32 {
        let tcx_gain = lpd_common::decode_gain(tcx_global_gain);

        // 1/sqrt(rms)
        let rms = rms.sqrt().recip() * lg as f32;

        tcx_gain * rms
    }

    /// Creates comfort noise in spectrum.
    ///
    /// # Parameters
    ///
    /// - `coeffs`: Spectral coefficients.
    /// - `frame`: Index of this ACELP frame.
    fn create_comfort_noise(&mut self, coeffs: &mut [f32], nf_random_seed: &mut u32, frame: usize) {
        let lg = coeffs.len();

        let noise_factor = self.tcx_info[frame].noise_factor;
        let noise_level: f32 = 0.0625 * (8 - noise_factor) as f32;
        let neg_noise_level = -noise_level;

        let nf_bgn = lg / 6;
        let nf_end = lg;

        for chunk in coeffs[nf_bgn..nf_end].chunks_mut(8) {
            // Fill all consecutive zero coeffs in chunk with noise.
            if chunk.iter().all(|&x| x == 0.0f32) {
                for coeff in chunk {
                    *coeff = if generate_random_sign(nf_random_seed) == -1.0 {
                        neg_noise_level
                    } else {
                        noise_level
                    };
                }
            }
        }
    }

    /// FDNS decoding.
    ///
    /// # Parameters
    ///
    /// - `frame`: Index of this ACELP frame.
    /// - `next_frame`: Index of next ACELP frame.
    /// - `r`: Spectral coefficients.
    /// - `a`: Input LPC coefficients of all ACELP frames.
    /// - `fdns_npts`: Number of lines (FDNS_NPTS).
    fn fdns_decode(
        &mut self,
        frame: usize,
        next_frame: usize,
        r: &mut [f32],
        a: &[[f32; M_LP_FILTER_ORDER]; 5],
        fdns_npts: usize,
    ) {
        // Spectral coefficients low frequency de-emphasis.
        self.adapt_low_freq_deemph(r);

        // Calculate energy.
        let energy_rms = 1e-2 + calc_energy(r);

        // Calculate gain.
        self.tcx_info[frame].gain =
            Self::calc_tcx_gain(self.tcx_info[frame].global_gain, energy_rms, r.len());

        // Apply ODFT and Noise Shaping. LP coefficient (A1, A2) weighting is done inside, on the
        // fly.
        Self::lpc2mdct_and_noise_shaping(r, fdns_npts, &a[frame], &a[next_frame]);
    }

    /// Finds pitch for TCX20 (time domain) concealment.
    ///
    /// # Parameters
    ///
    /// - `coeffs`: TCX coefficients.
    fn find_mpitch(&mut self, coeffs: &[f32]) {
        let mut n: u16 = 2;
        let lg = coeffs.len();

        // Find index of max val below 400 Hz, by comparing sums of squares of tuples in the array.
        let coeffs_lf = &coeffs[2usize..lg >> 4];
        n += coeffs_lf
            .iter()
            .tuples()
            .enumerate()
            .max_by(|(_, (tup1_val1, tup1_val2)), (_, (tup2_val1, tup2_val2))| {
                (*tup1_val1 * *tup1_val1 + *tup1_val2 * *tup1_val2)
                    .total_cmp(&(*tup2_val1 * *tup2_val1 + *tup2_val2 * *tup2_val2))
            })
            .map(|(index, _)| 2 * index as u16)
            .unwrap();

        let pitch = 2.0_f32 * lg as f32 / n as f32;

        // Note: For valid lg values, pitch cannot get greater than 256.
        if pitch >= 256.0_f32 {
            self.last_pitch = 256;
        } else {
            let mut mpitch = pitch;
            while mpitch < 256.0 {
                mpitch += pitch;
            }
            self.last_pitch = (mpitch - pitch) as u16;
        }
    }

    /// Interpolated Noise Shaping for mdct coefficients.
    ///
    /// This algorithm shapes temporally the spectral noise between
    /// the two spectral noise representions (FDNS_NPTS of resolution).
    /// The noise is shaped monotonically between the two points
    /// using a curved shape to favor the lower gain in mid-frame.
    /// ODFT and amplitude calculation are applied to the 2 LPC coefficients first.
    ///
    /// # Parameters
    ///
    /// - `r`: Input TCX coefficients / Output time domain signal.
    /// - `fdns_npts`: Number of lines (FDNS_NPTS).
    /// - `a1`: Input LPC coefficients of current TCX frame.
    /// - `a2`: Input LPC coefficients of next TCX frame.
    fn lpc2mdct_and_noise_shaping(
        r: &mut [f32],
        fdns_npts: usize,
        a1: &[f32; M_LP_FILTER_ORDER],
        a2: &[f32; M_LP_FILTER_ORDER],
    ) {
        let mut scratch_1 = [Complex {
            re: 0.0_f32,
            im: 0.0_f32,
        }; FDNS_NPTS * 2];
        let mut scratch_2 = [Complex {
            re: 0.0_f32,
            im: 0.0_f32,
        }; FDNS_NPTS * 2];

        {
            // ODFT. E_LPC_a_weight() for A1 and A2 vectors is included into the loop below.
            let win = match fdns_npts {
                64 => &SINE_TABLE_64[..M_LP_FILTER_ORDER + 1],
                48 => &SINE_TABLE_48[..M_LP_FILTER_ORDER + 1],
                _ => panic!(
                    "No sine table available for FD noise shaping resolution of size {}.",
                    fdns_npts
                ),
            };

            scratch_1[0] = win[0].conj();
            scratch_2[0] = win[0].conj();

            let mut f = 1.0_f32;
            for i in 1..=M_LP_FILTER_ORDER {
                f *= 0.92_f32;
                scratch_1[i] = a1[i - 1] * f * win[i].conj();
                scratch_2[i] = a2[i - 1] * f * win[i].conj();
            }

            fft(&mut scratch_1[..fdns_npts * 2]);
            fft(&mut scratch_2[..fdns_npts * 2]);
        }

        // Get amplitude and apply gains.
        let r_chunk_len = r.len() / fdns_npts;
        let mut last_rr = 0.0_f32;

        for (r_chunk, c1, c2) in izip!(
            r.chunks_mut(r_chunk_len),
            scratch_1.iter(),
            scratch_2.iter()
        ) {
            let g1 = f32::max(c1.norm(), f32::EPSILON).recip();
            let g2 = f32::max(c2.norm(), f32::EPSILON).recip();

            let inv_g1_g2 = (g1 + g2).recip();

            let a = 2.0_f32 * g1 * g2 * inv_g1_g2;
            let b = (g2 - g1) * inv_g1_g2;

            for r_element in r_chunk.iter_mut() {
                last_rr = a * *r_element + b * last_rr;
                *r_element = last_rr;
            }
        }
    }

    /// Returns last gain of TCX.
    pub fn last_gain(&self) -> f32 {
        self.last_gain
    }

    /// Sets last gain of TCX.
    pub fn set_last_gain(&mut self, gain: f32) {
        self.last_gain = gain;
    }
}

#[cfg(test)]
mod tests {
    use itertools::izip;

    use super::TcxData;
    use crate::aac_dec::lpd::common::LpdMode;
    use crate::aac_dec::lpd::constants::M_LP_FILTER_ORDER;

    #[test]
    fn adapt_low_freq_deemph() {
        let mut tcx_data: TcxData = Default::default();

        let coeffs_ref: [f32; 256] = [
            0.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, -1.0,
            0.0, 0.0, -1.0, -1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0,
            1.0, 0.0, 0.0, 0.0, -1.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0,
            0.0, -2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0,
            0.0, 0.0, 0.0, 0.0, -1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0, -1.0, -1.0, 0.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0, 0.0, 0.0, -1.0,
            0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, -1.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.375, 0.375, -0.375,
            -0.375, 0.375, -0.375, 0.375, 0.375, -0.375, 0.375, -0.375, 0.375, 0.375, -0.375,
            0.375, -0.375, -0.375, 0.375, 0.375, 0.375, 0.375, -0.375, -0.375, 0.375, -0.375,
            0.375, 0.375, 0.375, 0.375, 0.375, -0.375, 0.375, 0.375, 0.375, -0.375, 0.375, 0.375,
            -0.375, -0.375, -0.375, 0.375, -0.375, 0.375, -0.375, 0.375, 0.375, -0.375, -0.375,
            0.375, -0.375, 0.375, -0.375, -0.375, -0.375, 0.375, -0.375, 0.375, 0.375, -0.375,
            -0.375, 0.375, 0.375, 0.375, -0.375, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0, -0.375,
            -0.375, 0.375, -0.375, -0.375, 0.375, -0.375, 0.375, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, -0.375, -0.375, -0.375, -0.375, -0.375, -0.375,
        ];

        let coeffs_res: [f32; 256 / 4] = [
            0.0, 0.0, 0.0, 0.0, -0.6334016, 0.6334016, 0.0, 0.0, 0.0, 0.7751118, 0.0, 0.7751118,
            0.0, 0.0, 0.7751118, 0.0, -0.8946503, 0.0, 0.0, -0.8946503, -0.8946503, 0.0,
            -0.8946503, 0.0, 0.0, 0.0, 0.0, 0.0, -0.8946503, 0.0, 0.0, 0.0, 0.0, 0.8946503, 0.0,
            0.0, 0.0, -0.8946503, 0.8946503, -0.8946503, 0.0, -0.8946503, 0.8946503, 0.0, 0.0, 0.0,
            -0.8946503, 0.0, 0.0, 0.0, -2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0,
        ];

        let mut alfd_gains_ref: [f32; 32] = [0.0f32; 32];
        alfd_gains_ref[0..8].copy_from_slice(&[
            0.633401572,
            0.775111794,
            0.89465028,
            0.89465028,
            0.89465028,
            0.89465028,
            1.0,
            1.0,
        ]);
        alfd_gains_ref[8..32].copy_from_slice(&[0.0f32; 24]);

        let mut coeffs: [f32; 256] = [0.0f32; 256];
        coeffs.copy_from_slice(&coeffs_ref);

        tcx_data.adapt_low_freq_deemph(&mut coeffs);

        for (ref_val, dut_val) in izip!(alfd_gains_ref, tcx_data.last_alfd_gains) {
            assert!((ref_val - dut_val).abs() < f32::EPSILON);
        }

        assert!(coeffs[..256 / 4] == coeffs_res);
        assert!(coeffs[256 / 4..] == coeffs_ref[256 / 4..]);
    }

    #[test]
    fn create_comfort_noise() {
        let mut tcx_data: TcxData = Default::default();

        // Stimuli
        const TCX_COEFFS_LEN: usize = 256;

        let mut coeffs = [0.0f32; TCX_COEFFS_LEN];
        let mut nf_random_seed: u32 = 997897179;
        let frame: usize = 0;

        tcx_data.tcx_info[frame].noise_factor = 3;

        // Set coeffs:
        let offset = (TCX_COEFFS_LEN / 6) % 8;
        let neg_offset: isize = offset as isize - 8;
        // Offset to the remainder, i.e. the last bytes that are not accessed as a group of eight
        let remainder_offset = (TCX_COEFFS_LEN as isize + neg_offset) as usize;
        for coeff in coeffs
            .iter_mut()
            .take(TCX_COEFFS_LEN / 6 + 4 * 8 + 3)
            .skip(TCX_COEFFS_LEN / 6 + 3)
        {
            *coeff = 1.0;
        }
        coeffs[remainder_offset - 1] = -1.0;

        // Execute test function:
        tcx_data.create_comfort_noise(&mut coeffs, &mut nf_random_seed, frame);

        // Check results:

        // Must be zero because these coeffs must not be touched:
        assert!(coeffs[0..TCX_COEFFS_LEN / 6] == [0.0f32; TCX_COEFFS_LEN / 6]);

        // Must not be touched because at least one element in each of the four groups
        // of eight is not zero.
        assert!(coeffs[TCX_COEFFS_LEN / 6..TCX_COEFFS_LEN / 6 + 3] == [0.0f32; 3]);
        assert!(coeffs[TCX_COEFFS_LEN / 6 + 3..TCX_COEFFS_LEN / 6 + 4 * 8 + 3] == [1.0f32; 4 * 8]);
        assert!(coeffs[TCX_COEFFS_LEN / 6 + 4 * 8 + 3..TCX_COEFFS_LEN / 6 + 5 * 8] == [0.0f32; 5]);

        // This area was filled with zeroes, should be replaced by noise here:
        for c in coeffs[TCX_COEFFS_LEN / 6 + 5 * 8..remainder_offset - 8].iter() {
            assert!(c.abs() == (0.0625f32 * (8 - tcx_data.tcx_info[frame].noise_factor) as f32));
        }

        // Must be zero because the last element of this 8's group is not zero:
        assert!(coeffs[(remainder_offset - 8)..(remainder_offset - 1)] == [0.0f32; 7]);

        // Must stay -1:
        assert!(coeffs[remainder_offset - 1] == -1.0);

        // The rest must have been replaced by noise:
        for c in coeffs[remainder_offset..TCX_COEFFS_LEN].iter() {
            assert!(c.abs() == (0.0625f32 * (8 - tcx_data.tcx_info[frame].noise_factor) as f32));
        }
    }

    #[test]
    fn find_mpitch() {
        let mut tcx_data: TcxData = Default::default();

        let mut coeffs = [
            0.0f32, -1.0, 0.0, 2.0, 1.0, -3.0, -1.0, 0.0, 1.0, 1.0, 3.0, 1.0, -1.0, -1.0, 0.0, 0.0,
            1.0, -3.0, -2.0, 1.0, 0.0, 0.0, -2.0, 0.0, 1.0, 1.0, 2.0, 3.0, 0.0, 1.0, 0.0, -2.0,
            0.0, 1.0, 1.0, -3.0, -1.0, 1.0, 0.0, -1.0, -4.0, -2.0, 1.0, 0.0, -1.0, 1.0, -3.0, 0.0,
            -1.0, -1.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0, 3.0, 0.0, 2.0, 1.0, 1.0, -2.0, -1.0, -2.0,
            0.0, 1.0, 0.0, 1.0, 1.0, -1.0, 2.0, 2.0, -1.0, -1.0, -1.0, 0.0, 1.0, -2.0, 1.0, -1.0,
            2.0, -3.0, -1.0, 0.0, 1.0, 0.0, -1.0, -1.0, 3.0, 0.0, 2.0, 0.0, -1.0, 1.0, -1.0, -1.0,
            0.0, 0.0, 2.0, 0.0, -1.0, -2.0, 0.0, 0.0, 2.0, 1.0, -1.0, -1.0, 1.0, -2.0, -1.0, -3.0,
            2.0, 0.0, -1.0, 0.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 2.0, -1.0, 0.0, -1.0, -1.0,
            0.0, 2.0, 1.0, 0.0, -2.0, 0.0, 0.0, 0.0, 1.0, -3.0, 1.0, 0.0, 2.0, -1.0, -1.0, 1.0,
            0.0, 0.0, 0.0, 0.0, -3.0, 1.0, 0.0, 1.0, 0.0, 3.0, 1.0, -1.0, 1.0, -1.0, 1.0, 0.0, 0.0,
            0.0, -2.0, 0.0, 1.0, 1.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0, -1.0, 0.0, 0.0, 1.0,
            -1.0, 1.0, 0.0, -1.0, 1.0, -1.0, 0.0, -2.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0,
            0.0, -1.0, 1.0, -1.0, -2.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0, 0.0, -2.0, 1.0, 1.0,
            1.0, -1.0, 1.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -3.0, -1.0, -1.0, 2.0, 2.0,
            -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, -1.0, 1.0, 1.0, 0.0,
            0.0, 0.0, 0.0, -2.0, 0.0, 0.0, -1.0, 0.0, -1.0, 0.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0,
            -1.0, 0.0, -1.0, 1.0, -2.0, -1.0, -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            -1.0, 1.0, 1.0, 1.0, 0.0, 1.0, -1.0, 0.0, 0.0, -2.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0,
            -1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, -1.0, 1.0, -1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0,
            -1.0, 1.0, 0.0, -1.0, 1.0, 1.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0, -1.0, 1.0,
            0.0, 0.0, 2.0, 0.0, -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, -1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
            0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 1.0, 0.0, 1.0,
            1.0, 0.0, -2.0, 0.0, 0.0, 0.0, 1.0, -1.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0,
            -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, -1.0, 2.0, -1.0, -1.0,
            -1.0, 1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 0.0, 1.0,
            0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, -1.0, 0.0, 0.0, 1.0, -1.0, 0.0, -1.0,
            1.0, -1.0, 0.0, -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0,
            -1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, -1.0, 0.0, 0.0, 0.0, -1.0, 1.0, -1.0, -1.0, 0.0,
            -1.0, -1.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, -1.0, 1.0, 0.0, -1.0, 0.0,
            1.0, -1.0, 0.0, -2.0, -1.0, 0.0, 1.0, -2.0, 0.0, 1.0, 0.0, 0.0, 2.0, -1.0, 1.0, 0.0,
            0.0, -1.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, -1.0, -1.0, 1.0,
            1.0, -2.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        ];

        tcx_data.find_mpitch(&coeffs);
        assert!(tcx_data.last_pitch == 204);

        coeffs[2] = 5.0;
        tcx_data.find_mpitch(&coeffs);
        assert!(tcx_data.last_pitch == 256);
    }

    #[test]
    fn decode() {
        let mut tcx_data: TcxData = Default::default();

        // stimuli
        #[rustfmt::skip]
        const LSP_COEFFS: [[[f32; M_LP_FILTER_ORDER]; 5]; 1] = [
            [
                [0.981501937,0.933061481,0.820757091,0.701940715,0.556541741,0.397402704,0.202517852,0.0152972667,-0.180541918,-0.367029428,-0.529337406,-0.667762995,-0.786592007,-0.873870611,-0.956118822,-0.990257442],
                [0.977786005,0.922106385,0.80881542,0.694516957,0.55100286,0.392754704,0.198622525,0.0144161638,-0.179302573,-0.364903897,-0.526001036,-0.664451659,-0.784106076,-0.871904671,-0.954301059,-0.988964081],
                [0.97373426,0.910357416,0.796530485,0.687018394,0.545439661,0.38809678,0.194724053,0.0135350497,-0.178062841,-0.362776399,-0.5226565,-0.661127031,-0.781607687,-0.869924366,-0.952447176,-0.987590611],
                [0.97373426,0.910357416,0.796530485,0.668107271,0.522502661,0.334691793,0.166403919,-0.0145772146,-0.192223638,-0.375603944,-0.533780873,-0.670800984,-0.789268076,-0.875706732,-0.948848188,-0.985563517],
                [0.97373426,0.910357416,0.796530485,0.648754358,0.499181032,0.28018856,0.137945831,-0.0426779576,-0.206344426,-0.388359845,-0.544813395,-0.680362165,-0.796807408,-0.881366253,-0.945121467,-0.983384371],
            ],
        ];

        #[rustfmt::skip]
        const LP_COEFFS_IN_THIS: [f32; 16] =
        [
            0.742490232,  0.0966611505,-0.00601732731, 0.00487732887,-0.00562047958,-0.0994597077,-0.0532849431,-0.0701349974,
           -0.0125170946, 0.0134738088,  0.0493193269,  0.0476083755,  0.0355670452, 0.0683313608, 0.0426821113, 0.0400534272,
        ];

        #[rustfmt::skip]
        const LP_COEFFS_OUT_REF_NEXT: [f32; 16] =
        [
            0.773913920,  0.212655425,  0.111046135,  0.103870273,  0.0704214573,-0.0348611474,-0.0191773772,-0.0519524217,
           -0.0127791762, 0.0131654143, 0.0464330316, 0.0487658978, 0.0357013941, 0.0718967319, 0.0441479683, 0.0410540700,
        ];

        #[rustfmt::skip]
        const SIG_OUT_REF: [f32; 256] =
        [
            0.00000000, -0.367373079, 0.0387325697, -0.371456712, 0.0387179032, -0.00403566798, 0.000420648197, -4.38452589e-05,
            4.41940074e-06, -4.45455271e-07, 4.48998421e-08, -4.52569759e-09, 4.23385244e-10, 0.390924305, 0.354352802, -0.0331501924,
            0.398023039, -0.427830219, 0.0349502824, -0.00285515655, 0.000189785584, -1.26152690e-05, 8.38551671e-07, -5.57395090e-08,
            2.82622281e-09, -1.43301149e-10, 7.26595867e-12, -3.68414040e-13, 1.35351435e-14, -4.97266944e-16, 1.82690647e-17, -6.71186188e-19,
            1.70075717e-20, -4.30964633e-22, 1.09204603e-23, -2.76719808e-25, 4.45194345e-27, -7.16240768e-29, 1.15230758e-30, -0.370347679,
            0.00302366493, 0.532161593, -0.00434477767, 3.54724834e-05, -0.550478518, -0.550013661, -0.550014079, 0.000464404555,
            0.576026022, 0.579615712, 0.00361499633, 2.25463154e-05, 2.91724717e-07, 3.77459930e-09, 0.605190217, 0.00783050060,
            0.000146104081, 2.72605848e-06, -0.197948083, -0.201641515, 0.200780913, 0.209919840, 0.210127383, -0.200588197,
            -0.216071337, -0.216456249, 0.205703408, 0.216198504, -0.210149378, -0.221025735, -0.221303210, 0.210019141,
            -0.214520797, 0.214406520, 0.00544302864, 0.000138179385, 0.716796815, 0.0180134736, 0.717246056, 0.0180247631,
            0.000452112465, 1.13402702e-05, 2.84446315e-07, 7.13472481e-09, 0.738653779, 0.0186682548, 0.000471809326, 1.19242022e-05,
            3.04667878e-07, 7.78437936e-09, 1.98893846e-10, 0.744188726, 0.0191751719, 0.000494077918, 1.27306803e-05, 3.28025635e-07,
            8.49664428e-09, 2.20083299e-10, 5.70068125e-12, 1.47661207e-13, 3.84771365e-15, 0.753578067, 0.0196365230, 0.000511682883,
            1.34882948e-05, 3.55560275e-07, -0.238743737, -0.245037198, -0.250495732, 0.237177894, 0.250284523, -0.237183571,
            0.244084910, 0.257342488, 0.00708905281, 0.000195283224, 5.50538562e-06, 1.55206735e-07, 4.37555725e-09, 1.23354840e-10,
            0.848709047, 0.0242425110, 0.000692462665, 0.848728836, -0.844998240, -0.0240642726, -0.000685314124, -1.95167104e-05,
            -5.44767715e-07, -1.52060391e-08, -0.277348548, -0.285090148, -0.290853202, -0.291008830, -0.291013032, 0.275296658,
            0.297110438, -0.282257408, 0.282642812, 0.297301084, -0.291111976, 0.291266173, -0.291262329, -0.305776209,
            0.301816136, 0.316393226, 0.316742957, -0.301553041, -0.328818172, -0.329450339, 0.314187974, 0.329111129,
            -0.328892857, -0.343662560, 0.328566253, 0.343655229, -0.344771922, -0.359702021, -0.360025793, 0.344416887,
            0.376620322, 0.377290428, 0.377304375, -0.361601710, -0.394897699, -0.395554185, 0.379969537, 0.395259857,
            0.414033443, -0.399196088, 0.399468094, 0.414110601, -0.418938786, 0.418858409, 0.432803720, -0.418627650,
            0.437980920, -0.437695891, -0.450592220, 0.437510163, 0.466987610, -0.455456704, 0.455603689, 0.467218280,
            -0.473050833, 0.472987235, -0.472987950, -0.483299166, 0.491002858, 0.500058532, 0.500142694, 0.500143528,
            -0.510387421, 0.510306478, 0.00403426308, -1.64586043, -0.0108481087, -7.15014830e-05, -4.71276792e-07, -3.10625481e-09,
            -1.61361150e-11, -8.38225499e-14, -0.552308321, -0.555177391, 0.563028038, -0.562999070, -0.567152441, -0.567167759,
            0.568996012, 0.571552515, 0.00128603179, -1.82486820, -0.00208279537, -1.82348406, -0.00208121561, -2.37537483e-06,
            -1.14071985e-09, -1.82179725, 1.82092237, 1.82267165, -1.83992445, 1.83992219, -1.83992219, -1.84040666,
            0.000519282883, 1.89467096, 1.89413655, -1.89520550, 0.00263867574, -3.67380176e-06, 5.11499731e-09, -7.12155942e-12,
            2.93595816e-14, -1.21038811e-16, 4.98998721e-19, -2.05718910e-21, 1.95544293e-23, -2.30945444, 2.33140683, -0.0221609809,
            0.000395543466, -7.05991488e-06, 0.772446275, -0.786233246, -0.786379457, -0.786375403, -0.786375523, -0.786375523,
            -0.796669304, 0.853471041, -0.0306236632, 2.64073253, 2.54765511, -0.102930076, 0.00415856950, -0.000168014056
        ];

        const TCX_COEFFS_LEN: usize = 256;
        let mut io_buf = [0.0f32; TCX_COEFFS_LEN];

        let mut lp_coeffs = [[0.0f32; M_LP_FILTER_ORDER]; 5];
        let mut nf_random_seed: u32 = 997897179;
        let last_mod: LpdMode = LpdMode::Tcx20;
        let fdns_npts: usize = 64;
        let this_frame: usize = 0;
        let next_frame: usize = 1;

        // Initialize arguments.
        for i in [
            13, 14, 16, 41, 48, 49, 54, 76, 78, 84, 91, 101, 120, 123, 218, 219, 221, 225, 226,
            238, 251, 252,
        ] {
            io_buf[i] = 1.0;
        }

        for i in [
            1, 3, 17, 39, 44, 45, 46, 124, 195, 211, 213, 217, 220, 222, 223, 227, 237,
        ] {
            io_buf[i] = -1.0;
        }

        // Only LSP coefficients of next_frame will be converted
        // since last_mod != LpdMode::Acelp.
        let lsp_coeffs = LSP_COEFFS[0];
        lp_coeffs[this_frame].copy_from_slice(&LP_COEFFS_IN_THIS);

        tcx_data.last_alfd_gains[0] = 0.817174435;
        tcx_data.last_alfd_gains[1] = 0.817174435;
        tcx_data.last_alfd_gains[2] = 0.817174435;
        tcx_data.last_alfd_gains[3] = 0.817174435;

        tcx_data.last_alfd_gains[4] = 1.0;
        tcx_data.last_alfd_gains[5] = 1.0;
        tcx_data.last_alfd_gains[6] = 1.0;
        tcx_data.last_alfd_gains[7] = 1.0;

        tcx_data.tcx_info[this_frame].noise_factor = 3;

        tcx_data.tcx_info[this_frame].global_gain = 71;

        // Run DUT.
        tcx_data.decode(
            &mut io_buf,
            &lsp_coeffs,
            &mut lp_coeffs,
            &mut nf_random_seed,
            last_mod,
            fdns_npts,
            next_frame,
            this_frame,
        );

        // Compare LP coeffs against reference.
        for (c, r) in izip!(lp_coeffs[this_frame], LP_COEFFS_IN_THIS) {
            assert!((c - r).abs() < f32::EPSILON);
        }
        for (c, r) in izip!(lp_coeffs[next_frame], LP_COEFFS_OUT_REF_NEXT) {
            assert!((c - r).abs() < f32::EPSILON);
        }

        // Compare output against reference.
        for (c, r) in izip!(io_buf, SIG_OUT_REF) {
            assert!((c - r).abs() < 1E-6);
        }
    }
}
