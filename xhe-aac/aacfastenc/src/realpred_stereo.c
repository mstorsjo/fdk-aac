
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
#include <assert.h>
#include "realpred_stereo.h"
#include "mathlib.h"
#include "interface.h"
#include "glob_con.h"

static const float k_delta = 0.1f;
static const float k_delta_i = 10.0f;
static const float k_max_val = 3.0f;

#define SFB_PER_PRED_BAND (2)
#define PRED_COEFF_CHANGED (0xFF)
#define DELTA_LIMIT 60

static void iisaacfenc_QuantizeRealPredCoeffs(float predCoefRe, int *predCoefReQ) {
  const int signRe = predCoefRe >= 0.0f ? 1 : -1;

  predCoefRe = min(predCoefRe, k_max_val);
  predCoefRe = max(predCoefRe, -k_max_val);

  *predCoefReQ = signRe * (int)(fabs(predCoefRe) * k_delta_i + 0.5f);
}

static void iisaacfenc_DeQuantizeRealPredCoeffs(int predCoefReQ, float *predCoefRe) {
  *predCoefRe = predCoefReQ * k_delta;
}

static void iisaacfenc_setBandPred(const int idx, const int *sfbOffset,
                                   float *specL, float *specR,
                                   float *specMid, float *specSide,
                                   const float *specRes,
                                   float *thrL, float *thrR,
                                   const float *thrMS,
                                   float *nrgL, float *nrgR,
                                   const float *nrgDmx, const float *nrgRes,
                                   const int bSwap) {
  int i;
  const float maxNrgLR = max(nrgL[idx], nrgR[idx]);
  const float maxNrgDmxRes = max(nrgDmx[idx], nrgRes[idx]);

  if (bSwap) {
    for (i = sfbOffset[idx]; i < sfbOffset[idx + 1]; i++) {
      specL[i] = specSide[i];
      specR[i] = specRes[i];
    }
  } else {
    for (i = sfbOffset[idx]; i < sfbOffset[idx + 1]; i++) {
      specL[i] = specMid[i];
      specR[i] = specRes[i];
    }
  }

  if ((maxNrgDmxRes > FLT_MIN) && (maxNrgDmxRes < maxNrgLR)) {
    thrL[idx] = thrR[idx] = thrMS[idx] * (maxNrgDmxRes + FLT_MIN) / (maxNrgLR + FLT_MIN);
  } else {
    thrL[idx] = thrR[idx] = thrMS[idx];
  }
  nrgL[idx] = nrgDmx[idx];
  nrgR[idx] = nrgRes[idx];
}

static float iisaacfenc_calcRealPredictionCoefficient(float *mdctSpectrumMid,
                                                      float *mdctSpectrumSide,
                                                      const int numLines,
                                                      const int bSwap) {
  float covarianceMidSide = 0.0f;
  float energyMid = 0.0f;
  float energySide = 0.0f;

  covarianceMidSide = dotFLOAT(&mdctSpectrumMid[0], &mdctSpectrumSide[0], numLines);

  if (bSwap == 0) {
    energyMid = dotFLOAT(&mdctSpectrumMid[0], &mdctSpectrumMid[0], numLines);
    return covarianceMidSide / (energyMid + FLT_MIN);
  } else {
    energySide = dotFLOAT(&mdctSpectrumSide[0], &mdctSpectrumSide[0], numLines);
    return covarianceMidSide / (energySide + FLT_MIN);
  }
}

static int limitCoef(int coef) {
  const int limit = DELTA_LIMIT / 2;
  if (coef > limit) {
    coef = limit;
  }
  if (coef < -limit) {
    coef = -limit;
  }
  return coef;
}

