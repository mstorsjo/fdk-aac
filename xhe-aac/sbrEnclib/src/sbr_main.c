
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

#include "iisutillib.h"
#include "sbr_main.h"
#include "sbr.h"
#include "sbr_def.h"
#include "freq_sca.h"
#include "cmondata.h"
#include "mathlib.h"
#include "ivaltime.h"
#include "iisutillib.h"
#include "cpuinfo.h"
#include "env_est.h"

#ifndef HBE_MAX_BITRATE_MONO
#define HBE_MAX_BITRATE_MONO 17000
#endif

#ifndef HBE_MAX_BITRATE_MULTI
#define HBE_MAX_BITRATE_MULTI HBE_MAX_BITRATE_MONO
#endif

#if defined __GNUC__ || defined __clang__
#define SBR_WITH_AAC __attribute__((unused))
#else
#define SBR_WITH_AAC
#endif

#define INVALID_TABLE_IDX -1

typedef struct
{
  CODEC_TYPE coreCoder;
  unsigned int bitrateFrom;
  unsigned int bitrateTo;

  unsigned int sampleRate;
  SBR_CHANNELMODE sbrChannelmode;

  unsigned int startFreq;
  unsigned int startFreqSpeech;
  unsigned int stopFreq;
  unsigned int stopFreqSpeech;

  int numNoiseBands;
  int noiseFloorOffset;
  int noiseMaxLevel;
  SBR_STEREO_MODE stereoMode;
  int freqScale;

} TUNINGTABLE;

