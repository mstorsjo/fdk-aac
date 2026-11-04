
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

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

#include "mathlib.h"
#include "quantize.h"
#include "cpuinfo.h"
#include "bit_cnt.h"
#include "qc_data.h"
#include "quantize.h"
#include "psy_const.h"
#include "glob_con.h"
#include "sf_estim.h"

static const int MAX_SCF_DELTA = 60;
static const float C1 = -69.33295f;
static const float C2 = 5.77078f;
static const float LOG2_1 = 1.442695041f;

static const float PE_C1 = 3.0f;
static const float PE_C2 = 1.3219281f;
static const float PE_C3 = 0.5593573f;

static const int maxAllowedScf = 255 + 4 * LOG_NORM_PCM - 100;
static const int minAllowedScf = 0 + 4 * LOG_NORM_PCM - 100;

#define SCF_EMPTY_BAND INT_MIN

struct sf_estim_data {
  void (*CalcFormFactorChannel_Ptr)(float *const sfbFormFactor,
                                    float const *const mdctSpectrum,
                                    int const sfbCnt,
                                    int const *const sfbOffsets);
};

typedef enum {
  LIMIT_MAX_SCF = 0,
  LIMIT_MIN_SCF = 1,
  LIMIT_BOTH = 2
} SCF_LIMIT_MODE;

static void limitScalefactor(
    int *scf,
    int *minSf,
    int *maxSf,
    int sfbCnt,
    SCF_LIMIT_MODE scaleMode) {
  int i = 0;

  switch (scaleMode) {
    case LIMIT_MAX_SCF:

      if (*minSf != INT_MAX) {
        *maxSf = *minSf + MAX_SCF_DELTA;
      }

      for (i = 0; i < sfbCnt; i++) {
        if ((scf[i] != SCF_EMPTY_BAND) && (scf[i] > *maxSf)) {
          scf[i] = *maxSf;
        }
      }
      break;

    case LIMIT_MIN_SCF:

      if (*minSf != INT_MIN) {
        *minSf = *maxSf - MAX_SCF_DELTA;
      }

      for (i = 0; i < sfbCnt; i++) {
        if ((scf[i] != SCF_EMPTY_BAND) && (scf[i] < *minSf)) {
          scf[i] = *minSf;
        }
      }
      break;

    default:
      assert(0);
      break;
  }
}

static void iisaacfenc_CalcFormFactorChannel_NoOpt(float *const sfbFormFactor,
                                                   float const *const mdctSpectrum,
                                                   int const sfbCnt,
                                                   int const *const sfbOffsets) {
  int i;

  for (i = 0; i < sfbCnt; i++) {
    int j;

    sfbFormFactor[i] = FLT_MIN;

    for (j = sfbOffsets[i]; j < sfbOffsets[i + 1]; j++) {
      sfbFormFactor[i] += (float)sqrt(fabs(mdctSpectrum[j]));
    }
  }
}

static void iisaacfenc_getMinMaxScf(
    const int *scf,
    const int sfbCnt,
    int *minSf,
    int *maxSf) {
  int i = 0;
  int minSf_local = INT_MAX;
  int maxSf_local = SCF_EMPTY_BAND;

  for (i = 0; i < sfbCnt; i++) {
    if (scf[i] != SCF_EMPTY_BAND) {
      minSf_local = min(scf[i], minSf_local);
      maxSf_local = max(scf[i], maxSf_local);
    }
  }

  if (NULL != minSf) {
    *minSf = minSf_local;
  }

  if (NULL != maxSf) {
    *maxSf = maxSf_local;
  }
}

static float iisaacfenc_reCalcSfbDistAndSignQuantSpec(QUANTIZER_DATA const *const quantizerData,
                                                      float const *const mdctSpectrum,
                                                      float const *const expSpec,
                                                      int const useLloydMaxQuantizer,
                                                      int *const quantSpec,
                                                      int const noOfLines,
                                                      int const gain) {
  int i = 0;
  float sfbDist = FLT_MAX;

  iisaacfenc_calcSfbDist(quantizerData,
                         mdctSpectrum,
                         expSpec,
                         useLloydMaxQuantizer,
                         quantSpec,
                         noOfLines,
                         gain,
                         &sfbDist);

  for (i = 0; i < noOfLines; i++) {
    if (mdctSpectrum[i] < 0) {
      quantSpec[i] = -quantSpec[i];
    }
  }

  return sfbDist;
}

static int iisaacfenc_improveScf(QUANTIZER_DATA const *const quantizerData,
                                 float const *const spec,
                                 float const *const expSpec,
                                 int const useLloydMaxQuantizer,
                                 int *const quantSpec,
                                 int *const quantSpecTmp,
                                 int const sfbWidth,
                                 float const thresh,
                                 int scf,
                                 int const minScf,
                                 float *const dist,
                                 int *const minScfCalculated) {
  float sfbDist = FLT_MAX;
  int scfBest = scf;
  int j = 0;

  iisaacfenc_calcSfbDist(quantizerData,
                         spec,
                         expSpec,
                         useLloydMaxQuantizer,
                         quantSpec,
                         sfbWidth,
                         scf,
                         &sfbDist);
  *minScfCalculated = scf;

  if (sfbDist > 1.25f * thresh) {
    int scfEstimated = scf;
    float sfbDistBest = sfbDist;
    int cnt = 0;

    while ((sfbDist > 1.25f * thresh) && (cnt++ < 3) && (scf < maxAllowedScf)) {
      scf++;
      iisaacfenc_calcSfbDist(quantizerData,
                             spec,
                             expSpec,
                             useLloydMaxQuantizer,
                             quantSpecTmp,
                             sfbWidth,
                             scf,
                             &sfbDist);
      if (sfbDist < sfbDistBest) {
        scfBest = scf;
        sfbDistBest = sfbDist;
        copyINT(quantSpecTmp, quantSpec, sfbWidth);
      }
    }

    cnt = 0;
    scf = scfEstimated;
    sfbDist = sfbDistBest;
    while ((sfbDist > 1.25f * thresh) && (cnt++ < 1) && (scf > minScf)) {
      scf--;
      iisaacfenc_calcSfbDist(quantizerData,
                             spec,
                             expSpec,
                             useLloydMaxQuantizer,
                             quantSpecTmp,
                             sfbWidth,
                             scf,
                             &sfbDist);
      if (sfbDist < sfbDistBest) {
        scfBest = scf;
        sfbDistBest = sfbDist;
        copyINT(quantSpecTmp, quantSpec, sfbWidth);
      }
      *minScfCalculated = scf;
    }
    *dist = sfbDistBest;
  } else {
    float sfbDistBest = sfbDist;
    float sfbDistAllowed = min(sfbDist * 1.25f, thresh);
    int cnt = 0;
    for (cnt = 0; cnt < 3; cnt++) {
      scf++;
      if (scf <= maxAllowedScf) {
        iisaacfenc_calcSfbDist(quantizerData,
                               spec,
                               expSpec,
                               useLloydMaxQuantizer,
                               quantSpecTmp,
                               sfbWidth,
                               scf,
                               &sfbDist);
        if (sfbDist < sfbDistAllowed) {
          *minScfCalculated = scfBest + 1;
          scfBest = scf;
          sfbDistBest = sfbDist;
          copyINT(quantSpecTmp, quantSpec, sfbWidth);
        }
      }
    }
    *dist = sfbDistBest;
  }

  for (j = 0; j < sfbWidth; j++) {
    if (spec[j] < 0) {
      quantSpec[j] = -quantSpec[j];
    }
  }

  return scfBest;
}

