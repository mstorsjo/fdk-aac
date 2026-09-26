
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
#include "iisLPDComLib_constants.h"
#include "iisLPDComLib_tools.h"
#include "iisLPDComLib_lpc.h"
#include "iisLPDEncLib_acelp_AdaptiveCB.h"

#define M 16
#define L_FRAME 256
#define L_SUBFR 64
#define HP_ORDER 3
#define L_INTERPOL1 4
#define L_INTERPOL2 16
#define PIT_MIN 34
#define UP_SAMP 4

static const float lpdenc_TableCorrWeight[10 + 64 + 199 + 64 + 10] = {
    0.27886460203638F, 0.27953844463414F, 0.28021228723190F, 0.28088612982966F,
    0.28155997242742F, 0.28223381502518F, 0.28290765762294F, 0.28358150022070F,
    0.28425534281846F, 0.28492918541622F,
    0.28560302801398F, 0.28627687061174F, 0.28695071320950F, 0.28762455580726F,
    0.28829839840502F, 0.28897224100278F, 0.28964608360054F, 0.29031992619830F,
    0.29099376879606F, 0.29166761139382F, 0.29234145399158F, 0.29301529658934F,
    0.29368913918710F, 0.29436298178486F, 0.29503682438262F, 0.29571066698038F,
    0.29638450957814F, 0.29705835217590F, 0.29773219477366F, 0.29840603737142F,
    0.29907987996918F, 0.29975372256694F, 0.30042756516470F, 0.30110140776246F,
    0.30177525036022F, 0.30244909295798F, 0.30312293555574F, 0.30379677815350F,
    0.30447062075126F, 0.30514446334902F, 0.30581830594678F, 0.30649214854454F,
    0.30716599114230F, 0.30783983374006F, 0.30851367633782F, 0.30918751893558F,
    0.30986136153334F, 0.31053520413110F, 0.31120904672886F, 0.31188288932662F,
    0.31255673192438F, 0.31323057452214F, 0.31390441711990F, 0.31457825971766F,
    0.31525210231542F, 0.31592594491318F, 0.31659978751094F, 0.31727363010870F,
    0.31794747270646F, 0.31862131530422F, 0.31929515790198F, 0.31996900049974F,
    0.32064284309750F, 0.32131668569526F, 0.32199052829302F, 0.32266437089078F,
    0.32333821348854F, 0.32401205608630F, 0.32468589868406F, 0.32535974128182F,
    0.32603358387958F, 0.32670742647734F, 0.32738126907510F, 0.32805511167286F,
    0.32872895427062F, 0.32940279686838F, 0.33008397607000F, 0.33077263774882F,
    0.33146893201434F, 0.33217301337469F, 0.33288504090694F, 0.33360517843561F,
    0.33433359471985F, 0.33507046364991F, 0.33581596445331F, 0.33657028191132F,
    0.33733360658650F, 0.33810613506181F, 0.33888807019203F, 0.33967962136837F,
    0.34048100479698F, 0.34129244379225F, 0.34211416908597F, 0.34294641915312F,
    0.34378944055574F, 0.34464348830569F, 0.34550882624786F, 0.34638572746505F,
    0.34727447470606F, 0.34817536083854F, 0.34908868932841F, 0.35001477474762F,
    0.35095394331240F, 0.35190653345404F, 0.35287289642476F, 0.35385339694105F,
    0.35484841386753F, 0.35585834094415F, 0.35688358756025F, 0.35792457957894F,
    0.35898176021598F, 0.36005559097716F, 0.36114655265930F, 0.36225514641972F,
    0.36338189491998F, 0.36452734355021F, 0.36569206174066F, 0.36687664436818F,
    0.36808171326584F, 0.36930791884477F, 0.37055594183846F, 0.37182649518049F,
    0.37312032602824F, 0.37443821794611F, 0.37578099326358F, 0.37714951562500F,
    0.37854469274995F, 0.37996747942529F, 0.38141888075233F, 0.38289995567554F,
    0.38441182082240F, 0.38595565468744F, 0.38753270219810F, 0.38914427970450F,
    0.39079178044081F, 0.39247668051248F, 0.39420054547049F, 0.39596503754272F,
    0.39777192360213F, 0.39962308396279F, 0.40152052210853F, 0.40346637547426F,
    0.40546292741845F, 0.40751262054712F, 0.40961807157476F, 0.41178208793838F,
    0.41400768641652F, 0.41629811404829F, 0.41865687169900F, 0.42108774068111F,
    0.42359481291430F, 0.42618252519967F, 0.42885569829436F, 0.43161958160963F,
    0.43447990452326F, 0.43744293550645F, 0.44051555052483F, 0.44370531250000F,
    0.44702056402976F, 0.45047053608887F, 0.45406547610287F, 0.45781679965235F,
    0.46173727119116F, 0.46584122063850F, 0.47014480466210F, 0.47466632408736F,
    0.47942661240838F, 0.48444951522162F, 0.49000000000000F, 0.50000000000000F,
    0.50000000000000F, 0.50000000000000F, 0.50000000000000F, 0.50000000000000F,
    0.50000000000000F, 0.50000000000000F, 0.49000000000000F, 0.48444951522162F,
    0.47942661240838F, 0.47466632408736F, 0.47014480466210F, 0.46584122063850F,
    0.46173727119116F, 0.45781679965235F, 0.45406547610287F, 0.45047053608887F,
    0.44702056402976F, 0.44370531250000F, 0.44051555052483F, 0.43744293550645F,
    0.43447990452326F, 0.43161958160963F, 0.42885569829436F, 0.42618252519967F,
    0.42359481291430F, 0.42108774068111F, 0.41865687169900F, 0.41629811404829F,
    0.41400768641652F, 0.41178208793838F, 0.40961807157476F, 0.40751262054712F,
    0.40546292741845F, 0.40346637547426F, 0.40152052210853F, 0.39962308396279F,
    0.39777192360213F, 0.39596503754272F, 0.39420054547049F, 0.39247668051248F,
    0.39079178044081F, 0.38914427970450F, 0.38753270219810F, 0.38595565468744F,
    0.38441182082240F, 0.38289995567554F, 0.38141888075233F, 0.37996747942529F,
    0.37854469274995F, 0.37714951562500F, 0.37578099326358F, 0.37443821794611F,
    0.37312032602824F, 0.37182649518049F, 0.37055594183846F, 0.36930791884477F,
    0.36808171326584F, 0.36687664436818F, 0.36569206174066F, 0.36452734355021F,
    0.36338189491998F, 0.36225514641972F, 0.36114655265930F, 0.36005559097716F,
    0.35898176021598F, 0.35792457957894F, 0.35688358756025F, 0.35585834094415F,
    0.35484841386753F, 0.35385339694105F, 0.35287289642476F, 0.35190653345404F,
    0.35095394331240F, 0.35001477474762F, 0.34908868932841F, 0.34817536083854F,
    0.34727447470606F, 0.34638572746505F, 0.34550882624786F, 0.34464348830569F,
    0.34378944055574F, 0.34294641915312F, 0.34211416908597F, 0.34129244379225F,
    0.34048100479698F, 0.33967962136837F, 0.33888807019203F, 0.33810613506181F,
    0.33733360658650F, 0.33657028191132F, 0.33581596445331F, 0.33507046364991F,
    0.33433359471985F, 0.33360517843561F, 0.33288504090694F, 0.33217301337469F,
    0.33146893201434F, 0.33077263774882F, 0.33008397607000F, 0.32940279686838F,
    0.32872895427062F, 0.32806230648432F, 0.32740271564992F, 0.32805511167286F, 0.32738126907510F, 0.32670742647734F, 0.32603358387958F,
    0.32535974128182F, 0.32468589868406F, 0.32401205608630F, 0.32333821348854F,
    0.32266437089078F, 0.32199052829302F, 0.32131668569526F, 0.32064284309750F,
    0.31996900049974F, 0.31929515790198F, 0.31862131530422F, 0.31794747270646F,
    0.31727363010870F, 0.31659978751094F, 0.31592594491318F, 0.31525210231542F,
    0.31457825971766F, 0.31390441711990F, 0.31323057452214F, 0.31255673192438F,
    0.31188288932662F, 0.31120904672886F, 0.31053520413110F, 0.30986136153334F,
    0.30918751893558F, 0.30851367633782F, 0.30783983374006F, 0.30716599114230F,
    0.30649214854454F, 0.30581830594678F, 0.30514446334902F, 0.30447062075126F,
    0.30379677815350F, 0.30312293555574F, 0.30244909295798F, 0.30177525036022F,
    0.30110140776246F, 0.30042756516470F, 0.29975372256694F, 0.29907987996918F,
    0.29840603737142F, 0.29773219477366F, 0.29705835217590F, 0.29638450957814F,
    0.29571066698038F, 0.29503682438262F, 0.29436298178486F, 0.29368913918710F,
    0.29301529658934F, 0.29234145399158F, 0.29166761139382F, 0.29099376879606F,
    0.29031992619830F, 0.28964608360054F, 0.28897224100278F, 0.28829839840502F,
    0.28762455580726F, 0.28695071320950F, 0.28627687061174F, 0.28560302801398F,
    0.28492918541622F, 0.28425534281846F, 0.28358150022070F, 0.28290765762294F,
    0.28223381502518F, 0.28155997242742F, 0.28088612982966F, 0.28021228723190F,
    0.27953844463414F, 0.27886460203638F};

