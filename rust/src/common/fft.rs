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
//! Fast Fourier transform (FFT)

#[macro_use]
mod dit_fft;
#[macro_use]
mod fft_n2;
mod fft_tables;

use crate::common::tables::sine_tables;
use arrayvec::ArrayVec;
use dit_fft::*;
use fft_n2::*;
use itertools::izip;
use num_complex::Complex;
use paste::paste;
use std::f32::consts;

const N3: usize = 3;
const N5: usize = 5;
const N15: usize = 15;

const FC30: f32 = 0.5000000;
const FC31: f32 = -0.86602540;
const FC51: f32 = 0.95105652;
const FC52: f32 = -1.53884180;
const FC53: f32 = -0.36327126;
const FC54: f32 = 0.55901699;
const FC55: f32 = -1.25;
const FC61: f32 = 0.86602540;

#[inline]
fn sumdiff_pi_fourth(a: &Complex<f32>) -> Complex<f32> {
    let w = *a * consts::FRAC_1_SQRT_2;
    Complex {
        re: (w.im - w.re),
        im: (w.im + w.re),
    }
}

/// Perform an inplace complex valued FFT of length 2
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_2(data: &mut [Complex<f32>]) {
    let tmp = data[0] + data[1];

    data[1] = data[0] - data[1];
    data[0] = tmp;
}

/// Perform an inplace complex valued FFT of length 3 according to the algorithm after winograd.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_3(data: &mut [Complex<f32>]) {
    let mut c1 = data[1] + data[2];
    let c2 = (data[1] - data[2]) * FC31;
    let d = data[0];
    data[0] = d + c1;
    c1 = d - (c1 * FC30);

    // combination
    data[1].re = c1.re - c2.im;
    data[1].im = c1.im + c2.re;

    data[2].re = c1.re + c2.im;
    data[2].im = c1.im - c2.re;
}

/// Perform an inplace complex valued FFT of length 4
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_4(data: &mut [Complex<f32>]) {
    let a0 = data[0] + data[2]; // A + C
    let mut a1 = data[1] + data[3]; // B + D
    let tmp = data[0] - data[2]; // A - C

    data[0] = a0 + a1; // A' = A + B + C + D
    data[2] = a0 - a1; // C' = (A + C) - (B + D)

    a1 -= data[3] * 2.0; // B - D

    data[1].re = tmp.re + a1.im; // Re B' = Re A - Re C + Im B - Im D
    data[1].im = tmp.im - a1.re; // Im B' = Im A - Im C - Re B + Re D
    data[3].re = tmp.re - a1.im; // Re D' = Re A - Re C - Im B + Im D
    data[3].im = tmp.im + a1.re; // Im D' = Im A - Im C + Re B - Re D
}

/// Perform an inplace complex valued FFT of length 5 according to the algorithm after winograd.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_5(data: &mut [Complex<f32>]) {
    let (mut c1, mut c2, mut c3, mut c4, mut ct);

    c1 = data[1] + data[4];
    c4 = data[1] - data[4];
    c3 = data[2] + data[3];
    c2 = data[2] - data[3];

    ct = (c1 - c3) * FC54;
    c1 += c3;
    data[0] += c1;
    c1 = data[0] + (c1 * FC55);
    c3 = c1 - ct;
    c1 += ct;
    ct = (c4 + c2) * FC51;
    c4 = ct + (c4 * FC52);
    c2 = ct + (c2 * FC53);

    // combination
    data[1].re = c1.re + c2.im;
    data[4].re = c1.re - c2.im;

    data[2].re = c3.re - c4.im;
    data[3].re = c3.re + c4.im;

    data[1].im = c1.im - c2.re;
    data[4].im = c1.im + c2.re;
    data[2].im = c3.im + c4.re;
    data[3].im = c3.im - c4.re;
}

