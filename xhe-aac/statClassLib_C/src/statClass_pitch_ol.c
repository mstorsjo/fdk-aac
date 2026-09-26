
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
#include <string.h>
#include <math.h>
#include "mathlib.h"
#include "statClass_pitch_ol.h"
#include "statClasslib.h"
#include "iisutillib.h"

#define L_FRAME 256
#define L_SUBFR 64

#define OPL_DECIM 2

#define PIT_MIN 34
#define PIT_FR_HALF_8b 92

#define PIT_FR1_8b 92
#define PIT_MAX 231
#define PIT_SHARP 0.85f
#define L_INTERPOL2 16
#define L_INTERPOL (L_INTERPOL2 + 1)

#define NSECT 4
#define NHFR 3
#define PIT_MIN2 20

#define THR_relE -11.0f

#define THRES0 1.17f
#define DELTA0 2.0f
#define STEP 1.0f

#define THRES1 0.4f
#define DELTA1 14
#define THRES3 0.7f

#define CORR_TH0 0.4f
#define CORR_TH1 0.5f

#define LEN_X ((PIT_MAX / OPL_DECIM) - (PIT_MIN2 / OPL_DECIM) + 1)

#define COH_FAC 1.4f

#define L_FIR 5
#define L_MEM (L_FIR - 2)

#define OLD_WSP_SIZE 231
#define WSP_SIZE 384
#define WSP_MEM_SIZE 500

static short const nb_sect[3] = {4, 4, 3};
static short const len[4] = {40, 40, 62, 115};
static short const pit_max[4] = {16, 31, 61, 115};
static short const sec_length[4] = {7, 15, 30, 54};
static float const h_fir[5] = {0.13f, 0.23f, 0.28f, 0.23f, 0.13f};

static void lp_decim2(const float x[], float y[], const short l, float *mem);
static void pitch_neighbour(const short sect0, const short pitch_tmp[], short pitch[NHFR][NSECT],
                            const float corr_tmp[], float corr[3][NSECT], const float thres1[3]);
static void find_mult(float *fac, const short pitch0, const short pitch1, const short pit_max0,
                      float *corr, float delta, const float step);
static short pitch_coherence(const short pitch0, const short pitch1, const float fac_max,
                             const short diff_max);

void STATCLASS_pitch_ol_init(float *old_thres,
                             short *old_pitch,
                             short *delta_pit);

void STATCLASS_pitch_ol(short pitch[3],
                        float voicing[3],
                        short *old_pitch,
                        float corr_shift,
                        float *old_thres,
                        short *delta_pit,
                        float *st_old_wsp,
                        const float *wsp,
                        float mem_decim2[3],
                        const float relE);

typedef struct _statclass_pitch {
  float oldThres;
  short oldPitch;
  short deltaPit;

  SCFLOAT *oldWsp;
  SCFLOAT memDecim[3];

  float corrShift;
  float relE;

  float *tmpWsp;
  float *tmpOldWsp;
  float tmpMemDecim[3];

} STATCLASS_PITCH;

static short STATCLASS_find_max(
    const float *vec,
    const short lvec) {
  short j, ind;
  float tmp;

  ind = 0;
  tmp = vec[0];

  for (j = 1; j < lvec; j++) {
    if (vec[j] > tmp) {
      ind = j;
      tmp = vec[j];
    }
  }

  return ind;
}

void STATCLASS_pitch_ol_init(
    float *old_thres,
    short *old_pitch,
    short *delta_pit) {
  *old_thres = 0.0f;
  *old_pitch = 0;
  *delta_pit = 0;
}

