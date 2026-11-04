
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

static void fft15(float* vec) {
  float r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, i0, i1, i2, i3, i4, i5, i6,
      i7, i8, i9, i10, i11, i12, i13, i14, i15, i16, i17, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9,
      tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18, tmp19, tmp20, tmp21, tmp22, tmp23, tmp24, tmp25,
      tmp26, tmp27, tmp28, tmp29;

  r1 = vec[2] + vec[8];
  r2 = vec[2] - vec[8];
  r3 = vec[4] + vec[16];
  r4 = vec[4] - vec[16];
  r5 = vec[6] + vec[24];
  r6 = vec[6] - vec[24];
  r7 = vec[10] + vec[20];
  r8 = vec[10] - vec[20];
  r9 = vec[12] + vec[18];
  r10 = vec[12] - vec[18];
  r11 = vec[14] + vec[26];
  r12 = vec[14] - vec[26];
  r13 = vec[22] + vec[28];
  r14 = vec[22] - vec[28];

  tmp2 = r1 + r3;
  tmp4 = r1 - r3;
  tmp6 = r2 + r14;
  tmp8 = r2 - r14;
  tmp10 = r4 + r12;
  tmp12 = r4 - r12;
  tmp14 = r5 + r9;
  tmp16 = r5 - r9;
  tmp18 = r11 + r13;
  tmp20 = r11 - r13;

  i1 = vec[3] + vec[9];
  i2 = vec[3] - vec[9];
  i3 = vec[5] + vec[17];
  i4 = vec[5] - vec[17];
  i5 = vec[7] + vec[25];
  i6 = vec[7] - vec[25];
  i7 = vec[11] + vec[21];
  i8 = vec[11] - vec[21];
  i9 = vec[13] + vec[19];
  i10 = vec[13] - vec[19];
  i11 = vec[15] + vec[27];
  i12 = vec[15] - vec[27];
  i13 = vec[23] + vec[29];
  i14 = vec[23] - vec[29];

  tmp3 = i1 + i3;
  tmp5 = i1 - i3;
  tmp7 = i2 + i14;
  tmp9 = i2 - i14;
  tmp11 = i4 + i12;
  tmp13 = i4 - i12;
  tmp15 = i5 + i9;
  tmp17 = i5 - i9;
  tmp19 = i11 + i13;
  tmp21 = i11 - i13;

  tmp28 = tmp4 + tmp20;
  tmp29 = tmp5 + tmp21;
  r4 = tmp2 + tmp18;
  i4 = tmp3 + tmp19;
  r3 = (r4 + tmp14) * -1.25f;
  i3 = (i4 + tmp15) * -1.25f;
  r2 = (tmp29 - i8) * -8.660254037844387e-1f;
  i2 = (tmp28 - r8) * 8.660254037844387e-1f;
  r1 = r4 + r7;
  i1 = i4 + i7;
  r0 = r1 + vec[0] + tmp14;
  i0 = i1 + vec[1] + tmp15;
  r7 = tmp4 - tmp20;
  i7 = tmp5 - tmp21;
  r8 = (tmp3 - tmp19) * -4.841229182759272e-1f;
  i8 = (tmp2 - tmp18) * 4.841229182759272e-1f;
  tmp0 = tmp6 + r10;
  tmp1 = tmp7 + i10;
  tmp2 = r6 - tmp10;
  tmp3 = i6 - tmp11;
  r10 = tmp7 * -2.308262652881440f;
  i10 = tmp6 * 2.308262652881440f;
  r11 = tmp8 * 1.332676064001459f;
  i11 = tmp9 * 1.332676064001459f;
  r6 = (r7 - tmp16) * 5.590169943749475e-1f;
  i6 = (i7 - tmp17) * 5.590169943749475e-1f;
  r12 = (tmp1 + tmp3) * 5.877852522924733e-1f;
  i12 = (tmp0 + tmp2) * -5.877852522924733e-1f;
  r13 = (tmp7 - tmp11) * -8.816778784387098e-1f;
  i13 = (tmp6 - tmp10) * 8.816778784387098e-1f;
  r14 = (tmp8 + tmp12) * 5.090369604551274e-1f;
  i14 = (tmp9 + tmp13) * 5.090369604551274e-1f;
  r16 = tmp11 * 5.449068960040204e-1f;
  i16 = tmp10 * -5.449068960040204e-1f;
  r17 = tmp12 * 3.146021430912046e-1f;
  i17 = tmp13 * 3.146021430912046e-1f;

  r4 *= 1.875f;
  i4 *= 1.875f;
  r1 *= -1.5f;
  i1 *= -1.5f;
  r7 *= -8.385254915624212e-1f;
  i7 *= -8.385254915624212e-1f;
  r5 = tmp29 * 1.082531754730548f;
  i5 = tmp28 * -1.082531754730548f;
  r9 = tmp1 * 1.538841768587627f;
  i9 = tmp0 * -1.538841768587627f;
  r15 = tmp3 * 3.632712640026803e-1f;
  i15 = tmp2 * -3.632712640026803e-1f;

  tmp2 = r0 + r1;
  tmp4 = r3 + r6;
  tmp6 = r3 - r6;
  tmp8 = r4 + r5;
  tmp10 = r4 - r5;
  tmp12 = r7 + r8;
  tmp14 = r7 - r8;
  tmp16 = r13 + r16;
  tmp18 = r14 + r17;
  tmp20 = r10 - r13;
  tmp22 = r11 - r14;
  tmp24 = r12 + r15;
  tmp26 = r12 - r9;

  r1 = tmp2 + r2;
  r2 = tmp2 - r2;
  r3 = tmp4 + tmp26;
  r4 = tmp4 - tmp26;
  r5 = tmp6 + tmp24;
  r6 = tmp6 - tmp24;
  r7 = tmp16 + tmp18;
  r8 = tmp16 - tmp18;
  r9 = tmp20 - tmp22;
  r10 = tmp20 + tmp22;
  r11 = r1 + tmp8;
  r12 = r2 + tmp10;
  r13 = r11 - tmp12;
  r14 = r12 - tmp14;
  r15 = r12 + tmp14;
  r16 = r11 + tmp12;

  tmp3 = i0 + i1;
  tmp5 = i3 + i6;
  tmp7 = i3 - i6;
  tmp9 = i4 + i5;
  tmp11 = i4 - i5;
  tmp13 = i7 + i8;
  tmp15 = i7 - i8;
  tmp17 = i13 + i16;
  tmp19 = i14 + i17;
  tmp21 = i10 - i13;
  tmp23 = i11 - i14;
  tmp25 = i12 + i15;
  tmp27 = i12 - i9;

  i1 = tmp3 + i2;
  i2 = tmp3 - i2;
  i3 = tmp5 + tmp27;
  i4 = tmp5 - tmp27;
  i5 = tmp7 + tmp25;
  i6 = tmp7 - tmp25;
  i7 = tmp17 + tmp19;
  i8 = tmp17 - tmp19;
  i9 = tmp21 - tmp23;
  i10 = tmp21 + tmp23;
  i11 = i1 + tmp9;
  i12 = i2 + tmp11;
  i13 = i11 - tmp13;
  i14 = i12 - tmp15;
  i15 = i12 + tmp15;
  i16 = i11 + tmp13;

  *vec++ = r0;
  *vec++ = i0;
  *vec++ = r13 + r5 + r7;
  *vec++ = i13 + i5 + i7;
  *vec++ = r15 + r3 - r9;
  *vec++ = i15 + i3 - i9;
  *vec++ = r0 + r4;
  *vec++ = i0 + i4;
  *vec++ = r13 + r6 - r7;
  *vec++ = i13 + i6 - i7;
  *vec++ = r2;
  *vec++ = i2;
  *vec++ = r0 + r5;
  *vec++ = i0 + i5;
  *vec++ = r16 + r3 - r10;
  *vec++ = i16 + i3 - i10;
  *vec++ = r15 + r4 + r9;
  *vec++ = i15 + i4 + i9;
  *vec++ = r0 + r6;
  *vec++ = i0 + i6;
  *vec++ = r1;
  *vec++ = i1;
  *vec++ = r14 + r5 + r8;
  *vec++ = i14 + i5 + i8;
  *vec++ = r0 + r3;
  *vec++ = i0 + i3;
  *vec++ = r16 + r4 + r10;
  *vec++ = i16 + i4 + i10;
  *vec++ = r14 + r6 - r8;
  *vec++ = i14 + i6 - i8;
}

