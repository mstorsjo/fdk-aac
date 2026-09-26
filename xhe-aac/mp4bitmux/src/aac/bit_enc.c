
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
#include <string.h>
#include <float.h>
#include <math.h>

#include "iisutillib.h"
#include "bit_enc.h"
#include "bit_aac.h"
#include "codebook.h"

#if defined __GNUC__ || defined __clang__
#define HUFFCODEBOOK __attribute__((unused))
#else
#define HUFFCODEBOOK
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

static const int sampleRates[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, -1, -1, -1};

void InitDefaultBitstreamConfiguration(BS_CONFIGURATION *config) {
  config->loopBitrate = -1;
  config->thr = 0x0;

  config->hyper = -1;
  config->drm = -1;

  config->useHCR = 0;
  config->useRVLC = 0;
  config->useVCB11 = 0;

  config->variableBitrate = 0;
  config->useNoPadding = 0;

  config->bitResInitFillLevel = FLT_MIN;
  config->bitResDistribution = FLT_MIN;
  config->timeWarpedMdct = 0;
  config->useNoiseFilling = 0;
}

HANDLE_ERROR_INFO
CreateBitstreamEncoder(HANDLE_BITSTREAM_ENC *bsEnc,
                       BS_CONFIGURATION config,
                       const int sampleRate,
                       const BS_CHANNEL_MAPPING *channelMapping,
                       const STREAM_TYPE streamType,
                       const AUDIO_OBJECT_TYPE aot) {
  int i = 0;
  unsigned int ui = 0;
  HANDLE_ERROR_INFO err = noError;
  int nrBlocksPerFrame = 1;
  int nrEffectiveChannels = 0;

  char wrThreshConfig[256] = {'\0'};

  BIT_ENC_CONFIG *beConfig;

  if (channelMapping->noOfChannels > MAX_BIT_ENC_CHANNELS) {
    err = iisUtil_ERROR(CDI, "number of channels too large");
    goto errorReturn;
  }

  *bsEnc = (HANDLE_BITSTREAM_ENC)iisCalloc(1, sizeof(struct BSENC));
  if (*bsEnc == NULL) {
    err = iisUtil_ERROR(CDI, "out of memory");
    goto errorReturn;
  }
  (*bsEnc)->DataReadyToWrite = 0;

  (*bsEnc)->config.streamType = streamType;
  (*bsEnc)->config.aot = aot;
  (*bsEnc)->config.sampleRate = sampleRate;
  (*bsEnc)->config.sampleRateIndex = 15;

  (*bsEnc)->config.variableBitrate = config.variableBitrate;
  if (aot == AOT_USAC || aot == AOT_MPEGH) {
    (*bsEnc)->config.useNoPadding = 0;
  } else
    (*bsEnc)->config.useNoPadding = config.useNoPadding;
  (*bsEnc)->superFrameConfig = 0;

  (*bsEnc)->config.timeWarpedMdct = config.timeWarpedMdct;
  for (i = 0; i < MAX_BIT_ENC_CHANNELS; i++) {
    (*bsEnc)->bitEncChannelData[i].timeWarpedMdct = config.timeWarpedMdct;
  }

  (*bsEnc)->config.useNoiseFilling = config.useNoiseFilling;

  for (ui = 0; ui < sizeof(sampleRates) / sizeof(int); ui++) {
    if (sampleRate == sampleRates[ui]) {
      (*bsEnc)->config.sampleRateIndex = ui;
      break;
    }
  }

  if ((*bsEnc)->config.sampleRateIndex == -1) {
    err = iisUtil_ERROR(CDI, "invalid sample rate");
    goto errorReturn;
  }

  if (config.thr != 0x0) {
    strcpy(wrThreshConfig, (const char *)config.thr);
  }

  beConfig = &((*bsEnc)->config);
  beConfig->nChannels = channelMapping->noOfChannels;

  beConfig->channelMapping = *channelMapping;

  beConfig->channelMapping.element =
      (BS_MAPPING_ELEMENT **)iisCalloc(channelMapping->noOfElements,
                                       sizeof(BS_MAPPING_ELEMENT *));
  if (beConfig->channelMapping.element == 0) {
    err = iisUtil_ERROR(CDI, "out of memory");
    goto errorReturn;
  }

  for (i = 0; i < channelMapping->noOfElements; i++) {
    beConfig->channelMapping.element[i] = iisMalloc(sizeof(BS_MAPPING_ELEMENT));
    if (beConfig->channelMapping.element[i] == NULL) {
      err = iisUtil_ERROR(CDI, "out of memory");
      goto errorReturn;
    }
    memcpy(beConfig->channelMapping.element[i],
           channelMapping->element[i],
           sizeof(BS_MAPPING_ELEMENT));
  }

  nrEffectiveChannels = beConfig->channelMapping.noOfEffectiveChannels;

  for (i = 0; i < channelMapping->noOfElements; i++) {
    switch (beConfig->channelMapping.element[i]->elementType) {
      case ID_CCE: {
        err = iisUtil_ERROR(CDI, "cce not yet supported");
        goto errorReturn;

      } break;

      default:

        break;
    }
  }

  if ((nrEffectiveChannels < 0) || (nrEffectiveChannels > beConfig->nChannels)) {
    err = iisUtil_ERROR(CDI, "invalid number of effective channels.");
    goto errorReturn;
  }

  beConfig->bitResInitFillLevel = config.bitResInitFillLevel;
  if (config.bitResDistribution > 0.0f && config.bitResDistribution <= 1.0f) {
    beConfig->bitResDistribution = config.bitResDistribution;
  }

  if (err != noError) {
    goto errorReturn;
  }

  (*bsEnc)->mxBitstream =
      CreateBitBuffer(MAX_FRAMES_IN_HF * (6144 * beConfig->nChannels * nrBlocksPerFrame + 1024));

  if ((*bsEnc)->mxBitstream == NULL) {
    err = iisUtil_ERROR(CDI, "out of memory");
    goto errorReturn;
  }

  if (((*bsEnc)->config.aot == AOT_USAC) || ((*bsEnc)->config.aot == AOT_MPEGH)) {
    const int resetDistance = -1;

    for (i = 0; i < channelMapping->noOfChannels; i++) {
      createSpectralDataArith2(&(*bsEnc)->bitEncChannelData[i].hEncSpecDataArith, resetDistance);
    }
  }

  return (noError);

errorReturn:
  DeleteBitstreamEncoder(*bsEnc);
  *bsEnc = NULL;
  return err;
}

