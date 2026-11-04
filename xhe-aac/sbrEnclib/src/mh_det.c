
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

#include "sbr.h"
#include "sbr_def.h"
#include "mathlib.h"
#include "mh_det.h"

#define MAX_NSFB MAX_FREQ_COEFFS

static const DETECTOR_PARAMETERS_MH paramsAac = {
    9,
    {20.0f,
     1.26f,
     15.0f,
     1.26f,
     0.3f,
     0.1f,
     0.3f,
     0.5f,
     0.995f,
     0.995f,
     0.8f},
    50};

static const DETECTOR_PARAMETERS_MH paramsAac32 = {
    9,
    {25.0f,
     1.26f,
     15.0f,
     1.26f,
     0.3f,
     0.1f,
     0.3f,
     0.5f,
     0.998f,
     0.998f,
     0.8f},
    50};

static void diff(const float* pTonalityVector,
                 const float* pTonalityVectorSbr,
                 float* pDiffVectorSfb,
                 const int* pFreqBandTable,
                 int nSfb,
                 const int* pIndexVector,
                 CODEC_TYPE coreCodec) {
  int i, ll, lu, k;
  float maxValOrig, maxValSbr;

  for (i = 0; i < nSfb; i++) {
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i + 1];

    maxValOrig = 0;
    maxValSbr = 0;

    for (k = ll; k < lu; k++) {
      if (pTonalityVector[k] > maxValOrig)
        maxValOrig = pTonalityVector[k];

      if (coreCodec == CODEC_SAAC) {
        if (pTonalityVectorSbr) {
          if (pTonalityVectorSbr[pIndexVector[k]] > maxValSbr) {
            maxValSbr = pTonalityVectorSbr[pIndexVector[k]];
          }
        }
      } else {
        if (pTonalityVector[pIndexVector[k]] > maxValSbr)
          maxValSbr = pTonalityVector[pIndexVector[k]];
      }
    }

    if (maxValSbr >= 1)
      pDiffVectorSfb[i] = maxValOrig / maxValSbr;
    else
      pDiffVectorSfb[i] = maxValOrig;
  }
}

static void calculateFlatnessMeasure(const float* pTonalityVector,
                                     const float* pTonalityVectorSbr,
                                     const int* pIndexVector,
                                     float* pTfmOrigVec,
                                     float* pTfmSbrVec,
                                     const int* pFreqBandTable,
                                     int nSfb,
                                     CODEC_TYPE coreCodec) {
  int i, j;

  for (i = 0; i < nSfb; i++) {
    int ll = pFreqBandTable[i];
    int lu = pFreqBandTable[i + 1];

    pTfmOrigVec[i] = 1;
    pTfmSbrVec[i] = 1;

    if (lu - ll > 1) {
      float amOrig, amTransp, gmOrig, gmTransp, sfmOrig, sfmTransp;

      amOrig = amTransp = 0;
      gmOrig = gmTransp = 1;
      for (j = ll; j < lu; j++) {
        sfmOrig = pTonalityVector[j];
        if (coreCodec == CODEC_SAAC) {
          sfmTransp = pTonalityVectorSbr[pIndexVector[j]];
        } else {
          sfmTransp = pTonalityVector[pIndexVector[j]];
        }
        amOrig += sfmOrig;
        gmOrig *= sfmOrig;
        amTransp += sfmTransp;
        gmTransp *= sfmTransp;

#ifndef _NOT_AVOID_FLOAT_DENORMALS

        if (gmOrig < 1.e-17f) {
          gmOrig = 0.f;
        }
        if (gmTransp < 1.e-17f) {
          gmTransp = 0.f;
        }

#endif
      }
      amOrig /= lu - ll;
      amTransp /= lu - ll;
      gmOrig = (float)pow(gmOrig, 1.0 / (lu - ll));
      gmTransp = (float)pow(gmTransp, 1.0 / (lu - ll));

      if (amOrig != 0.0f)
        pTfmOrigVec[i] = gmOrig / amOrig;

      if (amTransp != 0.0f)
        pTfmSbrVec[i] = gmTransp / amTransp;
    }
  }
}

