
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

#define PARAMLIST_VERSION_NUMBER "03.06.03"
#define PARAMLIST_MODULE_NAME "iisParamList"

#define PARAMLIST_BUILD_DATE __DATE__
#define PARAMLIST_BUILD_INFO "Release build"

#ifdef __GNUC__
#define PARAMLIST_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
#define PARAMLIST_COMPILER_INFO "Compiler: GCC"
#else
#ifdef _MSC_VER
#define PARAMLIST_COMPILER_VERSION _MSC_VER
#define PARAMLIST_COMPILER_INFO "Compiler: Visual C"
#else
#define PARAMLIST_COMPILER_VERSION 0
#define PARAMLIST_COMPILER_INFO "Compiler: unknown"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iisutillib.h"
#include "iisLinkedList.h"
#include "iisParamList.h"

#define PARAMLIST_PRIVATE_HANDLE_OFFSET_BYTES (sizeof(void *) - sizeof(struct paramlist_instance_struct) % sizeof(void *))

typedef struct paramlist_private_data_struct {
  int nSize;
  LINKEDLIST_INSTANCE_HANDLE hLinkedList;
  int iterating;
} PARAMLIST_PRIVATE_DATA, *PARAMLIST_PRIVATE_DATA_HANDLE;

static PARAMLIST_PRIVATE_DATA_HANDLE iisParamListGetPrivateDataHandle(
    PARAMLIST_INSTANCE_HANDLE const hInstance);

static PARAMLIST_PRIVATE_DATA_HANDLE iisParamListGetPrivateDataHandle(
    PARAMLIST_INSTANCE_HANDLE const hInstance) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (PARAMLIST_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct paramlist_instance_struct) + PARAMLIST_PRIVATE_HANDLE_OFFSET_BYTES);
  return hPrivateData;
}

HANDLE_ERROR_INFO iisParamListNew(
    PARAMLIST_INSTANCE_HANDLE *phInstance) {
  HANDLE_ERROR_INFO retError = noError;
  int nSize = 0;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  nSize += sizeof(struct paramlist_instance_struct);
  nSize += PARAMLIST_PRIVATE_HANDLE_OFFSET_BYTES;
  nSize += sizeof(struct paramlist_private_data_struct);

  *phInstance = (PARAMLIST_INSTANCE_HANDLE)iisMalloc(nSize);

  if (*phInstance != NULL) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (PARAMLIST_PRIVATE_DATA_HANDLE)((char *)(*phInstance) + sizeof(struct paramlist_instance_struct) + PARAMLIST_PRIVATE_HANDLE_OFFSET_BYTES);
    hPrivateData->nSize = nSize;
    sprintf((*phInstance)->infoModuleVersion, "%s %s", PARAMLIST_MODULE_NAME, PARAMLIST_VERSION_NUMBER);
    hPrivateData->iterating = 0;
    if (retError == noError) {
      retError = iisLinkedListNew(&hPrivateData->hLinkedList);
    }
  } else {
    retError = iisUtil_ERROR(CDI, "Memory allocation in function iisParamListNew failed");
  }

  return retError;
}

HANDLE_ERROR_INFO iisParamListDelete(
    PARAMLIST_INSTANCE_HANDLE hInstance) {
  HANDLE_ERROR_INFO retError = noError;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  PARAM_INSTANCE_HANDLE hParam = NULL;

  if (hInstance != NULL) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);

    if (hPrivateData->hLinkedList != NULL) {
      iisLinkedListStartIteration(hPrivateData->hLinkedList);
      while (!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList)) {
        hParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);
        if (hParam) iisParamDelete(hParam);
      }
      iisLinkedListStopIteration(hPrivateData->hLinkedList);
      iisLinkedListRemoveAllPtr(hPrivateData->hLinkedList);
      iisLinkedListDelete(hPrivateData->hLinkedList);
    }

    iisFree(hInstance);

  } else {
    retError = iisUtil_ERROR(CDI, "Deleting instance in function iisParamListDelete failed");
  }
  return retError;
}

