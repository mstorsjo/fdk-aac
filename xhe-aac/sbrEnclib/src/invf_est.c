
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
#include <string.h>
#include <math.h>

#include "iisutillib.h"
#include "invf_est.h"
#include "mathlib.h"
#include "sbr_misc.h"
#include "sbr_def.h"
#include "sbr.h"

#define MAX_NUM_REGIONS 10

static const DETECTOR_PARAMETERS detectorParamsAAC = {
    {1.0f, 10.0f, 14.0f, 19.0f},
    {0.0f, 3.0f, 7.0f, 10.0f},
    {-20.0f, -15.0f, -10.0f, -5.0f},
    4,
    4,
    4,
    1.0f,
    {{INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF, INVF_OFF},
     {INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF}},
    {{INVF_LOW_LEVEL, INVF_LOW_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_LOW_LEVEL, INVF_LOW_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF}},
    {-4, -3, -2, -1, 0}};

static const DETECTOR_PARAMETERS detectorParamsAACSpeech = {
    {1.0f, 10.0f, 14.0f, 19.0f},
    {0.0f, 3.0f, 7.0f, 10.0f},
    {-20.0f, -15.0f, -10.0f, -5.0f},
    4,
    4,
    4,
    1.0f,
    {{INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF}},
    {{INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_LOW_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF},
     {INVF_HIGH_LEVEL, INVF_HIGH_LEVEL, INVF_MID_LEVEL, INVF_OFF, INVF_OFF}},
    {-4, -3, -2, -1, 0}};

typedef const float FIR_FILTER[5];

static FIR_FILTER fir_0 = {1.0f};
static FIR_FILTER fir_1 = {0.3333333f, 0.6666666f};
static FIR_FILTER fir_2 = {0.125f, 0.375f, 0.5f};
static FIR_FILTER fir_3 = {0.0585786f, 0.2f, 0.3414214f, 0.4f};
static FIR_FILTER fir_4 = {0.0318305f, 0.1151638f, 0.2181695f, 0.3015028f, 0.3333333f};

static FIR_FILTER *fir_table[5] = {
    &fir_0,
    &fir_1,
    &fir_2,
    &fir_3,
    &fir_4};

static void
calculateDetectorValues(float **quotaMatrixOrig,
                        int *indexVector,
                        float *nrgVector,
                        float **quotaMatrixPatch,
                        DETECTOR_VALUES *detectorValues,
                        int startChannel,
                        int stopChannel,
                        int startIndex,
                        int stopIndex,
                        int smoothingLength,
                        SBR_WITH_LD CODEC_TYPE coreCodec) {
  int i, j;

  float origQuota, sbrQuota;

  const float *filter = *fir_table[smoothingLength];

  float quotaVecOrig[64], quotaVecSbr[64];

  float **quotaMatrixSbr;

  if (quotaMatrixPatch) {
    quotaMatrixSbr = quotaMatrixPatch;
  } else {
    quotaMatrixSbr = quotaMatrixOrig;
  }

  setFLOAT(0.0f, quotaVecOrig, 64);
  setFLOAT(0.0f, quotaVecSbr, 64);

  detectorValues->avgNrg = 0.f;
  for (j = startIndex; j < stopIndex; j++) {
    for (i = startChannel; i < stopChannel; i++) {
      quotaVecOrig[i] += (quotaMatrixOrig[j][i]);

      if (indexVector[i] != -1) {
        quotaVecSbr[i] += (quotaMatrixSbr[j][indexVector[i]]);
      }
    }
    detectorValues->avgNrg += nrgVector[j];
  }

  detectorValues->avgNrg /= (stopIndex - startIndex);

  for (i = startChannel; i < stopChannel; i++) {
    quotaVecOrig[i] /= (stopIndex - startIndex);
    quotaVecSbr[i] /= (stopIndex - startIndex);
  }

  assert((stopIndex - startIndex) > 0);

  origQuota = 0.0f;
  sbrQuota = 0.0f;
  for (i = startChannel; i < stopChannel; i++) {
    origQuota += quotaVecOrig[i];
    sbrQuota += quotaVecSbr[i];
  }

  origQuota /= (stopChannel - startChannel);
  sbrQuota /= (stopChannel - startChannel);

  assert((stopChannel - startChannel) > 0);

  memmove(detectorValues->origQuotaMean, detectorValues->origQuotaMean + 1, smoothingLength * sizeof(float));
  memmove(detectorValues->sbrQuotaMean, detectorValues->sbrQuotaMean + 1, smoothingLength * sizeof(float));

  detectorValues->origQuotaMean[smoothingLength] = origQuota;
  detectorValues->sbrQuotaMean[smoothingLength] = sbrQuota;

  detectorValues->origQuotaMeanFilt = 0.f;
  detectorValues->sbrQuotaMeanFilt = 0.f;

  for (i = 0; i < smoothingLength + 1; i++) {
    detectorValues->origQuotaMeanFilt += detectorValues->origQuotaMean[i] * filter[i];
    detectorValues->sbrQuotaMeanFilt += detectorValues->sbrQuotaMean[i] * filter[i];
  }
}

