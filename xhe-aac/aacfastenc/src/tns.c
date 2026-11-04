
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
#include <assert.h>
#include <string.h>

#include "aacenc_internal.h"
#include "mathlib.h"
#include "tns.h"
#include "iisutillib.h"
#include "tns_func.h"
#include "psy_configuration.h"
#include "glob_con.h"
#include "psy_data.h"

#ifndef PI
#define PI 3.1415926535897931f
#endif

#define FILTER_DIRECTION 0
#define MIN_NRG 1.0f / 65536.0f

#define QFAC(res) (((1 << (res - 1)) - 0.5f) / (PI / 2.0f))
#define QFAC_M(res) (((1 << (res - 1)) + 0.5f) / (PI / 2.0f))
#define IQFAC(res) (1.0f / (float)QFAC(res))
#define IQFAC_M(res) (1.0f / (float)QFAC_M(res))
#define MAX_TNS_FILTER_GAIN_FOR_CMPLX_PRED (18.0f)

static const float qTable[] = {(float)QFAC(2), (float)QFAC(3), (float)QFAC(4),
                               (float)QFAC(5)};
static const float qTableM[] = {(float)QFAC_M(2), (float)QFAC_M(3), (float)QFAC_M(4),
                                (float)QFAC_M(5)};

#undef QFAC
#undef QFAC_M
#undef IQFAC
#undef IQFAC_M

#define NUM_ANALYSIS_SECTIONS (5)

typedef struct {
  float parcor[HEAAC_TNS_MAX_ORDER];
  int quantParcor[HEAAC_TNS_MAX_ORDER];
  int coefRes;
  float predictionGain;
  int startBand;
  int stopBand;
  int direction;
  int order;
  int bFilterShortened;
} TNS_FILTER_DESIGN;

static const float tnsCoeff3[8] =
    {
        -0.98480775F,
        -0.86602540F,
        -0.64278761F,
        -0.34202014F,
        0.00000000F,
        0.43388374F,
        0.78183148F,
        0.97492791F};

static const float tnsCoeff4[16] =
    {
        -0.99573418F,
        -0.96182564F,
        -0.89516329F,
        -0.79801723F,
        -0.67369564F,
        -0.52643216F,
        -0.36124167F,
        -0.18374952F,
        0.00000000F,
        0.20791169F,
        0.40673664F,
        0.58778525F,
        0.74314483F,
        0.86602540F,
        0.95105652F,
        0.99452190F};

static void calcTnsGainDecoderHeadroomRatio(
    float const *const spectrum,
    TNS_INFO *tnsInfo,
    int const *const sfbOffset,
    int const tnsStopLine,
    int const subBlockNumber,
    int const sfbCnt,
    float const predictionGain,
    float *const tnsGainHeadroomRatio);

static float iisaacfenc_AutoToParcor(
    float const *const autoCorr,
    float *const reflCoeff,
    int const numOfCoeff);

static void iisaacfenc_Parcor2Index(const float parcor[], int index[], int order, int bitsPerCoeff);

static void iisaacfenc_Index2Parcor(const int index[], float parcor[], int order, int bitsPerCoeff);

static void iisaacfenc_AnalysisFilterLattice(const float signal[], const int numOfLines,
                                             const float parCoeff[], const int order,
                                             float output[]);

static void iisaacfenc_SynthesisFilter(const float signal[], int numOfLines,
                                       const float predictorCoeff[], int order,
                                       float output[], int direction);

static void iisaacfenc_ParcorToLpc(const float reflCoeff[], float LpcCoeff[],
                                   int numOfCoeff);

static float iisaacfenc_maxAbsDiffFLOAT(const float *f1, const float *f2, const int size);

int iisaacfenc_InitTnsConfiguration(AACENC_CODEC_TYPE codecType,
                                    int bitRate,
                                    int sampleRate,
                                    int channels,
                                    int blockType,
                                    TNS_CONFIG *tC,
                                    PSY_CONFIGURATION *pC,
                                    int active) {
  int i;
  float acfTimeRes = 4.5f;

  if (channels <= 0)
    return 1;

  tC->tnsActive = (active) ? 1 : 0;
  tC->coefRes = (blockType == SHORT_WINDOW) ? 3 : 4;
  tC->codecType = codecType;

  switch (codecType) {
    case AACENC_CODEC_AAC:
      tC->maxOrder = (blockType == SHORT_WINDOW) ? 5 : 12;
      break;
    case AACENC_CODEC_XHEAAC:
      tC->maxOrder = (blockType == SHORT_WINDOW) ? 7 : 14;
      break;
    default:
      break;
  }

  if (bitRate < 16000)
    tC->maxOrder -= 2;

  tC->lpcStartBand = (blockType == SHORT_WINDOW) ? 0 : ((sampleRate < 18783) ? 4 : 8);
  tC->lpcStartLine = pC->sfbOffset[tC->lpcStartBand];

  switch (codecType) {
    case AACENC_CODEC_AAC:
      if (sampleRate >= 75132)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 9 : 31;
      else if (sampleRate >= 55426)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 10 : 34;
      else if (sampleRate >= 46009)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 40;
      else if (sampleRate >= 37566)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 42;
      else if (sampleRate >= 27713)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 51;
      else if (sampleRate >= 18783)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 46;
      else if (sampleRate >= 9391)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 42;
      else
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 39;
      break;
    case AACENC_CODEC_XHEAAC:
      if (sampleRate >= 75132)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 9 : 31;
      else if (sampleRate >= 55426)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 10 : 34;
      else if (sampleRate >= 46009)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 40;
      else if (sampleRate >= 37566)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 42;
      else if (sampleRate >= 27713)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 14 : 51;
      else if (sampleRate >= 18783)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 15 : 47;
      else if (sampleRate >= 9391)
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 15 : 43;
      else
        tC->lpcStopBand = (blockType == SHORT_WINDOW) ? 15 : 40;
      break;
    default:
      break;
  }
  if (tC->lpcStopBand > pC->sfbActive)
    tC->lpcStopBand = pC->sfbActive;
  tC->lpcStopLine = pC->sfbOffset[tC->lpcStopBand];

  tC->tnsStopBand = tC->lpcStopBand;
  if (codecType == AACENC_CODEC_MPEGH)
    tC->tnsStopLine = pC->sfbOffset[tC->tnsStopBand];
  else
    tC->tnsStopLine = tC->lpcStopLine;

  i = tC->tnsStopBand;
  while (pC->sfbOffset[i] > (tC->lpcStartLine + (tC->lpcStopLine - tC->lpcStartLine) / 4)) i--;
  tC->tnsStartBand = i;
  tC->tnsStartLine = pC->sfbOffset[i];

  switch (codecType) {
    case AACENC_CODEC_AAC:
      acfTimeRes *= (float)(tC->maxOrder * tC->maxOrder);
      break;
    case AACENC_CODEC_XHEAAC:
      acfTimeRes *= (float)((tC->maxOrder - 2) * (tC->maxOrder - 2));
      break;
    default:
      break;
  }
  tC->acfWindow[0] = 1.0f;
  for (i = 1; i <= HEAAC_TNS_MAX_ORDER; i++) {
    tC->acfWindow[i] = tC->acfWindow[i - 1] * (acfTimeRes - i) / (acfTimeRes + i);
  }
  return 0;
}

