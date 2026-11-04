
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

#ifndef SBR_MAIN_H
#define SBR_MAIN_H

#include "iisutillib.h"
#include "bit_buf.h"

#define LOW_BITRATE_TUNING_LIMIT 8000

#define MAX_NUM_NOISE_COEFFS 5
#define MAX_NUM_CHANNELS 2
#define MAX_SBR_BITBUF_SIZE 4096

#define MAX_TRANS_FAC 8
#define MAX_CODEC_FRAME_RATIO 2

#define SEND_NEVER_SBR_HEADER -99.0
#define SEND_ONCE_SBR_HEADER -1.0

typedef enum codecType {
  CODEC_SAAC = 1,
  CODEC_UNSPECIFIED = 99
} CODEC_TYPE;

typedef struct
{
  int bitRate;
  int nChannelsInput;
  int nChannels;
  int sampleFreq;
  int transFac;
  int standardBitrate;
  int frameSize;
  CODEC_TYPE coreCodec;
} CODEC_PARAM;

typedef enum {
  SBR_MONO,
  SBR_LEFT_RIGHT,
  SBR_COUPLING,
  SBR_SWITCH_LRC
} SBR_STEREO_MODE;

typedef enum {
  SINGLE_RATE = 0,
  DUAL_RATE,
  QUAD_RATE
} SR_MODE;

typedef enum {
  FREQ_RES_LOW = 0,
  FREQ_RES_HIGH
} FREQ_RES;

typedef enum {
  SBR_AMP_RES_1_5 = 0,
  SBR_AMP_RES_3_0
} AMP_RES;

typedef enum {
  XPOS_MDCT,
  XPOS_MDCT_CROSS,
  XPOS_LC,
  XPOS_RESERVED,
  XPOS_SWITCHED
} XPOS_MODE;

typedef enum {
  INVF_OFF = 0,
  INVF_LOW_LEVEL,
  INVF_MID_LEVEL,
  INVF_HIGH_LEVEL,
  INVF_SWITCHED
} INVF_MODE;

typedef enum {
  SBR_NOT_SCALABLE = 0,
  SBR_MONO_BASE = 1,
  SBR_STEREO_ENHANCE = 2,
  SBR_STEREO_BASE = 3,
  SBR_NO_DATA = 99
} SBR_LAYER;

typedef enum {
  SBR_CHANNELMODE_INVALID = 0,
  SBR_CHANNELMODE_MONO = 1,
  SBR_CHANNELMODE_STEREO = 2,
  SBR_CHANNELMODE_MPS212 = 3,
  SBR_CHANNELMODE_PS = 4
} SBR_CHANNELMODE;

typedef struct sbrConfiguration {
  CODEC_PARAM codecSettings;
  float SendHeaderDataTime;
  float SendHeaderDataOffs;
  int useWaveCoding;
  int crcSbr;
  int dynBwSupported;
  int detectMissingHarmonics;
  int parametricCoding;
  FREQ_RES freq_res_fixfix[2];
  float threshold_amp_res_FF;

  int downScaleFactor;

  float tranDetNewThresh;
  float tranDetNewAver;
  int noiseFloorOffset[MAX_NUM_NOISE_COEFFS];
  unsigned int useSpeechConfig;

  SR_MODE srMode;
  AMP_RES amp_res;
  int switch_amp_res_FF;
  int ana_max_level;
  int tran_fc;
  int tran_det_mode;
  int spread;
  int stat;
  int e;
  FREQ_RES freqResFillPre;
  FREQ_RES freqResFillPost;
  SBR_STEREO_MODE stereoMode;
  int deltaTAcrossFrames;
  float dF_edge_1stEnv;
  float dF_edge_incr;
  INVF_MODE sbr_invf_mode;
  XPOS_MODE sbr_xpos_mode;
  int sbr_xpos_ctrl;
  int sbr_xpos_level;
  int startFreq;
  int stopFreq;
  int stopFreqChanged;

  int bParametricStereo;

  int dynBwEnabled;

  int bMonoStereoScal;

  int bDownSampledSbr;

  int bUseQmfInput;

  int freqScale;
  int alterScale;
  int sbr_noise_bands;

  int sbr_limiter_bands;
  int sbr_limiter_gains;
  int sbr_interpol_freq;
  int sbr_smoothing_length;

  float noiseLevel;
  int useHBE;

  int timeInputStride;

  int bs_interTes;

  int bReducedCoreCoderFrameLength;
  int bSbr41;
  int usePVC;

  int sibilantTuning;
} sbrConfiguration, *sbrConfigurationPtr;