HANDLE_ERROR_INFO iisParamListAddParam(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAM_INSTANCE_HANDLE const hParam,
    PARAMLIST_MODE const mode) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  HANDLE_ERROR_INFO error = noError;
  PARAM_INSTANCE_HANDLE hParamCopy = NULL;

  if ((hInstance == NULL) ||
      (hParam == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid handle!");
  }

  if (error == noError) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);

    if (hPrivateData->iterating > 0) {
      error = iisUtil_ERROR(CDI, "Nested calls of iisParamListIterate in callback forbidden!");
    }
  }

  if (error == noError) {
    if (mode == PARAMLIST_MODE_REPLACE && iisParamListParamExists(hInstance, hParam->paramTag)) {
      if (hPrivateData->hLinkedList != NULL) {
        hPrivateData->iterating = 1;
        iisLinkedListStartIteration(hPrivateData->hLinkedList);
        while ((!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList)) &&
               (hPrivateData->iterating > 0) &&
               (error == noError)) {
          PARAM_INSTANCE_HANDLE hListParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);
          if (hListParam->paramTag == hParam->paramTag) {
            hListParam->paramValue = hParam->paramValue;
          }
        }
        iisLinkedListStopIteration(hPrivateData->hLinkedList);
        hPrivateData->iterating = 0;
      }
    } else if (mode != PARAMLIST_MODE_APPENDIFMISSING || iisParamListParamExists(hInstance, hParam->paramTag) == 0) {
      if (error == noError) {
        hParamCopy = iisParamCopy(hParam);
      }

      if (error == noError) {
        if (!iisLinkedListAddPtr(hPrivateData->hLinkedList, hParamCopy, LINKEDLIST_ADD_MODE_AT_END)) {
          error = iisUtil_ERROR(CDI, "Can not add parameter in iisParamListAddParam!");
        }
      }
      if (error == noError) {
        hInstance->storedElementsCount++;
      }
    }
  }

  if (error != noError) {
    if (hParamCopy) iisParamDelete(hParamCopy);
  }

  return error;
}

