
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

#define FFT_INTERNAL_TRIG_PREC double
#define BORDER_FOR_SECOND_SCRATCH 100

static const int primeFactors[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                                   43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
                                   103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157,
                                   163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223,
                                   227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277,
                                   281, 283, 293, 0};

static int fft_n(float* x, int length) {
  switch (length) {
    case 2:
      fft2(x);
      return 1;
    case 3:
      fft3(x);
      return 1;
    case 4:
      fft4(x);
      return 1;
    case 5:
      fft5(x);
      return 1;
    case 7:
      fft7(x);
      return 1;
    case 8:
      fft8(x);
      return 1;
    case 9:
      fft9(x);
      return 1;
    case 15:
      fft15(x);
      return 1;
    case 16:
      fft16(x);
      return 1;
    case 32:
      fft32(x);
      return 1;
    case 60:
      fft60(x);
      return 1;
    case 64:
      fft64(x);
      return 1;
    case 128:
      fft128(x);
      return 1;
    case 240:
      fft240(x);
      return 1;
    case 256:
      cfft(x, x + 1, 256, 2, -1);
      return 1;
    case 384:
      fft384(x);
      return 1;
    case 480:
      fft480(x);
      return 1;
    case 512:
      cfft(x, x + 1, 512, 2, -1);
      return 1;
    case 768:
      fft768(x);
      return 1;
    case 1024:
      cfft(x, x + 1, 1024, 2, -1);
      return 1;
    default:
      return 0;
  }
}

static int ifft_n(float* x, int length) {
  switch (length) {
    case 5:
      ifft5(x);
      return 1;
    case 8:
      ifft8(x);
      return 1;
    case 9:
      ifft9(x);
      return 1;
    case 256:
      cfft(x, x + 1, 256, 2, 1);
      return 1;
    case 512:
      cfft(x, x + 1, 512, 2, 1);
      return 1;
    case 1024:
      cfft(x, x + 1, 1024, 2, 1);
      return 1;
    default:
      return 0;
  }
}

static int factorize(int length, int* restrict numFactors, int* restrict factor, int* restrict isPrime) {
  int remainder = length;
  int idx = 0, cnt = 0;
  int actFac = primeFactors[idx];
  int inc = 0;

  *numFactors = 0;

  while (remainder > 1 && actFac != 0) {
    if (remainder % actFac == 0) {
      if (inc == 0) {
        inc = 1;
        (*numFactors)++;
      }
      remainder /= actFac;
    } else {
      actFac = primeFactors[++idx];
      inc = 0;
    }
  }
  if (remainder > 1) {
    (*numFactors)++;
  }

  if (*numFactors > IISFFT_MAXFACTORS)
    return 0;

  idx = 0;
  cnt = 0;
  inc = 0;
  remainder = length;
  actFac = primeFactors[idx];
  (factor)[cnt] = 1;
  while (remainder > 1 && actFac != 0) {
    if (remainder % actFac == 0) {
      (factor)[cnt] *= actFac;
      remainder /= actFac;
      inc = 1;
      if (factor[cnt] == actFac) {
        isPrime[cnt] = 1;
      } else {
        isPrime[cnt] = 0;
      }
    } else {
      actFac = primeFactors[++idx];
      if (inc == 1) {
        cnt++;
      }
      inc = 0;
      (factor)[cnt] = 1;
    }
  }
  if (remainder > 1) {
    factor[cnt] = remainder;
  }
  return 1;
}

