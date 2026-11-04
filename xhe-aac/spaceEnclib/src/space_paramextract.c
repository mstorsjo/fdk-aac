
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
#include <float.h>
#include <string.h>

#include "iisutillib.h"
#include "mathlib.h"

#include "spaceEnclib_const.h"
#include "space_paramextract.h"
#include "space_tree.h"

#if defined __GNUC__ || defined __clang__
#define ABSTEREOSPEECH __attribute__((unused))
#else
#define ABSTEREOSPEECH
#endif

#define HYBRID_RESOLUTION 141
#define HYBRID_RESOLUTION_USAC 77
#define MAX_NRG_COMP_RATIO 1.e-5f
#define LIMIT(x) (((x) > 1) ? 1 : ((x) < 0) ? 0 \
                                            : (x))
#define DB_TO_LINEAR(x) ((float)pow(10.0f, (x) / 20.0f))

#define COSF(x) ((float)cos(x))
#define ACOSF(x) ((float)acos(x))
#define SINF(x) ((float)sin(x))
#define TANF(x) ((float)tan(x))
#define ATANF(x) ((float)atan(x))
#define SQRTF(x) ((float)sqrt(x))
#define LOGF(x) ((float)log(x))
#define POWF(x, y) ((float)pow((x), (y)))

static HANDLE_ERROR_INFO paramextract_calculateUsacDmx(HANDLE_TTO_BOX hTtoBox,
                                                       float **ppHybridDataReal1,
                                                       float **ppHybridDataImag1,
                                                       float const *const *ppHybridDataReal2,
                                                       float const *const *ppHybridDataImag2,
                                                       int nParamBands,
                                                       int nTimeSlots,
                                                       int nHybridBands);

typedef struct T_TTO_BOX {
  float *pCld;
  float *pIcc;
  float *pIccDownmix;
  float *pIccCombined;
  int *pIccDownmixIdx;

  float *pIpd;

  float *pCldQuant;
  float *pIccQuant;
  float *pIccDownmixQuant;
  float *pIccCombinedQuant;

  float *pIpdQuant;

  float *pPwrHybridData1;
  float *pPwrHybridData2;
  float *pProdHybridDataReal;
  float *pProdHybridDataImag;

  float *pPwrHybrid1;
  float *pPwrHybrid2;
  float *pProdHybridReal;
  float *pProdHybridImag;

  CPLX *pCplxDmWeight1;
  CPLX *pCplxDmWeight2;
  CPLX *pCplxDmWeight3;
  CPLX *pCplxDmWeight4;

  float *pDmWeight1;
  float *pDmWeight2;
  float *pDmWeight3;
  float *pDmWeight4;
  float *pQuantizationFactor;
  int nParametersMax;

  const int *pSubband2ParameterIndex;

  int pSubband2ParameterAnalysisIndex[HYBRID_RESOLUTION_USAC];
  int bStereoSbr;
  int nHybBandsCore;
  int bUsac212;

  const int *pSubbandImagSign;
  int nHybridBandsMax;
  int nParameterBands;
  int bLowDelay;
  int bFrameKeep;

  int bOttModeLfe;
  int nParameterBandsLfe;
  int bApplyLfeFilter;

  int bPhaseAlignLfe;

  int iccCorrelationCoherenceBorder;
  BOX_QUANTMODE boxQuantMode;

  const float *pIccQuantTable;
  int nIccQuantSteps;
  int nIccQuantOffset;

  const float *pCldQuantTableDec;
  const float *pCldQuantTableEnc;
  int nCldQuantSteps;
  int nCldQuantOffset;

  int bCalcResiduals;
  int bCalcIccDiff;
  int nResidualBands;
  int bUseCoarseQuantCld;
  int bUseCoarseQuantIcc;

  int bUseCoarseQuantIpd;

  int bCalcNoIcc;
  int bOneIcc;

  int nOttBandsPhase;

  DOWNMIXTYPE downmixType;
  IPDMODE ipdMode;
  int bDetectIpdRelevancy;
  int bInterpolateDownmix;

  int prevResult[10];
  int frameCount;
  int prevIpdDecision;

  float epsilonFloat;
  int nTimeSlotsMax;

} TTO_BOX;

static const int subband2Parameter4[HYBRID_RESOLUTION] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    0, 1, 1, 1, 0, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3};

static const int subband2Parameter5[HYBRID_RESOLUTION] = {
    0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1,
    1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4};

static const int subband2Parameter7[HYBRID_RESOLUTION] = {
    0, 0, 0, 0, 1, 1, 0, 0, 2, 0, 1,
    1, 2, 2, 2, 1, 2, 3, 3, 3, 3, 4,
    4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6};

static const int subband2Parameter10[HYBRID_RESOLUTION] = {
    0, 0, 1, 1, 2, 2, 0, 0, 3, 1, 2,
    2, 3, 3, 4, 2, 4, 4, 5, 5, 6, 6,
    7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9};

static const int subband2Parameter14[HYBRID_RESOLUTION] = {
    0, 0, 1, 1, 2, 2, 0, 0, 4, 1, 2,
    3, 4, 4, 5, 3, 5, 6, 6, 7, 7, 8,
    8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11,
    11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13};

static const int subband2Parameter20[HYBRID_RESOLUTION] = {
    0, 1, 2, 3, 4, 4, 1, 0, 6, 3, 4,
    5, 6, 7, 8, 5, 8, 9, 10, 11, 12, 13,
    14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17,
    17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19};

static const int subband2Parameter28[HYBRID_RESOLUTION] = {
    0, 1, 2, 3, 4, 4, 1, 0, 6, 3, 4,
    5, 6, 7, 8, 5, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20,
    21, 21, 21, 22, 22, 22, 23, 23, 23, 23, 24,
    24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 26,
    26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27};

static const int subbandImagSign[HYBRID_RESOLUTION] = {
    1, 1, 1, 1, 1, 1, -1, -1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1};

static const int subband2Parameter4_res[HYBRID_RESOLUTION_USAC] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

static const int subband2Parameter5_res[HYBRID_RESOLUTION_USAC] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

static const int subband2Parameter7_res[HYBRID_RESOLUTION_USAC] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4,
    4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

