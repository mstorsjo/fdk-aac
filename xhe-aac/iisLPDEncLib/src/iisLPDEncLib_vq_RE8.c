
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
#include "iisLPDEncLib_vq_RE8.h"
#include "iisLPDComLib_vq_RE8.h"
#include "mathlib.h"

#define NB_SPHERE 32

static const int p_iislpdCodec_tab_pow2[8] = {128, 64, 32, 16, 8, 4, 2, 1};

static const int p_absoluteLeaders_pos[NB_SPHERE] = {
    0, 2, 5, 8, 13, 18, 20, 22, 23, 25, 26, 27, 27, 28, 28, 28,
    29, 30, 31, 31, 32, 32, 32, 32, 32, 34, 35, 35, 35, 35, 35, 35};

static const int p_absoluteLeaders_nb[NB_SPHERE] = {
    2, 3, 3, 5, 5, 2, 2, 1, 2, 1, 1, 0, 1, 0, 0, 1,
    1, 1, 0, 1, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1};

static const int p_absoluteLeaders_nq[NB_LEADER + 2] = {
    2, 2, 3, 3, 2, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 4,
    4, 3, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 0, 100};

static const unsigned int p_absoluteLeaders_id[NB_LEADER] = {
    0x0001, 0x0004, 0x0008, 0x000B, 0x0020, 0x000C, 0x0015, 0x0024,
    0x0010, 0x001F, 0x0028, 0x0040, 0x004F, 0x0029, 0x002C, 0x0044,
    0x0059, 0x00A4, 0x0060, 0x00A8, 0x00C4, 0x012D, 0x0200, 0x0144,
    0x0204, 0x0220, 0x0335, 0x04E4, 0x0400, 0x0584, 0x0A20, 0x0A40,
    0x09C4, 0x12C4, 0x0C20, 0x2000, 0x4E20};

extern const int p_lpdcom_tab_factorial[8];
extern const int p_lpdcom_signedLeadersFirstAssociated[NB_LEADER];
extern const unsigned int p_lpdcom_signedLeadersOffset[NB_LDSIGN];
extern const unsigned char p_lpdcom_signedLeadersCodes[NB_LDSIGN];

static void getLatticeBaseIndex(int *p_codebookVec, int *p_absLeader, int *p_IdxCodebook);
static int identifyAbsoluteLeader(int *p_Re8In);
static void getRE8Coordinates(int *p_Re8In, int *p_Re8Coordinates);
static void getPermutationRankAndSignCode(int *p_codebookVec, int *p_permutationRank, int *p_signCode);

static void getPermutationRankAndSignCode(
    int *p_codebookVec,
    int *p_permutationRank,
    int *p_signCode) {
  int p_signedLeader[8];
  int p_alphabet[8];
  int alphabetSize;
  int p_codebookVecIndexed[8];
  int p_numberOccurrences[8];
  int A;
  int B;
  int idx;
  int tmp;
  int abs_i;
  int abs_j;
  int i, j, k;

  for (i = 0; i < 8; i++) {
    p_signedLeader[i] = p_codebookVec[i];
  }

  for (k = 0; k < 7; k++) {
    j = k;
    for (i = k + 1; i < 8; i++) {
      abs_j = abs(p_signedLeader[j]);
      abs_i = abs(p_signedLeader[i]);
      if (abs_i >= abs_j) {
        if (abs_i > p_signedLeader[j]) {
          j = i;
        }
      }
    }
    if (j > k) {
      tmp = p_signedLeader[k];
      p_signedLeader[k] = p_signedLeader[j];
      p_signedLeader[j] = tmp;
    }
  }

  *p_signCode = 0;
  for (i = 0; i < 8; i++) {
    if (p_signedLeader[i] < 0) {
      *p_signCode += p_iislpdCodec_tab_pow2[i];
    }
  }

  p_alphabet[0] = p_signedLeader[0];
  alphabetSize = 1;

  for (i = 1; i < 8; i++) {
    if (p_signedLeader[i] != p_signedLeader[i - 1]) {
      p_alphabet[alphabetSize] = p_signedLeader[i];
      alphabetSize++;
    }
  }

  for (i = 0; i < 8; i++) {
    for (j = 0; j < alphabetSize; j++) {
      if (p_codebookVec[i] == p_alphabet[j]) {
        p_codebookVecIndexed[i] = j;
        break;
      }
    }
  }

  *p_permutationRank = 0;

  for (j = 0; j < alphabetSize; j++) {
    p_numberOccurrences[j] = 0;
  }

  B = 1;

  for (i = 7; i >= 0; i--) {
    idx = p_codebookVecIndexed[i];
    p_numberOccurrences[idx]++;
    B *= p_numberOccurrences[idx];
    A = 0;
    for (j = 0; j < idx; j++) {
      A += p_numberOccurrences[j];
    }
    if (A > 0) {
      *p_permutationRank += A * p_lpdcom_tab_factorial[i] / B;
    }
  }
}

