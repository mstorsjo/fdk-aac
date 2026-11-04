
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define IMLRESTRICT

#include "mathlib.h"
#include "cpuinfo.h"

#define AVOID_COMPILER_WARNING(expr) \
  do {                               \
    (void)(expr);                    \
  } while (0);

void foo(int t);

void foo(int t) {
  AVOID_COMPILER_WARNING(t);
}

#if defined NO_OPTIMIZATION

#if (__STDC_VERSION__ >= 199901L) || defined _MSC_VER
#define iml_sqrtf(a) sqrtf(a)
#define iml_powf(a, b) powf(a, b)
#define iml_expf(a) expf(a)
#define iml_logf(a) logf(a)

#define iml_fabsf(a) fabsf(a)
#define iml_ceilf(a) ceilf(a)
#define iml_floorf(a) floorf(a)

#define iml_sinf(a) sinf(a)
#define iml_cosf(a) cosf(a)
#else
#define iml_sqrtf(a) (float)sqrt((double)(a))
#define iml_powf(a, b) (float)pow((double)(a), (double)(b))
#define iml_expf(a) (float)exp((double)(a))
#define iml_logf(a) (float)log((double)(a))

#define iml_fabsf(a) (float)fabs((double)(a))
#define iml_ceilf(a) (float)ceil((double)(a))
#define iml_floorf(a) (float)floor((double)(a))

#define iml_sinf(a) (float)sin((double)(a))
#define iml_cosf(a) (float)cos((double)(a))

#endif

static const float ILOG2 = (float)1.442695041f;
static const float ILOG10 = (float)0.4342944819f;

void InitMathOpt(void) {
}

#if !defined(FUNCTION_addFLOAT)
void addFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] + Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] + Y[i], _b = X[i + 1] + Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

#if !defined(FUNCTION_sumFLOAT)
float sumFLOAT(const float *X, int n) {
  int i;
  float sum = 0;

  for (i = 0; i < n; i++) {
    sum += X[i];
  }
  return sum;
}
#endif

#if !defined(FUNCTION_addFLOATflex)
void addFLOATflex(const float *IMLRESTRICT X, int incX,
                  const float *IMLRESTRICT Y, int incY,
                  float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] + Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = X[ix] + Y[iy], _b = X[ix + incX] + Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}
#endif

#if !defined(FUNCTION_saddFLOAT)
void saddFLOAT(float b, const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < n; i++) {
    Z[i] = X[i] + b;
  }
}
#endif

#if !defined(FUNCTION_subFLOAT)

void subFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] - Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] - Y[i], _b = X[i + 1] - Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void subFLOATflex(const float *IMLRESTRICT X, int incX,
                  const float *IMLRESTRICT Y, int incY,
                  float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] - Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = X[ix] - Y[iy], _b = X[ix + incX] - Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_multFLOAT)

void multFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] * Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] * Y[i], _b = X[i + 1] * Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

#if !defined(FUNCTION_multFLOATflex)
void multFLOATflex(const float *IMLRESTRICT X, int incX,
                   const float *IMLRESTRICT Y, int incY,
                   float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] * Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = X[ix] * Y[iy], _b = X[ix + incX] * Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}
#endif

#if !defined(FUNCTION_divFLOAT)

void divFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] / Y[i];
  }
  for (; i < n; i += 2) {
    float _a = X[i] / Y[i], _b = X[i + 1] / Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

#if !defined(FUNCTION_divFLOAT_Approx)
void divFLOAT_Approx(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  divFLOAT(X, Y, Z, n);
}
#endif

void divFLOATflex(const float *IMLRESTRICT X, int incX,
                  const float *IMLRESTRICT Y, int incY,
                  float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] / Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = X[ix] / Y[iy], _b = X[ix + incX] / Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_copyFLOAT)

void copyFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  if ((n < 1) || (X == Z)) {
    return;
  }

  if (((Z > X) && (Z < X + n)) || ((Z < X) && (X < Z + n))) {
    memmove(Z, X, sizeof(float) * n);
  } else {
    memcpy(Z, X, sizeof(float) * n);
  }
}
#endif

void moveFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  memmove(Z, X, sizeof(float) * n);
}

void moveINT(const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  memmove(Z, X, sizeof(int) * n);
}

void copyFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)+(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)+(X[ix]), _b = (float)+(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_smulFLOAT)

