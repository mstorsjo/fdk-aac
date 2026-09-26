
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
#include "mathlib.h"

#include "spaceEnclib_const.h"
#include "space_tree.h"
#include "space_paramextract.h"

struct SPACE_TREE {
  SPACETREE_MODE mode;
  SPACE_TREE_DESCRIPTION descr;
  HANDLE_TTO_BOX ttoBox[MAX_NUM_BOXES];
  int nParamBands;
  int bOneIcc;
  int bUseCoarseQuantTtoIcc;
  int bUseCoarseQuantTtoCld;

  int bUseCoarseQuantTtoIpd;

  QUANTMODE quantMode;
  int bCalcResiduals[MAX_NUM_BOXES];
  int bCalcDPL;
  int frameCount;
  int bLowDelay;
  int bFrameKeep;

  unsigned int nResidualMapping[MAX_NUM_BOXES];

  float **ppBandEnergies;
  float *pBandEnergiesSum[MAX_NUM_BOXES];
  float *pParamBandEnergyFactor[2 * MAX_NUM_BOXES];
  float *pParamBandEnergyFactorZero;
  int *pCld_prev[MAX_NUM_BOXES];
  int *pIcc_prev[MAX_NUM_BOXES];

  float epsilonFloat;
  int nChannelsInMax;
  int nTimeSlotsMax;
  int nHybridBandsMax;

  TTO_MIX_MATRIX *pDmxPrev;
  TTO_MIX_MATRIX *pDmxCurr;
  int bInterpolateDownmix;

  int bPsStyleDmx;

  int bUseTsd;
  int bUsac212;
};

enum {
  BOX_0 = 0,
  BOX_1 = 1,
  BOX_2 = 2,
  BOX_3 = 3,
  BOX_4 = 4,
  BOX_5 = 5,
  BOX_6 = 6,
  BOX_7 = 7
};

enum {
  MAX_KEEP_FRAMECOUNT = 100
};

static const SPACE_TREE_DESCRIPTION SpaceTreeConfigTable[] = {
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {5, 0, 1, 6, 0, {0, 0, 0, 0, 1, 0, 0}},
    {5, 0, 1, 6, 0, {0, 0, 1, 0, 0, 0, 0}},
    {3, 1, 2, 6, 0, {1, 0, 0, 0, 0, 0, 0}},
    {5, 1, 2, 8, 0, {1, 0, 0, 0, 0, 0, 0}},
    {5, 1, 2, 8, 0, {1, 0, 0, 0, 0, 0, 0}},
    {2, 0, 6, 8, 0, {0, 0, 0, 0, 0, 0, 0}},
    {2, 0, 6, 8, 0, {0, 0, 0, 0, 0, 0, 0}},
    {1, 0, 1, 2, 0, {0, 0, 0, 0, 0, 0, 0}},
    {7, 1, 2, 10, 0, {1, 0, 0, 0, 0, 0, 0}},
    {4, 0, 6, 10, 0, {0, 0, 0, 0, 0, 0, 0}},
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}},
    {1, 0, 1, 2, 0, {0, 0, 0, 0, 0, 0, 0}}};

static CPLX CPLXsub(CPLX a, CPLX b) {
  CPLX c;

  c.real = a.real - b.real;
  c.imag = a.imag - b.imag;

  return c;
}

static CPLX CPLXmult(CPLX a, CPLX b) {
  CPLX c;

  c.real = a.real * b.real - a.imag * b.imag;
  c.imag = a.real * b.imag + a.imag * b.real;

  return c;
}

static int invertMat22(const CPLX M[2][2], CPLX Minv[2][2]) {
  CPLX det, detInv;
  float detAbs2;
  float eps = 1.0e-6f;

  det = CPLXsub(CPLXmult(M[0][0], M[1][1]),
                CPLXmult(M[0][1], M[1][0]));
  detAbs2 = det.real * det.real + det.imag * det.imag;

  if (detAbs2 < eps) {
    return 1;
  }

  detInv.real = det.real / detAbs2;
  detInv.imag = -det.imag / detAbs2;

  Minv[0][0] = CPLXmult(M[1][1], detInv);
  Minv[0][1] = CPLXmult(M[0][1], detInv);
  Minv[1][0] = CPLXmult(M[1][0], detInv);
  Minv[1][1] = CPLXmult(M[0][0], detInv);
  Minv[0][1].real *= -1;
  Minv[0][1].imag *= -1;
  Minv[1][0].real *= -1;
  Minv[1][0].imag *= -1;

  return 0;
}

