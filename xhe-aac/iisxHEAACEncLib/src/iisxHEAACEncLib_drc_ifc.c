
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mathlib.h"
#include "iisxHEAACEncLib_drc_ifc.h"
#include "iisDRCRealtimeLRAControl_api.h"
#include "iisxHEAACEncLib_common.h"

#define MAX_DRC_ENC_PAYLOAD_BYTES 2048
#define MAX_DRC_ENC_PAYLOAD_BITS (MAX_DRC_ENC_PAYLOAD_BYTES << 3)

typedef struct {
  float *lraControlDrcGains;
  unsigned int lraControlDrcGainsLength;
  unsigned int lraControlDrcGainProcessingIsActive;
} DRC_IFC_LRACONTROL_GAIN_DATA;

typedef struct drc_ifc_data_struct {
  int nSize;
  int signalGroupCnt;
  unsigned char pUniDrcConfigBs[MAX_DRC_ENC_PAYLOAD_BYTES];
  int iUniDrcConfigLength;
  unsigned char pUniDrcGainBs[MAX_DRC_ENC_PAYLOAD_BYTES];
  int iUniDrcGainLength;

  float **pFrameBuffer;
  float **gainBuffer;

  HANDLE_DRCGAINGEN_STATES hDRCGainGenStates;
  HANDLE_DRCGAINGEN_PARAMS hDRCGainGenParams;
  HANDLE_IISDRCGAINENC_PARAMS hDRCGainEncoder;
  UniDrcConfig *pUniDrcConfig;
  LoudnessInfoSet *pLoudnessInfoSet;
  IISDRCGAINENC_CONFIG *config_drcGainEnc;
  CONFIG_DRCGAINGEN *config_drcGainGen;
  DRC_CHARACTERISTIC_INDEX outsideCharacteristics[INPUT_GAINSET_COUNT_MAX];
  HANDLE_RT_LRAC hRtLRAC;
  int bRealtimeLRAC;
  float targetLra;
  int bTargetLraSet;
  float lraInputSignal;
  unsigned int lraCountInputSignal;

  int nSequences;
  int nAudioChannels;
  int nFrameLength;

  int decoderDelay;
  int maxCompressorLookAhead;
  int bCopyGainsBefore;

  DRC_IFC_LRACONTROL_GAIN_DATA lraControlDrcGainData;

  int uniDrcLoudnessInfoSetPresent;
  int uniDrcConfigPresent;
  int sampleRate;

} DRC_IFC_DATA;

static DRC_IFC_RETURN iisxHEAACEncLib_GetDrcGainGenParams(XHEAACENCLIB_HANDLE_DRCENCODER hInstance, HANDLE_DRCGAINGEN_PARAMS **phDrcGainGenParams);

static DRC_IFC_RETURN openDrcBuffers(XHEAACENCLIB_HANDLE_DRCENCODER hInstance) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;
  int k = 0, ch = 0;

  if ((hInstance->nSequences <= 0) || (hInstance->nFrameLength <= 0) || (hInstance->nAudioChannels <= 0)) {
    hInstance->gainBuffer = NULL;
    hInstance->pFrameBuffer = NULL;
    return DRC_IFC_ERROR_MEMORY;
  }

  hInstance->gainBuffer = (float **)iisMalloc(sizeof(float *) * hInstance->nSequences);
  if (NULL == hInstance->gainBuffer) {
    retValueDrc = DRC_IFC_ERROR_MEMORY;
  } else {
    for (k = 0; k < hInstance->nSequences; k++) {
      hInstance->gainBuffer[k] = (float *)iisMalloc(sizeof(float) * hInstance->nFrameLength);
      if (NULL == hInstance->gainBuffer[k]) {
        retValueDrc = DRC_IFC_ERROR_MEMORY;
        break;
      }
      memset(hInstance->gainBuffer[k], 0, hInstance->nFrameLength * sizeof(float));
    }
  }

  hInstance->pFrameBuffer = (float **)iisMalloc(sizeof(float *) * hInstance->nAudioChannels);
  if (NULL == hInstance->pFrameBuffer) {
    retValueDrc = DRC_IFC_ERROR_MEMORY;
  } else {
    for (ch = 0; ch < hInstance->nAudioChannels; ch++) {
      hInstance->pFrameBuffer[ch] = (float *)iisMalloc(sizeof(float) * (hInstance->nFrameLength + hInstance->maxCompressorLookAhead));
      if (NULL == hInstance->pFrameBuffer[ch]) {
        retValueDrc = DRC_IFC_ERROR_MEMORY;
        break;
      }
    }
  }

  return retValueDrc;
}

static DRC_IFC_RETURN closeDrcBuffers(XHEAACENCLIB_HANDLE_DRCENCODER hInstance) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;
  int k, ch;

  if (hInstance->pFrameBuffer) {
    for (ch = 0; ch < hInstance->nAudioChannels; ch++) {
      iisFree(hInstance->pFrameBuffer[ch]);
    }
    iisFree(hInstance->pFrameBuffer);
  }

  if (hInstance->gainBuffer) {
    for (k = 0; k < hInstance->nSequences; k++) {
      iisFree(hInstance->gainBuffer[k]);
    }
    iisFree(hInstance->gainBuffer);
  }

  return retValueDrc;
}

