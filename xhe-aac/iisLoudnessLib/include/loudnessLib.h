
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

#ifndef LOUDNESSLIB_H
#define LOUDNESSLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

struct LOUDNESS_METER;
typedef struct LOUDNESS_METER *HLOUDNESS_METER;

typedef struct LOUDMTR_LIBINFO {
  const char *version;
  const char *info;
  const char *copyright;
  const char *comment;
} LOUDMTR_LIBINFO;

typedef enum {
  LS_POS_LFE = 0,
  LS_POS_M_FRONT,
  LS_POS_M_SIDE,
  LS_POS_M_BACK,
  LS_POS_U_B_ALL
} LOUDMTR_LS_POS;

typedef enum LOUDMTR_ERROR {
  ERRLOUD_Success = 0,
  ERRLOUD_InvalidArg,
  ERRLOUD_OutOfMemory,
  ERRLOUD_NullPointer
} LOUDMTR_ERROR;

typedef enum LOUDMTR_TPFILTER {
  LOUDMTR_TPFILTER_IIS = 0,
  LOUDMTR_TPFILTER_ITU = 1
} LOUDMTR_TPFILTER;

typedef enum LOUDMTR_WORKLOAD {
  LOUDMTR_WORKLOAD_ENABLE_ALL = 0,
  LOUDMTR_WORKLOAD_DISABLE_LOUDNESS = 1,
  LOUDMTR_WORKLOAD_DISABLE_SP = 2,
  LOUDMTR_WORKLOAD_DISABLE_TP = 4
} LOUDMTR_WORKLOAD;

LOUDMTR_ERROR LoudMeter_Create(HLOUDNESS_METER *phLoudnessMeter);

LOUDMTR_ERROR LoudMeter_Destroy(HLOUDNESS_METER *phLoudnessMeter);

LOUDMTR_ERROR LoudMeter_Config(HLOUDNESS_METER hLoudnessMeter,
                               LOUDMTR_LS_POS *channelConfig,
                               int *channelGroup,
                               int nCh,
                               unsigned int nSamplesPerSec,
                               LOUDMTR_TPFILTER filterConfig);

LOUDMTR_ERROR LoudMeter_SetWorkloadConfig(HLOUDNESS_METER hLoudnessMeter,
                                          LOUDMTR_WORKLOAD workloadConfig);

LOUDMTR_ERROR LoudMeter_Reset(HLOUDNESS_METER hLoudnessMeter);

LOUDMTR_ERROR LoudMeter_Feed(HLOUDNESS_METER hLoudnessMeter,
                             const float *pAudio,
                             unsigned int nSamplesPerCh);

float LoudMeter_GetMomentaryLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetMomentaryKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetShortTermLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetShortTermKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetUngatedLongTermLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetGatedLongTermLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetLongTermGatingThreshold(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetInstantaneousLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetInstantaneousKWeightedEnergy(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetLoudnessRange(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetLoudnessRange2(HLOUDNESS_METER hLoudnessMeter,
                                  float *pLow,
                                  float *pHigh);

float LoudMeter_GetLRAGatingThreshold(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetMaxTruePeak(HLOUDNESS_METER hLoudnessMeter,
                               int channel);

float LoudMeter_GetMomentaryTruePeak(HLOUDNESS_METER hLoudnessMeter,
                                     int channel);

float LoudMeter_GetShortTermTruePeak(HLOUDNESS_METER hLoudnessMeter,
                                     int channel);

float LoudMeter_GetMaxSamplePeak(HLOUDNESS_METER hLoudnessMeter,
                                 int channel);

float LoudMeter_GetMomentarySamplePeak(HLOUDNESS_METER hLoudnessMeter,
                                       int channel);

float LoudMeter_GetShortTermSamplePeak(HLOUDNESS_METER hLoudnessMeter,
                                       int channel);

float LoudMeter_GetMaxMomentaryLoudness(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetMaxShortTermLoudness(HLOUDNESS_METER hLoudnessMeter);

unsigned long long LoudMeter_GetMomentaryHistogram(HLOUDNESS_METER hLoudnessMeter,
                                                   unsigned long long *hist,
                                                   int minLUFS,
                                                   int maxLUFS);

unsigned long long LoudMeter_GetShortTermHistogram(HLOUDNESS_METER hLoudnessMeter,
                                                   unsigned long long *hist,
                                                   int minLUFS,
                                                   int maxLUFS);

LOUDMTR_ERROR LoudMeter_InitGatedMidTermLoudness(HLOUDNESS_METER hLoudnessMeter, unsigned int length);

float LoudMeter_GetGatedMidTermLoudness(HLOUDNESS_METER hLoudnessMeter);

unsigned int LoudMeter_GetGatedMidTermLoudnessCount(HLOUDNESS_METER hLoudnessMeter);

LOUDMTR_ERROR LoudMeter_InitMidTermLoudnessRange(HLOUDNESS_METER hLoudnessMeter, unsigned int length);

float LoudMeter_GetMidTermLoudnessRange(HLOUDNESS_METER hLoudnessMeter);

float LoudMeter_GetMidTermLoudnessRange2(HLOUDNESS_METER hLoudnessMeter,
                                         float *pLow,
                                         float *pHigh);

unsigned int LoudMeter_GetMidTermLoudnessRangeCount(HLOUDNESS_METER hLoudnessMeter);

const LOUDMTR_LIBINFO *LoudMeter_GetLibraryInfo(void);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#endif
