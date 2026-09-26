
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

#include <math.h>
#include "iisLPDComLib_lpc.h"
#include "iisLPDEncLib_lpc_LPCAnalysis.h"
#include "iisLPDComLib_tcx_Tools.h"
#include "iisLPDEncLib_lpc_Quantize.h"
#include "iisLPDComLib_constants.h"
#include "mathlib.h"

static HANDLE_ERROR_INFO LPDEnc_lpc_LPC0Calc(
    float const *const speech,
    const int windowSize,
    float const *const analysisWin,
    float *lspOld,
    float *lsfOld,
    float *lsp,
    float *lpc) {
  HANDLE_ERROR_INFO error = noError;
  int i;
  float autocorr[M + 1];

  if (speech == NULL || analysisWin == NULL || lsp == NULL || lspOld == NULL || lsfOld == NULL) {
    error = iisUtil_ERROR(CDI, "NULL pointer");
  }

  if (error == noError) {
    for (i = 0; i < M; i++) {
      lspOld[i] = (float)cos(3.141592654 * (float)(i + 1) / (float)(M + 1));
    }

    LPDCom_lpc_Autocorrelate(&speech[-(windowSize / 2)], autocorr, M, windowSize, analysisWin);
    LPDCom_lpc_ApplyLagWindow(autocorr, M);
    LPDCom_lpc_LevinsonDurbin(lpc, autocorr, M);
    LPDCom_lpc_LPCToLSP(lpc, lsp, lspOld);
    copyFLOAT(lsp, lspOld, M);

    LPDCom_lpc_LSPToLSF(lsp, lsfOld, M);
  }

  return error;
}

static HANDLE_ERROR_INFO LPDEnc_lpc_LPCCalc(
    float const *const speech,
    const int windowSize,
    float const *const analysisWin,
    float *lspOld,
    float *lsfOld,
    float *lsp,
    float *lsf,
    const int lDiv,
    const int nbDiv,
    const int nbSubfr,
    float *A,
    float *Ap) {
  HANDLE_ERROR_INFO error = noError;

  if (speech == NULL || analysisWin == NULL || lsp == NULL || lsf == NULL || lspOld == NULL || lsfOld == NULL) {
    error = iisUtil_ERROR(CDI, "NULL pointer");
  }

  if (error == noError) {
    int i;
    float autocorr[M + 1];

    copyFLOAT(lspOld, lsp, M);
    copyFLOAT(lsfOld, lsf, M);

    for (i = 0; i < nbDiv; i++) {
      float *p_Ap = &Ap[(i + 1) * (M + 1)];
      float *p_lspAct = &lsp[(i + 1) * M];
      float *p_lspLast = &lsp[i * M];
      float *p_lsfAct = &lsf[(i + 1) * M];

      LPDCom_lpc_Autocorrelate(&speech[((i + 1) * lDiv) - (windowSize / 2)], autocorr, M, windowSize, analysisWin);
      LPDCom_lpc_ApplyLagWindow(autocorr, M);
      LPDCom_lpc_LevinsonDurbin(p_Ap, autocorr, M);
      LPDCom_lpc_LPCToLSP(p_Ap, p_lspAct, p_lspLast);

      LPDCom_tcx_InterpolateAcelpLSPs(p_lspLast, p_lspAct, &A[i * nbSubfr * (M + 1)], nbSubfr, M);

      LPDCom_lpc_LSPToLSF(p_lspAct, p_lsfAct, M);
    }
  }

  return error;
}

HANDLE_ERROR_INFO LPDEnc_lpc_LPCAnalysis(
    float const *const speech,
    const int windowSize,
    Coder_State_Plus *CoderMemoryState,
    const int isAceStart,
    float *A,
    float *lsp_q,
    int *qIndices,
    int *nb_qIndices,
    int *nb_qBits,
    const int bIndepFromLPC0) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  float Ap[(NB_DIV + 1) * (M + 1)];

  float lsp[(NB_DIV + 1) * M];

  float lsf[(NB_DIV + 1) * M];
  float lsf_q[(NB_DIV + 1) * M];
  float *analysisWin = CoderMemoryState->window;
  float *lspOld = CoderMemoryState->lspold;
  float *lsfOld = CoderMemoryState->lsfold;

  if (CoderMemoryState == NULL || speech == NULL || A == NULL || lsp_q == NULL || qIndices == NULL || nb_qIndices == NULL || nb_qBits == NULL) {
    error = iisUtil_ERROR(CDI, "NULL pointer");
  }
  if (analysisWin == NULL || lspOld == NULL || lsfOld == NULL) {
    error = iisUtil_ERROR(CDI, "NULL pointer");
  }

  if (error == noError) {
    if (isAceStart) {
      error = LPDEnc_lpc_LPC0Calc(speech,
                                  windowSize,
                                  analysisWin,
                                  lspOld,
                                  lsfOld,
                                  lsp,
                                  Ap);
    }
  }

  if (error == noError) {
    error = LPDEnc_lpc_LPCCalc(speech,
                               windowSize,
                               analysisWin,
                               lspOld,
                               lsfOld,
                               lsp,
                               lsf,
                               CoderMemoryState->lDiv,
                               CoderMemoryState->nbDiv,
                               CoderMemoryState->nbSubfr,
                               A,
                               Ap);
  }

  if (error == noError) {
    if (!isAceStart) {
      LPDCom_lpc_LSPToLSF(CoderMemoryState->lspold_q, lsf_q, M);
      copyFLOAT(CoderMemoryState->lspold_q, lsp_q, M);
    }

    LPDEnc_lpc_Quantize(&lsf[M],
                        &lsf_q[M],
                        isAceStart,
                        qIndices,
                        nb_qIndices,
                        nb_qBits,
                        CoderMemoryState->nbDiv,
                        bIndepFromLPC0);

    for (i = 0; i < CoderMemoryState->nbDiv; i++) {
      float *ptrLsf_q = &lsf_q[(i + 1) * M];
      float *ptrLsp_q = &lsp_q[(i + 1) * M];

      LPDCom_lpc_LSFToLSP(ptrLsf_q, ptrLsp_q, M);
    }

    if (isAceStart) {
      LPDCom_lpc_LSFToLSP(lsf_q, lsp_q, M);
      copyFLOAT(lsp_q, CoderMemoryState->lspold_q, M);
    }

    copyFLOAT(&lsf[CoderMemoryState->nbDiv * M], CoderMemoryState->lsfold, M);
    copyFLOAT(&lsp[CoderMemoryState->nbDiv * M], CoderMemoryState->lspold, M);
  }

  return error;
}
