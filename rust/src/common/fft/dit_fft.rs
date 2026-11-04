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
//! Radix-2 decimation in time (DIT) FFT

use num_complex::Complex;
use std::f32::consts;

/// Scramble `data` (i.e. bit reversal of input data).
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
fn scramble(data: &mut [Complex<f32>]) {
    let n: usize = data.len();

    match n {
        32 => {
            data.swap(1, 16);
            data.swap(2, 8);
            data.swap(3, 24);
            data.swap(5, 20);
            data.swap(6, 12);
            data.swap(7, 28);
            data.swap(9, 18);
            data.swap(11, 26);
            data.swap(13, 22);
            data.swap(15, 30);
            data.swap(19, 25);
            data.swap(23, 29);
        }
        64 => {
            // Note: This version needs on ARM926 only 33% of the cycles compared to
            // the generic loop below
            // This table indicates, which entries of the vector should be exchanged.
            #[rustfmt::skip]
            static BR_OFFSETS64: [[usize; 2]; 28] = [
                [1, 32],  [2, 16],  [3, 48],  [4, 8],   [5, 40],  [6, 24],  [7, 56],
                [9, 36],  [10, 20], [11, 52], [13, 44], [14, 28], [15, 60], [17, 34],
                [19, 50], [21, 42], [22, 26], [23, 58], [25, 38], [27, 54], [29, 46],
                [31, 62], [35, 49], [37, 41], [39, 57], [43, 53], [47, 61], [55, 59]
            ];
            for i in (0..28).rev() {
                data.swap(BR_OFFSETS64[i][0], BR_OFFSETS64[i][1]);
            }
        }
        128 => {
            // In order to bit-reverse an 6-bit value, the lookup
            // table must be used twice:
            // rev(n) = bitreverse[n&0xF]<<2 + bitreverse[n>>4]>>2
            static BITREVERSE: [u8; 16] = [
                0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF,
            ];

            // These bits indicate, if the entry x[n] has to be exchanged or not.
            // The last words are skipped and replaced by hard-coded exchanges at
            // the procedure end. The table was generated with a C-code.
            static BITS: [u32; 3] = [0xEEEEFEFE, 0xAAAAEAEA, 0x8888A8A8 /* 0x00008080 */];

            let mut n = 0;
            for item in BITS.iter().take(2) {
                // bits[0,1]
                let mut bits16 = *item;
                for _j in 0..16 {
                    let rev_n = ((BITREVERSE[n & 0xF] << 2) + (BITREVERSE[n >> 4] >> 2)) as usize;
                    data.swap(2 * n + 1, rev_n + 64);
                    if bits16 & 1 != 0 {
                        data.swap(2 * n, rev_n);
                    }
                    bits16 >>= 2;
                    n += 1;
                }
            }

            let mut bits16 = BITS[2];
            for _j in 0..8 {
                if bits16 & 2 != 0 {
                    let rev_n = ((BITREVERSE[n & 0xF] << 2) + (BITREVERSE[n >> 4] >> 2)) as usize;
                    data.swap(2 * n + 1, rev_n + 64);
                }
                n += 1;
                if bits16 & 8 != 0 {
                    let rev_n = ((BITREVERSE[n & 0xF] << 2) + (BITREVERSE[n >> 4] >> 2)) as usize;
                    data.swap(2 * n + 1, rev_n + 64);
                }
                bits16 >>= 4;
                n += 1;
            }
            data.swap(103, 115);
            data.swap(111, 123);
        }
        256 => {
            // In order to bit-reverse a 7-bit value, the lookup
            // table must be used twice:
            // rev_n = (bitreverse[n&0xF]<<3)+(bitreverse[n>>4]>>1)
            static BITREVERSE: [u8; 16] = [
                0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF,
            ];

            // These bits indicate, if the entry x[n] has to be exchanged or not.
            // The last words are skipped and replaced by hard-coded exchange at
            // the procedure end. The table was generated with a C-code.
            // Note: The table is the same as for case 512, but packed in LONG
            static BITS: [u32; 7] = [
                0xFEFEFFFE, 0xEEEEFEEE, 0xEAEAEEEA, 0xAAAAEAAA, 0xA8A8AAA8, 0x8888A888,
                0x80808880, // 0x00008000
            ];
            let mut n = 0;
            for bits_item in BITS.iter() {
                let mut bits16: u32 = *bits_item;
                for _j in 0..16 {
                    if bits16 & 2 != 0 {
                        // If bit #1 is not set, then bit #0 is also not set - see table
                        // 'bits' above
                        let rev_n =
                            ((BITREVERSE[n & 0xF] << 3) + (BITREVERSE[n >> 4] >> 1)) as usize;
                        data.swap(2 * n + 1, rev_n + 128);
                        if bits16 & 1 != 0 {
                            data.swap(2 * n, rev_n);
                        }
                    }
                    bits16 >>= 2;
                    n += 1;
                }
                data.swap(239, 247);
            }
        }
        512 => {
            // A 4-bit bit-reverse is done via lookup-table.
            // Usage: rev(n) = bitreverse[n];  with n = [0..15]
            // In order to bit-reverse an 8-bit value, the lookup
            // table must be used for each nibble:
            // rev(n) = bitreverse[n&0xF]<<4 + bitreverse[n>>4];
            static BITREVERSE: [u8; 16] = [
                0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF,
            ];

            // These bits indicate, if the entry x[n] has to be exchanged or not.
            // Each word must be used twice, the table is thus reduced to the half.
            // The last words are skipped and replaced by hard-coded exchanges at
            // the procedure end. The table was generated with a C-code.
            static BITS: [u32; 14] = [
                0xFFFE, 0xFEFE, 0xFEEE, 0xEEEE, 0xEEEA, 0xEAEA, 0xEAAA, 0xAAAA, 0xAAA8, 0xA8A8,
                0xA888, 0x8888, 0x8880, 0x8080, // 0x8000, 0x0000
            ];

            let mut n = 0;
            for bits_item in BITS.iter() {
                // duplicate bits16 into high and low part of 32-bit
                let mut bits16 = *bits_item + (*bits_item << 16);

                for _j in 0..16 {
                    if bits16 & 2 != 0 {
                        let rev_n = ((BITREVERSE[n & 0xF] << 4) + BITREVERSE[n >> 4]) as usize;
                        data.swap(2 * n + 1, rev_n + 256);
                        if bits16 & 1 != 0 {
                            data.swap(2 * n, rev_n);
                        }
                    }
                    bits16 >>= 2;
                    n += 1;
                }
            }
            data.swap(463, 487);
            data.swap(479, 503);
        }
        _ => {
            let ldn = n.trailing_zeros();
            for m in 1..n - 1 {
                let j = m.reverse_bits() >> (usize::BITS - ldn);
                if j > m {
                    data.swap(m, j);
                }
            }
        }
    }
}

