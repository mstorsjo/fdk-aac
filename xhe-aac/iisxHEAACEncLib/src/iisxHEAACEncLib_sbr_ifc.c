
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

#include <stdlib.h>
#include <limits.h>

#include <assert.h>

#include "mathlib.h"

#include "iisxHEAACEncLib_sbr_ifc.h"
#include "iisSigMap.h"
#include "cmondata.h"
#include "sbr_main.h"
#include "time_buffer.h"
#include "iis_fft.h"
#include "iisSwitchingDecision.h"
#include "iisxHEAACEncLib_SigMap_ifc.h"
#include "iisxHEAACEncLib_common.h"

#define MAX_DELAY_FRAMES 10

struct SbrEncEncoder {
  int nFrameLengthCore;
  int nFrameLengthSbr;
  int sampleRate;
  int bUseDownsampledSbr;
  int chBitRate;
  unsigned int useSpeechConfig;
  unsigned int overrideTimeDiffCoding;
  PS_MODE PSMode;

  CHANNEL_MAPPING *cm;
  sbrConfiguration *sbrSettings[SIGMAP_MAX_ELEMENTS];
  int bUseCRC;
  float sendHeaderTimeInterval;
  int sendHeaderDelay;

  HANDLE_MP4TIMEBUF hInputBuffer[SIGMAP_MAX_ELEMENTS];
  int nQmfSlotsDelay[SIGMAP_MAX_ELEMENTS];
  int nQmfSimSpace[SIGMAP_MAX_ELEMENTS];
  int nQmfSlotsTotal[SIGMAP_MAX_ELEMENTS];
  int nAdditionalOutputDelay[SIGMAP_MAX_ELEMENTS];
  float *pTimeSigTmp;

  HANDLE_MP4TIMEBUF hOutputBuffer[SIGMAP_MAX_ELEMENTS];

  int cmonWritePtr;
  int cmonReadPtr;
  int cmonDataValid;
  HANDLE_COMMON_DATA hCmonData[MAX_DELAY_FRAMES][SIGMAP_MAX_ELEMENTS];
  int headerFrame;

  HANDLE_BIT_BUF hBitBufExtPayload;
  unsigned char tmpBuffer[1024];

  HANDLE_SBR_ENCODER hSbrEnc[SIGMAP_MAX_ELEMENTS];
  XHEAACENCLIB_CODEC_TYPE coreCoder;

  int raisedXoverFreq;

  int bReducedCoreCoderFrameLength;
  int bSbr41;
};

static float
GetMatchingEntry(const float *array, const int len, const float limit) {
  int k = len - 1;
  assert(array != NULL);
  while (k > 0 && array[k] > limit) --k;
  return array[k];
}

static int
GetNumberOfEffectiveChannels(const CHANNEL_MAPPING *const cm) {
  int numberLFE = 0;
  int el;
  for (el = 0; el < cm->nElements; el++) {
    if (cm->elInfo[el].elType == ID_LFE)
      numberLFE++;
  }
  return (cm->nChannels - numberLFE);
}

static XHEAACENCLIB_SIGMAP_INDEX iisxHEAACEncLibSbrEncGetCicpIndex(XHEAACENCLIB_CONFIG_HANDLE self) {
  XHEAACENCLIB_SIGMAP_INDEX CicpIndex;

  if (self->aot == AUD_OBJ_TYP_PS) {
    CicpIndex = XHEAACENCLIB_SIGMAP_CICP_1;
  } else {
    CicpIndex = self->CicpIndexCoreCoder;
  }
  return CicpIndex;
}

