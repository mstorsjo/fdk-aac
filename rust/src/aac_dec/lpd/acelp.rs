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
//! Algebraic Code Excited Linear Prediction (ACELP) decoding

use crate::aac_dec::error_codes::AacDecoderError;
use crate::aac_dec::lpd::common::LpdMode;
use crate::aac_dec::lpd::constants::*;
use crate::aac_dec::lpd::lpc;
use crate::common::bitstream::Bitstream;
use itertools::izip;

/// Minimum pitch lag with resolution 1.
const PIT_FR1_12K8: u16 = 160;
/// Minimum pitch lag with resolution 1/2.
const PIT_FR2_12K8: u16 = 128;
/// Frequency scale denominator
const FSCALE_DENOM: u32 = 12800;

/// ACELP synth pre-emphasis factor
const PREEMPH_FAC: f32 = 0.68;
/// ACELP code pre-emphasis factor ( *2 )
const TILT_CODE2: f32 = 0.3;
/// Offset from past to current data in exc and synth buffer.
const BUFFER_OFFSET: usize = PIT_MAX_MAX + L_INTERPOL;

static POW_10_MEAN_ENERGY: [f32; 4] = [7.94328234, 31.6227766, 125.89254117, 501.18723362];

/// Table for decoding adaptive codebook gain (scaled by 2.0f) and
/// innovative codebook gain (scaled by 16.0f).
/// Table entries are pairs of adaptive/innovative codebook gains.
#[rustfmt::skip]
static T_QUA_GAIN7B: [f32; 128 * 2] = [
    0.012445, 0.215546,  0.028326, 0.965442,  0.053042, 0.525819,
    0.065409, 1.495322,  0.078212, 2.323725,  0.100504, 0.751276,
    0.112617, 3.427530,  0.113124, 0.309583,  0.121763, 1.140685,
    0.143515, 7.519609,  0.162430, 0.568752,  0.164940, 1.904113,
    0.165429, 4.947562,  0.194985, 0.855463,  0.213527, 1.281019,
    0.223544, 0.414672,  0.243135, 2.781766,  0.257180, 1.659565,
    0.269488, 0.636749,  0.286539, 1.003938,  0.328124, 2.225436,
    0.328761, 0.330278,  0.336807, 11.500983, 0.339794, 3.805726,
    0.344454, 1.494626,  0.346165, 0.738748,  0.363605, 1.141454,
    0.398729, 0.517614,  0.415276, 2.928666,  0.416282, 0.862935,
    0.423421, 1.873310,  0.444151, 0.202244,  0.445842, 1.301113,
    0.455671, 5.519512,  0.484764, 0.387607,  0.488696, 0.967884,
    0.488730, 0.666771,  0.508189, 1.516224,  0.508792, 2.348662,
    0.531504, 3.883870,  0.548649, 1.112861,  0.551182, 0.514986,
    0.564397, 1.742030,  0.566598, 0.796454,  0.589255, 3.081743,
    0.598816, 1.271936,  0.617654, 0.333501,  0.619073, 2.040522,
    0.625282, 0.950244,  0.630798, 0.594883,  0.638918, 4.863197,
    0.650102, 1.464846,  0.668412, 0.747138,  0.669490, 2.583027,
    0.683757, 1.125479,  0.691216, 1.739274,  0.718441, 3.297789,
    0.722608, 0.902743,  0.728827, 2.194941,  0.729586, 0.633849,
    0.730907, 7.432957,  0.731017, 0.431076,  0.731543, 1.387847,
    0.759183, 1.045210,  0.768606, 1.789648,  0.771245, 4.085637,
    0.772613, 0.778145,  0.786483, 1.283204,  0.792467, 2.412891,
    0.802393, 0.544588,  0.807156, 0.255978,  0.814280, 1.544409,
    0.817839, 0.938798,  0.826959, 2.910633,  0.830453, 0.684066,
    0.833431, 1.171532,  0.841208, 1.908628,  0.846440, 5.333522,
    0.868280, 0.841519,  0.868662, 1.435230,  0.871449, 3.675784,
    0.881317, 2.245058,  0.882020, 0.480249,  0.882476, 1.105804,
    0.902856, 0.684850,  0.904419, 1.682113,  0.909384, 2.787801,
    0.916558, 7.500981,  0.918444, 0.950341,  0.919721, 1.296319,
    0.940272, 4.682978,  0.940273, 1.991736,  0.950291, 3.507281,
    0.957455, 1.116284,  0.957723, 0.793034,  0.958217, 1.497824,
    0.962628, 2.514156,  0.968507, 0.588605,  0.974739, 0.339933,
    0.991738, 1.750201,  0.997210, 0.936131,  1.002422, 1.250008,
    1.006040, 2.167232,  1.008848, 3.129940,  1.014404, 5.842819,
    1.027798, 4.287319,  1.039404, 1.489295,  1.039628, 8.947958,
    1.043214, 0.765733,  1.045089, 2.537806,  1.058994, 1.031496,
    1.060415, 0.478612,  1.072132, 12.8,      1.074778, 1.910049,
    1.076570, 15.9999,   1.107853, 3.843067,  1.110673, 1.228576,
    1.110969, 2.758471,  1.140058, 1.603077,  1.155384, 0.668935,
    1.176229, 6.717108,  1.179008, 2.011940,  1.187735, 0.963552,
    1.199569, 4.891432,  1.206311, 3.316329,  1.215323, 2.507536,
    1.223150, 1.387102,  1.296012, 9.684225];

/// Factor table for interpolation of LPC coeffs in LSP domain.
const LSP_INTERPOL_FACTOR: [[f32; 4]; 2] = [
    [0.125, 0.375, 0.625, 0.875],   // for coreCoderFrameLength = 1024
    [0.166667, 0.5, 0.833333, 0.0], // for coreCoderFrameLength = 768
];

/// Map subframe number to number of bits. Used for acb_index.
const NUM_ACB_IDX_BITS: [[u8; NB_SUBFR]; 2] = {
    [
        [9, 6, 9, 6], // coreCoderFrameLength == 1024
        [9, 6, 6, 0], // coreCoderFrameLength == 768
    ]
};

const TAB_COREMODE2NBITS: [u8; 8] = [20, 28, 36, 44, 52, 64, 12, 16];

// START ACE_LTP
const A2: f32 = 0.18;
const B: f32 = 0.64;
const L_INTERPOL2: usize = 16;

static PRED_LT4_INTER4_2_0: [f32; L_INTERPOL2 + 1] = [
    0.940000, 0.059072, -0.056359, 0.052062, -0.046497, 0.040059, -0.033186, 0.026318, -0.019855,
    0.014122, -0.009344, 0.005635, -0.002993, 0.001315, -0.000417, 0.000063, 0.000000,
];

static PRED_LT4_INTER4_2_1: [f32; L_INTERPOL2] = [
    0.337560, -0.158569, 0.103705, -0.073660, 0.053143, -0.037767, 0.025901, -0.016813, 0.010080,
    -0.005363, 0.002330, -0.000627, -0.000116, 0.000259, -0.000133, 0.000007,
];

static PRED_LT4_INTER4_2_2: [f32; L_INTERPOL2] = [
    0.632268, -0.199393, 0.106749, -0.063705, 0.038227, -0.021674, 0.010702, -0.003645, -0.000530,
    0.002594, -0.003182, 0.002844, -0.002044, 0.001151, -0.000434, 0.000048,
];

static PRED_LT4_INTER4_2_3: [f32; L_INTERPOL2] = [
    0.856390, -0.131059, 0.047606, -0.015182, -0.000983, 0.009308, -0.013028, 0.013821, -0.012766,
    0.010657, -0.008101, 0.005562, -0.003362, 0.001692, -0.000618, 0.000098,
];
// END ACE_LTP

/// Structure which holds the ACELP internal persistent memory.
#[derive(Default, Debug)]
#[repr(C)]
pub struct AcelpData {
    old_syn_mem: [f32; M_LP_FILTER_ORDER], // Synthesis filter states
    a: [f32; M_LP_FILTER_ORDER],
    gc_threshold: f32,
    pub de_emph_mem: f32,
    past_gpit: f32,
    past_gcode: f32,
    old_t0: u16,
    old_t0_frac: u8,
    deemph_mem_wsyn: f32,
    pub wsyn_rms: f32,
    seed_ace: i16,
    acelp_info: [AcelpInfo; NB_DIV],
}

