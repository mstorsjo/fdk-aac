
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

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iisutillib.h"

#include "iisLevelerLib.h"
#include "iisLevelerLibVersion.h"

#define IF_ENV(env, value) if (getenv(#env) && 0 == strcmp(getenv(#env), #value))

#define GAMMA_A (-70)
#define GAMMA_A_LIN (3.1622776e-4)
#define GAMMA_R_DELTA (10)
#define GAMMA_R_DELTA_LIN (3.16227766017)

#define MAX_DELAY_IN_MILLISECONDS (3000)
#define MIN_DELAY_IN_MILLISECONDS (10)

#define RAMP_DURATION_IN_MILLISECONDS (100)

#define MAX_GATED_LONG_TERM_LOUDNESS_ERROR (6)
#define MIN_GATED_LONG_TERM_LOUDNESS_ERROR (-6)

#define SILENCE_DETECTION_RELEASE_TIME (50)
#define SILENCE_DETECTION_ATTACK_TIME (400)

#define INTERMEDIATE_TARGET_LOUDNESS (-24)

#define DELAY_IN_MILLISECONDS (10)

#define PI 3.14159265f

#define LOUD_MIN (-150)

#define LIN2DB(x) (((x) > 0) ? (20 * (float)log10(x)) : (LOUD_MIN))
#define DB2LIN(x) (pow(10, ((x)) * 0.05))
#define NRG2DB(x) (((x) > 0) ? (10 * (float)log10(x)) : (LOUD_MIN))
#define NRG2LIN(x) (sqrt(x))
#define DB2NRG(x) (pow(10, ((x)) * 0.10))

#define LERP(a, A, b, B, x) (((A) - (B)) / ((a) - (b)) * (x) + ((B) - ((A) - (B)) / ((a) - (b)) * (b)))
#define LORP(x, a) log10(1.f + (a) * (x)) / log10(1.f + (a))
#define EXRP(x, a) ((pow(10.f, (a) * (x)) - 1.f) / (pow(10, (a)) - 1.f))

#define EN_LIMIT (1.00000000e-15)

#define INSTANTANEOUS_LOUDNESS_SMOOTH_TIME_CONSTANT (5)
#define MID_TERM_LOUDNESS_SMOOTH_TIME_CONSTANT (50000)
#define CURRENT_LOUDNESS_SMOOTH_TIME_CONSTANT (5000)
#define GAIN_SMOOTHING_DECAY_SLOW_TIME_CONSTANT (3000)
#define GAIN_SMOOTHING_ATTACK_FAST_TIME_CONSTANT (5)
#define GAIN_SMOOTHING_ATTACK_SLOW_TIME_CONSTANT (500)

#define DELAY_IN_MILLISECONDS (10)
#define MILLISECONDS_PER_SECOND (1000)

#define DEFAULT_REL_MAX_GAIN DB2LIN(10)
#define DEFAULT_ABS_MAX_GAIN DB2LIN(40)

#define LEAKY_INTEGRATOR_FACTOR(t, samplingRate) (exp((-1.0 / (((double)(t) / (MILLISECONDS_PER_SECOND)) * ((double)samplingRate)))))
#define LEAKY_INTEGRATOR(now, prev, fac) ((1.0 - (fac)) * (now) + (fac) * (prev))

typedef struct BIQUAD_COEFF {
  float b0, b1, b2;
  float a1, a2;
} BIQUAD_COEFF;

typedef struct BIQUAD_STATE {
  float z1, z2;
} BIQUAD_STATE;

static void setKweightFilt(BIQUAD_COEFF *preFilt, BIQUAD_COEFF *rlbFilt, unsigned int fs);

typedef struct {
  float *buffer;
  size_t size;
} RING_BUFFER, *RING_BUFFER_HANDLE;

static int s_RingBuffer_Create(RING_BUFFER_HANDLE *phRingBuffer, size_t size) {
  assert(phRingBuffer != 0);

  *phRingBuffer = iisCalloc(sizeof(RING_BUFFER), 1);

  if (*phRingBuffer == NULL) {
    return -1;
  }

  (*phRingBuffer)->buffer = iisCalloc(size, sizeof(float));
  if ((*phRingBuffer)->buffer == NULL) return -1;
  (*phRingBuffer)->size = size;

  return 0;
}

static void s_RingBuffer_Destroy(RING_BUFFER_HANDLE *phRingBuffer) {
  assert(phRingBuffer != 0);

  if (phRingBuffer && *phRingBuffer) {
    if ((*phRingBuffer)->buffer) {
      iisFree((*phRingBuffer)->buffer);
    }
    iisFree((*phRingBuffer));
  }
}

static void s_RingBuffer_Update(RING_BUFFER_HANDLE hRingBuffer, size_t num) {
  assert(hRingBuffer != NULL);
  assert(num <= hRingBuffer->size);
  memmove(hRingBuffer->buffer, (float *)hRingBuffer->buffer + num, (hRingBuffer->size - num) * sizeof(float));
}

static void s_RingBuffer_Reset(RING_BUFFER_HANDLE hRingBuffer) {
  assert(hRingBuffer != 0);
  memset(hRingBuffer->buffer, 0, sizeof(float) * hRingBuffer->size);
}

struct IIS_LEVELER_LIB_STRUCT {
  IIS_LEVELER_LIB_CONFIG config;