static void fft16(float* vec) {
  const float INV_SQRT2 = 7.071067811865475e-1f;
  const float COS_PI_DIV8 = 9.238795325112867e-1f;
  const float COS_3PI_DIV8 = 3.826834323650898e-1f;
  const float SQRT2PLUS1 = 2.414213562373095f;
  const float SQRT2MINUS1 = 4.142135623730952e-1f;

  float temp10, temp11, temp12, temp13, temp14, temp15, temp16, temp17, temp18, temp19, temp110, temp111, temp112,
      temp113, temp114, temp115, temp20, temp21, temp22, temp23, temp24, temp25, temp26, temp27, temp28, temp29,
      temp210, temp211, temp212, temp213, temp214, temp215, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8,
      vec9, vec10, vec11, vec12, vec13, vec14, vec15;

  vec0 = vec[0] + vec[16];
  vec1 = vec[1] + vec[17];
  vec2 = vec[2] + vec[18];
  vec3 = vec[3] + vec[19];
  vec4 = vec[4] + vec[20];
  vec5 = vec[5] + vec[21];
  vec6 = vec[6] + vec[22];
  vec7 = vec[7] + vec[23];
  vec8 = vec[8] + vec[24];
  vec9 = vec[9] + vec[25];
  vec10 = vec[10] + vec[26];
  vec11 = vec[11] + vec[27];
  vec12 = vec[12] + vec[28];
  vec13 = vec[13] + vec[29];
  vec14 = vec[14] + vec[30];
  vec15 = vec[15] + vec[31];

  temp10 = vec0 + vec8;
  temp12 = vec0 - vec8;
  temp11 = vec1 + vec9;
  temp13 = vec1 - vec9;
  temp14 = vec2 + vec10;
  temp16 = vec2 - vec10;
  temp15 = vec3 + vec11;
  temp17 = vec3 - vec11;
  temp18 = vec4 + vec12;
  temp110 = vec4 - vec12;
  temp19 = vec5 + vec13;
  temp111 = vec5 - vec13;
  temp112 = vec6 + vec14;
  temp114 = vec6 - vec14;
  temp113 = vec7 + vec15;
  temp115 = vec7 - vec15;

  temp20 = temp10 + temp18;
  temp24 = temp10 - temp18;
  temp21 = temp11 + temp19;
  temp25 = temp11 - temp19;
  temp28 = temp12 - temp111;
  temp210 = temp12 + temp111;
  temp29 = temp13 + temp110;
  temp211 = temp13 - temp110;
  temp22 = temp14 + temp112;
  temp27 = temp14 - temp112;
  temp23 = temp15 + temp113;
  temp26 = temp113 - temp15;

  temp11 = temp16 + temp114;
  temp12 = temp16 - temp114;
  temp10 = temp17 + temp115;
  temp13 = temp17 - temp115;
  temp212 = (temp10 + temp12) * INV_SQRT2;
  temp214 = (temp10 - temp12) * INV_SQRT2;
  temp213 = (temp13 - temp11) * INV_SQRT2;
  temp215 = (temp11 + temp13) * -INV_SQRT2;

  vec0 = vec[0] - vec[16];
  vec1 = vec[1] - vec[17];
  vec2 = vec[2] - vec[18];
  vec3 = vec[3] - vec[19];
  vec4 = vec[4] - vec[20];
  vec5 = vec[5] - vec[21];
  vec6 = vec[6] - vec[22];
  vec7 = vec[7] - vec[23];
  vec8 = vec[8] - vec[24];
  vec9 = vec[9] - vec[25];
  vec10 = vec[10] - vec[26];
  vec11 = vec[11] - vec[27];
  vec12 = vec[12] - vec[28];
  vec13 = vec[13] - vec[29];
  vec14 = vec[14] - vec[30];
  vec15 = vec[15] - vec[31];

  temp19 = (vec2 + vec14) * -COS_3PI_DIV8;
  temp110 = (vec2 - vec14) * COS_PI_DIV8;
  temp18 = (vec3 + vec15) * COS_3PI_DIV8;
  temp111 = (vec3 - vec15) * COS_PI_DIV8;
  temp15 = (vec4 + vec12) * -INV_SQRT2;
  temp16 = (vec4 - vec12) * INV_SQRT2;
  temp14 = (vec5 + vec13) * INV_SQRT2;
  temp17 = (vec5 - vec13) * INV_SQRT2;
  temp113 = (vec6 + vec10) * -COS_PI_DIV8;
  temp114 = (vec6 - vec10) * COS_3PI_DIV8;
  temp112 = (vec7 + vec11) * COS_PI_DIV8;
  temp115 = (vec7 - vec11) * COS_3PI_DIV8;

  vec2 = temp18 * SQRT2PLUS1 - temp112 * SQRT2MINUS1;
  vec3 = temp19 * SQRT2PLUS1 - temp113 * SQRT2MINUS1;
  vec4 = temp110 * SQRT2MINUS1 - temp114 * SQRT2PLUS1;
  vec5 = temp111 * SQRT2MINUS1 - temp115 * SQRT2PLUS1;

  temp18 += temp112;
  temp19 += temp113;
  temp110 += temp114;
  temp111 += temp115;
  vec6 = vec0 + temp14;
  vec10 = vec0 - temp14;
  vec7 = vec1 + temp15;
  vec11 = vec1 - temp15;

  vec12 = temp16 - vec9;
  vec14 = temp16 + vec9;
  vec13 = vec8 + temp17;
  vec15 = vec8 - temp17;

  temp10 = vec6 - vec14;
  temp12 = vec6 + vec14;
  temp11 = vec7 + vec15;
  temp13 = vec7 - vec15;
  temp14 = vec10 + vec12;
  temp16 = vec10 - vec12;
  temp15 = vec11 + vec13;
  temp17 = vec11 - vec13;

  vec10 = temp18 + temp110;
  temp110 = temp18 - temp110;
  vec11 = temp19 + temp111;
  temp111 = temp19 - temp111;

  temp112 = vec2 + vec4;
  temp114 = vec2 - vec4;
  temp113 = vec3 + vec5;
  temp115 = vec3 - vec5;

  *vec++ = temp20 + temp22;
  *vec++ = temp21 + temp23;
  *vec++ = temp12 + vec10;
  *vec++ = temp13 + vec11;
  *vec++ = temp210 + temp212;
  *vec++ = temp211 + temp213;
  *vec++ = temp10 + temp112;
  *vec++ = temp11 + temp113;
  *vec++ = temp24 - temp26;
  *vec++ = temp25 - temp27;
  *vec++ = temp16 + temp114;
  *vec++ = temp17 + temp115;
  *vec++ = temp28 + temp214;
  *vec++ = temp29 + temp215;
  *vec++ = temp14 + temp110;
  *vec++ = temp15 + temp111;
  *vec++ = temp20 - temp22;
  *vec++ = temp21 - temp23;
  *vec++ = temp12 - vec10;
  *vec++ = temp13 - vec11;
  *vec++ = temp210 - temp212;
  *vec++ = temp211 - temp213;
  *vec++ = temp10 - temp112;
  *vec++ = temp11 - temp113;
  *vec++ = temp24 + temp26;
  *vec++ = temp25 + temp27;
  *vec++ = temp16 - temp114;
  *vec++ = temp17 - temp115;
  *vec++ = temp28 - temp214;
  *vec++ = temp29 - temp215;
  *vec++ = temp14 - temp110;
  *vec++ = temp15 - temp111;
}