TUNINGTABLE tuningTableGeneral[] = {

    {CODEC_SAAC, 6000, 28000, 2400, SBR_CHANNELMODE_MONO, 9, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 9000, 14001, 3000, SBR_CHANNELMODE_MONO, 8, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 9000, 28001, 4000, SBR_CHANNELMODE_MONO, 6, 6, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 6000, 44001, 4800, SBR_CHANNELMODE_MONO, 6, 6, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 44001, 120000, 4800, SBR_CHANNELMODE_MONO, 8, 6, 8, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 4000, 6000, 6000, SBR_CHANNELMODE_MONO, 6, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 6000, 48001, 6000, SBR_CHANNELMODE_MONO, 6, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 4000, 8000, 8000, SBR_CHANNELMODE_MONO, 12, 6, 10, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 8000, 10000, 8000, SBR_CHANNELMODE_MONO, 7, 6, 11, 10, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 8000, SBR_CHANNELMODE_MONO, 11, 7, 13, 12, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 8000, SBR_CHANNELMODE_MONO, 14, 10, 13, 13, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16002, 160001, 8000, SBR_CHANNELMODE_MONO, 5, 10, 6, 13, 1, 0, 6, SBR_MONO, 1},

    {CODEC_SAAC, 6000, 8000, 8320, SBR_CHANNELMODE_MONO, 16, 16, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 6000, 8000, 8960, SBR_CHANNELMODE_MONO, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 9000, 9600, SBR_CHANNELMODE_MONO, 12, 4, 8, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 9000, 12001, 9600, SBR_CHANNELMODE_MONO, 12, 4, 9, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11000, 30000, 9600, SBR_CHANNELMODE_MONO, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 30000, 48000, 9600, SBR_CHANNELMODE_MONO, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 48000, 68001, 9600, SBR_CHANNELMODE_MONO, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 10000, 11025, SBR_CHANNELMODE_MONO, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 11025, SBR_CHANNELMODE_MONO, 8, 5, 12, 9, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 11025, SBR_CHANNELMODE_MONO, 12, 8, 13, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 8000, 10000, 11200, SBR_CHANNELMODE_MONO, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 11200, SBR_CHANNELMODE_MONO, 8, 5, 12, 9, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 11200, SBR_CHANNELMODE_MONO, 12, 8, 13, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 10000, 12000, SBR_CHANNELMODE_MONO, 4, 3, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 12000, SBR_CHANNELMODE_MONO, 7, 4, 11, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 12000, SBR_CHANNELMODE_MONO, 11, 7, 12, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16001, 48000, 12000, SBR_CHANNELMODE_MONO, 11, 7, 12, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 48000, 60000, 12000, SBR_CHANNELMODE_MONO, 11, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 60000, 76000, 12000, SBR_CHANNELMODE_MONO, 11, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},

    {CODEC_SAAC, 11000, 36000, 14700, SBR_CHANNELMODE_MONO, 11, 12, 12, 13, 2, 0, 3, SBR_MONO, 2},

    {CODEC_SAAC, 6000, 10000, 16000, SBR_CHANNELMODE_MONO, 1, 1, 0, 0, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 16000, SBR_CHANNELMODE_MONO, 2, 1, 6, 0, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 14001, 16000, SBR_CHANNELMODE_MONO, 7, 2, 9, 0, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 14001, 16000, 16000, SBR_CHANNELMODE_MONO, 7, 2, 12, 0, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 16000, 18000, 16000, SBR_CHANNELMODE_MONO, 8, 2, 12, 3, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 16000, SBR_CHANNELMODE_MONO, 6, 5, 13, 7, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 16000, SBR_CHANNELMODE_MONO, 10, 9, 12, 8, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 16000, SBR_CHANNELMODE_MONO, 12, 12, 13, 13, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 16000, SBR_CHANNELMODE_MONO, 14, 14, 13, 13, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64000, 16000, SBR_CHANNELMODE_MONO, 15, 15, 13, 13, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 64000, 96001, 16000, SBR_CHANNELMODE_MONO, 15, 15, 13, 13, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 11000, 28000, 17640, SBR_CHANNELMODE_MONO, 10, 9, 12, 8, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 17640, SBR_CHANNELMODE_MONO, 12, 12, 13, 13, 2, 0, 3, SBR_MONO, 2},

    {CODEC_SAAC, 6000, 9000, 19200, SBR_CHANNELMODE_MONO, 8, 8, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 9000, 16000, 19200, SBR_CHANNELMODE_MONO, 8, 8, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 16000, 18000, 19200, SBR_CHANNELMODE_MONO, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 18000, 22000, 19200, SBR_CHANNELMODE_MONO, 4, 4, 8, 5, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 24000, 19200, SBR_CHANNELMODE_MONO, 5, 8, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 24000, 28000, 19200, SBR_CHANNELMODE_MONO, 6, 8, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 19200, SBR_CHANNELMODE_MONO, 11, 11, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 19200, SBR_CHANNELMODE_MONO, 11, 11, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 19200, SBR_CHANNELMODE_MONO, 13, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 64000, 96001, 19200, SBR_CHANNELMODE_MONO, 11, 11, 11, 11, 1, 0, 6, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 11369, 20000, SBR_CHANNELMODE_MONO, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11369, 16000, 20000, SBR_CHANNELMODE_MONO, 3, 1, 4, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 18000, 20000, SBR_CHANNELMODE_MONO, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 20000, SBR_CHANNELMODE_MONO, 4, 4, 8, 5, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 20000, SBR_CHANNELMODE_MONO, 7, 6, 8, 6, 2, 0, 6, SBR_MONO, 2},

    {CODEC_SAAC, 28000, 36000, 20000, SBR_CHANNELMODE_MONO, 8, 8, 12, 12, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 20000, SBR_CHANNELMODE_MONO, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 20000, SBR_CHANNELMODE_MONO, 13, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 11369, 22050, SBR_CHANNELMODE_MONO, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11369, 16000, 22050, SBR_CHANNELMODE_MONO, 3, 1, 4, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 18000, 22050, SBR_CHANNELMODE_MONO, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 22050, SBR_CHANNELMODE_MONO, 7, 3, 8, 5, 2, 0, 6, SBR_MONO, 2},

    {CODEC_SAAC, 22000, 24000, 22050, SBR_CHANNELMODE_MONO, 6, 6, 9, 6, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 24000, 28000, 22050, SBR_CHANNELMODE_MONO, 7, 6, 9, 6, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 22050, SBR_CHANNELMODE_MONO, 10, 10, 9, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 22050, SBR_CHANNELMODE_MONO, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 22050, SBR_CHANNELMODE_MONO, 13, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 12000, 24000, SBR_CHANNELMODE_MONO, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16000, 24000, SBR_CHANNELMODE_MONO, 3, 1, 4, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 18000, 24000, SBR_CHANNELMODE_MONO, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 24000, SBR_CHANNELMODE_MONO, 7, 3, 8, 5, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 24000, SBR_CHANNELMODE_MONO, 7, 6, 8, 6, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 24000, SBR_CHANNELMODE_MONO, 10, 10, 9, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 24000, SBR_CHANNELMODE_MONO, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64000, 24000, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 64000, 96001, 24000, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 96000, 128001, 24000, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 32000, SBR_CHANNELMODE_MONO, 4, 4, 4, 4, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 32000, SBR_CHANNELMODE_MONO, 7, 7, 6, 6, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 32000, SBR_CHANNELMODE_MONO, 9, 9, 8, 8, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 32000, SBR_CHANNELMODE_MONO, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 32000, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 44100, SBR_CHANNELMODE_MONO, 4, 4, 4, 4, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 44100, SBR_CHANNELMODE_MONO, 7, 7, 6, 6, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 44100, SBR_CHANNELMODE_MONO, 9, 9, 8, 8, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 44100, SBR_CHANNELMODE_MONO, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 44100, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 48000, SBR_CHANNELMODE_MONO, 4, 4, 9, 9, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 48000, SBR_CHANNELMODE_MONO, 7, 7, 10, 10, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 48000, SBR_CHANNELMODE_MONO, 9, 9, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 48000, SBR_CHANNELMODE_MONO, 11, 11, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 48000, SBR_CHANNELMODE_MONO, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 12000, 28001, 4000, SBR_CHANNELMODE_MPS212, 6, 6, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11000, 88001, 4800, SBR_CHANNELMODE_MPS212, 8, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},

    {CODEC_SAAC, 4000, 6000, 6000, SBR_CHANNELMODE_MPS212, 16, 16, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 6000, 24000, 6000, SBR_CHANNELMODE_MPS212, 6, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 24000, 48000, 6000, SBR_CHANNELMODE_MPS212, 6, 6, 8, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 8000, 7350, SBR_CHANNELMODE_MPS212, 16, 16, 8, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 4000, 8000, 8000, SBR_CHANNELMODE_MPS212, 16, 16, 8, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 8000, 10000, 8000, SBR_CHANNELMODE_MPS212, 7, 6, 11, 10, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 8000, SBR_CHANNELMODE_MPS212, 11, 7, 13, 12, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16000, 8000, SBR_CHANNELMODE_MPS212, 12, 10, 11, 13, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 160001, 8000, SBR_CHANNELMODE_MPS212, 5, 10, 6, 13, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 8000, 8320, SBR_CHANNELMODE_MPS212, 16, 16, 8, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 8000, 8960, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 8000, 9600, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 8000, 12000, 9600, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 40001, 9600, SBR_CHANNELMODE_MPS212, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 40000, 48000, 9600, SBR_CHANNELMODE_MPS212, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 48000, 114001, 9600, SBR_CHANNELMODE_MPS212, 11, 4, 11, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 10000, 11025, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 11025, SBR_CHANNELMODE_MPS212, 8, 5, 12, 9, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 11025, SBR_CHANNELMODE_MPS212, 12, 8, 13, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 8000, 10000, 11200, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 11200, SBR_CHANNELMODE_MPS212, 8, 5, 12, 9, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16001, 11200, SBR_CHANNELMODE_MPS212, 12, 8, 13, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 10000, 12000, SBR_CHANNELMODE_MPS212, 4, 3, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 12000, SBR_CHANNELMODE_MPS212, 7, 4, 11, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16000, 12000, SBR_CHANNELMODE_MPS212, 11, 7, 12, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 40000, 12000, SBR_CHANNELMODE_MPS212, 11, 7, 12, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 40000, 60000, 12000, SBR_CHANNELMODE_MPS212, 11, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 60000, 76000, 12000, SBR_CHANNELMODE_MPS212, 11, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},

    {CODEC_SAAC, 11000, 15000, 14700, SBR_CHANNELMODE_MPS212, 5, 5, 12, 12, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 15000, 36000, 14700, SBR_CHANNELMODE_MPS212, 5, 6, 12, 12, 2, 0, 3, SBR_MONO, 2},

    {CODEC_SAAC, 6000, 10000, 16000, SBR_CHANNELMODE_MPS212, 1, 1, 0, 0, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 10000, 12000, 16000, SBR_CHANNELMODE_MPS212, 2, 1, 6, 0, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 16000, 16000, SBR_CHANNELMODE_MPS212, 4, 2, 10, 0, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 16000, 18000, 16000, SBR_CHANNELMODE_MPS212, 5, 2, 11, 3, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 16000, SBR_CHANNELMODE_MPS212, 6, 5, 12, 7, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 16000, SBR_CHANNELMODE_MPS212, 10, 9, 12, 8, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 16000, SBR_CHANNELMODE_MPS212, 12, 12, 13, 13, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 16000, SBR_CHANNELMODE_MPS212, 14, 14, 13, 13, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 16000, SBR_CHANNELMODE_MPS212, 15, 15, 13, 13, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 11000, 28000, 17640, SBR_CHANNELMODE_MPS212, 10, 9, 12, 8, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 17640, SBR_CHANNELMODE_MPS212, 11, 12, 12, 13, 1, 0, 6, SBR_MONO, 2},

    {CODEC_SAAC, 12000, 16000, 19200, SBR_CHANNELMODE_MPS212, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 16000, 18000, 19200, SBR_CHANNELMODE_MPS212, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 18000, 22000, 19200, SBR_CHANNELMODE_MPS212, 4, 4, 8, 5, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 19200, SBR_CHANNELMODE_MPS212, 8, 8, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 28000, 36000, 19200, SBR_CHANNELMODE_MPS212, 11, 11, 11, 11, 1, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 19200, SBR_CHANNELMODE_MPS212, 11, 11, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 19200, SBR_CHANNELMODE_MPS212, 13, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 11369, 20000, SBR_CHANNELMODE_MPS212, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11369, 16000, 20000, SBR_CHANNELMODE_MPS212, 3, 1, 4, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 18000, 20000, SBR_CHANNELMODE_MPS212, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 22000, 20000, SBR_CHANNELMODE_MPS212, 4, 4, 8, 5, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 22000, 28000, 20000, SBR_CHANNELMODE_MPS212, 7, 6, 8, 6, 2, 0, 6, SBR_MONO, 2},

    {CODEC_SAAC, 28000, 36000, 20000, SBR_CHANNELMODE_MPS212, 8, 8, 12, 12, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 20000, SBR_CHANNELMODE_MPS212, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64001, 20000, SBR_CHANNELMODE_MPS212, 13, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 11369, 22050, SBR_CHANNELMODE_MPS212, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 11369, 16000, 22050, SBR_CHANNELMODE_MPS212, 3, 1, 4, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 16000, 18000, 22050, SBR_CHANNELMODE_MPS212, 3, 1, 5, 4, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 23000, 22050, SBR_CHANNELMODE_MPS212, 5, 3, 9, 5, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 23000, 26000, 22050, SBR_CHANNELMODE_MPS212, 10, 6, 10, 6, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 26000, 30000, 22050, SBR_CHANNELMODE_MPS212, 10, 10, 10, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 30000, 36000, 22050, SBR_CHANNELMODE_MPS212, 10, 10, 10, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 22050, SBR_CHANNELMODE_MPS212, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 60000, 22050, SBR_CHANNELMODE_MPS212, 12, 13, 10, 12, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 60000, 64001, 22050, SBR_CHANNELMODE_MPS212, 15, 13, 12, 12, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 8000, 12000, 24000, SBR_CHANNELMODE_MPS212, 1, 1, 1, 1, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 18000, 23000, 24000, SBR_CHANNELMODE_MPS212, 5, 3, 9, 5, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 23000, 26000, 24000, SBR_CHANNELMODE_MPS212, 10, 6, 10, 6, 2, 0, 6, SBR_MONO, 2},
    {CODEC_SAAC, 26000, 30000, 24000, SBR_CHANNELMODE_MPS212, 10, 10, 10, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 30000, 36000, 24000, SBR_CHANNELMODE_MPS212, 10, 10, 10, 9, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 36000, 44000, 24000, SBR_CHANNELMODE_MPS212, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 44000, 64000, 24000, SBR_CHANNELMODE_MPS212, 12, 13, 9, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 64000, 96001, 24000, SBR_CHANNELMODE_MPS212, 14, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 96000, 128001, 24000, SBR_CHANNELMODE_MPS212, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 32000, SBR_CHANNELMODE_MPS212, 4, 4, 4, 4, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 32000, SBR_CHANNELMODE_MPS212, 7, 7, 6, 6, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 32000, SBR_CHANNELMODE_MPS212, 9, 9, 8, 8, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 32000, SBR_CHANNELMODE_MPS212, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 32000, SBR_CHANNELMODE_MPS212, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 44100, SBR_CHANNELMODE_MPS212, 4, 4, 4, 4, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 44100, SBR_CHANNELMODE_MPS212, 7, 7, 6, 6, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 44100, SBR_CHANNELMODE_MPS212, 9, 9, 8, 8, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 44100, SBR_CHANNELMODE_MPS212, 11, 11, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 44100, SBR_CHANNELMODE_MPS212, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 24000, 36000, 48000, SBR_CHANNELMODE_MPS212, 4, 4, 9, 9, 2, 0, 3, SBR_MONO, 3},
    {CODEC_SAAC, 36000, 60000, 48000, SBR_CHANNELMODE_MPS212, 7, 7, 10, 10, 2, 0, 3, SBR_MONO, 2},
    {CODEC_SAAC, 60000, 72000, 48000, SBR_CHANNELMODE_MPS212, 9, 9, 10, 10, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 72000, 100000, 48000, SBR_CHANNELMODE_MPS212, 11, 11, 11, 11, 2, 0, 3, SBR_MONO, 1},
    {CODEC_SAAC, 100000, 160001, 48000, SBR_CHANNELMODE_MPS212, 13, 13, 11, 11, 2, 0, 3, SBR_MONO, 1},

    {CODEC_SAAC, 11000, 88001, 4800, SBR_CHANNELMODE_STEREO, 8, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 36000, 52000, 6000, SBR_CHANNELMODE_STEREO, 8, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 52000, 76000, 6000, SBR_CHANNELMODE_STEREO, 8, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 2},

    {CODEC_SAAC, 16000, 160001, 8000, SBR_CHANNELMODE_STEREO, 5, 10, 6, 13, 1, 0, 6, SBR_SWITCH_LRC, 3},

    {CODEC_SAAC, 40000, 52000, 9600, SBR_CHANNELMODE_STEREO, 11, 4, 11, 6, 1, 0, 6, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 52000, 114001, 9600, SBR_CHANNELMODE_STEREO, 11, 4, 11, 6, 1, 0, 6, SBR_SWITCH_LRC, 3},

    {CODEC_SAAC, 40000, 60000, 12000, SBR_CHANNELMODE_STEREO, 11, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 60000, 76000, 12000, SBR_CHANNELMODE_STEREO, 11, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 76000, 150000, 12000, SBR_CHANNELMODE_STEREO, 11, 2, 14, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},

    {CODEC_SAAC, 16000, 24000, 16000, SBR_CHANNELMODE_STEREO, 4, 2, 1, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 24000, 28000, 16000, SBR_CHANNELMODE_STEREO, 8, 7, 10, 8, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 28000, 36000, 16000, SBR_CHANNELMODE_STEREO, 10, 9, 12, 11, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 36000, 44000, 16000, SBR_CHANNELMODE_STEREO, 13, 13, 13, 13, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 44000, 52000, 16000, SBR_CHANNELMODE_STEREO, 15, 15, 13, 13, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 52000, 60000, 16000, SBR_CHANNELMODE_STEREO, 15, 15, 13, 13, 3, 0, -3, SBR_SWITCH_LRC, 1},
    {CODEC_SAAC, 60000, 76000, 16000, SBR_CHANNELMODE_STEREO, 15, 15, 13, 13, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 76000, 128000, 16000, SBR_CHANNELMODE_STEREO, 15, 15, 13, 13, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 128000, 164001, 16000, SBR_CHANNELMODE_STEREO, 15, 15, 13, 13, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 36000, 44000, 19200, SBR_CHANNELMODE_STEREO, 11, 11, 11, 11, 2, 0, 3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 44000, 124001, 19200, SBR_CHANNELMODE_STEREO, 13, 13, 12, 12, 2, 0, 3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 16000, 24000, 20000, SBR_CHANNELMODE_STEREO, 2, 1, 1, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 24000, 28000, 20000, SBR_CHANNELMODE_STEREO, 5, 4, 6, 5, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 28000, 32000, 20000, SBR_CHANNELMODE_STEREO, 5, 4, 8, 7, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 32000, 36000, 20000, SBR_CHANNELMODE_STEREO, 7, 6, 8, 7, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 36000, 44000, 20000, SBR_CHANNELMODE_STEREO, 10, 10, 9, 9, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 44000, 52000, 20000, SBR_CHANNELMODE_STEREO, 12, 12, 9, 9, 3, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 52000, 60000, 20000, SBR_CHANNELMODE_STEREO, 13, 13, 10, 10, 3, 0, -3, SBR_SWITCH_LRC, 1},
    {CODEC_SAAC, 60000, 76000, 20000, SBR_CHANNELMODE_STEREO, 14, 14, 12, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 76000, 128001, 20000, SBR_CHANNELMODE_STEREO, 14, 14, 12, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 16000, 24000, 22050, SBR_CHANNELMODE_STEREO, 2, 1, 1, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 24000, 28000, 22050, SBR_CHANNELMODE_STEREO, 5, 4, 6, 5, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 28000, 32000, 22050, SBR_CHANNELMODE_STEREO, 5, 4, 8, 7, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 32000, 36000, 22050, SBR_CHANNELMODE_STEREO, 7, 6, 8, 7, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 36000, 44000, 22050, SBR_CHANNELMODE_STEREO, 10, 10, 9, 9, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 44000, 52000, 22050, SBR_CHANNELMODE_STEREO, 12, 12, 10, 9, 3, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 52000, 60000, 22050, SBR_CHANNELMODE_STEREO, 13, 13, 10, 10, 3, 0, -3, SBR_SWITCH_LRC, 1},
    {CODEC_SAAC, 60000, 76000, 22050, SBR_CHANNELMODE_STEREO, 15, 14, 12, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 76000, 128001, 22050, SBR_CHANNELMODE_STEREO, 15, 15, 13, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 16000, 24000, 24000, SBR_CHANNELMODE_STEREO, 2, 1, 1, 0, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 24000, 28000, 24000, SBR_CHANNELMODE_STEREO, 5, 5, 6, 6, 1, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 28000, 36000, 24000, SBR_CHANNELMODE_STEREO, 7, 6, 8, 7, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 36000, 44000, 24000, SBR_CHANNELMODE_STEREO, 10, 10, 9, 9, 2, 0, -3, SBR_SWITCH_LRC, 2},

    {CODEC_SAAC, 44000, 52000, 24000, SBR_CHANNELMODE_STEREO, 12, 13, 9, 11, 2, 0, 3, SBR_SWITCH_LRC, 1},
    {CODEC_SAAC, 52000, 60000, 24000, SBR_CHANNELMODE_STEREO, 13, 13, 10, 10, 3, 0, -3, SBR_SWITCH_LRC, 1},
    {CODEC_SAAC, 60000, 76000, 24000, SBR_CHANNELMODE_STEREO, 14, 14, 11, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 76000, 128000, 24000, SBR_CHANNELMODE_STEREO, 15, 15, 12, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 128000, 164001, 24000, SBR_CHANNELMODE_STEREO, 15, 15, 12, 12, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 32000, 60000, 32000, SBR_CHANNELMODE_STEREO, 4, 4, 4, 4, 2, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 60000, 80000, 32000, SBR_CHANNELMODE_STEREO, 7, 7, 6, 6, 3, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 80000, 112000, 32000, SBR_CHANNELMODE_STEREO, 9, 9, 8, 8, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 112000, 144000, 32000, SBR_CHANNELMODE_STEREO, 11, 11, 10, 10, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 144000, 256001, 32000, SBR_CHANNELMODE_STEREO, 13, 13, 11, 11, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 32000, 60000, 44100, SBR_CHANNELMODE_STEREO, 4, 4, 4, 4, 2, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 60000, 80000, 44100, SBR_CHANNELMODE_STEREO, 7, 7, 6, 6, 3, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 80000, 112000, 44100, SBR_CHANNELMODE_STEREO, 9, 9, 8, 8, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 112000, 144000, 44100, SBR_CHANNELMODE_STEREO, 11, 11, 10, 10, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 144000, 256001, 44100, SBR_CHANNELMODE_STEREO, 13, 13, 11, 11, 3, 0, -3, SBR_LEFT_RIGHT, 1},

    {CODEC_SAAC, 32000, 60000, 48000, SBR_CHANNELMODE_STEREO, 4, 4, 9, 9, 2, 0, -3, SBR_SWITCH_LRC, 3},
    {CODEC_SAAC, 60000, 80000, 48000, SBR_CHANNELMODE_STEREO, 7, 7, 10, 10, 2, 0, -3, SBR_SWITCH_LRC, 2},
    {CODEC_SAAC, 80000, 112000, 48000, SBR_CHANNELMODE_STEREO, 9, 9, 10, 10, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 112000, 144000, 48000, SBR_CHANNELMODE_STEREO, 11, 11, 11, 11, 3, 0, -3, SBR_LEFT_RIGHT, 1},
    {CODEC_SAAC, 144000, 256001, 48000, SBR_CHANNELMODE_STEREO, 13, 13, 11, 11, 3, 0, -3, SBR_LEFT_RIGHT, 1},

};

