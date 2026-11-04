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
//! Tables for the PCM downmixing functions.
use super::types::{ChannelMode, SpZ, SpeakerPosition};

// CAUTION: The maximum x-value should be less or equal to
// PCMDMX_SPKR_POS_X_MAX_WIDTH.
pub(super) static SPKR_SLOT_POS: [SpeakerPosition; 24] = [
    // 0  CENTER_FRONT_CHANNEL
    SpeakerPosition {
        x: 0,
        y: 0,
        z: SpZ::Nrm as i8,
    },
    // 1  LEFT_FRONT_CHANNEL
    SpeakerPosition {
        x: -2,
        y: 0,
        z: SpZ::Nrm as i8,
    },
    // 2  RIGHT_FRONT_CHANNEL
    SpeakerPosition {
        x: 2,
        y: 0,
        z: SpZ::Nrm as i8,
    },
    // 3  LEFT_REAR_CHANNEL
    SpeakerPosition {
        x: -3,
        y: 4,
        z: SpZ::Nrm as i8,
    },
    // 4  RIGHT_REAR_CHANNEL
    SpeakerPosition {
        x: 3,
        y: 4,
        z: SpZ::Nrm as i8,
    },
    // 5  LOW_FREQUENCY_CHANNEL
    SpeakerPosition {
        x: 0,
        y: 0,
        z: SpZ::Lfe as i8,
    },
    // 6  LEFT_MULTIPRPS_CHANNEL
    SpeakerPosition {
        x: -2,
        y: 2,
        z: SpZ::Mul as i8,
    },
    // 7  RIGHT_MULTIPRPS_CHANNEL
    SpeakerPosition {
        x: 2,
        y: 2,
        z: SpZ::Mul as i8,
    },
    // 8  LEFT_SIDE_CHANNEL
    SpeakerPosition {
        x: -3,
        y: 2,
        z: SpZ::Nrm as i8,
    },
    // 9  RIGHT_SIDE_CHANNEL
    SpeakerPosition {
        x: 3,
        y: 2,
        z: SpZ::Nrm as i8,
    },
    // 10 CENTER_REAR_CHANNEL
    SpeakerPosition {
        x: 0,
        y: 4,
        z: SpZ::Nrm as i8,
    },
    // 11 CENTER_FRONT_CHANNEL_TOP
    SpeakerPosition {
        x: 0,
        y: 0,
        z: SpZ::Top as i8,
    },
    // 12 LEFT_FRONT_CHANNEL_TOP
    SpeakerPosition {
        x: -2,
        y: 0,
        z: SpZ::Top as i8,
    },
    // 13 RIGHT_FRONT_CHANNEL_TOP
    SpeakerPosition {
        x: 2,
        y: 0,
        z: SpZ::Top as i8,
    },
    // 14 LEFT_SIDE_CHANNEL_TOP
    SpeakerPosition {
        x: -3,
        y: 2,
        z: SpZ::Top as i8,
    },
    // 15 RIGHT_SIDE_CHANNEL_TOP
    SpeakerPosition {
        x: 3,
        y: 2,
        z: SpZ::Top as i8,
    },
    // 16 CENTER_SIDE_CHANNEL_TOP
    SpeakerPosition {
        x: 0,
        y: 2,
        z: SpZ::Top as i8,
    },
    // 17 LEFT_REAR_CHANNEL_TOP
    SpeakerPosition {
        x: -2,
        y: 4,
        z: SpZ::Top as i8,
    },
    // 18 RIGHT_REAR_CHANNEL_TOP
    SpeakerPosition {
        x: 2,
        y: 4,
        z: SpZ::Top as i8,
    },
    // 19 CENTER_REAR_CHANNEL_TOP
    SpeakerPosition {
        x: 0,
        y: 4,
        z: SpZ::Top as i8,
    },
    // 20 CENTER_FRONT_CHANNEL_BOTTOM
    SpeakerPosition {
        x: 0,
        y: 0,
        z: SpZ::Bot as i8,
    },
    // 21 LEFT_FRONT_CHANNEL_BOTTOM
    SpeakerPosition {
        x: -2,
        y: 0,
        z: SpZ::Bot as i8,
    },
    // 22 RIGHT_FRONT_CHANNEL_BOTTOM
    SpeakerPosition {
        x: 2,
        y: 0,
        z: SpZ::Bot as i8,
    },
    // 23 LOW_FREQUENCY_CHANNEL_2
    SpeakerPosition {
        x: 1,
        y: 0,
        z: SpZ::Lfe as i8,
    },
];

pub(super) static OUT_CH_MODE_TABLE: [ChannelMode; 9] = [
    ChannelMode::Undefined,
    // 1 channel
    ChannelMode::Mode_1_0_0_0,
    // 2 channels
    ChannelMode::Mode_2_0_0_0,
    // 3 channels
    ChannelMode::Mode_3_0_0_0,
    // 4 channels
    ChannelMode::Mode_3_0_1_0,
    // 5 channels
    ChannelMode::Mode_3_0_2_0,
    // 6 channels
    ChannelMode::Mode_3_0_2_1,
    // 7 channels
    ChannelMode::Mode_3_0_3_1,
    // 8 channels
    ChannelMode::Mode_3_0_4_1,
];

pub(super) static AB_MIX_LVL_VALUE_TAB: [f32; 8] = [
    1.0, /* scaled by 0 - No scale in float */
    0.841, 0.707, 0.596, 0.500, 0.422, 0.355, 0.0,
];

pub(super) static LFE_MIX_LVL_VALUE_TAB: [f32; 16] = [
    3.162, 2.0, 1.679, 1.413, 1.189, 1.0, 0.841, 0.707, 0.596, 0.500, 0.316, 0.178, 0.100, 0.032,
    0.010, 0.000,
];

// MPEG matrix mixdown:
// Set 1:  L' = (1 + 2^-0.5 + A )^-1 * [L + C * 2^-0.5 + A * Ls];
//         R' = (1 + 2^-0.5 + A )^-1 * [R + C * 2^-0.5 + A * Rs];

// Set 2:  L' = (1 + 2^-0.5 + 2A )^-1 * [L + C * 2^-0.5 - A * (Ls + Rs)];
//         R' = (1 + 2^-0.5 + 2A )^-1 * [R + C * 2^-0.5 + A * (Ls + Rs)];

// M = (3 + 2A)^-1 * [L + C + R + A*(Ls + Rs)];

pub(super) static MPEG_MIX_DOWN_IDX2_COEF: [f32; 4] = [0.70710678, 0.5, 0.35355339, 0.0];

pub(super) static MPEG_MIX_DOWN_IDX2_PRE_FACT: [[f32; 4]; 3] = [
    [
        0.4142135623730950,
        0.4530818393219728,
        0.4852813742385703,
        0.5857864376269050,
    ],
    [
        0.3203772410170407,
        0.3693980625181293,
        0.4142135623730950,
        0.5857864376269050,
    ],
    [
        0.2265409196609864,
        0.25,
        0.2697521433898179,
        0.3333333333333333,
    ],
];
