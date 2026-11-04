
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

#ifndef IISXHEAACENCLIB_SBR_IFC_H
#define IISXHEAACENCLIB_SBR_IFC_H

#include "iisutillib.h"
#include "extpayload.h"
#include "usacconfig.h"

#include "iisxHEAACEncLibConfig.h"
#include "iisSwitchingDecision.h"

#define XHEAACENCLIB_SBR_MAX_ELEMENTS 15
#define XHEAACENCLIB_SBR_MAX_EXT_PAYLOAD 8

struct SbrEncEncoder;
typedef struct SbrEncEncoder XHEAACENCLIB_SBRENCODER, *XHEAACENCLIB_HANDLE_SBRENCODER;

struct COMMON_DATA;
struct iis_swdeci;

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncConfigure(
    XHEAACENCLIB_HANDLE_SBRENCODER *p_hSbrEnc,
    XHEAACENCLIB_CONFIG_HANDLE hConfig);

float iisxHEAACEncLibSbrEncGetXOverFreq(
    XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncOpen(XHEAACENCLIB_HANDLE_SBRENCODER self,
                          int *sbrEncoderDelay,
                          int *sbrDecoderDelay);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrInitDelayCompensation(
    XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
    const int coreCoderDelay,
    const int mpsDelay,
    int *pAdditionalDelay);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncClose(
    XHEAACENCLIB_HANDLE_SBRENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncEncode(XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                            const float *const pTimeSigIn,
                            const int nSamplesIn,
                            float *pTimeSigOut,
                            unsigned int *pnSamplesOut,
                            HANDLE_IIS_SWDECI hSwdeci,
                            int const bUsacIndependenceFlag);

struct COMMON_DATA **
iisxHEAACEncLibSbrEncGetCmonDataHandle(
    XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetOffsets(
    const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
    const unsigned int *const channelOffsets);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetSbrTransmissionConfig(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                              const int bUseCRC,
                                              const float sendHeaderTimeInterval);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetRaisedXoverFreq(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                        const int raisedXoverFreq);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetPsTimeSignal(
    const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
    float **pSamplesOut,
    unsigned int *nSamplesOut);

int iisxHEAACEncLibSbrEncGetPsTimeSignalDelay(
    const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

int iisxHEAACEncLibSbrEncGetEstimateSbrBitrate(
    const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

int iisxHEAACEncLibSbrEncGetSbrPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

int iisxHEAACEncLibSbrEncGetPsPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetRightBorderFIX(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                       int rightBorderFIX);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSendHeader(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncContainsHeader(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                    int *headerFrame);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSendHeaderDelay(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                     int *sendHeaderDelay);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrMoveExtPayload(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                 HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
                                 int *usedBits);

int iisxHEAACEncLibSbrEncGetHbePresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el);

int iisxHEAACEncLibSbrEncGetBsInterTes(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el);

int iisxHEAACEncLibSbrEncGetPvcPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetUsacSbrDfltHeaderData(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                              USAC_SBR_HEADER *pUsacSbrDfltHeader,
                                              const int el);

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetStopFreq(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                 float *stopFreq);

#endif
