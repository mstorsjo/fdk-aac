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
//! Constants and tables for the HBE module
//!
//! This module contains constants and tables used by the HBE module.

use num_complex::Complex;

pub(super) const HBE_QMF_FILTER_STATE_ANA_SIZE: u16 = 400;
pub(super) const HBE_QMF_FILTER_STATE_SYN_SIZE: u16 = 200;

pub(super) const HBE_QMF_WIN_LEN: u8 = 13;
pub(super) const HBE_QMF_IN_BUF_SIZE: u8 = 13;

pub(super) const HBE_MAX_QMF_SYN_BANDS: u8 = 20;
pub(super) const HBE_QMF_SYNTH_CHANNELS: u8 = 64;
pub(super) const HBE_MAX_OUT_SLOTS: u8 = 11;

pub const HBE_MAX_NUM_PATCHES: u8 = 6;

pub const HBE_MAX_STRETCH: u8 = 4;

pub(super) const HBE_TWID_M_MOD: u8 = 96;

pub(super) const HBE_LO: usize = 0;

pub(super) const HBE_HI: usize = 1;

pub(super) const HBE_MIN_NRG: f32 = 2.2204e-16;

pub(super) const PMIN: u8 = 12;

pub(super) static START_SUBBAND2K_L: [u8; 33] = [
    0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 8, 8, 8, 8, 8, 10, 10, 10, 12, 12, 12,
    12, 12, 12, 12,
];

pub(super) static WINGAIN: [f32; 3] = [5.0 / 3.0, 5.6568 / 3.0, 6.0 / 3.0];

pub(super) static TABLE_STRETCH: [f32; 3] = [0.5, 1.0 / 3.0, 0.25];

pub(super) static SLOT_STRETCH3: [u8; 10] = [0, 0, 0, 1, 3, 4, 6, 7, 9, 10];

pub(super) static SLOT_STRETCH4: [u8; 9] = [0, 0, 0, 0, 2, 4, 6, 8, 10];

pub(super) static HINT1_CPLX: [Complex<f32>; 8] = [
    Complex::<f32> {
        re: 0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: -0.39840335,
    },
];

pub(super) static HINT2_CPLX: [Complex<f32>; 8] = [
    Complex::<f32> {
        re: 0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: -0.39840335,
        im: 0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: -0.39840335,
    },
    Complex::<f32> {
        re: 0.39840335,
        im: 0.39840335,
    },
];

pub(super) static COS_64: [f32; 64] = [
    0.0122716, -0.0368074, 0.061321, -0.0857957, 0.110221, -0.134579, 0.158857, -0.183039,
    0.207114, -0.231057, 0.254868, -0.278519, 0.302009, -0.32531, 0.348422, -0.371324, 0.393988,
    -0.416429, 0.438619, -0.460545, 0.48218, -0.503538, 0.524593, -0.545319, 0.565729, -0.585798,
    0.605515, -0.624866, 0.643829, -0.662405, 0.680604, -0.698372, 0.71574, -0.732655, 0.74913,
    -0.765173, 0.780736, -0.795829, 0.81046, -0.824586, 0.838232, -0.851356, 0.863968, -0.876075,
    0.887639, -0.898669, 0.909171, -0.919112, 0.928511, -0.93734, 0.945605, -0.953309, 0.96043,
    -0.966973, 0.972942, -0.978323, 0.983103, -0.987302, 0.990906, -0.993905, 0.996313, -0.998119,
    0.999322, -0.999925,
];

