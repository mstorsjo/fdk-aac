
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
#include <math.h>
#include <string.h>

#include "iisDRCRealtimeLRAControl_api.h"
#include "iisDRCGainGenerator_api.h"
#include "iisutillib.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define RT_LRAC_DEFAULT_TARGET_LRA 10.0f
#define RT_LRAC_DEFAULT_SCALING 1.3f
#define RT_LRAC_DEFAULT_PRL -24.0f
#define RT_LRAC_START_LRA_ASSUMPTION 14.0f

#define RT_LRAC_RAMP_UP_TIME 300.0f
#define RT_LRAC_LRA_SMOOTHING_TIME 30.0f
#define RT_LRAC_MIN_SLOPE 0.2f
#define RT_LRAC_MAX_LRA_REDUCTION 15.0f
#define RT_LRAC_LOOKAHEAD 10
#define RT_LRAC_MAX_LOOKAHEAD 30

#define RT_LRAC_MAX_STATIC_CURVE_NODES 4
#define RT_LRAC_PARAMETRIC_DRC_ID 1

typedef struct T_RT_LRAC {
  HANDLE_DRCGAINGEN_STATES hUniDrcGainGenStates;
  HANDLE_DRCGAINGEN_PARAMS hUniDrcGainGenParams;
  CONFIG_DRCGAINGEN configDrcGainGen;
  int externalDrcGainGen;

  int frameSize;
  int numInputChannels;

  int nodeInputLevel[RT_LRAC_MAX_STATIC_CURVE_NODES];
  int nodeOutputGain[RT_LRAC_MAX_STATIC_CURVE_NODES];
  int nodeCount;

  float fastAttack;
  float fastDecay;
  float slowAttack;
  float slowDecay;
  float holdOff;
  float attackThr;
  float decayThr;
  int parametricDrcId;

  float targetLRA;
  float loudness;
  float inputLRAassumption;
  int adaptToInput;

  float currentLRA, prevLRA;
  float smoothLRAcoef;
  unsigned int rampBlocks;
} RT_LRAC_INSTANCE;

static int
_roundToInt(float floatVal) {
  if (floatVal > 0.0f)
    return (int)(floatVal + 0.5f);
  else
    return (int)(floatVal - 0.5f);
}

static float
_getSmoothingCoef(float timeConstantMs, float sampleRate) {
  return (float)(1.0 - exp((double)(-1.0f / ((timeConstantMs / 1000.0f) * sampleRate))));
}

static float
_getLevelerLRA(const float inputLRA) {
  if (inputLRA >= 19.0f) {
    return inputLRA - 11.53f;
  } else {
    return 0.36f * inputLRA + 0.63f;
  }
}

static float
_getScaling(const float inputLRA, const float targetLRA) {
  float levelerLRA = _getLevelerLRA(inputLRA);
  float ratio = (inputLRA - targetLRA) / (inputLRA - levelerLRA);
  float scaling;

  scaling = (0.675f / 0.8f) * ratio;

  scaling = max(scaling, 0.0f);
  scaling = min(scaling, 1.0f - RT_LRAC_MIN_SLOPE);
  return scaling;
}

static float
_scaledLeveler_getLoudnessCorrection(float scaling, const float LRA) {
  float LC_lowLRA, LC_highLRA;

  scaling = min(scaling, 1.0f);
  scaling = max(scaling, 0.0f);

  LC_lowLRA = -2.0340972f * scaling * scaling + 0.2474f * scaling;
  LC_highLRA = -4.8055289f * scaling * scaling + 3.4920f * scaling;

  if (LRA <= 12.0f) {
    return LC_lowLRA;
  } else if (LRA >= 25.0f) {
    return LC_highLRA;
  } else {
    return LC_lowLRA + (LC_highLRA - LC_lowLRA) * (LRA - 12.0f) / (25.0f - 12.0f);
  }
}

