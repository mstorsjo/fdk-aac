
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
#include <string.h>
#include "statClasslib.h"
#include "iisutillib.h"
#include "statClass_preemph.h"
#include "statClass_features.h"
#include "statClass_gmm.h"
#include "statClass_decision.h"

#define MU_PREEMPH 0.68
#define FRAME_SIZE 256
#define DELAY_LPC 128

typedef struct _statclass {
  STATCLASS_CONFIG config;
  STATCLASS_DECISION decision;
  STATCLASS_INFO info;

  int nFrames;
  int delay;

  SCFLOAT* pInputBuffer;
  SCFLOAT* pIn;
  int nSamplesLeft;

  int bProcess;
  int bLookAhead;

  SCFLOAT memPreEmph;

  HANDLE_STATCLASS_FEATURES hFeatures;
  STATCLASS_EXTRACTED_FEATURES* pExtrFeatures;

  HANDLE_STATCLASS_GMM hGmm;
  STATCLASS_GMM_BUFFER gmmBuf;

  HANDLE_STATCLASS_SW_DECISION hSwDec;

} STATCLASS;

STATCLASS_ERROR_CODE STATCLASS_Open(HANDLE_STATCLASS* hStatClass,
                                    STATCLASS_CONFIG* pConfig) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i;

  if (STATCLASS_NO_ERROR == error) {
    if (pConfig->samplingRate != 16000 && pConfig->samplingRate != 14700) {
      error = STATCLASS_INVALID_CONFIG;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass = (HANDLE_STATCLASS)iisCalloc(1, sizeof(STATCLASS)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (pConfig) {
      (*hStatClass)->config = *pConfig;
    } else {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (pConfig->nFramesLookAhead == 0) {
      (*hStatClass)->bLookAhead = 0;
      (*hStatClass)->gmmBuf.bufSizeNext = 0;
      (*hStatClass)->gmmBuf.bufSizePast = GMM_BUF_SIZE_PAST;

    } else {
      (*hStatClass)->bLookAhead = 1;

      if (pConfig->nFramesLookAhead > GMM_BUF_SIZE_MAX) {
        (*hStatClass)->gmmBuf.bufSizeNext = GMM_BUF_SIZE_MAX;
        (*hStatClass)->gmmBuf.bufSizePast = GMM_BUF_SIZE_MAX;

      } else {
        (*hStatClass)->gmmBuf.bufSizeNext = pConfig->nFramesLookAhead;
        (*hStatClass)->gmmBuf.bufSizePast = pConfig->nFramesLookAhead;
      }
    }
    (*hStatClass)->gmmBuf.bufSize = (pConfig->granularity / FRAME_SIZE) + (*hStatClass)->gmmBuf.bufSizeNext + (*hStatClass)->gmmBuf.bufSizePast;
  }

  if (STATCLASS_NO_ERROR == error) {
    (*hStatClass)->memPreEmph = 0;
    (*hStatClass)->nFrames = (*hStatClass)->config.granularity / FRAME_SIZE;
    if ((*hStatClass)->config.granularity % FRAME_SIZE) {
      error = STATCLASS_INVALID_CONFIG;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hStatClass)->pInputBuffer = (SCFLOAT*)iisCalloc(pConfig->granularity, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    } else {
      (*hStatClass)->pIn = (*hStatClass)->pInputBuffer;
      (*hStatClass)->nSamplesLeft = (*hStatClass)->config.granularity;
      (*hStatClass)->bProcess = 0;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Feature_Open(&(*hStatClass)->hFeatures);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Gmm_Open(&(*hStatClass)->hGmm);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Gmm_Init((*hStatClass)->hGmm, (*hStatClass)->config.samplingRate);
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hStatClass)->pExtrFeatures = (STATCLASS_EXTRACTED_FEATURES*)iisCalloc(pConfig->granularity / FRAME_SIZE, sizeof(STATCLASS_EXTRACTED_FEATURES)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < (pConfig->granularity / FRAME_SIZE); i++) {
      if (NULL == ((*hStatClass)->pExtrFeatures[i].plpCep = (SCFLOAT*)iisCalloc(16, sizeof(SCFLOAT)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hStatClass)->gmmBuf.data = (STATCLASS_GMM_LDS*)iisCalloc((*hStatClass)->gmmBuf.bufSize, sizeof(STATCLASS_GMM_LDS)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    (*hStatClass)->gmmBuf.nFrames = (*hStatClass)->config.granularity / FRAME_SIZE;

    if ((*hStatClass)->bLookAhead) {
      (*hStatClass)->gmmBuf.pNext = (*hStatClass)->gmmBuf.data;
      (*hStatClass)->gmmBuf.pPresent = (*hStatClass)->gmmBuf.data + (*hStatClass)->gmmBuf.bufSizeNext;

    } else {
      (*hStatClass)->gmmBuf.pNext = NULL;
      (*hStatClass)->gmmBuf.pPresent = (*hStatClass)->gmmBuf.data;
    }
    (*hStatClass)->gmmBuf.pPast = (*hStatClass)->gmmBuf.pPresent + (*hStatClass)->gmmBuf.nFrames;

    for (i = 0; i < (*hStatClass)->gmmBuf.bufSize; i++) {
      (*hStatClass)->gmmBuf.data[i].bPause = 1;
      (*hStatClass)->gmmBuf.data[i].bTransient = 0;
      (*hStatClass)->gmmBuf.data[i].lldMusic = 0;
      (*hStatClass)->gmmBuf.data[i].lldSpeech = 0;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    (*hStatClass)->delay = DELAY_LPC;
    if ((*hStatClass)->bLookAhead) {
      (*hStatClass)->delay += ((*hStatClass)->gmmBuf.bufSizeNext * FRAME_SIZE);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Decision_Open(&(*hStatClass)->hSwDec, (*hStatClass)->config.stability, (*hStatClass)->config.speechBias);
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Init(HANDLE_STATCLASS hStatClass,
                                    int* pnSamplesNext) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (hStatClass == NULL) {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (pnSamplesNext) {
      if (hStatClass->config.granularity % FRAME_SIZE) {
        error = STATCLASS_INVALID_CONFIG;
      }
      *pnSamplesNext = hStatClass->config.granularity;
    } else {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Advance(HANDLE_STATCLASS hStatClass,
                                       SCFLOAT* pBuffer,
                                       int nSamplesInBuffer,
                                       int* pnSamplesNext,
                                       int* pnSamplesUsed,
                                       STATCLASS_DECISION* pDecision) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  int frameNbr = 0;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == pnSamplesUsed) {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (pnSamplesNext) {
      if (nSamplesInBuffer < hStatClass->nSamplesLeft) {
        memcpy(hStatClass->pIn, pBuffer, nSamplesInBuffer * sizeof(SCFLOAT));

        hStatClass->nSamplesLeft -= nSamplesInBuffer;
        hStatClass->pIn += nSamplesInBuffer;
        *pnSamplesNext = hStatClass->nSamplesLeft;
        *pnSamplesUsed = nSamplesInBuffer;

        hStatClass->bProcess = 0;

      } else if (nSamplesInBuffer > hStatClass->nSamplesLeft) {
        memcpy(hStatClass->pIn, pBuffer, hStatClass->nSamplesLeft * sizeof(SCFLOAT));

        *pnSamplesUsed = hStatClass->nSamplesLeft;
        hStatClass->nSamplesLeft = hStatClass->config.granularity;
        hStatClass->pIn = hStatClass->pInputBuffer;

        *pnSamplesNext = hStatClass->config.granularity;

        hStatClass->bProcess = 1;

      } else {
        memcpy(hStatClass->pIn, pBuffer, nSamplesInBuffer * sizeof(SCFLOAT));

        hStatClass->nSamplesLeft = hStatClass->config.granularity;
        hStatClass->pIn = hStatClass->pInputBuffer;
        *pnSamplesNext = hStatClass->config.granularity;
        *pnSamplesUsed = nSamplesInBuffer;

        hStatClass->bProcess = 1;
      }

    } else {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  if (hStatClass->bProcess) {
    if (STATCLASS_NO_ERROR == error) {
      f_preemph(hStatClass->pInputBuffer, (SCFLOAT)MU_PREEMPH, hStatClass->config.granularity, &hStatClass->memPreEmph);
    }
    if (hStatClass->bLookAhead) {
      hStatClass->gmmBuf.pAct = hStatClass->gmmBuf.pNext + hStatClass->gmmBuf.nFrames - 1;
    } else {
      hStatClass->gmmBuf.pAct = hStatClass->gmmBuf.pPresent + hStatClass->gmmBuf.nFrames - 1;
    }

    for (frameNbr = 0; frameNbr < hStatClass->nFrames; frameNbr++) {
      if (STATCLASS_NO_ERROR == error) {
        error = STATCLASS_Feature_Advance(hStatClass->hFeatures,
                                          &hStatClass->pInputBuffer[frameNbr * FRAME_SIZE],
                                          &hStatClass->pExtrFeatures[frameNbr]);
      }

      if (hStatClass->pExtrFeatures[frameNbr].bIsTransient == 1 && hStatClass->config.codecType == STATCLASS_CODEC_MPEGH) {
        hStatClass->gmmBuf.pAct->bTransient = 1;
      } else {
        hStatClass->gmmBuf.pAct->bTransient = 0;
      }

      if (hStatClass->pExtrFeatures[frameNbr].bIsPause == 1) {
        hStatClass->gmmBuf.pAct->lldMusic = 0;
        hStatClass->gmmBuf.pAct->lldSpeech = 0;
        hStatClass->gmmBuf.pAct->bPause = 1;
      } else {
        if (STATCLASS_NO_ERROR == error) {
          error = STATCLASS_Gmm_Normalization(hStatClass->hGmm,
                                              &hStatClass->pExtrFeatures[frameNbr]);
        }

        if (STATCLASS_NO_ERROR == error) {
          error = STATCLASS_Gmm_Likelihood(hStatClass->hGmm,
                                           &hStatClass->pExtrFeatures[frameNbr],
                                           hStatClass->gmmBuf.pAct);
        }
      }
      if (hStatClass->gmmBuf.pAct > hStatClass->gmmBuf.data) {
        hStatClass->gmmBuf.pAct--;
      }
    }

    if (STATCLASS_NO_ERROR == error) {
      error = STATCLASS_Decision_Get(hStatClass->hSwDec,
                                     &hStatClass->gmmBuf,
                                     pDecision,
                                     hStatClass->bLookAhead);
    }
  }
  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Close(HANDLE_STATCLASS* hStatClass) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->hFeatures) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Feature_Close(&(*hStatClass)->hFeatures);
    }
  }
  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->hGmm) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Gmm_Close(&(*hStatClass)->hGmm);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->hSwDec) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Decision_Close(&(*hStatClass)->hSwDec);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->pInputBuffer) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hStatClass)->pInputBuffer);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < ((*hStatClass)->config.granularity / FRAME_SIZE); i++) {
      if (NULL == (*hStatClass)->pExtrFeatures[i].plpCep) {
        error = STATCLASS_INVALID_POINTER;
      } else {
        iisFree((*hStatClass)->pExtrFeatures[i].plpCep);
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->pExtrFeatures) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hStatClass)->pExtrFeatures);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hStatClass)->gmmBuf.data) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hStatClass)->gmmBuf.data);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (*hStatClass) {
      iisFree(*hStatClass);
    } else {
      error = STATCLASS_INVALID_POINTER;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_GetInfo(HANDLE_STATCLASS hStatClass,
                                       STATCLASS_INFO* pInfo) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  pInfo->granularity = hStatClass->config.granularity;
  pInfo->nSamplesDelay = hStatClass->delay;
  pInfo->samplingRate = hStatClass->config.samplingRate;

  return error;
}
