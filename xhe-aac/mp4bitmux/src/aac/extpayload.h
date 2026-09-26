
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

#ifndef EXTPAYLOAD_H
#define EXTPAYLOAD_H

#define MAX_EXTENSION_PAYLOADS 48

#define MAX_EXTENSION_PAYLOAD_SIZE 8800
#include "iisutillib.h"
#include "bit_buf.h"
typedef struct outbuf *HANDLE_OUT_BUF;

#define NO_NIBBLE 0xFF
#define AUTO_INDEX 0xFF

#define SUPPORT_EXT_DATA

typedef enum {
  PAY_MUX_UNKNOWN,
  PAY_MUX_V1,
  PAY_MUX_V2,
  PAY_MUX_ELD,
  PAY_MUX_USAC,
  PAY_MUX_3DA_EXTENSION
} EXTENSION_PAYLOAD_VERSION;

typedef enum {
  PAY_NONE = 0,
  PAY_UNKNOWN,
  PAY_DRC,
  PAY_SBR,
  PAY_SAC,
  PAY_SAOC,
  PAY_ANC,
  PAY_FILL,
  PAY_3DA_EXT_ELEMENT,
  PAY_CONTENT_END
} EXTENSION_PAYLOAD_CONTENT;

typedef enum {
  FEATURE_USAC_EXT_PAYLOAD_NO_FEATURE = 0,
  FEATURE_USAC_EXT_PAYLOAD_SUPPORT_FRAGMENTATION = 1,
  FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH = 2,
  FEATURE_USAC_EXT_PAYLOAD_WRITE_ZERO_LENGTH_ELEMENT = 4,
  FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES = 8

} EXTENSION_CONTAINER_FEATURES;

typedef enum {
  EXT_UNKNOWN = -1,
  EXT_FILL = 0,
  EXT_FILL_DATA = 1,
  EXT_DATA_ELEMENT = 2,
  EXT_DATA_LENGTH = 3,
  EXT_UNI_DRC = 4,
  EXT_LDSAC_DATA = 9,
  EXT_SAOC_DATA = 10,
  EXT_DYNAMIC_RANGE = 11,
  EXT_SAC_DATA = 12,
  EXT_SBR_DATA = 13,
  EXT_SBR_DATA_CRC = 14,
  EXT_NO_TAG_USED = 100,
  EXT_LDSBR_DATA,
  EXT_USACSBR_DATA,
  EXT_SAC_DRM_DATA,
  EXT_USACMPS212_DATA,
  EXT_USAC_UNI_DRC,
  EXT_3DA_EXTENSION_DATA
} EXTENSION_PAYLOAD_TYPES;

struct EXTPAYLOAD_CONTAINER;
typedef struct EXTPAYLOAD_CONTAINER *HANDLE_EXTPAYLOAD_CONTAINER;

HANDLE_ERROR_INFO
openExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
                              EXTENSION_PAYLOAD_VERSION version, int channelOffset);

void closeExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer);

HANDLE_ERROR_INFO
addExtensionPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                    EXTENSION_PAYLOAD_TYPES type,
                    unsigned int length,
                    unsigned int writeLengthInfo,
                    unsigned char nibbleData,
                    unsigned char const *const data,
                    unsigned int element_index,
                    unsigned int element_number);

HANDLE_ERROR_INFO
removeExtensionPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer);

void resetExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer);

unsigned int getTotalSize_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer);

unsigned int getIndividualSize_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int index);

unsigned int getTotalnumber_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer);

unsigned int getTotalSize_elements(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int index, int maxElements);

HANDLE_ERROR_INFO getData_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int index, unsigned char *extPayloadData);

EXTENSION_PAYLOAD_TYPES getExtPayloadType(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, unsigned int index);

int getChannelOffset(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer);

HANDLE_ERROR_INFO
writeExtensionPayloadContainer_bitbuf(HANDLE_BIT_BUF hBitStream,
                                      HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                                      unsigned int *nBitsWritten);
HANDLE_ERROR_INFO
writeExtensionPayloadContainer_outbuf(HANDLE_OUT_BUF hOutBuf,
                                      HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                                      unsigned int *nBitsWritten);

HANDLE_ERROR_INFO
writeExtensionPayloadElement(HANDLE_BIT_BUF hBitStream,
                             HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                             unsigned int element_index,
                             unsigned int *nBitsWritten);

HANDLE_ERROR_INFO setChannelOffset(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                                   int channelOffset);

HANDLE_ERROR_INFO
addExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature);

HANDLE_ERROR_INFO
removeExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature);

unsigned int
hasExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature);

HANDLE_ERROR_INFO
writeExtensionPayloadFragmentationType(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int index,
                                       unsigned int type);

HANDLE_ERROR_INFO
setExtensionPayloadContainerDefaultLength(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int defaultLength);

#endif
