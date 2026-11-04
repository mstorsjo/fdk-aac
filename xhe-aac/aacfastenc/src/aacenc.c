
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

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "aacenc.h"
#include "adj_thr.h"
#include "audioobjecttypes.h"
#include "bandwidth.h"
#include "bit_aac.h"
#include "bit_buf.h"
#include "bit_enc.h"
#include "block_switch.h"
#include "bs_configuration.h"
#include "extpayload.h"
#include "glob_con.h"
#include "iisBitFrame.h"
#include "iisSigMap.h"
#include "lpd_wrapper.h"
#include "mathlib.h"
#include "psy_configuration.h"
#include "psy_data.h"
#include "psy_main.h"
#include "qc_data.h"
#include "qc_main.h"
#include "quantize.h"
#include "sf_estim.h"
#include "stack_alignment.h"
#include "time_buffer.h"
#include "tns.h"
#include "iisutillib.h"

#include "readonlybitbuf.h"
#include "writeonlybitbuf.h"

#include "int_dec.h"

#include "aacenc_internal.h"

#define AAC_LIBRARY_VERSION "05.20.05"

#define MIN_BUFSIZE_PER_EFF_CHAN (6144)
#define MAX_EXT_PAYLOAD_BYTES (((1 << 4) - 1) + ((1 << 8) - 1) - 1)
#define MAX_EXT_PAYLOAD_BITS ((MAX_EXT_PAYLOAD_BYTES)*8)
#define MAX_FILL_ELEMENT_BITS (MAX_EXT_PAYLOAD_BITS + 3 + 4 + 8)
#define MAX_BYTES_FIL_ELEMENT ((MAX_FILL_ELEMENT_BITS + 7) / 8)
#define MAX_FILL_ELEMENTS (4)
#define MAX_SIGNAL_GROUPS 32
#define BITS_PER_BYTE 8
#define MIN_FADING_TIME_SEC (.005)
#define RESIDUAL_CHANNEL 1

#define MAX_ABS_SAMPLE_RANGE (float)(1 << 15)

#ifndef INT64
#if !(defined(WIN32) || defined(WIN64))

#define INT64 long long
#else
#define INT64 __int64
#endif
#endif

#define SIMULATED_VBR_BITRES_FILL_LEVEL (0.7f)
#define BITRES_FRAMES 1000.0

struct AACENC_CONFIG_TAG {
  int sampleRate;
  int bitRate;
  int bitReservoirPenalty;
  float bitResDistribution;
  int bitrateMode;

  float bandWidth;
  int useIS;
  int useTns;
  int useRequantization;
  int useStereoPreprocessor;
  int calcCrc;
  int mpeg4Flag;
  int ancDataBitRate;
  int useNoiseFilling;
  float bitResInitFillLevel;

  int adtsPrivateBit;
  int adtsOrigCopy;

  TRANS_MUX transMux;

  LPD_WRAPPER_CODING_MODE lpdCodingMode;

  AACENC_GRANULE_LEN nGranuleLength;

  BIT_DISTRIBUTION_MODE bitDistributionMode;

  AACENC_CODEC_TYPE codecType;

  int bResidualCoding;
  CHANNEL_MAPPING* channelMapping;

  unsigned int bitRateFractRemainder;
  unsigned int bitRateFractTimeBase;

  int acelpModeIndex;
  int optimizedSpeedPulseSearch;

  int useExtendedBitReservoir;
  int useLloydMaxQuantizer;
};

struct AACENC_ENCODER_TAG {
  LPD_WRAPPER_HANDLE hLpdWrapper;

  AACENC_CONFIG_HANDLE hConfig;

  CHANNEL_MAPPING* channelMapping;

  QC_STATE* qcKernel;
  QC_OUT* qcOut;
  PSY_OUT* psyOut;
  PSY_INTERNAL* psyKernel;

  HANDLE_BIT_BUF hBitstream;
  HANDLE_TFDEC hTfDec;

  HANDLE_BIT_BUF hAdditionalDataBitBuffer;

  AACENC_GRANULE_LEN nGranuleLength;

  char FacPrm[SIGMAP_MAX_SIGNALS][1000];
  int Nbits_fac[SIGMAP_MAX_SIGNALS];

  BS_CONFIGURATION bs_configuration;
  HANDLE_BITSTREAM_ENC hBsEnc;

  IISBITFRAME_HANDLE hBitFrame;

  int bWritePce;
  int frameCntPce;
  int updateRatePce;

  int cbBufSizeMin;
  int nDelay;
  int nAddEncDelay;
  int nStandDelay;
  int bandwidth;
  AACENC_BITRATE_MODE bitrateMode;

  int additionalElemBits[SIGMAP_MAX_ELEMENTS];

  AACENC_SAP_TYPE syncFrame;
  int sfbActiveLongBackup;
  int sfbActiveShortBackup;
  int lpLineLongBackup;
  int lpLineShortBackup;

  INTERN_CORE_MODE coreMode[SIGMAP_MAX_SIGNALS];
  INTERN_CORE_MODE coreModePrev[SIGMAP_MAX_SIGNALS];
  INTERN_CORE_MODE coreModeNext[SIGMAP_MAX_SIGNALS];

  int useCpuOptimisation;

  int minFrameBytes;
  int bForceBitResFilling;

  void* headerBitsCallbackHandle;
  IISAACFENC_HEADER_BITS_CALLBACK headerBitsCallbackFunc;
};

typedef struct
{
  AACENC_BITRATE_MODE bitrateMode;
  int chanBitrate[2];
} IISAACFENC_CONFIG_TAB_ENTRY_VBR;

static const IISAACFENC_CONFIG_TAB_ENTRY_VBR configTabVBR_xHE[] = {

    {AACENC_BR_MODE_CBR, {0, 0}},
    {AACENC_BR_MODE_VBR_1, {32000, 40000}},
    {AACENC_BR_MODE_VBR_2, {40000, 32000}},
    {AACENC_BR_MODE_VBR_3, {56000, 48000}},
    {AACENC_BR_MODE_VBR_4, {72000, 64000}},
    {AACENC_BR_MODE_VBR_5, {104000, 96000}},
    {AACENC_BR_MODE_VBR_6, {136000, 128000}},
    {AACENC_BR_MODE_VBR_0, {20000, 24000}}};

static const IISAACFENC_CONFIG_TAB_ENTRY_VBR configTabVBR_AAC[] = {
    {AACENC_BR_MODE_CBR, {0, 0}},
    {AACENC_BR_MODE_VBR_1, {32000, 20000}},
    {AACENC_BR_MODE_VBR_2, {40000, 32000}},
    {AACENC_BR_MODE_VBR_3, {56000, 48000}},
    {AACENC_BR_MODE_VBR_4, {72000, 64000}},
    {AACENC_BR_MODE_VBR_5, {104000, 96000}},
    {AACENC_BR_MODE_VBR_6, {136000, 128000}},
    {AACENC_BR_MODE_VBR_0, {20000, 24000}}};

typedef enum {
  AACENC_EXTELEREMOVAL_STATE_NOT_SUPPORTED = -1,
  AACENC_EXTELEREMOVAL_STATE_KEEP = 0,
  AACENC_EXTELEREMOVAL_STATE_REMOVE = 1
} AACENC_EXTELEREMOVAL_STATE;

static const float sendPceTimeIntervalDefault = .5f;

struct IISAACFENC_SSR_OUT_INFO;

static int isError(
    AACFASTENC_ERROR const statusCode);

static AACFASTENC_ERROR AACENCAPI IISAACFENC_AacEncNew(
    AACENC_ENCODER_HANDLE* phAacEnc);

static AACFASTENC_ERROR AACENCAPI aacEncDistributeExtensionPayload(
    HANDLE_EXTPAYLOAD_CONTAINER* hExtensionContainer,
    int* additionalElemBits,
    int numExternalContainersInUse,
    int* nBitsOutOfBand,
    int* externalElemBits,
    int* nBitsMps,
    CHANNEL_MAPPING* cm);

static AACFASTENC_ERROR AACENCAPI resetExtensionPayload(
    HANDLE_EXTPAYLOAD_CONTAINER* hExtensionContainer,
    int numExternalContainersInUse);

static void
iisaacfenc_ProcessMeBitDistribution(
    CHANNEL_MAPPING* cm,
    QC_STATE* hQC,
    PSY_OUT* psyOut,
    QC_OUT* qcOut,
    unsigned int nBitsTransportOverhead,
    int* nBitsAcelp,
    int* elemSideInfoBits,
    IISBITFRAME_HANDLE hBitFrame,
    int* pAdditionalElemBits,
    BIT_DISTRIBUTION_MODE bitDistributionMode,
    const int highBWframe);

static AACFASTENC_ERROR calculateHeaderBits(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int const nBitsOutOfBand,
    int* headerBits);

static AACFASTENC_ERROR computeHeaderLengthFillBytes(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int headerBits,
    int nBitsOutOfBand,
    int nBitsPce);

static AACFASTENC_ERROR mappingBitFrameErrorToAacError(
    IISBITFRAME_ERROR bitFrameError);

static AACFASTENC_ERROR mappingLpdErrorToAacError(
    LPD_WRAPPER_ERROR lpdError);

static AACFASTENC_ERROR mappingIpfStateFromAacToLpdWrapper(
    AACENC_IPF_STATE ipfStateAacenc,
    LPD_WRAPPER_IPF_STATE* pMappedIpfStateLpdWrapper);

static AACFASTENC_ERROR updateBitreservoirAndExtendedBitreservoir(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const int extBitResSize,
    const int intervalSamples);

static AACFASTENC_ERROR extendedBitReservoirUpdateFillRate(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const int intervalSamples) {
  int fillRate = 0;
  int extBitRes = 0;
  int extBitResMax = 0;

  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  IISBITFRAME_ERROR errorInfo = IISBITFRAME_NO_ERROR;

  if (hAacEnc == NULL) {
    error = AACENC_INIT_ERROR;
    return error;
  }

  if (intervalSamples > 0) {
    extBitRes = IISBITFRAME_GetExtendedBitreservoir(hAacEnc->hBitFrame);

    extBitResMax = IISBITFRAME_GetExtendedBitreservoirMax(hAacEnc->hBitFrame);

    if (extBitResMax - extBitRes <= 0) {
      extBitRes = 0;
    }

    fillRate = (int)((extBitResMax - extBitRes) / (float)(intervalSamples / hAacEnc->hConfig->nGranuleLength) + 0.5f);

    fillRate = max(0, min(fillRate, IISBITFRAME_GetAverageBitsPerFrame(hAacEnc->hBitFrame)));

    if (fillRate % 8 > 0) {
      fillRate += (8 - (fillRate % 8));
    }
  }

  errorInfo = IISBITFRAME_SetExtendedBitReservoirFillRate(hAacEnc->hBitFrame, fillRate);

  if (errorInfo != IISBITFRAME_NO_ERROR) {
    error = AACENC_UNKNOWN_ERROR;
  }

  return error;
}

static int getLPDHeadroom(
    const int totalBitrate,
    const int maxChannels,
    const int sampleRate,
    const int granuleLength);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_SetHeaderBitsCallback(
    AACENC_ENCODER_HANDLE hAacEnc,
    void* headerBitsCallbackHandle,
    IISAACFENC_HEADER_BITS_CALLBACK const headerBitsCallbackFunc) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  if (hAacEnc) {
    hAacEnc->headerBitsCallbackHandle = headerBitsCallbackHandle;
    hAacEnc->headerBitsCallbackFunc = headerBitsCallbackFunc;
  }
  return (error);
}

int IISAACFENC_GetVBRBitrate(AACENC_BITRATE_MODE bitrateMode, CHANNEL_MAPPING_HANDLE hChMap, AACENC_SETUP_CODEC_TYPE codecType) {
  int error = 0;
  int bitrate = 0;

  if (hChMap == NULL) {
    error = -1;
  }

  if (!error) {
    int numberEffectiveChannels = hChMap->nEffectiveChannels;
    int monoStereoMode = hChMap->monoStereoMode;

    if ((monoStereoMode == 0) || (monoStereoMode == 1)) {
      switch (bitrateMode) {
        case AACENC_BR_MODE_VBR_0:
        case AACENC_BR_MODE_VBR_1:

          if (codecType == AACENC_SETUP_CODEC_XHEAAC) {
            if (hChMap->cicpLayoutIndex == SIGMAP_CICP_1) {
              bitrate = configTabVBR_xHE[bitrateMode].chanBitrate[0];
            } else if (hChMap->cicpLayoutIndex == SIGMAP_CICP_2) {
              bitrate = configTabVBR_xHE[bitrateMode].chanBitrate[1];
            }
          } else {
            bitrate = configTabVBR_AAC[bitrateMode].chanBitrate[monoStereoMode];
          }
          break;

        case AACENC_BR_MODE_VBR_2:
        case AACENC_BR_MODE_VBR_3:
        case AACENC_BR_MODE_VBR_4:
        case AACENC_BR_MODE_VBR_5:
        case AACENC_BR_MODE_VBR_6:

          if (codecType == AACENC_SETUP_CODEC_XHEAAC) {
            bitrate = configTabVBR_xHE[bitrateMode].chanBitrate[monoStereoMode];
          } else {
            bitrate = configTabVBR_AAC[bitrateMode].chanBitrate[monoStereoMode];
          }
          break;

        case AACENC_BR_MODE_INVALID:
        case AACENC_BR_MODE_CBR:
        default:
          bitrate = 0;
          break;
      }
    }

    bitrate *= numberEffectiveChannels;
  }

  return bitrate;
}

static AACFASTENC_ERROR UpdateBitFrame(AACENC_ENCODER_HANDLE const hAacEnc);

static int isVbr(int const bitrateMode);

static int getAverageBitsPerVbrFrame(AACENC_ENCODER_HANDLE const hAacEnc);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_GetConfiguration(const AACENC_SETUP setup,
                            AACENC_CONFIG_HANDLE* phConfig) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  int confError = 0;
  int nEffectiveChannels = 0;
  int channelBitrate = 0;
  AACENC_CONFIG* hConfig = NULL;

  if (*phConfig == NULL) {
    hConfig = (AACENC_CONFIG*)iisCalloc(1, sizeof(AACENC_CONFIG));
    if (hConfig == NULL) {
      error = AACENC_GET_CONF_ERROR;
    } else {
      *phConfig = hConfig;
    }
  } else {
    hConfig = *phConfig;
  }

  if (!isError(error)) {
    switch (setup.codecType) {
      case AACENC_SETUP_CODEC_XHEAAC:

        confError |= setup.fullbandLpd;
        confError |= setup.useFDP;
        confError |= setup.useLtpPost;
        confError |= setup.useStereoLpd;
        confError |= setup.useEnhancedNoiseFilling;
        confError |= setup.lpdStereoModeIndex;
        confError |= setup.stereoFilling;
        confError |= setup.mctMode;
        break;
      default:
        break;
    }
  }
  if (confError > 0) {
    return AACENC_CONF_EXP_ERROR;
  }

  if (!isError(error)) {
    nEffectiveChannels = setup.channelMapping->nEffectiveChannels;
    if (nEffectiveChannels < 1) {
      error = AACENC_CONF_EXP_ERROR;
    }
  }

  if (!isError(error)) {
    hConfig->sampleRate = setup.sampleRate;
    switch (setup.quality) {
      case QUAL_FAST:
        hConfig->useRequantization = 0;
        break;
      case QUAL_MEDIUM:
        hConfig->useRequantization = 1;
        break;
      case QUAL_HIGH:
        hConfig->useRequantization = 2;
        break;
      default:
        error = AACENC_CONF_EXP_ERROR;
    }
  }

  if (!isError(error)) {
    hConfig->useTns = 1;
    hConfig->useTns = (setup.useTns == 0 || setup.useTns == 1) ? setup.useTns : hConfig->useTns;
  }

  if (!isError(error)) {
    hConfig->bitrateMode = setup.bitrateMode;
    switch (hConfig->bitrateMode) {
      case AACENC_BR_MODE_CBR:
        hConfig->bitRate = setup.bitRate;
        hConfig->bitReservoirPenalty = setup.bitReservoirPenalty;
        hConfig->bitResDistribution = setup.bitResDistribution;
        break;
      case AACENC_BR_MODE_VBR_0:
      case AACENC_BR_MODE_VBR_1:
      case AACENC_BR_MODE_VBR_2:
      case AACENC_BR_MODE_VBR_3:
      case AACENC_BR_MODE_VBR_4:
      case AACENC_BR_MODE_VBR_5:
      case AACENC_BR_MODE_VBR_6:
        hConfig->bitRate = IISAACFENC_GetVBRBitrate(setup.bitrateMode, setup.channelMapping, setup.codecType);
        hConfig->bitReservoirPenalty = 0;
        hConfig->bitResDistribution = -1.0f;
        break;
      default:
        hConfig->bitRate = 0;
        hConfig->bitReservoirPenalty = 0;
        hConfig->bitResDistribution = -1.0f;
    }

    channelBitrate = hConfig->bitRate / nEffectiveChannels;
    if (channelBitrate < 1) {
      error = AACENC_CONF_EXP_ERROR;
    }

    switch (setup.codecType) {
      case AACENC_SETUP_CODEC_XHEAAC:
        hConfig->codecType = AACENC_CODEC_XHEAAC;
        break;
      case AACENC_SETUP_CODEC_MPEGH:
        hConfig->codecType = AACENC_CODEC_MPEGH;
        break;
      default:
        error = AACENC_CONF_EXP_ERROR;
        break;
    }

    hConfig->useNoiseFilling = setup.useNoiseFilling;
    hConfig->calcCrc = setup.calcCrc;
    hConfig->nGranuleLength = setup.nGranuleLength;
    hConfig->mpeg4Flag = setup.mpeg4;
    hConfig->useIS = 1;
    hConfig->bResidualCoding = setup.bResidualCoding;
    hConfig->bandWidth = setup.bandWidth;
    hConfig->sampleRate = setup.sampleRate;
    hConfig->ancDataBitRate = setup.ancDataBitRate;
    hConfig->channelMapping = setup.channelMapping;
    hConfig->useExtendedBitReservoir = setup.useExtendedBitReservoir;
    hConfig->bitRateFractRemainder = setup.bitRateFractRemainder;
    hConfig->bitRateFractTimeBase = setup.bitRateFractTimeBase;
    hConfig->useLloydMaxQuantizer = setup.useLloydMaxQuantizer;

    hConfig->acelpModeIndex = setup.acelpModeIndex;

    hConfig->optimizedSpeedPulseSearch = setup.optimizedSpeedPulseSearch;
    switch (setup.codingMode) {
      case AACENC_SETUP_CODING_MODE_FD:
        hConfig->lpdCodingMode = LPD_WRAPPER_CODING_MODE_INVALID;
        break;
      case AACENC_SETUP_CODING_MODE_LPD:
      case AACENC_SETUP_CODING_MODE_SWITCHED:
        hConfig->lpdCodingMode = LPD_WRAPPER_CODING_MODE_SWITCHED;
        break;
      case AACENC_SETUP_CODING_MODE_ACELP:
        hConfig->lpdCodingMode = LPD_WRAPPER_CODING_MODE_ACELP;
        break;
      case AACENC_SETUP_CODING_MODE_TCX:
        hConfig->lpdCodingMode = LPD_WRAPPER_CODING_MODE_TCX;
        break;
      default:
        error = AACENC_CONF_EXP_ERROR;
        break;
    }

    switch (setup.transMux) {
      case AACENC_MUX_RAW:
        hConfig->transMux = MUX_RAW;
        break;
      default:
        error = AACENC_CONF_EXP_ERROR;
        break;
    }

    switch (setup.bitDistributionMode) {
      case AACENC_BD_MODE_INTER_ELEMENT:
        hConfig->bitDistributionMode = BD_MODE_INTER_ELEMENT;
        break;
      case AACENC_BD_MODE_INTRA_ELEMENT:
        hConfig->bitDistributionMode = BD_MODE_INTRA_ELEMENT;
        break;
      default:
        error = AACENC_CONF_EXP_ERROR;
        break;
    }

    switch (setup.codecType) {
      case AACENC_CODEC_XHEAAC:
        hConfig->useIS = 0;
        break;
      case AACENC_CODEC_AAC:
      default:
        break;
    }
  }

  if (!isError(error)) {
    if (hConfig->codecType == AACENC_CODEC_MPEGH) confError = 1;
  }
  if (!isError(error)) {
    if (confError > 0) {
      error = AACENC_CONF_EXP_ERROR;
    }
  }
  return error;
}

