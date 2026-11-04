
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

#define DOG_NUM_ITERATIONS 3

#define DOG_LOUD_MIN -150.0f

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "iisDRCOfflineGain_api.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct s_loudness_info {
  float I;
  float LRA;
  float lo;
  float hi;
  float I_ungated;
  float LRA_ungated;
  float I_dialog;
  float I_dialog_percentage;
  float I_anchor;
  float LRA_dialog;
  float lo_dialog;
  float hi_dialog;
} DOG_LOUDNESS_INFO;

static float
_getGain(
    const DOG_DRC_CHARACTERISTIC_NODES* pDrcCharNodes,
    const float inLevelDb) {
  unsigned int n;
  float w;
  const float* nodeLevel;
  const float* nodeGain;

  if (pDrcCharNodes == NULL) return 0.0f;
  if (pDrcCharNodes->nodeCount > DOG_MAX_DRC_CHAR_NODES) return 0.0f;
  if (pDrcCharNodes->nodeCount < 1) return 0.0f;

  nodeLevel = pDrcCharNodes->nodeLevel;
  nodeGain = pDrcCharNodes->nodeGain;

  if (inLevelDb < nodeLevel[0])
    return nodeGain[0];

  for (n = 0; n < (pDrcCharNodes->nodeCount - 1); n++) {
    if ((inLevelDb >= nodeLevel[n]) && (inLevelDb < nodeLevel[n + 1])) {
      w = (nodeLevel[n + 1] - inLevelDb) / (nodeLevel[n + 1] - nodeLevel[n]);
      return (w * nodeGain[n] + (1.0f - w) * nodeGain[n + 1]);
    }
  }

  return nodeGain[pDrcCharNodes->nodeCount - 1];
}

static DOG_ERROR
_getGainArray(
    const DOG_DRC_CHARACTERISTIC_NODES* pDrcCharNodes,
    const unsigned int frameSize,
    const float* pInLevelDb,
    const float* pInGainDb,
    float* pOutGainDb) {
  unsigned int i;
  if (pInLevelDb == NULL) return DOG_NULL_POINTER;
  if (pOutGainDb == NULL) return DOG_NULL_POINTER;

  if (pInGainDb == NULL) {
    for (i = 0; i < frameSize; i++) {
      pOutGainDb[i] = _getGain(pDrcCharNodes, pInLevelDb[i]);
    }
  } else {
    for (i = 0; i < frameSize; i++) {
      pOutGainDb[i] = _getGain(pDrcCharNodes, pInLevelDb[i] + pInGainDb[i]);
    }
  }

  return DOG_OK;
}

static DOG_DRC_CHARACTERISTIC_NODES
_getCharNodes(
    const float lo_in,
    const float hi_in,
    const float lo_out,
    const float hi_out,
    const int firstStage) {
  DOG_DRC_CHARACTERISTIC_NODES drcCharNodes = {0};
  float gain_lo, gain_hi;
  float gain_diff, in_diff;
  unsigned int n = 0;

  gain_lo = lo_out - lo_in;
  gain_hi = hi_out - hi_in;

  gain_diff = gain_hi - gain_lo;
  in_diff = hi_in - lo_in;

  if (firstStage) {
    drcCharNodes.nodeLevel[n] = lo_in - 10.0f;
    drcCharNodes.nodeGain[n] = 0.0f;
    n++;
  }

  drcCharNodes.nodeLevel[n] = lo_in;
  drcCharNodes.nodeGain[n] = gain_lo;
  n++;
  drcCharNodes.nodeLevel[n] = 0;
  drcCharNodes.nodeGain[n] = gain_lo + (0 - lo_in) / in_diff * gain_diff;
  n++;

  drcCharNodes.nodeCount = n;

  return drcCharNodes;
}

