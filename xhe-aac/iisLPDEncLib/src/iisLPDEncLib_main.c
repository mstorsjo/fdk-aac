
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

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iisLPDComLib_tools.h"
#include "iisLPDEncLib_bs.h"
#include "iisLPDEncLib_main.h"
#include "iisLPDEncLib_closedLoop.h"
#include "mathlib.h"
#include "iisLPDEncLib_cmConfig.h"
#include "iisLPDEncLib_bpf.h"
#include "iisLPDComLib_lpc.h"

static void arithResetFlag(
    Coder_State_Plus *st,
    const LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE phArith,
    int const bUsacIndependencyFlag,
    int const nbDiv,
    int mod[]);

static void getLpdMode(
    int mod[],
    LPD_CHANNEL_STREAM *lpd_channel_stream);

static HANDLE_ERROR_INFO LPDEnc_main_SaveFrameState(Coder_State_Plus *st) {
  SAVE_ENC_STATE_FOR_RECALC_FRAME *saveFrameState = NULL;
  HANDLE_ERROR_INFO err = noError;

  if (st == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer");
  }
  saveFrameState = &st->saveFrameState;

  saveFrameState->LPDmem = st->LPDmem;
  memcpy(&(saveFrameState->old_d_wsp), &(st->old_d_wsp), (PIT_MAX_MAX / OPL_DECIM) * sizeof(float));
  memcpy(&(saveFrameState->lsfold), &(st->lsfold), M * sizeof(float));
  memcpy(&(saveFrameState->lspold), &(st->lspold), M * sizeof(float));
  memcpy(&(saveFrameState->lspold_q), &(st->lspold_q), M * sizeof(float));
  saveFrameState->mem_wsp = st->mem_wsp;
  memcpy(&(saveFrameState->mem_lp_decim2), &(st->mem_lp_decim2), 3 * sizeof(float));
  saveFrameState->ada_w = st->ada_w;
  saveFrameState->ol_wght_flg = st->ol_wght_flg;
  memcpy(&(saveFrameState->old_ol_lag), &(st->old_ol_lag), 5 * sizeof(int));
  saveFrameState->old_T0_med = st->old_T0_med;
  memcpy(&(saveFrameState->hp_old_wsp), &(st->hp_old_wsp), (L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM)) * sizeof(float));
  memcpy(&(saveFrameState->hp_ol_ltp_mem), &(st->hp_ol_ltp_mem), (3 * 2 + 1) * sizeof(float));
  saveFrameState->restrictedMode = st->restrictedMode;
  memcpy(&(saveFrameState->fdSynth), &(st->fdSynth), (PIT_MAX_MAX + L_INTERPOL + M) * sizeof(float));
  memcpy(&(saveFrameState->fdOrig), &(st->fdOrig), (2 * L_DIV_1024 + 1 + M) * sizeof(float));

  SAFECALL(err, LPDEnc_arithEncWrapper_Copy(st->phArith, &(saveFrameState->phArith)));

  memcpy(&(saveFrameState->oldPitchGain), &(st->oldPitchGain), IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN * sizeof(float));
  saveFrameState->pOldPitchGainCircle = st->pOldPitchGainCircle;

  return err;
}

static HANDLE_ERROR_INFO LPDEnc_main_ResetFrameState(Coder_State_Plus *st) {
  SAVE_ENC_STATE_FOR_RECALC_FRAME *saveFrameState = NULL;
  HANDLE_ERROR_INFO err = noError;

  if (st == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer");
  }
  saveFrameState = &st->saveFrameState;

  st->LPDmem = saveFrameState->LPDmem;
  memcpy(&(st->old_d_wsp), &(saveFrameState->old_d_wsp), (PIT_MAX_MAX / OPL_DECIM) * sizeof(float));
  memcpy(&(st->lsfold), &(saveFrameState->lsfold), M * sizeof(float));
  memcpy(&(st->lspold), &(saveFrameState->lspold), M * sizeof(float));
  memcpy(&(st->lspold_q), &(saveFrameState->lspold_q), M * sizeof(float));
  st->mem_wsp = saveFrameState->mem_wsp;
  memcpy(&(st->mem_lp_decim2), &(saveFrameState->mem_lp_decim2), 3 * sizeof(float));
  st->ada_w = saveFrameState->ada_w;
  st->ol_wght_flg = saveFrameState->ol_wght_flg;
  memcpy(&(st->old_ol_lag), &(saveFrameState->old_ol_lag), 5 * sizeof(int));
  st->old_T0_med = saveFrameState->old_T0_med;
  memcpy(&(st->hp_old_wsp), &(saveFrameState->hp_old_wsp), (L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM)) * sizeof(float));
  memcpy(&(st->hp_ol_ltp_mem), &(saveFrameState->hp_ol_ltp_mem), (3 * 2 + 1) * sizeof(float));
  st->restrictedMode = saveFrameState->restrictedMode;
  memcpy(&(st->fdSynth), &(saveFrameState->fdSynth), (PIT_MAX_MAX + L_INTERPOL + M) * sizeof(float));
  memcpy(&(st->fdOrig), &(saveFrameState->fdOrig), (2 * L_DIV_1024 + 1 + M) * sizeof(float));

  SAFECALL(err, LPDEnc_arithEncWrapper_Copy(saveFrameState->phArith, &(st->phArith)));

  return err;
}

