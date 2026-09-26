
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
#include <stdlib.h>

#include "cplxpred_stereo.h"
#include "realpred_stereo.h"
#include "mathlib.h"
#include "glob_con.h"
#include "aacenc_internal.h"

#define HIGH_BIT_COST 5000
#define DELTA_LIMIT 60

static const float k_delta = 0.1f;
static const float k_delta_i = 10.0f;
static const float k_max_val = 3.0f;

#define SFB_PER_PRED_BAND (2)
#define PRED_COEFF_CHANGED (0xFF)

static const float h_long_sin_curr[] = {-0.000000f, -0.000000f, 0.500000f, 0.000000f, -0.500000f, 0.000000f, 0.000000f};
static const float h_long_kbd_curr[] = {0.091497f, -0.000000f, 0.581427f, 0.000000f, -0.581427f, 0.000000f, -0.091497f};
static const float h_long_sin_kbd_curr[] = {0.045748f, 0.057238f, 0.540714f, 0.000000f, -0.540714f, -0.057238f, -0.045748f};
static const float h_long_kbd_sin_curr[] = {0.045748f, -0.057238f, 0.540714f, 0.000000f, -0.540714f, 0.057238f, -0.045748f};
static const float h_start_sin_curr[] = {0.102658f, 0.103791f, 0.567149f, 0.000000f, -0.567149f, -0.103791f, -0.102658f};
static const float h_start_kbd_curr[] = {0.150512f, 0.047969f, 0.608574f, 0.000000f, -0.608574f, -0.047969f, -0.150512f};
static const float h_start_sin_kbd_curr[] = {0.104763f, 0.105207f, 0.567861f, 0.000000f, -0.567861f, -0.105207f, -0.104763f};
static const float h_start_kbd_sin_curr[] = {0.148406f, 0.046553f, 0.607863f, 0.000000f, -0.607863f, -0.046553f, -0.148406f};
static const float h_stop_sin_curr[] = {0.102658f, -0.103791f, 0.567149f, 0.000000f, -0.567149f, 0.103791f, -0.102658f};
static const float h_stop_kbd_curr[] = {0.150512f, -0.047969f, 0.608574f, 0.000000f, -0.608574f, 0.047969f, -0.150512f};
static const float h_stop_sin_kbd_curr[] = {0.148406f, -0.046553f, 0.607863f, 0.000000f, -0.607863f, 0.046553f, -0.148406f};
static const float h_stop_kbd_sin_curr[] = {0.104763f, -0.105207f, 0.567861f, 0.000000f, -0.567861f, 0.105207f, -0.104763f};
static const float h_stopstart_sin_curr[] = {0.205316f, -0.000000f, 0.634298f, 0.000000f, -0.634298f, 0.000000f, -0.205316f};
static const float h_stopstart_kbd_curr[] = {0.209526f, -0.000000f, 0.635722f, 0.000000f, -0.635722f, 0.000000f, -0.209526f};
static const float h_stopstart_sin_kbd_curr[] = {0.207421f, 0.001416f, 0.635010f, 0.000000f, -0.635010f, -0.001416f, -0.207421f};
static const float h_stopstart_kbd_sin_curr[] = {0.207421f, -0.001416f, 0.635010f, 0.000000f, -0.635010f, 0.001416f, -0.207421f};
static const float h_long_sin_prev[] = {-0.000000f, 0.106103f, 0.250000f, 0.318310f, 0.250000f, 0.106103f, -0.000000f};
static const float h_long_kbd_prev[] = {0.059509f, 0.123714f, 0.186579f, 0.213077f, 0.186579f, 0.123714f, 0.059509f};
static const float h_stop_sin_prev[] = {0.038498f, 0.039212f, 0.039645f, 0.039790f, 0.039645f, 0.039212f, 0.038498f};
static const float h_stop_kbd_prev[] = {0.026142f, 0.026413f, 0.026577f, 0.026631f, 0.026577f, 0.026413f, 0.026142f};

static const float *h_long_curr[2][2] = {{h_long_sin_curr, h_long_sin_kbd_curr}, {h_long_kbd_sin_curr, h_long_kbd_curr}};
static const float *h_start_curr[2][2] = {{h_start_sin_curr, h_start_sin_kbd_curr}, {h_start_kbd_sin_curr, h_start_kbd_curr}};
static const float *h_stop_curr[2][2] = {{h_stop_sin_curr, h_stop_sin_kbd_curr}, {h_stop_kbd_sin_curr, h_stop_kbd_curr}};
static const float *h_stopstart_curr[2][2] = {{h_stopstart_sin_curr, h_stopstart_sin_kbd_curr}, {h_stopstart_kbd_sin_curr, h_stopstart_kbd_curr}};
static const float *h_long_prev[2] = {h_long_sin_prev, h_long_kbd_prev};
static const float *h_stop_prev[2] = {h_stop_sin_prev, h_stop_kbd_prev};

