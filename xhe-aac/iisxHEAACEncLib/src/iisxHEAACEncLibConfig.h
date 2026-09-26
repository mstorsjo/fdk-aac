
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

#ifndef IISXHEAACENCLIBCONFIG_H
#define IISXHEAACENCLIBCONFIG_H

#include "iisxHEAACEncLib_returnCodes.h"
#include "spaceEnclib.h"
#include "iisSigMap.h"
#include "iisParamList.h"
#include "IIS_MP4AEnc_ASC_Interface.h"
#include "iisParam.h"
#include "iisutillib.h"

#define XHEAACENCLIB_MAX_DRC_SEQUENCES (4)

typedef enum xheaacenclib_coding_mode {
  XHEAACENCLIB_CODING_MODE_FD = 0,
  XHEAACENCLIB_CODING_MODE_LPD,
  XHEAACENCLIB_CODING_MODE_SWITCHED
} XHEAACENCLIB_CODING_MODE;

typedef enum {

  MPEGS_DOWNMIX_DEFAULT = 0,

  MPEGS_DOWNMIX_FORCE_STEREO,
  MPEGS_DOWNMIX_MATRIX_COMPAT,

  MPEGS_DOWNMIX_ARBITRARY_MONO,
  MPEGS_DOWNMIX_ARBITRARY_STEREO

} MPEGS_DOWNMIX_CONFIG;

typedef enum {
  MPEGS_PAYLOAD_EMBED = 0,
  MPEGS_NO_PAYLOAD_EMBED = 1,
  MPEGS_PAYLOAD_EMBED_ASCEXT = 2
} MPEGS_PAYLOAD_MODE;

typedef enum {
  METADATA_NONE = 0,
  METADATA_MPEG,
  METADATA_MPEG_ETSI
} METADATA_MODE;

typedef enum frameLength {
  FRAMELENGTH_768 = 768,
  FRAMELENGTH_960 = 960,
  FRAMELENGTH_1024 = 1024,
  FRAMELENGTH_1920 = 1920,
  FRAMELENGTH_2048 = 2048,
  FRAMELENGTH_4096 = 4096
} FRAMELENGTH;

typedef enum codec_type_tag {
  XHEAACENCLIB_CODEC_AAC = 0,
  XHEAACENCLIB_CODEC_USAC = 1,
  XHEAACENCLIB_CODEC_UNSPECIFIED = 99
} XHEAACENCLIB_CODEC_TYPE;

typedef struct samplingFactor {
  int upFac;
  int downFac;
} SAMPLING_FACTOR;

typedef enum {
  SBR_SIGNALING_IMPLICIT = 0,
  SBR_SIGNALING_EXPL_BC = 1,
  SBR_SIGNALING_EXPL_HIER = 2,
  SBR_SIGNALING_DISABLE = 3
} SBR_SIGNALING_MODE;

typedef enum {
  AUD_OBJ_TYP_LC = 2,
  AUD_OBJ_TYP_HEAAC = 5,
  AUD_OBJ_TYP_PS = 29,
  AUD_OBJ_TYP_USAC = 42,
  AUD_OBJ_TYP_MP2_LC = 129,
  AUD_OBJ_TYP_MP2_SBR = 132
} AUD_OBJ_TYP;

typedef enum {
  CONFIG_SET_INVALID = -1,
  CONFIG_SET_MP4 = 0,
  CONFIG_SET_DRM_30 = 1,
  CONFIG_SET_DRM_PLUS = 2,
  CONFIG_SET_DASH = 3,
  CONFIG_SET_SEEKABLE = 4
} CONFIG_SET;

typedef enum {
  XHEAACENCLIB_MPEG2AAC_INVALID = -1,
  XHEAACENCLIB_MPEG2AAC_OFF = 0,
  XHEAACENCLIB_MPEG2AAC_ON
} XHEAACENCLIB_MPEG2AAC;

typedef enum {
  XHEAACENCLIB_PRESET_INVALID = -1,
  XHEAACENCLIB_PRESET_AUDIO_ARCHIVE = 0,
  XHEAACENCLIB_PRESET_AUDIO_STREAMING,
  XHEAACENCLIB_PRESET_AV_STREAMING,
  XHEAACENCLIB_PRESET_LIVE_STREAMING
} XHEAACENCLIB_PRESET;