static int
configureEnc1(UniDrcConfig *pUniDrcConfig,
              int baseChannelCount,
              XHEAACENCLIB_DRCENCODER_SETUP *setup) {
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  UniDrcConfigExtension *uniDrcConfigExtensionTmp = NULL;
  int n, g, s, ch, b, gainSequenceIndex, drcInstructionsUniDrcMaxCount, gainSequenceMaxCount;
  int allEffect = 0;
  if (setup->aot != AUD_OBJ_TYP_USAC) {
    pUniDrcConfig->sampleRatePresent = 1;
    pUniDrcConfig->sampleRate = setup->sampleRate;
  } else {
    pUniDrcConfig->sampleRatePresent = 0;
  }
  pUniDrcConfig->downmixInstructionsCount = 0;
  pUniDrcConfig->drcDescriptionBasicPresent = 0;
  pUniDrcConfig->drcCoefficientsBasicCount = 0;
  pUniDrcConfig->drcInstructionsBasicCount = 0;

  pUniDrcConfig->drcCoefficientsUniDrcCount = 0;
  pUniDrcConfig->drcInstructionsUniDrcCount = 0;

  pUniDrcConfig->uniDrcConfigExtPresent = 1;
  pUniDrcConfig->uniDrcConfigExtension.numUniDrcConfigExtensions = 2;

  pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType = (IISDRCGAINENC_UNIDRCCONFEXT_TYPE *)iisCalloc(pUniDrcConfig->uniDrcConfigExtension.numUniDrcConfigExtensions, sizeof(IISDRCGAINENC_UNIDRCCONFEXT_TYPE));
  if (pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType == NULL) {
    return 33;
  }
  pUniDrcConfig->uniDrcConfigExtension.pExtBitSize = (int *)iisCalloc(pUniDrcConfig->uniDrcConfigExtension.numUniDrcConfigExtensions, sizeof(int));
  if (pUniDrcConfig->uniDrcConfigExtension.pExtBitSize == NULL) {
    return 33;
  }

  pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType[0] = IISDRCGAINENC_UNIDRCCONFEXT_V1;
  pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType[1] = IISDRCGAINENC_UNIDRCCONFEXT_TERM;

  uniDrcConfigExtensionTmp = &pUniDrcConfig->uniDrcConfigExtension;

  if (baseChannelCount == 2) {
    uniDrcConfigExtensionTmp->downmixInstructionsV1Present = 1;

    uniDrcConfigExtensionTmp->downmixInstructionsV1Count = 1;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1 = (DownmixInstructions *)iisCalloc(1, sizeof(DownmixInstructions));
    if (uniDrcConfigExtensionTmp->pDownmixInstructionsV1 == NULL) {
      return 33;
    }

    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].downmixId = 1;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].targetChannelCount = 1;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].targetLayout = 1;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].downmixCoefficientsPresent = 1;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].pDownmixCoefficient = (float *)iisCalloc(baseChannelCount, sizeof(float));
    if (uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].pDownmixCoefficient == NULL) {
      return 33;
    }
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].pDownmixCoefficient[0] = 0.7079f;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].pDownmixCoefficient[1] = 0.7079f;
    uniDrcConfigExtensionTmp->pDownmixInstructionsV1[0].pLfeChannel = NULL;
  }

  uniDrcConfigExtensionTmp->drcCoeffsAndInstructionsUniDrcV1Present = 1;

  retValueDrcEnc = iisDRCGainEnc_getDrcInstructionsUniDrcMaxCount(&drcInstructionsUniDrcMaxCount);
  if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) return 4;

  if (setup->nInstructionCount < 0 || setup->nInstructionCount > drcInstructionsUniDrcMaxCount) {
    return 1;
  }
  uniDrcConfigExtensionTmp->drcInstructionsUniDrcV1Count = setup->nInstructionCount;

  uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1 = (DrcInstructionsUniDrc *)iisCalloc(setup->nInstructionCount, sizeof(DrcInstructionsUniDrc));
  if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1 == NULL) {
    return 33;
  }

  for (n = 0; n < uniDrcConfigExtensionTmp->drcInstructionsUniDrcV1Count; n++) {
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetId = n + 1;

    if (setup->instructionSetup[n].drcSetEffect & IISDRCGAINENC_DRCSETEFFECT_BIT_FADE) {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixIdPresent = 1;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixId = 0x7F;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcApplyToDownmix = 1;
    } else {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixIdPresent = 0;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixId = 0;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcApplyToDownmix = 0;
    }
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].additionalDownmixIdPresent = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].additionalDownmixIdCount = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcLocation = 1;
    if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixIdPresent == 1 && uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].downmixId == 0x7F && uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcApplyToDownmix == 1) {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcChannelCount = 1;
    } else {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcChannelCount = baseChannelCount;
    }

    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].dependsOnDrcSetPresent = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].dependsOnDrcSet = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].noIndependentUse = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].requiresEq = 0;

    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainSetIndex = (int *)iisCalloc(baseChannelCount, sizeof(int));
    if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainSetIndex == NULL) {
      return 33;
    }

    for (ch = 0; ch < baseChannelCount; ch++) {
      if (setup->instructionSetup[n].gainSetIndex >= setup->nCharacteristicCount || setup->instructionSetup[n].gainSetIndex < 0) {
        return 1;
      }
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainSetIndex[ch] = setup->instructionSetup[n].gainSetIndex;
    }

    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcChannelGroupCount = 1;

    if ((setup->instructionSetup[n].drcSetEffect & IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER) || (setup->instructionSetup[n].drcSetEffect & IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF)) {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pDuckingModifications = (DuckingModifications *)iisCalloc(uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcChannelCount, sizeof(DuckingModifications));
      if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pDuckingModifications == NULL) {
        return 33;
      }
      for (ch = 0; ch < baseChannelCount; ch++) {
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pDuckingModifications[ch].duckingScalingPresent = 0;
      }
    } else {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications = (GainModifications *)iisCalloc(uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcChannelGroupCount, sizeof(GainModifications));
      if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications == NULL) {
        return 33;
      }

      g = 0;
      {
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainScalingPresent = (int *)iisCalloc(1, sizeof(int));
        if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainScalingPresent == NULL) {
          return 33;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAttenuationScaling = (float *)iisCalloc(1, sizeof(float));
        if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAttenuationScaling == NULL) {
          return 33;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAmplificationScaling = (float *)iisCalloc(1, sizeof(float));
        if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAmplificationScaling == NULL) {
          return 33;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffsetPresent = (int *)iisCalloc(1, sizeof(int));
        if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffsetPresent == NULL) {
          return 33;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffset = (float *)iisCalloc(1, sizeof(float));
        if (uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffset == NULL) {
          return 33;
        }

        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].shapeFilterPresent = 0;
        if (setup->instructionSetup[n].attenuationScaling != 1.f || setup->instructionSetup[n].amplificationScaling != 1.f) {
          uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainScalingPresent[0] = 1;
        } else {
          uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainScalingPresent[0] = 0;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAttenuationScaling[0] = setup->instructionSetup[n].attenuationScaling;
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pAmplificationScaling[0] = setup->instructionSetup[n].amplificationScaling;
        if (setup->instructionSetup[n].gainOffset != 0.0f) {
          uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffsetPresent[0] = 1;
        } else {
          uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffsetPresent[0] = 0;
        }
        uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].pGainModifications[g].pGainOffset[0] = setup->instructionSetup[n].gainOffset;
      }
    }
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].limiterPeakTargetPresent = 0;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].limiterPeakTarget = 0.0f;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetEffect = setup->instructionSetup[n].drcSetEffect;

    allEffect |= setup->instructionSetup[n].drcSetEffect;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessPresent = 1;
    uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueUpper = setup->instructionSetup[n].TargetLoudnessValueUpper;
    if (setup->instructionSetup[n].TargetLoudnessValueLower < setup->instructionSetup[n].TargetLoudnessValueUpper) {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLowerPresent = 1;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLower = setup->instructionSetup[n].TargetLoudnessValueLower;
    } else {
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLowerPresent = 0;
      uniDrcConfigExtensionTmp->pDrcInstructionsUniDrcV1[n].drcSetTargetLoudnessValueLower = 0;
    }
  }

  if ((allEffect & (IISDRCGAINENC_DRCSETEFFECT_BIT_NIGHT | IISDRCGAINENC_DRCSETEFFECT_BIT_NOISY | IISDRCGAINENC_DRCSETEFFECT_BIT_LIMITED | IISDRCGAINENC_DRCSETEFFECT_BIT_GENERAL_COMPR)) != (IISDRCGAINENC_DRCSETEFFECT_BIT_NIGHT | IISDRCGAINENC_DRCSETEFFECT_BIT_NOISY | IISDRCGAINENC_DRCSETEFFECT_BIT_LIMITED | IISDRCGAINENC_DRCSETEFFECT_BIT_GENERAL_COMPR) &&
      (!((allEffect & (!IISDRCGAINENC_DRCSETEFFECT_BIT_FADE)) == 0) && (allEffect & IISDRCGAINENC_DRCSETEFFECT_BIT_FADE))) {
    return 1;
  }

  uniDrcConfigExtensionTmp->drcCoefficientsUniDrcV1Count = 1;

  uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1 = (DrcCoefficientsUniDrc *)iisCalloc(uniDrcConfigExtensionTmp->drcCoefficientsUniDrcV1Count, sizeof(DrcCoefficientsUniDrc));
  if (uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1 == NULL) {
    return 33;
  }

  gainSequenceIndex = 0;
  n = 0;
  {
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].drcFrameSizePresent = 0;
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].drcLocation = 1;

    retValueDrcEnc = iisDRCGainEnc_getGainSequenceMaxCount(&gainSequenceMaxCount);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) return 4;

    if (setup->nCharacteristicCount < 0 || setup->nCharacteristicCount >= gainSequenceMaxCount) {
      return 1;
    }
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].gainSequenceCount = setup->nCharacteristicCount;
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].gainSetCount = setup->nCharacteristicCount;
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].drcCharacteristicLeftPresent = 0;
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].drcCharacteristicRightPresent = 0;
    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].shapeFiltersPresent = 0;

    uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams = (GainSetParams *)iisCalloc(setup->nCharacteristicCount, sizeof(GainSetParams));
    if (uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams == NULL) {
      return 33;
    }

    for (s = 0; s < uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].gainSetCount; s++) {
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].gainCodingProfile = setup->coefficientsSetup[0].gainCodingProfile[s];
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].gainInterpolationType = IISDRCGAINENC_GAININTERPOLATIONTYPE_LINEAR;
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].fullFrame = 0;
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].timeAlignment = 0;
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].timeDeltaMinPresent = 0;
      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].bandCount = 1;

      uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams = (GainSequenceParams *)iisCalloc(uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].bandCount, sizeof(GainSequenceParams));
      if (uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams == NULL) {
        return 33;
      }

      b = 0;
      {
        if (setup->coefficientsSetup[0].drcCharacteristics[s] <= 0) {
          uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].drcCharacteristicPresent = 0;
        } else {
          uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].drcCharacteristicPresent = 1;
        }
        uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].drcCharacteristicFormatIsCICP = 1;
        uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].drcCharacteristic = setup->coefficientsSetup[0].drcCharacteristics[s];
        uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].crossoverFreqIndex = 0;
        uniDrcConfigExtensionTmp->pDrcCoefficientsUniDrcV1[n].pGainSetParams[s].pGainSequenceParams[b].gainSequenceIndex = gainSequenceIndex++;
      }
    }
  }

  uniDrcConfigExtensionTmp->loudEqInstructionsPresent = 0;

  pUniDrcConfig->channelLayout.baseChannelCount = baseChannelCount;
  pUniDrcConfig->channelLayout.layoutSignalingPresent = 0;
  pUniDrcConfig->channelLayout.definedLayout = 0;

  return (0);
}