static void invertMixMatrix(const TTO_MIX_MATRIX *mix,
                            TTO_MIX_MATRIX *mixInv) {
  mixInv->infinity = invertMat22(mix->m, mixInv->m);
}

static void interpolateMixMatrix(const TTO_MIX_MATRIX *from,
                                 const TTO_MIX_MATRIX *to,
                                 const float alpha,
                                 TTO_MIX_MATRIX *out) {
  int ich, och;

  if (!to->infinity) {
    if (!from->infinity) {
      float beta = 1.0f - alpha;

      for (och = 0; och < 2; och++) {
        for (ich = 0; ich < 2; ich++) {
          out->m[och][ich].real = beta * from->m[och][ich].real + alpha * to->m[och][ich].real;
          out->m[och][ich].imag = beta * from->m[och][ich].imag + alpha * to->m[och][ich].imag;
        }
      }
    } else {
      for (och = 0; och < 2; och++) {
        for (ich = 0; ich < 2; ich++) {
          out->m[och][ich].real = alpha * to->m[och][ich].real;
          out->m[och][ich].imag = alpha * to->m[och][ich].imag;
        }
      }
    }
  }

  out->infinity = to->infinity;
}

static HANDLE_ERROR_INFO calculateDownmix(HANDLE_SPACE_TREE hST,
                                          int nTimeSlots,
                                          int nHybridBands,
                                          int paramSetSlotPrev,
                                          int paramSetSlot,
                                          float ***pppHybridInReal,
                                          float ***pppHybridInImag,
                                          float ***pppHybridOutReal,
                                          float ***pppHybridOutImag,
                                          float **ppUmxMatReal[2][2],
                                          float **ppUmxMatImag[2][2]) {
  HANDLE_ERROR_INFO error = noError;

  int bCalcResiduals = hST->bCalcResiduals[0];

  TTO_MIX_MATRIX *dmxPrev = hST->pDmxPrev;
  TTO_MIX_MATRIX *dmxCurr = hST->pDmxCurr;
  TTO_MIX_MATRIX dmx;
  TTO_MIX_MATRIX umxPrev, umxCurr, umx;

  int tsFrom = paramSetSlotPrev + 1;
  int hb, ts;
  int i, j;

  memset(&umx, 0, sizeof(TTO_MIX_MATRIX));
  memset(&dmx, 0, sizeof(TTO_MIX_MATRIX));

  if ((NULL == dmxPrev) || (NULL == dmxCurr)) {
    error = iisUtil_ERROR(CDI, "Parameter error.");
  }

  SAFECALL(error, GetTtoBoxDownmixMatrix(hST->ttoBox[BOX_0], nHybridBands, dmxCurr));

  if (noError == error) {
    for (hb = 0; hb < nHybridBands; hb++) {
      invertMixMatrix(&dmxPrev[hb], &umxPrev);
      invertMixMatrix(&dmxCurr[hb], &umxCurr);

      for (ts = tsFrom; ts < nTimeSlots; ts++) {
        if (ts <= paramSetSlot) {
          float alpha = (float)(ts - paramSetSlotPrev) / (paramSetSlot - paramSetSlotPrev);

          interpolateMixMatrix(&dmxPrev[hb], &dmxCurr[hb], alpha, &dmx);

          if (bCalcResiduals) {
            interpolateMixMatrix(&umxPrev, &umxCurr, alpha, &umx);
            if (!umxPrev.infinity && !umxCurr.infinity) {
              invertMixMatrix(&umx, &dmx);
              if (dmx.infinity) {
                interpolateMixMatrix(&dmxPrev[hb], &dmxCurr[hb], alpha, &dmx);
              }
            }
          }
        }

        if (bCalcResiduals) {
          for (i = 0; i < 2; i++) {
            for (j = 0; j < 2; j++) {
              if (umx.infinity) {
                ppUmxMatReal[i][j][ts][hb] = dmx.m[i][j].real;
                ppUmxMatImag[i][j][ts][hb] = dmx.m[i][j].imag;
              } else {
                ppUmxMatReal[i][j][ts][hb] = umx.m[i][j].real;
                ppUmxMatImag[i][j][ts][hb] = umx.m[i][j].imag;
              }
            }
          }
        }

        pppHybridOutReal[0][ts][hb] = dmx.m[0][0].real * pppHybridInReal[0][ts][hb] - dmx.m[0][0].imag * pppHybridInImag[0][ts][hb];
        pppHybridOutImag[0][ts][hb] = dmx.m[0][0].real * pppHybridInImag[0][ts][hb] + dmx.m[0][0].imag * pppHybridInReal[0][ts][hb];
        pppHybridOutReal[0][ts][hb] += dmx.m[0][1].real * pppHybridInReal[1][ts][hb] - dmx.m[0][1].imag * pppHybridInImag[1][ts][hb];
        pppHybridOutImag[0][ts][hb] += dmx.m[0][1].real * pppHybridInImag[1][ts][hb] + dmx.m[0][1].imag * pppHybridInReal[1][ts][hb];

        if (hST->bUseTsd) {
          dmx.m[1][0].real = 1.0f;
          dmx.m[1][1].real = -1.0f;
          dmx.m[1][0].imag = 0.0f;
          dmx.m[1][1].imag = 0.0f;
        }

        assert(!(bCalcResiduals && hST->bUseTsd));
        if ((bCalcResiduals) || (hST->bUseTsd)) {
          pppHybridOutReal[1][ts][hb] = dmx.m[1][0].real * pppHybridInReal[0][ts][hb] - dmx.m[1][0].imag * pppHybridInImag[0][ts][hb];
          pppHybridOutImag[1][ts][hb] = dmx.m[1][0].real * pppHybridInImag[0][ts][hb] + dmx.m[1][0].imag * pppHybridInReal[0][ts][hb];
          pppHybridOutReal[1][ts][hb] += dmx.m[1][1].real * pppHybridInReal[1][ts][hb] - dmx.m[1][1].imag * pppHybridInImag[1][ts][hb];
          pppHybridOutImag[1][ts][hb] += dmx.m[1][1].real * pppHybridInImag[1][ts][hb] + dmx.m[1][1].imag * pppHybridInReal[1][ts][hb];
        }
      }

      dmxPrev[hb] = dmxCurr[hb];
    }
  }

  return error;
}

