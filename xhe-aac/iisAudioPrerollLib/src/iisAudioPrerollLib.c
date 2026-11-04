
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

#define AUDIOPREROLLLIB_VERSION_NUMBER "08.04.02"
#define AUDIOPREROLLLIB_MODULE_NAME "iisAudioPrerollLib"

#define AUDIOPREROLLLIB_BUILD_DATE __DATE__
#define AUDIOPREROLLLIB_BUILD_INFO "Release build"

#ifdef __GNUC__
#define AUDIOPREROLLLIB_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
#define AUDIOPREROLLLIB_COMPILER_INFO "Compiler: GCC"
#else
#ifdef _MSC_VER
#define AUDIOPREROLLLIB_COMPILER_VERSION _MSC_VER
#define AUDIOPREROLLLIB_COMPILER_INFO "Compiler: Visual C"
#else
#define AUDIOPREROLLLIB_COMPILER_VERSION 0
#define AUDIOPREROLLLIB_COMPILER_INFO "Compiler: unknown"
#endif
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iisutillib.h"
#include "iisAudioPrerollLib.h"
#include "readonlybitbuf.h"
#include "writeonlybitbuf.h"

#define MAX_AU_SIZE_PER_CHANNEL (6144)
#define IIS_AUDIOPREROLLLIB_MAX_CONFIG_SIZE (286)

#define IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES (3 + 15)
#define BITS_PER_BYTE (8)

#define IIS_AUDIOPREROLLLIB_MAX_NUM_CONCURRENT_PREROLL_REQUESTS (IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES + 1)

typedef struct audioprerolllib_private_data_struct {
  int nSize;

  int nPreRollAu;

  int crossOverFlagOffset;

  int preRollBufSize;
  int auOffset;
  unsigned char *charBuf;
  wobitbufHandle hBitBuf;

  unsigned char *bufferAU[IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES];
  wobitbufHandle hBitBufferAU[IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES];
  int nBufferedAUs;
  int preRollRequestsInQueue;

  int nSaveAUByteLength;
  int isValid;

} AUDIOPREROLLLIB_PRIVATE_DATA, *AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE;

static AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE iisAudioPreRollLibGetPrivateDataHandle(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hInstance);

static AUDIOPREROLLLIB_RETURN iisAudioPreRollLibWriteNumPreRollFrames(wobitbufHandle hBitBuf, int numberPreRollFrames, int *bitsUsed);

static AUDIOPREROLLLIB_RETURN iisAudioPreRollGetAuPreRollFromBuffers(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll);

static AUDIOPREROLLLIB_RETURN iisAudioPreRollResetAUBuffers(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll);

static int isError(AUDIOPREROLLLIB_RETURN const retValue);

static AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE iisAudioPreRollLibGetPrivateDataHandle(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hInstance) {
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct audioprerolllib_instance_struct));
  return hPrivateData;
}

static AUDIOPREROLLLIB_RETURN iisAudioPreRollLibWriteNumPreRollFrames(wobitbufHandle hBitBuf, int numberPreRollFrames, int *bitsUsed) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;

  if (!isError(retError)) {
    if (numberPreRollFrames > IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES || numberPreRollFrames < 0) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    } else if (numberPreRollFrames >= 3) {
      if (hBitBuf != NULL) {
        wobitbuf_WriteBits(hBitBuf, 3, 2);
        wobitbuf_WriteBits(hBitBuf, numberPreRollFrames - 3, 4);
      }
      if (bitsUsed != NULL) {
        *bitsUsed += 6;
      }
    } else if (numberPreRollFrames >= 0) {
      if (hBitBuf != NULL) {
        wobitbuf_WriteBits(hBitBuf, numberPreRollFrames, 2);
      }
      if (bitsUsed != NULL) {
        *bitsUsed += 2;
      }
    } else {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }
  }
  return retError;
}

static AUDIOPREROLLLIB_RETURN iisAudioPreRollResetAUBuffers(AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll) {
  int iBuffer = 0;
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hPreRoll == NULL) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    for (iBuffer = 0; !isError(retError) && iBuffer < hPrivateData->nPreRollAu; iBuffer++) {
      wobitbuf_Reset(hPrivateData->hBitBufferAU[iBuffer]);

      if (wobitbuf_Init(hPrivateData->hBitBufferAU[iBuffer],
                        hPrivateData->bufferAU[iBuffer],
                        hPrivateData->nSaveAUByteLength * BITS_PER_BYTE,
                        0)) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }
    }
    hPrivateData->nBufferedAUs = 0;
  }
  return retError;
}

