
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

#include "iisutillib.h"

#include "spaceEnclib_const.h"
#include "space_bitstream.h"
#include "sac_tonality.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static const float part4[4] = {2.f, 4.f, 17.f, 41.f};
static const float part5[5] = {1.f, 2.f, 6.f, 14.f, 41.f};
static const float part7[7] = {1.f, 1.f, 2.f, 4.f, 6.f, 9.f, 41.f};
static const float part9[9] = {1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 5.f, 9.f, 41.f};
static const float part10[10] = {.5f, .5f, 1.f, 1.f, 2.f, 2.f, 2.f, 5.f, 9.f, 41.f};
static const float part12[12] = {1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 12.f, 29.f};
static const float part14[14] = {.5f, .5f, .5f, .5f, 1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 12.f, 29.f};
static const float part20[20] = {.25f, .25f, .25f, .25, .5f, .5f, .5f, .5f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 3.f, 4.f, 5.f, 12.f, 29.f};
static const float part28[28] = {0.25f, 0.25f, 0.25f, 0.25f, 0.5f, 0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 6.f, 7.f, 16.f};
static const float part15[15] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 3.f, 4.f, 5.f, 12.f, 29.f};
static const float part23[23] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 2.f, 3.f, 3.f, 4.f, 5.f, 6.f, 7.f, 16.f};

typedef struct T_MEASURE_TONALITY {
  float **ppQmfReal;
  float **ppQmfImag;

  float spec_prev_real[NUM_QMF_BANDS * 8];
  float spec_prev_imag[NUM_QMF_BANDS * 8];

  float spec_zoom_real[NUM_QMF_BANDS * 8];
  float spec_zoom_imag[NUM_QMF_BANDS * 8];

  float p_cross_real[NUM_QMF_BANDS * 8];
  float p_cross_imag[NUM_QMF_BANDS * 8];

  float p_sum[NUM_QMF_BANDS * 8];
  float p_sum_prev[NUM_QMF_BANDS * 8];

  float bufReal[NUM_QMF_BANDS][6];
  float bufImag[NUM_QMF_BANDS][6];

  float winBufReal[NUM_QMF_BANDS][16];
  float winBufImag[NUM_QMF_BANDS][16];

  float p_max[NUM_QMF_BANDS * 8];

  float coh_spec[NUM_QMF_BANDS * 8];
  float pow_spec[NUM_QMF_BANDS * 8];

  const float *part;

  int numParameterBands;
  int numInputChannels;
  int timeSlots;
  int numSlotOffset;
  int qmfBands;
  int samplingFreq;

} MEASURE_TONALITY, *HANDLE_MEASURE_TONALITY;

typedef struct T_SPATIAL_TONALITY {
  HANDLE_MEASURE_TONALITY hMeasureTonality;

  int numParameterBands;
  int numInputChannels;
  int timeSlots;
  int numSlotOffset;
  int qmfBands;
  int samplingFreq;

  int prevTonality[MAX_NUM_BINS];

} SPATIAL_TONALITY;

static void ZoomFFT16(float *inReal, float *inImag, float *outReal, float *outImag, int qmfBand, float dfrac);

