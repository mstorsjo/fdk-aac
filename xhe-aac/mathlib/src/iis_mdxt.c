
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
#include <stdlib.h>

#include "mathlib.h"
#include "iisutillib.h"

#include "iis_mdxt.h"
#include "iis_dxt.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

#define MAX_N 512

typedef struct T_IIS_MDXT {
  int len;
  HANDLE_IIS_DXT phIisDxt;
} IIS_MDXT;

static int isError(IIS_MDXT_ERROR errorValue) {
  int retValue = 0;
  if (errorValue != IIS_MDXT_NO_ERROR) {
    retValue = 1;
  }
  return retValue;
}

static int checkOverlap(float const* const vec1,
                        float const* const vec2,
                        int const len1,
                        int const len2,
                        int const inplace) {
  if (vec1 == NULL) return 1;
  if (vec2 == NULL) return 1;

  if (inplace && vec1 == vec2) {
    return 0;
  }
  if (vec1 >= vec2 + len2 ||
      vec2 >= vec1 + len1) {
    return 0;
  }
  return 1;
}

static IIS_MDXT_ERROR checkOverlap_flex(float const* const pTimeBuffer,
                                        float const* const pAdditionalBuffer,
                                        float const* const pWinPerm,
                                        float const* const pWinOpt,
                                        float const* const pSpecBuffer,
                                        int const N,
                                        WINANDFEEDMODE const winAndFeedMode) {
  int fac = 1;
  WINANDFEEDMODE winAndFeedModeLocal;

  if (pAdditionalBuffer == NULL && pWinPerm == NULL && pWinOpt == NULL)
    winAndFeedModeLocal = PLAIN;
  else if (pAdditionalBuffer == NULL && pWinPerm != NULL && pWinOpt != NULL)
    winAndFeedModeLocal = TWOSIDEDWIN;
  else if (pAdditionalBuffer == NULL && pWinPerm != NULL && pWinOpt == NULL)
    winAndFeedModeLocal = TRANSITWIN;
  else if (pAdditionalBuffer != NULL && pWinPerm != NULL && pWinOpt != NULL)
    winAndFeedModeLocal = TWOSIDEDWINANDFEED;
  else if (pAdditionalBuffer != NULL && pWinPerm != NULL && pWinOpt == NULL)
    winAndFeedModeLocal = ONESIDEDWINANDFEED;
  else
    return IIS_MDXT_INVALID_CONFIG;

  if (!((winAndFeedMode == TRANSONESIDEDWINANDFEED) && (winAndFeedModeLocal == ONESIDEDWINANDFEED))) {
    if (winAndFeedMode != winAndFeedModeLocal) return IIS_MDXT_INVALID_CONFIG;
  }

  if (winAndFeedMode == PLAIN || winAndFeedMode == TWOSIDEDWIN || winAndFeedMode == TRANSITWIN) fac = 2;

  if (checkOverlap(pTimeBuffer, pSpecBuffer, fac * N, N, 1)) {
    return IIS_MDXT_INVALID_BUFFER;
  }

  if (winAndFeedMode == TWOSIDEDWIN || winAndFeedMode == TRANSITWIN || winAndFeedMode == TWOSIDEDWINANDFEED || winAndFeedMode == ONESIDEDWINANDFEED || winAndFeedMode == TRANSONESIDEDWINANDFEED) {
    if (checkOverlap(pTimeBuffer, pWinPerm, fac * N, N, 0) ||
        checkOverlap(pWinPerm, pSpecBuffer, N, N, 0)) {
      return IIS_MDXT_INVALID_BUFFER;
    }
  }

  if (winAndFeedMode == TWOSIDEDWIN || winAndFeedMode == TWOSIDEDWINANDFEED) {
    if (checkOverlap(pWinOpt, pTimeBuffer, N, fac * N, 0) ||
        checkOverlap(pWinOpt, pWinPerm, N, N, 1) ||
        checkOverlap(pWinOpt, pSpecBuffer, N, N, 0)) {
      return IIS_MDXT_INVALID_BUFFER;
    }
  }

  if (winAndFeedMode == ONESIDEDWINANDFEED || winAndFeedMode == TRANSONESIDEDWINANDFEED || winAndFeedMode == TWOSIDEDWINANDFEED) {
    if (checkOverlap(pTimeBuffer, pAdditionalBuffer, fac * N, N, 0) ||
        checkOverlap(pAdditionalBuffer, pWinPerm, N, N, 0) ||
        checkOverlap(pAdditionalBuffer, pSpecBuffer, N, N, 0)) {
      return IIS_MDXT_INVALID_BUFFER;
    }
  }

  if (winAndFeedMode == TWOSIDEDWINANDFEED) {
    if (checkOverlap(pWinOpt, pAdditionalBuffer, N, N, 0)) {
      return IIS_MDXT_INVALID_BUFFER;
    }
  }

  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_MDXT_Create(HANDLE_IIS_MDXT* const phIisMdxt,
                               IIS_MDXT_CORE const core,
                               int const len) {
  IIS_MDXT_ERROR retValue = IIS_MDXT_NO_ERROR;
  HANDLE_IIS_MDXT hIisMdxt = NULL;
  IIS_KERNEL dxtCore = IIS_INVALID;

  if (*phIisMdxt != NULL) {
    retValue = IIS_MDXT_INVALID_HANDLE;
  } else if (len % 2) {
    retValue = IIS_MDXT_LENGTH_ERROR;
  }

  if (!isError(retValue)) {
    hIisMdxt = (HANDLE_IIS_MDXT)iisCalloc(1, sizeof(IIS_MDXT));
    if (NULL == hIisMdxt) {
      retValue = IIS_MDXT_INTERNAL_ERROR;
    }
  }

  if (!isError(retValue)) {
    hIisMdxt->len = len;
    hIisMdxt->phIisDxt = NULL;

    switch (core) {
      case IIS_MDXT_MDCT_IV:
      case IIS_MDXT_MDST_IV:
      case IIS_MDXT_IMDCT_IV:
      case IIS_MDXT_IMDST_IV:
        dxtCore = IIS_DCT_IV;
        break;

      case IIS_MDXT_MDCT_II:
      case IIS_MDXT_MDST_II:
        dxtCore = IIS_DCT_II;
        break;

      case IIS_MDXT_IMDCT_II:
      case IIS_MDXT_IMDST_II:
        dxtCore = IIS_DCT_III;
        break;

      default:
        iisFree(hIisMdxt);
        hIisMdxt = NULL;
        retValue = IIS_MDXT_INVALID_TYPE;
        break;
    }
  }

  if (!isError(retValue)) {
    IIS_DXT_ERROR dxtRetValue = IIS_DXT_NO_ERROR;
    dxtRetValue = IIS_DXT_Create(&(hIisMdxt->phIisDxt), dxtCore, len);

    if (IIS_DXT_NO_ERROR != dxtRetValue) {
      iisFree(hIisMdxt);
      hIisMdxt = NULL;
      retValue = IIS_MDXT_INTERNAL_ERROR;
    } else {
      *phIisMdxt = hIisMdxt;
    }
  }

  return retValue;
}

IIS_MDXT_ERROR IIS_MDXT_AddKernel(HANDLE_IIS_MDXT const hIisMdxt,
                                  IIS_MDXT_CORE const core,
                                  int const len) {
  IIS_MDXT_ERROR err = IIS_MDXT_NO_ERROR;

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  switch (core) {
    case IIS_MDXT_MDCT_IV:
    case IIS_MDXT_MDST_IV:
    case IIS_MDXT_IMDCT_IV:
    case IIS_MDXT_IMDST_IV:
      if (IIS_DXT_NO_ERROR != (IIS_DXT_AddAdditionalKernel(hIisMdxt->phIisDxt, IIS_DCT_IV, len))) {
        err = IIS_MDXT_INTERNAL_ERROR;
      }
      break;
    case IIS_MDXT_MDCT_II:
    case IIS_MDXT_MDST_II:
      if (IIS_DXT_NO_ERROR != (IIS_DXT_AddAdditionalKernel(hIisMdxt->phIisDxt, IIS_DCT_II, len))) {
        err = IIS_MDXT_INTERNAL_ERROR;
      }
      break;
    case IIS_MDXT_IMDCT_II:
    case IIS_MDXT_IMDST_II:
      if (IIS_DXT_NO_ERROR != (IIS_DXT_AddAdditionalKernel(hIisMdxt->phIisDxt, IIS_DCT_III, len))) {
        err = IIS_MDXT_INTERNAL_ERROR;
      }
      break;
    default:
      err = IIS_MDXT_INVALID_TYPE;
      break;
  }
  return err;
}

IIS_MDXT_ERROR IIS_MDXT_Destroy(HANDLE_IIS_MDXT* const phIisMdxt) {
  if (phIisMdxt != NULL) {
    if ((*phIisMdxt) != NULL) {
      HANDLE_IIS_MDXT hIisMdxt = *phIisMdxt;
      if (hIisMdxt->phIisDxt != NULL) {
        IIS_DXT_Destroy(&(hIisMdxt->phIisDxt));
      }
      iisFree(*phIisMdxt);
      *phIisMdxt = NULL;
    }
  }
  return IIS_MDXT_NO_ERROR;
}

static const int vz[IIS_MDXT_NUM_CORES][4] = {{+1, -1, -1, -1},
                                              {+1, +1, +1, -1},
                                              {+1, -1, -1, -1},
                                              {+1, +1, +1, -1},
                                              {+1, +1, +1, +1},
                                              {+1, -1, -1, +1},
                                              {+1, +1, +1, +1},
                                              {+1, -1, -1, +1}};

IIS_MDXT_ERROR IIS_MDXT_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                              IIS_MDXT_CORE const type,
                              float const* const pInBuffer,
                              float* const pOutBuffer,
                              int const len) {
  return IIS_MDXT_flex_Apply(hIisMdxt, type, pInBuffer, NULL, NULL, NULL, pOutBuffer, len, PLAIN);
}

