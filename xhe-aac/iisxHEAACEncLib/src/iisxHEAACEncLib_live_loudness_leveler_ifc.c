
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
#include <memory.h>
#include <math.h>
#include <float.h>
#include "iisxHEAACEncLib_live_loudness_leveler_ifc.h"
#include "iisLevelerLib.h"
#include "limiterlib.h"
#include "iisxHEAACEncLib_common.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define MIN_LOUDNESS (-150)

typedef struct xheaacenclib_live_loudness_instance_struct {
  unsigned int nInputChannels;
  unsigned int isInitialized;
  unsigned int liveLoudnessLookaheadInSr;
  unsigned int lookaheadCompensationNeeded;
  unsigned int isComplianceStageActive;
  float* leveler_out_buffer;
  float* leveler_in_buffer;
  IIS_LEVELER_LIB_HANDLE hLeveler;
  IIS_LEVELER_LIB_OUTPUT_FORMAT gainFormat;
  TDLimiterPtr hLimiter;
} XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE;

static XHEAACENCLIB_RETURN liveLoudnessNew(XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE* phInstance);
static XHEAACENCLIB_RETURN prepareLevelerInputBuffer(HANDLE_MP4TIMEBUF const collectiveBuffer, int const offset, int const nCh, float* const input_buffer, int const maxSamplesPerChannel, int const validSamplesReadOffset);
static XHEAACENCLIB_RETURN updateCollectiveBuffer(HANDLE_MP4TIMEBUF const collectiveBuffer, float* const input_buffer, unsigned int* const lookaheadCompensationNeeded, unsigned int const delayOfLevelerLib,
                                                  unsigned int const nChannels, unsigned int const nProcessingSamples, unsigned int const blockCnt, unsigned int* const writeOffsetCurrent, unsigned int* const writeOffsetNext);

