
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

#include "iisxHEAACEncLib_delayAndBuffer.h"
#include <stdlib.h>
#include <math.h>
#include "iisxHEAACEncLib_common.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static XHEAACENCLIB_RETURN liveLoudnessDelayCalculationAndBufferCreation(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE const hLiveLoudness,
    XHEAACENCLIB_CONFIG_HANDLE const hConfig);

static XHEAACENCLIB_RETURN drcDelayCalculationAndBufferCreation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    HANDLE_MP4TIMEBUF* const hCoreDelayBuffer,
    XHEAACENCLIB_DRC_DELAY* const drcDelayData,
    MPEG4_DELAY* const mpeg4DelayParameter,
    unsigned int const nDelaySwDeci,
    int* const encoderDelay);

static XHEAACENCLIB_RETURN addDelayCalculationAndEncoderBufferCreation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SWDECI* const switchingDecision,
    XHEAACENCLIB_ENCBUFFER* const encBufferData,
    MPEG4_DELAY* const mpeg4DelayParameter,
    TIME_SIGNAL_DATA* const timeSignal,
    int* const encoderDelay);

static unsigned int calculateGlobalSamplesNext(unsigned int const sampleRateIn,
                                               unsigned int const sampleRateOut,
                                               unsigned int const mpeg4Delay,
                                               unsigned int const mpeg4StandDelay,
                                               unsigned int const nInChannels,
                                               unsigned int const nSamplesRequired,
                                               unsigned int const gnSamplesNext);

static unsigned int calculateGCD(unsigned int const valueA, unsigned int const valueB);

