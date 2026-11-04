
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include "aacenc_internal.h"
#include "psy_data.h"
#include "tns.h"
#include "iisSigMap.h"
#include "iisutillib.h"
#include "psy_const.h"
#include "quantize.h"

typedef struct sf_estim_data SFESTIM_DATA;

enum {
  MS_NONE = 0,
  MS_SOME = 1,
  MS_ALL = 2
};

typedef enum {
  NF_UNDEFINED = -1,
  NF_OFF = 0,
  NF_MODE_1 = 1,
  NF_MODE_2 = 2,
  NF_STEREOFILLING_CPE = 3,
  NF_STEREOFILLING_MCT = 4,
  NF_EXACT_ENERGY_MCT = 5
} NOISEFILLING_MODE;

struct STEREO_INFO {
  int msDigest;
  int msMask[MAX_GROUPED_SFB];

  float umxMatRe[MAX_SFB][4];
  float umxMatIm[MAX_SFB][4];
  float qNoiseMatLr[MAX_SFB][4];
  float qNoiseMatMs[MAX_SFB][4];

  int bCplxPredMdct;
  int bCplxPredMdctActive;
  int bCplxPredMdctActivePrev;
  int bCplxPredMdctRealOnly;
  int bCplxPredMdctResetPredictors;
  int nGroupsPrev;
  int predCoefReQ[MAX_GROUPED_SFB];
  int predCoefImQ[MAX_GROUPED_SFB];
  int predCoefPrevReQ[MAX_GROUPED_SFB];
  int predCoefPrevImQ[MAX_GROUPED_SFB];
  float invPredGain[MAX_GROUPED_SFB];
  int windowSequencePrev;
  int sfbPerPredBand;
  int bSwap;
  float tnsGainHeadroomRatioPrev;
  int bPrevTnsActive;
  int bPrevTnsLeftRight;
  int bPrevFrame;
  MDCT_SPECTRUM mdctSpectrumLPrev;
  MDCT_SPECTRUM mdctSpectrumRPrev;
  MDCT_SPECTRUM mdctSpectrumDmx;
  MDCT_SPECTRUM mdctSpectrumDmxPrev;
  MDCT_SPECTRUM mdstSpectrumDmx;
  int bCplxPredMdctRealOnlyPrev;

  int bIntensityStereo;
  int bUniSte;
};

typedef struct {
  int sfbCnt;
  int sfbActive;
  int sfbPerGroup;
  int maxSfbPerGroup;
  int windowSequence;
  int windowShape;
  int prevWindowShape;
  int groupingMask;
  int noOfGroups;
  int groupLen[TRANS_FAC];
  int sfbOffsets[MAX_GROUPED_SFB + 1];
  float origTimeSig[2 * (FRAME_LEN_LONG)];
  ALIGN_16_BYTE float mdctSpectrum[FRAME_LEN_LONG];
  float sfbEnergy[MAX_GROUPED_SFB];
  float sfbThreshold[MAX_GROUPED_SFB];
  float sfbMinSnr[MAX_GROUPED_SFB];
  float sfbEnFac[MAX_GROUPED_SFB];
  float sfbEnSumMS;
  float sfbFormFactor[MAX_GROUPED_SFB];
  float sfbRelevLines[MAX_GROUPED_SFB];
  float chaosMeasure;
  TNS_INFO tnsInfo;
  NOISEFILLING_MODE noiseFillingMode;
  int noiseNrg[MAX_GROUPED_SFB];
  int isScale[MAX_GROUPED_SFB];
  int isBook[MAX_GROUPED_SFB];
  int granuleLength;

  int lastEnFacPatch;
  int wasVeryTonal;
  int isVeryTonal;
  AACENC_CODEC_TYPE codecType;

  QUANTIZER_DATA *quantizerData;
  SFESTIM_DATA *sfestimData;
} PSY_OUT_CHANNEL;

typedef struct {
  int commonWindow;
  float frameEnergyF;
  int blockType[2];
  struct STEREO_INFO toolsInfo;
} PSY_OUT_ELEMENT;

typedef struct {
  PSY_OUT_ELEMENT *psyOutElement[SIGMAP_MAX_ELEMENTS];

  PSY_OUT_CHANNEL *psyOutChannel[SIGMAP_MAX_SIGNALS];
} PSY_OUT;

void iisaacfenc_InitPsyOutElement(const int commonWindow,
                                  const int msDigest,
                                  PSY_OUT_ELEMENT *hPsyOutElement);

void iisaacfenc_InitPsyOutChannel(const int windowSequence,
                                  const int windowShape,
                                  const int noOfGroups,
                                  const int useCpuOptimization,
                                  const int sfbCnt,
                                  const int *sfbOffset,
                                  const int granuleLength,
                                  PSY_OUT_CHANNEL *psyOutCh);

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
                               PSY_OUT_CHANNEL *psyOutCh);

#endif
