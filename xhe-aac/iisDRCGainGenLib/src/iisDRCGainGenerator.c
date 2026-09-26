
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
#include <stdio.h>
#include <math.h>
#include <iisutillib.h>
#include "iisDRCGainGenerator.h"
#include "iisDRCGainGeneratorProcess.h"

const float bs1770IntTimeS = 0.032f;

const int lookaheadDefaultValueMs = 10;

const float defaultFastAttack = 10;
const float defaultFastDecay = 1000;
const float defaultSlowAttack = 100;
const float defaultSlowDecay = 3000;
const float defaultHoldOff = 10;

const float defaultAttackThr = 15;
const float defaultDecayThr = 20;

static void _fillCompressorParams(
    int const nodeCount,
    float const* nodeInputLevel,
    float const* nodeOutputGain,
    DrcGainGenCompressorParams* compressorParams) {
  int n;
  float slope = 0.0f;

  if (compressorParams == NULL) {
    return;
  }

  if (nodeCount > MAXDRCCURVENODES) {
    compressorParams->drcCurveNodeCount = 0;
    return;
  }

  compressorParams->drcCurveNodeCount = nodeCount;

  for (n = 0; n < nodeCount; n++) {
    compressorParams->nodeInputLevel[n] = nodeInputLevel[n];
    compressorParams->nodeOutputGain[n] = nodeOutputGain[n];
  }

  for (n = 0; n < nodeCount - 1; n++) {
    float divisor = (nodeInputLevel[n] - nodeInputLevel[n + 1]);
    if (divisor == 0.0f) {
      compressorParams->drcCurveNodeCount = 0;
      return;
    }
    slope = (nodeOutputGain[n] - nodeOutputGain[n + 1]) / divisor;
    compressorParams->segmentSlope[n] = slope;
    compressorParams->segmentOffset[n] = nodeOutputGain[n + 1] - nodeInputLevel[n + 1] * slope;
  }
}

