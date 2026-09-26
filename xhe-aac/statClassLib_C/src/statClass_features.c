
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
#include "statClass_features.h"
#include "statClass_pitch_ol.h"
#include "statClass_pauseDetection.h"
#include "statClass_signal2mel.h"
#include "statClass_timeTransient.h"
#include "statClass_lpc.h"
#include "iisutillib.h"
#include "statClass_mel2plp.h"
#include "statClass_speechClassification.h"
#include "statClass_deltaLsf.h"
#include "statClass_deltaCep.h"
#include "statClass_ltpDelayVar.h"
#include "statClass_peakednessFreqVsTime.h"

#define MEM_SIZE_NEXT 128
#define MEM_SIZE_LAST 112
#define MEM_SIZE (MEM_SIZE_LAST + MEM_SIZE_NEXT)

#define FRAME_SIZE 256
#define LPC_ORDER 16

#define OLD_WSP_SIZE 231
#define WSP_SIZE 384
#define WSP_MEM_SIZE 500
#define EXC_SIZE 256
#define MEL_SIZE 40
#define PLP_CEP_SIZE 17

typedef struct _statclass_features {
  HANDLE_STATCLASS_PITCH hPitch;
  float voicing[3];
  float voicingFrame;
  short pitch[3];

  SCFLOAT* memSignal;
  SCFLOAT* totalSignal;
  SCFLOAT* wsp;

  HANDLE_STATCLASS_LPC hLpc;
  SCFLOAT* aLevinson;
  SCFLOAT* exc;

  HANDLE_STATCLASS_SIGNAL2MEL hSignal2Mel;
  SCFLOAT* mel;

  HANDLE_STATCLASS_TIMETRANSIENT hTimeTransient;
  int isTransient;

  HANDLE_STATCLASS_MEL2PLP hMel2Plp;
  SCFLOAT* plpCep;

  HANDLE_STATCLASS_SPEECH_CLASSIFICATION hSpeechClass;
  int speechClass;

  HANDLE_STATCLASS_DELTA_LSF hDeltaLsf;
  SCFLOAT deltaLsf;

  HANDLE_STATCLASS_DELTA_CEP hDeltaCep;
  SCFLOAT deltaCep;

  HANDLE_STATCLASS_LTP_DELAY_VAR hLtpDelVar;
  SCFLOAT ltpDelVar;

  HANDLE_STATCLASS_PEAKEDNESS hPeak;
  SCFLOAT peakResults[2];

} STATCLASS_FEATURES;