HANDLE_ERROR_INFO
AdvanceBitstreamEncoder(HANDLE_BITSTREAM_ENC bsEnc,
                        const int additionalBits,
                        const int *additionalElemBits,
                        const int externalElemBits,
                        int headerBits,
                        int totalVarFrameBits,
                        const int *coreMode,
                        const int frameStatus,
                        IISBITFRAME_HANDLE hBitFrame,
                        int isVbr) {
  int i;
  int ch;
  int useACE[2] = {0};

  const BS_CHANNEL_MAPPING *channelMapping = &(bsEnc->config.channelMapping);
  BS_TNS_DATA *bs_tns_data0 = NULL, *bs_tns_data1 = NULL;
  int bTnsActive, bCommonTns, bTnsDataPresentBoth;

  int bUsacIndependenceFlag = 0;
  int bRAP = 0;

  bUsacIndependenceFlag = frameStatus & USAC_INDEPENDENCE_FLAG;
  if (frameStatus & RAP_FRAME) {
    bRAP = 1;
  }

  for (i = 0; i < bsEnc->config.nChannels; i++)
    bsEnc->frameData.blockType[i] = bsEnc->bitEncChannelData[i].blockType;

  if (bsEnc->config.variableBitrate != 1) {
    bsEnc->frameData.totalFrameBits =
        IISBITFRAME_GetNumberOfBitsForCurrentFrame(hBitFrame, !bsEnc->config.useNoPadding);
  } else {
    bsEnc->frameData.totalFrameBits = totalVarFrameBits;
  }

  bsEnc->frameData.availableFrameBits = bsEnc->frameData.totalFrameBits - additionalBits;
  bsEnc->frameData.totHeaderBits = headerBits;
  bsEnc->frameData.availableFrameBits -= headerBits;
  bsEnc->frameData.extraFillBytes = 0;

  bsEnc->frameData.sideInfoBits = 0;
  bsEnc->frameData.dseBits = 0;
  bsEnc->frameData.additionalElemBits = 0;

  if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
    bsEnc->frameData.sideInfoBits += SI_USAC_INDEP_FLAG;

    bsEnc->frameData.sideInfoBits += 1;
  }

  for (i = 0; i < channelMapping->noOfElements; i++) {
    int channelNdx[2];
    BS_MAPPING_ELEMENT *elem = channelMapping->element[i];
    BS_TNS_DATA *bs_tns_data = NULL;
    BS_PITCH_DATA *bs_pitch_data = NULL;
    int bTnsDataPresent[2] = {0};

    bsEnc->frameData.elemSideInfoBits[i] = bsEnc->frameData.sideInfoBits;

    if (additionalElemBits)
      bsEnc->frameData.additionalElemBits += additionalElemBits[i];

    switch (elem->elementType) {
      case ID_SCE:
        channelNdx[0] = elem->element.sce.channelNdx;
        break;
      case ID_CCE:
        channelNdx[0] = elem->element.cce.channelNdx;
        break;
      case ID_LFE:
        channelNdx[0] = elem->element.lfe.channelNdx;
        break;
      case ID_CPE:
        channelNdx[0] = elem->element.cpe.channelNdx[0];
        channelNdx[1] = elem->element.cpe.channelNdx[1];
        break;
      default:
        break;
    }

    switch (elem->elementType) {
      case ID_SCE:
        if (coreMode != NULL) {
          useACE[0] = (coreMode[channelNdx[0]] == 1);
        }

        if (!useACE[0]) {
          bs_tns_data = &bsEnc->bitEncChannelData[channelNdx[0]].bs_tns_data;

          if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
            bsEnc->frameData.sideInfoBits += SI_ID_BITS + SI_SCE_BITS + SI_ICS_BITS - 1;
            bsEnc->frameData.sideInfoBits += 1;
          } else {
            bsEnc->frameData.sideInfoBits += SI_ID_BITS + SI_SCE_BITS + SI_ICS_BITS;
          }

          if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
            bsEnc->frameData.sideInfoBits -= 2;
            bsEnc->frameData.sideInfoBits -= SI_ID_BITS + SI_SCE_BITS;
            bsEnc->frameData.sideInfoBits += 1;
            if (bsEnc->config.useNoiseFilling) {
              bsEnc->frameData.sideInfoBits += SI_ICS_BITS_NF;
            }
          }

          bsEnc->frameData.sideInfoBits += encodeTnsData(bs_tns_data,
                                                         bsEnc->config.aot,
                                                         NULL);

          if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
            if (bsEnc->config.timeWarpedMdct) {
              assert(0);
              bs_pitch_data = &bsEnc->bitEncChannelData[channelNdx[0]].bs_pitch_data;
              bsEnc->frameData.sideInfoBits += encodePitchData(bs_pitch_data, NULL);
            }
            bsEnc->frameData.sideInfoBits += 1;
          }

          switch (bsEnc->frameData.blockType[channelNdx[0]]) {
            case LONG_WINDOW:
            case START_WINDOW:
            case STOP_WINDOW:
            case STOPSTART_WINDOW:
            case LOW_OVER_WINDOW:

              bsEnc->frameData.sideInfoBits += SI_ICS_INFO_BITS_LONG;
              if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
                bsEnc->frameData.sideInfoBits -= (1 + 1);
              }
              break;
            case SHORT_WINDOW:
              bsEnc->frameData.sideInfoBits += SI_ICS_INFO_BITS_SHORT;
              if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
                bsEnc->frameData.sideInfoBits -= (1);
              }
              break;

            default:
              WARN("Unknown window type");
              break;
          }

        } else {
          bsEnc->frameData.sideInfoBits += 1;
        }
        break;

      case ID_LFE:

        if (bsEnc->frameData.blockType[channelNdx[0]] != LONG_WINDOW) {
          return iisUtil_ERROR(CDI, "encoder error: LFE must be long window");
        }
        if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
          bsEnc->frameData.sideInfoBits += 8 + 2 + 1 + 6;
          bsEnc->frameData.sideInfoBits += 1;

        } else {
          bsEnc->frameData.sideInfoBits += SI_ID_BITS + SI_LFE_BITS + SI_ICS_BITS + SI_ICS_INFO_BITS_LONG;
        }

        break;

      case ID_CPE:

        if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
          useACE[0] = (coreMode[channelNdx[0]] == 1);
          useACE[1] = (coreMode[channelNdx[1]] == 1);

          bsEnc->frameData.sideInfoBits += 1 + 1;

          if (!useACE[0] && !useACE[1]) {
            bsEnc->frameData.sideInfoBits += 1;
            bsEnc->frameData.sideInfoBits += 1;

            if (bsEnc->bitEncChannelData[channelNdx[0]].commonWindow == 1) {
              switch (bsEnc->frameData.blockType[channelNdx[0]]) {
                case LONG_WINDOW:
                case START_WINDOW:
                case STOP_WINDOW:
                case STOPSTART_WINDOW:
                  bsEnc->frameData.sideInfoBits += 2 + 1;
                  bsEnc->frameData.sideInfoBits += 6;
                  bsEnc->frameData.sideInfoBits += 1;
                  if (bsEnc->bitEncChannelData[channelNdx[0]].maxSfb != bsEnc->bitEncChannelData[channelNdx[1]].maxSfb) {
                    bsEnc->frameData.sideInfoBits += 6;
                  }
                  break;
                case SHORT_WINDOW:
                  bsEnc->frameData.sideInfoBits += 2 + 1;
                  bsEnc->frameData.sideInfoBits += 4 + 7;
                  bsEnc->frameData.sideInfoBits += 1;
                  if (bsEnc->bitEncChannelData[channelNdx[0]].maxSfb != bsEnc->bitEncChannelData[channelNdx[1]].maxSfb) {
                    bsEnc->frameData.sideInfoBits += 4;
                  }

                  break;

                default:
                  WARN("Unknown window type");
                  break;
              }

              if (bsEnc->bitEncChannelData[channelNdx[0]].commonWindow && bsEnc->bitEncChannelData[channelNdx[1]].commonWindow) {
                int maxSfbSte;
                maxSfbSte = bsEnc->bitEncChannelData[channelNdx[0]].maxSfb;

                bsEnc->frameData.sideInfoBits +=
                    encodeMSInfo(bsEnc->bitEncChannelData[channelNdx[0]].sfbCnt,
                                 bsEnc->bitEncChannelData[channelNdx[0]].grpSfb,
                                 maxSfbSte,
                                 bsEnc->bitEncChannelData[channelNdx[0]].jsFlag,
                                 0,
                                 bsEnc->config.aot,
                                 bsEnc->bitEncChannelData[channelNdx[0]].bCplxPredMdct,
                                 bsEnc->bitEncChannelData[channelNdx[0]].predCoefRe,
                                 bsEnc->bitEncChannelData[channelNdx[0]].predCoefIm,
                                 bsEnc->bitEncChannelData[channelNdx[0]].predCoefPrevRe,
                                 bsEnc->bitEncChannelData[channelNdx[0]].predCoefPrevIm,
                                 bsEnc->bitEncChannelData[channelNdx[0]].bResetPredictors,
                                 bsEnc->bitEncChannelData[channelNdx[0]].nGroupsPrev,
                                 bsEnc->bitEncChannelData[channelNdx[0]].blockType,
                                 bsEnc->bitEncChannelData[channelNdx[0]].windowSequencePrev,
                                 bsEnc->bitEncChannelData[channelNdx[0]].sfbPerPredBand,
                                 bsEnc->bitEncChannelData[channelNdx[0]].bSwappedChannel,
                                 bsEnc->bitEncChannelData[channelNdx[0]].bPrevFrame,
                                 NULL,
                                 bUsacIndependenceFlag);
              }
            }

            bs_tns_data0 = &bsEnc->bitEncChannelData[channelNdx[0]].bs_tns_data;
            bs_tns_data1 = &bsEnc->bitEncChannelData[channelNdx[1]].bs_tns_data;

            bTnsActive = bs_tns_data0->tnsActive || bs_tns_data1->tnsActive;
            bCommonTns = bs_tns_data0->bCommonTns;

            bTnsDataPresent[0] = bs_tns_data0->tnsActive;
            bTnsDataPresent[1] = bs_tns_data1->tnsActive;

            bTnsDataPresentBoth = bTnsDataPresent[0] && bTnsDataPresent[1];

            if (bTnsActive) {
              if (bsEnc->bitEncChannelData[channelNdx[0]].commonWindow) {
                bsEnc->frameData.sideInfoBits += 1;
              }
              bsEnc->frameData.sideInfoBits += 1;

              if (bCommonTns) {
                bsEnc->frameData.sideInfoBits += encodeTnsData(bs_tns_data0, bsEnc->config.aot, NULL);
                bTnsDataPresent[0] = bTnsDataPresent[1] = 0;
              } else {
                bsEnc->frameData.sideInfoBits += 1;
                if (bTnsDataPresentBoth == 0) {
                  bsEnc->frameData.sideInfoBits += 1;
                }
              }
            }

            if (bsEnc->config.timeWarpedMdct) {
              assert(0);
              bsEnc->frameData.sideInfoBits += 1;
              bs_pitch_data = &bsEnc->bitEncChannelData[channelNdx[0]].bs_pitch_data;
              if (bs_pitch_data->commonPitch == 1) {
                bsEnc->frameData.sideInfoBits += encodePitchData(bs_pitch_data, NULL);
              }
            }

          } else {
            bTnsDataPresent[0] = bsEnc->bitEncChannelData[channelNdx[0]].bs_tns_data.tnsActive;
            bTnsDataPresent[1] = bsEnc->bitEncChannelData[channelNdx[1]].bs_tns_data.tnsActive;
          }

          for (ch = 0; ch < 2; ch++) {
            if (!useACE[ch]) {
              if (useACE[0] != useACE[1])
                bsEnc->frameData.sideInfoBits += 1;

              bs_tns_data = &bsEnc->bitEncChannelData[channelNdx[ch]].bs_tns_data;
              bs_pitch_data = &bsEnc->bitEncChannelData[channelNdx[ch]].bs_pitch_data;

              bsEnc->frameData.sideInfoBits += 8;
              bsEnc->frameData.sideInfoBits += 1;

              if (bsEnc->config.useNoiseFilling) {
                bsEnc->frameData.sideInfoBits += SI_ICS_BITS_NF;
              }
              if (bsEnc->bitEncChannelData[channelNdx[ch]].commonWindow == 0) {
                switch (bsEnc->frameData.blockType[channelNdx[ch]]) {
                  case LONG_WINDOW:
                  case START_WINDOW:
                  case STOP_WINDOW:
                  case STOPSTART_WINDOW:
                    bsEnc->frameData.sideInfoBits += 2 + 1;
                    bsEnc->frameData.sideInfoBits += 6;
                    break;

                  case SHORT_WINDOW:
                    bsEnc->frameData.sideInfoBits += 2 + 1;
                    bsEnc->frameData.sideInfoBits += 4 + 7;
                    break;

                  default:
                    WARN("Unknown window type");
                    break;
                }
              }

              if (bsEnc->config.timeWarpedMdct && bs_pitch_data->commonPitch == 0) {
                assert(0);

                bsEnc->frameData.sideInfoBits += encodePitchData(bs_pitch_data, NULL);
              }

              if (bTnsDataPresent[ch]) {
                bsEnc->frameData.sideInfoBits += encodeTnsData(bs_tns_data, bsEnc->config.aot, NULL);
              }
            }
          }
        }

        break;
      case ID_DSE:

        if (elem->element.dse.byteCount > 0) {
          bsEnc->frameData.dseBits += SI_ID_BITS + 4 + 1 + 8;
          if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
            bsEnc->frameData.sideInfoBits -= SI_ID_BITS;
          }
          if (elem->element.dse.byteCount >= 255) {
            bsEnc->frameData.dseBits += 8;
          }
          bsEnc->frameData.dseBits += elem->element.dse.byteCount * 8;
        }

        break;

      default:
        return iisUtil_ERROR(CDI, "unknown element type");
    }
    bsEnc->frameData.elemSideInfoBits[i] = bsEnc->frameData.sideInfoBits - bsEnc->frameData.elemSideInfoBits[i];
  }
  bsEnc->frameData.sideInfoBits += SI_ID_BITS;
  if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
    bsEnc->frameData.sideInfoBits -= SI_ID_BITS;
  }

  if (externalElemBits > 0 && !isVbr) {
    int bitsNeeded = (externalElemBits / 8) * 8;
    int bitResLevel = IISBITFRAME_GetBitreservoir(hBitFrame);
    int bitsGranted = 0;

    if ((IISBITFRAME_GetExtendedBitreservoirMax(hBitFrame) > 0) && bRAP == 1) {
      bitsNeeded += (headerBits / 8) * 8;
      IISBITFRAME_ExtendedBitReservoirRequestBits(hBitFrame, bitsNeeded, &bitsGranted);
    }

    bsEnc->frameData.totalFrameBits += bitsGranted;
    bsEnc->frameData.availableFrameBits += bitsGranted;
    bitsNeeded -= bitsGranted;

    if (bitsNeeded > bitResLevel) {
      bitsNeeded = bitResLevel;
    }
    bsEnc->frameData.totalFrameBits += bitsNeeded;
    bsEnc->frameData.availableFrameBits += bitsNeeded;
    IISBITFRAME_UpdateBitReservoir(hBitFrame, bitResLevel - bitsNeeded);
  }

  bsEnc->frameData.availableDynpartBits =
      bsEnc->frameData.availableFrameBits -
      bsEnc->frameData.sideInfoBits -
      bsEnc->frameData.dseBits -
      bsEnc->frameData.additionalElemBits;

  if (bsEnc->frameData.availableDynpartBits <= 0 && !isVbr) {
    int bitsNeeded = ((-bsEnc->frameData.availableDynpartBits + 7) / 8) * 8;
    int bitResLevel = IISBITFRAME_GetBitreservoir(hBitFrame);

    bsEnc->frameData.totalFrameBits += bitsNeeded;
    bsEnc->frameData.availableFrameBits += bitsNeeded;
    bsEnc->frameData.availableDynpartBits += bitsNeeded;
    IISBITFRAME_UpdateBitReservoir(hBitFrame, bitResLevel - bitsNeeded);
  }

  return (noError);
}

