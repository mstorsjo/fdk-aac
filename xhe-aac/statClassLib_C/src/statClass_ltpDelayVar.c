
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

#include "statClass_ltpDelayVar.h"
#include "iisutillib.h"

#define MEM_SIZE (100)

typedef struct _statclass_ltp_delay_var {
  SCFLOAT* gainBuf;
  int* classBuf;
  int* delayBuf;
  int* delta;

} STATCLASS_LTP_DELAY_VAR;

SCFLOAT feature_ltpDelayVar(short* delay,
                            float* gain,
                            int class,
                            int* classBuf,
                            int* delayBuf,
                            SCFLOAT* gainBuf,
                            int* delta) {
  int i;
  SCFLOAT feature = 0, voiced = 0, totalGain = 0;

  for (i = 0; i < 2; i++) {
    delayBuf[MEM_SIZE + i] = delay[i];
    gainBuf[MEM_SIZE + i] = gain[i];
    classBuf[MEM_SIZE + i] = class;
  }

  for (i = 0; i < MEM_SIZE + 2; i++) {
    if (gainBuf[i] < 0) {
      gainBuf[i] = 0;
    }
  }

  for (i = 0; i < MEM_SIZE + 1; i++) {
    delta[i] = delayBuf[i + 1] - delayBuf[i];
    if (delta[i] < 0) {
      delta[i] *= -1;
    }
  }

  for (i = 0; i < MEM_SIZE + 1; i++) {
    if (delta[i] > 1 && delta[i] < 10 && classBuf[i + 1] == 4) {
      voiced = voiced + gainBuf[i + 1] + gainBuf[i];
    } else if (delta[i] > 2 && delta[i] < 20 && classBuf[i + 1] == 3) {
      voiced = voiced + gainBuf[i + 1] + gainBuf[i];
    } else if (delta[i] <= 1 && classBuf[i + 1] == 4) {
      voiced = voiced - 0.5f * gainBuf[i + 1];
    } else if (delta[i] <= 1 && classBuf[i + 1] == 3) {
      voiced = voiced - gainBuf[i + 1];
    } else if (delta[i] >= 10 && classBuf[i + 1] == 4) {
      voiced = voiced - gainBuf[i];
    } else if (delta[i] >= 20 && classBuf[i + 1] >= 3) {
      voiced = voiced - 0.5f * gainBuf[i];
    } else if (classBuf[i + 1] < 2) {
    } else if (classBuf[i + 1] == 2) {
      voiced = voiced + gainBuf[i];
    } else {
    }
    totalGain = totalGain + gainBuf[i + 1];
  }

  if (totalGain > 0) {
    voiced = voiced / totalGain;
  } else {
    voiced = 0;
  }

  feature = voiced;

  for (i = 0; i < MEM_SIZE; i++) {
    delayBuf[i] = delayBuf[i + 2];
    gainBuf[i] = gainBuf[i + 2];
    classBuf[i] = classBuf[i + 2];
  }

  return feature;
}

STATCLASS_ERROR_CODE STATCLASS_LtpDelayVar_Open(HANDLE_STATCLASS_LTP_DELAY_VAR* hLtpDelVar) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i;

  if (NULL == hLtpDelVar) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    if (NULL == (*hLtpDelVar = (STATCLASS_LTP_DELAY_VAR*)iisCalloc(1, sizeof(STATCLASS_LTP_DELAY_VAR)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hLtpDelVar)->gainBuf = (SCFLOAT*)iisCalloc((MEM_SIZE + 2), sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hLtpDelVar)->delayBuf = (int*)iisCalloc((MEM_SIZE + 2), sizeof(int)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hLtpDelVar)->classBuf = (int*)iisCalloc((MEM_SIZE + 2), sizeof(int)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < (MEM_SIZE + 2); i++) {
      (*hLtpDelVar)->classBuf[i] = 0;
      (*hLtpDelVar)->delayBuf[i] = 0;
      (*hLtpDelVar)->gainBuf[i] = 0;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hLtpDelVar)->delta = (int*)iisCalloc((MEM_SIZE + 1), sizeof(int)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_LtpDelayVar_Advance(HANDLE_STATCLASS_LTP_DELAY_VAR hLtpDelVar,
                                                   short* pPitch,
                                                   float* pGain,
                                                   int class,
                                                   SCFLOAT* ltpDelayVar) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    *ltpDelayVar = feature_ltpDelayVar(pPitch,
                                       pGain,
                                       class,
                                       hLtpDelVar->classBuf,
                                       hLtpDelVar->delayBuf,
                                       hLtpDelVar->gainBuf,
                                       hLtpDelVar->delta);
  }

  return error;
}
STATCLASS_ERROR_CODE STATCLASS_LtpDelayVar_Close(HANDLE_STATCLASS_LTP_DELAY_VAR* hLtpDelVar) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (NULL == (*hLtpDelVar)->gainBuf) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hLtpDelVar)->gainBuf);
  }

  if (NULL == (*hLtpDelVar)->delayBuf) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hLtpDelVar)->delayBuf);
  }

  if (NULL == (*hLtpDelVar)->classBuf) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hLtpDelVar)->classBuf);
  }

  if (NULL == (*hLtpDelVar)->delta) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hLtpDelVar)->delta);
  }

  if (NULL == *hLtpDelVar) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree(*hLtpDelVar);
  }

  return error;
}
