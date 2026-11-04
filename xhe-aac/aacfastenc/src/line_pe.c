
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

#include "bit_cnt.h"
#include "line_pe.h"

static const float LOG2_1 = 1.442695041f;

static const float C1 = 3.0f;
static const float C2 = 1.3219281f;
static const float C3 = 0.5593573f;

void iisaacfenc_prepareSfbPe1(float *sfbRelevLines,
                              const float *sfbEnergy,
                              const float *sfbThreshold,
                              const float *sfbFormFactor,
                              const int *sfbOffset,
                              const int sfbCnt) {
  int sfb;

  for (sfb = 0; sfb < sfbCnt; sfb++) {
    if (sfbEnergy[sfb] > sfbThreshold[sfb]) {
      const float sfbWidth = (float)(sfbOffset[sfb + 1] - sfbOffset[sfb]);
      sfbRelevLines[sfb] = sfbFormFactor[sfb] * (float)sqrt(sqrt(sfbWidth / sfbEnergy[sfb]));
    } else {
      sfbRelevLines[sfb] = 0.0f;
    }
  }
}

void iisaacfenc_prepareSfbPe2(PE_CHANNEL_DATA *peChanData,
                              const float *sfbEnergy,
                              const float *sfbRelevLines,
                              const int sfbCnt) {
  int sfb;

  for (sfb = 0; sfb < sfbCnt; sfb++) {
    if (sfbRelevLines[sfb] > 0.0f)
      peChanData->sfbLdEnergy[sfb] = (float)log(sfbEnergy[sfb]) * LOG2_1;
    else
      peChanData->sfbLdEnergy[sfb] = 0.0f;
  }
}

void iisaacfenc_calcSfbPe(PE_CHANNEL_DATA *peChanData,
                          const float *sfbEnergy,
                          const float *sfbThreshold,
                          const float *sfbRelevLines,
                          const int sfbCnt,
                          const int *isBook,
                          const int *isScale) {
  int sfb;
  float nLines, ldThr, ldRatio;

  int lastValIs = 0;
  int delta;

  peChanData->pe = 0.0f;
  peChanData->constPart = 0.0f;
  peChanData->nActiveLines = 0.0f;
  for (sfb = 0; sfb < sfbCnt; sfb++) {
    if (sfbEnergy[sfb] > sfbThreshold[sfb]) {
      ldThr = (float)log(sfbThreshold[sfb]) * LOG2_1;
      ldRatio = peChanData->sfbLdEnergy[sfb] - ldThr;
      nLines = sfbRelevLines[sfb];
      if (ldRatio >= C1) {
        peChanData->sfbPe[sfb] = nLines * ldRatio;
        peChanData->sfbConstPart[sfb] = nLines * peChanData->sfbLdEnergy[sfb];
      } else {
        peChanData->sfbPe[sfb] = nLines * (C2 + C3 * ldRatio);
        peChanData->sfbConstPart[sfb] = nLines *
                                        (C2 + C3 * peChanData->sfbLdEnergy[sfb]);
        nLines *= C3;
      }
      peChanData->sfbNActiveLines[sfb] = nLines;
    }

    else if (isBook[sfb]) {
      delta = isScale[sfb] - lastValIs;
      lastValIs = isScale[sfb];
      peChanData->sfbPe[sfb] = (float)iisaacfenc_bitCountScalefactorDelta(delta);
      peChanData->sfbConstPart[sfb] = 0.0f;
      peChanData->sfbNActiveLines[sfb] = 0.0f;
    } else {
      peChanData->sfbPe[sfb] = 0.0f;
      peChanData->sfbConstPart[sfb] = 0.0f;
      peChanData->sfbNActiveLines[sfb] = 0.0f;
    }
    peChanData->pe += peChanData->sfbPe[sfb];
    peChanData->constPart += peChanData->sfbConstPart[sfb];
    peChanData->nActiveLines += peChanData->sfbNActiveLines[sfb];
  }
}
