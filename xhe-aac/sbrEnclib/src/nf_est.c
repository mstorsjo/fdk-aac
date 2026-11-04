
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
#include "mathlib.h"
#include "sbr.h"
#include "nf_est.h"

static const float smoothFilter[4] = {0.05857864376269f, 0.2f, 0.34142135623731f, 0.4f};
static const int smoothFilterLength = 4;

static void
smoothingOfNoiseLevels(float* NoiseLevels,
                       int nEnvelopes,
                       int noNoiseBands,
                       float** prevNoiseLevels,
                       int smoothLength,
                       const float* smoothingFilter,
                       int transientFlag) {
  int i, band, env;

  for (env = 0; env < nEnvelopes; env++) {
    if (transientFlag) {
      for (i = 0; i < smoothLength; i++) {
        memcpy(prevNoiseLevels[i], NoiseLevels + env * noNoiseBands, noNoiseBands * sizeof(float));
      }
    } else {
      for (i = 1; i < smoothLength; i++) {
        memcpy(prevNoiseLevels[i - 1], prevNoiseLevels[i], noNoiseBands * sizeof(float));
      }
      memcpy(prevNoiseLevels[smoothLength - 1], NoiseLevels + env * noNoiseBands, noNoiseBands * sizeof(float));
    }

    for (band = 0; band < noNoiseBands; band++) {
      NoiseLevels[band + env * noNoiseBands] = 0;
      for (i = 0; i < smoothLength; i++) {
        NoiseLevels[band + env * noNoiseBands] += smoothingFilter[i] * prevNoiseLevels[i][band];
      }
    }
  }
}

static int
findStartAndStopPositions(const SBR_FRAME_INFO* frame_info,
                          int* startPos,
                          int* stopPos,
                          int transientFlag,
                          int timeSlotsPerFrame,
                          int numberOfEstiamtesPerFrame,
                          int totalNumberOfEstimates,
                          int startIndex) {
  int nNoiseEnvelopes;
  float timeSlotsPerEstimate;

  nNoiseEnvelopes = frame_info->nNoiseEnvelopes;
  startPos[0] = frame_info->bordersNoise[0];
  stopPos[0] = frame_info->bordersNoise[1];

  if (nNoiseEnvelopes == 1) {
    startPos[1] = 0;
    stopPos[1] = 0;
  } else {
    if (transientFlag) {
      startPos[1] = stopPos[0] + 1;
      stopPos[1] = frame_info->bordersNoise[2];
    } else {
      startPos[1] = stopPos[0];
      stopPos[1] = frame_info->bordersNoise[2];
    }
  }

  timeSlotsPerEstimate = (float)timeSlotsPerFrame / numberOfEstiamtesPerFrame;

  startPos[0] = (int)(startPos[0] / timeSlotsPerEstimate);
  startPos[1] = (int)ceil(startPos[1] / timeSlotsPerEstimate);

  if (transientFlag)
    stopPos[0] = (int)(stopPos[0] / timeSlotsPerEstimate);
  else
    stopPos[0] = (int)ceil(stopPos[0] / timeSlotsPerEstimate);

  stopPos[1] = (int)ceil(stopPos[1] / timeSlotsPerEstimate);

  if (stopPos[0] - startPos[0] < 2) {
    startPos[0] = 0;
    stopPos[0] = 2;
  }

  if ((stopPos[1] - startPos[1]) < 2)
    stopPos[1] = startPos[1] + 2;

  if (stopPos[1] > (totalNumberOfEstimates - startIndex))
    stopPos[1] = (totalNumberOfEstimates - startIndex);

  return (nNoiseEnvelopes);
}

