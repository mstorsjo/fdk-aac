
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

#include <string.h>
#include <math.h>
#include <stddef.h>

#include "spaceEnclib_const.h"

#include "mathlib.h"
#include "bit_buf.h"
#include "qmflib.h"
#include "qmflib_lowfreqfilter.h"

#include "spaceEnclib.h"
#include "space_hybrid.h"
#include "space_tree.h"
#include "space_bitstream.h"
#include "space_onsetdetect.h"
#include "space_framewindowing.h"
#include "space_filter.h"

#include "sac_tsd.h"
#include "ApplDet_C_ifc.h"

#include "sac_tonality.h"
#include "space_staticgain.h"
#include "space_delay.h"
#include "space_paramextract.h"

#define MP4SPACEENC_LIBRARY_VERSION "03.05.17"

enum {
  SHORT_WINDOWS = 3
};

#define TSD_WINDOWING_WEIGHT 0.05f
#define TSD_ACTIVE_THRESHOLD 2.50f

struct MP4SPACE_ENCODER {
  float cpcStopFrequency;
  int const *qmf2HybridTable;
  HANDLE_FRAMEWINDOW hFrameWindow;
  int nSamplesValid;

  unsigned short int useHyperFraming;
  int bFirstFrameFlag;
  const float *pCldQuantTableEnc;
  int frameCount;

  MP4SPACEENC_BANDS_CONFIG nParamBands;
  MP4SPACEENC_TEMPSHAPECONFIG tempShapeConfig;
  MP4SPACEENC_SMOOTHCONFIG smoothConfig;
  unsigned short int useLowDelay;
  unsigned short int useLimiter;
  int bQmfOutput;

  MP4SPACEENC_MODE encMode;

  unsigned short int useFrameKeep;
  unsigned int independencyFactor;
  unsigned int nSampleRate;
  unsigned int nInputChannels;
  unsigned int nOutputChannels;
  unsigned int nTotalOutputChannels;
  unsigned int nChPerResidualElement;
  unsigned int nFrameTimeSlots;
  unsigned int nQmfBands;
  unsigned int nHybridBands;
  unsigned int nFrameLength;

  int nSamplesNext;
  unsigned int nAnalysisLengthTimeSlots;
  unsigned int nAnalysisLookaheadTimeSlots;
  unsigned int nUpdateHybridPositionTimeSlots;
  int *pnOutputBits;
  int nInputDelay;
  int nOutputBufferDelay;
  int nSurroundAnalysisBufferDelay;
  int nBitstreamDelayBuffer;
  int nBitstreamBufferRead;
  int nBitstreamBufferWrite;
  int nDiscardOutFrames;
  int avoid_keep;

  float epsilonFloat;

  unsigned short int useOneIcc;
  unsigned short int useCoarseQuantCld;
  unsigned short int useCoarseQuantIcc;
  unsigned short int useCoarseQuantCpc;

  MP4SPACEENC_IPDQUANTMODE useCoarseQuantIpd;

  unsigned short int bCalcDPL;
  unsigned int nPredictionBands;
  DECORRCONFIG decorrConfig;
  MP4SPACEENC_QUANTMODE quantMode;
  unsigned int coreCoderDelay;
  int timeAlignment;
  int bDMXAlign;

  int forceIndependency;

  int independencyCount;
  int independencyFlag;
  int **ppTrCurrPos;
  int trPrevPos[2 * MAX_NUM_TRANS];
  float *pFrameWindowAna[MAX_NUM_PARAMS];
  float *pFrameWindowSyn[MAX_NUM_PARAMS];
  FRAMEWIN_LIST frameWinList;
  SPATIALFRAME saveFrame;

  SPACE_TREE_SETUP spaceTreeSetup;
  MPEG4SPACEENC_SSCBUF sscBuf;
  HANDLE_QMFLIB_ANALYSIS *phQmfFiltIn;
  HANDLE_QMFLIB_SYNTHESIS *phQmfFiltOut;
  HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS *phHybFiltIn;
  HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS *phHybFiltOut;
  HANDLE_DC_FILTER *phDCFilterSigIn;
  HANDLE_ONSET_DETECT *phOnset;
  HANDLE_SPACE_TREE hSpaceTree;
  MP4SPACEENC_RES_CONFIG residualConfig;
  HANDLE_SPATIAL_TONALITY hSpatialTonality;
  HANDLE_BIT_BUF hBitBuf;
  HANDLE_BSF_INSTANCE hBitstreamFormatter;
  HANDLE_STATIC_GAIN_CONFIG hStaticGainConfig;
  HANDLE_STATIC_GAIN hStaticGain;
  HANDLE_DELAY_CONFIG hDelayConfig;
  HANDLE_DELAY hDelay;

  float *pInputBuffer;
  float **ppTmpRemapBuffer;
  float **ppTimeSigIn;
  float **ppTimeSigOut;
  float ***pppQmfSigOutReal;
  float ***pppQmfSigOutImag;
  float ***pppQmfInReal;
  float ***pppQmfInImag;
  float ***pppHybridInReal;
  float ***pppHybridInImag;

  float ***pppResidualDiffSigReal;
  float ***pppResidualDiffSigImag;

  float ***pppResDiffQmfReal;
  float ***pppResDiffQmfImag;
  float ***pppResDiffProcReal;
  float ***pppResDiffProcImag;

  float ***pppHybridOutReal;
  float ***pppHybridOutImag;
  float ***pppQmfOutReal;
  float ***pppQmfOutImag;
  float ***pppProcDataInReal;
  float ***pppProcDataInImag;
  float ***pppProcDataOutReal;
  float ***pppProcDataOutImag;
  unsigned char **ppBitstreamDelayBuffer;

  int nEncoderDelay;
  int nDecoderDelay;

  float *pOutputDelayBuffer;

  float **ppUmxMatReal[2][2];
  float **ppUmxMatImag[2][2];

  APPLDET_CONFIG appDetConfig;
  HANDLE_APPLDET *hApplDet;
  float totalVerdict[2];

  float TsdState;

  int bsHighRateMode;
  MP4SPACEENC_DOWNMIXTYPE downmixType;
  MP4SPACEENC_IPDMODE ipdMode;
  unsigned short int useIpdRelevancy;
  unsigned short int useOpdSmoothing;
  unsigned short int useDmxOverlapAdd;

  int bStereoSbr;
  int bPseudoLr;
  int bPsStyleDmx;
  int speechFlag;
};

static unsigned int const pValidBands[8] = {4, 5, 7, 10, 14, 20, 28, 40};

static HANDLE_ERROR_INFO
__CreateSpatialSpecificConfig(HANDLE_MP4SPACE_ENCODER const hEnc);

static HANDLE_ERROR_INFO
mp4SpaceEnc_SpaceTreeSetup(HANDLE_MP4SPACE_ENCODER const hEnc,
                           SPACE_TREE_SETUP *hSpaceTreeSetup);

static HANDLE_ERROR_INFO
mp4SpaceEnc_Reset(HANDLE_MP4SPACE_ENCODER const hEnc);

static void mp4SpaceEnc_rotateDmxResToPseudoLR(float *hRe,
                                               float *hIm);

static void mp4SpaceEnc_MoveBufUmxParamToFront(int ts,
                                               int delayUmxMat2Mdct,
                                               int numParamBands,
                                               float cld[][28],
                                               float umxMatRe[][28][4],
                                               float umxMatIm[][28][4],
                                               int nTimeSlots);

static void mp4SpaceEnc_AddBufUmxParamToEnd(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
                                            int ts,
                                            int delayUmxMat2Mdct,
                                            float cld[][28],
                                            float umxMatRe[][28][4],
                                            float umxMatIm[][28][4],
                                            int nTimeSlots,
                                            int const subband2parameterBand[],
                                            int bPseudoLr);

static HANDLE_ERROR_INFO
mapTempShapeConfig(TEMPSHAPECONFIG *const outTempShapeConfig,
                   MP4SPACEENC_TEMPSHAPECONFIG const inTempShapeConfig);