static void oddFFT(float* restrict x, int length, float* restrict scratch) {
  int i, k, n;
  float *src1, *src2, *dest1, *dest2;
  FFT_INTERNAL_TRIG_PREC sinValOrg, cosValOrg;

  dest1 = scratch + 1;
  dest2 = scratch + length + 1;
  src1 = x + 2;
  src2 = x + 2 * length - 1;

  scratch[0] = x[0];
  scratch[length] = x[1];

  for (i = 2; i < length; i += 2) {
    float tmp1R, tmp1I, tmp2R, tmp2I;
    tmp1R = *src1++;
    tmp1I = *src1++;
    tmp2I = *src2--;
    tmp2R = *src2--;
    *dest1++ = tmp1R + tmp2R;
    *dest1++ = tmp1R - tmp2R;
    *dest2++ = tmp1I + tmp2I;
    *dest2++ = tmp1I - tmp2I;

    x[0] += tmp1R + tmp2R;
    x[1] += tmp1I + tmp2I;
  }

  dest1 = x + 2;
  dest2 = x + 2 * length - 2;
  for (k = 2; k < length; k += 2) {
    FFT_INTERNAL_TRIG_PREC sinVal = 0.f, cosVal = 1.f;
    cosValOrg = (FFT_INTERNAL_TRIG_PREC)cos(-M_PI * k / length);
    sinValOrg = (FFT_INTERNAL_TRIG_PREC)sin(-M_PI * k / length);

    *dest1 = *dest2 = scratch[0];
    *(dest1 + 1) = *(dest2 + 1) = scratch[length];

    src1 = scratch + 1;
    src2 = scratch + length + 1;

    for (n = 2; n < length; n += 2) {
      float rePre, reMre, imPim, imMim;

      FFT_INTERNAL_TRIG_PREC tmp = cosVal * cosValOrg - sinVal * sinValOrg;
      sinVal = sinVal * cosValOrg + cosVal * sinValOrg;
      cosVal = tmp;

      rePre = *src1++;
      reMre = *src1++;
      imPim = *src2++;
      imMim = *src2++;

      *dest1 += (float)cosVal * rePre - (float)sinVal * imMim;
      *(dest1 + 1) += (float)sinVal * reMre + (float)cosVal * imPim;
      *dest2 += (float)cosVal * rePre + (float)sinVal * imMim;
      *(dest2 + 1) += (float)-sinVal * reMre + (float)cosVal * imPim;
    }
    dest1 += 2;
    dest2 -= 2;
  }
}

static int modMultInv(int n1, int n2) {
  int a1 = 0, a2 = 1;
  int n1_new = n1, n2_new = n2;

  if (n2 == 1) {
    return 1;
  }

  while (n1_new > 1) {
    int quot = n1_new / n2_new;
    int rem = n1_new - quot * n2_new;
    int a1_new;
    n1_new = n2_new;
    n2_new = rem;
    a1_new = a2 - a1 * quot;
    a2 = a1;
    a1 = a1_new;
  }

  if (a2 < 0) {
    a2 += n2;
  }

  return a2;
}

static int getGeneratorStupid(int groupLength) {
  int generator = 2;
  int count = 1, number = generator;

  while (generator < 100) {
    while (number != 1) {
      number = (number * generator) % groupLength;
      count++;
    }
    if (count == groupLength - 1) {
      return generator;
    } else {
      generator++;
      count = 1;
      number = generator;
    }
  }

  return -1;
}

static int getGenerator(int groupLength) {
  int generator = 2;
  int count, number, factorCount, found, count2;
  int factors[16] = {0};

  factorCount = 0;
  number = groupLength - 1;
  found = 0;
  count = 0;
  while (number != 1) {
    if (primeFactors[count] == 0) {
      return getGeneratorStupid(groupLength);
    }
    if (number % primeFactors[count] == 0) {
      number /= primeFactors[count];
      if (found == 0) {
        factors[factorCount++] = primeFactors[count];
        found = 1;
      }
    } else {
      count++;
      found = 0;
    }
  }

  for (count = 0; factors[count] != 0; count++) {
    factors[count] = (groupLength - 1) / factors[count];
  }

  number = generator;
  count = 0;
  while (factors[count] != 0) {
    for (count2 = 0; count2 < factors[count] - 1; count2++) {
      number = (number * generator) % groupLength;
    }
    if (number != 1) {
      count++;
      number = generator;
      if (factors[count] == 0) {
        return generator;
      }
    } else {
      count = 0;
      generator++;
      number = generator;
    }
  }

  return -1;
}

