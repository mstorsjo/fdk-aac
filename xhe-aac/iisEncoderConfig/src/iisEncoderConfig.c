
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

#define ENCCONFIG_VERSION_NUMBER "02.04.05"
#define ENCCONFIG_MODULE_NAME "iisEncoderConfig"

#define ENCCONFIG_BUILD_DATE __DATE__
#define ENCCONFIG_BUILD_INFO "Release build"

#if defined(__GNUC__)
#define ENCCONFIG_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
#define ENCCONFIG_COMPILER_INFO "Compiler: GCC"
#elif defined(_MSC_VER)
#define ENCCONFIG_COMPILER_VERSION _MSC_VER
#define ENCCONFIG_COMPILER_INFO "Compiler: Visual C"
#else
#define ENCCONFIG_COMPILER_VERSION 0
#define ENCCONFIG_COMPILER_INFO "Compiler: unknown"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iisutillib.h"
#include "iisEncoderConfig.h"
#include "iisEncoderConfigCTable.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define ENCCONFIG_SMC_INTERVAL_DEFAULT (233472)
#define ENCCONFIG_USAC_MAX_IF_INTERVAL_DEFAULT_MS (2000)
#define ENCCONFIG_SMC_INTERVAL_DEFAULT_MS (5000)
#define ENCCONFIG_SMC_INTERVAL_SEEKABLE_DEFAULT_MS (10000)
#define ENCCONFIG_SMC_INTERVAL_STEPS_RECOMMENDED (4096)
#define ENCCONFIG_SMC_ABSOLUTE_RAP_MIN_INTERVAL_MS_USAC (855)
#define ENCCONFIG_SMC_ABSOLUTE_RAP_MIN_INTERVAL_MS_GENERAL (150)
#define ENCCONFIG_MILLISECONDS_IN_A_SECOND (1000)
#define ENCCONFIG_DEFAULT_DRC_TARGET_LOUDNESS_RANGE (10.0f)
#define ENCCONFIG_MAX_DRC_SEQUENCES (4)
#define ENCCONFIG_BITRATE_RESTRICTION_FACTOR (2000)

typedef struct encconfig_private_data_struct {
  int nSize;
  ENCCONFIGCTAB_INSTANCE_HANDLE hEncoderConfigCTable;

} ENCCONFIG_PRIVATE_DATA, *ENCCONFIG_PRIVATE_DATA_HANDLE;

typedef struct encconfig_defaultCbr {
  PARAMLIST_AOT aot;
  PARAMLIST_CHANNELCONFIG channelConfig;
  int maxBitrate;
  int defaultBitrate;
} ENCCONFIG_DEFAULTCBR;

static const ENCCONFIG_DEFAULTCBR defaultCbr[] = {
    {PARAMLIST_AOT_42, PARAMLIST_CHANNELCONFIG_MONO, 32000, 32000},
    {PARAMLIST_AOT_42, PARAMLIST_CHANNELCONFIG_STEREO, 64000, 64000},
};

typedef struct encconfig_defaultMode {
  PARAMLIST_BITRATEMODE bitratemode;
  PARAMLIST_CHANNELCONFIG channelConfig;
  PARAMLIST_AOT aot;
  PARAMLIST_BITRATE bitrate;
} ENCCONFIG_DEFAULTMODE;

static const ENCCONFIG_DEFAULTMODE defaultMode[] = {

    {PARAMLIST_BITRATEMODE_VBR0, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 20000},
    {PARAMLIST_BITRATEMODE_VBR0, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 24000},
    {PARAMLIST_BITRATEMODE_VBR1, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 32000},
    {PARAMLIST_BITRATEMODE_VBR1, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 40000},
    {PARAMLIST_BITRATEMODE_VBR2, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 40000},
    {PARAMLIST_BITRATEMODE_VBR2, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 64000},
    {PARAMLIST_BITRATEMODE_VBR3, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 56000},
    {PARAMLIST_BITRATEMODE_VBR3, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 96000},
    {PARAMLIST_BITRATEMODE_VBR4, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 72000},
    {PARAMLIST_BITRATEMODE_VBR4, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 128000},

    {PARAMLIST_BITRATEMODE_VBR5, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 104000},
    {PARAMLIST_BITRATEMODE_VBR5, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 192000},

    {PARAMLIST_BITRATEMODE_VBR6, PARAMLIST_CHANNELCONFIG_MONO, PARAMLIST_AOT_42, 136000},
    {PARAMLIST_BITRATEMODE_VBR6, PARAMLIST_CHANNELCONFIG_STEREO, PARAMLIST_AOT_42, 256000},
};

static ENCCONFIG_PRIVATE_DATA_HANDLE iisEncoderConfigGetPrivateDataHandle(
    ENCCONFIG_INSTANCE_HANDLE const hInstance);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetStereoConfigIdxDependingVals(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,
    PARAMLIST_INSTANCE_HANDLE hUserParamList);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetSbrRatioDependingVals(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,
    PARAMLIST_INSTANCE_HANDLE hUserParamList);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSanityCheck(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetMandatoryParameters(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetDefaultParameters(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetDefaultParametersAACMetadata(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

static int isError(
    ENCODERCONFIG_RETURN_CODE errorValue);

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSanityCheckMpeg4Metadata(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList);

static ENCCONFIG_PRIVATE_DATA_HANDLE iisEncoderConfigGetPrivateDataHandle(
    ENCCONFIG_INSTANCE_HANDLE const hInstance) {
  ENCCONFIG_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (ENCCONFIG_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct encconfig_instance_struct));
  return hPrivateData;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetStereoConfigIdxDependingVals(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,
    PARAMLIST_INSTANCE_HANDLE hUserParamList) {
  PARAM_INSTANCE_HANDLE hparam_stereoConfigIdx = NULL;
  PARAM_INSTANCE_HANDLE hparam_tsd = NULL;
  PARAM_INSTANCE_HANDLE hparam_ch_mode = NULL;

  int stereoConfigIdx = -1;
  int bUseTSD = 0;

  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO retInfo = noError;

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hparam_ch_mode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (PARAMLIST_CHANNELCONFIG_STEREO == hparam_ch_mode->paramValue._int) {
      if (iisParamListParamExists(hUserParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX)) {
        retInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, &hparam_stereoConfigIdx);
        if (retInfo != noError) {
          retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
        }
      } else {
        retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, &hparam_stereoConfigIdx);
        if (retInfo != noError) {
          retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
        }
      }

      if (!isError(retError)) {
        if (hparam_stereoConfigIdx) {
          stereoConfigIdx = hparam_stereoConfigIdx->paramValue._int;
        }
      }

      if (!isError(retError)) {
        if (stereoConfigIdx == PARAMLIST_STEREOCONFIGIDX_2 || stereoConfigIdx == PARAMLIST_STEREOCONFIGIDX_3) {
          retInfo = iisParamListRemoveParamTag(hUserParamList, PARAMLIST_PARAMETER_TSD);
          if (retInfo != noError) {
            retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
          }
          if (!isError(retError)) {
            retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TSD, PARAMLIST_TSD_OFF, PARAMLIST_MODE_REPLACE);
            if (retInfo != noError) {
              retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
            }
          }
        } else if (iisParamListParamExists(hUserParamList, PARAMLIST_PARAMETER_TSD)) {
          if (!isError(retError)) {
            retInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_TSD, &hparam_tsd);
            if (retInfo != noError) {
              retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
            }
          }
          bUseTSD = hparam_tsd->paramValue._int;
          if (!isError(retError)) {
            retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TSD, bUseTSD, PARAMLIST_MODE_REPLACE);
            if (retInfo != noError) {
              retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
            }
          }
        }
      }
    }
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetSbrRatioDependingVals(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,
    PARAMLIST_INSTANCE_HANDLE hUserParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errInfo = noError;

  PARAM_INSTANCE_HANDLE hparam_sbrratio = NULL;
  PARAM_INSTANCE_HANDLE hparam_aot = NULL;
  PARAM_INSTANCE_HANDLE hparam_granule = NULL;

  PARAMLIST_SBRRATIO sbrratio = PARAMLIST_SBRRATIO_INVALID;
  PARAMLIST_GRANULELENGTH granule = PARAMLIST_GRANULELENGTH_1024;
  PARAMLIST_SBR sbr = PARAMLIST_SBR_OFF;
  PARAMLIST_FRAMESAMPLES fsamples = PARAMLIST_FRAMESAMPLES_1024;
  PARAMLIST_AOT aot = PARAMLIST_AOT_INVALID;

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_SBRRATIO)) {
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_SBRRATIO, &hparam_sbrratio);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    } else if (iisParamListParamExists(hUserParamList, PARAMLIST_PARAMETER_SBRRATIO)) {
      errInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_SBRRATIO, &hparam_sbrratio);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }
  if (!isError(retError)) {
    if (hparam_sbrratio != NULL) {
      sbrratio = (PARAMLIST_SBRRATIO)hparam_sbrratio->paramValue._int;
    }
  }
  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_AOT)) {
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AOT, &hparam_aot);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    } else if (iisParamListParamExists(hUserParamList, PARAMLIST_PARAMETER_AOT)) {
      errInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_AOT, &hparam_aot);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }
  if (!isError(retError)) {
    if (hparam_aot != NULL) {
      aot = (PARAMLIST_AOT)hparam_aot->paramValue._int;
    }
  }
  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_GRANULELENGTH)) {
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_GRANULELENGTH, &hparam_granule);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    } else if (iisParamListParamExists(hUserParamList, PARAMLIST_PARAMETER_GRANULELENGTH)) {
      errInfo = iisParamListGetParam(hUserParamList, PARAMLIST_PARAMETER_GRANULELENGTH, &hparam_granule);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }
  if (!isError(retError)) {
    if (hparam_granule != NULL) {
      granule = (PARAMLIST_GRANULELENGTH)hparam_granule->paramValue._int;
    }
  }

  switch (aot) {
    case PARAMLIST_AOT_42:
    case PARAMLIST_AOT_INVALID:
    default:
      switch (sbrratio) {
        case PARAMLIST_SBRRATIO_1_1:

          granule = PARAMLIST_GRANULELENGTH_1024;
          sbr = PARAMLIST_SBR_ON;
          fsamples = PARAMLIST_FRAMESAMPLES_1024;
          break;
        case PARAMLIST_SBRRATIO_2_1:
          granule = PARAMLIST_GRANULELENGTH_1024;
          sbr = PARAMLIST_SBR_ON;
          fsamples = PARAMLIST_FRAMESAMPLES_2048;
          break;
        case PARAMLIST_SBRRATIO_4_1:
          granule = PARAMLIST_GRANULELENGTH_1024;
          sbr = PARAMLIST_SBR_ON;
          fsamples = PARAMLIST_FRAMESAMPLES_4096;
          break;
        case PARAMLIST_SBRRATIO_8_3:
          granule = PARAMLIST_GRANULELENGTH_768;
          sbr = PARAMLIST_SBR_ON;
          fsamples = PARAMLIST_FRAMESAMPLES_2048;
          break;
        case PARAMLIST_SBRRATIO_NONE:
        case PARAMLIST_SBRRATIO_INVALID:
        default:
          if (granule == PARAMLIST_GRANULELENGTH_768) {
            granule = PARAMLIST_GRANULELENGTH_768;
            sbr = PARAMLIST_SBR_OFF;
            fsamples = PARAMLIST_FRAMESAMPLES_768;
          } else {
            granule = PARAMLIST_GRANULELENGTH_1024;
            sbr = PARAMLIST_SBR_OFF;
            fsamples = PARAMLIST_FRAMESAMPLES_1024;
          }
          break;
      }
      break;
  }
  if (!isError(retError)) {
    errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SBR, sbr, PARAMLIST_MODE_REPLACE);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }
  if (!isError(retError)) {
    errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_GRANULELENGTH, granule, PARAMLIST_MODE_REPLACE);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }
  if (!isError(retError)) {
    errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_FRAMESAMPLES, fsamples, PARAMLIST_MODE_REPLACE);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }
  if (!isError(retError)) {
    errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SBRRATIO, sbrratio, PARAMLIST_MODE_REPLACE);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigConvertRapIntMsToSamplesRoundDown(const int rapIntDistanceMs, const int outputSamplingRate, const int rapFrameAlignment, int *const rapIntSamples) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  const int timeScale = ENCCONFIG_MILLISECONDS_IN_A_SECOND;

  if (rapIntSamples == NULL) {
    retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
  } else {
    *rapIntSamples = 0;
  }

  if (rapIntDistanceMs > 0) {
    if (!isError(retError)) {
      int maxRapIntDist = INT_MAX;

      if (outputSamplingRate > timeScale) {
        maxRapIntDist = INT_MAX / (outputSamplingRate / timeScale + 1);
      }

      if ((maxRapIntDist < rapIntDistanceMs) || (INT_MAX / (rapIntDistanceMs % timeScale + 1) < outputSamplingRate)) {
        *rapIntSamples = 0;
        retError = ENCODERCONFIG_ERROR_INVALID_RAP_INTERVAL;
      }
    }

    if (!isError(retError)) {
      const int rapIntSec = rapIntDistanceMs / timeScale;

      const int rapIntRemainder = rapIntDistanceMs - (rapIntSec * timeScale);

      const int rapIntDistSamples = (rapIntSec * outputSamplingRate) + (rapIntRemainder * outputSamplingRate) / timeScale;

      const int rapIntDistSamplesAligned = (rapIntDistSamples / rapFrameAlignment) * rapFrameAlignment;

      *rapIntSamples = rapIntDistSamplesAligned;
    }
  }

  return retError;
}

