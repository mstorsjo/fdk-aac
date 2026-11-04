
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

static void fft60(float* in) {
  const int table1[] = {0, 45, 30, 15, 16, 1, 46, 31, 32, 17, 2, 47, 48, 33, 18, 3, 4, 49, 34, 19,
                        20, 5, 50, 35, 36, 21, 6, 51, 52, 37, 22, 7, 8, 53, 38, 23, 24, 9, 54, 39,
                        40, 25, 10, 55, 56, 41, 26, 11, 12, 57, 42, 27, 28, 13, 58, 43, 44, 29, 14, 59};
  const int table2[] = {0, 15, 30, 45, 4, 19, 34, 49, 8, 23, 38, 53, 12, 27, 42, 57, 16, 31, 46, 1,
                        20, 35, 50, 5, 24, 39, 54, 9, 28, 43, 58, 13, 32, 47, 2, 17, 36, 51, 6, 21,
                        40, 55, 10, 25, 44, 59, 14, 29, 48, 3, 18, 33, 52, 7, 22, 37, 56, 11, 26, 41};
  const int a = 4;
  const int b = 15;
  const int L = 60;
  const int* idx1 = table1;
  const int* idx2 = table2;

  ALIGN_16_BYTE float temp[30], out[120];
  int k, l;

  for (k = 0; k < a; k++) {
    for (l = 0; l < b; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1 + 1];
      idx1 += a;
    }

    fft15(temp);
    idx1 -= L;

    for (l = 0; l < b; l++) {
      in[2 * *idx1] = temp[2 * l];
      in[2 * *idx1 + 1] = temp[2 * l + 1];
      idx1 += a;
    }
    idx1 -= L - 1;
  }

  idx1 -= a;

  for (k = 0; k < b; k++) {
    for (l = 0; l < a; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1++ + 1];
    }

    fft4(temp);

    for (l = 0; l < a; l++) {
      out[2 * *idx2] = temp[2 * l];
      out[2 * *idx2++ + 1] = temp[2 * l + 1];
    }
  }
  memmove(in, out, 2 * L * sizeof(float));
}

static void fft64(float* vec) {
  const float w[] = {1.0000000000f, 0.9951847267f, 0.9807852804f, 0.9569403357f, 0.9238795325f, 0.8819212643f,
                     0.8314696123f, 0.7730104534f, 0.7071067812f, 0.6343932842f, 0.5555702330f, 0.4713967368f,
                     0.3826834324f, 0.2902846773f, 0.1950903220f, 0.0980171403f, 0.0000000000f, -0.0980171403f,
                     -0.1950903220f, -0.2902846773f, -0.3826834324f, -0.4713967368f, -0.5555702330f, -0.6343932842f,
                     -0.7071067812f, -0.7730104534f, -0.8314696123f, -0.8819212643f, -0.9238795325f, -0.9569403357f,
                     -0.9807852804f, -0.9951847267f, -1.0000000000f, -0.9951847267f, -0.9807852804f, -0.9569403357f,
                     -0.9238795325f, -0.8819212643f, -0.8314696123f, -0.7730104534f, -0.7071067812f, -0.6343932842f,
                     -0.5555702330f, -0.4713967368f, -0.3826834324f, -0.2902846773f, -0.1950903220f, -0.0980171403f};

  float temp1[64], temp2[64];
  int i;

  for (i = 0; i < 32; i++) {
    temp1[2 * i] = vec[4 * i];
    temp1[2 * i + 1] = vec[4 * i + 1];
    temp2[2 * i] = vec[4 * i + 2];
    temp2[2 * i + 1] = vec[4 * i + 3];
  }

  fft32(temp1);
  fft32(temp2);

  for (i = 0; i < 32; i++) {
    float re, im, wre, wim, tre, tim;

    re = temp2[2 * i];
    im = temp2[2 * i + 1];

    wre = w[i];
    wim = w[i + 16];

    tre = re * wre - im * wim;
    tim = re * wim + im * wre;

    vec[2 * i] = temp1[2 * i] + tre;
    vec[2 * i + 1] = temp1[2 * i + 1] + tim;
    vec[2 * i + 64] = temp1[2 * i] - tre;
    vec[2 * i + 65] = temp1[2 * i + 1] - tim;
  }
}