  struct IIS_LEVELER_LIB_SETTINGS {
    float targetLoudness;
    float targetLoudnessLin;
    float relativeMaxGain;
    float relativeMaxGainLin;
    float silenceLevelThreshold;
    int loudnessComplianceStageEnabled;
    int silenceDetectionEnabled;
    float adaptiveAttack;
    float dynamicRangePreservation;
    float dynamicRangePreservationFac;

    float channelWeights[IIS_LEVELER_LIB_MAX_CHANNELS];
    int ignoreChannel[IIS_LEVELER_LIB_MAX_CHANNELS];
  } settings;

  IIS_LEVELER_LIB_STATE state;

  unsigned int rampSampleCount;
  unsigned int rampSampleNumber;

  float intermediateTargetLoudnessDeltaLin;

  RING_BUFFER_HANDLE hFrameBuffer;

  int frameSize;

  struct {
    BIQUAD_COEFF coeff;
    BIQUAD_STATE state[IIS_LEVELER_LIB_MAX_CHANNELS];
  } preFilt, rlbFilt;

  float prevSmoothPreGain;

  float maxGain;

  RING_BUFFER_HANDLE hEnergyBuffer;
  RING_BUFFER_HANDLE hInstantaneousLevelBuffer;
  RING_BUFFER_HANDLE hMidTermLevelBuffer;
  RING_BUFFER_HANDLE hMeasurementConfidenceBuffer;
  RING_BUFFER_HANDLE hRoughGainBuffer;

  float linearChannelWeights[IIS_LEVELER_LIB_MAX_CHANNELS];

  float currentInstantaneousEnergy;
  double currentMeasurementConfidence;
  float currentMidTermEnergy;
  float currentMidTermEnergyAbsGated;
  float initialMidTermEnergy;

  int intrinsicDelayInSamples;
  int delayInSamples;
  float gainSmoothDecaySlowFactor;
  float gainSmoothAttackFastFactor;
  float gainSmoothAttackSlowFactor;

  float correctionFactor;

  float instantaneousLeakyIntegratorFactor;
  float midTermLeakyIntegratorFactor;

  long long totalNumberOfSamplesPerChannel;

  float preGainGatedLongTermLoudness;
};