static XHEAACENCLIB_RETURN drcExtGainOverwrite(XHEAACENCLIB_HANDLE_DRCENCODER const hInstance,
                                               DRC_IFC_LRACONTROL_GAIN_DATA const lraControlDrcGainData) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == hInstance || lraControlDrcGainData.lraControlDrcGains == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  else if (hInstance->nSequences >= INPUT_GAINSET_COUNT_MAX) {
    retValue = XHEAACENCLIB_RETURN_ERROR_DRC_TOO_MANY_SEQUENCES;
  }

  if (!isError(retValue)) {
    int sequence;

    for (sequence = 0; sequence < hInstance->nSequences; sequence++) {
      if (DRC_CHAR_UNDEFINED == hInstance->outsideCharacteristics[sequence]) {
        unsigned int sample;

        for (sample = 0; sample < (unsigned int)hInstance->nFrameLength; sample++) {
          if (sample < lraControlDrcGainData.lraControlDrcGainsLength) {
            hInstance->gainBuffer[sequence][sample] = lraControlDrcGainData.lraControlDrcGains[sample];
          } else {
            hInstance->gainBuffer[sequence][sample] = 0.0f;
          }
        }
      }
    }
  }

  return retValue;
}

static DRC_IFC_RETURN drcExtGainInit(XHEAACENCLIB_HANDLE_DRCENCODER hInstance, XHEAACENCLIB_DRCENCODER_SETUP *setup) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  int gainSequenceMaxCount = 0;
  int drcInstructionsUniDrcMaxCount = 0;

  if (hInstance == NULL) {
    retValueDrc = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    retValueDrcEnc = iisDRCGainEnc_getGainSequenceMaxCount(&gainSequenceMaxCount);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrc = DRC_IFC_ERROR_UNKNOWN;
    }
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    retValueDrcEnc = iisDRCGainEnc_getDrcInstructionsUniDrcMaxCount(&drcInstructionsUniDrcMaxCount);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrc = DRC_IFC_ERROR_UNKNOWN;
    }
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    if (setup->nCharacteristicCount > INPUT_GAINSET_COUNT_MAX) {
      retValueDrc = DRC_IFC_ERROR_INVALID_SETUP;
    }
  }
  if (retValueDrc == DRC_IFC_NO_ERROR) {
    if (setup->nInstructionCount > drcInstructionsUniDrcMaxCount) {
      retValueDrc = DRC_IFC_ERROR_INVALID_SETUP;
    }
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    int characteristic = 0;

    for (characteristic = 0; characteristic < setup->nCharacteristicCount; characteristic++) {
      hInstance->outsideCharacteristics[characteristic] = setup->coefficientsSetup[0].drcCharacteristics[characteristic];
    }
    for (; characteristic < INPUT_GAINSET_COUNT_MAX; characteristic++) {
      hInstance->outsideCharacteristics[characteristic] = DRC_CHAR_UNDEFINED;
    }
  }

  return retValueDrc;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_New(
    XHEAACENCLIB_HANDLE_DRCENCODER *phInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_HANDLE_DRCENCODER hInstance;

  *phInstance = (XHEAACENCLIB_HANDLE_DRCENCODER)iisCalloc(sizeof(DRC_IFC_DATA), 1);

  if (*phInstance != NULL) {
    hInstance = *phInstance;
    hInstance->pUniDrcConfig = (UniDrcConfig *)iisCalloc(sizeof(UniDrcConfig), 1);
    hInstance->config_drcGainEnc = (IISDRCGAINENC_CONFIG *)iisCalloc(sizeof(IISDRCGAINENC_CONFIG), 1);
    hInstance->config_drcGainGen = (CONFIG_DRCGAINGEN *)iisCalloc(sizeof(CONFIG_DRCGAINGEN), 1);

  } else {
    retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
  }
  return retValue;
}

