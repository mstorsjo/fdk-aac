
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

#ifndef aacenc_h_
#define aacenc_h_

#include "iisSigMap.h"
#include "extpayload.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#pragma pack(push, 1)
#endif

#ifndef AACENCAPI
#ifdef WIN32
#define AACENCAPI __stdcall
#else
#define AACENCAPI
#endif
#endif

typedef enum {
  QUAL_INVALID = -1,
  QUAL_FAST = 0,
  QUAL_MEDIUM = 2,
  QUAL_HIGH = 3
} AACENC_QUALITY;

typedef enum {
  AACENC_MUX_RAW = 0,
} AACENC_TRANS_MUX;

typedef enum {
  AACENC_BR_MODE_INVALID = -1,
  AACENC_BR_MODE_CBR = 0,
  AACENC_BR_MODE_VBR_1 = 1,
  AACENC_BR_MODE_VBR_2 = 2,
  AACENC_BR_MODE_VBR_3 = 3,
  AACENC_BR_MODE_VBR_4 = 4,
  AACENC_BR_MODE_VBR_5 = 5,
  AACENC_BR_MODE_VBR_6 = 6,
  AACENC_BR_MODE_VBR_0 = 7
} AACENC_BITRATE_MODE;

typedef enum {
  AACENC_SAP_TYPE_NONE = 0,
  AACENC_SAP_TYPE_WDWTYPE = 1,
  AACENC_SAP_TYPE_WDWTYPE_HIGHBW = 2
} AACENC_SAP_TYPE;

typedef enum {
  AACENC_PCE_DATA_MATRIX_MIXDOWN_IDX_PRESENT = 0,
  AACENC_PCE_DATA_MATRIX_MIXDOWN_IDX = 1,
  AACENC_PCE_DATA_PSEUDO_SURROUND_ENABLE = 2
} AACENC_PCE_DATA;

typedef enum {
  AACENC_GRANULE_INVALID = -1,
  AACENC_GRANULE_768 = 768,
  AACENC_GRANULE_1024 = 1024
} AACENC_GRANULE_LEN;

typedef enum aacenc_setup_coding_mode {
  AACENC_SETUP_CODING_MODE_FD = 0,
  AACENC_SETUP_CODING_MODE_LPD,
  AACENC_SETUP_CODING_MODE_SWITCHED,
  AACENC_SETUP_CODING_MODE_ACELP,
  AACENC_SETUP_CODING_MODE_TCX
} AACENC_SETUP_CODING_MODE;

typedef enum aacenc_coding_mode {
  AACENC_CODING_MODE_FD = 0,
  AACENC_CODING_MODE_LPD
} AACENC_CODING_MODE;

typedef enum aacenc_mct_mode {
  AACENC_MCT_MODE_OFF = 0,
  AACENC_MCT_MODE_ROTATION,
  AACENC_MCT_MODE_PREDICTION
} AACENC_MCT_MODE;

typedef enum aacenc_mpegh_profilelevel {
  AACENC_MPEGH_PROFILELEVEL_INVALID = 0x00,
  AACENC_MPEGH_PROFILELEVEL_LC_LEVEL_1 = 0x0B,
  AACENC_MPEGH_PROFILELEVEL_LC_LEVEL_2 = 0x0C,
  AACENC_MPEGH_PROFILELEVEL_LC_LEVEL_3 = 0x0D,
  AACENC_MPEGH_PROFILELEVEL_LC_LEVEL_4 = 0x0E,
  AACENC_MPEGH_PROFILELEVEL_LC_LEVEL_5 = 0x0F,
  AACENC_MPEGH_PROFILELEVEL_BASELINE_1 = 0x10,
  AACENC_MPEGH_PROFILELEVEL_BASELINE_2 = 0x11,
  AACENC_MPEGH_PROFILELEVEL_BASELINE_3 = 0x12,
  AACENC_MPEGH_PROFILELEVEL_BASELINE_4 = 0x13,
  AACENC_MPEGH_PROFILELEVEL_BASELINE_5 = 0x14
} AACENC_MPEGH_PROFILELEVEL;

