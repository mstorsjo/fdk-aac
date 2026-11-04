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
//! Decorrelator filter

use super::{decorr_common::DecorrType, decorr_constants::*, decorr_tables::*};
use itertools::izip;
use num_complex::Complex;

#[derive(Debug, Default, Copy, Clone)]
#[repr(C)]
pub(super) struct DecorrFilterInstance {
    pub(super) offset_delay_buffer: usize,
    pub(super) offset_state_buffer: usize,

    pub(super) numerator_real: Option<&'static [f32]>,
    coeffs_packed: Option<&'static [Complex<f32>]>,
}

impl DecorrFilterInstance {
    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply_cplx_ps(
        filter: &[DecorrFilterInstance],
        data_in: &[Complex<f32>],
        data_out: &mut [Complex<f32>],
        delay_buffer: &mut [Complex<f32>],
        state_buffer: &mut [Complex<f32>],
        num_bands: usize,
        reverb_filter_order: usize,
        offset_delay_buffer: usize,
        offset_state_buffer: &mut [usize; NUM_ALLPASS_LINKS],
    ) {
        // Traverse all hybrid-bands between start- and stop-index.
        for (band, (inp, out)) in izip!(data_in.iter(), data_out.iter_mut())
            .enumerate()
            .take(num_bands)
        {
            let idx = band * offset_delay_buffer;
            // 1. Input delay.
            let mut data_a = delay_buffer[idx];
            delay_buffer[idx] = *inp;
            // 2. Phi(k)-stage
            let phi_coeff = &filter[band].coeffs_packed.unwrap()[0..4];
            let coeff = &phi_coeff[0];

            // The first two entries of the coefficient table are the Phi(k)-multiplicants.
            let data_b = data_a * (*coeff);

            // 3. Process all three filter stages.

            // Stage 0
            let state = &mut state_buffer[band * reverb_filter_order + offset_state_buffer[0]];
            let coeff = &phi_coeff[1];

            // Multiply output of last stage by coefficient.
            let stage_mult = data_b * (*coeff);
            // Read and add value from state buffer (this is the input for the next stage).
            data_a = stage_mult + *state;
            // Perform complex mult. with shifted coeff and add stage input to the shifted result.
            let stage_mult = data_b - (data_a * (*coeff).conj());

            // Store result to state buffer.
            *state = stage_mult;

            // Stage 1
            let state = &mut state_buffer[band * reverb_filter_order + offset_state_buffer[1]];
            let coeff = &phi_coeff[2];

            let stage_mult = data_a * (*coeff);
            // Read and add value from state buffer (this is the input for the next stage).
            let data_b = stage_mult + *state;
            // Perform complex mult with shifted coeff and add stage input to the shifted result.
            let stage_mult = data_a - (data_b * (*coeff).conj());

            // Store result to state buffer.
            *state = stage_mult;

            // Stage 2
            let state = &mut state_buffer[band * reverb_filter_order + offset_state_buffer[2]];
            let coeff = &phi_coeff[3];

            let stage_mult = data_b * (*coeff);
            // Read and add value from state buffer (this is the input for the next stage).
            let data_a = stage_mult + *state;
            // Perform complex mult with shifted coeff and add stage input to the shifted result.
            let stage_mult = data_b - (data_a * (*coeff).conj());
            // Store result to state buffer.
            *state = stage_mult;

            // Write filter output
            *out = data_a;
        }

        // Update state_buffer_offset with respect to ring buffer boundaries.
        if offset_state_buffer[0] == 2 {
            offset_state_buffer[0] = 0;
        } else {
            offset_state_buffer[0] += 1;
        }
        if offset_state_buffer[1] == 6 {
            offset_state_buffer[1] = 3;
        } else {
            offset_state_buffer[1] += 1;
        }
        if offset_state_buffer[2] == 11 {
            offset_state_buffer[2] = 7;
        } else {
            offset_state_buffer[2] += 1;
        }
    }