void smulFLOAT(float a, const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)a * (X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)a * (X[i]), _b = (float)a * (X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void smulFLOATflex(float a, const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)a * (X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)a * (X[ix]), _b = (float)a * (X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_addINT)

void addINT(const int *IMLRESTRICT X, int const *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] + Y[i];
  }
  for (; i < n; i += 2) {
    int _a = X[i] + Y[i], _b = X[i + 1] + Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void addINTflex(const int *IMLRESTRICT X, int incX, int const *IMLRESTRICT Y, int incY, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] + Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = X[ix] + Y[iy], _b = X[ix + incX] + Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_subINT)

void subINT(const int *IMLRESTRICT X, int const *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] - Y[i];
  }
  for (; i < n; i += 2) {
    int _a = X[i] - Y[i], _b = X[i + 1] - Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void subINTflex(const int *IMLRESTRICT X, int incX,
                const int *IMLRESTRICT Y, int incY,
                int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] - Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = X[ix] - Y[iy], _b = X[ix + incX] - Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}
#endif

void multINT(const int *IMLRESTRICT X, int const *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] * Y[i];
  }
  for (; i < n; i += 2) {
    int _a = X[i] * Y[i], _b = X[i + 1] * Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void multINTflex(const int *IMLRESTRICT X, int incX, int const *IMLRESTRICT Y, int incY, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] * Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = X[ix] * Y[iy], _b = X[ix + incX] * Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

void divINT(const int *IMLRESTRICT X, int const *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  for (i = 0; i < (n & 1); i++) {
    Z[i] = X[i] / Y[i];
  }
  for (; i < n; i += 2) {
    int _a = X[i] / Y[i], _b = X[i + 1] / Y[i + 1];
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void divINTflex(const int *IMLRESTRICT X, int incX,
                const int *IMLRESTRICT Y, int incY,
                int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = X[0] / Y[0];
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = X[ix] / Y[iy], _b = X[ix + incX] / Y[iy + incY];
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_smulINT)

void smulINT(int a, const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)a * (X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)a * (X[i]), _b = (int)a * (X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void smulINTflex(int a, const int *IMLRESTRICT X, int incX, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)a * (X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)a * (X[ix]), _b = (int)a * (X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_copyINT)

void copyINT(const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  if ((n < 1) || (X == Z)) {
    return;
  }

  if (((Z > X) && (Z < X + n)) || ((Z < X) && (X < Z + n))) {
    memmove(Z, X, sizeof(int) * n);
  } else {
    memcpy(Z, X, sizeof(int) * n);
  }
}
#endif

void copyINTflex(const int *IMLRESTRICT X, int incX, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)+(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)+(X[ix]), _b = (int)+(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void copyCHAR(const char *IMLRESTRICT X, char *IMLRESTRICT Z, int n) {
  int i = 0;
  for (; i < n; i++) {
    Z[i] = X[i];
  }
}

#if !defined(FUNCTION_minFLOAT)

void minFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)((X[0]) <= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)((X[i]) <= (Y[i]) ? (X[i]) : (Y[i])), _b = (float)((X[i + 1]) <= (Y[i + 1]) ? (X[i + 1]) : (Y[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void minFLOATflex(const float *IMLRESTRICT X, int incX,
                  const float *IMLRESTRICT Y, int incY,
                  float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)((X[0]) <= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)((X[ix]) <= (Y[iy]) ? (X[ix]) : (Y[iy])), _b = (float)((X[ix + incX]) <= (Y[iy + incY]) ? (X[ix + incX]) : (Y[iy + incY]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_findminFLOAT)
float findminFLOAT(const float *X, int n) {
  int i = 0;
  float min = X[0];
  for (; i < n; i++) {
    if (min > X[i])
      min = X[i];
  }
  return min;
}
#endif

#if !defined(FUNCTION_maxFLOAT)

void maxFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)((X[i]) >= (Y[i]) ? (X[i]) : (Y[i])), _b = (float)((X[i + 1]) >= (Y[i + 1]) ? (X[i + 1]) : (Y[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void maxFLOATflex(const float *IMLRESTRICT X, int incX,
                  const float *IMLRESTRICT Y, int incY,
                  float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)((X[ix]) >= (Y[iy]) ? (X[ix]) : (Y[iy])), _b = (float)((X[ix + incX]) >= (Y[iy + incY]) ? (X[ix + incX]) : (Y[iy + incY]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}
#if !defined(FUNCTION_findmaxFLOAT)
float findmaxFLOAT(const float *X, int n) {
  int i = 0;
  float max = X[0];
  for (; i < n; i++) {
    max = (max > X[i] ? max : X[i]);
  }
  return max;
}
#endif

#if !defined(FUNCTION_absFLOAT)

void absFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_fabsf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_fabsf(X[i]), _b = iml_fabsf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void absFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_fabsf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_fabsf(X[ix]), _b = iml_fabsf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_limitFLOAT)

void limitFLOAT(float a, float b, const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)((X[0]) < (a) ? (a) : ((X[0]) > (b) ? (b) : (X[0])));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)((X[i]) < (a) ? (a) : ((X[i]) > (b) ? (b) : (X[i]))),
          _b = (float)((X[i + 1]) < (a) ? (a) : ((X[i + 1]) > (b) ? (b) : (X[i + 1])));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void limitFLOATflex(float a, float b, const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)((X[0]) < (a) ? (a) : ((X[0]) > (b) ? (b) : (X[0])));
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)((X[ix]) < (a) ? (a) : ((X[ix]) > (b) ? (b) : (X[ix]))),
          _b = (float)((X[ix + incX]) < (a) ? (a) : ((X[ix + incX]) > (b) ? (b) : (X[ix + incX])));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_signFLOAT)

void signFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= 0 ? 1.0 : -1.0);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)((X[i]) >= 0 ? 1.0 : -1.0), _b = (float)((X[i + 1]) >= 0 ? 1.0 : -1.0);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void signFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= 0 ? 1.0 : -1.0);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = (float)((X[ix]) >= 0 ? 1.0 : -1.0), _b = (float)((X[ix + incX]) >= 0 ? 1.0 : -1.0);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_minINT)

void minINT(const int *IMLRESTRICT X, const int *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)((X[0]) <= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)((X[i]) <= (Y[i]) ? (X[i]) : (Y[i])), _b = (int)((X[i + 1]) <= (Y[i + 1]) ? (X[i + 1]) : (Y[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void minINTflex(const int *IMLRESTRICT X, int incX, const int *IMLRESTRICT Y, int incY, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)((X[0]) <= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)((X[ix]) <= (Y[iy]) ? (X[ix]) : (Y[iy])), _b = (int)((X[ix + incX]) <= (Y[iy + incY]) ? (X[ix + incX]) : (Y[iy + incY]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_maxINT)

void maxINT(const int *IMLRESTRICT X, const int *IMLRESTRICT Y, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)((X[0]) >= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)((X[i]) >= (Y[i]) ? (X[i]) : (Y[i])), _b = (int)((X[i + 1]) >= (Y[i + 1]) ? (X[i + 1]) : (Y[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void maxINTflex(const int *IMLRESTRICT X, int incX, const int *IMLRESTRICT Y, int incY, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iy = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)((X[0]) >= (Y[0]) ? (X[0]) : (Y[0]));
    i = 1;
    ix = incX;
    iy = incY;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)((X[ix]) >= (Y[iy]) ? (X[ix]) : (Y[iy])), _b = (int)((X[ix + incX]) >= (Y[iy + incY]) ? (X[ix + incX]) : (Y[iy + incY]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iy += 2 * incY;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_absINT)

void absINT(const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)abs(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)abs(X[i]), _b = (int)abs(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void absINTflex(const int *IMLRESTRICT X, int incX, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)abs(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)abs(X[ix]), _b = (int)abs(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_limitINT)

void limitINT(int a, int b, const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)((X[0]) < (a) ? (a) : ((X[0]) > (b) ? (b) : (X[0])));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)((X[i]) < (a) ? (a) : ((X[i]) > (b) ? (b) : (X[i]))),
        _b = (int)((X[i + 1]) < (a) ? (a) : ((X[i + 1]) > (b) ? (b) : (X[i + 1])));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void limitINTflex(int a, int b, const int *IMLRESTRICT X, int incX, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)((X[0]) < (a) ? (a) : ((X[0]) > (b) ? (b) : (X[0])));
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)((X[ix]) < (a) ? (a) : ((X[ix]) > (b) ? (b) : (X[ix]))),
        _b = (int)((X[ix + incX]) < (a) ? (a) : ((X[ix + incX]) > (b) ? (b) : (X[ix + incX])));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_signINT)

void signINT(const int *IMLRESTRICT X, int *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (int)((X[0]) >= 0 ? 1.0 : -1.0);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    int _a = (int)((X[i]) >= 0 ? 1.0 : -1.0), _b = (int)((X[i + 1]) >= 0 ? 1.0 : -1.0);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void signINTflex(const int *IMLRESTRICT X, int incX, int *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (int)((X[0]) >= 0 ? 1.0 : -1.0);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    int _a = (int)((X[ix]) >= 0 ? 1.0 : -1.0), _b = (int)((X[ix + incX]) >= 0 ? 1.0 : -1.0);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_floorFLOAT)

void floorFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_floorf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_floorf(X[i]), _b = iml_floorf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void floorFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_floorf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_floorf(X[ix]), _b = iml_floorf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

#if !defined(FUNCTION_ceilFLOAT)

void ceilFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_ceilf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_ceilf(X[i]), _b = iml_ceilf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void ceilFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_ceilf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_ceilf(X[ix]), _b = iml_ceilf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

int ceillog2(int a) {
  int b = 0;

  a--;
  while (a > 0) {
    b++;
    a = a >> 1;
  }

  return b;
}

#if !defined(FUNCTION_nintFLOAT)

void nintFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_floorf(X[0] + 0.5f);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_floorf(X[i] + 0.5f), _b = iml_floorf(X[i + 1] + 0.5f);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}
#endif

void nintFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_floorf(X[0] + 0.5f);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_floorf(X[ix] + 0.5f), _b = iml_floorf(X[ix + incX] + 0.5f);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void truncFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= 0.0f ? iml_floorf(X[0]) : -iml_floorf(-X[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = (float)((X[i]) >= 0.0f ? iml_floorf(X[i]) : -iml_floorf(-X[i])),
          _b = (float)((X[i + 1]) >= 0.0f ? iml_floorf(X[i + 1]) : -iml_floorf(-X[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void truncFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = (float)((X[0]) >= 0.0f ? iml_floorf(X[0]) : -iml_floorf(-X[0]));
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = ((X[ix]) >= 0.0f ? iml_floorf(X[ix]) : -iml_floorf(-X[ix])),
          _b = ((X[ix + incX]) >= 0.0f ? iml_floorf(X[ix + incX]) : -iml_floorf(-X[ix + incX]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void roundFLOAT2FLOAT16(const float *IMLRESTRICT X, float *IMLRESTRICT Y, int n) {
  int i = 0;
  for (; i < n; i++) {
    if (X[i] > 32767.0f)
      Y[i] = 32767;
    else if (X[i] < -32768.0f)
      Y[i] = -32768;
    else if (X[i] > 0.0f)
      Y[i] = (signed short)(X[i] + 0.5f);
    else if (X[i] <= 0.0f)
      Y[i] = (signed short)(X[i] - 0.5f);

    Y[i] = (float)Y[i];
  }
}

void roundFLOAT2INT(const float *IMLRESTRICT X, int *IMLRESTRICT Y, int n) {
  int i = 0;
  for (; i < n; i++) {
    if (X[i] > 0.0f)
      Y[i] = (int)(X[i] + 0.5f);
    else if (X[i] <= 0.0f)
      Y[i] = (int)(X[i] - 0.5f);
  }
}

void roundFLOAT2SHORT(const float *IMLRESTRICT X, signed short *IMLRESTRICT Y, int n) {
  int i = 0;
  for (; i < n; i++) {
    if (X[i] > 32767.0f)
      Y[i] = 32767;
    else if (X[i] < -32768.0f)
      Y[i] = -32768;
    else if (X[i] > 0.0f)
      Y[i] = (signed short)(X[i] + 0.5f);
    else if (X[i] <= 0.0f)
      Y[i] = (signed short)(X[i] - 0.5f);
  }
}

void sinFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_sinf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_sinf(X[i]), _b = iml_sinf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void sinFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_sinf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_sinf(X[ix]), _b = iml_sinf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void cosFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_cosf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_cosf(X[i]), _b = iml_cosf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void cosFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_cosf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_cosf(X[ix]), _b = iml_cosf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void expFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_expf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_expf(X[i]), _b = iml_expf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void expFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_expf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_expf(X[ix]), _b = iml_expf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void spowFLOAT(float a, const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_powf(X[0], a);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_powf(X[i], a), _b = iml_powf(X[i + 1], a);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void spowFLOATflex(float a, const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_powf(X[0], a);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_powf(X[ix], a), _b = iml_powf(X[ix + incX], a);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void logFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_logf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_logf(X[i]), _b = iml_logf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void logFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_logf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_logf(X[ix]), _b = iml_logf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void log2FLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_logf(X[0]) * ILOG2;
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_logf(X[i]) * ILOG2, _b = iml_logf(X[i + 1]) * ILOG2;
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void log2FLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_logf(X[0]) * ILOG2;
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_logf(X[ix]) * ILOG2, _b = iml_logf(X[ix + incX]) * ILOG2;
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void log10FLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_logf(X[0]) * ILOG10;
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_logf(X[i]) * ILOG10, _b = iml_logf(X[i + 1]) * ILOG10;
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void log10FLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_logf(X[0]) * ILOG10;
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_logf(X[ix]) * ILOG10, _b = iml_logf(X[ix + incX]) * ILOG10;
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void alogbFLOAT(float a, float b, const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = a * iml_logf(b * (X[0]));
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = a * iml_logf(b * (X[i])), _b = a * iml_logf(b * (X[i + 1]));
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void alogbFLOATflex(float a, float b, const float *IMLRESTRICT X, int incX,
                    float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = a * iml_logf(b * (X[0]));
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = a * iml_logf(b * (X[ix])), _b = a * iml_logf(b * (X[ix + incX]));
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

void sqrtFLOAT(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i;
  if (n & 1) {
    Z[0] = iml_sqrtf(X[0]);
    i = 1;
  } else
    i = 0;
  for (; i < n; i += 2) {
    float _a = iml_sqrtf(X[i]), _b = iml_sqrtf(X[i + 1]);
    Z[i] = _a;
    Z[i + 1] = _b;
  }
}

void sqrtFLOATflex(const float *IMLRESTRICT X, int incX, float *IMLRESTRICT Z, int incZ, int n) {
  int i = 0, ix = 0, iz = 0;
  if (n & 1) {
    Z[0] = iml_sqrtf(X[0]);
    i = 1;
    ix = incX;
    iz = incZ;
  }
  for (; i < n; i += 2) {
    float _a = iml_sqrtf(X[ix]), _b = iml_sqrtf(X[ix + incX]);
    Z[iz] = _a;
    Z[iz + incZ] = _b;
    ix += 2 * incX;
    iz += 2 * incZ;
  }
}

float dotFLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, int n) {
  float acc = 0.0f;
  int i;

  if (n) {
    acc = X[0] * Y[0];
  }

  for (i = 1; i < n; i++) acc += X[i] * Y[i];
  return acc;
}

float dotFLOATflex(const float *IMLRESTRICT X, int incX,
                   const float *IMLRESTRICT Y, int incY, int n) {
  float acc = 0.0f;
  int i, iX = incX, iY = incY;

  if (n) {
    acc = X[0] * Y[0];
  }

  for (i = 1; i < n; i++) {
    acc += X[iX] * Y[iY];
    iX += incX;
    iY += incY;
  }
  return acc;
}

float dist2FLOAT(const float *IMLRESTRICT X, const float *IMLRESTRICT Y, int n) {
  float t, acc;
  int i;

  t = X[0] - Y[0];
  acc = t * t;

  for (i = 1; i < n; i++) {
    t = X[i] - Y[i];
    acc += t * t;
  }
  return acc;
}

float dist2FLOATflex(const float *IMLRESTRICT X, int incX,
                     const float *IMLRESTRICT Y, int incY, int n) {
  float t, acc = 0.0f;
  int i = 1, ix = incX, iy = incY;

  if (n) {
    t = X[0] - Y[0];
    acc = t * t;
  }

  for (i = 1; i < n; i++) {
    t = X[ix] - Y[iy];
    acc += t * t;
    ix += incX;
    iy += incY;
  }
  return acc;
}

float norm2FLOAT(const float *IMLRESTRICT X, int n) {
  float acc = 0.0f;
  int i;

  if (n) {
    acc = X[0] * X[0];
  }

  for (i = 1; i < n; i++) acc += X[i] * X[i];
  return acc;
}

void norm2FCOMPLEX(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i = 0, j = 0;

  for (; j < n; i += 2, j++) {
    Z[j] = iml_sqrtf(X[i] * X[i] + X[i + 1] * X[i + 1]);
  }
}

void rad2FCOMPLEX(const float *IMLRESTRICT X, float *IMLRESTRICT Z, int n) {
  int i = 0;
  if (n & 1) {
    Z[0] = X[0] * X[0] + X[1] * X[1];
    i += 1;
  }
  if (n & 2) {
    float _a = X[2 * i] * X[2 * i] + X[2 * i + 1] * X[2 * i + 1];
    float _b = X[2 * i + 2] * X[2 * i + 2] + X[2 * i + 3] * X[2 * i + 3];
    Z[i] = _a;
    Z[i + 1] = _b;
    i += 2;
  }
  for (; i < n; i += 4) {
    float _a = X[2 * i] * X[2 * i] + X[2 * i + 1] * X[2 * i + 1];
    float _b = X[2 * i + 2] * X[2 * i + 2] + X[2 * i + 3] * X[2 * i + 3];
    float _c = X[2 * i + 4] * X[2 * i + 4] + X[2 * i + 5] * X[2 * i + 5];
    float _d = X[2 * i + 6] * X[2 * i + 6] + X[2 * i + 7] * X[2 * i + 7];
    Z[i] = _a;
    Z[i + 1] = _b;
    Z[i + 2] = _c;
    Z[i + 3] = _d;
  }
}

float iisml_randomSign(unsigned int *seed) {
  float sign = 0.f;
  *seed = ((*seed) * 69069) + 5;
  if (((*seed) & 0x10000) > 0) {
    sign = -1.f;
  } else {
    sign = +1.f;
  }
  return sign;
}

void quantFLOATtoUINT(float a, float b, const float *IMLRESTRICT X, unsigned int *IMLRESTRICT Z, int n) {
  int i = 0;
  if (n & 1) {
    Z[0] = (unsigned int)(a + b * X[i]);
    i++;
  }
  if (n & 2) {
    float _a, _b;
    _a = (a + b * X[i]);
    _b = (a + b * X[i + 1]);
    Z[i] = (unsigned int)_a;
    Z[i + 1] = (unsigned int)_b;
    i += 2;
  }
  for (; i < n; i += 4) {
    float t1 = a + b * X[i];
    float t2 = a + b * X[i + 1];
    float t3 = a + b * X[i + 2];
    float t4 = a + b * X[i + 3];
    Z[i] = (unsigned int)(t1);
    Z[i + 1] = (unsigned int)(t2);
    Z[i + 2] = (unsigned int)(t3);
    Z[i + 3] = (unsigned int)(t4);
  }
}

#if !defined(FUNCTION_setFLOAT)
void setFLOAT(float a, float X[], int n) {
  int i;
  for (i = 0; i < n; i++) X[i] = a;
}
#endif

void setFLOATflex(float a, float X[], int incX, int n) {
  int i, ix = 0;
  for (i = 0; i < n; i++) {
    X[ix] = a;
    ix += incX;
  }
}

void setINT(int a, int X[], int n) {
  int i;
  for (i = 0; i < n; i++) X[i] = a;
}

void setINTflex(int a, int X[], int incX, int n) {
  int i, ix = 0;
  for (i = 0; i < n; i++) {
    X[ix] = a;
    ix += incX;
  }
}

void smultFLOATip(float a, float *X, int n) {
  int i;
  for (i = 0; i < n; i++) X[i] *= a;
}

void smultINTip(int a, int *X, int n) {
  int i;
  for (i = 0; i < n; i++) X[i] *= a;
}

void shellsortFLOAT(float *in, int n) {
  float v;
  int i, j;
  int inc = 1;

  do
    inc = 3 * inc + 1;
  while (inc <= n);

  do {
    inc = inc / 3;
    for (i = inc + 1; i <= n; i++) {
      v = in[i - 1];
      j = i;
      while (in[j - inc - 1] > v) {
        in[j - 1] = in[j - inc - 1];
        j -= inc;
        if (j <= inc)
          break;
      }
      in[j - 1] = v;
    }
  } while (inc > 1);
}

void shellsortINT(int *in, int n) {
  int i, j, v;
  int inc = 1;

  do
    inc = 3 * inc + 1;
  while (inc <= n);

  do {
    inc = inc / 3;
    for (i = inc + 1; i <= n; i++) {
      v = in[i - 1];
      j = i;
      while (in[j - inc - 1] > v) {
        in[j - 1] = in[j - inc - 1];
        j -= inc;
        if (j <= inc)
          break;
      }
      in[j - 1] = v;
    }
  } while (inc > 1);
}
#endif