static BOX_QUANTMODE
mapQuantModeToBoxQuantMode(QUANTMODE const inQuantMode) {
  switch (inQuantMode) {
    case QUANTMODE_FINE:
      return BOX_QUANTMODE_FINE;
    case QUANTMODE_EBQ1:
      return BOX_QUANTMODE_EBQ1;
    case QUANTMODE_EBQ2:
      return BOX_QUANTMODE_EBQ2;
    case QUANTMODE_INVALID:
    default:
      return BOX_QUANTMODE_INVALID;
  }
}

HANDLE_ERROR_INFO
SpaceTree_CalcDownmixHold(HANDLE_SPACE_TREE hST,
                          int nparamSet,
                          int paramSet,
                          int nTimeSlots,
                          int nHybridBands,
                          SPATIALFRAME *hSTOut,
                          float ***pppHybridInReal,
                          float ***pppHybridInImag,
                          float **ppUmxMatReal[2][2],
                          float **ppUmxMatImag[2][2],
                          float ***pppHybridOutReal,
                          float ***pppHybridOutImag) {
  HANDLE_ERROR_INFO error = noError;

  int bCalcResiduals = hST->bCalcResiduals[0];

  TTO_MIX_MATRIX *dmxPrev = hST->pDmxPrev;
  TTO_MIX_MATRIX dmx;
  TTO_MIX_MATRIX umx;

  int hb, ts, tsFrom, tsTo;
  int i, j;

  memset(&umx, 0, sizeof(TTO_MIX_MATRIX));

  if (paramSet == 0) {
    tsFrom = 0;
  } else {
    tsFrom = hSTOut->framingInfo.bsParamSlots[paramSet - 1] + 1;
  }

  if (paramSet == nparamSet - 1) {
    tsTo = nTimeSlots;
  } else {
    tsTo = hSTOut->framingInfo.bsParamSlots[paramSet] + 1;
  }

  for (hb = 0; hb < nHybridBands; hb++) {
    dmx = dmxPrev[hb];

    if (bCalcResiduals) {
      invertMixMatrix(&dmx, &umx);
    }

    for (ts = tsFrom; ts < tsTo; ts++) {
      if (bCalcResiduals) {
        for (i = 0; i < 2; i++) {
          for (j = 0; j < 2; j++) {
            if (umx.infinity) {
              ppUmxMatReal[i][j][ts][hb] = dmx.m[i][j].real;
              ppUmxMatImag[i][j][ts][hb] = dmx.m[i][j].imag;
            } else {
              ppUmxMatReal[i][j][ts][hb] = umx.m[i][j].real;
              ppUmxMatImag[i][j][ts][hb] = umx.m[i][j].imag;
            }
          }
        }
      }

      pppHybridOutReal[0][ts][hb] = dmx.m[0][0].real * pppHybridInReal[0][ts][hb] - dmx.m[0][0].imag * pppHybridInImag[0][ts][hb];
      pppHybridOutImag[0][ts][hb] = dmx.m[0][0].real * pppHybridInImag[0][ts][hb] + dmx.m[0][0].imag * pppHybridInReal[0][ts][hb];
      pppHybridOutReal[0][ts][hb] += dmx.m[0][1].real * pppHybridInReal[1][ts][hb] - dmx.m[0][1].imag * pppHybridInImag[1][ts][hb];
      pppHybridOutImag[0][ts][hb] += dmx.m[0][1].real * pppHybridInImag[1][ts][hb] + dmx.m[0][1].imag * pppHybridInReal[1][ts][hb];

      if (hST->bUseTsd) {
        dmx.m[1][0].real = 1.0f;
        dmx.m[1][1].real = -1.0f;
        dmx.m[1][0].imag = 0.0f;
        dmx.m[1][1].imag = 0.0f;
      }

      if ((bCalcResiduals) || (hST->bUseTsd)) {
        pppHybridOutReal[1][ts][hb] = dmx.m[1][0].real * pppHybridInReal[0][ts][hb] - dmx.m[1][0].imag * pppHybridInImag[0][ts][hb];
        pppHybridOutImag[1][ts][hb] = dmx.m[1][0].real * pppHybridInImag[0][ts][hb] + dmx.m[1][0].imag * pppHybridInReal[0][ts][hb];
        pppHybridOutReal[1][ts][hb] += dmx.m[1][1].real * pppHybridInReal[1][ts][hb] - dmx.m[1][1].imag * pppHybridInImag[1][ts][hb];
        pppHybridOutImag[1][ts][hb] += dmx.m[1][1].real * pppHybridInImag[1][ts][hb] + dmx.m[1][1].imag * pppHybridInReal[1][ts][hb];
      }
    }
  }

  return error;
}