static DOG_ERROR
_addLimiterSection(
    DOG_DRC_CHARACTERISTIC_NODES* pDrcCharNodes,
    const float externalLoudnessOffset) {
  unsigned int n;
  const float limiterHeadroom = 3.0f + externalLoudnessOffset;
  const float limiterTargetLoudness = -16.0f;
  float A = -limiterHeadroom - (limiterTargetLoudness - (-31.0f));

  if (pDrcCharNodes->nodeCount < 2)
    return DOG_NOT_OK;

  {
    float level0gain = _getGain(pDrcCharNodes, 0.0f);

    if (pDrcCharNodes->nodeLevel[0] >= 0.0f)
      return DOG_OK;

    if (pDrcCharNodes->nodeGain[0] > (A - pDrcCharNodes->nodeLevel[0]))
      pDrcCharNodes->nodeGain[0] = (A - pDrcCharNodes->nodeLevel[0]);

    for (n = 1; n < pDrcCharNodes->nodeCount; n++) {
      if (pDrcCharNodes->nodeLevel[n] <= pDrcCharNodes->nodeLevel[n - 1])
        return DOG_NOT_OK;
      if (pDrcCharNodes->nodeLevel[n] >= 0.0f)
        break;
    }

    pDrcCharNodes->nodeCount = n + 1;
    if (pDrcCharNodes->nodeCount > DOG_MAX_DRC_CHAR_NODES)
      return DOG_NOT_OK;
    pDrcCharNodes->nodeLevel[n] = 0.0f;
    pDrcCharNodes->nodeGain[n] = level0gain;
  }

  for (n = 1; n < pDrcCharNodes->nodeCount; n++) {
    float c, intersectionLevel, intersectionGain;

    if (pDrcCharNodes->nodeGain[n] < (A - pDrcCharNodes->nodeLevel[n]))
      continue;

    c = (pDrcCharNodes->nodeGain[n] - pDrcCharNodes->nodeGain[n - 1]) / (pDrcCharNodes->nodeLevel[n] - pDrcCharNodes->nodeLevel[n - 1]);

    intersectionLevel = (A + c * pDrcCharNodes->nodeLevel[n - 1] - pDrcCharNodes->nodeGain[n - 1]) / (1 + c);
    if (intersectionLevel > pDrcCharNodes->nodeLevel[n])
      return DOG_NOT_OK;
    intersectionGain = A - intersectionLevel;

    pDrcCharNodes->nodeLevel[n] = intersectionLevel;
    pDrcCharNodes->nodeGain[n] = intersectionGain;
    n++;

    pDrcCharNodes->nodeCount = n + 1;
    if (pDrcCharNodes->nodeCount > DOG_MAX_DRC_CHAR_NODES)
      return DOG_NOT_OK;
    pDrcCharNodes->nodeLevel[n] = 0.0f;
    pDrcCharNodes->nodeGain[n] = A;
    break;
  }

  return DOG_OK;
}

static DOG_DRC_CHARACTERISTIC_NODES
_getLimiterNodes(const float loudnessValue, const float externalLoudnessOffset) {
  DOG_DRC_CHARACTERISTIC_NODES drcCharNodes = {0};
  unsigned int n = 0;
  const float limiterHeadroom = 6.0f + externalLoudnessOffset;
  const float limiterTargetLoudness = -16.0f;
  float A = -limiterHeadroom - (limiterTargetLoudness - loudnessValue);

  drcCharNodes.nodeLevel[n] = A;
  drcCharNodes.nodeGain[n] = 0.0f;
  n++;

  drcCharNodes.nodeLevel[n] = 0.0f;
  drcCharNodes.nodeGain[n] = A;
  n++;

  drcCharNodes.nodeCount = n;

  return drcCharNodes;
}

