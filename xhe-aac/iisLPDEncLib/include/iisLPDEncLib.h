
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

#ifndef IISLPDENCLIB_H
#define IISLPDENCLIB_H

#include "iisutillib.h"
#include "iisLPDComLib_constants.h"
#include "options.h"
#include "iisArithEncoder.h"

#define NUM_TBE_BITS_HR (39)

#ifndef LPD_IPF_STATE_DEFINED
#define LPD_IPF_STATE_DEFINED
typedef enum lpd_ipf_state {
  LPD_IPF_STATE_NO,
  LPD_IPF_STATE_RAP_FIRST_PREROLL,
  LPD_IPF_STATE_RAP_NEXT_PREROLL,
  LPD_IPF_STATE_RAP_IPF,
  LPD_IPF_STATE_RAP_IPF_PREROLL,
  LPD_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL,
  LPD_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL,
  LPD_IPF_STATE_CONFIGCHANGE_IPF
} LPD_IPF_STATE;
#endif

#ifndef LPD_CODING_MODE_DEFINED
#define LPD_CODING_MODE_DEFINED
typedef enum lpd_coding_mode {
  LPD_CODING_MODE_INVALID = -1,
  LPD_CODING_MODE_SWITCHED = 0,
  LPD_CODING_MODE_ACELP = 1,
  LPD_CODING_MODE_TCX = 2
} LPD_CODING_MODE;
#endif

typedef enum transition_directon {
  LPDENC_TRANSITION_UNDEFINED,
  LPDENC_TRANSITION_FD2LPD,
  LPDENC_TRANSITION_LPD2FD,
  LPDENC_TRANSITION_LPD2FD2LPD
} LPDENC_TRANSITION_TYPE;

typedef struct EncoderConfig {
  short mode;
  int fscale;
  short FileFormat;
  short bc;

  LPD_CODING_MODE codingMode;

  int totalBitRate;
  int isVbr;
  int lpdChannelBitRate;
  int L_frame;
  int L_next;
  int L_next_st;
  int reduced_bitstream;
  short mode_index_nominal;
  int optimizedSpeedPulseSearch;
} LPDENCLIB_CONFIG;

typedef struct lpd_enc_private_data_struct* LPD_ENC_PRIVATE_DATA_HANDLE;
typedef struct lpd_enc_public_data_struct {
  unsigned char outStream[4 * NBITS_MAX];
  int outputStreamLenBits;
  int nOutFrames;

  int useLpd;
  int wasReset;
  int modes[NB_DIV];

  unsigned char facData[4 * NBITS_MAX];
  int nBitsFac;
  int totalBitRate;

  LPD_ENC_PRIVATE_DATA_HANDLE hPrivateData;
} LPD_ENC_PUBLIC_DATA, *LPD_ENC_PUBLIC_DATA_HANDLE;

typedef struct lpd_channel_stream {
  int acelp_core_mode;
  int lpd_mode;
  int bpf_control_info;
  int core_mode_last;
  int fac_data_present;
  int fac_data[NB_DIV][LFAC_1024];
  int acelp_mean_energy[NB_DIV];
  int acelp_acb_index[NB_DIV][NB_SUBFR_1024];
  int acelp_ltp_filtering[NB_DIV][NB_SUBFR_1024];
  int acelp_icb_index[NB_DIV][NB_SUBFR_1024][8];
  int acelp_gains[NB_DIV][NB_SUBFR_1024];
  int tcx_noise_factor[NB_DIV];
  int tcx_global_gain[NB_DIV];
  int tcx_force_arith_reset[NB_DIV];
  int tcx_quant[L_FRAME_1024];
  int lpc_params[NPRM_LPC_NEW];
} LPD_CHANNEL_STREAM;

HANDLE_ERROR_INFO iisLPDEncLib_Open(
    LPD_ENC_PUBLIC_DATA_HANDLE* phPublicData,
    const int fullbandLpd,
    const int stereoLpdActive);

HANDLE_ERROR_INFO iisLPDEncLib_Config(
    LPD_ENC_PUBLIC_DATA_HANDLE hPublicData,
    const LPDENCLIB_CONFIG* hInitData);

LPD_ENC_PUBLIC_DATA_HANDLE iisLPDEncLib_Close(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData);

HANDLE_ERROR_INFO iisLPDEncLib_EncodeFrame(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    float const* const pTimeInput,
    float const* const pAMRPast,
    int isAceStart,
    int const bUsacIndependencyFlag,
    int const bUsacNoiseFilling,
    ARIENC_PUBLIC_DATA_HANDLE const hArithEnc,
    int maxBitsToUse,
    LPD_IPF_STATE const ipfState);

int iisLPDEncLib_GetLastSubfrWasACELP(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData);

int iisLPDEncLib_UpdateBitRes(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int bitReservoir,
    int bitReservoirMax);

void iisLPDEncLib_SetFdOverlapLen(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int lastWasShort,
    int nextIsShort);

void iisLPDEncLib_SetRestrictedMode(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int mode);

HANDLE_ERROR_INFO iisLPDEncLib_StereoBitstreamMUX(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    unsigned char* lpdStereoBitstream,
    int* lpdStereoBitCount);

HANDLE_ERROR_INFO iisLPDEncLib_StereoDMXInput(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    int offset,
    const float** pDMX);

HANDLE_ERROR_INFO iisLPDEncLib_StereoParamCoding(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData,
    float* pInput_buffer_L,
    float* pInput_buffer_R,
    int fFirstFrame,
    const int disableStereoLpd);

HANDLE_ERROR_INFO iisLPDEncLib_StereoResidualCoding(
    LPD_ENC_PUBLIC_DATA_HANDLE const hPublicData);

HANDLE_ERROR_INFO iisLPDEncLib_FdLpdTransition(
    const LPD_ENC_PUBLIC_DATA_HANDLE hPublicData,
    int ch,
    float* origTimeSig,
    float* synthTime,
    int nGranuleLength,
    int lowpassLine,
    int lfac,
    LPDENC_TRANSITION_TYPE transitionType,
    char* facPrm,
    int* Nbits_fac);

#endif
