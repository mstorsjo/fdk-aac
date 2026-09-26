
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "lpd_wrapper.h"
#include "iisBitFrame.h"
#include "../../iisLPDComLib/include/options.h"
#include "iisLPDEncLib.h"
#include "qc_data.h"
#include "iisSigMap.h"
#include "bit_enc.h"
#include "time_buffer.h"
#include "glob_con.h"
#include "extpayload.h"
#include "psy_const.h"
#include "psy_data.h"
#include "bit_aac.h"
#include "iisutillib.h"

#define UNUSED_IN_FDONLY

struct lpdWrapper_tag {
  int nChannels;
  int nElements;
  ELEMENT_INFO pElInfo[SIGMAP_MAX_ELEMENTS];
  LPDENCLIB_CONFIG amrwbpConfig;
  int nGranuleLength;
  LPD_WRAPPER_CODEC_TYPE codecType;
  int sampleRate;
  int totalBitRate;
  int isVbr;
  int useNoiseFilling;
  LPD_ENC_PUBLIC_DATA_HANDLE phAcelp[SIGMAP_MAX_SIGNALS];
};

static LPD_WRAPPER_ERROR mappingIpfStateFromLpdWrapperToLpd(
    LPD_WRAPPER_IPF_STATE ipfStateLpdWrapper,
    LPD_IPF_STATE *pMappedIpfStateLpd);

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_open(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE *phLpdWrapper,
    UNUSED_IN_FDONLY LPD_WRAPPER_SETUP_HANDLE hLpdWrapperSetup) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;
  int el = 0;
  int ch = 0;
  int chCnt = 0;

  *phLpdWrapper = (LPD_WRAPPER *)iisCalloc(sizeof(LPD_WRAPPER), 1);

  if (*phLpdWrapper != NULL) {
    (*phLpdWrapper)->nChannels = hLpdWrapperSetup->nChannels;
    (*phLpdWrapper)->nElements = hLpdWrapperSetup->nElements;

    for (el = 0; el < (*phLpdWrapper)->nElements; el++) {
      (*phLpdWrapper)->pElInfo[el] = *hLpdWrapperSetup->phElInfo[el];
    }

    (*phLpdWrapper)->nGranuleLength = hLpdWrapperSetup->nGranuleLength;
    (*phLpdWrapper)->codecType = hLpdWrapperSetup->codecType;
    (*phLpdWrapper)->sampleRate = hLpdWrapperSetup->sampleRate;
    (*phLpdWrapper)->totalBitRate = hLpdWrapperSetup->totalBitRate;
    (*phLpdWrapper)->isVbr = hLpdWrapperSetup->isVbr;
    (*phLpdWrapper)->useNoiseFilling = hLpdWrapperSetup->useNoiseFilling;

    (*phLpdWrapper)->amrwbpConfig.fscale = (*phLpdWrapper)->sampleRate;

    (*phLpdWrapper)->amrwbpConfig.mode = hLpdWrapperSetup->acelpModeIndex;
    (*phLpdWrapper)->amrwbpConfig.FileFormat = FRAW;
    (*phLpdWrapper)->amrwbpConfig.bc = 0;
    (*phLpdWrapper)->amrwbpConfig.L_next = 64;
    (*phLpdWrapper)->amrwbpConfig.L_next_st = 64;
    (*phLpdWrapper)->amrwbpConfig.reduced_bitstream = 0;
    (*phLpdWrapper)->amrwbpConfig.totalBitRate = hLpdWrapperSetup->totalBitRate;
    (*phLpdWrapper)->amrwbpConfig.isVbr = hLpdWrapperSetup->isVbr;
    (*phLpdWrapper)->amrwbpConfig.L_frame = hLpdWrapperSetup->nGranuleLength;
    (*phLpdWrapper)->amrwbpConfig.optimizedSpeedPulseSearch = hLpdWrapperSetup->optimizedSpeedPulseSearch;
    switch (hLpdWrapperSetup->codingMode) {
      case LPD_WRAPPER_CODING_MODE_INVALID:
        (*phLpdWrapper)->amrwbpConfig.codingMode = LPD_CODING_MODE_INVALID;
        break;
      case LPD_WRAPPER_CODING_MODE_SWITCHED:
        (*phLpdWrapper)->amrwbpConfig.codingMode = LPD_CODING_MODE_SWITCHED;
        break;
      case LPD_WRAPPER_CODING_MODE_ACELP:
        (*phLpdWrapper)->amrwbpConfig.codingMode = LPD_CODING_MODE_ACELP;
        break;
      case LPD_WRAPPER_CODING_MODE_TCX:
        (*phLpdWrapper)->amrwbpConfig.codingMode = LPD_CODING_MODE_TCX;
        break;
      default:
        break;
    }

    if (hLpdWrapperSetup->codingMode != LPD_WRAPPER_CODING_MODE_INVALID) {
      for (el = 0; el < (*phLpdWrapper)->nElements; el++) {
        ELEMENT_INFO elInfo = (*phLpdWrapper)->pElInfo[el];

        for (ch = 0; ch < elInfo.nChannelsInEl; ch++) {
          HANDLE_ERROR_INFO lpd_error = NULL;
          lpd_error = iisLPDEncLib_Open(&(*phLpdWrapper)->phAcelp[chCnt],
                                        0,
                                        0);
          if (lpd_error) {
            errorLPD = LPD_WRAPPER_INIT_ERROR;
            break;
          }

          (*phLpdWrapper)->amrwbpConfig.lpdChannelBitRate = 0;
          lpd_error = iisLPDEncLib_Config((*phLpdWrapper)->phAcelp[chCnt], &(*phLpdWrapper)->amrwbpConfig);
          if (lpd_error) {
            errorLPD = LPD_WRAPPER_INIT_ERROR;
            break;
          }
          chCnt++;
        }
      }
    }
  }

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_process(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int bitReservoir,
    UNUSED_IN_FDONLY int bitReservoirMax,
    UNUSED_IN_FDONLY HANDLE_BITSTREAM_ENC hBsEnc,
    UNUSED_IN_FDONLY PSY_DATA *psyData[],
    UNUSED_IN_FDONLY int el,
    UNUSED_IN_FDONLY INTERN_CORE_MODE *coreMode,
    UNUSED_IN_FDONLY INTERN_CORE_MODE *coreModePrev,
    UNUSED_IN_FDONLY INTERN_CORE_MODE *coreModeNext,
    UNUSED_IN_FDONLY int maxBitsToUse,
    UNUSED_IN_FDONLY const int bUsacIndepFlag,
    UNUSED_IN_FDONLY const LPD_WRAPPER_IPF_STATE ipfState) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;
  int ch;
  HANDLE_ERROR_INFO err = NULL;

  ELEMENT_INFO *elInfo = &hLpdWrapper->pElInfo[el];
  int *chIdx = elInfo->ChannelIndex;
  LPD_IPF_STATE mappedIpfState = LPD_IPF_STATE_NO;

  if (errorLPD == LPD_WRAPPER_NO_ERROR) {
    errorLPD = mappingIpfStateFromLpdWrapperToLpd(ipfState, &mappedIpfState);
  }

  for (ch = 0; ch < elInfo->nChannelsInEl; ch++) {
    if (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) {
      float const *pAcelpTimeSignal = NULL;
      float const *pAMRPastSignal = NULL;
      int aceStartFlag = ((coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD && coreModePrev[chIdx[ch]] == INTERN_CORE_MODE_FD)) ? 1 : 0;
      int useNoiseFilling = (elInfo->elType == ID_LFE) ? 0 : hLpdWrapper->useNoiseFilling;
      const int acelpOffset = hLpdWrapper->nGranuleLength / 2;
      const int nPastAcelpOffset = acelpOffset - (hLpdWrapper->nGranuleLength / 4);
      const int lastWasShort = (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) && (coreModePrev[chIdx[ch]] == INTERN_CORE_MODE_FD) && (psyData[ch]->blockSwitchingControl.prevWindowSequence == SHORT_WINDOW);
      const int nextIsShort = (coreMode[chIdx[ch]] == INTERN_CORE_MODE_LPD) && (coreModeNext[chIdx[ch]] == INTERN_CORE_MODE_FD) && (psyData[ch]->blockSwitchingControl.nextWindowSequence == SHORT_WINDOW);

      iisLPDEncLib_SetFdOverlapLen(hLpdWrapper->phAcelp[chIdx[ch]],
                                   lastWasShort,
                                   nextIsShort);

      pAcelpTimeSignal = MP4TIMEBUF_AccessBuffer(psyData[ch]->psyInputBuffer,
                                                 acelpOffset,
                                                 0);
      pAMRPastSignal = MP4TIMEBUF_AccessBuffer(psyData[ch]->psyInputBuffer,
                                               nPastAcelpOffset,
                                               0);
      if (!(hLpdWrapper->isVbr)) {
        iisLPDEncLib_UpdateBitRes(hLpdWrapper->phAcelp[chIdx[ch]],
                                  bitReservoir,
                                  bitReservoirMax);
      }
      {
        if (errorLPD == LPD_WRAPPER_NO_ERROR) {
          err = iisLPDEncLib_EncodeFrame(hLpdWrapper->phAcelp[chIdx[ch]],
                                         pAcelpTimeSignal,
                                         pAMRPastSignal,
                                         aceStartFlag,
                                         bUsacIndepFlag,
                                         useNoiseFilling,
                                         getArithEncoder(hBsEnc->bitEncChannelData[chIdx[ch]].hEncSpecDataArith),
                                         maxBitsToUse,
                                         mappedIpfState

          );
        }

        if (err != noError) {
          errorLPD = LPD_WRAPPER_UNKNOWN_ERROR;
          freeErrorTraceback(err);
        }
      }

    } else {
      hLpdWrapper->phAcelp[chIdx[ch]]->useLpd = 0;
    }
  }

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_fac_process(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int ch0,
    UNUSED_IN_FDONLY int ch,
    UNUSED_IN_FDONLY PSY_OUT_CHANNEL *psyOutChannel,
    UNUSED_IN_FDONLY QC_OUT_CHANNEL *qcOutChannel,
    UNUSED_IN_FDONLY INTERN_CORE_MODE coreModePrev,
    UNUSED_IN_FDONLY INTERN_CORE_MODE coreModeNext,
    UNUSED_IN_FDONLY char *facPrm,
    UNUSED_IN_FDONLY int *Nbits_fac) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  int lfac = 0;
  int lowpassLine = 0;
  int nGranuleLength = 0;
  LPDENC_TRANSITION_TYPE transitionType = LPDENC_TRANSITION_UNDEFINED;

  *Nbits_fac = 0;
  nGranuleLength = psyOutChannel->granuleLength;
  lfac = psyOutChannel->windowSequence == SHORT_WINDOW
             ? (nGranuleLength / (2 * TRANS_FAC))
             : (nGranuleLength / TRANS_FAC);

  lowpassLine = (int)((float)psyOutChannel->sfbOffsets[psyOutChannel->sfbActive] * (float)lfac / (float)nGranuleLength);

  if (coreModePrev == INTERN_CORE_MODE_LPD && coreModeNext == INTERN_CORE_MODE_LPD) {
    transitionType = LPDENC_TRANSITION_LPD2FD2LPD;
  } else if (coreModePrev == INTERN_CORE_MODE_LPD) {
    transitionType = LPDENC_TRANSITION_LPD2FD;
  } else if (coreModeNext == INTERN_CORE_MODE_LPD) {
    transitionType = LPDENC_TRANSITION_FD2LPD;
  }

  iisLPDEncLib_FdLpdTransition(hLpdWrapper->phAcelp[ch0],
                               ch,
                               psyOutChannel->origTimeSig,
                               qcOutChannel->synthTime,
                               nGranuleLength,
                               lowpassLine,
                               lfac,
                               transitionType,
                               facPrm,
                               Nbits_fac);

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_set_restricted_mode(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int ch,
    UNUSED_IN_FDONLY int mode) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  iisLPDEncLib_SetRestrictedMode(hLpdWrapper->phAcelp[ch],
                                 mode);

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_transfer_acelp_data(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int ch,
    UNUSED_IN_FDONLY unsigned char *acelpData,
    UNUSED_IN_FDONLY int *acelpDataBitCnt) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  if (hLpdWrapper->phAcelp[ch]->useLpd != 0) {
    memcpy(acelpData,
           hLpdWrapper->phAcelp[ch]->outStream,
           (hLpdWrapper->phAcelp[ch]->outputStreamLenBits + 7) / 8);

    *acelpDataBitCnt = hLpdWrapper->phAcelp[ch]->outputStreamLenBits;

  } else {
    *acelpDataBitCnt = 0;
  }

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_transfer_fac_data(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int ch,
    UNUSED_IN_FDONLY unsigned char *facData,
    UNUSED_IN_FDONLY int *facDataBitCnt) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  if (hLpdWrapper->phAcelp[ch]->modes[0] == 0) {
    memcpy(facData,
           hLpdWrapper->phAcelp[ch]->facData,
           (hLpdWrapper->phAcelp[ch]->nBitsFac + 7) / 8);

    *facDataBitCnt = hLpdWrapper->phAcelp[ch]->nBitsFac;
  } else {
    *facDataBitCnt = 0;
  }

  return errorLPD;
}

