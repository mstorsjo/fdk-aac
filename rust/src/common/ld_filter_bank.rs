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
//! AAC Low-Delay (AAC-LD) synthesis filterbank
//!
//! Do inverse low delay filtering for ELD.
//
//! Supported lengths are:
//! * 120, 128, 160, 240, 256, 320, 480, 512

use itertools::izip;

use crate::common::dct;
use crate::common::tables::ld_filter_bank_tables;

fn mult_e2_dinv_f(
    data: &[f32],
    out: &mut [f32],
    coefs: &[f32],
    overlap_buffer: &mut [f32],
    gain: f32,
) {
    let n = data.len();
    let chunk_size = n / 4;

    let (data_left, data_right) = data.split_at(n / 2);
    let (ds0, ds1) = data_left.split_at(chunk_size);
    let (ds2, ds3) = data_right.split_at(chunk_size);

    let mut out_chunks = out.chunks_exact_mut(chunk_size).take(4);
    let out_s0 = out_chunks.next().unwrap();
    let out_s1 = out_chunks.next().unwrap();
    let out_s2 = out_chunks.next().unwrap();
    let out_s3 = out_chunks.next().unwrap();

    let mut overlap_chunks = overlap_buffer.chunks_exact_mut(chunk_size).take(6);
    let ob_s0 = overlap_chunks.next().unwrap();
    let ob_s1 = overlap_chunks.next().unwrap();
    let ob_s2 = overlap_chunks.next().unwrap();
    let ob_s3 = overlap_chunks.next().unwrap();
    let ob_s4 = overlap_chunks.next().unwrap();
    let ob_s5 = overlap_chunks.next().unwrap();

    let mut coefs_chunks = coefs.chunks_exact(chunk_size).take(12);
    let coefs_s0 = coefs_chunks.next().unwrap(); // 0
    let _coefs_s1 = coefs_chunks.next().unwrap(); // n/4
    let coefs_s2 = coefs_chunks.next().unwrap(); // n/2
    let coefs_s3 = coefs_chunks.next().unwrap(); // 3n/4
    let coefs_s4 = coefs_chunks.next().unwrap(); // n
    let coefs_s5 = coefs_chunks.next().unwrap(); // 5n/4
    let coefs_s6 = coefs_chunks.next().unwrap(); // 3n/2
    let coefs_s7 = coefs_chunks.next().unwrap(); // 7n/4
    let coefs_s8 = coefs_chunks.next().unwrap(); // 2n
    let coefs_s9 = coefs_chunks.next().unwrap(); // 9n/4
    let coefs_s10 = coefs_chunks.next().unwrap(); // 5n/2
    let coefs_s11 = coefs_chunks.next().unwrap(); // 11n/4

    for (d1, d2, ov0, ov2, ov4, out2, out3, c2, c8, c10, c5, c6) in izip!(
        ds1.iter().rev(),
        ds2.iter(),
        ob_s0.iter_mut(),
        ob_s2.iter_mut(),
        ob_s4.iter_mut(),
        out_s2.iter_mut().rev(),
        out_s3.iter_mut(),
        coefs_s2.iter(),
        coefs_s8.iter(),
        coefs_s10.iter(),
        coefs_s5.iter().rev(),
        coefs_s6.iter()
    )
    .take(chunk_size)
    {
        let z2 = *d2;
        let z0 = z2 + (*ov2 * *c8);
        *ov2 = *d1 + (*ov4 * *c10);
        *out2 = ((*ov2 * *c5) + (*ov0 * *c6)) * gain;
        *ov0 = z0;
        *ov4 = z2;
        *out3 = *ov0 * *c2 * gain;
    }

    for (d0, d3, ov3, ov5, ov1, out0, out1, c9, c11, c0, c3, c4, c7) in izip!(
        ds0.iter().rev(),
        ds3.iter(),
        ob_s3.iter_mut(),
        ob_s5.iter_mut(),
        ob_s1.iter_mut(),
        out_s0.iter_mut(),
        out_s1.iter_mut().rev(),
        coefs_s9.iter(),
        coefs_s11.iter(),
        coefs_s0.iter().rev(),
        coefs_s3.iter(),
        coefs_s4.iter().rev(),
        coefs_s7.iter()
    )
    .take(chunk_size)
    {
        let z2 = *d3;
        let z0 = z2 + (*ov3 * *c9);

        *ov3 = *d0 + (*ov5 * *c11);
        *out0 = ((*ov3 * *c0) + (*ov1 * *c3)) * gain;
        *out1 = ((*ov3 * *c4) + (*ov1 * *c7)) * gain;
        *ov1 = z0;
        *ov5 = z2;
    }
}

/// Low Delay synthesis filtering. For filtering, the Low Delay filterbank
/// uses the DCT IV from common::dct.
///
/// # Parameters
///
/// - `data`: input signal. Data will be invalid after call.
/// - `out`: output data in time domain
/// - `overlap_buffer`: overlap buffer. Length must be 3/2*data.len().
///
/// # Examples
/// ```
/// use aac::common::ld_filter_bank;
///
/// let mut data = [0.0_f32; 480];
/// let mut out = [0.0_f32; 480];
/// let mut overlap_buffer = [0.0_f32; 720];
///
/// ld_filter_bank::synthesize(&mut data, &mut out, &mut overlap_buffer);
/// ```
pub fn synthesize(data: &mut [f32], out: &mut [f32], overlap_buffer: &mut [f32]) {
    let n = data.len();

    // Select LD window slope
    let coefs: &[f32] = match n {
        120 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_120,
        128 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_128,
        160 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_160,
        240 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_240,
        256 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_256,
        480 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_480,
        512 => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_512,
        _ => &ld_filter_bank_tables::LOW_DELAY_SYNTHESIS_480,
    };

    // Determine downscale factor for current length.
    let ds_fac = 512 / n;

    let gain = ((n * ds_fac) as f32).recip();

    dct::dctiv(data);

    mult_e2_dinv_f(data, out, coefs, overlap_buffer, gain);
}
