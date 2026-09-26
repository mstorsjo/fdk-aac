
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

#include "iisxHEAACEncLib_mpegs_cfg_expert.h"
#include "iisxHEAACEncLib_mpegs_bitrate_cfg.h"
#include "iisxHEAACEncLib_common.h"

static const MPEGS_BITRATE_BORDERS mpegscfgexpert_treeBitrateTableHE[] =
    {

        {MP4SPACEENC_USAC_212, 12000, 72000, 32000, 20000, 0, 0.0f, 0},
        {MP4SPACEENC_ESCAPE, 0, 0, 0, 0, 0, 0.0f, 0}};

static XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_usacBitrateSettings(MP4SPACEENC_SETUP* pSpaceEncSetupTmp, XHEAACENCLIB_CONFIG_HANDLE const hConfig, const MPEGS_BITRATE_BORDERS bitrateBorders);
static XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_getFrameTimeSlots(unsigned int* pFrameTimeSlots,
                                                                           const int granuleLength, const int numQmfBands);

XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_getBitrateDependentSettings(MP4SPACEENC_SETUP* pSpaceEncSetupTmp,
                                                                              XHEAACENCLIB_CONFIG_HANDLE const hConfig,
                                                                              const MP4SPACEENC_MODE encMode,
                                                                              const int numQmfBands,
                                                                              const MPEGS_BITRATE_BORDERS bitrateBorders) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  MP4SPACEENC_SETUP spaceEncSetupTmp = {0};

  if (pSpaceEncSetupTmp == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    const unsigned int minimumBitrate = bitrateBorders.minBr;
    const unsigned int maximumBitrate = bitrateBorders.maxBr;
    const unsigned int arbitraryDmxBitrate = bitrateBorders.arbDmxBr;
    spaceEncSetupTmp = *pSpaceEncSetupTmp;

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLibMpegscfgexpert_getFrameTimeSlots(&spaceEncSetupTmp.frameTimeSlots,
                                                                 hConfig->nFrameSamples,
                                                                 numQmfBands);
    }

    if (!isError(retValue)) {
      if (hConfig->bitRate < arbitraryDmxBitrate) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue)) {
      if ((hConfig->bitRate >= minimumBitrate) && (hConfig->bitRate <= maximumBitrate)) {
        if (encMode == MP4SPACEENC_USAC_212) {
          retValue = iisxHEAACEncLibMpegscfgexpert_usacBitrateSettings(&spaceEncSetupTmp, hConfig, bitrateBorders);
        }
      } else {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue)) {
      *pSpaceEncSetupTmp = spaceEncSetupTmp;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_usacBitrateSettings(MP4SPACEENC_SETUP* pSpaceEncSetupTmp,
                                                                             XHEAACENCLIB_CONFIG_HANDLE const hConfig,
                                                                             const MPEGS_BITRATE_BORDERS bitrateBorders) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  MP4SPACEENC_SETUP spaceEncSetupTmp;

  if (pSpaceEncSetupTmp == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    const unsigned int minimumBitrate = bitrateBorders.minBr;
    const unsigned int maximumBitrate = bitrateBorders.maxBr;
    const unsigned int unifiedStereoBorder = bitrateBorders.noResBr;
    const unsigned int lowBitrateBorder = bitrateBorders.lessPbBr;
    spaceEncSetupTmp = *pSpaceEncSetupTmp;

    spaceEncSetupTmp.encMode = MP4SPACEENC_USAC_212;
    spaceEncSetupTmp.downmixType = MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED;
    spaceEncSetupTmp.ipdMode = MP4SPACEENC_IPDMODE_NO_RESIDUAL;
    spaceEncSetupTmp.tempShapeConfig = MP4SPACEENC_TEMPSHAPE_OFF;
    spaceEncSetupTmp.quantMode = MP4SPACEENC_QUANTMODE_FINE;
    spaceEncSetupTmp.smoothConfig = MP4SPACEENC_SMOOTHCONFIG_AUTOOFF;
    spaceEncSetupTmp.cpcStopFrequency = 0.0f;
    spaceEncSetupTmp.bUseOneIcc = 0;
    spaceEncSetupTmp.bUseCoarseQuant = 0;
    spaceEncSetupTmp.residualConfig.mode = MP4SPACEENC_RES_MODE_NONE;

    if ((hConfig->bitRate >= minimumBitrate) && (hConfig->bitRate <= lowBitrateBorder)) {
      spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_10;
      spaceEncSetupTmp.bsHighRateMode = MP4SPACEENC_USAC212_LOW;
      spaceEncSetupTmp.bUseCoarseQuantIPD = 0;
      spaceEncSetupTmp.bOpdSmoothing = 0;
      spaceEncSetupTmp.bIpdRelevancy = 1;
    } else if ((hConfig->bitRate > lowBitrateBorder) && (hConfig->bitRate <= maximumBitrate)) {
      spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_20;
      spaceEncSetupTmp.bsHighRateMode = MP4SPACEENC_USAC212_HIGH;

      if (hConfig->bitRate < unifiedStereoBorder) {
        spaceEncSetupTmp.bUseCoarseQuantIPD = 1;
        spaceEncSetupTmp.bOpdSmoothing = 1;
      } else {
        spaceEncSetupTmp.bUseCoarseQuantIPD = 0;
        spaceEncSetupTmp.bOpdSmoothing = 0;
      }
      spaceEncSetupTmp.bIpdRelevancy = 0;
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
    }
    if (!isError(retValue)) {
      if (hConfig->useMpegsHighRateMode == TOOL_MODE_ON) {
        spaceEncSetupTmp.bsHighRateMode = MP4SPACEENC_USAC212_HIGH;
      } else if (hConfig->useMpegsHighRateMode == TOOL_MODE_OFF) {
        spaceEncSetupTmp.bsHighRateMode = MP4SPACEENC_USAC212_LOW;
      }
    }
    if (!isError(retValue)) {
      if (hConfig->mpegsSetParamBands > 0) {
        switch (hConfig->mpegsSetParamBands) {
          case 4:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_4;
            break;
          case 5:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_5;
            break;
          case 7:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_7;
            break;
          case 9:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_9;
            break;
          case 10:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_10;
            break;
          case 12:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_12;
            break;
          case 14:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_14;
            break;
          case 15:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_15;
            break;
          case 20:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_20;
            break;
          case 23:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_23;
            break;
          case 28:
            spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_28;
            break;
          default:
            retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
            break;
        }
      }
    }

    if (!isError(retValue)) {
      if (hConfig->bUseTSD) {
        spaceEncSetupTmp.tempShapeConfig = MP4SPACEENC_TEMPSHAPE_TSD;
      }
    }

    if (!isError(retValue)) {
      *pSpaceEncSetupTmp = spaceEncSetupTmp;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_getFrameTimeSlots(
    unsigned int* pFrameTimeSlots,

    const int granuleLength,
    const int numQmfBands) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (pFrameTimeSlots == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *pFrameTimeSlots = HE_AAC_FRAMES_PER_SPATIAL_FRAME * granuleLength / numQmfBands;
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibMpegscfgexpert_getBitrateTable(MPEGS_BITRATE_BORDERS* brBorders,
                                                                  const MP4SPACEENC_MODE TreeConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  MPEGS_BITRATE_BORDERS tmpBorders = {0};
  int i = 0;
  int idxFound = 0;

  if (brBorders == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else {
    tmpBorders = *brBorders;
  }

  if (!isError(retValue)) {
    while (idxFound == 0) {
      tmpBorders = mpegscfgexpert_treeBitrateTableHE[i];
      if (tmpBorders.treeConfig == TreeConfig) {
        idxFound = 1;
      }
      if (tmpBorders.treeConfig == MP4SPACEENC_ESCAPE) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
        break;
      }
      ++i;
    }

    if (!isError(retValue)) {
      *brBorders = tmpBorders;
    }
  }
  return retValue;
}
