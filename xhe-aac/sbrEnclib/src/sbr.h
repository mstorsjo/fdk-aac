
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

#ifndef SBR_H
#define SBR_H

#include "tran_det.h"
#include "fram_gen.h"
#include "nf_est.h"
#include "invf_est.h"
#include "env_est.h"
#include "code_env.h"
#include "sbr_main.h"
#include "ton_corr.h"
#include "sbr_def.h"
#include "qmflib.h"

#if defined __GNUC__ || defined __clang__
#define SBR_WITH_LD __attribute__((unused))
#else
#define SBR_WITH_LD
#endif

#include "sib_det.h"

struct SBR_BITSTREAM_DATA {
  int TotalBits;
  int PayloadBits;
  int FillBits;
  int HeaderActive;
  int CRCActive;
  int rightBorderFIX;
};

typedef struct SBR_BITSTREAM_DATA *HANDLE_SBR_BITSTREAM_DATA;

struct SBR_HEADER_DATA {
  AMP_RES sbr_amp_res;
  int sbr_start_frequency;
  int sbr_stop_frequency;
  int sbr_xover_band;
  int sbr_noise_bands;
  int sbr_data_extra;
  int header_extra_1;
  int header_extra_2;
  int sbr_lc_stereo_mode;
  int sbr_limiter_bands;
  int sbr_limiter_gains;
  int sbr_interpol_freq;
  int sbr_smoothing_length;
  int alterScale;
  int freqScale;

  SR_MODE sampleRateMode;

  int coupling;
  int prev_coupling;
};

typedef struct SBR_HEADER_DATA *HANDLE_SBR_HEADER_DATA;

struct SBR_CONFIG_DATA {
  CODEC_TYPE coreCodec;

  int allowSwitch;

  int bitRate;
  int nChannelsInput;
  int nChannels;

  int nSfb[2];
  int num_Master;
  int sampleFreq;
  int frameSize;
  int inputSampleFreq;
  int inputFrameSize;
  float xOverFreq;
  float dynXOverFreq;

  int *freqBandTable[2];
  int *v_k_master;

  int sbrDelayEnc;
  int sbrDelayDec;

  SBR_STEREO_MODE stereoMode;

  int useWaveCoding;
  int useParametricCoding;
  int xposCtrlSwitch;
  int switchTransposers;

  int bMonoStereoScal;

  int thresholdAmpResFF;
  int bUseQmfInput;

  int overrideTimeDiffCoding;

  AMP_RES initAmpResFF;
  float noiseLevel;
  int bReducedCoreCoderFrameLength;
  int bSbr41;
  int bs_interTes;
  int sibilantTuning;
  int switchingDecision;

  int downScaleFactor;
};

typedef struct SBR_CONFIG_DATA *HANDLE_SBR_CONFIG_DATA;

struct SBR_ENV_DATA {
  int sbr_xpos_ctrl;
  FREQ_RES freq_res_fixfix[2];

  INVF_MODE sbr_invf_mode;
  INVF_MODE sbr_invf_mode_vec[MAX_NUM_NOISE_VALUES];

  XPOS_MODE sbr_xpos_mode;

  int ienvelope[MAX_ENVELOPES][MAX_FREQ_COEFFS];

  int codeBookScfLavBalance;
  int codeBookScfLav;
  const int *hufftableTimeC;
  const int *hufftableFreqC;
  const int *hufftableTimeL;
  const int *hufftableFreqL;

  const int *hufftableLevelTimeC;
  const int *hufftableBalanceTimeC;
  const int *hufftableLevelFreqC;
  const int *hufftableBalanceFreqC;
  const int *hufftableLevelTimeL;
  const int *hufftableBalanceTimeL;
  const int *hufftableLevelFreqL;
  const int *hufftableBalanceFreqL;

  const int *hufftableNoiseTimeL;
  const int *hufftableNoiseTimeC;
  const int *hufftableNoiseFreqL;
  const int *hufftableNoiseFreqC;

  const int *hufftableNoiseLevelTimeL;
  const int *hufftableNoiseLevelTimeC;
  const int *hufftableNoiseBalanceTimeL;
  const int *hufftableNoiseBalanceTimeC;
  const int *hufftableNoiseLevelFreqL;
  const int *hufftableNoiseLevelFreqC;
  const int *hufftableNoiseBalanceFreqL;
  const int *hufftableNoiseBalanceFreqC;

  HANDLE_SBR_GRID hSbrBSGrid;

  int noHarmonics;
  int addHarmonicFlag;
  int addHarmonic[MAX_FREQ_COEFFS];
  int bsSinusoidalPosition;

  int si_sbr_start_env_bits_balance;
  int si_sbr_start_env_bits;
  int si_sbr_start_noise_bits_balance;
  int si_sbr_start_noise_bits;

  int noOfEnvelopes;
  int noScfBands[MAX_ENVELOPES];
  int domain_vec[MAX_ENVELOPES];
  int domain_vec_noise[MAX_ENVELOPES];
  int sbr_noise_levels[MAX_NUM_NOISE_VALUES];
  int noOfnoisebands;

  int balance;
  AMP_RES init_sbr_amp_res;
  AMP_RES currentAmpResFF;
  int ton_HF[SBR_GLOBAL_TONALITY_VALUES];
  int global_tonality;

  int extended_data;
  int extension_size;
  int extension_id;
  unsigned char extended_data_buffer[SBR_EXTENDED_DATA_MAX_CNT];

  int bs_temp_shape[MAX_ENVELOPES];
  int bs_temp_shape_mode_bits[MAX_ENVELOPES];
  int bs_temp_shape_mode_value[MAX_ENVELOPES];

  int sbrPatchingMode;

  int oversamplingFlag;
  int pitchInBins;
};

typedef struct SBR_ENV_DATA *HANDLE_SBR_ENV_DATA;

struct ENV_CHANNEL {
  HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector;
  HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope;
  HANDLE_SBR_CODE_ENVELOPE h_sbrCodeNoiseFloor;
  HANDLE_SBR_EXTRACT_ENVELOPE h_sbrExtractEnvelope;
  float *pAnaTimeBufTmp;
  HANDLE_QMFLIB_ANALYSIS h_sbrQmf;
  HANDLE_QMFLIB_SYNTHESIS h_sbrSynthQmf;
  HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame;
  HANDLE_SBR_TON_CORR_EST hTonCorr;

  HANDLE_QMFLIB_ANALYSIS h_sbrPatchQmf;
  HANDLE_SBR_SIBILANT_DETECTOR h_sbrSibilantDetector;
  int no_channels;
  int no_channels_syn;
  int no_col;

  struct SBR_ENV_DATA encEnvData;
};

typedef struct ENV_CHANNEL *HANDLE_ENV_CHANNEL;

float **
GetQmfRealBufferRead(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfImagBufferRead(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfRealBufferWrite(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfImagBufferWrite(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

HANDLE_QMFLIB_ANALYSIS
GetSbrQmfHandle(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

HANDLE_QMFLIB_SYNTHESIS
GetSbrSynthQmfHandle(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfRealBufferReadPV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfImagBufferReadPV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfRealBufferWritePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

float **
GetQmfImagBufferWritePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

HANDLE_QMFLIB_ANALYSIS
GetSbrQmfHandlePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

int GetEnvDownsamplerDelay(HANDLE_ENV_CHANNEL *hEnvChannel, int ch);

#endif
