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
//! Tables used by the decorellator module

use super::{decorr_common::ReverbBandFiltType, decorr_constants::*};
use num_complex::Complex;

// ------------------------------------------------------------------------- //
// REV_FILTTYPE_<xyz> tables define the type of processing (filtering with
// different properties or pure delay) done in each reverb band.
// This is mapped to specialized routines.

/// Defines the MPS processing done in each reverb band.
pub(super) static REV_FILTTYPE_MPS: [ReverbBandFiltType; NUM_DECORR_BANDS] = [
    ReverbBandFiltType::CommonReal,
    ReverbBandFiltType::CommonReal,
    ReverbBandFiltType::CommonReal,
    ReverbBandFiltType::CommonReal,
];

/// Defines the PS processing done in each reverb band.
pub(super) static REV_FILTTYPE_PS: [ReverbBandFiltType; NUM_DECORR_BANDS] = [
    ReverbBandFiltType::IndepCplxPs,
    ReverbBandFiltType::Delay,
    ReverbBandFiltType::Delay,
    ReverbBandFiltType::NotExist,
];

/// Defines the LD processing done in each reverb band.
pub(super) static REV_FILTTYPE_LD: [ReverbBandFiltType; NUM_DECORR_BANDS] = [
    ReverbBandFiltType::NotExist,
    ReverbBandFiltType::CommonReal,
    ReverbBandFiltType::CommonReal,
    ReverbBandFiltType::CommonReal,
];
// ------------------------------------------------------------------------- //

// The REV_BANDOFFSET_<xyz> tables are mapping (hybrid) bands to the corresponding reverb bands.
// Within each reverb band the same processing is applied.
// Instead of QMF split frequencies, the corresponding hybrid band offsets are stored directly.

/// Maps the hybrid bands to the corresponding reverb bands.
/// `REV_BANDOFFSET_USAC[]` is equivalent to `REV_BANDOFFSET_MPS_HQ[]`.
#[rustfmt::skip]
pub(super) static REV_BANDOFFSET_MPS_HQ: [ [usize; NUM_DECORR_BANDS]; NUM_DECORR_CONFIGS ] =
    [ [8, 21, 30, 71], [8, 56, 71, 71], [0, 21, 71, 71]];

/// Maps the hybrid bands to the corresponding reverb bands.
#[rustfmt::skip]
pub(super) static REV_BANDOFFSET_PS_HQ: [usize; NUM_DECORR_BANDS] = [30, 42, 71, 71];

/// Maps the hybrid bands to the corresponding reverb bands.
#[rustfmt::skip]
pub(super) static REV_BANDOFFSET_LD: [ [usize; NUM_DECORR_BANDS]; NUM_DECORR_CONFIGS ] =
    [[0, 14, 23, 64], [0, 49, 64, 64], [0, 14, 64, 64]];
// ------------------------------------------------------------------------- //

/// Defines the number of delay elements within each reverb band.
pub(super) static REV_DELAY_MPS: [usize; NUM_DECORR_BANDS] = [8, 7, 2, 1];
/// Defines the number of delay elements within each reverb band.
pub(super) static REV_DELAY_PS_HQ: [usize; NUM_DECORR_BANDS] = [2, 14, 1, 0];
/// Defines the number of delay elements within each reverb band.
pub(super) static REV_DELAY_USAC: [usize; NUM_DECORR_BANDS] = [11, 10, 5, 2];
// ------------------------------------------------------------------------- //

/// Defines the filter order within each reverb band.
pub(super) static REV_FILTERORDER_MPS: [usize; NUM_DECORR_BANDS] = [
    DECORR_FILTER_ORDER_BAND_0_MPS,
    DECORR_FILTER_ORDER_BAND_1_MPS,
    DECORR_FILTER_ORDER_BAND_2_MPS,
    DECORR_FILTER_ORDER_BAND_3_MPS,
];
/// Defines the filter order within each reverb band.
pub(super) static REV_FILTERORDER_PS: [usize; NUM_DECORR_BANDS] = [
    DECORR_FILTER_ORDER_PS,
    DECORR_FILTER_ORDER_PS,
    DECORR_FILTER_ORDER_PS,
    DECORR_FILTER_ORDER_PS,
];
/// Defines the filter order within each reverb band.
pub(super) static REV_FILTERORDER_USAC: [usize; NUM_DECORR_BANDS] = [
    DECORR_FILTER_ORDER_BAND_0_USAC,
    DECORR_FILTER_ORDER_BAND_1_USAC,
    DECORR_FILTER_ORDER_BAND_2_USAC,
    DECORR_FILTER_ORDER_BAND_3_USAC,
];

