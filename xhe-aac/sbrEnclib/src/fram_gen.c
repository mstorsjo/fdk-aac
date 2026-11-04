
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
#include <string.h>
#include <math.h>
#include <assert.h>
#include "iisutillib.h"
#include "fram_gen.h"
#include "mathlib.h"
#include "sbr_misc.h"

#if defined __GNUC__ || defined __clang__
#define SBR_UNUSED __attribute__((unused))
#else
#define SBR_UNUSED
#endif

static SBR_FRAME_INFO
    frameInfo1_2048 = {1,
                       {0, 16},
                       {FREQ_RES_HIGH},
                       0,
                       1,
                       {0, 16}};

static SBR_FRAME_INFO
    frameInfo2_2048 = {2,
                       {0, 8, 16},
                       {FREQ_RES_HIGH, FREQ_RES_HIGH},
                       0,
                       2,
                       {0, 8, 16}};

static SBR_FRAME_INFO
    frameInfo4_2048 = {4,
                       {0, 4, 8, 12, 16},
                       {FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH},
                       0,
                       2,
                       {0, 8, 16}};

static SBR_FRAME_INFO
    frameInfo8_2048 = {8,
                       {0, 2, 4, 6, 8, 10, 12, 14, 16},
                       {FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH,
                        FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH},
                       0,
                       2,
                       {0, 8, 16}};

static SBR_FRAME_INFO
    frameInfo16_2048 = {16,
                        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                        {FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH,
                         FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH,
                         FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH,
                         FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH, FREQ_RES_HIGH},
                        0,
                        2,
                        {0, 8, 16}};

#define INVALID_TABLE_IDX -1

static void
addFreqLeft(FREQ_RES *vector,
            int *length_vector,
            FREQ_RES value) {
  int i;

  for (i = *length_vector; i > 0; i--) {
    vector[i] = vector[i - 1];
  }
  vector[0] = value;
  (*length_vector)++;
}

static void
addFreqVecLeft(FREQ_RES *dst,
               int *length_dst,
               FREQ_RES *src,
               int length_src) {
  int i;

  for (i = length_src - 1; i >= 0; i--) {
    addFreqLeft(dst, length_dst, src[i]);
  }
}

static void
addFreqRight(FREQ_RES *vector,
             int *length_vector,
             FREQ_RES value) {
  vector[*length_vector] = value;
  (*length_vector)++;
}

static void
fillFrameTran(const int *v_tuningSegm,
              const FREQ_RES *v_tuningFreq,
              FREQ_RES freqResFillPost,
              int tran,
              int *aBorders,
              int *pBorderVecLen,
              FREQ_RES *aFreqRes,
              int *pFreqResVecLen,
              int *pBmin,
              int *pBmax) {
  int bord, i;

  *pBorderVecLen = 0;
  *pFreqResVecLen = 0;

  if (v_tuningSegm[0]) {
    AddRight(aBorders, pBorderVecLen, (tran - v_tuningSegm[0]));

    addFreqRight(aFreqRes, pFreqResVecLen, v_tuningFreq[0]);
  }

  bord = tran;
  AddRight(aBorders, pBorderVecLen, tran);

  if (v_tuningSegm[1]) {
    bord += v_tuningSegm[1];

    AddRight(aBorders, pBorderVecLen, bord);

    addFreqRight(aFreqRes, pFreqResVecLen, v_tuningFreq[1]);
  }

  if (v_tuningSegm[2] != 0) {
    bord += v_tuningSegm[2];

    AddRight(aBorders, pBorderVecLen, bord);

    addFreqRight(aFreqRes, pFreqResVecLen, v_tuningFreq[2]);
  }

  addFreqRight(aFreqRes, pFreqResVecLen, freqResFillPost);

  *pBmin = aBorders[0];
  for (i = 0; i < *pBorderVecLen; i++) {
    if (aBorders[i] < *pBmin) {
      *pBmin = aBorders[i];
    }
  }

  *pBmax = aBorders[0];
  for (i = 0; i < *pBorderVecLen; i++) {
    if (aBorders[i] > *pBmax) {
      *pBmax = aBorders[i];
    }
  }
}

static void
fillFramePre(int dmax,
             FREQ_RES freqResFillPre,
             int rest,
             int bmin,
             int *aBorders,
             int *pBorderVecLen,
             FREQ_RES *aFreqRes,
             int *pFreqResVecLen) {
  int numParts, d, j, S, s = 0, segm, bord;

  numParts = 1;
  d = rest;

  while (d > dmax) {
    numParts++;

    segm = rest / numParts;
    S = (int)floor((segm - 2) * 0.5);
    s = min(8, 2 * S + 2);
    d = rest - (numParts - 1) * s;
  }

  bord = bmin;

  for (j = 0; j <= numParts - 2; j++) {
    bord = bord - s;

    AddLeft(aBorders, pBorderVecLen, bord);

    addFreqLeft(aFreqRes, pFreqResVecLen, freqResFillPre);
  }
}

static void
calcFillLengthMax(CODEC_TYPE coreCodec,
                  int numberTimeSlots,
                  int tranPos,
                  int *pfmax) {
  switch (coreCodec) {
    case CODEC_SAAC:
      switch (numberTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          if (tranPos < 4)
            *pfmax = 6;
          else if (tranPos == 4 || tranPos == 5)
            *pfmax = 4;
          else
            *pfmax = 8;
          break;
        default:
          *pfmax = 0;
          break;
      }
      break;
    default:
      *pfmax = 8;
  }
}

