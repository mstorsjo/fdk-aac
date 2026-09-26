
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
#include <float.h>

#include "aacenc_internal.h"
#include "block_switch.h"
#include "psy_const.h"

#include "cpuinfo.h"
#include "mathlib.h"
#include "iisutillib.h"
#include "glob_con.h"

#if defined __GNUC__ || defined __clang__
#define SSE_OPTI __attribute__((unused))
#else
#define SSE_OPTI
#endif

#ifndef _NOT_AVOID_FLOAT_DENORMALS
static const ALIGN_16_BYTE float _myMin = 1.e-15f;
#endif

static const int lowRateGroupingTable[TRANS_FAC][MAX_NO_OF_GROUPS] = {
    {1, 7, 0, 0},
    {2, 6, 0, 0},
    {3, 5, 0, 0},
    {4, 4, 0, 0},
    {5, 3, 0, 0},
    {6, 2, 0, 0},
    {7, 1, 0, 0},
    {7, 1, 0, 0}};
static const int suggestedGroupingTable[TRANS_FAC][MAX_NO_OF_GROUPS] = {
    {1, 3, 3, 1},
    {1, 1, 3, 3},
    {2, 1, 3, 2},
    {3, 1, 3, 1},
    {3, 1, 1, 3},
    {3, 2, 1, 2},
    {3, 3, 1, 1},
    {3, 3, 1, 1}};

static void iisaacfenc_CalcWindowEnergy_NoOpt(
    float const *const timeSignal,
    float windowNrgF[2 * TRANS_FAC + 1],
    float *const firState1,
    float *const firState2,
    int const windowLen) {
  int i, w, k = 0;
  float tempUnfiltered, tempFiltered;

  for (w = 0; w < 2 * TRANS_FAC; w++) {
    windowNrgF[w] = HLM_MIN_NRG;

    for (i = 0; i < windowLen; i++) {
      tempUnfiltered = timeSignal[k++];
      tempFiltered = 0.375f * tempUnfiltered - 0.5f * (*firState1) + 0.125f * (*firState2);
#ifndef _NOT_AVOID_FLOAT_DENORMALS
      if ((float)fabs(tempFiltered) <= _myMin) tempFiltered = 0.0f;
#endif
      windowNrgF[w] += tempFiltered * tempFiltered;

      *firState2 = *firState1;
      *firState1 = tempUnfiltered;
    }
  }
}

static void initCalcWindowEnergyOpt(ATTACK_DETECTION *const attackDetection,
                                    SSE_OPTI int const useCpuOptimization) {
  if (NULL != attackDetection) {
    attackDetection->CalcWindowEnergy_Ptr = &iisaacfenc_CalcWindowEnergy_NoOpt;
  }
}

static void initAttackDetection(ATTACK_DETECTION *const attackDetection,
                                AACENC_CODEC_TYPE const codecType,
                                int const sampleRate,
                                float const bitsPerSample) {
  assert(attackDetection);

  attackDetection->firState1 = attackDetection->firState2 = 0.0f;

  attackDetection->attackRatio = (float)sampleRate * (float)sampleRate / 134217728.0f + 7.4375f;

  if (sampleRate <= 12000) {
    attackDetection->facAccWindowNrgF = 0.578125f;
  } else {
    attackDetection->facAccWindowNrgF = 0.828125f - 3000.0f / (float)sampleRate;
  }

  if ((sampleRate <= 24000) && (bitsPerSample < 1.375f) && (codecType == AACENC_CODEC_AAC)) {
    attackDetection->facAccWindowNrgF += min(0.25f, 1.375f - bitsPerSample);
  }

  if ((sampleRate >= 44100) && (bitsPerSample < 1.25f) && (codecType == AACENC_CODEC_AAC)) {
    attackDetection->facAccWindowNrgF += min(0.1875f, 1.25f - bitsPerSample);
  }

  {
    int i = 0;
    for (i = 0; i <= 2 * TRANS_FAC; i++) {
      attackDetection->windowNrgF[i] = HLM_MIN_NRG;
    }
  }
  attackDetection->accWindowNrgF = attackDetection->accWindowNrgS = HLM_MIN_NRG;
  attackDetection->lastWindowNrgF = attackDetection->lastWindowNrgS = HLM_MIN_NRG;
  attackDetection->maxAccNrgFRatio = attackDetection->lastMaxAccNrgFRatio = 1.0f;
  attackDetection->maxWinNrgFRatio = attackDetection->lastMaxWinNrgFRatio = 1.0f;
  attackDetection->minWinNrgFRatio = attackDetection->lastMinWinNrgFRatio = 1.0f;

  attackDetection->attack = 0;
  attackDetection->nextAttack = 0;
}

