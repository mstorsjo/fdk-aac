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
//! Inverse quantization

use crate::aac_dec::{channel_info::IcsInfo, constants, error_codes::AacDecoderError, utils};
use crate::common::quantize::{inverse_quantize_band, MAX_QUANTIZED_VALUE};
use itertools::{izip, Itertools};

/// Inverse quantize spectral data. Processing takes place on scalefactor bands.
/// Each value of the sfb is processed accordingly to the formula:
/// - `spectrum[i] = Sign(quant_spectrum[i]) * abs(quant_spectrum[i])^(4/3) * 2^(scf/4).`
///
/// # Parameters
///
/// - `ics_info`: individual channel stream info with valid internal data.
/// - `frame_length`: aac frame length.
/// - `code_book`: codebook buffer (slice) for each window.
/// - `scale_factors`: spectral scale factors (slice) for each sfb in each  window.
/// - `quant_spectrum`:  input buffer (slice) to the first line of the sfb to be inverse quantized.
/// - `spectrum`:   output buffer (slice), which holds the inverse quantized output.
/// - `band_is_noise`:  output buffer (slice) which signals whether a particular sfb is noise.
///
/// # Return
/// - `AacDecoderError`
///
/// # Examples
/// ```
/// use aac::aac_dec::{
///     channel_info::IcsInfo, constants::MAX_WINS_X_SFBS,
///     inverse_quantization::inverse_quantize_spectral_data, utils,
/// };
/// use aac::common::quantize::MAX_QUANTIZED_VALUE;
///
/// // input data
/// let mut quant_spectrum: Vec<i16> = Vec::new();
/// let qboundry: i16 = MAX_QUANTIZED_VALUE as i16;
/// for i in -qboundry..=qboundry {
///     quant_spectrum.push(i);
/// }
///
/// // output data
/// let mut inv_quant_spectrum: Vec<f32> = vec![0.0; quant_spectrum.len()];
///
/// let ics: IcsInfo = Default::default();
/// let cbook = [utils::CBTYPE_BOOKSCL; MAX_WINS_X_SFBS];
/// let scalefactors = [21_i16; MAX_WINS_X_SFBS];
/// let mut band_is_noise = [false; MAX_WINS_X_SFBS];
///
/// inverse_quantize_spectral_data(
///     &ics,
///     &cbook,
///     &scalefactors,
///     &quant_spectrum,
///     &mut inv_quant_spectrum,
///     &mut band_is_noise,
/// );
/// ```
pub fn inverse_quantize_spectral_data(
    ics_info: &IcsInfo,
    code_book: &[u8],
    scale_factors: &[i16],
    quant_spectrum: &[i16],
    spectrum: &mut [f32],
    band_is_noise: &mut [bool],
) -> AacDecoderError {
    let scf_band_offsets = ics_info.scale_factor_bands();
    let window_len = quant_spectrum.len() / ics_info.windows_per_frame();
    let bands_per_window = constants::MAX_WINS_X_SFBS / ics_info.windows_per_frame();

    let mut quant_spec_window = quant_spectrum.chunks_exact(window_len);
    let mut spec_window = spectrum.chunks_exact_mut(window_len);

    for ((group, scf), cbook, is_noise) in izip!(
        scale_factors.chunks_exact(bands_per_window).enumerate(),
        code_book.chunks_exact(bands_per_window),
        band_is_noise.chunks_exact_mut(bands_per_window),
    )
    .take(ics_info.n_window_groups())
    {
        // Iterate over all windows beloging to a group.
        // The same scale_factors must be applied over all windows.
        let win_per_group = usize::from(ics_info.window_group_length(group));
        for _win in 0..win_per_group {
            let quant_spec = quant_spec_window.next().unwrap();
            let spec = spec_window.next().unwrap();
            let mut start_idx;
            let mut stop_idx = 0;

            for (scf_band, cbook_band, is_noise_band, (band_offset_curr, band_offset_next)) in
                izip!(
                    scf.iter(),
                    cbook.iter(),
                    is_noise.iter_mut(),
                    scf_band_offsets.iter().tuple_windows()
                )
                .take(ics_info.max_sf_bands())
            {
                start_idx = usize::from(*band_offset_curr);
                stop_idx = usize::from(*band_offset_next);

                if !([
                    utils::CBTYPE_ZERO_HCB,
                    utils::CBTYPE_INTENSITY_HCB,
                    utils::CBTYPE_INTENSITY_HCB2,
                    utils::CBTYPE_NOISE_HCB,
                ]
                .contains(cbook_band))
                {
                    let loc_max = quant_spec[start_idx..stop_idx]
                        .iter()
                        .max_by_key(|x| x.unsigned_abs())
                        .unwrap()
                        .unsigned_abs();

                    if loc_max > MAX_QUANTIZED_VALUE {
                        return AacDecoderError::ParseError;
                    }

                    if loc_max != 0 {
                        inverse_quantize_band(
                            &mut spec[start_idx..stop_idx],
                            &quant_spec[start_idx..stop_idx],
                            loc_max,
                            *scf_band,
                        );
                        *is_noise_band = false;
                        continue;
                    }
                }
                spec[start_idx..stop_idx].fill(0.0);
            }
            // Make sure that the array is cleared at the end.
            spec[stop_idx..].fill(0.0);
        }
    }
    AacDecoderError::Ok
}