static AUDIOPREROLLLIB_RETURN iisAudioPreRollGetAuPreRollFromBuffers(AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll) {
  int numberPreRoll = 0;
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hPreRoll == NULL) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    for (numberPreRoll = 0; !isError(retError) && numberPreRoll < hPrivateData->nPreRollAu; numberPreRoll++) {
      int auBuffersize = 0;

      if (hPrivateData->hBitBufferAU[numberPreRoll] == NULL) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }

      if (!isError(retError)) {
        if (wobitbuf_GetBitsWritten(hPrivateData->hBitBufferAU[numberPreRoll]) % BITS_PER_BYTE != 0) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }
      }

      if (!isError(retError)) {
        auBuffersize = wobitbuf_GetBitsWritten(hPrivateData->hBitBufferAU[numberPreRoll]);

        if (wobitbuf_WriteBytes(hPrivateData->hBitBuf, hPrivateData->bufferAU[numberPreRoll], auBuffersize / BITS_PER_BYTE) != 0) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }
      }
    }
  }
  return retError;
}

static AUDIOPREROLLLIB_RETURN iisAudioPreRollCyclicShiftPreRollBuffers(unsigned char **phBufferAU, wobitbufHandle *const phBitBufferAU, int nPreRoll, int byteLengthAU) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;

  unsigned char *fifoBufferAU = NULL;
  wobitbufHandle fifoBitBufferAU = NULL;

  if (*phBufferAU == NULL) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  if (*phBitBufferAU == NULL) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  fifoBufferAU = phBufferAU[0];
  fifoBitBufferAU = phBitBufferAU[0];

  if (!isError(retError)) {
    int i;

    for (i = 0; i < nPreRoll - 1; i++) {
      phBufferAU[i] = phBufferAU[i + 1];
      phBitBufferAU[i] = phBitBufferAU[i + 1];
    }

    phBufferAU[nPreRoll - 1] = fifoBufferAU;
    phBitBufferAU[nPreRoll - 1] = fifoBitBufferAU;

    memset(phBufferAU[nPreRoll - 1], 0, (size_t)byteLengthAU * sizeof(unsigned char));

    wobitbuf_Reset(phBitBufferAU[nPreRoll - 1]);

    if (wobitbuf_Init(phBitBufferAU[nPreRoll - 1],
                      phBufferAU[nPreRoll - 1],
                      byteLengthAU * sizeof(unsigned char) * BITS_PER_BYTE,
                      0)) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }
  }
  return retError;
}