int iisaacfenc_wrap_lpd_last_sub_frame_was_lpd(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY int ch) {
  int lastSubFrameWasLpd = 0;

  lastSubFrameWasLpd = iisLPDEncLib_GetLastSubfrWasACELP(hLpdWrapper->phAcelp[ch]);

  return lastSubFrameWasLpd;
}

LPD_WRAPPER_ERROR iisaacfenc_wrap_lpd_close(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;
  int i = 0;

  for (i = 0; i < hLpdWrapper->nChannels; i++) {
    if (hLpdWrapper->phAcelp[i]) {
      iisLPDEncLib_Close(hLpdWrapper->phAcelp[i]);
      hLpdWrapper->phAcelp[i] = NULL;
    }
  }

  iisFree(hLpdWrapper);

  return errorLPD;
}

LPD_WRAPPER_ERROR iisaacfenc_calculate_max_bits_to_use(
    UNUSED_IN_FDONLY LPD_WRAPPER_HANDLE hLpdWrapper,
    UNUSED_IN_FDONLY HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
    UNUSED_IN_FDONLY int numExtContainer,
    UNUSED_IN_FDONLY int el,
    UNUSED_IN_FDONLY unsigned int nBitsTransportOverhead,
    UNUSED_IN_FDONLY int maximumNumberOfBitsForThisFrame,
    UNUSED_IN_FDONLY unsigned int *maxBitsToUse) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  if (hLpdWrapper == NULL) {
    return LPD_WRAPPER_INIT_ERROR;
  }

  *maxBitsToUse = maximumNumberOfBitsForThisFrame;
  if (hExtContainer != NULL) {
    int i = 0;
    for (i = 0; i < numExtContainer; i++) {
      if (!hasExtensionPayloadContainerFeature(hExtContainer[i], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
        *maxBitsToUse -= getTotalSize_extPayload(hExtContainer[i]);
      }
    }
  }

  *maxBitsToUse -= nBitsTransportOverhead;
  *maxBitsToUse -= 2;
  *maxBitsToUse -= hLpdWrapper->pElInfo[el].nChannelsInEl;

  return errorLPD;
}

