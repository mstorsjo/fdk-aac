
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

#include "iisLPDComLib_acelp_Util.h"

#define NB_QUA_GAIN7B 128
#define PIT_UP_SAMP 4
#define PIT_L_INTERPOL2 16
#define PIT_FIR_SIZE2 (PIT_UP_SAMP * PIT_L_INTERPOL2 + 1)
#define F_PIT_SHARP 0.85F
#define L_SUBFR 64

const int lpdcom_NBITS_FIXED[][4] = {
    {0, 0, 0, 0},
    {16, 16, 16, 16},
    {20, 20, 20, 20},
    {28, 28, 28, 28},
    {36, 36, 36, 36},
    {44, 44, 44, 44},
    {52, 52, 52, 52},
    {64, 64, 64, 64}};

const int lpdcom_NBITS_CORE_768[] = {
    0,
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 16) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 20) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 28) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 36) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 44) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 52) + (3 * 7) + 46),
    4 * (2 + 2 + (9 + 6 + 6) + 3 + (3 * 64) + (3 * 7) + 46)};

const int lpdcom_NBITS_CORE_1024[] = {

    (int)(0),
    (int)(8.80 * 80),
    (int)(9.60 * 80),
    (int)(11.20 * 80),
    (int)(12.80 * 80),
    (int)(14.40 * 80),
    (int)(16.00 * 80),
    (int)(18.40 * 80)};

const float lpdcom_qua_gain7b[NB_QUA_GAIN7B * 2] = {
    0.012445F, 0.215546F,
    0.028326F, 0.965442F,
    0.053042F, 0.525819F,
    0.065409F, 1.495322F,
    0.078212F, 2.323725F,
    0.100504F, 0.751276F,
    0.112617F, 3.427530F,
    0.113124F, 0.309583F,
    0.121763F, 1.140685F,
    0.143515F, 7.519609F,
    0.162430F, 0.568752F,
    0.164940F, 1.904113F,
    0.165429F, 4.947562F,
    0.194985F, 0.855463F,
    0.213527F, 1.281019F,
    0.223544F, 0.414672F,
    0.243135F, 2.781766F,
    0.257180F, 1.659565F,
    0.269488F, 0.636749F,
    0.286539F, 1.003938F,
    0.328124F, 2.225436F,
    0.328761F, 0.330278F,
    0.336807F, 11.500983F,
    0.339794F, 3.805726F,
    0.344454F, 1.494626F,
    0.346165F, 0.738748F,
    0.363605F, 1.141454F,
    0.398729F, 0.517614F,
    0.415276F, 2.928666F,
    0.416282F, 0.862935F,
    0.423421F, 1.873310F,
    0.444151F, 0.202244F,
    0.445842F, 1.301113F,
    0.455671F, 5.519512F,
    0.484764F, 0.387607F,
    0.488696F, 0.967884F,
    0.488730F, 0.666771F,
    0.508189F, 1.516224F,
    0.508792F, 2.348662F,
    0.531504F, 3.883870F,
    0.548649F, 1.112861F,
    0.551182F, 0.514986F,
    0.564397F, 1.742030F,
    0.566598F, 0.796454F,
    0.589255F, 3.081743F,
    0.598816F, 1.271936F,
    0.617654F, 0.333501F,
    0.619073F, 2.040522F,
    0.625282F, 0.950244F,
    0.630798F, 0.594883F,
    0.638918F, 4.863197F,
    0.650102F, 1.464846F,
    0.668412F, 0.747138F,
    0.669490F, 2.583027F,
    0.683757F, 1.125479F,
    0.691216F, 1.739274F,
    0.718441F, 3.297789F,
    0.722608F, 0.902743F,
    0.728827F, 2.194941F,
    0.729586F, 0.633849F,
    0.730907F, 7.432957F,
    0.731017F, 0.431076F,
    0.731543F, 1.387847F,
    0.759183F, 1.045210F,
    0.768606F, 1.789648F,
    0.771245F, 4.085637F,
    0.772613F, 0.778145F,
    0.786483F, 1.283204F,
    0.792467F, 2.412891F,
    0.802393F, 0.544588F,
    0.807156F, 0.255978F,
    0.814280F, 1.544409F,
    0.817839F, 0.938798F,
    0.826959F, 2.910633F,
    0.830453F, 0.684066F,
    0.833431F, 1.171532F,
    0.841208F, 1.908628F,
    0.846440F, 5.333522F,
    0.868280F, 0.841519F,
    0.868662F, 1.435230F,
    0.871449F, 3.675784F,
    0.881317F, 2.245058F,
    0.882020F, 0.480249F,
    0.882476F, 1.105804F,
    0.902856F, 0.684850F,
    0.904419F, 1.682113F,
    0.909384F, 2.787801F,
    0.916558F, 7.500981F,
    0.918444F, 0.950341F,
    0.919721F, 1.296319F,
    0.940272F, 4.682978F,
    0.940273F, 1.991736F,
    0.950291F, 3.507281F,
    0.957455F, 1.116284F,
    0.957723F, 0.793034F,
    0.958217F, 1.497824F,
    0.962628F, 2.514156F,
    0.968507F, 0.588605F,
    0.974739F, 0.339933F,
    0.991738F, 1.750201F,
    0.997210F, 0.936131F,
    1.002422F, 1.250008F,
    1.006040F, 2.167232F,
    1.008848F, 3.129940F,
    1.014404F, 5.842819F,
    1.027798F, 4.287319F,
    1.039404F, 1.489295F,
    1.039628F, 8.947958F,
    1.043214F, 0.765733F,
    1.045089F, 2.537806F,
    1.058994F, 1.031496F,
    1.060415F, 0.478612F,
    1.072132F, 12.8F,
    1.074778F, 1.910049F,
    1.076570F, 15.9999F,
    1.107853F, 3.843067F,
    1.110673F, 1.228576F,
    1.110969F, 2.758471F,
    1.140058F, 1.603077F,
    1.155384F, 0.668935F,
    1.176229F, 6.717108F,
    1.179008F, 2.011940F,
    1.187735F, 0.963552F,
    1.199569F, 4.891432F,
    1.206311F, 3.316329F,
    1.215323F, 2.507536F,
    1.223150F, 1.387102F,
    1.296012F, 9.684225F};