int iisaacfenc_rdOptimizeRealPred(const int sfbCnt,
                                  const int sfbPerGroup,
                                  const int maxSfbPerGroup,
                                  int *msMask,
                                  const float *predCoeffRe,
                                  int *predCoeffReQ,
                                  const int *predCoeffPrevReQ,
                                  const int bResetPredictors) {
  int sfboffs, sfb;
  int prevBandCoeffReQ = 0;
  int nBitsTime = 0;
  int nBitsFreq = 0;
  int numPredBands = 0;
  int predBand;

  int deltaReFreq[MAX_GROUPED_SFB];
  int deltaReTime[MAX_GROUPED_SFB];
  float predErrorOrig = 0.0f;
  float predErrorTime = 0.0f;
  float tmp;
  int tmpCoeffRe;
  setINT(0, deltaReFreq, MAX_GROUPED_SFB);
  setINT(0, deltaReTime, MAX_GROUPED_SFB);

  numPredBands = 0;
  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    prevBandCoeffReQ = 0;

    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
      if (msMask[sfb]) {
        deltaReFreq[numPredBands] = predCoeffReQ[sfb] - prevBandCoeffReQ;
        numPredBands++;
        prevBandCoeffReQ = predCoeffReQ[sfb];
      }
    }
  }

  for (predBand = 0; predBand < numPredBands - 1; predBand++) {
    if (((deltaReFreq[predBand] == 1) && (deltaReFreq[predBand + 1] == -1)) ||
        ((deltaReFreq[predBand] == -1) && (deltaReFreq[predBand + 1] == 1))) {
      deltaReFreq[predBand] = 0;
      deltaReFreq[predBand + 1] = 0;
    }
  }
  if ((deltaReFreq[predBand] == 1) ||
      (deltaReFreq[predBand] == -1)) {
    deltaReFreq[predBand] = 0;
  }

  for (predBand = 0; predBand < numPredBands; predBand++) {
    if (deltaReFreq[predBand] == 0)
      nBitsFreq += 1;
    else if (deltaReFreq[predBand] == 1)
      nBitsFreq += 4;
    else
      nBitsFreq += 2 + abs(deltaReFreq[predBand]);
  }

  if (!bResetPredictors) {
    predBand = 0;
    for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
      for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
        if (msMask[sfb]) {
          float tmpPrevCoeffRe = (float)predCoeffPrevReQ[sfb] * k_delta;
          float tmpCoeffReDeq = (float)predCoeffReQ[sfb] * k_delta;

          tmp = (tmpCoeffReDeq - predCoeffRe[sfb]);
          predErrorOrig += tmp * tmp;

          deltaReTime[predBand] = predCoeffReQ[sfb] - predCoeffPrevReQ[sfb];

          if (fabs(predCoeffRe[sfb] - tmpPrevCoeffRe) <= 1.0f * k_delta) {
            deltaReTime[predBand] = 0;
          }

          if (deltaReTime[predBand] == 0)
            nBitsTime += 1;
          else if (deltaReTime[predBand] == 1)
            nBitsTime += 4;
          else
            nBitsTime += 2 + abs(deltaReTime[predBand]);

          tmp = (float)(predCoeffPrevReQ[sfb] + deltaReTime[predBand]) * k_delta - predCoeffRe[sfb];
          predErrorTime += tmp * tmp;
          predBand++;
        }
      }
    }
  }

  if (!nBitsTime && !nBitsFreq) {
    return 0;
  }

  if (bResetPredictors || (nBitsTime >= nBitsFreq) || (predErrorTime > 5.0f * predErrorOrig)) {
    predBand = 0;
    for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
      prevBandCoeffReQ = 0;

      for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
        if (msMask[sfb]) {
          tmpCoeffRe = predCoeffReQ[sfb];
          predCoeffReQ[sfb] = limitCoef(prevBandCoeffReQ + deltaReFreq[predBand]);

          predBand++;
          prevBandCoeffReQ = predCoeffReQ[sfb];

          if (tmpCoeffRe != predCoeffReQ[sfb]) {
            msMask[sfb] = PRED_COEFF_CHANGED;
          }

          if (sfb + 1 < sfboffs + maxSfbPerGroup) {
            predCoeffReQ[sfb + 1] = predCoeffReQ[sfb];
            msMask[sfb + 1] = msMask[sfb];
          }
        }
      }
    }
    return nBitsFreq;
  }

  else {
    predBand = 0;
    for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
      for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
        if (msMask[sfb]) {
          tmpCoeffRe = predCoeffReQ[sfb];
          predCoeffReQ[sfb] = limitCoef(predCoeffPrevReQ[sfb] + deltaReTime[predBand]);
          predBand++;

          if (tmpCoeffRe != predCoeffReQ[sfb]) {
            msMask[sfb] = PRED_COEFF_CHANGED;
          }

          if (sfb + 1 < sfboffs + maxSfbPerGroup) {
            predCoeffReQ[sfb + 1] = predCoeffReQ[sfb];
            msMask[sfb + 1] = msMask[sfb];
          }
        }
      }
    }

    return nBitsTime;
  }
}

