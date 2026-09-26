
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
#include <assert.h>
#include <string.h>

#include "iisutillib.h"
#include "iisfft.h"
#include "cfft.h"

#define INCLUDED_FROM_IISFFT_C
#include "fft_2_9.h"
#include "fft_15_16.h"
#include "fft_32.h"
#include "fft_60_128.h"
#include "fft_240_480.h"
#include "fft_384_768.h"
#include "fft_generic.h"

static void fftf_swapvec(float* data, int len) {
  float* ptr = data;

  while (ptr < data + 2 * len) {
    float tmp = ptr[0];
    ptr[0] = ptr[1];
    ptr[1] = tmp;
    ptr += 2;
  }
}

static IIS_FFT_ERROR iisfft_apply_stackbuffer(float* x, int length) {
  if (!fft_n(x, length)) {
    float* scratch = NULL;
    int* scratch2 = NULL;
    int num_factors = 0;
    int factors[IISFFT_MAXFACTORS];
    int isPrime[IISFFT_MAXFACTORS] = {0};
    int i, lengthOfPrimeScratch = BORDER_FOR_SECOND_SCRATCH;
    if (!factorize(length, &num_factors, factors, isPrime))
      return IIS_FFT_LENGTH_ERROR;
    scratch = (float*)ALLOCA(sizeof(float) * 2 * length);

    for (i = 0; i < num_factors; i++) {
      if (isPrime[i] == 1 && factors[i] > lengthOfPrimeScratch) {
        lengthOfPrimeScratch = factors[i];
      }
    }
    if (lengthOfPrimeScratch > BORDER_FOR_SECOND_SCRATCH) {
      scratch2 = (int*)ALLOCA(sizeof(int) * lengthOfPrimeScratch);
    }
    pfaDFT(x, length, scratch, num_factors, factors, scratch2, isPrime);
  }
  return IIS_FFT_NO_ERROR;
}

void iisfft_apply(Iisfft* handle, float* x) {
  if (handle->sign == -1) {
    if (!fft_n(x, handle->length))
      pfaDFT(x, handle->length, handle->scratch, handle->num_factors, handle->factors, handle->scratch2, handle->isPrime);
  } else {
    if (!ifft_n(x, handle->length)) {
      fftf_swapvec(x, handle->length);
      pfaDFT(x, handle->length, handle->scratch, handle->num_factors, handle->factors, handle->scratch2, handle->isPrime);
      fftf_swapvec(x, handle->length);
    }
  }
}

static int need_scratch(int n) {
  return n != 2 && n != 3 && n != 4 && n != 5 && n != 7 && n != 8 && n != 9 && n != 15 && n != 16 && n != 32 &&
         n != 60 && n != 64 && n != 128 && n != 240 && n != 256 && n != 384 && n != 480 && n != 512 && n != 768 &&
         n != 1024;
}

IIS_FFT_ERROR iisfft_plan(Iisfft* handle, int length, int sign) {
  memset(handle, 0, sizeof(Iisfft));
  if (length < 2)
    return IIS_FFT_LENGTH_ERROR;
  handle->length = length;
  handle->sign = sign;
  if (need_scratch(length)) {
    int i, lengthOfPrimeScratch = BORDER_FOR_SECOND_SCRATCH;
    if (!factorize(length, &handle->num_factors, handle->factors, handle->isPrime))
      return IIS_FFT_LENGTH_ERROR;
    handle->scratch = (float*)iisMalloc(sizeof(float) * 2 * length);

    for (i = 0; i < handle->num_factors; i++) {
      if (handle->isPrime[i] == 1 && handle->factors[i] > lengthOfPrimeScratch) {
        lengthOfPrimeScratch = handle->factors[i];
      }
    }
    if (lengthOfPrimeScratch > BORDER_FOR_SECOND_SCRATCH) {
      handle->scratch2 = (int*)iisMalloc(sizeof(int) * lengthOfPrimeScratch);
      if (!handle->scratch2)
        return IIS_FFT_MEMORY_ERROR;
    }
    if (!handle->scratch)
      return IIS_FFT_MEMORY_ERROR;
  }

  return IIS_FFT_NO_ERROR;
}

