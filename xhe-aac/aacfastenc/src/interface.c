
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

#include <limits.h>
#include <float.h>

#include "mathlib.h"
#include "interface.h"
#include "sf_estim.h"

void iisaacfenc_InitPsyOutElement(const int commonWindow,
                                  const int msDigest,
                                  PSY_OUT_ELEMENT *hPsyOutElement) {
  hPsyOutElement->commonWindow = commonWindow;
  hPsyOutElement->frameEnergyF = 0.000000059604644775390625f;
  hPsyOutElement->toolsInfo.msDigest = msDigest;
  setINT(0, hPsyOutElement->toolsInfo.msMask, MAX_GROUPED_SFB);
  setINT(0, hPsyOutElement->toolsInfo.predCoefPrevReQ, MAX_GROUPED_SFB);
  setINT(0, hPsyOutElement->toolsInfo.predCoefPrevImQ, MAX_GROUPED_SFB);
  setINT(0, hPsyOutElement->toolsInfo.predCoefReQ, MAX_GROUPED_SFB);
  setINT(0, hPsyOutElement->toolsInfo.predCoefImQ, MAX_GROUPED_SFB);
  setFLOAT(0.0f, hPsyOutElement->toolsInfo.mdctSpectrumDmxPrev.Long, FRAME_LEN_LONG);
  setFLOAT(0.0f, hPsyOutElement->toolsInfo.mdctSpectrumDmx.Long, FRAME_LEN_LONG);
  hPsyOutElement->toolsInfo.bCplxPredMdctRealOnlyPrev = 0;
  hPsyOutElement->toolsInfo.bCplxPredMdct = 0;
  hPsyOutElement->toolsInfo.bCplxPredMdctActive = 0;
  hPsyOutElement->toolsInfo.bCplxPredMdctActivePrev = 0;
  hPsyOutElement->toolsInfo.bCplxPredMdctRealOnly = 1;
  hPsyOutElement->toolsInfo.tnsGainHeadroomRatioPrev = 1.f;
}

void iisaacfenc_InitPsyOutChannel(const int windowSequence,
                                  const int windowShape,
                                  const int noOfGroups,
                                  SSE_OPTI const int useCpuOptimization,
                                  const int sfbCnt,
                                  const int *sfbOffset,
                                  const int granuleLength,
                                  PSY_OUT_CHANNEL *psyOutCh) {
  int j;
  int mask;
  int grpLen = 1;

  psyOutCh->noOfGroups = noOfGroups;
  psyOutCh->sfbCnt = sfbCnt;
  psyOutCh->sfbActive = 0;
  psyOutCh->sfbPerGroup = sfbCnt;
  psyOutCh->maxSfbPerGroup = 0;
  psyOutCh->granuleLength = granuleLength;

  psyOutCh->windowSequence = windowSequence;
  psyOutCh->windowShape = windowShape;

  copyINT(sfbOffset, psyOutCh->sfbOffsets, sfbCnt + 1);

  if (windowSequence == SHORT_WINDOW) {
    smultINTip(TRANS_FAC, psyOutCh->sfbOffsets, sfbCnt + 1);
    grpLen = TRANS_FAC;
  }

  mask = 0;
  for (j = 1; j < grpLen; j++) {
    mask <<= 1;
    mask |= 1;
  }

  psyOutCh->groupingMask = mask;

  setFLOAT(0.0f, psyOutCh->sfbEnergy, sfbCnt);
  setFLOAT(FLT_MAX, psyOutCh->sfbThreshold, sfbCnt);
  setFLOAT(0.0f, psyOutCh->mdctSpectrum, granuleLength);
  setFLOAT(0.8f, psyOutCh->sfbMinSnr, sfbCnt);
  setFLOAT(1.0f, psyOutCh->sfbEnFac, sfbCnt);

  psyOutCh->sfbEnSumMS = 0.0f;

  setINT(0, psyOutCh->tnsInfo.numOfFilters, TRANS_FAC);
  setINT(INT_MIN, psyOutCh->noiseNrg, sfbCnt);

  psyOutCh->lastEnFacPatch = 1;
  psyOutCh->chaosMeasure = 0.75f;

  psyOutCh->wasVeryTonal = psyOutCh->isVeryTonal = 0;

  iisaacfenc_QuantizeInit(&psyOutCh->quantizerData, useCpuOptimization);
  iisaacfenc_sfEstimInit(&psyOutCh->sfestimData, useCpuOptimization);
}

void iisaacfenc_BuildInterface(const MDCT_SPECTRUM *groupedMdctSpectrum,
                               const SFB_THRESHOLD *groupedSfbThreshold,
                               const SFB_ENERGY *groupedSfbEnergy,
                               const SFB_ENERGY_SUM *sfbEnergySumMS,
                               const int windowSequence,
                               const int windowShape,
                               const int prevWindowShape,
                               const int groupedSfbCnt,
                               const int groupedSfbActive,
                               const int *groupedSfbOffset,
                               const int maxSfbPerGroup,
                               const float *groupedSfbMinSnr,
                               const int noOfGroups,
                               const int *groupLen,
                               const int granuleLength,
                               PSY_OUT_CHANNEL *psyOutCh) {
  int j;
  int grp;
  int mask;

  psyOutCh->maxSfbPerGroup = maxSfbPerGroup;

  psyOutCh->sfbCnt = groupedSfbCnt;
  psyOutCh->sfbActive = groupedSfbActive;
  psyOutCh->sfbPerGroup = groupedSfbCnt / noOfGroups;
  psyOutCh->windowSequence = windowSequence;
  psyOutCh->windowShape = windowShape;
  psyOutCh->prevWindowShape = prevWindowShape;
  psyOutCh->granuleLength = granuleLength;

  copyFLOAT(groupedMdctSpectrum->Long, psyOutCh->mdctSpectrum, granuleLength);
  copyINT(groupedSfbOffset, psyOutCh->sfbOffsets, groupedSfbCnt + 1);
  copyFLOAT(groupedSfbEnergy->Long, psyOutCh->sfbEnergy, groupedSfbCnt);
  copyFLOAT(groupedSfbThreshold->Long, psyOutCh->sfbThreshold, groupedSfbCnt);
  copyFLOAT(groupedSfbMinSnr, psyOutCh->sfbMinSnr, groupedSfbCnt);

  mask = 0;
  psyOutCh->noOfGroups = noOfGroups;

  for (grp = 0; grp < noOfGroups; grp++) {
    psyOutCh->groupLen[grp] = groupLen[grp];
    mask <<= 1;
    for (j = 1; j < groupLen[grp]; j++) {
      mask <<= 1;
      mask |= 1;
    }
  }
  psyOutCh->groupingMask = mask;

  if (windowSequence != SHORT_WINDOW) {
    psyOutCh->sfbEnSumMS = sfbEnergySumMS->Long;
  } else {
    psyOutCh->sfbEnSumMS = 0.0f;
    for (j = 0; j < TRANS_FAC; j++) {
      psyOutCh->sfbEnSumMS += sfbEnergySumMS->Short[j];
    }
  }
}
