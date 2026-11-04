
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

#include "statClass_deltaCep.h"
#include "iisutillib.h"

#define LPC_ORDER 16
#define MEMSIZE (50 * 17)
#define BUF_SIZE 50

typedef struct _statclass_delta_cep {
  SCFLOAT* deltaLsfBuf;
  SCFLOAT* w;
  SCFLOAT sum;

} STATCLASS_DELTA_CEP;

SCFLOAT feature_deltaCep(SCFLOAT* cep,
                         int cep_L,
                         SCFLOAT* deltaLsfBuf,
                         SCFLOAT* w,
                         SCFLOAT sum) {
  int i, j;
  int deltaLsfBufSize = BUF_SIZE;
  SCFLOAT feature = 0, tmp = 0;

  for (i = 0; i < cep_L; i++) {
    deltaLsfBuf[(deltaLsfBufSize - 1) + i * deltaLsfBufSize] = cep[i];
  }

  for (i = 0; i < deltaLsfBufSize; i++) {
    for (j = 1; j < (LPC_ORDER + 1); j++) {
      tmp = ((deltaLsfBuf[(deltaLsfBufSize - 1) + j * deltaLsfBufSize] - deltaLsfBuf[(deltaLsfBufSize - i - 1) + j * deltaLsfBufSize]) * (deltaLsfBuf[(deltaLsfBufSize - 1) + j * deltaLsfBufSize] - deltaLsfBuf[(deltaLsfBufSize - i - 1) + j * deltaLsfBufSize]));
      feature += (tmp * w[i]) / (sum * LPC_ORDER);
    }
  }

  for (i = 0; i < cep_L; i++) {
    for (j = 0; j < (BUF_SIZE - 1); j++) {
      deltaLsfBuf[j + i * deltaLsfBufSize] = deltaLsfBuf[(j + 1) + i * deltaLsfBufSize];
    }
  }

  return feature;
}

STATCLASS_ERROR_CODE STATCLASS_DeltaCep_Open(HANDLE_STATCLASS_DELTA_CEP* hDeltaCep) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i;

  if (NULL == hDeltaCep) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    if (NULL == (*hDeltaCep = (STATCLASS_DELTA_CEP*)iisCalloc(1, sizeof(STATCLASS_DELTA_CEP)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hDeltaCep)->deltaLsfBuf = (SCFLOAT*)iisCalloc(MEMSIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < MEMSIZE; i++) {
      (*hDeltaCep)->deltaLsfBuf[i] = 0;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hDeltaCep)->w = (SCFLOAT*)iisCalloc(BUF_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < BUF_SIZE; i++) {
      (*hDeltaCep)->w[i] = (SCFLOAT)(BUF_SIZE - i);
      (*hDeltaCep)->sum += (SCFLOAT)(BUF_SIZE - i);
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_DeltaCep_Advance(HANDLE_STATCLASS_DELTA_CEP hDeltaCep,
                                                SCFLOAT* pCep,
                                                SCFLOAT* pDeltaCep) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    *pDeltaCep = feature_deltaCep(pCep,
                                  (LPC_ORDER + 1),
                                  hDeltaCep->deltaLsfBuf,
                                  hDeltaCep->w,
                                  hDeltaCep->sum);
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_DeltaCep_Close(HANDLE_STATCLASS_DELTA_CEP* hDeltaCep) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (NULL == (*hDeltaCep)->deltaLsfBuf) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hDeltaCep)->deltaLsfBuf);
  }

  if (NULL == (*hDeltaCep)->w) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree((*hDeltaCep)->w);
  }

  if (NULL == *hDeltaCep) {
    error = STATCLASS_INVALID_POINTER;
  } else {
    iisFree(*hDeltaCep);
  }

  return error;
}