static int iisaacfenc_countSingleScfBits(
    const int scf,
    const int scfLeft,
    const int scfRight) {
  int scfBits = 0;
  int bitDemandLeft = 0;
  int bitDemandRight = 0;

  bitDemandLeft = iisaacfenc_bitCountScalefactorDelta(scfLeft - scf);
  bitDemandRight = iisaacfenc_bitCountScalefactorDelta(scf - scfRight);

  scfBits = bitDemandLeft + bitDemandRight;

  return scfBits;
}

static float iisaacfenc_calcSingleSpecPe(
    const int scf,
    const float sfbConstPePart,
    const float nLines) {
  float specPe = 0.0f;
  float ldRatio = 0.0f;

  ldRatio = sfbConstPePart - 0.375f * scf;
  if (ldRatio >= PE_C1) {
    specPe = 0.7f * nLines * ldRatio;
  } else {
    specPe = 0.7f * nLines * (PE_C2 + PE_C3 * ldRatio);
  }

  return specPe;
}

static int iisaacfenc_countScfBitsDiff(
    const int *scfOld,
    const int *scfNew,
    const int sfbCnt,
    const int startSfb,
    const int stopSfb) {
  int scfBitsDiff = 0;
  int sfb = 0;
  int sfbLast = 0;
  int sfbPrev = 0;
  int sfbNext = 0;

  sfbLast = startSfb;
  while ((sfbLast < stopSfb) && (scfOld[sfbLast] == SCF_EMPTY_BAND)) {
    sfbLast++;
  }

  sfbPrev = startSfb - 1;
  while ((sfbPrev >= 0) && (scfOld[sfbPrev] == SCF_EMPTY_BAND)) {
    sfbPrev--;
  }
  if (sfbPrev >= 0) {
    scfBitsDiff += iisaacfenc_bitCountScalefactorDelta(scfNew[sfbPrev] - scfNew[sfbLast]) -
                   iisaacfenc_bitCountScalefactorDelta(scfOld[sfbPrev] - scfOld[sfbLast]);
  }

  for (sfb = sfbLast + 1; sfb < stopSfb; sfb++) {
    if (scfOld[sfb] != SCF_EMPTY_BAND) {
      scfBitsDiff += iisaacfenc_bitCountScalefactorDelta(scfNew[sfbLast] - scfNew[sfb]) -
                     iisaacfenc_bitCountScalefactorDelta(scfOld[sfbLast] - scfOld[sfb]);
      sfbLast = sfb;
    }
  }

  sfbNext = stopSfb;
  while ((sfbNext < sfbCnt) && (scfOld[sfbNext] == SCF_EMPTY_BAND)) {
    sfbNext++;
  }
  if (sfbNext < sfbCnt) {
    scfBitsDiff += iisaacfenc_bitCountScalefactorDelta(scfNew[sfbLast] - scfNew[sfbNext]) -
                   iisaacfenc_bitCountScalefactorDelta(scfOld[sfbLast] - scfOld[sfbNext]);
  }
  return scfBitsDiff;
}

static float iisaacfenc_calcSpecPeDiff(
    const int *scfOld,
    const int *scfNew,
    float *sfbConstPePart,
    const float *sfbNRelevantLines,
    const int startSfb,
    const int stopSfb) {
  float specPeDiff = 0.0f;
  int sfb = 0;

  for (sfb = startSfb; sfb < stopSfb; sfb++) {
    if (scfOld[sfb] != SCF_EMPTY_BAND) {
      float ldRatioOld = 0.0f;
      float ldRatioNew = 0.0f;
      float pOld = 0.0f;
      float pNew = 0.0f;
      ldRatioOld = sfbConstPePart[sfb] - 0.375f * scfOld[sfb];
      ldRatioNew = sfbConstPePart[sfb] - 0.375f * scfNew[sfb];
      if (ldRatioOld >= PE_C1) {
        pOld = ldRatioOld;
      } else {
        pOld = PE_C2 + PE_C3 * ldRatioOld;
      }
      if (ldRatioNew >= PE_C1) {
        pNew = ldRatioNew;
      } else {
        pNew = PE_C2 + PE_C3 * ldRatioNew;
      }
      specPeDiff += 0.7f * sfbNRelevantLines[sfb] * (pNew - pOld);
    }
  }

  return specPeDiff;
}

static void iisaacfenc_assimilateSingleScf(
    PSY_OUT_CHANNEL *psyOutChan,
    float *expSpec,
    const int useLloydMaxQuantizer,
    int *quantSpec,
    int *quantSpecTmp,
    int *scf,
    const int *minScf,
    float *sfbDist,
    float *sfbConstPePart,
    const float *sfbNRelevantLines,
    int *minScfCalculated,
    const int restartOnSuccess) {
  int sfbLast = -1;
  int sfbAct = -1;
  int sfbNext = -1;
  int scfAct = 0;
  int *scfLast = NULL;
  int *scfNext = NULL;
  int scfMin = INT_MAX;
  int scfMax = INT_MAX;
  int sfbWidth = 0;
  int sfbOffs = 0;
  float sfbPeOld = 0.0f;
  float sfbPeNew = 0.0f;
  float sfbDistNew = 0.0f;
  int j = 0;
  int success = 0;
  float deltaPe = 0.0f;
  float deltaPeNew = 0.0f;
  float deltaPeTmp = 0.0f;
  int updateMinScfCalculated = 0;
  int prevScfLast[MAX_GROUPED_SFB] = {0};
  int prevScfNext[MAX_GROUPED_SFB] = {0};
  float deltaPeLast[MAX_GROUPED_SFB] = {0.0f};

  setINT(INT_MAX, prevScfLast, psyOutChan->sfbActive);
  setINT(INT_MAX, prevScfNext, psyOutChan->sfbActive);
  setFLOAT(FLT_MAX, deltaPeLast, psyOutChan->sfbActive);

  do {
    sfbNext++;
    while ((sfbNext < psyOutChan->sfbCnt) && (scf[sfbNext] == SCF_EMPTY_BAND)) {
      sfbNext++;
    }
    if ((sfbLast >= 0) && (sfbAct >= 0) && (sfbNext < psyOutChan->sfbCnt)) {
      scfAct = scf[sfbAct];
      scfLast = scf + sfbLast;
      scfNext = scf + sfbNext;
      scfMin = min(*scfLast, *scfNext);
      scfMax = max(*scfLast, *scfNext);
    } else if ((sfbLast == -1) && (sfbAct >= 0) && (sfbNext < psyOutChan->sfbCnt)) {
      scfAct = scf[sfbAct];
      scfLast = &scfAct;
      scfNext = scf + sfbNext;
      scfMin = *scfNext;
      scfMax = *scfNext;
    } else if ((sfbLast >= 0) && (sfbAct >= 0) && (sfbNext == psyOutChan->sfbCnt)) {
      scfAct = scf[sfbAct];
      scfLast = scf + sfbLast;
      scfNext = &scfAct;
      scfMin = *scfLast;
      scfMax = *scfLast;
    }
    if (sfbAct >= 0) {
      scfMin = max(scfMin, minScf[sfbAct]);
    }

    if (sfbAct >= 0 && sfbAct < psyOutChan->sfbActive) {
      if ((sfbLast >= 0 || sfbNext < psyOutChan->sfbCnt) &&
          (scfAct > scfMin) &&
          (scfAct <= scfMin + MAX_SCF_DELTA) &&
          (scfAct >= scfMax - MAX_SCF_DELTA) &&
          (*scfLast != prevScfLast[sfbAct] ||
           *scfNext != prevScfNext[sfbAct] ||
           deltaPe < deltaPeLast[sfbAct])) {
        success = 0;

        sfbWidth = psyOutChan->sfbOffsets[sfbAct + 1] - psyOutChan->sfbOffsets[sfbAct];
        sfbOffs = psyOutChan->sfbOffsets[sfbAct];

        sfbPeOld = iisaacfenc_calcSingleSpecPe(scfAct,
                                               sfbConstPePart[sfbAct],
                                               sfbNRelevantLines[sfbAct]) +
                   iisaacfenc_countSingleScfBits(scfAct, *scfLast, *scfNext);
        deltaPeNew = deltaPe;
        updateMinScfCalculated = 1;
        do {
          scfAct--;

          if (scfAct < minScfCalculated[sfbAct] && scfAct >= scfMax - MAX_SCF_DELTA) {
            sfbPeNew = iisaacfenc_calcSingleSpecPe(scfAct,
                                                   sfbConstPePart[sfbAct],
                                                   sfbNRelevantLines[sfbAct]) +
                       iisaacfenc_countSingleScfBits(scfAct, *scfLast, *scfNext);

            deltaPeTmp = deltaPe + sfbPeNew - sfbPeOld;
            if (deltaPeTmp < 10.0f) {
              iisaacfenc_calcSfbDist(psyOutChan->quantizerData,
                                     psyOutChan->mdctSpectrum + sfbOffs,
                                     expSpec + sfbOffs,
                                     useLloydMaxQuantizer,
                                     quantSpecTmp + sfbOffs,
                                     sfbWidth,
                                     scfAct,
                                     &sfbDistNew);
              if (sfbDistNew < sfbDist[sfbAct]) {
                scf[sfbAct] = scfAct;
                sfbDist[sfbAct] = sfbDistNew;
                copyINT(quantSpecTmp + sfbOffs, quantSpec + sfbOffs, sfbWidth);

                for (j = sfbOffs; j < sfbOffs + sfbWidth; j++) {
                  if (psyOutChan->mdctSpectrum[j] < 0.0f) {
                    quantSpec[j] = -quantSpec[j];
                  }
                }
                deltaPeNew = deltaPeTmp;
                success = 1;
              }

              if (updateMinScfCalculated) {
                minScfCalculated[sfbAct] = scfAct;
              }
            } else {
              updateMinScfCalculated = 0;
            }
          }
        } while (scfAct > scfMin);

        deltaPe = deltaPeNew;

        prevScfLast[sfbAct] = *scfLast;
        prevScfNext[sfbAct] = *scfNext;
        deltaPeLast[sfbAct] = deltaPe;
      }
    }
    if (success && restartOnSuccess) {
      sfbLast = -1;
      sfbAct = -1;
      sfbNext = -1;
      scfLast = 0;
      scfNext = 0;
      scfMin = INT_MAX;
      scfMax = INT_MAX;
      success = 0;
    } else {
      sfbLast = sfbAct;
      sfbAct = sfbNext;
    }
  } while (sfbNext < psyOutChan->sfbCnt);
}

