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
//! DCT implementations of DCT IV, DST IV and DCT II
//!
//! Calculate standard DCTs. Implementations are based on a single,
//! standard complex FFT-kernel. These are specifically helpful in cases where
//! optimized FFT libraries are already available. The FFT used
//! in these implementation is the FFT of the `common` crate.
//
//! Supported lengths are:
//! * DCT IV: 8, 10, 16, 24, 32, 40, 48, 64, 96, 120, 128, 160, 192, 240, 256, 384, 480, 512, 768,
//!   960, 1024
//! * DST IV: 8, 10, 16, 24, 32, 40, 48, 64, 96, 120, 128, 160, 192, 240, 256, 384, 480, 512, 768,
//!   960, 1024
//! * DCT II: 8, 12, 16, 20, 24, 32, 40, 48, 64, 96, 120, 128

use crate::common::checked_cast;
use crate::common::fft::fft;
use crate::common::tables::sine_tables;
use crate::common::tables::window_tables;
use arrayvec::ArrayVec;
use itertools::izip;
use num_complex::Complex;
use std::f32::consts;

/// Calculate DCT type IV of `data.len()` length. The DCT IV is
/// calculated by a complex FFT, with some pre- and post twiddling.
/// A factor of `sqrt(2/p_in.len())` is NOT applied.
///
/// # Parameters
///
/// data input/output data (in place processing).
pub fn dctiv(data: &mut [f32]) {
    let dct_len = data.len();
    let fft_len = dct_len / 2;

    let mut accu1: Complex<f32>;
    let mut accu2: Complex<f32>;

    // get tables
    let twiddle =
        window_tables::get_table(dct_len as u16, window_tables::WindowShape::Sine).unwrap();

    // pre-twiddling
    {
        let (im_part, re_part) = data.split_at_mut(fft_len);

        let im_chunk = im_part.chunks_exact_mut(2);
        let re_chunk = re_part.rchunks_exact_mut(2);
        let twi_chunk = twiddle.chunks_exact(2);

        for (real, imag, twi) in izip!(re_chunk, im_chunk, twi_chunk) {
            accu1 = Complex {
                re: real[1],
                im: imag[0],
            } * twi[0];
            accu2 = Complex {
                re: real[0],
                im: imag[1],
            } * twi[1];

            (imag[0], imag[1]) = (accu1.im, accu1.re);
            (real[0], real[1]) = (accu2.im, -accu2.re);
        }
    }

    if (fft_len & 1) == 1 {
        accu1 = Complex {
            re: data[fft_len],
            im: data[fft_len - 1],
        } * twiddle[fft_len - 1];

        data[fft_len] = accu1.re;
        data[fft_len - 1] = accu1.im;
    }

    // fft
    let complex_data = checked_cast::mut_complex_f32_from_f32(data);
    fft(&mut complex_data[0..fft_len]);

    // post-twiddling
    let mut sin_step = 0;
    let sin_twiddle = sine_tables::get_table(dct_len as u16, &mut sin_step).unwrap();

    accu1 = Complex {
        re: data[dct_len - 2],
        im: data[dct_len - 2 + 1],
    };

    data[dct_len - 2 + 1] = -data[1];
    // data[0] = data[0]

    let data_upper_boundary = ((fft_len - 1) >> 1) * 2;

    {
        let mut sin_twi = sin_twiddle[sin_step];
        accu2 = accu1 * sin_twi;
        data[1] = accu2.re;
        data[dct_len - 2] = accu2.im;

        let l_data_last = data[fft_len - 1];
        let r_data_last = data[fft_len];

        let (left, right) = data.split_at_mut(fft_len);
        let le_chunk = left.chunks_exact_mut(2).skip(1);
        let ri_chunk = right.rchunks_exact_mut(2).skip(1);

        let mut k = 1;
        for (l_data, r_data) in izip!(le_chunk, ri_chunk).take(data_upper_boundary >> 1) {
            accu2 = Complex {
                re: l_data[1],
                im: l_data[0],
            } * sin_twi;

            accu1.re = r_data[0];
            accu1.im = r_data[1];

            r_data[1] = -accu2.re;
            l_data[0] = accu2.im;

            k += 1;
            sin_twi = sin_twiddle[k * sin_step];

            accu2 = accu1 * sin_twi;
            l_data[1] = accu2.re;
            r_data[0] = accu2.im;
        }

        if (fft_len & 1) == 1 {
            // Handle odd fft length.
            accu2 = Complex {
                re: r_data_last,
                im: l_data_last,
            } * sin_twi;

            data[fft_len] = -accu2.re;
            data[fft_len - 1] = accu2.im;
        }
    }

    if (fft_len & 1) == 0 {
        accu1 *= consts::FRAC_1_SQRT_2;

        data[dct_len - 4 - (data_upper_boundary - 2)] = accu1.re + accu1.im;
        data[data_upper_boundary - 2 + 2 + 1] = accu1.re - accu1.im;
    }
}

