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
//! Bass postfilter

use itertools::izip;

use crate::aac_dec::lpd::constants::*;
use crate::common::arith_ops::calc_energy;
use crate::common::arith_ops::sum_mult;
use crate::common::arith_ops::sum_sqr;

static FILT_LP: [f32; 1 + L_FILT] = [
    0.088250, 0.086410, 0.081074, 0.072768, 0.062294, 0.050623, 0.038774, 0.027692, 0.018130,
    0.010578, 0.005221, 0.001946, 0.000385,
];

/// Performs a low-frequency pitch enhancement on a time domain signal.
///
/// # Parameters
///
/// - `syn_in`: Time domain input signal.
/// - `syn_out`: Time domain output signal.
/// - `t_sf`: Array with past decoded pitch period values for each subframe.
/// - `pit_gain`: Pitch gain values of each subframe.
/// - `l_frame`: Length of filtering, must be multiple of L_SUBFR.
/// - `l_next`: Length of allowed look ahead on `syn_in[..l_frame+l_next]`.
/// - `mem_bpf`: Filter memory.
pub fn filter_1sf_delay(
    syn_in: &[f32],
    syn_out: &mut [f32],
    t_sf: &[u16],
    pit_gain: &[f32],
    l_frame: usize,
    l_next: usize,
    mem_bpf: &mut [f32],
) {
    let frame_length = syn_out.len();

    let mut noise_buf = [0.0f32; L_FILT + (2 * L_SUBFR)];
    for (sf, i_subfr) in (0..l_frame).step_by(L_SUBFR).enumerate() {
        let mut t = t_sf[sf] as usize;
        let mut gain = pit_gain[sf].clamp(0.0, 1.0);

        let slice_offset = PIT_MAX_MAX + i_subfr;

        if gain > 0.0 {
            const L_EXTRA: usize = 96;

            // Pitch tracker: Test pitch/2 to avoid continuous pitch doubling.
            // Note: Pitch is limited to PIT_MIN (34 = 376Hz) in the encoder.
            let t2 = t >> 1;

            // Calculate cumulative sums.
            let ener = 0.01 + calc_energy(&syn_in[slice_offset - L_EXTRA..slice_offset + L_SUBFR]);

            let corr = 0.01
                + sum_mult(
                    &syn_in[slice_offset - L_EXTRA..slice_offset + L_SUBFR],
                    &syn_in[slice_offset - t2 - L_EXTRA..slice_offset - t2 + L_SUBFR],
                );

            let tmp =
                0.01 + sum_sqr(&syn_in[slice_offset - t2 - L_EXTRA..slice_offset - t2 + L_SUBFR]);

            // Use t2 if normalized correlation > 0.95.
            if (corr * ener.sqrt().recip()) > (0.95 * tmp.sqrt()) {
                t = t2;
            }

            // Make sure that noise calculation below stays in defined signal
            // parts at the end of the synth_buf, i.e. restrict the below
            // used index (i+i_subfr+T) < l_frame + l_next.
            let lg = ((l_frame + l_next) as isize - (t + i_subfr) as isize)
                .clamp(0, L_SUBFR as isize) as usize;

            // Limit gain to avoid problem on burst.
            if lg > 0 {
                let ener = 0.01 + calc_energy(&syn_in[slice_offset + t..slice_offset + t + lg]);

                let tmp = 0.01 + sum_sqr(&syn_in[slice_offset..slice_offset + lg]);

                gain = gain.min((tmp / ener).sqrt());
            }

            // Calculate noise based on voiced pitch.
            {
                let noise_in = &mut noise_buf[L_FILT + L_SUBFR..];

                gain *= 0.5;

                for i in 0..lg {
                    noise_in[i] = gain
                        * (syn_in[slice_offset + i]
                            - 0.5 * syn_in[slice_offset + i - t]
                            - 0.5 * syn_in[slice_offset + i + t]);
                }

                for i in lg..L_SUBFR {
                    noise_in[i] = gain * (syn_in[slice_offset + i] - syn_in[slice_offset + i - t]);
                }
            }
        } else {
            let noise_in = &mut noise_buf[L_FILT + L_SUBFR..];
            noise_in.fill(0.0);
        }

        noise_buf[..L_FILT + L_SUBFR].copy_from_slice(&mem_bpf[..L_FILT + L_SUBFR]);
        mem_bpf[..L_FILT + L_SUBFR].copy_from_slice(&noise_buf[L_SUBFR..L_FILT + 2 * L_SUBFR]);

        // Subtract low-pass filtered noise from voiced speech.
        subtract_lp_noise(
            &syn_in[slice_offset - L_SUBFR..slice_offset],
            &mut syn_out[i_subfr..i_subfr + L_SUBFR],
            &noise_buf[..L_SUBFR + 2 * L_FILT],
        );
    }

    syn_out[l_frame..].copy_from_slice(
        &syn_in[PIT_MAX_MAX + l_frame - L_SUBFR..PIT_MAX_MAX + frame_length - L_SUBFR],
    );
}