static void iisaacfenc_assimilateMultipleScf(
    PSY_OUT_CHANNEL *psyOutChan,
    float *expSpec,
    const int useLloydMaxQuantizer,
    int *quantSpec,
    int *quantSpecTmp,
    int *scf,
    const int *minScf,
    float *sfbDist,
    float *sfbConstPePart,
    const float *sfbNRelevantLines) {
  int sfb = 0;
  int startSfb = 0;
  int stopSfb = 0;
  int scfTmp[MAX_GROUPED_SFB] = {0};
  int scfMin = 0;
  int scfMax = 0;
  int scfAct = 0;
  int possibleRegionFound = 0;
  int sfbWidth = 0;
  int sfbOffs = 0;
  int j = 0;
  float sfbDistNew[MAX_GROUPED_SFB] = {0.0f};
  float distOldSum = 0.0f;
  float distNewSum = 0.0f;
  int deltaScfBits = 0;
  float deltaSpecPe = 0.0f;
  float deltaPe = 0.0f;
  float deltaPeNew = 0.0f;
  int sfbCnt = 0;

  sfbCnt = psyOutChan->sfbCnt;

  scfMin = INT_MAX;
  scfMax = SCF_EMPTY_BAND;
  for (sfb = 0; sfb < sfbCnt; sfb++) {
    if (scf[sfb] != SCF_EMPTY_BAND) {
      scfMin = min(scfMin, scf[sfb]);
      scfMax = max(scfMax, scf[sfb]);
    }
  }

  if ((scfMin != scfMax) && (scfMax != SCF_EMPTY_BAND) && (scfMax <= scfMin + MAX_SCF_DELTA)) {
    scfAct = scfMax;

    do {
      scfAct--;
      copyINT(scf, scfTmp, MAX_GROUPED_SFB);
      stopSfb = 0;
      do {
        sfb = stopSfb;
        while (sfb < sfbCnt && (scf[sfb] == SCF_EMPTY_BAND || scf[sfb] <= scfAct)) {
          sfb++;
        }
        startSfb = sfb;
        sfb++;
        while (sfb < sfbCnt && (scf[sfb] == SCF_EMPTY_BAND || scf[sfb] > scfAct)) {
          sfb++;
        }
        stopSfb = sfb;

        possibleRegionFound = 0;
        if (startSfb < sfbCnt) {
          possibleRegionFound = 1;
          for (sfb = startSfb; sfb < stopSfb; sfb++) {
            if (scf[sfb] != SCF_EMPTY_BAND)
              if (scfAct < minScf[sfb]) {
                possibleRegionFound = 0;
                break;
              }
          }
        }

        if (possibleRegionFound != 0) {
          for (sfb = startSfb; sfb < stopSfb; sfb++) {
            if (scfTmp[sfb] != SCF_EMPTY_BAND) {
              scfTmp[sfb] = scfAct;
            }
          }

          deltaScfBits = iisaacfenc_countScfBitsDiff(scf,
                                                     scfTmp,
                                                     sfbCnt,
                                                     startSfb,
                                                     stopSfb);

          deltaSpecPe = iisaacfenc_calcSpecPeDiff(scf,
                                                  scfTmp,
                                                  sfbConstPePart,
                                                  sfbNRelevantLines,
                                                  startSfb,
                                                  stopSfb);

          deltaPeNew = deltaPe + deltaScfBits + deltaSpecPe;

          if (deltaPeNew < 10.0f) {
            distOldSum = distNewSum = 0.0f;
            for (sfb = startSfb; sfb < stopSfb; sfb++) {
              if (scfTmp[sfb] != SCF_EMPTY_BAND) {
                distOldSum += sfbDist[sfb];

                sfbWidth = psyOutChan->sfbOffsets[sfb + 1] - psyOutChan->sfbOffsets[sfb];
                sfbOffs = psyOutChan->sfbOffsets[sfb];
                iisaacfenc_calcSfbDist(psyOutChan->quantizerData,
                                       psyOutChan->mdctSpectrum + sfbOffs,
                                       expSpec + sfbOffs,
                                       useLloydMaxQuantizer,
                                       quantSpecTmp + sfbOffs,
                                       sfbWidth,
                                       scfAct,
                                       &sfbDistNew[sfb]);

                if (sfbDistNew[sfb] > psyOutChan->sfbThreshold[sfb]) {
                  distNewSum = 2.0f * distOldSum;
                  break;
                }
                distNewSum += sfbDistNew[sfb];
              }
            }

            if (distNewSum < distOldSum) {
              deltaPe = deltaPeNew;
              for (sfb = startSfb; sfb < stopSfb; sfb++) {
                if (scf[sfb] != SCF_EMPTY_BAND) {
                  sfbWidth = psyOutChan->sfbOffsets[sfb + 1] - psyOutChan->sfbOffsets[sfb];
                  sfbOffs = psyOutChan->sfbOffsets[sfb];
                  scf[sfb] = scfAct;
                  sfbDist[sfb] = sfbDistNew[sfb];
                  copyINT(quantSpecTmp + sfbOffs, quantSpec + sfbOffs, sfbWidth);

                  for (j = sfbOffs; j < sfbOffs + sfbWidth; j++) {
                    if (psyOutChan->mdctSpectrum[j] < 0.0f) {
                      quantSpec[j] = -quantSpec[j];
                    }
                  }
                }
              }
            }
          }
        }
      } while (stopSfb <= sfbCnt);
    } while (scfAct > scfMin);
  }
}