static void
fillFramePost(int numberTimeSlots,
              int dmax,
              FREQ_RES freqResFillPost,
              int fmax,
              int bmax,
              int *aBorders,
              int *pBorderVecLen,
              FREQ_RES *aFreqRes,
              int *pFreqResVecLen,
              int *pNumParts,
              int *d) {
  int j, rest, segm, S, s = 0, bord;

  rest = 2 * numberTimeSlots - bmax;
  *d = rest;

  if (*d > 0) {
    *pNumParts = 1;

    while (*d > dmax) {
      *pNumParts = *pNumParts + 1;

      segm = rest / (*pNumParts);
      S = (int)floor((segm - 2) * 0.5);
      s = min(fmax, 2 * S + 2);
      *d = rest - (*pNumParts - 1) * s;
    }

    bord = bmax;
    for (j = 0; j <= *pNumParts - 2; j++) {
      bord += s;

      AddRight(aBorders, pBorderVecLen, bord);

      addFreqRight(aFreqRes, pFreqResVecLen, freqResFillPost);
    }
  } else {
    *pNumParts = 1;

    *pBorderVecLen = *pBorderVecLen - 1;
    *pFreqResVecLen = *pFreqResVecLen - 1;
  }
}

static void
fillFrameInter(int numberTimeSlots,
               FRAME_GEN_TUNING tuning,
               int *aBordersFollow,
               int *pBorderVecLenFollow,
               FREQ_RES *aFreqResFollow,
               int *pFreqResVecLenFollow,
               int fillIdxFollow,
               int bmin,
               int *aBorders,
               int *pBorderVecLen,
               FREQ_RES *aFreqRes,
               int *pFreqResVecLen,
               int *pNumBordersFromLeft) {
  int middle, b_new, numBordFollow, bordMaxFollow, i;

  switch (numberTimeSlots) {
    case NUMBER_TIME_SLOTS_2048:

      if (fillIdxFollow >= 1) {
        *pBorderVecLenFollow = fillIdxFollow;
        *pFreqResVecLenFollow = fillIdxFollow;
      }
      numBordFollow = *pBorderVecLenFollow;
      bordMaxFollow = aBordersFollow[numBordFollow - 1];

      middle = bmin - bordMaxFollow;

      while (middle < 0) {
        numBordFollow--;
        bordMaxFollow = aBordersFollow[numBordFollow - 1];
        middle = bmin - bordMaxFollow;
      }
      *pBorderVecLenFollow = numBordFollow;
      *pFreqResVecLenFollow = numBordFollow;

      *pNumBordersFromLeft = numBordFollow - 1;

      b_new = *pBorderVecLen;

      if (middle <= tuning.dmax) {
        if (middle >= tuning.dmin) {
          AddVecLeft(aBorders, pBorderVecLen, aBordersFollow, *pBorderVecLenFollow);
          addFreqVecLeft(aFreqRes, pFreqResVecLen, aFreqResFollow, *pFreqResVecLenFollow);
        } else {
          if (tuning.segmentLength[0] != 0) {
            *pBorderVecLen = b_new - 1;
            AddVecLeft(aBorders, pBorderVecLen, aBordersFollow, *pBorderVecLenFollow);

            *pFreqResVecLen = b_new - 1;
            addFreqVecLeft(aFreqRes + 1, pFreqResVecLen, aFreqResFollow, *pFreqResVecLenFollow);
          } else {
            if (*pBorderVecLenFollow > 1) {
              AddVecLeft(aBorders, pBorderVecLen, aBordersFollow, *pBorderVecLenFollow - 1);
              addFreqVecLeft(aFreqRes, pFreqResVecLen, aFreqResFollow, *pBorderVecLenFollow - 1);

              *pNumBordersFromLeft = *pNumBordersFromLeft - 1;
            } else {
              for (i = 0; i < *pBorderVecLen - 1; i++) {
                aBorders[i] = aBorders[i + 1];
              }

              for (i = 0; i < *pFreqResVecLen - 1; i++) {
                aFreqRes[i] = aFreqRes[i + 1];
              }

              *pBorderVecLen = b_new - 1;
              *pFreqResVecLen = b_new - 1;

              AddVecLeft(aBorders, pBorderVecLen, aBordersFollow, *pBorderVecLenFollow);
              addFreqVecLeft(aFreqRes, pFreqResVecLen, aFreqResFollow, *pFreqResVecLenFollow);
            }
          }
        }
      } else {
        fillFramePre(tuning.dmax,
                     tuning.freqResFillPre,
                     middle,
                     bmin,
                     aBorders,
                     pBorderVecLen,
                     aFreqRes,
                     pFreqResVecLen);

        AddVecLeft(aBorders, pBorderVecLen, aBordersFollow, *pBorderVecLenFollow);
        addFreqVecLeft(aFreqRes, pFreqResVecLen, aFreqResFollow, *pFreqResVecLenFollow);
      }
      break;

    default:
      assert(0);
  }
}