static void iisaacfenc_deleteBsChannelMapping(BS_CHANNEL_MAPPING* bs_cm) {
  if (bs_cm) {
    if (bs_cm->element) {
      int i;
      for (i = 0; i < bs_cm->noOfElements; i++) {
        if (bs_cm->element[i]) {
          if (bs_cm->element[i]->elementType == ID_CCE) {
            iisFree(bs_cm->element[i]->element.cce.coupledWith);
          }
          iisFree(bs_cm->element[i]);
        }
      }
      iisFree(bs_cm->element);
    }
    iisFree(bs_cm);
  }
}

static HANDLE_ERROR_INFO iisaacfenc_allocBsChannelMapping(BS_CHANNEL_MAPPING** pbs_cm, const CHANNEL_MAPPING* cm) {
  HANDLE_ERROR_INFO errorInfo = noError;
  BS_CHANNEL_MAPPING* bs_cm = NULL;
  int i;

  if (*pbs_cm == NULL) {
    *pbs_cm = (BS_CHANNEL_MAPPING*)iisCalloc(1, sizeof(BS_CHANNEL_MAPPING));
    if (*pbs_cm == NULL) {
      errorInfo = iisUtil_ERROR(CDI, "out of memory");
    }
  }

  if (noError == errorInfo) {
    bs_cm = *pbs_cm;

    bs_cm->noOfChannels = cm->nChannels;
    bs_cm->noOfElements = cm->nElements;

    if (bs_cm->element != NULL) {
      for (i = 0; i < bs_cm->noOfElements; i++) {
        if (bs_cm->element[i] != NULL) {
          iisFree(bs_cm->element[i]);
        }
      }
      iisFree(bs_cm->element);
    }

    bs_cm->element = (BS_MAPPING_ELEMENT**)iisCalloc(bs_cm->noOfElements, sizeof(BS_MAPPING_ELEMENT*));

    if (bs_cm->element == 0) {
      errorInfo = iisUtil_ERROR(CDI, "out of memory");
    }
  }

  if (noError == errorInfo) {
    for (i = 0; i < bs_cm->noOfElements; i++) {
      if (bs_cm->element[i] == NULL) {
        bs_cm->element[i] = (BS_MAPPING_ELEMENT*)iisCalloc(1, sizeof(BS_MAPPING_ELEMENT));
        if (bs_cm->element[i] == NULL) {
          errorInfo = iisUtil_ERROR(CDI, "out of memory");
          break;
        }
      }

      if (noError == errorInfo) {
        bs_cm->element[i]->elementType = cm->elInfo[i].elType;
        bs_cm->element[i]->instanceTag = cm->elInfo[i].instanceTag;

        switch (bs_cm->element[i]->elementType) {
          case ID_SCE:
            bs_cm->element[i]->element.sce.channelNdx = cm->elInfo[i].ChannelIndex[0];
            break;
          case ID_CPE:
            bs_cm->element[i]->element.cpe.channelNdx[0] = cm->elInfo[i].ChannelIndex[0];
            bs_cm->element[i]->element.cpe.channelNdx[1] = cm->elInfo[i].ChannelIndex[1];
            bs_cm->element[i]->element.cpe.nChannels = cm->elInfo[i].nChannelsInEl;
            break;
          case ID_LFE:
            bs_cm->element[i]->element.lfe.channelNdx = cm->elInfo[i].ChannelIndex[0];
            break;
          case ID_CCE:
            bs_cm->element[i]->element.cce.channelNdx = cm->elInfo[i].ChannelIndex[0];

            errorInfo = iisUtil_ERROR(CDI, "cce not yet supported");
            break;
          case ID_DSE:
            if (cm->elInfo[i].isDRC) {
              bs_cm->element[i]->element.dse.is_drc = 1;
            } else {
              bs_cm->element[i]->element.dse.is_drc = 0;
            }
            bs_cm->element[i]->element.dse.byteCount = 0;
            bs_cm->element[i]->element.dse.data_byte_align_flag = 0;
            break;
          case ID_PCE:
          case ID_FIL:
          case ID_END:
          default:
            break;
        }
      }

      if (noError != errorInfo) {
        break;
      }
    }
  }

  if (noError != errorInfo) {
    iisaacfenc_deleteBsChannelMapping(bs_cm);
  }

  return errorInfo;
}

static HANDLE_ERROR_INFO iisaacfenc_reassignBsChannelMapping(BS_CHANNEL_MAPPING* bs_cm, const CHANNEL_MAPPING* cm) {
  HANDLE_ERROR_INFO errorInfo = noError;
  int i;

  if (bs_cm == 0) {
    errorInfo = iisUtil_ERROR(CDI, "invalid BS_CHANNEL_MAPPING struct");
  }

  if (noError == errorInfo) {
    if ((bs_cm->noOfChannels == cm->nChannels) && (bs_cm->noOfElements == cm->nElements)) {
      for (i = 0; i < bs_cm->noOfElements; i++) {
        assert(bs_cm->element[i]->elementType == cm->elInfo[i].elType);
        assert(bs_cm->element[i]->instanceTag == cm->elInfo[i].instanceTag);

        switch (bs_cm->element[i]->elementType) {
          case ID_SCE:
            bs_cm->element[i]->element.sce.channelNdx = cm->elInfo[i].ChannelIndex[0];
            break;

          case ID_CPE:
            bs_cm->element[i]->element.cpe.channelNdx[0] = cm->elInfo[i].ChannelIndex[0];
            bs_cm->element[i]->element.cpe.channelNdx[1] = cm->elInfo[i].ChannelIndex[1];
            bs_cm->element[i]->element.cpe.nChannels = cm->elInfo[i].nChannelsInEl;
            break;

          case ID_LFE:
            bs_cm->element[i]->element.lfe.channelNdx = cm->elInfo[i].ChannelIndex[0];
            break;

          case ID_CCE:
            bs_cm->element[i]->element.cce.channelNdx = cm->elInfo[i].ChannelIndex[0];

            errorInfo = iisUtil_ERROR(CDI, "cce not yet supported");
            break;

          case ID_DSE:
          case ID_PCE:
          case ID_FIL:
          case ID_END:
          default:
            break;
        }

        if (noError != errorInfo) {
          break;
        }
      }
    } else {
      errorInfo = iisUtil_ERROR(CDI, "channel mappings don't match!");
    }
  }

  return errorInfo;
}

static void iisaacfenc_wrap_tns(BS_TNS_DATA* bs_tns_data, TNS_INFO* tnsOutInfo, BLOCK_TYPE blockType, AACENC_CODEC_TYPE codecType) {
  bs_tns_data->tnsEnabled = tnsOutInfo != NULL;
  if (tnsOutInfo) {
    int i;
    int numOfWindows = (blockType == 2 ? TRANS_FAC : 1);

    bs_tns_data->tnsActive = 0;
    bs_tns_data->numOfSubblocks = numOfWindows;

    for (i = 0; i < numOfWindows; i++) {
      bs_tns_data->subBlock[i].numOfFilters = tnsOutInfo->numOfFilters[i];
      if (tnsOutInfo->numOfFilters[i]) {
        int filter;
        bs_tns_data->tnsActive = 1;
        bs_tns_data->subBlock[i].coefficientResolution = tnsOutInfo->coefRes[i];

        for (filter = 0; filter < bs_tns_data->subBlock[i].numOfFilters; filter++) {
          int k;

          bs_tns_data->subBlock[i].filters[filter].startBand = 0;
          bs_tns_data->subBlock[i].filters[filter].stopBand = tnsOutInfo->length[i][filter];

          bs_tns_data->subBlock[i].filters[filter].order = tnsOutInfo->order[i][filter];
          bs_tns_data->subBlock[i].filters[filter].direction = tnsOutInfo->direction[i][filter];
          for (k = 0; k < HEAAC_TNS_MAX_ORDER; k++) {
            bs_tns_data->subBlock[i].filters[filter].coefficients[k] = tnsOutInfo->coef[i][filter][k];
          }
        }
      }
    }
  }

  switch (codecType) {
    case AACENC_CODEC_XHEAAC:
      bs_tns_data->bCommonTns = tnsOutInfo->bCommonTns;
      bs_tns_data->bTnsOnLr = tnsOutInfo->bTnsOnLr;
      break;
    default:
      break;
  }
}

static void iisaacfenc_transferBitEncChannelSideData(
    BIT_ENC_CHANNEL_DATA* bEncData,
    PSY_OUT_CHANNEL* hPsyOut,
    PSY_OUT_ELEMENT* hPsyOutElem,
    AACENC_CODEC_TYPE codecType) {
  bEncData->blockType = hPsyOut->windowSequence;
  bEncData->mdctWindowShape = hPsyOut->windowShape;
  bEncData->maxSfb = hPsyOut->maxSfbPerGroup;
  bEncData->grpSfb = hPsyOut->sfbPerGroup;
  bEncData->sfbCnt = hPsyOut->sfbCnt;
  bEncData->commonWindow = hPsyOutElem->commonWindow;
  bEncData->groupingMask = hPsyOut->groupingMask;

  memcpy(bEncData->jsFlag, hPsyOutElem->toolsInfo.msMask, sizeof(bEncData->jsFlag));

  bEncData->bCplxPredMdct = hPsyOutElem->toolsInfo.bCplxPredMdct;
  memcpy(bEncData->predCoefRe, hPsyOutElem->toolsInfo.predCoefReQ, sizeof(bEncData->predCoefRe));
  memcpy(bEncData->predCoefIm, hPsyOutElem->toolsInfo.predCoefImQ, sizeof(bEncData->predCoefIm));
  memcpy(bEncData->predCoefPrevRe, hPsyOutElem->toolsInfo.predCoefPrevReQ, sizeof(bEncData->predCoefPrevRe));
  memcpy(bEncData->predCoefPrevIm, hPsyOutElem->toolsInfo.predCoefPrevImQ, sizeof(bEncData->predCoefPrevIm));
  bEncData->bResetPredictors = hPsyOutElem->toolsInfo.bCplxPredMdctResetPredictors;
  bEncData->nGroupsPrev = hPsyOutElem->toolsInfo.nGroupsPrev;
  bEncData->windowSequencePrev = hPsyOutElem->toolsInfo.windowSequencePrev;
  bEncData->sfbPerPredBand = hPsyOutElem->toolsInfo.sfbPerPredBand;
  bEncData->bSwappedChannel = hPsyOutElem->toolsInfo.bSwap;
  bEncData->bPrevFrame = hPsyOutElem->toolsInfo.bPrevFrame;

  memcpy(bEncData->sfbBandOffset, hPsyOut->sfbOffsets, sizeof(bEncData->sfbBandOffset));

  iisaacfenc_wrap_tns(&bEncData->bs_tns_data, &hPsyOut->tnsInfo, bEncData->blockType, codecType);
  bEncData->bs_prediction_data.predictorDataEnabled = 0;
  bEncData->bs_ltp_data.ltpDataEnabled = 0;
}

void AACENCAPI IISAACFENC_AacEncClose(AACENC_ENCODER* hAacEnc) {
  int error = 0;

  if (hAacEnc) {
    if (hAacEnc->hLpdWrapper)
      iisaacfenc_wrap_lpd_close(hAacEnc->hLpdWrapper);
    iisaacfenc_QCDelete(hAacEnc->qcKernel);
    iisaacfenc_QCOutDelete(hAacEnc->qcOut);
    if ((!error) && (hAacEnc->hTfDec != NULL)) {
      iisaacfenc_DeleteIntDec(hAacEnc->hTfDec);
    }

    if (hAacEnc->psyKernel) {
      iisaacfenc_PsyDelete(hAacEnc->psyKernel);
    }
    if (hAacEnc->psyOut) {
      iisaacfenc_PsyOutDelete(hAacEnc->psyOut);
    }

    DeleteBitBuffer(hAacEnc->hBitstream);

    if (hAacEnc->hAdditionalDataBitBuffer != NULL) {
      DeleteBitBuffer(hAacEnc->hAdditionalDataBitBuffer);
    }
    if (hAacEnc->channelMapping) {
      iisFree(hAacEnc->channelMapping);
    }
    if (hAacEnc->hBsEnc) {
      DeleteBitstreamEncoder(hAacEnc->hBsEnc);
    }
    if (hAacEnc->hBitFrame) {
      IISBITFRAME_DeleteBitFrame(hAacEnc->hBitFrame);
      hAacEnc->hBitFrame = NULL;
    }
    if (hAacEnc->hConfig != NULL) {
      IISAACFENC_AacConfigClose(&hAacEnc->hConfig);
    }

    iisFree(hAacEnc);
  }
}

void AACENCAPI IISAACFENC_AacConfigClose(AACENC_CONFIG_HANDLE* const phConfig) {
  if (*phConfig != NULL) {
    iisFree(*phConfig);
    *phConfig = NULL;
  }
}

static AACENC_EXTELEREMOVAL_STATE iisaacfenc_detectExtensionElementRemoval(
    AACENC_IPF_STATE const ipfState,
    unsigned char const* const bitstreamBufferWoExtElement,
    int const* const numValidBufferBitsWoExtElement) {
  AACENC_EXTELEREMOVAL_STATE removeExtensionElement = AACENC_EXTELEREMOVAL_STATE_KEEP;

  if (bitstreamBufferWoExtElement == NULL || numValidBufferBitsWoExtElement == NULL) {
    removeExtensionElement = AACENC_EXTELEREMOVAL_STATE_NOT_SUPPORTED;
  } else {
    switch (ipfState) {
      case AACENC_IPF_STATE_RAP_FIRST_PREROLL:
      case AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL:
      case AACENC_IPF_STATE_RAP_NEXT_PREROLL:
      case AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL:
      case AACENC_IPF_STATE_RAP_IPF:
      case AACENC_IPF_STATE_RAP_IPF_PREROLL:
      case AACENC_IPF_STATE_CONFIGCHANGE_IPF_PREROLL:

        removeExtensionElement = AACENC_EXTELEREMOVAL_STATE_REMOVE;
        break;
      default:
        removeExtensionElement = AACENC_EXTELEREMOVAL_STATE_KEEP;
    }
  }

  return removeExtensionElement;
}

