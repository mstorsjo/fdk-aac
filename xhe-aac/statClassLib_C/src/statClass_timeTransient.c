
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
#include <string.h>
#include <float.h>
#include <assert.h>
#include "statClass_features.h"

#include "iisutillib.h"
#include "mathlib.h"
#include "statClass_timeTransient.h"

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

#ifndef max
#define max(a, b) ((a < b) ? b : a)
#endif

#ifndef PI
#define PI (3.14159265358979323846264338327950288419716939937511)
#endif

#define NUMBER_OF_BLOCKS 8

typedef struct statclass_timetransient {
  SCFLOAT firState1;
  SCFLOAT firState2;

  SCFLOAT accNrg;
  SCFLOAT maxAccNrg;
  SCFLOAT minAccNrg;
  SCFLOAT lastBlockNrg;

} STATCLASS_TIMETRANSIENT;

STATCLASS_ERROR_CODE STATCLASS_TimeTransient_Open(
    HANDLE_STATCLASS_TIMETRANSIENT* hTimeTransient) {
  STATCLASS_ERROR_CODE retCode = STATCLASS_NO_ERROR;

  if (hTimeTransient != NULL) {
    if ((*hTimeTransient) == NULL) {
      (*hTimeTransient) = (HANDLE_STATCLASS_TIMETRANSIENT)iisMalloc(sizeof(STATCLASS_TIMETRANSIENT));
      if ((*hTimeTransient) == NULL) {
        retCode = STATCLASS_MEMORY_ALLOC_ERROR;
      } else {
        memset((*hTimeTransient), 0, sizeof(STATCLASS_TIMETRANSIENT));
        (*hTimeTransient)->accNrg = FLT_MIN;
        (*hTimeTransient)->maxAccNrg = FLT_MIN;
        (*hTimeTransient)->minAccNrg = FLT_MAX;
      }
    }
  }

  return retCode;
}

STATCLASS_ERROR_CODE STATCLASS_TimeTransient_Detect(
    HANDLE_STATCLASS_TIMETRANSIENT hTimeTransient,
    SCFLOAT* pSignal,
    int inLen,
    int* pIsTransient) {
  STATCLASS_ERROR_CODE retCode = STATCLASS_NO_ERROR;
  int i, j;
  SCFLOAT enRatio;
  SCFLOAT tmpFilt;
  SCFLOAT pEnergy[NUMBER_OF_BLOCKS];

  if (hTimeTransient == NULL) {
    return STATCLASS_INVALID_POINTER;
  }
  if (pSignal == NULL) {
    return STATCLASS_INVALID_POINTER;
  }
  if (pIsTransient == NULL) {
    return STATCLASS_INVALID_POINTER;
  }
  if (inLen < 0) {
    return STATCLASS_FATAL_ERROR;
  }

  for (i = 0; i < NUMBER_OF_BLOCKS; i++) {
    pEnergy[i] = (SCFLOAT)0.0;

    for (j = 0; j < inLen / NUMBER_OF_BLOCKS; j++) {
      SCFLOAT const tmpUnfilt = pSignal[i * inLen / NUMBER_OF_BLOCKS + j];
      tmpFilt = 0.375f * tmpUnfilt - 0.5f * hTimeTransient->firState1 + 0.125f * hTimeTransient->firState2;

      if (fabs(tmpFilt) <= 1e-15) {
        tmpFilt = 0.f;
      }

      pEnergy[i] += tmpFilt * tmpFilt;
      hTimeTransient->firState2 = hTimeTransient->firState1;
      hTimeTransient->firState1 = tmpUnfilt;
    }
    pEnergy[i] /= (inLen / NUMBER_OF_BLOCKS);
  }

  hTimeTransient->maxAccNrg = FLT_MIN;
  hTimeTransient->minAccNrg = FLT_MAX;

  assert((NUMBER_OF_BLOCKS % 2) == 0);

  for (i = 0; i < NUMBER_OF_BLOCKS; i++) {
    const SCFLOAT blockNrg = pEnergy[i] + FLT_MIN;

    enRatio = blockNrg / hTimeTransient->accNrg;

    if (enRatio > hTimeTransient->maxAccNrg) {
      hTimeTransient->maxAccNrg = enRatio;
    }

    if (enRatio < hTimeTransient->minAccNrg) {
      hTimeTransient->minAccNrg = enRatio;
    }

    hTimeTransient->accNrg *= (SCFLOAT)0.9;

    if (blockNrg > hTimeTransient->accNrg) {
      hTimeTransient->accNrg = blockNrg;
    }

    hTimeTransient->lastBlockNrg = blockNrg;
  }

  (*pIsTransient) = 0;

  if ((hTimeTransient->maxAccNrg > 20.0) && (hTimeTransient->minAccNrg < 0.008)) {
    (*pIsTransient) = 1;
  } else {
    (*pIsTransient) = 0;
  }

  return retCode;
}

STATCLASS_ERROR_CODE STATCLASS_TimeTransient_Close(
    HANDLE_STATCLASS_TIMETRANSIENT* hTimeTransient) {
  STATCLASS_ERROR_CODE retCode = STATCLASS_NO_ERROR;

  if (hTimeTransient != NULL) {
    if ((*hTimeTransient) != NULL) iisFree((*hTimeTransient));
  }

  return retCode;
}
