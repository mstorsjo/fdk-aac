
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

#ifndef USACCONFIG_H
#define USACCONFIG_H

#include "audioobjecttypes.h"

#include "bit_buf.h"
#include "audiospecificconfig.h"

#define USAC_MAX_ELEMENTS (255)
#define USAC_MAX_CHANNELS (USAC_MAX_ELEMENTS * 2)
#define USAC_MAX_EXTENSION_CONFIGS 20
#define USAC_MAX_CONFIG_EXTENSION 10
#define USAC_MAX_EXTENSION_CONFIG_LENGTH 2048

typedef enum {
  USAC_CHANNEL_POS_L = 0,
  USAC_CHANNEL_POS_R,
  USAC_CHANNEL_POS_C,
  USAC_CHANNEL_POS_LFE,
  USAC_CHANNEL_POS_Ls,
  USAC_CHANNEL_POS_Rs,
  USAC_CHANNEL_POS_Lc,
  USAC_CHANNEL_POS_Rc,
  USAC_CHANNEL_POS_Lsr,
  USAC_CHANNEL_POS_Rsr,
  USAC_CHANNEL_POS_Cs,
  USAC_CHANNEL_POS_Lsd,
  USAC_CHANNEL_POS_Rsd,
  USAC_CHANNEL_POS_Lss,
  USAC_CHANNEL_POS_Rss,
  USAC_CHANNEL_POS_Lw,
  USAC_CHANNEL_POS_Rw,
  USAC_CHANNEL_POS_Lv,
  USAC_CHANNEL_POS_Rv,
  USAC_CHANNEL_POS_Cv,
  USAC_CHANNEL_POS_Lvr,
  USAC_CHANNEL_POS_Rvr,
  USAC_CHANNEL_POS_Cvr,
  USAC_CHANNEL_POS_Lvss,
  USAC_CHANNEL_POS_Rvss,
  USAC_CHANNEL_POS_Ts,
  USAC_CHANNEL_POS_LFE2,
  USAC_CHANNEL_POS_Lb,
  USAC_CHANNEL_POS_Rb,
  USAC_CHANNEL_POS_Cb,
  USAC_CHANNEL_POS_Lvs,
  USAC_CHANNEL_POS_Rvs

} USAC_CHANNEL_POSITION;

typedef enum {

  USAC_CONFIG_SBR_RATIO_INDEX_NO_SBR = 0,
  USAC_CONFIG_SBR_RATIO_INDEX_4_1 = 1,
  USAC_CONFIG_SBR_RATIO_INDEX_8_3 = 2,
  USAC_CONFIG_SBR_RATIO_INDEX_2_1 = 3

} USAC_CONFIG_SBR_RATIO_INDEX;

typedef enum {

  USAC_CONFIG_OUT_FRAMELENGTH_768 = 768,
  USAC_CONFIG_OUT_FRAMELENGTH_1024 = 1024,
  USAC_CONFIG_OUT_FRAMELENGTH_2048 = 2048,
  USAC_CONFIG_OUT_FRAMELENGTH_4096 = 4096

} USAC_CONFIG_OUT_FRAMLENGTH;

typedef enum {

  USAC_ELEMENT_TYPE_SCE = 0,
  USAC_ELEMENT_TYPE_CPE = 1,
  USAC_ELEMENT_TYPE_LFE = 2,
  USAC_ELEMENT_TYPE_EXT = 3

} USAC_ELEMENT_TYPE;

typedef enum {
  USAC_ID_EXT_ELE_FILL = 0,
  USAC_ID_EXT_ELE_MPEGS = 1,
  USAC_ID_EXT_ELE_SAOC = 2,
  USAC_ID_EXT_ELE_PREROLL = 3,
  USAC_ID_EXT_ELE_UNI_DRC = 4
} USAC_EXT_ELEMENT_TYPE;

typedef enum {
  USAC_ID_CONFIG_EXT_FILL = 0,
  USAC_ID_CONFIG_EXT_LOUDNESS_INFO = 2,
  USAC_ID_CONFIG_EXT_STREAM_ID = 7
} USAC_CONFIG_EXTENSION_TYPE;

typedef enum {
  USAC_ID_CONFIG_EXT_LOUDNESS_INFO_DRM = 0,
  USAC_ID_CONFIG_EXT_LOUDNESS_INFO_DRM_SAMPLEPEAK = 1

} USAC_CONFIG_EXTENSION_TYPE_DRM;

typedef struct {
  unsigned int tw_mdct;
  unsigned int noiseFilling;

} USAC_CORE_CONFIG;