static void calculateDetectorInput(const float* const* const ppTonalityMatrix,
                                   const float* const* const ppTonalityMatrixPatch,
                                   const int* pIndexVector,
                                   float** ppDiffMatrixSfb,
                                   float** ppTfmOrigMatrix,
                                   float** ppTfmSbrMatrix,
                                   const int* pFreqBandTable,
                                   int nSfb,
                                   int noEstPerFrame,
                                   int move,
                                   CODEC_TYPE coreCodec) {
  int est;

  for (est = 0; est < move; est++) {
    memcpy(ppDiffMatrixSfb[est],
           ppDiffMatrixSfb[est + noEstPerFrame],
           nSfb * sizeof(float));

    memcpy(ppTfmOrigMatrix[est],
           ppTfmOrigMatrix[est + noEstPerFrame],
           nSfb * sizeof(float));

    memcpy(ppTfmSbrMatrix[est],
           ppTfmSbrMatrix[est + noEstPerFrame],
           nSfb * sizeof(float));
  }

  for (est = 0; est < noEstPerFrame; est++) {
    diff(ppTonalityMatrix[est + move],
         (ppTonalityMatrixPatch != NULL) ? ppTonalityMatrixPatch[est + move] : 0,
         ppDiffMatrixSfb[est + move],
         pFreqBandTable,
         nSfb,
         pIndexVector,
         coreCodec);

    calculateFlatnessMeasure(ppTonalityMatrix[est + move],
                             (ppTonalityMatrixPatch) ? ppTonalityMatrixPatch[est + move] : NULL,
                             pIndexVector,
                             ppTfmOrigMatrix[est + move],
                             ppTfmSbrMatrix[est + move],
                             pFreqBandTable,
                             nSfb,
                             coreCodec);
  }
}

static void removeLowPassDetection(int* pbAddHarmSfb,
                                   int** ppDetectionVectors,
                                   int start,
                                   int stop,
                                   int nSfb,
                                   const int* pFreqBandTable,
                                   const float* pNrgVectorFreq,
                                   THRES_HOLDS thresHolds) {
  int i, est;
  int maxDerivPos = pFreqBandTable[nSfb];
  int numBands = pFreqBandTable[nSfb];
  float nrgLow, nrgHigh;
  float maxVal, maxValAbove, val, val2;
  int bLPsignal = 0;

  maxVal = 0;
  for (i = numBands - 1 - 2; i > pFreqBandTable[0]; i--) {
    nrgLow = pNrgVectorFreq[i];
    nrgHigh = pNrgVectorFreq[i + 2];

    if (nrgLow != 0.0f) {
      val = (nrgLow - nrgHigh) / nrgLow;
      if (val > maxVal) {
        maxDerivPos = i;
        maxVal = val;
      }
      if (maxVal > thresHolds.derivThresMax)
        break;
    }
  }

  maxValAbove = 0;
  for (i = numBands - 1 - 2; i > maxDerivPos + 2; i--) {
    nrgLow = pNrgVectorFreq[i];
    nrgHigh = pNrgVectorFreq[i + 2];

    if (nrgLow != 0.0f && nrgLow > nrgHigh) {
      val = (nrgLow - nrgHigh) / nrgLow;
      if (val > maxValAbove) {
        maxValAbove = val;
      }
    } else {
      if (nrgHigh != 0.0f && nrgHigh > nrgLow) {
        val = (nrgHigh - nrgLow) / nrgHigh;
        if (val > maxValAbove) {
          maxValAbove = val;
        }
      }
    }
  }

  if (maxVal > thresHolds.derivThresMax && maxValAbove < thresHolds.derivThresAbove) {
    bLPsignal = 1;

    for (i = maxDerivPos - 1; i > maxDerivPos - 5 && i >= 0; i--) {
      if (pNrgVectorFreq[i] != 0.0f) {
        val2 = ((pNrgVectorFreq[i] - pNrgVectorFreq[maxDerivPos + 2]) / pNrgVectorFreq[i]);
        if (val2 < thresHolds.derivThresBelow) {
          bLPsignal = 0;
          break;
        }
      } else {
        bLPsignal = 0;
        break;
      }
    }
  }

  if (bLPsignal) {
    for (i = 0; i < nSfb; i++) {
      if (maxDerivPos >= pFreqBandTable[i] && maxDerivPos < pFreqBandTable[i + 1])
        break;
    }

    if (pbAddHarmSfb[i]) {
      pbAddHarmSfb[i] = 0;
      for (est = start; est < stop; est++) {
        ppDetectionVectors[est][i] = 0;
      }
    }
  }
}

static int isDetectionOfNewToneAllowed(const SBR_FRAME_INFO* pFrameInfo,
                                       int* pbPrevTransientFrame,
                                       int* pbPrevTransientPos,
                                       int* pbPrevTransientFlag,
                                       int transientPosOffset,
                                       int bTransientFlag,
                                       int transientPos,
                                       int deltaTime,
                                       int timeSlots,
                                       int* pDetectionStartPos,
                                       int noEstPerFrame) {
  int bTransientFrame;
  int bNewDetectionAllowed;

  bTransientFrame = 0;
  if (bTransientFlag) {
    if (transientPos + transientPosOffset < pFrameInfo->borders[pFrameInfo->nEnvelopes]) {
      bTransientFrame = 1;
      if (noEstPerFrame > 1) {
        if (transientPos + transientPosOffset > timeSlots / 2.0) {
          *pDetectionStartPos = noEstPerFrame;
        } else {
          *pDetectionStartPos = (int)(noEstPerFrame / 2.0);
        }

      } else {
        *pDetectionStartPos = noEstPerFrame;
      }
    }
  } else {
    if (*pbPrevTransientFlag == 1 && *pbPrevTransientFrame == 0) {
      bTransientFrame = 1;
      *pDetectionStartPos = 0;
    }
  }

  bNewDetectionAllowed = 0;
  if (bTransientFrame) {
    bNewDetectionAllowed = 1;
  } else {
    if (*pbPrevTransientFrame == 1 &&
        abs(pFrameInfo->borders[0] - (*pbPrevTransientPos + transientPosOffset - timeSlots)) < deltaTime) {
      bNewDetectionAllowed = 1;
      *pDetectionStartPos = 0;
    }
  }

  *pbPrevTransientFlag = bTransientFlag;
  *pbPrevTransientFrame = bTransientFrame;
  *pbPrevTransientPos = transientPos;

  return (bNewDetectionAllowed);
}