static AACFASTENC_ERROR iisaacfenc_removeExtensionElementFill(
    unsigned char* const bitstreamBuffer,
    int const bufferSizeBytes,
    int* const numValidBufferBits,
    BS_EXT_ELE_FIL_INFO const* const extEleFilInfo,
    unsigned int* const usacByteAlignmentBits) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  int origAuSizeInBytes = 0;
  int patchedAuSizeInBytes = 0;
  unsigned int numByteAlignmentBits = 0;

  if (!isError(error)) {
    if ((NULL == bitstreamBuffer) || (NULL == numValidBufferBits) || (NULL == extEleFilInfo)) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    if (NULL == usacByteAlignmentBits) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    origAuSizeInBytes = (*numValidBufferBits + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE;

    if (origAuSizeInBytes > bufferSizeBytes) {
      error = AACENC_BUFSIZE_ERROR;
    }
  }

  if (!isError(error)) {
    if ((*numValidBufferBits % BITS_PER_BYTE) != 0) {
      error = AACENC_UNKNOWN_ERROR;
    }
  }

  if (!isError(error)) {
    patchedAuSizeInBytes = (extEleFilInfo->extEleFil_posBgn + BITS_PER_BYTE) / BITS_PER_BYTE;
    numByteAlignmentBits = (unsigned int)(BITS_PER_BYTE - 1) - extEleFilInfo->extEleFil_posBgn % BITS_PER_BYTE;

    if (patchedAuSizeInBytes > bufferSizeBytes) {
      error = AACENC_BUFSIZE_ERROR;
    }
  }

  if (!isError(error)) {
    if ((patchedAuSizeInBytes < origAuSizeInBytes) && (patchedAuSizeInBytes > 0) && (extEleFilInfo->extEleFil_present == 1)) {
      int extEleFil_posBgnInCurrentByte;
      int usacExtElementPresent;

      memset(bitstreamBuffer + patchedAuSizeInBytes, 0, (size_t)bufferSizeBytes - (size_t)patchedAuSizeInBytes);

      *numValidBufferBits = patchedAuSizeInBytes * BITS_PER_BYTE;

      extEleFil_posBgnInCurrentByte = (BITS_PER_BYTE - 1) - (extEleFilInfo->extEleFil_posBgn % BITS_PER_BYTE);

      usacExtElementPresent = (bitstreamBuffer[patchedAuSizeInBytes - 1] >> extEleFil_posBgnInCurrentByte) & 0x1;

      if (usacExtElementPresent == 1) {
        unsigned int bit = 0;

        bitstreamBuffer[patchedAuSizeInBytes - 1] &= (~(1 << extEleFil_posBgnInCurrentByte));

        for (bit = 0; bit < numByteAlignmentBits; bit++) {
          bitstreamBuffer[patchedAuSizeInBytes - 1] &= (~(1 << bit));
        }

        *usacByteAlignmentBits = numByteAlignmentBits;
      } else {
        error = AACENC_UNKNOWN_ERROR;
      }
    }
  }

  return error;
}

static AACFASTENC_ERROR iisaacfenc_extractAudioPreRollInformationFromBitstream(
    unsigned char const* const bitstreamBuffer,
    int const bufferSizeBytes,
    int* const usacIndepFlag,
    int* const audioPreRollExtElementPresent,
    int* const audioPreRollExtElementBits,
    int* const audioPreRollStartIndex) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (!isError(error)) {
    if ((NULL == bitstreamBuffer) || (NULL == usacIndepFlag) || (NULL == audioPreRollExtElementPresent) || (NULL == audioPreRollExtElementBits) || (NULL == audioPreRollStartIndex)) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    robitbuf bitstreamReader;
    const int index_of_usacExtElementPresent = 1;
    int usacIndependencyFlag;
    int usacExtElementPresent;
    int usacExtElementUseDefaultLength;
    int usacExtElementPayloadLength = 0;
    int aprExtElementBits;

    robitbuf_Init(&bitstreamReader, bitstreamBuffer, bufferSizeBytes * BITS_PER_BYTE, 0);

    usacIndependencyFlag = (int)robitbuf_ReadBits(&bitstreamReader, 1);
    usacExtElementPresent = (int)robitbuf_ReadBits(&bitstreamReader, 1);
    aprExtElementBits = 1;

    if (1 == usacExtElementPresent) {
      usacExtElementUseDefaultLength = (int)robitbuf_ReadBits(&bitstreamReader, 1);
      aprExtElementBits += 1;

      if (1 == usacExtElementUseDefaultLength) {
        error = AACENC_UNKNOWN_ERROR;
      } else {
        usacExtElementPayloadLength = (int)robitbuf_ReadBits(&bitstreamReader, 8);
        aprExtElementBits += 8;

        if (255 == usacExtElementPayloadLength) {
          const int valueAdd = (int)robitbuf_ReadBits(&bitstreamReader, 16);
          usacExtElementPayloadLength += valueAdd - 2;
          aprExtElementBits += 16;
        }

        aprExtElementBits += usacExtElementPayloadLength * BITS_PER_BYTE;
      }
    }

    if ((0 == usacIndependencyFlag) && (1 == usacExtElementPresent)) {
      *usacIndepFlag = 0;
      *audioPreRollExtElementPresent = 0;
      *audioPreRollExtElementBits = 0;
      *audioPreRollStartIndex = 0;
      error = AACENC_UNKNOWN_ERROR;
    } else {
      *usacIndepFlag = usacIndependencyFlag;
      *audioPreRollExtElementPresent = usacExtElementPresent;
      *audioPreRollExtElementBits = aprExtElementBits;
      *audioPreRollStartIndex = index_of_usacExtElementPresent;
    }
  }

  return error;
}

static AACFASTENC_ERROR iisaacfenc_removeExtensionElementAudioPreRoll(
    unsigned char* const bitstreamBuffer,
    int const bufferSizeBytes,
    int* const numValidBufferBits,
    unsigned int* const usacByteAlignmentBits) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  int usacIndependencyFlag = 0;
  int audioPreRollExtElementPresent = 0;
  int audioPreRollExtElementBits = 0;
  int audioPreRollStartIndex = 0;

  if (!isError(error)) {
    if ((NULL == bitstreamBuffer) || (NULL == numValidBufferBits) || (NULL == usacByteAlignmentBits)) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    if ((*numValidBufferBits % BITS_PER_BYTE) != 0) {
      error = AACENC_UNKNOWN_ERROR;
    }
  }

  if (!isError(error)) {
    if (*usacByteAlignmentBits >= BITS_PER_BYTE) {
      error = AACENC_UNKNOWN_ERROR;
    }
  }

  if (!isError(error)) {
    if (*numValidBufferBits / BITS_PER_BYTE > bufferSizeBytes) {
      error = AACENC_BUFSIZE_ERROR;
    }
  }

  if (!isError(error)) {
    error = iisaacfenc_extractAudioPreRollInformationFromBitstream(bitstreamBuffer,
                                                                   bufferSizeBytes,
                                                                   &usacIndependencyFlag,
                                                                   &audioPreRollExtElementPresent,
                                                                   &audioPreRollExtElementBits,
                                                                   &audioPreRollStartIndex);
  }

  if (!isError(error) && audioPreRollExtElementPresent && usacIndependencyFlag) {
    robitbuf bitstreamReader;
    wobitbuf bitstreamWriter;
    int bit;
    int bitsWrittenBeforeByteAlignment;
    const int nBitsTrailing = *numValidBufferBits - (audioPreRollStartIndex + audioPreRollExtElementBits) - *usacByteAlignmentBits;

    robitbuf_Init(&bitstreamReader, bitstreamBuffer, bufferSizeBytes * BITS_PER_BYTE, audioPreRollStartIndex + audioPreRollExtElementBits);

    wobitbuf_Init(&bitstreamWriter, bitstreamBuffer, bufferSizeBytes * BITS_PER_BYTE, 0);
    wobitbuf_Seek(&bitstreamWriter, audioPreRollStartIndex);

    wobitbuf_WriteBits(&bitstreamWriter, 0, 1);

    for (bit = 0; bit < nBitsTrailing; bit++) {
      unsigned int readBit = robitbuf_ReadBits(&bitstreamReader, 1);
      wobitbuf_WriteBits(&bitstreamWriter, readBit, 1);
    }

    bitsWrittenBeforeByteAlignment = wobitbuf_GetBitsWritten(&bitstreamWriter);

    wobitbuf_ByteAlignSet(&bitstreamWriter, 0);

    *usacByteAlignmentBits = (unsigned int)(wobitbuf_GetBitsWritten(&bitstreamWriter) - bitsWrittenBeforeByteAlignment);

    *numValidBufferBits = wobitbuf_GetBitsWritten(&bitstreamWriter) + audioPreRollStartIndex;

    memset(bitstreamBuffer + (*numValidBufferBits / BITS_PER_BYTE), 0, (size_t)bufferSizeBytes - (size_t)(*numValidBufferBits / BITS_PER_BYTE));
  }

  return error;
}

static AACFASTENC_ERROR iisaacfenc_prepareRemoveExtensionElements(
    unsigned char const* const bitstreamBuffer,
    unsigned char* const bitstreamBufferWoExtElements,
    int const bufferSizeBytes,
    int const numValidBufferBits,
    int* const numValidBufferBitsWoExtElements) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  const int origAuSizeInBytes = (numValidBufferBits + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE;

  if (!isError(error)) {
    if ((NULL == bitstreamBuffer) || (NULL == bitstreamBufferWoExtElements) || (NULL == numValidBufferBitsWoExtElements)) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    if (origAuSizeInBytes > bufferSizeBytes) {
      error = AACENC_BUFSIZE_ERROR;
    }
  }

  if (!isError(error)) {
    if ((numValidBufferBits % BITS_PER_BYTE) != 0) {
      error = AACENC_UNKNOWN_ERROR;
    }
  }

  if (!isError(error)) {
    memset(bitstreamBufferWoExtElements, 0, bufferSizeBytes);

    memcpy(bitstreamBufferWoExtElements, bitstreamBuffer, origAuSizeInBytes);

    *numValidBufferBitsWoExtElements = origAuSizeInBytes * BITS_PER_BYTE;
    ;
  }
  return error;
}

