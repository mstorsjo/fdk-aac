
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mathlib.h"
#include "iisLPDEncLib_main.h"
#include "iisLPDEncLib.h"
#include "iisLPDEncLib_arithEncWrapper.h"
#include "iisLPDEncLib_cmConfig.h"
#include "iisLPDComLib_tools.h"
#include "iisLPDEncLib_transitions.h"

#define N_FRAMES_LPD_MAX 4

typedef struct lpd_enc_private_data_struct {
  int size;

  Coder_State_Plus pAcelpEnc;

  int wasFirst;
  int externalPrefilter;

} LPD_ENC_PRIVATE_DATA;

typedef enum lpd_codec_type {
  LPD_CODEC_USAC,
  LPD_CODEC_MPEGH
} LPD_CODEC_TYPE;

typedef enum lpd_tool_config {
  LPD_TOOL_DEFAULT = -1,
  LPD_TOOL_INACTIVE = 0,
  LPD_TOOL_ACTIVE
} LPD_TOOL_CONFIG;

static LPD_ENC_PRIVATE_DATA_HANDLE GetPrivateDataHandle(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  hPrivateData = hPublicData->hPrivateData;

  return hPrivateData;
}

HANDLE_ERROR_INFO iisLPDEncLib_Open(
    LPD_ENC_PUBLIC_DATA_HANDLE* phPublicData,
    const int fullbandLpd,
    const int stereoLpdActive) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int size = 0;

  (void)stereoLpdActive;

  if (phPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer in function iisLPDEncLib_Open");
  }

  size += sizeof(struct lpd_enc_public_data_struct);
  size += sizeof(struct lpd_enc_private_data_struct);

  *phPublicData = (LPD_ENC_PUBLIC_DATA_HANDLE)iisCalloc(1, sizeof(LPD_ENC_PUBLIC_DATA));

  if (*phPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Memory allocation in function iisLPDEncLib_Open failed");
  }

  (*phPublicData)->hPrivateData = (LPD_ENC_PRIVATE_DATA_HANDLE)iisCalloc(1, sizeof(LPD_ENC_PRIVATE_DATA));
  if ((*phPublicData)->hPrivateData == NULL) {
    iisFree(*phPublicData);
    *phPublicData = NULL;
    return iisUtil_ERROR(CDI, "Memory allocation in function iisLPDEncLib_Open failed");
  }
  (*phPublicData)->hPrivateData->size = size;

  hPrivateData = GetPrivateDataHandle(*phPublicData);

  LPDEnc_main_Open(&hPrivateData->pAcelpEnc,
                   fullbandLpd);

  return noError;
}

HANDLE_ERROR_INFO iisLPDEncLib_Config(
    LPD_ENC_PUBLIC_DATA_HANDLE hPublicData,
    const LPDENCLIB_CONFIG* hInitData) {
  HANDLE_ERROR_INFO hError = noError;
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer in function iisLPDEncLib_Config");
  }

  hPrivateData = GetPrivateDataHandle(hPublicData);

  hPrivateData->wasFirst = 1;
  hPrivateData->externalPrefilter = 0;
  hPublicData->totalBitRate = hInitData->totalBitRate;
  hPrivateData->pAcelpEnc.isVbr = hInitData->isVbr;

  hError = LPDEnc_main_Config(&hPrivateData->pAcelpEnc,
                              1,
                              hInitData->L_frame,
                              hInitData->fscale,
                              0,
                              hInitData->optimizedSpeedPulseSearch,
                              hInitData->mode,
                              hInitData->codingMode);

  return hError;
}

LPD_ENC_PUBLIC_DATA_HANDLE iisLPDEncLib_Close(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData) {
  if (hPublicData != NULL) {
    LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

    hPrivateData = GetPrivateDataHandle(hPublicData);
    if (hPrivateData != NULL) {
      LPDEnc_arithEncWrapper_CloseArithEnc(hPrivateData->pAcelpEnc.saveFrameState.phArith);
      hPrivateData->pAcelpEnc.saveFrameState.phArith = LPDEnc_arithEncWrapper_Close(hPrivateData->pAcelpEnc.saveFrameState.phArith);

      LPDEnc_main_Close(&hPrivateData->pAcelpEnc);
      iisFree(hPrivateData);
    }
    iisFree(hPublicData);
  }
  return NULL;
}