static void calcTnsGainDecoderHeadroomRatio(
    float const *const spectrum,
    TNS_INFO *tnsInfo,
    int const *const sfbOffset,
    int const tnsStopLine,
    int const subBlockNumber,
    int const sfbCnt,
    float const predictionGain,
    float *const tnsGainHeadroomRatio) {
  int line;
  float maxSpecBelowTnsRange = 0.f;
  float maxSpecTnsRange = 0.f;
  float tnsHeadroom = 1.0f;
  int cnt;
  int len = 0;
  int startLine;

  for (cnt = 0; cnt < tnsInfo->numOfFilters[subBlockNumber]; cnt++) {
    len += tnsInfo->length[subBlockNumber][cnt];
  }

  startLine = sfbOffset[sfbCnt - len];

  for (line = 0; line < startLine; line++) {
    maxSpecBelowTnsRange = max(maxSpecBelowTnsRange, fabsf(spectrum[line]));
  }

  for (line = startLine; line < tnsStopLine; line++) {
    maxSpecTnsRange = max(maxSpecTnsRange, fabsf(spectrum[line]));
  }

  if (maxSpecTnsRange < maxSpecBelowTnsRange) {
    if (maxSpecTnsRange > 0.0f) {
      tnsHeadroom = maxSpecBelowTnsRange / maxSpecTnsRange;
    }
  }

  *tnsGainHeadroomRatio = predictionGain / (tnsHeadroom * tnsHeadroom);
}

static void calcAcf(
    float *const rxx,
    int const rxxLength,
    float const *const x,
    int const xlen) {
  int i;
  for (i = 0; i < rxxLength; i++) {
    rxx[i] = dotFLOAT(x, &x[i], xlen - i);
  }
}

static void normalizeAcf(
    float *const rxx,
    int const rxxLength,
    float const factor) {
  int i;
  rxx[0] = factor / max(MIN_NRG, rxx[0]);
  for (i = 1; i < rxxLength; i++) {
    rxx[i] *= rxx[0];
  }
}

static void mergeNormalizedAcfs(
    float *const rxx1,
    float const *const rxx2,
    int const rxxLength) {
  int i;
  for (i = 1; i < rxxLength; i++) {
    rxx1[i] += rxx2[i];
  }
}

static void designTnsFilter(
    float const *const rxx,
    int const rxxLength,
    float const *const acfWindow,
    int const maxOrder,
    int const coefRes,
    int const startBand,
    int const stopBand,
    float const threshFac,
    TNS_FILTER_DESIGN *const tnsFilterDesign) {
  float predictionGain;
  float rxxWindowed[HEAAC_TNS_MAX_ORDER + 1] = {0.0f};
  float sumSqrCoef = 0.0f;
  int i;

  assert(rxx != NULL);
  assert(rxxLength >= 0);
  assert(acfWindow != NULL);
  assert(maxOrder >= 0 && maxOrder < HEAAC_TNS_MAX_ORDER);
  assert(coefRes >= 0);
  assert(startBand >= 0);
  assert(startBand >= 0);
  assert(startBand - stopBand >= 0);
  assert(threshFac > 0);
  assert(tnsFilterDesign != NULL);

  memset(tnsFilterDesign, 0, sizeof(*tnsFilterDesign));

  assert(acfWindow[0] == 1.0f);
  multFLOAT(rxx, acfWindow, rxxWindowed, maxOrder + 1);

  predictionGain = iisaacfenc_AutoToParcor(
      rxxWindowed,
      tnsFilterDesign->parcor,
      maxOrder);

  tnsFilterDesign->predictionGain = predictionGain;

  iisaacfenc_Parcor2Index(
      tnsFilterDesign->parcor,
      tnsFilterDesign->quantParcor,
      maxOrder,
      coefRes);

  tnsFilterDesign->direction = FILTER_DIRECTION;
  tnsFilterDesign->startBand = startBand;
  tnsFilterDesign->stopBand = stopBand;

  for (i = maxOrder - 1; i >= 0; i--) {
    if (tnsFilterDesign->quantParcor[i] != 0)
      break;
  }
  tnsFilterDesign->order = i + 1;

  for (; i >= 0; i--) {
    sumSqrCoef += tnsFilterDesign->quantParcor[i] * tnsFilterDesign->quantParcor[i];
  }

  if ((predictionGain < (float)threshFac * 1.3125f) && (sumSqrCoef <= threshFac * ((maxOrder + 5) >> 1))) {
    tnsFilterDesign->order = 0;
  }
  return;
}

