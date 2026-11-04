
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
#include <assert.h>
#include <math.h>

#include "mathlib.h"
#include "iisxHEAACEncLib_mpegs_ifc.h"
#include "iisSigMap.h"
#include "time_buffer.h"
#include "usacconfig.h"
#include "aacenc.h"
#include "spaceEnclib.h"
#include "iisxHEAACEncLib_mpegs_cfg_expert.h"
#include "iisxHEAACEncLib_common.h"
#define CLASSICAL_MPS

#define MPEGS_INDEP_INTERVAL (5.0)
#define MAX_AAC_CHANNEL_BITS 6144
#define MAX_MPEGS_PAYLOAD_SIZE ((MAX_AAC_CHANNEL_BITS / 8) * 2)

struct MpegsEncoder {
  CHANNEL_MAPPING *cm;
  MP4SPACEENC_SETUP mp4SpaceEncSetup;
  int mpegsMuxMode;
  int mpegsIndepFactor;
  int mpegsIndepCnt;
  int bMpegsEnhancedMatrixMode;
  unsigned int nSscTransmitCount;
  unsigned int nSscTransmitFactor;

  HANDLE_MP4SPACE_ENCODER *phMp4SpaceEnc;
  MP4SPACEENC_INFO *pMp4SpaceEncInfo;
  HANDLE_MP4TIMEBUF hMpegsInputBuffer[USAC_MAX_ELEMENTS];
  unsigned int nBitsMpegsElementPayload[USAC_MAX_ELEMENTS];
  unsigned char **ppMpegsPayloadBuffer;
  float *pTmpMpegsOutBuffer;
  float *pTmpMpegs212InBuffer;
  unsigned int mpegsOutSamplesRead;
  unsigned int mpegsPayloadBufferSize;
  unsigned int sizeTmpMpegsOutBuffer;

  HANDLE_UNISTE *phUniSte;
};