static void getLatticeBaseIndex(
    int *p_codebookVec,
    int *p_absLeader,
    int *p_IdxCodebook) {
  int permutationRank;
  int offset;
  int signCode;
  int i;
  int ks;

  getPermutationRankAndSignCode(p_codebookVec, &permutationRank, &signCode);

  ks = -1;
  for (i = p_lpdcom_signedLeadersFirstAssociated[*p_absLeader]; i < NB_LDSIGN; i++) {
    if (signCode == p_lpdcom_signedLeadersCodes[i]) {
      ks = i;
      break;
    }
  }

  offset = p_lpdcom_signedLeadersOffset[ks];

  *p_IdxCodebook = offset + permutationRank;
}

static int identifyAbsoluteLeader(
    int *p_Re8In) {
  int i;
  int shell;
  int p_shellVec[8];
  int numberAbsLeaders;
  int pos;
  int idxAbsLeader;
  long id;

  for (i = 0; i < 8; i++) {
    p_shellVec[i] = p_Re8In[i] * p_Re8In[i];
  }
  shell = 0;
  for (i = 0; i < 8; i++) {
    shell += p_shellVec[i];
  }
  shell >>= 3;

  idxAbsLeader = NB_LEADER + 1;
  if (shell == 0) {
    idxAbsLeader = NB_LEADER;
  } else {
    if (shell <= NB_SPHERE) {
      id = 0;
      for (i = 0; i < 8; i++) {
        id += p_shellVec[i] * p_shellVec[i];
      }
      id = id >> 3;

      numberAbsLeaders = p_absoluteLeaders_nb[shell - 1];
      pos = p_absoluteLeaders_pos[shell - 1];
      for (i = 0; i < numberAbsLeaders; i++) {
        if (id == (long)p_absoluteLeaders_id[pos]) {
          idxAbsLeader = pos;
          break;
        }
        pos++;
      }
    }
  }
  return idxAbsLeader;
}

static void getRE8Coordinates(
    int *p_Re8In,
    int *p_Re8Coordinates) {
  int i;
  int tmp;
  int sum;

  p_Re8Coordinates[7] = p_Re8In[7];
  tmp = p_Re8In[7];
  sum = 5 * p_Re8In[7];

  for (i = 6; i >= 1; i--) {
    p_Re8Coordinates[i] = (p_Re8In[i] - tmp) >> 1;
    sum -= p_Re8In[i];
  }

  p_Re8Coordinates[0] = (p_Re8In[0] + sum) >> 2;
}

static void getVoronoiIndex(
    int *p_Re8In,
    int *p_NumCodebook,
    int *p_idxVoronoi,
    int *p_codebookVec,
    int *absLeader) {
  int i;
  int r;
  int scaling;
  int p_voronoiVec[8];
  int p_codebookVecTmp[8];
  int p_idxVoronoiMod[8];
  int p_idxVoronoiTmp[8];
  int iter;
  int absLeaderTmp;
  int numCodebookTmp;
  int mask;
  float sphere;

  *absLeader = identifyAbsoluteLeader(p_Re8In);

  *p_NumCodebook = p_absoluteLeaders_nq[*absLeader];

  if (*p_NumCodebook <= 4) {
    for (i = 0; i < 8; i++) {
      p_codebookVec[i] = p_Re8In[i];
    }
  } else {
    sphere = 0.0;
    for (i = 0; i < 8; i++) {
      sphere += (float)p_Re8In[i] * (float)p_Re8In[i];
    }

    sphere *= 0.125f;
    r = 1;
    sphere *= 0.25f;

    while (sphere > 11.0) {
      r++;
      sphere *= 0.25f;
    }

    getRE8Coordinates(p_Re8In, p_idxVoronoiMod);

    scaling = 1 << r;
    mask = scaling - 1;

    for (iter = 0; iter < 2; iter++) {
      for (i = 0; i < 8; i++) {
        p_idxVoronoiTmp[i] = p_idxVoronoiMod[i] & mask;
      }
      LPDCom_vq_VoronoiIndexToVector(p_idxVoronoiTmp, scaling, p_voronoiVec);

      for (i = 0; i < 8; i++) {
        p_codebookVecTmp[i] = (p_Re8In[i] - p_voronoiVec[i]) / scaling;
      }

      absLeaderTmp = identifyAbsoluteLeader(p_codebookVecTmp);

      numCodebookTmp = p_absoluteLeaders_nq[absLeaderTmp];
      if (numCodebookTmp > 4) {
        r++;
        scaling = scaling << 1;
        mask = ((mask << 1) + 1);
      } else {
        if (numCodebookTmp < 3) {
          numCodebookTmp = 3;
        }

        *absLeader = absLeaderTmp;
        *p_NumCodebook = numCodebookTmp + 2 * r;

        for (i = 0; i < 8; i++) {
          p_idxVoronoi[i] = p_idxVoronoiTmp[i];
        }

        for (i = 0; i < 8; i++) {
          p_codebookVec[i] = p_codebookVecTmp[i];
        }

        r--;
        scaling = scaling >> 1;
        mask = mask >> 1;
      }
    }
  }
  return;
}

void LPDEnc_vq_getAVQindices(
    int *p_Re8In,
    int *p_NumCodebook,
    int *p_IdxCodebook,
    int *p_idxVoronoi) {
  int absLeader, p_codebookVec[8];

  getVoronoiIndex(p_Re8In, p_NumCodebook, p_idxVoronoi, p_codebookVec, &absLeader);

  if (*p_NumCodebook > 0) {
    getLatticeBaseIndex(p_codebookVec, &absLeader, p_IdxCodebook);
  }

  return;
}
