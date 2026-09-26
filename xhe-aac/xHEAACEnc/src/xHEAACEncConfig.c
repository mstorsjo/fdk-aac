
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

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "xHEAACEncConfig.h"
#include "xHEAACEncCommon.h"
#include "xHEAACEncParameter.h"
#include "xHEAACEncWarningHandling.h"

#if defined __GNUC__ || defined __clang__
#define ADVANCED __attribute__((unused))
#else
#define ADVANCED
#endif
#include "xHEAACEncLoudness.h"

static IIS_XHEAACENC_RETURN_CODE adaptLoudnessInstanceInParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig);
static IIS_XHEAACENC_RETURN_CODE exchangeLoudnessInstanceInParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness);
static IIS_XHEAACENC_RETURN_CODE addLoudnessVerificationFlagToParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness);
static IIS_XHEAACENC_RETURN_CODE loudnessSanityChecks(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig);
static IIS_XHEAACENC_RETURN_CODE getMeasuredLoudnessAndSamplePeak(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, int* const loudnessInstanceAvailable, float* const loudnessMeasured,
                                                                  float* const samplePeakMeasured, int* const calculatedLoudnessReliable, int* const measuredLoudnessType);
static IIS_XHEAACENC_RETURN_CODE validateOrAddParamLoudnessLevel(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, float const loudnessMeasured,
                                                                 int const measuredLoudnessReliable, int const measuredLoudnessType);
static IIS_XHEAACENC_RETURN_CODE validateOrAddParamSamplePeak(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, float const samplePeakMeasured, int const measuredLoudnessReliable);
static IIS_XHEAACENC_RETURN_CODE determineLoudnessValidationOrAddition(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
                                                                       int* const validateProvidedLoudness,
                                                                       int* const addMeasuredLoudnessToConfig,
                                                                       int* const loudnessType,
                                                                       int const calculatedLoudnessReliable);