IIS_MDXT_ERROR IIS_IMDXT_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                               IIS_MDXT_CORE const type,
                               float const* const pInBuffer,
                               float* const pOutBuffer,
                               int const len) {
  return IIS_IMDXT_flex_Apply(hIisMdxt, type, pInBuffer, NULL, NULL, NULL, pOutBuffer, len, PLAIN);
}

IIS_MDXT_ERROR IIS_MDXT_flex_Apply(
    HANDLE_IIS_MDXT const hIisMdxt,
    IIS_MDXT_CORE const type,
    float const* const pCurrBuffer,
    float* const pPrevBuffer,
    float const* const pWinCurr,
    float const* const pWinPrev,
    float* const pOutBuffer,
    int const len,
    WINANDFEEDMODE const winAndFeedMode) {
  int N;
  int i;
  int inplace = 0;
  float tmp = 0;
  IIS_DXT_ERROR err_dxt = IIS_DXT_NO_ERROR;
  IIS_MDXT_ERROR err_mdxt = IIS_MDXT_NO_ERROR;

  if (hIisMdxt == NULL) return IIS_MDXT_INVALID_HANDLE;

  if (winAndFeedMode == TRANSONESIDEDWINANDFEED) return IIS_MDXT_UNSUPPORTED;

  if (len != hIisMdxt->len) return IIS_MDXT_LENGTH_ERROR;

  if (((unsigned int)type) >= (sizeof(vz) / sizeof(vz[0]))) return IIS_MDXT_INVALID_TYPE;

  N = hIisMdxt->len;

  if (pCurrBuffer == pOutBuffer) {
    inplace = 1;
  }

  err_mdxt = checkOverlap_flex(pCurrBuffer, pPrevBuffer, pWinCurr, pWinPrev, pOutBuffer, N, winAndFeedMode);
  if (err_mdxt != IIS_MDXT_NO_ERROR) {
    return err_mdxt;
  }

  switch (winAndFeedMode) {
    case PLAIN:
      if (!inplace) {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[N / 2 + i] = vz[type][0] * pCurrBuffer[i] + vz[type][1] * pCurrBuffer[N - 1 - i];
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] + vz[type][3] * pCurrBuffer[N + N / 2 + i];
        }
      } else {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][0] * pCurrBuffer[i] + vz[type][1] * pCurrBuffer[N - 1 - i];
        }
        copyFLOAT(&pOutBuffer[0], &pOutBuffer[N / 2], N / 2);
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] + vz[type][3] * pCurrBuffer[N + N / 2 + i];
        }
      }
      break;
    case TWOSIDEDWIN:
      if (!inplace) {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[N / 2 + i] = vz[type][0] * pCurrBuffer[i] * pWinPrev[i] + vz[type][1] * pCurrBuffer[N - 1 - i] * pWinPrev[N - 1 - i];
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N + N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
      } else {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][0] * pCurrBuffer[i] * pWinPrev[i] + vz[type][1] * pCurrBuffer[N - 1 - i] * pWinPrev[N - 1 - i];
        }
        copyFLOAT(&pOutBuffer[0], &pOutBuffer[N / 2], N / 2);
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N + N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
      }
      break;
    case TRANSITWIN:
      if (!inplace) {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[N / 2 + i] = vz[type][0] * pCurrBuffer[i] + vz[type][1] * pCurrBuffer[N - 1 - i];
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N + N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
      } else {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][0] * pCurrBuffer[i] + vz[type][1] * pCurrBuffer[N - 1 - i];
        }
        copyFLOAT(&pOutBuffer[0], &pOutBuffer[N / 2], N / 2);
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N + N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N + N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
      }
      break;
    case ONESIDEDWINANDFEED:
      if (!inplace) {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[N / 2 + i] = vz[type][0] * pPrevBuffer[i] + vz[type][1] * pPrevBuffer[N - 1 - i];
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
        multFLOAT(pCurrBuffer, pWinCurr, pPrevBuffer, N);
      } else {
        for (i = 0; i < N / 2; i++) {
          pPrevBuffer[i] = vz[type][0] * pPrevBuffer[i] + vz[type][1] * pPrevBuffer[N - 1 - i];
        }
        for (i = 0; i < N / 2; i++) {
          pPrevBuffer[N / 2 + i] = pCurrBuffer[N / 2 + i] * pWinCurr[N / 2 + i];
          pOutBuffer[N / 2 + i] = vz[type][2] * pCurrBuffer[N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
        for (i = 0; i < N / 2; i++) {
          tmp = pCurrBuffer[i] * pWinCurr[i];
          pOutBuffer[i] = pOutBuffer[N / 2 + i];
          pOutBuffer[N / 2 + i] = pPrevBuffer[i];
          pPrevBuffer[i] = tmp;
        }
      }
      break;
    case TWOSIDEDWINANDFEED:
      if (!inplace) {
        for (i = 0; i < N / 2; i++) {
          pOutBuffer[N / 2 + i] = vz[type][0] * pPrevBuffer[i] * pWinPrev[i] + vz[type][1] * pPrevBuffer[N - 1 - i] * pWinPrev[N - 1 - i];
          pOutBuffer[i] = vz[type][2] * pCurrBuffer[N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
        copyFLOAT(pCurrBuffer, pPrevBuffer, N);
      } else {
        for (i = 0; i < N / 2; i++) {
          pPrevBuffer[i] = vz[type][0] * pPrevBuffer[i] * pWinPrev[i] + vz[type][1] * pPrevBuffer[N - 1 - i] * pWinPrev[N - 1 - i];
        }
        for (i = 0; i < N / 2; i++) {
          pPrevBuffer[N / 2 + i] = pCurrBuffer[N / 2 + i];
          pOutBuffer[N / 2 + i] = vz[type][2] * pCurrBuffer[N / 2 - 1 - i] * pWinCurr[N / 2 + i] + vz[type][3] * pCurrBuffer[N / 2 + i] * pWinCurr[N / 2 - 1 - i];
        }
        for (i = 0; i < N / 2; i++) {
          tmp = pCurrBuffer[i];
          pOutBuffer[i] = pOutBuffer[N / 2 + i];
          pOutBuffer[N / 2 + i] = pPrevBuffer[i];
          pPrevBuffer[i] = tmp;
        }
      }
      break;
    default:
      return IIS_MDXT_UNSUPPORTED;
  }

  switch (type) {
    case IIS_MDXT_MDCT_IV:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_MDCT_II:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_II, pOutBuffer, pOutBuffer, N);
      pOutBuffer[0] /= 2.0f;
      break;
    case IIS_MDXT_MDST_IV:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_MDST_II:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_II, pOutBuffer, pOutBuffer, N);
      pOutBuffer[N - 1] /= 2.0f;
      break;
    default:
      return IIS_MDXT_INVALID_TYPE;
  }

  if (err_dxt != IIS_DXT_NO_ERROR) return IIS_MDXT_INTERNAL_ERROR;

  smultFLOATip(2.0, pOutBuffer, N);

  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_IMDXT_flex_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                    IIS_MDXT_CORE const type,
                                    float const* const pCurrBuffer,
                                    float* const pOlaBuffer,
                                    float const* const pWinCurr,
                                    float const* const pWinPrev,
                                    float* const pOutBuffer,
                                    int const len,
                                    WINANDFEEDMODE const winAndFeedMode) {
  int N;
  int i;
  float tmp;
  IIS_DXT_ERROR err_dxt = IIS_DXT_NO_ERROR;
  IIS_MDXT_ERROR err_mdxt = IIS_MDXT_NO_ERROR;

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  if (((unsigned int)type) >= (sizeof(vz) / sizeof(vz[0]))) return IIS_MDXT_INVALID_TYPE;

  N = hIisMdxt->len;

  err_mdxt = checkOverlap_flex(pOutBuffer, pOlaBuffer, pWinPrev, pWinCurr, pCurrBuffer, N, winAndFeedMode);
  if (err_mdxt != IIS_MDXT_NO_ERROR) {
    return err_mdxt;
  }

  for (i = 0; i < N; i++) {
    pOutBuffer[i] = pCurrBuffer[i] / N;
  }

  switch (type) {
    case IIS_MDXT_IMDCT_IV:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_IMDCT_II:
      pOutBuffer[0] *= 2;
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_III, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_IMDST_IV:
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_IMDST_II:
      pOutBuffer[N - 1] *= 2;
      err_dxt = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_III, pOutBuffer, pOutBuffer, N);
      break;
    default:
      return IIS_MDXT_INVALID_TYPE;
  }

  if (err_dxt != IIS_DXT_NO_ERROR) {
    return IIS_MDXT_INTERNAL_ERROR;
  }

  for (i = 0; i < N / 2; i++) {
    tmp = pOutBuffer[i];
    pOutBuffer[i] = vz[type][0] * pOutBuffer[N / 2 + i];
    pOutBuffer[N / 2 + i] = vz[type][3] * tmp;
  }
  switch (winAndFeedMode) {
    case PLAIN:
      for (i = 0; i < N / 2; i++) {
        pOutBuffer[2 * N - 1 - i] = pOutBuffer[N - 1 - i];
        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i];
        pOutBuffer[N + i] = vz[type][2] * vz[type][3] * pOutBuffer[2 * N - 1 - i];
      }
      break;
    case TWOSIDEDWIN:
      for (i = 0; i < N / 2; i++) {
        pOutBuffer[2 * N - 1 - i] = pOutBuffer[N - 1 - i];
        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i] * pWinPrev[N - 1 - i];
        pOutBuffer[N + i] = vz[type][2] * vz[type][3] * pOutBuffer[2 * N - 1 - i] * pWinCurr[N - 1 - i];
        pOutBuffer[i] *= pWinPrev[i];
        pOutBuffer[2 * N - 1 - i] *= pWinCurr[i];
      }
      break;
    case TRANSITWIN:
      for (i = 0; i < N / 2; i++) {
        pOutBuffer[2 * N - 1 - i] = pOutBuffer[N - 1 - i];
        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i] * pWinPrev[N - 1 - i];
        pOutBuffer[N + i] = vz[type][2] * vz[type][3] * pOutBuffer[2 * N - 1 - i];
        pOutBuffer[i] *= pWinPrev[i];
      }
      break;
    case ONESIDEDWINANDFEED:
      for (i = 0; i < N / 2; i++) {
        tmp = pOlaBuffer[N - 1 - i] * pWinPrev[i];
        pOlaBuffer[N - 1 - i] = pOutBuffer[N - 1 - i];

        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i];

        pOutBuffer[i] = pOutBuffer[i] * pWinPrev[i] + pOlaBuffer[i] * pWinPrev[N - 1 - i];
        pOutBuffer[N - 1 - i] = pOutBuffer[N - 1 - i] * pWinPrev[N - 1 - i] + tmp;

        pOlaBuffer[i] = vz[type][2] * vz[type][3] * pOlaBuffer[N - 1 - i];
      }
      break;
    case TWOSIDEDWINANDFEED:
      for (i = 0; i < N / 2; i++) {
        tmp = pOlaBuffer[N - 1 - i];
        pOlaBuffer[N - 1 - i] = pOutBuffer[N - 1 - i];

        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i];

        pOutBuffer[i] = pOutBuffer[i] * pWinPrev[i] + pOlaBuffer[i];
        pOutBuffer[N - 1 - i] = pOutBuffer[N - 1 - i] * pWinPrev[N - 1 - i] + tmp;

        pOlaBuffer[i] = vz[type][2] * vz[type][3] * pOlaBuffer[N - 1 - i] * pWinCurr[N - 1 - i];
        pOlaBuffer[N - 1 - i] = pOlaBuffer[N - 1 - i] * pWinCurr[i];
      }
      break;
    case TRANSONESIDEDWINANDFEED:
      for (i = 0; i < N / 2; i++) {
        pOlaBuffer[N - 1 - i] = pOutBuffer[N - 1 - i];

        pOutBuffer[N - 1 - i] = vz[type][1] * vz[type][0] * pOutBuffer[i];

        pOutBuffer[i] = pOlaBuffer[i];
        pOutBuffer[N - 1 - i] = pOutBuffer[N - 1 - i] * pWinPrev[N - 1 - i];

        pOlaBuffer[i] = vz[type][2] * vz[type][3] * pOlaBuffer[N - 1 - i];
      }
      break;
    default:
      return IIS_MDXT_UNSUPPORTED;
  }

  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_MDXT_REFERENCE_IMPLEMENTATION_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                       IIS_MDXT_CORE const type,
                                                       float const* const pInBuffer,
                                                       float* const pOutBuffer,
                                                       int const len) {
  return IIS_MDXT_REFERENCE_IMPLEMENTATION_flex_Apply(hIisMdxt, type, pInBuffer, NULL, NULL, NULL, pOutBuffer, len, PLAIN);
}