IIS_LEVELER_LIB_VERSION_INFO
iisLevelerLib_GetVersionInfo(void) {
  IIS_LEVELER_LIB_VERSION_INFO versionInfo = {0};
  versionInfo.major = IIS_LEVELER_LIB_VERSION_MAJOR;
  versionInfo.minor = IIS_LEVELER_LIB_VERSION_MINOR;
  versionInfo.patch = IIS_LEVELER_LIB_VERSION_PATCH;
  return versionInfo;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Create(
    IIS_LEVELER_LIB_HANDLE *phLeveler,
    const IIS_LEVELER_LIB_CONFIG *pConfig) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  IIS_LEVELER_LIB_HANDLE hLeveler = NULL;

  if (NULL == phLeveler || NULL == pConfig) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *phLeveler = iisCalloc(1, sizeof(struct IIS_LEVELER_LIB_STRUCT));

  if (NULL == *phLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_MEMORY;
    goto error;
  }

  hLeveler = *phLeveler;
  hLeveler->config = *pConfig;
  hLeveler->state = IIS_LEVELER_LIB_STATE_INITIALIZED;

  if (hLeveler->config.maxSamplesPerChannel < 1) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  if (hLeveler->config.numberOfChannels < 1 || hLeveler->config.numberOfChannels > IIS_LEVELER_LIB_MAX_CHANNELS) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  if (hLeveler->config.samplingRate < IIS_LEVELER_LIB_MIN_SAMPLING_RATE || hLeveler->config.samplingRate > IIS_LEVELER_LIB_MAX_SAMPLING_RATE) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  if (hLeveler->config.delayMultiple < 0) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  hLeveler->rampSampleNumber = RAMP_DURATION_IN_MILLISECONDS * hLeveler->config.samplingRate / MILLISECONDS_PER_SECOND;
  hLeveler->rampSampleCount = hLeveler->rampSampleNumber;

  hLeveler->intrinsicDelayInSamples = hLeveler->config.samplingRate * DELAY_IN_MILLISECONDS / MILLISECONDS_PER_SECOND;

  if (hLeveler->config.delayMultiple) {
    hLeveler->delayInSamples = ((hLeveler->intrinsicDelayInSamples - 1) / hLeveler->config.delayMultiple + 1) * hLeveler->config.delayMultiple;
  } else {
    hLeveler->delayInSamples = hLeveler->intrinsicDelayInSamples;
  }

  hLeveler->gainSmoothAttackFastFactor = LEAKY_INTEGRATOR_FACTOR(GAIN_SMOOTHING_ATTACK_FAST_TIME_CONSTANT, hLeveler->config.samplingRate);
  hLeveler->gainSmoothDecaySlowFactor = LEAKY_INTEGRATOR_FACTOR(GAIN_SMOOTHING_DECAY_SLOW_TIME_CONSTANT, hLeveler->config.samplingRate);

  hLeveler->instantaneousLeakyIntegratorFactor = LEAKY_INTEGRATOR_FACTOR(INSTANTANEOUS_LOUDNESS_SMOOTH_TIME_CONSTANT, hLeveler->config.samplingRate);

  hLeveler->midTermLeakyIntegratorFactor = LEAKY_INTEGRATOR_FACTOR(MID_TERM_LOUDNESS_SMOOTH_TIME_CONSTANT, hLeveler->config.samplingRate);

  hLeveler->frameSize = hLeveler->config.maxSamplesPerChannel;

  if (IIS_LEVELER_LIB_OUTPUT_FORMAT_SAMPLES == hLeveler->config.outputFormat) {
    if (s_RingBuffer_Create(&hLeveler->hFrameBuffer, hLeveler->config.numberOfChannels * (hLeveler->frameSize + hLeveler->delayInSamples))) {
      goto error;
    }
  }

  int additionalDelay = hLeveler->delayInSamples - hLeveler->intrinsicDelayInSamples;
  if (s_RingBuffer_Create(&hLeveler->hEnergyBuffer, (size_t)hLeveler->config.maxSamplesPerChannel + additionalDelay)) {
    goto error;
  }
  if (s_RingBuffer_Create(&hLeveler->hInstantaneousLevelBuffer, (size_t)hLeveler->config.maxSamplesPerChannel)) {
    goto error;
  }
  if (s_RingBuffer_Create(&hLeveler->hMidTermLevelBuffer, (size_t)hLeveler->config.maxSamplesPerChannel)) {
    goto error;
  }
  if (s_RingBuffer_Create(&hLeveler->hMeasurementConfidenceBuffer, (size_t)hLeveler->config.maxSamplesPerChannel)) {
    goto error;
  }
  if (s_RingBuffer_Create(&hLeveler->hRoughGainBuffer, (size_t)hLeveler->config.maxSamplesPerChannel)) {
    goto error;
  }

  setKweightFilt(&(hLeveler->preFilt.coeff), &(hLeveler->rlbFilt.coeff), hLeveler->config.samplingRate);

  if (iisLevelerLib_Set_targetLoudness(hLeveler, -23.f)) {
    goto error;
  }

  if (iisLevelerLib_Set_relativeMaxGain(hLeveler, IIS_LEVELER_LIB_DEFAULT_RELATIVE_MAX_GAIN)) {
    goto error;
  }

  if (iisLevelerLib_Set_adaptiveAttack(hLeveler, 1.0f)) {
    goto error;
  }

  if (iisLevelerLib_Set_dynamicRangePreservation(hLeveler, 1.0f)) {
    goto error;
  }

  {
    float channelWeights[IIS_LEVELER_LIB_MAX_CHANNELS] = {0};
    int ignoreChannel[IIS_LEVELER_LIB_MAX_CHANNELS] = {0};
    iisLevelerLib_Set_channelWeights(hLeveler, channelWeights);
    iisLevelerLib_Set_ignoreChannel(hLeveler, ignoreChannel);
  }

  iisLevelerLib_Reset(*phLeveler);

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  if (returnValue) {
    if (phLeveler) {
      if (*phLeveler) {
        s_RingBuffer_Destroy(&(*phLeveler)->hEnergyBuffer);
        s_RingBuffer_Destroy(&(*phLeveler)->hInstantaneousLevelBuffer);
        s_RingBuffer_Destroy(&(*phLeveler)->hMidTermLevelBuffer);
        s_RingBuffer_Destroy(&(*phLeveler)->hMeasurementConfidenceBuffer);
        s_RingBuffer_Destroy(&(*phLeveler)->hRoughGainBuffer);
        s_RingBuffer_Destroy(&(*phLeveler)->hFrameBuffer);
      }
      iisFree(*phLeveler);
      *phLeveler = NULL;
    }
  }

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Destroy(
    IIS_LEVELER_LIB_HANDLE *phLeveler) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == phLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  if (*phLeveler) {
    s_RingBuffer_Destroy(&(*phLeveler)->hFrameBuffer);

    s_RingBuffer_Destroy(&(*phLeveler)->hEnergyBuffer);
    s_RingBuffer_Destroy(&(*phLeveler)->hInstantaneousLevelBuffer);
    s_RingBuffer_Destroy(&(*phLeveler)->hMidTermLevelBuffer);
    s_RingBuffer_Destroy(&(*phLeveler)->hMeasurementConfidenceBuffer);
    s_RingBuffer_Destroy(&(*phLeveler)->hRoughGainBuffer);

    iisFree(*phLeveler);
    *phLeveler = NULL;
  }

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Reset(
    IIS_LEVELER_LIB_HANDLE hLeveler) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  s_RingBuffer_Reset(hLeveler->hEnergyBuffer);
  s_RingBuffer_Reset(hLeveler->hInstantaneousLevelBuffer);
  s_RingBuffer_Reset(hLeveler->hMidTermLevelBuffer);
  s_RingBuffer_Reset(hLeveler->hMeasurementConfidenceBuffer);
  s_RingBuffer_Reset(hLeveler->hRoughGainBuffer);

  hLeveler->preGainGatedLongTermLoudness = 1.f;
  hLeveler->currentMeasurementConfidence = 0.0;
  hLeveler->currentInstantaneousEnergy = 0;
  hLeveler->initialMidTermEnergy = DB2NRG(-23.f + (IIS_LEVELER_LIB_DEFAULT_RELATIVE_MAX_GAIN - hLeveler->settings.relativeMaxGain));
  hLeveler->currentMidTermEnergy = hLeveler->initialMidTermEnergy;
  hLeveler->currentMidTermEnergyAbsGated = hLeveler->initialMidTermEnergy;

  hLeveler->totalNumberOfSamplesPerChannel = 0;

  memset(hLeveler->preFilt.state, 0, sizeof(hLeveler->preFilt.state));
  memset(hLeveler->rlbFilt.state, 0, sizeof(hLeveler->rlbFilt.state));

  hLeveler->intermediateTargetLoudnessDeltaLin = DB2LIN(hLeveler->settings.targetLoudness - INTERMEDIATE_TARGET_LOUDNESS);
  hLeveler->prevSmoothPreGain = hLeveler->intermediateTargetLoudnessDeltaLin * DB2LIN(hLeveler->settings.relativeMaxGain - IIS_LEVELER_LIB_DEFAULT_RELATIVE_MAX_GAIN);

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

int iisLevelerLib_Get_delayInSamples(
    IIS_LEVELER_LIB_HANDLE hLeveler) {
  if (!hLeveler) {
    return -1;
  } else {
    return hLeveler->delayInSamples;
  }
}

int iisLevelerLib_CalculateDelayInSamples(int samplingRate, int delayMultiple) {
  int delay = samplingRate * DELAY_IN_MILLISECONDS / MILLISECONDS_PER_SECOND;

  if (delayMultiple)
    delay = ((delay - 1) / delayMultiple + 1) * delayMultiple;

  return delay;
}

static void s_feedEnergyBuffer(IIS_LEVELER_LIB_HANDLE hLeveler, const float *frame, int size) {
  float totalEnergy = 0;
  float *energyBuffer = hLeveler->hEnergyBuffer->buffer;
  const BIQUAD_COEFF preC = hLeveler->preFilt.coeff;
  const BIQUAD_COEFF rlbC = hLeveler->rlbFilt.coeff;
  BIQUAD_STATE preS[IIS_LEVELER_LIB_MAX_CHANNELS];
  BIQUAD_STATE rlbS[IIS_LEVELER_LIB_MAX_CHANNELS];
  memcpy(preS, hLeveler->preFilt.state, hLeveler->config.numberOfChannels * sizeof(BIQUAD_STATE));
  memcpy(rlbS, hLeveler->rlbFilt.state, hLeveler->config.numberOfChannels * sizeof(BIQUAD_STATE));

  for (int i = 0; i < size; i++) {
    float energy = 0;
    for (int c = 0; c < hLeveler->config.numberOfChannels; c++) {
      float z0 = 0;
      float tmp = frame[i * hLeveler->config.numberOfChannels + c];

      if (hLeveler->settings.ignoreChannel[c]) {
        continue;
      }

      z0 = tmp - preC.a1 * preS[c].z1 - preC.a2 * preS[c].z2;
      tmp = preC.b0 * z0 + preC.b1 * preS[c].z1 + preC.b2 * preS[c].z2;
      preS[c].z2 = preS[c].z1;
      preS[c].z1 = z0;

      z0 = tmp - rlbC.a1 * rlbS[c].z1 - rlbC.a2 * rlbS[c].z2;
      tmp = rlbC.b0 * z0 + rlbC.b1 * rlbS[c].z1 + rlbC.b2 * rlbS[c].z2;
      rlbS[c].z2 = rlbS[c].z1;
      rlbS[c].z1 = z0;

      energy +=
          hLeveler->linearChannelWeights[c] *
          tmp * tmp;
    }

    int additionalDelay = hLeveler->delayInSamples - hLeveler->intrinsicDelayInSamples;
    energyBuffer[additionalDelay + i] = energy;
    totalEnergy += energy;
  }

  if (totalEnergy < EN_LIMIT) {
    for (int c = 0; c < hLeveler->config.numberOfChannels; c++) {
      preS[c].z1 = preS[c].z2 = 0.0f;
      rlbS[c].z1 = rlbS[c].z2 = 0.0f;
    }
  }

  memcpy(hLeveler->preFilt.state, preS, hLeveler->config.numberOfChannels * sizeof(BIQUAD_STATE));
  memcpy(hLeveler->rlbFilt.state, rlbS, hLeveler->config.numberOfChannels * sizeof(BIQUAD_STATE));
}

static void s_computeLevels(IIS_LEVELER_LIB_HANDLE hLeveler, int samplesPerChannel) {
  float *energyBuffer = hLeveler->hEnergyBuffer->buffer;
  float *midTermLevelBuffer = hLeveler->hMidTermLevelBuffer->buffer;
  float *instantaneousLevelBuffer = hLeveler->hInstantaneousLevelBuffer->buffer;
  float *measurementConfidenceBuffer = hLeveler->hMeasurementConfidenceBuffer->buffer;
  for (int i = 0; i < samplesPerChannel; i++) {
    float estimatedInstantaneousLoudness = 0;
    float energy = energyBuffer[i];
    double currentMeasurementConfidence = hLeveler->currentMeasurementConfidence;

    hLeveler->currentInstantaneousEnergy = LEAKY_INTEGRATOR(energy, hLeveler->currentInstantaneousEnergy, hLeveler->instantaneousLeakyIntegratorFactor);
    estimatedInstantaneousLoudness = NRG2LIN(hLeveler->currentInstantaneousEnergy);

    instantaneousLevelBuffer[i] = estimatedInstantaneousLoudness;

    currentMeasurementConfidence = LEAKY_INTEGRATOR((estimatedInstantaneousLoudness < GAMMA_A_LIN) ? 0 : 1, currentMeasurementConfidence, hLeveler->midTermLeakyIntegratorFactor);
    hLeveler->currentMeasurementConfidence = currentMeasurementConfidence;

    float estimatedMidTermLoudness = 0;
    if (hLeveler->settings.dynamicRangePreservation != 0.f) {
      float midTermEnergy = hLeveler->currentInstantaneousEnergy;
      if (estimatedInstantaneousLoudness < GAMMA_A_LIN) {
        midTermEnergy = hLeveler->initialMidTermEnergy;
      }

      hLeveler->currentMidTermEnergyAbsGated = LEAKY_INTEGRATOR(midTermEnergy, hLeveler->currentMidTermEnergyAbsGated, hLeveler->midTermLeakyIntegratorFactor);
      if (estimatedInstantaneousLoudness >= NRG2LIN(hLeveler->currentMidTermEnergyAbsGated) / GAMMA_R_DELTA_LIN) {
        hLeveler->currentMidTermEnergy = LEAKY_INTEGRATOR(midTermEnergy, hLeveler->currentMidTermEnergy, hLeveler->midTermLeakyIntegratorFactor);
      }
      estimatedMidTermLoudness = NRG2LIN(hLeveler->currentMidTermEnergy);

      midTermLevelBuffer[i] = estimatedMidTermLoudness;
      measurementConfidenceBuffer[i] = currentMeasurementConfidence;
    }
  }
}

static void s_gainSmoothing(IIS_LEVELER_LIB_HANDLE hLeveler, int samplesPerChannel, float *gainsOut) {
  float *instantaneousLevelBuffer = hLeveler->hInstantaneousLevelBuffer->buffer;
  float *roughGainBuffer = hLeveler->hRoughGainBuffer->buffer;
  for (int i = 0; i < samplesPerChannel; i++) {
    float attackFactor = hLeveler->gainSmoothAttackFastFactor;
    float decayFactor = hLeveler->gainSmoothDecaySlowFactor;
    float preGain = 0;

    hLeveler->totalNumberOfSamplesPerChannel++;

    if (hLeveler->totalNumberOfSamplesPerChannel > (long long)hLeveler->delayInSamples) {
      if (hLeveler->settings.silenceDetectionEnabled && instantaneousLevelBuffer[i] < hLeveler->settings.silenceLevelThreshold) {
        preGain = hLeveler->prevSmoothPreGain;
      } else {
        assert(samplesPerChannel - i - 1 < (int)hLeveler->hRoughGainBuffer->size);
        preGain = roughGainBuffer[i];
        if (hLeveler->settings.adaptiveAttack != 0.f && preGain > 0) {
          float x = hLeveler->prevSmoothPreGain / preGain;
          float x_max = 3.f;
          if (x <= x_max) {
            attackFactor = hLeveler->gainSmoothAttackSlowFactor;
          } else {
            attackFactor = hLeveler->gainSmoothAttackFastFactor;
          }
        }

        if (preGain > hLeveler->maxGain) {
          preGain = hLeveler->maxGain;
        }
      }
    } else {
      preGain = hLeveler->prevSmoothPreGain;
    }

    float smoothPreGain = 0;
    float fac = 0;

    if (preGain > hLeveler->prevSmoothPreGain) {
      fac = decayFactor;
    } else {
      fac = attackFactor;
    }
    smoothPreGain = LEAKY_INTEGRATOR(preGain, hLeveler->prevSmoothPreGain, fac);

    hLeveler->prevSmoothPreGain = smoothPreGain;

    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      gainsOut[i * hLeveler->config.numberOfChannels + ch] = smoothPreGain;
    }
  }
}