static void iisaacfenc_QuantizeCplxPredCoeffs(float predCoefRe, float predCoefIm, int *predCoefReQ, int *predCoefImQ) {
  const int signRe = predCoefRe >= 0.0f ? 1 : -1;
  const int signIm = predCoefIm >= 0.0f ? 1 : -1;

  predCoefRe = min(predCoefRe, k_max_val);
  predCoefRe = max(predCoefRe, -k_max_val);

  predCoefIm = min(predCoefIm, k_max_val);
  predCoefIm = max(predCoefIm, -k_max_val);

  *predCoefReQ = signRe * (int)(fabs(predCoefRe) * k_delta_i + 0.5f);
  *predCoefImQ = signIm * (int)(fabs(predCoefIm) * k_delta_i + 0.5f);
}

static void iisaacfenc_DeQuantizeCplxPredCoeffs(int predCoefReQ, int predCoefImQ, float *predCoefRe, float *predCoefIm) {
  *predCoefRe = predCoefReQ * k_delta;
  *predCoefIm = predCoefImQ * k_delta;
}

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

static float iisaacfenc_calcComplexPredictionCoefficient(const float *mdctSpectrumMid,
                                                         const float *mdctSpectrumSide,
                                                         const float *mdstSpectrumDmx,
                                                         float *predCoeffRe,
                                                         float *predCoeffIm,
                                                         const int numLines,
                                                         const int bSwap) {
  float covarianceDmxImSide, covarianceReIm, energyDmxRe, tmp;
  float covarianceMidSide = dotFLOAT(mdctSpectrumMid, mdctSpectrumSide, numLines);
  float energyDmxIm = dotFLOAT(mdstSpectrumDmx, mdstSpectrumDmx, numLines);

  if (bSwap == 0) {
    covarianceDmxImSide = dotFLOAT(mdstSpectrumDmx, mdctSpectrumSide, numLines);
    covarianceReIm = dotFLOAT(mdctSpectrumMid, mdstSpectrumDmx, numLines);
    energyDmxRe = dotFLOAT(mdctSpectrumMid, mdctSpectrumMid, numLines);
  } else {
    covarianceDmxImSide = dotFLOAT(mdstSpectrumDmx, mdctSpectrumMid, numLines);
    covarianceReIm = dotFLOAT(mdctSpectrumSide, mdstSpectrumDmx, numLines);
    energyDmxRe = dotFLOAT(mdctSpectrumSide, mdctSpectrumSide, numLines);
  }

  tmp = 1.0f / (energyDmxRe * energyDmxIm - covarianceReIm * covarianceReIm + FLT_MIN);
  *predCoeffRe = tmp * (energyDmxIm * covarianceMidSide - covarianceReIm * covarianceDmxImSide);
  *predCoeffIm = tmp * (energyDmxRe * covarianceDmxImSide - covarianceReIm * covarianceMidSide);

  return covarianceMidSide / (energyDmxRe + FLT_MIN);
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

static int iisaacfenc_rdOptimizeCplxPred(const int sfbCnt,
                                         const int sfbPerGroup,
                                         const int maxSfbPerGroup,
                                         int *msMask,
                                         const float *predCoeffRe,
                                         int *predCoeffReQ,
                                         const int *predCoeffPrevReQ,
                                         const float *predCoeffIm,
                                         int *predCoeffImQ,
                                         const int *predCoeffPrevImQ,
                                         const int bResetPredictors) {
  int sfboffs, sfb;
  int prevBandCoeffReQ = 0;
  int prevBandCoeffImQ = 0;
  int nBitsTime = 0;
  int nBitsFreq = 0;
  int numPredBands = 0;
  int predBand;

  int deltaFreqRe[MAX_GROUPED_SFB];
  int deltaTimeRe[MAX_GROUPED_SFB];
  int deltaFreqIm[MAX_GROUPED_SFB];
  int deltaTimeIm[MAX_GROUPED_SFB];
  float predErrorOrig = 0.0f;
  float predErrorTime = 0.0f;
  float tmp;
  int tmpCoeffRe;
  int tmpCoeffIm;
  setINT(0, deltaFreqRe, MAX_GROUPED_SFB);
  setINT(0, deltaTimeRe, MAX_GROUPED_SFB);
  setINT(0, deltaFreqIm, MAX_GROUPED_SFB);
  setINT(0, deltaTimeIm, MAX_GROUPED_SFB);

  numPredBands = 0;
  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    prevBandCoeffReQ = 0;
    prevBandCoeffImQ = 0;

    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
      if (msMask[sfb]) {
        deltaFreqRe[numPredBands] = predCoeffReQ[sfb] - prevBandCoeffReQ;
        deltaFreqIm[numPredBands] = predCoeffImQ[sfb] - prevBandCoeffImQ;
        numPredBands++;
        prevBandCoeffReQ = predCoeffReQ[sfb];
        prevBandCoeffImQ = predCoeffImQ[sfb];
      }
    }
  }

  numPredBands = 0;
  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup - 1; sfb += SFB_PER_PRED_BAND) {
      if (((deltaFreqRe[numPredBands] == 1) && (deltaFreqRe[numPredBands + 1] == -1)) ||
          ((deltaFreqRe[numPredBands] == -1) && (deltaFreqRe[numPredBands + 1] == 1))) {
        deltaFreqRe[numPredBands] = 0;
        deltaFreqRe[numPredBands + 1] = 0;
      }
      if (((deltaFreqIm[numPredBands] == 1) && (deltaFreqIm[numPredBands + 1] == -1)) ||
          ((deltaFreqIm[numPredBands] == -1) && (deltaFreqIm[numPredBands + 1] == 1))) {
        deltaFreqIm[numPredBands] = 0;
        deltaFreqIm[numPredBands + 1] = 0;
      }
      numPredBands++;
    }

    if ((deltaFreqRe[numPredBands] == 1) ||
        (deltaFreqRe[numPredBands] == -1)) {
      deltaFreqRe[numPredBands] = 0;
    }
    if ((deltaFreqIm[numPredBands] == 1) ||
        (deltaFreqIm[numPredBands] == -1)) {
      deltaFreqIm[numPredBands] = 0;
    }
    numPredBands++;
  }

  for (predBand = 0; predBand < numPredBands; predBand++) {
    if (deltaFreqRe[predBand] == 0)
      nBitsFreq += 1;
    else if (deltaFreqRe[predBand] == 1)
      nBitsFreq += 4;
    else if (deltaFreqRe[predBand] < -DELTA_LIMIT ||
             deltaFreqRe[predBand] > DELTA_LIMIT) {
      nBitsFreq += HIGH_BIT_COST;
      assert(0);
    } else
      nBitsFreq += 2 + abs(deltaFreqRe[predBand]);

    if (deltaFreqIm[predBand] == 0)
      nBitsFreq += 1;
    else if (deltaFreqIm[predBand] == 1)
      nBitsFreq += 4;
    else if (deltaFreqIm[predBand] < -DELTA_LIMIT ||
             deltaFreqIm[predBand] > DELTA_LIMIT) {
      nBitsFreq += HIGH_BIT_COST;
      assert(0);
    } else
      nBitsFreq += 2 + abs(deltaFreqIm[predBand]);
  }

  if (!bResetPredictors) {
    predBand = 0;
    for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
      for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
        if (msMask[sfb]) {
          float tmpPrevCoeffRe = (float)predCoeffPrevReQ[sfb] * k_delta;
          float tmpCoeffReDeq = (float)predCoeffReQ[sfb] * k_delta;
          float tmpPrevCoeffIm = (float)predCoeffPrevImQ[sfb] * k_delta;
          float tmpCoeffImDeq = (float)predCoeffImQ[sfb] * k_delta;

          tmp = (tmpCoeffReDeq - predCoeffRe[sfb]);
          predErrorOrig += tmp * tmp;
          tmp = (tmpCoeffImDeq - predCoeffIm[sfb]);
          predErrorOrig += tmp * tmp;

          deltaTimeRe[predBand] = predCoeffReQ[sfb] - predCoeffPrevReQ[sfb];
          deltaTimeIm[predBand] = predCoeffImQ[sfb] - predCoeffPrevImQ[sfb];

          if (fabs(predCoeffRe[sfb] - tmpPrevCoeffRe) <= 1.0f * k_delta) {
            deltaTimeRe[predBand] = 0;
          }
          if (fabs(predCoeffIm[sfb] - tmpPrevCoeffIm) <= 1.0f * k_delta) {
            deltaTimeIm[predBand] = 0;
          }

          if (deltaTimeRe[predBand] == 0)
            nBitsTime += 1;
          else if (deltaTimeRe[predBand] == 1)
            nBitsTime += 4;
          else if (deltaTimeRe[predBand] < -DELTA_LIMIT ||
                   deltaTimeRe[predBand] > DELTA_LIMIT) {
            nBitsTime += HIGH_BIT_COST;
            assert(0);
          } else
            nBitsTime += 2 + abs(deltaTimeRe[predBand]);
          if (deltaTimeIm[predBand] == 0)
            nBitsTime += 1;
          else if (deltaTimeIm[predBand] == 1)
            nBitsTime += 4;
          else if (deltaTimeIm[predBand] < -DELTA_LIMIT ||
                   deltaTimeIm[predBand] > DELTA_LIMIT) {
            nBitsTime += HIGH_BIT_COST;
            assert(0);
          } else
            nBitsTime += 2 + abs(deltaTimeIm[predBand]);

          tmp = (float)(predCoeffPrevReQ[sfb] + deltaTimeRe[predBand]) * k_delta - predCoeffRe[sfb];
          predErrorTime += tmp * tmp;
          tmp = (float)(predCoeffPrevImQ[sfb] + deltaTimeIm[predBand]) * k_delta - predCoeffIm[sfb];
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
      prevBandCoeffImQ = 0;

      for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
        if (msMask[sfb]) {
          tmpCoeffRe = predCoeffReQ[sfb];
          tmpCoeffIm = predCoeffImQ[sfb];
          predCoeffReQ[sfb] = prevBandCoeffReQ + deltaFreqRe[predBand];
          predCoeffImQ[sfb] = prevBandCoeffImQ + deltaFreqIm[predBand];
          predCoeffReQ[sfb] = limitCoef(predCoeffReQ[sfb]);
          predCoeffImQ[sfb] = limitCoef(predCoeffImQ[sfb]);
          predBand++;
          prevBandCoeffReQ = predCoeffReQ[sfb];
          prevBandCoeffImQ = predCoeffImQ[sfb];

          if ((tmpCoeffRe != predCoeffReQ[sfb]) || (tmpCoeffIm != predCoeffImQ[sfb])) {
            msMask[sfb] = PRED_COEFF_CHANGED;
          }

          if (sfb + 1 < sfboffs + maxSfbPerGroup) {
            predCoeffReQ[sfb + 1] = predCoeffReQ[sfb];
            predCoeffImQ[sfb + 1] = predCoeffImQ[sfb];
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
          tmpCoeffIm = predCoeffImQ[sfb];
          predCoeffReQ[sfb] = predCoeffPrevReQ[sfb] + deltaTimeRe[predBand];
          predCoeffImQ[sfb] = predCoeffPrevImQ[sfb] + deltaTimeIm[predBand];
          predCoeffReQ[sfb] = limitCoef(predCoeffReQ[sfb]);
          predCoeffImQ[sfb] = limitCoef(predCoeffImQ[sfb]);
          predBand++;

          if ((tmpCoeffRe != predCoeffReQ[sfb]) || (tmpCoeffIm != predCoeffImQ[sfb])) {
            msMask[sfb] = PRED_COEFF_CHANGED;
          }

          if (sfb + 1 < sfboffs + maxSfbPerGroup) {
            predCoeffReQ[sfb + 1] = predCoeffReQ[sfb];
            predCoeffImQ[sfb + 1] = predCoeffImQ[sfb];
            msMask[sfb + 1] = msMask[sfb];
          }
        }
      }
    }

    return nBitsTime;
  }
}