int drcGainGeneratorOpen(HANDLE_DRCGAINGEN_STATES* phIisDRCGainGenStates,
                         HANDLE_DRCGAINGEN_PARAMS* phIisDRCGainGenParams,
                         CONFIG_DRCGAINGEN* pConfig_drcGainGen,
                         CONFIG_FORMATCONV4DRCGAINGEN* pConfig_formatConv) {
  int k = 0, m = 0, n = 0, b = 0;
  int numChannelsTmp[MAXNUMGAINBANDSEQUENCES] = {0};
  int ref2DrcInstruction[MAXNUMGAINBANDSEQUENCES] = {0};
  int numGainBandSequencesTmp;
  int gainBandSequenceCount = 0, numLevelEstimInstances = 0;
  int numInputChannelsLevEstim[MAXNUMLEVELELSTIMINSTANCES] = {0};
  int lookAheadInMsLevEstim[MAXNUMLEVELELSTIMINSTANCES] = {0};
  int applyExternalDrcGains[MAXNUMLEVELELSTIMINSTANCES] = {0};
  DRC_LEVELCALC_MODE levelCalcMode[MAXNUMLEVELELSTIMINSTANCES] = {0};
  int maxChannelCount = 0;
  IisDRCGainGenStates* iisDRCGainGenStates = NULL;
  IisDRCGainGenParams* iisDRCGainGenParams = NULL;

  if (pConfig_drcGainGen == NULL) {
    return -8;
  }
  (void)pConfig_formatConv;

  if (pConfig_drcGainGen->baseChannelCount <= 0) {
    return -2;
  }

  if (pConfig_drcGainGen->baseChannelCount > MAXNUMCHANNELS) {
    return 1;
  }
  if (pConfig_drcGainGen->sequenceCount > MAXNUMSEQUENCES) {
    return 1;
  }

  for (n = 0; n < pConfig_drcGainGen->sequenceCount; n++) {
    if (pConfig_drcGainGen->bandCountPerSequence[n] > MAXNUMBANDS) {
      return 1;
    }
  }
  if (pConfig_drcGainGen->drcInstructionCount > MAXNUMINSTRUCTIONS) {
    return 1;
  }

  numGainBandSequencesTmp = 0;
  for (n = 0; n < pConfig_drcGainGen->sequenceCount; n++) {
    int this_ref2DrcInstruction = -1;
    int drcInstructionFound = 0;

    for (k = 0; k < pConfig_drcGainGen->drcInstructionCount; k++) {
      for (m = 0; m < pConfig_drcGainGen->numChannels[k]; m++) {
        if (n == pConfig_drcGainGen->sequenceIndex[k][m]) {
          this_ref2DrcInstruction = k;
          drcInstructionFound = 1;
          break;
        }
      }
      if (drcInstructionFound) {
        break;
      }
    }

    if (this_ref2DrcInstruction < 0) {
      return 1;
    }

    for (b = 0; b < pConfig_drcGainGen->bandCountPerSequence[n]; b++) {
      ref2DrcInstruction[numGainBandSequencesTmp] = this_ref2DrcInstruction;

      if (pConfig_drcGainGen->drcCharacteristicIndex[numGainBandSequencesTmp] == 0) {
        numChannelsTmp[numGainBandSequencesTmp] = 0;
      } else {
        numChannelsTmp[numGainBandSequencesTmp] = pConfig_drcGainGen->baseChannelCount;
      }
      numGainBandSequencesTmp++;
      if (numGainBandSequencesTmp > MAXNUMGAINBANDSEQUENCES) {
        return 1;
      }
    }
  }

  maxChannelCount = pConfig_drcGainGen->baseChannelCount;

  for (k = 0; k < pConfig_drcGainGen->drcInstructionCount; k++) {
    if (maxChannelCount < pConfig_drcGainGen->numChannels[k]) {
      maxChannelCount = pConfig_drcGainGen->numChannels[k];
    }
  }

  if (*phIisDRCGainGenParams == NULL) {
    iisDRCGainGenParams = (IisDRCGainGenParams*)iisCalloc(1, sizeof(struct T_DRCGAINGEN_PARAMS));
    if (iisDRCGainGenParams == NULL) {
      return -3;
    }
    iisDRCGainGenParams->frameSize = pConfig_drcGainGen->frameSize;
    if (numGainBandSequencesTmp != 0) {
      iisDRCGainGenParams->iisDRCGainGenParamsInstance = (IisDRCGainGenParamsInstance*)iisCalloc(numGainBandSequencesTmp, sizeof(struct T_DRCGAINGEN_PARAMSINSTANCE));
    } else {
      iisDRCGainGenParams->iisDRCGainGenParamsInstance = NULL;
    }
    for (k = 0; k < numGainBandSequencesTmp; k++) {
      iisDRCGainGenParams->iisDRCGainGenParamsInstance[k].channelWeight = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcLevelCalculationMode = pConfig_drcGainGen->drcLevelCalculationMode[k];
    }
  }

  iisDRCGainGenParams->maxLookaheadSamples = (int)((float)pConfig_drcGainGen->maxLookaheadMs * 0.001f * (float)pConfig_drcGainGen->sampleRate);

  if (pConfig_drcGainGen->numExtParams <= 0) {
    iisDRCGainGenParams->numExtParams = 0;
  } else {
    iisDRCGainGenParams->numExtParams = pConfig_drcGainGen->numExtParams;
  }
  iisDRCGainGenParams->numExtParamsDefined = 0;

  if (iisDRCGainGenParams->iisDRCGainGenExtParams == NULL && pConfig_drcGainGen->numExtParams > 0) {
    iisDRCGainGenParams->iisDRCGainGenExtParams = (IisDRCGainGenExtParams*)iisCalloc(pConfig_drcGainGen->numExtParams, sizeof(struct T_DRCGAINGEN_EXTPARAMS));
  }

  for (n = 0; n < pConfig_drcGainGen->sequenceCount; n++) {
    for (b = 0; b < pConfig_drcGainGen->bandCountPerSequence[n]; b++) {
      IisDRCGainGenParamsInstance* pGainGenParams = &iisDRCGainGenParams->iisDRCGainGenParamsInstance[gainBandSequenceCount];
      int downmixId = pConfig_drcGainGen->downmixId[ref2DrcInstruction[gainBandSequenceCount]];
      pGainGenParams->sideChainSignalPresent = pConfig_drcGainGen->sideChainSignalPresent[gainBandSequenceCount];
      pGainGenParams->drcCharacteristicIndex = pConfig_drcGainGen->drcCharacteristicIndex[gainBandSequenceCount];

      for (m = 0; m < numChannelsTmp[gainBandSequenceCount]; m++) {
        if (pConfig_drcGainGen->sideChainSignalPresent[gainBandSequenceCount] == 0) {
          pGainGenParams->channelWeight[m] = 0.0f;

          if (downmixId == 0) {
            if (n == pConfig_drcGainGen->sequenceIndex[ref2DrcInstruction[gainBandSequenceCount]][m]) {
              pGainGenParams->channelWeight[m] = 1.0f;
            }
          } else {
            if (n == pConfig_drcGainGen->sequenceIndex[ref2DrcInstruction[gainBandSequenceCount]][0]) {
              pGainGenParams->channelWeight[m] = 1.0f;
            }
          }

        } else {
          pGainGenParams->channelWeight[m] = pConfig_drcGainGen->sideChainChannelWeight[gainBandSequenceCount][m];
        }
      }
      gainBandSequenceCount++;
    }
  }

  for (k = 0; k < MAXNUMLEVELELSTIMINSTANCES; k++) {
    lookAheadInMsLevEstim[k] = -1;
  }

  numLevelEstimInstances = 0;
  for (k = 0; k < numGainBandSequencesTmp; k++) {
    for (m = 0; m < numLevelEstimInstances; m++) {
      if (lookAheadInMsLevEstim[m] == pConfig_drcGainGen->lookAhead4SequenceMs[k]) {
        if (applyExternalDrcGains[m] == pConfig_drcGainGen->applyExternalDrcGains[k]) {
          if (levelCalcMode[m] == pConfig_drcGainGen->drcLevelCalculationMode[k]) {
            break;
          }
        }
      }
    }

    if (m >= numLevelEstimInstances) {
      numInputChannelsLevEstim[numLevelEstimInstances] = pConfig_drcGainGen->baseChannelCount;
      lookAheadInMsLevEstim[numLevelEstimInstances] = pConfig_drcGainGen->lookAhead4SequenceMs[k];
      applyExternalDrcGains[numLevelEstimInstances] = pConfig_drcGainGen->applyExternalDrcGains[k];
      levelCalcMode[numLevelEstimInstances] = pConfig_drcGainGen->drcLevelCalculationMode[k];
      numLevelEstimInstances++;
      if (numLevelEstimInstances > MAXNUMLEVELELSTIMINSTANCES) {
        return 1;
      }
    }
    iisDRCGainGenParams->iisDRCGainGenParamsInstance[k].levelEstimInstanceToApply = m;
    iisDRCGainGenParams->iisDRCGainGenParamsInstance[k].applyExternalDrcGains = pConfig_drcGainGen->applyExternalDrcGains[k];
  }

  if (*phIisDRCGainGenStates == NULL) {
    iisDRCGainGenStates = (IisDRCGainGenStates*)iisCalloc(1, sizeof(struct T_DRCGAINGEN_STATES));
    if (iisDRCGainGenStates == NULL) {
      return -3;
    }
    iisDRCGainGenStates->iisDRCGainGenStatesInstance = (IisDRCGainGenStatesInstance*)iisCalloc(numGainBandSequencesTmp, sizeof(struct T_DRCGAINGEN_STATESINSTANCE));
    iisDRCGainGenStates->audioBufferApplyDelayPointer = (float**)iisCalloc(maxChannelCount, sizeof(float*));

    for (k = 0; k < numGainBandSequencesTmp; k++) {
      iisDRCGainGenStates->iisDRCGainGenStatesInstance[k].levelDb = (float*)iisCalloc(pConfig_drcGainGen->frameSize, sizeof(float));

      iisDRCGainGenStates->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.int_y1 = (float*)iisCalloc(numChannelsTmp[k], sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.ySum = (float*)iisCalloc(pConfig_drcGainGen->frameSize, sizeof(float));
    }
  }

  {
    iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter = (IisDRCGainGenLevelCalcParams_BS1770Filter*)iisCalloc(numLevelEstimInstances, sizeof(struct T_DRCGAINGEN_PARAMSINSTANCE));
    iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter = (IisDRCGainGenLevelCalcStates_BS1770Filter*)iisCalloc(numLevelEstimInstances, sizeof(struct T_DRCGAINGENSTATE_LEVELCALCBS1770FILTER));

    iisDRCGainGenStates->audioWithGainApplied = (float**)iisCalloc(maxChannelCount, sizeof(float*));
    for (m = 0; m < maxChannelCount; m++) {
      iisDRCGainGenStates->audioWithGainApplied[m] = (float*)iisCalloc(pConfig_drcGainGen->frameSize, sizeof(float));
    }
    for (k = 0; k < numLevelEstimInstances; k++) {
      iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[k].frameSize = pConfig_drcGainGen->frameSize;
      iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[k].numChannels = numInputChannelsLevEstim[k];

      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x1 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x2 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y1 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y2 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x1 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x2 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y1 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y2 = (float*)iisCalloc(maxChannelCount, sizeof(float));
      iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare = (float**)iisCalloc(maxChannelCount, sizeof(float*));
      for (m = 0; m < maxChannelCount; m++) {
        iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare[m] = (float*)iisCalloc((pConfig_drcGainGen->frameSize + iisDRCGainGenParams->maxLookaheadSamples), sizeof(float));
      }

      iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[k].applyExternalDrcGains = applyExternalDrcGains[k];
      if (applyExternalDrcGains[k]) {
        iisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].externalGain = (float*)iisCalloc((pConfig_drcGainGen->frameSize + iisDRCGainGenParams->maxLookaheadSamples), sizeof(float));
      }
    }
  }

  if (pConfig_drcGainGen->useProcessWrapper) {
    iisDRCGainGenStates->audioInWithLookahead = (float**)iisCalloc(maxChannelCount, sizeof(float*));
    for (m = 0; m < maxChannelCount; m++) {
      iisDRCGainGenStates->audioInWithLookahead[m] = (float*)iisCalloc((pConfig_drcGainGen->frameSize + iisDRCGainGenParams->maxLookaheadSamples), sizeof(float));
    }
  }

  iisDRCGainGenParams->numGainBandSequences = numGainBandSequencesTmp;
  iisDRCGainGenParams->maxChannelCount = maxChannelCount;
  for (k = 0; k < numGainBandSequencesTmp; k++) {
    iisDRCGainGenParams->iisDRCGainGenParamsInstance[k].numChannels = numChannelsTmp[k];
  }
  iisDRCGainGenParams->numLevelEstimInstances = numLevelEstimInstances;

  *phIisDRCGainGenStates = iisDRCGainGenStates;
  *phIisDRCGainGenParams = iisDRCGainGenParams;

  return 0;
}

