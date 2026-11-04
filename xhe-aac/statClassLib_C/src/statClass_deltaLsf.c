
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
#include "statClass_deltaLsf.h"
#include "statClass_utils.h"
#include "iisutillib.h"
#include "statClass_utils.h"

#define MEMSIZE (51 * 16)
#define MODEL_ORDER 16
#define LSF_LAST_BUF_SIZE 50

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

typedef struct _statclass_delta_lsf {
  SCFLOAT* deltaLsfBuf;
  SCFLOAT* lsf;
  SCFLOAT* oldLsf;
  int deltaLsfBufSize;

} STATCLASS_DELTA_LSF;

SCFLOAT feature_deltaLsfs(SCFLOAT* a,
                          SCFLOAT* deltaLsfBuf,
                          SCFLOAT* lsf,
                          SCFLOAT* oldLsf,
                          int* deltaBuf) {
  int i, j;
  int deltaLsfBufPastFrame = LSF_LAST_BUF_SIZE;
  SCFLOAT feature = 0, tmp = 0;
  int deltaLsfBufSize = *deltaBuf;

  STATCLASS_a_lsp_conversion(a,
                             lsf,
                             oldLsf);

  memcpy(oldLsf, lsf, (MODEL_ORDER) * sizeof(SCFLOAT));

  deltaLsfBuf[deltaLsfBufPastFrame] = lsf[0];

  for (i = 1; i < MODEL_ORDER; i++) {
    deltaLsfBuf[(deltaLsfBufPastFrame) + i * (deltaLsfBufPastFrame + 1)] = lsf[i] - lsf[i - 1];
  }

  deltaLsfBufSize = min(deltaLsfBufPastFrame + 1, deltaLsfBufSize + 1);

  for (i = 0; i < (deltaLsfBufSize - 1); i++) {
    for (j = 0; j < MODEL_ORDER; j++) {
      tmp = (SCFLOAT)pow((deltaLsfBuf[deltaLsfBufPastFrame + j * (deltaLsfBufPastFrame + 1)] - deltaLsfBuf[(deltaLsfBufPastFrame - i - 1) + j * (deltaLsfBufPastFrame + 1)]), 2);
      feature += (SCFLOAT)(tmp / (deltaLsfBufSize * MODEL_ORDER));
    }
  }

  for (i = 0; i < (MODEL_ORDER); i++) {
    for (j = 0; j < deltaLsfBufPastFrame; j++) {
      deltaLsfBuf[j + i * (deltaLsfBufPastFrame + 1)] = deltaLsfBuf[(j + 1) + i * (deltaLsfBufPastFrame + 1)];
    }
  }

  *deltaBuf = deltaLsfBufSize;

  return feature;
}

STATCLASS_ERROR_CODE STATCLASS_DeltaLsf_Open(HANDLE_STATCLASS_DELTA_LSF* hDeltaLsf) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int i;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == hDeltaLsf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      if (NULL == (*hDeltaLsf = (STATCLASS_DELTA_LSF*)iisCalloc(1, sizeof(STATCLASS_DELTA_LSF)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      }
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hDeltaLsf)->deltaLsfBuf = (SCFLOAT*)iisCalloc(816, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < MEMSIZE; i++) {
      (*hDeltaLsf)->deltaLsfBuf[i] = 0.1847996f;
    }
    (*hDeltaLsf)->deltaLsfBufSize = 0;
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hDeltaLsf)->lsf = (SCFLOAT*)iisCalloc(MODEL_ORDER, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hDeltaLsf)->oldLsf = (SCFLOAT*)iisCalloc(MODEL_ORDER, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}
STATCLASS_ERROR_CODE STATCLASS_DeltaLsf_Advance(HANDLE_STATCLASS_DELTA_LSF hDeltaLsf,
                                                SCFLOAT* pA,
                                                SCFLOAT* pLsf) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    *pLsf = feature_deltaLsfs(pA,
                              hDeltaLsf->deltaLsfBuf,
                              hDeltaLsf->lsf,
                              hDeltaLsf->oldLsf,
                              &hDeltaLsf->deltaLsfBufSize);
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_DeltaLsf_Close(HANDLE_STATCLASS_DELTA_LSF* hDeltaLsf) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hDeltaLsf)->deltaLsfBuf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hDeltaLsf)->deltaLsfBuf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hDeltaLsf)->lsf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hDeltaLsf)->lsf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hDeltaLsf)->oldLsf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hDeltaLsf)->oldLsf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hDeltaLsf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hDeltaLsf);
    }
  }

  return error;
}