static void s_gainRamping(IIS_LEVELER_LIB_HANDLE hLeveler, int samplesPerChannel, float *gainsOut) {
  for (int i = 0; i < samplesPerChannel; i++) {
    float rampFactor = (float)hLeveler->rampSampleCount / (float)hLeveler->rampSampleNumber;
    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      gainsOut[i * hLeveler->config.numberOfChannels + ch] = rampFactor * gainsOut[i * hLeveler->config.numberOfChannels + ch] + (1.f - rampFactor);
    }
    switch (hLeveler->state) {
      case IIS_LEVELER_LIB_STATE_INITIALIZED:
        assert(0);
        break;

      case IIS_LEVELER_LIB_STATE_RAMPING_UP:
        if (hLeveler->rampSampleCount < hLeveler->rampSampleNumber) {
          hLeveler->rampSampleCount++;
        } else {
          hLeveler->state = IIS_LEVELER_LIB_STATE_ON;
          return;
        }
        break;
      case IIS_LEVELER_LIB_STATE_RAMPING_DOWN:
        if (hLeveler->rampSampleCount > 0) {
          hLeveler->rampSampleCount--;
        } else {
          hLeveler->state = IIS_LEVELER_LIB_STATE_OFF;
        }
        break;
      case IIS_LEVELER_LIB_STATE_ON:
      case IIS_LEVELER_LIB_STATE_OFF:
        break;
    }
  }
}

