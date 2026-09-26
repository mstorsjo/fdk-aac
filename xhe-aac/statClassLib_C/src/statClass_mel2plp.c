
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
#include "statClass_mel2plp.h"
#include "statClass_features.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "statClass_lpc.h"

#include "iis_fft.h"

#define COMPRESS 0.33
#define MEL_SIZE 40
#define N_BANDS 40
#define MODEL_ORDER 16
#define FFT_LEN 78
#define M 16
#define LIFT 0.6

SCFLOAT bandCfHz[40] = {
    199.99996666f, 266.66663332f, 333.33329998f, 399.99996664f, 466.66663330f, 533.33329996f, 599.99996662f, 666.66663328f, 733.33329994f, 799.99996660f,
    866.66663326f, 933.33329992f, 999.75891088f, 1070.91205249f, 1147.12918454f, 1228.77071274f, 1316.22269300f, 1409.89865693f, 1510.24156731f, 1617.72591273f,
    1732.85995125f, 1856.18811384f, 1988.29357876f, 2129.80102925f, 2281.37960744f, 2443.74607852f, 2617.66822005f, 2803.96845257f, 3003.52772853f, 3217.28969803f,
    3446.26517102f, 3691.53689712f, 3954.26468555f, 4235.69088950f, 4537.14628082f, 4860.05634277f, 5205.94801070f, 5576.45689241f, 5973.33500237f, 6398.45904649f};

typedef struct _statclass_mel2plp {
  float* levinson;
  float* levinsonNorm;
  float predError;

  float* fftReal;
  float* eql;
  float* cep;
  float* liftWeights;

  HANDLE_IIS_FFT hIisFft;

} STATCLASS_MEL2PLP;

float STATCLASS_mel2plp_lev_dur(
    float* a,
    float* r,
    int m) {
  float buf[M];
  float* rc;
  float s, at, err;
  int i, j, l;

  rc = &buf[0];

  rc[0] = (-r[1]) / r[0];
  a[0] = 1.0;
  a[1] = rc[0];
  err = r[0] + r[1] * rc[0];

  for (i = 2; i <= m; i++) {
    s = 0.0;
    for (j = 0; j < i; j++) s += r[i - j] * a[j];
    rc[i - 1] = (-s) / (err);
    for (j = 1; j <= (i / 2); j++) {
      l = i - j;
      at = a[j] + rc[i - 1] * a[l];
      a[l] += rc[i - 1] * a[j];
      a[j] = at;
    }
    a[i] = rc[i - 1];
    err += rc[i - 1] * s;
    if (err <= 0.0)
      err = 0.01f;
  }

  return err;
}

