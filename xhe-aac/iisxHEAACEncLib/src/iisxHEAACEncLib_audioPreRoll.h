
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

#ifndef XHEAACENCLIB_AUDIOPREROLL_H
#define XHEAACENCLIB_AUDIOPREROLL_H

#include "iisxHEAACEncLib_returnCodes.h"
#include "iisxHEAACEncLib_extentsionData.h"
#include "iisxHEAACEncLib_bitDistribution.h"
#include "iisxHEAACEncLib_syncframe.h"
#include "iisAudioPrerollLib.h"
#include "iisxHEAACEncLibConfig.h"
#include "iisxHEAACEncLib_aac_ifc.h"

typedef enum {
  XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_NO = 0,
  XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_ACT_FRAME,
  XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_NEXT_FRAME
} XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA;

typedef struct {
  AUDIOPREROLLLIB_INSTANCE_HANDLE hAudioPreRoll;
  int nPreRollAUBufferBytes;
  int nPreRollAUBufferBits;
  unsigned char *PreRollAUBuffer;
  unsigned int PreRollAUBufferSizeInBytes;
  XHEAACENCLIB_APR_BITRESMODE bitResMode;
  XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA transmitPayload;
  int bFirstIpfSend;
} XHEAACENCLIB_AUDIOPREROLL_DATA;

XHEAACENCLIB_RETURN iisxHEAACEncLib_getIpfState(
    XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAAC_AACENC_IPF_STATE *const ipfState);

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_configure(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll,
    int const mpeg4StandDelay,
    unsigned int *const minOutBufSize);

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_request(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_AUDIOPREROLL_DATA const *const audioPreRoll,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    int *const lastRapFrameDist,
    int *const bForceIndepFlag,
    XHEAAC_AACENC_IPF_STATE *const ipfState);

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_writeExtPayload(
    XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll,
    XHEAACENCLIB_BD_DATA *const bitDistribution,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_EXT_ELEMENT_LIST *const extEleList);

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_postEncode(
    XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll);
#endif