HANDLE_ERROR_INFO iisParamListAddParamValue(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_PARAMETER const paramTag,
    PARAM_FORMAT const paramFormat,
    void *pValue,
    PARAMLIST_MODE const mode) {
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE param = {0};

  if (hInstance == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (errorInfo == noError) {
    switch (paramFormat) {
      case PARAM_CHAR_ARRAY:
      case PARAM_SHORT_ARRAY:
      case PARAM_INT_ARRAY:
      case PARAM_FLOAT_ARRAY:
      case PARAM_DOUBLE_ARRAY:
        errorInfo = iisUtil_ERROR(CDI, "*_ARRAY seems to be a function mismatch. Try iisParamListAddParamArray()");
        break;
      case PARAM_CHAR:
        param.paramValue._char = *((char *)pValue);
        break;
      case PARAM_SHORT:
        param.paramValue._short = *((short *)pValue);
        break;
      case PARAM_INT:
        param.paramValue._int = *((int *)pValue);
        break;
      case PARAM_FLOAT:
        param.paramValue._float = *((float *)pValue);
        break;
      case PARAM_DOUBLE:
        param.paramValue._double = *((double *)pValue);
        break;
      case PARAM_VOID_POINTER:
        param.paramValue._pvoid = pValue;
        break;
      default:
        break;
    }
  }

  if (errorInfo == noError) {
    param.paramFormat = paramFormat;
    param.paramLength = 1;
    param.paramTag = paramTag;

    errorInfo = iisParamListAddParam(hInstance, &param, mode);
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListAddParamArray(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_PARAMETER const paramTag,
    PARAM_FORMAT const paramFormat,
    void *pValue,
    int length,
    PARAMLIST_MODE const mode) {
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE param = {0};

  if (hInstance == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (errorInfo == noError) {
    switch (paramFormat) {
      case PARAM_CHAR_ARRAY:
      case PARAM_SHORT_ARRAY:
      case PARAM_INT_ARRAY:
      case PARAM_FLOAT_ARRAY:
      case PARAM_DOUBLE_ARRAY:
        break;
      case PARAM_CHAR:
      case PARAM_SHORT:
      case PARAM_INT:
      case PARAM_FLOAT:
      case PARAM_DOUBLE:
      case PARAM_VOID_POINTER:
        if (length > 1) {
          errorInfo = iisUtil_ERROR(CDI, "Wrong paramFormat selected");
        }
        break;
      default:
        break;
    }
  }

  if (errorInfo == noError) {
    param.paramFormat = paramFormat;
    param.paramLength = length;
    param.paramTag = paramTag;
    param.paramValue._pvoid = pValue;

    errorInfo = iisParamListAddParam(hInstance, &param, mode);
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListAddParamValueInt(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_PARAMETER const paramTag,
    int const valueINT,
    PARAMLIST_MODE const mode) {
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE param = {0};

  if (hInstance == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (errorInfo == noError) {
    param.paramFormat = PARAM_INT;
    param.paramLength = 1;
    param.paramTag = paramTag;
    param.paramValue._int = valueINT;

    errorInfo = iisParamListAddParam(hInstance, &param, mode);
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListAddParamValueFloat(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_PARAMETER const paramTag,
    float const valueFLOAT,
    PARAMLIST_MODE const mode) {
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE param = {0};

  if (hInstance == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (errorInfo == noError) {
    param.paramFormat = PARAM_FLOAT;
    param.paramLength = 1;
    param.paramTag = paramTag;
    param.paramValue._float = valueFLOAT;

    errorInfo = iisParamListAddParam(hInstance, &param, mode);
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListAddParamValuePointer(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_PARAMETER const paramTag,
    void *valuePtr,
    PARAMLIST_MODE const mode) {
  HANDLE_ERROR_INFO errorInfo = noError;
  PARAM_INSTANCE param = {0};

  if (hInstance == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (valuePtr == NULL) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid pointer parameter");
  }

  if (errorInfo == noError) {
    param.paramFormat = PARAM_VOID_POINTER;
    param.paramLength = 1;
    param.paramTag = paramTag;
    param.paramValue._pvoid = valuePtr;

    errorInfo = iisParamListAddParam(hInstance, &param, mode);
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListOverwrite(
    PARAMLIST_INSTANCE_HANDLE *phDest,
    PARAMLIST_INSTANCE_HANDLE const hSrc) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateDataSrc = NULL;
  HANDLE_ERROR_INFO error = noError;
  PARAM_INSTANCE_HANDLE hParam = NULL;

  if (hSrc == NULL || phDest == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (error == noError) {
    if (*phDest == NULL) {
      error = iisParamListNew(phDest);
      if (error == noError && *phDest == NULL) {
        error = iisUtil_ERROR(CDI, "Error while creating destination param list");
      }
    }
  }

  if (error == noError) {
    hPrivateDataSrc = iisParamListGetPrivateDataHandle(hSrc);
  }

  if (error == noError) {
    if (hPrivateDataSrc->iterating > 0) {
      error = iisUtil_ERROR(CDI, "Nested calls of iisParamListIterate in callback forbidden!");
    } else {
      if (hPrivateDataSrc->hLinkedList != NULL) {
        hPrivateDataSrc->iterating = 1;
        iisLinkedListStartIteration(hPrivateDataSrc->hLinkedList);
        while ((!iisLinkedListIsEndOfIterate(hPrivateDataSrc->hLinkedList)) &&
               (hPrivateDataSrc->iterating > 0) &&
               (error == noError)) {
          hParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateDataSrc->hLinkedList);
          if (hParam) {
            error = iisParamListAddParam(*phDest, hParam, PARAMLIST_MODE_REPLACE);
          }
        }

        iisLinkedListStopIteration(hPrivateDataSrc->hLinkedList);
        hPrivateDataSrc->iterating = 0;
      }
    }
  }

  return error;
}

PARAMLIST_INSTANCE_HANDLE iisParamListCopy(
    PARAMLIST_INSTANCE_HANDLE const hInstance) {
  PARAMLIST_INSTANCE_HANDLE hInstanceCopy = NULL;
  HANDLE_ERROR_INFO error = noError;
  error = iisParamListOverwrite(&hInstanceCopy, hInstance);
  if (error != noError) {
    if (hInstanceCopy != NULL) {
      iisParamListDelete(hInstanceCopy);
      hInstanceCopy = NULL;
    }
  }
  return hInstanceCopy;
}

HANDLE_ERROR_INFO iisParamListPrint(
    PARAMLIST_INSTANCE_HANDLE const hInstance) {
  HANDLE_ERROR_INFO errorInfo = noError;

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListCompare(
    PARAMLIST_INSTANCE_HANDLE const hParamList1,
    PARAMLIST_INSTANCE_HANDLE const hParamList2,
    PARAMLIST_CALLBACK_COMPARE const callbackFunc,
    void *ptr) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData1 = NULL;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData2 = NULL;
  PARAM_INSTANCE_HANDLE hParam1 = NULL;
  PARAM_INSTANCE_HANDLE hParam2 = NULL;
  HANDLE_ERROR_INFO errorInfo = noError;
  int callbackFuncReturn;

  if ((hParamList1 == NULL) ||
      (hParamList2 == NULL) ||
      (callbackFunc == NULL)) {
    errorInfo = iisUtil_ERROR(CDI, "Handle error!");
  }

  if (errorInfo == noError) {
    hPrivateData1 = iisParamListGetPrivateDataHandle(hParamList1);
    hPrivateData2 = iisParamListGetPrivateDataHandle(hParamList2);

    if ((hPrivateData1->iterating > 0) ||
        (hPrivateData2->iterating > 0)) {
      errorInfo = iisUtil_ERROR(CDI, "\nNested calls of iisParamListIterate in callback forbidden!");
    } else {
      if ((hPrivateData1->hLinkedList != NULL) &&
          (hPrivateData2->hLinkedList != NULL)) {
        hPrivateData1->iterating = 1;
        iisLinkedListStartIteration(hPrivateData1->hLinkedList);
        while ((!iisLinkedListIsEndOfIterate(hPrivateData1->hLinkedList)) &&
               (hPrivateData1->iterating > 0)) {
          hParam1 = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData1->hLinkedList);

          hPrivateData2->iterating = 1;
          iisLinkedListStartIteration(hPrivateData2->hLinkedList);
          while ((!iisLinkedListIsEndOfIterate(hPrivateData2->hLinkedList)) &&
                 (hPrivateData2->iterating > 0) &&
                 errorInfo == noError) {
            hParam2 = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData2->hLinkedList);
            if (hParam1 && hParam2) {
              callbackFuncReturn = callbackFunc(ptr, hParamList1, hParam1, hParamList2, hParam2);

              if (callbackFuncReturn != 0) {
                errorInfo = iisUtil_ERROR(CDI, "Error triggered by callback function!");
              }
            }
          }
          iisLinkedListStopIteration(hPrivateData2->hLinkedList);
          hPrivateData2->iterating = 0;
        }

        iisLinkedListStopIteration(hPrivateData1->hLinkedList);
        hPrivateData1->iterating = 0;
      }
    }
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListCopyParam(
    PARAMLIST_INSTANCE_HANDLE dest,
    PARAMLIST_INSTANCE_HANDLE const src,
    PARAMLIST_PARAMETER paramTag) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateDataSrc = NULL;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateDataDest = NULL;
  PARAM_INSTANCE_HANDLE hParam = NULL;
  HANDLE_ERROR_INFO errorInfo = noError;

  if ((dest == NULL) ||
      (src == NULL)) {
    errorInfo = iisUtil_ERROR(CDI, "Handle error!");
  }

  if (errorInfo == noError) {
    hPrivateDataSrc = iisParamListGetPrivateDataHandle(src);
    hPrivateDataDest = iisParamListGetPrivateDataHandle(dest);

    if ((hPrivateDataSrc->iterating > 0) ||
        (hPrivateDataDest->iterating > 0)) {
      errorInfo = iisUtil_ERROR(CDI, "\nNested calls of iisParamListIterate in callback forbidden!");
    } else {
      if ((hPrivateDataSrc->hLinkedList != NULL) &&
          (hPrivateDataDest->hLinkedList != NULL)) {
        hPrivateDataSrc->iterating = 1;
        iisLinkedListStartIteration(hPrivateDataSrc->hLinkedList);
        while ((!iisLinkedListIsEndOfIterate(hPrivateDataSrc->hLinkedList)) &&
               (hPrivateDataSrc->iterating > 0)) {
          hParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateDataSrc->hLinkedList);
          if (hParam->paramTag == (int)paramTag) {
            iisParamListAddParam(dest, hParam, PARAMLIST_MODE_REPLACE);
            break;
          }
        }

        iisLinkedListStopIteration(hPrivateDataSrc->hLinkedList);
        hPrivateDataSrc->iterating = 0;
      }
    }
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListCopyUnique(
    PARAMLIST_INSTANCE_HANDLE dest,
    PARAMLIST_INSTANCE_HANDLE const src) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateDataSrc = NULL;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateDataDest = NULL;
  PARAM_INSTANCE_HANDLE hParamSrc = NULL;
  PARAM_INSTANCE_HANDLE hParamDest = NULL;
  HANDLE_ERROR_INFO errorInfo = noError;
  int foundFlag = 0;

  if ((dest == NULL) || (src == NULL)) {
    errorInfo = iisUtil_ERROR(CDI, "Handle error!");
  }

  if (errorInfo == noError) {
    hPrivateDataSrc = iisParamListGetPrivateDataHandle(src);
    hPrivateDataDest = iisParamListGetPrivateDataHandle(dest);

    if ((hPrivateDataSrc->iterating > 0) || (hPrivateDataDest->iterating > 0)) {
      errorInfo = iisUtil_ERROR(CDI, "\nNested calls of iisParamListIterate in callback forbidden!");
    } else {
      if ((hPrivateDataSrc->hLinkedList != NULL) && (hPrivateDataDest->hLinkedList != NULL)) {
        hPrivateDataSrc->iterating = 1;
        iisLinkedListStartIteration(hPrivateDataSrc->hLinkedList);

        while ((!iisLinkedListIsEndOfIterate(hPrivateDataSrc->hLinkedList)) && (hPrivateDataSrc->iterating > 0)) {
          hParamSrc = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateDataSrc->hLinkedList);

          hPrivateDataDest->iterating = 1;
          iisLinkedListStartIteration(hPrivateDataDest->hLinkedList);
          foundFlag = 0;

          while ((!iisLinkedListIsEndOfIterate(hPrivateDataDest->hLinkedList)) && (hPrivateDataDest->iterating > 0)) {
            hParamDest = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateDataDest->hLinkedList);

            if (hParamSrc->paramTag == hParamDest->paramTag) {
              foundFlag = 1;
            }
          }

          iisLinkedListStopIteration(hPrivateDataDest->hLinkedList);
          hPrivateDataDest->iterating = 0;

          if (foundFlag == 0) {
            iisParamListAddParam(dest, hParamSrc, PARAMLIST_MODE_REPLACE);
          }
        }
        iisLinkedListStopIteration(hPrivateDataSrc->hLinkedList);
        hPrivateDataSrc->iterating = 0;
      }
    }
  }

  return errorInfo;
}

HANDLE_ERROR_INFO iisParamListGetParam(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    int const paramTag,
    PARAM_INSTANCE_HANDLE *phParam) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  HANDLE_ERROR_INFO error = noError;
  PARAM_INSTANCE_HANDLE storedParam;

  if (hInstance == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle!");
  }

  if (error == noError) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);
  }

  if (error == noError) {
    iisLinkedListStartIteration(hPrivateData->hLinkedList);
    while (!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList) && (error == noError)) {
      storedParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);

      if (storedParam->paramTag == paramTag) {
        switch (storedParam->paramFormat) {
          case PARAM_CHAR_ARRAY:
            if (storedParam->paramLength < 0) {
              error = iisUtil_ERROR(CDI, "Parameter length and format of stored parameter do not fit!");
            }
            break;
          case PARAM_SHORT_ARRAY:
            if (storedParam->paramLength < 0) {
              error = iisUtil_ERROR(CDI, "Parameter length and format of stored parameter do not fit!");
            }
            break;
          case PARAM_INT_ARRAY:
            if (storedParam->paramLength < 0) {
              error = iisUtil_ERROR(CDI, "Parameter length and format of stored parameter do not fit!");
            }
            break;
          case PARAM_FLOAT_ARRAY:
            if (storedParam->paramLength < 0) {
              error = iisUtil_ERROR(CDI, "Parameter length and format of stored parameter do not fit!");
            }
            break;
          case PARAM_DOUBLE_ARRAY:
            if (storedParam->paramLength < 0) {
              error = iisUtil_ERROR(CDI, "Parameter length and format of stored parameter do not fit!");
            }
            break;
          case PARAM_CHAR:
          case PARAM_SHORT:
          case PARAM_INT:
          case PARAM_FLOAT:
          case PARAM_DOUBLE:
          case PARAM_VOID_POINTER:
            break;
          default:
            break;
        }

        *phParam = storedParam;

        break;
      }
    }
    iisLinkedListStopIteration(hPrivateData->hLinkedList);
  }

  return error;
}

int iisParamListParamExists(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    int const paramTag) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData;
  PARAM_INSTANCE_HANDLE hParam = NULL;
  int retVal = 0;

  if (hInstance == NULL) {
    retVal = -1;
  }

  if (retVal >= 0) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);

    iisLinkedListStartIteration(hPrivateData->hLinkedList);
    while (!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList)) {
      hParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);

      if (hParam->paramTag == paramTag) {
        retVal = 1;
        break;
      }
    }
    iisLinkedListStopIteration(hPrivateData->hLinkedList);
  }

  return retVal;
}

