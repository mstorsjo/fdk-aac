
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
#include <limits.h>

#include "iisLPDComLib_tcx_Tools.h"
#include "iisLPDEncLib_tcx_Main.h"
#include "iisLPDEncLib_tcx_Tools.h"
#include "iisLPDEncLib_bs.h"
#include "iisLPDComLib_constants.h"
#include "iisLPDEncLib_vq_RE8.h"
#include "iisLPDComLib_vq_RE8.h"
#include "iisLPDComLib_lpc.h"
#include "iisLPDComLib_tools.h"

#define MIN_TCX_GLOBAL_GAIN 100.0f

static int const ibark2[11] = {0, 8, 16, 24, 36, 50, 68, 92, 126, 176, 256};

HANDLE_ERROR_INFO LPDEnc_tcx_Encode(
    float Ai[],
    float A[],
    float speech[],
    float wsig[],
    float synth[],
    float wsyn[],
    Coder_State_Plus* const st,
    const int sfOffset,
    int L_frame,
    int lDiv,
    int nb_bits,
    LPD_state* LPDmem,
    int* n_param,
    int lfacPrev,
    int lfacNext,
    float denom_nbits,
    const int bDependentWindow,
    const int bUseNoiseFilling,
    float* tcx_snr,
    LPD_CHANNEL_STREAM* lpd_channel_stream) {
  int i, j, k, i1, i2, n, mode, i_subfr, lg, lext, index, sqTargetBits;
  float tmp, gain, fac_ns, ener, gain_tcx, nsfill_en_thres, tcxBitrateLevel;
  float *p_A, Ap[M + 1];
  const float* sinewindowPrev;
  const float* sinewindowNext;
  float mem_xnq;
  float* xn;
  float xn1[2 * LFAC_1024], xn_buf[128 + L_FRAME_1024 + 128];
  float x[N_MAX] = {0.f}, x_tmp[N_MAX], en[N_MAX];
  float sqGain;
  float alfd_gains[N_MAX / (4 * 8)];
  float sqEnc[N_MAX] = {0.f};
  int sqQ[N_MAX] = {0};
  float sqErrorNrg;
  int maxK;
  float gainlpc[2 * FDNS_NPTS_1024], gainlpc2[2 * FDNS_NPTS_1024];
  float xn2[2 * LFAC_1024], facwindow[2 * LFAC_1024];
  float facelp[LFAC_1024];
  float x1[LFAC_1024], x2[LFAC_1024];
  float gainPrev, gainNext;
  int y[LFAC_1024];
  int TTT;
  float tmp_snr, dsnr;

  int lFAC;
  HANDLE_ERROR_INFO err = noError;

  (void)sfOffset;
  (void)bDependentWindow;
  (void)bUseNoiseFilling;

  lFAC = lDiv / 2;

  setFLOAT(0.0f, xn_buf, 128 + L_frame + 128);

  mode = L_frame / lDiv;
  if (mode > 2) {
    mode = 3;
  }

  if (lfacPrev == 64) {
    sinewindowPrev = lpdcom_sineWindow128;
  } else if (lfacPrev == 48) {
    sinewindowPrev = lpdcom_sineWindow96;
  } else if (lfacPrev == 96) {
    sinewindowPrev = lpdcom_sineWindow192;
  } else {
    sinewindowPrev = lpdcom_sineWindow256;
  }
  if (lfacNext == 64) {
    sinewindowNext = lpdcom_sineWindow128;
  } else if (lfacNext == 48) {
    sinewindowNext = lpdcom_sineWindow96;
  } else if (lfacNext == 96) {
    sinewindowNext = lpdcom_sineWindow192;
  } else {
    sinewindowNext = lpdcom_sineWindow256;
  }

  lg = L_frame;
  lext = lFAC;

  xn = xn_buf + lFAC;

  *n_param = lg;

  sqTargetBits = nb_bits - 3 - 7;
  if (!bUseNoiseFilling) {
    sqTargetBits += 3;
  }

  for (i = 0; i < lFAC; i++) {
    xn_buf[i] = LPDmem->Txn[i + 128 - lFAC];
  }

  copyFLOAT(speech, xn, L_frame + lFAC);

  tmp = xn[-1];

  LPDCom_tools_Deemphasise(xn, TILT_FAC, L_frame, &tmp);

  copyFLOAT(&xn[L_frame - 128], LPDmem->Txn, 128);

  copyFLOAT(&speech[L_frame], &xn[L_frame], lext);
  LPDCom_tools_Deemphasise(&xn[L_frame], TILT_FAC, lext, &tmp);

  for (i = 0; i < M + lfacPrev; i++) {
    xn1[i] = xn_buf[lFAC - M + i];
  }
  for (i = 0; i < M + lfacNext; i++) {
    xn2[i] = xn_buf[L_frame - M + i];
  }

  if (LPDmem->mode >= -1) {
    for (i = 0; i < lFAC - lfacPrev; i++) {
      xn_buf[i] = 0.0f;
    }
    for (i = lFAC - lfacPrev; i < (lFAC + lfacPrev); i++) {
      xn_buf[i] *= sinewindowPrev[i - lFAC + lfacPrev];
    }
    for (i = 0; i < (2 * lfacNext); i++) {
      xn_buf[L_frame + lFAC - lfacNext + i] *= sinewindowNext[(2 * lfacNext) - 1 - i];
    }
    for (i = 0; i < lFAC - lfacNext; i++) {
      xn_buf[L_frame + lFAC + lfacNext + i] = 0.0f;
    }
  }

  LPDCom_tcx_ApplyMDCT(st->hTcxMdct, xn_buf, x, (2 * lFAC), L_frame - (2 * lFAC), (2 * lFAC));

  smulFLOAT(1.0f, x, x, lg);

  LPDCom_lpc_AWeight(A + (M + 1),
                     Ap,
                     GAMMA1,
                     M);

  LPDCom_tcx_LpcToSpecGains(st->hTcxFft,
                            Ap,
                            M,
                            gainlpc,
                            (FDNS_NPTS_1024 * lDiv) / L_DIV_1024);

  LPDCom_lpc_AWeight(A + (2 * (M + 1)),
                     Ap,
                     GAMMA1,
                     M);

  LPDCom_tcx_LpcToSpecGains(st->hTcxFft,
                            Ap,
                            M,
                            gainlpc2,
                            (FDNS_NPTS_1024 * lDiv) / L_DIV_1024);

  LPDEnc_tcx_InvSpectralNoiseShaping(x, lg, (FDNS_NPTS_1024 * lDiv) / L_DIV_1024, gainlpc, gainlpc2);

  for (i = 0; i < lg; i++) {
    x_tmp[i] = x[i];
  }

  tmp = 0.01f;
  for (j = 0; j < 10; j++) {
    i1 = (ibark2[j] * lg) / 256;
    i2 = (ibark2[j + 1] * lg) / 256;
    n = (i2 - i1);

    ener = 0.0f;
    for (i = i1; i < i2; i++) ener += x[i] * x[i];

    tmp += ener * 2.0f / (float)(lg * n);
  }
  ener = 10.0f * (float)log10(0.1f * tmp);

  tmp = ener / 60.0f;
  tcxBitrateLevel = 0.5f + 0.5f * tmp;

  if (tcxBitrateLevel > 1.25f) {
    tcxBitrateLevel = 1.25f;
  }
  if (tcxBitrateLevel < 0.25f) {
    tcxBitrateLevel = 0.25f;
  }

  sqTargetBits = (int)(tcxBitrateLevel * (float)sqTargetBits);

  LPDEnc_tcx_AdaptLowFrequenciesEmphasis(x,
                                         lg);

  sqGain = LPDEnc_tcx_GetGlobalGain(x, sqTargetBits, lg);
  if (sqGain < MIN_TCX_GLOBAL_GAIN) {
    sqGain = MIN_TCX_GLOBAL_GAIN;
  }

  for (i = 0; i < lg; i++) {
    sqEnc[i] = x[i] / sqGain;

    if (sqEnc[i] > 0.f) {
      sqQ[i] = ((int)(0.5f + sqEnc[i]));
    } else {
      sqQ[i] = ((int)(-0.5f + sqEnc[i]));
    }
  }

  for (; i < L_frame; i++) {
    sqEnc[i] = 0.0f;
  }

  for (i = 0; i < lg; i++) {
    lpd_channel_stream->tcx_quant[i] = sqQ[i];

    x[i] = (float)sqQ[i];
  }

  for (i = 0; i < lg; i++) {
    en[i] = x[i] * x[i];
  }

  if (mode == 3)
    tmp = 0.9441f;
  else if (mode == 2)
    tmp = 0.8913f;
  else
    tmp = 0.7943f;

  ener = 0.0f;
  for (i = 0; i < lg; i++) {
    if (en[i] > ener) ener = en[i];
    en[i] = ener;
    ener *= tmp;
  }
  ener = 0.0f;
  for (i = lg - 1; i >= 0; i--) {
    if (en[i] > ener) ener = en[i];
    en[i] = ener;
    ener *= tmp;
  }

  nsfill_en_thres = 0.707f;

  tmp = 0.0625f;
  k = 1;
  for (i = 0; i < lg; i++) {
    if (en[i] <= nsfill_en_thres) {
      tmp += sqEnc[i] * sqEnc[i];
      k++;
    }
  }
  fac_ns = (float)sqrt(tmp / (float)k);

  LPDCom_tcx_AdaptLowFrequencyDeemphasis(x,
                                         lg,
                                         alfd_gains);

  gain_tcx = LPDCom_tools_GetGainExhaustiveSearch(x_tmp, x, lg);
  if (gain_tcx == 0.0f) {
    gain_tcx = sqGain;
  }

  ener = 0.0001f;
  for (i = 0; i < lg; i++) {
    tmp = x_tmp[i] - gain_tcx * x[i];
    ener += tmp * tmp;
  }

  tmp = (float)sqrt((ener * (2.0f / (float)lg)) / (float)lg);

  for (i = 0; i < L_frame; i++) {
    wsyn[i] = wsig[i] + tmp;
  }

  tmp_snr = LPDCom_tools_segSNR(wsig, wsyn, L_frame, L_SUBFR);
  dsnr = 3.0f + 7.0f + (float)sqTargetBits;

  dsnr *= (float)lDiv / (float)L_frame;
  dsnr -= denom_nbits;
  dsnr *= 6.0f / (float)lDiv;
  if (dsnr > 0) {
    dsnr /= 2.0f;
  }

  if (*tcx_snr >= tmp_snr - dsnr) {
    *tcx_snr = tmp_snr - dsnr;
    LPDmem->nbits = 3 + 7 + sqTargetBits;

    return noError;
  } else {
    *tcx_snr = tmp_snr - dsnr;
  }

  LPDmem->fac_ns = 0.5f * LPDmem->fac_ns + 0.5f * fac_ns;

  ener = 0.01f;
  for (i = 0; i < lg; i++) {
    ener += x[i] * x[i];
  }
  tmp = 2.0f * (float)sqrt(ener) / (float)lg;
  gain = gain_tcx * tmp;

  index = (int)floor(0.5f + 28.0f * (float)log10(gain));
  if (index < 0) {
    index = 0;
  } else if (index > 127) {
    index = 127;
  }

  lpd_channel_stream->tcx_global_gain[sfOffset] = index;

  gain_tcx = (float)pow(10.0f, (float)index / 28.0f) / tmp;

  sqErrorNrg = 0.f;
  n = 0;
  for (k = lg / 2; k < lg;) {
    tmp = 0.f;

    maxK = MIN(lg, k + 8);
    for (i = k; i < maxK; i++) {
      tmp += sqQ[i] * sqQ[i];
    }
    if (tmp == 0.f) {
      tmp = 0.f;
      for (i = k; i < maxK; i++) {
        tmp += sqEnc[i] * sqEnc[i];
      }

      sqErrorNrg += (float)log10((tmp / (double)8) + 0.000000001);
      n += 1;
    }
    k = maxK;
  }
  if (n > 0)
    fac_ns = (float)pow(10., sqErrorNrg / (double)(2 * n));
  else
    fac_ns = 0.f;

  tmp = 8.0f - (16.0f * fac_ns);

  index = (int)floor(tmp + 0.5);
  if (index < 0) {
    index = 0;
  }
  if (index > 7) {
    index = 7;
  }

  lpd_channel_stream->tcx_noise_factor[sfOffset] = index;

  LPDCom_tcx_SpectralNoiseShaping(x, lg, (FDNS_NPTS_1024 * lDiv) / L_DIV_1024, gainlpc, gainlpc2);

  LPDCom_tcx_ApplyInvMDCT(st->hTcxMdct, x, xn_buf, (2 * lFAC), L_frame - (2 * lFAC), (2 * lFAC));
  smulFLOAT((2.0f / lg), xn_buf, xn_buf, L_frame + (2 * lFAC));

  for (i = 0; i < lfacPrev; i++) {
    facwindow[i] = sinewindowPrev[i] * sinewindowPrev[(2 * lfacPrev) - 1 - i];
    facwindow[lfacPrev + i] = 1.0f - (sinewindowPrev[lfacPrev + i] * sinewindowPrev[lfacPrev + i]);
  }

  for (i = 0; i < lfacPrev; i++) {
    xn1[M + i] -= sqGain * xn_buf[lFAC + i] * sinewindowPrev[lfacPrev + i];
  }

  for (i = 0; i < lfacNext; i++) {
    xn2[M + i] -= sqGain * xn_buf[i + L_frame] * sinewindowNext[(2 * lfacNext) - 1 - i];
  }

  for (i = 0; i < M; i++) {
    xn1[i] -= LPDmem->Txnq[1 + 128 - M + i];
    xn2[i] -= sqGain * xn_buf[L_frame - M + i];
  }

  for (i = 0; i < lfacPrev; i++) {
    facelp[i] = LPDmem->Txnq[1 + 128 + i] * facwindow[lfacPrev + i] + LPDmem->Txnq[1 + 128 - 1 - i] * facwindow[lfacPrev - 1 - i];
  }

  ener = 0.0f;
  for (i = 0; i < lfacPrev; i++) {
    ener += xn1[M + i] * xn1[M + i];
  }

  ener *= 2.0f;
  tmp = 0.0f;
  for (i = 0; i < lfacPrev; i++) {
    tmp += facelp[i] * facelp[i];
  }

  if (tmp > ener) {
    gain = (float)sqrt(ener / tmp);
  } else {
    gain = 1.0f;
  }

  for (i = 0; i < lfacPrev; i++) {
    xn1[M + i] -= gain * facelp[i];
  }

  LPDCom_lpc_AWeight(A + (M + 1), Ap, GAMMA1, M);

  LPDCom_lpc_Analyze(Ap, xn1 + M, x1, lfacPrev);

  LPDCom_lpc_AWeight(A + (2 * (M + 1)), Ap, GAMMA1, M);

  LPDCom_lpc_Analyze(Ap, xn2 + M, x2, lfacNext);

  if (((lDiv == 192) && (((lfacPrev != 96) && (lfacPrev != 48)) || ((lfacNext != 96) && (lfacNext != 48)))) ||
      ((lDiv == 256) && (((lfacPrev != 128) && (lfacPrev != 64)) || ((lfacNext != 128) && (lfacNext != 64))))) {
    return iisUtil_ERROR(CDI, "Invalid value for lfacPrev or lfacNext");
  }

  if (lfacPrev == 128) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 128), x1, x1);
  } else if (lfacPrev == 96) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 96), x1, x1);
  } else if (lfacPrev == 48) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 48), x1, x1);
  } else {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 64), x1, x1);
  }
  if (lfacNext == 128) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 128), x2, x2);
  } else if (lfacNext == 96) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 96), x2, x2);
  } else if (lfacNext == 48) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 48), x2, x2);
  } else {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 64), x2, x2);
  }

  gainPrev = sqGain * 0.5f * (float)sqrt(((float)lfacPrev) / (float)L_frame);
  gainNext = sqGain * 0.5f * (float)sqrt(((float)lfacNext) / (float)L_frame);

  for (i = 0; i < lfacPrev; i++) {
    x1[i] /= gainPrev;
  }
  for (i = 0; i < lfacNext; i++) {
    x2[i] /= gainNext;
  }
  for (i = 0; i < lfacPrev / 4; i++) {
    k = i * lg / (8 * lfacPrev);
    x1[i] /= alfd_gains[k];
  }
  for (i = 0; i < lfacNext / 4; i++) {
    k = i * lg / (8 * lfacNext);
    x2[i] /= alfd_gains[k];
  }

  for (i = 0; i < lfacNext; i += 8) {
    LPDCom_vq_GetNearestRE8Vector(&x2[i], &y[i]);
  }
  for (i = 0; i < lfacNext; i++) {
    LPDmem->RE8prm[i] = y[i];
  }
  for (i = 0; i < lfacNext; i++) {
    x2[i] = (float)y[i];
  }

  for (i = 0; i < lfacPrev; i += 8) {
    LPDCom_vq_GetNearestRE8Vector(&x1[i], &y[i]);
  }
  for (i = 0; i < lfacPrev; i++) {
    x1[i] = (float)y[i];
  }

  gainPrev = gain_tcx * 0.5f * (float)sqrt(((float)lfacPrev) / (float)L_frame);
  gainNext = gain_tcx * 0.5f * (float)sqrt(((float)lfacNext) / (float)L_frame);

  for (i = 0; i < lfacPrev; i++) {
    x1[i] *= gainPrev;
  }
  for (i = 0; i < lfacNext; i++) {
    x2[i] *= gainNext;
  }
  for (i = 0; i < lfacPrev / 4; i++) {
    k = i * lg / (8 * lfacPrev);
    x1[i] *= alfd_gains[k];
  }
  for (i = 0; i < lfacNext / 4; i++) {
    k = i * lg / (8 * lfacNext);
    x2[i] *= alfd_gains[k];
  }

  if (lfacPrev == 128) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 128), x1, xn1);
  } else if (lfacPrev == 96) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 96), x1, xn1);
  } else if (lfacPrev == 48) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 48), x1, xn1);
  } else {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 64), x1, xn1);
  }
  if (lfacNext == 128) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 128), x2, xn2);
  } else if (lfacNext == 96) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 96), x2, xn2);
  } else if (lfacNext == 48) {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 48), x2, xn2);
  } else {
    LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(st->hTcxMdct, 64), x2, xn2);
  }

  smulFLOAT((2.0f / (float)lfacPrev), xn1, xn1, lfacPrev);
  smulFLOAT((2.0f / (float)lfacNext), xn2, xn2, lfacNext);

  setFLOAT(0.0f, xn1 + lfacPrev, lfacPrev);
  setFLOAT(0.0f, xn2 + lfacNext, lfacNext);

  LPDCom_lpc_AWeight(A + (M + 1), Ap, GAMMA1, M);
  LPDCom_lpc_Synthesize(Ap, xn1, xn1, 2 * lfacPrev, xn1 + lfacPrev, 0);

  LPDCom_lpc_AWeight(A + (2 * (M + 1)), Ap, GAMMA1, M);
  LPDCom_lpc_Synthesize(Ap, xn2, xn2, lfacNext, xn2 + lfacNext, 0);

  for (i = 0; i < lfacPrev; i++) {
    xn1[i] += facelp[i];
  }

  for (i = 0; i < L_frame + (2 * lFAC); i++) {
    xn_buf[i] *= gain_tcx;
  }
  if (LPDmem->mode >= -1) {
    for (i = 0; i < (2 * lfacPrev); i++) {
      xn_buf[i + lFAC - lfacPrev] *= sinewindowPrev[i];
    }
    for (i = 0; i < lFAC - lfacPrev; i++) {
      xn_buf[i] = 0.0f;
    }
  }
  for (i = 0; i < (2 * lfacNext); i++) {
    xn_buf[i + L_frame + lFAC - lfacNext] *= sinewindowNext[(2 * lfacNext) - 1 - i];
  }
  for (i = 0; i < lFAC - lfacNext; i++) {
    xn_buf[i + L_frame + lFAC + lfacNext] = 0.0f;
  }

  if (LPDmem->mode != 0) {
    for (i = 0; i < (2 * lFAC); i++) {
      xn_buf[i] += LPDmem->Txnq[1 + 128 - lFAC + i];
    }
    mem_xnq = LPDmem->Txnq[128 - lFAC];
  } else {
    for (i = 0; i < lfacPrev; i++) {
      lpd_channel_stream->fac_data[sfOffset][i] = y[i];
    }

    for (i = 0; i < (2 * lfacPrev); i++) {
      xn_buf[i + lFAC] += xn1[i];
    }
    mem_xnq = LPDmem->Txnq[128];
  }

  copyFLOAT(xn_buf + L_frame + lFAC - 128 - 1, LPDmem->Txnq, 1 + 256);

  for (i = 0; i < lfacNext; i++) {
    xn_buf[i + L_frame + (lFAC - lfacNext)] += xn2[i];
  }

  if (LPDmem->mode > 0) {
    LPDCom_tools_Preemphasise(xn_buf, TILT_FAC, lFAC, &mem_xnq);

    p_A = LPDmem->Aq;

    TTT = lFAC % L_SUBFR;
    if (TTT != 0) {
      copyFLOAT(&xn_buf[0], &(LPDmem->syn[M + 128 - lFAC]), TTT);
      LPDCom_lpc_Analyze(p_A, &(LPDmem->syn[M + 128 - lFAC]), &(LPDmem->Aexc[(PIT_MAX_MAX + L_INTERPOL) - lFAC]), TTT);

      p_A += (M + 1);
    }

    for (i_subfr = TTT; i_subfr < lFAC; i_subfr += L_SUBFR) {
      copyFLOAT(&xn_buf[i_subfr], &(LPDmem->syn[M + 128 - lFAC + i_subfr]), L_SUBFR);
      LPDCom_lpc_Analyze(p_A, &(LPDmem->syn[M + 128 - lFAC + i_subfr]), &(LPDmem->Aexc[(PIT_MAX_MAX + L_INTERPOL) - lFAC + i_subfr]), L_SUBFR);
      p_A += (M + 1);
    }

    p_A = LPDmem->Ai;
    for (i_subfr = 0; i_subfr < lFAC; i_subfr += L_SUBFR) {
      LPDCom_lpc_AWeight(p_A, Ap, GAMMA1, M);
      LPDCom_lpc_Analyze(Ap, &(LPDmem->syn[M + 128 - lFAC + i_subfr]), &(LPDmem->wsyn[1 + 128 - lFAC + i_subfr]), L_SUBFR);
      p_A += (M + 1);
    }

    tmp = LPDmem->wsyn[0 + 128 - lFAC];
    LPDCom_tools_Deemphasise(&(LPDmem->wsyn[1 + 128 - lFAC]), TILT_FAC, lFAC, &tmp);
  }

  k = ((L_frame / L_SUBFR) - 2) * (M + 1);
  copyFLOAT(Ai + k, LPDmem->Ai, 2 * (M + 1));

  copyFLOAT(A + (2 * (M + 1)), LPDmem->Aq, M + 1);
  copyFLOAT(LPDmem->Aq, LPDmem->Aq + (M + 1), M + 1);

  copyFLOAT(&(LPDmem->syn[M]), synth - 128, 128);
  LPDmem->Txnq_fac = xn[L_frame - 1];

  LPDCom_tools_Preemphasise(xn, TILT_FAC, L_frame, &mem_xnq);

  for (i_subfr = 0; i_subfr < L_frame; i_subfr += L_SUBFR) {
    copyFLOAT(&xn[i_subfr], &synth[i_subfr], L_SUBFR);
    LPDCom_lpc_Analyze(A + (2 * (M + 1)), &synth[i_subfr], &xn[i_subfr], L_SUBFR);
  }
  copyFLOAT(synth + L_frame - (M + 128), LPDmem->syn, (M + 128));

  if (L_frame < PIT_MAX_MAX + L_INTERPOL) {
    copyFLOAT(LPDmem->Aexc + L_frame, x, PIT_MAX_MAX + L_INTERPOL - L_frame);
    copyFLOAT(x, LPDmem->Aexc, PIT_MAX_MAX + L_INTERPOL - L_frame);
    copyFLOAT(xn, LPDmem->Aexc + (PIT_MAX_MAX + L_INTERPOL) - L_frame, L_frame);
  } else {
    copyFLOAT(xn + L_frame - (PIT_MAX_MAX + L_INTERPOL), LPDmem->Aexc, (PIT_MAX_MAX + L_INTERPOL));
  }

  copyFLOAT(&(LPDmem->wsyn[1]), wsyn - 128, 128);

  p_A = Ai;
  for (i_subfr = 0; i_subfr < L_frame; i_subfr += L_SUBFR) {
    LPDCom_lpc_AWeight(p_A, Ap, GAMMA1, M);
    LPDCom_lpc_Analyze(Ap, &synth[i_subfr], &wsyn[i_subfr], L_SUBFR);
    p_A += (M + 1);
  }
  tmp = wsyn[-1];
  LPDCom_tools_Deemphasise(wsyn, TILT_FAC, L_frame, &tmp);

  copyFLOAT(wsyn + L_frame - (1 + 128), LPDmem->wsyn, (1 + 128));

  LPDmem->mode = mode;

  LPDmem->nbits = 3 + 7 + sqTargetBits;

  return err;
}

