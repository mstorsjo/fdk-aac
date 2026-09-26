
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
#include "iisDRCGainGenerator.h"
#include "iisDRCLevelComputationBS1770.h"
#include "iisDRCGainGeneratorTables.h"

const float levelThr_bs1770 = 1e-7f;
const float nullBandRefLevel_bs1770 = -31.f;

void levelComputation_initFilterParams(IisDRCGainGenLevelCalcParams_BS1770Filter *iisDRCGainGenLevelCalcParams, const int sampleRate, const int backgroundCustomFilters) {
  const float pi = 3.14159265f;
  float w0 = 0.f, A = 0.f, alpha = 0.f, sinw0 = 0.f, cosw0 = 0.f, sqrtA = 0.f;
  float b0 = 0.f, b1 = 0.f, b2 = 0.f, a0 = 0.f, a1 = 0.f, a2 = 0.f;

  (void)backgroundCustomFilters;

  if (iisDRCGainGenLevelCalcParams == NULL) return;
  if (sampleRate == 0) return;

  iisDRCGainGenLevelCalcParams->skipPreFilter = 0;

  w0 = 2 * pi * 1500 / sampleRate;
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

  iisDRCGainGenLevelCalcParams->pre_a1 = a1 / a0;
  iisDRCGainGenLevelCalcParams->pre_a2 = a2 / a0;
  iisDRCGainGenLevelCalcParams->pre_b0 = b0 / a0;
  iisDRCGainGenLevelCalcParams->pre_b1 = b1 / a0;
  iisDRCGainGenLevelCalcParams->pre_b2 = b2 / a0;

  {
    w0 = 2 * pi * 38 / sampleRate;
    sinw0 = (float)sin(w0);
    cosw0 = (float)cos(w0);
    alpha = sinw0;

    b0 = (1 + cosw0) / 2;
    b1 = -(1 + cosw0);
    b2 = (1 + cosw0) / 2;
    a0 = 1 + alpha;
    a1 = -2 * cosw0;
    a2 = 1 - alpha;
  }

  iisDRCGainGenLevelCalcParams->rlb_a1 = a1 / a0;
  iisDRCGainGenLevelCalcParams->rlb_a2 = a2 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b0 = b0 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b1 = b1 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b2 = b2 / a0;
}

void levelComputation_initFilterParams_legacy(IisDRCGainGenLevelCalcParams_BS1770Filter *iisDRCGainGenLevelCalcParams, const int sampleRate) {
  const float pi = 3.14159265f;
  float w0 = 0.f, alpha = 0.f, sinw0 = 0.f, cosw0 = 0.f;
  float b0 = 0.f, b1 = 0.f, b2 = 0.f, a0 = 0.f, a1 = 0.f, a2 = 0.f;
  float Q = 0.107693615847821f, f0 = 1736.049781397266f;

  if (iisDRCGainGenLevelCalcParams == NULL) return;
  if (sampleRate == 0) return;

  iisDRCGainGenLevelCalcParams->skipPreFilter = 1;

  iisDRCGainGenLevelCalcParams->pre_a1 = 0;
  iisDRCGainGenLevelCalcParams->pre_a2 = 0;
  iisDRCGainGenLevelCalcParams->pre_b0 = 1;
  iisDRCGainGenLevelCalcParams->pre_b1 = 0;
  iisDRCGainGenLevelCalcParams->pre_b2 = 0;

  w0 = 2 * pi * f0 / sampleRate;
  sinw0 = (float)sin(w0);
  cosw0 = (float)cos(w0);
  alpha = (float)(sinw0 / (2 * Q));

  b0 = alpha;
  b1 = 0;
  b2 = -alpha;
  a0 = 1 + alpha;
  a1 = -2 * cosw0;
  a2 = 1 - alpha;

  iisDRCGainGenLevelCalcParams->rlb_a1 = a1 / a0;
  iisDRCGainGenLevelCalcParams->rlb_a2 = a2 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b0 = b0 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b1 = b1 / a0;
  iisDRCGainGenLevelCalcParams->rlb_b2 = b2 / a0;
}