static void detectAttack(
    ATTACK_DETECTION *const attackDetection,
    float const *const timeSignal,
    int const granuleLength) {
  int i;
  float blockNrgF[TRANS_FAC] = {0.0f};
  float blockNrgS[TRANS_FAC] = {0.0f};

  assert(attackDetection);
  assert(timeSignal);
  assert(granuleLength > 0 && granuleLength % (2 * TRANS_FAC) == 0);

  attackDetection->attack = attackDetection->nextAttack;
  attackDetection->attackIndex = attackDetection->nextAttackIndex;

  attackDetection->lastMaxAccNrgFRatio = attackDetection->maxAccNrgFRatio;
  attackDetection->lastMaxWinNrgFRatio = attackDetection->maxWinNrgFRatio;
  attackDetection->lastMinWinNrgFRatio = attackDetection->minWinNrgFRatio;

  attackDetection->CalcWindowEnergy_Ptr(timeSignal,
                                        attackDetection->windowNrgF,
                                        &attackDetection->firState1,
                                        &attackDetection->firState2,
                                        granuleLength / (2 * TRANS_FAC));

  attackDetection->windowNrgF[2 * TRANS_FAC] = attackDetection->windowNrgF[2 * TRANS_FAC - 1];
  attackDetection->lastWindowNrgS = 0.5f * attackDetection->lastWindowNrgS + attackDetection->windowNrgF[0];
  if (attackDetection->lastWindowNrgS > attackDetection->accWindowNrgS) {
    attackDetection->accWindowNrgS = attackDetection->lastWindowNrgS;
  }

  attackDetection->nextAttack = 0;
  attackDetection->nextAttackIndex = 0;
  attackDetection->maxAccNrgFRatio = HLM_MIN_NRG;
  attackDetection->maxWinNrgFRatio = HLM_MIN_NRG;
  attackDetection->minWinNrgFRatio = 1.0f;

  for (i = 0; i < TRANS_FAC; i++) {
    blockNrgF[i] = attackDetection->windowNrgF[2 * i] + attackDetection->windowNrgF[2 * i + 1];
    blockNrgS[i] = attackDetection->windowNrgF[2 * i + 1] + attackDetection->windowNrgF[2 * i + 2];
  }

  for (i = 0; i < TRANS_FAC; i++) {
    {
      float enRatioF = blockNrgF[i] / attackDetection->lastWindowNrgF;
      if (enRatioF > attackDetection->maxWinNrgFRatio) {
        attackDetection->maxWinNrgFRatio = enRatioF;
      } else if (enRatioF < attackDetection->minWinNrgFRatio) {
        attackDetection->minWinNrgFRatio = enRatioF;
      }
      attackDetection->lastWindowNrgF = blockNrgF[i];
    }
    {
      float enRatioS = blockNrgS[i] / attackDetection->lastWindowNrgS;
      if (enRatioS > attackDetection->maxWinNrgFRatio) {
        attackDetection->maxWinNrgFRatio = enRatioS;
      } else if (enRatioS < attackDetection->minWinNrgFRatio) {
        attackDetection->minWinNrgFRatio = enRatioS;
      }
      attackDetection->lastWindowNrgS = blockNrgS[i];
    }
  }

  for (i = 0; i < TRANS_FAC; i++) {
    float enRatio = blockNrgF[i] / attackDetection->accWindowNrgF;
    if (enRatio > attackDetection->maxAccNrgFRatio) {
      attackDetection->maxAccNrgFRatio = enRatio;
      attackDetection->nextAttackIndex = i;
    }
    enRatio = blockNrgS[i] / attackDetection->accWindowNrgS;
    if (enRatio > attackDetection->maxAccNrgFRatio) {
      attackDetection->maxAccNrgFRatio = enRatio;
      attackDetection->nextAttackIndex = i;
    }

    attackDetection->accWindowNrgF *= attackDetection->facAccWindowNrgF;
    if (blockNrgF[i] > attackDetection->accWindowNrgF) {
      attackDetection->accWindowNrgF = blockNrgF[i];
    }
    attackDetection->accWindowNrgS *= attackDetection->facAccWindowNrgF;
    if (blockNrgS[i] > attackDetection->accWindowNrgS) {
      attackDetection->accWindowNrgS = blockNrgS[i];
    }
  }

  if (
      (
          (attackDetection->maxAccNrgFRatio > attackDetection->attackRatio) ||
          ((attackDetection->maxAccNrgFRatio > 2.875f) && (attackDetection->facAccWindowNrgF <= 0.78125f) && (attackDetection->maxWinNrgFRatio > 144.0f)))) {
    attackDetection->nextAttack = 1;
  } else if ((attackDetection->attack)) {
    if (((attackDetection->minWinNrgFRatio < 0.0625f) && (attackDetection->attackIndex == TRANS_FAC - 1)) ||
        ((attackDetection->maxWinNrgFRatio > 96.0f) && (attackDetection->lastMaxWinNrgFRatio > 144.0f))) {
      attackDetection->nextAttack = 1;
    }
  }
}