/// Perform an inplace complex valued FFT of length 6
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_6(data: &mut [Complex<f32>]) {
    let (c0, c1, c2, c3, c4, c5);
    let (i1e, i2e);

    c0 = data[0];
    c1 = data[1];
    c2 = data[2];
    c3 = data[3];
    c4 = data[4];
    c5 = data[5];

    let mut t = c0 + (c2 + c4);
    let mut s = c1 + (c3 + c5);

    data[0] = t + s;
    data[3] = t - s;

    t = c0 - ((c4 + c2) / 2.0);
    s.re = (c4.im - c2.im) * FC61;
    s.im = (c4.re - c2.re) * FC61;

    data[4].re = t.re - s.re;
    i1e = t.im + s.im;
    data[5].re = t.re + s.re;
    i2e = t.im - s.im;

    t = c1 - ((c5 + c3) / 2.0);
    s = (c5 - c3) * FC61;

    let (r1o, i1o) = (t.re - s.im, t.im + s.re);
    let (r2o, i2o) = (t.re + s.im, t.im - s.re);

    let mut rr = (r1o / 2.0) + i1o * FC61;
    let mut ss = (i1o / 2.0) - r1o * FC61;

    data[1].re = data[4].re + rr;
    data[1].im = i1e + ss;
    data[4].re -= rr;
    data[4].im = i1e - ss;

    rr = i2o * FC61 - (r2o / 2.0);
    ss = r2o * FC61 + (i2o / 2.0);

    data[2].re = data[5].re + rr;
    data[2].im = i2e - ss;
    data[5].re -= rr;
    data[5].im = i2e + ss;
}

/// Perform an inplace complex valued FFT of length 8
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline(always)]
fn fft_8(data: &mut [Complex<f32>]) {
    let mut y = [Complex { re: 0.0, im: 0.0 }; 8];

    let mut a1 = data[0] + data[4];
    let mut a2 = data[2] + data[6];

    y[0] = a1 + a2;
    y[2] = a1 - a2;

    a1 = data[0] - data[4];
    a2 = data[2] - data[6];

    y[1].re = a1.re + a2.im;
    y[1].im = a1.im - a2.re;
    y[3].re = a1.re - a2.im;
    y[3].im = a1.im + a2.re;

    a1 = data[1] + data[5];
    a2 = data[3] + data[7];

    y[4] = a1 + a2;
    y[6] = a1 - a2;

    a1 = data[1] - data[5];
    a2 = data[3] - data[7];

    y[5].re = a1.re + a2.im;
    y[5].im = a1.im - a2.re;
    y[7].re = a1.re - a2.im;
    y[7].im = a1.im + a2.re;

    let mut u = y[0];
    let mut v = y[4];
    data[0] = u + v;
    data[4] = u - v;

    u = y[2];
    v.re = y[6].im;
    v.im = y[6].re;

    data[2] = u + v.conj();
    data[6] = u - v.conj();

    v = y[5] * fft_tables::W_PI_FOURTH_TAB[0].conj();

    u = y[1];
    data[1] = u + v;
    data[5] = u - v;

    u.re = y[7].im;
    u.im = y[7].re;

    v = u * fft_tables::W_PI_FOURTH_TAB[0];

    u = y[3];
    data[3] = u + v.conj();
    data[7] = u - v.conj();
}

/// Perform an inplace complex valued FFT of length 10
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline]
fn fft_10(data: &mut [Complex<f32>]) {
    let (y0, y1, y2, y3, y4, y5, y6, y7, y8, y9);
    let (mut a0, mut a1, mut a2, mut a3, mut a4);
    let (mut c1, mut c2, mut c3, mut c4, mut ct);

    // 2 fft5 stages

    a0 = data[0];
    a1 = data[2];
    a2 = data[4];
    a3 = data[6];
    a4 = data[8];

    c1 = a3 + a2;
    c4 = a3 - a2;
    c3 = a1 + a4;
    c2 = a1 - a4;
    ct = (c1 - c3) * FC54;
    c1 += c3;

    y0 = a0 + c1;

    c1 = y0 + (c1 * FC55);
    c3 = c1 - ct;
    c1 += ct;
    ct = (c4 + c2) * FC51;
    c4 = ct + (c4 * FC52);
    c2 = ct + (c2 * FC53);

    // combination
    y2 = Complex {
        re: (c1.re + c2.im),
        im: (c1.im - c2.re),
    };
    y4 = Complex {
        re: (c3.re - c4.im),
        im: (c3.im + c4.re),
    };
    y6 = Complex {
        re: (c3.re + c4.im),
        im: (c3.im - c4.re),
    };
    y8 = Complex {
        re: (c1.re - c2.im),
        im: (c1.im + c2.re),
    };

    a0 = data[5];
    a1 = data[1];
    a2 = data[3];
    a3 = data[7];
    a4 = data[9];

    c1 = a1 + a4;
    c4 = a1 - a4;
    c3 = a3 + a2;
    c2 = a3 - a2;
    ct = (c1 - c3) * FC54;
    c1 += c3;

    y1 = a0 + c1;

    c1 = y1 + (c1 * FC55);
    c3 = c1 - ct;
    c1 += ct;
    ct = (c4 + c2) * FC51;
    c4 = ct + (c4 * FC52);
    c2 = ct + (c2 * FC53);

    // combination
    y3 = Complex {
        re: (c1.re + c2.im),
        im: (c1.im - c2.re),
    };
    y5 = Complex {
        re: (c3.re - c4.im),
        im: (c3.im + c4.re),
    };
    y7 = Complex {
        re: (c3.re + c4.im),
        im: (c3.im - c4.re),
    };
    y9 = Complex {
        re: (c1.re - c2.im),
        im: (c1.im + c2.re),
    };

    // 5 fft2 stages
    data[0] = y0 + y1;
    data[5] = y0 - y1;
    data[2] = y2 + y3;
    data[7] = y2 - y3;
    data[4] = y4 + y5;
    data[9] = y4 - y5;
    data[6] = y6 + y7;
    data[1] = y6 - y7;
    data[8] = y8 + y9;
    data[3] = y8 - y9;
}

