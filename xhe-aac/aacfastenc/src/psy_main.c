
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
#include <float.h>
#include <math.h>

#include "aacenc_internal.h"
#include "iisSigMap.h"
#include "iisutillib.h"
#include "pns.h"
#include "transform.h"
#include "cplxpred_stereo.h"
#include "grp_data.h"
#include "spreading.h"
#include "glob_con.h"
#include "band_nrg.h"
#include "tns_func.h"
#include "ms_stereo.h"
#include "block_switch.h"
#include "psy_main.h"
#include "mathlib.h"
#include "time_buffer.h"
#include "unified_stereo_psych.h"
#include "psy_configuration.h"
#include "pre_echo_control.h"
#include "psy_data.h"
#include "sf_estim.h"

#define NEARLY_FLT_MAX (FLT_MAX * 1.e-2f)

static int iisaacfenc_advancePsychLong(PSY_DATA* psyData,
                                       TNS_DATA* tnsData,
                                       const PSY_CONFIGURATION psyConf[2],
                                       PSY_OUT_CHANNEL* psyOutChannel,
                                       float* pScratch,
                                       MDCT_SPECTRUM* originalMdctSpectrum,
                                       TNS_DATA* tnsData2,
                                       PSY_OUT_CHANNEL* psyOutChannel2,
                                       const int ch,
                                       HANDLE_LAPPED_TRANSFORM hLappedTransform,
                                       int useACEPrev,
                                       int useACENext,
                                       const int prevBlockType,
                                       const int commonWindow,
                                       int extLowpassLine);

static int iisaacfenc_advancePsychLongMS(PSY_DATA* psyData[2],
                                         const PSY_CONFIGURATION psyConf[2],
                                         struct STEREO_INFO* toolsInfo);

static int iisaacfenc_advancePsychShort(PSY_DATA* psyData,
                                        TNS_DATA* tnsData,
                                        const PSY_CONFIGURATION psyConf[2],
                                        PSY_OUT_CHANNEL* psyOutChannel,
                                        float* pScratch,
                                        MDCT_SPECTRUM* originalMdctSpectrum,
                                        TNS_DATA* tnsData2,
                                        PSY_OUT_CHANNEL* psyOutChannel2,
                                        const int ch,
                                        HANDLE_LAPPED_TRANSFORM hLappedTransform,
                                        int useACEPrev,
                                        int useACENext,
                                        const int prevBlockType,
                                        const int commonWindow,
                                        int extLowpassLine

);

static int iisaacfenc_advancePsychShortMS(PSY_DATA* psyData[2],
                                          const PSY_CONFIGURATION psyConf[2],
                                          struct STEREO_INFO* toolsInfo);

static void psyStereo(
    const AACENC_CODEC_TYPE codecType,
    PSY_OUT_ELEMENT* psyOutElement,
    PSY_DATA* psyData[2],
    TNS_DATA* tnsData[2],
    const PSY_CONFIGURATION* psyConf,
    const int groupedSfbCnt[2],
    int maxSfbPerGroup[2],
    PSY_OUT_CHANNEL* psyOutChannel[2],
    int* ptrSfbOffset[2],
    const HANDLE_UNISTE hUniSte,
    const MDCT_SPECTRUM* currMdstEstimate,
    const MDCT_SPECTRUM* lastMdctSpectrum);

static void iisaacfenc_PrepareCplxPredLong(PSY_DATA* psyData[2],
                                           const PSY_CONFIGURATION psyConf[2],
                                           MDCT_SPECTRUM* mdstEstimate,
                                           struct STEREO_INFO* steInfo);
static void iisaacfenc_PrepareCplxPredShort(PSY_DATA* psyData[2],
                                            const PSY_CONFIGURATION psyConf[2],
                                            MDCT_SPECTRUM* mdstEstimate,
                                            float* pScratch,
                                            struct STEREO_INFO* steInfo);

int iisaacfenc_PsyNew(
    PSY_INTERNAL** phPsy,
    int nChan,
    int granuleLength) {
  int error = 0;
  int i;
  int memoryIncrease = 0;
  PSY_INTERNAL* hPsy = NULL;

  if (*phPsy == NULL) {
    *phPsy = (PSY_INTERNAL*)iisCalloc(1, sizeof(PSY_INTERNAL));
    if (*phPsy == NULL) {
      error = 1;
    } else {
      memoryIncrease = 1;
    }
  } else {
    if (granuleLength > (*phPsy)->granuleLength) {
      memoryIncrease = 1;
    } else if (granuleLength < (*phPsy)->granuleLength) {
      memoryIncrease = -1;
    }
  }

  if (!error) {
    hPsy = (*phPsy);
    hPsy->granuleLength = granuleLength;
    for (i = 0; i < nChan; i++) {
      if (hPsy->psyData[i] == NULL) {
        hPsy->psyData[i] = (PSY_DATA*)iisCalloc(sizeof(PSY_DATA), 1);
        if (hPsy->psyData[i] == NULL) {
          error = 1;
        }
      }
      if (!error) {
        HANDLE_ERROR_INFO tmpError = noError;
        const int nDelay = hPsy->granuleLength + 3 * (hPsy->granuleLength / TRANS_FAC) + (hPsy->granuleLength / (2 * TRANS_FAC)) + (hPsy->granuleLength / TRANS_FAC);
        const int inputBufferSize = hPsy->granuleLength * 3;
        const int simultSpaceSize = nDelay;
        const int buffWriteOffset = nDelay;

        if (!error) {
          if (memoryIncrease != 0) {
            if (hPsy->psyData[i]->psyInputBuffer) {
              MP4TIMEBUF_Delete(&(hPsy->psyData[i]->psyInputBuffer));
            }
            tmpError = MP4TIMEBUF_Create(&(hPsy->psyData[i]->psyInputBuffer),
                                         inputBufferSize,
                                         simultSpaceSize,
                                         1,
                                         buffWriteOffset);
          } else if (hPsy->psyData[i]->psyInputBuffer == NULL) {
            tmpError = MP4TIMEBUF_Create(&(hPsy->psyData[i]->psyInputBuffer),
                                         inputBufferSize,
                                         simultSpaceSize,
                                         1,
                                         buffWriteOffset);
          }
        }
        if (tmpError != noError) {
          error = 1;
          freeErrorTraceback(tmpError);
        }
      }

      if (!error) {
        if (hPsy->tnsData[i] == NULL) {
          hPsy->tnsData[i] = (TNS_DATA*)iisCalloc(sizeof(TNS_DATA), 1);
          if (hPsy->tnsData[i] == NULL) {
            error = 1;
          }
        }
      }

      if (error != 0) {
        break;
      }
    }
  }

  if (!error) {
    TRANSFORM_GRANULE_LEN granuleLengthTf = TRANSFORM_GRANULE_LEN_INVALID;

    switch (granuleLength) {
      case 1024:
        granuleLengthTf = TRANSFORM_GRANULE_LEN_1024;
        break;
      case 768:
        granuleLengthTf = TRANSFORM_GRANULE_LEN_768;
        break;
      default:
        error = 1;
        break;
    }
    if (!error) {
      error = iisaacfenc_CreateLappedTransform(&hPsy->hLappedTransform, granuleLengthTf);
    }
  }

  if (!error) {
    if (memoryIncrease > 0) {
      if (hPsy->pScratch != NULL) {
        iisFree(hPsy->pScratch);
      }
      hPsy->pScratch = (float*)iisCalloc(hPsy->granuleLength * 2 * sizeof(float), 1);
      if (hPsy->pScratch == NULL) {
        error = 1;
      }
    }
  }

  if (error != 0) {
    iisaacfenc_PsyDelete(hPsy);
    hPsy = NULL;
  }

  return error;
}

int iisaacfenc_psyGetDelay(PSY_INTERNAL* hPsy) {
  int nDelay = hPsy->granuleLength + hPsy->granuleLength / (2 * TRANS_FAC) + 4 * (hPsy->granuleLength / TRANS_FAC);
  return nDelay;
}

