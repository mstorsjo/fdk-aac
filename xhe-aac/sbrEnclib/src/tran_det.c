
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
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "iisutillib.h"
#include "mathlib.h"
#include "tran_det.h"
#include "fram_gen.h"

#if defined __GNUC__ || defined __clang__
#define SBR_WITH_AAC __attribute__((unused))
#else
#define SBR_WITH_AAC
#endif

static float spectralChange(float Energies[MAX_NUMBER_TIME_SLOTS][MAX_FREQ_COEFFS],
                            float EnergyTotal,
                            int nSfb,
                            int start,
                            int border,
                            int stop) {
  int i, j;
  int len1 = border - start;
  int len2 = stop - border;
  float lenRatio = (float)len1 / (float)len2;
  float delta;
  float delta_sum = 0.0f;
  float nrg1[MAX_FREQ_COEFFS];
  float nrg2[MAX_FREQ_COEFFS];

  float pos_weight = (0.5f - (float)len1 / (float)(len1 + len2));
  pos_weight = 1.0f - 4.0f * pos_weight * pos_weight;

  for (j = 0; j < nSfb; j++) {
    nrg1[j] = 1.0e6f * NORM_SBR_PCM_LEVEL_SQ * len1;
    nrg2[j] = 1.0e6f * NORM_SBR_PCM_LEVEL_SQ * len2;

    for (i = start; i < border; i++) {
      nrg1[j] += Energies[i][j];
    }

    for (i = border; i < stop; i++) {
      nrg2[j] += Energies[i][j];
    }
  }

  for (j = 0; j < nSfb; j++) {
    delta = (float)fabs(log(nrg2[j] / nrg1[j] * lenRatio));

    delta_sum += (float)(sqrt(nrg1[j] + nrg2[j]) * delta);
  }

  delta_sum /= (float)sqrt(EnergyTotal);

  return delta_sum * pos_weight;
}

static float addLowbandEnergies(const float *const *const Energies,
                                const int *freqBandTable,
                                int slots,
                                CODEC_TYPE coreCodec) {
  int i, k;
  float nrgTotal;

  switch (coreCodec) {
    case CODEC_SAAC:
      nrgTotal = 1.0f * NORM_SBR_PCM_LEVEL_SQ;
      break;

    default:
      nrgTotal = 1.0f;
      break;
  }

  for (k = 0; k < freqBandTable[0]; k++) {
    for (i = 0; i < slots; i++) {
      nrgTotal += Energies[i][k];
    }
  }

  return nrgTotal;
}

static float addHighbandEnergies(const float *const *const Energies,
                                 float EnergiesM[MAX_NUMBER_TIME_SLOTS][MAX_FREQ_COEFFS],
                                 const int *freqBandTable,
                                 int nSfb,
                                 int sbrSlots,
                                 int timeStep,
                                 CODEC_TYPE coreCodec) {
  int i, j, k, slotIn, slotOut;
  int li, ui;
  float nrgTotal;
  switch (coreCodec) {
    case CODEC_SAAC:
      nrgTotal = 1.0f * NORM_SBR_PCM_LEVEL_SQ;
      break;

    default:
      nrgTotal = 1.0f;
      break;
  }

  for (slotOut = 0; slotOut < sbrSlots; slotOut++) {
    slotIn = timeStep * slotOut;

    for (j = 0; j < nSfb; j++) {
      EnergiesM[slotOut][j] = 0;

      li = freqBandTable[j];
      ui = freqBandTable[j + 1];

      for (k = li; k < ui; k++) {
        for (i = 0; i < timeStep; i++) {
          EnergiesM[slotOut][j] += Energies[slotIn + i][k];
        }
      }
    }
  }

  for (slotOut = 0; slotOut < sbrSlots; slotOut++) {
    for (j = 0; j < nSfb; j++) {
      nrgTotal += EnergiesM[slotOut][j];
    }
  }

  return (nrgTotal);
}