static void iisaacfenc_assimilateMultipleScf2(
    PSY_OUT_CHANNEL *psyOutChan,
    float *expSpec,
    const int useLloydMaxQuantizer,
    int *quantSpec,
    int *quantSpecTmp,
    int *scf,
    const int *minScf,
    float *sfbDist,
    float *sfbConstPePart,
    const float *sfbNRelevantLines,
    const int bNoiseFilling) {
  int sfb = 0;
  int startSfb = 0;
  int stopSfb = 0;
  int scfTmp[MAX_GROUPED_SFB] = {0};
  int scfAct = 0;
  int scfNew = 0;
  int scfPrev = 0;
  int scfNext = 0;
  int scfPrevNextMin = 0;
  int scfPrevNextMax = 0;
  int scfLo = 0;
  int scfHi = 0;
  int scfMin = 0;
  int scfMax = 0;
  int *sfbOffs = psyOutChan->sfbOffsets;
  int j = 0;
  float sfbDistNew[MAX_GROUPED_SFB] = {0.0f};
  float sfbDistMax[MAX_GROUPED_SFB] = {0.0f};
  float distOldSum = 0.0f;
  float distNewSum = 0.0f;
  int deltaScfBits = 0;
  float deltaSpecPe = 0.0f;
  float deltaPe = 0.0f;
  float deltaPeNew = 0.0f;
  int sfbCnt = psyOutChan->sfbActive;
  int bSuccess = 0;
  int bCheckScf = 0;
  int i = 0;

  scfMin = INT_MAX;
  scfMax = SCF_EMPTY_BAND;
  for (sfb = 0; sfb < sfbCnt; sfb++) {
    if (scf[sfb] != SCF_EMPTY_BAND) {
      scfMin = min(scfMin, scf[sfb]);
      scfMax = max(scfMax, scf[sfb]);
    }
  }

  stopSfb = 0;
  scfAct = SCF_EMPTY_BAND;
  if (scfMin == scfMax)
    return;
  do {
    scfPrev = scfAct;

    sfb = stopSfb;
    while (sfb < sfbCnt && (scf[sfb] == SCF_EMPTY_BAND)) {
      sfb++;
    }
    startSfb = sfb;
    scfAct = scf[startSfb];
    sfb++;
    while (sfb < sfbCnt && ((scf[sfb] == SCF_EMPTY_BAND) || (scf[sfb] == scf[startSfb]))) {
      sfb++;
    }
    stopSfb = sfb;

    if (stopSfb < sfbCnt) {
      scfNext = scf[stopSfb];
    } else {
      scfNext = scfAct;
    }

    if (scfPrev == SCF_EMPTY_BAND)
      scfPrev = scfAct;

    scfPrevNextMax = max(scfPrev, scfNext);
    scfPrevNextMin = min(scfPrev, scfNext);

    scfHi = max(scfPrevNextMax, scfAct);

    if (scfPrevNextMax >= scfAct) {
      scfLo = min(scfAct, scfPrevNextMin);
    } else {
      scfLo = scfPrevNextMax;
    }

    if (startSfb < sfbCnt && scfHi - scfLo <= MAX_SCF_DELTA) {
      if (scfHi > scf[startSfb]) {
        for (sfb = startSfb; sfb < stopSfb; sfb++) {
          if (scf[sfb] != SCF_EMPTY_BAND) {
            sfbDistMax[sfb] = (float)pow(psyOutChan->sfbThreshold[sfb] * sfbDist[sfb] * sfbDist[sfb],
                                         1.0f / 3.0f);
            sfbDistMax[sfb] = max(sfbDistMax[sfb],
                                  psyOutChan->sfbEnergy[sfb] * 1.e-3f);
            sfbDistMax[sfb] = min(sfbDistMax[sfb],
                                  psyOutChan->sfbThreshold[sfb]);
          }
        }

        bCheckScf = 1;
        for (scfNew = scf[startSfb] + 1; scfNew <= scfHi; scfNew++) {
          copyINT(scf, scfTmp, MAX_GROUPED_SFB);

          for (sfb = startSfb; sfb < stopSfb; sfb++) {
            if (scfTmp[sfb] != SCF_EMPTY_BAND) {
              scfTmp[sfb] = scfNew;
            }
          }

          deltaScfBits = iisaacfenc_countScfBitsDiff(scf,
                                                     scfTmp,
                                                     sfbCnt,
                                                     startSfb,
                                                     stopSfb);

          deltaSpecPe = iisaacfenc_calcSpecPeDiff(scf,
                                                  scfTmp,
                                                  sfbConstPePart,
                                                  sfbNRelevantLines,
                                                  startSfb,
                                                  stopSfb);

          deltaPeNew = deltaPe + deltaScfBits + deltaSpecPe;

          if (deltaPeNew < 0.0f) {
            bSuccess = 1;

            for (sfb = startSfb; sfb < stopSfb; sfb++) {
              if (scfTmp[sfb] != SCF_EMPTY_BAND) {
                iisaacfenc_calcSfbDist(psyOutChan->quantizerData,
                                       psyOutChan->mdctSpectrum + sfbOffs[sfb],
                                       expSpec + sfbOffs[sfb],
                                       useLloydMaxQuantizer,
                                       quantSpecTmp + sfbOffs[sfb],
                                       sfbOffs[sfb + 1] - sfbOffs[sfb],
                                       scfNew,
                                       &sfbDistNew[sfb]);

                if (sfbDistNew[sfb] > sfbDistMax[sfb]) {
                  bSuccess = 0;
                  if (sfbDistNew[sfb] >= psyOutChan->sfbEnergy[sfb]) {
                    bCheckScf = 0;
                  }
                  break;
                }
              }
            }
            if (bCheckScf == 0) {
              break;
            }

            if (bSuccess != 0) {
              deltaPe = deltaPeNew;
              for (sfb = startSfb; sfb < stopSfb; sfb++) {
                if (scf[sfb] != SCF_EMPTY_BAND) {
                  scf[sfb] = scfNew;
                  sfbDist[sfb] = sfbDistNew[sfb];
                  copyINT(quantSpecTmp + sfbOffs[sfb], quantSpec + sfbOffs[sfb],
                          sfbOffs[sfb + 1] - sfbOffs[sfb]);

                  for (j = sfbOffs[sfb]; j < sfbOffs[sfb + 1]; j++) {
                    if (psyOutChan->mdctSpectrum[j] < 0.0f) {
                      quantSpec[j] = -quantSpec[j];
                    }
                  }
                }
              }
            }
          }
        }
      }

      if (scfAct == scf[startSfb] &&
          scfLo < scfAct &&
          scfMax - scfMin <= MAX_SCF_DELTA) {
        int bminScfViolation = 0;
        copyINT(scf, scfTmp, MAX_GROUPED_SFB);
        scfNew = scfLo;

        for (sfb = startSfb; sfb < stopSfb; sfb++) {
          if (scfTmp[sfb] != SCF_EMPTY_BAND) {
            scfTmp[sfb] = scfNew;
            if (scfNew < minScf[sfb]) {
              bminScfViolation = 1;
            }
          }
        }

        if (bminScfViolation == 0) {
          deltaScfBits = iisaacfenc_countScfBitsDiff(scf,
                                                     scfTmp,
                                                     sfbCnt,
                                                     startSfb,
                                                     stopSfb);

          deltaSpecPe = iisaacfenc_calcSpecPeDiff(scf,
                                                  scfTmp,
                                                  sfbConstPePart,
                                                  sfbNRelevantLines,
                                                  startSfb,
                                                  stopSfb);

          deltaPeNew = deltaPe + deltaScfBits + deltaSpecPe;
        }

        if (bminScfViolation == 0 && deltaPeNew < 0.0f) {
          distOldSum = distNewSum = 0.0f;
          for (sfb = startSfb; sfb < stopSfb; sfb++) {
            if (scfTmp[sfb] != SCF_EMPTY_BAND) {
              distOldSum += sfbDist[sfb];

              iisaacfenc_calcSfbDist(psyOutChan->quantizerData,
                                     psyOutChan->mdctSpectrum + sfbOffs[sfb],
                                     expSpec + sfbOffs[sfb],
                                     useLloydMaxQuantizer,
                                     quantSpecTmp + sfbOffs[sfb],
                                     sfbOffs[sfb + 1] - sfbOffs[sfb],
                                     scfNew,
                                     &sfbDistNew[sfb]);

              if (sfbDistNew[sfb] > psyOutChan->sfbThreshold[sfb]) {
                distNewSum = 2.0f * distOldSum;
                break;
              }
              distNewSum += sfbDistNew[sfb];
            }
          }

          if (distNewSum < 0.8f * distOldSum) {
            deltaPe = deltaPeNew;
            for (sfb = startSfb; sfb < stopSfb; sfb++) {
              if (scf[sfb] != SCF_EMPTY_BAND) {
                scf[sfb] = scfNew;
                sfbDist[sfb] = sfbDistNew[sfb];
                copyINT(quantSpecTmp + sfbOffs[sfb], quantSpec + sfbOffs[sfb],
                        sfbOffs[sfb + 1] - sfbOffs[sfb]);

                for (j = sfbOffs[sfb]; j < sfbOffs[sfb + 1]; j++) {
                  if (psyOutChan->mdctSpectrum[j] < 0.0f) {
                    quantSpec[j] = -quantSpec[j];
                  }
                }
              }
            }
          }
        }
      }

      if ((scfMax - scfMin <= MAX_SCF_DELTA - 3) && (bNoiseFilling == 0)) {
        copyINT(scf, scfTmp, sfbCnt);
        for (i = 0; i < 3; i++) {
          scfNew = scfTmp[startSfb] - 1;

          for (sfb = startSfb; sfb < stopSfb; sfb++) {
            if (scfTmp[sfb] != SCF_EMPTY_BAND)
              scfTmp[sfb] = scfNew;
          }

          deltaScfBits = iisaacfenc_countScfBitsDiff(scf,
                                                     scfTmp,
                                                     sfbCnt,
                                                     startSfb,
                                                     stopSfb);
          deltaPeNew = deltaPe + deltaScfBits;

          if (deltaPeNew <= 0.0f) {
            bSuccess = 1;
            distOldSum = distNewSum = 0.0f;
            for (sfb = startSfb; sfb < stopSfb; sfb++) {
              if (scfTmp[sfb] != SCF_EMPTY_BAND) {
                float sfbEnQ = 0.0f;

                iisaacfenc_calcSfbQuantEnergyAndDist(psyOutChan->mdctSpectrum + sfbOffs[sfb],
                                                     quantSpec + sfbOffs[sfb],
                                                     sfbOffs[sfb + 1] - sfbOffs[sfb],
                                                     scfNew,
                                                     &sfbEnQ,
                                                     &sfbDistNew[sfb]);
                distOldSum += sfbDist[sfb];
                distNewSum += sfbDistNew[sfb];

                if (sfbDistNew[sfb] > 1.122f * sfbDist[sfb] ||
                    sfbEnQ < 0.7079f * psyOutChan->sfbEnergy[sfb]) {
                  bSuccess = 0;
                  break;
                }
              }
            }

            if (distNewSum < distOldSum && bSuccess != 0) {
              deltaPe = deltaPeNew;
              for (sfb = startSfb; sfb < stopSfb; sfb++) {
                if (scf[sfb] != SCF_EMPTY_BAND) {
                  scf[sfb] = scfNew;
                  sfbDist[sfb] = sfbDistNew[sfb];
                }
              }
            }
          }
        }
      }
    }
  } while (stopSfb <= sfbCnt);
}