static void
qmfBasedNoiseFloorDetection(float* noiseLevel,
                            float** quotaMatrixOrig,
                            float** quotaMatrixPatch,
                            int* indexVector,
                            int startIndex,
                            int stopIndex,
                            int startChannel,
                            int stopChannel,
                            float ana_max_level,
                            float noiseFloorOffset,
                            int missingHarmonicFlag,
                            float weightFac,
                            INVF_MODE diffThres,
                            INVF_MODE inverseFilteringLevel,
                            const int sbrPatchingMode,
                            CODEC_TYPE coreCodec) {
  int l, k;
  float meanOrig = 0, meanSbr = 0, diff;
  float tonalityOrig, tonalitySbr;
  float** quotaMatrixSbr;

  if (coreCodec == CODEC_SAAC) {
    quotaMatrixSbr = quotaMatrixPatch;
  } else {
    quotaMatrixSbr = quotaMatrixOrig;
  }

  if (missingHarmonicFlag == 1) {
    for (l = startChannel; l < stopChannel; l++) {
      tonalityOrig = 0;
      for (k = startIndex; k < stopIndex; k++) {
        tonalityOrig += quotaMatrixOrig[k][l];
      }
      tonalityOrig /= (stopIndex - startIndex);
      tonalitySbr = 0;
      for (k = startIndex; k < stopIndex; k++) {
        tonalitySbr += quotaMatrixSbr[k][indexVector[l]];
      }
      tonalitySbr /= (stopIndex - startIndex);

      if (tonalityOrig > meanOrig)
        meanOrig = tonalityOrig;
      if (tonalitySbr > meanSbr)
        meanSbr = tonalitySbr;
    }
  } else {
    for (l = startChannel; l < stopChannel; l++) {
      tonalityOrig = 0;
      for (k = startIndex; k < stopIndex; k++) {
        tonalityOrig += quotaMatrixOrig[k][l];
      }
      tonalityOrig /= (stopIndex - startIndex);
      tonalitySbr = 0;
      for (k = startIndex; k < stopIndex; k++) {
        tonalitySbr += quotaMatrixSbr[k][indexVector[l]];
      }
      tonalitySbr /= (stopIndex - startIndex);

      meanOrig += tonalityOrig;
      meanSbr += tonalitySbr;
    }
    meanOrig /= (stopChannel - startChannel);
    meanSbr /= (stopChannel - startChannel);
  }

  if (meanOrig < 0.000976562 && meanSbr < 0.000976562) {
    meanOrig = 101.5936673f;
    meanSbr = 101.5936673f;
  }

  if (meanOrig < 1)
    meanOrig = 1;
  if (meanSbr < 1)
    meanSbr = 1;

  if (missingHarmonicFlag == 1) {
    diff = 1;
  } else {
    diff = max(1, weightFac * meanSbr / meanOrig);
  }

  if (inverseFilteringLevel <= diffThres) {
    diff = 1;
  }

  *noiseLevel = diff / meanOrig;
  if (!sbrPatchingMode) {
    *noiseLevel += 4 / (meanOrig * meanOrig);
  }

  if (!missingHarmonicFlag)
    *noiseLevel *= noiseFloorOffset;

  *noiseLevel = min(*noiseLevel, ana_max_level);
}