void levelComputation_initIntParams(IisDRCGainGenLevelCalcParams_BS1770Int *iisDRCGainGenLevelCalcParams, const float integrationTime, const int sampleRate, const float PRL) {
  float tmp;

  if (iisDRCGainGenLevelCalcParams == NULL) return;
  if (sampleRate == 0) return;
  if (integrationTime == 0.0f) return;

  tmp = 1.f - (float)pow(0.01, (double)(1.f / (integrationTime * sampleRate)));
  iisDRCGainGenLevelCalcParams->int_b0 = tmp;
  iisDRCGainGenLevelCalcParams->int_a1 = -(1 - tmp);
  iisDRCGainGenLevelCalcParams->levelThr = levelThr_bs1770;
  iisDRCGainGenLevelCalcParams->PRL = PRL;
  iisDRCGainGenLevelCalcParams->nullBandReferenceLevel = nullBandRefLevel_bs1770;

  if (iisDRCGainGenLevelCalcParams->PRL < -70.0f) {
    iisDRCGainGenLevelCalcParams->PRL = -70.0f;
  }
}

int levelComputation_BS1770Filter(const float *const *audioInput,
                                  float **xSquare,
                                  IisDRCGainGenLevelCalcParams_BS1770Filter *iisDRCGainGenLevelCalcParams,
                                  IisDRCGainGenLevelCalcStates_BS1770Filter *iisDRCGainGenLevelCalcStates) {
  int k = 0, ch = 0;
  float x = 0, y = 0;

  float pre_b0 = iisDRCGainGenLevelCalcParams->pre_b0;
  float pre_b1 = iisDRCGainGenLevelCalcParams->pre_b1;
  float pre_b2 = iisDRCGainGenLevelCalcParams->pre_b2;
  float pre_a1 = iisDRCGainGenLevelCalcParams->pre_a1;
  float pre_a2 = iisDRCGainGenLevelCalcParams->pre_a2;

  float rlb_b0 = iisDRCGainGenLevelCalcParams->rlb_b0;
  float rlb_b1 = iisDRCGainGenLevelCalcParams->rlb_b1;
  float rlb_b2 = iisDRCGainGenLevelCalcParams->rlb_b2;
  float rlb_a1 = iisDRCGainGenLevelCalcParams->rlb_a1;
  float rlb_a2 = iisDRCGainGenLevelCalcParams->rlb_a2;
  int numSamples = 0, startIndex = 0;

  if (audioInput == NULL || xSquare == NULL || iisDRCGainGenLevelCalcStates == NULL) {
    return -18;
  } else {
    for (k = 0; k < iisDRCGainGenLevelCalcParams->numChannels; k++) {
      if (audioInput[k] == NULL) {
        return -19;
      }
    }
    for (k = 0; k < iisDRCGainGenLevelCalcParams->numChannels; k++) {
      if (xSquare[k] == NULL) {
        return -20;
      }
    }
  }

  if (iisDRCGainGenLevelCalcStates->bFirstFrameFiltering) {
    numSamples = iisDRCGainGenLevelCalcParams->frameSize + iisDRCGainGenLevelCalcParams->sequenceHoldLookaheadSamples;
    startIndex = 0;
  } else {
    numSamples = iisDRCGainGenLevelCalcParams->frameSize;
    startIndex = iisDRCGainGenLevelCalcParams->sequenceHoldLookaheadSamples;
  }

  for (ch = 0; ch < iisDRCGainGenLevelCalcParams->numChannels; ch++) {
    if (!iisDRCGainGenLevelCalcStates->bFirstFrameFiltering && iisDRCGainGenLevelCalcParams->sequenceHoldLookaheadSamples > 0) {
      memmove(xSquare[ch], xSquare[ch] + iisDRCGainGenLevelCalcParams->frameSize, sizeof(int) * iisDRCGainGenLevelCalcParams->sequenceHoldLookaheadSamples);
    }

    for (k = startIndex; k < startIndex + numSamples; k++) {
      x = audioInput[ch][k];

      x += iisGainGen_noDenormalsInLevelEstim_Noise[k % MAXNUMDENORMAL];
      if (!iisDRCGainGenLevelCalcParams->skipPreFilter) {
        y = pre_b0 * x + pre_b1 * iisDRCGainGenLevelCalcStates->pre_x1[ch] + pre_b2 * iisDRCGainGenLevelCalcStates->pre_x2[ch] - pre_a1 * iisDRCGainGenLevelCalcStates->pre_y1[ch] - pre_a2 * iisDRCGainGenLevelCalcStates->pre_y2[ch];
        iisDRCGainGenLevelCalcStates->pre_x2[ch] = iisDRCGainGenLevelCalcStates->pre_x1[ch];
        iisDRCGainGenLevelCalcStates->pre_x1[ch] = x;
        iisDRCGainGenLevelCalcStates->pre_y2[ch] = iisDRCGainGenLevelCalcStates->pre_y1[ch];
        iisDRCGainGenLevelCalcStates->pre_y1[ch] = y;
        x = y;
      }

      y = rlb_b0 * x + rlb_b1 * iisDRCGainGenLevelCalcStates->rlb_x1[ch] + rlb_b2 * iisDRCGainGenLevelCalcStates->rlb_x2[ch] - rlb_a1 * iisDRCGainGenLevelCalcStates->rlb_y1[ch] - rlb_a2 * iisDRCGainGenLevelCalcStates->rlb_y2[ch];
      iisDRCGainGenLevelCalcStates->rlb_x2[ch] = iisDRCGainGenLevelCalcStates->rlb_x1[ch];
      iisDRCGainGenLevelCalcStates->rlb_x1[ch] = x;
      iisDRCGainGenLevelCalcStates->rlb_y2[ch] = iisDRCGainGenLevelCalcStates->rlb_y1[ch];
      iisDRCGainGenLevelCalcStates->rlb_y1[ch] = y;

      xSquare[ch][k] = y * y;
    }
  }

  iisDRCGainGenLevelCalcStates->bFirstFrameFiltering = 0;

  return 0;
}

