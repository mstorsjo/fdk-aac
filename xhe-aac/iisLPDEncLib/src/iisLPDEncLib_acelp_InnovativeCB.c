
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
#include <string.h>
#include <float.h>

#include "iisLPDEncLib_acelp_InnovativeCB.h"

#define L_SUBFR 64
#define NB_PULSE_MAX 24
#define NPMAXPT ((NB_PULSE_MAX + 4 - 1) / 4)

const unsigned char lpdenc_TableIPosition[36] = {
    0, 1, 2, 3,
    1, 2, 3, 0,
    2, 3, 0, 1,
    3, 0, 1, 2,
    0, 1, 2, 3,
    1, 2, 3, 0,
    2, 3, 0, 1,
    3, 0, 1, 2,
    0, 1, 2, 3};

static void correlateHVec1(float h[], float vec[], unsigned char track,
                           float sign[], float (*rrixix)[16],
                           float cor[], int dn2_pos[],
                           int nb_pulse) {
  short i, j;
  int dn;
  int *dn2;
  float *p0;
  float s;
  dn2 = &dn2_pos[track * 8];
  p0 = rrixix[track];
  for (i = 0; i < nb_pulse; i++) {
    dn = dn2[i];
    s = 0.0F;
    for (j = 0; j < (L_SUBFR - dn); j++) {
      s += h[j] * vec[dn + j];
    }
    cor[dn >> 2] = sign[dn] * s + p0[dn >> 2];
  }
  return;
}
static void correlateHVec2(float h[], float vec[], unsigned char track,
                           float sign[], float (*rrixix)[16],
                           float cor[]) {
  int i, j;
  float *p0;
  float s;
  p0 = rrixix[track];
  for (i = 0; i < 16; i++) {
    s = 0.0F;
    for (j = 0; j < L_SUBFR - track; j++) {
      s += h[j] * vec[track + j];
    }
    cor[i] = s * sign[track] + p0[i];
    track += 4;
  }
  return;
}

static void search2Pulses(int nb_pos_ix,
                          unsigned char track_x,
                          unsigned char track_y,
                          float *ps,
                          float *alp,
                          int *ix,
                          int *iy,
                          float dn[],
                          int *dn2,
                          float cor_x[],
                          float cor_y[],
                          float (*rrixiy)[256]) {
  int x, x2, y, x_save = 0, y_save = 0, i, *pos_x;
  float ps0, alp0;
  float ps1, ps2, sq, sqk;
  float alp1, alp2, alpk;
  float *p1, *p2;
  float s;

  pos_x = &dn2[track_x << 3];

  ps0 = *ps;
  alp0 = *alp;
  sqk = -1.0F;
  alpk = 1.0F;

  for (i = 0; i < nb_pos_ix; i++) {
    x = pos_x[i];
    x2 = x >> 2;

    ps1 = ps0 + dn[x];
    alp1 = alp0 + cor_x[x2];
    p1 = cor_y;
    p2 = &rrixiy[track_x][x2 << 4];
    for (y = track_y; y < L_SUBFR; y += 4) {
      ps2 = ps1 + dn[y];
      alp2 = alp1 + (*p1++) + (*p2++);
      sq = ps2 * ps2;
      s = (alpk * sq) - (sqk * alp2);
      if (s > 0.0F) {
        sqk = sq;
        alpk = alp2;
        y_save = y;
        x_save = x;
      }
    }
  }
  *ps = ps0 + dn[x_save] + dn[y_save];
  *alp = alpk;
  *ix = x_save;
  *iy = y_save;
  return;
}