    pub(super) fn apply_pass(
        data_in: &[Complex<f32>],
        data_out: &mut [Complex<f32>],
        delay_buffer: &mut [Complex<f32>],
        num_bands: usize,
        offset_delay_buffer: usize,
    ) {
        let mut idx = 0;
        for (inp, out) in izip!(data_in.iter(), data_out.iter_mut(),).take(num_bands) {
            *out = delay_buffer[idx];
            delay_buffer[idx] = *inp;
            idx += offset_delay_buffer;
        }
    }

    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply_real(
        filter_coeffs: &[f32],
        data_in: &[Complex<f32>],
        data_out: &mut [Complex<f32>],
        delay_buffer: &mut [Complex<f32>],
        state_buffer: &mut [Complex<f32>],
        num_bands: usize,
        reverb_filter_order: usize,
        offset_delay_buffer: usize,
    ) {
        debug_assert!(state_buffer.len() >= num_bands * reverb_filter_order);

        if reverb_filter_order == 2 {
            let filt0_low = filter_coeffs[0];
            let filt0_high = filter_coeffs[1];

            let mut state_chunk = state_buffer
                .chunks_exact_mut(reverb_filter_order)
                .take(num_bands);
            for (band, (inp, out)) in izip!(data_in.iter(), data_out.iter_mut())
                .enumerate()
                .take(num_bands)
            {
                let idx = band * offset_delay_buffer;
                let tmp_dly = delay_buffer[idx];
                let state = state_chunk.next().unwrap();

                delay_buffer[idx] = *inp;
                *out = state[0] + tmp_dly * filt0_low;
                state[0] = state[1] + tmp_dly * filt0_high - *out * filt0_high;
                state[1] = tmp_dly - *out * filt0_low;
            }
        } else if reverb_filter_order == 3 {
            let filt0_low = filter_coeffs[0];
            let filt0_high = filter_coeffs[1];
            let filt1_low = filter_coeffs[2];

            let mut state_chunk = state_buffer
                .chunks_exact_mut(reverb_filter_order)
                .take(num_bands);
            for (band, (inp, out)) in izip!(data_in.iter(), data_out.iter_mut())
                .enumerate()
                .take(num_bands)
            {
                let idx = band * offset_delay_buffer;
                let tmp_dly = delay_buffer[idx];
                let state = state_chunk.next().unwrap();

                delay_buffer[idx] = *inp;
                *out = state[0] + tmp_dly * filt0_low;

                state[0] = state[1] + tmp_dly * filt0_high - *out * filt1_low;
                state[1] = state[2] + (tmp_dly * filt1_low) - *out * filt0_high;
                state[2] = tmp_dly - *out * filt0_low;
            }
        } else if reverb_filter_order == 6 {
            let filt0_low = filter_coeffs[0];
            let filt0_high = filter_coeffs[1];
            let filt1_low = filter_coeffs[2];
            let filt1_high = filter_coeffs[3];
            let filt2_low = filter_coeffs[4];
            let filt2_high = filter_coeffs[5];

            let mut state_chunk = state_buffer
                .chunks_exact_mut(reverb_filter_order)
                .take(num_bands);
            for (band, (inp, out)) in izip!(data_in.iter(), data_out.iter_mut())
                .enumerate()
                .take(num_bands)
            {
                let idx = band * offset_delay_buffer;
                let tmp_dly = delay_buffer[idx];
                let state = state_chunk.next().unwrap();

                delay_buffer[idx] = *inp;
                *out = state[0] + tmp_dly * filt0_low;

                state[0] = state[1] + (tmp_dly * filt0_high - *out * filt2_high);
                state[1] = state[2] + (tmp_dly * filt1_low - *out * filt2_low);
                state[2] = state[3] + (tmp_dly * filt1_high - *out * filt1_high);
                state[3] = state[4] + (tmp_dly * filt2_low - *out * filt1_low);
                state[4] = state[5] + (tmp_dly * filt2_high - *out * filt0_high);
                state[5] = tmp_dly - *out * filt0_low;
            }
        } else {
            let mut filt0_low;
            let mut filt0_high;

            let mut state_chunk = state_buffer
                .chunks_exact_mut(reverb_filter_order)
                .take(num_bands);
            for (band, (inp, out)) in izip!(data_in.iter(), data_out.iter_mut())
                .enumerate()
                .take(num_bands)
            {
                let idx = band * offset_delay_buffer;
                let mut j = 0;
                let tmp_dly = delay_buffer[idx];
                let state = state_chunk.next().unwrap();

                delay_buffer[idx] = *inp;
                filt0_low = filter_coeffs[j];
                *out = state[j] + tmp_dly * filt0_low;
                j += 1;

                while j < reverb_filter_order {
                    filt0_low = filter_coeffs[j];
                    filt0_high = filter_coeffs[reverb_filter_order - j];

                    state[j - 1] = state[j] + tmp_dly * filt0_low - *out * filt0_high;

                    j += 1;
                }

                filt0_low = filter_coeffs[j];
                filt0_high = filter_coeffs[reverb_filter_order - j];
                state[j - 1] = tmp_dly * filt0_low - *out * filt0_high;
            }
        }
    }

    #[expect(clippy::too_many_arguments)]
    pub(super) fn init(
        &mut self,
        filter_order: usize,
        reverb_band: usize,
        hybrid_band: usize,
        num_sample_delay: usize,
        decorr_type: DecorrType,
        offset_state_buffer: &mut usize,
        offset_delay_buffer: &mut usize,
    ) {
        match decorr_type {
            DecorrType::Usac => match reverb_band {
                0 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL0_USAC[0]),
                1 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL1_USAC[0]),
                2 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL2_USAC[0]),
                3 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL3_USAC[0]),
                _ => self.numerator_real = None,
            },
            DecorrType::Ld => match reverb_band {
                1 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL1_LD[0]),
                2 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL2_LD[0]),
                3 => self.numerator_real = Some(&DECORR_NUMERATOR_REAL3_LD[0]),
                _ => self.numerator_real = None,
            },
            DecorrType::Ps => match reverb_band {
                0 => self.coeffs_packed = Some(&DECORR_PS_COEFFS_CPLX[hybrid_band]),
                _ => self.coeffs_packed = None,
            },
        }

        if decorr_type != DecorrType::Ps || (decorr_type == DecorrType::Ps && reverb_band == 0) {
            self.offset_state_buffer = *offset_state_buffer;
            *offset_state_buffer += filter_order;
        }

        self.offset_delay_buffer = *offset_delay_buffer;
        *offset_delay_buffer += num_sample_delay;
    }

    pub(super) fn new() -> DecorrFilterInstance {
        Default::default()
    }
}
