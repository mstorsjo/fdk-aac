
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "fpucontrol.h"
#include "mathlib.h"
#include "mp4audioheader.h"

#include "iisxHEAACEncLib.h"
#include "iisxHEAACEncLib_delayAndBuffer.h"
#include "iisxHEAACEncLib_encoderStateMachine.h"
#include "iisxHEAACEncLib_extentsionData.h"
#include "iisxHEAACEncLib_live_loudness_leveler_ifc.h"
#include "iisxHEAACEncLib_loaswrapper.h"
#include "iisxHEAACEncLib_loudness_ifc.h"
#include "iisxHEAACEncLib_lraControl.h"
#include "iisxHEAACEncLib_drc_loudness_ifc.h"
#include "iisxHEAACEncLib_rapOnDemand.h"
#include "iisxHEAACEncLib_resampler_ifc.h"
#include "iisxHEAACEncLib_submitDrcMetadata.h"
#include "iisxHEAACEncLib_updateParamList.h"
#include "iisxHEAACEncLibConfig.h"
#include "iisxHEAACEncLib_encodeFrame_ifc.h"

#define XHEAACENCLIB_VERSION_NUMBER "04.00.00"

#define MAX_AAC_FRAMES_PER_MPS_FRAME ((128 * 64) / 1024)
#define MAX_SBR_PAYLOADS_PER_FRAME 20
#define MAX_AU_PER_MULTIFRAME 30
#define MAX_DRM_SIZE 64
#define MAX_SMC_SIZE 256
#define MAX_NUM_STREAMS 2

#define MAX_EXT_PAYLOAD_SIZE 269
#define IISXHEAACENCLIB_DEFAULT_LENGTH_UNIDRC 2

#define IISXHEAACENCLIB_STREAM_ID_EXTCONF_SIZE 2
#define IISXHEAACENCLIB_MAX_STREAM_ID ((1 << (IISXHEAACENCLIB_STREAM_ID_EXTCONF_SIZE * 8)) - 1)

#define PRIVATE_DATA_OFFSET_BYTES (sizeof(void *) - sizeof(struct xheaacenclib_instance_struct) % sizeof(void *))

#define IISXHEAACENCLIB_SAMPLE_PEAK_MIN -107.0F

#define IISXHEAACENCLIB_LIVE_PEAK_TO_LOUDNESS_RATIO 20

typedef struct auInfo_struct {
  int nAccessUnits;
  int *pByteCnt;
  int *pNumValidSamples;
} AUINFO;

typedef struct smcBuf_struct {
  int nSmcSizeBits;
  unsigned char smcBuffer[MAX_SMC_SIZE];
} SMCBUF;

typedef struct {
  int encBufEmpty;
  int samplesToFlush;
  int outSamplesStillValid;
} TIME_SIGNAL_FLUSHING;

typedef struct {
  XHEAACENCLIB_HANDLE_DRCENCODER hDrc;
  XHEAACENCLIB_DRCENCODER_SETUP setupDrc;
  XHEAACENCLIB_LOUDNESS_HANDLE hLoudness;
  XHEAACENCLIB_HANDLE_DRC_LOUDNESS hDrcLoudness;
} XHEAACENCLIB_DRC_AND_LOUDNESS_DATA;

typedef struct xheaacenclib_instance_struct {
  PARAMLIST_INSTANCE_HANDLE hCodecParamList;
  XHEAACENCLIB_CONFIG_HANDLE hConfig;

  XHEAACENCLIB_HANDLE_AACENCODER hAacEnc;
  float aacCoreBandwidth;

  XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc;
  float **ppQmfSamplesReal;
  float **ppQmfSamplesImag;
  float *pQmfDownSamplerOut;

  XHEAACENCLIB_HANDLE_MPEGSENCODER hMpegsEnc;

  XHEAACENCLIB_SWDECI switchingDecision;

  XHEAACENCLIB_AUDIOPREROLL_DATA audioPreRoll;

  XHEAACENCLIB_LRACONTROL_DRC_GAIN_DATA lraControlDrcGainData;

  XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA drcExternalNodes;

  XHEAACENCLIB_AUINFOLIST_HANDLE hAuInfoList;
  AUINFO auInfo;

  SMCBUF smcBuf;

  ASCBUF asc;

  struct tag_resamplelib *hPreResampler;
  struct tag_resamplelib *hCoreDownSampler;

  XHEAACENCLIB_SYNCFRAME_HANDLE hUsacIndepFlag;

  XHEAACENCLIB_BD_DATA bitDistribution;

  XHEAACENCLIB_ENCODER_STATE encoderState;

  unsigned char const *pAncBytes;
  unsigned int numAncBytes;

  TIME_SIGNAL_DATA timeSignal;

  TIME_SIGNAL_FLUSHING timeSignalFlushing;

  float *pPreResamplerOutLr;

  float *pCoreDownSamplerIn;

  HANDLE_MP4TIMEBUF collectiveBuffer;

  XHEAACENCLIB_EXTENDED_BIT_RESERVOIR_PARAMS ebrParams;

  DELAY_PARAMETER delayParamter;
  MPEG4_DELAY mpeg4DelayParameter;

  int nTrashAUs;

  HANDLE_STREAM_FORMAT hLoaswriter;
  LOASWRITER_SMC_WRITTEN smcWritten;
  HANDLE_BITBUFFER hBb;

  XHEAACENCLIB_DRC_AND_LOUDNESS_DATA drcAndLoudness;

  XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE hLiveLoudness;
  unsigned int unprocessedSamplesInCollectiveBuf;
  unsigned int nSamplesLeftToFlushFromLeveler;

  LoudnessInfoSet *loudnessInfoSet;
  XHEAACENCLIB_LOUDNESS_SETUP_MEASURE setupLoudnessMeasurement;
  XHEAACENCLIB_DRC_LOUDNESS_SETUP setupDrcLoudness;

  XHEAACENCLIB_ENCBUFFER encBufferData;

  unsigned int bitRatePenalty;
  int TLOverheadPerAU;
  int useSbrSpeechConfig;

  SAMPLING_FACTOR sbrRatio;

  XHEAACENCLIB_EXTPAYLOAD_LIST sbrPayloadList;
  XHEAACENCLIB_EXTPAYLOAD_LIST mpsPayloadList;

  XHEAACENCLIB_BITRESERVOIR_DATA bitReservoirData;

  HANDLE_IIS_FPU_CONTROL hFpuCtrl;

  int lastRapFrameDist;

  XHEAACENCLIB_SIGMAP_HANDLE hSigMap;

  HANDLE_MP4TIMEBUF hDelayBufferInputChannels[USAC_MAX_CHANNELS];

  XHEAACENCLIB_EXT_ELEMENT_LIST extEleList;
  XHEAACENCLIB_CONFIG_EXTENSION_LIST configExtensionList;

  HANDLE_MP4TIMEBUF hCoreDelayBuffer;
  XHEAACENCLIB_DRC_DELAY drcDelay;
  int rapFrameInAdvance;

  XHEAACENCLIB_WARNING_LIST warningList;

} XHEAACENCLIB_INSTANCE;

static XHEAACENCLIB_RETURN errorMapping_drc(DRC_IFC_RETURN const retValueDrc);

