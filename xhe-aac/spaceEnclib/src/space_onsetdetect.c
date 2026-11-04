
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

#include "mathlib.h"

#include "spaceEnclib_const.h"
#include "space_onsetdetect.h"

struct ONSET_DETECT {
  int maxTimeSlots;
  int minTransientDistance;
  int avgEnergyDistance;
  int lowerBoundOnsetDetection;
  int upperBoundOnsetDetection;
  float *pEnergyHist;
  float epsilonFloat;
};

HANDLE_ERROR_INFO
OnsetDetect_Open(HANDLE_ONSET_DETECT *phOnset, ONSET_DETECT_CONFIG *pOnsetDetectConfig) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == phOnset) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    *phOnset = (HANDLE_ONSET_DETECT)iisCalloc(1, sizeof(struct ONSET_DETECT));
    if (*phOnset == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    }
  }

  if (noError == error) {
    if (pOnsetDetectConfig != NULL) {
      (*phOnset)->maxTimeSlots = pOnsetDetectConfig->maxTimeSlots;
      (*phOnset)->lowerBoundOnsetDetection = pOnsetDetectConfig->lowerBoundOnsetDetection;
      (*phOnset)->upperBoundOnsetDetection = pOnsetDetectConfig->upperBoundOnsetDetection;
      (*phOnset)->epsilonFloat = pOnsetDetectConfig->epsilonFloat;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer to pOnsetDetectConfig.");
    }
  }

  if (noError == error) {
    if ((*phOnset)->upperBoundOnsetDetection < (*phOnset)->lowerBoundOnsetDetection) {
      error = iisUtil_ERROR(CDI, "Invalid configuration.");
    }
  }

  if (noError == error) {
    (*phOnset)->minTransientDistance = 8;

    (*phOnset)->avgEnergyDistance = 16;
  }

  if (noError == error) {
    if ((*phOnset)->maxTimeSlots > 0) {
      if (NULL == ((*phOnset)->pEnergyHist = (float *)iisCalloc(1, ((*phOnset)->avgEnergyDistance + (*phOnset)->maxTimeSlots) * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "error allocating memory.");
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid maxTimeSlots");
    }
  }

  if (noError == error) {
    setFLOAT((*phOnset)->epsilonFloat, (*phOnset)->pEnergyHist, (*phOnset)->avgEnergyDistance + (*phOnset)->maxTimeSlots);
  }

  return error;
}

HANDLE_ERROR_INFO
OnsetDetect_Close(HANDLE_ONSET_DETECT *phOnset) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL != phOnset) {
    if (NULL != *phOnset) {
      if (NULL != (*phOnset)->pEnergyHist) {
        iisFree((*phOnset)->pEnergyHist);
      }
      (*phOnset)->pEnergyHist = NULL;

      iisFree(*phOnset);
      *phOnset = NULL;
    }
  }
  return error;
}

HANDLE_ERROR_INFO
OnsetDetect_Update(HANDLE_ONSET_DETECT hOnset,
                   int timeSlots) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hOnset) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (error == noError) {
    if (timeSlots > hOnset->maxTimeSlots) {
      error = iisUtil_ERROR(CDI, "Invalid number timeSlots.");
    } else {
      int i;

      for (i = 0; i < hOnset->avgEnergyDistance; i++) {
        hOnset->pEnergyHist[i] = hOnset->pEnergyHist[i + timeSlots];
      }

      setFLOAT(hOnset->epsilonFloat, &hOnset->pEnergyHist[hOnset->avgEnergyDistance], timeSlots);
    }
  }

  return error;
}

HANDLE_ERROR_INFO
OnsetDetect_Apply(HANDLE_ONSET_DETECT hOnset,
                  int nTimeSlots,
                  int nHybridBands,
                  float **ppHybridDataReal,
                  float **ppHybridDataImag,
                  int prevPos,
                  float threshold,
                  int pTransientPos[MAX_NUM_TRANS], int bsHighRateMode) {
  HANDLE_ERROR_INFO error = noError;
  int currPos = nTimeSlots;
  int ts = 0;
  float p1 = 0.0f;
  float p2 = 0.0f;
  float *env = NULL;
  int trCnt = 0;
  int minTransDist = 0;
  int tsOffset = 0;

  if (NULL == hOnset) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (NULL == pTransientPos) {
    error = iisUtil_ERROR(CDI, "Invalid Pointer");
  }

  if (error == noError) {
    tsOffset = hOnset->avgEnergyDistance;

    setINT(-1, pTransientPos, MAX_NUM_TRANS);
  }

  if (error == noError) {
    env = hOnset->pEnergyHist;
    minTransDist = hOnset->minTransientDistance;
  }

  if (error == noError) {
    if ((nTimeSlots < 0) ||
        (nTimeSlots > hOnset->maxTimeSlots)) {
      error = iisUtil_ERROR(CDI, "Invalid number nTimeSlots.");
    }
  }

  if (noError == error) {
    if (hOnset->lowerBoundOnsetDetection < -1) {
      error = iisUtil_ERROR(CDI, "Invalid lowerBoundOnsetDetection.");
    }
  }

  if (noError == error) {
    if (hOnset->upperBoundOnsetDetection > nHybridBands) {
      error = iisUtil_ERROR(CDI, "upperBoundOnsetDetection exceeds nHybridBands.");
    }
  }

  if (error == noError) {
    const int lowerBoundOnsetDetection = hOnset->lowerBoundOnsetDetection;
    const int upperBoundOnsetDetection = hOnset->upperBoundOnsetDetection;
    const float epsilonFloat = hOnset->epsilonFloat;

    if ((ppHybridDataReal != NULL) && (ppHybridDataImag != NULL)) {
      for (ts = 0; ts < nTimeSlots; ++ts) {
        if ((ppHybridDataReal[ts] != NULL) && (ppHybridDataImag[ts] != NULL)) {
          env[tsOffset + ts] =
              norm2FLOAT(&ppHybridDataReal[ts][lowerBoundOnsetDetection + 1], upperBoundOnsetDetection - lowerBoundOnsetDetection - 1) +
              norm2FLOAT(&ppHybridDataImag[ts][lowerBoundOnsetDetection + 1], upperBoundOnsetDetection - lowerBoundOnsetDetection - 1) + epsilonFloat;
        } else {
          error = iisUtil_ERROR(CDI, "invalid pointer.");
          break;
        }
      }
    } else {
      error = iisUtil_ERROR(CDI, "invalid pointer.");
    }
  }

  if (error == noError) {
    if (prevPos > 0) {
      currPos = max(nTimeSlots, prevPos - nTimeSlots + minTransDist);
    }
  }

  if (error == noError) {
    const int M = hOnset->avgEnergyDistance;

    for (; (currPos < 2 * nTimeSlots) && (trCnt < MAX_NUM_TRANS); ++currPos) {
      p1 = env[(tsOffset - nTimeSlots) + currPos];

      p2 = 0.0f;
      for (ts = currPos - M; ts < currPos; ++ts) {
        assert((tsOffset - nTimeSlots) + ts >= 0);
        p2 += env[(tsOffset - nTimeSlots) + ts];
      }
      p2 /= (float)M;

      if (bsHighRateMode) {
        if (p1 / p2 > threshold * threshold) {
          pTransientPos[trCnt++] = currPos;
          currPos += minTransDist;
        }
      }
    }
  }

  return error;
}