static void s_computeRoughGains(IIS_LEVELER_LIB_HANDLE hLeveler, int samplesPerChannel) {
  float *midTermLevelBuffer = hLeveler->hMidTermLevelBuffer->buffer;
  float *instantaneousLevelBuffer = hLeveler->hInstantaneousLevelBuffer->buffer;
  float *measurementConfidenceBuffer = hLeveler->hMeasurementConfidenceBuffer->buffer;
  float *roughGainBuffer = hLeveler->hRoughGainBuffer->buffer;
  if (hLeveler->state == IIS_LEVELER_LIB_STATE_RAMPING_DOWN) {
    for (int i = 0; i < samplesPerChannel; i++) {
      roughGainBuffer[i] = 1.f;
    }
  } else {
    for (int i = 0; i < samplesPerChannel; i++) {
      float estimatedInstantaneousLoudness = 0;
      float estimatedMidTermLoudness = 0;

      float preGainInstantaneous = 1.f;
      float preGainMidTerm = 1.f;

      float preGain = 1.f;

      assert(samplesPerChannel - i - 1 < (int)hLeveler->hInstantaneousLevelBuffer->size);
      estimatedInstantaneousLoudness = instantaneousLevelBuffer[i];
      if (estimatedInstantaneousLoudness > 0) {
        preGainInstantaneous = hLeveler->settings.targetLoudnessLin / estimatedInstantaneousLoudness;
      }

      estimatedMidTermLoudness = midTermLevelBuffer[i];
      if (estimatedMidTermLoudness > 0) {
        preGainMidTerm = hLeveler->settings.targetLoudnessLin / estimatedMidTermLoudness;
      }

      if (preGainInstantaneous > preGainMidTerm) {
        float measurementConfidence = measurementConfidenceBuffer[i];
        preGain = hLeveler->settings.dynamicRangePreservationFac * measurementConfidence * preGainMidTerm + (1.f - hLeveler->settings.dynamicRangePreservationFac * measurementConfidence) * preGainInstantaneous;
      } else {
        preGain = preGainInstantaneous;
      }

      preGain = hLeveler->correctionFactor * preGain * hLeveler->preGainGatedLongTermLoudness;

      roughGainBuffer[i] = preGain;
    }
  }
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Process(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int samplesPerChannel,
    const float *audioIn,
    float *out,
    float currentLoudness) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == audioIn || NULL == out) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }
  if (audioIn == out) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }
  if (samplesPerChannel > hLeveler->config.maxSamplesPerChannel) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  if (hLeveler->state == IIS_LEVELER_LIB_STATE_INITIALIZED) {
    iisLevelerLib_Set_enabled(hLeveler, 1);
  }
  if (hLeveler->state != IIS_LEVELER_LIB_STATE_OFF) {
    if (hLeveler->settings.loudnessComplianceStageEnabled && currentLoudness > hLeveler->settings.targetLoudness - 2 * MAX_GATED_LONG_TERM_LOUDNESS_ERROR) {
      float overcompensationFactor = 2.f;
      float gatedLongTermLoudnessError = -overcompensationFactor * (hLeveler->settings.targetLoudness - currentLoudness);
      if (gatedLongTermLoudnessError > MAX_GATED_LONG_TERM_LOUDNESS_ERROR) {
        gatedLongTermLoudnessError = MAX_GATED_LONG_TERM_LOUDNESS_ERROR;
      } else if (gatedLongTermLoudnessError < MIN_GATED_LONG_TERM_LOUDNESS_ERROR) {
        gatedLongTermLoudnessError = MIN_GATED_LONG_TERM_LOUDNESS_ERROR;
      }
      float currentLoudnessSmoothFactor = LEAKY_INTEGRATOR_FACTOR(CURRENT_LOUDNESS_SMOOTH_TIME_CONSTANT, ((float)hLeveler->config.samplingRate / (float)samplesPerChannel));
      hLeveler->preGainGatedLongTermLoudness = LEAKY_INTEGRATOR(DB2LIN(-gatedLongTermLoudnessError), hLeveler->preGainGatedLongTermLoudness, currentLoudnessSmoothFactor);
    } else {
      hLeveler->preGainGatedLongTermLoudness = 1.0;
    }

    s_feedEnergyBuffer(hLeveler, audioIn, samplesPerChannel);
    s_computeLevels(hLeveler, samplesPerChannel);
    s_computeRoughGains(hLeveler, samplesPerChannel);
    s_gainSmoothing(hLeveler, samplesPerChannel, out);

    s_RingBuffer_Update(hLeveler->hEnergyBuffer, samplesPerChannel);

    if (hLeveler->state == IIS_LEVELER_LIB_STATE_RAMPING_UP ||
        hLeveler->state == IIS_LEVELER_LIB_STATE_RAMPING_DOWN) {
      s_gainRamping(hLeveler, samplesPerChannel, out);
    }
  } else {
    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      for (int i = 0; i < samplesPerChannel; i++) {
        out[i * hLeveler->config.numberOfChannels + ch] = 1.f;
      }
    }
  }

  if (IIS_LEVELER_LIB_OUTPUT_FORMAT_SAMPLES == hLeveler->config.outputFormat) {
    float *frameBuffer = hLeveler->hFrameBuffer->buffer;
    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      for (int i = 0; i < samplesPerChannel; i++) {
        frameBuffer[(i + hLeveler->delayInSamples) * hLeveler->config.numberOfChannels + ch] = audioIn[i * hLeveler->config.numberOfChannels + ch];

        frameBuffer[i * hLeveler->config.numberOfChannels + ch] *= out[i * hLeveler->config.numberOfChannels + ch];
      }
    }

    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      for (int i = 0; i < samplesPerChannel; i++) {
        out[i * hLeveler->config.numberOfChannels + ch] = frameBuffer[i * hLeveler->config.numberOfChannels + ch];
      }
    }

    s_RingBuffer_Update(hLeveler->hFrameBuffer, samplesPerChannel * hLeveler->config.numberOfChannels);
  } else if (IIS_LEVELER_LIB_OUTPUT_FORMAT_GAINS_DB == hLeveler->config.outputFormat) {
    for (int ch = 0; ch < hLeveler->config.numberOfChannels; ch++) {
      for (int i = 0; i < samplesPerChannel; i++) {
        out[i * hLeveler->config.numberOfChannels + ch] = LIN2DB(out[i * hLeveler->config.numberOfChannels + ch]);
      }
    }
  } else if (IIS_LEVELER_LIB_OUTPUT_FORMAT_GAINS_LINEAR == hLeveler->config.outputFormat) {
  }

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_state(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    IIS_LEVELER_LIB_STATE *state) {
  if (NULL == hLeveler || NULL == state) {
    return IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
  }

  *state = hLeveler->state;

  return IIS_LEVELER_LIB_RETURN_NO_ERROR;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_enabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int *enabled) {
  if (NULL == hLeveler || NULL == enabled) {
    return IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
  }
  switch (hLeveler->state) {
    case IIS_LEVELER_LIB_STATE_ON:
    case IIS_LEVELER_LIB_STATE_RAMPING_UP:
      *enabled = 1;
      break;
    case IIS_LEVELER_LIB_STATE_OFF:
    case IIS_LEVELER_LIB_STATE_RAMPING_DOWN:
    default:
      *enabled = 0;
      break;
  }

  return IIS_LEVELER_LIB_RETURN_NO_ERROR;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_enabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int enable) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  if (enable) {
    switch (hLeveler->state) {
      case IIS_LEVELER_LIB_STATE_INITIALIZED:
        hLeveler->state = IIS_LEVELER_LIB_STATE_ON;
        hLeveler->rampSampleCount = hLeveler->rampSampleNumber;
        break;
      case IIS_LEVELER_LIB_STATE_OFF:
        iisLevelerLib_Reset(hLeveler);
        hLeveler->rampSampleCount = 0;

      case IIS_LEVELER_LIB_STATE_RAMPING_DOWN:
        hLeveler->state = IIS_LEVELER_LIB_STATE_RAMPING_UP;
        break;
      case IIS_LEVELER_LIB_STATE_ON:
      case IIS_LEVELER_LIB_STATE_RAMPING_UP:

        break;
    }
  }

  if (!enable) {
    switch (hLeveler->state) {
      case IIS_LEVELER_LIB_STATE_INITIALIZED:
        hLeveler->state = IIS_LEVELER_LIB_STATE_OFF;
        hLeveler->rampSampleCount = 0;
        break;
      case IIS_LEVELER_LIB_STATE_ON:
        hLeveler->rampSampleCount = hLeveler->rampSampleNumber;

      case IIS_LEVELER_LIB_STATE_RAMPING_UP:
        hLeveler->state = IIS_LEVELER_LIB_STATE_RAMPING_DOWN;
        break;
      case IIS_LEVELER_LIB_STATE_OFF:
      case IIS_LEVELER_LIB_STATE_RAMPING_DOWN:

        break;
    }
  }

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_relativeMaxGain(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float relativeMaxGain) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  float previousRelativeMaxGain = hLeveler->settings.relativeMaxGain;

  if (relativeMaxGain < IIS_LEVELER_LIB_MIN_RELATIVE_MAX_GAIN ||
      relativeMaxGain > IIS_LEVELER_LIB_MAX_RELATIVE_MAX_GAIN) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  hLeveler->settings.relativeMaxGain = relativeMaxGain;
  hLeveler->settings.relativeMaxGainLin = DB2LIN(relativeMaxGain);
  hLeveler->settings.silenceLevelThreshold = 1.f / DB2LIN(relativeMaxGain);

  iisLevelerLib_Set_targetLoudness(hLeveler, hLeveler->settings.targetLoudness);

  if (0 != memcmp(&relativeMaxGain, &previousRelativeMaxGain, sizeof(float))) {
    iisLevelerLib_Reset(hLeveler);
  }

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_targetLoudness(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float targetLoudness) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;
  float maxGainDB = 0.f;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  float previousTargetLoudness = hLeveler->settings.targetLoudness;

  if (targetLoudness < IIS_LEVELER_LIB_MIN_TARGET_LOUDNESS ||
      targetLoudness > IIS_LEVELER_LIB_MAX_TARGET_LOUDNESS) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  maxGainDB = targetLoudness + hLeveler->settings.relativeMaxGain;
  hLeveler->settings.targetLoudness = targetLoudness;
  hLeveler->settings.targetLoudnessLin = DB2LIN(targetLoudness);

  hLeveler->maxGain = DB2LIN(maxGainDB);

  hLeveler->intermediateTargetLoudnessDeltaLin = DB2LIN(hLeveler->settings.targetLoudness - INTERMEDIATE_TARGET_LOUDNESS);

  if (0 != memcmp(&targetLoudness, &previousTargetLoudness, sizeof(float))) {
    iisLevelerLib_Reset(hLeveler);
  }

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_loudnessComplianceStageEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int loudnessComplianceStageEnabled) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  hLeveler->settings.loudnessComplianceStageEnabled = !!loudnessComplianceStageEnabled;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_silenceDetectionEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int silenceDetectionEnabled) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  hLeveler->settings.silenceDetectionEnabled = !!silenceDetectionEnabled;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_adaptiveAttack(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float adaptiveAttack) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  if (adaptiveAttack < 0.f || adaptiveAttack > 1.f) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  hLeveler->settings.adaptiveAttack = adaptiveAttack;

  {
    float timeConstant = LERP(0, GAIN_SMOOTHING_ATTACK_FAST_TIME_CONSTANT, 1, GAIN_SMOOTHING_ATTACK_SLOW_TIME_CONSTANT, EXRP(adaptiveAttack, 1.f));
    hLeveler->gainSmoothAttackSlowFactor = LEAKY_INTEGRATOR_FACTOR(timeConstant, hLeveler->config.samplingRate);
  }

  hLeveler->correctionFactor = DB2LIN(3.f + ((1.f - hLeveler->settings.adaptiveAttack) * 2.5f));

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_dynamicRangePreservation(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float dynamicRangePreservation) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  if (dynamicRangePreservation < 0.f || dynamicRangePreservation > 1.f) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_INVALID_ARGUMENT;
    goto error;
  }

  if ((hLeveler->settings.dynamicRangePreservation == 0.f && dynamicRangePreservation != 0.f) ||
      (hLeveler->settings.dynamicRangePreservation != 0.f && dynamicRangePreservation == 0.f)) {
    iisLevelerLib_Reset(hLeveler);
  }

  hLeveler->settings.dynamicRangePreservation = dynamicRangePreservation;
  hLeveler->settings.dynamicRangePreservationFac = LORP(dynamicRangePreservation, 100.f);

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_relativeMaxGain(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float *relativeMaxGain) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == relativeMaxGain) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *relativeMaxGain = hLeveler->settings.relativeMaxGain;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_loudnessComplianceStageEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int *loudnessComplianceStageEnabled) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == loudnessComplianceStageEnabled) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *loudnessComplianceStageEnabled = hLeveler->settings.loudnessComplianceStageEnabled;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_silenceDetectionEnabled(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    int *silenceDetectionEnabled) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == silenceDetectionEnabled) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *silenceDetectionEnabled = hLeveler->settings.silenceDetectionEnabled;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_adaptiveAttack(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float *adaptiveAttack) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == adaptiveAttack) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *adaptiveAttack = hLeveler->settings.adaptiveAttack;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_dynamicRangePreservation(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float *dynamicRangePreservation) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == dynamicRangePreservation) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *dynamicRangePreservation = hLeveler->settings.dynamicRangePreservation;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Get_targetLoudness(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    float *targetLoudness) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == targetLoudness) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  *targetLoudness = hLeveler->settings.targetLoudness;

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_channelWeights(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    const float channelWeights[IIS_LEVELER_LIB_MAX_CHANNELS]) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == channelWeights) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  for (int i = 0; i < IIS_LEVELER_LIB_MAX_CHANNELS; i++) {
    hLeveler->settings.channelWeights[i] = channelWeights[i];
    hLeveler->linearChannelWeights[i] = DB2NRG(channelWeights[i]);
  }

  iisLevelerLib_Reset(hLeveler);

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