HANDLE_ERROR_INFO
SpaceTree_Open(HANDLE_SPACE_TREE *phSpaceTree, SPACE_TREE_SETUP *hSetup, int bLowDelay, int bFrameKeep) {
  HANDLE_ERROR_INFO error = noError;
  HANDLE_SPACE_TREE hST = NULL;
  int bTtoBoxFrontBackCombin[MAX_NUM_BOXES] = {0};
  int box = 0;
  int nResidualChannelsMax = 0;
  int nResidualChannelsTmp = 0;

  if (NULL == phSpaceTree || NULL == hSetup) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    *phSpaceTree = (HANDLE_SPACE_TREE)iisCalloc(1, sizeof(struct SPACE_TREE));
    if (*phSpaceTree == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hST = *phSpaceTree;
    }
  }

  if (noError == error) {
    hST->frameCount = 0;
    hST->bFrameKeep = bFrameKeep;
    hST->bLowDelay = bLowDelay;

    if (hSetup->mode == SPACETREE_USAC_212) {
      hST->bUsac212 = 1;
    }
  }

  if (noError == error) {
    hST->mode = hSetup->mode;
    hST->nParamBands = hSetup->nParamBands;
    hST->bOneIcc = hSetup->bOneIcc;
    hST->bUseCoarseQuantTtoIcc = hSetup->bUseCoarseQuantTtoIcc;
    hST->bUseCoarseQuantTtoCld = hSetup->bUseCoarseQuantTtoCld;

    hST->bUseCoarseQuantTtoIpd = hSetup->bUseCoarseQuantTtoIpd;

    hST->bCalcDPL = hSetup->bCalcDPL;
    hST->quantMode = hSetup->quantMode;
    hST->epsilonFloat = hSetup->epsilonFloat;
    hST->nChannelsInMax = hSetup->nChannelsInMax;
    hST->nTimeSlotsMax = hSetup->nTimeSlotsMax;
    hST->nHybridBandsMax = hSetup->nHybridBandsMax;

    hST->bInterpolateDownmix = hSetup->bInterpolateDownmix;
    hST->bPsStyleDmx = hSetup->bPsStyleDmx;
    hST->bUseTsd = hSetup->bUseTsd;

    bTtoBoxFrontBackCombin[BOX_0] = 0;
    hST->descr = SpaceTreeConfigTable[hST->mode];
    nResidualChannelsMax = hST->descr.nOttBoxes;
  }

  if (noError == error) {
    for (box = 0; box < hST->descr.nOttBoxes + hST->descr.nTttBoxes; box++) {
      hST->bCalcResiduals[box] = hSetup->bCalcResiduals[box];
      if (hST->bCalcResiduals[box]) {
        hST->nResidualMapping[box] = hST->descr.nInChannels + nResidualChannelsTmp;
        nResidualChannelsTmp++;
      } else {
        hST->nResidualMapping[box] = hST->descr.nOttBoxes + hST->descr.nTttBoxes;
      }
    }

    for (; box < MAX_NUM_BOXES; box++) {
      if (hSetup->bCalcResiduals[box]) {
        error = iisUtil_ERROR(CDI, "Invalid residual configuration specified (number of residuals exceeds number of boxes).");
      }
    }
  }

  if (noError == error) {
    if (nResidualChannelsTmp > nResidualChannelsMax) {
      error = iisUtil_ERROR(CDI, "Invalid residual configuration specified (max. number of allowed residual channels exceeded).");
    } else {
      hST->descr.nResidualChannels = nResidualChannelsTmp;
    }
  }

  if (noError == error) {
    if (NULL == (hST->ppBandEnergies = (float **)iisCallocMatrix2D(hST->nChannelsInMax, hST->nParamBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (noError == error) {
    for (box = 0; box < MAX_NUM_BOXES; box++) {
      if (NULL == (hST->pBandEnergiesSum[box] = (float *)iisCalloc(1, hST->nParamBands * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for pBandEnergiesSum[box]");
        break;
      }
    }
  }

  if (noError == error) {
    for (box = 0; box < 2 * MAX_NUM_BOXES; box++) {
      if (NULL == (hST->pParamBandEnergyFactor[box] = (float *)iisCalloc(1, hST->nParamBands * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for pParamBandEnergyFactor[box]");
        break;
      }
    }
  }

  if (noError == error) {
    if (NULL == (hST->pParamBandEnergyFactorZero = (float *)iisCalloc(1, hST->nParamBands * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for pParamBandEnergyFactorZero");
    }
  }

  if (hST->bInterpolateDownmix && error == noError) {
    if (error == noError) {
      if (NULL == (hST->pDmxPrev = (TTO_MIX_MATRIX *)iisCalloc(1, hST->nHybridBandsMax * sizeof(TTO_MIX_MATRIX)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for pDmxPrev");
      }
    }

    if (error == noError) {
      if (NULL == (hST->pDmxCurr = (TTO_MIX_MATRIX *)iisCalloc(1, hST->nHybridBandsMax * sizeof(TTO_MIX_MATRIX)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for pDmxCurr");
      }
    }
  }

  if (noError == error) {
    for (box = 0; (box < hST->descr.nOttBoxes) && (noError == error); ++box) {
      TTO_BOX_CONFIG boxConfig;
      boxConfig.bCalcResiduals = hST->bCalcResiduals[box];
      boxConfig.bCalcIccDiff = hST->bCalcResiduals[box];
      boxConfig.nResidualBands = hSetup->nResidualBands[box];
      boxConfig.nParametersMax = hST->nParamBands;
      boxConfig.subbandConfig = hST->nParamBands;
      boxConfig.bUseCoarseQuantCld = hST->bUseCoarseQuantTtoCld;
      boxConfig.bUseCoarseQuantIcc = hST->bUseCoarseQuantTtoIcc;
      boxConfig.bUseCoherenceIccOnly = bTtoBoxFrontBackCombin[box];
      boxConfig.bCalcNoIcc = hST->descr.bOttModeLfe[box];

      boxConfig.boxQuantMode = hST->descr.bOttModeLfe[box] ? BOX_QUANTMODE_FINE : mapQuantModeToBoxQuantMode(hST->quantMode);
      boxConfig.epsilonFloat = hST->epsilonFloat;
      boxConfig.nTimeSlotsMax = hST->nTimeSlotsMax;
      boxConfig.nHybridBandsMax = hST->nHybridBandsMax;
      boxConfig.bOneIcc = hST->bOneIcc;
      boxConfig.bLowDelay = hST->bLowDelay;
      boxConfig.bFrameKeep = hST->bFrameKeep;

      boxConfig.bUsac212 = hST->bUsac212;
      boxConfig.bUseCoarseQuantIpd = hST->bUseCoarseQuantTtoIpd;
      boxConfig.downmixType = hSetup->downmixType;
      boxConfig.ipdMode = hSetup->ipdMode;
      boxConfig.bDetectIpdRelevancy = hSetup->bDetectIpdRelevancy;
      boxConfig.bInterpolateDownmix = hSetup->bInterpolateDownmix;

      if (!hSetup->bCalcIccDiff && hSetup->mode == SPACETREE_USAC_212) {
        boxConfig.bCalcIccDiff = 0;
      }

      if (boxConfig.ipdMode != IPDMODE_NONE) {
        boxConfig.bUseCoherenceIccOnly = 1;
        boxConfig.bCalcIccDiff = 0;
      }

      boxConfig.nHybBandsCore = hSetup->nHybBandsCore;
      boxConfig.bStereoSbr = hSetup->bStereoSbr;
      boxConfig.nOttBandsPhase = hSetup->nOttBandsPhase;

      SAFECALL(error, CreateTtoBox(&hST->ttoBox[box], &boxConfig));
    }
  }

  return error;
}

HANDLE_ERROR_INFO
SpaceTree_Apply(HANDLE_SPACE_TREE hST,
                int paramSet,
                int nChannelsIn,
                int nTimeSlots,
                int nHybridBands,
                float ***pppHybridDmxInReal,
                float ***pppHybridDmxInImag,
                float **ppUmxMatReal[2][2],
                float **ppUmxMatImag[2][2],
                float ***pppHybridInReal,
                float ***pppHybridInImag,
                float ***pppHybridOutReal,
                float ***pppHybridOutImag,
                SPATIALFRAME *hSTOut,
                CLASSIC_MPS int avoid_keep,
                int speechFlag) {
  HANDLE_ERROR_INFO error = noError;
  int ts, hb;
  int nChOut;

  if (NULL == hST) {
    error = iisUtil_ERROR(CDI, "Invalid Space Tree Handle");
  }
  if (NULL == hSTOut) {
    error = iisUtil_ERROR(CDI, "Invalid Space Tree Output Handle");
  }

  if (error == noError) {
    if (nChannelsIn > hST->nChannelsInMax) {
      error = iisUtil_ERROR(CDI, "Invalid nChannelsIn");
    }
  }

  if (error == noError) {
    if (nChannelsIn > 10) {
      error = iisUtil_ERROR(CDI, "Invalid nChannelsIn");
    }
  }

  if (error == noError) {
    if (nTimeSlots > hST->nTimeSlotsMax) {
      error = iisUtil_ERROR(CDI, "Invalid number nTimeSlots.");
    }
  }

  if (error == noError) {
    if (nHybridBands > hST->nHybridBandsMax) {
      error = iisUtil_ERROR(CDI, "Invalid number nHybridBands.");
    }
  }

  if (error == noError) {
    if ((pppHybridInReal != NULL) &&
        (pppHybridInImag != NULL) &&
        (pppHybridOutReal != NULL) &&
        (pppHybridOutImag != NULL)) {
      int ch;
      for (ch = 0; ch < nChannelsIn; ch++) {
        if ((pppHybridInReal[ch] == NULL) ||
            (pppHybridInImag[ch] == NULL)) {
          error = iisUtil_ERROR(CDI, "Invalid pointer.");
          break;
        }
      }
      if (hST->bCalcResiduals[0]) {
        nChOut = 2;
      } else {
        nChOut = 1;
      }
      for (ch = 0; ch < nChOut; ch++) {
        if ((pppHybridOutReal[ch] == NULL) ||
            (pppHybridOutImag[ch] == NULL)) {
          error = iisUtil_ERROR(CDI, "Invalid pointer.");
          break;
        }
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer.");
    }
  }

  {
    int phaseModeTmp = 0;

    SAFECALL(error,
             ApplyTtoBox(hST->ttoBox[BOX_0],
                         nTimeSlots,
                         nHybridBands,
                         pppHybridInReal[0],
                         pppHybridInImag[0],
                         pppHybridInReal[1],
                         pppHybridInImag[1],
                         hST->bCalcResiduals[BOX_0] ? pppHybridOutReal[hST->nResidualMapping[BOX_0]] : NULL,
                         hST->bCalcResiduals[BOX_0] ? pppHybridOutImag[hST->nResidualMapping[BOX_0]] : NULL,
                         hSTOut->ottData.icc[BOX_0][paramSet],
                         &(hSTOut->ICCLosslessData.bsQuantCoarseXXX[BOX_0][paramSet]),
                         hSTOut->ottData.cld[BOX_0][paramSet],
                         &(hSTOut->CLDLosslessData.bsQuantCoarseXXX[BOX_0][paramSet]),
                         hSTOut->ottData.ipd[BOX_0][paramSet],
                         &phaseModeTmp,
                         &(hSTOut->numBinsIPD),
                         hSTOut->residualData[BOX_0].iccDiffData[paramSet].bsIccDiff,
                         &(hSTOut->residualData[BOX_0].iccDiffData[paramSet].bsIccDiffPresent),
                         hSTOut->bUseBBCues,
                         hST->mode,
                         speechFlag));
    if (error == noError) {
      hSTOut->bsPhaseMode = (phaseModeTmp & 0x2) ? 1 : 0;
      hSTOut->IPDLosslessData.bsQuantCoarseXXX[BOX_0][paramSet] = (phaseModeTmp & 0x1) ? 0 : 1;
    }
  }

  if (error == noError) {
    if (hST->bInterpolateDownmix) {
      int paramSetSlotPrev;

      if (paramSet == 0) {
        paramSetSlotPrev = -1;
      } else {
        paramSetSlotPrev = hSTOut->framingInfo.bsParamSlots[paramSet - 1];
      }

      SAFECALL(error,
               calculateDownmix(hST,
                                nTimeSlots / 2,
                                nHybridBands,
                                paramSetSlotPrev,
                                hSTOut->framingInfo.bsParamSlots[paramSet],
                                pppHybridDmxInReal,
                                pppHybridDmxInImag,
                                pppHybridOutReal,
                                pppHybridOutImag,
                                ppUmxMatReal,
                                ppUmxMatImag));
    } else {
      if (hST->bUseTsd) {
        if (pppHybridOutReal == NULL) {
          error = iisUtil_ERROR(CDI, "Invalid buffer pppHybridOutReal");
        }

        if (pppHybridOutImag == NULL) {
          error = iisUtil_ERROR(CDI, "Invalid buffer pppHybridOutImag");
        }

        if (error == noError) {
          int i, j;
          for (i = 0; i < nTimeSlots; i++) {
            for (j = 0; j < nHybridBands; j++) {
              pppHybridOutReal[1][i][j] = 0.5f * pppHybridInReal[0][i][j] - 0.5f * pppHybridInReal[1][i][j];
              pppHybridOutImag[1][i][j] = 0.5f * pppHybridInImag[0][i][j] - 0.5f * pppHybridInImag[1][i][j];
            }
          }
        }
      }

      if (error == noError) {
        for (ts = 0; ts < nTimeSlots; ts++) {
          for (hb = 0; hb < nHybridBands; hb++) {
            pppHybridOutReal[0][ts][hb] = pppHybridInReal[0][ts][hb];
            pppHybridOutImag[0][ts][hb] = pppHybridInImag[0][ts][hb];
          }
        }
      }
    }
  }
  return error;
}

HANDLE_ERROR_INFO
SpaceTree_Close(HANDLE_SPACE_TREE *phSpaceTree) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL != phSpaceTree) {
    if (NULL != *phSpaceTree) {
      int box;
      HANDLE_SPACE_TREE const hST = *phSpaceTree;

      for (box = 0; box < hST->descr.nOttBoxes; ++box) {
        error = DestroyTtoBox(&hST->ttoBox[box]);
        if (noError != error) {
          error = handBack(error);
        }
      }

      if ((*phSpaceTree)->ppBandEnergies != NULL) {
        iisFreeMatrix2D((void **)(*phSpaceTree)->ppBandEnergies);
        (*phSpaceTree)->ppBandEnergies = NULL;
      }

      for (box = 0; box < MAX_NUM_BOXES; box++) {
        if (NULL != (*phSpaceTree)->pBandEnergiesSum[box]) {
          iisFree((*phSpaceTree)->pBandEnergiesSum[box]);
        }
        (*phSpaceTree)->pBandEnergiesSum[box] = NULL;
      }

      for (box = 0; box < 2 * MAX_NUM_BOXES; box++) {
        if (NULL != (*phSpaceTree)->pParamBandEnergyFactor[box]) {
          iisFree((*phSpaceTree)->pParamBandEnergyFactor[box]);
        }
        (*phSpaceTree)->pParamBandEnergyFactor[box] = NULL;
      }
      if (NULL != (*phSpaceTree)->pParamBandEnergyFactorZero) {
        iisFree((*phSpaceTree)->pParamBandEnergyFactorZero);
      }
      (*phSpaceTree)->pParamBandEnergyFactorZero = NULL;

      if (NULL != (*phSpaceTree)->pDmxPrev) {
        iisFree((*phSpaceTree)->pDmxPrev);
      }
      (*phSpaceTree)->pDmxPrev = NULL;

      if (NULL != (*phSpaceTree)->pDmxCurr) {
        iisFree((*phSpaceTree)->pDmxCurr);
      }
      (*phSpaceTree)->pDmxCurr = NULL;

      iisFree(*phSpaceTree);
      *phSpaceTree = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO SpaceTree_GetDescription(HANDLE_SPACE_TREE hSpaceTree, SPACE_TREE_DESCRIPTION *pSpaceTreeDescription) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hSpaceTree == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (error == noError) {
    if (pSpaceTreeDescription == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid pointer.");
    }
  }

  if (error == noError) {
    *pSpaceTreeDescription = hSpaceTree->descr;
  }

  return error;
}

float SpaceTree_ParamBand2Freq(int nParamBands, int nSampleRate, int nParamBand, int nQmfBands) {
  return paramBand2Freq(nParamBands, nSampleRate, nParamBand, nQmfBands);
}

float SpaceTree_GetUniSteCld(HANDLE_SPACE_TREE hSpaceTree, int parameterBand) {
  return getUniSteCld(hSpaceTree->ttoBox[0], parameterBand);
}
