
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
#include <stdlib.h>
#include <string.h>

#include "aacenc_internal.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "iisSigMap.h"
#include "qc_data.h"
#include "adj_thr_data.h"
#include "pns.h"
#include "bit_enc.h"
#include "quantize.h"
#include "sf_estim.h"
#include "glob_con.h"
#include "qc_main.h"
#include "adj_thr.h"
#include "iisBitFrame.h"
#include "bit_aac.h"
#include "dyn_bits.h"

typedef struct {
  QCDATA_BR_MODE bitrateMode;
  float vbrQualFactor[2];
} IISAACFENC_TAB_VBR_QUAL_FACTOR;

static const IISAACFENC_TAB_VBR_QUAL_FACTOR tableVbrQualFactorAAC[] = {
    {QCDATA_BR_MODE_CBR, {0.0f}},
    {QCDATA_BR_MODE_VBR_1, {0.001953125f}},
    {QCDATA_BR_MODE_VBR_2, {0.001953125f}},
    {QCDATA_BR_MODE_VBR_3, {0.0023193359375f}},
    {QCDATA_BR_MODE_VBR_4, {0.00164794921875f}},
    {QCDATA_BR_MODE_VBR_5, {0.0008544921875f}},
    {QCDATA_BR_MODE_VBR_6, {0.00054931640625f}}};

static const IISAACFENC_TAB_VBR_QUAL_FACTOR tableVbrQualFactorXHE[] = {
    {QCDATA_BR_MODE_CBR, {0.0f, 0.0f}},
    {QCDATA_BR_MODE_VBR_1, {0.00226480375f, 0.00146484375f}},
    {QCDATA_BR_MODE_VBR_2, {0.001480537109375f, 0.001800537109375f}},
    {QCDATA_BR_MODE_VBR_3, {0.0023193359375f, 0.0023193359375f}},
    {QCDATA_BR_MODE_VBR_4, {0.00164794921875f, 0.00164794921875f}},
    {QCDATA_BR_MODE_VBR_5, {0.0008544921875f, 0.0008544921875f}},
    {QCDATA_BR_MODE_VBR_6, {0.00054931640625f, 0.00054931640625f}},
    {QCDATA_BR_MODE_VBR_0, {0.003082275390625f, 0.003082275390625f}}};

static const int maxBitsPerChannel = 6144;

static int iisaacfenc_calcMaxValueInSfb(int sfbCnt,
                                        int maxSfbPerGroup,
                                        int sfbPerGroup,
                                        int sfbOffset[MAX_GROUPED_SFB],
                                        signed int quantSpectrum[FRAME_LEN_LONG],
                                        unsigned int maxValue[MAX_GROUPED_SFB]);

static void iisaacfenc_equalizeScfOfZeroQuantizedSfb(int *quantizedSpectrum,
                                                     int *scalefac,
                                                     float *mdctSpectrum,
                                                     const int sfbCnt,
                                                     int sfbPerGroup,
                                                     int maxSfbPerGroup,
                                                     int *sfbOffs) {
  int sfbOffset;
  int sfb1;
  int sfb2;
  int l;
  int sumQuant;
  int anyScfUnequal;

  for (sfbOffset = 0; sfbOffset < sfbCnt; sfbOffset += sfbPerGroup) {
    anyScfUnequal = 0;
    for (sfb1 = maxSfbPerGroup - 1; sfb1 >= 0; sfb1--) {
      sumQuant = 0;
      if (scalefac[sfbOffset + maxSfbPerGroup - 1] - scalefac[sfbOffset + sfb1] != 0)
        anyScfUnequal = 1;
      for (l = sfbOffs[sfbOffset + sfb1]; l < sfbOffs[sfbOffset + sfb1 + 1]; l++) {
        sumQuant += abs(quantizedSpectrum[l]);
      }
      if (sumQuant > 0 || sfb1 == 0)
        break;
    }

    if (anyScfUnequal != 0) {
      int offset = sfbOffs[sfbOffset + sfb1];
      int numLines = sfbOffs[sfbOffset + maxSfbPerGroup] - sfbOffs[sfbOffset + sfb1];
      setFLOAT(0.f, mdctSpectrum + offset, numLines);
      for (sfb2 = sfb1 + 1; sfb2 < maxSfbPerGroup; sfb2++) {
        scalefac[sfbOffset + sfb2] = scalefac[sfbOffset + sfb1];
      }
    }
  }
}

static int iisaacfenc_EBNew(ELEMENT_BITS **eB) {
  if (*eB == NULL) {
    *eB = (ELEMENT_BITS *)iisCalloc(sizeof(ELEMENT_BITS), 1);
  }
  return (*eB == 0);
}

static void iisaacfenc_EBDelete(ELEMENT_BITS *eB) {
  if (eB != 0)
    iisFree(eB);
}

