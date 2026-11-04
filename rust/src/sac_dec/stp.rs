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
//! Subband domain temporal processing (STP)

use itertools::izip;
use num_complex::Complex;

use super::constants::*;
use super::process::HybData;
use crate::common::bitstream::Bitstream;

// Constants.

// Band-pass.
const BP_GF_START: usize = 6;
const BP_GF_SIZE: usize = 25;
const BP_N_BANDS: usize = BP_GF_SIZE - BP_GF_START;
const START_BAND: usize = BP_GF_START + 7;

// High-pass.
const HP_SIZE_QMF: usize = 9;
const HP_SIZE_HYB: usize = HP_SIZE_QMF - HYB_FB_BANDS_QMF_IN + HYB_FB_BANDS_HYB_OUT - 1;

const HYB_BAND_BORDER: usize = 12;

/// Bandpass factors.
static BP: [f32; HP_SIZE_HYB - HYB_BAND_BORDER] =
    [0.73919999599457, 0.97909998893738, 0.99930000305176];

/// Spectral flattening factors.
#[rustfmt::skip]
static BP_GF: [f32; BP_N_BANDS] = [
    9.58636403083804e-17, 6.55181740522387e-13, 1.30321008789540e-11,
    7.07280964021387e-11, 2.56000048637391e-10, 7.31161607961806e-10,
    1.57498101245299e-09, 3.15394514689640e-09, 5.36989606589438e-09,
    8.92492871984067e-09, 1.21655581785931e-08, 1.45164849062856e-08,
    1.48378925935694e-08, 1.32385889114978e-08, 1.18050451671684e-08,
    1.05542845008678e-08, 9.89363414292606e-09, 9.63117771103266e-09,
    9.84941097874272e-09,
];

#[repr(C)]
#[derive(Debug)]
pub(super) struct StpDec {
    run_dry_ener: f32,
    run_wet_ener: [f32; MAX_OUTPUT_CHANNELS],
    old_dry_ener_ld: f32,
    old_wet_ener_ld: [f32; MAX_OUTPUT_CHANNELS],
    prev_scale: [f32; MAX_OUTPUT_CHANNELS],
    update_old_ener: u8,
    temp_shape_enable_channel_stp: [bool; MAX_OUTPUT_CHANNELS],
}

impl Default for StpDec {
    fn default() -> Self {
        Self {
            run_dry_ener: 0.0,
            run_wet_ener: [0.0; MAX_OUTPUT_CHANNELS],
            old_dry_ener_ld: 0.0,
            old_wet_ener_ld: [0.0; MAX_OUTPUT_CHANNELS],
            prev_scale: [1.0; MAX_OUTPUT_CHANNELS],
            update_old_ener: 0,
            temp_shape_enable_channel_stp: [false; MAX_OUTPUT_CHANNELS],
        }
    }
}