static void primeFFT(float* restrict x, int length, float* restrict scratch, int* restrict scratch2) {
  int i, k, middle = (length - 1) / 2;
  float *src1, *src2, *dest1, *dest2;
  int *mapping, *map;
  int generator;

  int mappingTable[25][97] = {
      {0, 2},
      {0, 2, 4},
      {0, 2, 4, 8, 6},
      {0, 2, 6, 4, 12, 8, 10},
      {0, 2, 4, 8, 16, 10, 20, 18, 14, 6, 12},
      {0, 2, 4, 8, 16, 6, 12, 24, 22, 18, 10, 20, 14},
      {0, 2, 6, 18, 20, 26, 10, 30, 22, 32, 28, 16, 14, 8, 24, 4, 12},
      {0, 2, 4, 8, 16, 32, 26, 14, 28, 18, 36, 34, 30, 22, 6, 12, 24, 10, 20},
      {0, 2, 10, 4, 20, 8, 40, 16, 34, 32, 22, 18, 44, 36, 42, 26, 38, 6, 30, 12, 14, 24, 28},
      {0, 2, 4, 8, 16, 32, 6, 12, 24, 48, 38, 18, 36, 14, 28, 56, 54, 50, 42, 26, 52, 46, 34, 10, 20, 40, 22, 44, 30},
      {0, 2, 6, 18, 54, 38, 52, 32, 34, 40, 58, 50, 26, 16, 48, 20, 60, 56, 44, 8, 24, 10, 30, 28, 22, 4, 12, 36, 46, 14, 42},
      {0, 2, 4, 8, 16, 32, 64, 54, 34, 68, 62, 50, 26, 52, 30, 60, 46, 18, 36, 72, 70, 66, 58, 42, 10, 20, 40, 6, 12, 24, 48, 22, 44, 14, 28, 56, 38},
      {0, 2, 12, 72, 22, 50, 54, 78, 58, 20, 38, 64, 56, 8, 48, 42, 6, 36, 52, 66, 68, 80, 70, 10, 60, 32, 28, 4, 24, 62, 44, 18, 26, 74, 34, 40, 76, 46, 30, 16, 14},
      {0, 2, 6, 18, 54, 76, 56, 82, 74, 50, 64, 20, 60, 8, 24, 72, 44, 46, 52, 70, 38, 28, 84, 80, 68, 32, 10, 30, 4, 12, 36, 22, 66, 26, 78, 62, 14, 42, 40, 34, 16, 48, 58},
      {0, 2, 10, 50, 62, 28, 46, 42, 22, 16, 80, 24, 26, 36, 86, 54, 82, 34, 76, 4, 20, 6, 30, 56, 92, 84, 44, 32, 66, 48, 52, 72, 78, 14, 70, 68, 58, 8, 40, 12, 60, 18, 90, 74, 88, 64, 38},
      {0, 2, 4, 8, 16, 32, 64, 22, 44, 88, 70, 34, 68, 30, 60, 14, 28, 56, 6, 12, 24, 48, 96, 86, 66, 26, 52, 104, 102, 98, 90, 74, 42, 84, 62, 18, 36, 72, 38, 76, 46, 92, 78, 50, 100, 94, 82, 58, 10, 20, 40, 80, 54},
      {0, 2, 4, 8, 16, 32, 64, 10, 20, 40, 80, 42, 84, 50, 100, 82, 46, 92, 66, 14, 28, 56, 112, 106, 94, 70, 22, 44, 88, 58, 116, 114, 110, 102, 86, 54, 108, 98, 78, 38, 76, 34, 68, 18, 36, 72, 26, 52, 104, 90, 62, 6, 12, 24, 48, 96, 74, 30, 60},
      {0, 2, 4, 8, 16, 32, 64, 6, 12, 24, 48, 96, 70, 18, 36, 72, 22, 44, 88, 54, 108, 94, 66, 10, 20, 40, 80, 38, 76, 30, 60, 120, 118, 114, 106, 90, 58, 116, 110, 98, 74, 26, 52, 104, 86, 50, 100, 78, 34, 68, 14, 28, 56, 112, 102, 82, 42, 84, 46, 92, 62},
      {0, 2, 4, 8, 16, 32, 64, 128, 122, 110, 86, 38, 76, 18, 36, 72, 10, 20, 40, 80, 26, 52, 104, 74, 14, 28, 56, 112, 90, 46, 92, 50, 100, 66, 132, 130, 126, 118, 102, 70, 6, 12, 24, 48, 96, 58, 116, 98, 62, 124, 114, 94, 54, 108, 82, 30, 60, 120, 106, 78, 22, 44, 88, 42, 84, 34, 68},
      {0, 2, 14, 98, 118, 116, 102, 4, 28, 54, 94, 90, 62, 8, 56, 108, 46, 38, 124, 16, 112, 74, 92, 76, 106, 32, 82, 6, 42, 10, 70, 64, 22, 12, 84, 20, 140, 128, 44, 24, 26, 40, 138, 114, 88, 48, 52, 80, 134, 86, 34, 96, 104, 18, 126, 30, 68, 50, 66, 36, 110, 60, 136, 100, 132, 72, 78, 120, 130, 58, 122},
      {0, 2, 10, 50, 104, 82, 118, 6, 30, 4, 20, 100, 62, 18, 90, 12, 60, 8, 40, 54, 124, 36, 34, 24, 120, 16, 80, 108, 102, 72, 68, 48, 94, 32, 14, 70, 58, 144, 136, 96, 42, 64, 28, 140, 116, 142, 126, 46, 84, 128, 56, 134, 86, 138, 106, 92, 22, 110, 112, 122, 26, 130, 66, 38, 44, 74, 78, 98, 52, 114, 132, 76, 88},
      {0, 2, 6, 18, 54, 4, 12, 36, 108, 8, 24, 72, 58, 16, 48, 144, 116, 32, 96, 130, 74, 64, 34, 102, 148, 128, 68, 46, 138, 98, 136, 92, 118, 38, 114, 26, 78, 76, 70, 52, 156, 152, 140, 104, 154, 146, 122, 50, 150, 134, 86, 100, 142, 110, 14, 42, 126, 62, 28, 84, 94, 124, 56, 10, 30, 90, 112, 20, 60, 22, 66, 40, 120, 44, 132, 80, 82, 88, 106},
      {0, 2, 4, 8, 16, 32, 64, 128, 90, 14, 28, 56, 112, 58, 116, 66, 132, 98, 30, 60, 120, 74, 148, 130, 94, 22, 44, 88, 10, 20, 40, 80, 160, 154, 142, 118, 70, 140, 114, 62, 124, 82, 164, 162, 158, 150, 134, 102, 38, 76, 152, 138, 110, 54, 108, 50, 100, 34, 68, 136, 106, 46, 92, 18, 36, 72, 144, 122, 78, 156, 146, 126, 86, 6, 12, 24, 48, 96, 26, 52, 104, 42, 84},
      {0, 2, 6, 18, 54, 162, 130, 34, 102, 128, 28, 84, 74, 44, 132, 40, 120, 4, 12, 36, 108, 146, 82, 68, 26, 78, 56, 168, 148, 88, 86, 80, 62, 8, 24, 72, 38, 114, 164, 136, 52, 156, 112, 158, 118, 176, 172, 160, 124, 16, 48, 144, 76, 50, 150, 94, 104, 134, 46, 138, 58, 174, 166, 142, 70, 32, 96, 110, 152, 100, 122, 10, 30, 90, 92, 98, 116, 170, 154, 106, 140, 64, 14, 42, 126, 22, 66, 20, 60},
      {0, 2, 10, 50, 56, 86, 42, 16, 80, 12, 60, 106, 142, 128, 58, 96, 92, 72, 166, 54, 76, 186, 154, 188, 164, 44, 26, 130, 68, 146, 148, 158, 14, 70, 156, 4, 20, 100, 112, 172, 84, 32, 160, 24, 120, 18, 90, 62, 116, 192, 184, 144, 138, 108, 152, 178, 114, 182, 134, 88, 52, 66, 136, 98, 102, 122, 28, 140, 118, 8, 40, 6, 30, 150, 168, 64, 126, 48, 46, 36, 180, 124, 38, 190, 174, 94, 82, 22, 110, 162, 34, 170, 74, 176, 104, 132, 78}};

  if (length < BORDER_FOR_SECOND_SCRATCH) {
    for (i = 1;; i++) {
      if (primeFactors[i] == length) {
        mapping = mappingTable[i];
        break;
      }
      assert(primeFactors[i] != 0);
    }
  } else {
    mapping = scratch2;

    generator = getGenerator(length);
    assert(generator != -1);

    mapping[0] = 0;
    mapping[1] = 1;
    for (i = 2; i < length; i++) {
      mapping[i] = mapping[i - 1] * generator;
      if (mapping[i] > length - 1) {
        mapping[i] = (mapping[i] % length - 1) + 1;
      }
    }

    for (i = 1; i < length; i++) {
      mapping[i] *= 2;
    }
  }

  scratch[0] = x[0];
  scratch[1] = x[1];
  scratch[2] = x[2];
  scratch[3] = x[3];
  map = mapping + length - 1;
  for (i = 4; i < 2 * length; map--) {
    scratch[i++] = x[(*map)];
    scratch[i++] = x[(*map) + 1];
  }

  x[length] = x[1];
  dest1 = x + 1;
  dest2 = x + length + 1;
  src1 = scratch + 2;
  src2 = scratch + length + 1;

  for (i = 2; i < length; i += 2) {
    float tmp1R, tmp1I, tmp2R, tmp2I;
    tmp1R = *src1++;
    tmp1I = *src1++;
    tmp2R = *src2++;
    tmp2I = *src2++;
    *dest1++ = tmp1R + tmp2R;
    *dest1++ = tmp1R - tmp2R;
    *dest2++ = tmp1I + tmp2I;
    *dest2++ = tmp1I - tmp2I;

    scratch[0] += tmp1R + tmp2R;
    scratch[1] += tmp1I + tmp2I;
  }

  dest1 = scratch + 2;
  for (i = 1; i < length; i++) {
    *dest1++ = x[0];
    *dest1++ = x[length];
  }

  for (k = 1; k <= middle; k++) {
    float sinVal, cosVal;
    int length1, length2;
    float rePre, reMre, imPim, imMim;

    cosVal = (float)cos(-M_PI * mapping[k] / length);
    sinVal = (float)sin(-M_PI * mapping[k] / length);

    length1 = middle - k + 1;
    length2 = middle - length1;
    src1 = x + 1;
    src2 = x + length + 1;
    dest1 = scratch + 2 * k;
    dest2 = scratch + 2 * (middle + k);

    for (i = 0; i < length1; i++) {
      rePre = *src1++;
      reMre = *src1++;
      imPim = *src2++;
      imMim = *src2++;

      *dest1++ += cosVal * rePre - sinVal * imMim;
      *dest1++ += cosVal * imPim + sinVal * reMre;

      *dest2++ += cosVal * rePre + sinVal * imMim;
      *dest2++ += cosVal * imPim - sinVal * reMre;
    }
    if (dest2 == scratch + 2 * length) {
      dest2 = scratch + 2;
    }
    for (i = 0; i < length2; i++) {
      rePre = *src1++;
      reMre = *src1++;
      imPim = *src2++;
      imMim = *src2++;

      *dest1++ += cosVal * rePre - sinVal * imMim;
      *dest1++ += cosVal * imPim + sinVal * reMre;

      *dest2++ += cosVal * rePre + sinVal * imMim;
      *dest2++ += cosVal * imPim - sinVal * reMre;
    }
  }

  x[0] = scratch[0];
  x[1] = scratch[1];
  map = mapping + 1;
  for (i = 2; i < 2 * length; map++) {
    x[(*map)] = scratch[i++];
    x[(*map) + 1] = scratch[i++];
  }
}