const float lpdenc_TableInterpolate4_1[UP_SAMP * L_INTERPOL1 + 1] = {

    0.900000F,
    0.818959F, 0.604850F, 0.331379F, 0.083958F,
    -0.075795F, -0.130717F, -0.105685F, -0.046774F,
    0.004467F, 0.027789F, 0.025642F, 0.012571F,
    0.001927F, -0.001571F, -0.000753F, 0.000000f};

static void sortPitches(int n,
                        int *ra) {
  int l, j, ir, i, rra;
  l = (n >> 1) + 1;
  ir = n;
  for (;;) {
    if (l > 1) {
      rra = ra[--l];
    } else {
      rra = ra[ir];
      ra[ir] = ra[1];
      if (--ir == 1) {
        ra[1] = rra;
        return;
      }
    }
    i = l;
    j = l << 1;
    while (j <= ir) {
      if (j < ir && ra[j] < ra[j + 1]) {
        ++j;
      }
      if (rra < ra[j]) {
        ra[i] = ra[j];
        j += (i = j);
      } else {
        j = ir + 1;
      }
    }
    ra[i] = rra;
  }
}

static void findNormalizedCorrelation(float exc[],
                                      float xn[],
                                      float h[],
                                      int t_min,
                                      int t_max,
                                      int offset,
                                      float corr_norm[]) {
  float excf[L_SUBFR];
  float alp, ps, norm;
  int t, j, k;

  k = -(t_min + offset);

  LPDCom_tools_convolve(&exc[k], h, excf);

  for (t = t_min; t <= t_max; t++) {
    ps = 0.0F;
    alp = 0.01F;
    for (j = 0; j < L_SUBFR; j++) {
      ps += xn[j] * excf[j];
      alp += excf[j] * excf[j];
    }

    norm = (float)(1.0F / sqrt(alp));

    corr_norm[t] = ps * norm;

    if (t != t_max) {
      k--;
      for (j = L_SUBFR - 1; j > 0; j--) {
        excf[j] = excf[j - 1] + exc[k] * h[j];
      }
      excf[0] = exc[k];
    }
  }
  return;
}