static HANDLE_ERROR_INFO
mp4SpaceEnc_Reset(HANDLE_MP4SPACE_ENCODER const hEnc) {
  HANDLE_ERROR_INFO error = noError;
  int param = 0;

  if (noError == error) {
    if (NULL == hEnc) {
      error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
    }
  }

  if (noError == error) {
    hEnc->encMode = MP4SPACEENC_INVALID_MODE;
    hEnc->nParamBands = MP4SPACEENC_BANDS_INVALID;
    hEnc->quantMode = MP4SPACEENC_QUANTMODE_INVALID;
    hEnc->smoothConfig = MP4SPACEENC_SMOOTHCONFIG_INVALID;
    hEnc->useOneIcc = 0;
    hEnc->useCoarseQuantIcc = 0;
    hEnc->useCoarseQuantCld = 0;
    hEnc->useCoarseQuantCpc = 0;

    hEnc->useCoarseQuantIpd = 0;

    hEnc->useFrameKeep = 0;
    hEnc->useLowDelay = 0;

    hEnc->useLimiter = 0;
    hEnc->bCalcDPL = 0;
    hEnc->useHyperFraming = 0;
    hEnc->independencyFactor = 0;
    hEnc->independencyCount = 0;
    hEnc->independencyFlag = 1;

    hEnc->independencyCount = -1;
    hEnc->forceIndependency = 0;

    hEnc->decorrConfig = DECORR_INVALID;

    hEnc->coreCoderDelay = 0;

    hEnc->downmixType = MP4SPACEENC_DOWNMIXTYPE_CLASSICMPS;
    hEnc->ipdMode = MP4SPACEENC_IPDMODE_NONE;
    hEnc->useIpdRelevancy = 0;
    hEnc->useOpdSmoothing = 0;
    hEnc->useDmxOverlapAdd = 1;

    hEnc->bStereoSbr = 0;
    hEnc->bPseudoLr = 0;
    hEnc->bPsStyleDmx = 0;
    hEnc->speechFlag = 0;

    hEnc->nPredictionBands = 0;

    hEnc->bDMXAlign = 0;
    hEnc->bFirstFrameFlag = 1;

    hEnc->nSampleRate = 0;
    hEnc->nInputChannels = 0;
    hEnc->nOutputChannels = 0;
    hEnc->nTotalOutputChannels = 0;
    hEnc->nFrameTimeSlots = 0;
    hEnc->nAnalysisLengthTimeSlots = 0;
    hEnc->nAnalysisLookaheadTimeSlots = 0;
    hEnc->nUpdateHybridPositionTimeSlots = 0;
    hEnc->nFrameLength = 0;
    hEnc->cpcStopFrequency = 0.f;
    hEnc->qmf2HybridTable = NULL;

    hEnc->nSamplesNext = 0;
    hEnc->nSamplesValid = 0;

    hEnc->phQmfFiltIn = NULL;
    hEnc->phHybFiltIn = NULL;
    hEnc->phQmfFiltOut = NULL;
    hEnc->phHybFiltOut = NULL;

    hEnc->ppTimeSigIn = NULL;
    hEnc->ppTimeSigOut = NULL;
    hEnc->pppQmfSigOutReal = NULL;
    hEnc->pppQmfSigOutImag = NULL;
    hEnc->pppQmfInReal = NULL;
    hEnc->pppQmfInImag = NULL;
    hEnc->pppHybridInReal = NULL;
    hEnc->pppHybridInImag = NULL;
    hEnc->pppResidualDiffSigReal = NULL;
    hEnc->pppResidualDiffSigImag = NULL;
    hEnc->pppResDiffQmfReal = NULL;
    hEnc->pppResDiffQmfImag = NULL;
    hEnc->pppResDiffProcReal = NULL;
    hEnc->pppResDiffProcImag = NULL;

    hEnc->pppHybridOutReal = NULL;
    hEnc->pppHybridOutImag = NULL;
    hEnc->pppQmfOutReal = NULL;
    hEnc->pppQmfOutImag = NULL;
    hEnc->pppProcDataInReal = NULL;
    hEnc->pppProcDataInImag = NULL;
    hEnc->pppProcDataOutReal = NULL;
    hEnc->pppProcDataOutImag = NULL;

    for (param = 0; param < MAX_NUM_PARAMS; param++) {
      hEnc->pFrameWindowAna[param] = NULL;
      hEnc->pFrameWindowSyn[param] = NULL;
    }

    hEnc->ppBitstreamDelayBuffer = NULL;
    hEnc->pnOutputBits = NULL;
    hEnc->pOutputDelayBuffer = NULL;

    hEnc->spaceTreeSetup.nParamBands = 0;
    hEnc->spaceTreeSetup.bOneIcc = 0;
    hEnc->spaceTreeSetup.mode = SPACETREE_INVALID_MODE;
    hEnc->spaceTreeSetup.epsilonFloat = 0.0f;
    hEnc->hSpaceTree = NULL;

    hEnc->spaceTreeSetup.downmixType = DOWNMIXTYPE_CLASSICMPS;
    hEnc->spaceTreeSetup.ipdMode = IPDMODE_NONE;
    hEnc->spaceTreeSetup.bDetectIpdRelevancy = 0;

    hEnc->spaceTreeSetup.bStereoSbr = 0;
    hEnc->spaceTreeSetup.bPsStyleDmx = 0;

    hEnc->spaceTreeSetup.bOttBandsPhasePresent = 0;
    hEnc->spaceTreeSetup.nOttBandsPhase = 0;

    hEnc->spaceTreeSetup.bUseTsd = 0;
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_GetFrameLen(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, int *nFrameLength) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hMp4SpaceEnc || NULL == nFrameLength) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (error == noError) {
    *nFrameLength = hMp4SpaceEnc->nFrameLength;
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_SetIndependencyCount(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, int value) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hMp4SpaceEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (error == noError) {
    hMp4SpaceEnc->independencyCount = value;
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_Open(
    HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc,
    HANDLE_MP4SPACEENC_SETUP const hSetup,
    int const bQmfOutput,
    CLASSIC_MPS const MP4SPACEENC_ENCODERTYPE encoderType) {
  HANDLE_ERROR_INFO error = noError;
  HANDLE_MP4SPACE_ENCODER hEnc = NULL;

  if (NULL == phMp4SpaceEnc || NULL == hSetup) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (noError == error) {
    *phMp4SpaceEnc = (HANDLE_MP4SPACE_ENCODER)iisCalloc(1, sizeof(struct MP4SPACE_ENCODER));
    if (*phMp4SpaceEnc == NULL) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    } else {
      hEnc = *phMp4SpaceEnc;
    }
  }

  SAFECALL(error, mp4SpaceEnc_Reset(hEnc));

  if (noError == error) {
    hEnc->nSampleRate = hSetup->sampleRate;
    hEnc->encMode = hSetup->encMode;
    hEnc->quantMode = hSetup->quantMode;
    hEnc->smoothConfig = hSetup->smoothConfig;
    hEnc->tempShapeConfig = hSetup->tempShapeConfig;
    hEnc->useOneIcc = hSetup->bUseOneIcc;
    hEnc->bCalcDPL = hSetup->bCalcDPL;
    hEnc->useCoarseQuantCld = hSetup->bUseCoarseQuant;
    hEnc->useCoarseQuantCpc = hSetup->bUseCoarseQuant;

    hEnc->useLowDelay = (hSetup->bLdMode > 0);
    hEnc->useFrameKeep = (hSetup->bLdMode == 2);

    hEnc->useCoarseQuantIpd = hSetup->bUseCoarseQuantIPD;

    hEnc->useCoarseQuantIcc = 0;
    hEnc->qmf2HybridTable = getQmf2HybridTable(hEnc->useLowDelay);
    hEnc->cpcStopFrequency = hSetup->cpcStopFrequency;
    hEnc->independencyFactor = hSetup->independencyFactor;
    hEnc->independencyCount = 0;

    hEnc->useLimiter = 0;

    hEnc->residualConfig = hSetup->residualConfig;

    hEnc->bDMXAlign = hSetup->bDMXAlign;
    hEnc->bQmfOutput = bQmfOutput;

    hEnc->timeAlignment = hSetup->timeAlignment;
    hEnc->frameCount = 0;

    hEnc->bsHighRateMode = hSetup->bsHighRateMode;

    hEnc->downmixType = hSetup->downmixType;
    hEnc->ipdMode = hSetup->ipdMode;
    hEnc->useOpdSmoothing = hSetup->bOpdSmoothing;
    hEnc->useIpdRelevancy = hSetup->bIpdRelevancy;
    hEnc->useDmxOverlapAdd = (hSetup->residualConfig.mode == MP4SPACEENC_RES_MODE_NONE) &&
                             ((hSetup->ipdMode == MP4SPACEENC_IPDMODE_NONE) ||
                              (hSetup->ipdMode == MP4SPACEENC_IPDMODE_NO_RESIDUAL));
    hEnc->useDmxOverlapAdd = hEnc->useDmxOverlapAdd &&
                             (hSetup->downmixType == MP4SPACEENC_DOWNMIXTYPE_CLASSICMPS);
    hEnc->bStereoSbr = hSetup->bStereoSbr;
    hEnc->bPseudoLr = hSetup->bPseudoLr;
    hEnc->bPsStyleDmx = hSetup->bPsStyleDmx;
    if (hEnc->bPsStyleDmx == 1 && hSetup->downmixType == MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL) {
      error = iisUtil_ERROR(CDI, "PsStyleDmx (above residual) and downmixType SIMPLIFIEC_ABOVE_RESIDUAL exclude each other");
    }
  }

  if (error == noError) {
    hEnc->nQmfBands = 64;
    hEnc->nHybridBands = hEnc->nQmfBands + 13;
  }

  if (noError == error) {
    unsigned int k = 0;
    while (k < sizeof(pValidBands) && pValidBands[k] != (unsigned int)hSetup->nParamBands) ++k;
    if (sizeof(pValidBands) == k) {
      error = iisUtil_ERROR(CDI, "Invalid Number of Parameter Bands");
      hEnc->nParamBands = MP4SPACEENC_BANDS_INVALID;
    } else {
      hEnc->nParamBands = hSetup->nParamBands;
    }
  }

  if (noError == error) {
    hEnc->decorrConfig = DECORR_QMFSPLIT0;
  }

  if (noError == error) {
    hEnc->nFrameTimeSlots = hSetup->frameTimeSlots;

    if (hEnc->nFrameTimeSlots < 1) {
      error = iisUtil_ERROR(CDI, "Invalid number nFrameTimeSlots.");
    } else {
      hEnc->nFrameLength = hEnc->nQmfBands * hEnc->nFrameTimeSlots;
      {
        hEnc->nAnalysisLengthTimeSlots = 2 * hEnc->nFrameTimeSlots;
        hEnc->nUpdateHybridPositionTimeSlots = 0;
      }
      {
        hEnc->nAnalysisLookaheadTimeSlots = hEnc->nAnalysisLengthTimeSlots - hEnc->nFrameTimeSlots;
      }
    }
  }

  if (noError == error) {
    hEnc->epsilonFloat = FLOAT_EPSILON;
  }

  SAFECALL(error, mp4SpaceEnc_SpaceTreeSetup(hEnc, &hEnc->spaceTreeSetup));

  SAFECALL(error, StaticGain_OpenConfig(&hEnc->hStaticGainConfig, hEnc->encMode));

  InitMathOpt();

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_Init(
    HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc,
    unsigned int *const pSamplesFirst,
    int dmxDelay,
    unsigned int *const pDiscardOutFrames,
    float aacCoreBandwidth) {
  HANDLE_ERROR_INFO error = noError;
  int ch = 0;

  HANDLE_MP4SPACE_ENCODER hEnc = NULL;
  SPACE_TREE_DESCRIPTION spaceTreeDescription = {0};
  int nChIn = 0;
  int nChOut = 0;
  int nChInArbDmx = 0;
  int nChRes = 0;
  int nChTotOut = 0;

  if (NULL == phMp4SpaceEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handlepointer");
  } else if (NULL == *phMp4SpaceEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (noError == error) {
    hEnc = *phMp4SpaceEnc;
  }

  if (error == noError) {
    hEnc->spaceTreeSetup.nHybBandsCore = 13 + (int)(64.0f * aacCoreBandwidth / (0.5f * (*phMp4SpaceEnc)->nSampleRate));
    hEnc->spaceTreeSetup.bStereoSbr = (*phMp4SpaceEnc)->bStereoSbr;
  }

  SAFECALL(error, SpaceTree_Open(&hEnc->hSpaceTree, &hEnc->spaceTreeSetup, hEnc->useLowDelay, hEnc->useFrameKeep));

  SAFECALL(error, SpaceTree_GetDescription(hEnc->hSpaceTree, &spaceTreeDescription));

  if (error == noError) {
    nChIn = hEnc->nInputChannels = spaceTreeDescription.nOutChannels;
    nChOut = hEnc->nOutputChannels = spaceTreeDescription.nInChannels;
    nChRes = spaceTreeDescription.nResidualChannels;
    nChTotOut = hEnc->nTotalOutputChannels = nChOut + nChRes;

    if (hEnc->residualConfig.codec == MP4SPACEENC_RES_CODEC_PCM) {
      nChOut += spaceTreeDescription.nResidualChannels;
      hEnc->nOutputChannels += spaceTreeDescription.nResidualChannels;
    }
  }

  if (error == noError) {
    if (NULL == (hEnc->phQmfFiltIn = (HANDLE_QMFLIB_ANALYSIS *)iisCalloc(nChIn, sizeof(HANDLE_QMFLIB_ANALYSIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (error == noError) {
    if (NULL == (hEnc->phHybFiltIn = (HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS *)iisCalloc(nChIn, sizeof(HANDLE_LOW_FREQUENCY_FILTER_ANALYSIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  for (ch = 0; (ch < nChIn) && (noError == error); ch++) {
    if (noError != (error = QMFlib_CreateAnalysis(&hEnc->phQmfFiltIn[ch], hEnc->nQmfBands, 0))) {
      error = handBack(error);
      break;
    }
    if (error == noError) {
      error = QMFlib_LowFrequencyFilterCreateAnalysis(&hEnc->phHybFiltIn[ch]);
    }
    if (error == noError) {
      error = QMFlib_LowFrequencyFilterInitAnalysis(hEnc->phHybFiltIn[ch], THREE_TO_SIXTEEN, hEnc->nQmfBands, hEnc->nQmfBands, 0);
    } else {
      error = handBack(error);
      break;
    }

    if (error == noError) {
      error = QMFlib_InitAnalysis(hEnc->phQmfFiltIn[ch], hEnc->nQmfBands);
    }
    if (error == noError) {
      error = QMFlib_ConfigAnalysis(hEnc->phQmfFiltIn[ch], FM_SBRQMF, 0);
    }
    if (error != noError) {
      error = handBack(error);
      break;
    }
  }

  if (error == noError) {
    if (NULL == (hEnc->phQmfFiltOut = (HANDLE_QMFLIB_SYNTHESIS *)iisCalloc(nChOut, sizeof(HANDLE_QMFLIB_SYNTHESIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  for (ch = 0; (ch < nChOut) && (noError == error); ch++) {
    if (noError != (error = QMFlib_CreateSynthesis(&hEnc->phQmfFiltOut[ch], hEnc->nQmfBands, 0))) {
      error = handBack(error);
      break;
    }

    if (error == noError) {
      error = QMFlib_InitSynthesis(hEnc->phQmfFiltOut[ch], hEnc->nQmfBands);
    }
    if (error == noError) {
      error = QMFlib_ConfigSynthesis(hEnc->phQmfFiltOut[ch], FM_SBRQMF);
    }
    if (error != noError) {
      error = handBack(error);
      break;
    }
  }

  if (error == noError) {
    if (NULL == (hEnc->phHybFiltOut = (HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS *)iisCalloc(nChTotOut, sizeof(HANDLE_LOW_FREQUENCY_FILTER_SYNTHESIS)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  for (ch = 0; (ch < nChTotOut) && (noError == error); ch++) {
    if (error == noError) {
      error = QMFlib_LowFrequencyFilterCreateSynthesis(&hEnc->phHybFiltOut[ch]);
    }
    if (error == noError) {
      error = QMFlib_LowFrequencyFilterInitSynthesis(hEnc->phHybFiltOut[ch], THREE_TO_SIXTEEN, hEnc->nQmfBands, hEnc->nQmfBands);
    } else {
      error = handBack(error);
      break;
    }
  }

  if (hEnc->residualConfig.mode == 0) {
    if (error == noError) {
      if (NULL == (hEnc->phDCFilterSigIn = (HANDLE_DC_FILTER *)iisCalloc(nChIn, sizeof(HANDLE_DC_FILTER)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
      }
    }
    for (ch = 0; (ch < nChIn) && (noError == error); ch++) {
      if (noError != (error = CreateDCFilter(&hEnc->phDCFilterSigIn[ch], hEnc->nSampleRate, hEnc->nFrameLength))) {
        error = handBack(error);
        break;
      }
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->phOnset = (HANDLE_ONSET_DETECT *)iisCalloc(nChIn, sizeof(HANDLE_ONSET_DETECT)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
    }
  }

  if (noError == error) {
    ONSET_DETECT_CONFIG onsetDetectConfig;
    onsetDetectConfig.maxTimeSlots = hEnc->nFrameTimeSlots;
    onsetDetectConfig.lowerBoundOnsetDetection = freq2HybridBand(1725.f, hEnc->nSampleRate, hEnc->nQmfBands, hEnc->useLowDelay);
    onsetDetectConfig.upperBoundOnsetDetection = hEnc->nHybridBands;
    onsetDetectConfig.epsilonFloat = hEnc->epsilonFloat;

    for (ch = 0; (ch < nChIn) && (noError == error); ch++) {
      error = OnsetDetect_Open(&hEnc->phOnset[ch], &onsetDetectConfig);
      if (noError != error) {
        error = handBack(error);
      }
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->ppTrCurrPos = (int **)iisCallocMatrix2D(nChIn, MAX_NUM_TRANS, sizeof(int)))) {
      error = iisUtil_ERROR(CDI, "Unable to calloc memory");
    }
  }

  if (noError == error) {
    FRAMEWINDOW_CONFIG framewindowConfig;
    framewindowConfig.nTimeSlotsMax = hEnc->nFrameTimeSlots;
    framewindowConfig.bLowDelay = hEnc->useLowDelay;
    framewindowConfig.bFrameKeep = hEnc->useFrameKeep;

    SAFECALL(error, FrameWindow_Create(&(hEnc->hFrameWindow), &framewindowConfig))
  }

  if (noError == error && hEnc->encMode == MP4SPACEENC_USAC_212) {
    if (hEnc->residualConfig.codec != MP4SPACEENC_RES_CODEC_AAC) {
      error = staticGain_SetDmxGain(hEnc->hStaticGainConfig, MP4SPACEENC_DMXGAIN_6_dB);

    } else {
      error = staticGain_SetDmxGain(hEnc->hStaticGainConfig, MP4SPACEENC_DMXGAIN_4_5_dB);
    }
  }

  SAFECALL(error, StaticGain_Open(&hEnc->hStaticGain, hEnc->hStaticGainConfig));

  if (error == noError) {
    hEnc->hBitBuf = CreateBitBuffer(MAX_MPEGS_BITS);
    if (NULL == hEnc->hBitBuf) {
      error = iisUtil_ERROR(CDI, "Unable to create bitBuffer.");
    }
  }

  if (error == noError) {
    BSF_RESIDUAL_CONFIG config;
    BSF_RESIDUAL_CONFIG_ARB_DMX configArbDmx;

    memset(&config, 0, sizeof(BSF_RESIDUAL_CONFIG));
    memset(&configArbDmx, 0, sizeof(BSF_RESIDUAL_CONFIG_ARB_DMX));

    config.sampleRate = GetResidualSampleRate(hEnc->nSampleRate, hEnc->nFrameTimeSlots, hEnc->residualConfig.framesPerSpatial, hEnc->encMode);
    config.codec = hEnc->residualConfig.codec;

    for (ch = 0; ch < MAX_NUM_BOXES; ch++) {
      config.enabled[ch] = hEnc->spaceTreeSetup.bCalcResiduals[ch];
      if (config.enabled[ch]) {
        config.bandWidth[ch] = SpaceTree_ParamBand2Freq(hEnc->nParamBands, hEnc->nSampleRate, hEnc->residualConfig.bands[ch], hEnc->nQmfBands);
        config.bitRate[ch] = hEnc->residualConfig.bitRate[ch];
        if (hEnc->residualConfig.bRestrictFramesize) {
          config.restrictResidualFramesize[ch] = hEnc->residualConfig.bRestrictFramesize;
        } else {
          config.restrictResidualFramesize[ch] = 0;
        }
      }
    }

    error = CreateSpatialBitstreamEncoder(&(hEnc->hBitstreamFormatter), &config, &configArbDmx);
    if (noError != error) {
      error = handBack(error);
    }
  }

  if (error == noError) {
    error = __CreateSpatialSpecificConfig(hEnc);
    if (noError != error) {
      error = handBack(error);
    }
  }

  if (error == noError) {
    int sscLenTmp;
    error = WriteSpatialSpecificConfig(hEnc->hBitBuf, GetSpatialSpecificConfig(hEnc->hBitstreamFormatter), &sscLenTmp);
    if (noError != error) {
      error = handBack(error);
    }
  }

  if (error == noError) {
    hEnc->sscBuf.nSscSizeBits = GetBitsAvail(hEnc->hBitBuf);

    if (NULL == (hEnc->sscBuf.pSsc = (unsigned char *)iisCalloc(1, (hEnc->sscBuf.nSscSizeBits + 7) / 8 * sizeof(unsigned char)))) {
      error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
    }
  }

  if (error == noError) {
    ByteAlign(hEnc->hBitBuf);

    ReadBytes(hEnc->hBitBuf, hEnc->sscBuf.pSsc, (hEnc->sscBuf.nSscSizeBits + 7) / 8);
  }

  if (noError == error) {
    if (NULL == (hEnc->ppTimeSigIn = (float **)iisCallocMatrix2D(nChIn, hEnc->nFrameLength, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  SAFECALL(error, mp4SpaceEnc_InitDelayCompensation(hEnc, dmxDelay));

  if (error == noError) {
    if (pDiscardOutFrames != NULL) {
      *pDiscardOutFrames = hEnc->nDiscardOutFrames;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer.");
    }
  }

  if (error == noError) {
    hEnc->nSamplesNext = hEnc->nFrameLength * (nChIn + nChInArbDmx);
    hEnc->nSamplesValid = 0;
    if (pSamplesFirst != NULL) {
      *pSamplesFirst = hEnc->nSamplesNext;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (error == noError) {
    int nChans = 0;
    nChans = 1;

    if (noError != (error = CreateSpatialTonality(&hEnc->hSpatialTonality,
                                                  hEnc->nParamBands, nChans, hEnc->nFrameTimeSlots, 0,
                                                  hEnc->nQmfBands, hEnc->nSampleRate))) {
      error = handBack(error);
    }
  }

  if ((noError == error) && (hEnc->bQmfOutput)) {
    if (noError == error) {
      if (NULL == (hEnc->pppQmfSigOutReal = (float ***)iisCallocMatrix3D(nChTotOut, 2 * hEnc->nFrameTimeSlots, hEnc->nQmfBands, sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }

    if (noError == error) {
      if (NULL == (hEnc->pppQmfSigOutImag = (float ***)iisCallocMatrix3D(nChTotOut, 2 * hEnc->nFrameTimeSlots, hEnc->nQmfBands, sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->ppTimeSigOut = (float **)iisCallocMatrix2D(nChTotOut, hEnc->nFrameLength, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppQmfInReal = (float ***)iisCallocMatrix3D(nChIn, hEnc->nFrameTimeSlots, hEnc->nQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppQmfInImag = (float ***)iisCallocMatrix3D(nChIn, hEnc->nFrameTimeSlots, hEnc->nQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppHybridInReal = (float ***)iisCallocMatrix3D(nChIn, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppHybridInImag = (float ***)iisCallocMatrix3D(nChIn, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppProcDataInReal = (float ***)iisCallocMatrix3D(nChIn, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppProcDataInImag = (float ***)iisCallocMatrix3D(nChIn, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppQmfOutReal = (float ***)iisCallocMatrix3D(nChTotOut, hEnc->nFrameTimeSlots * (RESIDUAL_CODING_DELAY + 1), hEnc->nQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppQmfOutImag = (float ***)iisCallocMatrix3D(nChTotOut, hEnc->nFrameTimeSlots * (RESIDUAL_CODING_DELAY + 1), hEnc->nQmfBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (hEnc->encMode == MP4SPACEENC_USAC_212) {
    if (noError == error) {
      if (NULL == (hEnc->pppHybridOutReal = (float ***)iisCallocMatrix3D((hEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD) ? 2 : hEnc->nTotalOutputChannels, 2 * hEnc->nFrameTimeSlots, MAX_HYBRID_BANDS, sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppHybridOutImag = (float ***)iisCallocMatrix3D((hEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD) ? 2 : nChTotOut, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }
  if (noError == error) {
    if (NULL == (hEnc->pppProcDataOutReal = (float ***)iisCallocMatrix3D((hEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD) ? 2 : nChTotOut, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if (NULL == (hEnc->pppProcDataOutImag = (float ***)iisCallocMatrix3D((hEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD) ? 2 : nChTotOut, hEnc->nAnalysisLengthTimeSlots, hEnc->nHybridBands, sizeof(float)))) {
      error = iisUtil_ERROR(CDI, "Memory allocation failed.");
    }
  }

  if (noError == error) {
    if ((hEnc->encMode == MP4SPACEENC_USAC_212) && !hEnc->useDmxOverlapAdd) {
      int i, j;

      for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
          if (noError == error) {
            if (NULL == (hEnc->ppUmxMatReal[i][j] = (float **)iisCallocMatrix2D(hEnc->nFrameTimeSlots, MAX_HYBRID_BANDS, sizeof(float)))) {
              error = iisUtil_ERROR(CDI, "Memory allocation failed.");
            }
          }

          if (noError == error) {
            if (NULL == (hEnc->ppUmxMatImag[i][j] = (float **)iisCallocMatrix2D(hEnc->nFrameTimeSlots, MAX_HYBRID_BANDS, sizeof(float)))) {
              error = iisUtil_ERROR(CDI, "Memory allocation failed.");
            }
          }
        }
      }
    }
  }

  if (noError == error) {
    int paramSet;

    for (paramSet = 0; paramSet < MAX_NUM_PARAMS; paramSet++) {
      if (NULL == (hEnc->pFrameWindowAna[paramSet] = (float *)iisCalloc(1, hEnc->nAnalysisLengthTimeSlots * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
        break;
      }
    }
  }

  if (noError == error) {
    int paramSet;

    for (paramSet = 0; paramSet < MAX_NUM_PARAMS; paramSet++) {
      if (NULL == (hEnc->pFrameWindowSyn[paramSet] = (float *)iisCalloc(1, hEnc->nAnalysisLengthTimeSlots * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
        break;
      }
    }
  }

  if (NULL == (hEnc->hApplDet = (HANDLE_APPLDET *)iisCalloc(nChIn, sizeof(HANDLE_APPLDET)))) {
    error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
  }
  if (noError == error) {
    hEnc->appDetConfig.fs = hEnc->nSampleRate;
    hEnc->appDetConfig.frameSize = 1024;

    for (ch = 0; ch < nChIn; ch++) {
      if (AD_NO_ERROR != APPLDET_C_IFC_Open(&(hEnc->hApplDet[ch]), &hEnc->appDetConfig)) {
        error = iisUtil_ERROR(CDI, "Memory Allocation Failed");
        break;
      }
    }
  }

  return error;
}

static HANDLE_ERROR_INFO
__FeedDeinterPreScale(HANDLE_MP4SPACE_ENCODER hEnc,
                      float const *const pSamples,
                      int const nSamples,
                      int *pnSamplesFed) {
  HANDLE_ERROR_INFO error = noError;
  float *pPreGain;
  float const *pInput = NULL;
  int ch = 0;
  int nSamplesFed = 0;

  if (error == noError) {
    if (hEnc == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (nSamples != 0) {
    int nChIn = 0;
    int nChInArbDmx = 0;
    int nChInWithDmx = 0;

    int samplesToFeed = 0;
    int nSamplesPerChannel = 0;

    if (error == noError) {
      nChIn = hEnc->nInputChannels;
      nChInWithDmx = nChIn + nChInArbDmx;
      samplesToFeed = min(nSamples, hEnc->nSamplesNext - hEnc->nSamplesValid);
      nSamplesPerChannel = samplesToFeed / nChInWithDmx;
    }

    if (error == noError) {
      if (samplesToFeed < 0) {
        error = iisUtil_ERROR(CDI, "Invalid samplesToFeed.");
      }
    }

    if (error == noError) {
      if (samplesToFeed % nChInWithDmx != 0) {
        error = iisUtil_ERROR(CDI, "Number of Samples must be a multiple of channel number");
      }
    }

    if (error == noError) {
      if ((unsigned int)samplesToFeed > nChInWithDmx * hEnc->nFrameLength) {
        error = iisUtil_ERROR(CDI, "number of samples exceeds max buffer length.");
      }
    }

    if (error == noError) {
      if (pSamples == NULL) {
        error = iisUtil_ERROR(CDI, "Invalid handle.");
      }
    }

    if (error == noError) {
      if (hEnc->bDMXAlign < 1) {
        pInput = pSamples;
      } else {
        pInput = hEnc->pInputBuffer;

        copyFLOAT(&(pInput[hEnc->nFrameLength * nChInWithDmx]), hEnc->pInputBuffer, (hEnc->nInputDelay * nChInWithDmx));

        copyFLOAT(pSamples, &(hEnc->pInputBuffer[hEnc->nInputDelay * nChInWithDmx]), (hEnc->nFrameLength * nChInWithDmx));
      }
    }

    if (error == noError) {
      pPreGain = GetPreGainPtr(hEnc->hStaticGain);

      for (ch = 0; ch < nChIn; ch++) {
        copyFLOAT(&(hEnc->ppTimeSigIn[ch][hEnc->nFrameLength]), &(hEnc->ppTimeSigIn[ch][0]), hEnc->nSurroundAnalysisBufferDelay);

        smulFLOATflex(pPreGain[ch],
                      pInput + ch, nChInWithDmx,
                      &(hEnc->ppTimeSigIn[ch][hEnc->nSurroundAnalysisBufferDelay]), 1,
                      nSamplesPerChannel);
      }

      hEnc->nSamplesValid += samplesToFeed;
      nSamplesFed = samplesToFeed;
    }
  } else {
    error = iisUtil_ERROR(CDI, "Flushing not implemented");
  }

  if (error == noError) {
    if (NULL == pnSamplesFed) {
      error = iisUtil_ERROR(CDI, "Invalid Pointer");
    } else {
      *pnSamplesFed = nSamplesFed;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_Encode(
    HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
    float const *const pInputSamples,
    unsigned int const nInputSamples,
    unsigned int *const pSamplesConsumed,
    unsigned int *const pSamplesNext,
    float *const pOutputSamples,
    float **const ppQmfOutputSamplesReal,
    float **const ppQmfOutputSamplesImag,
    unsigned int *const pnOutputSamples,
    const unsigned int nOutputSamplesBufferSize,
    unsigned char *const pOutputBuffer,
    const int nOutputBufferSize,
    int *const pOutBits,
    int bUsacIndependencyFlag) {
  HANDLE_ERROR_INFO error = noError;
  SPATIALFRAME *pFrameData = NULL;
  int ch = 0;
  int ps = 0;
  int winCnt = 0;
  unsigned int ts = 0;
  int nChIn = 0;
  int nChOut = 0;
  int nChTotOut = 0;
  int nChInArbDmx = 0;
  int nChInWithDmx = 0;
  int nSamplesPerChannel = 0;
  int currTransPos = -1;
  unsigned int nOutputSamplesMax = 0;
  unsigned int nOutputSamples = 0;
  unsigned int nFrameTimeSlots = 0;
  unsigned int slot = 0;
  SPACE_TREE_DESCRIPTION spaceTreeDescription = {0};

  float ***pppTreeOutputReal = NULL;
  float ***pppTreeOutputImag = NULL;

  if (error == noError) {
    if (NULL == hMp4SpaceEnc) {
      error = iisUtil_ERROR(CDI, "Invalid Handle");
    }
  }

  if (error == noError) {
    if (NULL == pSamplesConsumed) {
      error = iisUtil_ERROR(CDI, "Invalid Pointer pSamplesConsumed");
    }
  }

  SAFECALL(error, SpaceTree_GetDescription(hMp4SpaceEnc->hSpaceTree, &spaceTreeDescription));

  if (error == noError) {
    nChIn = hMp4SpaceEnc->nInputChannels;
    nChInWithDmx = nChIn + nChInArbDmx;
    nChOut = hMp4SpaceEnc->nOutputChannels;
    {
      nChTotOut = hMp4SpaceEnc->nTotalOutputChannels;
    }

    nSamplesPerChannel = nInputSamples / nChInWithDmx;
    nOutputSamplesMax = nSamplesPerChannel * nChOut;
    nFrameTimeSlots = hMp4SpaceEnc->nFrameTimeSlots;
  }

  if (error == noError) {
    if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212 && bUsacIndependencyFlag) {
      hMp4SpaceEnc->forceIndependency = 1;
    }
  }

  if (noError == error) {
    if (0 != nInputSamples % nChInWithDmx) {
      error = iisUtil_ERROR(CDI, "Number of Input Samples must be a multiple of nChannelsInWithDmx");
    }
  }

  if (noError == error) {
    pFrameData = GetSpatialFrame(hMp4SpaceEnc->hBitstreamFormatter, RESIDUAL_CODING_DELAY);
    if (NULL == pFrameData) {
      error = iisUtil_ERROR(CDI, "Invalid Pointer to Spatial Frame.");
    }
  }

  if (noError == error) {
    if (hMp4SpaceEnc->nDiscardOutFrames > 0) {
      hMp4SpaceEnc->independencyCount = 0;
      hMp4SpaceEnc->independencyFlag = 1;
    } else {
      hMp4SpaceEnc->independencyFlag = (hMp4SpaceEnc->independencyCount == 0) ? 1 : 0;
      if (hMp4SpaceEnc->independencyFactor > 0) {
        hMp4SpaceEnc->independencyCount++;
        hMp4SpaceEnc->independencyCount = hMp4SpaceEnc->independencyCount % ((int)hMp4SpaceEnc->independencyFactor);
      } else {
        hMp4SpaceEnc->independencyCount = -1;
      }
    }
  }

  if (noError == error) {
    int consumed = 0;
    *pSamplesConsumed = 0;
    error = __FeedDeinterPreScale(hMp4SpaceEnc,
                                  pInputSamples,
                                  nInputSamples,
                                  &consumed);
    if (noError != error) {
      error = handBack(error);
    }
    *pSamplesConsumed += consumed;
  }

  if (noError == error) {
    if (hMp4SpaceEnc->nSamplesNext != hMp4SpaceEnc->nSamplesValid) {
      error = iisUtil_ERROR(CDI, "Not Enough input samples");
    }
  }

  if (error == noError) {
    if (hMp4SpaceEnc->residualConfig.mode == 0) {
      for (ch = 0; ch < nChIn; ch++) {
        if (noError != (error = ApplyDCFilter(hMp4SpaceEnc->phDCFilterSigIn[ch], hMp4SpaceEnc->ppTimeSigIn[ch], nSamplesPerChannel))) {
          error = handBack(error);
          break;
        }
      }
    }
  }

  if (noError == error) {
    for (ts = 0; ts < nFrameTimeSlots; ts++) {
      for (ch = 0; ch < nChIn; ch++) {
        {
          if (noError != QMFlib_CalculateAnalysis(hMp4SpaceEnc->phQmfFiltIn[ch],
                                                  &hMp4SpaceEnc->ppTimeSigIn[ch][ts * hMp4SpaceEnc->nQmfBands],
                                                  hMp4SpaceEnc->pppQmfInReal[ch][ts],
                                                  hMp4SpaceEnc->pppQmfInImag[ch][ts])) {
            error = handBack(error);
            break;
          }
        }
      }
    }
  }

  if (noError == error && !hMp4SpaceEnc->useLowDelay) {
    for (ch = 0; ch < nChIn; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        error = QMFlib_LowFrequencyFilterCalculateAnalysis(hMp4SpaceEnc->phHybFiltIn[ch],
                                                           hMp4SpaceEnc->pppQmfInReal[ch][ts],
                                                           hMp4SpaceEnc->pppQmfInImag[ch][ts],

                                                           hMp4SpaceEnc->pppHybridInReal[ch][hMp4SpaceEnc->nAnalysisLookaheadTimeSlots + ts],
                                                           hMp4SpaceEnc->pppHybridInImag[ch][hMp4SpaceEnc->nAnalysisLookaheadTimeSlots + ts]);
        if (error != noError) {
          error = handBack(error);
          break;
        }
      }
    }
  }

  if (error == noError) {
    if (hMp4SpaceEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD) {
      pFrameData->tsdData.bsTsdEnable = 0;
      for (ch = 0; ch < nChIn; ch++) {
        for (ts = 0; ts < nFrameTimeSlots * hMp4SpaceEnc->nQmfBands; ts += hMp4SpaceEnc->appDetConfig.frameSize / 2) {
          unsigned short int verdict;
          float confidence;
          if (AD_NO_ERROR != APPLDET_C_IFC_Advance(hMp4SpaceEnc->hApplDet[ch],
                                                   &hMp4SpaceEnc->ppTimeSigIn[ch][ts],
                                                   &verdict,
                                                   &confidence)) {
            error = iisUtil_ERROR(CDI, "Applause detection failed");
            break;
          }

          hMp4SpaceEnc->totalVerdict[ch] = (1.0f - TSD_WINDOWING_WEIGHT) * hMp4SpaceEnc->totalVerdict[ch] +
                                           (TSD_WINDOWING_WEIGHT)*verdict;
        }
      }

      if ((hMp4SpaceEnc->totalVerdict[0] > TSD_ACTIVE_THRESHOLD) &&
          (hMp4SpaceEnc->totalVerdict[1] > TSD_ACTIVE_THRESHOLD)) {
        pFrameData->tsdData.bsTsdEnable = 1;
      }

      if (pFrameData->tsdData.bsTsdEnable) {
        SAFECALL(error, CalcTSDpositions(&hMp4SpaceEnc->TsdState,
                                         &pFrameData->tsdData,
                                         hMp4SpaceEnc->pppHybridInReal,
                                         hMp4SpaceEnc->pppHybridInImag,
                                         hMp4SpaceEnc->nSampleRate,
                                         nFrameTimeSlots,
                                         &pFrameData->bUseBBCues,
                                         hMp4SpaceEnc->nHybridBands));
      }
    } else {
      pFrameData->tsdData.bsTsdEnable = 0;
    }
  }

  if (error == noError) {
    currTransPos = -1;
    for (ch = 0; (ch < nChIn) && (noError == error); ch++) {
      if (ch != 3) {
        error = OnsetDetect_Apply(hMp4SpaceEnc->phOnset[ch],
                                  nFrameTimeSlots,
                                  hMp4SpaceEnc->nHybridBands,
                                  &hMp4SpaceEnc->pppHybridInReal[ch][hMp4SpaceEnc->nAnalysisLookaheadTimeSlots],
                                  &hMp4SpaceEnc->pppHybridInImag[ch][hMp4SpaceEnc->nAnalysisLookaheadTimeSlots],
                                  hMp4SpaceEnc->trPrevPos[1],
                                  SPACE_ONSET_THRESHOLD,
                                  hMp4SpaceEnc->ppTrCurrPos[ch], hMp4SpaceEnc->bsHighRateMode);

        if (noError != error) {
          error = handBack(error);
        }

        if (hMp4SpaceEnc->ppTrCurrPos[ch][0] >= 0) {
          if (currTransPos < 0) {
            currTransPos = hMp4SpaceEnc->ppTrCurrPos[ch][0];
          } else if (hMp4SpaceEnc->ppTrCurrPos[ch][0] < currTransPos) {
            currTransPos = hMp4SpaceEnc->ppTrCurrPos[ch][0];
          }
        }
      }
    }
  }

  if (noError == error) {
    hMp4SpaceEnc->trPrevPos[0] = max(-1, hMp4SpaceEnc->trPrevPos[1] - (int)nFrameTimeSlots);
    hMp4SpaceEnc->trPrevPos[1] = currTransPos;
  }

  for (ch = 0; (ch < nChIn) && (noError == error); ch++) {
    error = OnsetDetect_Update(hMp4SpaceEnc->phOnset[ch],
                               nFrameTimeSlots);
    if (noError != error) {
      error = handBack(error);
    }
  }

  if (noError == error) {
    error = FrameWindow_GetWindow(hMp4SpaceEnc->hFrameWindow,
                                  hMp4SpaceEnc->trPrevPos,
                                  nFrameTimeSlots,
                                  &pFrameData->framingInfo,
                                  hMp4SpaceEnc->pFrameWindowAna,
                                  hMp4SpaceEnc->pFrameWindowSyn,
                                  &hMp4SpaceEnc->frameWinList,
                                  hMp4SpaceEnc->avoid_keep);

    if (noError != error) {
      error = handBack(error);
    }
  }

  if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
    if (hMp4SpaceEnc->useDmxOverlapAdd) {
      pppTreeOutputReal = hMp4SpaceEnc->pppProcDataOutReal;
      pppTreeOutputImag = hMp4SpaceEnc->pppProcDataOutImag;
    } else {
      pppTreeOutputReal = hMp4SpaceEnc->pppHybridOutReal;
      pppTreeOutputImag = hMp4SpaceEnc->pppHybridOutImag;
    }
  }

  winCnt = 0;
  for (ps = 0; (noError == error) && (ps < hMp4SpaceEnc->frameWinList.n); ++ps) {
    if (hMp4SpaceEnc->frameWinList.dat[ps].hold == FW_HOLD) {
      error = DuplicateParameterSet(&hMp4SpaceEnc->saveFrame, 0,
                                    pFrameData, ps);
      if (noError != error) {
        error = handBack(error);
      }

      if ((hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) && !hMp4SpaceEnc->useDmxOverlapAdd) {
        SAFECALL(error,
                 SpaceTree_CalcDownmixHold(hMp4SpaceEnc->hSpaceTree,
                                           hMp4SpaceEnc->frameWinList.n,
                                           ps,
                                           nFrameTimeSlots,
                                           hMp4SpaceEnc->nHybridBands,
                                           pFrameData,
                                           hMp4SpaceEnc->pppHybridInReal,
                                           hMp4SpaceEnc->pppHybridInImag,
                                           hMp4SpaceEnc->ppUmxMatReal,
                                           hMp4SpaceEnc->ppUmxMatImag,
                                           hMp4SpaceEnc->pppHybridOutReal,
                                           hMp4SpaceEnc->pppHybridOutImag));
      }

    } else {
      for (ch = 0; ch < nChIn; ch++) {
        for (ts = 0; ts < hMp4SpaceEnc->nAnalysisLengthTimeSlots; ++ts) {
          smulFLOAT(hMp4SpaceEnc->pFrameWindowAna[winCnt][ts],
                    hMp4SpaceEnc->pppHybridInReal[ch][ts],
                    hMp4SpaceEnc->pppProcDataInReal[ch][ts],
                    hMp4SpaceEnc->nHybridBands);
          smulFLOAT(hMp4SpaceEnc->pFrameWindowAna[winCnt][ts],
                    hMp4SpaceEnc->pppHybridInImag[ch][ts],
                    hMp4SpaceEnc->pppProcDataInImag[ch][ts],
                    hMp4SpaceEnc->nHybridBands);
        }
      }

      if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
        SAFECALL(error,
                 SpaceTree_Apply(hMp4SpaceEnc->hSpaceTree,
                                 ps,
                                 nChIn,
                                 hMp4SpaceEnc->nAnalysisLengthTimeSlots,
                                 hMp4SpaceEnc->nHybridBands,
                                 hMp4SpaceEnc->pppHybridInReal,
                                 hMp4SpaceEnc->pppHybridInImag,
                                 hMp4SpaceEnc->ppUmxMatReal,
                                 hMp4SpaceEnc->ppUmxMatImag,
                                 hMp4SpaceEnc->pppProcDataInReal,
                                 hMp4SpaceEnc->pppProcDataInImag,
                                 pppTreeOutputReal,
                                 pppTreeOutputImag,
                                 pFrameData,
                                 hMp4SpaceEnc->avoid_keep,
                                 hMp4SpaceEnc->speechFlag));
      }

      SAFECALL(error,
               DuplicateParameterSet(pFrameData, ps, &hMp4SpaceEnc->saveFrame, 0));

      if (noError == error) {
        int nCh = 0;

        if (hMp4SpaceEnc->useDmxOverlapAdd && hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
          nCh = (hMp4SpaceEnc->tempShapeConfig == 3) ? 2 : nChTotOut;
        } else if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
          nCh = 0;
        } else {
          nCh = nChTotOut;
        }

        for (ch = 0; ch < nCh; ch++) {
          for (ts = 0; ts < hMp4SpaceEnc->nAnalysisLengthTimeSlots; ++ts) {
            smultFLOATip(hMp4SpaceEnc->pFrameWindowSyn[winCnt][ts],
                         hMp4SpaceEnc->pppProcDataOutReal[ch][ts],
                         hMp4SpaceEnc->nHybridBands);
            smultFLOATip(hMp4SpaceEnc->pFrameWindowSyn[winCnt][ts],
                         hMp4SpaceEnc->pppProcDataOutImag[ch][ts],
                         hMp4SpaceEnc->nHybridBands);

            addFLOAT(hMp4SpaceEnc->pppProcDataOutReal[ch][ts],
                     hMp4SpaceEnc->pppHybridOutReal[ch][ts],
                     hMp4SpaceEnc->pppHybridOutReal[ch][ts],
                     hMp4SpaceEnc->nHybridBands);
            addFLOAT(hMp4SpaceEnc->pppProcDataOutImag[ch][ts],
                     hMp4SpaceEnc->pppHybridOutImag[ch][ts],
                     hMp4SpaceEnc->pppHybridOutImag[ch][ts],
                     hMp4SpaceEnc->nHybridBands);
          }
        }
      }
      ++winCnt;
    }
    if (hMp4SpaceEnc->avoid_keep > 0) {
      hMp4SpaceEnc->avoid_keep--;
    }
  }

  if ((error == noError) && (hMp4SpaceEnc->tempShapeConfig == MP4SPACEENC_TEMPSHAPE_TSD)) {
    SAFECALL(error, CalcTSDphase(&pFrameData->tsdData,
                                 hMp4SpaceEnc->pppHybridOutReal,
                                 hMp4SpaceEnc->pppHybridOutImag,
                                 nFrameTimeSlots));
  }

  if (noError == error) {
    for (ch = 0; ch < nChIn; ch++) {
      for (slot = 0; slot < hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots; slot++) {
        copyFLOAT(hMp4SpaceEnc->pppHybridInReal[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppHybridInReal[ch][slot],
                  hMp4SpaceEnc->nHybridBands);
        copyFLOAT(hMp4SpaceEnc->pppHybridInImag[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppHybridInImag[ch][slot],
                  hMp4SpaceEnc->nHybridBands);
      }
      for (slot = 0; slot < nFrameTimeSlots; slot++) {
        setFLOAT(0.0f, hMp4SpaceEnc->pppHybridInReal[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots + slot], hMp4SpaceEnc->nHybridBands);
        setFLOAT(0.0f, hMp4SpaceEnc->pppHybridInImag[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots + slot], hMp4SpaceEnc->nHybridBands);
      }
    }
  }

  if (noError == error && !hMp4SpaceEnc->useLowDelay) {
    for (ch = 0; ch < nChOut; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        error = QMFlib_LowFrequencyFilterCalculateSynthesis(hMp4SpaceEnc->phHybFiltOut[ch],
                                                            hMp4SpaceEnc->pppHybridOutReal[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + ts],
                                                            hMp4SpaceEnc->pppHybridOutImag[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + ts],

                                                            hMp4SpaceEnc->pppQmfOutReal[ch][ts],
                                                            hMp4SpaceEnc->pppQmfOutImag[ch][ts]);
        if (error != noError) {
          error = handBack(error);
          break;
        }
      }
    }

    for (; ch < nChTotOut; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        error = QMFlib_LowFrequencyFilterCalculateSynthesis(hMp4SpaceEnc->phHybFiltOut[ch],
                                                            hMp4SpaceEnc->pppHybridOutReal[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + ts],
                                                            hMp4SpaceEnc->pppHybridOutImag[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + ts],

                                                            hMp4SpaceEnc->pppQmfOutReal[ch][RESIDUAL_CODING_DELAY * nFrameTimeSlots + ts],
                                                            hMp4SpaceEnc->pppQmfOutImag[ch][RESIDUAL_CODING_DELAY * nFrameTimeSlots + ts]);
        if (error != noError) {
          error = handBack(error);
          break;
        }
      }
    }
  }

  if ((error == noError) && (hMp4SpaceEnc->smoothConfig == MP4SPACEENC_SMOOTHCONFIG_AUTOON)) {
    int independencyFlag = hMp4SpaceEnc->independencyFlag;

    if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
      independencyFlag = (hMp4SpaceEnc->independencyCount == 0) || hMp4SpaceEnc->forceIndependency;
    }

    SAFECALL(error, ApplySpatialTonality(hMp4SpaceEnc->hSpatialTonality,
                                         &pFrameData->smgData,
                                         hMp4SpaceEnc->pppQmfOutReal,
                                         hMp4SpaceEnc->pppQmfOutImag,
                                         independencyFlag));
  }

  if (noError == error) {
    for (ch = 0; ch < nChTotOut; ch++) {
      for (slot = 0; slot < hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots; slot++) {
        copyFLOAT(hMp4SpaceEnc->pppHybridOutReal[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppHybridOutReal[ch][slot],
                  hMp4SpaceEnc->nHybridBands);
        copyFLOAT(hMp4SpaceEnc->pppHybridOutImag[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppHybridOutImag[ch][slot],
                  hMp4SpaceEnc->nHybridBands);
      }
      for (slot = 0; slot < nFrameTimeSlots; slot++) {
        setFLOAT(0.0f, hMp4SpaceEnc->pppHybridOutReal[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots + slot], hMp4SpaceEnc->nHybridBands);
        setFLOAT(0.0f, hMp4SpaceEnc->pppHybridOutImag[ch][hMp4SpaceEnc->nUpdateHybridPositionTimeSlots + nFrameTimeSlots + slot], hMp4SpaceEnc->nHybridBands);
      }
    }
  }

  if ((hMp4SpaceEnc->bQmfOutput) && (noError == error)) {
    for (ch = 0; ch < nChOut; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        copyFLOAT(hMp4SpaceEnc->pppQmfOutReal[ch][ts],
                  hMp4SpaceEnc->pppQmfSigOutReal[ch][ts + hMp4SpaceEnc->nOutputBufferDelay / hMp4SpaceEnc->nQmfBands],
                  hMp4SpaceEnc->nQmfBands);
        copyFLOAT(hMp4SpaceEnc->pppQmfOutImag[ch][ts],
                  hMp4SpaceEnc->pppQmfSigOutImag[ch][ts + hMp4SpaceEnc->nOutputBufferDelay / hMp4SpaceEnc->nQmfBands],
                  hMp4SpaceEnc->nQmfBands);
      }
    }
  }

  if (noError == error) {
    for (ch = 0; ch < nChOut; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        error = QMFlib_CalculateSynthesis(hMp4SpaceEnc->phQmfFiltOut[ch],
                                          hMp4SpaceEnc->pppQmfOutReal[ch][ts],
                                          hMp4SpaceEnc->pppQmfOutImag[ch][ts],
                                          &hMp4SpaceEnc->ppTimeSigOut[ch][ts * hMp4SpaceEnc->nQmfBands]);

        if (noError != error) {
          error = handBack(error);
          break;
        }
      }
    }
  }

  if (noError == error) {
    for (ch = nChOut; ch < nChTotOut; ch++) {
      for (slot = 0; slot < nFrameTimeSlots * RESIDUAL_CODING_DELAY; slot++) {
        copyFLOAT(hMp4SpaceEnc->pppQmfOutReal[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppQmfOutReal[ch][slot],
                  hMp4SpaceEnc->nQmfBands);

        copyFLOAT(hMp4SpaceEnc->pppQmfOutImag[ch][nFrameTimeSlots + slot],
                  hMp4SpaceEnc->pppQmfOutImag[ch][slot],
                  hMp4SpaceEnc->nQmfBands);
      }

      for (slot = 0; slot < nFrameTimeSlots; slot++) {
        setFLOAT(0.0, hMp4SpaceEnc->pppQmfOutReal[ch][RESIDUAL_CODING_DELAY * nFrameTimeSlots + slot], hMp4SpaceEnc->nQmfBands);
        setFLOAT(0.0, hMp4SpaceEnc->pppQmfOutImag[ch][RESIDUAL_CODING_DELAY * nFrameTimeSlots + slot], hMp4SpaceEnc->nQmfBands);
      }
    }
  }

  if (noError == error) {
    {
      pFrameData->bsIndependencyFlag = (hMp4SpaceEnc->independencyCount == 0);
      if (hMp4SpaceEnc->independencyCount == -1) {
        pFrameData->bsIndependencyFlag = (hMp4SpaceEnc->nDiscardOutFrames >= hMp4SpaceEnc->nBitstreamDelayBuffer - 1) ? 1 : 0;
      }
      if (hMp4SpaceEnc->forceIndependency) pFrameData->bsIndependencyFlag = 1;
    }
    error = WriteSpatialFrame(hMp4SpaceEnc->hBitBuf, hMp4SpaceEnc->hBitstreamFormatter, bUsacIndependencyFlag);
    if (noError != error) {
      error = handBack(error);
    }
  }

  if (error == noError) {
    hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferWrite] = GetBitsAvail(hMp4SpaceEnc->hBitBuf);
    if (hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferWrite] > MAX_MPEGS_BITS) {
      error = iisUtil_ERROR(CDI, "Output Buffer too small.");
    }
  }

  if (error == noError) {
    ByteAlign(hMp4SpaceEnc->hBitBuf);
    ReadBytes(hMp4SpaceEnc->hBitBuf,
              hMp4SpaceEnc->ppBitstreamDelayBuffer[hMp4SpaceEnc->nBitstreamBufferWrite],
              (hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferWrite] + 7) / 8);
  }

  if (error == noError) {
    if (pOutBits != NULL) {
      if (hMp4SpaceEnc->nDiscardOutFrames == 0) {
        int const outBits = hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferRead];
        if (nOutputBufferSize >= (outBits + 7) / 8) {
          *pOutBits = outBits;
        } else {
          error = iisUtil_ERROR(CDI, "Output Buffer too small");
        }
      } else {
        *pOutBits = 0;
      }
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer: pOutBits");
    }
  }

  if (error == noError) {
    if (hMp4SpaceEnc->nDiscardOutFrames == 0) {
      if (pOutputBuffer != NULL) {
        copyCHAR((char *)hMp4SpaceEnc->ppBitstreamDelayBuffer[hMp4SpaceEnc->nBitstreamBufferRead],
                 (char *)pOutputBuffer,
                 (hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferRead] + 7) / 8);

      } else {
        error = iisUtil_ERROR(CDI, "Invalid handle");
      }
    }
  }

  if (error == noError) {
    hMp4SpaceEnc->nBitstreamBufferRead = (hMp4SpaceEnc->nBitstreamBufferRead + 1) % hMp4SpaceEnc->nBitstreamDelayBuffer;
    hMp4SpaceEnc->nBitstreamBufferWrite = (hMp4SpaceEnc->nBitstreamBufferWrite + 1) % hMp4SpaceEnc->nBitstreamDelayBuffer;
  }

  if (noError == error) {
    if (NULL != pSamplesNext) {
      *pSamplesNext = hMp4SpaceEnc->nSamplesNext;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (noError == error) {
    nOutputSamples = (hMp4SpaceEnc->nDiscardOutFrames == 0) ? (nOutputSamplesMax) : 0;
    if (nOutputSamples > nOutputSamplesBufferSize) {
      error = iisUtil_ERROR(CDI, "Output Sample Buffer too small");
    }
  }

  if (noError == error) {
    if (NULL != pnOutputSamples) {
      *pnOutputSamples = nOutputSamples;
    } else {
      error = iisUtil_ERROR(CDI, "Invalid pointer: pnOutputSamples");
    }
  }

  if ((error == noError) && (hMp4SpaceEnc->bQmfOutput)) {
    for (ch = 0; ch < nChOut; ch++) {
      for (ts = 0; ts < nFrameTimeSlots; ts++) {
        copyFLOAT(hMp4SpaceEnc->pppQmfSigOutReal[ch][ts],
                  ppQmfOutputSamplesReal[ts * nChOut + ch],
                  hMp4SpaceEnc->nQmfBands);
        copyFLOAT(hMp4SpaceEnc->pppQmfSigOutImag[ch][ts],
                  ppQmfOutputSamplesImag[ts * nChOut + ch],
                  hMp4SpaceEnc->nQmfBands);
      }
    }

    for (ch = 0; ch < nChOut; ch++) {
      for (ts = 0; ts < hMp4SpaceEnc->nOutputBufferDelay / hMp4SpaceEnc->nQmfBands; ts++) {
        copyFLOAT(hMp4SpaceEnc->pppQmfSigOutReal[ch][ts + nFrameTimeSlots],
                  hMp4SpaceEnc->pppQmfSigOutReal[ch][ts],
                  hMp4SpaceEnc->nQmfBands);
        copyFLOAT(hMp4SpaceEnc->pppQmfSigOutImag[ch][ts + nFrameTimeSlots],
                  hMp4SpaceEnc->pppQmfSigOutImag[ch][ts],
                  hMp4SpaceEnc->nQmfBands);
      }
    }
  }

  {
    if (error == noError) {
      for (ch = 0; ch < nChOut; ch++) {
        copyFLOATflex(hMp4SpaceEnc->ppTimeSigOut[ch],
                      1,
                      &hMp4SpaceEnc->pOutputDelayBuffer[ch + (hMp4SpaceEnc->nOutputBufferDelay) * nChOut],
                      nChOut,
                      nOutputSamplesMax / nChOut);
      }
    }

    if (error == noError) {
      int bUsacDiscardThisFrame = 0;
      if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
        if (hMp4SpaceEnc->nDiscardOutFrames == 0) {
          bUsacDiscardThisFrame = 0;
        } else {
          bUsacDiscardThisFrame = 1;
        }
      } else {
        bUsacDiscardThisFrame = 0;
      }

      if (bUsacDiscardThisFrame == 0) {
        if (pOutputSamples != NULL) {
          copyFLOAT(hMp4SpaceEnc->pOutputDelayBuffer, pOutputSamples, nOutputSamplesMax);

          error = StaticPostGain_Apply(hMp4SpaceEnc->hStaticGain, pOutputSamples, nOutputSamplesMax);

        } else {
          error = iisUtil_ERROR(CDI, "Invalid pointer to pOutputSamples");
        }
      }
    }

    if (error == noError) {
      moveFLOAT(&hMp4SpaceEnc->pOutputDelayBuffer[nOutputSamplesMax], hMp4SpaceEnc->pOutputDelayBuffer, nChOut * (hMp4SpaceEnc->nOutputBufferDelay));
    }
  }

  if (error == noError && hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
    if (hMp4SpaceEnc->nDiscardOutFrames <= (hMp4SpaceEnc->nBitstreamDelayBuffer - 1)) {
      if (hMp4SpaceEnc->independencyFactor > 0) {
        hMp4SpaceEnc->independencyCount = (hMp4SpaceEnc->independencyCount + 1) % hMp4SpaceEnc->independencyFactor;
      } else {
        hMp4SpaceEnc->independencyCount = -1;
      }

      hMp4SpaceEnc->forceIndependency = 0;
    }
  }

  if (hMp4SpaceEnc->nDiscardOutFrames > 0) {
    hMp4SpaceEnc->nDiscardOutFrames--;
  }

  if (noError == error) {
    hMp4SpaceEnc->nSamplesValid = 0;
  }

  hMp4SpaceEnc->frameCount++;

  return error;
}

int mp4SpaceEnc_GetBsFixedGainDmx(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc) {
  return GetSpatialSpecificConfig(hMp4SpaceEnc->hBitstreamFormatter)->bsFixedGainDMX;
}

static HANDLE_ERROR_INFO mapBandConfigs(BOX_SUBBAND_CONFIG *const outBandConfig,
                                        MP4SPACEENC_BANDS_CONFIG const inBandConfig) {
  HANDLE_ERROR_INFO error = noError;

  if (outBandConfig == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  switch (inBandConfig) {
    case MP4SPACEENC_BANDS_4:
      *outBandConfig = BOX_SUBBANDS_4;
      break;
    case MP4SPACEENC_BANDS_5:
      *outBandConfig = BOX_SUBBANDS_5;
      break;
    case MP4SPACEENC_BANDS_7:
      *outBandConfig = BOX_SUBBANDS_7;
      break;
    case MP4SPACEENC_BANDS_9:
      *outBandConfig = BOX_SUBBANDS_9;
      break;
    case MP4SPACEENC_BANDS_10:
      *outBandConfig = BOX_SUBBANDS_10;
      break;
    case MP4SPACEENC_BANDS_12:
      *outBandConfig = BOX_SUBBANDS_12;
      break;
    case MP4SPACEENC_BANDS_14:
      *outBandConfig = BOX_SUBBANDS_14;
      break;
    case MP4SPACEENC_BANDS_15:
      *outBandConfig = BOX_SUBBANDS_15;
      break;
    case MP4SPACEENC_BANDS_20:
      *outBandConfig = BOX_SUBBANDS_20;
      break;
    case MP4SPACEENC_BANDS_23:
      *outBandConfig = BOX_SUBBANDS_23;
      break;
    case MP4SPACEENC_BANDS_28:
      *outBandConfig = BOX_SUBBANDS_28;
      break;
    case MP4SPACEENC_BANDS_INVALID:
    default:
      *outBandConfig = BOX_SUBBANDS_INVALID;
      break;
  }

  return error;
}

static void mp4SpaceEnc_rotateDmxResToPseudoLR(float *hRe,
                                               float *hIm) {
  float const cos_pi_4 = 1.0f / (float)sqrt(2.0f);

  for (int r = 0; r < 2; ++r) {
    const float hr0 = hRe[2 * r + 0], hr1 = hRe[2 * r + 1];
    const float hi0 = hIm[2 * r + 0], hi1 = hIm[2 * r + 1];

    hRe[2 * r + 0] = (hr0 + hr1) * cos_pi_4;
    hRe[2 * r + 1] = (hr0 - hr1) * cos_pi_4;

    hIm[2 * r + 0] = (hi0 + hi1) * cos_pi_4;
    hIm[2 * r + 1] = (hi0 - hi1) * cos_pi_4;
  }
}

static void mp4SpaceEnc_MoveBufUmxParamToFront(int ts,
                                               int delayUmxMat2Mdct,
                                               int numParamBands,
                                               float cld[][28],
                                               float umxMatRe[][28][4],
                                               float umxMatIm[][28][4],
                                               int nTimeSlots) {
  int paramBand = 0, inCh = 0, outCh = 0;
  for (ts = 0; ts < delayUmxMat2Mdct; ts++) {
    for (paramBand = 0; paramBand < numParamBands; paramBand++) {
      for (inCh = 0; inCh < 2; inCh++) {
        for (outCh = 0; outCh < 2; outCh++) {
          umxMatRe[ts][paramBand][2 * outCh + inCh] = umxMatRe[nTimeSlots + ts][paramBand][2 * outCh + inCh];
          umxMatIm[ts][paramBand][2 * outCh + inCh] = umxMatIm[nTimeSlots + ts][paramBand][2 * outCh + inCh];
        }
      }
      cld[ts][paramBand] = cld[nTimeSlots + ts][paramBand];
    }
  }
}

static void mp4SpaceEnc_AddBufUmxParamToEnd(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
                                            int ts,
                                            int delayUmxMat2Mdct,
                                            float cld[][28],
                                            float umxMatRe[][28][4],
                                            float umxMatIm[][28][4],
                                            int nTimeSlots,
                                            int const subband2parameterBand[],
                                            int bPseudoLr) {
  int paramBand = 0, inCh = 0, outCh = 0, hybBand = 0;

  for (ts = 0; ts < nTimeSlots; ts++) {
    for (hybBand = 0; hybBand < (int)hMp4SpaceEnc->nHybridBands; hybBand++) {
      paramBand = subband2parameterBand[hybBand];
      assert(paramBand < (int)hMp4SpaceEnc->nParamBands);
      for (inCh = 0; inCh < 2; inCh++) {
        for (outCh = 0; outCh < 2; outCh++) {
          umxMatRe[delayUmxMat2Mdct + ts][paramBand][2 * outCh + inCh] = hMp4SpaceEnc->ppUmxMatReal[outCh][inCh][ts][hybBand];
          umxMatIm[delayUmxMat2Mdct + ts][paramBand][2 * outCh + inCh] = hMp4SpaceEnc->ppUmxMatImag[outCh][inCh][ts][hybBand];

          staticGain_ApplyInverseDmxGain(hMp4SpaceEnc->hStaticGain, &(umxMatRe[delayUmxMat2Mdct + ts][paramBand][2 * outCh + inCh]), 1);
          staticGain_ApplyInverseDmxGain(hMp4SpaceEnc->hStaticGain, &(umxMatIm[delayUmxMat2Mdct + ts][paramBand][2 * outCh + inCh]), 1);
        }
      }

      if (paramBand >= (int)(hMp4SpaceEnc->residualConfig.bands[0] + 1)) {
        umxMatRe[delayUmxMat2Mdct + ts][paramBand][1] = umxMatIm[delayUmxMat2Mdct + ts][paramBand][1] = 0.0f;
        umxMatRe[delayUmxMat2Mdct + ts][paramBand][3] = umxMatIm[delayUmxMat2Mdct + ts][paramBand][3] = 0.0f;
      }

      if (bPseudoLr && hMp4SpaceEnc->nOutputChannels == 2) {
        mp4SpaceEnc_rotateDmxResToPseudoLR(umxMatRe[delayUmxMat2Mdct + ts][paramBand], umxMatIm[delayUmxMat2Mdct + ts][paramBand]);
      }

      cld[delayUmxMat2Mdct + ts][paramBand] = SpaceTree_GetUniSteCld(hMp4SpaceEnc->hSpaceTree, paramBand);
    }
  }
}

HANDLE_ERROR_INFO
mp4SpaceEnc_UniSteUpdateFrame(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
                              float umxMatRe[][28][4],
                              float umxMatIm[][28][4],
                              float cld[][28],
                              int delayUmxMat2Mdct,
                              int bPseudoLr) {
  HANDLE_ERROR_INFO error = noError;

  int ts = 0;
  MPS_MODE mode = MPS_MODE_USAC;
  const int *subband2parameterBand = NULL;
  BOX_SUBBAND_CONFIG nParamBands;

  int nTimeSlots = hMp4SpaceEnc->nFrameTimeSlots;

  if (hMp4SpaceEnc->residualConfig.mode > 0) {
    mode = MPS_MODE_USAC_UNISTE;
  }

  error = mapBandConfigs(&nParamBands, hMp4SpaceEnc->nParamBands);
  if (error == noError) {
    subband2parameterBand = getSubband2ParameterIndex(nParamBands, mode);

    mp4SpaceEnc_MoveBufUmxParamToFront(ts, delayUmxMat2Mdct, (int)hMp4SpaceEnc->nParamBands, cld, umxMatRe, umxMatIm, nTimeSlots);

    mp4SpaceEnc_AddBufUmxParamToEnd(hMp4SpaceEnc, ts, delayUmxMat2Mdct, cld, umxMatRe, umxMatIm, nTimeSlots, subband2parameterBand, bPseudoLr);
  }
  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_Close(HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc) {
  HANDLE_ERROR_INFO error = noError;
  unsigned int ch = 0;
  int i;

  if (NULL != phMp4SpaceEnc) {
    if (NULL != *phMp4SpaceEnc) {
      HANDLE_MP4SPACE_ENCODER const hEnc = *phMp4SpaceEnc;

      if (hEnc->phQmfFiltIn != NULL) {
        for (ch = 0; ch < hEnc->nInputChannels; ch++) {
          if (NULL != hEnc->phQmfFiltIn[ch]) {
            QMFlib_DestroyAnalysis(&hEnc->phQmfFiltIn[ch]);
          }
        }
        iisFree(hEnc->phQmfFiltIn);
      }
      hEnc->phQmfFiltIn = NULL;

      if (hEnc->phHybFiltIn != NULL) {
        for (ch = 0; ch < hEnc->nInputChannels; ch++) {
          if (NULL != hEnc->phHybFiltIn[ch]) {
            QMFlib_LowFrequencyFilterDestroyAnalysis(&hEnc->phHybFiltIn[ch]);
          }
        }
        iisFree(hEnc->phHybFiltIn);
      }
      hEnc->phHybFiltIn = NULL;

      if (hEnc->phDCFilterSigIn != NULL) {
        for (ch = 0; ch < hEnc->nInputChannels; ch++) {
          if (NULL != hEnc->phDCFilterSigIn[ch]) {
            DestroyDCFilter(&hEnc->phDCFilterSigIn[ch]);
          }
        }
        iisFree(hEnc->phDCFilterSigIn);
        hEnc->phDCFilterSigIn = NULL;
      }

      if (hEnc->phQmfFiltOut != NULL) {
        for (ch = 0; ch < hEnc->nOutputChannels; ch++) {
          if (NULL != hEnc->phQmfFiltOut[ch]) {
            QMFlib_DestroySynthesis(&hEnc->phQmfFiltOut[ch]);
          }
        }
        iisFree(hEnc->phQmfFiltOut);
      }
      hEnc->phQmfFiltOut = NULL;

      if (hEnc->phHybFiltOut != NULL) {
        for (ch = 0; ch < hEnc->nTotalOutputChannels; ch++) {
          if (NULL != hEnc->phHybFiltOut[ch]) {
            QMFlib_LowFrequencyFilterDestroySynthesis(&hEnc->phHybFiltOut[ch]);
          }
        }
        iisFree(hEnc->phHybFiltOut);
      }
      hEnc->phHybFiltOut = NULL;

      if (hEnc->phOnset != NULL) {
        for (ch = 0; ch < hEnc->nInputChannels; ch++) {
          if (NULL != hEnc->phOnset[ch]) {
            error = OnsetDetect_Close(&hEnc->phOnset[ch]);
            if (noError != error) {
              error = handBack(error);
            }
          }
        }
        iisFree(hEnc->phOnset);
      }
      hEnc->phOnset = NULL;

      if (hEnc->ppTrCurrPos) {
        iisFreeMatrix2D((void **)hEnc->ppTrCurrPos);
      }
      hEnc->ppTrCurrPos = NULL;

      if (hEnc->hFrameWindow) {
        FrameWindow_Destroy(&hEnc->hFrameWindow);
      }

      if (NULL != hEnc->hSpaceTree) {
        error = SpaceTree_Close(&hEnc->hSpaceTree);
        if (noError != error) {
          error = handBack(error);
        }
      }

      if (NULL != hEnc->hSpatialTonality) {
        DestroySpatialTonality(&hEnc->hSpatialTonality);
      }

      if (NULL != hEnc->hStaticGain) {
        error = StaticGain_Close(&hEnc->hStaticGain);
        if (noError != error) {
          error = handBack(error);
        }
      }
      if (NULL != hEnc->hStaticGainConfig) {
        error = StaticGain_CloseConfig(&hEnc->hStaticGainConfig);
        if (noError != error) {
          error = handBack(error);
        }
      }

      if (NULL != hEnc->hDelay) {
        error = Delay_Close(&hEnc->hDelay);
        if (noError != error) {
          error = handBack(error);
        }
      }
      if (NULL != hEnc->hDelayConfig) {
        error = Delay_CloseConfig(&hEnc->hDelayConfig);
        if (noError != error) {
          error = handBack(error);
        }
      }

      if (NULL != hEnc->hBitBuf) {
        DeleteBitBuffer(hEnc->hBitBuf);
      }
      hEnc->hBitBuf = NULL;

      if (NULL != hEnc->hBitstreamFormatter) {
        error = DestroySpatialBitstreamEncoder(&(hEnc->hBitstreamFormatter));
        if (noError != error) {
          error = handBack(error);
        }
      }

      if (hEnc->pInputBuffer != NULL) {
        iisFree((void *)hEnc->pInputBuffer);
        hEnc->pInputBuffer = NULL;
      }

      if (hEnc->pppQmfInReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfInReal);
        hEnc->pppQmfInReal = NULL;
      }

      if (hEnc->pppQmfInImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfInImag);
        hEnc->pppQmfInImag = NULL;
      }

      if (hEnc->pppHybridInReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppHybridInReal);
        hEnc->pppHybridInReal = NULL;
      }

      if (hEnc->pppHybridInImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppHybridInImag);
        hEnc->pppHybridInImag = NULL;
      }

      if (hEnc->pppResidualDiffSigReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResidualDiffSigReal);
        hEnc->pppResidualDiffSigReal = NULL;
      }

      if (hEnc->pppResidualDiffSigImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResidualDiffSigImag);
        hEnc->pppResidualDiffSigImag = NULL;
      }

      if (hEnc->pppResDiffQmfReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResDiffQmfReal);
        hEnc->pppResDiffQmfReal = NULL;
      }

      if (hEnc->pppResDiffQmfImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResDiffQmfImag);
        hEnc->pppResDiffQmfImag = NULL;
      }

      if (hEnc->pppResDiffProcReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResDiffProcReal);
        hEnc->pppResDiffProcReal = NULL;
      }

      if (hEnc->pppResDiffProcImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppResDiffProcImag);
        hEnc->pppResDiffProcImag = NULL;
      }

      if (hEnc->pppProcDataInReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppProcDataInReal);
        hEnc->pppProcDataInReal = NULL;
      }

      if (hEnc->pppProcDataInImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppProcDataInImag);
        hEnc->pppProcDataInImag = NULL;
      }

      if (hEnc->pppQmfOutReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfOutReal);
        hEnc->pppQmfOutReal = NULL;
      }

      if (hEnc->pppQmfOutImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfOutImag);
        hEnc->pppQmfOutImag = NULL;
      }

      if (hEnc->pOutputDelayBuffer != NULL) {
        iisFree(hEnc->pOutputDelayBuffer);
        hEnc->pOutputDelayBuffer = NULL;
      }

      if (hEnc->pnOutputBits != NULL) {
        iisFree(hEnc->pnOutputBits);
        hEnc->pnOutputBits = NULL;
      }

      if (hEnc->pppHybridOutReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppHybridOutReal);
        hEnc->pppHybridOutReal = NULL;
      }

      if (hEnc->pppHybridOutImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppHybridOutImag);
        hEnc->pppHybridOutImag = NULL;
      }

      if (hEnc->pppProcDataOutReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppProcDataOutReal);
        hEnc->pppProcDataOutReal = NULL;
      }

      if (hEnc->pppProcDataOutImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppProcDataOutImag);
        hEnc->pppProcDataOutImag = NULL;
      }

      {
        int j;

        for (i = 0; i < 2; i++) {
          for (j = 0; j < 2; j++) {
            if (hEnc->ppUmxMatReal[i][j] != NULL) {
              iisFreeMatrix2D((void **)hEnc->ppUmxMatReal[i][j]);
            }

            if (hEnc->ppUmxMatImag[i][j] != NULL) {
              iisFreeMatrix2D((void **)hEnc->ppUmxMatImag[i][j]);
            }
          }
        }
      }

      if (hEnc->ppBitstreamDelayBuffer != NULL) {
        iisFreeMatrix2D((void **)hEnc->ppBitstreamDelayBuffer);
        hEnc->ppBitstreamDelayBuffer = NULL;
      }

      if (hEnc->ppTimeSigIn != NULL) {
        iisFreeMatrix2D((void **)hEnc->ppTimeSigIn);
        hEnc->ppTimeSigIn = NULL;
      }

      if (hEnc->pppQmfSigOutReal != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfSigOutReal);
        hEnc->pppQmfSigOutReal = NULL;
      }

      if (hEnc->pppQmfSigOutImag != NULL) {
        iisFreeMatrix3D((void ***)hEnc->pppQmfSigOutImag);
        hEnc->pppQmfSigOutImag = NULL;
      }

      if (hEnc->ppTimeSigOut != NULL) {
        iisFreeMatrix2D((void **)hEnc->ppTimeSigOut);
        hEnc->ppTimeSigOut = NULL;
      }

      if (hEnc->ppTmpRemapBuffer != NULL) {
        iisFreeMatrix2D((void **)hEnc->ppTmpRemapBuffer);
        hEnc->ppTmpRemapBuffer = NULL;
      }

      for (i = 0; i < MAX_NUM_PARAMS; i++) {
        if (hEnc->pFrameWindowAna[i] != NULL) {
          iisFree(hEnc->pFrameWindowAna[i]);
        }
        hEnc->pFrameWindowAna[i] = NULL;
      }

      for (i = 0; i < MAX_NUM_PARAMS; i++) {
        if (hEnc->pFrameWindowSyn[i] != NULL) {
          iisFree(hEnc->pFrameWindowSyn[i]);
        }
        hEnc->pFrameWindowSyn[i] = NULL;
      }

      if (hEnc->sscBuf.pSsc != NULL) {
        iisFree(hEnc->sscBuf.pSsc);
      }
      hEnc->sscBuf.pSsc = NULL;

      for (ch = 0; ch < hEnc->nInputChannels; ch++) {
        APPLDET_C_IFC_Close(&((*phMp4SpaceEnc)->hApplDet[ch]));
      }
      iisFree((*phMp4SpaceEnc)->hApplDet);

      iisFree(*phMp4SpaceEnc);
      *phMp4SpaceEnc = NULL;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_GetInfo(
    HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
    MP4SPACEENC_INFO *const pInfo) {
  HANDLE_ERROR_INFO error = noError;

  if (noError == error) {
    if (NULL == hMp4SpaceEnc) {
      error = iisUtil_ERROR(CDI, "Invalid handle.");
    }
  }

  if (noError == error) {
    if (NULL == pInfo) {
      error = iisUtil_ERROR(CDI, "Invalid Info Handle");
    }
  }

  if (error == noError) {
    pInfo->nSampleRate = hMp4SpaceEnc->nSampleRate;
    pInfo->nSamplesFrame = hMp4SpaceEnc->nFrameLength;
    pInfo->nDmxDelay = *(Delay_GetInfoDmxDelayPtr(hMp4SpaceEnc->hDelay));
    pInfo->nCodecDelay = *(Delay_GetInfoCodecDelayPtr(hMp4SpaceEnc->hDelay));
    pInfo->nPayloadDelay = (*(Delay_GetBitstreamFrameBufferSizePtr(hMp4SpaceEnc->hDelay))) - 1;
    pInfo->pSscBuf = &hMp4SpaceEnc->sscBuf;
  }

  if (noError == error) {
    strcpy(pInfo->pVersion, MP4SPACEENC_LIBRARY_VERSION);
  }

  if (noError == error) {
    if (NULL == hMp4SpaceEnc) {
      error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
    }
  }

  return error;
}

int mp4SpaceEnc_GetNumFramesBitstreamDelay(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc) {
  int nFramesBitstreamDelay = -1;

  if (hMp4SpaceEnc) {
    nFramesBitstreamDelay = hMp4SpaceEnc->nBitstreamDelayBuffer;
  }

  return nFramesBitstreamDelay;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_GetNumBitsPayloadThisFrame(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc, int *bitsNextPayload) {
  HANDLE_ERROR_INFO error = noError;
  if (error == noError) {
    if (hMp4SpaceEnc == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid Info Handle");
    }
  }
  if (error == noError) {
    if (bitsNextPayload == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid Parameter");
    }
  }
  if (error == noError) {
    if (hMp4SpaceEnc->nBitstreamDelayBuffer > 1) {
      *bitsNextPayload = hMp4SpaceEnc->pnOutputBits[hMp4SpaceEnc->nBitstreamBufferRead];
    } else {
      *bitsNextPayload = 0;
      assert(0);
    }
  }

  if (error != noError) {
    if (bitsNextPayload) {
      *bitsNextPayload = 0;
    }
  }

  return error;
}

float mp4SpaceEnc_ParamBand2Freq(int nParamBands, int nSampleRate, int nParamBand, int nQmfBands) {
  return SpaceTree_ParamBand2Freq(nParamBands, nSampleRate, nParamBand, nQmfBands);
}

HANDLE_ERROR_INFO mp4SpaceEnc_InitDelayCompensation(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, const int coreCoderDelay) {
  HANDLE_ERROR_INFO error = noError;
  int timeDomDownmix = 0;

  if (hMp4SpaceEnc != NULL) {
    if (error == noError) {
      hMp4SpaceEnc->coreCoderDelay = coreCoderDelay;
    }

    if (error == noError) {
      if (hMp4SpaceEnc->pOutputDelayBuffer != NULL) {
        iisFree(hMp4SpaceEnc->pOutputDelayBuffer);
      }
      hMp4SpaceEnc->pOutputDelayBuffer = NULL;

      if (hMp4SpaceEnc->pnOutputBits != NULL) {
        iisFree(hMp4SpaceEnc->pnOutputBits);
      }
      hMp4SpaceEnc->pnOutputBits = NULL;

      if (hMp4SpaceEnc->ppBitstreamDelayBuffer != NULL) {
        iisFreeMatrix2D((void **)hMp4SpaceEnc->ppBitstreamDelayBuffer);
        hMp4SpaceEnc->ppBitstreamDelayBuffer = NULL;
      }
    }
    if (error == noError) {
      int bUsac212 = 0;
      if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
        bUsac212 = 1;
      }
      error = Delay_OpenConfig(&hMp4SpaceEnc->hDelayConfig,
                               NUM_QMF_BANDS,
                               hMp4SpaceEnc->nFrameLength,
                               RESIDUAL_CODING_DELAY,
                               coreCoderDelay,
                               hMp4SpaceEnc->timeAlignment,
                               hMp4SpaceEnc->useLowDelay,
                               bUsac212);
    }
    if (error == noError) {
      error = Delay_SetDmxAlign(hMp4SpaceEnc->hDelayConfig, hMp4SpaceEnc->bDMXAlign);
    }

    if (error == noError) {
      error = Delay_SetTimeDomDmx(hMp4SpaceEnc->hDelayConfig, timeDomDownmix);
    }
    if (error == noError) {
      error = Delay_SetMpsResidualCoding(hMp4SpaceEnc->hDelayConfig, (hMp4SpaceEnc->residualConfig.mode > 0));
    }

    if (error == noError) {
      error = Delay_Open(&hMp4SpaceEnc->hDelay, hMp4SpaceEnc->hDelayConfig);
    }

    if (error == noError) {
      hMp4SpaceEnc->nBitstreamDelayBuffer = *Delay_GetBitstreamFrameBufferSizePtr(hMp4SpaceEnc->hDelay);
      hMp4SpaceEnc->nOutputBufferDelay = *Delay_GetOutputAudioBufferDelayPtr(hMp4SpaceEnc->hDelay);
      hMp4SpaceEnc->nSurroundAnalysisBufferDelay = *Delay_GetSurroundAnalysisBufferDelayPtr(hMp4SpaceEnc->hDelay);
      hMp4SpaceEnc->nBitstreamBufferRead = 0;
      hMp4SpaceEnc->nBitstreamBufferWrite = hMp4SpaceEnc->nBitstreamDelayBuffer - 1;

      hMp4SpaceEnc->nDiscardOutFrames = *Delay_GetDiscardOutFramesPtr(hMp4SpaceEnc->hDelay);
      hMp4SpaceEnc->nInputDelay = *Delay_GetDmxAlignBufferDelayPtr(hMp4SpaceEnc->hDelay);
    }

    if (error == noError) {
      if (NULL == (hMp4SpaceEnc->ppBitstreamDelayBuffer = (unsigned char **)iisCallocMatrix2D(hMp4SpaceEnc->nBitstreamDelayBuffer, MAX_MPEGS_BITS / 8, sizeof(char)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation error.");
      }
    }

    if (error == noError) {
      if (NULL == (hMp4SpaceEnc->pnOutputBits = (int *)iisCalloc(1, hMp4SpaceEnc->nBitstreamDelayBuffer * sizeof(int)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation error.");
      }
    }

    hMp4SpaceEnc->independencyCount = 0;
    hMp4SpaceEnc->independencyFlag = 1;

    if (error == noError) {
      int i = 0;
      SPATIALFRAME *pFrameData = GetSpatialFrame(hMp4SpaceEnc->hBitstreamFormatter, 0);

      int bsIndependencyFlagUsac = 0;
      pFrameData->bsIndependencyFlag = 1;
      pFrameData->framingInfo.numParamSets = 1;
      pFrameData->framingInfo.bsFramingType = 0;

      if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
        pFrameData->framingInfo.bsParamSlots[0] = hMp4SpaceEnc->nFrameTimeSlots - 1;
        bsIndependencyFlagUsac = 1;
      }

      for (i = 0; i < hMp4SpaceEnc->nBitstreamDelayBuffer - 1; i++) {
        WriteSpatialFrame(hMp4SpaceEnc->hBitBuf, hMp4SpaceEnc->hBitstreamFormatter, bsIndependencyFlagUsac);
        hMp4SpaceEnc->pnOutputBits[i] = GetBitsAvail(hMp4SpaceEnc->hBitBuf);
        ByteAlign(hMp4SpaceEnc->hBitBuf);
        ReadBytes(hMp4SpaceEnc->hBitBuf, hMp4SpaceEnc->ppBitstreamDelayBuffer[i], (hMp4SpaceEnc->pnOutputBits[i] + 7) / 8);
      }
    }

    hMp4SpaceEnc->nEncoderDelay = *Delay_GetInfoDmxDelayPtr(hMp4SpaceEnc->hDelay);
    if (hMp4SpaceEnc->encMode == MP4SPACEENC_USAC_212) {
      if (hMp4SpaceEnc->bDMXAlign) {
        hMp4SpaceEnc->nDiscardOutFrames = hMp4SpaceEnc->nEncoderDelay / hMp4SpaceEnc->nFrameLength + 1;
        hMp4SpaceEnc->nInputDelay = hMp4SpaceEnc->nFrameLength * hMp4SpaceEnc->nDiscardOutFrames - hMp4SpaceEnc->nEncoderDelay;
      } else {
        hMp4SpaceEnc->nDiscardOutFrames = 0;
        hMp4SpaceEnc->nInputDelay = 0;
      }
    }

    if (hMp4SpaceEnc->ppTimeSigIn != NULL) {
      iisFreeMatrix2D((void **)hMp4SpaceEnc->ppTimeSigIn);
      hMp4SpaceEnc->ppTimeSigIn = NULL;
    }

    if (hMp4SpaceEnc->ppTmpRemapBuffer) {
      iisFreeMatrix2D((void **)hMp4SpaceEnc->ppTmpRemapBuffer);
      hMp4SpaceEnc->ppTmpRemapBuffer = NULL;
    }

    if (error == noError) {
      if (NULL == (hMp4SpaceEnc->pOutputDelayBuffer = (float *)iisCalloc(1, (hMp4SpaceEnc->nFrameLength + hMp4SpaceEnc->nOutputBufferDelay) * hMp4SpaceEnc->nOutputChannels * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation error.");
      }
    }

    if (noError == error) {
      if (NULL == (hMp4SpaceEnc->ppTimeSigIn = (float **)iisCallocMatrix2D(hMp4SpaceEnc->nInputChannels, (hMp4SpaceEnc->nFrameLength + hMp4SpaceEnc->nSurroundAnalysisBufferDelay), sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }

    if (noError == error) {
      const int nBufferedChannels = 2;
      if (NULL == (hMp4SpaceEnc->ppTmpRemapBuffer = (float **)iisCallocMatrix2D(nBufferedChannels, hMp4SpaceEnc->nFrameLength + hMp4SpaceEnc->nSurroundAnalysisBufferDelay, sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }

    if (hMp4SpaceEnc->bDMXAlign > 0) {
      if (NULL == (hMp4SpaceEnc->pInputBuffer = (float *)iisCalloc((hMp4SpaceEnc->nInputChannels) * (hMp4SpaceEnc->nFrameLength + hMp4SpaceEnc->nInputDelay), sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
  }

  return error;
}

static HANDLE_ERROR_INFO
mapTempShapeConfig(TEMPSHAPECONFIG *const outTempShapeConfig,
                   MP4SPACEENC_TEMPSHAPECONFIG const inTempShapeConfig) {
  HANDLE_ERROR_INFO error = noError;

  if (outTempShapeConfig == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (error == noError) {
    switch (inTempShapeConfig) {
      case MP4SPACEENC_TEMPSHAPE_OFF:
        *outTempShapeConfig = TEMPSHAPE_OFF;
        break;
      case MP4SPACEENC_TEMPSHAPE_STP:
        *outTempShapeConfig = TEMPSHAPE_STP;
        break;
      case MP4SPACEENC_TEMPSHAPE_GES:
        *outTempShapeConfig = TEMPSHAPE_GES;
        break;
      case MP4SPACEENC_TEMPSHAPE_TSD:
        *outTempShapeConfig = TEMPSHAPE_TSD;
        break;
      case MP4SPACEENC_TEMPSHAPE_INVALID:
      default:
        *outTempShapeConfig = TEMPSHAPE_INVALID;
        break;
    }
  }
  return error;
}

static HANDLE_ERROR_INFO
__CreateSpatialSpecificConfig(HANDLE_MP4SPACE_ENCODER const hEnc) {
  HANDLE_ERROR_INFO error = noError;
  SPATIALSPECIFICCONFIG *hSsc = NULL;
  SPACE_TREE_DESCRIPTION spaceTreeDescription = {0};

  if (NULL == hEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }

  if (noError == error) {
    hSsc = GetSpatialSpecificConfig(hEnc->hBitstreamFormatter);
    if (NULL == hSsc) {
      error = iisUtil_ERROR(CDI, "GetSpatialSpecificConfig failed.");
    }
  }

  if (noError == error) {
    memset(hSsc, 0, sizeof(SPATIALSPECIFICCONFIG));
  }

  if (error == noError) {
    hSsc->bLowDelay = hEnc->useLowDelay;
  }

  SAFECALL(error, SpaceTree_GetDescription(hEnc->hSpaceTree, &spaceTreeDescription));

  if (noError == error) {
    int i;

    hSsc->numBands = hEnc->spaceTreeSetup.nParamBands;

    hSsc->bsHighRateMode = hEnc->bsHighRateMode;

    hSsc->bsTreeConfig = TREE_USAC_212;
    hSsc->bsHighRateMode = hEnc->bsHighRateMode;
    hSsc->treeDescription.numOttBoxes = spaceTreeDescription.nOttBoxes;
    hSsc->treeDescription.numTttBoxes = spaceTreeDescription.nTttBoxes;
    hSsc->treeDescription.numInChan = spaceTreeDescription.nInChannels;
    hSsc->treeDescription.numOutChan = spaceTreeDescription.nOutChannels;
    for (i = 0; i < MAX_NUM_BOXES; i++) {
      hSsc->treeDescription.ottModeLfe[i] = spaceTreeDescription.bOttModeLfe[i];
      {
        hSsc->ottConfig[i].bsOttBands = hSsc->numBands;
      }
    }

    hSsc->bsOttBandsPhase = hEnc->spaceTreeSetup.nOttBandsPhase;
    hSsc->bsOttBandsPhasePresent = hEnc->spaceTreeSetup.bOttBandsPhasePresent;
  }

  if (error == noError) {
    hSsc->bsSamplingFrequency = hEnc->nSampleRate;
    hSsc->bsFrameLength = hEnc->nFrameTimeSlots - 1;
    hSsc->bsOneIcc = hEnc->spaceTreeSetup.bOneIcc;

    hSsc->bsIpdMode = hEnc->spaceTreeSetup.ipdMode;
    hSsc->bsOpdSmoothing = hEnc->useOpdSmoothing;

    hSsc->bsDecorrConfig = hEnc->decorrConfig;

    switch (hEnc->quantMode) {
      case MP4SPACEENC_QUANTMODE_FINE:
        hSsc->bsQuantMode = QUANTMODE_FINE;
        break;
      case MP4SPACEENC_QUANTMODE_RSVD3:
      case MP4SPACEENC_QUANTMODE_INVALID:
      default:
        error = iisUtil_ERROR(CDI, "Invalid quantmode.");
        break;
    }
  }

  if (error == noError) {
    hSsc->bsFixedGainDMX = staticGain_GetDmxGain(hEnc->hStaticGain);
    hSsc->bsMatrixMode = hEnc->bCalcDPL;
    hSsc->bsEnvQuantMode = 0;
    hSsc->bs3DaudioMode = 0;
    error = mapTempShapeConfig(&hSsc->bsTempShapeConfig, hEnc->tempShapeConfig);
  }

  if (error == noError) {
    int i;
    hSsc->bsResidualCoding = (hEnc->residualConfig.mode != MP4SPACEENC_RES_MODE_NONE) ? 1 : 0;
    if (hSsc->bsResidualCoding) {
      hSsc->bsResidualSamplingFrequency = GetResidualSampleRate(hEnc->nSampleRate,
                                                                hEnc->nFrameTimeSlots,
                                                                hEnc->residualConfig.framesPerSpatial,
                                                                hEnc->encMode);
      hSsc->bsResidualFramesPerSpatialFrame = hEnc->residualConfig.framesPerSpatial - 1;

      for (i = 0; i < MAX_NUM_BOXES; i++) {
        hSsc->residualConfig[i].bsResidualPresent = hEnc->spaceTreeSetup.bCalcResiduals[i];
        if (hSsc->residualConfig[i].bsResidualPresent) {
          hSsc->residualConfig[i].bsResidualBands = hEnc->spaceTreeSetup.nResidualBands[i];
        }
      }

      hSsc->bsPseudoLr = hEnc->bPseudoLr;
    }
  }

  return error;
}

static void
__DecodeResidualMode(unsigned int mode,
                     int *pbCalcResiduals) {
  int box;

  for (box = 0; box < MAX_NUM_BOXES; box++) {
    pbCalcResiduals[box] = ((mode & 1) != 0);
    mode >>= 1;
  }
}

static HANDLE_ERROR_INFO
mp4SpaceEnc_SpaceTreeSetup(HANDLE_MP4SPACE_ENCODER const hEnc,
                           SPACE_TREE_SETUP *hSpaceTreeSetup) {
  HANDLE_ERROR_INFO error = noError;
  QUANTMODE tmpQuantmode = QUANTMODE_INVALID;

  int tmpCalcIccDiff = -1;
  DOWNMIXTYPE tmpDownmixType = DOWNMIXTYPE_INVALID;
  IPDMODE tmpIpdMode = IPDMODE_INVALID;

  if (noError == error) {
    if (NULL == hEnc || NULL == hSpaceTreeSetup) {
      error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
    }
  }

  if (noError == error) {
    switch (hEnc->quantMode) {
      case MP4SPACEENC_QUANTMODE_FINE:
        tmpQuantmode = QUANTMODE_FINE;
        break;
      case MP4SPACEENC_QUANTMODE_RSVD3:
      case MP4SPACEENC_QUANTMODE_INVALID:
      default:
        tmpQuantmode = QUANTMODE_INVALID;
        error = iisUtil_ERROR(CDI, "Invalid quantmode.");
        break;
    }
  }

  if (noError == error && hEnc->encMode == MP4SPACEENC_USAC_212) {
    if (hEnc->residualConfig.mode != MP4SPACEENC_RES_MODE_NONE) {
      switch (hEnc->residualConfig.codec) {
        case MP4SPACEENC_RES_CODEC_AAC:
          tmpCalcIccDiff = 1;
          break;
        case MP4SPACEENC_RES_CODEC_PCM:
          tmpCalcIccDiff = 0;
          break;
        default:
          error = iisUtil_ERROR(CDI, "Invalid residual codec");
          break;
      }
    } else {
      tmpCalcIccDiff = 0;
    }
    hSpaceTreeSetup->bStereoSbr = hEnc->bStereoSbr;
    hSpaceTreeSetup->bPsStyleDmx = hEnc->bPsStyleDmx;
  }

  if (noError == error && hEnc->encMode == MP4SPACEENC_USAC_212) {
    switch (hEnc->downmixType) {
      case MP4SPACEENC_DOWNMIXTYPE_CLASSICMPS:
        tmpDownmixType = DOWNMIXTYPE_CLASSICMPS;
        break;
      case MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL:
        tmpDownmixType = DOWNMIXTYPE_SIMPLIFIED_ABOVE_RESIDUAL;
        break;
      case MP4SPACEENC_DOWNMIXTYPE_SIMPLIFIED:
        tmpDownmixType = DOWNMIXTYPE_SIMPLIFIED;
        break;
      case MP4SPACEENC_DOWNMIXTYPE_INVALID:
      default:
        tmpDownmixType = DOWNMIXTYPE_INVALID;
        error = iisUtil_ERROR(CDI, "Invalid downmix type.");
        break;
    }
  }

  if (noError == error && hEnc->encMode == MP4SPACEENC_USAC_212) {
    switch (hEnc->ipdMode) {
      case MP4SPACEENC_IPDMODE_NONE:
        tmpIpdMode = IPDMODE_NONE;
        if (tmpDownmixType != DOWNMIXTYPE_CLASSICMPS) {
          tmpIpdMode = IPDMODE_INVALID;
          error = iisUtil_ERROR(CDI, "Invalid combination of downmix type and IPD mode.");
        }
        break;
      case MP4SPACEENC_IPDMODE_NO_RESIDUAL:
        tmpIpdMode = IPDMODE_NO_RESIDUAL;
        break;
      case MP4SPACEENC_IPDMODE_RESIDUAL:
        tmpIpdMode = IPDMODE_RESIDUAL;
        break;
      case MP4SPACEENC_IPDMODE_INVALID:
      default:
        tmpIpdMode = IPDMODE_INVALID;
        error = iisUtil_ERROR(CDI, "Invalid IPD mode.");
        break;
    }
  }

  if (error == noError) {
    switch (hEnc->encMode) {
      case MP4SPACEENC_USAC_212:
        hSpaceTreeSetup->mode = SPACETREE_USAC_212;
        hSpaceTreeSetup->nParamBands = hEnc->nParamBands;
        hSpaceTreeSetup->bOneIcc = hEnc->useOneIcc;
        hSpaceTreeSetup->bCalcIccDiff = tmpCalcIccDiff;
        hSpaceTreeSetup->downmixType = tmpDownmixType;
        hSpaceTreeSetup->ipdMode = tmpIpdMode;
        hSpaceTreeSetup->bDetectIpdRelevancy = hEnc->useIpdRelevancy;
        hSpaceTreeSetup->bInterpolateDownmix = !hEnc->useDmxOverlapAdd;
        hSpaceTreeSetup->bOttBandsPhasePresent = 0;
        switch (hSpaceTreeSetup->nParamBands) {
          case 4:
          case 5:
            hSpaceTreeSetup->nOttBandsPhase = 2;
            break;
          case 7:
            hSpaceTreeSetup->nOttBandsPhase = 3;
            break;
          case 10:
            hSpaceTreeSetup->nOttBandsPhase = 5;
            break;
          case 14:
            hSpaceTreeSetup->nOttBandsPhase = 7;
            break;
          case 20:
          case 28:
            hSpaceTreeSetup->nOttBandsPhase = 10;
            break;
          default:
            hSpaceTreeSetup->nOttBandsPhase = 0;
            break;
        }
        hSpaceTreeSetup->bUseCoarseQuantTtoCld = hEnc->useCoarseQuantCld;
        hSpaceTreeSetup->bUseCoarseQuantTtoIcc = hEnc->useCoarseQuantIcc;
        hSpaceTreeSetup->bUseCoarseQuantTtoIpd = hEnc->useCoarseQuantIpd;
        hSpaceTreeSetup->quantMode = tmpQuantmode;

        hSpaceTreeSetup->bUseTsd = (hEnc->tempShapeConfig == 3);

        __DecodeResidualMode(hEnc->residualConfig.mode, hSpaceTreeSetup->bCalcResiduals);
        memcpy(hSpaceTreeSetup->nResidualBands, hEnc->residualConfig.bands, sizeof(hEnc->residualConfig.bands));

        if (hSpaceTreeSetup->nOttBandsPhase < hSpaceTreeSetup->nResidualBands[0]) {
          hSpaceTreeSetup->nOttBandsPhase = hSpaceTreeSetup->nResidualBands[0];
        }

        hSpaceTreeSetup->bCalcDPL = 0;
        hSpaceTreeSetup->bUseCoarseQuantTttLowCldCpc = 0;
        hSpaceTreeSetup->bUseCoarseQuantTttHighCldCpc = 0;
        hSpaceTreeSetup->bUseCoarseQuantTttLowIcc = 0;
        hSpaceTreeSetup->bUseCoarseQuantTttHighIcc = 0;
        hSpaceTreeSetup->epsilonFloat = hEnc->epsilonFloat;
        hSpaceTreeSetup->nChannelsInMax = 2;
        hSpaceTreeSetup->nTimeSlotsMax = 2 * hEnc->nFrameTimeSlots;
        hSpaceTreeSetup->nHybridBandsMax = MAX_HYBRID_BANDS;

        break;

      case MP4SPACEENC_INVALID_MODE:
      default:
        error = iisUtil_ERROR(CDI, "Invalid Encoder Mode");
        break;
    }
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_SetDmxGain(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, MP4SPACEENC_DMX_GAIN dmxGain) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hMp4SpaceEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  } else if (dmxGain < 0 || dmxGain > 7) {
    error = iisUtil_ERROR(CDI, "Invalid downmix gain");
  }

  if (error == noError) {
    error = staticGain_SetDmxGain(hMp4SpaceEnc->hStaticGainConfig, dmxGain);
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_SetIndependencyFactor(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, const unsigned int independencyFactor) {
  HANDLE_ERROR_INFO error = noError;

  if (NULL == hMp4SpaceEnc) {
    error = iisUtil_ERROR(CDI, "Invalid Handle");
  }
  if (error == noError) {
    hMp4SpaceEnc->independencyFactor = independencyFactor;
  }

  return error;
}

int mp4SpaceEnc_GetIndependencyFlag(HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc, unsigned char *pBuf) {
  int framingType = 0;
  int numParamSets = 0;
  int independencyFlag = 0;

  int data = (pBuf[0] << 8) + pBuf[1];

  int numSlots = hMp4SpaceEnc->nFrameTimeSlots;

  framingType = (pBuf[0] & 0x80) >> 7;

  numParamSets = ((pBuf[0] & 0x70) >> 4) + 1;

  assert(numParamSets <= 2);

  if (framingType == 1) {
    int i;
    int mask = 0;

    int numBitsToSkip = ceillog2(numSlots - numParamSets + 1);

    for (i = 0; i < numBitsToSkip; i++) {
      mask += 1 << i;
    }

    if (numParamSets > 1) {
      int bsParamSlot;

      mask = mask << (16 - 4 - numBitsToSkip);

      bsParamSlot = (data & mask) >> (16 - 4 - numBitsToSkip);

      numBitsToSkip += ceillog2(numSlots - numParamSets + 1 - bsParamSlot);
    }

    numBitsToSkip += 1 + 3 + 1;

    mask = 1 << (16 - numBitsToSkip);

    independencyFlag = (data & mask) >> (16 - numBitsToSkip);

  } else {
    independencyFlag = (pBuf[0] & 0x08) >> 3;
  }

  return independencyFlag;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_ForceIndependency(
    HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc) {
  HANDLE_ERROR_INFO error = noError;

  if (hMp4SpaceEnc == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
  }

  if (error == noError) {
    hMp4SpaceEnc->forceIndependency = 1;
  }

  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_GetUsacMps212Config(HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc, MP4SPACEENC_USAC_MPS212_CONFIG *pUsacMps212Config) {
  HANDLE_ERROR_INFO error = noError;

  if (error == noError) {
    if (hMp4SpaceEnc == NULL) {
      error = iisUtil_ERROR(CDI, "Invalid handle");
    }
  }

  if (error == noError) {
    SPATIALSPECIFICCONFIG *hSsc = GetSpatialSpecificConfig(hMp4SpaceEnc->hBitstreamFormatter);
    if (noError != (error = GetUsacMps212Config(hSsc, pUsacMps212Config))) {
      error = handBack(error);
    }
  }
  return error;
}

HANDLE_ERROR_INFO
mp4SpaceEnc_SetSpeechFlag(
    HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc,
    int speechFlag) {
  HANDLE_ERROR_INFO error = noError;

  if (hMp4SpaceEnc == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid Encoder Handle");
  }

  if (error == noError) {
    hMp4SpaceEnc->speechFlag = speechFlag;
  }

  return error;
}
