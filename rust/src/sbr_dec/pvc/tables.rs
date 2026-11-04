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
//! pvc_dec tables

use super::consts::*;

/// Annex D - Table D.1 — Smoothing window table for ns =16
pub(super) static SC_16: [f32; 16] = [
    7.9120074720078801e-002,
    7.8929352455315405e-002,
    7.8356252741836802e-002,
    7.7397889626247995e-002,
    7.6049149540866515e-002,
    7.4302186813164264e-002,
    7.2145604983589517e-002,
    6.9563171210312247e-002,
    6.6531787206720316e-002,
    6.3018197524834882e-002,
    5.8973400826190611e-002,
    5.4322528277253423e-002,
    4.8944795582858927e-002,
    4.2628371779453236e-002,
    3.4946569619925177e-002,
    2.4770667091351901e-002,
];

/// Annex D - Table D.3 — Smoothing window table for ns =12
pub(super) static SC_12: [f32; 12] = [
    1.0440702692045410e-001,
    1.0395945931915132e-001,
    1.0261281883061703e-001,
    1.0035462721037963e-001,
    9.7161686576578310e-002,
    9.2995740369570576e-002,
    8.7795494664707929e-002,
    8.1461666975864891e-002,
    7.3826916738979523e-002,
    6.4587661325082549e-002,
    5.3116303570036522e-002,
    3.7720597498577493e-002,
];

/// Annex D - Table D.2 — Smoothing window table for ns =4
pub(super) static SC_4: [f32; 4] = [
    2.9233807677393114e-001,
    2.8099141963307617e-001,
    2.4582604080136389e-001,
    1.8084446279162875e-001,
];

/// Annex D - Table D.4 — Smoothing window table for ns =3
pub(super) static SC_3: [f32; 3] = [
    3.7911649807579761e-001,
    3.5280765527510910e-001,
    2.6807584664909323e-001,
];

pub(super) static G_A_SCALING_COEF_MODE1: [f32; NBLOW + 1] =
    [1.0 / 256.0, 1.0 / 256.0, 1.0 / 128.0, 1.0 / 2.0];

pub(super) static G_A_SCALING_COEF_MODE2: [f32; NBLOW + 1] =
    [1.0 / 128.0, 1.0 / 128.0, 1.0 / 64.0, 1.0];

/// Annex D - Table D.7 — Prediction coefficient matrix table for kb={0,1,2} for bs_pvc_mode =1
pub(super) static G_3A_TAB1_MODE1: [[[i8; NBLOW]; NBHIGH_MODE1]; NTAB1] = [
    [
        [79, -13, 120],
        [91, 15, 87],
        [87, 24, 85],
        [82, 32, 80],
        [77, 25, 80],
        [101, 79, 32],
        [69, 61, 54],
        [87, 35, 55],
    ],
    [
        [76, 5, 89],
        [95, 14, 71],
        [83, 40, 64],
        [55, 65, 64],
        [30, 72, 61],
        [-3, 110, 51],
        [21, 84, 63],
        [10, 91, 57],
    ],
    [
        [71, -6, 99],
        [95, 19, 67],
        [87, 35, 65],
        [52, 78, 61],
        [60, 68, 53],
        [46, 124, 25],
        [46, 52, 61],
        [49, 56, 51],
    ],
];

