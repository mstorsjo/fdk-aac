
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
#include <string.h>
#include "iisutillib.h"
#include "polyfilt.h"
#include "filtgen.h"
#include "resample.h"
#include "../share/rsl_dbgprint.h"

#include "kernfunc.h"

#define myMin(x, y) ((x) > (y) ? (y) : (x))

enum {
  MAX_FILTER_LENGTH = 32768,
  FIRST_UP_FAKTOR = 256
};

static const int MAX_FAST_FACTOR = 64;
static const int MAX_INTERP_FACTOR = 256;

#include "rsstruct.h"

int PolyCalculateGCD(int a, int b) {
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

static bool
GetResamplerMode(int L, int M, enum RESAMPLER_MODE* mode,
                 bool* fastConvFlag) {
  unsigned int product;

  product = M * MAX_INTERP_FACTOR;
  if ((unsigned int)L > product)
    return false;

  product = L * M;
  if (product < (unsigned int)MAX_FAST_FACTOR) {
    if (*fastConvFlag)
      *mode = fastResample;
    else {
      *mode = zeroResample;
      *fastConvFlag = false;
    }

  } else {
    *fastConvFlag = false;
    if (L < MAX_INTERP_FACTOR) {
      *mode = zeroResample;
    } else
      *mode = firstResample;
  }

  return true;
}

int PolyCalcInCount(struct RESAMPLER* hResampler,
                    int outBlockSize) {
  int M = 0;
  int L = 0;
  int restLeft = 0;
  int inSamplesNeeded = 0;
  int restTotal = 0;
  int numChan = 0;

  M = hResampler->inputFrequency;
  L = hResampler->outputFrequency;
  restLeft = hResampler->restLeft;
  numChan = hResampler->numChannels;

  if (restLeft == -1) {
    if (M <= L) {
      inSamplesNeeded = (M * (outBlockSize / numChan)) / L;
      restTotal = (M * (outBlockSize / numChan)) % L;
      restLeft = L - restTotal;
      hResampler->restLeft = restLeft;
      return (inSamplesNeeded);
    }
    if (M > L) {
      inSamplesNeeded = (M * (outBlockSize / numChan) - 1) / L;
      restTotal = (M * (outBlockSize - 1)) % L;
      restLeft = L - restTotal;
      hResampler->restLeft = restLeft;
      return (++inSamplesNeeded);
    }

  } else {
    inSamplesNeeded = (M * (outBlockSize / numChan)) / L;
  }

  restTotal = (M * outBlockSize / numChan) % L;

  if (restLeft <= restTotal) {
    restLeft = L - (restTotal - restLeft);
  } else {
    restLeft -= restTotal;
    inSamplesNeeded--;
  }

  hResampler->restLeft = restLeft;
  inSamplesNeeded *= numChan;
  return (++inSamplesNeeded);
}

static HANDLE_ERROR_INFO
CreateResampler(HANDLE_RESAMPLER* handleResampler,
                struct RESAMPLER_PARAM* resamplerParam,
                int numChannels,
                int* inBlockSize,
                int* outBlockSize)

{
  int transformLength = 0;
  int L, M, L1 = 0, L2;
  int gcd;
  int i;
  int filterLength = 0;
  int polyFilterLength = 0;
  int outDelaySize = 0;
  int filterScale;
  int inputFrequency, outputFrequency;

  struct FILT_PARAM filtParam;
  HANDLE_RESAMPLER hResampler;
  HANDLE_ERROR_INFO errorInfo;
  HANDLE_WORK_BUFFER workBuffer = 0;

  *handleResampler = NULL;

  inputFrequency = resamplerParam->inputFrequency;
  outputFrequency = resamplerParam->outputFrequency;

  if ((inputFrequency <= 0) || (outputFrequency <= 0)) {
    errorInfo = iisUtil_ERROR(CDI, "Invalid input frequencies !");
    return errorInfo;
  }

  hResampler = (HANDLE_RESAMPLER)iisCalloc(sizeof(struct RESAMPLER), 1);
  if (!hResampler)
    return iisUtil_ERROR(CDI, "out of memory !");

  if (resamplerParam->inOutAdjust) {
    if (*inBlockSize % numChannels)
      return iisUtil_ERROR(CDI, "input block size inconsistent");
  } else {
    if (*outBlockSize % numChannels)
      return iisUtil_ERROR(CDI, "output block size inconsistent");
  }

  gcd = PolyCalculateGCD(inputFrequency, outputFrequency);

  L = outputFrequency / gcd;
  M = inputFrequency / gcd;

  if (resamplerParam->bypassFlag) {
    hResampler->mode = bypass;
    filtParam.coef = NULL;
  } else {
    if (!GetResamplerMode(L, M, &hResampler->mode,
                          &resamplerParam->fastConvFlag))
      return iisUtil_ERROR(CDI, "sampling rate conversion ratio too large !");

    if (resamplerParam->type != (FASTRES | ZERORES | FIRSTRES)) {
      if (hResampler->mode == fastResample) {
        if (!(resamplerParam->type & FASTRES))

          hResampler->mode = zeroResample;
      }

      if (hResampler->mode == zeroResample) {
        if (!(resamplerParam->type & ZERORES))

          hResampler->mode = firstResample;
      }

      if (hResampler->mode == firstResample) {
        if (!(resamplerParam->type & FIRSTRES)) {
          errorInfo = iisUtil_ERROR(CDI, "sampling rate conversion ratio not supported by chosen resampling mode");
          return errorInfo;
        }
      }
    }

    if (L > resamplerParam->firstUpsamplingFactor) {
      L1 = resamplerParam->firstUpsamplingFactor;

    } else {
      L1 = L;
    }

    (*resamplerParam).L = L;
    (*resamplerParam).M = M;
    filtParam.lowpassFreq = resamplerParam->lowpassFrequency / (float)(L1 * inputFrequency);
    filtParam.bandwidth = resamplerParam->bandwidth / (float)(L1 * inputFrequency);
    filtParam.attenuation = resamplerParam->attenuation;
    filtParam.nTaps = resamplerParam->filterLength;
    filtParam.coef = NULL;
    filtParam.gain = resamplerParam->gain;

    EstimateKaiserParam(&filtParam);

    filterScale = M * L1;
    if (filterScale > filtParam.nTaps) {
      filterScale = M;
    }

    polyFilterLength = filtParam.nTaps / filterScale;
    if (filtParam.nTaps % filterScale)
      polyFilterLength++;

    if (resamplerParam->forceOddFlag && (polyFilterLength & 0X01))
      polyFilterLength++;

    filtParam.nTaps = polyFilterLength * filterScale;
    if (resamplerParam->forceOddFlag && !(filtParam.nTaps & 0X01))
      filtParam.nTaps++;

    if (filtParam.nTaps > MAX_FILTER_LENGTH) {
      if (hResampler->mode == zeroResample) {
        hResampler->mode = firstResample;
        resamplerParam->fakeFirstFlag = true;
      }

      if (inputFrequency > outputFrequency) {
        L2 = (int)(((L1 * outputFrequency) / inputFrequency) + 0.5);

        filtParam.lowpassFreq = resamplerParam->lowpassFrequency / (float)(L2 * inputFrequency);
        filtParam.bandwidth = resamplerParam->bandwidth / (float)(L2 * inputFrequency);

        filtParam.nTaps = 0;

        EstimateKaiserParam(&filtParam);

        if (resamplerParam->forceOddFlag && !(filtParam.nTaps & 0X01))
          filtParam.nTaps++;

        if (filtParam.nTaps > MAX_FILTER_LENGTH)
          return iisUtil_ERROR(CDI, "Filter length too large !");
        else {
          L1 = L2;
          resamplerParam->firstUpsamplingFactor = L2;
        }
      } else {
        return iisUtil_ERROR(CDI, "Filter length too large !");
      }
    }

    filtParam.coef = iisCalloc(filtParam.nTaps, sizeof(double));

    KaiserBesselWindow(&filtParam);
    WindowLowpass(&filtParam);

    filterLength = filtParam.nTaps;
    resamplerParam->filterLength = filterLength;
    resamplerParam->gain *= (float)L1;
  }

  {
    float tmp = (((float)filterLength - 1.0f) / 2.0f) /
                ((float)M * (float)L1 / (float)L);

    hResampler->outputDelay = (int)tmp;
    hResampler->outputDelayFrac = tmp;

    if (tmp > hResampler->outputDelay)
      hResampler->outputDelay++;
  }

  switch (hResampler->mode) {
    case bypass:

      resamplerParam->delay = 0;

      if (resamplerParam->inOutAdjust)
        *outBlockSize = *inBlockSize;
      else
        *inBlockSize = *outBlockSize;

      *handleResampler = hResampler;
      break;
    case fastResample: {
      HANDLE_FAST_RESAMPLER* hFastResampler;

      hFastResampler = (HANDLE_FAST_RESAMPLER*)iisCalloc(sizeof(HANDLE_FAST_RESAMPLER), (unsigned int)numChannels);
      if (!hFastResampler)
        return iisUtil_ERROR(CDI, "out of memory !");

      polyFilterLength = filterLength / (L * M);
      if (filterLength % (L * M))
        polyFilterLength++;

      for (i = 0; i < numChannels; i++) {
        errorInfo = CreateFastResampler(&(hFastResampler[i]), L, M,
                                        polyFilterLength);
        if (errorInfo != noError)
          return handBack(errorInfo);
      }

      if (resamplerParam->inOutAdjust) {
        *outBlockSize = ((*inBlockSize / numChannels + (M - 1)) / M) * L;
        *outBlockSize *= numChannels;
      } else {
        i = *outBlockSize * M / numChannels;
        *inBlockSize = i / L;
        if (i % L)
          (*inBlockSize)++;
        if (*inBlockSize % M)
          *inBlockSize = (*inBlockSize / M + 1) * M;
        *inBlockSize *= numChannels;
      }

      transformLength = 32;
      while (transformLength < *inBlockSize / (numChannels * M) + polyFilterLength)
        transformLength *= 2;
      outDelaySize = hFastResampler[0]->outDelaySize;

      hResampler->hFastResampler = hFastResampler;
    } break;

    case zeroResample: {
      HANDLE_ZERO_RESAMPLER* hZeroOrderInt;

      transformLength = 0;
      hZeroOrderInt = (HANDLE_ZERO_RESAMPLER*)iisCalloc(sizeof(HANDLE_ZERO_RESAMPLER), 1);
      if (!hZeroOrderInt)
        return iisUtil_ERROR(CDI, "out of memory !");

      polyFilterLength = filterLength / L;
      if (filterLength % L)
        polyFilterLength++;

      if (resamplerParam->inOutAdjust) {
        i = *inBlockSize * L / numChannels;
        *outBlockSize = i / M;
        if (i % M)
          (*outBlockSize)++;

        *outBlockSize *= numChannels;

      } else {
        i = *outBlockSize * M / numChannels;
        *inBlockSize = i / L;
        if (i % L)
          (*inBlockSize)++;

        (*inBlockSize)++;
        *inBlockSize *= numChannels;
      }

      errorInfo = CreateZeroOrderInterpolator(&hZeroOrderInt[0], L, M, resamplerParam->inOutAdjust);

      if (errorInfo != noError)
        return handBack(errorInfo);

      M = 1;
      hResampler->hZeroResampler = hZeroOrderInt;
    } break;

    case firstResample:

    {
      HANDLE_FIRST_RESAMPLER* hFirstOrderInt;

      transformLength = 0;
      hFirstOrderInt = (HANDLE_FIRST_RESAMPLER*)iisCalloc(sizeof(HANDLE_FIRST_RESAMPLER), 1);
      if (!hFirstOrderInt)
        return iisUtil_ERROR(CDI, "out of memory !");

      polyFilterLength = filterLength / L1;
      if (filterLength % (L1))

        polyFilterLength++;

      if (resamplerParam->inOutAdjust) {
        i = *inBlockSize * L / numChannels;
        *outBlockSize = i / M;
        if (i % M)
          (*outBlockSize)++;

        *outBlockSize *= numChannels;

      } else {
        i = *outBlockSize * M / numChannels;
        *inBlockSize = i / L;
        if (i % L)
          (*inBlockSize)++;

        (*inBlockSize)++;
        *inBlockSize *= numChannels;
      }

      errorInfo = CreateFirstOrderInterpolator(&hFirstOrderInt[0], L, M,
                                               resamplerParam->fakeFirstFlag, resamplerParam->inOutAdjust,
                                               L1);

      if (errorInfo != noError)
        return handBack(errorInfo);

      M = 1;

      hResampler->hFirstResampler = hFirstOrderInt;
    }

    break;

    default:
      return iisUtil_ERROR(CDI, "Unknown mode !");
  }

  hResampler->inBlockPerChan = *inBlockSize / numChannels;
  hResampler->outBlockPerChan = *outBlockSize / numChannels;
  hResampler->filtBlockSize = polyFilterLength;
  hResampler->outDelaySize = outDelaySize;
  hResampler->fixedNumberOfSamplesInput = *inBlockSize;
  hResampler->fixedNumberOfSamplesOutput = *outBlockSize;

  if (hResampler->mode != bypass) {
    errorInfo = CreatePolyphaseChannel(&(hResampler->hPolyphaseChannel), L1,
                                       M, transformLength, filterLength, filtParam.coef,
                                       resamplerParam->gain);
    if (errorInfo != noError)
      return handBack(errorInfo);

    workBuffer = (HANDLE_WORK_BUFFER)iisCalloc(sizeof(struct WORK_BUFFER), (unsigned int)numChannels);
    if (!workBuffer)
      return iisUtil_ERROR(CDI, "out of memory !");

    errorInfo = CreateWorkBuffers(workBuffer,
                                  hResampler->inBlockPerChan,
                                  hResampler->outBlockPerChan,
                                  hResampler->filtBlockSize,
                                  hResampler->outDelaySize,
                                  numChannels);

    if (errorInfo != noError)
      return handBack(errorInfo);
  }

  hResampler->workBuffer = workBuffer;

  hResampler->numChannels = numChannels;

  hResampler->inputFrequency = inputFrequency;
  hResampler->outputFrequency = outputFrequency;
  hResampler->restLeft = -1;

  *handleResampler = hResampler;

  if (filtParam.coef) {
    iisFree(filtParam.coef);
  }

  return noError;
}

int Resample(struct RESAMPLER* hResampler,
             float* inBuffer,
             float* outBuffer,
             int inSamples)

{
  int outSamples;
  int i, j;
  int numChannels;
  int outChannel, inChannel;
  int L, M, last_l, last_m;

  outSamples = hResampler->outBlockPerChan * hResampler->numChannels;

  numChannels = hResampler->numChannels;
  inChannel = hResampler->inBlockPerChan;
  outChannel = hResampler->outBlockPerChan;

  switch (hResampler->mode) {
    case bypass:
      break;

    case zeroResample:
    case firstResample:

      FillWorkBuffers(hResampler->workBuffer, hResampler->filtBlockSize, inBuffer,
                      inSamples / numChannels, numChannels, hResampler->lastInResampled);
      break;

    case fastResample:

      FillWorkBuffers(hResampler->workBuffer, 0, inBuffer,
                      inSamples / numChannels, numChannels, hResampler->lastInResampled);
      break;
    default:
      break;
  }

  switch (hResampler->mode) {
    case bypass:
      for (i = 0; i < inSamples; i++)
        outBuffer[i] = inBuffer[i];

      outSamples = inSamples;
      break;

    case fastResample:
      if (inChannel > inSamples / numChannels) {
        L = hResampler->hPolyphaseChannel->L;
        M = hResampler->hPolyphaseChannel->M;
        last_l = hResampler->hFastResampler[0]->last_l;
        last_m = hResampler->hFastResampler[0]->last_m;
        inChannel = inSamples / numChannels;

        if (inSamples == 0) {
          for (i = 0; i < numChannels; i++) {
            hResampler->hFastResampler[i]->lastBlockFlag = 1;
          }

          if (last_m != 0) {
            for (i = 0; i < numChannels; i++) {
              for (j = 0; j < M - last_m; j++) {
                hResampler->workBuffer[i].inWorkBuffer[j] = 0;
              }
            }
            inChannel = M - last_m;
            outChannel = (int)((float)last_m / (float)M * (float)L);
            if (((float)last_m / (float)M * (float)L) - (float)outChannel > 0.f) {
              outChannel++;
            }
            outChannel += (L - last_l) % L;
          } else {
          }
        } else {
          outChannel = (inChannel + last_m) / M * L;
        }
        outSamples = outChannel * numChannels;
      }
      for (i = 0; i < numChannels; i++) {
        FastResample(hResampler->hFastResampler[i],
                     hResampler->hPolyphaseChannel,
                     hResampler->workBuffer[i],
                     outChannel, inChannel);
      }

      break;

    case zeroResample:

      if (inChannel != inSamples / numChannels) {
        inChannel = inSamples / numChannels;
        outChannel = (int)((float)inChannel / (float)hResampler->inputFrequency * (float)hResampler->outputFrequency);
      }

      inSamples = 0;
      outSamples = 0;

      ZeroOrderInterpolator(hResampler->hZeroResampler[0],
                            hResampler->workBuffer,
                            hResampler->hPolyphaseChannel,
                            &inChannel, &outChannel, numChannels);

      outSamples += outChannel * numChannels;
      inSamples += inChannel * numChannels;

      break;

    case firstResample:

      if (inChannel != inSamples / numChannels) {
        inChannel = inSamples / numChannels;
        outChannel = (int)((float)inChannel / (float)hResampler->inputFrequency * (float)hResampler->outputFrequency);
      }
      inSamples = 0;
      outSamples = 0;

      FirstOrderInterpolation(hResampler->hFirstResampler[0],
                              hResampler->workBuffer,
                              hResampler->hPolyphaseChannel,
                              &inChannel, &outChannel, numChannels);

      outSamples += outChannel * numChannels;
      inSamples += inChannel * numChannels;

      break;

    default:
      RSL_PRINT_MSG(stdout, "Unknown resampling mode!\n");
      inSamples = 0;
      outSamples = 0;
  }

  if (hResampler->mode != bypass) {
    CopyWorkBuffer(hResampler->workBuffer, hResampler->outBlockPerChan,
                   hResampler->outDelaySize, numChannels, outBuffer);
  }

  hResampler->lastInResampled = inSamples;
  return outSamples;
}

HANDLE_ERROR_INFO
CreateWorkBuffers(HANDLE_WORK_BUFFER workBuffer, int inBlockPerChan, int outBlockPerChan,
                  int filtBlockSize, int outDelaySize, int numChannels) {
  int chan;
  for (chan = 0; chan < numChannels; chan++) {
    workBuffer[chan].inWorkBuffer = (float*)iisCalloc(sizeof(float), (unsigned int)(inBlockPerChan + filtBlockSize));
    if (!(*workBuffer).inWorkBuffer)
      return iisUtil_ERROR(CDI, "out of memory !");

    workBuffer[chan].outWorkBuffer = (float*)iisCalloc(sizeof(float), (unsigned int)(outBlockPerChan + outDelaySize));
    if (!(*workBuffer).outWorkBuffer)
      return iisUtil_ERROR(CDI, "out of memory !");
  }
  return noError;
}

void FillWorkBuffers(HANDLE_WORK_BUFFER workBuffer,
                     int filtBlockSize, float* inBuffer,
                     int inChannel, int numChannels, int inDoneBefore) {
  int i, chan;

  for (chan = 0; chan < numChannels; chan++) {
    for (i = 0; i < filtBlockSize; i++) {
      workBuffer[chan].inWorkBuffer[i] = workBuffer[chan].inWorkBuffer[i + inDoneBefore / numChannels];
    }
    for (i = 0; i < inChannel; i++) {
      workBuffer[chan].inWorkBuffer[i + filtBlockSize] = inBuffer[numChannels * i + chan];
    }
  }
}

void CopyWorkBuffer(HANDLE_WORK_BUFFER workBuffer, int outBlockPerChan, int outDelaySize,
                    int numChannels, float* outBuffer) {
  int chan, i;

  for (chan = 0; chan < numChannels; chan++) {
    for (i = 0; i < outBlockPerChan; i++)
      outBuffer[numChannels * i + chan] = workBuffer[chan].outWorkBuffer[i];

    for (i = 0; i < outDelaySize; i++)
      workBuffer[chan].outWorkBuffer[i] =
          workBuffer[chan].outWorkBuffer[i + outBlockPerChan];
  }
}

void DeleteWorkBuffer(HANDLE_WORK_BUFFER workBuffer, int numChannels) {
  int i;
  for (i = 0; i < numChannels; i++) {
    iisFree(workBuffer[i].inWorkBuffer);
    iisFree(workBuffer[i].outWorkBuffer);
  }
}

void DeleteResampler(HANDLE_RESAMPLER hResampler) {
  int i;

  if (hResampler) {
    DeletePolyphaseChannel(hResampler->hPolyphaseChannel);

    switch (hResampler->mode) {
      case bypass:
        break;

      case fastResample:

        if (hResampler->hFastResampler) {
          for (i = 0; i < hResampler->numChannels; i++)
            DeleteFastResampler(hResampler->hFastResampler[i]);
          iisFree(hResampler->hFastResampler);
          DeleteWorkBuffer(hResampler->workBuffer, hResampler->numChannels);
          iisFree(hResampler->workBuffer);
        }

        break;

      case zeroResample:
        if (hResampler->hZeroResampler) {
          DeleteZeroOrderInterpolator(hResampler->hZeroResampler[0]);
          iisFree(hResampler->hZeroResampler);
          DeleteWorkBuffer(hResampler->workBuffer, hResampler->numChannels);
          iisFree(hResampler->workBuffer);
        }

        break;

      case firstResample:
        if (hResampler->hFirstResampler) {
          DeleteFirstOrderInterpolator(hResampler->hFirstResampler[0]);
          iisFree(hResampler->hFirstResampler);
          DeleteWorkBuffer(hResampler->workBuffer, hResampler->numChannels);
          iisFree(hResampler->workBuffer);
        }

        break;
    }

    iisFree(hResampler);
  }
}

int getMode(HANDLE_RESAMPLER hR) {
  return hR->mode;
}

int InitResample(struct RESAMPLER** phResampler,
                 int inputSampleRate,
                 int outputSampleRate,
                 int numChannels,
                 double attenuation,
                 double lpfrequency,
                 double bandwidth,
                 int maxFilterLength,
                 int* pinBlockSize,
                 int* poutBlockSize,

                 int type) {
  struct RESAMPLER_PARAM rP;
  HANDLE_ERROR_INFO errorInfo = NULL;

  rP.inputFrequency = inputSampleRate;
  rP.outputFrequency = outputSampleRate;

  if (inputSampleRate == outputSampleRate)
    rP.bypassFlag = true;
  else
    rP.bypassFlag = false;

  if (lpfrequency != 0.0) {
    rP.lowpassFrequency = (float)lpfrequency;
    rP.bandwidth = (float)(bandwidth != 0.0 ? bandwidth : 0.166f * rP.lowpassFrequency);

    rP.lowpassFrequency = myMin(rP.lowpassFrequency, (0.5f * myMin(rP.inputFrequency, rP.outputFrequency)));

    rP.lowpassFrequency += rP.bandwidth;
    rP.bypassFlag = false;
  } else {
    rP.lowpassFrequency = 0.5f * myMin(rP.inputFrequency, rP.outputFrequency);
    rP.bandwidth = (float)(bandwidth != 0.0 ? bandwidth : 0.166f * rP.lowpassFrequency);
  }

  rP.attenuation = (float)attenuation;
  rP.filterLength = maxFilterLength;

  rP.type = type;
  rP.fastConvFlag = true;
  rP.fakeFirstFlag = false;

  if (*pinBlockSize != 0) {
    if (*poutBlockSize != 0) {
      return 1;
    } else {
      rP.inOutAdjust = true;
    }
  } else {
    if (*poutBlockSize != 0) {
      rP.inOutAdjust = false;
    } else {
      return 1;
    }
  }

  rP.minPhaseFlag = false;
  rP.forceOddFlag = true;

  rP.flushFlag = false;
  rP.gain = 1.0f;
  rP.firstUpsamplingFactor = FIRST_UP_FAKTOR;

  errorInfo = CreateResampler(phResampler, &rP, numChannels, pinBlockSize, poutBlockSize);
  if (errorInfo != noError) return 1;

  return 0;
}