typedef enum aacenc_ipf_state {
  AACENC_IPF_STATE_NO,
  AACENC_IPF_STATE_RAP_FIRST_PREROLL,
  AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL,
  AACENC_IPF_STATE_RAP_NEXT_PREROLL,
  AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL,
  AACENC_IPF_STATE_RAP_IPF,
  AACENC_IPF_STATE_CONFIGCHANGE_IPF,
  AACENC_IPF_STATE_RAP_IPF_PREROLL,
  AACENC_IPF_STATE_CONFIGCHANGE_IPF_PREROLL

} AACENC_IPF_STATE;

typedef enum {
  AACENC_NO_ERROR = 0,
  AACENC_UNKNOWN_ERROR,
  AACENC_INIT_ERROR,
  AACENC_ELEMENT_ERROR,
  AACENC_CONF_EXP_ERROR,
  AACENC_GET_CONF_ERROR,
  AACENC_NSAMPLES_ERROR,
  AACENC_BUFSIZE_ERROR,
  AACENC_LITTLE_BITRES_ERROR,
  AACENC_OVERFLOW_BITRES_ERROR,
  AACENC_BITS_QUANT_ERROR,
  AACENC_WRITE_SEC_ERROR,
  AACENC_WRITE_SCAL_ERROR,
  AACENC_WRITE_SPEC_ERROR,
  AACENC_WRITE_DSE_ERROR,
  AACENC_WRITTEN_BITS_ERROR,
  AACENC_STACK_ALIGNMENT_ERROR,
  AACENC_ILLEGAL_PARAMETER_ERROR,
  AACENC_INVALID_POINTER_ERROR,
  AACENC_WARNING_MIN = 128,
  AACENC_WARNING_STACK_ALIGNMENT = AACENC_WARNING_MIN,
  AACENC_INVALID_BITRATE,
  AACENC_OPERATIONPOINT_UNSUPPORTED,
  AACENC_MCTTUNING_NOT_FOUND
} AACFASTENC_ERROR;

typedef enum {
  AACENC_BD_MODE_INTER_ELEMENT = 0,
  AACENC_BD_MODE_INTRA_ELEMENT = 1
} AACENC_BIT_DISTRIBUTION_MODE;

typedef enum {
  AACENC_SETUP_CODEC_UNKNOWN = -1,
  AACENC_SETUP_CODEC_AAC = 0,
  AACENC_SETUP_CODEC_XHEAAC = 1,
  AACENC_SETUP_CODEC_MPEGH = 2
} AACENC_SETUP_CODEC_TYPE;

typedef struct AACENC_ENCODER_TAG AACENC_ENCODER, *AACENC_ENCODER_HANDLE;

typedef void (*AACENC_MESSAGE_CALLBACK_PUBLIC)(char*);

typedef struct UNISTE_TAG {
  float umxMatRe[32 + 300][28][4];
  float umxMatIm[32 + 300][28][4];
  float cld[32 + 300][28];
  int nTimeSlots;
  int nParamBands;
  int nResidualBands;
  float paramBandBorders[28 + 1];
  float paramBandBordersLong[28 + 1];
  float paramBandBordersShort[28 + 1];
  int delayUmxMat2Mdct;
  int bStereoSbr;
  int bPseudoLr;
  float pseudoLrUmxMat[4];
  int nResidualLinesMdctLong;
  int nResidualLinesMdctShort;
} UNISTE, *HANDLE_UNISTE;

