
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
#include "sbr.h"
#include "ton_corr.h"
#include "invf_est.h"
#include "mathlib.h"
#include "sbr_misc.h"

#ifndef _TMS320C6700
typedef double SBR_DOUBLE;
#else
typedef float SBR_DOUBLE;
#endif

typedef struct {
  SBR_DOUBLE r00r;
  SBR_DOUBLE r11r;
  SBR_DOUBLE r01r;
  SBR_DOUBLE r01i;
  SBR_DOUBLE r02r;
  SBR_DOUBLE r02i;
  SBR_DOUBLE r12r;
  SBR_DOUBLE r12i;
  SBR_DOUBLE r22r;
  SBR_DOUBLE det;
} ACORR_COEFS;

static void
calcAutoCorrSecondOrder(ACORR_COEFS *ac,
                        float **realBuf,
                        float **imagBuf,
                        int bd,
                        int len) {
  int j, jminus1, jminus2;
  SBR_DOUBLE rel = 1.0 / (1.0 + RELAXATION);

  memset(ac, 0, sizeof(ACORR_COEFS));

  for (j = 0; j < len - 1; j++) {
    jminus1 = j - 1;
    jminus2 = jminus1 - 1;

    ac->r00r += realBuf[j][bd] * realBuf[j][bd] +
                imagBuf[j][bd] * imagBuf[j][bd];

    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
                imagBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
                realBuf[j][bd] * imagBuf[jminus1][bd];

    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
                imagBuf[j][bd] * imagBuf[jminus2][bd];

    ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
                realBuf[j][bd] * imagBuf[jminus2][bd];
  }

  ac->r22r = ac->r11r + realBuf[-2][bd] * realBuf[-2][bd] +
             imagBuf[-2][bd] * imagBuf[-2][bd];

  ac->r12r = ac->r01r + realBuf[-1][bd] * realBuf[-2][bd] +
             imagBuf[-1][bd] * imagBuf[-2][bd];

  ac->r12i = ac->r01i + imagBuf[-1][bd] * realBuf[-2][bd] -
             realBuf[-1][bd] * imagBuf[-2][bd];

  jminus1 = j - 1;
  jminus2 = jminus1 - 1;

  ac->r00r += realBuf[j][bd] * realBuf[j][bd] +
              imagBuf[j][bd] * imagBuf[j][bd];

  ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
              imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

  ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
              imagBuf[j][bd] * imagBuf[jminus1][bd];

  ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
              realBuf[j][bd] * imagBuf[jminus1][bd];

  ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
              imagBuf[j][bd] * imagBuf[jminus2][bd];

  ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
              realBuf[j][bd] * imagBuf[jminus2][bd];

  ac->det = ac->r11r * ac->r22r - rel * (ac->r12r * ac->r12r + ac->r12i * ac->r12i);

#ifndef _NOT_AVOID_FLOAT_DENORMALS
  if (fabs(ac->det) < 1.e-30f) ac->det = 0.f;
  if (fabs(ac->r00r) < 1.e-30f) ac->r00r = 0.f;
  if (fabs(ac->r11r) < 1.e-30f) ac->r11r = 0.f;
  if (fabs(ac->r01r) < 1.e-30f) ac->r01r = 0.f;
  if (fabs(ac->r01i) < 1.e-30f) ac->r01i = 0.f;
  if (fabs(ac->r02r) < 1.e-30f) ac->r02r = 0.f;
  if (fabs(ac->r02i) < 1.e-30f) ac->r02i = 0.f;
  if (fabs(ac->r12r) < 1.e-30f) ac->r12r = 0.f;
  if (fabs(ac->r12i) < 1.e-30f) ac->r12i = 0.f;
  if (fabs(ac->r22r) < 1.e-30f) ac->r22r = 0.f;
#endif
}