// ------------------------------------------------------------------------- //
/// Initialization values of ring buffer offsets for the 3 concatenated allpass filters.
// (PS type decorrelator)
pub(super) static STATE_BUFFER_OFFSET_INIT: [usize; NUM_ALLPASS_LINKS] = [0, 3, 7];

// ------------------------------------------------------------------------- //
/// Mapping of hybrid bands to processing bands table for PS decorrorelator
/// running in legacy PS decoder.
#[rustfmt::skip]
pub static KERNELS_20_TO_71_PS: [usize; (71) + 1] = [
    0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 14,
    15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19];

/// Mapping of processing bands to hybrid bands table for PS decorrelator
/// running in legacy PS decoder.
#[rustfmt::skip]
pub static KERNELS_20_TO_71_OFFSET_PS: [usize; DECORR_MAX_PARAMETER_BANDS_PS + 1] =
    [0, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 21, 25, 30, 42, 71];

/// Mapping of hybrid bands to processing bands table for MPS decorrorelator.
#[rustfmt::skip]
pub static KERNELS_28_TO_71: [usize; (71) + 1] = [
    0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
    23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
    26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27];

/// Mapping of processing bands to hybrid bands table for MPS decorrelator.
#[rustfmt::skip]
pub static KERNELS_28_TO_71_OFFSET: [usize; DECORR_MAX_PARAMETER_BANDS_MPS + 1] =
    [0,  2,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
     17, 18, 19, 21, 23, 25, 27, 30, 33, 37, 42, 48, 55, 71];

/// Mapping of hybrid bands to processing bands table for LD decorrorelator.
// LD-MPS defined in SAOC standart (mapping qmf -> param bands).
#[rustfmt::skip]
pub static KERNELS_23_TO_64: [usize; (64) + 1] = [
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 12, 13, 13, 14,
    14, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19,
    19, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
];

/// Mapping of processing bands to hybrid bands table for LD decorrelator.
#[rustfmt::skip]
pub static  KERNELS_23_TO_64_OFFSET: [usize; DECORR_MAX_PARAMETER_BANDS_LD + 1] = [
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
    12, 14, 16, 18, 20, 23, 26, 30, 35, 41, 48, 64];
// ------------------------------------------------------------------------- //