static void detectionCleanUp(const float* const* const ppTonalityMatrix,
                             const int* const* const ppSignMatrix,
                             int nSfb,
                             int** ppDetectionVectors,
                             const int* pFreqBandTable,
                             int bNewDetectionAllowed,
                             int* pbAddHarmSfb,
                             const int* pbPrevAddHarmSfb,
                             int start,
                             int stop,
                             const float* pNrgVectorFreq,
                             THRES_HOLDS thresHolds) {
  int i, j, li, ui, est;

  for (est = start; est < stop; est++) {
    for (i = 0; i < nSfb; i++) {
      pbAddHarmSfb[i] = pbAddHarmSfb[i] || ppDetectionVectors[est][i];
    }
  }

  if (bNewDetectionAllowed == 1) {
    for (i = 0; i < nSfb - 1; i++) {
      if (pbAddHarmSfb[i] && pbAddHarmSfb[i + 1]) {
        float maxVal1, maxVal2;
        int maxPos1, maxPos2, maxPosTime1, maxPosTime2;

        li = pFreqBandTable[i];
        ui = pFreqBandTable[i + 1];

        maxPosTime1 = start;
        maxPos1 = li;
        maxVal1 = ppTonalityMatrix[start][li];
        for (est = start; est < stop; est++) {
          for (j = li; j < ui; j++) {
            if (ppTonalityMatrix[est][j] > maxVal1) {
              maxVal1 = ppTonalityMatrix[est][j];
              maxPos1 = j;
              maxPosTime1 = est;
            }
          }
        }

        li = pFreqBandTable[i + 1];
        ui = pFreqBandTable[i + 2];

        maxPosTime2 = start;
        maxPos2 = li;
        maxVal2 = ppTonalityMatrix[start][li];
        for (est = start; est < stop; est++) {
          for (j = li; j < ui; j++) {
            if (ppTonalityMatrix[est][j] > maxVal2) {
              maxVal2 = ppTonalityMatrix[est][j];
              maxPos2 = j;
              maxPosTime2 = est;
            }
          }
        }

        if (maxPos2 - maxPos1 < 2) {
          if (pbPrevAddHarmSfb[i] == 1 && pbPrevAddHarmSfb[i + 1] == 0) {
            pbAddHarmSfb[i + 1] = 0;
            for (est = start; est < stop; est++) {
              ppDetectionVectors[est][i + 1] = 0;
            }
          } else {
            if (pbPrevAddHarmSfb[i] == 0 && pbPrevAddHarmSfb[i + 1] == 1) {
              pbAddHarmSfb[i] = 0;
              for (est = start; est < stop; est++) {
                ppDetectionVectors[est][i] = 0;
              }
            } else {
              if (maxVal1 > maxVal2) {
                if (ppSignMatrix[maxPosTime1][maxPos2] < 0 && ppSignMatrix[maxPosTime1][maxPos1] > 0) {
                  pbAddHarmSfb[i + 1] = 0;
                  for (est = start; est < stop; est++) {
                    ppDetectionVectors[est][i + 1] = 0;
                  }
                }
              } else {
                if (ppSignMatrix[maxPosTime2][maxPos2] < 0 && ppSignMatrix[maxPosTime2][maxPos1] > 0) {
                  pbAddHarmSfb[i] = 0;
                  for (est = start; est < stop; est++) {
                    ppDetectionVectors[est][i] = 0;
                  }
                }
              }
            }
          }
        }
      }
    }

    removeLowPassDetection(pbAddHarmSfb,
                           ppDetectionVectors,
                           start,
                           stop,
                           nSfb,
                           pFreqBandTable,
                           pNrgVectorFreq,
                           thresHolds);
  } else {
    for (i = 0; i < nSfb; i++) {
      if (pbAddHarmSfb[i] - pbPrevAddHarmSfb[i] > 0)
        pbAddHarmSfb[i] = 0;
    }
  }
}

