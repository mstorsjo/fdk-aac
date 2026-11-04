
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
#include "mathlib.h"
#include "iis_fft.h"
#include "cpuinfo.h"
#include "iisutillib.h"

#include "qmflib.h"
#include "qmflib_struct.h"
#include "qmflib_tables.h"

#define QMF_BANDS_MAX 128

#define MAX_L QMF_BANDS_MAX
#define MAX_L2 2 * MAX_L

static HANDLE_ERROR_INFO
QMFlib_CalculateAnalysis_NoOpt(HANDLE_QMFLIB_ANALYSIS hAnalysisQmf,
                               float const* const timeSig,
                               float* Sr,
                               float* Si);

static HANDLE_ERROR_INFO
QMFlib_CalculateSynthesis_NoOpt(HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf,
                                float const* const Sr,
                                float const* const Si,
                                float* timeSig);

HANDLE_ERROR_INFO
QMFlib_CreateAnalysisExt(
    HANDLE_QMFLIB_ANALYSIS* hAnalysisQmf,
    int nQmfBands,
    int partiallyComplex,
    QMF_FILTERMODE filtermode,
    int bDownsampledSBR) {
  HANDLE_ERROR_INFO hErr = noError;

  hErr = QMFlib_CreateAnalysis(
      hAnalysisQmf,
      nQmfBands,
      partiallyComplex);
  if (hErr != noError) return hErr;

  hErr = QMFlib_ConfigAnalysis(
      *hAnalysisQmf,
      filtermode,
      bDownsampledSBR);
  if (hErr != noError) return hErr;

  hErr = QMFlib_InitAnalysis(
      *hAnalysisQmf,
      nQmfBands);

  return hErr;
}