static XHEAACENCLIB_RETURN liveLoudnessDelayCalculationAndBufferCreation(
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE const hLiveLoudness,
    XHEAACENCLIB_CONFIG_HANDLE const hConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned int liveLoudnessDelayInSr = 0;

  if (hConfig == NULL || hLiveLoudness == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_live_loudness_ifc_getLookahead(hLiveLoudness, &liveLoudnessDelayInSr);

    if (isError(retValue)) {
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN drcDelayCalculationAndBufferCreation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    HANDLE_MP4TIMEBUF* const hCoreDelayBuffer,
    XHEAACENCLIB_DRC_DELAY* const drcDelayData,
    MPEG4_DELAY* const mpeg4DelayParameter,
    unsigned int const nDelaySwDeci,
    int* const encoderDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int mpegD_drc_on = 0;

  if (hConfig == NULL || hCoreDelayBuffer == NULL || drcDelayData == NULL || mpeg4DelayParameter == NULL || encoderDelay == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    printErrorConsole(CDI, "Invalid handle");
  } else if (hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF && hDrc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    printErrorConsole(CDI, "Invalid handle");
  }

  if (!isError(retValue)) {
    mpegD_drc_on = hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF;
  }

  if (!isError(retValue) && (mpegD_drc_on || hConfig->bMpeg4DrcOn)) {
    DRC_IFC_RETURN retValueDrc = DRC_IFC_NO_ERROR;
    int codecDelay = *encoderDelay;
    int drcDelay = 0;

    if ((mpegD_drc_on) && hConfig->bMpeg4DrcOn) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      printErrorConsole(CDI, "MPEG-D DRC and MPEG-4 DRC are not allowed to be active together");
    }

    if (!isError(retValue) && hConfig->bMpeg4DrcOn) {
      drcDelayData->drcEncoderDelay = 0;
      drcDelayData->drcDecoderDelay = 0;
      drcDelayData->drcLookAhead = 0;
    }

    if (!isError(retValue) && (mpegD_drc_on)) {
      retValueDrc = iisxHEAACEncLib_drc_get_encoder_delay(hDrc, &drcDelayData->drcEncoderDelay);
      if (retValueDrc != DRC_IFC_NO_ERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_DELAY_AND_BUFFER;
        printErrorConsole(CDI, "Could not create DRC delay buffer");
      }

      if (!isError(retValue)) {
        retValueDrc = iisxHEAACEncLib_drc_get_decoder_delay(hDrc, &drcDelayData->drcDecoderDelay);
        if (retValueDrc != DRC_IFC_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_DELAY_AND_BUFFER;
          printErrorConsole(CDI, "Could not create DRC delay buffer");
        }
      }

      if (!isError(retValue)) {
        {
          retValueDrc = iisxHEAACEncLib_drc_get_compressor_lookAhead(hDrc, &drcDelayData->drcLookAhead);
          if (retValueDrc != DRC_IFC_NO_ERROR) {
            retValue = XHEAACENCLIB_RETURN_ERROR_DELAY_AND_BUFFER;
            printErrorConsole(CDI, "Could not create DRC delay buffer");
          }
        }
      }
    } else {
      drcDelayData->drcEncoderDelay = hConfig->nFrameSamples;
    }

    if (!isError(retValue)) {
      drcDelay = drcDelayData->drcEncoderDelay + drcDelayData->drcDecoderDelay;

      {
        drcDelay += drcDelayData->drcLookAhead + nDelaySwDeci;
      }

      if (hConfig->aot == AUD_OBJ_TYP_USAC || mpegD_drc_on) {
        if ((hConfig->bUseSBR) || (hConfig->stereoConfigIndex > 0)) {
          codecDelay -= QMF_SYNTHESIS_DELAY;
        }
      }

      drcDelayData->drcCompensationDelay = drcDelay - codecDelay;

      drcDelayData->hDrcLookaheadBuffer = NULL;

      {
        if (drcDelayData->drcCompensationDelay > 0) {
          HANDLE_ERROR_INFO errorInfo = noError;

          *encoderDelay += drcDelayData->drcCompensationDelay;
          mpeg4DelayParameter->mpeg4DelayWOSwitching += drcDelayData->drcCompensationDelay;

          errorInfo = MP4TIMEBUF_Create(&drcDelayData->hDrcLookaheadBuffer,
                                        drcDelayData->drcLookAhead + hConfig->nFrameSamples,
                                        hConfig->nFrameSamples + drcDelayData->drcLookAhead,
                                        hConfig->nInChannels,
                                        drcDelayData->drcLookAhead);
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
          }

          if (!isError(retValue)) {
            errorInfo = MP4TIMEBUF_Create(hCoreDelayBuffer,
                                          drcDelayData->drcCompensationDelay + hConfig->nFrameSamples,
                                          hConfig->nFrameSamples,
                                          hConfig->nInChannels,
                                          drcDelayData->drcCompensationDelay);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
            }
          }
        } else {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = MP4TIMEBUF_Create(hCoreDelayBuffer,
                                        abs(drcDelayData->drcCompensationDelay - drcDelayData->drcLookAhead) + hConfig->nFrameSamples,
                                        drcDelayData->drcLookAhead + hConfig->nFrameSamples,
                                        hConfig->nInChannels,
                                        abs(drcDelayData->drcCompensationDelay - drcDelayData->drcLookAhead));
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
          }
        }
      }
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN addDelayCalculationAndEncoderBufferCreation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SWDECI* const switchingDecision,
    XHEAACENCLIB_ENCBUFFER* const encBufferData,
    MPEG4_DELAY* const mpeg4DelayParameter,
    TIME_SIGNAL_DATA* const timeSignal,
    int* const encoderDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  int addDelay = 0;
  int writeOffset = 0;

  if (hConfig == NULL || switchingDecision == NULL || encBufferData == NULL || mpeg4DelayParameter == NULL || timeSignal == NULL || encoderDelay == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  int const simSpace = max(switchingDecision->swInfo.bufferWriteOffset, (int)timeSignal->nSamplesRequired);

  mpeg4DelayParameter->mpeg4Delay = *encoderDelay;

  if ((*encoderDelay) % hConfig->nFrameSamples && hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE) {
    *encoderDelay += hConfig->nFrameSamples - (*encoderDelay % hConfig->nFrameSamples);
  } else if ((*encoderDelay - mpeg4DelayParameter->mpeg4StandDelay) % hConfig->nFrameSamples && hConfig->primingMode == XHEAACENC_PRIMINGMODE_STDDELAY) {
    *encoderDelay += hConfig->nFrameSamples - ((*encoderDelay - mpeg4DelayParameter->mpeg4StandDelay) % hConfig->nFrameSamples);
  }

  else if ((*encoderDelay) % hConfig->nFrameSamples &&
           hConfig->primingMode == XHEAACENC_PRIMINGMODE_FULL &&
           hConfig->metadataMode != METADATA_NONE) {
    *encoderDelay += hConfig->nFrameSamples - ((*encoderDelay) % hConfig->nFrameSamples);

    mpeg4DelayParameter->mpeg4Delay = *encoderDelay;
  }

  addDelay = *encoderDelay - mpeg4DelayParameter->mpeg4DelayWOSwitching;

  addDelay = (int)((((float)addDelay * (float)hConfig->sampleRateIn) / (float)hConfig->sampleRateOut) + 0.5f);

  addDelay -= switchingDecision->swInfo.bufferWriteOffset;
  addDelay = max(0, addDelay);

  if (!isError(retValue)) {
    int const nSamplesRequiredPerChannel = timeSignal->nSamplesRequired / hConfig->nInChannels;

    int const bufSize = switchingDecision->swInfo.bufferSize +
                        nSamplesRequiredPerChannel +
                        addDelay +
                        (hConfig->nFrameSamples * hConfig->sampleRateIn / hConfig->sampleRateOut);

    writeOffset = switchingDecision->swInfo.bufferWriteOffset + addDelay;
    encBufferData->addDelay = 0;

    if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_FULL && hConfig->metadataMode != METADATA_NONE) {
      encBufferData->addDelay = addDelay;
    }

    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = MP4TIMEBUF_Create(&encBufferData->hEncBuffer,
                                  bufSize,
                                  simSpace,
                                  hConfig->nInChannels,
                                  writeOffset);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    int gnSamplesNextTemp = 0;
    switchingDecision->nSwDeciBufferOffset = switchingDecision->swInfo.bufferWriteOffset;

    encBufferData->encBufValidSamples = (writeOffset)*hConfig->nInChannels;

    if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_FULL && hConfig->metadataMode != METADATA_NONE) {
      gnSamplesNextTemp = max(((int)timeSignal->gnSamplesNext - addDelay), ((int)timeSignal->nSamplesRequired));
    } else {
      gnSamplesNextTemp = max(((int)timeSignal->gnSamplesNext - addDelay), ((int)timeSignal->nSamplesRequired - addDelay));
    }
    timeSignal->gnSamplesNext = (unsigned int)max(gnSamplesNextTemp, (int)(1 * hConfig->nInChannels));
  }
  return retValue;
}

