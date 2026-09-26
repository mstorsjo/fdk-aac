
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
#include <assert.h>

#include "iisxHEAACEncLib_asc_ifc.h"
#include "mp4audioheader.h"
#include "IIS_MP4AEnc_ASC_Interface.h"
#include "iisxHEAACEncLib_SigMap_ifc.h"
#include "iisxHEAACEncLib_common.h"

struct iisxHEAAC_asc_encoder_tag {
  HANDLE_ASC hAsc;
  HANDLE_CODER_CONFIG_INFO hCoderConfig;
  ASCBUF asc;
};

static XHEAACENCLIB_RETURN addExtInfoUSAC(XHEAACENCLIB_EXT_ELEMENT *extEle, HANDLE_USACC hUsacc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int type_supported = 0;
  int actIdx = 0;
  XHEAACENCLIB_EXT_ELE_TYPE type;

  if (extEle == NULL || hUsacc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    actIdx = hUsacc->usacDecoderConfig.numElements;
    type = extEle->dataType;
    hUsacc->usacDecoderConfig.usacElementType[actIdx] = USAC_ELEMENT_TYPE_EXT;

    switch (type) {
      case XHEAACENCLIB_EXT_ELE_FILL:
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = USAC_ID_EXT_ELE_FILL;
        type_supported = 1;
        break;
      case XHEAACENCLIB_EXT_ELE_MPEGS:
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = USAC_ID_EXT_ELE_MPEGS;
        type_supported = 1;
        break;
      case XHEAACENCLIB_EXT_ELE_SAOC:
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = USAC_ID_EXT_ELE_SAOC;
        type_supported = 1;
        break;
      case XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL:
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = USAC_ID_EXT_ELE_PREROLL;
        type_supported = 1;
        break;
      case XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC:
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = USAC_ID_EXT_ELE_UNI_DRC;
        type_supported = 1;
        break;
      default:
        assert(0);
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_EXT_ELEMENT;
        break;
    }
  }

  if (!isError(retValue)) {
    if (type_supported) {
      hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementConfigLength = extEle->configLength;

      memcpy(hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementConfigPayload,
             extEle->config, extEle->configLength);

      hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementDefaultLengthPresent =
          hasExtensionPayloadContainerFeature(extEle->container, FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH);

      if (hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementDefaultLengthPresent) {
        hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementDefaultLength = extEle->defaultLength;
      }

      hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementPayloadFrag =
          hasExtensionPayloadContainerFeature(extEle->container, FEATURE_USAC_EXT_PAYLOAD_SUPPORT_FRAGMENTATION);
    }
  }

  if (!isError(retValue)) {
    if (extEle->container)
      resetExtensionPayloadContainer(extEle->container);
  }

  if (!isError(retValue)) {
    hUsacc->usacDecoderConfig.numElements++;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN addFillInfoUSAC(HANDLE_USACC hUsacc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int actIdx = 0;

  if (hUsacc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    actIdx = hUsacc->usacDecoderConfig.numElements;

    hUsacc->usacDecoderConfig.usacElementType[actIdx] = USAC_ELEMENT_TYPE_EXT;

    hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementType = XHEAACENCLIB_EXT_ELE_FILL;
    hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementDefaultLengthPresent = 0;
    hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementDefaultLength = 0;
    hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementConfigLength = 0;
    hUsacc->usacDecoderConfig.usacElementConfig[actIdx].usacExtConfig.usacExtElementPayloadFrag = 0;

    hUsacc->usacDecoderConfig.numElements++;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN mapFrameLength(USAC_CONFIG_OUT_FRAMLENGTH *const outFrameLength,
                                          FRAMELENGTH const inFrameLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (outFrameLength == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    switch (inFrameLength) {
      case FRAMELENGTH_768:
        *outFrameLength = USAC_CONFIG_OUT_FRAMELENGTH_768;
        break;
      case FRAMELENGTH_1024:
        *outFrameLength = USAC_CONFIG_OUT_FRAMELENGTH_1024;
        break;
      case FRAMELENGTH_2048:
        *outFrameLength = USAC_CONFIG_OUT_FRAMELENGTH_2048;
        break;
      case FRAMELENGTH_4096:
        *outFrameLength = USAC_CONFIG_OUT_FRAMELENGTH_4096;
        break;
      case FRAMELENGTH_960:
      case FRAMELENGTH_1920:
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_FRAME_SIZE;
        break;
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN fillUsacConfig(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                          HANDLE_USACC hUsacc,
                                          XHEAACENCLIB_HANDLE_AACENCODER hAacEnc,
                                          XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                          XHEAACENCLIB_HANDLE_MPEGSENCODER hMpegsEnc,
                                          XHEAACENCLIB_EXT_ELEMENT *extEle,
                                          const int numExtEle,
                                          XHEAACENCLIB_CONFIG_EXTENSION *configExtension,
                                          const int numConfigExtension) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  USAC_ELEMENT_TYPE eleType[USAC_MAX_ELEMENTS];
  int numCoreElement = 0;
  int eleIdx = 0;
  int extElementIdx;

  if (hConfig == NULL || hUsacc == NULL || extEle == NULL || configExtension == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hUsacc->usacChannelConfig.numOutChannels = 0;
    hUsacc->usacConfigExtension.numConfigExtensions = 0;
    hUsacc->usacSamplingFrequency = (unsigned int)hConfig->sampleRateOut;
    retValue = mapFrameLength(&hUsacc->outputFrameLength, hConfig->nFrameSamples);
  }

  if (!isError(retValue)) {
    if (!hConfig->bUseSBR) {
      hUsacc->sbrRatioIndex = USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR;
    } else if (hConfig->bUseSBR41) {
      hUsacc->sbrRatioIndex = USAC_CONFIG_SBR_RATIO_INDEX_4_1;
    } else if (hConfig->granuleLength == XHEAACENCLIB_GRANULELENGTH_768) {
      hUsacc->sbrRatioIndex = USAC_CONFIG_SBR_RATIO_INDEX_8_3;
    } else {
      hUsacc->sbrRatioIndex = USAC_CONFIG_SBR_RATIO_INDEX_2_1;
    }

    hUsacc->channelConfigurationIndex = hConfig->CicpIndex;
    hUsacc->usacDecoderConfig.numElements = 0;
  }

  if (!isError(retValue)) {
    IIS_XHEAACENCLIB_SIGMAP_ERROR retValueSigMapIfc = IIS_XHEAACENCLIB_SIGMAP_NO_ERROR;
    retValueSigMapIfc = iisxHEAACEncLib_SigMap_GetCoreElements(hConfig->CicpIndex, eleType, &numCoreElement);
    if (retValueSigMapIfc != IIS_XHEAACENCLIB_SIGMAP_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SIGMAP;
    }
  }

  if (!isError(retValue)) {
    for (extElementIdx = 0; extElementIdx < numExtEle; ++extElementIdx) {
      if (extEle[extElementIdx].signalElemBelong == 0) {
        retValue = addExtInfoUSAC(&extEle[extElementIdx], hUsacc);
      }
    }
  }

  if (!isError(retValue)) {
    int configExtensionIdx;
    int copyToCfgExt = 0;
    for (configExtensionIdx = 0; configExtensionIdx < numConfigExtension; configExtensionIdx++) {
      switch (configExtension[configExtensionIdx].dataType) {
        case XHEAACENCLIB_ID_CONFIG_EXT_FILL:
          hUsacc->usacConfigExtension.usacConfigExtType[configExtensionIdx] = USAC_ID_CONFIG_EXT_FILL;
          copyToCfgExt = 1;
          break;

        case XHEAACENCLIB_ID_CONFIG_EXT_LOUDNESS_INFO:
          hUsacc->usacConfigExtension.usacConfigExtType[configExtensionIdx] = USAC_ID_CONFIG_EXT_LOUDNESS_INFO;
          copyToCfgExt = 1;
          break;
        case XHEAACENCLIB_ID_CONFIG_EXT_STREAM_ID:
          hUsacc->usacConfigExtension.usacConfigExtType[configExtensionIdx] = USAC_ID_CONFIG_EXT_STREAM_ID;
          copyToCfgExt = 1;
          break;
        default:
          break;
      }
      if (copyToCfgExt == 1) {
        hUsacc->usacConfigExtension.usacConfigExtLength[configExtensionIdx] = configExtension[configExtensionIdx].configLength;
        memcpy(&hUsacc->usacConfigExtension.usacConfigExtPayload[configExtensionIdx], &configExtension[configExtensionIdx].config, configExtension[configExtensionIdx].configLength);
        hUsacc->usacConfigExtension.numConfigExtensions++;
      }
    }
  }

  for (eleIdx = 0; eleIdx < numCoreElement; eleIdx++) {
    USAC_CORE_CONFIG *pUsacCoreConfig = NULL;
    USAC_SBR_CONFIG *pUsacSbrConfig = NULL;
    USAC_MPS212_CONFIG *pUsacMps212Config = NULL;
    MP4SPACEENC_USAC_MPS212_CONFIG usacMps212ConfigTmp;
    unsigned int numElement = hUsacc->usacDecoderConfig.numElements;
    hUsacc->usacDecoderConfig.numElements++;
    if (!isError(retValue)) {
      hUsacc->usacDecoderConfig.usacElementType[numElement] = eleType[eleIdx];
      switch (eleType[eleIdx]) {
        case USAC_ELEMENT_TYPE_SCE:
          pUsacCoreConfig = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacSceConfig.usacCoreConfig;
          pUsacSbrConfig = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacSceConfig.usacSbrConfig;
          break;
        case USAC_ELEMENT_TYPE_CPE:
          pUsacCoreConfig = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacCpeConfig.usacCoreConfig;
          pUsacSbrConfig = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacCpeConfig.usacSbrConfig;
          pUsacMps212Config = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacCpeConfig.usacMps212Config;
          hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacCpeConfig.stereoConfigIndex = hConfig->stereoConfigIndex;
          break;
        case USAC_ELEMENT_TYPE_LFE:
          pUsacCoreConfig = &hUsacc->usacDecoderConfig.usacElementConfig[numElement].usacLfeConfig.usacCoreConfig;
          pUsacCoreConfig->tw_mdct = 0;
          pUsacCoreConfig->noiseFilling = 0;
          break;
        default:

          assert(0);
          break;
      }
    }
    if ((!isError(retValue)) && (USAC_ELEMENT_TYPE_LFE != eleType[eleIdx]) && (USAC_ELEMENT_TYPE_EXT != eleType[eleIdx])) {
      pUsacCoreConfig->noiseFilling = iisxHEAACEncLibAacEncUseNoiseFilling(hAacEnc);
      if (hUsacc->sbrRatioIndex != USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR) {
        pUsacSbrConfig->harmonicSBR = iisxHEAACEncLibSbrEncGetHbePresent(hSbrEnc, eleIdx);
        pUsacSbrConfig->bs_interTes = iisxHEAACEncLibSbrEncGetBsInterTes(hSbrEnc, eleIdx);
        pUsacSbrConfig->bs_pvc = iisxHEAACEncLibSbrEncGetPvcPresent(hSbrEnc, eleIdx);

        retValue = iisxHEAACEncLibSbrEncGetUsacSbrDfltHeaderData(hSbrEnc, &(pUsacSbrConfig->sbrDfltHeader), eleIdx);
        if (isError(retValue)) {
          break;
        }
      }
    }

    if ((hUsacc->sbrRatioIndex != USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR) &&
        (hConfig->stereoConfigIndex > 0) &&
        (USAC_ELEMENT_TYPE_LFE != eleType[eleIdx] && USAC_ELEMENT_TYPE_EXT != eleType[eleIdx])) {
      if (!isError(retValue)) {
        retValue = iisxHEAACEncLibMpegsEncGetUsacMps212Config(hMpegsEnc, eleIdx, &usacMps212ConfigTmp);
      }

      if (!isError(retValue)) {
        pUsacMps212Config->bsFreqRes = usacMps212ConfigTmp.bsFreqRes;
        pUsacMps212Config->bsFixedGainDMX = usacMps212ConfigTmp.bsFixedGainDMX;
        pUsacMps212Config->bsTempShapeConfig = usacMps212ConfigTmp.bsTempShapeConfig;
        pUsacMps212Config->bsDecorrConfig = usacMps212ConfigTmp.bsDecorrConfig;
        pUsacMps212Config->bsHighRateMode = usacMps212ConfigTmp.bsHighRateMode;
        pUsacMps212Config->bsPhaseCoding = usacMps212ConfigTmp.bsPhaseCoding;
        pUsacMps212Config->bsOttBandsPhasePresent = usacMps212ConfigTmp.bsOttBandsPhasePresent;
        pUsacMps212Config->bsOttBandsPhase = usacMps212ConfigTmp.bsOttBandsPhase;
        pUsacMps212Config->bsResidualBands = usacMps212ConfigTmp.bsResidualBands;
        pUsacMps212Config->bsPseudoLr = usacMps212ConfigTmp.bsPseudoLr;
        pUsacMps212Config->bsEnvQuantMode = usacMps212ConfigTmp.bsEnvQuantMode;
      }
    }

    if (!isError(retValue)) {
      for (extElementIdx = 0; extElementIdx < numExtEle; ++extElementIdx) {
        if (extEle[extElementIdx].signalElemBelong == eleIdx + 1) {
          retValue = addExtInfoUSAC(&extEle[extElementIdx], hUsacc);
        }
      }
    }
  }

  if (!isError(retValue)) {
    for (extElementIdx = 0; extElementIdx < numExtEle; ++extElementIdx) {
      if (extEle[extElementIdx].signalElemBelong == -1) {
        retValue = addExtInfoUSAC(&extEle[extElementIdx], hUsacc);
      }
    }
  }

  if (!isError(retValue)) {
    retValue = addFillInfoUSAC(hUsacc);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN getASC(
    HANDLE_CODER_CONFIG_INFO hCoderConfig,
    ASCBUF *hAscBuf) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  ASC_BUFFER asc[MAX_LAYERS];
  int i;

  if (hCoderConfig == NULL || hAscBuf == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_Mp4ASC_AudioSpecificConfigGet(hCoderConfig, &asc[0]);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_ASC;
    }
  }

  if (!isError(retValue)) {
    hAscBuf->nAscSizeBits = asc[0].ascSize;
    for (i = 0; i < ((hAscBuf->nAscSizeBits + 7) >> 3); i++) {
      hAscBuf->pAsc[i] = asc[0].asc[i];
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN openAudioSpecificConfig(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                            XHEAACENCLIB_HANDLE_AACENCODER hAacEnc,
                                            XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                            XHEAACENCLIB_HANDLE_MPEGSENCODER hMpegsEnc,
                                            XHEAACENCLIB_EXT_ELEMENT *extEle,
                                            const int numExtEle,
                                            XHEAACENCLIB_CONFIG_EXTENSION *configExtension,
                                            const int numConfigExtension,
                                            ASCBUF *asc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  HANDLE_CODER_CONFIG_INFO hCoderConfig = NULL;
  HANDLE_USACC hUsacc = NULL;

  if (hConfig == NULL || extEle == NULL || configExtension == NULL || asc == NULL || hAacEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->bUseSBR && hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_Mp4ASC_CoderConfigInfoCreate(&hCoderConfig, 1, 1);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_ASC;
    }
  }

  if (!isError(retValue)) {
    switch (hConfig->aot) {
      case AUD_OBJ_TYP_USAC:
        hUsacc = (HANDLE_USACC)ngsMalloc(sizeof(USAC_CONFIG));
        if (hUsacc == NULL) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
        }
        break;
      default:
        break;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      retValue = fillUsacConfig(hConfig, hUsacc,
                                hAacEnc, hSbrEnc, hMpegsEnc,
                                extEle, numExtEle,
                                configExtension, numConfigExtension);
    }
  }

  if (!isError(retValue)) {
    hCoderConfig[0].samplingRate = (int)(hConfig->sampleRateAAC);
    hCoderConfig[0].bitRate = hConfig->bitRate;
    hCoderConfig[0].samplesFrame = 1024;
    hCoderConfig[0].bitsFrame = (int)((hCoderConfig[0].bitRate *
                                       (float)hCoderConfig[0].samplesFrame) /
                                      hCoderConfig[0].samplingRate);

    hCoderConfig[0].asc.samplingFrequency = (unsigned int)hCoderConfig[0].samplingRate;
    hCoderConfig[0].asc.epConfig = 0;
    hCoderConfig[0].asc.directMapping = 0;

    switch (hConfig->aot) {
      case AUD_OBJ_TYP_LC:
      case AUD_OBJ_TYP_MP2_LC:
      case AUD_OBJ_TYP_MP2_SBR:
        hCoderConfig[0].asc.aot = AOT_AAC_LC;
        break;
      case AUD_OBJ_TYP_HEAAC:
        hCoderConfig[0].asc.aot = AOT_AAC_LC;
        break;

      case AUD_OBJ_TYP_PS:
        hCoderConfig[0].asc.aot = AOT_AAC_LC;
        break;

      case AUD_OBJ_TYP_USAC:
        hCoderConfig[0].asc.aot = AOT_USAC;
        hCoderConfig[0].asc.samplingFrequency = (hConfig->bUseSBR) ? (unsigned int)hConfig->sampleRateOut : hCoderConfig[0].asc.samplingFrequency;
        hCoderConfig[0].samplingRate = (hConfig->bUseSBR) ? (int)hConfig->sampleRateOut : hCoderConfig[0].samplingRate;
        break;

      default:
        hCoderConfig[0].asc.aot = AOT_AAC_LC;
        hCoderConfig[0].asc.samplingFrequency = (unsigned int)hConfig->sampleRateOut;
    }

    hCoderConfig[0].asc.epConfig = 0;
    hCoderConfig[0].asc.directMapping = 0;

    if ((hConfig->CicpIndex >= XHEAACENCLIB_SIGMAP_CICP_1) && (hConfig->CicpIndex <= XHEAACENCLIB_SIGMAP_CICP_14)) {
      hCoderConfig[0].asc.channelConfiguration = hConfig->CicpIndex;
      switch (hConfig->CicpIndex) {
        case XHEAACENCLIB_SIGMAP_CICP_1:
          if (hConfig->nChannelsCoreCoder == 2) {
            hCoderConfig[0].asc.channelConfiguration = 2;
          }
          break;
        case XHEAACENCLIB_SIGMAP_CICP_2:
          if (hConfig->aot == AUD_OBJ_TYP_PS) {
            hCoderConfig[0].asc.channelConfiguration = 1;
          }
          break;
        default:
          break;
      }
    } else {
      hCoderConfig[0].asc.channelConfiguration = 0;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      hCoderConfig[0].asc.hAotSpecificPart.aotSpecificConfig =
          (struct AOT_SPECIFIC_CONFIG *)hUsacc;

      hCoderConfig[0].asc.hAotSpecificPart.WriteAotSpecificConfig =
          (void (*)(struct AUDIO_SPECIFIC_CONFIG * audioSpecificConfig,
                    struct AOT_SPECIFIC_CONFIG * aotSpecificConfig,
                    HANDLE_BITBUFFER hBitBuffer)) &
          WriteUsacConfig;

      hCoderConfig[0].ascFlag = 1;
    }

    hConfig->hCoderConfig = hCoderConfig;
  }

  if (asc && !isError(retValue)) {
    asc->pAsc = (unsigned char *)ngsMalloc(ASC_SIZE * sizeof(unsigned char));
    if (asc->pAsc != NULL) {
      retValue = getASC(hConfig->hCoderConfig, asc);
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }

  return retValue;
}

void closeAudioSpecificConfig(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                              ASCBUF *asc) {
  if (hConfig->hCoderConfig) {
    if (hConfig->hCoderConfig[0].asc.hAotSpecificPart.aotSpecificConfig) {
      AUDIO_OBJECT_TYPE aot = hConfig->hCoderConfig[0].asc.aot;
      if ((aot >= AOT_AAC_MAIN) && (aot != AOT_USAC)) {
      }
      iisFree(hConfig->hCoderConfig[0].asc.hAotSpecificPart.aotSpecificConfig);
    }
    IIS_Mp4ASC_CoderConfigInfoDelete(hConfig->hCoderConfig);
    hConfig->hCoderConfig = NULL;
  }

  if (asc->pAsc) {
    iisFree(asc->pAsc);
    asc->pAsc = NULL;
  }
}