typedef enum {
  XHEAACENCLIB_SIGMAP_FROM_FILE = -1,
  XHEAACENCLIB_SIGMAP_CICP_1 = 1,
  XHEAACENCLIB_SIGMAP_CICP_2 = 2,
  XHEAACENCLIB_SIGMAP_CICP_5 = 5,
  XHEAACENCLIB_SIGMAP_CICP_6 = 6,
  XHEAACENCLIB_SIGMAP_CICP_7 = 7,
  XHEAACENCLIB_SIGMAP_CICP_12 = 12,
  XHEAACENCLIB_SIGMAP_CICP_14 = 14,
  XHEAACENCLIB_SIGMAP_CICP_13 = 13,
  XHEAACENCLIB_SIGMAP_INVALID = -1000
} XHEAACENCLIB_SIGMAP_INDEX;

typedef enum {
  PS_OFF = 0,
  PS_ON = 1
} PS_MODE;

typedef enum {
  TOOL_MODE_DEFAULT = 0,
  TOOL_MODE_OFF,
  TOOL_MODE_ON
} TOOL_MODE;

typedef enum {
  XHEAACENCLIB_BR_MODE_INVALID = -1,
  XHEAACENCLIB_BR_MODE_CBR = 0,
  XHEAACENCLIB_BR_MODE_VBR_1 = 1,
  XHEAACENCLIB_BR_MODE_VBR_2 = 2,
  XHEAACENCLIB_BR_MODE_VBR_3 = 3,
  XHEAACENCLIB_BR_MODE_VBR_4 = 4,
  XHEAACENCLIB_BR_MODE_VBR_5 = 5,
  XHEAACENCLIB_BR_MODE_VBR_6 = 6,
  XHEAACENCLIB_BR_MODE_VBR_0 = 7
} XHEAACENCLIB_BITRATE_MODE;

typedef enum {
  XHEAACENCLIB_QUAL_INVALID = -1,
  XHEAACENCLIB_QUAL_FAST = 0,
  XHEAACENCLIB_QUAL_MEDIUM,
  XHEAACENCLIB_QUAL_HIGH
} XHEAACENCLIB_QUALITY;

typedef enum {
  XHEAACENCLIB_GRANULELENGTH_768 = 768,
  XHEAACENCLIB_GRANULELENGTH_960 = 960,
  XHEAACENCLIB_GRANULELENGTH_1024 = 1024
} XHEAACENCLIB_GRANULELENGTH;

typedef enum {
  XHEAACENCLIB_SAP_TYPE_NONE = 0,
  XHEAACENCLIB_SAP_TYPE_WDWTYPE,
  XHEAACENCLIB_SAP_TYPE_WDWTYPE_HIGHBW
} XHEAACENCLIB_SAP_TYPE;

typedef enum {
  TT_RAW = 0,
  TT_ADIF = 1,
  TT_ADTS = 2,
  TT_ADTSCRC = 3,
  TT_LOAS = 4,
  TT_LOAS_NOSMC = 5,
  TT_LATM = 6,
  TT_LATM_NOSMC = 7,

  tt_dummy
} TRANSPORT_TYPE;

typedef enum {
  XHEAACENCLIB_DRCMODE_INVALID = -1,
  XHEAACENCLIB_DRCMODE_OFF = 0,
  XHEAACENCLIB_DRCMODE_LN_NE_LI_GE_FILMSTD,
  XHEAACENCLIB_DRCMODE_LN_NE_LI_LRACONTROL_GE_FILMSTD,
  XHEAACENCLIB_DRCMODE_LN_NE_LI_GE_LRACONTROL
} XHEAACENCLIB_DRCMODE;

typedef enum {
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_INVALID = -2,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT = -1,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_NONE = 0,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_FILMSTANDARD = 1,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_FILMLIGHT = 2,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_MUSICSTANDARD = 3,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_MUSICLIGHT = 4,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF_SPEECH = 5,
  XHEAACENCLIB_MPEG4_DRC_LIGHT_EXT_GAIN = 6
} XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF;