static void determineNextWindowSequence(BLOCK_SWITCHING_CONTROL *blSwControl,
                                        const int granuleLength,
                                        AACENC_CODEC_TYPE codecType,
                                        const int preSapFrame,
                                        const float useLowOverlapBlockSwitching,
                                        const int useAcelpPrev,
                                        const int useAcelp,
                                        const int useAcelpNext,
                                        const int noShortBlocks) {
  const int STOPx_WINDOW = (codecType == AACENC_CODEC_MPEGH) && (useLowOverlapBlockSwitching > 0.f)
                               ? STOPSTART_WINDOW
                               : STOP_WINDOW;

  assert(blSwControl);
  assert(granuleLength > 0);

  if (!useAcelpPrev && !useAcelp && !useAcelpNext &&
      noShortBlocks && preSapFrame == 0) {
    switch (blSwControl->prevWindowSequence) {
      case LONG_WINDOW:
      case STOP_WINDOW:
        blSwControl->windowSequence = LONG_WINDOW;
        break;

      case START_WINDOW:
      case SHORT_WINDOW:
      case STOPSTART_WINDOW:
        blSwControl->windowSequence = STOP_WINDOW;
        break;

      default:

        break;
    }
    blSwControl->nextWindowSequence = LONG_WINDOW;

  } else if (blSwControl->attackDetection.nextAttack || preSapFrame || useAcelpNext || useAcelp) {
    if (blSwControl->attackDetection.nextAttack || useAcelpNext) {
      blSwControl->nextWindowSequence = SHORT_WINDOW;
    } else {
      blSwControl->nextWindowSequence = STOPx_WINDOW;
    }

    if (blSwControl->windowSequence == LONG_WINDOW) {
      blSwControl->windowSequence = START_WINDOW;
      if (useAcelp && useAcelpNext) {
        blSwControl->windowSequence = STOP_WINDOW;
      }
    }

    if (blSwControl->windowSequence == LONG_WINDOW && useAcelp && useAcelpNext) {
      blSwControl->windowSequence = STOP_WINDOW;
    } else if (blSwControl->windowSequence == STOP_WINDOW) {
      switch (codecType) {
        case AACENC_CODEC_XHEAAC:
          blSwControl->windowSequence = STOPSTART_WINDOW;
          break;
        case AACENC_CODEC_AAC:
        default:
          blSwControl->windowSequence = SHORT_WINDOW;
          break;
      }
    }
  } else if ((blSwControl->noStartStopSequence == 0) &&
             (blSwControl->attackDetection.maxAccNrgFRatio > 0.390625f * blSwControl->attackDetection.attackRatio) &&
             (blSwControl->attackDetection.nextAttackIndex < TRANS_FAC / 2) &&
             (blSwControl->windowSequence == LONG_WINDOW)) {
    blSwControl->nextWindowSequence = STOPx_WINDOW;
    blSwControl->windowSequence = START_WINDOW;
  } else {
    if (blSwControl->windowSequence == SHORT_WINDOW || (blSwControl->windowSequence == STOPSTART_WINDOW)) {
      blSwControl->nextWindowSequence = STOPx_WINDOW;
    } else {
      blSwControl->nextWindowSequence = LONG_WINDOW;
    }
  }
}