XHEAACENCLIB_RETURN iisxHEAACEncLib_live_loudness_ifc_init(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE* const phInstance,
    XHEAACENCLIB_LIVE_LOUDNESS_SETUP const setup) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  IIS_LEVELER_LIB_RETURN retValueLevelerLib = IIS_LEVELER_LIB_RETURN_NO_ERROR;
  IIS_LEVELER_LIB_CONFIG levelerConfig = {0};
  int loudnessComplianceStageEnabled = 1;

  if (NULL == phInstance) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = liveLoudnessNew(phInstance);
  }

  if (!isError(retValue)) {
    levelerConfig.numberOfChannels = setup.numInputChannels;
    levelerConfig.samplingRate = setup.inSampleRate;
    levelerConfig.maxSamplesPerChannel = LIVE_LOUDNESS_PROCESSING_BLOCK_SIZE;
    levelerConfig.outputFormat = IIS_LEVELER_LIB_OUTPUT_FORMAT_SAMPLES;
    levelerConfig.delayMultiple = 0;

    retValueLevelerLib = iisLevelerLib_Create(&(*phInstance)->hLeveler, &levelerConfig);

    if (retValueLevelerLib != IIS_LEVELER_LIB_RETURN_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_INIT_FAIL;
    }

    if (!isError(retValue)) {
      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        retValueLevelerLib = iisLevelerLib_Set_targetLoudness((*phInstance)->hLeveler, setup.targetLoudnessLevel);
      }

      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        retValueLevelerLib = iisLevelerLib_Set_loudnessComplianceStageEnabled((*phInstance)->hLeveler, loudnessComplianceStageEnabled);
      }

      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        retValueLevelerLib = iisLevelerLib_Set_silenceDetectionEnabled((*phInstance)->hLeveler, 1);
      }

      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        if (setup.liveMode == 0) {
          retValueLevelerLib = iisLevelerLib_Set_adaptiveAttack((*phInstance)->hLeveler, 1.0);
        }
        if (setup.liveMode == 1) {
          retValueLevelerLib = iisLevelerLib_Set_adaptiveAttack((*phInstance)->hLeveler, 0.0);
        }
      }

      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        if (setup.liveMode == 0) {
          retValueLevelerLib = iisLevelerLib_Set_dynamicRangePreservation((*phInstance)->hLeveler, 1.0);
        }
        if (setup.liveMode == 1) {
          retValueLevelerLib = iisLevelerLib_Set_dynamicRangePreservation((*phInstance)->hLeveler, 0.0);
        }
      }

      if (retValueLevelerLib == IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        if (setup.liveLoudnessRelMaxGain > 0.0f) {
          retValueLevelerLib = iisLevelerLib_Set_relativeMaxGain((*phInstance)->hLeveler, setup.liveLoudnessRelMaxGain);
        }
      }

      if (retValueLevelerLib != IIS_LEVELER_LIB_RETURN_NO_ERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_INIT_FAIL;
      }
    }
  }
  if (!isError(retValue)) {
    (*phInstance)->hLimiter = createLimiter(
        1,
        TDL_RELEASE_DEFAULT_MS,
        (float)pow(10.0, setup.limiterThreshold / 20.0),
        TDL_SMOOTHTYPE_DEFAULT,
        setup.numInputChannels,
        setup.inSampleRate);
    if ((*phInstance)->hLimiter == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_INIT_FAIL;
    }
  }

  if (!isError(retValue)) {
    (*phInstance)->isInitialized = 1;
    (*phInstance)->liveLoudnessLookaheadInSr = iisLevelerLib_Get_delayInSamples((*phInstance)->hLeveler);
    (*phInstance)->liveLoudnessLookaheadInSr += getLimiterDelay((*phInstance)->hLimiter);
    (*phInstance)->nInputChannels = setup.numInputChannels;
    (*phInstance)->leveler_out_buffer = (float*)calloc(levelerConfig.maxSamplesPerChannel * setup.numInputChannels, sizeof(float));
    (*phInstance)->leveler_in_buffer = (float*)calloc(levelerConfig.maxSamplesPerChannel * setup.numInputChannels, sizeof(float));
    (*phInstance)->gainFormat = levelerConfig.outputFormat;
    (*phInstance)->lookaheadCompensationNeeded = setup.compensateLevelerDelay;
    (*phInstance)->isComplianceStageActive = loudnessComplianceStageEnabled;
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_live_loudness_ifc_processing(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE const hInstance,
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    int const nSamples,
    int const validSamplesReadOffset,
    int const isFlushing,
    float lastMeasuredLoudness) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  IIS_LEVELER_LIB_RETURN retValueLevelerLib = IIS_LEVELER_LIB_RETURN_NO_ERROR;
  int limiterError = TDLIMIT_OK;

  if (hInstance == NULL || collectiveBuffer == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (hInstance->lookaheadCompensationNeeded == 1 || hInstance->isComplianceStageActive == 0) {
    lastMeasuredLoudness = MIN_LOUDNESS;
  }

  if (!isError(retValue) && nSamples > 0) {
    unsigned int blockCnt = 0;
    unsigned int numSamplesLeftToLevel = nSamples / hInstance->nInputChannels;
    unsigned int samplesLeftToReadInFlushing = 0;
    if (isFlushing) {
      samplesLeftToReadInFlushing = (nSamples - validSamplesReadOffset) / hInstance->nInputChannels;
    }
    unsigned int writeOffsetCurrent = 0;
    unsigned int writeOffsetNext = 0;
    unsigned int skipLevelerInputPreparation = 0;

    do {
      unsigned int delayOfLevelerLib = hInstance->liveLoudnessLookaheadInSr;
      int blockReadOffset = blockCnt * LIVE_LOUDNESS_PROCESSING_BLOCK_SIZE;
      unsigned int nProcessingSamples = min((unsigned int)LIVE_LOUDNESS_PROCESSING_BLOCK_SIZE, numSamplesLeftToLevel);
      unsigned int nSamplesToReadFromInBuffer = nProcessingSamples;
      if (isFlushing && skipLevelerInputPreparation == 0) {
        nSamplesToReadFromInBuffer = min(samplesLeftToReadInFlushing, nProcessingSamples);
      }

      if (!isError(retValue) && skipLevelerInputPreparation == 0) {
        retValue = prepareLevelerInputBuffer(collectiveBuffer, blockReadOffset, hInstance->nInputChannels, hInstance->leveler_in_buffer, nSamplesToReadFromInBuffer, validSamplesReadOffset);
        if (isFlushing == 1 && nSamplesToReadFromInBuffer < LIVE_LOUDNESS_PROCESSING_BLOCK_SIZE) {
          skipLevelerInputPreparation = 1;

          memset(&hInstance->leveler_in_buffer[(nSamplesToReadFromInBuffer * hInstance->nInputChannels)], 0.0f, sizeof(float) * ((LIVE_LOUDNESS_PROCESSING_BLOCK_SIZE - nSamplesToReadFromInBuffer) * hInstance->nInputChannels));
        }
      }

      if (!isError(retValue) && nSamples > 0) {
        retValueLevelerLib = iisLevelerLib_Process(hInstance->hLeveler, nProcessingSamples, hInstance->leveler_in_buffer, hInstance->leveler_out_buffer, lastMeasuredLoudness);
        if (retValueLevelerLib != IIS_LEVELER_LIB_RETURN_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS;
        }
      }

      if (!isError(retValue) && nSamples > 0) {
        limiterError = applyLimiter(hInstance->hLimiter, hInstance->leveler_out_buffer, nProcessingSamples);
        if (limiterError != TDLIMIT_OK) {
          retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS;
        }
      }

      if (!isError(retValue)) {
        retValue = updateCollectiveBuffer(collectiveBuffer,
                                          hInstance->leveler_out_buffer,
                                          &hInstance->lookaheadCompensationNeeded,
                                          delayOfLevelerLib,
                                          hInstance->nInputChannels,
                                          nProcessingSamples,
                                          blockCnt,
                                          &writeOffsetCurrent,
                                          &writeOffsetNext);
      }
      numSamplesLeftToLevel -= nProcessingSamples;
      if (isFlushing == 1 && skipLevelerInputPreparation == 0) {
        samplesLeftToReadInFlushing -= nProcessingSamples;
      }
      blockCnt++;
    } while (numSamplesLeftToLevel > 0);
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_live_loudness_ifc_delete(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE hInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  IIS_LEVELER_LIB_RETURN retValueLevelerLib = IIS_LEVELER_LIB_RETURN_NO_ERROR;
  int limiterError = TDLIMIT_OK;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hInstance->isInitialized) {
    retValueLevelerLib = iisLevelerLib_Destroy(&(hInstance->hLeveler));

    if (retValueLevelerLib != IIS_LEVELER_LIB_RETURN_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    }
  }

  if (!isError(retValue)) {
    limiterError = destroyLimiter(hInstance->hLimiter);

    if (limiterError != TDLIMIT_OK) {
      retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
    } else {
      hInstance->hLimiter = NULL;
    }
  }

  if (!isError(retValue)) {
    if (hInstance->leveler_out_buffer != NULL) {
      free(hInstance->leveler_out_buffer);
      hInstance->leveler_out_buffer = NULL;
    }
    if (hInstance->leveler_in_buffer != NULL) {
      free(hInstance->leveler_in_buffer);
      hInstance->leveler_in_buffer = NULL;
    }

    iisFree(hInstance);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_live_loudness_ifc_getLookahead(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE const hInstance,
    unsigned int* const nSamplesLookaheadInSr) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hInstance == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *nSamplesLookaheadInSr = hInstance->liveLoudnessLookaheadInSr;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN liveLoudnessNew(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE* phInstance) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (phInstance != NULL) {
    int nSize = sizeof(XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE);

    *phInstance = (XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE*)iisCalloc(1, nSize);
    (*phInstance)->hLeveler = NULL;

    if (*phInstance == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  } else {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN prepareLevelerInputBuffer(
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    int const offset,
    int const nCh,
    float* const leveler_in_buffer,
    int const maxSamplesPerChannel,
    int const validSamplesReadOffset) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  float* pInBufferSamples = NULL;
  unsigned int internalOffset;

  if (collectiveBuffer == NULL || leveler_in_buffer == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  internalOffset = offset + (validSamplesReadOffset / nCh);

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = MP4TIMEBUF_SaveAccessBuffer(collectiveBuffer, internalOffset, maxSamplesPerChannel, 0, &pInBufferSamples);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    memcpy(leveler_in_buffer, pInBufferSamples, sizeof(float) * maxSamplesPerChannel * nCh);
  }

  return retValue;
}

static XHEAACENCLIB_RETURN updateCollectiveBuffer(
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    float* const input_buffer,
    unsigned int* const lookaheadCompensationNeeded,
    unsigned int const delayOfLevelerLib,
    unsigned int const nChannels,
    unsigned int const nProcessingSamples,
    unsigned int const blockCnt,
    unsigned int* const writeOffsetCurrent,
    unsigned int* const writeOffsetNext) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  float* samples_to_level = NULL;
  unsigned int nSamplesToWriteToCollBuffer = 0;
  unsigned int readyToUpdateCollBuffer = 1;

  if (((blockCnt * nProcessingSamples) + nProcessingSamples < delayOfLevelerLib) && (*lookaheadCompensationNeeded == 1)) {
    readyToUpdateCollBuffer = 0;
  }

  if (collectiveBuffer == NULL || input_buffer == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && readyToUpdateCollBuffer == 1) {
    HANDLE_ERROR_INFO errorInfo = noError;

    if (*lookaheadCompensationNeeded == 1) {
      nSamplesToWriteToCollBuffer = ((blockCnt * nProcessingSamples) + nProcessingSamples) - delayOfLevelerLib;
      *writeOffsetCurrent = 0;
      *writeOffsetNext = *writeOffsetCurrent + nSamplesToWriteToCollBuffer;
      samples_to_level = &input_buffer[nChannels * (nProcessingSamples - nSamplesToWriteToCollBuffer)];
      *lookaheadCompensationNeeded = 0;

    } else {
      nSamplesToWriteToCollBuffer = nProcessingSamples;
      *writeOffsetCurrent = *writeOffsetNext;
      *writeOffsetNext = *writeOffsetCurrent + nProcessingSamples;
      samples_to_level = input_buffer;
    }

    errorInfo = MP4TIMEBUF_UpdateBuffer(collectiveBuffer, samples_to_level, *writeOffsetCurrent, nSamplesToWriteToCollBuffer);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_LIVE_LOUDNESS_PROCESS;
    }
  }

  return retValue;
}