DRC_IFC_RETURN iisxHEAACEncLib_drc_ifc_Delete(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;

  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  int err = 0, k, m;

  if (hInstance != NULL) {
    if (hInstance->pUniDrcConfig->uniDrcConfigExtension.downmixInstructionsV1Present == 1) {
      for (k = 0; k < hInstance->pUniDrcConfig->uniDrcConfigExtension.downmixInstructionsV1Count; k++) {
        iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1[k].pDownmixCoefficient);
        hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1[k].pDownmixCoefficient = NULL;

        iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1[k].pLfeChannel);
        hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1[k].pLfeChannel = NULL;
      }
      iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1);
      hInstance->pUniDrcConfig->uniDrcConfigExtension.pDownmixInstructionsV1 = NULL;
    }

    if (hInstance->pUniDrcConfig->uniDrcConfigExtension.drcCoeffsAndInstructionsUniDrcV1Present == 1) {
      for (k = 0; k < hInstance->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count; k++) {
        for (m = 0; m < hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[k].gainSequenceCount; m++) {
          iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[k].pGainSetParams[m].pGainSequenceParams);
          hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[k].pGainSetParams[m].pGainSequenceParams = NULL;
        }
        iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[k].pGainSetParams);
        hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[k].pGainSetParams = NULL;
      }
      iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1);
      hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1 = NULL;

      for (k = 0; k < hInstance->pUniDrcConfig->uniDrcConfigExtension.drcInstructionsUniDrcV1Count; k++) {
        if (hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications) {
          for (m = 0; m < hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].drcChannelGroupCount; m++) {
            iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainOffset);
            hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainOffset = NULL;
            iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainOffsetPresent);
            hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainOffsetPresent = NULL;
            iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pAmplificationScaling);
            hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pAmplificationScaling = NULL;
            iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pAttenuationScaling);
            hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pAttenuationScaling = NULL;
            iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainScalingPresent);
            hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications[m].pGainScalingPresent = NULL;
          }
          iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications);
          hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainModifications = NULL;
        }
        iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pDuckingModifications);
        hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pDuckingModifications = NULL;
        iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainSetIndex);
        hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k].pGainSetIndex = NULL;
      }
      iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1);
      hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1 = NULL;
    }
    iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType);
    hInstance->pUniDrcConfig->uniDrcConfigExtension.pUniDrcConfigExtType = NULL;
    iisFree(hInstance->pUniDrcConfig->uniDrcConfigExtension.pExtBitSize);
    hInstance->pUniDrcConfig->uniDrcConfigExtension.pExtBitSize = NULL;

    iisFree(hInstance->pUniDrcConfig);
    iisFree(hInstance->config_drcGainEnc);
    iisFree(hInstance->config_drcGainGen);

    err = drcGainGeneratorClose(&hInstance->hDRCGainGenStates, &hInstance->hDRCGainGenParams);
    if (err != 0) return DRC_IFC_ERROR_UNKNOWN;

    retValueDrcEnc = iisDRCGainEnc_close(&(hInstance->hDRCGainEncoder));
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) return DRC_IFC_ERROR_UNKNOWN;

    realtimeLRAControlClose(&hInstance->hRtLRAC);

    err = closeDrcBuffers(hInstance);
    if (err != 0) return DRC_IFC_ERROR_UNKNOWN;

    iisFree(hInstance);

  } else {
    retValueDrc = DRC_IFC_ERROR_UNKNOWN;
  }
  return retValueDrc;
}