TUNINGTABLE tuningTableSaacQuadRate[] = {
    {CODEC_SAAC, 6000, 44001, 4800, SBR_CHANNELMODE_MONO, 6, 6, 6, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 6000, 48001, 6000, SBR_CHANNELMODE_MONO, 12, 6, 11, 8, 2, 0, -3, SBR_MONO, 2},

    {CODEC_SAAC, 12000, 32000, 7350, SBR_CHANNELMODE_MONO, 12, 10, 11, 13, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 4000, 8000, 8000, SBR_CHANNELMODE_MONO, 12, 6, 10, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 8000, 48001, 8000, SBR_CHANNELMODE_MONO, 11, 10, 11, 13, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 16001, 48001, 8820, SBR_CHANNELMODE_MONO, 12, 10, 12, 12, 2, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 9000, 9600, SBR_CHANNELMODE_MONO, 12, 4, 8, 6, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 9000, 56001, 9600, SBR_CHANNELMODE_MONO, 12, 4, 9, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 7000, 11025, SBR_CHANNELMODE_MONO, 12, 2, 6, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 7000, 9000, 11025, SBR_CHANNELMODE_MONO, 12, 2, 7, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 9000, 11000, 11025, SBR_CHANNELMODE_MONO, 5, 2, 7, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 11000, 15000, 11025, SBR_CHANNELMODE_MONO, 5, 2, 9, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 15000, 19000, 11025, SBR_CHANNELMODE_MONO, 6, 2, 8, 0, 1, 0, -3, SBR_MONO, 3},

    {CODEC_SAAC, 6000, 7000, 12000, SBR_CHANNELMODE_MONO, 11, 2, 6, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 7000, 9000, 12000, SBR_CHANNELMODE_MONO, 11, 2, 7, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 9000, 11000, 12000, SBR_CHANNELMODE_MONO, 5, 2, 7, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 11000, 15000, 12000, SBR_CHANNELMODE_MONO, 5, 2, 9, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 15000, 19000, 12000, SBR_CHANNELMODE_MONO, 6, 2, 8, 0, 1, 0, -3, SBR_MONO, 3},
    {CODEC_SAAC, 19000, 76000, 12000, SBR_CHANNELMODE_MONO, 11, 2, 14, 0, 1, 0, -3, SBR_MONO, 3},

    {CODEC_SAAC, 12000, 32000, 7350, SBR_CHANNELMODE_MPS212, 12, 10, 11, 13, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 12000, 48001, 8000, SBR_CHANNELMODE_MPS212, 12, 10, 11, 13, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 16001, 48001, 8820, SBR_CHANNELMODE_MPS212, 12, 10, 12, 12, 2, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 8000, 56001, 9600, SBR_CHANNELMODE_MPS212, 5, 4, 6, 6, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 11000, 14000, 11025, SBR_CHANNELMODE_MPS212, 7, 7, 9, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 14000, 18000, 11025, SBR_CHANNELMODE_MPS212, 11, 7, 11, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 18001, 32000, 11025, SBR_CHANNELMODE_MPS212, 11, 7, 11, 8, 1, 0, 6, SBR_MONO, 3},

    {CODEC_SAAC, 11000, 14000, 12000, SBR_CHANNELMODE_MPS212, 7, 7, 9, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 14000, 18000, 12000, SBR_CHANNELMODE_MPS212, 11, 7, 11, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 18000, 21000, 12000, SBR_CHANNELMODE_MPS212, 11, 7, 11, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 21000, 72001, 12000, SBR_CHANNELMODE_MPS212, 11, 7, 12, 8, 1, 0, 6, SBR_MONO, 3},
    {CODEC_SAAC, 40000, 48001, 8000, SBR_CHANNELMODE_STEREO, 11, 10, 15, 13, 2, 0, -3, SBR_LEFT_RIGHT, 3},
    {CODEC_SAAC, 40000, 56001, 9600, SBR_CHANNELMODE_STEREO, 11, 4, 15, 6, 2, 0, -3, SBR_LEFT_RIGHT, 3},
    {CODEC_SAAC, 40000, 72001, 12000, SBR_CHANNELMODE_STEREO, 11, 7, 15, 8, 2, 0, -3, SBR_LEFT_RIGHT, 3},
};

static int getNearestTunedSamplingFreq(unsigned int samplingRate, TUNINGTABLE *tuningTable, int paramSets, CODEC_TYPE coreCoder, SBR_CHANNELMODE sbrChannelmode) {
  unsigned int sampleRatePrev = 0;
  unsigned int sampleRateCurr = 0;
  int k = 0;

  while (k < paramSets) {
    sampleRatePrev = sampleRateCurr;
    while (k < paramSets && (tuningTable[k].sampleRate == sampleRateCurr || coreCoder != tuningTable[k].coreCoder || sbrChannelmode != tuningTable[k].sbrChannelmode)) {
      ++k;
    }
    if (k >= paramSets) {
      WARN("All SBR settings are lower than this sampling rate! Using biggest entry.");
      return sampleRatePrev;
    }
    if (tuningTable[k].sampleRate == samplingRate) {
      return samplingRate;
    }
    sampleRateCurr = tuningTable[k].sampleRate;
    assert(sampleRatePrev < sampleRateCurr);

    if (sampleRateCurr > samplingRate) {
      WARN("No SBR settings for this sampling rate! Using nearest entry.");
      if ((unsigned int)sqrt((double)sampleRatePrev * sampleRateCurr) + 0.5f > samplingRate) {
        return sampleRatePrev;
      } else {
        return sampleRateCurr;
      }
    }
  }

  assert(0);
  return 0;
}

static int
getSbrTuningTableIndex(CODEC_TYPE coreCoder,
                       unsigned int bitrate,
                       SBR_CHANNELMODE sbrChannelmode,
                       unsigned int sampleRate,
                       TUNINGTABLE *tuningTable,
                       int paramSets) {
  int i;
  unsigned int sampleRateM = 0;

  switch (coreCoder) {
    case CODEC_SAAC:
      sampleRateM = getNearestTunedSamplingFreq(sampleRate, tuningTable, paramSets, coreCoder, sbrChannelmode);
      break;
    default:
      assert(0);
  }

  for (i = 0; i < paramSets; i++) {
    if (coreCoder == tuningTable[i].coreCoder)
      if (sbrChannelmode == tuningTable[i].sbrChannelmode) {
        if ((sampleRateM == tuningTable[i].sampleRate) &&
            (bitrate >= tuningTable[i].bitrateFrom) &&
            (bitrate < tuningTable[i].bitrateTo)) {
          return i;
        }
      }
  }

  return INVALID_TABLE_IDX;
}

static int
getDownsampledStopFreq(CODEC_TYPE coreCoder,
                       int sampleRate,
                       int startFreq,
                       int stopFreq,
                       SR_MODE srMode) {
  int maxStopFreqRaw = sampleRate / 4;
  int startBand, stopBand;
  HANDLE_ERROR_INFO err;

  while (stopFreq > 0 && GetSbrStopFreqRAW(stopFreq, sampleRate, coreCoder, srMode) > maxStopFreqRaw) {
    stopFreq--;
  }

  if (GetSbrStopFreqRAW(stopFreq, sampleRate, coreCoder, srMode) > maxStopFreqRaw)
    return -1;

  err = FindStartAndStopBand(min(sampleRate, 192000),
                             startFreq,
                             stopFreq,
                             srMode,
                             &startBand,
                             &stopBand,
                             coreCoder);
  if (err)
    return -1;

  return stopFreq;
}

unsigned int
IsProSettingAvail(CODEC_TYPE coreCoder,
                  unsigned int bitrate,
                  unsigned int numOutputChannels,
                  unsigned int sampleRateInput,
                  int bNoCoupling,
                  unsigned int *sampleRateCore,
                  int bDownSampledSbr) {
  int idx = INVALID_TABLE_IDX;
  TUNINGTABLE *tuningTable = NULL;
  int paramSets = 0;

  tuningTable = tuningTableGeneral;
  paramSets = sizeof(tuningTableGeneral) / sizeof(tuningTableGeneral[0]);

  if (sampleRateInput < 16000)
    return 0;

  if (bNoCoupling && numOutputChannels == 2) {
    numOutputChannels = 1;
    bitrate /= 2;
  }

  *sampleRateCore = sampleRateInput / 2;
  idx = getSbrTuningTableIndex(coreCoder, bitrate, numOutputChannels, *sampleRateCore, tuningTable, paramSets);

  if (idx == INVALID_TABLE_IDX && sampleRateInput > 44100) {
    *sampleRateCore = 22050;
    idx = getSbrTuningTableIndex(coreCoder, bitrate, numOutputChannels, *sampleRateCore, tuningTable, paramSets);
  }

  if (idx == INVALID_TABLE_IDX && sampleRateInput > 32000) {
    *sampleRateCore = 16000;
    idx = getSbrTuningTableIndex(coreCoder, bitrate, numOutputChannels, *sampleRateCore, tuningTable, paramSets);
  }

  if (idx == INVALID_TABLE_IDX && sampleRateInput > 24000) {
    *sampleRateCore = 12000;
    idx = getSbrTuningTableIndex(coreCoder, bitrate, numOutputChannels, *sampleRateCore, tuningTable, paramSets);
  }

  if (idx == INVALID_TABLE_IDX && sampleRateInput > 22050) {
    *sampleRateCore = 11025;
    idx = getSbrTuningTableIndex(coreCoder, bitrate, numOutputChannels, *sampleRateCore, tuningTable, paramSets);
  }

  if (idx == INVALID_TABLE_IDX)
    return 0;

  if (bDownSampledSbr) {
    int dsStopFreq = getDownsampledStopFreq(coreCoder,
                                            (*sampleRateCore) * 2,
                                            tuningTable[idx].startFreq,
                                            tuningTable[idx].stopFreq,
                                            DUAL_RATE);
    if (dsStopFreq < 0)
      return 0;
  }

  return 1;
}

