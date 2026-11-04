
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

#include "iisLPDEncLib_acelp_ImprovedInnovativeCB.h"

#define L_SUBFR 64
#define NB_PULSE_MAX 24
#define NPMAXPT ((NB_PULSE_MAX + 4 - 1) / 4)
#define NUM_TRACKS 4
#define NB_MAX 8

enum TRACKPOS {
  TRACKPOS_FIXED_FIRST = 0,
  TRACKPOS_FIXED_EVEN = 1,
  TRACKPOS_FIXED_FIRST_TWO = 2,
  TRACKPOS_FIXED_TWO = 3,
  TRACKPOS_FREE_ONE = 4,
  TRACKPOS_FREE_TWO = 5,
  TRACKPOS_FREE_THREE = 6,
  TRACKPOS_GRADIENT = 7
};

typedef struct {
  int nbiter;
  float alp;
  int nb_pulse;
  int fixedpulses;
  int nbpos[10];
  enum TRACKPOS codetrackpos;
} PulseConfig;

static void configureCBStructure(
    PulseConfig *pulse_config,
    int nbbits) {
  switch (nbbits) {
    case 16:
      pulse_config->nbiter = 4;
      pulse_config->alp = 2.0F;
      pulse_config->fixedpulses = 0;
      pulse_config->nb_pulse = 3;
      pulse_config->nbpos[0] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 20:
      pulse_config->nbiter = 4;
      pulse_config->alp = 2.0F;
      pulse_config->nb_pulse = 4;
      pulse_config->fixedpulses = 0;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 28:
      pulse_config->nbiter = 4;
      pulse_config->alp = 1.5F;
      pulse_config->nb_pulse = 6;
      pulse_config->fixedpulses = 0;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 8;
      pulse_config->nbpos[2] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 36:
      pulse_config->nbiter = 4;
      pulse_config->alp = 1.0F;
      pulse_config->nb_pulse = 8;
      pulse_config->fixedpulses = 2;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 8;
      pulse_config->nbpos[2] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 44:
      pulse_config->nbiter = 4;
      pulse_config->alp = 1.0F;
      pulse_config->nb_pulse = 10;
      pulse_config->fixedpulses = 2;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 6;
      pulse_config->nbpos[2] = 8;
      pulse_config->nbpos[3] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 52:
      pulse_config->nbiter = 4;
      pulse_config->alp = 1.0F;
      pulse_config->nb_pulse = 12;
      pulse_config->fixedpulses = 4;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 6;
      pulse_config->nbpos[2] = 8;
      pulse_config->nbpos[3] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
    case 64:
      pulse_config->nbiter = 3;
      pulse_config->alp = 0.8F;
      pulse_config->nb_pulse = 16;
      pulse_config->fixedpulses = 4;
      pulse_config->nbpos[0] = 4;
      pulse_config->nbpos[1] = 4;
      pulse_config->nbpos[2] = 6;
      pulse_config->nbpos[3] = 6;
      pulse_config->nbpos[4] = 8;
      pulse_config->nbpos[5] = 8;
      pulse_config->codetrackpos = TRACKPOS_FIXED_FIRST;
      break;
  }
}

static void toeplitzMultiplication(
    float R[],
    float c[],
    float d[],
    int L_subfr) {
  int k, j;
  float s;

  for (k = 0; k < L_subfr; k++) {
    s = R[k] * c[0];

    for (j = 1; j < k; j++) {
      s += R[k - j] * c[j];
    }

    for (; j < L_subfr; j++) {
      s += R[j - k] * c[j];
    }
    d[k] = s;
  }

  return;
}