static unsigned int calculateGlobalSamplesNext(unsigned int const sampleRateIn,
                                               unsigned int const sampleRateOut,
                                               unsigned int const mpeg4Delay,
                                               unsigned int const mpeg4StandDelay,
                                               unsigned int const nInChannels,
                                               unsigned int const nSamplesRequired,
                                               unsigned int const gnSamplesNext) {
  unsigned int delay = mpeg4Delay - mpeg4StandDelay;
  unsigned int gcd = calculateGCD(sampleRateIn, sampleRateOut);
  unsigned int upSamplingFactor = sampleRateIn / gcd * nInChannels;
  unsigned int downSamplingFactor = sampleRateOut / gcd;
  unsigned int overflowThreshold = UINT_MAX / upSamplingFactor;
  unsigned int resampledMpeg4Delay;
  unsigned int globalSamplesNext;

  if (delay <= overflowThreshold) {
    resampledMpeg4Delay = (delay * upSamplingFactor) / downSamplingFactor;
  } else {
    resampledMpeg4Delay = (delay / downSamplingFactor) * upSamplingFactor;
  }

  globalSamplesNext = nSamplesRequired + resampledMpeg4Delay;

  if (globalSamplesNext % nInChannels) {
    globalSamplesNext += globalSamplesNext % nInChannels;
  }

  globalSamplesNext = max(gnSamplesNext, globalSamplesNext);

  return globalSamplesNext;
}