HANDLE_ERROR_INFO
AdjustSbrSettings(const sbrConfigurationPtr config,
                  unsigned int bitRate,
                  SBR_CHANNELMODE sbrChannelmode,
                  unsigned int fsCore,
                  unsigned int transFac,
                  unsigned int standardBitrate,
                  unsigned int vbrMode,
                  unsigned int useSpeechConfig,
                  int bNoCoupling,
                  SR_MODE srMode,
                  int bDownSampledSbr,
                  SBR_WITH_AAC int bParametricStereo,
                  int bUseQmfInput,
                  int userStartFreq,
                  int userStopFreq) {
  unsigned int i, sampleRate;
  int idx = INVALID_TABLE_IDX;
  TUNINGTABLE *tuningTable = NULL;
  int paramSets = 0;

  CODEC_TYPE coreCoder = config->codecSettings.coreCodec;
  int numChannels = -1;

  switch (sbrChannelmode) {
    case SBR_CHANNELMODE_MONO:
    case SBR_CHANNELMODE_MPS212:
      numChannels = 1;
      break;
    case SBR_CHANNELMODE_STEREO:
      numChannels = 2;
      break;
    case SBR_CHANNELMODE_INVALID:
    default:
      assert(0);
      break;
  }

  if (coreCoder == CODEC_UNSPECIFIED) {
    return iisUtil_ERROR(CDI, "Codec not specified");
  }

  if (srMode == QUAD_RATE && coreCoder == CODEC_SAAC) {
    tuningTable = tuningTableSaacQuadRate;
    paramSets = sizeof(tuningTableSaacQuadRate) / sizeof(tuningTableSaacQuadRate[0]);
  } else {
    tuningTable = tuningTableGeneral;
    paramSets = sizeof(tuningTableGeneral) / sizeof(tuningTableGeneral[0]);
  }

  fsCore *= config->downScaleFactor;

  config->codecSettings.bitRate = bitRate;
  config->codecSettings.nChannels = numChannels;
  config->codecSettings.sampleFreq = fsCore;
  config->codecSettings.transFac = transFac;
  config->codecSettings.standardBitrate = standardBitrate;

  sampleRate = fsCore;

  if (srMode == DUAL_RATE) {
    sampleRate *= 2;
    config->srMode = DUAL_RATE;
  } else if (srMode == QUAD_RATE) {
    sampleRate *= 4;
    config->srMode = QUAD_RATE;
  }

  bNoCoupling = bNoCoupling && (numChannels == 2);
  if (bNoCoupling) {
    numChannels = 1;
    bitRate /= 2;
  }

  if (vbrMode) {
    if (vbrMode < 30)
      bitRate = 24000;
    else if (vbrMode < 40)
      bitRate = 28000;
    else if (vbrMode < 60)
      bitRate = 32000;
    else if (vbrMode < 75)
      bitRate = 40000;
    else
      bitRate = 48000;
    bitRate *= numChannels;

    if (numChannels == 1) {
      if (sampleRate == 44100 || sampleRate == 48000) {
        if (vbrMode < 40) bitRate = 32000;
      }
    }
    config->codecSettings.bitRate = bitRate;
  }

  if (coreCoder == CODEC_SAAC) {
    if (numChannels == 1) {
      config->useHBE = (bitRate < HBE_MAX_BITRATE_MONO) ? 1 : 0;
    } else {
      config->useHBE = (bitRate / numChannels < HBE_MAX_BITRATE_MULTI) ? 1 : 0;
    }

    if ((24000 == bitRate) && (numChannels == 1)) {
      config->tranDetNewThresh *= 1.02f;
    }
  }

  idx = getSbrTuningTableIndex(coreCoder, bitRate, sbrChannelmode, fsCore, tuningTable, paramSets);

  if (idx != INVALID_TABLE_IDX) {
    if (useSpeechConfig) {
      config->startFreq = tuningTable[idx].startFreqSpeech;
      config->stopFreq = tuningTable[idx].stopFreqSpeech;
    } else {
      config->startFreq = tuningTable[idx].startFreq;
      config->stopFreq = tuningTable[idx].stopFreq;
    }

    switch (coreCoder) {
      case CODEC_SAAC: {
        int k0 = 0;

        if (userStartFreq >= 0) {
          const int k0_wished = (int)(((float)(userStartFreq * 2 * SBR_QMF_CHANNELS) / sampleRate) + 0.5);
          int offset_wished;
          int const *v_offset;
          int bs_start_freq = 0;

          if (srMode == QUAD_RATE) {
            offset_wished = k0_wished - GetStartMin(sampleRate / 2, srMode, coreCoder);
            v_offset = GetVOffset(sampleRate / 2, coreCoder);
          } else {
            offset_wished = k0_wished - GetStartMin(sampleRate, srMode, coreCoder);
            v_offset = GetVOffset(sampleRate, coreCoder);
          }

          while ((bs_start_freq < 15) && (v_offset[bs_start_freq] < offset_wished)) {
            ++bs_start_freq;
          }

          if (srMode == QUAD_RATE) {
            k0 = GetStartMin(sampleRate / 2, srMode, coreCoder) + v_offset[bs_start_freq];
          } else {
            k0 = GetStartMin(sampleRate, srMode, coreCoder) + v_offset[bs_start_freq];
          }
          config->startFreq = bs_start_freq;
        }

        if (userStopFreq >= 0) {
          const int k2_wished = (int)(((float)(userStopFreq * 2 * SBR_QMF_CHANNELS) / sampleRate) + 0.5);
          int p_stopDkSort[13] = {0};
          int k2;
          int bs_stop_freq = 0;

          if (srMode == QUAD_RATE) {
            k2 = GetStopMin(sampleRate / 2, srMode);
          } else {
            k2 = GetStopMin(sampleRate, srMode);
          }

          GetStopDkSort(k2, p_stopDkSort);

          while ((k2 < k2_wished) && (bs_stop_freq < 13)) {
            k2 += p_stopDkSort[bs_stop_freq];
            ++bs_stop_freq;
          }
          if (k2 < k2_wished) {
            if (k0 * 2 >= k2_wished) {
              bs_stop_freq = 14;
            } else {
              bs_stop_freq = 15;
            }
          }
          config->stopFreq = bs_stop_freq;
        }
      } break;
      default:
        assert(0);
    }

    if (bDownSampledSbr) {
      int dsStopFreq = getDownsampledStopFreq(coreCoder,
                                              sampleRate,
                                              config->startFreq,
                                              config->stopFreq,
                                              srMode);
      if (dsStopFreq < 0)
        return iisUtil_ERROR(CDI, "dsStopFreq < 0");

      config->stopFreq = dsStopFreq;
    }
    config->bDownSampledSbr = bDownSampledSbr;

    config->sbr_noise_bands = tuningTable[idx].numNoiseBands;

    for (i = 0; i < MAX_NUM_NOISE_COEFFS; i++)
      config->noiseFloorOffset[i] = tuningTable[idx].noiseFloorOffset;

    config->ana_max_level = tuningTable[idx].noiseMaxLevel;

    if (bNoCoupling)
      config->stereoMode = SBR_LEFT_RIGHT;
    else
      config->stereoMode = tuningTable[idx].stereoMode;

    config->bs_interTes = 0;

    if (config->bMonoStereoScal) {
      return iisUtil_ERROR(CDI, "SBR Scalable not supported.");
    }
    config->freqScale = tuningTable[idx].freqScale;

    if (numChannels == 1) {
      switch (coreCoder) {
        case CODEC_SAAC:
          if (bitRate < 12000) {
            config->freq_res_fixfix[0] = FREQ_RES_LOW;
            config->freq_res_fixfix[1] = FREQ_RES_LOW;
            config->freqResFillPre = FREQ_RES_LOW;
            config->freqResFillPost = FREQ_RES_LOW;
          }
          break;
        default:
          assert(0);
      }

      if (useSpeechConfig) {
        switch (coreCoder) {
          case CODEC_SAAC:
            if (bitRate <= 24000) {
              config->freq_res_fixfix[0] = FREQ_RES_LOW;
              config->freq_res_fixfix[1] = FREQ_RES_LOW;
              config->freqResFillPre = FREQ_RES_LOW;
              config->freqResFillPost = FREQ_RES_LOW;
            }
            break;
          default:
            break;
        }
      }

    } else {
      switch (coreCoder) {
        case CODEC_SAAC:
          if (bitRate < 28000) {
            config->freq_res_fixfix[0] = FREQ_RES_LOW;
            config->freq_res_fixfix[1] = FREQ_RES_LOW;
            config->freqResFillPre = FREQ_RES_LOW;
            config->freqResFillPost = FREQ_RES_LOW;
          }
          break;
        default:
          break;
      }
    }

    switch (coreCoder) {
      case CODEC_SAAC:
        if (useSpeechConfig) {
          config->parametricCoding = 0;
        }
        if (numChannels == 1) {
          if (bitRate < 12000) {
            config->parametricCoding = 0;
          }
        } else {
          if (bitRate < 20000) {
            config->parametricCoding = 0;
          }
        }
        break;
      default:
        break;
    }

    config->useSpeechConfig = useSpeechConfig;

    config->bUseQmfInput = bUseQmfInput;

    return noError;
  } else {
    return iisUtil_ERROR(CDI, "Invalid Table Index");
  }
}

unsigned int SbrGetDelayEnc(struct SBR_CONFIG_DATA *cfg) {
  return cfg->sbrDelayEnc;
}

unsigned int SbrGetDelayDec(struct SBR_CONFIG_DATA *cfg) {
  return cfg->sbrDelayDec;
}

void SbrSetAllowPatchingSwitch(HANDLE_SBR_ENCODER hEnv, int bAllowSwitch) {
  if (hEnv) {
    hEnv->sbrConfigData->allowSwitch = bAllowSwitch;
  }

  return;
}

HANDLE_ERROR_INFO
InitializeSbrDefaults(sbrConfigurationPtr config,
                      CODEC_TYPE coreCoder,
                      int coreSbrFrameLenFac,
                      unsigned int codecGranuleLen) {
  int i;
  config->useSpeechConfig = 0;
  config->codecSettings.coreCodec = coreCoder;
  config->codecSettings.frameSize = codecGranuleLen;

  switch (coreSbrFrameLenFac) {
    case 1:
      config->srMode = SINGLE_RATE;
      break;
    case 2:
      config->srMode = DUAL_RATE;
      break;
    case 4:
      config->srMode = QUAD_RATE;
      break;
    default:
      return iisUtil_ERROR(CDI, "Invalid coreSbrFrameLenFac");
  }

  switch (coreCoder) {
    case CODEC_SAAC:
      config->SendHeaderDataTime = -1.0f;
      config->useWaveCoding = 0;
      config->crcSbr = 0;
      config->dynBwSupported = 1;
      config->tranDetNewThresh = 6000.0f;
      config->tranDetNewAver = 0;
      config->parametricCoding = 1;
      config->bMonoStereoScal = 0;
      break;
    default:
      return iisUtil_ERROR(CDI, "Invalid core coder");
  }

  config->SendHeaderDataOffs = 0.0f;

  config->amp_res = SBR_AMP_RES_3_0;
  config->tran_fc = 0;
  config->tran_det_mode = 1;
  config->spread = 1;
  config->stat = 0;
  config->e = 1;
  config->deltaTAcrossFrames = 1;

  switch (coreCoder) {
    default:
      config->dF_edge_1stEnv = 0.3f;
      config->dF_edge_incr = 0.3f;
  }

  config->sbr_invf_mode = INVF_SWITCHED;
  config->sbr_xpos_mode = XPOS_LC;
  config->sbr_xpos_ctrl = SBR_XPOS_CTRL_DEFAULT;
  config->sbr_xpos_level = 0;

  switch (coreCoder) {
    case CODEC_SAAC:
      config->dynBwEnabled = 1;
      break;
    default:
      return iisUtil_ERROR(CDI, "Invalid core coder");
  }

  config->freqResFillPre = FREQ_RES_HIGH;
  config->freqResFillPost = FREQ_RES_HIGH;

  config->downScaleFactor = 1;

  config->bDownSampledSbr = 0;

  config->stereoMode = SBR_SWITCH_LRC;
  config->ana_max_level = 6;
  for (i = 0; i < MAX_NUM_NOISE_COEFFS; i++)
    config->noiseFloorOffset[i] = 0;
  config->startFreq = 5;
  config->stopFreq = 9;
  config->stopFreqChanged = 0;
  config->freq_res_fixfix[0] = FREQ_RES_HIGH;
  config->freq_res_fixfix[1] = FREQ_RES_HIGH;

  config->freqScale = SBR_FREQ_SCALE_DEFAULT;
  config->alterScale = SBR_ALTER_SCALE_DEFAULT;
  config->sbr_noise_bands = SBR_NOISE_BANDS_DEFAULT;

  config->sbr_limiter_bands = SBR_LIMITER_BANDS_DEFAULT;
  config->sbr_limiter_gains = SBR_LIMITER_GAINS_DEFAULT;
  config->sbr_interpol_freq = SBR_INTERPOL_FREQ_DEFAULT;
  config->sbr_smoothing_length = SBR_SMOOTHING_LENGTH_DEFAULT;

  switch (coreCoder) {
    case CODEC_SAAC:

      config->noiseLevel = -1.0;
      config->useHBE = 0;
      config->bs_interTes = 0;
      config->sibilantTuning = 0;
      break;
    default:
      assert(0);
  }

  return noError;
}