static int iisaacfenc_InitElementBits(QC_STATE hQC,
                                      const CHANNEL_MAPPING *const cm,
                                      int bitrateTot,
                                      int averageBitsTot,
                                      int staticBitsTot) {
  int error = 0;
  int maxChannelBits = maxBitsPerChannel;
  int i, lastEl = -1;
  float bitFacSum = 0.0f;
  int avgBits, avgBitsSum = 0;
  int maxBitResBits;
  unsigned int maxLFEBitsVbr = 1000;
  unsigned int LFEBitsPerChannelVbr;

  LFEBitsPerChannelVbr = (maxLFEBitsVbr + cm->nEffectiveChannels - 1) / cm->nEffectiveChannels;

  avgBits = (int)(averageBitsTot - staticBitsTot);
  maxBitResBits = (int)(maxChannelBits * cm->nEffectiveChannels - averageBitsTot);
  maxBitResBits -= (maxBitResBits % 8);

  for (i = 0; i < cm->nElements; i++) {
    if ((cm->elInfo[i].elType == ID_SCE)) {
      hQC.elementBits[i]->chBitrate = (int)(cm->elInfo[i].relativeBits * bitrateTot);
      hQC.elementBits[i]->averageBits = (int)(cm->elInfo[i].relativeBits * avgBits);
      if (hQC.bitrateMode == QCDATA_BR_MODE_CBR) {
        hQC.elementBits[i]->maxBits = maxChannelBits;
      } else {
        hQC.elementBits[i]->maxBits = maxChannelBits - LFEBitsPerChannelVbr;
      }

      hQC.elementBits[i]->maxBitResBits = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->bitResLevel = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->relativeBits = cm->elInfo[i].relativeBits;

      avgBitsSum += hQC.elementBits[i]->averageBits;
      bitFacSum += hQC.elementBits[i]->relativeBits;
      lastEl = i;
    }
    if ((cm->elInfo[i].elType == ID_CPE)) {
      hQC.elementBits[i]->chBitrate = (int)(cm->elInfo[i].relativeBits * 0.5f * bitrateTot);
      hQC.elementBits[i]->averageBits = (int)(cm->elInfo[i].relativeBits * avgBits);
      if (hQC.bitrateMode == QCDATA_BR_MODE_CBR) {
        hQC.elementBits[i]->maxBits = 2 * maxChannelBits;
      } else {
        hQC.elementBits[i]->maxBits = 2 * maxChannelBits - (2 * LFEBitsPerChannelVbr);
      }

      hQC.elementBits[i]->maxBitResBits = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->bitResLevel = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->relativeBits = cm->elInfo[i].relativeBits;

      avgBitsSum += hQC.elementBits[i]->averageBits;
      bitFacSum += hQC.elementBits[i]->relativeBits;
      lastEl = i;
    }
    if (cm->elInfo[i].elType == ID_LFE) {
      hQC.elementBits[i]->chBitrate = (int)(cm->elInfo[i].relativeBits * bitrateTot);
      hQC.elementBits[i]->averageBits = (int)(cm->elInfo[i].relativeBits * avgBits);
      if (hQC.bitrateMode == QCDATA_BR_MODE_CBR) {
        hQC.elementBits[i]->maxBits = maxChannelBits;
      } else {
        hQC.elementBits[i]->maxBits = maxLFEBitsVbr;
      }

      hQC.elementBits[i]->maxBitResBits = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->bitResLevel = (int)(cm->elInfo[i].relativeBits * maxBitResBits);
      hQC.elementBits[i]->relativeBits = cm->elInfo[i].relativeBits;

      avgBitsSum += hQC.elementBits[i]->averageBits;
      bitFacSum += hQC.elementBits[i]->relativeBits;
      lastEl = i;
    }
  }

  if (lastEl >= 0) {
    hQC.elementBits[lastEl]->averageBits += (averageBitsTot - staticBitsTot) - avgBitsSum;
    hQC.elementBits[lastEl]->relativeBits += (1.0f - bitFacSum);
  } else {
    error = 1;
  }

  return error;
}

static int iisaacfenc_increaseChannelGain(
    INTERN_CORE_MODE *coreMode,
    const CHANNEL_MAPPING *const cm,
    int el,
    int ch,
    QC_OUT_CHANNEL *qcOutChannel,
    QC_STATE *qcKernel,
    int *chConstraintsFulfilled,
    int *chDynBits,
    int *maxChDynBits,
    int *iter) {
  int error = 0;
  ELEMENT_INFO elInfo = cm->elInfo[el];

  if (coreMode[elInfo.ChannelIndex[ch]] == INTERN_CORE_MODE_FD) {
    if (qcOutChannel->globalGain < 255) {
      if (qcKernel->bitrateMode == QCDATA_BR_MODE_CBR) {
        if ((*iter > 0) || ((chConstraintsFulfilled == 0) || (chDynBits[ch] > maxChDynBits[ch]))) {
          qcOutChannel->globalGain++;
          if (qcKernel->useNoiseFilling && elInfo.elType != ID_LFE) {
            if (qcOutChannel->noiseLevel % 32 > 0) {
              qcOutChannel->noiseLevel--;
            } else if (qcOutChannel->noiseLevel / 32 > 0) {
              qcOutChannel->noiseLevel -= 32;
            }
          }
        }
      } else {
        if ((*iter > 0) || (chConstraintsFulfilled == 0)) {
          qcOutChannel->globalGain++;
          if (qcKernel->useNoiseFilling && elInfo.elType != ID_LFE) {
            if (qcOutChannel->noiseLevel % 32 > 0) {
              qcOutChannel->noiseLevel--;
            } else if (qcOutChannel->noiseLevel / 32 > 0) {
              qcOutChannel->noiseLevel -= 32;
            }
          }
        }
      }
      *iter += 1;
    } else {
      error = -1;
    }
  }

  return error;
}

