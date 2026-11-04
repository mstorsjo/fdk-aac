
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

#include <stdlib.h>

#include "IISutillib/matrixCalloc.h"
#include "IISutillib/ngsalloc.h"

#if defined MEM_ALIGNMENT && defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
#include <assert.h>
static_assert(MEM_ALIGNMENT_SIZE % sizeof(double) == 0, "MEM_ALIGNMENT_SIZE must be a multiple of sizeof(double)");
static_assert(MEM_ALIGNMENT_SIZE % sizeof(void **) == 0, "MEM_ALIGNMENT_SIZE must be a multiple of sizeof(void**)");
#endif

void **iisCallocMatrix2D(size_t dim1, size_t dim2, size_t size) {
  void **p1;
  size_t i;
  int err = 0;
  size_t tmp;

  if (!dim1 || !dim2) return NULL;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  p1 = iisCalloc(dim1 * sizeof(void *) + tmp, 1);
  *((size_t *)p1) = dim1;
  p1 = p1 + (tmp / sizeof(void **));

  for (i = 0; i < dim1; i++) {
    p1[i] = iisCalloc(dim2, size);
    if (!p1[i]) {
      err = 1;
      break;
    }
  }

  if (err) {
    for (i = 0; i < dim1; i++) {
      if (!p1[i]) break;
      iisFree(p1[i]);
    }

    p1 = p1 - (tmp / sizeof(void **));
    iisFree(p1);

    return NULL;
  }

  return p1;
}

void iisFreeMatrix2D(void **p) {
  size_t i, tmp, dim1;

  if (!p) return;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  dim1 = *((size_t *)(p - (tmp / sizeof(void **))));

  for (i = 0; i < dim1; i++) {
    iisFree(p[i]);
  }

  p = p - (tmp / sizeof(void **));
  iisFree(p);
}

void ***iisCallocMatrix3D(size_t dim1, size_t dim2, size_t dim3, size_t size) {
  void ***p1;
  size_t i, j;
  int err = 0;
  size_t tmp;

  if (!dim1 || !dim2 || !dim3) return NULL;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 2 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  p1 = iisCalloc(dim1 * sizeof(void **) + tmp, 1);
  *((size_t *)p1) = dim1;
  *((size_t *)p1 + 1) = dim2;
  p1 = p1 + (tmp / sizeof(void ***));

  for (i = 0; i < dim1; i++) {
    p1[i] = iisCalloc(dim2, sizeof(void *));
    if (!p1[i]) {
      err = 1;
      break;
    }

    for (j = 0; j < dim2; j++) {
      p1[i][j] = iisCalloc(dim3, size);
      if (!p1[i][j]) {
        err = 1;
        break;
      }
    }

    if (err) break;
  }

  if (err) {
    for (i = 0; i < dim1; i++) {
      if (!p1[i]) break;

      for (j = 0; j < dim2; j++) {
        if (!p1[i][j]) break;
        iisFree(p1[i][j]);
      }

      iisFree(p1[i]);
    }

    p1 = p1 - (tmp / sizeof(void ***));
    iisFree(p1);

    return NULL;
  }

  return p1;
}

void iisFreeMatrix3D(void ***p) {
  size_t i, j, tmp, dim1, dim2;
  void ***base = NULL;

  if (!p) return;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 2 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;

  base = p - (tmp / sizeof(void ***));
  dim1 = *((size_t *)base);
  dim2 = *((size_t *)base + 1);

  for (i = 0; i < dim1; i++) {
    for (j = 0; j < dim2; j++) {
      iisFree(p[i][j]);
    }
    iisFree(p[i]);
  }

  p = base;
  iisFree(p);
}

void ****iisCallocMatrix4D(size_t dim1, size_t dim2, size_t dim3, size_t dim4, size_t size) {
  void ****p1;
  size_t i, j, k;
  int err = 0;
  size_t tmp;

  if (!dim1 || !dim2 || !dim3 || !dim4) return NULL;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 3 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  p1 = iisCalloc(dim1 * sizeof(void ***) + tmp, 1);
  *((size_t *)p1) = dim1;
  *((size_t *)p1 + 1) = dim2;
  *((size_t *)p1 + 2) = dim3;
  p1 = p1 + (tmp / sizeof(void ****));

  for (i = 0; i < dim1; i++) {
    p1[i] = iisCalloc(dim2, sizeof(void **));
    if (!p1[i]) {
      err = 1;
      break;
    }

    for (j = 0; j < dim2; j++) {
      p1[i][j] = iisCalloc(dim3, sizeof(void *));
      if (!p1[i][j]) {
        err = 1;
        break;
      }

      for (k = 0; k < dim3; k++) {
        p1[i][j][k] = iisCalloc(dim4, size);
        if (!p1[i][j][k]) {
          err = 1;
          break;
        }
      }

      if (err) break;
    }

    if (err) break;
  }

  if (err) {
    for (i = 0; i < dim1; i++) {
      if (!p1[i]) break;

      for (j = 0; j < dim2; j++) {
        if (!p1[i][j]) break;

        for (k = 0; k < dim3; k++) {
          if (!p1[i][j][k]) break;
          iisFree(p1[i][j][k]);
        }

        iisFree(p1[i][j]);
      }

      iisFree(p1[i]);
    }

    p1 = p1 - (tmp / sizeof(void ****));
    iisFree(p1);

    return NULL;
  }

  return p1;
}