static int
writeIndividualChannelStream(int commonWindow,
                             BIT_ENC_CHANNEL_DATA *bitEncChannelData,
                             HANDLE_BIT_BUF hBitStream,
                             int useAcelp,
                             AUDIO_OBJECT_TYPE aot,
                             int bUsacIndepFlag,
                             int timeWarpedMdct,
                             int bTnsDataPresent,
                             int useNoiseFilling,
                             int writeBitstreamMode

)

{
  int bit_cnt = 0;
  int totBitCount = 0;

  totBitCount += encodeGlobalGain(&bitEncChannelData->bs_scalefac_data, bitEncChannelData->bs_section_data.firstSCF, hBitStream);
  if (aot == AOT_USAC || aot == AOT_MPEGH) {
    if (useNoiseFilling) {
      totBitCount += encodeNoiseLevel(bitEncChannelData->bs_scalefac_data.noiseLevel, hBitStream);
    }
  }

  if (!commonWindow) {
    totBitCount += bit_cnt = encodeIcsInfo(bitEncChannelData->blockType,
                                           bitEncChannelData->mdctWindowShape,
                                           bitEncChannelData->groupingMask,
                                           commonWindow,
                                           bitEncChannelData->maxSfb,
                                           &bitEncChannelData->bs_prediction_data,
                                           &bitEncChannelData->bs_ltp_data,
                                           NULL,
                                           bitEncChannelData->jsFlag,
                                           NULL,
                                           hBitStream,
                                           aot

    );
  }

  if (aot == AOT_USAC || aot == AOT_MPEGH) {
    if (timeWarpedMdct && bitEncChannelData->bs_pitch_data.commonPitch == 0) {
      totBitCount += encodePitchData(&bitEncChannelData->bs_pitch_data,
                                     hBitStream);
    }
  }
  (void)(useAcelp);

  totBitCount += bit_cnt = encodeScaleFactorData(bitEncChannelData->sfbBandOffset,
                                                 &bitEncChannelData->bs_section_data,
                                                 &bitEncChannelData->bs_scalefac_data,
                                                 bitEncChannelData->aQuantSpectrum,
                                                 aot,
                                                 useNoiseFilling,
                                                 hBitStream);
  if (aot == AOT_USAC || aot == AOT_MPEGH) {
    if (bTnsDataPresent) {
      totBitCount += bit_cnt = encodeTnsData(&bitEncChannelData->bs_tns_data, aot, hBitStream);
    }
  }

  if (bitEncChannelData->hEncSpecDataArith && (aot == AOT_USAC || aot == AOT_MPEGH)) {
    if (writeBitstreamMode == BITSTREAM_MODE_CODING) {
      totBitCount += bit_cnt = getSpectralDataArith2(bitEncChannelData->hEncSpecDataArith,
                                                     hBitStream,
                                                     bUsacIndepFlag);
    } else {
      totBitCount += bit_cnt = encodeSpectralDataArith2(bitEncChannelData->hEncSpecDataArith,
                                                        bitEncChannelData->sfbBandOffset,
                                                        bitEncChannelData->sfbCnt,
                                                        bitEncChannelData->grpSfb,
                                                        bitEncChannelData->maxSfb,
                                                        bitEncChannelData->groupingMask,
                                                        bitEncChannelData->blockType,
                                                        bitEncChannelData->aQuantSpectrum,
                                                        bUsacIndepFlag,
                                                        hBitStream);
    }
  }
  return totBitCount;
}