IIS_MDXT_ERROR IIS_MDXT_REFERENCE_IMPLEMENTATION_flex_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                            IIS_MDXT_CORE const type,
                                                            float const* const pCurrBuffer,
                                                            float* const pPrevBuffer,
                                                            float const* const pWinCurr,
                                                            float const* const pWinPrev,
                                                            float* const pOutBuffer,
                                                            int const len,
                                                            WINANDFEEDMODE const winAndFeedMode) {
  int N;
  int n, k;
  float outputBuffer[16384];
  float inputBuffer[16384];
  float* pOut = pOutBuffer;
  float const* pIn;
  IIS_MDXT_ERROR err = IIS_MDXT_NO_ERROR;

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (winAndFeedMode == TRANSONESIDEDWINANDFEED) {
    return IIS_MDXT_UNSUPPORTED;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;

  err = checkOverlap_flex(pCurrBuffer, pPrevBuffer, pWinCurr, pWinPrev, pOutBuffer, N, winAndFeedMode);
  if (err != IIS_MDXT_NO_ERROR) {
    return err;
  }

  if (pCurrBuffer == pOutBuffer) pOut = outputBuffer;

  switch (winAndFeedMode) {
    case PLAIN:
      pIn = pCurrBuffer;
      break;
    case TWOSIDEDWIN:
      multFLOAT(pWinPrev, pCurrBuffer, inputBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, &pCurrBuffer[N], 1, &inputBuffer[N], 1, N);
      pIn = inputBuffer;
      break;
    case TRANSITWIN:
      copyFLOAT(pCurrBuffer, inputBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, &pCurrBuffer[N], 1, &inputBuffer[N], 1, N);
      pIn = inputBuffer;
      break;
    case TWOSIDEDWINANDFEED:
      multFLOAT(pWinPrev, pPrevBuffer, inputBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, pCurrBuffer, 1, &inputBuffer[N], 1, N);
      copyFLOAT(pCurrBuffer, pPrevBuffer, N);
      pIn = inputBuffer;
      break;
    case ONESIDEDWINANDFEED:
      copyFLOAT(pPrevBuffer, inputBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, pCurrBuffer, 1, &inputBuffer[N], 1, N);
      multFLOAT(pWinCurr, pCurrBuffer, pPrevBuffer, N);
      pIn = inputBuffer;
      break;
    default:
      return IIS_MDXT_UNSUPPORTED;
  }

  for (k = 0; k < N; k++) {
    pOut[k] = 0;
    for (n = 0; n < 2 * N; n++) {
      switch (type) {
        case IIS_MDXT_MDCT_IV:
          pOut[k] += pIn[n] * (float)cos(M_PI / N * (n + .5 + N / 2) * (k + .5));
          break;
        case IIS_MDXT_MDST_IV:
          pOut[k] += pIn[n] * (float)sin(M_PI / N * (n + .5 + N / 2) * (k + .5));
          break;
        case IIS_MDXT_MDCT_II:
          pOut[k] += pIn[n] * (float)cos(M_PI / N * (n + .5 + N / 2) * (k + 0.0));
          break;
        case IIS_MDXT_MDST_II:
          pOut[k] += pIn[n] * (float)sin(M_PI / N * (n + .5 + N / 2) * (k + 1.0));
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }
    }
    switch (type) {
      case IIS_MDXT_MDCT_II:
        if (k == 0) pOut[k] /= 2;
        break;
      case IIS_MDXT_MDST_II:
        if (k == N - 1) pOut[k] /= 2;
        break;
      default:
        break;
    }
  }
  for (k = 0; k < N; k++) {
    pOutBuffer[k] = pOut[k] * 2.0f;
  }
  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_IMDXT_REFERENCE_IMPLEMENTATION_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                        IIS_MDXT_CORE const type,
                                                        float const* const pInBuffer,
                                                        float* const pOutBuffer,
                                                        int const len) {
  return IIS_IMDXT_REFERENCE_IMPLEMENTATION_flex_Apply(hIisMdxt, type, pInBuffer, NULL, NULL, NULL, pOutBuffer, len, PLAIN);
}

IIS_MDXT_ERROR IIS_IMDXT_REFERENCE_IMPLEMENTATION_flex_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                             IIS_MDXT_CORE const type,
                                                             float const* const pCurrBuffer,
                                                             float* const pOlaBuffer,
                                                             float const* const pWinCurr,
                                                             float const* const pWinPrev,
                                                             float* const pOutBuffer,
                                                             int const len,
                                                             WINANDFEEDMODE const winAndFeedMode) {
  int N;
  int n, k;
  float outputBuffer[16384];
  float* pOut = pOutBuffer;
  IIS_MDXT_ERROR err = IIS_MDXT_NO_ERROR;

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;

  err = checkOverlap_flex(pOutBuffer, pOlaBuffer, pWinPrev, pWinCurr, pCurrBuffer, N, winAndFeedMode);
  if (err != IIS_MDXT_NO_ERROR) {
    return err;
  }

  if (pCurrBuffer == pOutBuffer || winAndFeedMode == TWOSIDEDWINANDFEED || winAndFeedMode == ONESIDEDWINANDFEED || winAndFeedMode == TRANSONESIDEDWINANDFEED) pOut = outputBuffer;

  for (n = 0; n < 2 * N; n++) {
    pOut[n] = 0;
    for (k = 0; k < N; k++) {
      switch (type) {
        case IIS_MDXT_IMDCT_IV:
          pOut[n] += pCurrBuffer[k] * (float)cos(M_PI / N * (n + .5 + N / 2) * (k + .5));
          break;
        case IIS_MDXT_IMDST_IV:
          pOut[n] += pCurrBuffer[k] * (float)sin(M_PI / N * (n + .5 + N / 2) * (k + .5));
          break;
        case IIS_MDXT_IMDCT_II:
          pOut[n] += pCurrBuffer[k] * (float)cos(M_PI / N * (n + .5 + N / 2) * (k + 0.0));
          break;
        case IIS_MDXT_IMDST_II:
          pOut[n] += pCurrBuffer[k] * (float)sin(M_PI / N * (n + .5 + N / 2) * (k + 1.0));
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }
    }
  }
  for (n = 0; n < 2 * N; n++) {
    pOut[n] = pOut[n] / N;
  }

  switch (winAndFeedMode) {
    case PLAIN:
      copyFLOAT(pOut, pOutBuffer, 2 * N);
      break;
    case TWOSIDEDWIN:
      multFLOAT(pWinPrev, pOut, pOutBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, &pOut[N], 1, &pOutBuffer[N], 1, N);
      break;
    case TRANSITWIN:
      multFLOAT(pWinPrev, pOut, pOutBuffer, N);
      copyFLOAT(&pOut[N], &pOutBuffer[N], N);
      break;
    case TWOSIDEDWINANDFEED:
      multFLOAT(pWinPrev, pOut, pOutBuffer, N);
      addFLOAT(pOlaBuffer, pOutBuffer, pOutBuffer, N);
      multFLOATflex(&pWinCurr[N - 1], -1, &pOut[N], 1, pOlaBuffer, 1, N);
      break;
    case ONESIDEDWINANDFEED:
      multFLOAT(pWinPrev, pOut, pOutBuffer, N);
      multFLOATflex(&pWinPrev[N - 1], -1, pOlaBuffer, 1, pOlaBuffer, 1, N);
      addFLOAT(pOlaBuffer, pOutBuffer, pOutBuffer, N);
      copyFLOAT(&pOut[N], pOlaBuffer, N);
      break;
    case TRANSONESIDEDWINANDFEED:
      multFLOAT(&pWinPrev[N / 2], &pOut[N / 2], &pOutBuffer[N / 2], N / 2);
      copyFLOAT(pOlaBuffer, pOutBuffer, N / 2);
      copyFLOAT(&pOut[N], pOlaBuffer, N);
      break;
    default:
      return IIS_MDXT_UNSUPPORTED;
  }

  return IIS_MDXT_NO_ERROR;
}