static void
deleteEnvChannel(HANDLE_ENV_CHANNEL *hEnvCut,
                 CODEC_TYPE coreCodec) {
  if (*hEnvCut) {
    DeleteFrameInfoGenerator((*hEnvCut)->hSbrEnvFrame);
    QMFlib_DestroyAnalysis(&((*hEnvCut)->h_sbrQmf));
    QMFlib_DestroySynthesis(&((*hEnvCut)->h_sbrSynthQmf));
    iisFree((*hEnvCut)->pAnaTimeBufTmp);
    (*hEnvCut)->pAnaTimeBufTmp = NULL;
    deleteSbrCodeEnvelope((*hEnvCut)->h_sbrCodeEnvelope);

    deleteSbrCodeEnvelope((*hEnvCut)->h_sbrCodeNoiseFloor);

    DeleteSbrTransientDetector((*hEnvCut)->h_sbrTransientDetector);

    deleteExtractSbrEnvelope((*hEnvCut)->h_sbrExtractEnvelope);

    DeleteTonCorrParamExtr((*hEnvCut)->hTonCorr, coreCodec);

    switch (coreCodec) {
      case CODEC_SAAC:

        QMFlib_DestroyAnalysis(&((*hEnvCut)->h_sbrPatchQmf));

        break;
      default:
        assert(0);
    }

    if ((*hEnvCut)->h_sbrSibilantDetector != NULL) {
      DeleteSbrSibilantDetector(&(*hEnvCut)->h_sbrSibilantDetector);
    }

    iisFree(*hEnvCut);

    *hEnvCut = NULL;
  }
}

void EnvClose(HANDLE_SBR_ENCODER hEnvEnc) {
  int i;
  CODEC_TYPE coreCodec;
  if (hEnvEnc != NULL) {
    if (hEnvEnc->sbrConfigData != NULL) {
      coreCodec = hEnvEnc->sbrConfigData->coreCodec;
      for (i = 0; i < MAX_NUM_CHANNELS; i++) {
        if (hEnvEnc->hEnvChannel[i] != NULL) {
          deleteEnvChannel(&hEnvEnc->hEnvChannel[i],
                           coreCodec);
          hEnvEnc->hEnvChannel[i] = NULL;
        }
      }
    }

    if (hEnvEnc->hHdrTimer)
      DeleteIntervalTimer(&(hEnvEnc->hHdrTimer));

    if (hEnvEnc->sbrConfigData != NULL) {
      if (hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_LOW]) {
        iisFree(hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_LOW]);
      }
      if (hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_HIGH]) {
        iisFree(hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_HIGH]);
      }
      if (hEnvEnc->sbrConfigData->v_k_master) {
        iisFree(hEnvEnc->sbrConfigData->v_k_master);
      }
      iisFree(hEnvEnc->sbrConfigData);
    }
    if (hEnvEnc->sbrHeaderData) {
      iisFree(hEnvEnc->sbrHeaderData);
    }
    if (hEnvEnc->sbrBitstreamData) {
      iisFree(hEnvEnc->sbrBitstreamData);
    }
    if (hEnvEnc->downsampledOutSignal) {
      iisFree(hEnvEnc->downsampledOutSignal);
    }
    iisFree(hEnvEnc);
  }
}

static HANDLE_ERROR_INFO
updateFreqBandTable(HANDLE_SBR_CONFIG_DATA sbrConfigData,
                    HANDLE_SBR_HEADER_DATA sbrHeaderData,
                    int buildNewMaster) {
  HANDLE_ERROR_INFO errorInfo = noError;
  int k0, k2;

  if (buildNewMaster) {
    errorInfo =
        FindStartAndStopBand(sbrConfigData->sampleFreq,
                             sbrHeaderData->sbr_start_frequency,
                             sbrHeaderData->sbr_stop_frequency,
                             sbrHeaderData->sampleRateMode,
                             &k0,
                             &k2,
                             sbrConfigData->coreCodec);

    if (errorInfo != noError)
      return handBack(errorInfo);

    errorInfo =
        UpdateFreqScale(sbrConfigData->v_k_master, &sbrConfigData->num_Master,
                        k0, k2, sbrHeaderData->freqScale,
                        sbrHeaderData->alterScale,
                        sbrHeaderData->sampleRateMode);

    if (errorInfo != noError)
      return handBack(errorInfo);

    sbrHeaderData->sbr_xover_band = 0;
  }

  errorInfo =
      UpdateHiRes(sbrConfigData->freqBandTable[FREQ_RES_HIGH],
                  &sbrConfigData->nSfb[FREQ_RES_HIGH],
                  sbrConfigData->v_k_master,
                  sbrConfigData->num_Master,
                  &sbrHeaderData->sbr_xover_band,
                  sbrHeaderData->sampleRateMode,
                  sbrConfigData->coreCodec);

  if (errorInfo != noError)
    return handBack(errorInfo);

  UpdateLoRes(sbrConfigData->freqBandTable[FREQ_RES_LOW],
              &sbrConfigData->nSfb[FREQ_RES_LOW],
              sbrConfigData->freqBandTable[FREQ_RES_HIGH],
              sbrConfigData->nSfb[FREQ_RES_HIGH]);

  sbrConfigData->xOverFreq = 0.5f * sbrConfigData->freqBandTable[FREQ_RES_LOW][0] * sbrConfigData->sampleFreq / SBR_QMF_CHANNELS;
  return noError;
}

static HANDLE_ERROR_INFO
resetEnvChannel(HANDLE_SBR_CONFIG_DATA sbrConfigData,
                HANDLE_SBR_HEADER_DATA sbrHeaderData,
                HANDLE_ENV_CHANNEL hEnv) {
  HANDLE_ERROR_INFO errorInfo = noError;

  hEnv->hTonCorr->h_sbrNoiseFloorEstimate->noiseBands = sbrHeaderData->sbr_noise_bands;

  errorInfo =
      ResetTonCorrParamExtr(hEnv->hTonCorr,
                            sbrConfigData->xposCtrlSwitch,
                            sbrConfigData->freqBandTable[FREQ_RES_HIGH][0],
                            sbrConfigData->v_k_master,
                            sbrConfigData->num_Master,
                            sbrConfigData->sampleFreq,
                            (const int *const *const)sbrConfigData->freqBandTable,
                            sbrConfigData->nSfb,
                            hEnv->no_channels,
                            sbrConfigData->coreCodec);

  if (errorInfo != noError)
    return handBack(errorInfo);

  hEnv->h_sbrCodeNoiseFloor->nSfb[FREQ_RES_LOW] = hEnv->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands;
  hEnv->h_sbrCodeNoiseFloor->nSfb[FREQ_RES_HIGH] = hEnv->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands;

  hEnv->h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW] = sbrConfigData->nSfb[FREQ_RES_LOW];
  hEnv->h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH] = sbrConfigData->nSfb[FREQ_RES_HIGH];

  switch (sbrConfigData->coreCodec) {
    case CODEC_SAAC:
      hEnv->h_sbrCodeEnvelope->offset = 2 * sbrConfigData->nSfb[FREQ_RES_LOW] - sbrConfigData->nSfb[FREQ_RES_HIGH];
      break;
    default:

      break;
  }

  hEnv->encEnvData.noHarmonics = sbrConfigData->nSfb[FREQ_RES_HIGH];

  hEnv->h_sbrCodeEnvelope->upDate = 0;
  hEnv->h_sbrCodeNoiseFloor->upDate = 0;

  return noError;
}

static void
initXOverFreqVector(struct SBR_CONFIG_DATA *hSbrConfigData,
                    HANDLE_COMMON_DATA hCmonData) {
  int band;
  int len = -1;
  const float qmfHzPerBand = hSbrConfigData->sampleFreq * 0.5f / SBR_QMF_CHANNELS;
  switch (hSbrConfigData->coreCodec) {
    case CODEC_SAAC:
      len = 1 << SI_SBR_XOVER_BAND_BITS_SAAC;
      break;
    default:
      assert(0);
  }

  len = (hSbrConfigData->num_Master < len) ? hSbrConfigData->num_Master : len;

  for (band = 0; band < len; band++) {
    hCmonData->allowedXOverFreqs[band] = qmfHzPerBand * hSbrConfigData->v_k_master[band];
  }
  hCmonData->allowedXOverFreqsLen = len;
}

static int checkFLOATBitExactness(float *a, float *b) {
  int result = (*(int *)a == *(int *)b);
  return result;
}

