
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

#ifndef INCLUDED_FROM_IISFFT_C
#error "this file must not be included"
#endif

static void fft32(float* vec) {
  const float INV_SQRT2 = 7.071067811865475e-1f;
  const float COS_PI_DIV8 = 9.238795325112867e-1f;
  const float COS_3PI_DIV8 = 3.826834323650898e-1f;
  const float SQRT2PLUS1 = 2.414213562373095f;
  const float SQRT2MINUS1 = 4.142135623730952e-1f;

  const float c[4] = {9.807852804032304e-1f, 8.314696123025452e-1f, 5.555702330196023e-1f, 1.950903220161283e-1f};

  float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, temp10,
      temp11, temp12, temp13, temp14, temp15, temp16, temp17, temp18, temp19, temp110, temp111, temp112, temp113,
      temp114, temp115, temp20, temp21, temp22, temp23, temp24, temp25, temp26, temp27, temp28, temp29, temp210,
      temp211, temp212, temp213, temp214, temp215, temp30, temp31, temp32, temp33, temp34, temp35, temp36, temp37,
      temp38, temp39, temp310, temp311, temp312, temp313, temp314, temp315, temp316, temp317, temp318, temp319,
      temp320, temp321, temp322, temp323, temp324, temp325, temp326, temp327, temp328, temp329, temp330, temp331,
      temp40, temp41, temp42, temp43, temp44, temp45, temp46, temp47, temp48, temp49, temp410, temp411, temp412,
      temp413, temp414, temp415;

  temp20 = vec[2] - vec[34];
  temp21 = vec[3] - vec[35];
  temp30 = vec[0] + vec[32];
  temp31 = vec[1] + vec[33];
  temp32 = vec[2] + vec[34];
  temp33 = vec[3] + vec[35];

  temp22 = vec[6] - vec[38];
  temp23 = vec[7] - vec[39];
  temp34 = vec[4] + vec[36];
  temp35 = vec[5] + vec[37];
  temp36 = vec[6] + vec[38];
  temp37 = vec[7] + vec[39];

  temp24 = vec[10] - vec[42];
  temp25 = vec[11] - vec[43];
  temp38 = vec[8] + vec[40];
  temp39 = vec[9] + vec[41];
  temp310 = vec[10] + vec[42];
  temp311 = vec[11] + vec[43];

  temp26 = vec[14] - vec[46];
  temp27 = vec[15] - vec[47];
  temp312 = vec[12] + vec[44];
  temp313 = vec[13] + vec[45];
  temp314 = vec[14] + vec[46];
  temp315 = vec[15] + vec[47];

  temp28 = vec[18] - vec[50];
  temp29 = vec[19] - vec[51];
  temp316 = vec[16] + vec[48];
  temp317 = vec[17] + vec[49];
  temp318 = vec[18] + vec[50];
  temp319 = vec[19] + vec[51];

  temp210 = vec[22] - vec[54];
  temp211 = vec[23] - vec[55];
  temp320 = vec[20] + vec[52];
  temp321 = vec[21] + vec[53];
  temp322 = vec[22] + vec[54];
  temp323 = vec[23] + vec[55];

  temp212 = vec[26] - vec[58];
  temp213 = vec[27] - vec[59];
  temp324 = vec[24] + vec[56];
  temp325 = vec[25] + vec[57];
  temp326 = vec[26] + vec[58];
  temp327 = vec[27] + vec[59];

  temp214 = vec[30] - vec[62];
  temp215 = vec[31] - vec[63];
  temp328 = vec[28] + vec[60];
  temp329 = vec[29] + vec[61];
  temp330 = vec[30] + vec[62];
  temp331 = vec[31] + vec[63];

  temp41 = -(temp20 + temp214);
  temp42 = temp20 - temp214;
  temp40 = temp21 + temp215;
  temp43 = temp21 - temp215;
  temp45 = -(temp22 + temp212);
  temp46 = temp22 - temp212;
  temp44 = temp23 + temp213;
  temp47 = temp23 - temp213;
  temp49 = -(temp24 + temp210);
  temp410 = temp24 - temp210;
  temp48 = temp25 + temp211;
  temp411 = temp25 - temp211;
  temp413 = -(temp26 + temp28);
  temp414 = temp26 - temp28;
  temp412 = temp27 + temp29;
  temp415 = temp27 - temp29;

  temp20 = temp40 * c[3] + temp44 * c[2] + temp48 * c[1] + temp412 * c[0];
  temp24 = temp40 * c[2] + temp44 * c[0] + temp48 * c[3] - temp412 * c[1];
  temp28 = temp40 * c[1] + temp44 * c[3] - temp48 * c[0] + temp412 * c[2];
  temp212 = temp40 * c[0] - temp44 * c[1] + temp48 * c[2] - temp412 * c[3];
  temp21 = temp41 * c[3] + temp45 * c[2] + temp49 * c[1] + temp413 * c[0];
  temp25 = temp41 * c[2] + temp45 * c[0] + temp49 * c[3] - temp413 * c[1];
  temp29 = temp41 * c[1] + temp45 * c[3] - temp49 * c[0] + temp413 * c[2];
  temp213 = temp41 * c[0] - temp45 * c[1] + temp49 * c[2] - temp413 * c[3];
  temp22 = temp42 * c[0] + temp46 * c[1] + temp410 * c[2] + temp414 * c[3];
  temp26 = temp42 * c[1] - temp46 * c[3] - temp410 * c[0] - temp414 * c[2];
  temp210 = temp42 * c[2] - temp46 * c[0] + temp410 * c[3] + temp414 * c[1];
  temp214 = temp42 * c[3] - temp46 * c[2] + temp410 * c[1] - temp414 * c[0];
  temp23 = temp43 * c[0] + temp47 * c[1] + temp411 * c[2] + temp415 * c[3];
  temp27 = temp43 * c[1] - temp47 * c[3] - temp411 * c[0] - temp415 * c[2];
  temp211 = temp43 * c[2] - temp47 * c[0] + temp411 * c[3] + temp415 * c[1];
  temp215 = temp43 * c[3] - temp47 * c[2] + temp411 * c[1] - temp415 * c[0];

  temp40 = temp20 + temp22;
  temp414 = temp20 - temp22;
  temp41 = temp21 + temp23;
  temp415 = temp21 - temp23;
  temp42 = temp24 + temp26;
  temp412 = temp24 - temp26;
  temp43 = temp25 + temp27;
  temp413 = temp25 - temp27;
  temp44 = temp28 + temp210;
  temp410 = temp28 - temp210;
  temp45 = temp29 + temp211;
  temp411 = temp29 - temp211;
  temp46 = temp212 + temp214;
  temp48 = temp212 - temp214;
  temp47 = temp213 + temp215;
  temp49 = temp213 - temp215;

  temp10 = temp30 + temp316;
  temp11 = temp31 + temp317;
  temp12 = temp32 + temp318;
  temp13 = temp33 + temp319;
  temp14 = temp34 + temp320;
  temp15 = temp35 + temp321;
  temp16 = temp36 + temp322;
  temp17 = temp37 + temp323;
  temp18 = temp38 + temp324;
  temp19 = temp39 + temp325;
  temp110 = temp310 + temp326;
  temp111 = temp311 + temp327;
  temp112 = temp312 + temp328;
  temp113 = temp313 + temp329;
  temp114 = temp314 + temp330;
  temp115 = temp315 + temp331;

  tmp0 = temp10 + temp18;
  tmp2 = temp10 - temp18;
  tmp1 = temp11 + temp19;
  tmp3 = temp11 - temp19;
  tmp4 = temp12 + temp110;
  tmp6 = temp12 - temp110;
  tmp5 = temp13 + temp111;
  tmp7 = temp13 - temp111;
  tmp8 = temp14 + temp112;
  tmp10 = temp14 - temp112;
  tmp9 = temp15 + temp113;
  tmp11 = temp15 - temp113;
  tmp12 = temp16 + temp114;
  tmp14 = temp16 - temp114;
  tmp13 = temp17 + temp115;
  tmp15 = temp17 - temp115;

  temp20 = tmp0 + tmp8;
  temp24 = tmp0 - tmp8;
  temp21 = tmp1 + tmp9;
  temp25 = tmp1 - tmp9;
  temp28 = tmp2 - tmp11;
  temp210 = tmp2 + tmp11;
  temp29 = tmp3 + tmp10;
  temp211 = tmp3 - tmp10;
  temp22 = tmp4 + tmp12;
  temp27 = tmp4 - tmp12;
  temp23 = tmp5 + tmp13;
  temp26 = tmp13 - tmp5;

  tmp1 = tmp6 + tmp14;
  tmp2 = tmp6 - tmp14;
  tmp0 = tmp7 + tmp15;
  tmp3 = tmp7 - tmp15;
  temp212 = (tmp0 + tmp2) * INV_SQRT2;
  temp214 = (tmp0 - tmp2) * INV_SQRT2;
  temp213 = (tmp3 - tmp1) * INV_SQRT2;
  temp215 = (tmp1 + tmp3) * -INV_SQRT2;

  temp10 = temp30 - temp316;
  temp11 = temp31 - temp317;
  temp12 = temp32 - temp318;
  temp13 = temp33 - temp319;
  temp14 = temp34 - temp320;
  temp15 = temp35 - temp321;
  temp16 = temp36 - temp322;
  temp17 = temp37 - temp323;
  temp18 = temp38 - temp324;
  temp19 = temp39 - temp325;
  temp110 = temp310 - temp326;
  temp111 = temp311 - temp327;
  temp112 = temp312 - temp328;
  temp113 = temp313 - temp329;
  temp114 = temp314 - temp330;
  temp115 = temp315 - temp331;

  temp30 = temp20 + temp22;
  temp316 = temp20 - temp22;
  temp31 = temp21 + temp23;
  temp317 = temp21 - temp23;
  temp38 = temp24 - temp26;
  temp324 = temp24 + temp26;
  temp39 = temp25 - temp27;
  temp325 = temp25 + temp27;
  temp312 = temp28 + temp214;
  temp328 = temp28 - temp214;
  temp313 = temp29 + temp215;
  temp329 = temp29 - temp215;
  temp34 = temp210 + temp212;
  temp320 = temp210 - temp212;
  temp35 = temp211 + temp213;
  temp321 = temp211 - temp213;

  tmp9 = (temp12 + temp114) * -COS_3PI_DIV8;
  tmp10 = (temp12 - temp114) * COS_PI_DIV8;
  tmp8 = (temp13 + temp115) * COS_3PI_DIV8;
  tmp11 = (temp13 - temp115) * COS_PI_DIV8;
  tmp5 = (temp14 + temp112) * -INV_SQRT2;
  tmp6 = (temp14 - temp112) * INV_SQRT2;
  tmp4 = (temp15 + temp113) * INV_SQRT2;
  tmp7 = (temp15 - temp113) * INV_SQRT2;
  tmp13 = (temp16 + temp110) * -COS_PI_DIV8;
  tmp14 = (temp16 - temp110) * COS_3PI_DIV8;
  tmp12 = (temp17 + temp111) * COS_PI_DIV8;
  tmp15 = (temp17 - temp111) * COS_3PI_DIV8;

  temp12 = tmp8 * SQRT2PLUS1 - tmp12 * SQRT2MINUS1;
  temp13 = tmp9 * SQRT2PLUS1 - tmp13 * SQRT2MINUS1;
  temp14 = tmp10 * SQRT2MINUS1 - tmp14 * SQRT2PLUS1;
  temp15 = tmp11 * SQRT2MINUS1 - tmp15 * SQRT2PLUS1;

  tmp8 += tmp12;
  tmp9 += tmp13;
  tmp10 += tmp14;
  tmp11 += tmp15;
  temp16 = temp10 + tmp4;
  temp110 = temp10 - tmp4;
  temp17 = temp11 + tmp5;
  temp111 = temp11 - tmp5;

  temp112 = tmp6 - temp19;
  temp114 = tmp6 + temp19;
  temp113 = temp18 + tmp7;
  temp115 = temp18 - tmp7;

  tmp0 = temp16 - temp114;
  tmp2 = temp16 + temp114;
  tmp1 = temp17 + temp115;
  tmp3 = temp17 - temp115;
  tmp4 = temp110 + temp112;
  tmp6 = temp110 - temp112;
  tmp5 = temp111 + temp113;
  tmp7 = temp111 - temp113;

  temp110 = tmp8 + tmp10;
  tmp10 = tmp8 - tmp10;
  temp111 = tmp9 + tmp11;
  tmp11 = tmp9 - tmp11;

  tmp12 = temp12 + temp14;
  tmp14 = temp12 - temp14;
  tmp13 = temp13 + temp15;
  tmp15 = temp13 - temp15;

  temp32 = tmp2 + temp110;
  temp318 = tmp2 - temp110;
  temp33 = tmp3 + temp111;
  temp319 = tmp3 - temp111;
  temp36 = tmp0 + tmp12;
  temp322 = tmp0 - tmp12;
  temp37 = tmp1 + tmp13;
  temp323 = tmp1 - tmp13;
  temp314 = tmp4 + tmp10;
  temp330 = tmp4 - tmp10;
  temp315 = tmp5 + tmp11;
  temp331 = tmp5 - tmp11;
  temp310 = tmp6 + tmp14;
  temp326 = tmp6 - tmp14;
  temp311 = tmp7 + tmp15;
  temp327 = tmp7 - tmp15;

  temp10 = vec[0] - vec[32];
  temp11 = vec[1] - vec[33];
  temp12 = vec[4] - vec[36];
  temp13 = vec[5] - vec[37];
  temp14 = vec[8] - vec[40];
  temp15 = vec[9] - vec[41];
  temp16 = vec[12] - vec[44];
  temp17 = vec[13] - vec[45];
  temp18 = vec[16] - vec[48];
  temp19 = vec[17] - vec[49];
  temp110 = vec[20] - vec[52];
  temp111 = vec[21] - vec[53];
  temp112 = vec[24] - vec[56];
  temp113 = vec[25] - vec[57];
  temp114 = vec[28] - vec[60];
  temp115 = vec[29] - vec[61];

  tmp9 = (temp12 + temp114) * -COS_3PI_DIV8;
  tmp10 = (temp12 - temp114) * COS_PI_DIV8;
  tmp8 = (temp13 + temp115) * COS_3PI_DIV8;
  tmp11 = (temp13 - temp115) * COS_PI_DIV8;
  tmp5 = (temp14 + temp112) * -INV_SQRT2;
  tmp6 = (temp14 - temp112) * INV_SQRT2;
  tmp4 = (temp15 + temp113) * INV_SQRT2;
  tmp7 = (temp15 - temp113) * INV_SQRT2;
  tmp13 = (temp16 + temp110) * -COS_PI_DIV8;
  tmp14 = (temp16 - temp110) * COS_3PI_DIV8;
  tmp12 = (temp17 + temp111) * COS_PI_DIV8;
  tmp15 = (temp17 - temp111) * COS_3PI_DIV8;

  temp12 = tmp8 * SQRT2PLUS1 - tmp12 * SQRT2MINUS1;
  temp13 = tmp9 * SQRT2PLUS1 - tmp13 * SQRT2MINUS1;
  temp14 = tmp10 * SQRT2MINUS1 - tmp14 * SQRT2PLUS1;
  temp15 = tmp11 * SQRT2MINUS1 - tmp15 * SQRT2PLUS1;

  tmp8 += tmp12;
  tmp9 += tmp13;
  tmp10 += tmp14;
  tmp11 += tmp15;
  temp16 = temp10 + tmp4;
  temp110 = temp10 - tmp4;
  temp17 = temp11 + tmp5;
  temp111 = temp11 - tmp5;

  temp112 = tmp6 - temp19;
  temp114 = tmp6 + temp19;
  temp113 = temp18 + tmp7;
  temp115 = temp18 - tmp7;

  tmp0 = temp16 - temp114;
  tmp2 = temp16 + temp114;
  tmp1 = temp17 + temp115;
  tmp3 = temp17 - temp115;
  tmp4 = temp110 + temp112;
  tmp6 = temp110 - temp112;
  tmp5 = temp111 + temp113;
  tmp7 = temp111 - temp113;

  temp110 = tmp8 + tmp10;
  tmp10 = tmp8 - tmp10;
  temp111 = tmp9 + tmp11;
  tmp11 = tmp9 - tmp11;

  tmp12 = temp12 + temp14;
  tmp14 = temp12 - temp14;
  tmp13 = temp13 + temp15;
  tmp15 = temp13 - temp15;

  temp10 = tmp2 + temp110;
  temp18 = tmp2 - temp110;
  temp11 = tmp3 + temp111;
  temp19 = tmp3 - temp111;
  temp12 = tmp0 + tmp12;
  temp110 = tmp0 - tmp12;
  temp13 = tmp1 + tmp13;
  temp111 = tmp1 - tmp13;
  temp16 = tmp4 + tmp10;
  temp114 = tmp4 - tmp10;
  temp17 = tmp5 + tmp11;
  temp115 = tmp5 - tmp11;
  temp14 = tmp6 + tmp14;
  temp112 = tmp6 - tmp14;
  temp15 = tmp7 + tmp15;
  temp113 = tmp7 - tmp15;

  *vec++ = temp30;
  *vec++ = temp31;
  *vec++ = temp10 + temp40;
  *vec++ = temp11 + temp41;
  *vec++ = temp32;
  *vec++ = temp33;
  *vec++ = temp12 + temp42;
  *vec++ = temp13 + temp43;
  *vec++ = temp34;
  *vec++ = temp35;
  *vec++ = temp14 + temp44;
  *vec++ = temp15 + temp45;
  *vec++ = temp36;
  *vec++ = temp37;
  *vec++ = temp16 + temp46;
  *vec++ = temp17 + temp47;
  *vec++ = temp38;
  *vec++ = temp39;
  *vec++ = temp18 + temp48;
  *vec++ = temp19 + temp49;
  *vec++ = temp310;
  *vec++ = temp311;
  *vec++ = temp110 + temp410;
  *vec++ = temp111 + temp411;
  *vec++ = temp312;
  *vec++ = temp313;
  *vec++ = temp112 + temp412;
  *vec++ = temp113 + temp413;
  *vec++ = temp314;
  *vec++ = temp315;
  *vec++ = temp114 + temp414;
  *vec++ = temp115 + temp415;
  *vec++ = temp316;
  *vec++ = temp317;
  *vec++ = temp10 - temp40;
  *vec++ = temp11 - temp41;
  *vec++ = temp318;
  *vec++ = temp319;
  *vec++ = temp12 - temp42;
  *vec++ = temp13 - temp43;
  *vec++ = temp320;
  *vec++ = temp321;
  *vec++ = temp14 - temp44;
  *vec++ = temp15 - temp45;
  *vec++ = temp322;
  *vec++ = temp323;
  *vec++ = temp16 - temp46;
  *vec++ = temp17 - temp47;
  *vec++ = temp324;
  *vec++ = temp325;
  *vec++ = temp18 - temp48;
  *vec++ = temp19 - temp49;
  *vec++ = temp326;
  *vec++ = temp327;
  *vec++ = temp110 - temp410;
  *vec++ = temp111 - temp411;
  *vec++ = temp328;
  *vec++ = temp329;
  *vec++ = temp112 - temp412;
  *vec++ = temp113 - temp413;
  *vec++ = temp330;
  *vec++ = temp331;
  *vec++ = temp114 - temp414;
  *vec++ = temp115 - temp415;
}