typedef struct AACENC_SETUP_TAG {
  AACENC_SETUP_CODEC_TYPE codecType;
  int bitRate;
  int bitReservoirPenalty;
  float bitResDistribution;
  AACENC_BITRATE_MODE bitrateMode;
  AACENC_QUALITY quality;
  int sampleRate;
  int mpeg4;
  int ancDataBitRate;
  int useNoiseFilling;
  AACENC_GRANULE_LEN nGranuleLength;
  AACENC_BIT_DISTRIBUTION_MODE bitDistributionMode;
  int bResidualCoding;
  CHANNEL_MAPPING* channelMapping;
  AACENC_TRANS_MUX transMux;
  int calcCrc;
  float bandWidth;
  int useTns;

  int useEnhancedNoiseFilling;
  int igfStartFq;
  int igfStopFq;
  int igfUseHighRes;
  int igfAfterTnsSynth;
  int igfUseWhitening;
  int igfUseENF;
  int igfUseIndependentTiling;

  AACENC_MCT_MODE mctMode;

  int stereoFilling;
  int useLtpPost;
  int useFDP;
  int fullbandLpd;
  int useStereoLpd;
  int lpdStereoModeIndex;

  int elementLengthPresent;

  AACENC_MPEGH_PROFILELEVEL mpegh3daProfileLevelIndication;

  int bitRateFractRemainder;
  int bitRateFractTimeBase;

  AACENC_SETUP_CODING_MODE codingMode;
  int acelpModeIndex;
  int optimizedSpeedPulseSearch;
  int useExtendedBitReservoir;
  int useLloydMaxQuantizer;
} AACENC_SETUP, *AACENC_SETUP_HANDLE;

typedef struct AACENC_CONFIG_TAG AACENC_CONFIG, *AACENC_CONFIG_HANDLE;

typedef struct {
  float bandWidth;
  int nDelay;
  int nAddEncDelay;
  int nStandDelay;
  int cbBufSizeMin;
  int nAncBytesPerFrame;

  char version[50];
} AACENC_INFO;

typedef int (*IISAACFENC_HEADER_BITS_CALLBACK)(void* handle, int bitsPerFrame, int bitReservoir, int mode, int* headerBits);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_SetHeaderBitsCallback(
    AACENC_ENCODER_HANDLE hAacEnc,
    void* headerBitsCallbackHandle,
    IISAACFENC_HEADER_BITS_CALLBACK const headerBitsCallbackFunc);

int IISAACFENC_GetVBRBitrate(AACENC_BITRATE_MODE bitrateMode, CHANNEL_MAPPING_HANDLE hChMap, AACENC_SETUP_CODEC_TYPE codecType);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_GetConfiguration(const AACENC_SETUP setup,
                            AACENC_CONFIG_HANDLE* phConfig);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncUpdate(AACENC_ENCODER_HANDLE* phAacEnc,
                        const AACENC_CONFIG_HANDLE hConfig);

void AACENCAPI IISAACFENC_AacEncDelete(AACENC_ENCODER_HANDLE hAacEnc);

void AACENCAPI IISAACFENC_AacEncClose(AACENC_ENCODER_HANDLE hAacEnc);

void AACENCAPI IISAACFENC_AacConfigClose(AACENC_CONFIG_HANDLE* const phConfig);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetMinFrameBytes(
    AACENC_ENCODER_HANDLE hAacEnc,
    int minAuBytes);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncResetRateControl(
    AACENC_ENCODER_HANDLE hAacEnc);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncEncodeFrame(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float* pSamples,
    const int nSamples,
    unsigned char* const pOutput,
    unsigned char* const pOutputApr,
    const int cbSize,
    int* const cbOutBits,
    int* const cbOutBitsApr,
    AACENC_CODING_MODE* codingModeNext,
    const HANDLE_UNISTE* phUniSte,
    unsigned char* pMetadataData,
    unsigned int* pNumMetadataData,
    const unsigned int metadataMaxBytesPerFrame,
    unsigned int nBitsTransportOverhead,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER* hExternalContainer,
    int numExternalContainersInUse,
    const AACENC_IPF_STATE ipfState);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetInfo(const AACENC_ENCODER_HANDLE hAacEnc,
                         AACENC_INFO* pInfo);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetOffsets(const AACENC_ENCODER_HANDLE hAacEnc,
                            const unsigned int* const channelOffsets);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSnapToLowSfbBorder(const int sampleRate,
                                    const AACENC_GRANULE_LEN granuleLength,
                                    const float desiredBandwidth,
                                    const float tol,
                                    float* adjBandwidth);

int AACENCAPI IISAACFENC_AacEncSetAdditionalProgramConfig(AACENC_ENCODER_HANDLE const hAacEnc,
                                                          const AACENC_PCE_DATA dataElement, unsigned int data);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetPceTimeInterval(AACENC_ENCODER_HANDLE const hAacEnc,
                                    float* sendPceTimeInterval);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetPceTimeInterval(AACENC_ENCODER_HANDLE const hAacEnc,
                                    const float sendPceTimeInterval);