/// Perform an inplace complex valued FFT of length 12
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline]
fn fft_12(data: &mut [Complex<f32>]) {
    let mut work_buff: ArrayVec<Complex<f32>, 12> = ArrayVec::new();

    let (mut c1, mut c2, mut d);
    let mut cnt = 0;

    let src: &[Complex<f32>] = data;

    c1 = src[cnt + 4] + src[cnt + 8];
    c2 = (src[cnt + 4] - src[cnt + 8]) * FC31;
    d = src[cnt];

    work_buff.push(d + c1);

    c1 = d - (c1 * 0.5);

    // combination
    work_buff.push(Complex {
        re: c1.re - c2.im,
        im: c1.im + c2.re,
    });
    work_buff.push(Complex {
        re: c1.re + c2.im,
        im: c1.im - c2.re,
    });

    cnt += 1;

    let mut rot_vec_item = fft_tables::ROT_VECTOR_12.iter();
    for _i in 0..2 {
        c1 = src[cnt + 4] + src[cnt + 8];
        c2 = (src[cnt + 4] - src[cnt + 8]) * FC31;
        d = src[cnt];

        work_buff.push(d + c1);

        c1 = d - (c1 * 0.5);

        // combination
        work_buff.push(
            Complex {
                re: (c1.re - c2.im),
                im: (c1.im + c2.re),
            } * rot_vec_item.next().unwrap().conj(),
        );
        work_buff.push(
            Complex {
                re: (c1.re + c2.im),
                im: (c1.im - c2.re),
            } * rot_vec_item.next().unwrap().conj(),
        );

        cnt += 1;
    }
    // sample 2,3 is complex multiplied with (0.0,1.0)
    // sample 4,5 is complex multiplied with (-1.0,0.0)

    c1 = src[cnt + 4] + src[cnt + 8];
    c2 = (src[cnt + 4] - src[cnt + 8]) * FC31;
    d = src[cnt];

    work_buff.push(d + c1);

    c1 = d - (c1 * 0.5);

    // combination
    work_buff.push(Complex {
        re: c1.im + c2.re,
        im: c2.im - c1.re,
    });
    work_buff.push(Complex {
        re: -(c1.re + c2.im),
        im: c2.re - c1.im,
    });

    // Perform 3 times the fft of length 4. The input samples are at the address
    // of work_buff and the output samples are at the address of data.
    let src = &work_buff[..];
    let dest = data;

    for cnt in 0..3 {
        // inline FFT4 merged with incoming resorting loop
        let (a0, mut a1, tmp);

        a0 = src[cnt] + src[cnt + 6]; // A + B
        a1 = src[cnt + 3] + src[cnt + 9]; // C + D

        dest[cnt] = a0 + a1; // A' = (A + B) + (C + D)
        dest[cnt + 6] = a0 - a1; // C' = (A + B) - (C + D)

        a1 = src[cnt + 3] - src[cnt + 9]; // C - D
        tmp = src[cnt] - src[cnt + 6]; // A - B

        dest[cnt + 3].re = tmp.re + a1.im; // Re B' = Re A - Re B + Im C - Im D
        dest[cnt + 3].im = tmp.im - a1.re; // Im B' = Im A - Im B - Re C + Re D
        dest[cnt + 9].re = tmp.re - a1.im; // Re D' = Re A - Re B - Im C + Im D
        dest[cnt + 9].im = tmp.im + a1.re; // Im D' = Im A - Im B + Re C - Re D
    }
}

