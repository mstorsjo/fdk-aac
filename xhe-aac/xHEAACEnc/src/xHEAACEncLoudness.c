
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
#include <limits.h>
#include <math.h>
#include <string.h>

#include "iisutillib.h"
#include "mathlib.h"
#include "xHEAACEnc.h"
#include "xHEAACEncLoudness.h"
#include "iisDRCOfflineGain_api.h"

#define XHEAACENC_LOUDNESS_ENV_DEFAULT_NUM_BLOCKS 12000
#define XHEAACENC_LOUDNESS_ENV_INCREMENT_NUM_BLOCKS 108000
#define XHEAACENC_LOUDNESS_ENV_OVERHEAD_NUM_BLOCKS 6000
#define XHEAACENC_LOUDNESS_ENV_MIN_NUM_BLOCKS 4

#if (XHEAACENC_MAX_DRC_CHAR_NODES != DOG_MAX_DRC_CHAR_NODES)
#error "The definition of XHEAACENC_MAX_DRC_CHAR_NODES doesn't match the definition of DOG_MAX_DRC_CHAR_NODES"
#endif

#ifndef PI
#define PI 3.1415926535897931f
#endif

static void lpFilter_init(LP_FILTER_HANDLE lp_filter);
static void lpFilter_put(LP_FILTER_HANDLE lp_filter, float input);
static float lpFilter_get(LP_FILTER_HANDLE lp_filter, float const *const filter_taps);
static void genFIRLowPass(float *filterCoeffs, float omegaC);
static void windowFilterCoeffs(float *filterCoeffs);
static float kaiserBessel(float x);
static float sinc(float x);

static int isError(
    IIS_XHEAACENC_RETURN_CODE const errorValue);

static IIS_XHEAACENC_RETURN_CODE getNumChannelsFromChannelConfig(
    IIS_XHEAACENC_CHANNELCONFIG const channelConfig,
    int *const nChannels);

static IIS_XHEAACENC_RETURN_CODE getLoudspeakerPositions(
    int const nChannels,
    LOUDMTR_LS_POS *const channelConfigLoudness,
    int *const channelGroupLoudness);

static IIS_XHEAACENC_RETURN_CODE loudnessNew(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE *const phLoudness);

static IIS_XHEAACENC_RETURN_CODE initLoudnessEnvelopeArray(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup);

static IIS_XHEAACENC_RETURN_CODE calculateLoudnessEnvelopeLength(
    int *const instantaneousLoudnessLength,
    int const audioBlockLength,
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup);

static int useDefaultLoudnessEnvelopeLength(
    unsigned int const audioInputLengthAvailable,
    uint64_t const audioInputLengthSamples);

static IIS_XHEAACENC_RETURN_CODE extendLoudnessBuffer(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness);
static IIS_XHEAACENC_RETURN_CODE filterInput(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    float const *const pSamples,
    int const nSamples,
    int const nChannels,
    float const *const filter_taps);