static void fft128(float* vec) {
  const float w[] = {
      1.0000000000f,
      0.9987954562f,
      0.9951847267f,
      0.9891765100f,
      0.9807852804f,
      0.9700312532f,
      0.9569403357f,
      0.9415440652f,
      0.9238795325f,
      0.9039892931f,
      0.8819212643f,
      0.8577286100f,
      0.8314696123f,
      0.8032075315f,
      0.7730104534f,
      0.7409511254f,
      0.7071067812f,
      0.6715589548f,
      0.6343932842f,
      0.5956993045f,
      0.5555702330f,
      0.5141027442f,
      0.4713967368f,
      0.4275550934f,
      0.3826834324f,
      0.3368898534f,
      0.2902846773f,
      0.2429801799f,
      0.1950903220f,
      0.1467304745f,
      0.0980171403f,
      0.0490676743f,
      0.0000000000f,
      -0.0490676743f,
      -0.0980171403f,
      -0.1467304745f,
      -0.1950903220f,
      -0.2429801799f,
      -0.2902846773f,
      -0.3368898534f,
      -0.3826834324f,
      -0.4275550934f,
      -0.4713967368f,
      -0.5141027442f,
      -0.5555702330f,
      -0.5956993045f,
      -0.6343932842f,
      -0.6715589548f,
      -0.7071067812f,
      -0.7409511254f,
      -0.7730104534f,
      -0.8032075315f,
      -0.8314696123f,
      -0.8577286100f,
      -0.8819212643f,
      -0.9039892931f,
      -0.9238795325f,
      -0.9415440652f,
      -0.9569403357f,
      -0.9700312532f,
      -0.9807852804f,
      -0.9891765100f,
      -0.9951847267f,
      -0.9987954562f,
      -1.0000000000f,
      -0.9987954562f,
      -0.9951847267f,
      -0.9891765100f,
      -0.9807852804f,
      -0.9700312532f,
      -0.9569403357f,
      -0.9415440652f,
      -0.9238795325f,
      -0.9039892931f,
      -0.8819212643f,
      -0.8577286100f,
      -0.8314696123f,
      -0.8032075315f,
      -0.7730104534f,
      -0.7409511254f,
      -0.7071067812f,
      -0.6715589548f,
      -0.6343932842f,
      -0.5956993045f,
      -0.5555702330f,
      -0.5141027442f,
      -0.4713967368f,
      -0.4275550934f,
      -0.3826834324f,
      -0.3368898534f,
      -0.2902846773f,
      -0.2429801799f,
      -0.1950903220f,
      -0.1467304745f,
      -0.0980171403f,
      -0.0490676743f,
  };

  ALIGN_16_BYTE float temp1[128], temp2[128];
  int i;

  for (i = 0; i < 64; i++) {
    temp1[2 * i] = vec[4 * i];
    temp1[2 * i + 1] = vec[4 * i + 1];
    temp2[2 * i] = vec[4 * i + 2];
    temp2[2 * i + 1] = vec[4 * i + 3];
  }

  fft64(temp1);
  fft64(temp2);

  for (i = 0; i < 64; i++) {
    float re, im, wre, wim, tre, tim;

    re = temp2[2 * i];
    im = temp2[2 * i + 1];

    wre = w[i];
    wim = w[i + 32];

    tre = re * wre - im * wim;
    tim = re * wim + im * wre;

    vec[2 * i] = temp1[2 * i] + tre;
    vec[2 * i + 1] = temp1[2 * i + 1] + tim;
    vec[2 * i + 128] = temp1[2 * i] - tre;
    vec[2 * i + 129] = temp1[2 * i + 1] - tim;
  }
}
