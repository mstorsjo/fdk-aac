
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
#include <math.h>

#include "iisxHEAACEncLib_encodeFrame_ifc.h"
#include "iisxHEAACEncLib_aac_ifc.h"

#include "smpl_resampler.h"
#include "mathlib.h"
#include "glob_con.h"
#include "iisxHEAACEncLib_mpegs_ifc.h"

#ifndef _NOT_AVOID_FLOAT_DENORMALS
static const float myInMin = 1.f / (float)(1 << 27);
__inline static int checkForNaN(float a) {
#if (defined(_MSC_VER) && (_MSC_VER >= 1900)) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
  return isnan(a);
#else

  return (a != a);
#endif
}

static void flushToZero(float limit, float *const X, const int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (fabs(X[i]) < limit) {
      X[i] = 0.f;
    }
    if (checkForNaN(X[i])) {
      X[i] = 0.f;
    }
  }
}
#endif

static XHEAACENCLIB_RETURN encodeDrc(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    DYNAMIC_DATA_EXTENSION *const dynamicDataExt,
    float *const drcIn) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int framesBeforeIpf = 0;
  int isSyncFrame = 0;

  if (hAudioPreRoll == NULL || hUsacIndepFlag == NULL || hDrc == NULL || hConfig == NULL || dynamicDataExt == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    printErrorConsole(CDI, "Invalid handle");
  } else if (drcIn == NULL) {
    {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
      printErrorConsole(CDI, "Invalid handle");
    }
  }

  if (!isError(retValue)) {
    if ((hAudioPreRoll != NULL) && (iisAudioPreRollLibGetnPreRollAu(hAudioPreRoll) <= 2)) {
      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 2, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
      if (isSyncFrame) framesBeforeIpf = 2;

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 1, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
      }
      if (isSyncFrame) framesBeforeIpf = 1;
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_drc_ifc_apply(hDrc,
                                             hConfig->nInChannels,
                                             drcIn,
                                             hConfig->nFrameSamples,
                                             &dynamicDataExt->pDynamicDataExtension,
                                             &dynamicDataExt->pDynamicDataExtensionLength,
                                             framesBeforeIpf, hConfig->aot);
  }

  if (isError(retValue)) {
    printErrorConsole(CDI, "DRC Gains could not be calculated, error in iisxHEAACEncLib_encodeFrame_ifc_preResamplerEncodeDrc()");
  }

  return retValue;
}

