
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

#ifndef FRAM_GEN_H
#define FRAM_GEN_H

#include "sbr_def.h"
#include "sbr_main.h"
#include "errorhnd.h"

#define MAX_ENVELOPES_VARVAR MAX_ENVELOPES
#define MAX_ENVELOPES_FIXVAR_VARFIX 4
#define MAX_NUM_REL 3

#define FRAME_MIDDLE_SLOT_2048 4
#define NUMBER_TIME_SLOTS_2048 16
#undef MAX_NUMBER_TIME_SLOTS
#define MAX_NUMBER_TIME_SLOTS NUMBER_TIME_SLOTS_2048

typedef enum {
  SBR_FRAME_CLASS_INVALID = -1,
  FIXFIX = 0,
  FIXVAR,
  VARFIX,
  VARVAR
} FRAME_CLASS;

#define DC 4711
#define EMPTY (-99)

typedef struct
{
  int staticFraming;
  int numEnvStatic;
  FREQ_RES freq_res_fixfix[2];

  int dmin;
  int dmax;
  int allowSpread;
  int segmentLength[3];
  FREQ_RES segmentRes[3];
  FREQ_RES freqResFillPre;
  FREQ_RES freqResFillPost;

  int minEnvSize4highRes;
} FRAME_GEN_TUNING;

typedef struct
{
  int numberTimeSlots;

  FRAME_CLASS frameClass;
  int bs_num_env;
  int bs_abs_bord;
  int n;
  int p;
  int bs_rel_bord[MAX_NUM_REL];
  FREQ_RES v_f[MAX_ENVELOPES_FIXVAR_VARFIX];

  int bs_abs_bord_0;
  int bs_abs_bord_1;
  int bs_num_rel_0;
  int bs_num_rel_1;
  int bs_rel_bord_0[MAX_NUM_REL];
  int bs_rel_bord_1[MAX_NUM_REL];
  FREQ_RES v_fLR[MAX_ENVELOPES_VARVAR];
} SBR_GRID;
typedef SBR_GRID *HANDLE_SBR_GRID;

typedef struct
{
  int nEnvelopes;
  int borders[MAX_ENVELOPES + 1];
  FREQ_RES freqRes[MAX_ENVELOPES];
  int shortEnv;
  int nNoiseEnvelopes;
  int bordersNoise[MAX_NOISE_ENVELOPES + 1];
} SBR_FRAME_INFO;

typedef SBR_FRAME_INFO *HANDLE_SBR_FRAME_INFO;

typedef struct
{
  int frameMiddleSlot;
  FRAME_GEN_TUNING tuning;

  FRAME_CLASS frameClassOld;

  FREQ_RES aFreqResFollow[MAX_ENVELOPES_VARVAR];
  FREQ_RES aFreqRes[2 * MAX_ENVELOPES_VARVAR + 1];

  int aBorders[2 * MAX_ENVELOPES_VARVAR + 1];
  int aBordersFollow[MAX_ENVELOPES_VARVAR];

  int spreadFlag;

  int borderVecLen;
  int freqResVecLen;
  int borderVecLenFollow;

  int tranIdxFollow;
  int fillIdxFollow;
  int freqResVecLenFollow;

  int sibBorders[MAX_ENVELOPES + 1];
  int sibBordersCnt;
  int sibBorderPrevFrame;
  int lastTilt;

  HANDLE_SBR_GRID hSbrGrid;
  HANDLE_SBR_FRAME_INFO hSbrFrameInfo;
} SBR_ENVELOPE_FRAME;
typedef SBR_ENVELOPE_FRAME *HANDLE_SBR_ENVELOPE_FRAME;

HANDLE_ERROR_INFO
CreateFrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME *hSbrEnvFrame,
                         int allowSpread,
                         int numEnvStatic,
                         int staticFraming,
                         FREQ_RES freqResFillPre,
                         FREQ_RES freqResFillPost,
                         int timeSlots,
                         FREQ_RES *freq_res_fixfix,
                         CODEC_TYPE coreCodec,
                         sbrConfigurationPtr params

);

void DeleteFrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame);

HANDLE_SBR_FRAME_INFO
FrameInfoGenerator(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame,
                   int *v_transient_info,
                   int rightBorderFIX,
                   CODEC_TYPE coreCodec,
                   const int switchingDecision,
                   const int fixfixGridGranularity);

HANDLE_SBR_FRAME_INFO
FrameInfoGeneratorSibilant(HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame,
                           const int *v_transient_info,
                           CODEC_TYPE coreCodec,
                           const int switchingDecision,
                           const int fixfixGridGranularity);

#endif
