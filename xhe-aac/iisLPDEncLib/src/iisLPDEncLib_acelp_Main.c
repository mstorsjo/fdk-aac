
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
#include <math.h>

#include "mathlib.h"
#include "iisLPDEncLib.h"
#include "iisLPDComLib_constants.h"
#include "iisLPDEncLib_acelp_Main.h"
#include "iisLPDComLib_tcx_Tools.h"
#include "options.h"
#include "iisLPDComLib_lpc.h"
#include "iisLPDComLib_tools.h"
#include "iisLPDComLib_acelp_Util.h"
#include "iisLPDEncLib_acelp_AdaptiveCB.h"
#include "iisLPDEncLib_acelp_InnovativeCB.h"
#include "iisLPDEncLib_acelp_ImprovedInnovativeCB.h"

#define MAX_PIT_GAIN_FOR_TO_LONG_PIT_LAG 0.75f

static int quantizeGains(
    float code[],
    int lcode,
    float *gain_pit,
    float *gain_code,
    float *coeff,

    float mean_ener,
    float const max_pit_gain) {
  int i, indice = 0, min_ind, size;
  float ener_code, gcode0;
  float dist, dist_min, g_pitch, g_code;
  const float *t_qua_gain, *p;
  float lambda = LAMBDA_OPTIMIZER;

  t_qua_gain = lpdcom_qua_gain7b;
  min_ind = 0;
  size = 128;

  ener_code = 0.01F;
  for (i = 0; i < lcode; i++) {
    ener_code += code[i] * code[i];
  }
  ener_code = (float)(10.0 * log10(ener_code / (float)lcode));

  gcode0 = mean_ener - ener_code;
  gcode0 = (float)pow(10.0, gcode0 / 20.0);

  dist_min = FLT_MAX;
  p = (const float *)(t_qua_gain + min_ind * 2);
  for (i = 0; i < size; i++) {
    g_pitch = *p++;
    g_code = gcode0 * *p++;
    if (max_pit_gain <= 0.0f || g_pitch < max_pit_gain) {
      dist = g_pitch * g_pitch * coeff[0] + g_pitch * coeff[1] + g_code * g_code * coeff[2] + g_code * coeff[3] + g_pitch * g_code * coeff[4];

      if (lambda > 0.0f) {
        dist += coeff[5] + lambda * (float)(fabs(coeff[5] - (g_pitch * g_pitch * coeff[0] + g_code * g_code * coeff[2] + g_pitch * g_code * coeff[4])));
      }

      if (dist < dist_min) {
        dist_min = dist;
        indice = i;
      }
    }
  }
  indice += min_ind;

  *gain_pit = t_qua_gain[indice * 2];
  *gain_code = t_qua_gain[indice * 2 + 1] * gcode0;

  return indice;
}