int drcGainGeneratorSetExtParams(HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams,
                                 int* nodeInputLevel,
                                 int* nodeOutputGain,
                                 int nodeCount,
                                 float fastAttack,
                                 float fastDecay,
                                 float slowAttack,
                                 float slowDecay,
                                 float holdOff,
                                 float attackThr,
                                 float decayThr,
                                 int parametricDrcId) {
  int k = 0, m = 0, updateExistingParams = 0;

  if (hIisDRCGainGenParams == NULL || nodeInputLevel == NULL || nodeOutputGain == NULL) {
    return -1;
  }

  if (fastAttack < 0.0f || fastDecay < 0.0f || slowAttack < 0.0f || slowDecay < 0.0f || holdOff < 0.0f) {
    return -3;
  }

  if (nodeCount > MAXDRCCURVENODES) {
    return -4;
  }

  for (k = 0; k < hIisDRCGainGenParams->numExtParamsDefined; k++) {
    if (parametricDrcId == hIisDRCGainGenParams->iisDRCGainGenExtParams[k].parametricDrcId) {
      updateExistingParams = 1;
      break;
    }
  }

  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].nodeCount = nodeCount;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].fastAttack = fastAttack;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].fastDecay = fastDecay;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].slowAttack = slowAttack;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].slowDecay = slowDecay;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].holdOff = holdOff;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].attackThr = attackThr;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].decayThr = decayThr;
  hIisDRCGainGenParams->iisDRCGainGenExtParams[k].parametricDrcId = parametricDrcId;
  for (m = 0; m < nodeCount; m++) {
    hIisDRCGainGenParams->iisDRCGainGenExtParams[k].nodeInputLevel[m] = (float)nodeInputLevel[m];
    hIisDRCGainGenParams->iisDRCGainGenExtParams[k].nodeOutputGain[m] = (float)nodeOutputGain[m];
  }

  if (!updateExistingParams) {
    hIisDRCGainGenParams->numExtParamsDefined++;
  }

  if (hIisDRCGainGenParams->numExtParamsDefined > hIisDRCGainGenParams->numExtParams) {
    return -2;
  }

  return 0;
}

