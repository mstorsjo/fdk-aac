
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
#include <stdio.h>
#include <float.h>

#include "iisBitFrame.h"
#include "iisutillib.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MAX_BITS_PER_CHANNEL (6144)

struct IISBITFRAME_TAG {
  int bitReservoir;
  int bitReservoirPrevious;
  int bitReservoirMax;
  int averageBytesPerFrame;
  int restOfAverageBytesPerFrame;
  int paddingBits;
  int paddingRest;
  int bPaddingOn;
  int paddingLastFrame;
  int bitRate;
  int samplesPerFrame;
  int sampleRate;
  int bitrateRemainder;
  int timeBase;
  STREAM_TYPE bitStreamType;
  int extendedBitReservoir;
  int extendedBitReservoirMax;
  int extendedBitReservoirFillRate;
  int extendedBitReservoirGrantedBits;
};

static int isError(
    IISBITFRAME_ERROR const retValue) {
  return (retValue != IISBITFRAME_NO_ERROR);
}

static int iisbitframe_GetAverageBytesPerFrame(
    IISBITFRAME_HANDLE hBitFrame);

static int iisbitframe_GetRestOfAverageBytesPerFrame(
    IISBITFRAME_HANDLE hBitFrame);

static void iisbitframe_InitFramePadding(
    IISBITFRAME_HANDLE hBitFrame);

