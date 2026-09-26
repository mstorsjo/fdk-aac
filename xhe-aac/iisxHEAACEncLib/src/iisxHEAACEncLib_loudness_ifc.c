
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
#include <limits.h>
#include <math.h>
#include "mathlib.h"
#include "iisxHEAACEncLib_loudness_ifc.h"
#include "iisxHEAACEncLib_common.h"
#include "loudnessLib.h"

typedef struct loudness_ifc_data_struct {
  HLOUDNESS_METER hLoudnessMeter;
  int nChannels;
  float loudness;
  float samplePeak;
  float loudnessRange;
  unsigned int loudnessRangeCount;
} LOUDNESS_IFC_DATA;

LOUDNESS_IFC_RETURN iisxHEAACEncLib_loudness_ifc_new(
    XHEAACENCLIB_LOUDNESS_HANDLE* phInstance) {
  LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;

  LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;

  *phInstance = iisCalloc(1, sizeof(LOUDNESS_IFC_DATA));

  if (!(*phInstance)) {
    retValueLoudness = LOUDNESS_IFC_ERROR_MEMORY;
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    errorLoudness = LoudMeter_Create(&(*phInstance)->hLoudnessMeter);
    if (errorLoudness != ERRLOUD_Success) {
      retValueLoudness = LOUDNESS_IFC_ERROR_UNKNOWN;
    }
  }
  return retValueLoudness;
}

LOUDNESS_IFC_RETURN iisxHEAACEncLib_loudness_ifc_config(
    XHEAACENCLIB_LOUDNESS_HANDLE hInstance,
    XHEAACENCLIB_LOUDNESS_SETUP_MEASURE const loudnessSetup) {
  LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;
  LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;
  LOUDMTR_LS_POS channelConfig[2] = {LS_POS_M_FRONT, LS_POS_M_FRONT};
  int channelGroup[2] = {1, 1};

  if (!hInstance) {
    retValueLoudness = LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    if (loudnessSetup.nChannels > 2) {
      retValueLoudness = LOUDNESS_IFC_ERROR_UNKNOWN;
    } else {
      hInstance->nChannels = loudnessSetup.nChannels;
    }
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    errorLoudness = LoudMeter_Config(
        hInstance->hLoudnessMeter,
        channelConfig,
        channelGroup,
        loudnessSetup.nChannels,
        loudnessSetup.sampleRate,
        LOUDMTR_TPFILTER_ITU);
    if (errorLoudness != ERRLOUD_Success) {
      retValueLoudness = LOUDNESS_IFC_ERROR_UNKNOWN;
    }
  }

  LoudMeter_InitGatedMidTermLoudness(hInstance->hLoudnessMeter, 6000);
  LoudMeter_InitMidTermLoudnessRange(hInstance->hLoudnessMeter, 36000);

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    errorLoudness = LoudMeter_SetWorkloadConfig(hInstance->hLoudnessMeter, LOUDMTR_WORKLOAD_DISABLE_TP);
    if (errorLoudness != ERRLOUD_Success) {
      retValueLoudness = LOUDNESS_IFC_ERROR_UNKNOWN;
    }
  }

  return retValueLoudness;
}

LOUDNESS_IFC_RETURN iisxHEAACEncLib_loudness_ifc_delete(
    XHEAACENCLIB_LOUDNESS_HANDLE hInstance) {
  LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;
  LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;

  if (!hInstance) {
    retValueLoudness = LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    errorLoudness = LoudMeter_Destroy(&hInstance->hLoudnessMeter);
    if (errorLoudness != ERRLOUD_Success) {
      retValueLoudness = LOUDNESS_IFC_ERROR_UNKNOWN;
    }
  }
  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    iisFree(hInstance);
  }

  return retValueLoudness;
}

LOUDNESS_IFC_RETURN iisxHEAACEncLib_loudness_ifc_Process(
    XHEAACENCLIB_LOUDNESS_HANDLE const hLoudness,
    float const* const pSamples,
    int const nSamples, unsigned int const bLiveLoudnessLevelSet) {
  LOUDNESS_IFC_RETURN retValueLoudness = LOUDNESS_IFC_NO_ERROR;

  if (!hLoudness || !pSamples) {
    retValueLoudness = LOUDNESS_IFC_ERROR_INVALID_HANDLE;
  } else if (hLoudness->nChannels <= 0) {
    retValueLoudness = LOUDNESS_IFC_ERROR_INVALID_VALUE;
  } else if (nSamples % hLoudness->nChannels) {
    retValueLoudness = LOUDNESS_IFC_ERROR_INVALID_VALUE;
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    int nBlockStart = 0;

    if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
      LOUDMTR_ERROR errorLoudness = ERRLOUD_Success;
      errorLoudness = LoudMeter_Feed(hLoudness->hLoudnessMeter,
                                     &pSamples[nBlockStart],
                                     nSamples / hLoudness->nChannels);

      if (ERRLOUD_Success != errorLoudness) {
        retValueLoudness = LOUDNESS_IFC_ERROR_LOUDNESS_MEASUREMENT;
      }
    }
  }

  if (retValueLoudness == LOUDNESS_IFC_NO_ERROR) {
    if (bLiveLoudnessLevelSet == 1) {
      hLoudness->loudness = LoudMeter_GetGatedMidTermLoudness(hLoudness->hLoudnessMeter);
      hLoudness->samplePeak = LoudMeter_GetMaxSamplePeak(hLoudness->hLoudnessMeter, 0);
    }
    hLoudness->loudnessRange = LoudMeter_GetMidTermLoudnessRange(hLoudness->hLoudnessMeter);
    hLoudness->loudnessRangeCount = LoudMeter_GetMidTermLoudnessRangeCount(hLoudness->hLoudnessMeter);
  }

  return retValueLoudness;
}

float iisxHEAACEncLib_loudness_ifc_getLoudness(
    XHEAACENCLIB_LOUDNESS_HANDLE const hInstance) {
  return hInstance->loudness;
}

float iisxHEAACEncLib_loudness_ifc_getLoudnessRange(
    XHEAACENCLIB_LOUDNESS_HANDLE const hInstance) {
  return hInstance->loudnessRange;
}

unsigned int iisxHEAACEncLib_loudness_ifc_getLoudnessRangeCount(
    XHEAACENCLIB_LOUDNESS_HANDLE const hInstance) {
  return hInstance->loudnessRangeCount;
}