static IIS_XHEAACENC_RETURN_CODE downsampleInput(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    float const *const pSamples,
    int const nSamples,
    int const nChannels);

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Open(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE *const phLoudness,
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;

  int nChannels = 0;

  HLOUDNESS_METER hLoudnessMeter = NULL;
  HLOUDNESS_METER hLoudnessMeter_unmodified = NULL;
  LOUDMTR_TPFILTER tpFilterConfig = LOUDMTR_TPFILTER_IIS;
  LOUDMTR_LS_POS *channelConfigLoudness = NULL;
  int *channelGroupLoudness = NULL;
  int sampleRateForLoudMeasurement = 0;
  float omega = 0;

  if (!phLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (loudnessSetup.sampleRate <= 0) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  if (!isError(retValue)) {
    if (!(*phLoudness)) {
      retValue = loudnessNew(phLoudness);
      (*phLoudness)->isInitialized = 1;
    } else if (1 == (*phLoudness)->isInitialized) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_ALREADY_INITIALIZED;
    }
  }

  if (!isError(retValue)) {
    retValue = getNumChannelsFromChannelConfig(loudnessSetup.channelConfig, &nChannels);
  }

  if (!isError(retValue)) {
    if (loudnessSetup.sampleRate >= 44100 && nChannels <= 2) {
      int i = 0;
      for (i = 0; i < nChannels; i++) {
        lpFilter_init((*phLoudness)->lp_filter[i]);
      }
      (*phLoudness)->lpFilteringEnabled = 1;
    } else {
      (*phLoudness)->lpFilteringEnabled = 0;
    }
  }

  if (!isError(retValue)) {
    if (loudnessSetup.sampleRate >= 88200) {
      (*phLoudness)->downsampleEnabled = 1;
      sampleRateForLoudMeasurement = loudnessSetup.sampleRate / 2;
    } else {
      sampleRateForLoudMeasurement = loudnessSetup.sampleRate;
    }
    (*phLoudness)->audioBlockLength = (loudnessSetup.sampleRate * XHEAACENC_LOUDNESS_ENV_BLOCK_LENGTH_MS) / 1000;
  }

  if (!isError(retValue) && (*phLoudness)->lpFilteringEnabled) {
    omega = (float)LOWPASS_CUTOFF / ((float)loudnessSetup.sampleRate / 2);
    genFIRLowPass((*phLoudness)->filter_taps, omega);
  }

  if (!isError(retValue)) {
    retValue = initLoudnessEnvelopeArray(*phLoudness, loudnessSetup);
  }

  if (!isError(retValue)) {
    (*phLoudness)->usesDefaultLoudnessEnvelopeLength = useDefaultLoudnessEnvelopeLength(loudnessSetup.audioInputLengthAvailable, loudnessSetup.audioInputLengthSamples);
  }

  if (!isError(retValue)) {
    channelConfigLoudness = (LOUDMTR_LS_POS *)calloc(nChannels, sizeof(LOUDMTR_LS_POS));
    channelGroupLoudness = (int *)calloc(nChannels, sizeof(int));

    retValue = getLoudspeakerPositions(nChannels, channelConfigLoudness, channelGroupLoudness);
  }

  if (!isError(retValue)) {
    errorLoudness = LoudMeter_Create(&hLoudnessMeter);
    if (errorLoudness == ERRLOUD_Success && (*phLoudness)->lpFilteringEnabled) {
      errorLoudness = LoudMeter_Create(&hLoudnessMeter_unmodified);
    }
    if (errorLoudness != ERRLOUD_Success) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
    }
  }

  if (!isError(retValue)) {
    errorLoudness = LoudMeter_Config(hLoudnessMeter, channelConfigLoudness, channelGroupLoudness, nChannels, sampleRateForLoudMeasurement, tpFilterConfig);
    if (errorLoudness == ERRLOUD_Success && (*phLoudness)->lpFilteringEnabled) {
      errorLoudness = LoudMeter_Config(hLoudnessMeter_unmodified, channelConfigLoudness, channelGroupLoudness, nChannels, loudnessSetup.sampleRate, tpFilterConfig);
    }
    if (errorLoudness != ERRLOUD_Success) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
    }
  }

  if (!isError(retValue)) {
    errorLoudness = LoudMeter_SetWorkloadConfig(hLoudnessMeter, LOUDMTR_WORKLOAD_DISABLE_TP);
    if (errorLoudness == ERRLOUD_Success && (*phLoudness)->lpFilteringEnabled) {
      errorLoudness = LoudMeter_SetWorkloadConfig(hLoudnessMeter_unmodified, LOUDMTR_WORKLOAD_DISABLE_TP);
    }
    if (errorLoudness != ERRLOUD_Success) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
    }
  }

  if (!isError(retValue)) {
    (*phLoudness)->hLoudnessMeter = hLoudnessMeter;
    if ((*phLoudness)->lpFilteringEnabled) {
      (*phLoudness)->hLoudnessMeter_unmodified = hLoudnessMeter_unmodified;
    }
    (*phLoudness)->nChannels = nChannels;
    (*phLoudness)->hLoudnessData->minRequiredSamplesProcessed = 0;
    (*phLoudness)->sampleRate = sampleRateForLoudMeasurement;
  }
  if (!isError(retValue)) {
    if ((*phLoudness)->lpFilteringEnabled) {
      (*phLoudness)->lp_samples = (float *)iisMalloc(sizeof(float) * (*phLoudness)->audioBlockLength * (*phLoudness)->nChannels);
    }
  }

  if (channelConfigLoudness) {
    free(channelConfigLoudness);
    channelConfigLoudness = NULL;
  }
  if (channelGroupLoudness) {
    free(channelGroupLoudness);
    channelGroupLoudness = NULL;
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Measure(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    float const *const pSamples,
    int const nSamples) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  int nEffectiveSamples = 0;

  if (!hLoudness || !pSamples) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hLoudness->nChannels <= 0) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  } else if (nSamples % hLoudness->nChannels) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  } else if (hLoudness->hLoudnessData->audioBlocksProcessed >= hLoudness->hLoudnessData->instantaneousLoudnessLength && nSamples > 0) {
    if (!hLoudness->usesDefaultLoudnessEnvelopeLength) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT_TOO_MANY_INPUT_SAMPLES;
    }
  } else if (hLoudness->measureAnchorLoudness == 1 && hLoudness->hLoudnessData->vaBuffer == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int nBlockStart = 0;
    int nBlockLength;

    while (nBlockStart < nSamples) {
      nBlockLength = min((hLoudness->audioBlockLength * hLoudness->nChannels - hLoudness->hLoudnessData->nSamplesInLoudnessMeter), (nSamples - nBlockStart));

      assert(nBlockLength <= hLoudness->audioBlockLength * hLoudness->nChannels);

      if (hLoudness->hLoudnessData->audioBlocksProcessed >= hLoudness->hLoudnessData->instantaneousLoudnessLength) {
        if (hLoudness->usesDefaultLoudnessEnvelopeLength) {
          retValue = extendLoudnessBuffer(hLoudness);
        } else {
          retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT_TOO_MANY_INPUT_SAMPLES;
        }
      }
      if (!isError(retValue) && hLoudness->downsampleEnabled) {
        if (!isError(retValue)) {
          retValue = filterInput(hLoudness, &pSamples[nBlockStart], nBlockLength, hLoudness->nChannels, hLoudness->filter_taps);
        }

        retValue = downsampleInput(hLoudness, hLoudness->lp_samples, nBlockLength, hLoudness->nChannels);

        nEffectiveSamples = nBlockLength / 2;
      }

      if (!isError(retValue) && !hLoudness->downsampleEnabled && hLoudness->lpFilteringEnabled) {
        retValue = filterInput(hLoudness, &pSamples[nBlockStart], nBlockLength, hLoudness->nChannels, hLoudness->filter_taps);
        nEffectiveSamples = nBlockLength;
      }

      if (!isError(retValue)) {
        LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;
        if (hLoudness->downsampleEnabled || hLoudness->lpFilteringEnabled) {
          errorLoudness = LoudMeter_Feed(hLoudness->hLoudnessMeter,
                                         hLoudness->lp_samples,
                                         nEffectiveSamples / hLoudness->nChannels);

          if (errorLoudness == ERRLOUD_Success && hLoudness->lpFilteringEnabled) {
            errorLoudness = LoudMeter_Feed(hLoudness->hLoudnessMeter_unmodified,
                                           &pSamples[nBlockStart],
                                           nBlockLength / hLoudness->nChannels);
          }
        } else {
          errorLoudness = LoudMeter_Feed(hLoudness->hLoudnessMeter,
                                         &pSamples[nBlockStart],
                                         nBlockLength / hLoudness->nChannels);
        }

        if (ERRLOUD_Success != errorLoudness) {
          retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
        } else {
          hLoudness->hLoudnessData->nSamplesInLoudnessMeter += nBlockLength;
          nBlockStart += nBlockLength;
        }
      }

      if (!isError(retValue)) {
        if (hLoudness->hLoudnessData->nSamplesInLoudnessMeter == hLoudness->audioBlockLength * hLoudness->nChannels) {
          assert(hLoudness->hLoudnessData->instantaneousLoudness != NULL);
          hLoudness->hLoudnessData->instantaneousLoudness[hLoudness->hLoudnessData->audioBlocksProcessed] = LoudMeter_GetInstantaneousLoudness(hLoudness->hLoudnessMeter);

          hLoudness->hLoudnessData->nSamplesInLoudnessMeter = 0;
          hLoudness->hLoudnessData->audioBlocksProcessed++;

          if (hLoudness->hLoudnessData->minRequiredSamplesProcessed == 0 &&
              hLoudness->hLoudnessData->audioBlocksProcessed >= XHEAACENC_LOUDNESS_ENV_MIN_NUM_BLOCKS) {
            hLoudness->hLoudnessData->minRequiredSamplesProcessed = 1;
          }
        }
      }

      if (isError(retValue)) {
        break;
      }
    }
  }

  if (!isError(retValue)) {
    hLoudness->loudness = LoudMeter_GetGatedLongTermLoudness(hLoudness->hLoudnessMeter);

    if (hLoudness->downsampleEnabled || hLoudness->lpFilteringEnabled) {
      hLoudness->samplePeak = LoudMeter_GetMaxSamplePeak(hLoudness->hLoudnessMeter_unmodified, 0);
    } else {
      hLoudness->samplePeak = LoudMeter_GetMaxSamplePeak(hLoudness->hLoudnessMeter, 0);
    }
    hLoudness->loudnessRange = LoudMeter_GetLoudnessRange(hLoudness->hLoudnessMeter);
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_LraControlDrcGainDataCreate(
    IIS_XHEAACENC_LOUDNESS_DATA *const pLoudnessData,
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA **const phLraControlDrcGainData) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!pLoudnessData) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!(pLoudnessData->minRequiredSamplesProcessed)) {
    retValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS_SAMPLES_TOO_FEW;
  }

  if (!isError(retValue)) {
    if (pLoudnessData->instantaneousLoudnessLength > 0) {
      if (!(*phLraControlDrcGainData)) {
        *phLraControlDrcGainData = (IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA *)iisCalloc(1, sizeof(IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA));
      }

      if (*phLraControlDrcGainData) {
        (*phLraControlDrcGainData)->lraControlDrcGainsLength = (unsigned int)pLoudnessData->audioBlocksProcessed;

        if (pLoudnessData->nSamplesInLoudnessMeter > 0) {
          (*phLraControlDrcGainData)->lraControlDrcGainsLength++;
        }

        (*phLraControlDrcGainData)->lraControlDrcGains = (float *)iisCalloc((*phLraControlDrcGainData)->lraControlDrcGainsLength, sizeof(float));

        if ((*phLraControlDrcGainData)->lraControlDrcGains != NULL) {
          memcpy((*phLraControlDrcGainData)->lraControlDrcGains, pLoudnessData->instantaneousLoudness, (*phLraControlDrcGainData)->lraControlDrcGainsLength * sizeof(float));

          if (pLoudnessData->nSamplesInLoudnessMeter > 0) {
            assert((*phLraControlDrcGainData)->lraControlDrcGainsLength >= 2);

            (*phLraControlDrcGainData)->lraControlDrcGains[(*phLraControlDrcGainData)->lraControlDrcGainsLength - 1] =
                (*phLraControlDrcGainData)->lraControlDrcGains[(*phLraControlDrcGainData)->lraControlDrcGainsLength - 2];
          }

        } else {
          retValue = IIS_XHEAACENC_ERROR_MEMORY;
        }

      } else {
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
      }
    }
  }
  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_ConvertToDrcGains(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA *const hLraControlDrcGainData,
    float const loudnessLevel,
    IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE const loudnessLevelType,
    unsigned char const *const vaBuffer,
    float const targetLRA,
    PARAMLIST_DRCMODE const drcMode) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  DOG_DRC_CHARACTERISTIC_NODES dogCharacteristicNodes = {0};

  if (hLraControlDrcGainData == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (
      drcMode != PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  } else if (targetLRA < 6.0f || targetLRA > 16.0f) {
    retValue = IIS_XHEAACENC_ERROR_PARAM_TARGET_LOUDNESS_RANGE;
  }

  if (!isError(retValue)) {
    DOG_ERROR dogError = DOG_OK;
    DOG_CONFIG dogConfig = getDefaultDogConfig();
    dogConfig.targetLRA = targetLRA;
    dogConfig.inputLoudnessLevelAvailable = 1;
    dogConfig.inputLoudnessLevel = loudnessLevel;
    dogConfig.alignDialogLoudness = DOG_ALIGN_PROGRAM_LOUDNESS;

    if (loudnessLevelType == IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_EXTERNAL_SET) {
      dogConfig.alignDialogLoudness = DOG_ALIGN_INPUT_LOUDNESS;
    } else if (loudnessLevelType == IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_INTERNAL_MEASURED) {
      dogConfig.alignDialogLoudness = DOG_ALIGN_ANCHOR_LOUDNESS;
    }
    if (loudnessLevelType == IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_INTERNAL_MEASURED && vaBuffer == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    } else if (loudnessLevelType == IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_PRL && vaBuffer != NULL) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS;
    }

    if (!isError(retValue)) {
      dogError = drcOfflineGain_process_va(hLraControlDrcGainData->lraControlDrcGainsLength,
                                           &dogConfig,
                                           vaBuffer,
                                           &dogCharacteristicNodes,
                                           hLraControlDrcGainData->lraControlDrcGains);
      if (dogError != DOG_OK) {
        retValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS;
      }
    }
  }

  if (!isError(retValue)) {
    hLraControlDrcGainData->drcExternalNodeNumNodes = dogCharacteristicNodes.nodeCount;

    if ((sizeof(hLraControlDrcGainData->drcExternalNodeLevels) / sizeof(int) < hLraControlDrcGainData->drcExternalNodeNumNodes) ||
        (sizeof(hLraControlDrcGainData->drcExternalNodeGains) / sizeof(int) < hLraControlDrcGainData->drcExternalNodeNumNodes) ||
        (sizeof(dogCharacteristicNodes.nodeLevel) / sizeof(float) < hLraControlDrcGainData->drcExternalNodeNumNodes) ||
        (sizeof(dogCharacteristicNodes.nodeGain) / sizeof(float) < hLraControlDrcGainData->drcExternalNodeNumNodes)) {
      retValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS;
    } else {
      assert(hLraControlDrcGainData->drcExternalNodeNumNodes <= XHEAACENC_MAX_DRC_CHAR_NODES);
    }
  }

  if (!isError(retValue)) {
    unsigned int node;
    for (node = 0; node < hLraControlDrcGainData->drcExternalNodeNumNodes; node++) {
      hLraControlDrcGainData->drcExternalNodeLevels[node] = (int)floor((double)dogCharacteristicNodes.nodeLevel[node] + 0.5);

      if ((node > 0) && (hLraControlDrcGainData->drcExternalNodeLevels[node] <= hLraControlDrcGainData->drcExternalNodeLevels[node - 1])) {
        hLraControlDrcGainData->drcExternalNodeLevels[node] = hLraControlDrcGainData->drcExternalNodeLevels[node - 1] + 1;
      }

      hLraControlDrcGainData->drcExternalNodeGains[node] = (int)floor((double)dogCharacteristicNodes.nodeGain[node] + 0.5);
    }

    if (drcMode == PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL) {
      hLraControlDrcGainData->drcGainOffset[0] = dogCharacteristicNodes.gainOffset[0];
      hLraControlDrcGainData->drcGainOffset[1] = dogCharacteristicNodes.gainOffset[1];
      hLraControlDrcGainData->drcGainOffset[2] = dogCharacteristicNodes.gainOffset[2];
    } else {
      assert(0);
    }
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Delete(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE hLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hLoudness != NULL) {
    LoudMeter_Destroy(&hLoudness->hLoudnessMeter);
    if (hLoudness->hLoudnessMeter_unmodified) {
      LoudMeter_Destroy(&hLoudness->hLoudnessMeter_unmodified);
    }
    if (hLoudness->hLoudnessData != NULL) {
      if (hLoudness->hLoudnessData != NULL) {
        if (hLoudness->hLoudnessData->instantaneousLoudness != NULL)
          iisFree(hLoudness->hLoudnessData->instantaneousLoudness);
        if (hLoudness->hLoudnessData->vaBuffer != NULL)
          iisFree(hLoudness->hLoudnessData->vaBuffer);
      }
      iisFree(hLoudness->hLoudnessData);
      hLoudness->hLoudnessData = NULL;
    }
    if (hLoudness->lp_samples) {
      iisFree(hLoudness->lp_samples);
      hLoudness->lp_samples = NULL;
    }
    for (int i = 0; i < MAX_CHANNELS_LOUDNESS; i++) {
      if (hLoudness->lp_filter[i]) {
        iisFree(hLoudness->lp_filter[i]);
        hLoudness->lp_filter[i] = NULL;
      }
    }
    if (hLoudness->filter_taps) {
      iisFree(hLoudness->filter_taps);
      hLoudness->filter_taps = NULL;
    }

    iisFree(hLoudness);
    hLoudness = NULL;
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_LraControlDrcGainDataDelete(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA *pLraControlDrcGainsData) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (pLraControlDrcGainsData != NULL) {
    if (pLraControlDrcGainsData->lraControlDrcGains) {
      iisFree(pLraControlDrcGainsData->lraControlDrcGains);
      pLraControlDrcGainsData->lraControlDrcGains = NULL;
    }

    iisFree(pLraControlDrcGainsData);
    pLraControlDrcGainsData = NULL;
  }

  return retValue;
}

static int isError(
    IIS_XHEAACENC_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue >= IIS_XHEAACENC_ERROR_FIRST) {
    retValue = 1;
  }
  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getNumChannelsFromChannelConfig(
    IIS_XHEAACENC_CHANNELCONFIG const channelConfig,
    int *const nChannels) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!nChannels) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    switch (channelConfig) {
      case IIS_XHEAACENC_CHANNELCONFIG_MONO:
        *nChannels = 1;
        break;

      case IIS_XHEAACENC_CHANNELCONFIG_STEREO:
        *nChannels = 2;
        break;
      default:
        retValue = IIS_XHEAACENC_ERROR_PARAM_CHANNELCONFIG;
        break;
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getLoudspeakerPositions(
    int const nChannels,
    LOUDMTR_LS_POS *const chConfigLoudness,
    int *const chGroupLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  int idx_ch;

  if (!chConfigLoudness || !chGroupLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (idx_ch = 0; idx_ch < nChannels; idx_ch++) {
      if (idx_ch > 2) {
        if (idx_ch == 3) {
          chConfigLoudness[idx_ch] = LS_POS_LFE;
        } else if (idx_ch == 4 || idx_ch == 5) {
          chConfigLoudness[idx_ch] = LS_POS_M_SIDE;
        } else if (idx_ch == 6 || idx_ch == 7) {
          chConfigLoudness[idx_ch] = LS_POS_M_BACK;
        }
      } else {
        chConfigLoudness[idx_ch] = LS_POS_M_FRONT;
      }
      chGroupLoudness[idx_ch] = 1;
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE loudnessNew(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE *const phLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  *phLoudness = iisCalloc(1, sizeof(IIS_XHEAACENC_LOUDNESS_INSTANCE));

  if (!(*phLoudness)) {
    retValue = IIS_XHEAACENC_ERROR_MEMORY;
  }

  if (!isError(retValue)) {
    (*phLoudness)->hLoudnessData = iisCalloc(1, sizeof(IIS_XHEAACENC_LOUDNESS_DATA));
    if (!(*phLoudness)->hLoudnessData) {
      retValue = IIS_XHEAACENC_ERROR_MEMORY;
    }
  }
  if (!isError(retValue)) {
    int i = 0;
    for (i = 0; i < MAX_CHANNELS_LOUDNESS; i++) {
      (*phLoudness)->lp_filter[i] = (LP_FILTER_HANDLE)iisMalloc(sizeof(LP_FILTER));
      if (!(*phLoudness)->lp_filter[i]) {
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
        break;
      }
    }
  }
  (*phLoudness)->filter_taps = (float *)iisMalloc(sizeof(float) * LOWPASS_FILTER_TAP_NUM);
  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE initLoudnessEnvelopeArray(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = calculateLoudnessEnvelopeLength(&hLoudness->hLoudnessData->instantaneousLoudnessLength, hLoudness->audioBlockLength, loudnessSetup);
  }

  if (!isError(retValue)) {
    if (hLoudness->hLoudnessData->instantaneousLoudnessLength > 0) {
      int loudnessEnvOverhead = XHEAACENC_LOUDNESS_ENV_OVERHEAD_NUM_BLOCKS;
      const int num = hLoudness->hLoudnessData->instantaneousLoudnessLength + loudnessEnvOverhead;

      hLoudness->hLoudnessData->instantaneousLoudness = (float *)iisCalloc(num, sizeof(float));
      if (hLoudness->hLoudnessData->instantaneousLoudness == NULL) {
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
      }
    } else {
      hLoudness->hLoudnessData->instantaneousLoudness = NULL;
    }
  }

  if (!isError(retValue)) {
    hLoudness->hLoudnessData->vaBuffer = NULL;
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE calculateLoudnessEnvelopeLength(
    int *const instantaneousLoudnessLength,
    int const audioBlockLength,
    IIS_XHEAACENC_LOUDNESS_SETUP const loudnessSetup) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!instantaneousLoudnessLength) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (audioBlockLength <= 0) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  if (!isError(retValue)) {
    if (useDefaultLoudnessEnvelopeLength(loudnessSetup.audioInputLengthAvailable, loudnessSetup.audioInputLengthSamples)) {
      *instantaneousLoudnessLength = XHEAACENC_LOUDNESS_ENV_DEFAULT_NUM_BLOCKS;
    } else {
      if ((loudnessSetup.audioInputLengthSamples / (uint64_t)audioBlockLength) <= INT_MAX) {
        *instantaneousLoudnessLength = (int)(loudnessSetup.audioInputLengthSamples / (uint64_t)audioBlockLength);
      } else {
        retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT_TOO_MANY_INPUT_SAMPLES;
      }
      if (loudnessSetup.audioInputLengthSamples % audioBlockLength) {
        (*instantaneousLoudnessLength)++;
      }
    }
  }

  if (*instantaneousLoudnessLength <= 0) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  return retValue;
}

static int useDefaultLoudnessEnvelopeLength(
    unsigned int const audioInputLengthAvailable,
    uint64_t const audioInputLengthSamples) {
  int retValue = 0;
  if (!audioInputLengthAvailable || audioInputLengthSamples <= 0) {
    retValue = 1;
  }
  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE extendLoudnessBuffer(IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  int size = 0;
  int loudnessEnvOverhead = 0;

  if (!hLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    loudnessEnvOverhead = XHEAACENC_LOUDNESS_ENV_OVERHEAD_NUM_BLOCKS;
    size = hLoudness->hLoudnessData->instantaneousLoudnessLength + loudnessEnvOverhead;

    hLoudness->hLoudnessData->instantaneousLoudness = (float *)iisRealloc(hLoudness->hLoudnessData->instantaneousLoudness, (size + (int)XHEAACENC_LOUDNESS_ENV_INCREMENT_NUM_BLOCKS) * sizeof(float));
    if (hLoudness->hLoudnessData->instantaneousLoudness == NULL) {
      retValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue) && hLoudness->hLoudnessData->vaBuffer != NULL) {
    hLoudness->hLoudnessData->vaBuffer = (unsigned char *)iisRealloc(hLoudness->hLoudnessData->vaBuffer, (size + (int)XHEAACENC_LOUDNESS_ENV_INCREMENT_NUM_BLOCKS) * sizeof(unsigned char));
    if (hLoudness->hLoudnessData->vaBuffer == NULL) {
      retValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(retValue)) {
    hLoudness->hLoudnessData->instantaneousLoudnessLength += XHEAACENC_LOUDNESS_ENV_INCREMENT_NUM_BLOCKS;
  }

  return retValue;
}
void lpFilter_init(LP_FILTER_HANDLE lp_filter) {
  int i;
  for (i = 0; i < LOWPASS_FILTER_TAP_NUM; ++i)
    lp_filter->history[i] = 0;
  lp_filter->last_index = 0;
}

void lpFilter_put(LP_FILTER_HANDLE lp_filter, float input) {
  lp_filter->history[lp_filter->last_index++] = input;
  if (lp_filter->last_index == LOWPASS_FILTER_TAP_NUM) {
    lp_filter->last_index = 0;
  }
}

float lpFilter_get(LP_FILTER_HANDLE lp_filter, float const *const filter_taps) {
  float acc = 0;
  int index = lp_filter->last_index, i;
  for (i = 0; i < LOWPASS_FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index - 1 : LOWPASS_FILTER_TAP_NUM - 1;
    acc += lp_filter->history[index] * filter_taps[i];
  }
  return acc;
}

static IIS_XHEAACENC_RETURN_CODE filterInput(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    float const *const pSamples,
    int const nSamples,
    int const nChannels,
    float const *const filter_taps) {
  IIS_XHEAACENC_RETURN_CODE retErr = IIS_XHEAACENC_NO_ERROR;
  int i = 0;

  if (hLoudness == NULL || pSamples == NULL) {
    retErr = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (nChannels == 1) {
    for (i = 0; i < nSamples; i++) {
      lpFilter_put(hLoudness->lp_filter[0], pSamples[i]);
      hLoudness->lp_samples[i] = lpFilter_get(hLoudness->lp_filter[0], filter_taps);
    }
  } else if (nChannels == 2) {
    for (i = 0; i < nSamples; i += 2) {
      lpFilter_put(hLoudness->lp_filter[0], pSamples[i]);
      hLoudness->lp_samples[i] = lpFilter_get(hLoudness->lp_filter[0], filter_taps);

      lpFilter_put(hLoudness->lp_filter[1], pSamples[i + 1]);
      hLoudness->lp_samples[i + 1] = lpFilter_get(hLoudness->lp_filter[1], filter_taps);
    }
  } else if (nChannels > 2) {
    assert(0);
    retErr = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  return retErr;
}
static IIS_XHEAACENC_RETURN_CODE downsampleInput(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    float const *const pSamples,
    int const nSamples,
    int const nChannels) {
  IIS_XHEAACENC_RETURN_CODE retErr = IIS_XHEAACENC_NO_ERROR;
  int i = 0;

  if (hLoudness == NULL || pSamples == NULL) {
    retErr = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (nChannels == 1) {
    for (i = 0; i < (nSamples / 2); i++) {
      hLoudness->lp_samples[i] = pSamples[2 * i];
    }
  } else if (nChannels == 2) {
    assert(nSamples % 2 == 0);
    for (i = 0; i < (nSamples / 2); i += 2) {
      hLoudness->lp_samples[i] = pSamples[2 * i];
      hLoudness->lp_samples[i + 1] = pSamples[2 * i + 1];
    }
  } else if (nChannels > 2) {
    assert(0);
    retErr = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  return retErr;
}

static void genFIRLowPass(float *filterCoeffs, float omegaC) {
  int j = 0;
  float arg = 0.0f;

  for (j = 0; j < LOWPASS_FILTER_TAP_NUM; j++) {
    arg = (float)j - (float)(LOWPASS_FILTER_TAP_NUM - 1) / 2.0;
    filterCoeffs[j] = omegaC * sinc(omegaC * arg * PI);
  }

  windowFilterCoeffs(filterCoeffs);
}

static float kaiserBessel(float x) {
  float Sum = 0.0, XtoIpower;
  int i, j, Factorial;
  for (i = 1; i < 10; i++) {
    XtoIpower = pow(x / 2.0, (float)i);
    Factorial = 1;
    for (j = 1; j <= i; j++) Factorial *= j;
    Sum += pow(XtoIpower / (float)Factorial, 2.0);
  }
  return (1.0 + Sum);
}

static float sinc(float x) {
  return (sin(x) / x);
}

static void windowFilterCoeffs(float *filterCoeffs) {
  int j;
  float *winCoeff;
  float arg;

  winCoeff = (float *)iisMalloc(sizeof(float) * (LOWPASS_FILTER_TAP_NUM + 2));

  for (j = 0; j < LOWPASS_FILTER_TAP_NUM; j++) {
    arg = BETA_WIN * sqrt(1.0 - pow(((float)(2 * j + 2) - (LOWPASS_FILTER_TAP_NUM + 1)) / (LOWPASS_FILTER_TAP_NUM + 1), 2.0));
    winCoeff[j] = kaiserBessel(arg) / kaiserBessel(BETA_WIN);
  }

  for (j = 0; j < LOWPASS_FILTER_TAP_NUM; j++) {
    filterCoeffs[j] *= winCoeff[j];
  }

  iisFree(winCoeff);
}