/// Perform an inplace complex valued FFT of length $length.
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
macro_rules! dit_fft {
    ($length:literal, $sine_table_sfx:literal) => {
        paste! {
            fn [<fft_$length>](data: &mut [Complex<f32>]) {
                dit_fft(data, &sine_tables::[<SINE_TABLE_$sine_table_sfx>]);
            }
        }
    };
}

/// Perform inplace complex valued FFT of `data.len()` length.
/// It is based on dit-tuckey-algorithm. It scrambles data at entry (i.e. loop is made with
/// scrambled data).
///
/// # Parameters
///
/// - `data`: Input/Output data buffer.
/// - `trigdata`: sine table of length of at least `data.len()` sine values.
pub(super) fn dit_fft(data: &mut [Complex<f32>], trigdata: &[Complex<f32>]) {
    let n: usize = data.len();
    let ldn = n.trailing_zeros();

    scramble(data);

    // 1+2 stage radix 4s
    for data_in in data.chunks_exact_mut(4).take(n >> 2) {
        let a0 = data_in[0] + data_in[1];
        let a1 = data_in[2] + data_in[3];
        let s0 = data_in[0] - data_in[1];
        let s1 = data_in[2] - data_in[3];

        // Re A' = Re A + Re B + Re C + Re D
        // Im A' = Im A + Im B + Im C + Im D
        data_in[0] = a0 + a1;

        data_in[1].re = s0.re + s1.im; // Re B' = Re A - Re B + Im C - Im D
        data_in[1].im = s0.im - s1.re; // Im B' = Im A - Im B - (Re C - Re D)

        // Re C' = Re A + Re B - (Re C + Re D)
        // Im C' = Im A + Im B - (Im C + Im D)
        data_in[2] = a0 - a1;

        data_in[3].re = s0.re - s1.im; // Re D' = Re A - Re B - (Im C - Im D)
        data_in[3].im = s0.im + s1.re; // Im D' = Im A - Im B + Re C - Re D
    }

    let v_tmp0 = Complex {
        re: consts::FRAC_1_SQRT_2,
        im: -consts::FRAC_1_SQRT_2,
    };
    let v_tmp1 = Complex {
        re: consts::FRAC_1_SQRT_2,
        im: consts::FRAC_1_SQRT_2,
    };

    for ldm in 3..(ldn as usize) + 1 {
        let m = 1 << ldm;
        let mh = m >> 1;
        let mq = m >> 2;

        let trigstep = ((trigdata.len() - 1) << (2 + 1)) >> ldm;

        debug_assert!(trigstep > 0);

        // Do first iteration with c=1.0 and s=0.0 separately to avoid loosing too
        // much precision. The impact on the overal FFT precision is large.
        {
            // block 1

            let j = 0;
            for inc in 0..n / m {
                let r = inc * m;
                let mut t1 = r + j;
                let mut t2 = t1 + mh;

                let v = data[t2];
                let u = data[t1];

                data[t1] = u + v;
                data[t2] = u - v;

                t1 += mq;
                t2 = t1 + mh;

                let v = data[t2];
                let u = data[t1];

                data[t1].re = u.re + v.im;
                data[t1].im = u.im - v.re;

                data[t2].re = u.re - v.im;
                data[t2].im = u.im + v.re;
            }
        } // end of  block 1

        for j in 1..(mh >> 2) {
            let cs = trigdata[j * trigstep];

            for inc in 0..n / m {
                let r = inc * m;
                let mut t1 = r + j;
                let mut t2 = t1 + mh;

                let mut v = data[t2] * cs.conj();
                let u = data[t1];

                data[t1] = u + v;
                data[t2] = u - v;

                t1 += mq;
                t2 = t1 + mh;

                v.re = data[t2].im;
                v.im = data[t2].re;
                v = (v * cs).conj();
                let u = data[t1];

                data[t1] = u + v;
                data[t2] = u - v;

                // Same as above but for t1,t2 with j>mh/4 and thus cs swapped
                t1 = r + mq - j;
                t2 = t1 + mh;

                let v = data[t2] * cs;
                let u = data[t1];

                data[t1].re = u.re + v.im;
                data[t1].im = u.im - v.re;

                data[t2].re = u.re - v.im;
                data[t2].im = u.im + v.re;

                t1 += mq;
                t2 = t1 + mh;

                let v = data[t2] * cs;
                let u = data[t1];

                data[t1] = u - v;
                data[t2] = u + v;
            }
        }

        {
            // block 2

            let j = mh >> 2;
            for inc in 0..n / m {
                let r = inc * m;
                let mut t1 = r + j;
                let mut t2 = t1 + mh;

                let mut v = data[t2] * v_tmp0;
                let u = data[t1];

                data[t1] = u + v;
                data[t2] = u - v;

                t1 += mq;
                t2 = t1 + mh;

                v.re = data[t2].im;
                v.im = data[t2].re;
                v = (v * v_tmp1).conj();
                let u = data[t1];

                data[t1] = u + v;
                data[t2] = u - v;
            }
        } // end of block 2
    }
}
