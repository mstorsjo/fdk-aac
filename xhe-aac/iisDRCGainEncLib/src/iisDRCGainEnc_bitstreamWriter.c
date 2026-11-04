
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
#include <math.h>
#include "iisDRCGainEnc_bitstreamWriter.h"
#include "iisDRCGainEnc_tables.h"

typedef enum {
  DRCCUSTOMCHARACTERISTIC_SIDE_LEFT = 0,
  DRCCUSTOMCHARACTERISTIC_SIDE_RIGHT = 1
} DRCCUSTOMCHARACTERISTIC_SIDE;

static IISDRCGAINENC_RETURN
mapMethodValue(const IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION methodDefinition,
               const float methodValue,
               int *pEncVal,
               int *pSizeBits) {
  switch (methodDefinition) {
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_UNKNOWN_OTHER:
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_PROGRAM_LOUDNESS:
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_ANCHOR_LOUDNESS:
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MAX_OF_LOUDNESS_RANGE:
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MOMENTARY_LOUDNESS_MAX:
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_SHORT_TERM_LOUDNESS_MAX:
      *pEncVal = roundInt(4.f * (methodValue + 57.75f));
      if (*pEncVal > 0x0FF) {
        *pEncVal = 0x0FF;
      } else if (*pEncVal < 0) {
        *pEncVal = 0;
      }
      *pSizeBits = 8;
      break;
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_LOUDNESS_RANGE:
      if (methodValue < 0.f) {
        *pEncVal = 0;
      } else if (methodValue <= 32.f) {
        *pEncVal = roundInt(4.f * methodValue);
      } else if (methodValue <= 70.f) {
        *pEncVal = roundInt(2.f * (methodValue - 32.f)) + 128;
      } else if (methodValue < 121.f) {
        *pEncVal = roundInt((methodValue - 70.f)) + 204;
      } else {
        *pEncVal = 255;
      }
      *pSizeBits = 8;
      break;
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_MIXING_LEVEL:
      *pEncVal = roundInt(methodValue - 80.f);
      if (*pEncVal > 0x1F) {
        *pEncVal = 0x1F;
      } else if (*pEncVal < 0) {
        *pEncVal = 0;
      }
      *pSizeBits = 5;
      break;
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_ROOM_TYPE:
      *pEncVal = roundInt(methodValue);
      if (*pEncVal > 0x2) {
        *pEncVal = 0x2;
      } else if (*pEncVal < 0) {
        *pEncVal = 0;
      }
      *pSizeBits = 2;
      break;
    case IISDRCGAINENC_LOUDNESS_METHOD_DEFINITION_SHORT_TERM_LOUDNESS:
      *pEncVal = roundInt(2.f * (methodValue + 116.f));
      if (*pEncVal > 0x0FF) {
        *pEncVal = 0x0FF;
      } else if (*pEncVal < 0) {
        *pEncVal = 0;
      }
      *pSizeBits = 8;
      break;
    default:
      return IISDRCGAINENC_RETURN_ERROR_UNKNOWN_METHOD_DEF;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapPeakValue(const float peakValue,
             int *pEncVal,
             int *pSizeBits) {
  *pEncVal = roundInt(32.f * (20.f - peakValue));

  if (*pEncVal > 0x0FFF) {
    *pEncVal = 0x0FFF;
  } else if (*pEncVal < 0x1) {
    *pEncVal = 0x1;
  }

  *pSizeBits = 12;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapDuckingModifications(DuckingModifications *duckingModifications,
                        int *pSigma,
                        int *pMu) {
  if (duckingModifications->duckingScalingPresent == 1) {
    if (duckingModifications->duckingScaling > 1.f) {
      *pSigma = 0;
      *pMu = roundInt(8.f * (duckingModifications->duckingScaling - 1.f) - 1.f);
    } else {
      *pSigma = 1;
      *pMu = roundInt((-8.f) * (duckingModifications->duckingScaling - 1.f) - 1.f);
    }

    if (*pMu > 7) {
      *pMu = 7;
    } else if (*pMu < 0) {
      *pMu = 0;
    }
  } else {
    *pSigma = 0;
    *pMu = 0;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeLoudnessMeasure(wobitbufHandle hBitstream,
                     LoudnessMeasure *pLoudnessMeasure) {
  int wobitbufErr = 0, encVal = 0, sizeBits = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessMeasure->methodDefinition, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  retVal = mapMethodValue(pLoudnessMeasure->methodDefinition,
                          pLoudnessMeasure->methodValue,
                          &encVal,
                          &sizeBits);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessMeasure->measurementSystem, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessMeasure->reliability, 2);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeLoudnessInfo(wobitbufHandle hBitstream,
                  const int version,
                  LoudnessInfo *pLoudnessInfo) {
  int wobitbufErr = 0, k = 0, encVal = 0, sizeBits = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->drcSetId, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (version >= 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->eqSetId, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->downmixId, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->samplePeakLevelPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  if (pLoudnessInfo->samplePeakLevelPresent) {
    retVal = mapPeakValue(pLoudnessInfo->samplePeakLevel, &encVal, &sizeBits);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->truePeakLevelPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  if (pLoudnessInfo->truePeakLevelPresent) {
    retVal = mapPeakValue(pLoudnessInfo->truePeakMeasure.truePeakLevel, &encVal, &sizeBits);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->truePeakMeasure.truePeakLevelMeasurementSystem, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->truePeakMeasure.truePeakLevelReliability, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfo->measurementCount, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  for (k = 0; k < pLoudnessInfo->measurementCount; k++) {
    retVal = writeLoudnessMeasure(hBitstream, &(pLoudnessInfo->pLoudnessMeasure[k]));
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeLoudnessInfoSetExtension(wobitbufHandle hBitstream,
                              LoudnessInfoSetExtension *pLoudnessInfoSetExtension) {
  int wobitbufErr = 0, k, m, version = 1, bitCount = 0;
  int extSizeBits = 0, bitSizeLen = 0, bitSize = 0, extBitSize = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  unsigned char bitstreamBufferTmp[DRC_PAYLOAD_MAX_SIZE_BYTES] = {0};
  wobitbuf bs;
  wobitbufHandle hBitstreamTmp = &bs;
  wobitbuf_Init(hBitstreamTmp, bitstreamBufferTmp, DRC_PAYLOAD_MAX_SIZE_BITS, bitCount);

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfoSetExtension->pLoudnessInfoSetExtType[0], 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  k = 0;
  while (pLoudnessInfoSetExtension->pLoudnessInfoSetExtType[k] != IISDRCGAINENC_UNIDRCLOUDEXT_TERM) {
    switch (pLoudnessInfoSetExtension->pLoudnessInfoSetExtType[k]) {
      case IISDRCGAINENC_UNIDRCLOUDEXT_EQ:
        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pLoudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount, 6);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pLoudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count, 6);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        for (m = 0; m < pLoudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1AlbumCount; m++) {
          retVal = writeLoudnessInfo(hBitstreamTmp, version, &(pLoudnessInfoSetExtension->loudnessInfoSetExtEq.pLoudnessInfoV1Album[m]));
          if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
        }
        for (m = 0; m < pLoudnessInfoSetExtension->loudnessInfoSetExtEq.loudnessInfoV1Count; m++) {
          retVal = writeLoudnessInfo(hBitstreamTmp, version, &(pLoudnessInfoSetExtension->loudnessInfoSetExtEq.pLoudnessInfoV1[m]));
          if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
        }
        break;
      default:
        for (m = 0; m < pLoudnessInfoSetExtension->pExtBitSize[k]; m++) {
          wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, 0, 1);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }
        break;
    }

    extBitSize = wobitbuf_GetBitsWritten(hBitstreamTmp);
    pLoudnessInfoSetExtension->pExtBitSize[k] = extBitSize;
    bitSize = pLoudnessInfoSetExtension->pExtBitSize[k] - 1;
    extSizeBits = (int)(log((float)bitSize) / log(2.f)) + 1;

    if (extSizeBits < 4) {
      extSizeBits = 4;
    }

    bitSizeLen = extSizeBits - 4;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitSizeLen, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitSize, extSizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBytes(hBitstream, bitstreamBufferTmp, extBitSize / 8);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitstreamBufferTmp[extBitSize / 8] >> (8 - extBitSize % 8), extBitSize % 8);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    k++;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfoSetExtension->pLoudnessInfoSetExtType[k], 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeChannelLayoutDefaultSyntax(wobitbufHandle hBitstream,
                                ChannelLayout *pChannelLayout) {
  int wobitbufErr = 0, k;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pChannelLayout->layoutSignalingPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pChannelLayout->layoutSignalingPresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pChannelLayout->definedLayout, 8);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pChannelLayout->definedLayout == 0) {
      for (k = 0; k < pChannelLayout->baseChannelCount; k++) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pChannelLayout->pSpeakerPosition[k], 7);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapDownmixCoefficientV1(const float downmixCoefficientLin,
                        const float downmixOffset,
                        int *pEncVal) {
  int k;
  float downmixCoefficientDb;

  downmixCoefficientDb = 20.f * (float)log10((double)downmixCoefficientLin) - downmixOffset;

  if (downmixCoefficientDb > bsDownmixCoefficientV1Table[0]) {
    *pEncVal = 0;
  } else {
    k = 1;
    while (k <= 31) {
      if (downmixCoefficientDb > bsDownmixCoefficientV1Table[k] || k == 31) {
        break;
      }
      k++;
    }
    if (downmixCoefficientDb - bsDownmixCoefficientV1Table[k] > bsDownmixCoefficientV1Table[k - 1] - downmixCoefficientDb) {
      *pEncVal = k - 1;
    } else {
      *pEncVal = k;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDownmixCoefficientV1(wobitbufHandle hBitstream,
                          const float *pDownmixCoefficient,
                          const int baseChannelCount,
                          const int targetChannelCount) {
  int wobitbufErr = 0;
  int k, m, bsDownmixOffset = 0, encVal;
  float downmixOffset[3];
  float a;
  float quantErrLin, quantErrMin;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  a = 20.f * (float)log10((float)targetChannelCount / (float)baseChannelCount);
  downmixOffset[0] = 0.f;
  downmixOffset[1] = 0.5f * roundInt(a);
  downmixOffset[2] = 0.5f * roundInt(2 * a);

  quantErrMin = 1000.f;

  for (k = 0; k < 3; k++) {
    quantErrLin = 0.f;
    for (m = 0; m < targetChannelCount * baseChannelCount; m++) {
      retVal = mapDownmixCoefficientV1(pDownmixCoefficient[m], downmixOffset[k], &encVal);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

      if (pDownmixCoefficient[m] > 0.f) {
        quantErrLin += (float)fabs(pDownmixCoefficient[m] - pow(10, (bsDownmixCoefficientV1Table[encVal] + downmixOffset[k]) * 0.05f));
      }
    }
    if (quantErrMin > quantErrLin) {
      quantErrMin = quantErrLin;
      bsDownmixOffset = k;
    }
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, bsDownmixOffset, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  for (k = 0; k < targetChannelCount * baseChannelCount; k++) {
    retVal = mapDownmixCoefficientV1(pDownmixCoefficient[k], downmixOffset[bsDownmixOffset], &encVal);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, 5);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapDownmixCoefficient(const float downmixCoefficientLin,
                      const int bLfeChannel,
                      int *pEncVal) {
  int k;
  float downmixCoefficientDb;
  const float *bsDownmixCoefficientTableTmp = NULL;

  if (bLfeChannel == 1) {
    bsDownmixCoefficientTableTmp = bsDownmixCoefficientLfeTable;
  } else {
    bsDownmixCoefficientTableTmp = bsDownmixCoefficientTable;
  }

  downmixCoefficientDb = 20.f * (float)log10((double)downmixCoefficientLin);

  if (downmixCoefficientDb > bsDownmixCoefficientTableTmp[0]) {
    *pEncVal = 0;
  } else {
    k = 1;
    while (k <= 15) {
      if (downmixCoefficientDb > bsDownmixCoefficientTableTmp[k] || k == 15) {
        break;
      }
      k++;
    }
    if (downmixCoefficientDb - bsDownmixCoefficientTableTmp[k] > bsDownmixCoefficientTableTmp[k - 1] - downmixCoefficientDb) {
      *pEncVal = k - 1;
    } else {
      *pEncVal = k;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDownmixInstructions(wobitbufHandle hBitstream,
                         DownmixInstructions *pDownmixInstructions,
                         const int version,
                         const int baseChannelCount) {
  int wobitbufErr = 0, k, encVal;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDownmixInstructions->downmixId, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDownmixInstructions->targetChannelCount, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDownmixInstructions->targetLayout, 8);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDownmixInstructions->downmixCoefficientsPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pDownmixInstructions->downmixCoefficientsPresent) {
    if (version == 1) {
      retVal = writeDownmixCoefficientV1(hBitstream,
                                         pDownmixInstructions->pDownmixCoefficient,
                                         baseChannelCount,
                                         pDownmixInstructions->targetChannelCount);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    } else {
      for (k = 0; k < pDownmixInstructions->targetChannelCount * baseChannelCount; k++) {
        retVal = mapDownmixCoefficient(pDownmixInstructions->pDownmixCoefficient[k], pDownmixInstructions->pLfeChannel[k], &encVal);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

        wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, 4);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDrcCoefficientsBasic(wobitbufHandle hBitstream,
                          DrcCoefficientsBasic *pDrcCoefficientsBasic) {
  int wobitbufErr = 0;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsBasic->drcLocation, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsBasic->drcCharacteristic, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDrcInstructionsBasic(wobitbufHandle hBitstream,
                          DrcInstructionsBasic *pDrcInstructionsBasic) {
  int wobitbufErr = 0, k;
  int bsLimiterPeakTarget;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetId, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcLocation, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->downmixId, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->additionalDownmixIdPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  if (pDrcInstructionsBasic->additionalDownmixIdPresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->additionalDownmixIdCount, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    for (k = 0; k < pDrcInstructionsBasic->additionalDownmixIdCount; k++) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->pAdditionalDownmixId[k], 7);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  } else {
    pDrcInstructionsBasic->additionalDownmixIdCount = 0;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetEffect, 16);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if ((pDrcInstructionsBasic->drcSetEffect & (IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER | IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF)) == 0) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->limiterPeakTargetPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    if (pDrcInstructionsBasic->limiterPeakTargetPresent) {
      bsLimiterPeakTarget = roundInt(8.f * pDrcInstructionsBasic->limiterPeakTarget);
      if (bsLimiterPeakTarget > 0xFF) {
        bsLimiterPeakTarget = 0xFF;
      } else if (bsLimiterPeakTarget < 0) {
        bsLimiterPeakTarget = 0;
      }

      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsLimiterPeakTarget, 8);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetTargetLoudnessPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pDrcInstructionsBasic->drcSetTargetLoudnessPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetTargetLoudnessValueUpper + 63, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetTargetLoudnessValueLowerPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcInstructionsBasic->drcSetTargetLoudnessValueLowerPresent == 1) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsBasic->drcSetTargetLoudnessValueLower + 63, 6);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeCustomDrcCharacterisitc(wobitbufHandle hBitstream,
                             CustomDrcCharacteristic *pCustomDrcCharacteristic,
                             const DRCCUSTOMCHARACTERISTIC_SIDE side) {
  int wobitbufErr = 0, k, bsGain, bsIoRatio, bsExp, bsNodeLevelDelta, bsNodeGain;
  float bsNodeLevelPrevious = DRC_INPUT_CURVE_REFERENCE_LOUDNESS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pCustomDrcCharacteristic->characteristicFormat, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pCustomDrcCharacteristic->characteristicFormat == 0) {
    if (side == DRCCUSTOMCHARACTERISTIC_SIDE_LEFT) {
      bsGain = pCustomDrcCharacteristic->gainDb;
    } else {
      bsGain = (-1) * pCustomDrcCharacteristic->gainDb;
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bsGain, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    bsIoRatio = roundInt(((float)pCustomDrcCharacteristic->ioRatio - 0.05f) / 0.15f);
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bsIoRatio, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    bsExp = roundInt((pCustomDrcCharacteristic->exp - 1) * 0.5f);
    if (bsExp >= 15) {
      bsExp = 15;
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bsExp, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pCustomDrcCharacteristic->flipSign, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  } else {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pCustomDrcCharacteristic->charNodeCount - 1, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    for (k = 0; k < pCustomDrcCharacteristic->charNodeCount; k++) {
      bsNodeLevelDelta = roundInt(fabs(pCustomDrcCharacteristic->pNodeLevel[k] - bsNodeLevelPrevious)) - 1;
      if (bsNodeLevelDelta < 0) bsNodeLevelDelta = 0;
      if (bsNodeLevelDelta > 31) bsNodeLevelDelta = 31;
      if (side == DRCCUSTOMCHARACTERISTIC_SIDE_LEFT) {
        bsNodeLevelPrevious = bsNodeLevelPrevious - (bsNodeLevelDelta + 1);
      } else {
        bsNodeLevelPrevious = bsNodeLevelPrevious + (bsNodeLevelDelta + 1);
      }
      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsNodeLevelDelta, 5);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      bsNodeGain = roundInt((pCustomDrcCharacteristic->pNodeGain[k] + 64.f) * 2.f);
      if (bsNodeGain < 0) bsNodeGain = 0;
      if (bsNodeGain > 255) bsNodeGain = 255;
      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsNodeGain, 8);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeShapeFilterParams(wobitbufHandle hBitstream,
                       ShapeFilterParams *pShapeFilterParams) {
  int wobitbufErr = 0;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfCutFilterPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pShapeFilterParams->lfCutFilterPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfCutParams.cornerFreqIndex, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfCutParams.filterStrengthIndex, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfBoostFilterPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pShapeFilterParams->lfBoostFilterPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfBoostParams.cornerFreqIndex, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->lfBoostParams.filterStrengthIndex, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfCutFilterPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pShapeFilterParams->hfCutFilterPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfCutParams.cornerFreqIndex, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfCutParams.filterStrengthIndex, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfBoostFilterPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pShapeFilterParams->hfBoostFilterPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfBoostParams.cornerFreqIndex, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pShapeFilterParams->hfBoostParams.filterStrengthIndex, 2);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeGainSequenceParams(wobitbufHandle hBitstream,
                        GainSequenceParams *pGainSequenceParams,
                        int *pGainSequenceIndexPrev,
                        const int bandCount,
                        const int drcBandType,
                        const int version) {
  int wobitbufErr = 0, k;
  int indexPresent;

  if (version == 1) {
    for (k = 0; k < bandCount; k++) {
      if (pGainSequenceParams[k].gainSequenceIndex == *pGainSequenceIndexPrev + 1) {
        indexPresent = 0;
      } else {
        indexPresent = 1;
      }

      wobitbufErr = wobitbuf_WriteBits(hBitstream, indexPresent, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      if (indexPresent == 1) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].gainSequenceIndex, 6);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        *pGainSequenceIndexPrev = pGainSequenceParams[k].gainSequenceIndex;
      } else {
        (*pGainSequenceIndexPrev) = (*pGainSequenceIndexPrev) + 1;
      }

      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristicPresent, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      if (pGainSequenceParams[k].drcCharacteristicPresent) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristicFormatIsCICP, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainSequenceParams[k].drcCharacteristicFormatIsCICP == 1) {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristic, 7);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        } else {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristicLeftIndex, 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristicRightIndex, 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }
      }
    }
  } else {
    for (k = 0; k < bandCount; k++) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].drcCharacteristic, 7);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }
  for (k = 1; k < bandCount; k++) {
    if (drcBandType == 1) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].crossoverFreqIndex, 4);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    } else {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSequenceParams[k].startSubBandIndex, 10);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeGainSetParams(wobitbufHandle hBitstream,
                   GainSetParams *pGainSetParams,
                   int *pGainSequenceIndexPrev,
                   const int version) {
  int wobitbufErr = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->gainCodingProfile, 2);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->gainInterpolationType, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->fullFrame, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->timeAlignment, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->timeDeltaMinPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pGainSetParams->timeDeltaMinPresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->timeDeltaMin - 1, 11);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  if (pGainSetParams->gainCodingProfile != IISDRCGAINENC_GAINCODINGPROFILE_CONSTANT) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->bandCount, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pGainSetParams->bandCount > 1) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainSetParams->drcBandType, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }

    retVal = writeGainSequenceParams(hBitstream, pGainSetParams->pGainSequenceParams, pGainSequenceIndexPrev, pGainSetParams->bandCount, pGainSetParams->drcBandType, version);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDrcCoefficientsUniDrc(wobitbufHandle hBitstream,
                           DrcCoefficientsUniDrc *pDrcCoefficientsUniDrc,
                           const int version) {
  int wobitbufErr = 0, k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;
  int gainSequenceIndexPrev = -1;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcLocation, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcFrameSizePresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pDrcCoefficientsUniDrc->drcFrameSizePresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcFrameSize - 1, 15);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  if (version == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcCharacteristicLeftPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    if (pDrcCoefficientsUniDrc->drcCharacteristicLeftPresent) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcCharacteristicLeftCount, 4);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      for (k = 0; k < pDrcCoefficientsUniDrc->drcCharacteristicLeftCount; k++) {
        retVal = writeCustomDrcCharacterisitc(hBitstream, &pDrcCoefficientsUniDrc->pCustomDrcCharacteristicLeft[k], DRCCUSTOMCHARACTERISTIC_SIDE_LEFT);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcCharacteristicRightPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcCoefficientsUniDrc->drcCharacteristicRightPresent) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->drcCharacteristicRightCount, 4);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      for (k = 0; k < pDrcCoefficientsUniDrc->drcCharacteristicRightCount; k++) {
        retVal = writeCustomDrcCharacterisitc(hBitstream, &pDrcCoefficientsUniDrc->pCustomDrcCharacteristicRight[k], DRCCUSTOMCHARACTERISTIC_SIDE_RIGHT);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->shapeFiltersPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcCoefficientsUniDrc->shapeFiltersPresent) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->shapeFilterCount, 4);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      for (k = 0; k < pDrcCoefficientsUniDrc->shapeFilterCount; k++) {
        retVal = writeShapeFilterParams(hBitstream, &pDrcCoefficientsUniDrc->pShapeFilterParams[k]);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
      }
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->gainSequenceCount, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->gainSetCount, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    for (k = 0; k < pDrcCoefficientsUniDrc->gainSetCount; k++) {
      retVal = writeGainSetParams(hBitstream, &(pDrcCoefficientsUniDrc->pGainSetParams[k]), &gainSequenceIndexPrev, version);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }
  } else {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcCoefficientsUniDrc->gainSetCount, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    for (k = 0; k < pDrcCoefficientsUniDrc->gainSetCount; k++) {
      retVal = writeGainSetParams(hBitstream, &(pDrcCoefficientsUniDrc->pGainSetParams[k]), &gainSequenceIndexPrev, version);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
getDrcSetComplexityLevel(DrcInstructionsUniDrc *pDrcInstructionsUniDrc) {
  pDrcInstructionsUniDrc->drcSetComplexityLevel = 2;

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeGainModifications(wobitbufHandle hBitstream,
                       GainModifications *pGainModifications,
                       const int drcChannelGroupCount,
                       const int *pBandCountForChannelGroup,
                       const int version) {
  int wobitbufErr = 0, k, m;
  int bsAttenuationScaling, bsAmplificationScaling, sigma, mu;

  for (k = 0; k < drcChannelGroupCount; k++) {
    if (version == 0) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pGainScalingPresent[0], 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      if (pGainModifications[k].pGainScalingPresent[0] == 1) {
        bsAttenuationScaling = roundInt(8.f * pGainModifications[k].pAttenuationScaling[0]);
        wobitbufErr = wobitbuf_WriteBits(hBitstream, bsAttenuationScaling, 4);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        bsAmplificationScaling = roundInt(8.f * pGainModifications[k].pAmplificationScaling[0]);
        wobitbufErr = wobitbuf_WriteBits(hBitstream, bsAmplificationScaling, 4);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }

      wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pGainOffsetPresent[0], 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      if (pGainModifications[k].pGainOffsetPresent[0] == 1) {
        if (pGainModifications[k].pGainOffset[0] > 0.f) {
          sigma = 0;
          mu = roundInt(4.f * pGainModifications[k].pGainOffset[0] - 1.f);
          if (mu > 31) {
            mu = 31;
          } else if (mu < 0) {
            mu = 0;
          }
        } else {
          sigma = 1;
          mu = roundInt((-4.f) * pGainModifications[k].pGainOffset[0] - 1.f);
          if (mu > 31) {
            mu = 31;
          } else if (mu < 0) {
            mu = 0;
          }
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstream, sigma, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        wobitbufErr = wobitbuf_WriteBits(hBitstream, mu, 5);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    } else if (version == 1) {
      for (m = 0; m < pBandCountForChannelGroup[k]; m++) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pTargetCharacteristicLeftPresent[m], 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainModifications[k].pTargetCharacteristicLeftPresent[m]) {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pTargetCharacteristicLeftIndex[m], 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pTargetCharacteristicRightPresent[m], 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainModifications[k].pTargetCharacteristicLeftPresent[m]) {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pTargetCharacteristicRightIndex[m], 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pGainScalingPresent[m], 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainModifications[k].pGainScalingPresent[m]) {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, roundInt(8.f * pGainModifications[k].pAttenuationScaling[m]), 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
          wobitbufErr = wobitbuf_WriteBits(hBitstream, roundInt(8.f * pGainModifications[k].pAmplificationScaling[m]), 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].pGainOffsetPresent[m], 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainModifications[k].pGainOffsetPresent[m] == 1) {
          if (pGainModifications[k].pGainOffset[m] > 0.f) {
            sigma = 0;
            mu = roundInt(4.f * pGainModifications[k].pGainOffset[m] - 1.f);
            if (mu > 31) {
              mu = 31;
            } else if (mu < 0) {
              mu = 0;
            }
          } else {
            sigma = 1;
            mu = roundInt((-4.f) * pGainModifications[k].pGainOffset[m] - 1.f);
            if (mu > 31) {
              mu = 31;
            } else if (mu < 0) {
              mu = 0;
            }
          }

          wobitbufErr = wobitbuf_WriteBits(hBitstream, sigma, 1);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
          wobitbufErr = wobitbuf_WriteBits(hBitstream, mu, 5);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }
      }

      if (pBandCountForChannelGroup[k] == 1) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].shapeFilterPresent, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pGainModifications[k].shapeFilterPresent == 1) {
          wobitbufErr = wobitbuf_WriteBits(hBitstream, pGainModifications[k].shapeFilterIndex, 4);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }
      }
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeDrcInstructionsUniDrc(wobitbufHandle hBitstream,
                           DrcInstructionsUniDrc *pDrcInstructionsUniDrc,
                           const int baseChannelCount,
                           const int version) {
  int wobitbufErr = 0, k;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;
  int bsLimiterPeakTarget, bsSequenceIndex, mu, sigma, repeatParametersCount, repeatGainSetIndexCount;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetId, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (version == 1) {
    retVal = getDrcSetComplexityLevel(pDrcInstructionsUniDrc);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetComplexityLevel, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcLocation, 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (version == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->downmixIdPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  } else {
    pDrcInstructionsUniDrc->downmixIdPresent = 1;
  }

  if (pDrcInstructionsUniDrc->downmixIdPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->downmixId, 7);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (version == 1) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcApplyToDownmix, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->additionalDownmixIdPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcInstructionsUniDrc->additionalDownmixIdPresent) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->additionalDownmixIdCount, 3);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      for (k = 0; k < pDrcInstructionsUniDrc->additionalDownmixIdCount; k++) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->pAdditionalDownmixId[k], 7);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    } else {
      pDrcInstructionsUniDrc->additionalDownmixIdCount = 0;
    }
  } else {
    pDrcInstructionsUniDrc->downmixId = 0;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetEffect, 16);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if ((pDrcInstructionsUniDrc->drcSetEffect & (IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER | IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF)) == 0) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->limiterPeakTargetPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcInstructionsUniDrc->limiterPeakTargetPresent) {
      bsLimiterPeakTarget = roundInt((-1.f) * 8.f * pDrcInstructionsUniDrc->limiterPeakTarget);
      if (bsLimiterPeakTarget > 255) {
        bsLimiterPeakTarget = 255;
      } else if (bsLimiterPeakTarget < 0) {
        bsLimiterPeakTarget = 0;
      }

      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsLimiterPeakTarget, 8);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetTargetLoudnessPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  if (pDrcInstructionsUniDrc->drcSetTargetLoudnessPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetTargetLoudnessValueUpper + 63, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetTargetLoudnessValueLowerPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcInstructionsUniDrc->drcSetTargetLoudnessValueLowerPresent == 1) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->drcSetTargetLoudnessValueLower + 63, 6);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->dependsOnDrcSetPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pDrcInstructionsUniDrc->dependsOnDrcSetPresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->dependsOnDrcSet, 6);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  } else {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->noIndependentUse, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  if (version == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, 0, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  if ((pDrcInstructionsUniDrc->drcSetEffect & (IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_OTHER | IISDRCGAINENC_DRCSETEFFECT_BIT_DUCK_SELF)) != 0) {
    for (k = 0; k < baseChannelCount; k++) {
      sigma = 0;
      mu = 0;
      bsSequenceIndex = pDrcInstructionsUniDrc->pGainSetIndex[k] + 1;
      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsSequenceIndex, 6);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcInstructionsUniDrc->pDuckingModifications[k].duckingScalingPresent, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      if (pDrcInstructionsUniDrc->pDuckingModifications[k].duckingScalingPresent == 1) {
        retVal = mapDuckingModifications(&pDrcInstructionsUniDrc->pDuckingModifications[k], &sigma, &mu);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

        wobitbufErr = wobitbuf_WriteBits(hBitstream, sigma, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        wobitbufErr = wobitbuf_WriteBits(hBitstream, mu, 3);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }

      repeatParametersCount = 0;
      for (; k < baseChannelCount - 1; k++) {
        if ((pDrcInstructionsUniDrc->pDuckingModifications[k].duckingScalingPresent != pDrcInstructionsUniDrc->pDuckingModifications[k + 1].duckingScalingPresent) || (pDrcInstructionsUniDrc->pGainSetIndex[k] != pDrcInstructionsUniDrc->pGainSetIndex[k + 1]) || (repeatParametersCount >= 32)) {
          break;
        }
        if (pDrcInstructionsUniDrc->pDuckingModifications[k + 1].duckingScalingPresent != 0) {
          int nextSigma, nextMu;

          retVal = mapDuckingModifications(&pDrcInstructionsUniDrc->pDuckingModifications[k + 1], &nextSigma, &nextMu);
          if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

          if ((sigma != nextSigma) || (mu != nextMu)) {
            break;
          }
        }
        repeatParametersCount++;
      }

      if (repeatParametersCount > 0) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, 1, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        wobitbufErr = wobitbuf_WriteBits(hBitstream, repeatParametersCount - 1, 5);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      } else {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, 0, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    }
  } else {
    for (k = 0; k < pDrcInstructionsUniDrc->drcChannelCount; k++) {
      repeatGainSetIndexCount = 0;
      bsSequenceIndex = pDrcInstructionsUniDrc->pGainSetIndex[k] + 1;
      wobitbufErr = wobitbuf_WriteBits(hBitstream, bsSequenceIndex, 6);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      for (; k < pDrcInstructionsUniDrc->drcChannelCount - 1; k++) {
        if ((pDrcInstructionsUniDrc->pGainSetIndex[k] != pDrcInstructionsUniDrc->pGainSetIndex[k + 1]) || (repeatGainSetIndexCount >= 32)) {
          break;
        }
        repeatGainSetIndexCount++;
      }

      if (repeatGainSetIndexCount > 0) {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, 1, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        wobitbufErr = wobitbuf_WriteBits(hBitstream, repeatGainSetIndexCount - 1, 5);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      } else {
        wobitbufErr = wobitbuf_WriteBits(hBitstream, 0, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    }

    retVal = writeGainModifications(hBitstream, pDrcInstructionsUniDrc->pGainModifications, pDrcInstructionsUniDrc->drcChannelGroupCount, pDrcInstructionsUniDrc->pBandCountForChannelGroup, version);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
writeUniDrcConfigExtension(wobitbufHandle hBitstream,
                           UniDrcConfigExtension *pUniDrcConfigExtension,
                           const int baseChannelCount) {
  int wobitbufErr = 0, k, m, bitCount = 0, version = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;
  int extSizeBits = 0, bitSizeLen = 0, bitSize = 0, extBitSize = 0;

  unsigned char bitstreamBufferTmp[DRC_PAYLOAD_MAX_SIZE_BYTES] = {0};
  wobitbuf bs;
  wobitbufHandle hBitstreamTmp = &bs;
  wobitbuf_Init(hBitstreamTmp, bitstreamBufferTmp, DRC_PAYLOAD_MAX_SIZE_BITS, bitCount);

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfigExtension->pUniDrcConfigExtType[0], 4);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  k = 0;
  while (pUniDrcConfigExtension->pUniDrcConfigExtType[k] != IISDRCGAINENC_UNIDRCCONFEXT_TERM) {
    switch (pUniDrcConfigExtension->pUniDrcConfigExtType[k]) {
      case IISDRCGAINENC_UNIDRCCONFEXT_V1:
        version = 1;

        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pUniDrcConfigExtension->downmixInstructionsV1Present, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pUniDrcConfigExtension->downmixInstructionsV1Present == 1) {
          wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pUniDrcConfigExtension->downmixInstructionsV1Count, 7);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

          for (m = 0; m < pUniDrcConfigExtension->downmixInstructionsV1Count; m++) {
            retVal = writeDownmixInstructions(hBitstreamTmp, &(pUniDrcConfigExtension->pDownmixInstructionsV1[m]), version, baseChannelCount);
            if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
          }
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pUniDrcConfigExtension->drcCoeffsAndInstructionsUniDrcV1Present, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        if (pUniDrcConfigExtension->drcCoeffsAndInstructionsUniDrcV1Present == 1) {
          wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pUniDrcConfigExtension->drcCoefficientsUniDrcV1Count, 3);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

          for (m = 0; m < pUniDrcConfigExtension->drcCoefficientsUniDrcV1Count; m++) {
            retVal = writeDrcCoefficientsUniDrc(hBitstreamTmp, &(pUniDrcConfigExtension->pDrcCoefficientsUniDrcV1[m]), version);
            if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
          }

          wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, pUniDrcConfigExtension->drcInstructionsUniDrcV1Count, 6);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

          for (m = 0; m < pUniDrcConfigExtension->drcInstructionsUniDrcV1Count; m++) {
            retVal = writeDrcInstructionsUniDrc(hBitstreamTmp, &(pUniDrcConfigExtension->pDrcInstructionsUniDrcV1[m]), baseChannelCount, version);
            if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
          }
        }

        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, 0, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, 0, 1);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

        break;
      default:
        for (m = 0; m < pUniDrcConfigExtension->pExtBitSize[k]; m++) {
          wobitbufErr = wobitbuf_WriteBits(hBitstreamTmp, 0, 1);
          if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
        }
        break;
    }

    extBitSize = wobitbuf_GetBitsWritten(hBitstreamTmp);
    pUniDrcConfigExtension->pExtBitSize[k] = extBitSize;
    bitSize = pUniDrcConfigExtension->pExtBitSize[k] - 1;
    extSizeBits = (int)(log((float)bitSize) / log(2.f)) + 1;

    if (extSizeBits < 4) {
      extSizeBits = 4;
    }

    bitSizeLen = extSizeBits - 4;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitSizeLen, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitSize, extSizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    wobitbufErr = wobitbuf_WriteBytes(hBitstream, bitstreamBufferTmp, extBitSize / 8);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bitstreamBufferTmp[extBitSize / 8] >> (8 - extBitSize % 8), extBitSize % 8);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    k++;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfigExtension->pUniDrcConfigExtType[k], 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapGainInitial(const float gainInitial,
               IISDRCGAINENC_GAINCODINGPROFILE gainCodingProfile,
               float *gainInitialQuantized,
               int *pEncVal,
               int *pSizeBits) {
  int sigma, mu;

  switch (gainCodingProfile) {
    case IISDRCGAINENC_GAINCODINGPROFILE_REGULAR:
      if (gainInitial < 0.f) {
        sigma = 1;
        *pEncVal = sigma << 8;
        mu = roundInt(-8.f * gainInitial);
      } else {
        sigma = 0;
        *pEncVal = 0;
        mu = roundInt(8.f * gainInitial);
      }
      if (mu > 255) {
        mu = 255;
      } else if (mu < 0) {
        mu = 0;
      }
      *gainInitialQuantized = (float)pow(-1, sigma) * (float)mu * 0.125f;
      *pEncVal |= mu;
      *pSizeBits = 9;

      break;
    case IISDRCGAINENC_GAINCODINGPROFILE_FADING:
      if (gainInitial > -0.0625f) {
        *gainInitialQuantized = 0.f;
        *pEncVal = 0;
        *pSizeBits = 1;
      } else {
        *pEncVal = roundInt(-8.f * gainInitial - 1.f);
        if (*pEncVal > 1023) {
          *pEncVal = 1023;
        } else if (*pEncVal < 0) {
          *pEncVal = 0;
        }
        *gainInitialQuantized = -((float)(*pEncVal) + 1.f) * 0.125f;
        *pEncVal |= 1 << 10;
        *pSizeBits = 11;
      }
      break;
    case IISDRCGAINENC_GAINCODINGPROFILE_DUCKING_CLIPPING:
      if (gainInitial > -0.0625f) {
        *gainInitialQuantized = 0.f;
        *pEncVal = 0;
        *pSizeBits = 1;
      } else {
        *pEncVal = roundInt(-8.f * gainInitial - 1.f);
        if (*pEncVal > 255) {
          *pEncVal = 255;
        } else if (*pEncVal < 0) {
          *pEncVal = 0;
        }
        *gainInitialQuantized = -((float)(*pEncVal) + 1.f) * 0.125f;
        *pEncVal |= 1 << 8;
        *pSizeBits = 9;
      }
      break;
    default:
      return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDGAINCODINGPROFILE;
      break;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapSlope(const float slope,
         int *pEncVal,
         int *pSizeBits) {
  IISDRCGAINENC_RETURN returnValue = IISDRCGAINENC_RETURN_NOERROR;
  int k, selectedTableEntry;
  int numSlopeTableEntries = sizeof(bsSlopeTable) / sizeof(SLOPE_TABLE);
  float quantizationBorder;

  selectedTableEntry = numSlopeTableEntries - 1;

  for (k = 0; k < numSlopeTableEntries - 1; k++) {
    quantizationBorder = (bsSlopeTable[k].value + bsSlopeTable[k + 1].value) / 2.0f;
    if (slope < quantizationBorder) {
      selectedTableEntry = k;
      break;
    }
  }

  if ((selectedTableEntry >= 0) && (selectedTableEntry < numSlopeTableEntries)) {
    *pEncVal = bsSlopeTable[selectedTableEntry].encVal;
    *pSizeBits = bsSlopeTable[selectedTableEntry].sizeBits;
  } else {
    returnValue = IISDRCGAINENC_RETURN_ERROR_ILLEGAL_ARRAY_ACCESS;
  }

  return returnValue;
}

static IISDRCGAINENC_RETURN
mapTimeDelta(const int timeDeltaSamples,
             const int deltaTmin,
             const int nNodesMax,
             const int nBitsMax,
             int *pEncVal,
             int *pSizeBits) {
  int timeDelta = roundInt((float)timeDeltaSamples / (float)deltaTmin);
  if (timeDelta < 1) {
    return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDTIMEDELTA;
  } else if (timeDelta == 1) {
    *pEncVal = 0;
    *pSizeBits = 2;
  } else if (timeDelta <= 5) {
    *pEncVal = timeDelta - 2;
    *pEncVal |= 1 << 2;
    *pSizeBits = 4;
  } else if (timeDelta <= 13) {
    *pEncVal = timeDelta - 6;
    *pEncVal |= 2 << 3;
    *pSizeBits = 5;
  } else if (timeDelta <= (2 * nNodesMax - 1)) {
    *pEncVal = timeDelta - 14;
    *pEncVal |= 3 << nBitsMax;
    *pSizeBits = 2 + nBitsMax;
  } else {
    return IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDTIMEDELTA;
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

static IISDRCGAINENC_RETURN
mapGainDelta(const float gainDelta,
             IISDRCGAINENC_GAINCODINGPROFILE gainCodingProfile,
             float *gainDeltaQuantized,
             int *pEncVal,
             int *pSizeBits) {
  IISDRCGAINENC_RETURN returnValue = IISDRCGAINENC_RETURN_NOERROR;
  int k;
  int selectedTableEntry;
  int numGainDeltaTableEntries = 0;
  const GAINDELTA_TABLE *pBsGainDeltaTable = NULL;
  float quantizationBorder;

  if (gainCodingProfile == IISDRCGAINENC_GAINCODINGPROFILE_REGULAR || gainCodingProfile == IISDRCGAINENC_GAINCODINGPROFILE_FADING) {
    pBsGainDeltaTable = bsGainDeltaGainCodingProfile0_1;
    numGainDeltaTableEntries = sizeof(bsGainDeltaGainCodingProfile0_1) / sizeof(GAINDELTA_TABLE);
  } else if (gainCodingProfile == IISDRCGAINENC_GAINCODINGPROFILE_DUCKING_CLIPPING) {
    pBsGainDeltaTable = bsGainDeltaGainCodingProfile2;
    numGainDeltaTableEntries = sizeof(bsGainDeltaGainCodingProfile2) / sizeof(GAINDELTA_TABLE);
  } else {
    returnValue = IISDRCGAINENC_RETURN_ERROR_UNSUPPORTEDGAINCODINGPROFILE;
  }

  if (IISDRCGAINENC_RETURN_NOERROR == returnValue) {
    selectedTableEntry = numGainDeltaTableEntries - 1;

    for (k = 0; k < numGainDeltaTableEntries - 1; k++) {
      quantizationBorder = (pBsGainDeltaTable[k].value + pBsGainDeltaTable[k + 1].value) / 2.0f;
      if (gainDelta < quantizationBorder) {
        selectedTableEntry = k;
        break;
      }
    }

    if ((selectedTableEntry >= 0) && (selectedTableEntry < numGainDeltaTableEntries)) {
      *gainDeltaQuantized = pBsGainDeltaTable[selectedTableEntry].value;
      *pEncVal = pBsGainDeltaTable[selectedTableEntry].encVal;
      *pSizeBits = pBsGainDeltaTable[selectedTableEntry].sizeBits;
    } else {
      returnValue = IISDRCGAINENC_RETURN_ERROR_ILLEGAL_ARRAY_ACCESS;
    }
  }

  return returnValue;
}

static IISDRCGAINENC_RETURN
writeDrcGainSequence(wobitbufHandle hBitstream,
                     DrcGainSequence *pDrcGainSequence) {
  int wobitbufErr = 0, k, encVal, sizeBits, prevNodeTime;
  float prevNodeGain, gainDbDelta = 0.0f, gainInitialQuantized;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcGainSequence->drcGainCodingMode, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pDrcGainSequence->drcGainCodingMode == 0) {
    retVal = mapGainInitial(pDrcGainSequence->pGainDb[0], pDrcGainSequence->gainCodingProfile, &gainInitialQuantized, &encVal, &sizeBits);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  } else {
    for (k = 0; k < pDrcGainSequence->nNodes - 1; k++) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, 0, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }
    wobitbufErr = wobitbuf_WriteBits(hBitstream, 1, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    if (pDrcGainSequence->gainInterpolationType == IISDRCGAINENC_GAININTERPOLATIONTYPE_SPLINE) {
      for (k = 0; k < pDrcGainSequence->nNodes; k++) {
        retVal = mapSlope(pDrcGainSequence->pSlope[k], &encVal, &sizeBits);
        if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

        wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
        if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
      }
    }

    if (pDrcGainSequence->fullFrame == 0) {
      wobitbufErr = wobitbuf_WriteBits(hBitstream, pDrcGainSequence->frameEndFlag, 1);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    } else {
    }

    prevNodeTime = -1;
    for (k = 0; k < pDrcGainSequence->nNodes - 1; k++) {
      retVal = mapTimeDelta(pDrcGainSequence->pTimeSamples[k] - prevNodeTime, pDrcGainSequence->deltaTmin, pDrcGainSequence->nNodesMax, pDrcGainSequence->nBitsMaxTime, &encVal, &sizeBits);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

      prevNodeTime = pDrcGainSequence->pTimeSamples[k];

      wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }

    if (pDrcGainSequence->frameEndFlag == 0) {
      retVal = mapTimeDelta(pDrcGainSequence->pTimeSamples[k] - prevNodeTime, pDrcGainSequence->deltaTmin, pDrcGainSequence->nNodesMax, pDrcGainSequence->nBitsMaxTime, &encVal, &sizeBits);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

      wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    }

    retVal = mapGainInitial(pDrcGainSequence->pGainDb[0], pDrcGainSequence->gainCodingProfile, &gainInitialQuantized, &encVal, &sizeBits);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

    wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

    prevNodeGain = gainInitialQuantized;
    for (k = 1; k < pDrcGainSequence->nNodes; k++) {
      retVal = mapGainDelta(pDrcGainSequence->pGainDb[k] - prevNodeGain, pDrcGainSequence->gainCodingProfile, &gainDbDelta, &encVal, &sizeBits);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

      wobitbufErr = wobitbuf_WriteBits(hBitstream, encVal, sizeBits);
      if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

      prevNodeGain += gainDbDelta;
    }
  }

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeLoudnessInfoSetBits(LoudnessInfoSet *pLoudnessInfoSet,
                                       unsigned char *pBitstreamBuffer,
                                       int *pBitCount) {
  int k = 0, wobitbufErr = 0, version = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbuf bs;
  wobitbufHandle hBitstream = &bs;
  wobitbuf_Init(hBitstream, pBitstreamBuffer, DRC_PAYLOAD_MAX_SIZE_BITS - *pBitCount, *pBitCount);

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfoSet->loudnessInfoAlbumCount, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfoSet->loudnessInfoCount, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  for (k = 0; k < pLoudnessInfoSet->loudnessInfoAlbumCount; k++) {
    retVal = writeLoudnessInfo(hBitstream, version, &pLoudnessInfoSet->pLoudnessInfoAlbum[k]);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  for (k = 0; k < pLoudnessInfoSet->loudnessInfoCount; k++) {
    retVal = writeLoudnessInfo(hBitstream, version, &pLoudnessInfoSet->pLoudnessInfo[k]);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pLoudnessInfoSet->loudnessInfoSetExtPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pLoudnessInfoSet->loudnessInfoSetExtPresent) {
    retVal = writeLoudnessInfoSetExtension(hBitstream, &pLoudnessInfoSet->loudnessInfoSetExtension);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  *pBitCount += wobitbuf_GetBitsWritten(hBitstream);

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcConfigBits(UniDrcConfig *pUniDrcConfig,
                                    unsigned char *pBitstreamBuffer,
                                    int *pBitCount) {
  int k, wobitbufErr = 0, version = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbuf bs;
  wobitbufHandle hBitstream = &bs;
  wobitbuf_Init(hBitstream, pBitstreamBuffer, DRC_PAYLOAD_MAX_SIZE_BITS - *pBitCount, *pBitCount);

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->sampleRatePresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pUniDrcConfig->sampleRatePresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->sampleRate - 1000, 18);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->downmixInstructionsCount, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->drcDescriptionBasicPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pUniDrcConfig->drcDescriptionBasicPresent) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->drcCoefficientsBasicCount, 3);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->drcInstructionsBasicCount, 4);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  } else {
    pUniDrcConfig->drcCoefficientsBasicCount = 0;
    pUniDrcConfig->drcInstructionsBasicCount = 0;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->drcCoefficientsUniDrcCount, 3);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->drcInstructionsUniDrcCount, 6);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->channelLayout.baseChannelCount, 7);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  retVal = writeChannelLayoutDefaultSyntax(hBitstream, &pUniDrcConfig->channelLayout);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  for (k = 0; k < pUniDrcConfig->downmixInstructionsCount; k++) {
    retVal = writeDownmixInstructions(hBitstream, &(pUniDrcConfig->pDownmixInstructions[k]), version, pUniDrcConfig->channelLayout.baseChannelCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  for (k = 0; k < pUniDrcConfig->drcCoefficientsBasicCount; k++) {
    retVal = writeDrcCoefficientsBasic(hBitstream, &(pUniDrcConfig->pDrcCoefficientsBasic[k]));
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  for (k = 0; k < pUniDrcConfig->drcInstructionsBasicCount; k++) {
    retVal = writeDrcInstructionsBasic(hBitstream, &(pUniDrcConfig->pDrcInstructionsBasic[k]));
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  for (k = 0; k < pUniDrcConfig->drcCoefficientsUniDrcCount; k++) {
    retVal = writeDrcCoefficientsUniDrc(hBitstream, &(pUniDrcConfig->pDrcCoefficientsUniDrc[k]), version);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  for (k = 0; k < pUniDrcConfig->drcInstructionsUniDrcCount; k++) {
    retVal = writeDrcInstructionsUniDrc(hBitstream, &(pUniDrcConfig->pDrcInstructionsUniDrc[k]), pUniDrcConfig->channelLayout.baseChannelCount, version);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, pUniDrcConfig->uniDrcConfigExtPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  if (pUniDrcConfig->uniDrcConfigExtPresent) {
    retVal = writeUniDrcConfigExtension(hBitstream, &(pUniDrcConfig->uniDrcConfigExtension), pUniDrcConfig->channelLayout.baseChannelCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  *pBitCount += wobitbuf_GetBitsWritten(hBitstream);

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcGainBits(UniDrcGain *pUniDrcGain,
                                  const int gainSequenceCount,
                                  unsigned char *pBitstreamBuffer,
                                  int *pBitCount) {
  int k, wobitbufErr = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbuf bs;
  wobitbufHandle hBitstream = &bs;
  wobitbuf_Init(hBitstream, pBitstreamBuffer, DRC_PAYLOAD_MAX_SIZE_BITS - *pBitCount, *pBitCount);

  for (k = 0; k < gainSequenceCount; k++) {
    if (pUniDrcGain->pDrcGainSequence[k].gainCodingProfile < IISDRCGAINENC_GAINCODINGPROFILE_CONSTANT) {
      retVal = writeDrcGainSequence(hBitstream, &pUniDrcGain->pDrcGainSequence[k]);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }
  }

  wobitbufErr = wobitbuf_WriteBits(hBitstream, 0, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;

  *pBitCount += wobitbuf_GetBitsWritten(hBitstream);

  return IISDRCGAINENC_RETURN_NOERROR;
}

IISDRCGAINENC_RETURN
iisDRCGainEnc_writeUniDrcBits(HANDLE_IISDRCGAINENC_PARAMS hIisDrcGainEnc_params,
                              const int bLoudnessInfoSetPresent,
                              const int bUniDrcConfigPresent,
                              unsigned char *pBitstreamBuffer,
                              int *pBitCount) {
  int wobitbufErr = 0;
  IISDRCGAINENC_RETURN retVal = IISDRCGAINENC_RETURN_NOERROR;

  wobitbuf bs;
  wobitbufHandle hBitstream = &bs;
  wobitbuf_Init(hBitstream, pBitstreamBuffer, DRC_PAYLOAD_MAX_SIZE_BITS - *pBitCount, *pBitCount);

  wobitbufErr = wobitbuf_WriteBits(hBitstream, bLoudnessInfoSetPresent, 1);
  if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
  *pBitCount += 1;

  if (bLoudnessInfoSetPresent == 1) {
    wobitbufErr = wobitbuf_WriteBits(hBitstream, bUniDrcConfigPresent, 1);
    if (wobitbufErr) return IISDRCGAINENC_RETURN_ERROR_WRITEBITS;
    *pBitCount += 1;

    if (bUniDrcConfigPresent == 1) {
      retVal = iisDRCGainEnc_writeUniDrcConfigBits(hIisDrcGainEnc_params->pUniDrcConfig, pBitstreamBuffer, pBitCount);
      if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
    }

    retVal = iisDRCGainEnc_writeLoudnessInfoSetBits(hIisDrcGainEnc_params->pLoudnessInfoSet, pBitstreamBuffer, pBitCount);
    if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;
  }

  retVal = iisDRCGainEnc_writeUniDrcGainBits(hIisDrcGainEnc_params->pUniDrcGain, hIisDrcGainEnc_params->gainSequenceCount, pBitstreamBuffer, pBitCount);
  if (retVal != IISDRCGAINENC_RETURN_NOERROR) return retVal;

  return IISDRCGAINENC_RETURN_NOERROR;
}