IISBITFRAME_ERROR IISBITFRAME_CreateNewBitFrame(
    IISBITFRAME_HANDLE* const phBitFrame) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (!phBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *phBitFrame = iisCalloc(1, sizeof(IISBITFRAME));

    if (NULL == *phBitFrame) {
      retValue = IISBITFRAME_INVALID_HANDLE;
    }
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_InitBitFrame(
    IISBITFRAME_HANDLE hBitFrame,
    int const bitRate,
    int const totalBitrate,
    int const numChannels,
    int const maxChannels,
    int const sampleRate,
    int const granuleLength,
    int const bitrateRemainder,
    int const timeBase,
    float const bitresFrames,
    float const bitResInitFillLevel,
    float const bitresDistribution,
    STREAM_TYPE const bitStreamType) {
  float granuleBits = 0.0f;
  unsigned int bitResMaxBits = 0;
  unsigned int defaultBitresMaxBytes = 0;
  float bitResInitFactor = 0.8f;
  float bitresDist = 0.0f;
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (!isError(retValue)) {
    if (bitResInitFillLevel > FLT_MIN || bitResInitFillLevel < FLT_MIN) {
      bitResInitFactor = bitResInitFillLevel;
    }

    hBitFrame->bitRate = bitRate;
    hBitFrame->sampleRate = sampleRate;
    hBitFrame->samplesPerFrame = granuleLength;
    hBitFrame->bitStreamType = bitStreamType;
    hBitFrame->timeBase = 1;
    hBitFrame->bitrateRemainder = 0;

    if (timeBase > 1) {
      hBitFrame->timeBase = timeBase;
      hBitFrame->bitrateRemainder = bitrateRemainder;
    }

    granuleBits = (float)totalBitrate * granuleLength / sampleRate;

    bitResMaxBits = (unsigned int)min((unsigned int)(bitresFrames * granuleBits), max(maxChannels * MAX_BITS_PER_CHANNEL - granuleBits, 0));

    if (bitRate / numChannels > MAX_BITS_PER_CHANNEL * sampleRate / granuleLength) {
      retValue = IISBITFRAME_BUFSIZE_ERROR;
    }
  }

  if (!isError(retValue)) {
    bitresDist = (float)bitRate / totalBitrate;

    if (bitStreamType == MPEG4_SCA || bitStreamType == MPEG2_AAC) {
      if (bitresDistribution > FLT_MIN || bitresDistribution < FLT_MIN) {
        bitresDist = bitresDistribution;
      }
    }

    defaultBitresMaxBytes = (((unsigned int)((float)bitResMaxBits * bitresDist)) / 8);

    hBitFrame->bitReservoirMax = defaultBitresMaxBytes * 8;
    hBitFrame->bitReservoir = ((int)(bitResInitFactor * defaultBitresMaxBytes)) * 8;
    hBitFrame->bitReservoirPrevious = hBitFrame->bitReservoir;

    iisbitframe_InitFramePadding(hBitFrame);
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_InitExtendedBitFrame(
    IISBITFRAME_HANDLE hBitFrame,
    const int extendedBits,
    const int extendedBitresFillRate) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;
  int extendedBitresSize = 0;
  int extendedBitresFillRateByteAligned = 0;

  if (hBitFrame == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    extendedBitresSize = extendedBits - extendedBits % 8;
    extendedBitresFillRateByteAligned = extendedBitresFillRate;
    if (extendedBitresFillRate % 8 > 0) {
      extendedBitresFillRateByteAligned += (8 - (extendedBitresFillRate % 8));
    }

    extendedBitresSize = min(extendedBitresSize, hBitFrame->bitReservoirMax * 3 / 4);
    extendedBitresSize = extendedBitresSize - (extendedBitresSize % 8);

    hBitFrame->bitReservoirMax = hBitFrame->bitReservoirMax - extendedBitresSize;
    if (extendedBitresSize > hBitFrame->bitReservoir) {
      retValue = IISBITFRAME_BUFSIZE_ERROR;
    } else {
      hBitFrame->extendedBitReservoir = extendedBitresSize;
      hBitFrame->extendedBitReservoirMax = extendedBitresSize;
      hBitFrame->bitReservoir = hBitFrame->bitReservoir - extendedBitresSize;
      hBitFrame->bitReservoirPrevious = hBitFrame->bitReservoir;
    }
  }

  if (!isError(retValue)) {
    hBitFrame->extendedBitReservoirFillRate = min(extendedBitresSize, extendedBitresFillRateByteAligned);

    hBitFrame->extendedBitReservoirGrantedBits = 0;
  }

  return retValue;
}

int IISBITFRAME_FramePadding(
    IISBITFRAME_HANDLE hBitFrame) {
  int denominator = 0;

  hBitFrame->paddingLastFrame = hBitFrame->bPaddingOn;

  hBitFrame->paddingRest += hBitFrame->restOfAverageBytesPerFrame;
  denominator = hBitFrame->sampleRate * hBitFrame->timeBase;

  hBitFrame->paddingBits = 8 * hBitFrame->paddingRest / denominator;

  if (hBitFrame->paddingRest >= hBitFrame->sampleRate * hBitFrame->timeBase) {
    hBitFrame->bPaddingOn = 1;
    hBitFrame->paddingBits -= 8;
    hBitFrame->paddingRest -= denominator;
  } else {
    hBitFrame->bPaddingOn = 0;
  }

  return hBitFrame->bPaddingOn;
}

int IISBITFRAME_GetBitreservoir(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->bitReservoir;
}

int IISBITFRAME_GetExtendedBitreservoir(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->extendedBitReservoir;
}

int IISBITFRAME_GetExtendedBitreservoirGrantedBits(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->extendedBitReservoirGrantedBits;
}

IISBITFRAME_ERROR IISBITFRAME_SetExtendedBitReservoirFillRate(
    IISBITFRAME_HANDLE hBitFrame,
    const int extendedBitresFillRate) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (NULL == hBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hBitFrame->extendedBitReservoirFillRate = extendedBitresFillRate;
  }

  return retValue;
}

int IISBITFRAME_GetExtendedBitreservoirFillRate(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->extendedBitReservoirFillRate;
}

int IISBITFRAME_GetBitreservoirMax(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->bitReservoirMax;
}

int IISBITFRAME_GetExtendedBitreservoirMax(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->extendedBitReservoirMax;
}

int IISBITFRAME_GetBitreservoirPrevious(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->bitReservoirPrevious;
}

IISBITFRAME_ERROR IISBITFRAME_GetBitreservoirInfo(
    IISBITFRAME_HANDLE hBitFrame,
    int* bitReservoir,
    int* bitReservoirMax) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (hBitFrame == NULL || bitReservoir == NULL || bitReservoirMax == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *bitReservoir = hBitFrame->bitReservoir;
    *bitReservoirMax = hBitFrame->bitReservoirMax;
  }

  return retValue;
}