void multE2_DinvF(float* const x,
                  float const* const fb,
                  float* const z,
                  int N) {
  int i;
  float z0, z2;

  for (i = 0; i < N / 4; i++) {
    z2 = x[N / 2 + i];
    z0 = z2 + (z[N / 2 + i] * fb[2 * N + i]);

    z[N / 2 + i] = x[N / 2 - 1 - i] + (z[N + i] * fb[2 * N + N / 2 + i]);

    x[N / 2 - 1 - i] = (z[N / 2 + i] * fb[N + N / 2 - 1 - i]) + (z[i] * fb[N + N / 2 + i]);

    z[i] = z0;

    z[N + i] = z2;
  }

  for (i = N / 4; i < N / 2; i++) {
    z2 = x[N / 2 + i];
    z0 = z2 + (z[N / 2 + i] * fb[2 * N + i]);

    z[N / 2 + i] = x[N / 2 - 1 - i] + (z[N + i] * fb[2 * N + N / 2 + i]);

    x[N / 2 + i] = (z[N / 2 + i] * fb[N / 2 - 1 - i]) + (z[i] * fb[N / 2 + i]);
    x[N / 2 - 1 - i] = (z[N / 2 + i] * fb[N + N / 2 - 1 - i]) + (z[i] * fb[N + N / 2 + i]);

    z[i] = z0;

    z[N + i] = z2;
  }

  for (i = 0; i < N / 4; i++) {
    float tmp = x[i];
    x[i] = x[N * 3 / 4 + i];
    x[N / 2 + i] = x[N / 4 + i];
    x[N / 4 + i] = tmp;
    x[N * 3 / 4 + i] = z[i] * fb[N / 2 + i];
  }
}

