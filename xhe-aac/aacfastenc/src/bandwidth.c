
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

#include <assert.h>
#include <stdlib.h>

#include "bandwidth.h"
#include "iisSigMap.h"
#include "glob_con.h"

typedef struct {
  int chanBitRateFrom;
  int chanBitRateTo;
  int bandWidthMono;
  int bandWidth2AndMoreChan;
} iisaacfenc_BANDWIDTH_TAB;

static const iisaacfenc_BANDWIDTH_TAB bandWidthTable[] = {
    {0, 11999, 3750, 4000},
    {12000, 15999, 5000, 5500},
    {16000, 19999, 6000, 7000},
    {20000, 23999, 7000, 9000},
    {24000, 27999, 8000, 11000},
    {28000, 31999, 9000, 13000},
    {32000, 35999, 10000, 13500},
    {36000, 39999, 11000, 13500},
    {40000, 43999, 12000, 14000},
    {44000, 47999, 12500, 14000},
    {48000, 59999, 13781, 15000},
    {60000, 71999, 14250, 15750},
    {72000, 79999, 15000, 16500},
    {80000, 85999, 15750, 17226},
    {86000, 95999, 16500, 17915},
    {96000, 103999, 17226, 18604},
    {104000, 111999, 17915, 19293},
    {112000, 119999, 18604, 19982},
    {120000, 127999, 19293, 21750},
    {128000, 143999, 19982, 24000},
    {144000, 159999, 24000, 24000},
    {160000, 191999, 32000, 32000},
    {192000, 576000, 48000, 48000}};

typedef struct {
  BANDWIDTH_BITRATE_MODE bitrateMode;
  int bandWidthMono;
  int bandWidth2AndMoreChan;
} iisaacfenc_BANDWIDTH_TAB_VBR;

static const iisaacfenc_BANDWIDTH_TAB_VBR bandWidthTableVBR[] = {
    {BANDWIDTH_BR_MODE_CBR, 0, 0},
    {BANDWIDTH_BR_MODE_VBR_1, 13000, 13000},
    {BANDWIDTH_BR_MODE_VBR_2, 13000, 13000},
    {BANDWIDTH_BR_MODE_VBR_3, 15750, 15750},
    {BANDWIDTH_BR_MODE_VBR_4, 16500, 16500},
    {BANDWIDTH_BR_MODE_VBR_5, 19293, 19293},
    {BANDWIDTH_BR_MODE_VBR_6, 21750, 21750},
    {BANDWIDTH_BR_MODE_VBR_0, 13000, 13000},
};

int iisaacfenc_DetermineBandWidth(
    float* bandWidth,
    int proposedBandWidth,
    int bitrate,
    BANDWIDTH_BITRATE_MODE bitrateMode,
    int sampleRate,
    CHANNEL_MAPPING_HANDLE hChMap) {
  unsigned int i = 0;
  int err = 0;
  int nChannels = hChMap->nChannels;
  int nEffectiveChannels = hChMap->nEffectiveChannels;
  int nCPE = 0;

  if ((nChannels < 1) ||
      (nChannels < nEffectiveChannels) ||
      (bandWidth == NULL)) {
    err = 1;
    return err;
  }

  for (i = 0; i < (unsigned int)hChMap->nElements; i++) {
    if (hChMap->elInfo[i].elType == ID_CPE) {
      nCPE++;
    }
  }

  switch (bitrateMode) {
    case BANDWIDTH_BR_MODE_VBR_0:
    case BANDWIDTH_BR_MODE_VBR_1:
    case BANDWIDTH_BR_MODE_VBR_2:
    case BANDWIDTH_BR_MODE_VBR_3:
    case BANDWIDTH_BR_MODE_VBR_4:
    case BANDWIDTH_BR_MODE_VBR_5:
    case BANDWIDTH_BR_MODE_VBR_6:
      if (proposedBandWidth != 0) {
        *bandWidth = (float)proposedBandWidth;
      } else {
        if (nCPE > 0) {
          *bandWidth = (float)bandWidthTableVBR[bitrateMode].bandWidth2AndMoreChan;
        } else {
          *bandWidth = (float)bandWidthTableVBR[bitrateMode].bandWidthMono;
        }
      }
      break;
    case BANDWIDTH_BR_MODE_CBR:

      if (proposedBandWidth != 0) {
        int bandWidthMax = sampleRate / 2;

        if (proposedBandWidth < bandWidthMax) {
          *bandWidth = (float)proposedBandWidth;
        } else {
          *bandWidth = (float)bandWidthMax;
        }
      } else {
        int chanBitRate = bitrate / nEffectiveChannels;
        int entryNo = 0;
        err = 1;

        if (nCPE > 0) {
          entryNo = 1;
        }

        for (i = 0; i < sizeof(bandWidthTable) / sizeof(bandWidthTable[0]); i++) {
          if (chanBitRate >= bandWidthTable[i].chanBitRateFrom &&
              chanBitRate <= bandWidthTable[i].chanBitRateTo) {
            err = 0;

            switch (entryNo) {
              case 0:
                *bandWidth = (float)bandWidthTable[i].bandWidthMono;
                break;
              case 1:
                *bandWidth = (float)bandWidthTable[i].bandWidth2AndMoreChan;
                break;
              default:
                assert(0);
            }
            break;
          }
        }
      }
      break;
    default:
      *bandWidth = 0;
      err = 1;
      break;
  }

  if (err == 0) {
    if (*bandWidth > sampleRate / 2) {
      *bandWidth = (float)sampleRate / 2;
    }
  }

  return (err);
}
