
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

#ifndef spaceEnclib_h_
#define spaceEnclib_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "iisutillib.h"

#define MP4SPACEENC_RES_MAX_CHANNELS 8

#define MAX_NUM_ARBDMX_RES_ELEMS 1

#define MP4SPACEENC_RES_MODE_BOX_0 0x01
#define MP4SPACEENC_RES_MODE_BOX_1 0x02
#define MP4SPACEENC_RES_MODE_BOX_2 0x04
#define MP4SPACEENC_RES_MODE_BOX_3 0x08
#define MP4SPACEENC_RES_MODE_BOX_4 0x10
#define MP4SPACEENC_RES_MODE_BOX_5 0x20
#define MP4SPACEENC_RES_MODE_BOX_6 0x40
#define MP4SPACEENC_RES_MODE_BOX_7 0x80

#define MP4SPACEENC_RES_MODE_NONE 0

#define MP4SPACEENC_RES_MODE_USAC MP4SPACEENC_RES_MODE_BOX_0

#define MP4SPACEENC_RES_MODE_5151 (MP4SPACEENC_RES_MODE_BOX_0 | \
                                   MP4SPACEENC_RES_MODE_BOX_1 | \
                                   MP4SPACEENC_RES_MODE_BOX_3)

#define MP4SPACEENC_RES_MODE_5152 (MP4SPACEENC_RES_MODE_BOX_0 | \
                                   MP4SPACEENC_RES_MODE_BOX_1)

#define MP4SPACEENC_RES_MODE_525 (MP4SPACEENC_RES_MODE_BOX_1 | \
                                  MP4SPACEENC_RES_MODE_BOX_2 | \
                                  MP4SPACEENC_RES_MODE_BOX_3)

#define MP4SPACEENC_RES_MODE_7271 (MP4SPACEENC_RES_MODE_BOX_1 | \
                                   MP4SPACEENC_RES_MODE_BOX_2 | \
                                   MP4SPACEENC_RES_MODE_BOX_3 | \
                                   MP4SPACEENC_RES_MODE_BOX_4 | \
                                   MP4SPACEENC_RES_MODE_BOX_5)

#define MP4SPACEENC_RES_MODE_7272 (MP4SPACEENC_RES_MODE_BOX_1 | \
                                   MP4SPACEENC_RES_MODE_BOX_2 | \
                                   MP4SPACEENC_RES_MODE_BOX_3 | \
                                   MP4SPACEENC_RES_MODE_BOX_4 | \
                                   MP4SPACEENC_RES_MODE_BOX_5)

#define MP4SPACEENC_RES_MODE_7571 (MP4SPACEENC_RES_MODE_BOX_0 | \
                                   MP4SPACEENC_RES_MODE_BOX_1)

#define MP4SPACEENC_RES_MODE_7572 (MP4SPACEENC_RES_MODE_BOX_0 | \
                                   MP4SPACEENC_RES_MODE_BOX_1)

#define MP4SPACEENC_RES_MODE_929 (MP4SPACEENC_RES_MODE_BOX_1 | \
                                  MP4SPACEENC_RES_MODE_BOX_2 | \
                                  MP4SPACEENC_RES_MODE_BOX_7)

#define MP4SPACEENC_RES_MODE_959 (MP4SPACEENC_RES_MODE_BOX_0 | \
                                  MP4SPACEENC_RES_MODE_BOX_1 | \
                                  MP4SPACEENC_RES_MODE_BOX_2 | \
                                  MP4SPACEENC_RES_MODE_BOX_3)

#ifndef RESIDUAL_CODING_DELAY
#define RESIDUAL_CODING_DELAY 2
#endif

typedef enum {
  MP4SPACEENC_ENCODERTYPE_INVALID = -1,
  MP4SPACEENC_ENCODERTYPE_CLASSICMPS = 0,
  MP4SPACEENC_ENCODERTYPE_LOW_DELAY = 1,
  MP4SPACEENC_ENCODERTYPE_USAC = 2
} MP4SPACEENC_ENCODERTYPE;

