
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

#define SIGMAP_VERSION_NUMBER "02.00.19"
#define SIGMAP_MODULE_NAME "iisSigMap"
#define SIGMAP_BUILD_DATE __DATE__
#define SIGMAP_BUILD_INFO "Release build"

#ifdef __GNUC__
#define SIGMAP_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
#define SIGMAP_COMPILER_INFO "Compiler: GCC"
#else
#ifdef _MSC_VER
#define SIGMAP_COMPILER_VERSION _MSC_VER
#define SIGMAP_COMPILER_INFO "Compiler: Visual C"
#else
#define SIGMAP_COMPILER_VERSION 0
#define SIGMAP_COMPILER_INFO "Compiler: unknown"
#endif
#endif

#if defined __GNUC__ || defined __clang__
#define SIGMAP_MPH __attribute__((unused))
#else
#define SIGMAP_MPH
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iisutillib.h"
#include "iisSigMap.h"
#include "iisSigMapConfig.h"
#include "iisSigMapCommon.h"
#include "iisSigMapUsac.h"

typedef struct sigmap_private_data_struct {
  int nSize;
  SIGMAP_CONFIG_HANDLE hConfig;
} SIGMAP_PRIVATE_DATA, *SIGMAP_PRIVATE_DATA_HANDLE;

static SIGMAP_PRIVATE_DATA_HANDLE iisSigMapGetPrivateDataHandle(
    SIGMAP_INSTANCE_HANDLE const hInstance);

static SIGMAP_PRIVATE_DATA_HANDLE iisSigMapGetPrivateDataHandle(
    SIGMAP_INSTANCE_HANDLE const hInstance) {
  SIGMAP_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (void *)((char *)hInstance + sizeof(struct sigmap_instance_struct));
  return hPrivateData;
}

SIGMAP_RETURN iisSigMapAddElement(SIGMAP_SETUP_HANDLE hSetup,
                                  SIGMAP_ELEMENT_TYPE elementType,
                                  float relativeBits,
                                  SIGMAP_MPH int sigGroupID) {
  if (hSetup->type == SIGMAP_TYPE_USAC) {
    return iisSigMapUsacAddElementToSetup(hSetup, elementType, relativeBits);
  }
  return SIGMAP_ERROR_INVALID_SETUP;
}

void iisSigMapDeleteObjectsFromSetup(
    SIGMAP_SETUP_HANDLE hSetup) {
  if (hSetup != NULL) {
    iisSigMapCommonDeleteObjectsFromSetup(hSetup);
  }
}