static void detection(const float* pTonalityVec,
                      const float* pDiffVecSfb,
                      int nSfb,
                      int* pbHarmVec,
                      const int* pFreqBandTable,
                      const float* pTfmOrigVec,
                      const float* pTfmSbrVec,
                      THRES_HOLDS thresHolds,
                      GUIDE_VECTORS guideVectors,
                      GUIDE_VECTORS newGuideVectors) {
  int i, j, ll, lu;
  float thresTemp, thresOrig;

  float tfmThresSbr = thresHolds.tfmThresSbr;
  float tfmThresOrig = thresHolds.tfmThresOrig;
  float decayGuideOrig = thresHolds.decayGuideOrig;
  float decayGuideDiff = thresHolds.decayGuideDiff;

  for (i = 0; i < nSfb; i++) {
    thresTemp = guideVectors.pGuideVectorDiff[i] != 0.0f ? max(decayGuideDiff * guideVectors.pGuideVectorDiff[i], thresHolds.thresHoldDiffGuide) : thresHolds.thresHoldDiff;

    thresTemp = (float)min(thresTemp, thresHolds.thresHoldDiff);

    if (pDiffVecSfb[i] > thresTemp) {
      pbHarmVec[i] = 1;
      newGuideVectors.pGuideVectorDiff[i] = pDiffVecSfb[i];
    } else {
      if (guideVectors.pGuideVectorDiff[i] != 0.0f) {
        guideVectors.pGuideVectorOrig[i] = thresHolds.thresHoldToneGuide;
      }
    }
  }

  for (i = 0; i < nSfb; i++) {
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i + 1];

    thresOrig = max(guideVectors.pGuideVectorOrig[i] * decayGuideOrig, thresHolds.thresHoldToneGuide);
    thresOrig = min(thresOrig, thresHolds.thresHoldTone);

    if (guideVectors.pGuideVectorOrig[i] != 0.0f) {
      for (j = ll; j < lu; j++) {
        if (pTonalityVec[j] > thresOrig) {
          pbHarmVec[i] = 1;
          newGuideVectors.pGuideVectorOrig[i] = pTonalityVec[j];
        }
      }
    }
  }

  thresOrig = thresHolds.thresHoldTone;

  for (i = 0; i < nSfb; i++) {
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i + 1];

    if (pbHarmVec[i] == 0) {
      if (lu - ll > 1) {
        for (j = ll; j < lu; j++) {
          if (pTonalityVec[j] > thresOrig && (pTfmSbrVec[i] > tfmThresSbr && pTfmOrigVec[i] < tfmThresOrig)) {
            pbHarmVec[i] = 1;
            newGuideVectors.pGuideVectorOrig[i] = pTonalityVec[j];
          }
        }
      } else {
        if (i < nSfb - 1) {
          ll = pFreqBandTable[i];

          if (i > 0) {
            if (pTonalityVec[ll] > thresHolds.thresHoldTone &&
                (pDiffVecSfb[i + 1] < 1.0f / thresHolds.thresHoldTone ||
                 pDiffVecSfb[i - 1] < 1.0f / thresHolds.thresHoldTone)) {
              pbHarmVec[i] = 1;
              newGuideVectors.pGuideVectorOrig[i] = pTonalityVec[ll];
            }
          } else {
            if (pTonalityVec[ll] > thresHolds.thresHoldTone &&
                pDiffVecSfb[i + 1] < 1.0f / thresHolds.thresHoldTone) {
              pbHarmVec[i] = 1;
              newGuideVectors.pGuideVectorOrig[i] = pTonalityVec[ll];
            }
          }
        }
      }
    }
  }
}

