
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#include <string.h>

#include "iisutillib.h"
#include "iisSigMapCommon.h"

static const int mapSigMapCicpToTableEntry[][2] = {

    {SIGMAP_CICP_1, 0},
    {SIGMAP_CICP_2, 1},
    {SIGMAP_CICP_3, 2},
    {SIGMAP_CICP_4, 3},
    {SIGMAP_CICP_5, 4},
    {SIGMAP_CICP_6, 5},
    {SIGMAP_CUSTOMIZED_3_OVER_5_1, 5},
    {SIGMAP_CUSTOMIZED_4_OVER_5_1, 5},
    {SIGMAP_CICP_7, 6},
    {SIGMAP_CICP_8, 7},
    {SIGMAP_CICP_9, 8},
    {SIGMAP_CICP_10, 9},
    {SIGMAP_CICP_11, 10},
    {SIGMAP_CICP_12, 11},
    {SIGMAP_CUSTOMIZED_7_1_SIDE, 11},
    {SIGMAP_CICP_13, 12},
    {SIGMAP_CICP_14, 13},
    {SIGMAP_CICP_15, 14},
    {SIGMAP_CICP_16, 15},
    {SIGMAP_CICP_17, 16},
    {SIGMAP_CICP_18, 17},
    {SIGMAP_CICP_19, 18},
    {SIGMAP_CICP_20, 19},
    {SIGMAP_INVALID, -1}};

static const int mapSigMapCicpToNumberOfChannels[][2] = {

    {SIGMAP_CICP_1, 1},
    {SIGMAP_CICP_2, 2},
    {SIGMAP_CICP_3, 3},
    {SIGMAP_CICP_4, 4},
    {SIGMAP_CICP_5, 5},
    {SIGMAP_CICP_6, 6},
    {SIGMAP_CUSTOMIZED_3_OVER_5_1, 6},
    {SIGMAP_CUSTOMIZED_4_OVER_5_1, 6},
    {SIGMAP_CICP_7, 8},
    {SIGMAP_CICP_8, 2},
    {SIGMAP_CICP_9, 3},
    {SIGMAP_CICP_10, 4},
    {SIGMAP_CICP_11, 7},
    {SIGMAP_CICP_12, 8},
    {SIGMAP_CUSTOMIZED_7_1_SIDE, 8},
    {SIGMAP_CICP_13, 24},
    {SIGMAP_CICP_14, 8},
    {SIGMAP_CICP_15, 12},
    {SIGMAP_CICP_16, 10},
    {SIGMAP_CICP_17, 12},
    {SIGMAP_CICP_18, 14},
    {SIGMAP_CICP_19, 12},
    {SIGMAP_CICP_20, 14},
    {SIGMAP_INVALID, -1}};