static int writeSingleChannelElement(int instanceTag,
                                     int channel,
                                     HANDLE_BITSTREAM_ENC bsEnc,
                                     HANDLE_BIT_BUF hBitStream,
                                     int useAcelp,
                                     int bUsacIndepFlag,
                                     int writeBitstreamMode

) {
  unsigned int crcTabIndex;
  int bitCnt = 0;

  if (bsEnc->config.aot != AOT_USAC && bsEnc->config.aot != AOT_MPEGH) {
    bitCnt += WriteBits(hBitStream, ID_SCE, 3);
  }

  crcTabIndex = CrcSetBegin(hBitStream, IDC_SCE);

  if (bsEnc->config.aot != AOT_USAC && bsEnc->config.aot != AOT_MPEGH) {
    bitCnt += WriteBits(hBitStream, instanceTag, 4);
  }

  if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
    bitCnt += WriteBits(hBitStream, useAcelp, 1);
    if (!useAcelp) {
      int fac_data_present = (bsEnc->bitEncChannelData[channel].FacDataBitCnt > 0) ? 1 : 0;

      bitCnt += WriteBits(hBitStream, bsEnc->bitEncChannelData[channel].bs_tns_data.tnsActive, 1);

      bitCnt += writeIndividualChannelStream(0,
                                             &bsEnc->bitEncChannelData[channel],
                                             hBitStream, useAcelp,
                                             bsEnc->config.aot,
                                             bUsacIndepFlag,
                                             bsEnc->config.timeWarpedMdct,
                                             bsEnc->bitEncChannelData[channel].bs_tns_data.tnsActive,
                                             bsEnc->config.useNoiseFilling,
                                             writeBitstreamMode

      );

      bitCnt += WriteBits(hBitStream, fac_data_present, 1);

    } else {
      int i, tmp = 0;
      BIT_ENC_CHANNEL_DATA *bEncData = &bsEnc->bitEncChannelData[channel];

      for (i = 0; i < bEncData->acelpDataBitCnt; i += 8) {
        int bitsToWrite = min(8, bEncData->acelpDataBitCnt - i);
        tmp += WriteBits(hBitStream, bEncData->acelpData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
      }
      assert(tmp == bEncData->acelpDataBitCnt);
      bitCnt += tmp;
    }

    {
      int i, tmp = 0;
      BIT_ENC_CHANNEL_DATA *bEncData = &bsEnc->bitEncChannelData[channel];
      if (bEncData->FacDataBitCnt > 0) {
        for (i = 0; i < bEncData->FacDataBitCnt; i += 8) {
          int bitsToWrite = min(8, bEncData->FacDataBitCnt - i);
          tmp += WriteBits(hBitStream, bEncData->FacData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
        }
        assert(tmp == bEncData->FacDataBitCnt);
        bitCnt += tmp;
      }
    }

  } else {
    bitCnt += writeIndividualChannelStream(0,
                                           &bsEnc->bitEncChannelData[channel],
                                           hBitStream,
                                           useAcelp,
                                           bsEnc->config.aot,
                                           bUsacIndepFlag,
                                           bsEnc->config.timeWarpedMdct,
                                           bsEnc->bitEncChannelData[channel].bs_tns_data.tnsActive,
                                           bsEnc->config.useNoiseFilling,
                                           writeBitstreamMode);
  }

  CrcSetEnd(hBitStream, crcTabIndex);

  return bitCnt;
}

static int writeChannelPairElement(int instanceTag,
                                   int channel1,
                                   int channel2,
                                   HANDLE_BITSTREAM_ENC bsEnc,
                                   int commonWindow,
                                   HANDLE_BIT_BUF hBitStream,
                                   int useAcelp[2],
                                   int bUsacIndepFlag,
                                   int writeBitstreamMode

) {
  int bit_cnt = 0;

  unsigned int crcTabIndex1, crcTabIndex2;
  int i = 0;
  int tmp = 0;
  int maxSfbSte;

  if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
    BIT_ENC_CHANNEL_DATA *bEncData = NULL;
    BS_TNS_DATA *bs_tns_data0 = &bsEnc->bitEncChannelData[channel1].bs_tns_data;
    BS_TNS_DATA *bs_tns_data1 = &bsEnc->bitEncChannelData[channel2].bs_tns_data;
    int bTnsActive, bCommonTns, bTnsDataPresentBoth;
    int bTnsDataPresent[2];

    bTnsActive = bs_tns_data0->tnsActive || bs_tns_data1->tnsActive;
    bCommonTns = bs_tns_data0->bCommonTns;
    bTnsDataPresent[0] = bs_tns_data0->tnsActive;
    bTnsDataPresent[1] = bs_tns_data1->tnsActive;
    bTnsDataPresentBoth = bTnsDataPresent[0] && bTnsDataPresent[1];

    crcTabIndex1 = CrcSetBegin(hBitStream, IDC_CPE);

    bit_cnt += WriteBits(hBitStream, useAcelp[0], 1);
    bit_cnt += WriteBits(hBitStream, useAcelp[1], 1);

    if (!useAcelp[0] && !useAcelp[1]) {
      bit_cnt += WriteBits(hBitStream, bTnsActive, 1);
      bit_cnt += WriteBits(hBitStream, commonWindow, 1);
      if (commonWindow) {
        bit_cnt += encodeIcsInfo(bsEnc->bitEncChannelData[channel1].blockType,
                                 bsEnc->bitEncChannelData[channel1].mdctWindowShape,
                                 bsEnc->bitEncChannelData[channel1].groupingMask,
                                 commonWindow,
                                 bsEnc->bitEncChannelData[channel1].maxSfb,
                                 &bsEnc->bitEncChannelData[channel1].bs_prediction_data,
                                 &bsEnc->bitEncChannelData[channel1].bs_ltp_data,
                                 &bsEnc->bitEncChannelData[channel2].bs_ltp_data,
                                 bsEnc->bitEncChannelData[channel1].jsFlag,
                                 bsEnc->bitEncChannelData[channel2].jsFlag,
                                 hBitStream,
                                 bsEnc->config.aot);

        bit_cnt += WriteBits(hBitStream, bsEnc->bitEncChannelData[channel1].maxSfb == bsEnc->bitEncChannelData[channel2].maxSfb, 1);
        if (bsEnc->bitEncChannelData[channel1].maxSfb != bsEnc->bitEncChannelData[channel2].maxSfb) {
          if (bsEnc->bitEncChannelData[channel1].blockType == SHORT_WINDOW) {
            bit_cnt += WriteBits(hBitStream, bsEnc->bitEncChannelData[channel2].maxSfb, 4);
          } else {
            bit_cnt += WriteBits(hBitStream, bsEnc->bitEncChannelData[channel2].maxSfb, 6);
          }
        }

        maxSfbSte = max(bsEnc->bitEncChannelData[channel1].maxSfb, bsEnc->bitEncChannelData[channel2].maxSfb);

        bit_cnt += encodeMSInfo(bsEnc->bitEncChannelData[channel1].sfbCnt,
                                bsEnc->bitEncChannelData[channel1].grpSfb,
                                maxSfbSte,
                                bsEnc->bitEncChannelData[channel1].jsFlag,
                                0,
                                bsEnc->config.aot,
                                bsEnc->bitEncChannelData[channel1].bCplxPredMdct,
                                bsEnc->bitEncChannelData[channel1].predCoefRe,
                                bsEnc->bitEncChannelData[channel1].predCoefIm,
                                bsEnc->bitEncChannelData[channel1].predCoefPrevRe,
                                bsEnc->bitEncChannelData[channel1].predCoefPrevIm,
                                bsEnc->bitEncChannelData[channel1].bResetPredictors,
                                bsEnc->bitEncChannelData[channel1].nGroupsPrev,
                                bsEnc->bitEncChannelData[channel1].blockType,
                                bsEnc->bitEncChannelData[channel1].windowSequencePrev,
                                bsEnc->bitEncChannelData[channel1].sfbPerPredBand,
                                bsEnc->bitEncChannelData[channel1].bSwappedChannel,
                                bsEnc->bitEncChannelData[channel1].bPrevFrame,
                                hBitStream,
                                bUsacIndepFlag);
      }

      if (bsEnc->config.timeWarpedMdct) {
        assert(0);
        bit_cnt += WriteBits(hBitStream, bsEnc->bitEncChannelData[channel1].bs_pitch_data.commonPitch, 1);
        if (bsEnc->config.timeWarpedMdct && (bsEnc->bitEncChannelData[channel1].bs_pitch_data.commonPitch == 1)) {
          bit_cnt += encodePitchData(&bsEnc->bitEncChannelData[channel1].bs_pitch_data, hBitStream);
        }
      }

      if (bTnsActive) {
        if (commonWindow) {
          bit_cnt += WriteBits(hBitStream, bCommonTns, 1);
        }

        if (bsEnc->config.aot == AOT_USAC) {
          bit_cnt += WriteBits(hBitStream, bs_tns_data0->bTnsOnLr, 1);
        }

        if (bsEnc->config.aot == AOT_MPEGH) {
          if (!bsEnc->config.useEnhancedNoiseFilling || bsEnc->config.igfAfterTnsSynth) {
            bit_cnt += WriteBits(hBitStream, bs_tns_data0->bTnsOnLr, 1);
          } else {
            assert(bs_tns_data0->bTnsOnLr == 1);
          }
        }
        if (bCommonTns) {
          bit_cnt += encodeTnsData(&bsEnc->bitEncChannelData[channel1].bs_tns_data, bsEnc->config.aot, hBitStream);
          bTnsDataPresent[0] = bTnsDataPresent[1] = 0;
        } else {
          bit_cnt += WriteBits(hBitStream, bTnsDataPresentBoth, 1);
          if (bTnsDataPresentBoth == 0) {
            bit_cnt += WriteBits(hBitStream, bTnsDataPresent[1], 1);
          }
        }
      }
    } else {
      commonWindow = 0;

      bsEnc->bitEncChannelData[channel1].bs_pitch_data.commonPitch = 0;
      bsEnc->bitEncChannelData[channel2].bs_pitch_data.commonPitch = 0;
    }

    bEncData = &bsEnc->bitEncChannelData[channel1];
    if (useAcelp[0]) {
      for (i = 0; i < bEncData->acelpDataBitCnt; i += 8) {
        int bitsToWrite = min(8, bEncData->acelpDataBitCnt - i);
        bit_cnt += WriteBits(hBitStream, bEncData->acelpData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
      }
    } else {
      int fac_data_present = (bsEnc->bitEncChannelData[channel1].FacDataBitCnt > 0) ? 1 : 0;

      if (useAcelp[0] != useAcelp[1])
        bit_cnt += WriteBits(hBitStream, bTnsDataPresent[0], 1);

      bit_cnt += writeIndividualChannelStream(commonWindow,
                                              &bsEnc->bitEncChannelData[channel1],
                                              hBitStream,
                                              useAcelp[0],
                                              bsEnc->config.aot,
                                              bUsacIndepFlag,
                                              bsEnc->config.timeWarpedMdct,
                                              bTnsDataPresent[0],
                                              bsEnc->config.useNoiseFilling,
                                              writeBitstreamMode

      );

      bit_cnt += WriteBits(hBitStream, fac_data_present, 1);
    }

    tmp = 0;

    if (bEncData->FacDataBitCnt > 0) {
      for (i = 0; i < bEncData->FacDataBitCnt; i += 8) {
        int bitsToWrite = min(8, bEncData->FacDataBitCnt - i);
        tmp += WriteBits(hBitStream, bEncData->FacData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
      }
      assert(tmp == bEncData->FacDataBitCnt);
      bit_cnt += tmp;
    }

    crcTabIndex2 = CrcSetBegin(hBitStream, IDC_2ICS);
    bEncData = &bsEnc->bitEncChannelData[channel2];
    if (useAcelp[1]) {
      for (i = 0; i < bEncData->acelpDataBitCnt; i += 8) {
        int bitsToWrite = min(8, bEncData->acelpDataBitCnt - i);
        bit_cnt += WriteBits(hBitStream, bEncData->acelpData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
      }
    } else {
      int fac_data_present = (bsEnc->bitEncChannelData[channel2].FacDataBitCnt > 0) ? 1 : 0;

      if (useAcelp[0] != useAcelp[1])
        bit_cnt += WriteBits(hBitStream, bTnsDataPresent[1], 1);

      bit_cnt += writeIndividualChannelStream(commonWindow,
                                              &bsEnc->bitEncChannelData[channel2],
                                              hBitStream,
                                              useAcelp[1],
                                              bsEnc->config.aot,
                                              bUsacIndepFlag,
                                              bsEnc->config.timeWarpedMdct,
                                              bTnsDataPresent[1],
                                              bsEnc->config.useNoiseFilling,
                                              writeBitstreamMode);

      bit_cnt += WriteBits(hBitStream, fac_data_present, 1);
    }

    tmp = 0;

    if (bEncData->FacDataBitCnt > 0) {
      for (i = 0; i < bEncData->FacDataBitCnt; i += 8) {
        int bitsToWrite = min(8, bEncData->FacDataBitCnt - i);
        tmp += WriteBits(hBitStream, bEncData->FacData[i / 8] >> (8 - bitsToWrite), bitsToWrite);
      }
      assert(tmp == bEncData->FacDataBitCnt);
      bit_cnt += tmp;
    }

    CrcSetEnd(hBitStream, crcTabIndex1);
    CrcSetEnd(hBitStream, crcTabIndex2);
  } else {
    bit_cnt += WriteBits(hBitStream, ID_CPE, 3);

    crcTabIndex1 = CrcSetBegin(hBitStream, IDC_CPE);

    bit_cnt += WriteBits(hBitStream, instanceTag, 4);
    bit_cnt += WriteBits(hBitStream, commonWindow, 1);
    if (commonWindow) {
      bit_cnt += encodeIcsInfo(bsEnc->bitEncChannelData[channel1].blockType,
                               bsEnc->bitEncChannelData[channel1].mdctWindowShape,
                               bsEnc->bitEncChannelData[channel1].groupingMask,
                               commonWindow,
                               bsEnc->bitEncChannelData[channel1].maxSfb,
                               &bsEnc->bitEncChannelData[channel1].bs_prediction_data,
                               &bsEnc->bitEncChannelData[channel1].bs_ltp_data,
                               &bsEnc->bitEncChannelData[channel2].bs_ltp_data,
                               bsEnc->bitEncChannelData[channel1].jsFlag,
                               bsEnc->bitEncChannelData[channel2].jsFlag,
                               hBitStream,
                               bsEnc->config.aot);

      bit_cnt += encodeMSInfo(bsEnc->bitEncChannelData[channel1].sfbCnt,
                              bsEnc->bitEncChannelData[channel1].grpSfb,
                              bsEnc->bitEncChannelData[channel1].maxSfb,
                              bsEnc->bitEncChannelData[channel1].jsFlag,
                              0,
                              bsEnc->config.aot,
                              0, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0,
                              hBitStream,
                              bUsacIndepFlag);
    }

    bit_cnt += writeIndividualChannelStream(commonWindow,
                                            &bsEnc->bitEncChannelData[channel1],
                                            hBitStream,
                                            0,
                                            bsEnc->config.aot,
                                            bUsacIndepFlag,
                                            bsEnc->config.timeWarpedMdct,
                                            0,
                                            bsEnc->config.useNoiseFilling,
                                            writeBitstreamMode);

    crcTabIndex2 = CrcSetBegin(hBitStream, IDC_2ICS);

    bit_cnt += writeIndividualChannelStream(commonWindow,
                                            &bsEnc->bitEncChannelData[channel2],
                                            hBitStream,
                                            0,
                                            bsEnc->config.aot,
                                            bUsacIndepFlag,
                                            bsEnc->config.timeWarpedMdct,
                                            0,
                                            bsEnc->config.useNoiseFilling,
                                            writeBitstreamMode);

    CrcSetEnd(hBitStream, crcTabIndex1);
    CrcSetEnd(hBitStream, crcTabIndex2);
  }

  return bit_cnt;
}

static HANDLE_ERROR_INFO USAC_byteAlignment(
    HUFFCODEBOOK HANDLE_BITSTREAM_ENC bsEnc,
    HANDLE_BIT_BUF hBitStream,
    int alignBits,
    int *nBits) {
  int bitCount = 0;

  if (alignBits >= 8)
    return iisUtil_ERROR(CDI, "alignment bits >= 8");

  bitCount += WriteBits(hBitStream, 0, alignBits);

  if (nBits != NULL) {
    *nBits += bitCount;
  }

  return (noError);
}

static const struct s_fillElementConfig {
  int fillBitsMin;
  int fillBitsMax;
  int bExtElementPresent;
  int bUseDefaultLength;
  int bEscape;

} fillElementConfigTable[] = {

    {0, 7, 0, 0, 0},
    {8, 8, 1, 1, 0},
    {9, 2048, 1, 0, 0},
    {2049, (255 + 65535) * 8, 1, 0, 1}};

static HANDLE_ERROR_INFO resetExtEleFilInfo(BS_EXT_ELE_FIL_INFO *const extEleFilInfo) {
  HANDLE_ERROR_INFO err = noError;

  extEleFilInfo->extEleFil_present = 0;
  extEleFilInfo->extEleFil_posBgn = 0;
  extEleFilInfo->extEleFil_posEnd = 0;
  extEleFilInfo->extEleFil_numBits = 0;

  return err;
}

static HANDLE_ERROR_INFO setExtEleFilInfo(BS_EXT_ELE_FIL_INFO *const extEleFilInfo,
                                          const int bs_extIsPresent,
                                          const int bs_posBgn,
                                          const int bs_posEnd,
                                          const int bs_numBits) {
  HANDLE_ERROR_INFO err = noError;

  resetExtEleFilInfo(extEleFilInfo);

  if (bs_extIsPresent == 1) {
    if ((bs_posEnd - bs_posBgn) == bs_numBits) {
      extEleFilInfo->extEleFil_present = bs_extIsPresent;
      extEleFilInfo->extEleFil_posBgn = bs_posBgn;
      extEleFilInfo->extEleFil_posEnd = bs_posEnd;
      extEleFilInfo->extEleFil_numBits = bs_numBits;
    } else {
      err = iisUtil_ERROR(CDI, "Invalid Extension Element Fill data.");
    }
  }

  return err;
}

static HANDLE_ERROR_INFO WriteUsacFillElement(HANDLE_BITSTREAM_ENC hBsEnc,
                                              int *nBitsByteAlign,
                                              int *nBits) {
  HANDLE_ERROR_INFO err = noError;
  int i = 0;
  int len = 0;
  int countBits = 0;
  int bs_posBgn = 0;

  int const tableSize = sizeof(fillElementConfigTable) / sizeof(struct s_fillElementConfig);
  int nFillBitsToWrite;

  if ((hBsEnc != NULL) && (nBitsByteAlign != NULL)) {
    nFillBitsToWrite = hBsEnc->frameData.totFillBits;
    *nBitsByteAlign = 0;

    for (i = 0; i < tableSize; i++) {
      if ((nFillBitsToWrite >= fillElementConfigTable[i].fillBitsMin) &&
          (nFillBitsToWrite <= fillElementConfigTable[i].fillBitsMax)) {
        bs_posBgn = GetBitsAvail(hBsEnc->mxBitstream);

        countBits += WriteBits(hBsEnc->mxBitstream, fillElementConfigTable[i].bExtElementPresent, 1);

        if (fillElementConfigTable[i].bExtElementPresent) {
          countBits += WriteBits(hBsEnc->mxBitstream, fillElementConfigTable[i].bUseDefaultLength, 1);
          nFillBitsToWrite--;

          len = (nFillBitsToWrite / 8);
          len--;

          if (fillElementConfigTable[i].bUseDefaultLength == 0) {
            if (fillElementConfigTable[i].bEscape) {
              len -= 2;

              countBits += WriteBits(hBsEnc->mxBitstream, 255, 8);
              countBits += WriteBits(hBsEnc->mxBitstream, max(0, len - 255 + 2), 16);

            } else {
              countBits += WriteBits(hBsEnc->mxBitstream, len, 8);
            }
          }

          if (len > 0) {
            while (len--) {
              countBits += WriteBits(hBsEnc->mxBitstream, 0xa5, 8);
            }
          }
        }

        *nBitsByteAlign = (nFillBitsToWrite % 8);

        err = setExtEleFilInfo(&hBsEnc->extEleFilInfo,
                               (int)fillElementConfigTable[i].bExtElementPresent,
                               (int)bs_posBgn,
                               (int)GetBitsAvail(hBsEnc->mxBitstream),
                               (int)countBits);
      }
    }

    if (nBits != NULL) {
      *nBits = countBits;
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO WriteBitstreamInternal(
    HANDLE_BITSTREAM_ENC bsEnc,
    const int *additionalElemBits,
    unsigned char const *const *additionalElemData,
    const int *saasSbrBits,
    const unsigned char **saasSbrPayload,
    int *bitstreamOutBytes,
    HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
    int *coreMode,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER *hExternalContainer,
    int numExternalContainersInUse,
    int *minBitsForValidBitstream,
    const int writeBitstreamMode) {
  int i;
  int bitsInBuffer;
  int bitCount = 0;
  int const sbrDataPresent = (saasSbrPayload != NULL);

  int useAcelp[2] = {0};
  int coreModeFD[2] = {0};

  int nBitsByteAlign = 0;

  BS_CHANNEL_MAPPING *channelMapping = &(bsEnc->config.channelMapping);

  HANDLE_ERROR_INFO err;

  int nBitsOutOfBand = 0;

  HANDLE_BIT_BUF hMxBitstream = NULL;
  {
    hMxBitstream = bsEnc->mxBitstream;
  }

  if (bitstreamOutBytes)
    *bitstreamOutBytes = 0;

  resetExtEleFilInfo(&bsEnc->extEleFilInfo);

  if (coreMode == NULL)
    coreMode = coreModeFD;

  bitsInBuffer = GetBitsAvail(hMxBitstream);

  if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
    bitCount += WriteBits(hMxBitstream, bUsacIndepFlag ? 1 : 0, 1);
  }

  switch (bsEnc->config.streamType) {
    case MPEG2_AAC:
    case MPEG4_AAC: {
      int channelNdx = 0;
      {
        int j = 0;

        for (j = 0; j < numExternalContainersInUse; ++j) {
          if (getChannelOffset(hExternalContainer[j]) == 0) {
            unsigned int containerBits = 0;

            if (0 != writeExtensionPayloadElement(hMxBitstream, hExternalContainer[j], 0, &containerBits)) {
              return iisUtil_ERROR(CDI, "extension payload field could not be written");
            }
            if (hasExtensionPayloadContainerFeature(hExternalContainer[j], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
              nBitsOutOfBand += containerBits;
            } else {
              bsEnc->frameData.totDynBits += (int)containerBits;
            }

            bitCount += (int)containerBits;
          }
        }
      }

      for (i = 0; i < channelMapping->noOfElements; i++) {
        int nChannelElemBits = 0;
        BS_MAPPING_ELEMENT *elem;
        elem = channelMapping->element[i];

        switch (elem->elementType) {
          case ID_SCE:
            useAcelp[0] = (coreMode[elem->element.sce.channelNdx] == 1);

            nChannelElemBits = writeSingleChannelElement(elem->instanceTag,
                                                         elem->element.sce.channelNdx,
                                                         bsEnc,
                                                         hMxBitstream, useAcelp[0],
                                                         bUsacIndepFlag, writeBitstreamMode);
            if (nChannelElemBits == 0) {
              return iisUtil_ERROR(CDI, "No SCE data written, although SCE signaled");
            }

            channelNdx++;
            break;

          case ID_CPE:

            useAcelp[0] = (coreMode[elem->element.cpe.channelNdx[0]] == 1);

            if ((bsEnc->config.aot != AOT_MPEGH) || (elem->element.cpe.nChannels == 2)) {
              useAcelp[1] = (coreMode[elem->element.cpe.channelNdx[1]] == 1);

              if (bsEnc->bitEncChannelData[elem->element.cpe.channelNdx[0]].commonWindow !=
                  bsEnc->bitEncChannelData[elem->element.cpe.channelNdx[1]].commonWindow) {
                return iisUtil_ERROR(CDI, "commonWindow mismatch");
              }

              nChannelElemBits += writeChannelPairElement(elem->instanceTag,
                                                          elem->element.cpe.channelNdx[0],
                                                          elem->element.cpe.channelNdx[1],
                                                          bsEnc,
                                                          bsEnc->bitEncChannelData[elem->element.cpe.channelNdx[0]].commonWindow,
                                                          hMxBitstream, useAcelp,
                                                          bUsacIndepFlag, writeBitstreamMode);

              if (nChannelElemBits == 0) {
                return iisUtil_ERROR(CDI, "No CPE data written, although CPE signaled");
              }

              channelNdx += 2;
            } else {
              nChannelElemBits += writeSingleChannelElement(elem->instanceTag,
                                                            elem->element.cpe.channelNdx[0],
                                                            bsEnc,
                                                            hMxBitstream, useAcelp[0],
                                                            bUsacIndepFlag, writeBitstreamMode);
              if (nChannelElemBits == 0) {
                return iisUtil_ERROR(CDI, "No mono CPE data written, although mono CPE signaled");
              }
              channelNdx += 2;
            }

            break;

          default:
            return iisUtil_ERROR(CDI, "unknown element type");
        }

        bitCount += nChannelElemBits;

        if (additionalElemBits) {
          int j;
          int unalignedBits = additionalElemBits[i] % 8;
          int nAdditionalElemBits = 0;

          for (j = 0; j < additionalElemBits[i] >> 3; j++) {
            nAdditionalElemBits += WriteBits(hMxBitstream, additionalElemData[i][j], 1 << 3);
          }
          if (unalignedBits > 0)
            nAdditionalElemBits += WriteBits(hMxBitstream, additionalElemData[i][j] >> (8 - unalignedBits), unalignedBits);

          if (nAdditionalElemBits != additionalElemBits[i]) {
            return iisUtil_ERROR(CDI, "additionalElemBits not equal to the number of bits written");
          }

          bsEnc->frameData.totDynBits += additionalElemBits[i];

          bitCount += nAdditionalElemBits;
        }

        if (hExtContainer != NULL) {
          unsigned int containerBits = 0;

          if (0 != writeExtensionPayloadElement(hMxBitstream, hExtContainer, i, &containerBits)) {
            return iisUtil_ERROR(CDI, "extension payload field could not be written");
          }
          bsEnc->frameData.totDynBits += (int)containerBits;

          bitCount += (int)containerBits;
        }

        if ((bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) && (ID_LFE != elem->elementType) && (sbrDataPresent)) {
          if ((saasSbrPayload[i] != NULL) && (saasSbrBits[i] > 0)) {
            int bitCnt;
            for (bitCnt = 0; bitCnt < saasSbrBits[i]; bitCnt += 8) {
              bitCount += WriteBits(hMxBitstream, saasSbrPayload[i][bitCnt / 8], min(8, saasSbrBits[i] - bitCnt));
            }
            bsEnc->frameData.totDynBits += saasSbrBits[i];
          } else {
            return iisUtil_ERROR(CDI, "SBR payload missing.");
          }
        }

        if (elem->elementType != ID_DSE) {
          int j;
          for (j = 0; j < numExternalContainersInUse; ++j) {
            if (getChannelOffset(hExternalContainer[j]) == channelNdx) {
              unsigned int containerBits = 0;

              if (0 != writeExtensionPayloadElement(hMxBitstream, hExternalContainer[j], 0, &containerBits)) {
                return iisUtil_ERROR(CDI, "extension payload field could not be written");
              }
              if (hasExtensionPayloadContainerFeature(hExternalContainer[j], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
                nBitsOutOfBand += containerBits;
              } else {
                bsEnc->frameData.totDynBits += (int)containerBits;
              }
              bitCount += (int)containerBits;
            }
          }
        }
      }
      assert(channelNdx == bsEnc->config.nChannels);
      break;
    }

    default:
      return iisUtil_ERROR(CDI, "wrong AAC stream type");
  }

  {
    int j = 0;

    for (j = 0; j < numExternalContainersInUse; ++j) {
      if (getChannelOffset(hExternalContainer[j]) == -1) {
        unsigned int containerBits = 0;

        if (0 != writeExtensionPayloadElement(hMxBitstream, hExternalContainer[j], 0, &containerBits)) {
          return iisUtil_ERROR(CDI, "extension payload field could not be written");
        }
        if (hasExtensionPayloadContainerFeature(hExternalContainer[j], FEATURE_USAC_EXT_PAYLOAD_OUT_OF_BITRES)) {
          nBitsOutOfBand += containerBits;
        } else {
          bsEnc->frameData.totDynBits += (int)containerBits;
        }
        bitCount += (int)containerBits;
      }
    }
  }

  if (writeBitstreamMode == BITSTREAM_MODE_CODING) {
    if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
      int nUSACFillBits = 0;

      if (nBitsOutOfBand > 0) {
        bsEnc->frameData.totFillBits -= (nBitsOutOfBand % 8);
        if (bsEnc->frameData.totFillBits < 0) {
          bsEnc->frameData.totFillBits += 8;
        }
      }

      err = WriteUsacFillElement(bsEnc, &nBitsByteAlign, &nUSACFillBits);
      if (err != noError) return handBack(err);
      err = USAC_byteAlignment(bsEnc, hMxBitstream, nBitsByteAlign, &nUSACFillBits);
      if (err != noError) return handBack(err);
    }

    bitsInBuffer = GetBitsAvail(hMxBitstream) - bitsInBuffer;
    err = IncAuReady(hMxBitstream, bitsInBuffer);
    if (err != noError) return handBack(err);

    if (bitsInBuffer - nBitsOutOfBand != (bsEnc->frameData.totDynBits +
                                          bsEnc->frameData.sideInfoBits +
                                          bsEnc->frameData.dseBits +
                                          bsEnc->frameData.totFillBits)) {
      return iisUtil_ERROR(CDI, "no of counted and calculated Bits differ");
    }

    bsEnc->DataReadyToWrite = 1;
  }

  if (writeBitstreamMode == BITSTREAM_MODE_SIMULATE && minBitsForValidBitstream != NULL) {
    *minBitsForValidBitstream = bitCount;
  }

  bsEnc->usacByteAlignmentBits = nBitsByteAlign;

  return (noError);
}

HANDLE_ERROR_INFO WriteBitstream(
    HANDLE_BITSTREAM_ENC bsEnc,
    const int *additionalElemBits,
    unsigned char const *const *additionalElemData,
    const int *saasSbrBits,
    const unsigned char **saasSbrPayload,
    int *bitstreamOutBytes,
    HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
    int *coreMode,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER *hExternalContainer,
    int numExternalContainersInUse) {
  HANDLE_ERROR_INFO err = noError;

  if (!bsEnc) {
    err = iisUtil_ERROR(CDI, "cannot be a NULL bsEnc handle");
  }

  if (err == noError) {
    err = WriteBitstreamInternal(bsEnc,
                                 additionalElemBits,
                                 additionalElemData,
                                 saasSbrBits,
                                 saasSbrPayload,
                                 bitstreamOutBytes,
                                 hExtContainer,
                                 (int *)coreMode,
                                 bUsacIndepFlag,
                                 hExternalContainer,
                                 numExternalContainersInUse,
                                 NULL,
                                 BITSTREAM_MODE_CODING);
  }
  return err;
}

extern HANDLE_ERROR_INFO WriteBitstreamSimulationMode(
    HANDLE_BITSTREAM_ENC bsEnc,
    const int *additionalElemBits,
    unsigned char const *const *additionalElemData,
    const int *saasSbrBits,
    const unsigned char **saasSbrPayload,
    int *bitstreamOutBytes,
    HANDLE_EXTPAYLOAD_CONTAINER hExtContainer,
    int *coreMode,
    const int bUsacIndepFlag,
    HANDLE_EXTPAYLOAD_CONTAINER *hExternalContainer,
    int numExternalContainersInUse,
    int *minBitsForValidBitstream

) {
  HANDLE_ERROR_INFO err = noError;

  if (bsEnc == NULL) {
    err = iisUtil_ERROR(CDI, "cannot be a NULL bsEnc handle");
  }

  if (minBitsForValidBitstream == NULL) {
    err = iisUtil_ERROR(CDI, "cannot be a NULL minBitsForValidBitstream for counting bits in simulation mode");
  }

  if (err == noError) {
    err = WriteBitstreamInternal(bsEnc,
                                 additionalElemBits,
                                 additionalElemData,
                                 saasSbrBits,
                                 saasSbrPayload,
                                 bitstreamOutBytes,
                                 hExtContainer,
                                 (int *)coreMode,
                                 bUsacIndepFlag,
                                 hExternalContainer,
                                 numExternalContainersInUse,
                                 minBitsForValidBitstream,
                                 BITSTREAM_MODE_SIMULATE);
  }

  return err;
}

void DeleteBitstreamEncoder(HANDLE_BITSTREAM_ENC bsEnc) {
  if (bsEnc) {
    if (bsEnc->config.channelMapping.element) {
      int i;
      if (bsEnc->config.aot == AOT_USAC || bsEnc->config.aot == AOT_MPEGH) {
        for (i = 0; i < bsEnc->config.channelMapping.noOfChannels; i++) {
          destroySpectralDataArith2(&(bsEnc)->bitEncChannelData[i].hEncSpecDataArith);
        }
      }

      for (i = 0; i < bsEnc->config.channelMapping.noOfElements; i++) {
        if (bsEnc->config.channelMapping.element[i]) {
          if (bsEnc->config.channelMapping.element[i]->elementType == ID_CCE)
            iisFree(bsEnc->config.channelMapping.element[i]->element.cce.coupledWith);
          iisFree(bsEnc->config.channelMapping.element[i]);
        }
      }
      iisFree(bsEnc->config.channelMapping.element);
    }

    if (bsEnc->config.writeThr.file)
      fclose(bsEnc->config.writeThr.file);

    if (bsEnc->mxHuffCodeBookBuffer)
      DeleteBitBuffer(bsEnc->mxHuffCodeBookBuffer);

    if (bsEnc->mxBitstream)
      DeleteBitBuffer(bsEnc->mxBitstream);

    iisFree(bsEnc);
  }
}

HANDLE_BIT_BUF
bsencGetBitstream(HANDLE_BITSTREAM_ENC bsEnc) {
  return bsEnc->mxBitstream;
}
