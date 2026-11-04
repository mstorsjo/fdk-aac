
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include "aacenc.h"
#include "iisxHEAACEncLib_aac_ifc.h"
#include "iisxHEAACEncLib_common.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

struct AacEncoder {
  AACENC_CONFIG_HANDLE hAacConfig;
  AACENC_ENCODER_HANDLE hCoreCoder;
};

static int headerBitsCallbackLOAS(void *handle, int bitsPerFrame, int bitReservoir, int mode, int *headerBits) {
  HANDLE_STREAM_FORMAT hLatmLoas = (HANDLE_STREAM_FORMAT)handle;

  switch (mode) {
    case 0:
      *headerBits = IIS_LoasWriter_CountBitDemandHeader(hLatmLoas, 0, 0);
      break;
    case 1:
      *headerBits = IIS_LoasWriter_CountBitDemandHeader(hLatmLoas, bitsPerFrame + bitReservoir, 1);
      break;
    case 2:
      *headerBits = IIS_LoasWriter_CountBitDemandHeader(hLatmLoas, min(bitsPerFrame * 2, bitsPerFrame + bitReservoir), 2);
      break;
  }
  return (0);
}

static XHEAACENCLIB_RETURN mappingIpfStateFromXheAacInterfaceToAac(
    XHEAAC_AACENC_IPF_STATE ipfStatXheAacenc,
    AACENC_IPF_STATE *pMappedIpfStateAacenc);

static AACENC_BITRATE_MODE BitrateMode2AacEncBitrateMode(XHEAACENCLIB_BITRATE_MODE bitrateMode) {
  AACENC_BITRATE_MODE aacBitrateMode;

  switch (bitrateMode) {
    case XHEAACENCLIB_BR_MODE_CBR:
      aacBitrateMode = AACENC_BR_MODE_CBR;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_0:
      aacBitrateMode = AACENC_BR_MODE_VBR_0;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_1:
      aacBitrateMode = AACENC_BR_MODE_VBR_1;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_2:
      aacBitrateMode = AACENC_BR_MODE_VBR_2;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_3:
      aacBitrateMode = AACENC_BR_MODE_VBR_3;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_4:
      aacBitrateMode = AACENC_BR_MODE_VBR_4;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_5:
      aacBitrateMode = AACENC_BR_MODE_VBR_5;
      break;
    case XHEAACENCLIB_BR_MODE_VBR_6:
      aacBitrateMode = AACENC_BR_MODE_VBR_6;
      break;
    case XHEAACENCLIB_BR_MODE_INVALID:
    default:
      assert(0);
      aacBitrateMode = AACENC_BR_MODE_INVALID;
      break;
  }

  return aacBitrateMode;
}