static AACFASTENC_ERROR iisaacfenc_removeExtensionElementsFromAU(
    unsigned char const* const bitstreamBuffer,
    unsigned char* const bitstreamBufferWoExtElements,
    int const bufferSizeBytes,
    int const numValidBufferBits,
    int* const numValidBufferBitsWoExtElements,
    unsigned int* const usacByteAlignmentBits,
    BS_EXT_ELE_FIL_INFO const* const extEleFilInfo,
    AACENC_IPF_STATE const ipfState) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  const AACENC_EXTELEREMOVAL_STATE extElementState = iisaacfenc_detectExtensionElementRemoval(ipfState,
                                                                                              bitstreamBufferWoExtElements,
                                                                                              numValidBufferBitsWoExtElements);

  if (!isError(error)) {
    if ((NULL == bitstreamBuffer) || (NULL == bitstreamBufferWoExtElements) || (NULL == numValidBufferBitsWoExtElements) || (NULL == extEleFilInfo)) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (!isError(error)) {
    if (NULL == usacByteAlignmentBits) {
      error = AACENC_INVALID_POINTER_ERROR;
    }
  }

  if (extElementState == AACENC_EXTELEREMOVAL_STATE_REMOVE) {
    if (!isError(error)) {
      error = iisaacfenc_prepareRemoveExtensionElements(bitstreamBuffer,
                                                        bitstreamBufferWoExtElements,
                                                        bufferSizeBytes,
                                                        numValidBufferBits,
                                                        numValidBufferBitsWoExtElements);
    }

    if (!isError(error)) {
      error = iisaacfenc_removeExtensionElementFill(bitstreamBufferWoExtElements,
                                                    bufferSizeBytes,
                                                    numValidBufferBitsWoExtElements,
                                                    extEleFilInfo,
                                                    usacByteAlignmentBits);
    }

    if (!isError(error)) {
      error = iisaacfenc_removeExtensionElementAudioPreRoll(bitstreamBufferWoExtElements,
                                                            bufferSizeBytes,
                                                            numValidBufferBitsWoExtElements,
                                                            usacByteAlignmentBits);
    }
  } else {
    if (!isError(error)) {
      *numValidBufferBitsWoExtElements = 0;
    }
  }

  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetInfo(
    const AACENC_ENCODER_HANDLE hAacEnc,
    AACENC_INFO* pInfo) {
  if (hAacEnc && pInfo) {
    pInfo->bandWidth = hAacEnc->hConfig->bandWidth;
    pInfo->nDelay = hAacEnc->nDelay;
    pInfo->nAddEncDelay = hAacEnc->nAddEncDelay;
    pInfo->nStandDelay = hAacEnc->nStandDelay;
    pInfo->cbBufSizeMin = hAacEnc->cbBufSizeMin;

    pInfo->nAncBytesPerFrame = 0;

    strcpy(pInfo->version, AAC_LIBRARY_VERSION);
    return (AACENC_NO_ERROR);
  }

  return (AACENC_UNKNOWN_ERROR);
}

static int iisaacfenc_ReAssignChannels(CHANNEL_MAPPING* chMap,
                                       const unsigned int* const channelOffsets) {
  int err = 0;
  int el, ch;
  int chCnt = 0;
  int checkSum = 0;

  if (channelOffsets != NULL) {
    for (el = 0; el < chMap->nElements; el++) {
      switch (chMap->elInfo[el].elType) {
        case ID_SCE:
        case ID_LFE:
        case ID_CPE:
          for (ch = 0; ch < chMap->elInfo[el].nChannelsInEl; ch++) {
            chMap->elInfo[el].ChannelIndex[ch] = channelOffsets[chCnt];
            checkSum += channelOffsets[chCnt];
            chCnt++;
          }
          break;
        default:

          break;
      }
    }
    if (checkSum != ((chMap->nChannels * (chMap->nChannels - 1)) / 2))
      err = 1;

  } else {
    err = 1;
  }

  return err;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetOffsets(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const unsigned int* const channelOffsets) {
  int err = 0;

  if (hAacEnc != NULL) {
    err = iisaacfenc_ReAssignChannels(hAacEnc->channelMapping, channelOffsets);
    if (err == 0) {
      BS_CHANNEL_MAPPING* bs_ch_map;
      bs_ch_map = &hAacEnc->hBsEnc->config.channelMapping;
      iisaacfenc_reassignBsChannelMapping(bs_ch_map, hAacEnc->channelMapping);
    }
  } else {
    err = 1;
  }

  return err;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetBitReservoirInfo(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const bitReservoirMax,
    int* const bitReservoir,
    float* const bitReservoirLevel) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if ((NULL == hAacEnc) || (NULL == bitReservoirMax) || (NULL == bitReservoir) || (NULL == bitReservoirLevel)) {
    error = AACENC_INVALID_POINTER_ERROR;
  } else {
    const IISBITFRAME_HANDLE hBitFrame = hAacEnc->hBitFrame;
    const int extBitResMax = IISBITFRAME_GetExtendedBitreservoirMax(hBitFrame);
    const int extBitRes = IISBITFRAME_GetExtendedBitreservoir(hBitFrame);

    IISBITFRAME_GetBitreservoirInfo(hBitFrame, bitReservoir, bitReservoirMax);

    (*bitReservoir) += extBitRes;
    (*bitReservoirMax) += extBitResMax;

    if (0 != (*bitReservoirMax)) {
      (*bitReservoirLevel) = (float)(*bitReservoir) / (float)(*bitReservoirMax);
    } else {
      (*bitReservoirLevel) = 0.0f;
    }
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetAvgFrameSize(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const avgBitsPerFrame) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if ((NULL == hAacEnc) || (NULL == avgBitsPerFrame)) {
    error = AACENC_INVALID_POINTER_ERROR;
  } else {
    *avgBitsPerFrame = IISBITFRAME_GetAverageBitsPerFrame(hAacEnc->hBitFrame);
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetBitReservoirLevel(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float bitReservoirLevel) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  assert(!isVbr(hAacEnc->bitrateMode));

  if ((NULL == hAacEnc) || (bitReservoirLevel < 0.0f) || (bitReservoirLevel > 1.0)) {
    error = AACENC_INIT_ERROR;
  } else {
    int bitResMax = IISBITFRAME_GetBitreservoirMax(hAacEnc->hBitFrame);
    int extBitResMax = IISBITFRAME_GetExtendedBitreservoirMax(hAacEnc->hBitFrame);

    const int totalTemp = (int)((float)(bitResMax + extBitResMax) * bitReservoirLevel + 4.f) / 8;
    int bitresTemp = (int)((float)bitResMax * bitReservoirLevel + 4.f) / 8;
    int extBitresTemp = (int)((float)extBitResMax * bitReservoirLevel) / 8;

    assert(bitResMax % 8 == 0);
    assert(extBitResMax % 8 == 0);

    assert(totalTemp - (bitresTemp + extBitresTemp) <= 2);
    assert(totalTemp - (bitresTemp + extBitresTemp) >= 0);

    while (totalTemp > bitresTemp + extBitresTemp) {
      int delta = totalTemp - (bitresTemp + extBitresTemp);

      if (extBitResMax > (extBitresTemp * 8)) {
        extBitresTemp++;
        delta--;
      }
      if ((delta > 0) && (bitResMax > (bitresTemp * 8))) {
        bitresTemp++;
      }
    }
    assert(totalTemp == (bitresTemp + extBitresTemp));
    assert(extBitResMax >= (extBitresTemp * 8));
    assert(bitResMax >= bitresTemp * 8);
    assert(bitResMax + extBitResMax >= (bitresTemp + extBitresTemp) * 8);

    IISBITFRAME_UpdateBitReservoir(hAacEnc->hBitFrame, bitresTemp * 8);
    IISBITFRAME_UpdateExtendedBitReservoir(hAacEnc->hBitFrame, extBitresTemp * 8);
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncForceBitReservoirFilling(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int bForceBitResFilling) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (NULL == hAacEnc) {
    error = AACENC_INIT_ERROR;
  } else {
    hAacEnc->bForceBitResFilling = bForceBitResFilling;
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_ExtendedBitReservoirSetFillrate(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float const averageFrameRatio) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (NULL == hAacEnc) {
    error = AACENC_INIT_ERROR;
  } else {
    const int averageBitsPerFrame = IISBITFRAME_GetAverageBitsPerFrame(hAacEnc->hBitFrame);
    int currFillRate = IISBITFRAME_GetExtendedBitreservoirFillRate(hAacEnc->hBitFrame);
    int fillRate = max(currFillRate, (int)(averageBitsPerFrame * averageFrameRatio) / 8 * 8);
    IISBITFRAME_ERROR errorInfo = IISBITFRAME_SetExtendedBitReservoirFillRate(hAacEnc->hBitFrame, fillRate);
    if (errorInfo != IISBITFRAME_NO_ERROR) {
      error = AACENC_UNKNOWN_ERROR;
    }
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetMinFrameBytes(
    AACENC_ENCODER_HANDLE hAacEnc,
    int minFrameBytes) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (minFrameBytes > 0) {
    hAacEnc->minFrameBytes = minFrameBytes;
    hAacEnc->qcKernel->forcePE = iisaacfenc_bits2pe((float)minFrameBytes * 8);
    hAacEnc->hBsEnc->frameData.minFrameBytes = minFrameBytes;
  } else {
    hAacEnc->minFrameBytes = 0;
    hAacEnc->qcKernel->forcePE = 0.f;
    hAacEnc->hBsEnc->frameData.minFrameBytes = 0;
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncResetRateControl(
    AACENC_ENCODER_HANDLE hAacEnc) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (NULL == hAacEnc) {
    error = AACENC_INIT_ERROR;
  } else {
    IISBITFRAME_ERROR retValueBitFrame = IISBITFRAME_ResetRateControl(hAacEnc->hBitFrame);
    error = mappingBitFrameErrorToAacError(retValueBitFrame);
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncEncodeFrame(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float* pSamples,
    const int nSamples,
    unsigned char* const pOutput,
    unsigned char* const pOutputApr,
    const int cbSize,
    int* const cbOutBits,
    int* const cbOutBitsApr,
    AACENC_CODING_MODE* codingModeNext,
    const HANDLE_UNISTE* phUniSte,
    unsigned char* pMetadataData,
    unsigned int* pNumMetadataData,
    const unsigned int metadataMaxBytesPerFrame,
    unsigned int nBitsTransportOverhead,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER* hExternalContainer,
    int numExternalContainersInUse,
    const AACENC_IPF_STATE ipfState

) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  HANDLE_ERROR_INFO err = NULL;

  if (hAacEnc == NULL || pSamples == NULL) {
    error = AACENC_INIT_ERROR;
  } else {
    for (int i = 0; i < nSamples; i++) {
      if (isnan(pSamples[i]) || fabs(pSamples[i]) > MAX_ABS_SAMPLE_RANGE) {
        error = AACENC_ILLEGAL_PARAMETER_ERROR;
        break;
      }
    }
  }

  if (AACENC_NO_ERROR == error) {
    int totDynBitsAcelp = 0;
    int nBitsAcelp[SIGMAP_MAX_ELEMENTS] = {0};
    int totFacBits = 0;
    int bSwitchedModes = 0;
    int auLength = 0;
    int headerBits = 0;
    int nBitsPce = 0;
    int bitresSaveBits = 0;
    int chIdx[SIGMAP_MAX_SIGNALS_PER_ELEMENT] = {0};
    int nBitsMpegsPayload = 0;
    int externalElemBits = 0;
    int maxBitsToUseLpd = 0;
    float* pTimeSignal = NULL;
    int maximumNumberOfBitsForThisFrame = 0;
    int bitReservoir = 0;
    int bitReservoirMax = 0;
    int frameStatus = 0;

    INTERN_CORE_MODE coreModePrev[SIGMAP_MAX_SIGNALS] = {INTERN_CORE_MODE_FD};
    INTERN_CORE_MODE coreMode[SIGMAP_MAX_SIGNALS] = {INTERN_CORE_MODE_FD};
    INTERN_CORE_MODE coreModeNext[SIGMAP_MAX_SIGNALS] = {INTERN_CORE_MODE_FD};

    int isAce[2] = {0};
    int isAceNext[2] = {0};
    int isAcePrev[2] = {0};
    int nBitsOutOfBand = 0;

    CHANNEL_MAPPING* cm = hAacEnc->channelMapping;
    PSY_CONFIGURATION* psyConf0 = &hAacEnc->psyKernel->psyConf[0];
    PSY_CONFIGURATION* psyConf1 = &hAacEnc->psyKernel->psyConf[1];
    AACENC_CODEC_TYPE codecType = hAacEnc->hConfig->codecType;

    if (NULL != phUniSte) {
      int const nElements = cm->nElements;
      int const nCh = cm->nChannels;
      int el = 0;
      int ch = 0;
      int i = 0;
      float tmp = 0.f;

      for (el = 0; el < nElements; el++) {
        if (NULL != phUniSte[el]) {
          assert(cm->elInfo[el].nChannelsInEl == 2);
          if (phUniSte[el]->bPseudoLr == 1) {
            for (i = 0; i < nSamples / cm->nChannels; i++) {
              tmp = (pSamples[nCh * i + ch] + pSamples[nCh * i + 1 + ch]) / (float)sqrt(2.0f);
              pSamples[nCh * i + 1 + ch] = (pSamples[nCh * i + ch] - pSamples[nCh * i + 1 + ch]) / (float)sqrt(2.0f);
              pSamples[nCh * i + ch] = tmp;
            }
          }
        }
        ch += cm->elInfo[el].nChannelsInEl;
      }
    }

    if (hAacEnc->useCpuOptimisation != 0) {
      if (iisaacfenc_AacEncStackAlignment_Check() != 0) {
        return AACENC_STACK_ALIGNMENT_ERROR;
      }
    }

    if (((int)hAacEnc->nGranuleLength) * cm->nChannels != nSamples) {
      return AACENC_NSAMPLES_ERROR;
    }

    if (!error) {
      nBitsOutOfBand = 0;
      for (int i = 0; i < numExternalContainersInUse; ++i) {
        if (hasExtensionPayloadContainerFeature(hExternalContainer[i], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
          nBitsOutOfBand += getTotalSize_extPayload(hExternalContainer[i]);
        }
      }

      error = calculateHeaderBits(hAacEnc, ((nBitsOutOfBand + 7) / 8) * 8, &headerBits);
      headerBits += nBitsTransportOverhead;
      nBitsOutOfBand = 0;
    }

    if (!error) {
      int ch;
      if (codingModeNext != NULL) {
        for (ch = 0; ch < cm->nChannels; ch++) {
          switch (codingModeNext[ch]) {
            case AACENC_CODING_MODE_FD:
              coreModeNext[ch] = INTERN_CORE_MODE_FD;
              break;
            case AACENC_CODING_MODE_LPD:
              coreModeNext[ch] = INTERN_CORE_MODE_LPD;
              break;
            default:
              error = 1;
              assert(0);
              break;
          }
        }
      }

      if (((codecType == AACENC_CODEC_XHEAAC) || (codecType == AACENC_CODEC_MPEGH)) &&
          (hAacEnc->hConfig->lpdCodingMode != LPD_WRAPPER_CODING_MODE_INVALID) &&
          (hAacEnc->hConfig->bitRate <= 6000)) {
        for (ch = 0; ch < cm->nChannels; ch++) {
          iisaacfenc_wrap_lpd_set_restricted_mode(hAacEnc->hLpdWrapper,
                                                  ch,
                                                  (coreModeNext[ch] == INTERN_CORE_MODE_LPD) ? 0xff : 0xfe);

          coreModeNext[ch] = INTERN_CORE_MODE_LPD;
        }
      } else if (((codecType == AACENC_CODEC_XHEAAC) || (codecType == AACENC_CODEC_MPEGH)) &&
                 (hAacEnc->hConfig->lpdCodingMode != LPD_WRAPPER_CODING_MODE_INVALID) &&
                 (hAacEnc->hConfig->bitRate <= 8000)) {
        for (ch = 0; ch < cm->nChannels; ch++) {
          coreModeNext[ch] = INTERN_CORE_MODE_LPD;
        }
      }
    }

    if (!error && !isVbr(hAacEnc->bitrateMode)) {
      IISBITFRAME_FramePadding(hAacEnc->hBitFrame);
    }

    if (!error) {
      error = iisaacfenc_psyFeedInputBufferFloat(pSamples,
                                                 nSamples,
                                                 hAacEnc->psyKernel,
                                                 cm);
    }

    if (!isError(error)) {
      int el = 0;
      int ch = 0;

      if (AACENC_SAP_TYPE_WDWTYPE_HIGHBW == hAacEnc->syncFrame) {
        hAacEnc->sfbActiveLongBackup = psyConf0->sfbActive;
        hAacEnc->sfbActiveShortBackup = psyConf1->sfbActive;
        hAacEnc->lpLineLongBackup = psyConf0->lowpassLine;
        hAacEnc->lpLineShortBackup = psyConf1->lowpassLine;

        psyConf0->sfbActive = psyConf0->sfbCnt;
        psyConf1->sfbActive = psyConf1->sfbCnt;
        psyConf0->lowpassLine = psyConf0->sfbOffset[psyConf0->sfbCnt];
        psyConf1->lowpassLine = psyConf1->sfbOffset[psyConf1->sfbCnt];
      }

      for (el = 0; el < cm->nElements; el++) {
        ELEMENT_INFO elInfo = cm->elInfo[el];
        chIdx[0] = elInfo.ChannelIndex[0];
        chIdx[1] = elInfo.ChannelIndex[1];

        if (!isError(error) &&
            (cm->elInfo[el].elType == ID_SCE ||
             cm->elInfo[el].elType == ID_CPE ||
             cm->elInfo[el].elType == ID_LFE)) {
          PSY_DATA* psyData[2] = {NULL, NULL};
          TNS_DATA* tnsData[2] = {NULL, NULL};
          PSY_OUT_CHANNEL* psyOutChannel[2] = {NULL, NULL};

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            hAacEnc->psyKernel->psyData[elInfo.ChannelIndex[ch]]->isLFE = (elInfo.elType == ID_LFE);
          }

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            psyData[ch] = hAacEnc->psyKernel->psyData[chIdx[ch]];
            tnsData[ch] = hAacEnc->psyKernel->tnsData[chIdx[ch]];
            psyOutChannel[ch] = hAacEnc->psyOut->psyOutChannel[chIdx[ch]];

            coreMode[chIdx[ch]] = hAacEnc->coreMode[chIdx[ch]];
            coreModePrev[chIdx[ch]] = hAacEnc->coreModePrev[chIdx[ch]];
            hAacEnc->coreModePrev[chIdx[ch]] = hAacEnc->coreMode[chIdx[ch]];
            hAacEnc->coreMode[chIdx[ch]] = coreModeNext[chIdx[ch]];

            isAce[ch] = (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD);
            isAceNext[ch] = (coreModeNext[chIdx[ch]] == INTERN_CORE_MODE_LPD);
            isAcePrev[ch] = (coreModePrev[chIdx[ch]] == INTERN_CORE_MODE_LPD);
          }

          if (!isError(error)) {
            error = iisaacfenc_psyMain(codecType,
                                       elInfo.nChannelsInEl,
                                       psyData,
                                       tnsData,
                                       hAacEnc->psyKernel->psyConf,
                                       psyOutChannel,
                                       hAacEnc->psyOut->psyOutElement[el],
                                       hAacEnc->psyKernel->pScratch,
                                       hAacEnc->psyKernel->hLappedTransform,
                                       (AACENC_SAP_TYPE_NONE != hAacEnc->syncFrame),
                                       isAcePrev,
                                       isAce,
                                       isAceNext,
                                       bUsacIndepFlag,
                                       (phUniSte) ? phUniSte[el] : 0);
          }

          if (!isError(error)) {
            for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
              if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) {
                psyOutChannel[ch]->sfbActive = 0;
                psyOutChannel[ch]->maxSfbPerGroup = 0;
              }
            }
          }
        }
      }

      if (!isError(error)) {
        for (el = 0; el < cm->nElements; el++) {
          ELEMENT_INFO elInfo = cm->elInfo[el];
          chIdx[0] = elInfo.ChannelIndex[0];
          chIdx[1] = elInfo.ChannelIndex[1];

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD) {
              BIT_ENC_CHANNEL_DATA* bEncData = &hAacEnc->hBsEnc->bitEncChannelData[chIdx[ch]];

              iisaacfenc_transferBitEncChannelSideData(bEncData,
                                                       hAacEnc->psyOut->psyOutChannel[chIdx[ch]],
                                                       hAacEnc->psyOut->psyOutElement[el],
                                                       codecType);
            }
          }
        }
      }

      if (!isError(error)) {
        unsigned int metadataBytes2Write = 0;
        if (pNumMetadataData != NULL) {
          if (*pNumMetadataData != 0) {
            BS_MAPPING_ELEMENT* elem = NULL;
            BS_CHANNEL_MAPPING bsCm = hAacEnc->hBsEnc->config.channelMapping;

            for (el = bsCm.noOfElements - 1; el >= 0; el--) {
              elem = bsCm.element[el];
              if ((elem->elementType == ID_DSE) && (elem->element.dse.is_drc == 1)) break;
            }

            if (elem != NULL) {
              assert(elem->elementType == ID_DSE);
              assert(elem->element.dse.is_drc == 1);
            }

            assert(pNumMetadataData);
            assert(pMetadataData);

            assert(*pNumMetadataData <= metadataMaxBytesPerFrame);

            metadataBytes2Write = min(*pNumMetadataData, metadataMaxBytesPerFrame);

            elem->element.dse.byteCount = metadataBytes2Write;
            memcpy(elem->element.dse.data_stream_byte, pMetadataData, elem->element.dse.byteCount);
          }
        }

        if (hExternalContainer != NULL) {
          error = aacEncDistributeExtensionPayload(hExternalContainer,
                                                   hAacEnc->additionalElemBits,
                                                   numExternalContainersInUse,
                                                   &nBitsOutOfBand,
                                                   &externalElemBits,
                                                   &nBitsMpegsPayload,
                                                   cm);
        }
      }

      if (!isError(error)) {
        if (isVbr(hAacEnc->bitrateMode)) {
          maximumNumberOfBitsForThisFrame = getAverageBitsPerVbrFrame(hAacEnc);
          maximumNumberOfBitsForThisFrame += getLPDHeadroom(hAacEnc->hConfig->bitRate,
                                                            hAacEnc->hConfig->channelMapping->nChannels,
                                                            hAacEnc->hConfig->sampleRate,
                                                            hAacEnc->nGranuleLength);
        } else {
          IISBITFRAME_GetBitreservoirInfo(hAacEnc->hBitFrame,
                                          &bitReservoir,
                                          &bitReservoirMax);
          maximumNumberOfBitsForThisFrame = IISBITFRAME_GetNumberOfBitsForCurrentFrame(hAacEnc->hBitFrame, 1) + bitReservoir;
        }
        if (ipfState == AACENC_IPF_STATE_RAP_IPF || ipfState == AACENC_IPF_STATE_RAP_IPF_PREROLL || ipfState == AACENC_IPF_STATE_CONFIGCHANGE_IPF) {
          maximumNumberOfBitsForThisFrame += IISBITFRAME_GetExtendedBitreservoir(hAacEnc->hBitFrame);
        }
      }

      if (bUsacIndepFlag) {
        frameStatus |= USAC_INDEPENDENCE_FLAG;
      }
      if (ipfState == AACENC_IPF_STATE_RAP_IPF || ipfState == AACENC_IPF_STATE_RAP_IPF_PREROLL || ipfState == AACENC_IPF_STATE_CONFIGCHANGE_IPF) {
        frameStatus |= RAP_FRAME;
      }

      if (!isError(error)) {
        err = AdvanceBitstreamEncoder(hAacEnc->hBsEnc,
                                      nBitsMpegsPayload,
                                      hAacEnc->additionalElemBits,
                                      externalElemBits,
                                      headerBits + nBitsPce,
                                      0,
                                      (const int*)coreMode,
                                      frameStatus,
                                      hAacEnc->hBitFrame,
                                      isVbr(hAacEnc->bitrateMode));

        if (noError != err) {
          error = AACENC_UNKNOWN_ERROR;
          freeErrorTraceback(err);
        }
      }

      for (el = 0; el < cm->nElements; el++) {
        ELEMENT_INFO elInfo = cm->elInfo[el];
        chIdx[0] = elInfo.ChannelIndex[0];
        chIdx[1] = elInfo.ChannelIndex[1];

        if ((!isError(error)) &&
            (cm->elInfo[el].elType == ID_SCE ||
             cm->elInfo[el].elType == ID_CPE ||
             cm->elInfo[el].elType == ID_LFE)) {
          PSY_DATA* psyData[2] = {NULL, NULL};

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            psyData[ch] = hAacEnc->psyKernel->psyData[chIdx[ch]];
          }

          if ((hAacEnc->hLpdWrapper != NULL) && (!isError(error))) {
            LPD_WRAPPER_IPF_STATE mappedIpfState = LPD_WRAPPER_IPF_STATE_NO;
            LPD_WRAPPER_ERROR wrapperError = LPD_WRAPPER_NO_ERROR;

            wrapperError = iisaacfenc_calculate_max_bits_to_use(hAacEnc->hLpdWrapper,
                                                                hExternalContainer,
                                                                numExternalContainersInUse,
                                                                el,
                                                                headerBits,
                                                                maximumNumberOfBitsForThisFrame,
                                                                (unsigned int*)&maxBitsToUseLpd);
            error = mappingLpdErrorToAacError(wrapperError);

            if (!isError(error)) {
              error = mappingIpfStateFromAacToLpdWrapper(ipfState, &mappedIpfState);
            }

            if (!isError(error)) {
              wrapperError = iisaacfenc_wrap_lpd_process(hAacEnc->hLpdWrapper,
                                                         bitReservoir,
                                                         bitReservoirMax,
                                                         hAacEnc->hBsEnc,
                                                         psyData,
                                                         el,
                                                         coreMode,
                                                         coreModePrev,
                                                         coreModeNext,
                                                         maxBitsToUseLpd,
                                                         bUsacIndepFlag,
                                                         mappedIpfState

              );
              error = mappingLpdErrorToAacError(wrapperError);
            }
          }

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            PSY_DATA* psyDataChannel = hAacEnc->psyKernel->psyData[chIdx[ch]];
            PSY_CONFIGURATION* psyConf = hAacEnc->psyKernel->psyConf;
            PSY_OUT_CHANNEL* psyOutChannel = hAacEnc->psyOut->psyOutChannel[chIdx[ch]];

            pTimeSignal = MP4TIMEBUF_AccessBuffer(psyDataChannel->psyInputBuffer, TRANSFORM_OFFSET_LONG, 0);
            setFLOAT(0.0f, psyOutChannel->origTimeSig, 2 * (FRAME_LEN_LONG));
            copyFLOAT(pTimeSignal, psyOutChannel->origTimeSig, 2 * psyConf[0].granuleLength);
          }

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            HANDLE_ERROR_INFO tmpError = noError;
            PSY_DATA* psyDataChannel = hAacEnc->psyKernel->psyData[chIdx[ch]];
            PSY_CONFIGURATION* psyConf = hAacEnc->psyKernel->psyConf;

            tmpError = MP4TIMEBUF_InvalidateBuffer(psyDataChannel->psyInputBuffer, psyConf[0].granuleLength);
            if (tmpError != noError) {
              freeErrorTraceback(tmpError);
            }
          }
        }
      }

      {
        for (el = 0; (el < cm->nElements) && !isError(error); el++) {
          ELEMENT_INFO elInfo = cm->elInfo[el];
          chIdx[0] = elInfo.ChannelIndex[0];
          chIdx[1] = elInfo.ChannelIndex[1];
          nBitsAcelp[el] = 0;

          if ((elInfo.elType == ID_SCE) || (elInfo.elType == ID_CPE)) {
            for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
              if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) {
                LPD_WRAPPER_ERROR wrapperError = LPD_WRAPPER_NO_ERROR;
                BIT_ENC_CHANNEL_DATA* bEncData = &hAacEnc->hBsEnc->bitEncChannelData[elInfo.ChannelIndex[ch]];

                wrapperError = iisaacfenc_wrap_lpd_transfer_acelp_data(hAacEnc->hLpdWrapper,
                                                                       chIdx[ch],
                                                                       bEncData->acelpData,
                                                                       &bEncData->acelpDataBitCnt);

                error = mappingLpdErrorToAacError(wrapperError);
                totDynBitsAcelp += bEncData->acelpDataBitCnt;
                nBitsAcelp[el] += bEncData->acelpDataBitCnt;
                bEncData->commonWindow = 0;
              }
            }
          }
        }
      }

      if (!isError(error)) {
        if ((hAacEnc->bForceBitResFilling == 1) && (IISBITFRAME_GetExtendedBitreservoir(hAacEnc->hBitFrame) == 0)) {
          float bitresFillLevel = 0.f;
          int bitresBits = 0;
          int maxbitresBits = 0;

          bitresBits = IISBITFRAME_GetBitreservoir(hAacEnc->hBitFrame);
          maxbitresBits = IISBITFRAME_GetBitreservoirMax(hAacEnc->hBitFrame);

          if (maxbitresBits > 0) {
            bitresFillLevel = ((float)bitresBits) / maxbitresBits;
            bitresSaveBits = (int)((1.0f - bitresFillLevel) * 0.1f * maxbitresBits);
            bitresSaveBits = min(bitresSaveBits, hAacEnc->hBsEnc->frameData.availableFrameBits / 8);
          }
        }
      }
      if (!isError(error)) {
        error = iisaacfenc_AdjustBitrate(hAacEnc->qcKernel,
                                         cm,
                                         hAacEnc->hBsEnc->frameData.availableFrameBits,
                                         hAacEnc->hBsEnc->frameData.additionalElemBits + nBitsMpegsPayload + hAacEnc->hBsEnc->frameData.dseBits + bitresSaveBits);
      }
    }

    iisaacfenc_ProcessMeBitDistribution(cm,
                                        hAacEnc->qcKernel,
                                        hAacEnc->psyOut,
                                        hAacEnc->qcOut,
                                        headerBits + nBitsPce,
                                        nBitsAcelp,
                                        (hAacEnc->hBsEnc->frameData.elemSideInfoBits),
                                        hAacEnc->hBitFrame,
                                        hAacEnc->additionalElemBits,
                                        hAacEnc->hConfig->bitDistributionMode,
                                        (AACENC_SAP_TYPE_WDWTYPE_HIGHBW == hAacEnc->syncFrame));

    if (!isError(error)) {
      error = iisaacfenc_QCMain(hAacEnc->psyOut,
                                hAacEnc->qcOut,
                                hAacEnc->qcKernel,
                                hAacEnc->hBsEnc,
                                hAacEnc->hBitFrame,
                                cm,
                                coreMode,
                                coreModePrev,
                                bUsacIndepFlag);
    }

    if (!isError(error)) {
      error = iisaacfenc_FinalizeBitConsumption(cm,
                                                hAacEnc->qcKernel,
                                                hAacEnc->qcOut, coreMode);
    }

    if (!isError(error)) {
      int el = 0;
      int ch;

      for (el = 0; (el < cm->nElements) && !isError(error); el++) {
        ELEMENT_INFO elInfo = cm->elInfo[el];
        chIdx[0] = elInfo.ChannelIndex[0];
        chIdx[1] = elInfo.ChannelIndex[1];

        if ((elInfo.elType == ID_SCE) || (elInfo.elType == ID_CPE)) {
          for (ch = 0; (ch < elInfo.nChannelsInEl) && !isError(error); ch++) {
            BIT_ENC_CHANNEL_DATA* bEncData = &hAacEnc->hBsEnc->bitEncChannelData[elInfo.ChannelIndex[ch]];

            if (coreModePrev[chIdx[ch]] == INTERN_CORE_MODE_FD && coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) {
              LPD_WRAPPER_ERROR wrapperError = LPD_WRAPPER_NO_ERROR;

              wrapperError = iisaacfenc_wrap_lpd_transfer_fac_data(hAacEnc->hLpdWrapper,
                                                                   chIdx[ch],
                                                                   bEncData->FacData,
                                                                   &bEncData->FacDataBitCnt);
              error = mappingLpdErrorToAacError(wrapperError);
              totFacBits += bEncData->FacDataBitCnt;
            } else {
              bEncData->FacDataBitCnt = 0;
            }
          }
        }
      }
    }

    if (!isError(error)) {
      int ch;

      bSwitchedModes = 0;

      if (cm->nChannels <= 2) {
        for (ch = 0; ch < cm->nChannels; ch++) {
          if ((coreMode[ch] == INTERN_CORE_MODE_FD) &&
              ((coreModeNext[ch] == INTERN_CORE_MODE_LPD) || (coreModePrev[ch] == INTERN_CORE_MODE_LPD))) {
            bSwitchedModes = 1;
          } else {
            bSwitchedModes = 0 || bSwitchedModes;
          }
        }
      }

      if (bSwitchedModes == 1) {
        int el = 0;

        error = iisaacfenc_AdvanceIntDecSaac(hAacEnc->hTfDec,
                                             hAacEnc->psyOut->psyOutChannel,
                                             hAacEnc->qcOut->qcChannel,
                                             hAacEnc->psyKernel->psyConf,
                                             cm->nChannels,
                                             cm->nElements,
                                             ((int*)coreModeNext),
                                             ((int*)coreModePrev),
                                             cm->elInfo,
                                             hAacEnc->psyOut->psyOutElement);

        for (el = 0; (el < cm->nElements) && !isError(error); el++) {
          ELEMENT_INFO elInfo = cm->elInfo[el];
          chIdx[0] = elInfo.ChannelIndex[0];
          chIdx[1] = elInfo.ChannelIndex[1];

          if ((elInfo.elType == ID_SCE) || (elInfo.elType == ID_CPE)) {
            for (ch = 0; (ch < elInfo.nChannelsInEl) && !isError(error); ch++) {
              LPD_WRAPPER_ERROR wrapperError = LPD_WRAPPER_NO_ERROR;
              BIT_ENC_CHANNEL_DATA* bEncData = &hAacEnc->hBsEnc->bitEncChannelData[elInfo.ChannelIndex[ch]];
              int lastSubFrameWasLpd = 0;

              wrapperError = iisaacfenc_wrap_fac_process(hAacEnc->hLpdWrapper,
                                                         chIdx[0],
                                                         chIdx[ch],
                                                         hAacEnc->psyOut->psyOutChannel[chIdx[ch]],
                                                         hAacEnc->qcOut->qcChannel[chIdx[ch]],
                                                         coreModePrev[chIdx[ch]],
                                                         coreModeNext[chIdx[ch]],
                                                         hAacEnc->FacPrm[chIdx[ch]],
                                                         &hAacEnc->Nbits_fac[chIdx[ch]]);
              error = mappingLpdErrorToAacError(wrapperError);

              lastSubFrameWasLpd = iisaacfenc_wrap_lpd_last_sub_frame_was_lpd(hAacEnc->hLpdWrapper, chIdx[0]);

              if ((coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD) &&
                  (coreModePrev[chIdx[ch]] == INTERN_CORE_MODE_LPD) &&
                  (lastSubFrameWasLpd != 0)) {
                if (!isVbr(hAacEnc->bitrateMode)) {
                  int bitsLeftInBitRes = IISBITFRAME_GetBitreservoir(hAacEnc->hBitFrame) + hAacEnc->hBsEnc->frameData.availableDynpartBits - hAacEnc->qcOut->totDynBitsUsed - totDynBitsAcelp - totFacBits;
                  if (bitsLeftInBitRes - hAacEnc->Nbits_fac[chIdx[ch]] < 0) {
                    hAacEnc->Nbits_fac[chIdx[ch]] = 0;
                  }
                }

                memcpy(bEncData->FacData, hAacEnc->FacPrm[chIdx[ch]], (hAacEnc->Nbits_fac[chIdx[ch]] + 7) / 8);
                totFacBits += bEncData->FacDataBitCnt = hAacEnc->Nbits_fac[chIdx[ch]];
              }
            }
          }
        }
      }
    }
    hAacEnc->hBsEnc->frameData.totDynBits = hAacEnc->qcOut->totDynBitsUsed + totDynBitsAcelp + totFacBits;

    if (!isError(error)) {
      if (hAacEnc->headerBitsCallbackHandle != NULL) {
        error = computeHeaderLengthFillBytes(hAacEnc,
                                             headerBits,
                                             nBitsOutOfBand,
                                             nBitsPce);
      }
    }

    if (!isError(error)) {
      error = UpdateBitFrame(hAacEnc);
    }

    if (!isError(error)) {
      err = WriteBitstream(hAacEnc->hBsEnc,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           (int*)coreMode,
                           bUsacIndepFlag,
                           hExternalContainer,
                           numExternalContainersInUse);

      if (noError == err) {
        int elem = 0;
        for (elem = 0; elem < cm->nElements; elem++) {
          int channelIndex = cm->elInfo[elem].ChannelIndex[0];

          memcpy(hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.predCoefPrevReQ, hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.predCoefReQ, MAX_GROUPED_SFB * sizeof(int));
          memcpy(hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.predCoefPrevImQ, hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.predCoefImQ, MAX_GROUPED_SFB * sizeof(int));

          hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.bCplxPredMdctActivePrev = hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.bCplxPredMdctActive;
          hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.bCplxPredMdctRealOnlyPrev = hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.bCplxPredMdctRealOnly;
          hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.windowSequencePrev = hAacEnc->hBsEnc->bitEncChannelData[channelIndex].blockType;

          if (hAacEnc->hBsEnc->bitEncChannelData[channelIndex].grpSfb > 0) {
            hAacEnc->psyOut[0].psyOutElement[elem]->toolsInfo.nGroupsPrev = hAacEnc->hBsEnc->bitEncChannelData[channelIndex].sfbCnt / hAacEnc->hBsEnc->bitEncChannelData[channelIndex].grpSfb;
          }
        }
      }

      if (noError != err) {
        error = AACENC_UNKNOWN_ERROR;
        freeErrorTraceback(err);
      }

      if ((hExternalContainer != NULL) && !isError(error)) {
        error = resetExtensionPayload(hExternalContainer, numExternalContainersInUse);
      }

      if (!isError(error)) {
        err = DecAuReady(bsencGetBitstream(hAacEnc->hBsEnc), &auLength);
        if (noError != err) {
          error = AACENC_UNKNOWN_ERROR;
          freeErrorTraceback(err);
        }
      }
    }

    if (!isError(error)) {
      if (cbOutBits == NULL) {
        error = AACENC_UNKNOWN_ERROR;
      } else {
        *cbOutBits = 0;
      }
    }

    if (!isError(error)) {
      *cbOutBits = GetBitsAvail(bsencGetBitstream(hAacEnc->hBsEnc));
    }

    if (!isError(error)) {
      int nEffectiveChannels = cm->nEffectiveChannels;

      if (*cbOutBits - nBitsOutOfBand > MIN_BUFSIZE_PER_EFF_CHAN * nEffectiveChannels) {
        error = AACENC_UNKNOWN_ERROR;
      }
    }

    if (!isError(error)) {
      if (*cbOutBits > 8 * cbSize) {
        error = AACENC_BUFSIZE_ERROR;
      }
    }
    if (!isError(error)) {
      ReadBytes(bsencGetBitstream(hAacEnc->hBsEnc), pOutput, ((*cbOutBits) + 7) / 8);
    }

    if (!isError(error)) {
      if (pOutputApr && cbOutBitsApr) {
        error = iisaacfenc_removeExtensionElementsFromAU(pOutput,
                                                         pOutputApr,
                                                         cbSize,
                                                         *cbOutBits,
                                                         cbOutBitsApr,
                                                         &hAacEnc->hBsEnc->usacByteAlignmentBits,
                                                         &hAacEnc->hBsEnc->extEleFilInfo,
                                                         ipfState);
      }
    }

    if (!isError(error)) {
      if (AACENC_SAP_TYPE_WDWTYPE_HIGHBW == hAacEnc->syncFrame) {
        psyConf0->sfbActive = hAacEnc->sfbActiveLongBackup;
        psyConf1->sfbActive = hAacEnc->sfbActiveShortBackup;
        psyConf0->lowpassLine = hAacEnc->lpLineLongBackup;
        psyConf1->lowpassLine = hAacEnc->lpLineShortBackup;
      }
      hAacEnc->syncFrame = AACENC_SAP_TYPE_NONE;
    }
  }

  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSnapToLowSfbBorder(const int sampleRate,
                                    const AACENC_GRANULE_LEN granuleLength,
                                    const float desiredBandwidth,
                                    const float tol,
                                    float* adjBandwidth) {
  return iisaacfenc_snapToLowSfbBorder(sampleRate,
                                       LONG_WINDOW,
                                       granuleLength,
                                       desiredBandwidth,
                                       tol,
                                       adjBandwidth);
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetPceTimeInterval(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float* sendPceTimeInterval) {
  int error = AACENC_NO_ERROR;

  if (NULL == hAacEnc) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    if (hAacEnc->bWritePce == 0) {
      *sendPceTimeInterval = 0.f;
    } else if (hAacEnc->updateRatePce == -1) {
      *sendPceTimeInterval = -1.f;
    } else {
      *sendPceTimeInterval = (float)hAacEnc->updateRatePce * (float)hAacEnc->nGranuleLength / (float)hAacEnc->hConfig->sampleRate;
    }
  }

  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetPceTimeInterval(
    AACENC_ENCODER_HANDLE const hAacEnc,
    const float sendPceTimeInterval) {
  int error = AACENC_NO_ERROR;
  int bChannelConfigZero = 0;

  if (hAacEnc == NULL) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    switch (hAacEnc->channelMapping->nChannels) {
      case 1:
      case 2:
      case 6:
      case 8:
        bChannelConfigZero = 0;
        break;
      default:
        bChannelConfigZero = 1;
        break;
    }
  }
  if (!isError(error)) {
    if (sendPceTimeInterval >= (((float)INT_MAX * (float)hAacEnc->nGranuleLength) / (float)hAacEnc->hConfig->sampleRate)) {
      error = AACENC_INIT_ERROR;
    }
  }

  if (!isError(error)) {
    hAacEnc->bWritePce = 0;
    hAacEnc->updateRatePce = 0;
    hAacEnc->frameCntPce = 0;

    if (sendPceTimeInterval == 0.f) {
      if ((bChannelConfigZero == 1) &&
          (hAacEnc->hConfig->mpeg4Flag == 1)) {
        error = AACENC_INIT_ERROR;
      }
    } else if (sendPceTimeInterval > 0.f) {
      hAacEnc->bWritePce = 1;
      hAacEnc->updateRatePce = (int)(((float)hAacEnc->hConfig->sampleRate * sendPceTimeInterval) / ((float)hAacEnc->nGranuleLength));
      hAacEnc->updateRatePce = max(1, hAacEnc->updateRatePce);
    } else {
      hAacEnc->bWritePce = 1;

      hAacEnc->updateRatePce = -1;
    }
  }

  return error;
}