static unsigned int *iisxHEAACEncLibMpegsEncGetMpegsPayload(XHEAACENCLIB_HANDLE_MPEGSENCODER self);
static unsigned char **iisxHEAACEncLibMpegsEncGetMpegsPayloadBuffer(XHEAACENCLIB_HANDLE_MPEGSENCODER self);

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncConfigure(
    XHEAACENCLIB_HANDLE_MPEGSENCODER *p_hMpegsEnc,
    XHEAACENCLIB_CONFIG_HANDLE hConfig) {
  XHEAACENCLIB_HANDLE_MPEGSENCODER hMpegsEnc;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (p_hMpegsEnc == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hMpegsEnc = (XHEAACENCLIB_HANDLE_MPEGSENCODER)iisCalloc(1, sizeof(XHEAACENCLIB_MPEGSENCODER));
    if (hMpegsEnc == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }

    if (!isError(retValue)) {
      hMpegsEnc->cm = hConfig->cm;
      hConfig->nChannelsCoreCoder = hConfig->cm->nChannels;

      retValue = iisxHEAACEncLibMpegsConfigurationExpert(hConfig,
                                                         &hMpegsEnc->mp4SpaceEncSetup);
    }

    if (!isError(retValue)) {
      if (hConfig->stereoConfigIndex > 1) {
        retValue = iisxHEAACEncLibMpegsResidualConfig(hConfig->bitRate, hConfig->stereoConfigIndex, &hMpegsEnc->mp4SpaceEncSetup);
      }
    }

    if (!isError(retValue)) {
      if ((int)(hConfig->nFrameSamples / 64) == (int)hMpegsEnc->mp4SpaceEncSetup.frameTimeSlots) {
        if (hMpegsEnc->mp4SpaceEncSetup.residualConfig.codec == MP4SPACEENC_RES_CODEC_PCM) {
          hMpegsEnc->mpegsMuxMode = 2;
        } else {
          hMpegsEnc->mpegsMuxMode = 1;
        }
      } else if ((int)(2 * hConfig->nFrameSamples / 64) == (int)hMpegsEnc->mp4SpaceEncSetup.frameTimeSlots) {
        hMpegsEnc->mpegsMuxMode = 2;
      } else {
        hMpegsEnc->mpegsMuxMode = 3;
      }

      hMpegsEnc->mpegsIndepFactor = (int)max(1, (MPEGS_INDEP_INTERVAL * hConfig->sampleRateOut) / (64 * hMpegsEnc->mp4SpaceEncSetup.frameTimeSlots));
      hMpegsEnc->mpegsIndepCnt = 0;
    }

    *p_hMpegsEnc = hMpegsEnc;
  }

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncOpen(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                            CLASSICAL_MPS MP4SPACEENC_ENCODERTYPE mpsEncoderType,
                            int frameLengthAAC,
                            float aacCoreBandwidth,
                            int bUseSbrQmfInput) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  HANDLE_MP4SPACE_ENCODER hMp4SpaceEnc = NULL;
  MP4SPACEENC_INFO mp4SpaceEncInfo;
  int nFrameLength = frameLengthAAC;
  int nDmxDelay = 0;
  int i = 0;
  int nMpegs212Instances = 0;
  int bResidualCoding;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    int const nElements = self->cm->nElements;
    bResidualCoding = (self->mp4SpaceEncSetup.residualConfig.mode != 0);

    if (NULL == (self->phMp4SpaceEnc = (HANDLE_MP4SPACE_ENCODER *)iisCalloc(nElements, sizeof(HANDLE_MP4SPACE_ENCODER)))) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
    }
    if (NULL == (self->pMp4SpaceEncInfo = (MP4SPACEENC_INFO *)iisCalloc(nElements, sizeof(MP4SPACEENC_INFO)))) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
    }

    if (bResidualCoding) {
      if (NULL == (self->phUniSte = (HANDLE_UNISTE *)iisCalloc(nElements, sizeof(HANDLE_UNISTE)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
      } else {
        for (i = 0; (i < nElements) && (!isError(retValue)); i++) {
          if (NULL == (self->phUniSte[i] = (HANDLE_UNISTE)iisCalloc(1, sizeof(UNISTE)))) {
            retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
          }
        }
      }
    } else {
      self->phUniSte = NULL;
    }

    for (i = 0; (i < nElements) && (!isError(retValue)); i++) {
      if (self->cm->elInfo[i].bMpegs212 > 0) {
        int dmxDelay = 0;
        unsigned int discardOutFrames = 0;
        unsigned int nSamplesMpegs;

        hMp4SpaceEnc = self->phMp4SpaceEnc[i];
        mp4SpaceEncInfo = self->pMp4SpaceEncInfo[i];
        self->mpegsOutSamplesRead = 0;

        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = mp4SpaceEnc_Open(&hMp4SpaceEnc,
                                     &self->mp4SpaceEncSetup,
                                     bUseSbrQmfInput, mpsEncoderType);
        if (errorInfo == noError) {
          errorInfo = mp4SpaceEnc_Init(&hMp4SpaceEnc, &nSamplesMpegs, dmxDelay, &discardOutFrames, aacCoreBandwidth);
        }
        if (errorInfo == noError) {
          errorInfo = mp4SpaceEnc_GetInfo(hMp4SpaceEnc, &mp4SpaceEncInfo);
        }
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
        }

        self->phMp4SpaceEnc[i] = hMp4SpaceEnc;
        self->pMp4SpaceEncInfo[i] = mp4SpaceEncInfo;
        nMpegs212Instances++;
        nFrameLength = max(nFrameLength, self->pMp4SpaceEncInfo[i].nSamplesFrame);
        nDmxDelay = max(nDmxDelay, self->pMp4SpaceEncInfo[i].nDmxDelay);
      } else {
        self->phMp4SpaceEnc[i] = NULL;
      }
    }

    if (isError(retValue) || nMpegs212Instances < 1) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MPEGS_INIT_FAIL;
    }

    if (!isError(retValue)) {
      int el = 0, nChannels = 0, writeOffset = 0;

      for (el = 0; el < nElements; el++) {
        if (self->cm->elInfo[el].bMpegs212 > 0) {
          nChannels = (bResidualCoding) ? 2 : 1;
          writeOffset = 0;
        } else {
          nChannels = self->cm->elInfo[el].nChannelsInEl;
          writeOffset = nDmxDelay - 320;
        }
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = MP4TIMEBUF_Create(&self->hMpegsInputBuffer[el],
                                      nFrameLength + writeOffset,
                                      nFrameLength,
                                      nChannels,
                                      writeOffset);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
        }
      }
    }

    if (!isError(retValue)) {
      self->mpegsPayloadBufferSize = MAX_MPEGS_PAYLOAD_SIZE;
      if (NULL == (self->ppMpegsPayloadBuffer = (unsigned char **)iisCalloc(nElements, sizeof(char *)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
      } else {
        for (i = 0; ((i < nElements) && (!isError(retValue))); i++) {
          if (self->cm->elInfo[i].bMpegs212 > 0) {
            if (NULL == (self->ppMpegsPayloadBuffer[i] = (unsigned char *)iisCalloc(self->mpegsPayloadBufferSize, sizeof(char)))) {
              retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
            }
          } else {
            self->ppMpegsPayloadBuffer[i] = NULL;
          }
        }
      }
    }

    if (!isError(retValue)) {
      self->sizeTmpMpegsOutBuffer = nFrameLength * nMpegs212Instances * ((bResidualCoding) ? 2 : 1);

      if (NULL == (self->pTmpMpegsOutBuffer = (float *)iisCalloc(self->sizeTmpMpegsOutBuffer, sizeof(float)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
      }
    }

    if (!isError(retValue)) {
      if (NULL == (self->pTmpMpegs212InBuffer = (float *)iisCalloc(nFrameLength * 2, sizeof(float)))) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CALLOC_FAIL;
      }
    }
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncInitDelayCompensation(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                             int addEncDelay,
                                             int standardDelay,
                                             int *nAddEncDelay,
                                             int *nStandardDelay,
                                             int *nSamplesFrameMax) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nCodecDelayTmp, nSamplesFrameTmp, nAddEncDelayTmp, nStandardDelayTmp;
  int const nElements = self->cm->nElements;
  int elem;

  if (self == NULL || nAddEncDelay == NULL || nStandardDelay == NULL || nSamplesFrameMax == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  for (elem = 0; ((elem < nElements) && (!isError(retValue))); elem++) {
    if (NULL != self->phMp4SpaceEnc[elem]) {
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = mp4SpaceEnc_InitDelayCompensation(self->phMp4SpaceEnc[elem], addEncDelay + standardDelay);
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
      }
    }
  }

  for (elem = 0; ((elem < nElements) && (!isError(retValue))); elem++) {
    if (NULL != self->phMp4SpaceEnc[elem]) {
      HANDLE_ERROR_INFO errorInfo = noError;
      errorInfo = mp4SpaceEnc_GetInfo(self->phMp4SpaceEnc[elem], &(self->pMp4SpaceEncInfo[elem]));
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
      }
    }
  }

  if (!isError(retValue)) {
    nCodecDelayTmp = self->pMp4SpaceEncInfo[0].nCodecDelay;
    nAddEncDelayTmp = self->pMp4SpaceEncInfo[0].nDmxDelay + addEncDelay;
    nStandardDelayTmp = nCodecDelayTmp - nAddEncDelayTmp;
    nSamplesFrameTmp = self->pMp4SpaceEncInfo[0].nSamplesFrame;

    for (elem = 1; elem < nElements; elem++) {
      nCodecDelayTmp = max(nCodecDelayTmp, self->pMp4SpaceEncInfo[elem].nCodecDelay);

      nAddEncDelayTmp = self->pMp4SpaceEncInfo[elem].nDmxDelay + addEncDelay;

      nStandardDelayTmp = max(nStandardDelayTmp, nCodecDelayTmp - nAddEncDelayTmp);

      nSamplesFrameTmp = max(nSamplesFrameTmp, self->pMp4SpaceEncInfo[elem].nSamplesFrame);
    }
    *nStandardDelay = nStandardDelayTmp;
    *nAddEncDelay = nCodecDelayTmp - nStandardDelayTmp;
    *nSamplesFrameMax = nSamplesFrameTmp;
  }

  return retValue;
}

static const float kDmxGainTable[8] =
    {
        1.0f,
        1.18920711500272f,
        1.41421356237310f,
        1.68179283050743f,
        2.0f,
        2.37841423000544f,
        2.82842712474619f,
        4.0f};

static int UniSteInit(HANDLE_UNISTE hUniSte,
                      HANDLE_MP4SPACE_ENCODER const hMp4SpaceEnc,
                      MP4SPACEENC_SETUP *const pMp4SpaceEncSetup,
                      int const samplingRateCore,
                      int const nAacResampAndLookaheadDelay,
                      int const sampleRateOut,
                      int const aacCoreBandWidth) {
  int error = 0;
  int ts = 0, paramBand = 0;

  MP4SPACEENC_INFO mp4SpaceEncInfo;

  int nResidualBands = pMp4SpaceEncSetup->residualConfig.bands[0];
  int nEncoderDelay = 0;
  int nQmfBands = 64;
  mp4SpaceEnc_GetInfo(hMp4SpaceEnc, &mp4SpaceEncInfo);
  nEncoderDelay = mp4SpaceEncInfo.nDmxDelay;

  hUniSte->bStereoSbr = pMp4SpaceEncSetup->bStereoSbr;
  hUniSte->bPseudoLr = pMp4SpaceEncSetup->bPseudoLr;
  hUniSte->nParamBands = pMp4SpaceEncSetup->nParamBands;
  hUniSte->nTimeSlots = 32 + 300;

  hUniSte->delayUmxMat2Mdct = nEncoderDelay + nAacResampAndLookaheadDelay;
  hUniSte->delayUmxMat2Mdct -= 256;

  hUniSte->delayUmxMat2Mdct -= mp4SpaceEncInfo.nSamplesFrame;
  hUniSte->delayUmxMat2Mdct -= 704;

  hUniSte->delayUmxMat2Mdct = (int)((hUniSte->delayUmxMat2Mdct / 64.0f) + 0.5f);

  for (ts = 0; ts < hUniSte->nTimeSlots; ts++) {
    int bsFixedGainDmx = mp4SpaceEnc_GetBsFixedGainDmx(hMp4SpaceEnc);
    float icc0_cld0_ipd0_coeff = 0.5f * kDmxGainTable[bsFixedGainDmx];

    for (paramBand = 0; paramBand < hUniSte->nParamBands; paramBand++) {
      int j;
      hUniSte->umxMatRe[ts][paramBand][0] = icc0_cld0_ipd0_coeff;
      hUniSte->umxMatRe[ts][paramBand][1] = icc0_cld0_ipd0_coeff;
      hUniSte->umxMatRe[ts][paramBand][2] = icc0_cld0_ipd0_coeff;
      hUniSte->umxMatRe[ts][paramBand][3] = -icc0_cld0_ipd0_coeff;
      for (j = 0; j < 4; j++) {
        hUniSte->umxMatIm[ts][paramBand][j] = 0.0f;
      }
    }
  }

  if (hUniSte->bPseudoLr) {
    int i;
    float tmpRe[4];
    float tmpIm[4];

    hUniSte->pseudoLrUmxMat[0] = hUniSte->pseudoLrUmxMat[1] = hUniSte->pseudoLrUmxMat[2] = 1.f / (float)sqrt(2.f);
    hUniSte->pseudoLrUmxMat[3] = -1.f / (float)sqrt(2.f);

    for (ts = 0; ts < hUniSte->nTimeSlots; ts++) {
      for (paramBand = 0; paramBand < hUniSte->nParamBands; paramBand++) {
        const float *hRe = hUniSte->umxMatRe[ts][paramBand];
        const float *hIm = hUniSte->umxMatIm[ts][paramBand];
        const float *p = hUniSte->pseudoLrUmxMat;

        tmpRe[0] = hRe[0] * p[0] + hRe[1] * p[2];
        tmpRe[1] = hRe[0] * p[1] + hRe[1] * p[3];
        tmpRe[2] = hRe[2] * p[0] + hRe[3] * p[2];
        tmpRe[3] = hRe[2] * p[1] + hRe[3] * p[3];

        tmpIm[0] = hIm[0] * p[0] + hIm[1] * p[2];
        tmpIm[1] = hIm[0] * p[1] + hIm[1] * p[3];
        tmpIm[2] = hIm[2] * p[0] + hIm[3] * p[2];
        tmpIm[3] = hIm[2] * p[1] + hIm[3] * p[3];

        for (i = 0; i < 4; i++) {
          hUniSte->umxMatRe[ts][paramBand][i] = tmpRe[i];
          hUniSte->umxMatIm[ts][paramBand][i] = tmpIm[i];
        }
      }
    }
  } else {
    hUniSte->pseudoLrUmxMat[0] = hUniSte->pseudoLrUmxMat[3] = 1.0f;
    hUniSte->pseudoLrUmxMat[1] = hUniSte->pseudoLrUmxMat[2] = 0.0f;
  }

  hUniSte->nResidualLinesMdctLong = (int)(0.5f + (mp4SpaceEnc_ParamBand2Freq(hUniSte->nParamBands, sampleRateOut, nResidualBands, nQmfBands)) * (2.0f * 1024.0f / samplingRateCore));
  hUniSte->nResidualLinesMdctShort = (int)(0.5f + (mp4SpaceEnc_ParamBand2Freq(hUniSte->nParamBands, sampleRateOut, nResidualBands, nQmfBands)) * (2.0f * 128.0f / samplingRateCore));

  for (paramBand = 0; paramBand < hUniSte->nParamBands + 1; paramBand++) {
    hUniSte->paramBandBorders[paramBand] = mp4SpaceEnc_ParamBand2Freq(hUniSte->nParamBands, sampleRateOut, paramBand, nQmfBands);

    if (hUniSte->bStereoSbr && (hUniSte->paramBandBorders[paramBand] > aacCoreBandWidth)) {
      hUniSte->paramBandBorders[paramBand - 1] = (float)aacCoreBandWidth;
      hUniSte->paramBandBordersLong[paramBand - 1] = hUniSte->paramBandBorders[paramBand - 1] * (2.0f * 1024.0f / samplingRateCore);
      hUniSte->paramBandBordersShort[paramBand - 1] = hUniSte->paramBandBorders[paramBand - 1] * (2.0f * 128.0f / samplingRateCore);

      hUniSte->paramBandBorders[paramBand] = 0.0f;
      break;
    } else {
      hUniSte->paramBandBordersLong[paramBand] = hUniSte->paramBandBorders[paramBand] * (2.0f * 1024.0f / samplingRateCore);
      hUniSte->paramBandBordersShort[paramBand] = hUniSte->paramBandBorders[paramBand] * (2.0f * 128.0f / samplingRateCore);
    }
  }

  return error;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncInitUnivSte(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                   int nAacResampAndLookaheadDelay,
                                   int sampleRate,
                                   int sampleRateAAC,
                                   float aacCoreBandwidth,
                                   int *dmxDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int const nElements = self->cm->nElements;
  int el;

  for (el = 0; ((el < nElements) && (!isError(retValue))); el++) {
    if (NULL != self->phMp4SpaceEnc[el]) {
      if (UniSteInit(self->phUniSte[el],
                     self->phMp4SpaceEnc[el],
                     &self->mp4SpaceEncSetup,
                     sampleRateAAC,
                     nAacResampAndLookaheadDelay,
                     sampleRate,
                     (int)aacCoreBandwidth)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MPEGS_INIT_FAIL;
      }
    } else {
      self->phUniSte[el] = NULL;
    }
  }

  if (!isError(retValue)) {
    int nEncoderDelay = 0;
    int tmpDelay = 0;
    int nFrameLength = 0;
    for (el = 0; el < nElements; el++) {
      tmpDelay = self->pMp4SpaceEncInfo[el].nDmxDelay;
      nEncoderDelay = max(nEncoderDelay, tmpDelay);
      nFrameLength = max(nFrameLength, self->pMp4SpaceEncInfo[el].nSamplesFrame);
    }
    *dmxDelay = nFrameLength + nEncoderDelay;
  }

  return retValue;
}

int iisxHEAACEncLibMpegsEncGetMaxDelay(XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  int el = 0;
  int offset = 0;
  int tmp = 0;
  if (self) {
    for (el = 0; el < self->cm->nElements; el++) {
      tmp = mp4SpaceEnc_GetNumFramesBitstreamDelay(self->phMp4SpaceEnc[el]);
      if (tmp > offset) {
        offset = tmp;
      }
    }
    return offset;
  } else {
    return -1;
  }
}

MP4SPACEENC_RES_CONFIG *
iisxHEAACEncLibMpegsEncGetResidualConfig(XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  if (self)
    return (&self->mp4SpaceEncSetup.residualConfig);
  else
    return (NULL);
}

XHEAACENCLIB_HANDLE_UNISTE *
iisxHEAACEncLibMpegsEncGetUnifiedStereoHandle(XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  if (self)
    return (self->phUniSte);
  else
    return (NULL);
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncEncode(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                              int coreCoderFrameLength,
                              XHEAACENCLIB_SYNCFRAME_HANDLE hUsacIndepFlag,
                              int speechFlag,
                              float *pPreResamplerOut,
                              int nChannels,
                              int *nSamplesPreResampOut,
                              float **ppQmfSamplesReal,
                              float **ppQmfSamplesImag,
                              int nChannelsCoreCoder,
                              float *pPreResamplerOutLr,
                              int const usePreResamplerOutLr) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int elem = 0;
  int nFrameLength = coreCoderFrameLength;
  unsigned int nMpegsOutSamples = 0, nSamplesConsumed = 0;
  int nMpegsBits = 0;
  int const nElements = self->cm->nElements;
  unsigned int mpegsPayloadOffset = 0;
  unsigned int nSamplesUnprocessed = 0;

  if (self == NULL || hUsacIndepFlag == NULL || pPreResamplerOut == NULL || nSamplesPreResampOut == NULL || (usePreResamplerOutLr && pPreResamplerOutLr == NULL)) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if ((self->mpegsIndepCnt % self->mpegsIndepFactor) == 0) {
      for (elem = 0; elem < nElements; elem++) {
        if (NULL != self->phMp4SpaceEnc[elem]) {
          mp4SpaceEnc_ForceIndependency(self->phMp4SpaceEnc[elem]);
        }
      }
    }

    for (elem = 0; elem < nElements; elem++) {
      if (NULL != self->phMp4SpaceEnc[elem]) {
        nFrameLength = max(self->pMp4SpaceEncInfo[elem].nSamplesFrame, nFrameLength);
      }
    }
  }

  for (elem = 0; ((elem < nElements) && !isError(retValue)); elem++) {
    if (NULL != self->phMp4SpaceEnc[elem]) {
      int offset = mp4SpaceEnc_GetNumFramesBitstreamDelay(self->phMp4SpaceEnc[elem]);
      int bUsacIndependenceFlagSpace = 0;
      unsigned int nSamplesNextSpaceEnc = 0;
      float *tmpMpegs212InputBuf = self->pTmpMpegs212InBuffer;
      const unsigned int chOffsetL = 0;
      const unsigned int chOffsetR = 1;

      retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hUsacIndepFlag, offset - 1, &bUsacIndependenceFlagSpace, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);

      if (!isError(retValue)) {
        HANDLE_ERROR_INFO errorInfo = noError;
        copyFLOATflex(pPreResamplerOut + chOffsetL, nChannels, tmpMpegs212InputBuf, 2, (*nSamplesPreResampOut / nChannels));
        copyFLOATflex(pPreResamplerOut + chOffsetR, nChannels, tmpMpegs212InputBuf + 1, 2, (*nSamplesPreResampOut / nChannels));
        errorInfo = mp4SpaceEnc_SetSpeechFlag(self->phMp4SpaceEnc[elem], speechFlag);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
        }
      }

      if (!isError(retValue)) {
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = mp4SpaceEnc_Encode(self->phMp4SpaceEnc[elem],
                                       tmpMpegs212InputBuf,
                                       nFrameLength * 2,
                                       &nSamplesConsumed,
                                       &nSamplesNextSpaceEnc,
                                       &self->pTmpMpegsOutBuffer[self->mpegsOutSamplesRead],
                                       ppQmfSamplesReal,
                                       ppQmfSamplesImag,
                                       &nMpegsOutSamples,
                                       self->sizeTmpMpegsOutBuffer - self->mpegsOutSamplesRead,
                                       &self->ppMpegsPayloadBuffer[elem][mpegsPayloadOffset],
                                       self->mpegsPayloadBufferSize - mpegsPayloadOffset,
                                       &nMpegsBits,
                                       bUsacIndependenceFlagSpace);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
        }
      }

      if (!isError(retValue)) {
        self->mpegsOutSamplesRead += nMpegsOutSamples;
        self->nBitsMpegsElementPayload[elem] = (mpegsPayloadOffset * 8) + nMpegsBits;

        if (self->phUniSte) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = mp4SpaceEnc_UniSteUpdateFrame(self->phMp4SpaceEnc[elem],
                                                    self->phUniSte[elem]->umxMatRe,
                                                    self->phUniSte[elem]->umxMatIm,
                                                    self->phUniSte[elem]->cld,
                                                    self->phUniSte[elem]->delayUmxMat2Mdct,
                                                    self->phUniSte[elem]->bPseudoLr);

          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
          }
        }
      }
    } else {
      nSamplesUnprocessed += nFrameLength * self->cm->elInfo[elem].nChannelsInEl;
    }
  }

  if (!isError(retValue)) {
    unsigned int nSamplesMpegs212Out = 0;
    unsigned int nSamplesWrite = nFrameLength * nChannelsCoreCoder;
    if ((self->mpegsOutSamplesRead + nSamplesUnprocessed) < nSamplesWrite) {
      *nSamplesPreResampOut = 0;
    } else {
      if (self->phUniSte) {
        moveFLOAT(pPreResamplerOutLr + nSamplesWrite,
                  pPreResamplerOutLr,
                  self->pMp4SpaceEncInfo[0].nDmxDelay * nChannelsCoreCoder);

        moveFLOAT(pPreResamplerOut,
                  pPreResamplerOutLr + (self->pMp4SpaceEncInfo[0].nDmxDelay * nChannelsCoreCoder),
                  nSamplesWrite);
      }

      for (elem = 0; !isError(retValue) && (elem < nElements); elem++) {
        if (NULL != self->phMp4SpaceEnc[elem]) {
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = MP4TIMEBUF_FeedBufferStereo(self->hMpegsInputBuffer[elem],
                                                  &self->pTmpMpegsOutBuffer[nSamplesMpegs212Out],
                                                  ((NULL != self->phUniSte) ? &self->pTmpMpegsOutBuffer[nSamplesMpegs212Out + 1] : NULL),
                                                  ((NULL != self->phUniSte) ? 2 : 1),
                                                  1.f,
                                                  ((NULL != self->phUniSte) ? (nMpegsOutSamples / 2) : nMpegsOutSamples));
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
          }

          if (!isError(retValue)) {
            nSamplesMpegs212Out += nMpegsOutSamples;
          }
        } else {
          int bStereo = (self->cm->elInfo[elem].nChannelsInEl == 2);
          int chLeft = self->cm->elInfo[elem].ChannelIndex[0];
          int chRight = self->cm->elInfo[elem].ChannelIndex[1];
          HANDLE_ERROR_INFO errorInfo = noError;
          errorInfo = MP4TIMEBUF_FeedBufferStereo(self->hMpegsInputBuffer[elem],
                                                  &pPreResamplerOut[chLeft],
                                                  (bStereo ? &pPreResamplerOut[chRight] : NULL),
                                                  nChannels,
                                                  1.f,
                                                  nFrameLength);
          if (errorInfo != noError) {
            retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
          }
        }
      }

      for (elem = 0; !isError(retValue) && (elem < nElements); elem++) {
        int chOutIdx = self->cm->elInfo[elem].ChannelIndex[0];
        copyFLOATflex(MP4TIMEBUF_AccessBuffer(self->hMpegsInputBuffer[elem], 0, 0),
                      self->cm->elInfo[elem].nChannelsInEl,
                      &pPreResamplerOut[chOutIdx],
                      nChannelsCoreCoder,
                      nFrameLength);

        if (self->cm->elInfo[elem].nChannelsInEl == 2) {
          chOutIdx = self->cm->elInfo[elem].ChannelIndex[1];

          copyFLOATflex(MP4TIMEBUF_AccessBuffer(self->hMpegsInputBuffer[elem], 0, 1),
                        self->cm->elInfo[elem].nChannelsInEl,
                        &pPreResamplerOut[chOutIdx],
                        nChannelsCoreCoder,
                        nFrameLength);
        }
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = MP4TIMEBUF_InvalidateBuffer(self->hMpegsInputBuffer[elem], nFrameLength);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_TIMEBUFFER;
        }
      }

      copyFLOAT(&self->pTmpMpegsOutBuffer[nSamplesMpegs212Out], self->pTmpMpegsOutBuffer, self->mpegsOutSamplesRead - nSamplesMpegs212Out);
      *nSamplesPreResampOut = nSamplesWrite;
      self->mpegsOutSamplesRead -= nSamplesMpegs212Out;
    }
  }

  self->mpegsIndepCnt++;

  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncGetUsacMps212Config(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                           int eleIdx,
                                           MP4SPACEENC_USAC_MPS212_CONFIG *usacMps212Config) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (self == NULL || usacMps212Config == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (self->phMp4SpaceEnc) {
      if (self->phMp4SpaceEnc[eleIdx]) {
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = mp4SpaceEnc_GetUsacMps212Config(self->phMp4SpaceEnc[eleIdx], usacMps212Config);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
        }
      }
    }
  }
  return retValue;
}