static void detectionWithPrediction(const float* const* ppTonalityMatrix,
                                    const float* const* ppDiffMatrixSfb,
                                    const int* const* const ppSignMatrix,
                                    int nSfb,
                                    const int* pFreqBandTable,
                                    const float* const* const ppTfmOrigMatrix,
                                    const float* const* const ppTfmSbrMatrix,
                                    int** ppDetectionVectors,
                                    THRES_HOLDS thresHolds,
                                    GUIDE_VECTORS* pGuideVectors,
                                    int totNoEst,
                                    int bNewDetectionAllowed,
                                    int* pbAddHarmSfb,
                                    int* pbAddHarmFlag,
                                    int* pbPrevAddHarmSfb,
                                    int detectionStart,
                                    const float* pNrgVectorFreq,
                                    const int bPitchDetected) {
  int est = 0;
  int start, scfBand;
  int stop = totNoEst;

  memset(pbAddHarmSfb, 0, nSfb * sizeof(int));

  if (bNewDetectionAllowed) {
    if (totNoEst > 1) {
      start = detectionStart + 1;

      memcpy(pGuideVectors[start].pGuideVectorDiff, pGuideVectors[0].pGuideVectorDiff, nSfb * sizeof(float));
      memcpy(pGuideVectors[start].pGuideVectorOrig, pGuideVectors[0].pGuideVectorOrig, nSfb * sizeof(float));
      if (start > 0)
        memset(pGuideVectors[start - 1].pbGuideVectorDetected, 0, nSfb * sizeof(int));
    } else {
      start = 0;
    }
  } else {
    start = 0;
  }

  for (est = start; est < stop; est++) {
    if (est > 0) {
      memcpy(pGuideVectors[est].pbGuideVectorDetected, ppDetectionVectors[est - 1], nSfb * sizeof(int));
    }

    memset(ppDetectionVectors[est], 0, nSfb * sizeof(int));

    memset(pGuideVectors[est + 1].pGuideVectorDiff, 0, nSfb * sizeof(float));
    memset(pGuideVectors[est + 1].pGuideVectorOrig, 0, nSfb * sizeof(float));
    memset(pGuideVectors[est + 1].pbGuideVectorDetected, 0, nSfb * sizeof(int));

    detection(ppTonalityMatrix[est],
              ppDiffMatrixSfb[est],
              nSfb,
              ppDetectionVectors[est],
              pFreqBandTable,
              ppTfmOrigMatrix[est],
              ppTfmSbrMatrix[est],
              thresHolds,
              pGuideVectors[est],
              pGuideVectors[est + 1]);
  }

  detectionCleanUp(ppTonalityMatrix,
                   ppSignMatrix,
                   nSfb,
                   ppDetectionVectors,
                   pFreqBandTable,
                   bNewDetectionAllowed,
                   pbAddHarmSfb,
                   pbPrevAddHarmSfb,
                   start,
                   stop,
                   pNrgVectorFreq,
                   thresHolds);

  *pbAddHarmFlag = 0;
  for (scfBand = 0; scfBand < nSfb; scfBand++) {
    if (bPitchDetected) {
      pbAddHarmSfb[scfBand] = 0;
    } else if (pbAddHarmSfb[scfBand]) {
      *pbAddHarmFlag = 1;
      break;
    }
  }

  memcpy(pbPrevAddHarmSfb, pbAddHarmSfb, nSfb * sizeof(int));
  memcpy(pGuideVectors[0].pbGuideVectorDetected, pbAddHarmSfb, nSfb * sizeof(int));

  for (scfBand = 0; scfBand < nSfb; scfBand++) {
    pGuideVectors[0].pGuideVectorDiff[scfBand] = 0;
    pGuideVectors[0].pGuideVectorOrig[scfBand] = 0;

    if (pbAddHarmSfb[scfBand] == 1) {
      for (est = start; est < stop; est++) {
        if (pGuideVectors[est].pGuideVectorDiff[scfBand] != 0.0f) {
          pGuideVectors[0].pGuideVectorDiff[scfBand] = pGuideVectors[est].pGuideVectorDiff[scfBand];
        }
        if (pGuideVectors[est].pGuideVectorOrig[scfBand] != 0.0f) {
          pGuideVectors[0].pGuideVectorOrig[scfBand] = pGuideVectors[est].pGuideVectorOrig[scfBand];
        }
      }
    }
  }
}

static void calculateCompVector(const int* pbAddHarmSfb,
                                const float* const* const ppTonalityMatrix,
                                const int* const* const ppSignMatrix,
                                int* pEnvComp,
                                int nSfb,
                                const int* pFreqBandTable,
                                int totNoEst,
                                int maxComp,
                                int* pPrevEnvComp,
                                int bNewDetectionAllowed) {
  int scfBand, est, l, ll, lu, maxPosF, maxPosT;
  float maxVal;
  int compValue;

  memset(pEnvComp, 0, nSfb * sizeof(int));

  for (scfBand = 0; scfBand < nSfb; scfBand++) {
    if (pbAddHarmSfb[scfBand]) {
      ll = pFreqBandTable[scfBand];
      lu = pFreqBandTable[scfBand + 1];

      maxPosF = 0;
      maxPosT = 0;
      maxVal = 0;

      for (est = 0; est < totNoEst; est++) {
        for (l = ll; l < lu; l++) {
          if (ppTonalityMatrix[est][l] > maxVal) {
            maxVal = ppTonalityMatrix[est][l];
            maxPosF = l;
            maxPosT = est;
          }
        }
      }

      if (maxPosF == ll && scfBand) {
        if (!pbAddHarmSfb[scfBand - 1]) {
          if (ppSignMatrix[maxPosT][maxPosF - 1] > 0 && ppSignMatrix[maxPosT][maxPosF] < 0) {
            compValue = (int)(fabs(ILOG2 * log(ppTonalityMatrix[maxPosT][maxPosF - 1] + EPS)) + 0.5f);

            if (compValue > maxComp)
              compValue = maxComp;

            pEnvComp[scfBand - 1] = compValue;
          }
        }
      }

      if (maxPosF == lu - 1 && scfBand + 1 < nSfb) {
        if (!pbAddHarmSfb[scfBand + 1]) {
          if (ppSignMatrix[maxPosT][maxPosF] > 0 && ppSignMatrix[maxPosT][maxPosF + 1] < 0) {
            compValue = (int)(fabs(ILOG2 * log(ppTonalityMatrix[maxPosT][maxPosF + 1] + EPS)) + 0.5f);

            if (compValue > maxComp)
              compValue = maxComp;

            pEnvComp[scfBand + 1] = compValue;
          }
        }
      }
    }
  }

  if (bNewDetectionAllowed == 0) {
    for (scfBand = 0; scfBand < nSfb; scfBand++) {
      if (pEnvComp[scfBand] != 0 && pPrevEnvComp[scfBand] == 0)
        pEnvComp[scfBand] = 0;
    }
  }

  memcpy(pPrevEnvComp, pEnvComp, nSfb * sizeof(int));
}