static void iisaacfenc_calcScaleFactors(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    const float *sfbFormFactor,
    int *minSfMaxQuant) {
  int sfbOffs = 0;
  int sfb = 0;
  int i = 0;
  int j = 0;
  int scfInt = 0;
  float thresh = 0.0f;
  float energy = 0.0f;
  float energyPart = 0.0f;
  float thresholdPart = 0.0f;
  float scfFloat = 0.0f;
  float maxSpec = 0.0f;

  setINT(SCF_EMPTY_BAND, scf, psyOutChan->sfbCnt);
  setINT(SCF_EMPTY_BAND, minSfMaxQuant, MAX_GROUPED_SFB);

  for (sfbOffs = 0; sfbOffs < psyOutChan->sfbCnt; sfbOffs += psyOutChan->sfbPerGroup) {
    for (sfb = 0; sfb < psyOutChan->maxSfbPerGroup; sfb++) {
      i = sfbOffs + sfb;
      thresh = psyOutChan->sfbThreshold[i];
      energy = psyOutChan->sfbEnergy[i];

      if ((energy > 0.0f) && (energy > thresh)) {
        assert(sfbFormFactor[i] > 0);

        energyPart = (float)log10(sfbFormFactor[i]);

        thresholdPart = (float)log10(6.75 * thresh + FLT_MIN);

        scfFloat = 8.8585f * (thresholdPart - energyPart);

        scfInt = (int)floor(scfFloat);

        maxSpec = 0.0f;
        for (j = psyOutChan->sfbOffsets[i]; j < psyOutChan->sfbOffsets[i + 1]; j++) {
          maxSpec = max(maxSpec, (float)fabs(psyOutChan->mdctSpectrum[j]));
        }

        minSfMaxQuant[i] = (int)ceil(C1 + C2 * log(maxSpec));
        minSfMaxQuant[i] = max(minSfMaxQuant[i], minAllowedScf);
        scfInt = max(scfInt, minSfMaxQuant[i]);

        scfInt = min(scfInt, maxAllowedScf);
        scfInt = max(scfInt, minAllowedScf);

        scf[i] = scfInt;
      }
    }
  }
}

static void iisaacfenc_analysisBySynthesisScfRefinement(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    float *sfbDist,
    float *expSpec,
    const int useLloydMaxQuantizer,
    int *quantSpec,
    int *quantSpecTmp,
    int *minScfCalculated,
    const int *minSfMaxQuant) {
  int sfbOffs;

  for (sfbOffs = 0; sfbOffs < psyOutChan->sfbCnt; sfbOffs += psyOutChan->sfbPerGroup) {
    int sfb;

    for (sfb = 0; sfb < psyOutChan->maxSfbPerGroup; sfb++) {
      int i = sfbOffs + sfb;
      float thresh = psyOutChan->sfbThreshold[i];
      float energy = psyOutChan->sfbEnergy[i];

      sfbDist[i] = energy;

      iisaacfenc_calcExpSpec(psyOutChan->quantizerData,
                             expSpec + psyOutChan->sfbOffsets[i],
                             psyOutChan->mdctSpectrum + psyOutChan->sfbOffsets[i],
                             psyOutChan->sfbOffsets[i + 1] - psyOutChan->sfbOffsets[i]);

      if (energy > 0.0f && energy > thresh) {
        int scfInt = scf[i];

        scfInt = iisaacfenc_improveScf(psyOutChan->quantizerData,
                                       psyOutChan->mdctSpectrum + psyOutChan->sfbOffsets[i],
                                       expSpec + psyOutChan->sfbOffsets[i],
                                       useLloydMaxQuantizer,
                                       quantSpec + psyOutChan->sfbOffsets[i],
                                       quantSpecTmp + psyOutChan->sfbOffsets[i],
                                       psyOutChan->sfbOffsets[i + 1] - psyOutChan->sfbOffsets[i],
                                       thresh,
                                       scfInt,
                                       minSfMaxQuant[i],
                                       &sfbDist[i],
                                       &minScfCalculated[i]);
        scf[i] = scfInt;
      }
    }
  }
}

