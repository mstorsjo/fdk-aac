
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

#ifndef TON_CORR_EST_H
#define TON_CORR_EST_H

#include "errorhnd.h"
#include "sbr_main.h"
#include "sbr_def.h"
#include "mh_det.h"
#include "invf_est.h"
#include "nf_est.h"

#define MAX_NUM_PATCHES 6

typedef struct {
  int sourceStartBand;
  int sourceStopBand;
  int guardStartBand;
  int targetStartBand;
  int targetBandOffs;
  int numBandsInPatch;
} PATCH_PARAM;

typedef struct {
  int stopBand;
  int startIndexMatrix;
  int numberOfEstimates;
  int numberOfEstimatesPerFrame;
  int noQmfChannels;
  int stepSize[2];
  int blockLength[2];
  int firstSample;
  float* nrgVector;
  float* pNrgVectorFreq;
  float** quotaMatrix;
  int** signMatrix;
} LPC_PARAM;

typedef struct
{
  int switchInverseFilt;
  int frameStartIndex;
  int frameStartIndexInvfEst;
  int prevTransientFlag;
  int transientNextFrame;
  int transientPosOffset;

  int* indexVector;

  int indexVectorDef[64];

  LPC_PARAM lpcParams;
  int indexVectorPV[64];
  LPC_PARAM lpcParamsPatch;

  PATCH_PARAM patchParam[MAX_NUM_PATCHES + 1];
  int guard;
  int shiftStartSb;
  int noOfPatches;

  HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMissingHarmonicsDetector;
  HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate;
  HANDLE_SBR_INV_FILT_EST hSbrInvFilt;
} SBR_TON_CORR_EST;

typedef SBR_TON_CORR_EST* HANDLE_SBR_TON_CORR_EST;

void TonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                      INVF_MODE* infVec,
                      float* noiseLevels,
                      int* missingHarmonicFlag,
                      int* missingHarmonicsIndex,
                      int* envelopeCompensation,
                      const SBR_FRAME_INFO* frameInfo,
                      const int* transientInfo,
                      const int* freqBandTable,
                      int nSfb,
                      CODEC_TYPE coreCodec,
                      XPOS_MODE xposType,
                      const int sbrPatchingMode,
                      const int bPitchDetected,
                      const int bSbr41,
                      float noiseLevelLoweringFactor[MAX_NOISE_ENVELOPES]);

HANDLE_ERROR_INFO
CreateTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST* hTonCorr,
                       int frameSize,
                       int timeSlots,
                       int nCols,
                       int encDelay,
                       int fs,
                       int noQmfChannels,
                       int xposCtrl,
                       int highBandStartSb,
                       const int* v_k_master,
                       int numMaster,
                       int ana_max_level,
                       const int* const* const freqBandTable,
                       const int* nSfb,
                       int noiseBands,
                       int* noiseFloorOffset,
                       unsigned int useMissHarmonicsDet,
                       unsigned int useSpeechConfig,
                       CODEC_TYPE coreCoder);

void DeleteTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                            CODEC_TYPE coreCodec);

HANDLE_ERROR_INFO
CalculateTonalityQuotas(LPC_PARAM* hLpcParam,
                        float** sourceBufferReal,
                        float** sourceBufferImag,
                        CODEC_TYPE coreCodec);

HANDLE_ERROR_INFO
ResetTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                      int xposctrl,
                      int highBandStartSb,
                      const int* v_k_master,
                      int numMaster,
                      int fs,
                      const int* const* const freqBandTable,
                      const int* nSfb,
                      int noQmfChannels,
                      CODEC_TYPE coreCodec);

#endif