static void iisaacfenc_QuantizeSpectrum_wrapper(QC_OUT_CHANNEL *qcOutChannel,
                                                PSY_OUT_CHANNEL *psyOutChannel,
                                                QC_STATE *qcKernel,
                                                ELEMENT_TYPE elType,
                                                int iter) {
  if ((!qcKernel->invQuant) || (iter > 0)) {
    iisaacfenc_QuantizeSpectrum(psyOutChannel->quantizerData,
                                psyOutChannel->sfbCnt,
                                psyOutChannel->maxSfbPerGroup,
                                psyOutChannel->sfbPerGroup,
                                psyOutChannel->sfbOffsets,
                                psyOutChannel->mdctSpectrum,
                                qcOutChannel->globalGain,
                                qcOutChannel->scf,
                                qcKernel->useLloydMaxQuantizer,
                                qcOutChannel->quantSpec);

    if (qcKernel->useNoiseFilling == 0 || elType == ID_LFE) {
      iisaacfenc_equalizeScfOfZeroQuantizedSfb(qcOutChannel->quantSpec,
                                               qcOutChannel->scf,
                                               psyOutChannel->mdctSpectrum,
                                               psyOutChannel->sfbCnt,
                                               psyOutChannel->sfbPerGroup,
                                               psyOutChannel->maxSfbPerGroup,
                                               psyOutChannel->sfbOffsets);
    }
  }
}

int iisaacfenc_QCOutNew(QC_OUT **phQC, int nChannels, int nElements, int granuleLength) {
  int error = 0;
  int i;
  QC_OUT *hQC = NULL;

  if (*phQC == NULL) {
    *phQC = (QC_OUT *)iisCalloc(sizeof(QC_OUT), 1);
    error = (*phQC == NULL);
  }

  if (!error) {
    hQC = *phQC;
    for (i = 0; i < nChannels; i++) {
      if (hQC->qcChannel[i] == NULL) {
        hQC->qcChannel[i] = (QC_OUT_CHANNEL *)iisCalloc(sizeof(QC_OUT_CHANNEL), 1);
        if (hQC->qcChannel[i] == NULL) {
          error = 1;
          break;
        }
      }
      if (hQC->qcChannel[i]->quantSpec != NULL) {
        iisFree(hQC->qcChannel[i]->quantSpec);
      }
      hQC->qcChannel[i]->quantSpec = (int *)iisCalloc(granuleLength, sizeof(int));
      hQC->qcChannel[i]->granuleLength = granuleLength;
      if (hQC->qcChannel[i]->maxValueInSfb == NULL) {
        hQC->qcChannel[i]->maxValueInSfb = (unsigned int *)iisCalloc(MAX_GROUPED_SFB, sizeof(unsigned int));
      }
      if (hQC->qcChannel[i]->scf == NULL) {
        hQC->qcChannel[i]->scf = (int *)iisCalloc(MAX_GROUPED_SFB, sizeof(int));
      }
      if (hQC->qcChannel[i]->synthTime != NULL) {
        iisFree(hQC->qcChannel[i]->synthTime);
      }
      hQC->qcChannel[i]->synthTime = (float *)iisCalloc(2 * granuleLength, sizeof(float));
      if (hQC->qcChannel[i]->quantSpec == NULL ||
          hQC->qcChannel[i]->maxValueInSfb == NULL ||
          hQC->qcChannel[i]->scf == NULL ||
          hQC->qcChannel[i]->synthTime == NULL) {
        error = 1;
        break;
      }
    }
    for (i = 0; i < nElements; i++) {
      if (hQC->qcElement[i] == NULL) {
        hQC->qcElement[i] = (QC_OUT_ELEMENT *)iisCalloc(sizeof(QC_OUT_ELEMENT), 1);
        if (hQC->qcElement[i] == NULL) {
          error = 1;
          break;
        }
      }
    }
  }
  if (error) {
    iisaacfenc_QCOutDelete(hQC);
    hQC = 0;
  }

  return error;
}

void iisaacfenc_QCOutDelete(QC_OUT *hQC) {
  int i;
  if (hQC) {
    for (i = 0; i < SIGMAP_MAX_ELEMENTS; i++) {
      if (hQC->qcElement[i]) iisFree(hQC->qcElement[i]);
    }

    for (i = 0; i < SIGMAP_MAX_SIGNALS; i++) {
      if (hQC->qcChannel[i]) {
        if (hQC->qcChannel[i]->quantSpec) iisFree(hQC->qcChannel[i]->quantSpec);
        if (hQC->qcChannel[i]->maxValueInSfb) iisFree(hQC->qcChannel[i]->maxValueInSfb);
        if (hQC->qcChannel[i]->scf) iisFree(hQC->qcChannel[i]->scf);
        if (hQC->qcChannel[i]->synthTime) iisFree(hQC->qcChannel[i]->synthTime);
        iisFree(hQC->qcChannel[i]);
      }
    }
    iisFree(hQC);
  }
}