typedef struct {
  char versionNo[20];
} SbrEncLibInfo;

struct SBR_BIT_DATA {
  int totalBits;
  int fillBits;
  int sbrPayload;

  int sbrByteCnt;
  int cntEscValue;
  int sbrExtByteCnt;
};

typedef struct SBR_BIT_DATA SBR_BIT_DATA;
typedef struct SBR_BIT_DATA *HANDLE_SBR_BIT_DATA;

void SbrEncGetLibInfo(SbrEncLibInfo *libInfo);

unsigned int
IsProSettingAvail(CODEC_TYPE coreCoder,
                  unsigned int bitrate,
                  unsigned int numOutputChannels,
                  unsigned int sampleRateInput,
                  int bNoCoupling,
                  unsigned int *sampleRateCore,
                  int bDownSampledSbr);

HANDLE_ERROR_INFO
AdjustSbrSettings(const sbrConfigurationPtr config,
                  unsigned int bitRate,
                  SBR_CHANNELMODE sbrChannelmode,
                  unsigned int fsCore,
                  unsigned int transFac,
                  unsigned int standardBitrate,
                  unsigned int vbrMode,
                  unsigned int useSpeechConfig,
                  int bNoCoupling,
                  SR_MODE srMode,
                  int bDownSampledSbr,
                  int bParametricStereo,
                  int bUseQmfInput,
                  int userStartFreq,
                  int userStopFreq);

HANDLE_ERROR_INFO
InitializeSbrDefaults(sbrConfigurationPtr config,
                      CODEC_TYPE coreCoder,
                      int coreSbrFrameLenFac,
                      unsigned int codecGranuleLen);

struct SBR_ENCODER {
  struct SBR_CONFIG_DATA *sbrConfigData;
  struct SBR_HEADER_DATA *sbrHeaderData;
  struct SBR_BITSTREAM_DATA *sbrBitstreamData;
  struct ENV_CHANNEL *hEnvChannel[MAX_NUM_CHANNELS];
  struct INTERVALTIMER *hHdrTimer;

  void (*GetEnergyFromCplxQmfData_Ptr)(float **energyValues, float **realValues, float **imagValues, int numberBands, int numberCols);

  float *downsampledOutSignal;
  int nSamplesDownsampledOutSignal;
  int nDelayDownsampler;

  float dynXOverFreqDelay[5];

  int timeInputStride;
  int nChannelsQmf;
  int nChannelsSbr;
};
typedef struct SBR_ENCODER *HANDLE_SBR_ENCODER;

struct COMMON_DATA;

typedef struct {
  int start_freq;
  int stop_freq;
  int header_extra1;
  int header_extra2;
  int freq_scale;
  int alter_scale;
  int noise_bands;
  int limiter_bands;
  int limiter_gains;
  int interpol_freq;
  int smoothing_mode;

} SBR_USAC_HEADER_DATA;

HANDLE_ERROR_INFO
EnvOpen(HANDLE_SBR_ENCODER *hEnvEncoder,
        sbrConfigurationPtr params,
        struct COMMON_DATA *hCmonData,
        CODEC_TYPE coreCoder);

void EnvClose(HANDLE_SBR_ENCODER hEnvEnc);

float SbrGetStopFreqRaw(HANDLE_SBR_ENCODER hEnv);

HANDLE_ERROR_INFO
SbrSetStopFreqRaw(HANDLE_SBR_ENCODER hEnv,
                  float *stopFreqRaw,
                  sbrConfigurationPtr params,
                  CODEC_TYPE coreCoder);

HANDLE_ERROR_INFO
SbrSetManualNoiseLevelOverride(HANDLE_SBR_ENCODER hEnvEncoder,
                               float noiseLevel);