static float interpolateNormalizedCorrelation(float *x,
                                              int frac) {
  float s, *x1, *x2;
  const float *c1, *c2;
  if (frac < 0) {
    frac += 4;
    x--;
  }
  x1 = &x[0];
  x2 = &x[1];
  c1 = &lpdenc_TableInterpolate4_1[frac];
  c2 = &lpdenc_TableInterpolate4_1[4 - frac];
  s = x1[0] * c1[0] + x2[0] * c2[0];
  s += x1[-1] * c1[4] + x2[1] * c2[4];
  s += x1[-2] * c1[8] + x2[2] * c2[8];
  s += x1[-3] * c1[12] + x2[3] * c2[12];
  return s;
}

int LPDEnc_acelp_FindPitchOpenLoop(float *wsp,
                                   int L_min,
                                   int L_max,
                                   int nFrame,
                                   int L_0,
                                   float *gain,
                                   float *hp_wsp_mem,
                                   float hp_old_wsp[],
                                   int weight_flg) {
  int i, j, k, L = 0;
  float o, R0, R1, R2, R0_max = -1.0e23f;
  const float *ww, *we;
  float *data_a, *data_b, *hp_wsp, *p, *p1;

  ww = &lpdenc_TableCorrWeight[10 + 64 + 198];
  we = &lpdenc_TableCorrWeight[10 + 64 + 98 + L_max - L_0];

  for (i = L_max; i > L_min; i--) {
    p = &wsp[0];
    p1 = &wsp[-i];

    R0 = 0.0;
    for (j = 0; j < nFrame; j += 2) {
      R0 += p[j] * p1[j];
      R0 += p[j + 1] * p1[j + 1];
    }

    R0 *= *ww--;

    if ((L_0 > 0) && (weight_flg == 1)) {
      R0 *= *we--;
    }

    if (R0 >= R0_max) {
      R0_max = R0;
      L = i;
    }
  }
  data_a = hp_wsp_mem;
  data_b = hp_wsp_mem + HP_ORDER;
  hp_wsp = hp_old_wsp + L_max;
  for (k = 0; k < nFrame; k++) {
    data_b[0] = data_b[1];
    data_b[1] = data_b[2];
    data_b[2] = data_b[3];
    data_b[HP_ORDER] = wsp[k];
    o = data_b[0] * 0.83787057505665F;
    o += data_b[1] * -2.50975570071058F;
    o += data_b[2] * 2.50975570071058F;
    o += data_b[3] * -0.83787057505665F;
    o -= data_a[0] * -2.64436711600664F;
    o -= data_a[1] * 2.35087386625360F;
    o -= data_a[2] * -0.70001156927424F;
    data_a[2] = data_a[1];
    data_a[1] = data_a[0];
    data_a[0] = o;
    hp_wsp[k] = o;
  }

  p = &hp_wsp[0];
  p1 = &hp_wsp[-L];
  R0 = 0.0F;
  R1 = 0.0F;
  R2 = 0.0F;

  for (j = 0; j < nFrame; j++) {
    R1 += p1[j] * p1[j];
    R2 += p[j] * p[j];
    R0 += p[j] * p1[j];
  }

  *gain = (float)(R0 / (sqrt(R1 * R2) + 1e-5));

  memmove(hp_old_wsp, &hp_old_wsp[nFrame], L_max * sizeof(float));

  return (L);
}