IIS_LEVELER_LIB_RETURN
iisLevelerLib_Set_ignoreChannel(
    IIS_LEVELER_LIB_HANDLE hLeveler,
    const int ignoreChannel[IIS_LEVELER_LIB_MAX_CHANNELS]) {
  IIS_LEVELER_LIB_RETURN returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNKNOWN;

  if (NULL == hLeveler || NULL == ignoreChannel) {
    returnValue = IIS_LEVELER_LIB_RETURN_ERROR_UNEXPECTED_NULL_POINTER;
    goto error;
  }

  for (int i = 0; i < IIS_LEVELER_LIB_MAX_CHANNELS; i++) {
    hLeveler->settings.ignoreChannel[i] = ignoreChannel[i];
  }

  iisLevelerLib_Reset(hLeveler);

  returnValue = IIS_LEVELER_LIB_RETURN_NO_ERROR;

error:

  return returnValue;
}

static void setKweightFilt(BIQUAD_COEFF *preFilt, BIQUAD_COEFF *rlbFilt, unsigned int fs) {
  float w0, A, alpha, sinw0, cosw0, sqrtA;
  float b0, b1, b2, a0, a1, a2;

  w0 = 2 * PI * 1500 / fs;
  sinw0 = (float)sin(w0);
  cosw0 = (float)cos(w0);
  A = 1.25892541f;
  sqrtA = (float)sqrt(A);
  alpha = (float)(sinw0 * 0.5 * sqrt(2));

  b0 = A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
  b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
  b2 = A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
  a0 = (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
  a1 = 2 * ((A - 1) - (A + 1) * cosw0);
  a2 = (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;

  preFilt->b0 = b0 / a0;
  preFilt->b1 = b1 / a0;
  preFilt->b2 = b2 / a0;
  preFilt->a1 = a1 / a0;
  preFilt->a2 = a2 / a0;

  w0 = 2 * PI * 38 / fs;
  sinw0 = (float)sin(w0);
  cosw0 = (float)cos(w0);
  alpha = sinw0;

  b0 = (1 + cosw0) / 2;
  b1 = -(1 + cosw0);
  b2 = (1 + cosw0) / 2;
  a0 = 1 + alpha;
  a1 = -2 * cosw0;
  a2 = 1 - alpha;

  rlbFilt->b0 = b0 / a0;
  rlbFilt->b1 = b1 / a0;
  rlbFilt->b2 = b2 / a0;
  rlbFilt->a1 = a1 / a0;
  rlbFilt->a2 = a2 / a0;
}
