
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
#include <stdio.h>

#include "iisLPDEncLib_lpc_Quantize.h"
#include "iisLPDEncLib_vq_Main.h"

#define M 16

static int get_num_prm(int qn1, int qn2) {
  return 2 + ((qn1 > 0) ? 9 : 0) + ((qn2 > 0) ? 9 : 0);
}

void LPDEnc_lpc_Quantize(
    float *LSF,
    float *LSF_Q,
    int lpc0,
    int *index,
    int *nb_indices,
    int *nbbits,
    int nbDiv,
    int bIndepFromLPC0) {
  int i;
  float lsfq[M];
  int *tmp_index, indxt[256], nbits, nbt, nit;
  tmp_index = &index[0];
  *nb_indices = 0;
  *nbbits = 0;

  for (i = 0; i < M; i++) LSF_Q[(nbDiv - 1) * M + i] = 0.0f;

  tmp_index[0] = LPDEnc_vq_EncodeFirstStage(&LSF[(nbDiv - 1) * M], &LSF_Q[(nbDiv - 1) * M]);

  nbt = LPDEnc_vq_EncodeSecondStage(&LSF[(nbDiv - 1) * M],
                                    &LSF_Q[(nbDiv - 1) * M],
                                    &tmp_index[1],
                                    0);
  nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);
  tmp_index += nit;
  *nb_indices += nit;
  *nbbits += 8 + nbt;

  if (lpc0) {
    *tmp_index = 0;
    tmp_index++;
    *nb_indices += 1;
    *nbbits += 1;

    for (i = 0; i < M; i++) LSF_Q[-M + i] = 0.0f;

    tmp_index[0] = LPDEnc_vq_EncodeFirstStage(&LSF[-M], &LSF_Q[-M]);

    nbits = LPDEnc_vq_EncodeSecondStage(&LSF[-M],
                                        &LSF_Q[-M],
                                        &tmp_index[1],
                                        0);
    nbt = 8 + nbits;
    nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

    for (i = 0; i < M; i++) lsfq[i] = LSF_Q[(nbDiv - 1) * M + i];

    nbits = LPDEnc_vq_EncodeSecondStage(&LSF[-M],
                                        &lsfq[0],
                                        indxt,
                                        3);
    if (nbits < nbt) {
      nbt = nbits;
      nit = get_num_prm(indxt[0], indxt[1]);
      tmp_index[-1] = 1;
      for (i = 0; i < M; i++) LSF_Q[-M + i] = lsfq[i];
      for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
    }
    tmp_index += nit;
    *nb_indices += nit;
    *nbbits += nbt;
  }

  *tmp_index = 0;
  tmp_index++;
  *nb_indices += 1;
  *nbbits += 1;

  for (i = 0; i < M; i++) {
    LSF_Q[M + i] = 0.0f;
  }

  tmp_index[0] = LPDEnc_vq_EncodeFirstStage(&LSF[M], &LSF_Q[M]);

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[M],
                                      &LSF_Q[M],
                                      &tmp_index[1],
                                      0);
  nbt = 8 + nbits;
  nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

  for (i = 0; i < M; i++) {
    lsfq[i] = LSF_Q[3 * M + i];
  }

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[M],
                                      &lsfq[0],
                                      indxt,
                                      3);
  if (nbits < nbt) {
    nbt = nbits;
    nit = get_num_prm(indxt[0], indxt[1]);
    tmp_index[-1] = 1;
    for (i = 0; i < M; i++) LSF_Q[M + i] = lsfq[i];
    for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
  }
  tmp_index += nit;
  *nb_indices += nit;
  *nbbits += nbt;

  *tmp_index = 0;
  tmp_index++;
  *nb_indices += 1;

  for (i = 0; i < M; i++) LSF_Q[i] = 0.0f;

  tmp_index[0] = LPDEnc_vq_EncodeFirstStage(&LSF[0], &LSF_Q[0]);

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[0],
                                      &LSF_Q[0],
                                      &tmp_index[1],
                                      0);
  nbt = 2 + 8 + nbits;
  nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

  if (!bIndepFromLPC0) {
    for (i = 0; i < M; i++) lsfq[i] = 0.5f * (LSF_Q[-M + i] + LSF_Q[M + i]);

    nbits = LPDEnc_vq_EncodeSecondStage(&LSF[0],
                                        lsfq,
                                        indxt,
                                        1);

    if (nbits < 10) {
      nbt = 2;
      nit = 0;
      tmp_index[-1] = 1;
      for (i = 0; i < M; i++) LSF_Q[i] = lsfq[i];
    }
  }

  for (i = 0; i < M; i++) lsfq[i] = LSF_Q[M + i];

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[0],
                                      lsfq,
                                      indxt,
                                      2);
  nbits += 1;
  if (nbits < nbt) {
    nbt = nbits;
    nit = get_num_prm(indxt[0], indxt[1]);
    tmp_index[-1] = 2;
    for (i = 0; i < M; i++) LSF_Q[i] = lsfq[i];
    for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
  }
  tmp_index += nit;
  *nb_indices += nit;
  *nbbits += nbt;

  *tmp_index = 0;
  tmp_index++;
  *nb_indices += 1;

  for (i = 0; i < M; i++) {
    LSF_Q[2 * M + i] = 0.0f;
  }

  tmp_index[0] = LPDEnc_vq_EncodeFirstStage(&LSF[2 * M], &LSF_Q[2 * M]);

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[2 * M],
                                      &LSF_Q[2 * M],
                                      &tmp_index[1],
                                      0);
  nbt = 2 + 8 + nbits;
  nit = 1 + get_num_prm(tmp_index[1], tmp_index[2]);

  for (i = 0; i < M; i++) {
    lsfq[i] = 0.5f * (LSF_Q[M + i] + LSF_Q[3 * M + i]);
  }

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[2 * M],
                                      lsfq,
                                      indxt,
                                      1);
  nbits += 1;
  if (nbits < nbt) {
    nbt = nbits;
    nit = get_num_prm(indxt[0], indxt[1]);
    tmp_index[-1] = 1;
    for (i = 0; i < M; i++) LSF_Q[2 * M + i] = lsfq[i];
    for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
  }

  for (i = 0; i < M; i++) {
    lsfq[i] = LSF_Q[M + i];
  }

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[2 * M],
                                      lsfq,
                                      indxt,
                                      2);
  nbits += 3;
  if (nbits < nbt) {
    nbt = nbits;
    nit = get_num_prm(indxt[0], indxt[1]);
    tmp_index[-1] = 2;
    for (i = 0; i < M; i++) LSF_Q[2 * M + i] = lsfq[i];
    for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
  }

  for (i = 0; i < M; i++) {
    lsfq[i] = LSF_Q[3 * M + i];
  }

  nbits = LPDEnc_vq_EncodeSecondStage(&LSF[2 * M],
                                      lsfq,
                                      indxt,
                                      2);
  nbits += 3;
  if (nbits < nbt) {
    nbt = nbits;
    nit = get_num_prm(indxt[0], indxt[1]);
    tmp_index[-1] = 3;
    for (i = 0; i < M; i++) LSF_Q[2 * M + i] = lsfq[i];
    for (i = 0; i < nit; i++) tmp_index[i] = indxt[i];
  }
  *nb_indices += nit;
  *nbbits += nbt;

  return;
}