/// Structure which holds the parameter data needed to decode one ACELP frame.
#[derive(Default, Debug)]
#[repr(C)]
struct AcelpInfo {
    /// ACELP core mode bitfield for whole ACELP frame.
    acelp_core_mode: u8,
    /// Mean excitation energy index for whole ACELP frame.
    mean_energy: u8,
    t0: [u16; NB_SUBFR],
    t0_frac: [u8; NB_SUBFR],
    /// Controls whether LTP postfilter is active for each ACELP subframe.
    ltp_filtering_flag: [u8; NB_SUBFR],
    /// Innovative codebook index for each ACELP subframe.
    icb_index: [[usize; 8]; NB_SUBFR],
    /// Gain index for each ACELP subframe.
    gains: [usize; NB_SUBFR],
    pitch_lag_offset: i16,
}

impl AcelpInfo {
    /// Decodes pitch lag.
    fn decode_pitch_lag(
        bs: &mut Bitstream,
        num_acb_idx_bits: u8,
        pit_vars: &[u16],
        t0_out: &mut u16,
        t0_frac_out: &mut u8,
        t0_min_out: &mut u16,
        t0_max_out: &mut u16,
    ) -> AacDecoderError {
        debug_assert!((num_acb_idx_bits == 9) || (num_acb_idx_bits == 6));

        let pit_min = pit_vars[0];
        let pit_max = pit_vars[1];
        let pit_fr1 = pit_vars[2];
        let pit_fr2 = pit_vars[3];

        let t0;
        let t0_frac;

        let mut acb_idx = bs.read(num_acb_idx_bits) as u16;

        if num_acb_idx_bits == 6 {
            // When the pitch value is encoded on 6 bits, a pitch resolution of 1/4 is
            // always used in the range [T1-8, T1+7.75], where T1 is nearest integer to
            // the fractional pitch lag of the previous subframe.
            t0 = *t0_min_out + acb_idx / 4;
            t0_frac = (acb_idx & 0x3) as u8;
        } else {
            // num_acb_idx_bits == 9
            // When the pitch value is encoded on 9 bits, a fractional pitch delay is
            // used with resolutions 0.25 in the range [TMIN, TFR2-0.25], resolutions
            // 0.5 in the range [TFR2, TFR1-0.5], and integers only in the range [TFR1,
            // TMAX]. NOTE: for small sampling rates TMAX can get smaller than TFR1.
            if acb_idx < (pit_fr2 - pit_min) * 4 {
                // First interval with 0.25 pitch resolution
                t0 = pit_min + (acb_idx / 4);
                t0_frac = (acb_idx & 0x3) as u8;
            } else if acb_idx < ((pit_fr2 - pit_min) * 4 + (pit_fr1 - pit_fr2) * 2) {
                // Second interval with 0.5 pitch resolution.
                acb_idx -= (pit_fr2 - pit_min) * 4;
                t0 = pit_fr2 + (acb_idx / 2);
                t0_frac = ((acb_idx & 0x1) * 2) as u8;
            } else {
                // Third interval with 1.0 pitch resolution.
                t0 = acb_idx + pit_fr1 - ((pit_fr2 - pit_min) * 4) - ((pit_fr1 - pit_fr2) * 2);
                t0_frac = 0;
            }
            // Find T0_min and T0_max for subframe 1 or 3
            *t0_max_out = (t0 + 7).clamp(pit_min + 15, pit_max);
            *t0_min_out = *t0_max_out - 15;
        }
        *t0_out = t0;
        *t0_frac_out = t0_frac;

        AacDecoderError::Ok
    }

    /// Initializes ACELP info structure.
    fn init(&mut self, sampling_rate: u32) {
        // Allowed sample rates for LPD are [6000; 24000]. Therefore we can guarantee a
        // value range of [-17; 30] for the pitch lag offset.
        let sampling_rate_clamped = sampling_rate.clamp(6000, 24000);
        self.pitch_lag_offset = ((sampling_rate_clamped * PIT_MIN_12K8 as u32 + (FSCALE_DENOM / 2))
            / FSCALE_DENOM) as i16
            - PIT_MIN_12K8 as i16;
    }

    /// Maps core mode to n bits.
    fn map_core_mode_to_n_bits(&self) -> u8 {
        TAB_COREMODE2NBITS[self.acelp_core_mode as usize]
    }

    /// Reads ACELP frame and store information in ACELP info structure.
    fn read(
        &mut self,
        bs: &mut Bitstream,
        acelp_core_mode: u8,
        core_coder_frame_length: usize,
    ) -> AacDecoderError {
        let nb_subfr = core_coder_frame_length / L_DIV;
        let num_acb_index_bits: [u8; NB_SUBFR] = if nb_subfr == 4 {
            NUM_ACB_IDX_BITS[0]
        } else {
            NUM_ACB_IDX_BITS[1]
        };

        // Casting pitch variables with as is safe because the pitch_lag_offset
        // always is in the range of -17 to 30 (see init()).
        let pit_min = ((PIT_MIN_12K8 as i16) + self.pitch_lag_offset) as u16;
        let pit_fr2 = ((PIT_FR2_12K8 as i16) - self.pitch_lag_offset) as u16;
        let pit_fr1 = PIT_FR1_12K8;
        let pit_max = ((PIT_MAX_12K8 as i16) + 6 * self.pitch_lag_offset) as u16;
        let mut t0: u16 = 0;
        let mut t0_frac: u8 = 0;
        let mut t0_min: u16 = 0;
        let mut t0_max: u16 = u16::MAX;

        self.acelp_core_mode = acelp_core_mode;

        // Decode mean energy with 2 bits : 18, 30, 42 or 54 dB
        self.mean_energy = bs.read(2) as u8;

        let n_bits = self.map_core_mode_to_n_bits();
        for (sfr, num_acb_index_bits_element) in
            num_acb_index_bits.iter().enumerate().take(nb_subfr)
        {
            // Read ACB index and store T0 and T0_frac for each ACELP subframe.
            let error_status = AcelpInfo::decode_pitch_lag(
                bs,
                *num_acb_index_bits_element,
                &[pit_min, pit_max, pit_fr1, pit_fr2],
                &mut t0,
                &mut t0_frac,
                &mut t0_min,
                &mut t0_max,
            );
            if error_status != AacDecoderError::Ok {
                return error_status;
            }
            self.t0[sfr] = t0;
            self.t0_frac[sfr] = t0_frac;
            self.ltp_filtering_flag[sfr] = bs.read_bit() as u8;

            let icb_bits: &[u8] = match n_bits {
                12 => &[1, 5, 1, 5],                 // 12 bits AMR-WB codebook is used
                16 => &[1, 5, 5, 5],                 // 16 bits AMR-WB codebook is used
                20 => &[5, 5, 5, 5],                 // 20 bits AMR-WB codebook is used
                28 => &[9, 9, 5, 5],                 // 28 bits AMR-WB codebook is used
                36 => &[9, 9, 9, 9],                 // 36 bits AMR-WB codebook is used
                44 => &[13, 13, 9, 9],               // 44 bits AMR-WB codebook is used
                52 => &[13, 13, 13, 13],             // 52 bits AMR-WB codebook is used
                64 => &[2, 2, 2, 2, 14, 14, 14, 14], // 64 bits AMR-WB codebook is used
                _ => &[],
            };
            for (i, num_bits) in icb_bits.iter().enumerate() {
                self.icb_index[sfr][i] = bs.read(*num_bits) as usize;
            }

            self.gains[sfr] = bs.read(7) as usize;
        }

        AacDecoderError::Ok
    }
}

impl AcelpData {
    // API methods / associated functions