void multFDG1G2(float* const x,
                float* const z,
                float const* const d,
                int const N,
                float* buf) {
  int i;
  float* xh = buf;
  float* secOne = buf + N / 2;
  float* secTwo = &x[N / 4];

  copyFLOAT(x, secOne, N / 4);
  copyFLOAT(secTwo, xh, N / 2);
  copyFLOAT(z, x, N / 2);
  copyFLOAT(&secTwo[N / 2], &x[N / 2], N / 4);

  for (i = 0; i < N / 4; i++) {
    z[N / 2 - 1 - i] = (xh[i] * d[i]);

    x[N - 1 - i] = (xh[N / 2 - 1 - i] * d[N + N / 2 - 1 - i]) + (x[N / 2 + i] * d[N + N / 2 + i]);
    x[N - 1 - i] = x[N - 1 - i] + (z[N / 2 + i] * d[2 * N + i]);

    z[N / 2 + i] = x[i];

    x[i] = x[i] + (z[N + N / 2 - 1 - i] * d[2 * N + N / 2 + i]);

    z[N + N / 2 - 1 - i] = x[N - 1 - i];
  }

  for (i = N / 4; i < N / 2; i++) {
    z[N / 2 - 1 - i] = (xh[i] * d[i]) + (x[N - 1 - i] * d[N - 1 - i]);

    x[N - 1 - i] = (xh[N / 2 - 1 - i] * d[N + N / 2 - 1 - i]);
    x[N - 1 - i] = x[N - 1 - i] + (z[N / 2 + i] * d[2 * N + i]);

    z[N / 2 + i] = x[i] + (secOne[-N / 4 + i]) * (d[N / 2 + i]);

    x[i] = z[N / 2 + i] + (z[N + N / 2 - 1 - i] * d[2 * N + N / 2 + i]);

    z[N + N / 2 - 1 - i] = x[N - 1 - i];
  }
}