void LPDEnc_tcx_FdFAC(
    HANDLE_TCX_MDCT hTcxMdct,
    float* orig,
    int lDiv,
    int lfac,
    int lowpassLine,
    int targetBitrate,
    float* synth,
    float* Aq,
    unsigned char* serialFac,
    int* nBitsFac) {
  float xn2[2 * LFAC_1024 + M];
  float facDec[2 * LFAC_1024];
  float rightFacSpec[LFAC_1024];
  float Ap[M + 1];
  float gain;
  float x2[LFAC_1024];
  float x[LFAC_1024];
  int param[LFAC_1024 + 1];
  int i, index;
  int nBitsEnc = 0;
  int rightStart = 2 * lDiv - lfac;
  int rightStartSynth = PIT_MAX_MAX + L_INTERPOL + M - lfac;

#define MAX_FACDATA_SIZE (1024)

  setFLOAT(0.0f, xn2, M);

  copyFLOAT(&orig[rightStart], xn2 + M, lfac);
  subFLOAT(xn2 + M, &synth[rightStartSynth], xn2 + M, lfac);

  LPDCom_lpc_AWeight(Aq, Ap, GAMMA1, M);
  LPDCom_lpc_Analyze(Ap, xn2 + M, x2, lfac);
  smulFLOAT((2.0f / (float)lfac), x2, x2, lfac);

  LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(hTcxMdct, lfac), x2, rightFacSpec);

  for (i = lowpassLine; i < lfac; i++) {
    rightFacSpec[i] = 0.0f;
  }

  gain = LPDEnc_tcx_GetGlobalGain(rightFacSpec, targetBitrate, lfac);
  index = (int)floor(0.5f + (28.0f * (float)log10(gain)));

  if (index < 0) {
    index = 0;
  }

  if (index > 127) {
    index = 127;
  }

  param[0] = index;
  gain = (float)pow(10.0f, ((float)index) / 28.0f);

  for (i = 0; i < lfac; i++) {
    rightFacSpec[i] /= gain;
  }
  for (i = 0; i < lfac; i += 8) {
    LPDCom_vq_GetNearestRE8Vector(&rightFacSpec[i], &param[i + 1]);
  }

  LPDEnc_bs_IntToBin(index, 7, 0, serialFac);

  nBitsEnc += 7;
  nBitsEnc += LPDEnc_bs_EncodeFac(&param[1], 7, serialFac, lfac);

  for (i = 0; i < lfac; i++) {
    x[i] = (float)param[i + 1] * gain;
  }

  LPDCom_tcx_ApplyDCT4(LPDCom_tcx_GetDCT4Handle(hTcxMdct, lfac), x, xn2);

  setFLOAT(0.0f, xn2 + lfac, lfac);
  LPDCom_lpc_Synthesize(Ap, xn2, facDec, 2 * lfac, xn2 + lfac, 0);

  *nBitsFac = nBitsEnc;
  for (i = 0; i < lfac; i++) {
    synth[rightStartSynth + i] += facDec[i];
  }
}