static HANDLE_ERROR_INFO
CreateSpatialMeasureTonality(HANDLE_MEASURE_TONALITY *phMeasureTonality,
                             int numParameterBands,
                             int numInputChannels,
                             int timeSlots,
                             int numSlotOffset,
                             int qmfBands,
                             int samplingFreq) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (NULL == (*phMeasureTonality = (HANDLE_MEASURE_TONALITY)iisCalloc(1, sizeof(MEASURE_TONALITY)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (error == noError) {
    (*phMeasureTonality)->numParameterBands = numParameterBands;
    (*phMeasureTonality)->numInputChannels = numInputChannels;
    (*phMeasureTonality)->timeSlots = timeSlots;
    (*phMeasureTonality)->numSlotOffset = numSlotOffset;
    (*phMeasureTonality)->qmfBands = qmfBands;
    (*phMeasureTonality)->samplingFreq = samplingFreq;
  }

  if (error == noError) {
    if (NULL == ((*phMeasureTonality)->ppQmfReal = (float **)iisCallocMatrix2D((*phMeasureTonality)->qmfBands, (*phMeasureTonality)->timeSlots, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory");
    }
  }

  if (error == noError) {
    if (NULL == ((*phMeasureTonality)->ppQmfImag = (float **)iisCallocMatrix2D((*phMeasureTonality)->qmfBands, (*phMeasureTonality)->timeSlots, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory");
    }
  }

  if (error == noError) {
    switch ((*phMeasureTonality)->numParameterBands) {
      case 4:
        (*phMeasureTonality)->part = part4;
        break;
      case 5:
        (*phMeasureTonality)->part = part5;
        break;
      case 7:
        (*phMeasureTonality)->part = part7;
        break;
      case 9:
        (*phMeasureTonality)->part = part9;
        break;
      case 10:
        (*phMeasureTonality)->part = part10;
        break;
      case 12:
        (*phMeasureTonality)->part = part12;
        break;
      case 14:
        (*phMeasureTonality)->part = part14;
        break;
      case 20:
        (*phMeasureTonality)->part = part20;
        break;
      case 28:
        (*phMeasureTonality)->part = part28;
        break;
      case 15:
        (*phMeasureTonality)->part = part15;
        break;
      case 23:
        (*phMeasureTonality)->part = part23;
        break;
      default:
        error = iisUtil_ERROR(CDI, "Invalid number parameter bands.");
        break;
    }
  }

  if (error == noError) {
    int i;

    for (i = 0; i < (*phMeasureTonality)->qmfBands * 8; i++) {
      (*phMeasureTonality)->spec_prev_real[i] = 0.0f;
      (*phMeasureTonality)->spec_prev_imag[i] = 0.0f;

      (*phMeasureTonality)->p_cross_real[i] = 0.0f;
      (*phMeasureTonality)->p_cross_imag[i] = 0.0f;

      (*phMeasureTonality)->p_sum[i] = 0.0f;
      (*phMeasureTonality)->p_sum_prev[i] = 0.0f;
    }

    for (i = 0; i < (*phMeasureTonality)->qmfBands; i++) {
      int j;

      for (j = 0; j < 6; j++) {
        (*phMeasureTonality)->bufReal[i][j] = 0.0f;
        (*phMeasureTonality)->bufImag[i][j] = 0.0f;
      }

      for (j = 0; j < 16; j++) {
        (*phMeasureTonality)->winBufReal[i][j] = 0.0f;
        (*phMeasureTonality)->winBufImag[i][j] = 0.0f;
      }
    }
  }

  return error;
}

static HANDLE_ERROR_INFO
DestroySpatialMeasureTonality(HANDLE_MEASURE_TONALITY *phMeasureTonality) {
  if (*phMeasureTonality) {
    if ((*phMeasureTonality)->ppQmfReal) {
      iisFreeMatrix2D((void **)(*phMeasureTonality)->ppQmfReal);
    }
    (*phMeasureTonality)->ppQmfReal = NULL;

    if ((*phMeasureTonality)->ppQmfImag) {
      iisFreeMatrix2D((void **)(*phMeasureTonality)->ppQmfImag);
    }
    (*phMeasureTonality)->ppQmfImag = NULL;

    iisFree(*phMeasureTonality);
  }
  *phMeasureTonality = NULL;

  return noError;
}

static HANDLE_ERROR_INFO
SpatialMeasureTonality(HANDLE_MEASURE_TONALITY hMeasureTonality, float ***qmfInputReal, float ***qmfInputImag, float *tonality) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hMeasureTonality == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    float **ppQmfReal = hMeasureTonality->ppQmfReal;
    float **ppQmfImag = hMeasureTonality->ppQmfImag;

    float *spec_zoom_real = hMeasureTonality->spec_zoom_real;
    float *spec_zoom_imag = hMeasureTonality->spec_zoom_imag;

    float *spec_prev_real = hMeasureTonality->spec_prev_real;
    float *spec_prev_imag = hMeasureTonality->spec_prev_imag;

    float *p_cross_real = hMeasureTonality->p_cross_real;
    float *p_cross_imag = hMeasureTonality->p_cross_imag;

    float *p_sum = hMeasureTonality->p_sum;
    float *p_sum_prev = hMeasureTonality->p_sum_prev;

    float *p_max = hMeasureTonality->p_max;

    float *coh_spec = hMeasureTonality->coh_spec;
    float *pow_spec = hMeasureTonality->pow_spec;

    int g, gmax;
    int i, j, q;

    const float *part = hMeasureTonality->part;
    const int timeSlots = hMeasureTonality->timeSlots;
    const int numSlotOffset = hMeasureTonality->numSlotOffset;
    int pstart, pstop;
    float pqmf;

    float beta;
    float dwin, dfrac;
    int nstart;

    const int delay = 0;

    for (q = 0; q < hMeasureTonality->qmfBands; q++) {
      int s;

      for (s = 0; s < timeSlots; s++) {
        float tmpReal = 0.0f;
        float tmpImag = 0.0f;
        int c;
        for (c = 0; c < hMeasureTonality->numInputChannels; c++) {
          tmpReal += qmfInputReal[c][s + numSlotOffset][q] / (float)hMeasureTonality->numInputChannels;
          tmpImag += qmfInputImag[c][s + numSlotOffset][q] / (float)hMeasureTonality->numInputChannels;
        }

        if (s + delay < timeSlots) {
          ppQmfReal[q][s + delay] = tmpReal;
          ppQmfImag[q][s + delay] = tmpImag;
        } else {
          ppQmfReal[q][s + delay - timeSlots] = hMeasureTonality->bufReal[q][s + delay - timeSlots];
          ppQmfImag[q][s + delay - timeSlots] = hMeasureTonality->bufImag[q][s + delay - timeSlots];
          hMeasureTonality->bufReal[q][s + delay - timeSlots] = tmpReal;
          hMeasureTonality->bufImag[q][s + delay - timeSlots] = tmpImag;
        }
      }
    }

    gmax = (int)ceil(timeSlots / 16.0f);
    dwin = ((float)timeSlots) / gmax;

    beta = hMeasureTonality->qmfBands * dwin / (0.025f * hMeasureTonality->samplingFreq);

    for (i = 0; i < hMeasureTonality->numParameterBands; i++) {
      tonality[i] = 1.0f;
    }

    for (g = 0; g < gmax; g++) {
      nstart = (int)floor((g + 1) * dwin + 0.5) - 16;

      dfrac = (float)floor((g + 1) * dwin + 0.5) - ((g + 1) * dwin);

      for (q = 0; q < hMeasureTonality->qmfBands; q++) {
        for (i = 0; i < 16; i++) {
          if (nstart + i < 0) {
            hMeasureTonality->winBufReal[q][i] = hMeasureTonality->winBufReal[q][16 + nstart + i];
            hMeasureTonality->winBufImag[q][i] = hMeasureTonality->winBufImag[q][16 + nstart + i];
          } else {
            hMeasureTonality->winBufReal[q][i] = ppQmfReal[q][nstart + i];
            hMeasureTonality->winBufImag[q][i] = ppQmfImag[q][nstart + i];
          }
        }
      }

      for (q = 0; q < hMeasureTonality->qmfBands; q++) {
        ZoomFFT16(&(hMeasureTonality->winBufReal[q][0]), &(hMeasureTonality->winBufImag[q][0]), &(spec_zoom_real[q * 8]), &(spec_zoom_imag[q * 8]), q, dfrac);
      }

      for (i = 0; i < 8 * hMeasureTonality->qmfBands; i++) {
        p_cross_real[i] = beta * (spec_zoom_real[i] * spec_prev_real[i] + spec_zoom_imag[i] * spec_prev_imag[i]) + (1 - beta) * p_cross_real[i];
        p_cross_imag[i] = beta * (spec_zoom_imag[i] * spec_prev_real[i] - spec_zoom_real[i] * spec_prev_imag[i]) + (1 - beta) * p_cross_imag[i];

        p_sum[i] = beta * (spec_zoom_real[i] * spec_zoom_real[i] + spec_zoom_imag[i] * spec_zoom_imag[i]) + (1 - beta) * p_sum[i];
        p_max[i] = max(p_sum[i], p_sum_prev[i]);
        p_sum_prev[i] = p_sum[i];

        coh_spec[i] = ((float)sqrt(p_cross_real[i] * p_cross_real[i] + p_cross_imag[i] * p_cross_imag[i]) + 1e-20f) / (p_max[i] + 1e-20f);
        pow_spec[i] = (spec_zoom_real[i] * spec_zoom_real[i] + spec_zoom_imag[i] * spec_zoom_imag[i]) +
                      (spec_prev_real[i] * spec_prev_real[i] + spec_prev_imag[i] * spec_prev_imag[i]);

        spec_prev_real[i] = spec_zoom_real[i];
        spec_prev_imag[i] = spec_zoom_imag[i];
      }

      pstart = 0;
      pqmf = 0;
      for (i = 0; i < hMeasureTonality->numParameterBands; i++) {
        float num = 0.0f;
        float den = 0.0f;
        float tmp_ton;

        pqmf += part[i];
        pstop = (int)(pqmf * 8 + 0.5);

        for (j = pstart; j < pstop; j++) {
          num += pow_spec[j] * coh_spec[j];
          den += pow_spec[j];
        }

        tmp_ton = (num + 1e-20f) / (den + 1e-20f);
        if (tmp_ton > 1.0f) tmp_ton = 1.0f;

        if (tmp_ton < tonality[i]) tonality[i] = tmp_ton;

        pstart = pstop;
      }
    }
  }

  return error;
}

void ZoomFFT16(float *inReal, float *inImag, float *outReal, float *outImag, int qmfBand, float dfrac) {
  const float wReal[16] = {1.000000f, 0.980785f, 0.923880f, 0.831470f, 0.707107f, 0.555570f, 0.382683f, 0.195090f,
                           0.000000f, -0.195090f, -0.382683f, -0.555570f, -0.707107f, -0.831470f, -0.923880f, -0.980785f};
  const float wImag[16] = {0.000000f, -0.195090f, -0.382683f, -0.555570f, -0.707107f, -0.831470f, -0.923880f, -0.980785f,
                           -1.000000f, -0.980785f, -0.923880f, -0.831470f, -0.707107f, -0.555570f, -0.382683f, -0.195090f};

  const int bitrev[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

  const double pi = 3.14159265359;

  float blackman[16];

  float vReal[16], vImag[16];
  float tReal, tImag;
  float eReal, eImag;

  int i, j, s1, s2;

  for (i = 0; i < 16; i++) {
    blackman[i] = 0.42f - 0.5f * (float)cos(2 * pi * (i + dfrac) / 15) + 0.08f * (float)cos(4 * pi * (i + dfrac) / 15);
  }

  for (i = 0; i < 16; i++) {
    vReal[bitrev[i]] = (inReal[i] * wReal[i] - inImag[i] * wImag[i]) * blackman[i];
    vImag[bitrev[i]] = (inReal[i] * wImag[i] + inImag[i] * wReal[i]) * blackman[i];
  }

  for (s1 = 1, s2 = 16; s1 < 8; s1 <<= 1, s2 >>= 1) {
    for (i = 0; i < 16; i += 2 * s1) {
      for (j = 0; j < s1; j++) {
        tReal = vReal[i + j + s1] * wReal[j * s2] - vImag[i + j + s1] * wImag[j * s2];
        tImag = vReal[i + j + s1] * wImag[j * s2] + vImag[i + j + s1] * wReal[j * s2];

        vReal[i + j + s1] = vReal[i + j] - tReal;
        vImag[i + j + s1] = vImag[i + j] - tImag;

        vReal[i + j] = vReal[i + j] + tReal;
        vImag[i + j] = vImag[i + j] + tImag;
      }
    }
  }

  for (j = 0; j < 8; j++) {
    tReal = vReal[j + 8] * wReal[j * 2] - vImag[j + 8] * wImag[j * 2];
    tImag = vReal[j + 8] * wImag[j * 2] + vImag[j + 8] * wReal[j * 2];

    if ((qmfBand % 2) == 0) {
      outReal[j] = vReal[j] + tReal;
      outImag[j] = vImag[j] + tImag;
    } else {
      outReal[j] = vReal[j] - tReal;
      outImag[j] = vImag[j] - tImag;
    }
  }

  for (i = 0; i < 8; i++) {
    if ((qmfBand % 2) == 0) {
      eReal = (float)cos(-2 * pi / 16 * i * dfrac);
      eImag = (float)sin(-2 * pi / 16 * i * dfrac);
    } else {
      eReal = (float)cos(-2 * pi / 16 * (i - 8) * dfrac);
      eImag = (float)sin(-2 * pi / 16 * (i - 8) * dfrac);
    }

    tReal = outReal[i] * eReal - outImag[i] * eImag;
    outImag[i] = outReal[i] * eImag + outImag[i] * eReal;
    outReal[i] = tReal;
  }
}

HANDLE_ERROR_INFO
CreateSpatialTonality(HANDLE_SPATIAL_TONALITY *phSpatialTonality,
                      int numParameterBands,
                      int numInputChannels,
                      int timeSlots,
                      int numSlotOffset,
                      int qmfBands,
                      int samplingFreq) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (NULL == (*phSpatialTonality = (HANDLE_SPATIAL_TONALITY)iisCalloc(1, sizeof(SPATIAL_TONALITY)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (error == noError) {
    (*phSpatialTonality)->numParameterBands = numParameterBands;
    (*phSpatialTonality)->numInputChannels = numInputChannels;
    (*phSpatialTonality)->timeSlots = timeSlots;
    (*phSpatialTonality)->numSlotOffset = numSlotOffset;
    (*phSpatialTonality)->qmfBands = qmfBands;
    (*phSpatialTonality)->samplingFreq = samplingFreq;
  }

  SAFECALL(error, CreateSpatialMeasureTonality(&(*phSpatialTonality)->hMeasureTonality,
                                               (*phSpatialTonality)->numParameterBands,
                                               (*phSpatialTonality)->numInputChannels,
                                               (*phSpatialTonality)->timeSlots,
                                               (*phSpatialTonality)->numSlotOffset,
                                               (*phSpatialTonality)->qmfBands,
                                               (*phSpatialTonality)->samplingFreq));

  if (error == noError) {
    (*phSpatialTonality)->prevTonality[0] = -1;
  }

  return error;
}

HANDLE_ERROR_INFO
DestroySpatialTonality(HANDLE_SPATIAL_TONALITY *phSpatialTonality) {
  if (*phSpatialTonality) {
    DestroySpatialMeasureTonality(&(*phSpatialTonality)->hMeasureTonality);

    iisFree((*phSpatialTonality));
  }
  *phSpatialTonality = NULL;

  return noError;
}

HANDLE_ERROR_INFO
ApplySpatialTonality(HANDLE_SPATIAL_TONALITY hSpatialTonality,
                     SMGDATA *pData,
                     float ***qmfInputReal,
                     float ***qmfInputImag,
                     int bsIndependencyFlag) {
  HANDLE_ERROR_INFO error = noError;
  float tonality[MAX_NUM_BINS] = {0.0f};

  if (error == noError) {
    if (hSpatialTonality == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pData == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  SAFECALL(error, SpatialMeasureTonality(hSpatialTonality->hMeasureTonality, qmfInputReal, qmfInputImag, tonality));

  if (error == noError) {
    int i;

    int cnt = 0;
    int keep = 1;

    for (i = 0; i < hSpatialTonality->numParameterBands; i++) {
      tonality[i] = tonality[i] > 0.8f ? 1.f : 0.f;

      if (tonality[i] != 0.0f) {
        cnt++;
      }

      if ((int)tonality[i] != hSpatialTonality->prevTonality[i]) {
        keep = 0;
      }
    }

    pData->bsSmoothControl = 1;

    if (cnt == 0) {
      pData->bsSmoothMode[0] = 0;
    } else if ((keep == 1) && (!bsIndependencyFlag)) {
      pData->bsSmoothMode[0] = 1;
    } else if (cnt == hSpatialTonality->numParameterBands) {
      pData->bsSmoothMode[0] = 2;
      pData->bsSmoothTime[0] = 2;
    } else {
      pData->bsSmoothMode[0] = 3;
      pData->bsSmoothTime[0] = 2;
      pData->bsFreqResStride[0] = 0;

      for (i = 0; i < hSpatialTonality->numParameterBands; i++) {
        pData->bsSmgData[0][i] = (int)tonality[i];
      }
    }

    for (i = 1; i < MAX_NUM_PARAMS; i++) {
      pData->bsSmoothMode[i] = 1;
    }

    for (i = 0; i < hSpatialTonality->numParameterBands; i++) {
      hSpatialTonality->prevTonality[i] = (int)tonality[i];
    }
  }

  return error;
}