HANDLE_ERROR_INFO
QMFlib_CreateAnalysis(HANDLE_QMFLIB_ANALYSIS* hAnalysisQmf,
                      int maxQmfBands,
                      int partiallyComplex) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (NULL == (*hAnalysisQmf = (HANDLE_QMFLIB_ANALYSIS)iisCalloc(1, sizeof(QMFLIB_ANALYSIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for hAnalysisQmf.");
    }
  }

  (*hAnalysisQmf)->QMFlib_CalculateAnalysis_Ptr = QMFlib_CalculateAnalysis_NoOpt;
  if (error == noError) {
    (*hAnalysisQmf)->maxL = maxQmfBands;
    (*hAnalysisQmf)->L = 32;
    (*hAnalysisQmf)->filtermode = FM_SBRQMF;
    (*hAnalysisQmf)->bDownsampledSBR = 0;

    if (partiallyComplex) {
      error = iisUtil_ERROR(CDI, "Low power QMF is disabled");
    }
  }

  if (error == noError) {
    if (NULL == ((*hAnalysisQmf)->pTimeSigBuffer = (float*)iisCalloc(10 * maxQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for pTimeSigBuffer");
    }
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_ConfigAnalysis(HANDLE_QMFLIB_ANALYSIS hAnalysisQmf, QMF_FILTERMODE filtermode, int bDownsampledSBR) {
  HANDLE_ERROR_INFO error = noError;
  int qmfBands;

  if (hAnalysisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hAnalysisQmf");
    return error;
  }

  qmfBands = hAnalysisQmf->L;

  switch (filtermode) {
    case FM_SBRQMF:
      if (((qmfBands != 16) && (qmfBands != 24) && (qmfBands != 32) && (qmfBands != 64) && (qmfBands != 128)) ||
          (bDownsampledSBR && (qmfBands != 32)))
        error = iisUtil_ERROR(CDI, "Invalid number of QMF bands");
      break;
  }
  if (error != noError)
    return (error);

  hAnalysisQmf->filtermode = filtermode;
  hAnalysisQmf->bDownsampledSBR = bDownsampledSBR;

  switch (filtermode) {
    case FM_SBRQMF:
      switch (qmfBands) {
        case 16:
          hAnalysisQmf->pFilter = QMFlib_window160;
          break;
        case 24:
          hAnalysisQmf->pFilter = QMFlib_window240;
          break;
        case 32:
          hAnalysisQmf->pFilter = QMFlib_window320;
          break;
        case 64:
          hAnalysisQmf->pFilter = QMFlib_window640;
          break;
        case 128:
          hAnalysisQmf->pFilter = QMFlib_window1280;
          break;
      }
      break;
  }

  {
    switch (qmfBands) {
      case 4:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real4;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag4;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real4;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag4;
        switch (filtermode) {
          case FM_SBRQMF:
            error = iisUtil_ERROR(CDI, "Mode not implemented");
            break;
        }
        break;
      case 8:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real8;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag8;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real8;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag8;
        switch (filtermode) {
          case FM_SBRQMF:
            error = iisUtil_ERROR(CDI, "Mode not implemented");
            break;
        }
        break;
      case 16:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real16;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag16;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real16;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag16;
        switch (filtermode) {
          case FM_SBRQMF:
            hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real16_SBRQMF;
            hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag16_SBRQMF;
            break;
        }
        break;
      case 24:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real24;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag24;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real24;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag24;
        switch (filtermode) {
          case FM_SBRQMF:
            hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real24_SBRQMF;
            hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag24_SBRQMF;
            break;
        }
        break;
      case 32:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real32;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag32;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real32;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag32;
        switch (filtermode) {
          case FM_SBRQMF:
            if (bDownsampledSBR) {
              hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real32_SBRQMF_1;
              hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag32_SBRQMF_1;
            } else {
              hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real32_SBRQMF;
              hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag32_SBRQMF;
            }
            break;
        }
        break;
      case 64:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real64;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag64;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real64;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag64;
        switch (filtermode) {
          case FM_SBRQMF:
            hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real64_SBRQMF;
            hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag64_SBRQMF;

            break;
        }
        break;
      case 128:
        hAnalysisQmf->twiddle1Real = QMFlib_twiddle1Real128;
        hAnalysisQmf->twiddle1Imag = QMFlib_twiddle1Imag128;
        hAnalysisQmf->twiddle2Real = QMFlib_twiddle2Real128;
        hAnalysisQmf->twiddle2Imag = QMFlib_twiddle2Imag128;
        switch (filtermode) {
          case FM_SBRQMF:
            hAnalysisQmf->twiddle3Real = QMFlib_twiddle3Real128_SBRQMF;
            hAnalysisQmf->twiddle3Imag = QMFlib_twiddle3Imag128_SBRQMF;
            break;
        }
        break;
    }
  }

  hAnalysisQmf->filtermode = filtermode;
  hAnalysisQmf->bDownsampledSBR = bDownsampledSBR;

  return (error);
}

HANDLE_ERROR_INFO
QMFlib_InitAnalysis(HANDLE_QMFLIB_ANALYSIS hAnalysisQmf, int qmfBands) {
  HANDLE_ERROR_INFO error = noError;

  if (hAnalysisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hAnalysisQmf");
    return error;
  }

  if (qmfBands > hAnalysisQmf->maxL) {
    error = iisUtil_ERROR(CDI, "number of QMF bands exceeds specified maximum");
  }

  if (error == noError) {
    hAnalysisQmf->L = qmfBands;
    hAnalysisQmf->timeSigBufferIndex = 0;

    setFLOAT(0.0f, hAnalysisQmf->pTimeSigBuffer, qmfBands * 10);
  }

  error = QMFlib_ConfigAnalysis(hAnalysisQmf, hAnalysisQmf->filtermode, hAnalysisQmf->bDownsampledSBR);

  return error;
}

static HANDLE_ERROR_INFO
QMFlib_CalculateAnalysis_NoOpt(
    HANDLE_QMFLIB_ANALYSIS hAnalysisQmf,
    float const* const timeSig,
    float* Sr,
    float* Si) {
  HANDLE_ERROR_INFO error = noError;

  if (hAnalysisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hAnalysisQmf");
    return error;
  }

  {
    int i;
    const int L = hAnalysisQmf->L;
    const int L2 = 2 * L;
    const int L_2 = L >> 1;
    float const* C = hAnalysisQmf->pFilter;
    const int timeSigBufferLen = 5 * L2;
    const float scale = 64.f / L;
    float Y[MAX_L2];

    float* timeSigBuffer = hAnalysisQmf->pTimeSigBuffer;
    int timeSigBufferIndex = hAnalysisQmf->timeSigBufferIndex;

    for (i = L - 1; i >= 0; i--) {
      timeSigBuffer[timeSigBufferIndex + i] = timeSig[L - 1 - i];
    }

    for (i = 0; i < L2; i++) {
      float accu;

      accu = timeSigBuffer[timeSigBufferIndex] * C[(i + 0 * L2)];
      timeSigBufferIndex += L2;
      if (timeSigBufferIndex >= timeSigBufferLen) timeSigBufferIndex -= timeSigBufferLen;

      accu += timeSigBuffer[timeSigBufferIndex] * C[(i + 1 * L2)];
      timeSigBufferIndex += L2;
      if (timeSigBufferIndex >= timeSigBufferLen) timeSigBufferIndex -= timeSigBufferLen;

      accu += timeSigBuffer[timeSigBufferIndex] * C[(i + 2 * L2)];
      timeSigBufferIndex += L2;
      if (timeSigBufferIndex >= timeSigBufferLen) timeSigBufferIndex -= timeSigBufferLen;

      accu += timeSigBuffer[timeSigBufferIndex] * C[(i + 3 * L2)];
      timeSigBufferIndex += L2;
      if (timeSigBufferIndex >= timeSigBufferLen) timeSigBufferIndex -= timeSigBufferLen;

      accu += timeSigBuffer[timeSigBufferIndex] * C[(i + 4 * L2)];
      timeSigBufferIndex += L2 + 1;
      if (timeSigBufferIndex >= timeSigBufferLen) timeSigBufferIndex -= timeSigBufferLen;
      Y[i] = accu;
    }

    {
      const float *twiddle1Real, *twiddle1Imag, *twiddle2Real, *twiddle2Imag, *twiddle3Real, *twiddle3Imag;
      float tmp[MAX_L2], re, im, wre, wim;

      twiddle1Real = hAnalysisQmf->twiddle1Real;
      twiddle1Imag = hAnalysisQmf->twiddle1Imag;
      twiddle2Real = hAnalysisQmf->twiddle2Real;
      twiddle2Imag = hAnalysisQmf->twiddle2Imag;
      twiddle3Real = hAnalysisQmf->twiddle3Real;
      twiddle3Imag = hAnalysisQmf->twiddle3Imag;

      for (i = 0; i < L; i++) {
        re = Y[i] * scale;
        im = -Y[i + L] * scale;
        wre = twiddle1Real[i];
        wim = twiddle1Imag[i];

        tmp[2 * i] = re * wre - im * wim;
        tmp[2 * i + 1] = re * wim + im * wre;
      }

      iis_fftf(tmp, L);

      for (i = 0; i < L_2; i++) {
        re = tmp[2 * i];
        im = -tmp[2 * i + 1];

        wre = twiddle2Real[i * 2];
        wim = twiddle2Imag[i * 2];

        Sr[2 * i] = re * wre - im * wim;
        Si[2 * i] = re * wim + im * wre;

        re = tmp[2 * i + L];
        im = tmp[2 * i + L + 1];

        wre = twiddle2Real[L - 1 - 2 * i];
        wim = twiddle2Imag[L - 1 - 2 * i];

        Sr[L - 1 - 2 * i] = re * wre - im * wim;
        Si[L - 1 - 2 * i] = re * wim + im * wre;
      }

      for (i = 0; i < L; i++) {
        re = Sr[i];
        im = Si[i];

        wre = twiddle3Real[i];
        wim = twiddle3Imag[i];

        Sr[i] = re * wre + im * wim;
        Si[i] = im * wre - re * wim;
      }
    }

    hAnalysisQmf->timeSigBufferIndex = (hAnalysisQmf->timeSigBufferIndex + (timeSigBufferLen - L)) % (timeSigBufferLen);
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_DestroyAnalysis(HANDLE_QMFLIB_ANALYSIS* hAnalysisQmf) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (*hAnalysisQmf != NULL) {
      if ((*hAnalysisQmf)->pTimeSigBuffer != NULL) {
        iisFree((*hAnalysisQmf)->pTimeSigBuffer);
      }
      (*hAnalysisQmf)->pTimeSigBuffer = NULL;

      iisFree(*hAnalysisQmf);
    }
    *hAnalysisQmf = NULL;
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_CalculateAnalysis(HANDLE_QMFLIB_ANALYSIS hAnalysisQmf,
                         float const* const timeSig,
                         float* Sr,
                         float* Si) {
  return hAnalysisQmf->QMFlib_CalculateAnalysis_Ptr(hAnalysisQmf, timeSig, Sr, Si);
}

HANDLE_ERROR_INFO
QMFlib_CreateSynthesisExt(
    HANDLE_QMFLIB_SYNTHESIS* hSynthesisQmf,
    int nQmfBands,
    int partiallyComplex,
    QMF_FILTERMODE filtermode) {
  HANDLE_ERROR_INFO hErr = noError;

  hErr = QMFlib_CreateSynthesis(
      hSynthesisQmf,
      nQmfBands,
      partiallyComplex);
  if (hErr != noError) return hErr;

  hErr = QMFlib_ConfigSynthesis(
      *hSynthesisQmf,
      filtermode);
  if (hErr != noError) return hErr;

  hErr = QMFlib_InitSynthesis(
      *hSynthesisQmf,
      nQmfBands);

  return hErr;
}

HANDLE_ERROR_INFO
QMFlib_CreateSynthesis(HANDLE_QMFLIB_SYNTHESIS* hSynthesisQmf, int maxQmfBands, int partiallyComplex) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (NULL == (*hSynthesisQmf = (HANDLE_QMFLIB_SYNTHESIS)iisCalloc(1, sizeof(QMFLIB_SYNTHESIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for hSynthesisQmf");
    }
  }

  if (error == noError) {
    if (NULL == ((*hSynthesisQmf)->pFilterStates = (float*)iisCalloc(2 * 10 * maxQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for pFilterStates");
    }
  }

  (*hSynthesisQmf)->QMFlib_CalculateSynthesis_Ptr = QMFlib_CalculateSynthesis_NoOpt;
  if (error == noError) {
    (*hSynthesisQmf)->maxL = maxQmfBands;
    (*hSynthesisQmf)->L = 64;
    (*hSynthesisQmf)->filtermode = FM_SBRQMF;

    if (partiallyComplex) {
      error = iisUtil_ERROR(CDI, "Low power QMF is disabled");
    }
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_ConfigSynthesis(HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf,
                       QMF_FILTERMODE filtermode) {
  HANDLE_ERROR_INFO error = noError;
  int qmfBands;

  if (hSynthesisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hSynthesisQmf");
    return error;
  }

  qmfBands = hSynthesisQmf->L;

  switch (hSynthesisQmf->filtermode) {
    case FM_SBRQMF:
      if ((qmfBands != 16) && (qmfBands != 24) && (qmfBands != 32) && (qmfBands != 64) && (qmfBands != 128))
        error = iisUtil_ERROR(CDI, "Invalid number of QMF bands");
      break;
  }
  if (error != noError)
    return (error);

  hSynthesisQmf->filtermode = filtermode;

  switch (filtermode) {
    case FM_SBRQMF:
      switch (qmfBands) {
        case 16:
          hSynthesisQmf->pFilter = QMFlib_window160;
          break;
        case 24:
          hSynthesisQmf->pFilter = QMFlib_window240;
          break;
        case 32:
          hSynthesisQmf->pFilter = QMFlib_window320;
          break;
        case 64:
          hSynthesisQmf->pFilter = QMFlib_window640;
          break;
        case 128:
          hSynthesisQmf->pFilter = QMFlib_window1280;
          break;
      }
      break;
  }

  {
    switch (qmfBands) {
      case 4:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real4;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag4;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real4;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag4;
        switch (filtermode) {
          case FM_SBRQMF:
            error = iisUtil_ERROR(CDI, "Mode not implemented");
            break;
        }
        break;
      case 8:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real8;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag8;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real8;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag8;
        switch (filtermode) {
          case FM_SBRQMF:
            error = iisUtil_ERROR(CDI, "Mode not implemented");
            break;
        }
        break;
      case 16:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real16;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag16;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real16;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag16;
        switch (filtermode) {
          case FM_SBRQMF:
            hSynthesisQmf->twiddle3Real = QMFlib_twiddle3Real16_SBRQMF_S;
            hSynthesisQmf->twiddle3Imag = QMFlib_twiddle3Imag16_SBRQMF_S;
            break;
        }
        break;
      case 24:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real24;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag24;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real24;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag24;
        switch (filtermode) {
          case FM_SBRQMF:
            hSynthesisQmf->twiddle3Real = QMFlib_twiddle3Real24_SBRQMF_S;
            hSynthesisQmf->twiddle3Imag = QMFlib_twiddle3Imag24_SBRQMF_S;
            break;
        }
        break;
      case 32:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real32;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag32;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real32;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag32;
        switch (filtermode) {
          case FM_SBRQMF:
            hSynthesisQmf->twiddle3Real = QMFlib_twiddle3Real32_SBRQMF_S;
            hSynthesisQmf->twiddle3Imag = QMFlib_twiddle3Imag32_SBRQMF_S;
            break;
        }
        break;
      case 64:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real64;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag64;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real64;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag64;
        switch (filtermode) {
          case FM_SBRQMF:
            hSynthesisQmf->twiddle3Real = QMFlib_twiddle3Real64_SBRQMF_S;
            hSynthesisQmf->twiddle3Imag = QMFlib_twiddle3Imag64_SBRQMF_S;
            break;
        }
        break;
      case 128:
        hSynthesisQmf->twiddle1Real = QMFlib_twiddle1Real128;
        hSynthesisQmf->twiddle1Imag = QMFlib_twiddle1Imag128;
        hSynthesisQmf->twiddle2Real = QMFlib_twiddle2Real128;
        hSynthesisQmf->twiddle2Imag = QMFlib_twiddle2Imag128;
        switch (filtermode) {
          case FM_SBRQMF:
            hSynthesisQmf->twiddle3Real = QMFlib_twiddle3Real128_SBRQMF_S;
            hSynthesisQmf->twiddle3Imag = QMFlib_twiddle3Imag128_SBRQMF_S;
            break;
        }
        break;
    }
  }
  return (error);
}

HANDLE_ERROR_INFO
QMFlib_InitSynthesis(HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf, int qmfBands) {
  HANDLE_ERROR_INFO error = noError;

  if (hSynthesisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hSynthesisQmf.");
    return error;
  }

  if (qmfBands > hSynthesisQmf->maxL) {
    error = iisUtil_ERROR(CDI, "number of QMF bands exceeds specified maximum");
  }

  if (error == noError) {
    hSynthesisQmf->L = qmfBands;
    hSynthesisQmf->filterStateIndex = 0;

    setFLOAT(0.0f, hSynthesisQmf->pFilterStates, 2 * qmfBands * 10);
  }

  error = QMFlib_ConfigSynthesis(hSynthesisQmf, hSynthesisQmf->filtermode);

  return error;
}

static HANDLE_ERROR_INFO
QMFlib_CalculateSynthesis_NoOpt(
    HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf,
    float const* const Sr,
    float const* const Si,
    float* timeSig) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  if (hSynthesisQmf == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hSynthesisQmf.");
    return error;
  }

  {
    float* filterStates = hSynthesisQmf->pFilterStates;
    int filterStateIndex = hSynthesisQmf->filterStateIndex;

    const int L = hSynthesisQmf->L;
    const int L2 = 2 * L;
    const int L_2 = L >> 1;
    float const* C = hSynthesisQmf->pFilter;
    const int filterBufferLen = 2 * 5 * L2;

    {
      const float *twiddle1Real, *twiddle1Imag, *twiddle2Real, *twiddle2Imag, *twiddle3Real, *twiddle3Imag;
      float tmp[MAX_L2], re, im, tmp_re, tmp_im, wre, wim;
      const float scale = 1.f / 64.f;

      twiddle1Real = hSynthesisQmf->twiddle1Real;
      twiddle1Imag = hSynthesisQmf->twiddle1Imag;
      twiddle2Real = hSynthesisQmf->twiddle2Real;
      twiddle2Imag = hSynthesisQmf->twiddle2Imag;
      twiddle3Real = hSynthesisQmf->twiddle3Real;
      twiddle3Imag = hSynthesisQmf->twiddle3Imag;

      for (i = 0; i < L_2; i++) {
        re = Sr[2 * i];
        im = Si[2 * i];

        wre = twiddle3Real[2 * i];
        wim = twiddle3Imag[2 * i];

        tmp_re = re * wre + im * wim;
        tmp_im = im * wre - re * wim;

        re = tmp_re;
        im = -tmp_im;

        wre = -twiddle2Real[i * 2];
        wim = twiddle2Imag[i * 2];

        tmp[2 * i] = re * wre - im * wim;
        tmp[2 * i + 1] = re * wim + im * wre;

        re = Sr[L - 1 - 2 * i];
        im = Si[L - 1 - 2 * i];

        wre = twiddle3Real[L - 1 - 2 * i];
        wim = twiddle3Imag[L - 1 - 2 * i];

        tmp_re = re * wre + im * wim;
        tmp_im = im * wre - re * wim;

        re = tmp_re;
        im = -tmp_im;

        wre = -twiddle2Real[L - 1 - 2 * i];
        wim = twiddle2Imag[L - 1 - 2 * i];

        tmp[L + 2 * i] = re * wre - im * wim;
        tmp[L + 2 * i + 1] = -(re * wim + im * wre);
      }

      iis_fftf(tmp, L);

      for (i = 0; i < L; i++) {
        re = tmp[2 * i];
        im = tmp[2 * i + 1];

        wre = scale * twiddle1Real[i];
        wim = scale * twiddle1Imag[i];

        filterStates[filterStateIndex + i] = re * wre - im * wim;
        filterStates[filterStateIndex + i + L] = re * wim + im * wre;
      }
    }

    for (i = 0; i < L; i++) {
      float realAccu;
      realAccu = filterStates[filterStateIndex] * C[(0 * L + i)];
      filterStateIndex += L2 + L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;
      realAccu += filterStates[filterStateIndex] * C[(1 * L + i)];
      filterStateIndex += L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;

      realAccu += filterStates[filterStateIndex] * C[(2 * L + i)];
      filterStateIndex += L2 + L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;
      realAccu += filterStates[filterStateIndex] * C[(3 * L + i)];
      filterStateIndex += L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;

      realAccu += filterStates[filterStateIndex] * C[(4 * L + i)];
      filterStateIndex += L2 + L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;
      realAccu += filterStates[filterStateIndex] * C[(5 * L + i)];
      filterStateIndex += L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;

      realAccu += filterStates[filterStateIndex] * C[(6 * L + i)];
      filterStateIndex += L2 + L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;
      realAccu += filterStates[filterStateIndex] * C[(7 * L + i)];
      filterStateIndex += L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;

      realAccu += filterStates[filterStateIndex] * C[(8 * L + i)];
      filterStateIndex += L2 + L;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;
      realAccu += filterStates[filterStateIndex] * C[(9 * L + i)];
      filterStateIndex += L + 1;
      if (filterStateIndex >= filterBufferLen) filterStateIndex -= filterBufferLen;

      timeSig[i] = realAccu;
    }
    hSynthesisQmf->filterStateIndex = (hSynthesisQmf->filterStateIndex + (filterBufferLen - L2)) % (filterBufferLen);
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_DestroySynthesis(HANDLE_QMFLIB_SYNTHESIS* hSynthesisQmf) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (*hSynthesisQmf != NULL) {
      if ((*hSynthesisQmf)->pFilterStates != NULL) {
        iisFree((*hSynthesisQmf)->pFilterStates);
      }
      (*hSynthesisQmf)->pFilterStates = NULL;

      iisFree(*hSynthesisQmf);
    }
    *hSynthesisQmf = NULL;
  }

  return error;
}

HANDLE_ERROR_INFO
QMFlib_CalculateSynthesis(HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf,
                          float const* const Sr,
                          float const* const Si,
                          float* timeSig) {
  return hSynthesisQmf->QMFlib_CalculateSynthesis_Ptr(hSynthesisQmf, Sr, Si, timeSig);
}

int QMFlib_GetFilterbankPrototype(int resolution,
                                  QMF_FILTERMODE filtermode,
                                  float* prototype) {
  const float* C;
  int i;

  switch (filtermode) {
    case FM_SBRQMF:
      switch (resolution) {
        case 128:
          C = QMFlib_window1280;
          break;
        case 64:
          C = QMFlib_window640;
          break;
        case 32:
          C = QMFlib_window320;
          break;
        case 24:
          C = QMFlib_window240;
          break;
        case 16:
          C = QMFlib_window160;
          break;
        default:
          return 1;
      }
      break;

    default:
      return (1);
  }

  for (i = 0; i < 10 * resolution; i++) {
    prototype[i] = (float)C[i];
  }

  for (i = 2 * resolution; i < 4 * resolution; i++) {
    prototype[i] = -prototype[i];
  }

  for (i = 6 * resolution; i < 8 * resolution; i++) {
    prototype[i] = -prototype[i];
  }
  return (0);
}

float*
QMFlib_GetSynthesisQmfStates(HANDLE_QMFLIB_SYNTHESIS hSynthesisQmf) {
  return (hSynthesisQmf->pFilterStates);
}