void STATCLASS_pitch_ol(
    short pitch[3],
    float voicing[3],
    short *old_pitch,
    float corr_shift,
    float *old_thres,
    short *delta_pit,
    float *st_old_wsp,
    const float *wsp,
    float mem_decim2[3],
    const float relE) {
  float old_wsp2[(PIT_MAX + L_FRAME + 2 * L_SUBFR) / OPL_DECIM], *wsp2;

  float tmp_mem[3], scale1[2 * DELTA1 - 1] = {0};
  float scaled_buf[LEN_X + 2 * (DELTA1 - 1)] = {0};
  float cor_buf[LEN_X], *pt1, *pt2, *pt3, *pt_cor0, *pt_cor1, *pt_cor2;
  float thres1[3];
  short diff, cnt, ind, offset, coh_flag, coh_flag1;

  short i, j, k, pit_min, sect0, old_tmp, old_tmp1, len_x;
  short pitchX[NHFR][NSECT], pitch_tmp[NHFR], tmp_buf[NHFR + 1];
  float enr, enr0[NSECT], enr1, fac;
  float scaledX[NHFR][NSECT], corX[NHFR][NSECT], cor_tmp[NHFR], cor_mean;

  if ((*old_pitch > 24) || (*old_thres < 0.1)) {
    pit_min = PIT_MIN / OPL_DECIM;
    sect0 = 1;
  } else {
    pit_min = PIT_MIN2 / OPL_DECIM;
    sect0 = 0;
  }

  len_x = ((PIT_MAX / OPL_DECIM) - pit_min + 1);

  wsp2 = old_wsp2 + (PIT_MAX / OPL_DECIM);
  moveFLOAT(st_old_wsp, old_wsp2, PIT_MAX / OPL_DECIM);

  lp_decim2(wsp, wsp2, L_FRAME, mem_decim2);

  moveFLOAT(mem_decim2, tmp_mem, 3);
  lp_decim2(&wsp[L_FRAME], &wsp2[L_FRAME / OPL_DECIM], 2 * L_SUBFR, tmp_mem);

  moveFLOAT(&old_wsp2[L_FRAME / OPL_DECIM], st_old_wsp, PIT_MAX / OPL_DECIM);

  corr_shift *= 0.5f;

  memset(scaled_buf, 0, sizeof(float) * (DELTA1 - 1));
  memset(scaled_buf + (DELTA1 - 1) + len_x, 0, sizeof(float) * (DELTA1 - 1));

  pt1 = scale1 + DELTA1 - 1;
  pt2 = pt1;
  for (i = 0; i < DELTA1; i++) {
    *pt1 = (-(*old_thres) / DELTA1 * i + *old_thres + 1.0f);
    *pt2-- = *pt1++;
  }

  old_tmp = *old_pitch + *delta_pit;

  if (old_tmp > PIT_MAX / OPL_DECIM) {
    old_tmp = PIT_MAX / OPL_DECIM;
  }
  if (old_tmp < pit_min) {
    old_tmp = pit_min;
  }

  old_tmp1 = old_tmp + *delta_pit;
  if (old_tmp1 > PIT_MAX / OPL_DECIM) {
    old_tmp1 = PIT_MAX / OPL_DECIM;
  }

  if (old_tmp1 < pit_min) {
    old_tmp1 = pit_min;
  }

  pt_cor0 = scaled_buf + DELTA1 - 1;

  pt_cor2 = pt_cor0 - pit_min + old_tmp;

  for (i = 0; i < NHFR; i++) {
    pt1 = wsp2 + i * 2 * (L_SUBFR / OPL_DECIM);
    pt2 = pt1 - pit_min;
    pt3 = pt1;
    enr = 0.01f;
    pt_cor1 = pt_cor0;

    for (j = sect0; j < nb_sect[i]; j++) {
      while (pt3 < pt1 + len[j]) {
        enr += *pt3 * *pt3;
        pt3++;
      }

      enr0[j] = enr;

      while (pt2 >= pt1 - pit_max[j]) {
        *pt_cor1++ = dotFLOAT(pt1, pt2--, len[j]);
      }
    }

    moveFLOAT(pt_cor0, cor_buf, len_x);

    pt_cor1 = pt_cor2 - (DELTA1 - 1);
    pt2 = scale1;

    for (k = 0; k < 2 * DELTA1 - 1; k++) {
      *pt_cor1++ *= (*pt2++);
    }

    pt_cor2 = pt_cor0 - pit_min + old_tmp1;

    pt_cor1 = pt_cor0;
    offset = 0;

    for (j = sect0; j < nb_sect[i]; j++) {
      ind = STATCLASS_find_max(pt_cor1, sec_length[j]) + offset;

      pitchX[i][j] = ind + pit_min;

      pt2 = pt1 - pitchX[i][j];
      enr1 = dotFLOAT(pt2, pt2, len[j]) + 0.01f;
      enr1 = 1.f / (float)sqrt(enr0[j] * enr1);
      corX[i][j] = cor_buf[ind] * enr1;
      scaledX[i][j] = pt_cor0[ind] * enr1;

      pt_cor1 += sec_length[j];

      offset += sec_length[j];
    }
  }

  pitchX[NHFR - 1][NSECT - 1] = pitchX[NHFR - 2][NSECT - 1];
  corX[NHFR - 1][NSECT - 1] = corX[NHFR - 2][NSECT - 1];
  scaledX[NHFR - 1][NSECT - 1] = scaledX[NHFR - 2][NSECT - 1];

  for (i = 0; i < 2; i++) {
    fac = THRES0;

    find_mult(&fac, pitchX[i][2], pitchX[i][3], pit_max[3], &scaledX[i][2], DELTA0, STEP);
    find_mult(&fac, pitchX[i][1], pitchX[i][2], pit_max[2], &scaledX[i][1], DELTA0, STEP);
  }

  fac = THRES0;

  find_mult(&fac, pitchX[i][2], pitchX[i][3], pit_max[3], &scaledX[i][2], 2.0f, 2.0f);
  find_mult(&fac, pitchX[i][1], pitchX[i][2], pit_max[2], &scaledX[i][1], DELTA0, STEP);

  for (i = 0; i < NHFR; i++) {
    ind = STATCLASS_find_max(scaledX[i] + sect0, (short)(NSECT - sect0));
    ind += sect0;
    pitch_tmp[i] = pitchX[i][ind];
    cor_tmp[i] = corX[i][ind];
    cor_tmp[i] += corr_shift;

    if (cor_tmp[i] > 1.0f) {
      cor_tmp[i] = 1.0f;
    }

    thres1[i] = THRES1 * cor_tmp[i];
  }

  pitch_neighbour(sect0, pitch_tmp, pitchX, cor_tmp, scaledX, thres1);

  scaledX[NHFR - 1][NSECT - 1] = scaledX[NHFR - 2][NSECT - 1];

  for (i = 0; i < NHFR; i++) {
    ind = STATCLASS_find_max(scaledX[i] + sect0, (NSECT - sect0));
    ind += sect0;

    pitch[i] = pitchX[i][ind];
    voicing[i] = corX[i][ind];
  }

  cor_mean = 0.5f * (voicing[0] + voicing[1]) + corr_shift;

  if (cor_mean > 1.0f) {
    cor_mean = 1.0f;
  }

  coh_flag = pitch_coherence((short)pitch[0], (short)pitch[1], COH_FAC, DELTA1);
  coh_flag1 = pitch_coherence((short)pitch[0], (short)*old_pitch, COH_FAC, DELTA1);

  if ((coh_flag == 0) || (coh_flag1 == 0) || (cor_mean < CORR_TH0) || (relE < THR_relE)) {
    *old_thres = 0.0f;
  } else {
    *old_thres += (0.16f * cor_mean);
  }

  if (*old_thres > THRES3) {
    *old_thres = THRES3;
  }

  tmp_buf[0] = *old_pitch;

  for (i = 0; i < NHFR; i++) {
    tmp_buf[i + 1] = pitch[i];
  }

  *delta_pit = 0;
  cnt = 0;

  for (i = 0; i < NHFR; i++) {
    diff = tmp_buf[i + 1] - tmp_buf[i];

    coh_flag = pitch_coherence((short)tmp_buf[i], (short)tmp_buf[i + 1], COH_FAC, DELTA1);

    if (coh_flag != 0) {
      *delta_pit += diff;
      cnt++;
    }
  }

  if (cnt == 2) {
    *delta_pit /= 2;
  }

  if (cnt == 3) {
    *delta_pit /= 3;
  }

  *old_pitch = pitch[1];

  for (i = 0; i < NHFR; i++) {
    pitch[i] *= OPL_DECIM;

    if (sect0 == 0) {
      if (pitch[i] <= PIT_MIN) {
        pitch[i] *= 2;
      }
    }
  }

  return;
}