static LPD_WRAPPER_ERROR mappingIpfStateFromLpdWrapperToLpd(
    LPD_WRAPPER_IPF_STATE ipfStateLpdWrapper,
    LPD_IPF_STATE *pMappedIpfStateLpd) {
  LPD_WRAPPER_ERROR errorLPD = LPD_WRAPPER_NO_ERROR;

  if (errorLPD == LPD_WRAPPER_NO_ERROR) {
    if (pMappedIpfStateLpd == NULL) {
      errorLPD = LPD_WRAPPER_INVALID_POINTER;
    }
  }

  if (errorLPD == LPD_WRAPPER_NO_ERROR) {
    switch (ipfStateLpdWrapper) {
      case LPD_WRAPPER_IPF_STATE_NO:
        *pMappedIpfStateLpd = LPD_IPF_STATE_NO;
        break;
      case LPD_WRAPPER_IPF_STATE_RAP_FIRST_PREROLL:
        *pMappedIpfStateLpd = LPD_IPF_STATE_RAP_FIRST_PREROLL;
        break;
      case LPD_WRAPPER_IPF_STATE_RAP_NEXT_PREROLL:
        *pMappedIpfStateLpd = LPD_IPF_STATE_RAP_NEXT_PREROLL;
        break;
      case LPD_WRAPPER_IPF_STATE_RAP_IPF:
        *pMappedIpfStateLpd = LPD_IPF_STATE_RAP_IPF;
        break;
      case LPD_WRAPPER_IPF_STATE_RAP_IPF_PREROLL:
        *pMappedIpfStateLpd = LPD_IPF_STATE_RAP_IPF_PREROLL;
        break;
      case LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL:
        *pMappedIpfStateLpd = LPD_IPF_STATE_CONFIGCHANGE_FIRST_PREROLL;
        break;
      case LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL:
        *pMappedIpfStateLpd = LPD_IPF_STATE_CONFIGCHANGE_NEXT_PREROLL;
        break;
      case LPD_WRAPPER_IPF_STATE_CONFIGCHANGE_IPF:
        *pMappedIpfStateLpd = LPD_IPF_STATE_CONFIGCHANGE_IPF;
        break;
      default:
        *pMappedIpfStateLpd = LPD_IPF_STATE_NO;
        errorLPD = LPD_WRAPPER_ILLEGAL_PARAMETER;
        break;
    }
  }
  return errorLPD;
}
