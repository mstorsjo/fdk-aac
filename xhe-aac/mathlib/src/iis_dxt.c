
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
#include <assert.h>
#include <stdlib.h>

#include "mathlib.h"
#include "iis_fft.h"
#include "iis_dxt.h"
#include "iisutillib.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440084436210485
#endif

typedef struct T_IIS_DXT_KERNEL {
  IIS_KERNEL kernel;
  float *modul;
  HANDLE_IIS_FFT hIisFft;
} IIS_DXT_KERNEL;

typedef struct T_IIS_DXT {
  IIS_DXT_KERNEL *pKernel[IIS_MAX_KERNELS];
  int len;
  HANDLE_IIS_FFT hIisFftForward;
  HANDLE_IIS_FFT hIisFftBackward;
  HANDLE_IIS_FFT hIisFft4;
  float *modulDXT23;
  float *modulDXT4;
  float *pInOutReIm;
} IIS_DXT;

static IIS_DXT_ERROR validateKernel(IIS_KERNEL kernel) {
  switch (kernel) {
    case IIS_DCT_II:
      break;
    case IIS_DST_II:
      break;
    case IIS_DCT_III:
      break;
    case IIS_DST_III:
      break;
    case IIS_DCT_IV:
      break;
    case IIS_DST_IV:
      break;
    case IIS_INVALID:
    default:
      return IIS_DXT_ERROR_UNSUPPORTED;
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DXT_Create(
    HANDLE_IIS_DXT *const phIisDxt,
    IIS_KERNEL const kernel,
    int const len) {
  HANDLE_IIS_DXT hIisDxt = NULL;
  IIS_DXT_ERROR dxtErr = IIS_DXT_NO_ERROR;
  int i = 0;

  if (phIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  if (len & 0X01) {
    return IIS_DXT_ERROR_LENGTH;
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    dxtErr = validateKernel(kernel);
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    *phIisDxt = NULL;

    hIisDxt = (HANDLE_IIS_DXT)iisCalloc(1, sizeof(IIS_DXT));
    if (hIisDxt == NULL) {
      dxtErr = IIS_DXT_ERROR_MEMORY;
    }
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    hIisDxt->pInOutReIm = (float *)iisCalloc(2 * len, sizeof(float));
    if (hIisDxt->pInOutReIm == NULL) {
      dxtErr = IIS_DXT_ERROR_MEMORY;
    }
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    hIisDxt->len = len;
    hIisDxt->modulDXT23 = NULL;
    hIisDxt->modulDXT4 = NULL;
    for (i = 0; i < IIS_MAX_KERNELS; i++)
      hIisDxt->pKernel[i] = NULL;
    *phIisDxt = hIisDxt;

  } else {
    if (hIisDxt) iisFree(hIisDxt);
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    dxtErr = IIS_DXT_AddAdditionalKernel(hIisDxt, kernel, len);
  }

  return dxtErr;
}

IIS_DXT_ERROR IIS_DXT_AddAdditionalKernel(
    HANDLE_IIS_DXT hIisDxt,
    IIS_KERNEL const kernel,
    int const len) {
  IIS_DXT_ERROR dxtErr = IIS_DXT_NO_ERROR;
  IIS_FFT_ERROR err = IIS_FFT_NO_ERROR;
  IIS_DXT_KERNEL *pKernel = NULL;
  int i;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  dxtErr = validateKernel(kernel);
  if (dxtErr != IIS_DXT_NO_ERROR) {
    return dxtErr;
  }

  if (hIisDxt->pKernel[kernel] != NULL) {
    return IIS_DXT_NO_ERROR;
  }

  hIisDxt->pKernel[kernel] = (IIS_DXT_KERNEL *)iisCalloc(1, sizeof(IIS_DXT_KERNEL));
  if (hIisDxt->pKernel[kernel] == NULL) {
    dxtErr = IIS_DXT_ERROR_MEMORY;
  }
  if (dxtErr == IIS_DXT_NO_ERROR) {
    pKernel = hIisDxt->pKernel[kernel];
    pKernel->kernel = kernel;
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    assert(len == hIisDxt->len);
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    switch (kernel) {
      case IIS_DCT_II:
      case IIS_DST_II:
      case IIS_DCT_III:
      case IIS_DST_III:
        if (hIisDxt->modulDXT23 == NULL) {
          hIisDxt->modulDXT23 = (float *)iisCalloc(2 * len, sizeof(float));
          if (hIisDxt->modulDXT23 == NULL) {
            dxtErr = IIS_DXT_ERROR_MEMORY;
          } else {
            for (i = 0; i < len; i++) {
              hIisDxt->modulDXT23[2 * i] = (float)cos((M_PI / (2 * len)) * i);
              hIisDxt->modulDXT23[2 * i + 1] = (float)sin((M_PI / (2 * len)) * i);
            }
          }
        }
        if (dxtErr == IIS_DXT_NO_ERROR) {
          pKernel->modul = hIisDxt->modulDXT23;
        }
        break;
      case IIS_DCT_IV:
      case IIS_DST_IV:
        if (hIisDxt->modulDXT4 == NULL) {
          hIisDxt->modulDXT4 = (float *)iisCalloc(len, sizeof(float));
          if (hIisDxt->modulDXT4 == NULL) {
            dxtErr = IIS_DXT_ERROR_MEMORY;
          } else {
            for (i = 0; i < len; i++) {
              hIisDxt->modulDXT4[i] = (float)sin(M_PI / len * (i + 0.125));
            }
          }
        }
        if (dxtErr == IIS_DXT_NO_ERROR) {
          pKernel->modul = hIisDxt->modulDXT4;
        }
        break;
      case IIS_INVALID:
      default:
        return IIS_DXT_ERROR_UNSUPPORTED;
    }
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    switch (kernel) {
      case IIS_DCT_II:
      case IIS_DST_II:
        if (hIisDxt->hIisFftForward == NULL) {
          err = IIS_RFFT_Create(&(hIisDxt->hIisFftForward), len, IIS_FFT_FWD);
          if (err != IIS_FFT_NO_ERROR) {
            dxtErr = IIS_DXT_ERROR_MEMORY;
          }
        }
        pKernel->hIisFft = hIisDxt->hIisFftForward;
        break;
      case IIS_DCT_III:
      case IIS_DST_III:
        if (hIisDxt->hIisFftBackward == NULL) {
          err = IIS_RFFT_Create(&(hIisDxt->hIisFftBackward), len, IIS_FFT_BWD);
          if (err != IIS_FFT_NO_ERROR) {
            dxtErr = IIS_DXT_ERROR_MEMORY;
          }
        }
        pKernel->hIisFft = hIisDxt->hIisFftBackward;
        break;
      case IIS_DCT_IV:
      case IIS_DST_IV:
        if (hIisDxt->hIisFft4 == NULL) {
          err = IIS_CFFT_Create(&(hIisDxt->hIisFft4), len / 2, IIS_FFT_FWD);
          if (err != IIS_FFT_NO_ERROR) {
            dxtErr = IIS_DXT_ERROR_MEMORY;
          }
        }
        pKernel->hIisFft = hIisDxt->hIisFft4;
        break;
      default:
        assert(0);
    }
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    switch (kernel) {
      case IIS_DCT_II:
        hIisDxt->pKernel[IIS_DST_II] = pKernel;
        break;
      case IIS_DST_II:
        hIisDxt->pKernel[IIS_DCT_II] = pKernel;
        break;
      case IIS_DCT_III:
        hIisDxt->pKernel[IIS_DST_III] = pKernel;
        break;
      case IIS_DST_III:
        hIisDxt->pKernel[IIS_DCT_III] = pKernel;
        break;
      case IIS_DCT_IV:
        hIisDxt->pKernel[IIS_DST_IV] = pKernel;
        break;
      case IIS_DST_IV:
        hIisDxt->pKernel[IIS_DCT_IV] = pKernel;
        break;
      default:
        assert(0);
    }
  }

  if (dxtErr != IIS_DXT_NO_ERROR) {
    IIS_DXT_Destroy(&hIisDxt);
  }

  return dxtErr;
}

IIS_DXT_ERROR IIS_DXT_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    IIS_KERNEL const kernel,
    float const *const pInBuffer,
    float *const pOutBuffer,
    int const len) {
  IIS_DXT_ERROR dxtErr = IIS_DXT_NO_ERROR;
  IIS_FFT_ERROR fftErr = IIS_FFT_NO_ERROR;
  IIS_DXT_KERNEL *pKernel = NULL;
  int N = 0;
  int N2 = 0;
  int i = 0;
  (void)len;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  dxtErr = validateKernel(kernel);
  assert(len == hIisDxt->len);

  if (hIisDxt->pKernel[kernel] == NULL) {
    return IIS_DXT_ERROR_WRONG_KERNEL;
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    pKernel = hIisDxt->pKernel[kernel];
    N = hIisDxt->len;
    N2 = N / 2;

    switch (kernel) {
      case IIS_DCT_II:
        for (i = 0; i < N2; i++) {
          hIisDxt->pInOutReIm[i] = pInBuffer[2 * i];
          hIisDxt->pInOutReIm[N - 1 - i] = pInBuffer[2 * i + 1];
        }
        break;
      case IIS_DST_II:
        for (i = 0; i < N2; i++) {
          hIisDxt->pInOutReIm[i] = pInBuffer[2 * i];
          hIisDxt->pInOutReIm[N - 1 - i] = -pInBuffer[2 * i + 1];
        }
        break;
      case IIS_DCT_III:
        for (i = 1; i < N2; i++) {
          float wr, wi, xr, xi;
          wr = pKernel->modul[2 * i];
          wi = pKernel->modul[2 * i + 1];
          xr = pInBuffer[i];
          xi = pInBuffer[N - i];
          hIisDxt->pInOutReIm[2 * i] = wr * xr + wi * xi;
          hIisDxt->pInOutReIm[2 * i + 1] = wi * xr - wr * xi;
        }
        hIisDxt->pInOutReIm[0] = pInBuffer[0];
        hIisDxt->pInOutReIm[1] = pInBuffer[N2] * 2.0f * (float)M_SQRT1_2;
        break;
      case IIS_DST_III:
        for (i = 1; i < N2; i++) {
          float wr, wi, xr, xi;

          wr = pKernel->modul[2 * i];
          wi = pKernel->modul[2 * i + 1];
          xr = pInBuffer[N - i - 1];
          xi = pInBuffer[i - 1];
          hIisDxt->pInOutReIm[2 * i] = wr * xr + wi * xi;
          hIisDxt->pInOutReIm[2 * i + 1] = wi * xr - wr * xi;
        }
        hIisDxt->pInOutReIm[0] = pInBuffer[N - 1];
        hIisDxt->pInOutReIm[1] = pInBuffer[N2 - 1] * 2.0f * (float)M_SQRT1_2;
        break;
      case IIS_DCT_IV:
        for (i = 0; i < N2 / 2; i++) {
          float re1, im1, re2, im2, sre, sim;

          re1 = pInBuffer[2 * i];
          im2 = pInBuffer[2 * i + 1];
          re2 = pInBuffer[2 * N2 - 2 - 2 * i];
          im1 = pInBuffer[2 * N2 - 1 - 2 * i];
          sre = pKernel->modul[N2 + i];
          sim = pKernel->modul[i];
          hIisDxt->pInOutReIm[2 * i] = re1 * sre + im1 * sim;
          hIisDxt->pInOutReIm[2 * i + 1] = im1 * sre - re1 * sim;
          sre = pKernel->modul[2 * N2 - 1 - i];
          sim = pKernel->modul[N2 - 1 - i];
          hIisDxt->pInOutReIm[2 * (N2 - 1 - i)] = re2 * sre + im2 * sim;
          hIisDxt->pInOutReIm[2 * (N2 - 1 - i) + 1] = im2 * sre - re2 * sim;
        }
        break;
      case IIS_DST_IV:
        for (i = 0; i < N2 / 2; i++) {
          float re1, im1, re2, im2, sre, sim;

          re1 = pInBuffer[2 * i];
          im2 = pInBuffer[2 * i + 1];
          re2 = pInBuffer[2 * N2 - 2 - 2 * i];
          im1 = pInBuffer[2 * N2 - 1 - 2 * i];
          sre = pKernel->modul[N2 + i];
          sim = pKernel->modul[i];
          hIisDxt->pInOutReIm[2 * i + 1] = re1 * sre - im1 * sim;
          hIisDxt->pInOutReIm[2 * i] = im1 * sre + re1 * sim;
          sre = pKernel->modul[2 * N2 - 1 - i];
          sim = pKernel->modul[N2 - 1 - i];
          hIisDxt->pInOutReIm[2 * (N2 - 1 - i) + 1] = re2 * sre - im2 * sim;
          hIisDxt->pInOutReIm[2 * (N2 - 1 - i)] = im2 * sre + re2 * sim;
        }
        break;
      default:
        break;
    }

    switch (kernel) {
      case IIS_DCT_II:
      case IIS_DST_II:
      case IIS_DCT_III:
      case IIS_DST_III:
        fftErr = IIS_FFT_Apply_RFFT(pKernel->hIisFft, hIisDxt->pInOutReIm, hIisDxt->pInOutReIm);
        if (fftErr != IIS_FFT_NO_ERROR) {
          dxtErr = IIS_DXT_ERROR_INTERNAL;
        }
        break;
      case IIS_DCT_IV:
      case IIS_DST_IV:
        fftErr = IIS_FFT_Apply_CFFT(pKernel->hIisFft,
                                    hIisDxt->pInOutReIm,
                                    hIisDxt->pInOutReIm + 1,
                                    hIisDxt->pInOutReIm,
                                    hIisDxt->pInOutReIm + 1);
        if (fftErr != IIS_FFT_NO_ERROR) {
          dxtErr = IIS_DXT_ERROR_INTERNAL;
        }
        break;
      default:
        break;
    }
  }

  if (dxtErr == IIS_DXT_NO_ERROR) {
    switch (kernel) {
      case IIS_DCT_II:
        for (i = 1; i < N2; i++) {
          float wr, wi, xr, xi;
          wr = pKernel->modul[2 * i];
          wi = pKernel->modul[2 * i + 1];
          xr = hIisDxt->pInOutReIm[2 * i];
          xi = hIisDxt->pInOutReIm[2 * i + 1];
          pOutBuffer[i] = wr * xr + wi * xi;
          pOutBuffer[N - i] = wi * xr - wr * xi;
        }
        pOutBuffer[0] = hIisDxt->pInOutReIm[0];
        pOutBuffer[N2] = hIisDxt->pInOutReIm[1] * (float)M_SQRT1_2;
        break;
      case IIS_DST_II:
        for (i = 1; i < N2; i++) {
          float wr, wi, xr, xi;
          wr = pKernel->modul[2 * i];
          wi = pKernel->modul[2 * i + 1];
          xr = hIisDxt->pInOutReIm[2 * i];
          xi = hIisDxt->pInOutReIm[2 * i + 1];
          pOutBuffer[N - i - 1] = wr * xr + wi * xi;
          pOutBuffer[i - 1] = wi * xr - wr * xi;
        }
        pOutBuffer[N - 1] = hIisDxt->pInOutReIm[0];
        pOutBuffer[N2 - 1] = hIisDxt->pInOutReIm[1] * (float)M_SQRT1_2;
        break;
      case IIS_DCT_III:
        for (i = 0; i < N2; i++) {
          pOutBuffer[2 * i] = hIisDxt->pInOutReIm[i] * N2;
          pOutBuffer[2 * i + 1] = hIisDxt->pInOutReIm[N - 1 - i] * N2;
        }
        break;
      case IIS_DST_III:
        for (i = 0; i < N2; i++) {
          pOutBuffer[2 * i] = hIisDxt->pInOutReIm[i] * N2;
          pOutBuffer[2 * i + 1] = -hIisDxt->pInOutReIm[N - 1 - i] * N2;
        }
        break;
      case IIS_DCT_IV:
        for (i = 0; i < N2; i++) {
          float re1, im1, sre, sim;
          re1 = hIisDxt->pInOutReIm[2 * i];
          im1 = hIisDxt->pInOutReIm[2 * i + 1];
          sre = pKernel->modul[N2 + i];
          sim = pKernel->modul[i];
          pOutBuffer[2 * i] = re1 * sre + im1 * sim;
          pOutBuffer[2 * N2 - 1 - 2 * i] = -im1 * sre + re1 * sim;
        }
        break;
      case IIS_DST_IV:
        for (i = 0; i < N2; i++) {
          float re1, im1, sre, sim;
          re1 = hIisDxt->pInOutReIm[2 * i];
          im1 = hIisDxt->pInOutReIm[2 * i + 1];
          sre = pKernel->modul[N2 + i];
          sim = pKernel->modul[i];
          pOutBuffer[2 * i] = re1 * sre + im1 * sim;
          pOutBuffer[2 * N2 - 1 - 2 * i] = im1 * sre - re1 * sim;
        }
      default:
        break;
    }
  }

  return dxtErr;
}

IIS_DXT_ERROR IIS_DXT_Destroy(
    HANDLE_IIS_DXT *const phIisDxt) {
  if (phIisDxt != NULL) {
    if ((*phIisDxt) != NULL) {
      HANDLE_IIS_DXT hIisDxt = *phIisDxt;
      int i;
      for (i = 0; i < IIS_MAX_KERNELS; i++) {
        IIS_DXT_KERNEL *pKernel = hIisDxt->pKernel[i];
        if (pKernel != NULL) {
          IIS_KERNEL kernel = pKernel->kernel;
          if (hIisDxt->pInOutReIm) {
            iisFree(hIisDxt->pInOutReIm);
            hIisDxt->pInOutReIm = NULL;
          }
          iisFree(pKernel);
          hIisDxt->pKernel[i] = NULL;
          switch (kernel) {
            case IIS_DCT_II:
              assert(!hIisDxt->pKernel[IIS_DST_II] || pKernel == hIisDxt->pKernel[IIS_DST_II]);
              assert(!hIisDxt->pKernel[IIS_DCT_II] || pKernel == hIisDxt->pKernel[IIS_DCT_II]);
              hIisDxt->pKernel[IIS_DST_II] = NULL;
              hIisDxt->pKernel[IIS_DCT_II] = NULL;
              break;
            case IIS_DST_II:
              assert(!hIisDxt->pKernel[IIS_DCT_II] || pKernel == hIisDxt->pKernel[IIS_DCT_II]);
              hIisDxt->pKernel[IIS_DCT_II] = NULL;
              assert(!hIisDxt->pKernel[IIS_DST_II] || pKernel == hIisDxt->pKernel[IIS_DST_II]);
              hIisDxt->pKernel[IIS_DST_II] = NULL;
              break;
            case IIS_DCT_III:
              assert(!hIisDxt->pKernel[IIS_DST_III] || pKernel == hIisDxt->pKernel[IIS_DST_III]);
              assert(!hIisDxt->pKernel[IIS_DCT_III] || pKernel == hIisDxt->pKernel[IIS_DCT_III]);
              hIisDxt->pKernel[IIS_DST_III] = NULL;
              hIisDxt->pKernel[IIS_DCT_III] = NULL;
              break;
            case IIS_DST_III:
              assert(!hIisDxt->pKernel[IIS_DCT_III] || pKernel == hIisDxt->pKernel[IIS_DCT_III]);
              assert(!hIisDxt->pKernel[IIS_DST_III] || pKernel == hIisDxt->pKernel[IIS_DST_III]);
              hIisDxt->pKernel[IIS_DCT_III] = NULL;
              hIisDxt->pKernel[IIS_DST_III] = NULL;
              break;
            case IIS_DCT_IV:
              assert(!hIisDxt->pKernel[IIS_DST_IV] || pKernel == hIisDxt->pKernel[IIS_DST_IV]);
              assert(!hIisDxt->pKernel[IIS_DCT_IV] || pKernel == hIisDxt->pKernel[IIS_DCT_IV]);
              hIisDxt->pKernel[IIS_DST_IV] = NULL;
              hIisDxt->pKernel[IIS_DCT_IV] = NULL;
              break;
            case IIS_DST_IV:
              assert(!hIisDxt->pKernel[IIS_DCT_IV] || pKernel == hIisDxt->pKernel[IIS_DCT_IV]);
              assert(!hIisDxt->pKernel[IIS_DST_IV] || pKernel == hIisDxt->pKernel[IIS_DST_IV]);
              hIisDxt->pKernel[IIS_DCT_IV] = NULL;
              hIisDxt->pKernel[IIS_DST_IV] = NULL;
              break;
            default:
              assert(0);
              break;
          }
        }
        if (hIisDxt->hIisFftForward) IIS_xFFT_Destroy(&(hIisDxt->hIisFftForward));
        if (hIisDxt->hIisFftBackward) IIS_xFFT_Destroy(&(hIisDxt->hIisFftBackward));
        if (hIisDxt->hIisFft4) IIS_xFFT_Destroy(&(hIisDxt->hIisFft4));
        if (hIisDxt->modulDXT23) {
          iisFree(hIisDxt->modulDXT23);
          hIisDxt->modulDXT23 = NULL;
        }
        if (hIisDxt->modulDXT4) {
          iisFree(hIisDxt->modulDXT4);
          hIisDxt->modulDXT4 = NULL;
        }
      }
      iisFree(*phIisDxt);
      *phIisDxt = NULL;
    }
  }

  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DCT2_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = 0;
    for (n = 0; n < N; n++) {
      acc += pInBuffer[n] * cos(M_PI / N * (n + 1.0 / 2.0) * k);
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DST2_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = 0;
    for (n = 0; n < N; n++) {
      acc += pInBuffer[n] * sin(M_PI / N * (n + 1.0 / 2.0) * (k + 1));
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }

  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DCT3_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = 1.0 / 2.0 * pInBuffer[0];
    for (n = 1; n < N; n++) {
      acc += pInBuffer[n] * cos(M_PI / N * n * (k + 1.0 / 2.0));
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DST3_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = (pow(-1.0, k) / 2.0) * pInBuffer[N - 1];
    for (n = 0; n < N - 1; n++) {
      acc += pInBuffer[n] * sin(M_PI / N * (n + 1.0) * (k + 1.0 / 2.0));
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DCT4_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = 0;
    for (n = 0; n < N; n++) {
      acc += pInBuffer[n] * cos(M_PI / N * (n + 1.0 / 2.0) * (k + 1.0 / 2.0));
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DXT_ERROR IIS_DST4_REFERENCE_IMPLEMENTATION_Apply(
    HANDLE_IIS_DXT const hIisDxt,
    float const *const pInBuffer,
    float *const pOutBuffer) {
  int N;
  int k, n;

  if (hIisDxt == NULL) {
    return IIS_DXT_ERROR_INVALID_HANDLE;
  }

  N = hIisDxt->len;

  for (k = 0; k < N; k++) {
    double acc = 0;
    for (n = 0; n < N; n++) {
      acc += pInBuffer[n] * sin(M_PI / N * (n + 1.0 / 2.0) * (k + 1.0 / 2.0));
    }
    hIisDxt->pInOutReIm[k] = (float)acc;
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = hIisDxt->pInOutReIm[k];
  }
  return IIS_DXT_NO_ERROR;
}

IIS_DCT_ERROR IIS_DCT_Create(HANDLE_IIS_DCT *const phIisDct,
                             int const len) {
  HANDLE_IIS_DCT hIisDct;

  IIS_DCT_ERROR err;
  if (IIS_DXT_NO_ERROR != (err = IIS_DXT_Create(&hIisDct, IIS_DCT_IV, len))) {
    return err;
  }

  *phIisDct = hIisDct;

  return IIS_DCT_NO_ERROR;
}

IIS_DCT_ERROR IIS_DCT_Apply(HANDLE_IIS_DCT const hIisDct,
                            float const *const pInBuffer,
                            float *const pOutBuffer) {
  IIS_DCT_ERROR err;
  if (IIS_DXT_NO_ERROR != (err = IIS_DXT_Apply(hIisDct, IIS_DCT_IV, pInBuffer, pOutBuffer, hIisDct->len))) {
    return err;
  }
  return IIS_DCT_NO_ERROR;
}

IIS_DCT_ERROR IIS_DST_Apply(HANDLE_IIS_DCT const hIisDct,
                            float const *const pInBuffer,
                            float *const pOutBuffer) {
  IIS_DCT_ERROR err;
  if (IIS_DXT_NO_ERROR != (err = IIS_DXT_Apply(hIisDct, IIS_DST_IV, pInBuffer, pOutBuffer, hIisDct->len))) {
    return err;
  }
  return IIS_DCT_NO_ERROR;
}