/// Calculate DST type IV of given length. The DST IV is
/// calculated by a complex FFT, with some pre- and post-twiddeling.
/// A factor of `sqrt(2/p_in.len())` is NOT applied.
///
/// # Parameters
///
/// data input/output data (in place processing).
pub fn dstiv(data: &mut [f32]) {
    let dst_len = data.len(); // L
    let fft_len = dst_len / 2; // M

    let mut accu1: Complex<f32>;
    let mut accu2: Complex<f32>;

    // get tables
    let twiddle =
        window_tables::get_table(dst_len as u16, window_tables::WindowShape::Sine).unwrap();
    {
        let (im_part, re_part) = data.split_at_mut(fft_len);
        let im_chunk = im_part.chunks_exact_mut(2);
        let re_chunk = re_part.rchunks_exact_mut(2);
        let twi_chunk = twiddle.chunks_exact(2);

        for (real, imag, twi) in izip!(re_chunk, im_chunk, twi_chunk) {
            accu1 = Complex {
                re: real[1],
                im: -imag[0],
            } * twi[0];
            accu2 = Complex {
                re: -real[0],
                im: imag[1],
            } * twi[1];

            (imag[0], imag[1]) = (accu1.im, accu1.re);
            (real[0], real[1]) = (accu2.im, -accu2.re);
        }
    }

    if (fft_len & 1) == 1 {
        accu1 = Complex {
            re: data[fft_len],
            im: -data[fft_len - 1],
        } * twiddle[fft_len - 1];

        data[fft_len] = accu1.re;
        data[fft_len - 1] = accu1.im;
    }

    // fft
    let complex_data = checked_cast::mut_complex_f32_from_f32(data);
    fft(&mut complex_data[0..fft_len]);

    // post-twiddling
    let mut sin_step = 0;
    let sin_twiddle = sine_tables::get_table(dst_len as u16, &mut sin_step).unwrap();

    accu1 = Complex {
        re: data[dst_len - 2],
        im: data[dst_len - 2 + 1],
    };

    data[dst_len - 2 + 1] = -data[0];
    data[0] = data[1];

    let data_upper_boundary = ((fft_len - 1) >> 1) * 2;

    {
        let mut sin_twi = sin_twiddle[sin_step];
        accu2 = accu1 * sin_twi;
        data[1] = -accu2.im;
        data[dst_len - 2] = -accu2.re;

        let l_data_last = data[fft_len - 1];
        let r_data_last = data[fft_len];

        let (left, right) = data.split_at_mut(fft_len);
        let le_chunk = left.chunks_exact_mut(2).skip(1);
        let ri_chunk = right.rchunks_exact_mut(2).skip(1);

        let mut k = 1;
        for (l_data, r_data) in izip!(le_chunk, ri_chunk).take(data_upper_boundary >> 1) {
            accu2 = Complex {
                re: l_data[1],
                im: l_data[0],
            } * sin_twi;

            accu1.re = r_data[0];
            accu1.im = r_data[1];

            r_data[1] = -accu2.im;
            l_data[0] = accu2.re;

            k += 1;
            sin_twi = sin_twiddle[k * sin_step];

            accu2 = accu1 * sin_twi;
            l_data[1] = -accu2.im;
            r_data[0] = -accu2.re;
        }

        if (fft_len & 1) == 1 {
            // Handle odd fft length.
            accu2 = Complex {
                re: r_data_last,
                im: l_data_last,
            } * sin_twi;

            data[fft_len] = -accu2.im;
            data[fft_len - 1] = accu2.re;
        }
    }

    if (fft_len & 1) == 0 {
        accu1 *= consts::FRAC_1_SQRT_2;

        data[data_upper_boundary - 2 + 2 + 1] = -accu1.re - accu1.im;
        data[dst_len - 4 - (data_upper_boundary - 2)] = accu1.im - accu1.re;
    }
}

