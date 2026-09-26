
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

#include <string.h>

#include "glob_con.h"
#include "extpayload.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "bit_buf.h"
typedef enum {
  NO_ER_ELEMENT = 0,
  DATA_ELEMENT_LENGHT_PART,
  EXTENSION_TYPE
} ER_BITSTREAM_ELEMENT;

static const EXTENSION_PAYLOAD_CONTENT PAYLOAD_ORDER_USAC[] = {PAY_SBR, PAY_SAC};
static const EXTENSION_PAYLOAD_CONTENT PAYLOAD_ORDER_UNKNOWN[] = {PAY_UNKNOWN};

static const EXTENSION_PAYLOAD_CONTENT PAYLOAD_ORDER_3DA_EXTENSION[] = {PAY_3DA_EXT_ELEMENT};

typedef enum {
  PAY_WRITER_BITBUF,
  PAY_WRITER_BITMUX
} EXTENSION_PAYLOAD_WRITER;

typedef struct {
  EXTENSION_PAYLOAD_TYPES type;
  unsigned int length;
  unsigned int writeLengthInfo;
  unsigned char nibbleData;
  unsigned char data[MAX_EXTENSION_PAYLOAD_SIZE];
  unsigned int element_index;
  unsigned int m_totalSize;
  unsigned int payload_number;
  EXTENSION_PAYLOAD_CONTENT m_content;

  unsigned int m_payloadFragmentationType;
} EXTPAYLOAD_FIELD, *HANDLE_EXTPAYLOAD_FIELD;

struct EXTPAYLOAD_CONTAINER {
  EXTENSION_PAYLOAD_VERSION version;
  unsigned int intializedFields;
  unsigned int nPayloads;
  HANDLE_EXTPAYLOAD_FIELD payload[MAX_EXTENSION_PAYLOADS];

  unsigned int features;
  int channelOffset;
  int payloadDefaultLength;
};

static const EXTENSION_PAYLOAD_CONTENT *__getContentOrder(EXTENSION_PAYLOAD_VERSION version,
                                                          unsigned int *nEntries) {
  switch (version) {
    case PAY_MUX_USAC:
      *nEntries = sizeof(PAYLOAD_ORDER_USAC) / sizeof(EXTENSION_PAYLOAD_CONTENT);
      return PAYLOAD_ORDER_USAC;

    case PAY_MUX_3DA_EXTENSION:
      *nEntries = sizeof(PAYLOAD_ORDER_3DA_EXTENSION) / sizeof(EXTENSION_PAYLOAD_CONTENT);
      return PAYLOAD_ORDER_3DA_EXTENSION;
    default:
      *nEntries = 0;
      return PAYLOAD_ORDER_UNKNOWN;
  }
}

static EXTENSION_PAYLOAD_CONTENT __getContent(HANDLE_EXTPAYLOAD_FIELD hField) {
  EXTENSION_PAYLOAD_CONTENT extPayloadContent;

  switch (hField->type) {
    case EXT_SBR_DATA:
    case EXT_SBR_DATA_CRC:
    case EXT_LDSBR_DATA:
    case EXT_USACSBR_DATA:
      extPayloadContent = PAY_SBR;
      break;

    case EXT_FILL:
    case EXT_FILL_DATA:
      extPayloadContent = PAY_FILL;
      break;

    case EXT_DATA_ELEMENT:
      switch (hField->nibbleData) {
        case 0x00:
          extPayloadContent = PAY_ANC;
          break;
        default:
          extPayloadContent = PAY_UNKNOWN;
          break;
      }
      break;

    case EXT_SAOC_DATA:
      extPayloadContent = PAY_SAOC;
      break;

    case EXT_DYNAMIC_RANGE:
      extPayloadContent = PAY_DRC;
      break;

    case EXT_LDSAC_DATA:
    case EXT_SAC_DATA:
    case EXT_USACMPS212_DATA:
      extPayloadContent = PAY_SAC;
      break;

    case EXT_SAC_DRM_DATA:
      extPayloadContent = PAY_SAC;
      break;

    case EXT_3DA_EXTENSION_DATA:
      extPayloadContent = PAY_3DA_EXT_ELEMENT;
      break;

    case EXT_UNI_DRC:
      extPayloadContent = PAY_FILL;
      break;

    case EXT_USAC_UNI_DRC:
      extPayloadContent = PAY_3DA_EXT_ELEMENT;
      break;

    default:
      extPayloadContent = PAY_UNKNOWN;
      break;
  }

  return extPayloadContent;
}

