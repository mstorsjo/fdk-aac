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
//! SAC Decoder frequency mapping.
use super::constants::MAX_PARAMETER_BANDS;
use itertools::izip;
use std::cmp::Ordering;

/// Indicates how parameter bands are grouped for entropy coding of XXX data.
/// Grouping is performed according to ISO/IEC 23003-1:2007, Table 70.
pub(super) static PB_STRIDE_TABLE: [u8; 4] = [1, 2, 5, 28];

#[repr(C)]
#[derive(Debug, Copy, Clone)]
/// Indicates how parameter bands are grouped for entropy coding of XXX data.
/// Grouping is performed according to ISO/IEC 23003-1:2007, Table 70.
pub(super) enum PbStride {
    S1 = 1,
    S2 = 2,
    S5 = 5,
    S28 = 28,
}

impl From<usize> for PbStride {
    fn from(value: usize) -> Self {
        match value {
            1 => PbStride::S1,
            2 => PbStride::S2,
            5 => PbStride::S5,
            28 => PbStride::S28,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<PbStride> for usize {
    fn from(value: PbStride) -> usize {
        match value {
            PbStride::S1 => 1,
            PbStride::S2 => 2,
            PbStride::S5 => 5,
            PbStride::S28 => 28,
        }
    }
}

impl From<i16> for PbStride {
    fn from(value: i16) -> Self {
        match value {
            1 => PbStride::S1,
            2 => PbStride::S2,
            5 => PbStride::S5,
            28 => PbStride::S28,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<PbStride> for i16 {
    fn from(value: PbStride) -> i16 {
        match value {
            PbStride::S1 => 1,
            PbStride::S2 => 2,
            PbStride::S5 => 5,
            PbStride::S28 => 28,
        }
    }
}

impl From<u8> for PbStride {
    fn from(value: u8) -> Self {
        match value {
            1 => PbStride::S1,
            2 => PbStride::S2,
            5 => PbStride::S5,
            28 => PbStride::S28,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<PbStride> for u8 {
    fn from(value: PbStride) -> u8 {
        match value {
            PbStride::S1 => 1,
            PbStride::S2 => 2,
            PbStride::S5 => 5,
            PbStride::S28 => 28,
        }
    }
}

/// Handles the stride for parameter band grouping. Otherwise said,
/// it initializes `a_map[]` with start and stop borders for the frequency grouping.
///
/// # Parameters
///
/// - `a_map`: In/Out array with groups of parameter bands.
/// - `stop_band`: Last frequency band for which grouping is required. `start_band` is implicit 0.
/// - `stride`: Indicates how parameter bands are grouped for entropy coding of XXX data. Grouping
///   is performed according to ISO/IEC 23003-1:2007, Table 70.
pub(super) fn create_mapping(stop_band: u8, stride: PbStride) -> [usize; MAX_PARAMETER_BANDS + 1] {
    let mut vdk = [0_usize; MAX_PARAMETER_BANDS + 1];

    let stride_i16 = i16::from(stride);
    let in_bands = i16::from(stop_band);

    // Number of output bands is always positive and minimum 1.
    let tmp = ((in_bands - 1) / stride_i16) + 1;
    let out_bands = if tmp > 0 { tmp as usize } else { 1 };

    let bands_achieved = out_bands as i16 * stride_i16;
    let mut bands_diff = in_bands - bands_achieved;

    vdk.iter_mut()
        .take(out_bands)
        .for_each(|v| *v = usize::from(stride));

    let mut k = if bands_diff > 0 { out_bands - 1 } else { 0 };

    loop {
        match bands_diff.cmp(&0) {
            Ordering::Greater => {
                vdk[k] += 1;
                k -= 1;
                bands_diff -= 1;
                if k >= out_bands {
                    k = out_bands - 1;
                }
            }
            Ordering::Less => {
                vdk[k] -= 1;
                k += 1;
                bands_diff += 1;
                if k >= out_bands {
                    k = 0;
                }
            }
            Ordering::Equal => {
                break;
            }
        }
    }

    // y[i+1] = x[i] + y[i];
    let x_times = (out_bands).min(MAX_PARAMETER_BANDS);
    let mut a_map = [0_usize; MAX_PARAMETER_BANDS + 1];
    let mut prev_out = a_map[0];

    for (out, prev_inp) in izip!(a_map.iter_mut().skip(1), vdk.iter()).take(x_times) {
        *out = *prev_inp + prev_out;
        prev_out = *out;
    }

    a_map
}

/// Maps the `input` spatial parameters data to the `output`, according to the frequency borders
/// provided via `map`.
///
/// # Parameters
///
/// - `input`: Spatial parameters.
/// - `output`: Mapped spatial parameters based on frequency borders.
/// - `a_map`: Mapping table, containing the frequency borders.
/// - `num_data_bands`: Number of data bands to process.
pub fn map_frequency(input: &[i8], output: &mut [i8], a_map: &mut [usize], num_data_bands: usize) {
    for (inp, boundaries) in izip!(input.iter(), a_map.windows(2)).take(num_data_bands) {
        output[boundaries[0]..boundaries[1]].fill(*inp);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // It should not panic.
    #[test]
    fn test_create_mapping_edge_cases() {
        const N: usize = u8::MAX as usize;

        let stride_params = [PbStride::S1, PbStride::S2, PbStride::S5, PbStride::S28];
        let mut stop_band_params = [0_u8; N];

        for (i, stop_band) in izip!(stop_band_params.iter_mut()).enumerate() {
            *stop_band = i as u8;
        }

        for stride in stride_params.iter() {
            for stop_band in stop_band_params.iter() {
                let _a_map = create_mapping(*stop_band, *stride);
                // println!(
                //     "stride = {}, stop_band = {}, a_map = {:?}",
                //     *stride, *stop_band, a_map
                // );
            }
        }
    }
}
