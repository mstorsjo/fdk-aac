
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

#define L_SUBFR 64

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "options.h"
#include "mathlib.h"
#include "iisLPDComLib_constants.h"
#include "iisLPDComLib_tools.h"

static const float filter_interp[L_FILT_MAX + 1] = {
    1.0f,
    0.63487604f, 0.0f,
    -0.20701353f, 0.0f,
    0.11879487f, 0.0f,
    -0.07926575f, 0.0f,
    0.05615642f, 0.0f,
    -0.04070711f, 0.0f,
    0.02957617f, 0.0f,
    -0.02122066f, 0.0f,
    0.01483115f, 0.0f,
    -0.00993903f, 0.0f,
    0.00624819f, 0.0f,
    -0.00355476f, 0.0f,
    0.00170582f, 0.0f,
    -0.00057701f, 0.0f,
    0.00006013f, 0.0f};

void LPDCom_tools_TimeDomainResample(
    const float pInSignal[],
    short numSamplesIn,
    const int srIn,
    float pOutSignal[],
    const int srOut,
    float filterMemory[],
    int just_filter) {
  short i, j, step_size, n = 0;
  short numSamplesOut;
  float signal[2 * L_FILT_MAX + L_FRAME_MAX];

  if (srIn < srOut) {
    if (filterMemory) {
      copyFLOAT(filterMemory, signal, UPSAMP_FILT_MEM_SIZE);
    } else if (UPSAMP_FILT_MEM_SIZE / 2 < numSamplesIn) {
      for (i = 0; i < UPSAMP_FILT_MEM_SIZE / 2; i++) {
        signal[UPSAMP_FILT_MEM_SIZE / 2 - 1 - i] = 2 * pInSignal[0] - pInSignal[i + 1];
        signal[numSamplesIn + UPSAMP_FILT_MEM_SIZE / 2 + i] = 2 * pInSignal[numSamplesIn - 1] - pInSignal[numSamplesIn - 2 - i];
      }
    }

    if (filterMemory) {
      copyFLOAT(pInSignal, signal + UPSAMP_FILT_MEM_SIZE, numSamplesIn);
    } else {
      copyFLOAT(pInSignal, signal + UPSAMP_FILT_MEM_SIZE / 2, numSamplesIn);
    }

    for (i = 0; i < numSamplesIn; i++) {
      pOutSignal[(i * 2)] = signal[i + UPSAMP_FILT_LEN];

      pOutSignal[(i * 2) + 1] = 0.f;
      for (j = 0; j < UPSAMP_FILT_LEN; j++) {
        pOutSignal[(i * 2) + 1] += signal[i + UPSAMP_FILT_LEN - j] * filter_interp[(j * 2) + 1] +
                                   signal[i + 1 + UPSAMP_FILT_LEN + j] * filter_interp[(j * 2) + 1];
      }
    }

    if (filterMemory) {
      copyFLOAT(signal + numSamplesIn, filterMemory, UPSAMP_FILT_MEM_SIZE);
    }
  } else {
    step_size = (1 == just_filter) ? 1 : 2;
    numSamplesOut = (1 == just_filter) ? numSamplesIn : numSamplesIn / 2;

    if (filterMemory) {
      copyFLOAT(filterMemory, signal, DOWNSAMP_FILT_MEM_SIZE);
    } else if (DOWNSAMP_FILT_MEM_SIZE / 2 < numSamplesIn) {
      for (i = 0; i < DOWNSAMP_FILT_MEM_SIZE / 2; i++) {
        signal[DOWNSAMP_FILT_MEM_SIZE / 2 - 1 - i] = 2 * pInSignal[0] - pInSignal[i + 1];
        signal[numSamplesIn + DOWNSAMP_FILT_MEM_SIZE / 2 + i] = 2 * pInSignal[numSamplesIn - 1] - pInSignal[numSamplesIn - 2 - i];
      }
    }

    if (filterMemory) {
      copyFLOAT(pInSignal, signal + DOWNSAMP_FILT_MEM_SIZE, numSamplesIn);
    } else {
      copyFLOAT(pInSignal, signal + DOWNSAMP_FILT_MEM_SIZE / 2, numSamplesIn);
    }

    for (i = 0; i < numSamplesOut; i++) {
      pOutSignal[i] = 0.f;

      for (j = 0; j < DOWNSAMP_FILT_LEN; j++) {
        pOutSignal[i] += signal[n + DOWNSAMP_FILT_LEN - j] * filter_interp[j] +
                         signal[n + 1 + DOWNSAMP_FILT_LEN + j] * filter_interp[j + 1];
      }

      pOutSignal[i] *= 0.5f;
      n += step_size;
    }

    if (0 == just_filter && filterMemory) {
      copyFLOAT(signal + numSamplesIn, filterMemory, DOWNSAMP_FILT_MEM_SIZE);
    }
  }
}

