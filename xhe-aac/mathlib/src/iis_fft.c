
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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "iisutillib.h"
#include "iis_fft.h"

#include <math.h>
#include "cfft.h"
#include "iisfft.h"

#define FFT_COMPLEX 1
#define FFT_REAL 2

typedef struct T_IIS_FFT {
  IIS_FFT_DIR sign;
  int len;
  float* buffer;
  float* sine_table;
  Iisfft iisfft;
  Cfft cfft;
} IIS_FFT;

static IIS_FFT_ERROR create(HANDLE_IIS_FFT* handle, int type, int len, IIS_FFT_DIR sign) {
  IIS_FFT_ERROR err = IIS_FFT_MEMORY_ERROR;
  HANDLE_IIS_FFT h = NULL;

  int trlen = (type == FFT_COMPLEX) ? len : len / 2;

  if (sign != IIS_FFT_FWD && sign != IIS_FFT_BWD)
    return IIS_FFT_INTERNAL_ERROR;

  if (len < 2 || (type == FFT_REAL && len % 4 != 0))
    return IIS_FFT_LENGTH_ERROR;

  h = (HANDLE_IIS_FFT)iisCalloc(1, sizeof(IIS_FFT));
  if (!h)
    return IIS_FFT_MEMORY_ERROR;

  h->len = len;
  h->sign = sign;

  if (!(trlen >= 256 && CFFT_PLAN_SUPPORT(trlen) && type == FFT_COMPLEX)) {
    h->buffer = (float*)iisMalloc(sizeof(float) * trlen * 2);
    if (!h->buffer)
      goto handle_error1;
  }

  if (type == FFT_REAL) {
    h->sine_table = create_sine_table(len);
    if (!h->sine_table)
      goto handle_error1;
  }

  if (trlen >= 256 && CFFT_PLAN_SUPPORT(trlen)) {
    int s = (type == FFT_REAL) ? IIS_FFT_FWD : sign;
    err = cfft_plan(&h->cfft, trlen, s) ? IIS_FFT_NO_ERROR : IIS_FFT_INTERNAL_ERROR;
  } else {
    int s = (type == FFT_REAL) ? IIS_FFT_FWD : sign;
    err = iisfft_plan(&h->iisfft, trlen, s);
  }
  if (err != IIS_FFT_NO_ERROR)
    goto handle_error2;

  *handle = h;
  return IIS_FFT_NO_ERROR;

handle_error2:
  iisFree(h->buffer);
handle_error1:
  iisFree(h);
  return err;
}

static IIS_FFT_ERROR destroy(HANDLE_IIS_FFT* handle) {
  if (handle && *handle) {
    iisfft_free(&(*handle)->iisfft);
    cfft_free(&(*handle)->cfft);
    if ((*handle)->sine_table)
      iisFree((*handle)->sine_table);
    if ((*handle)->buffer)
      iisFree((*handle)->buffer);
    iisFree(*handle);
    *handle = NULL;
  }
  return IIS_FFT_NO_ERROR;
}

IIS_FFT_ERROR IIS_CFFT_Create(HANDLE_IIS_FFT* handle, int len, IIS_FFT_DIR sign) {
  return create(handle, FFT_COMPLEX, len, sign);
}

IIS_FFT_ERROR IIS_RFFT_Create(HANDLE_IIS_FFT* handle, int len, IIS_FFT_DIR sign) {
  return create(handle, FFT_REAL, len, sign);
}

IIS_FFT_ERROR IIS_xFFT_Destroy(HANDLE_IIS_FFT* handle) { return destroy(handle); }

IIS_FFT_ERROR IIS_CFFT_Destroy(HANDLE_IIS_FFT* handle) { return destroy(handle); }

IIS_FFT_ERROR IIS_RFFT_Destroy(HANDLE_IIS_FFT* handle) { return destroy(handle); }

IIS_FFT_ERROR IIS_FFT_Apply_CFFT(HANDLE_IIS_FFT handle, const float* in_re, const float* in_im, float* out_re,
                                 float* out_im) {
  if (!handle)
    return IIS_FFT_INTERNAL_ERROR;

  if ((in_re != out_re) && (in_im != out_im)) {
    if ((in_re == in_im - 1) && (out_re == out_im - 1)) {
      memmove(out_re, in_re, sizeof *in_re * handle->len * 2);
      if (handle->cfft.len > 0) {
        cfft_apply(&handle->cfft, out_re, out_im, 2);
      } else {
        iisfft_apply(&handle->iisfft, out_re);
      }
    } else {
      if (handle->cfft.len > 0) {
        memmove(out_re, in_re, sizeof *in_re * handle->len);
        memmove(out_im, in_im, sizeof *in_im * handle->len);
        cfft_apply(&handle->cfft, out_re, out_im, 1);
      } else {
        fftf_interleave(in_re, in_im, handle->buffer, handle->len);
        iisfft_apply(&handle->iisfft, handle->buffer);
        fftf_deinterleave(handle->buffer, out_re, out_im, handle->len);
      }
    }
  } else {
    if (out_re == out_im - 1) {
      if (handle->cfft.len > 0) {
        cfft_apply(&handle->cfft, out_re, out_im, 2);
      } else {
        iisfft_apply(&handle->iisfft, out_re);
      }
    } else {
      if (handle->cfft.len > 0) {
        cfft_apply(&handle->cfft, out_re, out_im, 1);
      } else {
        fftf_interleave(out_re, out_im, handle->buffer, handle->len);
        iisfft_apply(&handle->iisfft, handle->buffer);
        fftf_deinterleave(handle->buffer, out_re, out_im, handle->len);
      }
    }
  }
  return IIS_FFT_NO_ERROR;
}

IIS_FFT_ERROR IIS_FFT_Apply_RFFT(HANDLE_IIS_FFT handle, const float* in, float* out) {
  if (!handle)
    return IIS_FFT_INTERNAL_ERROR;

  memmove(out, in, sizeof(float) * handle->len);

  if (handle->sign == IIS_FFT_BWD)
    rfft_pre(handle->sine_table, out, handle->len);

  if (handle->cfft.len > 0) {
    cfft_apply(&handle->cfft, out, out + 1, 2);
  } else {
    iisfft_apply(&handle->iisfft, out);
  }

  if (handle->sign == IIS_FFT_FWD)
    rfft_post(handle->sine_table, out, handle->len);

  return IIS_FFT_NO_ERROR;
}