static void
_scaledLeveler_getCharNodes(
    const float inputLRA,
    const float targetLRA,
    int nodeInputLevel[RT_LRAC_MAX_STATIC_CURVE_NODES],
    int nodeOutputGain[RT_LRAC_MAX_STATIC_CURVE_NODES],
    int *pNodeCount) {
  float scaling = _getScaling(inputLRA, targetLRA);
  float loudnessCorrection = _scaledLeveler_getLoudnessCorrection(scaling, inputLRA);

  *pNodeCount = 4;
  nodeInputLevel[0] = -39;
  nodeInputLevel[1] = -24;
  nodeInputLevel[2] = -16;
  nodeInputLevel[3] = -8;
  nodeOutputGain[0] = _roundToInt(scaling * 15.0f + loudnessCorrection);
  nodeOutputGain[1] = _roundToInt(loudnessCorrection);
  nodeOutputGain[2] = _roundToInt(scaling * -8.0f + loudnessCorrection);
  nodeOutputGain[3] = _roundToInt(scaling * -8.0f + min(scaling * -8.0f, -4.0f) + loudnessCorrection);

  assert(*pNodeCount <= RT_LRAC_MAX_STATIC_CURVE_NODES);
}

static void
_getStaticCurve(HANDLE_RT_LRAC hRtLRAC) {
  float LRA_reduction = hRtLRAC->currentLRA - hRtLRAC->targetLRA;
  float used_target_LRA;
  int n;
  LRA_reduction = max(LRA_reduction, 0.0f);
  LRA_reduction = min(LRA_reduction, RT_LRAC_MAX_LRA_REDUCTION);

  used_target_LRA = hRtLRAC->currentLRA - LRA_reduction;

  _scaledLeveler_getCharNodes(hRtLRAC->currentLRA, used_target_LRA, hRtLRAC->nodeInputLevel, hRtLRAC->nodeOutputGain, &hRtLRAC->nodeCount);

  for (n = 0; n < hRtLRAC->nodeCount; n++) {
    hRtLRAC->nodeInputLevel[n] += _roundToInt(-31.0f + 3.0f - (-24.0f));
  }
}

static void
_fillDrcConfig(CONFIG_DRCGAINGEN *pConfigDrc, CONFIG_RT_LRAC *pConfigRtLRAC) {
  int s, ch;

  if (pConfigDrc == NULL) return;
  if (pConfigRtLRAC == NULL) return;

  memset(pConfigDrc, 0, sizeof(CONFIG_DRCGAINGEN));

  pConfigDrc->frameSize = pConfigRtLRAC->frameSize;
  pConfigDrc->sampleRate = pConfigRtLRAC->sampleRate;
  pConfigDrc->baseChannelCount = pConfigRtLRAC->numInputChannels;
  pConfigDrc->drcInstructionCount = 1;
  pConfigDrc->sequenceCount = 1;
  for (s = 0; s < pConfigDrc->sequenceCount; s++) {
    pConfigDrc->numChannels[s] = pConfigRtLRAC->numInputChannels;
    pConfigDrc->downmixId[s] = 0;
    pConfigDrc->drcCharacteristicIndex[s] = -1;
    pConfigDrc->parametricDrcId[s] = RT_LRAC_PARAMETRIC_DRC_ID;
    pConfigDrc->drcLevelCalculationMode[s] = DRC_LEVEL_LEGACY;
    pConfigDrc->numExtParams++;
    for (ch = 0; ch < pConfigDrc->numChannels[s]; ch++) {
      pConfigDrc->sequenceIndex[s][ch] = 0;
    }
    pConfigDrc->PRL[s] = pConfigRtLRAC->loudness;
    pConfigDrc->bandCountPerSequence[s] = 1;
    pConfigDrc->lookAhead4SequenceMs[s] = RT_LRAC_LOOKAHEAD;
  }

  pConfigDrc->maxLookaheadMs = RT_LRAC_MAX_LOOKAHEAD;
  pConfigDrc->useProcessWrapper = 1;
}

CONFIG_RT_LRAC
realtimeLRAControlGetDefaultConfig(void) {
  CONFIG_RT_LRAC defaultConfig = {0};

  defaultConfig.frameSize = 1024;
  defaultConfig.sampleRate = 48000;
  defaultConfig.numInputChannels = 2;
  defaultConfig.loudness = RT_LRAC_DEFAULT_PRL;

  defaultConfig.targetLRA = RT_LRAC_DEFAULT_TARGET_LRA;
  defaultConfig.inputLRAassumption = RT_LRAC_START_LRA_ASSUMPTION;
  defaultConfig.adaptToInput = 1;

  defaultConfig.drcGainGenParams = NULL;

  return defaultConfig;
}

