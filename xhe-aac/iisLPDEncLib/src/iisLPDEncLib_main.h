
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

#ifndef IISLPDENCLIB_MAIN_H
#define IISLPDENCLIB_MAIN_H

#include <stdio.h>

#include "options.h"
#include "iisutillib.h"
#include "iis_fft.h"
#include "iisLPDComLib_constants.h"

#include "iisLPDEncLib_tcx_Tools.h"
#include "iisLPDComLib_tcx_Transform.h"
#include "iisLPDEncLib_arithEncWrapper.h"
#include "iisLPDEncLib.h"

#define IIS_LPD_ENC_MAX_SUPERFRAME_SAVE_PITCH_GAIN 5
#define IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN IIS_LPD_ENC_MAX_SUPERFRAME_SAVE_PITCH_GAIN *NB_DIV * 4

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

typedef struct enc_mode_calc_lpd_struct {
  int emergencyRestrictedMode;
  float EmergencyForceTCXLevel;
  int acelpCoreMode;
  int numberMoreBitsUsedThanBitRes;
  int isAceStart;
} ENC_MODE_CALC_LPD_DATA, *ENC_MODE_CALC_LPD_DATA_HANDLE;

typedef struct
{
  int mode;
  int nbits;
  float fac_ns;

  float Aq[2 * (M + 1)];
  float Ai[2 * (M + 1)];
  float syn[M + 128];
  float wsyn[1 + 128];

  float Aexc[PIT_MAX_MAX + L_INTERPOL];

  int RE8prm[LFAC_1024];

  float Txn[128];
  float Txnq[1 + (2 * 128)];
  float Txnq_fac;

  int enhancedPulseSearch;
} LPD_state;

typedef struct {
  LPD_state LPDmem;
  float old_d_wsp[PIT_MAX_MAX / OPL_DECIM];
  float lsfold[M];
  float lspold[M];
  float lspold_q[M];
  float mem_wsp;
  float mem_lp_decim2[3];
  float ada_w;
  int ol_wght_flg;
  int old_ol_lag[5];
  int old_T0_med;
  float hp_old_wsp[L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM)];
  float hp_ol_ltp_mem[3 * 2 + 1];
  LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE phArith;
  int restrictedMode;

  float fdSynth[2 * L_DIV_1024 + 1 + M];
  float fdOrig[2 * L_DIV_1024 + 1 + M];

  float oldPitchGain[IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN];
  int pOldPitchGainCircle;

} SAVE_ENC_STATE_FOR_RECALC_FRAME;

typedef struct Coder_State_Plus {
  int lFrame;
  int lDiv;
  int nbSubfr;
  short nbDiv;

  LPD_CHANNEL_STREAM lpd_channel_stream;

  float old_speech_pe[L_OLD_SPEECH_HIGH_RATE + L_LPC0_1024];
  int fscale;

  float bitResFillLevel;
  short acelp_core_mode_nominal;

  float mem_sig_in[4];
  float mem_preemph;

  LPD_state LPDmem;
  float old_d_wsp[PIT_MAX_MAX / OPL_DECIM];

  float lsfold[M];
  float lspold[M];
  float lspold_q[M];
  float mem_wsp;
  float mem_lp_decim2[3];

  float ada_w;
  int ol_wght_flg;
  int old_ol_lag[5];
  int old_T0_med;
  float hp_old_wsp[L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM)];
  float hp_ol_ltp_mem[3 * 2 + 1];

  float window[L_WINDOW_HIGH_RATE_1024];

  short prev_mod;

  HANDLE_TCX_MDCT hTcxMdct;

  HANDLE_IIS_FFT hTcxFft;

  LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE phArith;

  int restrictedMode;
  float TCXLevel;

  float fdSynth[PIT_MAX_MAX + L_INTERPOL + M];
  float fdOrig[2 * L_DIV_1024 + 1 + M];
  int lowpassLine;

  int tcx_arith_reset_flag[NB_DIV];
  int lastWasShort;
  int nextIsShort;

  ENC_MODE_CALC_LPD_DATA calcMode;
  SAVE_ENC_STATE_FOR_RECALC_FRAME saveFrameState;

  float oldPitchGain[IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN];
  int pOldPitchGainCircle;

  int nIpfReducePitchGain;
  int isVbr;

} Coder_State_Plus;

HANDLE_ERROR_INFO LPDEnc_main_Open(Coder_State_Plus *st,
                                   int fullbandLpd);

HANDLE_ERROR_INFO LPDEnc_main_Config(Coder_State_Plus *st,
                                     short full_reset,
                                     int L_frame,
                                     int fscale,
                                     int bitrate,
                                     int optimizedSpeedPulseSearch,
                                     int acelpModeIndex,
                                     LPD_CODING_MODE codingMode);

void LPDEnc_main_Close(Coder_State_Plus *st);
void LPDEnc_main_Reset(Coder_State_Plus *st, int lastWasShort);

int LPDEnc_main_PrepLABuffer(
    float channel_right[],
    int L_next,
    Coder_State_Plus *st);

HANDLE_ERROR_INFO LPDEnc_main_Encode(
    float channel_right[],
    unsigned char serial[],
    unsigned char serialFac[],
    int *nBitsFac,
    Coder_State_Plus *st,
    int isAceStart,
    int *total_nbbits,
    int *mode_out,
    int const bUsacIndependencyFlag,
    int const bUseNoiseFilling,
    int const bCommonWindowMode,
    int lpdBitRate,
    int maxBitsToUse,
    const LPD_IPF_STATE ipfState,
    LPD_CHANNEL_STREAM *lpd_channel_stream);

void getMinArithEncBits(
    int *mod,
    int *minSqBits);

#endif
