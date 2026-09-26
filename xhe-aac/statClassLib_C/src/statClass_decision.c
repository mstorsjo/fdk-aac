
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
#include <float.h>
#include <string.h>
#include "statClass_decision.h"
#include "iisutillib.h"

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

#ifndef max
#define max(a, b) ((a < b) ? b : a)
#endif

#define MAX_PAUSE 4

typedef struct _statclass_sw_decision {
  SCFLOAT lldPresent;
  SCFLOAT lldPast;
  SCFLOAT lldNext;

  SCFLOAT pSpeech;
  SCFLOAT pSpeechOld;

  SCFLOAT thresFixed;
  SCFLOAT thresFactor;
  SCFLOAT hyst_thres;
  SCFLOAT trans_thres;

  SCFLOAT speech_bias;

  int consecutivePause;

  STATCLASS_DECISION decOld;

} STATCLASS_SW_DECISION;

STATCLASS_ERROR_CODE STATCLASS_Decision_Open(HANDLE_STATCLASS_SW_DECISION* hSwDec, SCFLOAT stability, SCFLOAT speech_bias) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == hSwDec) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      if (NULL == (*hSwDec = (STATCLASS_SW_DECISION*)iisCalloc(1, sizeof(STATCLASS_SW_DECISION)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    (*hSwDec)->consecutivePause = 0;
    (*hSwDec)->decOld = STATCLASS_SPEECH;
    (*hSwDec)->thresFixed = 0.2f;
    (*hSwDec)->thresFactor = stability * (*hSwDec)->thresFixed;
    (*hSwDec)->hyst_thres = 0;
    (*hSwDec)->trans_thres = 0.75f * (*hSwDec)->thresFixed;
    (*hSwDec)->speech_bias = speech_bias;
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Decision_Close(HANDLE_STATCLASS_SW_DECISION* hSwDec) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hSwDec) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hSwDec);
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Decision_Get(HANDLE_STATCLASS_SW_DECISION hSwDec,
                                            STATCLASS_GMM_BUFFER* pGmmBuf,
                                            STATCLASS_DECISION* pDec,
                                            int bLookAhead) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i = 0;
  int isPause = 0;
  int isTransient = 0;
  int active = 0;
  int bSameSign = 0;
  SCFLOAT pMusic = (SCFLOAT)0.0;
  SCFLOAT pSpeech = (SCFLOAT)0.0;
  SCFLOAT tmpSpeech = (SCFLOAT)0.0;
  SCFLOAT tmpMusic = (SCFLOAT)0.0;

  for (i = 0; i < pGmmBuf->nFrames; i++) {
    isPause += pGmmBuf->pPresent[i].bPause;
    isTransient += pGmmBuf->pPresent[i].bTransient;
  }

  if (isPause < pGmmBuf->nFrames) {
    for (i = 0; i < pGmmBuf->nFrames; i++) {
      if (!pGmmBuf->pPresent[i].bPause) {
        tmpSpeech += pGmmBuf->pPresent[i].lldSpeech;
        tmpMusic += pGmmBuf->pPresent[i].lldMusic;
        active++;
      }
    }

    pSpeech = (SCFLOAT)exp(tmpSpeech / active);
    pMusic = (SCFLOAT)exp(tmpMusic / active);
    hSwDec->lldPresent = (SCFLOAT)((log(pSpeech) - log(pMusic)) / (fabs(log(pSpeech)) + fabs(log(pMusic)) + FLT_EPSILON));

    tmpSpeech = tmpMusic = (SCFLOAT)0.0;

    for (i = 0, active = 0; i < (pGmmBuf->bufSizePast + pGmmBuf->nFrames); i++) {
      if (!pGmmBuf->pPresent[i].bPause) {
        tmpSpeech += pGmmBuf->pPresent[i].lldSpeech;
        tmpMusic += pGmmBuf->pPresent[i].lldMusic;
        active++;
      }
    }

    pSpeech = (SCFLOAT)exp(tmpSpeech / active);
    pMusic = (SCFLOAT)exp(tmpMusic / active);
    hSwDec->lldPast = (SCFLOAT)((log(pSpeech) - log(pMusic)) / (fabs(log(pSpeech)) + fabs(log(pMusic)) + FLT_EPSILON));

    tmpSpeech = tmpMusic = (SCFLOAT)0.0;

    if (bLookAhead) {
      for (i = 0, active = 0; i < (pGmmBuf->bufSizeNext + pGmmBuf->nFrames); i++) {
        if (!pGmmBuf->pNext[i].bPause) {
          tmpSpeech += pGmmBuf->pNext[i].lldSpeech;
          tmpMusic += pGmmBuf->pNext[i].lldMusic;
          active++;
        }
      }

      pSpeech = (SCFLOAT)exp(tmpSpeech / active);
      pMusic = (SCFLOAT)exp(tmpMusic / active);
      hSwDec->lldNext = (SCFLOAT)((log(pSpeech) - log(pMusic)) / (fabs(log(pSpeech)) + fabs(log(pMusic)) + FLT_EPSILON));

      tmpSpeech = tmpMusic = (SCFLOAT)0.0;

    } else {
      hSwDec->lldNext = hSwDec->lldPresent;
    }

    if ((hSwDec->lldPast < 0 && hSwDec->lldNext < 0) || (hSwDec->lldPast > 0 && hSwDec->lldNext > 0)) {
      bSameSign = 1;

      if (bLookAhead) {
        for (i = 0, active = 0; i < pGmmBuf->bufSize; i++) {
          if (!pGmmBuf->data[i].bPause) {
            tmpSpeech += pGmmBuf->data[i].lldSpeech;
            tmpMusic += pGmmBuf->data[i].lldMusic;
            active++;
          }
        }

        pSpeech = (SCFLOAT)exp(tmpSpeech / active);
        pMusic = (SCFLOAT)exp(tmpMusic / active);
        hSwDec->pSpeech = (SCFLOAT)((log(pSpeech) - log(pMusic)) / (fabs(log(pSpeech)) + fabs(log(pMusic)) + FLT_EPSILON));
      } else {
        hSwDec->pSpeech = hSwDec->lldPast;
      }
    } else {
      bSameSign = 0;

      if ((bLookAhead && (fabs(hSwDec->lldPast) >= fabs(hSwDec->lldNext))) ||
          (!bLookAhead && (fabs(hSwDec->lldPresent + hSwDec->speech_bias - hSwDec->pSpeechOld) < hSwDec->trans_thres))) {
        hSwDec->pSpeech = hSwDec->lldPast;
      } else {
        hSwDec->pSpeech = hSwDec->lldNext;

        for (i = 0; i < pGmmBuf->bufSizePast; i++) {
          pGmmBuf->pPast[i].bPause = 1;
          pGmmBuf->pPast[i].lldMusic = (SCFLOAT)0.0;
          pGmmBuf->pPast[i].lldSpeech = (SCFLOAT)0.0;
        }
      }
    }

    if (hSwDec->speech_bias > 0) {
      hSwDec->pSpeech += (SCFLOAT)fabs(hSwDec->pSpeech * hSwDec->speech_bias);
    } else {
      hSwDec->pSpeech -= (SCFLOAT)fabs(hSwDec->pSpeech * hSwDec->speech_bias);
    }
    if (hSwDec->pSpeech > hSwDec->hyst_thres) {
      *pDec = STATCLASS_SPEECH;
    } else if (hSwDec->pSpeech < (-hSwDec->hyst_thres)) {
      *pDec = STATCLASS_MUSIC;
    } else {
      *pDec = hSwDec->decOld;
    }

    if ((*pDec != hSwDec->decOld) && !bSameSign) {
      for (i = 0; i < pGmmBuf->bufSizePast; i++) {
        pGmmBuf->pPast[i].bPause = 1;
        pGmmBuf->pPast[i].lldMusic = 0;
        pGmmBuf->pPast[i].lldSpeech = 0;
      }
    }

    if (hSwDec->thresFixed != 0.0) {
      hSwDec->hyst_thres -= hSwDec->thresFactor;
      hSwDec->hyst_thres = max(hSwDec->thresFactor, hSwDec->hyst_thres);
    }

    hSwDec->consecutivePause = 0;
  } else {
    hSwDec->consecutivePause += isPause;

    if (hSwDec->consecutivePause >= MAX_PAUSE) {
      for (i = 0; i < pGmmBuf->bufSizePast; i++) {
        pGmmBuf->pPast[i].bPause = 1;
        pGmmBuf->pPast[i].lldMusic = 0;
        pGmmBuf->pPast[i].lldSpeech = 0;
      }
      hSwDec->hyst_thres = 0.f;
    }
    *pDec = hSwDec->decOld;
  }

  if ((isTransient == 1) && (hSwDec->pSpeech < 0.2) && (hSwDec->pSpeech > 0.12)) {
    hSwDec->hyst_thres = 0.6f;
    *pDec = STATCLASS_MUSIC;
  }

  memmove((pGmmBuf->pAct + pGmmBuf->nFrames), pGmmBuf->pAct, (pGmmBuf->bufSize - pGmmBuf->nFrames) * sizeof(STATCLASS_GMM_LDS));
  hSwDec->decOld = *pDec;
  hSwDec->pSpeechOld = hSwDec->pSpeech;

  return error;
}