void SbrNoiseFloorEstimateQmf(HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate,
                              const SBR_FRAME_INFO* frame_info,
                              float* noiseLevels,
                              float** quotaMatrixOrig,
                              float** quotaMatrixPatch,
                              int* indexVector,
                              int missingHarmonicsFlag,
                              int startIndex,
                              int numberOfEstiamtesPerFrame,
                              int totalNumberOfEstimates,
                              int transientFlag,
                              INVF_MODE* pInvFiltLevels,
                              CODEC_TYPE coreCodec,
                              const int sbrPatchingMode,
                              int bSbr41,
                              float noiseLevelLoweringFactor[MAX_NOISE_ENVELOPES]) {
  int nNoiseEnvelopes, startPos[2], stopPos[2], env, band;

  int timeSlotsPerFrame = h_sbrNoiseFloorEstimate->timeSlots;
  int noNoiseBands = h_sbrNoiseFloorEstimate->noNoiseBands;
  int* freqBandTable = h_sbrNoiseFloorEstimate->freqBandTableQmf;
  float noiseLevelLoweringFactor_tmp[16] = {0};

  switch (coreCodec) {
    case CODEC_SAAC:

      nNoiseEnvelopes = frame_info->nNoiseEnvelopes;
      startPos[0] = startIndex;

      if (nNoiseEnvelopes == 1) {
        if (bSbr41) {
          stopPos[0] = startIndex + min(numberOfEstiamtesPerFrame, 4);
        } else {
          stopPos[0] = startIndex + min(numberOfEstiamtesPerFrame, 2);
        }
      } else {
        if (bSbr41) {
          stopPos[0] = startIndex + 2;
          startPos[1] = startIndex + 2;
          stopPos[1] = startIndex + 4;
        } else {
          stopPos[0] = startIndex + 1;
          startPos[1] = startIndex + 1;
          stopPos[1] = startIndex + min(numberOfEstiamtesPerFrame, 2);
        }
      }
      break;
    default:
      nNoiseEnvelopes = findStartAndStopPositions(frame_info,
                                                  startPos,
                                                  stopPos,
                                                  transientFlag,
                                                  timeSlotsPerFrame,
                                                  numberOfEstiamtesPerFrame,
                                                  totalNumberOfEstimates,
                                                  startIndex);
  }

  for (env = 0; env < nNoiseEnvelopes; env++) {
    for (band = 0; band < noNoiseBands; band++) {
      qmfBasedNoiseFloorDetection(&noiseLevels[band + env * noNoiseBands],
                                  quotaMatrixOrig,
                                  quotaMatrixPatch,
                                  indexVector,
                                  startPos[env],
                                  stopPos[env],
                                  freqBandTable[band],
                                  freqBandTable[band + 1],
                                  h_sbrNoiseFloorEstimate->ana_max_level,
                                  h_sbrNoiseFloorEstimate->noiseFloorOffset[band] *
                                      h_sbrNoiseFloorEstimate->noiseReductionFactor,
                                  missingHarmonicsFlag,
                                  h_sbrNoiseFloorEstimate->weightFac,
                                  h_sbrNoiseFloorEstimate->diffThres,
                                  pInvFiltLevels[band],
                                  sbrPatchingMode,
                                  coreCodec);

      noiseLevels[band + env * noNoiseBands] *= noiseLevelLoweringFactor[env];

      if (noiseLevels[band + env * noNoiseBands] >= 0.0 && noiseLevels[band + env * noNoiseBands] <= 1.0) {
        noiseLevelLoweringFactor_tmp[band + env * noNoiseBands] = (float)(1.f - sqrt(noiseLevels[band + env * noNoiseBands]));
      } else {
        noiseLevelLoweringFactor_tmp[band + env * noNoiseBands] = 1.f;
      }
    }
  }

  memcpy(noiseLevelLoweringFactor, noiseLevelLoweringFactor_tmp, (nNoiseEnvelopes * noNoiseBands) * sizeof(float));

  smoothingOfNoiseLevels(noiseLevels,
                         nNoiseEnvelopes,
                         h_sbrNoiseFloorEstimate->noNoiseBands,
                         h_sbrNoiseFloorEstimate->prevNoiseLevels,
                         h_sbrNoiseFloorEstimate->smoothLength,
                         h_sbrNoiseFloorEstimate->smoothFilter,
                         transientFlag);

  for (env = 0; env < nNoiseEnvelopes; env++) {
    for (band = 0; band < noNoiseBands; band++) {
      noiseLevels[band + env * noNoiseBands] = NOISE_FLOOR_OFFSET - (float)(ILOG2 * log(noiseLevels[band + env * noNoiseBands]));
    }
  }
}

static HANDLE_ERROR_INFO
downSampleLoRes(int* v_result,
                int num_result,
                const int* freqBandTableRef,
                int num_Ref) {
  int step;
  int i, j;
  int org_length, result_length;
  int v_index[MAX_FREQ_COEFFS / 2];

  org_length = num_Ref;
  result_length = num_result;

  v_index[0] = 0;
  i = 0;
  while (org_length > 0) {
    i++;
    step = org_length / result_length;
    org_length = org_length - step;
    result_length--;
    v_index[i] = v_index[i - 1] + step;
  }

  if (i != num_result)
    return iisUtil_ERROR(CDI, "error downsampling");

  for (j = 0; j <= i; j++) {
    v_result[j] = freqBandTableRef[v_index[j]];
  }

  return noError;
}