static void lp_decim2(
    const float x[],
    float y[],
    const short l,
    float *mem) {
  float temp, *p_x, x_buf[L_FRAME + L_MEM];
  float h[L_FIR] = {0};
  short i, j, k;

  p_x = x_buf;
  for (i = 0; i < L_MEM; i++) {
    *p_x++ = mem[i];
  }

  for (i = 0; i < l; i++) {
    *p_x++ = x[i];
  }

  for (i = 0; i < L_MEM; i++) {
    mem[i] = x[l - L_MEM + i];
  }

  for (i = 0; i < L_FIR; i++) {
    h[i] = h_fir[i];
  }

  for (i = 0, j = 0; i < l; i += 2, j++) {
    p_x = &x_buf[i];
    temp = 0.0f;

    for (k = 0; k < L_FIR; k++) {
      temp += *p_x++ * h[k];
    }

    y[j] = temp;
  }

  return;
}

static void find_mult(
    float *fac,
    const short pitch0,
    const short pitch1,
    const short pit_max0,
    float *corr,
    float delta,
    const float step) {
  short pit_min;

  pit_min = 2 * pitch0;

  while (pit_min <= pit_max0 + (short)delta) {
    if (abs(pit_min - pitch1) <= (short)delta) {
      *corr *= *fac;
      *fac *= THRES0;
    }

    pit_min += pitch0;
    delta += step;
  }
}