static void determineGroupingForCurrentFrame(BLOCK_SWITCHING_CONTROL *blSwControl) {
  assert(blSwControl);

  if (blSwControl->windowSequence == SHORT_WINDOW) {
    int i;
    blSwControl->noOfGroups = MAX_NO_OF_GROUPS;
    if ((blSwControl->prevWindowSequence == SHORT_WINDOW) && (blSwControl->syncRatio == 0.0f) &&
        (blSwControl->nextWindowSequence == SHORT_WINDOW) && (blSwControl->attackDetection.nextAttack)) {
      blSwControl->noOfGroups >>= 1;
      for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
        blSwControl->groupLen[i] = lowRateGroupingTable[blSwControl->attackDetection.attackIndex][i];
      }
    } else
      for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
        blSwControl->groupLen[i] = suggestedGroupingTable[blSwControl->attackDetection.attackIndex][i];
      }
  } else {
    blSwControl->noOfGroups = 1;
    blSwControl->groupLen[0] = 1;
  }
}

int iisaacfenc_InitBlockSwitching(BLOCK_SWITCHING_CONTROL *blSwControl,
                                  AACENC_CODEC_TYPE codecType,
                                  const int sampleRate,
                                  const float bitsPerSample,
                                  const int useCpuOptimization) {
  initCalcWindowEnergyOpt(&blSwControl->attackDetection, useCpuOptimization);

  initAttackDetection(&blSwControl->attackDetection,
                      codecType,
                      sampleRate,
                      bitsPerSample);

  blSwControl->syncRatio = (bitsPerSample < 1.625f) ? 0.0f : 0.15625f;

  blSwControl->prevWindowSequence = LONG_WINDOW;
  blSwControl->windowSequence = LONG_WINDOW;
  blSwControl->nextWindowSequence = LONG_WINDOW;

  blSwControl->prevWindowShape = KBD_WINDOW;
  blSwControl->windowShape = KBD_WINDOW;
  blSwControl->windowKernelNum = MDCT_IV;

  blSwControl->useAcelpPrev = 0;
  blSwControl->useAcelp = 0;
  blSwControl->useAcelpNext = 0;

  blSwControl->noStartStopSequence = 0;

  return 1;
}

int iisaacfenc_BlockSwitching(BLOCK_SWITCHING_CONTROL *blSwControl,
                              const int granuleLength,
                              const int isLFE,
                              AACENC_CODEC_TYPE codecType,
                              const int preSapFrame,
                              const float useLowOverlapBlockSwitching,
                              const int useAcelpPrev,
                              const int useAcelp,
                              const int useAcelpNext,
                              const int noShortBlocks) {
  if (isLFE) {
    blSwControl->noOfGroups = 1;
    blSwControl->groupLen[0] = 1;
    blSwControl->windowSequence = LONG_WINDOW;
    return 0;
  }

  {
    int i;
    for (i = 0; i < TRANS_FAC; i++) {
      blSwControl->groupLen[i] = 0;
    }
  }

  detectAttack(&blSwControl->attackDetection,
               blSwControl->timeSignal,
               granuleLength);

  blSwControl->prevWindowSequence = blSwControl->windowSequence;
  blSwControl->windowSequence = blSwControl->nextWindowSequence;
  blSwControl->useAcelpPrev = useAcelpPrev;
  blSwControl->useAcelp = useAcelp;
  blSwControl->useAcelpNext = useAcelpNext;

  determineNextWindowSequence(
      blSwControl,
      granuleLength,
      codecType,
      preSapFrame,
      useLowOverlapBlockSwitching,
      useAcelpPrev,
      useAcelp,
      useAcelpNext,
      noShortBlocks);

  determineGroupingForCurrentFrame(blSwControl);

  return 1;
}