static void
calcFrameClass(FRAME_CLASS *frameClass,
               FRAME_CLASS *frameClassOld,
               int tranFlag,
               int *pSpreadFlag) {
  switch (*frameClassOld) {
    case FIXFIX:
      if (tranFlag) {
        *frameClass = FIXVAR;
      } else {
        *frameClass = FIXFIX;
      }
      break;

    case FIXVAR:
      if (tranFlag) {
        *frameClass = VARVAR;
        *pSpreadFlag = 0;
      } else {
        if (*pSpreadFlag) {
          *frameClass = VARVAR;
        } else {
          *frameClass = VARFIX;
        }
      }
      break;

    case VARFIX:
      if (tranFlag) {
        *frameClass = FIXVAR;
      } else {
        *frameClass = FIXFIX;
      }
      break;

    case VARVAR:
      if (tranFlag) {
        *frameClass = VARVAR;
        *pSpreadFlag = 0;
      } else {
        if (*pSpreadFlag) {
          *frameClass = VARVAR;
        } else {
          *frameClass = VARFIX;
        }
      }
      break;

    default:
      assert(0);
  }

  *frameClassOld = *frameClass;
}

static void
specialCase(int allowSpread,
            int *pSpreadFlag,
            int *aBorders,
            int *pBorderVecLen,
            FREQ_RES *aFreqRes,
            int *pFreqResVecLen,
            int *pNumParts,
            int d) {
  int L;

  L = *pBorderVecLen;

  if (allowSpread) {
    *pSpreadFlag = 1;
    AddRight(aBorders, pBorderVecLen, aBorders[L - 1] + 8);
    addFreqRight(aFreqRes, pFreqResVecLen, FREQ_RES_HIGH);
    (*pNumParts)++;
  } else {
    if (d == 1) {
      *pBorderVecLen = L - 1;
      *pFreqResVecLen = L - 1;
    } else {
      if ((aBorders[L - 1] - aBorders[L - 2]) > 2) {
        aBorders[L - 1] = aBorders[L - 1] - 2;
        aFreqRes[*pFreqResVecLen - 1] = FREQ_RES_LOW;
      }
    }
  }
}

static void
calcCmonBorder(int numberTimeSlots,
               int tran,
               const int *aBorders,
               const int *pBorderVecLen,
               int *pCmonBorderIdx,
               int *pTranIdx) {
  int i;

  for (i = 0; i < *pBorderVecLen; i++) {
    if (aBorders[i] >= numberTimeSlots) {
      *pCmonBorderIdx = i;
      break;
    }
  }

  for (i = 0; i < *pBorderVecLen; i++) {
    if (aBorders[i] >= tran) {
      *pTranIdx = i;
      break;
    } else {
      *pTranIdx = EMPTY;
    }
  }
}

static void
keepForFollowUp(int numberTimeSlots,
                const int *aBorders,
                const int *pBorderVecLen,
                const FREQ_RES *aFreqRes,
                int cmonBorderIdx,
                int tranIdx,
                int numParts,
                int *aBordersFollow,
                int *pBorderVecLenFollow,
                FREQ_RES *aFreqResFollow,
                int *pFreqResVecLenFollow,
                int *pTranIdxFollow,
                int *pFillIdxFollow) {
  int L, i, j;

  L = *pBorderVecLen;

  (*pBorderVecLenFollow) = 0;
  (*pFreqResVecLenFollow) = 0;

  for (j = 0, i = cmonBorderIdx; i < L; i++, j++) {
    aBordersFollow[j] = aBorders[i] - numberTimeSlots;
    aFreqResFollow[j] = aFreqRes[i];
    (*pBorderVecLenFollow)++;
    (*pFreqResVecLenFollow)++;
  }

  if (tranIdx != EMPTY) {
    *pTranIdxFollow = tranIdx - cmonBorderIdx;
  } else {
    *pTranIdxFollow = EMPTY;
  }

  *pFillIdxFollow = L - (numParts - 1) - cmonBorderIdx;
}