/// Perform an inplace complex valued FFT of length 15. It is split into FFTs of length 3 and length
/// 5.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline]
fn fft_15(data: &mut [Complex<f32>]) {
    // work_buff is not read until it is filled, so this memory allocation on stack is safe.
    let mut work_buff1: ArrayVec<Complex<f32>, N15> = ArrayVec::new();
    let mut work_buff2: ArrayVec<Complex<f32>, N15> = ArrayVec::new();

    // Sort input vector for fft's of length 3
    // input3(0:2)   = [input(0) input(5) input(10)];
    // input3(3:5)   = [input(3) input(8) input(13)];
    // input3(6:8)   = [input(6) input(11) input(1)];
    // input3(9:11)  = [input(9) input(14) input(4)];
    // input3(12:14) = [input(12) input(2) input(7)];
    {
        debug_assert!(data.len() >= N15);
        work_buff1.push(data[0]);
        work_buff1.push(data[5]);
        work_buff1.push(data[10]);
        work_buff1.push(data[3]);
        work_buff1.push(data[8]);
        work_buff1.push(data[13]);
        work_buff1.push(data[6]);
        work_buff1.push(data[11]);
        work_buff1.push(data[1]);
        work_buff1.push(data[9]);
        work_buff1.push(data[14]);
        work_buff1.push(data[4]);
        work_buff1.push(data[12]);
        work_buff1.push(data[2]);
        work_buff1.push(data[7]);

        // Merge 3 loops into one, skip call of fft3
        for wb in work_buff1.chunks_exact_mut(N3).take(N5) {
            let mut c1 = wb[1] + wb[2];
            let c2 = (wb[1] - wb[2]) * FC31;
            c1 = wb[0] - (c1 * 0.5);

            wb[0] += wb[1] + wb[2];

            // combination
            wb[1] = Complex {
                re: c1.re - c2.im,
                im: c1.im + c2.re,
            };
            wb[2] = Complex {
                re: c1.re + c2.im,
                im: c1.im - c2.re,
            };
        }
    }

    // Sort input vector for fft's of length 5
    // input5(0:4)   = [output3(0) output3(3) output3(6) output3(9) output3(12)];
    // input5(5:9)   = [output3(1) output3(4) output3(7) output3(10) output3(13)];
    // input5(10:14) = [output3(2) output3(5) output3(8) output3(11) output3(14)];
    {
        let mut k = 0;
        for l in 0..N3 {
            work_buff2.push(work_buff1[l]);
            work_buff2.push(work_buff1[l + N3]);
            work_buff2.push(work_buff1[l + (2 * N3)]);
            work_buff2.push(work_buff1[l + (3 * N3)]);
            work_buff2.push(work_buff1[l + (4 * N3)]);

            fft_5(&mut work_buff2[k..k + N5]);

            k += 5;
        }
    }

    // Sort output vector of length 15
    // output = [out5(0)   out5(6)  out5(12) out5(3)  out5(9)
    //           out5(10)  out5(1)  out5(7)  out5(13) out5(4)
    //           out5(5)   out5(11) out5(2)  out5(8)  out5(14)];
    {
        data[0] = work_buff2[0];
        data[1] = work_buff2[6];
        data[2] = work_buff2[12];
        data[3] = work_buff2[3];
        data[4] = work_buff2[9];
        data[5] = work_buff2[10];
        data[6] = work_buff2[1];
        data[7] = work_buff2[7];
        data[8] = work_buff2[13];
        data[9] = work_buff2[4];
        data[10] = work_buff2[5];
        data[11] = work_buff2[11];
        data[12] = work_buff2[2];
        data[13] = work_buff2[8];
        data[14] = work_buff2[14];
    }
}