/// Calculate DCT type II of given length. The DCT IV is
/// calculated by a complex FFT, with some pre- and post-twiddeling.
/// A factor of `sqrt(2/p_in.len())` is NOT applied.
///
/// # Parameters
///
/// data input/output data (in place processing).
pub fn dctii(data: &mut [f32]) {
    let dct_len = data.len();
    let fft_len = dct_len / 2;
    let fft_len_half = fft_len / 2;

    // tmp buffer for output
    let mut work_buff: ArrayVec<f32, 1024> = ArrayVec::new();

    assert!(dct_len % 4 == 0);

    // get tables
    let mut sin_step: usize = 0;
    let sin_twiddle = sine_tables::get_table(dct_len as u16, &mut sin_step).unwrap();
    sin_step >>= 1;

    // pre-twiddling
    for d in data[..2 * fft_len].iter().step_by(2) {
        work_buff.push(*d);
    }

    for d in data[..2 * fft_len].iter().rev().step_by(2) {
        work_buff.push(*d);
    }

    // fft
    let complex_work_buff = checked_cast::mut_complex_f32_from_f32(work_buff.as_mut_slice());
    fft(&mut complex_work_buff[0..fft_len]);

    // post-twiddling
    let mut index: usize = sin_step * 4;
    let index_step: usize = index;
    let loop_split = (fft_len / 2).div_ceil(2);

    let (cplx_sec0, cplx_sec1) = complex_work_buff.split_at(fft_len_half);
    let (cplx_l0_a, cplx_l1_a) = cplx_sec0.split_at(loop_split);
    let (cplx_l1_b, cplx_l0_b) = cplx_sec1.split_at(fft_len_half - loop_split + 1);

    let (data_left, data_right) = data.split_at_mut(fft_len);
    let (left_p0, left_p1) = data_left.split_at_mut(fft_len_half);
    let (right_p0, right_p1) = data_right.split_at_mut(fft_len_half);

    let (l0_p0, l1_p0) = left_p0.split_at_mut(loop_split);
    let (l1_p1, l0_p1) = left_p1.split_at_mut(fft_len_half - loop_split + 1);
    let (l0_p2, l1_p2) = right_p0.split_at_mut(loop_split);
    let (l1_p3, l0_p3) = right_p1.split_at_mut(fft_len_half - loop_split + 1);

    let mut data_iter_first = izip!(
        l0_p0.iter_mut().skip(1),
        l0_p1.iter_mut().rev(),
        l0_p2.iter_mut().skip(1),
        l0_p3.iter_mut().rev(),
    )
    .take(loop_split - 1);

    let mut i = 1;
    for (a, b) in izip!(cplx_l0_a.iter().skip(1), cplx_l0_b.iter().rev()).take(loop_split - 1) {
        let accu1 = (*b - a.conj()) * sin_twiddle[index];
        let accu2 = *a + b.conj();

        let mut c = Complex::new(accu2.re + accu1.im, -(accu1.re + accu2.im));
        let mut accu3 = c * sin_twiddle[i * sin_step];

        let (p0, p1, p2, p3) = data_iter_first.next().unwrap();
        *p3 = accu3.im * 0.5;
        *p0 = accu3.re * 0.5;

        c = Complex::new(accu2.re - accu1.im, -(accu1.re - accu2.im));
        accu3 = c * sin_twiddle[(fft_len - i) * sin_step];
        *p2 = accu3.im * 0.5;
        *p1 = accu3.re * 0.5;

        index += index_step;
        i += 1;
    }

    // if m/2 is odd, revert increasing the index
    if fft_len_half & 1 != 0 {
        index -= index_step;
    }

    let mut data_iter = izip!(
        l1_p0.iter_mut(),
        l1_p1.iter_mut().rev(),
        l1_p2.iter_mut(),
        l1_p3.iter_mut().rev()
    )
    .take(fft_len_half - loop_split);

    let mut j = loop_split;
    for (a, b) in izip!(cplx_l1_a.iter(), cplx_l1_b.iter().rev()).take(fft_len_half - loop_split) {
        let accu1 = -((Complex::new(a.im + b.im, b.re - a.re) * sin_twiddle[index]).conj());
        let accu2 = Complex::new(a.re + b.re, a.im - b.im);

        let mut c = Complex::new(accu2.re + accu1.im, -(accu1.re + accu2.im));
        let mut accu3 = c * sin_twiddle[j * sin_step];

        let (p0, p1, p2, p3) = data_iter.next().unwrap();
        *p3 = accu3.im * 0.5;
        *p0 = accu3.re * 0.5;

        c = Complex::new(accu2.re - accu1.im, -(accu1.re - accu2.im));
        accu3 = c * sin_twiddle[(fft_len - j) * sin_step];
        *p2 = accu3.im * 0.5;
        *p1 = accu3.re * 0.5;

        // Create index helper variables for (4*i)*inc indexed equivalent values of short tables.
        index -= index_step;
        j += 1;
    }

    let accu = Complex::new(work_buff[fft_len], work_buff[fft_len + 1])
        * sin_twiddle[fft_len_half * sin_step];
    data[dct_len - fft_len_half] = accu.im;
    data[fft_len_half] = accu.re;

    data[0] = work_buff[0] + work_buff[1];
    data[fft_len] = (work_buff[0] - work_buff[1]) * sin_twiddle[fft_len * sin_step].re;
    // cos((PI/(2*L))*M);
}

#[cfg(test)]
mod tests {
    use crate::common::dct;
    use std::f32::consts::PI;

