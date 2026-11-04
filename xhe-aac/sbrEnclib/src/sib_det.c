
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
#include "sib_det.h"
#include "iisutillib.h"
#include "mathlib.h"
#include <math.h>
#include "fram_gen.h"

static void calculateSpectralTilt(const float *const *const energies,
                                  const int numberOfQmfBands,
                                  float *spectralTilt,
                                  const int calcLength,
                                  float *acf,
                                  float *memSpectralTilt);

HANDLE_ERROR_INFO
CreateSbrSibilantDetector(HANDLE_SBR_SIBILANT_DETECTOR *phSbrSibiliantDetector) {
  HANDLE_SBR_SIBILANT_DETECTOR h =
      (HANDLE_SBR_SIBILANT_DETECTOR)
          iisCalloc(1, sizeof(struct SBR_SIBILANT_DETECTOR));

  if (NULL == h) {
    return iisUtil_ERROR(CDI, "mem allocation failed");
  }

  setFLOAT(0.0f, h->spectralTilt, MAX_TILT_LENGTH);
  setFLOAT(0.0f, h->acf, MAX_ACF_LENGTH);
  h->memSpectralTilt = 0.0f;

  *phSbrSibiliantDetector = h;
  return noError;
}

HANDLE_ERROR_INFO
DeleteSbrSibilantDetector(HANDLE_SBR_SIBILANT_DETECTOR *phSbrSibiliantDetector) {
  if (phSbrSibiliantDetector != NULL) {
    if (*phSbrSibiliantDetector != NULL) {
      iisFree(*phSbrSibiliantDetector);
      *phSbrSibiliantDetector = NULL;
      return noError;
    }
  }
  return iisUtil_ERROR(CDI, "Invalid handle");
}

HANDLE_ERROR_INFO
AdvanceSbrSibilantDetector(HANDLE_SBR_SIBILANT_DETECTOR hSbrSibiliantDetector,
                           const float *const *const energies,
                           const int numberOfQmfBands,
                           const int no_cols) {
  if (NULL == hSbrSibiliantDetector)
    return iisUtil_ERROR(CDI, "invalid handle");

  calculateSpectralTilt(energies,
                        numberOfQmfBands,
                        hSbrSibiliantDetector->spectralTilt,
                        no_cols,
                        hSbrSibiliantDetector->acf,
                        &hSbrSibiliantDetector->memSpectralTilt);

  hSbrSibiliantDetector->no_cols = no_cols;

  return noError;
}

float sibilantDetected(HANDLE_SBR_SIBILANT_DETECTOR hSbrSibiliantDetector) {
  const int sibilant_threshold = 0;
  int k = 0;
  int sibcnt = 0;
  float returnvalue = 0.0f;

  if (NULL == hSbrSibiliantDetector) return 0.0f;

  for (k = 0; k < hSbrSibiliantDetector->no_cols; ++k) {
    if (hSbrSibiliantDetector->spectralTilt[k] > sibilant_threshold) {
      ++sibcnt;
    }
  }

  returnvalue = (float)sibcnt / hSbrSibiliantDetector->no_cols;

  return returnvalue;
}

HANDLE_ERROR_INFO
extractSibilantBorderCandidates(const float *const *const Energies,
                                const int noCols,
                                const int noQmfBands,
                                int *pSibBordersCnt,
                                const int splitband,
                                int *lastTilt,
                                int maxLookAhead)

{
  int band = 0;
  int gran = 0;
  float nrgLP[64] = {0};
  float nrgHP[64] = {0};
  int tilt[64] = {0};
  int usedLookAhead = noCols;

  assert(usedLookAhead <= maxLookAhead);
  assert(noCols <= 64);

  *pSibBordersCnt = 0;

  for (gran = 0; gran < noCols; gran++) {
    for (band = 0; band < splitband; band++) {
      nrgLP[gran] += Energies[gran + usedLookAhead][band];
    }

    for (band = splitband; band < noQmfBands; band++) {
      nrgHP[gran] += Energies[gran + usedLookAhead][band];
    }
    nrgLP[gran] = 10 * (float)log10(nrgLP[gran] + 1e-20);
    nrgHP[gran] = 10 * (float)log10(nrgHP[gran] + 1e-20);
  }

  for (gran = 0; gran < noCols; gran++) {
    if ((nrgHP[gran] - 0.25 * nrgLP[gran]) > -5) {
      tilt[gran] = 1;
    }
  }

  if (abs(tilt[0] - *lastTilt) == 1) {
    (*pSibBordersCnt)++;
  }
  for (gran = 1; gran < noCols; gran++) {
    if (abs(tilt[gran] - tilt[gran - 1]) == 1) {
      (*pSibBordersCnt)++;
    }
  }
  *lastTilt = tilt[noCols - 1];

  return noError;
}

#define TRIG_PREC double

static void CalcACF0_1(float *energies, float *acf, int numberOfBands) {
  int i = 0;

  float scale = 1.f / (float)numberOfBands;

  TRIG_PREC sinVal = 0.f, cosVal = 1.f;
  TRIG_PREC pi = 4. * atan(1.);
  TRIG_PREC cosValOrg = (TRIG_PREC)cos(pi / numberOfBands);
  TRIG_PREC sinValOrg = (TRIG_PREC)sin(pi / numberOfBands);

  acf[0] = 0.5f * energies[0];
  acf[1] = 0.5f * energies[0];

  for (i = 1; i < (numberOfBands + 1) / 2; i++) {
    TRIG_PREC tmp = cosVal * cosValOrg - sinVal * sinValOrg;
    sinVal = sinVal * cosValOrg + cosVal * sinValOrg;
    cosVal = tmp;

    acf[0] += (energies[i] + energies[numberOfBands - i]);
    acf[1] += (energies[i] - energies[numberOfBands - i]) * (float)cosVal;
  }
  if (0 == (numberOfBands % 2)) {
    acf[0] += energies[i];
    acf[1] += (float)(energies[i] * cosVal);
  }

  acf[0] = scale * acf[0];
  acf[1] = scale * acf[1];
}

static void calculateSpectralTilt(const float *const *const energies,
                                  const int numberOfQmfBands,
                                  float *spectralTilt,
                                  const int calcLength,
                                  float *acf,
                                  float *memSpectralTilt)

{
  int i;
  int j;
  float averagedEnergies[64];

  for (j = 0; j < calcLength; j++) {
    if (j == 0) {
      for (i = 0; i < numberOfQmfBands; i++) {
        averagedEnergies[i] = (energies[j][i] + energies[j + 1][i]) / 2;
      }
    } else {
      for (i = 0; i < numberOfQmfBands; i++) {
        averagedEnergies[i] = (energies[j - 1][i] + energies[j][i] + energies[j + 1][i]) / 3;
      }
    }

    CalcACF0_1(averagedEnergies, acf, numberOfQmfBands);

    acf[0] = max(acf[0], 0.00001f);

    spectralTilt[j] = (-acf[1]) / acf[0];

    if (j == 0) {
      spectralTilt[j] = 0.8f * (*memSpectralTilt) + 0.2f * spectralTilt[j];
    } else {
      spectralTilt[j] = 0.8f * spectralTilt[j - 1] + 0.2f * spectralTilt[j];
    }
  }
  *memSpectralTilt = spectralTilt[calcLength - 1];
}
