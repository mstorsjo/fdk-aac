
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

#include <stdlib.h>
#include <string.h>
#include "iisxHEAACEncLib_bitDistribution.h"
#include "iisxHEAACEncLib_audioPreRoll.h"

#define XHEAACENCLIB_BITDISTRIBUTION_DEFAULT_AVERAGE_FACTOR 0.05f

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct xheaacenclib_bitDistribution_element {
  float average;
  float averageFactor;
  int ignoreForAverage;
  int actFrameUsed;
  int actFrameRequest;
  int actFrameMinimum;
} XHEAACENCLIB_BITDISTRIBUTION_ELEMENT, *XHEAACENCLIB_BITDISTRIBUTION_ELEMENT_HANDLE;

typedef struct xheaacenclib_bitdistribution_struct {
  int numberElement;
  XHEAACENCLIB_BITDISTRIBUTION_ELEMENT *elements;
  int actFrameMaxNumBitsToUse;
  int actFrameComfortableNumBitsToUse;
} XHEAACENCLIB_BITDISTRIBUTION;

static XHEAACENCLIB_RETURN bitDistribution_init(XHEAACENCLIB_BITDISTRIBUTION_HANDLE hBitDistribution, XHEAACENCLIB_BITDISTRIBUTION_CONFIG_HANDLE const bitDistributionConfig);

static XHEAACENCLIB_RETURN bitDistribution_GetThresholds(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int *maxNumBitsToUse, int *comfortableNumBitsToUse);

static XHEAACENCLIB_RETURN bitDistribution_SetThisFrameBitAvailibility(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int maxNumBitsToUse, int comfortableNumBitsToUse);

static XHEAACENCLIB_RETURN bitDistribution_SetRequestThisFrame(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int nMinRequestedBits, int nComfortableRequestedBits);

static XHEAACENCLIB_RETURN addExtensionPayloadOverhead(int *const sizeBits, int writeZeroLength, int usacExtElementDefaultLength, int usacExtElementPayloadFrag);