    /// Decodes ACELP frame.
    ///
    /// # Parameters
    ///
    /// - `lsp_old`: LPC vector (LSP domain) corresponding to the beginning of current ACELP frame.
    /// - `lsp_new`: LPC vector (LSP domain) corresponding to the end of current ACELP frame.
    /// - `old_exc_mem`: Old excitation memory buffer.
    /// - `stab_fac`: Stability factor.
    /// - `num_lost_subframes`: Number of lost subframes.
    /// - `last_lpc_lost`: Flag indicating LPC4 of previous frame was lost.
    /// - `frame`: ACELP frame number (0-2 for frame length 768, 0-3 for 1024).
    /// - `synth`: Points to end+1 of past valid synthesis signal.
    /// - `p_t`: Rounded pitch t0/t0_frac values of each subframe.
    /// - `pit_gains`: Pitch gain values of each subframe.
    #[expect(clippy::too_many_arguments)]
    pub fn decode(
        &mut self,
        lsp_old: &[f32],
        lsp_new: &[f32],
        old_exc_mem: &mut [f32],
        stab_fac: f32,
        num_lost_subframes: i32,
        last_lpc_lost: bool,
        frame: usize,
        synth: &mut [f32],
        p_t: &mut [u16],
        pit_gains: &mut [f32],
    ) {
        // ACELP frame length.
        let l_div = synth.len();
        let n_subframes = p_t.len();

        // Integer part of the pitch lag
        let mut t0 = 0;
        // Fractional part of the pitch lag.
        let mut t0_frac = 0;

        // Rounded version of pitch t0/t0_frac.
        let mut t;

        // Maximum pitch lag.
        let pit_max = (PIT_MAX_12K8 as i16 + 6 * self.acelp_info[frame].pitch_lag_offset) as u16;

        let mut a = [0.0f32; M_LP_FILTER_ORDER];
        let mut code = [0.0f32; L_SUBFR];
        let mut exc2 = [0.0f32; L_SUBFR];

        let bfi = num_lost_subframes > 0; // Bad frame indicator.

        let mut syn_buf = [0.0f32; M_LP_FILTER_ORDER + L_DIV];
        syn_buf[..M_LP_FILTER_ORDER].copy_from_slice(&self.old_syn_mem[..M_LP_FILTER_ORDER]);

        let mut exc_buf = [0.0f32; BUFFER_OFFSET + L_DIV + 1];
        exc_buf[..BUFFER_OFFSET].copy_from_slice(&old_exc_mem[..BUFFER_OFFSET]);
        exc_buf[BUFFER_OFFSET..].fill(0.0);

        // Start of subframe loop.
        for (subfr_nr, subfr_pos) in (0..l_div).step_by(L_SUBFR).enumerate() {
            // Decode pitch lag (t0 and t0_frac).
            if bfi {
                self.conceal_pitch_lag(pit_max, &mut t0, &mut t0_frac);
            } else {
                t0 = self.acelp_info[frame].t0[subfr_nr];
                t0_frac = self.acelp_info[frame].t0_frac[subfr_nr];
            }

            // Find the pitch gain, the interpolation filter and the adaptive codebook vector.
            Self::pred_lt4(&mut exc_buf[subfr_pos..], t0, t0_frac);

            if (!bfi && self.acelp_info[frame].ltp_filtering_flag[subfr_nr] == 0)
                || (bfi && num_lost_subframes == 1 && stab_fac < 0.25)
            {
                // Find pitch excitation with LTP filter.
                // The postfilter looks 1 sample in the past.
                Self::pred_lt4_postfilter(&mut exc_buf[BUFFER_OFFSET - 1 + subfr_pos..]);
            }

            // Decode innovative codebook.
            if bfi {
                for code_element in code.iter_mut() {
                    *code_element = self.gen_rnd() / 16.0;
                }
            } else {
                let n_bits = self.acelp_info[frame].map_core_mode_to_n_bits();
                Self::decode_4t64(
                    &self.acelp_info[frame].icb_index[subfr_nr],
                    n_bits,
                    &mut code,
                );
            }

            code.iter_mut()
                .for_each(|code_element| *code_element /= 512.0);

            t = t0;
            if t0_frac > 2 {
                t += 1;
            }

            // Pre-emphasis filter.
            Self::preemph_code(&mut code);

            // Pitch sharpener.
            Self::pitch_sharp(&mut code, t.into());

            // Output pitch lag for bass post-filter.
            if t > pit_max {
                p_t[subfr_nr] = pit_max;
            } else {
                p_t[subfr_nr] = t;
            }

            // Decode adaptive and innovative codebook gains.
            let (mut pit_gain, code_gain, code_ener) = self.decode_gains(
                self.acelp_info[frame].gains[subfr_nr],
                &code,
                self.acelp_info[frame].mean_energy,
                bfi,
            );

            pit_gains[subfr_nr] = pit_gain;

            // Signal periodicity measure.
            let period_fac = Self::calc_period_factor(
                &exc_buf[BUFFER_OFFSET + subfr_pos..],
                pit_gain,
                code_ener,
                code_gain,
            );

            if last_lpc_lost && frame == 0 {
                pit_gain = pit_gain.min(1.0);
            }

            // Gain smoothing for noise enhancement.
            let code_gain_smoothed = self.noise_enhancer(code_gain, period_fac, stab_fac);

            // Build adaptive excitation.
            Self::build_adaptive_excitation(
                &code,
                &mut exc_buf[BUFFER_OFFSET + subfr_pos..],
                pit_gain,
                code_gain,
                code_gain_smoothed,
                period_fac,
                &mut exc2,
            );

            // Interpolate LSP vector.
            Self::interpolate_lsp(lsp_old, lsp_new, subfr_nr, n_subframes, &mut a);

            // LP synthesis.
            Self::syn_filt(
                &a,
                &exc2,
                &mut syn_buf[subfr_pos..M_LP_FILTER_ORDER + subfr_pos + L_SUBFR],
            );
        }
        // End of subframe loop.

        // Update pitch value for bfi procedure.
        self.old_t0_frac = t0_frac;
        self.old_t0 = t0;

        // Save old excitation and old synthesis memory for next ACELP frame. Check sizes.
        old_exc_mem.copy_from_slice(&exc_buf[l_div..BUFFER_OFFSET + l_div]);
        self.old_syn_mem
            .copy_from_slice(&syn_buf[l_div..l_div + M_LP_FILTER_ORDER]);

        // Deemphasis.
        Self::deemph(
            &syn_buf[M_LP_FILTER_ORDER..M_LP_FILTER_ORDER + l_div],
            synth,
            &mut self.de_emph_mem,
        );

        self.deemph_mem_wsyn = self.de_emph_mem;
    }

    /// Initializes ACELP data structure.
    ///
    /// The sampling rate is needed for the calculation of the pitch lag offset.
    pub fn init(&mut self, sampling_rate: u32) {
        for acelp_info in self.acelp_info.iter_mut() {
            acelp_info.init(sampling_rate);
        }
    }

    /// Creates a new instance of the ACELP data structure.
    pub fn new() -> AcelpData {
        Default::default()
    }

    /// ACELP post-processing.
    pub fn post_processing(
        synth_buf: &[f32],
        old_synth: &mut [f32],
        pitch: &[u16],
        old_t_pf: &mut [u16],
    ) {
        // Store last part of synth_buf (which is not handled by the IMDCT overlap) for next frame.
        old_synth[..synth_buf.len()].copy_from_slice(synth_buf);

        // For bass postfilter:
        old_t_pf.copy_from_slice(pitch);
    }