int iisaacfenc_QCNew(QC_STATE **phQC, int nElements) {
  int i;
  int error = 0;
  QC_STATE *hQC = NULL;

  if (*phQC == NULL) {
    (*phQC) = (QC_STATE *)iisCalloc(sizeof(QC_STATE), 1);
    if ((*phQC) == NULL) {
      error = 1;
    }
  }

  if (!error) {
    hQC = *phQC;
    error = iisaacfenc_AdjThrNew(&(hQC->hAdjThr), nElements);
    if (error) {
      iisaacfenc_QCDelete(hQC);
      hQC = NULL;
    }
  }

  if (!error) {
    error = iisaacfenc_BCNew(&(hQC->hBitCounter));
    if (error) {
      iisaacfenc_QCDelete(hQC);
      hQC = NULL;
    }
  }

  if (!error) {
    for (i = 0; i < nElements; i++) {
      error = iisaacfenc_EBNew(&(hQC->elementBits[i]));
      if (error) {
        iisaacfenc_QCDelete(hQC);
        hQC = NULL;
        break;
      }
    }
  }

  return error;
}

void iisaacfenc_QCDelete(QC_STATE *hQC) {
  int i;
  if (hQC) {
    iisaacfenc_AdjThrDelete(hQC->hAdjThr);
    iisaacfenc_BCDelete(hQC->hBitCounter);
    for (i = 0; i < SIGMAP_MAX_ELEMENTS; i++) {
      iisaacfenc_EBDelete(hQC->elementBits[i]);
      hQC->elementBits[i] = 0;
    }
    iisFree(hQC);
  }
}

int iisaacfenc_QCInit(QC_STATE *hQC,
                      struct QC_INIT *init) {
  int error = 0;

  if ((init == NULL) || (hQC == NULL)) {
    error = 1;
  }

  if (error == 0) {
    hQC->nChannels = init->channelMapping->nChannels;
    hQC->nElements = init->channelMapping->nElements;
    hQC->averageBitsTot = init->averageBits;
    hQC->maxBitFac = init->maxBitFac;
    hQC->bitrateMode = init->bitrateMode;
    hQC->useNoiseFilling = init->useNoiseFilling;
    hQC->globStatBits = 3;
  }

  if (error == 0) {
    error = iisaacfenc_InitElementBits(*hQC,
                                       init->channelMapping,
                                       init->bitrate,
                                       init->averageBits,
                                       hQC->globStatBits);
  }

  if (error == 0) {
    switch (hQC->bitrateMode) {
      case QCDATA_BR_MODE_CBR:
      case QCDATA_BR_MODE_VBR_0:
      case QCDATA_BR_MODE_VBR_1:
      case QCDATA_BR_MODE_VBR_2:
      case QCDATA_BR_MODE_VBR_3:
      case QCDATA_BR_MODE_VBR_4:
      case QCDATA_BR_MODE_VBR_5:
      case QCDATA_BR_MODE_VBR_6:
        if (init->codecType == AACENC_CODEC_AAC) {
          hQC->vbrQualFactor = tableVbrQualFactorAAC[hQC->bitrateMode].vbrQualFactor[0];
        } else if (init->codecType == AACENC_CODEC_XHEAAC) {
          if (init->channelMapping->cicpLayoutIndex == SIGMAP_CICP_1) {
            hQC->vbrQualFactor = tableVbrQualFactorXHE[hQC->bitrateMode].vbrQualFactor[0];
          } else {
            hQC->vbrQualFactor = tableVbrQualFactorXHE[hQC->bitrateMode].vbrQualFactor[1];
          }
        } else if (init->codecType == AACENC_CODEC_MPEGH) {
          hQC->vbrQualFactor = tableVbrQualFactorAAC[hQC->bitrateMode].vbrQualFactor[0];
        } else {
          error = 1;
        }
        break;
      case QCDATA_BR_MODE_INVALID:
      default:
        hQC->vbrQualFactor = 0.0f;
        error = 1;
        break;
    }
  }

  if (error == 0) {
    int bUseVbr = ((hQC->bitrateMode == QCDATA_BR_MODE_VBR_0) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_1) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_2) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_3) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_4) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_5) ||
                   (hQC->bitrateMode == QCDATA_BR_MODE_VBR_6));

    iisaacfenc_AdjThrInit(hQC->hAdjThr,
                          init->meanPe,
                          hQC->elementBits,
                          init->channelMapping->nElements,
                          bUseVbr,
                          hQC->vbrQualFactor);
  }

  if (error == 0) {
    error = iisaacfenc_BCInit(hQC->hBitCounter,
                              init->useNoiseFilling,
                              hQC->sideInfoTabLong,
                              hQC->sideInfoTabShort);
  }

  if (error == 0) {
    hQC->invQuant = init->invQuant;
    hQC->useLloydMaxQuantizer = init->useLloydMaxQuantizer;
  }

  return error;
}

