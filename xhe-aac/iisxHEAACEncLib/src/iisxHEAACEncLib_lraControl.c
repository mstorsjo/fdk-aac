
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

#include "iisxHEAACEncLib_lraControl.h"
#include "iisxHEAACEncLib_common.h"

XHEAACENCLIB_RETURN iisxHEAACEncLib_lraControl_drcGainInit(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_LRACONTROL_DRC_GAIN_DATA* const lraControlDrcGainData,
    int const drcEncoderDelay,
    int const drcDecoderDelay,
    int const drcLookAhead,
    int const mpeg4Priming,
    int const nTrashAUs) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == hConfig || lraControlDrcGainData == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (lraControlDrcGainData->lraControlDrcProcessingIsActive) {
    if (!isError(retValue)) {
      unsigned int drcCodecDelay = (unsigned int)(drcDecoderDelay + drcEncoderDelay + drcLookAhead);
      unsigned int primingDelay = (unsigned int)(mpeg4Priming + nTrashAUs * hConfig->nFrameSamples);
      const float conversionFactor_1000ms_to_100ms = (1.0f / 10.0f);

      if ((hConfig->bUseSBR) || (hConfig->stereoConfigIndex > 0)) {
        primingDelay -= QMF_SYNTHESIS_DELAY;
      }

      if (drcCodecDelay >= primingDelay) {
        lraControlDrcGainData->primingOffset = 0;
      } else {
        lraControlDrcGainData->primingOffset = primingDelay - drcCodecDelay;
      }

      lraControlDrcGainData->lraControlDrcFrameSize = (int)((float)hConfig->sampleRateOut * conversionFactor_1000ms_to_100ms);
      lraControlDrcGainData->lraControlDrcGainsInterpolatedLength = hConfig->nFrameSamples;

      lraControlDrcGainData->lraControlDrcGainsInterpolated = (float*)iisCalloc(lraControlDrcGainData->lraControlDrcGainsInterpolatedLength, sizeof(float));
      if (NULL == lraControlDrcGainData->lraControlDrcGainsInterpolated) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      }

      lraControlDrcGainData->memory.drcSlope = 0;
      lraControlDrcGainData->memory.offset = lraControlDrcGainData->primingOffset;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_lraControl_interpolateDrcGains(
    unsigned int const numberDrcGainOutputValuesRequested,
    XHEAACENCLIB_LRACONTROL_DRC_GAIN_DATA* const lraControlDrcGainData) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL == lraControlDrcGainData) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (lraControlDrcGainData->lraControlDrcGainsInterpolatedLength < numberDrcGainOutputValuesRequested) {
    retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
  }

  if (!isError(retValue)) {
    unsigned int gainIdx;

    if (0 == lraControlDrcGainData->memory.drcFrameindex) {
      lraControlDrcGainData->memory.drcGainRight = lraControlDrcGainData->memory.drcGainLeft = lraControlDrcGainData->lraControlDrcGains[0];
    }

    if (lraControlDrcGainData->memory.offset < numberDrcGainOutputValuesRequested) {
      for (gainIdx = 0; gainIdx < lraControlDrcGainData->memory.offset; gainIdx++) {
        lraControlDrcGainData->lraControlDrcGainsInterpolated[gainIdx] = lraControlDrcGainData->memory.drcGainLeft;
      }

      for (gainIdx = lraControlDrcGainData->memory.offset; gainIdx < numberDrcGainOutputValuesRequested; gainIdx++) {
        if ((lraControlDrcGainData->memory.drcSampleIndex >= lraControlDrcGainData->lraControlDrcFrameSize) || ((lraControlDrcGainData->memory.drcFrameindex == 0) && (lraControlDrcGainData->memory.drcSampleIndex >= (lraControlDrcGainData->lraControlDrcFrameSize >> 1)))) {
          lraControlDrcGainData->memory.drcFrameindex++;
          lraControlDrcGainData->memory.drcSampleIndex = 0;
          lraControlDrcGainData->memory.drcGainLeft = lraControlDrcGainData->memory.drcGainRight;

          if (lraControlDrcGainData->memory.drcFrameindex < lraControlDrcGainData->lraControlDrcGainsLength) {
            lraControlDrcGainData->memory.drcGainRight = lraControlDrcGainData->lraControlDrcGains[lraControlDrcGainData->memory.drcFrameindex];
            lraControlDrcGainData->memory.drcSlope = (lraControlDrcGainData->memory.drcGainRight - lraControlDrcGainData->memory.drcGainLeft) / lraControlDrcGainData->lraControlDrcFrameSize;
          } else {
            lraControlDrcGainData->memory.drcSlope = 0;
          }
        }

        lraControlDrcGainData->lraControlDrcGainsInterpolated[gainIdx] = lraControlDrcGainData->memory.drcSlope * lraControlDrcGainData->memory.drcSampleIndex + lraControlDrcGainData->memory.drcGainLeft;

        lraControlDrcGainData->memory.drcSampleIndex++;
      }
    } else {
      for (gainIdx = 0; gainIdx < numberDrcGainOutputValuesRequested; gainIdx++) {
        lraControlDrcGainData->lraControlDrcGainsInterpolated[gainIdx] = lraControlDrcGainData->memory.drcGainLeft;
      }
    }

    if (lraControlDrcGainData->memory.offset > 0) {
      if (numberDrcGainOutputValuesRequested >= lraControlDrcGainData->memory.offset) {
        lraControlDrcGainData->memory.offset = 0;
      } else {
        lraControlDrcGainData->memory.offset = lraControlDrcGainData->memory.offset - numberDrcGainOutputValuesRequested;
      }
    }
  }

  return retValue;
}
