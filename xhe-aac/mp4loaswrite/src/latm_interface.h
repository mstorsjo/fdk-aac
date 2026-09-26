
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

#ifndef LATM_INTERFACE_H
#define LATM_INTERFACE_H

#include <assert.h>

#include "bit_buf.h"
#include "IIS_loaswrite_interface.h"

#define LATM_MAX_STREAM_ID (LATM_MAX_PROGRAMS)

#define SMC_INTERVALL_DEFAULT (8)
#define NROFSUBFRAMES_DEFAULT (1)

typedef struct {
  int chunkLenBits;

  int prog;
  int layer;
} VAR_MUX_SETUP_INFO;

typedef enum { LATMVAR_SIMPLE_SEQUENCE } LATM_VAR_MODE;

typedef struct {
  int frameLengthType;
  int frameLengthBits;
  int varFrameLengthTable[4];

  int streamID;

  int m_nConsideredChannels;
  int bufferFullnessPosition;

} LATM_LAYER_INFO;

typedef struct {
  int audioMuxLengthBytes;
  int syncFrameCounter;
  int setupDataDistanceFrames;
  int setupDataOutOfBandFlag;
} LATM_SYNC_LAYER_INFO;

typedef struct LASI {
  LOASWRITER_MODE mode;

  LATM_SYNC_LAYER_INFO lsli;

  int allStreamsSameTimeFraming;

  int subFrameCnt;
  int nrOfSubframes;
  int nrOfSubframes_next;
  int nrOfSubframesPosition;

  int noProgram;
  int noLayer[LATM_MAX_PROGRAMS];

  int fractDelayPresent;

  LATM_LAYER_INFO m_linfo[LATM_MAX_PROGRAMS][MAX_LAYERS];

  VAR_MUX_SETUP_INFO vmsi[LATM_MAX_STREAM_ID];
  int varStreamCnt;
  LATM_VAR_MODE varMode;

  unsigned int otherDataLenBytes;

  int streamMuxConfigBits;

  HANDLE_BIT_BUF hSetupData;

  HANDLE_BIT_BUF hAssembleBuffer;
  int smcOutOfBitres;
} LATM_STREAM_INFO;

typedef struct LASI *HANDLE_LATM_STREAM_INFO;

HANDLE_ERROR_INFO
CreateLatmStream(HANDLE_LATM_STREAM_INFO *p_hAss,
                 LOASWRITER_MODE mode,
                 int fractDelayPresent,
                 int noProgram,
                 HANDLE_CODER_CONFIG_INFO layerConfig[LATM_MAX_PROGRAMS][MAX_LAYERS]);

HANDLE_ERROR_INFO
SetSmcInterval(HANDLE_LATM_STREAM_INFO hAss,
               int setupDataDistanceFrames);

HANDLE_ERROR_INFO
SetSmcOutOfBitres(HANDLE_LATM_STREAM_INFO hAss,
                  int smcOutOfBitres);

HANDLE_ERROR_INFO
SetNrOfSubframes(HANDLE_LATM_STREAM_INFO hAss,
                 int nrOfSubframes_next);

HANDLE_ERROR_INFO
AdvanceLatmStream(HANDLE_LATM_STREAM_INFO hAss,
                  HANDLE_BIT_BUF hLayerBitstream[LATM_MAX_PROGRAMS][MAX_LAYERS],
                  int bufferFullness[LATM_MAX_PROGRAMS][MAX_LAYERS],
                  unsigned char *outBuffer,
                  int *outNoBytes,
                  LOASWRITER_FRAME_INFO_HANDLE hFrameInfo);

HANDLE_ERROR_INFO
DeleteLatmStream(HANDLE_LATM_STREAM_INFO hAss);

unsigned int
CountLatmBitDemandHeader(HANDLE_LATM_STREAM_INFO hAss, unsigned int streamDataLength, int nMode);

HANDLE_ERROR_INFO
GetLatmStreamMuxConfig(HANDLE_LATM_STREAM_INFO hAss,
                       unsigned char *buffer,
                       int *nBits);

HANDLE_ERROR_INFO
TriggerSmc(HANDLE_LATM_STREAM_INFO hAss);

#endif