static void
iisaacfenc_ProcessMeBitDistribution(
    CHANNEL_MAPPING* cm,
    QC_STATE* hQC,
    PSY_OUT* psyOut,
    QC_OUT* qcOut,
    unsigned int nBitsTransportOverhead,
    int* nBitsAcelp,
    int* elemSideInfoBits,
    IISBITFRAME_HANDLE hBitFrame,
    int* pAdditionalElemBits,
    BIT_DISTRIBUTION_MODE bitDistributionMode,
    const int highBWframe) {
  unsigned int nBitsTransportOverheadAligned;
  int elementId, channelId;
  int nChannels;
  PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS];

  nBitsTransportOverheadAligned = ((nBitsTransportOverhead + 7) / 8) * 8;

  for (elementId = 0; elementId < cm->nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      nChannels = cm->elInfo[elementId].nChannelsInEl;

      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];

        iisaacfenc_CalcSingleFormFactor(psyOutChannel[channelId]->sfestimData,
                                        psyOutChannel[channelId]->sfbFormFactor,
                                        psyOutChannel[channelId]->mdctSpectrum,
                                        psyOutChannel[channelId]->sfbCnt,
                                        psyOutChannel[channelId]->sfbOffsets);
      }
    }
  }

  iisaacfenc_AdjustThresholds(cm,
                              hQC,
                              qcOut,
                              psyOut,
                              elemSideInfoBits,
                              hBitFrame,
                              pAdditionalElemBits,
                              bitDistributionMode,
                              nBitsTransportOverheadAligned,
                              nBitsAcelp,
                              highBWframe);
  return;
}