void iisxHEAACEncLibMpegsEncUpdateCpcStopFrequency(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                                   float aacCoreBandwidth) {
  if (self->mp4SpaceEncSetup.cpcStopFrequency > aacCoreBandwidth / 1.6f) {
    self->mp4SpaceEncSetup.cpcStopFrequency = aacCoreBandwidth / 1.6f;
  }
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpegsEncClose(
    XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  int el = 0;
  for (el = 0; el < self->cm->nElements && !isError(retValue); el++) {
    HANDLE_ERROR_INFO errorInfo = noError;
    if (self->phMp4SpaceEnc && self->phMp4SpaceEnc[el]) {
      errorInfo = mp4SpaceEnc_Close(&(self->phMp4SpaceEnc[el]));
      if (errorInfo != noError) {
        retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
      }
    }

    if (self->ppMpegsPayloadBuffer != NULL) {
      iisFree(self->ppMpegsPayloadBuffer[el]);
    }

    errorInfo = MP4TIMEBUF_Delete(&(self->hMpegsInputBuffer[el]));
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
    }

    if (self->phUniSte && self->phUniSte[el]) {
      iisFree(self->phUniSte[el]);
    }
  }

  if (!isError(retValue)) {
    iisFree(self->phUniSte);
    iisFree(self->phMp4SpaceEnc);
    iisFree(self->pMp4SpaceEncInfo);
    iisFree(self->ppMpegsPayloadBuffer);

    if (self->pTmpMpegsOutBuffer) {
      iisFree(self->pTmpMpegsOutBuffer);
      self->pTmpMpegsOutBuffer = NULL;
    }

    if (self->pTmpMpegs212InBuffer) {
      iisFree(self->pTmpMpegs212InBuffer);
      self->pTmpMpegs212InBuffer = NULL;
    }

    if (self) iisFree(self);
  }
  return retValue;
}