static void nextFFT(float* x, int length, float* scratch) {
  if (fft_n(x, length))
    return;

  assert(length % 2 != 0);
  oddFFT(x, length, scratch);
}

static inline int findFactor(const int length) {
  static const int factors[] = {16, 9, 8, 7, 5, 4, 3, 2, 0};
  int i = 0, factor = 0;
  for (i = 0; factors[i] != 0; i++) {
    if (length % factors[i] == 0) {
      factor = factors[i];
      break;
    }
  }
  return factor;
}

static inline void twiddle(float* x, const int length, const int n1, const int n2) {
  int i, ii;
  FFT_INTERNAL_TRIG_PREC sinValOrg, cosValOrg;
  FFT_INTERNAL_TRIG_PREC sinVal = 0.f, cosVal = 1.f;
  FFT_INTERNAL_TRIG_PREC twReal = 0.f, twImag = 1.f;

  cosValOrg = (FFT_INTERNAL_TRIG_PREC)cos(-2 * M_PI / length);
  sinValOrg = (FFT_INTERNAL_TRIG_PREC)sin(-2 * M_PI / length);

  for (i = 1; i < n1; i++) {
    FFT_INTERNAL_TRIG_PREC tmp = 0.f;
    twReal = 1.f;
    twImag = 0.f;

    tmp = cosVal * cosValOrg - sinVal * sinValOrg;
    sinVal = sinVal * cosValOrg + cosVal * sinValOrg;
    cosVal = tmp;

    for (ii = 1; ii < n2; ii++) {
      float xRe, xIm;
      FFT_INTERNAL_TRIG_PREC tmpReal;

      tmpReal = twReal * cosVal - twImag * sinVal;
      twImag = twImag * cosVal + sinVal * twReal;
      twReal = tmpReal;

      xRe = x[2 * (i * n2 + ii)];
      xIm = x[2 * (i * n2 + ii) + 1];

      x[2 * (i * n2 + ii)] = (float)twReal * xRe - (float)twImag * xIm;
      x[2 * (i * n2 + ii) + 1] = (float)twImag * xRe + (float)twReal * xIm;
    }
  }
}