float LPDCom_tools_segSNR(
    float x[],
    float xe[],
    short n,
    short nseg) {
  float snr = 0.0f;
  float signal, noise, error, fac;
  short i, j;

  for (i = 0; i < n; i += nseg) {
    signal = 1e-6f;
    noise = 1e-6f;

    for (j = 0; j < nseg; j++) {
      signal += (*x) * (*x);
      error = *x++ - *xe++;
      noise += error * error;
    }

    snr += (float)log10((double)(signal / noise));
  }

  fac = ((float)(10 * nseg)) / (float)n;
  snr = fac * snr;

  if (snr < -99.0f) {
    snr = -99.0f;
  }

  if (snr != snr) {
    snr = -999999.0f;
  }

  return (snr);
}

float LPDCom_tools_GetGain(
    float x[],
    float y[],
    int n) {
  float corr = 0.0f, ener = 1e-6f;
  short i;

  for (i = 0; i < n; i++) {
    corr += x[i] * y[i];
    ener += y[i] * y[i];
  }

  return (corr / ener);
}

float LPDCom_tools_GetGainExhaustiveSearch(
    float x[],
    float y[],
    int n) {
  float E_min = FLT_MAX;
  float E;
  float g = 0.0f;
  int i;
  float res = 0.0f;

  float corr[3] = {0.00f, 0.00f, 0.01f};
  float lambda = LAMBDA_OPTIMIZER;
  float tmp;

  for (i = 0; i < n; i++) {
    corr[0] += x[i] * x[i];
    corr[1] += x[i] * y[i];
    corr[2] += y[i] * y[i];
  }

  tmp = 2.0f * (float)sqrt(corr[2]) / (float)n;

  for (i = 0; i < 128; i++) {
    g = (float)pow(10, (float)i / 28.0f);
    g = g / tmp;
    if (lambda > 0.0f && corr[0] < g * g * corr[2]) {
      break;
    }

    E = corr[0] - 2 * g * corr[1] + g * g * corr[2] + lambda * corr[0] - lambda * g * g * corr[2];

    if (E < E_min) {
      E_min = E;
      res = (float)pow(10, (float)i / 28.0f) / tmp;
    }
  }

  return res;
}

void LPDCom_tools_convolve(
    float* x,
    float* h,
    float* y) {
  float temp;
  int i;
  int n;
  for (n = 0; n < L_SUBFR; n += 2) {
    temp = 0.0;

    for (i = 0; i <= n; i++) {
      temp += x[i] * h[n - i];
    }

    y[n] = temp;
    temp = 0.0;

    for (i = 0; i <= (n + 1); i += 2) {
      temp += x[i] * h[(n + 1) - i];
      temp += x[i + 1] * h[n - i];
    }

    y[n + 1] = temp;
  }
}

void LPDCom_tools_Preemphasise(
    float* signal,
    float mu,
    long L,
    float* mem) {
  long i;
  float temp;

  temp = signal[L - 1];

  for (i = L - 1; i > 0; i--) {
    signal[i] = signal[i] - mu * signal[i - 1];
  }

  signal[0] -= mu * (*mem);
  *mem = temp;
}

void LPDCom_tools_Deemphasise(
    float* signal,
    float mu,
    long L,
    float* mem) {
  long i;

  if (L <= 0) {
    return;
  }

  signal[0] = signal[0] + mu * (*mem);

  for (i = 1; i < L; i++) {
    signal[i] = signal[i] + mu * signal[i - 1];
  }

  *mem = signal[L - 1];

  if ((*mem < 1e-10) & (*mem > -1e-10)) {
    *mem = 0;
  }
}

void LPDEnc_tools_HighPass50Hz(
    float* signal,
    long lg,
    float* mem,
    long fscale) {
  int i;
  float x0, x1, x2, y0, y1, y2;
  float a1, a2, b1, b2, frac;

  y1 = mem[0];
  y2 = mem[1];
  x0 = mem[2];
  x1 = mem[3];

  if (fscale >= FSCALE_DENOM) {
    frac = ((float)(fscale - FSCALE_DENOM)) / ((float)FSCALE_DENOM);
    a1 = 1.98611621154089f + (frac * 0.00694181160232f);
    a2 = -0.98621192916075f - (frac * 0.00687010630146f);
    b1 = -1.98616407035082f - (frac * 0.00690595895189f);
    b2 = 0.99308203517541f + (frac * 0.00345297947594f);

    for (i = 0; i < lg; i++) {
      x2 = x1;
      x1 = x0;
      x0 = signal[i];
      y0 = (y1 * a1) + (y2 * a2) + (x0 * b2) + (x1 * b1) + (x2 * b2);
      signal[i] = y0;
      y2 = y1;
      y1 = y0;
    }
  } else {
    for (i = 0; i < lg; i++) {
      x2 = x1;
      x1 = x0;
      x0 = signal[i];
      y0 = y1 * 1.978881836F + y2 * -0.979125977F + x0 * 0.989501953F + x1 * -1.979003906F + x2 * 0.989501953F;
      signal[i] = y0;
      y2 = y1;
      y1 = y0;
    }
  }

  mem[0] = ((y1 > 1e-10) | (y1 < -1e-10)) ? y1 : 0;
  mem[1] = ((y2 > 1e-10) | (y2 < -1e-10)) ? y2 : 0;
  mem[2] = ((x0 > 1e-10) | (x0 < -1e-10)) ? x0 : 0;
  mem[3] = ((x1 > 1e-10) | (x1 < -1e-10)) ? x1 : 0;
}