int SbrStopFreqChanged(sbrConfigurationPtr params);

int SbrGetMaxAllowedStopFreq(int startFreq, int fs, CODEC_TYPE coreCoder, SR_MODE srMode);

unsigned int SbrGetDelayEnc(struct SBR_CONFIG_DATA *cfg);
unsigned int SbrGetDelayDec(struct SBR_CONFIG_DATA *cfg);

HANDLE_ERROR_INFO
EnvEncodeFrame(HANDLE_SBR_ENCODER hEnvEncoder,
               struct COMMON_DATA *hCmonData,
               const float *const pTimeDomainSamples,
               const float *const *const ppQmfSamplesReal,
               const float *const *const ppQmfSamplesImag,
               const int switchingDecision,
               const int bUsacIndependenceFlag);

struct bitbuffer;
struct CRC_INFO;
struct SBR_HEADER_DATA;

int WriteSbrExtensionData(struct bitbuffer *hBitStream,
                          struct COMMON_DATA *hCmonData,
                          SBR_LAYER sbrLayer,
                          int bCrcFlag,

                          int bWriteFillBits,

                          int nBitsExtensionType,
                          int byteAlignment,
                          CODEC_TYPE coreCodec);

void AppendSbrBitstream(HANDLE_SBR_BIT_DATA hSbrBit,
                        struct bitbuffer *hBitStream,
                        struct COMMON_DATA *hCmonData,
                        CODEC_TYPE coreCoder);

int SbrHeaderNPayload(struct COMMON_DATA *hCmonData);

#define AAC_SBR_CRC_POLY (0x0233)
#define AAC_SBR_CRC_MASK (0x0200)
#define AAC_SBR_CRC_MAXREGS 1
#define AAC_SBR_CRCINIT (0x0)

#define AAC_SI_FIL_SBR 13
#define AAC_SI_FIL_SBR_CRC 14

int SbrCountBits(HANDLE_SBR_BIT_DATA hSbrBitData,
                 struct COMMON_DATA *hCmonData,
                 CODEC_TYPE coreCoder);

int CreateSbrBitInfo(HANDLE_SBR_BIT_DATA *pHandleSbrBit);

void DeleteSbrBitInfo(HANDLE_SBR_BIT_DATA *pHandleSbrBit);

unsigned int
SbrGetDownsampledOutSignal(HANDLE_SBR_ENCODER hEnv, float **samplesOut);

int SbrGetDownsamplerDelay(HANDLE_SBR_ENCODER hEnv);

int SbrGetEstimateBitrate(sbrConfigurationPtr pSbrConfig);

int SbrGetSbrPresent(HANDLE_SBR_ENCODER hEnv);

int SbrGetPsPresent(HANDLE_SBR_ENCODER hEnv);

void SbrSetAllowPatchingSwitch(HANDLE_SBR_ENCODER hEnv, int bAllowSwitch);

HANDLE_ERROR_INFO
SbrSetRightBorderFIX(HANDLE_SBR_ENCODER hEnv, int value);

HANDLE_ERROR_INFO
SbrGetUsacDfltHeader(HANDLE_SBR_ENCODER hEnv, SBR_USAC_HEADER_DATA *pSbrUsacHeaderData);
int SbrSendSbrHeader(HANDLE_SBR_ENCODER hEnv);

int SbrSAPFramePrepare(HANDLE_SBR_ENCODER hEnv);

int SbrSetTimeDiffCodingFlag(HANDLE_SBR_ENCODER hEnv, int flag);

int SbrSetHeaderSendInterval(HANDLE_SBR_ENCODER hEnv,
                             sbrConfigurationPtr params);
int SbrSetCrcProtection(HANDLE_SBR_ENCODER hEnv,
                        sbrConfigurationPtr params);

int GetSbrStartFreqRAW(int startFreq,
                       int fs,
                       CODEC_TYPE coreCodec,
                       SR_MODE srMode);

int GetSbrStopFreqRAW(int stopFreq,
                      int fs,
                      CODEC_TYPE coreCodec,
                      SR_MODE srMode);

#endif