static XHEAACENCLIB_RETURN bitDistribution_init(XHEAACENCLIB_BITDISTRIBUTION_HANDLE hBitDistribution, XHEAACENCLIB_BITDISTRIBUTION_CONFIG_HANDLE const bitDistributionConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hBitDistribution == NULL || bitDistributionConfig == NULL || bitDistributionConfig->configElement == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hBitDistribution->numberElement = bitDistributionConfig->numberElements;
    hBitDistribution->actFrameComfortableNumBitsToUse = -1;
    hBitDistribution->actFrameMaxNumBitsToUse = -1;
    for (i = 0; i < hBitDistribution->numberElement; i++) {
      hBitDistribution->elements[i].actFrameMinimum = -1;
      hBitDistribution->elements[i].actFrameRequest = -1;
      hBitDistribution->elements[i].actFrameUsed = -1;
      hBitDistribution->elements[i].average = bitDistributionConfig->configElement[i].elementAverage;
      if (bitDistributionConfig->configElement[i].averageFactor < 0.0f || bitDistributionConfig->configElement[i].averageFactor > 1.0f) {
        hBitDistribution->elements[i].averageFactor = XHEAACENCLIB_BITDISTRIBUTION_DEFAULT_AVERAGE_FACTOR;
      } else {
        hBitDistribution->elements[i].averageFactor = bitDistributionConfig->configElement[i].averageFactor;
      }
      hBitDistribution->elements[i].ignoreForAverage = bitDistributionConfig->configElement[i].ignoreForAverage;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN bitDistribution_GetThresholds(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int *maxNumBitsToUse, int *comfortableNumBitsToUse) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hBitDistribution == NULL || (hBitDistribution->elements == NULL && hBitDistribution->numberElement > 0) || maxNumBitsToUse == NULL || comfortableNumBitsToUse == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hBitDistribution->actFrameComfortableNumBitsToUse < 0 || hBitDistribution->actFrameMaxNumBitsToUse < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_CALLING_SEQUENCE;
    }
  }

  if (!isError(retValue)) {
    if (elementID < 0 || elementID >= hBitDistribution->numberElement) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_ELEMENTID;
    }
  }

  if (!isError(retValue)) {
    *maxNumBitsToUse = hBitDistribution->actFrameMaxNumBitsToUse;
    *comfortableNumBitsToUse = hBitDistribution->actFrameComfortableNumBitsToUse;
    for (i = 0; i < hBitDistribution->numberElement; i++) {
      if (i != elementID) {
        int subtractMaxBitToUse = (int)hBitDistribution->elements[i].average;
        int subtractComfortableBitsToUse = (int)hBitDistribution->elements[i].average;
        if (hBitDistribution->elements[i].actFrameUsed >= 0) {
          subtractMaxBitToUse = hBitDistribution->elements[i].actFrameUsed;
          subtractComfortableBitsToUse = hBitDistribution->elements[i].actFrameUsed;
        } else {
          if (hBitDistribution->elements[i].actFrameRequest > 0) {
            subtractMaxBitToUse = hBitDistribution->elements[i].actFrameRequest;
            subtractComfortableBitsToUse = hBitDistribution->elements[i].actFrameRequest;
          }
          if (hBitDistribution->elements[i].actFrameMinimum > 0) {
            subtractMaxBitToUse = hBitDistribution->elements[i].actFrameRequest;
          }
        }
        if (subtractMaxBitToUse > 0) {
          *maxNumBitsToUse -= subtractMaxBitToUse;
        }
        if (subtractComfortableBitsToUse > 0) {
          *comfortableNumBitsToUse -= subtractComfortableBitsToUse;
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (*maxNumBitsToUse < 0) {
      *maxNumBitsToUse = 0;
    }
    if (*comfortableNumBitsToUse < 0) {
      *comfortableNumBitsToUse = 0;
    }
  }

  if (!isError(retValue)) {
    if (*maxNumBitsToUse > *comfortableNumBitsToUse) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_IMPLEMENTATION;
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN bitDistribution_SetThisFrameBitAvailibility(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int maxNumBitsToUse, int comfortableNumBitsToUse) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hBitDistribution == NULL || (hBitDistribution->elements == NULL && hBitDistribution->numberElement > 0)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (maxNumBitsToUse < 0 || comfortableNumBitsToUse < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_PARAMETER;
    }
  }

  if (!isError(retValue)) {
    hBitDistribution->actFrameComfortableNumBitsToUse = comfortableNumBitsToUse;
    hBitDistribution->actFrameMaxNumBitsToUse = maxNumBitsToUse;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN bitDistribution_SetRequestThisFrame(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int nMinRequestedBits, int nComfortableRequestedBits) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hBitDistribution == NULL || hBitDistribution->elements == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (elementID < 0 || elementID >= hBitDistribution->numberElement) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_ELEMENTID;
    }
  }

  if (!isError(retValue)) {
    hBitDistribution->elements[elementID].actFrameRequest = nComfortableRequestedBits;
    hBitDistribution->elements[elementID].actFrameMinimum = nMinRequestedBits;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN addExtensionPayloadOverhead(int *const sizeBits, int writeZeroLength, int usacExtElementDefaultLength, int usacExtElementPayloadFrag) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (sizeBits == NULL || *sizeBits < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int const payloadSizeBits = *sizeBits;
    int const payloadSizeBytes = (*sizeBits + 7) / 8;
    (*sizeBits)++;
    if (payloadSizeBits > 0 || writeZeroLength) {
      (*sizeBits)++;
      if (payloadSizeBytes != usacExtElementDefaultLength) {
        (*sizeBits) += 8;
        if (payloadSizeBytes >= 255) {
          (*sizeBits) += 16;
        }
      }
      if (payloadSizeBits > 0 && usacExtElementPayloadFrag) {
        (*sizeBits) += 2;
      }
      (*sizeBits) += (8 - (payloadSizeBits % 8)) % 8;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_New(XHEAACENCLIB_BITDISTRIBUTION_HANDLE *const phBitDistribution, XHEAACENCLIB_BITDISTRIBUTION_CONFIG_HANDLE const bitDistributionConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nSize = 0;

  if (phBitDistribution == NULL || *phBitDistribution != NULL || bitDistributionConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    nSize += sizeof(struct xheaacenclib_bitdistribution_struct);
    *phBitDistribution = (XHEAACENCLIB_BITDISTRIBUTION_HANDLE)iisMalloc(nSize);
    if (*phBitDistribution == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }
  if (!isError(retValue)) {
    memset(*phBitDistribution, 0, nSize);
  }

  if (!isError(retValue)) {
    if (bitDistributionConfig->numberElements < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_CONFIG;
    }
  }

  if (!isError(retValue)) {
    (*phBitDistribution)->numberElement = bitDistributionConfig->numberElements;
    if ((*phBitDistribution)->numberElement > 0) {
      (*phBitDistribution)->elements = (XHEAACENCLIB_BITDISTRIBUTION_ELEMENT *)iisMalloc(sizeof(XHEAACENCLIB_BITDISTRIBUTION_ELEMENT) * (*phBitDistribution)->numberElement);
      if ((*phBitDistribution)->elements == NULL) {
        retValue = XHEAACENCLIB_RETURN_ERROR_BD_CONFIG;
      } else {
        memset((*phBitDistribution)->elements, 0, sizeof(XHEAACENCLIB_BITDISTRIBUTION_ELEMENT) * (*phBitDistribution)->numberElement);
      }
    }
  }

  if (!isError(retValue)) {
    retValue = bitDistribution_init(*phBitDistribution, bitDistributionConfig);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_GetThresholds(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int *maxNumBitsToUse, int *comfortableNumBitsToUse) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int internMaxNumBitsToUse = 0;
  int internComfortableNumBitsToUse = 0;

  if (maxNumBitsToUse == NULL || comfortableNumBitsToUse == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = bitDistribution_GetThresholds(hBitDistribution, elementID, &internMaxNumBitsToUse, &internComfortableNumBitsToUse);
  }

  if (!isError(retValue)) {
    *maxNumBitsToUse = internMaxNumBitsToUse;
    *comfortableNumBitsToUse = internComfortableNumBitsToUse;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int elementID, int nUseBits) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hBitDistribution == NULL || (hBitDistribution->elements == NULL && hBitDistribution->numberElement > 0)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (elementID < 0 || elementID >= hBitDistribution->numberElement) {
    retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_ELEMENTID;
  } else if (hBitDistribution->actFrameComfortableNumBitsToUse < 0 || hBitDistribution->actFrameMaxNumBitsToUse < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_CALLING_SEQUENCE;
  }

  if (!isError(retValue)) {
    hBitDistribution->elements[elementID].actFrameUsed = nUseBits;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_GetUsedBitsActFrame(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution, int *pnUsedBits) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hBitDistribution == NULL || (hBitDistribution->elements == NULL && hBitDistribution->numberElement > 0)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hBitDistribution->actFrameComfortableNumBitsToUse < 0 || hBitDistribution->actFrameMaxNumBitsToUse < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_BD_WRONG_CALLING_SEQUENCE;
  }

  if (!isError(retValue)) {
    if (pnUsedBits != NULL) {
      *pnUsedBits = 0;
      for (i = 0; i < hBitDistribution->numberElement; i++) {
        if (hBitDistribution->elements[i].actFrameUsed > 0) {
          *pnUsedBits += hBitDistribution->elements[i].actFrameUsed;
        }
      }
    }
  }

  if (!isError(retValue)) {
    for (i = 0; i < hBitDistribution->numberElement; i++) {
      if (hBitDistribution->elements[i].ignoreForAverage > 0) {
        hBitDistribution->elements[i].ignoreForAverage--;
      } else {
        float averageFactor = hBitDistribution->elements[i].averageFactor;
        float actFrameUsed = (float)(hBitDistribution->elements[i].actFrameUsed);
        float average = hBitDistribution->elements[i].average;
        if (hBitDistribution->elements[i].average < 0.0f) {
          averageFactor = 1.0f;
        }
        if (actFrameUsed < 0.0f) {
          actFrameUsed = 0.0f;
        }
        if (average < 0.0f) {
          average = 0.0f;
        }
        hBitDistribution->elements[i].average = (average * (1.0f - averageFactor)) + (actFrameUsed * averageFactor);
      }
    }
  }

  if (!isError(retValue)) {
    for (i = 0; i < hBitDistribution->numberElement; i++) {
      hBitDistribution->elements[i].actFrameUsed = -1;
      hBitDistribution->elements[i].actFrameRequest = -1;
      hBitDistribution->elements[i].actFrameMinimum = -1;
    }
    hBitDistribution->actFrameMaxNumBitsToUse = -1;
    hBitDistribution->actFrameComfortableNumBitsToUse = -1;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_Delete(XHEAACENCLIB_BITDISTRIBUTION_HANDLE const hBitDistribution) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hBitDistribution != NULL) {
    if (hBitDistribution->elements != NULL) {
      iisFree(hBitDistribution->elements);
      hBitDistribution->elements = NULL;
    }
    iisFree(hBitDistribution);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_PreEncode(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_BD_DATA *const bitDistribution,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_APR_BITRESMODE const bitResMode,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN retValueApr = AUDIOPREROLLLIB_NO_ERROR;
  int maxNumBitsToUse = 0;
  int comfortableNumBitsToUse = 0;
  int nMinRequestedBits = 0;
  int nComfortableRequestedBits = 0;
  int mpsNeededBits = 0;
  int loasNeededBits = 0;
  int adtsNeededBits = -1;
  int usac_indep = 0;

  if (hConfig == NULL || hUsacIndepFlag == NULL || bitDistribution->hBitDistribution == NULL || hAacEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, &usac_indep, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
  }

  if (!isError(retValue)) {
    adtsNeededBits = -1;
    if (hConfig->transportFormat == TT_ADTS) {
      adtsNeededBits = 56;
    }
    if (hConfig->transportFormat == TT_ADTSCRC) {
      adtsNeededBits = 72;
    }
    if (adtsNeededBits >= 0) {
      retValue = bitDistribution_SetRequestThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_ADTS, adtsNeededBits, adtsNeededBits);
    }
  }

  if (!isError(retValue)) {
    if (hMpegsEnc != NULL) {
      if (!isError(retValue)) {
        retValue = iisxHEAACEncLibMpsThisExtPayloadSize(hMpegsEnc, &mpsNeededBits);
      }
      if (!isError(retValue)) {
        retValue = bitDistribution_SetRequestThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_MPS, mpsNeededBits, mpsNeededBits);
      }
    }
  }

  if (!isError(retValue)) {
    if (hLoaswriter != NULL) {
      if (!isError(retValue)) {
        loasNeededBits = IIS_LoasWriter_CountBitDemandHeader(hLoaswriter, 6144 * hConfig->nInChannels, 2);
      }
      if (!isError(retValue)) {
        retValue = bitDistribution_SetRequestThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_LOAS, loasNeededBits, loasNeededBits);
      }
    }
  }

  if (!isError(retValue)) {
    if (hAudioPreRoll != NULL) {
      if (bitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT) {
        nMinRequestedBits = 0;
        nComfortableRequestedBits = 0;
      } else {
        int isSyncFrame = 0;
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
        if (!isError(retValue) && isSyncFrame) {
          retValueApr = iisAudioPreRollLibRequestedAUSizeThisFrame(hAudioPreRoll, &nMinRequestedBits, &nComfortableRequestedBits);
          if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
            retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
          }
        }
      }
      if (!isError(retValue)) {
        retValue = addExtensionPayloadOverhead(&nMinRequestedBits, 0, 0, 0);
      }

      if (!isError(retValue)) {
        retValue = addExtensionPayloadOverhead(&nComfortableRequestedBits, 0, 0, 0);
      }
      if (!isError(retValue)) {
        retValue = bitDistribution_SetRequestThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_APR, nMinRequestedBits, nComfortableRequestedBits);
      }
    }
  }

  if (!isError(retValue)) {
    if (hAudioPreRoll != NULL && !isVbr(hConfig->bitrateMode)) {
      XHEAAC_AACENC_IPF_STATE ipfState = XHEAAC_AACENC_IPF_STATE_NO;

      retValue = iisxHEAACEncLib_getIpfState(hUsacIndepFlag, hAudioPreRoll, &ipfState);

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLibAacEncGetBitsToUseForExternalData(hAacEnc, &maxNumBitsToUse, &comfortableNumBitsToUse, usac_indep, ipfState);
      }
    }
  }

  if (!isError(retValue)) {
    comfortableNumBitsToUse = max(comfortableNumBitsToUse, maxNumBitsToUse);
    retValue = bitDistribution_SetThisFrameBitAvailibility(bitDistribution->hBitDistribution, maxNumBitsToUse, comfortableNumBitsToUse);
  }

  if (!isError(retValue)) {
    if (adtsNeededBits >= 0) {
      retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_ADTS, adtsNeededBits);
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_bitDistribution_FillConfig(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_BD_DATA *const bitDistribution,
    int const nTrashAUs,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrcEnc,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_BITDISTRIBUTION_CONFIG_HANDLE const bdConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int actID = 0;

  if (hConfig == NULL || bdConfig == NULL || bdConfig->configElement == NULL || bitDistribution == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hSbrEnc != NULL) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_SBR = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = 0;
        actID++;
      }
    } else {
      bitDistribution->elementID_SBR = -1;
    }
  }

  if (!isError(retValue)) {
    if (hMpegsEnc != NULL) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_MPS = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = 0;
        actID++;
      }
    } else {
      bitDistribution->elementID_MPS = -1;
    }
  }

  if (!isError(retValue)) {
    if (hLoaswriter != NULL) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_LOAS = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = nTrashAUs;
        actID++;
      }
    } else {
      bitDistribution->elementID_LOAS = -1;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->transportFormat == TT_ADTS || hConfig->transportFormat == TT_ADTSCRC) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_ADTS = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = 0;
        actID++;
      }
    } else {
      bitDistribution->elementID_ADTS = -1;
    }
  }

  if (!isError(retValue)) {
    if (hDrcEnc != NULL && (hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF)) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_DRC = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = 0;
        actID++;
      }
    } else {
      bitDistribution->elementID_DRC = -1;
    }
  }

  if (!isError(retValue)) {
    if (hAudioPreRoll != NULL) {
      if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
        retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
      } else {
        bitDistribution->elementID_APR = actID;
        bdConfig->configElement[actID].averageFactor = -1.0f;
        bdConfig->configElement[actID].elementAverage = -1.0f;
        bdConfig->configElement[actID].ignoreForAverage = 0;
        actID++;
      }
    } else {
      bitDistribution->elementID_APR = -1;
    }
  }

  if (!isError(retValue)) {
    if (actID > XHEAACENCLIB_MAX_BITDISTRIBUTION) {
      retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_MAX_SIZE_OF_BD;
    }
  }

  if (!isError(retValue)) {
    bdConfig->numberElements = actID;
  }

  return retValue;
}