float realtimeLRAControlGetInputLraAssumption(float loudness, int isLevelerActive) {
  if (isLevelerActive) {
    return 11.6f;
  } else {
    float inputLRAassumption = -0.3f * loudness + 6.8f;
    inputLRAassumption = min(inputLRAassumption, 16.1f);
    inputLRAassumption = max(inputLRAassumption, 11.6f);
    return inputLRAassumption;
  }
}

RLC_ERROR
realtimeLRAControlOpen(HANDLE_RT_LRAC *phRtLRAC,
                       CONFIG_RT_LRAC *pConfigRtLRAC) {
  HANDLE_RT_LRAC hRtLRAC;
  CONFIG_DRCGAINGEN configDrc;
  int err = 0;

  if (pConfigRtLRAC->sampleRate < 8000.0f || pConfigRtLRAC->sampleRate > 192000.0f)
    return RLC_OUT_OF_RANGE;
  if (pConfigRtLRAC->frameSize <= 0.0f || pConfigRtLRAC->frameSize > pConfigRtLRAC->sampleRate)
    return RLC_OUT_OF_RANGE;
  if (pConfigRtLRAC->numInputChannels <= 0)
    return RLC_OUT_OF_RANGE;
  if (pConfigRtLRAC->loudness < -64.0f || pConfigRtLRAC->loudness > -10.0f)
    return RLC_OUT_OF_RANGE;
  if (pConfigRtLRAC->targetLRA < 6.0f || pConfigRtLRAC->targetLRA > 16.0f)
    return RLC_OUT_OF_RANGE;
  if (pConfigRtLRAC->inputLRAassumption < 0.0f || pConfigRtLRAC->inputLRAassumption > 30.0f)
    return RLC_OUT_OF_RANGE;

  hRtLRAC = (HANDLE_RT_LRAC)iisCalloc(1, sizeof(RT_LRAC_INSTANCE));
  if (hRtLRAC == NULL)
    return RLC_MEMORY_ERROR;

  *phRtLRAC = hRtLRAC;

  hRtLRAC->numInputChannels = pConfigRtLRAC->numInputChannels;
  hRtLRAC->frameSize = pConfigRtLRAC->frameSize;

  hRtLRAC->currentLRA = pConfigRtLRAC->inputLRAassumption;
  hRtLRAC->targetLRA = pConfigRtLRAC->targetLRA;
  hRtLRAC->loudness = pConfigRtLRAC->loudness;
  hRtLRAC->inputLRAassumption = pConfigRtLRAC->inputLRAassumption;
  hRtLRAC->adaptToInput = pConfigRtLRAC->adaptToInput;

  hRtLRAC->rampBlocks = (int)(RT_LRAC_RAMP_UP_TIME * 10.0f);
  hRtLRAC->smoothLRAcoef = _getSmoothingCoef(RT_LRAC_LRA_SMOOTHING_TIME * 1000.0f, (float)pConfigRtLRAC->sampleRate / (float)pConfigRtLRAC->frameSize);
  hRtLRAC->prevLRA = hRtLRAC->currentLRA;

  hRtLRAC->fastAttack = 10.0f;
  hRtLRAC->fastDecay = 1000.0f;
  hRtLRAC->slowAttack = 1000.0f;
  hRtLRAC->slowDecay = 2000.0f;
  hRtLRAC->holdOff = 10.0f;
  hRtLRAC->attackThr = 15.0f;
  hRtLRAC->decayThr = 20.0f;

  if (pConfigRtLRAC->drcGainGenParams == NULL) {
    _fillDrcConfig(&configDrc, pConfigRtLRAC);

    err = drcGainGeneratorOpen(&hRtLRAC->hUniDrcGainGenStates, &hRtLRAC->hUniDrcGainGenParams, &configDrc, NULL);
    if (err) return RLC_GAIN_GEN_ERROR;

    _getStaticCurve(hRtLRAC);

    hRtLRAC->parametricDrcId = RT_LRAC_PARAMETRIC_DRC_ID;
  } else {
    hRtLRAC->hUniDrcGainGenParams = pConfigRtLRAC->drcGainGenParams;
    hRtLRAC->parametricDrcId = 0;
    hRtLRAC->externalDrcGainGen = 1;
  }

  err = drcGainGeneratorSetExtParams(
      hRtLRAC->hUniDrcGainGenParams,
      hRtLRAC->nodeInputLevel,
      hRtLRAC->nodeOutputGain,
      hRtLRAC->nodeCount,
      hRtLRAC->fastAttack, hRtLRAC->fastDecay,
      hRtLRAC->slowAttack, hRtLRAC->slowDecay,
      hRtLRAC->holdOff,
      hRtLRAC->attackThr, hRtLRAC->decayThr,
      hRtLRAC->parametricDrcId);
  if (err) return RLC_GAIN_GEN_ERROR;

  if (pConfigRtLRAC->drcGainGenParams == NULL) {
    err = drcGainGeneratorInit(hRtLRAC->hUniDrcGainGenStates, hRtLRAC->hUniDrcGainGenParams, &configDrc);
    if (err) return RLC_GAIN_GEN_ERROR;
  }

  return RLC_OK;
}