HANDLE_ERROR_INFO
EnvEncodeFrame(HANDLE_SBR_ENCODER hEnvEncoder,
               HANDLE_COMMON_DATA hCmonData,
               const float *const pTimeDomainSamples,
               const float *const *const ppQmfSamplesReal,
               const float *const *const ppQmfSamplesImag,
               const int isSwitchingDecisionResultSpeech,
               int const bUsacIndependenceFlag) {
  HANDLE_ERROR_INFO error = noError;

  hEnvEncoder->sbrConfigData->switchingDecision = isSwitchingDecisionResultSpeech;

  if (hEnvEncoder != NULL) {
    HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData = hEnvEncoder->sbrBitstreamData;

    if (IntervalTimerIsExpired(hEnvEncoder->hHdrTimer) || bUsacIndependenceFlag) {
      sbrBitstreamData->HeaderActive = 1;
    } else {
      sbrBitstreamData->HeaderActive = 0;
    }

    IntervalTimerAdvance(hEnvEncoder->hHdrTimer);

    if (hCmonData->dynBwEnabled) {
      int ch;
      int band;
      int cutoffSb;
      float newXOver;

      HANDLE_ERROR_INFO err;

      switch (hEnvEncoder->sbrConfigData->coreCodec) {
        case CODEC_SAAC: {
          int i;
          int const RAISEDELAY = 1;
          int const LOWERDELAY = 1;

          for (i = sizeof(hEnvEncoder->dynXOverFreqDelay) / sizeof(hEnvEncoder->dynXOverFreqDelay[0]) - 1; i > 0; i--) {
            hEnvEncoder->dynXOverFreqDelay[i] = hEnvEncoder->dynXOverFreqDelay[i - 1];
          }
          hEnvEncoder->dynXOverFreqDelay[0] = hCmonData->dynXOverFreqEnc;

          newXOver = hEnvEncoder->dynXOverFreqDelay[0];

          if (hEnvEncoder->dynXOverFreqDelay[0] > hEnvEncoder->dynXOverFreqDelay[RAISEDELAY]) {
            newXOver = hEnvEncoder->dynXOverFreqDelay[RAISEDELAY];
          }

          if (hEnvEncoder->dynXOverFreqDelay[0] < hEnvEncoder->dynXOverFreqDelay[LOWERDELAY]) {
            newXOver = hEnvEncoder->dynXOverFreqDelay[LOWERDELAY];
          }
        } break;
        default:
          return iisUtil_ERROR(CDI, "core codec not supported");
      }

      if (!checkFLOATBitExactness(&hEnvEncoder->sbrConfigData->dynXOverFreq, &newXOver)) {
        cutoffSb = (int)((2.0f * newXOver * SBR_QMF_CHANNELS / hEnvEncoder->sbrConfigData->sampleFreq) + 0.5f);
        for (band = 0; band < hEnvEncoder->sbrConfigData->num_Master; band++) {
          if (cutoffSb == hEnvEncoder->sbrConfigData->v_k_master[band])
            break;
        }
        assert(band < hEnvEncoder->sbrConfigData->num_Master);

        hEnvEncoder->sbrConfigData->dynXOverFreq = newXOver;
        hEnvEncoder->sbrHeaderData->sbr_xover_band = band;
        hEnvEncoder->sbrBitstreamData->HeaderActive = 1;

        err = updateFreqBandTable(hEnvEncoder->sbrConfigData,
                                  hEnvEncoder->sbrHeaderData,
                                  FALSE);

        if (err != noError)
          return handBack(err);

        for (ch = 0; ch < hEnvEncoder->sbrConfigData->nChannels; ch++) {
          err = resetEnvChannel(hEnvEncoder->sbrConfigData,
                                hEnvEncoder->sbrHeaderData,
                                hEnvEncoder->hEnvChannel[ch]);
          if (err != noError)
            return handBack(err);
        }
        initXOverFreqVector(hEnvEncoder->sbrConfigData,
                            hCmonData);
      }
    }

    int ch;

    for (ch = 0; ch < hEnvEncoder->nChannelsQmf; ch++) {
      float **realQmfData = GetQmfRealBufferWrite(hEnvEncoder->hEnvChannel, ch);
      float **imagQmfData = GetQmfImagBufferWrite(hEnvEncoder->hEnvChannel, ch);

      if (hEnvEncoder->sbrConfigData->bUseQmfInput) {
        int i;

        if (error == noError) {
          if ((ppQmfSamplesReal == NULL) ||
              (ppQmfSamplesImag == NULL)) {
            error = iisUtil_ERROR(CDI, "Invalid handle.");
          }
        }

        for (i = 0; i < hEnvEncoder->sbrConfigData->inputFrameSize / hEnvEncoder->hEnvChannel[0]->no_channels; i++) {
          if (error == noError) {
            if ((ppQmfSamplesReal[hEnvEncoder->timeInputStride * i + ch] == NULL) ||
                (ppQmfSamplesImag[hEnvEncoder->timeInputStride * i + ch] == NULL)) {
              error = iisUtil_ERROR(CDI, "Invalid handle.");
            }
          }
          if (error == noError) {
            copyFLOAT(ppQmfSamplesReal[hEnvEncoder->timeInputStride * i + ch], realQmfData[i], hEnvEncoder->hEnvChannel[0]->no_channels);
            copyFLOAT(ppQmfSamplesImag[hEnvEncoder->timeInputStride * i + ch], imagQmfData[i], hEnvEncoder->hEnvChannel[0]->no_channels);
          }
        }
      } else {
        if (error == noError) {
          {
            const float *timeInput = pTimeDomainSamples + ch;
            int stride = hEnvEncoder->timeInputStride;
            {
              const float *timeInputTmp;
              int ts;

              for (ts = 0; ts < hEnvEncoder->hEnvChannel[ch]->no_col; ts++) {
                if (stride > 1) {
                  copyFLOATflex(timeInput, stride, hEnvEncoder->hEnvChannel[ch]->pAnaTimeBufTmp, 1, hEnvEncoder->hEnvChannel[ch]->no_channels);
                  timeInputTmp = hEnvEncoder->hEnvChannel[ch]->pAnaTimeBufTmp;
                } else {
                  timeInputTmp = timeInput;
                }

                QMFlib_CalculateAnalysis(
                    GetSbrQmfHandle(hEnvEncoder->hEnvChannel, ch),
                    timeInputTmp,
                    realQmfData[ts],
                    imagQmfData[ts]);
                timeInput += stride * hEnvEncoder->hEnvChannel[ch]->no_channels;
              }
            }
          }
        }
      }
    }

    if (error == noError) {
      error = ExtractSbrEnvelope(hEnvEncoder,
                                 hEnvEncoder->sbrConfigData->stereoMode,
                                 hCmonData, isSwitchingDecisionResultSpeech,
                                 bUsacIndependenceFlag, SBR_EMERGENCY_MODE_OFF);
    }

    if (error == noError) {
      sbrBitstreamData->HeaderActive = 0;
      sbrBitstreamData->rightBorderFIX = 0;
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

static HANDLE_ERROR_INFO createEnvChannel(HANDLE_SBR_CONFIG_DATA sbrConfigData,
                                          HANDLE_SBR_HEADER_DATA sbrHeaderData,
                                          HANDLE_ENV_CHANNEL *hEnv,
                                          sbrConfigurationPtr params,
                                          CODEC_TYPE coreCoder) {
  HANDLE_ERROR_INFO errorInfo;
  HANDLE_ENV_CHANNEL hs;
  const int bDownSampledSbr = params->bDownSampledSbr;
  int numEnvStatic;
  int tran_fc, tran_off;
  int timeSlots, timeStep, startIndex;
  int noiseBands[2] = {3, 3};
  float sb_width;
  QMF_FILTERMODE fbtype;
  hs = (HANDLE_ENV_CHANNEL)iisCalloc(sizeof(struct ENV_CHANNEL), 1);
  if (hs == NULL) return iisUtil_ERROR(CDI, "out of memory");

  numEnvStatic = 1 << params->e;

  hs->encEnvData.freq_res_fixfix[0] = params->freq_res_fixfix[0];
  hs->encEnvData.freq_res_fixfix[1] = params->freq_res_fixfix[1];

  hs->encEnvData.sbr_xpos_mode = params->sbr_xpos_mode;

  if (hs->encEnvData.sbr_xpos_mode == XPOS_SWITCHED) {
    sbrConfigData->switchTransposers = TRUE;
    hs->encEnvData.sbr_xpos_mode = XPOS_MDCT;
  } else {
    sbrConfigData->switchTransposers = FALSE;
  }

  hs->encEnvData.sbr_xpos_ctrl = params->sbr_xpos_ctrl;

  switch (sbrConfigData->coreCodec) {
    default:
    case CODEC_SAAC:
      fbtype = FM_SBRQMF;
      break;
  }

  hs->encEnvData.extended_data = 0;
  hs->encEnvData.extension_size = 0;

  hs->no_channels = SBR_QMF_CHANNELS / ((bDownSampledSbr ? 2 : 1) * sbrConfigData->downScaleFactor);
  hs->no_channels_syn = SBR_QMF_CHANNELS / RESAMPLE_BY_2 * sbrConfigData->downScaleFactor;
  hs->no_col = sbrConfigData->inputFrameSize / hs->no_channels;

  hs->pAnaTimeBufTmp = (float *)iisCalloc(hs->no_channels, sizeof(float));
  errorInfo = QMFlib_CreateAnalysisExt(&(hs->h_sbrQmf),
                                       hs->no_channels,
                                       0,
                                       fbtype,
                                       bDownSampledSbr);

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  {
    errorInfo = QMFlib_CreateSynthesisExt(&(hs->h_sbrSynthQmf),
                                          hs->no_channels_syn,
                                          0,
                                          fbtype);

    if (errorInfo != noError) {
      deleteEnvChannel(&hs, sbrConfigData->coreCodec);
      return handBack(errorInfo);
    }
  }

  startIndex = SBR_QMF_PROTOFILT_LENGTH / ((bDownSampledSbr ? 2 : 1) * sbrConfigData->downScaleFactor) - hs->no_channels;

  switch ((int)(sbrConfigData->frameSize * sbrConfigData->downScaleFactor)) {
    case 4096:
    case 2048:
    case 1024:
      timeSlots = 16;
      break;
    default:
      deleteEnvChannel(&hs, sbrConfigData->coreCodec);
      return iisUtil_ERROR(CDI, "SBR:Illegal frame size");
  }

  timeStep = hs->no_col / timeSlots;
  assert(timeStep > 0);

  errorInfo = CreateExtractSbrEnvelope(&hs->h_sbrExtractEnvelope, hs->no_col,
                                       hs->no_channels, startIndex,
                                       timeSlots, timeStep,
                                       sbrConfigData);
  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  if (sbrConfigData->coreCodec == CODEC_SAAC) {
    if (sbrConfigData->sibilantTuning == 1) {
      errorInfo = CreateSbrSibilantDetector(&hs->h_sbrSibilantDetector);
    }
  }

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  errorInfo = CreateTonCorrParamExtr(&hs->hTonCorr,
                                     sbrConfigData->inputFrameSize,
                                     timeSlots,
                                     hs->no_col,
                                     sbrConfigData->sbrDelayEnc,
                                     sbrConfigData->inputSampleFreq / sbrConfigData->downScaleFactor,
                                     hs->no_channels,
                                     params->sbr_xpos_ctrl,
                                     sbrConfigData->freqBandTable[FREQ_RES_LOW][0],
                                     sbrConfigData->v_k_master,
                                     sbrConfigData->num_Master,
                                     params->ana_max_level,
                                     (const int *const *const)sbrConfigData->freqBandTable,
                                     sbrConfigData->nSfb,
                                     sbrHeaderData->sbr_noise_bands,
                                     params->noiseFloorOffset,
                                     sbrConfigData->useParametricCoding,
                                     params->useSpeechConfig,
                                     sbrConfigData->coreCodec);

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  hs->encEnvData.noOfnoisebands = hs->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands;

  if (hs->encEnvData.noOfnoisebands > MAX_NUM_NOISE_COEFFS) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid SBR tuning table: number of noise bands is too high");
  }

  noiseBands[0] = hs->encEnvData.noOfnoisebands;
  noiseBands[1] = hs->encEnvData.noOfnoisebands;

  hs->encEnvData.sbr_invf_mode = params->sbr_invf_mode;

  if (hs->encEnvData.sbr_invf_mode == INVF_SWITCHED) {
    hs->encEnvData.sbr_invf_mode = INVF_MID_LEVEL;
    hs->hTonCorr->switchInverseFilt = TRUE;
  } else {
    hs->hTonCorr->switchInverseFilt = FALSE;
  }

  if (errorInfo == noError) {
    errorInfo = CreateSbrCodeEnvelope(&hs->h_sbrCodeEnvelope,
                                      sbrConfigData->nSfb,
                                      params->deltaTAcrossFrames,
                                      params->dF_edge_1stEnv,
                                      params->dF_edge_incr);
  }

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  if (errorInfo == noError) {
    errorInfo = CreateSbrCodeEnvelope(&hs->h_sbrCodeNoiseFloor,
                                      noiseBands,
                                      params->deltaTAcrossFrames,
                                      0, 0);
  }
  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  if (errorInfo == noError) {
    sbrConfigData->thresholdAmpResFF = (int)params->threshold_amp_res_FF;
    errorInfo = InitSbrHuffmanTables(&hs->encEnvData,
                                     hs->h_sbrCodeEnvelope,
                                     hs->h_sbrCodeNoiseFloor,
                                     sbrHeaderData->sbr_amp_res,
                                     sbrConfigData->coreCodec);
  }

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  if (errorInfo == noError) {
    errorInfo = CreateFrameInfoGenerator(&hs->hSbrEnvFrame,
                                         params->spread,
                                         numEnvStatic,
                                         params->stat,
                                         params->freqResFillPre,
                                         params->freqResFillPost,
                                         timeSlots,

                                         params->freq_res_fixfix,
                                         sbrConfigData->coreCodec,
                                         params);
  }

  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  if (params->codecSettings.coreCodec == CODEC_SAAC) {
    sb_width = (float)sbrConfigData->sampleFreq / (2 * SBR_QMF_CHANNELS);
  } else {
    sb_width = (float)params->codecSettings.sampleFreq * 2 / (2 * SBR_QMF_CHANNELS);
  }

  tran_fc = params->tran_fc;
  if ((tran_fc == 0) && (sbrHeaderData->sbr_stop_frequency > 13)) {
    int startFreq =
        GetSbrStartFreqRAW(sbrHeaderData->sbr_start_frequency, sbrConfigData->sampleFreq,
                           sbrConfigData->coreCodec, sbrHeaderData->sampleRateMode);
    tran_fc = (sbrHeaderData->sbr_stop_frequency == 14 ? 2 * startFreq : 3 * startFreq);
  }
  if (tran_fc == 0) {
    if (params->srMode == QUAD_RATE) {
      tran_fc = GetSbrStopFreqRAW(sbrHeaderData->sbr_stop_frequency,
                                  sbrConfigData->sampleFreq / 2,
                                  sbrConfigData->coreCodec,
                                  sbrHeaderData->sampleRateMode);
    } else {
      tran_fc = GetSbrStopFreqRAW(sbrHeaderData->sbr_stop_frequency,
                                  sbrConfigData->sampleFreq,
                                  sbrConfigData->coreCodec,
                                  sbrHeaderData->sampleRateMode);
    }
  }

  tran_fc = (int)(tran_fc / sb_width + 0.5);
  tran_fc = min(SBR_QMF_CHANNELS, tran_fc);

  switch (sbrConfigData->coreCodec) {
    default:
      tran_off = timeStep * hs->hSbrEnvFrame->frameMiddleSlot;
      break;
  }

  if (errorInfo == noError) {
    errorInfo = CreateSbrTransientDetector(&hs->h_sbrTransientDetector,
                                           sbrConfigData->inputFrameSize * sbrConfigData->downScaleFactor,
                                           sbrConfigData->inputSampleFreq,
                                           (params->codecSettings.standardBitrate * params->codecSettings.nChannels) / (float)params->codecSettings.bitRate,
                                           params->tranDetNewThresh,
                                           tran_fc,
                                           params->tranDetNewAver,
                                           hs->no_col,
                                           hs->no_channels,
                                           tran_off,
                                           hs->h_sbrExtractEnvelope->YBufferWriteOffset - hs->h_sbrExtractEnvelope->YBufferReadOffset + hs->no_col,
                                           coreCoder,
                                           sbrConfigData->downScaleFactor);
  }
  if (errorInfo != noError) {
    deleteEnvChannel(&hs, sbrConfigData->coreCodec);
    return handBack(errorInfo);
  }

  sbrConfigData->xposCtrlSwitch = params->sbr_xpos_ctrl;
  hs->encEnvData.noHarmonics = sbrConfigData->nSfb[FREQ_RES_HIGH];
  hs->encEnvData.addHarmonicFlag = 0;

  {
    hs->encEnvData.sbrPatchingMode = 1;
  }

  *hEnv = hs;
  return errorInfo;
}

void GetEnergyFromCplxQmfData_Opt(float **energyValues,
                                  float **realValues,
                                  float **imagValues,
                                  int numberBands,
                                  int numberCols);

HANDLE_ERROR_INFO
EnvOpen(HANDLE_SBR_ENCODER *hEnvEncoder,
        sbrConfigurationPtr params,
        HANDLE_COMMON_DATA hCmonData,
        CODEC_TYPE coreCoder) {
  HANDLE_SBR_ENCODER hEnvEnc = NULL;
  HANDLE_ERROR_INFO errorInfo = noError;
  const int bDownSampledSbr = params->bDownSampledSbr;
  int ch;
  unsigned int i;
  if ((params->codecSettings.nChannels < 1) || (params->codecSettings.nChannels > MAX_NUM_CHANNELS))
    return iisUtil_ERROR(CDI, "unsupported number of channels");

  if (params->useHBE) {
    return iisUtil_ERROR(CDI, "usage of HBE not supported");
  }

  hEnvEnc = (struct SBR_ENCODER *)iisCalloc(1, sizeof(struct SBR_ENCODER));
  if (!hEnvEnc) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrBitstreamData = (struct SBR_BITSTREAM_DATA *)iisCalloc(1, sizeof(struct SBR_BITSTREAM_DATA));
  if (!hEnvEnc->sbrBitstreamData) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrConfigData = (struct SBR_CONFIG_DATA *)iisCalloc(1, sizeof(struct SBR_CONFIG_DATA));
  if (!hEnvEnc->sbrConfigData) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_LOW] = (int *)iisCalloc((MAX_FREQ_COEFFS / 2 + 1), sizeof(int));
  if (!hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_LOW]) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_HIGH] = (int *)iisCalloc((MAX_FREQ_COEFFS + 1), sizeof(int));
  if (!hEnvEnc->sbrConfigData->freqBandTable[FREQ_RES_HIGH]) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrConfigData->v_k_master = (int *)iisCalloc((MAX_FREQ_COEFFS + 1), sizeof(int));
  if (!hEnvEnc->sbrConfigData->v_k_master) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->sbrHeaderData = (struct SBR_HEADER_DATA *)iisCalloc(1, sizeof(struct SBR_HEADER_DATA));
  if (!hEnvEnc->sbrHeaderData) {
    errorInfo = iisUtil_ERROR(CDI, "out of memory");
    goto error_exit;
  }

  hEnvEnc->GetEnergyFromCplxQmfData_Ptr = &GetEnergyFromCplxQmfData_NoOpt;

  hEnvEnc->nChannelsSbr = params->codecSettings.nChannels;

  hEnvEnc->nChannelsQmf = params->codecSettings.nChannels;

  switch (coreCoder) {
    case CODEC_SAAC:
      hEnvEnc->timeInputStride = params->timeInputStride;
      break;
    default:
      assert(0);
      break;
  }

  hEnvEnc->sbrConfigData->overrideTimeDiffCoding = 0;

  hEnvEnc->hHdrTimer = CreateIntervalTimer();
  if (!hEnvEnc->hHdrTimer) {
    errorInfo = iisUtil_ERROR(CDI, "creation interval timer failed");
    goto error_exit;
  }

  hEnvEnc->sbrConfigData->bitRate = params->codecSettings.bitRate;
  hEnvEnc->sbrConfigData->nChannelsInput = params->codecSettings.nChannelsInput;
  hEnvEnc->sbrConfigData->nChannels = params->codecSettings.nChannels;
  hEnvEnc->sbrConfigData->coreCodec = params->codecSettings.coreCodec;
  hEnvEnc->sbrConfigData->bUseQmfInput = params->bUseQmfInput;
  hEnvEnc->sbrConfigData->bs_interTes = params->bs_interTes;
  hEnvEnc->sbrConfigData->sibilantTuning = params->sibilantTuning;
  if (params->codecSettings.coreCodec != CODEC_SAAC) {
    hEnvEnc->sbrConfigData->bs_interTes = 0;
    hEnvEnc->sbrConfigData->sibilantTuning = 0;
  }

  if (params->codecSettings.nChannels == 2)
    hEnvEnc->sbrConfigData->stereoMode = params->stereoMode;
  else
    hEnvEnc->sbrConfigData->stereoMode = SBR_MONO;

  if (params->bMonoStereoScal) {
    errorInfo = iisUtil_ERROR(CDI, "Scalable SBR not supported.");
    goto error_exit;
  }

  switch (params->srMode) {
    case DUAL_RATE:
      hEnvEnc->sbrHeaderData->sampleRateMode = DUAL_RATE;
      hEnvEnc->sbrConfigData->frameSize = 2 * params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->sampleFreq = 2 * params->codecSettings.sampleFreq;

      hEnvEnc->sbrConfigData->inputFrameSize = 2 * params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->inputSampleFreq = 2 * params->codecSettings.sampleFreq;

      if (bDownSampledSbr) {
        hEnvEnc->sbrConfigData->inputFrameSize = params->codecSettings.frameSize;
        hEnvEnc->sbrConfigData->inputSampleFreq = params->codecSettings.sampleFreq;
      }
      break;

    case QUAD_RATE:
      hEnvEnc->sbrHeaderData->sampleRateMode = QUAD_RATE;
      hEnvEnc->sbrConfigData->frameSize = 4 * params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->sampleFreq = 4 * params->codecSettings.sampleFreq;
      hEnvEnc->sbrConfigData->inputFrameSize = 4 * params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->inputSampleFreq = 4 * params->codecSettings.sampleFreq;

      if (bDownSampledSbr) {
        hEnvEnc->sbrConfigData->inputFrameSize = 2 * params->codecSettings.frameSize;
        hEnvEnc->sbrConfigData->inputSampleFreq = 2 * params->codecSettings.sampleFreq;
      }
      break;

    case SINGLE_RATE:

      hEnvEnc->sbrHeaderData->sampleRateMode = SINGLE_RATE;
      hEnvEnc->sbrConfigData->frameSize = params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->sampleFreq = params->codecSettings.sampleFreq;
      hEnvEnc->sbrConfigData->inputFrameSize = params->codecSettings.frameSize;
      hEnvEnc->sbrConfigData->inputSampleFreq = params->codecSettings.sampleFreq;

      if (bDownSampledSbr) {
        hEnvEnc->sbrConfigData->inputFrameSize = params->codecSettings.frameSize / 2;
        hEnvEnc->sbrConfigData->inputSampleFreq = params->codecSettings.sampleFreq / 2;
      }
      break;

    default:
      errorInfo = iisUtil_ERROR(CDI, "unknown SR_MODE");
      goto error_exit;
  }

  hEnvEnc->sbrConfigData->downScaleFactor = params->downScaleFactor;

  if (IntervalTimerInit(hEnvEnc->hHdrTimer,
                        params->SendHeaderDataTime,
                        params->SendHeaderDataOffs,
                        params->codecSettings.sampleFreq,
                        params->codecSettings.frameSize)) {
    errorInfo = iisUtil_ERROR(CDI, "initializing interval timer failed");
    goto error_exit;
  }

  hEnvEnc->sbrHeaderData->sbr_data_extra = 0;
  hEnvEnc->sbrBitstreamData->CRCActive = params->crcSbr;
  hEnvEnc->sbrBitstreamData->HeaderActive = 1;
  hEnvEnc->sbrHeaderData->sbr_start_frequency = params->startFreq;
  hEnvEnc->sbrHeaderData->sbr_stop_frequency = params->stopFreq;
  hEnvEnc->sbrHeaderData->sbr_xover_band = 0;
  hEnvEnc->sbrHeaderData->sbr_lc_stereo_mode = 0;

  hEnvEnc->sbrBitstreamData->rightBorderFIX = 0;
  hEnvEnc->sbrHeaderData->sbr_amp_res = params->amp_res;

  hEnvEnc->sbrHeaderData->freqScale = params->freqScale;
  hEnvEnc->sbrHeaderData->alterScale = params->alterScale;
  hEnvEnc->sbrHeaderData->sbr_noise_bands = params->sbr_noise_bands;
  hEnvEnc->sbrHeaderData->header_extra_1 = 0;
  if ((params->freqScale != SBR_FREQ_SCALE_DEFAULT) ||
      (params->alterScale != SBR_ALTER_SCALE_DEFAULT) ||
      (params->sbr_noise_bands != SBR_NOISE_BANDS_DEFAULT)) {
    hEnvEnc->sbrHeaderData->header_extra_1 = 1;
  }

  hEnvEnc->sbrHeaderData->sbr_limiter_bands = params->sbr_limiter_bands;
  hEnvEnc->sbrHeaderData->sbr_limiter_gains = params->sbr_limiter_gains;
  if ((hEnvEnc->sbrConfigData->sampleFreq > 48000) &&
      (hEnvEnc->sbrHeaderData->sbr_start_frequency >= 9)) {
    hEnvEnc->sbrHeaderData->sbr_limiter_gains = SBR_LIMITER_GAINS_INFINITE;
  }

  hEnvEnc->sbrHeaderData->sbr_interpol_freq = params->sbr_interpol_freq;
  hEnvEnc->sbrHeaderData->sbr_smoothing_length = params->sbr_smoothing_length;
  hEnvEnc->sbrHeaderData->header_extra_2 = 0;
  if ((params->sbr_limiter_bands != SBR_LIMITER_BANDS_DEFAULT) ||
      (hEnvEnc->sbrHeaderData->sbr_limiter_gains != SBR_LIMITER_GAINS_DEFAULT) ||
      (params->sbr_interpol_freq != SBR_INTERPOL_FREQ_DEFAULT) ||
      (params->sbr_smoothing_length != SBR_SMOOTHING_LENGTH_DEFAULT)) {
    hEnvEnc->sbrHeaderData->header_extra_2 = 1;
  }

  hEnvEnc->sbrConfigData->useWaveCoding = params->useWaveCoding;
  hEnvEnc->sbrConfigData->useParametricCoding = params->parametricCoding;
  hEnvEnc->sbrConfigData->bReducedCoreCoderFrameLength = params->bReducedCoreCoderFrameLength;
  hEnvEnc->sbrConfigData->bSbr41 = params->bSbr41;

  hEnvEnc->sbrConfigData->noiseLevel = params->noiseLevel;
  hEnvEnc->sbrConfigData->allowSwitch = 0;

  errorInfo = updateFreqBandTable(hEnvEnc->sbrConfigData,
                                  hEnvEnc->sbrHeaderData,
                                  TRUE);

  if (errorInfo != noError) {
    errorInfo = handBack(errorInfo);
    goto error_exit;
  }

  if (hEnvEnc->sbrConfigData->v_k_master[hEnvEnc->sbrConfigData->num_Master] >
      (SBR_QMF_CHANNELS / ((bDownSampledSbr ? 2 : 1) * hEnvEnc->sbrConfigData->downScaleFactor))) {
    errorInfo = iisUtil_ERROR(CDI, "Too high SBR stop frequency");
    goto error_exit;
  }

  for (ch = 0; ch < hEnvEnc->nChannelsQmf; ch++) {
    errorInfo = createEnvChannel(hEnvEnc->sbrConfigData,
                                 hEnvEnc->sbrHeaderData,
                                 &hEnvEnc->hEnvChannel[ch],
                                 params,
                                 coreCoder);
    if (errorInfo != noError) {
      errorInfo = handBack(errorInfo);
      goto error_exit;
    }
  }

  hCmonData->xOverFreq = hEnvEnc->sbrConfigData->xOverFreq;
  hCmonData->dynBwEnabled = (params->dynBwSupported && params->dynBwEnabled);
  hCmonData->dynXOverFreqEnc = hEnvEnc->sbrConfigData->xOverFreq;
  if (hCmonData->dynBwEnabled) {
    initXOverFreqVector(hEnvEnc->sbrConfigData,
                        hCmonData);
    hCmonData->dynXOverFreqEnc = hCmonData->allowedXOverFreqs[0];
  }

  switch (params->codecSettings.coreCodec) {
    case CODEC_SAAC:
      for (i = 0; i < sizeof(hEnvEnc->dynXOverFreqDelay) / sizeof(hEnvEnc->dynXOverFreqDelay[0]); i++) {
        hEnvEnc->dynXOverFreqDelay[i] = hCmonData->dynXOverFreqEnc;
      }
      break;
    default:
      break;
  }

  if ((params->codecSettings.sampleFreq > 24000) &&
      ((params->srMode == DUAL_RATE) || (params->srMode == QUAD_RATE)))
    hCmonData->noHoles = 1;
  else
    hCmonData->noHoles = 0;

  hCmonData->sbrNumChannels = hEnvEnc->nChannelsSbr;
  hEnvEnc->sbrConfigData->dynXOverFreq = hCmonData->xOverFreq;