static unsigned int __getTagSize(EXTENSION_PAYLOAD_TYPES type) {
  if (type < EXT_NO_TAG_USED)
    return 4;
  else
    return 0;
}

static unsigned int __getSizeOfField(HANDLE_EXTPAYLOAD_FIELD field, EXTENSION_PAYLOAD_VERSION version, unsigned int features, int payloadDefaultLength) {
  unsigned int size = 0;

  (void)payloadDefaultLength;

  size += field->length;

  if (version == PAY_MUX_3DA_EXTENSION)
    size += 0;
  else
    size += __getTagSize(field->type);

  if (field->nibbleData != NO_NIBBLE)
    size += 4;

  if (version == PAY_MUX_V1) {
    if ((size + 7) / 8 > 14) {
      size = size + 12;
    } else {
      size = size + 4;
    }
    size = size + 3;
  }

  if (field->writeLengthInfo) {
    unsigned int len = field->length;
    len += 8;
    size += 8;
    if (len >= 15 * 8) {
      size += 8;
      if (len >= (255 * 8 + 15 * 8)) {
        size += 16;
      }
    }
  }

  if (field->m_content == PAY_ANC) {
    int len = field->length;
    do {
      size += 8;
      len -= 255 * 8;
    } while (len > 0);
  }
  if (field->m_content == PAY_3DA_EXT_ELEMENT) {
    int len = field->length >> 3;
    int useDefaultLength = 0;

    size++;

    if ((len == 0) && !(features & FEATURE_USAC_EXT_PAYLOAD_WRITE_ZERO_LENGTH_ELEMENT)) {
    } else {
      if (features & FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH) {
        useDefaultLength = (len == payloadDefaultLength);
      }

      size++;

      if (useDefaultLength == 0) {
        size += 8;

        if (len >= 255)
          size += 16;
      }

      if ((len > 0) && (features & FEATURE_USAC_EXT_PAYLOAD_SUPPORT_FRAGMENTATION))
        size += 2;
    }
  }

  return (size);
}

static HANDLE_EXTPAYLOAD_FIELD __getNextPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                                                EXTENSION_PAYLOAD_CONTENT content,
                                                unsigned int element_index,
                                                unsigned int noPayload) {
  unsigned int i;
  unsigned int nextIndex = MAX_EXTENSION_PAYLOADS;

  if ((hExtContainer->version == PAY_MUX_3DA_EXTENSION) && (hExtContainer->intializedFields == 1))
    return (hExtContainer->payload[0]);

  for (i = 0; i < hExtContainer->nPayloads; i++) {
    if (hExtContainer->payload[i]->m_content == content) {
      if (element_index == hExtContainer->payload[i]->element_index) {
        if (noPayload == hExtContainer->payload[i]->payload_number) {
          nextIndex = i;
          break;
        }
      }
    }
  }

  if (nextIndex == MAX_EXTENSION_PAYLOADS)
    return NULL;
  else
    return (hExtContainer->payload[nextIndex]);
}

static int __getMaxIndex(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                         EXTENSION_PAYLOAD_CONTENT content) {
  unsigned int i;
  unsigned int minEl = 0;

  for (i = 0; i < hExtContainer->nPayloads; i++) {
    if (hExtContainer->payload[i]->m_content == content)
      if (hExtContainer->payload[i]->element_index >= minEl) {
        minEl = hExtContainer->payload[i]->element_index + 1;
      }
  }
  return (minEl);
}

void resetExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer) {
  unsigned int i;
  hExtContainer->nPayloads = 0;
  for (i = 0; i < hExtContainer->intializedFields; i++) {
    hExtContainer->payload[i]->element_index = 0;
    hExtContainer->payload[i]->m_content = (hExtContainer->version == PAY_MUX_3DA_EXTENSION) ? PAY_3DA_EXT_ELEMENT : PAY_NONE;
    hExtContainer->payload[i]->m_payloadFragmentationType = 0;
    hExtContainer->payload[i]->length = 0;
  }
}

