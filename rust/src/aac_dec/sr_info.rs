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
//! Sampling rate info

use crate::aac_dec::{constants, error_codes::AacDecoderError};

// 41 scfbands
static SFB_96_1024: [u16; 42] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132,
    144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024,
];

// 12 scfbands
static SFB_96_128: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128];

// 47 scfbands
static SFB_64_1024: [u16; 48] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 112, 124, 140,
    156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784,
    824, 864, 904, 944, 984, 1024,
];

// 12 scfbands
static SFB_64_128: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128];

// 49 scfbands
static SFB_48_1024: [u16; 50] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768, 800, 832, 864, 896, 928, 1024,
];

// 14 scfbands
static SFB_48_128: [u16; 15] = [0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128];

// 51 scfbands
static SFB_32_1024: [u16; 52] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768, 800, 832, 864, 896, 928, 960, 992, 1024,
];

// 47 scfbands
static SFB_24_1024: [u16; 48] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 124, 136,
    148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652,
    704, 768, 832, 896, 960, 1024,
];

// 15 scfbands
static SFB_24_128: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128,
];

// 43 scfbands
static SFB_16_1024: [u16; 44] = [
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 172, 184, 196, 212,
    228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832,
    896, 960, 1024,
];

// 15 scfbands
static SFB_16_128: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128,
];

// 40 scfbands
static SFB_8_1024: [u16; 41] = [
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 236, 252, 268,
    288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944,
    1024,
];

// 15 scfbands
static SFB_8_128: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128,
];

// 40 scfbands
static SFB_96_960: [u16; 41] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132,
    144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960,
];

// 12 scfbands
static SFB_96_120: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 120];

// 46 scfbands
static SFB_64_960: [u16; 47] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 112, 124, 140,
    156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784,
    824, 864, 904, 944, 960,
];

// 12 scfbands
static SFB_64_120: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 120];

// 49 scfbands
static SFB_48_960: [u16; 50] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768, 800, 832, 864, 896, 928, 960,
];

// 14 scfbands
static SFB_48_120: [u16; 15] = [0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 120];

// 49 scfbands
static SFB_32_960: [u16; 50] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768, 800, 832, 864, 896, 928, 960,
];

// 46 scfbands
static SFB_24_960: [u16; 47] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 124, 136,
    148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652,
    704, 768, 832, 896, 960,
];

// 15 scfbands
static SFB_24_120: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 120,
];

// 42 scfbands
static SFB_16_960: [u16; 43] = [
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 172, 184, 196, 212,
    228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832,
    896, 960,
];

// 15 scfbands
static SFB_16_120: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 120,
];

// 40 scfbands
static SFB_8_960: [u16; 41] = [
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 236, 252, 268,
    288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944,
    960,
];

// 15 scfbands
static SFB_8_120: [u16; 16] = [
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 120,
];

// 37 scfbands
static SFB_96_768: [u16; 38] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132,
    144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 640, 704, 768,
];

// 12 scfbands
static SFB_96_96: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 96];

// 41 scfbands
static SFB_64_768: [u16; 42] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 112, 124, 140,
    156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 768,
];

// 12 scfbands
static SFB_64_96: [u16; 13] = [0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 96];

// 43 scfbands
static SFB_48_768: [u16; 44] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768,
];

// 12 scfbands
static SFB_48_96: [u16; 13] = [0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96];

// 43 scfbands
static SFB_32_768: [u16; 44] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160,
    176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
    736, 768,
];

// 43 scfbands
static SFB_24_768: [u16; 44] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 124, 136,
    148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652,
    704, 768,
];

// 14 scfbands
static SFB_24_96: [u16; 15] = [0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 96];

// 39 scfbands
static SFB_16_768: [u16; 40] = [
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 172, 184, 196, 212,
    228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 768,
];

// 14 scfbands
static SFB_16_96: [u16; 15] = [0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 96];

// 37 scfbands
static SFB_8_768: [u16; 38] = [
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 236, 252, 268,
    288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 620, 664, 712, 764, 768,
];

// 14 scfbands
static SFB_8_96: [u16; 15] = [0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 96];

// 36 scfbands
static SFB_48_512: [u16; 37] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 68, 76, 84, 92, 100, 112, 124,
    136, 148, 164, 184, 208, 236, 268, 300, 332, 364, 396, 428, 460, 512,
];

// 37 scfbands
static SFB_32_512: [u16; 38] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132,
    144, 160, 176, 192, 212, 236, 260, 288, 320, 352, 384, 416, 448, 480, 512,
];

// 31 scfbands
static SFB_24_512: [u16; 32] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 80, 92, 104, 120, 140, 164, 192, 224,
    256, 288, 320, 352, 384, 416, 448, 480, 512,
];