/// Perform an inplace complex valued FFT of length 16
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
#[inline]
fn fft_16(data: &mut [Complex<f32>]) {
    let mut v = data[0] + data[8]; // A + B
    let mut u = data[4] + data[12]; // C + D

    data[0] = v + u; // A' = A + B + C + D

    let mut v2 = data[2] + data[10]; // A + B

    data[2] = v - u; // C' = (A + B) - (C + D)

    v -= 2.0 * data[8]; // A - B
    u -= 2.0 * data[12]; // C - D

    let mut v3 = data[1] + data[9]; // A + B
    data[1].re = u.im + v.re; // Re B' = Im C - Im D  + Re A - Re B
    data[1].im = v.im - u.re; // Im B'= -Re C + Re D + Im A - Im B

    let mut v4 = data[3] + data[11]; // A + B
    data[3].re = v.re - u.im; // Re D' = -Im C + Im D + Re A - Re B
    data[3].im = u.re + v.im; // Im D'= Re C - Re D + Im A - Im B

    let mut u2 = data[6] + data[14]; // C + D
    data[4] = v2 + u2; // A' = (A + B) + (C + D)
    data[6] = v2 - u2; // C' = (A + B) - (C + D)

    v2 -= 2.0 * data[10]; // A - B
    u2 -= 2.0 * data[14]; // C - D

    u = data[5] + data[13]; // C + D
    data[5].re = u2.im + v2.re; // Re B' = Im C - Im D  + Re A - Re B
    data[5].im = v2.im - u2.re; // Im B'= -Re C + Re D + Im A - Im B

    let mut u3 = data[7] + data[15]; // C + D
    data[7].re = v2.re - u2.im; // Re D' = -Im C + Im D + Re A - Re B
    data[7].im = u2.re + v2.im; // Im D' =  Re C - Re D + Im A - Im B
    data[8] = v3 + u; // A' = (A + B) + (C + D)
    data[10] = v3 - u; // C' = (A + B) - (C + D)

    v3 -= 2.0 * data[9]; // A - B
    u -= 2.0 * data[13]; // C - D

    data[9].re = u.im + v3.re; // Re B' = Im C - Im D  + Re A - Re B
    data[9].im = v3.im - u.re; // Im B'= -Re C + Re D + Im A - Im B
    data[12] = v4 + u3; // A' = (A + B) + (C + D)
    data[14] = v4 - u3; // C' = (A + B) - (C + D)

    v4 -= 2.0 * data[11]; // A - B
    data[11].re = v3.re - u.im; // Re D' = -Im C + Im D + Re A - Re B
    data[11].im = u.re + v3.im; // Im D' =  Re C - Re D + Im A - Im B

    u3 -= 2.0 * data[15]; // C - D
    data[13].re = u3.im + v4.re; // Re B' =  Im C - Im D + Re A - Re B
    data[13].im = v4.im - u3.re; // Im B' = -Re C + Re D + Im A - Im B
    data[15].re = v4.re - u3.im; // Re D' = -Im C + Im D + Re A - Re B
    data[15].im = u3.re + v4.im; // Im D' =  Re C - Re D + Im A - Im B

    // datat1 =  0
    // datat2 =  4
    v = data[4];
    u = data[0];
    data[0] = u + v;
    data[4] = u - v;

    // datat1 =  2
    // datat2 =  6
    v = data[6];
    u = data[2];
    data[2].re = u.re + v.im;
    data[2].im = u.im - v.re;
    data[6].re = u.re - v.im;
    data[6].im = u.im + v.re;

    // datat1 =  8
    // datat2 = 12
    v = data[12];
    u = data[8];
    data[8] = u + v;
    data[12] = u - v;

    // datat1 = 10
    // datat2 = 14
    v = data[14];
    u = data[10];
    data[10].re = u.re + v.im;
    data[10].im = u.im - v.re;
    data[14].re = u.re - v.im;
    data[14].im = u.im + v.re;

    // datat1 =  1
    // datat2 =  5
    let tmp = sumdiff_pi_fourth(&data[5]);
    v = Complex {
        re: tmp.im,
        im: tmp.re,
    };
    u = data[1];
    data[1] = u + v;
    data[5] = u - v;

    // datat1 =  3
    // datat2 =  7
    v = sumdiff_pi_fourth(&data[7]);
    u = data[3];
    data[3].re = u.re + v.re;
    data[3].im = u.im - v.im;
    data[7].re = u.re - v.re;
    data[7].im = u.im + v.im;

    // datat1 =  9
    // datat2 = 13
    let tmp = sumdiff_pi_fourth(&data[13]);
    v = Complex {
        re: tmp.im,
        im: tmp.re,
    };
    u = data[9];
    data[9] = u + v;
    data[13] = u - v;

    // datat1 = 11
    // datat2 = 15
    v = sumdiff_pi_fourth(&data[15]);
    u = data[11];
    data[11].re = u.re + v.re;
    data[11].im = u.im - v.im;
    data[15].re = u.re - v.re;
    data[15].im = u.im + v.im;

    // datat1 =  0
    // datat2 =  8
    v = data[8];
    u = data[0];
    data[0] = u + v;
    data[8] = u - v;

    // datat1 =  4
    // datat2 = 12
    v = data[12];
    u = data[4];
    data[4].re = u.re + v.im;
    data[4].im = u.im - v.re;
    data[12].re = u.re - v.im;
    data[12].im = u.im + v.re;

    // datat1 =  1
    // datat2 =  9
    v = data[9] * fft_tables::FFT16_W16[0].conj();
    u = data[1];
    data[1] = u + v;
    data[9] = u - v;

    // datat1 =  5
    // datat2 = 13
    v = Complex {
        re: data[13].im,
        im: data[13].re,
    } * fft_tables::FFT16_W16[0];
    u = data[5];
    data[5].re = u.re + v.re;
    data[5].im = u.im - v.im;
    data[13].re = u.re - v.re;
    data[13].im = u.im + v.im;

    // datat1 =  2
    // datat2 = 10
    let tmp = sumdiff_pi_fourth(&data[10]);
    v = Complex {
        re: tmp.im,
        im: tmp.re,
    };
    u = data[2];
    data[2] = u + v;
    data[10] = u - v;

    // datat1 =  6
    // datat2 = 14
    v = sumdiff_pi_fourth(&data[14]);
    u = data[6];
    data[6].re = u.re + v.re;
    data[6].im = u.im - v.im;
    data[14].re = u.re - v.re;
    data[14].im = u.im + v.im;

    // datat1 =  3
    // datat2 = 11
    v = data[11] * fft_tables::FFT16_W16[1].conj();
    u = data[3];
    data[3] = u + v;
    data[11] = u - v;

    // datat1 =  7
    // datat2 = 15
    v = Complex {
        re: data[15].im,
        im: data[15].re,
    } * fft_tables::FFT16_W16[1];
    u = data[7];
    data[7].re = u.re + v.re;
    data[7].im = u.im - v.im;
    data[15].re = u.re - v.re;
    data[15].im = u.im + v.im;
}

