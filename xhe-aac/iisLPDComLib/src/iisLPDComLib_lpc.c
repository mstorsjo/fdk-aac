
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
#include <stdio.h>
#include "iisLPDComLib_lpc.h"

#define ORDER 16
#define LSF_GAP 128

#define M 16
#define L_DIV_1024 256

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif
#define PI2 6.283185307

#define SCALE1 (6400.0 / PI)

#define SCALE2 (PI / 6400.0)

#define NO_ITER 4
#define NO_POINTS 100

#define NC 8

#define L_WINDOW_PLUS 512

const float lsf_init[16] = {
    375.0, 750.0, 1125.0, 1500.0, 1875.0, 2250.0, 2625.0, 3000.0,
    3375.0, 3750.0, 4125.0, 4500.0, 4875.0, 5250.0, 5625.0, 6000.0};

static const float lag_window[17] = {
    1.0001f,
    0.999566371183f,
    0.998266612613f,
    0.996104103033f,
    0.993084457421f,
    0.989215493202f,
    0.984507262707f,
    0.978971838951f,
    0.972623467445f,
    0.965478420258f,
    0.957554817200f,
    0.948872864246f,
    0.939454317093f,
    0.929322779179f,
    0.918503403664f,
    0.907022833824f,
    0.894909143448f,
};

void LPDCom_lpc_ApplyLagWindow(
    float r[],
    int m) {
  int i;

  for (i = 0; i <= m; i++) {
    r[i] *= lag_window[i];
  }

  return;
}

void LPDCom_lpc_CreateAutocorrWindow(
    float *fh,
    int n1,
    int n2) {
  double cc, cte;
  int i;

  cte = 0.25 * PI2 / (float)n1;
  cc = 0.5 * cte - 0.25 * PI2;

  for (i = 0; i < n1; i++) {
    *fh++ = (float)cos(cc);
    cc += cte;
  }

  cte = 0.25 * PI2 / (float)n2;
  cc = 0.5 * cte;

  for (i = 0; i < n2; i++) {
    *fh++ = (float)cos(cc);
    cc += cte;
  }
}

static void initChebyshevGrid(float *grid) {
  int i;
  float temp;

  grid[0] = 1.0;
  grid[NO_POINTS] = -1.0;

  temp = (float)(PI2 / (2.0f * (float)NO_POINTS));
  for (i = 1; i < NO_POINTS; i++) {
    grid[i] = (float)cos((float)i * temp);
  }
}

static float getChebyshevPolynomial(
    float x,
    float *f,
    int n) {
  float b1, b2, b0, x2;
  int i;

  x2 = 2.0f * x;
  b2 = 1.0;
  b1 = x2 + f[1];

  for (i = 2; i < n; i++) {
    b0 = x2 * b1 - b2 + f[i];
    b2 = b1;
    b1 = b0;
  }

  return (x * b1 - b2 + 0.5f * f[n]);
}

void LPDCom_lpc_LPCToLSP(
    float *a,
    float *lsp,
    float *old_lsp) {
  float f1[NC + 1], f2[NC + 1];
  float *pa1, *pa2, *pf1, *pf2;
  int j, i, nf, ip;
  float xlow, ylow, xhigh, yhigh, xmid, ymid, xint;
  int flag = 0;
  float grid[NO_POINTS + 1];

  if (flag == 0) {
    initChebyshevGrid(grid);
    flag = 1;
  }

  pf1 = f1;
  pf2 = f2;
  *pf1++ = 1.0;
  *pf2++ = 1.0;
  pa1 = a + 1;
  pa2 = a + M;

  for (i = 0; i <= NC - 1; i++) {
    *pf1 = *pa1 + *pa2 - *(pf1 - 1);
    *pf2 = *pa1++ - *pa2-- + *(pf2 - 1);
    pf1++;
    pf2++;
  }

  nf = 0;
  ip = 0;

  pf1 = f1;

  xlow = grid[0];
  ylow = getChebyshevPolynomial(xlow, pf1, NC);

  j = 0;
  while ((nf < M) && (j < NO_POINTS)) {
    j++;
    xhigh = xlow;
    yhigh = ylow;
    xlow = grid[j];
    ylow = getChebyshevPolynomial(xlow, pf1, NC);

    if (ylow * yhigh <= 0.0) {
      j--;

      for (i = 0; i < NO_ITER; i++) {
        xmid = 0.5f * (xlow + xhigh);
        ymid = getChebyshevPolynomial(xmid, pf1, NC);

        if (ylow * ymid <= 0.0) {
          yhigh = ymid;
          xhigh = xmid;
        } else {
          ylow = ymid;
          xlow = xmid;
        }
      }

      xint = xlow - ylow * (xhigh - xlow) / (yhigh - ylow);

      lsp[nf] = xint;
      nf++;

      ip = 1 - ip;
      pf1 = ip ? f2 : f1;

      xlow = xint;
      ylow = getChebyshevPolynomial(xlow, pf1, NC);
    }
  }

  if (nf < M) {
    for (i = 0; i < M; i++) {
      lsp[i] = old_lsp[i];
    }
  }
}