static void
calcSbrGrid(FRAME_CLASS frameClass,
            FREQ_RES freqResFillPre,
            const int *aBorders,
            int borderVecLen,
            const FREQ_RES *aFreqRes,
            int freqResVecLen,
            int cmonBorderIdx,
            int tranIdx,
            int spreadFlag,
            int numBordersFromLeft,
            HANDLE_SBR_GRID hSbrGrid) {
  int i, r, a, n, p, b, aL, aR, ntot, nmax, numBordersFromRight;

  FREQ_RES *v_f = hSbrGrid->v_f;
  FREQ_RES *v_fLR = hSbrGrid->v_fLR;
  int *v_r = hSbrGrid->bs_rel_bord;
  int *v_rL = hSbrGrid->bs_rel_bord_0;
  int *v_rR = hSbrGrid->bs_rel_bord_1;

  int length_v_r = 0;
  int length_v_rR = 0;
  int length_v_rL = 0;

  switch (frameClass) {
    case FIXVAR:

      a = aBorders[cmonBorderIdx];

      length_v_r = 0;
      i = cmonBorderIdx;
      while (i >= 1) {
        r = aBorders[i] - aBorders[i - 1];
        AddRight(v_r, &length_v_r, r);
        i--;
      }

      n = length_v_r;

      for (i = 0; i < cmonBorderIdx; i++) {
        v_f[i] = aFreqRes[cmonBorderIdx - 1 - i];
      }

      v_f[cmonBorderIdx] = freqResFillPre;

      if (cmonBorderIdx >= tranIdx && tranIdx != EMPTY) {
        p = cmonBorderIdx - tranIdx + 1;
      } else {
        p = 0;
      }

      hSbrGrid->frameClass = frameClass;
      hSbrGrid->bs_abs_bord = a;
      hSbrGrid->n = n;
      hSbrGrid->p = p;

      break;
    case VARFIX:

      a = aBorders[0];

      length_v_r = 0;
      for (i = 1; i < borderVecLen; i++) {
        r = aBorders[i] - aBorders[i - 1];
        AddRight(v_r, &length_v_r, r);
      }

      n = length_v_r;

      memcpy(v_f, aFreqRes, freqResVecLen * sizeof(int));

      if (tranIdx >= 0 && tranIdx != EMPTY) {
        p = tranIdx + 1;
      } else {
        p = 0;
      }

      hSbrGrid->frameClass = frameClass;
      hSbrGrid->bs_abs_bord = a;
      hSbrGrid->n = n;
      hSbrGrid->p = p;

      break;
    case VARVAR:
      if (spreadFlag) {
        b = borderVecLen;

        aL = aBorders[0];
        aR = aBorders[b - 1];

        ntot = b - 2;

        nmax = 2;
        if (ntot > nmax) {
          numBordersFromLeft = nmax;
          numBordersFromRight = ntot - nmax;
        } else {
          numBordersFromLeft = ntot;
          numBordersFromRight = 0;
        }

        length_v_rL = 0;
        for (i = 1; i <= numBordersFromLeft; i++) {
          r = aBorders[i] - aBorders[i - 1];
          AddRight(v_rL, &length_v_rL, r);
        }

        length_v_rR = 0;
        i = b - 1;
        while (i >= b - numBordersFromRight) {
          r = aBorders[i] - aBorders[i - 1];
          AddRight(v_rR, &length_v_rR, r);
          i--;
        }

        if (tranIdx > 0 && tranIdx != EMPTY) {
          p = b - tranIdx;
        } else {
          p = 0;
        }

        for (i = 0; i < b - 1; i++) {
          v_fLR[i] = aFreqRes[i];
        }
      } else {
        borderVecLen = cmonBorderIdx + 1;

        b = borderVecLen;

        aL = aBorders[0];
        aR = aBorders[b - 1];

        ntot = b - 2;
        numBordersFromRight = ntot - numBordersFromLeft;

        length_v_rL = 0;
        for (i = 1; i <= numBordersFromLeft; i++) {
          r = aBorders[i] - aBorders[i - 1];
          AddRight(v_rL, &length_v_rL, r);
        }

        length_v_rR = 0;
        i = b - 1;
        while (i >= b - numBordersFromRight) {
          r = aBorders[i] - aBorders[i - 1];
          AddRight(v_rR, &length_v_rR, r);
          i--;
        }

        if (cmonBorderIdx >= tranIdx && tranIdx != EMPTY) {
          p = cmonBorderIdx - tranIdx + 1;
        } else {
          p = 0;
        }

        for (i = 0; i < b - 1; i++) {
          v_fLR[i] = aFreqRes[i];
        }
      }

      hSbrGrid->frameClass = frameClass;
      hSbrGrid->bs_abs_bord_0 = aL;
      hSbrGrid->bs_abs_bord_1 = aR;
      hSbrGrid->bs_num_rel_0 = numBordersFromLeft;
      hSbrGrid->bs_num_rel_1 = numBordersFromRight;
      hSbrGrid->p = p;

      break;

    default:

      break;
  }
}

static void
createDefFrameInfo(int nTimeSlots,
                   int nEnv,
                   HANDLE_SBR_FRAME_INFO hSbrFrameInfo) {
  switch (nEnv) {
    case 1:
      switch (nTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          memcpy(hSbrFrameInfo, &frameInfo1_2048, sizeof(SBR_FRAME_INFO));
          break;
        default:
          assert(0);
      }
      break;

    case 2:
      switch (nTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          memcpy(hSbrFrameInfo, &frameInfo2_2048, sizeof(SBR_FRAME_INFO));
          break;
        default:
          assert(0);
      }
      break;

    case 4:
      switch (nTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          memcpy(hSbrFrameInfo, &frameInfo4_2048, sizeof(SBR_FRAME_INFO));
          break;
        default:
          assert(0);
      }
      break;

    case 8:
      switch (nTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          memcpy(hSbrFrameInfo, &frameInfo8_2048, sizeof(SBR_FRAME_INFO));
          break;
        default:
          assert(0);
      }
      break;

    case 16:
      switch (nTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          memcpy(hSbrFrameInfo, &frameInfo16_2048, sizeof(SBR_FRAME_INFO));
          break;
        default:
          assert(0);
      }
      break;

    default:
      assert(0);
  }
}