void SbrMissingHarmonicsDetectorQmf(HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMHDet,
                                    const float* const* const ppTonalityMatrix,
                                    const int* const* const ppSignMatrix,
                                    const float* const* const ppTonalityMatrixPatch,
                                    const int* pIndexVector,
                                    const SBR_FRAME_INFO* pFrameInfo,
                                    const int* pTranInfo,
                                    int* pbAddHarmFlag,
                                    int* pbAddHarmSfb,
                                    const int* pFreqBandTable,
                                    int nSfb,
                                    int* pEnvComp,
                                    const float* pNrgVectorFreq,
                                    const int bPitchDetected,
                                    CODEC_TYPE coreCoder) {
  int bTransientFlag = pTranInfo[1];
  int transientPos = pTranInfo[0];
  int bNewDetectionAllowed = 0;
  int transientDetStart = 0;

  THRES_HOLDS thresHolds = h_sbrMHDet->params.thresHolds;
  int maxComp = h_sbrMHDet->params.maxComp;
  int deltaTime = h_sbrMHDet->params.deltaTime;
  GUIDE_VECTORS* pGuideVectors = h_sbrMHDet->guideVectors;
  int** ppbDetectionVectors = h_sbrMHDet->ppbDetectionVectors;
  float** ppTfmSbrMatrix = h_sbrMHDet->ppTfmSbrMatrix;
  float** ppTfmOrigMatrix = h_sbrMHDet->ppTfmOrigMatrix;
  float** ppTonalityDiff = h_sbrMHDet->ppTonalityDiff;
  int* pbPrevTransientFlag = &h_sbrMHDet->bPrevTransientFlag;
  int* pbPrevTransientFrame = &h_sbrMHDet->bPrevTransientFrame;
  int* pPrevTransientPos = &h_sbrMHDet->prevTransientPos;
  int* pPrevCompVec = h_sbrMHDet->pPrevCompVec;
  int* pbPrevAddHarmSfb = h_sbrMHDet->pbPrevAddHarmSfb;
  int transientPosOffset = h_sbrMHDet->transientPosOffset;
  int timeSlots = h_sbrMHDet->timeSlots;
  int move = h_sbrMHDet->move;
  int noEstPerFrame = h_sbrMHDet->noEstPerFrame;
  int totNoEst = h_sbrMHDet->totNoEst;

  bNewDetectionAllowed = isDetectionOfNewToneAllowed(pFrameInfo,
                                                     pbPrevTransientFrame,
                                                     pPrevTransientPos,
                                                     pbPrevTransientFlag,
                                                     transientPosOffset,
                                                     bTransientFlag,
                                                     transientPos,
                                                     deltaTime,
                                                     timeSlots,
                                                     &transientDetStart,
                                                     noEstPerFrame);

  if (bPitchDetected) bNewDetectionAllowed = 0;

  calculateDetectorInput(ppTonalityMatrix,
                         ppTonalityMatrixPatch,
                         pIndexVector,
                         ppTonalityDiff,
                         ppTfmOrigMatrix,
                         ppTfmSbrMatrix,
                         pFreqBandTable,
                         nSfb,
                         noEstPerFrame,
                         move,
                         coreCoder);

  detectionWithPrediction(ppTonalityMatrix,
                          (const float* const*)ppTonalityDiff,
                          ppSignMatrix,
                          nSfb,
                          pFreqBandTable,
                          (const float* const*)ppTfmOrigMatrix,
                          (const float* const*)ppTfmSbrMatrix,
                          ppbDetectionVectors,
                          thresHolds,
                          pGuideVectors,
                          totNoEst,
                          bNewDetectionAllowed,
                          pbAddHarmSfb,
                          pbAddHarmFlag,
                          pbPrevAddHarmSfb,
                          transientDetStart,
                          pNrgVectorFreq,
                          bPitchDetected);

  calculateCompVector(pbAddHarmSfb,
                      ppTonalityMatrix,
                      ppSignMatrix,
                      pEnvComp,
                      nSfb,
                      pFreqBandTable,
                      totNoEst,
                      maxComp,
                      pPrevCompVec,
                      bNewDetectionAllowed);
}