int iisaacfenc_PsyDelete(PSY_INTERNAL* hPsy) {
  int error = 0;
  int i;

  if (hPsy) {
    if (hPsy->pScratch)
      iisFree(hPsy->pScratch);

    for (i = 0; i < SIGMAP_MAX_SIGNALS; i++) {
      if (hPsy->tnsData[i])
        iisFree(hPsy->tnsData[i]);

      if (hPsy->psyData[i]) {
        if (hPsy->psyData[i]->psyInputBuffer) {
          HANDLE_ERROR_INFO tmpError = noError;
          tmpError = MP4TIMEBUF_Delete(&(hPsy->psyData[i]->psyInputBuffer));
          if (tmpError != noError) {
            freeErrorTraceback(tmpError);
          }
        }
        iisFree(hPsy->psyData[i]);
      };
    }

    if (hPsy->hLappedTransform) {
      iisaacfenc_DestroyLappedTransform(&hPsy->hLappedTransform);
    }

    iisFree(hPsy);

    hPsy = 0;
  }
  return error;
}

int iisaacfenc_PsyOutNew(PSY_OUT** phpsyOut, int nChan, int nElem) {
  PSY_OUT* hPsyOut;
  int error = 0;
  int i;

  if (*phpsyOut == NULL) {
    *phpsyOut = (PSY_OUT*)iisCalloc(1, sizeof(PSY_OUT));
    error = (*phpsyOut == 0);
  }
  hPsyOut = *phpsyOut;

  if (!error) {
    for (i = 0; i < nElem; i++) {
      if (hPsyOut->psyOutElement[i] == NULL) {
        hPsyOut->psyOutElement[i] = (PSY_OUT_ELEMENT*)iisCalloc(sizeof(PSY_OUT_ELEMENT), 1);
        error = (hPsyOut->psyOutElement[i] == 0);
        if (error != 0) break;
      }
    }
  };

  if (!error) {
    for (i = 0; i < nChan; i++) {
      if (hPsyOut->psyOutChannel[i] == NULL) {
        hPsyOut->psyOutChannel[i] = (PSY_OUT_CHANNEL*)iisCalloc(sizeof(PSY_OUT_CHANNEL), 1);
        error = (hPsyOut->psyOutChannel[i] == 0);
        if (error != 0) break;
      }
    }
  };

  if (error) {
    iisaacfenc_PsyOutDelete(hPsyOut);
    hPsyOut = 0;
  }

  return error;
}

void iisaacfenc_PsyOutInit(const int nElements,
                           const int nChannels,
                           SSE_OPTI const int useCpuOptimization,
                           const PSY_INTERNAL* const hPsy,
                           const PSY_OUT* hPsyOut) {
  int el, ch;

  int commonWindow = 1;
  int windowSequence = LONG_WINDOW;
  int windowShape = KBD_WINDOW;
  int noOfGroups = 1;

  for (el = 0; el < nElements; el++) {
    iisaacfenc_InitPsyOutElement(commonWindow,
                                 MS_NONE,
                                 hPsyOut->psyOutElement[el]);
  }

  for (ch = 0; ch < nChannels; ch++) {
    windowShape = SINE_WINDOW;

    iisaacfenc_InitPsyOutChannel(windowSequence,
                                 windowShape,
                                 noOfGroups,
                                 useCpuOptimization,
                                 hPsy->psyConf[0].sfbCnt,
                                 hPsy->psyConf[0].sfbOffset,
                                 hPsy->psyConf[0].granuleLength,
                                 hPsyOut->psyOutChannel[ch]);
  }
}

int iisaacfenc_PsyOutDelete(PSY_OUT* hPsyOut) {
  int error = 0;
  int i;

  if (hPsyOut) {
    for (i = 0; i < SIGMAP_MAX_SIGNALS; i++) {
      if (hPsyOut->psyOutChannel[i]) {
        iisaacfenc_QuantizeDelete(&hPsyOut->psyOutChannel[i]->quantizerData);
        iisaacfenc_sfEstimDelete(&hPsyOut->psyOutChannel[i]->sfestimData);
        iisFree(hPsyOut->psyOutChannel[i]);
      }
    }

    for (i = 0; i < SIGMAP_MAX_ELEMENTS; i++) {
      if (hPsyOut->psyOutElement[i]) {
        iisFree(hPsyOut->psyOutElement[i]);
      }
    }

    iisFree(hPsyOut);
  };
  hPsyOut = 0;
  return error;
}

int iisaacfenc_psyMainInit(AACENC_CODEC_TYPE codecType,
                           PSY_INTERNAL* hPsy,
                           int useCpuOptimization,
                           int sampleRate,
                           int bitRate,
                           int tnsMask,
                           float bandwidth,
                           int useIS,
                           CHANNEL_MAPPING_HANDLE hChMap) {
  int ch, err = 0;
  int numberChannels = hChMap->nChannels;
  int numberEffectiveChannels = hChMap->nEffectiveChannels;
  int monoStereoMode = hChMap->monoStereoMode;
  int tnsChannels = 0;

  switch (monoStereoMode) {
    case 0:
      tnsChannels = 1;
      break;
    case 1:
      tnsChannels = 2;
      break;
    default:
      tnsChannels = 0;
      break;
  }

  if ((numberChannels <= 0) || (numberEffectiveChannels <= 0)) {
    err = 1;
  }

  if (!err) {
    err = iisaacfenc_InitPsyConfiguration(bitRate / numberEffectiveChannels,
                                          sampleRate,
                                          bandwidth,
                                          codecType,
                                          LONG_WINDOW, hPsy->granuleLength,
                                          useIS, &(hPsy->psyConf[0]));
  }

  if (!err) {
    err = iisaacfenc_InitTnsConfiguration(codecType, (bitRate * tnsChannels) / numberEffectiveChannels, sampleRate, tnsChannels, LONG_WINDOW,
                                          &(hPsy->psyConf[0].tnsConf), &hPsy->psyConf[0], tnsMask);
  }

  if (!err) {
    err = iisaacfenc_InitPsyConfiguration(bitRate / numberEffectiveChannels,
                                          sampleRate,
                                          bandwidth,
                                          codecType,
                                          SHORT_WINDOW, hPsy->granuleLength,
                                          useIS, &hPsy->psyConf[1]);
  }

  if (!err) {
    err = iisaacfenc_InitTnsConfiguration(codecType,
                                          (bitRate * tnsChannels) / numberEffectiveChannels,
                                          sampleRate,
                                          tnsChannels,
                                          SHORT_WINDOW,
                                          &(hPsy->psyConf[1].tnsConf),
                                          &hPsy->psyConf[1],
                                          tnsMask);
  }

  if (!err) {
    for (ch = 0; ch < numberChannels; ch++) {
      hPsy->psyData[ch]->isLFE = 0;
      iisaacfenc_InitBlockSwitching(&hPsy->psyData[ch]->blockSwitchingControl,
                                    codecType,
                                    sampleRate,
                                    hPsy->psyConf[0].bitsPerLine,
                                    useCpuOptimization);

      iisaacfenc_InitPreEchoControl(hPsy->psyData[ch]->sfbThresholdnm1,
                                    hPsy->psyConf[0].sfbCnt,
                                    hPsy->psyConf[0].sfbPcmQuantThreshold);
    }
  }

  for (ch = 0; ch < numberChannels; ch++) {
    hPsy->psyData[ch]->chBitRate = bitRate / numberEffectiveChannels;
  }

  return (err);
}

int iisaacfenc_psyFeedInputBufferFloat(const float* const input,
                                       const int nSamplesTot,
                                       PSY_INTERNAL* pK,
                                       const CHANNEL_MAPPING* const cm) {
  float tmp_pnt[2][FRAME_LEN_LONG];
  int i, j, k;
  int error = 0;

  for (i = 0; i < cm->nElements; i++) {
    for (j = 0; j < cm->elInfo[i].nChannelsInEl; j++) {
      const int ci = cm->elInfo[i].ChannelIndex[j];
      for (k = 0; k < nSamplesTot / cm->nChannels; k++) {
        tmp_pnt[j][k] = input[k * cm->nChannels + ci];
      }
    }

    if (!error) {
      HANDLE_ERROR_INFO tmpError = noError;
      for (j = 0; j < cm->elInfo[i].nChannelsInEl; j++) {
        error = (noError != (tmpError = MP4TIMEBUF_FeedBufferMono(pK->psyData[cm->elInfo[i].ChannelIndex[j]]->psyInputBuffer,
                                                                  tmp_pnt[j],
                                                                  nSamplesTot / cm->nChannels)));
        if (tmpError != noError) {
          freeErrorTraceback(tmpError);
        }
      }
    }
  }
  return (error != 0);
}