int IISBITFRAME_GetAverageBitsPerFrame(
    IISBITFRAME_HANDLE hBitFrame) {
  int averageBitsPerFrame = (int)((float)hBitFrame->bitRate * hBitFrame->samplesPerFrame / hBitFrame->sampleRate);
  return averageBitsPerFrame;
}

int IISBITFRAME_GetNumberOfBitsForCurrentFrame(
    IISBITFRAME_HANDLE hBitFrame,
    const int bUsePadding) {
  int bytesPerFrame = 0;
  int extendedBitreservoirSaveBits = 0;

  bytesPerFrame = hBitFrame->averageBytesPerFrame;

  if (bUsePadding != 0) {
    bytesPerFrame += hBitFrame->bPaddingOn;
  }

  IISBITFRAME_GetExtendedBitReservoirSaveBits(hBitFrame,
                                              &extendedBitreservoirSaveBits);

  bytesPerFrame -= extendedBitreservoirSaveBits >> 3;

  return bytesPerFrame * 8;
}

int IISBITFRAME_GetFrameBitreservoir(
    IISBITFRAME_HANDLE hBitFrame) {
  return hBitFrame->bitReservoir + hBitFrame->paddingBits;
}

IISBITFRAME_ERROR IISBITFRAME_UpdateBitReservoir(
    IISBITFRAME_HANDLE hBitFrame,
    const int bitReservoir) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (NULL == hBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hBitFrame->bitReservoir = bitReservoir;
  }
  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_UpdateExtendedBitReservoir(
    IISBITFRAME_HANDLE hBitFrame,
    const int extendedBitReservoir) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (NULL == hBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hBitFrame->extendedBitReservoir = extendedBitReservoir;
  }
  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_UpdateBitFrameAndExtraFillBytes(IISBITFRAME_HANDLE const hBitFrame,
                                                              int* const unusedBits,
                                                              int* const extraFillBits) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (NULL == hBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int extendedBitreservoirSaveBits = 0;

    retValue = IISBITFRAME_GetExtendedBitReservoirSaveBits(hBitFrame, &extendedBitreservoirSaveBits);

    if (!isError(retValue)) {
      hBitFrame->extendedBitReservoir += extendedBitreservoirSaveBits;
    }
  }

  if (!isError(retValue)) {
    int numBitsExceedingMaxBitreservoir;
    int diffBits = *unusedBits;
    int bitResMod8 = 0;
    int extendedBitResMod8 = 0;
    *unusedBits = 0;

    if (hBitFrame->extendedBitReservoirGrantedBits > 0) {
      hBitFrame->extendedBitReservoir -= hBitFrame->extendedBitReservoirGrantedBits;
      hBitFrame->extendedBitReservoirGrantedBits = 0;

      if ((diffBits < 0) && (hBitFrame->extendedBitReservoir > 0)) {
        int extraBits = min(abs(diffBits), hBitFrame->extendedBitReservoir);
        extraBits -= extraBits % 8;

        hBitFrame->extendedBitReservoir -= extraBits;
        diffBits += extraBits;
      }
      assert(hBitFrame->extendedBitReservoir >= 0);
    }

    hBitFrame->bitReservoirPrevious = hBitFrame->bitReservoir;
    hBitFrame->bitReservoir += diffBits;

    numBitsExceedingMaxBitreservoir = max((hBitFrame->bitReservoir) - (hBitFrame->bitReservoirMax), 0);
    hBitFrame->bitReservoir = min(hBitFrame->bitReservoir, hBitFrame->bitReservoirMax);
    hBitFrame->bitReservoir -= *extraFillBits;

    if (numBitsExceedingMaxBitreservoir > 0 && hBitFrame->extendedBitReservoir < hBitFrame->extendedBitReservoirMax) {
      hBitFrame->extendedBitReservoir += numBitsExceedingMaxBitreservoir;
      numBitsExceedingMaxBitreservoir = max((hBitFrame->extendedBitReservoir) - (hBitFrame->extendedBitReservoirMax), 0);
      hBitFrame->extendedBitReservoir = min(hBitFrame->extendedBitReservoir, hBitFrame->extendedBitReservoirMax);
    }

    *extraFillBits += numBitsExceedingMaxBitreservoir;

    if (hBitFrame->bitReservoir < 0 && hBitFrame->extendedBitReservoir > 0) {
      int byteMissingBitRes = (abs(hBitFrame->bitReservoir) + 7) / 8;
      if (byteMissingBitRes * 8 < hBitFrame->extendedBitReservoir) {
        hBitFrame->extendedBitReservoir -= byteMissingBitRes * 8;
        hBitFrame->bitReservoir += byteMissingBitRes * 8;
      }
    }

    bitResMod8 = hBitFrame->bitReservoir % 8;
    extendedBitResMod8 = hBitFrame->extendedBitReservoir % 8;

    hBitFrame->bitReservoir -= bitResMod8;
    hBitFrame->extendedBitReservoir -= extendedBitResMod8;

    *extraFillBits += bitResMod8 + extendedBitResMod8;
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_CalculateTotalNumFillBits(int const unusedBits,
                                                        int const extraFillBits,
                                                        int const fillBits,
                                                        int* const numFillBits) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  *numFillBits = fillBits;

  if (extraFillBits < 0) {
    assert(0);
    retValue = IISBITFRAME_INVALID_PARAM;
  }

  if (!isError(retValue)) {
    int unusedBitsMod8 = unusedBits % 8;
    *numFillBits += extraFillBits;
    *numFillBits += (unusedBitsMod8 >= 0) ? unusedBitsMod8 : (unusedBitsMod8 + 8);
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_GetExtendedBitReservoirSaveBits(
    IISBITFRAME_HANDLE hBitFrame,
    int* saveBits) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (hBitFrame == NULL || saveBits == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int diffBits = 0;
    *saveBits = 0;

    diffBits = hBitFrame->extendedBitReservoirMax - hBitFrame->extendedBitReservoir;
    if (diffBits > hBitFrame->extendedBitReservoirFillRate) {
      *saveBits = hBitFrame->extendedBitReservoirFillRate;
    } else {
      *saveBits = max(0, diffBits);
    }

    *saveBits = (*saveBits / 8) * 8;
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_ExtendedBitReservoirRequestBits(
    IISBITFRAME_HANDLE hBitFrame,
    const int requestedBits,
    int* grantedBits) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (hBitFrame == NULL || grantedBits == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *grantedBits = 0;

    if ((requestedBits > 0) && (hBitFrame->extendedBitReservoirGrantedBits == 0)) {
      *grantedBits = min(requestedBits, hBitFrame->extendedBitReservoir);
    }

    *grantedBits -= *grantedBits % 8;

    hBitFrame->extendedBitReservoirGrantedBits = *grantedBits;
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_DeleteBitFrame(
    IISBITFRAME_HANDLE hBitFrame) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (NULL == hBitFrame) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    iisFree(hBitFrame);
  }
  return IISBITFRAME_NO_ERROR;
}

IISBITFRAME_ERROR IISBITFRAME_ExtendedBitReservoirUpdateParameters(
    IISBITFRAME_HANDLE hBitFrame,
    const int extendedBitsMax

) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (hBitFrame == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int extendedBitresSize = extendedBitsMax - (extendedBitsMax % 8);
    int extendedBitresMaximumSize = (hBitFrame->extendedBitReservoirMax + hBitFrame->bitReservoirMax) * 3 / 4;
    extendedBitresMaximumSize = extendedBitresMaximumSize - (extendedBitresMaximumSize % 8);
    extendedBitresSize = min(extendedBitresSize, extendedBitresMaximumSize);

    if (extendedBitresSize >= hBitFrame->extendedBitReservoirMax) {
      if (hBitFrame->bitReservoir >= hBitFrame->bitReservoirMax - (extendedBitresSize - hBitFrame->extendedBitReservoirMax)) {
        hBitFrame->bitReservoirMax -= (extendedBitresSize - hBitFrame->extendedBitReservoirMax);
        hBitFrame->extendedBitReservoir += (hBitFrame->bitReservoir - hBitFrame->bitReservoirMax);
        hBitFrame->bitReservoir -= (extendedBitresSize - hBitFrame->extendedBitReservoirMax);
      } else {
        hBitFrame->bitReservoirMax -= (extendedBitresSize - hBitFrame->extendedBitReservoirMax);
      }
    } else {
      if (extendedBitresSize >= hBitFrame->extendedBitReservoir) {
        hBitFrame->bitReservoirMax += (hBitFrame->extendedBitReservoirMax - extendedBitresSize);
      } else {
        hBitFrame->bitReservoirMax += (hBitFrame->extendedBitReservoirMax - extendedBitresSize);
        hBitFrame->bitReservoir += (hBitFrame->extendedBitReservoir - extendedBitresSize);
        hBitFrame->extendedBitReservoir = extendedBitresSize;
      }
    }
    hBitFrame->extendedBitReservoirMax = extendedBitresSize;
  }

  return retValue;
}

IISBITFRAME_ERROR IISBITFRAME_ResetRateControl(
    IISBITFRAME_HANDLE hBitFrame) {
  IISBITFRAME_ERROR retValue = IISBITFRAME_NO_ERROR;

  if (hBitFrame == NULL) {
    retValue = IISBITFRAME_INVALID_HANDLE;
  } else {
    hBitFrame->paddingRest = 0;
  }
  return retValue;
}

static int iisbitframe_GetAverageBytesPerFrame(
    IISBITFRAME_HANDLE hBitFrame) {
  int result = 0;
  int additional = 0;
  int rest = 0;

  assert((hBitFrame->samplesPerFrame % 8) == 0);
  result = (hBitFrame->samplesPerFrame / 8) * hBitFrame->bitRate;
  additional = (hBitFrame->samplesPerFrame / 8) * hBitFrame->bitrateRemainder;

  rest = result % hBitFrame->sampleRate;
  result /= hBitFrame->sampleRate;
  result += (rest * hBitFrame->timeBase + additional) / (hBitFrame->sampleRate * hBitFrame->timeBase);

  return result;
}

static int iisbitframe_GetRestOfAverageBytesPerFrame(
    IISBITFRAME_HANDLE hBitFrame) {
  int result = 0;
  int additional = 0;

  result = (hBitFrame->samplesPerFrame / 8) * hBitFrame->bitRate;
  additional = (hBitFrame->samplesPerFrame / 8) * hBitFrame->bitrateRemainder;

  result %= hBitFrame->sampleRate;
  result = result * hBitFrame->timeBase + additional;
  result %= hBitFrame->sampleRate * hBitFrame->timeBase;

  return result;
}

static void iisbitframe_InitFramePadding(
    IISBITFRAME_HANDLE hBitFrame) {
  hBitFrame->paddingBits = 0;
  hBitFrame->paddingRest = 0;
  hBitFrame->averageBytesPerFrame = iisbitframe_GetAverageBytesPerFrame(hBitFrame);
  hBitFrame->restOfAverageBytesPerFrame = iisbitframe_GetRestOfAverageBytesPerFrame(hBitFrame);
}