HANDLE_ERROR_INFO
CreateSbrMissingHarmonicsDetector(HANDLE_SBR_MISSING_HARMONICS_DETECTOR* hSbrMHDet,
                                  int frameSize,
                                  int sampleFreq,
                                  int nSfb,
                                  int totNoEst,
                                  int move,
                                  int noEstPerFrame,
                                  int timeSlots,
                                  CODEC_TYPE coreCoder) {
  int i;

  HANDLE_SBR_MISSING_HARMONICS_DETECTOR hs;
  hs =
      (HANDLE_SBR_MISSING_HARMONICS_DETECTOR)iisCalloc(1, sizeof(SBR_MISSING_HARMONICS_DETECTOR));

  if (hs == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  switch (coreCoder) {
    case CODEC_SAAC:
      switch (frameSize) {
        case 4096:
        case 2048:
          if (sampleFreq <= 32000) {
            hs->params = paramsAac32;
          } else {
            hs->params = paramsAac;
          }
          hs->transientPosOffset = FRAME_MIDDLE_SLOT_2048;
          hs->timeSlots = NUMBER_TIME_SLOTS_2048;
          break;
        default:
          return iisUtil_ERROR(CDI, "Not supported frame size!");
      }
      break;
    default:
      return iisUtil_ERROR(CDI, "Not supported core coder type!");
  }
  hs->coreCoder = coreCoder;

  hs->timeSlots = timeSlots;

  hs->nSfb = nSfb;
  hs->totNoEst = totNoEst;
  hs->maxNoEst = totNoEst;
  hs->move = move;
  hs->noEstPerFrame = noEstPerFrame;

  hs->bPrevTransientFlag = 0;
  hs->bPrevTransientFrame = 0;
  hs->prevTransientPos = 0;

  hs->ppbDetectionVectors = (int**)iisCalloc(totNoEst, sizeof(int*));
  if (hs->ppbDetectionVectors == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < totNoEst; i++) {
    hs->ppbDetectionVectors[i] = (int*)iisCalloc(MAX_NSFB, sizeof(int));
    if (hs->ppbDetectionVectors[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->ppTonalityDiff = (float**)iisCalloc(totNoEst, sizeof(float*));
  if (hs->ppTonalityDiff == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < totNoEst; i++) {
    hs->ppTonalityDiff[i] = (float*)iisCalloc(MAX_NSFB, sizeof(float));
    if (hs->ppTonalityDiff[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->ppTfmOrigMatrix = (float**)iisCalloc(totNoEst, sizeof(float*));
  if (hs->ppTfmOrigMatrix == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < totNoEst; i++) {
    hs->ppTfmOrigMatrix[i] = (float*)iisCalloc(MAX_NSFB, sizeof(float));
    if (hs->ppTfmOrigMatrix[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->ppTfmSbrMatrix = (float**)iisCalloc(totNoEst, sizeof(float*));
  if (hs->ppTfmSbrMatrix == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < totNoEst; i++) {
    hs->ppTfmSbrMatrix[i] = (float*)iisCalloc(MAX_NSFB, sizeof(float));
    if (hs->ppTfmSbrMatrix[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->pPrevCompVec = (int*)iisCalloc(MAX_NSFB, sizeof(int));
  if (hs->pPrevCompVec == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  hs->pbPrevAddHarmSfb = (int*)iisCalloc(MAX_NSFB, sizeof(int));
  if (hs->pbPrevAddHarmSfb == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  hs->guideVectors = (GUIDE_VECTORS*)iisCalloc((totNoEst + 1), sizeof(GUIDE_VECTORS));
  if (hs->guideVectors == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < (totNoEst + 1); i++) {
    hs->guideVectors[i].pGuideVectorDiff = (float*)iisCalloc(MAX_NSFB, sizeof(float));
    if (hs->guideVectors[i].pGuideVectorDiff == NULL)
      return iisUtil_ERROR(CDI, "out of memory");

    hs->guideVectors[i].pGuideVectorOrig = (float*)iisCalloc(MAX_NSFB, sizeof(float));
    if (hs->guideVectors[i].pGuideVectorOrig == NULL)
      return iisUtil_ERROR(CDI, "out of memory");

    hs->guideVectors[i].pbGuideVectorDetected = (int*)iisCalloc(MAX_NSFB, sizeof(int));
    if (hs->guideVectors[i].pbGuideVectorDetected == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  *hSbrMHDet = hs;
  return noError;
}

void DeleteSbrMissingHarmonicsDetector(HANDLE_SBR_MISSING_HARMONICS_DETECTOR hSbrMHDet) {
  int i;

  if (hSbrMHDet) {
    iisFree(hSbrMHDet->pbPrevAddHarmSfb);

    for (i = 0; i < hSbrMHDet->totNoEst; i++) {
      if (hSbrMHDet->ppTfmOrigMatrix[i])
        iisFree(hSbrMHDet->ppTfmOrigMatrix[i]);
      if (hSbrMHDet->ppTfmSbrMatrix[i])
        iisFree(hSbrMHDet->ppTfmSbrMatrix[i]);
      if (hSbrMHDet->ppTonalityDiff[i])
        iisFree(hSbrMHDet->ppTonalityDiff[i]);
      if (hSbrMHDet->ppbDetectionVectors[i])
        iisFree(hSbrMHDet->ppbDetectionVectors[i]);
    }

    iisFree(hSbrMHDet->ppTfmOrigMatrix);
    iisFree(hSbrMHDet->ppTfmSbrMatrix);
    iisFree(hSbrMHDet->ppTonalityDiff);
    iisFree(hSbrMHDet->ppbDetectionVectors);
    iisFree(hSbrMHDet->pPrevCompVec);

    for (i = 0; i < hSbrMHDet->totNoEst + 1; i++) {
      iisFree(hSbrMHDet->guideVectors[i].pGuideVectorDiff);
      iisFree(hSbrMHDet->guideVectors[i].pGuideVectorOrig);
      iisFree(hSbrMHDet->guideVectors[i].pbGuideVectorDetected);
    }
    iisFree(hSbrMHDet->guideVectors);

    iisFree(hSbrMHDet);
  }
}

HANDLE_ERROR_INFO
ResetSbrMissingHarmonicsDetector(HANDLE_SBR_MISSING_HARMONICS_DETECTOR hSbrMissingHarmonicsDetector,
                                 int nSfb) {
  int i;
  float tempGuide[MAX_NSFB];
  int tempGuideInt[MAX_NSFB];
  int nSfbPrev;

  nSfbPrev = hSbrMissingHarmonicsDetector->nSfb;
  hSbrMissingHarmonicsDetector->nSfb = nSfb;

  memcpy(tempGuideInt, hSbrMissingHarmonicsDetector->pbPrevAddHarmSfb, nSfbPrev * sizeof(int));

  if (nSfb > nSfbPrev) {
    for (i = 0; i < (nSfb - nSfbPrev); i++) {
      hSbrMissingHarmonicsDetector->pbPrevAddHarmSfb[i] = 0;
    }

    for (i = 0; i < nSfbPrev; i++) {
      hSbrMissingHarmonicsDetector->pbPrevAddHarmSfb[i + (nSfb - nSfbPrev)] = tempGuideInt[i];
    }
  } else {
    for (i = 0; i < nSfb; i++) {
      hSbrMissingHarmonicsDetector->pbPrevAddHarmSfb[i] = tempGuideInt[i + (nSfbPrev - nSfb)];
    }
  }

  memcpy(tempGuide, hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorDiff, nSfbPrev * sizeof(float));

  if (nSfb > nSfbPrev) {
    for (i = 0; i < (nSfb - nSfbPrev); i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorDiff[i] = 0;
    }

    for (i = 0; i < nSfbPrev; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorDiff[i + (nSfb - nSfbPrev)] = tempGuide[i];
    }
  } else {
    for (i = 0; i < nSfb; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorDiff[i] = tempGuide[i + (nSfbPrev - nSfb)];
    }
  }

  memcpy(tempGuide, hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorOrig, nSfbPrev * sizeof(float));

  if (nSfb > nSfbPrev) {
    for (i = 0; i < (nSfb - nSfbPrev); i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorOrig[i] = 0;
    }

    for (i = 0; i < nSfbPrev; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorOrig[i + (nSfb - nSfbPrev)] = tempGuide[i];
    }
  } else {
    for (i = 0; i < nSfb; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pGuideVectorOrig[i] = tempGuide[i + (nSfbPrev - nSfb)];
    }
  }

  memcpy(tempGuideInt, hSbrMissingHarmonicsDetector->guideVectors[0].pbGuideVectorDetected, nSfbPrev * sizeof(int));

  if (nSfb > nSfbPrev) {
    for (i = 0; i < (nSfb - nSfbPrev); i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pbGuideVectorDetected[i] = 0;
    }

    for (i = 0; i < nSfbPrev; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pbGuideVectorDetected[i + (nSfb - nSfbPrev)] = tempGuideInt[i];
    }
  } else {
    for (i = 0; i < nSfb; i++) {
      hSbrMissingHarmonicsDetector->guideVectors[0].pbGuideVectorDetected[i] = tempGuideInt[i + (nSfbPrev - nSfb)];
    }
  }

  memcpy(tempGuideInt, hSbrMissingHarmonicsDetector->pPrevCompVec, nSfbPrev * sizeof(int));

  if (nSfb > nSfbPrev) {
    for (i = 0; i < (nSfb - nSfbPrev); i++) {
      hSbrMissingHarmonicsDetector->pPrevCompVec[i] = 0;
    }

    for (i = 0; i < nSfbPrev; i++) {
      hSbrMissingHarmonicsDetector->pPrevCompVec[i + (nSfb - nSfbPrev)] = tempGuideInt[i];
    }
  } else {
    for (i = 0; i < nSfb; i++) {
      hSbrMissingHarmonicsDetector->pPrevCompVec[i] = tempGuideInt[i + (nSfbPrev - nSfb)];
    }
  }
  return noError;
}