int iisaacfenc_psyMain(AACENC_CODEC_TYPE codecType,
                       const int channels,
                       PSY_DATA* psyData[2],
                       TNS_DATA* tnsData[2],
                       PSY_CONFIGURATION* psyConf,
                       PSY_OUT_CHANNEL* psyOutChannel[2],
                       PSY_OUT_ELEMENT* psyOutElement,
                       float* pScratch,
                       HANDLE_LAPPED_TRANSFORM hLappedTransform,
                       const int preSapFrame,
                       const int useACEPrev[2],
                       const int useACE[2],
                       const int useACENext[2],
                       const int bUsacIndependenceFlag,
                       const HANDLE_UNISTE hUniSte) {
  int commonWindow = 1;
  int groupedSfbCnt[2];
  int groupedSfbActive[2];
  int maxSfbPerGroup[2] = {0};
  int groupedSfbOffset[2][MAX_GROUPED_SFB + 1];
  float groupedSfbMinSnr[2][MAX_GROUPED_SFB];
  int* ptrSfbOffset[2];
  float minUnGroupedSfbNrg[MAX_GROUPED_SFB];
  float maxSfbNrgPerGroup[MAX_NO_OF_GROUPS];
  float grpdTnsPredictionGain[MAX_NO_OF_GROUPS];
  float grpdTnsGainHeadroomRatio[MAX_NO_OF_GROUPS];
  int grpdTnsActive[MAX_NO_OF_GROUPS];
  MDCT_SPECTRUM lastMdctSpectrum[2];
  MDCT_SPECTRUM currMdstEstimate;
  const int useStereoLpd = 0;

  int ch;
  int sfb;
  int lastWinSequence[2];
  int numZeroCrosses[2] = {0, 0};
  float dcOffset;
  float autoCorrAt1[2] = {0.0f, 0.0f};
  int noShortBlocks = 0;

  psyOutElement->frameEnergyF = 0.0f;
  psyOutElement->toolsInfo.bPrevFrame = 1;
  commonWindow = (!useACE[0] && !useACE[1]);

  for (ch = 0; ch < channels; ch++) {
    int nDelay = psyConf[0].granuleLength + 3 * (psyConf[0].granuleLength / TRANS_FAC) + (psyConf[0].granuleLength / (2 * TRANS_FAC)) + (psyConf[0].granuleLength / TRANS_FAC);

    lastWinSequence[ch] = psyData[ch]->blockSwitchingControl.windowSequence;

    psyData[ch]->blockSwitchingControl.timeSignal = MP4TIMEBUF_AccessBuffer(psyData[ch]->psyInputBuffer, nDelay, 0);

    dcOffset = sumFLOAT(psyData[ch]->blockSwitchingControl.timeSignal, psyConf[0].granuleLength);
    if ((float)fabs(dcOffset) > psyConf[0].granuleLength * FLT_MIN) {
      dcOffset /= (float)psyConf[0].granuleLength;
    } else {
      dcOffset = 0.0f;
    }

    pScratch[0] = psyData[ch]->blockSwitchingControl.timeSignal[0] - dcOffset;
    autoCorrAt1[ch] = 0.0f;
    for (sfb = 1; sfb < psyConf[0].granuleLength; sfb++) {
      float autoCorrValueAt1;
      pScratch[sfb] = psyData[ch]->blockSwitchingControl.timeSignal[sfb] - dcOffset;
      autoCorrValueAt1 = pScratch[sfb] * pScratch[sfb - 1];
      if (autoCorrValueAt1 < 0.0f) {
        numZeroCrosses[ch]++;
      }
      autoCorrAt1[ch] += autoCorrValueAt1;
    }
  }

  for (ch = 0; ch < channels; ch++) {
    float useLOvBlSwFrmNrg = 0.f;

    if (psyOutElement->frameEnergyF < (float)numZeroCrosses[ch]) {
      psyOutElement->frameEnergyF = (float)numZeroCrosses[ch];
    }
    if (psyOutElement->frameEnergyF < 65.0f && psyData[ch]->chBitRate > 17420 && psyData[ch]->chBitRate != 34071) {
      psyOutElement->frameEnergyF = 65.0f;
    }
    if ((psyData[ch]->chBitRate < 16384) || (psyData[ch]->chBitRate < 29696 && psyConf[0].lowpassLine <= 896)) {
      numZeroCrosses[ch] = (5 * numZeroCrosses[ch]) >> 2;
    }

    iisaacfenc_BlockSwitching(&psyData[ch]->blockSwitchingControl,
                              psyConf[0].granuleLength,
                              psyData[ch]->isLFE,
                              codecType,
                              preSapFrame,
                              useLOvBlSwFrmNrg,
                              useACEPrev[ch],
                              useACE[ch],
                              useACENext[ch],
                              noShortBlocks);
  }

  if (channels == 2) {
    if (!psyData[0]->isLFE && !psyData[1]->isLFE) {
      iisaacfenc_SyncBlockSwitching(&psyData[0]->blockSwitchingControl,
                                    &psyData[1]->blockSwitchingControl,
                                    codecType,
                                    useStereoLpd,
                                    &commonWindow);
    }
  }

  for (ch = 0; ch < channels; ch++) {
    int bForceKBDWindow = (psyOutChannel[ch]->wasVeryTonal || psyOutChannel[ch]->isVeryTonal);
    iisaacfenc_WindowShaping(&psyData[ch]->blockSwitchingControl,
                             codecType,
                             bForceKBDWindow,
                             preSapFrame || psyData[ch]->isLFE);
  }

  if (channels == 2) {
    if (!psyData[0]->isLFE && !psyData[1]->isLFE) {
      iisaacfenc_SyncWindowShaping(&psyData[0]->blockSwitchingControl,
                                   &psyData[1]->blockSwitchingControl,
                                   commonWindow);
    }
  }

  if (!useACE[0] || !useACE[1]) {
    switch (codecType) {
      case AACENC_CODEC_AAC:
        psyOutElement->toolsInfo.bIntensityStereo = 1;
        psyOutElement->toolsInfo.bCplxPredMdct = 0;
        psyOutElement->toolsInfo.bCplxPredMdctActive = 0;
        break;
      case AACENC_CODEC_XHEAAC:
      case AACENC_CODEC_MPEGH:
        psyOutElement->toolsInfo.bIntensityStereo = 0;
        psyOutElement->toolsInfo.bCplxPredMdct = 1;
        psyOutElement->toolsInfo.bCplxPredMdctActive = 0;
        break;
    }

    psyOutElement->toolsInfo.bUniSte = 0;
    if (hUniSte != NULL) {
      psyOutElement->toolsInfo.bCplxPredMdct = 0;
      psyOutElement->toolsInfo.bCplxPredMdctActive = 0;
      psyOutElement->toolsInfo.bUniSte = 1;
    }

    if ((channels == 2) && (psyOutElement->toolsInfo.bCplxPredMdct)) {
      iisaacfenc_PrepareCplxPredFlags(commonWindow, bUsacIndependenceFlag,
                                      lastWinSequence[0], lastWinSequence[1],
                                      psyData[0]->blockSwitchingControl.windowSequence, psyData[1]->blockSwitchingControl.windowSequence,
                                      &psyOutElement->toolsInfo.bPrevFrame, &psyOutElement->toolsInfo.bCplxPredMdctResetPredictors);
    }

    for (ch = 0; ch < channels; ch++) {
      if (useACE[ch])
        continue;

      psyOutElement->blockType[ch] = psyData[ch]->blockSwitchingControl.windowSequence;

      if (psyData[ch]->blockSwitchingControl.windowSequence != SHORT_WINDOW) {
        int residualLowpassLine = -1;

        if (hUniSte != NULL && ch == 1) {
          residualLowpassLine = hUniSte->nResidualLinesMdctLong;
        }

        iisaacfenc_advancePsychLong(psyData[ch],
                                    tnsData[ch],
                                    psyConf,
                                    psyOutChannel[ch],
                                    pScratch,
                                    &lastMdctSpectrum[ch],
                                    tnsData[1 - ch],
                                    psyOutChannel[1 - ch],
                                    ch,
                                    hLappedTransform,
                                    useACEPrev[ch],
                                    useACENext[ch],
                                    lastWinSequence[ch],
                                    commonWindow,
                                    residualLowpassLine);

        for (sfb = psyConf[0].sfbActive - 1; sfb >= 0; sfb--) {
          if (psyData[ch]->sfbEnergy.Long[sfb] > 0.0f) {
            break;
          }
        }
        maxSfbPerGroup[ch] = sfb + 1;

        if ((ch == 1) && (commonWindow)) {
          iisaacfenc_advancePsychLongMS(psyData, psyConf,
                                        &psyOutElement->toolsInfo);
          if (psyOutElement->toolsInfo.bCplxPredMdct) {
            iisaacfenc_PrepareCplxPredLong(
                psyData,
                psyConf,
                &currMdstEstimate,
                &psyOutElement->toolsInfo);
          }
        }
      } else {
        int residualLowpassLine = -1;

        if (hUniSte != NULL && ch == 1) {
          residualLowpassLine = hUniSte->nResidualLinesMdctShort;
        }

        iisaacfenc_advancePsychShort(psyData[ch],
                                     tnsData[ch],
                                     psyConf,
                                     psyOutChannel[ch],
                                     pScratch,
                                     &lastMdctSpectrum[ch],
                                     tnsData[1 - ch],
                                     psyOutChannel[1 - ch],
                                     ch,
                                     hLappedTransform,
                                     useACEPrev[ch],
                                     useACENext[ch],
                                     lastWinSequence[ch],
                                     commonWindow,
                                     residualLowpassLine);

        if ((ch == 1) && (commonWindow)) {
          iisaacfenc_advancePsychShortMS(psyData, psyConf,
                                         &psyOutElement->toolsInfo);
          if (psyOutElement->toolsInfo.bCplxPredMdct) {
            iisaacfenc_PrepareCplxPredShort(psyData, psyConf, &currMdstEstimate, pScratch, &psyOutElement->toolsInfo);
          }
        }
      }

      if (codecType == AACENC_CODEC_XHEAAC) {
        psyOutChannel[ch]->noiseFillingMode = NF_MODE_2;
        psyOutChannel[ch]->tnsInfo.bTnsOnLr = 1;
        if (ch == 1) {
          if (commonWindow) {
            psyOutChannel[0]->tnsInfo.bCommonTns = iisaacfenc_equalTnsFilters(tnsData, &psyOutChannel[0]->tnsInfo, &psyOutChannel[1]->tnsInfo,
                                                                              psyData[1]->blockSwitchingControl.windowSequence == SHORT_WINDOW);
            psyOutChannel[1]->tnsInfo.bCommonTns = psyOutChannel[0]->tnsInfo.bCommonTns;
          } else {
            psyOutChannel[1]->tnsInfo.bCommonTns = psyOutChannel[0]->tnsInfo.bCommonTns = 0;
          }
        }
      }
    }

    for (ch = 0; ch < channels; ch++) {
      int win = (psyData[ch]->blockSwitchingControl.windowSequence == SHORT_WINDOW ? 1 : 0);

      if (useACE[ch])
        continue;

      if (psyData[ch]->blockSwitchingControl.windowSequence == SHORT_WINDOW) {
        MDCT_SPECTRUM* mdctSpectrum = &psyData[ch]->mdctSpectrum;
        MDCT_SPECTRUM* mdctDownmix = (channels != 2 || !commonWindow) ? NULL : ((ch == 0) ? &psyOutElement->toolsInfo.mdctSpectrumDmxPrev : &psyOutElement->toolsInfo.mdctSpectrumDmx);

        iisaacfenc_groupShortData(mdctSpectrum,
                                  mdctDownmix,
                                  pScratch,
                                  &psyData[ch]->sfbThreshold,
                                  &psyData[ch]->sfbEnergy,
                                  &psyData[ch]->sfbEnergyMS,
                                  minUnGroupedSfbNrg,
                                  maxSfbNrgPerGroup,
                                  tnsData[ch]->Short.subBlockInfo,
                                  grpdTnsPredictionGain,
                                  grpdTnsGainHeadroomRatio,
                                  grpdTnsActive,
                                  psyConf[1].sfbCnt,
                                  psyConf[1].sfbActive,
                                  psyConf[1].sfbOffset,
                                  psyConf[1].sfbMinSnr,
                                  &groupedSfbCnt[ch],
                                  &groupedSfbActive[ch],
                                  groupedSfbOffset[ch],
                                  &maxSfbPerGroup[ch],
                                  groupedSfbMinSnr[ch],
                                  psyData[ch]->blockSwitchingControl.noOfGroups,
                                  psyData[ch]->blockSwitchingControl.groupLen,
                                  psyConf[1].granuleLength);

        ptrSfbOffset[ch] = groupedSfbOffset[ch];
      } else {
        groupedSfbCnt[ch] = psyConf[0].sfbCnt;
        groupedSfbActive[ch] = maxSfbPerGroup[ch];
        ptrSfbOffset[ch] = psyConf[win].sfbOffset;
        grpdTnsPredictionGain[0] = tnsData[ch]->Long.subBlockInfo.predictionGainMax;
        grpdTnsGainHeadroomRatio[0] = tnsData[ch]->Long.subBlockInfo.tnsGainHeadroomRatio;
        grpdTnsActive[0] = tnsData[ch]->Long.subBlockInfo.tnsActive;
      }

      {
        int grp;

        if (ch == 0) {
          psyOutElement->toolsInfo.tnsGainHeadroomRatioPrev = 1.f;
        }
        for (grp = 0; grp < psyData[ch]->blockSwitchingControl.noOfGroups; grp++) {
          if (grpdTnsGainHeadroomRatio[grp] > psyOutElement->toolsInfo.tnsGainHeadroomRatioPrev) {
            psyOutElement->toolsInfo.tnsGainHeadroomRatioPrev = grpdTnsGainHeadroomRatio[grp];
          }
        }
      }

      setINT(NO_NOISE_PNS, psyOutChannel[ch]->noiseNrg, groupedSfbCnt[ch]);
    }

    if (channels == 2) {
      psyOutElement->toolsInfo.msDigest = MS_NONE;
      psyOutElement->commonWindow = commonWindow;

      psyStereo(codecType,
                psyOutElement,
                psyData,
                tnsData,
                psyConf,
                groupedSfbCnt,
                maxSfbPerGroup,
                psyOutChannel,
                ptrSfbOffset,
                hUniSte,
                &currMdstEstimate,
                lastMdctSpectrum

      );
    }

    for (ch = 0; ch < channels; ch++) {
      if (useACE[ch])
        continue;
    }

    for (ch = 0; ch < channels; ch++) {
      if (useACE[ch])
        continue;

      if (psyData[ch]->blockSwitchingControl.windowSequence != SHORT_WINDOW) {
        iisaacfenc_BuildInterface(&psyData[ch]->mdctSpectrum,
                                  &psyData[ch]->sfbThreshold,
                                  &psyData[ch]->sfbEnergy,
                                  &psyData[ch]->sfbEnergySumMS,
                                  psyData[ch]->blockSwitchingControl.windowSequence,
                                  psyData[ch]->blockSwitchingControl.windowShape,
                                  psyData[ch]->blockSwitchingControl.prevWindowShape,
                                  psyConf[0].sfbCnt,
                                  (psyData[ch]->isLFE) ? psyConf[0].sfbActiveLFE : psyConf[0].sfbActive,
                                  psyConf[0].sfbOffset,
                                  maxSfbPerGroup[ch],
                                  psyConf[0].sfbMinSnr,
                                  psyData[ch]->blockSwitchingControl.noOfGroups,
                                  psyData[ch]->blockSwitchingControl.groupLen,
                                  psyConf[0].granuleLength,
                                  psyOutChannel[ch]);
      } else {
        iisaacfenc_BuildInterface(&psyData[ch]->mdctSpectrum,
                                  &psyData[ch]->sfbThreshold,
                                  &psyData[ch]->sfbEnergy,
                                  &psyData[ch]->sfbEnergySumMS,
                                  psyData[ch]->blockSwitchingControl.windowSequence,
                                  psyData[ch]->blockSwitchingControl.windowShape,
                                  psyData[ch]->blockSwitchingControl.prevWindowShape,
                                  groupedSfbCnt[ch],
                                  groupedSfbActive[ch],
                                  groupedSfbOffset[ch],
                                  maxSfbPerGroup[ch],
                                  groupedSfbMinSnr[ch],
                                  psyData[ch]->blockSwitchingControl.noOfGroups,
                                  psyData[ch]->blockSwitchingControl.groupLen,
                                  psyConf[0].granuleLength,
                                  psyOutChannel[ch]);
      }
    }
  }

  return (0);
}

