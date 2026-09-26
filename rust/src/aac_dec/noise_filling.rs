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
//! Noise filling

use super::utils::generate_random_sign;
use crate::common::pow::scale_factor_2_exp;
use itertools::izip;

use crate::aac_dec::channel_info::IcsInfo;

use super::constants;

// NOISE_LEVEL_TAB table implements pow(2, (float)(noise_level-14)/3.0f);
// NOISE_LEVEL_TAB[0] == 0 by definition.

#[rustfmt::skip]
static NOISE_LEVEL_TAB: [f32; 8] = [
    0.00000000000000000000, 0.04960628287400624131, 0.0625000000000000000,
    0.07874506561842957842, 0.09921256574801248262, 0.12500000000000000000,
    0.1574901312368591568,  0.19842513149602496520];

/// Apply noise on spectral coefficients
/// (ISO/IEC 23003-3 Second edition 2020-06 7.2.3 Decoding process)
///
/// # Parameters
///
/// - `ics_info`: Individual channel stream info with valid internal data
/// - `fd_noise_level_and_offset`: Index to get noise level value from noise level table
/// - `scale_factor`: Scale factor values for each group
/// - `spectrum`: Spectral coefficients
/// - `random_seed`: Internal state of random sign generator
/// - `band_is_noise`: Flags indicating noise presence in band
///
/// # Examples
///
/// ```
/// use aac::aac_dec::channel_info::IcsInfo;
/// use aac::aac_dec::constants::{MAX_SFB_SHORT, MAX_WINDOWS};
/// use aac::aac_dec::noise_filling;
///
/// // Precondition, should have a valid IcsInfo instance
/// // Refer respective components for instance creation methods
/// let ics_info = IcsInfo::new();
///
/// // Precondition, should have a valid data with respect to ics_info
/// let frame_length = 768_usize;
/// let fd_noise_level_and_offset = 0xC0;
/// let mut scale_factor = vec![0_i16; MAX_SFB_SHORT * MAX_WINDOWS];
/// let mut spectrum = vec![0.0f32; frame_length]; // frame length is 768
/// let mut band_is_noise = vec![false; MAX_SFB_SHORT * MAX_WINDOWS];
/// let mut seed = 0x12345;
///
/// noise_filling::noise_filling(
///     &ics_info,
///     fd_noise_level_and_offset,
///     &mut scale_factor,
///     &mut spectrum,
///     &mut seed,
///     &band_is_noise,
/// );
/// ```
pub fn noise_filling(
    ics_info: &IcsInfo,
    fd_noise_level_and_offset: u8,
    scale_factor: &mut [i16],
    spectrum: &mut [f32],
    random_seed: &mut u32,
    band_is_noise: &[bool],
) {
    let swb_offsets = ics_info.scale_factor_bands(); // swb: scalefactor window band

    // Obtain noise level and scale factor offset
    let noise_level = fd_noise_level_and_offset >> 5;
    let noise_val_pos = NOISE_LEVEL_TAB[usize::from(noise_level)];

    // noise_offset can change even when noise_level=0. Neccesary for IGF stereo filling
    let noise_offset = (i16::from(fd_noise_level_and_offset) & 0x1f) - 16;

    let max_sfb = ics_info.max_sf_bands();

    let mut noise_filling_start_offset = if !ics_info.is_long_block() { 20 } else { 160 };

    if spectrum.len() == 768 {
        noise_filling_start_offset = (3 * noise_filling_start_offset) / 4;
    }

    let sfb = swb_offsets
        .iter()
        .take_while(|&swf| *swf < noise_filling_start_offset)
        .count()
        .min(max_sfb);

    let window_length = spectrum.len() / ics_info.windows_per_frame();
    let mut grp_offset = 0;
    for (group, window_group_length) in izip!(ics_info
        .window_group_lengths()
        .iter()
        .take(ics_info.n_window_groups())
        .enumerate())
    {
        let start = group * constants::MAX_SFB_SHORT + sfb;
        let stop = group * constants::MAX_SFB_SHORT + max_sfb;
        for (scale_factor_new, scale_offset_band, band_is_noise_flag) in izip!(
            scale_factor[start..stop].iter_mut(),
            swb_offsets[sfb..=max_sfb].windows(2),
            band_is_noise[start..stop].iter(),
        ) {
            let scale_factor_curr = scale_offset_band[0];
            let scale_factor_next = scale_offset_band[1];
            // if all bins of one sfb in one window group are zero modify the scale
            // factor by noise_offset
            if *band_is_noise_flag {
                // Change scaling factors for empty signal bands
                *scale_factor_new += noise_offset;
            }

            let mut seed = *random_seed;
            // noiseVal_pos * pow(2.0f, 0.25f*scale1)
            let scaled_noise_val_pos = noise_val_pos * scale_factor_2_exp(*scale_factor_new);

            for spectral_values in spectrum[grp_offset * window_length..]
                .chunks_exact_mut(window_length)
                .take(usize::from(*window_group_length))
            {
                if *band_is_noise_flag {
                    for spectral_value in &mut spectral_values
                        [usize::from(scale_factor_curr)..usize::from(scale_factor_next)]
                        .iter_mut()
                    {
                        *spectral_value = generate_random_sign(&mut seed) * scaled_noise_val_pos;
                    }
                } else {
                    for spectral_value in &mut spectral_values
                        [usize::from(scale_factor_curr)..usize::from(scale_factor_next)]
                        .iter_mut()
                    {
                        if *spectral_value == 0.0 {
                            *spectral_value =
                                generate_random_sign(&mut seed) * scaled_noise_val_pos;
                        }
                    }
                }
            }

            {
                *random_seed = seed;
            }
        }
        grp_offset += usize::from(*window_group_length);
    }
}

