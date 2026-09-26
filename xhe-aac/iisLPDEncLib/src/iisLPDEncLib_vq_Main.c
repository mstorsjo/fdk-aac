
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "iisLPDEncLib_vq_RE8.h"
#include "iisLPDComLib_vq_RE8.h"
#include "mathlib.h"
#include "iisLPDEncLib_vq_Main.h"

#define LPC_ORDER 16
#define FREQ_MAX 6400.0f
#define LSF_GAP 50.0f
#define FREQ_DIV 400.0f

static void getFirstStageLSFWeights(
    float *p_lsfq,
    float *p_weight);

static void getFirstStageLSFWeights(
    float *p_lsfq,
    float *p_weight) {
  int i;
  float p_distance[LPC_ORDER + 1];

  p_distance[0] = p_lsfq[0];
  p_distance[LPC_ORDER] = FREQ_MAX - p_lsfq[LPC_ORDER - 1];

  subFLOAT(&p_lsfq[1], p_lsfq, &p_distance[1], LPC_ORDER - 1);

  for (i = 0; i < LPC_ORDER; i++) {
    p_weight[i] = (1.f / p_distance[i]) + (1.f / p_distance[i + 1]);
  }

  return;
}

int LPDEnc_vq_EncodeFirstStage(
    float *p_lsf,
    float *p_lsfq) {
  int idx;
  int order;
  float p_weight[LPC_ORDER];
  float p_differ[LPC_ORDER];
  float distance_min;
  int distance_min_idx;
  float distance;
  float temp;
  const float *pDictionary;
  extern float p_lpdcom_dico_lsf_abs_8b[];

  if (p_lsf == NULL || p_lsfq == NULL) {
    return -1;
  }

  getFirstStageLSFWeights(p_lsf, p_weight);

  subFLOAT(p_lsf, p_lsfq, p_differ, LPC_ORDER);

  distance_min = 1.0e30f;
  distance_min_idx = 0;
  pDictionary = p_lpdcom_dico_lsf_abs_8b;

  for (idx = 0; idx < 256; idx++) {
    distance = 0.f;

    for (order = 0; order < LPC_ORDER; order++) {
      temp = p_differ[order] - (*pDictionary++);
      distance += p_weight[order] * temp * temp;
    }

    if (distance < distance_min) {
      distance_min = distance;
      distance_min_idx = idx;
    }
  }

  pDictionary = &p_lpdcom_dico_lsf_abs_8b[distance_min_idx * LPC_ORDER];

  addFLOAT(p_lsfq, pDictionary, p_lsfq, LPC_ORDER);

  return distance_min_idx;
}

static void vectorQuantizeLSF(
    float *p_nvec,
    int *p_nvecq,
    int *p_qnData,
    int *p_codebookIndex,
    int *pp_kv,
    int Nsv) {
  int i;
  int l;
  int pos;
  int c[8];
  float x1[8];

  pos = Nsv;
  for (l = 0; l < Nsv; l++) {
    for (i = 0; i < 8; i++) {
      x1[i] = p_nvec[l * 8 + i];
      pp_kv[l * 8 + i] = 0;
    }

    LPDCom_vq_GetNearestRE8Vector(x1, c);

    LPDEnc_vq_getAVQindices(
        c,
        &p_qnData[l],
        &p_codebookIndex[l],
        &pp_kv[l * 8]);

    for (i = 0; i < 8; i++) {
      p_nvecq[l * 8 + i] = c[i];
    }

    if (p_qnData[l] > 0) {
      int iii;

      p_qnData[pos++] = p_codebookIndex[l];

      for (iii = 0; iii < 8; iii++) {
        p_qnData[pos++] = pp_kv[l * 8 + iii];
      }
    }
  }
  return;
}

int LPDEnc_vq_EncodeSecondStage(
    float *p_lsf,
    float *p_lsfq,
    int *p_qn_data,
    int mode) {
  int i, nbits;
  float w[LPC_ORDER], x[LPC_ORDER], tmp;
  int nq, xq[LPC_ORDER];
  int p_code_book_index[2] = {0};
  int pp_kv[16] = {0};

  if (p_lsf == NULL || p_lsfq == NULL || p_qn_data == NULL) {
    return -1;
  }

  LPDCom_vq_GetSecondStageLSFWeights(p_lsf, w, 1);

  subFLOAT(p_lsf, p_lsfq, x, LPC_ORDER);
  divFLOAT(x, w, x, LPC_ORDER);
  tmp = norm2FLOAT(x, LPC_ORDER);

  if (tmp < 8.0f) {
    p_qn_data[0] = 0;
    p_qn_data[1] = 0;
    if ((mode == 0) || (mode == 3))
      return (10);
    else if (mode == 1)
      return (2);
    else
      return (6);
  }

  LPDCom_vq_GetSecondStageLSFWeights(p_lsfq, w, mode);

  subFLOAT(p_lsf, p_lsfq, x, LPC_ORDER);

  divFLOAT(x, w, x, LPC_ORDER);

  vectorQuantizeLSF(x, xq, p_qn_data, p_code_book_index, pp_kv, 2);

  for (i = 0; i < LPC_ORDER; i++) p_lsfq[i] += (w[i] * (float)xq[i]);

  nbits = 0;
  for (i = 0; i < 2; i++) {
    nq = p_qn_data[i];

    if ((mode == 0) || (mode == 3)) {
      nbits += (2 + (nq * 4));
      if (nq > 6)
        nbits += nq - 3;
      else if (nq > 4)
        nbits += nq - 4;
      else if (nq == 0)
        nbits += 3;

    } else if (mode == 1) {
      nbits += nq * 5;
      if (nq == 0) nbits += 1;

    } else {
      nbits += (2 + (nq * 4));
      if (nq == 0)
        nbits += 1;
      else if (nq > 4)
        nbits += nq - 3;
    }
  }

  LPDCom_vq_ReorderLSF(p_lsfq, LSF_GAP, LPC_ORDER);

  return (nbits);
}

int LPDEnc_vq_getAVQindicesExtern(
    int *p_Re8In,
    int *p_NumCodebook,
    int *p_IdxCodebook,
    int *p_idxVoronoi) {
  if (p_Re8In == NULL ||
      p_NumCodebook == NULL ||
      p_IdxCodebook == NULL ||
      p_idxVoronoi == NULL) {
    return -1;
  }

  LPDEnc_vq_getAVQindices(p_Re8In,
                          p_NumCodebook,
                          p_IdxCodebook,
                          p_idxVoronoi);
  return 0;
}