IIS_MDXT_ERROR IIS_MDXT_LD_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                 IIS_MDXT_CORE const type,
                                 float const* const pCurrBuffer,
                                 float* const pPrevBuffer,
                                 float const* const pWinCoef,
                                 float* const pOutBuffer,
                                 int const len) {
  IIS_DXT_ERROR err = IIS_DXT_NO_ERROR;
  int N;
  float buf[MAX_N / 2 + MAX_N / 4];

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len > MAX_N) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;

  if (checkOverlap(pCurrBuffer, pPrevBuffer, N, 2 * N, 0) || checkOverlap(pCurrBuffer, pWinCoef, N, 3 * N, 0) || checkOverlap(pCurrBuffer, pOutBuffer, N, N, 1) ||
      checkOverlap(pPrevBuffer, pWinCoef, 2 * N, 3 * N, 0) || checkOverlap(pPrevBuffer, pOutBuffer, 2 * N, N, 0) || checkOverlap(pWinCoef, pOutBuffer, 3 * N, N, 0)) {
    return IIS_MDXT_INVALID_BUFFER;
  }

  copyFLOAT(pCurrBuffer, pOutBuffer, N);

  switch (type) {
    case IIS_MDXT_MDCT_LD:
      multFDG1G2(pOutBuffer, pPrevBuffer, pWinCoef, N, buf);
      err = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);
      break;
    case IIS_MDXT_MDST_LD:

      smultFLOATip(-1, pOutBuffer, N / 4);
      smultFLOATip(-1, pOutBuffer + 3 * N / 4, N / 4);

      multFDG1G2(pOutBuffer, pPrevBuffer, pWinCoef, N, buf);

      smultFLOATip(-1, pOutBuffer + N / 2, N / 2);

      err = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);

      smultFLOATip(-1, pOutBuffer, N);
      break;
    default:
      return IIS_MDXT_INVALID_TYPE;
  }

  if (err != IIS_DXT_NO_ERROR) return IIS_MDXT_INTERNAL_ERROR;

  smultFLOATip(2.0, pOutBuffer, N);

  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_IMDXT_LD_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                  IIS_MDXT_CORE const type,
                                  float const* const pCurrBuffer,
                                  float* const pOlaBuffer,
                                  float const* const pWinCoef,
                                  float* const pOutBuffer,
                                  int const len) {
  IIS_DXT_ERROR err = IIS_DXT_NO_ERROR;
  int N;
  float norm;

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;
  norm = 1.0f / (float)N;

  if (checkOverlap(pCurrBuffer, pOlaBuffer, N, 2 * N, 0) || checkOverlap(pCurrBuffer, pWinCoef, N, 3 * N, 0) || checkOverlap(pCurrBuffer, pOutBuffer, N, N, 1) ||
      checkOverlap(pOlaBuffer, pWinCoef, 2 * N, 3 * N, 0) || checkOverlap(pOlaBuffer, pOutBuffer, 2 * N, N, 0) || checkOverlap(pWinCoef, pOutBuffer, 3 * N, N, 0)) {
    return IIS_MDXT_INVALID_BUFFER;
  }

  smulFLOAT(norm, pCurrBuffer, pOutBuffer, N);

  switch (type) {
    case IIS_MDXT_IMDCT_LD:
      err = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);

      multE2_DinvF(pOutBuffer, pWinCoef, pOlaBuffer, N);
      break;
    case IIS_MDXT_IMDST_LD:
      err = IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);

      smultFLOATip(-1, pOutBuffer + N / 2, N / 2);

      multE2_DinvF(pOutBuffer, pWinCoef, pOlaBuffer, N);

      smultFLOATip(-1, pOutBuffer, N / 4);
      smultFLOATip(-1, pOutBuffer + 3 * N / 4, N / 4);
      break;
    default:
      return IIS_MDXT_INVALID_TYPE;
  }

  if (err != IIS_DXT_NO_ERROR) return IIS_MDXT_INTERNAL_ERROR;

  return IIS_MDXT_NO_ERROR;
}