#[cfg(test)]
mod tests {
    use super::noise_filling;
    use crate::{
        aac_dec::{
            channel_info::{BlockType, IcsInfo},
            constants,
            sr_info::SamplingRateInfo,
        },
        common::{
            bitstream::{Bitstream, Mode},
            flags::ACFlags,
        },
    };

    #[test]
    fn noise_filling_long_block() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);
        let frame_length = 768_usize;
        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 11);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(frame_length as u16, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // Init test data
        let fd_noise_level_and_offset = 0xC0;
        let mut scale_factor = vec![0_i16; constants::MAX_SFB_SHORT * constants::MAX_WINDOWS];
        let mut spectrum = vec![0.0f32; frame_length];
        let mut band_is_noise = vec![false; constants::MAX_SFB_SHORT * constants::MAX_WINDOWS];
        let mut seed = 0x12345;

        // calculation to fill test data
        let mut noise_filling_start_offset = if ics_info.window_sequence() == BlockType::Short {
            20
        } else {
            160
        };

        if spectrum.len() == 768 {
            noise_filling_start_offset = if ics_info.window_sequence() == BlockType::Short {
                15
            } else {
                120
            };
        }
        let swb_offset = ics_info.scale_factor_bands();
        let sfb = swb_offset
            .iter()
            .take_while(|&swf| *swf < noise_filling_start_offset)
            .count();

        // signal second band range as noise
        let band2_start = swb_offset[sfb + 1] as usize;
        let band2_stop = swb_offset[sfb + 2] as usize;
        spectrum[band2_start..band2_stop].fill(1.0_f32);

        band_is_noise[sfb + 1] = true;

        // signal third  band range as noiseless
        band_is_noise[sfb + 2] = false;

        // fill spectrum with 1.0 values for this range
        let band3_start = swb_offset[sfb + 2] as usize;
        let band3_stop = swb_offset[sfb + 3] as usize;
        spectrum[band3_start..band3_stop].fill(1.0_f32);

        // fill scale_factor with dummy values
        scale_factor[sfb + 1] = -16_i16;
        scale_factor[sfb + 2] = -12_i16;

        noise_filling(
            &ics_info,
            fd_noise_level_and_offset,
            &mut scale_factor,
            &mut spectrum,
            &mut seed,
            &band_is_noise,
        );

        assert_ne!(&spectrum[0..band2_start], &vec![0.0_f32; band2_start]);

        // case 1: Noisy band, expected non-zero value in spectrum for selected band range
        assert_ne!(
            &spectrum[band2_start..band2_stop],
            &vec![1.0_f32; band2_stop - band2_start]
        );

        // case 2: Noiseless band, expected no change in spectrum for selected band range,
        // since spectrum is value non-zero
        assert_eq!(
            &spectrum[band3_start..band3_stop],
            &vec![1.0_f32; band3_stop - band3_start]
        );

        // frame length is 768
        assert_ne!(
            &spectrum[band3_stop..frame_length],
            &vec![0.0_f32; frame_length - band3_stop]
        );

        assert_ne!(scale_factor[sfb + 1], -16_i16);
    }
}
