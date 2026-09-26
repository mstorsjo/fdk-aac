
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

#ifndef XHEAACENCLOUDNESS_
#define XHEAACENCLOUDNESS_

#include "loudnessLib.h"
#include "iisParamList.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XHEAACENC_MAX_DRC_CHAR_NODES (9)
#define XHEAACENC_MAX_DRC_SEQUENCES (4)
#define XHEAACENC_INVALID_LOUDNESS_LEVEL (3.402823466e+38F)
#define XHEAACENC_LOUDNESS_ENV_BLOCK_LENGTH_MS (100)
#define XHEAACENC_LOUDNESS_LEVEL_MIN (-57.75F)
#define XHEAACENC_LOUDNESS_LEVEL_MAX (6.0F)
#define XHEAACENC_SAMPLE_PEAK_MIN (-107.0F)
#define XHEAACENC_SAMPLE_PEAK_MAX (0.0F)
#define LOWPASS_FILTER_TAP_NUM (16)
#define MAX_CHANNELS_LOUDNESS (2)
#define LOWPASS_CUTOFF (18000)
#define BETA_WIN (1.7F)

typedef enum {
  IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_INVALID = -1,
  IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_EXTERNAL_SET = 0,
  IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_ANCHOR_INTERNAL_MEASURED,
  IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE_PRL
} IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE;

typedef struct {
  float history[LOWPASS_FILTER_TAP_NUM];
  unsigned int last_index;
} LP_FILTER, *LP_FILTER_HANDLE;

typedef struct {
  float *lraControlDrcGains;
  unsigned int lraControlDrcGainsLength;
  unsigned int drcExternalNodeNumNodes;
  int drcExternalNodeLevels[XHEAACENC_MAX_DRC_CHAR_NODES];
  int drcExternalNodeGains[XHEAACENC_MAX_DRC_CHAR_NODES];
  float drcGainOffset[XHEAACENC_MAX_DRC_SEQUENCES];
} IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA;

typedef struct xheaacenc_loudness_data_struct {
  float *instantaneousLoudness;
  int instantaneousLoudnessLength;
  int nSamplesInLoudnessMeter;
  int audioBlocksProcessed;
  int minRequiredSamplesProcessed;
  float loudnessRange;
  unsigned char *vaBuffer;
} IIS_XHEAACENC_LOUDNESS_DATA;

typedef struct xheaacenc_databuffer_struct IIS_XHEAACENC_DATA_BUFFER;

IIS_XHEAACENC_RETURN_CODE deleteDataBuffer(
    IIS_XHEAACENC_DATA_BUFFER **const phDataBuffer);

typedef struct xheaacenc_loudness_instance_struct {
  IIS_XHEAACENC_LOUDNESS_DATA *hLoudnessData;
  HLOUDNESS_METER hLoudnessMeter;
  float loudness;
  float samplePeak;
  float loudnessRange;
  int nChannels;
  int audioBlockLength;
  int usesDefaultLoudnessEnvelopeLength;
  IIS_XHEAACENC_DATA_BUFFER *buffer;
  int isImported;
  LP_FILTER_HANDLE lp_filter[MAX_CHANNELS_LOUDNESS];
  float *lp_samples;
  int sampleRate;
  HLOUDNESS_METER hLoudnessMeter_unmodified;
  int downsampleEnabled;
  int lpFilteringEnabled;
  float *filter_taps;
  int measureAnchorLoudness;
  float anchorLoudness;
  int isInitialized;
} IIS_XHEAACENC_LOUDNESS_INSTANCE;

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_ConvertToDrcGains(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA *const hLraControlDrcGainData,
    float const loudnessLevel,
    IIS_XHEAACENC_LOUDNESS_LEVEL_TYPE const loudnessLevelType,
    unsigned char const *const vaBuffer,
    float const targetLRA,
    PARAMLIST_DRCMODE const drcMode);

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_LraControlDrcGainDataCreate(
    IIS_XHEAACENC_LOUDNESS_DATA *const pLoudnessData,
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA **const phLraControlDrcGainData);

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Loudness_LraControlDrcGainDataDelete(
    IIS_XHEAACENC_LRACONTROL_DRC_GAINS_DATA *hLraControlDrcGainData);

#ifdef __cplusplus
}
#endif

#endif
