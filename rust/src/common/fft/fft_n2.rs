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
//! FFT calculation split into two smaller FFT sizes

use itertools::izip;
use num_complex::Complex;

/// Apply rotation vectors to `data` buffer. `data` is processed `dim1` samples at a time.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
/// - `dim1`: length of each row of input data.
/// - `rot_vec`: Input buffer to a rotation coefficient vector/table.
#[inline]
pub(super) fn fft_apply_rot_vector(
    data: &mut [Complex<f32>],
    dim1: usize,
    rot_vec: &[Complex<f32>],
) {
    let dim2 = data.len() / dim1;

    debug_assert!(rot_vec.len() == (dim1 - 1) * dim2);

    // apply rotational vector on data chunks of `dim1`.
    for (data_chunk, rot_vec_chunk) in
        izip!(data.chunks_exact_mut(dim1), rot_vec.chunks_exact(dim1 - 1)).take(dim2)
    {
        // this optimized version skips the 1st complex number from being multiplied with rot_vec
        for (c, rot_vec_item) in izip!(data_chunk.iter_mut().skip(1), rot_vec_chunk.iter()) {
            *c *= rot_vec_item.conj();
        }
    }
}

/// Perform an in-place complex valued FFT of size $dim_total, by combining the result of
/// `fft_$dim1()` and `fft_$dim2()`. Possible `$dim1_total` lengths: complex FFT of length 20, 24,
/// 48, 60, 80, 96, 120, 192, 240, 384, 480.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
macro_rules! fft_n2 {
    ($dim_total:literal, $dim1:expr, $dim2:expr) => {
        paste! {
        #[inline]
        pub(super) fn  [<fft_$dim_total>](data: &mut [Complex<f32>]) {
            let length = data.len();

            let rot_vector: &[Complex<f32>] = &fft_tables::[<ROT_VECTOR_$dim_total>];

            debug_assert!(length == $dim1 * $dim2);
            let work_buff1: &mut ArrayVec<Complex<f32>, {$dim1 * $dim2}> = &mut ArrayVec::new();
            let work_buff2: &mut ArrayVec<Complex<f32>, $dim2> = &mut ArrayVec::new();

            // Perform $dim2 times the fft of length $dim1. The input samples are at the
            // address of data[] and the output samples are at the address of dest[].
            for src_win in data.windows(1 + $dim2 * ($dim1 - 1)).take($dim2)
            {
                let offset = work_buff1.len();

                // Copy complex values from source (with $dim2 stride) to destination.
                for i in 0..$dim1 {
                    work_buff1.push(src_win[i * $dim2]);
                }

                // fft of size $dim1
                [<fft_$dim1>](&mut work_buff1[offset..]);
            }

            // Perform the modulation of the output of the fft of length dim1
            fft_apply_rot_vector(
                &mut work_buff1[$dim1..length],
                $dim1,
                rot_vector,
            );

            // Perform $dim1 times the fft of length $dim2. The input samples are at the
            // address of work_buff1[] and the output samples are at the address of data[].
            // The first iteration of the loop is unrolled here, because memory needs to be
            // marked as initialized after first iteration
            let dim_length = 1 + $dim1 * ($dim2 - 1);
            let i = 0;
            let src_win = &work_buff1[0..dim_length];
            let this_data = &mut data[i..(i + dim_length)];

            for src in src_win.iter().step_by($dim1).take($dim2)
            {
                work_buff2.push(*src);
            }

            [<fft_$dim2>](work_buff2); // fft of size $dim2

            for (src, dest) in izip!(
                work_buff2.iter(),
                this_data.iter_mut().step_by($dim1)
            )
            .take($dim2)
            {
                *dest = *src;
            }

            for (i, src_win) in izip!(1..$dim1, work_buff1[1..].windows(dim_length)) {
                let this_data = &mut data[i..(i + dim_length)];

                for j in 0..$dim2 {
                    work_buff2[j] = src_win[j * $dim1];
                }

                [<fft_$dim2>](work_buff2); // fft of size dim2

                for j in 0..$dim2 {
                    this_data[$dim1 * j] = work_buff2[j];
                }
            }
        }}
    };
}