void FrameSplitter(const float *const *const Energies,
                   HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                   const int *freqBandTable,
                   int nSfb,
                   int timeStep,
                   int noCols,
                   int *tran_vector,
                   CODEC_TYPE coreCodec,
                   const int isSwitchingDecisionResultSpeech,
                   int *fixfixGridGranularity,
                   const int bitRate,
                   const int nChannels) {
  if (tran_vector[1] == 0) {
    float EnergiesM[MAX_NUMBER_TIME_SLOTS][MAX_FREQ_COEFFS] = {{0.0f}};
    float EnergyTotal, newLowbandEnergy;
    int border;
    int sbrSlots = noCols / timeStep;
    float low_br_threshold_scaler = (bitRate <= LOW_BITRATE_TUNING_LIMIT) ? 16.f : 1.f;

    if (!h_sbrTransientDetector->useFrameSplitter)
      return;

    assert(sbrSlots * timeStep == noCols);

    newLowbandEnergy = addLowbandEnergies(Energies + noCols / 2,
                                          freqBandTable,
                                          noCols,
                                          coreCodec);

    EnergyTotal = 0.5f * (newLowbandEnergy + h_sbrTransientDetector->prevLowBandEnergy);

    h_sbrTransientDetector->totalHighBandEnergy = addHighbandEnergies(Energies,
                                                                      EnergiesM,
                                                                      freqBandTable,
                                                                      nSfb,
                                                                      sbrSlots,
                                                                      timeStep,
                                                                      coreCodec);

    EnergyTotal += h_sbrTransientDetector->totalHighBandEnergy;

    h_sbrTransientDetector->totalHighBandEnergy /= (sbrSlots * nSfb);
    if (
        !isSwitchingDecisionResultSpeech &&
        (bitRate > LOW_BITRATE_TUNING_LIMIT)) {
      float delta;

      border = (sbrSlots + 1) / 2;

      delta = spectralChange(EnergiesM,
                             EnergyTotal,
                             nSfb,
                             0,
                             border,
                             sbrSlots);

      if (delta > h_sbrTransientDetector->splitThr) {
        tran_vector[0] = 1;
      } else {
        tran_vector[0] = 0;
      }

    } else {
      float delta[NUMBER_TIME_SLOTS_2048 / 2];
      int i, j, slot;

      tran_vector[0] = 0;

      for (i = 0; i < sbrSlots / 2; i++) {
        if (i == 0) {
          EnergyTotal = addLowbandEnergies(Energies,
                                           freqBandTable,
                                           6,
                                           coreCodec);
          EnergyTotal = EnergyTotal / 1.5f;
        } else {
          EnergyTotal = addLowbandEnergies(Energies + i * 4 - 2,
                                           freqBandTable,
                                           8,
                                           coreCodec);
          EnergyTotal /= 2;
        }

        for (slot = 0; slot < 2; slot++) {
          for (j = 0; j < nSfb; j++) {
            EnergyTotal += EnergiesM[slot + i * 2][j];
          }
        }
        switch (coreCodec) {
          case CODEC_SAAC:
            EnergyTotal += 1.0f * NORM_SBR_PCM_LEVEL_SQ;
            break;
          default:
            EnergyTotal += 1.0f;
        }

        delta[i] = spectralChange(EnergiesM,
                                  EnergyTotal,
                                  nSfb,
                                  i * 2,
                                  i * 2 + 1,
                                  (i + 1) * 2);
      }

      if (bitRate == 24000 && nChannels == 2) {
        if (delta[0] > (h_sbrTransientDetector->splitThr * 4) ||
            delta[2] > (h_sbrTransientDetector->splitThr * 4) ||
            delta[4] > (h_sbrTransientDetector->splitThr * 4) ||
            delta[6] > (h_sbrTransientDetector->splitThr * 4)) {
          *fixfixGridGranularity = 8;
          tran_vector[0] = 1;
        } else {
          if (delta[1] > (h_sbrTransientDetector->splitThr * 2) ||
              delta[5] > (h_sbrTransientDetector->splitThr * 2)) {
            *fixfixGridGranularity = 4;
            tran_vector[0] = 1;
          } else {
            if (delta[3] > (h_sbrTransientDetector->splitThr)) {
              *fixfixGridGranularity = 2;
              tran_vector[0] = 1;
            } else {
              *fixfixGridGranularity = 1;
              tran_vector[0] = 0;
            }
          }
        }
      } else {
        if (delta[0] > (h_sbrTransientDetector->splitThr * low_br_threshold_scaler) ||
            delta[2] > (h_sbrTransientDetector->splitThr * low_br_threshold_scaler) ||
            delta[4] > (h_sbrTransientDetector->splitThr * low_br_threshold_scaler) ||
            delta[6] > (h_sbrTransientDetector->splitThr * low_br_threshold_scaler)) {
          *fixfixGridGranularity = 8;
          tran_vector[0] = 1;
        } else {
          if (delta[1] > (h_sbrTransientDetector->splitThr) ||
              delta[5] > (h_sbrTransientDetector->splitThr)) {
            *fixfixGridGranularity = 4;
            tran_vector[0] = 1;
          } else {
            if (delta[3] > (h_sbrTransientDetector->splitThr)) {
              *fixfixGridGranularity = 4;
              tran_vector[0] = 1;
            } else {
              *fixfixGridGranularity = 2;
              tran_vector[0] = 1;
            }
          }
        }
      }
    }

    h_sbrTransientDetector->prevLowBandEnergy = newLowbandEnergy;
  } else {
    if (bitRate <= LOW_BITRATE_TUNING_LIMIT) {
      if (isSwitchingDecisionResultSpeech == 0) {
        *fixfixGridGranularity = 8;
        tran_vector[0] = 1;
        tran_vector[1] = 0;
      }
    }
  }
}

