
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

#ifndef IISXHEAACENCLIB_ENCODEFRAME_IFC_H
#define IISXHEAACENCLIB_ENCODEFRAME_IFC_H

#include "iisutillib.h"
#include "resamplelib.h"
#include "time_buffer.h"
#include "extpayload.h"
#include "iisSwitchingDecision.h"
#include "iisAudioPrerollLib.h"

#include "iisxHEAACEncLib_aac_ifc.h"
#include "iisxHEAACEncLib_audioPreRoll.h"
#include "iisxHEAACEncLib_bitDistribution.h"
#include "iisxHEAACEncLib_common.h"
#include "iisxHEAACEncLib_delayAndBuffer.h"
#include "iisxHEAACEncLib_drc_ifc.h"
#include "iisxHEAACEncLib_extentsionData.h"
#include "iisxHEAACEncLib_mpegs_ifc.h"
#include "iisxHEAACEncLib_rapOnDemand.h"
#include "iisxHEAACEncLib_returnCodes.h"
#include "iisxHEAACEncLib_sbr_ifc.h"
#include "iisxHEAACEncLib_syncframe.h"
#include "iisxHEAACEncLib_updateParamList.h"
#include "iisxHEAACEncLibConfig.h"

#define MAX_SBR_PAYLOADS_PER_FRAME 20

typedef struct {
  unsigned char pAncDrcBitstream[1024];
  unsigned int nAncDrcBits;
} ENCODEFRAME_BITSTREAMS;

typedef struct {
  unsigned char* pDynamicDataExtension;
  int pDynamicDataExtensionLength;
} DYNAMIC_DATA_EXTENSION;

typedef struct {
  unsigned char* pPreRollAUBuffer;
  int* pPreRollAUBufferBits;
} ENCODEFRAME_PREROLLAUBUFFER;

typedef struct {
  HANDLE_EXTPAYLOAD_CONTAINER hExtPayload[MAX_SBR_PAYLOADS_PER_FRAME];
  int nPayloads;
} XHEAACENCLIB_EXTPAYLOAD_LIST;

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_preResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib* const hPreResampler,
    TIME_SIGNAL_DATA* const timeSignal,
    int const nSamples,
    unsigned int* const pSamplesNext,
    unsigned int* const nSamplesPreResampOut);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_feedDrcBuffers(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_MP4TIMEBUF const hCoreDelayBuffer,
    TIME_SIGNAL_DATA* const timeSignal,
    XHEAACENCLIB_DRC_DELAY* const drcDelay,
    unsigned int const nSamplesPreResampOut,
    float** const drcIn);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_sbrSyncframe(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    int* const isSyncFrame);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_sbrbitDistribution(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadSbr[],
    XHEAACENCLIB_BD_DATA* const bitDistribution);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_coreResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib* const hCoreDownSampler,
    TIME_SIGNAL_DATA* const timeSignal,
    float* ppCoreDownSamplerIn,
    unsigned int const nSamplesPreResampOut,
    unsigned int* const pnSamplesCoreDownSampOut,
    float** const ppAacCoreInTmp);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreCollectPointers(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXTPAYLOAD_LIST* const sbrPayloadList,
    XHEAACENCLIB_EXTPAYLOAD_LIST* const mpsPayloadList,
    HANDLE_EXTPAYLOAD_CONTAINER* const extContainer,
    int const mpegsEncExist);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreSwDeci(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_IIS_SWDECI const hSwDeci,
    int coreModeNext[]);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreSAP(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    int coreModeNext[]);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreResetSbr(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXTPAYLOAD_LIST* const sbrPayloadList,
    XHEAACENCLIB_EXTPAYLOAD_LIST* const mpsPayloadList,
    XHEAACENCLIB_BITRESERVOIR_DATA* const bitReservoirData);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCorePostEncode(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_AUDIOPREROLL_DATA* const audioPreRoll,
    XHEAACENCLIB_EXTENDED_BIT_RESERVOIR_PARAMS* const ebrParams,
    XHEAAC_AACENC_IPF_STATE const ipfState);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_drcProcess(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    DYNAMIC_DATA_EXTENSION* const dynamicDataExt,
    float* const drcIn,
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_BD_DATA* const bitDistribution);

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_mps(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadMps[],
    XHEAACENCLIB_BD_DATA* const bitDistribution,
    int coreCoderFrameLength,
    XHEAACENCLIB_SYNCFRAME_HANDLE hUsacIndepFlag,
    int speechFlag,
    float* pPreResamplerOut,
    int* nSamplesPreResampOut,
    float** ppQmfSamplesReal,
    float** ppQmfSamplesImag,
    float* pPreResamplerOutLr);
#endif