static void cooleyTukeyFFT(float* restrict x, const int length, float* restrict scratch, int* restrict scratch2, int isPrime) {
  int factor;
  int i, ii;
  int n1, n2;
  int cnt = 0;
  float *src, *dest;

  if (fft_n(x, length))
    return;

  factor = findFactor(length);
  if (factor > 0 && (length / factor > 1)) {
    n1 = factor;
    n2 = length / factor;

    dest = scratch;
    for (i = 0; i < 2 * n1; i += 2) {
      src = x + i;
      for (ii = 0; ii < n2; ii++) {
        *dest++ = *src;
        *dest++ = *(src + 1);
        src += 2 * n1;
      }
    }
    src = scratch;
    dest = x;
    for (i = 0; i < length; i++) {
      *dest++ = *src++;
      *dest++ = *src++;
    }

    for (i = 0; i < n1; i++) {
      cooleyTukeyFFT(x + 2 * i * n2, n2, scratch + 2 * i * n2, scratch2, isPrime);
    }

    twiddle(x, length, n1, n2);

    cnt = 0;
    for (i = 0; i < n2; i++) {
      for (ii = 0; ii < n1; ii++) {
        scratch[2 * cnt] = x[2 * (i + ii * n2)];
        scratch[2 * cnt + 1] = x[2 * (i + ii * n2) + 1];
        cnt++;
      }
    }

    for (i = 0; i < n2; i++) {
      nextFFT(scratch + 2 * i * n1, n1, x + 2 * i * n1);
    }
    cnt = 0;
    for (i = 0; i < n1; i++) {
      for (ii = 0; ii < n2; ii++) {
        x[2 * cnt] = scratch[2 * (i + ii * n1)];
        x[2 * cnt + 1] = scratch[2 * (i + ii * n1) + 1];
        cnt++;
      }
    }
  } else {
    if (isPrime == 1 && length > 23) {
      primeFFT(x, length, scratch, scratch2);
    } else {
      oddFFT(x, length, scratch);
    }
  }
}