typedef struct {
  int nSscSizeBits;
  unsigned char *pSsc;
} MPEG4SPACEENC_SSCBUF;

typedef struct {
  int nSampleRate;
  int nSamplesFrame;
  int nDmxDelay;
  int nCodecDelay;
  int nPayloadDelay;

  MPEG4SPACEENC_SSCBUF *pSscBuf;

  char pVersion[50];
} MP4SPACEENC_INFO;

typedef enum {
  MP4SPACEENC_INVALID_MODE = 0,
  MP4SPACEENC_ESCAPE = 15,
  MP4SPACEENC_USAC_212 = 16

} MP4SPACEENC_MODE;

typedef enum {
  MP4SPACEENC_BANDS_INVALID = 0,
  MP4SPACEENC_BANDS_4 = 4,
  MP4SPACEENC_BANDS_5 = 5,
  MP4SPACEENC_BANDS_7 = 7,
  MP4SPACEENC_BANDS_9 = 9,
  MP4SPACEENC_BANDS_10 = 10,
  MP4SPACEENC_BANDS_12 = 12,
  MP4SPACEENC_BANDS_14 = 14,
  MP4SPACEENC_BANDS_15 = 15,
  MP4SPACEENC_BANDS_20 = 20,
  MP4SPACEENC_BANDS_23 = 23,
  MP4SPACEENC_BANDS_28 = 28
} MP4SPACEENC_BANDS_CONFIG;

typedef enum {
  MP4SPACEENC_QUANTMODE_INVALID = -1,
  MP4SPACEENC_QUANTMODE_FINE = 0,
  MP4SPACEENC_QUANTMODE_EBQ1 = 1,
  MP4SPACEENC_QUANTMODE_EBQ2 = 2,
  MP4SPACEENC_QUANTMODE_RSVD3 = 3

} MP4SPACEENC_QUANTMODE;

typedef enum {

  MP4SPACEENC_SMOOTHCONFIG_INVALID = -1,
  MP4SPACEENC_SMOOTHCONFIG_AUTOOFF = 0,
  MP4SPACEENC_SMOOTHCONFIG_AUTOON = 1
} MP4SPACEENC_SMOOTHCONFIG;

typedef enum {
  MP4SPACEENC_TEMPSHAPE_INVALID = -1,
  MP4SPACEENC_TEMPSHAPE_OFF = 0,
  MP4SPACEENC_TEMPSHAPE_STP = 1,
  MP4SPACEENC_TEMPSHAPE_GES = 2,
  MP4SPACEENC_TEMPSHAPE_TSD = 3
} MP4SPACEENC_TEMPSHAPECONFIG;

typedef enum {
  MP4SPACEENC_DOWNMIXTYPE_INVALID = -1,
  MP4SPACEENC_DOWNMIXTYPE_CLASSICMPS = 0,
  MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED = 3,
  MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL = 4
} MP4SPACEENC_DOWNMIXTYPE;

typedef enum {
  MP4SPACEENC_IPDMODE_INVALID = -1,
  MP4SPACEENC_IPDMODE_NONE = 0,
  MP4SPACEENC_IPDMODE_NO_RESIDUAL = 1,
  MP4SPACEENC_IPDMODE_RESIDUAL = 2
} MP4SPACEENC_IPDMODE;

typedef enum {
  MP4SPACEENC_IPDQUANTMODE_INVALID = -1,
  MP4SPACEENC_IPDQUANTMODE_FINE = 0,
  MP4SPACEENC_IPDQUANTMODE_COARSE = 1
} MP4SPACEENC_IPDQUANTMODE;

typedef enum {
  MP4SPACEENC_RESTRICT_INVALID = -1,
  MP4SPACEENC_RESTRICT_OFF = 0,
  MP4SPACEENC_RESTRICT_LIGHT = 1,
  MP4SPACEENC_RESTRICT_MEDIUM = 2,
  MP4SPACEENC_RESTRICT_MAX = 3
} MP4SPACEENC_RESTRICT_RESIDUAL_FRAME;