static void search1Pulse(
    unsigned char track_x,
    unsigned char track_y,
    float *ps,
    float *alp,
    int *ix,
    float dn[],
    float cor_x[],
    float cor_y[]) {
  int x, x_save = 0;
  float ps0, alp0;
  float ps1, sq, sqk;
  float alp1, alpk;
  float s;

  ps0 = *ps;
  alp0 = *alp;
  sqk = -1.0F;
  alpk = 1.0F;

  for (x = track_x; x < L_SUBFR; x += 4) {
    ps1 = ps0 + dn[x];
    alp1 = alp0 + cor_x[x >> 2];
    sq = ps1 * ps1;
    s = (alpk * sq) - (sqk * alp1);
    if (s > 0.0F) {
      sqk = sq;
      alpk = alp1;
      x_save = x;
    }
  }

  if (track_y != track_x) {
    for (x = track_y; x < L_SUBFR; x += 4) {
      ps1 = ps0 + dn[x];
      alp1 = alp0 + cor_y[x >> 2];
      sq = ps1 * ps1;
      s = (alpk * sq) - (sqk * alp1);
      if (s > 0.0F) {
        sqk = sq;
        alpk = alp1;
        x_save = x;
      }
    }
  }

  *ps = ps0 + dn[x_save];
  *alp = alpk;
  *ix = x_save;
  return;
}

static int quantize1PulseNp1Bits(int pos, int N) {
  int mask;
  int index;
  mask = ((1 << N) - 1);

  index = (pos & mask);
  if ((pos & 16) != 0) {
    index += 1 << N;
  }
  return (index);
}

static int quantize2Pulses2Np1Bits(int pos1, int pos2, int N) {
  int mask;
  int index;
  mask = ((1 << N) - 1);

  if (((pos2 ^ pos1) & 16) == 0) {
    if ((pos1 - pos2) <= 0) {
      index = ((pos1 & mask) << N) + (pos2 & mask);
    } else {
      index = ((pos2 & mask) << N) + (pos1 & mask);
    }
    if ((pos1 & 16) != 0) {
      index += 1 << (2 * N);
    }
  } else {
    if (((pos1 & mask) - (pos2 & mask)) <= 0) {
      index = ((pos2 & mask) << N) + (pos1 & mask);
      if ((pos2 & 16) != 0) {
        index += 1 << (2 * N);
      }
    } else {
      index = ((pos1 & mask) << N) + (pos2 & mask);
      if ((pos1 & 16) != 0) {
        index += 1 << (2 * N);
      }
    }
  }
  return (index);
}

static int quantize3Pulses3Np1Bits(int pos1, int pos2, int pos3, int N) {
  int nb_pos;
  int index;
  nb_pos = (1 << (N - 1));

  if (((pos1 ^ pos2) & nb_pos) == 0) {
    index = quantize2Pulses2Np1Bits(pos1, pos2, (N - 1));
    index += (pos1 & nb_pos) << N;
    index += quantize1PulseNp1Bits(pos3, N) << (2 * N);
  } else if (((pos1 ^ pos3) & nb_pos) == 0) {
    index = quantize2Pulses2Np1Bits(pos1, pos3, (N - 1));
    index += (pos1 & nb_pos) << N;
    index += quantize1PulseNp1Bits(pos2, N) << (2 * N);
  } else {
    index = quantize2Pulses2Np1Bits(pos2, pos3, (N - 1));
    index += (pos2 & nb_pos) << N;
    index += quantize1PulseNp1Bits(pos1, N) << (2 * N);
  }
  return (index);
}

static int quantize4Pulses4Np1Bits(int pos1, int pos2, int pos3, int pos4, int N) {
  int nb_pos;
  int index;
  nb_pos = (1 << (N - 1));

  if (((pos1 ^ pos2) & nb_pos) == 0) {
    index = quantize2Pulses2Np1Bits(pos1, pos2, (N - 1));
    index += (pos1 & nb_pos) << N;
    index += quantize2Pulses2Np1Bits(pos3, pos4, N) << (2 * N);
  } else if (((pos1 ^ pos3) & nb_pos) == 0) {
    index = quantize2Pulses2Np1Bits(pos1, pos3, (N - 1));
    index += (pos1 & nb_pos) << N;
    index += quantize2Pulses2Np1Bits(pos2, pos4, N) << (2 * N);
  } else {
    index = quantize2Pulses2Np1Bits(pos2, pos3, (N - 1));
    index += (pos2 & nb_pos) << N;
    index += quantize2Pulses2Np1Bits(pos1, pos4, N) << (2 * N);
  }
  return (index);
}