#ifndef _NOT_AVOID_FLOAT_DENORMALS
static const float mySpecMin = 1.e-12f;
static void flushtozeroFLOAT_IP(const float limit, float* const X, const int len) {
  int i;

  for (i = 0; i < len; i++) {
    if (fabs(X[i]) < limit) {
      X[i] = 0.f;
    }
  }
}
#endif

static int iisaacfenc_advancePsychLong(PSY_DATA* psyData,
                                       TNS_DATA* tnsData,
                                       const PSY_CONFIGURATION psyConf[2],
                                       PSY_OUT_CHANNEL* psyOutChannel,
                                       float* pScratch,
                                       MDCT_SPECTRUM* originalMdctSpectrum,
                                       TNS_DATA* tnsData2,
                                       PSY_OUT_CHANNEL* psyOutChannel2,
                                       const int ch,
                                       HANDLE_LAPPED_TRANSFORM hLappedTransform,
                                       int useACEPrev,
                                       int useACENext,
                                       const int prevBlockType,
                                       const int commonWindow,
                                       int extLowpassLine

) {
  int i = 0;
  float* pTimeSignal = NULL;
  const float mdctScalingFac = 2.0f;
  float elw = 0.0f;
  int bTnsFilterShortened = 0;

  int sfbActive = psyConf[0].sfbActive;
  int lowpassLine = psyConf[0].lowpassLine;

  pTimeSignal = MP4TIMEBUF_AccessBuffer(psyData->psyInputBuffer, TRANSFORM_OFFSET_LONG, 0);

  iisaacfenc_ApplyWindow(hLappedTransform,
                         pTimeSignal,
                         pScratch,
                         psyData->blockSwitchingControl.windowShape,
                         psyData->blockSwitchingControl.prevWindowShape,
                         psyData->blockSwitchingControl.windowSequence,
                         psyData->blockSwitchingControl.windowKernelNum,
                         useACENext,
                         useACEPrev);

  iisaacfenc_ApplyLappedTransform(hLappedTransform,
                                  pScratch,
                                  psyData->mdctSpectrum.Long,
                                  psyData->blockSwitchingControl.windowSequence,
                                  psyData->blockSwitchingControl.windowKernelNum);

  smulFLOAT(mdctScalingFac, psyData->mdctSpectrum.Long, psyData->mdctSpectrum.Long,
            psyConf[0].lowpassLine);
#ifndef _NOT_AVOID_FLOAT_DENORMALS
  flushtozeroFLOAT_IP(mySpecMin, psyData->mdctSpectrum.Long, psyConf[0].lowpassLine);
#endif

  if (!psyData->isLFE) {
    if ((extLowpassLine > 0) && (extLowpassLine < psyConf[0].lowpassLine)) {
      lowpassLine = extLowpassLine;
      sfbActive = psyConf[0].sfbActive;
    } else {
      lowpassLine = psyConf[0].lowpassLine;
      sfbActive = psyConf[0].sfbActive;
    }

    setFLOAT(0.0f, psyData->mdctSpectrum.Long + lowpassLine, psyConf[0].granuleLength - lowpassLine);

    for (i = 0; i < lowpassLine; i++) {
      originalMdctSpectrum->Long[i] = psyData->mdctSpectrum.Long[i];
    }
    for (; i < psyConf[0].granuleLength; i++) {
      originalMdctSpectrum->Long[i] = 0.0f;
    }

    iisaacfenc_CalcBandEnergy(psyData->mdctSpectrum.Long,
                              psyConf[0].sfbOffset,
                              sfbActive,
                              psyData->sfbEnergy.Long,
                              &psyData->sfbEnergySum.Long);

    setFLOAT(0.0f, psyData->sfbEnergy.Long + sfbActive,
             psyConf[0].sfbCnt - sfbActive);

    smulFLOAT(psyConf[0].ratio, psyData->sfbEnergy.Long,
              psyData->sfbThreshold.Long, sfbActive);

    elw = 0.5f;
    for (i = psyConf[0].sfbStartELW; i < sfbActive; i++) {
      psyData->sfbThreshold.Long[i] *= elw;
      elw *= 0.5f;
    }

    iisaacfenc_SpreadingMax(sfbActive, psyConf[0].sfbMaskLowFactor,
                            psyConf[0].sfbMaskHighFactor, psyData->sfbThreshold.Long);

    elw = 2.0f;
    for (i = psyConf[0].sfbStartELW; i < sfbActive; i++) {
      psyData->sfbThreshold.Long[i] *= elw;
      elw *= 2.0f;
    }

    elw = psyData->sfbEnergy.Long[0];
    for (i = 1; i < sfbActive; i++) {
      const float sfbEn = psyData->sfbEnergy.Long[i] + psyData->sfbEnergy.Long[i - 1];
      if (elw < sfbEn) {
        elw = sfbEn;
      }
    }
    if (elw > FLT_MIN) {
      elw = psyConf[0].clipEnergy * psyData->sfbEnergySum.Long / elw;
    } else
      elw = psyConf[0].clipEnergy;

    for (i = 0; i < sfbActive; i++) {
      if (psyData->sfbThreshold.Long[i] > elw) {
        psyData->sfbThreshold.Long[i] = elw;
      } else if (psyData->sfbThreshold.Long[i] < psyConf[0].sfbPcmQuantThreshold[i]) {
        psyData->sfbThreshold.Long[i] = psyConf[0].sfbPcmQuantThreshold[i];
      }
    }

    iisaacfenc_TnsDetect(tnsData,
                         psyConf[0].tnsConf,
                         &psyOutChannel->tnsInfo,
                         psyConf[0].sfbCnt,
                         psyConf[0].sfbOffset,
                         psyData->mdctSpectrum.Long,
                         0,
                         0,
                         (int)psyData->blockSwitchingControl.windowSequence);

    bTnsFilterShortened = iisaacfenc_isTnsFilterShortened(&psyOutChannel->tnsInfo);

    if ((ch == 1) && (commonWindow) && bTnsFilterShortened == 0) {
      iisaacfenc_TnsSync(tnsData,
                         tnsData2,
                         &psyOutChannel->tnsInfo,
                         &psyOutChannel2->tnsInfo,
                         psyConf[0].tnsConf,
                         0,
                         (int)psyData->blockSwitchingControl.windowSequence);
    }

    iisaacfenc_TnsEncode(&psyOutChannel->tnsInfo,
                         tnsData,
                         psyConf[0].sfbCnt,
                         psyConf[0].tnsConf,
                         psyData->mdctSpectrum.Long,
                         psyConf[0].sfbOffset,
                         0,
                         (int)psyData->blockSwitchingControl.windowSequence);

    if (tnsData->Long.subBlockInfo.tnsActive) {
      setFLOAT(0.0f, psyData->mdctSpectrum.Long + lowpassLine,
               psyConf[0].sfbOffset[sfbActive] - lowpassLine);

      psyData->sfbEnergySum.Long = 0.0f;
      for (i = 0; i < sfbActive; i++) {
        float sfbEnergy;
        const int len = psyConf[0].sfbOffset[i + 1] - psyConf[0].sfbOffset[i];
        const float* pMdctSpectrum = psyData->mdctSpectrum.Long + psyConf[0].sfbOffset[i];
        sfbEnergy = dotFLOAT(pMdctSpectrum, pMdctSpectrum, len);

        if ((sfbEnergy > FLT_MIN) && (sfbEnergy < psyData->sfbEnergy.Long[i])) {
          psyData->sfbThreshold.Long[i] *= (sfbEnergy + FLT_MIN) / (psyData->sfbEnergy.Long[i] + FLT_MIN);
          if (psyData->sfbThreshold.Long[i] < psyConf[0].sfbPcmQuantThreshold[i]) {
            psyData->sfbThreshold.Long[i] = psyConf[0].sfbPcmQuantThreshold[i];
          }
        }
        psyData->sfbEnergy.Long[i] = sfbEnergy;
        psyData->sfbEnergySum.Long += sfbEnergy;
      }
    }
  } else {
    setFLOAT(0.0f, psyData->mdctSpectrum.Long + psyConf[0].lowpassLFE,
             psyConf[0].granuleLength - psyConf[0].lowpassLFE);

    iisaacfenc_CalcBandEnergy(psyData->mdctSpectrum.Long,
                              psyConf[0].sfbOffset,
                              psyConf[0].sfbActiveLFE,
                              psyData->sfbEnergy.Long,
                              &psyData->sfbEnergySum.Long);

    setFLOAT(0.0f, psyData->sfbEnergy.Long + psyConf[0].sfbActiveLFE,
             psyConf[0].sfbCnt - psyConf[0].sfbActiveLFE);

    smulFLOAT(psyConf[0].ratio, psyData->sfbEnergy.Long,
              psyData->sfbThreshold.Long, psyConf[0].sfbActiveLFE);

    iisaacfenc_SpreadingMax(psyConf[0].sfbActiveLFE, psyConf[0].sfbMaskLowFactor,
                            psyConf[0].sfbMaskHighFactor, psyData->sfbThreshold.Long);

    for (i = 0; i < psyConf[0].sfbActiveLFE; i++) {
      if (psyData->sfbThreshold.Long[i] > psyConf[0].clipEnergy) {
        psyData->sfbThreshold.Long[i] = psyConf[0].clipEnergy;
      } else if (psyData->sfbThreshold.Long[i] < psyConf[0].sfbPcmQuantThreshold[i]) {
        psyData->sfbThreshold.Long[i] = psyConf[0].sfbPcmQuantThreshold[i];
      }
    }

    tnsData->Long.subBlockInfo.tnsActive = 0;
    psyOutChannel->tnsInfo.numOfFilters[0] = 0;

    setFLOAT(FLT_MAX, psyData->sfbThreshold.Long + psyConf[0].sfbActiveLFE,
             psyConf[0].sfbCnt - psyConf[0].sfbActiveLFE);
  }

  if (!psyData->isLFE) {
    if (prevBlockType == SHORT_WINDOW) {
      setFLOAT(NEARLY_FLT_MAX, psyData->sfbThresholdnm1, psyConf[0].sfbActive);
    }

    iisaacfenc_PreEchoControl(psyData->sfbThresholdnm1,
                              psyData->sfbThreshold.Long,
                              psyConf[0].sfbActive,
                              psyConf[0].maxAllowedIncreaseFactor,
                              0.0f,
                              psyConf[0].minRemainingThresholdFactor);

    setFLOAT(FLT_MAX, psyData->sfbThreshold.Long + psyConf[0].sfbActive,
             psyConf[0].sfbCnt - psyConf[0].sfbActive);
  }

  return 0;
}