HANDLE_ERROR_INFO
openExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
                              EXTENSION_PAYLOAD_VERSION version,
                              int channelOffset) {
  HANDLE_EXTPAYLOAD_CONTAINER hExt;
  HANDLE_ERROR_INFO err = noError;
  hExt = (HANDLE_EXTPAYLOAD_CONTAINER)iisCalloc(1, sizeof(struct EXTPAYLOAD_CONTAINER));
  if (hExt == NULL) {
    err = iisUtil_ERROR(CDI, "memory allocation error");
    return err;
  }

  resetExtensionPayloadContainer(hExt);
  hExt->version = version;
  hExt->intializedFields = 0;
  hExt->channelOffset = channelOffset;
  hExt->payloadDefaultLength = 0;
  *hExtContainer = hExt;
  return err;
}

void closeExtensionPayloadContainer(HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer) {
  int i;
  if (*hExtContainer) {
    for (i = 0; i < MAX_EXTENSION_PAYLOADS; i++) {
      if ((*hExtContainer)->payload[i] != NULL) {
        iisFree((*hExtContainer)->payload[i]);
      }
    }
    iisFree(*hExtContainer);
    *hExtContainer = NULL;
  }
}

HANDLE_ERROR_INFO
addExtensionPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                    EXTENSION_PAYLOAD_TYPES type,
                    unsigned int length,
                    unsigned int writeLengthInfo,
                    unsigned char nibbleData,
                    unsigned char const *const data,
                    unsigned int element_index,
                    unsigned int payload_number) {
  HANDLE_ERROR_INFO err = noError;
  unsigned int index = hExtContainer->nPayloads;
  int unalignedBits;

  if (hExtContainer->nPayloads > MAX_EXTENSION_PAYLOADS) {
    err = iisUtil_ERROR(CDI, "MAX_EXTENSION_PAYLOADS exceeded");
    return err;
  }
  if (data == NULL && length > 0) {
    err = iisUtil_ERROR(CDI, "data pointer is NULL but length is > 0");
    return err;
  }

  if (length > MAX_EXTENSION_PAYLOAD_SIZE * 8) {
    err = iisUtil_ERROR(CDI, "MAX_EXTENSION_PAYLOAD_SIZE exceeded");
    return err;
  }
  if (hExtContainer->intializedFields < (index + 1)) {
    if (hExtContainer->payload[index] == NULL) {
      hExtContainer->payload[index] = (HANDLE_EXTPAYLOAD_FIELD)iisCalloc(1, sizeof(EXTPAYLOAD_FIELD));
      if (hExtContainer->payload[index] == NULL) {
        err = iisUtil_ERROR(CDI, "memory allocation error");
        return err;
      }
      hExtContainer->intializedFields++;
    } else {
      err = iisUtil_ERROR(CDI, "memory missmatch");
      return err;
    }
  }

  hExtContainer->payload[index]->type = type;
  hExtContainer->payload[index]->length = length;
  hExtContainer->payload[index]->writeLengthInfo = writeLengthInfo;
  hExtContainer->payload[index]->nibbleData = nibbleData;
  hExtContainer->payload[index]->payload_number = payload_number;

  if (length > 0) {
    memcpy(hExtContainer->payload[index]->data, data, sizeof(unsigned char) * (length >> 3));
  }
  unalignedBits = length % 8;
  if (unalignedBits)
    hExtContainer->payload[index]->data[length >> 3] = data[length >> 3] >> (8 - unalignedBits);

  hExtContainer->payload[index]->m_content = __getContent(hExtContainer->payload[index]);
  hExtContainer->payload[index]->m_totalSize = __getSizeOfField(hExtContainer->payload[index], hExtContainer->version, hExtContainer->features, hExtContainer->payloadDefaultLength);

  if (element_index != AUTO_INDEX)
    hExtContainer->payload[index]->element_index = element_index;
  else
    hExtContainer->payload[index]->element_index = __getMaxIndex(hExtContainer, hExtContainer->payload[index]->m_content);

  hExtContainer->nPayloads++;
  return (err);
}

HANDLE_ERROR_INFO
addExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature) {
  hExtContainer->features |= feature;

  return noError;
}

HANDLE_ERROR_INFO
removeExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature) {
  hExtContainer->features &= ~(feature);

  return noError;
}

HANDLE_ERROR_INFO
writeExtensionPayloadFragmentationType(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int index,
                                       unsigned int type) {
  hExtContainer->payload[index]->m_payloadFragmentationType = type;

  return noError;
}

HANDLE_ERROR_INFO
setExtensionPayloadContainerDefaultLength(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, int payloadDefaultLength) {
  hExtContainer->payloadDefaultLength = payloadDefaultLength;

  return noError;
}

