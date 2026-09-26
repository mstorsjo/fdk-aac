
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

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "iisLPDComLib_tools.h"
#include "iisLPDComLib_constants.h"
#include "options.h"
#include "iisLPDEncLib_tcx_Tools.h"
#include "mathlib.h"
#include "iisutillib.h"

#define ORDER 16
#define FREQ_MAX 6400.0f

#define L_INTERPOL1 4

extern const float lpdenc_TableInterpolate4_1[];

void LPDEnc_tcx_AdaptLowFrequenciesEmphasis(float x[],
                                            int lg) {
  int i, j, k, i_max;
  float max, fac, tmp;

  k = 8;
  i_max = lg / 4;

  max = 0.01f;
  for (i = 0; i < i_max; i += k) {
    tmp = 0.01f;

    for (j = i; j < i + k; j++) {
      tmp += x[j] * x[j];
    }

    if (tmp > max) {
      max = tmp;
    }
  }

  fac = 10.0f;
  for (i = 0; i < i_max; i += k) {
    tmp = 0.01f;

    for (j = i; j < i + k; j++) {
      tmp += x[j] * x[j];
    }

    tmp = (float)sqrt(sqrt(max / tmp));
    if (tmp < fac) {
      fac = tmp;
    }
    for (j = i; j < i + k; j++) {
      x[j] *= fac;
    }
  }
}

float LPDEnc_tcx_GetGlobalGain(
    float x[],
    int nbitsSQ,
    int lg) {
  int i, j, iter;
  float gain, ener, tmp, target, fac, offset;
  float en[N_MAX / 4] = {0};

  for (i = 0; i < lg; i += 4) {
    ener = 0.01f;
    for (j = i; j < i + 4; j++) {
      ener += x[j] * x[j];
    }

    tmp = (float)log10(ener);
    en[i / 4] = 9.0f + 10.0f * tmp;
  }

  target = (6.0f / 4.0f) * (float)(nbitsSQ - (lg / 16));
  target = target < 0.0f ? 0.0f : target;

  fac = 128.0f;
  offset = fac;

  for (iter = 0; iter < 10; iter++) {
    fac *= 0.5f;
    offset -= fac;
    ener = 0.0f;
    for (i = 0; i < lg / 4; i++) {
      tmp = en[i] - offset;

      if (tmp > 3.0f) ener += tmp;
    }

    if (ener > target) offset += fac;
  }

  gain = (float)pow(10.0f, offset / 20.0f);

  return (gain);
}

void LPDEnc_tcx_InvSpectralNoiseShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[]) {
  int i, k;
  float y, mem_x, g1, g2, a = 0.0f, b = 0.0f;

  k = lg / FDNS_NPTS;

  mem_x = 0;
  for (i = 0; i < lg; i++) {
    if ((i % k) == 0) {
      g1 = old_gains[i / k];
      g2 = new_gains[i / k];

      a = (g1 + g2) / (2.0f * g1 * g2);
      b = (g1 - g2) / (2.0f * g1 * g2);
    }

    y = a * x[i] + b * mem_x;

    mem_x = x[i];
    x[i] = y;
  }

  return;
}

int LPDEnc_tcx_SelectMidFrameLSF(
    float lsf_old[],
    float lsf_mid[],
    float lsf_new[]) {
  int i, side;
  float d[ORDER + 1], w[ORDER], tmp, dist1, dist2;

  d[0] = lsf_mid[0];
  d[ORDER] = FREQ_MAX - lsf_mid[ORDER - 1];
  for (i = 1; i < ORDER; i++) {
    d[i] = lsf_mid[i] - lsf_mid[i - 1];
  }

  for (i = 0; i < ORDER; i++) {
    w[i] = (1.0f / d[i]) + (1.0f / d[i + 1]);
  }

  dist1 = 0.0f;
  dist2 = 0.0f;
  for (i = 0; i < ORDER; i++) {
    tmp = lsf_mid[i] - lsf_old[i];
    dist1 += tmp * tmp * w[i];

    tmp = lsf_mid[i] - lsf_new[i];
    dist2 += tmp * tmp * w[i];
  }

  if (dist1 < dist2)
    side = 0;
  else
    side = 1;

  return (side);
}