void iisfft_free(Iisfft* handle) {
  handle->length = 0;
  if (handle->scratch)
    iisFree(handle->scratch);
  if (handle->scratch2)
    iisFree(handle->scratch2);
}

IIS_FFT_ERROR iis_fftf(float* fft_data, int length) {
  if (fft_n(fft_data, length)) {
    return IIS_FFT_NO_ERROR;
  } else if (length > IISFFT_MAXSTACKLENGTH) {
    Iisfft handle;
    IIS_FFT_ERROR err = iisfft_plan(&handle, length, -1);
    if (err == IIS_FFT_NO_ERROR) {
      iisfft_apply(&handle, fft_data);
      iisfft_free(&handle);
    }
    return err;
  } else {
    return iisfft_apply_stackbuffer(fft_data, length);
  }
}

IIS_FFT_ERROR iis_ifftf(float* fft_data, int length) {
  if (ifft_n(fft_data, length)) {
    return IIS_FFT_NO_ERROR;
  } else {
    IIS_FFT_ERROR err = IIS_FFT_NO_ERROR;
    fftf_swapvec(fft_data, length);
    err = iis_fftf(fft_data, length);
    fftf_swapvec(fft_data, length);
    return err;
  }
}

void fftf_interleave(const float* restrict re, const float* restrict im, float* restrict out, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    *out++ = *re++;
    *out++ = *im++;
  }
}

void fftf_deinterleave(const float* restrict in, float* restrict re, float* restrict im, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    *re++ = *in++;
    *im++ = *in++;
  }
}

float* create_sine_table(int len) {
  int i = 0;
  float* sine_table = (float*)iisMalloc(sizeof(float) * (len / 2 + 1));
  if (!sine_table)
    return NULL;

  for (i = 0; i < len / 2 + 1; i++)
    sine_table[i] = (float)sin(2.0 * M_PI * i / len);

  return sine_table;
}

void rfft_post(const float* restrict sine_table, float* restrict buf, int len) {
  float tmp1, tmp2, tmp3, tmp4, s, c;
  int i = 0;

  tmp1 = buf[0] + buf[1];
  buf[1] = buf[0] - buf[1];
  buf[0] = tmp1;

  for (i = 1; i <= (len + 2) / 4; i++) {
    s = sine_table[i];
    c = sine_table[i + len / 4];

    tmp1 = buf[2 * i] - buf[len - 2 * i];
    tmp2 = buf[2 * i + 1] + buf[len - 2 * i + 1];
    tmp3 = s * tmp1 - c * tmp2;
    tmp4 = c * tmp1 + s * tmp2;
    tmp1 = buf[2 * i] + buf[len - 2 * i];
    tmp2 = buf[2 * i + 1] - buf[len - 2 * i + 1];

    buf[2 * i] = 0.5f * (tmp1 - tmp3);
    buf[2 * i + 1] = 0.5f * (tmp2 - tmp4);
    buf[len - 2 * i] = 0.5f * (tmp1 + tmp3);
    buf[len - 2 * i + 1] = -0.5f * (tmp2 + tmp4);
  }
}

void rfft_pre(const float* restrict sine_table, float* restrict buf, int len) {
  const float scale = 1.0f / len;
  float tmp1, tmp2, tmp3, tmp4, s, c;
  int i = 0;

  tmp1 = buf[0] + buf[1];
  buf[1] = scale * (buf[0] - buf[1]);
  buf[0] = scale * tmp1;

  for (i = 1; i <= (len + 2) / 4; i++) {
    s = sine_table[i];
    c = sine_table[i + len / 4];

    tmp1 = buf[2 * i] - buf[len - 2 * i];
    tmp2 = buf[2 * i + 1] + buf[len - 2 * i + 1];
    tmp3 = s * tmp1 + c * tmp2;
    tmp4 = -c * tmp1 + s * tmp2;
    tmp1 = buf[2 * i] + buf[len - 2 * i];
    tmp2 = buf[2 * i + 1] - buf[len - 2 * i + 1];

    buf[2 * i] = scale * (tmp1 + tmp3);
    buf[2 * i + 1] = -scale * (tmp2 + tmp4);
    buf[len - 2 * i] = scale * (tmp1 - tmp3);
    buf[len - 2 * i + 1] = scale * (tmp2 - tmp4);
  }
}
