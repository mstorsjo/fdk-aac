
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
#include <math.h>
#include <float.h>

#include "iisSigMap.h"
#include "qc_data.h"
#include "adj_thr_data.h"
#include "glob_con.h"
#include "psy_const.h"
#include "adj_thr.h"
#include "iisBitFrame.h"
#include "mathlib.h"
#include "line_pe.h"
#include "iisutillib.h"

#define MIN_SNR_LIMIT 0.8f
#define MIN_SNR_LIMIT_LR MIN_SNR_LIMIT* MIN_SNR_LIMIT
#define NUM_NRG_LEVS 8

enum iisaacfenc_avoid_hole_state {
  iisaacfenc_NO_AH = 0,
  iisaacfenc_AH_INACTIVE = 1,
  iisaacfenc_AH_ACTIVE = 2
};

float iisaacfenc_bits2pe(const float bits) {
  const float b2pe = 1.18f;

  return (bits * b2pe);
}

static void iisaacfenc_reduceMinSnrAllChannels(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                               CHANNEL_MAPPING* cm,
                                               ATS_ELEMENT* adjThrElem[SIGMAP_MAX_ELEMENTS],
                                               const int nElements,
                                               float* reducedPe,
                                               const float desiredPe);

static void iisaacfenc_allowMoreHoles(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                      CHANNEL_MAPPING* cm,
                                      ATS_ELEMENT* adjThrElem[SIGMAP_MAX_ELEMENTS],
                                      PSY_OUT_ELEMENT* psyOutElem[SIGMAP_MAX_ELEMENTS],
                                      const int nElements,
                                      const float desiredPe,
                                      float* currentPe);

static void iisaacfenc_calcThreshExp(float thrExp[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                     PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                     const int nChannels) {
  int ch, sfb;
  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      thrExp[ch][sfb] = (float)pow(psyOutChan->sfbThreshold[sfb], 0.25f);
    }
  }
}

static void iisaacfenc_adaptMinSnr(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                   float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                   const int nChannels,
                                   const int commonWindow) {
  int ch, sfb;
  float avgEnergy[SIGMAP_MAX_SIGNALS_PER_ELEMENT], commonAvgEnergy = 0.0f;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    avgEnergy[ch] = 0.0f;

    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      avgEnergy[ch] += psyOutChan->sfbEnergy[sfb];
    }
    if (sfb > 0) {
      if ((psyOutChan->windowSequence == SHORT_WINDOW) && (psyOutChan->maxSfbPerGroup > 0)) {
        avgEnergy[ch] *= (float)psyOutChan->sfbOffsets[psyOutChan->sfbPerGroup] /
                         (float)(psyOutChan->sfbOffsets[sfb] * psyOutChan->sfbOffsets[psyOutChan->maxSfbPerGroup]);
      } else {
        avgEnergy[ch] /= (float)psyOutChan->sfbOffsets[sfb];
      }
    }
    commonAvgEnergy += avgEnergy[ch];
  }
  commonAvgEnergy /= (float)nChannels;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    const float energyThresh = (commonWindow ? commonAvgEnergy : avgEnergy[ch]) + FLT_MIN;
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      const float energy = psyOutChan->sfbEnergy[sfb] + FLT_MIN;
      const float thresh = psyOutChan->sfbThreshold[sfb];

      if ((thresh < weightedEn[ch][sfb]) && (energy < energyThresh) && (psyOutChan->sfbMinSnr[sfb] < MIN_SNR_LIMIT)) {
        psyOutChan->sfbMinSnr[sfb] *= energyThresh / energy;
        if (psyOutChan->sfbMinSnr[sfb] > MIN_SNR_LIMIT) {
          psyOutChan->sfbMinSnr[sfb] = MIN_SNR_LIMIT;
        }
      }
    }
    if ((commonWindow) && (psyOutChan->windowSequence == SHORT_WINDOW)) {
      avgEnergy[0] = avgEnergy[1] = 0.125f;
      for (sfb = 0; sfb < psyOutChan->sfbActive; sfb += psyOutChan->sfbPerGroup) {
        avgEnergy[0] += psyOutChan->sfbEnergy[sfb];
        avgEnergy[1] += psyOutChan->sfbEnergy[sfb + 1];
      }
      sfb = psyOutChan->sfbActive;
    }
  }
}

static void iisaacfenc_initAvoidHoleFlag(int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                         PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                         PSY_OUT_ELEMENT* psyOutElement,
                                         float weightedEnergy[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                         const int nChannels) {
  int ch, sfb;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    setINT(iisaacfenc_NO_AH, ahFlag[ch], MAX_GROUPED_SFB);

    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      if (psyOutChan->sfbThreshold[sfb] < weightedEnergy[ch][sfb] * 0.00251786f) {
        ahFlag[ch][sfb] = iisaacfenc_AH_INACTIVE;
      }
    }

    if ((sfb > 0) && (psyOutChan->sfbOffsets[sfb] == psyOutChan->sfbOffsets[sfb - 1] + 96)) {
      ahFlag[ch][sfb - 1] = iisaacfenc_NO_AH;
    }
  }

  if ((nChannels == 2) && (psyOutElement->commonWindow)) {
    PSY_OUT_CHANNEL* psyOutChan0 = psyOutChannel[0];
    PSY_OUT_CHANNEL* psyOutChan1 = psyOutChannel[1];
    const int maxSfbActive = max(psyOutChan0->sfbActive, psyOutChan1->sfbActive);

    for (sfb = 0; sfb < maxSfbActive; sfb++) {
      if (psyOutElement->toolsInfo.bUniSte) {
        ahFlag[0][sfb] = ahFlag[1][sfb] = iisaacfenc_AH_INACTIVE;
      }

      if (psyOutElement->toolsInfo.msMask[sfb] == MS_ON || psyOutElement->toolsInfo.bUniSte) {
        const float sfbEnM = psyOutChan0->sfbEnergy[sfb];
        const float sfbEnS = psyOutChan1->sfbEnergy[sfb];
        float maxThreshold = min(psyOutChan0->sfbMinSnr[sfb], psyOutChan1->sfbMinSnr[sfb]);
        float thrCorrFac = 1.0f;

        if (psyOutElement->toolsInfo.bCplxPredMdctActive || psyOutElement->toolsInfo.bUniSte) {
          thrCorrFac = psyOutElement->toolsInfo.invPredGain[sfb];
        }
        maxThreshold = min(maxThreshold, 0.4f * thrCorrFac) * max(sfbEnM, sfbEnS);

        if ((ahFlag[0][sfb] != iisaacfenc_NO_AH) || (ahFlag[1][sfb] != iisaacfenc_NO_AH)) {
          ahFlag[0][sfb] = ahFlag[1][sfb] = iisaacfenc_AH_INACTIVE;
        }

        if ((psyOutChan0->sfbThreshold[sfb] < weightedEnergy[0][sfb]) &&
            (psyOutChan0->sfbMinSnr[sfb] * sfbEnM < maxThreshold)) {
          if (psyOutElement->toolsInfo.bUniSte) {
            psyOutChan0->sfbMinSnr[sfb] = min(0.4f, maxThreshold / sfbEnM);
          } else {
            psyOutChan0->sfbMinSnr[sfb] = maxThreshold / sfbEnM;
          }
          if (psyOutChan0->sfbMinSnr[sfb] > MIN_SNR_LIMIT) {
            if (psyOutChan0->sfbMinSnr[sfb] <= 1.0f) {
              psyOutChan0->sfbMinSnr[sfb] = MIN_SNR_LIMIT;
            } else {
              ahFlag[0][sfb] = iisaacfenc_NO_AH;
            }
          }
        }
        if ((psyOutChan1->sfbThreshold[sfb] < weightedEnergy[1][sfb]) &&
            (psyOutChan1->sfbMinSnr[sfb] * sfbEnS < maxThreshold)) {
          psyOutChan1->sfbMinSnr[sfb] = maxThreshold / sfbEnS;
          if (psyOutChan1->sfbMinSnr[sfb] > MIN_SNR_LIMIT) {
            if (psyOutChan1->sfbMinSnr[sfb] <= 1.0f) {
              psyOutChan1->sfbMinSnr[sfb] = MIN_SNR_LIMIT;
            } else {
              ahFlag[1][sfb] = iisaacfenc_NO_AH;
            }
          }
        }
        if ((psyOutElement->toolsInfo.msMask[sfb] != MS_ON) && (sfb > 36)) {
          if ((psyOutChan0->sfbMinSnr[sfb] > MIN_SNR_LIMIT_LR) && (psyOutChan0->sfbMinSnr[sfb] <= MIN_SNR_LIMIT)) {
            psyOutChan0->sfbMinSnr[sfb] = MIN_SNR_LIMIT_LR;
          }
          if ((psyOutChan1->sfbMinSnr[sfb] > MIN_SNR_LIMIT_LR) && (psyOutChan1->sfbMinSnr[sfb] <= MIN_SNR_LIMIT)) {
            psyOutChan1->sfbMinSnr[sfb] = MIN_SNR_LIMIT_LR;
          }
        }
      } else {
        if (psyOutChan0->sfbMinSnr[sfb] > MIN_SNR_LIMIT_LR) {
          psyOutChan0->sfbMinSnr[sfb] = MIN_SNR_LIMIT_LR;
        }
        if (psyOutChan1->sfbMinSnr[sfb] > MIN_SNR_LIMIT_LR) {
          psyOutChan1->sfbMinSnr[sfb] = MIN_SNR_LIMIT_LR;
        }
      }
    }
  }
}