static int isError(AUDIOPREROLLLIB_RETURN const retValue) {
  return (retValue >= AUDIOPREROLLLIB_ERROR_FIRST);
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibNew(
    AUDIOPREROLLLIB_INSTANCE_HANDLE *phInstance,
    AUDIOPREROLLLIB_SETUP_HANDLE hSetup) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  int nSize = 0;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll = NULL;

  nSize += sizeof(struct audioprerolllib_instance_struct);
  nSize += sizeof(struct audioprerolllib_private_data_struct);

  if ((phInstance == NULL) || (hSetup == NULL)) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_INPUT;
  }

  if (!isError(retError)) {
    if ((hSetup->nChannels < 1) ||
        (hSetup->nPreRollAu < 0) ||
        (hSetup->nPreRollAu > IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES)) {
      retError = AUDIOPREROLLLIB_ERROR_INVALID_SETUP;
    }
  }

  if (!isError(retError)) {
    if (NULL != (hPreRoll = (AUDIOPREROLLLIB_INSTANCE_HANDLE)iisMalloc(nSize))) {
      memset(hPreRoll, 0, nSize);
      hPrivateData = (AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE)((char *)(hPreRoll) + sizeof(struct audioprerolllib_instance_struct));
      hPrivateData->nSize = nSize;

      sprintf(hPreRoll->infoModuleVersion, "%s %s", AUDIOPREROLLLIB_MODULE_NAME, AUDIOPREROLLLIB_VERSION_NUMBER);

      hPrivateData->nPreRollAu = hSetup->nPreRollAu;
      hPrivateData->nSaveAUByteLength = ((hSetup->nChannels * MAX_AU_SIZE_PER_CHANNEL) + 32 + 7) >> 3;
      hPrivateData->preRollBufSize = 16 + (IIS_AUDIOPREROLLLIB_MAX_CONFIG_SIZE << 3) + 6 + (hSetup->nPreRollAu * ((hPrivateData->nSaveAUByteLength) << 3));
      hPrivateData->hBitBuf = NULL;
      hPrivateData->charBuf = NULL;
      hPrivateData->crossOverFlagOffset = -1;
      hPrivateData->auOffset = -1;
      hPrivateData->preRollRequestsInQueue = 0;
      hPrivateData->nBufferedAUs = 0;

    } else {
      retError = AUDIOPREROLLLIB_ERROR_MEMORY;
    }
  }

  if (!isError(retError)) {
    if (NULL == (hPrivateData->charBuf = (unsigned char *)iisMalloc(((hPrivateData->preRollBufSize + 7) >> 3) * sizeof(unsigned char)))) {
      retError = AUDIOPREROLLLIB_ERROR_MEMORY;
    } else {
      memset(hPrivateData->charBuf, 0, ((hPrivateData->preRollBufSize + 7) >> 3));
      if (NULL == (hPrivateData->hBitBuf = (wobitbufHandle)iisMalloc(sizeof(wobitbuf)))) {
        retError = AUDIOPREROLLLIB_ERROR_MEMORY;
      } else {
        memset(hPrivateData->hBitBuf, 0, sizeof(wobitbuf));
      }
    }
  }

  if (!isError(retError)) {
    int i = 0;
    for (i = 0; i < hPrivateData->nPreRollAu && !isError(retError) && i < IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES; i++) {
      if (NULL == (hPrivateData->bufferAU[i] = (unsigned char *)iisMalloc(hPrivateData->nSaveAUByteLength * sizeof(unsigned char)))) {
        retError = AUDIOPREROLLLIB_ERROR_MEMORY;
      } else {
        memset(hPrivateData->bufferAU[i], 0, (hPrivateData->nSaveAUByteLength * sizeof(unsigned char)));
        if (NULL == (hPrivateData->hBitBufferAU[i] = (wobitbufHandle)iisMalloc(sizeof(wobitbuf)))) {
          retError = AUDIOPREROLLLIB_ERROR_MEMORY;
        } else {
          memset(hPrivateData->hBitBufferAU[i], 0, sizeof(wobitbuf));
        }
      }
    }
  }

  if (!isError(retError)) {
    int i;
    for (i = 0; i < hPrivateData->nPreRollAu && i < IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES && !isError(retError); i++) {
      if (wobitbuf_Init(hPrivateData->hBitBufferAU[i],
                        hPrivateData->bufferAU[i],
                        hPrivateData->nSaveAUByteLength * sizeof(unsigned char) * BITS_PER_BYTE,
                        0)) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }
    }
  }

  if (!isError(retError)) {
    if (0 != wobitbuf_Init(hPrivateData->hBitBuf,
                           hPrivateData->charBuf,
                           hPrivateData->preRollBufSize,
                           0)) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }
  }

  if (!isError(retError)) {
    *phInstance = hPreRoll;
  } else {
    iisAudioPreRollLibDelete(&hPreRoll);
  }

  return retError;
}