static int isFilterMerged(
    TNS_FILTER_DESIGN const *const tnsFilterDesign,
    int const windowIdx,
    int const filterIdx,
    TNS_INFO const *const tnsInfo) {
  int mergeFilter = 1;

  assert(windowIdx >= 0 && windowIdx < TRANS_FAC);
  assert(filterIdx >= 0 && filterIdx < MAX_NUM_OF_FILTERS);
  assert(tnsInfo->numOfFilters[windowIdx] == filterIdx);
  assert(tnsInfo->numOfFilters[windowIdx] < MAX_NUM_OF_FILTERS);

  if (mergeFilter) {
    mergeFilter &= (filterIdx > 0);
  }
  if (mergeFilter) {
    mergeFilter &= (tnsFilterDesign->order > 0) == (tnsInfo->order[windowIdx][filterIdx - 1] > 0);
  }
  if (mergeFilter) {
    int i;
    float coefDiff = 0.f;
    for (i = tnsInfo->order[windowIdx][filterIdx - 1] - 1; i >= 0; i--) {
      coefDiff += abs(tnsFilterDesign->quantParcor[i] - tnsInfo->coef[windowIdx][filterIdx - 1][i]);
    }
    if (coefDiff > 1.0f) {
      mergeFilter &= 0;
    }
  }
  if (mergeFilter) {
    mergeFilter &= ((!tnsFilterDesign->bFilterShortened) && (!tnsInfo->bTnsFilterShortened[windowIdx][filterIdx - 1])) || (tnsFilterDesign->order == 0);
  }

  return mergeFilter;
}

static int addOrMergeTnsFilter(
    TNS_FILTER_DESIGN const *const tnsFilterDesign,
    int const windowIdx,
    int const filterIdx,
    TNS_INFO *const tnsInfo) {
  int mergeFilter;
  int filterLen;

  assert(windowIdx >= 0 && windowIdx < TRANS_FAC);
  assert(filterIdx >= 0 && filterIdx < MAX_NUM_OF_FILTERS);
  assert(tnsInfo->numOfFilters[windowIdx] == filterIdx);

  mergeFilter = isFilterMerged(
      tnsFilterDesign,
      windowIdx,
      filterIdx,
      tnsInfo);

  filterLen = tnsFilterDesign->startBand - tnsFilterDesign->stopBand;
  assert(filterLen >= 0);
  if (mergeFilter) {
    tnsInfo->length[windowIdx][filterIdx] = 0;
    tnsInfo->order[windowIdx][filterIdx] = 0;
    tnsInfo->length[windowIdx][filterIdx - 1] += filterLen;
    setINT(0, tnsInfo->coef[windowIdx][filterIdx], HEAAC_TNS_MAX_ORDER);

    tnsInfo->predictionGainPerFilter[windowIdx][filterIdx] = tnsFilterDesign->predictionGain;
    copyINT(tnsFilterDesign->quantParcor, tnsInfo->coef[windowIdx][filterIdx], sizeof(tnsInfo->coef[windowIdx][filterIdx]) / sizeof(tnsInfo->coef[windowIdx][filterIdx][0]));

    return filterIdx;
  } else {
    assert(tnsInfo->numOfFilters[windowIdx] < TRUE_MAX_NUM_FILTERS);
    tnsInfo->length[windowIdx][filterIdx] = filterLen;
    tnsInfo->order[windowIdx][filterIdx] = tnsFilterDesign->order;
    tnsInfo->direction[windowIdx][filterIdx] = tnsFilterDesign->direction;
    tnsInfo->predictionGainPerFilter[windowIdx][filterIdx] = tnsFilterDesign->predictionGain;
    tnsInfo->bTnsFilterShortened[windowIdx][filterIdx] = tnsFilterDesign->bFilterShortened;
    assert(sizeof(tnsInfo->coef[windowIdx][filterIdx]) == sizeof(tnsFilterDesign->quantParcor));
    copyINT(tnsFilterDesign->quantParcor, tnsInfo->coef[windowIdx][filterIdx], sizeof(tnsInfo->coef[windowIdx][filterIdx]) / sizeof(tnsInfo->coef[windowIdx][filterIdx][0]));

    tnsInfo->numOfFilters[windowIdx]++;
    return filterIdx + 1;
  }
}

static int mergeTwoUpperFilters(
    TNS_INFO *const tnsInfo,
    int const subBlockNumber) {
  int n;

  assert(tnsInfo != NULL);
  assert(tnsInfo->numOfFilters[subBlockNumber] > 2);

  if (tnsInfo->numOfFilters[subBlockNumber] < 2) {
    return tnsInfo->numOfFilters[subBlockNumber];
  }

  tnsInfo->length[subBlockNumber][0] += tnsInfo->length[subBlockNumber][1];
  tnsInfo->length[subBlockNumber][1] = tnsInfo->length[subBlockNumber][2];
  tnsInfo->length[subBlockNumber][2] = tnsInfo->length[subBlockNumber][3];
  tnsInfo->order[subBlockNumber][0] = tnsInfo->order[subBlockNumber][1];
  tnsInfo->order[subBlockNumber][1] = tnsInfo->order[subBlockNumber][2];
  tnsInfo->order[subBlockNumber][2] = tnsInfo->order[subBlockNumber][3];
  assert(HEAAC_TNS_MAX_ORDER == sizeof(tnsInfo->coef[0][0]) / sizeof(tnsInfo->coef[0][0][0]));
  for (n = 0; n < HEAAC_TNS_MAX_ORDER; n++) {
    tnsInfo->coef[subBlockNumber][0][n] = tnsInfo->coef[subBlockNumber][1][n];
    tnsInfo->coef[subBlockNumber][1][n] = tnsInfo->coef[subBlockNumber][2][n];
    tnsInfo->coef[subBlockNumber][2][n] = tnsInfo->coef[subBlockNumber][3][n];
  }

  tnsInfo->numOfFilters[subBlockNumber]--;

  return tnsInfo->numOfFilters[subBlockNumber];
}