static DOG_ERROR
_rectangularWindow(
    const unsigned int sigLength,
    const unsigned int windowLength,
    float* inOutBuffer) {
  unsigned int i;
  float out_prev;
  float in_delayline[DOG_MAX_WINDOW_LENGTH + 1];
  unsigned int in_delayindex;

  if (inOutBuffer == NULL) return DOG_NULL_POINTER;
  if (windowLength > DOG_MAX_WINDOW_LENGTH) return DOG_NOT_OK;
  if (windowLength == 1) return DOG_OK;

  in_delayline[0] = inOutBuffer[0];
  in_delayindex = 1;
  inOutBuffer[0] = inOutBuffer[0] / (float)windowLength;
  out_prev = inOutBuffer[0];

  for (i = 1; i < windowLength; i++) {
    in_delayline[in_delayindex] = inOutBuffer[i];
    in_delayindex++;
    in_delayindex %= (windowLength + 1);
    inOutBuffer[i] = out_prev + inOutBuffer[i] / (float)windowLength;
    out_prev = inOutBuffer[i];
  }
  for (; i < sigLength + 2 * (windowLength - 1); i++) {
    in_delayline[in_delayindex] = inOutBuffer[i];
    in_delayindex++;
    in_delayindex %= (windowLength + 1);
    inOutBuffer[i] = out_prev + (inOutBuffer[i] - in_delayline[in_delayindex]) / (float)windowLength;
    out_prev = inOutBuffer[i];
  }

  return DOG_OK;
}

static DOG_ERROR
_slidingWindow(
    const unsigned int sigLength,
    const unsigned int windowLength,
    float* gainBuffer) {
  DOG_ERROR err = DOG_OK;
  unsigned int i;

  if (windowLength <= 1) return DOG_OK;
  if (windowLength > DOG_MAX_WINDOW_LENGTH) return DOG_NOT_OK;

  memmove(gainBuffer + (windowLength - 1), gainBuffer, sigLength * sizeof(float));
  for (i = 0; i < windowLength - 1; i++) {
    gainBuffer[(windowLength - 1) - i - 1] = gainBuffer[(windowLength - 1) + i];
    gainBuffer[(windowLength - 1) + sigLength + i] = gainBuffer[(windowLength - 1) + sigLength - i - 1];
  }

  err = _rectangularWindow(sigLength, windowLength, gainBuffer);
  if (err) return err;

  err = _rectangularWindow(sigLength, windowLength, gainBuffer);
  if (err) return err;

  memmove(gainBuffer, gainBuffer + 2 * (windowLength - 1), sigLength * sizeof(float));
  memset(gainBuffer + sigLength, 0, 2 * (windowLength - 1) * sizeof(float));

  return DOG_OK;
}

static DOG_ERROR
_accumulateBuffer(
    const unsigned int sigLength,
    const int sign,
    const float* sourceBuffer,
    float* targetBuffer) {
  unsigned int i;
  float signFactor = (sign ? -1.0f : 1.0f);
  if (sourceBuffer == NULL) return DOG_NULL_POINTER;
  if (targetBuffer == NULL) return DOG_NULL_POINTER;

  for (i = 0; i < sigLength; i++) {
    targetBuffer[i] += signFactor * sourceBuffer[i];
  }
  return DOG_OK;
}

static int _compare(const void* a, const void* b) {
  if (*(const float*)a < *(const float*)b)
    return -1;
  return *(const float*)a > *(const float*)b;
}

