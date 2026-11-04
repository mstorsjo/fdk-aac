
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

#include "mathlib.h"
#include "tns.h"
#include "grp_data.h"
#include "psy_data.h"

static void regroupSpectrum(
    MDCT_SPECTRUM *inputSpectrum,
    float *tempSpectrum,
    const int *sfbOffset,
    const int noOfSfbs,
    const int noOfGroups,
    const int *groupLen,
    const int granuleLength) {
  if (inputSpectrum && tempSpectrum) {
    int grp, sfb, off, bin;
    int wnd = 0;
    int i = 0;

    for (grp = 0; grp < noOfGroups; grp++) {
      for (sfb = 0; sfb < noOfSfbs; sfb++) {
        for (off = 0; off < groupLen[grp]; off++) {
          const float *wndSpectrum = inputSpectrum->Short[wnd + off];
          for (bin = sfbOffset[sfb]; bin < sfbOffset[sfb + 1]; bin++) {
            tempSpectrum[i++] = wndSpectrum[bin];
          }
        }
      }
      wnd += groupLen[grp];
    }
    copyFLOAT(tempSpectrum, inputSpectrum->Long, granuleLength);
  }
}

void iisaacfenc_groupShortData(
    MDCT_SPECTRUM *mdctSpectrum,
    MDCT_SPECTRUM *mdctDownmix,
    float *tempSpectrum,
    SFB_THRESHOLD *sfbThreshold,
    SFB_ENERGY *sfbEnergy,
    SFB_ENERGY *sfbEnergyMS,
    float *minUnGroupedSfbNrg,
    float *maxSfbNrgPerGroup,
    TNS_SUBBLOCK_INFO *tnsSubBlockInfo,
    float *grpdTnsPredictionGain,
    float *grpdTnsGainHeadroomRatio,
    int *grpdTnsActive,
    const int sfbCnt,
    const int sfbActive,
    const int *sfbOffset,
    const float *sfbMinSnr,
    int *groupedSfbCnt,
    int *groupedSfbActive,
    int *groupedSfbOffset,
    int *maxSfbPerGroup,
    float *groupedSfbMinSnr,
    const int noOfGroups,
    const int *groupLen,
    const int granuleLength) {
  int i, j;
  int sfb;
  int grp;
  int wnd;
  int offset;
  int highestSfb;

  *groupedSfbCnt = sfbCnt * noOfGroups;
  *groupedSfbActive = sfbCnt * noOfGroups;

  highestSfb = -1;
  for (wnd = 0; wnd < TRANS_FAC; wnd++) {
    for (sfb = sfbActive - 1; sfb > highestSfb; sfb--) {
      if (sfbEnergy->Short[wnd][sfb] > 0.0f) {
        break;
      }
    }
    if (highestSfb < sfb) {
      highestSfb = sfb;
    }
  }
  *maxSfbPerGroup = highestSfb + 1;

  i = 0;
  offset = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      groupedSfbOffset[i++] = offset + sfbOffset[sfb] * groupLen[grp];
    }
    offset += groupLen[grp] * granuleLength / TRANS_FAC;
  }
  groupedSfbOffset[i++] = granuleLength;

  i = 0;
  offset = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      groupedSfbMinSnr[i++] = sfbMinSnr[sfb];
    }
    offset += groupLen[grp] * granuleLength / TRANS_FAC;
  }

  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      float thresh = sfbThreshold->Short[wnd][sfb];
      for (j = 1; j < groupLen[grp]; j++) {
        thresh += sfbThreshold->Short[wnd + j][sfb];
      }
      sfbThreshold->Long[i++] = thresh;
    }
    wnd += groupLen[grp];
  }

  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    maxSfbNrgPerGroup[grp] = 0.0f;
    grpdTnsActive[grp] = 0;
    grpdTnsPredictionGain[grp] = 0.0f;
    grpdTnsGainHeadroomRatio[grp] = 1.0f;

    for (sfb = 0; sfb < sfbCnt; sfb++) {
      float energy = 0.0f;
      minUnGroupedSfbNrg[i] = 1.0e15f;

      for (j = 0; j < groupLen[grp]; j++) {
        energy += sfbEnergy->Short[wnd + j][sfb];
        grpdTnsActive[grp] |= tnsSubBlockInfo[wnd + j].tnsActive;
        if (grpdTnsPredictionGain[grp] < tnsSubBlockInfo[wnd + j].predictionGainMax) {
          grpdTnsPredictionGain[grp] = tnsSubBlockInfo[wnd + j].predictionGainMax;
        }
        if (grpdTnsGainHeadroomRatio[grp] < tnsSubBlockInfo[wnd + j].tnsGainHeadroomRatio) {
          grpdTnsGainHeadroomRatio[grp] = tnsSubBlockInfo[wnd + j].tnsGainHeadroomRatio;
        }
        if (sfbEnergy->Short[wnd + j][sfb] < minUnGroupedSfbNrg[i])
          minUnGroupedSfbNrg[i] = sfbEnergy->Short[wnd + j][sfb];
      }

      if (energy > maxSfbNrgPerGroup[grp])
        maxSfbNrgPerGroup[grp] = energy;

      sfbEnergy->Long[i++] = energy;
    }
    wnd += groupLen[grp];
  }

  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      float energy = sfbEnergyMS->Short[wnd][sfb];
      for (j = 1; j < groupLen[grp]; j++) {
        energy += sfbEnergyMS->Short[wnd + j][sfb];
      }
      sfbEnergyMS->Long[i++] = energy;
    }
    wnd += groupLen[grp];
  }

  regroupSpectrum(mdctSpectrum, tempSpectrum, sfbOffset, sfbCnt, noOfGroups, groupLen, granuleLength);

  regroupSpectrum(mdctDownmix, tempSpectrum, sfbOffset, sfbCnt, noOfGroups, groupLen, granuleLength);
}

void iisaacfenc_DegroupSpectrum(
    float *inputSpectrum,
    const int sfbCnt,
    const int noOfGroups,
    const int *groupLen,
    const int *sfbOffset,
    float *tmpSpectrum,
    unsigned int granuleLength) {
  int wnd = 0;
  int i = 0;
  int grp;
  int sfb;
  int j;
  int line;
  int nshort = granuleLength / TRANS_FAC;

  for (grp = 0; grp < noOfGroups; grp++) {
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      for (j = 0; j < groupLen[grp]; j++) {
        for (line = sfbOffset[sfb]; line < sfbOffset[sfb + 1]; line++) {
          tmpSpectrum[((wnd + j) * nshort) + line] = inputSpectrum[i];
          i++;
        }
      }
    }
    wnd += groupLen[grp];
  }
}