int AACENCAPI
IISAACFENC_AacEncSetBandwidth(AACENC_CONFIG_HANDLE const hAacConfig,
                              const float proposedBandwidth,
                              float* usedBandwidth);

AACFASTENC_ERROR
IISAACFENC_SAPPrepare(AACENC_ENCODER_HANDLE const hAacEnc,
                      const AACENC_SAP_TYPE syncType);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetMpeg4Flag(AACENC_ENCODER_HANDLE const hAacEnc,
                              const int mpeg4Flag);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncConfigureADTSPrivateAndOrigBit(AACENC_ENCODER_HANDLE const hAacEnc,
                                                const int PrivateBit,
                                                const int OrigCopy);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetupADTSPrivateAndOrigBit(AACENC_CONFIG_HANDLE hAacConfig,
                                            const int PrivateBit,
                                            const int OrigCopy);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetTransMux(AACENC_ENCODER_HANDLE const hAacEnc,
                             const AACENC_TRANS_MUX transMux,
                             const int calcCrc);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetBitReservoirInfo(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const bitReservoirMax,
    int* const bitReservoir,
    float* const bitReservoirLevel);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncGetAvgFrameSize(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const avgBitsPerFrame);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetBitReservoirLevel(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float bitReservoirLevel);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncForceBitReservoirFilling(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int bForceBitResFilling);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_ExtendedBitReservoirSetFillrate(
    AACENC_ENCODER_HANDLE const hAacEnc,
    float const averageFrameRatio);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetBlockSwitchingNoStartStop(AACENC_ENCODER_HANDLE const hAacEnc,
                                              const int noStartStopSequence);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncSetTns(AACENC_CONFIG* hConfig,
                        const int useTns);

int AACENCAPI IISAACFENC_AacEncGetUseNoiseFilling(AACENC_CONFIG_HANDLE hAacConfig);

int AACENCAPI IISAACFENC_AacEncGetSampleRate(AACENC_CONFIG_HANDLE hAacConfig);

AACENC_GRANULE_LEN AACENCAPI IISAACFENC_AacEncGetGranuleLength(AACENC_CONFIG_HANDLE hAacConfig);

AACFASTENC_ERROR
IISAACFENC_AacEncSetSampleRate(AACENC_CONFIG_HANDLE const hConfig,
                               const int sampleRate);

AACFASTENC_ERROR
IISAACFENC_AacEncSetAncDataBitRate(AACENC_CONFIG_HANDLE const hConfig,
                                   const int ancDataBitRate);

AACFASTENC_ERROR
IISAACFENC_AacEncSetUseIntensityStereo(AACENC_CONFIG_HANDLE const hConfig,
                                       const int useIntensityStereo);

AACFASTENC_ERROR
IISAACFENC_AacEncSetUseStereoPreprocessor(AACENC_CONFIG_HANDLE const hConfig,
                                          const int useStereoPreprocessor);

AACFASTENC_ERROR AACENCAPI
iisaacfenc_isLastShortWindow(AACENC_ENCODER_HANDLE const hAacEnc,
                             int* isLastShortWindow);

AACFASTENC_ERROR IISAACFENC_ExtendedBitReservoirUpdateParameters(
    const AACENC_ENCODER_HANDLE hAacEnc,
    const int nBits,
    const int intervalSamples,
    const float preRollAUFactor);

AACFASTENC_ERROR AACENCAPI
IISAACFENC_AacEncClone(
    AACENC_ENCODER_HANDLE hDest,
    const AACENC_ENCODER_HANDLE hSrc);

AACFASTENC_ERROR IISAACFENC_AacEncGetThresholdsForExternalBits(
    AACENC_ENCODER_HANDLE const hAacEnc,
    int* const absoluteMaxNumExternalBits,
    int* const comfortableMaxNumExternalBits,
    int const bUsacIndepFlag,
    AACENC_IPF_STATE const ipfState);

#ifdef WIN32
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
