
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

#ifndef NF_EST_H
#define NF_EST_H

#include "IISutillib/errorhnd.h"
#include "sbr_main.h"
#include "sbr_def.h"
#include "fram_gen.h"

typedef struct
{
  float** prevNoiseLevels;
  int* freqBandTableQmf;
  float ana_max_level;
  float weightFac;
  int noNoiseBands;
  int noiseBands;
  float noiseFloorOffset[MAX_NUM_NOISE_COEFFS];
  float noiseReductionFactor;
  int timeSlots;
  int smoothLength;
  const float* smoothFilter;
  INVF_MODE diffThres;
} SBR_NOISE_FLOOR_ESTIMATE;

typedef SBR_NOISE_FLOOR_ESTIMATE* HANDLE_SBR_NOISE_FLOOR_ESTIMATE;

void SbrNoiseFloorEstimateQmf(HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate,
                              const SBR_FRAME_INFO* frame_info,
                              float* noiseLevels,
                              float** quotaMatrixOrig,
                              float** quotaMatrixPatch,
                              int* indexVector,
                              int missingHarmonicsFlag,
                              int startIndex,
                              int numberOfEstiamtesPerFrame,
                              int totalNumberOfEstimates,
                              int transientFlag,
                              INVF_MODE* pInvFiltLevels,
                              CODEC_TYPE coreCodec,
                              const int sbrPatchingMode,
                              int bSbr41,
                              float noiseLevelLoweringFactor[MAX_NOISE_ENVELOPES]);

HANDLE_ERROR_INFO
CreateSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE* hSbr,
                            int ana_max_level,
                            const int* freqBandTable,
                            int nSfb,
                            int noiseBands,
                            int* noiseFloorOffset,
                            int timeSlots,
                            unsigned int useSpeechConfig);

HANDLE_ERROR_INFO
ResetSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbr,
                           const int* freqBandTable,
                           int nSfb);

void DeleteSbrNoiseFloorEstimate(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbrCut);

void CalcNoiseLevelLoweringFactors(HANDLE_SBR_FRAME_INFO frameInfo,
                                   const float* const* const Energies,
                                   float* noiseLevelLoweringFactor,
                                   const int* freqBandTable,
                                   int nSfb,
                                   int timeStep,
                                   int noCols);

void AdjustNoiseFloor(HANDLE_SBR_NOISE_FLOOR_ESTIMATE hSbrNoiseFloorEstimate,
                      float noiseReductionFactor);
#endif
