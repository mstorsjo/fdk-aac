
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
#include "iisLPDComLib_tcx_Tools.h"
#include "iisLPDEncLib_tcx_Main.h"
#include "iisLPDEncLib_cmConfig.h"
#include "iisLPDComLib_lpc.h"
#include "iisLPDEncLib_acelp_AdaptiveCB.h"
#include "iisLPDComLib_acelp_Util.h"
#include "iisLPDEncLib_acelp_Main.h"
#include "iisLPDEncLib_closedLoop.h"
#include "iisLPDComLib_constants.h"
#include "iisLPDEncLib_arithEncWrapper.h"
#include "iisLPDComLib_tools.h"
#include "iisLPDEncLib_lpc_LPCAnalysis.h"

#define TBE_BUFFER_OFFSET 160 + 16 + 12

#define MAX_PIT_GAIN_SHORT_AFTER_IPF_STARTUP 1.0f
#define MAX_PIT_GAIN_AT_IPF_STARTUP 0.9f
#define NUMBER_FRAMES_RESTRICTING_PIT_GAIN_AFTER_IPF_START 8

#define NBITS_LPC20_768 15
#define NBITS_LPC40_768 15
#define NBITS_LPC80_768 15
#define NBITS_LPC20_1024 34
#define NBITS_LPC40_1024 40
#define NBITS_LPC80_1024 44
#define HP_ORDER 3

extern const float lsf_init[16];

void LPDEnc_closedLoop_Config(Coder_State_Plus *st) {
  int i;

  setFLOAT(0.0f, st->old_d_wsp, PIT_MAX_MAX / OPL_DECIM);
  setFLOAT(0.0f, st->mem_lp_decim2, 3);

  st->LPDmem.mode = -1;
  st->LPDmem.fac_ns = 0.25f;
  st->LPDmem.nbits = 0;
  setFLOAT(0.0f, st->LPDmem.Aq, 2 * (M + 1));
  setFLOAT(0.0f, st->LPDmem.Ai, 2 * (M + 1));
  setFLOAT(0.0f, st->LPDmem.syn, M + 128);
  setFLOAT(0.0f, st->LPDmem.wsyn, 1 + 128);

  setFLOAT(0.0f, st->LPDmem.Aexc, PIT_MAX_MAX + L_INTERPOL);

  setFLOAT(0.0f, st->LPDmem.Txn, 128);
  setFLOAT(0.0f, st->LPDmem.Txnq, 1 + 256);
  st->LPDmem.Txnq_fac = 0.0f;

  setFLOAT(0.0f, st->hp_old_wsp, L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM));
  setFLOAT(0.0f, st->hp_ol_ltp_mem, 3 * 2 + 1);
  for (i = 0; i < 5; i++)
    st->old_ol_lag[i] = 40;
  st->mem_wsp = 0.0;
  st->old_T0_med = 40;
  st->ol_wght_flg = 0;
  st->ada_w = 0.0;

  copyFLOAT(lsf_init, st->lsfold, M);
  for (i = 0; i < M; i++)
    st->lspold[i] = (float)cos(3.141592654 * (float)(i + 1) / (float)(M + 1));
  copyFLOAT(st->lspold, st->lspold_q, M);

  {
    TCX_MDCT_ERROR error = LPDCom_tcx_OpenMDCT(&(st->hTcxMdct));
    (void)error;
  }

  return;
}

void LPDEnc_closedLoop_Close(Coder_State_Plus *st) {
  LPDCom_tcx_CloseMDCT(&(st->hTcxMdct));

  if (NULL != st->hTcxFft) {
    IIS_CFFT_Destroy(&st->hTcxFft);
    st->hTcxFft = NULL;
  }
}

