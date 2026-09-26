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
//! Common structures and enums used for `DrcGainDecoder`, like DRC compression characteristics
//! sets.
use super::super::reader::{CustomDrcCharNodes, CustomDrcCharSigmoid};

#[rustfmt::skip]
pub(super) static CICP_DRC_CHAR_SIGMOID_LEFT: [CustomDrcCharSigmoid; 6] = [
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 0.0, exp: 9.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 0.2, exp: 9.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 0.4, exp: 9.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 0.6, exp: 9.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 0.8, exp: 6.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: 32.0, io_ratio: 1.0, exp: 5.0, is_flip_sign: false},
];

#[rustfmt::skip]
pub(super) static CICP_DRC_CHAR_SIGMOID_RIGHT: [CustomDrcCharSigmoid; 6] = [
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 0.0, exp: 12.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 0.2, exp: 12.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 0.4, exp: 12.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 0.6, exp: 10.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 0.8, exp:  8.0, is_flip_sign: false},
    CustomDrcCharSigmoid { gain: -32.0, io_ratio: 1.0, exp:  6.0, is_flip_sign: false},
];

#[rustfmt::skip]
pub(super) static CICP_DRC_CHAR_NODES_LEFT: [CustomDrcCharNodes; 5] = [
    CustomDrcCharNodes {
        characteristic_node_count: 2,
        node_level: [-31.0, -41.0, -53.0, 0.0, 0.0],
        node_gain:  [  0.0,   0.0,   6.0, 0.0, 0.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 1,
        node_level: [-31.0, -43.0, 0.0, 0.0, 0.0],
        node_gain:  [  0.0,   6.0, 0.0, 0.0, 0.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 2,
        node_level: [-31.0, -41.0, -65.0, 0.0, 0.0],
        node_gain:  [  0.0,   0.0,  12.0, 0.0, 0.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 1,
        node_level: [-31.0, -55.0, 0.0, 0.0, 0.0],
        node_gain:  [  0.0,  12.0, 0.0, 0.0, 0.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 1,
        node_level: [-31.0, -50.0, 0.0, 0.0, 0.0],
        node_gain:  [  0.0,  15.0, 0.0, 0.0, 0.0],
    },
];

#[rustfmt::skip]
pub(super) static CICP_DRC_CHAR_NODES_RIGHT: [CustomDrcCharNodes; 5] = [
    CustomDrcCharNodes {
        characteristic_node_count: 4,
        node_level: [-31.0, -21.0, -11.0,   9.0,  19.0],
        node_gain:  [  0.0,   0.0,  -5.0, -24.0, -34.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 4,
        node_level: [-31.0, -26.0, -16.0,   4.0,  14.0],
        node_gain:  [  0.0,   0.0,  -5.0, -24.0, -34.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 3,
        node_level: [-31.0, -21.0,   9.0,  29.0, 0.0],
        node_gain:  [  0.0,   0.0, -15.0, -35.0, 0.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 4,
        node_level: [-31.0, -26.0, -16.0,   4.0,  14.0],
        node_gain:  [  0.0,   0.0,  -5.0, -24.0, -34.0],
    },
    CustomDrcCharNodes {
        characteristic_node_count: 4,
        node_level: [-31.0, -26.0, -16.0,   4.0,  14.0],
        node_gain:  [  0.0,   0.0,  -5.0, -24.0, -34.0],
    },
];
