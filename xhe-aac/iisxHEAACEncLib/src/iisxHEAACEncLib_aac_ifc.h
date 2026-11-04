
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

#ifndef IISXHEAACENCLIB_AAC_IFC_H
#define IISXHEAACENCLIB_AAC_IFC_H

#include "iisxHEAACEncLibConfig.h"
#include "extpayload.h"
#include "iisxHEAACEncLib_returnCodes.h"
#include "IIS_loaswrite_interface.h"
#include "iisutillib.h"
#include "iisSigMap.h"

struct AacEncoder;
typedef struct AacEncoder XHEAACENCLIB_AACENCODER, *XHEAACENCLIB_HANDLE_AACENCODER;

struct UNISTE_TAG;
typedef struct UNISTE_TAG XHEAACENCLIB_AACUNISTE, *XHEAACENCLIB_HANDLE_AACUNISTE;

struct channel_mapping_struct;

typedef struct xheaacenclib_aacinfo_struct {
  float bandWidth;
  int nDelay;
  int nAddEncDelay;
  int nStandDelay;
  int cbBufSizeMin;
  int nAncBytesPerFrame;
} XHEAACENCLIB_AACINFO, *XHEAACENCLIB_HANDLE_AACINFO;

typedef enum xheaac_aacenc_ipf_state {
  XHEAAC_AACENC_IPF_STATE_NO,
  XHEAAC_AACENC_IPF_STATE_RAP_FIRST_PREROLL,
  XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL,
  XHEAAC_AACENC_IPF_STATE_RAP_NEXT_PREROLL,
  XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL,
  XHEAAC_AACENC_IPF_STATE_RAP_IPF,
  XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_IPF,
  XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL,
  XHEAAC_AACENC_IPF_STATE_CONFIGCHANGE_IPF_PREROLL

} XHEAAC_AACENC_IPF_STATE;

typedef enum {
  XHEAAC_AACENC_PCE_DATA_MATRIX_MIXDOWN_IDX_PRESENT = 0,
  XHEAAC_AACENC_PCE_DATA_MATRIX_MIXDOWN_IDX = 1,
  XHEAAC_AACENC_PCE_DATA_PSEUDO_SURROUND_ENABLE = 2
} XHEAAC_AACENC_PCE_DATA;

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncConfigure(
    XHEAACENCLIB_HANDLE_AACENCODER* p_hAAcEnc,
    XHEAACENCLIB_CONFIG_HANDLE hConfig);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncOpen(XHEAACENCLIB_HANDLE_AACENCODER self,
                          int sampleRateIn,
                          float proposedBandwidth,
                          HANDLE_STREAM_FORMAT hLatmLoas);
struct PROGRAM_CONFIG;
XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetNoStartStopSequence(XHEAACENCLIB_HANDLE_AACENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetInfo(XHEAACENCLIB_HANDLE_AACENCODER self,
                             XHEAACENCLIB_HANDLE_AACINFO hAacInfo);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetBitReservoirInfo(XHEAACENCLIB_HANDLE_AACENCODER self,
                                         int bitReservoirMax,
                                         int* bitReservoir,
                                         float* bitReservoirLevel);

int iisxHEAACEncLibAacEncUseNoiseFilling(XHEAACENCLIB_HANDLE_AACENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSnapToLowSfbBorder(XHEAACENCLIB_HANDLE_AACENCODER self,
                                        const float desiredBandwidth,
                                        const float tol,
                                        float* adjBandwidth);

void iisxHEAACEncLibAacEncSetBandwidth(XHEAACENCLIB_HANDLE_AACENCODER self,
                                       float bandWidth);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSAPPrepare(XHEAACENCLIB_HANDLE_AACENCODER self,
                                const XHEAACENCLIB_SAP_TYPE syncType);

int iisxHEAACEncLibAacEncGetVBRBitrate(XHEAACENCLIB_BITRATE_MODE bitrateMode,
                                       CHANNEL_MAPPING_HANDLE hChMap,
                                       XHEAACENCLIB_CODEC_TYPE codecType);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncClose(
    XHEAACENCLIB_HANDLE_AACENCODER self);

struct HAWAII_STREAM;
struct lpd_enc_public_data_struct;
struct UNISTE_TAG;

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetMinAuBytes(XHEAACENCLIB_HANDLE_AACENCODER self,
                                   int minFrameBytes);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncEncode(XHEAACENCLIB_HANDLE_AACENCODER self,
                            float* pSamples,
                            const int nSamples,
                            unsigned char* const pOutput,
                            unsigned char* const pOutputApr,
                            const int cbSize,
                            int* const cbOutBits,
                            int* const cbOutBitsApr,
                            unsigned char* pAncDrcBitstream,
                            unsigned int nAncDrcBytes,
                            int* coreModeNext,
                            XHEAACENCLIB_HANDLE_UNISTE* phUniSte,
                            unsigned int nBitsTransportOverhead,
                            const int bUsacIndepFlag,
                            HANDLE_EXTPAYLOAD_CONTAINER* hExtElement,
                            int numExtElementInUse,
                            XHEAAC_AACENC_IPF_STATE const ipfState,
                            int nChannels);

int iisxHEAACEncLibAacEncGetResidualCoding(XHEAACENCLIB_CONFIG_HANDLE self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncUpdateExtendedBitReservoir(XHEAACENCLIB_HANDLE_AACENCODER self, int nBits, const int intervalSamples, float preRollAUFactor);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetBitReservoirLevel(XHEAACENCLIB_HANDLE_AACENCODER self, float bitReservoirLevel);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetBitsToUseForExternalData(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    int* maxNumBitsToUse,
    int* comfortableNumBitsToUse,
    const int bUsacIndepFlag,
    XHEAAC_AACENC_IPF_STATE ipfState);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncIsLastShortWindow(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    int* isLastShortWindow);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncGetPceTimeInterval(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    float* sendPceTimeInterval);

XHEAACENCLIB_RETURN
iisxHEAACEncLibAacEncSetPceTimeInterval(
    XHEAACENCLIB_HANDLE_AACENCODER self,
    float sendPceTimeInterval);

#endif