typedef enum {
  MP4SPACEENC_RES_CODEC_INVALID = 0,
  MP4SPACEENC_RES_CODEC_AAC = 1,
  MP4SPACEENC_RES_CODEC_PCM = 2
} MP4SPACEENC_RES_CODEC;

typedef struct {
  unsigned int mode;
  MP4SPACEENC_RES_CODEC codec;
  unsigned int bands[MP4SPACEENC_RES_MAX_CHANNELS];
  unsigned int bitRate[MP4SPACEENC_RES_MAX_CHANNELS];
  unsigned int framesPerSpatial;
  MP4SPACEENC_RESTRICT_RESIDUAL_FRAME bRestrictFramesize;
} MP4SPACEENC_RES_CONFIG;

typedef enum {
  MP4SPACEENC_USAC212_LOW,
  MP4SPACEENC_USAC212_HIGH
} MP4SPACEENC_USAC212MODE;

typedef struct {
  int bsFreqRes;
  int bsFixedGainDMX;
  int bsTempShapeConfig;
  int bsDecorrConfig;
  int bsHighRateMode;
  int bsPhaseCoding;
  int bsOttBandsPhasePresent;
  int bsOttBandsPhase;
  int bsResidualBands;
  int bsPseudoLr;
  int bsEnvQuantMode;

} MP4SPACEENC_USAC_MPS212_CONFIG;

typedef enum {
  MP4SPACEENC_SURGAIN_INVALID = -1,
  MP4SPACEENC_SURGAIN_0_dB = 0,
  MP4SPACEENC_SURGAIN_1_5_dB = 1,
  MP4SPACEENC_SURGAIN_3_dB = 2,
  MP4SPACEENC_SURGAIN_4_5_dB = 3,
  MP4SPACEENC_SURGAIN_6_dB = 4
} MP4SPACEENC_SURROUND_GAIN;

typedef enum {
  MP4SPACEENC_DMXGAIN_INVALID = -1,
  MP4SPACEENC_DMXGAIN_0_dB = 0,
  MP4SPACEENC_DMXGAIN_1_5_dB = 1,
  MP4SPACEENC_DMXGAIN_3_dB = 2,
  MP4SPACEENC_DMXGAIN_4_5_dB = 3,
  MP4SPACEENC_DMXGAIN_6_dB = 4,
  MP4SPACEENC_DMXGAIN_7_5_dB = 5,
  MP4SPACEENC_DMXGAIN_9_dB = 6,
  MP4SPACEENC_DMXGAIN_12_dB = 7
} MP4SPACEENC_DMX_GAIN;

typedef struct {
  unsigned int numElems;
  unsigned int bands;
  unsigned int bitRate[MAX_NUM_ARBDMX_RES_ELEMS];
  unsigned int framesPerSpatial;
} MP4SPACEENC_ARB_DMX_RES_CONFIG;

typedef struct {
  MP4SPACEENC_MODE encMode;
  MP4SPACEENC_BANDS_CONFIG nParamBands;
  MP4SPACEENC_QUANTMODE quantMode;
  MP4SPACEENC_SMOOTHCONFIG smoothConfig;
  MP4SPACEENC_TEMPSHAPECONFIG tempShapeConfig;
  unsigned int bUseCoarseQuant;
  unsigned int bUseCoarseQuantArbDmx;
  unsigned int bUseOneIcc;
  int bCalcDPL;
  int bApplyLfeFilter;
  int bPhaseAlignLfe;
  int bDMXAlign;
  int bArbitraryDMX;
  int bLdMode;
  int bTimeDomainDmx;
  int decorrConfig;
  int bitrateControlMode;

  unsigned int sampleRate;
  unsigned int frameTimeSlots;
  float cpcStopFrequency;
  unsigned int independencyFactor;

  int timeAlignment;
  MP4SPACEENC_RES_CONFIG residualConfig;
  MP4SPACEENC_ARB_DMX_RES_CONFIG arbDmxResidualConfig;

  MP4SPACEENC_USAC212MODE bsHighRateMode;
  MP4SPACEENC_DOWNMIXTYPE downmixType;
  MP4SPACEENC_IPDMODE ipdMode;
  unsigned int bUseCoarseQuantIPD;
  unsigned int bIpdRelevancy;
  unsigned int bOpdSmoothing;
  unsigned int bStereoSbr;
  unsigned int bPseudoLr;
  unsigned int bPsStyleDmx;

} MP4SPACEENC_SETUP, *HANDLE_MP4SPACEENC_SETUP;