static int quantize4Pulses4NBits(int pos[], int N) {
  int i, j, k, nb_pos, n_1;
  int posA[4], posB[4];
  int index = 0;
  n_1 = N - 1;
  nb_pos = (1 << n_1);
  i = 0;
  j = 0;
  for (k = 0; k < 4; k++) {
    if ((pos[k] & nb_pos) == 0) {
      posA[i++] = pos[k];
    } else {
      posB[j++] = pos[k];
    }
  }
  switch (i) {
    case 0:
      index = 1 << ((4 * N) - 3);
      index += quantize4Pulses4Np1Bits(posB[0], posB[1], posB[2], posB[3], n_1);
      break;
    case 1:
      index = quantize1PulseNp1Bits(posA[0], n_1) << ((3 * n_1) + 1);
      index += quantize3Pulses3Np1Bits(posB[0], posB[1], posB[2], n_1);
      break;
    case 2:
      index = quantize2Pulses2Np1Bits(posA[0], posA[1], n_1) << ((2 * n_1) + 1);
      index += quantize2Pulses2Np1Bits(posB[0], posB[1], n_1);
      break;
    case 3:
      index = quantize3Pulses3Np1Bits(posA[0], posA[1], posA[2], n_1) << N;
      index += quantize1PulseNp1Bits(posB[0], n_1);
      break;
    case 4:
      index = quantize4Pulses4Np1Bits(posA[0], posA[1], posA[2], posA[3], n_1);
      break;
  }
  index += (i & 3) << ((4 * N) - 2);
  return (index);
}

void LPDEnc_acelp_Correlate(float *x, float *y, float *h) {
  short i, j;
  float s;
  for (i = 0; i < L_SUBFR; i++) {
    s = 0.0F;
    for (j = i; j < L_SUBFR; j++) {
      s += x[j] * h[j - i];
    }
    y[i] = s;
  }
  return;
}

void LPDEnc_acelp_CalculateInnovativeCBGain(float xn[], float y1[], float y2[],
                                            float g_corr[]) {
  float temp1, temp2, temp3;
  float temp4;
  int i;
  temp1 = 0.01F + y2[0] * y2[0];
  temp2 = 0.01F + xn[0] * y2[0];
  temp3 = 0.01F + y1[0] * y2[0];
  temp4 = 0.01F + xn[0] * xn[0];
  temp1 += y2[1] * y2[1];
  temp2 += xn[1] * y2[1];
  temp3 += y1[1] * y2[1];
  temp1 += y2[2] * y2[2];
  temp2 += xn[2] * y2[2];
  temp3 += y1[2] * y2[2];
  temp1 += y2[3] * y2[3];
  temp2 += xn[3] * y2[3];
  temp3 += y1[3] * y2[3];
  for (i = 4; i < L_SUBFR; i += 6) {
    temp1 += y2[i] * y2[i];
    temp2 += xn[i] * y2[i];
    temp3 += y1[i] * y2[i];
    temp1 += y2[i + 1] * y2[i + 1];
    temp2 += xn[i + 1] * y2[i + 1];
    temp3 += y1[i + 1] * y2[i + 1];
    temp1 += y2[i + 2] * y2[i + 2];
    temp2 += xn[i + 2] * y2[i + 2];
    temp3 += y1[i + 2] * y2[i + 2];
    temp1 += y2[i + 3] * y2[i + 3];
    temp2 += xn[i + 3] * y2[i + 3];
    temp3 += y1[i + 3] * y2[i + 3];
    temp1 += y2[i + 4] * y2[i + 4];
    temp2 += xn[i + 4] * y2[i + 4];
    temp3 += y1[i + 4] * y2[i + 4];
    temp1 += y2[i + 5] * y2[i + 5];
    temp2 += xn[i + 5] * y2[i + 5];
    temp3 += y1[i + 5] * y2[i + 5];
  }
  for (i = 1; i < L_SUBFR; i += 1) {
    temp4 += xn[i] * xn[i];
  }
  g_corr[2] = temp1;
  g_corr[3] = -2.0F * temp2;
  g_corr[4] = 2.0F * temp3;
  g_corr[5] = temp4;
  return;
}