// 35 scfbands
static SFB_48_480: [u16; 36] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132,
    144, 156, 172, 188, 212, 240, 272, 304, 336, 368, 400, 432, 480,
];

// 37 scfbands
static SFB_32_480: [u16; 38] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 72, 80, 88, 96, 104, 112, 124,
    136, 148, 164, 180, 200, 224, 256, 288, 320, 352, 384, 416, 448, 480,
];

// 30 scfbands
static SFB_24_480: [u16; 31] = [
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 80, 92, 104, 120, 140, 164, 192, 224,
    256, 288, 320, 352, 384, 416, 448, 480,
];

#[repr(C)]
#[derive(Debug)]
struct SfbInfo {
    offset_long: Option<&'static [u16]>,
    offset_short: Option<&'static [u16]>,
}

static SFB_OFFSET_TABLES: [[SfbInfo; 13]; 5] = [
    [
        SfbInfo {
            offset_long: Some(&SFB_96_1024),
            offset_short: Some(&SFB_96_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_96_1024),
            offset_short: Some(&SFB_96_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_64_1024),
            offset_short: Some(&SFB_64_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_1024),
            offset_short: Some(&SFB_48_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_1024),
            offset_short: Some(&SFB_48_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_32_1024),
            offset_short: Some(&SFB_48_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_1024),
            offset_short: Some(&SFB_24_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_1024),
            offset_short: Some(&SFB_24_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_1024),
            offset_short: Some(&SFB_16_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_1024),
            offset_short: Some(&SFB_16_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_1024),
            offset_short: Some(&SFB_16_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_1024),
            offset_short: Some(&SFB_8_128),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_1024),
            offset_short: Some(&SFB_8_128),
        },
    ],
    [
        SfbInfo {
            offset_long: Some(&SFB_96_960),
            offset_short: Some(&SFB_96_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_96_960),
            offset_short: Some(&SFB_96_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_64_960),
            offset_short: Some(&SFB_64_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_960),
            offset_short: Some(&SFB_48_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_960),
            offset_short: Some(&SFB_48_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_32_960),
            offset_short: Some(&SFB_48_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_960),
            offset_short: Some(&SFB_24_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_960),
            offset_short: Some(&SFB_24_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_960),
            offset_short: Some(&SFB_16_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_960),
            offset_short: Some(&SFB_16_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_960),
            offset_short: Some(&SFB_16_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_960),
            offset_short: Some(&SFB_8_120),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_960),
            offset_short: Some(&SFB_8_120),
        },
    ],
    [
        SfbInfo {
            offset_long: Some(&SFB_96_768),
            offset_short: Some(&SFB_96_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_96_768),
            offset_short: Some(&SFB_96_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_64_768),
            offset_short: Some(&SFB_64_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_768),
            offset_short: Some(&SFB_48_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_48_768),
            offset_short: Some(&SFB_48_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_32_768),
            offset_short: Some(&SFB_48_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_768),
            offset_short: Some(&SFB_24_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_24_768),
            offset_short: Some(&SFB_24_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_768),
            offset_short: Some(&SFB_16_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_768),
            offset_short: Some(&SFB_16_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_16_768),
            offset_short: Some(&SFB_16_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_768),
            offset_short: Some(&SFB_8_96),
        },
        SfbInfo {
            offset_long: Some(&SFB_8_768),
            offset_short: Some(&SFB_8_96),
        },
    ],
    [
        SfbInfo {
            offset_long: Some(&SFB_48_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_32_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_512),
            offset_short: None,
        },
    ],
    [
        SfbInfo {
            offset_long: Some(&SFB_48_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_48_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_32_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
        SfbInfo {
            offset_long: Some(&SFB_24_480),
            offset_short: None,
        },
    ],
];

/// SamplingRateInfo
///
/// # Examples
///
/// Get an instance of SamplingRateInfo
///
/// ```
/// use aac::aac_dec::sr_info::SamplingRateInfo;
///
/// let mut sr_info = SamplingRateInfo::new();
/// sr_info.init(1024, 3, 48000);
/// ```
#[derive(Default, Debug, Copy, Clone)]
#[repr(C)]
pub struct SamplingRateInfo {
    scale_factor_bands_long: Option<&'static [u16]>,
    scale_factor_bands_short: Option<&'static [u16]>,
    sampling_rate_index: usize,
    sampling_rate: u32,
}

impl SamplingRateInfo {
    /// Create a SamplingRateInfo instance
    pub fn new() -> SamplingRateInfo {
        Default::default()
    }

    /// Initialize SamplingRateInfo instance
    pub fn init(
        &mut self,
        samples_per_frame: u16,
        mut sampling_rate_index: usize,
        sampling_rate: u32,
    ) -> Result<(), AacDecoderError> {
        if sampling_rate > constants::MAX_SAMPLERATE {
            return Err(AacDecoderError::UnsupportedSamplingrate);
        }

        // Search closest samplerate according to ISO/IEC 13818-7:2005(E) 8.2.4 (Table 38):
        if sampling_rate_index >= 15 || samples_per_frame == 768 {
            const BORDERS: [u32; 12] = [
                u32::MAX,
                92017,
                75132,
                55426,
                46009,
                37566,
                27713,
                23004,
                18783,
                13856,
                11502,
                9391,
            ];

            let mut sampling_rate_search = sampling_rate;
            if samples_per_frame == 768 {
                sampling_rate_search = (sampling_rate * 4) / 3;
            }

            sampling_rate_index = BORDERS
                .windows(2)
                .take_while(|w| !(w[0] > sampling_rate_search && sampling_rate_search >= w[1]))
                .count();
        }
        self.sampling_rate_index = sampling_rate_index;
        self.sampling_rate = sampling_rate;

        let index = match samples_per_frame {
            1024 => 0,
            960 => 1,
            768 => 2,
            512 => 3,
            480 => 4,
            _ => return Err(AacDecoderError::UnsupportedFormat),
        };

        if sampling_rate_index >= SFB_OFFSET_TABLES[index].len() {
            self.sampling_rate = 0;
            return Err(AacDecoderError::UnsupportedFormat);
        }

        self.scale_factor_bands_long = SFB_OFFSET_TABLES[index][sampling_rate_index].offset_long;
        self.scale_factor_bands_short = SFB_OFFSET_TABLES[index][sampling_rate_index].offset_short;

        assert!(self.scale_factor_bands_long.unwrap().last().unwrap() == &samples_per_frame);
        assert!(
            self.scale_factor_bands_short.is_none()
                || self.scale_factor_bands_short.unwrap().last().unwrap()
                    * constants::MAX_WINDOWS as u16
                    == samples_per_frame
        );

        Ok(())
    }

    /// Get number of total scale factor bands for long block.
    pub fn n_scale_factor_bands_long(&self) -> usize {
        self.scale_factor_bands_long.unwrap().len() - 1
    }

    /// Get number of total scale factor bands for short blocks.
    pub fn n_scale_factor_bands_short(&self) -> usize {
        self.scale_factor_bands_short.unwrap().len() - 1
    }

    /// Get scale factor bands for long block.
    pub fn scale_factor_bands_long(&self) -> Option<&'static [u16]> {
        self.scale_factor_bands_long
    }

    /// Get scale factor bands for short blocks.
    pub fn scale_factor_bands_short(&self) -> Option<&'static [u16]> {
        self.scale_factor_bands_short
    }

    /// Get sampling rate
    pub fn sampling_rate(&self) -> u32 {
        self.sampling_rate
    }
    /// Get sampling rate index value
    pub fn sampling_rate_index(&self) -> usize {
        self.sampling_rate_index
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn check_init() {
        let mut sr_info = SamplingRateInfo::new();

        assert!(Ok(()) == sr_info.init(1024, 15, 80000));
        assert_eq!(sr_info.sampling_rate_index, 1);

        assert!(Ok(()) == sr_info.init(1024, 15, 10000));
        assert_eq!(sr_info.sampling_rate_index, 10);

        assert!(Ok(()) == sr_info.init(1024, 15, 8500));
        assert_eq!(sr_info.sampling_rate_index, 11);

        assert!(Ok(()) == sr_info.init(1024, 15, 1000));
        assert_eq!(sr_info.sampling_rate_index, 11);

        assert!(Ok(()) == sr_info.init(1024, 4, 44100));
        assert!(49 == sr_info.scale_factor_bands_long().unwrap().len() - 1);
        assert!(49 == sr_info.n_scale_factor_bands_long());
        assert!(14 == sr_info.scale_factor_bands_short().unwrap().len() - 1);
        assert!(14 == sr_info.n_scale_factor_bands_short());

        assert!(Err(AacDecoderError::UnsupportedFormat) == sr_info.init(1023, 4, 44100));

        assert!(Err(AacDecoderError::UnsupportedFormat) == sr_info.init(1024, 13, 44100));

        assert!(
            Err(AacDecoderError::UnsupportedSamplingrate)
                == sr_info.init(1024, 4, constants::MAX_SAMPLERATE + 1)
        );
        assert!(Err(AacDecoderError::UnsupportedSamplingrate) == sr_info.init(1024, 4, u32::MAX));
    }
}