static float s_adjustAnchorLoudness(float integratedLoudness,
                                    float speechGatedLoudness,
                                    float speechPercentage,
                                    float lra) {
  const float fallbackBorder = 10.0f;
  const float speechBorder = 15.0f;

  float loudnessOffsetSpeech = integratedLoudness - speechGatedLoudness;
  const float c = 0.2f;
  float loudnessOffsetFallback = c * lra;

  const float maxLoudnessOffsetSpeech = 8.0f;
  const float maxLoudnessOffsetFallback = 6.0f;
  const float minLoudnessOffset = 0.0f;

  float loudnessOffset = 0.0f;

  if (loudnessOffsetSpeech > maxLoudnessOffsetSpeech) {
    loudnessOffsetSpeech = maxLoudnessOffsetSpeech;
  }
  if (loudnessOffsetSpeech < minLoudnessOffset) {
    loudnessOffsetSpeech = minLoudnessOffset;
  }
  if (loudnessOffsetFallback > maxLoudnessOffsetFallback) {
    loudnessOffsetFallback = maxLoudnessOffsetFallback;
  }
  if (loudnessOffsetFallback < minLoudnessOffset) {
    loudnessOffsetFallback = minLoudnessOffset;
  }

  if (speechPercentage <= fallbackBorder) {
    loudnessOffset = loudnessOffsetFallback;
  } else if (speechPercentage >= speechBorder) {
    loudnessOffset = loudnessOffsetSpeech;
  } else {
    float interpolation_ratio = (speechPercentage - fallbackBorder) / (speechBorder - fallbackBorder);
    assert(interpolation_ratio < 1.0f);
    assert(interpolation_ratio > 0.0f);
    loudnessOffset = (1.0f - interpolation_ratio) * loudnessOffsetFallback + interpolation_ratio * loudnessOffsetSpeech;
  }

  return integratedLoudness - loudnessOffset;
}

