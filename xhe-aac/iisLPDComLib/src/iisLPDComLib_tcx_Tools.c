
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
#include "mathlib.h"
#include "iis_fft.h"
#include "iisutillib.h"
#include "iisLPDComLib_tcx_Tools.h"
#include "iisLPDComLib_tcx_Transform.h"
#include "iisLPDComLib_tools.h"
#include "iisLPDComLib_constants.h"
#include "options.h"
#include "iisLPDComLib_lpc.h"

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

#define ORDER 16
#define FREQ_MAX 6400.0f

void LPDCom_tcx_AdaptLowFrequencyDeemphasis(
    float x[],
    int lg,
    float gains[]) {
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

  fac = 0.1f;
  for (i = 0; i < i_max; i += k) {
    tmp = 0.01f;

    for (j = i; j < i + k; j++) {
      tmp += x[j] * x[j];
    }

    tmp = (float)sqrt(tmp / max);

    if (tmp > fac) {
      fac = tmp;
    }

    for (j = i; j < i + k; j++) {
      x[j] *= fac;
    }

    gains[i / k] = fac;
  }
}

void LPDCom_tcx_LpcToSpecGains(HANDLE_IIS_FFT hTcxFft,
                               const float *lpcCoeffs,
                               const int lpcOrder,
                               float *mdct_gains,
                               const int lg) {
  float RealData[(FDNS_NPTS_1024 * L_FRAME_1024 / 2) / L_DIV_1024] = {0.0f};
  float ImagData[(FDNS_NPTS_1024 * L_FRAME_1024 / 2) / L_DIV_1024] = {0.0f};
  float tmp = 0;
  int i = 0;
  int sizeN = 2 * lg;

  for (i = 0; i < lpcOrder + 1; i++) {
    tmp = (float)(((float)i) * PI / (float)(sizeN));
    RealData[i] = lpcCoeffs[i] * (float)cos(tmp);
    ImagData[i] = -lpcCoeffs[i] * (float)sin(tmp);
  }

  if (NULL != hTcxFft) {
    IIS_FFT_Apply_CFFT(hTcxFft,
                       RealData,
                       ImagData,
                       RealData,
                       ImagData);
  }

  for (i = 0; i < sizeN / 2; i++) {
    mdct_gains[i] = 1.0f / (float)sqrt(RealData[i] * RealData[i] + ImagData[i] * ImagData[i]);
  }
}

void LPDCom_tcx_SpectralNoiseShaping(float x[], int lg, int FDNS_NPTS, float old_gains[], float new_gains[]) {
  int i, k;
  float y, mem_y, g1, g2, a = 0.0f, b = 0.0f;

  k = lg / FDNS_NPTS;

  mem_y = 0;
  for (i = 0; i < lg; i++) {
    if ((i % k) == 0) {
      g1 = old_gains[i / k];
      g2 = new_gains[i / k];
      a = 2.0f * g1 * g2 / (g1 + g2);
      b = (g2 - g1) / (g1 + g2);
    }

    y = a * x[i] + b * mem_y;

    x[i] = y;
    mem_y = y;
  }
}

void LPDCom_tcx_InterpolateTcxLSPs(float lsp_old[],
                                   float lsp_new[],
                                   float a[],
                                   int nb_subfr,
                                   int m) {
  float lsp[M], *p_a, inc, fnew, fold;
  int i;

  p_a = a;
  inc = 1.0f / (float)nb_subfr;
  fnew = 0.5f - 0.5f * inc;
  fold = 1.0f - fnew;

  for (i = 0; i < m; i++) {
    lsp[i] = (float)(lsp_old[i] * fold + lsp_new[i] * fnew);
  }

  LPDCom_lpc_LSPToLPC(lsp, p_a);
  p_a += (m + 1);

  LPDCom_lpc_LSPToLPC(lsp_old, p_a);
  p_a += (m + 1);
  LPDCom_lpc_LSPToLPC(lsp_new, p_a);
}

void LPDCom_tcx_InterpolateAcelpLSPs(
    float lsp_old[],
    float lsp_new[],
    float a[],
    int nb_subfr,
    int m) {
  float lsp[M], *p_a, inc, fnew, fold;
  int i, k;

  inc = 1.0f / (float)nb_subfr;
  p_a = a;
  fnew = 1.0f / (2.0f * nb_subfr);

  for (k = 0; k < nb_subfr; k++) {
    fold = 1.0f - fnew;

    for (i = 0; i < m; i++) {
      lsp[i] = (float)(lsp_old[i] * fold + lsp_new[i] * fnew);
    }

    fnew += inc;
    LPDCom_lpc_LSPToLPC(lsp, p_a);
    p_a += (m + 1);
  }

  LPDCom_lpc_LSPToLPC(lsp_new, p_a);
}