static void iisaacfenc_resetAHFlags(int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                    const int nChannels,
                                    PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT]) {
  int ch, sfb;
  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      if (ahFlag[ch][sfb] == iisaacfenc_AH_ACTIVE) {
        ahFlag[ch][sfb] = iisaacfenc_AH_INACTIVE;
      }
    }
  }
}

static void iisaacfenc_preparePe(PE_DATA* peData,
                                 float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                 PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                 PSY_OUT_ELEMENT* psyOutElement,
                                 const int vbrUsed,
                                 const int nChannels,
                                 const float peOffset) {
  int ch, sfb;
  int noShortWindowInFrame = 1;
  float chanAverageChaosMeasure = 0.0f;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];

    iisaacfenc_prepareSfbPe1(psyOutChan->sfbRelevLines,
                             psyOutChan->sfbEnergy,
                             psyOutChan->sfbThreshold,
                             psyOutChan->sfbFormFactor,
                             psyOutChan->sfbOffsets,
                             psyOutChan->sfbCnt);

    if (psyOutChan->windowSequence != SHORT_WINDOW) {
      float nLinesSum = 0.0f;

      int maxSfbPerGr = psyOutChan->maxSfbPerGroup;

      for (sfb = 0; sfb < maxSfbPerGr; sfb++) {
        nLinesSum += psyOutChan->sfbRelevLines[sfb];
      }
      if (sfb > 0) {
        nLinesSum /= (float)psyOutChan->sfbOffsets[sfb];
      }
      psyOutChan->chaosMeasure = nLinesSum;
      chanAverageChaosMeasure += psyOutChan->chaosMeasure;
    } else {
      psyOutChan->chaosMeasure = 0.75f;
      noShortWindowInFrame = 0;
    }
  }
  chanAverageChaosMeasure /= (float)nChannels;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];

    if (noShortWindowInFrame) {
      const int usePatch = ((!vbrUsed) &&
                            (psyOutElement->frameEnergyF > 64.0f) &&
                            ((psyOutElement->commonWindow ? chanAverageChaosMeasure : psyOutChan->chaosMeasure) > 0.78125f));

      if ((usePatch) && (psyOutChan->lastEnFacPatch)) {
        float nrgCoFac = FLT_MIN, nrgTotal = FLT_MIN;

        if (chanAverageChaosMeasure > 0.796875f) {
          if (chanAverageChaosMeasure > 0.8125f) {
            for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
              const float nrgSquareRoot = (float)sqrt(psyOutChan->sfbEnergy[sfb]);
              const float nrgFourthRoot = (float)sqrt(nrgSquareRoot);
              psyOutChan->sfbEnFac[sfb] = nrgSquareRoot * nrgFourthRoot;

              nrgCoFac += nrgFourthRoot;
              nrgTotal += psyOutChan->sfbEnergy[sfb];
            }
          } else {
            for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
              const float nrgSquareRoot = (float)sqrt(psyOutChan->sfbEnergy[sfb]);
              psyOutChan->sfbEnFac[sfb] = nrgSquareRoot;

              nrgCoFac += nrgSquareRoot;
              nrgTotal += psyOutChan->sfbEnergy[sfb];
            }
          }
        } else {
          for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
            const float nrgSquareRoot = (float)sqrt(psyOutChan->sfbEnergy[sfb]);
            const float nrgFourthRoot = (float)sqrt(nrgSquareRoot);
            psyOutChan->sfbEnFac[sfb] = nrgFourthRoot;

            nrgCoFac += nrgSquareRoot * nrgFourthRoot;
            nrgTotal += psyOutChan->sfbEnergy[sfb];
          }
        }
        nrgCoFac /= nrgTotal;
        nrgCoFac = (float)sqrt(nrgCoFac);
        for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
          psyOutChan->sfbEnFac[sfb] = psyOutChan->sfbEnFac[sfb] * nrgCoFac + FLT_MIN;
        }
      } else {
        for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
          psyOutChan->sfbEnFac[sfb] = 1.0f;
        }
      }
      psyOutChan->lastEnFacPatch = usePatch;
    }

    else {
      for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
        psyOutChan->sfbEnFac[sfb] = 1.0f;
      }

      psyOutChan->lastEnFacPatch = 1;

      if (psyOutChan->windowSequence == SHORT_WINDOW) {
        int grp, grpOffset = 0;
        for (grp = 0; grp < psyOutChan->noOfGroups; grp++) {
          setFLOAT(psyOutChan->groupLen[grp] * 0.125f, psyOutChan->sfbEnFac + grpOffset,
                   psyOutChan->maxSfbPerGroup);
          grpOffset += psyOutChan->sfbPerGroup;
        }
      }
    }

    if ((psyOutChan->windowSequence != SHORT_WINDOW) && (vbrUsed)) {
      float enFac = 1.0f, enAdd = 0.0f;
      for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
        if (psyOutChan->sfbOffsets[sfb + 1] == psyOutChan->sfbOffsets[sfb] + 32) {
          if (sfb == 30) {
            enAdd = 0.09375f;
          }
          psyOutChan->sfbEnFac[sfb] /= (enFac += enAdd);
        }
      }
    }

    divFLOAT(psyOutChan->sfbEnergy, psyOutChan->sfbEnFac,
             weightedEn[ch], psyOutChan->sfbActive);

    divFLOAT(psyOutChan->sfbThreshold, psyOutChan->sfbEnFac,
             psyOutChan->sfbThreshold, psyOutChan->sfbActive);

    if (!vbrUsed) {
      iisaacfenc_prepareSfbPe2(&peData->peChannelData[ch],
                               weightedEn[ch],
                               psyOutChan->sfbRelevLines,
                               psyOutChan->sfbActive);
    }
  }

  peData->offset = peOffset;
}

static void iisaacfenc_invWeightThr(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS], const int nChannels) {
  int ch;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    multFLOAT(psyOutChan->sfbThreshold, psyOutChan->sfbEnFac,
              psyOutChan->sfbThreshold, psyOutChan->sfbActive);
  }
}

static void iisaacfenc_calcPe(PE_DATA* peData,
                              PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                              float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                              const int nChannels) {
  int ch;

  peData->pe = peData->offset;
  peData->constPart = 0.0f;
  peData->nActiveLines = 0.0f;
  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    PE_CHANNEL_DATA* peChanData = &peData->peChannelData[ch];
    iisaacfenc_calcSfbPe(&peData->peChannelData[ch],
                         weightedEn[ch],
                         psyOutChan->sfbThreshold,
                         psyOutChan->sfbRelevLines,
                         psyOutChan->sfbActive,
                         psyOutChan->isBook,
                         psyOutChan->isScale);
    peData->pe += peChanData->pe;
    peData->constPart += peChanData->constPart;
    peData->nActiveLines += peChanData->nActiveLines;
  }
}

static void iisaacfenc_calcPeNoAH(float* pe,
                                  float* constPart,
                                  float* nActiveLines,
                                  PE_DATA* peData,
                                  int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                  PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                  const int nChannels) {
  int ch, sfb;

  *pe = peData->offset;
  *constPart = 0.0f;
  *nActiveLines = 0.0f;
  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    PE_CHANNEL_DATA* peChanData = &peData->peChannelData[ch];
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      if (ahFlag[ch][sfb] < iisaacfenc_AH_ACTIVE) {
        *pe += peChanData->sfbPe[sfb];
        *constPart += peChanData->sfbConstPart[sfb];
        *nActiveLines += peChanData->sfbNActiveLines[sfb];
      }
    }
  }
}

static void iisaacfenc_reduceThresholdsCBR(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                           int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           float thrExp[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           const int nChannels,
                                           const float redVal) {
  int ch, sfb;
  float sfbEn, sfbThrReduced;
  float sfbThrOrg;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      sfbEn = weightedEn[ch][sfb];
      sfbThrOrg = thrExp[ch][sfb] * thrExp[ch][sfb];
      sfbThrOrg *= sfbThrOrg;

      if ((sfbEn > sfbThrOrg) && (ahFlag[ch][sfb] != iisaacfenc_AH_ACTIVE)) {
        float tmp = thrExp[ch][sfb] + redVal;
        tmp *= tmp;
        sfbThrReduced = tmp * tmp;

        if ((sfbThrReduced > psyOutChan->sfbMinSnr[sfb] * sfbEn) &&
            (ahFlag[ch][sfb] != iisaacfenc_NO_AH)) {
          ahFlag[ch][sfb] = iisaacfenc_AH_ACTIVE;
          psyOutChan->sfbThreshold[sfb] = max(psyOutChan->sfbMinSnr[sfb] * sfbEn, psyOutChan->sfbThreshold[sfb]);
        } else {
          psyOutChan->sfbThreshold[sfb] = sfbThrReduced;
        }
      }
    }
  }
}