static XHEAACENCLIB_RETURN submitDrc(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_EXT_ELEMENT_LIST *const extEleList,
    XHEAACENCLIB_BD_DATA *const bitDistribution,
    DYNAMIC_DATA_EXTENSION *const dynamicDataExt) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int tmp_ExtensionPayloadSize = 0;

  if (extEleList == NULL || bitDistribution == NULL || hConfig == NULL || dynamicDataExt == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hConfig->aot == AUD_OBJ_TYP_USAC) {
    retValue = iisxHEAACEncLib_extentsionData_usacSubmit(extEleList,
                                                         XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC,
                                                         dynamicDataExt->pDynamicDataExtension,
                                                         dynamicDataExt->pDynamicDataExtensionLength,
                                                         0,
                                                         0);

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_extensionData_GetExtensionLength(extEleList, XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC, 0, &tmp_ExtensionPayloadSize);
    }
  } else {
    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_extensionData_generalSubmit(extEleList,
                                                             XHEAACENCLIB_EXT_ELE_AAC_UNI_DRC,
                                                             dynamicDataExt->pDynamicDataExtension,
                                                             dynamicDataExt->pDynamicDataExtensionLength,
                                                             -1);
    }
    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_extensionData_GetExtensionLength(extEleList, XHEAACENCLIB_EXT_ELE_AAC_UNI_DRC, -1, &tmp_ExtensionPayloadSize);
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_DRC, tmp_ExtensionPayloadSize);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_preResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib *const hPreResampler,
    TIME_SIGNAL_DATA *const timeSignal,
    int const nSamples,
    unsigned int *const pSamplesNext,
    unsigned int *const nSamplesPreResampOut) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (timeSignal == NULL || pSamplesNext == NULL || hConfig == NULL || nSamplesPreResampOut == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (hPreResampler) {
    if (!isError(retValue)) {
      if (0 != smpl_resampler_advance(hPreResampler,
                                      timeSignal->pPreResamplerIn,
                                      nSamples,
                                      &timeSignal->pPreResamplerOut,
                                      nSamplesPreResampOut,
                                      &timeSignal->pPreResamplerIn,
                                      pSamplesNext)) {
      }
#ifndef _NOT_AVOID_FLOAT_DENORMALS
      if (!isError(retValue)) {
        flushToZero(myInMin, timeSignal->pPreResamplerOut, *nSamplesPreResampOut);
      }
#endif
      if (!isError(retValue)) {
        if (pSamplesNext != NULL) {
          timeSignal->nSamplesNext = *pSamplesNext;
        }
      }
    }
  } else {
    timeSignal->pPreResamplerOut = timeSignal->pPreResamplerIn;
    *nSamplesPreResampOut = nSamples;
    *pSamplesNext = hConfig->nFrameSamples * hConfig->nInChannels;
    timeSignal->nSamplesNext = *pSamplesNext;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_feedDrcBuffers(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_MP4TIMEBUF const hCoreDelayBuffer,
    TIME_SIGNAL_DATA *const timeSignal,
    XHEAACENCLIB_DRC_DELAY *const drcDelay,
    unsigned int const nSamplesPreResampOut,
    float **const drcIn) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || hCoreDelayBuffer == NULL || timeSignal == NULL || drcDelay == NULL || drcIn == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  int tmpSamples = nSamplesPreResampOut / hConfig->nInChannels;
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = MP4TIMEBUF_FeedBufferMulti(hCoreDelayBuffer, timeSignal->pPreResamplerOut, &tmpSamples);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    assert(tmpSamples == 0);
    HANDLE_ERROR_INFO errorInfo = noError;
    if (drcDelay->drcCompensationDelay > 0) {
      if (!isError(retValue)) {
        tmpSamples = nSamplesPreResampOut / hConfig->nInChannels;
        errorInfo = MP4TIMEBUF_FeedBufferMulti(drcDelay->hDrcLookaheadBuffer, timeSignal->pPreResamplerOut, &tmpSamples);
      }

      if (errorInfo == noError) {
        errorInfo = MP4TIMEBUF_SaveAccessBuffer(hCoreDelayBuffer, 0, hConfig->nFrameSamples, 0, &(timeSignal->pPreResamplerOutDelayed));
      }
      if (errorInfo == noError) {
        errorInfo = MP4TIMEBUF_SaveAccessBuffer(drcDelay->hDrcLookaheadBuffer, 0, hConfig->nFrameSamples + drcDelay->drcLookAhead, 0, drcIn);
      }
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
      }
    } else {
      if (!isError(retValue)) {
        timeSignal->pPreResamplerOutDelayed = timeSignal->pPreResamplerOut;
        errorInfo = MP4TIMEBUF_SaveAccessBuffer(hCoreDelayBuffer, 0, hConfig->nFrameSamples + drcDelay->drcLookAhead, 0, drcIn);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_sbrSyncframe(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    int *const isSyncFrame) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int sendSbrHeaderDelay = 0;

  if (hUsacIndepFlag == NULL || hConfig == NULL || hSbrEnc == NULL || isSyncFrame == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hConfig->aot != AUD_OBJ_TYP_USAC) {
    retValue = iisxHEAACEncLibSbrEncSendHeaderDelay(hSbrEnc, &sendSbrHeaderDelay);

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, sendSbrHeaderDelay, isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER);
    }

    if (!isError(retValue) && *isSyncFrame == 1) {
      retValue = iisxHEAACEncLibSbrEncSendHeader(hSbrEnc);
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_sbrbitDistribution(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadSbr[],
    XHEAACENCLIB_BD_DATA *const bitDistribution) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int headerFlag = 0;
  int usedBits = 0;
  int elem = 0;

  if (bitDistribution == NULL || hConfig == NULL || hSbrEnc == NULL || hExtPayloadSbr == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSbrMoveExtPayload(hSbrEnc, hExtPayloadSbr, &usedBits);
  }

  if (!isError(retValue)) {
    usedBits = 0;
    for (elem = 0; elem < hConfig->cm->nElements; elem++) {
      if (hExtPayloadSbr[elem] != NULL) {
        usedBits += getTotalSize_extPayload(hExtPayloadSbr[elem]);
      }
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_SBR, usedBits);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSbrEncContainsHeader(hSbrEnc, &headerFlag);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_coreResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib *const hCoreDownSampler,
    TIME_SIGNAL_DATA *const timeSignal,
    float *ppCoreDownSamplerIn,
    unsigned int const nSamplesPreResampOut,
    unsigned int *const pnSamplesCoreDownSampOut,
    float **const ppAacCoreInTmp) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || timeSignal == NULL || pnSamplesCoreDownSampOut == NULL || ppAacCoreInTmp == NULL ||
      ppCoreDownSamplerIn == NULL || hCoreDownSampler == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hCoreDownSampler) {
    unsigned int nSamplesCoreDownSampNext = 0;

    copyFLOAT(timeSignal->pPreResamplerOutDelayed,
              ppCoreDownSamplerIn,
              nSamplesPreResampOut);

    if (0 != smpl_resampler_advance(hCoreDownSampler,
                                    ppCoreDownSamplerIn,
                                    nSamplesPreResampOut,
                                    ppAacCoreInTmp,
                                    pnSamplesCoreDownSampOut,
                                    &ppCoreDownSamplerIn,
                                    &nSamplesCoreDownSampNext)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
    }
  }

  if (!isError(retValue)) {
    if (*pnSamplesCoreDownSampOut != (hConfig->granuleLength * hConfig->nChannelsCoreCoder)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreCollectPointers(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_EXT_ELEMENT_LIST *const extEleList,
    XHEAACENCLIB_EXTPAYLOAD_LIST *const sbrPayloadList,
    XHEAACENCLIB_EXTPAYLOAD_LIST *const mpsPayloadList,
    HANDLE_EXTPAYLOAD_CONTAINER *const extContainer,
    int const mpegsEncExist) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int containersAdded = 0;
  int k = 0;
  int nInitContainers = extEleList->numExtEle;

  if (hConfig == NULL || extEleList == NULL || sbrPayloadList == NULL || mpsPayloadList == NULL || extContainer == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hConfig->bUseSBR) {
      int sbrContainersAdded = 0;
      for (k = containersAdded; k < containersAdded + hConfig->cm->nElements; k++) {
        if (hConfig->cm->elInfo[k].elType == ID_CPE || hConfig->cm->elInfo[k].elType == ID_SCE) {
          extContainer[containersAdded + sbrContainersAdded] = sbrPayloadList->hExtPayload[k - containersAdded];
          sbrContainersAdded++;
        }
      }
      extEleList->numExtEle += sbrPayloadList->nPayloads;
      containersAdded += sbrPayloadList->nPayloads;
    }

    if (mpegsEncExist) {
      for (k = containersAdded; k < containersAdded + mpsPayloadList->nPayloads; k++) {
        extContainer[k] = mpsPayloadList->hExtPayload[k - containersAdded];
      }
      extEleList->numExtEle += mpsPayloadList->nPayloads;
      containersAdded += mpsPayloadList->nPayloads;
    }

    for (k = containersAdded; k < containersAdded + nInitContainers; k++) {
      extContainer[k] = extEleList->extEle[k - containersAdded].container;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreSwDeci(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_IIS_SWDECI const hSwDeci,
    int coreModeNext[]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int coreModeTmp = 0;
  unsigned int ch = 0;
  int el = 0;

  if (hConfig == NULL || (hConfig->coreMode == XHEAACENCLIB_CODING_MODE_SWITCHED && hSwDeci == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hConfig->coreMode == XHEAACENCLIB_CODING_MODE_SWITCHED) {
      IIS_SWDECI_RESULT dec = IIS_SWDECI_RESULT_INVALID;
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = iisSwitchingDecisionGetNextDecision(hSwDeci, SWDECI_ID_CORE_CODER, &dec);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
      }

      switch (dec) {
        case IIS_SWDECI_RESULT_INVALID:
          break;
        case IIS_SWDECI_RESULT_MUSIC:
          coreModeTmp = 0;
          break;
        case IIS_SWDECI_RESULT_SPEECH:
          coreModeTmp = 1;
          break;
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
          break;
      }
    } else if (hConfig->coreMode == XHEAACENCLIB_CODING_MODE_LPD) {
      coreModeTmp = 1;
    }
  }

  if (!isError(retValue)) {
    for (ch = 0; ch < hConfig->nChannelsCoreCoder; ch++) {
      coreModeNext[ch] = coreModeTmp;
    }

    for (el = 0; el < hConfig->cm->nElements; el++) {
      if (hConfig->cm->elInfo[el].elType == ID_LFE) {
        coreModeNext[hConfig->cm->elInfo[el].ChannelIndex[0]] = 0;
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreSAP(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    int coreModeNext[]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || hAacEnc == NULL || hUsacIndepFlag == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hConfig->aot == AUD_OBJ_TYP_USAC) {
    if (iisxHEAACEncLibAacEncGetResidualCoding(hConfig) == 1 && hConfig->bitRate >= 48000) {
      coreModeNext[1] = 0;
    }
  }

  if (!isError(retValue) && hConfig->aot != AUD_OBJ_TYP_USAC) {
    int triggerSAP = 0;
    if (hConfig->configSet == CONFIG_SET_DASH) {
      XHEAACENCLIB_SAP_TYPE aacSyncType = XHEAACENCLIB_SAP_TYPE_NONE;
      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 1, &triggerSAP, XHEAACENCLIB_SYNCFRAME_TYPE_CORE_LOW_OVERLAP);

      if (!isError(retValue)) {
        if (triggerSAP) aacSyncType = XHEAACENCLIB_SAP_TYPE_WDWTYPE;

        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, &triggerSAP, XHEAACENCLIB_SYNCFRAME_TYPE_CORE_HIGH_BW);

        if (!isError(retValue) && triggerSAP) aacSyncType = XHEAACENCLIB_SAP_TYPE_WDWTYPE_HIGHBW;
      }

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLibAacEncSAPPrepare(hAacEnc, aacSyncType);
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCoreResetSbr(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_EXT_ELEMENT_LIST *const extEleList,
    XHEAACENCLIB_EXTPAYLOAD_LIST *const sbrPayloadList,
    XHEAACENCLIB_EXTPAYLOAD_LIST *const mpsPayloadList,
    XHEAACENCLIB_BITRESERVOIR_DATA *const bitReservoirData) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int j;

  if (hConfig == NULL || hAacEnc == NULL || extEleList == NULL || sbrPayloadList == NULL || mpsPayloadList == NULL || bitReservoirData == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hConfig->bUseSBR) {
      for (j = 0; (int)j < hConfig->cm->nElements; j++) {
        if (sbrPayloadList->hExtPayload[j] != NULL) {
          resetExtensionPayloadContainer(sbrPayloadList->hExtPayload[j]);
        }
      }
    }

    extEleList->numExtEle -= sbrPayloadList->nPayloads;
    for (j = 0; (int)j < mpsPayloadList->nPayloads; j++) {
      resetExtensionPayloadContainer(mpsPayloadList->hExtPayload[j]);
    }
    extEleList->numExtEle -= mpsPayloadList->nPayloads;

    retValue = iisxHEAACEncLibAacEncGetBitReservoirInfo(hAacEnc,
                                                        bitReservoirData->bitReservoirMax,
                                                        &bitReservoirData->bitReservoir,
                                                        &bitReservoirData->bitReservoirLevel);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_aacCorePostEncode(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll,
    XHEAACENCLIB_EXTENDED_BIT_RESERVOIR_PARAMS *const ebrParams,
    XHEAAC_AACENC_IPF_STATE const ipfState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || hAacEnc == NULL || audioPreRoll == NULL || ebrParams == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_audioPreRoll_postEncode(audioPreRoll);
  }

  if (!isError(retValue) && hConfig->rapOnDemandAdvancedMode) {
    if (hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND &&
        (ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF || ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL) &&
        audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL &&
        hConfig->aot == AUD_OBJ_TYP_USAC &&
        (hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE || hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SEEKABLE) && !isVbr(hConfig->bitrateMode)) {
      retValue = iisxHEAACEncLibAacEncUpdateExtendedBitReservoir(hAacEnc, ebrParams->nBitsAuPreRoll, 0, ebrParams->preRollAUFactor);
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_drcProcess(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    DYNAMIC_DATA_EXTENSION *const dynamicDataExt,
    float *const drcIn,
    XHEAACENCLIB_EXT_ELEMENT_LIST *const extEleList,
    XHEAACENCLIB_BD_DATA *const bitDistribution) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hAudioPreRoll == NULL || hUsacIndepFlag == NULL || hDrc == NULL || hConfig == NULL ||
      dynamicDataExt == NULL || extEleList == NULL || bitDistribution == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = encodeDrc(hConfig,
                         hAudioPreRoll,
                         hUsacIndepFlag,
                         hDrc,
                         dynamicDataExt,
                         drcIn);
  }

  if (!isError(retValue)) {
    retValue = submitDrc(hConfig,
                         extEleList,
                         bitDistribution,
                         dynamicDataExt);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_encodeFrame_ifc_mps(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadMps[],
    XHEAACENCLIB_BD_DATA *const bitDistribution,
    int coreCoderFrameLength,
    XHEAACENCLIB_SYNCFRAME_HANDLE hUsacIndepFlag,
    int speechFlag,
    float *pPreResamplerOut,
    int *nSamplesPreResampOut,
    float **ppQmfSamplesReal,
    float **ppQmfSamplesImag,
    float *pPreResamplerOutLr) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int usedBits = 0;

  if (hMpegsEnc == NULL || hExtPayloadMps == NULL || hUsacIndepFlag == NULL || pPreResamplerOut == NULL || hConfig == NULL ||
      nSamplesPreResampOut == NULL || ppQmfSamplesReal == NULL || bitDistribution == NULL || ppQmfSamplesImag == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int usePreResamplerOutLr = 0;
    if (hConfig->stereoConfigIndex > 0 && hConfig->pResidualConfig->mode) {
      usePreResamplerOutLr = 1;
    }

    retValue = iisxHEAACEncLibMpegsEncEncode(hMpegsEnc,
                                             coreCoderFrameLength,
                                             hUsacIndepFlag,
                                             speechFlag,
                                             pPreResamplerOut,
                                             hConfig->nInChannels,
                                             nSamplesPreResampOut,
                                             ppQmfSamplesReal,
                                             ppQmfSamplesImag,
                                             hConfig->nChannelsCoreCoder,
                                             pPreResamplerOutLr,
                                             usePreResamplerOutLr);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibMpsMoveExtPayload(hConfig->cm->nElements, hMpegsEnc, hExtPayloadMps, &usedBits);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_MPS, usedBits);
  }

  return retValue;
}
