
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

#ifndef IISDRCGAINGENERATOR_H
#define IISDRCGAINGENERATOR_H

#include <string.h>
#include <stdio.h>
#include "iisDRCGainGenerator_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN
#define MIN(x1, x2) ((x1) < (x2) ? (x1) : (x2))
#endif
#ifndef MAX
#define MAX(x1, x2) ((x1) > (x2) ? (x1) : (x2))
#endif

#define DRCCHARINDEXUPPER (255)
#define MAXDRCCURVENODES (9)

#define MAXNUMLEVELELSTIMINSTANCES MAXNUMGAINBANDSEQUENCES

typedef struct T_DRCGAINGEN_EXTPARAMS {
  float nodeInputLevel[MAXDRCCURVENODES];
  float nodeOutputGain[MAXDRCCURVENODES];
  int nodeCount;
  float fastAttack;
  float fastDecay;
  float slowAttack;
  float slowDecay;
  float holdOff;
  float attackThr;
  float decayThr;
  int parametricDrcId;
  int gainBandSequenceIndex;
} IisDRCGainGenExtParams;

typedef struct T_DRCGAINGENPARAMS_LEVELCALCBS1770FILTER {
  int frameSize;
  int numChannels;
  int sequenceLookaheadSamples;
  int sequenceHoldLookaheadSamples;
  int skipPreFilter;
  int applyExternalDrcGains;
  float pre_b0;
  float pre_b1;
  float pre_b2;
  float pre_a1;
  float pre_a2;
  float rlb_b0;
  float rlb_b1;
  float rlb_b2;
  float rlb_a1;
  float rlb_a2;
} IisDRCGainGenLevelCalcParams_BS1770Filter;

typedef struct {
  float int_b0;
  float int_a1;
  float levelThr;
  float PRL;
  float nullBandReferenceLevel;
} IisDRCGainGenLevelCalcParams_BS1770Int;

typedef struct {
  float nodeInputLevel[MAXDRCCURVENODES];
  float nodeOutputGain[MAXDRCCURVENODES];
  float segmentSlope[MAXDRCCURVENODES - 1];
  float segmentOffset[MAXDRCCURVENODES - 1];
  int drcCurveNodeCount;
  int holdOff;
  float fastAttack;
  float fastDecay;
  float slowAttack;
  float slowDecay;
  float attackThr;
  float decayThr;
} DrcGainGenCompressorParams;

typedef struct T_DRCGAINGEN_PARAMSINSTANCE {
  DRC_CHARACTERISTIC_INDEX drcCharacteristicIndex;
  DRC_LEVELCALC_MODE drcLevelCalculationMode;
  int frameSize;
  int fs;
  int numChannels;
  float* channelWeight;
  int sideChainSignalPresent;
  float gainOffsetSequence;
  int levelEstimInstanceToApply;
  int applyExternalDrcGains;
  IisDRCGainGenLevelCalcParams_BS1770Int iisDRCGainGenLevelCalcParams_BS1770Int;
  DrcGainGenCompressorParams drcGainGenCompressorParams;
  int sequenceHoldLookaheadSamples;
} IisDRCGainGenParamsInstance;

typedef struct T_DRCGAINGEN_PARAMS {
  int numGainBandSequences;
  int baseChannelCount;
  int maxChannelCount;
  int frameSize;
  int maxLookaheadSamples;
  IisDRCGainGenParamsInstance* iisDRCGainGenParamsInstance;
  IisDRCGainGenLevelCalcParams_BS1770Filter* iisDRCGainGenLevelCalcParams_BS1770Filter;
  IisDRCGainGenExtParams* iisDRCGainGenExtParams;
  int numExtParams;
  int numExtParamsDefined;
  int numLevelEstimInstances;
} IisDRCGainGenParams;

typedef struct T_DRCGAINGENSTATE_LEVELCALCBS1770FILTER {
  float* pre_x1;
  float* pre_x2;
  float* pre_y1;
  float* pre_y2;
  float* rlb_x1;
  float* rlb_x2;
  float* rlb_y1;
  float* rlb_y2;
  float** xSquare;
  float* externalGain;
  int bFirstFrameFiltering;
} IisDRCGainGenLevelCalcStates_BS1770Filter;

typedef struct {
  float* int_y1;
  int bFirstFrameIntegration;
  float* ySum;
} IisDRCGainGenLevelCalcStates_BS1770Int;

typedef struct {
  float smoothGain;
  float smoothLevel;
  int holdCount;
  int bFirstFrameCompressor;
  int bFirstFrameSmoothing;
} DrcGainGenCompressorStates;

typedef struct T_DRCGAINGEN_STATESINSTANCE {
  float* levelDb;
  IisDRCGainGenLevelCalcStates_BS1770Int iisDRCGainGenLevelCalcStates_BS1770Int;
  DrcGainGenCompressorStates drcGainGenCompressorStates;
} IisDRCGainGenStatesInstance;

typedef struct T_DRCGAINGEN_STATES {
  float** audioBufferApplyDelayPointer;
  float** audioWithGainApplied;
  IisDRCGainGenStatesInstance* iisDRCGainGenStatesInstance;
  IisDRCGainGenLevelCalcStates_BS1770Filter* iisDRCGainGenLevelCalcStates_BS1770Filter;
  float** audioInWithLookahead;
} IisDRCGainGenStates;

int drcGainGeneratorProcessInstance(float** audioIn,
                                    float* drcGain,
                                    IisDRCGainGenStatesInstance* iisDRCGainGenStatesInstance,
                                    IisDRCGainGenParamsInstance iisDRCGainGenParamsInstance);

#ifdef __cplusplus
}
#endif
#endif