unsigned int hasExtensionPayloadContainerFeature(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, EXTENSION_CONTAINER_FEATURES feature) {
  return (hExtContainer->features & feature) > 0 ? 1 : 0;
}

unsigned int getTotalSize_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer) {
  unsigned int sum = 0;
  unsigned int i;

  if ((hExtContainer->version == PAY_MUX_3DA_EXTENSION) && (hExtContainer->intializedFields == 1))
    return hExtContainer->payload[0]->m_totalSize;

  for (i = 0; i < hExtContainer->nPayloads; i++)
    sum += hExtContainer->payload[i]->m_totalSize;

  return (sum);
}

EXTENSION_PAYLOAD_TYPES getExtPayloadType(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer, unsigned int index) {
  if (index <= hExtContainer->nPayloads)
    return hExtContainer->payload[index]->type;

  return EXT_UNKNOWN;
}

int getChannelOffset(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer) {
  return hExtContainer->channelOffset;
}

unsigned int getTotalnumber_extPayload(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer) {
  return (hExtContainer->nPayloads);
}

static unsigned int __getNumPayloads(HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                                     EXTENSION_PAYLOAD_CONTENT content,
                                     unsigned int Element_index) {
  unsigned int i, n = 0;

  if ((hExtContainer->version == PAY_MUX_3DA_EXTENSION) && (hExtContainer->intializedFields == 1))
    return 1;

  for (i = 0; i < hExtContainer->nPayloads; i++) {
    if (hExtContainer->payload[i]->m_content == content && hExtContainer->payload[i]->element_index == Element_index) {
      n++;
    }
  }
  return (n);
}

static ER_BITSTREAM_ELEMENT __getDataESC(HANDLE_EXTPAYLOAD_FIELD field,
                                         HANDLE_ERROR_INFO *err) {
  (void)field;
  *err = noError;

  return NO_ER_ELEMENT;
}

static ER_BITSTREAM_ELEMENT __getNibbleESC(HANDLE_EXTPAYLOAD_FIELD field) {
  (void)field;

  return NO_ER_ELEMENT;
}

static void __internalWriteBits(EXTENSION_PAYLOAD_WRITER mode,
                                void *hBitStream,
                                unsigned char data,
                                unsigned int nBits,
                                ER_BITSTREAM_ELEMENT erElement) {
  (void)erElement;

  if (mode == PAY_WRITER_BITBUF) {
    WriteBits(hBitStream, data, nBits);
  }
}