int LPDEnc_acelp_GetMedianPitch(int prev_ol_lag,
                                int old_ol_lag[5]) {
  int tmp[6] = {0};
  int i;

  for (i = 4; i > 0; i--) {
    old_ol_lag[i] = old_ol_lag[i - 1];
  }
  old_ol_lag[0] = prev_ol_lag;
  for (i = 0; i < 5; i++) {
    tmp[i + 1] = old_ol_lag[i];
  }
  sortPitches(5, tmp);
  return tmp[3];
}

int LPDEnc_acelp_FindPitchClosedLoop(float exc[],
                                     float xn[],
                                     float h[],
                                     int t0_min,
                                     int t0_max,
                                     int *pit_frac,
                                     int i_subfr,
                                     int t0_fr2,
                                     int t0_fr1) {
  float corr_v[15 + 2 * L_INTERPOL1 + 1];
  float cor_max, max, temp;
  float *corr;
  int i, fraction, step;
  int t0, t_min, t_max;

  int offset = t0_min - L_INTERPOL1;

  t0_min -= offset;
  t0_max -= offset;
  t0_fr2 -= offset;
  t0_fr1 -= offset;

  t_min = t0_min - L_INTERPOL1;
  t_max = t0_max + L_INTERPOL1;

  corr = &corr_v[t_min];

  findNormalizedCorrelation(exc, xn, h, t_min, t_max, offset, corr);

  max = corr[t0_min];
  t0 = t0_min;
  for (i = t0_min + 1; i <= t0_max; i++) {
    if (corr[i] > max) {
      max = corr[i];
      t0 = i;
    }
  }

  if ((i_subfr == 0) & (t0 >= t0_fr1)) {
    *pit_frac = 0;
    return (t0 + offset);
  }

  step = 1;
  fraction = -3;
  if (((i_subfr == 0) & (t0 >= t0_fr2)) | (t0_fr2 == (PIT_MIN - offset))) {
    step = 2;
    fraction = -2;
  }
  if (t0 == t0_min) {
    fraction = 0;
  }
  cor_max = interpolateNormalizedCorrelation(&corr[t0], fraction);
  for (i = (fraction + step); i <= 3; i += step) {
    ;
    temp = interpolateNormalizedCorrelation(&corr[t0], i);
    if (temp > cor_max) {
      cor_max = temp;
      fraction = i;
    }
  }

  if (fraction < 0) {
    fraction += 4;
    t0 -= 1;
  }
  *pit_frac = fraction;
  return (t0 + offset);
}