static int iisEncoderConfigGetSamplerateValue(PARAMLIST_SAMPLERATE samplerate) {
  int samplerate_val = -1;

  switch (samplerate) {
    case PARAMLIST_SAMPLERATE_192000:
      samplerate_val = 192000;
      break;
    case PARAMLIST_SAMPLERATE_176400:
      samplerate_val = 176400;
      break;
    case PARAMLIST_SAMPLERATE_96000:
      samplerate_val = 96000;
      break;
    case PARAMLIST_SAMPLERATE_88200:
      samplerate_val = 88200;
      break;
    case PARAMLIST_SAMPLERATE_76800:
      samplerate_val = 76800;
      break;
    case PARAMLIST_SAMPLERATE_70560:
      samplerate_val = 70560;
      break;
    case PARAMLIST_SAMPLERATE_64000:
      samplerate_val = 64000;
      break;
    case PARAMLIST_SAMPLERATE_58800:
      samplerate_val = 58800;
      break;
    case PARAMLIST_SAMPLERATE_48000:
      samplerate_val = 48000;
      break;
    case PARAMLIST_SAMPLERATE_44100:
      samplerate_val = 44100;
      break;
    case PARAMLIST_SAMPLERATE_40000:
      samplerate_val = 40000;
      break;
    case PARAMLIST_SAMPLERATE_38400:
      samplerate_val = 38400;
      break;
    case PARAMLIST_SAMPLERATE_35280:
      samplerate_val = 35280;
      break;
    case PARAMLIST_SAMPLERATE_32000:
      samplerate_val = 32000;
      break;
    case PARAMLIST_SAMPLERATE_29400:
      samplerate_val = 29400;
      break;
    case PARAMLIST_SAMPLERATE_24000:
      samplerate_val = 24000;
      break;
    case PARAMLIST_SAMPLERATE_22050:
      samplerate_val = 22050;
      break;
    case PARAMLIST_SAMPLERATE_19200:
      samplerate_val = 19200;
      break;
    case PARAMLIST_SAMPLERATE_17640:
      samplerate_val = 17640;
      break;
    case PARAMLIST_SAMPLERATE_16000:
      samplerate_val = 16000;
      break;
    case PARAMLIST_SAMPLERATE_14700:
      samplerate_val = 14700;
      break;
    case PARAMLIST_SAMPLERATE_12800:
      samplerate_val = 12800;
      break;
    case PARAMLIST_SAMPLERATE_12000:
      samplerate_val = 12000;
      break;
    case PARAMLIST_SAMPLERATE_11760:
      samplerate_val = 11760;
      break;
    case PARAMLIST_SAMPLERATE_11025:
      samplerate_val = 11025;
      break;
    case PARAMLIST_SAMPLERATE_9600:
      samplerate_val = 9600;
      break;
    case PARAMLIST_SAMPLERATE_8820:
      samplerate_val = 8820;
      break;
    case PARAMLIST_SAMPLERATE_8000:
      samplerate_val = 8000;
      break;
    case PARAMLIST_SAMPLERATE_7350:
      samplerate_val = 7350;
      break;
    case PARAMLIST_SAMPLERATE_6000:
      samplerate_val = 6000;
      break;
    default:
      break;
  }

  return samplerate_val;
}

static float iisEncoderConfigGetSbrRatioValue(PARAMLIST_SBRRATIO sbrratio) {
  float sbrratio_val = -1;

  switch (sbrratio) {
    case PARAMLIST_SBRRATIO_NONE:
      sbrratio_val = 1.0f;
      break;
    case PARAMLIST_SBRRATIO_2_1:
      sbrratio_val = 2.0f;
      break;
    case PARAMLIST_SBRRATIO_8_3:
      sbrratio_val = 8.0f / 3.0f;
      break;
    case PARAMLIST_SBRRATIO_4_1:
      sbrratio_val = 4.0f;
      break;
    case PARAMLIST_SBRRATIO_INVALID:
      sbrratio_val = 1.0f;
      break;
    default:
      break;
  }

  return sbrratio_val;
}

static int iisEncoderConfigGetGranuleLengthValue(PARAMLIST_GRANULELENGTH granulelength) {
  int granulelength_val = -1;

  switch (granulelength) {
    case PARAMLIST_GRANULELENGTH_768:
      granulelength_val = 768;
      break;
    case PARAMLIST_GRANULELENGTH_960:
      granulelength_val = 960;
      break;
    case PARAMLIST_GRANULELENGTH_1024:
      granulelength_val = 1024;
      break;
    default:
      break;
  }

  return granulelength_val;
}