static void
extractTransientCandidates(const float *const *Energies,
                           int maLength,
                           int noBands,
                           float *transients,
                           int calcLength,
                           CODEC_TYPE coreCodec) {
  int band, tslot;
  float delta, threshold, meanVal, enerVal, temp;

  const int us = 0;
  const float iNoMean = 1.0f / (maLength + us);
  const float iNoEner = 1.0f / (maLength + us - 1.0f);
  const float comp = iNoEner / iNoMean;

  for (band = 0; band < noBands; band++) {
    meanVal = enerVal = 0.0f;
    for (tslot = -maLength; tslot < 0; tslot++) {
      float nrg = Energies[tslot][band];
#ifndef _NOT_AVOID_FLOAT_DENORMALS
      if (nrg < 1.e-12f) nrg = 0.f;
#endif
      meanVal += nrg;
      enerVal += nrg * nrg;
    }

    meanVal *= iNoMean;
    enerVal *= iNoEner;

    for (tslot = 0; tslot < calcLength; tslot++) {
      float nrg1 = Energies[tslot][band];
      float nrg2 = Energies[tslot - maLength][band];

#ifndef _NOT_AVOID_FLOAT_DENORMALS
      if (nrg1 < 1.e-8f) nrg1 = 0.f;
      if (nrg2 < 1.e-8f) nrg2 = 0.f;
#endif

      meanVal += iNoMean * (nrg1 - nrg2);

      enerVal += iNoEner * (nrg1 * nrg1 - nrg2 * nrg2);

      temp = enerVal - comp * meanVal * meanVal;

      if (temp < 0)
        temp = 0;

      temp = (float)sqrt(temp);

      switch (coreCodec) {
        case CODEC_SAAC:
          threshold = max(ABS_THRES_NS, meanVal + temp);
          break;

        default:
          threshold = max(ABS_THRES, meanVal + temp);
          break;
      }

      delta = (Energies[tslot + 1][band] - Energies[tslot - 1][band]);
      if (delta > threshold)

        transients[tslot] += delta / threshold - 1;
    }
  }
}

