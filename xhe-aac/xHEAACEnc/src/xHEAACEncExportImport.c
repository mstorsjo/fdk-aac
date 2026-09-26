
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

#include <assert.h>
#include <string.h>
#include "xHEAACEnc.h"

#include "xHEAACEncLoudness.h"

#define CHECKSUM_BYTES (4)
#define VERSION_BYTES_MAX (256)

struct xheaacenc_databuffer_struct {
  unsigned char* dataPacked;
  unsigned int dataSizeBytes;
};

static int isError(
    IIS_XHEAACENC_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue >= IIS_XHEAACENC_ERROR_FIRST) {
    retValue = 1;
  }
  return retValue;
}

int memCopyCheck(
    unsigned int const bytesCopied,
    unsigned int const bytestoCopy,
    unsigned int const numBytes);

static unsigned int getLoudnessInstanceStructSize(void) {
  unsigned int size = 0;

  size += sizeof(int);

  size += sizeof(float);

  size += sizeof(float);

  size += sizeof(float);

  size += sizeof(int);

  size += sizeof(int);

  size += sizeof(int);

  return size;
}

static IIS_XHEAACENC_RETURN_CODE getLoudnessDataStructSize(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    unsigned int* const size) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (hLoudness == NULL || hLoudness->hLoudnessData == NULL || size == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = hLoudness->hLoudnessData;

    *size += sizeof(int);

    *size += sizeof(int);

    *size += sizeof(int);

    if (pLoudnessData->instantaneousLoudness != NULL) {
      *size += sizeof(float) * pLoudnessData->audioBlocksProcessed;
    }

    if (hLoudness->measureAnchorLoudness && pLoudnessData->vaBuffer != NULL) {
      *size += sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed;
    }
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE getLoudnessInstanceDataSize(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    unsigned int* const size) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int dataSize = 0;

  if ((hLoudness == NULL) || (size == NULL)) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    unsigned int loudnessDataSize = 0;
    returnValue = getLoudnessDataStructSize(hLoudness, &loudnessDataSize);
    dataSize += loudnessDataSize;
  }

  if (!isError(returnValue)) {
    dataSize += getLoudnessInstanceStructSize();
    *size = dataSize;
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE exportLoudnessInstance(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const pLoudness,
    unsigned char* const buffer,
    unsigned int* const offset) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (pLoudness == NULL || buffer == NULL || offset == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    memcpy(buffer + *offset, &(pLoudness->measureAnchorLoudness), sizeof(int));
    *offset += sizeof(int);
    if (pLoudness->measureAnchorLoudness) {
      memcpy(buffer + *offset, &(pLoudness->anchorLoudness), sizeof(float));
      *offset += sizeof(float);
    } else {
      memcpy(buffer + *offset, &(pLoudness->loudness), sizeof(float));
      *offset += sizeof(float);
    }
    memcpy(buffer + *offset, &(pLoudness->samplePeak), sizeof(float));
    *offset += sizeof(float);
    memcpy(buffer + *offset, &(pLoudness->loudnessRange), sizeof(float));
    *offset += sizeof(float);
    memcpy(buffer + *offset, &(pLoudness->nChannels), sizeof(int));
    *offset += sizeof(int);
    memcpy(buffer + *offset, &(pLoudness->audioBlockLength), sizeof(int));
    *offset += sizeof(int);
    memcpy(buffer + *offset, &(pLoudness->sampleRate), sizeof(int));
    *offset += sizeof(int);
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE exportLoudnessData(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const pLoudness,
    unsigned char* const buffer,
    unsigned int* const offset) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (pLoudness == NULL || pLoudness->hLoudnessData == NULL || buffer == NULL || offset == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = pLoudness->hLoudnessData;

    memcpy(buffer + *offset, &(pLoudnessData->nSamplesInLoudnessMeter), sizeof(int));
    *offset += sizeof(int);
    memcpy(buffer + *offset, &(pLoudnessData->audioBlocksProcessed), sizeof(int));
    *offset += sizeof(int);
    memcpy(buffer + *offset, &(pLoudnessData->minRequiredSamplesProcessed), sizeof(int));
    *offset += sizeof(int);
    if (pLoudnessData->instantaneousLoudness != NULL) {
      memcpy(buffer + *offset, pLoudnessData->instantaneousLoudness, sizeof(float) * pLoudnessData->audioBlocksProcessed);
      *offset += sizeof(float) * pLoudnessData->audioBlocksProcessed;
    }
    if (pLoudness->measureAnchorLoudness && pLoudnessData->vaBuffer != NULL) {
      memcpy(buffer + *offset, pLoudnessData->vaBuffer, sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed);
      *offset += sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed;
    }
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE importLoudnessInstance(
    unsigned char const* const buffer,
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const pLoudness,
    unsigned int* const offset,
    unsigned int const numBytes

) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (pLoudness == NULL || buffer == NULL || offset == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudness->measureAnchorLoudness), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(float), numBytes)) {
      if (pLoudness->measureAnchorLoudness) {
        memcpy(&(pLoudness->anchorLoudness), buffer + *offset, sizeof(float));
      } else {
        memcpy(&(pLoudness->loudness), buffer + *offset, sizeof(float));
      }
      *offset += sizeof(float);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(float), numBytes)) {
      memcpy(&(pLoudness->samplePeak), buffer + *offset, sizeof(float));
      *offset += sizeof(float);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(float), numBytes)) {
      memcpy(&(pLoudness->loudnessRange), buffer + *offset, sizeof(float));
      *offset += sizeof(float);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudness->nChannels), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudness->audioBlockLength), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudness->sampleRate), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }
    pLoudness->usesDefaultLoudnessEnvelopeLength = 0;
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE importLoudnessData(
    unsigned char const* const buffer,
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const pLoudness,
    unsigned int* const offset,
    unsigned int const numBytes) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (pLoudness == NULL || pLoudness->hLoudnessData == NULL || buffer == NULL || offset == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    IIS_XHEAACENC_LOUDNESS_DATA* pLoudnessData = pLoudness->hLoudnessData;

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudnessData->nSamplesInLoudnessMeter), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudnessData->audioBlocksProcessed), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(int), numBytes)) {
      memcpy(&(pLoudnessData->minRequiredSamplesProcessed), buffer + *offset, sizeof(int));
      *offset += sizeof(int);
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (memCopyCheck(*offset, sizeof(float) * pLoudnessData->audioBlocksProcessed, numBytes)) {
      pLoudnessData->instantaneousLoudness = (float*)iisRealloc(pLoudnessData->instantaneousLoudness, sizeof(float) * pLoudnessData->audioBlocksProcessed);
      if (pLoudnessData->instantaneousLoudness == NULL) {
        return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
      }
      memcpy(pLoudnessData->instantaneousLoudness, buffer + *offset, sizeof(float) * pLoudnessData->audioBlocksProcessed);
      *offset += sizeof(float) * pLoudnessData->audioBlocksProcessed;
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }

    if (pLoudness->measureAnchorLoudness) {
      if (memCopyCheck(*offset, sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed, numBytes)) {
        pLoudnessData->vaBuffer = (unsigned char*)iisRealloc(pLoudnessData->vaBuffer, sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed);
        if (pLoudnessData->vaBuffer == NULL) {
          return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
        }
        memcpy(pLoudnessData->vaBuffer, buffer + *offset, sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed);
        *offset += sizeof(unsigned char) * pLoudnessData->audioBlocksProcessed;
      } else {
        return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
      }
    }

    if (pLoudnessData->audioBlocksProcessed > 0) {
      pLoudnessData->instantaneousLoudnessLength = pLoudnessData->audioBlocksProcessed;
      if (pLoudnessData->nSamplesInLoudnessMeter > 0) {
        pLoudnessData->instantaneousLoudnessLength++;
      }
    } else {
      return IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE initDataBuffer(
    IIS_XHEAACENC_DATA_BUFFER** const phDataBuffer) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  if (phDataBuffer == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    *phDataBuffer = (IIS_XHEAACENC_DATA_BUFFER*)iisCalloc(1, sizeof(struct xheaacenc_databuffer_struct));
    if (*phDataBuffer == NULL) {
      returnValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
    if (!isError(returnValue)) {
      (*phDataBuffer)->dataSizeBytes = 0;
    }
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE checksumXOR(
    unsigned char const* const data,
    unsigned int const dataSizeBytes,
    unsigned int* const checksum) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int tmpChecksum = 0;
  unsigned int chunk = 0;

  if (NULL == data || NULL == checksum) {
    returnValue = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  if (!isError(returnValue)) {
    for (i = 0; i < dataSizeBytes; i += 4) {
      unsigned char array[4] = {0};

      for (j = 0; j < 4 && (i + j) < dataSizeBytes; j++) {
        array[j] = data[i + j];
      }

      chunk = array[0] << 24 |
              array[1] << 16 |
              array[2] << 8 |
              array[3];

      tmpChecksum = tmpChecksum ^ chunk;
    }
    *checksum = tmpChecksum;
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE getVersionInfo(
    unsigned char* const versionInfo,
    unsigned int* const versionInfoSizeBytes) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (NULL == versionInfo || NULL == versionInfoSizeBytes) {
    returnValue = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  if (!isError(returnValue)) {
    const char* libVersionInfo;
    size_t libVersionLength;

    libVersionInfo = IIS_xHEAACEnc_GetVersionInfoStr();

    libVersionLength = strlen(libVersionInfo);

    if (libVersionLength <= (size_t)(*versionInfoSizeBytes)) {
      *versionInfoSizeBytes = (unsigned int)libVersionLength;
    }

    memcpy(versionInfo, libVersionInfo, *versionInfoSizeBytes);
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE packData(
    IIS_XHEAACENC_DATA_BUFFER* const hdataBuffer,
    unsigned char const* const dataIn,
    unsigned int const dataSizeBytesIn,
    unsigned char const** const dataOut,
    unsigned int* const dataSizeBytesOut) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int checksumValue = 0;
  unsigned char versionInfo[VERSION_BYTES_MAX] = {0x00};
  unsigned int versionSizeBytes = VERSION_BYTES_MAX;
  unsigned char* checksumData = NULL;
  unsigned int checksumDataSize = 0;

  if (NULL == hdataBuffer || NULL == dataIn || NULL == dataOut || NULL == dataSizeBytesOut) {
    returnValue = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  if (!isError(returnValue)) {
    if (NULL != hdataBuffer->dataPacked) {
      iisFree(hdataBuffer->dataPacked);
    }

    hdataBuffer->dataSizeBytes = dataSizeBytesIn + CHECKSUM_BYTES;
    hdataBuffer->dataPacked = (unsigned char*)iisCalloc(hdataBuffer->dataSizeBytes, sizeof(unsigned char));

    if (NULL == hdataBuffer->dataPacked) {
      returnValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(returnValue)) {
    returnValue = getVersionInfo(versionInfo, &versionSizeBytes);
  }

  if (!isError(returnValue)) {
    checksumDataSize = dataSizeBytesIn + versionSizeBytes;
    checksumData = (unsigned char*)iisCalloc(checksumDataSize, sizeof(unsigned char));

    if (NULL == checksumData) {
      returnValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(returnValue)) {
    memcpy(checksumData, dataIn, sizeof(unsigned char) * dataSizeBytesIn);

    returnValue = checksumXOR(checksumData, checksumDataSize, &checksumValue);
    if (NULL != checksumData) {
      iisFree(checksumData);
      checksumData = NULL;
    }
  }

  if (!isError(returnValue)) {
    memcpy(hdataBuffer->dataPacked, dataIn, sizeof(unsigned char) * dataSizeBytesIn);
    memcpy(hdataBuffer->dataPacked + dataSizeBytesIn, &checksumValue, sizeof(unsigned char) * CHECKSUM_BYTES);

    *dataOut = hdataBuffer->dataPacked;
    *dataSizeBytesOut = hdataBuffer->dataSizeBytes;
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE validateData(
    unsigned char const* const dataIn,
    unsigned int const dataSizeBytesIn) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int checksumValue = 0;
  unsigned char versionInfo[VERSION_BYTES_MAX] = {0x00};
  unsigned int versionSizeBytes = VERSION_BYTES_MAX;
  unsigned char* checksumData = NULL;
  unsigned int checksumDataSize = 0;
  unsigned int payloadSizeBytes = 0;

  if (NULL == dataIn) {
    returnValue = IIS_XHEAACENC_ERROR_INTERNAL;
  }

  if (dataSizeBytesIn < CHECKSUM_BYTES) {
    returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
  }

  if (!isError(returnValue)) {
    returnValue = getVersionInfo(versionInfo, &versionSizeBytes);
  }

  if (!isError(returnValue)) {
    payloadSizeBytes = dataSizeBytesIn - CHECKSUM_BYTES;
    checksumDataSize = payloadSizeBytes + versionSizeBytes;
    checksumData = (unsigned char*)iisCalloc(checksumDataSize, sizeof(unsigned char));

    if (NULL == checksumData) {
      returnValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(returnValue)) {
    memcpy(checksumData, dataIn, sizeof(unsigned char) * payloadSizeBytes);
    returnValue = checksumXOR(checksumData, checksumDataSize, &checksumValue);
    if (NULL != checksumData) {
      iisFree(checksumData);
      checksumData = NULL;
    }
  }

  if (!isError(returnValue)) {
    unsigned int checksumValueTransmitted;

    memcpy(&checksumValueTransmitted, dataIn + payloadSizeBytes, sizeof(unsigned char) * CHECKSUM_BYTES);

    if (checksumValue != checksumValueTransmitted) {
      returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }
  }

  return returnValue;
}

static IIS_XHEAACENC_RETURN_CODE validateLoudness(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (hLoudness == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    if (hLoudness->hLoudnessData->minRequiredSamplesProcessed == 0) {
      returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS_SAMPLES_TOO_FEW;
    } else if (hLoudness->measureAnchorLoudness && hLoudness->anchorLoudness > XHEAACENC_LOUDNESS_LEVEL_MAX) {
      returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASURED_OUT_OF_BOUND;
    } else if (hLoudness->loudness > XHEAACENC_LOUDNESS_LEVEL_MAX) {
      returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASURED_OUT_OF_BOUND;
    } else if (hLoudness->samplePeak > XHEAACENC_SAMPLE_PEAK_MAX) {
      returnValue = IIS_XHEAACENC_ERROR_SAMPLE_PEAK_MEASURED_OUT_OF_BOUND;
    }
  }

  return returnValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Export(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    unsigned char const** const loudnessData,
    unsigned int* const numBytes) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int bufferLength = 0;
  unsigned int bufferOffset = 0;
  unsigned char* dataBuffer = NULL;
  if ((hLoudness == NULL) || (loudnessData == NULL) || (numBytes == NULL)) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue)) {
    returnValue = validateLoudness(hLoudness);
  }

  if (!isError(returnValue)) {
    returnValue = getLoudnessInstanceDataSize(hLoudness, &bufferLength);
  }

  if (!isError(returnValue)) {
    returnValue = initDataBuffer(&(hLoudness->buffer));
  }

  if (!isError(returnValue)) {
    dataBuffer = (unsigned char*)iisCalloc(bufferLength, sizeof(unsigned char));
    if (NULL == dataBuffer) {
      returnValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (!isError(returnValue)) {
    returnValue = exportLoudnessInstance(hLoudness, dataBuffer, &bufferOffset);
  }

  if (!isError(returnValue)) {
    returnValue = exportLoudnessData(hLoudness, dataBuffer, &bufferOffset);
  }

  if (!isError(returnValue)) {
    returnValue = packData(hLoudness->buffer,
                           dataBuffer,
                           bufferLength,
                           loudnessData,
                           numBytes);
    if (isError(returnValue)) {
      *numBytes = 0;
      *loudnessData = NULL;
    }
  }

  if (dataBuffer != NULL) {
    iisFree(dataBuffer);
    dataBuffer = NULL;
  }

  return returnValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Loudness_Import(
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness,
    unsigned char const* const loudnessData,
    unsigned int const numBytes) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;
  unsigned int bufferOffset = 0;

  if (hLoudness == NULL || loudnessData == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (numBytes == 0) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  if (!isError(returnValue)) {
    returnValue = initDataBuffer(&(hLoudness->buffer));
  }

  if (!isError(returnValue)) {
    returnValue = validateData(loudnessData, numBytes);
  }

  if (!isError(returnValue)) {
    returnValue = importLoudnessInstance(loudnessData, hLoudness, &bufferOffset, numBytes);
  }

  if (!isError(returnValue)) {
    returnValue = importLoudnessData(loudnessData, hLoudness, &bufferOffset, numBytes);
  }

  if (!isError(returnValue)) {
    hLoudness->isImported = 1;
  }

  if (!isError(returnValue)) {
    if (bufferOffset + CHECKSUM_BYTES != numBytes) {
      returnValue = IIS_XHEAACENC_ERROR_LOUDNESS_DATA_CORRUPTED;
    }
  }

  return returnValue;
}

IIS_XHEAACENC_RETURN_CODE deleteDataBuffer(
    IIS_XHEAACENC_DATA_BUFFER** const phDataBuffer) {
  IIS_XHEAACENC_RETURN_CODE returnValue = IIS_XHEAACENC_NO_ERROR;

  if (phDataBuffer == NULL) {
    returnValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(returnValue) && *phDataBuffer != NULL) {
    if ((*phDataBuffer)->dataPacked != NULL) {
      iisFree((*phDataBuffer)->dataPacked);
      (*phDataBuffer)->dataPacked = NULL;
    }
    (*phDataBuffer)->dataSizeBytes = 0;

    iisFree(*phDataBuffer);
    *phDataBuffer = NULL;
  }

  return returnValue;
}

int memCopyCheck(
    unsigned int const bytesCopied,
    unsigned int const bytestoCopy,
    unsigned int const numBytes) {
  int retVal = 0;
  if (bytesCopied + bytestoCopy <= numBytes - CHECKSUM_BYTES) {
    retVal = 1;
  }
  return retVal;
}