static void iisaacfenc_reduceThresholdsVBR(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                           PSY_OUT_ELEMENT* psyOutElement,
                                           int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           float thrExp[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                           const int nChannels,
                                           const float vbrQualFactor) {
  int ch, sfb;
  float loudnessFac[SIGMAP_MAX_SIGNALS_PER_ELEMENT], avgLoudnessFac = 0.0f, avgSpecDensity = 0.0f;

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    loudnessFac[ch] = 0.0f;

    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      loudnessFac[ch] += psyOutChan->sfbFormFactor[sfb];
    }
    avgLoudnessFac += loudnessFac[ch];
    avgSpecDensity += psyOutChan->chaosMeasure;
  }
  avgLoudnessFac /= (float)nChannels;
  avgSpecDensity /= (float)nChannels;

  if ((nChannels == 2) && (psyOutElement->commonWindow) && (avgLoudnessFac > FLT_MIN)) {
    PSY_OUT_CHANNEL* psyOutChan0 = psyOutChannel[0];
    PSY_OUT_CHANNEL* psyOutChan1 = psyOutChannel[1];
    float msCompFac = 0.0f;
    const int maxSfbActive = max(psyOutChan0->sfbActive, psyOutChan1->sfbActive);
    const float avgFormFac = 0.4375f * avgLoudnessFac / (float)psyOutChan0->sfbOffsets[maxSfbActive];

    for (sfb = 0; sfb < maxSfbActive; sfb++) {
      float sfbMsCompFac = avgFormFac * (float)(psyOutChan0->sfbOffsets[sfb + 1] - psyOutChan0->sfbOffsets[sfb]);

      if (psyOutElement->toolsInfo.msMask[sfb] == MS_ON) {
        if ((psyOutChan0->sfbEnergy[sfb] < psyOutChan1->sfbEnergy[sfb]) &&
            (psyOutChan0->sfbMinSnr[sfb] > 1.0f) && (psyOutChan0->sfbFormFactor[sfb] < sfbMsCompFac) &&
            (psyOutChan0->sfbThreshold[sfb] < weightedEn[0][sfb])) {
          sfbMsCompFac = psyOutChan0->sfbFormFactor[sfb];
        } else if ((psyOutChan1->sfbEnergy[sfb] < psyOutChan0->sfbEnergy[sfb]) &&
                   (psyOutChan1->sfbMinSnr[sfb] > 1.0f) && (psyOutChan1->sfbFormFactor[sfb] < sfbMsCompFac) &&
                   (psyOutChan1->sfbThreshold[sfb] < weightedEn[1][sfb])) {
          sfbMsCompFac = psyOutChan1->sfbFormFactor[sfb];
        }
      }
      msCompFac += sfbMsCompFac;
    }
    avgLoudnessFac *= msCompFac / (0.4375f * avgLoudnessFac);
  }

  for (ch = 0; ch < nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[ch];
    const float redVal = vbrQualFactor * (16.0f + (psyOutElement->commonWindow
                                                       ? avgLoudnessFac * avgSpecDensity
                                                       : loudnessFac[ch] * psyOutChan->chaosMeasure));
    for (sfb = 0; sfb < psyOutChan->sfbActive; sfb++) {
      const float sfbEnergy = weightedEn[ch][sfb];
      const float sfbThresh = psyOutChan->sfbThreshold[sfb];

      if (sfbEnergy > sfbThresh) {
        float sfbThreshRed;

        float tmp = thrExp[ch][sfb] + redVal;
        tmp *= tmp;
        sfbThreshRed = tmp * tmp;

        if ((sfbThreshRed > psyOutChan->sfbMinSnr[sfb] * sfbEnergy) &&
            (ahFlag[ch][sfb] != iisaacfenc_NO_AH)) {
          ahFlag[ch][sfb] = iisaacfenc_AH_ACTIVE;
          psyOutChan->sfbThreshold[sfb] = max(psyOutChan->sfbMinSnr[sfb] * sfbEnergy, sfbThresh);
        } else {
          psyOutChan->sfbThreshold[sfb] = sfbThreshRed;
        }
      }
    }
  }
}

static void iisaacfenc_correctThreshAllChannels(CHANNEL_MAPPING* cm,
                                                PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                                ADJ_THR_STATE* adjThr,
                                                const float redVal,
                                                const float deltaPe,
                                                const int processElements,
                                                const int elementOffset) {
  int ch, i, elementId, channelId, nChannels;
  PSY_OUT_CHANNEL* psyOutChan[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  PE_CHANNEL_DATA* peChanData;
  PE_DATA* peData;
  ATS_ELEMENT* adjThrElement;
  float deltaSfbPe;
  float thrFactor;
  float normFactor;
  float sfbEn, sfbThr, sfbThrReduced;
  int nElements = elementOffset + processElements;

  normFactor = FLT_MIN;

  for (elementId = elementOffset; elementId < nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      adjThrElement = adjThr->adjThrStateElem[elementId];
      peData = &adjThrElement->peData;
      nChannels = cm->elInfo[elementId].nChannelsInEl;

      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChan[channelId] = psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
      }

      for (ch = 0; ch < nChannels; ch++) {
        peChanData = &peData->peChannelData[ch];
        for (i = 0; i < psyOutChan[ch]->sfbActive; i++) {
          if ((adjThrElement->ahFlag[ch][i] < iisaacfenc_AH_ACTIVE) || (deltaPe > 0)) {
            float sum = adjThrElement->thrExp[ch][i] + redVal;
            if (sum < 100 * 256 * nChannels * MAX_GROUPED_SFB * FLT_MIN) {
              sum = 100 * 256 * nChannels * MAX_GROUPED_SFB * FLT_MIN;
            }
            adjThrElement->sfbPeFac[ch][i] = peChanData->sfbNActiveLines[i] / sum;
            normFactor += adjThrElement->sfbPeFac[ch][i];

          } else {
            adjThrElement->sfbPeFac[ch][i] = 0.0f;
          }
        }
      }
    }
  }

  normFactor = 1.0f / normFactor;

  for (elementId = elementOffset; elementId < nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      adjThrElement = adjThr->adjThrStateElem[elementId];
      peData = &adjThrElement->peData;
      nChannels = cm->elInfo[elementId].nChannelsInEl;
      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChan[channelId] = psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
      }

      for (ch = 0; ch < nChannels; ch++) {
        peChanData = &peData->peChannelData[ch];
        for (i = 0; i < psyOutChan[ch]->sfbActive; i++) {
          if (adjThrElement->sfbPeFac[ch][i] == 0.0f) {
            deltaSfbPe = 0.0f;
          } else {
            deltaSfbPe = adjThrElement->sfbPeFac[ch][i] * normFactor * deltaPe;
          }
          if (peChanData->sfbNActiveLines[i] > 0.0f) {
            float sfbMinThr;
            float corrFac = 0.1f;
            sfbMinThr = (float)pow(adjThrElement->thrExp[ch][i], 4.f);
            sfbEn = adjThrElement->weightedEn[ch][i];
            sfbThr = psyOutChan[ch]->sfbThreshold[i];

            thrFactor = min(-deltaSfbPe / peChanData->sfbNActiveLines[i], 20.f);
            thrFactor = (float)pow(2.0f, thrFactor);
            sfbThrReduced = sfbThr * thrFactor;
            if (sfbThrReduced < corrFac * sfbMinThr) {
              sfbThrReduced = corrFac * sfbMinThr;
            }

            if ((sfbThrReduced > psyOutChan[ch]->sfbMinSnr[i] * sfbEn) &&
                (adjThrElement->ahFlag[ch][i] == iisaacfenc_AH_INACTIVE)) {
              sfbThrReduced = max(psyOutChan[ch]->sfbMinSnr[i] * sfbEn, sfbThr);
              adjThrElement->ahFlag[ch][i] = iisaacfenc_AH_ACTIVE;
            }
            psyOutChan[ch]->sfbThreshold[i] = sfbThrReduced;
          }
        }
      }
    }
  }
}

