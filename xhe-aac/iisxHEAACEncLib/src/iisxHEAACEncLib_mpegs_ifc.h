
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

#ifndef IISXHEAACENCLIB_MPEGS_IFC_H
#define IISXHEAACENCLIB_MPEGS_IFC_H

#include "iisutillib.h"
#include "iisxHEAACEncLib_returnCodes.h"
#include "iisxHEAACEncLibConfig.h"
#include "iisxHEAACEncLib_syncframe.h"
#include "extpayload.h"
#include "spaceEnclib.h"
#include "iisSigMap.h"

struct MpegsEncoder;
typedef struct MpegsEncoder XHEAACENCLIB_MPEGSENCODER, *XHEAACENCLIB_HANDLE_MPEGSENCODER;

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncConfigure(
    XHEAACENCLIB_HANDLE_MPEGSENCODER *p_hMpegsEnc,
    XHEAACENCLIB_CONFIG_HANDLE hConfig);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncOpen(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                            MP4SPACEENC_ENCODERTYPE mpsEncoderType,
                            int frameLengthAAC,
                            float aacCoreBandwidth,
                            int bUseSbrQmfInput);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncInitDelayCompensation(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                             int addEncDelay,
                                             int standardDelay,
                                             int *nAddEncDelayMax,
                                             int *nStandardDelayMax,
                                             int *nSamplesFrameMax);
XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncInitUnivSte(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                   int nAacResampAndLookaheadDelay,
                                   int sampleRate,
                                   int sampleRateAAC,
                                   float aacCoreBandwidth,
                                   int *dmxDelay);

int iisxHEAACEncLibMpegsEncGetMaxDelay(XHEAACENCLIB_HANDLE_MPEGSENCODER self);

MP4SPACEENC_RES_CONFIG *
iisxHEAACEncLibMpegsEncGetResidualConfig(XHEAACENCLIB_HANDLE_MPEGSENCODER self);

XHEAACENCLIB_HANDLE_UNISTE *
iisxHEAACEncLibMpegsEncGetUnifiedStereoHandle(XHEAACENCLIB_HANDLE_MPEGSENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncEncode(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                              int coreCoderFrameLength,
                              XHEAACENCLIB_SYNCFRAME_HANDLE hUsacIndepFlag,
                              int speechFlag,
                              float *pPreResamplerOut,
                              int nChannels,
                              int *nSamplesPreResampOut,
                              float **ppQmfSamplesReal,
                              float **ppQmfSamplesImag,
                              int nChannelsCoreCoder,
                              float *pPreResamplerOutLr,
                              int const usePreResamplerOutLr);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncGetUsacMps212Config(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                           int eleIdx,
                                           MP4SPACEENC_USAC_MPS212_CONFIG *usacMps212Config);

void iisxHEAACEncLibMpegsEncUpdateCpcStopFrequency(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                                   float aacCoreBandwidth);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncClose(
    XHEAACENCLIB_HANDLE_MPEGSENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpsThisExtPayloadSize(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                     int *payloadSize);

XHEAACENCLIB_RETURN iisxHEAACEncLibMpsMoveExtPayload(
    int const nElements,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadMps[],
    int *const usedBits);

#endif