error_exit:
  if (errorInfo != noError) {
    if (hEnvEnc)
      EnvClose(hEnvEnc);
    hEnvEnc = NULL;
  }

  *hEnvEncoder = hEnvEnc;
  return errorInfo;
}

float SbrGetStopFreqRaw(HANDLE_SBR_ENCODER hEnv) {
  int stopFreq = hEnv->sbrHeaderData->sbr_stop_frequency;
  int fs = hEnv->sbrConfigData->sampleFreq;
  float stopFreqRaw = -1.f;

  if (stopFreq > 13) {
    int startFreq =
        GetSbrStartFreqRAW(hEnv->sbrHeaderData->sbr_start_frequency,
                           fs,
                           hEnv->sbrConfigData->coreCodec,
                           hEnv->sbrHeaderData->sampleRateMode);
    stopFreqRaw = (float)(stopFreq == 14 ? 2 * startFreq : 3 * startFreq);

  } else {
    stopFreqRaw = (float)GetSbrStopFreqRAW(stopFreq,
                                           (hEnv->sbrHeaderData->sampleRateMode == QUAD_RATE) ? fs / 2 : fs,
                                           hEnv->sbrConfigData->coreCodec,
                                           hEnv->sbrHeaderData->sampleRateMode);
  }

  return stopFreqRaw;
}