int iisaacfenc_TnsDetect(TNS_DATA *tnsData,
                         const TNS_CONFIG tC,
                         TNS_INFO *tnsInfo,
                         const int sfbCnt,
                         const int sfbOffset[],
                         float *spectrum,
                         const int subBlockNumber,
                         const int startSfb,
                         const int blockType) {
  float rxx[NUM_ANALYSIS_SECTIONS][HEAAC_TNS_MAX_ORDER + 1] = {{0.0f}};

  const int rxxLength = tC.maxOrder + 1;
  int i;
  int filterIdx = 0;
  int idx[NUM_ANALYSIS_SECTIONS];
  int sectionSfb[NUM_ANALYSIS_SECTIONS] = {0};

  assert(tnsInfo != NULL);
  assert(sfbOffset != NULL);
  assert(spectrum != NULL);
  assert(tC.maxOrder <= HEAAC_TNS_MAX_ORDER);
  assert(startSfb < sfbCnt);
  assert(blockType == SHORT_WINDOW || subBlockNumber == 0);

  TNS_SUBBLOCK_INFO *tsbi = (blockType == SHORT_WINDOW)
                                ? &tnsData->Short.subBlockInfo[subBlockNumber]
                                : &tnsData->Long.subBlockInfo;

  tsbi->tnsActive = 0;
  tsbi->predictionGainMax = 1.0f;
  tnsInfo->numOfFilters[subBlockNumber] = 0;
  tnsInfo->coefRes[subBlockNumber] = tC.coefRes;
  for (i = 0; i < HEAAC_TNS_MAX_ORDER; i++) {
    int n;
    for (n = 0; n < MAX_NUM_OF_FILTERS; n++) {
      tnsInfo->coef[subBlockNumber][n][i] = 0;
    }
  }
  for (i = 0; i < TRANS_FAC; i++) {
  }
  tnsInfo->length[subBlockNumber][0] = tnsInfo->length[subBlockNumber][1] = 0;
  tnsInfo->length[subBlockNumber][2] = tnsInfo->length[subBlockNumber][3] = 0;
  tnsInfo->order[subBlockNumber][0] = tnsInfo->order[subBlockNumber][1] = 0;
  tnsInfo->order[subBlockNumber][2] = tnsInfo->order[subBlockNumber][3] = 0;
  memset(tnsInfo->bTnsFilterShortened[subBlockNumber], 0, MAX_NUM_OF_FILTERS * sizeof(int));

  if (!tC.tnsActive) {
    return 0;
  }

  i = tC.lpcStartLine;
  idx[4] = tC.lpcStopLine - tC.lpcStartLine;
  idx[0] = i + idx[4] / 8;
  idx[1] = i + idx[4] / 4;
  idx[2] = i + idx[4] / 2;
  while (sfbOffset[sectionSfb[2]] < idx[2]) sectionSfb[2]++;
  idx[3] = i + idx[4] * 3 / 4;
  while (sfbOffset[sectionSfb[3]] < idx[3]) sectionSfb[3]++;
  idx[4] = tC.lpcStopLine;
  sectionSfb[4] = sfbCnt;
  sectionSfb[1] = tC.tnsStartBand;

  assert(tC.tnsStopBand == tC.lpcStopBand);

  if (blockType != SHORT_WINDOW) {
    int rxxLength01 = rxxLength - ((blockType == START_WINDOW + 7) || (blockType == STOPSTART_WINDOW + 7) ? 0 : 7);
    calcAcf(rxx[0], rxxLength01, &spectrum[tC.lpcStartLine], idx[0] - tC.lpcStartLine);
    calcAcf(rxx[1], rxxLength01, &spectrum[idx[0]], idx[1] - idx[0]);
  } else {
    memset(rxx[0], 0, sizeof(rxx[0]));
    memset(rxx[1], 0, sizeof(rxx[1]));
  }

  {
    int sectionIdx;
    for (sectionIdx = 2; sectionIdx < NUM_ANALYSIS_SECTIONS; sectionIdx++) {
      calcAcf(rxx[sectionIdx], rxxLength, &spectrum[idx[sectionIdx - 1]], idx[sectionIdx] - idx[sectionIdx - 1]);
    }
  }

  if ((rxx[0][0] + rxx[1][0] < MIN_NRG) && (rxx[2][0] < MIN_NRG) && (rxx[3][0] < MIN_NRG) && (rxx[4][0] < MIN_NRG)) {
    return 0;
  }

  if (blockType != SHORT_WINDOW) {
    normalizeAcf(rxx[0], rxxLength, 0.5f);
    normalizeAcf(rxx[1], rxxLength, 0.5f);
    mergeNormalizedAcfs(rxx[1], rxx[0], rxxLength);
    rxx[1][0] = 1.0;
  }
  {
    int sectionIdx;
    for (sectionIdx = 2; sectionIdx < NUM_ANALYSIS_SECTIONS; sectionIdx++) {
      normalizeAcf(rxx[sectionIdx], rxxLength, 1.0f);
    }
  }

  rxx[2][0] = rxx[3][0] = rxx[4][0] = 1.0f;

  if (blockType != SHORT_WINDOW) {
    int sectionIdx;
    int filterStopSfb;
    float rxxForFilter[HEAAC_TNS_MAX_ORDER + 1] = {0.0f};

    assert(sizeof(rxxForFilter) == sizeof(rxx[0]));
    assert(rxxLength >= 0);
    assert(sizeof(rxxForFilter) / sizeof(rxxForFilter[0]) >= (unsigned int)rxxLength);

    filterStopSfb = sectionSfb[4];

    for (sectionIdx = (NUM_ANALYSIS_SECTIONS - 1); sectionIdx > 1; sectionIdx--) {
      int bCreateFilterForCurrentRange;
      int filterStartSfb = sectionSfb[sectionIdx - 1];

      assert(sectionIdx > 0);

      addFLOAT(rxx[sectionIdx], rxxForFilter, rxxForFilter, rxxLength);

      bCreateFilterForCurrentRange = iisaacfenc_maxAbsDiffFLOAT(rxx[sectionIdx - 1], rxx[sectionIdx], rxxLength) > 0.666f;
      bCreateFilterForCurrentRange |= (sectionIdx == 2);

      if (bCreateFilterForCurrentRange) {
        TNS_FILTER_DESIGN tnsFilterDesign;
        designTnsFilter(
            rxxForFilter,
            rxxLength,
            tC.acfWindow,
            tC.maxOrder,
            tC.coefRes,
            filterStopSfb,
            filterStartSfb,
            1.0f,
            &tnsFilterDesign);

        tsbi->predictionGainMax = max(tnsFilterDesign.predictionGain, tsbi->predictionGainMax);

        filterIdx = addOrMergeTnsFilter(
            &tnsFilterDesign,
            subBlockNumber,
            filterIdx,
            tnsInfo);

        setFLOAT(0.0f, rxxForFilter, rxxLength);
        filterStopSfb = sectionSfb[sectionIdx - 1];
      }
    }

    {
      int bAddOrMergeLowFrequencyRangeFilter = 0;
      float prevPredictionGainMax = tsbi->predictionGainMax;
      TNS_FILTER_DESIGN tnsFilterDesign;
      int wouldFilterBeMerged;

      designTnsFilter(
          rxx[1],
          rxxLength,
          tC.acfWindow,
          tC.maxOrder - 7,
          tC.coefRes,
          tC.tnsStartBand,
          tC.lpcStartBand,
          3.0f,
          &tnsFilterDesign);

      wouldFilterBeMerged = isFilterMerged(&tnsFilterDesign, subBlockNumber, filterIdx, tnsInfo);

      tsbi->predictionGainMax = max(tnsFilterDesign.predictionGain, tsbi->predictionGainMax);

      if (wouldFilterBeMerged) {
        bAddOrMergeLowFrequencyRangeFilter = 1;
      } else if (tnsInfo->numOfFilters[subBlockNumber] == TRUE_MAX_NUM_FILTERS) {
        if (!wouldFilterBeMerged && prevPredictionGainMax < tsbi->predictionGainMax) {
          if (tnsInfo->bTnsFilterShortened[subBlockNumber][0] == 0 && tnsInfo->bTnsFilterShortened[subBlockNumber][1] == 0) {
            filterIdx = mergeTwoUpperFilters(
                tnsInfo,
                subBlockNumber);
            bAddOrMergeLowFrequencyRangeFilter = 1;
          }
        }
      } else {
        bAddOrMergeLowFrequencyRangeFilter = 1;
      }

      if (bAddOrMergeLowFrequencyRangeFilter) {
        filterIdx = addOrMergeTnsFilter(
            &tnsFilterDesign,
            subBlockNumber,
            filterIdx,
            tnsInfo);
      }
    }

  } else {
    assert(blockType == SHORT_WINDOW);
    addFLOAT(rxx[3], rxx[4], rxx[3], rxxLength);
    addFLOAT(rxx[2], rxx[3], rxx[2], rxxLength);

    TNS_FILTER_DESIGN tnsFilterDesign;
    designTnsFilter(
        rxx[2],
        rxxLength,
        tC.acfWindow,
        tC.maxOrder,
        tC.coefRes,
        sectionSfb[4],
        sectionSfb[1],
        1.0f,
        &tnsFilterDesign);

    tsbi->predictionGainMax = max(tnsFilterDesign.predictionGain, tsbi->predictionGainMax);

    filterIdx = addOrMergeTnsFilter(
        &tnsFilterDesign,
        subBlockNumber,
        filterIdx,
        tnsInfo);
  }

  filterIdx = min(filterIdx, 3);

  for (; filterIdx > 0; filterIdx--) {
    if (tnsInfo->order[subBlockNumber][filterIdx - 1] > 0) {
      break;
    }
  }

  tnsInfo->predictionGainPerFilter[subBlockNumber][3] = 0;
  setINT(0, tnsInfo->coef[subBlockNumber][3], HEAAC_TNS_MAX_ORDER);

  tnsInfo->numOfFilters[subBlockNumber] = filterIdx;
  tsbi->tnsActive = (tnsInfo->numOfFilters[subBlockNumber] > 0);

  {
    int n;
    assert(MAX_NUM_OF_FILTERS == sizeof(tnsInfo->order[subBlockNumber]) / sizeof(tnsInfo->order[subBlockNumber][0]));
    assert(MAX_NUM_OF_FILTERS == sizeof(tnsInfo->length[subBlockNumber]) / sizeof(tnsInfo->length[subBlockNumber][0]));
    for (n = filterIdx; n < MAX_NUM_OF_FILTERS; n++) {
      tnsInfo->order[subBlockNumber][filterIdx] = 0;
      tnsInfo->length[subBlockNumber][filterIdx] = 0;
    }
  }

  calcTnsGainDecoderHeadroomRatio(spectrum,
                                  tnsInfo,
                                  sfbOffset,
                                  tC.tnsStopLine,
                                  subBlockNumber,
                                  sfbCnt,
                                  tsbi->predictionGainMax,
                                  &tsbi->tnsGainHeadroomRatio);

  return filterIdx;
}

