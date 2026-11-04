
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
#include "statClass_utils.h"
#include "statClass_speechClassification.h"

#define RC_LEN 17
#define FRAME_SIZE 256

typedef struct _statclass_speech_classification {
  int oldClass;
  SCFLOAT merit;
  SCFLOAT* rc;

} STATCLASS_SPEECH_CLASSIFICATION;

int feature_speechClassification(SCFLOAT* rc,
                                 SCFLOAT* signal,
                                 int signal_L,
                                 SCFLOAT* a,
                                 short* pitchDelay,
                                 float* voicing,
                                 int oldClass,
                                 SCFLOAT* pMerit) {
  SCFLOAT vm, tilt, zc = 0, ps, merit;
  int new_class;
  int i;

  vm = (SCFLOAT)0.5 * (voicing[0] + voicing[1]);

  poly2rc(a, rc, RC_LEN);

  tilt = rc[0];

  for (i = 1; i < signal_L; i++) {
    if ((signal[i - 1]) >= 0 && (signal[i] < 0)) {
      zc++;
    }
  }

  if (pitchDelay[1] - pitchDelay[0] < 0 && pitchDelay[2] - pitchDelay[1] < 0) {
    ps = (SCFLOAT)((-1) * (pitchDelay[1] - pitchDelay[0]) + (-1) * (pitchDelay[2] - pitchDelay[1]));
  } else if (pitchDelay[1] - pitchDelay[0] < 0) {
    ps = (SCFLOAT)((-1) * (pitchDelay[1] - pitchDelay[0]) + (pitchDelay[2] - pitchDelay[1]));
  } else if (pitchDelay[2] - pitchDelay[1] < 0) {
    ps = (SCFLOAT)((pitchDelay[1] - pitchDelay[0]) + (-1) * (pitchDelay[2] - pitchDelay[1]));
  } else {
    ps = (SCFLOAT)((pitchDelay[1] - pitchDelay[0]) + (pitchDelay[2] - pitchDelay[1]));
  }

  vm = (SCFLOAT)2.857 * vm - (SCFLOAT)1.286;
  if (vm > 1) {
    vm = 1;
  } else if (vm < 0) {
    vm = 0;
  }

  tilt = (SCFLOAT)(-0.5) * tilt + (SCFLOAT)0.5;

  if (tilt > 1) {
    tilt = 1;
  } else if (tilt < 0) {
    tilt = 0;
  }

  zc = (SCFLOAT)1.2 - zc / (SCFLOAT)50;

  if (zc > 1) {
    zc = 1;
  } else if (zc < 0) {
    zc = 0;
  }

  ps = (SCFLOAT)(-0.07143) * ps + (SCFLOAT)1.857;

  if (ps > 1) {
    ps = 1;
  } else if (ps < 0) {
    ps = 0;
  }

  merit = (2 * vm + tilt + zc + ps) / 5;
  *pMerit = merit;

  switch (oldClass) {
    case 4:
    case 3:
    case 2:
      if (merit <= (SCFLOAT)0.5) {
        new_class = 0;
      } else if (merit < (SCFLOAT)0.66) {
        new_class = 3;
      } else {
        new_class = 4;
      }
      break;
    case 0:
    case 1:
      if (merit > (SCFLOAT)0.66) {
        new_class = 2;
      } else if (merit > (SCFLOAT)0.5) {
        new_class = 1;
      } else {
        new_class = 0;
      }
      break;

    default:
      new_class = 0;
      break;
  }

  return new_class;
}

STATCLASS_ERROR_CODE STATCLASS_SpeechClassification_Open(HANDLE_STATCLASS_SPEECH_CLASSIFICATION* hSpeechClass) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (NULL == hSpeechClass) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    if (NULL == (*hSpeechClass = (STATCLASS_SPEECH_CLASSIFICATION*)iisCalloc(1, sizeof(STATCLASS_SPEECH_CLASSIFICATION)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hSpeechClass)->rc = (SCFLOAT*)iisCalloc(RC_LEN, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_SpeechClassification_Advance(HANDLE_STATCLASS_SPEECH_CLASSIFICATION hSpeechClass,
                                                            SCFLOAT* pWsp,
                                                            SCFLOAT* pA,
                                                            float* voicingHF,
                                                            short* pitchDelay,
                                                            int* speechClass) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    *speechClass = feature_speechClassification(hSpeechClass->rc,
                                                pWsp,
                                                FRAME_SIZE,
                                                pA,
                                                pitchDelay,
                                                voicingHF,
                                                hSpeechClass->oldClass,
                                                &hSpeechClass->merit);

    hSpeechClass->oldClass = *speechClass;
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_SpeechClassification_Close(HANDLE_STATCLASS_SPEECH_CLASSIFICATION* hSpeechClass) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hSpeechClass)->rc)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hSpeechClass)->rc);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hSpeechClass)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hSpeechClass);
    }
  }
  return error;
}
