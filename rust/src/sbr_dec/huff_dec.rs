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
//! Huffman Decoder
use crate::common::bitstream::Bitstream;

// SBR Huffman Table Overview:
// Envelope level, 1.5 dB:
//    1) HUFF_BOOK_ENV_LEVEL_10T[120][2]
//    2) HUFF_BOOK_ENV_LEVEL_10F[120][2]
//
// Envelope balance, 1.5 dB:
//    3) HUFF_BOOK_ENV_BALANCE_10T[48][2]
//    4) HUFF_BOOK_ENV_BALANCE_10F[48][2]
//
// Envelope level, 3.0 dB:
//    5) HUFF_BOOK_ENV_LEVEL_11T[62][2]
//    6) HUFF_BOOK_ENV_LEVEL_11F[62][2]
//
// Envelope balance, 3.0 dB:
//    7) HUFF_BOOK_ENV_BALANCE_11T[24][2]
//    8) HUFF_BOOK_ENV_BALANCE_11F[24][2]
//
// Noise level, 3.0 dB:
//    9) HUFF_BOOK_NOISE_LEVEL_11T[62][2]
//    -) (HUFF_BOOK_ENV_LEVEL_11F[62][2] is used for freq dir)
//
// Noise balance, 3.0 dB:
//   10) HUFF_BOOK_NOISE_BALANCE_11T[24][2]
//    -) (HUFF_BOOK_ENV_BALANCE_11F[24][2] is used for freq dir)
//
// (1.5 dB is never used for noise)