void iisaacfenc_TnsSync(TNS_DATA *tnsDataDest,
                        TNS_DATA *tnsDataSrc,
                        TNS_INFO *tnsInfoDest,
                        TNS_INFO *tnsInfoSrc,
                        const TNS_CONFIG tC,
                        const int subBlockNumber,
                        const int blockType) {
  TNS_SUBBLOCK_INFO *sbInfoDest;
  const TNS_SUBBLOCK_INFO *sbInfoSrc;
  int i, numOfFiltMax, absDiff, absDiffSum = 0;

  if (blockType == SHORT_WINDOW) {
    sbInfoDest = &tnsDataDest->Short.subBlockInfo[subBlockNumber];
    sbInfoSrc = &tnsDataSrc->Short.subBlockInfo[subBlockNumber];
    numOfFiltMax = 1;
  } else {
    sbInfoDest = &tnsDataDest->Long.subBlockInfo;
    sbInfoSrc = &tnsDataSrc->Long.subBlockInfo;
    numOfFiltMax = 3;
  }

  if (sbInfoDest->tnsActive || sbInfoSrc->tnsActive) {
    int numOfFiltSrc = tnsInfoSrc->numOfFilters[subBlockNumber];
    int numOfFiltDest = tnsInfoDest->numOfFilters[subBlockNumber];
    int idxSrc = 0, idxDest = 1, notDone = 1;
    int lenSrc = 0, lenDest = tnsInfoDest->length[subBlockNumber][0];

    while (notDone &= (idxSrc < numOfFiltSrc)) {
      do {
        lenSrc += tnsInfoSrc->length[subBlockNumber][idxSrc++];
        notDone = (idxSrc < numOfFiltSrc);
      } while ((notDone) && (lenSrc < lenDest));

      do {
        const int s = idxSrc - 1;
        const int d = idxDest - 1;
        absDiffSum = 0;
        for (i = 0; i < tC.maxOrder; i++) {
          absDiff = tnsInfoDest->coef[subBlockNumber][d][i] - tnsInfoSrc->coef[subBlockNumber][s][i];
          absDiffSum += absDiff * absDiff;
        }

        if (absDiffSum < 3) {
          tnsInfoDest->order[subBlockNumber][d] = tnsInfoSrc->order[subBlockNumber][s];
          tnsInfoDest->direction[subBlockNumber][d] = tnsInfoSrc->direction[subBlockNumber][s];
          for (i = 0; i < tC.maxOrder; i++) {
            tnsInfoDest->coef[subBlockNumber][d][i] = tnsInfoSrc->coef[subBlockNumber][s][i];
          }
          if (numOfFiltDest == 0) {
            assert(idxDest == 1);
            tnsInfoDest->length[subBlockNumber][d] = lenDest = tnsInfoSrc->length[subBlockNumber][s];
            tnsInfoDest->length[subBlockNumber][1] = tnsInfoDest->order[subBlockNumber][1] = 0;
            sbInfoDest->tnsActive = 1;
            tnsInfoDest->numOfFilters[subBlockNumber] = ++numOfFiltDest;
          }
        }
        lenDest += tnsInfoDest->length[subBlockNumber][idxDest++];
        notDone = (idxDest < numOfFiltDest);
      } while ((notDone) && (lenSrc >= lenDest));
    }

    while (idxSrc < numOfFiltSrc) {
      lenSrc += tnsInfoSrc->length[subBlockNumber][idxSrc++];
      notDone = 1;
    }
    while (idxDest < numOfFiltDest) {
      lenDest += tnsInfoDest->length[subBlockNumber][idxDest++];
    }

    if (lenSrc > lenDest) {
      if ((notDone) && (numOfFiltDest < numOfFiltMax)) {
        idxSrc = numOfFiltSrc - 1;
        idxDest = numOfFiltDest;
        absDiffSum = 0;
        for (i = 0; i < tC.maxOrder; i++) {
          absDiff = tnsInfoDest->coef[subBlockNumber][idxDest][i] - tnsInfoSrc->coef[subBlockNumber][idxSrc][i];
          absDiffSum += absDiff * absDiff;
        }

        if ((absDiffSum < 7) && (sbInfoDest->predictionGainMax > 1.8046875f)) {
          tnsInfoDest->length[subBlockNumber][idxDest] = lenSrc - lenDest;
          tnsInfoDest->order[subBlockNumber][idxDest] = tnsInfoSrc->order[subBlockNumber][idxSrc];
          tnsInfoDest->direction[subBlockNumber][idxDest] = tnsInfoSrc->direction[subBlockNumber][idxSrc];
          for (i = 0; i < tC.maxOrder; i++) {
            tnsInfoDest->coef[subBlockNumber][idxDest][i] = tnsInfoSrc->coef[subBlockNumber][idxSrc][i];
          }
          if (idxDest == 0) sbInfoDest->tnsActive = 1;
          tnsInfoDest->numOfFilters[subBlockNumber] = idxDest + 1;
        }
      } else if ((!notDone) && (numOfFiltDest > 0) && (absDiffSum < 3)) {
        tnsInfoDest->length[subBlockNumber][numOfFiltDest - 1] += lenSrc - lenDest;
      }
    } else if ((lenSrc < lenDest) && (numOfFiltSrc > 0 || numOfFiltDest < 2)) {
      idxDest = numOfFiltDest - 1;
      assert(idxDest >= 0);

      if (numOfFiltSrc == 0 && numOfFiltDest == 1) {
        absDiffSum = 0;
        for (i = 0; i < tC.maxOrder; i++) {
          absDiff = tnsInfoDest->coef[subBlockNumber][idxDest][i] - tnsInfoSrc->coef[subBlockNumber][idxDest][i];
          absDiffSum += absDiff * absDiff;
        }

        if (absDiffSum > 1) {
          return;
        }
      }
      tnsInfoDest->length[subBlockNumber][idxDest] += lenSrc - lenDest;
      if (tnsInfoDest->length[subBlockNumber][idxDest] <= 0) {
        tnsInfoDest->length[subBlockNumber][idxDest] = tnsInfoDest->order[subBlockNumber][idxDest] = 0;

        if (idxDest == 0) sbInfoDest->tnsActive = 0;
        tnsInfoDest->numOfFilters[subBlockNumber] = idxDest;
      }
    }

    if (notDone && (numOfFiltSrc == 2) && (numOfFiltDest == 2) && (lenSrc == lenDest) && (idxSrc == idxDest)) {
      absDiffSum = 0;
      for (i = 0; i < tC.maxOrder; i++) {
        absDiff = tnsInfoDest->coef[subBlockNumber][1][i] - tnsInfoSrc->coef[subBlockNumber][1][i];
        absDiffSum += absDiff * absDiff;
      }

      if (absDiffSum < 3) {
        tnsInfoDest->order[subBlockNumber][1] = tnsInfoSrc->order[subBlockNumber][1];
        tnsInfoDest->direction[subBlockNumber][1] = tnsInfoSrc->direction[subBlockNumber][1];
        for (i = 0; i < tC.maxOrder; i++) {
          tnsInfoDest->coef[subBlockNumber][1][i] = tnsInfoSrc->coef[subBlockNumber][1][i];
        }
      }
    }

    for (i = tnsInfoDest->numOfFilters[subBlockNumber]; i > 0; i--) {
      if (tnsInfoDest->order[subBlockNumber][i - 1] > 0) {
        break;
      }
    }
    if ((tnsInfoDest->numOfFilters[subBlockNumber] = i) > 0) {
      sbInfoDest->tnsActive = 1;
    } else {
      tnsInfoDest->length[subBlockNumber][0] = tnsInfoDest->order[subBlockNumber][0] = 0;
      sbInfoDest->tnsActive = 0;
    }
  }
}