static int
findRegion(float currVal,
           const float *borders,
           const int numBorders) {
  int i;

  if (currVal < borders[0]) {
    return 0;
  }

  for (i = 1; i < numBorders; i++) {
    if (currVal >= borders[i - 1] && currVal < borders[i]) {
      return i;
    }
  }

  if (currVal >= borders[numBorders - 1]) {
    return numBorders;
  }

  assert(0);
  return 0;
}

static INVF_MODE
decisionAlgorithm(const DETECTOR_PARAMETERS *detectorParams,
                  DETECTOR_VALUES detectorValues,
                  int transientFlag,
                  int *prevRegionSbr,
                  int *prevRegionOrig,
                  CODEC_TYPE coreCodec) {
  int invFiltLevel, regionSbr, regionOrig, regionNrg;

  const float *quantStepsSbr = detectorParams->quantStepsSbr;
  const float *quantStepsOrig = detectorParams->quantStepsOrig;
  const float *nrgBorders = detectorParams->nrgBorders;
  const int numRegionsSbr = detectorParams->numRegionsSbr;
  const int numRegionsOrig = detectorParams->numRegionsOrig;
  const int numRegionsNrg = detectorParams->numRegionsNrg;
  const float delta = detectorParams->delta;

  float quantStepsSbrTmp[MAX_NUM_REGIONS];
  float quantStepsOrigTmp[MAX_NUM_REGIONS];

  float origQuotaMeanFilt;
  float sbrQuotaMeanFilt;
  float nrg;

  switch (coreCodec) {
    case CODEC_SAAC:
      origQuotaMeanFilt = (float)(ILOG2 * 3.0 * log(detectorValues.origQuotaMeanFilt + EPS));
      sbrQuotaMeanFilt = (float)(ILOG2 * 3.0 * log(detectorValues.sbrQuotaMeanFilt + EPS));
      nrg = (float)(ILOG2 * 1.5 * log(detectorValues.avgNrg + EPS_NS));
      break;

    default:
      origQuotaMeanFilt = (float)(ILOG2 * 3.0 * log(detectorValues.origQuotaMeanFilt + EPS));
      sbrQuotaMeanFilt = (float)(ILOG2 * 3.0 * log(detectorValues.sbrQuotaMeanFilt + EPS));
      nrg = (float)(ILOG2 * 1.5 * log(detectorValues.avgNrg + EPS));
      break;
  }

  memcpy(quantStepsSbrTmp, quantStepsSbr, numRegionsSbr * sizeof(float));
  memcpy(quantStepsOrigTmp, quantStepsOrig, numRegionsOrig * sizeof(float));

  switch (coreCodec) {
    default:
      if (*prevRegionSbr < numRegionsSbr)
        quantStepsSbrTmp[*prevRegionSbr] = quantStepsSbr[*prevRegionSbr] + delta;
      if (*prevRegionSbr > 0)
        quantStepsSbrTmp[*prevRegionSbr - 1] = quantStepsSbr[*prevRegionSbr - 1] - delta;

      if (*prevRegionOrig < numRegionsOrig)
        quantStepsOrigTmp[*prevRegionOrig] = quantStepsOrig[*prevRegionOrig] + delta;
      if (*prevRegionOrig > 0)
        quantStepsOrigTmp[*prevRegionOrig - 1] = quantStepsOrig[*prevRegionOrig - 1] - delta;
  }

  regionSbr = findRegion(sbrQuotaMeanFilt, quantStepsSbrTmp, numRegionsSbr);
  regionOrig = findRegion(origQuotaMeanFilt, quantStepsOrigTmp, numRegionsOrig);
  regionNrg = findRegion(nrg, nrgBorders, numRegionsNrg);

  *prevRegionSbr = regionSbr;
  *prevRegionOrig = regionOrig;

  if (transientFlag == 1) {
    invFiltLevel = detectorParams->regionSpaceTransient[regionSbr][regionOrig];
  } else {
    invFiltLevel = detectorParams->regionSpace[regionSbr][regionOrig];
  }

  invFiltLevel = max(invFiltLevel + detectorParams->EnergyCompFactor[regionNrg], 0);

  return (INVF_MODE)(invFiltLevel);
}

void QmfInverseFilteringDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                                 float **quotaMatrix,
                                 float *nrgVector,
                                 float **quotaMatrixPatch,
                                 int *indexVector,
                                 int startIndex,
                                 int stopIndex,
                                 int transientFlag,
                                 INVF_MODE *infVec,
                                 CODEC_TYPE coreCodec) {
  int band;

  for (band = 0; band < hInvFilt->noDetectorBands; band++) {
    int startChannel = hInvFilt->freqBandTableInvFilt[band];
    int stopChannel = hInvFilt->freqBandTableInvFilt[band + 1];

    calculateDetectorValues(quotaMatrix,
                            indexVector,
                            nrgVector,
                            quotaMatrixPatch,
                            &hInvFilt->detectorValues[band],
                            startChannel,
                            stopChannel,
                            startIndex,
                            stopIndex,
                            hInvFilt->smoothingLength,
                            coreCodec);

    infVec[band] = decisionAlgorithm(hInvFilt->detectorParams,
                                     hInvFilt->detectorValues[band],
                                     transientFlag,
                                     &hInvFilt->prevRegionSbr[band],
                                     &hInvFilt->prevRegionOrig[band],
                                     coreCodec);
  }
}

HANDLE_ERROR_INFO
CreateInvFiltDetector(HANDLE_SBR_INV_FILT_EST *hInvFilt,
                      CODEC_TYPE coreCoder,
                      const int *freqBandTableDetector,
                      int numDetectorBands,
                      unsigned int useSpeechConfig

) {
  int i;
  HANDLE_SBR_INV_FILT_EST hs;
  HANDLE_ERROR_INFO err = noError;

  *hInvFilt = NULL;

  hs = (HANDLE_SBR_INV_FILT_EST)iisCalloc(1, sizeof(SBR_INV_FILT_EST));

  if (hs == NULL)
    return iisUtil_ERROR(CDI, "Memory allocation in function createInvFiltDetector() failed");

  switch (coreCoder) {
    case CODEC_SAAC:
      hs->freqBandTableInvFilt = (int *)iisCalloc((numDetectorBands + 1), sizeof(int));
      if (hs->freqBandTableInvFilt == NULL) {
        DeleteInvFiltDetector(hs);
        return iisUtil_ERROR(CDI, "Memory allocation in function createInvFiltDetector() failed");
      }
      break;
    default:
      return iisUtil_ERROR(CDI, "unknown codec type");
  }

  switch (coreCoder) {
    case CODEC_SAAC:
      if (useSpeechConfig) {
        hs->detectorParams = &detectorParamsAACSpeech;
      } else {
        hs->detectorParams = &detectorParamsAAC;
      }
      hs->smoothingLength = 2;
      hs->noDetectorBandsMax = numDetectorBands;
      break;
    default:
      return iisUtil_ERROR(CDI, "unknown codec type");
  }

  for (i = 0; i < hs->noDetectorBandsMax; i++) {
    memset(&hs->detectorValues[i], 0, sizeof(DETECTOR_VALUES));

    hs->detectorValues[i].origQuotaMean = (float *)iisCalloc(hs->smoothingLength + 1, sizeof(float));
    hs->detectorValues[i].sbrQuotaMean = (float *)iisCalloc(hs->smoothingLength + 1, sizeof(float));

    if (hs->detectorValues[i].origQuotaMean == NULL ||
        hs->detectorValues[i].sbrQuotaMean == NULL) {
      DeleteInvFiltDetector(hs);
      return iisUtil_ERROR(CDI, "Memory allocation in function createInvFiltDetector() failed");
    }

    hs->prevRegionOrig[i] = 0;
    hs->prevRegionSbr[i] = 0;
  }

  err = ResetInvFiltDetector(hs,
                             freqBandTableDetector,
                             numDetectorBands,
                             coreCoder);

  if (err != noError)
    return handBack(err);

  *hInvFilt = hs;
  return noError;
}

HANDLE_ERROR_INFO
ResetInvFiltDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                     const int *freqBandTableDetector,
                     int numDetectorBands,
                     CODEC_TYPE coreCodec) {
  switch (coreCodec) {
    case CODEC_SAAC:
      memcpy(hInvFilt->freqBandTableInvFilt, freqBandTableDetector, (numDetectorBands + 1) * sizeof(int));
      hInvFilt->noDetectorBands = numDetectorBands;
      break;
    default:
      return iisUtil_ERROR(CDI, "unknown codec type");
  }

  return noError;
}

void DeleteInvFiltDetector(HANDLE_SBR_INV_FILT_EST hs) {
  int i;

  if (hs) {
    for (i = 0; i < hs->noDetectorBandsMax; i++) {
      if (hs->detectorValues[i].origQuotaMean)
        iisFree(hs->detectorValues[i].origQuotaMean);
      if (hs->detectorValues[i].sbrQuotaMean)
        iisFree(hs->detectorValues[i].sbrQuotaMean);
    }

    if (hs->freqBandTableInvFilt)
      iisFree(hs->freqBandTableInvFilt);

    iisFree(hs);
  }
}