typedef enum {
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_INVALID = -2,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT = -1,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_NONE = 0,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_FILMSTANDARD = 1,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_FILMLIGHT = 2,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_MUSICSTANDARD = 3,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_MUSICLIGHT = 4,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF_SPEECH = 5,
  XHEAACENCLIB_MPEG4_DRC_HEAVY_EXT_GAIN = 6
} XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF;

typedef enum {
  XHEAACENCLIB_APR_BITRESMODE_INVALID = -1,
  XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL = 0,
  XHEAACENCLIB_APR_BITRESMODE_OUT
} XHEAACENCLIB_APR_BITRESMODE;

typedef enum {
  XHEAACENC_PRIMINGMODE_INVALID = -1,
  XHEAACENC_PRIMINGMODE_FULL = 0,
  XHEAACENC_PRIMINGMODE_NONE,
  XHEAACENC_PRIMINGMODE_STDDELAY
} XHEAACENC_PRIMINGMODE;

typedef enum {
  XHEAACENCLIB_RAP_PROPERTY_INVALID = -1,
  XHEAACENCLIB_RAP_PROPERTY_OFF = 0,
  XHEAACENCLIB_RAP_PROPERTY_ACCESS,
  XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE,
  XHEAACENCLIB_RAP_PROPERTY_SEEKABLE
} XHEAACENCLIB_RAP_PROPERTY;

typedef enum {
  XHEAACENCLIB_RAP_OCCURRENCE_INVALID = -1,
  XHEAACENCLIB_RAP_OCCURRENCE_OFF = 0,
  XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL,
  XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND
} XHEAACENCLIB_RAP_OCCURRENCE;

typedef void XHEAACENCLIB_HANDLE_UNISTE;