void TransientDetect(const float *const *Energies,
                     HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTran,
                     int *tran_vector,
                     int timeStep,
                     CODEC_TYPE coreCodec, int bSbr41) {
  int i, cond;

  int noCols = h_sbrTran->noCols;
  int qmfStartSample = noCols + h_sbrTran->startIndex;
  int frameShift = h_sbrTran->frameShift;
  int beginSample, nSamples;
  float mean = 0;
  float intThres;
  float relThres = 38400.0f / h_sbrTran->numSbInclude;

  tran_vector[2] = 0;

  if (bSbr41) {
    intThres = h_sbrTran->tranThr / (1.7f * h_sbrTran->numSbInclude);
  } else {
    intThres = h_sbrTran->tranThr / h_sbrTran->numSbInclude;
  }

  memmove(h_sbrTran->transients, h_sbrTran->transients + noCols, qmfStartSample * sizeof(float));
  memset(h_sbrTran->transients + qmfStartSample, 0, h_sbrTran->calcBufferLength * sizeof(float));

  extractTransientCandidates(Energies + h_sbrTran->startIndex,
                             h_sbrTran->movingAverageLength,
                             h_sbrTran->numSbInclude,
                             h_sbrTran->transients + qmfStartSample,
                             h_sbrTran->calcBufferLength,
                             coreCodec);

  beginSample = max(qmfStartSample - 2 * h_sbrTran->movingAverageLength, 0);
  nSamples = qmfStartSample - beginSample;
  for (i = beginSample; i < qmfStartSample; i++) {
    mean += h_sbrTran->transients[i];
  }

  mean /= nSamples;

  tran_vector[0] = 0;
  tran_vector[1] = 0;

  for (i = qmfStartSample - frameShift; i < qmfStartSample + noCols - frameShift; i++) {
    mean += (h_sbrTran->transients[i] - h_sbrTran->transients[i - nSamples]) / nSamples;

    if (!h_sbrTran->tran) {
      if ((h_sbrTran->transients[i - 1] > intThres) &&
          (h_sbrTran->transients[i] < h_sbrTran->transients[i - 1]) &&
          (h_sbrTran->transients[i - 1] > h_sbrTran->averageWeight * mean)) {
        h_sbrTran->tran = 1;
      } else {
        h_sbrTran->tran = 0;
      }
    }

    if (h_sbrTran->transients[i] > h_sbrTran->maxTran)
      h_sbrTran->maxTran = h_sbrTran->transients[i];

    cond = h_sbrTran->tran && (h_sbrTran->transients[i] < relThres);

    cond = cond &&
           (h_sbrTran->transients[i + 1] < h_sbrTran->maxTran) &&
           (h_sbrTran->transients[i + 2] < h_sbrTran->maxTran);

    if (cond) {
      tran_vector[0] =
          (int)floor((i - qmfStartSample + frameShift) / timeStep);
      tran_vector[1] = 1;
      h_sbrTran->tran = 0;
      h_sbrTran->maxTran = 0;

      break;
    }
  }
}

HANDLE_ERROR_INFO
CreateSbrTransientDetector(HANDLE_SBR_TRANSIENT_DETECTOR *hSbr,
                           int frameSize,
                           int sampleFreq,
                           float bitrateFactor,
                           float tranThr,
                           int numSbInclude,
                           float averageWeight,
                           int noCols,
                           int noRows,
                           int startIndex,
                           int bufferSize,
                           CODEC_TYPE coreCoder,
                           SBR_WITH_AAC int downScaleFactor)

{
  HANDLE_SBR_TRANSIENT_DETECTOR hs;
  float frameDur = (float)frameSize / (float)sampleFreq;

  float temp = frameDur - 0.010f;

  hs =
      (HANDLE_SBR_TRANSIENT_DETECTOR)
          iisCalloc(1, sizeof(SBR_TRANSIENT_DETECTOR));

  if (temp < 0.0001f)
    temp = 0.0001f;

  temp = 0.000075f / (temp * temp);

  if (hs == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  switch (coreCoder) {
    case CODEC_SAAC:
      hs->startIndex = startIndex;
      hs->frameShift = 0;
      hs->useFrameSplitter = 1;
      break;
    default:
      return iisUtil_ERROR(CDI, "unknown core coder");
  }

  hs->averageWeight = averageWeight;

  hs->movingAverageLength = (int)(8 * sampleFreq / 48000.0);

  if (hs->movingAverageLength > hs->startIndex)
    hs->movingAverageLength = hs->startIndex;

  hs->tran = 0;
  hs->maxTran = 0;

  hs->calcBufferLength = noCols + 2;
  if (hs->calcBufferLength + hs->startIndex + 1 > bufferSize)
    return iisUtil_ERROR(CDI, "Too short buffers in transient detector");

  assert((numSbInclude > 0) && (numSbInclude <= 64));
  hs->numSbInclude = numSbInclude;
  hs->tranThr = tranThr;
  hs->splitThr = temp * bitrateFactor;
  hs->noCols = noCols;
  hs->noChannels = noRows;
  hs->prevLowBandEnergy = 0;

  hs->transients = (float *)iisCalloc(bufferSize + noCols, sizeof(float));
  if (hs->transients == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  *hSbr = hs;
  return (noError);
}

void DeleteSbrTransientDetector(HANDLE_SBR_TRANSIENT_DETECTOR hSbrCut) {
  if (hSbrCut) {
    iisFree(hSbrCut->transients);
    iisFree(hSbrCut);
  }
}
