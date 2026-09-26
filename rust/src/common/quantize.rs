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
//! Quantization

use crate::common::{pow::scale_factor_2_exp, tables};

// Table: (FLOAT)(i & 7) / 8.f with  i = 0, 1, 2, 3, 4, 5, 6, 7
static INVERSE_QUANT_INTERPOLATION_TABLE: [f32; 8] = [
    0.000_f32, 0.125_f32, 0.250_f32, 0.375_f32, 0.500_f32, 0.625_f32, 0.750_f32, 0.875_f32,
];

pub const MAX_QUANTIZED_VALUE: u16 = 8191;

/// Inverse quantize one sfb. Each value of the sfb is processed accordingly to the formula:
/// - `spectrum[i] = Sign(spectrum[i]) * abs(spectrum[i])^(4/3) * 2^(scf/4).`
///
/// # Parameters
///
/// - `spectrum`:  output buffer (slice), which holds the inverse quantized output.
/// - `quant_spectrum`:  input buffer (slice) to the first line of the sfb to be inverse quantized.
/// - `abs_spec_local_max`:  absolute maximum spectrum value found in current sfb.
/// - `scf`:  scale factor of the sfb.
///
/// # Return
/// `()`
pub fn inverse_quantize_band(
    spectrum: &mut [f32],
    quant_spectrum: &[i16],
    abs_spec_local_max: u16,
    scf: i16,
) {
    let scale = scale_factor_2_exp(scf);
    let num_lines = spectrum.len();
    debug_assert!(quant_spectrum.len() >= num_lines);

    let inverse_quant_table = &tables::POW_4_OVER_3;
    // Set to the middle of the table (val = 0).
    let iq_tab_offset = tables::POW_4_OVER_3_TABLESIZE - 1;

    if abs_spec_local_max < tables::POW_4_OVER_3_TABLESIZE {
        for i in 0..num_lines {
            let q_idx = i32::from(quant_spectrum[i]) + i32::from(iq_tab_offset);
            // iq_tab_offset should always create >= 0 values.
            debug_assert!(q_idx >= 0);
            spectrum[i] = inverse_quant_table[q_idx as usize] * scale;
        }
    } else {
        for i in 0..num_lines {
            let spec_abs = quant_spectrum[i].unsigned_abs();
            if spec_abs < tables::POW_4_OVER_3_TABLESIZE {
                let q_idx = i32::from(quant_spectrum[i]) + i32::from(iq_tab_offset);
                debug_assert!(q_idx >= 0);
                spectrum[i] = inverse_quant_table[q_idx as usize] * scale;
            } else {
                let sign_spec = f32::from((quant_spectrum[i]).signum());

                let t0_idx = usize::from(spec_abs >> 3) + usize::from(iq_tab_offset);
                let t1_idx = t0_idx + 1;
                let td_idx = usize::from((spec_abs) & 7);

                let t0 = inverse_quant_table[t0_idx];
                let t1 = inverse_quant_table[t1_idx];
                let td = (t1 - t0) * INVERSE_QUANT_INTERPOLATION_TABLE[td_idx];

                spectrum[i] = (t0 + td) * sign_spec * 16.0 * scale;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Compare the value of the samples of output[] against ref_output[].
    // It fails if the difference is > DEVIATION_THRESHOLD.
    fn evaluate_inv_quant_output(output: &[f32], ref_output: &[f32]) {
        assert_eq!(output.len(), ref_output.len());

        let mut deviation: f64;
        const DEVIATION_THRESHOLD: f64 = 3.29674753186257798112e-06;
        let mut max_deviation_found: (usize, f64) = (0, 0.0);

        for i in 0..output.len() {
            if ref_output[i].abs() > 0.000000 {
                deviation = ((f64::from(output[i]) - f64::from(ref_output[i]))
                    / f64::from(ref_output[i]))
                .abs();
            } else {
                deviation = f64::from(output[i]);
            }

            // Store where the maximum deviation is found.
            if deviation > max_deviation_found.1 {
                max_deviation_found.0 = i; // index
                max_deviation_found.1 = deviation; // value
            }

            assert!(
                deviation < DEVIATION_THRESHOLD,
                "spectrum[{i}]: deviation = {deviation:.20e}"
            );
        }
        println!(
            "max deviation = {:.20e} @ spec[{}] = {}",
            max_deviation_found.1, max_deviation_found.0, output[max_deviation_found.0]
        );
    }

    // Implement ref software.
    fn inv_quant_band_reference(in_arr: &[i16], out_arr: &mut [f32], scf: i16) {
        let scale = f32::powf(2.0, 0.25 * scf as f32);
        for i in 0..in_arr.len() {
            let mut sign = 1.0;
            let mut q = in_arr[i];
            if q < 0 {
                let neg_q = q.checked_neg();
                if neg_q.is_some() {
                    q = -q;
                } else {
                    panic!("attempt to negate with overflow.");
                }

                sign = -sign;
            }
            out_arr[i] = sign * f32::powf(q.into(), 4.0 / 3.0) * scale;
        }
    }

    #[test]
    fn test_inverse_quantize_band() {
        // input data
        let mut quant_spectrum: Vec<i16> = Vec::new();
        let qboundary = i16::try_from(MAX_QUANTIZED_VALUE).unwrap();
        for i in -qboundary..=qboundary {
            quant_spectrum.push(i);
        }

        // output data
        let mut inv_quant_spectrum: Vec<f32> = vec![0.0; quant_spectrum.len()];
        let mut test_inv_quant_spec = inv_quant_spectrum.clone();

        // arguments
        let local_max = quant_spectrum
            .iter()
            .max_by_key(|x| x.unsigned_abs())
            .unwrap()
            .unsigned_abs();
        let scale_factor = 21; // as example/src/inverseQuantPrecFl.cpp

        // DUT
        inverse_quantize_band(
            &mut inv_quant_spectrum,
            &quant_spectrum,
            local_max,
            scale_factor,
        );

        // REF: inv quantize based on formula.
        inv_quant_band_reference(&quant_spectrum, &mut test_inv_quant_spec, scale_factor);

        // TEST: evaluate the result.
        evaluate_inv_quant_output(&inv_quant_spectrum, &test_inv_quant_spec);
    }
}