void LPDEnc_acelp_UpdateFixedCBTarget(float *x, float *x2, float *y,
                                      float gain) {
  short i;
  for (i = 0; i < L_SUBFR; i++) {
    x2[i] = x[i] - gain * y[i];
  }
}

void LPDEnc_acelp_GetFixedCBIndex(float dn[], float cn[], float H[], float *code,
                                  float y[], int nbbits, int _index[]) {
  float sign[L_SUBFR], vec[L_SUBFR];
  float cor_x[16], cor_y[16], h_buf[4 * L_SUBFR] = {0.0f};
  float rrixix[4][16];
  float rrixiy[4][256];
  float dn2[L_SUBFR];
  int ind[NPMAXPT * 4];
  int codvec[NB_PULSE_MAX] = {0};
  int nbpos[10];
  int pos_max[4];
  int dn2_pos[8 * 4];
  unsigned char ipos[NB_PULSE_MAX];
  int i, j, k, st, pos = 0, index, track, nb_pulse = 0, nbiter = 4;
  int L_index;
  float psk, ps, alpk, alp = 0.0F;
  float val;
  float s, cor;
  float *p0, *p1, *p2, *p3, *psign;
  float *h, *h_inv, *ptr_h1, *ptr_h2, *ptr_hf;
  switch (nbbits) {
    case 16:
      nbiter = 6;
      alp = 2.0;
      nb_pulse = 3;
      nbpos[0] = 8;
      break;
    case 20:
      nbiter = 4;
      alp = 2.0;
      nb_pulse = 4;
      nbpos[0] = 4;
      nbpos[1] = 8;
      break;
    case 28:
      nbiter = 4;
      alp = 1.5;
      nb_pulse = 6;
      nbpos[0] = 4;
      nbpos[1] = 8;
      nbpos[2] = 8;
      break;
    case 36:
      nbiter = 4;
      alp = 1.0;
      nb_pulse = 8;
      nbpos[0] = 4;
      nbpos[1] = 8;
      nbpos[2] = 8;
      break;
    case 44:
      nbiter = 4;
      alp = 1.0;
      nb_pulse = 10;
      nbpos[0] = 4;
      nbpos[1] = 6;
      nbpos[2] = 8;
      nbpos[3] = 8;
      break;
    case 52:
      nbiter = 4;
      alp = 1.0;
      nb_pulse = 12;
      nbpos[0] = 4;
      nbpos[1] = 6;
      nbpos[2] = 8;
      nbpos[3] = 8;
      break;
    case 64:
      nbiter = 3;
      alp = 0.8F;
      nb_pulse = 16;
      nbpos[0] = 4;
      nbpos[1] = 4;
      nbpos[2] = 6;
      nbpos[3] = 6;
      nbpos[4] = 8;
      nbpos[5] = 8;
      break;
  }

  val = (cn[0] * cn[0]) + 1.0F;
  cor = (dn[0] * dn[0]) + 1.0F;
  for (i = 1; i < L_SUBFR; i += 7) {
    val += (cn[i] * cn[i]);
    cor += (dn[i] * dn[i]);
    val += (cn[i + 1] * cn[i + 1]);
    cor += (dn[i + 1] * dn[i + 1]);
    val += (cn[i + 2] * cn[i + 2]);
    cor += (dn[i + 2] * dn[i + 2]);
    val += (cn[i + 3] * cn[i + 3]);
    cor += (dn[i + 3] * dn[i + 3]);
    val += (cn[i + 4] * cn[i + 4]);
    cor += (dn[i + 4] * dn[i + 4]);
    val += (cn[i + 5] * cn[i + 5]);
    cor += (dn[i + 5] * dn[i + 5]);
    val += (cn[i + 6] * cn[i + 6]);
    cor += (dn[i + 6] * dn[i + 6]);
  }
  s = (float)sqrt(cor / val);
  for (j = 0; j < L_SUBFR; j++) {
    cor = (s * cn[j]) + (alp * dn[j]);
    if (cor >= 0.0F) {
      sign[j] = 1.0F;
      vec[j] = -1.0F;
      dn2[j] = cor;
    } else {
      sign[j] = -1.0F;
      vec[j] = 1.0F;
      dn[j] = -dn[j];
      dn2[j] = -cor;
    }
  }

  for (i = 0; i < 4; i++) {
    for (k = 0; k < 8; k++) {
      ps = -1;
      for (j = i; j < L_SUBFR; j += 4) {
        if (dn2[j] > ps) {
          ps = dn2[j];
          pos = j;
        }
      }
      dn2[pos] = (float)k - 8;
      dn2_pos[i * 8 + k] = pos;
    }
    pos_max[i] = dn2_pos[i * 8];
  }

  h = h_buf + L_SUBFR;
  h_inv = h_buf + (3 * L_SUBFR);
  memcpy(h, H, L_SUBFR * sizeof(float));
  h_inv[0] = -h[0];
  h_inv[1] = -h[1];
  h_inv[2] = -h[2];
  h_inv[3] = -h[3];
  for (i = 4; i < L_SUBFR; i += 6) {
    h_inv[i] = -h[i];
    h_inv[i + 1] = -h[i + 1];
    h_inv[i + 2] = -h[i + 2];
    h_inv[i + 3] = -h[i + 3];
    h_inv[i + 4] = -h[i + 4];
    h_inv[i + 5] = -h[i + 5];
  }

  p0 = &rrixix[0][16 - 1];
  p1 = &rrixix[1][16 - 1];
  p2 = &rrixix[2][16 - 1];
  p3 = &rrixix[3][16 - 1];
  ptr_h1 = h;
  cor = 0.0F;
  for (i = 0; i < 16; i++) {
    cor += (*ptr_h1) * (*ptr_h1);
    ptr_h1++;
    *p3-- = cor * 0.5F;
    cor += (*ptr_h1) * (*ptr_h1);
    ptr_h1++;
    *p2-- = cor * 0.5F;
    cor += (*ptr_h1) * (*ptr_h1);
    ptr_h1++;
    *p1-- = cor * 0.5F;
    cor += (*ptr_h1) * (*ptr_h1);
    ptr_h1++;
    *p0-- = cor * 0.5F;
  }

  pos = 256 - 1;
  ptr_hf = h + 1;
  for (k = 0; k < 16; k++) {
    p3 = &rrixiy[2][pos];
    p2 = &rrixiy[1][pos];
    p1 = &rrixiy[0][pos];

    if (pos > 15) {
      p0 = &rrixiy[3][pos - 16];
    }

    cor = 0.0F;
    ptr_h1 = h;
    ptr_h2 = ptr_hf;
    for (i = k + 1; i < 16; i++) {
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p3 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p2 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p1 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p0 = cor;
      p3 -= (16 + 1);
      p2 -= (16 + 1);
      p1 -= (16 + 1);
      p0 -= (16 + 1);
    }
    cor += (*ptr_h1) * (*ptr_h2);
    ptr_h1++;
    ptr_h2++;
    *p3 = cor;
    cor += (*ptr_h1) * (*ptr_h2);
    ptr_h1++;
    ptr_h2++;
    *p2 = cor;
    cor += (*ptr_h1) * (*ptr_h2);
    ptr_h1++;
    ptr_h2++;
    *p1 = cor;
    pos -= 16;
    ptr_hf += 4;
  }

  pos = 256 - 1;
  ptr_hf = h + 3;
  for (k = 0; k < 16; k++) {
    p3 = &rrixiy[3][pos];
    p2 = &rrixiy[2][pos - 1];
    p1 = &rrixiy[1][pos - 1];
    p0 = &rrixiy[0][pos - 1];
    cor = 0.0F;
    ptr_h1 = h;
    ptr_h2 = ptr_hf;
    for (i = k + 1; i < 16; i++) {
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p3 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p2 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p1 = cor;
      cor += (*ptr_h1) * (*ptr_h2);
      ptr_h1++;
      ptr_h2++;
      *p0 = cor;
      p3 -= (16 + 1);
      p2 -= (16 + 1);
      p1 -= (16 + 1);
      p0 -= (16 + 1);
    }
    cor += (*ptr_h1) * (*ptr_h2);
    ptr_h1++;
    ptr_h2++;
    *p3 = cor;
    pos--;
    ptr_hf += 4;
  }

  p0 = &rrixiy[0][0];
  for (k = 0; k < 4; k++) {
    for (i = k; i < L_SUBFR; i += 4) {
      psign = sign;
      if (psign[i] < 0.0F) {
        psign = vec;
      }
      j = (k + 1) % 4;
      p0[0] = p0[0] * psign[j];
      p0[1] = p0[1] * psign[j + 4];
      p0[2] = p0[2] * psign[j + 8];
      p0[3] = p0[3] * psign[j + 12];
      p0[4] = p0[4] * psign[j + 16];
      p0[5] = p0[5] * psign[j + 20];
      p0[6] = p0[6] * psign[j + 24];
      p0[7] = p0[7] * psign[j + 28];
      p0[8] = p0[8] * psign[j + 32];
      p0[9] = p0[9] * psign[j + 36];
      p0[10] = p0[10] * psign[j + 40];
      p0[11] = p0[11] * psign[j + 44];
      p0[12] = p0[12] * psign[j + 48];
      p0[13] = p0[13] * psign[j + 52];
      p0[14] = p0[14] * psign[j + 56];
      p0[15] = p0[15] * psign[j + 60];
      p0 += 16;
    }
  }

  psk = -1.0;
  alpk = 1.0;
  for (k = 0; k < nbiter; k++) {
    for (i = 0; i < nb_pulse - (nb_pulse % 3); i += 3) {
      ipos[i] = lpdenc_TableIPosition[(k * 4) + i];
      ipos[i + 1] = lpdenc_TableIPosition[(k * 4) + i + 1];
      ipos[i + 2] = lpdenc_TableIPosition[(k * 4) + i + 2];
    }
    for (; i < nb_pulse; i++) {
      ipos[i] = lpdenc_TableIPosition[(k * 4) + i];
    }

    if ((nbbits == 20) | (nbbits == 28) | (nbbits == 12) | (nbbits == 16)) {
      pos = 0;
      ps = 0.0F;
      alp = 0.0F;
      memset(vec, 0, L_SUBFR * sizeof(float));

      if (nbbits == 28) {
        ipos[4] = 0;
        ipos[5] = 1;
      }

      if (nbbits == 16) {
        ipos[0] = 0;
        ipos[1] = 2;
        ipos[2] = 1;
        ipos[3] = 3;
      }
    } else if ((nbbits == 36) | (nbbits == 44)) {
      pos = 2;
      ind[0] = pos_max[ipos[0]];
      ind[1] = pos_max[ipos[1]];
      ps = dn[ind[0]] + dn[ind[1]];
      alp = rrixix[ipos[0]][ind[0] >> 2] + rrixix[ipos[1]][ind[1] >> 2] +
            rrixiy[ipos[0]][((ind[0] >> 2) << 4) + (ind[1] >> 2)];
      if (sign[ind[0]] < 0.0) {
        p0 = h_inv - ind[0];
      } else {
        p0 = h - ind[0];
      }
      if (sign[ind[1]] < 0.0) {
        p1 = h_inv - ind[1];
      } else {
        p1 = h - ind[1];
      }
      vec[0] = p0[0] + p1[0];
      vec[1] = p0[1] + p1[1];
      vec[2] = p0[2] + p1[2];
      vec[3] = p0[3] + p1[3];
      for (i = 4; i < L_SUBFR; i += 6) {
        vec[i] = p0[i] + p1[i];
        vec[i + 1] = p0[i + 1] + p1[i + 1];
        vec[i + 2] = p0[i + 2] + p1[i + 2];
        vec[i + 3] = p0[i + 3] + p1[i + 3];
        vec[i + 4] = p0[i + 4] + p1[i + 4];
        vec[i + 5] = p0[i + 5] + p1[i + 5];
      }
      if (nbbits == 44) {
        ipos[8] = 0;
        ipos[9] = 1;
      }
    } else {
      pos = 4;
      ind[0] = pos_max[ipos[0]];
      ind[1] = pos_max[ipos[1]];
      ind[2] = pos_max[ipos[2]];
      ind[3] = pos_max[ipos[3]];
      ps = dn[ind[0]] + dn[ind[1]] + dn[ind[2]] + dn[ind[3]];
      p0 = h - ind[0];
      if (sign[ind[0]] < 0.0) {
        p0 = h_inv - ind[0];
      }
      p1 = h - ind[1];
      if (sign[ind[1]] < 0.0) {
        p1 = h_inv - ind[1];
      }
      p2 = h - ind[2];
      if (sign[ind[2]] < 0.0) {
        p2 = h_inv - ind[2];
      }
      p3 = h - ind[3];
      if (sign[ind[3]] < 0.0) {
        p3 = h_inv - ind[3];
      }
      vec[0] = p0[0] + p1[0] + p2[0] + p3[0];
      for (i = 1; i < L_SUBFR; i += 3) {
        vec[i] = p0[i] + p1[i] + p2[i] + p3[i];
        vec[i + 1] = p0[i + 1] + p1[i + 1] + p2[i + 1] + p3[i + 1];
        vec[i + 2] = p0[i + 2] + p1[i + 2] + p2[i + 2] + p3[i + 2];
      }
      alp = 0.0F;
      alp += vec[0] * vec[0] + vec[1] * vec[1];
      alp += vec[2] * vec[2] + vec[3] * vec[3];
      for (i = 4; i < L_SUBFR; i += 6) {
        alp += vec[i] * vec[i];
        alp += vec[i + 1] * vec[i + 1];
        alp += vec[i + 2] * vec[i + 2];
        alp += vec[i + 3] * vec[i + 3];
        alp += vec[i + 4] * vec[i + 4];
        alp += vec[i + 5] * vec[i + 5];
      }
      alp *= 0.5F;
    }

    for (j = pos, st = 0; j < nb_pulse; j += 2, st++) {
      if ((nb_pulse - j) >= 2) {
        correlateHVec1(h, vec, ipos[j], sign, rrixix, cor_x, dn2_pos,
                       nbpos[st]);
        correlateHVec2(h, vec, ipos[j + 1], sign, rrixix, cor_y);

        search2Pulses(nbpos[st], ipos[j], ipos[j + 1], &ps, &alp,
                      &ind[j], &ind[j + 1], dn, dn2_pos, cor_x, cor_y, rrixiy);
      } else {
        correlateHVec2(h, vec, ipos[j], sign, rrixix, cor_x);
        correlateHVec2(h, vec, ipos[j + 1], sign, rrixix, cor_y);
        search1Pulse(ipos[j], ipos[j + 1], &ps, &alp,
                     &ind[j], dn, cor_x, cor_y);
      }
      if (j < (nb_pulse - 2)) {
        p0 = h - ind[j];
        if (sign[ind[j]] < 0.0) {
          p0 = h_inv - ind[j];
        }
        p1 = h - ind[j + 1];
        if (sign[ind[j + 1]] < 0.0) {
          p1 = h_inv - ind[j + 1];
        }
        vec[0] += p0[0] + p1[0];
        vec[1] += p0[1] + p1[1];
        vec[2] += p0[2] + p1[2];
        vec[3] += p0[3] + p1[3];
        for (i = 4; i < L_SUBFR; i += 6) {
          vec[i] += p0[i] + p1[i];
          vec[i + 1] += p0[i + 1] + p1[i + 1];
          vec[i + 2] += p0[i + 2] + p1[i + 2];
          vec[i + 3] += p0[i + 3] + p1[i + 3];
          vec[i + 4] += p0[i + 4] + p1[i + 4];
          vec[i + 5] += p0[i + 5] + p1[i + 5];
        }
      }
    }

    ps = ps * ps;
    s = (alpk * ps) - (psk * alp);
    if (s > 0.0F) {
      psk = ps;
      alpk = alp;
      memcpy(codvec, ind, nb_pulse * sizeof(int));
    }
  }

  memset(code, 0, L_SUBFR * sizeof(float));
  memset(y, 0, L_SUBFR * sizeof(float));
  memset(ind, 0xff, NPMAXPT * 4 * sizeof(int));
  for (k = 0; k < nb_pulse; k++) {
    i = codvec[k];
    val = sign[i];
    index = i / 4;
    track = i % 4;
    if (val > 0) {
      code[i] += 1;
      codvec[k] += (2 * L_SUBFR);
    } else {
      code[i] -= 1;
      index += 16;
    }
    i = track * NPMAXPT;
    while (ind[i] >= 0) {
      i++;
    }
    ind[i] = index;
    p0 = h_inv - codvec[k];
    y[0] += p0[0];
    for (i = 1; i < L_SUBFR; i += 3) {
      y[i] += p0[i];
      y[i + 1] += p0[i + 1];
      y[i + 2] += p0[i + 2];
    }
  }
  if (nbbits == 12) {
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      if (ind[k] >= 0) {
        _index[2 * (track % 2)] = track / 2;
        _index[2 * (track % 2) + 1] = quantize1PulseNp1Bits(ind[k], 4);
      }
    }
  } else if (nbbits == 16) {
    i = 1;
    _index[0] = 0;
    _index[1] = 0;
    _index[2] = 0;
    _index[3] = 0;
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      if (ind[k] >= 0) {
        _index[i++] = quantize1PulseNp1Bits(ind[k], 4);
      } else {
        if (track == 1) {
          _index[0] = 0;
        } else if (track == 3) {
          _index[0] = 1;
        } else {
        }
      }
    }
  } else if (nbbits == 20) {
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      _index[track] = quantize1PulseNp1Bits(ind[k], 4);
    }
  } else if (nbbits == 28) {
    for (track = 0; track < (4 - 2); track++) {
      k = track * NPMAXPT;
      _index[track] = quantize2Pulses2Np1Bits(ind[k], ind[k + 1], 4);
    }
    for (track = 2; track < 4; track++) {
      k = track * NPMAXPT;
      _index[track] = quantize1PulseNp1Bits(ind[k], 4);
    }
  } else if (nbbits == 36) {
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      _index[track] = quantize2Pulses2Np1Bits(ind[k], ind[k + 1], 4);
    }
  } else if (nbbits == 44) {
    for (track = 0; track < (4 - 2); track++) {
      k = track * NPMAXPT;
      _index[track] =
          quantize3Pulses3Np1Bits(ind[k], ind[k + 1], ind[k + 2], 4);
    }
    for (track = 2; track < 4; track++) {
      k = track * NPMAXPT;
      _index[track] = quantize2Pulses2Np1Bits(ind[k], ind[k + 1], 4);
    }
  } else if (nbbits == 52) {
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      _index[track] =
          quantize3Pulses3Np1Bits(ind[k], ind[k + 1], ind[k + 2], 4);
    }
  } else if (nbbits == 64) {
    for (track = 0; track < 4; track++) {
      k = track * NPMAXPT;
      L_index = quantize4Pulses4NBits(&ind[k], 4);
      _index[track] = ((L_index >> 14) & 3);
      _index[track + 4] = (L_index & 0x3FFF);
    }
  }
  return;
}