static AACENC_SETUP_CODEC_TYPE CodecType2AacEncCodecType(XHEAACENCLIB_CODEC_TYPE codecType) {
  AACENC_SETUP_CODEC_TYPE aacCodecType = AACENC_SETUP_CODEC_UNKNOWN;

  switch (codecType) {
    case XHEAACENCLIB_CODEC_AAC:
      aacCodecType = AACENC_SETUP_CODEC_AAC;
      break;
    case XHEAACENCLIB_CODEC_USAC:
      aacCodecType = AACENC_SETUP_CODEC_XHEAAC;
      break;
    default:
      assert(0);
  }

  return aacCodecType;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncConfigure(
    XHEAACENCLIB_HANDLE_AACENCODER *p_hAacEnc,
    XHEAACENCLIB_CONFIG_HANDLE hConfig) {
  XHEAACENCLIB_AACENCODER *aacEnc = NULL;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACENC_SETUP setup = {0};

  if (hConfig == NULL || p_hAacEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    aacEnc = (XHEAACENCLIB_HANDLE_AACENCODER)iisCalloc(1, sizeof(XHEAACENCLIB_AACENCODER));
    if (aacEnc == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue)) {
    memset(&setup, 0, sizeof(AACENC_SETUP));
    setup.bResidualCoding = iisxHEAACEncLibAacEncGetResidualCoding(hConfig);
    setup.bitRate = (hConfig->bitRate - hConfig->bitRatePenalty);
    setup.bitReservoirPenalty = hConfig->bitRatePenalty;
    if (hConfig->bitRatePenalty > 0) {
      setup.bitResDistribution = 1.0f;
    } else {
      setup.bitResDistribution = -1.0f;
    }
    setup.nGranuleLength = (AACENC_GRANULE_LEN)hConfig->granuleLength;
    setup.sampleRate = hConfig->sampleRateAAC;
    setup.bandWidth = 0;
    setup.bitDistributionMode = AACENC_BD_MODE_INTER_ELEMENT;
    setup.sampleRate = hConfig->sampleRateAAC;
    setup.ancDataBitRate = hConfig->nAncDataBitRate + hConfig->nMetadataBitRate + hConfig->bitRateCodingTools;
    setup.channelMapping = hConfig->cm;
    setup.useNoiseFilling = hConfig->useNoiseFilling;
    setup.useTns = hConfig->bUseTNS;

    setup.acelpModeIndex = hConfig->acelpModeIndex;
    setup.optimizedSpeedPulseSearch = hConfig->optimizedSpeedPulseSearch;
    switch (hConfig->coreMode) {
      case XHEAACENCLIB_CODING_MODE_FD:
        setup.codingMode = AACENC_SETUP_CODING_MODE_FD;
        break;
      case XHEAACENCLIB_CODING_MODE_LPD:
        setup.codingMode = AACENC_SETUP_CODING_MODE_LPD;
        break;
      case XHEAACENCLIB_CODING_MODE_SWITCHED:
        setup.codingMode = AACENC_SETUP_CODING_MODE_SWITCHED;
        break;
    }

    setup.bitRateFractRemainder = hConfig->bitRateFractRemainder;
    setup.bitRateFractTimeBase = hConfig->bitRateFractTimeBase;
    setup.useExtendedBitReservoir = (hConfig->audioPreRollBitResMode == XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL);

    switch (hConfig->bitrateMode) {
      case XHEAACENCLIB_BR_MODE_CBR:
        setup.bitrateMode = AACENC_BR_MODE_CBR;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_0:
        setup.bitrateMode = AACENC_BR_MODE_VBR_0;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_1:
        setup.bitrateMode = AACENC_BR_MODE_VBR_1;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_2:
        setup.bitrateMode = AACENC_BR_MODE_VBR_2;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_3:
        setup.bitrateMode = AACENC_BR_MODE_VBR_3;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_4:
        setup.bitrateMode = AACENC_BR_MODE_VBR_4;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_5:
        setup.bitrateMode = AACENC_BR_MODE_VBR_5;
        break;
      case XHEAACENCLIB_BR_MODE_VBR_6:
        setup.bitrateMode = AACENC_BR_MODE_VBR_6;
        break;
      case XHEAACENCLIB_BR_MODE_INVALID:
      default:
        assert(0);
        setup.bitrateMode = AACENC_BR_MODE_CBR;
        break;
    }

    switch (hConfig->quality) {
      case XHEAACENCLIB_QUAL_FAST:
        setup.quality = QUAL_FAST;
        break;
      case XHEAACENCLIB_QUAL_MEDIUM:
        setup.quality = QUAL_MEDIUM;
        break;
      case XHEAACENCLIB_QUAL_HIGH:
        setup.quality = QUAL_HIGH;
        break;
      case XHEAACENCLIB_QUAL_INVALID:
      default:
        assert(0);
        setup.quality = QUAL_HIGH;
        break;
    }

    switch (hConfig->aot) {
      case AUD_OBJ_TYP_LC:
      case AUD_OBJ_TYP_HEAAC:
      case AUD_OBJ_TYP_PS:
        setup.codecType = AACENC_SETUP_CODEC_AAC;
        break;
      case AUD_OBJ_TYP_USAC:
        setup.codecType = AACENC_SETUP_CODEC_XHEAAC;
        break;
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_AOT_CONFIGURATION;
        break;
    }

    setup.mpeg4 = 1;

    if (hConfig->mpeg2AAC == XHEAACENCLIB_MPEG2AAC_ON) {
      setup.mpeg4 = 0;
    }

    if ((hConfig->aot == AUD_OBJ_TYP_MP2_LC) ||
        (hConfig->aot == AUD_OBJ_TYP_MP2_SBR)) {
      setup.mpeg4 = 0;
    }

    setup.calcCrc = 0;
    if (!isError(retValue)) {
      switch (hConfig->transportFormat) {
        case TT_RAW:
        case TT_LOAS:
        case TT_LOAS_NOSMC:
        case TT_LATM:
        case TT_LATM_NOSMC:
          setup.transMux = AACENC_MUX_RAW;
          break;
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
          break;
      }
    }
  }

  if (!isError(retValue)) {
    if (IISAACFENC_GetConfiguration(setup, &(aacEnc->hAacConfig)) != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
    }
  }

  if (!isError(retValue)) {
    *p_hAacEnc = aacEnc;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncOpen(XHEAACENCLIB_HANDLE_AACENCODER self,
                          int sampleRateIn,
                          float proposedBandwidth,
                          HANDLE_STREAM_FORMAT hLatmLoas) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;
  float defBandWidth = 0;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    IISAACFENC_AacEncSetBandwidth(self->hAacConfig, proposedBandwidth, &defBandWidth);

    if (defBandWidth > (float)sampleRateIn / 2) {
      IISAACFENC_AacEncSetBandwidth(self->hAacConfig, (float)sampleRateIn * 0.5f, &proposedBandwidth);
    }
    if (aacError == AACENC_NO_ERROR) {
      aacError = IISAACFENC_AacEncUpdate(&self->hCoreCoder,
                                         self->hAacConfig);
    }
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AAC_ENC_UPDATE;
    }
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_SetHeaderBitsCallback(self->hCoreCoder, hLatmLoas, headerBitsCallbackLOAS);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AAC_HEADER_BITS;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetNoStartStopSequence(XHEAACENCLIB_HANDLE_AACENCODER self) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (self != NULL) {
    aacError = IISAACFENC_AacEncSetBlockSwitchingNoStartStop(self->hCoreCoder, 1);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }
  } else {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetInfo(XHEAACENCLIB_HANDLE_AACENCODER self,
                             XHEAACENCLIB_HANDLE_AACINFO hAacInfo) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;
  AACENC_INFO aacInfo = {0};

  if ((self == NULL) ||
      (hAacInfo == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_AacEncGetInfo(self->hCoreCoder, &aacInfo);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }
  }

  if (!isError(retValue)) {
    hAacInfo->bandWidth = aacInfo.bandWidth;
    hAacInfo->cbBufSizeMin = aacInfo.cbBufSizeMin;
    hAacInfo->nAncBytesPerFrame = aacInfo.nAncBytesPerFrame;
    hAacInfo->nDelay = aacInfo.nDelay;
    hAacInfo->nAddEncDelay = aacInfo.nAddEncDelay;
    hAacInfo->nStandDelay = aacInfo.nStandDelay;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetBitReservoirInfo(XHEAACENCLIB_HANDLE_AACENCODER self,
                                         int bitReservoirMax,
                                         int *bitReservoir,
                                         float *bitReservoirLevel) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError;
  int coreBitReservoirMax = 0;
  int coreBitReservoir = 0;
  float coreBitReservoirLevel = 0;

  if (self == NULL || bitReservoir == NULL || bitReservoirLevel == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_AacEncGetBitReservoirInfo(self->hCoreCoder,
                                                    &coreBitReservoirMax,
                                                    &coreBitReservoir,
                                                    &coreBitReservoirLevel);

    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR;
    }
  }

  if (bitReservoirMax == 0) {
    if (!isError(retValue)) {
      *bitReservoir = 0;
      *bitReservoirLevel = 0;
    }
  } else {
    if (!isError(retValue)) {
      aacError = IISAACFENC_AacEncGetBitReservoirInfo(self->hCoreCoder,
                                                      &coreBitReservoirMax,
                                                      &coreBitReservoir,
                                                      &coreBitReservoirLevel);
      if (aacError != AACENC_NO_ERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR;
      }
    }
    if (!isError(retValue)) {
      assert(coreBitReservoirMax <= bitReservoirMax);
      *bitReservoir = coreBitReservoir;

      assert(*bitReservoir >= 0 && *bitReservoir <= bitReservoirMax);
      *bitReservoirLevel = (float)(*bitReservoir) / (float)(bitReservoirMax);
    }
  }

  return retValue;
}

int iisxHEAACEncLibAacEncUseNoiseFilling(XHEAACENCLIB_HANDLE_AACENCODER self) {
  int useNoiseFilling = 0;
  if (self != NULL) {
    useNoiseFilling = IISAACFENC_AacEncGetUseNoiseFilling(self->hAacConfig);
  }
  return useNoiseFilling;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSnapToLowSfbBorder(XHEAACENCLIB_HANDLE_AACENCODER self,
                                        const float desiredBandwidth,
                                        const float tol,
                                        float *adjBandwidth) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError;

  aacError = IISAACFENC_AacEncSnapToLowSfbBorder(IISAACFENC_AacEncGetSampleRate(self->hAacConfig),
                                                 IISAACFENC_AacEncGetGranuleLength(self->hAacConfig),
                                                 desiredBandwidth,
                                                 tol,
                                                 adjBandwidth);
  if (aacError != AACENC_NO_ERROR) {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }
  return retValue;
}

void iisxHEAACEncLibAacEncSetBandwidth(XHEAACENCLIB_HANDLE_AACENCODER self,
                                       float bandWidth) {
  float tmp;
  IISAACFENC_AacEncSetBandwidth(self->hAacConfig, bandWidth, &tmp);
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSAPPrepare(XHEAACENCLIB_HANDLE_AACENCODER self,
                                const XHEAACENCLIB_SAP_TYPE syncType) {
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  switch (syncType) {
    case XHEAACENCLIB_SAP_TYPE_NONE:
      break;
    case XHEAACENCLIB_SAP_TYPE_WDWTYPE:
      aacError = IISAACFENC_SAPPrepare(self->hCoreCoder, AACENC_SAP_TYPE_WDWTYPE);
      break;
    case XHEAACENCLIB_SAP_TYPE_WDWTYPE_HIGHBW:
      aacError = IISAACFENC_SAPPrepare(self->hCoreCoder, AACENC_SAP_TYPE_WDWTYPE_HIGHBW);
      break;
    default:
      assert(0);
      break;
  }
  if (aacError) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SAP_SYNC;
  }
  return retValue;
}

int iisxHEAACEncLibAacEncGetVBRBitrate(XHEAACENCLIB_BITRATE_MODE bitrateMode,
                                       CHANNEL_MAPPING_HANDLE hChMap,
                                       XHEAACENCLIB_CODEC_TYPE codecType) {
  AACENC_BITRATE_MODE aacBitrateMode = BitrateMode2AacEncBitrateMode(bitrateMode);
  AACENC_SETUP_CODEC_TYPE aacCodecType = CodecType2AacEncCodecType(codecType);
  return IISAACFENC_GetVBRBitrate(aacBitrateMode, hChMap, aacCodecType);
}

XHEAACENCLIB_RETURN iisxHEAACEncLibAacEncClose(XHEAACENCLIB_HANDLE_AACENCODER self) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (self) {
    if (self->hCoreCoder) {
      IISAACFENC_AacEncClose(self->hCoreCoder);
    } else {
      IISAACFENC_AacConfigClose(&self->hAacConfig);
    }
    iisFree(self);
  }

  return retValue;
}