static void iisaacfenc_adaptThresholds(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT],
                                       PSY_OUT_ELEMENT* psyOutElement,
                                       const int nChannels,
                                       float weightedEn[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                       float thrExp[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                       int ahFlag[SIGMAP_MAX_SIGNALS_PER_ELEMENT][MAX_GROUPED_SFB],
                                       const float vbrQualFactor) {
  iisaacfenc_calcThreshExp(thrExp, psyOutChannel, nChannels);

  iisaacfenc_adaptMinSnr(psyOutChannel, weightedEn, nChannels, psyOutElement->commonWindow);

  iisaacfenc_initAvoidHoleFlag(ahFlag, psyOutChannel, psyOutElement, weightedEn, nChannels);

  iisaacfenc_reduceThresholdsVBR(psyOutChannel, psyOutElement, ahFlag, thrExp,
                                 weightedEn, nChannels, vbrQualFactor);
}

static void iisaacfenc_adaptThresholdsToPe(CHANNEL_MAPPING* cm,
                                           ADJ_THR_STATE* adjThr,
                                           PSY_OUT* psyOut,
                                           const float desiredPe,
                                           const int processElements,
                                           const int elementOffset) {
  float noRedPe, redPeNoAH, constPart, constPartNoAH;
  float redPe;
  float redVal;
  float constPartGlobal = 0.0f;
  float noRedPeGlobal = 0.0f;
  float redPeGlobal = 0.0f;
  float redPeNoAHGlobal = 0.0f;
  float constPartNoAHGlobal = 0.0f;
  float nActiveLinesNoAH, nActiveLinesNoAHGlobal = 0.0f;
  float desiredPeNoAHGlobal = 0.0f;
  float redPeGlobalTmp;
  float maxPeError = 0.05f;
  float nActiveLines, nActiveLinesGlobal = 0.0f;
  float avgThrExp;
  int iter, elementId, channelId, nChannels;
  ATS_ELEMENT* adjThrElement;
  PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  PSY_OUT_ELEMENT* psyOutElement;
  PE_DATA* peData;

  int nElements = elementOffset + processElements;
  if (nElements > cm->nElements) {
    nElements = cm->nElements;
  }
  maxPeError = (float)pow(maxPeError, nElements);

  for (elementId = elementOffset; elementId < nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      nChannels = cm->elInfo[elementId].nChannelsInEl;
      adjThrElement = adjThr->adjThrStateElem[elementId];
      peData = &adjThrElement->peData;
      psyOutElement = psyOut->psyOutElement[elementId];

      noRedPe = peData->pe;
      constPart = peData->constPart;
      nActiveLines = max(peData->nActiveLines, 1.0f);

      constPartGlobal += constPart;
      noRedPeGlobal += noRedPe;
      nActiveLinesGlobal += nActiveLines;

      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
      }

      iisaacfenc_calcThreshExp(adjThrElement->thrExp, psyOutChannel, nChannels);

      iisaacfenc_adaptMinSnr(psyOutChannel, adjThrElement->weightedEn, nChannels, psyOutElement->commonWindow);

      iisaacfenc_initAvoidHoleFlag(adjThrElement->ahFlag, psyOutChannel, psyOutElement, adjThrElement->weightedEn, nChannels);
    }
  }

  avgThrExp = (float)pow(2.0f, (constPartGlobal - noRedPeGlobal) / (4.0f * nActiveLinesGlobal));
  redVal = (float)pow(2.0f, (constPartGlobal - desiredPe) / (4.0f * nActiveLinesGlobal)) - avgThrExp;
  redVal = max(0.f, redVal);

  redPeGlobal = 0.0;

  for (elementId = elementOffset; elementId < nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      adjThrElement = adjThr->adjThrStateElem[elementId];
      nChannels = cm->elInfo[elementId].nChannelsInEl;
      peData = &adjThrElement->peData;

      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
      }

      iisaacfenc_reduceThresholdsCBR(psyOutChannel, adjThrElement->ahFlag, adjThrElement->thrExp, adjThrElement->weightedEn, nChannels, redVal);

      iisaacfenc_calcPe(peData, psyOutChannel, adjThrElement->weightedEn, nChannels);
      redPe = peData->pe;
      redPeGlobal += redPe;
    }
  }

  iter = 0;
  while (((float)fabs(redPeGlobal - desiredPe) > maxPeError * desiredPe) && (iter < 3)) {
    redPeGlobalTmp = 0.0f;
    redPeNoAHGlobal = 0.0f;
    constPartNoAHGlobal = 0.0f;
    nActiveLinesNoAHGlobal = 0.0f;

    for (elementId = elementOffset; elementId < nElements; elementId++) {
      if (cm->elInfo[elementId].elType != ID_DSE) {
        adjThrElement = adjThr->adjThrStateElem[elementId];
        peData = &adjThrElement->peData;
        nChannels = cm->elInfo[elementId].nChannelsInEl;
        for (channelId = 0; channelId < nChannels; channelId++) {
          psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
        }

        iisaacfenc_calcPeNoAH(&redPeNoAH, &constPartNoAH, &nActiveLinesNoAH,
                              peData, adjThrElement->ahFlag, psyOutChannel, nChannels);
        redPeNoAHGlobal += redPeNoAH;
        constPartNoAHGlobal += constPartNoAH;
        nActiveLinesNoAHGlobal += nActiveLinesNoAH;
      }
    }

    if (desiredPe < redPeGlobal) {
      desiredPeNoAHGlobal = desiredPe - (redPeGlobal - redPeNoAHGlobal);

      desiredPeNoAHGlobal = max(desiredPeNoAHGlobal, 0.0f);

      if (nActiveLinesNoAHGlobal > 0.0f) {
        avgThrExp = (float)pow(2.0f, (constPartNoAHGlobal - redPeNoAHGlobal) /
                                         (4.0f * nActiveLinesNoAHGlobal));
        redVal += (float)pow(2.0f, (constPartNoAHGlobal - desiredPeNoAHGlobal) /
                                       (4.0f * nActiveLinesNoAHGlobal)) -
                  avgThrExp;
        redVal = max(0.0f, redVal);
      }
    } else {
      redVal *= redPeGlobal / desiredPe;
      for (elementId = elementOffset; elementId < nElements; elementId++) {
        if (cm->elInfo[elementId].elType != ID_DSE) {
          nChannels = cm->elInfo[elementId].nChannelsInEl;
          for (channelId = 0; channelId < nChannels; channelId++) {
            psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
          }
          iisaacfenc_resetAHFlags(adjThr->adjThrStateElem[elementId]->ahFlag, nChannels, psyOutChannel);
        }
      }
    }

    for (elementId = elementOffset; elementId < nElements; elementId++) {
      if (cm->elInfo[elementId].elType != ID_DSE) {
        nChannels = cm->elInfo[elementId].nChannelsInEl;
        adjThrElement = adjThr->adjThrStateElem[elementId];
        peData = &adjThrElement->peData;
        for (channelId = 0; channelId < nChannels; channelId++) {
          psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
        }

        iisaacfenc_reduceThresholdsCBR(psyOutChannel, adjThrElement->ahFlag, adjThrElement->thrExp, adjThrElement->weightedEn, nChannels, redVal);

        iisaacfenc_calcPe(peData, psyOutChannel, adjThrElement->weightedEn, nChannels);
        redPe = peData->pe;
        redPeGlobalTmp += redPe;
      }
    }

    redPeGlobal = redPeGlobalTmp;
    iter++;
  }

  if (redPeGlobal < 1.15f * desiredPe) {
    iisaacfenc_correctThreshAllChannels(cm, psyOut->psyOutChannel, adjThr, redVal, desiredPe - redPeGlobal, processElements, elementOffset);
  } else {
    iisaacfenc_reduceMinSnrAllChannels(psyOut->psyOutChannel, cm, adjThr->adjThrStateElem, nElements,
                                       &redPeGlobal, desiredPe);

    iisaacfenc_allowMoreHoles(psyOut->psyOutChannel, cm, adjThr->adjThrStateElem, psyOut->psyOutElement, nElements, desiredPe, &redPeGlobal);
  }

  for (elementId = elementOffset; elementId < nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      adjThrElement = adjThr->adjThrStateElem[elementId];
      peData = &adjThrElement->peData;
      nChannels = cm->elInfo[elementId].nChannelsInEl;

      for (channelId = 0; channelId < nChannels; channelId++) {
        psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
      }

      iisaacfenc_calcPe(peData, psyOutChannel, adjThrElement->weightedEn, nChannels);
    }
  }
}

