
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
#include <assert.h>
#include <string.h>

#include "iisutillib.h"
#include "mathlib.h"
#include "bit_buf.h"

#include "spaceEnclib_const.h"
#include "sac_types.h"
#include "sac_nlc_enc.h"
#include "space_bitstream.h"

#define MAX_FREQ_RES_INDEX 8
#define MAX_SAMPLING_FREQUENCY_INDEX 13
#define SAMPLING_FREQUENCY_INDEX_ESCAPE 15

#define MAX_RES_FR_PER_SP_FR 4

typedef struct BSF_INSTANCE {
  SPATIALSPECIFICCONFIG spatialSpecificConfig;
  SPATIALFRAME frame[RESIDUAL_CODING_DELAY + 1];

  MP4SPACEENC_RES_CODEC resCodec;

  unsigned char ****bitBuf;
  int ***bytesWritten;
  int ***bitsLastByte;

} BSF_INSTANCE;

static const int SampleRateTable[MAX_SAMPLING_FREQUENCY_INDEX] =
    {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

static const int FreqResBinTable[MAX_FREQ_RES_INDEX] = {0, 28, 20, 14, 10, 7, 5, 4};
static const int FreqResStrideTable[] = {1, 2, 5, 28};

HANDLE_ERROR_INFO DuplicateParameterSet(SPATIALFRAME *hFrom,
                                        int setFrom,
                                        SPATIALFRAME *hTo,
                                        int setTo) {
  HANDLE_ERROR_INFO error = noError;
  int box = 0;

  if (NULL == hFrom ||
      NULL == hTo) {
    error = iisUtil_ERROR(CDI, "Invalid Pointer");
  }

  if (error == noError) {
    for (box = 0; box < MAX_NUM_BOXES; box++) {
      copyINT(hFrom->ottData.cld[box][setFrom],
              hTo->ottData.cld[box][setTo],
              MAX_NUM_BINS);
      copyINT(hFrom->ottData.icc[box][setFrom],
              hTo->ottData.icc[box][setTo],
              MAX_NUM_BINS);

      copyINT(hFrom->ottData.ipd[box][setFrom],
              hTo->ottData.ipd[box][setTo],
              MAX_NUM_BINS);
    }

    for (box = 0; box < MAX_NUM_BOXES; box++) {
      memmove(&(hTo->residualData[box].iccDiffData[setTo]),
              &(hFrom->residualData[box].iccDiffData[setFrom]),
              sizeof(RESIDUALICCDIFFDATA));
    }

    hTo->alpha = hFrom->alpha;

    for (box = 0; box < MAX_NUM_BOXES; box++) {
      hTo->CLDLosslessData.bsXXXDataMode[box][setTo] = hFrom->CLDLosslessData.bsXXXDataMode[box][setFrom];
      hTo->CLDLosslessData.bsDataPair[box][setTo] = hFrom->CLDLosslessData.bsDataPair[box][setFrom];
      hTo->CLDLosslessData.bsQuantCoarseXXX[box][setTo] = hFrom->CLDLosslessData.bsQuantCoarseXXX[box][setFrom];
      hTo->CLDLosslessData.bsFreqResStrideXXX[box][setTo] = hFrom->CLDLosslessData.bsFreqResStrideXXX[box][setFrom];
    }
    for (box = 0; box < MAX_NUM_BOXES; box++) {
      hTo->ICCLosslessData.bsXXXDataMode[box][setTo] = hFrom->ICCLosslessData.bsXXXDataMode[box][setFrom];
      hTo->ICCLosslessData.bsDataPair[box][setTo] = hFrom->ICCLosslessData.bsDataPair[box][setFrom];
      hTo->ICCLosslessData.bsQuantCoarseXXX[box][setTo] = hFrom->ICCLosslessData.bsQuantCoarseXXX[box][setFrom];
      hTo->ICCLosslessData.bsFreqResStrideXXX[box][setTo] = hFrom->ICCLosslessData.bsFreqResStrideXXX[box][setFrom];
    }

    for (box = 0; box < MAX_NUM_BOXES; box++) {
      hTo->IPDLosslessData.bsXXXDataMode[box][setTo] = hFrom->IPDLosslessData.bsXXXDataMode[box][setFrom];
      hTo->IPDLosslessData.bsDataPair[box][setTo] = hFrom->IPDLosslessData.bsDataPair[box][setFrom];
      hTo->IPDLosslessData.bsQuantCoarseXXX[box][setTo] = hFrom->IPDLosslessData.bsQuantCoarseXXX[box][setFrom];
      hTo->IPDLosslessData.bsFreqResStrideXXX[box][setTo] = hFrom->IPDLosslessData.bsFreqResStrideXXX[box][setFrom];
    }
  }

  return error;
}

static void clearFrame(SPATIALFRAME *pFrame) {
  int box = 0;
  int ch = 0;
  int resFrame = 0;
  int aacFrame = 0;

  memset(pFrame, 0, sizeof(SPATIALFRAME));

  pFrame->bsIndependencyFlag = 1;
  pFrame->framingInfo.numParamSets = 1;

  for (box = 0; box < MAX_NUM_BOXES; ++box) {
    for (resFrame = 0; resFrame < MAX_RESIDUAL_FRAMES; ++resFrame) {
      for (aacFrame = 0; aacFrame < MAX_AAC_FRAMES; ++aacFrame) {
        pFrame->residualData[box].aacFrameData[resFrame][aacFrame].windowGrouping[0] = 8;
      }
    }
  }

  for (ch = 0; ch < MAX_NUM_ARBDMX_CH; ++ch) {
    for (resFrame = 0; resFrame < MAX_RESIDUAL_FRAMES; ++resFrame) {
      for (aacFrame = 0; aacFrame < MAX_AAC_FRAMES; ++aacFrame) {
        pFrame->residualDataArbDmx[ch].aacFrameData[resFrame][aacFrame].windowGrouping[0] = 8;
      }
    }
  }
}

static void
fine2coarse(int *data, DATA_TYPE dataType, int startBand, int numBands) {
  int i;
  for (i = startBand; i < startBand + numBands; i++) {
    if (dataType == t_CLD)

      data[i] /= 2;
    else

      data[i] >>= 1;
  }
}

static void
coarse2fine(int *data, DATA_TYPE dataType, int startBand, int numBands) {
  int i;
  for (i = startBand; i < startBand + numBands; i++) {
    data[i] <<= 1;
  }

  if (dataType == t_CLD) {
    for (i = startBand; i < startBand + numBands; i++) {
      if (data[i] == -14)
        data[i] = -15;
      else if (data[i] == 14)
        data[i] = 15;
    }
  }
}

static void ecData(HANDLE_BIT_BUF bitstream, int data[MAX_NUM_PARAMS][MAX_NUM_BINS], int oldData[MAX_NUM_BINS], int quantCoarseXXXprev[MAX_NUM_PARAMS], LOSSLESSDATA *losslessData, DATA_TYPE dataType, int paramIdx, int numParamSets, int independencyFlag, int startBand, int stopBand, int defaultValue, MPS_MODE mps_mode) {
  int ps, pb, strOffset, pbStride, dataBands, i;
  int aStrides[MAX_NUM_BINS + 1] = {0};
  short cmpIdxData[2][MAX_NUM_BINS] = {{0}};
  short cmpOldData[MAX_NUM_BINS] = {0};

  if (independencyFlag || (losslessData->bsQuantCoarseXXX[paramIdx][0] != quantCoarseXXXprev[paramIdx])) {
    losslessData->bsXXXDataMode[paramIdx][0] = FINECOARSE;
  } else {
    losslessData->bsXXXDataMode[paramIdx][0] = KEEP;
    for (i = startBand; i < stopBand; i++) {
      if (data[0][i] != oldData[i]) {
        losslessData->bsXXXDataMode[paramIdx][0] = FINECOARSE;
        break;
      }
    }
  }

  WriteBits(bitstream, losslessData->bsXXXDataMode[paramIdx][0], 2);

  for (ps = 1; ps < numParamSets; ps++) {
    if (losslessData->bsQuantCoarseXXX[paramIdx][ps] != losslessData->bsQuantCoarseXXX[paramIdx][ps - 1]) {
      losslessData->bsXXXDataMode[paramIdx][ps] = FINECOARSE;
    } else {
      losslessData->bsXXXDataMode[paramIdx][ps] = KEEP;
      for (i = startBand; i < stopBand; i++) {
        if (data[ps][i] != data[ps - 1][i]) {
          losslessData->bsXXXDataMode[paramIdx][ps] = FINECOARSE;
          break;
        }
      }
    }

    WriteBits(bitstream, losslessData->bsXXXDataMode[paramIdx][ps], 2);
  }

  for (ps = 0; ps < (numParamSets - 1); ps++) {
    if (losslessData->bsXXXDataMode[paramIdx][ps] == FINECOARSE) {
      if (losslessData->bsXXXDataMode[paramIdx][ps + 1] == FINECOARSE) {
        if ((losslessData->bsQuantCoarseXXX[paramIdx][ps + 1] == losslessData->bsQuantCoarseXXX[paramIdx][ps]) &&
            (losslessData->bsFreqResStrideXXX[paramIdx][ps + 1] == losslessData->bsFreqResStrideXXX[paramIdx][ps])) {
          losslessData->bsDataPair[paramIdx][ps] = 1;
          losslessData->bsDataPair[paramIdx][ps + 1] = 1;

          ps++;
          continue;
        }
      }

      losslessData->bsDataPair[paramIdx][ps] = 0;

      losslessData->bsDataPair[paramIdx][ps + 1] = 0;

    } else {
      losslessData->bsDataPair[paramIdx][ps] = 0;

      losslessData->bsDataPair[paramIdx][ps + 1] = 0;
    }
  }

  for (ps = 0; ps < numParamSets; ps++) {
    if (losslessData->bsXXXDataMode[paramIdx][ps] == DEFAULT) {
      for (i = startBand; i < stopBand; i++)
        oldData[i] = defaultValue;

      quantCoarseXXXprev[paramIdx] = 0;
    }

    if (losslessData->bsXXXDataMode[paramIdx][ps] == FINECOARSE) {
      WriteBits(bitstream, losslessData->bsDataPair[paramIdx][ps], 1);
      WriteBits(bitstream, losslessData->bsQuantCoarseXXX[paramIdx][ps], 1);
      WriteBits(bitstream, losslessData->bsFreqResStrideXXX[paramIdx][ps], 2);

      if (losslessData->bsQuantCoarseXXX[paramIdx][ps] != quantCoarseXXXprev[paramIdx]) {
        if (quantCoarseXXXprev[paramIdx]) {
          coarse2fine(oldData, dataType, startBand, stopBand - startBand);
        } else {
          fine2coarse(oldData, dataType, startBand, stopBand - startBand);
        }
      }

      pbStride = FreqResStrideTable[losslessData->bsFreqResStrideXXX[paramIdx][ps]];
      dataBands = (stopBand - startBand - 1) / pbStride + 1;

      aStrides[0] = startBand;
      for (pb = 1; pb <= dataBands; pb++) {
        aStrides[pb] = aStrides[pb - 1] + pbStride;
      }

      strOffset = 0;
      while (aStrides[dataBands] > stopBand) {
        if (strOffset < dataBands)
          strOffset++;

        for (i = strOffset; i <= dataBands; i++) {
          aStrides[i]--;
        }
      }

      for (pb = 0; pb < dataBands; pb++) {
        cmpOldData[startBand + pb] = oldData[aStrides[pb]];
        cmpIdxData[0][startBand + pb] = data[ps][aStrides[pb]];

        for (i = aStrides[pb] + 1; i < aStrides[pb + 1]; i++) {
          assert(data[ps][i] == cmpIdxData[0][startBand + pb]);
        }

        if (losslessData->bsDataPair[paramIdx][ps]) {
          cmpIdxData[1][startBand + pb] = data[ps + 1][aStrides[pb]];
          for (i = aStrides[pb] + 1; i < aStrides[pb + 1]; i++) {
            assert(data[ps + 1][i] == cmpIdxData[1][startBand + pb]);
          }
        }
      }

      if (losslessData->bsDataPair[paramIdx][ps]) {
        EcDataPairEnc(bitstream, cmpIdxData, cmpOldData, dataType, 0, startBand, dataBands,
                      losslessData->bsQuantCoarseXXX[paramIdx][ps], independencyFlag && (ps == 0), mps_mode);
      } else {
        EcDataSingleEnc(bitstream, cmpIdxData, cmpOldData, dataType, 0, startBand, dataBands,
                        losslessData->bsQuantCoarseXXX[paramIdx][ps], independencyFlag && (ps == 0), mps_mode);
      }

      for (i = startBand; i < stopBand; i++) {
        if (losslessData->bsDataPair[paramIdx][ps]) {
          oldData[i] = data[ps + 1][i];
        } else {
          oldData[i] = data[ps][i];
        }
      }

      quantCoarseXXXprev[paramIdx] = losslessData->bsQuantCoarseXXX[paramIdx][ps];

      if (losslessData->bsDataPair[paramIdx][ps]) {
        ps++;
      }
    }
  }
}

static HANDLE_ERROR_INFO
getBsFreqResIndex(int numBands, int *pbsFreqResIndex, int bLowDelay) {
  HANDLE_ERROR_INFO error = noError;
  int i;

  if (NULL == pbsFreqResIndex)
    error = iisUtil_ERROR(CDI, "Invalid Pointer.");

  if (noError == error) {
    *pbsFreqResIndex = -1;
    for (i = 0; i < MAX_FREQ_RES_INDEX; i++) {
      if (bLowDelay == 0) {
        if (numBands == FreqResBinTable[i]) {
          *pbsFreqResIndex = i;
          break;
        }
      }
    }
    if (*pbsFreqResIndex < 0 ||
        *pbsFreqResIndex >= MAX_FREQ_RES_INDEX) {
      error = iisUtil_ERROR(CDI, "FreqRes not found.");
    }
  }
  return error;
}

static HANDLE_ERROR_INFO
getSamplingFrequencyIndex(int bsSamplingFrequency, int *pbsSamplingFrequencyIndex) {
  HANDLE_ERROR_INFO error = noError;
  int i = 0;

  if (NULL == pbsSamplingFrequencyIndex)
    error = iisUtil_ERROR(CDI, "Invalid Pointer.");

  if (noError == error) {
    *pbsSamplingFrequencyIndex = -1;
    for (i = 0; i < MAX_SAMPLING_FREQUENCY_INDEX; i++) {
      if (bsSamplingFrequency == SampleRateTable[i]) {
        *pbsSamplingFrequencyIndex = i;
        break;
      }
    }
    if (*pbsSamplingFrequencyIndex < 0 ||
        *pbsSamplingFrequencyIndex >= MAX_SAMPLING_FREQUENCY_INDEX) {
      *pbsSamplingFrequencyIndex = SAMPLING_FREQUENCY_INDEX_ESCAPE;
    }
  }
  return error;
}

HANDLE_ERROR_INFO
DestroySpatialBitstreamEncoder(HANDLE_BSF_INSTANCE *selfPtr) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (selfPtr != NULL) {
      if ((*selfPtr)->bitBuf != NULL) {
        iisFreeMatrix4D((void ****)(*selfPtr)->bitBuf);
        (*selfPtr)->bitBuf = NULL;
      }

      if ((*selfPtr)->bytesWritten != NULL) {
        iisFreeMatrix3D((void ***)(*selfPtr)->bytesWritten);
        (*selfPtr)->bytesWritten = NULL;
      }

      if ((*selfPtr)->bitsLastByte != NULL) {
        iisFreeMatrix3D((void ***)(*selfPtr)->bitsLastByte);
        (*selfPtr)->bitsLastByte = NULL;
      }

      if (*selfPtr != NULL) {
        iisFree(*selfPtr);
      }

      *selfPtr = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
CreateSpatialBitstreamEncoder(HANDLE_BSF_INSTANCE *selfPtr,
                              BSF_RESIDUAL_CONFIG *pConfig,
                              CLASSIC_MPS BSF_RESIDUAL_CONFIG_ARB_DMX *pConfigArbDmx) {
  HANDLE_ERROR_INFO error = noError;
  BSF_INSTANCE *self = NULL;
  int i;

  if (error == noError) {
    if (NULL == (self = iisCalloc(1, sizeof(BSF_INSTANCE)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc for BSF_INSTANCE");
    }
  }

  if (error == noError) {
    self->resCodec = pConfig->codec;

    for (i = 0; i < RESIDUAL_CODING_DELAY + 1; i++) {
      clearFrame(&self->frame[i]);
    }
  }

  self->bitBuf = (unsigned char ****)iisCallocMatrix4D(MAX_NUM_BOXES, MAX_RES_FR_PER_SP_FR, 2, BITBUFSIZE * MAX_NUM_CH_PER_RES_ELEM, sizeof(unsigned char));
  self->bytesWritten = (int ***)iisCallocMatrix3D(MAX_NUM_BOXES, MAX_RES_FR_PER_SP_FR, 2, sizeof(int));
  self->bitsLastByte = (int ***)iisCallocMatrix3D(MAX_NUM_BOXES, MAX_RES_FR_PER_SP_FR, 2, sizeof(int));

  *selfPtr = self;
  return error;
}

SPATIALSPECIFICCONFIG *GetSpatialSpecificConfig(HANDLE_BSF_INSTANCE selfPtr) {
  return &(selfPtr->spatialSpecificConfig);
}

HANDLE_ERROR_INFO
WriteSpatialSpecificConfig(HANDLE_BIT_BUF bitstream, SPATIALSPECIFICCONFIG *spatialSpecificConfig, int *pnBitsWritten) {
  HANDLE_ERROR_INFO error = noError;
  int bsSamplingFrequencyIndex = 0;
  int bsFreqRes = 0;
  int nBitsWritten = 0;

  if (spatialSpecificConfig != NULL) {
    TREEDESCRIPTION *treeDescription = NULL;

    if (error == noError) {
      if (NULL == (treeDescription = &(spatialSpecificConfig->treeDescription))) {
        error = iisUtil_ERROR(CDI, "Invalid pointer.");
      }
    }

    SAFECALL(error, getBsFreqResIndex(spatialSpecificConfig->numBands, &bsFreqRes, spatialSpecificConfig->bLowDelay));

    SAFECALL(error, getSamplingFrequencyIndex(spatialSpecificConfig->bsSamplingFrequency, &bsSamplingFrequencyIndex));

    if (error == noError) {
      if ((bsFreqRes >> 3) > 0) {
        error = iisUtil_ERROR(CDI, "Invalid bsFreqRes.");
      } else {
        nBitsWritten += WriteBits(bitstream, bsFreqRes, 3);
      }
    }

    if (error == noError) {
      if ((spatialSpecificConfig->bsFixedGainDMX >> 3) > 0) {
        error = iisUtil_ERROR(CDI, "Invalid bsFixedGainDMX.");
      } else {
        nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsFixedGainDMX, 3);
      }
    }

    if (error == noError) {
      if ((spatialSpecificConfig->bsTempShapeConfig >> 2) > 0) {
        error = iisUtil_ERROR(CDI, "Invalid bsTempShapeConfig.");
      } else {
        nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsTempShapeConfig, 2);
      }
    }

    if (error == noError) {
      if ((spatialSpecificConfig->bsDecorrConfig >> 2) > 0) {
        error = iisUtil_ERROR(CDI, "Invalid bsDecorrConfig.");
      } else {
        nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsDecorrConfig, 2);
      }
    }

    spatialSpecificConfig->bs3DaudioMode = 0;

    if (spatialSpecificConfig->bsTreeConfig == TREE_USAC_212) {
      if (error == noError) {
        if ((spatialSpecificConfig->bsHighRateMode >> 1) > 0) {
          error = iisUtil_ERROR(CDI, "Invalid bsHighRateMode.");
        } else {
          nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsHighRateMode, 1);
        }
      }

      if (error == noError) {
        if (spatialSpecificConfig->bsIpdMode > IPDMODE_INVALID && spatialSpecificConfig->bsIpdMode <= IPDMODE_LAST) {
          nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsIpdMode > IPDMODE_NONE, 1);
        } else {
          error = iisUtil_ERROR(CDI, "Invalid bsIpdMode.");
        }
      }

      if (error == noError) {
        if ((spatialSpecificConfig->bsOttBandsPhasePresent >> 1) > 0) {
          error = iisUtil_ERROR(CDI, "Invalid bsOttBandsPhasePresent.");
        } else {
          nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsOttBandsPhasePresent, 1);
        }
      }

      if (spatialSpecificConfig->bsOttBandsPhasePresent) {
        if (error == noError) {
          if ((spatialSpecificConfig->bsOttBandsPhase >> 5) > 0) {
            error = iisUtil_ERROR(CDI, "Invalid bsOttBandsPhase.");
          } else {
            nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsOttBandsPhase, 5);
          }
        }
      }

      if (spatialSpecificConfig->bsResidualCoding) {
        if (error == noError) {
          if ((spatialSpecificConfig->residualConfig[0].bsResidualBands >> 5) > 0) {
            error = iisUtil_ERROR(CDI, "Invalid bsResidualBands.");
          } else {
            nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->residualConfig[0].bsResidualBands, 5);
          }

          if (error == noError) {
            nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsPseudoLr, 1);
          }
        }
      }
    }

    if (spatialSpecificConfig->bsTempShapeConfig == 2) {
      if (error == noError) {
        if ((spatialSpecificConfig->bsTreeConfig == TREE_USAC_212) &&
            (spatialSpecificConfig->bsEnvQuantMode >> 1) > 0) {
          error = iisUtil_ERROR(CDI, "Invalid bsEnvQuantMode.");
        } else {
          nBitsWritten += WriteBits(bitstream, spatialSpecificConfig->bsEnvQuantMode, 1);
        }
      }
    }

    nBitsWritten += (8 - (nBitsWritten % 8)) % 8;

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  if (error == noError) {
    if (pnBitsWritten != NULL) {
      *pnBitsWritten = nBitsWritten;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  return error;
}

SPATIALFRAME *GetSpatialFrame(HANDLE_BSF_INSTANCE selfPtr, int nOffset) {
  return &selfPtr->frame[nOffset];
}

HANDLE_ERROR_INFO
GetUsacMps212Config(SPATIALSPECIFICCONFIG *hSsc, MP4SPACEENC_USAC_MPS212_CONFIG *pUsacMps212Config) {
  HANDLE_ERROR_INFO error = noError;
  int bsFreqRes = 0;

  if (error == noError) {
    if ((hSsc == NULL) || (pUsacMps212Config == NULL)) {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (error == noError) {
    if (hSsc->bsTreeConfig != TREE_USAC_212) {
      error = iisUtil_ERROR(CDI, "Invalid configuration.");
    }
  }

  SAFECALL(error, getBsFreqResIndex(hSsc->numBands, &bsFreqRes, hSsc->bLowDelay));

  if (error == noError) {
    pUsacMps212Config->bsFreqRes = bsFreqRes;
    pUsacMps212Config->bsFixedGainDMX = hSsc->bsFixedGainDMX;
    pUsacMps212Config->bsTempShapeConfig = hSsc->bsTempShapeConfig;
    pUsacMps212Config->bsDecorrConfig = hSsc->bsDecorrConfig;
    pUsacMps212Config->bsHighRateMode = hSsc->bsHighRateMode;
    pUsacMps212Config->bsPhaseCoding = (hSsc->bsIpdMode > 0) ? 1 : 0;
    pUsacMps212Config->bsOttBandsPhasePresent = hSsc->bsOttBandsPhasePresent;
    pUsacMps212Config->bsOttBandsPhase = hSsc->bsOttBandsPhase;
    pUsacMps212Config->bsResidualBands = hSsc->residualConfig[0].bsResidualBands;
    pUsacMps212Config->bsPseudoLr = hSsc->bsPseudoLr;
    pUsacMps212Config->bsEnvQuantMode = hSsc->bsEnvQuantMode;
  }

  return error;
}

static HANDLE_ERROR_INFO
writeFramingInfo(HANDLE_BIT_BUF hBitstream,
                 FRAMINGINFO *pFramingInfo,
                 int frameLength,
                 int bLowDelay,
                 CLASSIC_MPS TREECONFIG treeConfig,
                 int bsHighRateMode) {
  HANDLE_ERROR_INFO error = noError;

  if (pFramingInfo != NULL) {
    if (error == noError) {
      if (bsHighRateMode) {
        WriteBits(hBitstream, pFramingInfo->bsFramingType, 1);
        if (bLowDelay == 0) {
          WriteBits(hBitstream, pFramingInfo->numParamSets - 1, 3);
        }

      } else {
        if (pFramingInfo->bsFramingType != 0) {
          error = iisUtil_ERROR(CDI, "Invalid framing type (only fixed framing supported in MPS 212 low bitrate mode)");
          return error;
        }
        if (pFramingInfo->numParamSets > 1) {
          error = iisUtil_ERROR(CDI, "Too many parameter sets (only 1 supported in MPS 212 low bitrate mode)");
          return error;
        }
      }

      if (pFramingInfo->bsFramingType) {
        int ps = 0;
        int numParamSets = pFramingInfo->numParamSets;

        {
          for (ps = 0; ps < numParamSets; ps++) {
            int bitsParamSlot = 0;
            while ((1 << bitsParamSlot) < (frameLength + 1)) bitsParamSlot++;
            if (bitsParamSlot > 0) WriteBits(hBitstream, pFramingInfo->bsParamSlots[ps], bitsParamSlot);
          }
        }
      }
    }
  } else {
    error = iisUtil_ERROR(CDI, "Invalid pointer to pFramingInfo");
  }

  return error;
}

static HANDLE_ERROR_INFO
writeSmgData(HANDLE_BIT_BUF hBitstream,
             SMGDATA *pSmgData,
             int numParamSets,
             int dataBands,
             int bsHighRateMode) {
  HANDLE_ERROR_INFO error = noError;

  int i, j;

  if (pSmgData != NULL) {
    if (bsHighRateMode)

    {
      for (i = 0; i < numParamSets; i++) {
        WriteBits(hBitstream, pSmgData->bsSmoothMode[i], 2);
        if (pSmgData->bsSmoothMode[i] >= 2) {
          WriteBits(hBitstream, pSmgData->bsSmoothTime[i], 2);
        }
        if (pSmgData->bsSmoothMode[i] == 3) {
          WriteBits(hBitstream, pSmgData->bsFreqResStride[i], 2);
          for (j = 0; j < dataBands; j += FreqResStrideTable[pSmgData->bsFreqResStride[i]]) {
            WriteBits(hBitstream, pSmgData->bsSmgData[i][j], 1);
          }
        }
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

static HANDLE_ERROR_INFO
writeOttData(HANDLE_BIT_BUF hBitstream, OTTDATA *pOttData, OTTCONFIG ottConfig[MAX_NUM_BOXES], LOSSLESSDATA *pCLDLosslessData, LOSSLESSDATA *pICCLosslessData, int numOttBoxes, int numBands, int numParamSets, int bsOneIcc, int *ottModeLfe, int bsIndependencyFlag, MPS_MODE mps_mode) {
  HANDLE_ERROR_INFO error = noError;

  if (pOttData != NULL && ottModeLfe != NULL) {
    int i;

    for (i = 0; i < numOttBoxes; i++) {
      ecData(hBitstream, pOttData->cld[i], pOttData->cld_old[i], pOttData->quantCoarseCldPrev[i], pCLDLosslessData, t_CLD, i, numParamSets, bsIndependencyFlag, 0, ottConfig[i].bsOttBands, 15, mps_mode);
    }
    if (bsOneIcc) {
      ecData(hBitstream, pOttData->icc[0], pOttData->icc_old[0], pOttData->quantCoarseIccPrev[0], pICCLosslessData, t_ICC, 0, numParamSets, bsIndependencyFlag, 0, numBands, 0, mps_mode);
    } else {
      for (i = 0; i < numOttBoxes; i++) {
        if (!ottModeLfe[i]) {
          ecData(hBitstream, pOttData->icc[i], pOttData->icc_old[i], pOttData->quantCoarseIccPrev[i], pICCLosslessData, t_ICC, i, numParamSets, bsIndependencyFlag, 0, numBands, 0, mps_mode);
        }
      }
    }
  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

static HANDLE_ERROR_INFO
writeTsdData(HANDLE_BIT_BUF hBitstream, TSDDATA *pTsdData, int nFrameLength) {
  HANDLE_ERROR_INFO error = noError;
  int i, nBitsTrSlots;
  int nFrameTimeSlots = nFrameLength + 1;

  if (nFrameTimeSlots < 7)
    nBitsTrSlots = 1;
  else if (nFrameTimeSlots < 13)
    nBitsTrSlots = 2;
  else if (nFrameTimeSlots < 25)
    nBitsTrSlots = 3;
  else if (nFrameTimeSlots < 49)
    nBitsTrSlots = 4;
  else
    nBitsTrSlots = 5;

  if (pTsdData != NULL) {
    if (pTsdData->bsTsdEnable == 1) {
      pTsdData->bsTsdNumTrSlots--;
      WriteBits(hBitstream, pTsdData->bsTsdNumTrSlots, nBitsTrSlots);

      if (pTsdData->TsdCodewordLength > 48) {
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[3], pTsdData->TsdCodewordLength - 48);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[2], 16);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[1], 16);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[0], 16);
      } else if (pTsdData->TsdCodewordLength > 32) {
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[2], pTsdData->TsdCodewordLength - 32);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[1], 16);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[0], 16);
      } else if (pTsdData->TsdCodewordLength > 16) {
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[1], pTsdData->TsdCodewordLength - 16);
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[0], 16);
      } else {
        WriteBits(hBitstream, pTsdData->bsTsdCodedPos[0], pTsdData->TsdCodewordLength);
      }

      for (i = 0; i < nFrameTimeSlots; i++) {
        if (pTsdData->tsdSepData[i])
          WriteBits(hBitstream, pTsdData->bsTsdTrPhaseData[i], 3);
      }
    }
  } else {
    error = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return error;
}

HANDLE_ERROR_INFO
WriteSpatialFrame(HANDLE_BIT_BUF bitstream, HANDLE_BSF_INSTANCE selfPtr, int bUsacIndependencyFlag) {
  HANDLE_ERROR_INFO error = noError;
  SPATIALFRAME *frame = NULL;
  SPATIALSPECIFICCONFIG *config = NULL;
  int numParamSets = 0;
  int numOttBoxes = 0;
  int *ottModeLfe = NULL;
  int i, j;
  int ch = 0;
  MPS_MODE mps_mode = MPS_MODE_INVALID;

  if (error == noError) {
    if (selfPtr != NULL) {
      frame = &(selfPtr->frame[RESIDUAL_CODING_DELAY]);
      config = &(selfPtr->spatialSpecificConfig);
      numOttBoxes = selfPtr->spatialSpecificConfig.treeDescription.numOttBoxes;
      ottModeLfe = selfPtr->spatialSpecificConfig.treeDescription.ottModeLfe;
    } else {
      error = iisUtil_ERROR(CDI, "invalid pointer.");
    }
  }

  if (error == noError) {
    if (frame != NULL) {
      numParamSets = frame->framingInfo.numParamSets;
    } else {
      error = iisUtil_ERROR(CDI, "invalid pointer.");
    }
  }

  if (error == noError) {
    if (config == NULL) {
      error = iisUtil_ERROR(CDI, "invalid pointer.");
    }
  }

  if (error == noError) {
    if (frame->bUseBBCues) {
      for (i = 0; i < MAX_NUM_BOXES; i++) {
        if (numParamSets == 1) {
          frame->CLDLosslessData.bsFreqResStrideXXX[i][0] = 3;
          frame->ICCLosslessData.bsFreqResStrideXXX[i][0] = 3;
        } else {
          for (j = 1; j < MAX_NUM_PARAMS; j++) {
            frame->CLDLosslessData.bsFreqResStrideXXX[i][j] = 3;
            frame->ICCLosslessData.bsFreqResStrideXXX[i][j] = 3;
          }
        }
      }
    }

    for (ch = 0; ch < MAX_NUM_ARBDMX_CH; ch++) {
      for (i = 0; i < MAX_NUM_PARAMS; i++) {
        frame->ADGLosslessData.bsQuantCoarseXXX[ch][i] = 0;
        frame->ADGLosslessData.bsFreqResStrideXXX[ch][i] = 0;
      }
    }
  }

  SAFECALL(error, writeFramingInfo(bitstream, &(frame->framingInfo), selfPtr->spatialSpecificConfig.bsFrameLength, selfPtr->spatialSpecificConfig.bLowDelay, selfPtr->spatialSpecificConfig.bsTreeConfig, selfPtr->spatialSpecificConfig.bsHighRateMode));

  if (error == noError) {
    if (selfPtr->spatialSpecificConfig.bsTreeConfig == TREE_USAC_212 && !bUsacIndependencyFlag) {
      WriteBits(bitstream, frame->bsIndependencyFlag, 1);
    }
  }

  mps_mode = MPS_MODE_USAC;

  SAFECALL(error, writeOttData(bitstream, &(frame->ottData), config->ottConfig, &(frame->CLDLosslessData), &(frame->ICCLosslessData), numOttBoxes, config->numBands, numParamSets, selfPtr->spatialSpecificConfig.bsOneIcc, ottModeLfe, frame->bsIndependencyFlag, mps_mode));

  if (error == noError) {
    if ((selfPtr->spatialSpecificConfig.bsTreeConfig == TREE_USAC_212) && (selfPtr->spatialSpecificConfig.bsIpdMode != IPDMODE_NONE)) {
      if (frame->bsPhaseMode == 0) {
        WriteBits(bitstream, 0, 1);
        for (i = 0; i < MAX_NUM_BINS; i++) {
          frame->ottData.ipd_old[0][i] = 0;
        }

      } else {
        WriteBits(bitstream, 1, 1);
        WriteBits(bitstream, selfPtr->spatialSpecificConfig.bsOpdSmoothing, 1);

        ecData(bitstream, frame->ottData.ipd[0], frame->ottData.ipd_old[0], frame->ottData.quantCoarseIpdPrev[0], &(frame->IPDLosslessData), t_IPD, 0, numParamSets, frame->bsIndependencyFlag, 0, selfPtr->spatialSpecificConfig.bsOttBandsPhase, 0, mps_mode);
      }
    }
  }

  SAFECALL(error, writeSmgData(bitstream, &(frame->smgData), numParamSets, config->numBands, selfPtr->spatialSpecificConfig.bsHighRateMode));

  if (error == noError) {
    if (selfPtr->spatialSpecificConfig.bsTreeConfig == TREE_USAC_212 && config->bsTempShapeConfig == TEMPSHAPE_TSD) {
      TSDDATA *pTsdData = &(frame->tsdData);
      WriteBits(bitstream, pTsdData->bsTsdEnable, 1);
    }
  }

  if (error == noError) {
    if (selfPtr->spatialSpecificConfig.bsTreeConfig == TREE_USAC_212) {
      SAFECALL(error, writeTsdData(bitstream, &(frame->tsdData), config->bsFrameLength));
    }
  }

  if (error == noError) {
    memmove(&selfPtr->frame[0], &selfPtr->frame[1], RESIDUAL_CODING_DELAY * sizeof(SPATIALFRAME));
    clearFrame(&selfPtr->frame[RESIDUAL_CODING_DELAY]);

    copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.cld_old[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.cld_old[0], MAX_NUM_BOXES * MAX_NUM_BINS);
    copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.icc_old[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.icc_old[0], MAX_NUM_BOXES * MAX_NUM_BINS);
    copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.quantCoarseCldPrev[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.quantCoarseCldPrev[0], MAX_NUM_BOXES * MAX_NUM_PARAMS);
    copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.quantCoarseIccPrev[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.quantCoarseIccPrev[0], MAX_NUM_BOXES * MAX_NUM_PARAMS);
    copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.quantCoarseIpdPrev[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.quantCoarseIpdPrev[0], MAX_NUM_BOXES * MAX_NUM_PARAMS);
    if (selfPtr->spatialSpecificConfig.bsIpdMode != IPDMODE_NONE) {
      copyINT(selfPtr->frame[RESIDUAL_CODING_DELAY - 1].ottData.ipd_old[0], selfPtr->frame[RESIDUAL_CODING_DELAY].ottData.ipd_old[0], MAX_NUM_BOXES * MAX_NUM_BINS);
    }

    selfPtr->frame[RESIDUAL_CODING_DELAY].alpha = selfPtr->frame[RESIDUAL_CODING_DELAY - 1].alpha;
  }

  return error;
}

int GetResidualSampleRate(int spatialSampleRate,
                          int frameTimeSlots,
                          int resFramesPerSpatial,
                          CLASSIC_MPS MP4SPACEENC_MODE encMode) {
  int residualSampleRate = SampleRateTable[0];
  int targetSampleRate = (int)((resFramesPerSpatial * 16.f / frameTimeSlots) * spatialSampleRate);
  int distance;
  int i;

  distance = abs(SampleRateTable[0] - targetSampleRate);

  for (i = 1; i < MAX_SAMPLING_FREQUENCY_INDEX; i++) {
    if (abs(SampleRateTable[i] - targetSampleRate) > distance) {
      break;
    }
    distance = abs(SampleRateTable[i] - targetSampleRate);
    residualSampleRate = SampleRateTable[i];
  }

  return residualSampleRate;
}