static void EstimateScaleFactorsChannel(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    const int invQuant,
    const int useLloydMaxQuantizer,
    int *quantSpec,
    float *expSpec,
    float *sfbDist,
    int *minScfCalculated,
    int *minScf,
    int *maxScf) {
  int i = 0;
  int bEnergySpectrumIsZero = 0;
  int quantSpecTmp[FRAME_LEN_LONG] = {0};
  int minSfMaxQuant[MAX_GROUPED_SFB] = {0};

  int maxSf = *maxScf;
  int minSf = *minScf;

  if (invQuant > 0) {
    setFLOAT(0.0f, expSpec, psyOutChan->granuleLength);
    setINT(0, quantSpec, psyOutChan->granuleLength);
  }

  iisaacfenc_calcScaleFactors(psyOutChan,
                              scf,
                              psyOutChan->sfbFormFactor,
                              minSfMaxQuant);

  if (invQuant > 0) {
    float sfbConstPePart[MAX_GROUPED_SFB] = {0.0f};
    float *sfbNRelevantLines = psyOutChan->sfbRelevLines;

    iisaacfenc_analysisBySynthesisScfRefinement(psyOutChan,
                                                scf,
                                                sfbDist,
                                                expSpec,
                                                useLloydMaxQuantizer,
                                                quantSpec,
                                                quantSpecTmp,
                                                minScfCalculated,
                                                minSfMaxQuant);

    iisaacfenc_getMinMaxScf(scf,
                            psyOutChan->sfbCnt,
                            &minSf,
                            &maxSf);

    limitScalefactor(scf,
                     &minSf,
                     &maxSf,
                     psyOutChan->sfbCnt,
                     LIMIT_MAX_SCF);

    for (i = 0; i < psyOutChan->sfbCnt; i++) {
      if (scf[i] != SCF_EMPTY_BAND) {
        sfbDist[i] = iisaacfenc_reCalcSfbDistAndSignQuantSpec(psyOutChan->quantizerData,
                                                              psyOutChan->mdctSpectrum + psyOutChan->sfbOffsets[i],
                                                              expSpec + psyOutChan->sfbOffsets[i],
                                                              useLloydMaxQuantizer,
                                                              quantSpec + psyOutChan->sfbOffsets[i],
                                                              psyOutChan->sfbOffsets[i + 1] - psyOutChan->sfbOffsets[i],
                                                              scf[i]);
      }
    }

    for (i = 0; i < psyOutChan->sfbCnt; i++) {
      sfbConstPePart[i] = 0.f;
      if (scf[i] != SCF_EMPTY_BAND) {
        sfbConstPePart[i] = (float)log((double)(psyOutChan->sfbEnergy[i] * 6.75f / psyOutChan->sfbFormFactor[i])) * LOG2_1;
      }
    }

    iisaacfenc_assimilateSingleScf(psyOutChan,
                                   expSpec,
                                   useLloydMaxQuantizer,
                                   quantSpec,
                                   quantSpecTmp,
                                   scf,
                                   minSfMaxQuant,
                                   sfbDist,
                                   sfbConstPePart,
                                   sfbNRelevantLines,
                                   minScfCalculated,
                                   1);

    if (invQuant > 1) {
      iisaacfenc_assimilateMultipleScf(psyOutChan,
                                       expSpec,
                                       useLloydMaxQuantizer,
                                       quantSpec,
                                       quantSpecTmp,
                                       scf,
                                       minSfMaxQuant,
                                       sfbDist,
                                       sfbConstPePart,
                                       sfbNRelevantLines);

      iisaacfenc_assimilateMultipleScf2(psyOutChan,
                                        expSpec,
                                        useLloydMaxQuantizer,
                                        quantSpec,
                                        quantSpecTmp,
                                        scf,
                                        minSfMaxQuant,
                                        sfbDist,
                                        sfbConstPePart,
                                        sfbNRelevantLines,
                                        0);
    }
  }

  iisaacfenc_getMinMaxScf(scf,
                          psyOutChan->sfbCnt,
                          &minSf,
                          &maxSf);

  if (maxSf > SCF_EMPTY_BAND) {
    bEnergySpectrumIsZero = 0;
  } else {
    bEnergySpectrumIsZero = 1;
  }

  if ((bEnergySpectrumIsZero == 0) && (maxSf - minSf > MAX_SCF_DELTA)) {
    limitScalefactor(scf,
                     &minSf,
                     &maxSf,
                     psyOutChan->sfbCnt,
                     LIMIT_MIN_SCF);

    for (i = 0; i < psyOutChan->sfbCnt; i++) {
      if (invQuant > 0) {
        sfbDist[i] = iisaacfenc_reCalcSfbDistAndSignQuantSpec(psyOutChan->quantizerData,
                                                              psyOutChan->mdctSpectrum + psyOutChan->sfbOffsets[i],
                                                              expSpec + psyOutChan->sfbOffsets[i],
                                                              useLloydMaxQuantizer,
                                                              quantSpec + psyOutChan->sfbOffsets[i],
                                                              psyOutChan->sfbOffsets[i + 1] - psyOutChan->sfbOffsets[i],
                                                              scf[i]);
      }
    }
  }

  *maxScf = maxSf;
  *minScf = minSf;
}

static int iisaacfenc_CalcNoiseLevel(const float lineError) {
  int noiseLevel = (int)(14.0f + 4.0f * log(fmax(lineError, FLT_MIN)) / log(2.0f));

  noiseLevel = max(0, noiseLevel);

  noiseLevel = min(noiseLevel, 7);

  return noiseLevel;
}

static int GetNoiseFillingStartOffset(
    PSY_OUT_CHANNEL *psyOutChan) {
  int binStart = 0;

  if (psyOutChan->windowSequence == SHORT_WINDOW) {
    binStart = 20;
  } else {
    binStart = 160;
  }

  if (psyOutChan->sfbOffsets[psyOutChan->sfbCnt] == 768) {
    binStart = (binStart * 3) >> 2;
  }
  return binStart;
}

static int iisaacfenc_GetNoiseFillingStartSfb(
    PSY_OUT_CHANNEL *psyOutChan,
    const int nfStartBin) {
  int sfbStart = 0;

  if (psyOutChan->windowSequence == SHORT_WINDOW) {
    if (psyOutChan->sfbOffsets[psyOutChan->sfbCnt] == 768) {
      sfbStart = 4;
    } else {
      sfbStart = 5;
    }
  } else {
    while (psyOutChan->sfbOffsets[sfbStart] < nfStartBin) {
      sfbStart++;
    }
  }

  if (sfbStart > psyOutChan->maxSfbPerGroup) {
    sfbStart = psyOutChan->maxSfbPerGroup;
  }
  return sfbStart;
}