static void iisaacfenc_allowMoreHoles(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                      CHANNEL_MAPPING* cm,
                                      ATS_ELEMENT* adjThrElem[SIGMAP_MAX_ELEMENTS],
                                      PSY_OUT_ELEMENT* psyOutElem[SIGMAP_MAX_ELEMENTS],
                                      const int nElements,
                                      const float desiredPe,
                                      float* currentPe) {
  int el, ch, sfb, sfbOffs;
  float actPe;
  PE_DATA* peData;
  ATS_ELEMENT* adjThrElement;

  actPe = *currentPe;

  if (actPe > desiredPe) {
    for (el = 0; el < nElements; el++) {
      if (cm->elInfo[el].elType != ID_DSE && cm->elInfo[el].nChannelsInEl == 2 &&
          psyOutElem[el]->commonWindow) {
        PSY_OUT_CHANNEL* psyOutChanL = psyOutChannel[cm->elInfo[el].ChannelIndex[0]];
        PSY_OUT_CHANNEL* psyOutChanR = psyOutChannel[cm->elInfo[el].ChannelIndex[1]];
        const int sfbMin = (MAX_NO_OF_GROUPS + 1 - psyOutChanL->noOfGroups) >> 1;
        adjThrElement = adjThrElem[el];
        peData = &adjThrElement->peData;
        for (sfbOffs = psyOutChanL->maxSfbPerGroup - 1; sfbOffs > sfbMin; sfbOffs--) {
          for (sfb = sfbOffs; sfb < psyOutChanL->sfbActive; sfb += psyOutChanL->sfbPerGroup) {
            if (psyOutElem[el]->toolsInfo.msMask[sfb]) {
              if (adjThrElement->ahFlag[1][sfb] != iisaacfenc_NO_AH &&
                  psyOutChanL->sfbMinSnr[sfb] * psyOutChanL->sfbEnergy[sfb] > psyOutChanR->sfbEnergy[sfb]) {
                adjThrElement->ahFlag[1][sfb] = iisaacfenc_NO_AH;
                psyOutChanR->sfbThreshold[sfb] = 2.0f * adjThrElement->weightedEn[1][sfb];
                actPe -= peData->peChannelData[1].sfbPe[sfb];
                peData->peChannelData[1].sfbPe[sfb] = 0.0f;
              }

              else if (adjThrElement->ahFlag[0][sfb] != iisaacfenc_NO_AH &&
                       psyOutChanR->sfbMinSnr[sfb] * psyOutChanR->sfbEnergy[sfb] > psyOutChanL->sfbEnergy[sfb]) {
                adjThrElement->ahFlag[0][sfb] = iisaacfenc_NO_AH;
                psyOutChanL->sfbThreshold[sfb] = 2.0f * adjThrElement->weightedEn[0][sfb];
                actPe -= peData->peChannelData[0].sfbPe[sfb];
                peData->peChannelData[0].sfbPe[sfb] = 0.0f;
              }
            }
          }
          if (actPe <= desiredPe) {
            break;
          }
        }
      }
      if (actPe <= desiredPe) {
        break;
      }
    }
  }

  if (actPe > desiredPe) {
    for (el = 0; el < nElements; el++) {
      if (cm->elInfo[el].elType != ID_DSE) {
        adjThrElement = adjThrElem[el];
        peData = &adjThrElement->peData;
        for (ch = cm->elInfo[el].nChannelsInEl - 1; ch >= 0; ch--) {
          PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[cm->elInfo[el].ChannelIndex[ch]];

          for (sfbOffs = psyOutChan->maxSfbPerGroup - 1; sfbOffs > 0; sfbOffs--) {
            for (sfb = sfbOffs; sfb < psyOutChan->sfbActive; sfb += psyOutChan->sfbPerGroup) {
              if (adjThrElement->ahFlag[ch][sfb] != iisaacfenc_NO_AH &&
                  adjThrElement->weightedEn[ch][sfb] > psyOutChan->sfbThreshold[sfb]) {
                const float oldPe = peData->peChannelData[ch].sfbPe[sfb];

                psyOutChan->sfbMinSnr[sfb] = 0.9f;
                psyOutChan->sfbThreshold[sfb] = adjThrElement->weightedEn[ch][sfb] * psyOutChan->sfbMinSnr[sfb];

                peData->peChannelData[ch].sfbPe[sfb] = psyOutChan->sfbRelevLines[sfb] * 1.40625f;
                actPe += peData->peChannelData[ch].sfbPe[sfb] - oldPe;
                peData->pe += peData->peChannelData[ch].sfbPe[sfb] - oldPe;
              }
            }
          }
          if (actPe <= desiredPe) {
            break;
          }
        }
      }
      if (actPe <= desiredPe) {
        break;
      }
    }
  }

  if (actPe > desiredPe) {
    int startSfb[SIGMAP_MAX_SIGNALS];
    float* sfbEnergy[SIGMAP_MAX_SIGNALS];
    float en[NUM_NRG_LEVS];
    float avgEn = 0.f, minEn = FLT_MAX;
    int ahCnt = 0, enIdx = 0;
    int minSfb = MAX_SFB, maxSfb = 0, done = 0;

    for (el = 0; el < nElements; el++) {
      if (cm->elInfo[el].elType != ID_DSE) {
        adjThrElement = adjThrElem[el];

        for (ch = 0; ch < cm->elInfo[el].nChannelsInEl; ch++) {
          const int chIdx = cm->elInfo[el].ChannelIndex[ch];
          PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[chIdx];

          maxSfb = max(maxSfb, psyOutChan->sfbActive);

          if (psyOutChan->windowSequence != SHORT_WINDOW) {
            startSfb[chIdx] = adjThrElement->ahParam.startSfbL;
          } else {
            startSfb[chIdx] = adjThrElement->ahParam.startSfbS;
          }
          minSfb = min(minSfb, startSfb[chIdx]);

          sfbEnergy[chIdx] = adjThrElement->weightedEn[ch];

          for (sfb = startSfb[chIdx]; sfb < psyOutChan->sfbActive; sfb++) {
            if (adjThrElement->ahFlag[ch][sfb] != iisaacfenc_NO_AH &&
                sfbEnergy[chIdx][sfb] > psyOutChan->sfbThreshold[sfb]) {
              minEn = min(minEn, sfbEnergy[chIdx][sfb]);
              avgEn += sfbEnergy[chIdx][sfb];
              ahCnt++;
            }
          }
        }
      }
    }
    avgEn = (ahCnt > 0) ? (avgEn / ahCnt) : FLT_MAX;

    for (enIdx = 0; enIdx < NUM_NRG_LEVS; enIdx++) {
      en[enIdx] = minEn * (float)pow(avgEn / (minEn + FLT_MIN), (2.0f * enIdx + 1.0f) / (2.0f * NUM_NRG_LEVS - 1.0f));
    }

    done = 0;
    enIdx = 0;
    sfb = maxSfb;

    while (!done) {
      for (el = 0; el < nElements; el++) {
        adjThrElement = adjThrElem[el];
        if (cm->elInfo[el].elType == ID_DSE || actPe <= 1.09375f * desiredPe) {
          continue;
        }
        peData = &adjThrElement->peData;
        for (ch = 0; ch < cm->elInfo[el].nChannelsInEl; ch++) {
          const int chIdx = cm->elInfo[el].ChannelIndex[ch];
          PSY_OUT_CHANNEL* psyOutChan = psyOutChannel[chIdx];

          if (sfb >= startSfb[chIdx] && sfb < psyOutChan->sfbActive) {
            if (adjThrElement->ahFlag[ch][sfb] != iisaacfenc_NO_AH &&
                sfbEnergy[chIdx][sfb] < en[enIdx]) {
              adjThrElement->ahFlag[ch][sfb] = iisaacfenc_NO_AH;
              psyOutChan->sfbThreshold[sfb] = 2.0f * sfbEnergy[chIdx][sfb];
              actPe -= peData->peChannelData[ch].sfbPe[sfb];
              peData->pe -= peData->peChannelData[ch].sfbPe[sfb];
              peData->peChannelData[ch].sfbPe[sfb] = 0.0f;
            }
            if (actPe <= 1.09375f * desiredPe) {
              done = 1;
              break;
            }
          }
        }
        if (done) {
          break;
        }
      }

      sfb--;
      if (sfb < minSfb) {
        sfb = maxSfb;
        enIdx++;
        if (enIdx >= NUM_NRG_LEVS) {
          done = 1;
        }
      }
    }
  }

  *currentPe = actPe;
}

static int iisaacfenc_getSFB(PSY_OUT_CHANNEL* psyOutChannel,
                             const int position) {
  if (psyOutChannel) {
    int thisPos = position;

    if (psyOutChannel->noOfGroups > 1) {
      thisPos = (int)((float)thisPos * (float)psyOutChannel->groupLen[0] / (float)TRANS_FAC);
    }

    if (thisPos > psyOutChannel->granuleLength || thisPos < 0) {
      return -1;
    } else {
      int index;
      for (index = psyOutChannel->sfbActive; index >= 0; index--) {
        if (psyOutChannel->sfbOffsets[index] <= thisPos)
          return index;
      }
      return psyOutChannel->sfbActive - 1;
    }
  } else
    return -1;
}

