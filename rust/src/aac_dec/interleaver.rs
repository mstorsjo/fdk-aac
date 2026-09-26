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
//! Interleave data.

use crate::aac_dec::error_codes::AacDecoderError;
use itertools::izip;
use std::ops::Mul;

/// Interleaves time data and adjusts its scaling.
///
/// # Parameter
///
/// - `interleaved_samples_out`: The output buffer, containing the interleaved samples, if the
///   function returns `Ok(())`.
/// - `deinterleaved_samples_in`: The input buffer containing the deinterleaved samples.
/// - `num_channels`: The number of audio channels. This must be greater than `0`.
/// - `gain`: The gain factor to apply to the samples.
///
/// # Return
///
/// - Returns `Ok(())` on success or an `AacDecoderError::DecodeFrameError`, if lengths of the
///   input/output buffers do not match or `num_channels == 0`.
pub fn interleave_time_data<T>(
    interleaved_samples_out: &mut [T],
    deinterleaved_samples_in: &[T],
    num_channels: usize,
    gain: T,
) -> Result<(), AacDecoderError>
where
    T: Mul<Output = T> + std::fmt::Debug + Copy,
{
    if interleaved_samples_out.len() != deinterleaved_samples_in.len() || num_channels == 0 {
        return Err(AacDecoderError::DecodeFrameError);
    }

    let num_samples_per_channel = deinterleaved_samples_in.len() / num_channels;

    for (ch, inp_chunk) in deinterleaved_samples_in
        .chunks_exact(num_samples_per_channel)
        .enumerate()
    {
        for (out, inp) in izip!(
            interleaved_samples_out[ch..]
                .iter_mut()
                .step_by(num_channels),
            inp_chunk.iter()
        ) {
            *out = *inp * gain;
        }
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    // Test whether the interleave_time_data() works as expected. The test condition can only work
    // for integer data types, for the time being.
    fn test_interleave_time_data() {
        const NUM_CH: usize = 2;
        const NUM_SAMPLES_PER_CHANNEL: usize = 4;
        const GAIN: u32 = 2;

        let mut interleaved_samples_out = [0_u32; NUM_CH * NUM_SAMPLES_PER_CHANNEL];
        let mut deinterleaved_samples_in = [0_u32; NUM_CH * NUM_SAMPLES_PER_CHANNEL];

        for c in 0..NUM_CH {
            for s in 0..NUM_SAMPLES_PER_CHANNEL {
                deinterleaved_samples_in[c * NUM_SAMPLES_PER_CHANNEL + s] = (c + 1) as u32;
            }
        }

        let _ = interleave_time_data(
            &mut interleaved_samples_out,
            &deinterleaved_samples_in,
            NUM_CH,
            GAIN,
        );

        for s in 0..NUM_SAMPLES_PER_CHANNEL {
            for c in 0..NUM_CH {
                let inp = deinterleaved_samples_in[s + c * NUM_SAMPLES_PER_CHANNEL];
                let out = interleaved_samples_out[c + s * NUM_CH];

                assert_eq!(inp * GAIN, out);
            }
        }
    }
}