/// Annex D - Table D.8 — Prediction coefficient matrix table for kb=3 for bs_pvc_mode =1
pub(super) static G_2A_TAB2_MODE1: [[i8; NBHIGH_MODE1]; NTAB2] = [
    [-53, -47, -52, -46, -30, -21, -25, -24],
    [-128, -128, -128, -128, -128, -128, -128, -128],
    [-124, -116, -120, -125, -112, -109, -122, -128],
    [-41, -40, -64, -57, -49, -27, -15, -10],
    [-91, -90, -86, -88, -80, -79, -72, -72],
    [-41, -53, -63, -61, -59, -55, -55, -50],
    [-54, -75, -72, -77, -84, -74, -69, -72],
    [-63, -60, -61, -59, -58, -54, -54, -53],
    [-32, -31, -40, -51, -53, -53, -50, -52],
    [-37, -31, -33, -37, -36, -39, -39, -42],
    [-32, -34, -35, -35, -32, -29, -27, -26],
    [-54, -46, -51, -50, -43, -37, -39, -37],
    [-46, -32, -37, -43, -37, -34, -29, -31],
    [-27, -37, -48, -46, -40, -35, -37, -35],
    [-64, -75, -65, -35, -29, -36, -36, -28],
    [-37, -50, -58, -49, -49, -47, -45, -44],
    [-55, -41, -38, -30, -23, -25, -33, -36],
    [10, 7, 10, 8, 25, 36, 31, 34],
    [30, 31, 17, 14, 34, 45, 51, 50],
    [-16, -38, -36, 24, 31, 25, 10, 30],
    [9, -8, -26, 5, 25, 17, 14, 11],
    [9, 16, 14, -26, -12, 32, 34, -6],
    [-14, -27, -8, 14, 24, 21, 13, 16],
    [21, 19, 22, 10, 13, 31, 29, 27],
    [-6, -1, -2, -1, 9, 17, 3, 11],
    [-2, -6, -14, -8, 12, 30, 17, 18],
    [-6, -8, 11, 23, 29, 23, 14, 22],
    [0, -13, -3, 10, 28, 23, -3, 8],
    [-22, -22, 3, 18, 30, 20, 9, 4],
    [2, -2, 4, -5, 12, 14, 7, 2],
    [-10, 2, 7, 11, 23, 23, 1, -1],
    [-11, -5, -2, 4, 18, 20, 12, 13],
    [16, 16, 14, 4, 7, 17, 15, 19],
    [12, 15, -5, -14, 10, 18, 9, 13],
    [13, 29, -15, -12, 42, 6, 59, 50],
    [-4, 8, 6, 2, 14, 23, 8, 14],
    [7, 2, -18, -18, 43, -10, 35, 19],
    [4, 2, 5, 8, 11, 14, -5, -5],
    [0, 4, 16, 24, 34, 37, 29, 31],
    [-5, 13, 7, 0, 12, 15, -4, 2],
    [0, 0, 0, 1, 5, 7, 3, 5],
    [4, 5, 8, 19, -1, -21, 12, 6],
    [5, 19, 14, 11, 18, 21, 9, 10],
    [9, 3, 9, 5, 18, 22, 17, 18],
    [20, 26, 6, 1, 16, 17, -2, 2],
    [1, 11, 11, 12, 24, 33, 16, 19],
    [18, 13, 10, 16, 28, 29, 13, 16],
    [3, 9, 20, 21, 27, 26, 1, -1],
    [8, 18, 19, 14, 22, 29, 20, 27],
    [7, 21, 28, 27, 32, 33, 17, 14],
    [18, 24, 25, 23, 32, 37, 26, 30],
    [12, 26, 29, 34, 47, 51, 39, 40],
    [14, 26, 23, 16, 10, 14, -1, 6],
    [26, 28, 24, 20, 26, 22, 10, 14],
    [30, 39, 37, 38, 39, 42, 33, 33],
    [-15, 10, 22, 28, 40, 37, 21, 25],
    [8, 18, 9, 8, 22, 23, -17, -10],
    [12, 11, 0, -4, 4, 9, -4, 3],
    [-5, -15, -8, 38, 36, 24, 29, 32],
    [-7, 1, 12, 15, 7, 8, 6, 7],
    [7, 6, 8, 4, 7, 13, 7, 9],
    [-2, 1, 6, 5, 19, 27, 20, 25],
    [9, 12, 14, 1, 8, 5, -5, -3],
    [7, 6, 3, 10, 22, 18, 4, 7],
    [4, 1, 0, 4, 31, 32, 14, 10],
    [3, -1, -10, -5, 21, 26, 0, 3],
    [-4, 24, 11, 45, 53, 35, 18, 9],
    [2, -2, 1, -1, 12, 17, 13, 15],
    [-6, -23, -39, -1, 13, 5, 13, 16],
    [-15, -32, -16, 1, 6, 6, 6, 16],
    [-23, -44, -41, 15, 20, 11, 13, 22],
    [0, -1, -18, -27, -1, 8, 2, -7],
    [-32, -38, -27, -2, 9, 2, -7, 4],
    [-32, -30, -12, 9, 19, 12, 13, 9],
    [-4, 2, 4, -1, 0, -1, -8, -9],
    [-2, -5, -19, -14, -2, -2, 8, 12],
    [-13, -17, -48, -29, 5, 17, -3, -1],
    [-6, -17, -22, -2, 13, 14, -2, 2],
    [-9, -5, -37, -33, 20, -35, 7, -2],
    [-2, 8, 0, -37, -27, 26, 19, -19],
    [-7, -2, -1, -12, -13, 0, 5, 2],
    [-17, -34, -40, -21, -22, -11, 14, 25],
    [-5, -4, -6, -20, -21, -19, -18, -24],
    [-18, -4, -3, 0, 4, -4, -16, -11],
    [0, -6, -12, -15, -11, -6, -5, -7],
    [-21, -16, -33, -29, -17, 7, 2, 5],
    [-9, -16, -26, -25, 6, 21, 6, 12],
    [-15, -28, -40, -22, 6, -14, 7, 9],
    [-1, -2, -2, -7, -1, -1, 2, -7],
    [-35, -12, -16, -15, -1, -1, -22, -15],
    [-16, -15, -3, 3, 3, -2, 0, 5],
    [-15, -10, -32, -33, -11, 1, -12, -8],
    [2, 3, -27, -36, -25, -3, 2, 8],
    [-20, -15, -11, -20, -14, -8, -10, -18],
    [-13, -12, -10, -12, -11, -15, -25, -22],
    [-9, -13, -20, -22, -17, -16, -18, -15],
    [-21, -10, -5, -6, -17, -13, -13, -9],
    [1, 3, -15, -10, 5, -8, -31, -21],
    [-11, -10, -10, -12, -5, -5, -1, 0],
    [-8, 1, -5, -6, -1, 3, -2, 4],
    [4, -5, 3, -3, -11, -9, -10, -5],
    [6, 9, -5, -12, -7, -6, -4, -1],
    [-11, -10, -15, -18, -11, -8, -11, -7],
    [-11, -7, -6, -4, 7, 9, 1, -5],
    [-41, -23, -24, -20, 0, 12, -2, -15],
    [-20, 4, -23, -33, 3, -24, 0, -6],
    [-26, -30, -1, 10, 19, 1, 0, -9],
    [-15, -6, -9, -11, 1, 6, 5, 10],
    [-10, -10, -4, -10, -24, 17, -14, -2],
    [-2, 8, 5, 18, -3, -48, 14, 7],
    [-15, -2, -9, -14, -5, 2, -6, -8],
    [-12, -22, -20, -13, -2, 1, -9, -10],
    [-1, -6, -5, -7, -1, 1, 4, 3],
    [0, -7, -12, -4, 5, -4, -9, -5],
    [-8, -1, -17, -20, -5, 4, -8, 3],
    [-21, -15, -19, -12, 2, 14, 11, 4],
    [-9, 1, -8, -12, -8, -17, -8, 4],
    [-21, -16, -9, -4, 16, 13, -8, -8],
    [-24, -2, -18, -24, -19, -9, -11, -8],
    [-19, -21, -23, -22, -14, -11, -12, -7],
    [-22, -14, -17, -18, -7, -2, -3, 2],
    [-6, -3, 2, 13, -6, -28, 15, 1],
    [-1, 8, 5, -10, -9, -5, -15, -15],
    [-12, -20, -18, -10, -18, -18, -8, 6],
    [-24, -6, -8, -24, -8, -23, -18, -7],
    [-27, -23, -16, 0, 0, -17, -13, -8],
    [-9, -5, -5, -9, -7, -7, -11, -16],
    [-3, -1, -14, -18, -14, -11, -15, -13],
];