static int iisaacfenc_calcMaxValueInSfb(int sfbCnt,
                                        int maxSfbPerGroup,
                                        int sfbPerGroup,
                                        int sfbOffset[MAX_GROUPED_SFB],
                                        signed int quantSpectrum[FRAME_LEN_LONG],
                                        unsigned int maxValue[MAX_GROUPED_SFB]) {
  int sfbOffs, sfb;
  int maxValueAll = 0;

  for (sfbOffs = 0; sfbOffs < sfbCnt; sfbOffs += sfbPerGroup) {
    for (sfb = 0; sfb < maxSfbPerGroup; sfb++) {
      int line;
      const int lastLine = sfbOffset[sfbOffs + sfb + 1];
      int maxThisSfb = 0;

      for (line = sfbOffset[sfbOffs + sfb]; line < lastLine; line++) {
        if (abs(quantSpectrum[line]) > maxThisSfb) {
          maxThisSfb = abs(quantSpectrum[line]);
        }
      }

      maxValue[sfbOffs + sfb] = maxThisSfb;
      if (maxThisSfb > maxValueAll) {
        maxValueAll = maxThisSfb;
      }
    }
  }

  return maxValueAll;
}

int iisaacfenc_FinalizeBitConsumption(CHANNEL_MAPPING *const cm,
                                      QC_STATE *const qcKernel,
                                      QC_OUT *qcOut, INTERN_CORE_MODE *coreMode) {
  int i;
  int ch = 0, isAac = 0, chIdx[2] = {0};

  qcOut->totStaticBitsUsed = qcKernel->globStatBits;
  qcOut->totDynBitsUsed = 0;

  for (i = 0; i < cm->nElements; i++) {
    chIdx[0] = cm->elInfo[i].ChannelIndex[0];
    chIdx[1] = cm->elInfo[i].ChannelIndex[1];

    isAac = 0;
    for (ch = 0; ch < cm->elInfo[i].nChannelsInEl; ch++) {
      if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD) {
        isAac = 1;
      }
    }
    if (cm->elInfo[i].elType != ID_DSE && isAac) {
      qcOut->totStaticBitsUsed += qcOut->qcElement[i]->staticBitsUsed;
      qcOut->totDynBitsUsed += qcOut->qcElement[i]->dynBitsUsed;
    }
  }

  return 0;
}

int iisaacfenc_AdjustBitrate(QC_STATE *hQC,
                             CHANNEL_MAPPING *cm,
                             int frameLen,
                             int dseBits) {
  int i;
  int codeBits;
  int codeBitsLast;

  codeBitsLast = hQC->averageBitsTot - hQC->globStatBits - hQC->dseBitsLast;
  codeBits = frameLen - dseBits - hQC->globStatBits;

  for (i = (cm->nElements - 1); i >= 0; i--) {
    if (cm->elInfo[i].elType != ID_DSE) {
      hQC->elementBits[i]->relativeBitsVar = hQC->elementBits[i]->relativeBits;
    }
  }

  if (codeBits != codeBitsLast) {
    int firstEl = 0;
    int totalBits = 0;

    for (i = (cm->nElements - 1); i >= 0; i--) {
      if (cm->elInfo[i].elType != ID_DSE) {
        hQC->elementBits[i]->averageBits = (int)(hQC->elementBits[i]->relativeBitsVar * codeBits);
        totalBits += hQC->elementBits[i]->averageBits;
        firstEl = i;
      }
    }
    hQC->elementBits[firstEl]->averageBits += codeBits - totalBits;
  }

  hQC->averageBitsTot = frameLen;
  hQC->dseBitsLast = dseBits;

  return 0;
}