void LPDCom_lpc_LSPToLSF(float lsp[], float lsf[], long m) {
  short i;

  for (i = 0; i < m; i++) {
    lsf[i] = (float)(acos(lsp[i]) * SCALE1);
  }
}

void LPDCom_lpc_LSFToLSP(
    float lsf[],
    float lsp[],
    int m) {
  int i;

  for (i = 0; i < m; i++) {
    lsp[i] = (float)cos((double)lsf[i] * (double)SCALE2);
  }
}

void LPDCom_lpc_Analyze(
    float *a,
    float *x,
    float *y,
    int l) {
  float s;
  int i;

  for (i = 0; i < l; i++) {
    s = x[i];
    s += a[1] * x[i - 1];
    s += a[2] * x[i - 2];
    s += a[3] * x[i - 3];
    s += a[4] * x[i - 4];
    s += a[5] * x[i - 5];
    s += a[6] * x[i - 6];
    s += a[7] * x[i - 7];
    s += a[8] * x[i - 8];
    s += a[9] * x[i - 9];
    s += a[10] * x[i - 10];
    s += a[11] * x[i - 11];
    s += a[12] * x[i - 12];
    s += a[13] * x[i - 13];
    s += a[14] * x[i - 14];
    s += a[15] * x[i - 15];
    s += a[16] * x[i - 16];
    y[i] = s;
  }
}

void LPDCom_lpc_Autocorrelate(
    float const *const x,
    float *r,
    int m,
    int n,
    float const *const fh) {
  float t[L_WINDOW_PLUS] = {0};
  float s;
  int i, j;

  for (i = 0; i < n; i++) {
    t[i] = x[i] * fh[i];
  }

  for (i = 0; i <= m; i++) {
    s = 0.0;

    for (j = 0; j < n - i; j++) {
      s += t[j] * t[j + i];
    }

    r[i] = s;
  }

  if (r[0] < 1.0) {
    r[0] = 1.0;
  }
}

void LPDCom_lpc_Synthesize(
    float a[],
    float x[],
    float y[],
    int l,
    float mem[],
    int update_m) {
  float buf[L_DIV_1024 + M];
  volatile float s;
  float *yy;
  int i, j;

  memcpy(buf, mem, M * sizeof(float));
  yy = &buf[M];

  for (i = 0; i < l; i++) {
    s = x[i];

    for (j = 1; j <= M; j += 4) {
      s -= a[j] * yy[i - j];
      s -= a[j + 1] * yy[i - (j + 1)];
      s -= a[j + 2] * yy[i - (j + 2)];
      s -= a[j + 3] * yy[i - (j + 3)];
    }
    yy[i] = s;
    y[i] = s;
  }

  if (update_m) {
    memcpy(mem, &yy[l - M], M * sizeof(float));
  }
}

