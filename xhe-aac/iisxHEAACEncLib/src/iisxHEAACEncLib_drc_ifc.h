
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

#ifndef IISXHEAACENCLIB_DRC_IFC_H
#define IISXHEAACENCLIB_DRC_IFC_H

#include "iisxHEAACEncLibConfig.h"
#include "iisDRCGainGenerator_api.h"
#include "iisDRCGainEnc_api.h"
#include "iisDRCGainEnc_uniDrc.h"
#include "time_buffer.h"
#include "iisxHEAACEncLib_returnCodes.h"

#define LOOKAHEADMS 20.0f

#define DRC_IFC_ERROR_FIRST 1000

#define INPUT_GAINSET_COUNT_MAX (10)
#define INPUT_DRCINSTRUCTIONSUNIDRCV1_COUNT_MAX (5)
#define INPUT_DRCCOEFFICIENTSUNIDRCV1_COUNT_MAX (1)
#define INPUT_DRCNODE_COUNT_MAX (9)
#define XHEAACENCLIB_MAX_DRC_CHAR_NODES (8)

typedef enum {
  DRC_IFC_NO_ERROR = 0,
  DRC_IFC_WARNING_PARAM_NULL,
  DRC_IFC_WARNING_2,
  DRC_IFC_ERROR_INVALID_HANDLE = DRC_IFC_ERROR_FIRST,
  DRC_IFC_ERROR_MEMORY,
  DRC_IFC_ERROR_INVALID_SETUP,
  DRC_IFC_ERROR_DELAY,
  DRC_IFC_ERROR_TIME_BUFF,
  DRC_IFC_ERROR_CHARACTERISTIC_IDX_NOTSUPPORTED,
  DRC_IFC_ERROR_CHANNEL_MODE_NOTSUPPORTED,
  DRC_IFC_ERROR_GAIN_ENC,
  DRC_IFC_ERROR_GAIN_GEN,
  DRC_IFC_ERROR_WRONG_USE_OF_DRC_EXT_GAIN,
  DRC_IFC_ERROR_WRONG_NUMBER_SEQUENCES,
  DRC_IFC_ERROR_TOO_LESS_EXTERNAL_DRC_GAINS,
  DRC_IFC_ERROR_SEND_EXTERNAL_DRC_GAINS_IN_FLUSHINGMODE,
  DRC_IFC_ERROR_RTLRAC,
  DRC_IFC_ERROR_UNKNOWN
} DRC_IFC_RETURN;

typedef struct drc_ifc_data_struct *XHEAACENCLIB_HANDLE_DRCENCODER;

typedef struct {
  int drcSetEffect;
  int TargetLoudnessValueUpper;
  int TargetLoudnessValueLower;
  int gainSetIndex;
  float attenuationScaling;
  float amplificationScaling;
  float gainOffset;
} XHEAACENCLIB_DRCENCODER_SETUP_INSTRUCTION;

typedef struct drcencoder_setup_coefficients_struct {
  DRC_CHARACTERISTIC_INDEX drcCharacteristics[INPUT_GAINSET_COUNT_MAX];
  IISDRCGAINENC_GAINCODINGPROFILE gainCodingProfile[INPUT_GAINSET_COUNT_MAX];
} XHEAACENCLIB_DRCENCODER_SETUP_COEFFICIENTS;

typedef struct drcencoder_setup_struct {
  int sampleRate;
  int nFrameLength;
  int nAudioChannels;
  float loudnessLevel[MAXNUMSEQUENCES];
  XHEAACENCLIB_DRCENCODER_SETUP_COEFFICIENTS coefficientsSetup[INPUT_DRCCOEFFICIENTSUNIDRCV1_COUNT_MAX];
  int bRealtimeLRAC;
  float targetLra;
  int bTargetLraSet;
  int bIsLevelerActive;

  AUD_OBJ_TYP aot;
  XHEAACENCLIB_DRCENCODER_SETUP_INSTRUCTION instructionSetup[INPUT_DRCINSTRUCTIONSUNIDRCV1_COUNT_MAX];

  int nInstructionCount;
  int nCharacteristicCount;
  float albumLoudnessLevel;
  int bAlbumLoudnessLevelSet;
  int nNodes;
  int inLevel[INPUT_DRCNODE_COUNT_MAX];
  int outGain[INPUT_DRCNODE_COUNT_MAX];
} XHEAACENCLIB_DRCENCODER_SETUP;

typedef struct {
  int drcExternalNodeNumNodes;
  int drcExternalNodeLevels[XHEAACENCLIB_MAX_DRC_CHAR_NODES];
  int drcExternalNodeGains[XHEAACENCLIB_MAX_DRC_CHAR_NODES];
  float drcGainOffset[XHEAACENCLIB_MAX_DRC_SEQUENCES];
} XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA;

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_New(
    XHEAACENCLIB_HANDLE_DRCENCODER *phInstance

);

DRC_IFC_RETURN iisxHEAACEncLib_drc_ifc_init(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    XHEAACENCLIB_DRCENCODER_SETUP setup, LoudnessInfoSet *loudnessInfoSet);

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_apply(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int const nChannels,
    float const *const pSamples,
    int const nSamples,
    unsigned char **pUniDrcGainBs,
    int *iUniDrcGainLength,
    int bStartPreroll, AUD_OBJ_TYP aot);

XHEAACENCLIB_RETURN iisxHEAACEncLib_GetUniDrcConfig(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    unsigned char **pUniDrcConfigBs,
    int *iUniDrcConfigLength);

DRC_IFC_RETURN iisxHEAACEncLib_drc_ifc_Delete(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance);

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_compressor_lookAhead(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nLookAhead);

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_decoder_delay(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nDecoderDelay);

DRC_IFC_RETURN iisxHEAACEncLib_drc_get_encoder_delay(
    XHEAACENCLIB_HANDLE_DRCENCODER hInstance,
    int *const nEncoderDelay);

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_set_lra_control_drc_gains(
    XHEAACENCLIB_HANDLE_DRCENCODER const hInstance,
    float *lraControlDrcGains,
    unsigned int const lraControlDrcGainsLength,
    unsigned int const lraControlDrcGainProcessingIsActive);

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_set_loudness_range(
    XHEAACENCLIB_HANDLE_DRCENCODER const hInstance,
    float const loudnessRange,
    unsigned int const loudnessRangeCount);

XHEAACENCLIB_RETURN iisxHEAACEncLib_drc_ifc_setupConfig(
    XHEAACENCLIB_DRCENCODER_SETUP *const hDrcSetup,
    XHEAACENCLIB_DRCMODE const drcMode,
    XHEAACENCLIB_DRC_EXTERNAL_NODES_DATA const *const drcExternalNodes);

#endif