static void getPulseSign(
    const float cn[],
    float dn[],
    float dn2[],
    float sign[],
    float vec[],
    float alp) {
  int i;
  float val;
  float s, cor;

  val = (cn[0] * cn[0]) + 1.0F;
  cor = (dn[0] * dn[0]) + 1.0F;

  for (i = 1; i < L_SUBFR; i++) {
    val += (cn[i] * cn[i]);
    cor += (dn[i] * dn[i]);
  }

  s = (float)sqrt(cor / val);
  for (i = 0; i < L_SUBFR; i++) {
    cor = (s * cn[i]) + (alp * dn[i]);

    if (cor >= 0.0F) {
      sign[i] = 1.0F;
      vec[i] = -1.0F;
      dn2[i] = cor;
    } else {
      sign[i] = -1.0F;
      vec[i] = 1.0F;
      dn[i] = -dn[i];
      dn2[i] = -cor;
    }
  }
  return;
}

static void findTrackCandidates(
    float dn2[],
    int dn2_pos[],
    int pos_max[],
    int L_subfr,
    int tracks) {
  int i, k, j;
  float *ps_ptr;

  for (i = 0; i < tracks; i++) {
    for (k = 0; k < NB_MAX; k++) {
      ps_ptr = &dn2[i];

      for (j = i + tracks; j < L_subfr; j += tracks) {
        if (dn2[j] > *ps_ptr) {
          ps_ptr = &dn2[j];
        }
      }

      *ps_ptr = (float)k - NB_MAX;
      dn2_pos[i * 8 + k] = (int)(ps_ptr - dn2);
    }
    pos_max[i] = dn2_pos[i * 8];
  }
}

