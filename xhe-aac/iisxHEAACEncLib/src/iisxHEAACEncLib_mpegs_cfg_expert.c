
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
#include <string.h>

#include "iisxHEAACEncLib_mpegs_cfg_expert.h"
#include "iisxHEAACEncLib_mpegs_bitrate_cfg.h"
#include "iisxHEAACEncLib_common.h"
#include "iisxHEAACEncLib_returnCodes.h"

#ifndef max
#define max(a, b) ((a > b) ? a : b)
#endif

enum {
  MPEGS_UNIFIED_STEREO_40_KBPS = 40000,
  MPEGS_UNIFIED_STEREO_48_KBPS = 48000,
  MPEGS_UNIFIED_STEREO_64_KBPS = 64000
};

static XHEAACENCLIB_RETURN
mpegscfgexpert_checkConfiguration(AUD_OBJ_TYP aot,
                                  XHEAACENCLIB_SIGMAP_INDEX *CicpIndex,
                                  MPEGS_DOWNMIX_CONFIG *mpegsDownmixConfig);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsConfigurationExpert(XHEAACENCLIB_CONFIG_HANDLE hConfig,
                                        MP4SPACEENC_SETUP *pSpaceEncSetup) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  MP4SPACEENC_SETUP spaceEncSetupTmp = {0};
  const int numQmfBands = 64;
  MPEGS_BITRATE_BORDERS bitrateBorders = {0};
  XHEAACENCLIB_SIGMAP_INDEX CicpIndex = hConfig->CicpIndex;
  MPEGS_DOWNMIX_CONFIG mpegsDownmixConfig = hConfig->mpegsDownmixCfg;

  if (pSpaceEncSetup == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    retValue = mpegscfgexpert_checkConfiguration(hConfig->aot, &CicpIndex, &mpegsDownmixConfig);
  }

  if (!isError(retValue)) {
    memset(&spaceEncSetupTmp, 0, sizeof(MP4SPACEENC_SETUP));

    spaceEncSetupTmp.bCalcDPL = (mpegsDownmixConfig == MPEGS_DOWNMIX_MATRIX_COMPAT) ? 1 : 0;
    spaceEncSetupTmp.bUseCoarseQuant = 0;
    spaceEncSetupTmp.bUseOneIcc = 0;
    spaceEncSetupTmp.cpcStopFrequency = 0.5f * hConfig->sampleRateOut;
    spaceEncSetupTmp.nParamBands = MP4SPACEENC_BANDS_28;
    spaceEncSetupTmp.quantMode = MP4SPACEENC_QUANTMODE_FINE;
    spaceEncSetupTmp.residualConfig.codec = MP4SPACEENC_RES_CODEC_AAC;
    spaceEncSetupTmp.residualConfig.framesPerSpatial = 1;
    spaceEncSetupTmp.sampleRate = hConfig->sampleRateOut;
    spaceEncSetupTmp.bCalcDPL = 0;
    spaceEncSetupTmp.smoothConfig = MP4SPACEENC_SMOOTHCONFIG_AUTOOFF;
    spaceEncSetupTmp.bTimeDomainDmx = 0;
    spaceEncSetupTmp.bPsStyleDmx = 0;

    if (hConfig->aot == AUD_OBJ_TYP_USAC && CicpIndex == XHEAACENCLIB_SIGMAP_CICP_2) {
      spaceEncSetupTmp.encMode = MP4SPACEENC_USAC_212;
      spaceEncSetupTmp.residualConfig.mode = MP4SPACEENC_RES_MODE_NONE;
    }
    spaceEncSetupTmp.tempShapeConfig = MP4SPACEENC_TEMPSHAPE_OFF;
  }
  if (!isError(retValue)) {
    {
      spaceEncSetupTmp.bArbitraryDMX = 0;
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibMpegscfgexpert_getBitrateTable(&bitrateBorders,
                                                             spaceEncSetupTmp.encMode);
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibMpegscfgexpert_getBitrateDependentSettings(&spaceEncSetupTmp,
                                                                         hConfig,
                                                                         spaceEncSetupTmp.encMode,
                                                                         numQmfBands,
                                                                         bitrateBorders);
  }

  if (!isError(retValue)) {
    int downmixFramesPerMPSFrame = spaceEncSetupTmp.frameTimeSlots * numQmfBands / hConfig->nFrameSamples;
    spaceEncSetupTmp.timeAlignment = (downmixFramesPerMPSFrame - 1) * hConfig->nFrameSamples;
  }

  if (!isError(retValue)) {
    if (hConfig->mpegsIndependencyTimeInterval >= 0.f) {
      spaceEncSetupTmp.independencyFactor = max(1, (unsigned int)((hConfig->mpegsIndependencyTimeInterval * spaceEncSetupTmp.sampleRate) /
                                                                  (float)(numQmfBands * spaceEncSetupTmp.frameTimeSlots)));
    } else {
      spaceEncSetupTmp.independencyFactor = 0;
    }
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_SIGMAP_INDEX CicpIndexCoreCoderTmp = XHEAACENCLIB_SIGMAP_INVALID;
    CicpIndexCoreCoderTmp = XHEAACENCLIB_SIGMAP_CICP_1;
    hConfig->CicpIndexCoreCoder = CicpIndexCoreCoderTmp;
  }

  if (!isError(retValue)) {
    if (pSpaceEncSetup != NULL) {
      *pSpaceEncSetup = spaceEncSetupTmp;
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN
mpegscfgexpert_checkConfiguration(AUD_OBJ_TYP aot,
                                  XHEAACENCLIB_SIGMAP_INDEX *pCicpIndex,
                                  MPEGS_DOWNMIX_CONFIG *mpegsDownmixConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (pCicpIndex == NULL || mpegsDownmixConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    MPEGS_DOWNMIX_CONFIG mpegsDownmixConfigTmp = *mpegsDownmixConfig;
    XHEAACENCLIB_SIGMAP_INDEX CicpIndex = *pCicpIndex;

    if (aot == AUD_OBJ_TYP_USAC) {
      if (CicpIndex != XHEAACENCLIB_SIGMAP_CICP_2) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue)) {
      if (mpegsDownmixConfigTmp != MPEGS_DOWNMIX_DEFAULT &&
          mpegsDownmixConfigTmp != MPEGS_DOWNMIX_FORCE_STEREO &&
          mpegsDownmixConfigTmp != MPEGS_DOWNMIX_MATRIX_COMPAT &&
          mpegsDownmixConfigTmp != MPEGS_DOWNMIX_ARBITRARY_MONO &&
          mpegsDownmixConfigTmp != MPEGS_DOWNMIX_ARBITRARY_STEREO) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
      }
    }

    if (!isError(retValue)) {
      *mpegsDownmixConfig = mpegsDownmixConfigTmp;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsResidualConfig(int bitrate, int stereoConfigIndex, MP4SPACEENC_SETUP *pSpaceEncSetup) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (pSpaceEncSetup == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    pSpaceEncSetup->residualConfig.mode = MP4SPACEENC_RES_MODE_USAC;
    pSpaceEncSetup->residualConfig.codec = MP4SPACEENC_RES_CODEC_PCM;
    pSpaceEncSetup->nParamBands = MP4SPACEENC_BANDS_28;
    pSpaceEncSetup->downmixType = MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL;
    pSpaceEncSetup->residualConfig.framesPerSpatial = 1;
    pSpaceEncSetup->ipdMode = MP4SPACEENC_IPDMODE_RESIDUAL;
    pSpaceEncSetup->residualConfig.bitRate[0] = 0;
    pSpaceEncSetup->bStereoSbr = (stereoConfigIndex == 3);
    pSpaceEncSetup->bPseudoLr = 0;
  }
  if (!isError(retValue)) {
    if (bitrate >= MPEGS_UNIFIED_STEREO_64_KBPS) {
      pSpaceEncSetup->residualConfig.bands[0] = 24;
      pSpaceEncSetup->bPsStyleDmx = 0;
    } else if (bitrate >= MPEGS_UNIFIED_STEREO_48_KBPS) {
      pSpaceEncSetup->residualConfig.bands[0] = 20;
      pSpaceEncSetup->bPsStyleDmx = 0;
    } else if (bitrate >= MPEGS_UNIFIED_STEREO_40_KBPS) {
      pSpaceEncSetup->residualConfig.bands[0] = 14;
      pSpaceEncSetup->bPsStyleDmx = 0;
    } else {
      exit(0);
    }
  }

  return retValue;
}