int iisaacfenc_SyncBlockSwitching(BLOCK_SWITCHING_CONTROL *blSwControlLeft,
                                  BLOCK_SWITCHING_CONTROL *blSwControlRight,
                                  AACENC_CODEC_TYPE codecType,
                                  const int useStereoLpd,
                                  int *commonWindow) {
  int i;
  (void)codecType;

  if (useStereoLpd) {
    assert(0 && "useStereoLpd only allowed with MPEG-H.");
  } else {
    if (blSwControlLeft->useAcelpNext || blSwControlRight->useAcelpNext ||
        blSwControlLeft->useAcelp || blSwControlRight->useAcelp ||
        blSwControlLeft->useAcelpPrev || blSwControlRight->useAcelpPrev) {
      *commonWindow = 0;
      return 0;
    }
  }

  if ((blSwControlLeft->nextWindowSequence != SHORT_WINDOW) &&
      (blSwControlRight->nextWindowSequence == SHORT_WINDOW)) {
    if ((blSwControlLeft->attackDetection.maxAccNrgFRatio > blSwControlRight->syncRatio * blSwControlRight->attackDetection.attackRatio) ||
        (blSwControlLeft->attackDetection.maxWinNrgFRatio > blSwControlRight->attackDetection.attackRatio)) {
      blSwControlLeft->attackDetection.nextAttack = 1;
      blSwControlLeft->nextWindowSequence = SHORT_WINDOW;
      if (blSwControlLeft->windowSequence == LONG_WINDOW) {
        blSwControlLeft->windowSequence = START_WINDOW;
      } else if (blSwControlLeft->windowSequence == STOP_WINDOW) {
        if (blSwControlRight->windowSequence == STOPSTART_WINDOW) {
          blSwControlLeft->windowSequence = STOPSTART_WINDOW;
        } else {
          blSwControlLeft->windowSequence = SHORT_WINDOW;
          blSwControlLeft->noOfGroups = MAX_NO_OF_GROUPS;
          if ((blSwControlLeft->prevWindowSequence == SHORT_WINDOW) && (blSwControlLeft->syncRatio == 0.0f)) {
            blSwControlLeft->noOfGroups >>= 1;
            for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
              blSwControlLeft->groupLen[i] = lowRateGroupingTable[blSwControlLeft->attackDetection.attackIndex][i];
            }
          } else
            for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
              blSwControlLeft->groupLen[i] = suggestedGroupingTable[blSwControlLeft->attackDetection.attackIndex][i];
            }
        }
      }
    } else if ((blSwControlLeft->windowSequence == SHORT_WINDOW) && (blSwControlLeft->attackDetection.attackIndex == TRANS_FAC - 1) &&
               (blSwControlRight->windowSequence == SHORT_WINDOW) && (blSwControlRight->attackDetection.attackIndex == TRANS_FAC - 1)) {
      blSwControlLeft->attackDetection.nextAttack = 1;
      blSwControlLeft->attackDetection.nextAttackIndex = 0;
      blSwControlLeft->nextWindowSequence = SHORT_WINDOW;
    }
  }

  else if ((blSwControlLeft->nextWindowSequence == SHORT_WINDOW) &&
           (blSwControlRight->nextWindowSequence != SHORT_WINDOW)) {
    if ((blSwControlRight->attackDetection.maxAccNrgFRatio > blSwControlLeft->syncRatio * blSwControlLeft->attackDetection.attackRatio) ||
        (blSwControlRight->attackDetection.maxWinNrgFRatio > blSwControlLeft->attackDetection.attackRatio)) {
      blSwControlRight->attackDetection.nextAttack = 1;
      blSwControlRight->nextWindowSequence = SHORT_WINDOW;
      if (blSwControlRight->windowSequence == LONG_WINDOW) {
        blSwControlRight->windowSequence = START_WINDOW;
      } else if (blSwControlRight->windowSequence == STOP_WINDOW) {
        if (blSwControlLeft->windowSequence == STOPSTART_WINDOW) {
          blSwControlRight->windowSequence = STOPSTART_WINDOW;
        } else {
          blSwControlRight->windowSequence = SHORT_WINDOW;
          blSwControlRight->noOfGroups = MAX_NO_OF_GROUPS;
          if ((blSwControlRight->prevWindowSequence == SHORT_WINDOW) && (blSwControlRight->syncRatio == 0.0f)) {
            blSwControlRight->noOfGroups >>= 1;
            for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
              blSwControlRight->groupLen[i] = lowRateGroupingTable[blSwControlRight->attackDetection.attackIndex][i];
            }
          } else
            for (i = 0; i < MAX_NO_OF_GROUPS; i++) {
              blSwControlRight->groupLen[i] = suggestedGroupingTable[blSwControlRight->attackDetection.attackIndex][i];
            }
        }
      }
    } else if ((blSwControlLeft->windowSequence == SHORT_WINDOW) && (blSwControlLeft->attackDetection.attackIndex == TRANS_FAC - 1) &&
               (blSwControlRight->windowSequence == SHORT_WINDOW) && (blSwControlRight->attackDetection.attackIndex == TRANS_FAC - 1)) {
      blSwControlRight->attackDetection.nextAttack = 1;
      blSwControlRight->attackDetection.nextAttackIndex = 0;
      blSwControlRight->nextWindowSequence = SHORT_WINDOW;
    }
  }

  else if ((blSwControlLeft->nextWindowSequence == LONG_WINDOW) &&
           (blSwControlRight->nextWindowSequence == STOP_WINDOW)) {
    if ((blSwControlLeft->windowSequence == STOP_WINDOW) &&
        (blSwControlRight->windowSequence == START_WINDOW)) {
      blSwControlRight->windowSequence = LONG_WINDOW;
      blSwControlRight->nextWindowSequence = LONG_WINDOW;
    } else if ((blSwControlLeft->windowSequence == STOP_WINDOW) && (blSwControlRight->windowSequence == STOPSTART_WINDOW)) {
      blSwControlLeft->windowSequence = STOPSTART_WINDOW;
      blSwControlLeft->nextWindowSequence = STOP_WINDOW;
    } else if (blSwControlLeft->windowSequence == LONG_WINDOW) {
      blSwControlLeft->windowSequence = START_WINDOW;
      blSwControlLeft->nextWindowSequence = STOP_WINDOW;
    }
  }

  else if ((blSwControlLeft->nextWindowSequence == STOP_WINDOW) &&
           (blSwControlRight->nextWindowSequence == LONG_WINDOW)) {
    if ((blSwControlLeft->windowSequence == START_WINDOW) &&
        (blSwControlRight->windowSequence == STOP_WINDOW)) {
      blSwControlLeft->windowSequence = LONG_WINDOW;
      blSwControlLeft->nextWindowSequence = LONG_WINDOW;
    } else if ((blSwControlLeft->windowSequence == STOPSTART_WINDOW) && (blSwControlRight->windowSequence == STOP_WINDOW)) {
      blSwControlRight->windowSequence = STOPSTART_WINDOW;
      blSwControlRight->nextWindowSequence = STOP_WINDOW;
    } else if (blSwControlRight->windowSequence == LONG_WINDOW) {
      blSwControlRight->windowSequence = START_WINDOW;
      blSwControlRight->nextWindowSequence = STOP_WINDOW;
    }
  }

  else if ((blSwControlLeft->nextWindowSequence == LONG_WINDOW || blSwControlLeft->nextWindowSequence == STOP_WINDOW) && (blSwControlRight->nextWindowSequence == STOPSTART_WINDOW) && (blSwControlLeft->syncRatio == 0.0f)) {
    blSwControlLeft->nextWindowSequence = STOPSTART_WINDOW;
    if (blSwControlLeft->windowSequence == LONG_WINDOW) {
      blSwControlLeft->windowSequence = START_WINDOW;
    } else if (blSwControlLeft->windowSequence == STOP_WINDOW) {
      blSwControlLeft->windowSequence = STOPSTART_WINDOW;
    }
  }

  else if ((blSwControlRight->nextWindowSequence == LONG_WINDOW || blSwControlRight->nextWindowSequence == STOP_WINDOW) && (blSwControlLeft->nextWindowSequence == STOPSTART_WINDOW) && (blSwControlRight->syncRatio == 0.0f)) {
    blSwControlRight->nextWindowSequence = STOPSTART_WINDOW;
    if (blSwControlRight->windowSequence == LONG_WINDOW) {
      blSwControlRight->windowSequence = START_WINDOW;
    } else if (blSwControlRight->windowSequence == STOP_WINDOW) {
      blSwControlRight->windowSequence = STOPSTART_WINDOW;
    }
  }

  if (blSwControlLeft->windowSequence != blSwControlRight->windowSequence) {
    *commonWindow = 0;
  }

  if ((*commonWindow) && (blSwControlLeft->windowSequence == SHORT_WINDOW)) {
    if (blSwControlLeft->attackDetection.lastMaxAccNrgFRatio > blSwControlRight->attackDetection.lastMaxAccNrgFRatio) {
      blSwControlRight->noOfGroups = blSwControlLeft->noOfGroups;
      for (i = 0; i < TRANS_FAC; i++) {
        blSwControlRight->groupLen[i] = blSwControlLeft->groupLen[i];
      }
    } else {
      blSwControlLeft->noOfGroups = blSwControlRight->noOfGroups;
      for (i = 0; i < TRANS_FAC; i++) {
        blSwControlLeft->groupLen[i] = blSwControlRight->groupLen[i];
      }
    }
  }

  return 1;
}

