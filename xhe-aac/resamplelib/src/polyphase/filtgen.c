
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

#include "filtgen.h"
#include <stdio.h>
#include <math.h>
#include <limits.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

static const double Magic = 1.66F;

static double
besselI0(double x) {
  const double threshRatio = 1.0e-21;
  const double xSquare_forth = x * x / (2 * 2);
  double I0 = 0;

  int n = 0;
  double nthTerm = 1.0;
  I0 += nthTerm;

  n = 1;
  while (1) {
    nthTerm = nthTerm * (xSquare_forth / (n * n));
    I0 += nthTerm;
    if ((nthTerm < threshRatio * I0) || (n > 40)) {
      break;
    }
    n += 1;
  }
  return I0;
}

static void
EstimateFilterLength(struct FILT_PARAM* filtParam) {
  filtParam->nTaps = (int)((filtParam->attenuation - 7.95) / (14.36 * Magic * filtParam->bandwidth) + 1.0);
}

static void
EstimateBandwidth(struct FILT_PARAM* filtParam) {
  filtParam->bandwidth = (float)((filtParam->attenuation - 7.95) / (14.36 * Magic * filtParam->nTaps));
}

static void
EstimateBeta(struct FILT_PARAM* filtParam) {
  double attenuation;
  double beta;

  attenuation = filtParam->attenuation;

  if ((attenuation < 25.0) || (attenuation > 140.0)) {
    filtParam->beta = 0.0f;
    return;
  }

  if (attenuation > 50.0)
    beta = 0.1102 * (attenuation - 8.7);

  else {
    attenuation -= 21.0;
    beta = 0.5842 * pow(attenuation, 0.4) + 0.07886 * attenuation;
  }

  filtParam->beta = (float)beta;
}

void EstimateKaiserParam(struct FILT_PARAM* filtParam) {
  if (!filtParam->nTaps)
    EstimateFilterLength(filtParam);
  else
    EstimateBandwidth(filtParam);

  EstimateBeta(filtParam);
}

void KaiserBesselWindow(struct FILT_PARAM* filtParam) {
  double IBeta, Beta;
  double temp, inm1;
  double* coef;
  int i, Ndiv2;
  double tmpRound = 4e-16;

  Beta = filtParam->beta;
  coef = filtParam->coef;

  IBeta = 1.0 / besselI0(Beta);

  if (filtParam->nTaps & 0x01) {
    Ndiv2 = (filtParam->nTaps - 1) / 2;
    inm1 = 1.0 / (double)Ndiv2;
    coef[Ndiv2] = 1.0;
    for (i = 1; i < (Ndiv2 + 1); i++) {
      temp = i * inm1;

      temp = (1.0 - (temp * temp - tmpRound));
      coef[Ndiv2 + i] = besselI0(Beta * sqrt(temp)) * IBeta;

      coef[Ndiv2 - i] = coef[Ndiv2 + i];
    }

  } else {
    Ndiv2 = filtParam->nTaps / 2;
    inm1 = 1.0 / (double)Ndiv2;

    for (i = 0; i < Ndiv2; i++) {
      temp = ((double)i + 0.5) * inm1;

      coef[Ndiv2 + i] = besselI0(Beta * sqrt(1.0 - temp * temp - tmpRound)) * IBeta;

      coef[Ndiv2 - i - 1] = coef[Ndiv2 + i];
    }
  }
}

void WindowLowpass(struct FILT_PARAM* filtParam) {
  double normFreq;
  double temp;
  double gain;
  double* coef;
  int i, Ndiv2;

  normFreq = filtParam->lowpassFreq - Magic * filtParam->bandwidth / 2.0;
  coef = filtParam->coef;

  if (filtParam->nTaps & 0x01) {
    Ndiv2 = (filtParam->nTaps - 1) / 2;
    coef[Ndiv2] *= 2.0 * PI * normFreq;
    gain = coef[Ndiv2];

    for (i = 1; i < (Ndiv2 + 1); i++) {
      temp = (double)i;
      temp = sin(2.0 * PI * temp * normFreq) / temp;

      coef[Ndiv2 + i] *= temp;
      gain += coef[Ndiv2 + i];

      coef[Ndiv2 - i] *= temp;
      gain += coef[Ndiv2 - i];
    }

  } else {
    Ndiv2 = filtParam->nTaps / 2;
    gain = 0.0;

    for (i = 0; i < Ndiv2; i++) {
      temp = (double)i + 0.5;
      temp = sin(2.0 * PI * temp * normFreq) / temp;

      coef[Ndiv2 + i] *= temp;
      gain += coef[Ndiv2 + i];

      coef[Ndiv2 - i - 1] *= temp;
      gain += coef[Ndiv2 - i - 1];
    }
  }

  gain = filtParam->gain / gain;
  for (i = 0; i < filtParam->nTaps; i++)
    coef[i] *= gain;
}
