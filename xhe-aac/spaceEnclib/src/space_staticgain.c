
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

#include <math.h>
#include "mathlib.h"

#include "spaceEnclib_const.h"
#include "space_staticgain.h"

#define DMX_GAIN_TABLE \
  { 1.00000000f, 0.84089650f, 0.70710706f, 0.59460385f, 0.50000000f, 0.42044825f, 0.35355341f, 0.25000000f }

struct STATIC_GAIN {
  MP4SPACEENC_MODE encMode;
  MP4SPACEENC_DMX_GAIN fixedGainDMX;
  int preGainFactorDb;

  int numInChan;
  int numOutChan;
  float PostGain;
  float pPreGain[MAX_INPUT_CHANNELS];
  float pPreGainTdDmxCorrection[MAX_INPUT_CHANNELS];
};

HANDLE_ERROR_INFO
StaticGain_OpenConfig(HANDLE_STATIC_GAIN_CONFIG *phStaticGainConfig, MP4SPACEENC_MODE encMode) {
  HANDLE_ERROR_INFO error = noError;
  HANDLE_STATIC_GAIN_CONFIG hStGCfg = NULL;

  if (NULL == phStaticGainConfig) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    *phStaticGainConfig = (HANDLE_STATIC_GAIN_CONFIG)iisCalloc(1, sizeof(struct STATIC_GAIN_CONFIG));
    if (*phStaticGainConfig == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hStGCfg = *phStaticGainConfig;
    }
  }

  if (noError == error && hStGCfg != NULL) {
    hStGCfg->encMode = encMode;

    {
      hStGCfg->fixedGainDMX = MP4SPACEENC_DMX_GAIN_USAC_DEFAULT;
    }

    hStGCfg->preGainFactorDb = 0;
  }
  return error;
}

HANDLE_ERROR_INFO
StaticGain_CloseConfig(HANDLE_STATIC_GAIN_CONFIG *phStaticGainConfig) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL != phStaticGainConfig) {
    if (NULL != *phStaticGainConfig) {
      iisFree(*phStaticGainConfig);
      *phStaticGainConfig = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
StaticGain_Open(HANDLE_STATIC_GAIN *phStaticGain, HANDLE_STATIC_GAIN_CONFIG hStaticGainConfig) {
  float DmxGainTable[] = DMX_GAIN_TABLE;

  HANDLE_ERROR_INFO error = noError;
  HANDLE_STATIC_GAIN hStG = NULL;
  int k;

  if (NULL == phStaticGain) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (noError == error) {
    *phStaticGain = (HANDLE_STATIC_GAIN)iisCalloc(1, sizeof(struct STATIC_GAIN));
    if (*phStaticGain == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hStG = *phStaticGain;
    }
  }

  if (noError == error) {
    if (hStaticGainConfig != NULL) {
      hStG->encMode = hStaticGainConfig->encMode;
      hStG->fixedGainDMX = hStaticGainConfig->fixedGainDMX;
      hStG->preGainFactorDb = hStaticGainConfig->preGainFactorDb;
      hStG->PostGain = DmxGainTable[hStG->fixedGainDMX];
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer to hStaticGainConfig.");
    }
  }

  if (noError == error) {
    float fPreGainFactor = (float)pow(10.f, (((float)hStG->preGainFactorDb) / 20.f));

    {
      hStG->pPreGain[0] = 1.f;
      hStG->pPreGain[1] = 1.f;
      hStG->pPreGainTdDmxCorrection[0] = hStG->pPreGain[0];
      hStG->pPreGainTdDmxCorrection[1] = hStG->pPreGain[1];
      for (k = 2; k < MAX_INPUT_CHANNELS; k++) {
        hStG->pPreGain[k] = 0.f;
      }
      hStG->numInChan = 2;
      hStG->numOutChan = 1;
    }
    for (k = 0; k < hStG->numInChan; k++) {
      hStG->pPreGain[k] *= fPreGainFactor;
    }
  }
  return error;
}

HANDLE_ERROR_INFO StaticGain_Close(HANDLE_STATIC_GAIN *phStaticGain) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL != phStaticGain) {
    if (NULL != *phStaticGain) {
      iisFree(*phStaticGain);
      *phStaticGain = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
StaticPostGain_Apply(HANDLE_STATIC_GAIN hStaticGain,
                     float *pOutputSamples,
                     const int nOutputSamples) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hStaticGain) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }
  if (error == noError) {
    float postGain = hStaticGain->PostGain;
    smulFLOAT(postGain, pOutputSamples, pOutputSamples, nOutputSamples);
  }
  return error;
}

HANDLE_ERROR_INFO
staticGain_ApplyInverseDmxGain(HANDLE_STATIC_GAIN hStaticGain,
                               float *pOutputSamples,
                               const int nOutputSamples) {
  HANDLE_ERROR_INFO error = noError;
  float DmxGainTable[] = DMX_GAIN_TABLE;

  if (NULL == hStaticGain) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  }

  if (error == noError) {
    float invDmxGain = 1.f / DmxGainTable[hStaticGain->fixedGainDMX];
    smulFLOAT(invDmxGain, pOutputSamples, pOutputSamples, nOutputSamples);
  }

  return error;
}

float *GetPreGainPtr(HANDLE_STATIC_GAIN hStaticGain) {
  return hStaticGain->pPreGain;
}

float *GetPostGainPtr(HANDLE_STATIC_GAIN hStaticGain) {
  return &(hStaticGain->PostGain);
}

FIXEDGAINDMXCONFIG staticGain_GetDmxGain(HANDLE_STATIC_GAIN hStaticGain) {
  switch (hStaticGain->fixedGainDMX) {
    case -1:
      return FIXEDGAINDMX_INVALID;
      break;
    case 0:
      return FIXEDGAINDMX_0;
      break;
    case 1:
      return FIXEDGAINDMX_1;
      break;
    case 2:
      return FIXEDGAINDMX_2;
      break;
    case 3:
      return FIXEDGAINDMX_3;
      break;
    case 4:
      return FIXEDGAINDMX_4;
      break;
    case 5:
      return FIXEDGAINDMX_5;
      break;
    case 6:
      return FIXEDGAINDMX_6;
      break;
    case 7:
      return FIXEDGAINDMX_7;
      break;

    default:
      return FIXEDGAINDMX_INVALID;
      break;
  }
}

HANDLE_ERROR_INFO staticGain_SetDmxGain(HANDLE_STATIC_GAIN_CONFIG hStaticGainCfg, MP4SPACEENC_DMX_GAIN dmxGain) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hStaticGainCfg) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (error == noError) {
    hStaticGainCfg->fixedGainDMX = dmxGain;
  }

  return error;
}