static void iisaacfenc_KeepPreviousScalefactor(
    const int *sfbOffsets,
    const int sfbOffs,
    const int sfbStart,
    int *scf,
    int *quantSpec) {
  int sfb = 0;
  int i = 0;

  for (sfb = sfbOffs; sfb < sfbOffs + sfbStart; sfb++) {
    const int lineOffs = sfbOffsets[sfb];

    if (scf[sfb] > SCF_EMPTY_BAND) {
      for (i = lineOffs; i < sfbOffsets[sfb + 1]; i++) {
        if (quantSpec[i] != 0) {
          break;
        }
      }

      if (i == sfbOffsets[sfb + 1]) {
        scf[sfb] = SCF_EMPTY_BAND;
      }
    }

    if (scf[sfb] == SCF_EMPTY_BAND) {
      if (sfb > sfbOffs) {
        scf[sfb] = scf[sfb - 1];
      }

      for (i = lineOffs; i < sfbOffsets[sfb + 1]; i++) {
        quantSpec[i] = 0;
      }
    }
  }
}

static void iisaacfenc_UpdateNrgRatio(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    const int sfbStart,
    float *minNrgDiff) {
  int sfbOffs = 0;
  int sfbFirst = 0;
  int sfb = 0;

  for (sfbOffs = 0; sfbOffs < psyOutChan->sfbCnt; sfbOffs += psyOutChan->sfbPerGroup) {
    sfbFirst = sfbOffs;

    while ((sfbFirst < sfbOffs + psyOutChan->maxSfbPerGroup - 1) && (scf[sfbFirst] == SCF_EMPTY_BAND)) {
      sfbFirst++;
    }

    if ((sfbFirst > sfbOffs) && (scf[sfbFirst] > SCF_EMPTY_BAND)) {
      for (sfb = sfbOffs + sfbStart; sfb < sfbFirst; sfb++) {
        const int lineOffs = psyOutChan->sfbOffsets[sfb];
        const int sfbWidth = psyOutChan->sfbOffsets[sfb + 1] - lineOffs;

        if (psyOutChan->sfbEnergy[sfb] > FLT_MIN) {
          const float nrgScf = (float)(log(psyOutChan->sfbEnergy[sfb] / (float)sfbWidth) / log(2.0f)) * 2.0f;
          if (*minNrgDiff > nrgScf - (float)scf[sfbFirst]) {
            *minNrgDiff = nrgScf - (float)scf[sfbFirst];
          }
        } else {
          *minNrgDiff = -99.9f;
        }
      }
    }
  }
}

static void iisaacfenc_KeepFirstFoundScfForEmptySfbs(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf) {
  int sfbOffs = 0;
  int sfbStart = 0;

  for (sfbOffs = 0; sfbOffs < psyOutChan->sfbCnt; sfbOffs += psyOutChan->sfbPerGroup) {
    sfbStart = sfbOffs;

    while ((sfbStart < sfbOffs + psyOutChan->maxSfbPerGroup - 1) && (scf[sfbStart] == SCF_EMPTY_BAND)) {
      sfbStart++;
    }

    if (sfbStart > sfbOffs) {
      setINT(scf[sfbStart], scf + sfbOffs, sfbStart - sfbOffs);
    }
  }
}

static void iisaacfenc_CalcNoiseOffset(
    const float minNrgDiff,
    const int noiseFillingStartSfb,
    const int splitTransform,
    const int windowSequence,
    const int maxSfbPerGroup,
    const int *minScfCalculated,
    const unsigned int noiseLevel,
    int *scf,
    int *noiseOffset) {
  int sfb = 0;

  if (noiseLevel > 0) {
    if (minNrgDiff < FLT_MAX) {
      (*noiseOffset) += (int)(minNrgDiff + 1.3333f * (float)(26 - noiseLevel));

      if (*noiseOffset <= 0) {
        *noiseOffset = 0;
      } else if ((splitTransform) && (windowSequence == START_WINDOW || windowSequence == STOPSTART_WINDOW)) {
        const int remNLOfset = *noiseOffset - min(((*noiseOffset) + 4) & 56, 31);
        assert(windowSequence == START_WINDOW || windowSequence == STOPSTART_WINDOW);

        *noiseOffset = ((*noiseOffset) + 4) & 56;

        for (sfb = noiseFillingStartSfb; sfb < maxSfbPerGroup; sfb++) {
          if ((scf[sfb] != SCF_EMPTY_BAND) && (minScfCalculated[sfb] == 0)) {
            scf[sfb] += remNLOfset;
          }
        }
      }
    } else {
      *noiseOffset = 0;
    }
  }
}

static void iisaacfenc_WriteNoiseLevelAndNoiseOffsetToBitstream(
    const unsigned int noiseLevel,
    const int noiseOffset,
    unsigned int *bitstreamPayload) {
  *bitstreamPayload = 0;

  if (noiseLevel > 0) {
    *bitstreamPayload = (noiseLevel << 5);
  }

  *bitstreamPayload += min((unsigned int)noiseOffset, 31);
}

static void EstimateNoiseFillingChannel(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    int *quantSpec,
    float *expSpec,
    int maxSf,
    int *minScfCalculated,
    unsigned int *noiseLevel) {
  int sfb = 0;
  int sfbOffs = 0;
  int bEnergySpectrumIsZero = 0;
  float energyPart = 0.0f;
  int noMaxErr = 0;
  int binStart = 0;
  int sfbStart = 0;
  int zeroLines = 0;
  float lineError = FLT_MIN;
  float minNrgDiff = FLT_MAX;
  int noiseOffset = 0;
  if (maxSf > SCF_EMPTY_BAND) {
    bEnergySpectrumIsZero = 0;
  } else {
    bEnergySpectrumIsZero = 1;
  }

  binStart = GetNoiseFillingStartOffset(psyOutChan);
  sfbStart = iisaacfenc_GetNoiseFillingStartSfb(psyOutChan, binStart);
  setINT(SCF_EMPTY_BAND, minScfCalculated, MAX_GROUPED_SFB);

  switch (psyOutChan->noiseFillingMode) {
    case NF_MODE_2:
      noMaxErr = 1;
      break;

    default:
      assert(0);
      break;
  }

  if (bEnergySpectrumIsZero == 0) {
    for (sfbOffs = 0; sfbOffs < psyOutChan->sfbCnt; sfbOffs += psyOutChan->sfbPerGroup) {
      iisaacfenc_KeepPreviousScalefactor(psyOutChan->sfbOffsets,
                                         sfbOffs,
                                         sfbStart,
                                         scf,
                                         quantSpec);

      for (sfb = sfbOffs + sfbStart; sfb < sfbOffs + psyOutChan->maxSfbPerGroup; sfb++) {
        const int lineOffs = psyOutChan->sfbOffsets[sfb];
        const int sfbWidth = psyOutChan->sfbOffsets[sfb + 1] - lineOffs;
        int lineZerosMDCT = 0;

        if (scf[sfb] > SCF_EMPTY_BAND) {
          energyPart = iisaacfenc_calcSfbZeroError(expSpec + lineOffs,
                                                   quantSpec + lineOffs,
                                                   sfbWidth,
                                                   scf[sfb],
                                                   noMaxErr,
                                                   &lineZerosMDCT);

          if (lineZerosMDCT == sfbWidth) {
            scf[sfb] = SCF_EMPTY_BAND;
          }
        }

        switch (psyOutChan->noiseFillingMode) {
          case NF_MODE_2:

            if (scf[sfb] > SCF_EMPTY_BAND) {
              {
                lineError += energyPart;
                zeroLines += max(0, lineZerosMDCT - noMaxErr);
              }
            }

            if (scf[sfb] == SCF_EMPTY_BAND) {
              scf[sfb] = scf[sfb - 1];

              minScfCalculated[sfb] = 0;

              if (psyOutChan->sfbEnergy[sfb] > FLT_MIN) {
                const float nrgScf = (float)(log(psyOutChan->sfbEnergy[sfb] / (float)sfbWidth) / log(2.0f)) * 2.0f;

                if ((scf[sfb] > SCF_EMPTY_BAND) && (minNrgDiff > nrgScf - (float)scf[sfb])) {
                  minNrgDiff = nrgScf - (float)scf[sfb];
                }
              } else {
                minNrgDiff = -99.9f;
              }
            }
            break;

          default:
            assert(0);
            break;
        }

        if (scf[sfb] == SCF_EMPTY_BAND) {
          int i = 0;

          for (i = lineOffs; i < psyOutChan->sfbOffsets[sfb + 1]; i++) {
            quantSpec[i] = 0;
          }
        }
      }
    }

    switch (psyOutChan->noiseFillingMode) {
      case NF_MODE_2:
        if (zeroLines > 0) {
          *noiseLevel = (unsigned int)iisaacfenc_CalcNoiseLevel(lineError / (float)zeroLines);
        }

        if (psyOutChan->sfbCnt == 2 * psyOutChan->sfbPerGroup) {
          *noiseLevel = min(*noiseLevel, 1);
        }

        if ((*noiseLevel == 0) &&
            (psyOutChan->chaosMeasure > 0.75f)) {
          *noiseLevel = 1;
        }
        iisaacfenc_UpdateNrgRatio(psyOutChan,
                                  scf,
                                  sfbStart,
                                  &minNrgDiff);
        break;

      default:
        assert(0);
        break;
    }

    switch (psyOutChan->noiseFillingMode) {
      case NF_MODE_2:
        iisaacfenc_CalcNoiseOffset(minNrgDiff,
                                   sfbStart,
                                   0,
                                   psyOutChan->windowSequence,
                                   psyOutChan->maxSfbPerGroup,
                                   minScfCalculated,
                                   *noiseLevel,
                                   scf,
                                   &noiseOffset);

        iisaacfenc_WriteNoiseLevelAndNoiseOffsetToBitstream(*noiseLevel,
                                                            noiseOffset,
                                                            noiseLevel);

        iisaacfenc_KeepFirstFoundScfForEmptySfbs(psyOutChan,
                                                 scf);
        break;

      default:
        assert(0);
        break;
    }
  }
}