static void iisaacfenc_reduceMinSnrAllChannels(PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                               CHANNEL_MAPPING* cm,
                                               ATS_ELEMENT* adjThrElem[SIGMAP_MAX_ELEMENTS],
                                               const int nElements,
                                               float* reducedPe,
                                               const float desiredPe) {
  float deltaPe;

  float newGlobalPe = 0.0;
  int previousSFB[SIGMAP_MAX_SIGNALS][2];
  int nextSFBat[2];
  int ch, grp, sfb, lineFreqIdx, specPos, nChannelsEl, el, channelIndex;
  int long_updated = 0, short_updated = 0, blockTypeIndex;
  ATS_ELEMENT* adjThrElement;
  PE_DATA* peData;
  PSY_OUT_CHANNEL* psyOutChan;

  for (el = 0; el < nElements; el++) {
    if (cm->elInfo[el].elType != ID_DSE) {
      newGlobalPe += adjThrElem[el]->peData.pe;
    }
  }

  for (ch = 0; ch < SIGMAP_MAX_SIGNALS; ch++) {
    previousSFB[ch][0] = previousSFB[ch][1] = -1;
  }

  nextSFBat[0] = nextSFBat[1] = psyOutChannel[0]->granuleLength;

  lineFreqIdx = psyOutChannel[0]->granuleLength - 4;

  while (newGlobalPe > desiredPe && lineFreqIdx > 0) {
    if (lineFreqIdx <= nextSFBat[0] || lineFreqIdx <= nextSFBat[1]) {
      long_updated = short_updated = 0;
      specPos = lineFreqIdx;
      for (el = 0; el < nElements; el++) {
        if (cm->elInfo[el].elType != ID_DSE) {
          nChannelsEl = cm->elInfo[el].nChannelsInEl;
          adjThrElement = adjThrElem[el];
          peData = &adjThrElement->peData;

          for (ch = 0; ch < nChannelsEl; ch++) {
            channelIndex = cm->elInfo[el].ChannelIndex[ch];
            psyOutChan = psyOutChannel[channelIndex];

            sfb = iisaacfenc_getSFB(psyOutChan, specPos);

            if (psyOutChan->noOfGroups > 1) {
              blockTypeIndex = 0;
            } else {
              blockTypeIndex = 1;
            }

            if (sfb >= 0) {
              if (sfb < psyOutChan->maxSfbPerGroup) {
                deltaPe = 0.0;

                for (grp = 0; grp < psyOutChan->noOfGroups; grp++) {
                  int thisSfb = sfb + grp * psyOutChan->sfbPerGroup;

                  if (adjThrElement->weightedEn[ch][thisSfb] > psyOutChan->sfbThreshold[thisSfb] &&
                      adjThrElement->ahFlag[ch][thisSfb] != iisaacfenc_NO_AH &&
                      psyOutChan->sfbMinSnr[thisSfb] < MIN_SNR_LIMIT &&
                      previousSFB[channelIndex][blockTypeIndex] != sfb) {
                    psyOutChan->sfbMinSnr[thisSfb] = MIN_SNR_LIMIT;
                    psyOutChan->sfbThreshold[thisSfb] = adjThrElement->weightedEn[ch][thisSfb] * psyOutChan->sfbMinSnr[thisSfb];

                    deltaPe += psyOutChan->sfbRelevLines[thisSfb] * 1.5f -
                               peData->peChannelData[ch].sfbPe[thisSfb];
                    peData->peChannelData[ch].sfbPe[thisSfb] = psyOutChan->sfbRelevLines[thisSfb] * 1.5f;
                  }
                }

                previousSFB[channelIndex][blockTypeIndex] = sfb;

                peData->pe += deltaPe;
                peData->peChannelData[ch].pe += deltaPe;
                newGlobalPe += deltaPe;
                *reducedPe += deltaPe;
              }

              if (psyOutChan->noOfGroups > 1 && !short_updated) {
                if (sfb > 0) {
                  nextSFBat[0] = (int)(((float)psyOutChan->sfbOffsets[sfb - 1] * (float)TRANS_FAC) /
                                       (float)psyOutChan->groupLen[0]);
                  short_updated = 1;
                } else {
                  nextSFBat[0] = 0;
                }
              } else if (psyOutChan->noOfGroups == 1 && !long_updated) {
                if (sfb > 0) {
                  nextSFBat[1] = psyOutChan->sfbOffsets[sfb - 1];
                  long_updated = 1;
                } else {
                  nextSFBat[1] = 0;
                }
              }
            }

            if (newGlobalPe <= desiredPe)
              break;
          }
        }
        if (newGlobalPe <= desiredPe)
          break;
      }
    }

    lineFreqIdx = max(nextSFBat[0] * short_updated, nextSFBat[1] * long_updated);
  }
}

static float iisaacfenc_calcBitSave(float fillLevel,
                                    const float clipLow,
                                    const float clipHigh,
                                    const float minBitSave,
                                    const float maxBitSave) {
  float bitsave;

  fillLevel = max(fillLevel, clipLow);
  fillLevel = min(fillLevel, clipHigh);

  bitsave = maxBitSave - ((maxBitSave - minBitSave) / (clipHigh - clipLow)) *
                             (fillLevel - clipLow);

  return (bitsave);
}

static float iisaacfenc_calcBitSpend(float fillLevel,
                                     const float clipLow,
                                     const float clipHigh,
                                     const float minBitSpend,
                                     const float maxBitSpend) {
  float bitspend;

  fillLevel = max(fillLevel, clipLow);
  fillLevel = min(fillLevel, clipHigh);

  bitspend = minBitSpend + ((maxBitSpend - minBitSpend) / (clipHigh - clipLow)) *
                               (fillLevel - clipLow);

  return (bitspend);
}

static void iisaacfenc_adjustPeMinMax(const float currPe,
                                      float* peMin,
                                      float* peMax) {
  float minFacHi = 0.3f, maxFacHi = 1.0f, minFacLo = 0.14f, maxFacLo = 0.07f;
  float diff;
  float minDiff = currPe / 6.0f;

  if (currPe > *peMax) {
    diff = (currPe - *peMax);
    *peMin += diff * minFacHi;
    *peMax += diff * maxFacHi;
  } else if (currPe < *peMin) {
    diff = (*peMin - currPe);
    *peMin -= diff * minFacLo;
    *peMax -= diff * maxFacLo;
  } else {
    *peMin += (currPe - *peMin) * minFacHi;
    *peMax -= (*peMax - currPe) * maxFacLo;
  }

  if ((*peMax - *peMin) < minDiff) {
    float partLo, partHi;

    partLo = max(0.0f, currPe - *peMin);
    partHi = max(0.0f, *peMax - currPe);

    *peMax = currPe + partHi / (partLo + partHi) * minDiff;
    *peMin = currPe - partLo / (partLo + partHi) * minDiff;
    *peMin = max(0.0f, *peMin);
  }
}

static float iisaacfenc_bitresCalcBitFac(const int bitresBits,
                                         const int maxBitresBits,
                                         const float pe,
                                         const int windowSequence,
                                         const int avgBits,
                                         const float maxBitFac,
                                         const int highBWframe,
                                         ADJ_THR_STATE AdjThr,
                                         ATS_ELEMENT* adjThrChan) {
  BRES_PARAM* bresParam;
  float pex;
  float fillLevel;
  float bitSave, bitSpend, bitresFac;
  float maxBitSave, minBitSave, maxBitSpend, minBitSpend;

  if (windowSequence != SHORT_WINDOW)
    bresParam = &(AdjThr.bresParamLong);
  else
    bresParam = &(AdjThr.bresParamShort);

  pex = max(pe, adjThrChan->peMin);
  pex = min(pex, adjThrChan->peMax);

  if (maxBitresBits <= 0 || avgBits <= 0) {
    bitresFac = 1.0f;
  }

  else {
    const float limitation = (0.5f * maxBitresBits / avgBits);
    fillLevel = (float)(bitresBits) / (maxBitresBits);

    maxBitSave = min(bresParam->maxBitSave, limitation);
    minBitSave = max(bresParam->minBitSave, -maxBitSave);

    maxBitSpend = min(bresParam->maxBitSpend, limitation);
    minBitSpend = max(bresParam->minBitSpend, -maxBitSpend);

    if (highBWframe) {
      maxBitSpend = 0.1f * maxBitresBits / avgBits;
    }
    bitSave = iisaacfenc_calcBitSave(fillLevel,
                                     bresParam->clipSaveLow, bresParam->clipSaveHigh,
                                     minBitSave, maxBitSave);

    bitSpend = iisaacfenc_calcBitSpend(fillLevel,
                                       bresParam->clipSpendLow, bresParam->clipSpendHigh,
                                       minBitSpend, maxBitSpend);

    bitresFac = 1.0f - bitSave +
                ((bitSpend + bitSave) / (adjThrChan->peMax - adjThrChan->peMin)) *
                    (pex - adjThrChan->peMin);

    bitresFac = min(bitresFac, maxBitFac);
  }

  if (!highBWframe) {
    iisaacfenc_adjustPeMinMax(pe, &adjThrChan->peMin, &adjThrChan->peMax);
  }

  return bitresFac;
}