static int iisaacfenc_advancePsychLongMS(PSY_DATA* psyData[2],
                                         const PSY_CONFIGURATION psyConf[2],
                                         struct STEREO_INFO* toolsInfo) {
  int bSwapPrev = toolsInfo->bSwap;
  iisaacfenc_CalcBandEnergyMS(psyData[0]->mdctSpectrum.Long,
                              psyData[1]->mdctSpectrum.Long,
                              psyConf[0].sfbOffset,
                              psyConf[0].sfbActive,
                              psyData[0]->sfbEnergyMS.Long,
                              &psyData[0]->sfbEnergySumMS.Long,
                              psyData[1]->sfbEnergyMS.Long,
                              &psyData[1]->sfbEnergySumMS.Long);

  toolsInfo->bSwap = (psyData[0]->sfbEnergySumMS.Long < (0.2f * psyData[1]->sfbEnergySumMS.Long)) ? 1 : 0;
  if (toolsInfo->bSwap != bSwapPrev) {
    toolsInfo->bPrevFrame = 0;
  }

  return 0;
}

static int iisaacfenc_advancePsychShort(PSY_DATA* psyData,
                                        TNS_DATA* tnsData,
                                        const PSY_CONFIGURATION psyConf[2],
                                        PSY_OUT_CHANNEL* psyOutChannel,
                                        float* pScratch,
                                        MDCT_SPECTRUM* originalMdctSpectrum,
                                        TNS_DATA* tnsData2,
                                        PSY_OUT_CHANNEL* psyOutChannel2,
                                        const int ch,
                                        HANDLE_LAPPED_TRANSFORM hLappedTransform,
                                        int useACEPrev,
                                        int useACENext,
                                        const int prevBlockType,
                                        const int commonWindow,
                                        int extLowpassLine) {
  int i, w;
  float* pTimeSignal = NULL;
  const float mdctScalingFac = 2.0f;
  float elw;
  const int shortGranuleLength = psyConf[1].granuleLength / TRANS_FAC;
  int bTnsFilterShortened = 0;
  int sfbActive = psyConf[1].sfbActive;
  int lowpassLine = psyConf[1].lowpassLine;

  if (ch == 1) {
    psyOutChannel->tnsInfo.bCommonTns = 1;
  }

  for (w = 0; w < TRANS_FAC; w++) {
    int prevWindowShape;
    int windowKernelNum;
    int useLpdModePrev;
    int useLpdModeNext;

    i = ((TRANS_FAC - 1) * shortGranuleLength) >> 1;
    pTimeSignal = MP4TIMEBUF_AccessBuffer(psyData->psyInputBuffer, i + w * shortGranuleLength, 0);

    prevWindowShape = (w == 0) ? psyData->blockSwitchingControl.prevWindowShape : psyData->blockSwitchingControl.windowShape;
    windowKernelNum = (w == 0) ? psyData->blockSwitchingControl.windowKernelNum : ((psyData->blockSwitchingControl.windowKernelNum & 1) ? MDST_IV : MDCT_IV);
    useLpdModePrev = useACEPrev;
    useLpdModeNext = (w != TRANS_FAC - 1) ? 0 : useACENext;

    iisaacfenc_ApplyWindow(hLappedTransform,
                           pTimeSignal,
                           pScratch + psyConf[0].granuleLength,
                           psyData->blockSwitchingControl.windowShape,
                           prevWindowShape,
                           SHORT_WINDOW,
                           windowKernelNum,
                           useLpdModeNext,
                           useLpdModePrev);

    iisaacfenc_ApplyLappedTransform(hLappedTransform,
                                    pScratch + psyConf[0].granuleLength,
                                    psyData->mdctSpectrum.Short[w],
                                    SHORT_WINDOW,
                                    windowKernelNum);

    smulFLOAT(mdctScalingFac, psyData->mdctSpectrum.Short[w], psyData->mdctSpectrum.Short[w], psyConf[1].lowpassLine);
#ifndef _NOT_AVOID_FLOAT_DENORMALS
    flushtozeroFLOAT_IP(mySpecMin, psyData->mdctSpectrum.Short[w], psyConf[1].lowpassLine);
#endif

    if ((extLowpassLine > 0) && (extLowpassLine < psyConf[1].lowpassLine)) {
      lowpassLine = extLowpassLine;
    }
    setFLOAT(0.0f, psyData->mdctSpectrum.Short[w] + lowpassLine, shortGranuleLength - lowpassLine);

    for (i = 0; i < lowpassLine; i++) {
      originalMdctSpectrum->Short[w][i] = psyData->mdctSpectrum.Short[w][i];
    }
    for (; i < shortGranuleLength; i++) {
      pScratch[i + w * shortGranuleLength] = 0.0f;
    }

    iisaacfenc_CalcBandEnergy(psyData->mdctSpectrum.Short[w],
                              psyConf[1].sfbOffset,
                              sfbActive,
                              psyData->sfbEnergy.Short[w],
                              &psyData->sfbEnergySum.Short[w]);

    setFLOAT(0.0f, psyData->sfbEnergy.Short[w] + sfbActive,
             psyConf[1].sfbCnt - sfbActive);

    smulFLOAT(psyConf[1].ratio, psyData->sfbEnergy.Short[w],
              psyData->sfbThreshold.Short[w], sfbActive);

    elw = 0.125f;
    for (i = psyConf[1].sfbStartELW; i < sfbActive; i++) {
      psyData->sfbThreshold.Short[w][i] *= elw;
      elw *= 0.125f;
    }

    iisaacfenc_SpreadingMax(sfbActive, psyConf[1].sfbMaskLowFactor,
                            psyConf[1].sfbMaskHighFactor, psyData->sfbThreshold.Short[w]);

    elw = 8.0f;
    for (i = psyConf[1].sfbStartELW; i < sfbActive; i++) {
      psyData->sfbThreshold.Short[w][i] *= elw;
      elw *= 8.0f;
    }

    for (i = 0; i < sfbActive; i++) {
      if (psyData->sfbThreshold.Short[w][i] > psyConf[1].clipEnergy) {
        psyData->sfbThreshold.Short[w][i] = psyConf[1].clipEnergy;
      } else if (psyData->sfbThreshold.Short[w][i] < psyConf[1].sfbPcmQuantThreshold[i]) {
        psyData->sfbThreshold.Short[w][i] = psyConf[1].sfbPcmQuantThreshold[i];
      }
    }

    iisaacfenc_TnsDetect(tnsData,
                         psyConf[1].tnsConf,
                         &psyOutChannel->tnsInfo,
                         psyConf[1].sfbCnt,
                         psyConf[1].sfbOffset,
                         psyData->mdctSpectrum.Short[w],
                         w, 0,
                         (int)psyData->blockSwitchingControl.windowSequence);
  }

  if (prevBlockType != SHORT_WINDOW) {
    setFLOAT(NEARLY_FLT_MAX, psyData->sfbThresholdnm1, sfbActive);
  }

  for (w = 0; w < TRANS_FAC; w++) {
    bTnsFilterShortened = iisaacfenc_isTnsFilterShortened(&psyOutChannel->tnsInfo);

    if ((ch == 1) && (commonWindow) && bTnsFilterShortened == 0) {
      iisaacfenc_TnsSync(tnsData,
                         tnsData2,
                         &psyOutChannel->tnsInfo,
                         &psyOutChannel2->tnsInfo,
                         psyConf[1].tnsConf,
                         w,
                         (int)psyData->blockSwitchingControl.windowSequence);
    }

    iisaacfenc_TnsEncode(&psyOutChannel->tnsInfo,
                         tnsData,
                         psyConf[1].sfbCnt,
                         psyConf[1].tnsConf,
                         psyData->mdctSpectrum.Short[w],
                         psyConf[1].sfbOffset,
                         w,
                         (int)psyData->blockSwitchingControl.windowSequence);

    if (tnsData->Short.subBlockInfo[w].tnsActive) {
      setFLOAT(0.0f, psyData->mdctSpectrum.Short[w] + lowpassLine,
               psyConf[1].sfbOffset[sfbActive] - lowpassLine);

      psyData->sfbEnergySum.Short[w] = 0.0f;
      for (i = 0; i < sfbActive; i++) {
        float sfbEnergy;
        const int len = psyConf[1].sfbOffset[i + 1] - psyConf[1].sfbOffset[i];
        const float* pMdctSpectrum = psyData->mdctSpectrum.Short[w] + psyConf[1].sfbOffset[i];
        sfbEnergy = dotFLOAT(pMdctSpectrum, pMdctSpectrum, len);

        if ((sfbEnergy > FLT_MIN) && (sfbEnergy < psyData->sfbEnergy.Short[w][i])) {
          psyData->sfbThreshold.Short[w][i] *= (sfbEnergy + FLT_MIN) / (psyData->sfbEnergy.Short[w][i] + FLT_MIN);
          if (psyData->sfbThreshold.Short[w][i] < psyConf[1].sfbPcmQuantThreshold[i]) {
            psyData->sfbThreshold.Short[w][i] = psyConf[1].sfbPcmQuantThreshold[i];
          }
        }
        psyData->sfbEnergy.Short[w][i] = sfbEnergy;
        psyData->sfbEnergySum.Short[w] += sfbEnergy;
      }
    }

    iisaacfenc_PreEchoControl(psyData->sfbThresholdnm1,
                              psyData->sfbThreshold.Short[w],
                              sfbActive,
                              psyConf[1].maxAllowedIncreaseFactor,
                              ((prevBlockType != SHORT_WINDOW) && (w == 0)) ? 0.0f : 0.125f,
                              psyConf[1].minRemainingThresholdFactor);

    setFLOAT(NEARLY_FLT_MAX, psyData->sfbThreshold.Short[w] + sfbActive,
             psyConf[1].sfbCnt - sfbActive);
  }

  return 0;
}

