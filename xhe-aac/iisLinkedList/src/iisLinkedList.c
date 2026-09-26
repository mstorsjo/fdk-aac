
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

#define LINKEDLIST_VERSION_NUMBER "01.00.15"
#define LINKEDLIST_MODULE_NAME "iisLinkedList"
#define LINKEDLIST_BUILD_DATE __DATE__
#define LINKEDLIST_BUILD_INFO "Release build"

#ifdef __GNUC__
#define LINKEDLIST_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
#define LINKEDLIST_COMPILER_INFO "Compiler: GCC"
#else
#ifdef _MSC_VER
#define LINKEDLIST_COMPILER_VERSION _MSC_VER
#define LINKEDLIST_COMPILER_INFO "Compiler: Visual C"
#else
#define LINKEDLIST_COMPILER_VERSION 0
#define LINKEDLIST_COMPILER_INFO "Compiler: unknown"
#endif
#endif

#define LINKEDLIST_PRIVATE_DATA_OFFSET_BYTES (sizeof(void *) - sizeof(struct linkedlist_instance_struct) % sizeof(void *))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iisutillib.h"
#include "iisLinkedList.h"

typedef struct linkedlist_linear_data_struct {
  struct linkedlist_linear_data_struct *hNext;
  void *ptr;
} LINKEDLIST_LINEAR_DATA_LIST, *LINKEDLIST_LINEAR_DATA_LIST_HANDLE;

typedef struct linkedlist_private_data_struct {
  int nSize;
  int iteration;

  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hFirst;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hIterationPrev;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hIteration;

} LINKEDLIST_PRIVATE_DATA, *LINKEDLIST_PRIVATE_DATA_HANDLE;

static LINKEDLIST_PRIVATE_DATA_HANDLE iisLinkedListGetPrivateDataHandle(
    LINKEDLIST_INSTANCE_HANDLE const hInstance);

static LINKEDLIST_PRIVATE_DATA_HANDLE iisLinkedListGetPrivateDataHandle(
    LINKEDLIST_INSTANCE_HANDLE const hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (LINKEDLIST_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct linkedlist_instance_struct) + LINKEDLIST_PRIVATE_DATA_OFFSET_BYTES);
  return hPrivateData;
}

HANDLE_ERROR_INFO iisLinkedListNew(
    LINKEDLIST_INSTANCE_HANDLE *phInstance) {
  HANDLE_ERROR_INFO retError = noError;
  int nSize = 0;
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  nSize += sizeof(struct linkedlist_instance_struct);
  nSize += sizeof(struct linkedlist_private_data_struct);
  nSize += LINKEDLIST_PRIVATE_DATA_OFFSET_BYTES;
  *phInstance = (LINKEDLIST_INSTANCE_HANDLE)iisMalloc(nSize);

  if (*phInstance != NULL) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (LINKEDLIST_PRIVATE_DATA_HANDLE)((char *)(*phInstance) + sizeof(struct linkedlist_instance_struct) + LINKEDLIST_PRIVATE_DATA_OFFSET_BYTES);
    hPrivateData->nSize = nSize;

    sprintf((*phInstance)->infoModuleVersion, "%s %s", LINKEDLIST_MODULE_NAME, LINKEDLIST_VERSION_NUMBER);

  } else {
    retError = iisUtil_ERROR(CDI, "Memory allocation in function iisLinkedListOpen failed");
  }
  return retError;
}

HANDLE_ERROR_INFO iisLinkedListDelete(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  HANDLE_ERROR_INFO retError = noError;

  if (hInstance != NULL) {
    iisFree(hInstance);
  } else {
    retError = iisUtil_ERROR(CDI, "Deleting instance in function iisLinkedListDelete failed");
  }

  return retError;
}

int iisLinkedListAddPtr(
    LINKEDLIST_INSTANCE_HANDLE hInstance,
    void *ptr,
    LINKEDLIST_ADD_MODE bAddMode

) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hLinearList = NULL;
  int isAdded = 0;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  switch (bAddMode) {
    case LINKEDLIST_ADD_MODE_AT_END:
      if (hPrivateData->hFirst) {
        hLinearList = hPrivateData->hFirst;
        while (hLinearList->hNext) {
          hLinearList = hLinearList->hNext;
        }
        hLinearList->hNext = (LINKEDLIST_LINEAR_DATA_LIST_HANDLE)iisMalloc(sizeof(LINKEDLIST_LINEAR_DATA_LIST));
        hLinearList = hLinearList->hNext;
      } else {
        hPrivateData->hFirst = (LINKEDLIST_LINEAR_DATA_LIST_HANDLE)iisMalloc(sizeof(LINKEDLIST_LINEAR_DATA_LIST));
        hLinearList = hPrivateData->hFirst;
      }
      hLinearList->ptr = ptr;
      hLinearList->hNext = NULL;
      hInstance->storedItemsCount++;
      isAdded = 1;
      break;
    case LINKEDLIST_ADD_MODE_AT_START:
      hLinearList = hPrivateData->hFirst;
      hPrivateData->hFirst = (LINKEDLIST_LINEAR_DATA_LIST_HANDLE)iisMalloc(sizeof(LINKEDLIST_LINEAR_DATA_LIST));
      hPrivateData->hFirst->hNext = hLinearList;
      hLinearList = hPrivateData->hFirst;
      hLinearList->ptr = ptr;
      hInstance->storedItemsCount++;
      isAdded = 1;
      break;
    default:
      break;
  }
  return isAdded;
}

