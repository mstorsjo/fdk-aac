
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

#include <float.h>

#include "interface.h"
#include "ms_stereo.h"
#include "mathlib.h"
#include "glob_con.h"

static void iisaacfenc_setBandMS(int idx, const int *sfbOffset,
                                 float *specL, float *specR,
                                 float *thrL, float *thrR,
                                 const float *thrMS,
                                 float *nrgL, float *nrgR,
                                 const float *nrgM, const float *nrgS) {
  int i;
  float tmpValue;
  const float maxNrgLR = max(nrgL[idx], nrgR[idx]);
  const float maxNrgMS = max(nrgM[idx], nrgS[idx]);

  for (i = sfbOffset[idx]; i < sfbOffset[idx + 1]; i++) {
    tmpValue = specL[i];
    specL[i] = (specL[i] + specR[i]) * 0.5f;
    specR[i] = (tmpValue - specR[i]) * 0.5f;
  }

  if ((maxNrgMS > FLT_MIN) && (maxNrgMS < maxNrgLR)) {
    thrL[idx] = thrR[idx] = thrMS[idx] * (maxNrgMS + FLT_MIN) / (maxNrgLR + FLT_MIN);
  } else {
    thrL[idx] = thrR[idx] = thrMS[idx];
  }
  nrgL[idx] = nrgM[idx];
  nrgR[idx] = nrgS[idx];
}

void iisaacfenc_MsStereoProcessing(float *sfbEnergyLeft,
                                   float *sfbEnergyRight,
                                   const float *sfbEnergyMid,
                                   const float *sfbEnergySide,
                                   float *mdctSpectrumLeft,
                                   float *mdctSpectrumRight,
                                   float *sfbThresholdLeft,
                                   float *sfbThresholdRight,
                                   const int *isBook,
                                   int *msDigest,
                                   int *msMask,
                                   const int sfbCnt,
                                   const int sfbPerGroup,
                                   const int maxSfbPerGroup,
                                   const int *sfbOffset,
                                   const int fDualMono,
                                   const int mergeMSRegions) {
  int sfb, sfboffs, isUsed = 0;
  int msMaskTrueSomewhere = 0;
  int msMaskFalseSomewhere = 0;
  int numMsMaskTrue = 0;
  int numMsMaskFalse = 0;
  float sfbThresholdMS[MAX_GROUPED_SFB];

  if (fDualMono) {
    *msDigest = MS_NONE;
    setINT(0, msMask, sfbCnt);
    return;
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if (!isBook[sfb]) {
        float pnlr, pnms;

        sfbThresholdMS[sfb] = min(sfbThresholdLeft[sfb], sfbThresholdRight[sfb]);

        pnlr = (sfbThresholdLeft[sfb] / max(sfbEnergyLeft[sfb], sfbThresholdLeft[sfb])) *
               (sfbThresholdRight[sfb] / max(sfbEnergyRight[sfb], sfbThresholdRight[sfb]));

        pnms = (sfbThresholdMS[sfb] / max(sfbEnergyMid[sfb], sfbThresholdMS[sfb])) *
               (sfbThresholdMS[sfb] / max(sfbEnergySide[sfb], sfbThresholdMS[sfb]));

        if (pnms > pnlr) {
          msMask[sfb] = MS_ON;
          numMsMaskTrue++;
          msMaskTrueSomewhere = 1;
        } else {
          msMask[sfb] = 0;
          numMsMaskFalse++;
          if (pnms < pnlr) msMaskFalseSomewhere = 1;
        }
      } else {
        isUsed = 1;
        if (msMask[sfb]) {
          msMaskTrueSomewhere = 1;
        } else {
          msMaskFalseSomewhere = 1;
        }
      }
    }
  }

  if (msMaskTrueSomewhere) {
    if ((msMaskFalseSomewhere) || (isUsed)) {
      *msDigest = MS_SOME;

      if ((mergeMSRegions) && (!isUsed)) {
        if (numMsMaskFalse < 9) {
          *msDigest = MS_ALL;
          setINT(MS_ON, msMask, sfbCnt);
        } else if (numMsMaskTrue < 9) {
          *msDigest = MS_NONE;
          setINT(0, msMask, sfbCnt);
        }
      }
    } else {
      *msDigest = MS_ALL;
      setINT(MS_ON, msMask, sfbCnt);
    }
  } else {
    *msDigest = MS_NONE;
  }

  for (sfboffs = 0; sfboffs < sfbCnt; sfboffs += sfbPerGroup) {
    for (sfb = sfboffs; sfb < sfboffs + maxSfbPerGroup; sfb++) {
      if ((msMask[sfb] == MS_ON) && (!isBook[sfb])) {
        iisaacfenc_setBandMS(sfb, sfbOffset,
                             mdctSpectrumLeft, mdctSpectrumRight,
                             sfbThresholdLeft, sfbThresholdRight,
                             sfbThresholdMS,
                             sfbEnergyLeft, sfbEnergyRight,
                             sfbEnergyMid, sfbEnergySide);
      }
    }
  }
}

void iisaacfenc_Dematrix(int noOfGroups,
                         int sfbCnt,
                         int *groupLen,
                         int *sfbOffset,
                         int *msMask,
                         int msDigest,
                         float *specLeft,
                         float *specRight) {
  int wnd = 0;
  int line;
  int sfb;
  int grp;
  int j;

  float tmp;

  if (msDigest != MS_NONE) {
    for (grp = 0; grp < noOfGroups; grp++) {
      for (sfb = 0; sfb < sfbCnt; sfb++) {
        if (msMask[sfb + sfbCnt * grp] == 1) {
          for (j = 0; j < groupLen[grp]; j++) {
            for (line = sfbOffset[sfb]; line < sfbOffset[sfb + 1]; line++) {
              tmp = specLeft[((wnd + j) * 128) + line] - specRight[((wnd + j) * 128) + line];
              specLeft[((wnd + j) * 128) + line] += specRight[((wnd + j) * 128) + line];
              specRight[((wnd + j) * 128) + line] = tmp;
            }
          }
        }
      }
      wnd += groupLen[grp];
    }
  }
}
