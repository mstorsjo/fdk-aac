
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

#ifndef IISDRCGAINENC_UNIDRC_H
#define IISDRCGAINENC_UNIDRC_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum {
  IISDRCGAINENC_DELAY_MODE_REGULAR_DELAY = 0,
  IISDRCGAINENC_DELAY_MODE_LOW_DELAY = 1
} IISDRCGAINENC_DELAY_MODE;

typedef enum {
  IISDRCGAINENC_UNIDRCGAINEXT_TERM = 0x0
} IISDRCGAINENC_UNIDRCGAINEXT_TYPE;

typedef enum {
  IISDRCGAINENC_UNIDRCLOUDEXT_TERM = 0x0,
  IISDRCGAINENC_UNIDRCLOUDEXT_EQ = 0x1
} IISDRCGAINENC_UNIDRCLOUDEXT_TYPE;

typedef enum {
  IISDRCGAINENC_UNIDRCCONFEXT_TERM = 0x0,
  IISDRCGAINENC_UNIDRCCONFEXT_PARAM_DRC = 0x1,
  IISDRCGAINENC_UNIDRCCONFEXT_V1 = 0x2
} IISDRCGAINENC_UNIDRCCONFEXT_TYPE;

typedef enum {
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_UNKNOWN_OTHER = 0,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_PROGRAM_LOUDNESS,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_ANCHOR_LOUDNESS,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_LOUDNESS_RANGE,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MIXING_LEVEL,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_ROOM_TYPE,
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_SHORT_TERM_LOUDNESS
} IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION;

typedef enum {
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_UNKNOWN_OTHER = 0,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_EBU_R128,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1770_4_PREPROCESS,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_USER,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_EXPERT,
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM_BS1771_1
} IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM;

typedef enum {
  IISDRCGAINENC_LOUDNESS_RELIABILITY_UNKNOWN = 0,
  IISDRCGAINENC_LOUDNESS_RELIABILITY_UNVERIFIED,
  IISDRCGAINENC_LOUDNESS_RELIABILITY_NOT_TO_EXCEED_CEILING,
  IISDRCGAINENC_LOUDNESS_RELIABILITY_MEASURED_AND_ACCURATE
} IISDRCGAINENC_LOUDNESS_RELIABILITY;

typedef enum {
  IISDRCGAINENC_DRCSETEFFECT_BIT_NONE = (-1),
  IISDRCGAINENC_DRCSETEFFECT_BIT_NIGHT = 0x0001,
  IISDRCGAINENC_DRCSETEFFECT_BIT_NOISY = 0x0002,
  IISDRCGAINENC_DRCSETEFFECT_BIT_LIMITED = 0x0004,
  IISDRCGAINENC_DRCSETEFFECT_BIT_LOWLEVEL = 0x0008,
  IISDRCGAINENC_DRCSETEFFECT_BIT_DIALOG = 0x0010,
  IISDRCGAINENC_DRCSETEFFECT_BIT_GENERAL_COMPR = 0x0020,
  IISDRCGAINENC_DRCSETEFFECT_BIT_EXPAND = 0x0040,
  IISDRCGAINENC_DRCSETEFFECT_BIT_ARTISTIC = 0x0080,
  IISDRCGAINENC_DRCSETEFFECT_BIT_CLIPPING = 0x0100,
  IISDRCGAINENC_DRCSETEFFECT_BIT_FADE = 0x0200,
  IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER = 0x0400,
  IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF = 0x0800
} IISDRCGAINENC_DRCSETEFFECT_BIT;

typedef enum {
  IISDRCGAINENC_GAINCODINGPROFILE_REGULAR = 0,
  IISDRCGAINENC_GAINCODINGPROFILE_FADING = 1,
  IISDRCGAINENC_GAINCODINGPROFILE_DUCKING_CLIPPING = 2,
  IISDRCGAINENC_GAINCODINGPROFILE_CONSTANT = 3
} IISDRCGAINENC_GAINCODINGPROFILE;

typedef enum {
  IISDRCGAINENC_GAININTERPOLATIONTYPE_SPLINE = 0,
  IISDRCGAINENC_GAININTERPOLATIONTYPE_LINEAR = 1
} IISDRCGAINENC_GAININTERPOLATIONTYPE;