static void
solveNormalEquation(ACORR_COEFS *ac,
                    SBR_DOUBLE alphar[2],
                    SBR_DOUBLE alphai[2]) {
  SBR_DOUBLE fac;

  if (ac->det == 0.0) {
    alphar[1] = alphai[1] = 0;
  } else {
    fac = 1.0 / ac->det;
    alphar[1] = (ac->r01r * ac->r12r - ac->r01i * ac->r12i - ac->r02r * ac->r11r) * fac;
    alphai[1] = (ac->r01i * ac->r12r + ac->r01r * ac->r12i - ac->r02i * ac->r11r) * fac;
  }

  if (ac->r11r == 0.0) {
    alphar[0] = alphai[0] = 0;
  } else {
    fac = 1.0 / ac->r11r;
    alphar[0] = -(ac->r01r + alphar[1] * ac->r12r + alphai[1] * ac->r12i) * fac;
    alphai[0] = -(ac->r01i + alphai[1] * ac->r12r - alphar[1] * ac->r12i) * fac;
  }
}

HANDLE_ERROR_INFO
CalculateTonalityQuotas(LPC_PARAM *hLpcParam,
                        float **sourceBufferReal,
                        float **sourceBufferImag,
                        CODEC_TYPE coreCodec) {
  int k, band;
  SBR_DOUBLE alphar[2], alphai[2];
  ACORR_COEFS ac;
  HANDLE_ERROR_INFO error = noError;
  int nBands = hLpcParam->stopBand;
  int startIndex = hLpcParam->startIndexMatrix;
  int noEstPerFrame = hLpcParam->numberOfEstimatesPerFrame;
  int noQmfChannels = hLpcParam->noQmfChannels;
  int *stepSize = hLpcParam->stepSize;
  int *blockLength = hLpcParam->blockLength;
  int firstSample = hLpcParam->firstSample;
  float *nrgVector = hLpcParam->nrgVector;
  float *pNrgVectorFreq = hLpcParam->pNrgVectorFreq;
  float **quotaMatrix = hLpcParam->quotaMatrix;
  int **signMatrix = hLpcParam->signMatrix;

  SBR_DOUBLE RELAXATION_ENERGY = 1.0;

  if (noQmfChannels < nBands) {
    error = iisUtil_ERROR(CDI, "number of SBR bands is exceeding number of QMF channels, please adjust sbr start and stop frequency tuning");
  }

  if (error == noError) {
    switch (coreCodec) {
      case CODEC_SAAC:
        RELAXATION_ENERGY = 1.0 * NORM_SBR_PCM_LEVEL_SQ;
        break;

      default:
        RELAXATION_ENERGY = 1.0;
        break;
    }

    for (k = 0; k < startIndex; k++) {
      memcpy(quotaMatrix[k],
             quotaMatrix[k + noEstPerFrame],
             noQmfChannels * sizeof(float));
      memcpy(signMatrix[k],
             signMatrix[k + noEstPerFrame],
             noQmfChannels * sizeof(int));
    }

    memmove(nrgVector, nrgVector + noEstPerFrame, startIndex * sizeof(float));
    memset(nrgVector + startIndex, 0, noEstPerFrame * sizeof(float));
    memset(pNrgVectorFreq, 0, noQmfChannels * sizeof(float));
  }

  if (error == noError) {
    for (k = 0; k < noEstPerFrame; k++) {
      int _stepSize = 0;
      int _blockLength = 0;

      if (error == noError) {
        for (band = 0; band < nBands; band++) {
          switch (coreCodec) {
            case CODEC_SAAC:
              _stepSize = stepSize[0];
              _blockLength = blockLength[0];
              break;
            default:
              error = iisUtil_ERROR(CDI, "Not defined behavior for this coreCodec");
              break;
          }

          if (error == noError) {
            calcAutoCorrSecondOrder(&ac,
                                    sourceBufferReal + firstSample + k * _stepSize,
                                    sourceBufferImag + firstSample + k * _stepSize,
                                    band,
                                    _blockLength);

            solveNormalEquation(&ac, alphar, alphai);

            if (ac.r00r != 0.0) {
              SBR_DOUBLE tmp = -(alphar[0] * ac.r01r + alphai[0] * ac.r01i +
                                 alphar[1] * ac.r02r + alphai[1] * ac.r02i) /
                               (ac.r00r + RELAXATION_ENERGY);

              quotaMatrix[startIndex + k][band] = (float)(tmp / (1 - tmp + RELAXATION));
              if (ac.r11r != 0.0) {
                tmp = (ac.r01r / ac.r11r);
                if (tmp > 1.0f) tmp = 1.0f;
                if (tmp < -1.0f) tmp = -1.0f;
              } else {
                tmp = 1.0f;
              }
              if (tmp < 0)
                signMatrix[startIndex + k][band] = 1 - 2 * (band & 0x1);
              else
                signMatrix[startIndex + k][band] = 1 - 2 * ((band + 1) & 0x1);
            } else {
              quotaMatrix[startIndex + k][band] = 0;
              signMatrix[startIndex + k][band] = 1;
            }

            nrgVector[startIndex + k] += (float)ac.r00r;
            pNrgVectorFreq[band] += (float)ac.r00r;
          }
        }
      }
    }
  }

  if (error == noError) {
    for (band = 0; band < nBands; band++) {
      pNrgVectorFreq[band] /= noEstPerFrame;
    }
  }
  return error;
}

void TonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                      INVF_MODE *infVec,
                      float *noiseLevels,
                      int *missingHarmonicFlag,
                      int *missingHarmonicsIndex,
                      int *envelopeCompensation,
                      const SBR_FRAME_INFO *frameInfo,
                      const int *transientInfo,
                      const int *freqBandTable,
                      int nSfb,
                      CODEC_TYPE coreCodec,
                      XPOS_MODE xposType,
                      const int sbrPatchingMode,
                      const int bPitchDetected,
                      const int bSbr41,
                      float noiseLevelLoweringFactor[MAX_NOISE_ENVELOPES]) {
  int transientFlag = transientInfo[1];
  int transientPos = transientInfo[0];
  int transientFrame, transientFrameInvfEst;
  INVF_MODE *infVecPtr;

  transientFrame = 0;
  if (hTonCorr->transientNextFrame) {
    transientFrame = 1;
    hTonCorr->transientNextFrame = 0;

    if (transientFlag) {
      if (transientPos + hTonCorr->transientPosOffset >= frameInfo->borders[frameInfo->nEnvelopes]) {
        hTonCorr->transientNextFrame = 1;
      }
    }
  } else {
    if (transientFlag) {
      if (transientPos + hTonCorr->transientPosOffset < frameInfo->borders[frameInfo->nEnvelopes]) {
        transientFrame = 1;
        hTonCorr->transientNextFrame = 0;
      } else {
        hTonCorr->transientNextFrame = 1;
      }
    }
  }

  transientFrameInvfEst = transientFrame;

  if (hTonCorr->switchInverseFilt)
    QmfInverseFilteringDetector(hTonCorr->hSbrInvFilt,
                                hTonCorr->lpcParams.quotaMatrix,
                                hTonCorr->lpcParams.nrgVector,
                                hTonCorr->lpcParamsPatch.quotaMatrix,
                                hTonCorr->indexVector,
                                hTonCorr->frameStartIndexInvfEst,
                                hTonCorr->lpcParams.numberOfEstimatesPerFrame + hTonCorr->frameStartIndexInvfEst,
                                transientFrameInvfEst,
                                infVec,
                                coreCodec);

  switch (coreCodec) {
    case CODEC_SAAC:

      if (xposType == XPOS_LC && hTonCorr->h_sbrMissingHarmonicsDetector) {
        SbrMissingHarmonicsDetectorQmf(hTonCorr->h_sbrMissingHarmonicsDetector,
                                       (const float *const *const)hTonCorr->lpcParams.quotaMatrix,
                                       (const int *const *const)hTonCorr->lpcParams.signMatrix,
                                       (const float *const *const)hTonCorr->lpcParamsPatch.quotaMatrix,
                                       hTonCorr->indexVector,
                                       frameInfo,
                                       transientInfo,
                                       missingHarmonicFlag,
                                       missingHarmonicsIndex,
                                       freqBandTable,
                                       nSfb,
                                       envelopeCompensation,
                                       hTonCorr->lpcParams.pNrgVectorFreq,
                                       bPitchDetected,
                                       coreCodec);
      } else {
        *missingHarmonicFlag = 0;
        memset(missingHarmonicsIndex, 0, nSfb * sizeof(int));
      }
      break;
    default:
      assert(0);
  }

  infVecPtr = infVec;

  SbrNoiseFloorEstimateQmf(hTonCorr->h_sbrNoiseFloorEstimate,
                           frameInfo,
                           noiseLevels,
                           hTonCorr->lpcParams.quotaMatrix,
                           hTonCorr->lpcParamsPatch.quotaMatrix,
                           hTonCorr->indexVector,
                           *missingHarmonicFlag,
                           hTonCorr->frameStartIndex,
                           hTonCorr->lpcParams.numberOfEstimatesPerFrame,
                           hTonCorr->lpcParams.numberOfEstimates,
                           transientFrame,
                           infVecPtr,
                           coreCodec,
                           (sbrPatchingMode || bPitchDetected),
                           bSbr41,
                           noiseLevelLoweringFactor);
}