static unsigned int calculateGCD(unsigned int const valueA,
                                 unsigned int const valueB) {
  if (0 != valueB) {
    return calculateGCD(valueB, valueA % valueB);
  } else {
    return valueA;
  }
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_delayCalculationAndBufferCreation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_DRCENCODER const hDrc,
    XHEAACENCLIB_LIVE_LOUDNESS_INSTANCE_HANDLE const hLiveLoudness,
    XHEAACENCLIB_SWDECI* const switchingDecision,
    HANDLE_MP4TIMEBUF* const hCoreDelayBuffer,
    HANDLE_MP4TIMEBUF* const collectiveBuffer,
    XHEAACENCLIB_ENCBUFFER* const encBufferData,
    XHEAACENCLIB_DRC_DELAY* const drcDelayData,
    MPEG4_DELAY* const mpeg4DelayParameter,
    TIME_SIGNAL_DATA* const timeSignal,
    int* const nTrashAUs) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  int encoderDelay = 0;
  unsigned int liveLoudnessDelayInSr = 0;

  if (hConfig == NULL || switchingDecision == NULL || hCoreDelayBuffer == NULL || collectiveBuffer == NULL || encBufferData == NULL || drcDelayData == NULL || mpeg4DelayParameter == NULL || timeSignal == NULL || nTrashAUs == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF && hDrc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->bLiveLoudnessLevelSet && hLiveLoudness == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    encoderDelay = mpeg4DelayParameter->mpeg4DelayWOSwitching + switchingDecision->nDelaySwDeci;
  }

  if (!isError(retValue)) {
    if (hConfig->bLiveLoudnessLevelSet) {
      retValue = liveLoudnessDelayCalculationAndBufferCreation(
          hLiveLoudness,
          hConfig);
    }
  }

  if (!isError(retValue)) {
    retValue = drcDelayCalculationAndBufferCreation(
        hConfig,
        hDrc,
        hCoreDelayBuffer,
        drcDelayData,
        mpeg4DelayParameter,
        switchingDecision->nDelaySwDeci,
        &encoderDelay);
  }

  if (!isError(retValue)) {
    retValue = addDelayCalculationAndEncoderBufferCreation(hConfig, switchingDecision, encBufferData, mpeg4DelayParameter, timeSignal, &encoderDelay);
  }

  if (!isError(retValue)) {
    if (hConfig->bLiveLoudnessLevelSet) {
      retValue = iisxHEAACEncLib_live_loudness_ifc_getLookahead(hLiveLoudness, &liveLoudnessDelayInSr);
    }
  }

  if (!isError(retValue)) {
    if (XHEAACENC_PRIMINGMODE_NONE == hConfig->primingMode) {
      *nTrashAUs = encoderDelay / hConfig->nFrameSamples;

      if (*nTrashAUs >= 1) {
        timeSignal->gnSamplesNext = calculateGlobalSamplesNext((unsigned int)hConfig->sampleRateIn,
                                                               (unsigned int)hConfig->sampleRateOut,
                                                               (unsigned int)mpeg4DelayParameter->mpeg4Delay,
                                                               0,
                                                               hConfig->nInChannels,
                                                               timeSignal->nSamplesRequired,
                                                               timeSignal->gnSamplesNext);
      }
    } else if (XHEAACENC_PRIMINGMODE_STDDELAY == hConfig->primingMode) {
      *nTrashAUs = (encoderDelay - mpeg4DelayParameter->mpeg4StandDelay) / hConfig->nFrameSamples;

      if (*nTrashAUs >= 1) {
        timeSignal->gnSamplesNext = calculateGlobalSamplesNext((unsigned int)hConfig->sampleRateIn,
                                                               (unsigned int)hConfig->sampleRateOut,
                                                               (unsigned int)mpeg4DelayParameter->mpeg4Delay,
                                                               (unsigned int)mpeg4DelayParameter->mpeg4StandDelay,
                                                               hConfig->nInChannels,
                                                               timeSignal->nSamplesRequired,
                                                               timeSignal->gnSamplesNext);
      }
    } else {
      *nTrashAUs = 0;
    }

    if (XHEAACENC_PRIMINGMODE_NONE == hConfig->primingMode || XHEAACENC_PRIMINGMODE_STDDELAY == hConfig->primingMode) {
      timeSignal->gnSamplesNext += liveLoudnessDelayInSr * hConfig->nInChannels;
    }

    {
      int samplesNeeded = mpeg4DelayParameter->mpeg4Delay - mpeg4DelayParameter->mpeg4DelayWOSwitching + (hConfig->nFrameSamples * (*nTrashAUs + 1));

      int samplesNeededInputSR = (int)ceil(samplesNeeded * (hConfig->sampleRateIn / (float)hConfig->sampleRateOut));
      int samplesNeededInputSRMultiChannel = (int)samplesNeededInputSR * (int)hConfig->nInChannels;
      int minRequestNextFrame = samplesNeededInputSRMultiChannel - encBufferData->encBufValidSamples;

      timeSignal->additionalSampleRequest = minRequestNextFrame - (int)timeSignal->gnSamplesNext;
      timeSignal->gnSamplesNext = max((int)timeSignal->gnSamplesNext, minRequestNextFrame);
    }

    timeSignal->nSamplesUntilNext = timeSignal->gnSamplesNext;
    mpeg4DelayParameter->mpeg4Priming = encoderDelay - (*nTrashAUs * hConfig->nFrameSamples);

    hConfig->codecDelay = mpeg4DelayParameter->mpeg4Delay;

    {
      timeSignal->nSamplesMax = timeSignal->nSamplesUntilNext;
    }

    if (hConfig->bLiveLoudnessLevelSet) {
      int liveLoudnessDelayOutSr = (int)ceil(liveLoudnessDelayInSr * (hConfig->sampleRateOut / (float)hConfig->sampleRateIn));
      hConfig->codecDelay += liveLoudnessDelayOutSr;
      if (XHEAACENC_PRIMINGMODE_FULL == hConfig->primingMode) {
        mpeg4DelayParameter->mpeg4Priming += liveLoudnessDelayOutSr;
      }
    }

    if (!isError(retValue)) {
      HANDLE_ERROR_INFO errorInfo = noError;
      int bufferSizePerChannel;
      int simSpace;

      timeSignal->minInBufSize = timeSignal->nSamplesMax;
      bufferSizePerChannel = (int)min(timeSignal->minInBufSize + timeSignal->nSamplesRequired, (unsigned int)INT_MAX) / (int)hConfig->nInChannels;
      assert(timeSignal->minInBufSize <= (unsigned int)INT_MAX);

      simSpace = max(switchingDecision->swInfo.bufferWriteOffset, (int)timeSignal->nSamplesRequired);
      simSpace += liveLoudnessDelayInSr;

      errorInfo = MP4TIMEBUF_Create(collectiveBuffer,
                                    bufferSizePerChannel,
                                    simSpace,
                                    hConfig->nInChannels,
                                    0);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
      }
    }

    if (!isError(retValue)) {
      if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE) {
        if (mpeg4DelayParameter->mpeg4Priming != 0) {
          WARN("Priming not equal 0 also skipDelay is on.");
        }
      } else if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_STDDELAY) {
        if (mpeg4DelayParameter->mpeg4Priming != mpeg4DelayParameter->mpeg4StandDelay) {
          WARN("Priming not equal to standard Delay also skipDelay is set to partly.");
        }
      } else {
        if (mpeg4DelayParameter->mpeg4Priming != mpeg4DelayParameter->mpeg4Delay) {
          WARN("Priming not equal to Codec Delay without the skipDelayParameter");
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_standardDelayCalculation(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_IIS_SWDECI const hSwDeci,
    DELAY_PARAMETER* const delayParameter,
    MPEG4_DELAY* const mpeg4DelayParameter) {
  int additionalSbrDelay = 0;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int mpeg4AddEncDelay = 0;
  int mpeg4AddFlushing = 0;
  int mpeg4StandDelay = 0;

  if (hConfig == NULL || mpeg4DelayParameter == NULL || delayParameter == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->bUseSBR && hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  else if (hConfig->stereoConfigIndex > 0 && hMpegsEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (hConfig->bUseSBR) {
    int mpsDelay = 0;

    if (hConfig->stereoConfigIndex == 3) {
      mpsDelay = 384;
    }

    retValue = iisxHEAACEncLibSbrInitDelayCompensation(hSbrEnc,
                                                       (int)(hConfig->sbrRatio.upFac *
                                                             (delayParameter->aacCoreCoderDelay + delayParameter->coreResamplerDelay) / hConfig->sbrRatio.downFac),
                                                       mpsDelay,
                                                       &additionalSbrDelay);
  }

  if (!isError(retValue)) {
    mpeg4AddEncDelay =
        hConfig->sbrRatio.upFac * (delayParameter->aacCoreCoderAddEncDelay + delayParameter->coreResamplerDelay) / (hConfig->sbrRatio.downFac);
    mpeg4StandDelay =
        hConfig->sbrRatio.upFac * (delayParameter->aacCoreCoderStandDelay) / (hConfig->sbrRatio.downFac);

    mpeg4AddEncDelay += additionalSbrDelay;

    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      mpeg4StandDelay += delayParameter->sbrDecoderDelay;

      if (hConfig->stereoConfigIndex == 1 || hConfig->stereoConfigIndex == 2) {
        mpeg4StandDelay -= 384;
      } else if (hConfig->stereoConfigIndex == 3) {
        if (hConfig->bUseSBR41) {
          mpeg4StandDelay -= 768;
        } else {
          mpeg4StandDelay -= 384;
        }
      }
    } else {
      mpeg4AddFlushing += delayParameter->sbrDecoderDelay;
    }

    if (hConfig->bUseSBR) {
      if (hConfig->aot == AUD_OBJ_TYP_USAC) {
        if (hConfig->stereoConfigIndex <= 0) {
          if (hConfig->bUseSBR41) {
            mpeg4StandDelay += (160 - 16 + 1) * 4;
          } else {
            mpeg4StandDelay += (320 - 32 + 1) * 2;
          }
        }
      } else {
        mpeg4AddFlushing += (320 - 32 + 1) * 2;
      }
    }
  }

  if ((!isError(retValue)) &&
      (hConfig->stereoConfigIndex > 0)) {
    int nSamplesFrameMax;

    retValue = iisxHEAACEncLibMpegsEncInitDelayCompensation(hMpegsEnc,
                                                            mpeg4AddEncDelay,
                                                            mpeg4StandDelay,
                                                            &mpeg4AddEncDelay,
                                                            &mpeg4StandDelay,
                                                            &nSamplesFrameMax);

    if (!isError(retValue)) {
      if (hConfig->bUseSBR41) {
        mpeg4StandDelay += ((160 - 16 + 1) * 4) - (640 - 64 + 1);
      } else {
        mpeg4StandDelay += ((320 - 32 + 1) * 2) - (640 - 64 + 1);
      }
    }

    if (!isError(retValue)) {
      if (hSwDeci != NULL) {
        int mpegsInputDelay = 320 + 6 * 64 + nSamplesFrameMax + delayParameter->preResamplerDelay;

        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = iisSwitchingDecisionAttach(hSwDeci,
                                               SWDECI_ID_MPEGS_212,
                                               hConfig->sampleRateOut,
                                               nSamplesFrameMax,
                                               mpegsInputDelay);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (hConfig->stereoConfigIndex == 3) {
      if (hConfig->bUseSBR41) {
        mpeg4StandDelay += 768;
      } else {
        mpeg4StandDelay += 384;
      }
    }

    mpeg4AddEncDelay += delayParameter->preResamplerDelay;
  }

  if (!isError(retValue)) {
    mpeg4DelayParameter->mpeg4StandDelay = mpeg4StandDelay;
    mpeg4DelayParameter->mpeg4AddFlushing = mpeg4AddFlushing;
    mpeg4DelayParameter->mpeg4DelayWOSwitching = mpeg4DelayParameter->mpeg4StandDelay + mpeg4AddEncDelay;
    mpeg4DelayParameter->mpeg4Delay = 0;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_delayAndBuffer_fillEncBuffer(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SWDECI* const switchingDecision,
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    XHEAACENCLIB_ENCBUFFER* const encBufferData,
    XHEAACENCLIB_ENCODER_STATE const encoderState,
    int* const nSamples) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int channelSamples = 0;
  float* pSamples = NULL;
  int nChannelSamplesAvailable = 0;

  if (hConfig == NULL || collectiveBuffer == NULL || encBufferData == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (*nSamples % hConfig->nInChannels) {
    retValue = XHEAACENCLIB_RETURN_ERROR_nSAMPLES_NOT_MULTIPLE_nIP_CHANNELS;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    nChannelSamplesAvailable = *nSamples / hConfig->nInChannels;
    errorInfo = MP4TIMEBUF_SaveAccessBuffer(collectiveBuffer, 0, nChannelSamplesAvailable, 0, &pSamples);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    channelSamples = *nSamples / hConfig->nInChannels;
    errorInfo = MP4TIMEBUF_FeedBufferMulti(encBufferData->hEncBuffer, pSamples, &channelSamples);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    int nSamplesToInvalidate = nChannelSamplesAvailable - channelSamples;
    errorInfo = MP4TIMEBUF_InvalidateBuffer(collectiveBuffer, nSamplesToInvalidate);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    encBufferData->encBufValidSamples += *nSamples;
    *nSamples = channelSamples * hConfig->nInChannels;
    encBufferData->encBufValidSamples -= *nSamples;
  }

  if ((switchingDecision->hSwDeci != NULL) && (switchingDecision->swInfo.bufferSize > 0)) {
    int nSamplesNextSwDeci = 0;
    float* tmpInputBuffer = NULL;

    if (!isError(retValue)) {
      HANDLE_ERROR_INFO errorInfo = noError;
      if (encoderState == XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
        switchingDecision->nSwSamplesNextFrame = min(switchingDecision->nSwSamplesNextFrame, (encBufferData->encBufValidSamples / (int)hConfig->nInChannels) - switchingDecision->nSwDeciBufferOffset);
        switchingDecision->nSwSamplesNextFrame = max(0, switchingDecision->nSwSamplesNextFrame);
      }
      errorInfo = MP4TIMEBUF_SaveAccessBuffer(encBufferData->hEncBuffer, switchingDecision->nSwDeciBufferOffset, switchingDecision->nSwSamplesNextFrame, 0, &tmpInputBuffer);
      if (errorInfo != noError)
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SAVE_TIMEBUFFER;
    }

    if (!isError(retValue)) {
      HANDLE_ERROR_INFO errorInfo = noError;
      switchingDecision->nSwDeciBufferOffset += switchingDecision->nSwSamplesNextFrame;
      switchingDecision->nSwSamplesNextFrame *= hConfig->nInChannels;
      errorInfo = iisSwitchingDecisionFeedSamples(switchingDecision->hSwDeci,
                                                  tmpInputBuffer,
                                                  &switchingDecision->nSwSamplesNextFrame,
                                                  &nSamplesNextSwDeci);
      if (errorInfo != noError)
        retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION;

      if (switchingDecision->nSwSamplesNextFrame % hConfig->nInChannels != 0) {
        retValue = XHEAACENCLIB_RETURN_ERROR_nSAMPLES_NOT_MULTIPLE_nIP_CHANNELS;
      } else {
        switchingDecision->nSwSamplesNextFrame /= hConfig->nInChannels;
        switchingDecision->nSwDeciBufferOffset -= switchingDecision->nSwSamplesNextFrame;
      }
    }

    if (!isError(retValue) && nSamplesNextSwDeci > 0) {
      if (encoderState != XHEAACENCLIB_ENCODER_STATE_FLUSHING) {
        retValue = XHEAACENCLIB_RETURN_ERROR_ENC_NOT_FLUSHING_TOO_LESS_SAMPLES;
      } else {
        int tmp_fill_zero = nSamplesNextSwDeci * hConfig->nInChannels;
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = iisSwitchingDecisionFeedSamples(switchingDecision->hSwDeci, NULL, &tmp_fill_zero, &nSamplesNextSwDeci);
        if (errorInfo != noError)
          retValue = XHEAACENCLIB_RETURN_ERROR_IIS_SWITCHING_DECISION_FEED;
        if (tmp_fill_zero) {
          retValue = XHEAACENCLIB_RETURN_ERROR_BUFFER_SWITCHING_DECISION_TOO_SMALL;
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_collectiveBuffer_feedSamples(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    float const* const pSamples,
    unsigned int const nSamples) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == pSamples || NULL == hConfig) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (0 == hConfig->nInChannels) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    int nSamplesPerChannel = nSamples / hConfig->nInChannels;
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = MP4TIMEBUF_FeedBufferMulti(collectiveBuffer,
                                           pSamples,
                                           &nSamplesPerChannel);

    assert(nSamplesPerChannel == 0);

    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_collectiveBuffer_getNumAvailableSamples(
    HANDLE_MP4TIMEBUF const collectiveBuffer,
    unsigned int const nChannels,
    int* nSamplesAvailable) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == collectiveBuffer || NULL == nSamplesAvailable) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = MP4TIMEBUF_getValidSamples(collectiveBuffer, nSamplesAvailable);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    } else {
      *nSamplesAvailable *= nChannels;
    }
  }

  return retValue;
}