typedef struct {
  int baseChannelCount;
  int layoutSignalingPresent;
  int definedLayout;
  int *pSpeakerPosition;
} ChannelLayout;

typedef struct {
  int downmixId;
  int targetChannelCount;
  int targetLayout;
  int downmixCoefficientsPresent;
  float *pDownmixCoefficient;
  int *pLfeChannel;
} DownmixInstructions;

typedef struct {
  int gainSequenceIndex;
  int drcCharacteristicPresent;
  int drcCharacteristicFormatIsCICP;
  int drcCharacteristic;
  int drcCharacteristicLeftIndex;
  int drcCharacteristicRightIndex;
  int crossoverFreqIndex;
  int startSubBandIndex;
} GainSequenceParams;

typedef struct {
  IISDRCGAINENC_GAINCODINGPROFILE gainCodingProfile;
  IISDRCGAINENC_GAININTERPOLATIONTYPE gainInterpolationType;
  int fullFrame;
  int timeAlignment;
  int timeDeltaMinPresent;
  int timeDeltaMin;
  int bandCount;
  int drcBandType;
  GainSequenceParams *pGainSequenceParams;
} GainSetParams;

typedef struct {
  int characteristicFormat;

  int gainDb;
  int ioRatio;
  int exp;
  int flipSign;

  int charNodeCount;
  float *pNodeLevel;
  float *pNodeGain;
} CustomDrcCharacteristic;

typedef struct {
  int cornerFreqIndex;
  int filterStrengthIndex;
} ShapeFilterSingleParams;

typedef struct {
  int lfCutFilterPresent;
  ShapeFilterSingleParams lfCutParams;
  int lfBoostFilterPresent;
  ShapeFilterSingleParams lfBoostParams;
  int hfCutFilterPresent;
  ShapeFilterSingleParams hfCutParams;
  int hfBoostFilterPresent;
  ShapeFilterSingleParams hfBoostParams;
} ShapeFilterParams;

typedef struct {
  int drcLocation;
  int drcCharacteristic;
} DrcCoefficientsBasic;

typedef struct {
  int drcLocation;
  int drcFrameSizePresent;
  int drcFrameSize;
  int drcCharacteristicLeftPresent;
  int drcCharacteristicLeftCount;
  CustomDrcCharacteristic *pCustomDrcCharacteristicLeft;
  int drcCharacteristicRightPresent;
  int drcCharacteristicRightCount;
  CustomDrcCharacteristic *pCustomDrcCharacteristicRight;
  int shapeFiltersPresent;
  int shapeFilterCount;
  ShapeFilterParams *pShapeFilterParams;
  int gainSequenceCount;
  int gainSetCount;
  GainSetParams *pGainSetParams;
} DrcCoefficientsUniDrc;

typedef struct {
  int duckingScalingPresent;
  float duckingScaling;
} DuckingModifications;

typedef struct {
  int *pTargetCharacteristicLeftPresent;
  int *pTargetCharacteristicLeftIndex;
  int *pTargetCharacteristicRightPresent;
  int *pTargetCharacteristicRightIndex;
  int shapeFilterPresent;
  int shapeFilterIndex;
  int *pGainScalingPresent;
  float *pAttenuationScaling;
  float *pAmplificationScaling;
  int *pGainOffsetPresent;
  float *pGainOffset;
} GainModifications;

typedef struct {
  int drcSetId;
  int drcLocation;
  int downmixId;
  int additionalDownmixIdPresent;
  int additionalDownmixIdCount;
  int *pAdditionalDownmixId;
  int drcSetEffect;
  int limiterPeakTargetPresent;
  float limiterPeakTarget;
  int drcSetTargetLoudnessPresent;
  int drcSetTargetLoudnessValueUpper;
  int drcSetTargetLoudnessValueLowerPresent;
  int drcSetTargetLoudnessValueLower;
} DrcInstructionsBasic;

