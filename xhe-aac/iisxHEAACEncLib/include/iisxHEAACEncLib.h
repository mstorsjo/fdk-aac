
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

#ifndef IISXHEAACENCLIB_H
#define IISXHEAACENCLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iisParamList.h"
#include "iisParam.h"

#include "../src/iisxHEAACEncLib_returnCodes.h"
#include "../src/iisxHEAACEncLib_encoderStateEnum.h"

#define XHEAACENCLIB_MAX_ASC_SIZE 128
#define XHEAACENCLIB_MAX_TF_SIZE 64
#define XHEAACENCLIB_DATA_ANCILLARY_FIRST 0x00001000

#if defined(_WIN32)
#define XHEAACENCLIB_API __stdcall
#else
#define XHEAACENCLIB_API
#endif

typedef enum {
  XHEAACENCLIB_SYNCFRAME_INVALID = -1,
  XHEAACENCLIB_SYNCFRAME_NO = 0,
  XHEAACENCLIB_SYNCFRAME_STARTUP = 1,
  XHEAACENCLIB_SYNCFRAME_INDEPENDENT_FRAME = 2,
  XHEAACENCLIB_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME = 3,
  XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IF = 4,
  XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IPF = 5,
  XHEAACENCLIB_SYNCFRAME_HE_AAC_RAP_SWITCHABLE = 6,
  XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_SWITCHABLE = 8,
  XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_ACCESS = 9
} XHEAACENCLIB_SYNCFRAME_TYPES;

typedef enum {
  XHEAACENCLIB_DATA_INVALID = -1,
  XHEAACENCLIB_DATA_RAP_IN_X_SAMPLES = 0,
  XHEAACENCLIB_DATA_ANCILLARY = XHEAACENCLIB_DATA_ANCILLARY_FIRST,
  XHEAACENCLIB_DATA_MPEG4_PROG_REF_LEVEL,
  XHEAACENCLIB_DATA_MPEG4_DRC_PROFILE,
  XHEAACENCLIB_DATA_MPEG4_DRC_PROFILE_HEAVY,
  XHEAACENCLIB_DATA_MPEG4_CENTER_MIX_LEVEL,
  XHEAACENCLIB_DATA_MPEG4_SURROUND_MIX_LEVEL,
  XHEAACENCLIB_DATA_MPEG4_ETSI_DOWNMIX_PRESENT,
  XHEAACENCLIB_DATA_MPEG4_DOLBY_SURROUND_MODE,
  XHEAACENCLIB_DATA_MPEG4_PSEUDO_SUR_DMX_ENABLE,
  XHEAACENCLIB_DATA_MPEG4_METADATA_MODE,
  XHEAACENCLIB_DATA_MPEG4_DRC_EXT_DRC_GAIN,
  XHEAACENCLIB_DATA_MPEG4_DRC_EXT_COMP_GAIN
} XHEAACENCLIB_DATA;

#if defined(_WIN32)
#pragma pack(push, 1)
#endif

typedef void (*XHEAACENCLIB_MESSAGE_CALLBACK_PUBLIC)(char*);

typedef struct xheaacenclib_auinfo_struct {
  struct xheaacenclib_auinfo_struct* pNextAu;
  unsigned int auOffset;
  unsigned int auSize;
  unsigned int auSamplesValid;
  XHEAACENCLIB_SYNCFRAME_TYPES isSyncFrame;
} XHEAACENCLIB_AUINFO, *XHEAACENCLIB_AUINFO_HANDLE;

typedef struct xheaacenclib_auinfolist_struct {
  int nAccessUnits;
  XHEAACENCLIB_AUINFO_HANDLE hAccessUnits;
} XHEAACENCLIB_AUINFOLIST, *XHEAACENCLIB_AUINFOLIST_HANDLE;

typedef struct xheaacenclib_ascinfo_struct {
  unsigned int ascSizeBits;
  unsigned char ascBuffer[XHEAACENCLIB_MAX_ASC_SIZE];
} XHEAACENCLIB_ASCINFO, *XHEAACENCLIB_ASCINFO_HANDLE;

typedef struct xheaacenclib_tfinfo_struct {
  unsigned int tfSizeBits;
  unsigned char tfBuffer[XHEAACENCLIB_MAX_TF_SIZE];
} XHEAACENCLIB_TFINFO, *XHEAACENCLIB_TFINFO_HANDLE;

#if defined(_WIN32)
#pragma pack(pop)
#endif

typedef struct xheaacenclib_instance_struct* XHEAACENCLIB_INSTANCE_HANDLE;

char const* XHEAACENCLIB_API IIS_xHEAACEncLib_GetVersion(void);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_New(
    XHEAACENCLIB_INSTANCE_HANDLE* const phInstance);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Update(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_SubmitDrcMetadata(
    XHEAACENCLIB_INSTANCE_HANDLE const hInstance,
    float const* const drcGains,
    unsigned int const drcGainsLength,
    unsigned int const drcExternalNodeNumNodes,
    int const* const drcExternalNodeLevels,
    int const* const drcExternalNodeGains,
    float const* const drcGainOffset,
    unsigned int const drcGainOffsetLength);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Submit(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_DATA const type,
    PARAM_FORMAT const dataFormat,
    const void* const pData,
    int const dataSize);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetAuInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_AUINFOLIST_HANDLE* phAuInfoList);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetAscInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_ASCINFO_HANDLE hAscInfo);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetEncoderState(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_ENCODER_STATE* encoderState);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_UpdateParamList(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_SetMinAuBytes(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    int minAuBytes);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Encode(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    const float* const pSamples,
    const unsigned int nSamples,
    unsigned char* const pOutput,
    int* const pOutputBytes,
    const unsigned int outputBufSizeBytes);

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetEncodingWarnings(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    int* const pnWarning,
    XHEAACENCLIB_WARNING const** const ppWarnings);

void XHEAACENCLIB_API IIS_xHEAACEncLib_Delete(
    XHEAACENCLIB_INSTANCE_HANDLE* const phInstance);

#ifdef __cplusplus
}
#endif

#endif