static IIS_XHEAACENC_RETURN_CODE determineSamplePeakValidationOrAddition(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, int* const validateProvidedSamplePeak, int* const addMeasuredSamplePeakToConfig, int const calculatedLoudnessReliable);
static IIS_XHEAACENC_RETURN_CODE getLoudnessInstanceFromParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE* const phLoudness, int* const loudnessInstanceAvailable);
static IIS_XHEAACENC_RETURN_CODE getMeasuredLoudnessFromParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, float* const measuredLoudnessInParamList, int* const measuredLoudnessAvailable);
static IIS_XHEAACENC_RETURN_CODE getQuietLoudnessThresholdFromParamList(IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig, float* const quietLoudnessThresholdInParamList, int* const quietLoudnessThresholdAvailable);
static int isError(IIS_XHEAACENC_RETURN_CODE errorValue);
static int isInternalConfigError(ENCODERCONFIG_RETURN_CODE const configError);
static void mapInternalConfigErrorToString(ENCODERCONFIG_RETURN_CODE const configError, IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Config_validateMeasuredLoudness(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int loudnessMeasuredAvailable = 0;
    int quietLoudnessThresholdAvailable = 0;
    float loudnessMeasured = XHEAACENC_INVALID_LOUDNESS_LEVEL;
    float quietLoudnessThreshold = XHEAACENC_INVALID_LOUDNESS_LEVEL;

    retValue = getMeasuredLoudnessFromParamList(hConfig, &loudnessMeasured, &loudnessMeasuredAvailable);

    if (!isError(retValue)) {
      retValue = getQuietLoudnessThresholdFromParamList(hConfig, &quietLoudnessThreshold, &quietLoudnessThresholdAvailable);
    }

    if (!isError(retValue) && loudnessMeasuredAvailable && quietLoudnessThresholdAvailable) {
      if (loudnessMeasured < quietLoudnessThreshold) {
        retValue = submitConfigWarning(&hConfig->warningList, IIS_XHEAACENC_WARN_QUIET_MEASURED_LOUDNESS);
      }
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getMeasuredLoudnessFromParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    float* const measuredLoudnessInParamList,
    int* const measuredLoudnessAvailable) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig || !measuredLoudnessInParamList || !measuredLoudnessAvailable) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *measuredLoudnessAvailable = iisParamListParamExists(hConfig->hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS);

    if (*measuredLoudnessAvailable) {
      PARAM_INSTANCE_HANDLE hParam_measuredLoudness = NULL;
      HANDLE_ERROR_INFO errorInfo = iisParamListGetParam(hConfig->hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS, &hParam_measuredLoudness);
      if (errorInfo != noError || !hParam_measuredLoudness) {
        retValue = IIS_XHEAACENC_ERROR_INTERNAL;
      } else {
        *measuredLoudnessInParamList = hParam_measuredLoudness->paramValue._float;
      }
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getQuietLoudnessThresholdFromParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    float* const quietLoudnessThresholdInParamList,
    int* const quietLoudnessThresholdAvailable) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig || !quietLoudnessThresholdInParamList || !quietLoudnessThresholdAvailable) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *quietLoudnessThresholdAvailable = iisParamListParamExists(hConfig->hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD);

    if (*quietLoudnessThresholdAvailable) {
      PARAM_INSTANCE_HANDLE hParam_quietLoudnessThreshold = NULL;
      HANDLE_ERROR_INFO errorInfo = iisParamListGetParam(hConfig->hCodecParamList, PARAMLIST_PARAMETER_QUIET_LOUDNESS_THRESHOLD, &hParam_quietLoudnessThreshold);
      if (errorInfo != noError || !hParam_quietLoudnessThreshold) {
        retValue = IIS_XHEAACENC_ERROR_PARAM_QUIET_LOUDNESS_THRESHOLD;
      } else {
        *quietLoudnessThresholdInParamList = hParam_quietLoudnessThreshold->paramValue._float;
      }
    }
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Config_validateAndAddParamDrcMetadata(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  int loudnessInstanceAvailable = 0;
  float loudnessMeasured = XHEAACENC_INVALID_LOUDNESS_LEVEL;
  float samplePeakMeasured = 0.f;
  int measuredLoudnessReliable = 0;
  int measuredLoudnessType = -1;

  if (!hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = loudnessSanityChecks(hConfig);
  }

  if (!isError(retValue)) {
    retValue = getMeasuredLoudnessAndSamplePeak(hConfig,
                                                &loudnessInstanceAvailable,
                                                &loudnessMeasured,
                                                &samplePeakMeasured,
                                                &measuredLoudnessReliable,
                                                &measuredLoudnessType);
  }

  if (!isError(retValue) && loudnessInstanceAvailable) {
    retValue = adaptLoudnessInstanceInParamList(hConfig);
  }

  if (!isError(retValue) && loudnessInstanceAvailable) {
    retValue = validateOrAddParamLoudnessLevel(hConfig,
                                               loudnessMeasured,
                                               measuredLoudnessReliable,
                                               measuredLoudnessType);
  }

  if (!isError(retValue) && loudnessInstanceAvailable) {
    retValue = validateOrAddParamSamplePeak(hConfig, samplePeakMeasured, measuredLoudnessReliable);
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE loudnessSanityChecks(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE hLoudness = NULL;
  int loudnessInstanceAvailable = 0;

  if (!isError(retValue)) {
    retValue = getLoudnessInstanceFromParamList(hConfig, &hLoudness, &loudnessInstanceAvailable);
  }
  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getMeasuredLoudnessAndSamplePeak(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    int* const loudnessInstanceAvailable,
    float* const loudnessMeasured,
    float* const samplePeakMeasured,
    int* const measuredLoudnessReliable,
    int* const measuredLoudnessType) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE hLoudness = NULL;

  if (!hConfig || !loudnessInstanceAvailable || !loudnessMeasured || !samplePeakMeasured || !measuredLoudnessReliable) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = getLoudnessInstanceFromParamList(hConfig, &hLoudness, loudnessInstanceAvailable);
  }

  if (!isError(retValue) && *loudnessInstanceAvailable) {
    *measuredLoudnessType = 0;
    *loudnessMeasured = hLoudness->loudness;
    *samplePeakMeasured = hLoudness->samplePeak;
    *measuredLoudnessReliable = hLoudness->hLoudnessData->minRequiredSamplesProcessed;
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE adaptLoudnessInstanceInParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE hLoudness = NULL;
  int loudnessInstanceAvailable = 0;

  if (!hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = getLoudnessInstanceFromParamList(hConfig, &hLoudness, &loudnessInstanceAvailable);
  }

  if (!isError(retValue) && loudnessInstanceAvailable) {
    retValue = exchangeLoudnessInstanceInParamList(hConfig, hLoudness);
  }

  if (!isError(retValue) && loudnessInstanceAvailable) {
    retValue = addLoudnessVerificationFlagToParamList(hConfig, hLoudness);
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE exchangeLoudnessInstanceInParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig || !hLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = iisParamListRemoveParamTag(hConfig->hUserParamList, IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA);
    if (errorInfo != noError) {
      retValue = IIS_XHEAACENC_ERROR_INITIALIZATION;
    }
  }

  if (!isError(retValue)) {
    retValue = IIS_xHEAACEnc_Config_AddParamValuePointer(hConfig, IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA, hLoudness->hLoudnessData);
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE addLoudnessVerificationFlagToParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE const hLoudness) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig || !hLoudness) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE validateOrAddParamLoudnessLevel(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    float const loudnessMeasured,
    int const measuredLoudnessReliable,
    int const measuredLoudnessType) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (measuredLoudnessType != 0 && measuredLoudnessType != 1) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_VALUE;
  }

  if (!isError(retValue)) {
    int validateProvidedLoudness;
    int addMeasuredLoudnessToConfig;
    int providedLoudnessType;
    retValue = determineLoudnessValidationOrAddition(hConfig, &validateProvidedLoudness, &addMeasuredLoudnessToConfig, &providedLoudnessType, measuredLoudnessReliable);

    if (!isError(retValue)) {
      if (validateProvidedLoudness) {
      }

      else if (addMeasuredLoudnessToConfig) {
        HANDLE_ERROR_INFO errorInfo = iisParamListAddParamValueFloat(hConfig->hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS, loudnessMeasured, PARAMLIST_MODE_APPEND);
        if (errorInfo != noError) {
          retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
        }
      }
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE validateOrAddParamSamplePeak(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    float const samplePeakMeasured,
    int const measuredLoudnessReliable) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int validateProvidedSamplePeak;
    int addMeasuredSamplePeakToConfig;

    retValue = determineSamplePeakValidationOrAddition(hConfig, &validateProvidedSamplePeak, &addMeasuredSamplePeakToConfig, measuredLoudnessReliable);

    if (!isError(retValue)) {
      if (validateProvidedSamplePeak) {
      } else if (addMeasuredSamplePeakToConfig) {
        HANDLE_ERROR_INFO errorInfo = iisParamListAddParamValueFloat(hConfig->hCodecParamList, PARAMLIST_PARAMETER_INTERNAL_MEASURED_SAMPLE_PEAK, samplePeakMeasured, PARAMLIST_MODE_APPEND);
        if (errorInfo != noError) {
          retValue = IIS_XHEAACENC_ERROR_LOUDNESS_MEASUREMENT;
        }
      }
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE determineLoudnessValidationOrAddition(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    int* const validateProvidedLoudness,
    int* const addMeasuredLoudnessToConfig,
    ADVANCED int* const loudnessType,
    int const measuredLoudnessReliable

) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (!hConfig || !validateProvidedLoudness || !addMeasuredLoudnessToConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int providedLoudnessAvailable = 0;
    int providedProgRefLevelAvailable = 0;
    int providedAnchorLoudnessAvailable = 0;

    int disableLoudnessFlags = 0;

    *validateProvidedLoudness = (providedLoudnessAvailable || providedProgRefLevelAvailable || providedAnchorLoudnessAvailable);
    *addMeasuredLoudnessToConfig = !(*validateProvidedLoudness) &&
                                   !disableLoudnessFlags &&
                                   !providedLoudnessAvailable &&
                                   !providedProgRefLevelAvailable &&
                                   !providedAnchorLoudnessAvailable;

    if (!measuredLoudnessReliable) {
      if (*validateProvidedLoudness) {
        *validateProvidedLoudness = 0;
        retValue = submitConfigWarning(&hConfig->warningList, IIS_XHEAACENC_WARN_TOO_FEW_SAMPLES_TO_VALIDATE_LOUDNESS);
      } else if (*addMeasuredLoudnessToConfig) {
        *addMeasuredLoudnessToConfig = 0;
        retValue = IIS_XHEAACENC_ERROR_LOUDNESS_PROCESS_SAMPLES_TOO_FEW;
      }
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE determineSamplePeakValidationOrAddition(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    int* const validateProvidedSamplePeak,
    int* const addMeasuredSamplePeakToConfig,
    int const measuredLoudnessReliable) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  int disableLoudnessMeasurement = 0;

  if (!hConfig || !validateProvidedSamplePeak || !addMeasuredSamplePeakToConfig) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (measuredLoudnessReliable) {
      int loudnessAvailable = 0;
      int anchorLoudnessAvailable = 0;
      int samplePeakAvailable = 0;

      *validateProvidedSamplePeak = samplePeakAvailable && (loudnessAvailable || anchorLoudnessAvailable);
      *addMeasuredSamplePeakToConfig = !(*validateProvidedSamplePeak) && !samplePeakAvailable && !disableLoudnessMeasurement;

      if (samplePeakAvailable && !loudnessAvailable && !anchorLoudnessAvailable) {
        retValue = IIS_XHEAACENC_ERROR_SAMPLEPEAK_NO_LOUDNESS;
      }
    } else {
      *validateProvidedSamplePeak = 0;
      *addMeasuredSamplePeakToConfig = 0;
    }
  }

  return retValue;
}

static IIS_XHEAACENC_RETURN_CODE getLoudnessInstanceFromParamList(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE const hConfig,
    IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE* const phLoudness,
    int* const loudnessInstanceAvailable) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  if (!hConfig || !phLoudness || !loudnessInstanceAvailable) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *loudnessInstanceAvailable = IIS_xHEAACEnc_ParamExists(hConfig->hUserParamList, IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA);
  }

  if (!isError(retValue) && *loudnessInstanceAvailable) {
    PARAM_INSTANCE_HANDLE hParam_loudnessInstance = NULL;
    errorInfo = iisParamListGetParam(hConfig->hUserParamList, IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA, &hParam_loudnessInstance);

    if (errorInfo != noError || !hParam_loudnessInstance) {
      retValue = IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_DATA;
    } else {
      *phLoudness = (IIS_XHEAACENC_LOUDNESS_INSTANCE_HANDLE)hParam_loudnessInstance->paramValue._pvoid;
    }
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Config_refineAndVerifyEncoderConfig(
    IIS_XHEAACENC_MESSAGE_CALLBACK const messageCallback,
    PARAMLIST_INSTANCE_HANDLE const hUserParamList,
    PARAMLIST_INSTANCE_HANDLE hCodecParamList,
    PARAMLIST_INSTANCE_HANDLE const hMappedUserParamList) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  ENCODERCONFIG_RETURN_CODE retConfig = ENCODERCONFIG_NO_ERROR;
  ENCCONFIG_INSTANCE_HANDLE hEncoderConfig = NULL;

  if (!isError(retValue)) {
    if (hUserParamList == NULL || hCodecParamList == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (!isError(retValue)) {
    retConfig = iisEncoderConfigNew(&hEncoderConfig);
    retValue = IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(messageCallback, retConfig);
  }

  if (!isError(retValue)) {
    retConfig = iisEncoderConfigUpdate(hEncoderConfig,
                                       hMappedUserParamList,
                                       hCodecParamList);
    retValue = IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(messageCallback, retConfig);
  }

  if (!isError(retValue)) {
    if (hCodecParamList->storedElementsCount == 0) {
      IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_SEC, messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = iisParamListCompare(hMappedUserParamList,
                                                      hCodecParamList,
                                                      IIS_xHEAACEnc_ParamListCompareCB,
                                                      NULL);
    if (errorInfo != noError) {
      IIS_xHEAACEnc_mapInternalParamListErrorToString(PARAMLIST_ERROR_COMPARE, messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }
    freeErrorTraceback(errorInfo);
  }

  if (hEncoderConfig) {
    retConfig = iisEncoderConfigDelete(hEncoderConfig);
    if (!isError(retValue)) {
      retValue = IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(messageCallback, retConfig);
    }
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_Config_mappingConfigErrorToInternal(
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback,
    ENCODERCONFIG_RETURN_CODE configError) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (isInternalConfigError(configError)) {
    mapInternalConfigErrorToString(configError, messageCallback);
    retValue = IIS_XHEAACENC_ERROR_INTERNAL;
  } else {
    switch (configError) {
      case ENCODERCONFIG_NO_ERROR:
        retValue = IIS_XHEAACENC_NO_ERROR;
        break;
      case ENCODERCONFIG_ERROR_MEMORY:
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
        break;
      case ENCODERCONFIG_ERROR_INVALID_CHANNELCONFIG:
        retValue = IIS_XHEAACENC_ERROR_PARAM_CHANNELCONFIG;
        break;
      case ENCODERCONFIG_ERROR_INVALID_BITRATE:
        retValue = IIS_XHEAACENC_ERROR_PARAM_BITRATE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_BITRATEMODE:
        retValue = IIS_XHEAACENC_ERROR_PARAM_BITRATEMODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_INPUT_SR:
        retValue = IIS_XHEAACENC_ERROR_PARAM_INSAMPLERATE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_OUTPUT_SR:
        retValue = IIS_XHEAACENC_ERROR_PARAM_OUTSAMPLERATE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_INSAMPLERATE_AOT_OUTSAMPLERATE:
        retValue = IIS_XHEAACENC_ERROR_COMB_INSAMPLERATE_UNSUPPORTED;
        break;
      case ENCODERCONFIG_ERROR_INVALID_FRAMESAMPLES:
        retValue = IIS_XHEAACENC_ERROR_PARAM_FRAMESAMPLES;
        break;
      case ENCODERCONFIG_ERROR_INVALID_PRESET:
        retValue = IIS_XHEAACENC_ERROR_PARAM_PRESET;
        break;
      case ENCODERCONFIG_ERROR_INVALID_CONFIGSET:
      case ENCODERCONFIG_ERROR_FORBIDDEN_DRM_WITHOUT_DEVELOPER:
        retValue = IIS_XHEAACENC_ERROR_PARAM_OPERATING_MODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_TRANSPORTFORMAT:
        retValue = IIS_XHEAACENC_ERROR_PARAM_TRANSPORTFORMAT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_AOT:
        retValue = IIS_XHEAACENC_ERROR_PARAM_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_RAP_OCCURRENCE:
        retValue = IIS_XHEAACENC_ERROR_PARAM_RAP_OCCURRENCE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_USAC_IFI:
        retValue = IIS_XHEAACENC_ERROR_PARAM_MAX_IF_DISTANCE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_USAC_IFI_DRM:
        retValue = IIS_XHEAACENC_ERROR_PARAM_MAX_IF_DISTANCE_DRM;
        break;
      case ENCODERCONFIG_ERROR_INVALID_RAP_INTERVAL:
        retValue = IIS_XHEAACENC_ERROR_PARAM_RAP_INTERVAL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_LOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_LEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_ALBUM_LOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAM_ALBUM_LOUDNESS_LEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_ANCHOR_LOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAM_ANCHOR_LOUDNESS_LEVEL;
        break;
      case ENCODERCONFIG_ERROR_NO_LOUDNESS_PROVIDED:
        retValue = IIS_XHEAACENC_ERROR_PARAM_NO_LOUDNESS_PROVIDED;
        break;
      case ENCODERCONFIG_ERROR_INVALID_QUIET_LOUDNESS_THRESHOLD:
        retValue = IIS_XHEAACENC_ERROR_PARAM_QUIET_LOUDNESS_THRESHOLD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_DRCMODE:
        retValue = IIS_XHEAACENC_ERROR_PARAM_DRCMODE;
        break;
      case ENCODERCONFIG_ERROR_NO_CONFIG_IN_CONFIGTABLE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_BASE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_CORESAMPLERATE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_CORESR;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_DRM:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_DRM;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_BITRATE_BITRATEMODE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_BITRATE_BITRATEMODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_BITRATEMODE_CHANNELCONFIG:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_BITRATEMODE_CHANNELCONFIG;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_SAMPLERATE_NOSBR_DRM30:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_SR_NOSBR_DRM30;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_LOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_LOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_LRACONTROL_DATA:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_LRACONTROL_DATA;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_TARGET_LRA:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_TARGET_LOUDNESS_RANGE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MODE_LIVE_LRA_CONTROL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MODE_LIVE_LRA_CONTROL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_CHANNELCONFIG_LIVELOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_CHANNELCONFIG_LIVELOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_DISABLELOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_DISABLELOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_ALBUMLOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_ALBUMLOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_ANCHORLOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_ANCHORLOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LIVELOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_LIVELOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVELOUDNESSLEVEL_LOUDNESSLEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_SAMPLEPEAK_LIVESAMPLEPEAK:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_SAMPLEPEAK_LIVESAMPLEPEAK;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LIVELOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LIVELOUDNESSLEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LIVESAMPLEPEAK_NOLIVELOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LIVESAMPLEPEAK_NOLIVELOUDNESSLEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_SAMPLEPEAK_LIVELOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_SAMPLEPEAK_LIVELOUDNESSLEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_NO_LIVELOUDNESSLEVEL_LIVEMODE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_NO_LIVELOUDNESSLEVEL_LIVEMODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_NO_LIVELOUDNESSLEVEL_LIVERELMAXGAIN:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_NO_LIVELOUDNESSLEVEL_LIVELOUDNESSRELMAXGAIN;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_OPERATINGMODE_PRIMING:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_OPERATINGMODE_PRIMING;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_42_OPERATINGMODE_PRIMING:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_42_OPERATINGMODE_PRIMING_MODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LOUDNESS_DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LOUDNESS_DRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_DISABLELOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_DISABLELOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_ALBUMLOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_ALBUMLOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_ANCHORLOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_ANCHORLOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_LOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_LOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_ANCHORLOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_ANCHORLOUDNESSLEVEL_QUIETLOUDNESSTHRESHOLD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_LOUDNESSLEVEL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_LOUDNESSLEVEL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_MPEGDDRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEL_MPEGDDRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_AOT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEl_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_PROGREFLEVEL_QUIETLOUDNESSTHRESHOLD:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_PROGREFLEVEL_QUIETLOUDNESSTHRESHOLD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_FLUSHINGMODE_DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_FLUSHMODE_DRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_TRANSPORTFORMAT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_TRANSPORTFORMAT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_OCCURRENCE_RAP_INTERVAL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_OCCURRENCE_RAP_INTERVAL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_INTERVAL_RAP_MIN_INTERVAL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_RAP_INTERVAL_RAP_MIN_INTERVAL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_DRM:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_DRM;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MPEG2AAC_AOT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG2AAC_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_NOSTARTSTOP_AOT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_NOSTARTSTOP_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MPEG4DRC_LOUDNESS:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_LOUDNESS;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MDMODE_PROGREFLVL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MDMODE_PROGREFLVL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MPEG4DRC_DISABLELOUDNESSMEASUREMENT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_DISABLELOUDNESSMEASUREMENT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MPEG4DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_MPEG4DRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MD:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_MD;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_NONE_DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_NONE_DRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_MPEG_D_DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_DRC_MODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_DRC_MPEG4DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_DRC_MPEG4DRC;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_VBR_MPEG4DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_VBR_MPEG4DRC;
        break;
      case ENCODERCONFIG_ERROR_REQUIRED_TARGETLOUDNESS_FOR_MPEG4DRCLIGHT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_TARGETLOUDNESS_FOR_MPEG4DRCLIGHT;
        break;
      case ENCODERCONFIG_ERROR_REQUIRED_TARGETLOUDNESS_FOR_MPEG4DRCHEAVY:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_TARGETLOUDNESS_FOR_MPEG4DRCHEAVY;
        break;
      case ENCODERCONFIG_ERROR_REQUIRED_MPEG4PROGREFLEVEL_FOR_MPEG4DRC:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4PROGREFLEVEL_MPEG4DRC;
        break;
      case ENCODERCONFIG_ERROR_REQUIRED_MPEG4DRC_FOR_MPEG4METADATA:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRC_FOR_MPEG4METADATA;
        break;
      case ENCODERCONFIG_ERROR_FORBIDDEN_MPEG4DRCHEAVY_NOT_MPEG4DRCLIGHT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MPEG4DRCHEAVY_NO_MPEG4DRCLIGHT;
        break;
      case ENCODERCONFIG_ERROR_FORBIDDEN_STREAMID_FOR_NOT_AOT42:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_STREAMID_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_PRESET_AOT:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_PRESET_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_PRESET_AOT_CHANNELCONFIG:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_PRESET_AOT_CHANNELCONFIG;
        break;
      case ENCODERCONFIG_ERROR_REQUIRED_STREAMID_FOR_SWITCHABLE_AND_AOT42:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_STREAMID_SWITCHABLE_AOT;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_SHORTEST_RAP_INTVL_RAP_OCCURRENCE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_SHORTEST_RAP_INTVL_RAP_OCCURRENCE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_MPEG_DRC_HEAVY:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MPEG_DRC_HEAVY;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_DRC_PRES_MODE:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_DRC_PRES_MODE;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_CHANNELCONFIG_DOWNMIX:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_CHANNELCONFIG_DOWNMIX;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_CHANNELCONFIG_DOWNMIX:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_CHANNELCONFIG_DOWNMIX;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_DRC_PRES_TARGETREFLVL:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_DRC_PRES_TARGETREFLVL;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_MODE_SURROUND:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_MODE_SURROUND;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_MD_CHANNELCONFIG_SURROUND:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_MD_CHANNELCONFIG_SURROUND;
        break;
      case ENCODERCONFIG_ERROR_INVALID_COMB_AOT_SBRSIGNALING:
        retValue = IIS_XHEAACENC_ERROR_PARAMCOMB_AOT_SBRSIGNALING;
        break;

      case ENCODERCONFIG_ERROR_INVALID_HANDLE:
      case ENCODERCONFIG_ERROR_INVALID_PARAM:
      case ENCODERCONFIG_ERROR_UNKNOWN_ERROR_IN_SUBMODULE:
      case ENCODERCONFIG_WARNING_RESERVED:
      default:
        assert(0);
        retValue = IIS_XHEAACENC_ERROR_UNKNOWN;
        break;
    }
  }

  return retValue;
}

static int isInternalConfigError(
    ENCODERCONFIG_RETURN_CODE const configError) {
  int isInternalError = 0;

  switch (configError) {
    case ENCODERCONFIG_ERROR_INVALID_SBR_RATIO:
    case ENCODERCONFIG_ERROR_INVALID_SKIPDELAY:
    case ENCODERCONFIG_ERROR_INVALID_RAP_PROPERTY:
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_RAP_OCCURRENCE:
    case ENCODERCONFIG_ERROR_FORBIDDEN_RAP_PROPERTY_CONFIGSET:
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_PRIMING:
    case ENCODERCONFIG_ERROR_INVALID_COMB_AOT42_RAP_PROPERTY_PRIMING:
    case ENCODERCONFIG_ERROR_INVALID_COMB_TSD_UNIFIEDSTEREO:
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_ON_DEMAND_ADVANCED_MODE:
    case ENCODERCONFIG_ERROR_INVALID_COMB_SHORTEST_RAP_INTVL_APR_BITRESMODE:
    case ENCODERCONFIG_ERROR_MISSING_QUIET_LOUDNESS_THRESHOLD:

      isInternalError = 1;
      break;
    default:
      isInternalError = 0;
  }

  return isInternalError;
}

static void mapInternalConfigErrorToString(
    ENCODERCONFIG_RETURN_CODE const configError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  char* pErrorString = NULL;

  switch (configError) {
    case ENCODERCONFIG_ERROR_INVALID_SBR_RATIO:
      pErrorString = "Internal error :  C1001";
      break;
    case ENCODERCONFIG_ERROR_INVALID_SKIPDELAY:
      pErrorString = "Internal error :  C1003";
      break;
    case ENCODERCONFIG_ERROR_INVALID_RAP_PROPERTY:
      pErrorString = "Internal error :  C1004";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_RAP_OCCURRENCE:
      pErrorString = "Internal error :  C1005";
      break;
    case ENCODERCONFIG_ERROR_FORBIDDEN_RAP_PROPERTY_CONFIGSET:
      pErrorString = "Internal error :  C1006";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_PROPERTY_PRIMING:
      pErrorString = "Internal error :  C1007";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_AOT42_RAP_PROPERTY_PRIMING:
      pErrorString = "Internal error :  C1008";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_TSD_UNIFIEDSTEREO:
      pErrorString = "Internal error :  C1009";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_RAP_ON_DEMAND_ADVANCED_MODE:
      pErrorString = "Internal error :  C1010";
      break;
    case ENCODERCONFIG_ERROR_INVALID_COMB_SHORTEST_RAP_INTVL_APR_BITRESMODE:
      pErrorString = "Internal error :  C1011";
      break;
    case ENCODERCONFIG_ERROR_MISSING_QUIET_LOUDNESS_THRESHOLD:
      pErrorString = "Internal error :  C1012";
      break;
    default:
      pErrorString = "Internal error :  unknown";
  }
  IIS_xHEAACEnc_Message(messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, pErrorString);
}

static int isError(
    IIS_XHEAACENC_RETURN_CODE errorValue) {
  int retValue = 0;
  if (errorValue >= IIS_XHEAACENC_ERROR_FIRST) {
    retValue = 1;
  }
  return retValue;
}