int drcGainGeneratorInit(HANDLE_DRCGAINGEN_STATES hIisDRCGainGenStates,
                         HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams,
                         CONFIG_DRCGAINGEN* pConfig_drcGainGen) {
  int err = 0, k = 0, m = 0, n = 0, p = 0, gainBandSequenceCount = 0;
  int bDrcCharacteristicSupported = 0;
  int bIsExternalParamSet = 0;
  int bParametricDrcIdFound = 0;
  int currLevInst = 0, levInstApplied[MAXNUMSEQUENCES] = {0};

  if (pConfig_drcGainGen == NULL) {
    return -9;
  }

  if (hIisDRCGainGenParams->numExtParams != hIisDRCGainGenParams->numExtParamsDefined) {
    return -1;
  }

  gainBandSequenceCount = 0;

  hIisDRCGainGenParams->baseChannelCount = pConfig_drcGainGen->baseChannelCount;
  for (k = 0; k < pConfig_drcGainGen->sequenceCount; k++) {
    for (m = 0; m < pConfig_drcGainGen->bandCountPerSequence[k]; m++) {
      IisDRCGainGenParamsInstance* pGainGenParams = &hIisDRCGainGenParams->iisDRCGainGenParamsInstance[gainBandSequenceCount];

      pGainGenParams->gainOffsetSequence = pConfig_drcGainGen->gainOffsetSequence[gainBandSequenceCount];
      pGainGenParams->frameSize = pConfig_drcGainGen->frameSize;
      pGainGenParams->fs = pConfig_drcGainGen->sampleRate;

      if (pGainGenParams->drcCharacteristicIndex == -1) {
        bDrcCharacteristicSupported = 1;
      }
      if (bDrcCharacteristicSupported == 0) {
        return -1;
      }

      pGainGenParams->sequenceHoldLookaheadSamples = 0;

      levelComputation_initIntParams(&pGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int, bs1770IntTimeS, pConfig_drcGainGen->sampleRate, pConfig_drcGainGen->PRL[gainBandSequenceCount]);

      if ((int)pConfig_drcGainGen->drcCharacteristicIndex[gainBandSequenceCount] <= DRCCHARINDEXUPPER) {
        bIsExternalParamSet = 0;
        if (pConfig_drcGainGen->drcCharacteristicIndex[gainBandSequenceCount] == -1) {
          bIsExternalParamSet = 1;
          if (hIisDRCGainGenParams->numExtParams > 0 && hIisDRCGainGenParams->iisDRCGainGenExtParams != NULL) {
            bParametricDrcIdFound = 0;
            for (p = 0; p < hIisDRCGainGenParams->numExtParams; p++) {
              if (hIisDRCGainGenParams->iisDRCGainGenExtParams[p].parametricDrcId == pConfig_drcGainGen->parametricDrcId[gainBandSequenceCount]) {
                _fillCompressorParams(
                    hIisDRCGainGenParams->iisDRCGainGenExtParams[p].nodeCount,
                    hIisDRCGainGenParams->iisDRCGainGenExtParams[p].nodeInputLevel,
                    hIisDRCGainGenParams->iisDRCGainGenExtParams[p].nodeOutputGain,
                    &pGainGenParams->drcGainGenCompressorParams);

                pGainGenParams->drcGainGenCompressorParams.attackThr = hIisDRCGainGenParams->iisDRCGainGenExtParams[p].attackThr;
                pGainGenParams->drcGainGenCompressorParams.decayThr = hIisDRCGainGenParams->iisDRCGainGenExtParams[p].decayThr;
                pGainGenParams->drcGainGenCompressorParams.fastAttack = (float)(1 - exp((double)(-1.0f / ((hIisDRCGainGenParams->iisDRCGainGenExtParams[p].fastAttack / 1000.0f) * pConfig_drcGainGen->sampleRate))));
                pGainGenParams->drcGainGenCompressorParams.fastDecay = (float)(1 - exp((double)(-1.0f / ((hIisDRCGainGenParams->iisDRCGainGenExtParams[p].fastDecay / 1000.0f) * pConfig_drcGainGen->sampleRate))));
                pGainGenParams->drcGainGenCompressorParams.slowAttack = (float)(1 - exp((double)(-1.0f / ((hIisDRCGainGenParams->iisDRCGainGenExtParams[p].slowAttack / 1000.0f) * pConfig_drcGainGen->sampleRate))));
                pGainGenParams->drcGainGenCompressorParams.slowDecay = (float)(1 - exp((double)(-1.0f / ((hIisDRCGainGenParams->iisDRCGainGenExtParams[p].slowDecay / 1000.0f) * pConfig_drcGainGen->sampleRate))));
                pGainGenParams->drcGainGenCompressorParams.holdOff = (int)(hIisDRCGainGenParams->iisDRCGainGenExtParams[p].holdOff * 0.00533333 * pConfig_drcGainGen->sampleRate + 0.5f);

                hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].drcGainGenCompressorStates.holdCount = 0;
                hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].drcGainGenCompressorStates.smoothGain = 0.f;
                hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].drcGainGenCompressorStates.smoothLevel = -135.f;

                hIisDRCGainGenParams->iisDRCGainGenExtParams[p].gainBandSequenceIndex = gainBandSequenceCount;

                bParametricDrcIdFound = 1;
              }
            }

            if (bParametricDrcIdFound == 0) {
              return -1;
            }
          } else {
            return -1;
          }
        } else {
          return -1;
        }

        if (bIsExternalParamSet == 0) {
          return -1;
        }

      } else {
        return -1;
      }

      for (n = 0; n < pGainGenParams->numChannels; n++) {
        hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].iisDRCGainGenLevelCalcStates_BS1770Int.int_y1[n] = 0.f;
      }
      hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].iisDRCGainGenLevelCalcStates_BS1770Int.bFirstFrameIntegration = 1;
      hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].drcGainGenCompressorStates.bFirstFrameCompressor = 1;
      hIisDRCGainGenStates->iisDRCGainGenStatesInstance[gainBandSequenceCount].drcGainGenCompressorStates.bFirstFrameSmoothing = 1;

      gainBandSequenceCount++;
    }
  }

  for (n = 0; n < gainBandSequenceCount; n++) {
    IisDRCGainGenLevelCalcParams_BS1770Filter* pFilterParams;
    currLevInst = hIisDRCGainGenParams->iisDRCGainGenParamsInstance[n].levelEstimInstanceToApply;
    pFilterParams = &hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[currLevInst];
    if (levInstApplied[currLevInst] != 0) {
      continue;
    } else {
      levInstApplied[currLevInst] = 1;
    }

    if (pConfig_drcGainGen->lookAhead4SequenceMs[n] < 0) {
      {
        pFilterParams->sequenceLookaheadSamples = (int)((float)lookaheadDefaultValueMs * 0.001f * (float)pConfig_drcGainGen->sampleRate);
      }

    } else {
      pFilterParams->sequenceLookaheadSamples = (int)((float)pConfig_drcGainGen->lookAhead4SequenceMs[n] * 0.001f * (float)pConfig_drcGainGen->sampleRate);
    }

    if ((pFilterParams->sequenceLookaheadSamples + pFilterParams->sequenceHoldLookaheadSamples) >
        (int)((float)pConfig_drcGainGen->maxLookaheadMs * 0.001f * (float)pConfig_drcGainGen->sampleRate)) {
      if (pFilterParams->sequenceHoldLookaheadSamples > 0) {
        pFilterParams->sequenceHoldLookaheadSamples = 0;

        for (k = 0; k < gainBandSequenceCount; k++) {
          if (hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].levelEstimInstanceToApply == currLevInst) {
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].sequenceHoldLookaheadSamples = 0;
          }
        }
      }

      if ((pFilterParams->sequenceLookaheadSamples) >
          (int)((float)pConfig_drcGainGen->maxLookaheadMs * 0.001f * (float)pConfig_drcGainGen->sampleRate)) {
        pFilterParams->sequenceLookaheadSamples = (int)((float)lookaheadDefaultValueMs * 0.001f * (float)pConfig_drcGainGen->sampleRate);

        for (k = 0; k < gainBandSequenceCount; k++) {
          if (hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].levelEstimInstanceToApply == currLevInst) {
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.attackThr = defaultAttackThr;
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.decayThr = defaultDecayThr;
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.fastAttack = (float)(1 - exp((double)(-1.0f / ((defaultFastAttack / 1000.0f) * pConfig_drcGainGen->sampleRate))));
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.fastDecay = (float)(1 - exp((double)(-1.0f / ((defaultFastDecay / 1000.0f) * pConfig_drcGainGen->sampleRate))));
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.slowAttack = (float)(1 - exp((double)(-1.0f / ((defaultSlowAttack / 1000.0f) * pConfig_drcGainGen->sampleRate))));
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.slowDecay = (float)(1 - exp((double)(-1.0f / ((defaultSlowDecay / 1000.0f) * pConfig_drcGainGen->sampleRate))));
            hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].drcGainGenCompressorParams.holdOff = (int)(defaultHoldOff * 0.00533333f * pConfig_drcGainGen->sampleRate + 0.5f);
          }
        }

        if ((pFilterParams->sequenceLookaheadSamples) >
            (int)((float)pConfig_drcGainGen->maxLookaheadMs * 0.001f * (float)pConfig_drcGainGen->sampleRate)) {
          return 1;
        } else {
        }
      } else {
      }
    }

    switch (hIisDRCGainGenParams->iisDRCGainGenParamsInstance[n].drcLevelCalculationMode) {
      case DRC_LEVEL_BS1770:
        levelComputation_initFilterParams(pFilterParams, pConfig_drcGainGen->sampleRate, 0);
        break;
      case DRC_LEVEL_RLB:
        levelComputation_initFilterParams(pFilterParams, pConfig_drcGainGen->sampleRate, 0);
        pFilterParams->skipPreFilter = 1;
        break;
      case DRC_LEVEL_LEGACY:
        levelComputation_initFilterParams_legacy(pFilterParams, pConfig_drcGainGen->sampleRate);
        break;
      default:
        return -1;
    }
  }

  for (k = 0; k < hIisDRCGainGenParams->numLevelEstimInstances; k++) {
    hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].bFirstFrameFiltering = 1;

    for (n = 0; n < hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[k].numChannels; n++) {
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x1[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x2[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y1[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y2[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x1[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x2[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y1[n] = 0.f;
      hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y2[n] = 0.f;
      for (m = 0; m < pConfig_drcGainGen->frameSize; m++) {
        hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare[n][m] = 0.f;
      }
    }
  }

  return err;
}

int drcGainGeneratorUpdateExtParams(HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams,
                                    const int parametricDrcId) {
  int err = 0, k = 0;
  float sampleRate;
  IisDRCGainGenExtParams* pExtParams;
  IisDRCGainGenParamsInstance* pGainGenParams;

  for (k = 0; k < hIisDRCGainGenParams->numExtParamsDefined; k++) {
    if (parametricDrcId == hIisDRCGainGenParams->iisDRCGainGenExtParams[k].parametricDrcId) {
      break;
    }
  }
  if (k >= hIisDRCGainGenParams->numExtParamsDefined) {
    return -1;
  }

  pExtParams = &hIisDRCGainGenParams->iisDRCGainGenExtParams[k];
  pGainGenParams = &hIisDRCGainGenParams->iisDRCGainGenParamsInstance[pExtParams->gainBandSequenceIndex];

  _fillCompressorParams(
      pExtParams->nodeCount,
      pExtParams->nodeInputLevel,
      pExtParams->nodeOutputGain,
      &pGainGenParams->drcGainGenCompressorParams);

  sampleRate = pGainGenParams->fs;

  pGainGenParams->drcGainGenCompressorParams.attackThr = pExtParams->attackThr;
  pGainGenParams->drcGainGenCompressorParams.decayThr = pExtParams->decayThr;
  pGainGenParams->drcGainGenCompressorParams.fastAttack = (float)(1 - exp((double)(-1.0f / ((pExtParams->fastAttack / 1000.0f) * sampleRate))));
  pGainGenParams->drcGainGenCompressorParams.fastDecay = (float)(1 - exp((double)(-1.0f / ((pExtParams->fastDecay / 1000.0f) * sampleRate))));
  pGainGenParams->drcGainGenCompressorParams.slowAttack = (float)(1 - exp((double)(-1.0f / ((pExtParams->slowAttack / 1000.0f) * sampleRate))));
  pGainGenParams->drcGainGenCompressorParams.slowDecay = (float)(1 - exp((double)(-1.0f / ((pExtParams->slowDecay / 1000.0f) * sampleRate))));
  pGainGenParams->drcGainGenCompressorParams.holdOff = (int)(pExtParams->holdOff * 0.00533333 * sampleRate + 0.5f);

  return err;
}

int drcGainGeneratorProcessInstance(float** audioIn,
                                    float* drcGain,
                                    IisDRCGainGenStatesInstance* iisDRCGainGenStatesInstance,
                                    IisDRCGainGenParamsInstance iisDRCGainGenParamsInstance) {
  int err = 0;
  int k = 0;

  if (audioIn == NULL || drcGain == NULL || iisDRCGainGenStatesInstance == NULL) {
    return -10;
  } else {
    for (k = 0; k < iisDRCGainGenParamsInstance.numChannels; k++) {
      if (audioIn[k] == NULL) {
        return -11;
      }
    }
  }

  err = drcGainGen_levelComputation((const float* const*)audioIn, iisDRCGainGenStatesInstance->levelDb, iisDRCGainGenParamsInstance, iisDRCGainGenStatesInstance);
  if (err) return (err);

  err = drcGainGen_compressorGainComputation(drcGain, iisDRCGainGenStatesInstance, iisDRCGainGenParamsInstance);
  if (err) return (err);

  err = drcGainGen_compressorGainSmoothing(drcGain, iisDRCGainGenStatesInstance, iisDRCGainGenParamsInstance);
  if (err) return (err);

  if (iisDRCGainGenParamsInstance.gainOffsetSequence != 0.f) {
    err = drcGainGen_applyGainOffsetSequence(drcGain, iisDRCGainGenParamsInstance);
    if (err) return (err);
  }

  return 0;
}

int drcGainGeneratorProcess(float** audioIn,
                            float** drcGainDb,
                            int nInputSamples,
                            HANDLE_DRCGAINGEN_STATES hIisDRCGainGenStates,
                            HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams) {
  int err = 0, k = 0, ch = 0, sample = 0;
  int levelEstimInstance;
  int appliedLevelFilter[MAXNUMLEVELELSTIMINSTANCES] = {0};
  const double dBtoLinFactor = log(10.0) / 20.0;

  if (audioIn == NULL || drcGainDb == NULL || hIisDRCGainGenStates == NULL || hIisDRCGainGenParams == NULL) {
    return -10;
  } else {
    for (k = 0; k < hIisDRCGainGenParams->baseChannelCount; k++) {
      if (audioIn[k] == NULL) {
        return -11;
      }
    }
    for (k = 0; k < hIisDRCGainGenParams->numGainBandSequences; k++) {
      if (drcGainDb[k] == NULL) {
        return -12;
      }
    }
  }
  (void)nInputSamples;

  for (k = 0; k < hIisDRCGainGenParams->numGainBandSequences; k++) {
    IisDRCGainGenLevelCalcParams_BS1770Filter* pFilterParams;
    levelEstimInstance = hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].levelEstimInstanceToApply;
    int frameSize = hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance].frameSize;
    pFilterParams = &hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance];
    if (appliedLevelFilter[levelEstimInstance] == 0) {
      for (ch = 0; ch < hIisDRCGainGenParams->baseChannelCount; ch++) {
        hIisDRCGainGenStates->audioBufferApplyDelayPointer[ch] = &audioIn[ch][pFilterParams->sequenceLookaheadSamples];
      }

      if (hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].applyExternalDrcGains != 0) {
        float* pExternalGain = hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].externalGain;
        float* externalGainApplyDelayPointer = pExternalGain + pFilterParams->sequenceLookaheadSamples;

        memcpy(pExternalGain + hIisDRCGainGenParams->maxLookaheadSamples, drcGainDb[k], sizeof(float) * frameSize);

        for (ch = 0; ch < hIisDRCGainGenParams->baseChannelCount; ch++) {
          for (sample = 0; sample < pFilterParams->frameSize; sample++) {
            hIisDRCGainGenStates->audioWithGainApplied[ch][sample] = hIisDRCGainGenStates->audioBufferApplyDelayPointer[ch][sample] * (float)exp(dBtoLinFactor * externalGainApplyDelayPointer[sample]);
          }

          hIisDRCGainGenStates->audioBufferApplyDelayPointer[ch] = hIisDRCGainGenStates->audioWithGainApplied[ch];
        }
      }

      levelComputation_BS1770Filter((const float* const*)hIisDRCGainGenStates->audioBufferApplyDelayPointer, hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].xSquare, &hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance], &hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance]);
      appliedLevelFilter[levelEstimInstance] = 1;
    }
  }

  for (k = 0; k < hIisDRCGainGenParams->numGainBandSequences; k++) {
    levelEstimInstance = hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].levelEstimInstanceToApply;

    err = drcGainGeneratorProcessInstance(hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].xSquare, drcGainDb[k], &hIisDRCGainGenStates->iisDRCGainGenStatesInstance[k], hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k]);

    if (hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].applyExternalDrcGains != 0) {
      int frameSize = hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance].frameSize;
      for (sample = 0; sample < frameSize; sample++) {
        drcGainDb[k][sample] += hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].externalGain[sample];
      }
    }
  }

  for (levelEstimInstance = 0; levelEstimInstance < hIisDRCGainGenParams->numLevelEstimInstances; levelEstimInstance++) {
    if (hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance].applyExternalDrcGains) {
      int frameSize = hIisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Filter[levelEstimInstance].frameSize;
      memmove(hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].externalGain, &hIisDRCGainGenStates->iisDRCGainGenLevelCalcStates_BS1770Filter[levelEstimInstance].externalGain[frameSize], hIisDRCGainGenParams->maxLookaheadSamples * sizeof(float));
    }
  }

  return err;
}