    /// Initializes ACELP internal memory in case of FAC before ACELP decoder is called.
    ///
    /// # Parameters
    ///
    /// - `old_exc_mem`: Old excitation memory buffer.
    /// - `synth`: Points to end+1 of past valid synthesis signal.
    /// - `cur_and_last_lpd_modes`: Current and last lpd modes.
    /// - `a_new`: LP synthesis filter coeffs corresponding to last frame.
    /// - `a_old`: LP synthesis filter coeffs corresponding to the frame before last frame.
    /// - `core_coder_frame_length`: Length of core coder frame (1024|768).
    /// - `clear_old_exc`: Clear old excitation buffer.
    #[expect(clippy::too_many_arguments)]
    pub fn prepare_internal_mem(
        &mut self,
        old_exc_mem: &mut [f32],
        synth: &[f32],
        cur_and_last_lpd_modes: &[LpdMode],
        a_new: &[f32],
        a_old: &[f32],
        core_coder_frame_length: usize,
        clear_old_exc: bool,
    ) {
        let l_div = core_coder_frame_length / NB_DIV; // length of one ACELP/TCX20 frame
        let l_div_partial = BUFFER_OFFSET - l_div;

        let mut synth_buf = [0.0f32; BUFFER_OFFSET + M_LP_FILTER_ORDER];

        let lpd_mode = cur_and_last_lpd_modes[0];
        let last_lpd_mode = cur_and_last_lpd_modes[1];
        let last_last_lpd_mode = cur_and_last_lpd_modes[2];

        if lpd_mode == LpdMode::TcxTdConceal {
            // Bypass Domain conversion. TCX_TD Concealment does no deemphasis in the end.
            synth_buf.copy_from_slice(&synth[1..(BUFFER_OFFSET + M_LP_FILTER_ORDER) + 1]);
            // Set deemphasis memory state for TD concealment.
            self.deemph_mem_wsyn = synth[BUFFER_OFFSET + M_LP_FILTER_ORDER];
        } else {
            // Convert past [BUFFER_OFFSET+M_LP_FILTER_ORDER] synthesis to preemph domain.
            Self::preemph(synth, &mut synth_buf[..BUFFER_OFFSET + M_LP_FILTER_ORDER]);
        }

        // Set deemphasis memory state.
        self.de_emph_mem = synth[BUFFER_OFFSET + M_LP_FILTER_ORDER];

        // Update ACELP synth filter memory.
        self.old_syn_mem[0..M_LP_FILTER_ORDER]
            .copy_from_slice(&synth_buf[BUFFER_OFFSET..BUFFER_OFFSET + M_LP_FILTER_ORDER]);

        if clear_old_exc {
            old_exc_mem[..BUFFER_OFFSET].fill(0.0);
            return;
        }

        // Update past [BUFFER_OFFSET] samples of exc memory.
        if last_lpd_mode == LpdMode::Tcx20 {
            match last_last_lpd_mode {
                LpdMode::Acelp => {
                    // ACELP -> TCX20 -> ACELP transition
                    // Delay valid part of excitation buffer (from previous ACELP frame) by * l_div
                    // samples;
                    old_exc_mem.copy_within(l_div..l_div + l_div_partial, 0);
                }
                _ => {
                    // TCX -> TCX20 -> ACELP transition
                    Self::compute_residual(a_old, &synth_buf, old_exc_mem, l_div_partial);
                }
            }
            Self::compute_residual(
                a_new,
                &synth_buf[l_div_partial..],
                &mut old_exc_mem[l_div_partial..],
                l_div,
            );
        } else {
            // Prev frame was FD, TCX40 or TCX80.

            let exc_a_new_length = if core_coder_frame_length / 2 > BUFFER_OFFSET {
                BUFFER_OFFSET
            } else {
                core_coder_frame_length / 2
            };

            let exc_a_old_length = BUFFER_OFFSET - exc_a_new_length;

            Self::compute_residual(a_old, &synth_buf, old_exc_mem, exc_a_old_length);
            Self::compute_residual(
                a_new,
                &synth_buf[exc_a_old_length..],
                &mut old_exc_mem[exc_a_old_length..],
                exc_a_new_length,
            );
        }
    }

    /// ACELP pre-processing.
    pub fn pre_processing(
        synth_buf: &mut [f32],
        old_synth: &[f32],
        pitch: &mut [u16],
        old_t_pf: &[u16],
        pit_gain: &mut [f32],
        old_gain_pf: &[f32],
    ) {
        // Init beginning of synth_buf with old synthesis from previous frame.
        synth_buf[..old_synth.len()].copy_from_slice(old_synth);

        // For bass postfilter:
        pitch[..old_t_pf.len()].copy_from_slice(old_t_pf);
        pitch[old_t_pf.len()..].fill(L_SUBFR as u16);
        pit_gain[..old_t_pf.len()].copy_from_slice(old_gain_pf);
        pit_gain[old_t_pf.len()..].fill(0.0);
    }