void LPDEnc_closedLoop_Reset(Coder_State_Plus *st) {
  int i;

  setFLOAT(0.0f, st->old_d_wsp, PIT_MAX_MAX / OPL_DECIM);
  setFLOAT(0.0f, st->mem_lp_decim2, 3);

  st->LPDmem.mode = -1;
  st->LPDmem.fac_ns = 0.25f;
  st->LPDmem.nbits = 0;
  setFLOAT(0.0f, st->LPDmem.Aq, 2 * (M + 1));
  setFLOAT(0.0f, st->LPDmem.Ai, 2 * (M + 1));
  setFLOAT(0.0f, st->LPDmem.syn, M + 128);
  setFLOAT(0.0f, st->LPDmem.wsyn, 1 + 128);

  setFLOAT(0.0f, st->LPDmem.Aexc, PIT_MAX_MAX + L_INTERPOL);

  setFLOAT(0.0f, st->LPDmem.Txn, 128);
  setFLOAT(0.0f, st->LPDmem.Txnq, 1 + 256);
  st->LPDmem.Txnq_fac = 0.0f;

  setFLOAT(0.0f, st->hp_old_wsp, L_FRAME_1024 / OPL_DECIM + (PIT_MAX_MAX / OPL_DECIM));
  setFLOAT(0.0f, st->hp_ol_ltp_mem, 3 * 2 + 1);

  for (i = 0; i < 5; i++) {
    st->old_ol_lag[i] = 40;
  }

  st->mem_wsp = 0.0;
  st->old_T0_med = 40;
  st->ol_wght_flg = 0;
  st->ada_w = 0.0;

  copyFLOAT(lsf_init, st->lsfold, M);
  for (i = 0; i < M; i++)
    st->lspold[i] = (float)cos(3.141592654 * (float)(i + 1) / (float)(M + 1));
  copyFLOAT(st->lspold, st->lspold_q, M);

  st->mem_preemph = 0.0;
  setFLOAT(0.0f, st->mem_sig_in, 4);

  LPDEnc_arithEncWrapper_SetForceReset(st->phArith);

  for (i = 0; i < IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN; i++) {
    st->oldPitchGain[i] = 0.0f;
  }
  st->pOldPitchGainCircle = 0;

  for (i = 0; i < IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN; i++) {
    st->oldPitchGain[i] = 0.0f;
  }
  st->pOldPitchGainCircle = 0;

  return;
}