XHEAACENCLIB_RETURN
iisxHEAACEncLibMpsThisExtPayloadSize(XHEAACENCLIB_HANDLE_MPEGSENCODER self,
                                     int *payloadSize) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int el = 0;

  if (self == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (payloadSize == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
  }

  if (!isError(retValue)) {
    *payloadSize = 0;
    if (self) {
      for (el = 0; el < self->cm->nElements && !isError(retValue); el++) {
        int tmp_PayloadSize = 0;
        HANDLE_ERROR_INFO errorInfo = noError;
        errorInfo = mp4SpaceEnc_GetNumBitsPayloadThisFrame(self->phMp4SpaceEnc[el], &tmp_PayloadSize);
        if (errorInfo != noError) {
          retValue = XHEAACENCLIB_RETURN_ERROR_MP4_SPACE_ENC;
        }
        if (!isError(retValue) && tmp_PayloadSize > 0) {
          *payloadSize += tmp_PayloadSize;
        }
      }
      if (isError(retValue) && payloadSize) {
        *payloadSize = 0;
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLibMpsMoveExtPayload(
    int const nElements,
    XHEAACENCLIB_HANDLE_MPEGSENCODER const hMpegsEnc,
    HANDLE_EXTPAYLOAD_CONTAINER hExtPayloadMps[],
    int *const usedBits) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hMpegsEnc == NULL || hExtPayloadMps == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    HANDLE_ERROR_INFO errorInfo = noError;
    unsigned int *mpsPayloadBits;
    unsigned char **mpsPayloadBuffer;

    mpsPayloadBits = iisxHEAACEncLibMpegsEncGetMpegsPayload(hMpegsEnc);
    mpsPayloadBuffer = iisxHEAACEncLibMpegsEncGetMpegsPayloadBuffer(hMpegsEnc);
    errorInfo = addExtensionPayload(hExtPayloadMps[0],
                                    EXT_USACMPS212_DATA,
                                    mpsPayloadBits[0],
                                    0,
                                    NO_NIBBLE,
                                    *mpsPayloadBuffer,
                                    0,
                                    0);
    if (errorInfo != noError) {
      retValue = XHEAACENCLIB_RETURN_ERROR_EXT_PAYLOAD;
    } else if (usedBits != NULL) {
      *usedBits += mpsPayloadBits[0];
    }
  }

  if (!isError(retValue)) {
    int elem = 0;
    *usedBits = 0;
    for (elem = 0; elem < nElements; elem++) {
      if (hExtPayloadMps[elem] != NULL) {
        *usedBits += getTotalSize_extPayload(hExtPayloadMps[elem]);
      }
    }
  }

  return retValue;
}

static unsigned int *iisxHEAACEncLibMpegsEncGetMpegsPayload(XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  if (self)
    return (self->nBitsMpegsElementPayload);
  else
    return (NULL);
}

static unsigned char **iisxHEAACEncLibMpegsEncGetMpegsPayloadBuffer(XHEAACENCLIB_HANDLE_MPEGSENCODER self) {
  if (self)
    return (self->ppMpegsPayloadBuffer);
  else
    return (NULL);
}