static unsigned int __writeExtensionPayloadField(EXTENSION_PAYLOAD_WRITER mode,
                                                 void *hBitStream,
                                                 HANDLE_EXTPAYLOAD_FIELD field,
                                                 HANDLE_ERROR_INFO *err,
                                                 EXTENSION_PAYLOAD_VERSION version,
                                                 unsigned int features,
                                                 int payloadDefaultLength) {
  unsigned int bitsWritten = 0, unalignedBits;
  unsigned int n;

  (void)payloadDefaultLength;

  if (field == NULL) {
    *err = iisUtil_ERROR(CDI, "extension payload field not found");
    return 0;
  }

  if (field->writeLengthInfo) {
    unsigned int realLength = (field->length >> 3) + 1;
    unsigned int len = realLength;
    __internalWriteBits(mode, hBitStream, EXT_DATA_LENGTH, 4, EXTENSION_TYPE);
    len = (len >= 15) ? 15 : len;
    __internalWriteBits(mode, hBitStream, len, 4, EXTENSION_TYPE);
    bitsWritten += 8;
    if (realLength >= 15) {
      len = realLength - 15;
      len = (len > 255) ? 255 : len;
      __internalWriteBits(mode, hBitStream, len, 8, EXTENSION_TYPE);
      bitsWritten += 8;
      if (realLength >= (15 + 255)) {
        len = realLength - 15 - 255;
        __internalWriteBits(mode, hBitStream, len, 16, EXTENSION_TYPE);
        bitsWritten += 16;
      }
    }
  }

  if (version == PAY_MUX_V1) {
    int tmpBytes = (field->length + 4 + 7) / 8;
    __internalWriteBits(mode, hBitStream, ID_FIL, 3, __getDataESC(field, err));
    bitsWritten += 3;
    if (tmpBytes > 14) {
      __internalWriteBits(mode, hBitStream, 15, 4, __getDataESC(field, err));
      __internalWriteBits(mode, hBitStream, tmpBytes - 14, 8, __getDataESC(field, err));
      bitsWritten += 12;

    } else {
      __internalWriteBits(mode, hBitStream, tmpBytes, 4, __getDataESC(field, err));
      bitsWritten += 4;
    }
  }

  if (version == PAY_MUX_3DA_EXTENSION)
    bitsWritten += 0;
  else {
    if (field->type < EXT_NO_TAG_USED) {
      __internalWriteBits(mode, hBitStream, field->type, 4, EXTENSION_TYPE);
      bitsWritten += 4;
    }
  }

  if (field->nibbleData != NO_NIBBLE) {
    __internalWriteBits(mode, hBitStream, field->nibbleData, 4, __getNibbleESC(field));
    bitsWritten += 4;
  }

  if (field->m_content == PAY_ANC) {
    int len = field->length >> 3;
    do {
      __internalWriteBits(mode, hBitStream, ((len > 255) ? 255 : len), 8, DATA_ELEMENT_LENGHT_PART);
      bitsWritten += 8;
      len -= 255;
    } while (len > 0);
  }

  if (field->m_content == PAY_3DA_EXT_ELEMENT) {
    int len = field->length >> 3;
    int useDefaultLength = 0;

    assert((field->length % 8) == 0);

    if (len == 0 && !(features & FEATURE_USAC_EXT_PAYLOAD_WRITE_ZERO_LENGTH_ELEMENT)) {
      __internalWriteBits(mode, hBitStream, 0, 1, EXTENSION_TYPE);
      bitsWritten++;

    } else {
      __internalWriteBits(mode, hBitStream, 1, 1, EXTENSION_TYPE);
      bitsWritten++;

      if (features & FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH) {
        useDefaultLength = (len == payloadDefaultLength);
      }

      __internalWriteBits(mode, hBitStream, useDefaultLength, 1, EXTENSION_TYPE);
      bitsWritten++;

      if (useDefaultLength == 0) {
        if (len >= 255) {
          int valueAdd = len - 255 + 2;
          __internalWriteBits(mode, hBitStream, 255, 8, EXTENSION_TYPE);

          __internalWriteBits(mode, hBitStream, (unsigned char)(valueAdd >> 8), 8, EXTENSION_TYPE);
          __internalWriteBits(mode, hBitStream, (unsigned char)valueAdd, 8, EXTENSION_TYPE);

          bitsWritten += 24;
        } else {
          __internalWriteBits(mode, hBitStream, len, 8, EXTENSION_TYPE);

          bitsWritten += 8;
        }
      }

      if ((len > 0) && (features & FEATURE_USAC_EXT_PAYLOAD_SUPPORT_FRAGMENTATION)) {
        __internalWriteBits(mode, hBitStream, field->m_payloadFragmentationType, 2, EXTENSION_TYPE);
        bitsWritten += 2;
      }
    }
  }

  for (n = 0; n < (field->length >> 3); n++) {
    __internalWriteBits(mode, hBitStream, field->data[n], 8, __getDataESC(field, err));
  }
  unalignedBits = field->length % 8;
  if (unalignedBits)
    __internalWriteBits(mode, hBitStream, field->data[n], unalignedBits, __getDataESC(field, err));

  bitsWritten += field->length;

  return (bitsWritten);
}

HANDLE_ERROR_INFO
writeExtensionPayloadElement(HANDLE_BIT_BUF hBitStream,
                             HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
                             unsigned int element_index,
                             unsigned int *nBitsWritten) {
  EXTENSION_PAYLOAD_WRITER module = PAY_WRITER_BITBUF;
  unsigned int bitsWritten = 0;
  unsigned int ci, nContents;
  const EXTENSION_PAYLOAD_CONTENT *payOrder = __getContentOrder(hExtContainer->version, &nContents);
  HANDLE_EXTPAYLOAD_FIELD currentField;
  HANDLE_ERROR_INFO err = noError;

  for (ci = 0; ci < nContents; ci++) {
    unsigned int nPayloads = __getNumPayloads(hExtContainer, payOrder[ci], element_index);
    unsigned int np;
    for (np = 0; np < nPayloads; np++) {
      currentField = __getNextPayload(hExtContainer, payOrder[ci], element_index, np);
      assert(currentField);
      bitsWritten += __writeExtensionPayloadField(module, hBitStream, currentField, &err, hExtContainer->version, hExtContainer->features, hExtContainer->payloadDefaultLength);
    }
  }

  *nBitsWritten = bitsWritten;
  return (err);
}