int AACENCAPI
IISAACFENC_AacEncSetBandwidth(
    AACENC_CONFIG_HANDLE const hAacConfig,
    const float proposedBandwidth,
    float* usedBandwidth) {
  int error = 0;
  float tmpUsedBandwidth = 0.0f;
  float ScaleBitrate = 1.0f;

  if (hAacConfig == NULL) {
    error = 1;
  }

  if (!error) {
    int aacCoreBitRate = hAacConfig->bitRate;

    if (aacCoreBitRate > 0) {
      aacCoreBitRate -= hAacConfig->ancDataBitRate;
    }

    if (aacCoreBitRate < 0) {
      error = 1;
    }

    if (!error) {
      if (hAacConfig->channelMapping->cicpLayoutIndex == SIGMAP_CUSTOMIZED_3_OVER_5_1) {
        ScaleBitrate = 5.0f / 3.0f;
      }
      if (hAacConfig->channelMapping->cicpLayoutIndex == SIGMAP_CUSTOMIZED_4_OVER_5_1) {
        ScaleBitrate = 5.0f / 4.0f;
      }
      error = iisaacfenc_DetermineBandWidth(&tmpUsedBandwidth,
                                            (int)proposedBandwidth,
                                            (int)(aacCoreBitRate * ScaleBitrate),
                                            0,
                                            hAacConfig->sampleRate,
                                            hAacConfig->channelMapping);
    }
  }

  if (!error) {
    hAacConfig->bandWidth = tmpUsedBandwidth;
    if (usedBandwidth != NULL) {
      *usedBandwidth = tmpUsedBandwidth;
    } else {
      error = 1;
    }
  }

  return error;
}