pub(super) static TWIDDLE_CPLX: [Complex<f32>; HBE_TWID_M_MOD as usize / 2] = [
    Complex::<f32> {
        re: 1.000000000,
        im: 0.000000000,
    },
    Complex::<f32> {
        re: 0.997859001,
        im: -0.065403096,
    },
    Complex::<f32> {
        re: 0.991445005,
        im: -0.130526006,
    },
    Complex::<f32> {
        re: 0.980785012,
        im: -0.195089996,
    },
    Complex::<f32> {
        re: 0.965925992,
        im: -0.258819014,
    },
    Complex::<f32> {
        re: 0.946929991,
        im: -0.321438998,
    },
    Complex::<f32> {
        re: 0.923879981,
        im: -0.382683009,
    },
    Complex::<f32> {
        re: 0.896872997,
        im: -0.442288995,
    },
    Complex::<f32> {
        re: 0.866024971,
        im: -0.500000000,
    },
    Complex::<f32> {
        re: 0.831470013,
        im: -0.555570006,
    },
    Complex::<f32> {
        re: 0.793353021,
        im: -0.608761013,
    },
    Complex::<f32> {
        re: 0.751839995,
        im: -0.659345984,
    },
    Complex::<f32> {
        re: 0.707107008,
        im: -0.707107008,
    },
    Complex::<f32> {
        re: 0.659345984,
        im: -0.751839995,
    },
    Complex::<f32> {
        re: 0.608761013,
        im: -0.793353021,
    },
    Complex::<f32> {
        re: 0.555570006,
        im: -0.831470013,
    },
    Complex::<f32> {
        re: 0.500000000,
        im: -0.866024971,
    },
    Complex::<f32> {
        re: 0.442288995,
        im: -0.896872997,
    },
    Complex::<f32> {
        re: 0.382683009,
        im: -0.923879981,
    },
    Complex::<f32> {
        re: 0.321438998,
        im: -0.946929991,
    },
    Complex::<f32> {
        re: 0.258819014,
        im: -0.965925992,
    },
    Complex::<f32> {
        re: 0.195089996,
        im: -0.980785012,
    },
    Complex::<f32> {
        re: 0.130526006,
        im: -0.991445005,
    },
    Complex::<f32> {
        re: 0.065403096,
        im: -0.997859001,
    },
    Complex::<f32> {
        re: 0.000000000,
        im: -1.000000000,
    },
    Complex::<f32> {
        re: -0.065403096,
        im: -0.997859001,
    },
    Complex::<f32> {
        re: -0.130526006,
        im: -0.991445005,
    },
    Complex::<f32> {
        re: -0.195089996,
        im: -0.980785012,
    },
    Complex::<f32> {
        re: -0.258819014,
        im: -0.965925992,
    },
    Complex::<f32> {
        re: -0.321438998,
        im: -0.946929991,
    },
    Complex::<f32> {
        re: -0.382683009,
        im: -0.923879981,
    },
    Complex::<f32> {
        re: -0.442288995,
        im: -0.896872997,
    },
    Complex::<f32> {
        re: -0.500000000,
        im: -0.866024971,
    },
    Complex::<f32> {
        re: -0.555570006,
        im: -0.831470013,
    },
    Complex::<f32> {
        re: -0.608761013,
        im: -0.793353021,
    },
    Complex::<f32> {
        re: -0.659345984,
        im: -0.751839995,
    },
    Complex::<f32> {
        re: -0.707107008,
        im: -0.707107008,
    },
    Complex::<f32> {
        re: -0.751839995,
        im: -0.659345984,
    },
    Complex::<f32> {
        re: -0.793353021,
        im: -0.608761013,
    },
    Complex::<f32> {
        re: -0.831470013,
        im: -0.555570006,
    },
    Complex::<f32> {
        re: -0.866024971,
        im: -0.500000000,
    },
    Complex::<f32> {
        re: -0.896872997,
        im: -0.442288995,
    },
    Complex::<f32> {
        re: -0.923879981,
        im: -0.382683009,
    },
    Complex::<f32> {
        re: -0.946929991,
        im: -0.321438998,
    },
    Complex::<f32> {
        re: -0.965925992,
        im: -0.258819014,
    },
    Complex::<f32> {
        re: -0.980785012,
        im: -0.195089996,
    },
    Complex::<f32> {
        re: -0.991445005,
        im: -0.130526006,
    },
    Complex::<f32> {
        re: -0.997859001,
        im: -0.065403000,
    },
];

pub(super) static POST_TWIDDLE_COS_8: [f32; 8] = [
    -0.0490112, 0.145142, -0.235687, 0.3172, -0.386505, 0.440948, -0.478485, 0.497589,
];

pub(super) static POST_TWIDDLE_COS_16: [f32; 16] = [
    -0.0245361, 0.0733643, -0.12149, 0.168457, -0.213776, 0.257050, -0.297852, 0.335785,
    -0.3704830, 0.401611, -0.428864, 0.451996, -0.470764, 0.485016, -0.494598, 0.499390,
];

pub(super) static POST_TWIDDLE_COS_24: [f32; 24] = [
    -0.0163574, 0.0490112, -0.0814514, 0.113525, -0.145142, 0.176117, -0.206360, 0.235687,
    -0.2640380, 0.2912290, -0.3172000, 0.341797, -0.364929, 0.386505, -0.406433, 0.424591,
    -0.4409480, 0.4554440, -0.4679570, 0.478485, -0.486938, 0.493317, -0.497589, 0.499725,
];

pub(super) static POST_TWIDDLE_COS_32: [f32; 32] = [
    -0.0122681, 0.0367737, -0.0612183, 0.0854797, -0.1095581, 0.1333618, -0.1568298, 0.1799622,
    -0.2026062, 0.2247925, -0.2464600, 0.2674866, -0.2879028, 0.3076172, -0.3265991, 0.3447571,
    -0.3621216, 0.3786011, -0.3941650, 0.4087830, -0.4224243, 0.4350586, -0.4466248, 0.4570923,
    -0.4664917, 0.4747620, -0.4819031, 0.4878540, -0.4926453, 0.4962463, -0.4986572, 0.4998474,
];