static int iisaacfenc_advancePsychShortMS(PSY_DATA* psyData[2],
                                          const PSY_CONFIGURATION psyConf[2],
                                          struct STEREO_INFO* toolsInfo) {
  int w;
  float nrgSumM = 0.0f, nrgSumS = 0.0f;
  const int bSwapPrev = toolsInfo->bSwap;

  for (w = 0; w < TRANS_FAC; w++) {
    iisaacfenc_CalcBandEnergyMS(psyData[0]->mdctSpectrum.Short[w],
                                psyData[1]->mdctSpectrum.Short[w],
                                psyConf[1].sfbOffset,
                                psyConf[1].sfbActive,
                                psyData[0]->sfbEnergyMS.Short[w],
                                &psyData[0]->sfbEnergySumMS.Short[w],
                                psyData[1]->sfbEnergyMS.Short[w],
                                &psyData[1]->sfbEnergySumMS.Short[w]);
    nrgSumM += psyData[0]->sfbEnergySumMS.Short[w];
    nrgSumS += psyData[1]->sfbEnergySumMS.Short[w];
  }

  toolsInfo->bSwap = (nrgSumM < (0.2f * nrgSumS)) ? 1 : 0;
  if (toolsInfo->bSwap != bSwapPrev) {
    toolsInfo->bPrevFrame = 0;
  }
  return 0;
}