fft_n2!(20, 4, 5);
fft_n2!(24, 2, 12);
dit_fft!(32, 32);
fft_n2!(48, 4, 12);
fft_n2!(60, 4, 15);
dit_fft!(64, 512);
fft_n2!(80, 5, 16);
fft_n2!(96, 3, 32);
fft_n2!(120, 8, 15);
dit_fft!(128, 512);
fft_n2!(192, 16, 12);
fft_n2!(240, 16, 15);
dit_fft!(256, 512);
fft_n2!(384, 12, 32);
fft_n2!(480, 32, 15);
dit_fft!(512, 512);

/// Perform an inplace complex valued FFT of length `data.len()`
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
///
/// # Examples
/// ```
/// use aac::common::fft;
/// use num_complex::Complex;
///
/// const N: usize = 4;
/// let mut data: [Complex<f32>; N] = [Complex { re: 1.0, im: 0.0 }; N];
///
/// fft::fft(&mut data); // will perform fft_4()
///
/// assert!(data[0] == Complex { re: 4.0, im: 0.0 });
/// ```
pub fn fft(data: &mut [Complex<f32>]) {
    match data.len() {
        2 => fft_2(data),
        3 => fft_3(data),
        4 => fft_4(data),
        5 => fft_5(data),
        6 => fft_6(data),
        8 => fft_8(data),
        10 => fft_10(data),
        12 => fft_12(data),
        15 => fft_15(data),
        16 => fft_16(data),
        20 => fft_20(data),
        24 => fft_24(data),
        32 => fft_32(data),
        48 => fft_48(data),
        60 => fft_60(data),
        64 => fft_64(data),
        80 => fft_80(data),
        96 => fft_96(data),
        120 => fft_120(data),
        128 => fft_128(data),
        192 => fft_192(data),
        240 => fft_240(data),
        256 => fft_256(data),
        384 => fft_384(data),
        480 => fft_480(data),
        512 => fft_512(data),
        _ => panic!("FFT length {} not supported!", data.len()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_pi_fourth() {
        let v = Complex { re: 1.0, im: 0.0 };
        let res = sumdiff_pi_fourth(&v);
        println!("res = {res}");
    }
}