HANDLE_ERROR_INFO iisParamListIterate(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    PARAMLIST_CALLBACK_TYPE const callbackFunc,
    void *ptr) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData;
  HANDLE_ERROR_INFO error = noError;
  PARAM_INSTANCE_HANDLE hParam = NULL;
  int callbackReturn = 0;

  if ((hInstance == NULL) ||
      (callbackFunc == NULL)) {
    error = iisUtil_ERROR(CDI, "Invalid handle!");
  }

  if (error == noError) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);

    if (hPrivateData->iterating > 0) {
      error = iisUtil_ERROR(CDI, "Nested calls of iisParamListIterate in callback forbidden!");
    } else {
      if (hPrivateData->hLinkedList != NULL) {
        hPrivateData->iterating = 1;
        iisLinkedListStartIteration(hPrivateData->hLinkedList);
        while ((!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList)) &&
               (hPrivateData->iterating > 0)) {
          hParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);
          if (hParam) {
            callbackReturn = callbackFunc(hInstance, hParam, ptr);
          }
          if (callbackReturn != 0) {
            break;
          }
        }

        iisLinkedListStopIteration(hPrivateData->hLinkedList);
        hPrivateData->iterating = 0;
      }
    }
  }

  if (callbackReturn != 0) {
    error = iisUtil_ERROR(CDI, "Iteration Callback returned with non zero");
  }

  return error;
}