/// Subtracts low-pass filtered noise from input signal.
///
/// # Parameters
///
/// - `syn_in`: Input signal.
/// - `syn_out`: Output signal.
/// - `syn_noise`: Noise to be subtracted.
fn subtract_lp_noise(syn_in: &[f32], syn_out: &mut [f32], noise: &[f32]) {
    let filt_len = L_FILT;

    debug_assert!(syn_in.len() == syn_out.len());
    debug_assert!(syn_in.len() + 2 * filt_len == noise.len());

    for (dst, src, noise_chunk) in izip!(syn_out, syn_in, noise.windows(2 * filt_len + 1)) {
        let mut tmp = noise_chunk[filt_len] * FILT_LP[0];
        for (i, f) in FILT_LP.iter().enumerate().skip(1) {
            tmp += (noise_chunk[filt_len - i] + noise_chunk[filt_len + i]) * f;
        }
        *dst = *src - tmp;
    }
}

#[cfg(test)]
mod tests {
    use crate::aac_dec::lpd::bass_postfilter;
    use crate::aac_dec::lpd::constants::*;

    #[test]
    fn subtract_lp_noise() {
        #[rustfmt::skip]
        let syn: [f32; L_SUBFR] = [-5.14266777, -7.11786365, -8.27681446, -8.7266817, -3.19677734, -1.03949273, 0.502002001, 0.829239607, 0.205120921, -0.390450925, -0.920013964, -0.782229065, -0.279657423, 0.41472888, 0.865146279, 1.04557538, 0.8908149, 0.579751194, 0.169757307, -0.087559022, -0.239443138, -0.175314456, -0.0249502659, 0.0899149402, 0.151974484, 0.106476389, 0.0218340233, -0.0801076442, -0.138267234, -0.143681481, -0.098644711, -0.0357645676, 0.0182616524, 0.0444040187, 0.0412825868, 0.0193603989, -0.00675555691, -0.0227916203, -0.0249009356, -0.0129445195, 0.00294341706, 0.0163939167, 0.0212215893, -4.3536849, -6.04159689, -7.03632927, -7.42485237, -2.72568274, -0.889005601, 0.425860107, 0.707886338, 0.178839773, -0.328353226, -0.780977785, -0.66640228, -0.24073194, 0.349469304, 0.733472764, 0.888417006, 0.758151888, 0.494259238, 0.145512402, -0.073934868, -0.203790277];
        #[rustfmt::skip]
        let noise: [f32; L_SUBFR+2*L_FILT] =[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.26018798, -1.7408551, -2.02196455, -2.13057208, -0.77924484, -0.252812982, 0.12282753, 0.201954871, 0.0491617136, -0.0961446315, -0.22499676, -0.190793306, -0.0676834062, 0.101974264, 0.211775854, 0.255522579, 0.217439398, 0.141331896, 0.0412160568, -0.0214965157, -0.0584445558, -1.40440011, -0.528964937, 1.0768528, 1.12183106, 0.696810781, 0.325978577, -0.642178475, -1.10806072, -1.31893289, -1.30701661, -0.391867757, 0.0117221437, 0.241293684, 0.209640682, 0.0398227647, -0.111083545, -0.215522841, -0.209527135, -0.123706423, 0.0326375887, 0.139845654, 0.183056951, -0.611983299, -0.969228565, -1.21904469, -1.3342973, -0.532117665, -0.192981824, 0.0836759507, 0.168828443, 0.0895789489, -0.00508045312, -0.11579033, -0.125872791, -0.0667414665, 0.0324471816, 0.110489711, 0.151243836, 0.140662149, 0.098822087, 0.0351695046, -0.010217377, -0.0400076099, 0.273971558, 0.0960410684, -0.173811719, -0.212675944, -0.127761677, -0.0592564866, 0.134181425, 0.161667913, 0.246302977, 0.264500856, 0.0787399411, 0.00953181554];
        let mut syn_out: [f32; L_SUBFR] = [0.0f32; L_SUBFR];

        bass_postfilter::subtract_lp_noise(&syn, &mut syn_out, &noise);

        #[rustfmt::skip]
        let syn_out_ref: [f32; L_SUBFR] =[-4.50937939, -6.44563389, -7.59105253, -8.05418682, -2.56333113, -0.467553258, 0.995234132, 1.23315096, 0.516234457, -0.168232188, -0.775715530, -0.700012922, -0.242041484, 0.423949242, 0.858386457, 1.03045976, 0.871041059, 0.556905925, 0.145810500, -0.108625256, -0.251493841, -0.170714989, 0.00426424667, 0.150832474, 0.249816477, 0.243943870, 0.198774725, 0.133211121, 0.105481490, 0.121956512, 0.178258061, 0.240781665, 0.283589095, 0.290295899, 0.263927311, 0.220070019, 0.178645715, 0.158877507, 0.167383432, 0.203895152, 0.254993886, 0.308879167, 0.353038669, -3.98955321, -5.65676641, -6.64503145, -7.04203892, -2.36555004, -0.563838422, 0.706556439, 0.938023686, 0.356103599, -0.202608839, -0.702284873, -0.627856851, -0.233854607, 0.333671600, 0.703284740, 0.850563347, 0.717502654, 0.453934431, 0.107352570, -0.108979449, -0.235487878];

        debug_assert!(syn_out == syn_out_ref);
    }
}
