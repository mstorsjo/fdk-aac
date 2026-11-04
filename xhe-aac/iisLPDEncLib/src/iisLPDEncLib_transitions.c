
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

#include <stdio.h>
#include <math.h>
#include "iisLPDEncLib_transitions.h"

#ifndef FRAME_LEN_LONG
#define FRAME_LEN_LONG 1024
#endif

extern const float lpdcom_sineWindow64[64];
extern const float lpdcom_sineWindow96[96];
extern const float lpdcom_sineWindow128[128];
extern const float lpdcom_sineWindow192[192];
extern const float lpdcom_sineWindow256[256];

static void updatePastFDSynth(
    Coder_State_Plus* pAcelpEnc,
    float* pPastFDSynthesis,
    float* pPastFDOrig,
    int lowpassLine);

void LPDEnc_transition_FdAcelp(
    Coder_State_Plus* pAcelpEnc,
    float* origTimeSig,
    float* synthTime,
    int ch,
    int lowpassLine,
    int nGranuleLength,
    int lfac,
    int isFdToLpd,
    int isLpdToFd,
    int lastSubframeWasAcelp,
    char* facPrm,
    int* Nbits_fac) {
  int i = 0;
  int index = 0;
  int facPrmTmp[LFAC_1024 + 1] = {0};
  float* zir = pAcelpEnc->LPDmem.Txnq;
  float leftFacTimeData[2 * LFAC_1024 + M] = {0.0f};
  float leftFacSpec[LFAC_1024] = {0.0f};
  float facWindow[2 * LFAC_1024] = {0.0f};
  float facelp[LFAC_1024] = {0.0f};
  float* Aq = NULL;
  const float* sineWindow = NULL;
  HANDLE_TCX_DCT4 hTcxDctIV = NULL;

  (void)ch;

  *Nbits_fac = 0;

  if (isLpdToFd == 1 && lastSubframeWasAcelp == 1) {
    float tmp[2 * LFAC_1024] = {0.0f};
    float Ap[M + 1] = {0.0f};
    float ener = 0.0f;
    float gain = 0.0f;
    int leftStart = 0;

    LPDCom_tcx_OpenDCT4(&hTcxDctIV, lfac);
    switch (lfac) {
      case 48:
        sineWindow = lpdcom_sineWindow96;
        break;
      case 64:
        sineWindow = lpdcom_sineWindow128;
        break;
      case 96:
        sineWindow = lpdcom_sineWindow192;
        break;
      case 128:
        sineWindow = lpdcom_sineWindow256;
        break;
      default:
        break;
    }

    for (i = 0; i < lfac; i++) {
      facWindow[i] = sineWindow[i] * sineWindow[(2 * lfac) - 1 - i];
      facWindow[lfac + i] = 1.0f - (sineWindow[lfac + i] * sineWindow[lfac + i]);
    }

    leftStart = (nGranuleLength / 2) - lfac - M;
    copyFLOAT(&origTimeSig[leftStart], leftFacTimeData, 2 * lfac + M);

    subFLOAT(&leftFacTimeData[lfac + M], &synthTime[leftStart + lfac + M], &leftFacTimeData[lfac + M], lfac);

    smulFLOAT((1 << 15), leftFacTimeData, leftFacTimeData, 2 * lfac + M);

    subFLOAT(&leftFacTimeData[lfac], &zir[1 + 128 - M], &leftFacTimeData[lfac], M);

    for (i = 0; i < lfac; i++) {
      facelp[i] = zir[1 + 128 + i] * facWindow[lfac + i] + zir[1 + 128 - 1 - i] * facWindow[lfac - 1 - i];
    }

    {
      float tmp_ener = 0.0f;
      ener = 0.0f;
      for (i = 0; i < lfac; i++) {
        ener += leftFacTimeData[i + M + lfac] * leftFacTimeData[i + M + lfac];
      }
      ener *= 2.0f;
      tmp_ener = 0.0f;
      for (i = 0; i < lfac; i++) {
        tmp_ener += facelp[i] * facelp[i];
      }

      if (tmp_ener > ener) {
        gain = (float)sqrt(ener / tmp_ener);
      } else {
        gain = 1.0f;
      }

      for (i = 0; i < lfac; i++) {
        leftFacTimeData[i + M + lfac] -= gain * facelp[i];
      }
    }

    Aq = pAcelpEnc->LPDmem.Aq;
    Aq += M + 1;
    LPDCom_lpc_AWeight(Aq, Ap, GAMMA1, M);

    LPDCom_lpc_Analyze(Ap, leftFacTimeData + M + lfac, tmp + lfac, lfac);
    smulFLOAT((2.0f / (float)lfac), tmp, tmp, 2 * lfac);
    setFLOAT(0.0, tmp, lfac);

    LPDCom_tcx_ApplyDCT4(hTcxDctIV, &tmp[lfac], leftFacSpec);
    for (i = lowpassLine; i < lfac; i++) leftFacSpec[i] = 0.0f;

    gain = LPDEnc_tcx_GetGlobalGain(leftFacSpec, 240, lfac);

    index = (int)floor(0.5f + (28.0f * (float)log10(gain)));
    if (index < 0) index = 0;
    if (index > 127) index = 127;
    LPDEnc_bs_IntToBin(index, 7, 0, (unsigned char*)facPrm);
    *Nbits_fac += 7;
    gain = (float)pow(10.0f, ((float)index) / 28.0f);
    for (i = 0; i < lfac; i++)
      leftFacSpec[i] /= gain;

    for (i = 0; i < lfac; i += 8) {
      LPDCom_vq_GetNearestRE8Vector(&leftFacSpec[i], &facPrmTmp[i]);
    }

    *Nbits_fac += LPDEnc_bs_EncodeFac(facPrmTmp, 7, (unsigned char*)facPrm, lfac);

    LPDCom_tcx_CloseDCT4(&hTcxDctIV);
  } else {
    *Nbits_fac = 0;
  }

  if (isFdToLpd == 1) {
    updatePastFDSynth(pAcelpEnc,
                      &synthTime[nGranuleLength - 1],
                      origTimeSig + nGranuleLength - 1,
                      lowpassLine);
  }
}

static void updatePastFDSynth(
    Coder_State_Plus* pAcelpEnc,
    float* pPastFDSynthesis,
    float* pPastFDOrig,
    int lowpassLine) {
  smulFLOAT((1 << 15), pPastFDSynthesis + 1 + 2 * pAcelpEnc->lDiv - (PIT_MAX_MAX + L_INTERPOL + M), pAcelpEnc->fdSynth, PIT_MAX_MAX + L_INTERPOL + M);
  smulFLOAT((1 << 15), pPastFDOrig - M, pAcelpEnc->fdOrig, FRAME_LEN_LONG / 2 + 1 + M);

  pAcelpEnc->lowpassLine = lowpassLine;
}
