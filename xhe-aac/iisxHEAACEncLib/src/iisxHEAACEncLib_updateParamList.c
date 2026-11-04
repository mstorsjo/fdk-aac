
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

#include "iisxHEAACEncLib_updateParamList.h"

#include "mpeg4_profiles.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static AUDIO_PROFILE_LEVEL getProfileLevelIndication(const AUD_OBJ_TYP trueAot,
                                                     const int sampleRateCore,
                                                     const int sampleRateOut,
                                                     const SBR_SIGNALING_MODE sbrSig,
                                                     const XHEAACENCLIB_SIGMAP_HANDLE hSigMap) {
  AUDIO_PROFILE_LEVEL profLev = MAIN_AUDIO_PROFILE_L4;
  int effCh = hSigMap->nEffectiveChannels;
  AUD_OBJ_TYP aot = trueAot;

  if ((aot == AUD_OBJ_TYP_HEAAC) || (aot == AUD_OBJ_TYP_PS)) {
    if ((sbrSig == SBR_SIGNALING_IMPLICIT) || (sbrSig == SBR_SIGNALING_EXPL_BC)) {
      aot = AUD_OBJ_TYP_LC;
    }
  }

  switch (aot) {
    case AUD_OBJ_TYP_LC:
    case AUD_OBJ_TYP_MP2_LC:
    case AUD_OBJ_TYP_MP2_SBR:
      if (effCh <= 2 && sampleRateCore <= 24000) {
        profLev = AAC_PROFILE_L1;
      } else if (effCh <= 2 && sampleRateCore <= 48000) {
        profLev = AAC_PROFILE_L2;
      } else if (effCh <= 5 && sampleRateCore <= 48000) {
        profLev = AAC_PROFILE_L4;
      } else if (effCh <= 5 && sampleRateCore <= 96000) {
        profLev = AAC_PROFILE_L5;
      } else if (effCh <= 7 && sampleRateCore <= 48000) {
        profLev = AAC_PROFILE_L6;
      } else if (effCh <= 7 && sampleRateCore <= 96000) {
        profLev = AAC_PROFILE_L7;
      } else {
        profLev = MAIN_AUDIO_PROFILE_L4;
      }
      break;

    case AUD_OBJ_TYP_HEAAC:
      if (sbrSig == SBR_SIGNALING_DISABLE) {
      }
      if (effCh <= 2 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L2;
      } else if (effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L3;
      } else if ((effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) || (effCh <= 5 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000)) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L4;
      } else if (effCh <= 5 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 96000) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L5;
      } else if ((effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) || (effCh <= 7 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000)) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L6;
      } else if (effCh <= 7 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 96000) {
        profLev = HIGH_EFFICIENCY_AAC_PROFILE_L7;
      } else {
        profLev = MAIN_AUDIO_PROFILE_L4;
      }
      break;

    case AUD_OBJ_TYP_PS:
      if (effCh <= 2 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) {
        profLev = HIGH_EFFICIENCY_AAC_V2_PROFILE_L2;
      } else if (effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) {
        profLev = HIGH_EFFICIENCY_AAC_V2_PROFILE_L3;
      } else if ((effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) || (effCh <= 5 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000)) {
        profLev = HIGH_EFFICIENCY_AAC_V2_PROFILE_L4;
      }

      else if ((effCh <= 2 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000) || (effCh <= 7 && (sampleRateCore <= 24000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 48000)) {
        profLev = HIGH_EFFICIENCY_AAC_V2_PROFILE_L6;
      } else if (effCh <= 7 && (sampleRateCore <= 48000 || sbrSig == SBR_SIGNALING_DISABLE) && sampleRateOut <= 96000) {
        profLev = HIGH_EFFICIENCY_AAC_V2_PROFILE_L7;
      } else {
        profLev = MAIN_AUDIO_PROFILE_L4;
      }
      break;

    case AUD_OBJ_TYP_USAC:

      if (effCh <= 2 && sampleRateOut <= 48000) {
        profLev = EXTENDED_HE_AAC_PROFILE_L2;
      } else if (effCh <= 5 && sampleRateOut <= 48000) {
        profLev = BASELINE_USAC_PROFILE_L3;
      } else if (effCh <= 5 && sampleRateOut <= 96000) {
        profLev = BASELINE_USAC_PROFILE_L4;
      } else {
        profLev = NO_AUDIO_PROFILE_SPECIFIED;
      }
      break;

    default:

      break;
  }

  if (profLev == NO_AUDIO_PROFILE_SPECIFIED || (profLev == MAIN_AUDIO_PROFILE_L4 && effCh != 22)) {
    assert(0);
  }

  return profLev;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_updateParamList(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_AUDIOPREROLL_DATA const* const audioPreRoll,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    XHEAACENCLIB_SIGMAP_HANDLE const hSigMap,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    XHEAACENCLIB_BITRESERVOIR_DATA* const bitReservoirData,
    MPEG4_DELAY const* const mpeg4DelayParameter,
    TIME_SIGNAL_DATA const* const timeSignal,
    float const aacCoreBandwidth,
    int const rapFrameInAdvance,
    unsigned int const numAncBytes) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIO_PROFILE_LEVEL audioProfileLevel = NO_AUDIO_PROFILE_SPECIFIED;
  PARAMLIST_SAMPLERATE srOut = PARAMLIST_SAMPLERATE_INVALID;
  XHEAACENCLIB_AACINFO aacInfo;
  int maxFrameBits;
  int nMaxBitRate = 0;
  int nBitResMax = 0;
  float framesPerSecond;
  float bandwidth = 0.0f;
  float maxBitsPerSecond = 0.0f;
  float maxBitsPerSegment = 0.0f;
  float bitsPerFrame = 0.0f;
  int nMaxBitRatePerSegment = 0;
  int minSamplesInAdvanceRap = 0;

  if ((hConfig == NULL) || (audioPreRoll == NULL) || (hAacEnc == NULL) || (hSigMap == NULL) || (hCodecParamList == NULL) || (bitReservoirData == NULL) || (mpeg4DelayParameter == NULL) || (timeSignal == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibAacEncGetInfo(hAacEnc, &aacInfo);
    audioProfileLevel = getProfileLevelIndication(hConfig->aot,
                                                  hConfig->sampleRateAAC,
                                                  hConfig->sampleRateOut,
                                                  hConfig->sbrSignaling,
                                                  hSigMap);
  }

  if (!isError(retValue)) {
    maxFrameBits = hSigMap->nEffectiveChannels * MAX_AAC_CHANNEL_BITS;
    framesPerSecond = (float)hConfig->sampleRateOut / (float)hConfig->nFrameSamples;
    bitsPerFrame = (float)hConfig->bitRate / framesPerSecond;

    maxBitsPerSecond = bitsPerFrame * ((float)framesPerSecond - 1.0f) + maxFrameBits;
    if (hConfig->randomAccessIntervalInFrames > 0) {
      maxBitsPerSegment = bitsPerFrame * ((float)hConfig->randomAccessIntervalInFrames - 1.0f) + maxFrameBits;
    }
    if (audioPreRoll->hAudioPreRoll != NULL && (audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT)) {
      int nTmpAU = iisAudioPreRollLibGetnPreRollAu(audioPreRoll->hAudioPreRoll);
      nTmpAU = max(0, nTmpAU);

      if (nTmpAU > 0) {
        maxBitsPerSecond += bitsPerFrame * (nTmpAU - 1) + maxFrameBits + iisAudioPreRollLibPayloadSizeWithoutAUs(audioPreRoll->hAudioPreRoll);
      } else {
        maxBitsPerSecond += bitsPerFrame * nTmpAU + iisAudioPreRollLibPayloadSizeWithoutAUs(audioPreRoll->hAudioPreRoll);
      }
      if (hConfig->randomAccessIntervalInFrames > 0) {
        maxBitsPerSegment += bitsPerFrame * nTmpAU + iisAudioPreRollLibPayloadSizeWithoutAUs(audioPreRoll->hAudioPreRoll);
      }
    }
    nMaxBitRate = (int)ceil(maxBitsPerSecond);
    if (hConfig->randomAccessIntervalSamples > 0) {
      float f_nMaxBitRatePerSegment = maxBitsPerSegment / ((float)hConfig->randomAccessIntervalSamples) * (float)(hConfig->sampleRateOut);
      nMaxBitRatePerSegment = (int)ceil(f_nMaxBitRatePerSegment);
    } else {
      nMaxBitRatePerSegment = 0;
    }
    nBitResMax = maxFrameBits;

    switch (hConfig->sampleRateOut) {
      case 192000:
        srOut = PARAMLIST_SAMPLERATE_192000;
        break;
      case 176400:
        srOut = PARAMLIST_SAMPLERATE_176400;
        break;
      case 96000:
        srOut = PARAMLIST_SAMPLERATE_96000;
        break;
      case 88200:
        srOut = PARAMLIST_SAMPLERATE_88200;
        break;
      case 64000:
        srOut = PARAMLIST_SAMPLERATE_64000;
        break;
      case 48000:
        srOut = PARAMLIST_SAMPLERATE_48000;
        break;
      case 44100:
        srOut = PARAMLIST_SAMPLERATE_44100;
        break;
      case 40000:
        srOut = PARAMLIST_SAMPLERATE_40000;
        break;
      case 38400:
        srOut = PARAMLIST_SAMPLERATE_38400;
        break;
      case 35280:
        srOut = PARAMLIST_SAMPLERATE_35280;
        break;
      case 32000:
        srOut = PARAMLIST_SAMPLERATE_32000;
        break;
      case 29400:
        srOut = PARAMLIST_SAMPLERATE_29400;
        break;
      case 24000:
        srOut = PARAMLIST_SAMPLERATE_24000;
        break;
      case 22050:
        srOut = PARAMLIST_SAMPLERATE_22050;
        break;
      case 19200:
        srOut = PARAMLIST_SAMPLERATE_19200;
        break;
      case 16000:
        srOut = PARAMLIST_SAMPLERATE_16000;
        break;
      case 12000:
        srOut = PARAMLIST_SAMPLERATE_12000;
        break;
      case 11025:
        srOut = PARAMLIST_SAMPLERATE_11025;
        break;
      case 9600:
        srOut = PARAMLIST_SAMPLERATE_9600;
        break;
      case 8000:
        srOut = PARAMLIST_SAMPLERATE_8000;
        break;
      case 6000:
        srOut = PARAMLIST_SAMPLERATE_6000;
        break;
      default:
        assert(0);
        retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
        break;
    }
  }

  if (!isError(retValue)) {
    if (hConfig->bUseSBR) {
      if (hSbrEnc == NULL) {
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
      }

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLibSbrEncGetStopFreq(hSbrEnc, &bandwidth);
      }
    } else {
      bandwidth = aacCoreBandwidth;
    }
  }

  if (!isError(retValue)) {
    iisxHEAACEncLibAacEncGetBitReservoirInfo(hAacEnc, bitReservoirData->bitReservoirMax, &bitReservoirData->bitReservoir, &bitReservoirData->bitReservoirLevel);
  }

  if (!isError(retValue)) {
    if (rapFrameInAdvance >= 0) {
      if ((hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) && (hConfig->aot == AUD_OBJ_TYP_USAC) && (hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE || hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SEEKABLE) && (hConfig->audioPreRollBitResMode != XHEAACENCLIB_APR_BITRESMODE_OUT)) {
        minSamplesInAdvanceRap = hConfig->randomAccessIntervalMin - hConfig->nFrameSamples;
      }
      minSamplesInAdvanceRap = max(minSamplesInAdvanceRap, (rapFrameInAdvance) * (int)hConfig->nFrameSamples);
    } else {
      minSamplesInAdvanceRap = -1;
    }
  }

  if (!isError(retValue)) {
    if (mpeg4DelayParameter->mpeg4Priming != 0 && hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE) {
      WARN("Priming is not equal to 0");
    } else if (mpeg4DelayParameter->mpeg4Priming != mpeg4DelayParameter->mpeg4StandDelay && hConfig->primingMode == XHEAACENC_PRIMINGMODE_STDDELAY) {
      WARN("Priming is not equal to standard Delay");
    }

    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_BITRATELIMIT, nMaxBitRate, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MAXBITRATEPERSEGMENT, nMaxBitRatePerSegment, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRBITS_MAX, nBitResMax, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_CODECDELAY, hConfig->codecDelay, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_PRIMING, mpeg4DelayParameter->mpeg4Priming, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_STANDARDDELAY, mpeg4DelayParameter->mpeg4StandDelay, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_PROFILE_LEVEL, audioProfileLevel, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_NEXT, timeSignal->nSamplesUntilNext, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_LEFT, 0, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SAMPLES_MAX, timeSignal->nSamplesMax, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_ADDITIONAL_SAMPLE_REQUEST, timeSignal->additionalSampleRequest, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MIN_OUTBUF_SIZE, timeSignal->minOutBufSize, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MIN_INBUF_SIZE, timeSignal->minInBufSize, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_ANC_BYTES_PER_FRAME, timeSignal->nAncBytesPerFrame, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_ANC_BYTES_LEFT, numAncBytes, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_OUTSAMPLERATE, srOut, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_AUDIOBANDWIDTH, bandwidth, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRBITS, bitReservoirData->bitReservoir, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_BITRESERVOIRLEVEL, bitReservoirData->bitReservoirLevel, PARAMLIST_MODE_REPLACE);
    iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_MIN_SAMPLES_IN_ADVANCE, minSamplesInAdvanceRap, PARAMLIST_MODE_REPLACE);
  }

  return retValue;
}
