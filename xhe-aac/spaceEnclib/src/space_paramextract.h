
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

#ifndef INCLUDED_SPACE_PARAMEXTRACT_H
#define INCLUDED_SPACE_PARAMEXTRACT_H

#include "iisutillib.h"

#include "spaceEnclib_const.h"
#include "space_bitstream.h"
#include "space_tree.h"

#define MAX_TTT_MODES 2

typedef struct T_TTO_BOX *HANDLE_TTO_BOX;

typedef enum {

  BOX_SUBBANDS_INVALID = 0,
  BOX_SUBBANDS_4 = 4,
  BOX_SUBBANDS_5 = 5,
  BOX_SUBBANDS_7 = 7,
  BOX_SUBBANDS_9 = 9,
  BOX_SUBBANDS_10 = 10,
  BOX_SUBBANDS_12 = 12,
  BOX_SUBBANDS_14 = 14,
  BOX_SUBBANDS_15 = 15,
  BOX_SUBBANDS_20 = 20,
  BOX_SUBBANDS_23 = 23,
  BOX_SUBBANDS_28 = 28,
  BOX_SUBBANDS_MAX = BOX_SUBBANDS_28

} BOX_SUBBAND_CONFIG;

typedef enum {

  BOX_QUANTMODE_INVALID = -1,
  BOX_QUANTMODE_FINE = 0,
  BOX_QUANTMODE_EBQ1 = 1,
  BOX_QUANTMODE_EBQ2 = 2,
  BOX_QUANTMODE_RESERVED3 = 3,
  BOX_QUANTMODE_RESERVED4 = 4,
  BOX_QUANTMODE_RESERVED5 = 5,
  BOX_QUANTMODE_RESERVED6 = 6,
  BOX_QUANTMODE_RESERVED7 = 7

} BOX_QUANTMODE;

typedef struct T_TTO_BOX_CONFIG {
  int bCalcResiduals;
  int nResidualBands;
  int bCalcIccDiff;
  int bUseCoarseQuantCld;
  int bUseCoarseQuantIcc;

  int bUseCoarseQuantIpd;

  int bUseCoherenceIccOnly;
  int bOneIcc;
  int bCalcNoIcc;
  int nParametersMax;

  BOX_SUBBAND_CONFIG subbandConfig;
  BOX_QUANTMODE boxQuantMode;

  float epsilonFloat;
  int nTimeSlotsMax;
  int nHybridBandsMax;

  int bLowDelay;
  int bFrameKeep;

  int nBandsLfe;
  int bApplyLfeFilter;

  int bPhaseAlignLfe;

  DOWNMIXTYPE downmixType;
  IPDMODE ipdMode;
  int bDetectIpdRelevancy;
  int bInterpolateDownmix;

  int nHybBandsCore;
  int bStereoSbr;

  int nOttBandsPhase;
  int bUsac212;

} TTO_BOX_CONFIG;

typedef struct {
  float real;
  float imag;
} CPLX;

typedef struct {
  CPLX m[2][2];
  int infinity;
} TTO_MIX_MATRIX;

HANDLE_ERROR_INFO calculateCld(
    int const nParamBand,
    float const *const pPwr1,
    float const *const pPwr2,
    float *const pCld);

void QuantizeCoef(const float *input,
                  const int nBands,
                  const float *quantTable,
                  const int idxOffset,
                  const int nQuantSteps,
                  int *quantOut);

void deQuantizeCoef(const int *input,
                    const int nBands,
                    const float *quantTable,
                    const int idxOffset,
                    float *dequantOut);

HANDLE_ERROR_INFO
CreateTtoBox(HANDLE_TTO_BOX *hTtoBox, TTO_BOX_CONFIG *ttoBoxConfig);

HANDLE_ERROR_INFO
DestroyTtoBox(HANDLE_TTO_BOX *hTtoBox);

HANDLE_ERROR_INFO
ApplyTtoBox(HANDLE_TTO_BOX hTtoBox,
            int nTimeSlots,
            int nHybridBands,
            float **ppHybridDataReal1,
            float **ppHybridDataImag1,
            float **ppHybridDataReal2,
            float **ppHybridDataImag2,
            float **ppHybridDataRealResidual,
            float **ppHybridDataImagResidual,
            int *pIccIdx,
            int *pbIccQuantCoarse,
            int *pCldIdx,
            int *pbCldQuantCoarse,
            int *pIpdIdx,
            int *bsPhaseMode,
            int *numBinsIPD,
            int *pIccDiff,
            int *pbIccDiffPresent,
            int bUseBBCues,
            SPACETREE_MODE mode,
            int speechFlag);

HANDLE_ERROR_INFO
GetTtoBoxDownmixMatrix(HANDLE_TTO_BOX hTtoBox,
                       int nHybridBands,
                       TTO_MIX_MATRIX *downmix);

int getCldQuantOffset(int bUseCoarseQuant);

int getIccQuantOffset(int bUseCoarseQuant);

int getCpcQuantOffset(int bUseCoarseQuant);

int getNumberCldQuantLevels(int bUseCoarseQuant);

int getNumberIccQuantLevels(int bUseCoarseQuant);

int getNumberCpcQuantLevels(int bUseCoarseQuant);

float *
getCldQuant(HANDLE_TTO_BOX hTtoBox);

const int *
getSubband2ParameterIndex(BOX_SUBBAND_CONFIG subbandConfig, MPS_MODE mode);

const int *
getSubbandImagSign(BOX_SUBBAND_CONFIG subbandConfig, int bLowDelay);

float paramBand2Freq(BOX_SUBBAND_CONFIG boxSubbandConfig, int nSampleRate, int nParamBand, int nQmfBands);

int getNumHybBandsCore(const HANDLE_TTO_BOX hTtoBox);

int getBStereoSbr(const HANDLE_TTO_BOX hTtoBox);

float getUniSteCld(const HANDLE_TTO_BOX hTtoBox, int parameterBand);

#endif
