
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

#ifndef IISXHEAACENCLIB_ASC_IFC_H
#define IISXHEAACENCLIB_ASC_IFC_H

#include "iisutillib.h"
#include "iisxHEAACEncLibConfig.h"
#include "iisxHEAACEncLib_sbr_ifc.h"
#include "iisxHEAACEncLib_aac_ifc.h"
#include "iisxHEAACEncLib_mpegs_ifc.h"
#include "extpayload.h"

#define MAX_ASC_SIZE 64
#define MAX_DRC_CONFIG_SIZE 6144 / 8
#define MAX_EXT_DATA_DELAY_FRAMES 7
#define MAX_EXT_DATA_SPREADING 32
#define EXT_DATA_DELAY_BUFFER_LENGTH (MAX_EXT_DATA_DELAY_FRAMES + MAX_EXT_DATA_SPREADING + 1)

typedef enum {
  XHEAACENCLIB_EXT_ELE_FILL = 0,
  XHEAACENCLIB_EXT_ELE_MPEGS,
  XHEAACENCLIB_EXT_ELE_SAOC,
  XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL,
  XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC,
  XHEAACENCLIB_EXT_ELE_AAC_UNI_DRC,
} XHEAACENCLIB_EXT_ELE_TYPE;

typedef enum {
  XHEAACENCLIB_ID_CONFIG_EXT_FILL = 0,
  XHEAACENCLIB_ID_CONFIG_EXT_LOUDNESS_INFO,
  XHEAACENCLIB_ID_CONFIG_EXT_STREAM_ID,
} XHEAACENCLIB_CONFIG_EXTENSION_TYPE;

typedef enum {
  EXT_DATA_CONTINUE = 0,
  EXT_DATA_STOP = 1,
  EXT_DATA_START = 2,
  EXT_DATA_START_STOP = 3
} EXT_DATA_PAYLOAD_FRAG;

struct iisxHEAAC_asc_encoder_tag;
typedef struct iisxHEAAC_asc_encoder_tag XHEAACENCLIB_ASCENCODER, *XHEAACENCLIB_ASCENCODER_HANDLE;

typedef struct ascBuf_struct {
  int nAscSizeBits;
  unsigned char *pAsc;
} ASCBUF;

typedef struct {
  int signalElemBelong;
  XHEAACENCLIB_EXT_ELE_TYPE dataType;
  HANDLE_EXTPAYLOAD_CONTAINER container;

  char delayBuffer[EXT_DATA_DELAY_BUFFER_LENGTH][MAX_EXTENSION_PAYLOAD_SIZE];
  int size[EXT_DATA_DELAY_BUFFER_LENGTH];
  EXT_DATA_PAYLOAD_FRAG type[EXT_DATA_DELAY_BUFFER_LENGTH];

  int delay;
  int readPoint;
  int writePoint;

  int spreadLength;
  int spreadFrameCounter;
  int isFlushed;

  char config[MAX_DRC_CONFIG_SIZE];
  unsigned int configLength;
  unsigned int defaultLength;
} XHEAACENCLIB_EXT_ELEMENT, *XHEAACENCLIB_EXT_ELEMENT_HANDLE;

typedef struct {
  XHEAACENCLIB_CONFIG_EXTENSION_TYPE dataType;
  char config[MAX_DRC_CONFIG_SIZE];
  unsigned int configLength;
} XHEAACENCLIB_CONFIG_EXTENSION, *XHEAACENCLIB_CONFIG_EXTENSION_HANDLE;

XHEAACENCLIB_RETURN openAudioSpecificConfig(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                            XHEAACENCLIB_HANDLE_AACENCODER hAacEnc,
                                            XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                            XHEAACENCLIB_HANDLE_MPEGSENCODER hMpegsEnc,
                                            XHEAACENCLIB_EXT_ELEMENT *extEle,
                                            const int numExtEle,
                                            XHEAACENCLIB_CONFIG_EXTENSION *configExtension,
                                            const int numConfigExtension,
                                            ASCBUF *asc);

void closeAudioSpecificConfig(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                              ASCBUF *asc);

#endif
