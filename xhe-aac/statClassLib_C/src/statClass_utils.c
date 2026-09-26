
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
#include "iisutillib.h"
#include "statClass_utils.h"

#define L_WINDOW_PLUS 512
#define PI2 6.283185307
#define ORDER 16
#define NO_POINTS 100
#define NO_ITER 4
#define M16k 20
#define TILT_FAC 0.68f
#define L_SUBFR 64
#define GAMMA1 0.92f
#define NC 8
#define NO_POINTS 100

#define SCALE1 1

const SCFLOAT STATCLASS_lag_window[17] = {
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

const SCFLOAT STATCLASS_E_ROM_Grid[101] = {
    1.00000F, 0.999507F, 0.998027F, 0.995562F, 0.992115F,
    0.987688F, 0.982287F, 0.975917F, 0.968583F, 0.960294F,
    0.951057F, 0.940881F, 0.929776F, 0.917755F, 0.904827F,
    0.891007F, 0.876307F, 0.860742F, 0.844328F, 0.827081F,
    0.809017F, 0.790155F, 0.770513F, 0.750111F, 0.728969F,
    0.707107F, 0.684547F, 0.661312F, 0.637424F, 0.612907F,
    0.587785F, 0.562083F, 0.535827F, 0.509041F, 0.481754F,
    0.453990F, 0.425779F, 0.397148F, 0.368124F, 0.338738F,
    0.309017F, 0.278991F, 0.248690F, 0.218143F, 0.187381F,
    0.156434F, 0.125333F, 0.0941082F, 0.0627904F, 0.0314107F,
    -8.09643e-008F,
    -0.0314108F, -0.0627906F, -0.0941084F, -0.125333F, -0.156435F,
    -0.187381F, -0.218143F, -0.248690F, -0.278991F, -0.309017F,
    -0.338738F, -0.368125F, -0.397148F, -0.425779F, -0.453991F,
    -0.481754F, -0.509041F, -0.535827F, -0.562083F, -0.587785F,
    -0.612907F, -0.637424F, -0.661312F, -0.684547F, -0.707107F,
    -0.728969F, -0.750111F, -0.770513F, -0.790155F, -0.809017F,
    -0.827081F, -0.844328F, -0.860742F, -0.876307F, -0.891007F,
    -0.904827F, -0.917755F, -0.929777F, -0.940881F, -0.951057F,
    -0.960294F, -0.968583F, -0.975917F, -0.982287F, -0.987688F,
    -0.992115F, -0.995562F, -0.998027F, -0.999507F, -1.00000F};

void rc2poly(SCFLOAT *k, SCFLOAT *a, int M) {
  int l, i, j;

  for (l = 0; l < M; l++) {
    a[l] = -k[l];

    for (i = 0, j = l - 1; i < j; i++, j--) {
      SCFLOAT tmp = a[i] + k[l] * a[j];
      a[j] = a[j] + k[l] * a[i];
      a[i] = tmp;
    }
    if (i == j) {
      a[i] = a[i] + k[l] * a[i];
    }
  }

  for (i = M; i > 0; i--) {
    a[i] = -a[i - 1];
  }
  a[0] = 1.0;

  return;
}

void poly2rc(SCFLOAT *a, SCFLOAT *k, int M) {
  SCFLOAT *aTmp = k;
  int l, i, j;

  for (i = 0; i < M - 1; i++) {
    aTmp[i] = -a[i + 1] / a[0];
  }

  for (l = M - 2; l >= 0; l--) {
    SCFLOAT tmp;

    k[l] = aTmp[l];
    tmp = (SCFLOAT)(1.0 - (k[l] * k[l]));

    for (i = 0, j = l - 1; i < j; i++, j--) {
      SCFLOAT tmpA, tmpB;

      tmpA = (aTmp[i] + (k[l] * aTmp[l - 1 - i])) / tmp;
      tmpB = (aTmp[j] + (k[l] * aTmp[l - 1 - j])) / tmp;
      aTmp[i] = tmpA;
      aTmp[j] = tmpB;
    }

    if (i == j) {
      aTmp[i] = (aTmp[i] + (k[l] * aTmp[l - 1 - i])) / tmp;
    }
  }

  for (i = 0; i < M - 1; i++) {
    aTmp[i] *= -1.0;
  }

  return;
}

void lpc2cep(SCFLOAT *a, SCFLOAT *c, int len) {
  int i, m, n;
  SCFLOAT sum = 0;
  SCFLOAT norm = a[0];

  c[0] = (SCFLOAT)(-log(a[0]));

  for (i = 1; i < len; i++) {
    c[i] = (SCFLOAT)0.0;
  }

  for (i = 0; i < len; i++) {
    a[i] = a[i] / norm;
  }

  for (n = 1; n < len; n++) {
    sum = (SCFLOAT)0.0;

    for (m = 1; m <= n; m++) {
      sum += (n - m) * a[m] * c[(n - m)];
    }

    c[n] = -(a[n] + sum / n);
  }

  return;
}

void STATCLASS_autocorrPlus(
    SCFLOAT *x,
    SCFLOAT *r,
    int m,
    int n,
    SCFLOAT *fh) {
  SCFLOAT t[L_WINDOW_PLUS];
  SCFLOAT s;
  int i, j;
  if (n <= 0) {
    return;
  }
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

  return;
}

void STATCLASS_cos_window(SCFLOAT *fh, int n1, int n2) {
  SCFLOAT cc, cte;
  int i;

  cte = (SCFLOAT)(0.25 * PI2 / (SCFLOAT)n1);
  cc = (SCFLOAT)(0.5 * cte - 0.25 * PI2);

  for (i = 0; i < n1; i++) {
    *fh++ = (SCFLOAT)cos(cc);
    cc += cte;
  }

  cte = (SCFLOAT)(0.25 * PI2 / (SCFLOAT)n2);
  cc = (SCFLOAT)(0.5 * cte);

  for (i = 0; i < n2; i++) {
    *fh++ = (SCFLOAT)cos(cc);
    cc += cte;
  }

  return;
}

void STATCLASS_lag_wind(
    SCFLOAT r[],
    int m) {
  int i;
  for (i = 0; i <= m; i++) {
    r[i] *= STATCLASS_lag_window[i];
  }
  return;
}

void STATCLASS_lev_dur(SCFLOAT *a, SCFLOAT *r, int m) {
  SCFLOAT buf[ORDER];
  SCFLOAT *rc;
  SCFLOAT s, at, err;
  int i, j, l;
  rc = &buf[0];
  rc[0] = (-r[1]) / r[0];
  a[0] = 1.0F;
  a[1] = rc[0];
  err = r[0] + r[1] * rc[0];
  for (i = 2; i <= m; i++) {
    s = 0.0F;
    for (j = 0; j < i; j++) {
      s += r[i - j] * a[j];
    }
    rc[i - 1] = (-s) / (err);
    for (j = 1; j <= (i >> 1); j++) {
      l = i - j;
      at = a[j] + rc[i - 1] * a[l];
      a[l] += rc[i - 1] * a[j];
      a[j] = at;
    }
    a[i] = rc[i - 1];
    err += rc[i - 1] * s;
    if (err <= 0.0F) {
      err = 0.01F;
    }
  }
  return;
}

void STATCLASS_a_isp_conversion(SCFLOAT *a, SCFLOAT *isp, SCFLOAT *old_isp,
                                int m) {
  SCFLOAT f1[(ORDER >> 1) + 1], f2[ORDER >> 1];
  SCFLOAT *pf;
  SCFLOAT xlow, ylow, xhigh, yhigh, xmid, ymid, xint;
  int j, i, nf, ip, order, nc;
  nc = m >> 1;

  for (i = 0; i < nc; i++) {
    f1[i] = a[i] + a[m - i];
    f2[i] = a[i] - a[m - i];
  }
  f1[nc] = 2.0F * a[nc];

  for (i = 2; i < nc; i++) {
    f2[i] += f2[i - 2];
  }

  nf = 0;
  ip = 0;
  pf = f1;
  order = nc;
  xlow = STATCLASS_E_ROM_Grid[0];
  ylow = STATCLASS_chebyshev(xlow, pf, nc);
  j = 0;
  while ((nf < m - 1) && (j < NO_POINTS)) {
    j++;
    xhigh = xlow;
    yhigh = ylow;
    xlow = STATCLASS_E_ROM_Grid[j];
    ylow = STATCLASS_chebyshev(xlow, pf, order);
    if (ylow * yhigh <= 0.0F) {
      j--;

      for (i = 0; i < NO_ITER; i++) {
        xmid = 0.5F * (xlow + xhigh);
        ymid = STATCLASS_chebyshev(xmid, pf, order);
        if (ylow * ymid <= 0.0F) {
          yhigh = ymid;
          xhigh = xmid;
        } else {
          ylow = ymid;
          xlow = xmid;
        }
      }

      xint = xlow - ylow * (xhigh - xlow) / (yhigh - ylow);
      isp[nf] = xint;
      nf++;
      ip = 1 - ip;
      pf = ip ? f2 : f1;
      order = ip ? (nc - 1) : nc;
      xlow = xint;
      ylow = STATCLASS_chebyshev(xlow, pf, order);
    }
  }
  isp[m - 1] = a[m];

  if (nf < m - 1) {
    for (i = 0; i < m; i++) {
      isp[i] = old_isp[i];
    }
  }
  return;
}

SCFLOAT STATCLASS_chebyshev(SCFLOAT x, SCFLOAT *f, int n) {
  SCFLOAT b1, b2, b0, x2;
  int i;

  x2 = 2.0F * x;
  b2 = f[0];
  b1 = x2 * b2 + f[1];
  for (i = 2; i < n; i++) {
    b0 = x2 * b1 - b2 + f[i];
    b2 = b1;
    b1 = b0;
  }
  return (x * b1 - b2 + 0.5F * f[n]);
}

void STATCLASS_int_lpc_np1(SCFLOAT isf_old[],
                           SCFLOAT isf_new[],
                           SCFLOAT a[],
                           int nb_subfr,
                           int m) {
  SCFLOAT isf[ORDER], *p_a, inc, fnew, fold;
  int i, k;

  inc = (SCFLOAT)1.0 / (SCFLOAT)nb_subfr;
  p_a = a;
  fnew = (SCFLOAT)0.0;

  for (k = 0; k < nb_subfr; k++) {
    fold = (SCFLOAT)1.0 - fnew;

    for (i = 0; i < m; i++) {
      isf[i] = (SCFLOAT)(isf_old[i] * fold + isf_new[i] * fnew);
    }
    fnew += inc;
    STATCLASS_f_isp_a_conversion(isf, &p_a[k * (ORDER + 1)], m);
  }

  STATCLASS_f_isp_a_conversion(isf_new, p_a, m);

  return;
}

void STATCLASS_f_isp_a_conversion(SCFLOAT *isp, SCFLOAT *a, int m) {
  SCFLOAT f1[(M16k / 2) + 1], f2[M16k / 2];
  int i, j, nc;
  nc = m / 2;

  STATCLASS_f_isp_pol_get(&isp[0], f1, nc);
  STATCLASS_f_isp_pol_get(&isp[1], f2, nc - 1);

  for (i = (nc - 1); i > 1; i--) {
    f2[i] -= f2[i - 2];
  }

  for (i = 0; i < nc; i++) {
    f1[i] *= (SCFLOAT)(1.0 + isp[m - 1]);
    f2[i] *= (SCFLOAT)(1.0 - isp[m - 1]);
  }

  a[0] = 1.0;
  for (i = 1, j = m - 1; i < nc; i++, j--) {
    a[i] = (SCFLOAT)(0.5 * (f1[i] + f2[i]));
    a[j] = (SCFLOAT)(0.5 * (f1[i] - f2[i]));
  }
  a[nc] = (SCFLOAT)(0.5 * f1[nc] * (1.0 + isp[m - 1]));
  a[m] = isp[m - 1];
  return;
}

void STATCLASS_f_isp_pol_get(SCFLOAT isp[], SCFLOAT f[], int n) {
  SCFLOAT b;
  int i, j;
  f[0] = 1;
  b = (SCFLOAT)(-2.0 * *isp);
  f[1] = b;
  for (i = 2; i <= n; i++) {
    isp += 2;
    b = (SCFLOAT)(-2.0 * *isp);
    f[i] = (SCFLOAT)(b * f[i - 1] + 2.0 * f[i - 2]);
    for (j = i - 1; j > 1; j--) {
      f[j] += b * f[j - 1] + f[j - 2];
    }
    f[1] += b;
  }
  return;
}

void STATCLASS_find_wsp(SCFLOAT A[],
                        SCFLOAT speech[],
                        SCFLOAT wsp[],
                        SCFLOAT *mem_wsp,
                        int lg) {
  int i_subfr;
  SCFLOAT *p_A, Ap[ORDER + 1];
  p_A = A;
  for (i_subfr = 0; i_subfr < lg; i_subfr += L_SUBFR) {
    STATCLASS_a_weight(p_A, Ap, GAMMA1, ORDER);

    STATCLASS_residu(Ap, &speech[i_subfr], &wsp[i_subfr], L_SUBFR);

    p_A += (ORDER + 1);
  }
  STATCLASS_deemph(wsp, TILT_FAC, lg, mem_wsp);
  return;
}

void STATCLASS_deemph(SCFLOAT *signal, SCFLOAT mu, int L, SCFLOAT *mem) {
  int i;
  signal[0] = signal[0] + mu * (*mem);
  for (i = 1; i < L; i++) {
    signal[i] = signal[i] + mu * signal[i - 1];
  }
  *mem = signal[L - 1];
  if ((*mem < 1e-10) & (*mem > -1e-10)) {
    *mem = 0;
  }
  return;
}

void STATCLASS_a_weight(SCFLOAT *a, SCFLOAT *ap, SCFLOAT gamma, int m) {
  SCFLOAT f;
  int i;
  ap[0] = a[0];
  f = gamma;
  for (i = 1; i <= m; i++) {
    ap[i] = f * a[i];
    f *= gamma;
  }
  return;
}

void STATCLASS_residu(SCFLOAT *a, SCFLOAT *x, SCFLOAT *y, int l) {
  SCFLOAT s;
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
  return;
}

void STATCLASS_a_lsp_conversion(
    SCFLOAT *a,
    SCFLOAT *lsf,
    SCFLOAT *old_lsf) {
  SCFLOAT f1[NC + 1], f2[NC + 1];
  SCFLOAT *pf1;
  int j, i, nf, ip;
  SCFLOAT xlow, ylow, xhigh, yhigh, xmid, ymid, xint, isp[16];

  f1[0] = 1.0;
  f2[0] = 1.0;

  for (i = 1, j = ORDER; i <= NC; i++, j--) {
    f1[i] = a[i] + a[j] - f1[i - 1];
    f2[i] = a[i] - a[j] + f2[i - 1];
  }

  nf = 0;
  ip = 0;

  pf1 = f1;

  xlow = STATCLASS_E_ROM_Grid[0];
  ylow = STATCLASS_chebyshev(xlow, pf1, NC);

  j = 0;
  while ((nf < ORDER) && (j < NO_POINTS)) {
    j++;
    xhigh = xlow;
    yhigh = ylow;
    xlow = STATCLASS_E_ROM_Grid[j];
    ylow = STATCLASS_chebyshev(xlow, pf1, NC);

    if (ylow * yhigh <= 0.0) {
      j--;

      for (i = 0; i < NO_ITER; i++) {
        xmid = (SCFLOAT)0.5 * (xlow + xhigh);
        ymid = STATCLASS_chebyshev(xmid, pf1, NC);

        if (ylow * ymid <= 0.0) {
          yhigh = ymid;
          xhigh = xmid;
        } else {
          ylow = ymid;
          xlow = xmid;
        }
      }

      xint = xlow - ylow * (xhigh - xlow) / (yhigh - ylow);

      lsf[nf] = xint;
      nf++;

      ip = 1 - ip;
      pf1 = ip ? f2 : f1;

      xlow = xint;
      ylow = STATCLASS_chebyshev(xlow, pf1, NC);
    }
  }

  if (nf < ORDER) {
    for (i = 0; i < ORDER; i++) {
      lsf[i] = old_lsf[i];
    }
  }

  STATCLASS_lsp_lsf_conversion(lsf, isp, 16);
  memcpy(lsf, isp, 16 * sizeof(SCFLOAT));

  return;
}

void STATCLASS_lsp_lsf_conversion(SCFLOAT isp[], SCFLOAT isf[], long m) {
  short i;

  for (i = 0; i < m; i++) {
    isf[i] = (float)(acos(isp[i]) * SCALE1);
  }

  return;
}