static void iisaacfenc_filterAndAdd(const float *in, int length, const float *filter, float *out, float factorEven, float factorOdd) {
  float s;

  int i = 0;
  s = filter[6] * in[2] + filter[5] * in[1] + filter[4] * in[0] + filter[3] * in[0] +
      filter[2] * in[1] + filter[1] * in[2] + filter[0] * in[3];
  out[i] += s * factorEven;

  i = 1;
  s = filter[6] * in[1] + filter[5] * in[0] + filter[4] * in[0] + filter[3] * in[1] +
      filter[2] * in[2] + filter[1] * in[3] + filter[0] * in[4];
  out[i] += s * factorOdd;

  i = 2;
  s = filter[6] * in[0] + filter[5] * in[0] + filter[4] * in[1] + filter[3] * in[2] +
      filter[2] * in[3] + filter[1] * in[4] + filter[0] * in[5];
  out[i] += s * factorEven;

  for (i = 3; i < length - 4; i += 2) {
    s = filter[6] * in[i - 3] + filter[5] * in[i - 2] + filter[4] * in[i - 1] + filter[3] * in[i] +
        filter[2] * in[i + 1] + filter[1] * in[i + 2] + filter[0] * in[i + 3];
    out[i] += s * factorOdd;

    s = filter[6] * in[i - 2] + filter[5] * in[i - 1] + filter[4] * in[i] + filter[3] * in[i + 1] +
        filter[2] * in[i + 2] + filter[1] * in[i + 3] + filter[0] * in[i + 4];
    out[i + 1] += s * factorEven;
  }

  i = length - 3;
  s = filter[6] * in[i - 3] + filter[5] * in[i - 2] + filter[4] * in[i - 1] + filter[3] * in[i] +
      filter[2] * in[i + 1] + filter[1] * in[i + 2] + filter[0] * in[i + 2];
  out[i] += s * factorOdd;

  i = length - 2;
  s = filter[6] * in[i - 3] + filter[5] * in[i - 2] + filter[4] * in[i - 1] + filter[3] * in[i] +
      filter[2] * in[i + 1] + filter[1] * in[i + 1] + filter[0] * in[i];
  out[i] += s * factorEven;

  i = length - 1;
  s = filter[6] * in[i - 3] + filter[5] * in[i - 2] + filter[4] * in[i - 1] + filter[3] * in[i] +
      filter[2] * in[i] + filter[1] * in[i - 1] + filter[0] * in[i - 2];
  out[i] += s * factorOdd;
}