    /// Reads ACELP frame.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `acelp_core_mode`: ACELP core mode for whole ACELP frame.
    /// - `core_coder_frame_length`: Length of core coder frame (1024|768).
    /// - `frame`: ACELP frame number (0-2 for frame length 768, 0-3 for 1024).
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        acelp_core_mode: u8,
        core_coder_frame_length: usize,
        frame: usize,
    ) -> AacDecoderError {
        self.acelp_info[frame].read(bs, acelp_core_mode, core_coder_frame_length)
    }

    /// Resets ACELP internal memory.
    pub fn reset(&mut self) {
        self.gc_threshold = 0.0;
        self.past_gpit = 0.0;
        self.past_gcode = 0.0;
        self.old_t0 = 0;
        self.old_t0_frac = 0;
        self.deemph_mem_wsyn = 0.0;
        self.wsyn_rms = 0.0;
        self.seed_ace = 0;
    }

    /// TCX time domain concealment
    /// Compare to figure 13a on page 54 in 3GPP TS 26.290
    #[expect(clippy::too_many_arguments)]
    pub fn tcx_td_conceal(
        &mut self,
        old_exc_mem: &mut [f32],
        pitch: u16,
        lsp_old: &[f32; M_LP_FILTER_ORDER],
        lsp_new: &[f32; M_LP_FILTER_ORDER],
        n_lost_sf: i32,
        synth: &mut [f32],
        core_coder_frame_length: usize,
    ) {
        let mut exc_buf: [f32; BUFFER_OFFSET + L_DIV] = [0.0; BUFFER_OFFSET + L_DIV];
        let mut syn_buf: [f32; M_LP_FILTER_ORDER + L_DIV] = [0.0; M_LP_FILTER_ORDER + L_DIV];
        let mut ns_buf: [f32; L_DIV + 1] = [0.0; L_DIV + 1];

        //let ns = &ns_buf[1..];
        let t = (pitch as usize).min(PIT_MAX_MAX);
        let l_div = core_coder_frame_length / NB_DIV;

        syn_buf[..M_LP_FILTER_ORDER].copy_from_slice(&self.old_syn_mem[..M_LP_FILTER_ORDER]);

        exc_buf[..BUFFER_OFFSET].copy_from_slice(&old_exc_mem[..BUFFER_OFFSET]);

        // If we lost all packets (i.e. 1 packet of TCX-80 ms, 2 packets of
        // the TCX-40 ms or 4 packets of the TCX-20ms), we lost the whole
        // coded frame extrapolation strategy: repeat lost excitation and
        // use extrapolated LSFs.

        // AMR-WB+ like TCX TD concealment

        // Number of lost frame cmpt.
        let fact_exc = if n_lost_sf < 2 { 0.8 } else { 0.4 };

        // Repeat past excitation.
        for i in 0..l_div {
            exc_buf[BUFFER_OFFSET + i] = fact_exc * exc_buf[BUFFER_OFFSET + i - t];
        }

        self.wsyn_rms *= fact_exc;

        // Divide excitation buffer into low and high part.
        let (exc_buf_lo, exc_buf_hi) = exc_buf.split_at_mut(BUFFER_OFFSET);

        // Init deemph_mem_wsyn.
        self.deemph_mem_wsyn = exc_buf_lo[BUFFER_OFFSET - 1];

        ns_buf[0] = self.deemph_mem_wsyn;

        for (subfr_nr, (exc_sub, ns_sub)) in izip!(
            exc_buf_hi.chunks_exact(L_SUBFR),
            ns_buf.chunks_exact_mut(L_SUBFR)
        )
        .enumerate()
        .take(l_div / L_SUBFR)
        {
            let mut t_res: [f32; L_SUBFR] = [0.0; L_SUBFR];
            let mut a: [f32; M_LP_FILTER_ORDER] = [0.0; M_LP_FILTER_ORDER];
            let mut b: [f32; M_LP_FILTER_ORDER] = [0.0; M_LP_FILTER_ORDER];

            let syn_sub =
                &mut syn_buf[subfr_nr * L_SUBFR..(subfr_nr + 1) * L_SUBFR + M_LP_FILTER_ORDER];

            // Interpolate LSP coefficients.
            Self::interpolate_lsp(lsp_old, lsp_new, subfr_nr, l_div / L_SUBFR, &mut a);

            Self::syn_filt(&a, exc_sub, syn_sub);

            lpc::apply_abs_weighting(&mut b, &a);

            Self::compute_residual(&b, syn_sub, &mut t_res, L_SUBFR);

            Self::deemph(&t_res, ns_sub, &mut self.deemph_mem_wsyn);

            // Amplitude limiter (saturate at wsyn_rms).
            for smp in ns_sub.iter_mut() {
                *smp = smp.clamp(-self.wsyn_rms, self.wsyn_rms);
            }

            Self::preemph(&ns_sub[1..], &mut t_res);

            Self::syn_filt(&b, &t_res, syn_sub);

            synth[subfr_nr * L_SUBFR..(subfr_nr + 1) * L_SUBFR]
                .copy_from_slice(&syn_sub[..L_SUBFR]);
        }

        // Save old excitation and old synthesis memory for next ACELP frame.
        old_exc_mem.copy_from_slice(&exc_buf[l_div..l_div + BUFFER_OFFSET]);
        self.old_syn_mem
            .copy_from_slice(&syn_buf[l_div..l_div + M_LP_FILTER_ORDER]);
        self.de_emph_mem = self.deemph_mem_wsyn;
    }

    /// Calculates zero input response (ZIR) of the ACELP synthesis filter.
    ///
    /// # Parameters
    ///
    /// - `a`: LP synthesis filter coefficients.
    /// - `zir`: ZIR output buffer.
    /// - `do_deemph`: Do deemphasis.
    pub fn zir(&mut self, a: &[f32], zir: &mut [f32], do_deemph: bool) {
        let mut tmp_buf = [0.0; M_LP_FILTER_ORDER + LFAC];
        debug_assert!(M_LP_FILTER_ORDER == a.len());
        tmp_buf[..M_LP_FILTER_ORDER].copy_from_slice(&(self.old_syn_mem));
        zir.iter_mut().for_each(|m| *m = 0.0);

        Self::syn_filt(a, zir, &mut tmp_buf[..M_LP_FILTER_ORDER + zir.len()]);
        if !do_deemph {
            // If last LPD mode was TD concealment, then bypass deemph.
            zir.copy_from_slice(&tmp_buf[..zir.len()]);
        } else {
            Self::deemph(
                &tmp_buf[M_LP_FILTER_ORDER..M_LP_FILTER_ORDER + zir.len()],
                zir,
                &mut self.de_emph_mem,
            );
        }
    }

    // Internal methods / associated functions

    /// Adds pulses to fixed codebook.
    ///
    /// # Parameters
    ///
    /// - `pos`: Position of pulse.
    /// - `nb_pulse`: Number of pulses.
    /// - `track`: Track.
    /// - `code`: Fixed codebook.
    fn add_pulses(pos: &[usize], nb_pulse: usize, track: usize, code: &mut [f32]) {
        let mut i: usize;
        for pos_element in pos.iter().take(nb_pulse) {
            i = ((*pos_element & (16 - 1)) << 2) + track;
            if (*pos_element & 16) == 0 {
                code[i] += 512f32;
            } else {
                code[i] -= 512f32;
            }
        }
    }

    ///  Updates adaptive codebook `exc`.
    ///  Enhances pitch of `code` and builds post-processed excitation `exc2`.
    ///
    /// # Parameters
    ///
    /// - `code`: Innovative codevector.
    /// - `exc`: Filtered adaptive codebook.
    /// - `pit_gain`: Adaptive codebook gain, also called pitch gain.
    /// - `code_gain`: Innovative codebook gain.
    /// - `code_gain_smoothed`: Smoothed innovative codebook gain.
    /// - `period_fac`: Periodicity factor.
    /// - `exc2`: Post-processed excitation.
    fn build_adaptive_excitation(
        code: &[f32],
        exc: &mut [f32],
        pit_gain: f32,
        code_gain: f32,
        code_gain_smoothed: f32,
        period_fac: f32,
        exc2: &mut [f32],
    ) {
        let (mut tmp, cpe, code_smooth_prev, mut code_smooth): (f32, f32, f32, f32);

        let mut code_i: f32;
        let (mut cpe_code_smooth, mut cpe_code_smooth_prev): (f32, f32);

        // cpe = (1+r_v)/8 * 2 ; ( SF = -1)
        cpe = 0.125 * (1.0 + period_fac);

        // u'(n)
        tmp = exc[0] * pit_gain; // v(0)*g_p
        exc[0] = tmp + code[0] * code_gain;

        // u(n)
        code_smooth_prev = code[0] * code_gain_smoothed; // c(0) * g_sc
        code_i = code[1];
        code_smooth = code_i * code_gain_smoothed; // c(1) * g_sc
        tmp += code_smooth_prev; // tmp = v(0)*g_p + c(0)*g_sc
        cpe_code_smooth = cpe * code_smooth;
        exc2[0] = tmp - cpe_code_smooth;
        cpe_code_smooth_prev = cpe * code_smooth_prev;

        for (e, e2, c) in izip!(
            exc.iter_mut().take(L_SUBFR - 1).skip(1),
            exc2.iter_mut().take(L_SUBFR - 1).skip(1),
            code.iter().take(L_SUBFR).skip(2)
        ) {
            // u'(n)
            tmp = *e * pit_gain;
            *e = tmp + code_i * code_gain;
            // u(n)
            tmp += code_smooth; // += g_sc * c(i)
            tmp -= cpe_code_smooth_prev;
            cpe_code_smooth_prev = cpe_code_smooth;
            code_i = *c;
            code_smooth = code_i * code_gain_smoothed;
            cpe_code_smooth = cpe * code_smooth;
            *e2 = tmp - cpe_code_smooth; // tmp - c_pe * g_sc * c(i+1)
        }

        // u'(n)
        tmp = exc[L_SUBFR - 1] * pit_gain;
        exc[L_SUBFR - 1] = tmp + code_i * code_gain;
        // u(n)
        tmp += code_smooth;
        tmp -= cpe_code_smooth_prev;
        exc2[L_SUBFR - 1] = tmp;
    }

    /// Calculates period/voicing factor.
    ///
    /// # Parameters
    ///
    /// - `pit_exc`: Pitch excitation.
    /// - `pit_gain`: Adaptive codebook gain, also called pitch gain.
    /// - `code_ener`: Unbiased innovative code vector energy.
    /// - `code_gain`: Innovative codebook gain.
    ///
    /// # Return
    ///
    /// Period/voice factor (-1=unvoiced to 1=voiced).
    fn calc_period_factor(pit_exc: &[f32], pit_gain: f32, code_ener: f32, code_gain: f32) -> f32 {
        // Energy of pitch excitation
        let mut pit_exc_ener = 0.0f32;
        pit_exc
            .iter()
            .take(L_SUBFR)
            .for_each(|e| pit_exc_ener += *e * *e);
        pit_exc_ener = pit_exc_ener * pit_gain * pit_gain;

        // Energy of innovative code excitation.
        let code_ener = code_ener * code_gain * code_gain;

        let num = pit_exc_ener - code_ener;
        let den = pit_exc_ener + code_ener + 0.01;

        let period_factor = (num / den).clamp(-1.0, 1.0);

        if period_factor.is_nan() {
            0.0 // undefined --> return neither voiced nor unvoiced
        } else {
            period_factor
        }
    }

    /// Computes the LP residual by filtering the input speech through the analysis filter `a`.
    ///
    /// # Parameters
    ///
    /// - `a` LP filter coefficients.
    /// - `x` Input signal.
    /// - `y` Output signal (residual).
    /// - `l` Length of filtering.
    fn compute_residual(a: &[f32], x: &[f32], y: &mut [f32], l: usize) {
        for i in 0..l {
            let mut s = 0.0;

            for j in 0..M_LP_FILTER_ORDER {
                s += a[j] * x[M_LP_FILTER_ORDER + i - j - 1];
            }

            y[i] = s + x[M_LP_FILTER_ORDER + i];
        }
    }

    /// Conceals pitch lag.
    fn conceal_pitch_lag(&mut self, pit_max: u16, t0: &mut u16, t0_frac: &mut u8) {
        *t0 = if self.old_t0 >= pit_max {
            pit_max - 5
        } else {
            self.old_t0
        };
        *t0_frac = self.old_t0_frac;
    }

    /// Decodes 1 pulse with N+1 bits.
    ///
    /// # Parameters
    ///
    /// - `index`: Pulse index.
    /// - `n`: Number of bits for position.
    /// - `offset`: Offset.
    /// - `pos`: Position of the pulse.
    fn decode_1p_n1(index: usize, n: u16, offset: usize, pos: &mut [usize]) {
        let mut pos1;
        let mask = (1 << n) - 1;

        pos1 = (index & mask) + offset;
        let i = (index >> n) & 1;
        if i == 1 {
            pos1 += 16;
        }
        pos[0] = pos1;
    }

    /// Decodes 2 pulses with 2*N+1 bits.
    ///
    /// # Parameters
    ///
    /// - `index`: Pulse index.
    /// - `n`: Number of bits for position.
    /// - `offset`: Offset.
    /// - `pos`: Position of the pulse.
    fn decode_2p_2_n1(index: usize, n: u16, offset: usize, pos: &mut [usize]) {
        let mask: usize = (1 << n) - 1;

        let mut pos1 = ((index >> n) & mask) + offset;
        let i = (index >> (2 * n)) & 1;
        let mut pos2: usize = (index & mask) + offset;
        if pos2 < pos1 {
            if i == 1 {
                pos1 += 16;
            } else {
                pos2 += 16;
            }
        } else if i == 1 {
            pos1 += 16;
            pos2 += 16;
        }
        pos[0] = pos1;
        pos[1] = pos2;
    }

    /// Decodes 3 pulses with 3*N+1 bits.
    ///
    /// # Parameters
    ///
    /// - `index`: Pulse index.
    /// - `n`: Number of bits for position.
    /// - `offset`: Offset.
    /// - `pos`: Position of the pulse.
    fn decode_3p_3_n1(index: usize, n: u16, offset: usize, pos: &mut [usize]) {
        let mut mask = (1 << ((2 * n) - 1)) - 1;
        let mut idx = index & mask;
        let mut j = offset;
        if ((index >> ((2 * n) - 1)) & 1) == 1 {
            j += 1 << (n - 1);
        }
        Self::decode_2p_2_n1(idx, n - 1, j, pos);
        mask = (1 << (n + 1)) - 1;
        idx = (index >> (2 * n)) & mask;
        Self::decode_1p_n1(idx, n, offset, &mut pos[2..]);
    }

    /// Decodes 4 pulses with 4*N bits.
    ///
    /// # Parameters
    ///
    /// - `index`: Pulse index.
    /// - `n`: Number of bits for position.
    /// - `offset`: Offset.
    /// - `pos`: Position of the pulse.
    fn decode_4p_4_n(index: usize, n: u16, offset: usize, pos: &mut [usize]) {
        let n_1 = n - 1;
        let j = offset + (1 << n_1);
        match (index >> ((4 * n) - 2)) & 3 {
            0 => {
                if ((index >> ((4 * n_1) + 1)) & 1) == 0 {
                    Self::decode_4p_4_n1(index, n_1, offset, pos);
                } else {
                    Self::decode_4p_4_n1(index, n_1, j, pos);
                }
            }
            1 => {
                Self::decode_1p_n1(index >> ((3 * n_1) + 1), n_1, offset, pos);
                Self::decode_3p_3_n1(index, n_1, j, &mut pos[1..]);
            }
            2 => {
                Self::decode_2p_2_n1(index >> ((2 * n_1) + 1), n_1, offset, pos);
                Self::decode_2p_2_n1(index, n_1, j, &mut pos[2..]);
            }
            3 => {
                Self::decode_3p_3_n1(index >> (n_1 + 1), n_1, offset, pos);
                Self::decode_1p_n1(index, n_1, j, &mut pos[3..]);
            }
            _ => panic!(),
        }
    }

    /// Decodes 4 pulses with 4*N+1 bits.
    ///
    /// # Parameters
    ///
    /// - `index`: Pulse index.
    /// - `n`: Number of bits for position.
    /// - `offset`: Offset.
    /// - `pos`: Position of the pulse.
    fn decode_4p_4_n1(index: usize, n: u16, offset: usize, pos: &mut [usize]) {
        let mut mask = (1 << ((2 * n) - 1)) - 1;
        let mut idx = index & mask;
        let mut j = offset;
        if ((index >> ((2 * n) - 1)) & 1) == 1 {
            j += 1 << (n - 1);
        }
        Self::decode_2p_2_n1(idx, n - 1, j, pos);
        mask = (1 << ((2 * n) + 1)) - 1;
        idx = (index >> (2 * n)) & mask;
        Self::decode_2p_2_n1(idx, n, offset, &mut pos[2..]);
    }

    /// Decodes 4 tracks x 16 positions per track = 64 algebraic codebook samples.
    ///
    /// 20, 36, 44, 52, 64, 72, 88 bits algebraic codebook.
    ///
    /// 20 bits 5+5+5+5 --> 4 pulses in a frame of 64 samples.
    /// 36 bits 9+9+9+9 --> 8 pulses in a frame of 64 samples.
    /// 44 bits 13+9+13+9 --> 10 pulses in a frame of 64 samples.
    /// 52 bits 13+13+13+13 --> 12 pulses in a frame of 64 samples.
    /// 64 bits 2+2+2+2+14+14+14+14 --> 16 pulses in a frame of 64 samples.
    /// 72 bits 10+2+10+2+10+14+10+14 --> 18 pulses in a frame of 64 samples.
    /// 88 bits 11+11+11+11+11+11+11+11 --> 24 pulses in a frame of 64 samples.
    ///
    /// All pulses can have two (2) possible amplitudes: +1 or -1.
    /// Each pulse can have sixteen (16) possible positions.
    ///
    /// Codevector length    64
    /// Number of tracks      4
    /// Number of positions  16
    ///
    /// # Parameters
    ///
    /// - `index`: Index.
    /// - `nbits`: Algebraic codebook bits.
    /// - `code`: (Q9) Algebraic (fixed) codebook excitation.
    fn decode_4t64(index: &[usize; 8], nbits: u8, code: &mut [f32]) {
        code.fill(0f32);

        // Decode the positions and signs of pulses and build the codeword.
        let mut pos = [0usize; 6];
        match nbits {
            12 => {
                for k in (0..4).step_by(2) {
                    Self::decode_1p_n1(index[2 * (k / 2) + 1], 4, 0, &mut pos);
                    Self::add_pulses(&pos, 1, 2 * (index[2 * (k / 2)]) + k / 2, code);
                }
            }
            16 => {
                let mut i = 0;
                let mut offset = index[i];
                i += 1;
                if offset == 0 {
                    offset = 1;
                } else {
                    offset = 3;
                }
                for k in 0..4 {
                    if k != offset {
                        Self::decode_1p_n1(index[i], 4, 0, &mut pos);
                        Self::add_pulses(&pos, 1, k, code);
                        i += 1;
                    }
                }
            }
            20 => {
                for (k, index_element) in index.iter().enumerate().take(4) {
                    Self::decode_1p_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 1, k, code);
                }
            }
            28 => {
                for (k, index_element) in index.iter().enumerate().take(2) {
                    Self::decode_2p_2_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 2, k, code);
                }
                for (k, index_element) in index.iter().enumerate().take(4).skip(2) {
                    Self::decode_1p_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 1, k, code);
                }
            }
            36 => {
                for (k, index_element) in index.iter().enumerate().take(4) {
                    Self::decode_2p_2_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 2, k, code);
                }
            }
            44 => {
                for (k, index_element) in index.iter().enumerate().take(2) {
                    Self::decode_3p_3_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 3, k, code);
                }
                for (k, index_element) in index.iter().enumerate().take(4).skip(2) {
                    Self::decode_2p_2_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 2, k, code);
                }
            }
            52 => {
                for (k, index_element) in index.iter().enumerate().take(4) {
                    Self::decode_3p_3_n1(*index_element, 4, 0, &mut pos);
                    Self::add_pulses(&pos, 3, k, code);
                }
            }
            64 => {
                for k in 0..4 {
                    Self::decode_4p_4_n((index[k] << 14) + index[k + 4], 4, 0, &mut pos);
                    Self::add_pulses(&pos, 4, k, code);
                }
            }
            _ => debug_assert!(false),
        }
    }

    /// Decodes adaptive and innovative codebook gains.
    ///
    /// # Parameters
    ///
    /// - `index`: ACELP subframe gain index.
    /// - `code`: Innovative code vector.
    /// - `mean_ener_bits`: Mean energy defined in open-loop (2 bits).
    /// - `bfi`: Bad frame indicator.
    ///
    /// # Returns
    /// - `(pit_gain, code_gain, code_vec_energy)`
    ///     - `pit_gain`: Adaptive codebook gain, also called pitch gain.
    ///     - `code_gain`: Innovative codebook gain.
    ///     - `code_vec_energy`: Innovative codebook vector energy.
    fn decode_gains(
        &mut self,
        index: usize,
        code: &[f32],
        mean_ener_bits: u8,
        bfi: bool,
    ) -> (f32, f32, f32) {
        let (pit_gain, code_gain, code_vec_energy);

        // code_ener = sum(code[]^2)
        let mut code_ener = 0.0;
        code.iter().for_each(|e| code_ener += *e * *e);

        // Export energy of code for calc_period_factor()
        code_vec_energy = code_ener;

        code_ener += 0.01;

        let gcode_inov = (code_ener / L_SUBFR as f32).sqrt().recip();

        if bfi {
            // Bad frame
            let mut tgpit = self.past_gpit;

            tgpit = tgpit.clamp(0.5, 0.95);
            pit_gain = tgpit;
            tgpit *= 0.95;
            self.past_gpit = tgpit;

            tgpit = 1.4 - tgpit;
            let tgcode = self.past_gcode * tgpit;
            code_gain = tgcode * gcode_inov;
            self.past_gcode = tgcode;
        } else {
            // Good frame
            //-------------- Decode gains ---------------
            // gcode0 = pow(10.0, (float)mean_ener/20.0);
            // gcode0 = gcode0 / sqrt(code_ener/L_SUBFR);
            let mut gcode0 = POW_10_MEAN_ENERGY[mean_ener_bits as usize];
            gcode0 *= gcode_inov;

            let i = index << 1;
            pit_gain = T_QUA_GAIN7B[i]; // Adaptive codebook gain
            let l_tmp = T_QUA_GAIN7B[i + 1] * gcode0; // Innovative codebook gain
            code_gain = l_tmp;

            // Update bad frame handler.
            self.past_gpit = pit_gain;

            self.past_gcode = l_tmp / gcode_inov;
        }

        (pit_gain, code_gain, code_vec_energy) // return gain values.
    }

    /// Calculates de-emphasis 1/(1 - mu z^-1) on input signal.
    ///
    /// # Parameters
    ///
    /// - `x`: Input signal.
    /// - `y`: Output signal.
    /// - `mem`: Memory.
    fn deemph(x: &[f32], y: &mut [f32], mem: &mut f32) {
        let mut yi = *mem;
        for (dst, src) in izip!(y, x) {
            *dst = *src + PREEMPH_FAC * yi;
            yi = *dst;
        }
        if yi.abs() < 1e-10 {
            yi = 0.0;
        }
        *mem = yi;
    }

    /// Generates random value.
    fn gen_rnd(&mut self) -> f32 {
        {
            self.seed_ace = ((((self.seed_ace as i32) * 31821) >> 1) + 13849) as i16;
        }
        self.seed_ace as f32
    }

    /// Interpolates LPC vector in LSP domain for current subframe and converts to LP domain.
    ///
    /// # Parameters
    ///
    /// - `lsp_old`: LPC vector (LSP domain) corresponding to the beginning of current ACELP frame.
    /// - `lsp_new`: LPC vector (LSP domain) corresponding to the end of current ACELP frame.
    /// - `subfr_nr`: Number of current ACELP subframe 0..3.
    /// - `nb_subfr`: Total number of ACELP subframes in this frame.
    /// - `a`: Interpolated LP filter coefficients for current ACELP subframe.
    fn interpolate_lsp(
        lsp_old: &[f32],
        lsp_new: &[f32],
        subfr_nr: usize,
        nb_subfr: usize,
        a: &mut [f32],
    ) {
        let mut lsp_interpol = [0.0f32; M_LP_FILTER_ORDER];

        debug_assert!(nb_subfr == 3 || nb_subfr == 4);
        let fac_old: f32 = LSP_INTERPOL_FACTOR[nb_subfr & 0x1][nb_subfr - 1 - subfr_nr];
        let fac_new: f32 = LSP_INTERPOL_FACTOR[nb_subfr & 0x1][subfr_nr];

        for (lsp_old_element, lsp_new_element, lsp_interpol_element) in
            izip!(lsp_old.iter(), lsp_new.iter(), lsp_interpol.iter_mut()).take(M_LP_FILTER_ORDER)
        {
            *lsp_interpol_element = *lsp_old_element * fac_old + *lsp_new_element * fac_new;
        }
        lpc::lsp_to_lpc(&lsp_interpol, a);
    }

    /// Enhances excitation on noise. (modifies gain of code).
    /// If signal is noisy and LPC filter is stable, move gain
    /// of code 1.5 dB toward gain of code threshold.
    /// This decreases noise energy variation by 3 dB.
    ///
    /// # Parameters
    ///
    /// - `code_gain`: Quantized codebook gain.
    /// - `period_fac`: Periodicity factor.
    /// - `stab_fac`: Stability factor.
    /// - `p_gc_threshold`: modified gain of previous subframe.
    ///
    /// # Return
    ///
    /// Smoothed gain of code.
    fn noise_enhancer(&mut self, code_gain: f32, period_fac: f32, stab_fac: f32) -> f32 {
        let gc_thres = self.gc_threshold;

        self.gc_threshold = if code_gain < gc_thres {
            (code_gain * 1.19).min(gc_thres) // +1.5dB
        } else {
            (code_gain * (1.0 / 1.19)).max(gc_thres) // -1.5dB
        };

        // Voicing factor lambda = 0.5*(1-period_fac).
        // Gain smoothing factor S_m = lambda*stab_fac (=fac) =
        //   = 0.5(stab_fac - stab_fac * period_fac).
        let fac = 0.5 * (stab_fac - (stab_fac * period_fac));
        debug_assert!(fac >= 0.0);

        (fac * self.gc_threshold) + ((1.0 - fac) * code_gain)
    }

    /// Applies pitch sharpener to the innovative codebook vector.
    ///
    /// # Parameters
    ///
    /// - `x`: innovative codebook vector.
    /// - `pitch_lag`: decoded pitch lag.
    fn pitch_sharp(x: &mut [f32], pitch_lag: usize) {
        const PIT_SHARP: f32 = 0.85; // Pitch sharpening factor
        if L_SUBFR > pitch_lag {
            for i in 0..(L_SUBFR - pitch_lag) {
                x[i + pitch_lag] += x[i] * PIT_SHARP;
            }
        }
    }

    /// LTP post-filtering.
    ///
    /// # Parameters
    ///
    /// - `exc`: excitation buffer.
    fn pred_lt4_postfilter(exc: &mut [f32]) {
        let mut sum0: f32;
        let mut sum1: f32;
        let mut a_exc0 = A2 * exc[0];
        let mut a_exc1 = A2 * exc[1];

        for i in (1..=L_SUBFR).step_by(2) {
            sum0 = a_exc0 + B * exc[i];
            sum1 = a_exc1 + B * exc[i + 1];
            a_exc0 = A2 * exc[i + 1];
            a_exc1 = A2 * exc[i + 2];
            exc[i] = sum0 + a_exc0;
            exc[i + 1] = sum1 + a_exc1;
        }
    }

    /// LTP filtering.
    ///
    /// # Parameters
    ///
    /// - `exc`: Excitation buffer.
    /// - `t0`: Integer pitch lag.
    /// - `frac`: Fraction of lag in range 0..3.
    fn pred_lt4(exc: &mut [f32], t0: u16, frac: u8) {
        let mut frac = usize::from(frac);
        let t0_us = usize::from(t0);

        let exc_start_idx = if frac > 0 {
            BUFFER_OFFSET - t0_us - L_INTERPOL2
        } else {
            BUFFER_OFFSET + 1 - t0_us - L_INTERPOL2
        };

        frac = match frac {
            1 => 3,
            3 => 1,
            _ => frac,
        };

        let (down_interpolation, up_interpolation) = match frac {
            0 => (
                &PRED_LT4_INTER4_2_0[..L_INTERPOL2],
                &PRED_LT4_INTER4_2_0[1..],
            ),
            1 => (&PRED_LT4_INTER4_2_3[..], &PRED_LT4_INTER4_2_1[..]),
            2 => (&PRED_LT4_INTER4_2_2[..], &PRED_LT4_INTER4_2_2[..]),
            _ => (&PRED_LT4_INTER4_2_1[..], &PRED_LT4_INTER4_2_3[..]),
        };

        for j in 0..=L_SUBFR {
            let window = &exc[exc_start_idx + j..exc_start_idx + j + 32];
            let (first, second) = window.split_at(16);
            let mut accu = first
                .iter()
                .zip(down_interpolation.iter().rev())
                .map(|(x, y)| x * y)
                .sum::<f32>();
            accu += second
                .iter()
                .zip(up_interpolation.iter())
                .map(|(x, y)| x * y)
                .sum::<f32>();
            exc[BUFFER_OFFSET + j] = accu;
        }
    }

    /// Calculates pre-emphasis (1 - mu z^-1) on input signal.
    ///
    /// # Parameters
    ///
    /// - `x`: Input signal.
    /// - `y`: Output signal.
    fn preemph(x: &[f32], y: &mut [f32]) {
        for i in 0..y.len().min(x.len() - 1) {
            y[i] = x[1 + i] - PREEMPH_FAC * x[i];
        }
    }

    /// Calculates pre-emphasis 1/(1 - TILT_CODE z^-1) on innovative codebook vector.
    ///
    /// # Parameters
    ///
    /// - `x`: Innovative codebook vector.
    fn preemph_code(x: &mut [f32]) {
        //(i/o)   : input signal overwritten by the output
        for i in (1..x.len()).rev() {
            x[i] -= x[i - 1] * TILT_CODE2;
        }
    }

    /// Performs LP synthesis by filtering the post-processed excitation
    /// through the LP synthesis filter 1/a(z).
    ///
    /// # Parameters
    ///
    /// - `a`: LP filter coefficients.
    /// - `x`: Post-processed excitation.
    /// - `y`: LP synthesis signal ([M_LP_FILTER_ORDER..]) and filter memory (..M_LP_FILTER_ORDER]).
    fn syn_filt(a: &[f32], x: &[f32], y: &mut [f32]) {
        let mut l_tmp;
        for i in 0..x.len() {
            l_tmp = 0.0;
            for j in 0..M_LP_FILTER_ORDER {
                l_tmp -= a[j] * y[M_LP_FILTER_ORDER + i - (j + 1)];
            }
            l_tmp = l_tmp.clamp(-LPD_SYN_FILT_LIMIT, LPD_SYN_FILT_LIMIT);

            y[M_LP_FILTER_ORDER + i] = l_tmp + x[i];
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn preemph() {
        const ARR_LEN: usize = 64;
        let buf_in = [10.0; ARR_LEN + 1];
        let mut buf_out = [0.0; ARR_LEN];

        AcelpData::preemph(&buf_in, &mut buf_out);
        assert!(
            &[10.0 * (1.0 - PREEMPH_FAC); ARR_LEN]
                .iter()
                .zip(buf_out.iter())
                .all(|(a, b)| a == b),
            "Arrays are not equal"
        );
    }

    #[test]
    fn deemph() {
        const ARR_LEN: usize = 64;
        let buf_in = [10.0; ARR_LEN];
        let mut buf_out = [0.0; ARR_LEN];
        let mut m = 0.0;

        AcelpData::deemph(&buf_in, &mut buf_out, &mut m);
        assert!(
            &[
                10.000000, 16.799999, 21.424000, 24.568319, 26.706457, 28.160391, 29.149065,
                29.821363, 30.278526, 30.589397, 30.800791, 30.944538, 31.042286, 31.108755,
                31.153954, 31.184689, 31.205587, 31.219799, 31.229464, 31.236034, 31.240503,
                31.243542, 31.245609, 31.247015, 31.247971, 31.248619, 31.249062, 31.249361,
                31.249565, 31.249704, 31.249800, 31.249865, 31.249908, 31.249937, 31.249958,
                31.249971, 31.249981, 31.249987, 31.249990, 31.249994, 31.249996, 31.249998,
                31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998,
                31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998,
                31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998, 31.249998,
                31.249998
            ]
            .iter()
            .zip(buf_out.iter())
            .all(|(a, b)| (a - b).abs() < 0.00001),
            "Arrays are not equal"
        );
    }

    #[test]
    fn pitch_sharp() {
        let mut codebook_vector: [f32; L_SUBFR] = [0.0; L_SUBFR];

        // Test case: pitch_lag < L_SUBFR
        let pitch_lag: usize = 16;
        for item in codebook_vector[..pitch_lag].iter_mut() {
            *item = 1.0
        }
        for item in codebook_vector[pitch_lag..].iter_mut() {
            *item = 0.15;
        }
        AcelpData::pitch_sharp(&mut codebook_vector, pitch_lag);
        assert_eq!(codebook_vector, [1.0; L_SUBFR]);

        // Test case: pitch_lag > L_SUBFR
        let pitch_lag: usize = 100;
        codebook_vector = [100.0; L_SUBFR];
        AcelpData::pitch_sharp(&mut codebook_vector, pitch_lag);
        assert_eq!(codebook_vector, [100.0; L_SUBFR]); // no change (should exit immediately)
    }

    #[test]
    fn syn_filt() {
        // Prediction coefficients
        let a: [f32; M_LP_FILTER_ORDER] = [0.0; M_LP_FILTER_ORDER];
        // Input signal
        let x: [f32; L_SUBFR] = [10.0; L_SUBFR];
        // Filter states / output signal
        let mut y: [f32; M_LP_FILTER_ORDER + L_SUBFR] = [0.0; M_LP_FILTER_ORDER + L_SUBFR];

        AcelpData::syn_filt(&a, &x, &mut y);
    }

    #[test]
    fn calc_period_factor_corner_cases() {
        const ARR_LEN: usize = 4;
        let sqrs_sum_up_to_inf = [-77.777E+18f32; ARR_LEN];
        let sqrs_sum_up_to_100 = [-((100_f32 / ARR_LEN as f32).sqrt()); ARR_LEN];
        let sqrs_sum_up_to_nix = [-((10E-8_f32 / ARR_LEN as f32).sqrt()); ARR_LEN];

        let mut exc;
        let mut pit_gain;
        let mut code_ener;
        let mut code_gain;

        // 0.0f32/0.0f32 = NaN --> neither unvoiced nor voiced
        exc = &sqrs_sum_up_to_inf;
        pit_gain = 0.0;
        code_ener = 0.005;
        code_gain = 1.0;
        assert!(AcelpData::calc_period_factor(exc, pit_gain, code_ener, code_gain) == 0.0);

        // ~0.01/0.0f32 = INF --> 1.0 (voiced)
        exc = &sqrs_sum_up_to_nix;
        pit_gain = 1.0;
        code_ener = -(10E-8_f32 + 0.01); // Negative energies are impossible in calling part
        code_gain = 1.0;
        assert!(AcelpData::calc_period_factor(exc, pit_gain, code_ener, code_gain) == 1.0);

        // ~-100.0f32/0.0f32 = -INF --> -1.0 (unvoiced)
        // (case neg. numerator and pos. denominator is not possible)

        // f32::INFINITY/f32::INFINITY = NaN --> neither unvoiced nor voiced
        exc = &sqrs_sum_up_to_inf;
        pit_gain = 1.0;
        code_ener = 1.0;
        code_gain = 1.0;
        assert!(AcelpData::calc_period_factor(exc, pit_gain, code_ener, code_gain) == 0.0);

        // f32::NAN/f32::INFINITY = NaN --> neither unvoiced nor voiced (case is not possible)

        // 100.0f32/f32::INFINITY 0 --> 0 neither unvoiced nor voiced (case is not possible)

        // f32::NEG_INFINITY/f32::INFINITY = NaN --> neither unvoiced nor voiced
        exc = &sqrs_sum_up_to_100;
        pit_gain = 1.0;
        code_ener = f32::INFINITY;
        code_gain = 10E10;
        assert!(code_ener.is_infinite());
        assert!(AcelpData::calc_period_factor(exc, pit_gain, code_ener, code_gain) == 0.0);

        // f32::NEG_INFINITY/1.0f32 = -INF --> -1.0 (unvoiced) (case is not possible)
    }
}