typedef struct MP4SPACE_ENCODER *HANDLE_MP4SPACE_ENCODER;

HANDLE_ERROR_INFO
mp4SpaceEnc_GetFrameLen(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, int *nFrameLength);

HANDLE_ERROR_INFO
mp4SpaceEnc_SetIndependencyCount(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, int value);

int mp4SpaceEnc_GetIndependencyFlag(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, unsigned char *pBuf);

HANDLE_ERROR_INFO
mp4SpaceEnc_Open(
    HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc,
    HANDLE_MP4SPACEENC_SETUP const hSetup,
    int const bQmfOutput,
    const MP4SPACEENC_ENCODERTYPE encoderType);

HANDLE_ERROR_INFO
mp4SpaceEnc_Init(
    HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc,
    unsigned int *const pSamplesFirst,
    int dmxDelay,
    unsigned int *const pDiscardOutFrames,
    float aacCoreBandwidth);

HANDLE_ERROR_INFO
mp4SpaceEnc_Encode(
    HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
    float const *const pInputSamples,
    unsigned int const nInputSamples,
    unsigned int *const pSamplesConsumed,
    unsigned int *const pSamplesNext,
    float *const pOutputSamples,
    float **const ppQmfOutputSamplesReal,
    float **const ppQmfOutputSamplesImag,
    unsigned int *const pnOutputSamples,
    const unsigned int nOutputSamplesBufferSize,
    unsigned char *const pOutputBuffer,
    const int nOutputBufferSize,
    int *const pOutBits,
    int bUsacIndependencyFlag);

HANDLE_ERROR_INFO
mp4SpaceEnc_UniSteUpdateFrame(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
                              float umxMatRe[][28][4],
                              float umxMatIm[][28][4],
                              float cld[][28],
                              int delayUmxMat2Mdct,
                              int bPseudoLr);

int mp4SpaceEnc_GetBsFixedGainDmx(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc);

HANDLE_ERROR_INFO
mp4SpaceEnc_Close(
    HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc);

HANDLE_ERROR_INFO
mp4SpaceEnc_GetInfo(
    HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
    MP4SPACEENC_INFO *const pInfo);

int mp4SpaceEnc_GetNumFramesBitstreamDelay(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc);

HANDLE_ERROR_INFO
mp4SpaceEnc_GetNumBitsPayloadThisFrame(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc, int *bitsNextPayload);

float mp4SpaceEnc_ParamBand2Freq(int nParamBands, int nSampleRate, int nParamBand, int nQmfBands);

HANDLE_ERROR_INFO
mp4SpaceEnc_InitDelayCompensation(
    HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc,
    const int coreCoderDelay);

HANDLE_ERROR_INFO
mp4SpaceEnc_ForceIndependency(
    HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc);

HANDLE_ERROR_INFO
mp4SpaceEnc_SetSpeechFlag(
    HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc,
    int speechFlag);

HANDLE_ERROR_INFO
mp4SpaceEnc_SetDmxGain(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, MP4SPACEENC_DMX_GAIN dmxGain);

HANDLE_ERROR_INFO
mp4SpaceEnc_SetIndependencyFactor(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, const unsigned int independencyFactor);

HANDLE_ERROR_INFO
mp4SpaceEnc_GetUsacMps212Config(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc, MP4SPACEENC_USAC_MPS212_CONFIG *pUsacMps212Config);

#ifdef __cplusplus
}
#endif

#endif