int drcGainGeneratorProcessWrapper(const float* audioIn,
                                   float** drcGainDb,
                                   HANDLE_DRCGAINGEN_STATES hIisDRCGainGenStates,
                                   HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams) {
  int err = 0;
  int c, s;
  int maxLookahead = hIisDRCGainGenParams->maxLookaheadSamples;
  int frameSize = hIisDRCGainGenParams->frameSize;
  int numChannels = hIisDRCGainGenParams->baseChannelCount;

  if (hIisDRCGainGenStates->audioInWithLookahead == NULL) {
    return -10;
  }
  for (c = 0; c < numChannels; c++) {
    if (hIisDRCGainGenStates->audioInWithLookahead[c] == NULL) {
      return -10;
    }
  }

  for (c = 0; c < numChannels; c++) {
    for (s = 0; s < frameSize; s++) {
      hIisDRCGainGenStates->audioInWithLookahead[c][maxLookahead + s] = audioIn[numChannels * s + c];
    }
  }

  err = drcGainGeneratorProcess(hIisDRCGainGenStates->audioInWithLookahead, drcGainDb, 0, hIisDRCGainGenStates, hIisDRCGainGenParams);
  if (err) return err;

  for (c = 0; c < numChannels; c++) {
    memmove(hIisDRCGainGenStates->audioInWithLookahead[c], hIisDRCGainGenStates->audioInWithLookahead[c] + frameSize, maxLookahead * sizeof(float));
  }

  return err;
}