static int
findClosestEntry(int goalSb,
                 const int *v_k_master,
                 int numMaster,
                 int direction) {
  int index;

  if (goalSb <= v_k_master[0])
    return v_k_master[0];

  if (goalSb >= v_k_master[numMaster])
    return v_k_master[numMaster];

  if (direction) {
    index = 0;
    while (v_k_master[index] < goalSb) {
      index++;
    }
  } else {
    index = numMaster;
    while (v_k_master[index] > goalSb) {
      index--;
    }
  }

  return v_k_master[index];
}

static HANDLE_ERROR_INFO
resetPatch(HANDLE_SBR_TON_CORR_EST hTonCorr,
           int xposctrl,
           int highBandStartSb,
           const int *v_k_master,
           int numMaster,
           float qmfBW,
           CODEC_TYPE coreCodec) {
  int patch, k, i;
  int targetStopBand;

  PATCH_PARAM *patchParam = hTonCorr->patchParam;

  int sbGuard = hTonCorr->guard;
  int sourceStartBand;
  int patchDistance;
  int numBandsInPatch;

  int lsb = v_k_master[0];
  int usb = v_k_master[numMaster];
  int xoverOffset = highBandStartSb - v_k_master[0];

  int goalSb;

  if (xposctrl == 1) {
    lsb += xoverOffset;
    xoverOffset = 0;
  }

  goalSb = (int)(16000.0f / qmfBW + 0.5f);
  goalSb = findClosestEntry(goalSb, v_k_master, numMaster, 1);

  if (abs(usb - goalSb) < 4) {
    goalSb = usb;
  }

  sourceStartBand = hTonCorr->shiftStartSb + xoverOffset;
  targetStopBand = lsb + xoverOffset;

  patch = 0;
  while (targetStopBand < usb) {
    if (patch + 1 > MAX_NUM_PATCHES)
      return iisUtil_ERROR(CDI, "Number of patches to high");

    patchParam[patch].guardStartBand = targetStopBand;
    targetStopBand += sbGuard;
    patchParam[patch].targetStartBand = targetStopBand;

    numBandsInPatch = goalSb - targetStopBand;

    if (numBandsInPatch >= lsb - sourceStartBand) {
      patchDistance = targetStopBand - sourceStartBand;
      patchDistance = patchDistance & ~1;
      numBandsInPatch = lsb - (targetStopBand - patchDistance);

      numBandsInPatch = findClosestEntry(targetStopBand + numBandsInPatch,
                                         v_k_master, numMaster, 0) -
                        targetStopBand;
    }

    patchDistance = targetStopBand + numBandsInPatch - lsb;

    patchDistance = (patchDistance + 1) & ~1;

    if (numBandsInPatch <= 0) {
      patch--;
    } else {
      patchParam[patch].sourceStartBand = targetStopBand - patchDistance;
      patchParam[patch].targetBandOffs = patchDistance;
      patchParam[patch].numBandsInPatch = numBandsInPatch;
      patchParam[patch].sourceStopBand = patchParam[patch].sourceStartBand + numBandsInPatch;

      targetStopBand += patchParam[patch].numBandsInPatch;
    }

    sourceStartBand = hTonCorr->shiftStartSb;

    if (abs(targetStopBand - goalSb) < 3) {
      goalSb = usb;
    }

    patch++;
  }

  patch--;

  if (patchParam[patch].numBandsInPatch < 3 && patch > 0) {
    patch--;
    targetStopBand = patchParam[patch].targetStartBand + patchParam[patch].numBandsInPatch;
  }

  hTonCorr->noOfPatches = patch + 1;

  hTonCorr->indexVector = hTonCorr->indexVectorDef;

  if (hTonCorr->noOfPatches > MAX_NUM_PATCHES)
    return iisUtil_ERROR(CDI, "Number of patches too high");

  for (k = 0; k < hTonCorr->patchParam[0].guardStartBand; k++)
    hTonCorr->indexVectorDef[k] = k;

  for (i = 0; i < hTonCorr->noOfPatches; i++) {
    int sourceStart = hTonCorr->patchParam[i].sourceStartBand;
    int targetStart = hTonCorr->patchParam[i].targetStartBand;
    int numberOfBands = hTonCorr->patchParam[i].numBandsInPatch;
    int startGuardBand = hTonCorr->patchParam[i].guardStartBand;

    for (k = 0; k < (targetStart - startGuardBand); k++)
      hTonCorr->indexVectorDef[startGuardBand + k] = -1;

    for (k = 0; k < numberOfBands; k++)
      hTonCorr->indexVectorDef[targetStart + k] = sourceStart + k;
  }

  if (coreCodec == CODEC_SAAC) {
    int band = hTonCorr->patchParam[0].targetStartBand;
    for (k = hTonCorr->patchParam[0].targetStartBand; k < targetStopBand; k++) {
      hTonCorr->indexVectorPV[k] = band++;
    }
    hTonCorr->indexVector = hTonCorr->indexVectorPV;
  }

  return noError;
}