void iisAudioPreRollLibDelete(
    AUDIOPREROLLLIB_INSTANCE_HANDLE *hInstance) {
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll = NULL;

  if ((hInstance != NULL) && (*hInstance != NULL)) {
    int i;

    hPreRoll = *hInstance;

    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    iisFree(hPrivateData->charBuf);
    iisFree(hPrivateData->hBitBuf);

    for (i = 0; i < IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES; i++) {
      if (hPrivateData->bufferAU[i] != NULL) {
        iisFree(hPrivateData->bufferAU[i]);
      }

      if (hPrivateData->hBitBufferAU[i] != NULL) {
        iisFree(hPrivateData->hBitBufferAU[i]);
      }
    }

    iisFree(hPreRoll);

    hPreRoll = NULL;
    *hInstance = NULL;
  }

  return;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibWriteConfig(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    unsigned char const *pConfig,
    int configSizeBytes,
    int applyCrossfade) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int nBits = 0;
  int prerollDataLength = 0;
  int bPrerollActive = 0;
  unsigned char *prerollSaveBuffer = NULL;

  if ((configSizeBytes > 0 && pConfig == NULL) || (hPreRoll == NULL)) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_INPUT;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    if (hPrivateData->preRollRequestsInQueue) {
      bPrerollActive = 1;
    }

    if (bPrerollActive) {
      robitbuf bitBuf;

      prerollDataLength = wobitbuf_GetBitsWritten(hPrivateData->hBitBuf);

      prerollSaveBuffer = (unsigned char *)iisMalloc((size_t)prerollDataLength / 8 + 1);

      if (NULL == prerollSaveBuffer) {
        retError = AUDIOPREROLLLIB_ERROR_MEMORY;
      } else {
        robitbuf_Init(&bitBuf, hPrivateData->hBitBuf->charBuffer, wobitbuf_GetBufferSize(hPrivateData->hBitBuf), hPrivateData->auOffset);
        robitbuf_ReadBytes(&bitBuf, prerollSaveBuffer, prerollDataLength / 8);
        prerollSaveBuffer[prerollDataLength / 8] = (unsigned char)robitbuf_ReadBits(&bitBuf, prerollDataLength % 8);
      }
    }
    if (!isError(retError)) {
      if (0 != wobitbuf_Init(hPrivateData->hBitBuf,
                             hPrivateData->charBuf,
                             hPrivateData->preRollBufSize,
                             0)) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }
    }
  }

  if (!isError(retError)) {
    if (configSizeBytes >= 30) {
      wobitbuf_WriteBits(hPrivateData->hBitBuf, 255, 8);
      wobitbuf_WriteBits(hPrivateData->hBitBuf, configSizeBytes - 30, 8);
      nBits += 16;
    } else if (configSizeBytes >= 15) {
      wobitbuf_WriteBits(hPrivateData->hBitBuf, 15, 4);
      wobitbuf_WriteBits(hPrivateData->hBitBuf, configSizeBytes - 15, 4);
      nBits += (4 + 4);
    } else {
      wobitbuf_WriteBits(hPrivateData->hBitBuf, configSizeBytes, 4);
      nBits += 4;
    }

    if (pConfig) {
      wobitbuf_WriteBytes(hPrivateData->hBitBuf, pConfig, configSizeBytes);
      nBits += (configSizeBytes << 3);
    }

    hPrivateData->crossOverFlagOffset = nBits;
    wobitbuf_WriteBits(hPrivateData->hBitBuf, applyCrossfade, 1);
    wobitbuf_WriteBits(hPrivateData->hBitBuf, 0, 1);
    nBits += 2;

    if (nBits != wobitbuf_GetBitsWritten(hPrivateData->hBitBuf)) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    } else {
      hPrivateData->auOffset = nBits;
    }
  }

  if (!isError(retError)) {
    if (bPrerollActive) {
      wobitbuf_WriteBytes(hPrivateData->hBitBuf, prerollSaveBuffer, prerollDataLength / 8);
      wobitbuf_WriteBits(hPrivateData->hBitBuf, (int)prerollSaveBuffer[prerollDataLength / 8], prerollDataLength % 8);
    }
  }

  if (NULL != prerollSaveBuffer) {
    iisFree(prerollSaveBuffer);
  }

  return retError;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibRequest(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hPreRoll == NULL) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    hPrivateData->preRollRequestsInQueue++;

    if (hPrivateData->preRollRequestsInQueue > (hPrivateData->nPreRollAu + 1) || hPrivateData->preRollRequestsInQueue > IIS_AUDIOPREROLLLIB_MAX_NUM_CONCURRENT_PREROLL_REQUESTS || hPrivateData->preRollRequestsInQueue < 0) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }

    if (!isError(retError)) {
      hPrivateData->isValid = 1;
    }
  }
  return retError;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibGetAuPreRollData(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    unsigned char **pBuffer,
    int *nBytesBuffer,
    int const maxExtensionBytes,
    int *preRollAvailable) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  *preRollAvailable = 0;

  if ((hPreRoll == NULL) || (pBuffer == NULL) || (nBytesBuffer == NULL)) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);
    *nBytesBuffer = 0;
    *pBuffer = NULL;
  }

  if (!isError(retError)) {
    if (hPrivateData->preRollRequestsInQueue > 0 && hPrivateData->nBufferedAUs == hPrivateData->nPreRollAu) {
      int configLength = (hPrivateData->auOffset);

      wobitbuf_Reset(hPrivateData->hBitBuf);

      if (wobitbuf_Init(hPrivateData->hBitBuf,
                        hPrivateData->charBuf,
                        hPrivateData->preRollBufSize,
                        hPrivateData->auOffset)) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }

      if (!isError(retError)) {
        if (wobitbuf_GetBitsWritten(hPrivateData->hBitBuf) != 0) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }
      }

      if (!isError(retError)) {
        if (isError(iisAudioPreRollLibWriteNumPreRollFrames(hPrivateData->hBitBuf, hPrivateData->nPreRollAu, NULL))) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }
      }

      if (!isError(retError)) {
        retError = iisAudioPreRollGetAuPreRollFromBuffers(hPreRoll);
      }

      if (!isError(retError)) {
        if ((maxExtensionBytes >= 0 && (wobitbuf_GetBitsWritten(hPrivateData->hBitBuf) + configLength + BITS_PER_BYTE - 1) / BITS_PER_BYTE > maxExtensionBytes) || !hPrivateData->isValid) {
          wobitbuf_Reset(hPrivateData->hBitBuf);

          if (wobitbuf_Init(hPrivateData->hBitBuf,
                            hPrivateData->charBuf,
                            hPrivateData->preRollBufSize,
                            hPrivateData->auOffset)) {
            retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
          }

          if (!isError(retError)) {
            if (isError(iisAudioPreRollLibWriteNumPreRollFrames(hPrivateData->hBitBuf, 0, NULL))) {
              retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
            }
          }

          if (!isError(retError)) {
            retError = AUDIOPREROLLLOB_WARNING_DROPPED_PREROLL;
          }
        }
      }

      if (!isError(retError)) {
        *pBuffer = hPrivateData->charBuf;
        *nBytesBuffer = (wobitbuf_GetBitsWritten(hPrivateData->hBitBuf) + configLength + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
      }

      if (!isError(retError)) {
        *preRollAvailable = 1;

        hPrivateData->preRollRequestsInQueue--;

        if (hPrivateData->preRollRequestsInQueue < 0) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }

        if (!isError(retError) && hPrivateData->preRollRequestsInQueue == 0) {
          AUDIOPREROLLLIB_RETURN err = iisAudioPreRollResetAUBuffers(hPreRoll);

          if (isError(err)) {
            retError = err;
          }
        }
      }
    }
  }
  return retError;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibCollectAccessUnit(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    unsigned char const *iisAudioPreRollAu,
    int const iisAudioPreRollLength) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int nBits = 0;

  if ((iisAudioPreRollAu == NULL) || (hPreRoll == NULL)) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_INPUT;
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    if (hPrivateData->nBufferedAUs > hPrivateData->nPreRollAu || hPrivateData->nBufferedAUs > IIS_AUDIOPREROLLLIB_MAX_NUM_PREROLL_FRAMES || hPrivateData->nBufferedAUs < 0) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }

    if (!isError(retError) && hPrivateData->nPreRollAu > 0 && hPrivateData->preRollRequestsInQueue > 0) {
      wobitbufHandle hBitBufferAU = NULL;

      if (!isError(retError) && hPrivateData->nBufferedAUs == hPrivateData->nPreRollAu) {
        retError = iisAudioPreRollCyclicShiftPreRollBuffers(hPrivateData->bufferAU, hPrivateData->hBitBufferAU, hPrivateData->nPreRollAu, hPrivateData->nSaveAUByteLength);

        if (!isError(retError)) {
          hPrivateData->nBufferedAUs--;
        }
      }

      if (!isError(retError)) {
        hBitBufferAU = hPrivateData->hBitBufferAU[hPrivateData->nBufferedAUs];

        if (hBitBufferAU == NULL || wobitbuf_GetBitsWritten(hBitBufferAU) != 0) {
          retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
        }
      }

      if (!isError(retError)) {
        nBits = wobitbuf_GetBitsWritten(hBitBufferAU);

        if (iisAudioPreRollLength >= ((1 << 16) - 1)) {
          wobitbuf_WriteBits(hBitBufferAU, ((1 << 16) - 1), 16);
          wobitbuf_WriteBits(hBitBufferAU, iisAudioPreRollLength - ((1 << 16) - 1), 16);
          nBits += 32;
        } else {
          wobitbuf_WriteBits(hBitBufferAU, iisAudioPreRollLength, 16);
          nBits += 16;
        }
      }

      if ((iisAudioPreRollAu[0] >> 6) & 1) {
        hPrivateData->isValid = 0;
      }

      wobitbuf_WriteBytes(hBitBufferAU, iisAudioPreRollAu, iisAudioPreRollLength);
      nBits += (iisAudioPreRollLength << 3);

      if (nBits != wobitbuf_GetBitsWritten(hBitBufferAU)) {
        retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
      }

      if (!isError(retError)) {
        hPrivateData->nBufferedAUs++;
      }
    }
  }
  return retError;
}