static DOG_LOUDNESS_INFO _calculateLoudness(
    const float* InstL,
    const float* gain,
    const unsigned char* vaBuffer,
    const unsigned int sigLength,
    float* tmpBuffer) {
  unsigned int i, runLength;
  float i_u = 0.0f, i_g = 0.0f, i_d = 0.0f;
  float* nrgMom = NULL;
  float* S = NULL;
  float t = 0.0f;
  float t2 = 0.0f;
  int counter = 0, absgatedCnt;
  DOG_LOUDNESS_INFO Loudness;
  const double dBtoNrgFactor = log(10.0) / 10.0;
  const float abs_thres = (float)exp(dBtoNrgFactor * (-70.0 + 0.691));
  float rel_thres = 0.0f;
  unsigned int location = 0;
  float lo_ungated, hi_ungated;

  memset(&Loudness, 0, sizeof(Loudness));
  Loudness.I = DOG_LOUD_MIN;
  Loudness.I_ungated = DOG_LOUD_MIN;
  Loudness.I_dialog = DOG_LOUD_MIN;
  Loudness.I_anchor = DOG_LOUD_MIN;
  memset(tmpBuffer, 0, (sigLength + 2 * 30) * sizeof(float));

  nrgMom = tmpBuffer;

  if (gain == NULL) {
    for (i = 0; i < sigLength; i++) {
      nrgMom[i] = (float)exp(dBtoNrgFactor * (InstL[i] + 0.691));
    }
  } else {
    for (i = 0; i < sigLength; i++) {
      nrgMom[i] = (float)exp(dBtoNrgFactor * (InstL[i] + gain[i] + 0.691));
    }
  }

  for (i = 0; i < sigLength; i++) {
    i_u = i_u + nrgMom[i];
  }
  i_u /= (float)sigLength;
  if (i_u > 0.0f) {
    Loudness.I_ungated = (float)(-0.691 + 10.0 * log10(i_u));
  } else {
    return Loudness;
  }

  _rectangularWindow(sigLength, 4, nrgMom);

  nrgMom += 3;
  if (sigLength > 3)
    runLength = sigLength - 3;
  else
    runLength = 1;

  for (i = 0; i < runLength; i++) {
    if (nrgMom[i] >= abs_thres) {
      t = t + nrgMom[i];
      counter++;
    }
  }

  if (counter == 0) {
    return Loudness;
  }

  absgatedCnt = counter;

  t /= (float)counter;
  t = (float)(-0.691 + 10.0 * log10(t));
  if ((t - 10) > -70) {
    t = t - 10;
  } else {
    t = -70;
  }
  rel_thres = (float)exp(dBtoNrgFactor * (t + 0.691));

  counter = 0;
  for (i = 0; i < runLength; i++) {
    if (nrgMom[i] >= rel_thres) {
      i_g = i_g + nrgMom[i];
      counter++;
    }
  }
  if (counter) {
    i_g /= (float)counter;
    Loudness.I = (float)(-0.691 + 10.0 * log10(i_g));
  } else {
    return Loudness;
  }

  if (vaBuffer != NULL) {
    nrgMom -= 3;
    counter = 0;
    for (i = 0; i < sigLength; i++) {
      if (vaBuffer[i] && (nrgMom[i] >= abs_thres)) {
        i_d += nrgMom[i];
        counter++;
      }
    }
    if (counter) {
      i_d /= (float)counter;
      Loudness.I_dialog = (float)(-0.691 + 10.0 * log10(i_d));
      Loudness.I_dialog_percentage = (float)counter / (float)absgatedCnt * 100.0f;
    } else {
      Loudness.I_dialog = DOG_LOUD_MIN;
      Loudness.I_dialog_percentage = 0.0f;
    }
  }

  memset(tmpBuffer, 0, (sigLength + 2 * 30) * sizeof(float));
  S = tmpBuffer;

  if (gain == NULL) {
    for (i = 0; i < sigLength; i++) {
      S[i] = (float)exp(dBtoNrgFactor * (InstL[i] + 0.691));
    }
  } else {
    for (i = 0; i < sigLength; i++) {
      S[i] = (float)exp(dBtoNrgFactor * (InstL[i] + gain[i] + 0.691));
    }
  }

  _rectangularWindow(sigLength, 30, S);

  for (i = 0; i < sigLength; i++) {
    if (S[i] > 0.0f)
      S[i] = (float)(-0.691 + 10.0 * log10(S[i]));
    else
      S[i] = DOG_LOUD_MIN;
  }

  S += 29;
  if (sigLength > 29)
    runLength = sigLength - 29;
  else
    runLength = 1;

  counter = 0;
  for (i = 0; i < runLength; i++) {
    if (S[i] >= -70.0f) {
      t2 = t2 + (float)exp(dBtoNrgFactor * (S[i] + 0.691));
      counter++;
    }
  }
  if (counter == 0)
    return Loudness;

  t2 /= (float)counter;
  t2 = (float)(-0.691 + 10.0 * log10(t2));
  t2 = t2 - 20;

  qsort(S, runLength, sizeof(float), _compare);
  counter = 0;

  for (i = 0; i < runLength; i++) {
    if (S[i] < t2)
      counter++;
  }
  location = counter + (int)floor(0.1 * (double)(runLength - counter));
  Loudness.lo = S[location];
  location = counter + (int)floor(0.95 * (double)(runLength - counter));
  Loudness.hi = S[location];
  Loudness.LRA = Loudness.hi - Loudness.lo;

  location = (int)floor(0.1 * (double)runLength);
  lo_ungated = S[location];
  location = (int)floor(0.95 * (double)runLength);
  hi_ungated = S[location];
  Loudness.LRA_ungated = hi_ungated - lo_ungated;

  if (vaBuffer != NULL) {
    int dialogCounter;

    Loudness.I_anchor = s_adjustAnchorLoudness(Loudness.I, Loudness.I_dialog, Loudness.I_dialog_percentage, Loudness.LRA);

    memset(tmpBuffer, 0, (sigLength + 2 * 30) * sizeof(float));
    S = tmpBuffer;

    if (gain == NULL) {
      for (i = 0; i < sigLength; i++) {
        S[i] = (float)exp(dBtoNrgFactor * (InstL[i] + 0.691));
      }
    } else {
      for (i = 0; i < sigLength; i++) {
        S[i] = (float)exp(dBtoNrgFactor * (InstL[i] + gain[i] + 0.691));
      }
    }

    _rectangularWindow(sigLength, 30, S);

    for (i = 0; i < sigLength; i++) {
      if (S[i] > 0.0f)
        S[i] = (float)(-0.691 + 10.0 * log10(S[i]));
      else
        S[i] = DOG_LOUD_MIN;
    }

    dialogCounter = 0;
    for (i = 0; i < sigLength; i++) {
      if (vaBuffer[i]) {
        S[dialogCounter] = S[i];
        dialogCounter++;
      }
    }
    if (dialogCounter == 0)
      return Loudness;

    qsort(S, dialogCounter, sizeof(float), _compare);

    location = (int)floor(0.1 * (double)dialogCounter);
    Loudness.lo_dialog = S[location];
    location = (int)floor(0.95 * (double)dialogCounter);
    Loudness.hi_dialog = S[location];
    Loudness.LRA_dialog = Loudness.hi_dialog - Loudness.lo_dialog;
  }

  return Loudness;
}