void realtimeLRAControlReset(HANDLE_RT_LRAC hRtLRAC) {
  if (hRtLRAC == NULL) return;
}

RLC_ERROR
realtimeLRAControlProcess(HANDLE_RT_LRAC hRtLRAC,
                          const float LRA_in,
                          const unsigned int LRA_counter_in,
                          float *audioIn,
                          float **drcGainDb) {
  int err;
  if (hRtLRAC == NULL) return RLC_NULL_POINTER;

  if (hRtLRAC->adaptToInput) {
    if (LRA_counter_in < hRtLRAC->rampBlocks) {
      float ramp_factor = (float)LRA_counter_in / (float)hRtLRAC->rampBlocks;
      hRtLRAC->currentLRA = (1.0f - ramp_factor) * hRtLRAC->inputLRAassumption + ramp_factor * LRA_in;
    } else {
      hRtLRAC->currentLRA = LRA_in;
    }
  } else {
    hRtLRAC->currentLRA = hRtLRAC->inputLRAassumption;
  }

  hRtLRAC->currentLRA = (1.0f - hRtLRAC->smoothLRAcoef) * hRtLRAC->prevLRA + hRtLRAC->smoothLRAcoef * hRtLRAC->currentLRA;
  hRtLRAC->prevLRA = hRtLRAC->currentLRA;

  _getStaticCurve(hRtLRAC);

  err = drcGainGeneratorSetExtParams(
      hRtLRAC->hUniDrcGainGenParams,
      hRtLRAC->nodeInputLevel,
      hRtLRAC->nodeOutputGain,
      hRtLRAC->nodeCount,
      hRtLRAC->fastAttack, hRtLRAC->fastDecay,
      hRtLRAC->slowAttack, hRtLRAC->slowDecay,
      hRtLRAC->holdOff,
      hRtLRAC->attackThr, hRtLRAC->decayThr,
      hRtLRAC->parametricDrcId);
  if (err) return RLC_GAIN_GEN_ERROR;

  err = drcGainGeneratorUpdateExtParams(hRtLRAC->hUniDrcGainGenParams, hRtLRAC->parametricDrcId);
  if (err) return RLC_GAIN_GEN_ERROR;

  if (!hRtLRAC->externalDrcGainGen) {
    err = drcGainGeneratorProcessWrapper(audioIn, drcGainDb, hRtLRAC->hUniDrcGainGenStates, hRtLRAC->hUniDrcGainGenParams);
    if (err) return RLC_PROCESSING_ERROR;
  }

  return RLC_OK;
}

void realtimeLRAControlClose(HANDLE_RT_LRAC *phRtLRAC) {
  HANDLE_RT_LRAC hRtLRAC;
  if (phRtLRAC == NULL) return;

  hRtLRAC = *phRtLRAC;
  if (hRtLRAC == NULL) return;

  if (!hRtLRAC->externalDrcGainGen) {
    drcGainGeneratorClose(&hRtLRAC->hUniDrcGainGenStates, &hRtLRAC->hUniDrcGainGenParams);
    hRtLRAC->hUniDrcGainGenStates = NULL;
    hRtLRAC->hUniDrcGainGenParams = NULL;
  }

  iisFree(*phRtLRAC);
  *phRtLRAC = NULL;
}