HANDLE_ERROR_INFO LPDEnc_closedLoop_Run(
    float speech[],
    int mod[],
    int n_param_tcx[],
    unsigned char serialFac[],
    int *nBitsFac,
    float ol_gain[],
    Coder_State_Plus *st,
    int isAceStart,
    float *pitchLags,
    float *pitchGains,
    int lpdBitRate,
    const int bUsacIndependencyFlag,
    const int bUseNoiseFilling,
    const LPD_IPF_STATE ipfState,
    LPD_CHANNEL_STREAM *lpd_channel_stream) {
  float A[(NB_SUBFR_SUPERFR_1024 + 1) * (M + 1)];
  float Aq[(NB_SUBFR_SUPERFR_1024 + 1) * (M + 1)];

  float lsp_q[(NB_DIV + 1) * M];

  int nb_indices, nb_bits;
  float *synth_tcx, synth_tcx_buf[128 + L_FRAME_1024];
  float *synth, synth_buf[128 + L_FRAME_1024];
  float wsig[L_FRAME_1024];
  float *wsyn, wsyn_buf[128 + L_FRAME_1024];
  float *wsyn_tcx, wsyn_tcx_buf[128 + L_FRAME_1024];

  float old_d_wsp[(PIT_MAX_MAX / OPL_DECIM) + L_DIV_1024];
  float *d_wsp;

  int i, j, k, i2, i1;
  int loop1;
  int loop2;
  float snr, snr1, snr2;
  float tmp;
  float ener, cor_max, t0;
  float *p, *p1;
  int Top[2 * NB_DIV] = {0};
  float Tnc[2 * NB_DIV] = {0};

  int n_param = 0;
  int restrictedMode_tmp = 0;
  int PIT_MIN;
  int PIT_MAX;
  LPD_state LPDmem[5], LPDmem_tmp;

  float nbits_acelp, nbits_tcx20;

  int lfacNext, lfacPrev;
  int l_pit_search;
  int tmp_nbits_lpc20, tmp_nbits_lpc40, tmp_nbits_lpc80;

  int lDiv;
  int nbSubfr;
  int nbSubfrSuperfr;
  int lWindow;
  HANDLE_ERROR_INFO err = noError;

  int appy_IPF_APR_restrictions = 0;
  int local_tcx_global_gain[NB_DIV] = {0};
  int local_tcx_noise_level[NB_DIV] = {0};
  int local_tcx_quant[L_FRAME_1024] = {0};

  lDiv = st->lDiv;
  nbSubfr = st->nbSubfr;
  nbSubfrSuperfr = st->nbDiv * nbSubfr;
  if (st->fscale <= FSCALE_DENOM) {
    lWindow = (L_WINDOW_1024 * lDiv) / L_DIV_1024;
  } else {
    lWindow = (L_WINDOW_HIGH_RATE_1024 * lDiv) / L_DIV_1024;
  }

  synth_tcx = synth_tcx_buf + 128;
  synth = synth_buf + 128;
  wsyn = wsyn_buf + 128;
  wsyn_tcx = wsyn_tcx_buf + 128;

  if (lDiv == 192) {
    tmp_nbits_lpc20 = NBITS_LPC20_768;
    tmp_nbits_lpc40 = NBITS_LPC40_768;
    tmp_nbits_lpc80 = NBITS_LPC80_768;
  } else {
    tmp_nbits_lpc20 = NBITS_LPC20_1024;
    tmp_nbits_lpc40 = NBITS_LPC40_1024;
    tmp_nbits_lpc80 = NBITS_LPC80_1024;
  }

  if (st->fscale == 0) {
    PIT_MIN = PIT_MIN_12k8;
    PIT_MAX = PIT_MAX_12k8;
  } else {
    i = (((st->fscale * PIT_MIN_12k8) + (FSCALE_DENOM / 2)) / FSCALE_DENOM) - PIT_MIN_12k8;
    PIT_MIN = PIT_MIN_12k8 + i;
    PIT_MAX = PIT_MAX_12k8 + (6 * i);
  }

  d_wsp = old_d_wsp + PIT_MAX_MAX / OPL_DECIM;

  copyFLOAT(st->old_d_wsp, old_d_wsp, PIT_MAX_MAX / OPL_DECIM);

  LPDEnc_lpc_LPCAnalysis(speech,
                         lWindow,
                         st,
                         isAceStart,
                         A,
                         lsp_q,
                         lpd_channel_stream->lpc_params,
                         &nb_indices,
                         &nb_bits,
                         appy_IPF_APR_restrictions);

  LPDEnc_cmConfig_RestrictAcmAndTcxLevel(st, lpdBitRate);

  if (lDiv == 192) {
    nbits_tcx20 = st->TCXLevel * (float)(((lpdcom_NBITS_CORE_768[lpd_channel_stream->acelp_core_mode] - NBITS_MODE) >> 2) + tmp_nbits_lpc20 - NBITS_LPC);
  } else {
    nbits_tcx20 = st->TCXLevel * (float)(((lpdcom_NBITS_CORE_1024[lpd_channel_stream->acelp_core_mode] - NBITS_MODE) >> 2) + tmp_nbits_lpc20 - NBITS_LPC);
  }

  if (isAceStart) {
    float tmpBuffer[2 * L_DIV_1024 + M];
    float tmpRes[L_SUBFR * 3];
    float ANull[9 * (M + 1)];
    float memHP[4];
    float mem = 0.0f;
    int lfac;
    int nbitsFac = (int)((float)nbits_tcx20 / 2.f);

    if (st->lastWasShort) {
      lfac = (st->lFrame) / 16;
    } else {
      lfac = lDiv / 2;
    }

    LPDCom_lpc_InterpolateLSP(st->lspold_q, st->lspold_q, ANull, (2 * lDiv) / L_SUBFR, M);

    setFLOAT(0.0f, tmpBuffer, 2 * L_DIV_1024 + M);
    setFLOAT(0.0f, tmpRes, 2 * L_SUBFR);

    LPDEnc_tcx_FdFAC(st->hTcxMdct, &st->fdOrig[1 + M], lDiv, lfac, st->lowpassLine, nbitsFac, st->fdSynth, ANull, serialFac, nBitsFac);

    setFLOAT(0.0f, memHP, 4);
    LPDEnc_tools_HighPass50Hz(st->fdOrig, 2 * lDiv + 1 + M, memHP, st->fscale);
    mem = 0.0f;
    LPDCom_tools_Preemphasise(st->fdOrig, PREEMPH_FAC, 2 * lDiv + 1 + M, &mem);
    copyFLOAT(st->fdOrig + 2 * lDiv + 1 + M - 2 * L_SUBFR, tmpBuffer, 2 * L_SUBFR);
    mem = st->fdOrig[2 * lDiv + 1 + M - (2 * L_SUBFR + 1)];
    LPDCom_tools_Deemphasise(tmpBuffer, PREEMPH_FAC, 2 * L_SUBFR, &mem);
    copyFLOAT(tmpBuffer, st->LPDmem.Txn, 2 * L_SUBFR);

    mem = 0.0f;
    LPDCom_tools_Preemphasise(st->fdSynth, PREEMPH_FAC, PIT_MAX_MAX + L_INTERPOL + M, &mem);
    copyFLOAT(st->fdSynth + PIT_MAX_MAX + L_INTERPOL + M - M - 128, st->LPDmem.syn, M + 128);
    copyFLOAT(st->fdSynth, tmpBuffer, PIT_MAX_MAX + L_INTERPOL + M);

    mem = 0.0f;

    LPDEnc_acelp_FindWsp(ANull, tmpBuffer + M + PIT_MAX_MAX + L_INTERPOL - L_SUBFR * 3, tmpRes, &mem, L_SUBFR * 3);
    copyFLOAT(tmpRes + L_SUBFR * 3 - M - 128, st->LPDmem.wsyn, M + 128);

    LPDCom_lpc_Analyze(ANull, &tmpBuffer[M], st->LPDmem.Aexc, PIT_MAX_MAX + L_INTERPOL);

    mem = 0.0f;
    LPDEnc_acelp_FindWsp(A, st->fdOrig + 1 + M, tmpBuffer + M, &(st->mem_wsp), 2 * lDiv);
    for (i = 0; i < 2 * lDiv; i += lDiv) {
      LPDEnc_acelp_Downsample(&tmpBuffer[i + M], lDiv, st->mem_lp_decim2);
      moveFLOAT(tmpBuffer + M + i, tmpBuffer + M + i / OPL_DECIM, lDiv / OPL_DECIM);
    }
    copyFLOAT(tmpBuffer + M + 2 * lDiv / OPL_DECIM - PIT_MAX_MAX / OPL_DECIM, old_d_wsp, PIT_MAX_MAX / OPL_DECIM);

    {
      int L_max = PIT_MAX / OPL_DECIM;
      int nFrame = (2 * L_SUBFR) / OPL_DECIM;
      float *hp_wsp_mem = st->hp_ol_ltp_mem;
      float *hp_old_wsp = st->hp_old_wsp;
      for (i = 0; i < 2 * lDiv / OPL_DECIM; i += nFrame) {
        float *data_a, *data_b, *hp_wsp, o;
        float *wsp = tmpBuffer + M + i;
        data_a = hp_wsp_mem;
        data_b = hp_wsp_mem + HP_ORDER;
        hp_wsp = hp_old_wsp + L_max;
        for (k = 0; k < nFrame; k++) {
          data_b[0] = data_b[1];
          data_b[1] = data_b[2];
          data_b[2] = data_b[3];
          data_b[HP_ORDER] = wsp[k];
          o = data_b[0] * 0.83787057505665F;
          o += data_b[1] * -2.50975570071058F;
          o += data_b[2] * 2.50975570071058F;
          o += data_b[3] * -0.83787057505665F;
          o -= data_a[0] * -2.64436711600664F;
          o -= data_a[1] * 2.35087386625360F;
          o -= data_a[2] * -0.70001156927424F;
          data_a[2] = data_a[1];
          data_a[1] = data_a[0];
          data_a[0] = o;
          hp_wsp[k] = o;
        }
        memmove(hp_old_wsp, &hp_old_wsp[nFrame], L_max * sizeof(float));
      }
    }
  }

  copyFLOAT(st->lspold_q, lsp_q, M);
  copyFLOAT(&lsp_q[st->nbDiv * M], st->lspold_q, M);

  for (i = 0; i < st->nbDiv; i++) {
    LPDEnc_acelp_FindWsp(&A[i * nbSubfr * (M + 1)], &speech[i * lDiv], &wsig[i * lDiv], &(st->mem_wsp), lDiv);
    copyFLOAT(&wsig[i * lDiv], d_wsp, lDiv);

    LPDEnc_acelp_Downsample(d_wsp, lDiv, st->mem_lp_decim2);

    l_pit_search = 2 * L_SUBFR;
    if (nbSubfr < 4) l_pit_search = 3 * L_SUBFR;

    Top[i * 2] = LPDEnc_acelp_FindPitchOpenLoop(d_wsp,
                                                (PIT_MIN / OPL_DECIM) + 1,
                                                PIT_MAX / OPL_DECIM,
                                                l_pit_search / OPL_DECIM,
                                                st->old_T0_med,
                                                &(ol_gain[i]),
                                                st->hp_ol_ltp_mem,
                                                st->hp_old_wsp,
                                                st->ol_wght_flg);

    if (ol_gain[i] > 0.6) {
      st->old_T0_med = LPDEnc_acelp_GetMedianPitch(Top[i * 2], st->old_ol_lag);
      st->ada_w = 1.0;
    } else {
      st->ada_w *= 0.9f;
    }

    if (st->ada_w < 0.8) {
      st->ol_wght_flg = 0;
    } else {
      st->ol_wght_flg = 1;
    }

    cor_max = 0.0f;
    p = &d_wsp[0];
    p1 = d_wsp - Top[i * 2];

    for (j = 0; j < l_pit_search / OPL_DECIM; j++) {
      cor_max += *p++ * *p1++;
    }

    t0 = 0.01f;
    p = d_wsp - Top[i * 2];

    for (j = 0; j < l_pit_search / OPL_DECIM; j++, p++) {
      t0 += *p * *p;
    }
    t0 = (float)(1.0 / sqrt(t0));
    Tnc[i * 2] = cor_max * t0;

    ener = 0.01f;
    for (j = 0; j < l_pit_search / OPL_DECIM; j++) {
      ener += d_wsp[j] * d_wsp[j];
    }
    Tnc[i * 2] /= (float)sqrt(ener);

    if (nbSubfr < 4) {
      Top[(i * 2) + 1] = Top[i * 2];
      Tnc[(i * 2) + 1] = Tnc[i * 2];
    } else {
      Top[(i * 2) + 1] = LPDEnc_acelp_FindPitchOpenLoop(d_wsp + ((2 * L_SUBFR) / OPL_DECIM),
                                                        (PIT_MIN / OPL_DECIM) + 1,
                                                        PIT_MAX / OPL_DECIM,
                                                        (2 * L_SUBFR) / OPL_DECIM,
                                                        st->old_T0_med,
                                                        &(ol_gain[i]),
                                                        st->hp_ol_ltp_mem,
                                                        st->hp_old_wsp,
                                                        st->ol_wght_flg);

      if (ol_gain[i] > 0.6) {
        st->old_T0_med = LPDEnc_acelp_GetMedianPitch(Top[(i * 2) + 1], st->old_ol_lag);
        st->ada_w = 1.0;
      } else {
        st->ada_w = st->ada_w * 0.9f;
      }

      if (st->ada_w < 0.8) {
        st->ol_wght_flg = 0;
      } else {
        st->ol_wght_flg = 1;
      }

      cor_max = 0.0f;
      p = d_wsp + (2 * L_SUBFR) / OPL_DECIM;
      p1 = d_wsp + ((2 * L_SUBFR) / OPL_DECIM) - Top[(i * 2) + 1];

      for (j = 0; j < (2 * L_SUBFR) / OPL_DECIM; j++) {
        cor_max += *p++ * *p1++;
      }

      t0 = 0.01f;
      p = d_wsp + ((2 * L_SUBFR) / OPL_DECIM) - Top[(i * 2) + 1];

      for (j = 0; j < (2 * L_SUBFR) / OPL_DECIM; j++, p++) {
        t0 += *p * *p;
      }

      t0 = (float)(1.0 / sqrt(t0));
      Tnc[(i * 2) + 1] = cor_max * t0;

      ener = 0.01f;
      for (j = 0; j < (2 * L_SUBFR) / OPL_DECIM; j++) {
        ener += d_wsp[((2 * L_SUBFR) / OPL_DECIM) + j] * d_wsp[((2 * L_SUBFR) / OPL_DECIM) + j];
      }
      Tnc[(i * 2) + 1] /= (float)sqrt(ener);
    }

    moveFLOAT(&old_d_wsp[lDiv / OPL_DECIM], old_d_wsp, PIT_MAX_MAX / OPL_DECIM);
  }

  {
    int Tmax, T;
    float max;

    k = 0;
    max = Tnc[k];
    for (i = 1; i < (2 * st->nbDiv); i++) {
      if (Tnc[i] > max) {
        max = Tnc[i];
        k = i;
      }
    }
    Tmax = Top[k];

    if (max > 0.75f) {
      T = Tmax;
      for (i = k - 1; i >= 0; i--) {
        if (Top[i] < T) {
          T = T + 8;
          if (T > PIT_MAX) T = PIT_MAX;

          if ((Top[i] * 4) < T)
            Top[i] *= 4;
          else if ((Top[i] * 3) < T)
            Top[i] *= 3;
          else if ((Top[i] * 2) < T)
            Top[i] *= 2;
        } else {
          T = T - 8;
          if (T < PIT_MIN) T = PIT_MIN;

          if ((Top[i] / 4) > T)
            Top[i] /= 4;
          else if ((Top[i] / 3) > T)
            Top[i] /= 3;
          else if ((Top[i] / 2) > T)
            Top[i] /= 2;
        }

        T = Top[i];
      }

      T = Tmax;
      for (i = k + 1; i < (2 * st->nbDiv); i++) {
        if (Top[i] < T) {
          T = T + 8;
          if (T > PIT_MAX) T = PIT_MAX;

          if ((Top[i] * 4) < T)
            Top[i] *= 4;
          else if ((Top[i] * 3) < T)
            Top[i] *= 3;
          else if ((Top[i] * 2) < T)
            Top[i] *= 2;
        } else {
          T = T - 8;
          if (T < PIT_MIN) T = PIT_MIN;

          if ((Top[i] / 4) > T)
            Top[i] /= 4;
          else if ((Top[i] / 3) > T)
            Top[i] /= 3;
          else if ((Top[i] / 2) > T)
            Top[i] /= 2;
        }

        T = Top[i];
      }
    }
  }

  LPDEnc_cmConfig_AdaptAcm(st, lpdBitRate, Tnc);

  if (lDiv == 192) {
    nbits_acelp = (float)(((lpdcom_NBITS_CORE_768[lpd_channel_stream->acelp_core_mode] - NBITS_MODE) >> 2) + tmp_nbits_lpc20 - NBITS_LPC);
  } else {
    nbits_acelp = (float)(((lpdcom_NBITS_CORE_1024[lpd_channel_stream->acelp_core_mode] - NBITS_MODE) >> 2) + tmp_nbits_lpc20 - NBITS_LPC);
  }

  LPDmem[0] = st->LPDmem;

  snr2 = 0.0;

  for (i1 = 0; i1 < 4; i1++) {
    mod[i1] = -3;
  }

  if ((st->restrictedMode & 0xF) == 0x0) {
    return iisUtil_ERROR(CDI, "invalid restricted Mode");
  }

  loop1 = 2;
  loop2 = 2;

  if (LPD_IPF_STATE_RAP_FIRST_PREROLL == ipfState || LPD_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL == ipfState) {
    st->nIpfReducePitchGain = NUMBER_FRAMES_RESTRICTING_PIT_GAIN_AFTER_IPF_START;
  }
  for (i1 = 0; i1 < loop1; i1++) {
    snr1 = 0.0;

    for (i2 = 0; i2 < loop2; i2++) {
      k = (i1 * 2) + i2;

      restrictedMode_tmp = LPDEnc_cmConfig_RestrictSubFrameAcm(&(st->calcMode),
                                                               k,
                                                               mod,
                                                               st->restrictedMode,
                                                               appy_IPF_APR_restrictions);

      if ((restrictedMode_tmp & 0xF) == 0x0) {
        return iisUtil_ERROR(CDI, "invalid restricted Mode");
      }

      snr = SNR_MIN;

      if ((restrictedMode_tmp)&1) {
        float avgLongPitchGain, avgShortPitchGain;

        int maxPitchLag = PIT_MAX_MAX;
        float maxPitchGain = -1.0f;
        if (st->nIpfReducePitchGain > 0) {
          st->nIpfReducePitchGain--;
          maxPitchGain = MAX_PIT_GAIN_SHORT_AFTER_IPF_STARTUP;
        }
        if (LPD_IPF_STATE_RAP_FIRST_PREROLL == ipfState || LPD_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL == ipfState) {
          if ((k - 1) * lDiv < PIT_MAX_MAX) {
            maxPitchLag = (k - 1) * lDiv;
            maxPitchGain = MAX_PIT_GAIN_AT_IPF_STARTUP;
          }
        }

        LPDCom_tcx_InterpolateAcelpLSPs(&lsp_q[k * M], &lsp_q[(k + 1) * M], Aq, nbSubfr, M);

        LPDmem[k + 1] = LPDmem[k];
        SAFECALL(err, LPDEnc_acelp_Encode(&A[k * (nbSubfrSuperfr / 4) * (M + 1)],
                                          Aq,
                                          &speech[k * lDiv],
                                          &wsig[k * lDiv],
                                          &synth[k * lDiv],
                                          &wsyn[k * lDiv],
                                          &LPDmem[k + 1],
                                          k,
                                          lDiv,
                                          Tnc[k * 2],
                                          Tnc[(k * 2) + 1],
                                          Top[k * 2],
                                          Top[(k * 2) + 1],
                                          st->fscale,
                                          &pitchLags[k * lDiv / L_SUBFR],
                                          &pitchGains[k * lDiv / L_SUBFR],
                                          maxPitchLag,
                                          maxPitchGain,
                                          &snr,
                                          st,
                                          lpd_channel_stream));

        mod[k] = 0;
        n_param_tcx[k] = 0;
        avgLongPitchGain = 0;
        avgShortPitchGain = 0;
        for (i = 0; i < lDiv / L_SUBFR; i++) {
          avgShortPitchGain += pitchGains[k * lDiv / L_SUBFR + i];
          st->oldPitchGain[st->pOldPitchGainCircle++] = pitchGains[k * lDiv / L_SUBFR + i];
          st->pOldPitchGainCircle %= IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN;
        }
        avgShortPitchGain /= (float)(lDiv / L_SUBFR);
        i = st->pOldPitchGainCircle + IIS_LPD_ENC_MAX_SUPERFRAME_SAVE_PITCH_GAIN * NB_DIV * (4 - (lDiv / L_SUBFR));
        if (i < IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN) {
          for (; i < IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN; i++) {
            avgLongPitchGain += st->oldPitchGain[i];
          }
        }
        i %= IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN;
        for (; i < st->pOldPitchGainCircle; i++) {
          avgLongPitchGain += st->oldPitchGain[i];
        }
        avgLongPitchGain /= IIS_LPD_ENC_MAX_SUPERFRAME_SAVE_PITCH_GAIN * NB_DIV * (lDiv / L_SUBFR);
        if (avgLongPitchGain > 1.0f && avgShortPitchGain > 0.8f) {
          snr = SNR_MIN;
        }
      } else {
        for (i = 0; i < lDiv / L_SUBFR; i++) {
          st->oldPitchGain[st->pOldPitchGainCircle++] = 0;
          st->pOldPitchGainCircle %= IIS_LPD_ENC_MAX_SUBFRAME_SAVE_PITCH_GAIN;
        }
      }

      if ((restrictedMode_tmp >> 1) & 1) {
        LPDCom_tcx_InterpolateTcxLSPs(&lsp_q[k * M], &lsp_q[(k + 1) * M], Aq, nbSubfr, M);

        LPDmem_tmp = LPDmem[k];
        if (k == 0 && st->lastWasShort) {
          lfacPrev = (st->lFrame) / 16;
        } else {
          lfacPrev = lDiv / 2;
        }
        if (k == 3 && st->nextIsShort) {
          lfacNext = (st->lFrame) / 16;
          ;
        } else {
          lfacNext = lDiv / 2;
        }

        tmp = snr;

        if (noError == err) {
          err = LPDEnc_tcx_Encode(&A[k * (nbSubfrSuperfr / 4) * (M + 1)],
                                  Aq,
                                  &speech[k * lDiv],
                                  &wsig[k * lDiv],
                                  synth_tcx,
                                  wsyn_tcx,
                                  st,
                                  k,
                                  lDiv,
                                  lDiv,
                                  ((int)nbits_tcx20) - tmp_nbits_lpc20,
                                  &LPDmem_tmp,
                                  &n_param,
                                  lfacPrev,
                                  lfacNext,
                                  nbits_acelp - (float)tmp_nbits_lpc20,
                                  (!bUsacIndependencyFlag && !k),
                                  bUseNoiseFilling,
                                  &tmp,
                                  lpd_channel_stream);

          if (noError != err) {
            err = handBack(err);
          }
        }

        if (tmp > snr) {
          snr = tmp;
          mod[k] = 1;
          n_param_tcx[k] = n_param;

          local_tcx_global_gain[k] = lpd_channel_stream->tcx_global_gain[k];
          local_tcx_noise_level[k] = lpd_channel_stream->tcx_noise_factor[k];

          LPDmem[k + 1] = LPDmem_tmp;
          copyINT(lpd_channel_stream->tcx_quant, &local_tcx_quant[k * lDiv], lDiv);

          copyFLOAT(synth_tcx - 128, &synth[(k * lDiv) - 128], lDiv + 128);
          copyFLOAT(wsyn_tcx - 128, &wsyn[(k * lDiv) - 128], lDiv + 128);
        }
      }

      snr1 += 0.5f * snr;
    }

    if ((restrictedMode_tmp >> 2) & 1) {
      k = (i1 * 2);

      LPDCom_tcx_InterpolateTcxLSPs(&lsp_q[k * M], &lsp_q[(k + 2) * M], Aq, (nbSubfrSuperfr / 2), M);

      LPDmem_tmp = LPDmem[k];

      if (k == 0 && st->lastWasShort) {
        lfacPrev = (st->lFrame) / 16;
        ;
      } else {
        lfacPrev = lDiv / 2;
      }
      if (k == 2 && st->nextIsShort) {
        lfacNext = (st->lFrame) / 16;
        ;
      } else {
        lfacNext = lDiv / 2;
      }

      tmp = snr1;

      if (noError == err) {
        err = LPDEnc_tcx_Encode(&A[k * (nbSubfrSuperfr / 4) * (M + 1)],
                                Aq,
                                &speech[k * lDiv],
                                &wsig[k * lDiv],
                                synth_tcx,
                                wsyn_tcx,
                                st,
                                k,
                                2 * lDiv,
                                lDiv,
                                (2 * (int)nbits_tcx20) - tmp_nbits_lpc40,
                                &LPDmem_tmp,
                                &n_param,
                                lfacPrev,
                                lfacNext,
                                nbits_acelp - (float)tmp_nbits_lpc40 * 0.5f,
                                (!bUsacIndependencyFlag && !k),
                                bUseNoiseFilling,
                                &tmp,
                                lpd_channel_stream);

        if (noError != err) {
          err = handBack(err);
        }
      }

      if (tmp > snr1) {
        snr1 = tmp;
        for (i = 0; i < 2; i++) {
          mod[k + i] = 2;
          n_param_tcx[k + i] = n_param;
          local_tcx_global_gain[k + i] = lpd_channel_stream->tcx_global_gain[k + i];
          local_tcx_noise_level[k + i] = lpd_channel_stream->tcx_noise_factor[k + i];
        }

        LPDmem[k + 2] = LPDmem_tmp;
        copyINT(lpd_channel_stream->tcx_quant, &local_tcx_quant[k * lDiv], 2 * lDiv);

        copyFLOAT(synth_tcx - 128, &synth[(k * lDiv) - 128], (2 * lDiv) + 128);
        copyFLOAT(wsyn_tcx - 128, &wsyn[(k * lDiv) - 128], (2 * lDiv) + 128);
      }
    }

    snr2 += 0.5f * snr1;
  }

  if ((restrictedMode_tmp >> 3) & 1) {
    k = 0;

    LPDCom_tcx_InterpolateTcxLSPs(&lsp_q[k * M], &lsp_q[(k + 4) * M], Aq, nbSubfrSuperfr, M);

    LPDmem_tmp = LPDmem[k];
    if (st->lastWasShort) {
      lfacPrev = (st->lFrame) / 16;
      ;
    } else {
      lfacPrev = lDiv / 2;
    }
    if (st->nextIsShort) {
      lfacNext = (st->lFrame) / 16;
      ;
    } else {
      lfacNext = lDiv / 2;
    }

    tmp = snr2;

    if (noError == err) {
      err = LPDEnc_tcx_Encode(&A[k * (nbSubfrSuperfr / 4) * (M + 1)],
                              Aq,
                              &speech[k * lDiv],
                              &wsig[k * lDiv],
                              synth_tcx,
                              wsyn_tcx,
                              st,
                              k,
                              4 * lDiv,
                              lDiv,
                              (4 * (int)nbits_tcx20) - tmp_nbits_lpc80,
                              &LPDmem_tmp,
                              &n_param,
                              lfacPrev,
                              lfacNext,
                              nbits_acelp - (float)tmp_nbits_lpc80 * 0.25f,
                              (!bUsacIndependencyFlag && !k),
                              bUseNoiseFilling,
                              &tmp,
                              lpd_channel_stream);

      if (noError != err) {
        err = handBack(err);
      }
    }

    if (tmp > snr2) {
      for (i = 0; i < 4; i++) {
        mod[k + i] = 3;
        n_param_tcx[k + i] = n_param;
        local_tcx_global_gain[k + i] = lpd_channel_stream->tcx_global_gain[k + i];
        local_tcx_noise_level[k + i] = lpd_channel_stream->tcx_noise_factor[k + i];
      }
      LPDmem[k + 4] = LPDmem_tmp;
      copyINT(lpd_channel_stream->tcx_quant, local_tcx_quant, 4 * lDiv);

      copyFLOAT(synth_tcx - 128, &synth[(k * lDiv) - 128], (4 * lDiv) + 128);
      copyFLOAT(wsyn_tcx - 128, &wsyn[(k * lDiv) - 128], (4 * lDiv) + 128);
    }
  }

  copyINT(local_tcx_noise_level, lpd_channel_stream->tcx_noise_factor, NB_DIV);
  copyINT(local_tcx_global_gain, lpd_channel_stream->tcx_global_gain, NB_DIV);
  copyINT(local_tcx_quant, lpd_channel_stream->tcx_quant, L_FRAME_1024);

  st->LPDmem = LPDmem[st->nbDiv];

  copyFLOAT(old_d_wsp, st->old_d_wsp, PIT_MAX_MAX / OPL_DECIM);

  return err;
}
