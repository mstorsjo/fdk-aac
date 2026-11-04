
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

#include "polyfilt.h"
#include "convolve.h"
#include "iisutillib.h"
#include "iis_fft.h"

static HANDLE_ERROR_INFO
CreatePolyphaseFilters(HANDLE_FIR_FILTER** hPolyFilters, int L, int M,
                       int transformLength, int filterLength,
                       double* coef, double gain) {
  int i, index;
  int tmpIndex = 0;
  int l, m;
  int filterIndex;
  float* tmpCoef;
  HANDLE_FIR_FILTER* hFIRFilter;
  HANDLE_ERROR_INFO errorInfo;

  tmpCoef = (float*)iisCalloc(sizeof(float), (unsigned int)filterLength);
  if (!tmpCoef)
    return iisUtil_ERROR(CDI, "out of memory !");

  hFIRFilter = (HANDLE_FIR_FILTER*)iisCalloc(sizeof(HANDLE_FIR_FILTER), (unsigned int)(L * M));
  if (!hFIRFilter)
    return iisUtil_ERROR(CDI, "out of memory !");

  for (m = 0; m < M; m++) {
    for (l = 0; l < L; l++) {
      i = 0;
      tmpIndex = l * M - m * L;
      if (tmpIndex < 0) {
        tmpIndex += L * M;
        i = 1;
      }
      tmpCoef[0] = 0;

      for (index = tmpIndex; index < filterLength; index += L * M) {
        tmpCoef[i++] = (float)(gain * coef[index]);
      }

      filterIndex = m * L;

      errorInfo = CreateFIRFilter(&(hFIRFilter[filterIndex + l]),
                                  transformLength, i, tmpCoef);

      if (errorInfo != noError) {
        return handBack(errorInfo);
      }
    }
  }

  *hPolyFilters = hFIRFilter;

  iisFree(tmpCoef);
  tmpCoef = NULL;

  return noError;
}

static void
DeletePolyphaseFilters(HANDLE_FIR_FILTER* hFIRFilter,
                       int numFilters) {
  int i;

  if (hFIRFilter) {
    for (i = 0; i < numFilters; i++)
      DeleteFIRFilter(hFIRFilter[i]);

    iisFree(hFIRFilter);
  }
}

HANDLE_ERROR_INFO
CreatePolyphaseChannel(HANDLE_POLYPHASE* hPolyphaseChannel,
                       int L, int M, int transformLength,
                       int filterLen, double* coef, double gain) {
  HANDLE_POLYPHASE hPolyphase;
  HANDLE_ERROR_INFO errorInfo;
  int i;

  if (coef == NULL)
    return iisUtil_ERROR(CDI, "invalid parameter");

  hPolyphase = (POLYPHASE*)iisCalloc(sizeof(POLYPHASE), 1);
  if (!hPolyphase)
    return iisUtil_ERROR(CDI, "out of memory");

  hPolyphase->M = M;
  hPolyphase->L = L;

  hPolyphase->filtBlockSize = filterLen / (L * M);

  if (filterLen % (L * M))
    hPolyphase->filtBlockSize++;

  if (transformLength) {
    hPolyphase->inFiltBuf = (float*)iisCalloc(sizeof(float), (unsigned int)transformLength);
    if (!hPolyphase->inFiltBuf)
      return iisUtil_ERROR(CDI, "out of memory !");

    hPolyphase->outFiltBuf = (float**)iisCalloc(L + 1, sizeof(float*));
    if (!hPolyphase->outFiltBuf)
      return iisUtil_ERROR(CDI, "out of memory !");

    for (i = 0; i < L + 1; i++) {
      hPolyphase->outFiltBuf[i] = (float*)iisCalloc((unsigned int)transformLength, sizeof(float));
      if (!hPolyphase->outFiltBuf[i])
        return iisUtil_ERROR(CDI, "out of memory !");
    }

    hPolyphase->filtBlockSize = transformLength - hPolyphase->filtBlockSize;
    if (IIS_FFT_NO_ERROR != (IIS_RFFT_Create(&hPolyphase->hIisFft_R, transformLength, IIS_FFT_FWD))) {
      return iisUtil_ERROR(CDI, "IIS_RFFT_Create failed!");
    }

    if (IIS_FFT_NO_ERROR != (IIS_RFFT_Create(&hPolyphase->hIisFft_RBwd, transformLength, IIS_FFT_BWD))) {
      return iisUtil_ERROR(CDI, "IIS_RFFT_Create failed!");
    }
  }

  errorInfo = CreatePolyphaseFilters(&(hPolyphase->hPolyFilters), L, M,
                                     transformLength, filterLen, coef, gain);
  if (errorInfo != noError)
    return handBack(errorInfo);

  *hPolyphaseChannel = hPolyphase;

  return noError;
}

void DeletePolyphaseChannel(HANDLE_POLYPHASE hPolyphaseChannel) {
  int numFilters;
  int L, i;

  if (hPolyphaseChannel) {
    L = hPolyphaseChannel->L;
    if (hPolyphaseChannel->inFiltBuf)
      iisFree(hPolyphaseChannel->inFiltBuf);

    if (hPolyphaseChannel->outFiltBuf) {
      for (i = 0; i < L + 1; i++) {
        if (hPolyphaseChannel->outFiltBuf[i]) {
          iisFree(hPolyphaseChannel->outFiltBuf[i]);
        }
      }
      iisFree(hPolyphaseChannel->outFiltBuf);
    }

    if (hPolyphaseChannel->hIisFft_R)
      IIS_RFFT_Destroy(&hPolyphaseChannel->hIisFft_R);

    if (hPolyphaseChannel->hIisFft_RBwd)
      IIS_RFFT_Destroy(&hPolyphaseChannel->hIisFft_RBwd);

    numFilters = hPolyphaseChannel->L * hPolyphaseChannel->M;

    DeletePolyphaseFilters(hPolyphaseChannel->hPolyFilters, numFilters);

    iisFree(hPolyphaseChannel);
  }
}