static void
sbrGrid2FrameInfo(HANDLE_SBR_GRID hSbrGrid,
                  HANDLE_SBR_FRAME_INFO hSbrFrameInfo,
                  FREQ_RES *freq_res_fixfix) {
  int frameSplit = 0;
  int nEnv = 0, border = 0, i, k, p;
  int *v_r = hSbrGrid->bs_rel_bord;
  FREQ_RES *v_f = hSbrGrid->v_f;

  FRAME_CLASS frameClass = hSbrGrid->frameClass;

  int numberTimeSlots = hSbrGrid->numberTimeSlots;

  switch (frameClass) {
    case FIXFIX:

      createDefFrameInfo(numberTimeSlots,
                         hSbrGrid->bs_num_env,
                         hSbrFrameInfo);

      frameSplit = (hSbrFrameInfo->nEnvelopes > 1);
      if (freq_res_fixfix[frameSplit] == FREQ_RES_LOW) {
        for (i = 0; i < hSbrFrameInfo->nEnvelopes; i++) {
          hSbrFrameInfo->freqRes[i] = FREQ_RES_LOW;
        }
      }

      hSbrGrid->v_f[0] = hSbrFrameInfo->freqRes[0];
      break;

    case FIXVAR:
    case VARFIX:

      nEnv = hSbrGrid->n + 1;
      assert(nEnv <= MAX_ENVELOPES_FIXVAR_VARFIX);
      hSbrFrameInfo->nEnvelopes = nEnv;

      border = hSbrGrid->bs_abs_bord;

      if (nEnv == 1) {
        hSbrFrameInfo->nNoiseEnvelopes = 1;
      } else {
        hSbrFrameInfo->nNoiseEnvelopes = 2;
      }

      break;

    default:

      break;
  }

  switch (frameClass) {
    case FIXVAR:

      hSbrFrameInfo->borders[0] = 0;
      hSbrFrameInfo->borders[nEnv] = border;
      for (k = 0, i = nEnv - 1; k < nEnv - 1; k++, i--) {
        border -= v_r[k];
        hSbrFrameInfo->borders[i] = border;
      }

      p = hSbrGrid->p;
      if (p == 0) {
        hSbrFrameInfo->shortEnv = 0;
      } else {
        hSbrFrameInfo->shortEnv = nEnv + 1 - p;
      }

      for (k = 0, i = nEnv - 1; k < nEnv; k++, i--) {
        hSbrFrameInfo->freqRes[i] = v_f[k];
      }

      if (p == 0 || p == 1) {
        hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
      } else {
        hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
      }

      break;

    case VARFIX:

      hSbrFrameInfo->borders[0] = border;
      for (k = 0; k < nEnv - 1; k++) {
        border += v_r[k];
        hSbrFrameInfo->borders[k + 1] = border;
      }
      hSbrFrameInfo->borders[nEnv] = numberTimeSlots;

      p = hSbrGrid->p;
      if (p == 0 || p == 1) {
        hSbrFrameInfo->shortEnv = 0;
      } else {
        hSbrFrameInfo->shortEnv = p - 1;
      }

      for (k = 0; k < nEnv; k++) {
        hSbrFrameInfo->freqRes[k] = v_f[k];
      }

      switch (p) {
        case 0:

          hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[1];
          break;
        case 1:

          hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
          break;
        default:

          hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
          break;
      }
      break;

    case VARVAR:

      nEnv = hSbrGrid->bs_num_rel_0 + hSbrGrid->bs_num_rel_1 + 1;
      assert(nEnv <= MAX_ENVELOPES_VARVAR);
      hSbrFrameInfo->nEnvelopes = nEnv;

      hSbrFrameInfo->borders[0] = border = hSbrGrid->bs_abs_bord_0;
      for (k = 0, i = 1; k < hSbrGrid->bs_num_rel_0; k++, i++) {
        border += hSbrGrid->bs_rel_bord_0[k];
        hSbrFrameInfo->borders[i] = border;
      }

      border = hSbrGrid->bs_abs_bord_1;
      hSbrFrameInfo->borders[nEnv] = border;
      for (k = 0, i = nEnv - 1; k < hSbrGrid->bs_num_rel_1; k++, i--) {
        border -= hSbrGrid->bs_rel_bord_1[k];
        hSbrFrameInfo->borders[i] = border;
      }

      p = hSbrGrid->p;
      if (p == 0) {
        hSbrFrameInfo->shortEnv = 0;
      } else {
        hSbrFrameInfo->shortEnv = nEnv + 1 - p;
      }

      for (k = 0; k < nEnv; k++) {
        hSbrFrameInfo->freqRes[k] = hSbrGrid->v_fLR[k];
      }

      if (nEnv == 1) {
        hSbrFrameInfo->nNoiseEnvelopes = 1;
        hSbrFrameInfo->bordersNoise[0] = hSbrGrid->bs_abs_bord_0;
        hSbrFrameInfo->bordersNoise[1] = hSbrGrid->bs_abs_bord_1;
      } else {
        hSbrFrameInfo->nNoiseEnvelopes = 2;
        hSbrFrameInfo->bordersNoise[0] = hSbrGrid->bs_abs_bord_0;

        if (p == 0 || p == 1) {
          hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
        } else {
          hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
        }
        hSbrFrameInfo->bordersNoise[2] = hSbrGrid->bs_abs_bord_1;
      }
      break;

    default:

      break;
  }

  if (frameClass == VARFIX || frameClass == FIXVAR) {
    hSbrFrameInfo->bordersNoise[0] = hSbrFrameInfo->borders[0];
    if (nEnv == 1) {
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv];
    } else {
      hSbrFrameInfo->bordersNoise[2] = hSbrFrameInfo->borders[nEnv];
    }
  }
}

