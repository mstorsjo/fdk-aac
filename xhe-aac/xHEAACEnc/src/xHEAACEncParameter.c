
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
#include <string.h>
#include <limits.h>
#include "iisutillib.h"
#include "iisParamList.h"
#include "xHEAACEnc.h"
#include "xHEAACEncCommon.h"
#include "xHEAACEncParameter.h"
#include "xHEAACEncMapParameters.h"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

static void messageCallback(char *str) {
  fputs(str, stdout);
  fflush(stdout);
}

static void mapInternalDevAdvErrorString(
    int paramTag,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

void mapInternalParamErrorToString(
    PARAMLIST_ERROR_CODE const paramListError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback);

static int isInSampleRateSupported(
    int const inSamplingRate);

static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_AddParam(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat,
    void const *const paramValue,
    int const paramLength);

static int isZeroOrOne(
    int flag);

static IIS_XHEAACENC_RETURN_CODE IIS_xHEAACEnc_AddParam(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat,
    void const *const paramValue,
    int const paramLength) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE paramAdd = {0};
  int copyVoidPointer = 0;

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    if (hConfig == NULL || paramValue == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    paramAdd.paramLength = paramLength;
    paramAdd.paramTag = (int)paramTag;
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    switch ((int const)paramTag) {
      case IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA:
        if (paramFormat == IIS_XHEAACENC_PARAM_VOID_POINTER) {
          paramAdd.paramFormat = PARAM_VOID_POINTER;
        } else {
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
        }
        break;

      case IIS_XHEAACENC_PARAMETER_AOT:
      case IIS_XHEAACENC_PARAMETER_BITRATEMODE:
      case IIS_XHEAACENC_PARAMETER_BITRATE:
      case IIS_XHEAACENC_PARAMETER_INSAMPLERATE:
      case IIS_XHEAACENC_PARAMETER_CHANNELCONFIG:
      case IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT:
      case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS:

      case IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE:
      case IIS_XHEAACENC_PARAMETER_STREAMID:
      case IIS_XHEAACENC_PARAMETER_LIVE_MODE:
        if (paramFormat == IIS_XHEAACENC_PARAM_INT) {
          paramAdd.paramFormat = PARAM_INT;
        } else {
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
        }
        break;

      case IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL:
        if (paramFormat == IIS_XHEAACENC_PARAM_FLOAT) {
          paramAdd.paramFormat = PARAM_FLOAT;
        } else {
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
        }
        break;

      case IIS_XHEAACENC_PARAMETER_LIB_NAME:
      case IIS_XHEAACENC_PARAMETER_BITRATELIMIT:
      case IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX:
      case IIS_XHEAACENC_PARAMETER_PRIMING:
      case IIS_XHEAACENC_PARAMETER_OUTSAMPLERATE:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_MAX:
      case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_SAMPLES:
      case IIS_XHEAACENC_PARAMETER_RAP_MIN_INTERVAL_SAMPLES:
      case IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE:
      case IIS_XHEAACENC_PARAMETER_STANDARDDELAY:
      case IIS_XHEAACENC_PARAMETER_CODECDELAY:
      case IIS_XHEAACENC_PARAMETER_FRAMESAMPLES:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT:
      case IIS_XHEAACENC_PARAMETER_PROFILE_LEVEL:
      case IIS_XHEAACENC_PARAMETER_MAXBITRATEPERSEGMENT:
        paramAdd.paramFormat = PARAM_INT;
        retValue = IIS_XHEAACENC_ERROR_PARAM_READ_ONLY;
        break;

      default:
        assert(0);

        retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
        break;
    }
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    switch (paramAdd.paramFormat) {
      case PARAM_CHAR_ARRAY:
      case PARAM_SHORT_ARRAY:
      case PARAM_INT_ARRAY:
      case PARAM_FLOAT_ARRAY:
      case PARAM_DOUBLE_ARRAY:
        if (paramLength < 0) {
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMLENGTH;
        }
        break;
      case PARAM_CHAR:
      case PARAM_SHORT:
      case PARAM_INT:
      case PARAM_FLOAT:
      case PARAM_DOUBLE:
      case PARAM_VOID_POINTER:
        if (paramLength != 1) {
          retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMLENGTH;
        }
        break;
      default:
        retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
        break;
    }
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    switch (paramAdd.paramFormat) {
      case PARAM_CHAR:
        paramAdd.paramValue._char = *((char const *)paramValue);
        break;
      case PARAM_SHORT:
        paramAdd.paramValue._short = *((short const *)paramValue);
        break;
      case PARAM_INT:
        paramAdd.paramValue._int = *((int const *)paramValue);
        break;
      case PARAM_FLOAT:
        paramAdd.paramValue._float = *((float const *)paramValue);
        break;
      case PARAM_DOUBLE:
        paramAdd.paramValue._double = *((double const *)paramValue);
        break;
      case PARAM_VOID_POINTER:
        paramAdd.paramValue._pvoid = (void *)paramValue;
        break;
      case PARAM_CHAR_ARRAY:
        copyVoidPointer = sizeof(char) * paramLength;
        break;
      case PARAM_SHORT_ARRAY:
        copyVoidPointer = sizeof(short) * paramLength;
        break;
      case PARAM_INT_ARRAY:
        copyVoidPointer = sizeof(int) * paramLength;
        break;
      case PARAM_FLOAT_ARRAY:
        copyVoidPointer = sizeof(float) * paramLength;
        break;
      case PARAM_DOUBLE_ARRAY:
        copyVoidPointer = sizeof(double) * paramLength;
        break;
      default:
        retValue = IIS_XHEAACENC_ERROR_INVALID_PARAMFORMAT;
        break;
    }
    if (copyVoidPointer > 0) {
      paramAdd.paramValue._pvoid = iisMalloc(copyVoidPointer);
      if (paramAdd.paramValue._pvoid != NULL) {
        memcpy(paramAdd.paramValue._pvoid, paramValue, copyVoidPointer);
      } else {
        copyVoidPointer = 0;
        retValue = IIS_XHEAACENC_ERROR_MEMORY;
      }
    }
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    errorInfo = iisParamListAddParam(hConfig->hUserParamList, &paramAdd, PARAMLIST_MODE_REPLACE);
  }

  if ((retValue < IIS_XHEAACENC_ERROR_FIRST) && (errorInfo != noError)) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (copyVoidPointer > 0) {
    iisFree(paramAdd.paramValue._pvoid);
    paramAdd.paramValue._pvoid = NULL;
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Open(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE *phConfig,
    int const inSampleRate,
    IIS_XHEAACENC_CHANNELCONFIG const channelConfig) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;
  IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig = NULL;

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    if (phConfig == NULL) {
      retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    if (!isInSampleRateSupported(inSampleRate)) {
      retValue = IIS_XHEAACENC_ERROR_INSAMPLERATE_UNSUPPORTED;
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    if (*phConfig == NULL) {
      (*phConfig) = iisCalloc(1, sizeof(IIS_XHEAACENC_CONFIG_INSTANCE));
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    if (!*phConfig) {
      retValue = IIS_XHEAACENC_ERROR_MEMORY_ALLOCATION;
    }
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    HANDLE_ERROR_INFO errorInfo = noError;

    if (!(*phConfig)->hUserParamList) {
      errorInfo = iisParamListNew(&((*phConfig)->hUserParamList));
    }

    if (!(*phConfig)->hCodecParamList && errorInfo == noError) {
      errorInfo = iisParamListNew(&((*phConfig)->hCodecParamList));
    }
    if (errorInfo != noError) {
      mapInternalParamErrorToString(PARAMLIST_ERROR_NEW_LIST, messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }

    if (errorInfo) freeErrorTraceback(errorInfo);
  }

  if (retValue == IIS_XHEAACENC_NO_ERROR) {
    hConfig = (*phConfig);

    if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
      retValue = IIS_xHEAACEnc_Config_AddParamValueInt(hConfig, IIS_XHEAACENC_PARAMETER_CHANNELCONFIG, channelConfig);
    }

    if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
      retValue = IIS_xHEAACEnc_Config_AddParamValueInt(hConfig, IIS_XHEAACENC_PARAMETER_INSAMPLERATE, inSampleRate);
    }
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_Delete(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig != NULL) {
    if (hConfig->hUserParamList) {
      errorInfo = iisParamListDelete(hConfig->hUserParamList);
      if (errorInfo == noError) {
        hConfig->hUserParamList = NULL;
      }
    }

    if (hConfig->hCodecParamList && errorInfo == noError) {
      errorInfo = iisParamListDelete(hConfig->hCodecParamList);
      if (errorInfo == noError) {
        hConfig->hCodecParamList = NULL;
      }
    }

    if (errorInfo != noError) {
      mapInternalParamErrorToString(PARAMLIST_ERROR_DELETE, hConfig->messageCallback);
      retValue = IIS_XHEAACENC_ERROR_INTERNAL;
    }
    iisFree(hConfig);
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return retValue;
}

static int checkFLOATBitExactness(float *a, float *b) {
  int result = (*(int *)a == *(int *)b);
  return result;
}
#ifdef _MSC_VER
typedef __int64 myLongLong;
#else
typedef long long myLongLong;
#endif

static int checkDOUBLEBitExactness(double *a, double *b) {
  int result = (*(myLongLong *)a == *(myLongLong *)b);
  return result;
}

int IIS_xHEAACEnc_ParamListCompareCB(
    void *ptr,
    PARAMLIST_INSTANCE_HANDLE const hParamListUser,
    PARAM_INSTANCE_HANDLE const hParamUser,
    PARAMLIST_INSTANCE_HANDLE const hParamListCodec,
    PARAM_INSTANCE_HANDLE const hParamCodec) {
  int error = 0;
  (void)(ptr);
  (void)(hParamListUser);
  (void)(hParamListCodec);

  if (hParamUser && hParamCodec) {
    if (hParamUser->paramTag == hParamCodec->paramTag) {
      int unEqual = 0;
      switch (hParamUser->paramFormat) {
        case PARAM_CHAR:
          if (hParamUser->paramValue._char != hParamCodec->paramValue._char) unEqual = 1;
          break;
        case PARAM_SHORT:
          if (hParamUser->paramValue._short != hParamCodec->paramValue._short) unEqual = 1;
          break;
        case PARAM_INT:
          if (hParamUser->paramValue._int != hParamCodec->paramValue._int) unEqual = 1;
          break;
        case PARAM_FLOAT:
          if (!checkFLOATBitExactness(&hParamUser->paramValue._float, &hParamCodec->paramValue._float)) unEqual = 1;
          break;
        case PARAM_DOUBLE:
          if (!checkDOUBLEBitExactness(&hParamUser->paramValue._double, &hParamCodec->paramValue._double)) unEqual = 1;
          break;
        case PARAM_VOID_POINTER:
          if (hParamUser->paramValue._pvoid != hParamCodec->paramValue._pvoid) unEqual = 1;
          break;
        case PARAM_CHAR_ARRAY:
        case PARAM_INT_ARRAY:
        case PARAM_SHORT_ARRAY:
        case PARAM_FLOAT_ARRAY:
        case PARAM_DOUBLE_ARRAY:
        default:
          break;
      }

      if (unEqual) {
        switch ((PARAMLIST_PARAMETER const)hParamUser->paramTag) {
          default:
            error = 1;
            break;
        }
      }
    }
  }

  return error;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValueInt(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    int const paramValue) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED;
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    retValue = IIS_xHEAACEnc_AddParam(hConfig, paramTag, IIS_XHEAACENC_PARAM_INT, (void const *)&paramValue, 1);
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValueFloat(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    float const paramValue) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED;
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    retValue = IIS_xHEAACEnc_AddParam(hConfig, paramTag, IIS_XHEAACENC_PARAM_FLOAT, (void const *)&paramValue, 1);
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamValuePointer(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    void const *const paramValue) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED;
  }
  if (paramValue == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }
  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    retValue = IIS_xHEAACEnc_AddParam(hConfig, paramTag, IIS_XHEAACENC_PARAM_VOID_POINTER, paramValue, 1);
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_AddParamArray(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const paramTag,
    IIS_XHEAACENC_PARAM_FORMAT const paramFormat,
    void const *const pParamValue,
    int const paramLength) {
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED;
  }

  if (pParamValue == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    retValue = IIS_xHEAACEnc_AddParam(hConfig, paramTag, paramFormat, pParamValue, paramLength);
  }

  return retValue;
}

IIS_XHEAACENC_RETURN_CODE IIS_XHEAACAPI IIS_xHEAACEnc_Config_DeleteParam(
    IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig,
    IIS_XHEAACENC_PARAMETER const param) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_RETURN_CODE retValue = IIS_XHEAACENC_NO_ERROR;

  if (hConfig == NULL) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_HANDLE;
  } else if (hConfig->isFinalized) {
    retValue = IIS_XHEAACENC_ERROR_CONFIG_ALREADY_FINALIZED;
  }

  if (retValue < IIS_XHEAACENC_ERROR_FIRST) {
    errorInfo = iisParamListRemoveParamTag(hConfig->hUserParamList, param);
  }

  if (errorInfo != noError) {
    retValue = IIS_XHEAACENC_ERROR_INVALID_PARAM;
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return retValue;
}

int IIS_xHEAACEnc_ParamExists(
    PARAMLIST_INSTANCE_HANDLE hParamList,
    int const paramTag) {
  int retValue = -1;

  if (hParamList != NULL) {
    retValue = iisParamListParamExists(hParamList, paramTag);
  }

  return retValue;
}

int IIS_xHEAACEnc_ParamGetValueInt(
    PARAMLIST_INSTANCE_HANDLE hParamList,
    int const paramTag) {
  int retValue = -1;
  PARAM_INSTANCE_HANDLE hParam = NULL;

  if (hParamList != NULL) {
    iisParamListGetParam(hParamList, paramTag, &hParam);
    if (hParam)
      retValue = hParam->paramValue._int;
  }

  return retValue;
}

int IIS_xHEAACEnc_Config_FinalizeCB(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr) {
  HANDLE_ERROR_INFO errorInfo = noError;
  IIS_XHEAACENC_PARAM_INFO_HANDLE hInfo = (IIS_XHEAACENC_PARAM_INFO_HANDLE)ptr;
  int i = 0;
  int skipCheck = 0;
  int error = 0;

  (void)hParamList;

  if (!hParam) {
    error = 1;
  }
  if (!ptr) {
    error = 1;
  }

  if (error == 0 && hInfo->returnError == IIS_XHEAACENC_NO_ERROR) {
    if (error == 0) {
      for (i = 0; i < hInfo->numberIgnoreParam; i++) {
        if (hParam->paramTag == hInfo->listIgnoreParam[i]) {
          skipCheck = 1;
        }
      }
    }

    if (error == 0 && skipCheck == 0) {
      switch (hParam->paramTag) {
        case IIS_XHEAACENC_PARAMETER_AOT:
          switch (hParam->paramValue._int) {
            case IIS_XHEAACENC_AOT_USAC:
              break;
            case IIS_XHEAACENC_AOT_INVALID:
            default:
              hParam->paramValue._int = IIS_XHEAACENC_AOT_INVALID;
              hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_AOT;
              break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_BITRATE:
          if ((hParam->paramValue._int < IIS_XHEAACENC_BITRATE_MIN) ||
              (hParam->paramValue._int > IIS_XHEAACENC_BITRATE_MAX)) {
            hParam->paramValue._int = -1;
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_BITRATE;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_BITRATEMODE:
          switch (hParam->paramValue._int) {
            case IIS_XHEAACENC_BITRATEMODE_CBR:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR0:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR1:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR2:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR3:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR4:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR5:
            case IIS_XHEAACENC_BITRATEMODE_AAC_VBR6:
              break;
            default:
              hParam->paramValue._int = IIS_XHEAACENC_BITRATEMODE_INVALID;
              hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_BITRATEMODE;
              break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_CHANNELCONFIG:
          switch (hParam->paramValue._int) {
            case IIS_XHEAACENC_CHANNELCONFIG_MONO:
            case IIS_XHEAACENC_CHANNELCONFIG_STEREO:
              break;
            default:
              hParam->paramValue._int = IIS_XHEAACENC_CHANNELCONFIG_INVALID;
              hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_CHANNELCONFIG;
              break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_INSAMPLERATE:
          if (hParam->paramValue._int <= 0) {
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_INSAMPLERATE;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT:
          switch (hParam->paramValue._int) {
            case IIS_XHEAACENC_TRANSPORTFORMAT_RAW:
            case IIS_XHEAACENC_TRANSPORTFORMAT_ADTS:
            case IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS:
              break;
            default:
              hParam->paramValue._int = IIS_XHEAACENC_TRANSPORTFORMAT_INVALID;
              hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_TRANSPORTFORMAT;
          }
          break;

        case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS:
          if (hParam->paramValue._int < -1) {
            hParam->paramValue._int = -1;
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_RAP_INTERVAL;
          }
          break;

        case IIS_XHEAACENC_PARAMETER_STREAMID:
          if (hParam->paramValue._int < 0 || hParam->paramValue._int > (1 << 16) - 1) {
            hParam->paramValue._int = -1;
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_STREAMID;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE:
          switch (hParam->paramValue._int) {
            case IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL:
            case IIS_XHEAACENC_RAP_OCCURRENCE_ON_DEMAND:
              break;
            case IIS_XHEAACENC_RAP_OCCURRENCE_INVALID:
            default:
              hParam->paramValue._int = IIS_XHEAACENC_RAP_OCCURRENCE_INVALID;
              hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_RAP_OCCURRENCE;
              break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA:
          if (hParam->paramValue._pvoid == NULL) {
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_LOUDNESS_DATA;
            break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL:
          if (hParam->paramValue._float > -10.0f || hParam->paramValue._float < -31.0f) {
            hParam->paramValue._float = 1.0f;
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_LIVELOUDNESSLEVEL;
            break;
          }
          break;
        case IIS_XHEAACENC_PARAMETER_LIVE_MODE:
          if (!isZeroOrOne(hParam->paramValue._int)) {
            hParam->paramValue._int = -1;
            hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_LIVEMODE;
            break;
          }
          break;

        case PARAMLIST_PARAMETER_INTERNAL_MEASURED_LOUDNESS:
          if (hParam->paramValue._float > 6.0f) {
            hParam->paramValue._float = 1.0f;
            hInfo->returnError = IIS_XHEAACENC_ERROR_LOUDNESS_MEASURED_OUT_OF_BOUND;
            break;
          }
          break;
        case PARAMLIST_PARAMETER_INTERNAL_MEASURED_ANCHOR_LOUDNESS:
          if (hParam->paramValue._float > 6.0f) {
            hParam->paramValue._float = 1.0f;
            hInfo->returnError = IIS_XHEAACENC_ERROR_LOUDNESS_MEASURED_OUT_OF_BOUND;
            break;
          }
          break;
        case PARAMLIST_PARAMETER_INTERNAL_MEASURED_SAMPLE_PEAK:
          if (hParam->paramValue._float > 0.0f) {
            hParam->paramValue._float = 1.0f;
            hInfo->returnError = IIS_XHEAACENC_ERROR_SAMPLE_PEAK_MEASURED_OUT_OF_BOUND;
            break;
          }
          break;
        case PARAMLIST_PARAMETER_INTERNAL_LOUDNESS_VERIFICATION:
          break;
        default:

          assert(0);
          break;
      }
      if (hInfo->returnError == IIS_XHEAACENC_ERROR_INTERNAL) {
        mapInternalDevAdvErrorString(hParam->paramTag, hInfo->messageCallback);
      }
    }
  }

  if ((errorInfo != noError) && (error == 0)) {
    error = 1;
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return error;
}

int IIS_xHEAACEnc_RemoveNotAllowedParamsCB(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr) {
  HANDLE_ERROR_INFO errorInfo = noError;
  int error = 0;
  IIS_XHEAACENC_PARAM_INFO_HANDLE hInfo = (IIS_XHEAACENC_PARAM_INFO_HANDLE)ptr;
  int removeParam = 1;

  if (error == 0) {
    if (hInfo == NULL) {
      error = 1;
    }
  }

  if (error == 0) {
    if (hParam->paramTag >= 0 &&
        hParam->paramTag <= IIS_XHEAACENC_PARAMETER_LAST) {
      removeParam = 0;
    }
  }

  if (error == 0) {
    if (hInfo->advancedLiveEncoding) {
      if (hParam->paramTag == IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL ||
          hParam->paramTag == IIS_XHEAACENC_PARAMETER_LIVE_MODE) {
        removeParam = 0;
      }
    }
  }

  if (error == 0) {
    switch (hParam->paramTag) {
      case IIS_XHEAACENC_PARAMETER_LIB_NAME:
      case IIS_XHEAACENC_PARAMETER_BITRATELIMIT:
      case IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX:
      case IIS_XHEAACENC_PARAMETER_PRIMING:
      case IIS_XHEAACENC_PARAMETER_OUTSAMPLERATE:
      case IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE:
      case IIS_XHEAACENC_PARAMETER_STANDARDDELAY:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_MAX:
      case IIS_XHEAACENC_PARAMETER_CODECDELAY:
      case IIS_XHEAACENC_PARAMETER_FRAMESAMPLES:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT:
      case IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT:
      case IIS_XHEAACENC_PARAMETER_PROFILE_LEVEL:
      case IIS_XHEAACENC_PARAMETER_MAXBITRATEPERSEGMENT:
        removeParam = 1;
        break;
    }
  }

  if (error == 0) {
    if (removeParam) {
      if (hInfo->returnError == IIS_XHEAACENC_NO_ERROR) {
        hInfo->returnError = IIS_XHEAACENC_ERROR_PARAM_UNSUPPORTED;
      }
      errorInfo = iisParamListRemoveParamTag(hParamList, hParam->paramTag);
      if (errorInfo != noError) {
        error = 1;
      }
    }
  }

  return error;
}

static void valueToString_AOT(PARAM_INSTANCE_HANDLE hParam, char *valueString) {
  const char *valueStr = NULL;

  if (hParam->paramTag != IIS_XHEAACENC_PARAMETER_AOT) {
    assert(0);
    valueStr = "";
  } else {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_AOT_USAC:
        valueStr = "(42)  -  USAC [xHE-AAC/USAC]";
        break;
      default:
        valueStr = "AOT_INVALID";
        break;
    }
    sprintf(valueString, "%s", valueStr);
  }
}

static void valueToString_BitrateMode(PARAM_INSTANCE_HANDLE hParam, char *valueString) {
  const char *valueStr = NULL;

  if (hParam->paramTag != IIS_XHEAACENC_PARAMETER_BITRATEMODE) {
    assert(0);
    valueStr = "";
  } else {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_BITRATEMODE_CBR:
        valueStr = "CBR (Constant Bitrate Mode)";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR0:
        valueStr = "Variable Bitrate (VBR) Mode 0";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR1:
        valueStr = "Variable Bitrate (VBR) Mode 1";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR2:
        valueStr = "Variable Bitrate (VBR) Mode 2";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR3:
        valueStr = "Variable Bitrate (VBR) Mode 3";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR4:
        valueStr = "Variable Bitrate Mode (VBR) 4";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR5:
        valueStr = "Variable Bitrate (VBR) Mode 5";
        break;

      case IIS_XHEAACENC_BITRATEMODE_AAC_VBR6:
        valueStr = "Variable Bitrate (VBR) Mode 6";
        break;
      default:
        valueStr = "BITRATEMODE_INVALID";
        break;
    }
    sprintf(valueString, "%s", valueStr);
  }
}

static void valueToString_TransportFormat(PARAM_INSTANCE_HANDLE hParam, char *valueString) {
  char *valueStr = NULL;

  if (hParam->paramTag != IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT) {
    assert(0);
    valueStr = "";
  } else {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_TRANSPORTFORMAT_RAW:
        valueStr = "Raw (Plain Access Units)";
        break;

      case IIS_XHEAACENC_TRANSPORTFORMAT_ADTS:
        valueStr = "Audio Data Transport Stream (ADTS) Format";
        break;

      case IIS_XHEAACENC_TRANSPORTFORMAT_LATMLOAS:
        valueStr = "Low Overhead Audio Transport Multiplex (LATM/LOAS)";
        break;
      default:
        valueStr = "TRANSPORTFORMAT_INVALID";
        break;
    }
    sprintf(valueString, "%s", valueStr);
  }
}

static void valueToString_RapOccurrence(PARAM_INSTANCE_HANDLE hParam, char *valueString) {
  char *valueStr = NULL;

  if (hParam->paramTag != IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE) {
    assert(0);
    valueStr = "";
  } else {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_RAP_OCCURRENCE_CONSTANT_INTERVAL:
        valueStr = "Constant Interval Random Access Points";
        break;

      case IIS_XHEAACENC_RAP_OCCURRENCE_ON_DEMAND:
        valueStr = "On Demand Random Access Points";
        break;

      default:
        valueStr = "RAP_INVALID";
        break;
    }
    sprintf(valueString, "%s", valueStr);
  }
}

static void valueToString_ChannelConfig(PARAM_INSTANCE_HANDLE hParam, char *valueString) {
  const char *valueStr = NULL;
  if (hParam->paramTag != IIS_XHEAACENC_PARAMETER_CHANNELCONFIG) {
    assert(0);
    valueStr = "";
  } else {
    switch (hParam->paramValue._int) {
      case IIS_XHEAACENC_CHANNELCONFIG_MONO:
        valueStr = "Mono";
        break;
      case IIS_XHEAACENC_CHANNELCONFIG_STEREO:
        valueStr = "Stereo";
        break;
      default:
        valueStr = "CHANNELCONFIG_INVALID";
        break;
    }
    sprintf(valueString, "%s ", valueStr);
  }
}

static void valueToString(PARAM_INSTANCE_HANDLE hParam, char *valueStr) {
  if (hParam) {
    switch (hParam->paramFormat) {
      case PARAM_CHAR: {
        char value;
        value = hParam->paramValue._char;
        sprintf(valueStr, "%c", value);
      } break;

      case PARAM_INT: {
        int value;
        value = hParam->paramValue._int;
        sprintf(valueStr, "%d", value);
      } break;

      case PARAM_DOUBLE: {
        double value;
        value = hParam->paramValue._double;
        sprintf(valueStr, "%f", value);
      } break;

      case PARAM_FLOAT: {
        float value;
        value = hParam->paramValue._float;
        sprintf(valueStr, "%f", value);
      } break;

      case PARAM_SHORT: {
        short value;
        value = hParam->paramValue._short;
        sprintf(valueStr, "%d", value);
      } break;

      case PARAM_CHAR_ARRAY: {
        int i;
        int length = 0;
        char *a = (char *)hParam->paramValue._pvoid;
        for (i = 0; i < hParam->paramLength; i++) {
          length += sprintf(valueStr + length, "%c ", a[i]);
        }
      } break;
      case PARAM_SHORT_ARRAY: {
        int i;
        int length = 0;
        short *a = (short *)hParam->paramValue._pvoid;
        for (i = 0; i < hParam->paramLength; i++) {
          length += sprintf(valueStr + length, "%d ", a[i]);
        }
      } break;
      case PARAM_INT_ARRAY: {
        int i;
        int length = 0;
        int *a = (int *)hParam->paramValue._pvoid;
        for (i = 0; i < hParam->paramLength; i++) {
          length += sprintf(valueStr + length, "%d ", a[i]);
        }
      } break;
      case PARAM_FLOAT_ARRAY: {
        int i;
        int length = 0;
        float *a = (float *)hParam->paramValue._pvoid;
        for (i = 0; i < hParam->paramLength; i++) {
          length += sprintf(valueStr + length, "%f ", a[i]);
        }
      } break;
      case PARAM_DOUBLE_ARRAY: {
        int i;
        int length = 0;
        double *a = (double *)hParam->paramValue._pvoid;
        for (i = 0; i < hParam->paramLength; i++) {
          length += sprintf(valueStr + length, "%f ", a[i]);
        }
      } break;
      default:
        break;
    }
  }
}

static void getParamString(PARAM_INSTANCE_HANDLE hParam, char *msg)

{
  char valueString[128] = {'\0'};
  const char *tagString = NULL;

  if (hParam) {
    switch (hParam->paramTag) {
      case IIS_XHEAACENC_PARAMETER_AOT:
        tagString = "Audio Object Type (AOT)";
        valueToString_AOT(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATE:
        tagString = "Bitrate";
        valueToString(hParam, valueString);
        sprintf(valueString + strlen(valueString), " %s", "bps");
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATEMODE:
        tagString = "Bitrate Mode";
        valueToString_BitrateMode(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_CHANNELCONFIG:
        tagString = "Channel Config";
        valueToString_ChannelConfig(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_INSAMPLERATE:
        tagString = "Input Sampling Rate";
        valueToString(hParam, valueString);
        sprintf(valueString + strlen(valueString), " %s", "Hz");
        break;
      case IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT:
        tagString = "Transport Format";
        valueToString_TransportFormat(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE:
        tagString = "RAP Occurrence";
        valueToString_RapOccurrence(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_STREAMID:
        tagString = "Stream ID";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_SAMPLES:
        tagString = "RAP Interval";
        valueToString(hParam, valueString);
        sprintf(valueString + strlen(valueString), " %s", "Samples");
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_MIN_INTERVAL_SAMPLES:
        tagString = "Minimum RAP Interval";
        valueToString(hParam, valueString);
        sprintf(valueString + strlen(valueString), " %s", "Samples");
        break;
      case IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX:
        tagString = "Max Bitreservoir Bits";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATELIMIT:
        tagString = "Bitrate Limit";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_PRIMING:
        tagString = "Priming";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_OUTSAMPLERATE:
        tagString = "Output Sampling Rate";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_MAX:
        tagString = "Samples Max";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE:
        tagString = "Minimum Output Buffer Size";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_CODECDELAY:
        tagString = "Codec Delay";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT:
        tagString = "Samples Next";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT:
        tagString = "Samples Left";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_STANDARDDELAY:
        tagString = "Standard Delay";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_FRAMESAMPLES:
        tagString = "Frame Samples";
        valueToString(hParam, valueString);
        sprintf(valueString + strlen(valueString), " %s", "Samples");
        break;
      case IIS_XHEAACENC_PARAMETER_PROFILE_LEVEL:
        tagString = "Profile Level";
        valueToString(hParam, valueString);
        break;
      case IIS_XHEAACENC_PARAMETER_MAXBITRATEPERSEGMENT:
        tagString = "Max Bitrate Per Segment";
        valueToString(hParam, valueString);
        break;
      default:
        break;
    }

    if (tagString && strcmp(valueString, "")) {
      int k = max(35, (int)strlen(tagString)) - (int)strlen(tagString);

      sprintf(msg, "    %s%*s:  %s\n", tagString, k, "", valueString);
    }
  }
}

int IIS_xHEAACEnc_PrintAPIConfigParamsCB(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr

) {
  int error = 0;
  char msg[256] = {'\0'};

  IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE hConfig = (IIS_XHEAACENC_CONFIG_INSTANCE_HANDLE)ptr;

  (void)hParamList;

  if (hParam) {
    getParamString(hParam, msg);
  }

  if (strcmp(msg, "")) {
    IIS_xHEAACEnc_Message(hConfig->messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_CONFIG, msg);
  }
  return error;
}

int IIS_xHEAACEnc_MapParamListToInternCB(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr) {
  HANDLE_ERROR_INFO errorInfo = noError;
  int error = 0;
  (void)(ptr);

  if (hParam) {
    switch (hParam->paramTag) {
      case IIS_XHEAACENC_PARAMETER_AOT:
        hParam->paramTag = PARAMLIST_PARAMETER_AOT;
        errorInfo = IIS_xHEAACEnc_MapAotToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATE:
        hParam->paramTag = PARAMLIST_PARAMETER_BITRATE;
        errorInfo = IIS_xHEAACEnc_MapBitrateToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATEMODE:
        hParam->paramTag = PARAMLIST_PARAMETER_BITRATEMODE;
        errorInfo = IIS_xHEAACEnc_MapBitrateModeToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_CHANNELCONFIG:
        hParam->paramTag = PARAMLIST_PARAMETER_CHANNELCONFIG;
        errorInfo = IIS_xHEAACEnc_MapChannelConfigToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_INSAMPLERATE:
        hParam->paramTag = PARAMLIST_PARAMETER_INSAMPLERATE;
        break;
      case IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT:
        hParam->paramTag = PARAMLIST_PARAMETER_TRANSPORTFORMAT;
        errorInfo = IIS_xHEAACEnc_MapTransportFormatToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS:
        hParam->paramTag = PARAMLIST_PARAMETER_RAP_INTERVAL;
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_SAMPLES:
        hParam->paramTag = PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES;
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_MIN_INTERVAL_SAMPLES:
        hParam->paramTag = PARAMLIST_PARAMETER_RAP_MIN_INTERVAL_SAMPLES;
        break;
      case IIS_XHEAACENC_PARAMETER_LIB_NAME:
        hParam->paramTag = PARAMLIST_PARAMETER_LIB_NAME;
        break;
      case IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX:
        hParam->paramTag = PARAMLIST_PARAMETER_BITRESERVOIRBITS_MAX;
        break;
      case IIS_XHEAACENC_PARAMETER_BITRATELIMIT:
        hParam->paramTag = PARAMLIST_PARAMETER_BITRATELIMIT;
        break;
      case IIS_XHEAACENC_PARAMETER_PRIMING:
        hParam->paramTag = PARAMLIST_PARAMETER_PRIMING;
        break;
      case IIS_XHEAACENC_PARAMETER_OUTSAMPLERATE:
        hParam->paramTag = PARAMLIST_PARAMETER_OUTSAMPLERATE;
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_MAX:
        hParam->paramTag = PARAMLIST_PARAMETER_SAMPLES_MAX;
        break;
      case IIS_XHEAACENC_PARAMETER_STREAMID:
        hParam->paramTag = PARAMLIST_PARAMETER_STREAMID;
        break;
      case IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE:
        hParam->paramTag = PARAMLIST_PARAMETER_RAP_OCCURRENCE;
        errorInfo = IIS_xHEAACEnc_MapRapOccurrenceToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE:
        hParam->paramTag = PARAMLIST_PARAMETER_MIN_OUTBUF_SIZE;
        break;
      case IIS_XHEAACENC_PARAMETER_STANDARDDELAY:
        hParam->paramTag = PARAMLIST_PARAMETER_STANDARDDELAY;
        break;

      case IIS_XHEAACENC_PARAMETER_FRAMESAMPLES:
        hParam->paramTag = PARAMLIST_PARAMETER_FRAMESAMPLES;
        errorInfo = IIS_xHEAACEnc_MapFrameSamplesToIntern(hParam);
        break;
      case IIS_XHEAACENC_PARAMETER_CODECDELAY:
        hParam->paramTag = PARAMLIST_PARAMETER_CODECDELAY;
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT:
        hParam->paramTag = PARAMLIST_PARAMETER_SAMPLES_NEXT;
        break;
      case IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT:
        hParam->paramTag = PARAMLIST_PARAMETER_SAMPLES_LEFT;
        break;
      case IIS_XHEAACENC_PARAMETER_MAXBITRATEPERSEGMENT:
        hParam->paramTag = PARAMLIST_PARAMETER_MAXBITRATEPERSEGMENT;
        break;
      case IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA:
        hParam->paramTag = PARAMLIST_PARAMETER_LOUDNESS_DATA;
        break;
      case IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL:
        hParam->paramTag = PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL;
        break;
      case IIS_XHEAACENC_PARAMETER_LIVE_MODE:
        hParam->paramTag = PARAMLIST_PARAMETER_LIVE_MODE;
        break;

      case IIS_XHEAACENC_PARAMETER_INVALID:
      default:
        if (hParamList && hParam) {
          errorInfo = iisParamListRemoveParamTag(hParamList, hParam->paramTag);
          error = 1;
        }
        break;
    }
  }

  if (errorInfo != noError) {
    error = 1;
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return error;
}

void IIS_xHEAACEnc_mapInternalParamListErrorToString(
    PARAMLIST_ERROR_CODE const paramListError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  char *pErrorString = NULL;

  switch (paramListError) {
    case PARAMLIST_ERROR_NEW_LIST:
      pErrorString = "Internal error :  PL1001";
      break;
    case PARAMLIST_ERROR_NEW_PARAM:
      pErrorString = "Internal error :  PL1002";
      break;
    case PARAMLIST_ERROR_DELETE:
      pErrorString = "Internal error :  PL1003";
      break;
    case PARAMLIST_ERROR_ITERATE:
      pErrorString = "Internal error :  PL1004";
      break;
    case PARAMLIST_ERROR_COMPARE:
      pErrorString = "Internal error :  PL1005";
      break;
    case PARAMLIST_ERROR_COPY:
      pErrorString = "Internal error :  PL1006";
      break;
    case PARAMLIST_ERROR_SEC:
      pErrorString = "Internal error :  PL1007";
      break;
    default:
      pErrorString = "Internal error :  unknown";
  }
  IIS_xHEAACEnc_Message(messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, pErrorString);
}

int IIS_xHEAACEnc_MapParamListToAPICB(
    PARAMLIST_INSTANCE_HANDLE const hParamList,
    PARAM_INSTANCE_HANDLE const hParam,
    void *ptr) {
  HANDLE_ERROR_INFO errorInfo = noError;
  int error = 0;
  (void)(ptr);

  if (hParam) {
    switch (hParam->paramTag) {
      case PARAMLIST_PARAMETER_AOT:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_AOT;
        errorInfo = IIS_xHEAACEnc_MapAotToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_BITRATE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_BITRATE;
        break;
      case PARAMLIST_PARAMETER_BITRATEMODE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_BITRATEMODE;
        errorInfo = IIS_xHEAACEnc_MapBitrateModeToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_CHANNELCONFIG:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_CHANNELCONFIG;
        errorInfo = IIS_xHEAACEnc_MapChannelConfigToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_INSAMPLERATE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_INSAMPLERATE;
        break;
      case PARAMLIST_PARAMETER_TRANSPORTFORMAT:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_TRANSPORTFORMAT;
        errorInfo = IIS_xHEAACEnc_MapTransportFormatToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_RAP_OCCURRENCE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_RAP_OCCURRENCE;
        errorInfo = IIS_xHEAACEnc_MapRapOccurrenceToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_STREAMID:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_STREAMID;
        break;
      case PARAMLIST_PARAMETER_RAP_INTERVAL:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_MS;
        break;
      case PARAMLIST_PARAMETER_RAP_INTERVAL_SAMPLES:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_RAP_INTERVAL_SAMPLES;
        break;
      case PARAMLIST_PARAMETER_RAP_MIN_INTERVAL_SAMPLES:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_RAP_MIN_INTERVAL_SAMPLES;
        break;
      case PARAMLIST_PARAMETER_LIB_NAME:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_LIB_NAME;
        break;
      case PARAMLIST_PARAMETER_BITRESERVOIRBITS_MAX:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_BITRESERVOIRBITS_MAX;
        break;
      case PARAMLIST_PARAMETER_BITRATELIMIT:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_BITRATELIMIT;
        break;

      case PARAMLIST_PARAMETER_PRIMING:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_PRIMING;
        break;

      case PARAMLIST_PARAMETER_SAMPLES_MAX:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_SAMPLES_MAX;
        break;
      case PARAMLIST_PARAMETER_MIN_OUTBUF_SIZE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_MIN_OUTBUF_SIZE;
        break;
      case PARAMLIST_PARAMETER_CODECDELAY:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_CODECDELAY;
        break;
      case PARAMLIST_PARAMETER_SAMPLES_NEXT:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_SAMPLES_NEXT;
        break;
      case PARAMLIST_PARAMETER_SAMPLES_LEFT:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_SAMPLES_LEFT;
        break;
      case PARAMLIST_PARAMETER_STANDARDDELAY:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_STANDARDDELAY;
        break;
      case PARAMLIST_PARAMETER_FRAMESAMPLES:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_FRAMESAMPLES;
        errorInfo = IIS_xHEAACEnc_MapFrameSamplesToAPI(hParam);
        break;
      case PARAMLIST_PARAMETER_LOUDNESS_DATA:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_LOUDNESS_DATA;
        break;
      case PARAMLIST_PARAMETER_LIVE_LOUDNESS_LEVEL:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_LIVE_LOUDNESS_LEVEL;
        break;
      case PARAMLIST_PARAMETER_LIVE_MODE:
        hParam->paramTag = IIS_XHEAACENC_PARAMETER_LIVE_MODE;
        break;

      default:
        if (hParamList && hParam)
          errorInfo = iisParamListRemoveParamTag(hParamList, hParam->paramTag);
        break;
    }
  }

  if (errorInfo != noError) {
    error = 1;
  }

  if (errorInfo) freeErrorTraceback(errorInfo);
  return error;
}

static void mapInternalDevAdvErrorString(
    int paramTag,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  char *pErrorString = NULL;

  switch (paramTag) {
    default:
      pErrorString = "Internal error :  unknown";
  }
  IIS_xHEAACEnc_Message(messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, pErrorString);
}

void mapInternalParamErrorToString(
    PARAMLIST_ERROR_CODE const paramListError,
    IIS_XHEAACENC_MESSAGE_CALLBACK messageCallback) {
  char *pErrorString = NULL;

  switch (paramListError) {
    case PARAMLIST_ERROR_NEW_LIST:
      pErrorString = "Internal error :  P1001";
      break;
    case PARAMLIST_ERROR_NEW_PARAM:
      pErrorString = "Internal error :  P1002";
      break;
    case PARAMLIST_ERROR_DELETE:
      pErrorString = "Internal error :  P1003";
      break;
    case PARAMLIST_ERROR_ITERATE:
      pErrorString = "Internal error :  P1004";
      break;
    case PARAMLIST_ERROR_COMPARE:
      pErrorString = "Internal error :  P1005";
      break;
    case PARAMLIST_ERROR_COPY:
      pErrorString = "Internal error :  P1006";
      break;
    case PARAMLIST_ERROR_SEC:
      pErrorString = "Internal error :  P1007";
      break;
    default:
      pErrorString = "Internal error :  unknown";
  }
  IIS_xHEAACEnc_Message(messageCallback, IIS_XHEAACENC_MESSAGE_LEVEL_ERROR, pErrorString);
}

static int isInSampleRateSupported(
    int const inSampleRate) {
  int isSupported = 0;

  int allowedInSampleRates[] = {8000, 11025, 12000, 16000, 22050, 24000, 29400, 32000, 38400, 44100, 48000, 88200, 96000};
  int arrLen = sizeof(allowedInSampleRates) / sizeof(allowedInSampleRates[0]);

  for (int i = 0; i < arrLen; i++) {
    if (allowedInSampleRates[i] == inSampleRate) {
      isSupported = 1;
      break;
    }
  }

  return isSupported;
}

static int isZeroOrOne(
    int flag) {
  int returnValue = 1;

  if (flag != 0 && flag != 1) {
    returnValue = 0;
  }

  return returnValue;
}