int levelComputation_BS1770Int(const float *const *xSquare,
                               float *levelOut,
                               IisDRCGainGenParamsInstance *iisDRCGainGenParams,
                               IisDRCGainGenLevelCalcStates_BS1770Int *iisDRCGainGenLevelCalcStates) {
  int k = 0, ch = 0;
  float y = 0;

  float int_b0 = iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.int_b0;
  float int_a1 = iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.int_a1;

  if (xSquare == NULL || iisDRCGainGenLevelCalcStates == NULL) {
    return -21;
  } else {
    for (k = 0; k < iisDRCGainGenParams->numChannels; k++) {
      if (xSquare[k] == NULL) {
        return -22;
      }
    }
  }

  memset(iisDRCGainGenLevelCalcStates->ySum, 0, sizeof(float) * iisDRCGainGenParams->frameSize);

  for (ch = 0; ch < iisDRCGainGenParams->numChannels; ch++) {
    if (iisDRCGainGenParams->channelWeight[ch] == 1.0f) {
      for (k = 0; k < iisDRCGainGenParams->frameSize; k++) {
        y = int_b0 * xSquare[ch][k] - int_a1 * iisDRCGainGenLevelCalcStates->int_y1[ch];
        iisDRCGainGenLevelCalcStates->int_y1[ch] = y;
        iisDRCGainGenLevelCalcStates->ySum[k] += y;
      }
    } else if ((iisDRCGainGenParams->channelWeight[ch] != 0.0f)) {
      float yWeighted;
      for (k = 0; k < iisDRCGainGenParams->frameSize; k++) {
        y = int_b0 * xSquare[ch][k] - int_a1 * iisDRCGainGenLevelCalcStates->int_y1[ch];
        iisDRCGainGenLevelCalcStates->int_y1[ch] = y;
        yWeighted = y * iisDRCGainGenParams->channelWeight[ch];
        iisDRCGainGenLevelCalcStates->ySum[k] += yWeighted;
      }
    }
  }

  for (k = 0; k < iisDRCGainGenParams->frameSize; k++) {
    if (iisDRCGainGenLevelCalcStates->ySum[k] < iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.levelThr) {
      iisDRCGainGenLevelCalcStates->ySum[k] = iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.levelThr;
    }

    levelOut[k] = 10.f * (float)log10((double)iisDRCGainGenLevelCalcStates->ySum[k]);
    if (iisDRCGainGenParams->drcLevelCalculationMode == DRC_LEVEL_BS1770) {
      levelOut[k] -= 0.691f;
    }

    levelOut[k] -= (iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.PRL - iisDRCGainGenParams->iisDRCGainGenLevelCalcParams_BS1770Int.nullBandReferenceLevel);

    levelOut[k] += 3.0f;
  }

  return 0;
}