static float iisaacfenc_energy(const float *x, int n) {
  int i;
  float result = 0.0f;

  for (i = 0; i < n; i++) {
    result += x[i] * x[i];
  }
  return result;
}

static int iisaacfenc_estimateDeltaRateLines(const float mdctSpectrum0[], const float mdctSpectrum1[],
                                             const float sfbThres,
                                             int nLines) {
  static const float eps = 1e-6f;
  int deltaRate = 0;
  const float LOG2 = (float)log(2.0f);
  static const float C3 = 0.56f;
  float thr;

  int k;
  float sfbNLines0, sfbNLines1;
  float sfbFormFactor0 = eps;
  float sfbFormFactor1 = eps;

  float en0 = iisaacfenc_energy(mdctSpectrum0, nLines) + eps;
  float en1 = iisaacfenc_energy(mdctSpectrum1, nLines) + eps;

  float avgFormFactor0 = (float)pow(en0 / nLines, 0.25f);
  float avgFormFactor1 = (float)pow(en1 / nLines, 0.25f);

  for (k = 0; k < nLines; k++) {
    sfbFormFactor0 += (float)sqrt(fabs(mdctSpectrum0[k]));
    sfbFormFactor1 += (float)sqrt(fabs(mdctSpectrum1[k]));
  }

  sfbNLines0 = sfbFormFactor0 / avgFormFactor0;
  sfbNLines1 = sfbFormFactor1 / avgFormFactor1;

  thr = min(en0, en1);
  thr = max(thr, sfbThres);

  deltaRate = (int)(deltaRate + ((C3 * (sfbNLines0 * max(0, log(en0 / thr) / LOG2) - sfbNLines1 * max(0, log(en1 / thr) / LOG2))) / 1.18f));

  return deltaRate;
}