static void iisaacfenc_PrepareCplxPredLong(PSY_DATA* psyData[2],
                                           const PSY_CONFIGURATION psyConf[2],
                                           MDCT_SPECTRUM* mdstEstimate,
                                           struct STEREO_INFO* steInfo) {
  int i;

  if (!steInfo->bSwap) {
    for (i = 0; i < psyConf[0].lowpassLine; i++) {
      steInfo->mdctSpectrumDmxPrev.Long[i] = 0.5f * (steInfo->mdctSpectrumLPrev.Long[i] + steInfo->mdctSpectrumRPrev.Long[i]);
      steInfo->mdctSpectrumDmx.Long[i] = 0.5f * (psyData[0]->mdctSpectrum.Long[i] + psyData[1]->mdctSpectrum.Long[i]);
    }
  } else {
    for (i = 0; i < psyConf[0].lowpassLine; i++) {
      steInfo->mdctSpectrumDmxPrev.Long[i] = 0.5f * (steInfo->mdctSpectrumLPrev.Long[i] - steInfo->mdctSpectrumRPrev.Long[i]);
      steInfo->mdctSpectrumDmx.Long[i] = 0.5f * (psyData[0]->mdctSpectrum.Long[i] - psyData[1]->mdctSpectrum.Long[i]);
    }
  }

  if (!steInfo->bPrevFrame) {
    setFLOAT(0.0f, steInfo->mdctSpectrumDmxPrev.Long + psyConf[0].lowpassLine, psyConf[0].granuleLength - psyConf[0].lowpassLine);
  }

  setFLOAT(0.0f, steInfo->mdctSpectrumDmx.Long + psyConf[0].lowpassLine, psyConf[0].granuleLength - psyConf[0].lowpassLine);
  iisaacfenc_Mdct2Mdst(mdstEstimate->Long,
                       steInfo->mdctSpectrumDmx.Long,
                       steInfo->bPrevFrame ? steInfo->mdctSpectrumDmxPrev.Long : NULL,
                       psyConf[0].granuleLength,
                       psyData[0]->blockSwitchingControl.windowSequence,
                       psyData[0]->blockSwitchingControl.windowShape,
                       psyData[0]->blockSwitchingControl.prevWindowShape);

  setFLOAT(0.0f, mdstEstimate->Long + psyConf[0].lowpassLine, psyConf[0].granuleLength - psyConf[0].lowpassLine);
}