STATCLASS_ERROR_CODE STATCLASS_Feature_Open(HANDLE_STATCLASS_FEATURES* hFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i = 0;

  if (hFeatures == NULL) {
    error = STATCLASS_INVALID_POINTER;
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == (*hFeatures = (STATCLASS_FEATURES*)iisCalloc(1, sizeof(STATCLASS_FEATURES)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < 3; i++) {
      (*hFeatures)->voicing[i] = (SCFLOAT)0.0;
      (*hFeatures)->pitch[i] = 0;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_PitchOl_Open(&(*hFeatures)->hPitch);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Lpc_Open(&(*hFeatures)->hLpc);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Signal2Mel_Open(&(*hFeatures)->hSignal2Mel);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_TimeTransient_Open(&(*hFeatures)->hTimeTransient);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Mel2Plp_Open(&(*hFeatures)->hMel2Plp);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_SpeechClassification_Open(&(*hFeatures)->hSpeechClass);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_DeltaLsf_Open(&(*hFeatures)->hDeltaLsf);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_DeltaCep_Open(&(*hFeatures)->hDeltaCep);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_LtpDelayVar_Open(&(*hFeatures)->hLtpDelVar);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_PeakednessFreqVsTime_Open(&(*hFeatures)->hPeak);
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hFeatures)->aLevinson = (SCFLOAT*)iisCalloc((LPC_ORDER + 1), sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hFeatures)->wsp = (SCFLOAT*)iisCalloc(WSP_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hFeatures)->memSignal = (SCFLOAT*)iisCalloc(MEM_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    } else {
      memset((*hFeatures)->memSignal, 0, MEM_SIZE);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hFeatures)->totalSignal = (SCFLOAT*)iisCalloc(MEM_SIZE + FRAME_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hFeatures)->exc = (SCFLOAT*)iisCalloc(EXC_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hFeatures)->mel = (SCFLOAT*)iisCalloc(MEL_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hFeatures)->plpCep = (SCFLOAT*)iisCalloc(PLP_CEP_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Feature_Advance(HANDLE_STATCLASS_FEATURES hFeatures,
                                               SCFLOAT* pSignal,
                                               STATCLASS_EXTRACTED_FEATURES* pExtrFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  memcpy(hFeatures->totalSignal, hFeatures->memSignal, sizeof(SCFLOAT) * MEM_SIZE);
  memcpy(&hFeatures->totalSignal[MEM_SIZE], pSignal, sizeof(SCFLOAT) * FRAME_SIZE);

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_TimeTransient_Detect(hFeatures->hTimeTransient,
                                           pSignal,
                                           FRAME_SIZE,
                                           &hFeatures->isTransient);
    pExtrFeatures->bIsTransient = hFeatures->isTransient;
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Signal2Mel_Advance(hFeatures->hSignal2Mel,
                                         &hFeatures->totalSignal[112],
                                         hFeatures->totalSignal,
                                         hFeatures->mel);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Mel2Plp_Advance(hFeatures->hMel2Plp,
                                      hFeatures->mel,
                                      hFeatures->plpCep);

    memcpy(pExtrFeatures->plpCep, &hFeatures->plpCep[1], 16 * sizeof(SCFLOAT));
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_Lpc_Advance(hFeatures->hLpc,
                                  hFeatures->totalSignal,
                                  hFeatures->wsp,
                                  hFeatures->aLevinson,
                                  hFeatures->exc);
  }

  if (STATCLASS_NO_ERROR == error) {
    STATCLASS_PitchOl_Advance(hFeatures->hPitch,
                              hFeatures->pitch,
                              hFeatures->voicing,
                              hFeatures->wsp);

    hFeatures->voicingFrame = 0.0f;
    hFeatures->voicingFrame = (hFeatures->voicing[0] + hFeatures->voicing[1]) / 2;
    pExtrFeatures->voicingFrame = (SCFLOAT)hFeatures->voicingFrame;
  }

  if (STATCLASS_NO_ERROR == error) {
    STATCLASS_SpeechClassification_Advance(hFeatures->hSpeechClass,
                                           hFeatures->wsp,
                                           hFeatures->aLevinson,
                                           hFeatures->voicing,
                                           hFeatures->pitch,
                                           &hFeatures->speechClass);
  }

  if (STATCLASS_NO_ERROR == error) {
    pExtrFeatures->bIsPause = STATCLASS_pauseDetection(&hFeatures->totalSignal[MEM_SIZE_LAST], FRAME_SIZE);
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_DeltaLsf_Advance(hFeatures->hDeltaLsf,
                                       hFeatures->aLevinson,
                                       &hFeatures->deltaLsf);

    pExtrFeatures->deltaLsf = hFeatures->deltaLsf;
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_DeltaCep_Advance(hFeatures->hDeltaCep,
                                       hFeatures->plpCep,
                                       &hFeatures->deltaCep);

    pExtrFeatures->deltaCep = hFeatures->deltaCep;
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_LtpDelayVar_Advance(hFeatures->hLtpDelVar,
                                          hFeatures->pitch,
                                          hFeatures->voicing,
                                          hFeatures->speechClass,
                                          &hFeatures->ltpDelVar);

    pExtrFeatures->ltpDelayVar = hFeatures->ltpDelVar;
  }

  if (STATCLASS_NO_ERROR == error) {
    error = STATCLASS_PeakednessFreqVsTime(hFeatures->hPeak,
                                           hFeatures->exc,
                                           hFeatures->wsp,
                                           hFeatures->peakResults);

    pExtrFeatures->peakTime = hFeatures->peakResults[0];
    pExtrFeatures->peakFreq = hFeatures->peakResults[1];
  }

  memcpy(hFeatures->memSignal, &hFeatures->totalSignal[FRAME_SIZE], MEM_SIZE * sizeof(SCFLOAT));
  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Feature_Close(HANDLE_STATCLASS_FEATURES* hFeatures) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hPitch) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_PitchOl_Close(&(*hFeatures)->hPitch);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hLpc) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Lpc_Close(&(*hFeatures)->hLpc);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hMel2Plp) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Mel2Plp_Close(&(*hFeatures)->hMel2Plp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hSignal2Mel) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_Signal2Mel_Close(&(*hFeatures)->hSignal2Mel);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hTimeTransient) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_TimeTransient_Close(&(*hFeatures)->hTimeTransient);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hSpeechClass) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_SpeechClassification_Close(&(*hFeatures)->hSpeechClass);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hDeltaLsf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_DeltaLsf_Close(&(*hFeatures)->hDeltaLsf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hDeltaCep) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_DeltaCep_Close(&(*hFeatures)->hDeltaCep);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hLtpDelVar) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_LtpDelayVar_Close(&(*hFeatures)->hLtpDelVar);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->hPeak) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      error = STATCLASS_PeakednessFreqVsTime_Close(&(*hFeatures)->hPeak);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->aLevinson) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->aLevinson);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->wsp) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->wsp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->memSignal) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->memSignal);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->totalSignal) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->totalSignal);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->exc) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->exc);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->mel) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->mel);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hFeatures)->plpCep) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hFeatures)->plpCep);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hFeatures) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hFeatures);
    }
  }

  return error;
}