void iisaacfenc_PrepareCplxPredFlags(const int bCommonWindow, const int bUsacIndepFlag,
                                     const int lastWinSequence0, const int lastWinSequence1,
                                     const int currWinSequence0, const int currWinSequence1,
                                     int *bCplxPredUsePrevFrame, int *bCplxPredResetPredictors) {
  if ((bUsacIndepFlag) ||
      (!bCommonWindow) ||
      (lastWinSequence0 == SHORT_WINDOW && currWinSequence0 != SHORT_WINDOW) ||
      (lastWinSequence0 != SHORT_WINDOW && currWinSequence0 == SHORT_WINDOW) ||
      (lastWinSequence1 == SHORT_WINDOW && currWinSequence1 != SHORT_WINDOW) ||
      (lastWinSequence1 != SHORT_WINDOW && currWinSequence1 == SHORT_WINDOW)) {
    *bCplxPredUsePrevFrame = 0;
    *bCplxPredResetPredictors = 1;
  } else {
    *bCplxPredUsePrevFrame = 1;

    *bCplxPredResetPredictors = 0;
  }
}

void iisaacfenc_Mdct2Mdst(float *mdstSpectrumOut,
                          const float *mdctSpectrumCurr,
                          const float *mdctSpectrumPrev,
                          const int length,
                          const BLOCK_TYPE blockType,
                          const int windowShapeCurr,
                          const int windowShapePrev) {
  const float *h_curr = NULL;
  const float *h_prev = NULL;

  switch (blockType) {
    case LONG_WINDOW:
    case SHORT_WINDOW:
      h_curr = h_long_curr[windowShapePrev][windowShapeCurr];
      h_prev = h_long_prev[windowShapePrev];
      break;

    case START_WINDOW:
      h_curr = h_start_curr[windowShapePrev][windowShapeCurr];
      h_prev = h_long_prev[windowShapePrev];
      break;

    case STOP_WINDOW:
      h_curr = h_stop_curr[windowShapePrev][windowShapeCurr];
      h_prev = h_stop_prev[windowShapePrev];
      break;

    case STOPSTART_WINDOW:
      h_curr = h_stopstart_curr[windowShapePrev][windowShapeCurr];
      h_prev = h_stop_prev[windowShapePrev];
      break;
    default:
      assert(0);
      break;
  }

  setFLOAT(0.0f, mdstSpectrumOut, length);
  iisaacfenc_filterAndAdd(mdctSpectrumCurr, length, h_curr, mdstSpectrumOut, 1, 1);

  if (mdctSpectrumPrev) {
    iisaacfenc_filterAndAdd(mdctSpectrumPrev, length, h_prev, mdstSpectrumOut, -1, 1);
  }
}