int iisxHEAACEncLibAacEncGetResidualCoding(XHEAACENCLIB_CONFIG_HANDLE self) {
  int bResidualCoding;

  if (self->aot == AUD_OBJ_TYP_PS) {
    if (self->pResidualConfig && self->pResidualConfig->codec == MP4SPACEENC_RES_CODEC_PCM) {
      bResidualCoding = self->pResidualConfig->mode != 0;
    } else {
      bResidualCoding = 0;
    }
  } else if (self->stereoConfigIndex > 0) {
    bResidualCoding = self->pResidualConfig->mode != 0;
  } else {
    bResidualCoding = 0;
  }
  return bResidualCoding;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetMinAuBytes(XHEAACENCLIB_HANDLE_AACENCODER self,
                                   int minFrameBytes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else {
    aacError = IISAACFENC_AacEncSetMinFrameBytes(self->hCoreCoder, minFrameBytes);
  }

  if (aacError != AACENC_NO_ERROR) {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetPceTimeInterval(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    float *sendPceTimeInterval) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  else {
    aacError = IISAACFENC_AacEncGetPceTimeInterval(self->hCoreCoder, sendPceTimeInterval);
  }
  if (aacError != AACENC_NO_ERROR) {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetPceTimeInterval(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    float sendPceTimeInterval) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else {
    aacError = IISAACFENC_AacEncSetPceTimeInterval(self->hCoreCoder, sendPceTimeInterval);
  }
  if (aacError != AACENC_NO_ERROR) {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncEncode(XHEAACENCLIB_HANDLE_AACENCODER self,
                            float *pSamples,
                            const int nSamples,
                            unsigned char *const pOutput,
                            unsigned char *const pOutputApr,
                            const int cbSize,
                            int *const cbOutBits,
                            int *const cbOutBitsApr,
                            unsigned char *pAncDrcBitstream,
                            unsigned int nAncDrcBytes,
                            int *coreModeNext,
                            XHEAACENCLIB_HANDLE_UNISTE *phUniSte,
                            unsigned int nBitsTransportOverhead,
                            const int bUsacIndepFlag,
                            HANDLE_EXTPAYLOAD_CONTAINER *hExtElement,
                            int numExtElementInUse,
                            XHEAAC_AACENC_IPF_STATE const ipfState,
                            int nChannels) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;
  AACENC_CODING_MODE aacCodingModeNext[SIGMAP_MAX_SIGNALS] = {AACENC_CODING_MODE_FD};
  AACENC_IPF_STATE mappedIpfStateAacenc = AACENC_IPF_STATE_NO;
  int ch = 0;

  for (ch = 0; !isError(retValue) && ch < nChannels; ch++) {
    switch (coreModeNext[ch]) {
      case 0:
        aacCodingModeNext[ch] = AACENC_CODING_MODE_FD;
        break;
      case 1:
        aacCodingModeNext[ch] = AACENC_CODING_MODE_LPD;
        break;
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
        break;
    }
  }

  if (!isError(retValue)) {
    retValue = mappingIpfStateFromXheAacInterfaceToAac(ipfState, &mappedIpfStateAacenc);
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_AacEncEncodeFrame(self->hCoreCoder,
                                            pSamples,
                                            nSamples,
                                            pOutput,
                                            pOutputApr,
                                            cbSize,
                                            cbOutBits,
                                            cbOutBitsApr,
                                            aacCodingModeNext,
                                            (const HANDLE_UNISTE *)phUniSte,
                                            pAncDrcBitstream,
                                            &nAncDrcBytes,
                                            nAncDrcBytes,
                                            nBitsTransportOverhead,
                                            bUsacIndepFlag,
                                            hExtElement,
                                            numExtElementInUse,
                                            mappedIpfStateAacenc);

    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncUpdateExtendedBitReservoir(XHEAACENCLIB_HANDLE_AACENCODER self,
                                                int nBits,
                                                const int intervalSamples,
                                                float preRollAUFactor) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_ExtendedBitReservoirUpdateParameters(self->hCoreCoder, nBits, intervalSamples, preRollAUFactor);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetBitReservoirLevel(XHEAACENCLIB_HANDLE_AACENCODER self, float bitReservoirLevel)

{
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_AacEncSetBitReservoirLevel(self->hCoreCoder, bitReservoirLevel);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BIT_RESERVOIR;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetBitsToUseForExternalData(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    int *maxNumBitsToUse,
    int *comfortableNumBitsToUse,
    const int bUsacIndepFlag,
    XHEAAC_AACENC_IPF_STATE ipfState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError;
  AACENC_IPF_STATE mappedIpfStateAacenc = AACENC_IPF_STATE_NO;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = mappingIpfStateFromXheAacInterfaceToAac(ipfState, &mappedIpfStateAacenc);
  }

  if (!isError(retValue)) {
    aacError = IISAACFENC_AacEncGetThresholdsForExternalBits(self->hCoreCoder, maxNumBitsToUse, comfortableNumBitsToUse, bUsacIndepFlag, mappedIpfStateAacenc);

    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BIT_DISTRIBUTION;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncIsLastShortWindow(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    int *isLastShortWindow) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AACFASTENC_ERROR aacError = AACENC_NO_ERROR;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (isLastShortWindow == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
  }

  if (!isError(retValue)) {
    aacError = iisaacfenc_isLastShortWindow(self->hCoreCoder, isLastShortWindow);
    if (aacError != AACENC_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN mappingIpfStateFromXheAacInterfaceToAac(
    XHEAAC_AACENC_IPF_STATE ipfStatXheAacenc,
    AACENC_IPF_STATE *pMappedIpfStateAacenc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (pMappedIpfStateAacenc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    switch (ipfStatXheAacenc) {
      case XHEAAC_AACENC_IPF_STATE_NO:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_NO;
        break;
      case XHEAAC_AACENC_IPF_STATE_RAP_FIRST_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_RAP_FIRST_PREROLL;
        break;
      case XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL;
        break;
      case XHEAAC_AACENC_IPF_STATE_RAP_NEXT_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_RAP_NEXT_PREROLL;
        break;
      case XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL;
        break;
      case XHEAAC_AACENC_IPF_STATE_RAP_IPF:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_RAP_IPF;
        break;
      case XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_IPF:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_CONFIGCHANGE_IPF;
        break;
      case XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_RAP_IPF_PREROLL;
        break;
      case XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_IPF_PREROLL:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_CONFIGCHANGE_IPF_PREROLL;
        break;
      default:
        *pMappedIpfStateAacenc = AACENC_IPF_STATE_NO;
        if (!isError(retValue)) {
          retValue = XHEAACENCLIB_RETURN_ERROR_IPF_INVALID;
        }
        break;
    }
  }

  return retValue;
}