static void ldMDCT(int const N,
                   float const* const pInBuffer,
                   float* const pOutBuffer) {
  int n, k;

  for (k = 0; k < N; k++) {
    pOutBuffer[k] = 0;
    for (n = 0; n < 4 * N; n++) {
      pOutBuffer[k] += 2 * pInBuffer[n] * (float)cos(M_PI / N * (n + .5 - N / 2.0) * (k + .5));
    }
  }
}

static void ldIMDCT(int const N,
                    float const* const pInBuffer,
                    float* const pOutBuffer) {
  int n, k;

  for (n = 0; n < 4 * N; n++) {
    pOutBuffer[n] = 0;
    for (k = 0; k < N; k++) {
      pOutBuffer[n] -= pInBuffer[k] * (float)cos(M_PI / N * (n + .5 - N / 2.0) * (k + .5)) / N;
    }
  }
}

static void ldMDST(int const N,
                   float const* const pInBuffer,
                   float* const pOutBuffer) {
  int n, k;

  for (k = 0; k < N; k++) {
    pOutBuffer[k] = 0;
    for (n = 0; n < 4 * N; n++) {
      pOutBuffer[k] += 2 * pInBuffer[n] * (float)sin(M_PI / N * (n + .5 - N / 2.0) * (k + .5));
    }
  }
}

static void ldIMDST(int const N,
                    float const* const pInBuffer,
                    float* const pOutBuffer) {
  int n, k;

  for (n = 0; n < 4 * N; n++) {
    pOutBuffer[n] = 0;
    for (k = 0; k < N; k++) {
      pOutBuffer[n] -= pInBuffer[k] * (float)sin(M_PI / N * (n + .5 - N / 2.0) * (k + .5)) / N;
    }
  }
}

static const int vzLd[4][4] = {{+1, +1, +1, -1},
                               {-1, +1, +1, +1},
                               {+1, +1, +1, -1},
                               {-1, +1, +1, +1}};

