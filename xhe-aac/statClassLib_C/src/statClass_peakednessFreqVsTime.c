
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

#include <float.h>
#include <math.h>
#include <string.h>
#include "statClass_peakednessFreqVsTime.h"
#include "statClass_features.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "iis_fft.h"

#define MEM_SIZE (1792)
#define L_NEXT 256
#define L_EXC 256
#define L_FRAME 256
#define L_PAST (2048 - L_FRAME - L_NEXT)
#define WIN_SIZE (MEM_SIZE + L_FRAME)
#define WIN_SIZE_2 1024
#define HOP_SIZE 64

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

typedef struct _statclass_peakedness_freq_vs_time {
  SCFLOAT* memSignal;
  SCFLOAT* memExc;

  SCFLOAT* x;
  SCFLOAT* xTwiddle;
  SCFLOAT* x2;
  float* fx;
  SCFLOAT* xExc;
  int* peaks;
  int xInt[WIN_SIZE / (2 * HOP_SIZE)][HOP_SIZE];

  HANDLE_IIS_FFT hIisFft;

} STATCLASS_PEAKEDNESS;

STATCLASS_ERROR_CODE STATCLASS_PeakednessFreqVsTime_Open(HANDLE_STATCLASS_PEAKEDNESS* hPeak) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i, j;

  if (NULL == hPeak) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    if (NULL == (*hPeak = (STATCLASS_PEAKEDNESS*)iisCalloc(1, sizeof(STATCLASS_PEAKEDNESS)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->memSignal = (SCFLOAT*)iisCalloc(MEM_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->memExc = (SCFLOAT*)iisCalloc(MEM_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->x = (SCFLOAT*)iisCalloc(WIN_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->xTwiddle = (SCFLOAT*)iisCalloc(WIN_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < WIN_SIZE; i++) {
      (*hPeak)->xTwiddle[i] = (SCFLOAT)(0.5 - 0.5 * cos(3.14159265 * 2 * i / WIN_SIZE));
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->xExc = (SCFLOAT*)iisCalloc(WIN_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->peaks = (int*)iisCalloc(WIN_SIZE_2, sizeof(int)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->x2 = (SCFLOAT*)iisCalloc(WIN_SIZE_2, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPeak)->fx = (float*)iisCalloc(WIN_SIZE, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < WIN_SIZE / (2 * HOP_SIZE); i++) {
      for (j = 0; j < HOP_SIZE; j++) {
        (*hPeak)->xInt[i][j] = WIN_SIZE / 4 + (i)*HOP_SIZE + 1 + j;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    IIS_FFT_ERROR err;
    err = IIS_RFFT_Create(&((*hPeak)->hIisFft), WIN_SIZE, -1);
    if (err != IIS_FFT_NO_ERROR)
      error = STATCLASS_MEMORY_ALLOC_ERROR;
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_PeakednessFreqVsTime_Close(HANDLE_STATCLASS_PEAKEDNESS* hPeak) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->memSignal) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->memSignal);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->memExc) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->memExc);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->x) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->x);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->xTwiddle) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->xTwiddle);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->xExc) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->xExc);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->peaks) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->peaks);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->x2) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->x2);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hPeak)->fx) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPeak)->fx);
    }
  }

  if ((*hPeak)->hIisFft)
    IIS_RFFT_Destroy(&((*hPeak)->hIisFft));

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hPeak) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hPeak);
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_PeakednessFreqVsTime(HANDLE_STATCLASS_PEAKEDNESS hPeak,
                                                    SCFLOAT* exc,
                                                    SCFLOAT* signal,
                                                    SCFLOAT result[2]) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  SCFLOAT xExcTmp, mean, mean2, pTd, pFd, fmoval, rmsval, rmaxTd = 0, r, en1, en2, rMax = 0, xMax = 0;
  SCFLOAT *mem_signal, *mem_exc, *x, *x2, *x_exc, *xTwiddle;
  int* peaks;
  float* fx;
  int i, j, k, tmp;

  mem_signal = hPeak->memSignal;
  mem_exc = hPeak->memExc;
  x = hPeak->x;
  x2 = hPeak->x2;
  fx = hPeak->fx;
  xTwiddle = hPeak->xTwiddle;
  peaks = hPeak->peaks;
  x_exc = hPeak->xExc;

  memcpy(x, mem_signal, MEM_SIZE * sizeof(SCFLOAT));
  memcpy(&x[MEM_SIZE], signal, L_FRAME * sizeof(SCFLOAT));
  memcpy(x_exc, mem_exc, MEM_SIZE * sizeof(SCFLOAT));
  memcpy(&x_exc[MEM_SIZE], exc, L_FRAME * sizeof(SCFLOAT));

  for (i = 0; i < WIN_SIZE / (2 * HOP_SIZE); i++) {
    mean = 0;
    mean2 = 0;

    for (j = 0; j < HOP_SIZE; j++) {
      tmp = hPeak->xInt[i][j];
      xExcTmp = x_exc[tmp - 1];

      mean += xExcTmp * xExcTmp;
      mean2 += xExcTmp * xExcTmp * xExcTmp * xExcTmp;
    }
    mean /= HOP_SIZE;
    mean2 /= HOP_SIZE;

    rmsval = (SCFLOAT)sqrt(mean) + (FLT_EPSILON);
    fmoval = (SCFLOAT)sqrt(sqrt(mean2)) + (FLT_EPSILON);
    r = fmoval / rmsval;

    if (r > rmaxTd) {
      rmaxTd = r;
    }
  }

  pTd = rmaxTd;

  for (i = 0; i < WIN_SIZE; i++) {
    fx[i] = (float)(x[i] * xTwiddle[i]);
  }

  IIS_FFT_Apply_RFFT(hPeak->hIisFft, fx, fx);

  for (i = 1; i < WIN_SIZE_2; i++) {
    x2[i] = (SCFLOAT)(sqrt(fx[i * 2] * fx[i * 2] + fx[i * 2 + 1] * fx[i * 2 + 1]));
  }

  x2[0] = (SCFLOAT)fx[0];

  for (i = 0; i < WIN_SIZE_2; i++) {
    if (x2[i] > xMax) {
      xMax = x2[i];
    }
  }

  j = 0;
  for (i = 8; i < WIN_SIZE_2 - 9; i++) {
    if (x2[i] > 0.5 * xMax) {
      peaks[j] = i + 1;
      j++;
    }
  }

  for (i = 0; i < j; i++) {
    mean = 0;
    mean2 = 0;
    tmp = peaks[i];

    for (k = tmp - 8; k <= tmp + 8; k++) {
      mean += x2[k - 1] * x2[k - 1];
      mean2 += x2[k - 1] * x2[k - 1] * x2[k - 1] * x2[k - 1];
    }

    mean /= 17;
    mean2 /= 17;

    en2 = (SCFLOAT)sqrt(mean) + (FLT_EPSILON);
    en1 = (SCFLOAT)sqrt(sqrt(mean2)) + (FLT_EPSILON);
    r = en1 / en2;

    if (r > rMax) {
      rMax = r;
    }
  }

  pFd = rMax;

  memcpy(mem_signal, &x[L_FRAME], MEM_SIZE * sizeof(SCFLOAT));
  memcpy(mem_exc, &x_exc[L_FRAME], MEM_SIZE * sizeof(SCFLOAT));

  result[0] = pTd;
  result[1] = pFd;

  return error;
}
