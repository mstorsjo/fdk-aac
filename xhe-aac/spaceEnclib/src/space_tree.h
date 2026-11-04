
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

#ifndef space_tree_h_
#define space_tree_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "iisutillib.h"

#include "spaceEnclib_const.h"
#include "space_bitstream.h"

#define MAX_NUM_BOXES 8

typedef enum {
  SPACETREE_INVALID_MODE = 0,
  SPACETREE_5151 = 1,
  SPACETREE_5152 = 2,
  SPACETREE_525 = 3,
  SPACETREE_7271 = 4,
  SPACETREE_7272 = 5,
  SPACETREE_7571 = 6,
  SPACETREE_7572 = 7,
  SPACETREE_212 = 8,
  SPACETREE_929 = 9,
  SPACETREE_959 = 10

  ,
  SPACETREE_USAC_212 = 16

} SPACETREE_MODE;

typedef struct SPACE_TREE *HANDLE_SPACE_TREE;

typedef struct {
  int nParamBands;
  int bOneIcc;
  int bUseCoarseQuantTtoCld;
  int bUseCoarseQuantTtoIcc;

  int bUseCoarseQuantTtoIpd;

  int bUseCoarseQuantTttLowCldCpc;
  int bUseCoarseQuantTttHighCldCpc;
  int bUseCoarseQuantTttLowIcc;
  int bUseCoarseQuantTttHighIcc;
  QUANTMODE quantMode;
  int bCalcResiduals[MAX_NUM_BOXES];
  int nResidualBands[MAX_NUM_BOXES];
  int bCalcDPL;
  SPACETREE_MODE mode;

  float epsilonFloat;
  int nChannelsInMax;
  int nTimeSlotsMax;
  int nHybridBandsMax;

  int bApplyLfeFilter;
  int nBandsLfe;

  int bPhaseAlignLfe;

  int bCalcIccDiff;
  int nHybBandsCore;
  int bStereoSbr;
  int bPsStyleDmx;
  DOWNMIXTYPE downmixType;
  IPDMODE ipdMode;
  int bDetectIpdRelevancy;
  int bInterpolateDownmix;
  int bOttBandsPhasePresent;
  int nOttBandsPhase;

  int bUseTsd;

} SPACE_TREE_SETUP;

typedef struct {
  int nOttBoxes;
  int nTttBoxes;
  int nInChannels;
  int nOutChannels;
  int nResidualChannels;

  int bOttModeLfe[MAX_NUM_BOXES];

} SPACE_TREE_DESCRIPTION;

HANDLE_ERROR_INFO
SpaceTree_CalcDownmixHold(HANDLE_SPACE_TREE hST,
                          int nparamSet,
                          int paramSet,
                          int nTimeSlots,
                          int nHybridBands,
                          SPATIALFRAME *hSTOut,
                          float ***pppHybridInReal,
                          float ***pppHybridInImag,
                          float **ppUmxMatReal[2][2],
                          float **ppUmxMatImag[2][2],
                          float ***pppHybridOutReal,
                          float ***pppHybridOutImag);

HANDLE_ERROR_INFO
SpaceTreeOutput_Reset(SPATIALFRAME *hSTOut);

HANDLE_ERROR_INFO
SpaceTree_Open(HANDLE_SPACE_TREE *phSpaceTree, SPACE_TREE_SETUP *hSetup, int bLowDelay, int bFrameKeep);

HANDLE_ERROR_INFO
SpaceTree_Apply(HANDLE_SPACE_TREE hSpaceTree,
                int paramSet,
                int nChannelsIn,
                int nTimeSlots,
                int nHybridBands,
                float ***pppHybridDmxInReal,
                float ***pppHybridDmxInImag,
                float **ppUmxMatReal[2][2],
                float **ppUmxMatImag[2][2],
                float ***pppHybridInReal,
                float ***pppHybridInImag,
                float ***pppHybridOutReal,
                float ***pppHybridOutImag,
                SPATIALFRAME *hSpaceTreeOut,
                int avoid_keep,
                int speechFlag);

HANDLE_ERROR_INFO
SpaceTree_Close(HANDLE_SPACE_TREE *phSpaceTree);

HANDLE_ERROR_INFO
SpaceTree_GetDescription(HANDLE_SPACE_TREE hSpaceTree, SPACE_TREE_DESCRIPTION *pSpaceTreeDescription);

int SpaceTree_Hybrid2ParamBand(int nParamBands, int nHybridBand, MPS_MODE mode);

float SpaceTree_ParamBand2Freq(int nParamBands, int nSampleRate, int nParamBand, int nQmfBands);

float SpaceTree_GetUniSteCld(HANDLE_SPACE_TREE hSpaceTree, int parameterBand);

#ifdef __cplusplus
}
#endif

#endif