void iisaacfenc_RealPredStereoProcessing(float *sfbEnergyLeft,
                                         float *sfbEnergyRight,
                                         const float *sfbEnergyMid,
                                         const float *sfbEnergySide,
                                         float *mdctSpectrumLeft,
                                         float *mdctSpectrumRight,
                                         float *sfbThresholdLeft,
                                         float *sfbThresholdRight,
                                         int *isBook,
                                         int *msDigest,
                                         int *msMask,
                                         const int sfbCnt,
                                         const int sfbPerGroup,
                                         const int maxSfbPerGroup,
                                         const int *sfbOffset,
                                         const int fDualMono,
                                         const int mergeMSRegions,
                                         struct STEREO_INFO *toolsInfo,
                                         const int forcePrediction,
                                         const int forcePredictionStartingBand) {
  int sfb;
  int sfboffs;
  int msMaskTrueSomewhere = 0;
  int msMaskFalseSomewhere = 0;
  int numMsMaskTrue = 0;
  int numMsMaskFalse = 0;
  int numPredCoeffsNonzero = 0;
  int numPredBands = 0;
  int numBitsRealPred = 0;
  int bUseBasicMs = 0;
  float sfbThresholdMS[MAX_GROUPED_SFB];
  float sfbEnergyDmx[MAX_GROUPED_SFB];
  float sfbEnergyRes[MAX_GROUPED_SFB];
  float sfbPredictionCoeffReal[MAX_GROUPED_SFB];

  float sumEnergyMid = 0.0f;
  float sumEnergySide = 0.0f;
  float sumEnergyRes = 0.0f;

  float mdctSpectrumMid[MAX_GRANULE_LEN];
  float mdctSpectrumSide[MAX_GRANULE_LEN];
  float mdctSpectrumRes[MAX_GRANULE_LEN];
  toolsInfo->bCplxPredMdctActive = 0;
  setINT(0, toolsInfo->predCoefReQ, MAX_GROUPED_SFB);
  setINT(0, toolsInfo->predCoefImQ, MAX_GROUPED_SFB);

  if (isBook) {
    setINT(0, isBook, sfbCnt);
  }

  if (fDualMono) {
    *msDigest = MS_NONE;
    setINT(0, msMask, sfbCnt);
    toolsInfo->bCplxPredMdctResetPredictors = 1;
    return;
  }

  toolsInfo->bCplxPredMdctRealOnly = 1;
  toolsInfo->sfbPerPredBand = SFB_PER_PRED_BAND;

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
      float predictionCoeffReal;

      int i;
      int startLine = sfbOffset[sfb];
      int endLine = sfbOffset[min(sfb + SFB_PER_PRED_BAND, sfboffs + maxSfbPerGroup)];

      numPredBands++;

      for (i = startLine; i < endLine; i++) {
        mdctSpectrumMid[i] = (mdctSpectrumLeft[i] + mdctSpectrumRight[i]) * 0.5f;
        mdctSpectrumSide[i] = (mdctSpectrumLeft[i] - mdctSpectrumRight[i]) * 0.5f;
      }

      predictionCoeffReal = iisaacfenc_calcRealPredictionCoefficient(&mdctSpectrumMid[startLine],
                                                                     &mdctSpectrumSide[startLine],
                                                                     endLine - startLine,
                                                                     toolsInfo->bSwap);

      sfbPredictionCoeffReal[sfb] = predictionCoeffReal;

      iisaacfenc_QuantizeRealPredCoeffs(predictionCoeffReal, &toolsInfo->predCoefReQ[sfb]);

      if (sfb + 1 < sfboffs + maxSfbPerGroup) {
        toolsInfo->predCoefReQ[sfb + 1] = toolsInfo->predCoefReQ[sfb];
      }
    }
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      float predictionCoeffRealQuantInv;

      int i;

      int startLine = sfbOffset[sfb];
      int endLine = sfbOffset[sfb + 1];

      sfbEnergyDmx[sfb] = 0.0f;
      sfbEnergyRes[sfb] = 0.0f;

      iisaacfenc_DeQuantizeRealPredCoeffs(toolsInfo->predCoefReQ[sfb], &predictionCoeffRealQuantInv);

      toolsInfo->predCoefImQ[sfb] = 0;

      if (toolsInfo->bSwap == 0) {
        for (i = startLine; i < endLine; i++) {
          mdctSpectrumRes[i] = mdctSpectrumSide[i] - mdctSpectrumMid[i] * predictionCoeffRealQuantInv;
          sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
        }
        sfbEnergyDmx[sfb] = sfbEnergyMid[sfb];
        toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb] + FLT_MIN) / (sfbEnergySide[sfb] + FLT_MIN));
      } else {
        for (i = startLine; i < endLine; i++) {
          mdctSpectrumRes[i] = mdctSpectrumMid[i] - mdctSpectrumSide[i] * predictionCoeffRealQuantInv;
          sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
        }
        sfbEnergyDmx[sfb] = sfbEnergySide[sfb];
        toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb] + FLT_MIN) / (sfbEnergyMid[sfb] + FLT_MIN));
      }

      sumEnergyRes += sfbEnergyRes[sfb];
      sumEnergyMid += sfbEnergyMid[sfb];
      sumEnergySide += sfbEnergySide[sfb];

      sfbThresholdMS[sfb] = min(sfbThresholdLeft[sfb], sfbThresholdRight[sfb]);
    }
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
      float penaltyLR, penaltyPred;

      float thresholdLeft = sfbThresholdLeft[sfb];
      float thresholdRight = sfbThresholdRight[sfb];
      float thresholdMS = sfbThresholdMS[sfb];
      float energyLeft = sfbEnergyLeft[sfb] + FLT_MIN;
      float energyRight = sfbEnergyRight[sfb] + FLT_MIN;
      float energyDmx = sfbEnergyDmx[sfb] + FLT_MIN;
      float energyRes = sfbEnergyRes[sfb] + FLT_MIN;

      if (sfb + 1 < sfboffs + maxSfbPerGroup) {
        thresholdLeft += sfbThresholdLeft[sfb + 1];
        thresholdRight += sfbThresholdRight[sfb + 1];
        thresholdMS += sfbThresholdMS[sfb + 1];
        energyLeft += sfbEnergyLeft[sfb + 1];
        energyRight += sfbEnergyRight[sfb + 1];
        energyDmx += sfbEnergyDmx[sfb + 1];
        energyRes += sfbEnergyRes[sfb + 1];
      }

      penaltyLR = (thresholdLeft / max(energyLeft, thresholdLeft)) *
                  (thresholdRight / max(energyRight, thresholdRight));

      penaltyPred = (thresholdMS / max(energyDmx, thresholdMS)) *
                    (thresholdMS / max(energyRes, thresholdMS));

      if (penaltyPred > penaltyLR ||
          (forcePredictionStartingBand >= 0 && sfb - sfboffs >= forcePredictionStartingBand)) {
        msMask[sfb] = PRED_ON;
        numMsMaskTrue += SFB_PER_PRED_BAND;
        msMaskTrueSomewhere = 1;

        if (toolsInfo->predCoefReQ[sfb] != 0) {
          numPredCoeffsNonzero++;
        }
      } else {
        msMask[sfb] = 0;
        numMsMaskFalse += SFB_PER_PRED_BAND;
        msMaskFalseSomewhere = 1;
      }

      if (sfb + 1 < sfboffs + maxSfbPerGroup) {
        msMask[sfb + 1] = msMask[sfb];
      }
    }
  }

  if (msMaskTrueSomewhere) {
    toolsInfo->bCplxPredMdctActive = 1;
    if (msMaskFalseSomewhere) {
      *msDigest = MS_SOME;

      if (mergeMSRegions) {
        if (numMsMaskFalse < 9) {
          *msDigest = MS_ALL;
          setINT(PRED_ON, msMask, sfbCnt);
        } else if (numMsMaskTrue < 9) {
          *msDigest = MS_NONE;
          setINT(0, msMask, sfbCnt);
          toolsInfo->bCplxPredMdctActive = 0;
        }
      }

    } else {
      *msDigest = MS_ALL;
    }
  } else {
    *msDigest = MS_NONE;
    toolsInfo->bCplxPredMdctActive = 0;
  }

  numBitsRealPred = iisaacfenc_rdOptimizeRealPred(sfbCnt,
                                                  sfbPerGroup,
                                                  maxSfbPerGroup,
                                                  msMask,
                                                  sfbPredictionCoeffReal,
                                                  toolsInfo->predCoefReQ,
                                                  toolsInfo->predCoefPrevReQ,
                                                  toolsInfo->bCplxPredMdctResetPredictors);

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if (msMask[sfb] == PRED_COEFF_CHANGED) {
        float predictionCoeffRealQuantInv;
        int i;

        int startLine = sfbOffset[sfb];
        int endLine = sfbOffset[sfb + 1];

        msMask[sfb] = PRED_ON;
        sumEnergyRes -= sfbEnergyRes[sfb];

        sfbEnergyRes[sfb] = 0.0f;

        iisaacfenc_DeQuantizeRealPredCoeffs(toolsInfo->predCoefReQ[sfb], &predictionCoeffRealQuantInv);

        if (toolsInfo->bSwap == 0) {
          for (i = startLine; i < endLine; i++) {
            mdctSpectrumRes[i] = mdctSpectrumSide[i] - mdctSpectrumMid[i] * predictionCoeffRealQuantInv;
            sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
          }
          toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb] + FLT_MIN) / (sfbEnergySide[sfb] + FLT_MIN));
        } else {
          for (i = startLine; i < endLine; i++) {
            mdctSpectrumRes[i] = mdctSpectrumMid[i] - mdctSpectrumSide[i] * predictionCoeffRealQuantInv;
            sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
          }
          toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb] + FLT_MIN) / (sfbEnergyMid[sfb] + FLT_MIN));
        }

        sumEnergyRes += sfbEnergyRes[sfb];
      }
    }
  }

  if ((*msDigest != MS_NONE) &&
      (!forcePrediction)) {
    if (numPredCoeffsNonzero == 0) {
      bUseBasicMs = 1;
    }

    else if (numPredCoeffsNonzero < 4) {
      bUseBasicMs = 1;
    }

    else if (1.625f * sumEnergyRes > (toolsInfo->bSwap ? sumEnergyMid : sumEnergySide)) {
      bUseBasicMs = 1;
    }
  }

  if (!forcePrediction) {
    if (bUseBasicMs) {
      toolsInfo->bCplxPredMdctActive = 0;
      toolsInfo->bSwap = 0;
    } else {
      float meanBitsPerPredBand = (float)numBitsRealPred / (float)numPredBands;
      float minGain = 1.625f * meanBitsPerPredBand;

      bUseBasicMs = 0;

      if (toolsInfo->bCplxPredMdctActivePrev) {
        minGain *= 0.75f;
      }

      if (toolsInfo->bCplxPredMdctResetPredictors) {
        minGain *= 0.75f;
      }

      if (minGain * sumEnergyRes > (toolsInfo->bSwap ? sumEnergyMid : sumEnergySide)) {
        bUseBasicMs = 1;
      }
    }
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if (!bUseBasicMs && msMask[sfb] == PRED_ON) {
        iisaacfenc_setBandPred(sfb, sfbOffset,
                               mdctSpectrumLeft, mdctSpectrumRight,
                               mdctSpectrumMid, mdctSpectrumSide,
                               mdctSpectrumRes,
                               sfbThresholdLeft, sfbThresholdRight,
                               sfbThresholdMS,
                               sfbEnergyLeft, sfbEnergyRight,
                               sfbEnergyDmx, sfbEnergyRes,
                               toolsInfo->bSwap);

        msMask[sfb] = MS_ON;
      } else {
        toolsInfo->predCoefReQ[sfb] = 0;
        msMask[sfb] = 0;
      }
    }
  }
}