static int iisEncoderConfigGetFrameSamplesValue(PARAMLIST_FRAMESAMPLES frameSamples) {
  int frameSamples_val = -1;

  switch (frameSamples) {
    case PARAMLIST_FRAMESAMPLES_768:
      frameSamples_val = 768;
      break;
    case PARAMLIST_FRAMESAMPLES_960:
      frameSamples_val = 960;
      break;
    case PARAMLIST_FRAMESAMPLES_1024:
      frameSamples_val = 1024;
      break;
    case PARAMLIST_FRAMESAMPLES_1920:
      frameSamples_val = 1920;
      break;
    case PARAMLIST_FRAMESAMPLES_2048:
      frameSamples_val = 2048;
      break;
    case PARAMLIST_FRAMESAMPLES_4096:
      frameSamples_val = 4096;
      break;
    default:
      break;
  }

  return frameSamples_val;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSanityCheck(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errInfo = noError;

  int inSamplerate_val = -1;
  int outSamplerate_val = -1;
  int granulelength_val = -1;
  int frameSamples_val = -1;
  float sbrratio_val = -1;
  int const defaultModeTableSize = sizeof(defaultMode) / sizeof(ENCCONFIG_DEFAULTMODE);
  int i = 0;

  PARAMLIST_AOT aot = PARAMLIST_AOT_INVALID;
  PARAMLIST_SBRRATIO sbrratio = PARAMLIST_SBRRATIO_INVALID;
  PARAMLIST_BITRATEMODE bitrateMode = PARAMLIST_BITRATEMODE_INVALID;
  PARAMLIST_BITRATE bitrate = 0;
  PARAMLIST_SAMPLERATE outSamplerate = PARAMLIST_SAMPLERATE_INVALID;
  PARAMLIST_SAMPLERATE inSamplerate = PARAMLIST_SAMPLERATE_INVALID;
  PARAMLIST_CHANNELCONFIG channelconfig = PARAMLIST_CHANNELCONFIG_INVALID;
  PARAMLIST_CONFIGSET configset = PARAMLIST_CONFIGSET_INVALID;
  PARAMLIST_COREMODE coremode = PARAMLIST_COREMODE_INVALID;
  PARAMLIST_GRANULELENGTH granulelength = PARAMLIST_GRANULELENGTH_INVALID;
  PARAMLIST_FRAMESAMPLES frameSamples = PARAMLIST_FRAMESAMPLES_INVALID;
  PARAMLIST_RAP_PROPERTY rapProperty = PARAMLIST_RAP_PROPERTY_INVALID;
  PARAMLIST_RAP_OCCURRENCE rapOccurrence = PARAMLIST_RAP_OCCURRENCE_INVALID;
  PARAMLIST_TRANSPORTFORMAT transportFormat = PARAMLIST_TRANSPORTFORMAT_INVALID;
  PARAMLIST_STEREOCONFIGIDX stereoConfigIdx = PARAMLIST_STEREOCONFIGIDX_INVALID;
  PARAMLIST_MPEG2AAC mpeg2AAC = PARAMLIST_MPEG2AAC_INVALID;
  PARAMLIST_PRIMING primingMode = PARAMLIST_PRIMING_INVALID;
  PARAMLIST_FLUSHINGMODE flushingMode = PARAMLIST_FLUSHINGMODE_INVALID;
  PARAMLIST_DRCMODE drcMode = PARAMLIST_DRCMODE_INVALID;
  PARAMLIST_DISABLELOUDNESS disableLoudness = PARAMLIST_DISABLELOUDNESS_INVALID;
  PARAMLIST_HBE hbe = PARAMLIST_HBE_INVALID;

  int streamID = -1;
  int rapIntSamples = -1;
  int rapIntMs = -1;
  int absRapMinIntSamples = -1;
  int indepFlagIntervalMs = -1;
  int indepFlagIntervalSamples = -1;
  int rapOnDemandAdvancedMode = -1;
  int audioPreRollBitResMode = -1;
  int loudnessLevelSet = 0;
  int loudnessLevelMeasuredSet = 0;
  int disableLoudnessMeasurement = 0;
  int quietLoudnessThresholdSet = 0;
  int albumLoudnessLevelSet = 0;
  int anchorLoudnessLevelSet = 0;
  int anchorLoudnessLevelMeasuredSet = 0;
  int lraControlDataSet = 0;
  int targetLraSet = 0;
  int samplePeakSet = 0;
  int liveLoudnessLevelSet = 0;
  int liveSamplePeakSet = 0;
  int liveModeSet = 0;
  int liveRelMaxGainSet = 0;
  PARAMLIST_LIVE_LRAC liveLraControl = PARAMLIST_LIVE_LRAC_INVALID;

  PARAMLIST_MPEG4_DRC_LIGHT_PROF mpeg4DrcLight = PARAMLIST_MPEG4_DRC_LIGHT_PROF_INVALID;
  int bMpeg4ProgRefLevelSet = 0;
  int bNoStartStopParamSet = 0;
  int validate = 0;
  int allowShortestRapInterval = 0;

  PARAMLIST_MPEG4_METADATA_MODE mpeg4MetadataMode = PARAMLIST_MPEG4_METADATA_MODE_INVALID;

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_aot = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AOT, &hparam_aot);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_aot) aot = (PARAMLIST_AOT)hparam_aot->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_configset = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CONFIGSET, &hparam_configset);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_configset) configset = (PARAMLIST_CONFIGSET)hparam_configset->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_bitrate = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_BITRATE, &hparam_bitrate);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_bitrate) bitrate = (PARAMLIST_BITRATE)hparam_bitrate->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_bitrateMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_BITRATEMODE, &hparam_bitrateMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_bitrateMode) bitrateMode = (PARAMLIST_BITRATEMODE)hparam_bitrateMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_outSamplerate = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_OUTSAMPLERATE, &hparam_outSamplerate);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_outSamplerate) outSamplerate = (PARAMLIST_SAMPLERATE)hparam_outSamplerate->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_inSamplerate = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_INSAMPLERATE, &hparam_inSamplerate);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_inSamplerate) inSamplerate = (PARAMLIST_SAMPLERATE)hparam_inSamplerate->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_channelconfig = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hparam_channelconfig);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_channelconfig) channelconfig = (PARAMLIST_CHANNELCONFIG)hparam_channelconfig->paramValue._int;
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_SBRRATIO)) {
      PARAM_INSTANCE_HANDLE hparam_sbrratio = NULL;
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_SBRRATIO, &hparam_sbrratio);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hparam_sbrratio) sbrratio = (PARAMLIST_SBRRATIO)hparam_sbrratio->paramValue._int;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_COREMODE)) {
      PARAM_INSTANCE_HANDLE hparam_coremode = NULL;
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_COREMODE, &hparam_coremode);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hparam_coremode) coremode = (PARAMLIST_COREMODE)hparam_coremode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX)) {
      PARAM_INSTANCE_HANDLE hparam_stereoConfigIdx = NULL;
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, &hparam_stereoConfigIdx);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hparam_stereoConfigIdx) stereoConfigIdx = (PARAMLIST_STEREOCONFIGIDX)hparam_stereoConfigIdx->paramValue._int;
    }
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_granulelength = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_GRANULELENGTH, &hparam_granulelength);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_granulelength) granulelength = (PARAMLIST_GRANULELENGTH)hparam_granulelength->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_frameSamples = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_FRAMESAMPLES, &hparam_frameSamples);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_frameSamples) frameSamples = (PARAMLIST_FRAMESAMPLES)hparam_frameSamples->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_rapProperty = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_PROPERTY, &hparam_rapProperty);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_rapProperty) rapProperty = (PARAMLIST_RAP_PROPERTY)hparam_rapProperty->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_rapOccurrence = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_OCCURRENCE, &hparam_rapOccurrence);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_rapOccurrence) rapOccurrence = (PARAMLIST_RAP_OCCURRENCE)hparam_rapOccurrence->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_transportFormat = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_TRANSPORTFORMAT, &hparam_transportFormat);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_transportFormat) transportFormat = (PARAMLIST_TRANSPORTFORMAT)hparam_transportFormat->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_streamID = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_STREAMID, &hparam_streamID);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_streamID) streamID = hparam_streamID->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_mpeg4MetadataMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_METADATA_MODE, &hparam_mpeg4MetadataMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4MetadataMode) mpeg4MetadataMode = (PARAMLIST_MPEG4_METADATA_MODE)hparam_mpeg4MetadataMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_rapIntMs = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL, &hparam_rapIntMs);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_rapIntMs) rapIntMs = hparam_rapIntMs->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_rapIntSamples = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES, &hparam_rapIntSamples);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_rapIntSamples) rapIntSamples = hparam_rapIntSamples->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_indepFlagIntervalMs = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST, &hparam_indepFlagIntervalMs);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_indepFlagIntervalMs) indepFlagIntervalMs = hparam_indepFlagIntervalMs->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_indepFlagIntervalSamples = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST_SAMPLES, &hparam_indepFlagIntervalSamples);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_indepFlagIntervalSamples) indepFlagIntervalSamples = hparam_indepFlagIntervalSamples->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_absRapMinIntSamples = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_MIN_INTERVAL_SAMPLES, &hparam_absRapMinIntSamples);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_absRapMinIntSamples) absRapMinIntSamples = hparam_absRapMinIntSamples->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_rapOnDemandAdvancedMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_ON_DEMAND_ADVANCED_MODE, &hparam_rapOnDemandAdvancedMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_rapOnDemandAdvancedMode) rapOnDemandAdvancedMode = hparam_rapOnDemandAdvancedMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_audioPreRollBitResMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AUDIOPREROLLOUTOFBITRES, &hparam_audioPreRollBitResMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_audioPreRollBitResMode) audioPreRollBitResMode = hparam_audioPreRollBitResMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_primingMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_PRIMING_MODE, &hparam_primingMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_primingMode) primingMode = (PARAMLIST_PRIMING)hparam_primingMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_flushingMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_FLUSHINGMODE, &hparam_flushingMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_flushingMode) flushingMode = (PARAMLIST_FLUSHINGMODE)hparam_flushingMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_drcMode = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE, &hparam_drcMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_drcMode) drcMode = (PARAMLIST_DRCMODE)hparam_drcMode->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_mpeg2AAC = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG2AAC, &hparam_mpeg2AAC);

    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg2AAC) mpeg2AAC = (PARAMLIST_MPEG2AAC)hparam_mpeg2AAC->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_mpeg4_drc_light = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_PROF, &hparam_mpeg4_drc_light);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4_drc_light) mpeg4DrcLight = (PARAMLIST_MPEG4_DRC_LIGHT_PROF)hparam_mpeg4_drc_light->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_validate = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_VALIDATE_CONFIGURATION, &hparam_validate);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_validate) validate = (PARAMLIST_VALIDATE_CONFIGURATION)hparam_validate->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_disableLoudness = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_DISABLE_LOUDNESS, &hparam_disableLoudness);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_disableLoudness) {
      disableLoudness = (PARAMLIST_DISABLELOUDNESS)hparam_disableLoudness->paramValue._int;
    }
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_hbe = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_HBE, &hparam_hbe);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_hbe) hbe = (PARAMLIST_HBE)hparam_hbe->paramValue._int;
  }

  if (!isError(retError)) {
    PARAM_INSTANCE_HANDLE hparam_liveLraControl = NULL;
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_LIVE_LRAC, &hparam_liveLraControl);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_liveLraControl) {
      liveLraControl = (PARAMLIST_LIVE_LRAC)hparam_liveLraControl->paramValue._int;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ALLOW_SHORTEST_RAP_INTERVAL)) {
      allowShortestRapInterval = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_PROG_REF_LEVEL)) {
      bMpeg4ProgRefLevelSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_NO_START_STOP_SEQUENCE)) {
      bNoStartStopParamSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE)) {
      targetLraSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_DISABLE_LOUDNESS_MEASUREMENT)) {
      disableLoudnessMeasurement = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD)) {
      quietLoudnessThresholdSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_LEVEL)) {
      loudnessLevelSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS)) {
      loudnessLevelMeasuredSet = 1;
    }
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_SAMPLE_PEAK)) {
      samplePeakSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ALBUM_LOUDNESS_LEVEL)) {
      albumLoudnessLevelSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL)) {
      anchorLoudnessLevelSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS)) {
      anchorLoudnessLevelMeasuredSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_DATA)) {
      lraControlDataSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL)) {
      liveLoudnessLevelSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LIVE_SAMPLE_PEAK)) {
      liveSamplePeakSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LIVE_MODE)) {
      liveModeSet = 1;
    }

    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LIVE_LOUDNESS_REL_MAX_GAIN)) {
      liveRelMaxGainSet = 1;
    }
  }

  inSamplerate_val = iisEncoderConfigGetSamplerateValue(inSamplerate);
  if (inSamplerate_val == -1 && !isError(retError)) {
    retError = ENCODERCONFIG_ERROR_INVALID_INPUT_SR;
  }

  outSamplerate_val = iisEncoderConfigGetSamplerateValue(outSamplerate);
  if (outSamplerate_val == -1 && !isError(retError)) {
    retError = ENCODERCONFIG_ERROR_INVALID_OUTPUT_SR;
  }

  sbrratio_val = iisEncoderConfigGetSbrRatioValue(sbrratio);
  if (sbrratio_val < 0.f && !isError(retError)) {
    retError = ENCODERCONFIG_ERROR_INVALID_SBR_RATIO;
  }

  granulelength_val = iisEncoderConfigGetGranuleLengthValue(granulelength);
  if (granulelength_val == -1 && !isError(retError)) {
    retError = ENCODERCONFIG_ERROR_INVALID_GRANULELENGTH;
  }

  frameSamples_val = iisEncoderConfigGetFrameSamplesValue(frameSamples);
  if (frameSamples_val == -1 && !isError(retError)) {
    retError = ENCODERCONFIG_ERROR_INVALID_FRAMESAMPLES;
  }

  if (!isError(retError)) {
    if (inSamplerate == PARAMLIST_SAMPLERATE_29400 || inSamplerate == PARAMLIST_SAMPLERATE_38400) {
      if ((aot != PARAMLIST_AOT_42) || (inSamplerate != outSamplerate)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_INSAMPLERATE_AOT_OUTSAMPLERATE;
      }
    }
  }

  if (!isError(retError)) {
    if (channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) {
      if (sbrratio == PARAMLIST_SBRRATIO_NONE && stereoConfigIdx != PARAMLIST_STEREOCONFIGIDX_0) {
        stereoConfigIdx = PARAMLIST_STEREOCONFIGIDX_0;
        errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, stereoConfigIdx, PARAMLIST_MODE_REPLACE);
        if (errInfo != noError) {
          retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
        }
      }
    }

    if (!isError(retError)) {
      if (channelconfig == PARAMLIST_CHANNELCONFIG_MONO || channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) {
        const int max_bits = 6144;
        if (bitrate > (unsigned int)(max_bits * outSamplerate_val / granulelength_val / sbrratio_val + 0.5f) && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) {
          stereoConfigIdx = PARAMLIST_STEREOCONFIGIDX_0;
          errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, stereoConfigIdx, PARAMLIST_MODE_REPLACE);
          if (errInfo != noError) {
            retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
          }
        }
        if (!isError(retError)) {
          int num_core_chan = (channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) ? (stereoConfigIdx == PARAMLIST_STEREOCONFIGIDX_0) ? 2 : 1 : 1;
          if (bitrate > (unsigned int)(max_bits * outSamplerate_val * num_core_chan / granulelength_val / sbrratio_val + 0.5f)) {
            retError = ENCODERCONFIG_ERROR_INVALID_BITRATE;
          }
        }
      }
    }

    if (!isError(retError)) {
      if (channelconfig == PARAMLIST_CHANNELCONFIG_MONO && bitrate < 9000) {
        if ((int)(outSamplerate_val / sbrratio_val + 0.5f) < 6000) {
          if (!isError(retError)) {
            retError = ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_CORESAMPLERATE;
          }
        } else if ((int)(outSamplerate_val / sbrratio_val + 0.5f) > 24000) {
          if (!isError(retError)) {
            retError = ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_CORESAMPLERATE;
          }
        }
      }
    }

    if (coremode != PARAMLIST_COREMODE_FD) {
      if (((int)(outSamplerate_val / sbrratio_val + 0.5f) < 6000) ||
          ((int)(outSamplerate_val / sbrratio_val + 0.5f) > 24000) ||
          (sbrratio == PARAMLIST_SBRRATIO_NONE && outSamplerate == PARAMLIST_SAMPLERATE_9600 && bitrate < 23000 && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) ||
          (sbrratio == PARAMLIST_SBRRATIO_NONE && outSamplerate == PARAMLIST_SAMPLERATE_12000 && bitrate < 34000 && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) ||
          (sbrratio == PARAMLIST_SBRRATIO_NONE && outSamplerate == PARAMLIST_SAMPLERATE_16000 && bitrate < 40000 && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) ||
          (sbrratio == PARAMLIST_SBRRATIO_NONE && outSamplerate == PARAMLIST_SAMPLERATE_19200 && bitrate < 40000 && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) ||
          (sbrratio == PARAMLIST_SBRRATIO_NONE && outSamplerate == PARAMLIST_SAMPLERATE_24000 && bitrate < 40000 && channelconfig == PARAMLIST_CHANNELCONFIG_STEREO)) {
        if (!isError(retError)) {
          errInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_COREMODE, PARAMLIST_COREMODE_FD, PARAMLIST_MODE_REPLACE);
          if (errInfo != noError) {
            retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
          }
        }
      }
    }
  }

  if (!isError(retError)) {
    if (indepFlagIntervalMs == 0) {
      retError = ENCODERCONFIG_ERROR_INVALID_USAC_IFI;
    }
  }

  if (!isError(retError)) {
    if (indepFlagIntervalSamples == 0) {
      retError = ENCODERCONFIG_ERROR_INVALID_USAC_IFI;
    }

    else if (indepFlagIntervalSamples > 0) {
      if (indepFlagIntervalSamples % frameSamples_val != 0) {
        retError = ENCODERCONFIG_ERROR_INVALID_USAC_IFI;
      }
    }
  }

  if (!isError(retError)) {
    if (rapIntMs == 0) {
      retError = ENCODERCONFIG_ERROR_INVALID_RAP_INTERVAL;
    }
  }

  if (!isError(retError)) {
    if (rapIntSamples >= 0) {
      if (rapIntSamples == 0 || rapIntSamples % frameSamples_val != 0) {
        retError = ENCODERCONFIG_ERROR_INVALID_RAP_INTERVAL;
      }
    }
  }

  if (!isError(retError)) {
    if (rapOccurrence == PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL) {
      if (absRapMinIntSamples > rapIntSamples) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_INTERVAL_RAP_MIN_INTERVAL;
      }
    }
  }

  if (!isError(retError)) {
    if ((aot != PARAMLIST_AOT_42 || (rapProperty != PARAMLIST_RAP_PROPERTY_SWITCHABLE && rapProperty != PARAMLIST_RAP_PROPERTY_SEEKABLE) || rapOccurrence != PARAMLIST_RAP_OCCURRENCE_ON_DEMAND || audioPreRollBitResMode == 1) && (rapOnDemandAdvancedMode == PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_ON)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_ON_DEMAND_ADVANCED_MODE;
    }
  }

  if (!isError(retError) && validate == PARAMLIST_VALIDATE_CONFIGURATION_OFF) {
    if (albumLoudnessLevelSet == 1) {
      if (liveLoudnessLevelSet == 1) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_ALBUMLOUDNESS;
      } else if (loudnessLevelSet == 0 && loudnessLevelMeasuredSet == 0 && anchorLoudnessLevelSet == 0 && anchorLoudnessLevelMeasuredSet == 0) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_ALBUMLOUDNESS;
      }
    }

    if (!isError(retError) && disableLoudness != PARAMLIST_DISABLELOUDNESS_ON) {
      if (loudnessLevelSet != 1 && loudnessLevelMeasuredSet != 1 && anchorLoudnessLevelSet != 1 && bMpeg4ProgRefLevelSet != 1 && anchorLoudnessLevelMeasuredSet != 1 && liveLoudnessLevelSet != 1) {
        retError = ENCODERCONFIG_ERROR_NO_LOUDNESS_PROVIDED;
      }

      if (!isError(retError)) {
        if (loudnessLevelSet == 1 && anchorLoudnessLevelSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_ANCHORLOUDNESS;
        }
      }

      if (!isError(retError)) {
        if (liveLoudnessLevelSet == 0 && liveModeSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_NO_LIVELOUDNESSLEVEL_LIVEMODE;
        }
      }

      if (!isError(retError)) {
        if (liveLoudnessLevelSet == 0 && liveRelMaxGainSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_NO_LIVELOUDNESSLEVEL_LIVERELMAXGAIN;
        }
      }

      if (!isError(retError)) {
        if (liveLoudnessLevelSet == 1 && quietLoudnessThresholdSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        }
      }

      if (!isError(retError)) {
        if ((loudnessLevelMeasuredSet == 1 || anchorLoudnessLevelMeasuredSet == 1) && loudnessLevelSet == 0 && bMpeg4ProgRefLevelSet == 0 && anchorLoudnessLevelSet == 0 && liveLoudnessLevelSet == 0 && quietLoudnessThresholdSet == 0) {
          retError = ENCODERCONFIG_ERROR_MISSING_QUIET_LOUDNESS_THRESHOLD;
        } else if (loudnessLevelSet == 1 && quietLoudnessThresholdSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        } else if (bMpeg4ProgRefLevelSet == 1 && quietLoudnessThresholdSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_QUIETLOUDNESSTHRESHOLD;
        } else if (anchorLoudnessLevelSet == 1 && quietLoudnessThresholdSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_ANCHORLOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        } else if ((loudnessLevelMeasuredSet == 0 && anchorLoudnessLevelMeasuredSet == 0) && quietLoudnessThresholdSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_QUIET_LOUDNESS_THRESHOLD;
        }
      }
    }

    if (!isError(retError) && (liveLoudnessLevelSet == 1)) {
      PARAM_INSTANCE_HANDLE hParamChannelConfig = NULL;
      errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hParamChannelConfig);
      if (errInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hParamChannelConfig != NULL && hParamChannelConfig->paramFormat == PARAM_INT) {
        PARAMLIST_CHANNELCONFIG cfgChannelConfig = (PARAMLIST_CHANNELCONFIG)hParamChannelConfig->paramValue._int;
        if (cfgChannelConfig != PARAMLIST_CHANNELCONFIG_MONO && cfgChannelConfig != PARAMLIST_CHANNELCONFIG_STEREO) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_CHANNELCONFIG_LIVELOUDNESS;
        }
      } else {
        retError = ENCODERCONFIG_ERROR_INVALID_CHANNELCONFIG;
      }
    }

    if (!isError(retError)) {
      if ((liveLoudnessLevelSet == 1) && (aot != PARAMLIST_AOT_42)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LIVELOUDNESSLEVEL;
      }
    }

    if (!isError(retError)) {
      if (liveLoudnessLevelSet == 1 && disableLoudness != PARAMLIST_DISABLELOUDNESS_OFF) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_DISABLELOUDNESS;
      }
    }

    if (!isError(retError)) {
      if ((liveLoudnessLevelSet == 1) && (anchorLoudnessLevelSet == 1)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_ANCHORLOUDNESS;
      }
    }

    if (!isError(retError)) {
      if ((loudnessLevelSet == 1 || bMpeg4ProgRefLevelSet == 1) && liveLoudnessLevelSet == 1) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_LIVELOUDNESSLEVEL;
      }
    }

    if (!isError(retError) && (liveSamplePeakSet == 1)) {
      if (liveLoudnessLevelSet == 0) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_LIVESAMPLEPEAK_NOLIVELOUDNESSLEVEL;
      }
    }

    if (!isError(retError)) {
      if (samplePeakSet == 1 && liveLoudnessLevelSet == 1) {
        if (liveSamplePeakSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_SAMPLEPEAK_LIVESAMPLEPEAK;
        }

        else {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_SAMPLEPEAK_LIVELOUDNESSLEVEL;
        }
      }
    }

    if (!isError(retError) && (drcMode != PARAMLIST_DRCMODE_OFF)) {
      if (disableLoudness != PARAMLIST_DISABLELOUDNESS_OFF && liveLoudnessLevelSet != 1) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_DRC_LOUDNESS;
      }
    }

    if (!isError(retError)) {
      if (disableLoudness == PARAMLIST_DISABLELOUDNESS_ON) {
        if (loudnessLevelSet == 1 || anchorLoudnessLevelSet == 1 || bMpeg4ProgRefLevelSet == 1) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_DISABLELOUDNESS;
        }
      }
    }

    if (
        (drcMode != PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL) && (liveLraControl == PARAMLIST_LIVE_LRAC_ON)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_LIVE_LRA_CONTROL;
    }

    if (!isError(retError) && liveLraControl == PARAMLIST_LIVE_LRAC_OFF) {
      if (
          drcMode == PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL) {
        if (!lraControlDataSet) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_LRACONTROL_DATA;
        }
      }
    }

    if (!isError(retError)) {
      if (targetLraSet) {
        if (
            (drcMode != PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL)) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_TARGET_LRA;
        }
      }
    }
  }

  if (!isError(retError)) {
    if ((aot != PARAMLIST_AOT_42) && (drcMode != PARAMLIST_DRCMODE_OFF)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MPEG_D_DRC;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && drcMode != PARAMLIST_DRCMODE_OFF) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MPEG4DRC;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && bitrateMode != PARAMLIST_BITRATEMODE_CBR) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_VBR_MPEG4DRC;
    }
  }

  if (!isError(retError)) {
    if (aot == PARAMLIST_AOT_42 && bMpeg4ProgRefLevelSet == 1) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_AOT;
    } else if (aot != PARAMLIST_AOT_42 && (loudnessLevelSet == 1 || anchorLoudnessLevelSet == 1)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LOUDNESSLEVEL;
    } else if (mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && bMpeg4ProgRefLevelSet == 1 && mpeg4MetadataMode == PARAMLIST_MPEG4_METADATA_MODE_NONE) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_MPEGDDRC;
    } else if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && disableLoudnessMeasurement == 1) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MPEG4DRC_DISABLELOUDNESSMEASUREMENT;
    }
  }

  if (validate == PARAMLIST_VALIDATE_CONFIGURATION_OFF) {
    if (!isError(retError)) {
      if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && bMpeg4ProgRefLevelSet == 0) {
        retError = ENCODERCONFIG_ERROR_REQUIRED_MPEG4PROGREFLEVEL_FOR_MPEG4DRC;
      }
    }

    if (!isError(retError)) {
      if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && disableLoudness == PARAMLIST_DISABLELOUDNESS_ON) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_MPEG4DRC_LOUDNESS;
      }
    }

    if (!isError(retError)) {
      if (aot != PARAMLIST_AOT_42 && (mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT && drcMode == PARAMLIST_DRCMODE_OFF && mpeg4MetadataMode == PARAMLIST_MPEG4_METADATA_MODE_NONE) && disableLoudness == PARAMLIST_DISABLELOUDNESS_OFF) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LOUDNESS_DRC;
      }
    }

    if (!isError(retError)) {
      if (mpeg4MetadataMode != PARAMLIST_MPEG4_METADATA_MODE_NONE && !bMpeg4ProgRefLevelSet) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_MDMODE_PROGREFLVL;
      }
    }
  }

  if (!isError(retError)) {
    if (mpeg2AAC == PARAMLIST_MPEG2AAC_ON && aot == PARAMLIST_AOT_42) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MPEG2AAC_AOT;
    }
  }

  if (!isError(retError)) {
    if (bNoStartStopParamSet == 1 && aot == PARAMLIST_AOT_42) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_NOSTARTSTOP_AOT;
    }
  }

  if (!isError(retError)) {
    if (aot == PARAMLIST_AOT_42 && configset == PARAMLIST_CONFIGSET_DASH && streamID < 0) {
      retError = ENCODERCONFIG_ERROR_REQUIRED_STREAMID_FOR_SWITCHABLE_AND_AOT42;
    }
  }
  if (!isError(retError)) {
    if (aot != PARAMLIST_AOT_42 && streamID >= 0) {
      retError = ENCODERCONFIG_ERROR_FORBIDDEN_STREAMID_FOR_NOT_AOT42;
    }
  }

  if (!isError(retError)) {
    if (drcMode != PARAMLIST_DRCMODE_OFF && flushingMode != PARAMLIST_FLUSHINGMODE_DEFAULT) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_FLUSHINGMODE_DRC;
    }
  }

  if (!isError(retError)) {
    if ((rapProperty == PARAMLIST_RAP_PROPERTY_OFF && rapOccurrence != PARAMLIST_RAP_OCCURRENCE_OFF) ||
        (rapProperty != PARAMLIST_RAP_PROPERTY_OFF && rapOccurrence == PARAMLIST_RAP_OCCURRENCE_OFF)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_RAP_OCCURRENCE;
    }
  }

  if (!isError(retError)) {
    if (rapProperty == PARAMLIST_RAP_PROPERTY_OFF &&
        (transportFormat == PARAMLIST_TRANSPORTFORMAT_LATM ||
         transportFormat == PARAMLIST_TRANSPORTFORMAT_LATMLOAS)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_TRANSPORTFORMAT;
    }
  }

  if (!isError(retError)) {
    if (aot == PARAMLIST_AOT_42) {
      if (primingMode != PARAMLIST_PRIMING_NONE &&
          (rapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE || rapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE || rapProperty == PARAMLIST_RAP_PROPERTY_ACCESS)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT42_RAP_PROPERTY_PRIMING;
      }
    }

    else if (aot != PARAMLIST_AOT_42) {
      if (rapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE &&
          (primingMode == PARAMLIST_PRIMING_FULL)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_PRIMING;
      }
    }
  }

  if (!isError(retError)) {
    if (rapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE && configset != PARAMLIST_CONFIGSET_DASH) {
      retError = ENCODERCONFIG_ERROR_FORBIDDEN_RAP_PROPERTY_CONFIGSET;
    }
  }

  if (!isError(retError)) {
    if (rapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE && configset != PARAMLIST_CONFIGSET_SEEKABLE) {
      retError = ENCODERCONFIG_ERROR_FORBIDDEN_RAP_PROPERTY_CONFIGSET;
    }
  }

  if (!isError(retError)) {
    if ((rapOccurrence == PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL && rapIntMs < 0) ||
        (rapOccurrence != PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL && rapIntMs >= 0)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_OCCURRENCE_RAP_INTERVAL;
    }
  }

  if (!isError(retError)) {
    if ((rapOccurrence == PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL && rapIntSamples < 0) ||
        (rapOccurrence != PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL && rapIntSamples >= 0)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_RAP_OCCURRENCE_RAP_INTERVAL;
    }
  }

  if (!isError(retError)) {
    if (hbe == PARAMLIST_HBE_ON && stereoConfigIdx == PARAMLIST_STEREOCONFIGIDX_3) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_HBE_STEREOCONFIGIDX;
    }
  }

  if (!isError(retError)) {
    for (i = 0; i < defaultModeTableSize; i++) {
      if ((defaultMode[i].channelConfig == channelconfig) &&
          (defaultMode[i].aot == aot) &&
          (defaultMode[i].bitratemode == bitrateMode)) {
        if (bitrate != (PARAMLIST_BITRATE)defaultMode[i].bitrate) {
          retError = ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_BITRATEMODE;
        }
      }
    }
  }

  if (!isError(retError)) {
    if (aot != PARAMLIST_AOT_42) {
      for (i = 0; i < defaultModeTableSize; i++) {
        if ((defaultMode[i].channelConfig == channelconfig) &&
            (defaultMode[i].bitrate == bitrate) &&
            (defaultMode[i].bitratemode == bitrateMode)) {
          if (aot != (PARAMLIST_AOT)defaultMode[i].aot) {
            retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_BITRATEMODE_CHANNELCONFIG;
          } else {
            break;
          }
        }
      }
    }
  }

  if (!isError(retError)) {
    retError = iisEncoderConfigSanityCheckMpeg4Metadata(hCodecParamList);
  }

  if (!isError(retError)) {
    if (aot == PARAMLIST_AOT_42) {
      if (allowShortestRapInterval && audioPreRollBitResMode == 0) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_SHORTEST_RAP_INTVL_APR_BITRESMODE;
      }
    }
  }

  if (!isError(retError)) {
    if (allowShortestRapInterval && rapOccurrence != PARAMLIST_RAP_OCCURRENCE_ON_DEMAND) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_SHORTEST_RAP_INTVL_RAP_OCCURRENCE;
    }
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSanityCheckMpeg4Metadata(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errInfo = noError;

  PARAM_INSTANCE_HANDLE hparam_aot = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4_drc_light = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4_drc_heavy = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4MetadataMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4WritePceMixDnLvl = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4EtsiDmxPresent = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DolbySurroundMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DrcPresMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DolbyPseudoSurDmxEna = NULL;
  PARAM_INSTANCE_HANDLE hparam_channelconfig = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DRCLightTargetLevel = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DRCHeavyTargetLevel = NULL;

  PARAMLIST_AOT aot = PARAMLIST_AOT_INVALID;
  PARAMLIST_CHANNELCONFIG channelconfig = PARAMLIST_CHANNELCONFIG_INVALID;
  PARAMLIST_MPEG4_DRC_LIGHT_PROF mpeg4DrcLight = PARAMLIST_MPEG4_DRC_LIGHT_PROF_INVALID;
  PARAMLIST_MPEG4_DRC_HEAVY_PROF mpeg4DrcHeavy = PARAMLIST_MPEG4_DRC_HEAVY_PROF_INVALID;
  PARAMLIST_MPEG4_METADATA_MODE mpeg4MetadataMode = PARAMLIST_MPEG4_METADATA_MODE_INVALID;
  PARAMLIST_MPEG4_WRITE_PCE_MIXDN mpeg4WritePceMixDnLvl = PARAMLIST_MPEG4_WRITE_PCE_MIXDN_INVALID;
  PARAMLIST_MPEG4_ETSIDMXPRESENT mpeg4EtsiDmxPresent = PARAMLIST_MPEG4_ETSIDMXPRESENT_INVALID;
  PARAMLIST_MPEG4_DSUR_IND mpeg4DolbySurroundMode = PARAMLIST_MPEG4_DSUR_INVALID;
  PARAMLIST_MPEG4_DRCPRESENTATION mpeg4DrcPresMode = PARAMLIST_MPEG4_DRCPRESENTATION_INVALID;
  PARAMLIST_MPEG4_PSEUDO_SUR_ENABLE mpeg4DolbyPseudoSurDmxEna = PARAMLIST_MPEG4_PSEUDO_SUR_INVALID;

  float mpeg4DRCLightTargetLevel = 0.0f;
  float mpeg4DRCHeavyTargetLevel = 0.0f;

  int bMpeg4MetadataModeSet = 0;
  int bMpeg4CenterMixLvlSet = 0;
  int bMpeg4SurroundMixLvlSet = 0;

  int bMpeg4DRCLightTargetLevelSet = 0;
  int bMpeg4DRCHeavyTargetLevelSet = 0;

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AOT, &hparam_aot);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_aot) aot = (PARAMLIST_AOT)hparam_aot->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hparam_channelconfig);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_channelconfig) channelconfig = (PARAMLIST_CHANNELCONFIG)hparam_channelconfig->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_PROF, &hparam_mpeg4_drc_light);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4_drc_light) mpeg4DrcLight = (PARAMLIST_MPEG4_DRC_LIGHT_PROF)hparam_mpeg4_drc_light->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_PROF, &hparam_mpeg4_drc_heavy);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4_drc_heavy) mpeg4DrcHeavy = (PARAMLIST_MPEG4_DRC_HEAVY_PROF)hparam_mpeg4_drc_heavy->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_METADATA_MODE, &hparam_mpeg4MetadataMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4MetadataMode) mpeg4MetadataMode = (PARAMLIST_MPEG4_METADATA_MODE)hparam_mpeg4MetadataMode->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_WRITE_PCE_MIXDOWN_IDX, &hparam_mpeg4WritePceMixDnLvl);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4WritePceMixDnLvl) mpeg4WritePceMixDnLvl = (PARAMLIST_MPEG4_WRITE_PCE_MIXDN)hparam_mpeg4WritePceMixDnLvl->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_ETSI_DWNMIX_PRESENT, &hparam_mpeg4EtsiDmxPresent);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4EtsiDmxPresent) mpeg4EtsiDmxPresent = (PARAMLIST_MPEG4_ETSIDMXPRESENT)hparam_mpeg4EtsiDmxPresent->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DOLBY_SURROUND_MODE, &hparam_mpeg4DolbySurroundMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DolbySurroundMode) mpeg4DolbySurroundMode = (PARAMLIST_MPEG4_DSUR_IND)hparam_mpeg4DolbySurroundMode->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_PRES_MODE, &hparam_mpeg4DrcPresMode);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DrcPresMode) mpeg4DrcPresMode = (PARAMLIST_MPEG4_DRCPRESENTATION)hparam_mpeg4DrcPresMode->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_PSEUDO_SUR_DMX_ENABLE, &hparam_mpeg4DolbyPseudoSurDmxEna);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DolbyPseudoSurDmxEna) mpeg4DolbyPseudoSurDmxEna = (PARAMLIST_MPEG4_PSEUDO_SUR_ENABLE)hparam_mpeg4DolbyPseudoSurDmxEna->paramValue._int;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_TARGET_LEVEL, &hparam_mpeg4DRCLightTargetLevel);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DRCLightTargetLevel) mpeg4DRCLightTargetLevel = hparam_mpeg4DRCLightTargetLevel->paramValue._float;
  }

  if (!isError(retError)) {
    errInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_TARGET_LEVEL, &hparam_mpeg4DRCHeavyTargetLevel);
    if (errInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DRCHeavyTargetLevel) mpeg4DRCHeavyTargetLevel = hparam_mpeg4DRCHeavyTargetLevel->paramValue._float;
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_METADATA_MODE)) {
      bMpeg4MetadataModeSet = 1;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_CENTER_MIX_LEVEL)) {
      bMpeg4CenterMixLvlSet = 1;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_SURROUND_MIX_LEVEL)) {
      bMpeg4SurroundMixLvlSet = 1;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_TARGET_LEVEL)) {
      bMpeg4DRCLightTargetLevelSet = 1;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_TARGET_LEVEL)) {
      bMpeg4DRCHeavyTargetLevelSet = 1;
    }
  }

  if (!isError(retError)) {
    if ((mpeg4MetadataMode != PARAMLIST_MPEG4_METADATA_MODE_NONE ||
         bMpeg4CenterMixLvlSet ||
         bMpeg4SurroundMixLvlSet ||
         mpeg4WritePceMixDnLvl == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON ||
         mpeg4EtsiDmxPresent == PARAMLIST_MPEG4_ETSIDMXPRESENT_TRUE ||
         mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_NOT_USED || mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_IS_USED ||
         mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_1 || mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2 ||
         mpeg4DolbyPseudoSurDmxEna == PARAMLIST_MPEG4_PSEUDO_SUR_ON) &&
        aot == PARAMLIST_AOT_42) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MD;
    } else if ((mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT ||
                mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT) &&
               aot == PARAMLIST_AOT_42) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MPEG4DRC;
    }
  }

  if (!isError(retError) && bMpeg4MetadataModeSet) {
    if ((channelconfig == PARAMLIST_CHANNELCONFIG_MONO || channelconfig == PARAMLIST_CHANNELCONFIG_STEREO) &&
        (mpeg4EtsiDmxPresent == PARAMLIST_MPEG4_ETSIDMXPRESENT_TRUE ||
         mpeg4WritePceMixDnLvl == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON ||
         bMpeg4CenterMixLvlSet ||
         bMpeg4SurroundMixLvlSet)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_CHANNELCONFIG_DOWNMIX;
    } else if (mpeg4MetadataMode != PARAMLIST_MPEG4_METADATA_MODE_NONE && mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      retError = ENCODERCONFIG_ERROR_REQUIRED_MPEG4DRC_FOR_MPEG4METADATA;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT && mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      retError = ENCODERCONFIG_ERROR_FORBIDDEN_MPEG4DRCHEAVY_NOT_MPEG4DRCLIGHT;
    }
  }

  if (!isError(retError) && mpeg4MetadataMode == PARAMLIST_MPEG4_METADATA_MODE_NONE) {
    if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT ||
        mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT ||
        bMpeg4DRCHeavyTargetLevelSet ||
        bMpeg4DRCLightTargetLevelSet) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_NONE_DRC;
    } else if (mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_1 || mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_DRC_PRES_MODE;
    } else if ((channelconfig != PARAMLIST_CHANNELCONFIG_MONO && channelconfig != PARAMLIST_CHANNELCONFIG_STEREO) &&
               (mpeg4EtsiDmxPresent == PARAMLIST_MPEG4_ETSIDMXPRESENT_TRUE ||
                bMpeg4CenterMixLvlSet ||
                mpeg4WritePceMixDnLvl == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON ||
                bMpeg4SurroundMixLvlSet)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_CHANNELCONFIG_DOWNMIX;
    } else if (mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_NOT_USED || mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_IS_USED ||
               mpeg4DolbyPseudoSurDmxEna == PARAMLIST_MPEG4_PSEUDO_SUR_ON) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_SURROUND;
    }
  }

  if (!isError(retError) && mpeg4MetadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG) {
    if (mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT || bMpeg4DRCHeavyTargetLevelSet) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MPEG_DRC_HEAVY;
    } else if (mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_1 || mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_DRC_PRES_MODE;
    } else if ((mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_NOT_USED || mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_IS_USED) ||
               (mpeg4DolbyPseudoSurDmxEna == PARAMLIST_MPEG4_PSEUDO_SUR_ON && mpeg4WritePceMixDnLvl == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_OFF)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_SURROUND;
    }
  }

  if (!isError(retError) && mpeg4MetadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
    if ((channelconfig != PARAMLIST_CHANNELCONFIG_STEREO) &&
        (mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_NOT_USED || mpeg4DolbySurroundMode == PARAMLIST_MPEG4_DSUR_IS_USED)) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_CHANNELCONFIG_SURROUND;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_1 &&
        ((bMpeg4DRCLightTargetLevelSet && mpeg4DRCLightTargetLevel < -31) ||
         (bMpeg4DRCHeavyTargetLevelSet && mpeg4DRCHeavyTargetLevel < -20))) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_DRC_PRES_TARGETREFLVL;
    } else if (mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2 &&
               ((bMpeg4DRCLightTargetLevelSet && mpeg4DRCLightTargetLevel < -23) ||
                (bMpeg4DRCHeavyTargetLevelSet && mpeg4DRCHeavyTargetLevel < -23))) {
      retError = ENCODERCONFIG_ERROR_INVALID_COMB_MD_DRC_PRES_TARGETREFLVL;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT) {
      if (bMpeg4DRCHeavyTargetLevelSet == 0) {
        retError = ENCODERCONFIG_ERROR_REQUIRED_TARGETLOUDNESS_FOR_MPEG4DRCHEAVY;
      }
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      if (bMpeg4DRCLightTargetLevelSet == 0) {
        retError = ENCODERCONFIG_ERROR_REQUIRED_TARGETLOUDNESS_FOR_MPEG4DRCLIGHT;
      }
    }
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetMandatoryParameters(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO retInfo = noError;

  PARAMLIST_BITRATEMODE cfgBitRateMode = PARAMLIST_BITRATEMODE_INVALID;
  PARAMLIST_CHANNELCONFIG cfgChannelConfig = PARAMLIST_CHANNELCONFIG_INVALID;
  int cfgBitRate = -1;
  int userBitrateSet = 0;
  int cfgBitRateRestricted = 0;
  PARAMLIST_AOT cfgAot = PARAMLIST_AOT_INVALID;

  PARAM_INSTANCE_HANDLE hParamBitRateMode = NULL;
  PARAM_INSTANCE_HANDLE hParamBitRate = NULL;
  PARAM_INSTANCE_HANDLE hParamAot = NULL;
  PARAM_INSTANCE_HANDLE hParamChannelConfig = NULL;
  int continuousBitRates = 0;

  int i = 0;
  int const defaultCbrTableSize = sizeof(defaultCbr) / sizeof(ENCCONFIG_DEFAULTCBR);
  int const defaultModeTableSize = sizeof(defaultMode) / sizeof(ENCCONFIG_DEFAULTMODE);

  if (!isError(retError)) {
    if (hCodecParamList == NULL) {
      retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hParamChannelConfig);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamChannelConfig != NULL && hParamChannelConfig->paramFormat == PARAM_INT) {
      cfgChannelConfig = (PARAMLIST_CHANNELCONFIG)hParamChannelConfig->paramValue._int;
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_CHANNELCONFIG;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AOT, &hParamAot);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamAot != NULL) {
      if (hParamAot->paramFormat == PARAM_INT) {
        cfgAot = (PARAMLIST_AOT)hParamAot->paramValue._int;
      } else {
        retError = ENCODERCONFIG_ERROR_INVALID_AOT;
      }
    }
  }

  if (!isError(retError)) {
    if (hParamAot == NULL) {
      cfgAot = PARAMLIST_AOT_42;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_CONFIGSET, PARAMLIST_CONFIGSET_DASH, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TRANSPORTFORMAT, PARAMLIST_TRANSPORTFORMAT_RAW, PARAMLIST_MODE_APPENDIFMISSING);

    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_BITRATEMODE, PARAMLIST_BITRATEMODE_CBR, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }
  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_BITRATEMODE, &hParamBitRateMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamBitRateMode != NULL && hParamBitRateMode->paramFormat == PARAM_INT) {
      cfgBitRateMode = (PARAMLIST_BITRATEMODE)hParamBitRateMode->paramValue._int;
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_BITRATEMODE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TRANSPORTFORMAT, PARAMLIST_TRANSPORTFORMAT_RAW, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_BITRATE, &hParamBitRate);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamBitRate != NULL) {
      if (hParamBitRate->paramFormat == PARAM_INT) {
        cfgBitRate = hParamBitRate->paramValue._int;

        if (continuousBitRates == 0) {
          cfgBitRateRestricted = (cfgBitRate % ENCCONFIG_BITRATE_RESTRICTION_FACTOR);
        }

        if (cfgBitRateRestricted == 0) {
          userBitrateSet = 1;
        } else {
          retError = ENCODERCONFIG_ERROR_INVALID_BITRATE;
        }
      } else {
        retError = ENCODERCONFIG_ERROR_INVALID_BITRATE;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgBitRateMode == PARAMLIST_BITRATEMODE_CBR) {
      if (hParamBitRate == NULL) {
        for (i = 0; i < defaultCbrTableSize; i++) {
          if ((defaultCbr[i].channelConfig == cfgChannelConfig) &&
              (defaultCbr[i].aot == cfgAot)) {
            cfgBitRate = defaultCbr[i].defaultBitrate;
            break;
          }
        }
      }
    } else {
      for (i = 0; i < defaultModeTableSize; i++) {
        if ((defaultMode[i].channelConfig == cfgChannelConfig) &&
            (defaultMode[i].aot == cfgAot || cfgAot == PARAMLIST_AOT_INVALID) &&
            (defaultMode[i].bitratemode == cfgBitRateMode)) {
          if (hParamBitRate == NULL) {
            cfgBitRate = defaultMode[i].bitrate;
          }
          break;
        }
      }
      if (cfgBitRate == -1) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_BITRATEMODE_CHANNELCONFIG;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgAot != PARAMLIST_AOT_INVALID) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_AOT, cfgAot, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if ((cfgBitRateMode != PARAMLIST_BITRATEMODE_CBR) && userBitrateSet == 1) {
    retError = ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_BITRATEMODE;
  }

  if (!isError(retError)) {
    if (cfgBitRate >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_BITRATE, cfgBitRate, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetDefaultParameters(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO retInfo = noError;

  PARAM_INSTANCE_HANDLE hParamConfigSet = NULL;
  PARAM_INSTANCE_HANDLE hParamTransport = NULL;
  PARAM_INSTANCE_HANDLE hparam_frameSamples = NULL;
  PARAM_INSTANCE_HANDLE hParamAot = NULL;
  PARAM_INSTANCE_HANDLE hParamIndepFlagIntervalMs = NULL;
  PARAM_INSTANCE_HANDLE hParamIndepFlagIntervalSamples = NULL;
  PARAM_INSTANCE_HANDLE hParamRapIntSamples = NULL;
  PARAM_INSTANCE_HANDLE hParamRapIntMs = NULL;
  PARAM_INSTANCE_HANDLE hParamRapProperty = NULL;
  PARAM_INSTANCE_HANDLE hParamRapOccurrence = NULL;
  PARAM_INSTANCE_HANDLE hParamPrimingMode = NULL;
  PARAM_INSTANCE_HANDLE hParamDRCMode = NULL;
  PARAM_INSTANCE_HANDLE hParamTargetLra = NULL;
  PARAM_INSTANCE_HANDLE hParam_outSampleRate = NULL;
  PARAM_INSTANCE_HANDLE hparam_bitrate = NULL;
  PARAM_INSTANCE_HANDLE hParamChannelConfig = NULL;
  PARAM_INSTANCE_HANDLE hparam_audioPreRollBitResMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_allowShortestRapInterval = NULL;
  PARAM_INSTANCE_HANDLE hParam_quietLoudnessThreshold = NULL;

  PARAMLIST_CONFIGSET cfgConfigSet = PARAMLIST_CONFIGSET_INVALID;
  PARAMLIST_TRANSPORTFORMAT cfgTransportFormat = PARAMLIST_TRANSPORTFORMAT_INVALID;
  PARAMLIST_AOT cfgAot = PARAMLIST_AOT_INVALID;
  PARAMLIST_DRCMODE drcMode = PARAMLIST_DRCMODE_INVALID;
  PARAMLIST_DRCCHARACTERISTIC drcCharacteristic[ENCCONFIG_MAX_DRC_SEQUENCES] = {PARAMLIST_DRCCHARACTERISTIC_INVALID, PARAMLIST_DRCCHARACTERISTIC_INVALID, PARAMLIST_DRCCHARACTERISTIC_INVALID, PARAMLIST_DRCCHARACTERISTIC_INVALID};
  PARAMLIST_CHANNELCONFIG cfgChannelConfig = PARAMLIST_CHANNELCONFIG_INVALID;
  PARAMLIST_BITRATE bitrate = 0;

  int allowShortestRapInterval = -1;

  PARAMLIST_FRAMESAMPLES frameSamples = PARAMLIST_FRAMESAMPLES_INVALID;
  int frameSamples_val = -1;
  int cfgIndepFlagIntervalMs = -1;
  int cfgIndepFlagIntervalSamples = -1;
  int cfgRapIntSamples = -1;
  int cfgRapIntMs = -1;
  int cfgAbsRapMinIntSamples = -1;
  int cfgAbsRapMinIntMs = -1;
  int audioPreRollBitResMode = -1;
  int outSampleRate = -1;
  int outSamplerate_val = -1;
  int cfgRapOnDemandAdvancedMode = -1;
  int loudnessLevelSet = -1;
  int bMpeg4ProgRefLevelSet = -1;
  int loudnessLevelMeasuredSet = -1;
  int anchorLoudnessLevelSet = -1;
  int anchorLoudnessLevelMeasuredSet = -1;
  int liveLoudnessLevelSet = -1;
  float targetLra = -1.0f;
  int lraControlDataSet = 0;
  float quietLoudnessThreshold = -1.0f;

  PARAMLIST_PRIMING primingMode = PARAMLIST_PRIMING_INVALID;
  PARAMLIST_DISABLELOUDNESS disableLoudness = PARAMLIST_DISABLELOUDNESS_INVALID;
  PARAMLIST_RAP_PROPERTY cfgRapProperty = PARAMLIST_RAP_PROPERTY_INVALID;
  PARAMLIST_RAP_OCCURRENCE cfgRapOccurrence = PARAMLIST_RAP_OCCURRENCE_INVALID;
  PARAMLIST_LIVE_LRAC liveLraControl = PARAMLIST_LIVE_LRAC_OFF;

  if (!isError(retError)) {
    if (hCodecParamList == NULL) {
      retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CONFIGSET, &hParamConfigSet);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamConfigSet != NULL && hParamConfigSet->paramFormat == PARAM_INT) {
      cfgConfigSet = (PARAMLIST_CONFIGSET)hParamConfigSet->paramValue._int;
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_CONFIGSET;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_TRANSPORTFORMAT, &hParamTransport);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamTransport != NULL && hParamTransport->paramFormat == PARAM_INT) {
      cfgTransportFormat = (PARAMLIST_TRANSPORTFORMAT)hParamTransport->paramValue._int;
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_TRANSPORTFORMAT;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_FRAMESAMPLES, &hparam_frameSamples);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_frameSamples != NULL && hparam_frameSamples->paramFormat == PARAM_INT) {
      frameSamples = (PARAMLIST_FRAMESAMPLES)hparam_frameSamples->paramValue._int;
      frameSamples_val = iisEncoderConfigGetFrameSamplesValue(frameSamples);
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_FRAMESAMPLES;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AOT, &hParamAot);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamAot != NULL && hParamAot->paramFormat == PARAM_INT) {
      cfgAot = (PARAMLIST_AOT)hParamAot->paramValue._int;
    } else {
      retError = ENCODERCONFIG_ERROR_INVALID_AOT;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_LEVEL)) {
      loudnessLevelSet = 1;
    } else {
      loudnessLevelSet = 0;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_PROG_REF_LEVEL)) {
      bMpeg4ProgRefLevelSet = 1;
    } else {
      bMpeg4ProgRefLevelSet = 0;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS)) {
      loudnessLevelMeasuredSet = 1;
    } else {
      loudnessLevelMeasuredSet = 0;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ANCHOR_LOUDNESS_LEVEL)) {
      anchorLoudnessLevelSet = 1;
    } else {
      anchorLoudnessLevelSet = 0;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS)) {
      anchorLoudnessLevelMeasuredSet = 1;
    } else {
      anchorLoudnessLevelMeasuredSet = 0;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL)) {
      liveLoudnessLevelSet = 1;
    } else {
      liveLoudnessLevelSet = 0;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_PROPERTY, &hParamRapProperty);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamRapProperty != NULL && hParamRapProperty->paramFormat == PARAM_INT) {
      cfgRapProperty = (PARAMLIST_RAP_PROPERTY)hParamRapProperty->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_OCCURRENCE, &hParamRapOccurrence);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamRapOccurrence != NULL && hParamRapOccurrence->paramFormat == PARAM_INT) {
      cfgRapOccurrence = (PARAMLIST_RAP_OCCURRENCE)hParamRapOccurrence->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL, &hParamRapIntMs);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamRapIntMs != NULL && hParamRapIntMs->paramFormat == PARAM_INT) {
      cfgRapIntMs = hParamRapIntMs->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES, &hParamRapIntSamples);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamRapIntSamples != NULL && hParamRapIntSamples->paramFormat == PARAM_INT) {
      cfgRapIntSamples = hParamRapIntSamples->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE, &hParamDRCMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamDRCMode != NULL && hParamDRCMode->paramFormat == PARAM_INT) {
      drcMode = (PARAMLIST_DRCMODE)hParamDRCMode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_LOUDNESS_DATA)) {
      lraControlDataSet = 1;
    } else {
      lraControlDataSet = 0;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE, &hParamTargetLra);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamTargetLra != NULL && hParamTargetLra->paramFormat == PARAM_FLOAT) {
      targetLra = hParamTargetLra->paramValue._float;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST, &hParamIndepFlagIntervalMs);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamIndepFlagIntervalMs != NULL && hParamIndepFlagIntervalMs->paramFormat == PARAM_INT) {
      cfgIndepFlagIntervalMs = hParamIndepFlagIntervalMs->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST_SAMPLES, &hParamIndepFlagIntervalSamples);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamIndepFlagIntervalSamples != NULL && hParamIndepFlagIntervalSamples->paramFormat == PARAM_INT) {
      cfgIndepFlagIntervalSamples = hParamIndepFlagIntervalSamples->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_PRIMING_MODE, &hParamPrimingMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamPrimingMode != NULL && hParamPrimingMode->paramFormat == PARAM_INT) {
      primingMode = (PARAMLIST_PRIMING)hParamPrimingMode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_AUDIOPREROLLOUTOFBITRES, &hparam_audioPreRollBitResMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_audioPreRollBitResMode != NULL && hparam_audioPreRollBitResMode->paramFormat == PARAM_INT) {
      audioPreRollBitResMode = hparam_audioPreRollBitResMode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_BITRATE, &hparam_bitrate);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_bitrate) bitrate = (PARAMLIST_BITRATE)hparam_bitrate->paramValue._int;
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hParamChannelConfig);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParamChannelConfig != NULL && hParamChannelConfig->paramFormat == PARAM_INT) {
      cfgChannelConfig = (PARAMLIST_CHANNELCONFIG)hParamChannelConfig->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_OUTSAMPLERATE, &hParam_outSampleRate);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hParam_outSampleRate != NULL && hParam_outSampleRate->paramFormat == PARAM_INT) {
      outSampleRate = hParam_outSampleRate->paramValue._int;
      outSamplerate_val = iisEncoderConfigGetSamplerateValue(outSampleRate);
    }
  }

  if (!isError(retError)) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ALLOW_SHORTEST_RAP_INTERVAL)) {
      retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_ALLOW_SHORTEST_RAP_INTERVAL, &hparam_allowShortestRapInterval);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hparam_allowShortestRapInterval) allowShortestRapInterval = hparam_allowShortestRapInterval->paramValue._int;
    } else {
      allowShortestRapInterval = 0;
    }
  }

  if (!isError(retError) && (loudnessLevelMeasuredSet || anchorLoudnessLevelMeasuredSet) && !loudnessLevelSet && !bMpeg4ProgRefLevelSet && !anchorLoudnessLevelSet && !liveLoudnessLevelSet) {
    if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD)) {
      retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD, &hParam_quietLoudnessThreshold);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
      if (hParam_quietLoudnessThreshold != NULL && hParam_quietLoudnessThreshold->paramFormat == PARAM_FLOAT) {
        quietLoudnessThreshold = hParam_quietLoudnessThreshold->paramValue._float;
      }
    } else {
      quietLoudnessThreshold = -35.0f;
    }
  }

  if (!isError(retError)) {
    if (cfgRapProperty == PARAMLIST_RAP_PROPERTY_INVALID) {
      if (cfgConfigSet == PARAMLIST_CONFIGSET_DASH && cfgRapOccurrence != PARAMLIST_RAP_OCCURRENCE_OFF) {
        cfgRapProperty = PARAMLIST_RAP_PROPERTY_SWITCHABLE;
      }

      else if (cfgConfigSet == PARAMLIST_CONFIGSET_SEEKABLE && cfgRapOccurrence != PARAMLIST_RAP_OCCURRENCE_OFF) {
        cfgRapProperty = PARAMLIST_RAP_PROPERTY_SEEKABLE;
      }

      else if ((cfgTransportFormat == PARAMLIST_TRANSPORTFORMAT_LATM ||
                cfgTransportFormat == PARAMLIST_TRANSPORTFORMAT_LATMLOAS) &&
               cfgRapOccurrence != PARAMLIST_RAP_OCCURRENCE_OFF) {
        cfgRapProperty = PARAMLIST_RAP_PROPERTY_ACCESS;
      } else if (cfgRapOccurrence == PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL || cfgRapOccurrence == PARAMLIST_RAP_OCCURRENCE_ON_DEMAND) {
        cfgRapProperty = PARAMLIST_RAP_PROPERTY_ACCESS;
      } else {
        cfgRapProperty = PARAMLIST_RAP_PROPERTY_OFF;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgRapOccurrence == PARAMLIST_RAP_OCCURRENCE_INVALID) {
      if (cfgRapProperty != PARAMLIST_RAP_PROPERTY_OFF) {
        cfgRapOccurrence = PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL;
      } else {
        cfgRapOccurrence = PARAMLIST_RAP_OCCURRENCE_OFF;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgRapOccurrence == PARAMLIST_RAP_OCCURRENCE_CONSTANT_INTERVAL) {
      if (cfgRapIntMs < 0) {
        if (cfgRapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE) {
          cfgRapIntMs = ENCCONFIG_SMC_INTERVAL_SEEKABLE_DEFAULT_MS;
        } else {
          cfgRapIntMs = ENCCONFIG_SMC_INTERVAL_DEFAULT_MS;
        }
      }

      if (cfgRapIntMs >= 0) {
        if (cfgRapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE || cfgRapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE) {
          retError = iisEncoderConfigConvertRapIntMsToSamplesRoundDown(cfgRapIntMs, outSamplerate_val, ENCCONFIG_SMC_INTERVAL_STEPS_RECOMMENDED, &cfgRapIntSamples);

        } else {
          retError = iisEncoderConfigConvertRapIntMsToSamplesRoundDown(cfgRapIntMs, outSamplerate_val, frameSamples_val, &cfgRapIntSamples);
        }
      }
    }
  }

  if (!isError(retError)) {
    if (audioPreRollBitResMode < 0) {
      if (cfgAot == PARAMLIST_AOT_42) {
        if (cfgChannelConfig == PARAMLIST_CHANNELCONFIG_STEREO &&
            (bitrate >= 72000 && bitrate < 192000) &&
            (!allowShortestRapInterval) &&
            (cfgRapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE || cfgRapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE)) {
          audioPreRollBitResMode = 0;
        } else {
          audioPreRollBitResMode = 1;
        }
      } else {
        audioPreRollBitResMode = 0;
      }
    }
  }

  if (!isError(retError)) {
    if (allowShortestRapInterval) {
      cfgAbsRapMinIntSamples = frameSamples_val;

    } else {
      if (cfgAot == PARAMLIST_AOT_42) {
        cfgAbsRapMinIntMs = ENCCONFIG_SMC_ABSOLUTE_RAP_MIN_INTERVAL_MS_USAC;
      } else if (cfgAot != PARAMLIST_AOT_INVALID) {
        cfgAbsRapMinIntMs = ENCCONFIG_SMC_ABSOLUTE_RAP_MIN_INTERVAL_MS_GENERAL;
      }

      if ((cfgRapOccurrence != PARAMLIST_RAP_OCCURRENCE_OFF)) {
        retError = iisEncoderConfigConvertRapIntMsToSamplesRoundDown(cfgAbsRapMinIntMs, outSamplerate_val, ENCCONFIG_SMC_INTERVAL_STEPS_RECOMMENDED, &cfgAbsRapMinIntSamples);
      }
    }
  }

  if (!isError(retError) && !iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_RAP_ON_DEMAND_ADVANCED_MODE)) {
    if ((cfgAot == PARAMLIST_AOT_42) && (cfgRapProperty == PARAMLIST_RAP_PROPERTY_SWITCHABLE || cfgRapProperty == PARAMLIST_RAP_PROPERTY_SEEKABLE) && (cfgRapOccurrence == PARAMLIST_RAP_OCCURRENCE_ON_DEMAND) && (audioPreRollBitResMode != 1)) {
      cfgRapOnDemandAdvancedMode = PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_ON;
    } else {
      cfgRapOnDemandAdvancedMode = PARAMLIST_RAP_ON_DEMAND_ADVANCED_MODE_OFF;
    }
  }

  if (!isError(retError)) {
    if (disableLoudness == PARAMLIST_DISABLELOUDNESS_INVALID) {
      if (cfgAot == PARAMLIST_AOT_42) {
        disableLoudness = PARAMLIST_DISABLELOUDNESS_OFF;
      } else {
        if (loudnessLevelSet || bMpeg4ProgRefLevelSet || loudnessLevelMeasuredSet || anchorLoudnessLevelSet || anchorLoudnessLevelMeasuredSet || liveLoudnessLevelSet) {
          disableLoudness = PARAMLIST_DISABLELOUDNESS_OFF;
        } else {
          disableLoudness = PARAMLIST_DISABLELOUDNESS_ON;
        }
      }
    }
  }

  if (!isError(retError)) {
    if (drcMode == PARAMLIST_DRCMODE_INVALID) {
      if (cfgAot == PARAMLIST_AOT_42) {
        drcMode = PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL;
      } else {
        drcMode = PARAMLIST_DRCMODE_OFF;
      }
    }
  }

  if (!isError(retError)) {
    int i = 0;
    for (i = 0; i < ENCCONFIG_MAX_DRC_SEQUENCES; i++) {
      if (drcCharacteristic[i] == PARAMLIST_DRCCHARACTERISTIC_INVALID) {
        drcCharacteristic[i] = PARAMLIST_DRCCHARACTERISTIC_DEFAULT;
      }
    }
  }

  if (!isError(retError)) {
    if (targetLra < 0.0f) {
      targetLra = ENCCONFIG_DEFAULT_DRC_TARGET_LOUDNESS_RANGE;
    }
  }

  if (!isError(retError)) {
    if (!lraControlDataSet && (drcMode == PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL)) {
      liveLraControl = PARAMLIST_LIVE_LRAC_ON;
    }
  }

  if (!isError(retError)) {
    if (cfgAot == PARAMLIST_AOT_42) {
      if (cfgIndepFlagIntervalMs == -1) {
        switch (cfgConfigSet) {
          case PARAMLIST_CONFIGSET_MP4:
          case PARAMLIST_CONFIGSET_DASH:
          case PARAMLIST_CONFIGSET_SEEKABLE:

            cfgIndepFlagIntervalMs = ENCCONFIG_USAC_MAX_IF_INTERVAL_DEFAULT_MS;
            break;
          default:
            retError = ENCODERCONFIG_ERROR_INVALID_CONFIGSET;
            break;
        }
      }

      if (cfgIndepFlagIntervalMs >= 0) {
        retError = iisEncoderConfigConvertRapIntMsToSamplesRoundDown(cfgIndepFlagIntervalMs, outSamplerate_val, frameSamples_val, &cfgIndepFlagIntervalSamples);
      }
    }
  }

  if (!isError(retError)) {
    if (primingMode == PARAMLIST_PRIMING_INVALID) {
      if (cfgAot == PARAMLIST_AOT_42) {
        if (cfgConfigSet == PARAMLIST_CONFIGSET_DASH || cfgConfigSet == PARAMLIST_CONFIGSET_SEEKABLE || cfgConfigSet == PARAMLIST_CONFIGSET_MP4) {
          primingMode = PARAMLIST_PRIMING_NONE;
        } else {
          primingMode = PARAMLIST_PRIMING_FULL;
        }
      }

      else if (cfgAot != PARAMLIST_AOT_42) {
        if (cfgConfigSet == PARAMLIST_CONFIGSET_DASH) {
          primingMode = PARAMLIST_PRIMING_NONE;
        } else {
          primingMode = PARAMLIST_PRIMING_FULL;
        }
      }
    }
  }

  if (!isError(retError)) {
    if (cfgAot == PARAMLIST_AOT_42) {
      if (primingMode != PARAMLIST_PRIMING_NONE &&
          (cfgConfigSet == PARAMLIST_CONFIGSET_SEEKABLE || cfgConfigSet == PARAMLIST_CONFIGSET_DASH || cfgConfigSet == PARAMLIST_CONFIGSET_MP4)) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_AOT_42_OPERATINGMODE_PRIMING;
      }
    }

    else if (cfgAot != PARAMLIST_AOT_42) {
      if (cfgConfigSet == PARAMLIST_CONFIGSET_DASH &&
          primingMode == PARAMLIST_PRIMING_FULL) {
        retError = ENCODERCONFIG_ERROR_INVALID_COMB_OPERATINGMODE_PRIMING;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_DRC_MODE, drcMode, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_LIVE_LRAC, liveLraControl, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (
        drcMode == PARAMLIST_DRCMODE_LN_NE_LI_GE_LRACONTROL) {
      retInfo = iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_MPEGD_DRC_TARGET_LOUDNESS_RANGE, targetLra, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_DISABLE_LOUDNESS, disableLoudness, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (cfgIndepFlagIntervalMs >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST, cfgIndepFlagIntervalMs, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgIndepFlagIntervalSamples >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_USAC_MAX_IF_DIST_SAMPLES, cfgIndepFlagIntervalSamples, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_PROPERTY, cfgRapProperty, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_OCCURRENCE, cfgRapOccurrence, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (cfgRapIntMs >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL, cfgRapIntMs, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgRapIntSamples >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES, cfgRapIntSamples, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (cfgAbsRapMinIntSamples >= 0) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_MIN_INTERVAL_SAMPLES, cfgAbsRapMinIntSamples, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_PRIMING_MODE, primingMode, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError) && (loudnessLevelMeasuredSet || anchorLoudnessLevelMeasuredSet) && !loudnessLevelSet && !bMpeg4ProgRefLevelSet && !anchorLoudnessLevelSet && !liveLoudnessLevelSet) {
    retInfo = iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD, quietLoudnessThreshold, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_FLUSHINGMODE, PARAMLIST_FLUSHINGMODE_DEFAULT, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SBRSIGNALING, PARAMLIST_SBRSIGNALING_OFF, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TNS, 1, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_HBE, PARAMLIST_HBE_OFF, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_AUDIOPREROLLOUTOFBITRES, audioPreRollBitResMode, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_PREROLLFRAMES, PARAMLIST_PREROLLFRAMES_ON, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_ENHANCED_PULSESEARCH, PARAMLIST_PULSESEARCH_ENHANCED_ON, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_RAP_ON_DEMAND_ADVANCED_MODE, cfgRapOnDemandAdvancedMode, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_LOWDELAYSWITCHING, 0, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_COREMODE, PARAMLIST_COREMODE_FD, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_STEREOCONFIGIDX, PARAMLIST_STEREOCONFIGIDX_NONE, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_TSD, PARAMLIST_TSD_OFF, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_SBRPVC, PARAMLIST_SBRPVC_OFF, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    retError = iisEncoderConfigSetDefaultParametersAACMetadata(hCodecParamList);
  }

  return retError;
}

static ENCODERCONFIG_RETURN_CODE iisEncoderConfigSetDefaultParametersAACMetadata(
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO retInfo = noError;

  PARAM_INSTANCE_HANDLE hparam_mpeg4DrcLight = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DrcHeavy = NULL;
  PARAM_INSTANCE_HANDLE hparam_metadataMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4DrcPresMode = NULL;
  PARAM_INSTANCE_HANDLE hparam_mpeg4WritePCEMixdownIdx = NULL;
  PARAM_INSTANCE_HANDLE hparam_channelConfig = NULL;
  PARAMLIST_MPEG4_DRC_LIGHT_PROF mpeg4DrcLight = PARAMLIST_MPEG4_DRC_LIGHT_PROF_INVALID;
  PARAMLIST_MPEG4_DRC_HEAVY_PROF mpeg4DrcHeavy = PARAMLIST_MPEG4_DRC_HEAVY_PROF_INVALID;
  PARAMLIST_MPEG4_METADATA_MODE metadataMode = PARAMLIST_MPEG4_METADATA_MODE_INVALID;
  PARAMLIST_MPEG4_DRCPRESENTATION mpeg4DrcPresMode = PARAMLIST_MPEG4_DRCPRESENTATION_INVALID;
  PARAMLIST_MPEG4_WRITE_PCE_MIXDN mpeg4WritePCEMixdownIdx = PARAMLIST_MPEG4_WRITE_PCE_MIXDN_INVALID;

  PARAMLIST_CHANNELCONFIG channelConfig = PARAMLIST_CHANNELCONFIG_INVALID;

  float mpeg4LightTargetRefLevel = -23.f;
  float mpeg4HeavyTargetRefLevel = -20.f;

  if (!isError(retError)) {
    if (hCodecParamList == NULL) {
      retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_PROF, &hparam_mpeg4DrcLight);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DrcLight != NULL && hparam_mpeg4DrcLight->paramFormat == PARAM_INT) {
      mpeg4DrcLight = (PARAMLIST_MPEG4_DRC_LIGHT_PROF)hparam_mpeg4DrcLight->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_PROF, &hparam_mpeg4DrcHeavy);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DrcHeavy != NULL && hparam_mpeg4DrcHeavy->paramFormat == PARAM_INT) {
      mpeg4DrcHeavy = (PARAMLIST_MPEG4_DRC_HEAVY_PROF)hparam_mpeg4DrcHeavy->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_METADATA_MODE, &hparam_metadataMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_metadataMode != NULL && hparam_metadataMode->paramFormat == PARAM_INT) {
      metadataMode = (PARAMLIST_MPEG4_METADATA_MODE)hparam_metadataMode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_PRES_MODE, &hparam_mpeg4DrcPresMode);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4DrcPresMode != NULL && hparam_mpeg4DrcPresMode->paramFormat == PARAM_INT) {
      mpeg4DrcPresMode = hparam_mpeg4DrcPresMode->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_WRITE_PCE_MIXDOWN_IDX, &hparam_mpeg4WritePCEMixdownIdx);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_mpeg4WritePCEMixdownIdx != NULL && hparam_mpeg4WritePCEMixdownIdx->paramFormat == PARAM_INT) {
      mpeg4WritePCEMixdownIdx = hparam_mpeg4WritePCEMixdownIdx->paramValue._int;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_CHANNELCONFIG, &hparam_channelConfig);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
    if (hparam_channelConfig != NULL && hparam_channelConfig->paramFormat == PARAM_INT) {
      channelConfig = hparam_channelConfig->paramValue._int;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_INVALID) {
      mpeg4DrcLight = PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT;
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcHeavy == PARAMLIST_MPEG4_DRC_HEAVY_PROF_INVALID) {
      mpeg4DrcHeavy = PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT;
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_INVALID) {
      metadataMode = PARAMLIST_MPEG4_METADATA_MODE_NONE;
    }
  }

  if (!isError(retError)) {
    if (metadataMode != PARAMLIST_MPEG4_METADATA_MODE_NONE && mpeg4DrcLight == PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      mpeg4DrcLight = PARAMLIST_MPEG4_DRC_LIGHT_PROF_NONE;
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_NONE && mpeg4DrcLight != PARAMLIST_MPEG4_DRC_LIGHT_PROF_NOT_PRESENT) {
      if (mpeg4DrcHeavy != PARAMLIST_MPEG4_DRC_HEAVY_PROF_NOT_PRESENT) {
        metadataMode = PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI;
      } else {
        metadataMode = PARAMLIST_MPEG4_METADATA_MODE_MPEG;
      }
    }
  }

  if (!isError(retError)) {
    if (mpeg4DrcPresMode == PARAMLIST_MPEG4_DRCPRESENTATION_MODE_2) {
      mpeg4LightTargetRefLevel = -23.f;
      mpeg4HeavyTargetRefLevel = -23.f;
    } else {
      mpeg4LightTargetRefLevel = -31.f;
      mpeg4HeavyTargetRefLevel = -20.f;
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_PROF, mpeg4DrcLight, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG || metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
      retInfo = iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_LIGHT_TARGET_LEVEL, mpeg4LightTargetRefLevel, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_PROF, mpeg4DrcHeavy, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
      retInfo = iisParamListAddParamValueFloat(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_HEAVY_TARGET_LEVEL, mpeg4HeavyTargetRefLevel, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_METADATA_MODE, metadataMode, PARAMLIST_MODE_APPENDIFMISSING);
    if (retInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }

  if (!isError(retError)) {
    if (metadataMode != PARAMLIST_MPEG4_METADATA_MODE_NONE) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_WRITE_PCE_MIXDOWN_IDX, PARAMLIST_MPEG4_WRITE_PCE_MIXDN_OFF, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_ETSI_DWNMIX_PRESENT, PARAMLIST_MPEG4_ETSIDMXPRESENT_FALSE, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI && channelConfig != PARAMLIST_CHANNELCONFIG_MONO && channelConfig != PARAMLIST_CHANNELCONFIG_STEREO) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_CENTER_MIX_LEVEL, PARAMLIST_MPEG4_DMX_GAIN_3_dB, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if ((metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG && mpeg4WritePCEMixdownIdx == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON && channelConfig != PARAMLIST_CHANNELCONFIG_MONO && channelConfig != PARAMLIST_CHANNELCONFIG_STEREO) ||
        (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI && channelConfig != PARAMLIST_CHANNELCONFIG_MONO && channelConfig != PARAMLIST_CHANNELCONFIG_STEREO)) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_SURROUND_MIX_LEVEL, PARAMLIST_MPEG4_DMX_GAIN_3_dB, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DOLBY_SURROUND_MODE, PARAMLIST_MPEG4_DSUR_NOT_INDICATED, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_DRC_PRES_MODE, PARAMLIST_MPEG4_DRCPRESENTATION_NOT_INDICATED, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    if (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG_ETSI ||
        (metadataMode == PARAMLIST_MPEG4_METADATA_MODE_MPEG && mpeg4WritePCEMixdownIdx == PARAMLIST_MPEG4_WRITE_PCE_MIXDN_ON && channelConfig != PARAMLIST_CHANNELCONFIG_MONO && channelConfig != PARAMLIST_CHANNELCONFIG_STEREO)) {
      retInfo = iisParamListAddParamValueInt(hCodecParamList, PARAMLIST_PARAMETER_MPEG4_PSEUDO_SUR_DMX_ENABLE, PARAMLIST_MPEG4_PSEUDO_SUR_OFF, PARAMLIST_MODE_APPENDIFMISSING);
      if (retInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  return retError;
}

static int isError(
    ENCODERCONFIG_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue != ENCODERCONFIG_NO_ERROR) {
    retValue = 1;
  }
  return retValue;
}

ENCODERCONFIG_RETURN_CODE iisEncoderConfigNew(
    ENCCONFIG_INSTANCE_HANDLE *phInstance) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  int nSize = 0;
  ENCCONFIG_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  nSize += sizeof(struct encconfig_instance_struct);
  nSize += sizeof(struct encconfig_private_data_struct);

  *phInstance = (ENCCONFIG_INSTANCE_HANDLE)iisMalloc(nSize);

  if (*phInstance != NULL) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (ENCCONFIG_PRIVATE_DATA_HANDLE)((char *)(*phInstance) + sizeof(struct encconfig_instance_struct));
    hPrivateData->nSize = nSize;

    sprintf((*phInstance)->infoModuleVersion, "%s %s", ENCCONFIG_MODULE_NAME, ENCCONFIG_VERSION_NUMBER);

    if (!isError(retError)) {
      errorInfo = iisEncoderConfigCTableNew(&hPrivateData->hEncoderConfigCTable);
      if (errorInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }

  } else {
    retError = ENCODERCONFIG_ERROR_MEMORY;
  }
  return retError;
}

ENCODERCONFIG_RETURN_CODE iisEncoderConfigUpdate(
    ENCCONFIG_INSTANCE_HANDLE hInstance,
    PARAMLIST_INSTANCE_HANDLE hUserParamList,
    PARAMLIST_INSTANCE_HANDLE hCodecParamList) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  ENCCONFIG_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (!isError(retError)) {
    if (hInstance == NULL) {
      retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retError)) {
    hPrivateData = iisEncoderConfigGetPrivateDataHandle(hInstance);
  }

  if (!isError(retError)) {
    if (hCodecParamList == NULL) {
      errorInfo = iisParamListNew(&hCodecParamList);
      if (errorInfo != noError) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      }
    }
  }

  if (!isError(retError)) {
    retError = iisEncoderConfigSetMandatoryParameters(hUserParamList);
  }

  if (!isError(retError)) {
    if (hPrivateData->hEncoderConfigCTable != NULL) {
      ENCCONFIGCTAB_RETURN_CODE retCodeCTable = ENCCONFIGCTAB_NO_ERROR;
      retCodeCTable = iisEncoderConfigCTableUpdate(hPrivateData->hEncoderConfigCTable,
                                                   hUserParamList,
                                                   hCodecParamList);
      if (retCodeCTable != ENCCONFIGCTAB_INFO_CONFIG_FOUND) {
        retError = ENCODERCONFIG_ERROR_NO_CONFIG_IN_CONFIGTABLE;
      }
    }
  }

  if (!isError(retError)) {
    retError = iisEncoderConfigSetSbrRatioDependingVals(hCodecParamList, hUserParamList);
  }
  if (!isError(retError)) {
    retError = iisEncoderConfigSetStereoConfigIdxDependingVals(hCodecParamList, hUserParamList);
  }

  if (!isError(retError)) {
    errorInfo = iisParamListCopyUnique(hCodecParamList, hUserParamList);
    if (errorInfo != noError) {
      retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
    }
  }
  if (!isError(retError)) {
    retError = iisEncoderConfigSetDefaultParameters(hCodecParamList);
  }
  if (!isError(retError)) {
    retError = iisEncoderConfigSanityCheck(hCodecParamList);
  }

  return retError;
}

ENCODERCONFIG_RETURN_CODE iisEncoderConfigDelete(
    ENCCONFIG_INSTANCE_HANDLE hInstance) {
  ENCODERCONFIG_RETURN_CODE retError = ENCODERCONFIG_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  ENCCONFIG_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hInstance != NULL) {
    hPrivateData = iisEncoderConfigGetPrivateDataHandle(hInstance);

    if (hPrivateData->hEncoderConfigCTable) {
      errorInfo = iisEncoderConfigCTableDelete(hPrivateData->hEncoderConfigCTable);
      if (errorInfo != noError && !isError(retError)) {
        retError = ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE;
      } else {
        hPrivateData->hEncoderConfigCTable = NULL;
      }
    }

    iisFree(hInstance);
    hInstance = NULL;
  } else {
    retError = ENCODERCONFIG_ERROR_INVALID_HANDLE;
  }
  return retError;
}