#[rustfmt::skip]
pub(super) static DECORR_PS_COEFFS_CPLX: [[Complex<f32>; 4]; 30] = [
  [ Complex { re:  0.729774584, im:  0.683687835 }, Complex { re: -0.642173570, im:  0.109480372 },
    Complex { re: -0.540401549, im:  0.163929018 }, Complex { re: -0.485003641, im:  0.066501914 },
  ],
  [ Complex { re:  0.729774584, im: -0.683687835 }, Complex { re: -0.642173570, im: -0.109480372 },
    Complex { re: -0.540401549, im: -0.163929018 }, Complex { re: -0.485003641, im: -0.066501914 },
  ],
  [ Complex { re: -0.634696796, im:  0.772761268 }, Complex { re: -0.569623579, im:  0.316072497 },
    Complex { re: -0.358253384, im:  0.436533012 }, Complex { re: -0.449202739, im:  0.194596854 },
  ],
  [ Complex { re: -0.634696796, im: -0.772761268 }, Complex { re: -0.569623579, im: -0.316072497 },
    Complex { re: -0.358253384, im: -0.436533012 }, Complex { re: -0.449202739, im: -0.194596854 },
  ],
  [ Complex { re: -0.812465280, im:  0.583009579 }, Complex { re: -0.432719982, im: -0.486956120 },
    Complex { re: -0.055352055, im: -0.561998850 }, Complex { re: -0.380243605, im: -0.308327484 },
  ],
  [ Complex { re:  0.528845751, im:  0.848717958 }, Complex { re: -0.246929554, im: -0.602825548 },
    Complex { re:  0.266206280, im: -0.498036920 }, Complex { re: -0.283216509, im: -0.399298692 },
  ],
  [ Complex { re:  0.320199662, im: -0.947350081 }, Complex { re:  0.076568451, im: -0.646923580 },
    Complex { re:  0.553867222, im: -0.110171040 }, Complex { re: -0.101154542, im: -0.478976821 },
  ],
  [ Complex { re: -0.440644343, im:  0.897681771 }, Complex { re:  0.464240565, im: -0.457004971 },
    Complex { re:  0.313740579, im:  0.469545958 }, Complex { re:  0.161840883, im: -0.462015763 },
  ],
  [ Complex { re:  0.553609589, im: -0.832776335 }, Complex { re:  0.648046455, im: -0.066397571 },
    Complex { re: -0.313740579, im:  0.469545958 }, Complex { re:  0.377932524, im: -0.311155980 },
  ],
  [ Complex { re: -0.657177941, im:  0.753735467 }, Complex { re:  0.547269753, im:  0.353367603 },
    Complex { re: -0.553867222, im: -0.110171040 }, Complex { re:  0.484493956, im: -0.070118778 },
  ],
  [ Complex { re: -0.611665125, im: -0.791116789 }, Complex { re: -0.010232360, im:  0.651358691 },
    Complex { re:  0.216108269, im: -0.521731515 }, Complex { re:  0.382533518, im:  0.305481822 },
  ],
  [ Complex { re: -0.387032847, im: -0.922065928 }, Complex { re: -0.606008786, im:  0.125498386 },
    Complex { re:  0.205302856, im:  0.495644939 }, Complex { re: -0.089296183, im:  0.456411275 },
  ],
  [ Complex { re: -0.136234308, im: -0.990676644 }, Complex { re: -0.241268873, im: -0.534351322 },
    Complex { re: -0.469558363, im: -0.194497442 }, Complex { re: -0.422509446, im:  0.124912401 },
  ],
  [ Complex { re:  0.123774669, im: -0.992310350 }, Complex { re:  0.442803947, im: -0.332466608 },
    Complex { re:  0.443471787, im: -0.183692029 }, Complex { re: -0.289106817, im: -0.299274326 },
  ],
  [ Complex { re:  0.375415571, im: -0.926856596 }, Complex { re:  0.396286516, im:  0.338460660 },
    Complex { re: -0.172886615, im:  0.417385212 }, Complex { re:  0.123942928, im: -0.371503451 },
  ],
  [ Complex { re:  0.601675626, im: -0.798740534 }, Complex { re: -0.228620818, im:  0.431789587 },
    Complex { re: -0.162081202, im: -0.391298636 }, Complex { re:  0.362545275, im: -0.058005447 },
  ],
  [ Complex { re:  0.787257993, im: -0.616623752 }, Complex { re: -0.439845263, im: -0.120328048 },
    Complex { re:  0.365212060, im:  0.151275788 }, Complex { re:  0.204457859, im:  0.275001803 },
  ],
  [ Complex { re:  0.919615944, im: -0.392818680 }, Complex { re:  0.019946538, im: -0.422965321 },
    Complex { re: -0.339125484, im:  0.140470375 }, Complex { re: -0.138641424, im:  0.286410751 },
  ],
  [ Complex { re:  0.989801109, im: -0.142456183 }, Complex { re:  0.385043215, im: -0.067200799 },
    Complex { re:  0.129664962, im: -0.313038909 }, Complex { re: -0.293594190, im:  0.008764959 },
  ],
  [ Complex { re:  0.993068457, im:  0.117537397 }, Complex { re:  0.137112214, im:  0.331018167 },
    Complex { re:  0.118859548, im:  0.286952333 }, Complex { re: -0.131560249, im: -0.234917729 },
  ],
  [ Complex { re:  0.929197090, im:  0.369584587 }, Complex { re: -0.266487340, im:  0.187290440 },
    Complex { re: -0.260865757, im: -0.108054135 }, Complex { re:  0.134063250, im: -0.204792100 },
  ],
  [ Complex { re:  0.802505183, im:  0.596645147 }, Complex { re: -0.216821063, im: -0.197291987 },
    Complex { re:  0.234779182, im: -0.097248721 }, Complex { re:  0.219215946, im:  0.021764742 },
  ],
  [ Complex { re:  0.621558036, im:  0.783368118 }, Complex { re:  0.129104451, im: -0.226344198 },
    Complex { re: -0.086443308, im:  0.208692606 }, Complex { re:  0.072942083, im:  0.181724017 },
  ],
  [ Complex { re:  0.398589006, im:  0.917129655 }, Complex { re:  0.217924315, im:  0.067042273 },
    Complex { re: -0.075637894, im: -0.182606030 }, Complex { re: -0.111480674, im:  0.130112687 },
  ],
  [ Complex { re:  0.148672434, im:  0.988886499 }, Complex { re: -0.015333396, im:  0.194829266 },
    Complex { re:  0.156519454, im:  0.064832481 }, Complex { re: -0.143069537, im: -0.033161738 },
  ],
  [ Complex { re: -0.111295485, im:  0.993787359 }, Complex { re: -0.161235020, im:  0.022947141 },
    Complex { re: -0.130432879, im:  0.054027067 }, Complex { re: -0.030622180, im: -0.118492497 },
  ],
  [ Complex { re: -0.363739013, im:  0.931500902 }, Complex { re: -0.046053464, im: -0.121876956 },
    Complex { re:  0.043221654, im: -0.104346303 }, Complex { re:  0.072725757, im: -0.065551550 },
  ],
  [ Complex { re: -0.591591115, im:  0.806238149 }, Complex { re:  0.081671634, im: -0.053648236 },
    Complex { re:  0.032416240, im:  0.078259727 }, Complex { re:  0.068812376, im:  0.025632114 },
  ],
  [ Complex { re: -0.779447316, im:  0.626467782 }, Complex { re:  0.046781552, im:  0.045334478 },
    Complex { re: -0.052173151, im: -0.021610827 }, Complex { re:  0.006059286, im:  0.048577726 },
  ],
  [ Complex { re: -0.914607160, im:  0.404343596 }, Complex { re: -0.017018799, im:  0.027772155 },
    Complex { re:  0.026086576, im: -0.010805413 }, Complex { re: -0.020135840, im:  0.013916736 },
  ]
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL0_USAC: [
  [f32; DECORR_FILTER_ORDER_BAND_0_USAC + 1]; MAX_DECORR_SEED_USAC] =
[
  [  0.089800000,  0.128058420, -0.356361682, 0.122672356, 0.033342738, 0.000599386, -0.115077189,
    -0.173641017, -0.256828359, -0.314818340, 1.000000000,
  ],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL1_USAC: [
  [f32; DECORR_FILTER_ORDER_BAND_1_USAC + 1]; MAX_DECORR_SEED_USAC] =
[
  [ -0.122100000, -0.016263782, 0.045767743, 0.064217647, -0.126110713, 0.123204013, -0.088940066,
    -0.287137510,  1.000000000,
  ],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL2_USAC: [
  [f32; DECORR_FILTER_ORDER_BAND_2_USAC + 1]; MAX_DECORR_SEED_USAC] =
[
  [ 0.035700000, -0.032632773, 0.129403050, 1.000000000, ],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL3_USAC: [
  [f32; DECORR_FILTER_ORDER_BAND_3_USAC + 1]; MAX_DECORR_SEED_USAC] =
[
  [-0.013000000, 0.034742400, 1.000000000, ],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL1_LD: [
  [f32; DECORR_FILTER_ORDER_BAND_1_LD + 1]; MAX_DECORR_SEED_LD] =
[
  [
     0.335600000,  0.002489459, -0.157229071,  0.280750348,
    -0.194285738,  0.384060026, -0.408438898, -0.175048302,
     0.555958867, -0.493582951,  0.056741583, -0.065814836,
     0.337896164,  0.228442654, -0.702533060,  1.000000000,
  ],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL2_LD: [
  [f32; DECORR_FILTER_ORDER_BAND_2_LD + 1 + DECORR_ZERO_PADDING]; MAX_DECORR_SEED_LD] =
[
  [-0.462400000,  0.234119330,  0.516363762, -0.025348829, -0.287103101, 0.015317060, 1.000000000],
];

#[rustfmt::skip]
pub(super) static DECORR_NUMERATOR_REAL3_LD: [
  [f32; DECORR_FILTER_ORDER_BAND_3_LD + 1]; MAX_DECORR_SEED_LD] =
[
  [ 0.246800000,  0.020795821, -0.389849120, 1.000000000,],
];