HANDLE_ERROR_INFO LPDEnc_acelp_Encode(
    float A[],
    float Aq[],
    float speech[],
    float wsig[],
    float synth[],
    float wsyn[],
    LPD_state *LPDmem,
    const int sfOffset,
    int lDiv,
    float norm_corr,
    float norm_corr2,
    int T_op,
    int T_op2,
    int pit_adj,
    float *pitchLags,
    float *pitchGains,
    int const maxPitchLag,
    float const maxPitchGain,
    float *snr,
    Coder_State_Plus *st,
    LPD_CHANNEL_STREAM *lpd_channel_stream) {
  int i, i_subfr, select, nbits;
  int T0, T0_min, T0_max, index, pit_flag;
  int T0_frac;
  float tmp, ener, max_ener, mean_ener_code;
  float gain_pit, gain_code, gain1, gain2;
  float g_corr[6] = {0};
  float g_corr2[2];
  float *p_A, *p_Aq, Ap[M + 1];
  float h1[L_SUBFR];
  float code[L_SUBFR];
  float error[M + L_SUBFR + 8];
  float cn[L_SUBFR];
  float xn[L_SUBFR];
  float xn2[L_SUBFR];
  float dn[L_SUBFR];
  float y0[L_SUBFR];
  float y1[L_SUBFR];
  float y2[L_SUBFR];
  int PIT_MIN;
  int PIT_FR2;
  int PIT_FR1;
  int PIT_MAX;
  float exc_buf[L_DIV_1024 + PIT_MAX_MAX + L_INTERPOL + 1];
  float *exc;
  float mem_Txn, mem_Txnq;
  HANDLE_ERROR_INFO err = noError;

  int lFAC;
  float Rw[L_SUBFR], cn2[L_SUBFR];
  int enhancedPulseSearch;
  int k;

  int codec_mode = lpd_channel_stream->acelp_core_mode;

  (void)st;

  enhancedPulseSearch = LPDmem->enhancedPulseSearch;

  lFAC = lDiv / 2;

  if (LPDmem->mode > 0) {
    for (i = 0; i < lFAC; i++) {
      lpd_channel_stream->fac_data[sfOffset][i] = LPDmem->RE8prm[i];
    }
  }

  exc = exc_buf + PIT_MAX_MAX + L_INTERPOL;

  copyFLOAT(LPDmem->Aexc, exc_buf, PIT_MAX_MAX + L_INTERPOL);
  copyFLOAT(&(LPDmem->syn[M]), synth - 128, 128);
  copyFLOAT(&(LPDmem->wsyn[1]), wsyn - 128, 128);

  if (lDiv == 192) {
    nbits = ((lpdcom_NBITS_CORE_768[codec_mode] - NBITS_MODE) / 4) - NBITS_LPC;
  } else {
    nbits = ((lpdcom_NBITS_CORE_1024[codec_mode] - NBITS_MODE) / 4) - NBITS_LPC;
  }

  if (pit_adj == 0) {
    PIT_MIN = PIT_MIN_12k8;
    PIT_FR2 = PIT_FR2_12k8;
    PIT_FR1 = PIT_FR1_12k8;
    PIT_MAX = PIT_MAX_12k8;
  } else {
    i = (((pit_adj * PIT_MIN_12k8) + (FSCALE_DENOM / 2)) / FSCALE_DENOM) - PIT_MIN_12k8;
    PIT_MIN = PIT_MIN_12k8 + i;
    PIT_FR2 = PIT_FR2_12k8 - i;
    PIT_FR1 = PIT_FR1_12k8;
    PIT_MAX = PIT_MAX_12k8 + (6 * i);
  }

  T_op *= OPL_DECIM;
  T_op2 *= OPL_DECIM;

  T0_min = MIN(T_op, T_op2) - 4;
  if (T0_min < T_op - 8) T0_min = T_op - 8;
  if (T0_min < PIT_MIN) T0_min = PIT_MIN;

  T0_max = MAX(T_op, T_op2) + 4;
  if (T0_max > T0_min + 15) T0_max = T0_min + 15;

  if (T0_max > PIT_MAX) {
    T0_max = PIT_MAX;
    T0_min = T0_max - 15;
  }

  if (maxPitchLag < PIT_MAX_MAX) {
    if (T0_max > maxPitchLag && PIT_MIN <= maxPitchLag) {
      T0_max = maxPitchLag;
      T0_min = T0_max - 15;
      if (T0_min < PIT_MIN) {
        T0_min = PIT_MIN;
      }
    } else if (T0_max > maxPitchLag + L_SUBFR - 8 && PIT_MIN <= maxPitchLag + L_SUBFR - 8) {
      T0_max = maxPitchLag + L_SUBFR - 8;
      T0_min = T0_max - 15;
      if (T0_min < PIT_MIN) {
        T0_min = PIT_MIN;
        if (T0_max < T0_min) {
          T0_max = T0_min + 1;
        }
      }
    }
  }

  max_ener = 0.0;
  mean_ener_code = 0.0;
  p_Aq = Aq;
  for (i_subfr = 0; i_subfr < lDiv; i_subfr += L_SUBFR) {
    LPDCom_lpc_Analyze(p_Aq, &speech[i_subfr], &exc[i_subfr], L_SUBFR);
    ener = 0.01f;
    for (i = 0; i < L_SUBFR; i++) {
      ener += exc[i + i_subfr] * exc[i + i_subfr];
    }
    ener = 10.0f * (float)log10(ener / ((float)L_SUBFR));
    if (ener < 0.0) {
      ener = 0.0;
    }
    if (ener > max_ener) {
      max_ener = ener;
    }
    mean_ener_code += ener;
    p_Aq += (M + 1);
  }
  mean_ener_code /= lDiv / L_SUBFR;

  mean_ener_code -= 5.0f * (norm_corr + norm_corr2);

  index = (int)floor((mean_ener_code - 18.0f) / 12.0f + 0.5);
  if (index < 0) {
    index = 0;
  }
  if (index > 3) {
    index = 3;
  }
  mean_ener_code = (((float)index) * 12.0f) + 18.0f;

  while ((mean_ener_code < (max_ener - 27.0)) && (index < 3)) {
    index++;
    mean_ener_code += 12.0f;
  }

  lpd_channel_stream->acelp_mean_energy[sfOffset] = index;

  p_A = A;
  p_Aq = Aq;

  for (i_subfr = 0; i_subfr < lDiv; i_subfr += L_SUBFR) {
    pit_flag = i_subfr;
    if ((lDiv == 256) && (i_subfr == (2 * L_SUBFR))) {
      pit_flag = 0;

      T0_min = MIN(T_op, T_op2) - 4;
      if (T0_min < T_op2 - 8) T0_min = T_op2 - 8;
      if (T0_min < PIT_MIN) T0_min = PIT_MIN;

      T0_max = MAX(T_op, T_op2) + 4;
      if (T0_max > T0_min + 15) T0_max = T0_min + 15;

      if (T0_max > PIT_MAX) {
        T0_max = PIT_MAX;
        T0_min = T0_max - 15;
      }
      if (maxPitchLag < PIT_MAX_MAX) {
        if (T0_max > maxPitchLag + i_subfr && PIT_MIN <= maxPitchLag) {
          T0_max = maxPitchLag + i_subfr;
          T0_min = maxPitchLag + i_subfr - 15;
          if (T0_min < PIT_MIN) {
            T0_min = PIT_MIN;
          }
        } else if (T0_max > maxPitchLag + i_subfr + L_SUBFR - 8 && PIT_MIN <= maxPitchLag + L_SUBFR - 8) {
          T0_max = maxPitchLag + i_subfr + L_SUBFR - 8;
          T0_min = T0_max - 15;
          if (T0_min < PIT_MIN) {
            T0_min = PIT_MIN;
            if (T0_max < T0_min) {
              T0_max = T0_min + 1;
            }
          }
        }
      }
    }

    copyFLOAT(&wsig[i_subfr], xn, L_SUBFR);

    copyFLOAT(&synth[i_subfr - M], error, M);

    setFLOAT(0.0f, error + M, L_SUBFR);

    LPDCom_lpc_Synthesize(p_Aq, error + M, error + M, L_SUBFR, error, 0);

    LPDCom_lpc_AWeight(p_A, Ap, GAMMA1, M);

    LPDCom_lpc_Analyze(Ap, error + M, xn2, L_SUBFR);

    tmp = wsyn[i_subfr - 1];

    LPDCom_tools_Deemphasise(xn2, TILT_FAC, L_SUBFR, &tmp);

    copyFLOAT(xn2, y0, L_SUBFR);

    for (i = 0; i < L_SUBFR; i++) {
      xn[i] -= xn2[i];
    }

    LPDCom_lpc_Analyze(p_Aq, &speech[i_subfr], &exc[i_subfr], L_SUBFR);

    setFLOAT(0.0f, code, M);

    copyFLOAT(xn, code + M, L_SUBFR / 2);
    tmp = 0.0;
    LPDCom_tools_Preemphasise(code + M, TILT_FAC, L_SUBFR / 2, &tmp);
    LPDCom_lpc_AWeight(p_A, Ap, GAMMA1, M);
    LPDCom_lpc_Synthesize(Ap, code + M, code + M, L_SUBFR / 2, code, 0);
    LPDCom_lpc_Analyze(p_Aq, code + M, cn, L_SUBFR / 2);

    copyFLOAT(&exc[i_subfr + (L_SUBFR / 2)], cn + (L_SUBFR / 2), L_SUBFR / 2);

    setFLOAT(0.0f, h1, L_SUBFR);

    copyFLOAT(Ap, h1, M + 1);
    LPDCom_lpc_Synthesize(p_Aq, h1, h1, L_SUBFR, &h1[M + 1], 0);
    tmp = 0.0;
    LPDCom_tools_Deemphasise(h1, TILT_FAC, L_SUBFR, &tmp);

    T0 = LPDEnc_acelp_FindPitchClosedLoop(&exc[i_subfr], xn, h1, T0_min, T0_max, &T0_frac,
                                          pit_flag, PIT_FR2, PIT_FR1);

    if (pit_flag == 0) {
      if (T0 < PIT_FR2) {
        index = T0 * 4 + T0_frac - (PIT_MIN * 4);
      } else if (T0 < PIT_FR1) {
        index = T0 * 2 + (T0_frac >> 1) - (PIT_FR2 * 2) + ((PIT_FR2 - PIT_MIN) * 4);
      } else {
        index = T0 - PIT_FR1 + ((PIT_FR2 - PIT_MIN) * 4) + ((PIT_FR1 - PIT_FR2) * 2);
      }

      T0_min = T0 - 8;
      if (T0_min < PIT_MIN) {
        T0_min = PIT_MIN;
      }
      T0_max = T0_min + 15;
      if (T0_max > PIT_MAX) {
        T0_max = PIT_MAX;
        T0_min = T0_max - 15;
      }
    } else {
      i = T0 - T0_min;
      index = i * 4 + T0_frac;
    }

    lpd_channel_stream->acelp_acb_index[sfOffset][i_subfr / L_SUBFR] = index;

    pitchLags[i_subfr / L_SUBFR] = (float)T0 + T0_frac * 0.25f;

    LPDCom_acelp_LTPSynthesis(&exc[i_subfr], T0, T0_frac, L_SUBFR + 1);
    LPDCom_tools_convolve(&exc[i_subfr], h1, y1);
    gain1 = LPDEnc_acelp_CalculateAdaptiveCBGain(xn, y1, g_corr);

    LPDEnc_acelp_UpdateFixedCBTarget(xn, xn2, y1, gain1);
    ener = 0.0;
    for (i = 0; i < L_SUBFR; i++) {
      ener += xn2[i] * xn2[i];
    }

    for (i = 0; i < L_SUBFR; i++) {
      code[i] = (float)(0.18 * exc[i - 1 + i_subfr] + 0.64 * exc[i + i_subfr] + 0.18 * exc[i + 1 + i_subfr]);
    }
    LPDCom_tools_convolve(code, h1, y2);
    gain2 = LPDEnc_acelp_CalculateAdaptiveCBGain(xn, y2, g_corr2);

    LPDEnc_acelp_UpdateFixedCBTarget(xn, xn2, y2, gain2);
    tmp = 0.0;
    for (i = 0; i < L_SUBFR; i++) {
      tmp += xn2[i] * xn2[i];
    }

    if (tmp < ener) {
      select = 0;
      copyFLOAT(code, &exc[i_subfr], L_SUBFR);
      copyFLOAT(y2, y1, L_SUBFR);
      gain_pit = gain2;
      g_corr[0] = g_corr2[0];
      g_corr[1] = g_corr2[1];
    } else {
      select = 1;
      gain_pit = gain1;
    }
    if (maxPitchLag < PIT_MAX_MAX) {
      if (T0 > maxPitchLag + i_subfr) {
        if (gain_pit > MAX_PIT_GAIN_FOR_TO_LONG_PIT_LAG) {
          gain_pit = MAX_PIT_GAIN_FOR_TO_LONG_PIT_LAG;
        }
      }
    }
    if (maxPitchGain >= 0 && gain_pit > maxPitchGain) {
      gain_pit = maxPitchGain;
    }

    lpd_channel_stream->acelp_ltp_filtering[sfOffset][i_subfr / L_SUBFR] = select;

    LPDEnc_acelp_UpdateFixedCBTarget(xn, xn2, y1, gain_pit);

    tmp = 0.0;
    LPDCom_tools_Preemphasise(h1, TILT_CODE, L_SUBFR, &tmp);
    if (T0_frac > 2) {
      T0++;
    }

    LPDCom_acelp_PitchSharpening(h1, T0);

    if (enhancedPulseSearch) {
      LPDEnc_acelp_Correlate(h1, Rw, h1);

      for (k = 0; k < L_SUBFR; k++) {
        cn2[k] = xn2[k];

        for (i = 0; i < k; i++) {
          cn2[k] -= cn2[i] * h1[k - i];
        }
      }
      if (
          lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 16 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 17 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 20 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 28 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 36 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 44 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 52) {
        LPDEnc_acelp_GetFixedCBIndexV2(dn,
                                       cn2,
                                       Rw,
                                       code,
                                       y2,
                                       lpd_channel_stream->acelp_icb_index[sfOffset][i_subfr / L_SUBFR],
                                       lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR],
                                       h1);
      } else if (lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 64) {
        LPDEnc_acelp_GetFixedCBIndexV2(dn,
                                       cn2,
                                       Rw,
                                       code,
                                       y2,
                                       lpd_channel_stream->acelp_icb_index[sfOffset][i_subfr / L_SUBFR],
                                       lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR],
                                       h1);
      } else {
        return iisUtil_ERROR(CDI, "invalid mode for acelp frame!");
      }
    } else {
      LPDEnc_acelp_UpdateFixedCBTarget(cn, cn, &exc[i_subfr], gain_pit);

      LPDEnc_acelp_Correlate(xn2, dn, h1);

      if (
          lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 16 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 17 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 20 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 28 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 36 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 44 || lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 52) {
        LPDEnc_acelp_GetFixedCBIndex(dn,
                                     cn,
                                     h1,
                                     code,
                                     y2,
                                     lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR],
                                     lpd_channel_stream->acelp_icb_index[sfOffset][i_subfr / L_SUBFR]);
      } else if (lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR] == 64) {
        LPDEnc_acelp_GetFixedCBIndex(dn,
                                     cn,
                                     h1,
                                     code,
                                     y2,
                                     lpdcom_NBITS_FIXED[codec_mode][i_subfr / L_SUBFR],
                                     lpd_channel_stream->acelp_icb_index[sfOffset][i_subfr / L_SUBFR]);
      } else {
        return iisUtil_ERROR(CDI, "invalid mode for acelp frame!");
      }
    }

    tmp = 0.0;
    LPDCom_tools_Preemphasise(code, TILT_CODE, L_SUBFR, &tmp);
    LPDCom_acelp_PitchSharpening(code, T0);

    LPDEnc_acelp_CalculateInnovativeCBGain(xn, y1, y2, g_corr);

    if (maxPitchLag < PIT_MAX_MAX && T0 > maxPitchLag + i_subfr) {
      float tmp_maxPitchGain = MAX_PIT_GAIN_FOR_TO_LONG_PIT_LAG;
      if (maxPitchGain > 0) {
        tmp_maxPitchGain = min(tmp_maxPitchGain, maxPitchGain);
      }
      index = quantizeGains(code, L_SUBFR, &gain_pit, &gain_code, g_corr, mean_ener_code, tmp_maxPitchGain);
    } else {
      index = quantizeGains(code, L_SUBFR, &gain_pit, &gain_code, g_corr, mean_ener_code, maxPitchGain);
    }

    lpd_channel_stream->acelp_gains[sfOffset][i_subfr / L_SUBFR] = index;

    tmp = 0.0;
    for (i = 0; i < L_SUBFR; i++) {
      tmp += code[i] * code[i];
    }
    tmp *= gain_code * gain_code;

    for (i = 0; i < L_SUBFR; i++) {
      wsyn[i + i_subfr] = y0[i] + (gain_pit * y1[i]) + (gain_code * y2[i]);
    }

    for (i = 0; i < L_SUBFR; i++) {
      exc[i + i_subfr] = gain_pit * exc[i + i_subfr] + gain_code * code[i];
    }

    LPDCom_lpc_Synthesize(p_Aq, &exc[i_subfr], &synth[i_subfr], L_SUBFR, &synth[i_subfr - M], 0);

    p_A += (M + 1);
    p_Aq += (M + 1);

    pitchGains[i_subfr / L_SUBFR] = gain_pit;
  }

  copyFLOAT(exc + lDiv - (PIT_MAX_MAX + L_INTERPOL), LPDmem->Aexc, PIT_MAX_MAX + L_INTERPOL);
  copyFLOAT(synth + lDiv - (M + 128), LPDmem->syn, M + 128);
  copyFLOAT(wsyn + lDiv - (1 + 128), LPDmem->wsyn, 1 + 128);
  copyFLOAT(p_Aq - (2 * (M + 1)), LPDmem->Aq, 2 * (M + 1));
  copyFLOAT(p_A - (2 * (M + 1)), LPDmem->Ai, 2 * (M + 1));

  mem_Txn = LPDmem->Txn[128 - 1];
  mem_Txnq = LPDmem->Txnq_fac;

  p_Aq = Aq;
  for (i_subfr = 0; i_subfr < (lDiv - 2 * L_SUBFR); i_subfr += L_SUBFR) {
    LPDCom_lpc_AWeight(p_Aq, Ap, GAMMA1, M);

    copyFLOAT(&speech[i_subfr], error, L_SUBFR);
    LPDCom_tools_Deemphasise(error, TILT_FAC, L_SUBFR, &mem_Txn);

    copyFLOAT(&synth[i_subfr], error, L_SUBFR);
    LPDCom_tools_Deemphasise(error, TILT_FAC, L_SUBFR, &mem_Txnq);

    p_Aq += (M + 1);
  }

  LPDmem->Txnq[0] = mem_Txnq;
  for (i_subfr = 0; i_subfr < (2 * L_SUBFR); i_subfr += L_SUBFR) {
    LPDCom_lpc_AWeight(p_Aq, Ap, GAMMA1, M);

    copyFLOAT(&speech[i_subfr + (lDiv - 2 * L_SUBFR)], &(LPDmem->Txn[i_subfr]), L_SUBFR);
    LPDCom_tools_Deemphasise(&(LPDmem->Txn[i_subfr]), TILT_FAC, L_SUBFR, &mem_Txn);

    copyFLOAT(&synth[i_subfr + (lDiv - 2 * L_SUBFR)], &(LPDmem->Txnq[1 + i_subfr]), L_SUBFR);
    LPDCom_tools_Deemphasise(&(LPDmem->Txnq[1 + i_subfr]), TILT_FAC, L_SUBFR, &mem_Txnq);

    p_Aq += (M + 1);
  }
  LPDmem->Txnq_fac = mem_Txnq;

  LPDCom_lpc_AWeight(p_Aq, Ap, GAMMA1, M);

  copyFLOAT(&synth[lDiv - M], error, M);
  for (i_subfr = (2 * L_SUBFR); i_subfr < (4 * L_SUBFR); i_subfr += L_SUBFR) {
    setFLOAT(0.0f, error + M, L_SUBFR);

    LPDCom_lpc_Synthesize(p_Aq, error + M, error + M, L_SUBFR, error, 0);
    copyFLOAT(error + M, &(LPDmem->Txnq[1 + i_subfr]), L_SUBFR);
    LPDCom_tools_Deemphasise(&(LPDmem->Txnq[1 + i_subfr]), TILT_FAC, L_SUBFR, &mem_Txnq);
    copyFLOAT(error + L_SUBFR, error, M);
  }

  for (i_subfr = 0; i_subfr < lDiv; i_subfr += L_SUBFR) {
    tmp = LPDCom_tools_GetGain(&wsig[i_subfr], &wsyn[i_subfr], L_SUBFR);
    if (tmp > 1.122f) tmp = 1.122f;
    if (tmp < 0.891f) tmp = 0.891f;

    for (i = 0; i < L_SUBFR; i++) wsyn[i + i_subfr] *= tmp;
  }

  *snr = LPDCom_tools_segSNR(wsig, wsyn, lDiv, L_SUBFR);

  if (LPDmem->mode > 0) {
    int max = 1;
    int a;

    for (i = 0; i < lFAC; i++) {
      a = abs(LPDmem->RE8prm[i]);
      if (a > max) max = a;
    }
    tmp = max / 100.0f;

    *snr -= tmp;
  }

  if (LPDmem->mode > 0) {
    *snr -= 0.75f;
  } else {
    *snr += 0.75f;
  }

  *snr += 1.5f * ((norm_corr * norm_corr2) - 0.5f);

  LPDmem->fac_ns = 0.5f * LPDmem->fac_ns + 0.5f * 0.25f;

  LPDmem->mode = 0;

  LPDmem->nbits = nbits;

  return (err);
}