static const int subband2Parameter10_res[HYBRID_RESOLUTION_USAC] = {
    0, 0, 1, 1, 1, 1, 0, 0, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

static const int subband2Parameter14_res[HYBRID_RESOLUTION_USAC] = {
    0, 0, 1, 1, 1, 1, 0, 0, 3, 2, 2,
    3, 4, 4, 4, 4, 5, 6, 6, 7, 7, 8,
    8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11,
    11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};

static const int subband2Parameter20_res[HYBRID_RESOLUTION_USAC] = {
    0, 1, 2, 3, 3, 2, 1, 0, 5, 4, 4,
    5, 6, 7, 7, 6, 8, 9, 10, 11, 12, 13,
    14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17,
    17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19};

static const int subband2Parameter28_res[HYBRID_RESOLUTION_USAC] = {
    0, 1, 2, 3, 3, 2, 1, 0, 5, 4, 4,
    5, 6, 7, 7, 6, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 17, 17, 18, 18, 19, 19, 20, 20,
    21, 21, 21, 22, 22, 22, 23, 23, 23, 23, 24,
    24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 26,
    26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27};

static const float parameterBandQmfWidth4[4] = {
    2.00f, 4.00f, 17.00f, 41.00f};

static const float parameterBandQmfWidth5[5] = {
    1.00f, 2.00f, 6.00f, 14.00f, 41.00f};

static const float parameterBandQmfWidth7[7] = {
    1.00f, 1.00f, 2.00f, 4.00f, 6.00f, 9.00f, 41.00f};

static const float parameterBandQmfWidth9[9] = {
    1.00f, 1.00f, 1.00f, 2.00f, 2.00f, 2.00f, 5.00f, 9.00f, 41.00f};

static const float parameterBandQmfWidth10[10] = {
    0.50f, 0.50f, 1.00f, 1.00f, 2.00f, 2.00f, 2.00f, 5.00f, 9.00f, 41.00f};

static const float parameterBandQmfWidth12[12] = {
    1.00f, 1.00f, 1.00f, 1.00f, 2.00f, 2.00f, 3.00f, 3.00f,
    4.00f, 5.00f, 12.00f, 29.00f};

static const float parameterBandQmfWidth14[14] = {
    0.50f, 0.50f, 0.50f, 0.50f, 1.00f, 1.00f, 2.00f, 2.00f, 3.00f, 3.00f,
    4.00f, 5.00f, 12.00f, 29.00f};

static const float parameterBandQmfWidth15[15] = {
    1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f, 2.00f, 3.00f, 4.00f, 5.00f, 12.00f, 29.00f};

static const float parameterBandQmfWidth20[20] = {
    0.25f, 0.25f, 0.25f, 0.25f, 0.50f, 0.50f, 0.50f, 0.50f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f, 2.00f, 3.00f, 4.00f, 5.00f, 12.00f, 29.00f};

static const float parameterBandQmfWidth23[23] = {
    1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 2.00f, 2.00f, 2.00f,
    2.00f, 3.00f, 3.00f, 4.00f, 5.00f, 6.00f, 7.00f, 16.00f};

static const float parameterBandQmfWidth28[28] = {
    0.25f, 0.25f, 0.25f, 0.25f, 0.50f, 0.50f, 0.50f, 0.50f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 2.00f, 2.00f, 2.00f,
    2.00f, 3.00f, 3.00f, 4.00f, 5.00f, 6.00f, 7.00f, 16.00f};

#define MAX_CLD_QUANT_FINE 31
static const int offsetCldQuantFine = 15;
const float cldQuantTableFineDec[MAX_CLD_QUANT_FINE] = {
    -150.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0,
    -16.0, -13.0, -10.0, -8.0, -6.0, -4.0, -2.0,
    0.0,
    2.0, 4.0, 6.0, 8.0, 10.0, 13.0, 16.0,
    19.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 150.0};

const float cldQuantTableFineEnc[MAX_CLD_QUANT_FINE] = {
    -50.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0,
    -16.0, -13.0, -10.0, -8.0, -6.0, -4.0, -2.0,
    0.0,
    2.0, 4.0, 6.0, 8.0, 10.0, 13.0, 16.0,
    19.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0};

#define MAX_CLD_QUANT_COARSE 15
static const int offsetCldQuantCoarse = 7;
const float cldQuantTableCoarseDec[MAX_CLD_QUANT_COARSE] = {
    -150.0, -35.0, -25.0, -19.0, -13.0, -8.0, -4.0,
    0.0,
    4.0, 8.0, 13.0, 19.0, 25.0, 35.0, 150.0};

const float cldQuantTableCoarseEnc[MAX_CLD_QUANT_COARSE] = {
    -50.0, -35.0, -25.0, -19.0, -13.0, -8.0, -4.0,
    0.0,
    4.0, 8.0, 13.0, 19.0, 25.0, 35.0, 50.0};

#define MAX_ICC_QUANT_FINE 8
static const int offsetIccQuantFine = 0;
static const float iccQuantTableFine[MAX_ICC_QUANT_FINE] = {
    1.0f, 0.937f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.589f, -0.99f};

#define MAX_ICC_QUANT_COARSE 4
static const int offsetIccQuantCoarse = 0;
static const float iccQuantTableCoarse[MAX_ICC_QUANT_COARSE] = {
    1.0f, 0.84118f, 0.36764f, -0.5890f};

#ifndef PI
#define PI 3.14159265358979324f
#endif

static int getNumberParameterBands(BOX_SUBBAND_CONFIG subbandConfig) {
  int nParameterBands = 0;

  switch (subbandConfig) {
    case BOX_SUBBANDS_4:
      nParameterBands = 4;
      break;
    case BOX_SUBBANDS_5:
      nParameterBands = 5;
      break;
    case BOX_SUBBANDS_7:
      nParameterBands = 7;
      break;
    case BOX_SUBBANDS_9:
      nParameterBands = 9;
      break;
    case BOX_SUBBANDS_10:
      nParameterBands = 10;
      break;
    case BOX_SUBBANDS_12:
      nParameterBands = 12;
      break;
    case BOX_SUBBANDS_14:
      nParameterBands = 14;
      break;
    case BOX_SUBBANDS_15:
      nParameterBands = 15;
      break;
    case BOX_SUBBANDS_20:
      nParameterBands = 20;
      break;
    case BOX_SUBBANDS_23:
      nParameterBands = 23;
      break;
    case BOX_SUBBANDS_28:
      nParameterBands = 28;
      break;
    default:
      break;
  }

  return nParameterBands;
}

static const float *getParameterBandQmfWidth(BOX_SUBBAND_CONFIG subbandConfig) {
  const float *parameterBandQmfWidth = NULL;

  switch (subbandConfig) {
    case BOX_SUBBANDS_4:
      parameterBandQmfWidth = parameterBandQmfWidth4;
      break;
    case BOX_SUBBANDS_5:
      parameterBandQmfWidth = parameterBandQmfWidth5;
      break;
    case BOX_SUBBANDS_7:
      parameterBandQmfWidth = parameterBandQmfWidth7;
      break;
    case BOX_SUBBANDS_9:
      parameterBandQmfWidth = parameterBandQmfWidth9;
      break;
    case BOX_SUBBANDS_10:
      parameterBandQmfWidth = parameterBandQmfWidth10;
      break;
    case BOX_SUBBANDS_12:
      parameterBandQmfWidth = parameterBandQmfWidth12;
      break;
    case BOX_SUBBANDS_14:
      parameterBandQmfWidth = parameterBandQmfWidth14;
      break;
    case BOX_SUBBANDS_15:
      parameterBandQmfWidth = parameterBandQmfWidth15;
      break;
    case BOX_SUBBANDS_20:
      parameterBandQmfWidth = parameterBandQmfWidth20;
      break;
    case BOX_SUBBANDS_23:
      parameterBandQmfWidth = parameterBandQmfWidth23;
      break;
    case BOX_SUBBANDS_28:
      parameterBandQmfWidth = parameterBandQmfWidth28;
      break;
    default:
      break;
  }

  return parameterBandQmfWidth;
}

const int *getSubband2ParameterIndex(BOX_SUBBAND_CONFIG subbandConfig, MPS_MODE mode) {
  const int *pSubband2ParameterIndex = NULL;

  switch (mode) {
    case MPS_MODE_DEFAULT:
    case MPS_MODE_USAC:

      switch (subbandConfig) {
        case BOX_SUBBANDS_4:
          pSubband2ParameterIndex = subband2Parameter4;
          break;

        case BOX_SUBBANDS_5:
          pSubband2ParameterIndex = subband2Parameter5;
          break;

        case BOX_SUBBANDS_7:
          pSubband2ParameterIndex = subband2Parameter7;
          break;

        case BOX_SUBBANDS_10:
          pSubband2ParameterIndex = subband2Parameter10;
          break;

        case BOX_SUBBANDS_14:
          pSubband2ParameterIndex = subband2Parameter14;
          break;

        case BOX_SUBBANDS_20:
          pSubband2ParameterIndex = subband2Parameter20;
          break;

        case BOX_SUBBANDS_28:
          pSubband2ParameterIndex = subband2Parameter28;
          break;

        default:
          break;
      }
      break;

    case MPS_MODE_USAC_UNISTE:

      switch (subbandConfig) {
        case BOX_SUBBANDS_4:
          pSubband2ParameterIndex = subband2Parameter4_res;
          break;

        case BOX_SUBBANDS_5:
          pSubband2ParameterIndex = subband2Parameter5_res;
          break;

        case BOX_SUBBANDS_7:
          pSubband2ParameterIndex = subband2Parameter7_res;
          break;

        case BOX_SUBBANDS_10:
          pSubband2ParameterIndex = subband2Parameter10_res;
          break;

        case BOX_SUBBANDS_14:
          pSubband2ParameterIndex = subband2Parameter14_res;
          break;

        case BOX_SUBBANDS_20:
          pSubband2ParameterIndex = subband2Parameter20_res;
          break;

        case BOX_SUBBANDS_28:
          pSubband2ParameterIndex = subband2Parameter28_res;
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }

  return pSubband2ParameterIndex;
}

const int *getSubbandImagSign(BOX_SUBBAND_CONFIG subbandConfig,
                              CLASSIC_MPS int bLowDelay) {
  const int *pSubbandImagSign = NULL;

  switch (subbandConfig) {
    case BOX_SUBBANDS_4:
    case BOX_SUBBANDS_5:
    case BOX_SUBBANDS_7:
    case BOX_SUBBANDS_9:
    case BOX_SUBBANDS_10:
    case BOX_SUBBANDS_12:
    case BOX_SUBBANDS_14:
    case BOX_SUBBANDS_15:
    case BOX_SUBBANDS_20:
    case BOX_SUBBANDS_23:
    case BOX_SUBBANDS_28:
      pSubbandImagSign = subbandImagSign;
      break;
    default:
      break;
  }

  return pSubbandImagSign;
}

static int getIccCorrelationCoherenceBorder(BOX_SUBBAND_CONFIG subbandConfig, int bUseCoherenceOnly) {
  int iccCorrelationCoherenceBorder = 0;

  if (bUseCoherenceOnly) {
    iccCorrelationCoherenceBorder = 0;
  } else {
    switch (subbandConfig) {
      case BOX_SUBBANDS_4:
        iccCorrelationCoherenceBorder = 1;
        break;
      case BOX_SUBBANDS_5:
        iccCorrelationCoherenceBorder = 2;
        break;
      case BOX_SUBBANDS_7:
        iccCorrelationCoherenceBorder = 3;
        break;
      case BOX_SUBBANDS_9:
        iccCorrelationCoherenceBorder = 4;
        break;
      case BOX_SUBBANDS_10:
        iccCorrelationCoherenceBorder = 5;
        break;
      case BOX_SUBBANDS_12:
        iccCorrelationCoherenceBorder = 4;
        break;
      case BOX_SUBBANDS_14:
        iccCorrelationCoherenceBorder = 6;
        break;
      case BOX_SUBBANDS_15:
        iccCorrelationCoherenceBorder = 5;
        break;
      case BOX_SUBBANDS_20:
        iccCorrelationCoherenceBorder = 10;
        break;
      case BOX_SUBBANDS_23:
        iccCorrelationCoherenceBorder = 8;
        break;
      case BOX_SUBBANDS_28:
        iccCorrelationCoherenceBorder = 13;
        break;
      default:
        break;
    }
  }

  return iccCorrelationCoherenceBorder;
}

static void alignParameterBandsWithSbrXover(int *pSubband2ParameterAnalysisIndex, const int nHybBandsCore, const int nParamBands) {
  int lastSubband = nHybBandsCore - 1;

  if ((pSubband2ParameterAnalysisIndex[lastSubband] != pSubband2ParameterAnalysisIndex[lastSubband - 1]) &&
      (pSubband2ParameterAnalysisIndex[lastSubband] == pSubband2ParameterAnalysisIndex[lastSubband + 1])) {
    pSubband2ParameterAnalysisIndex[lastSubband] = pSubband2ParameterAnalysisIndex[lastSubband - 1];
  }

  if (pSubband2ParameterAnalysisIndex[lastSubband] == nParamBands - 1) {
    return;
  }
  int band = nHybBandsCore;
  while (
      (band < HYBRID_RESOLUTION_USAC) &&
      (pSubband2ParameterAnalysisIndex[lastSubband] == pSubband2ParameterAnalysisIndex[band])) {
    pSubband2ParameterAnalysisIndex[band] = pSubband2ParameterAnalysisIndex[lastSubband] + 1;
    band++;
  }
}

int getNumHybBandsCore(const HANDLE_TTO_BOX hTtoBox) {
  return hTtoBox->nHybBandsCore;
}

int getBStereoSbr(const HANDLE_TTO_BOX hTtoBox) {
  return hTtoBox->bStereoSbr;
}

HANDLE_ERROR_INFO CreateTtoBox(
    HANDLE_TTO_BOX *hTtoBox,
    TTO_BOX_CONFIG *ttoBoxConfig) {
  HANDLE_ERROR_INFO error = noError;
  MPS_MODE mode = MPS_MODE_DEFAULT;
  int i = 0;
  const int numberOfFrames = 10;

  if (error == noError) {
    if (NULL == (*hTtoBox = (HANDLE_TTO_BOX)iisCalloc(1, sizeof(TTO_BOX)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for hTtoBox");
    }
  }

  if (error == noError) {
    if (ttoBoxConfig != NULL) {
      (*hTtoBox)->frameCount = 0;
      (*hTtoBox)->prevIpdDecision = 0;
      (*hTtoBox)->nParametersMax = ttoBoxConfig->nParametersMax;

      (*hTtoBox)->bCalcResiduals = ttoBoxConfig->bCalcResiduals;
      (*hTtoBox)->nResidualBands = ttoBoxConfig->nResidualBands;

      (*hTtoBox)->bCalcIccDiff = ttoBoxConfig->bCalcIccDiff;

      (*hTtoBox)->bCalcResiduals = ttoBoxConfig->bCalcResiduals;
      (*hTtoBox)->bUseCoarseQuantCld = ttoBoxConfig->bUseCoarseQuantCld;
      (*hTtoBox)->bUseCoarseQuantIcc = ttoBoxConfig->bUseCoarseQuantIcc;

      (*hTtoBox)->bUseCoarseQuantIpd = ttoBoxConfig->bUseCoarseQuantIpd;
      (*hTtoBox)->bUsac212 = ttoBoxConfig->bUsac212;

      (*hTtoBox)->bCalcNoIcc = ttoBoxConfig->bCalcNoIcc;
      (*hTtoBox)->bOneIcc = ttoBoxConfig->bOneIcc;
      (*hTtoBox)->boxQuantMode = ttoBoxConfig->boxQuantMode;
      (*hTtoBox)->iccCorrelationCoherenceBorder = getIccCorrelationCoherenceBorder(ttoBoxConfig->subbandConfig, ttoBoxConfig->bUseCoherenceIccOnly);
      (*hTtoBox)->nHybridBandsMax = ttoBoxConfig->nHybridBandsMax;
      (*hTtoBox)->nParameterBands = getNumberParameterBands(ttoBoxConfig->subbandConfig);
      (*hTtoBox)->bLowDelay = ttoBoxConfig->bLowDelay;
      (*hTtoBox)->bFrameKeep = ttoBoxConfig->bFrameKeep;

      if ((*hTtoBox)->bCalcResiduals && !(*hTtoBox)->bCalcIccDiff && !ttoBoxConfig->bUseCoherenceIccOnly) {
        if ((*hTtoBox)->iccCorrelationCoherenceBorder < (*hTtoBox)->nResidualBands) {
          (*hTtoBox)->iccCorrelationCoherenceBorder = (*hTtoBox)->nResidualBands;
        }
      }
      (*hTtoBox)->bStereoSbr = ttoBoxConfig->bStereoSbr;
      (*hTtoBox)->nHybBandsCore = ttoBoxConfig->nHybBandsCore;

      (*hTtoBox)->downmixType = ttoBoxConfig->downmixType;
      (*hTtoBox)->ipdMode = ttoBoxConfig->ipdMode;
      (*hTtoBox)->bDetectIpdRelevancy = ttoBoxConfig->bDetectIpdRelevancy;
      (*hTtoBox)->bInterpolateDownmix = ttoBoxConfig->bInterpolateDownmix;
      (*hTtoBox)->nOttBandsPhase = ttoBoxConfig->nOttBandsPhase;

      {
        (*hTtoBox)->bOttModeLfe = 0;
        (*hTtoBox)->nParameterBandsLfe = -1;
        (*hTtoBox)->bApplyLfeFilter = 0;
        (*hTtoBox)->bPhaseAlignLfe = 0;
      }

      (*hTtoBox)->pIccQuantTable = (*hTtoBox)->bUseCoarseQuantIcc ? iccQuantTableCoarse : iccQuantTableFine;
      (*hTtoBox)->nIccQuantSteps = getNumberIccQuantLevels((*hTtoBox)->bUseCoarseQuantIcc);
      (*hTtoBox)->nIccQuantOffset = getIccQuantOffset((*hTtoBox)->bUseCoarseQuantIcc);
      (*hTtoBox)->pCldQuantTableDec = (*hTtoBox)->bUseCoarseQuantCld ? cldQuantTableCoarseDec : cldQuantTableFineDec;
      (*hTtoBox)->pCldQuantTableEnc = (*hTtoBox)->bUseCoarseQuantCld ? cldQuantTableCoarseEnc : cldQuantTableFineEnc;
      (*hTtoBox)->nCldQuantSteps = getNumberCldQuantLevels((*hTtoBox)->bUseCoarseQuantCld);
      (*hTtoBox)->nCldQuantOffset = getCldQuantOffset((*hTtoBox)->bUseCoarseQuantCld);
      (*hTtoBox)->epsilonFloat = ttoBoxConfig->epsilonFloat;
      (*hTtoBox)->nTimeSlotsMax = ttoBoxConfig->nTimeSlotsMax;

      (*hTtoBox)->nParametersMax = max((*hTtoBox)->nParametersMax, (*hTtoBox)->nParameterBands);

      for (i = 0; i < numberOfFrames; ++i) {
        (*hTtoBox)->prevResult[i] = 0;
      }

      {
        if ((*hTtoBox)->nResidualBands > 0) {
          mode = MPS_MODE_USAC_UNISTE;
        } else {
          mode = MPS_MODE_USAC;
        }
      }

      if (NULL == ((*hTtoBox)->pSubband2ParameterIndex = getSubband2ParameterIndex(ttoBoxConfig->subbandConfig, mode))) {
        error = iisUtil_ERROR(CDI, "Invalid subband2ParameterIndex");
      }
      if (NULL == ((*hTtoBox)->pSubbandImagSign = getSubbandImagSign(ttoBoxConfig->subbandConfig, (*hTtoBox)->bLowDelay))) {
        error = iisUtil_ERROR(CDI, "Invalid subbandImagSign");
      }
    } else {
      error = iisUtil_ERROR(CDI, "ttoBoxConfig == NULL");
    }
  }

  if (error == noError) {
    if ((*hTtoBox)->bStereoSbr && (*hTtoBox)->bCalcResiduals) {
      memcpy((*hTtoBox)->pSubband2ParameterAnalysisIndex, (*hTtoBox)->pSubband2ParameterIndex, HYBRID_RESOLUTION_USAC * sizeof(int));

      alignParameterBandsWithSbrXover((*hTtoBox)->pSubband2ParameterAnalysisIndex, (*hTtoBox)->nHybBandsCore, (*hTtoBox)->nParameterBands);
    }
  }

  if (error == noError) {
    if (((*hTtoBox)->boxQuantMode != BOX_QUANTMODE_FINE) &&
        ((*hTtoBox)->boxQuantMode != BOX_QUANTMODE_EBQ1) &&
        ((*hTtoBox)->boxQuantMode != BOX_QUANTMODE_EBQ2)) {
      error = iisUtil_ERROR(CDI, "quantMode no supported.");
    }
  }

  if (error == noError) {
    (*hTtoBox)->pSubbandImagSign = subbandImagSign;
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pCld = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pCld");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIcc = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIcc");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccDownmix = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccDownmix");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccCombined = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccCombined");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccDownmixIdx = (int *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(int)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccDownmixIdx");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIpd = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIpd");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pCldQuant = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pCldQuant");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccQuant = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccQuant");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccDownmixQuant = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccDownmixQuant");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIccCombinedQuant = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIccCombinedQuant");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pIpdQuant = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pIpdQuant");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pPwrHybridData1 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pPwrHybridData1");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pPwrHybridData2 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pPwrHybridData2");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pProdHybridDataReal = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pProdHybridDataReal");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pProdHybridDataImag = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pProdHybridDataImag");
    }
  }

  if ((*hTtoBox)->downmixType == DOWNMIXTYPE_SIMPLIFIED || (*hTtoBox)->downmixType == DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL) {
    if (error == noError) {
      if (NULL == ((*hTtoBox)->pPwrHybrid1 = (float *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pPwrHybrid1");
      }
    }

    if (error == noError) {
      if (NULL == ((*hTtoBox)->pPwrHybrid2 = (float *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc (*hTtoBox)->pPwrHybrid2");
      }
    }

    if (error == noError) {
      if (NULL == ((*hTtoBox)->pProdHybridReal = (float *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pProdHybridReal");
      }
    }

    if (error == noError) {
      if (NULL == ((*hTtoBox)->pProdHybridImag = (float *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pProdHybridImag");
      }
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pCplxDmWeight1 = (CPLX *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(CPLX)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight1");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pCplxDmWeight2 = (CPLX *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(CPLX)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight2");
    }
  }

  if (error == noError) {
    if ((*hTtoBox)->bCalcResiduals) {
      if (NULL == ((*hTtoBox)->pCplxDmWeight3 = (CPLX *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(CPLX)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight3");
      }
    }
  }

  if (error == noError) {
    if ((*hTtoBox)->bCalcResiduals) {
      if (NULL == ((*hTtoBox)->pCplxDmWeight4 = (CPLX *)iisCalloc(1, (*hTtoBox)->nHybridBandsMax * sizeof(CPLX)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight4");
      }
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pDmWeight1 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight1");
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pDmWeight2 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight2");
    }
  }

  if (error == noError) {
    if ((*hTtoBox)->bCalcResiduals) {
      if (NULL == ((*hTtoBox)->pDmWeight3 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight3");
      }
    }
  }

  if (error == noError) {
    if ((*hTtoBox)->bCalcResiduals) {
      if (NULL == ((*hTtoBox)->pDmWeight4 = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pDmWeight4");
      }
    }
  }

  if (error == noError) {
    if (NULL == ((*hTtoBox)->pQuantizationFactor = (float *)iisCalloc(1, (*hTtoBox)->nParametersMax * sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for (*hTtoBox)->pQuantizationFactor");
    }
  }

  return error;
}

HANDLE_ERROR_INFO DestroyTtoBox(HANDLE_TTO_BOX *hTtoBox) {
  HANDLE_ERROR_INFO error = noError;

  if (*hTtoBox != NULL) {
    if (error == noError) {
      if ((*hTtoBox)->pCld) {
        iisFree((*hTtoBox)->pCld);
      }
      (*hTtoBox)->pCld = NULL;

      if ((*hTtoBox)->pIcc) {
        iisFree((*hTtoBox)->pIcc);
      }
      (*hTtoBox)->pIcc = NULL;

      if ((*hTtoBox)->pIccDownmix) {
        iisFree((*hTtoBox)->pIccDownmix);
      }
      (*hTtoBox)->pIccDownmix = NULL;

      if ((*hTtoBox)->pIccCombined) {
        iisFree((*hTtoBox)->pIccCombined);
      }
      (*hTtoBox)->pIccCombined = NULL;

      if ((*hTtoBox)->pIccDownmixIdx) {
        iisFree((*hTtoBox)->pIccDownmixIdx);
      }
      (*hTtoBox)->pIccDownmixIdx = NULL;

      if ((*hTtoBox)->pIpd) {
        iisFree((*hTtoBox)->pIpd);
      }
      (*hTtoBox)->pIpd = NULL;

      if ((*hTtoBox)->pCldQuant) {
        iisFree((*hTtoBox)->pCldQuant);
      }
      (*hTtoBox)->pCldQuant = NULL;

      if ((*hTtoBox)->pIccQuant) {
        iisFree((*hTtoBox)->pIccQuant);
      }
      (*hTtoBox)->pIccQuant = NULL;

      if ((*hTtoBox)->pIccDownmixQuant) {
        iisFree((*hTtoBox)->pIccDownmixQuant);
      }
      (*hTtoBox)->pIccDownmixQuant = NULL;

      if ((*hTtoBox)->pIccCombinedQuant) {
        iisFree((*hTtoBox)->pIccCombinedQuant);
      }
      (*hTtoBox)->pIccCombinedQuant = NULL;

      if ((*hTtoBox)->pIpdQuant) {
        iisFree((*hTtoBox)->pIpdQuant);
      }
      (*hTtoBox)->pIpdQuant = NULL;

      if ((*hTtoBox)->pPwrHybridData1) {
        iisFree((*hTtoBox)->pPwrHybridData1);
      }
      (*hTtoBox)->pPwrHybridData1 = NULL;

      if ((*hTtoBox)->pPwrHybridData2) {
        iisFree((*hTtoBox)->pPwrHybridData2);
      }
      (*hTtoBox)->pPwrHybridData2 = NULL;

      if ((*hTtoBox)->pProdHybridDataReal) {
        iisFree((*hTtoBox)->pProdHybridDataReal);
      }
      (*hTtoBox)->pProdHybridDataReal = NULL;

      if ((*hTtoBox)->pProdHybridDataImag) {
        iisFree((*hTtoBox)->pProdHybridDataImag);
      }
      (*hTtoBox)->pProdHybridDataImag = NULL;

      if ((*hTtoBox)->pPwrHybrid1) {
        iisFree((*hTtoBox)->pPwrHybrid1);
      }
      (*hTtoBox)->pPwrHybrid1 = NULL;

      if ((*hTtoBox)->pPwrHybrid2) {
        iisFree((*hTtoBox)->pPwrHybrid2);
      }
      (*hTtoBox)->pPwrHybrid2 = NULL;

      if ((*hTtoBox)->pProdHybridReal) {
        iisFree((*hTtoBox)->pProdHybridReal);
      }
      (*hTtoBox)->pProdHybridReal = NULL;

      if ((*hTtoBox)->pProdHybridImag) {
        iisFree((*hTtoBox)->pProdHybridImag);
      }
      (*hTtoBox)->pProdHybridImag = NULL;

      if ((*hTtoBox)->pCplxDmWeight1) {
        iisFree((*hTtoBox)->pCplxDmWeight1);
      }
      (*hTtoBox)->pCplxDmWeight1 = NULL;

      if ((*hTtoBox)->pCplxDmWeight2) {
        iisFree((*hTtoBox)->pCplxDmWeight2);
      }
      (*hTtoBox)->pCplxDmWeight2 = NULL;

      if ((*hTtoBox)->pCplxDmWeight3) {
        iisFree((*hTtoBox)->pCplxDmWeight3);
      }
      (*hTtoBox)->pCplxDmWeight3 = NULL;

      if ((*hTtoBox)->pCplxDmWeight4) {
        iisFree((*hTtoBox)->pCplxDmWeight4);
      }
      (*hTtoBox)->pCplxDmWeight4 = NULL;

      if ((*hTtoBox)->pDmWeight1) {
        iisFree((*hTtoBox)->pDmWeight1);
      }
      (*hTtoBox)->pDmWeight1 = NULL;

      if ((*hTtoBox)->pDmWeight2) {
        iisFree((*hTtoBox)->pDmWeight2);
      }
      (*hTtoBox)->pDmWeight2 = NULL;

      if ((*hTtoBox)->pDmWeight3) {
        iisFree((*hTtoBox)->pDmWeight3);
      }
      (*hTtoBox)->pDmWeight3 = NULL;

      if ((*hTtoBox)->pDmWeight4) {
        iisFree((*hTtoBox)->pDmWeight4);
      }
      (*hTtoBox)->pDmWeight4 = NULL;

      if ((*hTtoBox)->pQuantizationFactor) {
        iisFree((*hTtoBox)->pQuantizationFactor);
      }
      (*hTtoBox)->pQuantizationFactor = NULL;

      iisFree(*hTtoBox);
    }
  }
  *hTtoBox = NULL;

  return error;
}

static HANDLE_ERROR_INFO calculateIcc(
    int const nParamBand,
    int const correlationCoherenceBorder,
    float const *const pPwr1,
    float const *const pPwr2,
    float const *const pProdReal,
    float const *const pProdImag,
    float *const pIcc) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  if (error == noError) {
    if (correlationCoherenceBorder > nParamBand) {
      error = iisUtil_ERROR(CDI, "Invalid border");
    }
  }

  if (error == noError) {
    if (pIcc != NULL) {
      for (i = 0; i < correlationCoherenceBorder; i++) {
        pIcc[i] = min(pProdReal[i] / SQRTF(pPwr1[i] * pPwr2[i]), 1.f);
      }
      for (; i < nParamBand; i++) {
        pIcc[i] = min(SQRTF((pProdReal[i] * pProdReal[i] + pProdImag[i] * pProdImag[i]) / (pPwr1[i] * pPwr2[i])), 1.f);
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer to pIcc");
    }
  }

  if (error == noError) {
    limitFLOAT(-1.0f, +1.0f, pIcc, pIcc, nParamBand);
  }

  return error;
}

void QuantizeCoef(
    const float *input,
    const int nBands,
    const float *quantTable,
    const int idxOffset,
    const int nQuantSteps,
    int *quantOut) {
  int band;
  int reverse = (quantTable[0] > quantTable[1]);

  for (band = 0; band < nBands; band++) {
    float qVal;
    float curVal = input[band];

    int lower = 0;
    int upper = nQuantSteps - 1;

    if (reverse) {
      while (upper - lower > 1) {
        int idx = (lower + upper) / 2;
        qVal = quantTable[idx];
        if (curVal >= qVal) {
          upper = idx;
        } else {
          lower = idx;
        }
      }

      if (curVal - quantTable[lower] >= quantTable[upper] - curVal) {
        quantOut[band] = lower - idxOffset;
      } else {
        quantOut[band] = upper - idxOffset;
      }
    } else {
      while (upper - lower > 1) {
        int idx = (lower + upper) / 2;
        qVal = quantTable[idx];
        if (curVal <= qVal) {
          upper = idx;
        } else {
          lower = idx;
        }
      }

      if (curVal - quantTable[lower] <= quantTable[upper] - curVal) {
        quantOut[band] = lower - idxOffset;
      } else {
        quantOut[band] = upper - idxOffset;
      }
    }
  }
}

void deQuantizeCoef(
    const int *input,
    const int nBands,
    const float *quantTable,
    const int idxOffset,
    float *deQuantOut) {
  int band;

  for (band = 0; band < nBands; band++) {
    deQuantOut[band] = quantTable[input[band] + idxOffset];
  }

  return;
}

HANDLE_ERROR_INFO calculateCld(
    int const nParamBand,
    float const *const pPwr1,
    float const *const pPwr2,
    float *const pCld) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  if (error == noError) {
    if (pCld != NULL) {
      for (i = 0; i < nParamBand; i++) {
        pCld[i] = 10.0f * (float)log10(pPwr1[i] / pPwr2[i]);
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer to pCld");
    }
  }

  return error;
}

static HANDLE_ERROR_INFO calculateIpd(
    int const nParamBand,
    float const *const pProdReal,
    float const *const pProdImag,
    float *const pIpd) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  if (error == noError) {
    if (pIpd != NULL) {
      for (i = 0; i < nParamBand; i++) {
        pIpd[i] = (float)atan2(pProdImag[i], pProdReal[i]);
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer to pIpd");
    }
  }

  return error;
}

static int getIpdQuantSteps(const int quantMode) {
  int nQuantSteps = 0;

  switch (quantMode) {
    case 0:
      nQuantSteps = 16;
      break;
    case 1:
      nQuantSteps = 8;
      break;
    default:
      break;
  }

  return nQuantSteps;
}

static void quantizeIpd(const float *input,
                        const int nBands,
                        int nQuantSteps,
                        int *quantOut) {
  int paramBand;

  for (paramBand = 0; paramBand < nBands; paramBand++) {
    float ipd = (float)fmod(input[paramBand] + 2 * PI, 2 * PI);
    float index = ipd * nQuantSteps / (2 * PI);

    quantOut[paramBand] = (int)floor(index + 0.5f);

    if (quantOut[paramBand] == nQuantSteps) {
      quantOut[paramBand] = 0;
    }
  }
}

static void deQuantizeIpd(const int *input,
                          const int nBands,
                          const int nQuantSteps,
                          float *deQuantOut) {
  int paramBand;

  for (paramBand = 0; paramBand < nBands; paramBand++) {
    deQuantOut[paramBand] = input[paramBand] * 2 * PI / nQuantSteps;
  }
}

static void avoidPredictionSingularity(HANDLE_TTO_BOX hTtoBox,
                                       int *pCldIdx,
                                       int *pIccIdx,
                                       int *pIpdIdx) {
  int nPhaseBands = hTtoBox->nOttBandsPhase;
  const float *pCld = hTtoBox->pCld;
  int antiPhase = getIpdQuantSteps(hTtoBox->bUseCoarseQuantIpd) / 2;
  int requantize = 0;
  int requantize_val = 1;
  int paramBand;

  for (paramBand = 0; paramBand < nPhaseBands; paramBand++) {
    if ((pIccIdx[paramBand] == 0) && (pIpdIdx[paramBand] == antiPhase)) {
      if (pCldIdx[paramBand] == 0) {
        pCldIdx[paramBand] = (pCld[paramBand] >= 0.0f) ? 1 : -1;
        requantize_val = 1;
      } else {
        requantize_val = 0;
      }
      requantize |= requantize_val;
    }
  }

  if (requantize) {
    deQuantizeCoef(pCldIdx, nPhaseBands, hTtoBox->pCldQuantTableDec, hTtoBox->nCldQuantOffset, hTtoBox->pCldQuant);
  }
}

static int ipdDetector(HANDLE_TTO_BOX hTtoBox, float *pPwr1, float *pPwr2, int numIpdBands) {
  float *cld = hTtoBox->pCld;
  float *icc = hTtoBox->pIcc;
  float *ipd = hTtoBox->pIpd;

  const float cldRelev0 = 30;
  const float cldRelev1 = 15;

  const float iccRelev0 = 0.1f;
  const float iccRelev1 = 1;

  const float ipdRelev0 = 0;
  const float ipdRelev1 = PI / 3;

  const float nrgRelev0 = 0;
  const float nrgRelev1 = 1.f / numIpdBands;

  const float thrAvgHigh = 0.6f;
  const float thrAvgLow = 0.3f;
  const float thrMaxHigh = 0.85f;
  const float thrMaxLow = 0.75f;

  float nrgSum, normNrg[28];
  float rAvg, rMax;
  int result, i;

  nrgSum = 1e-6f;
  for (i = 0; i < numIpdBands; i++) nrgSum += (pPwr1[i] + pPwr2[i]);
  for (i = 0; i < numIpdBands; i++) normNrg[i] = (pPwr1[i] + pPwr2[i]) / nrgSum;

  rAvg = rMax = 0;
  for (i = 0; i < numIpdBands; i++) {
    float rCld, rIcc, rIpd, rNrg, tmp;

    rCld = LIMIT(((float)fabs(cld[i]) - cldRelev0) / (cldRelev1 - cldRelev0));

    rIcc = LIMIT((icc[i] - iccRelev0) / (iccRelev1 - iccRelev0));

    rIpd = LIMIT(((float)fabs(ipd[i]) - ipdRelev0) / (ipdRelev1 - ipdRelev0));
    rIpd = SQRTF(rIpd);

    rIpd = (rIpd + .1f) / 1.1f;

    rNrg = LIMIT((normNrg[i] - nrgRelev0) / (nrgRelev1 - nrgRelev0));

    rAvg += rCld * rIcc * rIpd * normNrg[i];

    tmp = rCld * rIcc * rIpd * rNrg;
    if (tmp > rMax) rMax = tmp;
  }
  result = hTtoBox->prevIpdDecision;

  if ((rAvg > thrAvgHigh) || (rMax > thrMaxHigh)) {
    result = 1;
  } else if ((rAvg < thrAvgLow) && (rMax < thrMaxLow)) {
    result = 0;
  }

  hTtoBox->prevIpdDecision = result;
  return result;
}

static int applyPhaseCorrections(HANDLE_TTO_BOX hTtoBox,
                                 int *pCldIdx,
                                 int *pIccIdx,
                                 int *pIpdIdx,
                                 int bUseBBCues) {
  int nPhaseBands = hTtoBox->nOttBandsPhase;
  int quantMode = hTtoBox->bUseCoarseQuantIpd;
  float *powerHybridData1 = hTtoBox->pPwrHybridData1;
  float *powerHybridData2 = hTtoBox->pPwrHybridData2;
  int dominantPhase = 1;
  int phaseMode = 0;

  if (hTtoBox->ipdMode == IPDMODE_RESIDUAL) {
    avoidPredictionSingularity(hTtoBox, pCldIdx, pIccIdx, pIpdIdx);
  }

  if (hTtoBox->bDetectIpdRelevancy) {
    dominantPhase = ipdDetector(hTtoBox, powerHybridData1, powerHybridData2, nPhaseBands);
  }

  if ((dominantPhase) && (nPhaseBands > 0)) {
    if (quantMode == 0) {
      phaseMode = 3;
    } else if (quantMode == 1) {
      phaseMode = 2;
    }
  } else {
    float *coherence;
    float *correlation;
    float *ipd;
    int paramBand;
    int nParamBands = hTtoBox->nParameterBands;
    int nQuantSteps = getIpdQuantSteps(quantMode);

    if (hTtoBox->bCalcResiduals) {
      coherence = hTtoBox->pIccQuant;
      correlation = hTtoBox->pIcc;
      ipd = hTtoBox->pIpdQuant;
    } else {
      coherence = hTtoBox->pIcc;
      correlation = hTtoBox->pIcc;
      ipd = hTtoBox->pIpd;
    }

    for (paramBand = 0; paramBand < nPhaseBands; paramBand++) {
      correlation[paramBand] = coherence[paramBand] * COSF(ipd[paramBand]);
      pIpdIdx[paramBand] = 0;
    }

    if (bUseBBCues) {
      int i;
      for (i = 1; i < nParamBands; i++) {
        hTtoBox->pIcc[0] += hTtoBox->pIcc[i];
      }
      hTtoBox->pIcc[0] /= (float)nParamBands;
      for (i = 1; i < nParamBands; i++) {
        hTtoBox->pIcc[i] = hTtoBox->pIcc[0];
      }

      QuantizeCoef(hTtoBox->pIcc, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->nIccQuantSteps, pIccIdx);
      deQuantizeCoef(pIccIdx, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->pIccQuant);
      deQuantizeIpd(pIpdIdx, nParamBands, nQuantSteps, hTtoBox->pIpdQuant);
    } else {
      QuantizeCoef(hTtoBox->pIcc, nPhaseBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->nIccQuantSteps, pIccIdx);
      deQuantizeCoef(pIccIdx, nPhaseBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->pIccQuant);
      deQuantizeIpd(pIpdIdx, nPhaseBands, nQuantSteps, hTtoBox->pIpdQuant);
    }
  }

  return phaseMode;
}

static void getDownmixWeightsPar(int startBand,
                                 int stopBand,
                                 float *cld,
                                 float *icc,
                                 float maxWeight,
                                 CPLX *cplxWeight1,
                                 CPLX *cplxWeight2) {
  int i;

  for (i = startBand; i < stopBand; i++) {
    float cldLin = DB_TO_LINEAR(cld[i]);
    float cldLin21 = cldLin * cldLin + 1.0f;

    cplxWeight1[i].real = SQRTF(cldLin21 / (cldLin21 + 2.0f * icc[i] * cldLin));
    cplxWeight1[i].real = min(maxWeight, cplxWeight1[i].real);
    cplxWeight1[i].imag = 0.0f;
    cplxWeight2[i].real = cplxWeight1[i].real;
    cplxWeight2[i].imag = cplxWeight1[i].imag;
  }
}

static void getCoherentDownmixWeightsHyb(HANDLE_TTO_BOX hTtoBox,
                                         int nHybridBands,
                                         int startBand,
                                         int stopBand,
                                         CPLX *cplxWeight1,
                                         CPLX *cplxWeight2,
                                         CPLX *cplxWeight3,
                                         CPLX *cplxWeight4) {
  float *powerHybrid1 = hTtoBox->pPwrHybrid1;
  float *powerHybrid2 = hTtoBox->pPwrHybrid2;
  float *prodHybridReal = hTtoBox->pProdHybridReal;
  float *prodHybridImag = hTtoBox->pProdHybridImag;
  float powerSum, prodAbs, ratio, gain1, gain2, powerDmx;
  int i, paramBand;

  for (i = 0; i < nHybridBands; i++) {
    paramBand = hTtoBox->pSubband2ParameterIndex[i];
    if ((paramBand < startBand) || (paramBand >= stopBand)) {
      continue;
    }

    powerSum = powerHybrid1[i] + powerHybrid2[i];
    prodAbs = prodHybridReal[i] * prodHybridReal[i] +
              prodHybridImag[i] * prodHybridImag[i];
    ratio = (powerSum + 2.0f * prodHybridReal[i]) /
            (powerSum + 2.0f * SQRTF(prodAbs));

    gain2 = POWF(max(ratio, 0.0f), 0.25f);
    gain1 = 2.0f - gain2;

    powerDmx = gain1 * gain1 * powerHybrid1[i];
    powerDmx += gain2 * gain2 * powerHybrid2[i];
    powerDmx += 2.0f * gain1 * gain2 * prodHybridReal[i];

    powerSum = max(powerSum, FLOAT_EPSILON);
    powerDmx = max(powerDmx, MAX_NRG_COMP_RATIO * powerSum);
    double power_rms = sqrt(powerSum / powerDmx);

    cplxWeight1[i].real = (float)(power_rms)*gain1;
    cplxWeight2[i].real = (float)(power_rms)*gain2;
    cplxWeight1[i].imag = 0.0f;
    cplxWeight2[i].imag = 0.0f;

    if (cplxWeight3 != NULL) {
      cplxWeight3[i].real = (float)(1.0f / power_rms);
    }
    if (cplxWeight4 != NULL) {
      cplxWeight4[i].real = (float)(1.0f / power_rms);
    }
  }
}

static void getResidualWeightsPar(int startBand,
                                  int stopBand,
                                  float *cld,
                                  float *icc,
                                  float *ipd,
                                  CPLX *cplxWeight1,
                                  CPLX *cplxWeight2,
                                  CPLX *cplxWeight3,
                                  CPLX *cplxWeight4) {
  int i;

  for (i = startBand; i < stopBand; i++) {
    float tmp1, tmp2;
    float cldLin2;
    float alphaR, alphaI;
    float cldLin = DB_TO_LINEAR(cld[i]);
    cldLin2 = cldLin * cldLin;
    tmp1 = 2.0f * max(-0.99f, icc[i]) * cldLin;
    tmp2 = cldLin2 + 1.0f + tmp1 * COSF(ipd[i]);
    alphaR = (1.0f - cldLin2) / tmp2;
    alphaI = -tmp1 * SINF(ipd[i]) / tmp2;

    cplxWeight3[i].real = cplxWeight2[i].real + alphaR * cplxWeight1[i].real;
    cplxWeight3[i].imag = alphaI * cplxWeight1[i].real;
    cplxWeight4[i].real = -cplxWeight1[i].real + alphaR * cplxWeight2[i].real;
    cplxWeight4[i].imag = alphaI * cplxWeight2[i].real;
  }
}

static void getResidualWeightsHyb(HANDLE_TTO_BOX hTtoBox,
                                  int nHybridBands,
                                  int startBand,
                                  int stopBand,
                                  float *cld,
                                  float *icc,
                                  CPLX *cplxWeight1,
                                  CPLX *cplxWeight2,
                                  CPLX *cplxWeight3,
                                  CPLX *cplxWeight4)

{
  int i;
  int paramBand;

  for (i = 0; i < nHybridBands; i++) {
    paramBand = hTtoBox->pSubband2ParameterIndex[i];
    if ((paramBand < startBand) || (paramBand >= stopBand)) {
      continue;
    }

    float resGain = 1.0f;
    float cldLin = DB_TO_LINEAR(cld[paramBand]);
    float max_inv_det = 1.2f;
    float cldLin2 = (float)(cldLin * cldLin);
    float c_l = SQRTF((cldLin2) / (1 + (cldLin2)));
    float c_r = SQRTF(1 / (1 + (cldLin2)));
    float iccCorrLim = max(icc[paramBand], (1 / (max_inv_det * max_inv_det) - 1.0f) * (cldLin + 1.0f / cldLin) / 2.0f);
    float alpha = (float)(0.5f * acos(iccCorrLim));
    float beta = ATANF(tan(alpha) * (c_r - c_l) / (c_r + c_l));

    float H11 = (float)(c_l * cos(alpha + beta));
    float H21 = (float)(c_r * cos(-alpha + beta));
    float H12_res = max(0.5f, H11);
    float H22_res = -max(0.5f, H21);
    float w1 = cplxWeight1[i].real;
    float w2 = cplxWeight2[i].real;

    cplxWeight3[i].real *= resGain * (((1 - H11 * w1) / H12_res) - (H21 * w1 / H22_res)) / 2.0f;
    cplxWeight3[i].imag = 0;
    cplxWeight4[i].real *= resGain * (((1 - H21 * w2) / H22_res) - (H11 * w2 / H12_res)) / 2.0f;
    cplxWeight4[i].imag = 0;
  }
}

static void getClassicWeights(HANDLE_TTO_BOX hTtoBox,
                              int resBand,
                              int stopBand,
                              float *cld,
                              float *icc,
                              CPLX *cplxWeight1,
                              CPLX *cplxWeight2,
                              CPLX *cplxWeight3,
                              CPLX *cplxWeight4) {
  int paramBand;
  const float maxWeight = 1.2f;
  const float iccMin = 0.5f * (1.0f / (maxWeight * maxWeight) - 1.0f);
  float alpha, beta;
  float tmp1, tmp2, tmp3;
  float cldLin1;
  float iccLim;
  float *cldRes = hTtoBox->pCldQuant;
  float *iccRes = hTtoBox->pIccDownmixQuant;
  ;
  for (paramBand = 0; paramBand < stopBand; paramBand++) {
    if (paramBand == resBand) {
      cldRes = cld;
      iccRes = icc;
    }
    float cldLin = DB_TO_LINEAR(cldRes[paramBand]);
    cldLin1 = cldLin + 1.0f / cldLin;
    tmp1 = iccMin * cldLin1;
    iccLim = max(tmp1, iccRes[paramBand]);
    tmp1 = SQRTF(cldLin / cldLin1);
    tmp2 = 1.0f / SQRTF(cldLin * cldLin1);
    tmp3 = 1.0f / (cplxWeight3[paramBand].real - cplxWeight4[paramBand].real);

    alpha = 0.5f * ACOSF(iccLim);
    beta = ATANF((TANF(alpha)) * (tmp2 - tmp1) / (tmp2 + tmp1));

    cplxWeight1[paramBand].real = max(0.0f, tmp3);
    cplxWeight2[paramBand].real = cplxWeight1[paramBand].real;
    cplxWeight3[paramBand].real = (tmp2 * COSF(beta - alpha)) * cplxWeight1[paramBand].real;
    cplxWeight4[paramBand].real = (-tmp1 * COSF(beta + alpha)) * cplxWeight1[paramBand].real;

    cplxWeight3[paramBand].real *= 2.0f;
    cplxWeight3[paramBand].imag *= 2.0f;
    cplxWeight4[paramBand].real *= 2.0f;
    cplxWeight4[paramBand].imag *= 2.0f;
  }
}

static HANDLE_ERROR_INFO calculateTtoDownmixParamUsac(HANDLE_TTO_BOX hTtoBox,
                                                      int nHybridBands) {
  HANDLE_ERROR_INFO error = noError;

  int nParamBands = hTtoBox->nParameterBands;
  int bCalcResiduals = hTtoBox->bCalcResiduals;
  int nResidualBands = hTtoBox->nResidualBands;

  float *cld = hTtoBox->pCld;
  float *icc = hTtoBox->pIccDownmix;
  float *ipd = hTtoBox->pIpd;

  float *cldQuant = hTtoBox->pCldQuant;
  float *iccQuant = hTtoBox->pIccDownmixQuant;
  float *ipdQuant = hTtoBox->pIpdQuant;

  CPLX *cplxWeight1 = hTtoBox->pCplxDmWeight1;
  CPLX *cplxWeight2 = hTtoBox->pCplxDmWeight2;
  CPLX *cplxWeight3 = hTtoBox->pCplxDmWeight3;
  CPLX *cplxWeight4 = hTtoBox->pCplxDmWeight4;

  int i;
  float maxWeight = 1.2f;

  if (NULL == icc || NULL == cld || NULL == cplxWeight1 || NULL == cplxWeight2) {
    error = iisUtil_ERROR(CDI, "Parameter error.");
  }

  if (bCalcResiduals && (nResidualBands > nParamBands || NULL == iccQuant || NULL == cplxWeight3 || NULL == cplxWeight4)) {
    error = iisUtil_ERROR(CDI, "Parameter error.");
  }

  if (noError == error) {
    if (bCalcResiduals) {
      float *cldRes = cldQuant;
      float *iccRes = iccQuant;
      float *ipdRes = ipdQuant;
      float *coherence = NULL;
      float correlation[MAX_NUM_BINS];

      if (hTtoBox->ipdMode != IPDMODE_NONE) {
        for (i = 0; i < nResidualBands; i++) {
          correlation[i] = iccRes[i] * COSF(ipdRes[i]);
        }
        coherence = iccRes;
        iccRes = correlation;
      }

      if (hTtoBox->ipdMode == IPDMODE_RESIDUAL) {
        getDownmixWeightsPar(0, nResidualBands, cldRes, iccRes, maxWeight, cplxWeight1, cplxWeight2);
        getResidualWeightsPar(0, nResidualBands, cldRes, coherence, ipdRes, cplxWeight1, cplxWeight2, cplxWeight3, cplxWeight4);

        switch (hTtoBox->downmixType) {
          case DOWNMIXTYPE_CLASSICMPS:
            getDownmixWeightsPar(nResidualBands, nParamBands, cld, icc, maxWeight, cplxWeight1, cplxWeight2);
            getResidualWeightsPar(nResidualBands, nParamBands, cld, icc, ipd, cplxWeight1, cplxWeight2, cplxWeight3, cplxWeight4);
            break;

          case DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL:
            getCoherentDownmixWeightsHyb(hTtoBox, nHybridBands, nResidualBands, nParamBands, cplxWeight1, cplxWeight2, cplxWeight3, cplxWeight4);
            getResidualWeightsHyb(hTtoBox, nHybridBands, nResidualBands, nParamBands, cld, icc, cplxWeight1, cplxWeight2, cplxWeight3, cplxWeight4);
            break;
          default:
            error = iisUtil_ERROR(CDI, "Invalid downmixType");
            break;
        }
      } else {
        getClassicWeights(hTtoBox, nResidualBands, nParamBands, cld, icc, cplxWeight1, cplxWeight2, cplxWeight3, cplxWeight4);
      }
    } else {
      if (hTtoBox->ipdMode != IPDMODE_RESIDUAL) {
        maxWeight = 2.0f;
      }

      switch (hTtoBox->downmixType) {
        case DOWNMIXTYPE_CLASSICMPS:
          getDownmixWeightsPar(0, nParamBands, cld, icc, maxWeight, cplxWeight1, cplxWeight2);
          break;
        case DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL:
        case DOWNMIXTYPE_SIMPLIFIED:
          getCoherentDownmixWeightsHyb(hTtoBox, nHybridBands, 0, nParamBands, cplxWeight1, cplxWeight2, NULL, NULL);
          break;

        default:
          break;
      }
    }

    if ((hTtoBox->downmixType != DOWNMIXTYPE_SIMPLIFIED) && hTtoBox->bInterpolateDownmix) {
      for (i = nHybridBands - 1; i >= 0; i--) {
        int paramBand = hTtoBox->pSubband2ParameterIndex[i];

        if (hTtoBox->downmixType != DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL || nResidualBands > paramBand) {
          cplxWeight1[i] = cplxWeight1[paramBand];
          cplxWeight2[i] = cplxWeight2[paramBand];
          if (bCalcResiduals) {
            cplxWeight3[i] = cplxWeight3[paramBand];
            cplxWeight4[i] = cplxWeight4[paramBand];
          }
        }
      }
    }
  }

  return error;
}

HANDLE_ERROR_INFO ApplyTtoBox(
    HANDLE_TTO_BOX hTtoBox,
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
    CLASSIC_MPS SPACETREE_MODE mode,
    ABSTEREOSPEECH int speechFlag)

{
  HANDLE_ERROR_INFO error = noError;
  int i;
  int j;

  if (hTtoBox != NULL) {
    int nParamBands = hTtoBox->nParameterBands;
    int nResidualBands = hTtoBox->nResidualBands;

    float *powerHybridData1 = hTtoBox->pPwrHybridData1;
    float *powerHybridData2 = hTtoBox->pPwrHybridData2;
    float *prodHybridDataReal = hTtoBox->pProdHybridDataReal;
    float *prodHybridDataImag = hTtoBox->pProdHybridDataImag;

    float *powerHybrid1 = hTtoBox->pPwrHybrid1;
    float *powerHybrid2 = hTtoBox->pPwrHybrid2;
    float *prodHybridReal = hTtoBox->pProdHybridReal;
    float *prodHybridImag = hTtoBox->pProdHybridImag;

    if (error == noError) {
      if ((nHybridBands < 0) || (nHybridBands > hTtoBox->nHybridBandsMax)) {
        error = iisUtil_ERROR(CDI, "Invalid number nHybridBands.");
      }
      if (nTimeSlots > hTtoBox->nTimeSlotsMax) {
        error = iisUtil_ERROR(CDI, "Invalid nTimeSlots.");
      }
    }

    if (error == noError) {
      setFLOAT(hTtoBox->epsilonFloat, powerHybridData1, hTtoBox->nParameterBands);
      setFLOAT(hTtoBox->epsilonFloat, powerHybridData2, hTtoBox->nParameterBands);
      setFLOAT(hTtoBox->epsilonFloat, prodHybridDataReal, hTtoBox->nParameterBands);
      setFLOAT(0.f, prodHybridDataImag, hTtoBox->nParameterBands);

      if (hTtoBox->downmixType == DOWNMIXTYPE_SIMPLIFIED || hTtoBox->downmixType == DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL) {
        setFLOAT(hTtoBox->epsilonFloat, powerHybrid1, nHybridBands);
        setFLOAT(hTtoBox->epsilonFloat, powerHybrid2, nHybridBands);
        setFLOAT(hTtoBox->epsilonFloat, prodHybridReal, nHybridBands);
        setFLOAT(0.f, prodHybridImag, nHybridBands);
      }

      if ((ppHybridDataReal1 != NULL) &&
          (ppHybridDataImag1 != NULL) &&
          (ppHybridDataReal2 != NULL) &&
          (ppHybridDataImag2 != NULL)) {
        for (j = 0; j < nHybridBands; j++) {
          int paramIndex = hTtoBox->pSubband2ParameterIndex[j];
          float imagSign = (float)hTtoBox->pSubbandImagSign[j];

          for (i = 0; i < nTimeSlots; i++) {
            if ((ppHybridDataReal1[i] != NULL) &&
                (ppHybridDataImag1[i] != NULL) &&
                (ppHybridDataReal2[i] != NULL) &&
                (ppHybridDataImag2[i] != NULL)) {
              if (hTtoBox->bStereoSbr && hTtoBox->bCalcResiduals) {
                paramIndex = hTtoBox->pSubband2ParameterAnalysisIndex[j];
              }

              if (hTtoBox->downmixType == DOWNMIXTYPE_SIMPLIFIED || hTtoBox->downmixType == DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL) {
                float power1, power2, power12real, power12imag;

                power1 = ppHybridDataReal1[i][j] * ppHybridDataReal1[i][j] +
                         ppHybridDataImag1[i][j] * ppHybridDataImag1[i][j];
                power2 = ppHybridDataReal2[i][j] * ppHybridDataReal2[i][j] +
                         ppHybridDataImag2[i][j] * ppHybridDataImag2[i][j];
                power12real = ppHybridDataReal1[i][j] * ppHybridDataReal2[i][j] +
                              ppHybridDataImag1[i][j] * ppHybridDataImag2[i][j];
                power12imag = imagSign * ppHybridDataImag1[i][j] * ppHybridDataReal2[i][j] -
                              ppHybridDataReal1[i][j] * imagSign * ppHybridDataImag2[i][j];

                powerHybrid1[j] += power1;
                powerHybrid2[j] += power2;
                prodHybridReal[j] += power12real;
                prodHybridImag[j] += power12imag;

                powerHybridData1[paramIndex] += power1;
                powerHybridData2[paramIndex] += power2;
                prodHybridDataReal[paramIndex] += power12real;
                prodHybridDataImag[paramIndex] += power12imag;
              } else {
                powerHybridData1[paramIndex] += ppHybridDataReal1[i][j] * ppHybridDataReal1[i][j] +
                                                ppHybridDataImag1[i][j] * ppHybridDataImag1[i][j];
                powerHybridData2[paramIndex] += ppHybridDataReal2[i][j] * ppHybridDataReal2[i][j] +
                                                ppHybridDataImag2[i][j] * ppHybridDataImag2[i][j];
                prodHybridDataReal[paramIndex] += ppHybridDataReal1[i][j] * ppHybridDataReal2[i][j] +
                                                  ppHybridDataImag1[i][j] * ppHybridDataImag2[i][j];
                prodHybridDataImag[paramIndex] += imagSign * ppHybridDataImag1[i][j] * ppHybridDataReal2[i][j] -
                                                  ppHybridDataReal1[i][j] * imagSign * ppHybridDataImag2[i][j];
              }
            } else {
              error = iisUtil_ERROR(CDI, "Invalid pointer");
              break;
            }
          }
        }
      } else {
        error = iisUtil_ERROR(CDI, "Invalid pointer");
      }
    }

    SAFECALL(error, calculateIcc(nParamBands, nParamBands, powerHybridData1, powerHybridData2, prodHybridDataReal, prodHybridDataImag, hTtoBox->pIccDownmix));

    if (!hTtoBox->bCalcNoIcc) {
      SAFECALL(error,
               calculateIcc(nParamBands, hTtoBox->iccCorrelationCoherenceBorder, powerHybridData1, powerHybridData2, prodHybridDataReal, prodHybridDataImag, hTtoBox->pIcc));

      if ((hTtoBox->ipdMode != IPDMODE_NONE) && hTtoBox->bCalcResiduals) {
        int paramBand;

        for (paramBand = 0; paramBand < nResidualBands; paramBand++) {
          hTtoBox->pIccDownmix[paramBand] = hTtoBox->pIcc[paramBand];
        }
      }
    }

    {
      SAFECALL(error, calculateCld(nParamBands, powerHybridData1, powerHybridData2, hTtoBox->pCld));
    }

    if (hTtoBox->ipdMode != IPDMODE_NONE) {
      SAFECALL(error, calculateIpd(hTtoBox->nOttBandsPhase, prodHybridDataReal, prodHybridDataImag, hTtoBox->pIpd));
    }

    if (error == noError) {
      if (hTtoBox->bStereoSbr && hTtoBox->bCalcResiduals) {
        int paramBand;
        for (paramBand = 0; paramBand < nHybridBands; paramBand++) {
          int paramIndex = hTtoBox->pSubband2ParameterIndex[paramBand];
          int paramAnalysisIdx = hTtoBox->pSubband2ParameterAnalysisIndex[paramBand];

          if (paramAnalysisIdx < paramIndex) {
            hTtoBox->pIcc[paramIndex] = hTtoBox->pIcc[paramAnalysisIdx];
            hTtoBox->pIccDownmix[paramIndex] = hTtoBox->pIccDownmix[paramAnalysisIdx];
            hTtoBox->pCld[paramIndex] = hTtoBox->pCld[paramAnalysisIdx];
            hTtoBox->pIpd[paramIndex] = hTtoBox->pIpd[paramAnalysisIdx];
          }
        }

        int nParamBandsCore;
        nParamBandsCore = hTtoBox->pSubband2ParameterIndex[hTtoBox->nHybBandsCore] + 1;
        float iccLastCore = hTtoBox->pIcc[nParamBandsCore - 1];
        float cldLastCore = hTtoBox->pCld[nParamBandsCore - 1];
        float ipdLastCore = hTtoBox->pIpd[nParamBandsCore - 1];
        for (i = nParamBandsCore; i < nParamBands; i++) {
          hTtoBox->pIcc[i] = iccLastCore;
          hTtoBox->pCld[i] = cldLastCore;
        }
        for (i = nParamBandsCore; i < hTtoBox->nOttBandsPhase; i++) {
          hTtoBox->pIpd[i] = ipdLastCore;
        }
      }

      if (bUseBBCues) {
        for (i = 1; i < nParamBands; i++) {
          hTtoBox->pCld[0] += hTtoBox->pCld[i];
        }
        hTtoBox->pCld[0] /= (float)nParamBands;
        for (i = 1; i < nParamBands; i++) {
          hTtoBox->pCld[i] = hTtoBox->pCld[0];
        }

        if (hTtoBox->bCalcResiduals) {
          for (i = 1; i < nParamBands; i++) {
            hTtoBox->pIccDownmix[0] += hTtoBox->pIccDownmix[i];
          }
          hTtoBox->pIccDownmix[0] /= (float)nParamBands;

          for (i = 1; i < nResidualBands; i++) {
            hTtoBox->pIccDownmix[i] = hTtoBox->pIccDownmix[0];
          }
        }

        if (!hTtoBox->bOneIcc) {
          for (i = 1; i < nParamBands; i++) {
            hTtoBox->pIcc[0] += hTtoBox->pIcc[i];
          }
          hTtoBox->pIcc[0] /= (float)nParamBands;
          for (i = 1; i < nParamBands; i++) {
            hTtoBox->pIcc[i] = hTtoBox->pIcc[0];
          }
        }

        if (hTtoBox->ipdMode != IPDMODE_NONE) {
          for (i = 1; i < hTtoBox->nOttBandsPhase; i++) {
            hTtoBox->pIpd[0] += hTtoBox->pIpd[i];
          }
          hTtoBox->pIpd[0] /= (float)hTtoBox->nOttBandsPhase;
          for (i = 1; i < hTtoBox->nOttBandsPhase; i++) {
            hTtoBox->pIpd[i] = hTtoBox->pIpd[0];
          }
        }
      }
    }

    if (error == noError) {
      QuantizeCoef(hTtoBox->pIccDownmix, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->nIccQuantSteps, hTtoBox->pIccDownmixIdx);
      deQuantizeCoef(hTtoBox->pIccDownmixIdx, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->pIccDownmixQuant);

      if (!hTtoBox->bCalcNoIcc) {
        if (pIccIdx != NULL) {
          QuantizeCoef(hTtoBox->pIcc, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->nIccQuantSteps, pIccIdx);
          deQuantizeCoef(pIccIdx, nParamBands, hTtoBox->pIccQuantTable, hTtoBox->nIccQuantOffset, hTtoBox->pIccQuant);
        } else {
          error = iisUtil_ERROR(CDI, "Invalid pointer to pIccIdx.");
        }
      }
    }
    if (error == noError) {
      if (!hTtoBox->bCalcNoIcc) {
        if (pbIccQuantCoarse != NULL) {
          *pbIccQuantCoarse = hTtoBox->bUseCoarseQuantIcc;
        } else {
          error = iisUtil_ERROR(CDI, "Invalid pointer to pbIccQuantCoarse");
        }
      }
    }

    if (error == noError) {
      if (pCldIdx != NULL) {
        {
          QuantizeCoef(hTtoBox->pCld, nParamBands, hTtoBox->pCldQuantTableEnc, hTtoBox->nCldQuantOffset, hTtoBox->nCldQuantSteps, pCldIdx);
          deQuantizeCoef(pCldIdx, nParamBands, hTtoBox->pCldQuantTableDec, hTtoBox->nCldQuantOffset, hTtoBox->pCldQuant);
        }
      } else {
        error = iisUtil_ERROR(CDI, "Invalid pointer to pCldIdx.");
      }
    }

    if (error == noError) {
      if (pbCldQuantCoarse != NULL) {
        *pbCldQuantCoarse = hTtoBox->bUseCoarseQuantCld;
      } else {
        error = iisUtil_ERROR(CDI, "Invalid pointer to pbCldQuantCoarse");
      }
    }

    if (error == noError && hTtoBox->bUsac212) {
      if (hTtoBox->ipdMode != IPDMODE_NONE) {
        if (pIpdIdx != NULL) {
          int nQuantSteps = getIpdQuantSteps(hTtoBox->bUseCoarseQuantIpd);
          quantizeIpd(hTtoBox->pIpd, hTtoBox->nOttBandsPhase, nQuantSteps, pIpdIdx);
          deQuantizeIpd(pIpdIdx, hTtoBox->nOttBandsPhase, nQuantSteps, hTtoBox->pIpdQuant);
          *bsPhaseMode = applyPhaseCorrections(hTtoBox, pCldIdx, pIccIdx, pIpdIdx, bUseBBCues);

          *numBinsIPD = hTtoBox->nOttBandsPhase;
        } else {
          error = iisUtil_ERROR(CDI, "Invalid pointer to pIpdIdx.");
        }
      }
    }
    if (error == noError && hTtoBox->bUsac212) {
      if (!hTtoBox->bCalcNoIcc) {
        if (noError != (error = calculateTtoDownmixParamUsac(hTtoBox, nHybridBands))) {
          error = handBack(error);
        }
      } else {
        for (i = 0; i < nParamBands; i++) {
          hTtoBox->pCplxDmWeight1[i].real = 1.0f;
          hTtoBox->pCplxDmWeight2[i].real = 1.0f;
        }
      }
    }

    if (hTtoBox->bCalcResiduals) {
      if (error == noError) {
        if (hTtoBox->bCalcIccDiff) {
          if (pIccDiff != NULL) {
            subINT(hTtoBox->pIccDownmixIdx, pIccIdx, pIccDiff, hTtoBox->nResidualBands);
          } else {
            error = iisUtil_ERROR(CDI, "Invalid pointer to pIccDiff.");
          }
        }
      }

      if (error == noError) {
        if (pbIccDiffPresent != NULL) {
          *pbIccDiffPresent = hTtoBox->bCalcIccDiff;
        } else {
          error = iisUtil_ERROR(CDI, "Invalid pointer to pbIccDiffPresent");
        }
      }

      if (error == noError) {
        if (ppHybridDataRealResidual == NULL) {
          error = iisUtil_ERROR(CDI, "Invalid buffer hybridDataRealResidual");
        }
      }

      if (error == noError) {
        if (ppHybridDataImagResidual == NULL) {
          error = iisUtil_ERROR(CDI, "Invalid buffer hybridDataImagResidual");
        }
      }
    }

    if (error == noError) {
      if (!hTtoBox->bInterpolateDownmix && hTtoBox->bUsac212) {
        paramextract_calculateUsacDmx(hTtoBox,
                                      ppHybridDataReal1,
                                      ppHybridDataImag1,
                                      (float const *const *)ppHybridDataReal2,
                                      (float const *const *)ppHybridDataImag2,
                                      nParamBands,
                                      nTimeSlots,
                                      nHybridBands);
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle hTtoBox");
  }

  return error;
}

static HANDLE_ERROR_INFO paramextract_calculateUsacDmx(HANDLE_TTO_BOX hTtoBox,
                                                       float **ppHybridDataReal1,
                                                       float **ppHybridDataImag1,
                                                       float const *const *ppHybridDataReal2,
                                                       float const *const *ppHybridDataImag2,
                                                       CLASSIC_MPS int nParamBands,
                                                       int nTimeSlots,
                                                       int nHybridBands) {
  HANDLE_ERROR_INFO error = noError;
  CPLX *cplxWeight1;
  CPLX *cplxWeight2;
  int i = 0, j = 0;

  if (NULL == hTtoBox) {
    error = iisUtil_ERROR(CDI, "Invalid handle hTtoBox");
    return error;
  }

  cplxWeight1 = hTtoBox->pCplxDmWeight1;
  cplxWeight2 = hTtoBox->pCplxDmWeight2;

  for (i = 0; i < nTimeSlots; i++) {
    for (j = 0; j < nHybridBands; j++) {
      int paramIndex = hTtoBox->pSubband2ParameterIndex[j];

      ppHybridDataReal1[i][j] = cplxWeight1[paramIndex].real * ppHybridDataReal1[i][j] + cplxWeight2[paramIndex].real * ppHybridDataReal2[i][j];
      ppHybridDataImag1[i][j] = cplxWeight1[paramIndex].real * ppHybridDataImag1[i][j] + cplxWeight2[paramIndex].real * ppHybridDataImag2[i][j];
    }
  }
  return error;
}

HANDLE_ERROR_INFO
GetTtoBoxDownmixMatrix(HANDLE_TTO_BOX hTtoBox,
                       int nHybridBands,
                       TTO_MIX_MATRIX *pDownmix) {
  HANDLE_ERROR_INFO error = noError;
  if (hTtoBox == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle hTtoBox");
  } else {
    int bCalcResiduals = hTtoBox->bCalcResiduals;

    CPLX *cplxWeight1 = hTtoBox->pCplxDmWeight1;
    CPLX *cplxWeight2 = hTtoBox->pCplxDmWeight2;
    CPLX *cplxWeight3 = hTtoBox->pCplxDmWeight3;
    CPLX *cplxWeight4 = hTtoBox->pCplxDmWeight4;

    int hybBand;

    if ((NULL == cplxWeight1) || (NULL == cplxWeight2)) {
      error = iisUtil_ERROR(CDI, "Invalid parameter cplxWeight1/2.");
    }

    if (bCalcResiduals && ((NULL == cplxWeight3) || (NULL == cplxWeight4))) {
      error = iisUtil_ERROR(CDI, "Invalid parameter cplxWeight3/4.");
    }
    if (error == noError) {
      for (hybBand = 0; hybBand < nHybridBands; hybBand++) {
        pDownmix[hybBand].m[0][0].real = cplxWeight1[hybBand].real;
        pDownmix[hybBand].m[0][0].imag = cplxWeight1[hybBand].imag;
        pDownmix[hybBand].m[0][1].real = cplxWeight2[hybBand].real;
        pDownmix[hybBand].m[0][1].imag = cplxWeight2[hybBand].imag;

        if (bCalcResiduals) {
          pDownmix[hybBand].m[1][0].real = cplxWeight3[hybBand].real;
          pDownmix[hybBand].m[1][0].imag = cplxWeight3[hybBand].imag;
          pDownmix[hybBand].m[1][1].real = cplxWeight4[hybBand].real;
          pDownmix[hybBand].m[1][1].imag = cplxWeight4[hybBand].imag;
        } else {
          pDownmix[hybBand].m[1][0].real = 0.0f;
          pDownmix[hybBand].m[1][0].imag = 0.0f;
          pDownmix[hybBand].m[1][1].real = 0.0f;
          pDownmix[hybBand].m[1][1].imag = 0.0f;
        }
        pDownmix[hybBand].infinity = 0;
      }
    }
  }

  return error;
}

int getCldQuantOffset(int bUseCoarseQuant) {
  int quantOffset = 0;

  if (bUseCoarseQuant) {
    quantOffset = offsetCldQuantCoarse;
  } else {
    quantOffset = offsetCldQuantFine;
  }

  return quantOffset;
}

int getIccQuantOffset(int bUseCoarseQuant) {
  int quantOffset = 0;

  if (bUseCoarseQuant) {
    quantOffset = offsetIccQuantCoarse;
  } else {
    quantOffset = offsetIccQuantFine;
  }

  return quantOffset;
}

int getNumberCldQuantLevels(int bUseCoarseQuant) {
  int nQuantLevels = 0;

  if (bUseCoarseQuant) {
    nQuantLevels = MAX_CLD_QUANT_COARSE;
  } else {
    nQuantLevels = MAX_CLD_QUANT_FINE;
  }

  return nQuantLevels;
}

int getNumberIccQuantLevels(int bUseCoarseQuant) {
  int nQuantLevels = 0;

  if (bUseCoarseQuant) {
    nQuantLevels = MAX_ICC_QUANT_COARSE;
  } else {
    nQuantLevels = MAX_ICC_QUANT_FINE;
  }

  return nQuantLevels;
}

float *getCldQuant(HANDLE_TTO_BOX hTtoBox) {
  return hTtoBox->pCldQuant;
}

float paramBand2Freq(BOX_SUBBAND_CONFIG boxSubbandConfig, int nSampleRate, int nParamBand, int nQmfBands) {
  float qmfSlotWidth = (nSampleRate / 2.f) / nQmfBands;
  float qmfBandWidth = 0.f;
  int band;
  const float *parameterBandQmfWidth = getParameterBandQmfWidth(boxSubbandConfig);

  if (parameterBandQmfWidth != NULL) {
    for (band = 0; band < nParamBand; band++) {
      qmfBandWidth += parameterBandQmfWidth[band];
    }
  }

  return qmfBandWidth * qmfSlotWidth;
}

float getUniSteCld(const HANDLE_TTO_BOX hTtoBox, int parameterBand) {
  return hTtoBox->pCldQuant[parameterBand];
}