static void iisaacfenc_PrepareCplxPredShort(PSY_DATA* psyData[2],
                                            const PSY_CONFIGURATION psyConf[2],
                                            MDCT_SPECTRUM* mdstEstimate,
                                            float* pScratch,
                                            struct STEREO_INFO* steInfo) {
  int w, i;
  const int shortTransLength = psyConf[1].granuleLength / TRANS_FAC;
  const int shortLowpassLine = psyConf[1].lowpassLine;

  for (w = 0; w < TRANS_FAC; w++) {
    if (!steInfo->bSwap) {
      for (i = 0; i < shortLowpassLine; i++) {
        steInfo->mdctSpectrumDmxPrev.Short[w][i] = 0.5f * (steInfo->mdctSpectrumLPrev.Short[w][i] + steInfo->mdctSpectrumRPrev.Short[w][i]);
        steInfo->mdctSpectrumDmx.Short[w][i] = 0.5f * (psyData[0]->mdctSpectrum.Short[w][i] + psyData[1]->mdctSpectrum.Short[w][i]);
      }
    } else {
      for (i = 0; i < shortLowpassLine; i++) {
        steInfo->mdctSpectrumDmxPrev.Short[w][i] = 0.5f * (steInfo->mdctSpectrumLPrev.Short[w][i] - steInfo->mdctSpectrumRPrev.Short[w][i]);
        steInfo->mdctSpectrumDmx.Short[w][i] = 0.5f * (psyData[0]->mdctSpectrum.Short[w][i] - psyData[1]->mdctSpectrum.Short[w][i]);
      }
    }
  }

  setFLOAT(0.0f, steInfo->mdctSpectrumDmxPrev.Short[TRANS_FAC - 1] + shortLowpassLine, shortTransLength - shortLowpassLine);

  for (w = 0; w < TRANS_FAC; w++) {
    setFLOAT(0.0f, steInfo->mdctSpectrumDmx.Short[w] + shortLowpassLine, shortTransLength - shortLowpassLine);
    iisaacfenc_Mdct2Mdst(mdstEstimate->Short[w],
                         steInfo->mdctSpectrumDmx.Short[w],
                         (w > 0) ? steInfo->mdctSpectrumDmx.Short[w - 1]
                                 : (steInfo->bPrevFrame ? steInfo->mdctSpectrumDmxPrev.Short[TRANS_FAC - 1] : NULL),
                         shortTransLength,
                         psyData[0]->blockSwitchingControl.windowSequence,
                         psyData[0]->blockSwitchingControl.windowShape,
                         (w > 0) ? psyData[0]->blockSwitchingControl.windowShape
                                 : psyData[0]->blockSwitchingControl.prevWindowShape);

    setFLOAT(0.0f, mdstEstimate->Short[w] + shortLowpassLine, shortTransLength - shortLowpassLine);
  }

  if (pScratch) {
    int grp, sfb, off, bin;
    w = i = 0;
    for (grp = 0; grp < psyData[0]->blockSwitchingControl.noOfGroups; grp++) {
      for (sfb = 0; sfb < psyConf[1].sfbCnt; sfb++) {
        for (off = 0; off < psyData[0]->blockSwitchingControl.groupLen[grp]; off++) {
          const float* mdst_w = mdstEstimate->Short[w + off];
          for (bin = psyConf[1].sfbOffset[sfb]; bin < psyConf[1].sfbOffset[sfb + 1]; bin++) {
            pScratch[i++] = mdst_w[bin];
          }
        }
      }
      w += psyData[0]->blockSwitchingControl.groupLen[grp];
    }
    copyFLOAT(pScratch, mdstEstimate->Long, psyConf[1].granuleLength);
  }
}

void iisaacfenc_PsySetBlockSwitchingNoStartStop(PSY_DATA* psyData,
                                                int noStartStopSequence) {
  if (psyData != NULL) {
    iisaacfenc_SetBlockSwitchingNoStartStop(
        &psyData->blockSwitchingControl,
        noStartStopSequence);
  }
}

static void psyStereo(
    const AACENC_CODEC_TYPE codecType,
    PSY_OUT_ELEMENT* psyOutElement,
    PSY_DATA* psyData[2],
    TNS_DATA* tnsData[2],
    const PSY_CONFIGURATION* psyConf,
    const int groupedSfbCnt[2],
    int maxSfbPerGroup[2],
    PSY_OUT_CHANNEL* psyOutChannel[2],
    int* ptrSfbOffset[2],
    const HANDLE_UNISTE hUniSte,
    const MDCT_SPECTRUM* currMdstEstimate,
    const MDCT_SPECTRUM* lastMdctSpectrum) {
  if (psyOutElement->commonWindow == 1) {
    const int cfgNo = (psyData[0]->blockSwitchingControl.windowSequence == SHORT_WINDOW) ? 1 : 0;
    int tnsSimilar = 1;

    {
      maxSfbPerGroup[0] = maxSfbPerGroup[1] = max(maxSfbPerGroup[0], maxSfbPerGroup[1]);
    }
    if ((tnsData[0]->Long.subBlockInfo.predictionGainMax >= 16.0f * tnsData[1]->Long.subBlockInfo.predictionGainMax) ||
        (tnsData[1]->Long.subBlockInfo.predictionGainMax >= 16.0f * tnsData[0]->Long.subBlockInfo.predictionGainMax)) {
      tnsSimilar = 0;
    }

    if (psyOutElement->toolsInfo.bCplxPredMdct == 0) {
      if (psyOutElement->toolsInfo.bIntensityStereo == 1) {
      }

    } else {
      assert(codecType == AACENC_CODEC_XHEAAC || codecType == AACENC_CODEC_MPEGH);
      iisaacfenc_CplxPredStereoProcessing(psyData[0]->sfbEnergy.Long,
                                          psyData[1]->sfbEnergy.Long,
                                          psyData[0]->sfbEnergyMS.Long,
                                          psyData[1]->sfbEnergyMS.Long,
                                          psyData[0]->mdctSpectrum.Long,
                                          psyData[1]->mdctSpectrum.Long,
                                          currMdstEstimate->Long,
                                          psyData[0]->sfbThreshold.Long,
                                          psyData[1]->sfbThreshold.Long,
                                          psyOutChannel[1]->isBook,
                                          &psyOutElement->toolsInfo.msDigest,
                                          psyOutElement->toolsInfo.msMask,
                                          groupedSfbCnt[0],
                                          psyConf[cfgNo].sfbCnt,
                                          maxSfbPerGroup[0],
                                          ptrSfbOffset[0],
                                          (psyOutElement->commonWindow == 0),
                                          (psyConf[cfgNo].msMaskMerge && tnsSimilar),
                                          &psyOutElement->toolsInfo);
    }

    if (psyOutElement->toolsInfo.bCplxPredMdctActive == 0 && psyOutElement->toolsInfo.bUniSte == 0) {
      iisaacfenc_MsStereoProcessing(psyData[0]->sfbEnergy.Long,
                                    psyData[1]->sfbEnergy.Long,
                                    psyData[0]->sfbEnergyMS.Long,
                                    psyData[1]->sfbEnergyMS.Long,
                                    psyData[0]->mdctSpectrum.Long,
                                    psyData[1]->mdctSpectrum.Long,
                                    psyData[0]->sfbThreshold.Long,
                                    psyData[1]->sfbThreshold.Long,
                                    psyOutChannel[1]->isBook,
                                    &psyOutElement->toolsInfo.msDigest,
                                    psyOutElement->toolsInfo.msMask,
                                    groupedSfbCnt[0],
                                    psyConf[cfgNo].sfbCnt,
                                    maxSfbPerGroup[0],
                                    ptrSfbOffset[0],
                                    (psyOutElement->commonWindow == 0),
                                    (psyConf[cfgNo].msMaskMerge && tnsSimilar));
    }

    if (psyOutElement->toolsInfo.bUniSte == 1) {
      iisaacfenc_mapUniSteCldToPredGain(hUniSte,
                                        &psyConf[cfgNo],
                                        psyData[0]->blockSwitchingControl.windowSequence,
                                        groupedSfbCnt[0],
                                        psyConf[cfgNo].sfbCnt,
                                        maxSfbPerGroup[0],
                                        psyData[0]->blockSwitchingControl.groupLen,
                                        psyOutElement->toolsInfo.invPredGain);

      iisaacfenc_UniStePsyProcessing(psyData[0]->sfbThreshold.Long,
                                     psyData[1]->sfbThreshold.Long,
                                     &psyOutElement->toolsInfo.msDigest,
                                     psyOutElement->toolsInfo.msMask,
                                     groupedSfbCnt[0],
                                     psyConf[cfgNo].sfbCnt,
                                     maxSfbPerGroup[0],
                                     (psyOutElement->commonWindow == 0),
                                     psyOutElement->toolsInfo.invPredGain);
    }

  } else {
    setINT(0, psyOutElement->toolsInfo.msMask, MAX_GROUPED_SFB);

    setINT(0, psyOutChannel[0]->isBook, MAX_GROUPED_SFB);
    setINT(0, psyOutChannel[0]->isScale, MAX_GROUPED_SFB);
    setINT(0, psyOutChannel[1]->isBook, MAX_GROUPED_SFB);
    setINT(0, psyOutChannel[1]->isScale, MAX_GROUPED_SFB);
  }

  if (codecType == AACENC_CODEC_XHEAAC || codecType == AACENC_CODEC_MPEGH) {
    if (psyOutElement->toolsInfo.bCplxPredMdct == 1 && psyOutElement->toolsInfo.bCplxPredMdctActive == 1) {
      psyOutElement->toolsInfo.bCplxPredMdctActive = psyOutElement->toolsInfo.sfbPerPredBand - 1;
    }

    copyFLOAT(lastMdctSpectrum[0].Long, psyOutElement->toolsInfo.mdctSpectrumLPrev.Long, psyConf[0].granuleLength);

    copyFLOAT(lastMdctSpectrum[1].Long, psyOutElement->toolsInfo.mdctSpectrumRPrev.Long, psyConf[0].granuleLength);
  }
}