int iisaacfenc_TnsEncode(TNS_INFO *tnsInfo,
                         TNS_DATA *tnsData,
                         const int numOfSfb,
                         TNS_CONFIG tC,
                         float *spectrum,
                         const int sfbOffset[],
                         int subBlockNumber,
                         int blockType) {
  int i, startBand = numOfSfb, startLine = tC.tnsStopLine;

  TNS_SUBBLOCK_INFO *tsbi = (blockType == SHORT_WINDOW)
                                ? &tnsData->Short.subBlockInfo[subBlockNumber]
                                : &tnsData->Long.subBlockInfo;

  if (!tsbi->tnsActive) {
    return 0;
  }

  for (i = 0; i < tnsInfo->numOfFilters[subBlockNumber]; i++) {
    const int stopLine = startLine;
    float parcor[HEAAC_TNS_MAX_ORDER] = {0.0f};
    assert(tnsInfo->length[subBlockNumber][i] > 0);
    startBand -= tnsInfo->length[subBlockNumber][i];
    assert(startBand > 0);
    startLine = sfbOffset[startBand];
    iisaacfenc_Index2Parcor(tnsInfo->coef[subBlockNumber][i], parcor,
                            tnsInfo->order[subBlockNumber][i], tC.coefRes);

    iisaacfenc_AnalysisFilterLattice(&spectrum[startLine], stopLine - startLine, parcor,
                                     tnsInfo->order[subBlockNumber][i], &spectrum[startLine]);
  }

  return i;
}