void iisFreeMatrix4D(void ****p) {
  size_t i, j, k, tmp, dim1, dim2, dim3;
  void ****base = NULL;
  if (!p) return;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 3 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  base = p - (tmp / sizeof(void ****));
  dim1 = *((size_t *)base);
  dim2 = *((size_t *)base + 1);
  dim3 = *((size_t *)base + 2);

  for (i = 0; i < dim1; i++) {
    for (j = 0; j < dim2; j++) {
      for (k = 0; k < dim3; k++) {
        iisFree(p[i][j][k]);
      }
      iisFree(p[i][j]);
    }
    iisFree(p[i]);
  }

  p = base;
  iisFree(p);
}

void *****iisCallocMatrix5D(size_t dim1, size_t dim2, size_t dim3, size_t dim4, size_t dim5, size_t size) {
  void *****p1;
  size_t i, j, k, l;
  int err = 0;
  size_t tmp;

  if (!dim1 || !dim2 || !dim3 || !dim4 || !dim5) return NULL;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 4 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  p1 = iisCalloc(dim1 * sizeof(void ****) + tmp, 1);
  if (!p1) return NULL;

  *((size_t *)p1) = dim1;
  *((size_t *)p1 + 1) = dim2;
  *((size_t *)p1 + 2) = dim3;
  *((size_t *)p1 + 3) = dim4;
  p1 = p1 + (tmp / sizeof(void *****));

  for (i = 0; i < dim1; i++) {
    p1[i] = iisCalloc(dim2, sizeof(void ***));
    if (!p1[i]) {
      err = 1;
      break;
    }

    for (j = 0; j < dim2; j++) {
      p1[i][j] = iisCalloc(dim3, sizeof(void **));
      if (!p1[i][j]) {
        err = 1;
        break;
      }

      for (k = 0; k < dim3; k++) {
        p1[i][j][k] = iisCalloc(dim4, sizeof(void *));
        if (!p1[i][j][k]) {
          err = 1;
          break;
        }

        for (l = 0; l < dim4; l++) {
          p1[i][j][k][l] = iisCalloc(dim5, size);
          if (!p1[i][j][k][l]) {
            err = 1;
            break;
          }
        }

        if (err) break;
      }

      if (err) break;
    }

    if (err) break;
  }

  if (err) {
    for (i = 0; i < dim1; i++) {
      if (!p1[i]) break;

      for (j = 0; j < dim2; j++) {
        if (!p1[i][j]) break;

        for (k = 0; k < dim3; k++) {
          if (!p1[i][j][k]) break;

          for (l = 0; l < dim4; l++) {
            if (!p1[i][j][k][l]) break;
            iisFree(p1[i][j][k][l]);
          }

          iisFree(p1[i][j][k]);
        }

        iisFree(p1[i][j]);
      }

      iisFree(p1[i]);
    }

    p1 = p1 - (tmp / sizeof(void *****));
    iisFree(p1);

    return NULL;
  }

  return p1;
}

void iisFreeMatrix5D(void *****p) {
  size_t i, j, k, l, tmp, dim1, dim2, dim3, dim4;
  void *****base = NULL;
  if (!p) return;

  tmp = MEM_ALIGNMENT_SIZE;
  while (tmp < 4 * sizeof(size_t)) tmp += MEM_ALIGNMENT_SIZE;
  base = p - (tmp / sizeof(void *****));
  dim1 = *((size_t *)base);
  dim2 = *((size_t *)base + 1);
  dim3 = *((size_t *)base + 2);
  dim4 = *((size_t *)base + 3);

  for (i = 0; i < dim1; i++) {
    for (j = 0; j < dim2; j++) {
      for (k = 0; k < dim3; k++) {
        for (l = 0; l < dim4; l++) {
          iisFree(p[i][j][k][l]);
        }
        iisFree(p[i][j][k]);
      }
      iisFree(p[i][j]);
    }
    iisFree(p[i]);
  }

  p = base;
  iisFree(p);
}