HANDLE_ERROR_INFO LPDEnc_main_Open(Coder_State_Plus *st,
                                   int fullbandLpd) {
  if (st == NULL) {
    return iisUtil_ERROR(CDI, "Invalid pointer");
  }

  (void)fullbandLpd;

  LPDEnc_arithEncWrapper_Open(&(st->phArith));

  return noError;
}

HANDLE_ERROR_INFO LPDEnc_main_Config(Coder_State_Plus *st,
                                     short full_reset,
                                     int L_frame,
                                     int fscale,
                                     int bitrate,
                                     int optimizedSpeedPulseSearch,
                                     int acelpModeIndex,
                                     LPD_CODING_MODE codingMode) {
  int lWindow;
  (void)bitrate;
  (void)codingMode;

  st->fscale = fscale;
  st->LPDmem.enhancedPulseSearch = optimizedSpeedPulseSearch;
  st->saveFrameState.phArith = NULL;

  if (acelpModeIndex < MIN_ACELP_COREMODE || acelpModeIndex > MAX_ACELP_COREMODE) {
    st->acelp_core_mode_nominal = -1;
  } else {
    st->acelp_core_mode_nominal = acelpModeIndex;
  }
  st->lpd_channel_stream.acelp_core_mode = st->acelp_core_mode_nominal;
  LPDEnc_cmConfig_Reset(&(st->calcMode));

  st->nbDiv = NB_DIV;

  st->lFrame = L_frame;
  st->lDiv = L_frame / st->nbDiv;
  st->nbSubfr = (NB_SUBFR_1024 * st->lDiv) / L_DIV_1024;

  if (full_reset) {
    st->mem_preemph = 0.0;

    setFLOAT(0.0f, st->mem_sig_in, 4);
    LPDEnc_closedLoop_Config(st);

    st->prev_mod = -1;
  }

  if (st->fscale <= FSCALE_DENOM) {
    lWindow = (L_WINDOW_1024 * st->lDiv) / L_DIV_1024;
    ;
  } else {
    lWindow = (L_WINDOW_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024;
    ;
  }

  LPDCom_lpc_CreateAutocorrWindow(st->window, lWindow / 2, lWindow / 2);

  st->hTcxFft = NULL;
  IIS_CFFT_Create(&st->hTcxFft, 2 * (FDNS_NPTS_1024 * st->lDiv) / L_DIV_1024, -1);

  st->restrictedMode = 15;

  st->bitResFillLevel = 0;

  return noError;
}

void LPDEnc_main_Close(Coder_State_Plus *st) {
  st->phArith = LPDEnc_arithEncWrapper_Close(st->phArith);

  LPDEnc_closedLoop_Close(st);
}

void LPDEnc_main_Reset(Coder_State_Plus *st, int lastWasShort) {
  setFLOAT(0.0f, st->old_speech_pe, M + (L_NEXT_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024);

  LPDEnc_closedLoop_Reset(st);

  st->prev_mod = lastWasShort ? (-2) : (-1);

  return;
}

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
    LPD_CHANNEL_STREAM *lpd_channel_stream) {
  float old_speech[L_TOTAL_HIGH_RATE + L_LPC0_1024];
  float *speech, *new_speech;

  int i;
  int mod_buf[1 + NB_DIV], *mod;

  float ol_gain[NB_DIV];

  int n_param_tcx[NB_DIV];
  float pitchLags[16];
  float pitchGains[16];
  HANDLE_ERROR_INFO err = noError;
  int bitsToUse;
  int sqBits = 0;
  int minSqBits = 0;
  float TCXLevel_old = 0;

  (void)bCommonWindowMode;
  memset(lpd_channel_stream->lpc_params, 0, NPRM_LPC_NEW * sizeof(int));

  mod = mod_buf + 1;
  mod[-1] = st->prev_mod;

  new_speech = old_speech + M + (L_NEXT_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024;
  speech = old_speech + M;

  if (isAceStart) {
    new_speech += (L_LPC0_1024 * st->lDiv) / L_DIV_1024;
    speech += (L_LPC0_1024 * st->lDiv) / L_DIV_1024;
  }

  copyFLOAT(channel_right, new_speech, st->lFrame);

  LPDEnc_tools_HighPass50Hz(new_speech, st->lFrame, st->mem_sig_in, st->fscale);

  LPDCom_tools_Preemphasise(new_speech, PREEMPH_FAC, st->lFrame, &(st->mem_preemph));

  if (isAceStart) {
    copyFLOAT(st->old_speech_pe, old_speech, M + (((L_NEXT_HIGH_RATE_1024 + L_LPC0_1024) * st->lDiv) / L_DIV_1024));
    {
      float tmp;
      float tmp_buf[L_DIV_1024 + 1];

      copyFLOAT(&(speech[-st->lDiv - 1]), tmp_buf, st->lDiv + 1);
      tmp = tmp_buf[0];
      LPDCom_tools_Deemphasise(tmp_buf, PREEMPH_FAC, st->lDiv + 1, &tmp);
      copyFLOAT(&tmp_buf[st->lDiv - 128 + 1], st->LPDmem.Txn, 128);
    }
  } else {
    copyFLOAT(st->old_speech_pe, old_speech, M + ((L_NEXT_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024));
  }

  TCXLevel_old = st->TCXLevel;
  st->calcMode.isAceStart = isAceStart;

  SAFECALL(err, LPDEnc_main_SaveFrameState(st));
  do {
    int disableFacEncoding = 0;
    int activateFacEmergencyMode = 0;

    if (noError == err) {
      err = LPDEnc_closedLoop_Run(speech,
                                  mod,
                                  n_param_tcx,
                                  serialFac,
                                  nBitsFac,
                                  ol_gain,
                                  st,
                                  isAceStart,
                                  pitchLags,
                                  pitchGains,
                                  lpdBitRate,
                                  bUsacIndependencyFlag,
                                  bUseNoiseFilling,
                                  ipfState,
                                  lpd_channel_stream);
    }

    if (err != noError) {
      err = handBack(err);
      return err;
    }

    for (i = 0; i < st->nbDiv; i++) {
      mode_out[i] = mod[i];
    }

    if (mod[0] != 0 && mod[1] != 0 && mod[2] != 0 && mod[3] != 0) {
      lpd_channel_stream->bpf_control_info = 0;
    } else {
      lpd_channel_stream->bpf_control_info = LPDEnc_bpf_Decide(st->fscale, st->lFrame, speech, pitchLags, pitchGains, mod);
    }

    arithResetFlag(st,
                   st->phArith,
                   bUsacIndependencyFlag,
                   st->nbDiv,
                   mod);

    if (isAceStart == 1) {
      lpd_channel_stream->core_mode_last = 0;
    } else {
      lpd_channel_stream->core_mode_last = 1;
    }

    disableFacEncoding =
        (st->calcMode.emergencyRestrictedMode & 0x20) == 0x20;

    if ((((mod[0] == 0) && (mod[-1] != 0)) || ((mod[0] > 0) && (mod[-1] == 0))) && !disableFacEncoding) {
      lpd_channel_stream->fac_data_present = 1;
    } else {
      lpd_channel_stream->fac_data_present = 0;
    }

    getLpdMode(mod,
               lpd_channel_stream);

    LPDEnc_bs_EncodeMain(mod,
                         n_param_tcx,
                         serial,
                         st,
                         total_nbbits,
                         &sqBits,
                         bUsacIndependencyFlag,
                         disableFacEncoding,
                         lpd_channel_stream);

    bitsToUse = *total_nbbits;

    if (isAceStart && mod[0] == 0) {
      bitsToUse += *nBitsFac;
    }
    if (maxBitsToUse < bitsToUse) {
      getMinArithEncBits(mod, &minSqBits);
      if (minSqBits == sqBits && (st->calcMode.emergencyRestrictedMode & 0x8) == 0x8) {
        activateFacEmergencyMode = 1;

        if ((st->calcMode.emergencyRestrictedMode & 0x20) == 0x20) {
          err = iisUtil_ERROR(CDI, "ERROR: Not enough bits to encode frame");
          return err;
        }
      }
      SAFECALL(err, LPDEnc_main_ResetFrameState(st));
      {
        int lpc_bits_tcx20 = 0;
        int lpc_bits_tcx40 = 0;
        int ptrDummy = 0;
        unsigned char serialDummy[4 * NBITS_MAX];
        int mod_tcx20[4] = {1, 1, 1, 1};
        int mod_tcx40[4] = {2, 2, 1, 1};

        if (err == noError) {
          err = LPDEnc_bs_encodeLPC(st->nbDiv, mod_tcx20, lpd_channel_stream->core_mode_last, lpd_channel_stream->lpc_params, total_nbbits, &lpc_bits_tcx20, &ptrDummy, serialDummy);
        }
        if (err == noError) {
          err = LPDEnc_bs_encodeLPC(st->nbDiv, mod_tcx40, lpd_channel_stream->core_mode_last, lpd_channel_stream->lpc_params, total_nbbits, &lpc_bits_tcx40, &ptrDummy, serialDummy);
        }

        LPDEnc_cmConfig_SetEmergencyMode(&(st->calcMode), bitsToUse - maxBitsToUse, maxBitsToUse, lpc_bits_tcx20, lpc_bits_tcx40, activateFacEmergencyMode);
      }
    }
  } while (maxBitsToUse < bitsToUse);

  LPDEnc_cmConfig_Reset(&(st->calcMode));

  st->TCXLevel = TCXLevel_old;
  st->restrictedMode = 0xFF;
  st->prev_mod = (short)mode_out[st->nbDiv - 1];

  if (isAceStart) {
    copyFLOAT(&old_speech[(st->lFrame) + (L_LPC0_1024 * st->lDiv) / L_DIV_1024], st->old_speech_pe, M + ((L_NEXT_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024));
  } else {
    copyFLOAT(&old_speech[(st->lFrame)], st->old_speech_pe, M + ((L_NEXT_HIGH_RATE_1024 * st->lDiv) / L_DIV_1024));
  }

  return err;
}

void getMinArithEncBits(
    int *mod,
    int *minSqBits) {
  if (mod[0] == 0 || mod[1] == 0 || mod[2] == 0 || mod[3] == 0) {
    *minSqBits = -1;
  } else if (mod[0] == 1) {
    if (mod[2] == 1) {
      *minSqBits = 5 + 7 + 7 + 7;
    } else {
      *minSqBits = 5 + 7 + 7;
    }
  } else if (mod[0] == 2) {
    if (mod[2] == 1) {
      *minSqBits = 5 + 7 + 7;
    } else {
      *minSqBits = 5 + 7;
    }
  } else {
    *minSqBits = 5;
  }
}

static void arithResetFlag(
    Coder_State_Plus *st,
    const LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE phArith,
    int const bUsacIndependencyFlag,
    int const nbDiv,
    int mod[]) {
  int write_reset_flag = 0;
  int firstTcx = 1;
  int force_arith_reset = 0;
  int k = 0;

  while (k < nbDiv) {
    if (mod[k] == 0) {
      st->tcx_arith_reset_flag[k] = 0;
      st->lpd_channel_stream.tcx_force_arith_reset[k] = 0;
    } else {
      write_reset_flag = 0;
      if (firstTcx) {
        firstTcx = 0;
        if (bUsacIndependencyFlag) {
          force_arith_reset = 1;
        } else {
          force_arith_reset = LPDEnc_arithEncWrapper_GetForceReset(phArith);

          if (force_arith_reset) {
            force_arith_reset = 1;
          } else {
            force_arith_reset = -1;
          }
          write_reset_flag = 1;
        }
      } else {
        force_arith_reset = 0;
      }
      st->tcx_arith_reset_flag[k] = write_reset_flag;
      st->lpd_channel_stream.tcx_force_arith_reset[k] = force_arith_reset;
    }

    k++;
  }
}

static void getLpdMode(
    int mod[],
    LPD_CHANNEL_STREAM *lpd_channel_stream) {
  if (mod[0] == 3) {
    lpd_channel_stream->lpd_mode = 25;
  } else if ((mod[0] == 2) && (mod[2] == 2)) {
    lpd_channel_stream->lpd_mode = 24;
  } else {
    if (mod[0] == 2) {
      lpd_channel_stream->lpd_mode = 16 + mod[2] + 2 * mod[3];
    } else if (mod[2] == 2) {
      lpd_channel_stream->lpd_mode = 20 + mod[0] + 2 * mod[1];
    } else {
      lpd_channel_stream->lpd_mode = mod[0] + 2 * mod[1] + 4 * mod[2] + 8 * mod[3];
    }
  }
}

int LPDEnc_main_PrepLABuffer(
    float channel_right[],
    int L_next,
    Coder_State_Plus *st) {
  float *new_speech = st->old_speech_pe + M;

  copyFLOAT(channel_right, new_speech, L_next);

  LPDEnc_tools_HighPass50Hz(new_speech, L_next, st->mem_sig_in, st->fscale);

  LPDCom_tools_Preemphasise(new_speech, PREEMPH_FAC, L_next, &(st->mem_preemph));

  return (L_next);
}