int iisLinkedListRemovePtr(
    LINKEDLIST_INSTANCE_HANDLE hInstance,
    void *ptr) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hLinearListCurrent = NULL;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hLinearListPrevious = NULL;
  int isRemoved = 0;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  hLinearListCurrent = hPrivateData->hFirst;
  while (hLinearListCurrent != NULL) {
    if (hLinearListCurrent->ptr == ptr) {
      isRemoved = 1;
      if (hLinearListPrevious != NULL) {
        hLinearListPrevious->hNext = hLinearListCurrent->hNext;
      } else {
        hPrivateData->hFirst = hLinearListCurrent->hNext;
      }
      iisFree(hLinearListCurrent);
      hInstance->storedItemsCount--;
      return isRemoved;
    }
    hLinearListPrevious = hLinearListCurrent;
    hLinearListCurrent = hLinearListCurrent->hNext;
  }
  return isRemoved;
}

void iisLinkedListRemoveAllPtr(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  while (hPrivateData->hFirst) {
    iisLinkedListRemovePtr(hInstance, hPrivateData->hFirst->ptr);
  }
}

int iisLinkedListStartIteration(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  hPrivateData->iteration = 1;
  hPrivateData->hIterationPrev = NULL;
  hPrivateData->hIteration = hPrivateData->hFirst;

  return 1;
}

int iisLinkedListStopIteration(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  hPrivateData->iteration = 0;

  return 1;
}

void *iisLinkedListIterateRemoveCurrentElement(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hLinearListPrevious = NULL;
  LINKEDLIST_LINEAR_DATA_LIST_HANDLE hLinearListCurrent = NULL;
  void *ptr = NULL;

  if (hInstance != NULL) {
    hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

    hLinearListCurrent = hPrivateData->hFirst;
  }

  while (hLinearListCurrent != NULL) {
    if (hLinearListCurrent->hNext == hPrivateData->hIteration) {
      if (hLinearListPrevious != NULL) {
        hLinearListPrevious->hNext = hLinearListCurrent->hNext;
      } else {
        hPrivateData->hFirst = hLinearListCurrent->hNext;
      }
      ptr = hLinearListCurrent->ptr;
      iisFree(hLinearListCurrent);
      hInstance->storedItemsCount--;
      break;
    }
    hLinearListPrevious = hLinearListCurrent;
    hLinearListCurrent = hLinearListCurrent->hNext;
  }

  return ptr;
}

void *iisLinkedListIterate(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;
  void *ptr = NULL;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  if (hPrivateData->iteration == 1) {
    if (hPrivateData->hIteration) {
      ptr = hPrivateData->hIteration->ptr;
      if (hPrivateData->hIteration != hPrivateData->hFirst) {
        hPrivateData->hIterationPrev = hPrivateData->hIteration;
      }
      hPrivateData->hIteration = hPrivateData->hIteration->hNext;
    } else {
      hPrivateData->iteration = 0;
    }
  }

  return ptr;
}

int iisLinkedListIsEndOfIterate(
    LINKEDLIST_INSTANCE_HANDLE hInstance) {
  LINKEDLIST_PRIVATE_DATA_HANDLE hPrivateData;
  int endOfIterate = -1;

  hPrivateData = iisLinkedListGetPrivateDataHandle(hInstance);

  if (hPrivateData->iteration == 1) {
    if (hPrivateData->hIteration == NULL) {
      endOfIterate = 1;
    } else {
      endOfIterate = 0;
    }
  }

  return endOfIterate;
}

LINKEDLIST_INSTANCE_HANDLE iisLinkedListCopy(
    LINKEDLIST_INSTANCE_HANDLE const hInstance) {
  LINKEDLIST_INSTANCE_HANDLE hInstanceCopy = NULL;

  if (hInstance != NULL) {
    iisLinkedListNew(&hInstanceCopy);
    if (hInstanceCopy) {
      iisLinkedListStartIteration(hInstance);
      while (!iisLinkedListIsEndOfIterate(hInstance)) {
        void *ptr = NULL;
        ptr = iisLinkedListIterate(hInstance);
        if (ptr != NULL) {
          iisLinkedListAddPtr(hInstanceCopy, ptr, LINKEDLIST_ADD_MODE_AT_END);
        }
      }
      iisLinkedListStopIteration(hInstance);
    }
  }
  return hInstanceCopy;
}

void iisLinkedListCallBack(
    LINKEDLIST_INSTANCE_HANDLE hInstance,
    FCT_HANDLE function,
    void *ptr) {
  if (hInstance != NULL && function != NULL) {
    iisLinkedListStartIteration(hInstance);
    while (!iisLinkedListIsEndOfIterate(hInstance)) {
      void *tmpVal = NULL;
      tmpVal = iisLinkedListIterate(hInstance);
      function(hInstance, tmpVal, ptr);
    }
    iisLinkedListStopIteration(hInstance);
  }
}