int iisaacfenc_TnsDecode(TNS_INFO *tnsInfo,
                         int numOfSfb,
                         TNS_CONFIG tC,
                         const int sfbOffset[],
                         float *spectrum,
                         int subBlockNumber) {
  float lpcCoeff[TNS_MAX_ORDER] = {0};
  float parCoeff[TNS_MAX_ORDER] = {0};
  int i, filterIdx;
  int startBand = numOfSfb, startLine = tC.tnsStopLine;
  const float pi2 = 3.14159265358979323846f / 2.0f;
  float iqfac = ((1 << (tnsInfo->coefRes[subBlockNumber] - 1)) - 0.5F) / pi2;
  float iqfac_m = ((1 << (tnsInfo->coefRes[subBlockNumber] - 1)) + 0.5F) / pi2;

  for (filterIdx = 0; filterIdx < tnsInfo->numOfFilters[subBlockNumber]; filterIdx++) {
    const int stopLine = startLine;
    assert(tnsInfo->length[subBlockNumber][filterIdx] > 0);
    startBand -= tnsInfo->length[subBlockNumber][filterIdx];
    assert(startBand > 0);
    startLine = sfbOffset[startBand];

    for (i = 0; i < tnsInfo->order[subBlockNumber][filterIdx]; i++) {
      const float c = (tnsInfo->coef[subBlockNumber][filterIdx][i] >= 0) ? iqfac : iqfac_m;
      parCoeff[i + 1] = (float)sin((double)(tnsInfo->coef[subBlockNumber][filterIdx][i] / c));
    }

    iisaacfenc_ParcorToLpc(parCoeff, lpcCoeff, tnsInfo->order[subBlockNumber][filterIdx]);

    iisaacfenc_SynthesisFilter(&spectrum[startLine], stopLine - startLine, lpcCoeff, tnsInfo->order[subBlockNumber][filterIdx],
                               &spectrum[startLine], FILTER_DIRECTION);
  }

  return (0);
}

static float iisaacfenc_maxAbsDiffFLOAT(const float *f1, const float *f2, const int size) {
  float maxAbsDiff = 0.0f;
  int i;

  for (i = 0; i < size; i++) {
    const float absDiff = (float)fabs(f1[i] - f2[i]);
    if (absDiff > maxAbsDiff) {
      maxAbsDiff = absDiff;
    }
  }
  return maxAbsDiff;
}

static float iisaacfenc_AutoToParcor(
    float const *const autoCorr,
    float *const reflCoeff,
    int const numOfCoeff) {
  int i;
  float tmp;
  float *pWorkBuffer;
  float predictionGain = 0.0f;
  float workBuffer[2 * HEAAC_TNS_MAX_ORDER] = {0};
  assert(numOfCoeff <= HEAAC_TNS_MAX_ORDER);

  setFLOAT(0, reflCoeff, numOfCoeff);

  for (i = 0; i < numOfCoeff; i++) {
    workBuffer[i] = autoCorr[i];
    workBuffer[i + numOfCoeff] = autoCorr[i + 1];
  }

  pWorkBuffer = &(workBuffer[numOfCoeff]);

  for (i = 0; i < numOfCoeff; i++) {
    int j;
    if (workBuffer[0] <= MIN_NRG) {
      tmp = 0.0f;
    } else {
      tmp = -(workBuffer[numOfCoeff + i] / workBuffer[0]);
    }

    tmp = min(0.999f, max(-0.999f, tmp));

    for (j = i; j < numOfCoeff; j++) {
      float tmp2 = pWorkBuffer[j] + tmp * workBuffer[j - i];
      workBuffer[j - i] += tmp * pWorkBuffer[j];
      pWorkBuffer[j] = tmp2;
    }

    predictionGain = (autoCorr[0] + 1e-30f) / (workBuffer[0] + 1e-30f);

    reflCoeff[i] = tmp;
  }

  return predictionGain;
}

static void iisaacfenc_Parcor2Index(const float parcor[], int index[], int order,
                                    int bitsPerCoeff) {
  int i;
  float qFac, qFacM;
  float tmp;
  int minVal, maxVal;

  minVal = bitsPerCoeff == 3 ? -4 : -8;
  maxVal = bitsPerCoeff == 3 ? 3 : 7;

  qFac = qTable[bitsPerCoeff - 2];
  qFacM = qTableM[bitsPerCoeff - 2];

  for (i = 0; i < order; i++) {
    if (parcor[i] < 0.0f) {
      assert(parcor[i] >= -1.0f);
      tmp = (float)asin(parcor[i]) * qFacM;
    } else {
      assert(parcor[i] <= 1.0f);
      tmp = (float)asin(parcor[i]) * qFac;
    }
    if (tmp > 0.0f) {
      index[i] = min(maxVal, (int)(tmp + 0.5f));
    } else {
      index[i] = max(minVal, (int)(tmp - 0.5f));
    }
  }
}