static int ReAssignChannels(CHANNEL_MAPPING *chMap,
                            const unsigned int *const channelOffsets) {
  int err = 0;
  int el, ch;
  int chCnt = 0;
  int checkSum = 0;

  if (channelOffsets != NULL) {
    for (el = 0; el < chMap->nElements; el++) {
      switch (chMap->elInfo[el].elType) {
        case ID_SCE:
        case ID_LFE:
        case ID_CPE:
          for (ch = 0; ch < chMap->elInfo[el].nChannelsInEl; ch++) {
            chMap->elInfo[el].ChannelIndex[ch] = channelOffsets[chCnt];
            checkSum += channelOffsets[chCnt];
            chCnt++;
          }
          break;
        default:

          break;
      }
    }
    if (checkSum != ((chMap->nChannels * (chMap->nChannels - 1)) / 2))
      err = 1;

  } else {
    err = 1;
  }

  return err;
}
static PS_MODE iisxHEAACEncLibSbrEncGetPsMode(XHEAACENCLIB_CONFIG_HANDLE self);

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncConfigure(XHEAACENCLIB_HANDLE_SBRENCODER *p_hSbrEnc,
                                                   XHEAACENCLIB_CONFIG_HANDLE hConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_SBRENCODER *sbrEnc;
  CODEC_TYPE coreCoder = CODEC_UNSPECIFIED;
  int el;

  int lowestSbrStartFreq = INT_MAX;
  int lowestSbrStopFreq = INT_MAX;
  int highestSbrStartFreq = INT_MIN;
  int highestSbrStopFreq = INT_MIN;

  int tmpSbrNoiseBands = 0;
  int avgChBitRate;
  SBR_CHANNELMODE sbrChannelmode = SBR_CHANNELMODE_INVALID;
  sbrEnc = (XHEAACENCLIB_HANDLE_SBRENCODER)iisCalloc(1, sizeof(XHEAACENCLIB_SBRENCODER));

  if (p_hSbrEnc == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (sbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
  }

  if (!isError(retValue)) {
    sbrEnc->nFrameLengthCore = hConfig->granuleLength;
    sbrEnc->sampleRate = hConfig->sampleRateOut;
    sbrEnc->cm = hConfig->cm;
    sbrEnc->bUseDownsampledSbr = hConfig->bUseDownsampledSbr;
    sbrEnc->PSMode = iisxHEAACEncLibSbrEncGetPsMode(hConfig);
    if (hConfig->bUseSBR41) {
      sbrEnc->nFrameLengthSbr = sbrEnc->nFrameLengthCore * 4;
    } else {
      sbrEnc->nFrameLengthSbr = max(1024, sbrEnc->nFrameLengthCore) * (sbrEnc->bUseDownsampledSbr ? 1 : 2);
    }
    sbrEnc->useSpeechConfig = 0;
    sbrEnc->overrideTimeDiffCoding = 0;
    sbrEnc->coreCoder = hConfig->codecType;
    sbrEnc->raisedXoverFreq = 0;

    sbrEnc->bReducedCoreCoderFrameLength = (hConfig->granuleLength == 768);
    sbrEnc->bSbr41 = hConfig->bUseSBR41;
  }

  if (!isError(retValue)) {
    if ((sbrEnc->PSMode == PS_ON) && (iisxHEAACEncLibSbrEncGetCicpIndex(hConfig) != XHEAACENCLIB_SIGMAP_CICP_1)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_PARAMETER;
    }
  }

  if (!isError(retValue)) {
    sbrEnc->bUseCRC = 0;
    sbrEnc->sendHeaderTimeInterval = 1.0f;
    sbrEnc->sendHeaderDelay = 0;
  }

  if (!isError(retValue)) {
    switch (sbrEnc->coreCoder) {
      case XHEAACENCLIB_CODEC_AAC:
        assert(0);
        break;
      case XHEAACENCLIB_CODEC_USAC:
        coreCoder = CODEC_SAAC;
        break;
      case XHEAACENCLIB_CODEC_UNSPECIFIED:
      default:
        assert(0);
        break;
    }
  }

  avgChBitRate = hConfig->bitRate / GetNumberOfEffectiveChannels(sbrEnc->cm);

  for (el = 0; (!isError(retValue)) && (el < sbrEnc->cm->nElements); el++) {
    int coreSampleRate;

    if (sbrEnc->bSbr41) {
      coreSampleRate = sbrEnc->sampleRate / 4;
    } else {
      coreSampleRate = sbrEnc->sampleRate / (sbrEnc->bUseDownsampledSbr ? 1 : 2);
    }

    sbrEnc->chBitRate = avgChBitRate;

    switch (sbrEnc->cm->elInfo[el].elType) {
      case ID_LFE:

        sbrEnc->sbrSettings[el] = NULL;
        break;

      case ID_SCE:
      case ID_CPE:
        sbrEnc->sbrSettings[el] = (sbrConfiguration *)iisCalloc(1, sizeof(sbrConfiguration));
        if (sbrEnc->sbrSettings[el] == NULL) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
        }

        if (!isError(retValue)) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = InitializeSbrDefaults(sbrEnc->sbrSettings[el], coreCoder, 2, max(1024, sbrEnc->nFrameLengthCore));
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
          }
        }

        if (!isError(retValue)) {
          switch (sbrEnc->cm->elInfo[el].bMpegs212) {
            case 0:

              switch (sbrEnc->cm->elInfo[el].nChannelsInEl) {
                case 1:
                  if (sbrEnc->cm->elInfo[el].isPS) {
                    sbrChannelmode = SBR_CHANNELMODE_PS;
                  } else {
                    sbrChannelmode = SBR_CHANNELMODE_MONO;
                  }
                  break;
                case 2:
                  sbrChannelmode = SBR_CHANNELMODE_STEREO;
                  break;
                default:
                  retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
                  break;
              }

              break;

            case 1:
            case 2:
              sbrChannelmode = SBR_CHANNELMODE_MPS212;
              break;
            case 3:
              sbrChannelmode = SBR_CHANNELMODE_STEREO;
              break;
            default:
              break;
          }
        }

        if (!isError(retValue)) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = AdjustSbrSettings(sbrEnc->sbrSettings[el],

                                        (unsigned int)(sbrEnc->cm->elInfo[el].relativeBits * hConfig->bitRate),
                                        sbrChannelmode,
                                        coreSampleRate,
                                        8,
                                        24000,
                                        0,
                                        sbrEnc->useSpeechConfig,
                                        0,
                                        hConfig->bUseSBR41 ? QUAD_RATE : DUAL_RATE,
                                        sbrEnc->bUseDownsampledSbr,
                                        (sbrEnc->PSMode == PS_ON),
                                        0,
                                        -1,
                                        -1);

          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
          }
        }

        if (!isError(retValue)) {
          sbrEnc->sbrSettings[el]->timeInputStride = sbrEnc->cm->elInfo[el].nChannelsInEl;
          sbrEnc->sbrSettings[el]->crcSbr = sbrEnc->bUseCRC;
          sbrEnc->sbrSettings[el]->SendHeaderDataTime = sbrEnc->sendHeaderTimeInterval;
        }

        if (!isError(retValue)) {
          lowestSbrStartFreq = min(lowestSbrStartFreq, sbrEnc->sbrSettings[el]->startFreq);
          lowestSbrStopFreq = min(lowestSbrStopFreq, sbrEnc->sbrSettings[el]->stopFreq);

          highestSbrStartFreq = max(highestSbrStartFreq, sbrEnc->sbrSettings[el]->startFreq);
          highestSbrStopFreq = max(highestSbrStopFreq, sbrEnc->sbrSettings[el]->stopFreq);
        }

        if (hConfig->codecType == XHEAACENCLIB_CODEC_USAC) {
          if (!isError(retValue)) {
            sbrEnc->sbrSettings[el]->codecSettings.nChannelsInput = hConfig->nInChannels;
          }

          if (!isError(retValue)) {
            if (el == 0) {
              tmpSbrNoiseBands = sbrEnc->sbrSettings[el]->sbr_noise_bands;
            } else {
              tmpSbrNoiseBands = min(tmpSbrNoiseBands, sbrEnc->sbrSettings[el]->sbr_noise_bands);
            }
          }

          if (!isError(retValue)) {
            int elemBitrate = (unsigned int)(sbrEnc->cm->elInfo[el].relativeBits * hConfig->bitRate);

            if ((hConfig->aot == AUD_OBJ_TYP_USAC) && (elemBitrate <= 48000)) {
              sbrEnc->sbrSettings[el]->sibilantTuning = 1;
            }
          }
        }

        if (!isError(retValue)) {
          assert(hConfig->aot == AUD_OBJ_TYP_HEAAC || hConfig->aot == AUD_OBJ_TYP_PS || hConfig->aot == AUD_OBJ_TYP_USAC || hConfig->bUseHBE == 0);
          sbrEnc->sbrSettings[el]->useHBE = hConfig->bUseHBE;
        }

        if (!isError(retValue)) {
          sbrEnc->sbrSettings[el]->usePVC = hConfig->bUseSBRPVC;
        }

        if (!isError(retValue)) {
          sbrEnc->sbrSettings[el]->bReducedCoreCoderFrameLength = (hConfig->granuleLength == 768);
        }

        if (!isError(retValue)) {
          sbrEnc->sbrSettings[el]->bSbr41 = hConfig->bUseSBR41;
        }

        break;
      default:
        sbrEnc->sbrSettings[el] = NULL;
        break;
    }
  }

  if (!isError(retValue)) {
    for (el = 0; el < sbrEnc->cm->nElements; el++) {
      switch (sbrEnc->cm->elInfo[el].elType) {
        case ID_SCE:
        case ID_CPE:
          if (hConfig->codecType == XHEAACENCLIB_CODEC_AAC) {
            sbrEnc->sbrSettings[el]->startFreq = highestSbrStartFreq;
            sbrEnc->sbrSettings[el]->stopFreq = highestSbrStopFreq;
          } else {
            sbrEnc->sbrSettings[el]->startFreq = lowestSbrStartFreq;
            sbrEnc->sbrSettings[el]->stopFreq = lowestSbrStopFreq;
            sbrEnc->sbrSettings[el]->sbr_noise_bands = tmpSbrNoiseBands;
          }
          break;
        default:
          break;
      }
    }
  }

  *p_hSbrEnc = sbrEnc;

  return retValue;
}

