
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

#include "iisxHEAACEncLib_extentsionData.h"
#include "extpayload.h"

XHEAACENCLIB_RETURN iisxHEAACEncLib_extentsionData_usacSetUse(
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXT_ELE_TYPE const extensionDataType,
    unsigned int const maxSpreadingFrameLength,
    unsigned int const defaultLength,
    unsigned char const* const config,
    unsigned int const configLength,
    int const signalElemBelong) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int i = 0;
  EXTENSION_PAYLOAD_TYPES type = EXT_UNKNOWN;
  unsigned int nElem = extEleList->numExtEle;

  if (extEleList == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (nElem >= USAC_MAX_EXTENSION_CONFIGS) {
    retValue = XHEAACENCLIB_RETURN_ERROR_TOO_MANY_EXTENSION_CONFIGS;
  } else if (config == NULL && configLength > 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (maxSpreadingFrameLength < 1) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SPREADING;
    }
  }

  if (!isError(retValue)) {
    for (i = 0; i < nElem; ++i) {
      if ((extEleList->extEle[i].dataType == extensionDataType) &&
          (extEleList->extEle[i].signalElemBelong == signalElemBelong)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_EXTENSION_CONFIG_EXISTS;
      }
    }
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;

    errorInfo = openExtensionPayloadContainer(&(extEleList->extEle[nElem].container), PAY_MUX_3DA_EXTENSION, signalElemBelong);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
    }
  }

  if (!isError(retValue)) {
    extEleList->extEle[nElem].dataType = extensionDataType;
    extEleList->extEle[nElem].signalElemBelong = signalElemBelong;

    extEleList->extEle[nElem].spreadLength = maxSpreadingFrameLength;
    if (maxSpreadingFrameLength > 1) {
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = addExtensionPayloadContainerFeature(extEleList->extEle[nElem].container, FEATURE_USAC_EXT_PAYLOAD_SUPPORT_FRAGMENTATION);
      if (errorInfo != noError)
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
    }
  }

  if (!isError(retValue)) {
    extEleList->extEle[nElem].defaultLength = defaultLength;
    if (defaultLength > 0) {
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = addExtensionPayloadContainerFeature(extEleList->extEle[nElem].container, FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH);
      if (errorInfo == noError) {
        errorInfo = setExtensionPayloadContainerDefaultLength(extEleList->extEle[nElem].container, defaultLength);
      }
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
      }
    }
  }

  if (!isError(retValue)) {
    assert(configLength <= sizeof(extEleList->extEle[nElem].config));
    extEleList->extEle[nElem].configLength = configLength;

    if (configLength > 0) {
      memcpy(extEleList->extEle[nElem].config, config, configLength);
    }

    switch (extensionDataType) {
      case XHEAACENCLIB_EXT_ELE_FILL:
      case XHEAACENCLIB_EXT_ELE_MPEGS:
      case XHEAACENCLIB_EXT_ELE_SAOC:
      case XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL:
        type = EXT_3DA_EXTENSION_DATA;
        break;
      case XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC:
        type = EXT_USAC_UNI_DRC;
        break;
      default:
        assert(0);
        break;
    }

    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = addExtensionPayload(extEleList->extEle[nElem].container, type, 0, 0,
                                    NO_NIBBLE, NULL, 0, 0);
    if (errorInfo != noError)
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
  }

  if (!isError(retValue)) {
    extEleList->numExtEle++;
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_extentsionData_usacSubmit(
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXT_ELE_TYPE const extensionDataType,
    unsigned char* const extensionBuffer,
    unsigned int const extensionLength,
    int const signalElemBelong,
    int const bIsOutOfBitRes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  EXTENSION_PAYLOAD_TYPES extType = EXT_UNKNOWN;
  XHEAACENCLIB_EXT_ELEMENT_HANDLE extElement = NULL;
  char* extBufToWrite = NULL;
  unsigned int i = 0;
  unsigned int extLengthBytes = 0;

  unsigned char* dataPtr = NULL;
  int objBytesLeft = 0;
  int spreadBytesPerFrame = 0;
  unsigned int spreadingFrameLen = 0;

  if (extEleList == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (extensionBuffer == NULL && extensionLength != 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    unsigned int nElem = extEleList->numExtEle;

    for (i = 0; i < nElem; ++i) {
      if ((extEleList->extEle[i].dataType == extensionDataType) &&
          (extEleList->extEle[i].signalElemBelong == signalElemBelong)) {
        extElement = &(extEleList->extEle[i]);
      }
    }

    if (NULL == extElement)
      retValue = XHEAACENCLIB_RETURN_ERROR_NO_EXT_ELEMENT;
  }

  if (!isError(retValue)) {
    if (0 != getTotalnumber_extPayload(extElement->container)) {
      if (extensionLength)
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
      else
        return retValue;
    }
  }

  if (!isError(retValue)) {
    switch (extensionDataType) {
      case XHEAACENCLIB_EXT_ELE_FILL:
      case XHEAACENCLIB_EXT_ELE_MPEGS:
      case XHEAACENCLIB_EXT_ELE_SAOC:

        extType = EXT_3DA_EXTENSION_DATA;
        break;
      case XHEAACENCLIB_EXT_ELE_USAC_UNI_DRC:
        extType = EXT_USAC_UNI_DRC;
        break;

      case XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL:
        extType = EXT_3DA_EXTENSION_DATA;

        extElement->delay = 0;
        extElement->writePoint = 0;
        extElement->readPoint = 0;

        if (bIsOutOfBitRes) {
          addExtensionPayloadContainerFeature(extElement->container, FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES);
        } else {
          removeExtensionPayloadContainerFeature(extElement->container, FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES);
        }
        break;
      default:
        assert(0);
        break;
    }
  }

  if (!isError(retValue)) {
    dataPtr = extensionBuffer;
    extLengthBytes = (extensionLength + 7) / 8;
    objBytesLeft = extLengthBytes;

    spreadingFrameLen = extElement->spreadLength;

    if (extLengthBytes < spreadingFrameLen)
      spreadingFrameLen = extLengthBytes;

    if (extLengthBytes == 0)
      spreadingFrameLen = 1;

    if ((extElement->spreadFrameCounter > 0) && (extLengthBytes > 0)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
    }
  }

  if (!isError(retValue)) {
    spreadBytesPerFrame = extLengthBytes / spreadingFrameLen;

    if (extLengthBytes % spreadingFrameLen)
      spreadBytesPerFrame++;

    if ((extElement->spreadFrameCounter > 0) && (extLengthBytes == 0))
      spreadingFrameLen = 0;

    for (i = 0; i < spreadingFrameLen; ++i) {
      if (objBytesLeft <= spreadBytesPerFrame) {
        spreadBytesPerFrame = objBytesLeft;

        i = spreadingFrameLen - 1;
      }

      assert(spreadBytesPerFrame < MAX_EXTENSION_PAYLOAD_SIZE);

      if (spreadBytesPerFrame > 0) {
        memcpy(extElement->delayBuffer[extElement->writePoint], dataPtr, spreadBytesPerFrame);
      }

      extElement->size[extElement->writePoint] = spreadBytesPerFrame;

      if ((i == 0) && (spreadingFrameLen == 1)) {
        extElement->type[extElement->writePoint] = EXT_DATA_START_STOP;
      } else if ((i == 0) && (spreadingFrameLen > 1)) {
        extElement->type[extElement->writePoint] = EXT_DATA_START;
      } else if ((i > 0) && (i == (spreadingFrameLen - 1))) {
        extElement->type[extElement->writePoint] = EXT_DATA_STOP;
      } else {
        extElement->type[extElement->writePoint] = EXT_DATA_CONTINUE;
      }

      dataPtr = dataPtr ? dataPtr + spreadBytesPerFrame : NULL;
      objBytesLeft -= spreadBytesPerFrame;

      extElement->writePoint++;
      extElement->writePoint = extElement->writePoint % EXT_DATA_DELAY_BUFFER_LENGTH;

      extElement->spreadFrameCounter++;
    }

    extBufToWrite = extElement->delayBuffer[extElement->readPoint];
    extLengthBytes = extElement->size[extElement->readPoint];

    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = addExtensionPayload(extElement->container,
                                    extType,
                                    extLengthBytes * 8,
                                    0,
                                    NO_NIBBLE,
                                    (unsigned char*)extBufToWrite,
                                    0,
                                    0);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
    }
  }
  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = writeExtensionPayloadFragmentationType(extElement->container, 0,
                                                       extElement->type[extElement->readPoint]);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
    }
  }

  if (!isError(retValue)) {
    extElement->readPoint++;
    extElement->readPoint = extElement->readPoint % EXT_DATA_DELAY_BUFFER_LENGTH;

    extElement->spreadFrameCounter--;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_extensionData_generalSetUse(
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_EXT_ELE_TYPE const extensionDataType,
    unsigned int const defaultLength,
    int const signalElemBelong) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int i = 0;

  if (extEleList == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    unsigned int nElem = extEleList->numExtEle;

    if (nElem >= USAC_MAX_EXTENSION_CONFIGS) {
      retValue = XHEAACENCLIB_RETURN_ERROR_TOO_MANY_EXTENSION_CONFIGS;
    }

    if (!isError(retValue)) {
      for (i = 0; i < nElem; ++i) {
        if ((extEleList->extEle[i].dataType == extensionDataType) &&
            (extEleList->extEle[i].signalElemBelong == signalElemBelong)) {
          retValue = XHEAACENCLIB_RETURN_ERROR_EXTENSION_CONFIG_EXISTS;
        }
      }
    }

    if (!isError(retValue)) {
      EXTENSION_PAYLOAD_VERSION payload_ver = 0;
      payload_ver = (hConfig->aot == AUD_OBJ_TYP_USAC) ? PAY_MUX_3DA_EXTENSION : PAY_MUX_V1;
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = openExtensionPayloadContainer(&(extEleList->extEle[nElem].container), payload_ver, signalElemBelong);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
      }
    }

    if (!isError(retValue)) {
      extEleList->extEle[nElem].dataType = extensionDataType;
      extEleList->extEle[nElem].signalElemBelong = signalElemBelong;
    }

    if (!isError(retValue)) {
      extEleList->extEle[nElem].defaultLength = defaultLength;
      if (defaultLength > 0) {
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = addExtensionPayloadContainerFeature(extEleList->extEle[nElem].container, FEATURE_USAC_EXT_PAYLOAD_SUPPORT_DEFAULT_LENGTH);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_CONTAINER;
        }
      }
    }

    if (!isError(retValue)) {
      extEleList->numExtEle++;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_extensionData_generalSubmit(
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXT_ELE_TYPE const extensionDataType,
    unsigned char* const extensionBuffer,
    unsigned int const extensionLength,
    int const signalElemBelong) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  EXTENSION_PAYLOAD_TYPES extType = EXT_UNKNOWN;
  XHEAACENCLIB_EXT_ELEMENT_HANDLE extElement = NULL;
  char* extBufToWrite = NULL;
  unsigned int extBitsToWriteSize = 0;
  unsigned int i = 0;
  unsigned int extLengthBytes = 0;
  unsigned char* dataPtr = NULL;
  unsigned int nElem = extEleList->numExtEle;

  if (extEleList == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (extensionBuffer == NULL && extensionLength != 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (i = 0; i < nElem; ++i) {
      if ((extEleList->extEle[i].dataType == extensionDataType) &&
          (extEleList->extEle[i].signalElemBelong == signalElemBelong))
        extElement = &(extEleList->extEle[i]);
    }

    if (NULL == extElement)
      retValue = XHEAACENCLIB_RETURN_ERROR_NO_EXT_ELEMENT;
  }

  if (!isError(retValue)) {
    if (0 != getTotalnumber_extPayload(extElement->container)) {
      if (extensionLength)
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
      else
        return retValue;
    }
  }

  if (!isError(retValue)) {
    switch (extensionDataType) {
      case XHEAACENCLIB_EXT_ELE_AAC_UNI_DRC:
        extType = EXT_UNI_DRC;
        break;

      default:
        assert(0);
        break;
    }
  }

  if (!isError(retValue)) {
    dataPtr = extensionBuffer;
    extLengthBytes = (extensionLength + 7) / 8;

    assert(extLengthBytes < MAX_EXTENSION_PAYLOAD_SIZE);
    memcpy(extElement->delayBuffer[extElement->writePoint], dataPtr, extLengthBytes);

    extBufToWrite = extElement->delayBuffer[extElement->readPoint];

    if (extensionLength % 8 == 4) {
      extBitsToWriteSize = extensionLength;
    } else if (extensionLength % 8 == 0) {
      extBitsToWriteSize = extensionLength + 4;
    } else {
      if ((extensionLength % 8 < 4) && (extensionLength % 8 != 0)) {
        extBitsToWriteSize = (((extensionLength + 7) / 8) * 8) - 4;
      } else if (extensionLength % 8 > 4) {
        extBitsToWriteSize = (((extensionLength + 7) / 8) * 8) + 4;
      }
    }

    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = addExtensionPayload(extElement->container,
                                    extType,
                                    extBitsToWriteSize,
                                    0,
                                    NO_NIBBLE,
                                    (unsigned char*)extBufToWrite,
                                    0,
                                    0);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_extensionData_SetUseConfigExtension(
    XHEAACENCLIB_CONFIG_EXTENSION_LIST* const configExtensionList,
    XHEAACENCLIB_CONFIG_EXTENSION_TYPE const extensionDataType,
    unsigned char const* const config,
    unsigned int const configLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int extIndex = -1;
  unsigned int i = 0;

  if (configExtensionList == NULL || config == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (i = 0; i < configExtensionList->numConfigExtension; i++) {
      if (configExtensionList->configExtension[i].dataType == extensionDataType) {
        extIndex = (int)i;
        break;
      }
    }

    if (extIndex >= 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXTENSION_CONFIG_EXISTS;
    } else {
      extIndex = (int)configExtensionList->numConfigExtension;
    }
  }
  if (!isError(retValue) && extIndex >= USAC_MAX_CONFIG_EXTENSION) {
    retValue = XHEAACENCLIB_RETURN_ERROR_TOO_MANY_EXTENSION_CONFIGS;
  }

  if (!isError(retValue)) {
    memcpy(&(configExtensionList->configExtension[i].config), config, configLength);
    configExtensionList->configExtension[i].configLength = configLength;
    configExtensionList->configExtension[i].dataType = extensionDataType;

    configExtensionList->numConfigExtension++;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_extensionData_GetExtensionLength(
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList,
    XHEAACENCLIB_EXT_ELE_TYPE const extensionDataType,
    int const signalElemBelong,
    unsigned int* const extensionLength) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_EXT_ELEMENT_HANDLE extElement = NULL;
  unsigned int nElem = extEleList->numExtEle;
  unsigned int i = 0;

  if (extEleList == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (i = 0; i < nElem; ++i) {
      if ((extEleList->extEle[i].dataType == extensionDataType) &&
          (extEleList->extEle[i].signalElemBelong == signalElemBelong))
        extElement = &(extEleList->extEle[i]);
    }

    if (NULL == extElement)
      retValue = XHEAACENCLIB_RETURN_ERROR_NO_EXT_ELEMENT;
  }

  if (!isError(retValue)) {
    if (extensionLength != NULL) {
      *extensionLength = getTotalSize_extPayload(extElement->container);
    }
  }
  return retValue;
}