static void GlobalGainCalcAndSpectrumRequant(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    int *quantSpec,
    int *globalGain) {
  int i = 0;
  int j = 0;
  int maxSf = 0;
  int maxScfDelta = MAX_SCF_DELTA;
  int bEnergySpectrumIsZero = 0;

  iisaacfenc_getMinMaxScf(scf,
                          psyOutChan->sfbCnt,
                          NULL,
                          &maxSf);

  if (maxSf > SCF_EMPTY_BAND) {
    bEnergySpectrumIsZero = 0;
  } else {
    bEnergySpectrumIsZero = 1;
  }

  if (bEnergySpectrumIsZero == 0) {
    *globalGain = maxSf;
    maxScfDelta = max(0, min(MAX_SCF_DELTA, maxSf - minAllowedScf));

    for (i = 0; i < psyOutChan->sfbCnt; i++) {
      if (scf[i] == SCF_EMPTY_BAND) {
        scf[i] = maxScfDelta;

        for (j = psyOutChan->sfbOffsets[i]; j < psyOutChan->sfbOffsets[i + 1]; j++) {
          psyOutChan->mdctSpectrum[j] = 0.0f;
          quantSpec[j] = 0;
        }
      } else {
        scf[i] = maxSf - scf[i];
      }
    }
  } else {
    *globalGain = -99;

    for (i = 0; i < psyOutChan->sfbCnt; i++) {
      scf[i] = 0;

      for (j = psyOutChan->sfbOffsets[i]; j < psyOutChan->sfbOffsets[i + 1]; j++) {
        psyOutChan->mdctSpectrum[j] = 0.0f;
        quantSpec[j] = 0;
      }
    }
  }
}

static void EstimateNoiseFillingAndScaleFactorsChannel(
    PSY_OUT_CHANNEL *psyOutChan,
    int *scf,
    int *globalGain,
    unsigned int *noiseLevel,
    int bNoiseFilling,
    const int invQuant,
    const int useLloydMaxQuantizer,
    int *quantSpec) {
  int maxSf = 0;
  int minSf = 0;

  ALIGN_16_BYTE float expSpec[FRAME_LEN_LONG] = {0.0f};
  float sfbDist[MAX_GROUPED_SFB] = {0.0f};
  int minScfCalculated[MAX_GROUPED_SFB] = {0};

  *noiseLevel = 0;

  EstimateScaleFactorsChannel(psyOutChan,
                              scf,
                              invQuant,
                              useLloydMaxQuantizer,
                              quantSpec,
                              expSpec,
                              sfbDist,
                              minScfCalculated,
                              &minSf,
                              &maxSf);

  if (bNoiseFilling != 0) {
    EstimateNoiseFillingChannel(psyOutChan,
                                scf,
                                quantSpec,
                                expSpec,
                                maxSf,
                                minScfCalculated,
                                noiseLevel);
  }

  GlobalGainCalcAndSpectrumRequant(psyOutChan,
                                   scf,
                                   quantSpec,
                                   globalGain);
}

int iisaacfenc_CalcSingleFormFactor(SFESTIM_DATA const *const sfestimData,
                                    float *const sfbFormFactor,
                                    float const *const mdctSpectrum,
                                    int const sfbCnt,
                                    int const *const sfbOffsets) {
  int returnValue = 0;

  if ((NULL == sfestimData) || (NULL == sfbFormFactor) || (NULL == mdctSpectrum) || (NULL == sfbOffsets)) {
    returnValue = -1;
  }

  if (0 == returnValue) {
    sfestimData->CalcFormFactorChannel_Ptr(sfbFormFactor,
                                           mdctSpectrum,
                                           sfbCnt,
                                           sfbOffsets);
  }

  return returnValue;
}

void iisaacfenc_EstimateNoiseFillingAndScaleFactors(
    PSY_OUT_CHANNEL *psyOutChannel[],
    QC_OUT_CHANNEL *qcOutChannel[],
    const int nChannels,
    INTERN_CORE_MODE *coreMode,
    const int invQuant,
    const int useLloydMaxQuantizer,
    const int bNoiseFilling) {
  int j = 0;

  for (j = 0; j < nChannels; j++) {
    if (coreMode[j] == INTERN_CORE_MODE_FD) {
      EstimateNoiseFillingAndScaleFactorsChannel(psyOutChannel[j],
                                                 qcOutChannel[j]->scf,
                                                 &(qcOutChannel[j]->globalGain),
                                                 &(qcOutChannel[j]->noiseLevel),
                                                 bNoiseFilling,
                                                 invQuant,
                                                 useLloydMaxQuantizer,
                                                 qcOutChannel[j]->quantSpec);
    }
  }
}

void iisaacfenc_sfEstimInit(SFESTIM_DATA **phSfEstimData,
                            SSE_OPTI int const useCpuOptimization) {
  if (NULL != phSfEstimData) {
    *phSfEstimData = (SFESTIM_DATA *)iisCalloc(1, sizeof(SFESTIM_DATA));

    if (*phSfEstimData) {
      (*phSfEstimData)->CalcFormFactorChannel_Ptr = &iisaacfenc_CalcFormFactorChannel_NoOpt;
    }
  }
}

void iisaacfenc_sfEstimDelete(SFESTIM_DATA **phSfEstimData) {
  if (NULL != phSfEstimData) {
    if (NULL != *phSfEstimData) {
      iisFree(*phSfEstimData);
      *phSfEstimData = NULL;
    }
  }
}
