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
//! Channel map descriptor tables.

// The following arrays provide a channel map for each channel config (0
// to 14).
//
// The i-th channel will be mapped to the postion a[i-1]+1
// with i>0 and a[] is one of the following mapping arrays.

use crate::common::channel_map_descr::ChannelMapInfo;

static MAP_FALLBACK: &[u8] = &[
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
];
static MAP_CFG_1: &[u8] = &[0, 1];
static MAP_CFG_2: &[u8] = &[0, 1];
static MAP_CFG_3: &[u8] = &[2, 0, 1];
static MAP_CFG_4: &[u8] = &[2, 0, 1, 3];
static MAP_CFG_5: &[u8] = &[2, 0, 1, 3, 4];
static MAP_CFG_6: &[u8] = &[2, 0, 1, 4, 5, 3];
static MAP_CFG_7: &[u8] = &[2, 6, 7, 0, 1, 4, 5, 3];
static MAP_CFG_11: &[u8] = &[2, 0, 1, 4, 5, 6, 3];
static MAP_CFG_12: &[u8] = &[2, 0, 1, 6, 7, 4, 5, 3];
static MAP_CFG_13: &[u8] = &[
    2, 6, 7, 0, 1, 10, 11, 4, 5, 8, 3, 9, 14, 12, 13, 18, 19, 15, 16, 17, 20, 21, 22, 23,
];
static MAP_CFG_14: &[u8] = &[2, 0, 1, 4, 5, 3, 6, 7];

// CICP channel order is different than WAV channel order for the following
//chConfings:

static MAP_CFG_2_CICP_7: &[u8] = &[2, 0, 1, 6, 7, 4, 5, 3];
static MAP_CFG_2_CICP_12: &[u8] = &[2, 0, 1, 4, 5, 6, 7, 3];
static MAP_CFG_2_CICP_14: &[u8] = &[2, 0, 1, 4, 5, 3, 6, 7];

// Default table comprising channel map information for each channel
// config (0 to 14).

#[rustfmt::skip]
pub static MAP_INFO_TAB_DFLT: &[ChannelMapInfo] = &[
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 0
    ChannelMapInfo { channel_map: Some(MAP_CFG_1)  }, // ch_cfg: 1
    ChannelMapInfo { channel_map: Some(MAP_CFG_2) }, // ch_cfg: 2
    ChannelMapInfo { channel_map: Some(MAP_CFG_3) }, // ch_cfg: 3
    ChannelMapInfo { channel_map: Some(MAP_CFG_4) }, // ch_cfg: 4
    ChannelMapInfo { channel_map: Some(MAP_CFG_5) }, // ch_cfg: 5
    ChannelMapInfo { channel_map: Some(MAP_CFG_6) }, // ch_cfg: 6
    ChannelMapInfo { channel_map: Some(MAP_CFG_7) }, // ch_cfg: 7
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 8
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 9
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 10
    ChannelMapInfo { channel_map: Some(MAP_CFG_11) }, // ch_cfg: 11
    ChannelMapInfo { channel_map: Some(MAP_CFG_12) }, // ch_cfg: 12
    ChannelMapInfo { channel_map: Some(MAP_CFG_13) }, // ch_cfg: 13
    ChannelMapInfo { channel_map: Some(MAP_CFG_14) }, // ch_cfg: 14
];

#[rustfmt::skip]
pub static MAP_INFO_TAB_CICP: &[ChannelMapInfo] = &[
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 0
    ChannelMapInfo { channel_map: Some(MAP_CFG_1) }, // ch_cfg: 1
    ChannelMapInfo { channel_map: Some(MAP_CFG_2) }, // ch_cfg: 2
    ChannelMapInfo { channel_map: Some(MAP_CFG_3) }, // ch_cfg: 3
    ChannelMapInfo { channel_map: Some(MAP_CFG_4) }, // ch_cfg: 4
    ChannelMapInfo { channel_map: Some(MAP_CFG_5) }, // ch_cfg: 5
    ChannelMapInfo { channel_map: Some(MAP_CFG_6) }, // ch_cfg: 6
    ChannelMapInfo { channel_map: Some(MAP_CFG_2_CICP_7) }, // ch_cfg: 7
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 8
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 9
    ChannelMapInfo { channel_map: Some(MAP_FALLBACK) }, // ch_cfg: 10
    ChannelMapInfo { channel_map: Some(MAP_CFG_11) }, // ch_cfg: 11
    ChannelMapInfo { channel_map: Some(MAP_CFG_2_CICP_12) }, // ch_cfg: 12
    ChannelMapInfo { channel_map: Some(MAP_CFG_13) }, // ch_cfg: 13
    ChannelMapInfo { channel_map: Some(MAP_CFG_2_CICP_14) }, // ch_cfg: 14
];

#[cfg(test)]
static TEST_INVALID_MAP_INFO_EXCEEDED_IDX: &[u8] = &[
    25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
];

#[cfg(test)]
pub static TEST_INVALID_MAP_EXCEEDED_IDX: &[ChannelMapInfo] = &[ChannelMapInfo {
    channel_map: Some(TEST_INVALID_MAP_INFO_EXCEEDED_IDX),
}];

#[cfg(test)]
static TEST_MAP_FALLBACK: &[u8] = &[
    0, 1, 2, 3, 4, 5, 6, 7, 10, 9, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
];
#[cfg(test)]
static TEST_MAP_CFG_1: &[u8] = &[0, 1];

#[cfg(test)]
#[rustfmt::skip]
pub static TEST_VALID_MAP: &[ChannelMapInfo] = &[
    ChannelMapInfo { channel_map: Some(TEST_MAP_FALLBACK) },
    ChannelMapInfo { channel_map: Some(TEST_MAP_CFG_1) },
];

#[cfg(test)]
#[rustfmt::skip]
pub static TEST_VALID_MAP_WITH_NONE: &[ChannelMapInfo] = &[
    ChannelMapInfo { channel_map: Some(TEST_MAP_FALLBACK) },
    ChannelMapInfo { channel_map: Some(TEST_MAP_CFG_1) },
    ChannelMapInfo { channel_map: None },
];