pub(super) static POST_TWIDDLE_COS_40: [f32; 40] = [
    -0.0098267, 0.0294495, -0.0490112, 0.0685120, -0.0878906, 0.1071472, -0.1262512, 0.1451416,
    -0.1638184, 0.1822205, -0.2003784, 0.2182007, -0.2356873, 0.2528381, -0.2695618, 0.2858887,
    -0.3017883, 0.3171997, -0.3321228, 0.3465576, -0.3604126, 0.3737488, -0.3865051, 0.3986511,
    -0.4101868, 0.4211121, -0.4313660, 0.4409485, -0.4498596, 0.4580994, -0.4656067, 0.4724121,
    -0.4784851, 0.4837952, -0.4883728, 0.4922180, -0.4952698, 0.4975891, -0.4991455, 0.4999084,
];

pub(super) static POST_TWIDDLE_SIN_8: [f32; 8] = [
    0.4975891, -0.4784851, 0.4409485, -0.3865051, 0.3171997, -0.2356873, 0.1451416, -0.0490112,
];

pub(super) static POST_TWIDDLE_SIN_16: [f32; 16] = [
    0.4993896, -0.4945984, 0.4850159, -0.4707642, 0.4519958, -0.4288635, 0.4016113, -0.3704834,
    0.3357849, -0.2978516, 0.2570496, -0.2137756, 0.1684570, -0.1214905, 0.0733643, -0.0245361,
];

pub(super) static POST_TWIDDLE_SIN_24: [f32; 24] = [
    0.4997253, -0.4975891, 0.4933167, -0.4869385, 0.4784851, -0.4679565, 0.4554443, -0.4409485,
    0.4245911, -0.4064331, 0.3865051, -0.3649292, 0.3417969, -0.3171997, 0.2912292, -0.2640381,
    0.2356873, -0.2063599, 0.1761169, -0.1451416, 0.1135254, -0.0814514, 0.0490112, -0.0163574,
];

pub(super) static POST_TWIDDLE_SIN_32: [f32; 32] = [
    0.4998474, -0.4986572, 0.4962463, -0.4926453, 0.4878540, -0.4819031, 0.4747620, -0.4664917,
    0.4570923, -0.4466248, 0.4350586, -0.4224243, 0.4087830, -0.3941650, 0.3786011, -0.3621216,
    0.3447571, -0.3265991, 0.3076172, -0.2879028, 0.2674866, -0.2464600, 0.2247925, -0.2026062,
    0.1799622, -0.1568298, 0.1333618, -0.1095581, 0.0854797, -0.0612183, 0.0367737, -0.0122681,
];

pub(super) static POST_TWIDDLE_SIN_40: [f32; 40] = [
    0.4999084, -0.4991455, 0.4975891, -0.4952698, 0.4922180, -0.4883728, 0.4837952, -0.4784851,
    0.4724121, -0.4656067, 0.4580994, -0.4498596, 0.4409485, -0.4313660, 0.4211121, -0.4101868,
    0.3986511, -0.3865051, 0.3737488, -0.3604126, 0.3465576, -0.3321228, 0.3171997, -0.3017883,
    0.2858887, -0.2695618, 0.2528381, -0.2356873, 0.2182007, -0.2003784, 0.1822205, -0.1638184,
    0.1451416, -0.1262512, 0.1071472, -0.0878906, 0.0685120, -0.0490112, 0.0294495, -0.0098267,
];

pub(super) static PRE_MOD_COS: [f32; 32] = [
    -0.698376, 0.732654, 0.662416, -0.765167, -0.62486, 0.795837, 0.585798, -0.824589, -0.545325,
    0.851355, 0.503538, -0.87607, -0.460539, 0.898674, 0.41643, -0.919114, -0.371317, 0.937339,
    0.32531, -0.953306, -0.27852, 0.966976, 0.231058, -0.978317, -0.18304, 0.987301, 0.134581,
    -0.993907, -0.0857973, 0.998118, 0.0368072, -0.999925,
];

pub(super) static PRE_MOD_SIN: [f32; 32] = [
    0.715731, 0.680601, -0.749136, -0.643832, 0.780737, 0.605511, -0.810457, -0.565732, 0.838225,
    0.52459, -0.863973, -0.482184, 0.88764, 0.438616, -0.909168, -0.393992, 0.928506, 0.348419,
    -0.945607, -0.302006, 0.960431, 0.254866, -0.97294, -0.207111, 0.983105, 0.158858, -0.990903,
    -0.110222, 0.996313, 0.0613207, -0.999322, -0.0122715,
];