HANDLE_ERROR_INFO iisParamListStopIterate(
    PARAMLIST_INSTANCE_HANDLE const hInstance) {
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData;
  HANDLE_ERROR_INFO error = noError;

  if (hInstance == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (error == noError) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);
    hPrivateData->iterating = 0;
  }

  return error;
}

HANDLE_ERROR_INFO iisParamListRemoveParamTag(
    PARAMLIST_INSTANCE_HANDLE const hInstance,
    int const paramTag) {
  HANDLE_ERROR_INFO error = noError;
  PARAMLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  PARAM_INSTANCE_HANDLE storedParam = NULL;
  int found = 0;

  if (hInstance == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid handle");
  }

  if (error == noError) {
    hPrivateData = iisParamListGetPrivateDataHandle(hInstance);
  }

  if (error == noError) {
    if (hPrivateData->iterating == 0) {
      iisLinkedListStartIteration(hPrivateData->hLinkedList);
      while (!iisLinkedListIsEndOfIterate(hPrivateData->hLinkedList) && (error == noError)) {
        storedParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterate(hPrivateData->hLinkedList);

        if (storedParam) {
          switch (storedParam->paramFormat) {
            case PARAM_CHAR_ARRAY:
            case PARAM_SHORT_ARRAY:
            case PARAM_INT_ARRAY:
            case PARAM_FLOAT_ARRAY:
            case PARAM_DOUBLE_ARRAY:
              if (storedParam->paramLength < 1) {
                error = iisUtil_ERROR(CDI, "Invalid paramLength");
              }
              break;
            case PARAM_CHAR:
            case PARAM_SHORT:
            case PARAM_INT:
            case PARAM_FLOAT:
            case PARAM_DOUBLE:
            case PARAM_VOID_POINTER:
            default:
              break;
          }
          if (storedParam->paramTag == paramTag) {
            found = 1;
            break;
          }
        }
      }
      iisLinkedListStopIteration(hPrivateData->hLinkedList);

      if (found == 1) {
        if (iisLinkedListRemovePtr(hPrivateData->hLinkedList, storedParam)) {
          hInstance->storedElementsCount--;
          iisParamDelete(storedParam);
        }
      }
    } else {
      storedParam = (PARAM_INSTANCE_HANDLE)iisLinkedListIterateRemoveCurrentElement(hPrivateData->hLinkedList);
      if (storedParam) {
        hInstance->storedElementsCount--;
        if (storedParam->paramLength > 1) {
          if (storedParam->paramValue._pvoid) {
            iisFree(storedParam->paramValue._pvoid);
            storedParam->paramValue._pvoid = NULL;
          }
        }
        iisParamDelete(storedParam);
      }
    }
  }

  return error;
}