/// Annex D - Prediction coefficient matrix table for kb={0,1,2} for bs_pvc_mode =2
pub(super) static G_3A_TAB1_MODE2: [[[i8; NBLOW]; NBHIGH_MODE2]; NTAB1] = [
    [
        [17, 0, 9],
        [39, -66, 30],
        [15, -29, 24],
        [-3, -12, 26],
        [4, -37, 33],
        [-4, -16, 27],
    ],
    [
        [22, -14, 14],
        [40, -23, 11],
        [43, -28, 12],
        [41, -27, 13],
        [37, -30, 13],
        [50, -44, 14],
    ],
    [
        [46, -28, 15],
        [60, -58, 27],
        [32, -27, 24],
        [22, -12, 20],
        [27, -36, 30],
        [26, -36, 26],
    ],
];

/// Annex D - Table D.10 — Prediction coefficient matrix table for kb=3 for bs_pvc_mode =2
pub(super) static G_2A_TAB2_MODE2: [[i8; NBHIGH_MODE2]; NTAB2] = [
    [38, 37, 17, 12, -6, 21],
    [27, 24, 17, 14, 14, 14],
    [18, 16, 16, 16, 17, 16],
    [30, 36, 25, 21, 20, 18],
    [36, 22, 18, 19, 21, 28],
    [-22, -19, -21, -22, -20, -21],
    [-4, -3, -3, -4, -2, -2],
    [15, 12, 11, 10, 11, 11],
    [34, 11, 22, 24, 19, 25],
    [28, 20, 29, 32, 25, 26],
    [16, 8, 0, -1, 2, 5],
    [6, 7, 5, 3, 5, 4],
    [42, 31, 18, 18, 17, 24],
    [25, 25, 2, 4, 0, 4],
    [24, 23, 23, 21, 22, 21],
    [33, 30, 27, 25, 28, 27],
    [60, 53, 32, 29, 48, 52],
    [58, 31, 55, 56, 51, 49],
    [55, 52, 37, 39, 53, 52],
    [52, 46, 50, 49, 52, 49],
    [54, 51, 47, 47, 50, 47],
    [53, 32, 47, 50, 47, 44],
    [46, 43, 47, 52, 54, 48],
    [63, 57, 48, 40, 41, 41],
    [60, 48, 50, 55, 57, 54],
    [55, 54, 48, 43, 38, 36],
    [68, 56, 47, 45, 45, 45],
    [56, 43, 44, 44, 48, 45],
    [55, 54, 47, 35, 45, 50],
    [60, 57, 41, 46, 56, 55],
    [59, 58, 53, 50, 49, 45],
    [50, 49, 47, 44, 45, 40],
    [44, 49, 50, 48, 50, 45],
    [53, 52, 52, 52, 53, 51],
    [52, 56, 59, 60, 62, 58],
    [62, 60, 59, 58, 60, 57],
    [61, 65, 70, 65, 61, 56],
    [68, 65, 64, 62, 63, 58],
    [71, 71, 71, 66, 68, 64],
    [76, 74, 74, 70, 73, 69],
    [83, 82, 82, 76, 78, 73],
    [65, 61, 57, 44, 46, 46],
    [45, 55, 54, 48, 40, 54],
    [59, 50, 46, 45, 45, 41],
    [64, 57, 54, 53, 54, 50],
    [48, 45, 45, 46, 49, 48],
    [56, 61, 59, 55, 53, 52],
    [68, 61, 60, 56, 55, 51],
    [58, 54, 55, 55, 57, 54],
    [50, 54, 55, 48, 46, 42],
    [60, 51, 51, 49, 51, 48],
    [48, 49, 54, 55, 56, 52],
    [38, 39, 46, 41, 28, 22],
    [20, 21, 31, 23, 21, 28],
    [56, 45, 24, 19, 30, 43],
    [48, 34, 23, 26, 38, 43],
    [36, 32, 31, 16, 12, 17],
    [39, 31, 19, 23, 36, 42],
    [47, 19, 24, 19, 42, 50],
    [49, 30, 30, 30, 33, 40],
    [42, 18, 25, 23, 22, 36],
    [39, 15, 22, 29, 23, 28],
    [47, 38, 37, 34, 32, 34],
    [30, 27, 30, 24, 30, 36],
    [49, 38, 14, 21, 21, 37],
    [45, 34, 30, 20, 16, 34],
    [37, 27, 24, 17, 19, 31],
    [47, 27, 19, 27, 24, 34],
    [33, 36, 29, 28, 29, 27],
    [35, 30, 40, 41, 39, 37],
    [46, 42, 29, 23, 38, 45],
    [49, 44, 26, 14, 26, 36],
    [38, 22, 32, 29, 20, 30],
    [41, 32, 27, 27, 23, 23],
    [29, 6, 26, 30, 27, 29],
    [43, 35, 31, 31, 29, 28],
    [39, 26, 12, 14, 15, 26],
    [41, 29, 30, 34, 34, 36],
    [32, 33, 27, 24, 19, 33],
    [39, 14, 16, 20, 16, 26],
    [38, 36, 37, 37, 38, 40],
    [26, 36, 37, 41, 38, 36],
    [29, 29, 21, 18, 15, 24],
    [30, 20, 19, 18, 20, 24],
    [22, 19, 19, 26, 27, 29],
    [32, 39, 34, 36, 26, 25],
    [31, 23, 25, 24, 23, 24],
    [32, 27, 28, 28, 27, 26],
    [35, 25, 29, 31, 30, 33],
    [38, 31, 29, 27, 25, 26],
    [35, 30, 31, 32, 31, 30],
    [41, 32, 34, 32, 32, 31],
    [38, 35, 33, 34, 35, 35],
    [41, 31, 36, 37, 38, 41],
    [43, 34, 37, 39, 35, 33],
    [41, 33, 25, 14, 34, 45],
    [50, 41, 31, 28, 27, 33],
    [30, 26, 30, 36, 37, 37],
    [36, 29, 33, 34, 34, 37],
    [44, 37, 33, 34, 35, 37],
    [36, 30, 33, 38, 43, 44],
    [40, 36, 27, 31, 40, 45],
    [35, 19, 22, 34, 34, 41],
    [27, 35, 28, 32, 20, 13],
    [30, 22, 26, 30, 28, 29],
    [43, 28, 29, 32, 27, 28],
    [28, 27, 35, 31, 25, 30],
    [33, 35, 38, 32, 32, 34],
    [29, 11, 25, 30, 17, 25],
    [24, 23, 22, 23, 20, 22],
    [22, 25, 28, 32, 33, 34],
    [48, 30, 34, 36, 37, 38],
    [27, 31, 23, 29, 30, 33],
    [50, 43, 39, 31, 27, 26],
    [40, 32, 26, 27, 31, 35],
    [50, 33, 32, 33, 29, 31],
    [34, 24, 18, 21, 27, 32],
    [39, 39, 42, 36, 33, 33],
    [30, 15, 13, 26, 29, 35],
    [40, 37, 39, 33, 23, 37],
    [43, 39, 35, 25, 19, 20],
    [37, 43, 34, 34, 32, 33],
    [39, 27, 22, 23, 15, 21],
    [41, 38, 35, 21, 30, 40],
    [36, 28, 25, 26, 24, 25],
    [45, 21, 39, 43, 36, 35],
    [44, 18, 31, 35, 31, 32],
    [37, 15, 34, 39, 31, 33],
];

pub(super) static G_A_TAB1_DP_MODE1: [u8; NTAB1 - 1] = [17, 68];
pub(super) static G_A_TAB1_DP_MODE2: [u8; NTAB1 - 1] = [16, 52];