void LPDCom_lpc_SynthesizeVarLen(float a[], int m, float x[], float y[], int l,
                                 float mem[], int update_m) {
  float buf[L_DIV_1024 + M];
  float s;
  float *yy;
  int i, j;

  memcpy(buf, mem, m * sizeof(float));
  yy = &buf[m];

  for (i = 0; i < l; i++) {
    s = x[i];

    for (j = 1; j <= m; j++) {
      s -= a[j] * yy[i - j];
    }
    yy[i] = s;
    y[i] = s;
  }

  if (update_m) {
    memcpy(mem, &yy[l - m], m * sizeof(float));
  }
}

static void getLSPPolynomial(
    float lsp[],
    float f[],
    int n,
    int flag) {
  float b;
  float *plsp;
  int i, j;

  plsp = lsp + flag - 1;
  f[0] = 1;
  b = -2.0f * *plsp;
  f[1] = b;

  for (i = 2; i <= n; i++) {
    plsp += 2;
    b = -2.0f * *plsp;
    f[i] = b * f[i - 1] + 2.0f * f[i - 2];

    for (j = i - 1; j > 1; j--) {
      f[j] += b * f[j - 1] + f[j - 2];
    }

    f[1] += b;
  }
}

void LPDCom_lpc_LSPToLPC(
    float *lsp,
    float *a) {
  float f1[NC + 1], f2[NC + 1];
  int i, k;
  float *pf1, *pf2, *pf1_1, *pf2_1, *pa1, *pa2;

  getLSPPolynomial(lsp, f1, NC, 1);
  getLSPPolynomial(lsp, f2, NC, 2);

  pf1 = f1 + NC;
  pf1_1 = pf1 - 1;
  pf2 = f2 + NC;
  pf2_1 = pf2 - 1;
  k = NC - 1;

  for (i = 0; i <= k; i++) {
    *pf1-- += *pf1_1--;
    *pf2-- -= *pf2_1--;
  }

  pa1 = a;
  *pa1++ = 1.0;
  pa2 = a + M;
  pf1 = f1 + 1;
  pf2 = f2 + 1;

  for (i = 0; i <= k; i++) {
    *pa1++ = 0.5f * (*pf1 + *pf2);
    *pa2-- = 0.5f * (*pf1++ - *pf2++);
  }
}

float LPDCom_lpc_LevinsonDurbin(float LPC[], float CC[], int Order) {
  int Loop, i;
  float Value, Sum, Sigma2, Gain;
  float RC[24];

  LPC[0] = 1.0f;
  RC[0] = -CC[1] / CC[0];
  LPC[1] = RC[0];
  Sigma2 = CC[0] + CC[1] * RC[0];

  for (Loop = 2; Loop <= Order; Loop++) {
    Sum = 0.0f;
    for (i = 0; i < Loop; i++) {
      Sum += CC[Loop - i] * LPC[i];
    }

    RC[Loop - 1] = -Sum / Sigma2;

    Sigma2 = Sigma2 * (1.0f - RC[Loop - 1] * RC[Loop - 1]);

    if (Sigma2 <= 1.0E-09f) {
      Sigma2 = 1.0E-09f;
      for (i = Loop; i <= Order; i++) {
        RC[i - 1] = 0.0f;
        LPC[i] = 0.0f;
      }

      break;
    }

    for (i = 1; i <= (Loop / 2); i++) {
      Value = LPC[i] + RC[Loop - 1] * LPC[Loop - i];
      LPC[Loop - i] += RC[Loop - 1] * LPC[i];
      LPC[i] = Value;
    }

    LPC[Loop] = RC[Loop - 1];
  }

  Gain = (float)(10.0 * log10(CC[0] / Sigma2));

  return (Gain);
}

void LPDCom_lpc_AWeight(float *a, float *ap, float gamma, int m) {
  float f;
  int i;

  ap[0] = a[0];
  f = gamma;

  for (i = 1; i <= m; i++) {
    ap[i] = f * a[i];
    f *= gamma;
  }
}

void LPDCom_lpc_InterpolateLSP(float lsp_old[],
                               float lsp_new[],
                               float a[],
                               int nb_subfr,
                               int m) {
  float lsp[M], *p_a, inc, fnew, fold;
  int i, k;

  inc = 1.0f / (float)nb_subfr;
  p_a = a;
  fnew = 0.0f;

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