int iisaacfenc_AdjThrNew(ADJ_THR_STATE** phAdjThr,
                         int nElements) {
  int i;
  int error = 0;
  if ((*phAdjThr) == NULL) {
    (*phAdjThr) = (ADJ_THR_STATE*)iisCalloc(sizeof(ADJ_THR_STATE), 1);
    if ((*phAdjThr) == NULL) {
      error = 1;
    }
  }

  if (!error) {
    for (i = 0; i < nElements; i++) {
      if ((*phAdjThr)->adjThrStateElem[i] == NULL) {
        (*phAdjThr)->adjThrStateElem[i] = (ATS_ELEMENT*)iisCalloc(sizeof(ATS_ELEMENT), 1);
        if ((*phAdjThr)->adjThrStateElem[i] == NULL) {
          error = 1;
          iisaacfenc_AdjThrDelete((*phAdjThr));
          break;
        }
      }
    }
  }
  return error;
}

void iisaacfenc_AdjThrDelete(ADJ_THR_STATE* hAdjThr) {
  int i;
  if (hAdjThr) {
    for (i = 0; i < SIGMAP_MAX_ELEMENTS; i++) {
      if (hAdjThr->adjThrStateElem[i])
        iisFree(hAdjThr->adjThrStateElem[i]);
    }
    iisFree(hAdjThr);
  }
}

void iisaacfenc_AdjThrInit(ADJ_THR_STATE* hAdjThr,
                           const float meanPe,
                           ELEMENT_BITS* elBits[SIGMAP_MAX_ELEMENTS],
                           int nElements,
                           int bUseVbr,
                           float vbrQualFactor) {
  int i;
  hAdjThr->bresParamLong.clipSaveLow = 0.2f;
  hAdjThr->bresParamLong.clipSaveHigh = 0.95f;
  hAdjThr->bresParamLong.minBitSave = -0.05f;
  hAdjThr->bresParamLong.maxBitSave = 0.3f;
  hAdjThr->bresParamLong.clipSpendLow = 0.1f;
  hAdjThr->bresParamLong.clipSpendHigh = 0.95f;
  hAdjThr->bresParamLong.minBitSpend = -0.10f;
  hAdjThr->bresParamLong.maxBitSpend = 0.5f;

  hAdjThr->bresParamShort.clipSaveLow = 0.2f;
  hAdjThr->bresParamShort.clipSaveHigh = 0.75f;
  hAdjThr->bresParamShort.minBitSave = 0.0f;
  hAdjThr->bresParamShort.maxBitSave = 0.2f;
  hAdjThr->bresParamShort.clipSpendLow = 0.1f;
  hAdjThr->bresParamShort.clipSpendHigh = 0.75f;
  hAdjThr->bresParamShort.minBitSpend = -0.05f;
  hAdjThr->bresParamShort.maxBitSpend = 0.6f;

  for (i = 0; i < nElements; i++) {
    ATS_ELEMENT* atsElem = hAdjThr->adjThrStateElem[i];
    const int chBitrate = elBits[i]->chBitrate;

    atsElem->peMin = 0.8f * meanPe;
    atsElem->peMax = 1.2f * meanPe;

    atsElem->peOffset = 0.0f;
    if (chBitrate < 32000) {
      atsElem->peOffset = max(50.0f, 100.0f - 100.0f / 32000 * (float)chBitrate);
    }

    atsElem->bUseVbr = bUseVbr;
    atsElem->vbrQualFactor = vbrQualFactor;

    if (chBitrate > 20000) {
      atsElem->ahParam.startSfbL = 17;
      atsElem->ahParam.startSfbS = 3;
    } else if (chBitrate > 13000) {
      atsElem->ahParam.startSfbL = 8;
      atsElem->ahParam.startSfbS = 1;
    } else {
      atsElem->ahParam.startSfbL = 0;
      atsElem->ahParam.startSfbS = 0;
    }
  }
}

static void iisaacfenc_calcPeCorrection(float* correctionFac,
                                        const float peAct,
                                        const float peLast,
                                        const int bitsLast) {
  const float peActCorr = peAct * *correctionFac;

  if ((bitsLast > 8) &&
      (peLast > 200.0f) &&
      (peActCorr < 2.0f * peLast) && (peActCorr > 0.5f * peLast)) {
    float newFac = peLast / iisaacfenc_bits2pe((float)bitsLast);

    newFac = min(newFac, *correctionFac * 2.25f);
    *correctionFac = 0.75f * (*correctionFac) + 0.25f * newFac;
  } else {
    *correctionFac = 1.0f;
  }
}

void iisaacfenc_AdjThrUpdate(ATS_ELEMENT* AdjThrStateElement,
                             const int dynBitsUsed) {
  AdjThrStateElement->dynBitsLast = dynBitsUsed;
}