STATCLASS_ERROR_CODE STATCLASS_Mel2Plp_Open(HANDLE_STATCLASS_MEL2PLP* hMel2Plp) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  float fsq[N_BANDS], fTmp[N_BANDS];
  int i;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == hMel2Plp) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      if (NULL == (*hMel2Plp = (STATCLASS_MEL2PLP*)iisCalloc(1, sizeof(STATCLASS_MEL2PLP)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->eql = (float*)iisCalloc(N_BANDS, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->fftReal = (float*)iisCalloc(FFT_LEN, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->levinson = (float*)iisCalloc(MODEL_ORDER + 1, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->levinsonNorm = (float*)iisCalloc(MODEL_ORDER + 1, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->cep = (float*)iisCalloc(MODEL_ORDER + 1, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hMel2Plp)->liftWeights = (float*)iisCalloc(MODEL_ORDER + 1, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    } else {
      (*hMel2Plp)->liftWeights[0] = 1.0f;
      for (i = 1; i < (MODEL_ORDER + 1); i++) {
        (*hMel2Plp)->liftWeights[i] = (float)pow(i, LIFT);
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < N_BANDS; i++) {
      fsq[i] = (float)(bandCfHz[i] * bandCfHz[i]);
      fTmp[i] = fsq[i] + 160000;
      (*hMel2Plp)->eql[i] = ((fsq[i] / fTmp[i]) * (fsq[i] / fTmp[i])) * ((fsq[i] + 1440000) / (fsq[i] + 9610000));
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    IIS_FFT_ERROR err;
    err = IIS_CFFT_Create(&((*hMel2Plp)->hIisFft), FFT_LEN, 1);
    if (err != IIS_FFT_NO_ERROR)
      error = STATCLASS_MEMORY_ALLOC_ERROR;
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Mel2Plp_Advance(HANDLE_STATCLASS_MEL2PLP hMel2Plp,
                                               SCFLOAT* pMel,
                                               SCFLOAT* pPlp) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  float z[N_BANDS], sum, levinsonTmp, complexSignal[2 * FFT_LEN];
  int i, j;

  for (i = 0; i < N_BANDS; i++) {
    z[i] = hMel2Plp->eql[i] * (float)pMel[i];
    z[i] = (float)pow(z[i], COMPRESS);
  }

  z[0] = z[1];
  z[N_BANDS - 1] = z[N_BANDS - 2];

  for (i = 0; i < N_BANDS; i++) {
    hMel2Plp->fftReal[i] = z[i];
  }

  for (j = 0; i < FFT_LEN; i++) {
    hMel2Plp->fftReal[i] = z[(N_BANDS - 2) - j];
    j++;
  }

  for (i = 0, j = 0; i < FFT_LEN; i++) {
    complexSignal[2 * i] = hMel2Plp->fftReal[i];
    complexSignal[2 * i + 1] = 0.0f;
  }

  {
    float tmpRe[FFT_LEN];
    float tmpIm[FFT_LEN];

    for (i = 0; i < FFT_LEN; i++) {
      tmpRe[i] = complexSignal[2 * i];
      tmpIm[i] = complexSignal[2 * i + 1];
    }

    IIS_FFT_Apply_CFFT(hMel2Plp->hIisFft, tmpRe, tmpIm, tmpRe, tmpIm);

    for (i = 0; i < FFT_LEN; i++) {
      hMel2Plp->fftReal[i] = tmpRe[i] / FFT_LEN;
    }
  }

  if (hMel2Plp->fftReal[0] < 1.0f) {
    hMel2Plp->fftReal[0] = 1.0f;
  }

  hMel2Plp->predError = STATCLASS_mel2plp_lev_dur(hMel2Plp->levinson, hMel2Plp->fftReal, MODEL_ORDER);

  for (i = 0; i < (MODEL_ORDER + 1); i++) {
    hMel2Plp->levinsonNorm[i] = hMel2Plp->levinson[i] / hMel2Plp->predError;
  }

  hMel2Plp->cep[0] = -((float)log(hMel2Plp->levinsonNorm[0]));

  levinsonTmp = hMel2Plp->levinsonNorm[0];
  for (i = 0; i < (MODEL_ORDER + 1); i++) {
    hMel2Plp->levinsonNorm[i] = hMel2Plp->levinsonNorm[i] / levinsonTmp;
  }

  for (i = 1; i < 17; i++) {
    sum = 0.0;
    for (j = 1; j <= i; j++) {
      sum = sum + (i - j) * hMel2Plp->levinsonNorm[j] * hMel2Plp->cep[i - j];
    }
    hMel2Plp->cep[i] = -(hMel2Plp->levinsonNorm[i] + sum / i);
  }

  for (i = 0; i < (MODEL_ORDER + 1); i++) {
    pPlp[i] = (SCFLOAT)(hMel2Plp->cep[i] * hMel2Plp->liftWeights[i]);
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Mel2Plp_Close(HANDLE_STATCLASS_MEL2PLP* hMel2Plp) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (NULL == (*hMel2Plp)->eql) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->eql);
  }

  if (NULL == (*hMel2Plp)->fftReal) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->fftReal);
  }

  if (NULL == (*hMel2Plp)->levinson) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->levinson);
  }

  if (NULL == (*hMel2Plp)->levinsonNorm) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->levinsonNorm);
  }

  if (NULL == (*hMel2Plp)->cep) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->cep);
  }

  if (NULL == (*hMel2Plp)->liftWeights) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hMel2Plp)->liftWeights);
  }

  if (NULL == (*hMel2Plp)->hIisFft) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    IIS_FFT_ERROR err;
    err = IIS_CFFT_Destroy(&((*hMel2Plp)->hIisFft));
    if (err != IIS_FFT_NO_ERROR)
      error = STATCLASS_INVALID_POINTER;
  }

  if (NULL == (*hMel2Plp)) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree(*hMel2Plp);
  }

  return error;
}