static DOG_ERROR _gainScaling(
    const unsigned int sigLength,
    float* gain,
    const float attenuationScaling,
    const float amplificationScaling) {
  unsigned int i;
  for (i = 0; i < sigLength; i++) {
    if (gain[i] >= 0.0f)
      gain[i] *= amplificationScaling;
    else
      gain[i] *= attenuationScaling;
  }
  return DOG_OK;
}

DOG_CONFIG getDefaultDogConfig(void) {
  DOG_CONFIG defaultConfig;

  memset(&defaultConfig, 0, sizeof(DOG_CONFIG));

  defaultConfig.targetLRA = 10.0f;

  return defaultConfig;
}

DOG_ERROR
drcOfflineGain_process(
    const unsigned int sigLength,
    const DOG_CONFIG* dogConfig,
    DOG_DRC_CHARACTERISTIC_NODES* finalDrcChar,
    float* inOutBuffer) {
  return drcOfflineGain_process_va(sigLength, dogConfig, NULL, finalDrcChar, inOutBuffer);
}

DOG_ERROR
drcOfflineGain_process_va(
    const unsigned int sigLength,
    const DOG_CONFIG* dogConfig,
    const unsigned char* vaBuffer,
    DOG_DRC_CHARACTERISTIC_NODES* finalDrcChar,
    float* inOutBuffer) {
  DOG_ERROR err = DOG_OK;
  static int slidingWindowLengths[DOG_NUM_ITERATIONS] = {100, 10, 1};
  static float maxLraReduction[DOG_NUM_ITERATIONS] = {16.0f, 4.0f, 2.0f};
  float* gainBuffer = NULL;
  float* instlBuffer = NULL;
  float* tmpBuffer = NULL;
  DOG_LOUDNESS_INFO loudnessInfo, tmpLoudness;
  unsigned int i, n;
  float orig_I, processed_I;
  float LRA_reduction, lower_lo_in, usedTargetLRA;

  float gainOffset[DOG_NUM_GAIN_OFFSETS] = {0.0f, 0.0f, 0.0f};

  const float amplificationScaling[DOG_NUM_GAIN_OFFSETS] = {1.0f, 0.625f, 0.375f};
  const float attenuationScaling[DOG_NUM_GAIN_OFFSETS] = {1.0f, 1.0f, 0.5f};
  float inputLoudnessLevel;

  if ((dogConfig->targetLRA < 6.0f) || (dogConfig->targetLRA > 16.0f))
    return DOG_PARAM_OUT_OF_RANGE;

  if (dogConfig->alignDialogLoudness == DOG_ALIGN_INPUT_LOUDNESS) {
    if (dogConfig->inputLoudnessLevelAvailable == 0)
      return DOG_INVALID_PARAMETER_COMBINATION;
  }
  if (dogConfig->alignDialogLoudness == DOG_ALIGN_ANCHOR_LOUDNESS) {
    if (vaBuffer == NULL)
      return DOG_INVALID_PARAMETER_COMBINATION;
  }

  gainBuffer = calloc(sigLength + 2 * DOG_MAX_WINDOW_LENGTH, sizeof(float));
  if (gainBuffer == NULL) {
    err = DOG_MEMORY_ERROR;
    goto cleanup;
  }
  instlBuffer = calloc(sigLength + 2 * DOG_MAX_WINDOW_LENGTH, sizeof(float));
  if (instlBuffer == NULL) {
    err = DOG_MEMORY_ERROR;
    goto cleanup;
  }
  tmpBuffer = calloc(sigLength + 2 * DOG_MAX_WINDOW_LENGTH, sizeof(float));
  if (tmpBuffer == NULL) {
    err = DOG_MEMORY_ERROR;
    goto cleanup;
  }

  loudnessInfo = _calculateLoudness(inOutBuffer, NULL, vaBuffer, sigLength, tmpBuffer);
  if (dogConfig->alignDialogLoudness == DOG_ALIGN_ANCHOR_LOUDNESS) {
    orig_I = loudnessInfo.I_anchor;
  } else {
    orig_I = loudnessInfo.I;
  }
  if (dogConfig->inputLoudnessLevelAvailable) {
    inputLoudnessLevel = dogConfig->inputLoudnessLevel;
  } else {
    inputLoudnessLevel = orig_I;
  }

  usedTargetLRA = dogConfig->targetLRA;

  memcpy(instlBuffer, inOutBuffer, sigLength * sizeof(float));

  if (finalDrcChar != NULL) {
    memset(finalDrcChar, 0, sizeof(DOG_DRC_CHARACTERISTIC_NODES));
    finalDrcChar->nodeCount = 2;
    finalDrcChar->nodeLevel[0] = -100.0f;
    finalDrcChar->nodeLevel[1] = 0.0f;
  }

  for (i = 0; i < DOG_NUM_ITERATIONS; i++) {
    DOG_DRC_CHARACTERISTIC_NODES drcCharNodes;
    float prevLRA = loudnessInfo.LRA;
    float lowBorder = min(-70.0f, loudnessInfo.lo - 3.0f);
    memset(&drcCharNodes, 0, sizeof(DOG_DRC_CHARACTERISTIC_NODES));
    LRA_reduction = loudnessInfo.LRA - usedTargetLRA;

    if (LRA_reduction < 0) break;

    LRA_reduction = min(LRA_reduction, maxLraReduction[i]);

    for (lower_lo_in = loudnessInfo.lo; lower_lo_in > lowBorder; lower_lo_in -= 1.0f) {
      drcCharNodes = _getCharNodes(lower_lo_in, loudnessInfo.hi, loudnessInfo.lo + LRA_reduction / 2.0f, loudnessInfo.hi - LRA_reduction / 2.0f, i == 0);

      err = _getGainArray(&drcCharNodes, sigLength, instlBuffer, NULL, gainBuffer);
      if (err) goto cleanup;

      tmpLoudness = _calculateLoudness(instlBuffer, gainBuffer, vaBuffer, sigLength, tmpBuffer);
      if (tmpLoudness.LRA <= (loudnessInfo.LRA - LRA_reduction) + 0.5f) break;
      if (tmpLoudness.LRA >= prevLRA) break;
      prevLRA = tmpLoudness.LRA;
    }

    err = _slidingWindow(sigLength, slidingWindowLengths[i], gainBuffer);
    if (err) goto cleanup;

    tmpLoudness = _calculateLoudness(instlBuffer, gainBuffer, vaBuffer, sigLength, tmpBuffer);
    if ((tmpLoudness.LRA < loudnessInfo.LRA) || (tmpLoudness.LRA_ungated < loudnessInfo.LRA_ungated)) {
      if ((i == (DOG_NUM_ITERATIONS - 1)) && (finalDrcChar != NULL) && !dogConfig->internalFinalDrcStage) {
        *finalDrcChar = drcCharNodes;
      } else {
        err = _accumulateBuffer(sigLength, 0, gainBuffer, instlBuffer);
        if (err) goto cleanup;
      }

      loudnessInfo = tmpLoudness;
    }
  }

  if (dogConfig->alignDialogLoudness == DOG_ALIGN_INPUT_LOUDNESS) {
    gainOffset[0] = inputLoudnessLevel - orig_I;
    gainOffset[1] = amplificationScaling[1] * gainOffset[0];
    gainOffset[2] = amplificationScaling[2] * gainOffset[0];

    for (n = 0; n < DOG_NUM_GAIN_OFFSETS; n++) {
      gainOffset[n] = min(gainOffset[n], 8.0f);
      gainOffset[n] = max(gainOffset[n], -8.0f);
    }
  }

  if (dogConfig->alignDialogLoudness == DOG_ALIGN_ANCHOR_LOUDNESS) {
    processed_I = loudnessInfo.I_anchor;
  } else {
    processed_I = loudnessInfo.I;
  }

  for (n = 0; n < sigLength; n++) {
    instlBuffer[n] += orig_I - processed_I;
  }

  if (finalDrcChar != NULL) {
    if (!dogConfig->internalFinalDrcStage) {
      for (n = 0; n < finalDrcChar->nodeCount; n++) {
        finalDrcChar->nodeLevel[n] += orig_I - processed_I;

        finalDrcChar->nodeLevel[n] += -31.0f + 3.0f - max(-70.0f, inputLoudnessLevel);
      }
      err = _addLimiterSection(finalDrcChar, gainOffset[0]);
      if (err) goto cleanup;
    }
  }

  err = _accumulateBuffer(sigLength, 1, inOutBuffer, instlBuffer);
  if (err) goto cleanup;

  memcpy(gainBuffer, instlBuffer, sigLength * sizeof(float));

  if (dogConfig->alignDialogLoudness != DOG_ALIGN_INPUT_LOUDNESS) {
    for (n = 1; n < DOG_NUM_GAIN_OFFSETS; n++) {
      err = _gainScaling(sigLength, instlBuffer, attenuationScaling[n], amplificationScaling[n]);
      if (err) goto cleanup;

      tmpLoudness = _calculateLoudness(inOutBuffer, instlBuffer, vaBuffer, sigLength, tmpBuffer);
      if (dogConfig->alignDialogLoudness == DOG_ALIGN_ANCHOR_LOUDNESS) {
        gainOffset[n] = orig_I - tmpLoudness.I_anchor;
      } else {
        gainOffset[n] = orig_I - tmpLoudness.I;
      }

      gainOffset[n] = min(gainOffset[n], 8.0f);
      gainOffset[n] = max(gainOffset[n], -8.0f);

      memcpy(instlBuffer, gainBuffer, sigLength * sizeof(float));
    }
  }

  if (finalDrcChar != NULL) {
    for (n = 0; n < DOG_NUM_GAIN_OFFSETS; n++) {
      finalDrcChar->gainOffset[n] = gainOffset[n];
    }
  }

  if (dogConfig->internalFinalDrcStage) {
    DOG_DRC_CHARACTERISTIC_NODES limiterSection = _getLimiterNodes(orig_I, gainOffset[0]);

    err = _getGainArray(&limiterSection, sigLength, inOutBuffer, gainBuffer, instlBuffer);
    if (err) goto cleanup;
    err = _accumulateBuffer(sigLength, 0, instlBuffer, gainBuffer);
    if (err) goto cleanup;
  }

  memcpy(inOutBuffer, gainBuffer, sigLength * sizeof(float));

cleanup:

  free(gainBuffer);
  gainBuffer = NULL;
  free(instlBuffer);
  instlBuffer = NULL;
  free(tmpBuffer);
  tmpBuffer = NULL;

  if (err) return err;
  return DOG_OK;
}