static void search2PulsesV2(int nb_pos_ix,
                            unsigned char track_x,
                            unsigned char track_y,
                            float *R,
                            float *ps,
                            float *alp,
                            int *ix,
                            int *iy,
                            float dn[],
                            int *dn2,
                            float cor[],
                            float sign[]) {
  int i;
  int y_save = 0;
  int y;
  int x_save = 0;
  int x;
  int *pos_x;
  float ps0, alp0, alp1, ps1, alp2, ps2, sq, s, sqk, alpk, *pR, sgnx, *pRx, *pRy, sign_x, sign_y;

  pos_x = &dn2[track_x << 3];

  ps0 = *ps;
  alp0 = *alp + 2.0f * R[0];

  sqk = -1.0F;
  alpk = 1.0F;

  for (i = 0; i < nb_pos_ix; i++) {
    x = pos_x[i];
    sgnx = sign[x];

    ps1 = ps0 + dn[x];
    alp1 = alp0 + 2 * sgnx * cor[x];
    pR = R - x;

    for (y = track_y; y < L_SUBFR; y += 4) {
      ps2 = ps1 + dn[y];
      alp2 = alp1 + 2.0f * sign[y] * (cor[y] + sgnx * pR[y]);
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

  pRx = R - x_save;
  pRy = R - y_save;
  sign_x = sign[x_save];
  sign_y = sign[y_save];

  for (i = 0; i < L_SUBFR; i++) {
    cor[i] += pRx[i] * sign_x + pRy[i] * sign_y;
  }

  *ix = x_save;
  *iy = y_save;

  return;
}

static void search1PulseV2(unsigned char track_x,
                           unsigned char track_y, float *R, float *ps, float *alp,
                           int *ix, float dn[],
                           float cor[], float sign[]) {
  int x;
  int x_save = 0;
  float ps0, alp0;
  float ps1, sq, sqk;
  float alp1, alpk;
  float s;

  ps0 = *ps;
  alp0 = *alp + R[0];
  sqk = -1.0F;
  alpk = 1.0F;

  for (x = track_x; x < L_SUBFR; x += 4) {
    ps1 = ps0 + dn[x];
    alp1 = alp0 + 2 * sign[x] * cor[x];
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
      alp1 = alp0 + 2 * sign[x] * cor[x];
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
  int i;
  int j;
  int k;
  int nb_pos;
  int n_1;
  int posA[4];
  int posB[4];
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

void LPDEnc_acelp_GetFixedCBIndexV2(
    float dn[],
    float cn[],
    float Rw[],
    float code[],
    float y[],
    int _index[],
    int nbbits,
    float H[]) {
  PulseConfig pulse_config;
  int codvec[NB_PULSE_MAX] = {0};
  int k;
  int l;
  int pos = 0;
  int i;
  int j;
  int st;
  int index;
  int track;
  float cor[L_SUBFR], sign[L_SUBFR], vec[L_SUBFR], dn2[L_SUBFR];
  float R_buf[2 * L_SUBFR - 1], *R;
  int pos_max[4];
  int dn2_pos[8 * 4];
  float ps2k, alpk, ps, alp = 0.0F, ps2, s, psk = 0.0F, val;
  unsigned char ipos[NB_PULSE_MAX], restpulses, iPulse;
  float *p0;
  int ind[NPMAXPT * 4] = {0};
  int L_index;

  memset(&pulse_config, 0, sizeof(pulse_config));

  toeplitzMultiplication(Rw, cn, dn, L_SUBFR);

  configureCBStructure(&pulse_config, nbbits);

  for (k = 0; k < pulse_config.nb_pulse; k++) {
    codvec[k] = (k & 3);
  }

  memset(cor, 0, L_SUBFR * sizeof(float));

  R = R_buf + L_SUBFR - 1;
  R[0] = Rw[0];
  for (k = 1; k < L_SUBFR; k++) {
    R[k] = R[-k] = Rw[k];
  }

  getPulseSign(cn, dn, dn2, sign, vec, pulse_config.alp);

  findTrackCandidates(dn2, dn2_pos, pos_max, L_SUBFR, NUM_TRACKS);

  ps2k = -1.0;
  alpk = 1000.0;

  for (k = 0; k < pulse_config.nbiter; k++) {
    for (l = 0; l < pulse_config.nb_pulse; l++) {
      ipos[l] = lpdenc_TableIPosition[(k * 4) + l];
    }

    restpulses = pulse_config.nb_pulse & 3;

    if (restpulses) {
      switch (pulse_config.codetrackpos) {
        case TRACKPOS_FIXED_FIRST:

          for (iPulse = 0; iPulse < restpulses; iPulse++) {
            ipos[pulse_config.nb_pulse - restpulses + iPulse] = iPulse;
          }

          ipos[pulse_config.nb_pulse] = ipos[pulse_config.nb_pulse - 1];
          break;
        case TRACKPOS_FIXED_EVEN:

          ipos[pulse_config.nb_pulse - restpulses] = (k << 1) & 2;
          ipos[pulse_config.nb_pulse - restpulses + 1] = ipos[pulse_config.nb_pulse - restpulses] ^ 2;
          break;
        case TRACKPOS_FIXED_TWO:

          ipos[pulse_config.nb_pulse] = (ipos[pulse_config.nb_pulse - 1] + 1) & 3;
          break;
        case TRACKPOS_FREE_TWO:
          break;
        default:

          ipos[pulse_config.nb_pulse] = lpdenc_TableIPosition[(k * 4) + pulse_config.nb_pulse];
          break;
      }
    }

    if (pulse_config.fixedpulses == 0) {
      pos = 0;

      ps = 0.0F;

      alp = 0.0F;
      memset(cor, 0, L_SUBFR * sizeof(float));
    }

    else if (pulse_config.fixedpulses == 2) {
      pos = 2;

      ind[0] = pos_max[ipos[0]];
      ind[1] = pos_max[ipos[1]];

      ps = dn[ind[0]] + dn[ind[1]];

      p0 = R - ind[0];
      if (sign[ind[0]] > 0) {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = *p0;
          p0++;
        }
      } else {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = -*p0;
          p0++;
        }
      }

      p0 = R - ind[1];
      if (sign[ind[1]] > 0) {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] += *p0;
          p0++;
        }
      } else {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] -= *p0;
          p0++;
        }
      }

      alp = sign[ind[0]] * cor[ind[0]] + sign[ind[1]] * cor[ind[1]];

    }

    else if (pulse_config.fixedpulses == 4) {
      pos = 4;

      ind[0] = pos_max[ipos[0]];
      ind[1] = pos_max[ipos[1]];
      ind[2] = pos_max[ipos[2]];
      ind[3] = pos_max[ipos[3]];

      ps = dn[ind[0]] + dn[ind[1]] + dn[ind[2]] + dn[ind[3]];

      p0 = R - ind[0];
      if (sign[ind[0]] > 0) {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = *p0;
          p0++;
        }
      } else {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = -*p0;
          p0++;
        }
      }

      for (j = 1; j < 4; j++) {
        p0 = R - ind[j];
        if (sign[ind[j]] > 0) {
          for (i = 0; i < L_SUBFR; i++) {
            cor[i] += *p0;
            p0++;
          }
        } else {
          for (i = 0; i < L_SUBFR; i++) {
            cor[i] -= *p0;
            p0++;
          }
        }
      }

      alp = sign[ind[0]] * cor[ind[0]] + sign[ind[1]] * cor[ind[1]] + sign[ind[2]] * cor[ind[2]] + sign[ind[3]] * cor[ind[3]];
    }

    else {
      pos = pulse_config.fixedpulses;

      ind[0] = pos_max[ipos[0]];

      ps = dn[ind[0]];

      p0 = R - ind[0];
      if (sign[ind[0]] > 0) {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = *p0;
          p0++;
        }
      } else {
        for (i = 0; i < L_SUBFR; i++) {
          cor[i] = -*p0;
          p0++;
        }
      }
      alp = sign[ind[0]] * cor[ind[0]];

      for (j = 1; j < pulse_config.fixedpulses; j++) {
        if (j < 4) {
          ind[j] = pos_max[ipos[j]];
        } else {
          ind[j] = dn2_pos[ipos[j] * 8 + (j >> 2)];
        }

        ps += dn[ind[j]];

        p0 = R - ind[j];
        if (sign[ind[j]] > 0) {
          for (i = 0; i < L_SUBFR; i++) {
            cor[i] += *p0;
            p0++;
          }
        } else {
          for (i = 0; i < L_SUBFR; i++) {
            cor[i] -= *p0;
            p0++;
          }
        }

        alp += sign[ind[j]] * cor[ind[j]];
      }
    }

    for (j = pos, st = 0; j < pulse_config.nb_pulse; j += 2, st++) {
      if ((pulse_config.nb_pulse - j) >= 2) {
        search2PulsesV2(pulse_config.nbpos[st], ipos[j], ipos[j + 1], R, &ps, &alp,
                        &ind[j], &ind[j + 1], dn, dn2_pos, cor, sign);

      } else {
        search1PulseV2(ipos[j], ipos[j + 1], R, &ps, &alp,
                       &ind[j], dn, cor, sign);
      }

      if ((pulse_config.codetrackpos == TRACKPOS_FREE_TWO) && (j == pulse_config.nb_pulse - 2)) {
        j--;
      }
    }

    ps2 = ps * ps;
    s = (alpk * ps2) - (ps2k * alp);

    if (s > 0.0F) {
      ps2k = ps2;
      psk = ps;
      alpk = alp;
      memcpy(codvec, ind, pulse_config.nb_pulse * sizeof(int));
    }
  }

  memset(code, 0, L_SUBFR * sizeof(float));
  memset(y, 0, L_SUBFR * sizeof(float));
  memset(ind, 0xff, NPMAXPT * 4 * sizeof(int));

  for (k = 0; k < pulse_config.nb_pulse; k++) {
    i = codvec[k];
    val = sign[i];

    index = i / 4;
    track = i % 4;

    if (val * psk > 0) {
      code[i] += 1.0f;
      codvec[k] += (2 * L_SUBFR);
    } else {
      code[i] -= 1.0f;
      index += 16;
    }

    i = track * NPMAXPT;

    while (ind[i] >= 0) {
      i++;
    }

    ind[i] = index;
  }

  for (i = 0; i < L_SUBFR; i++) {
    if (code[i] != 0.0f) {
      for (k = 0; k < L_SUBFR - i; k++) {
        y[i + k] += code[i] * H[k];
      }
    }
  }

  if (nbbits == 12) {
    _index[0] = 0;
    _index[1] = 0;
    _index[2] = 0;
    _index[3] = 0;

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
}