static const int channelTypesCicp[][24] = {

    {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static const SIGMAP_ELEMENT_TYPE defaultElementsCicp[][16] = {

    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID},
    {SIGMAP_ELEMENT_SCE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_LFE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_CPE, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID, SIGMAP_ELEMENT_INVALID}};

static const int channelIndexMap[][24] = {

    {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 5, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 7, 5, 6, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 6, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 7, 3, 4, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 0, 10, 7, 8, 1, 2, 9, 11, 5, 6, 13, 14, 12, 17, 18, 19, 15, 16, 20, 21, 22, 23},
    {1, 2, 0, 5, 3, 4, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 10, 5, 6, 11, 3, 4, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 5, 3, 4, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 5, 3, 4, 6, 7, 8, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 7, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 0, 7, 5, 6, 3, 4, 8, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 0, 9, 7, 8, 5, 6, 10, 11, 12, 13, 1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

static int iisSigMapGetChannelsInElement(
    SIGMAP_ELEMENT_TYPE elType);

static int iisSigMapGetMpegs212(
    SIGMAP_ELEMENT_TYPE elType);

static SIGMAP_RETURN getCicpTableEntry(
    SIGMAP_INDEX index,
    int* tableEntry);

static SIGMAP_RETURN iisSigMapAddElementToChannelMap(
    SIGMAP_ELEMENT_TYPE elementType,
    CHANNEL_MAPPING_HANDLE hChMap,
    int* instanceTags,
    int* nChannelsTotal,
    int* nFullChannelsPerElement);

static SIGMAP_RETURN iisSigMapAddChannelIndexes(
    CHANNEL_MAPPING_HANDLE hChMap,
    int nChannels,
    int* nFullChannels);

static SIGMAP_RETURN iisSigMapSetRelativeBits(
    CHANNEL_MAPPING_HANDLE hChMap);

static int iisSigMapGetChannelsInElement(
    SIGMAP_ELEMENT_TYPE elType) {
  int nChannels;

  switch (elType) {
    case SIGMAP_ELEMENT_SCE:
    case SIGMAP_ELEMENT_LFE:
      nChannels = 1;
      break;
    case SIGMAP_ELEMENT_CPE:
    case SIGMAP_ELEMENT_CPE_PS:
    case SIGMAP_ELEMENT_CPE_1:
    case SIGMAP_ELEMENT_CPE_2:
    case SIGMAP_ELEMENT_CPE_3:
      nChannels = 2;
      break;
    case SIGMAP_ELEMENT_DSE:
    case SIGMAP_ELEMENT_DSE_DRC:
    case SIGMAP_ELEMENT_PCE:
      nChannels = 0;
      break;
    default:
      nChannels = -1;
      break;
  }

  return nChannels;
}

static int iisSigMapGetMpegs212(
    SIGMAP_ELEMENT_TYPE elType) {
  int bMpegs212;

  switch (elType) {
    case SIGMAP_ELEMENT_SCE:
    case SIGMAP_ELEMENT_CPE:
      bMpegs212 = 0;
      break;
    case SIGMAP_ELEMENT_CPE_1:
      bMpegs212 = 1;
      break;
    case SIGMAP_ELEMENT_CPE_2:
      bMpegs212 = 2;
      break;
    case SIGMAP_ELEMENT_CPE_3:
      bMpegs212 = 3;
      break;
    default:
      bMpegs212 = -1;
      break;
  }

  return bMpegs212;
}

static SIGMAP_RETURN getCicpTableEntry(
    SIGMAP_INDEX index,
    int* tableEntry) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int i;

  for (i = 0;; i++) {
    if (mapSigMapCicpToTableEntry[i][0] == SIGMAP_INVALID) {
      retCode = SIGMAP_ERROR_INVALID_SETUP;
      break;
    }
    if (mapSigMapCicpToTableEntry[i][0] == index) {
      *tableEntry = mapSigMapCicpToTableEntry[i][1];
      break;
    }
  }

  return retCode;
}

static SIGMAP_RETURN iisSigMapAddElementToChannelMap(
    SIGMAP_ELEMENT_TYPE elementType,
    CHANNEL_MAPPING_HANDLE hChMap,
    int* instanceTags,
    int* nChannelsTotal,
    int* nFullChannelsPerElement) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int nChannels = 0;
  int nFullChannels = 0;
  int tableEntry = -1;
  int bMpegs212;
  int i;

  if (hChMap == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    nChannels = iisSigMapGetCoreChannelsInElement(elementType);
    if (nChannels < 0) {
      retCode = SIGMAP_ERROR_INVALID_SETUP;
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    nFullChannels = iisSigMapGetChannelsInElement(elementType);
    if (nFullChannels < 0) {
      retCode = SIGMAP_ERROR_INVALID_SETUP;
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    retCode = getCicpTableEntry(hChMap->cicpLayoutIndex, &tableEntry);
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    for (i = 0; i < nFullChannels; i++) {
      if ((elementType == SIGMAP_ELEMENT_LFE && channelTypesCicp[tableEntry][*nChannelsTotal + i] != 1) || (elementType != SIGMAP_ELEMENT_LFE && channelTypesCicp[tableEntry][*nChannelsTotal + i] != 0)) {
        retCode = SIGMAP_ERROR_INVALID_SETUP;
      }
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    bMpegs212 = iisSigMapGetMpegs212(elementType);

    if (nChannels == 1) {
      if (elementType != SIGMAP_ELEMENT_LFE) {
        hChMap->elInfo[hChMap->nElements].elType = ID_SCE;
        hChMap->elInfo[hChMap->nElements].instanceTag = instanceTags[0]++;
        hChMap->nEffectiveChannels += nChannels;
      } else {
        hChMap->elInfo[hChMap->nElements].elType = ID_LFE;
        hChMap->elInfo[hChMap->nElements].instanceTag = instanceTags[2]++;
      }
    } else if (nChannels == 2) {
      hChMap->elInfo[hChMap->nElements].elType = ID_CPE;
      hChMap->elInfo[hChMap->nElements].instanceTag = instanceTags[1]++;
      hChMap->nEffectiveChannels += nChannels;
    }

    hChMap->elInfo[hChMap->nElements].nChannelsInEl = nChannels;
    hChMap->elInfo[hChMap->nElements].bMpegs212 = bMpegs212;
    hChMap->elInfo[hChMap->nElements].sigMapType = elementType;
    if (elementType == SIGMAP_ELEMENT_CPE_PS) {
      hChMap->elInfo[hChMap->nElements].isPS = 1;
    }
    nFullChannelsPerElement[hChMap->nElements] = nFullChannels;
    *nChannelsTotal += nFullChannels;
    hChMap->nChannels += nChannels;
    hChMap->nElements += 1;
  }

  return retCode;
}

static SIGMAP_RETURN iisSigMapAddChannelIndexes(
    CHANNEL_MAPPING_HANDLE hChMap,
    int nChannelsTotal,
    int* nFullChannels) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int tableEntry = -1;
  int channelIndex, channel, channelIndexCore, element, channelInElement;
  int channelFound;

  if (hChMap == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    retCode = getCicpTableEntry(hChMap->cicpLayoutIndex, &tableEntry);
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    channelIndexCore = 0;
    for (channelIndex = 0; channelIndex < nChannelsTotal; channelIndex++) {
      channel = 0;
      channelFound = 0;

      for (element = 0; element < hChMap->nElements; element++) {
        if (channelFound == 1) {
          break;
        }
        for (channelInElement = 0; channelInElement < nFullChannels[element]; channelInElement++) {
          if (channel == channelIndexMap[tableEntry][channelIndex]) {
            if (channelInElement < hChMap->elInfo[element].nChannelsInEl) {
              hChMap->elInfo[element].ChannelIndex[channelInElement] = channelIndexCore++;
            }
            channelFound = 1;
            break;
          }
          channel++;
        }
      }
    }
  }

  return retCode;
}

static SIGMAP_RETURN iisSigMapSetRelativeBits(
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int element = 0;
  int nSCE = 0;
  int nCPE = 0;
  int nLFE = 0;
  int nCPE1 = 0;
  int nCPE2 = 0;
  int nCPE3 = 0;
  int nQCE0 = 0;

  float sceFac = 1.0f;
  float cpeFac = 1.7f;
  float lfeFac = 0.11f;
  float cpe1Fac = 1.0f;
  float cpe2Fac = 1.5f;
  float cpe3Fac = 1.5f;
  float qce0Fac = 0.04f;

  float commonFactor = 0.f;

  if (hChMap == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    if (hChMap->nElements == 0) {
      retCode = SIGMAP_ERROR_UNKNOWN;
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    for (element = 0; element < hChMap->nElements; element++) {
      switch (hChMap->elInfo[element].sigMapType) {
        case SIGMAP_ELEMENT_SCE:
          nSCE++;
          break;
        case SIGMAP_ELEMENT_CPE:
        case SIGMAP_ELEMENT_QCE_D:
        case SIGMAP_ELEMENT_QCE_R:
          nCPE++;
          break;
        case SIGMAP_ELEMENT_CPE_1:
          nCPE1++;
          break;
        case SIGMAP_ELEMENT_CPE_2:
          nCPE2++;
          break;
        case SIGMAP_ELEMENT_CPE_3:
          nCPE3++;
          break;
        case SIGMAP_ELEMENT_LFE:
          nLFE++;
          break;
        case SIGMAP_ELEMENT_QCE_0:
          nQCE0++;
          break;
        default:
          break;
      }
    }

    commonFactor = 1.f / ((sceFac * (float)nSCE) + (cpeFac * (float)nCPE) + (cpe1Fac * (float)nCPE1) +
                          (cpe2Fac * (float)nCPE2) + (cpe3Fac * (float)nCPE3) + (lfeFac * (float)nLFE) +
                          (qce0Fac * (float)nQCE0));

    for (element = 0; element < hChMap->nElements; element++) {
      switch (hChMap->elInfo[element].sigMapType) {
        case SIGMAP_ELEMENT_SCE:
          hChMap->elInfo[element].relativeBits = commonFactor * sceFac;
          break;
        case SIGMAP_ELEMENT_CPE:
        case SIGMAP_ELEMENT_QCE_D:
        case SIGMAP_ELEMENT_QCE_R:
          hChMap->elInfo[element].relativeBits = commonFactor * cpeFac;
          break;
        case SIGMAP_ELEMENT_CPE_1:
          hChMap->elInfo[element].relativeBits = commonFactor * cpe1Fac;
          break;
        case SIGMAP_ELEMENT_CPE_2:
          hChMap->elInfo[element].relativeBits = commonFactor * cpe2Fac;
          break;
        case SIGMAP_ELEMENT_CPE_3:
          hChMap->elInfo[element].relativeBits = commonFactor * cpe3Fac;
          break;
        case SIGMAP_ELEMENT_LFE:
          hChMap->elInfo[element].relativeBits = commonFactor * lfeFac;
          break;
        case SIGMAP_ELEMENT_QCE_0:
          hChMap->elInfo[element].relativeBits = commonFactor * qce0Fac;
          break;
        default:
          hChMap->elInfo[element].relativeBits = 0.0f;
          break;
      }
    }
  }

  return retCode;
}

int iisSigMapGetCoreChannelsInElement(
    SIGMAP_ELEMENT_TYPE elType) {
  int nChannels;

  switch (elType) {
    case SIGMAP_ELEMENT_SCE:
    case SIGMAP_ELEMENT_CPE_PS:
    case SIGMAP_ELEMENT_CPE_1:
    case SIGMAP_ELEMENT_LFE:
      nChannels = 1;
      break;
    case SIGMAP_ELEMENT_CPE:
    case SIGMAP_ELEMENT_CPE_2:
    case SIGMAP_ELEMENT_CPE_3:
      nChannels = 2;
      break;
    case SIGMAP_ELEMENT_DSE:
    case SIGMAP_ELEMENT_DSE_DRC:
    case SIGMAP_ELEMENT_PCE:
      nChannels = 0;
      break;
    default:
      nChannels = -1;
      break;
  }

  return nChannels;
}

SIGMAP_RETURN iisSigMapSetMonoStereoMode(
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int i;

  if (hChMap == NULL) {
    return SIGMAP_ERROR_INVALID_HANDLE;
  }

  hChMap->monoStereoMode = -1;

  if (hChMap->nChannels > 0) {
    hChMap->monoStereoMode = 0;
  }

  for (i = 0; i < hChMap->nElements; i++) {
    if (hChMap->elInfo[i].elType == ID_CPE) {
      hChMap->monoStereoMode = 1;
      break;
    }
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapCommonAddElementToSetup(
    SIGMAP_SETUP_HANDLE hSetup,
    SIGMAP_ELEMENT_TYPE elementType,
    float relativeBits,
    int sigGroupID) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  SIGMAP_OBJECT_LIST_HANDLE hObjects = NULL;
  SIGMAP_ELEMENT_HANDLE hElements = NULL;

  if (hSetup != NULL) {
    if (hSetup->hObjects == NULL) {
      hSetup->hObjects = iisMalloc(sizeof(SIGMAP_OBJECT_LIST));
      if (hSetup->hObjects == NULL) {
        retCode = SIGMAP_ERROR_MEMORY;
      } else {
        memset(hSetup->hObjects, 0, sizeof(SIGMAP_OBJECT_LIST));
      }
    }

    if (retCode < SIGMAP_ERROR_FIRST) {
      hObjects = hSetup->hObjects;
      hElements = hObjects->hElements;
      if (hElements != NULL) {
        while (hElements->next != NULL) {
          hElements = hElements->next;
        }

        if (retCode < SIGMAP_ERROR_FIRST) {
          hElements->next = iisMalloc(sizeof(SIGMAP_ELEMENT));
          if (hElements->next == NULL) {
            retCode = SIGMAP_ERROR_MEMORY;
          } else {
            hElements = hElements->next;
            memset(hElements, 0, sizeof(SIGMAP_ELEMENT));
          }
        }
      } else {
        hObjects->hElements = iisMalloc(sizeof(SIGMAP_ELEMENT));
        if (hObjects->hElements == NULL) {
          retCode = SIGMAP_ERROR_MEMORY;
        } else {
          hElements = hObjects->hElements;
          memset(hObjects->hElements, 0, sizeof(SIGMAP_ELEMENT));
        }
      }
    }

    if (retCode < SIGMAP_ERROR_FIRST) {
      hElements->sigGroupID = sigGroupID;
      hElements->element = elementType;
      hElements->relativeBits = relativeBits;
      hObjects->nElements++;
    }

  } else {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapMapCicpLayoutIndex(
    SIGMAP_SETUP_HANDLE hSetup,
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int instanceTags[3] = {0};
  int tableEntry, nChannels = 0, nChannelsTotal = 0;
  int nFullChannels[SIGMAP_MAX_ELEMENTS] = {0};

  if (hChMap == NULL || hSetup == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    for (tableEntry = 0; tableEntry < (int)(sizeof(mapSigMapCicpToNumberOfChannels) / (sizeof(int) * 2)); tableEntry++) {
      if (mapSigMapCicpToNumberOfChannels[tableEntry][0] == hSetup->cicpLayoutIndex) {
        hChMap->cicpLayoutIndex = hSetup->cicpLayoutIndex;
        nChannels = mapSigMapCicpToNumberOfChannels[tableEntry][1];
        break;
      }

      if (mapSigMapCicpToNumberOfChannels[tableEntry][0] == SIGMAP_INVALID) {
        hChMap->cicpLayoutIndex = SIGMAP_INVALID;
        nChannels = -1;
        break;
      }
    }

    for (tableEntry = 0; tableEntry < (int)(sizeof(mapSigMapCicpToTableEntry) / (sizeof(int) * 2)); tableEntry++) {
      if (mapSigMapCicpToTableEntry[tableEntry][0] == hSetup->cicpLayoutIndex) {
        tableEntry = mapSigMapCicpToTableEntry[tableEntry][1];
        break;
      }
      if (mapSigMapCicpToTableEntry[tableEntry][0] == SIGMAP_INVALID) {
        tableEntry = -1;
        break;
      }
    }

    while (nChannelsTotal < nChannels && retCode < SIGMAP_ERROR_FIRST) {
      SIGMAP_ELEMENT_TYPE elementType = SIGMAP_ELEMENT_NOT_DEFINED;
      if (hChMap->nElements >= SIGMAP_MAX_ELEMENTS) {
        retCode = SIGMAP_ERROR_INVALID_SETUP;
        break;
      }

      if (hSetup->elementType[hChMap->nElements] == SIGMAP_ELEMENT_NOT_DEFINED && tableEntry >= 0) {
        elementType = defaultElementsCicp[tableEntry][hChMap->nElements];
      } else {
        elementType = hSetup->elementType[hChMap->nElements];
      }
      if (hSetup->type == SIGMAP_TYPE_USAC && (elementType == SIGMAP_ELEMENT_QCE_D ||
                                               elementType == SIGMAP_ELEMENT_QCE_0 || elementType == SIGMAP_ELEMENT_QCE_R)) {
        retCode = SIGMAP_ERROR_INVALID_SETUP;
        break;
      }
      if (hSetup->type == SIGMAP_TYPE_MPEGH && (elementType == SIGMAP_ELEMENT_CPE_PS ||
                                                elementType == SIGMAP_ELEMENT_DSE || elementType == SIGMAP_ELEMENT_DSE_DRC ||
                                                elementType == SIGMAP_ELEMENT_PCE || elementType == SIGMAP_ELEMENT_CCE)) {
        retCode = SIGMAP_ERROR_INVALID_SETUP;
        break;
      }
      retCode = iisSigMapAddElementToChannelMap(elementType,
                                                hChMap,
                                                instanceTags,
                                                &nChannelsTotal,
                                                nFullChannels);
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    retCode = iisSigMapAddChannelIndexes(hChMap, nChannels, nFullChannels);
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    retCode = iisSigMapSetRelativeBits(hChMap);
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapCommonParseSetupElements(
    SIGMAP_SETUP_HANDLE hSetup,
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  SIGMAP_ELEMENT_HANDLE hElement;
  int i;
  int instanceTagSce = 0;
  int instanceTagCpe = 0;
  int instanceTagLfe = 0;
  int instanceTagCce = 0;
  int instanceTagDse = 0;

  if (hSetup == NULL || hChMap == NULL || hSetup->hObjects == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode == SIGMAP_NO_ERROR) {
    if (hSetup->hObjects->nElements == 0) {
      retCode = SIGMAP_WARNING_NO_SETUP_IN_OBJECTS;
    }
  }

  if (retCode == SIGMAP_NO_ERROR) {
    hElement = hSetup->hObjects->hElements;

    for (i = 0; i < hSetup->hObjects->nElements; i++) {
      if (hElement == NULL) {
        retCode = SIGMAP_ERROR_INVALID_HANDLE;
        break;
      }

      switch (hElement->element) {
        case SIGMAP_ELEMENT_SCE:
        case SIGMAP_ELEMENT_CPE:
        case SIGMAP_ELEMENT_CPE_PS:
        case SIGMAP_ELEMENT_LFE:
        case SIGMAP_ELEMENT_CCE:
          if (hChMap->nChannels > 0) {
            hChMap->cicpLayoutIndex = SIGMAP_INVALID;
          }
          break;
        default:
          break;
      }

      switch (hElement->element) {
        case SIGMAP_ELEMENT_SCE:
          hChMap->elInfo[hChMap->nElements].elType = ID_SCE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagSce++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 1;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_SCE;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 1;
          hChMap->nEffectiveChannels += 1;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CPE:
          hChMap->elInfo[hChMap->nElements].elType = ID_CPE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagCpe++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 2;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[1] = hChMap->nChannels + 1;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_CPE;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 2;
          hChMap->nEffectiveChannels += 2;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CPE_PS:
          hChMap->elInfo[hChMap->nElements].elType = ID_SCE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagSce++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 1;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->elInfo[hChMap->nElements].isPS = 1;
          hChMap->nChannels += 1;
          hChMap->nEffectiveChannels += 1;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CPE_1:
          hChMap->elInfo[hChMap->nElements].elType = ID_SCE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagSce++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 1;
          hChMap->elInfo[hChMap->nElements].bMpegs212 = 1;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_CPE_1;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 1;
          hChMap->nEffectiveChannels += 1;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CPE_2:
          hChMap->elInfo[hChMap->nElements].elType = ID_CPE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagCpe++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 2;
          hChMap->elInfo[hChMap->nElements].bMpegs212 = 2;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[1] = hChMap->nChannels + 1;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_CPE_2;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 2;
          hChMap->nEffectiveChannels += 2;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CPE_3:
          hChMap->elInfo[hChMap->nElements].elType = ID_CPE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagCpe++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 2;
          hChMap->elInfo[hChMap->nElements].bMpegs212 = 3;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[1] = hChMap->nChannels + 1;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_CPE_3;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 2;
          hChMap->nEffectiveChannels += 2;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_LFE:
          hChMap->elInfo[hChMap->nElements].elType = ID_LFE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagLfe++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 1;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->elInfo[hChMap->nElements].sigMapType = SIGMAP_ELEMENT_LFE;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 1;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_CCE:
          hChMap->elInfo[hChMap->nElements].elType = ID_CCE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagCce++;
          hChMap->elInfo[hChMap->nElements].nChannelsInEl = 2;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[0] = hChMap->nChannels;
          hChMap->elInfo[hChMap->nElements].ChannelIndex[1] = hChMap->nChannels + 1;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nChannels += 2;
          hChMap->nEffectiveChannels += 2;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_DSE:
          hChMap->elInfo[hChMap->nElements].elType = ID_DSE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagDse++;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_DSE_DRC:
          hChMap->elInfo[hChMap->nElements].elType = ID_DSE;
          hChMap->elInfo[hChMap->nElements].instanceTag = instanceTagDse++;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->elInfo[hChMap->nElements].isDRC = 1;
          hChMap->nElements += 1;
          break;
        case SIGMAP_ELEMENT_PCE:
          hChMap->elInfo[hChMap->nElements].elType = ID_PCE;
          hChMap->elInfo[hChMap->nElements].relativeBits = hElement->relativeBits;
          hChMap->sigGroupID[hChMap->nElements] = hElement->sigGroupID;
          hChMap->nElements += 1;
          break;
        default:
          break;
      }
      hElement = hElement->next;
    }
  }

  return retCode;
}

void iisSigMapCommonDeleteObjectsFromSetup(
    SIGMAP_SETUP_HANDLE hSetup) {
  if (hSetup != NULL && hSetup->hObjects != NULL) {
    SIGMAP_LOUDSPEAKER_HANDLE hLoudspeaker = hSetup->hObjects->hLoudspeakers;
    SIGMAP_ELEMENT_HANDLE hElement = hSetup->hObjects->hElements;
    SIGMAP_SIGNAL_HANDLE hSignal = hSetup->hObjects->hSignals;

    while (hLoudspeaker != NULL) {
      SIGMAP_LOUDSPEAKER_HANDLE next = hLoudspeaker->next;
      iisFree(hLoudspeaker);
      hLoudspeaker = next;
    }

    while (hElement != NULL) {
      SIGMAP_ELEMENT_HANDLE next = hElement->next;
      iisFree(hElement);
      hElement = next;
    }

    while (hSignal != NULL) {
      SIGMAP_SIGNAL_HANDLE next = hSignal->next;
      iisFree(hSignal);
      hSignal = next;
    }
    iisFree(hSetup->hObjects);
    hSetup->hObjects = NULL;
  }
}