HANDLE_ERROR_INFO
CreateTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST *hTonCorr,
                       int frameSize,
                       int timeSlots,
                       int nCols,
                       int encDelay,
                       int fs,
                       int noQmfChannels,
                       int xposCtrl,
                       int highBandStartSb,
                       const int *v_k_master,
                       int numMaster,
                       int ana_max_level,
                       const int *const *const freqBandTable,
                       const int *nSfb,
                       int noiseBands,
                       int *noiseFloorOffset,
                       unsigned int useMissHarmonicsDet,
                       unsigned int useSpeechConfig,
                       CODEC_TYPE coreCoder) {
  int i;

  HANDLE_SBR_TON_CORR_EST hs;
  HANDLE_ERROR_INFO err;

  static int const lpcOrder = 2;

  *hTonCorr = NULL;

  hs = (HANDLE_SBR_TON_CORR_EST)iisCalloc(1, sizeof(SBR_TON_CORR_EST));

  if (hs == NULL)
    return iisUtil_ERROR(CDI, "Memory allocation in function CreateTonCorrParamExtr() failed");

  hs->lpcParams.numberOfEstimates = 0;

  switch (coreCoder) {
    case CODEC_SAAC:
      hs->lpcParams.blockLength[0] = 16 - lpcOrder;
      hs->lpcParams.blockLength[1] = 0xDDDD;
      hs->lpcParams.numberOfEstimatesPerFrame = nCols / 16;

      hs->frameStartIndexInvfEst = 0;

      hs->lpcParams.stepSize[0] = hs->lpcParams.blockLength[0] + lpcOrder;
      hs->lpcParams.stepSize[1] = 0xDDDD;

      hs->transientPosOffset = FRAME_MIDDLE_SLOT_2048;

      break;

    default:
      return iisUtil_ERROR(CDI, "unknown codec type");
  }

  if (!hs->lpcParams.numberOfEstimates)
    hs->lpcParams.numberOfEstimates = (int)((encDelay / (float)frameSize + 1) * (float)hs->lpcParams.numberOfEstimatesPerFrame);

  hs->lpcParams.firstSample = lpcOrder;

  hs->frameStartIndex = 0;

  hs->lpcParams.startIndexMatrix = hs->lpcParams.numberOfEstimates - hs->lpcParams.numberOfEstimatesPerFrame;

  hs->prevTransientFlag = 0;
  hs->transientNextFrame = 0;

  hs->lpcParams.noQmfChannels = noQmfChannels;

  hs->lpcParams.stopBand = freqBandTable[FREQ_RES_HIGH][nSfb[FREQ_RES_HIGH]];

  hs->lpcParams.pNrgVectorFreq = (float *)iisCalloc(noQmfChannels, sizeof(float));
  hs->lpcParams.nrgVector = (float *)iisCalloc(hs->lpcParams.numberOfEstimates, sizeof(float));
  hs->lpcParams.quotaMatrix = (float **)iisCalloc(hs->lpcParams.numberOfEstimates, sizeof(float *));
  hs->lpcParams.signMatrix = (int **)iisCalloc(hs->lpcParams.numberOfEstimates, sizeof(int *));

  if (hs->lpcParams.nrgVector == NULL || hs->lpcParams.quotaMatrix == NULL ||
      hs->lpcParams.signMatrix == NULL || hs->lpcParams.pNrgVectorFreq == NULL) {
    DeleteTonCorrParamExtr(hs, coreCoder);
    return iisUtil_ERROR(CDI, "Memory allocation in function createTonCorrParamExtr() failed");
  }

  for (i = 0; i < hs->lpcParams.numberOfEstimates; i++) {
    hs->lpcParams.quotaMatrix[i] = (float *)iisCalloc(noQmfChannels, sizeof(float));
    hs->lpcParams.signMatrix[i] = (int *)iisCalloc(noQmfChannels, sizeof(int));
    if (hs->lpcParams.quotaMatrix[i] == NULL || hs->lpcParams.signMatrix[i] == NULL) {
      DeleteTonCorrParamExtr(hs, coreCoder);
      return iisUtil_ERROR(CDI, "Memory allocation in function createTonCorrParamExtr() failed");
    }
  }

  switch (coreCoder) {
    case CODEC_SAAC:
      hs->lpcParamsPatch.blockLength[0] = 16 - lpcOrder;
      hs->lpcParamsPatch.blockLength[1] = 0xDDDD;
      hs->lpcParamsPatch.numberOfEstimatesPerFrame = nCols / 16;
      hs->frameStartIndexInvfEst = 0;

      hs->lpcParamsPatch.stepSize[0] = hs->lpcParams.blockLength[0] + lpcOrder;
      hs->lpcParamsPatch.stepSize[1] = 0xDDDD;
      hs->transientPosOffset = FRAME_MIDDLE_SLOT_2048;

      break;
    default:
      return iisUtil_ERROR(CDI, "unknown codec type");
  }

  if (coreCoder == CODEC_SAAC) {
    hs->lpcParamsPatch.numberOfEstimates = (int)((encDelay / (float)frameSize + 1) * (float)hs->lpcParamsPatch.numberOfEstimatesPerFrame);

    hs->lpcParamsPatch.firstSample = lpcOrder;

    hs->frameStartIndex = 0;

    hs->lpcParamsPatch.startIndexMatrix = hs->lpcParamsPatch.numberOfEstimates - hs->lpcParamsPatch.numberOfEstimatesPerFrame;

    hs->prevTransientFlag = 0;
    hs->transientNextFrame = 0;

    hs->lpcParamsPatch.noQmfChannels = noQmfChannels;

    hs->lpcParamsPatch.stopBand = freqBandTable[FREQ_RES_HIGH][nSfb[FREQ_RES_HIGH]];

    hs->lpcParamsPatch.pNrgVectorFreq = (float *)iisCalloc(noQmfChannels, sizeof(float));
    hs->lpcParamsPatch.nrgVector = (float *)iisCalloc(hs->lpcParamsPatch.numberOfEstimates, sizeof(float));
    hs->lpcParamsPatch.quotaMatrix = (float **)iisCalloc(hs->lpcParamsPatch.numberOfEstimates, sizeof(float *));
    hs->lpcParamsPatch.signMatrix = (int **)iisCalloc(hs->lpcParamsPatch.numberOfEstimates, sizeof(int *));

    if (hs->lpcParamsPatch.nrgVector == NULL || hs->lpcParamsPatch.quotaMatrix == NULL ||
        hs->lpcParamsPatch.signMatrix == NULL || hs->lpcParamsPatch.pNrgVectorFreq == NULL) {
      DeleteTonCorrParamExtr(hs, coreCoder);
      return iisUtil_ERROR(CDI, "Memory allocation in function createTonCorrParamExtr() failed");
    }

    for (i = 0; i < hs->lpcParamsPatch.numberOfEstimates; i++) {
      hs->lpcParamsPatch.quotaMatrix[i] = (float *)iisCalloc(noQmfChannels, sizeof(float));
      hs->lpcParamsPatch.signMatrix[i] = (int *)iisCalloc(noQmfChannels, sizeof(int));
      if (hs->lpcParamsPatch.quotaMatrix[i] == NULL || hs->lpcParamsPatch.signMatrix[i] == NULL) {
        DeleteTonCorrParamExtr(hs, coreCoder);
        return iisUtil_ERROR(CDI, "Memory allocation in function createTonCorrParamExtr() failed");
      }
    }
  }

  hs->guard = 0;
  hs->shiftStartSb = 1;

  err = resetPatch(hs,
                   xposCtrl,
                   highBandStartSb,
                   v_k_master,
                   numMaster,
                   fs / (2.0f * noQmfChannels),
                   coreCoder);

  if (err != noError)
    return handBack(err);

  err = CreateSbrNoiseFloorEstimate(&hs->h_sbrNoiseFloorEstimate,
                                    ana_max_level,
                                    freqBandTable[FREQ_RES_LOW],
                                    nSfb[FREQ_RES_LOW],
                                    noiseBands,
                                    noiseFloorOffset,
                                    timeSlots,
                                    useSpeechConfig);

  if (err != noError)
    return handBack(err);

  err = CreateInvFiltDetector(&hs->hSbrInvFilt,
                              coreCoder,
                              hs->h_sbrNoiseFloorEstimate->freqBandTableQmf,
                              hs->h_sbrNoiseFloorEstimate->noNoiseBands,
                              useSpeechConfig);

  if (err != noError)
    return handBack(err);

  switch (coreCoder) {
    case CODEC_SAAC:
      if (useMissHarmonicsDet) {
        err = CreateSbrMissingHarmonicsDetector(&hs->h_sbrMissingHarmonicsDetector,
                                                frameSize,
                                                fs,
                                                nSfb[FREQ_RES_HIGH],
                                                hs->lpcParams.numberOfEstimates,
                                                hs->lpcParams.startIndexMatrix,
                                                hs->lpcParams.numberOfEstimatesPerFrame,
                                                timeSlots,
                                                coreCoder);

        if (err != noError)
          return handBack(err);
      } else {
        hs->h_sbrMissingHarmonicsDetector = NULL;
      }
      break;
    default:
      assert(0);
  }

  *hTonCorr = hs;
  return noError;
}