#[rustfmt::skip]
const HUFF_BOOK_ENV_LEVEL_10T: [[i8; 2]; 120] = [
    [1, 2], [-64, -65], [3, 4], [-63, -66], [5, 6], [-62, -67],
    [7, 8], [-61, -68], [9, 10], [-60, -69], [11, 12], [-59, -70],
    [13, 14], [-58, -71], [15, 16], [-57, -72], [17, 18], [-73, -56],
    [19, 21], [-74, 20], [-55, -75], [22, 26], [23, 24], [-54, -76],
    [-77, 25], [-53, -78], [27, 34], [28, 29], [-52, -79], [30, 31],
    [-80, -51], [32, 33], [-83, -82], [-81, -50], [35, 57], [36, 40],
    [37, 38], [-88, -84], [-48, 39], [-90, -85], [41, 46], [42, 43],
    [-49, -87], [44, 45], [-89, -86], [-124, -123], [47, 50], [48, 49],
    [-122, -121], [-120, -119], [51, 54], [52, 53], [-118, -117], [-116, -115],
    [55, 56], [-114, -113], [-112, -111], [58, 89], [59, 74], [60, 67],
    [61, 64], [62, 63], [-110, -109], [-108, -107], [65, 66], [-106, -105],
    [-104, -103], [68, 71], [69, 70], [-102, -101], [-100, -99], [72, 73],
    [-98, -97], [-96, -95], [75, 82], [76, 79], [77, 78], [-94, -93],
    [-92, -91], [80, 81], [-47, -46], [-45, -44], [83, 86], [84, 85],
    [-43, -42], [-41, -40], [87, 88], [-39, -38], [-37, -36], [90, 105],
    [91, 98], [92, 95], [93, 94], [-35, -34], [-33, -32], [96, 97],
    [-31, -30], [-29, -28], [99, 102], [100, 101], [-27, -26], [-25, -24],
    [103, 104], [-23, -22], [-21, -20], [106, 113], [107, 110], [108, 109],
    [-19, -18], [-17, -16], [111, 112], [-15, -14], [-13, -12], [114, 117],
    [115, 116], [-11, -10], [-9, -8], [118, 119], [-7, -6], [-5, -4],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_LEVEL_10F: [[i8; 2]; 120] = [
    [1, 2], [-64, -65], [3, 4], [-63, -66], [5, 6], [-67, -62],
    [7, 8], [-68, -61], [9, 10], [-69, -60], [11, 13], [-70, 12],
    [-59, -71], [14, 16], [-58, 15], [-72, -57], [17, 19], [-73, 18],
    [-56, -74], [20, 23], [21, 22], [-55, -75], [-54, -53], [24, 27],
    [25, 26], [-76, -52], [-77, -51], [28, 31], [29, 30], [-50, -78],
    [-79, -49], [32, 36], [33, 34], [-48, -47], [-80, 35], [-81, -82],
    [37, 47], [38, 41], [39, 40], [-83, -46], [-45, -84], [42, 44],
    [-85, 43], [-44, -43], [45, 46], [-88, -87], [-86, -90], [48, 66],
    [49, 56], [50, 53], [51, 52], [-92, -42], [-41, -39], [54, 55],
    [-105, -89], [-38, -37], [57, 60], [58, 59], [-94, -91], [-40, -36],
    [61, 63], [-20, 62], [-115, -110], [64, 65], [-108, -107], [-101, -97],
    [67, 89], [68, 75], [69, 72], [70, 71], [-95, -93], [-34, -27],
    [73, 74], [-22, -17], [-16, -124], [76, 82], [77, 79], [-123, 78],
    [-122, -121], [80, 81], [-120, -119], [-118, -117], [83, 86], [84, 85],
    [-116, -114], [-113, -112], [87, 88], [-111, -109], [-106, -104], [90, 105],
    [91, 98], [92, 95], [93, 94], [-103, -102], [-100, -99], [96, 97],
    [-98, -96], [-35, -33], [99, 102], [100, 101], [-32, -31], [-30, -29],
    [103, 104], [-28, -26], [-25, -24], [106, 113], [107, 110], [108, 109],
    [-23, -21], [-19, -18], [111, 112], [-15, -14], [-13, -12], [114, 117],
    [115, 116], [-11, -10], [-9, -8], [118, 119], [-7, -6], [-5, -4],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_BALANCE_10T: [[i8; 2]; 48] = [
    [-64, 1], [-63, 2], [-65, 3], [-62, 4], [-66, 5], [-61, 6],
    [-67, 7], [-60, 8], [-68, 9], [10, 11], [-69, -59], [12, 13],
    [-70, -58], [14, 28], [15, 21], [16, 18], [-57, 17], [-71, -56],
    [19, 20], [-88, -87], [-86, -85], [22, 25], [23, 24], [-84, -83],
    [-82, -81], [26, 27], [-80, -79], [-78, -77], [29, 36], [30, 33],
    [31, 32], [-76, -75], [-74, -73], [34, 35], [-72, -55], [-54, -53],
    [37, 41], [38, 39], [-52, -51], [-50, 40], [-49, -48], [42, 45],
    [43, 44], [-47, -46], [-45, -44], [46, 47], [-43, -42], [-41, -40],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_BALANCE_10F: [[i8; 2]; 48] = [
    [-64, 1], [-65, 2], [-63, 3], [-66, 4], [-62, 5], [-61, 6],
    [-67, 7], [-68, 8], [-60, 9], [10, 11], [-69, -59], [-70, 12],
    [-58, 13], [14, 17], [-71, 15], [-57, 16], [-56, -73], [18, 32],
    [19, 25], [20, 22], [-72, 21], [-88, -87], [23, 24], [-86, -85],
    [-84, -83], [26, 29], [27, 28], [-82, -81], [-80, -79], [30, 31],
    [-78, -77], [-76, -75], [33, 40], [34, 37], [35, 36], [-74, -55],
    [-54, -53], [38, 39], [-52, -51], [-50, -49], [41, 44], [42, 43],
    [-48, -47], [-46, -45], [45, 46], [-44, -43], [-42, 47], [-41, -40],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_LEVEL_11T: [[i8; 2]; 62] = [
    [-64, 1], [-65, 2], [-63, 3], [-66, 4], [-62, 5], [-67, 6],
    [-61, 7], [-68, 8], [-60, 9], [10, 11], [-69, -59], [12, 14],
    [-70, 13], [-71, -58], [15, 18], [16, 17], [-72, -57], [-73, -74],
    [19, 22], [-56, 20], [-55, 21], [-54, -77], [23, 31], [24, 25],
    [-75, -76], [26, 27], [-78, -53], [28, 29], [-52, -95], [-94, 30],
    [-93, -92], [32, 47], [33, 40], [34, 37], [35, 36], [-91, -90],
    [-89, -88], [38, 39], [-87, -86], [-85, -84], [41, 44], [42, 43],
    [-83, -82], [-81, -80], [45, 46], [-79, -51], [-50, -49], [48, 55],
    [49, 52], [50, 51], [-48, -47], [-46, -45], [53, 54], [-44, -43],
    [-42, -41], [56, 59], [57, 58], [-40, -39], [-38, -37], [60, 61],
    [-36, -35], [-34, -33],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_LEVEL_11F: [[i8; 2]; 62] = [
    [-64, 1], [-65, 2], [-63, 3], [-66, 4], [-62, 5], [-67, 6],
    [7, 8], [-61, -68], [9, 10], [-60, -69], [11, 12], [-59, -70],
    [13, 14], [-58, -71], [15, 16], [-57, -72], [17, 19], [-56, 18],
    [-55, -73], [20, 24], [21, 22], [-74, -54], [-53, 23], [-75, -76],
    [25, 30], [26, 27], [-52, -51], [28, 29], [-77, -79], [-50, -49],
    [31, 39], [32, 35], [33, 34], [-78, -46], [-82, -88], [36, 37],
    [-83, -48], [-47, 38], [-86, -85], [40, 47], [41, 44], [42, 43],
    [-80, -44], [-43, -42], [45, 46], [-39, -87], [-84, -40], [48, 55],
    [49, 52], [50, 51], [-95, -94], [-93, -92], [53, 54], [-91, -90],
    [-89, -81], [56, 59], [57, 58], [-45, -41], [-38, -37], [60, 61],
    [-36, -35], [-34, -33],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_BALANCE_11T: [[i8; 2]; 24] = [
    [-64, 1], [-63, 2], [-65, 3], [-66, 4], [-62, 5], [-61, 6],
    [-67, 7], [-68, 8], [-60, 9], [10, 16], [11, 13], [-69, 12],
    [-76, -75], [14, 15], [-74, -73], [-72, -71], [17, 20], [18, 19],
    [-70, -59], [-58, -57], [21, 22], [-56, -55], [-54, 23], [-53, -52],
];

#[rustfmt::skip]
const HUFF_BOOK_ENV_BALANCE_11F: [[i8; 2]; 24] = [
    [-64, 1], [-65, 2], [-63, 3], [-66, 4], [-62, 5], [-61, 6],
    [-67, 7], [-68, 8], [-60, 9], [10, 13], [-69, 11], [-59, 12],
    [-58, -76], [14, 17], [15, 16], [-75, -74], [-73, -72], [18, 21],
    [19, 20], [-71, -70], [-57, -56], [22, 23], [-55, -54], [-53, -52],
];

#[rustfmt::skip]
const HUFF_BOOK_NOISE_LEVEL_11T: [[i8; 2]; 62] = [
    [-64, 1], [-63, 2], [-65, 3], [-66, 4], [-62, 5], [-67, 6],
    [7, 8], [-61, -68], [9, 30], [10, 15], [-60, 11], [-69, 12],
    [13, 14], [-59, -53], [-95, -94], [16, 23], [17, 20], [18, 19],
    [-93, -92], [-91, -90], [21, 22], [-89, -88], [-87, -86], [24, 27],
    [25, 26], [-85, -84], [-83, -82], [28, 29], [-81, -80], [-79, -78],
    [31, 46], [32, 39], [33, 36], [34, 35], [-77, -76], [-75, -74],
    [37, 38], [-73, -72], [-71, -70], [40, 43], [41, 42], [-58, -57],
    [-56, -55], [44, 45], [-54, -52], [-51, -50], [47, 54], [48, 51],
    [49, 50], [-49, -48], [-47, -46], [52, 53], [-45, -44], [-43, -42],
    [55, 58], [56, 57], [-41, -40], [-39, -38], [59, 60], [-37, -36],
    [-35, 61], [-34, -33],
];

#[rustfmt::skip]
const HUFF_BOOK_NOISE_BALANCE_11T: [[i8; 2]; 24] = [
    [-64, 1], [-65, 2], [-63, 3], [4, 9], [-66, 5], [-62, 6],
    [7, 8], [-76, -75], [-74, -73], [10, 17], [11, 14], [12, 13],
    [-72, -71], [-70, -69], [15, 16], [-68, -67], [-61, -60], [18, 21],
    [19, 20], [-59, -58], [-57, -56], [22, 23], [-55, -54], [-53, -52],
];

// Parametric stereo
// IID & ICC Huffman codebooks
#[rustfmt::skip]
const A_BOOK_PS_IID_TIME_DECODE: [[i8; 2]; 28] = [
    [-64, 1], [-65, 2], [-63, 3], [-66, 4], [-62, 5], [-67, 6],
    [-61, 7], [-68, 8], [-60, 9], [-69, 10], [-59, 11], [-70, 12],
    [-58, 13], [-57, 14], [-71, 15], [16, 17], [-56, -72], [18, 21],
    [19, 20], [-55, -78], [-77, -76], [22, 25], [23, 24], [-75, -74],
    [-73, -54], [26, 27], [-53, -52], [-51, -50],
];

#[rustfmt::skip]
const A_BOOK_PS_IID_FREQ_DECODE: [[i8; 2]; 28] = [
    [-64, 1], [2, 3], [-63, -65], [4, 5], [-62, -66], [6, 7],
    [-61, -67], [8, 9], [-68, -60], [-59, 10], [-69, 11], [-58, 12],
    [-70, 13], [-71, 14], [-57, 15], [16, 17], [-56, -72], [18, 19],
    [-55, -54], [20, 21], [-73, -53], [22, 24], [-74, 23], [-75, -78],
    [25, 26], [-77, -76], [-52, 27], [-51, -50],
];

#[rustfmt::skip]
const A_BOOK_PS_ICC_TIME_DECODE: [[i8; 2]; 14] = [
    [-64, 1], [-63, 2], [-65, 3], [-62, 4], [-66, 5], [-61, 6],
    [-67, 7], [-60, 8], [-68, 9], [-59, 10], [-69, 11], [-58, 12],
    [-70, 13], [-71, -57],
];

#[rustfmt::skip]
const A_BOOK_PS_ICC_FREQ_DECODE: [[i8; 2]; 14] = [
    [-64, 1], [-63, 2], [-65, 3], [-62, 4], [-66, 5], [-61, 6],
    [-67, 7], [-60, 8], [-59, 9], [-68, 10], [-58, 11], [-69, 12],
    [-57, 13], [-70, -71],
];

// IID-fine Huffman codebooks
#[rustfmt::skip]
const A_BOOK_PS_IID_FINE_TIME_DECODE: [[i8; 2]; 60] = [
    [1, -64], [-63, 2], [3, -65], [4, 59], [5, 7], [6, -67],
    [-68, -60], [-61, 8], [9, 11], [-59, 10], [-70, -58], [12, 41],
    [13, 20], [14, -71], [-55, 15], [-53, 16], [17, -77], [18, 19],
    [-85, -84], [-46, -45], [-57, 21], [22, 40], [23, 29], [-51, 24],
    [25, 26], [-83, -82], [27, 28], [-90, -38], [-92, -91], [30, 37],
    [31, 34], [32, 33], [-35, -34], [-37, -36], [35, 36], [-94, -93],
    [-89, -39], [38, -79], [39, -81], [-88, -40], [-74, -54], [42, -69],
    [43, 44], [-72, -56], [45, 52], [46, 50], [47, -76], [-49, 48],
    [-47, 49], [-87, -41], [-52, 51], [-78, -50], [53, -73], [54, -75],
    [55, 57], [56, -80], [-86, -42], [-48, 58], [-44, -43], [-66, -62],
];

#[rustfmt::skip]
const A_BOOK_PS_IID_FINE_FREQ_DECODE: [[i8; 2]; 60] = [
    [1, -64], [2, 4], [3, -65], [-66, -62], [-63, 5], [6, 7],
    [-67, -61], [8, 9], [-68, -60], [10, 11], [-69, -59], [12, 13],
    [-70, -58], [14, 18], [-57, 15], [16, -72], [-54, 17], [-75, -53],
    [19, 37], [-56, 20], [21, -73], [22, 29], [23, -76], [24, -78],
    [25, 28], [26, 27], [-85, -43], [-83, -45], [-81, -47], [-52, 30],
    [-50, 31], [32, -79], [33, 34], [-82, -46], [35, 36], [-90, -89],
    [-92, -91], [38, -71], [-55, 39], [40, -74], [41, 50], [42, -77],
    [-49, 43], [44, 47], [45, 46], [-86, -42], [-88, -87], [48, 49],
    [-39, -38], [-41, -40], [-51, 51], [52, 59], [53, 56], [54, 55],
    [-35, -34], [-37, -36], [57, 58], [-94, -93], [-84, -44], [-80, -48],
];

#[repr(C)]
#[derive(Debug)]
pub(super) enum ParametricStereoParam {
    Icc,
    Iid,
}

#[repr(C)]
#[derive(Debug)]
pub(super) enum EnvParam {
    Env,
    Noise,
}

#[repr(u8)]
#[derive(Debug, PartialEq, Copy, Clone, Default)]
pub(super) enum Domain {
    #[default]
    Freq = 0,
    Time,
}

#[derive(Debug)]
pub struct DomainError;

impl TryFrom<u32> for Domain {
    type Error = DomainError;
    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Domain::Freq),
            1 => Ok(Domain::Time),
            _ => Err(DomainError {}),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub(super) enum QuantRes {
    Coarse,
    Fine,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub enum AmpRes {
    #[default]
    Res1_5 = 0,
    Res3_0 = 1,
}

impl From<u32> for AmpRes {
    fn from(value: u32) -> Self {
        match value {
            0 => AmpRes::Res1_5,
            1 => AmpRes::Res3_0,
            _ => panic!("Invalid resolution value"),
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub enum CouplingMode {
    #[default]
    Off,
    Level,
    Balance,
}

/// Decodes one Huffman code word
///
/// Reads bits from the bitstream until a valid codeword is found.
/// The table entries are interpreted either as index to the next entry
/// or - if negative - as the codeword.
///
/// # Parameters
///
/// - `ref_huffman_table`: Reference to huffman codebook table
/// - `bs`: Handle to Bitbuffer
///
/// # Return
///
/// Decoded value
fn decode_huffman_codeword(ref_huffman_table: &[[i8; 2]], bs: &mut Bitstream) -> isize {
    let mut index: i8 = 0;

    while index >= 0 {
        let bit = bs.read_bit() as usize;
        index = ref_huffman_table[index as usize][bit];
    }

    // Add offset.
    let value: isize = isize::from(index) + 64;

    value
}

/// Decodes a certain amount of Huffman code words
///
/// # Parameters
///
/// - `bs`: Handle to bitbuffer
/// - `index`: Slice of index values
/// - `param`: Type: ICC or IID
/// - `domain`: Domain: Freq or Time
/// - `quant_res`: Quantisation Resolution: Coarse or Fine
///
/// # Return
///
/// Decoded values
pub(super) fn decode_huffman(
    bs: &mut Bitstream,
    index: &mut [i8],
    param: ParametricStereoParam,
    domain: Domain,
    quant_res: QuantRes,
) {
    let current_table: &[[i8; 2]] = match param {
        ParametricStereoParam::Icc => match domain {
            Domain::Freq => &A_BOOK_PS_ICC_FREQ_DECODE,
            Domain::Time => &A_BOOK_PS_ICC_TIME_DECODE,
        },
        ParametricStereoParam::Iid => match domain {
            Domain::Freq => match quant_res {
                QuantRes::Coarse => &A_BOOK_PS_IID_FREQ_DECODE,
                QuantRes::Fine => &A_BOOK_PS_IID_FINE_FREQ_DECODE,
            },
            Domain::Time => match quant_res {
                QuantRes::Coarse => &A_BOOK_PS_IID_TIME_DECODE,
                QuantRes::Fine => &A_BOOK_PS_IID_FINE_TIME_DECODE,
            },
        },
    };

    for item in index.iter_mut() {
        *item = decode_huffman_codeword(current_table, bs)
            .try_into()
            .unwrap();
    }
}

/// Decodes a certain amount of Huffman code words
///
/// # Parameters
///
/// - `bs`:Handle to bitbuffer
/// - `inde`: Slice of index values
/// - `param`: Type: Env or Noise
/// - `domain`: Coded: Freq or Time
/// - `amp_res`: Amplification resolution: Res1_5 or Res3_0
/// - `coupling`: Coupling: Level or Balance
/// - `compensation_factor`: Compensation factor
///
/// # Return
///
/// Decoded values
pub(super) fn decode_huffman_float(
    bs: &mut Bitstream,
    index: &mut [f32],
    param: EnvParam,
    domain: Domain,
    amp_res: AmpRes,
    coupling: CouplingMode,
    compensation_factor: u8,
) {
    let current_table: &[[i8; 2]] = match param {
        EnvParam::Env => match domain {
            Domain::Freq => match coupling {
                CouplingMode::Balance => match amp_res {
                    AmpRes::Res1_5 => &HUFF_BOOK_ENV_BALANCE_10F,
                    AmpRes::Res3_0 => &HUFF_BOOK_ENV_BALANCE_11F,
                },
                CouplingMode::Level | CouplingMode::Off => match amp_res {
                    AmpRes::Res1_5 => &HUFF_BOOK_ENV_LEVEL_10F,
                    AmpRes::Res3_0 => &HUFF_BOOK_ENV_LEVEL_11F,
                },
            },
            Domain::Time => match coupling {
                CouplingMode::Balance => match amp_res {
                    AmpRes::Res1_5 => &HUFF_BOOK_ENV_BALANCE_10T,
                    AmpRes::Res3_0 => &HUFF_BOOK_ENV_BALANCE_11T,
                },
                CouplingMode::Level | CouplingMode::Off => match amp_res {
                    AmpRes::Res1_5 => &HUFF_BOOK_ENV_LEVEL_10T,
                    AmpRes::Res3_0 => &HUFF_BOOK_ENV_LEVEL_11T,
                },
            },
        },
        EnvParam::Noise => match domain {
            Domain::Freq => match coupling {
                CouplingMode::Balance => &HUFF_BOOK_ENV_BALANCE_11F,
                CouplingMode::Level | CouplingMode::Off => &HUFF_BOOK_ENV_LEVEL_11F,
            },
            Domain::Time => match coupling {
                CouplingMode::Balance => &HUFF_BOOK_NOISE_BALANCE_11T,
                CouplingMode::Level | CouplingMode::Off => &HUFF_BOOK_NOISE_LEVEL_11T,
            },
        },
    };

    for item in index.iter_mut() {
        *item = (decode_huffman_codeword(current_table, bs) << compensation_factor) as f32;
    }
}
