
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

#include "iisxHEAACEncLib_audioPreRoll.h"

XHEAACENCLIB_RETURN iisxHEAACEncLib_getIpfState(
    XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    XHEAAC_AACENC_IPF_STATE* const ipfState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN retValueApr = AUDIOPREROLLLIB_NO_ERROR;

  unsigned int preRollDistToIpf = 0;
  unsigned int nIpfDelay = 0;
  int ipfRequest = 0;

  if (hAudioPreRoll == NULL || ipfState == NULL || hSyncFrame == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *ipfState = XHEAAC_AACENC_IPF_STATE_NO;

    retValueApr = iisAudioPreRollLibGetDelays(hAudioPreRoll, &nIpfDelay);
    if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL;
    }
  }

  if (!isError(retValue)) {
    for (preRollDistToIpf = 0; preRollDistToIpf <= nIpfDelay; preRollDistToIpf++) {
      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hSyncFrame, preRollDistToIpf, &ipfRequest, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);

      if (!isError(retValue)) {
        if (ipfRequest) {
          if (preRollDistToIpf == 0) {
            *ipfState = XHEAAC_AACENC_IPF_STATE_RAP_IPF;
          }

          if (preRollDistToIpf < nIpfDelay && preRollDistToIpf > 0) {
            if (*ipfState != XHEAAC_AACENC_IPF_STATE_RAP_IPF) {
              *ipfState = XHEAAC_AACENC_IPF_STATE_RAP_NEXT_PREROLL;
            } else {
              *ipfState = XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL;
            }
          }

          if (preRollDistToIpf == nIpfDelay) {
            if (*ipfState != XHEAAC_AACENC_IPF_STATE_RAP_IPF && *ipfState != XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL) {
              *ipfState = XHEAAC_AACENC_IPF_STATE_RAP_FIRST_PREROLL;
            }
          }
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_configure(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_AUDIOPREROLL_DATA* const audioPreRoll,
    int const mpeg4StandDelay,
    unsigned int* const minOutBufSize) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN audioPreRollError = AUDIOPREROLLLIB_NO_ERROR;

  if (hConfig == NULL || audioPreRoll == NULL || minOutBufSize == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    AUDIOPREROLLLIB_SETUP audioPreRollSetup = {0};

    if ((hConfig->aot == AUD_OBJ_TYP_USAC && hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SWITCHABLE) ||
        (hConfig->aot == AUD_OBJ_TYP_USAC && hConfig->rapProperty == XHEAACENCLIB_RAP_PROPERTY_SEEKABLE) ||
        (hConfig->aot == AUD_OBJ_TYP_USAC && hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE)) {
      if (hConfig->audioPreRollNumAU < 0) {
        audioPreRollSetup.nPreRollAu = (mpeg4StandDelay + (hConfig->nFrameSamples - 1)) / hConfig->nFrameSamples;
      } else {
        audioPreRollSetup.nPreRollAu = hConfig->audioPreRollNumAU;
      }

      audioPreRollSetup.nChannels = hConfig->nChannelsCoreCoder;
      audioPreRoll->bitResMode = hConfig->audioPreRollBitResMode;

      if (audioPreRollSetup.nPreRollAu < 0 || audioPreRollSetup.nPreRollAu > 3) {
        retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INVALID;
      }

      if (!isError(retValue)) {
        audioPreRollError = iisAudioPreRollLibNew(&audioPreRoll->hAudioPreRoll, &audioPreRollSetup);

        if (audioPreRollError >= AUDIOPREROLLLIB_ERROR_FIRST) {
          retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE;
        } else if (audioPreRollError != AUDIOPREROLLLIB_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_INSTANCE_WR;
        }
      }
    } else {
      audioPreRoll->hAudioPreRoll = NULL;
    }
  }

  if (!isError(retValue)) {
    if (audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT) {
      *minOutBufSize *= iisAudioPreRollLibGetnPreRollAu(audioPreRoll->hAudioPreRoll) + 1;
      *minOutBufSize += iisAudioPreRollLibPayloadSizeWithoutAUs(audioPreRoll->hAudioPreRoll);
    }
  }

  if (!isError(retValue)) {
    if (audioPreRoll->PreRollAUBuffer) {
      iisFree(audioPreRoll->PreRollAUBuffer);
      audioPreRoll->PreRollAUBuffer = NULL;
    }
    if (audioPreRoll->hAudioPreRoll) {
      audioPreRoll->PreRollAUBufferSizeInBytes = *minOutBufSize;
      audioPreRoll->PreRollAUBuffer = (unsigned char*)iisMalloc(audioPreRoll->PreRollAUBufferSizeInBytes);

      if (audioPreRoll->PreRollAUBuffer == NULL) {
        audioPreRoll->PreRollAUBufferSizeInBytes = 0;
      }
    }
    audioPreRoll->nPreRollAUBufferBytes = 0;
  }

  if ((!isError(retValue)) && (audioPreRoll->hAudioPreRoll)) {
    ASC_BUFFER* usacConfig = (ASC_BUFFER*)iisCalloc(MAX_LAYERS, sizeof(ASC_BUFFER));

    HANDLE_ERROR_INFO errorInfo = noError;
    errorInfo = IIS_Mp4ASC_AotSpecificConfigGet(hConfig->hCoderConfig, usacConfig);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_ASC;
    }

    if (!isError(retValue)) {
      audioPreRollError = iisAudioPreRollLibWriteConfig(audioPreRoll->hAudioPreRoll,
                                                        usacConfig[0].asc,
                                                        (usacConfig[0].ascSize + 7) / 8,
                                                        0);
      if (AUDIOPREROLLLIB_NO_ERROR != audioPreRollError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CONFIG;
      }
    }

    iisFree(usacConfig);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_request(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_AUDIOPREROLL_DATA const* const audioPreRoll,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    int* const lastRapFrameDist,
    int* const bForceIndepFlag,
    XHEAAC_AACENC_IPF_STATE* const ipfState) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN retValueApr = AUDIOPREROLLLIB_NO_ERROR;
  int indepFrame = 0;
  unsigned int nIpfDelay = 0;
  int ipfRequest = 0;

  if (hConfig == NULL || hUsacIndepFlag == NULL || audioPreRoll == NULL || ipfState == NULL ||
      lastRapFrameDist == NULL || bForceIndepFlag == NULL || audioPreRoll->hAudioPreRoll == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *bForceIndepFlag = 0;
    *ipfState = XHEAAC_AACENC_IPF_STATE_NO;

    retValueApr = iisAudioPreRollLibGetDelays(audioPreRoll->hAudioPreRoll, &nIpfDelay);
    if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL;
    }

    if (!isError(retValue)) {
      if (hConfig->aot == AUD_OBJ_TYP_USAC) {
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, nIpfDelay, &ipfRequest, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
      }
    }

    if (!isError(retValue)) {
      if (ipfRequest) {
        retValueApr = iisAudioPreRollLibRequest(audioPreRoll->hAudioPreRoll);
        if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL;
        }
      }
    }

    if (!isError(retValue)) {
      retValue = iisxHEAACEncLib_getIpfState(hUsacIndepFlag, audioPreRoll->hAudioPreRoll, ipfState);
    }

    if (!isError(retValue)) {
      if (*ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF || *ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL) {
        *lastRapFrameDist = 0;
        *lastRapFrameDist = hConfig->nFrameSamples;

      } else if (*lastRapFrameDist > 0 && *lastRapFrameDist < hConfig->randomAccessIntervalMin) {
        *lastRapFrameDist += hConfig->nFrameSamples;
      }
    }

    if (!isError(retValue)) {
      if (hConfig->aot == AUD_OBJ_TYP_USAC) {
        retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, &indepFrame, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
      }
    }

    if (!isError(retValue)) {
      if (*ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF || *ipfState == XHEAAC_AACENC_IPF_STATE_RAP_IPF_PREROLL || *ipfState == XHEAAC_AACENC_IPF_STATE_RAP_FIRST_PREROLL) {
        *bForceIndepFlag = 1;
        if (indepFrame != 1) {
          retValue = XHEAACENCLIB_RETURN_ERROR_IPF_NONINDEPENDENT_FRAME;
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_writeExtPayload(
    XHEAACENCLIB_AUDIOPREROLL_DATA* const audioPreRoll,
    XHEAACENCLIB_BD_DATA* const bitDistribution,
    XHEAACENCLIB_SYNCFRAME_HANDLE const hUsacIndepFlag,
    XHEAACENCLIB_EXT_ELEMENT_LIST* const extEleList) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  unsigned char* preRollData = NULL;
  int nPreRollBytes = 0;
  int bPreRollDataAvailable = 0;
  int maxBytesToUse = -1;
  int maxNumBitsAllowed = 0;
  int comfortableNumBitsAllowed = 0;
  unsigned int tmp_ExtensionPayloadSize = 0;
  int IPF_outBitRes = 0;

  if ((hUsacIndepFlag == NULL) || (bitDistribution == NULL) || (extEleList == NULL) || (audioPreRoll == NULL) || (audioPreRoll->hAudioPreRoll == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_IN_FAILSAVE_DUMP_PREROLL) {
      retValue = iisxHEAACEncLib_bitDistribution_GetThresholds(bitDistribution->hBitDistribution, bitDistribution->elementID_APR, &maxNumBitsAllowed, &comfortableNumBitsAllowed);
      if (!isError(retValue)) {
        maxBytesToUse = maxNumBitsAllowed - 50;
        maxBytesToUse /= 8;
      }
    }

    if (!isError(retValue)) {
      int isSyncFrame = 0;
      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, 0, &isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
      if (isSyncFrame && iisAudioPreRollLibGetAuPreRollData(audioPreRoll->hAudioPreRoll, &preRollData, &nPreRollBytes, maxBytesToUse, &bPreRollDataAvailable) >= AUDIOPREROLLLIB_ERROR_FIRST) {
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD_AUDIO_PREROLL;
      }
    }
  }
  if (!isError(retValue)) {
    if (bPreRollDataAvailable) {
      if (audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT) {
        IPF_outBitRes = 1;
      } else {
        IPF_outBitRes = 0;
      }
      retValue = iisxHEAACEncLib_extentsionData_usacSubmit(extEleList, XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL, preRollData, nPreRollBytes * 8, 0, IPF_outBitRes);
      audioPreRoll->transmitPayload = XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_ACT_FRAME;
      if (!isError(retValue) && audioPreRoll->bFirstIpfSend == 0) {
        audioPreRoll->bFirstIpfSend = 1;
        if (iisAudioPreRollLibChangeCrossfadeFlag(audioPreRoll->hAudioPreRoll, 1) != AUDIOPREROLLLIB_NO_ERROR) {
          retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_CROSSOVER_FLAG_CHANGE;
        }
      }
    } else {
      if (audioPreRoll->bitResMode == XHEAACENCLIB_APR_BITRESMODE_OUT) {
        IPF_outBitRes = 1;
      } else {
        IPF_outBitRes = 0;
      }
      retValue = iisxHEAACEncLib_extentsionData_usacSubmit(extEleList, XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL, NULL, 0, 0, IPF_outBitRes);
      audioPreRoll->transmitPayload = XHEAACENCLIB_AUDIOPREROLL_TRANSMIT_DATA_NO;
    }
  }

  if (!isError(retValue)) {
    if (IPF_outBitRes == 0) {
      retValue = iisxHEAACEncLib_extensionData_GetExtensionLength(extEleList, XHEAACENCLIB_EXT_ELE_AUDIO_PRE_ROLL, 0, &tmp_ExtensionPayloadSize);
    } else {
      tmp_ExtensionPayloadSize = 0;
    }
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLib_bitDistribution_SetUsedThisFrame(bitDistribution->hBitDistribution, bitDistribution->elementID_APR, (int)tmp_ExtensionPayloadSize);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_audioPreRoll_postEncode(
    XHEAACENCLIB_AUDIOPREROLL_DATA* const audioPreRoll) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN retValueApr = AUDIOPREROLLLIB_NO_ERROR;

  if (audioPreRoll == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (audioPreRoll->hAudioPreRoll == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if ((audioPreRoll->nPreRollAUBufferBits % 8) != 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_BYTE_ALIGNMENT;
    }
  }

  if (!isError(retValue)) {
    audioPreRoll->nPreRollAUBufferBytes = (audioPreRoll->nPreRollAUBufferBits + 7) / 8;
  }

  if (!isError(retValue)) {
    if (audioPreRoll->PreRollAUBuffer == NULL || audioPreRoll->nPreRollAUBufferBytes < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_NO_DATA;
    }
  }

  if (!isError(retValue)) {
    retValueApr = iisAudioPreRollLibCollectAccessUnit(audioPreRoll->hAudioPreRoll, audioPreRoll->PreRollAUBuffer, audioPreRoll->nPreRollAUBufferBytes);
    if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
    }
  }

  return retValue;
}