HANDLE_SBR_FRAME_INFO
FrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame,
                   int *v_transient_info,
                   int rightBorderFIX,
                   CODEC_TYPE coreCodec,
                   const int switchingDecision,
                   const int fixfixGridGranularity) {
  int numEnv = 0, tranPosInternal = 0, bmin = 0, bmax = 0, numParts = 0, d = 0;
  int cmonBorderIdx = 0, tranIdx = 0, numBordersFromLeft = 0, fmax = 0;

  int *aBorders = hSbrEnvFrame->aBorders;
  FREQ_RES *aFreqRes = hSbrEnvFrame->aFreqRes;
  int *aBordersFollow = hSbrEnvFrame->aBordersFollow;
  FREQ_RES *aFreqResFollow = hSbrEnvFrame->aFreqResFollow;

  int *pBorderVecLenFollow = &hSbrEnvFrame->borderVecLenFollow;
  int *pFreqResVecLenFollow = &hSbrEnvFrame->freqResVecLenFollow;
  int *pBorderVecLen = &hSbrEnvFrame->borderVecLen;
  int *pFreqResVecLen = &hSbrEnvFrame->freqResVecLen;
  int *pSpreadFlag = &hSbrEnvFrame->spreadFlag;
  int *pTranIdxFollow = &hSbrEnvFrame->tranIdxFollow;
  int *pFillIdxFollow = &hSbrEnvFrame->fillIdxFollow;
  FRAME_CLASS *frameClassOld = &hSbrEnvFrame->frameClassOld;
  FRAME_CLASS frameClass = {0};

  int numberTimeSlots = hSbrEnvFrame->hSbrGrid->numberTimeSlots;
  int frameMiddleSlot = hSbrEnvFrame->frameMiddleSlot;

  int tranPos = v_transient_info[0];
  int tranFlag = v_transient_info[1];

  FRAME_GEN_TUNING tuning = hSbrEnvFrame->tuning;

  hSbrEnvFrame->hSbrGrid->bs_num_env = 0;

  if (tuning.staticFraming) {
    frameClass = FIXFIX;
    *frameClassOld = FIXFIX;

    numEnv = tuning.numEnvStatic;

    hSbrEnvFrame->hSbrGrid->bs_num_env = numEnv;
    hSbrEnvFrame->hSbrGrid->frameClass = frameClass;
    if (coreCodec != CODEC_SAAC && hSbrEnvFrame->hSbrGrid->bs_num_env > 4) {
      hSbrEnvFrame->hSbrGrid->bs_num_env = 4;
    }
  } else {
    calcFrameClass(&frameClass, frameClassOld, tranFlag && (!rightBorderFIX), pSpreadFlag);

    if (tranFlag && (!rightBorderFIX)) {
      tranPosInternal = frameMiddleSlot + tranPos;

      fillFrameTran(tuning.segmentLength,
                    tuning.segmentRes,
                    tuning.freqResFillPost,
                    tranPosInternal,
                    aBorders,
                    pBorderVecLen,
                    aFreqRes,
                    pFreqResVecLen,
                    &bmin,
                    &bmax);

      calcFillLengthMax(coreCodec,
                        numberTimeSlots,
                        tranPos,
                        &fmax);
    }

    switch (frameClass) {
      case FIXVAR:

        fillFramePre(tuning.dmax,
                     tuning.freqResFillPre,
                     bmin,
                     bmin,
                     aBorders, pBorderVecLen,
                     aFreqRes, pFreqResVecLen);

        fillFramePost(numberTimeSlots,
                      tuning.dmax,
                      tuning.freqResFillPost,
                      fmax,
                      bmax,
                      aBorders,
                      pBorderVecLen,
                      aFreqRes,
                      pFreqResVecLen,
                      &numParts,
                      &d);

        if (numParts == 1 && d < tuning.dmin) {
          specialCase(tuning.allowSpread,
                      pSpreadFlag,
                      aBorders,
                      pBorderVecLen,
                      aFreqRes,
                      pFreqResVecLen,
                      &numParts,
                      d);
        }

        calcCmonBorder(numberTimeSlots,
                       tranPosInternal,
                       aBorders, pBorderVecLen,
                       &cmonBorderIdx,
                       &tranIdx);

        keepForFollowUp(numberTimeSlots,
                        aBorders,
                        pBorderVecLen,
                        aFreqRes,
                        cmonBorderIdx,
                        tranIdx,
                        numParts,
                        aBordersFollow,
                        pBorderVecLenFollow,
                        aFreqResFollow,
                        pFreqResVecLenFollow,
                        pTranIdxFollow,
                        pFillIdxFollow);

        calcSbrGrid(FIXVAR,
                    tuning.freqResFillPre,
                    aBorders,
                    *pBorderVecLen,
                    aFreqRes,
                    *pFreqResVecLen,
                    cmonBorderIdx,
                    tranIdx,
                    *pSpreadFlag,
                    DC,
                    hSbrEnvFrame->hSbrGrid);
        break;

      case VARFIX:

        calcSbrGrid(VARFIX,
                    tuning.freqResFillPre,
                    aBordersFollow, *pBorderVecLenFollow,
                    aFreqResFollow, *pFreqResVecLenFollow,
                    DC,
                    *pTranIdxFollow,
                    *pSpreadFlag,
                    DC,
                    hSbrEnvFrame->hSbrGrid);
        break;

      case VARVAR:

        if (*pSpreadFlag) {
          calcSbrGrid(VARVAR,
                      tuning.freqResFillPre,
                      aBordersFollow, *pBorderVecLenFollow,
                      aFreqResFollow, *pFreqResVecLenFollow,
                      DC,
                      *pTranIdxFollow,
                      1,
                      DC,
                      hSbrEnvFrame->hSbrGrid);

          *pSpreadFlag = 0;

          aBordersFollow[0] = hSbrEnvFrame->hSbrGrid->bs_abs_bord_1 - numberTimeSlots;
          aFreqResFollow[0] = FREQ_RES_HIGH;
          *pBorderVecLenFollow = 1;
          *pFreqResVecLenFollow = 1;

          *pTranIdxFollow = -DC;
          *pFillIdxFollow = -DC;
        } else {
          fillFrameInter(numberTimeSlots,
                         tuning,
                         aBordersFollow, pBorderVecLenFollow,
                         aFreqResFollow, pFreqResVecLenFollow,
                         *pFillIdxFollow,
                         bmin,
                         aBorders, pBorderVecLen,
                         aFreqRes, pFreqResVecLen,
                         &numBordersFromLeft);

          fillFramePost(numberTimeSlots,
                        tuning.dmax,
                        tuning.freqResFillPost,
                        fmax,
                        bmax,
                        aBorders,
                        pBorderVecLen,
                        aFreqRes,
                        pFreqResVecLen,
                        &numParts,
                        &d);

          if (numParts == 1 && d < tuning.dmin) {
            specialCase(tuning.allowSpread,
                        pSpreadFlag,
                        aBorders,
                        pBorderVecLen,
                        aFreqRes,
                        pFreqResVecLen,
                        &numParts,
                        d);
          }

          calcCmonBorder(numberTimeSlots,
                         tranPosInternal,
                         aBorders,
                         pBorderVecLen,
                         &cmonBorderIdx,
                         &tranIdx);

          keepForFollowUp(numberTimeSlots,
                          aBorders,
                          pBorderVecLen,
                          aFreqRes,
                          cmonBorderIdx,
                          tranIdx,
                          numParts,
                          aBordersFollow,
                          pBorderVecLenFollow,
                          aFreqResFollow,
                          pFreqResVecLenFollow,
                          pTranIdxFollow,
                          pFillIdxFollow);

          calcSbrGrid(VARVAR,
                      tuning.freqResFillPre,
                      aBorders, *pBorderVecLen,
                      aFreqRes, *pFreqResVecLen,
                      cmonBorderIdx,
                      tranIdx,
                      0,
                      numBordersFromLeft,
                      hSbrEnvFrame->hSbrGrid);
        }
        break;

      case FIXFIX:

        if (tranPos == 0) {
          numEnv = 1;
        } else if ((tranPos == 1) && (coreCodec != CODEC_SAAC)) {
          numEnv = 2;
        } else if (coreCodec != CODEC_SAAC) {
          numEnv = 4;
        } else {
          if (switchingDecision) {
            numEnv = fixfixGridGranularity;
          } else {
            numEnv = 2;
          }
        }

        hSbrEnvFrame->hSbrGrid->bs_num_env = numEnv;
        hSbrEnvFrame->hSbrGrid->frameClass = frameClass;
        break;

      default:
        assert(0);
    }
  }

  sbrGrid2FrameInfo(hSbrEnvFrame->hSbrGrid,
                    hSbrEnvFrame->hSbrFrameInfo,
                    tuning.freq_res_fixfix);

  return hSbrEnvFrame->hSbrFrameInfo;
}