SIGMAP_RETURN iisSigMapNew(
    SIGMAP_INSTANCE_HANDLE *phInstance) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  int nSize = 0;
  SIGMAP_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (phInstance == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    nSize += sizeof(struct sigmap_instance_struct);
    nSize += sizeof(struct sigmap_private_data_struct);

    *phInstance = (SIGMAP_INSTANCE_HANDLE)iisMalloc(nSize);
    if (*phInstance == NULL) {
      retCode = SIGMAP_ERROR_MEMORY;
    }
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (void *)((char *)(*phInstance) + sizeof(struct sigmap_instance_struct));
    hPrivateData->nSize = nSize;

    retCode = iisSigMapConfigNew(&(hPrivateData->hConfig));

    sprintf((*phInstance)->infoModuleVersion, "%s %s", SIGMAP_MODULE_NAME, SIGMAP_VERSION_NUMBER);
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapDelete(
    SIGMAP_INSTANCE_HANDLE hInstance) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  SIGMAP_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hInstance != NULL) {
    hPrivateData = iisSigMapGetPrivateDataHandle(hInstance);

    retCode = iisSigMapConfigDelete(hPrivateData->hConfig);

    iisFree(hInstance);

  } else {
    retCode = SIGMAP_ERROR_UNKNOWN;
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapConfigure(
    SIGMAP_INSTANCE_HANDLE hInstance,
    SIGMAP_SETUP_HANDLE hSetup,
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  SIGMAP_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  CHANNEL_MAPPING chMap;
  memset(&chMap, 0, sizeof chMap);

  if (hInstance == NULL || hSetup == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    hPrivateData = iisSigMapGetPrivateDataHandle(hInstance);

    if (hSetup->type == SIGMAP_TYPE_USAC) {
      retCode = iisSigMapUsacParseSetup(hSetup, &chMap);
    } else {
      retCode = SIGMAP_ERROR_INVALID_SETUP;
    }
  }

  if (retCode == SIGMAP_NO_ERROR) {
    retCode = iisSigMapConfigSetConfig(hPrivateData->hConfig, &chMap);
  }

  if (retCode == SIGMAP_NO_ERROR) {
    if (hChMap != NULL) {
      memmove(hChMap, &chMap, sizeof(CHANNEL_MAPPING));
    }
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapGetChannelMap(
    SIGMAP_INSTANCE_HANDLE hInstance,
    CHANNEL_MAPPING_HANDLE hChMap) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;
  SIGMAP_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hInstance == NULL || hChMap == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    hPrivateData = iisSigMapGetPrivateDataHandle(hInstance);
    retCode = iisSigMapConfigGetChannelMap(hPrivateData->hConfig, hChMap);
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapInitDefaultSignalGroupInfo(
    SIGNAL_GROUP_INFO *const pSignGroupInfo) {
  int speaker = 0;
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;

  if (pSignGroupInfo == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    pSignGroupInfo->signalGroupType = SIGMAP_SIGNAL_GROUP_TYPE_UNKNOWN;

    pSignGroupInfo->channelGroupInfo.speakerLayoutType = SIGMAP_SPEAKER_LAYOUT_UNKNOWN;

    pSignGroupInfo->channelGroupInfo.numSpeakers = 0;
    pSignGroupInfo->channelGroupInfo.cicpSpeakerLayoutIndex = SIGMAP_CICP_SPEAKER_LAYOUT_INVALID;

    for (speaker = 0; speaker < SIGMAP_MAX_CHANNELS; speaker++) {
      pSignGroupInfo->channelGroupInfo.cicpSpeakerIdxList[speaker] = SIGMAP_CICP_SPEAKER_INVALID;
    }
  }

  return retCode;
}

SIGMAP_RETURN iisSigMapAddSignalGroupInfo(
    CHANNEL_MAPPING_HANDLE hChMap,
    SIGNAL_GROUP_INFO const *const pSignGroupInfo,
    int sigGroupID) {
  SIGMAP_RETURN retCode = SIGMAP_NO_ERROR;

  if (pSignGroupInfo == NULL || hChMap == NULL) {
    retCode = SIGMAP_ERROR_INVALID_HANDLE;
  }

  if (retCode < SIGMAP_ERROR_FIRST) {
    hChMap->sigGroupInfo[sigGroupID] = *pSignGroupInfo;

    if (hChMap->sigGroupInfo[sigGroupID].signalGroupType != SIGMAP_SIGNAL_SIGNAL_GROUP_TYPE_CHANNELS) {
      int speaker = 0;
      hChMap->sigGroupInfo[sigGroupID].channelGroupInfo.speakerLayoutType = SIGMAP_SPEAKER_LAYOUT_UNKNOWN;
      hChMap->sigGroupInfo[sigGroupID].channelGroupInfo.numSpeakers = 0;
      hChMap->sigGroupInfo[sigGroupID].channelGroupInfo.cicpSpeakerLayoutIndex = SIGMAP_CICP_SPEAKER_LAYOUT_INVALID;

      for (speaker = 0; speaker < SIGMAP_MAX_CHANNELS; speaker++) {
        hChMap->sigGroupInfo[sigGroupID].channelGroupInfo.cicpSpeakerIdxList[speaker] = SIGMAP_CICP_SPEAKER_INVALID;
      }
    }
  }
  return retCode;
}