typedef struct xheaacenclib_config_struct {
  XHEAACENCLIB_CODEC_TYPE codecType;
  int isValidConfig;

  XHEAACENCLIB_CODING_MODE coreMode;
  int acelpModeIndex;

  AUD_OBJ_TYP aot;
  XHEAACENCLIB_GRANULELENGTH granuleLength;
  int sampleRateIn;
  int sampleRateOut;
  unsigned int bitRate;
  unsigned int bitRatePenalty;
  unsigned int nAncDataBitRate;
  unsigned int nInChannels;
  unsigned int bUseSBR;
  unsigned int bUseSBRPVC;
  unsigned int bRaisedXOverFreq;
  unsigned int bUseSBR41;
  unsigned int bUseHBE;
  unsigned int bUseTSD;
  TOOL_MODE useSbrFixBorderForIndepFlag;
  TOOL_MODE useMpegsHighRateMode;
  int mpegsSetParamBands;
  unsigned int useNoiseFilling;
  int bUseTNS;
  int TLOverheadPerAU;

  int optimizedSpeedPulseSearch;

  FRAMELENGTH nFrameSamples;
  SAMPLING_FACTOR sbrRatio;

  CONFIG_SET configSet;
  XHEAACENCLIB_PRESET preset;

  SBR_SIGNALING_MODE sbrSignaling;
  TRANSPORT_TYPE transportFormat;
  int randomAccessIntervalMs;
  int randomAccessIntervalSamples;
  int randomAccessIntervalMin;
  int randomAccessIntervalInFrames;
  int rapOnDemandAdvancedMode;

  int loasNrSubFrames;
  XHEAACENCLIB_BITRATE_MODE bitrateMode;
  XHEAACENCLIB_QUALITY quality;
  XHEAACENCLIB_SIGMAP_INDEX CicpIndex;
  int lowDelaySwitching;
  int stereoConfigIndex;

  HANDLE_CODER_CONFIG_INFO hCoderConfig;

  XHEAACENCLIB_MPEG2AAC mpeg2AAC;

  int usacIndepFlagIntervalMs;
  int usacIndepFlagIntervalSamples;

  MPEGS_DOWNMIX_CONFIG mpegsDownmixCfg;
  MPEGS_PAYLOAD_MODE mpegsPayloadMode;
  float mpegsIndependencyTimeInterval;

  unsigned int nChannelsCoreCoder;
  XHEAACENCLIB_SIGMAP_INDEX CicpIndexCoreCoder;
  int sampleRateAAC;

  int bUseDownsampledSbr;
  int bUseSbrQmfInput;
  int bUseQmfResampler;
  int bitRateFractRemainder;
  int bitRateFractTimeBase;

  MP4SPACEENC_RES_CONFIG *pResidualConfig;

  int bitRateCodingTools;

  CHANNEL_MAPPING *cm;

  int audioPreRollNumAU;
  XHEAACENCLIB_APR_BITRESMODE audioPreRollBitResMode;

  int streamID;

  int additionalSyncFlushing;
  XHEAACENC_PRIMINGMODE primingMode;

  XHEAACENCLIB_RAP_PROPERTY rapProperty;
  XHEAACENCLIB_RAP_OCCURRENCE rapOccurrence;

  XHEAACENCLIB_DRCMODE drcMode;
  float drcTargetLra;
  int drcTargetLraPresent;

  int nMetadataBitRate;
  METADATA_MODE metadataMode;
  int metadataModeSet;

  int bMpeg4DrcOn;
  XHEAACENCLIB_MPEG4_DRC_LIGHT_PROF mpeg4DrcLightProf;
  float mpeg4DRCLightTrgtLvl;
  int mpeg4DRCLightTrgtLvl_present;
  XHEAACENCLIB_MPEG4_DRC_HEAVY_PROF mpeg4DrcHeavyProf;
  float mpeg4DRCHeavyTrgtLvl;
  int mpeg4DRCHeavyTrgtLvl_present;
  float mpeg4ProgRefLevel;
  int bMpeg4ProgRefLevelSet;

  int bMpeg4NoStartStopSequence;
  int mpeg4CenterMixLvl;
  int mpeg4SurroundMixLvl;
  int mpeg4WritePceMixDnLvl;
  int mpeg4EtsiDmxPresent;
  int mpeg4DolbySurroundMode;
  int mpeg4DolbyPseudoSurDmxEna;
  int mpeg4DrcPresMode;

  float mpeg4DrcGain;
  float mpeg4CompGain;

  unsigned int bMpeg4CenterMixLvlSet;
  unsigned int bMpeg4SurroundMixLvlSet;
  unsigned int bMpeg4WritePceMixDnLvlSet;
  unsigned int bMpeg4EtsiDmxPresentSet;
  unsigned int bMpeg4DolbySurroundModeSet;
  unsigned int bMpeg4DolbyPseudoSurDmxEnaSet;
  unsigned int bMpeg4DrcPresModeSet;

  float loudnessLevel;
  int bLoudnessLevelSet;
  float albumLoudnessLevel;
  int bAlbumLoudnessLevelSet;
  float anchorLoudnessLevel;
  int bAnchorLoudnessLevelSet;
  void *loudnessEnvelope;
  int bLoudnessEnvelopeSet;
  float samplePeak;
  int samplePeakPresent;
  float quietLoudnessThreshold;
  int bQuietLoudnessThresholdSet;
  int bDisableLoudness;
  float liveLoudnessLevel;
  unsigned int bLiveLoudnessLevelSet;
  float liveSamplePeak;
  unsigned int bLiveSamplePeakSet;
  int liveMode;
  unsigned int bLiveModeSet;
  float liveRelMaxGain;
  unsigned int bLiveRelMaxGainSet;

  int bRealtimeLRAC;
  int codecDelay;

} XHEAACENCLIB_CONFIG, *XHEAACENCLIB_CONFIG_HANDLE;

XHEAACENCLIB_RETURN iisxHEAACEncLib_ConfigNew(
    XHEAACENCLIB_CONFIG_HANDLE *phConfig);

XHEAACENCLIB_RETURN iisxHEAACEncLib_ConfigDelete(
    XHEAACENCLIB_CONFIG_HANDLE *phConfig);

int iisxHEAACEncLibGetParamList(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr);

XHEAACENCLIB_RETURN applyConfigurationSettings(XHEAACENCLIB_CONFIG_HANDLE hConfig);

#endif