int iisaacfenc_WindowShaping(BLOCK_SWITCHING_CONTROL *blSwControl,
                             AACENC_CODEC_TYPE codecType,
                             const int forceKBDWindow,
                             const int isLFE) {
  blSwControl->prevWindowShape = blSwControl->windowShape;

  if (isLFE) {
    switch (codecType) {
      case AACENC_CODEC_XHEAAC:
        blSwControl->windowShape = KBD_WINDOW;
        break;
      case AACENC_CODEC_AAC:
      default:
        blSwControl->windowShape = SINE_WINDOW;
        break;
    }
    blSwControl->windowKernelNum = MDCT_IV;
    return 0;
  }

  switch (blSwControl->windowSequence) {
    case LONG_WINDOW:
    case STOP_WINDOW:
      if ((forceKBDWindow) ||
          (blSwControl->attackDetection.maxWinNrgFRatio > 2.875f) || (blSwControl->attackDetection.lastMaxWinNrgFRatio > 2.875f) ||
          (blSwControl->attackDetection.minWinNrgFRatio < 0.125f) || (blSwControl->attackDetection.lastMinWinNrgFRatio < 0.125f)) {
        blSwControl->windowShape = KBD_WINDOW;
      } else {
        blSwControl->windowShape = SINE_WINDOW;
      }
      break;
    case SHORT_WINDOW:
    case START_WINDOW:
    case STOPSTART_WINDOW:
      if ((codecType == AACENC_CODEC_XHEAAC) &&
          (blSwControl->useAcelpNext)) {
        blSwControl->windowShape = SINE_WINDOW;
      } else if (
          (blSwControl->attackDetection.maxWinNrgFRatio > 768.0f) || (blSwControl->attackDetection.lastMaxWinNrgFRatio > 768.0f) ||
          (blSwControl->attackDetection.minWinNrgFRatio < 0.001f) || (blSwControl->attackDetection.lastMinWinNrgFRatio < 0.001f)) {
        blSwControl->windowShape = KBD_WINDOW;
      } else {
        blSwControl->windowShape = SINE_WINDOW;
      }
      break;
    default:
      assert(0);
      break;
  }
  return 1;
}

int iisaacfenc_SyncWindowShaping(BLOCK_SWITCHING_CONTROL *blSwControlLeft,
                                 BLOCK_SWITCHING_CONTROL *blSwControlRight,
                                 const int commonWindow) {
  if ((commonWindow) &&
      (blSwControlLeft->windowShape == KBD_WINDOW || blSwControlRight->windowShape == KBD_WINDOW)) {
    blSwControlLeft->windowShape = blSwControlRight->windowShape = KBD_WINDOW;
  }

  return 1;
}

void iisaacfenc_SetBlockSwitchingNoStartStop(BLOCK_SWITCHING_CONTROL *blSwControl,
                                             const int noStartStopSequence) {
  if (blSwControl != NULL) {
    blSwControl->noStartStopSequence = noStartStopSequence;
  }
}