int iisaacfenc_AdjustThresholds(CHANNEL_MAPPING* cm,
                                QC_STATE* qcKernel,
                                QC_OUT* qcOut,
                                PSY_OUT* psyOut,
                                int* elemSideInfoBits,
                                IISBITFRAME_HANDLE hBitFrame,
                                int* pAdditionalElemBits,
                                BIT_DISTRIBUTION_MODE bitDistributionMode,
                                const int nBitsTransportOverheadAligned,
                                const int* nBitsAcelp,
                                const int highBWframe) {
  PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS_PER_ELEMENT];
  ATS_ELEMENT* AdjThrStateElement;
  PSY_OUT_ELEMENT* psyOutElement;
  ELEMENT_BITS* elBits;

  float* pNoRedPe;
  int avgBits;
  int bitresBits;
  int bitReservoir, bitReservoirMax;
  int maxBitresBits;
  int sideInfoBits;
  int elementId, channelId, nChannels = 0;
  int curWindowSequence = LONG_WINDOW;
  float grantedPe, grantedPeCorr;
  float bitFactor;
  float noRedPe;
  float totalGrantedPeCorr = 0.0f;
  float totalNoRedPe = 0.0f;

  if (cm->nElements > 1) {
    float avgSfbThresh[MAX_SFB_LONG];
    int minSfbActive = MAX_SFB_LONG;

    setFLOAT(0.0f, avgSfbThresh, MAX_SFB_LONG);

    for (elementId = 0; elementId < cm->nElements; elementId++) {
      if (cm->elInfo[elementId].elType < ID_LFE) {
        for (channelId = 0; channelId < cm->elInfo[elementId].nChannelsInEl; channelId++) {
          PSY_OUT_CHANNEL* psyOutCh = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];

          if (psyOutCh->windowSequence != SHORT_WINDOW) {
            nChannels++;
            if (minSfbActive > psyOutCh->sfbActive) {
              minSfbActive = psyOutCh->sfbActive;
            }
            addFLOAT(psyOutCh->sfbThreshold, avgSfbThresh, avgSfbThresh, minSfbActive);
          }
        }
      }
    }
    if (nChannels > 0) {
      smultFLOATip(0.5f / (float)nChannels, avgSfbThresh, minSfbActive);
    }

    for (elementId = 0; elementId < cm->nElements; elementId++) {
      if (cm->elInfo[elementId].elType < ID_LFE) {
        if ((cm->elInfo[elementId].elType == ID_CPE) &&
            (cm->elInfo[elementId].nChannelsInEl == 2) &&
            (psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[0]]->windowSequence != SHORT_WINDOW) &&
            (psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[1]]->windowSequence != SHORT_WINDOW)) {
          PSY_OUT_CHANNEL* psyOutCh_0 = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[0]];
          PSY_OUT_CHANNEL* psyOutCh_1 = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[1]];

          const float *sfbThresh_0 = psyOutCh_0->sfbThreshold, *sfbThresh_1 = psyOutCh_1->sfbThreshold;
          float *sfbMinSnr_0 = psyOutCh_0->sfbMinSnr, *sfbMinSnr_1 = psyOutCh_1->sfbMinSnr;
          int sfb;
          for (sfb = 0; sfb < minSfbActive; sfb++) {
            const float maxSfbThresh = max(sfbThresh_0[sfb], sfbThresh_1[sfb]);
            if ((maxSfbThresh < avgSfbThresh[sfb]) && (sfbMinSnr_0[sfb] < MIN_SNR_LIMIT)) {
              bitFactor = avgSfbThresh[sfb] / maxSfbThresh;
              sfbMinSnr_0[sfb] *= bitFactor;
              sfbMinSnr_1[sfb] *= bitFactor;
              if (sfbMinSnr_0[sfb] > MIN_SNR_LIMIT) {
                sfbMinSnr_0[sfb] = MIN_SNR_LIMIT;
                sfbMinSnr_1[sfb] = MIN_SNR_LIMIT;
              }
            }
          }
        } else {
          for (channelId = 0; channelId < cm->elInfo[elementId].nChannelsInEl; channelId++) {
            PSY_OUT_CHANNEL* psyOutCh = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];

            if (psyOutCh->windowSequence != SHORT_WINDOW) {
              const float* sfbThresh = psyOutCh->sfbThreshold;
              float* sfbMinSnr = psyOutCh->sfbMinSnr;
              int sfb;
              for (sfb = 0; sfb < minSfbActive; sfb++) {
                if ((sfbThresh[sfb] < avgSfbThresh[sfb]) && (sfbMinSnr[sfb] < MIN_SNR_LIMIT)) {
                  sfbMinSnr[sfb] *= avgSfbThresh[sfb] / sfbThresh[sfb];
                  if (sfbMinSnr[sfb] > MIN_SNR_LIMIT) {
                    sfbMinSnr[sfb] = MIN_SNR_LIMIT;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (elementId = 0; elementId < cm->nElements; elementId++) {
    if (cm->elInfo[elementId].elType != ID_DSE) {
      elBits = qcKernel->elementBits[elementId];

      qcOut->qcElement[elementId]->staticBitsUsed = elemSideInfoBits[elementId];

      bitReservoir = IISBITFRAME_GetBitreservoir(hBitFrame);
      bitReservoirMax = IISBITFRAME_GetBitreservoirMax(hBitFrame);

      elBits->bitResLevel = (int)(bitReservoir * elBits->relativeBits);
      elBits->maxBitResBits = (int)(bitReservoirMax * elBits->relativeBits);
      elBits->chBitDistribution[0] = (float)1.0 / (float)cm->elInfo[elementId].nChannelsInEl;
      elBits->chBitDistribution[1] = (float)1.0 - elBits->chBitDistribution[0];

      if (elBits->bitResLevel >= 0) {
        qcOut->qcElement[elementId]->staticBitsUsed = elemSideInfoBits[elementId];
        AdjThrStateElement = qcKernel->hAdjThr->adjThrStateElem[elementId];
        nChannels = cm->elInfo[elementId].nChannelsInEl;
        psyOutElement = psyOut->psyOutElement[elementId];
        pNoRedPe = &qcOut->qcElement[elementId]->pe;

        for (channelId = 0; channelId < nChannels; channelId++) {
          psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
        }

        iisaacfenc_preparePe(&AdjThrStateElement->peData,
                             AdjThrStateElement->weightedEn,
                             psyOutChannel,
                             psyOutElement,
                             qcKernel->hAdjThr->adjThrStateElem[0]->bUseVbr,
                             nChannels,
                             AdjThrStateElement->peOffset);

        if (qcKernel->hAdjThr->adjThrStateElem[0]->bUseVbr == 0) {
          iisaacfenc_calcPe(&AdjThrStateElement->peData, psyOutChannel, AdjThrStateElement->weightedEn, nChannels);

          noRedPe = AdjThrStateElement->peData.pe;
          qcOut->qcElement[elementId]->pe = noRedPe;
          *pNoRedPe = noRedPe;
        }

        for (channelId = 0; channelId < nChannels; channelId++) {
          psyOutChannel[channelId]->wasVeryTonal = psyOutChannel[channelId]->isVeryTonal;
          psyOutChannel[channelId]->isVeryTonal = 0;
        }

        if (qcKernel->hAdjThr->adjThrStateElem[0]->bUseVbr == 1) {
          iisaacfenc_adaptThresholds(psyOutChannel,
                                     psyOutElement,
                                     nChannels,
                                     AdjThrStateElement->weightedEn,
                                     AdjThrStateElement->thrExp,
                                     AdjThrStateElement->ahFlag,
                                     AdjThrStateElement->vbrQualFactor);
        }
      }
    }
  }

  if (qcKernel->hAdjThr->adjThrStateElem[0]->bUseVbr == 0) {
    for (elementId = 0; elementId < cm->nElements; elementId++) {
      elBits = qcKernel->elementBits[elementId];
      if (elBits->bitResLevel >= 0 && cm->elInfo[elementId].elType != ID_DSE) {
        nChannels = cm->elInfo[elementId].nChannelsInEl;
        AdjThrStateElement = qcKernel->hAdjThr->adjThrStateElem[elementId];

        if (IISBITFRAME_GetExtendedBitreservoirMax(hBitFrame) > 0) {
          sideInfoBits = qcOut->qcElement[elementId]->staticBitsUsed;
        } else {
          sideInfoBits = qcOut->qcElement[elementId]->staticBitsUsed + pAdditionalElemBits[elementId] + (nBitsTransportOverheadAligned / cm->nElements);
        }

        bitresBits = elBits->bitResLevel;
        avgBits = elBits->averageBits - qcOut->qcElement[elementId]->staticBitsUsed - nBitsAcelp[elementId];
        maxBitresBits = elBits->maxBitResBits;

        for (channelId = 0; channelId < nChannels; channelId++) {
          psyOutChannel[channelId] = psyOut->psyOutChannel[cm->elInfo[elementId].ChannelIndex[channelId]];
        }

        noRedPe = qcOut->qcElement[elementId]->pe;

        curWindowSequence = LONG_WINDOW;
        if (nChannels == 2) {
          if ((psyOutChannel[0]->windowSequence == SHORT_WINDOW) ||
              (psyOutChannel[1]->windowSequence == SHORT_WINDOW)) {
            curWindowSequence = SHORT_WINDOW;
          }
        } else {
          curWindowSequence = psyOutChannel[0]->windowSequence;
        }

        bitFactor = iisaacfenc_bitresCalcBitFac(bitresBits,
                                                maxBitresBits,
                                                (noRedPe + 5.0f * sideInfoBits) / nChannels,
                                                curWindowSequence,
                                                avgBits,
                                                qcKernel->maxBitFac,
                                                highBWframe,
                                                *qcKernel->hAdjThr,
                                                AdjThrStateElement);

        if (curWindowSequence != SHORT_WINDOW) {
          int i, j;
          float sumNrg = 0;
          for (i = 0; i < nChannels; i++) {
            for (j = 0; j < psyOutChannel[i]->sfbActive; j++) {
              sumNrg += psyOutChannel[i]->sfbEnergy[j];
            }
          }
          if (sumNrg < (6.0f * nChannels))
            bitFactor = min(bitFactor, 1.0625f);
        }

        grantedPe = bitFactor * iisaacfenc_bits2pe((float)avgBits);

        iisaacfenc_calcPeCorrection(&AdjThrStateElement->peCorrectionFactor,
                                    min(grantedPe, noRedPe),
                                    AdjThrStateElement->peLast,
                                    AdjThrStateElement->dynBitsLast);
        grantedPeCorr = grantedPe * AdjThrStateElement->peCorrectionFactor;
        AdjThrStateElement->grantedPe = grantedPe;
        AdjThrStateElement->noRedPe = noRedPe;

        totalGrantedPeCorr += grantedPeCorr;
        totalNoRedPe += noRedPe;

        if (qcKernel->forcePE > totalGrantedPeCorr && bitDistributionMode != BD_MODE_INTRA_ELEMENT) {
          totalGrantedPeCorr = qcKernel->forcePE;
          qcKernel->forcePE = 0.f;
        }

        if (bitDistributionMode == BD_MODE_INTRA_ELEMENT) {
          if (grantedPeCorr < noRedPe) {
            iisaacfenc_adaptThresholdsToPe(cm,
                                           qcKernel->hAdjThr,
                                           psyOut,
                                           grantedPeCorr,
                                           1,
                                           elementId);
          }
        }
      }
    }

    if (bitDistributionMode == BD_MODE_INTER_ELEMENT) {
      if (totalGrantedPeCorr < totalNoRedPe) {
        iisaacfenc_adaptThresholdsToPe(cm,
                                       qcKernel->hAdjThr,
                                       psyOut,
                                       totalGrantedPeCorr,
                                       qcKernel->nElements,
                                       0);
      }
    }

    for (elementId = 0; elementId < cm->nElements; elementId++) {
      elBits = qcKernel->elementBits[elementId];
      if (cm->elInfo[elementId].elType != ID_DSE && elBits->bitResLevel >= 0) {
        nChannels = cm->elInfo[elementId].nChannelsInEl;
        AdjThrStateElement = qcKernel->hAdjThr->adjThrStateElem[elementId];

        if ((nChannels > 1) && (AdjThrStateElement->peData.pe > AdjThrStateElement->peData.offset)) {
          elBits->chBitDistribution[0] = 0.2f + (1.0f - nChannels * 0.2f) * (AdjThrStateElement->peData.peChannelData[0].pe / (AdjThrStateElement->peData.pe - AdjThrStateElement->peData.offset));
          elBits->chBitDistribution[1] = 1.0f - elBits->chBitDistribution[0];
        }

        AdjThrStateElement->peLast = AdjThrStateElement->peData.pe;
      }
    }
  }

  iisaacfenc_invWeightThr(psyOut->psyOutChannel, cm->nChannels);

  return 0;
}
