
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

#include "iisutillib.h"
#include "mathlib.h"
#include "convolve.h"
#include "iis_fft.h"

static float
SingleFIRFilterMod_NoOpt(const HANDLE_FIR_FILTER hFIRFilter,
                         const float* inBuffer) {
  int i;
  float* coef = hFIRFilter->filtCoef;
  float y = 0.0f;

  for (i = 0; i < hFIRFilter->filterLength; i++) {
    y += (*coef++) * (*inBuffer);
    inBuffer--;
  }

  return y;
}

HANDLE_ERROR_INFO
CreateFIRFilter(HANDLE_FIR_FILTER* hFIRFilter,
                int transfSize,
                int filterLength,
                const float* coef) {
  int i;
  HANDLE_IIS_FFT hIisFft_R = NULL;

  *hFIRFilter = (HANDLE_FIR_FILTER)iisCalloc(sizeof(FIR_FILTER), 1);
  if (!*hFIRFilter)
    return iisUtil_ERROR(CDI, "out of memory");

  if (transfSize == 0) {
    if (filterLength < 3)
      return iisUtil_ERROR(CDI, "invalid number of filter coefficients");

    transfSize = filterLength;

  } else {
    if (filterLength > transfSize)
      return iisUtil_ERROR(CDI, "Filter length exceeds transformation size !");
  }

  (*hFIRFilter)->filtCoef = (float*)iisCalloc(sizeof(float), (unsigned int)(transfSize + 3));
  if (!(*hFIRFilter)->filtCoef)
    return iisUtil_ERROR(CDI, "out of memory");

  (*hFIRFilter)->SingleFIRFilterMod_Ptr = &SingleFIRFilterMod_NoOpt;

  for (i = 0; i < filterLength; i++)
    (*hFIRFilter)->filtCoef[i] = coef[i];

  if (transfSize > filterLength) {
    (*hFIRFilter)->filtCoefFFT = (float*)iisCalloc((unsigned int)transfSize, sizeof(float));
    if (!(*hFIRFilter)->filtCoefFFT) {
      return iisUtil_ERROR(CDI, "out of memory");
    }

    do {
      (*hFIRFilter)->filtCoef[i] = 0.0f;
      i++;
    } while (i < transfSize);

    if (IIS_FFT_NO_ERROR != (IIS_RFFT_Create(&hIisFft_R, transfSize, IIS_FFT_FWD))) {
      return iisUtil_ERROR(CDI, "IIS_RFFT_Create failed!");
    }
    if (IIS_FFT_NO_ERROR != (IIS_FFT_Apply_RFFT(hIisFft_R, (*hFIRFilter)->filtCoef, (*hFIRFilter)->filtCoefFFT))) {
      return iisUtil_ERROR(CDI, "IIS_FFT_Apply_RFFT failed!");
    }
    if (IIS_FFT_NO_ERROR != (IIS_RFFT_Destroy(&hIisFft_R))) {
      return iisUtil_ERROR(CDI, "IIS_RFFT_Destroy failed!");
    }

    (*hFIRFilter)->filterLengthFFT = transfSize;
    (*hFIRFilter)->filterLength = filterLength;

  } else {
    (*hFIRFilter)->filterLength = filterLength;
  }

  return noError;
}

void DeleteFIRFilter(HANDLE_FIR_FILTER hFIRFilter) {
  if (hFIRFilter) {
    if (hFIRFilter->filtCoef)
      iisFree(hFIRFilter->filtCoef);

    if (hFIRFilter->filtCoefFFT)
      iisFree(hFIRFilter->filtCoefFFT);

    iisFree(hFIRFilter);
  }
}

HANDLE_ERROR_INFO
CreateConvolutionChannel(HANDLE_CONV_CHAN* hConvChan,
                         int overlapLength) {
  *hConvChan = (HANDLE_CONV_CHAN)iisCalloc(sizeof(CONV_CHAN), 1);
  if (!(*hConvChan))
    return iisUtil_ERROR(CDI, "out of memory");

  (*hConvChan)->overlapBuffer = (float*)iisCalloc(sizeof(float), (unsigned int)overlapLength);

  (*hConvChan)->overlapLength = overlapLength;

  return noError;
}

void DeleteConvolutionChannel(HANDLE_CONV_CHAN hConvChan) {
  if (hConvChan) {
    if (hConvChan->overlapBuffer)
      iisFree(hConvChan->overlapBuffer);

    iisFree(hConvChan);
  }
}

void ForwardTransform(float* inFiltBuf, int inBlockSize, int fftLength, HANDLE_IIS_FFT hIisFft_R) {
  setFLOAT(0.f, inFiltBuf + inBlockSize, fftLength - inBlockSize);

  IIS_FFT_Apply_RFFT(hIisFft_R, inFiltBuf, inFiltBuf);
}

void InverseTransform(HANDLE_CONV_CHAN hConv, float* outFiltBuf, const int samplesValid, HANDLE_IIS_FFT hIisFft_RBwd) {
  int i;
  float* overlapBuf;

  overlapBuf = hConv->overlapBuffer;

  IIS_FFT_Apply_RFFT(hIisFft_RBwd, outFiltBuf, outFiltBuf);

  for (i = 0; i < hConv->overlapLength; i++) {
    outFiltBuf[i] += overlapBuf[i];
    if (samplesValid + i >= hConv->overlapLength) {
      overlapBuf[i] = outFiltBuf[samplesValid + i];
    } else {
      overlapBuf[i] = outFiltBuf[samplesValid + i] + overlapBuf[samplesValid + i];
    }
  }
}

void FastConvolution(const HANDLE_FIR_FILTER hFIRFilter,
                     const float* inFiltBuf,
                     float* outFiltBuf) {
  int i;
  const float* filtCoef;

  filtCoef = hFIRFilter->filtCoefFFT;

  outFiltBuf[0] = inFiltBuf[0] * filtCoef[0];
  outFiltBuf[1] = inFiltBuf[1] * filtCoef[1];

  for (i = 2; i < hFIRFilter->filterLengthFFT; i += 2) {
    outFiltBuf[i] = inFiltBuf[i] * filtCoef[i] - inFiltBuf[i + 1] * filtCoef[i + 1];
    outFiltBuf[i + 1] = inFiltBuf[i] * filtCoef[i + 1] + inFiltBuf[i + 1] * filtCoef[i];
  }
}

float SingleFIRFilter(const HANDLE_FIR_FILTER hFIRFilter,
                      const float* inBuffer,
                      int strides) {
  int i;
  float* coef = hFIRFilter->filtCoef;
  float y = 0.0f;

  for (i = 0; i < hFIRFilter->filterLength; i++) {
    y += (*coef++) * (*inBuffer);

    inBuffer -= strides;
  }

  return y;
}

float SingleFIRFilterMod(const HANDLE_FIR_FILTER hFIRFilter,
                         const float* inBuffer) {
  return hFIRFilter->SingleFIRFilterMod_Ptr(hFIRFilter, inBuffer);
}