AACFASTENC_ERROR
IISAACFENC_SAPPrepare(AACENC_ENCODER_HANDLE const hAacEnc,
                      const AACENC_SAP_TYPE syncType) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hAacEnc == NULL) {
    error = AACENC_INIT_ERROR;
  }
  if (!isError(error)) {
    hAacEnc->syncFrame = syncType;
  }
  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetMpeg4Flag(AACENC_ENCODER_HANDLE const hAacEnc,
                              const int mpeg4Flag) {
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  hAacEnc->hConfig->mpeg4Flag = mpeg4Flag;

  return aacError;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncConfigureADTSPrivateAndOrigBit(AACENC_ENCODER_HANDLE const hAacEnc,
                                                const int PrivateBit,
                                                const int OrigCopy) {
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  hAacEnc->hConfig->adtsPrivateBit = PrivateBit;
  hAacEnc->hConfig->adtsOrigCopy = OrigCopy;

  return aacError;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetupADTSPrivateAndOrigBit(AACENC_CONFIG_HANDLE hAacConfig,
                                            const int PrivateBit,
                                            const int OrigCopy) {
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (hAacConfig) {
    hAacConfig->adtsPrivateBit = PrivateBit;
    hAacConfig->adtsOrigCopy = OrigCopy;
  } else {
    aacError = AACENC_UNKNOWN_ERROR;
  }
  return aacError;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetTransMux(AACENC_ENCODER_HANDLE const hAacEnc,
                             const AACENC_TRANS_MUX transMux,
                             const int calcCrc) {
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;
  AACENC_CONFIG_HANDLE hConfig = NULL;

  if (hAacEnc == NULL) return AACENC_INIT_ERROR;

  switch (transMux) {
    case AACENC_MUX_RAW:
      break;
    default:
      aacError = AACENC_UNKNOWN_ERROR;
  }

  if (!isError(aacError)) {
    hConfig = hAacEnc->hConfig;
    hConfig->calcCrc = calcCrc;
    switch (transMux) {
      case AACENC_MUX_RAW:
        hConfig->transMux = MUX_RAW;
        break;
    }
  }

  if (!isError(aacError)) {
    switch (hConfig->transMux) {
      case AACENC_MUX_RAW:

        break;

      default:

        aacError = AACENC_UNKNOWN_ERROR;
        break;
    }
  }

  return aacError;
}

void AACENCAPI
IISAACFENC_AacEncDelete(AACENC_ENCODER_HANDLE hAacEnc) {
  IISAACFENC_AacEncClose(hAacEnc);
}

static AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncNew(AACENC_ENCODER_HANDLE* phAacEnc) {
  int error = 0;
  int warning = AACENC_NO_ERROR;
  AACFASTENC_ERROR err = AACENC_NO_ERROR;
  AACENC_ENCODER* hAacEnc = NULL;
  int useCpuOptimisation = 0;

  if (phAacEnc == NULL) {
    return AACENC_INIT_ERROR;
  }

  if (!error) {
    hAacEnc = (AACENC_ENCODER*)iisCalloc(sizeof(AACENC_ENCODER), 1);
    if (hAacEnc == NULL) {
      error = 1;
    }
  }

  if (iisaacfenc_AacEncStackAlignment_Check() == 0) {
    InitMathOpt();
    useCpuOptimisation = 1;
  } else {
    warning = AACENC_WARNING_STACK_ALIGNMENT;
  }

  if (!error) {
    hAacEnc->useCpuOptimisation = useCpuOptimisation;
  }

  if (!error) {
    hAacEnc->channelMapping = (CHANNEL_MAPPING*)iisCalloc(sizeof(CHANNEL_MAPPING), 1);
    if (hAacEnc->channelMapping == NULL) {
      error = 1;
    }
  }

  if (!error) {
    hAacEnc->hAdditionalDataBitBuffer = CreateBitBuffer(MAX_FILL_ELEMENT_BITS * MAX_FILL_ELEMENTS);
    if (hAacEnc->hAdditionalDataBitBuffer == NULL) {
      error = 1;
    }
  }

  if (!error) {
    int i;
    for (i = 0; i < SIGMAP_MAX_SIGNALS; i++) {
      hAacEnc->coreMode[i] = INTERN_CORE_MODE_FD;
      hAacEnc->coreModePrev[i] = INTERN_CORE_MODE_FD;
    }
  }

  if (error) {
    IISAACFENC_AacEncDelete(hAacEnc);
    hAacEnc = NULL;
  } else {
    *phAacEnc = hAacEnc;
  }

  if (error) {
    err = AACENC_INIT_ERROR;
  }

  if (!isError(err)) {
    err = (AACFASTENC_ERROR)warning;
  }

  return err;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncUpdate(AACENC_ENCODER_HANDLE* phAacEnc,
                        const AACENC_CONFIG_HANDLE hConfig) {
  int error = 0;
  int warning = AACENC_NO_ERROR;
  AACFASTENC_ERROR err = AACENC_NO_ERROR;
  CHANNEL_MAPPING* cm = NULL;
  AACENC_ENCODER_HANDLE hAacEnc = NULL;
  float ScaleBitrate = 1.0f;

  if (hConfig == NULL) {
    error = 1;
  }

  if (!error) {
    if (*phAacEnc == NULL) {
      error = IISAACFENC_AacEncNew(phAacEnc);
    }
  }

  if (!error) {
    hAacEnc = *phAacEnc;
  }

  if (!error) {
    int nChannels = hConfig->channelMapping->nChannels;
    int nEffectiveChannels = hConfig->channelMapping->nEffectiveChannels;

    if (nChannels < 1) {
      error = 1;
    }

    if (nChannels > SIGMAP_MAX_SIGNALS) {
      error = 1;
    }

    if (hConfig->bitRate != 0 && hConfig->bitRate / nEffectiveChannels < 3500) {
      error = 1;
    }

    if ((hConfig->nGranuleLength != AACENC_GRANULE_768) &&
        (hConfig->nGranuleLength != AACENC_GRANULE_1024)) {
      error = -1;
    }

    if (hConfig->bitRate != 0 && hConfig->bitRate > ((float)MIN_BUFSIZE_PER_EFF_CHAN) / ((float)hConfig->nGranuleLength) * hConfig->sampleRate * nEffectiveChannels) {
      error = 1;
    }
  }

  if (!error) {
    if (hConfig->bitRate < 0) {
      error = 1;
    }
  }
  if (!error) {
    if (hConfig->bitRate - hConfig->ancDataBitRate < 0) {
      error = 1;
    }
  }

  if (!error) {
    hAacEnc->hConfig = hConfig;
    hAacEnc->bitrateMode = (AACENC_BITRATE_MODE)hConfig->bitrateMode;
    hAacEnc->nGranuleLength = hConfig->nGranuleLength;

    memcpy(hAacEnc->channelMapping, hConfig->channelMapping, sizeof(CHANNEL_MAPPING));
  }
  if (!error) {
    cm = hAacEnc->channelMapping;
  }
  if (!error) {
    int aacCoreBitRate = hConfig->bitRate;
    if (aacCoreBitRate > 0) {
      aacCoreBitRate -= hConfig->ancDataBitRate;
    }

    if (aacCoreBitRate < 0) {
      error = 1;
    }

    if (!error) {
      if (cm->cicpLayoutIndex == SIGMAP_CUSTOMIZED_3_OVER_5_1) {
        ScaleBitrate = 5.0f / 3.0f;
      }
      if (cm->cicpLayoutIndex == SIGMAP_CUSTOMIZED_4_OVER_5_1) {
        ScaleBitrate = 5.0f / 4.0f;
      }
      error = iisaacfenc_DetermineBandWidth(&hAacEnc->hConfig->bandWidth,
                                            (hConfig->bitrateMode > AACENC_BR_MODE_VBR_2) ? 0 : (int)hConfig->bandWidth,
                                            (int)(aacCoreBitRate * ScaleBitrate),
                                            (BANDWIDTH_BITRATE_MODE)hConfig->bitrateMode,
                                            hConfig->sampleRate,
                                            hAacEnc->channelMapping);
    }
  }

  if (!error) {
    if (hAacEnc->hBitstream == 0 || GetBitBufSize(hAacEnc->hBitstream) < 64000 * cm->nChannels) {
      if (hAacEnc->hBitstream != 0) {
        DeleteBitBuffer(hAacEnc->hBitstream);
      }
      hAacEnc->hBitstream = CreateBitBuffer(64000 * cm->nChannels);
      if (hAacEnc->hBitstream == 0) {
        error = 1;
      }
    }
  }

  if (!error) {
    DoCRCCheck(hAacEnc->hBitstream, hConfig->calcCrc);
  }

  if (!error) {
    if (iisaacfenc_PsyNew(&hAacEnc->psyKernel,
                          cm->nChannels,
                          hAacEnc->nGranuleLength) != 0) {
      error = 1;
    }
  }
  if (!error) {
    error = iisaacfenc_PsyOutNew(&hAacEnc->psyOut, cm->nChannels, cm->nElements);
  }
  if (!error) {
    int psyBitrate = (int)(hConfig->bitRate * ScaleBitrate);

    assert(psyBitrate > 0);

    psyBitrate -= hConfig->ancDataBitRate;

    if (psyBitrate <= 0) {
      psyBitrate = 0;
      error = 1;
    }

    hAacEnc->bandwidth = (int)hAacEnc->hConfig->bandWidth;

    if (hAacEnc->bandwidth > hConfig->sampleRate / 2) {
      hAacEnc->bandwidth = hConfig->sampleRate / 2;
    }

    if (!error) {
      error = iisaacfenc_psyMainInit(hAacEnc->hConfig->codecType,
                                     hAacEnc->psyKernel,
                                     hAacEnc->useCpuOptimisation,
                                     hConfig->sampleRate,
                                     psyBitrate,
                                     hConfig->useTns,
                                     (float)hAacEnc->bandwidth,
                                     hConfig->useIS,
                                     hConfig->channelMapping);
    }
  }

  if (!error) {
    int ch;

    iisaacfenc_PsyOutInit(cm->nElements,
                          cm->nChannels,
                          hAacEnc->useCpuOptimisation,
                          hAacEnc->psyKernel,
                          hAacEnc->psyOut);

    for (ch = 0; ch < cm->nChannels; ch++) {
      hAacEnc->psyOut->psyOutChannel[ch]->codecType = hAacEnc->hConfig->codecType;
    }
  }

  if (!error) {
    error = iisaacfenc_QCOutNew(&hAacEnc->qcOut,
                                cm->nChannels,
                                cm->nElements,
                                hAacEnc->nGranuleLength);
  }

  if (!error) {
    error = iisaacfenc_QCNew(&hAacEnc->qcKernel,
                             cm->nElements);
  }

  if (!error) {
    struct QC_INIT qcInit;
    int aacCoreBitRate = hConfig->bitRate;
    int nChannelsEff = hAacEnc->channelMapping->nEffectiveChannels;

    memset(&qcInit, 0, sizeof(struct QC_INIT));

    aacCoreBitRate -= hConfig->ancDataBitRate;

    if (aacCoreBitRate < 0) {
      error = 1;
    }

    qcInit.channelMapping = hAacEnc->channelMapping;
    qcInit.maxBits = MIN_BUFSIZE_PER_EFF_CHAN * nChannelsEff;
    qcInit.bitRes = qcInit.maxBits;

    if ((signed int)(INT_MAX / hAacEnc->nGranuleLength) > hConfig->bitRate) {
      qcInit.averageBits = (hConfig->bitRate * hAacEnc->nGranuleLength) / hConfig->sampleRate;
    } else {
      qcInit.averageBits = (int)((float)hConfig->bitRate * (float)hAacEnc->nGranuleLength / (float)hConfig->sampleRate);
    }

    qcInit.transMux = hConfig->transMux;

    qcInit.meanPe = 9.0f * hAacEnc->nGranuleLength * hAacEnc->bandwidth / (hConfig->sampleRate / 2.0f);
    switch (hConfig->bitrateMode) {
      case AACENC_BR_MODE_CBR:
        qcInit.bitrateMode = QCDATA_BR_MODE_CBR;
        break;
      case AACENC_BR_MODE_VBR_0:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_0;
        break;
      case AACENC_BR_MODE_VBR_1:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_1;
        break;
      case AACENC_BR_MODE_VBR_2:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_2;
        break;
      case AACENC_BR_MODE_VBR_3:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_3;
        break;
      case AACENC_BR_MODE_VBR_4:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_4;
        break;
      case AACENC_BR_MODE_VBR_5:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_5;
        break;
      case AACENC_BR_MODE_VBR_6:
        qcInit.bitrateMode = QCDATA_BR_MODE_VBR_6;
        break;
      default:
        qcInit.bitrateMode = QCDATA_BR_MODE_INVALID;
        break;
    }
    qcInit.invQuant = hConfig->useRequantization;
    qcInit.maxBitFac = (float)((MIN_BUFSIZE_PER_EFF_CHAN)*nChannelsEff) /
                       (qcInit.averageBits ? qcInit.averageBits : 1);
    qcInit.bitrate = aacCoreBitRate;
    qcInit.useNoiseFilling = hConfig->useNoiseFilling;
    qcInit.codecType = hConfig->codecType;
    qcInit.crcPoly = 0x8005;
    qcInit.crcStartVal = 0xffff;
    qcInit.useNoiseFilling = hConfig->useNoiseFilling;
    qcInit.useLloydMaxQuantizer = hConfig->useLloydMaxQuantizer;

    if (!error) {
      error = iisaacfenc_QCInit(hAacEnc->qcKernel, &qcInit);
    }
  }

  if (!error) {
    int bChannelConfigZero = 0;
    switch (hAacEnc->channelMapping->cicpLayoutIndex) {
      case SIGMAP_CICP_1:
      case SIGMAP_CICP_2:
      case SIGMAP_CICP_3:
      case SIGMAP_CICP_4:
      case SIGMAP_CICP_5:
      case SIGMAP_CICP_6:
      case SIGMAP_CICP_7:
        bChannelConfigZero = 0;
        break;
      case SIGMAP_CICP_11:
      case SIGMAP_CICP_12:
      case SIGMAP_CICP_13:
      case SIGMAP_CICP_14:
        bChannelConfigZero = 0;
        break;
      default:

        bChannelConfigZero = 1;
        break;
    }

    hAacEnc->bWritePce = 0;
    hAacEnc->updateRatePce = 0;

    if ((bChannelConfigZero != 0) &&
        (hConfig->mpeg4Flag != 0)) {
      hAacEnc->bWritePce = 1;

      hAacEnc->updateRatePce = (int)(((float)hAacEnc->hConfig->sampleRate * sendPceTimeIntervalDefault) / ((float)hAacEnc->nGranuleLength));
      hAacEnc->updateRatePce = max(1, hAacEnc->updateRatePce);
    }

    hAacEnc->frameCntPce = 0;
  }

  if (!error) {
    hAacEnc->hTfDec = NULL;

    if (!error) {
      if (hAacEnc->hConfig->codecType == AACENC_CODEC_XHEAAC || hAacEnc->hConfig->codecType == AACENC_CODEC_MPEGH) {
        error = iisaacfenc_CreateIntDec(&hAacEnc->hTfDec, cm->nChannels, hConfig->sampleRate,
                                        hAacEnc->nGranuleLength);
      }
    }
  }

  if (!error) {
    BS_CHANNEL_MAPPING* bs_cm = NULL;
    HANDLE_ERROR_INFO errorInfo = noError;

    if (errorInfo == noError) {
      errorInfo = iisaacfenc_allocBsChannelMapping(&bs_cm, cm);
      if (errorInfo != noError) {
        error = AACENC_INIT_ERROR;
      }
    }

    if (errorInfo == noError) {
      InitDefaultBitstreamConfiguration(&hAacEnc->bs_configuration);
    }

    if (errorInfo == noError) {
      hAacEnc->bs_configuration.bitResInitFillLevel = hAacEnc->hConfig->bitResInitFillLevel;
      if (hAacEnc->hConfig->bitResDistribution > 0.0f && hAacEnc->hConfig->bitResDistribution <= 1.0f) {
        hAacEnc->bs_configuration.bitResDistribution = hAacEnc->hConfig->bitResDistribution;
      }
      hAacEnc->bs_configuration.useNoiseFilling = hAacEnc->hConfig->useNoiseFilling;
    }

    if (errorInfo == noError) {
      AUDIO_OBJECT_TYPE bsAot = AOT_AAC_LC;
      switch (hAacEnc->hConfig->codecType) {
        case AACENC_CODEC_AAC:
          bsAot = AOT_AAC_LC;
          break;
        case AACENC_CODEC_XHEAAC:
          bsAot = AOT_USAC;
          break;
        default:
          error = AACENC_INIT_ERROR;
          break;
      }
      if (hAacEnc->hBsEnc != NULL) {
        DeleteBitstreamEncoder(hAacEnc->hBsEnc);
      }
      errorInfo = CreateBitstreamEncoder(&(hAacEnc->hBsEnc),
                                         hAacEnc->bs_configuration,
                                         hConfig->sampleRate,
                                         bs_cm,
                                         MPEG2_AAC,
                                         bsAot);
      if (errorInfo != noError) {
        error = AACENC_INIT_ERROR;
      }
    }

    iisaacfenc_deleteBsChannelMapping(bs_cm);

    freeErrorTraceback(errorInfo);
  }

  if (!error) {
    IISBITFRAME_ERROR errorInfo = IISBITFRAME_NO_ERROR;
    int initBitrate = hConfig->bitRate;
    int initTotalBitrate = hConfig->bitRate + hConfig->bitReservoirPenalty;
    int nEffectiveChannels = hAacEnc->channelMapping->nEffectiveChannels;
    int bitReservoirMax = 0, bitReservoir = 0;
    float bitReservoirLevel = 1.0f;

    if (hAacEnc->hBitFrame != NULL) {
      IISAACFENC_AacEncGetBitReservoirInfo(hAacEnc, &bitReservoirMax, &bitReservoir, &bitReservoirLevel);
    }

    if (hAacEnc->bs_configuration.loopBitrate != -1) {
      initBitrate = hAacEnc->bs_configuration.loopBitrate;
      initTotalBitrate = hAacEnc->bs_configuration.loopBitrate;
    }

    errorInfo = IISBITFRAME_CreateNewBitFrame(&(hAacEnc->hBitFrame));

    if (errorInfo == IISBITFRAME_NO_ERROR && !isVbr(hAacEnc->bitrateMode)) {
      errorInfo = IISBITFRAME_InitBitFrame(hAacEnc->hBitFrame,
                                           initBitrate,
                                           initTotalBitrate,
                                           nEffectiveChannels,
                                           nEffectiveChannels,
                                           hConfig->sampleRate,
                                           hAacEnc->nGranuleLength,
                                           hConfig->bitRateFractRemainder,
                                           hConfig->bitRateFractTimeBase,
                                           BITRES_FRAMES,
                                           bitReservoirLevel,
                                           hAacEnc->bs_configuration.bitResDistribution,
                                           MPEG2_AAC);
    }

    if (errorInfo != IISBITFRAME_NO_ERROR) {
      error = AACENC_INIT_ERROR;
      IISBITFRAME_DeleteBitFrame(hAacEnc->hBitFrame);
      hAacEnc->hBitFrame = NULL;
    }
  }

  if (!error && !isVbr(hAacEnc->bitrateMode)) {
    IISBITFRAME_ERROR errorInfo = IISBITFRAME_NO_ERROR;

    int extendedBits = 0;
    int extendedBitReservoirFillRate = 0;

    errorInfo = IISBITFRAME_InitExtendedBitFrame(
        hAacEnc->hBitFrame,
        extendedBits,
        extendedBitReservoirFillRate);

    if (errorInfo != IISBITFRAME_NO_ERROR) {
      error = AACENC_INIT_ERROR;
      IISBITFRAME_DeleteBitFrame(hAacEnc->hBitFrame);
      hAacEnc->hBitFrame = NULL;
    }
  }

  if (!isError(error) && (hAacEnc->hConfig->lpdCodingMode != LPD_WRAPPER_CODING_MODE_INVALID)) {
    LPD_WRAPPER_SETUP lpdWrapperSetup;
    int el = 0;

    lpdWrapperSetup.nChannels = hAacEnc->hConfig->channelMapping->nChannels;
    lpdWrapperSetup.nElements = hAacEnc->hConfig->channelMapping->nElements;
    for (el = 0; el < lpdWrapperSetup.nElements; el++) {
      lpdWrapperSetup.phElInfo[el] = &hAacEnc->hConfig->channelMapping->elInfo[el];
    }
    switch (hAacEnc->hConfig->codecType) {
      case AACENC_CODEC_AAC:
        lpdWrapperSetup.codecType = LPD_WRAPPER_CODEC_AAC;
        break;
      case AACENC_CODEC_XHEAAC:
        lpdWrapperSetup.codecType = LPD_WRAPPER_CODEC_XHEAAC;
        break;
      default:
        error = AACENC_INIT_ERROR;
        break;
    }
    if (!error) {
      lpdWrapperSetup.sampleRate = hAacEnc->hConfig->sampleRate;
      lpdWrapperSetup.acelpModeIndex = hAacEnc->hConfig->acelpModeIndex;
      lpdWrapperSetup.totalBitRate = hAacEnc->hConfig->bitRate;
      lpdWrapperSetup.isVbr = isVbr(hAacEnc->hConfig->bitrateMode);
      lpdWrapperSetup.nGranuleLength = hAacEnc->hConfig->nGranuleLength;
      lpdWrapperSetup.codingMode = hAacEnc->hConfig->lpdCodingMode;
      lpdWrapperSetup.optimizedSpeedPulseSearch = hAacEnc->hConfig->optimizedSpeedPulseSearch;
      lpdWrapperSetup.useNoiseFilling = hAacEnc->hConfig->useNoiseFilling;

      error = iisaacfenc_wrap_lpd_open(&hAacEnc->hLpdWrapper,
                                       &lpdWrapperSetup);
    }
  }

  if (!error) {
    int nEffectiveChannels = hAacEnc->channelMapping->nEffectiveChannels;
    hAacEnc->nAddEncDelay = 3 * (hAacEnc->nGranuleLength / TRANS_FAC) +
                            (hAacEnc->nGranuleLength / (2 * TRANS_FAC)) +
                            (hAacEnc->nGranuleLength / TRANS_FAC);
    hAacEnc->nStandDelay = hAacEnc->nGranuleLength;
    hAacEnc->nDelay = hAacEnc->nGranuleLength +
                      3 * (hAacEnc->nGranuleLength / TRANS_FAC) +
                      (hAacEnc->nGranuleLength / (2 * TRANS_FAC)) +
                      (hAacEnc->nGranuleLength / TRANS_FAC);

    hAacEnc->cbBufSizeMin = (nEffectiveChannels * MIN_BUFSIZE_PER_EFF_CHAN) / 8;
  }

  if (!error) {
    hAacEnc->minFrameBytes = 0;
    hAacEnc->hBsEnc->frameData.minFrameBytes = 0;
  }

  if (error) {
    IISAACFENC_AacEncClose(hAacEnc);
    *phAacEnc = NULL;
  }

  if (error) {
    err = AACENC_INIT_ERROR;
  }

  if (!isError(err)) {
    err = (AACFASTENC_ERROR)warning;
  }
  return err;
}

int AACENCAPI IISAACFENC_AacEncGetUseNoiseFilling(AACENC_CONFIG_HANDLE hAacConfig) {
  int useNoiseFilling = -1;
  if (hAacConfig != NULL) {
    useNoiseFilling = hAacConfig->useNoiseFilling;
  }
  return useNoiseFilling;
}

int AACENCAPI IISAACFENC_AacEncGetSampleRate(AACENC_CONFIG_HANDLE hAacConfig) {
  int sampleRate = -1;
  if (hAacConfig != NULL) {
    sampleRate = hAacConfig->sampleRate;
  }
  return sampleRate;
}

AACENC_GRANULE_LEN AACENCAPI IISAACFENC_AacEncGetGranuleLength(AACENC_CONFIG_HANDLE hAacConfig) {
  AACENC_GRANULE_LEN granuleLength = AACENC_GRANULE_INVALID;
  if (hAacConfig != NULL) {
    granuleLength = hAacConfig->nGranuleLength;
  }
  return granuleLength;
}

AACFASTENC_ERROR AACENCAPI
iisaacfenc_isLastShortWindow(AACENC_ENCODER_HANDLE const hAacEnc,
                             int* isLastShortWindow) {
  int el, ch;
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  CHANNEL_MAPPING* cm = hAacEnc->channelMapping;

  *isLastShortWindow = 0;
  for (el = 0; el < cm->nElements; el++) {
    for (ch = 0; ch < cm->elInfo[el].nChannelsInEl; ch++) {
      if (cm->elInfo[el].elType == ID_SCE || cm->elInfo[el].elType == ID_CPE) {
        if (hAacEnc->psyKernel->psyData[cm->elInfo[el].ChannelIndex[ch]]->blockSwitchingControl.windowSequence == SHORT_WINDOW &&
            hAacEnc->psyKernel->psyData[cm->elInfo[el].ChannelIndex[ch]]->blockSwitchingControl.nextWindowSequence != SHORT_WINDOW) {
          *isLastShortWindow = 1;
          break;
        }
      }
    }
  }
  return error;
}

static AACFASTENC_ERROR UpdateBitFrame(AACENC_ENCODER_HANDLE const hAacEnc) {
  AACFASTENC_ERROR retValue = AACENC_NO_ERROR;

  if (NULL == hAacEnc) {
    retValue = AACENC_INVALID_POINTER_ERROR;
  }

  if (!isError(retValue)) {
    IISBITFRAME_ERROR retValueBitFrame = IISBITFRAME_NO_ERROR;
    int extraFillBits = hAacEnc->hBsEnc->frameData.extraFillBytes * 8;

    int frameSize = hAacEnc->hBsEnc->frameData.totDynBits + hAacEnc->hBsEnc->frameData.sideInfoBits +
                    hAacEnc->hBsEnc->frameData.totHeaderBits + hAacEnc->hBsEnc->frameData.additionalElemBits +
                    hAacEnc->hBsEnc->frameData.dseBits;
    int fillBits = max(hAacEnc->hBsEnc->frameData.minFrameBytes * 8 - frameSize, 0);
    int unusedBits = (hAacEnc->hBsEnc->frameData.totHeaderBits + hAacEnc->hBsEnc->frameData.availableFrameBits) - (frameSize + fillBits);

    if (!isVbr(hAacEnc->bitrateMode)) {
      retValueBitFrame = IISBITFRAME_UpdateBitFrameAndExtraFillBytes(hAacEnc->hBitFrame, &unusedBits, &extraFillBits);
      hAacEnc->hBsEnc->frameData.extraFillBytes = extraFillBits / 8;
    }

    if (IISBITFRAME_NO_ERROR == retValueBitFrame) {
      retValueBitFrame = IISBITFRAME_CalculateTotalNumFillBits(unusedBits, extraFillBits, fillBits, &hAacEnc->hBsEnc->frameData.totFillBits);
    }

    retValue = mappingBitFrameErrorToAacError(retValueBitFrame);
  }

  hAacEnc->minFrameBytes = 0;
  hAacEnc->hBsEnc->frameData.minFrameBytes = 0;

  return retValue;
}

static int isVbr(int const bitrateMode) {
  int isVbr = 0;

  switch (bitrateMode) {
    case AACENC_BR_MODE_VBR_0:
    case AACENC_BR_MODE_VBR_1:
    case AACENC_BR_MODE_VBR_2:
    case AACENC_BR_MODE_VBR_3:
    case AACENC_BR_MODE_VBR_4:
    case AACENC_BR_MODE_VBR_5:
    case AACENC_BR_MODE_VBR_6:
      isVbr = 1;
      break;
    default:
      isVbr = 0;
  }
  return isVbr;
}

static int getAverageBitsPerVbrFrame(AACENC_ENCODER_HANDLE const hAacEnc) {
  int averageBitsPerFrame = (int)((float)hAacEnc->hConfig->bitRate * hAacEnc->hConfig->nGranuleLength / hAacEnc->hConfig->sampleRate);
  return averageBitsPerFrame;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetBlockSwitchingNoStartStop(AACENC_ENCODER_HANDLE const hAacEnc,
                                              const int noStartStopSequence) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  int i;

  if (hAacEnc == NULL || hAacEnc->psyKernel == NULL) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    for (i = 0; i < SIGMAP_MAX_SIGNALS; i++) {
      iisaacfenc_PsySetBlockSwitchingNoStartStop(hAacEnc->psyKernel->psyData[i], noStartStopSequence);
    }
  }

  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetTns(AACENC_CONFIG* hConfig,
                        const int useTns) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hConfig == NULL) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    if (useTns == 0) {
      hConfig->useTns = 0;
    } else if (useTns == 1) {
      hConfig->useTns = 1;
    } else {
      error = AACENC_UNKNOWN_ERROR;
    }
  }

  return error;
}

AACFASTENC_ERROR
IISAACFENC_AacEncSetSampleRate(AACENC_CONFIG_HANDLE const hConfig,
                               const int sampleRate) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hConfig == NULL) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    hConfig->sampleRate = sampleRate;
  }

  return error;
}

AACFASTENC_ERROR
IISAACFENC_AacEncSetAncDataBitRate(AACENC_CONFIG_HANDLE const hConfig,
                                   const int ancDataBitRate) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hConfig == NULL) return AACENC_INIT_ERROR;

  if (ancDataBitRate > hConfig->bitRate) {
    error = AACENC_UNKNOWN_ERROR;
  }

  if (!isError(error)) {
    hConfig->ancDataBitRate = ancDataBitRate;
  }

  return error;
}

AACFASTENC_ERROR
IISAACFENC_AacEncSetUseIntensityStereo(AACENC_CONFIG_HANDLE const hConfig,
                                       const int useIntensityStereo) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hConfig == NULL) {
    error = AACENC_INIT_ERROR;
  }

  if (!isError(error)) {
    hConfig->useIS = (useIntensityStereo > 0 ? 1 : 0);
  }

  return error;
}

AACFASTENC_ERROR
IISAACFENC_AacEncSetUseStereoPreprocessor(AACENC_CONFIG_HANDLE const hConfig,
                                          const int useStereoPreprocessor) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hConfig == NULL) {
    return AACENC_INIT_ERROR;
  }

  hConfig->useStereoPreprocessor = (useStereoPreprocessor > 0 ? 1 : 0);

  return error;
}

