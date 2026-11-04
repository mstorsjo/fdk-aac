
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

#include "iisxHEAACEncLib_resampler_ifc.h"
#include "iisxHEAACEncLib_common.h"

static const float FILTER_TBW = 0.0125f;
static const float FILTER_TBW_PRERESAMPLER = 0.05f;
static const float FILTER_ATTENUATION = 60;
static const float FILTER_TBWAAC = 0.1f;
static const float FILTER_ATTENUATIONAAC = 75.0f;
static const float FILTER_TBWAAC_EXTREME = 0.03f;
static const float FILTER_ATTENUATIONAAC_EXTREME = 60.0f;

static int calculateGCD(int a, int b);

static int calculateGCD(int a, int b) {
  int remainder;

  if (a < b) {
    int tmp = a;
    a = b;
    b = tmp;
  }

  do {
    remainder = a % b;
    a = b;
    b = remainder;
  } while (b != 0);

  return (a);
}

static void correctFiltParams(smpl_filterParams* pFiltParam, int srIn, int srOut);
static void correctFiltParams(smpl_filterParams* pFiltParam, int srIn, int srOut) {
  const int nTapsMax = 28000;
  int gcd = calculateGCD(srIn, srOut);
  int factor;
  if (srOut / gcd > 256)
    factor = 256;
  else
    factor = (srOut / gcd);
  int filterScale = factor * (srIn / gcd);
  float intermSR = (float)(srIn * factor);
  float tbwMin = 0.f;
  float tbwNew = 0.f;

  int nTaps = (int)((pFiltParam->stopBandAttenuation - 7.95) / (14.36 * 1.66 * pFiltParam->transitionBandwidth / intermSR) + 1.0);
  if (filterScale > nTaps)
    filterScale = (srIn / gcd);
  int polyFilterLength = nTaps / filterScale;
  if (nTaps % filterScale)
    polyFilterLength++;
  nTaps = polyFilterLength * filterScale;

  tbwMin = (pFiltParam->stopBandAttenuation - 7.95f) / ((float)(nTapsMax - 1) * 23.8376f);

  if ((tbwMin * intermSR) > pFiltParam->transitionBandwidth && srIn > srOut) {
    factor = (int)(((factor * srOut) / srIn) + 0.5);
    intermSR = (float)(srIn * factor);
  }

  if ((tbwMin * intermSR) > pFiltParam->transitionBandwidth || (nTaps > nTapsMax && srIn < srOut)) {
    if (FILTER_ATTENUATIONAAC_EXTREME < pFiltParam->stopBandAttenuation) {
      pFiltParam->stopBandAttenuation = FILTER_ATTENUATIONAAC_EXTREME;
    }
    tbwMin = (pFiltParam->stopBandAttenuation - 7.95f) / ((float)(nTapsMax - 1) * 23.8376f);

    tbwMin *= 1.01f;
    if ((tbwMin * intermSR) > (FILTER_TBWAAC_EXTREME * (float)srIn)) {
      tbwNew = tbwMin * intermSR;
    } else {
      tbwNew = FILTER_TBWAAC_EXTREME * srIn;
    }
    pFiltParam->transitionBandwidth = tbwNew;
    assert(nTapsMax >
           ((pFiltParam->stopBandAttenuation - 7.95f) / (pFiltParam->transitionBandwidth / intermSR * 23.8376f) + 1.f));
  }
}