void LPDEnc_acelp_Downsample(float x[],
                             int l,
                             float *mem) {
  float x_buf[L_FRAME + 3];
  float temp;
  int i, j;

  memcpy(x_buf, mem, 3 * sizeof(float));
  memcpy(&x_buf[3], x, l * sizeof(float));
  for (i = 0; i < 3; i++) {
    mem[i] =
        ((x[l - 3 + i] > 1e-10) | (x[l - 3 + i] < -1e-10)) ? x[l - 3 + i] : 0;
  }
  for (i = 0, j = 0; i < l; i += 2, j++) {
    temp = x_buf[i] * 0.13F;
    temp += x_buf[i + 1] * 0.23F;
    temp += x_buf[i + 2] * 0.28F;
    temp += x_buf[i + 3] * 0.23F;
    temp += x_buf[i + 4] * 0.13F;
    x[j] = temp;
  }
  return;
}

float LPDEnc_acelp_CalculateAdaptiveCBGain(float xn[],
                                           float y1[],
                                           float g_corr[]) {
  float gain;
  float t0, t1;
  short i;
  t0 = xn[0] * y1[0];
  t1 = y1[0] * y1[0];
  for (i = 1; i < L_SUBFR; i += 7) {
    t0 += xn[i] * y1[i];
    t1 += y1[i] * y1[i];
    t0 += xn[i + 1] * y1[i + 1];
    t1 += y1[i + 1] * y1[i + 1];
    t0 += xn[i + 2] * y1[i + 2];
    t1 += y1[i + 2] * y1[i + 2];
    t0 += xn[i + 3] * y1[i + 3];
    t1 += y1[i + 3] * y1[i + 3];
    t0 += xn[i + 4] * y1[i + 4];
    t1 += y1[i + 4] * y1[i + 4];
    t0 += xn[i + 5] * y1[i + 5];
    t1 += y1[i + 5] * y1[i + 5];
    t0 += xn[i + 6] * y1[i + 6];
    t1 += y1[i + 6] * y1[i + 6];
  }
  g_corr[0] = t1;
  g_corr[1] = -2.0F * t0 + 0.01F;

  if (t1 != 0.0f) {
    gain = t0 / t1;
  } else {
    gain = 1.0F;
  }
  if (gain < 0.0) {
    gain = 0.0;
  } else if (gain > 1.2F) {
    gain = 1.2F;
  }
  return gain;
}

void LPDEnc_acelp_FindWsp(float A[],
                          float speech[],
                          float wsp[],
                          float *mem_wsp,
                          int lg) {
  int i_subfr;
  float *p_A, Ap[M + 1];
  p_A = A;
  for (i_subfr = 0; i_subfr < lg; i_subfr += L_SUBFR) {
    LPDCom_lpc_AWeight(p_A, Ap, GAMMA1, M);
    LPDCom_lpc_Analyze(Ap, &speech[i_subfr], &wsp[i_subfr], L_SUBFR);
    p_A += (M + 1);
  }
  LPDCom_tools_Deemphasise(wsp, TILT_FAC, lg, mem_wsp);
  return;
}