typedef struct {
  unsigned int start_freq;
  unsigned int stop_freq;
  unsigned int header_extra1;
  unsigned int header_extra2;
  unsigned int freq_scale;
  unsigned int alter_scale;
  unsigned int noise_bands;
  unsigned int limiter_bands;
  unsigned int limiter_gains;
  unsigned int interpol_freq;
  unsigned int smoothing_mode;

} USAC_SBR_HEADER;

typedef struct {
  unsigned int harmonicSBR;
  unsigned int bs_interTes;
  unsigned int bs_pvc;
  USAC_SBR_HEADER sbrDfltHeader;

} USAC_SBR_CONFIG;

typedef struct {
  USAC_CORE_CONFIG usacCoreConfig;
  USAC_SBR_CONFIG usacSbrConfig;

} USAC_SCE_CONFIG;

typedef struct {
  unsigned int bsFreqRes;
  unsigned int bsFixedGainDMX;
  unsigned int bsTempShapeConfig;
  unsigned int bsDecorrConfig;
  unsigned int bsHighRateMode;
  unsigned int bsPhaseCoding;
  unsigned int bsOttBandsPhasePresent;
  unsigned int bsOttBandsPhase;
  unsigned int bsResidualBands;
  unsigned int bsPseudoLr;
  unsigned int bsEnvQuantMode;

} USAC_MPS212_CONFIG;

typedef struct {
  USAC_CORE_CONFIG usacCoreConfig;
  USAC_SBR_CONFIG usacSbrConfig;
  unsigned int stereoConfigIndex;
  USAC_MPS212_CONFIG usacMps212Config;

} USAC_CPE_CONFIG;

typedef struct {
  USAC_CORE_CONFIG usacCoreConfig;

} USAC_LFE_CONFIG;

typedef struct {
  unsigned int usacExtElementType;
  unsigned int usacExtElementConfigLength;
  unsigned int usacExtElementDefaultLengthPresent;
  unsigned int usacExtElementDefaultLength;
  unsigned int usacExtElementPayloadFrag;
  unsigned char usacExtElementConfigPayload[6144 / 8];

} USAC_EXT_CONFIG;

typedef union {
  USAC_SCE_CONFIG usacSceConfig;
  USAC_CPE_CONFIG usacCpeConfig;
  USAC_LFE_CONFIG usacLfeConfig;
  USAC_EXT_CONFIG usacExtConfig;

} USAC_ELEMENT_CONFIG;

typedef struct {
  unsigned int numElements;
  USAC_ELEMENT_TYPE usacElementType[USAC_MAX_ELEMENTS];
  USAC_ELEMENT_CONFIG usacElementConfig[USAC_MAX_ELEMENTS];

} USAC_DECODER_CONFIG;

typedef struct {
  unsigned int numOutChannels;
  USAC_CHANNEL_POSITION usacOutputChannelPos[USAC_MAX_CHANNELS];

} USAC_CHANNEL_CONFIG;

typedef struct {
  unsigned int numConfigExtensions;
  unsigned int usacConfigExtType[USAC_MAX_ELEMENTS];
  unsigned int usacConfigExtLength[USAC_MAX_ELEMENTS];
  unsigned char usacConfigExtPayload[USAC_MAX_ELEMENTS][6144 / 8];
} USAC_CONFIG_EXTENSION;

typedef struct {
  unsigned int numConfigExtensionsDrm;
  unsigned int usacConfigExtTypeDrm[USAC_MAX_ELEMENTS];
  unsigned int usacConfigExtLengthDrm[USAC_MAX_ELEMENTS];
  unsigned char usacConfigExtPayloadDrm[USAC_MAX_ELEMENTS][6144 / 8];
} USAC_CONFIG_EXTENSION_DRM;

typedef struct USAC_CONFIG {
  unsigned int usacSamplingFrequencyIndex;
  unsigned int usacSamplingFrequency;

  USAC_CONFIG_OUT_FRAMLENGTH outputFrameLength;
  USAC_CONFIG_SBR_RATIO_INDEX sbrRatioIndex;

  unsigned int channelConfigurationIndex;

  USAC_CHANNEL_CONFIG usacChannelConfig;
  USAC_DECODER_CONFIG usacDecoderConfig;

  USAC_CONFIG_EXTENSION usacConfigExtension;
  USAC_CONFIG_EXTENSION_DRM usacConfigExtensionDrm;

} USAC_CONFIG;
typedef USAC_CONFIG *HANDLE_USACC;

void WriteUsacConfig(HANDLE_ASC hAsc,
                     struct AOT_SPECIFIC_CONFIG *aotSpecificConfig,
                     HANDLE_BIT_BUF hBitBuf);

#endif