DRC_IFC_RETURN iisxHEAACEncLib_drc_ifc_init(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    XHEAACENCLIB_DRCENCODER_SETUP setup, LoudnessInfoSet *loudnessInfoSet) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;

  DrcCoefficientsUniDrc *drcCoefficientsUniDrc = NULL;
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  int k = 0, m = 0, gainElementCount = 0, err = 0;
  int compressorLookAheadMs = 0;
  int maxCompressorLookAheadMs = 0;

  if (NULL == hInstance) {
    retValueDrc = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    hInstance->pLoudnessInfoSet = loudnessInfoSet;
    hInstance->nAudioChannels = setup.nAudioChannels;
    hInstance->nFrameLength = setup.nFrameLength;
    hInstance->sampleRate = setup.sampleRate;

    hInstance->uniDrcConfigPresent = 1;
    hInstance->uniDrcLoudnessInfoSetPresent = 1;

    hInstance->bRealtimeLRAC = setup.bRealtimeLRAC;
    hInstance->targetLra = setup.targetLra;
    hInstance->bTargetLraSet = setup.bTargetLraSet;
  }
  if (retValueDrc == DRC_IFC_NO_ERROR) {
    retValueDrc = drcExtGainInit(hInstance, &setup);
  }
  if (retValueDrc == DRC_IFC_NO_ERROR) {
    {
      if (configureEnc1(hInstance->pUniDrcConfig, hInstance->nAudioChannels, &setup) != 0) {
        retValueDrc = DRC_IFC_ERROR_INVALID_SETUP;
      }
    }

    if (retValueDrc == DRC_IFC_NO_ERROR) {
      hInstance->decoderDelay = hInstance->nFrameLength;
      compressorLookAheadMs = 10;
      maxCompressorLookAheadMs = 10;
      hInstance->maxCompressorLookAhead = (maxCompressorLookAheadMs * setup.sampleRate) / 1000;
    }
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    hInstance->config_drcGainEnc->frameSize = hInstance->nFrameLength;
    hInstance->config_drcGainEnc->sampleRate = setup.sampleRate;
    hInstance->config_drcGainEnc->nodeSettingMode = IISDRCGAINENC_NODESETTINGMODE_SINGLENODE;
    hInstance->config_drcGainEnc->pUniDrcConfig = hInstance->pUniDrcConfig;

    {
      hInstance->config_drcGainEnc->pLoudnessInfoSet = hInstance->pLoudnessInfoSet;
    }

    retValueDrcEnc = iisDRCGainEnc_open(&hInstance->hDRCGainEncoder, hInstance->config_drcGainEnc, &hInstance->nSequences);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrc = DRC_IFC_ERROR_GAIN_ENC;
    }
    if (hInstance->nSequences != setup.nCharacteristicCount) {
      retValueDrc = DRC_IFC_ERROR_WRONG_NUMBER_SEQUENCES;
    }
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    retValueDrc = openDrcBuffers(hInstance);
  }

  if (hInstance->pUniDrcConfig->drcCoefficientsUniDrcCount > 0) {
    drcCoefficientsUniDrc = &(hInstance->pUniDrcConfig->pDrcCoefficientsUniDrc[hInstance->pUniDrcConfig->drcCoefficientsUniDrcCount - 1]);
  }
  if (hInstance->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count > 0) {
    drcCoefficientsUniDrc = &(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcCoefficientsUniDrcV1[hInstance->pUniDrcConfig->uniDrcConfigExtension.drcCoefficientsUniDrcV1Count - 1]);
  }
  if (drcCoefficientsUniDrc == NULL)
    retValueDrc = DRC_IFC_ERROR_INVALID_SETUP;

  {
    if (retValueDrc == DRC_IFC_NO_ERROR) {
      gainElementCount = 0;

      hInstance->config_drcGainGen->frameSize = hInstance->nFrameLength;
      hInstance->config_drcGainGen->baseChannelCount = hInstance->nAudioChannels;
      hInstance->config_drcGainGen->sampleRate = setup.sampleRate;
      hInstance->config_drcGainGen->sequenceCount = drcCoefficientsUniDrc->gainSetCount;
      hInstance->config_drcGainGen->maxLookaheadMs = maxCompressorLookAheadMs;

      for (k = 0; k < drcCoefficientsUniDrc->gainSetCount; k++) {
        hInstance->config_drcGainGen->bandCountPerSequence[k] = drcCoefficientsUniDrc->pGainSetParams[k].bandCount;

        for (m = 0; m < drcCoefficientsUniDrc->pGainSetParams[k].bandCount; m++) {
          hInstance->config_drcGainGen->drcCharacteristicIndex[gainElementCount] = drcCoefficientsUniDrc->pGainSetParams[k].pGainSequenceParams[m].drcCharacteristic;
          hInstance->config_drcGainGen->gainOffsetSequence[gainElementCount] = 0;
          hInstance->config_drcGainGen->drcLevelCalculationMode[gainElementCount] = DRC_LEVEL_LEGACY;
          hInstance->config_drcGainGen->lookAhead4SequenceMs[gainElementCount] = compressorLookAheadMs;

          if (hInstance->config_drcGainGen->drcCharacteristicIndex[gainElementCount] == -1) {
            hInstance->config_drcGainGen->numExtParams++;
            hInstance->bCopyGainsBefore = 1;
            if (setup.bRealtimeLRAC) {
              hInstance->config_drcGainGen->applyExternalDrcGains[gainElementCount] = 0;

              hInstance->config_drcGainGen->drcLevelCalculationMode[gainElementCount] = DRC_LEVEL_LEGACY;
            } else {
              hInstance->config_drcGainGen->applyExternalDrcGains[gainElementCount] = 1;

              hInstance->config_drcGainGen->drcLevelCalculationMode[gainElementCount] = DRC_LEVEL_BS1770;
            }
          }
          gainElementCount++;
        }
      }

      hInstance->config_drcGainGen->drcInstructionCount = hInstance->pUniDrcConfig->drcInstructionsUniDrcCount;

      hInstance->config_drcGainGen->drcInstructionCount += hInstance->pUniDrcConfig->uniDrcConfigExtension.drcInstructionsUniDrcV1Count;

      for (k = 0; k < hInstance->config_drcGainGen->drcInstructionCount; k++) {
        DrcInstructionsUniDrc *drcInstructionsUniDrc;
        if (k < hInstance->pUniDrcConfig->drcInstructionsUniDrcCount) {
          drcInstructionsUniDrc = &(hInstance->pUniDrcConfig->pDrcInstructionsUniDrc[k]);
        } else {
          drcInstructionsUniDrc = &(hInstance->pUniDrcConfig->uniDrcConfigExtension.pDrcInstructionsUniDrcV1[k - hInstance->pUniDrcConfig->drcInstructionsUniDrcCount]);
        }

        hInstance->config_drcGainGen->numChannels[k] = drcInstructionsUniDrc->drcChannelCount;
        for (m = 0; m < hInstance->config_drcGainGen->numChannels[k]; m++) {
          hInstance->config_drcGainGen->sequenceIndex[k][m] = drcInstructionsUniDrc->pGainSetIndex[m];
        }
      }

      for (k = 0; k < gainElementCount; k++) {
        hInstance->config_drcGainGen->PRL[k] = setup.loudnessLevel[k];
      }
    }

    if (retValueDrc == DRC_IFC_NO_ERROR) {
      err = drcGainGeneratorOpen(&(hInstance->hDRCGainGenStates),
                                 &(hInstance->hDRCGainGenParams),
                                 hInstance->config_drcGainGen, NULL);
      if (err != 0) {
        retValueDrc = DRC_IFC_ERROR_GAIN_GEN;
      }
    }

    if (retValueDrc == DRC_IFC_NO_ERROR) {
      gainElementCount = 0;
      for (k = 0; k < drcCoefficientsUniDrc->gainSetCount; k++) {
        for (m = 0; m < drcCoefficientsUniDrc->pGainSetParams[k].bandCount; m++) {
          if (hInstance->config_drcGainGen->drcCharacteristicIndex[gainElementCount] == -1) {
            if (setup.bRealtimeLRAC) {
              CONFIG_RT_LRAC configRtLRAC;
              HANDLE_DRCGAINGEN_PARAMS *drcGainGenParams = NULL;
              RLC_ERROR rtLRACError = RLC_OK;

              retValueDrc = iisxHEAACEncLib_GetDrcGainGenParams(hInstance, &drcGainGenParams);
              if (retValueDrc == DRC_IFC_NO_ERROR) {
                configRtLRAC = realtimeLRAControlGetDefaultConfig();
                configRtLRAC.inputLRAassumption = realtimeLRAControlGetInputLraAssumption(setup.loudnessLevel[gainElementCount], setup.bIsLevelerActive);
                configRtLRAC.frameSize = setup.nFrameLength;
                configRtLRAC.sampleRate = setup.sampleRate;
                configRtLRAC.drcGainGenParams = *drcGainGenParams;
                if (setup.bTargetLraSet) {
                  configRtLRAC.targetLRA = setup.targetLra;
                }
                rtLRACError = realtimeLRAControlOpen(&hInstance->hRtLRAC, &configRtLRAC);
                if (rtLRACError != RLC_OK) {
                  retValueDrc = DRC_IFC_ERROR_RTLRAC;
                }
              }
            } else {
              float fastAttack = 1.f;
              float fastDecay = 2.f;
              float slowAttack = 10.f;
              float slowDecay = 20.f;
              float holdOff = 0.f;
              float attackThr = 10.f;
              float decayThr = 10.f;

              err = drcGainGeneratorSetExtParams(hInstance->hDRCGainGenParams, setup.inLevel, setup.outGain, setup.nNodes, fastAttack, fastDecay, slowAttack, slowDecay, holdOff, attackThr, decayThr, hInstance->config_drcGainGen->parametricDrcId[gainElementCount]);
              if (err != 0) {
                retValueDrc = DRC_IFC_ERROR_GAIN_ENC;
              }
            }
          }
          gainElementCount++;
        }
      }
    }

    if (retValueDrc == DRC_IFC_NO_ERROR) {
      err = drcGainGeneratorInit(hInstance->hDRCGainGenStates,
                                 hInstance->hDRCGainGenParams,
                                 hInstance->config_drcGainGen);
      if (err != 0) {
        retValueDrc = DRC_IFC_ERROR_GAIN_GEN;
      }
    }
  }

  return retValueDrc;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_apply(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int const nChannels,
    float const *const pSamples,
    int const nSamples,
    unsigned char **pUniDrcGainBs,
    int *iUniDrcGainLength,
    int bStartPreroll, AUD_OBJ_TYP aot) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;

  int bitCount = 0;
  int offset = 0, sample = 0, ch = 0, err = 0;

  if (NULL == hInstance || NULL == pUniDrcGainBs || NULL == iUniDrcGainLength) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    printErrorConsole(CDI, "Invalid handle");
  } else if (NULL == pSamples) {
    {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
      printErrorConsole(CDI, "Invalid handle");
    }
  }

  if (!isError(retValue)) {
    {
      offset = 0;
      for (sample = 0; sample < nSamples + hInstance->maxCompressorLookAhead; sample++) {
        for (ch = 0; ch < nChannels; ch++) {
          hInstance->pFrameBuffer[ch][sample] = pSamples[offset++];
        }
      }

      if (!isError(retValue)) {
        if (hInstance->bCopyGainsBefore == 1 && hInstance->lraControlDrcGainData.lraControlDrcGainProcessingIsActive) {
          retValue = drcExtGainOverwrite(hInstance, hInstance->lraControlDrcGainData);
        }
      }

      if (!isError(retValue)) {
        if (hInstance->bRealtimeLRAC) {
          RLC_ERROR rtLRACError = RLC_OK;

          rtLRACError = realtimeLRAControlProcess(hInstance->hRtLRAC,
                                                  hInstance->lraInputSignal,
                                                  hInstance->lraCountInputSignal,
                                                  NULL,
                                                  NULL);
          if (rtLRACError != RLC_OK) {
            retValue = XHEAACENCLIB_RETURN_ERROR_DRC;
          }
        }
      }

      if (!isError(retValue)) {
        err = drcGainGeneratorProcess(hInstance->pFrameBuffer, hInstance->gainBuffer, 0, hInstance->hDRCGainGenStates, hInstance->hDRCGainGenParams);
        if (err != 0) {
          retValue = XHEAACENCLIB_RETURN_ERROR_DRC_UNSUPPORTED_CHARACTERISTICS;
        }
      }

      if (!isError(retValue)) {
        if (hInstance->bCopyGainsBefore != 1 && hInstance->lraControlDrcGainData.lraControlDrcGainProcessingIsActive) {
          retValue = drcExtGainOverwrite(hInstance, hInstance->lraControlDrcGainData);
        }
      }
    }

    if (aot == AUD_OBJ_TYP_USAC && !isError(retValue)) {
      retValueDrcEnc = iisDRCGainEnc_writeUniDrcGain(hInstance->hDRCGainEncoder, (const float *const *)hInstance->gainBuffer, bStartPreroll, hInstance->pUniDrcGainBs, &bitCount);
      if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
      }
    } else {
      retValueDrcEnc = iisDRCGainEnc_writeUniDrc(hInstance->hDRCGainEncoder, (const float *const *)hInstance->gainBuffer, bStartPreroll, hInstance->uniDrcLoudnessInfoSetPresent, hInstance->uniDrcConfigPresent, hInstance->pUniDrcGainBs, &bitCount);
      if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
      }
      if (!isError(retValue)) {
        hInstance->uniDrcLoudnessInfoSetPresent = 0;
        hInstance->uniDrcConfigPresent = 0;
      }
    }
    if (!isError(retValue)) {
      hInstance->iUniDrcGainLength = bitCount;
      assert(hInstance->iUniDrcGainLength < MAX_DRC_ENC_PAYLOAD_BITS);

      *pUniDrcGainBs = hInstance->pUniDrcGainBs;
      *iUniDrcGainLength = hInstance->iUniDrcGainLength;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_GetUniDrcConfig(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    unsigned char **pUniDrcConfigBs,
    int *iUniDrcConfigLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;
  int bitCount = 0;

  if (NULL == hInstance || NULL == pUniDrcConfigBs || NULL == iUniDrcConfigLength) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValueDrcEnc = iisDRCGainEnc_writeUniDrcConfig(hInstance->hDRCGainEncoder, IISDRCGAINENC_SYNTAXMODE_DEFAULT, hInstance->pUniDrcConfigBs, &bitCount);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_DRC_INVALID_INSTRUCTIONS;
    }
    if (!isError(retValue)) {
      hInstance->iUniDrcConfigLength = ((bitCount & 0x7) == 0) ? (bitCount >> 3) : (bitCount >> 3) + 1;

      assert(hInstance->iUniDrcConfigLength < MAX_DRC_ENC_PAYLOAD_BYTES);

      *pUniDrcConfigBs = hInstance->pUniDrcConfigBs;
      *iUniDrcConfigLength = hInstance->iUniDrcConfigLength;
    }
  }

  return retValue;
}

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_decoder_delay(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nDecoderDelay) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;

  if (hInstance == NULL) {
    retValueDrc = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    assert(hInstance->decoderDelay > 0);
    *nDecoderDelay = hInstance->decoderDelay;
  }

  return retValueDrc;
}

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_encoder_delay(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nEncoderDelay) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;

  IISDRCGAINENC_RETURN retValueDrcEnc = IISDRCGAINENC_RETURN_NOERROR;

  if (hInstance == NULL) {
    retValueDrc = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    retValueDrcEnc = iisDRCGainEnc_getDrcGainEncoderDelay(hInstance->hDRCGainEncoder, nEncoderDelay);
    if (retValueDrcEnc != IISDRCGAINENC_RETURN_NOERROR) {
      retValueDrc = DRC_IFC_ERROR_UNKNOWN;
    }
  }

  return retValueDrc;
}

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_compressor_lookAhead(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nLookAhead) {
  DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;

  if (hInstance == NULL) {
    retValueDrc = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueDrc == DRC_IFC_NO_ERROR) {
    assert(hInstance->maxCompressorLookAhead > 0);
    *nLookAhead = hInstance->maxCompressorLookAhead;
  }

  return retValueDrc;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_set_lra_control_drc_gains(
    XHEAACENCLIB_HANDLE_DRCENCODER const hInstance,
    float *lraControlDrcGains,
    unsigned int const lraControlDrcGainsLength,
    unsigned int const lraControlDrcGainProcessingIsActive) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if ((NULL == hInstance) || (NULL == lraControlDrcGains)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hInstance->lraControlDrcGainData.lraControlDrcGains = lraControlDrcGains;
    hInstance->lraControlDrcGainData.lraControlDrcGainsLength = lraControlDrcGainsLength;
    hInstance->lraControlDrcGainData.lraControlDrcGainProcessingIsActive = lraControlDrcGainProcessingIsActive;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_set_loudness_range(
    XHEAACENCLIB_HANDLE_DRCENCODER const hInstance,
    float const loudnessRange,
    unsigned int const loudnessRangeCount) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == hInstance) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hInstance->lraInputSignal = loudnessRange;
    hInstance->lraCountInputSignal = loudnessRangeCount;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_setupConfig(
    XHEAACENCLIB_DRCENCODER_SETUP *const hDrcSetup,
    XHEAACENCLIB_DRCMODE const drcMode,
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA const *const drcExternalNodes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int k;

  if (hDrcSetup == NULL || drcExternalNodes == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (k = 0; k < INPUT_DRCINSTRUCTIONSUNIDRCV1_COUNT_MAX; k++) {
      hDrcSetup->instructionSetup[k].amplificationScaling = 1.f;
      hDrcSetup->instructionSetup[k].attenuationScaling = 1.f;
      hDrcSetup->instructionSetup[k].gainOffset = 0.0f;
    }

    for (k = 0; k < INPUT_GAINSET_COUNT_MAX; k++) {
      hDrcSetup->coefficientsSetup[0].drcCharacteristics[k] = -1;
      hDrcSetup->coefficientsSetup[0].gainCodingProfile[k] = IISDRCGAINENC_GAINCODINGPROFILE_REGULAR;
    }

    switch (drcMode) {
      case XHEAACENCLIB_DRCMODE_LN_NE_LI_GE_LRACONTROL:

        hDrcSetup->coefficientsSetup[0].drcCharacteristics[0] = DRC_UNDEFINED;
        hDrcSetup->instructionSetup[0].drcSetEffect = IISDRCGAINENC_DRCSETEFFECT_BIT_NOISY;
        hDrcSetup->instructionSetup[1].drcSetEffect = IISDRCGAINENC_DRCSETEFFECT_BIT_NIGHT | IISDRCGAINENC_DRCSETEFFECT_BIT_LIMITED | IISDRCGAINENC_DRCSETEFFECT_BIT_DIALOG;
        hDrcSetup->instructionSetup[2].drcSetEffect = IISDRCGAINENC_DRCSETEFFECT_BIT_GENERAL_COMPR;
        hDrcSetup->instructionSetup[3].drcSetEffect = IISDRCGAINENC_DRCSETEFFECT_BIT_GENERAL_COMPR;

        hDrcSetup->nCharacteristicCount = 1;
        hDrcSetup->instructionSetup[0].gainSetIndex = 0;
        hDrcSetup->instructionSetup[0].TargetLoudnessValueUpper = 0;
        hDrcSetup->instructionSetup[0].TargetLoudnessValueLower = 0;
        hDrcSetup->instructionSetup[1].gainSetIndex = 0;
        hDrcSetup->instructionSetup[1].TargetLoudnessValueUpper = 0;
        hDrcSetup->instructionSetup[1].TargetLoudnessValueLower = 0;
        hDrcSetup->instructionSetup[1].amplificationScaling = 0.625f;
        hDrcSetup->instructionSetup[2].gainSetIndex = 0;
        hDrcSetup->instructionSetup[2].TargetLoudnessValueUpper = 0;
        hDrcSetup->instructionSetup[2].TargetLoudnessValueLower = -27;
        hDrcSetup->instructionSetup[2].amplificationScaling = 0.375f;
        hDrcSetup->instructionSetup[2].attenuationScaling = 0.5f;
        hDrcSetup->instructionSetup[3].gainSetIndex = 0;
        hDrcSetup->instructionSetup[3].TargetLoudnessValueUpper = -27;
        hDrcSetup->instructionSetup[3].TargetLoudnessValueLower = 0;
        hDrcSetup->instructionSetup[3].amplificationScaling = 0.0f;
        hDrcSetup->instructionSetup[3].attenuationScaling = 0.0f;

        hDrcSetup->nInstructionCount = 4;

        if (drcExternalNodes->drcExternalNodeNumNodes > 0 || hDrcSetup->bRealtimeLRAC) {
          hDrcSetup->coefficientsSetup[0].drcCharacteristics[0] = -1;

          hDrcSetup->nNodes = drcExternalNodes->drcExternalNodeNumNodes;
          for (k = 0; k < drcExternalNodes->drcExternalNodeNumNodes; k++) {
            hDrcSetup->inLevel[k] = drcExternalNodes->drcExternalNodeLevels[k];
            hDrcSetup->outGain[k] = drcExternalNodes->drcExternalNodeGains[k];
          }
        }
        break;
      case XHEAACENCLIB_DRCMODE_OFF:
        retValue = XHEAACENCLIB_RETURN_ERROR_DRC_IFC_INVALID_SETUP;
        break;
      case XHEAACENCLIB_DRCMODE_INVALID:
      default:
        retValue = XHEAACENCLIB_RETURN_ERROR_DRC_IFC_INVALID_SETUP;
        break;
    }

    for (k = 0; k < XHEAACENCLIB_MAX_DRC_SEQUENCES; k++) {
      hDrcSetup->instructionSetup[k].gainOffset = drcExternalNodes->drcGainOffset[k];
    }
  }
  return retValue;
}

static DRC_IFC_RETURN iisxHEAACEncLib_GetDrcGainGenParams(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    HANDLE_DRCGAINGEN_PARAMS **phDrcGainGenParams) {
  DRC_IFC_RETURN retCode = DRC_IFC_NO_ERROR;

  if (hInstance == NULL) {
    retCode = DRC_IFC_ERROR_INVALID_HANDLE;
  }

  if (retCode == DRC_IFC_NO_ERROR) {
    *phDrcGainGenParams = &hInstance->hDRCGainGenParams;
  }

  return retCode;
}