    pub fn generate_sine(data: &mut [f32], f: f32, fs: f32) {
        let t = 1.0 / fs;
        for (i, dst) in data.iter_mut().enumerate() {
            *dst = (2.0 * PI * f * t * (i as f32)).sin();
        }
        // println!("{:.5?}", x);
    }

    pub fn compare_signals(p_data: &[f32], p_reference: &[f32], threshold: f32) -> i32 {
        let mut max_diff: f32 = 0.0;
        let mut max_value: f32 = 0.0;

        if p_data.len() == p_reference.len() {
            for n in 0..p_reference.len() {
                max_diff = max_diff.max((p_reference[n] - p_data[n]).abs());
                max_value = max_value.max(p_reference[n].abs());
            }
            let bits = (max_diff / max_value).log2().abs();

            if bits > threshold {
                println!("accuracy in bits; {bits} passed");
                0 // all fine
            } else {
                println!("accuracy in bits: {bits} - failed, below threshold {threshold}.");
                1 // accuracy too low
            }
        } else {
            1 // data length differs from reference
        }
    }

    pub fn dct4_ref(data: &[f32], p_out: &mut [f32]) {
        let dctiv_len = data.len();
        let len_flt = dctiv_len as f64;

        for k in (0..dctiv_len).step_by(1) {
            let mut mysum = 0.0;
            for n in (0..dctiv_len).step_by(1) {
                let k_flt = k as f64;
                let n_flt = n as f64;
                let val = data[n] as f64;
                mysum += val * ((PI as f64) / len_flt * (n_flt + 0.5) * (k_flt + 0.5)).cos();
            }
            p_out[k] = mysum as f32;
        }
    }

    pub fn dst4_ref(data: &[f32], p_out: &mut [f32]) {
        let dstiv_len = data.len();
        let len_flt = dstiv_len as f64;

        for k in (0..dstiv_len).step_by(1) {
            let mut mysum: f64 = 0.0;
            for n in (0..dstiv_len).step_by(1) {
                let k_flt = k as f64;
                let n_flt = n as f64;
                let val = data[n] as f64;
                mysum += val * ((PI as f64) / len_flt * (n_flt + 0.5) * (k_flt + 0.5)).sin();
            }
            p_out[k] = mysum as f32;
        }
    }

    pub fn dct2_ref(data: &[f32], p_out: &mut [f32]) {
        let dctiv_len = data.len();
        let len_flt = dctiv_len as f64;

        for k in (0..dctiv_len).step_by(1) {
            let mut mysum = 0.0;
            for n in (0..dctiv_len).step_by(1) {
                let k_flt = k as f64;
                let n_flt = n as f64;
                let val = data[n] as f64;
                mysum += val * ((PI as f64) / len_flt * (n_flt + 0.5) * (k_flt)).cos();
            }
            p_out[k] = mysum as f32;
        }
    }

    #[derive(Debug)]
    enum TransformType {
        _DCTI,
        _DSTI,
        DctII,
        _DSTII,
        _DCTIII,
        _DSTIII,
        DctIV,
        DstIV,
    }

    const TTTT: [TransformType; 3] = [
        TransformType::DctIV,
        TransformType::DstIV,
        TransformType::DctII,
    ]; // Transform Types To Test

    const TSTT: [usize; 21] = [
        8, 10, 16, 24, 32, 40, 48, 64, 96, 120, 128, 160, 192, 240, 256, 384, 480, 512, 768, 960,
        1024,
    ]; // Transform Sizes To Test

    #[test]
    fn cmp_to_ref() {
        for tt in TTTT {
            for ts in TSTT {
                let mut p_data = vec![0f32; ts];

                generate_sine(&mut p_data, 1500.0, 48000.0);
                let p_data_ref = p_data.clone();
                let mut p_reference = vec![0f32; ts];

                println!("! TEST {tt:?}-{ts} !");

                match tt {
                    TransformType::DctII => {
                        if p_data.len() % 4 != 0 {
                            println!("  --> skip, only lengths %4==0 are valid");
                            continue;
                        }
                        if p_data.len() > 128 {
                            println!("  --> skip, only lengths <128 are valid");
                            continue;
                        }
                        dct::dctii(&mut p_data);
                        dct2_ref(&p_data_ref, &mut p_reference);
                    }
                    TransformType::DctIV => {
                        dct::dctiv(&mut p_data);
                        dct4_ref(&p_data_ref, &mut p_reference);
                    }
                    TransformType::DstIV => {
                        dct::dstiv(&mut p_data);
                        dst4_ref(&p_data_ref, &mut p_reference);
                    }
                    _ => {
                        panic!("Not implemented")
                    }
                }

                assert_eq!(
                    compare_signals(&p_data, &p_reference, 16.0),
                    0,
                    "Compare_signals call failed."
                );
            }
        }

        println!("! TEST DONE !");
    }
}