int drcGainGeneratorClose(HANDLE_DRCGAINGEN_STATES* phIisDRCGainGenStates,
                          HANDLE_DRCGAINGEN_PARAMS* phIisDRCGainGenParams)

{
  int err = 0, k = 0;
  int m = 0;

  if (*phIisDRCGainGenStates != NULL) {
    for (k = 0; k < (*phIisDRCGainGenParams)->numGainBandSequences; k++) {
      if ((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].levelDb != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].levelDb);
        (*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].levelDb = NULL;
      }

      if ((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.int_y1 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.int_y1);
        (*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.int_y1 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.ySum != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.ySum);
        (*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance[k].iisDRCGainGenLevelCalcStates_BS1770Int.ySum = NULL;
      }
    }

    for (k = 0; k < (*phIisDRCGainGenParams)->numLevelEstimInstances; k++) {
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x1 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x1);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x1 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x2 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x2);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_x2 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y1 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y1);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y1 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y2 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y2);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].pre_y2 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x1 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x1);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x1 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x2 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x2);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_x2 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y1 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y1);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y1 = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y2 != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y2);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].rlb_y2 = NULL;
      }
      for (m = 0; m < (*phIisDRCGainGenParams)->maxChannelCount; m++) {
        if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare[m] != NULL) {
          iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare[m]);
          (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare[m] = NULL;
        }
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].xSquare = NULL;
      }
      if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].externalGain != NULL) {
        iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].externalGain);
        (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter[k].externalGain = NULL;
      }
    }

    for (m = 0; m < (*phIisDRCGainGenParams)->maxChannelCount; m++) {
      if ((*phIisDRCGainGenStates)->audioWithGainApplied[m] != NULL) {
        iisFree((*phIisDRCGainGenStates)->audioWithGainApplied[m]);
        (*phIisDRCGainGenStates)->audioWithGainApplied[m] = NULL;
      }
    }
    if ((*phIisDRCGainGenStates)->audioWithGainApplied != NULL) {
      iisFree((*phIisDRCGainGenStates)->audioWithGainApplied);
      (*phIisDRCGainGenStates)->audioWithGainApplied = NULL;
    }

    if ((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter != NULL) {
      iisFree((*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter);
      (*phIisDRCGainGenStates)->iisDRCGainGenLevelCalcStates_BS1770Filter = NULL;
    }

    if ((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance != NULL) {
      iisFree((*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance);
      (*phIisDRCGainGenStates)->iisDRCGainGenStatesInstance = NULL;
    }
    if ((*phIisDRCGainGenStates)->audioBufferApplyDelayPointer != NULL) {
      iisFree((*phIisDRCGainGenStates)->audioBufferApplyDelayPointer);
      (*phIisDRCGainGenStates)->audioBufferApplyDelayPointer = NULL;
    }

    if ((*phIisDRCGainGenStates)->audioInWithLookahead != NULL) {
      for (m = 0; m < (*phIisDRCGainGenParams)->maxChannelCount; m++) {
        if ((*phIisDRCGainGenStates)->audioInWithLookahead[m] != NULL) {
          iisFree((*phIisDRCGainGenStates)->audioInWithLookahead[m]);
          (*phIisDRCGainGenStates)->audioInWithLookahead[m] = NULL;
        }
      }

      iisFree((*phIisDRCGainGenStates)->audioInWithLookahead);
      (*phIisDRCGainGenStates)->audioInWithLookahead = NULL;
    }

    if ((*phIisDRCGainGenStates) != NULL) {
      iisFree(*phIisDRCGainGenStates);
      *phIisDRCGainGenStates = NULL;
    }
  }

  if (*phIisDRCGainGenParams != NULL) {
    for (k = 0; k < (*phIisDRCGainGenParams)->numGainBandSequences; k++) {
      if ((*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance != NULL) {
        if ((*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance[k].channelWeight != NULL) {
          iisFree((*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance[k].channelWeight);
          (*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance[k].channelWeight = NULL;
        }
      }
    }

    if ((*phIisDRCGainGenParams)->iisDRCGainGenLevelCalcParams_BS1770Filter != NULL) {
      iisFree((*phIisDRCGainGenParams)->iisDRCGainGenLevelCalcParams_BS1770Filter);
      (*phIisDRCGainGenParams)->iisDRCGainGenLevelCalcParams_BS1770Filter = NULL;
    }
    if ((*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance != NULL) {
      iisFree((*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance);
      (*phIisDRCGainGenParams)->iisDRCGainGenParamsInstance = NULL;
    }
    if ((*phIisDRCGainGenParams)->iisDRCGainGenExtParams != NULL) {
      iisFree((*phIisDRCGainGenParams)->iisDRCGainGenExtParams);
      (*phIisDRCGainGenParams)->iisDRCGainGenExtParams = NULL;
    }
    if (*phIisDRCGainGenParams != NULL) {
      iisFree(*phIisDRCGainGenParams);
      *phIisDRCGainGenParams = NULL;
    }
  }

  return err;
}

int drcGainGeneratorSetLoudness(float* loudnessSequences,
                                HANDLE_DRCGAINGEN_PARAMS hIisDRCGainGenParams) {
  int err = 0, k = 0;

  if (loudnessSequences == NULL || hIisDRCGainGenParams == NULL) {
    return -13;
  }

  for (k = 0; k < hIisDRCGainGenParams->numGainBandSequences; k++) {
    hIisDRCGainGenParams->iisDRCGainGenParamsInstance[k].iisDRCGainGenLevelCalcParams_BS1770Int.PRL = loudnessSequences[k];
  }

  return err;
}
