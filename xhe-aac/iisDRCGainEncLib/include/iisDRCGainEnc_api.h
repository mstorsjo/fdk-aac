
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

#ifndef IISDRCGAINENC_API_H
#define IISDRCGAINENC_API_H

#if defined __cplusplus
extern "C" {
#endif

#include "iisDRCGainEnc_uniDrc.h"

typedef enum {
  IISDRCGAINENC_SYNTAXMODE_DEFAULT
} IISDRCGAINENC_SYNTAXMODE;

typedef enum {
  IISDRCGAINENC_RETURN_NOERROR = 0,
  IISDRCGAINENC_RETURN_ERROR_UNKNOWN,
  IISDRCGAINENC_RETURN_ERROR_INPUTHANDLE,
  IISDRCGAINENC_RETURN_ERROR_NULLPOINTER,
  IISDRCGAINENC_RETURN_ERROR_MEMORY,
  IISDRCGAINENC_RETURN_ERROR_ILLEGAL_ARRAY_ACCESS,
  IISDRCGAINENC_RETURN_ERROR_WRONGCHANNELCOUNT,
  IISDRCGAINENC_RETURN_ERROR_WRONGGROUPCOUNT,
  IISDRCGAINENC_RETURN_ERROR_WRONGCONFIG,
  IISDRCGAINENC_RETURN_ERROR_WRITEBITS,
  IISDRCGAINENC_RETURN_ERROR_UNKNOWN_METHOD_DEF,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDNODESETTINGMODE,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDSYNTAXMODE,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDSAMPLERATE,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDTIMEDELTA,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDGAINCODINGPROFILE,
  IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDINSTRUCTIONTYPE
} IISDRCGAINENC_RETURN;

typedef enum {
  IISDRCGAINENC_NODESETTINGMODE_SINGLENODE = 0,
  IISDRCGAINENC_NODESETTINGMODE_ALLNODES,
  IISDRCGAINENC_NODESETTINGMODE_SALIENTNODE
} IISDRCGAINENC_NODESETTINGMODE;

typedef struct T_IISDRCGAINENC_PARAMS *HANDLE_IISDRCGAINENC_PARAMS;

typedef struct {
  UniDrcConfig *pUniDrcConfig;
  LoudnessInfoSet *pLoudnessInfoSet;
  IISDRCGAINENC_NODESETTINGMODE nodeSettingMode;
  int sampleRate;
  int frameSize;
} IISDRCGAINENC_CONFIG;

IISDRCGAINENC_RETURN
iisDRCGainEnc_open(HANDLE_IISDRCGAINENC_PARAMS *phIisDrcGainEnc_params,
                   IISDRCGAINENC_CONFIG *pIisDRCGainEnc_config,
                   int *pNumSequences);

IISDRCGAINENC_RETURN
iisDRCGainEnc_close(HANDLE_IISDRCGAINENC_PARAMS *phIisDrcGainEnc_params);

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcConfig(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                IISDRCGAINENC_SYNTAXMODE syntaxModeFlag,
                                unsigned char *pBitstreamBuffer,
                                int *pBitCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcGain(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                              const float *const *drcGainInputBuffer,
                              const int bIsPrerollFrame,
                              unsigned char *pBitstreamBuffer,
                              int *pBitCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeLoudnessInfoSet(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                   IISDRCGAINENC_SYNTAXMODE syntaxModeFlag,
                                   unsigned char *pBitstreamBuffer,
                                   int *pBitCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_replaceLoudnessInfoSet(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                     LoudnessInfoSet *pLoudnessInfoSet);

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrc(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                          const float *const *drcGainInputBuffer,
                          const int bIsPrerollFrame,
                          const int bLoudnessInfoSetPresent,
                          const int bUniDrcConfigPresent,
                          unsigned char *pBitstreamBuffer,
                          int *pBitCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_getDrcInstructionsUniDrcMaxCount(int *drcInstructionsUniDrcMaxCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_getGainSequenceMaxCount(int *gainSequenceMaxCount);

IISDRCGAINENC_RETURN
iisDRCGainEnc_getDrcGainEncoderDelay(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                                     int *drcGainEncoderDelay);

#if defined __cplusplus
}
#endif
#endif