const float lpdcom_inter4_2[PIT_FIR_SIZE2] = {

    0.940000f,
    0.856390f, 0.632268f, 0.337560f, 0.059072f,
    -0.131059f, -0.199393f, -0.158569f, -0.056359f,
    0.047606f, 0.106749f, 0.103705f, 0.052062f,
    -0.015182f, -0.063705f, -0.073660f, -0.046497f,
    -0.000983f, 0.038227f, 0.053143f, 0.040059f,
    0.009308f, -0.021674f, -0.037767f, -0.033186f,
    -0.013028f, 0.010702f, 0.025901f, 0.026318f,
    0.013821f, -0.003645f, -0.016813f, -0.019855f,
    -0.012766f, -0.000530f, 0.010080f, 0.014122f,
    0.010657f, 0.002594f, -0.005363f, -0.009344f,
    -0.008101f, -0.003182f, 0.002330f, 0.005635f,
    0.005562f, 0.002844f, -0.000627f, -0.002993f,
    -0.003362f, -0.002044f, -0.000116f, 0.001315f,
    0.001692f, 0.001151f, 0.000259f, -0.000417f,
    -0.000618f, -0.000434f, -0.000133f, 0.000063f,
    0.000098f, 0.000048f, 0.000007f, 0.000000f};

void LPDCom_acelp_LTPSynthesis(
    float exc[],
    int T0,
    int frac,
    int L_subfr) {
  int i, j;
  float s, *x0, *x1, *x2;
  const float *c1, *c2;

  x0 = &exc[-T0];
  frac = -frac;
  if (frac < 0) {
    frac += PIT_UP_SAMP;
    x0--;
  }

  for (j = 0; j < L_subfr; j++) {
    x1 = x0++;
    x2 = x1 + 1;
    c1 = &lpdcom_inter4_2[frac];
    c2 = &lpdcom_inter4_2[PIT_UP_SAMP - frac];
    s = 0.0;

    for (i = 0; i < PIT_L_INTERPOL2; i++, c1 += PIT_UP_SAMP, c2 += PIT_UP_SAMP) {
      s += (*x1--) * (*c1) + (*x2++) * (*c2);
    }
    exc[j] = s;
  }
}

void LPDCom_acelp_PitchSharpening(float *x, int pit_lag) {
  int i;
  for (i = pit_lag; i < L_SUBFR; i++) {
    x[i] += x[i - pit_lag] * F_PIT_SHARP;
  }
}