int iisAudioPreRollLibGetnPreRollAu(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll) {
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hPreRoll != NULL) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    return hPrivateData->nPreRollAu;
  }
  assert(0);
  return -1;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibGetDelays(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    unsigned int *totalDelay) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if ((hPreRoll == NULL) || (totalDelay == NULL)) {
    retError = AUDIOPREROLLLIB_ERROR_INVALID_INPUT;
  } else {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);

    *totalDelay = hPrivateData->nPreRollAu;
  }

  return retError;
}

int iisAudioPreRollLibIsReady(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll) {
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int bReady = 0;

  if (hPreRoll != NULL) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);
    bReady = (hPrivateData->preRollRequestsInQueue < hPrivateData->nPreRollAu + 1);
  }
  return bReady;
}

int iisAudioPreRollLibPayloadSizeWithoutAUs(
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hPreRoll) {
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int returnSize = 0;
  AUDIOPREROLLLIB_RETURN errorValue = AUDIOPREROLLLIB_NO_ERROR;

  if (hPreRoll != NULL) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);
    returnSize = hPrivateData->auOffset;
    errorValue = iisAudioPreRollLibWriteNumPreRollFrames(NULL, hPrivateData->nPreRollAu, &returnSize);
    if (isError(errorValue)) {
      returnSize = 0;
    }
  }

  return returnSize;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibChangeCrossfadeFlag(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    int applyCrossfade) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  wobitbuf bitBuff;

  if (!isError(retError)) {
    if (hPreRoll == NULL) {
      retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    if (applyCrossfade != 0 && applyCrossfade != 1) {
      retError = AUDIOPREROLLLIB_ERROR_INVALID_INPUT;
    }
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);
    if (hPrivateData->crossOverFlagOffset < 0) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }
  }

  if (!isError(retError)) {
    if (wobitbuf_Init(&bitBuff,
                      hPrivateData->charBuf,
                      hPrivateData->preRollBufSize,
                      hPrivateData->crossOverFlagOffset)) {
      retError = AUDIOPREROLLLIB_ERROR_UNKNOWN;
    }
  }

  if (!isError(retError)) {
    wobitbuf_WriteBits(&bitBuff, applyCrossfade, 1);
  }

  return retError;
}

AUDIOPREROLLLIB_RETURN iisAudioPreRollLibRequestedAUSizeThisFrame(
    AUDIOPREROLLLIB_INSTANCE_HANDLE hPreRoll,
    int *nMinRequestedBits,
    int *nComfortableRequestedBits) {
  AUDIOPREROLLLIB_RETURN retError = AUDIOPREROLLLIB_NO_ERROR;
  AUDIOPREROLLLIB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (!isError(retError)) {
    if (hPreRoll == NULL) {
      retError = AUDIOPREROLLLIB_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    hPrivateData = iisAudioPreRollLibGetPrivateDataHandle(hPreRoll);
  }

  if (!isError(retError)) {
    *nMinRequestedBits = ((iisAudioPreRollLibPayloadSizeWithoutAUs(hPreRoll) + 7) / 8) * 8;
    *nComfortableRequestedBits = ((wobitbuf_GetBitsWritten(hPrivateData->hBitBuf) + hPrivateData->auOffset + 7) / 8) * 8;
  }
  return retError;
}