static void pitch_neighbour(
    const short sect0,
    const short pitch_tmp[],
    short pitch[NHFR][NSECT],
    const float corr_tmp[],
    float corr[3][NSECT],
    const float thres1[3]) {
  short coh_flag;
  int delta, i, j, k, K;

  for (k = sect0; k < NSECT; k++) {
    if (k == (NSECT - 1)) {
      K = 2;
    } else {
      K = 3;
    }

    for (i = 0; i < K; i++) {
      for (j = 0; j < K; j++) {
        if (j != i) {
          if (corr_tmp[j] >= CORR_TH1) {
            delta = abs(pitch[i][k] - pitch_tmp[j]);

            coh_flag = pitch_coherence((short)pitch[i][k], (short)pitch_tmp[j], COH_FAC, DELTA1);

            if (coh_flag != 0) {
              corr[i][k] *= (-thres1[j] / DELTA1 * delta + thres1[j] + 1.0f);
            }
          }
        }
      }
    }
  }
}

static short pitch_coherence(
    const short pitch0,
    const short pitch1,
    const float fac_max,
    const short diff_max) {
  short smaller, larger;

  if (pitch1 < pitch0) {
    smaller = pitch1;
    larger = pitch0;
  } else {
    smaller = pitch0;
    larger = pitch1;
  }

  if (((float)larger < fac_max * (float)smaller) && ((larger - smaller) < diff_max)) {
    return 1;
  } else {
    return 0;
  }
}

STATCLASS_ERROR_CODE STATCLASS_PitchOl_Open(HANDLE_STATCLASS_PITCH *hPitch) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == hPitch) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      if (NULL == (*hPitch = (STATCLASS_PITCH *)iisCalloc(1, sizeof(STATCLASS_PITCH)))) {
        error = STATCLASS_MEMORY_ALLOC_ERROR;
      } else {
        (*hPitch)->corrShift = 1.0f;
        (*hPitch)->deltaPit = 0;
        (*hPitch)->oldPitch = 0;
        (*hPitch)->oldThres = 0.0f;
        (*hPitch)->relE = 0.0f;
      }
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hPitch)->oldWsp = (SCFLOAT *)iisCalloc(OLD_WSP_SIZE, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPitch)->tmpOldWsp = (float *)iisCalloc(OLD_WSP_SIZE, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPitch)->tmpWsp = (float *)iisCalloc(WSP_SIZE, sizeof(float)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    STATCLASS_pitch_ol_init(&(*hPitch)->oldThres,
                            &(*hPitch)->oldPitch,
                            &(*hPitch)->deltaPit);
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_PitchOl_Close(HANDLE_STATCLASS_PITCH *hPitch) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPitch)->oldWsp)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPitch)->oldWsp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPitch)->tmpOldWsp)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPitch)->tmpOldWsp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hPitch)->tmpWsp)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hPitch)->tmpWsp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hPitch) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hPitch);
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_PitchOl_Advance(HANDLE_STATCLASS_PITCH hPitch,
                                               short pitch[3],
                                               float voicing[3],
                                               SCFLOAT *wsp) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  float *pWspTmp = NULL;
  float *pWspOldTmp = NULL;
  float memDecimTmp[3];
  int i = 0;

  if (hPitch == NULL) {
    error = STATCLASS_INVALID_POINTER;
  }

  if (STATCLASS_NO_ERROR == error) {
    pWspTmp = hPitch->tmpWsp;
    pWspOldTmp = hPitch->tmpOldWsp;

    for (i = 0; i < WSP_SIZE; i++) {
      pWspTmp[i] = (float)wsp[i];
    }

    for (i = 0; i < OLD_WSP_SIZE; i++) {
      pWspOldTmp[i] = (float)hPitch->oldWsp[i];
    }

    for (i = 0; i < 3; i++) {
      memDecimTmp[i] = (float)hPitch->memDecim[i];
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    STATCLASS_pitch_ol(pitch,
                       voicing,
                       &hPitch->oldPitch,
                       hPitch->corrShift,
                       &hPitch->oldThres,
                       &hPitch->deltaPit,
                       pWspOldTmp,
                       pWspTmp,
                       memDecimTmp,
                       hPitch->relE);
  }

  if (STATCLASS_NO_ERROR == error) {
    for (i = 0; i < OLD_WSP_SIZE; i++) {
      hPitch->oldWsp[i] = (SCFLOAT)pWspOldTmp[i];
    }

    for (i = 0; i < 3; i++) {
      hPitch->memDecim[i] = (SCFLOAT)memDecimTmp[i];
    }
  }

  return error;
}