HANDLE_ERROR_INFO
SbrSetStopFreqRaw(HANDLE_SBR_ENCODER hEnv,
                  float *stopFreqRaw,
                  sbrConfigurationPtr params,
                  CODEC_TYPE coreCoder) {
  HANDLE_ERROR_INFO err = noError;

  int stopFreq = hEnv->sbrHeaderData->sbr_stop_frequency;
  int fs = hEnv->sbrConfigData->sampleFreq;
  int maxFreqCoeffs;
  int srMode = hEnv->sbrHeaderData->sampleRateMode;

  if (*stopFreqRaw != 0.0f) {
    int freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
    int diffFreq = 0;
    int diffFreqLast = abs(freqRaw - (int)(*stopFreqRaw));
    int startFreqRaw = GetSbrStartFreqRAW(hEnv->sbrHeaderData->sbr_start_frequency,
                                          fs,
                                          coreCoder,
                                          srMode);

    if (freqRaw <= *stopFreqRaw) {
      for (stopFreq = stopFreq + 1; stopFreq < 14; stopFreq++) {
        freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
        diffFreq = abs(freqRaw - (int)*stopFreqRaw);
        if (freqRaw >= *stopFreqRaw || stopFreq == 13)
          break;
        diffFreqLast = diffFreq;
      }
      if (diffFreqLast < diffFreq)
        stopFreq--;
      stopFreq = min(stopFreq, 13);
    } else {
      for (stopFreq = stopFreq - 1; stopFreq >= 0; stopFreq--) {
        freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
        diffFreq = abs(freqRaw - (int)*stopFreqRaw);
        if (freqRaw <= *stopFreqRaw || stopFreq == 0)
          break;
        diffFreqLast = diffFreq;
      }
      if (diffFreqLast < diffFreq)
        stopFreq++;
    }

    freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
    while (freqRaw < startFreqRaw + 3 * fs / (2 * SBR_QMF_CHANNELS)) {
      stopFreq++;
      freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
    }

    maxFreqCoeffs = MAX_FREQ_COEFFS;
    if (fs == 44100)
      maxFreqCoeffs = MAX_FREQ_COEFFS_FS44100;
    if (fs == 48000)
      maxFreqCoeffs = MAX_FREQ_COEFFS_FS48000;
    while (freqRaw > startFreqRaw + maxFreqCoeffs * fs / (2 * SBR_QMF_CHANNELS)) {
      stopFreq--;
      freqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
    }
  }

  if (hEnv->sbrHeaderData->sbr_stop_frequency != stopFreq) {
    int ch;

    hEnv->sbrHeaderData->sbr_stop_frequency = stopFreq;

    for (ch = 0; ch < hEnv->sbrConfigData->nChannels; ch++) {
      deleteEnvChannel(&hEnv->hEnvChannel[ch],
                       hEnv->sbrConfigData->coreCodec);

      err = updateFreqBandTable(hEnv->sbrConfigData,
                                hEnv->sbrHeaderData,
                                TRUE);

      if (err != noError)
        return handBack(err);

      err = createEnvChannel(hEnv->sbrConfigData,
                             hEnv->sbrHeaderData,
                             &hEnv->hEnvChannel[ch],
                             params,
                             coreCoder);

      if (err != noError)
        return handBack(err);
    }
  }

  *stopFreqRaw = (float)GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);

  return noError;
}

HANDLE_ERROR_INFO
SbrSetManualNoiseLevelOverride(HANDLE_SBR_ENCODER hEnvEncoder,
                               float noiseLevel) {
  if (noiseLevel < -1.0) {
    WARN("noiseLevel out of range");
    noiseLevel = -1.0;
  }
  if (noiseLevel > 30.0) {
    WARN("noiseLevel out of range");
    noiseLevel = 30.0;
  }

  hEnvEncoder->sbrConfigData->noiseLevel = noiseLevel;
  return noError;
}

int SbrGetMaxAllowedStopFreq(int startFreq, int fs, CODEC_TYPE coreCoder, SR_MODE srMode) {
  int stopFreq;
  int startFreqRaw, stopFreqRaw;
  int maxFreqCoeffs;
  int QMFbands = 64;

  startFreqRaw = GetSbrStartFreqRAW(startFreq,
                                    fs,
                                    coreCoder,
                                    srMode);

  maxFreqCoeffs = MAX_FREQ_COEFFS;
  if (fs == 44100)
    maxFreqCoeffs = MAX_FREQ_COEFFS_FS44100;
  if (fs == 48000)
    maxFreqCoeffs = MAX_FREQ_COEFFS_FS48000;

  stopFreq = 13;
  stopFreqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
  while (stopFreqRaw > startFreqRaw + maxFreqCoeffs * fs / (2 * QMFbands)) {
    stopFreq--;
    stopFreqRaw = GetSbrStopFreqRAW(stopFreq, fs, coreCoder, srMode);
  }

  return stopFreq;
}

int SbrStopFreqChanged(sbrConfigurationPtr params) {
  return params->stopFreqChanged;
}

unsigned int
SbrGetDownsampledOutSignal(HANDLE_SBR_ENCODER hEnv, float **samplesOut) {
  unsigned int nSamplesOut = 0;

  if ((samplesOut != NULL) && (hEnv != NULL)) {
    *samplesOut = NULL;

    if (hEnv->downsampledOutSignal) {
      *samplesOut = hEnv->downsampledOutSignal;
      nSamplesOut = (unsigned int)max(0, hEnv->nSamplesDownsampledOutSignal);
    }
  }

  return nSamplesOut;
}

int SbrGetDownsamplerDelay(HANDLE_SBR_ENCODER hEnv) {
  int nDelayDownsampler = 0;

  if (hEnv != NULL) {
    nDelayDownsampler = hEnv->nDelayDownsampler;
  }

  return nDelayDownsampler;
}

int SbrGetEstimateBitrate(sbrConfigurationPtr pSbrConfig) {
  int estimateBitrate = 0;

  if (pSbrConfig) {
    estimateBitrate += 2500 * pSbrConfig->codecSettings.nChannels;
  }

  return estimateBitrate;
}

int SbrGetSbrPresent(HANDLE_SBR_ENCODER hEnv) {
  int sbrIsPresent = (hEnv != NULL);

  return sbrIsPresent;
}

HANDLE_ERROR_INFO
SbrSetRightBorderFIX(HANDLE_SBR_ENCODER hEnv, int value) {
  HANDLE_ERROR_INFO error = noError;

  if (hEnv == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (error == noError) {
    hEnv->sbrBitstreamData->rightBorderFIX = value;
  }

  return error;
}

int SbrGetPsPresent(SBR_WITH_AAC HANDLE_SBR_ENCODER hEnv) {
  int psIsPresent = 0;
  (void)hEnv;

  return psIsPresent;
}

HANDLE_ERROR_INFO
SbrGetUsacDfltHeader(HANDLE_SBR_ENCODER hEnv, SBR_USAC_HEADER_DATA *pSbrUsacHeaderData) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hEnv == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (error == noError) {
    if (pSbrUsacHeaderData == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (error == noError) {
    pSbrUsacHeaderData->start_freq = hEnv->sbrHeaderData->sbr_start_frequency;
    pSbrUsacHeaderData->stop_freq = hEnv->sbrHeaderData->sbr_stop_frequency;
    pSbrUsacHeaderData->header_extra1 = hEnv->sbrHeaderData->header_extra_1;
    pSbrUsacHeaderData->header_extra2 = hEnv->sbrHeaderData->header_extra_2;
    pSbrUsacHeaderData->freq_scale = hEnv->sbrHeaderData->freqScale;
    pSbrUsacHeaderData->alter_scale = hEnv->sbrHeaderData->alterScale;
    pSbrUsacHeaderData->noise_bands = hEnv->sbrHeaderData->sbr_noise_bands;
    pSbrUsacHeaderData->limiter_bands = hEnv->sbrHeaderData->sbr_limiter_bands;
    pSbrUsacHeaderData->limiter_gains = hEnv->sbrHeaderData->sbr_limiter_gains;
    pSbrUsacHeaderData->interpol_freq = hEnv->sbrHeaderData->sbr_interpol_freq;
    pSbrUsacHeaderData->smoothing_mode = hEnv->sbrHeaderData->sbr_smoothing_length;
  }

  return error;
}

int SbrSetHeaderSendInterval(HANDLE_SBR_ENCODER hEnv,
                             sbrConfigurationPtr params) {
  if (hEnv == NULL)
    return -1;
  if (hEnv->hHdrTimer == NULL)
    return -1;
  if (params == NULL)
    return -1;

  return IntervalTimerInit(hEnv->hHdrTimer,
                           params->SendHeaderDataTime,
                           params->SendHeaderDataOffs,
                           params->codecSettings.sampleFreq,
                           params->codecSettings.frameSize);
}

int SbrSetCrcProtection(HANDLE_SBR_ENCODER hEnv,
                        sbrConfigurationPtr params) {
  if (hEnv == NULL)
    return -1;
  if (hEnv->hHdrTimer == NULL)
    return -1;
  if (params == NULL)
    return -1;

  hEnv->sbrBitstreamData->CRCActive = params->crcSbr;

  return 0;
}

int SbrSendSbrHeader(HANDLE_SBR_ENCODER hEnv) {
  if (hEnv == NULL)
    return -1;
  if (hEnv->hHdrTimer == NULL)
    return -1;

  return IntervalTimerTrigger(hEnv->hHdrTimer);
}

int SbrSAPFramePrepare(HANDLE_SBR_ENCODER hEnv) {
  if (hEnv == NULL)
    return -1;
  if (hEnv->sbrBitstreamData == NULL)
    return -1;

  hEnv->sbrBitstreamData->rightBorderFIX = 1;

  return 0;
}

int SbrSetTimeDiffCodingFlag(HANDLE_SBR_ENCODER hEnv, int flag) {
  if (hEnv == NULL)
    return -1;

  if (flag < 0 || flag > 1)
    return -1;

  hEnv->sbrConfigData->overrideTimeDiffCoding = flag;
  return flag;
}