int iisxHEAACEncLib_resampler_ifc_getCoreResamplerDelay(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    int const mpeg4DelayWOSwitching,
    int const aacCoreCoderDelay,
    int const sbrDecoderDelay) {
  int nDelay = mpeg4DelayWOSwitching -

               hConfig->sbrRatio.upFac * aacCoreCoderDelay / hConfig->sbrRatio.downFac -
               sbrDecoderDelay;

  if (hConfig->bUseSBR) {
    nDelay -= (640 - 64 + 2);
  }

  return nDelay;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_resampler_ifc_openCoreResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib** hCoreDownSampler,
    int* const coreResamplerDelay,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    float const aacCoreBandwidth,
    float** pCoreDownSamplerIn,
    unsigned int* const pSamplesCoreResWanted) {
  unsigned int nSamplesCoreResWanted = 0;
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || hCoreDownSampler == NULL || coreResamplerDelay == NULL || pCoreDownSamplerIn == NULL || pSamplesCoreResWanted == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  } else if (hConfig->bUseSBR && hSbrEnc == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (!hConfig->bUseQmfResampler) {
      smpl_filterParams *pFiltParam, filtParam;
      int srIn, srOut;

      srIn = hConfig->sampleRateOut * hConfig->sbrRatio.upFac;
      srOut = hConfig->sampleRateOut * hConfig->sbrRatio.downFac;

      if (hConfig->aot != AUD_OBJ_TYP_USAC) {
        if (hConfig->configSet == CONFIG_SET_DASH) {
          filtParam.lowpassFrequency = (hConfig->sampleRateAAC / 2 * (1.f - 0.25f * FILTER_TBWAAC)) * hConfig->sbrRatio.upFac;
          filtParam.transitionBandwidth = (hConfig->sampleRateAAC / 2 * FILTER_TBWAAC) * hConfig->sbrRatio.upFac;
          filtParam.stopBandAttenuation = FILTER_ATTENUATIONAAC;
        } else if (aacCoreBandwidth + aacCoreBandwidth * FILTER_TBWAAC <= hConfig->sampleRateAAC / 2) {
          filtParam.lowpassFrequency = aacCoreBandwidth * hConfig->sbrRatio.upFac;
          filtParam.transitionBandwidth = aacCoreBandwidth * hConfig->sbrRatio.upFac * FILTER_TBWAAC;
          filtParam.stopBandAttenuation = FILTER_ATTENUATIONAAC;
        } else {
          filtParam.lowpassFrequency = aacCoreBandwidth * hConfig->sbrRatio.upFac;
          filtParam.transitionBandwidth = aacCoreBandwidth * hConfig->sbrRatio.upFac * FILTER_TBWAAC_EXTREME;
          filtParam.stopBandAttenuation = FILTER_ATTENUATIONAAC_EXTREME;
        }
      } else {
        filtParam.lowpassFrequency = 0.5f * srOut * (1.f - 0.5f * FILTER_TBW);
        filtParam.transitionBandwidth = 0.5f * srOut * FILTER_TBW;
        filtParam.stopBandAttenuation = FILTER_ATTENUATION;
      }

      if ((hConfig->bUseSBR) &&
          (hConfig->sampleRateOut != hConfig->sampleRateAAC)) {
        pFiltParam = &filtParam;
      } else {
        pFiltParam = NULL;
      }

      if (0 != smpl_resampler_fo_construct(hCoreDownSampler,
                                           pFiltParam,
                                           srIn,
                                           srOut,
                                           hConfig->nChannelsCoreCoder,
                                           hConfig->granuleLength * hConfig->nChannelsCoreCoder,
                                           pCoreDownSamplerIn,
                                           &nSamplesCoreResWanted)) {
        retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
      }

      *coreResamplerDelay = smpl_resampler_get_delay(*hCoreDownSampler);
    } else {
      *coreResamplerDelay = iisxHEAACEncLibSbrEncGetPsTimeSignalDelay(hSbrEnc);
      nSamplesCoreResWanted = hConfig->nFrameSamples * hConfig->nChannelsCoreCoder;
    }

    *pSamplesCoreResWanted = nSamplesCoreResWanted;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_resampler_ifc_openPreResampler(
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    struct tag_resamplelib** hPreResampler,
    TIME_SIGNAL_DATA* const timeSignal,
    int* const preResamplerDelay,
    float const aacCoreBandwidth,
    unsigned int const nSamplesCoreResWanted) {
  smpl_filterParams *pFiltParam, filtParam;

  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hConfig == NULL || hPreResampler == NULL || timeSignal == NULL || preResamplerDelay == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      filtParam.lowpassFrequency = aacCoreBandwidth;
      filtParam.transitionBandwidth = aacCoreBandwidth * FILTER_TBW_PRERESAMPLER;
      filtParam.stopBandAttenuation = FILTER_ATTENUATION;
    } else {
      filtParam.lowpassFrequency = aacCoreBandwidth;
      filtParam.transitionBandwidth = aacCoreBandwidth * FILTER_TBWAAC;
      filtParam.stopBandAttenuation = FILTER_ATTENUATIONAAC;
    }

    if (hConfig->bUseSBR == 0) {
      switch (hConfig->quality) {
        case XHEAACENCLIB_QUAL_FAST:
        case XHEAACENCLIB_QUAL_MEDIUM:
          if (hConfig->sampleRateIn == hConfig->sampleRateOut) {
            pFiltParam = NULL;
          } else {
            pFiltParam = &filtParam;
          }
          break;
        case XHEAACENCLIB_QUAL_HIGH:
          if ((hConfig->sampleRateIn == hConfig->sampleRateOut) &&
              (aacCoreBandwidth >= min(12000, (float)hConfig->sampleRateOut / 2))) {
            pFiltParam = NULL;
          } else {
            pFiltParam = &filtParam;
          }
          break;
        default:
          pFiltParam = &filtParam;
          break;
      }
    } else {
      if (hConfig->aot == AUD_OBJ_TYP_USAC) {
        filtParam.lowpassFrequency = 0.5f * hConfig->sampleRateOut * (1.f - 0.5f * FILTER_TBW_PRERESAMPLER);
        filtParam.transitionBandwidth = 0.5f * hConfig->sampleRateOut * FILTER_TBW_PRERESAMPLER;
        filtParam.stopBandAttenuation = FILTER_ATTENUATION;

        pFiltParam = &filtParam;
      } else {
        pFiltParam = NULL;
      }
    }
    if (NULL != pFiltParam) {
      correctFiltParams(pFiltParam, hConfig->sampleRateIn, hConfig->sampleRateOut);
    }

    if (0 != smpl_resampler_fo_construct(hPreResampler,
                                         pFiltParam,
                                         hConfig->sampleRateIn,
                                         hConfig->sampleRateOut,
                                         hConfig->nInChannels,
                                         (nSamplesCoreResWanted * hConfig->nInChannels) / hConfig->nChannelsCoreCoder,
                                         &timeSignal->pPreResamplerIn,
                                         &timeSignal->nSamplesRequired)) {
      retValue = XHEAACENCLIB_RETURN_ERROR_CORE_RESAMPLER;
    }
  }

  if (!isError(retValue)) {
    *preResamplerDelay = smpl_resampler_get_delay(*hPreResampler);
    timeSignal->nSamplesNext = timeSignal->nSamplesRequired;
    timeSignal->nSamplesValid = 0;
  }

  timeSignal->pAacFrameCoreDelayInBuffer = (float*)iisCalloc((nSamplesCoreResWanted * hConfig->nInChannels) / hConfig->nChannelsCoreCoder, sizeof(float));
  return retValue;
}