impl StpDec {
    /// Applies STP onto the upmixed signal.
    ///
    /// # Parameters
    ///
    /// - `hyb_data`: Hybrid filter bank instance.
    pub(super) fn apply(&mut self, hyb_data: &mut HybData) {
        const UPDATE_ENERGY_RATE: u8 = 32;

        const LPF_COEFF1: f32 = 0.950;
        const ONE_MINUS_LPF_COEFF1: f32 = 0.05;
        const LPF_COEFF2: f32 = 0.450f32;
        const ONE_MINUS_LPF_COEFF2: f32 = 1.0f32 - 0.450f32;

        const SCALE_LIMIT: f32 = 2.82f32;
        const ONE_DIV_SCALE_LIMIT: f32 = 1.0f32 / 2.82f32;

        const ABS_THR: f32 = 1e-9;

        const LEFT: usize = 0;
        const RIGHT: usize = 1;

        let dry_ener_ld;
        let mut wet_ener_ld = [0.0f32; MAX_OUTPUT_CHANNELS];

        let mut scale = [0.0f32; MAX_OUTPUT_CHANNELS];

        if self.update_old_ener == UPDATE_ENERGY_RATE {
            // Update normalisation energy with latest smoothed energy.
            self.update_old_ener = 1;
            self.old_dry_ener_ld = self.run_dry_ener;
            self.old_wet_ener_ld.copy_from_slice(&self.run_wet_ener);
        } else {
            self.update_old_ener += 1;
        }

        // Form the 'direct' downmix signal.
        {
            let qmf_output_cplx_dry_lf = &(hyb_data.hyb_output_cplx_dry[LEFT])[START_BAND..];
            let qmf_output_cplx_dry_rf = &(hyb_data.hyb_output_cplx_dry[RIGHT])[START_BAND..];

            // Downmix.
            let dry_ener = izip!(qmf_output_cplx_dry_lf, qmf_output_cplx_dry_rf, BP_GF)
                .take(BP_N_BANDS)
                .map(|(l, r, b)| (l + r).norm_sqr() * b)
                .sum::<f32>();

            // Normalise the 'direct' signals.
            self.run_dry_ener =
                (LPF_COEFF1 * self.run_dry_ener) + (ONE_MINUS_LPF_COEFF1 * dry_ener);
            dry_ener_ld = dry_ener / (self.old_dry_ener_ld + ABS_THR);
        }

        // Normalise the 'diffuse' signals.
        {
            for (hybrid_wet, nrg_wet, nrg_ld_wet, nrg_ld_old_wet) in izip!(
                hyb_data.hyb_output_cplx_wet,
                &mut self.run_wet_ener,
                &mut wet_ener_ld,
                &self.old_wet_ener_ld
            ) {
                let qmf_output_cplx_wet = &hybrid_wet[START_BAND..];

                let wet_ener_x = izip!(qmf_output_cplx_wet, BP_GF)
                    .take(BP_N_BANDS)
                    .map(|(w, b)| w.norm_sqr() * b)
                    .sum::<f32>();

                *nrg_wet = (LPF_COEFF1 * *nrg_wet) + (ONE_MINUS_LPF_COEFF1 * wet_ener_x);
                *nrg_ld_wet = wet_ener_x / (nrg_ld_old_wet + ABS_THR);
            }
        }

        // Compute scale factor for the 'diffuse' signals.
        izip!(&mut scale, &wet_ener_ld).for_each(|(scale_ch, wet_nrg_ld_ch)| {
            *scale_ch = (dry_ener_ld / (*wet_nrg_ld_ch + ABS_THR)).sqrt()
        });

        let damp = 0.1f32;
        for (scale_ch, prev_scale_ch) in izip!(&mut scale, &mut self.prev_scale) {
            // Damp the scaling factor.
            *scale_ch = damp + (0.9f32 * *scale_ch);

            // Limit the scaling factor.
            *scale_ch = scale_ch.clamp(ONE_DIV_SCALE_LIMIT, SCALE_LIMIT);

            // Low-pass filter the scaling factor.
            *scale_ch = (LPF_COEFF2 * *scale_ch) + (ONE_MINUS_LPF_COEFF2 * *prev_scale_ch);
            *prev_scale_ch = *scale_ch;
        }

        // Combine 'direct' and scaled 'diffuse' signal.
        for (out_dry, out_wet, do_scaling, scale_ch) in izip!(
            &mut hyb_data.hyb_output_cplx_dry,
            &hyb_data.hyb_output_cplx_wet,
            &self.temp_shape_enable_channel_stp,
            &scale
        ) {
            let bands = if *do_scaling {
                HYB_BAND_BORDER
            } else {
                hyb_data.hybrid_bands
            };

            Self::combine_signal_cplx(&mut out_dry[..bands], &out_wet[..bands]);

            if *do_scaling {
                Self::combine_signal_cplx_scale1(
                    &mut out_dry[HYB_BAND_BORDER..HP_SIZE_HYB],
                    &out_wet[HYB_BAND_BORDER..HP_SIZE_HYB],
                    &BP,
                    *scale_ch,
                );

                Self::combine_signal_cplx_scale2(
                    &mut out_dry[HP_SIZE_HYB..hyb_data.hybrid_bands],
                    &out_wet[HP_SIZE_HYB..hyb_data.hybrid_bands],
                    *scale_ch,
                );
            }
        }
    }

    /// Initializes an STP instance.
    pub(super) fn init(&mut self) {
        *self = Default::default();
    }

    /// Creates a properly initialized STP instance.
    pub(super) fn new() -> Self {
        Default::default()
    }

    /// Clears STP specific data.
    pub(super) fn clear_data(&mut self) {
        self.temp_shape_enable_channel_stp.fill(false);
    }

    /// Reads STP specific data from `Bitstream`.
    /// # Parameters
    /// - `bs`: Bitstream instance with valid internal data (in).
    /// - `num_temp_shape_chan`: Number of channels, for STP to process.
    pub(super) fn read(&mut self, bs: &mut Bitstream, num_temp_shape_chan: usize) {
        for is_temp_shape_enabled in self
            .temp_shape_enable_channel_stp
            .iter_mut()
            .take(num_temp_shape_chan)
        {
            *is_temp_shape_enabled = bs.read_bit() != 0;
        }
    }

    #[inline(always)]
    fn combine_signal_cplx(
        hyb_output_cplx_dry: &mut [Complex<f32>],
        hyb_output_cplx_wet: &[Complex<f32>],
    ) {
        izip!(hyb_output_cplx_dry, hyb_output_cplx_wet).for_each(|(dry, wet)| *dry += *wet);
    }

    #[inline(always)]
    fn combine_signal_cplx_scale1(
        hyb_output_cplx_dry: &mut [Complex<f32>],
        hyb_output_cplx_wet: &[Complex<f32>],
        bp: &'static [f32; 3],
        scale_x: f32,
    ) {
        izip!(hyb_output_cplx_dry, hyb_output_cplx_wet)
            .enumerate()
            .for_each(|(n, (dry, wet))| *dry += *wet * scale_x * bp[n]);
    }

    #[inline(always)]
    fn combine_signal_cplx_scale2(
        hyb_output_cplx_dry: &mut [Complex<f32>],
        hyb_output_cplx_wet: &[Complex<f32>],
        scale_x: f32,
    ) {
        izip!(hyb_output_cplx_dry, hyb_output_cplx_wet)
            .for_each(|(dry, wet)| *dry += *wet * scale_x);
    }
}