static void iisaacfenc_transferBitEncChannelMainData(BIT_ENC_CHANNEL_DATA *bEncData,
                                                     int *aQuantSpectrum,
                                                     unsigned int sizeAQuantSpectrum,
                                                     QC_OUT_CHANNEL *loopResult,
                                                     PSY_OUT_CHANNEL *hPsyOut) {
  int sec;

  bEncData->bs_scalefac_data.globalGain = loopResult->globalGain - 4 * LOG_NORM_PCM;
  bEncData->bs_scalefac_data.noiseLevel = loopResult->noiseLevel;
  memcpy(bEncData->bs_scalefac_data.scalefac, loopResult->scf, sizeof(bEncData->bs_scalefac_data.scalefac));

  bEncData->bs_scalefac_data.isScalefacCoding = 1;
  memcpy(bEncData->bs_scalefac_data.isPosition, hPsyOut->isScale, sizeof(bEncData->bs_scalefac_data.isPosition));

  bEncData->bs_scalefac_data.nsScalefacCoding = 1;
  memcpy(bEncData->bs_scalefac_data.noiseNrg, hPsyOut->noiseNrg, sizeof(bEncData->bs_scalefac_data.noiseNrg));

  bEncData->bs_section_data.noOfSections = loopResult->sectionData.noOfSections;
  bEncData->bs_section_data.firstSCF = loopResult->sectionData.firstScf;

  for (sec = 0; sec < bEncData->bs_section_data.noOfSections; sec++) {
    bEncData->bs_section_data.section[sec].codeBook = loopResult->sectionData.section[sec].codeBook;
    bEncData->bs_section_data.section[sec].sfbStart = loopResult->sectionData.section[sec].sfbStart;
    bEncData->bs_section_data.section[sec].sfbCnt = loopResult->sectionData.section[sec].sfbCnt;
  }

  assert(sizeAQuantSpectrum * sizeof(*(aQuantSpectrum)) <= sizeof(bEncData->aQuantSpectrum));
  memcpy(bEncData->aQuantSpectrum, aQuantSpectrum, sizeAQuantSpectrum * sizeof(*(aQuantSpectrum)));

  memset(bEncData->nmr, 0, sizeof(bEncData->nmr));
  memset(bEncData->xfsf, 0, sizeof(bEncData->xfsf));
}