static void
calcFillLengthMaxSibi(CODEC_TYPE coreCodec,
                      const int numberTimeSlots,
                      int *pfmax) {
  switch (coreCodec) {
    case CODEC_SAAC:
      switch (numberTimeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          *pfmax = 4;
          break;
        default:
          *pfmax = 0;
          break;
      }
      break;
    default:
      *pfmax = 4;
  }
}

static void
updateSbrGridSibi(int *aBordersFollow,
                  int *pBorderVecLenFollow,
                  FREQ_RES *aFreqResFollow,
                  int *pFreqResVecLenFollow,
                  int *pTranIdxFollow,
                  int fmax) {
  int i;

  while (*pBorderVecLenFollow > 1) {
    (*pBorderVecLenFollow)--;
    (*pFreqResVecLenFollow)--;
  }

  for (i = 1; i < MAX_ENVELOPES_FIXVAR_VARFIX; i++) {
    aBordersFollow[i] = aBordersFollow[i - 1] + fmax;
    (*pBorderVecLenFollow)++;

    aFreqResFollow[i] = FREQ_RES_HIGH;
    (*pFreqResVecLenFollow)++;
  }

  if (*pTranIdxFollow >= 0) {
    *pTranIdxFollow = EMPTY;
  }
}

static void
calcFrameClassSibi(FRAME_CLASS *frameClass,
                   FRAME_CLASS *frameClassOld) {
  *frameClass = SBR_FRAME_CLASS_INVALID;

  switch (*frameClassOld) {
    case FIXFIX:
    case VARFIX:
      *frameClass = FIXFIX;
      break;

    case FIXVAR:
    case VARVAR:
      *frameClass = VARFIX;
      break;

    default:
      assert(0);
  }

  *frameClassOld = *frameClass;
}