HANDLE_ERROR_INFO
ResetTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                      int xposctrl,
                      int highBandStartSb,
                      const int *v_k_master,
                      int numMaster,
                      int fs,
                      const int *const *const freqBandTable,
                      const int *nSfb,
                      int noQmfChannels,
                      CODEC_TYPE coreCodec) {
  HANDLE_ERROR_INFO err;

  hTonCorr->guard = 0;
  hTonCorr->shiftStartSb = 1;

  err = resetPatch(hTonCorr,
                   xposctrl,
                   highBandStartSb,
                   v_k_master,
                   numMaster,
                   fs / (2.0f * noQmfChannels),
                   coreCodec);

  if (err != noError)
    return handBack(err);

  hTonCorr->lpcParams.stopBand = freqBandTable[FREQ_RES_HIGH][nSfb[FREQ_RES_HIGH]];

  err = ResetSbrNoiseFloorEstimate(hTonCorr->h_sbrNoiseFloorEstimate,
                                   freqBandTable[FREQ_RES_LOW],
                                   nSfb[FREQ_RES_LOW]);

  if (err != noError)
    return handBack(err);

  err = ResetInvFiltDetector(hTonCorr->hSbrInvFilt,
                             hTonCorr->h_sbrNoiseFloorEstimate->freqBandTableQmf,
                             hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands,
                             coreCodec);

  if (err != noError)
    return handBack(err);

  switch (coreCodec) {
    case CODEC_SAAC:

      if (hTonCorr->h_sbrMissingHarmonicsDetector) {
        err = ResetSbrMissingHarmonicsDetector(hTonCorr->h_sbrMissingHarmonicsDetector,
                                               nSfb[FREQ_RES_HIGH]);
        if (err != noError)
          return handBack(err);
      }
      break;
    default:
      assert(0);
  }

  return noError;
}

void DeleteTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                            CODEC_TYPE coreCodec) {
  int i;

  if (hTonCorr) {
    if (hTonCorr->lpcParams.quotaMatrix) {
      for (i = 0; i < hTonCorr->lpcParams.numberOfEstimates; i++) {
        iisFree(hTonCorr->lpcParams.quotaMatrix[i]);
      }
      iisFree(hTonCorr->lpcParams.quotaMatrix);
    }

    if (hTonCorr->lpcParams.signMatrix) {
      for (i = 0; i < hTonCorr->lpcParams.numberOfEstimates; i++) {
        iisFree(hTonCorr->lpcParams.signMatrix[i]);
      }
      iisFree(hTonCorr->lpcParams.signMatrix);
    }

    if (hTonCorr->lpcParams.nrgVector)
      iisFree(hTonCorr->lpcParams.nrgVector);
    if (hTonCorr->lpcParams.pNrgVectorFreq)
      iisFree(hTonCorr->lpcParams.pNrgVectorFreq);

    if (hTonCorr->lpcParamsPatch.quotaMatrix) {
      for (i = 0; i < hTonCorr->lpcParamsPatch.numberOfEstimates; i++)
        iisFree(hTonCorr->lpcParamsPatch.quotaMatrix[i]);
      iisFree(hTonCorr->lpcParamsPatch.quotaMatrix);
    }

    if (hTonCorr->lpcParamsPatch.signMatrix) {
      for (i = 0; i < hTonCorr->lpcParamsPatch.numberOfEstimates; i++) {
        iisFree(hTonCorr->lpcParamsPatch.signMatrix[i]);
      }
      iisFree(hTonCorr->lpcParamsPatch.signMatrix);
    }

    if (hTonCorr->lpcParamsPatch.nrgVector)
      iisFree(hTonCorr->lpcParamsPatch.nrgVector);
    if (hTonCorr->lpcParamsPatch.pNrgVectorFreq)
      iisFree(hTonCorr->lpcParamsPatch.pNrgVectorFreq);

    if (hTonCorr->h_sbrNoiseFloorEstimate)
      DeleteSbrNoiseFloorEstimate(hTonCorr->h_sbrNoiseFloorEstimate);

    switch (coreCodec) {
      case CODEC_SAAC:
        if (hTonCorr->h_sbrMissingHarmonicsDetector)
          DeleteSbrMissingHarmonicsDetector(hTonCorr->h_sbrMissingHarmonicsDetector);
        break;
      default:
        assert(0);
    }

    if (hTonCorr->hSbrInvFilt)
      DeleteInvFiltDetector(hTonCorr->hSbrInvFilt);

    iisFree(hTonCorr);
  }
}