int iisaacfenc_QCMain(PSY_OUT *psyOut,
                      QC_OUT *qcOut,
                      QC_STATE *qcKernel,
                      HANDLE_BITSTREAM_ENC hBsEnc,
                      IISBITFRAME_HANDLE hBitFrame,
                      CHANNEL_MAPPING *cm,
                      INTERN_CORE_MODE *coreMode,
                      INTERN_CORE_MODE *coreModePrev,
                      int bUsacIndepFlag) {
  int el = 0;
  int ch = 0;
  int error = 0;
  int maxDynBits[SIGMAP_MAX_ELEMENTS] = {0};
  int totalConstraintsFulfilled = 0;
  int totalChDynBits[SIGMAP_MAX_ELEMENTS][SIGMAP_MAX_CHANNELS] = {{0}, {0}};
  int totalFrameDynBits = 0;
  int totalMaxFrameDynBits = 0;
  int totalMaxChDynBits[SIGMAP_MAX_ELEMENTS][SIGMAP_MAX_CHANNELS] = {{0}, {0}};
  int totalMaxElDynBits[SIGMAP_MAX_ELEMENTS] = {0};
  int nChannelsReQuant = 0;
  int iter = 0;
  int maxValueInSfb = 0;
  int totDynBitsNoEmergency[SIGMAP_MAX_ELEMENTS] = {0};
  int totElDynBits[SIGMAP_MAX_ELEMENTS] = {0};
  int hasFDChannels = 0;
  int iterLimit = cm->nElements * 64;
  int headerBitsPerChannelVbr = 0;

  headerBitsPerChannelVbr = (hBsEnc->frameData.totHeaderBits + cm->nEffectiveChannels - 1) / cm->nEffectiveChannels;

  for (el = 0; el < cm->nElements && error == 0; el++) {
    ELEMENT_INFO elInfo = cm->elInfo[el];
    int nFdChannelsInElement = 0;
    int headerBitsPerElementVbr = 0;

    if (qcKernel->bitrateMode != QCDATA_BR_MODE_CBR && qcKernel->bitrateMode != QCDATA_BR_MODE_INVALID) {
      switch (elInfo.elType) {
        case ID_SCE:
          headerBitsPerElementVbr = headerBitsPerChannelVbr;
          break;
        case ID_CPE:
          headerBitsPerElementVbr = 2 * headerBitsPerChannelVbr;
          break;
        case ID_LFE:
          headerBitsPerElementVbr = 0;
          break;
        default:
          headerBitsPerElementVbr = 0;
          break;
      }
    }

    if (elInfo.elType == ID_SCE || elInfo.elType == ID_CPE || elInfo.elType == ID_LFE) {
      PSY_OUT_CHANNEL *psyOutChannel[2] = {NULL, NULL};
      QC_OUT_CHANNEL *qcOutChannel[2] = {NULL, NULL};

      for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
        if (coreMode[elInfo.ChannelIndex[ch]] == INTERN_CORE_MODE_FD) {
          nFdChannelsInElement++;
          psyOutChannel[ch] = psyOut->psyOutChannel[elInfo.ChannelIndex[ch]];
          qcOutChannel[ch] = qcOut->qcChannel[elInfo.ChannelIndex[ch]];

          if (coreModePrev[elInfo.ChannelIndex[ch]] == INTERN_CORE_MODE_LPD) {
            resetSpectralDataArithFrameCount2(hBsEnc->bitEncChannelData[elInfo.ChannelIndex[ch]].hEncSpecDataArith);
          }
        }
      }

      if (nFdChannelsInElement > 0) {
        ELEMENT_BITS *elBits = qcKernel->elementBits[el];
        int bitReservoir = IISBITFRAME_GetBitreservoir(hBitFrame);
        int bitReservoirMax = IISBITFRAME_GetBitreservoirMax(hBitFrame);
        elBits->bitResLevel = (int)(bitReservoir * elBits->relativeBits);
        elBits->maxBitResBits = (int)(bitReservoirMax * elBits->relativeBits);

        if (elBits->bitResLevel < 0) {
          error = 2;
          break;
        }
        if (elBits->bitResLevel > elBits->maxBitResBits) {
          error = 3;
          break;
        }

        qcOut->qcElement[el]->staticBitsUsed = hBsEnc->frameData.elemSideInfoBits[el];

        iisaacfenc_EstimateNoiseFillingAndScaleFactors(psyOutChannel,
                                                       qcOutChannel,
                                                       elInfo.nChannelsInEl,
                                                       &coreMode[elInfo.ChannelIndex[0]],
                                                       qcKernel->invQuant,
                                                       qcKernel->useLloydMaxQuantizer,
                                                       qcKernel->useNoiseFilling && elInfo.elType != ID_LFE);

        if (qcKernel->bitrateMode == QCDATA_BR_MODE_CBR) {
          maxDynBits[el] = elBits->averageBits +
                           elBits->bitResLevel -
                           7 -
                           qcOut->qcElement[el]->staticBitsUsed;
        } else {
          maxDynBits[el] = elBits->maxBits -
                           7 -
                           qcOut->qcElement[el]->staticBitsUsed;

          if (elInfo.elType == ID_SCE || elInfo.elType == ID_CPE) {
            maxDynBits[el] -= headerBitsPerElementVbr;
          }
        }
      }
    }
  }

  do {
    int chConstraintsFulfilled[SIGMAP_MAX_SIGNALS] = {0};
    totalFrameDynBits = 0;
    totalMaxFrameDynBits = 0;

    for (el = 0; el < cm->nElements && error == 0; el++) {
      ELEMENT_INFO elInfo = cm->elInfo[el];
      int chIdx[SIGMAP_MAX_SIGNALS_PER_ELEMENT] = {0};

      if (elInfo.elType == ID_SCE || elInfo.elType == ID_CPE || elInfo.elType == ID_LFE) {
        int nFdChannelsInElement = 0;
        QC_OUT_CHANNEL *qcOutChannel[2] = {NULL, NULL};
        PSY_OUT_CHANNEL *psyOutChannel[2] = {NULL, NULL};

        for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
          chIdx[ch] = elInfo.ChannelIndex[ch];
          if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD) {
            psyOutChannel[nFdChannelsInElement] = psyOut->psyOutChannel[chIdx[ch]];
            qcOutChannel[nFdChannelsInElement] = qcOut->qcChannel[chIdx[ch]];
            nFdChannelsInElement++;
          }
        }

        if (nFdChannelsInElement >= 1) {
          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            chConstraintsFulfilled[ch] = 1;
          }
          totElDynBits[el] = 0;

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            if (coreMode[elInfo.ChannelIndex[ch]] == INTERN_CORE_MODE_FD) {
              totalMaxChDynBits[el][ch] = (int)floor(qcKernel->elementBits[el]->chBitDistribution[ch] * maxDynBits[el]);
              totalMaxElDynBits[el] = maxDynBits[el];

              iisaacfenc_QuantizeSpectrum_wrapper(qcOutChannel[ch],
                                                  psyOutChannel[ch],
                                                  qcKernel,
                                                  elInfo.elType,
                                                  iter);

              maxValueInSfb = iisaacfenc_calcMaxValueInSfb(psyOutChannel[ch]->sfbCnt,
                                                           psyOutChannel[ch]->maxSfbPerGroup,
                                                           psyOutChannel[ch]->sfbPerGroup,
                                                           psyOutChannel[ch]->sfbOffsets,
                                                           qcOutChannel[ch]->quantSpec,
                                                           qcOutChannel[ch]->maxValueInSfb);

              while (maxValueInSfb > MAX_QUANT) {
                error = iisaacfenc_increaseChannelGain(
                    coreMode,
                    cm,
                    el,
                    ch,
                    qcOutChannel[ch],
                    qcKernel,
                    &chConstraintsFulfilled[ch],
                    &totalChDynBits[el][ch],
                    &totalMaxChDynBits[el][ch],
                    &iter);
                if (error == 0) {
                  iisaacfenc_QuantizeSpectrum_wrapper(qcOutChannel[ch],
                                                      psyOutChannel[ch],
                                                      qcKernel,
                                                      elInfo.elType,
                                                      iter);
                  maxValueInSfb = iisaacfenc_calcMaxValueInSfb(psyOutChannel[ch]->sfbCnt,
                                                               psyOutChannel[ch]->maxSfbPerGroup,
                                                               psyOutChannel[ch]->sfbPerGroup,
                                                               psyOutChannel[ch]->sfbOffsets,
                                                               qcOutChannel[ch]->quantSpec,
                                                               qcOutChannel[ch]->maxValueInSfb);
                } else {
                  break;
                }
              }

              qcKernel->hBitCounter->useNoiseFilling = (qcKernel->useNoiseFilling && elInfo.elType != ID_LFE);

              totalChDynBits[el][ch] = iisaacfenc_dynBitCount(qcKernel->hBitCounter,
                                                              qcOutChannel[ch]->quantSpec,
                                                              qcOutChannel[ch]->maxValueInSfb,
                                                              qcOutChannel[ch]->scf,
                                                              psyOutChannel[ch]->windowSequence,
                                                              psyOutChannel[ch]->sfbCnt,
                                                              psyOutChannel[ch]->maxSfbPerGroup,
                                                              psyOutChannel[ch]->sfbPerGroup,
                                                              psyOutChannel[ch]->sfbOffsets,
                                                              &qcOutChannel[ch]->sectionData,
                                                              psyOutChannel[ch]->noiseNrg,
                                                              psyOutChannel[ch]->isScale, psyOutChannel[ch]->groupingMask,
                                                              bUsacIndepFlag,
                                                              hBsEnc->bitEncChannelData[chIdx[ch]].hEncSpecDataArith);

              if (totalChDynBits[el][ch] == -1) {
                chConstraintsFulfilled[ch] = 0;
              }
              totElDynBits[el] += totalChDynBits[el][ch];
            }
          }
          if (iter == 0) {
            totDynBitsNoEmergency[el] = totElDynBits[el];
          }
        }
      }
    }

    if (iter > iterLimit) {
      error = 4;
    }

    for (el = 0; el < cm->nElements && error == 0; el++) {
      ELEMENT_INFO elInfo = cm->elInfo[el];

      totalMaxFrameDynBits += totalMaxElDynBits[el];

      for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
        int chIdx[SIGMAP_MAX_SIGNALS_PER_ELEMENT] = {0};
        chIdx[ch] = elInfo.ChannelIndex[ch];

        if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD && hasFDChannels == 0) {
          hasFDChannels = 1;
        }

        totalFrameDynBits += totalChDynBits[el][ch];
      }
    }

    if (totalFrameDynBits > totalMaxFrameDynBits && error == 0 && hasFDChannels) {
      int chCount = 0;
      int chIdx[SIGMAP_MAX_SIGNALS_PER_ELEMENT] = {0};

      totalConstraintsFulfilled = 0;
      nChannelsReQuant++;

      if (nChannelsReQuant > cm->nChannels) {
        nChannelsReQuant = 1;
      }
      for (el = 0; el < cm->nElements; el++) {
        ELEMENT_INFO elInfo = cm->elInfo[el];
        if (elInfo.elType == ID_SCE || elInfo.elType == ID_CPE || elInfo.elType == ID_LFE) {
          QC_OUT_CHANNEL *qcOutChannel[2] = {NULL, NULL};

          for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
            chCount++;
            chIdx[ch] = elInfo.ChannelIndex[ch];
            qcOutChannel[ch] = qcOut->qcChannel[chIdx[ch]];
            if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_FD) {
              if (chCount == nChannelsReQuant) {
                error = iisaacfenc_increaseChannelGain(
                    coreMode,
                    cm,
                    el,
                    ch,
                    qcOutChannel[ch],
                    qcKernel,
                    &chConstraintsFulfilled[ch],
                    &totalChDynBits[el][ch],
                    &totalMaxChDynBits[el][ch],
                    &iter);
              }
            }
          }
        }
      }
    } else {
      totalConstraintsFulfilled = 1;
    }
  } while (!totalConstraintsFulfilled);

  for (el = 0; el < cm->nElements; el++) {
    ATS_ELEMENT *adjThrStateElement = qcKernel->hAdjThr->adjThrStateElem[el];
    qcOut->qcElement[el]->dynBitsUsed = totElDynBits[el];

    iisaacfenc_AdjThrUpdate(adjThrStateElement, totDynBitsNoEmergency[el]);
  }

  for (el = 0; el < cm->nElements && error == 0; el++) {
    ELEMENT_INFO elInfo = cm->elInfo[el];

    if (elInfo.elType == ID_SCE || elInfo.elType == ID_CPE || elInfo.elType == ID_LFE) {
      QC_OUT_CHANNEL *qcOutChannel = NULL;
      PSY_OUT_CHANNEL *psyOutChannel[2] = {NULL, NULL};

      for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
        if (coreMode[elInfo.ChannelIndex[ch]] == INTERN_CORE_MODE_FD) {
          qcOutChannel = qcOut->qcChannel[elInfo.ChannelIndex[ch]];
          psyOutChannel[ch] = psyOut->psyOutChannel[elInfo.ChannelIndex[ch]];

          qcOutChannel->groupingMask = psyOutChannel[ch]->groupingMask;
          qcOutChannel->windowShape = psyOutChannel[ch]->windowShape;

          if (qcOutChannel->noiseLevel < 32) {
            assert(qcOutChannel->noiseLevel == 0);
          }

          iisaacfenc_transferBitEncChannelMainData(&hBsEnc->bitEncChannelData[elInfo.ChannelIndex[ch]],
                                                   qcOutChannel->quantSpec,
                                                   qcOutChannel->granuleLength,
                                                   qcOutChannel,
                                                   psyOutChannel[ch]);
        }
      }
    }
  }

  return error;
}