static XHEAACENCLIB_RETURN __initDelayCompensationPS(XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                                     const int coreCoderDelay,
                                                     int *pAdditionalDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el = 0;
  int relCoreDelay = 0;
  int fullFrameDelay = 0;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  for (el = 0; (el < hSbrEnc->cm->nElements) && (!isError(retValue)); el++) {
    switch (hSbrEnc->cm->elInfo[el].elType) {
      case ID_LFE:

        hSbrEnc->hInputBuffer[el] = NULL;
        break;

      case ID_SCE:
      case ID_CPE: {
        int simSpace = hSbrEnc->nFrameLengthSbr;

        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = MP4TIMEBUF_Create(&hSbrEnc->hInputBuffer[el],
                                      simSpace,
                                      simSpace,
                                      (hSbrEnc->PSMode == PS_OFF) ? hSbrEnc->cm->elInfo[el].nChannelsInEl : 2,
                                      0);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP;
        }

        relCoreDelay = (coreCoderDelay + SbrGetDelayDec(hSbrEnc->hSbrEnc[el]->sbrConfigData) - SbrGetDelayEnc(hSbrEnc->hSbrEnc[el]->sbrConfigData) + 1);

        fullFrameDelay = (relCoreDelay + hSbrEnc->nFrameLengthSbr - 1) / hSbrEnc->nFrameLengthSbr;

        if (errorInfo == noError) {
          errorInfo = MP4TIMEBUF_Create(&hSbrEnc->hOutputBuffer[el],
                                        hSbrEnc->nFrameLengthCore + ((fullFrameDelay * hSbrEnc->nFrameLengthSbr) - relCoreDelay) / (hSbrEnc->nFrameLengthSbr / hSbrEnc->nFrameLengthCore),
                                        hSbrEnc->nFrameLengthCore,
                                        1,
                                        ((fullFrameDelay * hSbrEnc->nFrameLengthSbr) - relCoreDelay) / (hSbrEnc->nFrameLengthSbr / hSbrEnc->nFrameLengthCore));
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP;
          }
        }
      } break;
      default:
        hSbrEnc->hInputBuffer[el] = NULL;
        break;
    }
  }

  if (!isError(retValue)) {
    if (fullFrameDelay >= MAX_DELAY_FRAMES) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CONFIGURATION;
    }
  }

  if (!isError(retValue)) {
    if (pAdditionalDelay != NULL) {
      *pAdditionalDelay = ((fullFrameDelay * hSbrEnc->nFrameLengthSbr) - relCoreDelay);
    } else {
      retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
    }
  }

  hSbrEnc->sendHeaderDelay = fullFrameDelay;

  if (!isError(retValue)) {
    while (!isError(retValue) && fullFrameDelay > 0) {
      iisxHEAACEncLibSbrEncSendHeader(hSbrEnc);
      retValue = iisxHEAACEncLibSbrEncEncode(hSbrEnc, NULL,
                                             0, NULL, NULL, NULL, 1);
      fullFrameDelay--;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncSetRightBorderFIX(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                                           int rightBorderFIX) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (NULL != hSbrEnc) {
    SbrSetRightBorderFIX(hSbrEnc->hSbrEnc[0], rightBorderFIX);
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrInitDelayCompensation(
    XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
    const int coreCoderDelay,
    const int mpsDelay,
    int *pAdditionalDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int relCoreDelay = 0;

  if (NULL == hSbrEnc) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  else if (hSbrEnc->PSMode == PS_ON) {
    retValue = __initDelayCompensationPS(hSbrEnc, coreCoderDelay, pAdditionalDelay);
  } else {
    int el;

    if (!isError(retValue)) {
      if (NULL == (hSbrEnc->pTimeSigTmp = (float *)iisCalloc(2 * hSbrEnc->nFrameLengthSbr, sizeof(float)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
      }
    }

    for (el = 0; (el < hSbrEnc->cm->nElements) && (!isError(retValue)); el++) {
      int sbrEncoderDelay = 0;

      switch (hSbrEnc->cm->elInfo[el].elType) {
        case ID_SCE:
        case ID_CPE:
          sbrEncoderDelay = SbrGetDelayEnc(hSbrEnc->hSbrEnc[el]->sbrConfigData);
          relCoreDelay =
              coreCoderDelay + SbrGetDelayDec(hSbrEnc->hSbrEnc[el]->sbrConfigData) - sbrEncoderDelay + 1;
          relCoreDelay += mpsDelay;
          break;
        default:
          break;
      }
    }

    for (el = 0; (el < hSbrEnc->cm->nElements) && (!isError(retValue)); el++) {
      switch (hSbrEnc->cm->elInfo[el].elType) {
        case ID_LFE:

          hSbrEnc->hInputBuffer[el] = NULL;
          hSbrEnc->nAdditionalOutputDelay[el] = 0;
          break;

        case ID_SCE:
        case ID_CPE: {
          int simSpace = hSbrEnc->nFrameLengthSbr;

          {
            hSbrEnc->nAdditionalOutputDelay[el] = 0;

            HANDLE_ERROR_INFO errorInfo = noError;
            errorInfo = MP4TIMEBUF_Create(&hSbrEnc->hInputBuffer[el],
                                          simSpace + relCoreDelay,
                                          simSpace,
                                          (hSbrEnc->PSMode == PS_OFF) ? hSbrEnc->cm->elInfo[el].nChannelsInEl : 2,
                                          relCoreDelay);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP;
            }
          }

          if (!isError(retValue)) {
            HANDLE_ERROR_INFO errorInfo = noError;
            errorInfo = MP4TIMEBUF_Create(&hSbrEnc->hOutputBuffer[el],
                                          hSbrEnc->nFrameLengthSbr + hSbrEnc->nAdditionalOutputDelay[el],
                                          hSbrEnc->nFrameLengthSbr,
                                          hSbrEnc->cm->elInfo[el].nChannelsInEl,
                                          hSbrEnc->nAdditionalOutputDelay[el]);
            if (errorInfo != noError) {
              retValue = XHEAACENCLIB_RETURN_ERROR_SBR_INIT_DELAY_COMP;
            }
          }
          break;
        }
        default:
          hSbrEnc->hInputBuffer[el] = NULL;
          break;
      }
    }

    if (!isError(retValue)) {
      *pAdditionalDelay = hSbrEnc->nAdditionalOutputDelay[0];
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncOpen(XHEAACENCLIB_HANDLE_SBRENCODER self,
                                              int *sbrEncoderDelay,
                                              int *sbrDecoderDelay) {
  int el, frame;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  CODEC_TYPE coreCoder = CODEC_UNSPECIFIED;

  self->cmonWritePtr = 0;
  self->cmonReadPtr = 0;
  self->cmonDataValid = 0;

  if (self == NULL || sbrEncoderDelay == NULL || sbrDecoderDelay == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }
  if (!isError(retValue)) {
    switch (self->coreCoder) {
      case XHEAACENCLIB_CODEC_AAC:
        assert(0);
        break;
      case XHEAACENCLIB_CODEC_USAC:
        coreCoder = CODEC_SAAC;
        break;
      case XHEAACENCLIB_CODEC_UNSPECIFIED:
      default:
        assert(0);
        coreCoder = CODEC_UNSPECIFIED;
        break;
    }
  }

  for (el = 0; !isError(retValue) && el < self->cm->nElements; el++) {
    switch (self->cm->elInfo[el].elType) {
      case ID_LFE:

        self->hCmonData[0][el] = NULL;
        self->hSbrEnc[el] = NULL;
        break;

      case ID_SCE:
      case ID_CPE:

        for (frame = 0; frame < MAX_DELAY_FRAMES; frame++) {
          self->hCmonData[frame][el] = CreateCommonData(MAX_SBR_BITBUF_SIZE);
        }
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = EnvOpen(&(self->hSbrEnc[el]),
                            self->sbrSettings[el],
                            self->hCmonData[0][el],
                            coreCoder);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SBR_ENC_OPEN;
        }

        for (frame = 1; !isError(retValue) && frame < MAX_DELAY_FRAMES; frame++) {
          CopyCommonData(self->hCmonData[frame][el], self->hCmonData[0][el]);
        }

        if (!isError(retValue)) {
          if (self->hSbrEnc[el]) {
            *sbrEncoderDelay = SbrGetDelayEnc(self->hSbrEnc[el]->sbrConfigData);
            *sbrDecoderDelay = SbrGetDelayDec(self->hSbrEnc[el]->sbrConfigData);
            SbrSetTimeDiffCodingFlag(self->hSbrEnc[el], self->overrideTimeDiffCoding);
          } else {
            retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
          }
        }
        break;

      default:

        self->hCmonData[0][el] = NULL;
        self->hSbrEnc[el] = NULL;
    }
  }

  if (!isError(retValue)) {
    if (NULL == (self->hBitBufExtPayload = CreateBitBuffer(2 * 6144))) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SBR_MEMORY_ALLOCATION;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncClose(XHEAACENCLIB_HANDLE_SBRENCODER self) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el, frame;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    for (el = 0; el < SIGMAP_MAX_ELEMENTS; el++) {
      if (self->sbrSettings[el]) {
        iisFree(self->sbrSettings[el]);
        self->sbrSettings[el] = NULL;
      }

      if (self->hSbrEnc[el]) {
        EnvClose(self->hSbrEnc[el]);
        self->hSbrEnc[el] = NULL;
      }

      for (frame = MAX_DELAY_FRAMES - 1; frame >= 0; frame--) {
        if (self->hCmonData[0][el]) {
          DeleteCommonData(&self->hCmonData[frame][el]);
          self->hCmonData[frame][el] = NULL;
        }
      }
      if (self->hInputBuffer[el]) {
        MP4TIMEBUF_Delete(&self->hInputBuffer[el]);
        self->hInputBuffer[el] = NULL;
      }
      if (self->hOutputBuffer[el]) {
        MP4TIMEBUF_Delete(&self->hOutputBuffer[el]);
        self->hOutputBuffer[el] = NULL;
      }
    }

    if (self->pTimeSigTmp) {
      iisFree(self->pTimeSigTmp);
    }
    self->pTimeSigTmp = NULL;

    if (self->hBitBufExtPayload) {
      DeleteBitBuffer(self->hBitBufExtPayload);
    }
    self->hBitBufExtPayload = NULL;

    iisFree(self);
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncEncode(XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                                const float *const pTimeSigIn,
                                                const int nSamplesIn,
                                                float *pTimeSigOut,
                                                unsigned int *pnSamplesOut,
                                                HANDLE_IIS_SWDECI hSwdeci,
                                                int const bUsacIndependenceFlag) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el = 0;
  int samplesPerCh = 0;
  unsigned int nSamplesOutTmp = 0;
  (void)pTimeSigOut;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hSbrEnc->PSMode == PS_ON) {
      samplesPerCh = nSamplesIn / 2;
    } else {
      samplesPerCh = nSamplesIn / hSbrEnc->cm->nChannels;
    }
  }
  if (!isError(retValue)) {
  }

  for (el = 0; (!isError(retValue)) && (el < hSbrEnc->cm->nElements); el++) {
    int frameSize = hSbrEnc->nFrameLengthSbr;

    switch (hSbrEnc->cm->elInfo[el].elType) {
      case ID_LFE:
        break;

      case ID_SCE:
      case ID_CPE: {
        HANDLE_ERROR_INFO errorInfo = noError;

        if (hSbrEnc->PSMode == PS_ON) {
          assert(hSbrEnc->cm->nChannels == 1);

          errorInfo = MP4TIMEBUF_FeedBufferStereo(hSbrEnc->hInputBuffer[el],
                                                  pTimeSigIn ? pTimeSigIn + hSbrEnc->cm->elInfo[el].ChannelIndex[0] : NULL,
                                                  pTimeSigIn ? pTimeSigIn + hSbrEnc->cm->elInfo[el].ChannelIndex[0] + 1 : NULL,
                                                  2,
                                                  1.0f,
                                                  samplesPerCh);

          if (errorInfo == noError && frameSize > samplesPerCh) {
            errorInfo = MP4TIMEBUF_FeedBufferStereo(hSbrEnc->hInputBuffer[el],
                                                    NULL,
                                                    NULL,
                                                    hSbrEnc->cm->nChannels,
                                                    1.0f,
                                                    frameSize - samplesPerCh);
          }
        } else {
          errorInfo = MP4TIMEBUF_FeedBufferStereo(hSbrEnc->hInputBuffer[el],
                                                  pTimeSigIn + (pTimeSigIn ? (hSbrEnc->cm->elInfo[el].ChannelIndex[0]) : 0),
                                                  pTimeSigIn + (pTimeSigIn ? (hSbrEnc->cm->elInfo[el].ChannelIndex[1]) : 0),
                                                  hSbrEnc->cm->nChannels,
                                                  1.0f,
                                                  samplesPerCh);

          if (errorInfo == noError && frameSize > samplesPerCh) {
            errorInfo = MP4TIMEBUF_FeedBufferStereo(hSbrEnc->hInputBuffer[el],
                                                    NULL,
                                                    NULL,
                                                    hSbrEnc->cm->nChannels,
                                                    1.0f,
                                                    frameSize - samplesPerCh);
          }
        }
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
        }
      }

        if (!isError(retValue) && hSbrEnc->coreCoder == XHEAACENCLIB_CODEC_USAC) {
          struct COMMON_DATA *pCmon = hSbrEnc->hCmonData[hSbrEnc->cmonWritePtr][el];

          float xOverFreqMin = hSbrEnc->hCmonData[0][el]->allowedXOverFreqs[0];

          if (NULL != hSwdeci) {
            int isHighBwCore = (IIS_SWDECI_RESULT_SPEECH == iisSwitchingDecisionGetFrameDecision(hSwdeci, SWDECI_ID_CORE_CODER)) ? 1 : 0;
            if (0 != isHighBwCore) {
              float limit = 0.f;

              if (hSbrEnc->bSbr41 && hSbrEnc->bReducedCoreCoderFrameLength) {
                limit = (float)(3 * hSbrEnc->sampleRate / 32 - 1);
              } else if (hSbrEnc->bSbr41 && !hSbrEnc->bReducedCoreCoderFrameLength) {
                limit = (float)(hSbrEnc->sampleRate / 8 - 1);
              } else if (!hSbrEnc->bSbr41 && hSbrEnc->bReducedCoreCoderFrameLength) {
                limit = (float)(3 * hSbrEnc->sampleRate / 16);
              } else {
                limit = (float)(hSbrEnc->sampleRate / 4);
              }

              pCmon->dynXOverFreqEnc = GetMatchingEntry(hSbrEnc->hCmonData[0][el]->allowedXOverFreqs,
                                                        hSbrEnc->hCmonData[0][el]->allowedXOverFreqsLen,
                                                        limit);
            } else {
              pCmon->dynXOverFreqEnc = xOverFreqMin;
            }
          } else if (0 < hSbrEnc->raisedXoverFreq) {
            float limit = 0.f;
            if (hSbrEnc->bSbr41 && hSbrEnc->bReducedCoreCoderFrameLength) {
              limit = (float)(3 * hSbrEnc->sampleRate / 32);
            } else if (hSbrEnc->bSbr41 && !hSbrEnc->bReducedCoreCoderFrameLength) {
              limit = (float)(hSbrEnc->sampleRate / 8);
            } else if (!hSbrEnc->bSbr41 && hSbrEnc->bReducedCoreCoderFrameLength) {
              limit = (float)(3 * hSbrEnc->sampleRate / 16);
            } else {
              limit = (float)(hSbrEnc->sampleRate / 4);
            }

            pCmon->dynXOverFreqEnc = GetMatchingEntry(hSbrEnc->hCmonData[0][el]->allowedXOverFreqs,
                                                      hSbrEnc->hCmonData[0][el]->allowedXOverFreqsLen,
                                                      limit);
          }
        }

        if (!isError(retValue)) {
          int isSwitchingDecisionResultSpeech = 0;
          if (hSbrEnc->raisedXoverFreq) {
            isSwitchingDecisionResultSpeech = 1;
          } else if (NULL != hSwdeci) {
            isSwitchingDecisionResultSpeech = (IIS_SWDECI_RESULT_SPEECH == iisSwitchingDecisionGetFrameDecision(hSwdeci, SWDECI_ID_CORE_CODER)) ? 1 : 0;
          } else {
            isSwitchingDecisionResultSpeech = 0;
          }
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = EnvEncodeFrame(hSbrEnc->hSbrEnc[el],
                                     hSbrEnc->hCmonData[hSbrEnc->cmonWritePtr][el],
                                     MP4TIMEBUF_AccessBuffer(hSbrEnc->hInputBuffer[el], 0, 0),
                                     NULL,
                                     NULL,
                                     isSwitchingDecisionResultSpeech,
                                     bUsacIndependenceFlag);

          if (noError != errorInfo) {
            retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
          }
        }

        if (!isError(retValue)) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = MP4TIMEBUF_InvalidateBuffer(hSbrEnc->hInputBuffer[el], frameSize);
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
          }
        }
        break;

      default:

        break;
    }
  }

  if (!isError(retValue)) {
    hSbrEnc->cmonDataValid++;
    assert(hSbrEnc->cmonDataValid <= MAX_DELAY_FRAMES);
    hSbrEnc->cmonWritePtr++;
    if (hSbrEnc->cmonWritePtr >= MAX_DELAY_FRAMES) {
      hSbrEnc->cmonWritePtr = 0;
    }

    if (nSamplesIn > 0 && pnSamplesOut) {
      *pnSamplesOut = nSamplesOutTmp;
    }
  }
  return retValue;
}

float iisxHEAACEncLibSbrEncGetXOverFreq(XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  float xOverFreq = 0.0f;
  if (hSbrEnc) {
    if (hSbrEnc->hCmonData[hSbrEnc->cmonReadPtr][0]) {
      xOverFreq = hSbrEnc->hCmonData[hSbrEnc->cmonReadPtr][0]->xOverFreq;
    }
  }

  return xOverFreq;
}

struct COMMON_DATA **iisxHEAACEncLibSbrEncGetCmonDataHandle(
    XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  if (hSbrEnc == NULL) {
    return NULL;
  } else {
    struct COMMON_DATA **hCmon = hSbrEnc->hCmonData[hSbrEnc->cmonReadPtr];
    hSbrEnc->cmonReadPtr++;
    if (hSbrEnc->cmonReadPtr == MAX_DELAY_FRAMES) {
      hSbrEnc->cmonReadPtr = 0;
    }
    hSbrEnc->cmonDataValid--;
    assert(hSbrEnc->cmonDataValid >= 0);

    return hCmon;
  }
}

XHEAACENCLIB_RETURN iisxHEAACEncLibSbrEncSetOffsets(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                                    const unsigned int *const channelOffsets) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (0 != ReAssignChannels(hSbrEnc->cm, channelOffsets)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetSbrTransmissionConfig(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                              const int bUseCRC,
                                              const float sendHeaderTimeInterval) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int el;
    hSbrEnc->bUseCRC = bUseCRC;
    hSbrEnc->sendHeaderTimeInterval = sendHeaderTimeInterval;

    for (el = 0; el < hSbrEnc->cm->nElements; el++) {
      if (hSbrEnc->sbrSettings[el] != NULL) {
        hSbrEnc->sbrSettings[el]->crcSbr = bUseCRC;
        hSbrEnc->sbrSettings[el]->SendHeaderDataTime = sendHeaderTimeInterval;
      }

      if (hSbrEnc->hSbrEnc[el]) {
        SbrSetHeaderSendInterval(hSbrEnc->hSbrEnc[el],
                                 hSbrEnc->sbrSettings[el]);

        SbrSetCrcProtection(hSbrEnc->hSbrEnc[el],
                            hSbrEnc->sbrSettings[el]);
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetPsTimeSignal(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                     float **pSamplesOut,
                                     unsigned int *nSamplesOut) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;
  float *timeSignal = NULL;

  if (hSbrEnc == NULL || pSamplesOut == NULL || nSamplesOut == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *nSamplesOut = SbrGetDownsampledOutSignal(hSbrEnc->hSbrEnc[0], &timeSignal);

    errorInfo = MP4TIMEBUF_FeedBufferStereo(hSbrEnc->hOutputBuffer[0],
                                            timeSignal,
                                            NULL,
                                            1,
                                            1.0f,
                                            hSbrEnc->nFrameLengthCore * hSbrEnc->nFrameLengthCore);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  if (!isError(retValue)) {
    *pSamplesOut = MP4TIMEBUF_AccessBuffer(hSbrEnc->hOutputBuffer[0], 0, 0);

    errorInfo = MP4TIMEBUF_InvalidateBuffer(hSbrEnc->hOutputBuffer[0], hSbrEnc->nFrameLengthCore);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
    }
  }

  return retValue;
}

int iisxHEAACEncLibSbrEncGetPsTimeSignalDelay(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  return SbrGetDownsamplerDelay(hSbrEnc->hSbrEnc[0]);
}

int iisxHEAACEncLibSbrEncGetEstimateSbrBitrate(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  int el;
  int estimateSbrBitrate = 0;

  if (hSbrEnc) {
    for (el = 0; el < hSbrEnc->cm->nElements; el++) {
      estimateSbrBitrate += SbrGetEstimateBitrate(hSbrEnc->sbrSettings[el]);
    }
  }

  return estimateSbrBitrate;
}

int iisxHEAACEncLibSbrEncGetSbrPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  return SbrGetSbrPresent(hSbrEnc->hSbrEnc[0]);
}

int iisxHEAACEncLibSbrEncGetPsPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  return SbrGetPsPresent(hSbrEnc->hSbrEnc[0]);
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSendHeader(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el;

  if (hSbrEnc != NULL) {
    if (hSbrEnc->cm) {
      for (el = 0; el < hSbrEnc->cm->nElements; el++) {
        SbrSendSbrHeader(hSbrEnc->hSbrEnc[el]);
      }
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncContainsHeader(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                    int *headerFrame) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue) && hSbrEnc->cm) {
    for (el = 0; el < hSbrEnc->cm->nElements; el++) {
      *headerFrame = hSbrEnc->headerFrame;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSendHeaderDelay(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                     int *sendHeaderDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    *sendHeaderDelay = 0;

    if (hSbrEnc) {
      *sendHeaderDelay = hSbrEnc->sendHeaderDelay;
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncSetRaisedXoverFreq(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                        const int raisedXoverFreq) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hSbrEnc->raisedXoverFreq = raisedXoverFreq;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrMoveExtPayload(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                 HANDLE_EXTPAYLOAD_CONTAINER *hExtContainer,
                                 int *usedBits) {
  int el;
  unsigned int sbrPayload = 0;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  struct COMMON_DATA **hCmon = iisxHEAACEncLibSbrEncGetCmonDataHandle(hSbrEnc);
  hSbrEnc->headerFrame = 1;

  if (usedBits != NULL) {
    *usedBits = 0;
  }

  for (el = 0; el < hSbrEnc->cm->nElements; el++) {
    if (hCmon[el]) {
      EXTENSION_PAYLOAD_TYPES extType;

      if (hSbrEnc->sbrSettings[el]->codecSettings.coreCodec != CODEC_SAAC)
        extType = hSbrEnc->sbrSettings[el]->crcSbr ? EXT_SBR_DATA_CRC : EXT_SBR_DATA;
      else
        extType = EXT_USACSBR_DATA;

      sbrPayload = WriteSbrExtensionData(hSbrEnc->hBitBufExtPayload,
                                         hCmon[el],
                                         SBR_NOT_SCALABLE,
                                         (hSbrEnc->sbrSettings[el]->codecSettings.coreCodec != CODEC_SAAC) ? hSbrEnc->sbrSettings[el]->crcSbr : 0,
                                         (hSbrEnc->sbrSettings[el]->codecSettings.coreCodec != CODEC_SAAC) ? -1 : 0,
                                         (hSbrEnc->sbrSettings[el]->codecSettings.coreCodec != CODEC_SAAC) ? 4 : 0,
                                         (hSbrEnc->sbrSettings[el]->codecSettings.coreCodec != CODEC_SAAC) ? 1 : 0,
                                         hSbrEnc->sbrSettings[el]->codecSettings.coreCodec);

      ByteAlign(hSbrEnc->hBitBufExtPayload);
      ReadBytes(hSbrEnc->hBitBufExtPayload, hSbrEnc->tmpBuffer, (GetBitsAvail(hSbrEnc->hBitBufExtPayload) >> 3));
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = addExtensionPayload(hExtContainer[el],
                                      extType,
                                      sbrPayload,
                                      0,
                                      NO_NIBBLE,
                                      hSbrEnc->tmpBuffer,
                                      0,
                                      0);

      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
        break;
      }

      ResetBitBuffer(hSbrEnc->hBitBufExtPayload);

      if (usedBits != NULL) {
        *usedBits += sbrPayload;
      }
    }
  }

  return retValue;
}

int iisxHEAACEncLibSbrEncGetHbePresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el) {
  int hbePresent = -1;
  if (hSbrEnc) {
    hbePresent = (hSbrEnc->sbrSettings[el]->useHBE) ? 1 : 0;
  }
  return hbePresent;
}

int iisxHEAACEncLibSbrEncGetBsInterTes(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el) {
  int bs_interTes = -1;
  if (hSbrEnc != NULL) {
    bs_interTes = (hSbrEnc->sbrSettings[el]->bs_interTes) ? 1 : 0;
  }
  return bs_interTes;
}

int iisxHEAACEncLibSbrEncGetPvcPresent(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc, const int el) {
  int pvcPresent = -1;
  if (hSbrEnc != NULL) {
    pvcPresent = (hSbrEnc->sbrSettings[el]->usePVC) ? 1 : 0;
  }
  return pvcPresent;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetUsacSbrDfltHeaderData(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                              USAC_SBR_HEADER *pUsacSbrDfltHeader,
                                              const int el) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  SBR_USAC_HEADER_DATA sbrUsacHeaderData = {0};

  if (hSbrEnc == NULL || pUsacSbrDfltHeader == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = SbrGetUsacDfltHeader(hSbrEnc->hSbrEnc[el], &sbrUsacHeaderData);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SBR;
    }
  }
  if (!isError(retValue)) {
    pUsacSbrDfltHeader->start_freq = sbrUsacHeaderData.start_freq;
    pUsacSbrDfltHeader->stop_freq = sbrUsacHeaderData.stop_freq;
    pUsacSbrDfltHeader->header_extra1 = sbrUsacHeaderData.header_extra1;
    pUsacSbrDfltHeader->header_extra2 = sbrUsacHeaderData.header_extra2;
    pUsacSbrDfltHeader->freq_scale = sbrUsacHeaderData.freq_scale;
    pUsacSbrDfltHeader->alter_scale = sbrUsacHeaderData.alter_scale;
    pUsacSbrDfltHeader->noise_bands = sbrUsacHeaderData.noise_bands;
    pUsacSbrDfltHeader->limiter_bands = sbrUsacHeaderData.limiter_bands;
    pUsacSbrDfltHeader->limiter_gains = sbrUsacHeaderData.limiter_gains;
    pUsacSbrDfltHeader->interpol_freq = sbrUsacHeaderData.interpol_freq;
    pUsacSbrDfltHeader->smoothing_mode = sbrUsacHeaderData.smoothing_mode;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibSbrEncGetStopFreq(const XHEAACENCLIB_HANDLE_SBRENCODER hSbrEnc,
                                 float *stopFreq) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else {
    *stopFreq = 0;
    if (hSbrEnc->hSbrEnc[0]) {
      *stopFreq = SbrGetStopFreqRaw(hSbrEnc->hSbrEnc[0]);
    }
  }

  return retValue;
}

static PS_MODE iisxHEAACEncLibSbrEncGetPsMode(XHEAACENCLIB_CONFIG_HANDLE self) {
  PS_MODE psMode;
  if (self && self->aot == AUD_OBJ_TYP_PS) {
    psMode = PS_ON;
  } else {
    psMode = PS_OFF;
  }
  return psMode;
}