HANDLE_SBR_FRAME_INFO
FrameInfoGeneratorSibilant(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame,
                           const int *v_transient_info,
                           CODEC_TYPE coreCodec,
                           const int switchingDecision,
                           const int fixfixGridGranularity) {
  int numEnv = 0;

  int fmax = 0;

  int *aBordersFollow = hSbrEnvFrame->aBordersFollow;
  FREQ_RES *aFreqResFollow = hSbrEnvFrame->aFreqResFollow;

  int *pBorderVecLenFollow = &hSbrEnvFrame->borderVecLenFollow;
  int *pFreqResVecLenFollow = &hSbrEnvFrame->freqResVecLenFollow;

  int *pSpreadFlag = &hSbrEnvFrame->spreadFlag;
  int *pTranIdxFollow = &hSbrEnvFrame->tranIdxFollow;

  FRAME_CLASS *frameClassOld = &hSbrEnvFrame->frameClassOld;
  FRAME_CLASS frameClass;

  int numberTimeSlots = hSbrEnvFrame->hSbrGrid->numberTimeSlots;

  const int tranPos = v_transient_info[0];

  FRAME_GEN_TUNING tuning = hSbrEnvFrame->tuning;

  hSbrEnvFrame->hSbrGrid->bs_num_env = 0;

  if (tuning.staticFraming) {
    frameClass = FIXFIX;
    *frameClassOld = FIXFIX;

    numEnv = tuning.numEnvStatic;

    hSbrEnvFrame->hSbrGrid->bs_num_env = numEnv;
    hSbrEnvFrame->hSbrGrid->frameClass = frameClass;
    if (coreCodec != CODEC_SAAC && hSbrEnvFrame->hSbrGrid->bs_num_env > 4) {
      hSbrEnvFrame->hSbrGrid->bs_num_env = 4;
    }

  } else {
    calcFrameClassSibi(&frameClass, frameClassOld);

    switch (frameClass) {
      case VARFIX:

        calcFillLengthMaxSibi(coreCodec,
                              numberTimeSlots,
                              &fmax);

        updateSbrGridSibi(aBordersFollow, pBorderVecLenFollow,
                          aFreqResFollow, pFreqResVecLenFollow,
                          pTranIdxFollow,
                          fmax);

        calcSbrGrid(VARFIX,
                    tuning.freqResFillPre,
                    aBordersFollow, *pBorderVecLenFollow,
                    aFreqResFollow, *pFreqResVecLenFollow,
                    DC,
                    *pTranIdxFollow,
                    *pSpreadFlag,
                    DC,
                    hSbrEnvFrame->hSbrGrid);

        break;

      case FIXFIX:

        if (tranPos == 0) {
          numEnv = 1;
        } else {
          if (switchingDecision) {
            if (coreCodec == CODEC_SAAC) {
              numEnv = fixfixGridGranularity;
            } else {
              numEnv = min(4, fixfixGridGranularity);
            }
          } else {
            numEnv = 2;
          }
        }

        hSbrEnvFrame->hSbrGrid->bs_num_env = numEnv;
        hSbrEnvFrame->hSbrGrid->frameClass = frameClass;
        break;

      default:
        assert(0);
        break;
    }
  }

  sbrGrid2FrameInfo(hSbrEnvFrame->hSbrGrid,
                    hSbrEnvFrame->hSbrFrameInfo,
                    tuning.freq_res_fixfix);

  return hSbrEnvFrame->hSbrFrameInfo;
}

HANDLE_ERROR_INFO
CreateFrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME *hSbrEnvFrame,
                         int allowSpread,
                         int numEnvStatic,
                         int staticFraming,
                         FREQ_RES freqResFillPre,
                         FREQ_RES freqResFillPost,
                         int timeSlots,
                         FREQ_RES *freq_res_fixfix,
                         CODEC_TYPE coreCodec,
                         SBR_UNUSED sbrConfigurationPtr params) {
  HANDLE_SBR_ENVELOPE_FRAME hs;
  hs =
      (HANDLE_SBR_ENVELOPE_FRAME)iisCalloc(1, sizeof(SBR_ENVELOPE_FRAME));

  if (!hs) {
    return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->hSbrFrameInfo = (HANDLE_SBR_FRAME_INFO)iisCalloc(1, sizeof(SBR_FRAME_INFO));
  if (!hs->hSbrFrameInfo) {
    return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->hSbrGrid = (HANDLE_SBR_GRID)iisCalloc(1, sizeof(SBR_GRID));
  if (!hs->hSbrGrid) {
    return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->frameClassOld = FIXFIX;
  hs->spreadFlag = 0;
  hs->borderVecLen = 0;
  hs->borderVecLenFollow = 0;
  hs->freqResVecLen = 0;
  hs->freqResVecLenFollow = 0;
  hs->tranIdxFollow = 0;
  hs->fillIdxFollow = 0;
  hs->hSbrGrid->numberTimeSlots = timeSlots;
  hs->sibBordersCnt = 0;
  hs->lastTilt = 0;

  hs->tuning.staticFraming = staticFraming;
  hs->tuning.numEnvStatic = numEnvStatic;
  hs->tuning.freq_res_fixfix[0] = freq_res_fixfix[0];
  hs->tuning.freq_res_fixfix[1] = freq_res_fixfix[1];
  hs->tuning.allowSpread = allowSpread;

  hs->tuning.segmentLength[0] = 0;
  hs->tuning.segmentLength[1] = 2;
  hs->tuning.segmentLength[2] = 4;
  hs->tuning.segmentRes[0] = FREQ_RES_LOW;
  hs->tuning.segmentRes[1] = FREQ_RES_LOW;
  hs->tuning.segmentRes[2] = FREQ_RES_LOW;
  hs->tuning.freqResFillPre = freqResFillPre;
  hs->tuning.freqResFillPost = freqResFillPost;
  hs->tuning.minEnvSize4highRes = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      switch (timeSlots) {
        case NUMBER_TIME_SLOTS_2048:
          hs->tuning.dmin = 4;
          hs->tuning.dmax = 12;
          hs->frameMiddleSlot = FRAME_MIDDLE_SLOT_2048;
          break;
        default:
          assert(0);
      }
      break;
    default:
      assert(0);
  }

  switch (coreCodec) {
    case CODEC_SAAC:
      break;
    default:
      assert(0);
  }

  *hSbrEnvFrame = hs;
  return noError;
}

void DeleteFrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame) {
  if (hSbrEnvFrame) {
    if (hSbrEnvFrame->hSbrGrid)
      iisFree(hSbrEnvFrame->hSbrGrid);

    if (hSbrEnvFrame->hSbrFrameInfo)
      iisFree(hSbrEnvFrame->hSbrFrameInfo);

    iisFree(hSbrEnvFrame);
  }
}