typedef struct {
  int drcSetId;
  int drcSetComplexityLevel;
  int drcApplyToDownmix;
  int requiresEq;
  int downmixIdPresent;
  int drcLocation;
  int downmixId;
  int additionalDownmixIdPresent;
  int additionalDownmixIdCount;
  int *pAdditionalDownmixId;
  int drcSetEffect;
  int limiterPeakTargetPresent;
  float limiterPeakTarget;
  int drcSetTargetLoudnessPresent;
  int drcSetTargetLoudnessValueUpper;
  int drcSetTargetLoudnessValueLowerPresent;
  int drcSetTargetLoudnessValueLower;
  int dependsOnDrcSetPresent;
  int dependsOnDrcSet;
  int noIndependentUse;
  int *pGainSetIndex;
  GainModifications *pGainModifications;
  DuckingModifications *pDuckingModifications;

  int drcChannelCount;
  int drcChannelGroupCount;
  int *pBandCountForChannelGroup;
} DrcInstructionsUniDrc;

typedef struct {
  IISDRCGAINENC_UNIDRCCONFEXT_TYPE *pUniDrcConfigExtType;
  int *pExtBitSize;

  int downmixInstructionsV1Present;
  int downmixInstructionsV1Count;
  DownmixInstructions *pDownmixInstructionsV1;
  int drcCoeffsAndInstructionsUniDrcV1Present;
  int drcCoefficientsUniDrcV1Count;
  DrcCoefficientsUniDrc *pDrcCoefficientsUniDrcV1;
  int drcInstructionsUniDrcV1Count;
  DrcInstructionsUniDrc *pDrcInstructionsUniDrcV1;
  int loudEqInstructionsPresent;
  int loudEqInstructionsCount;
  int numUniDrcConfigExtensions;
} UniDrcConfigExtension;

typedef struct {
  int sampleRatePresent;
  int sampleRate;
  int downmixInstructionsCount;
  int drcDescriptionBasicPresent;
  int drcCoefficientsBasicCount;
  int drcInstructionsBasicCount;
  int drcCoefficientsUniDrcCount;
  int drcInstructionsUniDrcCount;
  int uniDrcConfigExtPresent;
  DownmixInstructions *pDownmixInstructions;
  DrcCoefficientsBasic *pDrcCoefficientsBasic;
  DrcInstructionsBasic *pDrcInstructionsBasic;
  DrcCoefficientsUniDrc *pDrcCoefficientsUniDrc;
  DrcInstructionsUniDrc *pDrcInstructionsUniDrc;
  ChannelLayout channelLayout;
  UniDrcConfigExtension uniDrcConfigExtension;
} UniDrcConfig;

typedef struct {
  IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION methodDefinition;
  float methodValue;
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM measurementSystem;
  IISDRCGAINENC_LOUDNESS_RELIABILITY reliability;
} LoudnessMeasure;

typedef struct {
  float truePeakLevel;
  IISDRCGAINENC_LOUDNESS_MEASUREMENT_SYSTEM truePeakLevelMeasurementSystem;
  IISDRCGAINENC_LOUDNESS_RELIABILITY truePeakLevelReliability;
} TruePeakMeasure;

typedef struct {
  int drcSetId;
  int eqSetId;
  int downmixId;
  int samplePeakLevelPresent;
  float samplePeakLevel;
  int truePeakLevelPresent;
  TruePeakMeasure truePeakMeasure;
  int measurementCount;
  LoudnessMeasure *pLoudnessMeasure;
} LoudnessInfo;

typedef struct {
  int loudnessInfoV1AlbumCount;
  int loudnessInfoV1Count;
  LoudnessInfo *pLoudnessInfoV1Album;
  LoudnessInfo *pLoudnessInfoV1;
} LoudnessInfoSetExtEq;

typedef struct {
  IISDRCGAINENC_UNIDRCLOUDEXT_TYPE *pLoudnessInfoSetExtType;
  int *pExtBitSize;
  LoudnessInfoSetExtEq loudnessInfoSetExtEq;
  int numLoudnessExtensions;
} LoudnessInfoSetExtension;

typedef struct {
  int loudnessInfoAlbumCount;
  int loudnessInfoCount;
  int loudnessInfoSetExtPresent;
  LoudnessInfo *pLoudnessInfoAlbum;
  LoudnessInfo *pLoudnessInfo;
  LoudnessInfoSetExtension loudnessInfoSetExtension;
} LoudnessInfoSet;

#if defined __cplusplus
}
#endif
#endif
