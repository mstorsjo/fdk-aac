
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

#include <assert.h>
#include <stdlib.h>
#include "iisutillib.h"
#include "psy_const.h"
#include "mathlib.h"
#include "iis_dxt.h"
#include "win_coef.h"
#include "transform.h"
#include "glob_con.h"

#ifndef sgn
#define sgn(A) ((A) > 0 ? (1) : (-1))
#endif

typedef struct T_LAPPED_TRANSFORM {
  TRANSFORM_GRANULE_LEN nGranuleLength;
  const float *pLongWindowSine;
  const float *pShortWindowSine;
  const float *pLongWindowKBD;
  const float *pShortWindowKBD;
  const float *pLongStartWindowSine;
  const float *pLongStopWindowSine;
  float *pLongStartWindowKBD;
  float *pLongStopWindowKBD;
  float *pShortWindowSineLpdStart;
  float *pShortWindowKBDLpdStart;
  HANDLE_IIS_DXT hIIsDxtTransformLong;
  HANDLE_IIS_DXT hIIsDxtTransformShort;
} LAPPED_TRANSFORM;

int iisaacfenc_CreateLappedTransform(HANDLE_LAPPED_TRANSFORM *hLappedTransform,
                                     TRANSFORM_GRANULE_LEN nGranuleLength) {
  int error = 0;

  if (error == 0) {
    if (NULL == (*hLappedTransform = (HANDLE_LAPPED_TRANSFORM)iisCalloc(1, sizeof(LAPPED_TRANSFORM)))) {
      error = 1;
    }
  }

  if (error == 0) {
    (*hLappedTransform)->nGranuleLength = nGranuleLength;
  }

  if (error == 0) {
    if (NULL == ((*hLappedTransform)->pShortWindowSineLpdStart = (float *)iisCalloc((*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2), sizeof(float)))) {
      error = 1;
    }
  }

  if (error == 0) {
    if (NULL == ((*hLappedTransform)->pShortWindowKBDLpdStart = (float *)iisCalloc((*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2), sizeof(float)))) {
      error = 1;
    }
  }

  if (error == 0) {
    int offset = (*hLappedTransform)->nGranuleLength / (2 * TRANS_FAC);
    switch (nGranuleLength) {
      case TRANSFORM_GRANULE_LEN_1024:
        (*hLappedTransform)->pLongWindowSine = LongWindowSine1024;
        (*hLappedTransform)->pLongWindowKBD = LongWindowKBD1024;
        (*hLappedTransform)->pShortWindowSine = ShortWindowSine128;
        (*hLappedTransform)->pShortWindowKBD = ShortWindowKBD128;
        offset = 0;
        setFLOAT(0.0f, (*hLappedTransform)->pShortWindowSineLpdStart, (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowSine256, &(*hLappedTransform)->pShortWindowSineLpdStart[offset], (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        setFLOAT(0.0f, (*hLappedTransform)->pShortWindowKBDLpdStart, (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowKBD256, &(*hLappedTransform)->pShortWindowKBDLpdStart[offset], (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        break;
      case TRANSFORM_GRANULE_LEN_768:
        (*hLappedTransform)->pLongWindowSine = LongWindowSine768;
        (*hLappedTransform)->pLongWindowKBD = LongWindowKBD768;
        (*hLappedTransform)->pShortWindowSine = ShortWindowSine96;
        (*hLappedTransform)->pShortWindowKBD = ShortWindowKBD96;
        offset = 0;
        setFLOAT(0.0, (*hLappedTransform)->pShortWindowSineLpdStart, (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowSine192, &(*hLappedTransform)->pShortWindowSineLpdStart[offset], (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        setFLOAT(0.0, (*hLappedTransform)->pShortWindowKBDLpdStart, (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowKBD192, &(*hLappedTransform)->pShortWindowKBDLpdStart[offset], (*hLappedTransform)->nGranuleLength / (TRANS_FAC / 2));
        break;
      default:
        error = 1;
        break;
    }
  }

  if (error == 0) {
    if (IIS_DXT_NO_ERROR != IIS_DXT_Create(&(*hLappedTransform)->hIIsDxtTransformLong, IIS_DCT_IV, nGranuleLength)) {
      error = 1;
    }
    if (IIS_DXT_NO_ERROR != IIS_DXT_Create(&(*hLappedTransform)->hIIsDxtTransformShort, IIS_DCT_IV, nGranuleLength / TRANS_FAC)) {
      error = 1;
    }
  }

  return error;
}

int iisaacfenc_DestroyLappedTransform(HANDLE_LAPPED_TRANSFORM *hLappedTransform) {
  int error = 0;

  if (*hLappedTransform != NULL) {
    IIS_DXT_Destroy(&(*hLappedTransform)->hIIsDxtTransformLong);
    IIS_DXT_Destroy(&(*hLappedTransform)->hIIsDxtTransformShort);

    if ((*hLappedTransform)->pShortWindowSineLpdStart) {
      iisFree((*hLappedTransform)->pShortWindowSineLpdStart);
    }
    if ((*hLappedTransform)->pShortWindowKBDLpdStart) {
      iisFree((*hLappedTransform)->pShortWindowKBDLpdStart);
    }
    iisFree(*hLappedTransform);
  }
  *hLappedTransform = NULL;

  return error;
}

int iisaacfenc_ApplyWindow(
    HANDLE_LAPPED_TRANSFORM hLappedTransform,
    const float *timeSignalIn,
    float *windowedSignalOut,
    const int currWindowShape,
    const int prevWindowShape,
    const int windowSequence,
    const int windowKernelType,
    const int useLpdModeNext,
    const int useLpdModePrev) {
  int i;
  float wSgn;
  const float *leftWindowPart, *rightWindowPart;
  int L = (int)hLappedTransform->nGranuleLength;
  int O;

  const int S = L / TRANS_FAC;

  switch (windowSequence) {
    case SHORT_WINDOW:
    case LONG_WINDOW:

      if (windowSequence == SHORT_WINDOW) {
        L /= TRANS_FAC;
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
      } else {
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pLongWindowSine : hLappedTransform->pLongWindowKBD;
      }
      wSgn = (windowKernelType & 2) ? -1.0f : 1.0f;
      for (i = 0; i < L / 2; i++) {
        windowedSignalOut[i] = wSgn * timeSignalIn[i] * leftWindowPart[i];
      }
      for (i = L / 2; i < L; i++) {
        windowedSignalOut[i] = timeSignalIn[i] * leftWindowPart[i];
      }

      if (windowSequence == SHORT_WINDOW) {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
      } else {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pLongWindowSine : hLappedTransform->pLongWindowKBD;
      }
      wSgn = (windowKernelType & 1) ? 1.0f : -1.0f;
      for (i = 0; i < L / 2; i++) {
        windowedSignalOut[L + i] = timeSignalIn[L + i] * rightWindowPart[L - 1 - i];
      }
      for (i = L / 2; i < L; i++) {
        windowedSignalOut[L + i] = wSgn * timeSignalIn[L + i] * rightWindowPart[L - 1 - i];
      }

      break;

    case START_WINDOW:

      leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pLongWindowSine : hLappedTransform->pLongWindowKBD;

      wSgn = (windowKernelType & 2) ? -1.0f : 1.0f;
      for (i = 0; i < L / 2; i++) {
        windowedSignalOut[i] = wSgn * timeSignalIn[i] * leftWindowPart[i];
      }
      for (i = L / 2; i < L; i++) {
        windowedSignalOut[i] = timeSignalIn[i] * leftWindowPart[i];
      }

      if (useLpdModeNext) {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSineLpdStart : hLappedTransform->pShortWindowKBDLpdStart;
        O = (L / 2) - S;
      } else {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
        O = (L - S) / 2;
      }
      wSgn = (windowKernelType & 1) ? 1.0f : -1.0f;
      for (i = 0; i < O; i++) {
        windowedSignalOut[L + i] = timeSignalIn[L + i];
      }
      for (i = 0; i < (L / 2 - O); i++) {
        windowedSignalOut[L + O + i] = timeSignalIn[L + O + i] * rightWindowPart[(L / 2 - O) * 2 - 1 - i];
      }
      for (i = (L / 2 - O); i < (L / 2 - O) * 2; i++) {
        windowedSignalOut[L + O + i] = wSgn * timeSignalIn[L + O + i] * rightWindowPart[(L / 2 - O) * 2 - 1 - i];
      }

      for (i = 0; i < O; i++) {
        windowedSignalOut[L + O + (L / 2 - O) * 2 + i] = 0.0f;
      }
      break;

    case STOP_WINDOW:

      if (useLpdModePrev) {
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSineLpdStart : hLappedTransform->pShortWindowKBDLpdStart;
        O = (L / 2) - S;
      } else {
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
        O = (L - S) / 2;
      }

      wSgn = (windowKernelType & 2) ? -1.0f : 1.0f;
      for (i = 0; i < O; i++) {
        windowedSignalOut[i] = 0.0f;
      }

      for (i = 0; i < (L / 2 - O); i++) {
        windowedSignalOut[O + i] = wSgn * timeSignalIn[O + i] * leftWindowPart[i];
      }

      for (i = (L / 2 - O); i < (L / 2 - O) * 2; i++) {
        windowedSignalOut[O + i] = timeSignalIn[O + i] * leftWindowPart[i];
      }

      for (i = 0; i < O; i++) {
        windowedSignalOut[O + (L / 2 - O) * 2 + i] = timeSignalIn[O + (L / 2 - O) * 2 + i];
      }

      rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pLongWindowSine : hLappedTransform->pLongWindowKBD;

      wSgn = (windowKernelType & 1) ? 1.0f : -1.0f;
      for (i = 0; i < L / 2; i++) {
        windowedSignalOut[L + i] = timeSignalIn[L + i] * rightWindowPart[L - 1 - i];
      }
      for (i = L / 2; i < L; i++) {
        windowedSignalOut[L + i] = wSgn * timeSignalIn[L + i] * rightWindowPart[L - 1 - i];
      }

      break;
    case STOPSTART_WINDOW:
      if (useLpdModePrev) {
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSineLpdStart : hLappedTransform->pShortWindowKBDLpdStart;
        O = (L / 2) - S;
      } else {
        leftWindowPart = (prevWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
        O = (L - S) / 2;
      }

      wSgn = (windowKernelType & 2) ? -1.0f : 1.0f;

      for (i = 0; i < O; i++) {
        windowedSignalOut[i] = 0.0f;
      }

      for (i = 0; i < (L / 2 - O); i++) {
        windowedSignalOut[O + i] = wSgn * timeSignalIn[O + i] * leftWindowPart[i];
      }

      for (i = (L / 2 - O); i < (L / 2 - O) * 2; i++) {
        windowedSignalOut[O + i] = timeSignalIn[O + i] * leftWindowPart[i];
      }

      for (i = 0; i < O; i++) {
        windowedSignalOut[O + (L / 2 - O) * 2 + i] = timeSignalIn[O + (L / 2 - O) * 2 + i];
      }

      if (useLpdModeNext) {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSineLpdStart : hLappedTransform->pShortWindowKBDLpdStart;
        O = (L / 2) - S;
      } else {
        rightWindowPart = (currWindowShape == SINE_WINDOW) ? hLappedTransform->pShortWindowSine : hLappedTransform->pShortWindowKBD;
        O = (L - S) / 2;
      }

      wSgn = (windowKernelType & 1) ? 1.0f : -1.0f;

      for (i = 0; i < O; i++) {
        windowedSignalOut[L + i] = timeSignalIn[L + i];
      }

      for (i = 0; i < (L / 2 - O); i++) {
        windowedSignalOut[L + O + i] = timeSignalIn[L + O + i] * rightWindowPart[(L / 2 - O) * 2 - 1 - i];
      }

      for (i = (L / 2 - O); i < (L / 2 - O) * 2; i++) {
        windowedSignalOut[L + O + i] = wSgn * timeSignalIn[L + O + i] * rightWindowPart[(L / 2 - O) * 2 - 1 - i];
      }

      for (i = 0; i < O; i++) {
        windowedSignalOut[L + O + (L / 2 - O) * 2 + i] = 0.0f;
      }

      break;
    default:
      WARN("Invalid window type!");
      break;
  }

  return 0;
}

int iisaacfenc_ApplyLappedTransform(
    HANDLE_LAPPED_TRANSFORM hLappedTransform,
    const float *windowedSignalIn,
    float *transformOut,
    const int windowSequence,
    const int windowKernelType) {
  int i;
  float ws1, ws2, wSgn;
  float *dctIn = transformOut;
  int L = (int)hLappedTransform->nGranuleLength;

  switch (windowSequence) {
    case SHORT_WINDOW:
      L /= TRANS_FAC;
      break;
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
    case STOPSTART_WINDOW:
      break;
    default:
      WARN("Invalid window type!");
      break;
  }

  for (i = 0; i < L / 2; i++) {
    ws1 = windowedSignalIn[i];
    ws2 = windowedSignalIn[L - 1 - i];
    dctIn[L / 2 + i] = ws1 - ws2;
  }

  for (i = 0; i < L / 2; i++) {
    ws1 = windowedSignalIn[L * 2 - 1 - i];
    ws2 = windowedSignalIn[L + i];
    dctIn[L / 2 - 1 - i] = ws1 - ws2;
  }

  if (windowKernelType > 0) {
    for (i = 0; i < L / 2; i++) {
      wSgn = dctIn[i];
      dctIn[i] = dctIn[L - 1 - i];
      dctIn[L - 1 - i] = wSgn;
    }
  }

  {
    IIS_DXT_Apply((windowSequence == SHORT_WINDOW) ? hLappedTransform->hIIsDxtTransformShort : hLappedTransform->hIIsDxtTransformLong,
                  IIS_DCT_IV, dctIn, dctIn, (windowSequence == SHORT_WINDOW) ? hLappedTransform->nGranuleLength / TRANS_FAC : hLappedTransform->nGranuleLength);
  }
  if (windowKernelType > 0) {
    assert(windowKernelType <= 3);
    for (i = 1 - (windowKernelType >> 1); i < L; i += 2) {
      dctIn[i] *= -1.0f;
    }
  }
  return 0;
}