HANDLE_ERROR_INFO
CreateSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE* hSbr,
                            int ana_max_level,
                            const int* freqBandTable,
                            int nSfb,
                            int noiseBands,
                            int* noiseFloorOffset,
                            int timeSlots,
                            unsigned int useSpeechConfig) {
  int i;

  HANDLE_ERROR_INFO err;

  HANDLE_SBR_NOISE_FLOOR_ESTIMATE hs;
  hs = (HANDLE_SBR_NOISE_FLOOR_ESTIMATE)
      iisCalloc(1, sizeof(SBR_NOISE_FLOOR_ESTIMATE));
  if (hs == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  hs->smoothLength = smoothFilterLength;
  hs->smoothFilter = smoothFilter;

  if (useSpeechConfig) {
    hs->weightFac = 1.0f;
    hs->diffThres = INVF_LOW_LEVEL;
  } else {
    hs->weightFac = 0.25f;
    hs->diffThres = INVF_MID_LEVEL;
  }

  hs->prevNoiseLevels = (float**)iisCalloc(hs->smoothLength, sizeof(float*));
  if (hs->prevNoiseLevels == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < hs->smoothLength; i++) {
    hs->prevNoiseLevels[i] = (float*)iisCalloc(MAX_NUM_NOISE_VALUES, sizeof(float));
    if (hs->prevNoiseLevels[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->timeSlots = timeSlots;
  hs->ana_max_level = (float)pow(2, (float)ana_max_level / 3.0f);
  hs->noiseBands = noiseBands;

  hs->freqBandTableQmf = (int*)iisCalloc((MAX_NUM_NOISE_VALUES + 1), sizeof(int));
  if (hs->freqBandTableQmf == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  err = ResetSbrNoiseFloorEstimate(hs,
                                   freqBandTable,
                                   nSfb);

  if (err != noError)
    return handBack(err);

  for (i = 0; i < MAX_NUM_NOISE_COEFFS; i++) {
    hs->noiseFloorOffset[i] = (float)pow(2, noiseFloorOffset[i] / 3.0);
  }

  hs->noiseReductionFactor = 1.0f;

  *hSbr = hs;
  return noError;
}

HANDLE_ERROR_INFO
ResetSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbr,
                           const int* freqBandTable,
                           int nSfb) {
  int k2, kx;
  HANDLE_ERROR_INFO err;

  k2 = freqBandTable[nSfb];
  kx = freqBandTable[0];
  if (hSbr->noiseBands == 0) {
    hSbr->noNoiseBands = 1;
  } else {
    hSbr->noNoiseBands = (int)((hSbr->noiseBands * log((float)k2 / kx) * ILOG2) + 0.5);
    if (hSbr->noNoiseBands == 0)
      hSbr->noNoiseBands = 1;
    if (hSbr->noNoiseBands > MAX_NUM_NOISE_COEFFS) {
      return iisUtil_ERROR(CDI, "error: Combination of startFreq, stopFreq and noNoiseBands invalid");
    }
  }

  if (hSbr->noNoiseBands > nSfb) {
    return iisUtil_ERROR(CDI, "error: noNoiseBands too high");
  }

  err = downSampleLoRes(hSbr->freqBandTableQmf,
                        hSbr->noNoiseBands,
                        freqBandTable, nSfb);

  if (err != noError)
    return handBack(err);

  return noError;
}

void DeleteSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbrCut) {
  if (hSbrCut) {
    int i;

    if (hSbrCut->freqBandTableQmf)
      iisFree(hSbrCut->freqBandTableQmf);

    for (i = 0; i < hSbrCut->smoothLength; i++) {
      if (hSbrCut->prevNoiseLevels[i])
        iisFree(hSbrCut->prevNoiseLevels[i]);
    }

    if (hSbrCut->prevNoiseLevels)
      iisFree(hSbrCut->prevNoiseLevels);

    iisFree(hSbrCut);
  }
}

static float calcGeometricMean(float* data, int length) {
  int i;
  float mean = 0;
  for (i = 0; i < length; i++) {
    mean += (float)log(data[i]);
  }
  return (float)exp(mean / length);
}
static float calcMean(float* data, int length) {
  int i;
  float mean = 0;
  for (i = 0; i < length; i++) {
    mean += data[i];
  }
  return mean / (float)length;
}

void CalcNoiseLevelLoweringFactors(HANDLE_SBR_FRAME_INFO frameInfo,
                                   const float* const* const Energies,
                                   float* noiseLevelLoweringFactor,
                                   const int* freqBandTable,
                                   int nSfb,
                                   int timeStep,
                                   int noCols) {
  float SbrEnergies[MAX_NUMBER_TIME_SLOTS + FRAME_MIDDLE_SLOT_2048][MAX_FREQ_COEFFS] = {{0}};
  float SbrTimeSlotEnergies[MAX_NUMBER_TIME_SLOTS + FRAME_MIDDLE_SLOT_2048] = {0};

  int li, ui, j, k, slotOut, slotIn;
  int slot, freq, i;
  int sbrSlots = noCols / timeStep;

  const float LOWER_NOISE_START_THRESHOLD = 0.55f;

  const float LOWER_NOISE_STOP_THRESHOLD = 0.2f;

  for (i = 0; i < MAX_NOISE_ENVELOPES; i++) {
    noiseLevelLoweringFactor[i] = 1;
  }

  for (slotOut = 0; slotOut < sbrSlots + FRAME_MIDDLE_SLOT_2048; slotOut++) {
    slotIn = timeStep * slotOut;

    for (j = 0; j < nSfb; j++) {
      SbrEnergies[slotOut][j] = 0;

      li = freqBandTable[j];
      ui = freqBandTable[j + 1];

      for (k = li; k < ui; k++) {
        for (i = 0; i < timeStep; i++) {
          SbrEnergies[slotOut][j] += Energies[slotIn + i][k];
        }
      }
    }
  }

  for (slot = 0; slot < sbrSlots + FRAME_MIDDLE_SLOT_2048; slot++) {
    for (freq = 0; freq < nSfb; freq++) {
      SbrTimeSlotEnergies[slot] += SbrEnergies[slot][freq];
    }
  }

  for (i = 0; i < frameInfo->nNoiseEnvelopes; i++) {
    float temporalEnergyFlatness;
    float arithmeticMean;
    float geometricMean;

    int noiseEnvStartSlot = frameInfo->bordersNoise[i];
    int noiseEnvEndSlot = frameInfo->bordersNoise[i + 1];
    int nrSlotsInCurrentNoiseEnv = noiseEnvEndSlot - noiseEnvStartSlot;

    arithmeticMean = calcMean(SbrTimeSlotEnergies + noiseEnvStartSlot, nrSlotsInCurrentNoiseEnv);
    geometricMean = calcGeometricMean(SbrTimeSlotEnergies + noiseEnvStartSlot, nrSlotsInCurrentNoiseEnv);

    if (arithmeticMean == 0.0f) {
      temporalEnergyFlatness = 1;
    } else {
      temporalEnergyFlatness = geometricMean / arithmeticMean;
    }

    if (temporalEnergyFlatness < LOWER_NOISE_STOP_THRESHOLD) {
      noiseLevelLoweringFactor[i] = (float)pow(0.5, 60);

    } else if (temporalEnergyFlatness < LOWER_NOISE_START_THRESHOLD) {
      noiseLevelLoweringFactor[i] = (float)pow(2, (temporalEnergyFlatness - LOWER_NOISE_START_THRESHOLD) * (-30 / (LOWER_NOISE_STOP_THRESHOLD - LOWER_NOISE_START_THRESHOLD)));

    } else {
      noiseLevelLoweringFactor[i] = 1.0;
    }
  }
}

void AdjustNoiseFloor(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbrNoiseFloorEstimate, float noiseReductionFactor) {
  if (hSbrNoiseFloorEstimate) {
    hSbrNoiseFloorEstimate->noiseReductionFactor = noiseReductionFactor;
  }
}
