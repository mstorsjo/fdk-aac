
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "statClass_gmm.h"
#include "statClass_gmm_16k.h"
#include "statClass_gmm_14_7k.h"

#include "iisutillib.h"

STATCLASS_ERROR_CODE STATCLASS_Gmm_Open(HANDLE_STATCLASS_GMM* hGmm) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == hGmm) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      if (NULL == (*hGmm = (STATCLASS_GMM*)iisCalloc(1, sizeof(STATCLASS_GMM)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hGmm)->features = (SCFLOAT*)iisCalloc(FEATURE_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hGmm)->y = (SCFLOAT*)iisCalloc(FEATURE_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_Normalization(HANDLE_STATCLASS_GMM hGmm,
                                                 STATCLASS_EXTRACTED_FEATURES* pExtrFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  SCFLOAT* features;
  int i;

  features = hGmm->features;

  STATCLASS_Gmm_GetFeatures(hGmm,
                            pExtrFeatures);

  for (i = 0; i < FEATURE_SIZE; i++) {
    features[i] = (features[i] - hGmm->sNorm.mu[i]) / hGmm->sNorm.stddev[i];
  }

  STATCLASS_Gmm_SetFeatures(hGmm,
                            pExtrFeatures);

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_GetFeatures(HANDLE_STATCLASS_GMM hGmm,
                                               STATCLASS_EXTRACTED_FEATURES* pExtrFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (NULL == hGmm || NULL == pExtrFeatures) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    memcpy(hGmm->features, pExtrFeatures->plpCep, 16 * sizeof(SCFLOAT));
    hGmm->features[16] = pExtrFeatures->deltaLsf;
    hGmm->features[17] = pExtrFeatures->deltaCep;
    hGmm->features[18] = pExtrFeatures->ltpDelayVar;
    hGmm->features[19] = pExtrFeatures->voicingFrame;
    hGmm->features[20] = pExtrFeatures->peakTime;
    hGmm->features[21] = pExtrFeatures->peakFreq;
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_SetFeatures(HANDLE_STATCLASS_GMM hGmm,
                                               STATCLASS_EXTRACTED_FEATURES* pExtrFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  if (NULL == hGmm || NULL == pExtrFeatures) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    memcpy(pExtrFeatures->plpCep, hGmm->features, 16 * sizeof(SCFLOAT));
    pExtrFeatures->deltaLsf = hGmm->features[16];
    pExtrFeatures->deltaCep = hGmm->features[17];
    pExtrFeatures->ltpDelayVar = hGmm->features[18];
    pExtrFeatures->voicingFrame = hGmm->features[19];
    pExtrFeatures->peakTime = hGmm->features[20];
    pExtrFeatures->peakFreq = hGmm->features[21];
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_Init(HANDLE_STATCLASS_GMM hGmm, int internalSamplingRate) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  if (internalSamplingRate == 16000) {
    error = STATCLASS_Gmm_Init_16k(hGmm);
  } else if (internalSamplingRate == 14700) {
    error = STATCLASS_Gmm_Init_14_7k(hGmm);
  } else {
    error = STATCLASS_INVALID_CONFIG;
  }
  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_Close(HANDLE_STATCLASS_GMM* hGmm) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  int n;

  if (STATCLASS_NO_ERROR == error) {
    for (n = 0; n < N_MODELS; n++) {
      if (NULL == (*hGmm)->gmmModel[n].gmms) {
        error = STATCLASS_INVALID_POINTER;
      } else {
        iisFree((*hGmm)->gmmModel[n].gmms);
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hGmm)->gmmModel) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hGmm)->gmmModel);
    }
  }

  if (NULL == (*hGmm)->features) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hGmm)->features);
  }

  if (NULL == (*hGmm)->y) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hGmm)->y);
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hGmm) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hGmm);
    }
  }
  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Gmm_Likelihood(HANDLE_STATCLASS_GMM hGmm,
                                              STATCLASS_EXTRACTED_FEATURES* pExtrFeatures,
                                              STATCLASS_GMM_LDS* pLds) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  STATCLASS_GMMS* pGmms = NULL;
  int i = 0;
  int j = 0;
  int gmmCnt = 0;
  int modelCnt = 0;
  SCFLOAT s = (SCFLOAT)0.0;
  SCFLOAT f = (SCFLOAT)0.0;
  SCFLOAT l = (SCFLOAT)0.0;
  SCFLOAT* y = hGmm->y;

  for (modelCnt = 0; modelCnt < hGmm->nModels; modelCnt++) {
    for (gmmCnt = 0; gmmCnt < hGmm->gmmModel[modelCnt].nGmms; gmmCnt++) {
      pGmms = &hGmm->gmmModel[modelCnt].gmms[gmmCnt];

      if (!pExtrFeatures[0].bIsPause) {
        STATCLASS_Gmm_GetFeatures(hGmm,
                                  &pExtrFeatures[0]);

        for (i = 0; i < pGmms->dimension; i++) {
          hGmm->features[i] -= (SCFLOAT)pGmms->means[i];
        }

        for (i = 0; i < pGmms->dimension; i++) {
          y[i] = 0;
          for (j = 0; j < pGmms->dimension; j++) {
            y[i] += hGmm->features[j] * (SCFLOAT)pGmms->invSigma[i][j];
          }
        }

        s = 0;
        for (i = 0; i < pGmms->dimension; i++) {
          s += y[i] * hGmm->features[i];
        }

        f = (SCFLOAT)(exp(-0.5 * s) * pGmms->detSigma);
        l += pGmms->weights * f;
      } else {
        l = 0;
      }
    }

    if (modelCnt == MODEL_SPEECH) {
      if (l != 0.0) {
        pLds->lldSpeech = (SCFLOAT)log(l);
      } else {
        pLds->lldSpeech = (SCFLOAT)0.0;
      }
    } else if (modelCnt == MODEL_MUSIC) {
      if (l != 0.0) {
        pLds->lldMusic = (SCFLOAT)log(l);
      } else {
        pLds->lldMusic = (SCFLOAT)0.0;
      }
    } else {
    }
    l = 0;
  }
  pLds->bPause = pExtrFeatures->bIsPause;

  return error;
}