void iisaacfenc_CplxPredStereoProcessing(float *sfbEnergyLeft,
                                         float *sfbEnergyRight,
                                         const float *sfbEnergyMid,
                                         const float *sfbEnergySide,
                                         float *mdctSpectrumLeft,
                                         float *mdctSpectrumRight,
                                         const float *mdstSpectrumDmx,
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
                                         struct STEREO_INFO *toolsInfo) {
  int sfb, sfboffs;
  int msMaskTrueSomewhere = 0;
  int msMaskFalseSomewhere = 0;
  int numMsMaskTrue = 0;
  int numMsMaskFalse = 0;
  int numPredCoeffsNonzeroRe = 0;
  int numPredCoeffsNonzeroIm = 0;
  int numBitsCplxlPred = 0;
  int bUseBasicMs = 0;
  float sfbThresholdMS[MAX_GROUPED_SFB];
  float sfbEnergyDmx[MAX_GROUPED_SFB];
  float sfbEnergyRes[MAX_GROUPED_SFB];
  float sfbEnergyResRealOnly[MAX_GROUPED_SFB];
  float sfbPredictionCoeffRealOnly[MAX_GROUPED_SFB];
  float sfbPredictionCoeffRe[MAX_GROUPED_SFB];
  float sfbPredictionCoeffIm[MAX_GROUPED_SFB];

  int predCoefRealOnlyQ[MAX_GROUPED_SFB];

  float sumEnergyMid = 0.0f;
  float sumEnergySide = 0.0f;
  float sumEnergyRes = 0.0f;
  float sumEnergyResRealOnly = 0.0f;

  int numBandsPredActive = 0;
  int deltaBits = 0;
  int deltaBitsRealOnly = 0;

  float minCplxGain;

  ALIGN_16_BYTE float mdctSpectrumMid[MAX_GRANULE_LEN];
  ALIGN_16_BYTE float mdctSpectrumSide[MAX_GRANULE_LEN];
  ALIGN_16_BYTE float mdctSpectrumRes[MAX_GRANULE_LEN];
  ALIGN_16_BYTE float mdctSpectrumResRealOnly[MAX_GRANULE_LEN];

  setFLOAT(0.0f, mdctSpectrumRes, MAX_GRANULE_LEN);
  setFLOAT(0.0f, mdctSpectrumResRealOnly, MAX_GRANULE_LEN);

  toolsInfo->bCplxPredMdctActive = 0;
  setINT(0, toolsInfo->predCoefReQ, MAX_GROUPED_SFB);
  setINT(0, toolsInfo->predCoefImQ, MAX_GROUPED_SFB);
  setINT(0, predCoefRealOnlyQ, MAX_GROUPED_SFB);
  setINT(0, isBook, sfbCnt);

  if (fDualMono) {
    *msDigest = MS_NONE;
    setINT(0, msMask, sfbCnt);
    toolsInfo->bCplxPredMdctResetPredictors = 1;
    return;
  }

  toolsInfo->bCplxPredMdctRealOnly = 0;
  toolsInfo->sfbPerPredBand = SFB_PER_PRED_BAND;

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb += SFB_PER_PRED_BAND) {
      float predictionCoeffRealOnly, predictionCoeffRe, predictionCoeffIm;

      int i;
      int startLine = sfbOffset[sfb];
      int endLine = sfbOffset[min(sfb + SFB_PER_PRED_BAND, sfboffs + maxSfbPerGroup)];

      for (i = startLine; i < endLine; i++) {
        mdctSpectrumMid[i] = (mdctSpectrumLeft[i] + mdctSpectrumRight[i]) * 0.5f;
        mdctSpectrumSide[i] = (mdctSpectrumLeft[i] - mdctSpectrumRight[i]) * 0.5f;
      }

      predictionCoeffRealOnly = iisaacfenc_calcComplexPredictionCoefficient(&mdctSpectrumMid[startLine],
                                                                            &mdctSpectrumSide[startLine],
                                                                            &mdstSpectrumDmx[startLine],
                                                                            &predictionCoeffRe,
                                                                            &predictionCoeffIm,
                                                                            endLine - startLine,
                                                                            toolsInfo->bSwap);

      sfbPredictionCoeffRealOnly[sfb] = predictionCoeffRealOnly;
      sfbPredictionCoeffRe[sfb] = predictionCoeffRe;
      sfbPredictionCoeffIm[sfb] = predictionCoeffIm;

      iisaacfenc_QuantizeCplxPredCoeffs(predictionCoeffRe, predictionCoeffIm,
                                        &toolsInfo->predCoefReQ[sfb], &toolsInfo->predCoefImQ[sfb]);

      iisaacfenc_QuantizeRealPredCoeffs(predictionCoeffRealOnly, &predCoefRealOnlyQ[sfb]);

      if (sfb + 1 < sfboffs + maxSfbPerGroup) {
        toolsInfo->predCoefReQ[sfb + 1] = toolsInfo->predCoefReQ[sfb];
        toolsInfo->predCoefImQ[sfb + 1] = toolsInfo->predCoefImQ[sfb];
        predCoefRealOnlyQ[sfb + 1] = predCoefRealOnlyQ[sfb];
      }
    }
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      float predictionCoeffRealQuantInv;
      float predictionCoeffImagQuantInv;
      float predictionCoeffRealOnlyQuantInv;

      int i;

      int startLine = sfbOffset[sfb];
      int endLine = sfbOffset[sfb + 1];

      sfbEnergyDmx[sfb] = 0.0f;
      sfbEnergyRes[sfb] = 0.0f;
      sfbEnergyResRealOnly[sfb] = 0.0f;

      sfbThresholdMS[sfb] = min(sfbThresholdLeft[sfb], sfbThresholdRight[sfb]);

      iisaacfenc_DeQuantizeCplxPredCoeffs(toolsInfo->predCoefReQ[sfb], toolsInfo->predCoefImQ[sfb],
                                          &predictionCoeffRealQuantInv, &predictionCoeffImagQuantInv);
      iisaacfenc_DeQuantizeRealPredCoeffs(predCoefRealOnlyQ[sfb], &predictionCoeffRealOnlyQuantInv);

      if (toolsInfo->bSwap == 0) {
        for (i = startLine; i < endLine; i++) {
          mdctSpectrumRes[i] = mdctSpectrumSide[i] - mdctSpectrumMid[i] * predictionCoeffRealQuantInv - mdstSpectrumDmx[i] * predictionCoeffImagQuantInv;
          sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];

          mdctSpectrumResRealOnly[i] = mdctSpectrumSide[i] - mdctSpectrumMid[i] * predictionCoeffRealOnlyQuantInv;
          sfbEnergyResRealOnly[sfb] += mdctSpectrumResRealOnly[i] * mdctSpectrumResRealOnly[i];
        }

        sfbEnergyDmx[sfb] = sfbEnergyMid[sfb];
        toolsInfo->invPredGain[sfb] = min(1.0f, sfbEnergyRes[sfb] / (sfbEnergySide[sfb] + FLT_MIN));
      } else {
        for (i = startLine; i < endLine; i++) {
          mdctSpectrumRes[i] = mdctSpectrumMid[i] - mdctSpectrumSide[i] * predictionCoeffRealQuantInv - mdstSpectrumDmx[i] * predictionCoeffImagQuantInv;
          sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];

          mdctSpectrumResRealOnly[i] = mdctSpectrumMid[i] - mdctSpectrumSide[i] * predictionCoeffRealOnlyQuantInv;
          sfbEnergyResRealOnly[sfb] += mdctSpectrumResRealOnly[i] * mdctSpectrumResRealOnly[i];
        }

        sfbEnergyDmx[sfb] = sfbEnergySide[sfb];
        toolsInfo->invPredGain[sfb] = min(1.0f, sfbEnergyRes[sfb] / (sfbEnergyMid[sfb] + FLT_MIN));
      }

      sumEnergyRes += sfbEnergyRes[sfb];
      sumEnergyMid += sfbEnergyMid[sfb];
      sumEnergySide += sfbEnergySide[sfb];
      sumEnergyResRealOnly += sfbEnergyResRealOnly[sfb];
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

      if (penaltyPred > penaltyLR) {
        msMask[sfb] = PRED_ON;
        numMsMaskTrue += SFB_PER_PRED_BAND;
        msMaskTrueSomewhere = 1;

        if (toolsInfo->predCoefReQ[sfb] != 0) {
          numPredCoeffsNonzeroRe++;
        }
        if (toolsInfo->predCoefImQ[sfb] != 0) {
          numPredCoeffsNonzeroIm++;
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

  numBandsPredActive = 0;
  deltaBits = 0;
  deltaBitsRealOnly = 0;

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if (msMask[sfb] == PRED_ON) {
        float maxNrgLR = max(sfbEnergyLeft[sfb], sfbEnergyRight[sfb]);
        float maxNrgDmxRes = max(sfbEnergyDmx[sfb], sfbEnergyRes[sfb]);
        float tmpThrMs;

        if ((maxNrgDmxRes > FLT_MIN) && (maxNrgDmxRes < maxNrgLR)) {
          tmpThrMs = sfbThresholdMS[sfb] * (maxNrgDmxRes + FLT_MIN) / (maxNrgLR + FLT_MIN);
        } else {
          tmpThrMs = sfbThresholdMS[sfb];
        }

        numBandsPredActive++;

        deltaBits += iisaacfenc_estimateDeltaRateLines(toolsInfo->bSwap ? &mdctSpectrumMid[sfbOffset[sfb]] : &mdctSpectrumSide[sfbOffset[sfb]],
                                                       &mdctSpectrumRes[sfbOffset[sfb]],
                                                       tmpThrMs,
                                                       sfbOffset[sfb + 1] - sfbOffset[sfb]);

        deltaBitsRealOnly += iisaacfenc_estimateDeltaRateLines(toolsInfo->bSwap ? &mdctSpectrumMid[sfbOffset[sfb]] : &mdctSpectrumSide[sfbOffset[sfb]],
                                                               &mdctSpectrumResRealOnly[sfbOffset[sfb]],
                                                               tmpThrMs,
                                                               sfbOffset[sfb + 1] - sfbOffset[sfb]);
      }
    }
  }

  if (toolsInfo->bCplxPredMdctActivePrev) {
    if (toolsInfo->bCplxPredMdctRealOnlyPrev) {
      minCplxGain = 3.0f;
    } else {
      minCplxGain = 0.75f;
    }
  } else {
    minCplxGain = 3.0f;
  }

  if (deltaBits < numBandsPredActive * 2) {
    bUseBasicMs = 1;
  }

  if (numPredCoeffsNonzeroIm == 0) {
    toolsInfo->bCplxPredMdctRealOnly = 1;
  }

  else if ((numPredCoeffsNonzeroIm < 4) || (minCplxGain * sumEnergyRes > sumEnergyResRealOnly) || ((float)deltaBits < (float)deltaBitsRealOnly * (toolsInfo->bCplxPredMdctRealOnlyPrev ? 1.1f : 0.75f))) {
    if ((1.625f * sumEnergyRes < (toolsInfo->bSwap ? sumEnergyMid : sumEnergySide)) || (deltaBitsRealOnly > numBandsPredActive * 2)) {
      toolsInfo->bCplxPredMdctRealOnly = 1;
      copyFLOAT(mdctSpectrumResRealOnly, mdctSpectrumRes, MAX_GRANULE_LEN);
      setINT(0, toolsInfo->predCoefImQ, MAX_GROUPED_SFB);
      sumEnergyRes = sumEnergyResRealOnly;

      for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
        for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
          if (msMask[sfb] == PRED_ON) {
            toolsInfo->predCoefReQ[sfb] = predCoefRealOnlyQ[sfb];
            sfbPredictionCoeffRe[sfb] = sfbPredictionCoeffRealOnly[sfb];
            sfbEnergyRes[sfb] = sfbEnergyResRealOnly[sfb];
            if (toolsInfo->bSwap) {
              toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb]) / (sfbEnergyMid[sfb] + FLT_MIN));
            } else {
              toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb]) / (sfbEnergySide[sfb] + FLT_MIN));
            }
          }
        }
      }
    }
  }

  if (toolsInfo->bCplxPredMdctRealOnly) {
    numBitsCplxlPred = iisaacfenc_rdOptimizeRealPred(sfbCnt,
                                                     sfbPerGroup,
                                                     maxSfbPerGroup,
                                                     msMask,
                                                     sfbPredictionCoeffRe,
                                                     toolsInfo->predCoefReQ,
                                                     toolsInfo->predCoefPrevReQ,
                                                     toolsInfo->bCplxPredMdctResetPredictors);
  } else {
    numBitsCplxlPred = iisaacfenc_rdOptimizeCplxPred(sfbCnt,
                                                     sfbPerGroup,
                                                     maxSfbPerGroup,
                                                     msMask,
                                                     sfbPredictionCoeffRe,
                                                     toolsInfo->predCoefReQ,
                                                     toolsInfo->predCoefPrevReQ,
                                                     sfbPredictionCoeffIm,
                                                     toolsInfo->predCoefImQ,
                                                     toolsInfo->predCoefPrevImQ,
                                                     toolsInfo->bCplxPredMdctResetPredictors);
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if (msMask[sfb] == PRED_COEFF_CHANGED) {
        float predictionCoeffRealQuantInv;
        float predictionCoeffImagQuantInv;
        int i;

        int startLine = sfbOffset[sfb];
        int endLine = sfbOffset[sfb + 1];

        msMask[sfb] = PRED_ON;
        sumEnergyRes -= sfbEnergyRes[sfb];

        sfbEnergyRes[sfb] = 0.0f;

        iisaacfenc_DeQuantizeCplxPredCoeffs(toolsInfo->predCoefReQ[sfb], toolsInfo->predCoefImQ[sfb],
                                            &predictionCoeffRealQuantInv, &predictionCoeffImagQuantInv);

        if (toolsInfo->bSwap == 0) {
          for (i = startLine; i < endLine; i++) {
            mdctSpectrumRes[i] = mdctSpectrumSide[i] - mdctSpectrumMid[i] * predictionCoeffRealQuantInv - mdstSpectrumDmx[i] * predictionCoeffImagQuantInv;
            sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
          }
          toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb]) / (sfbEnergySide[sfb] + FLT_MIN));
        } else {
          for (i = startLine; i < endLine; i++) {
            mdctSpectrumRes[i] = mdctSpectrumMid[i] - mdctSpectrumSide[i] * predictionCoeffRealQuantInv - mdstSpectrumDmx[i] * predictionCoeffImagQuantInv;
            sfbEnergyRes[sfb] += mdctSpectrumRes[i] * mdctSpectrumRes[i];
          }
          toolsInfo->invPredGain[sfb] = min(1.0f, (sfbEnergyRes[sfb]) / (sfbEnergyMid[sfb] + FLT_MIN));
        }

        sumEnergyRes += sfbEnergyRes[sfb];
      }
    }
  }

  if (*msDigest != MS_NONE) {
    float hysteresisBitFac = 0.75f;
    if (toolsInfo->bCplxPredMdctActivePrev) {
      hysteresisBitFac = 1.25f;
    }

    if ((numPredCoeffsNonzeroRe < 4) && (numPredCoeffsNonzeroIm < 4)) {
      bUseBasicMs = 1;
    }

    else if (hysteresisBitFac * (float)(toolsInfo->bCplxPredMdctRealOnly ? deltaBitsRealOnly : deltaBits) < (float)numBitsCplxlPred) {
      bUseBasicMs = 1;
    }
  }

  if (bUseBasicMs) {
    toolsInfo->bCplxPredMdctActive = 0;
    toolsInfo->bSwap = 0;
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
        toolsInfo->predCoefImQ[sfb] = 0;
        msMask[sfb] = 0;
      }
    }
  }
}