static AACFASTENC_ERROR AACENCAPI aacEncDistributeExtensionPayload(
    HANDLE_EXTPAYLOAD_CONTAINER* hExtensionContainer,
    int* additionalElemBits,
    int numExternalContainersInUse,
    int* nBitsOutOfBand,
    int* externalElemBits,
    int* nBitsMps,
    CHANNEL_MAPPING* cm) {
  int i = 0;
  int externalElemBitsPerChannel = 0;
  int nElements = cm->nElements;
  int el = 0;
  EXTENSION_PAYLOAD_TYPES extType;
  int bitsToRemove = 0;

  if (hExtensionContainer == NULL) {
    return AACENC_UNKNOWN_ERROR;
  }

  for (el = 0; el < nElements; el++) {
    additionalElemBits[el] = 0;
  }

  for (i = 0; i < numExternalContainersInUse; ++i) {
    if (hasExtensionPayloadContainerFeature(hExtensionContainer[i], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
      *nBitsOutOfBand += getTotalSize_extPayload(hExtensionContainer[i]);
    } else {
      *externalElemBits += getTotalSize_extPayload(hExtensionContainer[i]);
    }
    extType = getExtPayloadType(hExtensionContainer[i], 0);
    if (extType == EXT_SBR_DATA ||
        extType == EXT_USACSBR_DATA ||
        extType == EXT_SAOC_DATA ||
        extType == EXT_USACMPS212_DATA ||
        extType == EXT_DYNAMIC_RANGE) {
      if (extType == EXT_USACMPS212_DATA) {
        *nBitsMps = getTotalSize_extPayload(hExtensionContainer[i]);
        *externalElemBits -= getTotalSize_extPayload(hExtensionContainer[i]);
      } else {
        bitsToRemove += getTotalSize_extPayload(hExtensionContainer[i]);
      }
    }
  }

  externalElemBitsPerChannel = *externalElemBits / cm->nEffectiveChannels;
  additionalElemBits[0] += *externalElemBits % cm->nEffectiveChannels;

  for (el = 0; el < nElements; el++) {
    switch (cm->elInfo[el].elType) {
      case ID_SCE:
        additionalElemBits[el] += externalElemBitsPerChannel;
        break;
      case ID_CPE:
        additionalElemBits[el] += 2 * externalElemBitsPerChannel;
        break;
      case ID_LFE:
      case ID_DSE:
        additionalElemBits[el] += 0;
        break;
      default:
        assert(0);
    }
  }
  *externalElemBits -= bitsToRemove;

  return AACENC_NO_ERROR;
}

static AACFASTENC_ERROR AACENCAPI resetExtensionPayload(
    HANDLE_EXTPAYLOAD_CONTAINER* hExtensionContainer,
    int numExternalContainersInUse) {
  int i = 0;

  if (hExtensionContainer == NULL) {
    return AACENC_UNKNOWN_ERROR;
  }

  for (i = 0; i < numExternalContainersInUse; ++i) {
    resetExtensionPayloadContainer(hExtensionContainer[i]);
  }
  return AACENC_NO_ERROR;
}

static AACFASTENC_ERROR calculateHeaderBits(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int const nBitsOutOfBand,
    int* headerBits) {
  AACFASTENC_ERROR err = AACENC_NO_ERROR;
  int bitReservoir = 0;
  int bitsPerFrame = 0;

  if (headerBits == NULL) {
    return AACENC_UNKNOWN_ERROR;
  }

  if (hAacEnc->headerBitsCallbackHandle != NULL) {
    bitReservoir = IISBITFRAME_GetBitreservoir(hAacEnc->hBitFrame);

    if (isVbr(hAacEnc->bitrateMode)) {
      bitsPerFrame = MIN_BUFSIZE_PER_EFF_CHAN * hAacEnc->hConfig->channelMapping->nEffectiveChannels;
    } else {
      bitsPerFrame = IISBITFRAME_GetNumberOfBitsForCurrentFrame(hAacEnc->hBitFrame, 1);
    }

    if (hAacEnc->headerBitsCallbackFunc(hAacEnc->headerBitsCallbackHandle, bitsPerFrame + nBitsOutOfBand, bitReservoir, 2, headerBits)) {
      err = AACENC_UNKNOWN_ERROR;
    }
  }

  return err;
}

static AACFASTENC_ERROR computeHeaderLengthFillBytes(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int headerBits,
    int nBitsOutOfBand,
    int nBitsPce) {
  AACFASTENC_ERROR err = AACENC_NO_ERROR;
  int avlDynBits = 0;
  int avlFrmBits = 0;
  int bitResCur = 0;
  int bitResMax = 0;
  int exFillBits = 0;
  int totFrmBits = 0;
  int headerFix = 0;
  int bitResTmp = 0;
  int headerDiff = 0;
  int actualHeaderBits = 0;
  int exBitResGranted = 0;
  int spaceInExBitRes = 0;
  int exBitResFillRate = 0;
  int loopCnt = 0;

  avlDynBits = hAacEnc->hBsEnc->frameData.availableDynpartBits;
  avlFrmBits = hAacEnc->hBsEnc->frameData.availableFrameBits;
  bitResCur = IISBITFRAME_GetBitreservoir(hAacEnc->hBitFrame);
  bitResMax = IISBITFRAME_GetBitreservoirMax(hAacEnc->hBitFrame);
  exBitResGranted = IISBITFRAME_GetExtendedBitreservoirGrantedBits(hAacEnc->hBitFrame);
  exFillBits = hAacEnc->hBsEnc->frameData.extraFillBytes << 3;
  totFrmBits = hAacEnc->hBsEnc->frameData.totalFrameBits;
  spaceInExBitRes = IISBITFRAME_GetExtendedBitreservoirMax(hAacEnc->hBitFrame) - IISBITFRAME_GetExtendedBitreservoir(hAacEnc->hBitFrame);
  exBitResFillRate = IISBITFRAME_GetExtendedBitreservoirFillRate(hAacEnc->hBitFrame);
  spaceInExBitRes -= exBitResFillRate;
  spaceInExBitRes = max(0, spaceInExBitRes);

  bitResMax += exBitResGranted;
  bitResMax += spaceInExBitRes;

  int frameSize = hAacEnc->hBsEnc->frameData.totDynBits + hAacEnc->hBsEnc->frameData.sideInfoBits +
                  headerBits + hAacEnc->hBsEnc->frameData.additionalElemBits + hAacEnc->hBsEnc->frameData.dseBits;
  int fillBits = max(hAacEnc->hBsEnc->frameData.minFrameBytes * 8 - frameSize, 0);
  int unusedBits = hAacEnc->hBsEnc->frameData.totalFrameBits - (frameSize + fillBits);

  bitResTmp = bitResCur + unusedBits - (nBitsOutOfBand % 8);
  bitResTmp = min(bitResTmp, bitResMax);
  bitResTmp -= exFillBits;
  bitResTmp -= bitResTmp % 8;

  if (hAacEnc->headerBitsCallbackHandle != NULL) {
    if (hAacEnc->headerBitsCallbackFunc(hAacEnc->headerBitsCallbackHandle, 0, 0, 0, &headerFix)) {
      err = AACENC_UNKNOWN_ERROR;
    }

    if (hAacEnc->headerBitsCallbackFunc(hAacEnc->headerBitsCallbackHandle, totFrmBits - headerBits + nBitsPce + (nBitsOutOfBand / 8) * 8, bitResCur - bitResTmp, 1, &actualHeaderBits)) {
      err = AACENC_UNKNOWN_ERROR;
    }

    headerDiff = headerBits - headerFix - actualHeaderBits;
  }

  while (headerDiff != 0) {
    avlDynBits += headerDiff;
    avlFrmBits += headerDiff;
    headerBits -= headerDiff;

    frameSize -= headerDiff;
    fillBits = max(hAacEnc->hBsEnc->frameData.minFrameBytes * 8 - frameSize, 0);
    unusedBits = hAacEnc->hBsEnc->frameData.totalFrameBits - (frameSize + fillBits);

    bitResTmp = bitResCur + unusedBits - (nBitsOutOfBand % 8);
    bitResTmp = min(bitResTmp, bitResMax);
    bitResTmp -= exFillBits;
    bitResTmp -= bitResTmp % 8;

    if (hAacEnc->headerBitsCallbackHandle != NULL) {
      if (hAacEnc->headerBitsCallbackFunc(hAacEnc->headerBitsCallbackHandle, totFrmBits - headerBits + nBitsPce + (nBitsOutOfBand / 8) * 8, bitResCur - bitResTmp, 1, &actualHeaderBits)) {
        err = AACENC_UNKNOWN_ERROR;
      }
    }
    headerDiff = headerBits - headerFix - actualHeaderBits;

    if ((headerDiff != 0) && (loopCnt > 0)) {
      exFillBits += 8;
      loopCnt = 0;
    } else {
      loopCnt++;
    }
  }

  hAacEnc->hBsEnc->frameData.availableDynpartBits = avlDynBits;
  hAacEnc->hBsEnc->frameData.availableFrameBits = avlFrmBits;
  hAacEnc->hBsEnc->frameData.extraFillBytes = exFillBits >> 3;
  hAacEnc->hBsEnc->frameData.totHeaderBits = headerBits;
  if (hAacEnc->headerBitsCallbackHandle != NULL) {
    hAacEnc->hBsEnc->frameData.totHeaderBits += nBitsPce;
  }
  return err;
}

static AACFASTENC_ERROR mappingBitFrameErrorToAacError(
    IISBITFRAME_ERROR bitFrameError) {
  AACFASTENC_ERROR retVal = AACENC_NO_ERROR;

  switch (bitFrameError) {
    case IISBITFRAME_NO_ERROR:
      retVal = AACENC_NO_ERROR;
      break;
    case IISBITFRAME_INVALID_HANDLE:
      retVal = AACENC_INVALID_POINTER_ERROR;
      break;
    case IISBITFRAME_INVALID_PARAM:
      retVal = AACENC_ILLEGAL_PARAMETER_ERROR;
      break;
    case IISBITFRAME_BUFSIZE_ERROR:
      retVal = AACENC_BUFSIZE_ERROR;
      break;
    default:
      retVal = AACENC_UNKNOWN_ERROR;
      break;
  }

  return retVal;
}

static AACFASTENC_ERROR mappingLpdErrorToAacError(
    LPD_WRAPPER_ERROR lpdError) {
  AACFASTENC_ERROR retVal = AACENC_NO_ERROR;

  switch (lpdError) {
    case LPD_WRAPPER_NO_ERROR:
      retVal = AACENC_NO_ERROR;
      break;
    case LPD_WRAPPER_INIT_ERROR:
      retVal = AACENC_INIT_ERROR;
      break;
    case LPD_WRAPPER_ILLEGAL_PARAMETER:
      retVal = AACENC_ILLEGAL_PARAMETER_ERROR;
      break;
    case LPD_WRAPPER_INVALID_POINTER:
      retVal = AACENC_INVALID_POINTER_ERROR;
      break;
    case LPD_WRAPPER_UNKNOWN_ERROR:
    default:
      retVal = AACENC_UNKNOWN_ERROR;
      break;
  }

  return retVal;
}

static AACFASTENC_ERROR mappingIpfStateFromAacToLpdWrapper(
    AACENC_IPF_STATE ipfStateAacenc,
    LPD_WRAPPER_IPF_STATE* pMappedIpfStateLpdWrapper) {
  AACFASTENC_ERROR err = AACENC_NO_ERROR;
  if (!isError(err)) {
    if (pMappedIpfStateLpdWrapper == NULL) {
      err = AACENC_INVALID_POINTER_ERROR;
    }
  }
  if (!isError(err)) {
    switch (ipfStateAacenc) {
      case AACENC_IPF_STATE_NO:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_NO;
        break;
      case AACENC_IPF_STATE_RAP_FIRST_PREROLL:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_RAP_FIRST_PREROLL;
        break;
      case AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL;
        break;
      case AACENC_IPF_STATE_RAP_NEXT_PREROLL:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_RAP_NEXT_PREROLL;
        break;
      case AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL;
        break;
      case AACENC_IPF_STATE_RAP_IPF:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_RAP_IPF;
        break;
      case AACENC_IPF_STATE_RAP_IPF_PREROLL:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_RAP_IPF_PREROLL;
        break;
      case AACENC_IPF_STATE_CONFIGCHANGE_IPF:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_IPF;
        break;
      default:
        *pMappedIpfStateLpdWrapper = LPD_WRAPPER_IPF_STATE_NO;
        err = AACENC_ILLEGAL_PARAMETER_ERROR;
        break;
    }
  }

  return err;
}

AACFASTENC_ERROR IISAACFENC_ExtendedBitReservoirUpdateParameters(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const int nBits,
    const int intervalSamples,
    const float preRollAUFactor) {
  int nPreRollFrameBits = 0;
  int extBitResSize = 0;
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (hAacEnc == NULL) {
    error = AACENC_INIT_ERROR;
    return error;
  }

  if (hAacEnc->hConfig->useExtendedBitReservoir != 0) {
    if (preRollAUFactor > 0.0f) {
      nPreRollFrameBits = IISBITFRAME_GetAverageBitsPerFrame(hAacEnc->hBitFrame);
      nPreRollFrameBits = (int)(nPreRollFrameBits * preRollAUFactor);
      extBitResSize = nBits + nPreRollFrameBits;
    } else {
      extBitResSize = nBits;
    }
  }

  if (!isError(error)) {
    error = updateBitreservoirAndExtendedBitreservoir(hAacEnc, extBitResSize, intervalSamples);
  }

  return error;
}

static AACFASTENC_ERROR updateBitreservoirAndExtendedBitreservoir(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const int extBitResSize,
    const int intervalSamples) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;
  IISBITFRAME_ERROR errorInfo = IISBITFRAME_NO_ERROR;

  errorInfo = IISBITFRAME_ExtendedBitReservoirUpdateParameters(hAacEnc->hBitFrame, extBitResSize);

  if (errorInfo != IISBITFRAME_NO_ERROR) {
    error = AACENC_UNKNOWN_ERROR;
  }

  if (!isError(error)) {
    error = extendedBitReservoirUpdateFillRate(hAacEnc, intervalSamples);
  }

  return error;
}

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncClone(
    AACENC_ENCODER_HANDLE hDest,
    const AACENC_ENCODER_HANDLE hSrc) {
  AACFASTENC_ERROR status = AACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  float* pTimeSignal = NULL;
  CHANNEL_MAPPING* cm = NULL;
  int el, ch;
  int bitResLevel;
  int chIdx[2] = {0};
  float nullVector[1024];

  if (hDest == NULL) return AACENC_UNKNOWN_ERROR;
  if (hSrc == NULL) return AACENC_UNKNOWN_ERROR;

  cm = hDest->channelMapping;
  setFLOAT(0, nullVector, 1024);

  for (el = 0; el < cm->nElements; el++) {
    ELEMENT_INFO elInfo = cm->elInfo[el];
    chIdx[0] = elInfo.ChannelIndex[0];
    chIdx[1] = elInfo.ChannelIndex[1];

    for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
      PSY_DATA* psyDataSrc = hSrc->psyKernel->psyData[chIdx[ch]];
      PSY_DATA* psyDataDest = hDest->psyKernel->psyData[chIdx[ch]];
      HANDLE_MP4TIMEBUF hInputBufferSrc = psyDataSrc->psyInputBuffer;
      HANDLE_MP4TIMEBUF hInputBufferDest = psyDataDest->psyInputBuffer;

      pTimeSignal = MP4TIMEBUF_AccessBuffer(hInputBufferSrc, TRANSFORM_OFFSET_LONG, 0);

      errorInfo = MP4TIMEBUF_InvalidateBuffer(hInputBufferDest, hDest->nDelay);
      if (errorInfo != noError) {
        status = AACENC_UNKNOWN_ERROR;
        break;
      }

      errorInfo = MP4TIMEBUF_FeedBufferMono(hInputBufferDest, pTimeSignal, hDest->nDelay);
      if (errorInfo != noError) {
        status = AACENC_UNKNOWN_ERROR;
        break;
      }

      psyDataDest->blockSwitchingControl = psyDataSrc->blockSwitchingControl;

      bitResLevel = IISBITFRAME_GetBitreservoir(hSrc->hBitFrame);
      IISBITFRAME_UpdateBitReservoir(hDest->hBitFrame, bitResLevel);
    }
  }

  return status;
}

AACFASTENC_ERROR IISAACFENC_AacEncGetThresholdsForExternalBits(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const absoluteMaxNumExternalBits,
    int* const comfortableMaxNumExternalBits,
    int const bUsacIndepFlag,
    AACENC_IPF_STATE const ipfState) {
  AACFASTENC_ERROR error = AACENC_NO_ERROR;

  if (!hAacEnc || !hAacEnc->hBitFrame || !absoluteMaxNumExternalBits || !comfortableMaxNumExternalBits) {
    error = AACENC_INVALID_POINTER_ERROR;
  } else if (isVbr(hAacEnc->bitrateMode)) {
    assert(0);
    error = AACENC_UNKNOWN_ERROR;
  }

  if (!isError(error)) {
    int avgBitsPerFrame = IISBITFRAME_GetAverageBitsPerFrame(hAacEnc->hBitFrame);
    int bitReservoir = IISBITFRAME_GetBitreservoir(hAacEnc->hBitFrame);
    int extBitReservoir = IISBITFRAME_GetExtendedBitreservoir(hAacEnc->hBitFrame);

    int maxAvailableBits = avgBitsPerFrame + bitReservoir;

    if (ipfState == AACENC_IPF_STATE_RAP_IPF || ipfState == AACENC_IPF_STATE_CONFIGCHANGE_IPF || ipfState == AACENC_IPF_STATE_RAP_IPF_PREROLL) {
      maxAvailableBits += extBitReservoir;
    }

    *absoluteMaxNumExternalBits = maxAvailableBits - (int)(0.9 * avgBitsPerFrame);

    (void)bUsacIndepFlag;

    *comfortableMaxNumExternalBits = min(max(maxAvailableBits - (int)(1.5 * avgBitsPerFrame), 0), MAX_EXTENSION_PAYLOAD_SIZE * 8);
  }

  return error;
}

static int getLPDHeadroom(
    const int totalBitrate,
    const int maxChannels,
    const int sampleRate,
    const int granuleLength) {
  int headroomBits = 0;
  int granuleBits = 0;

  granuleBits = totalBitrate * granuleLength / sampleRate;
  headroomBits = max(maxChannels * MIN_BUFSIZE_PER_EFF_CHAN - granuleBits, 0);
  headroomBits = (int)((float)headroomBits * SIMULATED_VBR_BITRES_FILL_LEVEL);

  return headroomBits;
}

static int isError(
    AACFASTENC_ERROR const statusCode) {
  return (statusCode != AACENC_NO_ERROR);
}
