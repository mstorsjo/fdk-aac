
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
#include <stdlib.h>

#include "iisxHEAACEncLib_rapOnDemand.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

XHEAACENCLIB_RETURN iisxHEAACEncLib_rapOnDemand_setup(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    PARAMLIST_INSTANCE_HANDLE const hCodecParamList,
    XHEAACENCLIB_AUDIOPREROLL_DATA const* const audioPreRoll,
    XHEAACENCLIB_EXTENDED_BIT_RESERVOIR_PARAMS* const ebrParams,
    XHEAACENCLIB_HANDLE_AACENCODER const hAacEnc,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    XHEAACENCLIB_WARNING_LIST* const warningList,
    int const nTrashAUs,
    int const lastRapFrameDist,
    unsigned int const rapInXSamples) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_RAP_SYNC_INFO rapSyncInfo;
  int syncFrameDelay = -1;
  int rapInXFrames = 0;

  memset(&rapSyncInfo, 0, sizeof(XHEAACENCLIB_RAP_SYNC_INFO));

  if ((hConfig == NULL) || (hUsacIndepFlag == NULL) || (hCodecParamList == NULL) || (audioPreRoll == NULL) ||
      (ebrParams == NULL) || (hAacEnc == NULL) || (warningList == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->rapOccurrence != XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) {
    retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_RAP_OCCURRENCE;
  } else if (rapInXSamples % hConfig->nFrameSamples != 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_POSITION;
  }

  if (!isError(retValue)) {
    int i;
    int tmp_syncFrame = 0;
    int intvlMinFrames;

    intvlMinFrames = hConfig->randomAccessIntervalMin / (hConfig->nFrameSamples);

    retValue = iisxHEAACEncLib_rapOnDemand_SetupRapSyncInfo(hConfig, hLoaswriter, audioPreRoll->hAudioPreRoll, &rapSyncInfo);

    if (!isError(retValue)) {
      rapInXFrames = rapInXSamples / (hConfig->nFrameSamples) + nTrashAUs + 1;
    }

    if (!isError(retValue)) {
      for (i = 0; i < intvlMinFrames; i++) {
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, rapInXFrames + i, &tmp_syncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
        if (tmp_syncFrame) {
          int syncPos = rapInXFrames + i;
          if (abs(syncPos - rapInXFrames) < intvlMinFrames && syncPos != rapInXFrames) {
            retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_CLOSE);

            retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE;
            break;
          }
        }
      }
    }

    if (!isError(retValue)) {
      for (i = 0; i < intvlMinFrames && i < rapInXFrames; i++) {
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, rapInXFrames - i, &tmp_syncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
        if (tmp_syncFrame) {
          int syncPos = rapInXFrames - i;
          if (abs(syncPos - rapInXFrames) < intvlMinFrames && syncPos != rapInXFrames) {
            retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_CLOSE);

            retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE;
            break;
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (lastRapFrameDist > 0) {
      int rapMinSamplesInAdvance = 0;
      PARAM_INSTANCE_HANDLE hParamRapMinSamplesInAdvance = NULL;
      HANDLE_ERROR_INFO errorInfo = noError;

      errorInfo = iisParamListGetParam(hCodecParamList, PARAMLIST_PARAMETER_RAP_MIN_SAMPLES_IN_ADVANCE, &hParamRapMinSamplesInAdvance);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_PARAM_LIST;
      }
      if (hParamRapMinSamplesInAdvance) rapMinSamplesInAdvance = hParamRapMinSamplesInAdvance->paramValue._int;

      if (!isError(retValue)) {
        if (rapInXSamples < (unsigned int)hConfig->randomAccessIntervalMin - (unsigned int)lastRapFrameDist) {
          retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_CLOSE);

          retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE;
        }
        if (rapInXSamples < (unsigned int)rapMinSamplesInAdvance) {
          retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_SOON);

          retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_SOON;
        }
      }
    }
  }

  if (!isError(retValue)) {
    rapInXFrames = rapInXSamples / (hConfig->nFrameSamples) + nTrashAUs + 1;
    retValue = iisxHEAACEncLib_rapOnDemand_SetupRapSyncInfo(hConfig, hLoaswriter, audioPreRoll->hAudioPreRoll, &rapSyncInfo);
  }

  if (!isError(retValue)) {
    int i;
    for (i = 0; i < rapSyncInfo.nSyncFrameType && !isError(retValue); i++) {
      if (!isError(retValue)) {
        retValue = iisxHEAACEncLib_syncFrame_GetDelay(hUsacIndepFlag, &syncFrameDelay, rapSyncInfo.frame_type[i].syncFrame);
      }

      if (!isError(retValue)) {
        int allowShortestRapInterval = 0;

        if (iisParamListParamExists(hCodecParamList, PARAMLIST_PARAMETER_ALLOW_SHORTEST_RAP_INTERVAL)) {
          allowShortestRapInterval = 1;
        }

        if (!allowShortestRapInterval) {
          int j;
          int nAUsInPreRoll = 0;

          if (audioPreRoll->hAudioPreRoll != NULL) {
            nAUsInPreRoll = iisAudioPreRollLibGetnPreRollAu(audioPreRoll->hAudioPreRoll);
          }

          if (rapSyncInfo.frame_type[i].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_IPF]) {
            for (j = 1; !isError(retValue) && j <= nAUsInPreRoll; j++) {
              int tmp_syncFrame = 0;

              if (rapInXFrames + rapSyncInfo.frame_type[i].diffToSyncFrame - j >= 0) {
                retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, rapInXFrames + rapSyncInfo.frame_type[i].diffToSyncFrame - j, &tmp_syncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
                if (tmp_syncFrame) {
                  retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_CLOSE);

                  retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_CLOSE;
                }
              }
            }
          }
        }
      }

      if (!isError(retValue)) {
        retValue = iisxHEAACEncLib_syncFrame_ForceSyncFrame(hUsacIndepFlag, rapInXFrames + rapSyncInfo.frame_type[i].diffToSyncFrame, rapSyncInfo.frame_type[i].syncFrame);
        if (isError(retValue)) {
          if (syncFrameDelay > rapInXFrames + rapSyncInfo.frame_type[i].diffToSyncFrame) {
            retValue = iisxHEAACEncLib_warnings_submit(warningList, XHEAACENCLIB_WARN_RAP_TOO_SOON);

            retValue = XHEAACENCLIB_RETURN_ERROR_RAP_TOO_SOON;
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (hConfig->rapOnDemandAdvancedMode) {
      if ((audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL) && audioPreRoll->hAudioPreRoll != NULL && hConfig->aot == AUD_OBJ_TYP_USAC && (hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE || hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SEEKABLE)) {
        int raIntCoreSampleRate = 0;
        int ebrFillRateSamples = 0;

        if (!isError(retValue)) {
          if (hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) {
            ebrFillRateSamples = max(min((unsigned int)INT_MAX, rapInXSamples), (unsigned int)hConfig->randomAccessIntervalMin);
            raIntCoreSampleRate = (ebrFillRateSamples * hConfig->sbrRatio.downFac) / hConfig->sbrRatio.upFac;
          } else {
            retValue = XHEAACENCLIB_RETURN_ERROR_WRONG_RAP_OCCURRENCE;
          }
        }

        ebrParams->rapIntCoreSampleRate = raIntCoreSampleRate;

        if (!isError(retValue)) {
          if (!isVbr(hConfig->bitrateMode)) {
            retValue = iisxHEAACEncLibAacEncUpdateExtendedBitReservoir(hAacEnc, ebrParams->nBitsAuPreRoll, raIntCoreSampleRate, ebrParams->preRollAUFactor);
          }
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_rapOnDemand_SetupRapSyncInfo(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAACENCLIB_RAP_SYNC_INFO_HANDLE const rapSyncInfo) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nAUsInPreRoll = 0;

  if (hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    memset(rapSyncInfo, 0, sizeof(XHEAACENCLIB_RAP_SYNC_INFO));

    rapSyncInfo->nSyncFrameType = 1;
    rapSyncInfo->frame_type[0].diffToSyncFrame = 0;
  }

  if (!isError(retValue)) {
    if (hLoaswriter != NULL) {
      rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC] = 1;
    }
  }

  if (!isError(retValue)) {
    rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_IS_RAP] = 1;
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      switch (hConfig->rapProperty) {
        case XHEAACENCLIB_RAP_PROPERTY_ACCESS:
          rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP] = 1;
          break;
        case XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE:
        case XHEAACENCLIB_RAP_PROPERTY_SEEKABLE:
          if (hAudioPreRoll == NULL) {
            retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
          }
          if (!isError(retValue)) {
            nAUsInPreRoll = iisAudioPreRollLibGetnPreRollAu(hAudioPreRoll);
          }
          if (!isError(retValue)) {
            rapSyncInfo->nSyncFrameType = 2;
            rapSyncInfo->frame_type[1].diffToSyncFrame = -1 * nAUsInPreRoll;
            rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP] = 1;
            rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_IPF] = 1;
            rapSyncInfo->frame_type[1].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP] = 1;
            if (hConfig->bUseSBR) {
              rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER] = 1;
              rapSyncInfo->frame_type[1].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER] = 1;
            }
          }
          break;
        case XHEAACENCLIB_RAP_PROPERTY_OFF:

          rapSyncInfo->nSyncFrameType = 0;
          break;
        case XHEAACENCLIB_RAP_PROPERTY_INVALID:
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_RAP_PROPERTY;
          break;
      }
    } else {
      switch (hConfig->rapProperty) {
        case XHEAACENCLIB_RAP_PROPERTY_ACCESS:
        case XHEAACENCLIB_RAP_PROPERTY_SEEKABLE:
          if (hConfig->bUseSBR && hConfig->aot != AUD_OBJ_TYP_LC) {
            rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER] = 1;
          }
          break;
        case XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE:
          rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_CORE_LOW_OVERLAP] = 1;
          rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_CORE_HIGH_BW] = 0;
          if (hConfig->bUseSBR) {
            rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER] = 1;
            rapSyncInfo->frame_type[0].syncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER] = 1;
          }
          break;
        case XHEAACENCLIB_RAP_PROPERTY_OFF:

          rapSyncInfo->nSyncFrameType = 0;
          break;
        case XHEAACENCLIB_RAP_PROPERTY_INVALID:
        default:
          retValue = XHEAACENCLIB_RETURN_ERROR_RAP_PROPERTY;
          break;
      }
    }
  }
  return retValue;
}