IIS_MDXT_ERROR IIS_MDXT_LD_REFERENCE_IMPLEMENTATION_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                          IIS_MDXT_CORE const type,
                                                          float const* const pCurrBuffer,
                                                          float* const pPrevBuffer,
                                                          float const* const pWinCoef,
                                                          float* const pOutBuffer,
                                                          int const len,
                                                          IIS_MDXT_MODE const mode) {
  int N;
  int i = 0;
  float buffer[16384];

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;

  if (checkOverlap(pCurrBuffer, pPrevBuffer, N, 2 * N, 0) || checkOverlap(pCurrBuffer, pWinCoef, N, 3 * N, 0) || checkOverlap(pCurrBuffer, pOutBuffer, N, N, 1) ||
      checkOverlap(pPrevBuffer, pWinCoef, 2 * N, 3 * N, 0) || checkOverlap(pPrevBuffer, pOutBuffer, 2 * N, N, 0) || checkOverlap(pWinCoef, pOutBuffer, 3 * N, N, 0)) {
    return IIS_MDXT_INVALID_BUFFER;
  }

  switch (mode) {
    case IIS_MDXT_LD_FAST:
      setFLOAT(0.F, buffer, 4 * N);

      copyFLOAT(pPrevBuffer + N / 4, buffer, (3 * N - N / 4));
      copyFLOAT(pCurrBuffer, buffer + (3 * N - N / 4), N);

      copyFLOAT(pPrevBuffer + N, pPrevBuffer, 2 * N);
      copyFLOAT(pCurrBuffer, pPrevBuffer + (2 * N), N);

      multFLOAT(pWinCoef, buffer, buffer, 4 * N);

      for (i = 0; i < N / 2; i++) {
        pOutBuffer[i] = vzLd[type][0] * (buffer[N / 2 - 1 - i] - buffer[2 * N + N / 2 - 1 - i]);
        pOutBuffer[i] += vzLd[type][1] * (buffer[N / 2 + i] - buffer[2 * N + N / 2 + i]);
        pOutBuffer[N / 2 + i] = vzLd[type][2] * (buffer[N + i] - buffer[3 * N + i]);
        pOutBuffer[N / 2 + i] += vzLd[type][3] * (buffer[2 * N - 1 - i] - buffer[4 * N - 1 - i]);
      }

      switch (type) {
        case IIS_MDXT_MDCT_LD:
          IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);
          break;
        case IIS_MDXT_MDST_LD:
          IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }
      smultFLOATip(2.0, pOutBuffer, N);
      break;
    case IIS_MDXT_LD_LOOP:
      setFLOAT(0.F, buffer, 4 * N);

      copyFLOAT(pPrevBuffer + N / 4, buffer, (3 * N - N / 4));
      copyFLOAT(pCurrBuffer, buffer + (3 * N - N / 4), N);

      copyFLOAT(pPrevBuffer + N, pPrevBuffer, 2 * N);
      copyFLOAT(pCurrBuffer, pPrevBuffer + (2 * N), N);

      multFLOAT(pWinCoef, buffer, buffer, 4 * N);

      switch (type) {
        case IIS_MDXT_MDCT_LD:
          ldMDCT(N, buffer, pOutBuffer);
          break;
        case IIS_MDXT_MDST_LD:
          ldMDST(N, buffer, pOutBuffer);
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }
      break;
    default:
      return IIS_MDXT_INVALID_MODE;
  }
  return IIS_MDXT_NO_ERROR;
}

IIS_MDXT_ERROR IIS_IMDXT_LD_REFERENCE_IMPLEMENTATION_Apply(HANDLE_IIS_MDXT const hIisMdxt,
                                                           IIS_MDXT_CORE const type,
                                                           float const* const pCurrBuffer,
                                                           float* const pOlaBuffer,
                                                           float const* const pWinCoef,
                                                           float* const pOutBuffer,
                                                           int const len,
                                                           IIS_MDXT_MODE const mode) {
  int N;
  int i = 0;
  float norm = 0;
  float buffer[16384] = {0.0f};

  if (hIisMdxt == NULL) {
    return IIS_MDXT_INVALID_HANDLE;
  }

  if (len != hIisMdxt->len) {
    return IIS_MDXT_LENGTH_ERROR;
  }

  N = hIisMdxt->len;
  norm = 1.0f / (float)N;

  if (checkOverlap(pCurrBuffer, pOlaBuffer, N, 2 * N, 0) || checkOverlap(pCurrBuffer, pWinCoef, N, 3 * N, 0) || checkOverlap(pCurrBuffer, pOutBuffer, N, N, 1) ||
      checkOverlap(pOlaBuffer, pWinCoef, 2 * N, 3 * N, 0) || checkOverlap(pOlaBuffer, pOutBuffer, 2 * N, N, 0) || checkOverlap(pWinCoef, pOutBuffer, 3 * N, N, 0)) {
    return IIS_MDXT_INVALID_BUFFER;
  }

  switch (mode) {
    case IIS_MDXT_LD_FAST:
      smulFLOAT(norm, pCurrBuffer, pOutBuffer, N);
      switch (type) {
        case IIS_MDXT_IMDCT_LD:
          IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DCT_IV, pOutBuffer, pOutBuffer, N);
          break;
        case IIS_MDXT_IMDST_LD:
          IIS_DXT_Apply(hIisMdxt->phIisDxt, IIS_DST_IV, pOutBuffer, pOutBuffer, N);
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }
      for (i = 0; i < N / 2; i++) {
        buffer[i] = -vzLd[type][0] * pOutBuffer[N / 2 - 1 - i];
        buffer[N / 2 + i] = -vzLd[type][1] * pOutBuffer[i];
        buffer[2 * N / 2 + i] = -vzLd[type][2] * pOutBuffer[N / 2 + i];
        buffer[3 * N / 2 + i] = -vzLd[type][3] * pOutBuffer[N - 1 - i];
        buffer[4 * N / 2 + i] = -buffer[i];
        buffer[5 * N / 2 + i] = -buffer[N / 2 + i];
        buffer[6 * N / 2 + i] = -buffer[2 * N / 2 + i];
        buffer[7 * N / 2 + i] = -buffer[3 * N / 2 + i];
      }

      multFLOAT(pWinCoef, buffer, buffer, 4 * N);

      for (i = 0; i < 3 * N; i++) {
        buffer[i] += pOlaBuffer[i];
      }

      for (i = 0; i < N; i++) {
        pOutBuffer[i] = buffer[N / 4 + i];
        pOlaBuffer[i] = buffer[N + i];
        pOlaBuffer[N + i] = buffer[2 * N + i];
        pOlaBuffer[2 * N + i] = buffer[3 * N + i];
      }
      break;
    case IIS_MDXT_LD_LOOP:
      switch (type) {
        case IIS_MDXT_IMDCT_LD:
          ldIMDCT(N, pCurrBuffer, buffer);
          break;
        case IIS_MDXT_IMDST_LD:
          ldIMDST(N, pCurrBuffer, buffer);
          break;
        default:
          return IIS_MDXT_INVALID_TYPE;
      }

      multFLOAT(pWinCoef, buffer, buffer, 4 * N);

      for (i = 0; i < 3 * N; i++) {
        buffer[i] += pOlaBuffer[i];
      }

      for (i = 0; i < N; i++) {
        pOutBuffer[i] = buffer[N / 4 + i];
        pOlaBuffer[i] = buffer[N + i];
        pOlaBuffer[N + i] = buffer[2 * N + i];
        pOlaBuffer[2 * N + i] = buffer[3 * N + i];
      }
      break;
    default:
      return IIS_MDXT_INVALID_MODE;
  }
  return IIS_MDXT_NO_ERROR;
}