HANDLE_ERROR_INFO iisLPDEncLib_EncodeFrame(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    float const* const pTimeInput,
    float const* const pAMRPast,
    int isAceStart,
    int const bUsacIndependencyFlag,
    int const bUsacNoiseFilling,
    ARIENC_PUBLIC_DATA_HANDLE const hArithEnc,
    int maxBitsToUse,
    LPD_IPF_STATE const ipfState) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  float inBuffer[L_DIV_1024 * N_FRAMES_LPD_MAX + L_NEXT_HIGH_RATE_1024];
  float inBuffer_Past[L_DIV_1024 * N_FRAMES_LPD_MAX + L_NEXT_HIGH_RATE_1024 + L_LPC0_1024];
  int const bCommonWindowMode = 0;
  int lFrameAcelp;
  int lNextHighRate;
  HANDLE_ERROR_INFO err = noError;

  if (hPublicData == NULL) {
    return iisUtil_ERROR(CDI, "invalid Pointer");
  }

  hPrivateData = GetPrivateDataHandle(hPublicData);

  LPDEnc_arithEncWrapper_SetArithEncoderInstance(hPrivateData->pAcelpEnc.phArith, hArithEnc);

  lFrameAcelp = (hPrivateData->pAcelpEnc.lFrame) / (hPrivateData->pAcelpEnc.nbDiv);
  lNextHighRate = (int)(L_NEXT_HIGH_RATE_1024 * (float)hPrivateData->pAcelpEnc.lFrame / 1024.f);

  if (isAceStart) {
    LPDEnc_main_Reset(&hPrivateData->pAcelpEnc, hPrivateData->pAcelpEnc.lastWasShort);
    hPublicData->wasReset = 1;
    hPublicData->nBitsFac = 0;

  } else {
    hPublicData->wasReset = 0;
  }

  hPublicData->outputStreamLenBits = 0;

  smulFLOAT((1 << 15), pTimeInput, inBuffer, N_FRAMES_LPD_MAX * lFrameAcelp + lNextHighRate);
  smulFLOAT((1 << 15), pAMRPast, inBuffer_Past, N_FRAMES_LPD_MAX * lFrameAcelp + lNextHighRate + lFrameAcelp);

  if (isAceStart) {
    LPDEnc_main_PrepLABuffer(&inBuffer_Past[0],
                             lNextHighRate + lFrameAcelp,
                             &hPrivateData->pAcelpEnc);
  }

  if (noError == err) {
    err = LPDEnc_main_Encode(
        &inBuffer[lNextHighRate],
        hPublicData->outStream,
        hPublicData->facData,
        &hPublicData->nBitsFac,
        &hPrivateData->pAcelpEnc,
        isAceStart,
        &hPublicData->outputStreamLenBits,
        hPublicData->modes,
        bUsacIndependencyFlag,
        bUsacNoiseFilling,
        bCommonWindowMode,
        hPublicData->totalBitRate,
        maxBitsToUse,
        ipfState,
        &hPrivateData->pAcelpEnc.lpd_channel_stream);

    if (err != noError) {
      err = handBack(err);
    }
  }

  if (hPrivateData->pAcelpEnc.lpd_channel_stream.acelp_core_mode < MIN_ACELP_COREMODE || hPrivateData->pAcelpEnc.lpd_channel_stream.acelp_core_mode > MAX_ACELP_COREMODE) {
    err = iisUtil_ERROR(CDI, "Invalid core mode");
  }

  hPublicData->useLpd = 1;

  hPublicData->nOutFrames = N_FRAMES_LPD_MAX;

  return err;
}

int iisLPDEncLib_UpdateBitRes(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int bitReservoir,
    int bitReservoirMax) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData;

  if (hPublicData == NULL) {
    return 0;
  }

  hPrivateData = GetPrivateDataHandle(hPublicData);
  hPrivateData->pAcelpEnc.bitResFillLevel = (float)bitReservoir / (float)bitReservoirMax;

  return 0;
}

int iisLPDEncLib_GetLastSubfrWasACELP(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData) {
  if (hPublicData == NULL) {
    return 0;
  }

  return (hPublicData->modes[3] == 0);
}

void iisLPDEncLib_SetFdOverlapLen(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int lastWasShort,
    int nextIsShort) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData;

  if (hPublicData != NULL) {
    hPrivateData = GetPrivateDataHandle(hPublicData);
    hPrivateData->pAcelpEnc.lastWasShort = lastWasShort;
    hPrivateData->pAcelpEnc.nextIsShort = nextIsShort;
  }
}

void iisLPDEncLib_SetRestrictedMode(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int mode) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData;

  if (hPublicData == NULL) {
    return;
  }

  hPrivateData = GetPrivateDataHandle(hPublicData);

  hPrivateData->pAcelpEnc.restrictedMode = mode;
}

HANDLE_ERROR_INFO iisLPDEncLib_FdLpdTransition(
    const LPD_ENC_PUBLIC_DATA_HANDLE hPublicData,
    int ch,
    float* origTimeSig,
    float* synthTime,
    int nGranuleLength,
    int lowpassLine,
    int lfac,
    LPDENC_TRANSITION_TYPE transitionType,
    char* facPrm,
    int* Nbits_fac) {
  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int isFdToLpd = (transitionType == LPDENC_TRANSITION_FD2LPD) || (transitionType == LPDENC_TRANSITION_LPD2FD2LPD);
  int isLpdToFd = (transitionType == LPDENC_TRANSITION_LPD2FD) || (transitionType == LPDENC_TRANSITION_LPD2FD2LPD);
  int lastSubframeWasAcelp = hPublicData->modes[3] == 0;

  if (hPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer");
  }

  hPrivateData = GetPrivateDataHandle(hPublicData);

  LPDEnc_transition_FdAcelp(&hPrivateData->pAcelpEnc,
                            origTimeSig,
                            synthTime,
                            ch,
                            lowpassLine,
                            nGranuleLength,
                            lfac,
                            isFdToLpd,
                            isLpdToFd,
                            lastSubframeWasAcelp,
                            facPrm,
                            Nbits_fac);
  return noError;
}