static void pfaDFT(float* restrict x, const int length, float* restrict scratch1, const int numFactors,
                   const int* const factor, int* restrict scratch2, const int* const isPrime) {
  float* tmp = scratch1;
  int i, ii, n1, n2, idx, incr, cnt;
  int n1_inv = 1;

  if (numFactors <= 1) {
    cooleyTukeyFFT(x, length, scratch1, scratch2, isPrime[0]);
    return;
  }

  n2 = factor[0];
  n1 = length / n2;

  n1_inv = modMultInv(n1, n2);

  idx = 0;
  incr = n1 * n1_inv;
  cnt = 0;
  for (i = 0; i < n1; i++) {
    for (ii = 0; ii < n2 - 1; ii++) {
      tmp[cnt++] = x[2 * idx];
      tmp[cnt++] = x[2 * idx + 1];

      idx += incr;
      if (idx > length) {
        idx -= length;
      }
    }
    tmp[cnt++] = x[2 * idx];
    tmp[cnt++] = x[2 * idx + 1];
    idx++;
  }

  for (cnt = 0; cnt < length; cnt += n2) {
    cooleyTukeyFFT(tmp + 2 * cnt, n2, x + 2 * cnt, scratch2, isPrime[0]);
  }
  for (cnt = 0; cnt < n1; cnt++) {
    for (i = 0; i < n2; i++) {
      x[2 * (cnt + i * n1)] = tmp[2 * (cnt * n2 + i)];
      x[2 * (cnt + i * n1) + 1] = tmp[2 * (cnt * n2 + i) + 1];
    }
  }
  for (cnt = 0; cnt < length; cnt += n1) {
    pfaDFT(x + 2 * cnt, n1, tmp, numFactors - 1, &factor[1], scratch2, &isPrime[1]);
  }

  cnt = 0;
  for (i = 0; i < n2; i++) {
    idx = i * n1;
    for (ii = 0; ii < n1; ii++) {
      tmp[2 * idx] = x[cnt++];
      tmp[2 * idx + 1] = x[cnt++];
      idx += n2;
      if (idx > length) {
        idx -= length;
      }
    }
  }

  for (cnt = 0; cnt < length; cnt++) {
    x[2 * cnt] = tmp[2 * cnt];
    x[2 * cnt + 1] = tmp[2 * cnt + 1];
  }
}