static void iisaacfenc_Index2Parcor(const int index[], float parcor[], int order,
                                    int bitsPerCoeff) {
  int i;

  for (i = 0; i < order; i++) {
    parcor[i] = bitsPerCoeff == 4 ? tnsCoeff4[index[i] + 8] : tnsCoeff3[index[i] + 4];
  }
}

static float iisaacfenc_AnalysisFIRLattice(int order, float x, float *state_par,
                                           const float *coef_par) {
  int i;
  float tmp, tmpSave = x;

  for (i = 0; i < order - 1; i++) {
    tmp = coef_par[i] * x + state_par[i];
    x += coef_par[i] * state_par[i];
    state_par[i] = tmpSave;
    tmpSave = tmp;
  }

  x += state_par[order - 1] * coef_par[order - 1];
  state_par[order - 1] = tmpSave;

  return x;
}

static void iisaacfenc_AnalysisFilterLattice(const float signal[], const int numOfLines,
                                             const float parCoeff[], const int order,
                                             float output[]) {
  float state_par[HEAAC_TNS_MAX_ORDER] = {0.0f};
  int j;

  if (order <= 0) {
    return;
  }
  for (j = 0; j < numOfLines; j++) {
    output[j] = iisaacfenc_AnalysisFIRLattice(order, signal[j], state_par, parCoeff);
  }
}

static void iisaacfenc_SynthesisFilter(const float signal[], int numOfLines,
                                       const float predictorCoeff[], int order,
                                       float output[], int direction) {
  float statusVar[TNS_MAX_ORDER] = {0.0f};
  const float *signalPtr;
  float *outputPtr;
  int modifier;
  int i, j;
  float tmp;

  if (direction != 0) {
    signalPtr = signal + numOfLines - 1;
    outputPtr = output + numOfLines - 1;
    modifier = -1;
  } else {
    signalPtr = signal;
    outputPtr = output;
    modifier = 1;
  }

  for (j = 0; j < numOfLines; j++) {
    tmp = signalPtr[j * modifier];
    for (i = 0; i < order; i++) {
      tmp -= predictorCoeff[i + 1] * statusVar[i];
    }
    outputPtr[j * modifier] = tmp;
    for (i = order - 1; i > 0; i--) {
      statusVar[i] = statusVar[i - 1];
    }
    statusVar[0] = signalPtr[j * modifier];
  }
}

static void iisaacfenc_ParcorToLpc(const float reflCoeff[], float LpcCoeff[],
                                   int numOfCoeff) {
  int i, j;
  float b[TNS_MAX_ORDER] = {0};
  LpcCoeff[0] = 1;
  b[0] = LpcCoeff[0];

  for (i = 1; i <= numOfCoeff; i++) {
    for (j = 1; j < i; j++) {
      b[j] = LpcCoeff[j] + reflCoeff[i] * LpcCoeff[i - j];
    }

    b[i] = reflCoeff[i];

    for (j = 0; j <= i; j++) {
      LpcCoeff[j] = b[j];
    }
  }
}

int iisaacfenc_equalTnsFilters(TNS_DATA *tnsData[2],
                               TNS_INFO const *tnsInfo0,
                               const TNS_INFO *tnsInfo1,
                               const int bShortBlock) {
  int i;
  unsigned int enDiff = 0;

  if (bShortBlock) {
    for (i = 0; i < TRANS_FAC; i++) {
      if ((tnsData[0]->Short.subBlockInfo[i].tnsActive != tnsData[1]->Short.subBlockInfo[i].tnsActive) ||
          (tnsInfo0->numOfFilters[i] != tnsInfo1->numOfFilters[i])) {
        return 0;
      }
      if (tnsInfo0->numOfFilters[i] > 0) {
        int k;

        if ((tnsInfo0->length[i][0] != tnsInfo1->length[i][0]) ||
            (tnsInfo0->order[i][0] != tnsInfo1->order[i][0])) {
          return 0;
        }

        for (k = 0; k < tnsInfo0->order[i][0]; k++) {
          enDiff += (tnsInfo0->coef[i][0][k] - tnsInfo1->coef[i][0][k]) *
                    (tnsInfo0->coef[i][0][k] - tnsInfo1->coef[i][0][k]);
        }
      }
    }
  } else {
    if ((tnsData[0]->Long.subBlockInfo.tnsActive != tnsData[1]->Long.subBlockInfo.tnsActive) ||
        (tnsInfo0->numOfFilters[0] != tnsInfo1->numOfFilters[0])) {
      return 0;
    }
    for (i = 0; i < tnsInfo0->numOfFilters[0]; i++) {
      int k;

      if ((tnsInfo0->length[0][i] != tnsInfo1->length[0][i]) ||
          (tnsInfo0->order[0][i] != tnsInfo1->order[0][i])) {
        return 0;
      }

      for (k = 0; k < tnsInfo0->order[0][i]; k++) {
        enDiff += (tnsInfo0->coef[0][i][k] - tnsInfo1->coef[0][i][k]) *
                  (tnsInfo0->coef[0][i][k] - tnsInfo1->coef[0][i][k]);
      }
    }
  }

  return (enDiff == 0);
}

int iisaacfenc_isTnsFilterShortened(
    TNS_INFO *tnsInfo) {
  int bFilterShortened = 0;
  int windowIdx;
  int filterIdx;

  for (windowIdx = 0; windowIdx < TRANS_FAC; windowIdx++) {
    for (filterIdx = 0; filterIdx < MAX_NUM_OF_FILTERS; filterIdx++) {
      if (tnsInfo->bTnsFilterShortened[windowIdx][filterIdx] == 1) {
        bFilterShortened = 1;
        break;
      }
    }
  }

  return bFilterShortened;
}