#[cfg(test)]
mod tests {
    use super::super::fft_tables;
    use super::*;

    // Test:
    // Fill the input signal [f32] with Complex numbers { re: 1.0; im: 0.0}
    // Call fft_apply_rot_vector(), with corresponding ROT_VECTOR[] table.
    // Check that the phase of the complex numbers (i.e imaginary part) of the signal
    // was changed by 180 degree, compared with ROT_VECTOR[].
    #[test]
    fn t_fft_apply_rot_vec() {
        let rot_vec = &fft_tables::ROT_VECTOR_480;
        let mut cplx_data = [Complex { re: 1.0, im: 0.0 }; 480]; // max size

        // Execute DUT
        const CL: usize = 32;
        const N: usize = 32 * 15; // 480
        fft_apply_rot_vector(&mut cplx_data[CL..N], CL, rot_vec);

        // Test: Check that the phase of the signal is inverted, compared with rot_vec[].
        let mut rot_vec_item = rot_vec.iter();

        for c in cplx_data.iter_mut() {
            if c.im != 0.0 || (c.im == 0.0 && c.im.signum() == -1.0) {
                // iterate through all elements of rot_vec
                let tmp_rot_vec = rot_vec_item.next().unwrap();
                println!("c.re = {:.7}; rot_vec[].re = {:.7} ", c.re, tmp_rot_vec.re);
                println!("c.im = {:.7}; rot_vec[].im = {:.7} ", c.im, tmp_rot_vec.im);
                assert!(c.re.abs() - tmp_rot_vec.re.abs() < 0.001 && c.im + tmp_rot_vec.im < 0.001);
            }
        }
    }

    // Testing the multiplication of a complex number with the conjugate of another complex number.
    // E.g.: cplxMult(&vi, &vr, ui, ur, w_PiFOURTH);
    #[test]
    fn t_complex_mult() {
        let a = Complex {
            re: 1.0_f32,
            im: -0.5_f32,
        };
        let b = Complex {
            re: 0.5_f32,
            im: 0.5_f32,
        };
        let (ur, ui) = cplx_mult(&a.im, &a.re, &b.re, &b.im);

        // reference C function: cplxMult(&vi, &vr, ui, ur, w_PiFOURTH);
        let ref_res = Complex { re: ui, im: ur };
        println!("reference: a = {a}; b = {b}; ref_res = {ref_res}");

        // Rust
        let res = a * b.conj();
        // println!("test#3:    a = {}; b = {}; res = {}", a, b, res);

        assert!(res == ref_res);
    }

    fn cplx_mult(a_re: &f32, a_im: &f32, b_re: &f32, b_im: &f32) -> (f32, f32) {
        let c_re = (a_re * b_re) - (a_im * b_im);
        let c_im = (a_re * b_im) + (a_im * b_re);
        (c_re, c_im)
    }

    // Small routine to test whether fft_n2() could make use of iterators.
    #[test]
    fn t_window_overlap() {
        // simulate fft_20().
        let mut data = vec![0; 40];
        let dim1 = 4;
        let dim2 = 5;
        for (i, item) in data.iter_mut().enumerate() {
            *item = i;
        }
        let slice_len = (2 * (1 + dim2 * (dim1 - 1))) as usize;
        let src_win = &mut data.windows(slice_len).step_by(2);

        let mut win_cnt = 0;
        loop {
            println!("win = {win_cnt}");
            win_cnt += 1;
            let x = src_win.next();
            match x {
                Some(val) => {
                    let y = val;
                    for i in y.iter() {
                        print!("{} ", *i);
                    }
                    println!();
                }
                None => {
                    println!("no more samples to process. Exiting loop().");
                    break;
                }
            }
        }
    }
}