static XHEAACENCLIB_RETURN iisxHEAACEncLibMaxBitReservoir(XHEAACENCLIB_INSTANCE_HANDLE hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibDrcFillConfiguration(XHEAACENCLIB_DRCENCODER_SETUP *const hDrcSetup, XHEAACENCLIB_CONFIG_HANDLE const hxHEConfig, XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA const *const drcExternalNodes);

static XHEAACENCLIB_RETURN iisxHEAACEncLibUpdateAuInfoList(XHEAACENCLIB_INSTANCE_HANDLE hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessOpen(XHEAACENCLIB_INSTANCE_HANDLE hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessProcessing(
    XHEAACENCLIB_INSTANCE_HANDLE const hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessDelete(XHEAACENCLIB_INSTANCE_HANDLE hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessPrepareFlushing(XHEAACENCLIB_INSTANCE_HANDLE hInstance);

static XHEAACENCLIB_RETURN iisxHEAACEncLibGetAudioPreRollBuffer(XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll, ENCODEFRAME_PREROLLAUBUFFER *const preRollAuBufer, unsigned int minOutBufSize);

static XHEAACENCLIB_RETURN errorMapping_drc(DRC_IFC_RETURN const retValueDrc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  switch (retValueDrc) {
    case DRC_IFC_NO_ERROR:
      retValue = XHEAACENCLIB_RETURN_NO_ERROR;
      break;
    case DRC_IFC_ERROR_INVALID_HANDLE:
    case DRC_IFC_WARNING_PARAM_NULL:
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
      break;
    case DRC_IFC_ERROR_INVALID_SETUP:
      retValue = XHEAACENCLIB_RETURN_ERROR_DRC_INVALID_INSTRUCTIONS;
      break;
    case DRC_IFC_ERROR_CHARACTERISTIC_IDX_NOTSUPPORTED:
      retValue = XHEAACENCLIB_RETURN_ERROR_DRC_UNSUPPORTED_CHARACTERISTICS;
      break;
    case DRC_IFC_ERROR_MEMORY:
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      break;
    case DRC_IFC_ERROR_GAIN_ENC:
    case DRC_IFC_ERROR_GAIN_GEN:
    case DRC_IFC_ERROR_RTLRAC:
      retValue = XHEAACENCLIB_RETURN_ERROR_DRC;
      break;
    case DRC_IFC_ERROR_WRONG_NUMBER_SEQUENCES:
      retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_SEQUENCES;
      break;
    case DRC_IFC_ERROR_TIME_BUFF:
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
      break;
    default:
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
      break;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibMaxBitReservoir(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_SIGMAP_INDEX usedCicpIndex = XHEAACENCLIB_SIGMAP_INVALID;
  int sampleRateOut = -1;
  int nFrameSize = -1;
  int effectiveChannels = -1;
  float granuleBits = -1.0f;

  if (!isError(retValue)) {
    if (hInstance == NULL || hInstance->hConfig == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    }
  }
  if (!isError(retValue)) {
    if (hInstance->hConfig->bitrateMode == XHEAACENCLIB_BR_MODE_CBR) {
      if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
        usedCicpIndex = hInstance->hConfig->CicpIndex;
        sampleRateOut = hInstance->hConfig->sampleRateOut;

        switch (hInstance->hConfig->nFrameSamples) {
          case FRAMELENGTH_768:
            nFrameSize = 768;
            break;
          case FRAMELENGTH_960:
            nFrameSize = 960;
            break;
          case FRAMELENGTH_1024:
            nFrameSize = 1024;
            break;
          case FRAMELENGTH_1920:
            nFrameSize = 1920;
            break;
          case FRAMELENGTH_2048:
            nFrameSize = 2048;
            break;
          case FRAMELENGTH_4096:
            nFrameSize = 4096;
            break;
          default:
            retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE;
            break;
        }
      } else {
        usedCicpIndex = hInstance->hConfig->CicpIndexCoreCoder;
        sampleRateOut = hInstance->hConfig->sampleRateAAC;

        switch (hInstance->hConfig->granuleLength) {
          case XHEAACENCLIB_GRANULELENGTH_768:
            nFrameSize = 768;
            break;
          case XHEAACENCLIB_GRANULELENGTH_960:
            nFrameSize = 960;
            break;
          case XHEAACENCLIB_GRANULELENGTH_1024:
            nFrameSize = 1024;
            break;
          default:
            retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE;
            break;
        }
      }

      switch (usedCicpIndex) {
        case XHEAACENCLIB_SIGMAP_CICP_1:
          effectiveChannels = 1;
          break;
        case XHEAACENCLIB_SIGMAP_CICP_2:
          effectiveChannels = 2;
          break;
        case XHEAACENCLIB_SIGMAP_CICP_5:
          effectiveChannels = 5;
          break;
        case XHEAACENCLIB_SIGMAP_CICP_6:
          effectiveChannels = 5;
          break;
        case XHEAACENCLIB_SIGMAP_CICP_7:
        case XHEAACENCLIB_SIGMAP_CICP_12:
        case XHEAACENCLIB_SIGMAP_CICP_14:
          effectiveChannels = 7;
          break;
        case XHEAACENCLIB_SIGMAP_CICP_13:
          effectiveChannels = 22;
          break;
        case XHEAACENCLIB_SIGMAP_INVALID:
        case XHEAACENCLIB_SIGMAP_FROM_FILE:
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_CICP_INDEX;
          break;
      }
      if (!isError(retValue)) {
        granuleBits = (float)hInstance->hConfig->bitRate * nFrameSize / sampleRateOut;

        hInstance->bitReservoirData.bitReservoirMax = (int)max(effectiveChannels * 6144 - granuleBits, 0);
        hInstance->bitReservoirData.bitReservoirMax >>= 3;
        hInstance->bitReservoirData.bitReservoirMax <<= 3;
      }
    } else {
      hInstance->bitReservoirData.bitReservoirMax = 0;
    }
  }
  return retValue;
}

char const *XHEAACENCLIB_API IIS_xHEAACEncLib_GetVersion(void) {
  return XHEAACENCLIB_VERSION_NUMBER;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_New(
    XHEAACENCLIB_INSTANCE_HANDLE *const phInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_INSTANCE_HANDLE hInstance = NULL;
  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;

  if (!phInstance || *phInstance) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *phInstance = (XHEAACENCLIB_INSTANCE_HANDLE)iisCalloc(1, sizeof(XHEAACENCLIB_INSTANCE));

    if (!(*phInstance)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    } else {
      hInstance = *phInstance;
    }
  }

  if (!isError(retValue)) {
    hInstance->encoderState = XHEAACENCLIB_ENCODER_STATE_STARTUP;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_ConfigNew(&(hInstance->hConfig));
  }

  if (!isError(retValue)) {
    retValueSigMapIfc = iisxHEAACEncLib_SigMap_New(&hInstance->hSigMap);
    if (retValueSigMapIfc != IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue)) {
    hInstance->hAuInfoList = (XHEAACENCLIB_AUINFOLIST *)iisMalloc(sizeof(XHEAACENCLIB_AUINFOLIST));
    hInstance->hAuInfoList->hAccessUnits = (XHEAACENCLIB_AUINFO *)iisMalloc(sizeof(XHEAACENCLIB_AUINFO));
    hInstance->hAuInfoList->hAccessUnits->pNextAu = NULL;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_New(&hInstance->hUsacIndepFlag);
  }

  if (isError(retValue)) {
    IIS_xHEAACEncLib_Delete(phInstance);
    hInstance = NULL;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN openSbr(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  unsigned int i;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance->hConfig->bUseSBR) {
    retValue = iisxHEAACEncLibSbrEncOpen(hInstance->hSbrEnc,
                                         &hInstance->delayParamter.sbrEncoderDelay,
                                         &hInstance->delayParamter.sbrDecoderDelay);

    if (!isError(retValue)) {
      if (NULL == (hInstance->ppQmfSamplesReal = (float **)iisCalloc(hInstance->hConfig->nFrameSamples * hInstance->hConfig->nChannelsCoreCoder / 64, sizeof(float *)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
      }
    }

    if (!isError(retValue)) {
      if (NULL == (hInstance->ppQmfSamplesImag = (float **)iisCalloc(hInstance->hConfig->nFrameSamples * hInstance->hConfig->nChannelsCoreCoder / 64, sizeof(float *)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
      }
    }

    for (i = 0; i < hInstance->hConfig->nFrameSamples * hInstance->hConfig->nChannelsCoreCoder / 64; i++) {
      if (!isError(retValue)) {
        if (NULL == (hInstance->ppQmfSamplesReal[i] = (float *)iisCalloc(64, sizeof(float)))) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
          break;
        }
      }
      if (!isError(retValue)) {
        if (NULL == (hInstance->ppQmfSamplesImag[i] = (float *)iisCalloc(64, sizeof(float)))) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
          break;
        }
      }
    }

    if (!isError(retValue)) {
      if (NULL == (hInstance->pQmfDownSamplerOut = (float *)iisCalloc((size_t)hInstance->hConfig->nInChannels * hInstance->hConfig->granuleLength, sizeof(float)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
      }
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN openMPS(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  const MP4SPACEENC_ENCODERTYPE mpsEncoderType = MP4SPACEENC_ENCODERTYPE_USAC;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int channelOffset = 0;
  int elem = 0;

  if (hInstance->hConfig->stereoConfigIndex > 0) {
    retValue = iisxHEAACEncLibMpegsEncOpen(hInstance->hMpegsEnc,
                                           mpsEncoderType,
                                           hInstance->hConfig->granuleLength,
                                           hInstance->aacCoreBandwidth,
                                           hInstance->hConfig->bUseSbrQmfInput);
  }

  if (hInstance->hMpegsEnc) {
    for (elem = 0; elem < hInstance->hConfig->cm->nElements; elem++) {
      if (hInstance->hConfig->cm->elInfo[elem].elType == ID_SCE) {
        channelOffset += 1;
      } else if (hInstance->hConfig->cm->elInfo[elem].elType == ID_CPE) {
        channelOffset += 2;
      }
      if (!isError(retValue)) {
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = openExtensionPayloadContainer(&hInstance->mpsPayloadList.hExtPayload[elem], PAY_MUX_USAC, channelOffset);
        hInstance->mpsPayloadList.nPayloads++;
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
        }
      }
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN openAAC(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  float proposedBandwidth = 0.0f;
  proposedBandwidth = hInstance->aacCoreBandwidth;

  if (hInstance->hConfig->bitRate == 64000 && hInstance->hConfig->codecType == XHEAACENCLIB_CODEC_USAC && hInstance->hConfig->nInChannels == 1) {
    proposedBandwidth = 15750;
  }

  retValue = iisxHEAACEncLibAacEncOpen(hInstance->hAacEnc, hInstance->hConfig->sampleRateIn, proposedBandwidth, hInstance->hLoaswriter);

  if (!isError(retValue)) {
    if (hInstance->hConfig->bMpeg4NoStartStopSequence) {
      retValue = iisxHEAACEncLibAacEncSetNoStartStopSequence(hInstance->hAacEnc);
    }
  }
  if (!isError(retValue)) {
    XHEAACENCLIB_AACINFO aacInfo;

    retValue = iisxHEAACEncLibAacEncGetInfo(hInstance->hAacEnc, &aacInfo);

    hInstance->delayParamter.aacCoreCoderDelay = aacInfo.nDelay;
    hInstance->delayParamter.aacCoreCoderAddEncDelay = aacInfo.nAddEncDelay;
    hInstance->delayParamter.aacCoreCoderStandDelay = aacInfo.nStandDelay;
    hInstance->timeSignal.minOutBufSize = aacInfo.cbBufSizeMin;
    hInstance->timeSignal.nAncBytesPerFrame = aacInfo.nAncBytesPerFrame;
    hInstance->aacCoreBandwidth = min(aacInfo.bandWidth, (float)hInstance->hConfig->sampleRateIn / 2);

    if (hInstance->hConfig->aot == AUD_OBJ_TYP_HEAAC && hInstance->hConfig->bUseHBE) {
      hInstance->delayParamter.aacCoreCoderDelay += hInstance->hConfig->granuleLength;
      hInstance->delayParamter.aacCoreCoderStandDelay += hInstance->hConfig->granuleLength;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessUpdateAvailableSamples(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    int const nChannels,
    int *const nSamplesAvailableForNextAu) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int liveLoudnessLevelerDelay = 0;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_live_loudness_ifc_getLookahead(hInstance->hLiveLoudness, &liveLoudnessLevelerDelay);
  }

  if (!isError(retValue)) {
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_ENCODING) {
      *nSamplesAvailableForNextAu -= (nChannels * liveLoudnessLevelerDelay);
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN openUnivSte(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  hInstance->hConfig->pResidualConfig = iisxHEAACEncLibMpegsEncGetResidualConfig(hInstance->hMpegsEnc);

  if ((hInstance->hConfig->stereoConfigIndex > 0) && hInstance->hConfig->pResidualConfig->mode) {
    int nAacResampAndLookaheadDelay = hInstance->hConfig->sbrRatio.upFac *
                                      (hInstance->delayParamter.aacCoreCoderDelay + hInstance->delayParamter.coreResamplerDelay) /
                                      hInstance->hConfig->sbrRatio.downFac;
    int dmxDelay;

    retValue = iisxHEAACEncLibMpegsEncInitUnivSte(hInstance->hMpegsEnc,
                                                  nAacResampAndLookaheadDelay,
                                                  hInstance->hConfig->sampleRateOut,
                                                  hInstance->hConfig->sampleRateAAC,
                                                  hInstance->aacCoreBandwidth,
                                                  &dmxDelay);

    if (!isError(retValue)) {
      if (NULL == (hInstance->pPreResamplerOutLr = (float *)iisCalloc((size_t)dmxDelay * (size_t)hInstance->hConfig->nChannelsCoreCoder, sizeof(float)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
      } else {
        setFLOAT(0.f, hInstance->pPreResamplerOutLr, dmxDelay * hInstance->hConfig->nChannelsCoreCoder);
      }
    } else
      retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN openLatmLOAS(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  HANDLE_CODER_CONFIG_INFO hCoderConfig = NULL;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  hInstance->smcBuf.nSmcSizeBits = 0;

  errorInfo = IIS_Mp4ASC_CoderConfigInfoCreate(&hCoderConfig, 1, 1);
  if (errorInfo != noError) {
    retValue = XHEAACENCLIB_RETURN_ERROR_MP4_ASC;
  }

  if (!isError(retValue)) {
    hCoderConfig[0].samplingRate = hInstance->hConfig->sampleRateAAC;
    hCoderConfig[0].bitRate = hInstance->hConfig->bitRate;
    hCoderConfig[0].samplesFrame = hInstance->hConfig->granuleLength;
    hCoderConfig[0].bitsFrame = ((hCoderConfig[0].bitRate *
                                  hCoderConfig[0].samplesFrame) /
                                 hCoderConfig[0].samplingRate);

    hCoderConfig[0].asc = hInstance->hConfig->hCoderConfig->asc;
    hCoderConfig[0].ascFlag = 1;
  }

  if (!isError(retValue)) {
    LOASWRITER_MODE latmMode = MODE_LOAS;
    switch (hInstance->hConfig->transportFormat) {
      case TT_LOAS:
        latmMode = MODE_LOAS;
        break;
      case TT_LOAS_NOSMC:
        latmMode = MODE_LOAS_NO_SMC;
        break;
      case TT_LATM:
        latmMode = MODE_LATM;
        break;
      case TT_LATM_NOSMC:
        latmMode = MODE_LATM_NO_SMC;
        break;
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibCreate_loaswriter(&(hInstance->hLoaswriter),
                                                  &(hInstance->hBb),
                                                  hCoderConfig,
                                                  latmMode);
    }

    if (!isError(retValue)) {
      if (hInstance->hConfig->audioPreRollBitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT) {
        retValue = iisxHEAACEncLibLoaswriter_setSmcOutOfBitres(hInstance->hLoaswriter, 1);
      }
    }
    if (!isError(retValue)) {
      if (hInstance->hConfig->randomAccessIntervalInFrames < 0 && (hInstance->hConfig->randomAccessIntervalInFrames != -1 || hInstance->hConfig->rapOccurrence != XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_INTERVAL;
      }
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibLoaswriter_setLatmSmcTimeInterval(hInstance->hLoaswriter, hInstance->hConfig->randomAccessIntervalInFrames);
    }

    if (!isError(retValue)) {
      if (hInstance->hConfig->loasNrSubFrames > -1) {
        retValue = iisxHEAACEncLibLoaswriter_setNrOfSubframes(hInstance->hLoaswriter, hInstance->hConfig->loasNrSubFrames);
      }
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibLoaswriter_getLatmStreamMuxConfig(hInstance->hLoaswriter,
                                                                  hInstance->smcBuf.smcBuffer,
                                                                  &(hInstance->smcBuf.nSmcSizeBits));
    }

    if (hCoderConfig) IIS_Mp4ASC_CoderConfigInfoDelete(hCoderConfig);
  }
  return retValue;
}

static XHEAACENCLIB_RETURN openCoreCoders(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int elem;
  int channelOffset = 0;

  unsigned int nSamplesCoreResWanted = 0;
  EXTENSION_PAYLOAD_VERSION payloadVersion = PAY_MUX_UNKNOWN;

  if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
    payloadVersion = PAY_MUX_USAC;
  } else {
    payloadVersion = PAY_MUX_V1;
  }

  if (hInstance->hConfig->bUseSBR) {
    for (elem = 0; elem < hInstance->hConfig->cm->nElements; elem++) {
      if (hInstance->hConfig->cm->elInfo[elem].elType == ID_SCE) {
        channelOffset += 1;
        hInstance->sbrPayloadList.nPayloads++;
      } else if (hInstance->hConfig->cm->elInfo[elem].elType == ID_CPE) {
        channelOffset += 2;
        hInstance->sbrPayloadList.nPayloads++;
      } else if (hInstance->hConfig->cm->elInfo[elem].elType == ID_LFE) {
        channelOffset += 1;
      }

      if (hInstance->hConfig->cm->elInfo[elem].elType != ID_LFE) {
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = openExtensionPayloadContainer(&hInstance->sbrPayloadList.hExtPayload[elem], payloadVersion, channelOffset);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
        }
      }
    }
  }

  if (!isError(retValue)) {
    retValue = openSbr(hInstance);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibAacEncSnapToLowSfbBorder(hInstance->hAacEnc,
                                                       iisxHEAACEncLibSbrEncGetXOverFreq(hInstance->hSbrEnc),
                                                       0.1f,
                                                       &hInstance->aacCoreBandwidth);

    if (isError(retValue)) {
      hInstance->aacCoreBandwidth = iisxHEAACEncLibSbrEncGetXOverFreq(hInstance->hSbrEnc);
    }
    iisxHEAACEncLibAacEncSetBandwidth(hInstance->hAacEnc, hInstance->aacCoreBandwidth);

    if (hInstance->hConfig->stereoConfigIndex > 0) {
      iisxHEAACEncLibMpegsEncUpdateCpcStopFrequency(hInstance->hMpegsEnc, hInstance->aacCoreBandwidth);
    }
  }

  if (!isError(retValue)) {
    retValue = openMPS(hInstance);
  }

  if (!isError(retValue)) {
    retValue = openAudioSpecificConfig(hInstance->hConfig,
                                       hInstance->hAacEnc,
                                       hInstance->hSbrEnc,
                                       hInstance->hMpegsEnc,
                                       hInstance->extEleList.extEle,
                                       hInstance->extEleList.numExtEle,
                                       hInstance->configExtensionList.configExtension,
                                       hInstance->configExtensionList.numConfigExtension,
                                       &hInstance->asc);
  }

  if (!isError(retValue)) {
    if ((hInstance->hConfig->transportFormat == TT_LOAS) || (hInstance->hConfig->transportFormat == TT_LATM) ||
        (hInstance->hConfig->transportFormat == TT_LOAS_NOSMC) || (hInstance->hConfig->transportFormat == TT_LATM_NOSMC)) {
      retValue = openLatmLOAS(hInstance);
    }
  }

  if (!isError(retValue)) {
    retValue = openAAC(hInstance);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_resampler_ifc_openCoreResampler(hInstance->hConfig,
                                                               &hInstance->hCoreDownSampler,
                                                               &hInstance->delayParamter.coreResamplerDelay,
                                                               hInstance->hSbrEnc,
                                                               hInstance->aacCoreBandwidth,
                                                               &hInstance->pCoreDownSamplerIn,
                                                               &nSamplesCoreResWanted);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_resampler_ifc_openPreResampler(hInstance->hConfig,
                                                              &hInstance->hPreResampler,
                                                              &hInstance->timeSignal,
                                                              &hInstance->delayParamter.preResamplerDelay,
                                                              hInstance->aacCoreBandwidth,
                                                              nSamplesCoreResWanted);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_standardDelayCalculation(
        hInstance->hConfig,
        hInstance->hSbrEnc,
        hInstance->hMpegsEnc,
        hInstance->switchingDecision.hSwDeci,
        &hInstance->delayParamter,
        &hInstance->mpeg4DelayParameter);
  }

  if (!isError(retValue)) {
    retValue = openUnivSte(hInstance);
  }

  if (!isError(retValue)) {
    hInstance->auInfo.pByteCnt = (int *)iisCalloc(MAX_AU_PER_MULTIFRAME, sizeof(int));
    if (hInstance->auInfo.pByteCnt == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }
  if (!isError(retValue)) {
    hInstance->auInfo.pNumValidSamples = (int *)iisCalloc(MAX_AU_PER_MULTIFRAME, sizeof(int));
    if (hInstance->auInfo.pNumValidSamples == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue)) {
    hInstance->pAncBytes = NULL;
    hInstance->numAncBytes = 0;
  }

  if ((hInstance->switchingDecision.hSwDeci != NULL) && (hInstance->hConfig->coreMode == XHEAACENCLIB_CODING_MODE_SWITCHED)) {
    if (!isError(retValue)) {
      HANDLE_ERROR_INFO errorInfo = noError;

      int aacInputDelay = iisxHEAACEncLib_resampler_ifc_getCoreResamplerDelay(hInstance->hConfig,
                                                                              hInstance->mpeg4DelayParameter.mpeg4DelayWOSwitching,
                                                                              hInstance->delayParamter.aacCoreCoderDelay,
                                                                              hInstance->delayParamter.sbrDecoderDelay);

      aacInputDelay += (512 * hInstance->hConfig->sbrRatio.upFac) / hInstance->hConfig->sbrRatio.downFac;

      errorInfo = iisSwitchingDecisionAttach(hInstance->switchingDecision.hSwDeci,
                                             SWDECI_ID_CORE_CODER,
                                             hInstance->hConfig->sampleRateOut,
                                             hInstance->hConfig->nFrameSamples,
                                             aacInputDelay);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
      }
    }
  }

  if (!isError(retValue)) {
    IIS_FPUControl_Create(&hInstance->hFpuCtrl);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessOpen(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_LIVE_LOUDNESS_SETUP liveLoudnessSetup;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    liveLoudnessSetup.inSampleRate = hInstance->hConfig->sampleRateIn;
    liveLoudnessSetup.numInputChannels = hInstance->hConfig->nInChannels;
    liveLoudnessSetup.targetLoudnessLevel = hInstance->hConfig->liveLoudnessLevel;
    liveLoudnessSetup.frameLength = hInstance->hConfig->nFrameSamples;

    if (hInstance->hConfig->liveLoudnessLevel < -IISXHEAACENCLIB_LIVE_PEAK_TO_LOUDNESS_RATIO && !hInstance->hConfig->bLiveSamplePeakSet) {
      hInstance->hConfig->bLiveSamplePeakSet = 1;
      hInstance->hConfig->liveSamplePeak = hInstance->hConfig->liveLoudnessLevel + IISXHEAACENCLIB_LIVE_PEAK_TO_LOUDNESS_RATIO;
    }

    if (hInstance->hConfig->bLiveModeSet) {
      liveLoudnessSetup.liveMode = hInstance->hConfig->liveMode;
    } else {
      liveLoudnessSetup.liveMode = 0;
    }

    if (hInstance->hConfig->bLiveRelMaxGainSet) {
      liveLoudnessSetup.liveLoudnessRelMaxGain = hInstance->hConfig->liveRelMaxGain;
    } else {
      liveLoudnessSetup.liveLoudnessRelMaxGain = -1.0f;
    }

    if (hInstance->hConfig->bLiveSamplePeakSet) {
      liveLoudnessSetup.limiterThreshold = hInstance->hConfig->liveSamplePeak;
    } else {
      liveLoudnessSetup.limiterThreshold = 0.0f;
    }

    if (XHEAACENC_PRIMINGMODE_FULL == hInstance->hConfig->primingMode) {
      liveLoudnessSetup.compensateLevelerDelay = 0;
    } else {
      liveLoudnessSetup.compensateLevelerDelay = 1;
    }

    retValue = iisxHEAACEncLib_live_loudness_ifc_init(&hInstance->hLiveLoudness, liveLoudnessSetup);
  }

  if (!isError(retValue)) {
    unsigned int liveLoudnessDelayInSr = 0;
    retValue = iisxHEAACEncLib_live_loudness_ifc_getLookahead(hInstance->hLiveLoudness, &liveLoudnessDelayInSr);
    if (!isError(retValue)) {
      hInstance->nSamplesLeftToFlushFromLeveler = liveLoudnessDelayInSr * hInstance->hConfig->nInChannels;
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessProcessing(
    XHEAACENCLIB_INSTANCE_HANDLE const hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nTotalSamplesInCollectiveBuffer = 0;

  if (hInstance == NULL || hInstance->hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hInstance->hConfig->nInChannels > INT_MAX) {
    assert(0);
    retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_collectiveBuffer_getNumAvailableSamples(hInstance->collectiveBuffer,
                                                                       hInstance->hConfig->nInChannels,
                                                                       &nTotalSamplesInCollectiveBuffer);
  }

  if (!isError(retValue)) {
    int isFlushing = 0;
    int nSamplesToLevel = 0;
    float lastMeasuredLoudness;
    lastMeasuredLoudness = iisxHEAACEncLib_loudness_ifc_getLoudness(hInstance->drcAndLoudness.hLoudness);
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
      isFlushing = 1;
    }

    if (isFlushing) {
      if (hInstance->nSamplesLeftToFlushFromLeveler == 0) {
        nSamplesToLevel = 0;
      } else {
        nSamplesToLevel = nTotalSamplesInCollectiveBuffer;
      }
    } else {
      nSamplesToLevel = nTotalSamplesInCollectiveBuffer - hInstance->unprocessedSamplesInCollectiveBuf;
    }

    retValue = iisxHEAACEncLib_live_loudness_ifc_processing(hInstance->hLiveLoudness,
                                                            hInstance->collectiveBuffer,
                                                            nSamplesToLevel,
                                                            hInstance->unprocessedSamplesInCollectiveBuf,
                                                            isFlushing,
                                                            lastMeasuredLoudness);

    if (!isError(retValue) && isFlushing) {
      hInstance->nSamplesLeftToFlushFromLeveler = 0;
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessDelete(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_live_loudness_ifc_delete(hInstance->hLiveLoudness);
    hInstance->hLiveLoudness = NULL;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibLiveLoudnessPrepareFlushing(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nSamplesAvailableForNextAu = 0;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hInstance->hConfig->nFrameSamples < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_collectiveBuffer_getNumAvailableSamples(hInstance->collectiveBuffer,
                                                                       hInstance->hConfig->nInChannels,
                                                                       &nSamplesAvailableForNextAu);
  }
  if (!isError(retValue) && nSamplesAvailableForNextAu < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    unsigned int levelerDelayLeftToFlush = hInstance->nSamplesLeftToFlushFromLeveler;
    unsigned int nSamplesToFullFrame = (unsigned int)hInstance->hConfig->nFrameSamples * hInstance->hConfig->nInChannels - (unsigned int)nSamplesAvailableForNextAu;
    unsigned int zerosToAppend = min(nSamplesToFullFrame, levelerDelayLeftToFlush);
    float *pSamplesZeros = (float *)iisCalloc(zerosToAppend, sizeof(float));

    retValue = iisxHEAACEncLib_collectiveBuffer_feedSamples(hInstance->hConfig, hInstance->collectiveBuffer, pSamplesZeros, zerosToAppend);
    hInstance->nSamplesLeftToFlushFromLeveler -= zerosToAppend;
    iisFree(pSamplesZeros);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibDrcFillConfiguration(
    XHEAACENCLIB_DRCENCODER_SETUP *const hDrcSetup,
    XHEAACENCLIB_CONFIG_HANDLE const hxHEConfig,
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA const *const drcExternalNodes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int k;

  if (!isError(retValue)) {
    if (hDrcSetup == NULL || hxHEConfig == NULL || drcExternalNodes == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    }
  } else if (drcExternalNodes->drcExternalNodeNumNodes > INPUT_DRCNODE_COUNT_MAX) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_NODES;
  }

  if (!isError(retValue)) {
    hDrcSetup->sampleRate = hxHEConfig->sampleRateOut;
    hDrcSetup->nAudioChannels = hxHEConfig->nInChannels;
    hDrcSetup->nFrameLength = hxHEConfig->nFrameSamples;
    hDrcSetup->aot = hxHEConfig->aot;
    hDrcSetup->bAlbumLoudnessLevelSet = hxHEConfig->bAlbumLoudnessLevelSet;
    hDrcSetup->albumLoudnessLevel = hxHEConfig->albumLoudnessLevel;

    if (hxHEConfig->bRealtimeLRAC) {
      hDrcSetup->bRealtimeLRAC = 1;
    }
    hDrcSetup->bTargetLraSet = hxHEConfig->drcTargetLraPresent;
    hDrcSetup->targetLra = hxHEConfig->drcTargetLra;
    hDrcSetup->bIsLevelerActive = hxHEConfig->bLiveLoudnessLevelSet;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_drc_ifc_setupConfig(
        hDrcSetup,
        hxHEConfig->drcMode,
        drcExternalNodes);
  }

  if (!isError(retValue)) {
    for (k = 0; k < hDrcSetup->nCharacteristicCount; k++) {
      if (hxHEConfig->bAnchorLoudnessLevelSet) {
        hDrcSetup->loudnessLevel[k] = hxHEConfig->anchorLoudnessLevel;
      } else if (hxHEConfig->bLoudnessLevelSet) {
        hDrcSetup->loudnessLevel[k] = hxHEConfig->loudnessLevel;
      } else if (hxHEConfig->bLiveLoudnessLevelSet) {
        hDrcSetup->loudnessLevel[k] = hxHEConfig->liveLoudnessLevel;
      } else {
        retValue = XHEAACENCLIB_RETURN_ERROR_SETTING_DEFAULT_LOUDNESS;
      }
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibUpdateAuInfoList(XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int isLowOverlap = 0;
  int isFixSBR = 0;
  int isSBRHeader = 0;

  int isSyncFrame = 0;
  int isHEaccess = 0;
  int isHEswitch = 0;
  int isIPF = 0;
  int isSMC = 0;
  int isRAP = 0;

  if (!isError(retValue)) {
    if (hInstance == NULL || hInstance->hAuInfoList == NULL || hInstance->hAuInfoList->hAccessUnits == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retValue)) {
    if (hInstance->auInfo.pByteCnt[0] == 0 && hInstance->auInfo.pNumValidSamples[0] == 0) {
      hInstance->hAuInfoList->nAccessUnits = 0;
      hInstance->hAuInfoList->hAccessUnits->auSize = 0;
      hInstance->hAuInfoList->hAccessUnits->auSamplesValid = 0;
      hInstance->hAuInfoList->hAccessUnits->auOffset = 0;
    } else {
      hInstance->hAuInfoList->nAccessUnits = 1;
      hInstance->hAuInfoList->hAccessUnits->auSize = hInstance->auInfo.pByteCnt[0];
      hInstance->hAuInfoList->hAccessUnits->auSamplesValid = hInstance->auInfo.pNumValidSamples[0];
      hInstance->hAuInfoList->hAccessUnits->auOffset = 0;
    }

    isSMC = (hInstance->hLoaswriter != NULL && hInstance->smcWritten == SMC_WRITTEN);
    isIPF = (hInstance->audioPreRoll.transmitPayload == XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_ACT_FRAME);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isLowOverlap, XHEAACENCLIB_SYNCFRAME_TYPE_CORE_LOW_OVERLAP);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isFixSBR, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isSBRHeader, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isRAP, XHEAACENCLIB_SYNCFRAME_TYPE_IS_RAP);
  }

  if (!isError(retValue)) {
    isHEswitch = ((isFixSBR == 1 || hInstance->hConfig->aot == AUD_OBJ_TYP_LC) && isLowOverlap == 1);
    isHEaccess = (isSBRHeader == 1 || (hInstance->hConfig->aot == AUD_OBJ_TYP_LC && isRAP == 1));
  }

  if (!isError(retValue)) {
    hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_INVALID;
    if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
      if (isIPF) {
        if (isSMC) {
          hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IPF;
        } else {
          hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_IMMEDIATE_PLAY_OUT_FRAME;
        }
      } else {
        if (isSyncFrame) {
          if (isSMC) {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_STREAM_MUX_CONFIG_IF;
          } else {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_INDEPENDENT_FRAME;
          }
        } else {
          if (isSMC) {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_INVALID;
            retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
          } else {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_NO;
          }
        }
      }
    } else {
      if (isHEswitch) {
        if (isSMC) {
          hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_SWITCHABLE;
        } else {
          hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_HE_AAC_RAP_SWITCHABLE;
        }
      } else {
        if (isHEaccess) {
          if (isSMC) {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENC_SYNCFRAME_STREAM_MUX_CONFIG_HE_AAC_ACCESS;
          } else {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_NO;
          }
        } else {
          if (isSMC) {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_INVALID;
            retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
          } else {
            hInstance->hAuInfoList->hAccessUnits->isSyncFrame = XHEAACENCLIB_SYNCFRAME_NO;
          }
        }
      }
    }
  }
  if (!isError(retValue)) {
    hInstance->hAuInfoList->hAccessUnits->pNextAu = NULL;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibGetAudioPreRollBuffer(
    XHEAACENCLIB_AUDIOPREROLL_DATA *const audioPreRoll,
    ENCODEFRAME_PREROLLAUBUFFER *const preRollAuBufer,
    unsigned int minOutBufSize) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (audioPreRoll == NULL || preRollAuBufer == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    preRollAuBufer->pPreRollAUBuffer = audioPreRoll->PreRollAUBuffer;
    preRollAuBufer->pPreRollAUBufferBits = &audioPreRoll->nPreRollAUBufferBits;

    if (audioPreRoll->PreRollAUBufferSizeInBytes != minOutBufSize) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Update(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;
  DRC_LOUDNESS_IFC_RETURN retValueDrcLoudness = DRC_LOUDNESS_IFC_NO_ERROR;
  int bitRateCodingTools = 0;
  int bLiveLoudnessMeasurement = 0;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }
  if (!isError(retValue)) {
    hInstance->hCodecParamList = hCodecParamList;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListIterate(hCodecParamList,
                                    iisxHEAACEncLibGetParamList,
                                    hInstance->hConfig);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->mpeg4DrcLightProf != XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      hInstance->hConfig->bMpeg4DrcOn = 1;
    }
  }

  if (!isError(retValue)) {
    if (!hInstance->hConfig->bLoudnessEnvelopeSet && hInstance->hConfig->drcMode == XHEAACENCLIB_DRCMODE_LN_NE_LI_GE_LRACONTROL) {
      hInstance->hConfig->bRealtimeLRAC = 1;
    }
  }

  if (hInstance->hConfig->bRealtimeLRAC) {
    bLiveLoudnessMeasurement = 1;
  }
  if (hInstance->hConfig->bLiveLoudnessLevelSet) {
    bLiveLoudnessMeasurement = 1;
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF) {
      retValue = iisxHEAACEncLib_drc_ifc_New(&hInstance->drcAndLoudness.hDrc);
    }
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->bLoudnessLevelSet || hInstance->hConfig->bAnchorLoudnessLevelSet || hInstance->hConfig->bLiveLoudnessLevelSet) {
      retValueDrcLoudness = iisxHEAACEncLib_drc_loudness_ifc_new(&hInstance->drcAndLoudness.hDrcLoudness);
      if (retValueDrcLoudness) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      }
    }
  }

  if (!isError(retValue)) {
    if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
      retValueSigMapIfc = iisxHEAACEncLib_SigMap_Configure(hInstance->hSigMap, hInstance->hConfig);
    }
    if (retValueSigMapIfc == IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
      retValueSigMapIfc = iisxHEAACEncLib_SigMap_GetChannelMap(hInstance->hSigMap, &hInstance->hConfig->cm);
    }
    if (retValueSigMapIfc != IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
    }
  }

  if (!isError(retValue)) {
    retValue = applyConfigurationSettings(hInstance->hConfig);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibMaxBitReservoir(hInstance);
  }

  if (!isError(retValue) && (hInstance->hConfig->stereoConfigIndex > 0)) {
    retValue = iisxHEAACEncLibMpegsEncConfigure(&hInstance->hMpegsEnc, hInstance->hConfig);
  }

  if (!isError(retValue) && (hInstance->hConfig->bUseSBR)) {
    retValue = iisxHEAACEncLibSbrEncConfigure(&hInstance->hSbrEnc, hInstance->hConfig);

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibSbrEncSetRaisedXoverFreq(hInstance->hSbrEnc, hInstance->hConfig->bRaisedXOverFreq);
    }
  }

  if (!isError(retValue)) {
    hInstance->hConfig->pResidualConfig = iisxHEAACEncLibMpegsEncGetResidualConfig(hInstance->hMpegsEnc);

    if (hInstance->hConfig->bUseSBR) {
      bitRateCodingTools += iisxHEAACEncLibSbrEncGetEstimateSbrBitrate(hInstance->hSbrEnc);
    }
    hInstance->hConfig->bitRateCodingTools = bitRateCodingTools;

    retValue = iisxHEAACEncLibAacEncConfigure(&hInstance->hAacEnc, hInstance->hConfig);
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->bLiveLoudnessLevelSet) {
      retValue = iisxHEAACEncLibLiveLoudnessOpen(hInstance);
    }
  }

  if (!isError(retValue)) {
    LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;
    if (bLiveLoudnessMeasurement) {
      retValueLoudness = iisxHEAACEncLib_loudness_ifc_new(&hInstance->drcAndLoudness.hLoudness);
      if (retValueLoudness) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      }
    }
  }
  if (!isError(retValue)) {
    LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;
    if (bLiveLoudnessMeasurement) {
      hInstance->setupLoudnessMeasurement.sampleRate = hInstance->hConfig->sampleRateIn;
      hInstance->setupLoudnessMeasurement.nChannels = hInstance->hConfig->nInChannels;
      retValueLoudness = iisxHEAACEncLib_loudness_ifc_config(hInstance->drcAndLoudness.hLoudness, hInstance->setupLoudnessMeasurement);
      if (retValueLoudness) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }
  }

  if (!isError(retValue)) {
    memset(&hInstance->switchingDecision.swInfo, 0, sizeof(IIS_SWDECI_INFO));

    if (hInstance->hConfig->coreMode == XHEAACENCLIB_CODING_MODE_SWITCHED) {
      HANDLE_ERROR_INFO errorInfo;
      IIS_SWDECI_SETUP swDeciSetup = {0};

      swDeciSetup.bitRate = hInstance->hConfig->bitRate;
      swDeciSetup.nChannelsIn = hInstance->hConfig->nInChannels;
      swDeciSetup.samplingRateIn = hInstance->hConfig->sampleRateIn;
      swDeciSetup.bDelayAway = 0;
      swDeciSetup.granularity = 1024;

      if (hInstance->hConfig->lowDelaySwitching) {
        swDeciSetup.bLowDelay = 11;
      }

      errorInfo = iisSwitchingDecisionOpen(&hInstance->switchingDecision.hSwDeci, &swDeciSetup);

      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
      }
      if (errorInfo) freeErrorTraceback(errorInfo);
    }
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
      if (hInstance->hConfig->configSet != CONFIG_SET_DRM_30 && hInstance->hConfig->configSet != CONFIG_SET_DRM_PLUS) {
        retValue = iisxHEAACEncLib_extentsionData_usacSetUse(&hInstance->extEleList, XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL, 1, 0, NULL, 0, 0);
      }
      if (!isError(retValue)) {
        if (hInstance->hConfig->streamID >= 0) {
          unsigned char configExtension[IISXHEAACENCLIB_STREAM_ID_EXTCONF_SIZE];
          int i = 0;
          if (hInstance->hConfig->streamID <= IISXHEAACENCLIB_MAX_STREAM_ID) {
            for (i = 0; i < IISXHEAACENCLIB_STREAM_ID_EXTCONF_SIZE; i++) {
              configExtension[IISXHEAACENCLIB_STREAM_ID_EXTCONF_SIZE - 1 - i] = (char)((hInstance->hConfig->streamID % (1 << 8 * (i + 1))) / (1 << 8 * i));
            }
            retValue = iisxHEAACEncLib_extensionData_SetUseConfigExtension(&hInstance->configExtensionList, XHEAACENCLIB_ID_CONFIG_EXT_STREAM_ID, configExtension, 2);
          } else {
            retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    unsigned char *pExtensionConfig = NULL;
    unsigned char *pConfigExtension = NULL;
    int extensionConfigLength = 0;
    int configExtensionLength = 0;

    if (hInstance->hConfig->bDisableLoudness) {
      if (hInstance->hConfig->bLoudnessLevelSet != 0 || hInstance->hConfig->bAnchorLoudnessLevelSet != 0) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    } else {
      if ((hInstance->hConfig->bLoudnessLevelSet == 0 && hInstance->hConfig->bAnchorLoudnessLevelSet == 0 && hInstance->hConfig->bLiveLoudnessLevelSet == 0) && hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue) && (hInstance->hConfig->bLoudnessLevelSet || hInstance->hConfig->bAnchorLoudnessLevelSet || hInstance->hConfig->bLiveLoudnessLevelSet)) {
      {
        float loudnessToWrite = hInstance->hConfig->loudnessLevel;
        float anchorLoudnessToWrite = hInstance->hConfig->anchorLoudnessLevel;
        float samplePeakToWrite = hInstance->hConfig->samplePeak;
        if (hInstance->hConfig->bQuietLoudnessThresholdSet) {
          loudnessToWrite = max(hInstance->hConfig->loudnessLevel, hInstance->hConfig->quietLoudnessThreshold);
          anchorLoudnessToWrite = max(hInstance->hConfig->anchorLoudnessLevel, hInstance->hConfig->quietLoudnessThreshold);
          samplePeakToWrite = max(hInstance->hConfig->samplePeak, IISXHEAACENCLIB_SAMPLE_PEAK_MIN);
        }

        if (hInstance->hConfig->bLiveLoudnessLevelSet) {
          hInstance->setupDrcLoudness.bLoudnessLevelPresent = hInstance->hConfig->bLiveLoudnessLevelSet;
          hInstance->setupDrcLoudness.loudnessLevel = hInstance->hConfig->liveLoudnessLevel;
        } else {
          hInstance->setupDrcLoudness.bLoudnessLevelPresent = hInstance->hConfig->bLoudnessLevelSet;
          hInstance->setupDrcLoudness.loudnessLevel = loudnessToWrite;
          hInstance->setupDrcLoudness.bAnchorLoudnessLevelPresent = hInstance->hConfig->bAnchorLoudnessLevelSet;
          hInstance->setupDrcLoudness.anchorLoudnessLevel = anchorLoudnessToWrite;
        }
        if (hInstance->hConfig->bLiveSamplePeakSet) {
          hInstance->setupDrcLoudness.samplePeakPresent = hInstance->hConfig->bLiveSamplePeakSet;
          hInstance->setupDrcLoudness.samplePeak = hInstance->hConfig->liveSamplePeak;
        } else {
          hInstance->setupDrcLoudness.samplePeakPresent = hInstance->hConfig->samplePeakPresent;
          hInstance->setupDrcLoudness.samplePeak = samplePeakToWrite;
        }

        hInstance->setupDrcLoudness.bAlbumLoudnessLevelPresent = hInstance->hConfig->bAlbumLoudnessLevelSet;
        hInstance->setupDrcLoudness.albumLoudnessLevel = hInstance->hConfig->albumLoudnessLevel;

        if (!isError(retValue)) {
          retValueDrcLoudness = iisxHEAACEncLib_drc_loudness_ifc_init(hInstance->drcAndLoudness.hDrcLoudness,
                                                                      hInstance->setupDrcLoudness, &hInstance->loudnessInfoSet);

          if (retValueDrcLoudness != DRC_LOUDNESS_IFC_NO_ERROR) {
            retValue = XHEAACENCLIB_RETURN_ERROR_DRC;
          }
        }
      }

      if (!isError(retValue)) {
        retValueDrcLoudness = iisxHEAACEncLib_GetLoudnessInfoSet(hInstance->drcAndLoudness.hDrcLoudness, &pConfigExtension, &configExtensionLength);
        if (retValueDrcLoudness != DRC_LOUDNESS_IFC_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_DRC;
        }
      }

      if (!isError(retValue)) {
        if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
          retValue = iisxHEAACEncLib_extensionData_SetUseConfigExtension(&hInstance->configExtensionList, XHEAACENCLIB_ID_CONFIG_EXT_LOUDNESS_INFO, pConfigExtension, configExtensionLength);
        }
      } else {
      }
    }

    if (!isError(retValue)) {
      if (((hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF) && (hInstance->hConfig->bLoudnessLevelSet == 0 && hInstance->hConfig->bAnchorLoudnessLevelSet == 0)) && (hInstance->hConfig->bLiveLoudnessLevelSet == 0)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue)) {
      if (hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF) {
        if (!isError(retValue)) {
          retValue = iisxHEAACEncLibDrcFillConfiguration(&(hInstance->drcAndLoudness.setupDrc), hInstance->hConfig, &hInstance->drcExternalNodes);
        }

        if (!isError(retValue)) {
          retValueDrc = iisxHEAACEncLib_drc_ifc_init(hInstance->drcAndLoudness.hDrc,
                                                     hInstance->drcAndLoudness.setupDrc, hInstance->loudnessInfoSet);

          if (retValueDrc != DRC_IFC_NO_ERROR) {
            retValue = errorMapping_drc(retValueDrc);
          }
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLib_GetUniDrcConfig(hInstance->drcAndLoudness.hDrc, &pExtensionConfig, &extensionConfigLength);
          if (isError(retValue)) {
            printErrorConsole(CDI, "Could not fetch DRC config");
          }
        }

        if (!isError(retValue)) {
          if (hInstance->hConfig->aot == AUD_OBJ_TYP_USAC) {
            unsigned int defaultLengthUniDrc = IISXHEAACENCLIB_DEFAULT_LENGTH_UNIDRC;
            retValue = iisxHEAACEncLib_extentsionData_usacSetUse(&hInstance->extEleList, XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC, 1, defaultLengthUniDrc, pExtensionConfig, extensionConfigLength, 0);
          } else {
            retValue = iisxHEAACEncLib_extensionData_generalSetUse(&hInstance->extEleList, hInstance->hConfig, XHEAACENCLIB_EXT_ELE_AAC_UNI_DRC, IISXHEAACENCLIB_DEFAULT_LENGTH_UNIDRC, -1);
          }
        } else {
          printErrorConsole(CDI, "DRC Config failed");
        }
      }
    }
  }

  if (!isError(retValue)) {
    retValue = openCoreCoders(hInstance);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_audioPreRoll_configure(hInstance->hConfig,
                                                      &hInstance->audioPreRoll,
                                                      hInstance->mpeg4DelayParameter.mpeg4StandDelay,
                                                      &hInstance->timeSignal.minOutBufSize);
  }

  if (!isError(retValue)) {
    if (hInstance->switchingDecision.hSwDeci != NULL) {
      HANDLE_ERROR_INFO errorInfo;
      errorInfo = iisSwitchingDecisionInit(hInstance->switchingDecision.hSwDeci, &(hInstance->switchingDecision.nSwSamplesNextFrame));
      hInstance->switchingDecision.nSwSamplesNextFrame = max(hInstance->switchingDecision.nSwSamplesNextFrame, 0);
      if (errorInfo == noError) {
        errorInfo = iisSwitchingDecisionGetInfo(hInstance->switchingDecision.hSwDeci, &hInstance->switchingDecision.swInfo);
      }

      if ((errorInfo == noError) && (hInstance->switchingDecision.swInfo.bufferSize > 0)) {
        hInstance->timeSignal.gnSamplesNext = max((int)(hInstance->timeSignal.gnSamplesNext), (int)(hInstance->switchingDecision.nSwSamplesNextFrame * hInstance->hConfig->nInChannels));

        if (hInstance->switchingDecision.swInfo.bDelayAway == 0) {
          hInstance->switchingDecision.nDelaySwDeci = (int)((float)hInstance->switchingDecision.swInfo.bufferWriteOffset *
                                                            hInstance->hConfig->sampleRateOut /
                                                            hInstance->hConfig->sampleRateIn);
        }
      }

      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
      }
      if (errorInfo) freeErrorTraceback(errorInfo);
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_delayCalculationAndBufferCreation(
        hInstance->hConfig,
        hInstance->drcAndLoudness.hDrc,
        hInstance->hLiveLoudness,
        &hInstance->switchingDecision,
        &hInstance->hCoreDelayBuffer,
        &hInstance->collectiveBuffer,
        &hInstance->encBufferData,
        &hInstance->drcDelay,
        &hInstance->mpeg4DelayParameter,
        &hInstance->timeSignal,
        &hInstance->nTrashAUs);
  }

  if (!isError(retValue)) {
    if ((hInstance->audioPreRoll.bitResMode == XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL) && hInstance->audioPreRoll.hAudioPreRoll != NULL && hInstance->hConfig->aot == AUD_OBJ_TYP_USAC && (hInstance->hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE || hInstance->hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SEEKABLE)) {
      int auSizeWithoutAU = iisAudioPreRollLibPayloadSizeWithoutAUs(hInstance->audioPreRoll.hAudioPreRoll);
      int numberAUinIPF = iisAudioPreRollLibGetnPreRollAu(hInstance->audioPreRoll.hAudioPreRoll);
      int raIntCoreSampleRate = 0;
      float preRollAUFactor = 1.2f * ((float)numberAUinIPF);

      if (!isError(retValue)) {
        if (hInstance->hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL) {
          raIntCoreSampleRate = (hInstance->hConfig->randomAccessIntervalSamples * hInstance->hConfig->sbrRatio.downFac) / hInstance->hConfig->sbrRatio.upFac;

        } else if (hInstance->hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) {
          raIntCoreSampleRate = (hInstance->hConfig->randomAccessIntervalMin * hInstance->hConfig->sbrRatio.downFac) / hInstance->hConfig->sbrRatio.upFac;

        } else {
          assert(0);
          retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
        }
      }

      hInstance->ebrParams.preRollAUFactor = preRollAUFactor;
      hInstance->ebrParams.nBitsAuPreRoll = auSizeWithoutAU;
      hInstance->ebrParams.rapIntCoreSampleRate = raIntCoreSampleRate;

      if (!isError(retValue)) {
        if (!isVbr(hInstance->hConfig->bitrateMode)) {
          retValue = iisxHEAACEncLibAacEncUpdateExtendedBitReservoir(hInstance->hAacEnc, auSizeWithoutAU, raIntCoreSampleRate, preRollAUFactor);
        }
      }
    }
  }

  if (!isError(retValue)) {
    iisxHEAACEncLib_lraControl_drcGainInit(hInstance->hConfig,
                                           &hInstance->lraControlDrcGainData,
                                           hInstance->drcDelay.drcEncoderDelay,
                                           hInstance->drcDelay.drcDecoderDelay,
                                           hInstance->drcDelay.drcLookAhead,
                                           hInstance->mpeg4DelayParameter.mpeg4Priming,
                                           hInstance->nTrashAUs);
  }

  if (!isError(retValue)) {
    int usacIndepDelay = 0;
    if (hInstance->hMpegsEnc != NULL) {
      usacIndepDelay = iisxHEAACEncLibMpegsEncGetMaxDelay(hInstance->hMpegsEnc);
      usacIndepDelay = max(usacIndepDelay - 1, 0);
    }
    retValue = iisxHEAACEncLib_syncframe_setup(hInstance->hUsacIndepFlag,
                                               hInstance->hConfig,
                                               hInstance->hSbrEnc,
                                               hInstance->audioPreRoll.hAudioPreRoll,
                                               &hInstance->mpeg4DelayParameter,
                                               hInstance->hLoaswriter,
                                               hInstance->nTrashAUs,
                                               &hInstance->rapFrameInAdvance,
                                               usacIndepDelay);
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_BITDISTRIBUTION_CONFIG bitDistributionConfig;
    XHEAACENCLIB_BITDISTRIBUTION_CONFIGELEMENT configElement[XHEAACENCLIB_MAX_BITDISTRIBUTION];
    bitDistributionConfig.configElement = configElement;

    retValue = iisxHEAACEncLib_bitDistribution_FillConfig(hInstance->hConfig,
                                                          &hInstance->bitDistribution,
                                                          hInstance->nTrashAUs,
                                                          hInstance->hMpegsEnc,
                                                          hInstance->hSbrEnc,
                                                          hInstance->hLoaswriter,
                                                          hInstance->drcAndLoudness.hDrc,
                                                          hInstance->audioPreRoll.hAudioPreRoll,
                                                          &bitDistributionConfig);
    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_bitDistribution_New(&(hInstance->bitDistribution.hBitDistribution), &bitDistributionConfig);
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_SubmitDrcMetadata(
    XHEAACENCLIB_INSTANCE_HANDLE const hInstance,
    float const *const drcGains,
    unsigned int const drcGainsLength,
    unsigned int const drcExternalNodeNumNodes,
    int const *const drcExternalNodeLevels,
    int const *const drcExternalNodeGains,
    float const *const drcGainOffset,
    unsigned int const drcGainOffsetLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if ((NULL == hInstance) || (NULL == drcGains) || (NULL == drcExternalNodeLevels) || (NULL == drcExternalNodeGains) || (NULL == drcGainOffset)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_submitDrcMetadata(&(hInstance->lraControlDrcGainData),
                                                 &(hInstance->drcExternalNodes),
                                                 drcGains,
                                                 drcGainsLength,
                                                 drcExternalNodeNumNodes,
                                                 drcExternalNodeLevels,
                                                 drcExternalNodeGains,
                                                 drcGainOffset,
                                                 drcGainOffsetLength);
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Submit(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_DATA const type,
    PARAM_FORMAT const dataFormat,
    const void *const pData,
    int const dataSize) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance != NULL) {
    switch (type) {
      case XHEAACENCLIB_DATA_ANCILLARY:
        assert(dataFormat == PARAM_CHAR_ARRAY);
        hInstance->pAncBytes = *(unsigned char *const *)pData;
        hInstance->numAncBytes = dataSize;
        break;
      case XHEAACENCLIB_DATA_RAP_IN_X_SAMPLES:

        assert(dataFormat == PARAM_INT);
        retValue = iisxHEAACEncLib_rapOnDemand_setup(hInstance->hConfig,
                                                     hInstance->hUsacIndepFlag,
                                                     hInstance->hCodecParamList,
                                                     &hInstance->audioPreRoll,
                                                     &hInstance->ebrParams,
                                                     hInstance->hAacEnc,
                                                     hInstance->hLoaswriter,
                                                     &hInstance->warningList,
                                                     hInstance->nTrashAUs,
                                                     hInstance->lastRapFrameDist,
                                                     *((const unsigned int *)pData));
        break;

      case XHEAACENCLIB_DATA_INVALID:
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
        break;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetAuInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_AUINFOLIST_HANDLE *phAuInfoList) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL || phAuInfoList == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    (*phAuInfoList) = hInstance->hAuInfoList;
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetEncoderState(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_ENCODER_STATE *encoderState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (!hInstance || !encoderState) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *encoderState = hInstance->encoderState;
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetAscInfo(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    XHEAACENCLIB_ASCINFO_HANDLE hAscInfo) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance != NULL && hAscInfo != NULL) {
    assert(hInstance->asc.nAscSizeBits >> 3 <= XHEAACENCLIB_MAX_ASC_SIZE);

    memset(hAscInfo, 0, sizeof(XHEAACENCLIB_ASCINFO));
    memcpy(hAscInfo->ascBuffer, hInstance->asc.pAsc, XHEAACENCLIB_MAX_ASC_SIZE * sizeof(unsigned char));
    memcpy(&(hAscInfo->ascSizeBits), &(hInstance->asc.nAscSizeBits), sizeof(unsigned int));
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_UpdateParamList(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_updateParamList(hInstance->hConfig,
                                               &hInstance->audioPreRoll,
                                               hInstance->hAacEnc,
                                               hInstance->hSigMap,
                                               hInstance->hSbrEnc,
                                               hInstance->hCodecParamList,
                                               &hInstance->bitReservoirData,
                                               &hInstance->mpeg4DelayParameter,
                                               &hInstance->timeSignal,
                                               hInstance->aacCoreBandwidth,
                                               hInstance->rapFrameInAdvance,
                                               hInstance->numAncBytes);
  }
  return retValue;
}

static int feedInput(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    const float *const pSamples,
    const int nSamples) {
  int samplesFed = 0;

  if (nSamples != 0) {
    int samplesToFeed = 0;

    samplesToFeed = hInstance->timeSignal.nSamplesNext - hInstance->timeSignal.nSamplesValid;
    samplesToFeed = min(nSamples, samplesToFeed);

    copyFLOAT(pSamples, (hInstance->timeSignal.pPreResamplerIn + hInstance->timeSignal.nSamplesValid), samplesToFeed);

    hInstance->timeSignal.nSamplesValid += samplesToFeed;
    samplesFed = samplesToFeed;
    hInstance->timeSignal.nSamplesValidInAU = hInstance->hConfig->nFrameSamples;
  }
  if (hInstance->timeSignal.nSamplesValid < hInstance->timeSignal.nSamplesNext) {
    int zeroesToFeed = hInstance->timeSignal.nSamplesNext - hInstance->timeSignal.nSamplesValid;

    if (hInstance->timeSignalFlushing.encBufEmpty == 0) {
      int additionalSyncFlushing = hInstance->hConfig->additionalSyncFlushing;
      hInstance->timeSignalFlushing.encBufEmpty = 1;

      hInstance->timeSignalFlushing.samplesToFlush = hInstance->mpeg4DelayParameter.mpeg4DelayWOSwitching + hInstance->mpeg4DelayParameter.mpeg4AddFlushing;
      additionalSyncFlushing -= hInstance->mpeg4DelayParameter.mpeg4AddFlushing;
      if (additionalSyncFlushing > 0) {
        hInstance->timeSignalFlushing.samplesToFlush += additionalSyncFlushing;
      }

      assert(hInstance->hConfig->additionalSyncFlushing <= 0 || additionalSyncFlushing >= 0);
      hInstance->timeSignalFlushing.outSamplesStillValid =
          ((hInstance->timeSignal.nSamplesValid * hInstance->hConfig->sampleRateOut / hInstance->hConfig->nInChannels) + hInstance->hConfig->sampleRateIn - 1) /
          hInstance->hConfig->sampleRateIn;
      hInstance->timeSignalFlushing.outSamplesStillValid += hInstance->mpeg4DelayParameter.mpeg4DelayWOSwitching;

      hInstance->timeSignalFlushing.samplesToFlush =
          (int)((float)hInstance->timeSignalFlushing.samplesToFlush *
                ((float)hInstance->hConfig->sampleRateIn / (float)hInstance->hConfig->sampleRateOut));
      hInstance->timeSignalFlushing.samplesToFlush *= hInstance->hConfig->nInChannels;
    }

    if (hInstance->timeSignalFlushing.samplesToFlush > 0) {
      setFLOAT(0.0f,
               hInstance->timeSignal.pPreResamplerIn + hInstance->timeSignal.nSamplesValid,
               zeroesToFeed);
      hInstance->timeSignalFlushing.samplesToFlush -= zeroesToFeed;
      hInstance->timeSignal.nSamplesValid += zeroesToFeed;
      samplesFed += zeroesToFeed;
      hInstance->timeSignal.nSamplesValidInAU = (unsigned int)max(0, min((int)hInstance->hConfig->nFrameSamples, hInstance->timeSignalFlushing.outSamplesStillValid));
      hInstance->timeSignalFlushing.outSamplesStillValid -= (int)hInstance->hConfig->nFrameSamples;
    }
  }

  return samplesFed;
}

static XHEAACENCLIB_RETURN encodeFrame(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    const int nSamples,
    unsigned int *const pSamplesNext,
    unsigned char *const pOutput,
    const int nOutputBufSize,
    int *const pOutputBits,
    XHEAAC_AACENC_IPF_STATE const ipfState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  unsigned int nSamplesPreResampOut = 0;

  unsigned int nSamplesCoreDownSampOut = 0;

  unsigned int mpegsPayloadLength[10] = {0};
  float *pAacCoreInTmp = NULL;

  DYNAMIC_DATA_EXTENSION dynamicDataExt;
  XHEAACENCLIB_HANDLE_UNISTE *phUniSte = iisxHEAACEncLibMpegsEncGetUnifiedStereoHandle(hInstance->hMpegsEnc);
  ENCODEFRAME_BITSTREAMS bitStreams = {0};
  ENCODEFRAME_PREROLLAUBUFFER preRollAuBufer = {0};

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_encodeFrame_ifc_preResampler(hInstance->hConfig,
                                                            hInstance->hPreResampler,
                                                            &hInstance->timeSignal,
                                                            nSamples,
                                                            pSamplesNext,
                                                            &nSamplesPreResampOut);
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF || hInstance->hConfig->bMpeg4DrcOn) {
      float *drcIn = NULL;

      {
        retValue = iisxHEAACEncLib_encodeFrame_ifc_feedDrcBuffers(hInstance->hConfig,
                                                                  hInstance->hCoreDelayBuffer,
                                                                  &hInstance->timeSignal,
                                                                  &hInstance->drcDelay,
                                                                  nSamplesPreResampOut,
                                                                  &drcIn);
      }

      if (!isError(retValue) && (hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF)) {
        retValue = iisxHEAACEncLib_encodeFrame_ifc_drcProcess(hInstance->hConfig,
                                                              hInstance->audioPreRoll.hAudioPreRoll,
                                                              hInstance->hUsacIndepFlag,
                                                              hInstance->drcAndLoudness.hDrc,
                                                              &dynamicDataExt,
                                                              drcIn,
                                                              &hInstance->extEleList,
                                                              &hInstance->bitDistribution);
      }

    } else {
      hInstance->timeSignal.pPreResamplerOutDelayed = hInstance->timeSignal.pPreResamplerOut;
    }
  }

  if (!isError(retValue) && (hInstance->hConfig->stereoConfigIndex > 0)) {
    int coreCoderFrameLength = 0;
    int speechFlag = 0;

    if (hInstance->switchingDecision.hSwDeci) {
      IIS_SWDECI_RESULT dec = IIS_SWDECI_RESULT_INVALID;
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = iisSwitchingDecisionGetNextDecision(hInstance->switchingDecision.hSwDeci,
                                                      SWDECI_ID_MPEGS_212,
                                                      &dec);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
      }

      speechFlag = (dec == IIS_SWDECI_RESULT_SPEECH) ? 1 : 0;
    }

    if (hInstance->hConfig->granuleLength == 768) {
      coreCoderFrameLength = 2048;
    } else {
      coreCoderFrameLength = hInstance->hConfig->granuleLength;
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_encodeFrame_ifc_mps(hInstance->hConfig,
                                                     hInstance->hMpegsEnc,
                                                     hInstance->mpsPayloadList.hExtPayload,
                                                     &hInstance->bitDistribution,
                                                     coreCoderFrameLength,
                                                     hInstance->hUsacIndepFlag,
                                                     speechFlag,
                                                     hInstance->timeSignal.pPreResamplerOutDelayed,
                                                     (int *)&nSamplesPreResampOut,
                                                     hInstance->ppQmfSamplesReal,
                                                     hInstance->ppQmfSamplesImag,
                                                     hInstance->pPreResamplerOutLr);
    }
  }

  if (!isError(retValue) && hInstance->hSbrEnc) {
    int isSyncFrame = 0;
    float *pPreSamplerOut = NULL;

    retValue = iisxHEAACEncLib_encodeFrame_ifc_sbrSyncframe(hInstance->hConfig, hInstance->hUsacIndepFlag, hInstance->hSbrEnc, &isSyncFrame);

    if (phUniSte && hInstance->hConfig->stereoConfigIndex == 3) {
      pPreSamplerOut = hInstance->pPreResamplerOutLr;
    } else {
      pPreSamplerOut = hInstance->timeSignal.pPreResamplerOutDelayed;
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibSbrEncEncode(hInstance->hSbrEnc,
                                             pPreSamplerOut,
                                             nSamplesPreResampOut,
                                             hInstance->pQmfDownSamplerOut,
                                             &nSamplesCoreDownSampOut,
                                             hInstance->switchingDecision.hSwDeci,
                                             isSyncFrame);
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_encodeFrame_ifc_sbrbitDistribution(hInstance->hConfig, hInstance->hSbrEnc,
                                                                    hInstance->sbrPayloadList.hExtPayload, &hInstance->bitDistribution);
    }
  }

  if (!isError(retValue) && hInstance->audioPreRoll.hAudioPreRoll != NULL) {
    retValue = iisxHEAACEncLib_audioPreRoll_writeExtPayload(&hInstance->audioPreRoll,
                                                            &hInstance->bitDistribution,
                                                            hInstance->hUsacIndepFlag,
                                                            &hInstance->extEleList);
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->bUseQmfResampler) {
      if (phUniSte && hInstance->hConfig->stereoConfigIndex == 3) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
      }
      pAacCoreInTmp = hInstance->pQmfDownSamplerOut;
    } else {
      retValue = iisxHEAACEncLib_encodeFrame_ifc_coreResampler(hInstance->hConfig,
                                                               hInstance->hCoreDownSampler,
                                                               &hInstance->timeSignal,
                                                               hInstance->pCoreDownSamplerIn,
                                                               nSamplesPreResampOut,
                                                               &nSamplesCoreDownSampOut,
                                                               &pAacCoreInTmp);
    }
  }
  if (!isError(retValue)) {
    if (hInstance->audioPreRoll.hAudioPreRoll) {
      retValue = iisxHEAACEncLibGetAudioPreRollBuffer(&hInstance->audioPreRoll, &preRollAuBufer, hInstance->timeSignal.minOutBufSize);
    }
  }

  if (!isError(retValue) && (hInstance->hAacEnc != NULL)) {
    if (nSamplesCoreDownSampOut > 0) {
      unsigned int i = 0;

      int subFrame = 0;
      HANDLE_EXTPAYLOAD_CONTAINER extContainer[USAC_MAX_EXTENSION_CONFIGS];
      int coreModeNext[SIGMAP_MAX_SIGNALS] = {0};

      unsigned int transportOverheadTotal = ((mpegsPayloadLength[subFrame] > 0) && (hInstance->hConfig->mpegsPayloadMode == MPEGS_NO_PAYLOAD_EMBED)) ? (8 * mpegsPayloadLength[subFrame]) : 0;
      unsigned int transportOverheadProcessed = 0;

      for (i = 0; i < nSamplesCoreDownSampOut / (hInstance->hConfig->granuleLength * hInstance->hConfig->nChannelsCoreCoder) && !isError(retValue); i++) {
        unsigned int transportOverhead = 0;
        int isSyncFrame = 0;

        if (i >= 1) {
          retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
        }

        if (!isError(retValue)) {
          if (i < nSamplesCoreDownSampOut / (hInstance->hConfig->granuleLength * hInstance->hConfig->nChannelsCoreCoder) - 1) {
            transportOverhead = transportOverheadTotal / (nSamplesCoreDownSampOut / (hInstance->hConfig->granuleLength * hInstance->hConfig->nChannelsCoreCoder));
          } else {
            transportOverhead = transportOverheadTotal - transportOverheadProcessed;
          }
          transportOverheadProcessed += transportOverhead;
        }

        if (!isError(retValue)) {
          unsigned int mpegsEncExist = 0;
          if (hInstance->hMpegsEnc) {
            mpegsEncExist = 1;
          }
          retValue = iisxHEAACEncLib_encodeFrame_ifc_aacCoreCollectPointers(hInstance->hConfig, &hInstance->extEleList,
                                                                            &hInstance->sbrPayloadList, &hInstance->mpsPayloadList, extContainer, mpegsEncExist);
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLib_encodeFrame_ifc_aacCoreSwDeci(hInstance->hConfig, hInstance->switchingDecision.hSwDeci, coreModeNext);
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLib_encodeFrame_ifc_aacCoreSAP(hInstance->hConfig, hInstance->hAacEnc, hInstance->hUsacIndepFlag, coreModeNext);
        }

        if (hInstance->hConfig->aot != AUD_OBJ_TYP_HEAAC || !hInstance->hConfig->bUseHBE) {
          memcpy(hInstance->timeSignal.pAacFrameCoreDelayInBuffer, pAacCoreInTmp, sizeof(float) * nSamplesCoreDownSampOut);
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLibAacEncEncode(hInstance->hAacEnc,
                                                 hInstance->timeSignal.pAacFrameCoreDelayInBuffer,
                                                 nSamplesCoreDownSampOut,
                                                 pOutput,
                                                 preRollAuBufer.pPreRollAUBuffer,
                                                 nOutputBufSize,
                                                 pOutputBits,
                                                 preRollAuBufer.pPreRollAUBufferBits,
                                                 bitStreams.pAncDrcBitstream,
                                                 bitStreams.nAncDrcBits / 8,
                                                 coreModeNext,
                                                 phUniSte,
                                                 hInstance->hConfig->TLOverheadPerAU,
                                                 isSyncFrame,
                                                 extContainer,
                                                 hInstance->extEleList.numExtEle,
                                                 ipfState,
                                                 hInstance->hConfig->nChannelsCoreCoder);
        }

        if (hInstance->hConfig->aot == AUD_OBJ_TYP_HEAAC && hInstance->hConfig->bUseHBE) {
          memcpy(hInstance->timeSignal.pAacFrameCoreDelayInBuffer, pAacCoreInTmp, sizeof(float) * nSamplesCoreDownSampOut);
        }

        if (!isError(retValue)) {
          retValue = iisxHEAACEncLib_encodeFrame_ifc_aacCoreResetSbr(hInstance->hConfig, hInstance->hAacEnc, &hInstance->extEleList,
                                                                     &hInstance->sbrPayloadList, &hInstance->mpsPayloadList, &hInstance->bitReservoirData);
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (hInstance->hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF || hInstance->hConfig->bMpeg4DrcOn) {
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = MP4TIMEBUF_InvalidateBuffer(hInstance->hCoreDelayBuffer, hInstance->hConfig->nFrameSamples);
      if (errorInfo == noError && hInstance->drcDelay.drcCompensationDelay > 0) {
        errorInfo = MP4TIMEBUF_InvalidateBuffer(hInstance->drcDelay.hDrcLookaheadBuffer, hInstance->hConfig->nFrameSamples);
      }
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
      }
    }
  }

  if (!isError(retValue) && hInstance->audioPreRoll.hAudioPreRoll != NULL) {
    retValue = iisxHEAACEncLib_encodeFrame_ifc_aacCorePostEncode(hInstance->hConfig, hInstance->hAacEnc, &hInstance->audioPreRoll, &hInstance->ebrParams, ipfState);
  }
  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_SetMinAuBytes(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    int minAuBytes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibAacEncSetMinAuBytes(hInstance->hAacEnc, minAuBytes);
  }

  return retValue;
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_Encode(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    const float *const pSamples,
    const unsigned int nSamples,
    unsigned char *const pOutput,
    int *const pOutputBytes,
    const unsigned int outputBufSizeBytes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAAC_AACENC_IPF_STATE ipfState = XHEAAC_AACENC_IPF_STATE_NO;

  int saveOutputBytes = 0;
  int nSamplesLeft = 0;
  float *pBufferToUse = NULL;
  int outBufSize = 0;

  unsigned int nSamplesNext = 0;
  int nSamplesEnc = 0;

  int bLiveLoudnessMeasurement = 0;

  if (hInstance == NULL || (pSamples == NULL && nSamples > 0) || pOutput == NULL || pOutputBytes == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (outputBufSizeBytes >= hInstance->timeSignal.minOutBufSize) {
      outBufSize = hInstance->timeSignal.minOutBufSize;
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_BUFFER_SIZE;
    }
  }

  if (!isError(retValue)) {
    if (nSamples % hInstance->hConfig->nInChannels) {
      retValue = XHEAACENCLIB_RETURN_ERROR_nSAMPLES_NOT_MULTIPLE_nIP_CHANNELS;
    }
  }

  if (!isError(retValue)) {
    if (nSamples > hInstance->timeSignal.nSamplesMax) {
      retValue = XHEAACENCLIB_RETURN_ERROR_TOO_MANY_INPUT_SAMPLES;
    }
  }

  if (!isError(retValue)) {
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
      if (nSamples > 0) {
        retValue = XHEAACENCLIB_RETURN_ERROR_UNEXPECTED_INPUT_SAMPLES;
      }
    }
  }

  if (!isError(retValue)) {
    *pOutputBytes = 0;
    hInstance->auInfo.nAccessUnits = 0;
  }

  if (!isError(retValue)) {
    IIS_FPUcontrol_SetDenormal_FTZ_AZ(hInstance->hFpuCtrl);
  }

  if (!isError(retValue)) {
    if (nSamples > 0) {
      retValue = iisxHEAACEncLib_collectiveBuffer_feedSamples(hInstance->hConfig, hInstance->collectiveBuffer, pSamples, nSamples);
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_encoderState_checkAndAdapt(&hInstance->encoderState, nSamples, hInstance->timeSignal.nSamplesUntilNext);
  }

  if (!isError(retValue)) {
    if ((hInstance->hConfig->primingMode == XHEAACENC_PRIMINGMODE_FULL) &&
        hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING &&
        hInstance->nSamplesLeftToFlushFromLeveler > 0) {
      retValue = iisxHEAACEncLibLiveLoudnessPrepareFlushing(hInstance);
    }
  }

  if (hInstance->hConfig->bRealtimeLRAC) {
    bLiveLoudnessMeasurement = 1;
  }
  if (hInstance->hConfig->bLiveLoudnessLevelSet) {
    bLiveLoudnessMeasurement = 1;
  }

  if (!isError(retValue)) {
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_ENCODING || hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
      int nSamplesAvailableForNextAu = 0;

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLib_collectiveBuffer_getNumAvailableSamples(hInstance->collectiveBuffer,
                                                                           hInstance->hConfig->nInChannels,
                                                                           &nSamplesAvailableForNextAu);
      }

      if (!isError(retValue) && hInstance->hConfig->bLiveLoudnessLevelSet) {
        retValue = iisxHEAACEncLibLiveLoudnessProcessing(hInstance);
      }

      if (!isError(retValue) && hInstance->hConfig->bLiveLoudnessLevelSet) {
        int compensateLiveLoudnessLevelingDelay = 1;
        if (hInstance->hConfig->primingMode == XHEAACENC_PRIMINGMODE_FULL) {
          compensateLiveLoudnessLevelingDelay = 0;
        }
        if (compensateLiveLoudnessLevelingDelay == 1) {
          retValue = iisxHEAACEncLibLiveLoudnessUpdateAvailableSamples(hInstance, hInstance->hConfig->nInChannels, &nSamplesAvailableForNextAu);
        }
      }

      if (bLiveLoudnessMeasurement) {
        float *loudnessBuffer = NULL;
        LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;

        if (!isError(retValue)) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = MP4TIMEBUF_SaveAccessBuffer(hInstance->collectiveBuffer, 0, nSamplesAvailableForNextAu / hInstance->hConfig->nInChannels, 0, &loudnessBuffer);
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SAVE_TIMEBUFFER;
          }
        }
        if (!isError(retValue)) {
          retValueLoudness = iisxHEAACEncLib_loudness_ifc_Process(hInstance->drcAndLoudness.hLoudness,
                                                                  loudnessBuffer,
                                                                  nSamplesAvailableForNextAu, hInstance->hConfig->bLiveLoudnessLevelSet);
          if (retValueLoudness != LOUDNESS_IFC_NO_ERROR) {
            retValue = XHEAACENCLIB_RETURN_ERROR_LOUDNESS_MEASUREMENT;
          }
        }
        if (!isError(retValue)) {
          if (hInstance->hConfig->bRealtimeLRAC) {
            float loudnessRange;
            unsigned int loudnessRangeCount;

            loudnessRange = iisxHEAACEncLib_loudness_ifc_getLoudnessRange(hInstance->drcAndLoudness.hLoudness);
            loudnessRangeCount = iisxHEAACEncLib_loudness_ifc_getLoudnessRangeCount(hInstance->drcAndLoudness.hLoudness);
            retValue = iisxHEAACEncLib_drc_ifc_set_loudness_range(hInstance->drcAndLoudness.hDrc, loudnessRange, loudnessRangeCount);
          }
        }
      }

      if (!isError(retValue)) {
        unsigned int nSamplesForNextValidAu = max(hInstance->timeSignal.gnSamplesNext, hInstance->hConfig->nInChannels * hInstance->timeSignal.nSamplesValidInAU);
        nSamplesLeft = min(nSamplesAvailableForNextAu, (int)min(nSamplesForNextValidAu, INT_MAX));
        do {
          int nBits = 0;

          if (!isError(retValue)) {
            retValue = iisxHEAACEncLib_syncFrame_startNextFrame(hInstance->hUsacIndepFlag);
            hInstance->smcWritten = NO_SMC_WRITTEN;
          }

          if (!isError(retValue)) {
            retValue = iisxHEAACEncLib_bitDistribution_PreEncode(hInstance->hConfig,
                                                                 hInstance->hUsacIndepFlag,
                                                                 &hInstance->bitDistribution,
                                                                 hInstance->hMpegsEnc,
                                                                 hInstance->hLoaswriter,
                                                                 hInstance->audioPreRoll.hAudioPreRoll,
                                                                 hInstance->audioPreRoll.bitResMode,
                                                                 hInstance->hAacEnc);
            if (isError(retValue)) {
            }
          }

          if (!isError(retValue)) {
            retValue = iisxHEAACEncLib_delayAndBuffer_fillEncBuffer(
                hInstance->hConfig,
                &hInstance->switchingDecision,
                hInstance->collectiveBuffer,
                &hInstance->encBufferData,
                hInstance->encoderState,
                &nSamplesLeft);
            if (isError(retValue)) {
            }
          }

          if (!isError(retValue)) {
            nSamplesEnc = min((int)hInstance->timeSignal.nSamplesRequired, hInstance->encBufferData.encBufValidSamples);
            if ((nSamplesEnc > 0) && (nSamplesEnc < (int)hInstance->timeSignal.nSamplesRequired) && (hInstance->encoderState != XHEAACENCLIB_ENCODER_STATE_FLUSHING)) {
              retValue = XHEAACENCLIB_RETURN_ERROR_ENC_NOT_FLUSHING_TOO_LESS_SAMPLES;
            }
          }

          if (!isError(retValue)) {
            HANDLE_ERROR_INFO errorInfo = MP4TIMEBUF_SaveAccessBuffer(hInstance->encBufferData.hEncBuffer, 0, (int)nSamplesEnc / hInstance->hConfig->nInChannels, 0, &pBufferToUse);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SAVE_TIMEBUFFER;
            }
          }

          if (!isError(retValue) && hInstance->hSbrEnc != NULL) {
            int isSyncFrame = 0;
            int sendSbrHeaderDelay = 0;
            iisxHEAACEncLibSbrEncSendHeaderDelay(hInstance->hSbrEnc, &sendSbrHeaderDelay);
            retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 1 + sendSbrHeaderDelay, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER);
            if (isSyncFrame && !isError(retValue)) {
              retValue = iisxHEAACEncLibSbrEncSetRightBorderFIX(hInstance->hSbrEnc, 1);
            }
          }

          if (!isError(retValue) && hInstance->audioPreRoll.hAudioPreRoll != NULL && !isError(retValue)) {
            int forceIndepFlag = 0;
            retValue = iisxHEAACEncLib_audioPreRoll_request(hInstance->hConfig, &hInstance->audioPreRoll, hInstance->hUsacIndepFlag, &hInstance->lastRapFrameDist, &forceIndepFlag, &ipfState);

            if (!isError(retValue) && forceIndepFlag != 0 && !isError(retValue)) {
              int tmp = 0;
              if (!isError(retValue)) {
                retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &tmp, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
              }
              if (!isError(retValue) && tmp == 0) {
                retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
              }

              if (hInstance->hConfig->useSbrFixBorderForIndepFlag != TOOL_MODE_OFF && hInstance->hConfig->bUseSBR) {
                if (!isError(retValue)) {
                  retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &tmp, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER);
                }
                if (!isError(retValue) && tmp == 0) {
                  retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
                }
              }
            }
          }

          if (!isError(retValue)) {
            int tmp = feedInput(hInstance, pBufferToUse, nSamplesEnc);
            if ((unsigned int)tmp != hInstance->timeSignal.nSamplesRequired && (tmp != 0 || hInstance->timeSignal.nSamplesValid != 0)) {
              retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
            }
          }

          if (!isError(retValue)) {
            if (hInstance->timeSignal.nSamplesNext == hInstance->timeSignal.nSamplesValid) {
              unsigned int i;

              for (i = 0; !isError(retValue) && i < hInstance->timeSignal.nSamplesValid; i++) {
                if (hInstance->timeSignal.pPreResamplerIn[i] * hInstance->timeSignal.pPreResamplerIn[i] > 64.0f) {
                  retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
                }
              }
              if (!isError(retValue)) {
                if (*pOutputBytes != 0) {
                  retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
                }
              }

              if (!isError(retValue)) {
                if ((hInstance->drcAndLoudness.hDrc) && (hInstance->lraControlDrcGainData.lraControlDrcProcessingIsActive)) {
                  retValue = iisxHEAACEncLib_lraControl_interpolateDrcGains(hInstance->hConfig->nFrameSamples, &hInstance->lraControlDrcGainData);

                  if (!isError(retValue)) {
                    retValue = iisxHEAACEncLib_drc_set_lra_control_drc_gains(hInstance->drcAndLoudness.hDrc,
                                                                             hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolated,
                                                                             hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolatedLength,
                                                                             hInstance->lraControlDrcGainData.lraControlDrcProcessingIsActive);
                  }

                  if (isError(retValue)) {
                    retValue = XHEAACENCLIB_RETURN_ERROR_LRACONTROL_DRC_GAIN;
                  }
                }
              }

              if (!isError(retValue)) {
                retValue = encodeFrame(hInstance,
                                       hInstance->timeSignal.nSamplesValid,
                                       &nSamplesNext,
                                       pOutput + *pOutputBytes,
                                       outBufSize - *pOutputBytes,
                                       &nBits,
                                       ipfState);

                if (isError(retValue)) {
                }
              }

              if (!isError(retValue)) {
                if (hInstance->nTrashAUs > 0) {
                  hInstance->nTrashAUs--;

                  if (hInstance->nTrashAUs == 0) {
                    if (!isVbr(hInstance->hConfig->bitrateMode)) {
                      retValue = iisxHEAACEncLibAacEncSetBitReservoirLevel(hInstance->hAacEnc, 1);
                    }
                  }
                } else {
                  if (!isError(retValue)) {
                    if ((nBits % 8) != 0) {
                      assert(0);
                    }

                    if ((nBits > MAX_AAC_CHANNEL_BITS * (int)hInstance->hConfig->nInChannels) &&
                        (hInstance->audioPreRoll.bitResMode != XHEAACENCLIB_APR_BITRESMODE_OUT)) {
                      assert(0);
                    }
                    saveOutputBytes = *pOutputBytes;
                    *pOutputBytes += nBits / 8;
                  }
                  if (!isError(retValue)) {
                    int AuSizeBeforeLoas = *pOutputBytes;
                    if ((hInstance->hConfig->transportFormat == TT_LOAS) || (hInstance->hConfig->transportFormat == TT_LATM) ||
                        (hInstance->hConfig->transportFormat == TT_LOAS_NOSMC) || (hInstance->hConfig->transportFormat == TT_LATM_NOSMC)) {
                      int loas_smc = 0;
                      if (!isError(retValue)) {
                        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hInstance->hUsacIndepFlag, 0, &loas_smc, XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC);
                      }
                      if (!isError(retValue)) {
                        if (hInstance->hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND && loas_smc != 0) {
                          retValue = iisxHEAACEncLibLoaswriter_triggerSmc(hInstance->hLoaswriter);
                        }
                      }
                      if (!isError(retValue)) {
                        retValue = iisxHEAACEncLibAdvance_loaswriter(hInstance->hLoaswriter,
                                                                     hInstance->hBb,
                                                                     pOutput,
                                                                     *pOutputBytes,
                                                                     pOutput,
                                                                     outBufSize,
                                                                     pOutputBytes,
                                                                     &hInstance->smcWritten);
                      }

                      if (!isError(retValue)) {
                        retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(hInstance->bitDistribution.hBitDistribution, hInstance->bitDistribution.elementID_LOAS, (*pOutputBytes - AuSizeBeforeLoas) * 8);
                      }

                      if ((loas_smc == 0 && hInstance->smcWritten == NO_SMC_WRITTEN) || (loas_smc == 1 && hInstance->smcWritten == SMC_WRITTEN)) {
                      } else {
                        assert(0);
                        retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
                      }
                    }
                  }

                  if (!isError(retValue)) {
                    hInstance->auInfo.pByteCnt[hInstance->auInfo.nAccessUnits] = (*pOutputBytes) - saveOutputBytes;
                    hInstance->auInfo.pNumValidSamples[hInstance->auInfo.nAccessUnits] = hInstance->timeSignal.nSamplesValidInAU;
                    hInstance->auInfo.nAccessUnits++;
                  }
                }
                if (!isError(retValue)) {
                  hInstance->timeSignal.nSamplesValid = 0;
                }
              }
            } else {
              nSamplesNext = hInstance->timeSignal.nSamplesNext - hInstance->timeSignal.nSamplesValid;
              hInstance->auInfo.pByteCnt[hInstance->auInfo.nAccessUnits] = 0;
              hInstance->auInfo.pNumValidSamples[hInstance->auInfo.nAccessUnits] = 0;
              hInstance->auInfo.nAccessUnits++;
            }
          }

          if (!isError(retValue)) {
            HANDLE_ERROR_INFO errorInfo = noError;
            errorInfo = MP4TIMEBUF_InvalidateBuffer(hInstance->encBufferData.hEncBuffer, nSamplesEnc / hInstance->hConfig->nInChannels);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
            }
          }

          if (!isError(retValue)) {
            if (hInstance->switchingDecision.hSwDeci != NULL) {
              hInstance->switchingDecision.nSwDeciBufferOffset -= (nSamplesEnc / hInstance->hConfig->nInChannels);
            }
          }

          if (!isError(retValue)) {
            int tmp = hInstance->encBufferData.encBufValidSamples - nSamplesEnc;
            HANDLE_ERROR_INFO errorInfo = noError;
            errorInfo = MP4TIMEBUF_getValidSamples(hInstance->encBufferData.hEncBuffer, &hInstance->encBufferData.encBufValidSamples);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
            }
            if (!isError(retValue)) {
              hInstance->encBufferData.encBufValidSamples *= hInstance->hConfig->nInChannels;
              if (tmp != hInstance->encBufferData.encBufValidSamples) {
                retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
              }
            }
          }

          if (!isError(retValue)) {
            hInstance->timeSignal.nSamplesRequired = nSamplesNext;
          }

          if (!isError(retValue)) {
            int bitsUsed = 0;
            retValue = iisxHEAACEncLib_bitDistribution_GetUsedBitsActFrame(hInstance->bitDistribution.hBitDistribution, &bitsUsed);
            if (isError(retValue)) {
            }
          }

          if (!isError(retValue)) {
            if (hInstance->switchingDecision.hSwDeci != NULL) {
              hInstance->switchingDecision.nSwSamplesNextFrame = iisSwitchingDecisionGetNbrSamplesNext(hInstance->switchingDecision.hSwDeci);
              hInstance->switchingDecision.nSwSamplesNextFrame = max(0, hInstance->switchingDecision.nSwSamplesNextFrame);
            }
            if (hInstance->encoderState != XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
              nSamplesNext = max(0, (int)hInstance->timeSignal.nSamplesRequired - (hInstance->encBufferData.encBufValidSamples - (hInstance->encBufferData.addDelay * (int)hInstance->hConfig->nInChannels)));

              if (hInstance->switchingDecision.hSwDeci != NULL) {
                nSamplesNext = max((int)nSamplesNext, (int)(((hInstance->switchingDecision.nSwSamplesNextFrame + hInstance->switchingDecision.nSwDeciBufferOffset) * (int)hInstance->hConfig->nInChannels) - hInstance->encBufferData.encBufValidSamples));
              }
            } else {
              nSamplesNext = 0;
            }
          }
        } while ((!isError(retValue)) && ((int)nSamplesNext <= nSamplesLeft) && (hInstance->auInfo.nAccessUnits < MAX_AU_PER_MULTIFRAME) && (hInstance->auInfo.nAccessUnits < 1));
      }
    }
  }

  if (!isError(retValue)) {
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_ENCODING) {
      int samplesNeeded = hInstance->mpeg4DelayParameter.mpeg4Delay - hInstance->mpeg4DelayParameter.mpeg4DelayWOSwitching + (hInstance->hConfig->nFrameSamples * (hInstance->nTrashAUs + 1));

      if (!isError(retValue)) {
        if (samplesNeeded <= FLT_MAX) {
          int samplesNeededInputSR = (int)ceil(samplesNeeded * (hInstance->hConfig->sampleRateIn / (float)hInstance->hConfig->sampleRateOut));
          int samplesNeededInputSRMultiChannel = samplesNeededInputSR * (int)hInstance->hConfig->nInChannels;
          int minRequestNextFrame = samplesNeededInputSRMultiChannel - hInstance->encBufferData.encBufValidSamples;

          nSamplesNext = max((int)nSamplesNext, (int)(1 * hInstance->hConfig->nInChannels));
          hInstance->timeSignal.additionalSampleRequest = minRequestNextFrame - (int)nSamplesNext;

          nSamplesNext = max((int)nSamplesNext, minRequestNextFrame);
        } else {
          retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_ENCODING || hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
      hInstance->timeSignal.gnSamplesNext = nSamplesNext;
      hInstance->timeSignal.nSamplesUntilNext = hInstance->timeSignal.gnSamplesNext;

      if (hInstance->hConfig->bLiveLoudnessLevelSet) {
        int availableSamplesCollectiveBuf;
        retValue = iisxHEAACEncLib_collectiveBuffer_getNumAvailableSamples(hInstance->collectiveBuffer,
                                                                           hInstance->hConfig->nInChannels,
                                                                           &availableSamplesCollectiveBuf);
        hInstance->unprocessedSamplesInCollectiveBuf = max(0, availableSamplesCollectiveBuf);
        if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
          hInstance->unprocessedSamplesInCollectiveBuf = 0;
        }
        nSamplesNext = hInstance->timeSignal.nSamplesUntilNext;
      }

    } else if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_STARTUP || hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_WAITING) {
      hInstance->timeSignal.nSamplesUntilNext = hInstance->timeSignal.nSamplesUntilNext - nSamples;
      nSamplesNext = hInstance->timeSignal.nSamplesUntilNext;
      hInstance->auInfo.pByteCnt[hInstance->auInfo.nAccessUnits] = 0;
      hInstance->auInfo.pNumValidSamples[hInstance->auInfo.nAccessUnits] = 0;
      hInstance->auInfo.nAccessUnits = 0;

    } else if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_READY_TO_CLOSE) {
      nSamplesNext = 0;
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }

    {
      hInstance->timeSignal.nSamplesMax = nSamplesNext;
    }

    if (hInstance->encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING && *pOutputBytes == 0) {
      retValue = iisxHEAACEncLib_encoderState_set(&hInstance->encoderState, XHEAACENCLIB_ENCODER_STATE_READY_TO_CLOSE);
    }
  }

  if (!isError(retValue)) {
    IIS_FPUcontrol_Restore(hInstance->hFpuCtrl);
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRBITS_MAX, hInstance->bitReservoirData.bitReservoirMax, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRBITS, hInstance->bitReservoirData.bitReservoir, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueFloat(hInstance->hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRLEVEL, hInstance->bitReservoirData.bitReservoirLevel, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_LEFT, nSamplesLeft, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_NEXT, nSamplesNext, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_MAX, hInstance->timeSignal.nSamplesMax, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = iisParamListAddParamValueInt(hInstance->hCodecParamList, PARAMLIST_PARAMETER_ADDITIONAL_SAMPLE_REQUEST, hInstance->timeSignal.additionalSampleRequest, PARAMLIST_MODE_REPLACE);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
    }
    if (errorInfo) freeErrorTraceback(errorInfo);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibUpdateAuInfoList(hInstance);
  }

  return retValue;
}

void XHEAACENCLIB_API IIS_xHEAACEncLib_Delete(
    XHEAACENCLIB_INSTANCE_HANDLE *const phInstance) {
  XHEAACENCLIB_AUINFO_HANDLE hAuInfo = NULL;

  if (phInstance != NULL) {
    if (*phInstance) {
      XHEAACENCLIB_INSTANCE_HANDLE hInstance = *phInstance;

      unsigned int i;

      if (hInstance->hUsacIndepFlag != NULL) {
        iisxHEAACEncLib_syncFrame_Delete(&hInstance->hUsacIndepFlag);
        hInstance->hUsacIndepFlag = NULL;
      }

      if (hInstance->bitDistribution.hBitDistribution != NULL) {
        iisxHEAACEncLib_bitDistribution_Delete(hInstance->bitDistribution.hBitDistribution);
        hInstance->bitDistribution.hBitDistribution = NULL;
      }

      if (hInstance->hLoaswriter != NULL) {
        iisxHEAACEncLibDelete_loaswriter(hInstance->hLoaswriter,
                                         hInstance->hBb);
        hInstance->hLoaswriter = NULL;
      }

      if (hInstance->hFpuCtrl != NULL) {
        IIS_FPUControl_Delete(hInstance->hFpuCtrl);
        hInstance->hFpuCtrl = NULL;
      }

      if (hInstance->hMpegsEnc != NULL) {
        iisxHEAACEncLibMpegsEncClose(hInstance->hMpegsEnc);
        hInstance->hMpegsEnc = NULL;
      }

      if (hInstance->pPreResamplerOutLr) {
        iisFree(hInstance->pPreResamplerOutLr);
        hInstance->pPreResamplerOutLr = NULL;
      }

      if (hInstance->timeSignal.pAacFrameCoreDelayInBuffer) {
        iisFree(hInstance->timeSignal.pAacFrameCoreDelayInBuffer);
        hInstance->timeSignal.pAacFrameCoreDelayInBuffer = NULL;
      }

      if (hInstance->auInfo.pByteCnt) {
        iisFree(hInstance->auInfo.pByteCnt);
        hInstance->auInfo.pByteCnt = NULL;
      }

      if (hInstance->auInfo.pNumValidSamples) {
        iisFree(hInstance->auInfo.pNumValidSamples);
        hInstance->auInfo.pNumValidSamples = NULL;
      }

      closeAudioSpecificConfig(hInstance->hConfig, &hInstance->asc);

      for (i = 0; i < MAX_SBR_PAYLOADS_PER_FRAME; i++) {
        if (hInstance->sbrPayloadList.hExtPayload[i]) {
          closeExtensionPayloadContainer(&hInstance->sbrPayloadList.hExtPayload[i]);
        }
      }
      for (i = 0; i < MAX_SBR_PAYLOADS_PER_FRAME; i++) {
        if (hInstance->mpsPayloadList.hExtPayload[i]) {
          closeExtensionPayloadContainer(&hInstance->mpsPayloadList.hExtPayload[i]);
        }
      }

      for (i = 0; i < hInstance->extEleList.numExtEle; i++) {
        if (hInstance->extEleList.extEle[i].container) {
          closeExtensionPayloadContainer(&hInstance->extEleList.extEle[i].container);
        }
      }

      if (hInstance->hCoreDelayBuffer != NULL) {
        MP4TIMEBUF_Delete(&hInstance->hCoreDelayBuffer);
      }
      if (hInstance->drcDelay.hDrcLookaheadBuffer != NULL) {
        MP4TIMEBUF_Delete(&hInstance->drcDelay.hDrcLookaheadBuffer);
      }

      if (hInstance->drcAndLoudness.hDrc) {
        iisxHEAACEncLib_drc_ifc_Delete(hInstance->drcAndLoudness.hDrc);
      }

      if (hInstance->lraControlDrcGainData.lraControlDrcGains) {
        iisFree(hInstance->lraControlDrcGainData.lraControlDrcGains);
        hInstance->lraControlDrcGainData.lraControlDrcGains = NULL;
        hInstance->lraControlDrcGainData.lraControlDrcGainsLength = 0;
      }

      if (hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolated) {
        iisFree(hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolated);
        hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolated = NULL;
        hInstance->lraControlDrcGainData.lraControlDrcGainsInterpolatedLength = 0;
      }

      if (hInstance->drcAndLoudness.hDrcLoudness) {
        iisxHEAACEncLib_drc_loudness_ifc_delete(hInstance->drcAndLoudness.hDrcLoudness);
      }

      if (hInstance->drcAndLoudness.hLoudness) {
        iisxHEAACEncLib_loudness_ifc_delete(hInstance->drcAndLoudness.hLoudness);
      }

      if (hInstance->hLiveLoudness) {
        iisxHEAACEncLibLiveLoudnessDelete(hInstance);
      }

      iisxHEAACEncLibAacEncClose(hInstance->hAacEnc);

      smpl_resampler_destruct(&hInstance->hPreResampler);

      smpl_resampler_destruct(&hInstance->hCoreDownSampler);

      if (hInstance->pQmfDownSamplerOut) {
        iisFree(hInstance->pQmfDownSamplerOut);
      }

      if (hInstance->hConfig->bUseSBR) {
        iisxHEAACEncLibSbrEncClose(hInstance->hSbrEnc);

        if (hInstance->ppQmfSamplesReal) {
          for (i = 0; i < hInstance->hConfig->nFrameSamples * hInstance->hConfig->nChannelsCoreCoder / 64; i++) {
            if (hInstance->ppQmfSamplesReal[i]) {
              iisFree(hInstance->ppQmfSamplesReal[i]);
            }
          }
          iisFree(hInstance->ppQmfSamplesReal);
        }

        if (hInstance->ppQmfSamplesImag) {
          for (i = 0; i < hInstance->hConfig->nFrameSamples * hInstance->hConfig->nChannelsCoreCoder / 64; i++) {
            if (hInstance->ppQmfSamplesImag[i]) {
              iisFree(hInstance->ppQmfSamplesImag[i]);
            }
          }
          iisFree(hInstance->ppQmfSamplesImag);
        }
      }

      if (hInstance->switchingDecision.hSwDeci != NULL) {
        iisSwitchingDecisionClose(&hInstance->switchingDecision.hSwDeci);
      }

      if (hInstance->audioPreRoll.hAudioPreRoll != NULL) {
        iisAudioPreRollLibDelete(&hInstance->audioPreRoll.hAudioPreRoll);
      }
      if (hInstance->audioPreRoll.PreRollAUBuffer != NULL) {
        iisFree(hInstance->audioPreRoll.PreRollAUBuffer);
      }

      hAuInfo = hInstance->hAuInfoList->hAccessUnits;
      while (hAuInfo) {
        XHEAACENCLIB_AUINFO_HANDLE hAuInfoNext = hAuInfo->pNextAu;
        iisFree(hAuInfo);
        hAuInfo = hAuInfoNext;
      }

      if (hInstance->hAuInfoList) {
        iisFree(hInstance->hAuInfoList);
      }
      if (hInstance->collectiveBuffer) {
        MP4TIMEBUF_Delete(&(hInstance->collectiveBuffer));
      }
      if (hInstance->encBufferData.hEncBuffer) {
        MP4TIMEBUF_Delete(&(hInstance->encBufferData.hEncBuffer));
      }

      if (hInstance->hSigMap) {
        iisxHEAACEncLib_SigMap_Delete(hInstance->hSigMap);
      }

      iisxHEAACEncLib_ConfigDelete(&(hInstance->hConfig));

      iisFree(hInstance);
    }
    *phInstance = NULL;
  }
}

XHEAACENCLIB_RETURN XHEAACENCLIB_API IIS_xHEAACEncLib_GetEncodingWarnings(
    XHEAACENCLIB_INSTANCE_HANDLE hInstance,
    int *const pnWarning,
    XHEAACENCLIB_WARNING const **const ppWarnings) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL || pnWarning == NULL || ppWarnings == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (hInstance->warningList.resetFlag == 1) {
    retValue = iisxHEAACEncLib_warnings_reset(&hInstance->warningList);
  }

  if (!isError(retValue)) {
    *pnWarning = hInstance->warningList.nWarnings;
    *ppWarnings = hInstance->warningList.warningBuffer;

    hInstance->warningList.resetFlag = 1;
  }

  return retValue;
}
