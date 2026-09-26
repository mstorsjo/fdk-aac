
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
#include "mathlib.h"

#include "sac_tsd.h"

#define TSD_PI 3.14159265358979324f

static void longadd(unsigned short a[], unsigned short b[], int lena, int lenb) {
  int h;
  long carry = 0;

  assert(lena >= lenb);

  for (h = 0; h < lenb; h++) {
    carry += ((unsigned long)a[h]) + ((unsigned long)b[h]);
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  for (; h < lena; h++) {
    carry = ((unsigned long)a[h]) + carry;
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  assert(carry == 0);
  return;
}

static void longmult1(unsigned short a[], unsigned short b, unsigned short d[], int len) {
  int k;
  unsigned long tmp;
  unsigned long b0 = (unsigned long)b;

  tmp = ((unsigned long)a[0]) * b0;
  d[0] = (unsigned short)tmp;

  for (k = 1; k < len; k++) {
    tmp = (tmp >> 16) + ((unsigned long)a[k]) * b0;
    d[k] = (unsigned short)tmp;
  }
}

static void longdiv(unsigned short b[], unsigned short a, unsigned short d[], unsigned short* pr, int len) {
  unsigned long r;
  unsigned long tmp;
  int k;

  assert(a != 0);

  r = 0;

  for (k = len - 1; k >= 0; k--) {
    tmp = ((unsigned long)b[k]) + (r << 16);

    if (tmp) {
      d[k] = (unsigned short)(tmp / a);
      r = tmp - d[k] * a;
    } else {
      d[k] = 0;
    }
  }
  *pr = (unsigned short)r;
}

static short long_norm_l(int x) {
  short bits = 0;

  if (x != 0) {
    if (x < 0) {
      x = ~x;
      if (x == 0) return 31;
    }
    for (bits = 0; x < (int)0x40000000L; bits++) {
      x <<= 1;
    }
  }

  return (bits);
}

static void longsub(unsigned short a[], unsigned short b[], int lena, int lenb) {
  int h;
  long carry = 0;

  assert(lena >= lenb);
  for (h = 0; h < lenb; h++) {
    carry += ((unsigned long)a[h]) - ((unsigned long)b[h]);
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  for (; h < lena; h++) {
    carry = ((unsigned long)a[h]) + carry;
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  assert(carry == 0);
  return;
}

static int longcntbits(unsigned short a[], int len) {
  int k;
  int bits = 0;

  for (k = len - 1; k >= 0; k--) {
    if (a[k] != 0) {
      bits = k * 16 + 31 - long_norm_l((int)a[k]);
      break;
    }
  }

  return bits;
}

HANDLE_ERROR_INFO
CalcTSDpositions(float* tsdState,
                 TSDDATA* tsdData,
                 float*** pInRealIn,
                 float*** pInImagIn,
                 unsigned int nSampleRate,
                 unsigned int nFrameTimeSlots,
                 int* pbUseBBCues,
                 unsigned int nHybridBands) {
  HANDLE_ERROR_INFO error = noError;

  int tsdEnvBandsStart = 10;
  int tsdEnvBandsStop = nHybridBands - 1;

  float tau = 3.271417e-5f;
  float tsdAlpha = 1.0f - (float)exp(-1.0f / (tau * nSampleRate));
  float tsd1minusAlpha = 1.0f - tsdAlpha;

  float tsdThr = 1.15f;
  float tsdPwr;
  float tsdPwrEnv;
  int ts, hb;
  int tsdPos[MAX_TIME_SLOTS];
  int tsdPosLen = 0;
  int maxNbrTrSlots;
  int bUseBBCues = 0;

  if (error == noError) {
    if (tsdState == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (tsdData == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pInRealIn == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pInImagIn == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    tsdPwrEnv = *tsdState;

    if (nFrameTimeSlots < 7) {
      maxNbrTrSlots = 2;
    } else if (nFrameTimeSlots < 13) {
      maxNbrTrSlots = 4;
    } else if (nFrameTimeSlots < 25) {
      maxNbrTrSlots = 8;
    } else if (nFrameTimeSlots < 49) {
      maxNbrTrSlots = 16;
    } else {
      maxNbrTrSlots = 32;
    }

    for (ts = 0; ts < (int)nFrameTimeSlots; ts++) {
      tsdPwr = 0.0f;
      for (hb = tsdEnvBandsStart; hb < (tsdEnvBandsStop + 1); hb++) {
        float tsdDmxReal = pInRealIn[0][ts][hb] + pInRealIn[1][ts][hb];
        float tsdDmxImag = pInImagIn[0][ts][hb] + pInImagIn[1][ts][hb];

        tsdPwr += tsdDmxReal * tsdDmxReal + tsdDmxImag * tsdDmxImag;
      }
      tsdPwrEnv = tsd1minusAlpha * tsdPwrEnv + tsdAlpha * tsdPwr;

      if (tsdPwr > (tsdThr * tsdPwrEnv)) {
        tsdData->tsdSepData[ts] = 1;
        tsdPos[tsdPosLen] = ts;
        tsdPosLen++;
      } else
        tsdData->tsdSepData[ts] = 0;
    }

    if (tsdPosLen > maxNbrTrSlots) {
      int l;

      for (l = 0; l < (tsdPosLen - maxNbrTrSlots); l++) {
        ts = tsdPos[tsdPosLen - 1 - l];
        tsdData->tsdSepData[ts] = 0;
        tsdPos[tsdPosLen] = -1;
      }
      tsdPosLen = maxNbrTrSlots;
    }

    if (tsdPosLen == 0) {
      tsdData->bsTsdEnable = 0;
    } else {
      unsigned short c[5];
      unsigned short b;
      unsigned short r[1];

      int i, k, h;

      tsdData->bsTsdNumTrSlots = tsdPosLen;

      *tsdState = tsdPwrEnv;

      {
        int slots, positions;

        for (i = 0; i < 4; i++)
          tsdData->bsTsdCodedPos[i] = 0;

        for (k = tsdPosLen - 1; k >= 0; k--) {
          positions = k + 1;
          slots = tsdPos[k];

          if (positions >= slots + 1) break;

          c[0] = 1;
          for (i = 1; i < 5; i++)
            c[i] = 0;

          for (h = 1; h <= positions; h++) {
            b = slots - positions + h;
            longmult1(c, b, c, 5);
            b = h;
            longdiv(c, b, c, r, 5);
          }
          longadd(tsdData->bsTsdCodedPos, c, 4, 4);
        }
      }

      {
        int N = nFrameTimeSlots;
        int* bits = &tsdData->TsdCodewordLength;

        c[0] = 1;
        for (i = 1; i < 5; i++)
          c[i] = 0;

        for (k = 0; k < tsdPosLen; k++) {
          b = N - k;
          longmult1(c, b, c, 5);
          b = k + 1;
          longdiv(c, b, c, r, 5);
        }

        b = 1;
        longsub(c, &b, 5, 1);
        *bits = longcntbits(c, 5);
      }
    }
  }

  if (error == noError) {
    if (tsdData->bsTsdEnable) {
      bUseBBCues = 1;
    } else {
      bUseBBCues = 0;
    }
  }

  if (error == noError) {
    if (pbUseBBCues == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    } else {
      *pbUseBBCues = bUseBBCues;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
CalcTSDphase(TSDDATA* tsdData,
             float*** pInRealOut,
             float*** pInImagOut,
             unsigned int nFrameTimeSlots) {
  HANDLE_ERROR_INFO error = noError;

  float phiDmx, phiRes, phiDiff, phiMean;
  float absSqDmx, absSqRes, weight;
  float sumReal, sumImag;
  int phiMeanQ;
  int ts, hb;
  int tsdPhaseBandsStop = 45;
  float*** pppTsdReal;
  float*** pppTsdImag;

  if (error == noError) {
    if (tsdData == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pInRealOut == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pInImagOut == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    pppTsdReal = pInRealOut;
    pppTsdImag = pInImagOut;

    for (ts = 0; ts < (int)nFrameTimeSlots; ts++) {
      if (tsdData->tsdSepData[ts]) {
        sumReal = 0.0f;
        sumImag = 0.0f;
        for (hb = 1; hb < (tsdPhaseBandsStop + 1); hb++) {
          phiDmx = (float)atan2(pppTsdImag[0][ts][hb], pppTsdReal[0][ts][hb]);
          phiRes = (float)atan2(pppTsdImag[1][ts][hb], pppTsdReal[1][ts][hb]);
          phiDiff = phiRes - phiDmx;

          absSqDmx = pppTsdReal[0][ts][hb] * pppTsdReal[0][ts][hb] +
                     pppTsdImag[0][ts][hb] * pppTsdImag[0][ts][hb];
          absSqRes = pppTsdReal[1][ts][hb] * pppTsdReal[1][ts][hb] +
                     pppTsdImag[1][ts][hb] * pppTsdImag[1][ts][hb];
          weight = (float)sqrt(absSqDmx * absSqRes);

          sumReal += weight * (float)cos(phiDiff);
          sumImag += weight * (float)sin(phiDiff);
        }

        phiMean = (float)atan2(sumImag, sumReal);

        {
          int nQuantSteps = 8;
          float fmodangle = (float)fmod(phiMean + 2.0f * TSD_PI, 2.0f * TSD_PI);
          float index = fmodangle * (float)nQuantSteps / (2.0f * TSD_PI);

          phiMeanQ = (int)floor(index + 0.5f);
          if (phiMeanQ == nQuantSteps) {
            phiMeanQ = 0;
          }
        }

        tsdData->bsTsdTrPhaseData[ts] = phiMeanQ;
      }
    }
  }

  return error;
}
