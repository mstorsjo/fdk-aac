
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

#include "iisutillib.h"
#include "polyfilt.h"
#include "kernfunc.h"
#include "iis_fft.h"

void GetDelayElements(int L, int M, int* l0, int* m0);
void GetDelayElements(int L, int M, int* l0, int* m0) {
  int tmp, swapFlag = 0;
  int tmpL0, tmpM0;

  if ((L == 1) || (M == 1)) {
    *l0 = 0;
    *m0 = 0;
    return;
  }

  if (L > M) {
    swapFlag = 1;
    tmp = L;
    L = M;
    M = tmp;
  }

  tmpM0 = 0;

  do {
    tmp = (++tmpM0) * M - 1;
  } while (tmp % L);

  tmpL0 = tmp / L;

  if (swapFlag) {
    tmp = tmpL0;
    tmpL0 = tmpM0;
    tmpM0 = tmp;
  }

  *l0 = tmpL0;
  *m0 = tmpM0;
}

HANDLE_ERROR_INFO
CreateFastResampler(HANDLE_FAST_RESAMPLER* hFastResampler,
                    int L, int M, int polyFilterLen) {
  int l0, m0;
  int l, m;
  int inDelaySize;
  int outDelaySize;
  HANDLE_ERROR_INFO errorInfo;

  *hFastResampler = (HANDLE_FAST_RESAMPLER)iisCalloc(sizeof(struct FAST_RESAMPLER), 1);
  if (!(*hFastResampler))
    return iisUtil_ERROR(CDI, "Out of memory !");

  if (M == 1) {
    l0 = 0;
    m0 = 0;
    inDelaySize = 0;
    outDelaySize = 0;
  } else {
    if (L == 1) {
      l0 = 1;
      m0 = 0;
      inDelaySize = M - 1;
      outDelaySize = 0;
    } else {
      GetDelayElements(L, M, &l0, &m0);
      inDelaySize = (M - 1) * l0;
      outDelaySize = (M - 1) * m0;
    }
  }

  (*hFastResampler)->l0 = l0;
  (*hFastResampler)->m0 = m0;
  (*hFastResampler)->inDelaySize = inDelaySize;
  (*hFastResampler)->outDelaySize = outDelaySize;
  (*hFastResampler)->numConvChannels = L * M;
  (*hFastResampler)->last_l = 0;
  (*hFastResampler)->last_m = 0;
  (*hFastResampler)->inBlockSize = 0;
  (*hFastResampler)->lastBlockFlag = 0;

  (*hFastResampler)->hConv = (HANDLE_CONV_CHAN*)iisCalloc(sizeof(HANDLE_CONV_CHAN*), (unsigned int)(L * M));
  if (!(*hFastResampler)->hConv)
    return iisUtil_ERROR(CDI, "Out of memory !");

  for (m = 0; m < M; m++) {
    for (l = 0; l < L; l++) {
      errorInfo = CreateConvolutionChannel(&((*hFastResampler)->hConv[m * L + l]), polyFilterLen);
      if (errorInfo != noError)
        return handBack(errorInfo);
    }
  }

  (*hFastResampler)->lastBlockOutBuf = (float*)iisCalloc(sizeof(float), (unsigned int)L);
  if (!(*hFastResampler)->lastBlockOutBuf)
    return iisUtil_ERROR(CDI, "out of memory !");

  (*hFastResampler)->lastBlockInBuf = (float*)iisCalloc(sizeof(float), (unsigned int)M);
  if (!(*hFastResampler)->lastBlockInBuf)
    return iisUtil_ERROR(CDI, "out of memory !");

  return noError;
}

void FastResample(HANDLE_FAST_RESAMPLER hResampleChannel,
                  const HANDLE_POLYPHASE hPolyphaseChannel,
                  const struct WORK_BUFFER workBufferChan,
                  int outBlockSize,
                  int inBlockSize) {
  float *inBuffer, *outBuffer;
  int L, M;
  int l, m, k, j;
  int upsamplePath, downsamplePath;
  int i, index;
  int inIndex;
  int inOffset, outOffset;
  int transformLength;
  int filtBlockSize = 0;
  int samplesValid = 0;
  int useNoFFT = 0;
  int polyFilterLen;
  int outFiltBufLen;
  float* inFiltBuf;
  float** outFiltBuf;
  float* lastBlockOutBuf;
  float* lastBlockInBuf;
  float* overlapBuf;
  HANDLE_FIR_FILTER* hFIRFilter;

  L = hPolyphaseChannel->L;
  M = hPolyphaseChannel->M;
  filtBlockSize = hPolyphaseChannel->filtBlockSize;
  upsamplePath = hResampleChannel->last_l;
  downsamplePath = hResampleChannel->last_m;

  inFiltBuf = hPolyphaseChannel->inFiltBuf;
  outFiltBuf = hPolyphaseChannel->outFiltBuf;
  hFIRFilter = hPolyphaseChannel->hPolyFilters;
  lastBlockOutBuf = hResampleChannel->lastBlockOutBuf;
  lastBlockInBuf = hResampleChannel->lastBlockInBuf;
  transformLength = hFIRFilter[0]->filterLengthFFT;
  polyFilterLen = hResampleChannel->hConv[0]->overlapLength;

  inBuffer = workBufferChan.inWorkBuffer;
  outBuffer = workBufferChan.outWorkBuffer;

  inOffset = 0;
  outOffset = 0;

  hResampleChannel->inBlockSize = inBlockSize;

  if (upsamplePath) {
    for (k = 0; k < ((L - upsamplePath) % L); k++) {
      outBuffer[k] = lastBlockOutBuf[k];
    }
  }

  if ((polyFilterLen < 0) && ((hResampleChannel->inBlockSize + L) / M + 1 < (transformLength - 2 * polyFilterLen))) {
    useNoFFT = 1;
  }

  while ((inOffset) < hResampleChannel->inBlockSize) {
    inIndex = 0;

    int Max_index = ((hResampleChannel->inBlockSize + downsamplePath) / M) * M;
    for (m = 0; m < M; m++) {
      i = 0;
      j = 0;
      index = inIndex + inOffset;

      while ((i < filtBlockSize) && ((index - downsamplePath) < hResampleChannel->inBlockSize)) {
        if (index < Max_index) {
          if ((index - downsamplePath) >= 0) {
            inFiltBuf[i + useNoFFT * (polyFilterLen)] = inBuffer[index - downsamplePath];
          } else {
            inFiltBuf[i + useNoFFT * (polyFilterLen)] = lastBlockInBuf[m];
          }
          j = i + 1;
        } else {
          if ((index - downsamplePath) >= 0) {
            lastBlockInBuf[m] = inBuffer[index - downsamplePath];
          }
        }
        if (index == hResampleChannel->inBlockSize - 1) {
          hResampleChannel->last_m = (hResampleChannel->last_m + m + 1) % M;
        }
        i++;
        index += M;
      }
      samplesValid = j;

      if (samplesValid) {
        if (!useNoFFT) {
          ForwardTransform(inFiltBuf, samplesValid,
                           hFIRFilter[0]->filterLengthFFT

                           ,
                           hPolyphaseChannel->hIisFft_R);
        } else {
          for (i = 0; i < polyFilterLen; i++) {
            inFiltBuf[i] = 0;
            inFiltBuf[polyFilterLen + samplesValid + i] = 0;
          }
        }

        for (l = 0; l < L; l++) {
          if (!useNoFFT) {
            FastConvolution(hFIRFilter[m * L + l],
                            inFiltBuf, outFiltBuf[0]);

            outFiltBufLen = transformLength;
          } else {
            for (i = 0; i < samplesValid + polyFilterLen; i++) {
              outFiltBuf[0][i] = SingleFIRFilterMod(hFIRFilter[m * L + l],
                                                    &inFiltBuf[i + polyFilterLen]);
            }

            overlapBuf = hResampleChannel->hConv[m * L + l]->overlapBuffer;
            for (i = 0; i < polyFilterLen; i++) {
              outFiltBuf[0][i] += overlapBuf[i];
              if (samplesValid + i >= polyFilterLen) {
                overlapBuf[i] = outFiltBuf[0][samplesValid + i];
              } else {
                overlapBuf[i] = outFiltBuf[0][samplesValid + i] + overlapBuf[samplesValid + i];
              }
            }
            outFiltBufLen = samplesValid;
          }

          if (m == 0) {
            for (i = 0; i < outFiltBufLen; i++) {
              outFiltBuf[l + 1][i] = outFiltBuf[0][i];
            }
          } else {
            for (i = 0; i < outFiltBufLen; i++) {
              outFiltBuf[l + 1][i] += outFiltBuf[0][i];
            }
          }

          if (m == M - 1) {
            if (!useNoFFT) {
              InverseTransform(hResampleChannel->hConv[l],
                               outFiltBuf[l + 1],
                               samplesValid, hPolyphaseChannel->hIisFft_RBwd);
            }
            i = 0;
            index = l + ((L - upsamplePath) % L);
            index += outOffset;

            while (i < samplesValid) {
              if (index < outBlockSize) {
                outBuffer[index] = outFiltBuf[l + 1][i];
              } else {
                if (l - hResampleChannel->last_l >= 0)
                  lastBlockOutBuf[l - hResampleChannel->last_l] = outFiltBuf[l + 1][i];
              }
              i++;
              if (index == outBlockSize - 1) {
                hResampleChannel->last_l = (l + 1) % L;
              }
              index += L;
            }
          }
        }
      }
      inIndex++;
    }
    inOffset += samplesValid * M;
    outOffset += samplesValid * L;
    if ((hResampleChannel->last_m - downsamplePath) > 0) {
      inOffset += (hResampleChannel->last_m - downsamplePath);
    }
  }

  if (hResampleChannel->lastBlockFlag == 1) {
    hResampleChannel->last_l = 0;
    hResampleChannel->last_m = 0;
  }
}

void DeleteFastResampler(HANDLE_FAST_RESAMPLER hFastResampler) {
  int i;

  if (hFastResampler) {
    if (hFastResampler->hConv) {
      for (i = 0; i < hFastResampler->numConvChannels; i++)
        DeleteConvolutionChannel(hFastResampler->hConv[i]);
      iisFree(hFastResampler->hConv);
    }

    if (hFastResampler->lastBlockOutBuf)
      iisFree(hFastResampler->lastBlockOutBuf);
    if (hFastResampler->lastBlockInBuf)
      iisFree(hFastResampler->lastBlockInBuf);

    iisFree(hFastResampler);
  }
}

HANDLE_ERROR_INFO
CreateZeroOrderInterpolator(HANDLE_ZERO_RESAMPLER* hResampleChannel,
                            int L, int M, int inOutAdjust) {
  *hResampleChannel = (HANDLE_ZERO_RESAMPLER)iisCalloc(sizeof(struct ZERO_RESAMPLER), 1);
  if (!*hResampleChannel)
    return iisUtil_ERROR(CDI, "out of memory");

  (*hResampleChannel)->quotient = M / L;
  (*hResampleChannel)->remainder = M % L;
  (*hResampleChannel)->inOutAdjust = inOutAdjust;

  return noError;
}

void DeleteZeroOrderInterpolator(HANDLE_ZERO_RESAMPLER hResampleChannel) {
  if (hResampleChannel) {
    iisFree(hResampleChannel);
  }
}

void ZeroOrderInterpolator(HANDLE_ZERO_RESAMPLER hResampleChannel,
                           const HANDLE_WORK_BUFFER workBuffer,
                           const HANDLE_POLYPHASE hPolyphaseChannel,
                           int* inBlockPerChan, int* outBlockPerChan,
                           int numChannels)

{
  int inIndex;
  int outIndex;
  int filtIndex;
  int inDelaySize;
  int chan;
  int tmp_inIndex;

  HANDLE_FIR_FILTER* hFIRFilter;

  inDelaySize = hPolyphaseChannel->filtBlockSize;
  hFIRFilter = hPolyphaseChannel->hPolyFilters;

  filtIndex = hResampleChannel->filtIndex;
  outIndex = 0;
  inIndex = hResampleChannel->inSampleIndex + inDelaySize;
  tmp_inIndex = inIndex;

  while ((inIndex < *inBlockPerChan + inDelaySize) && (outIndex < *outBlockPerChan)) {
    for (chan = 0; chan < numChannels; chan++) {
      workBuffer[chan].outWorkBuffer[outIndex] =
          SingleFIRFilterMod(hFIRFilter[filtIndex], &workBuffer[chan].inWorkBuffer[inIndex]);
    }

    outIndex++;

    inIndex += hResampleChannel->quotient;
    filtIndex += hResampleChannel->remainder;
    tmp_inIndex = inIndex;

    if (filtIndex >= hPolyphaseChannel->L) {
      filtIndex -= hPolyphaseChannel->L;
      inIndex++;
    }
  }

  if (hResampleChannel->inOutAdjust) {
    hResampleChannel->inSampleIndex = inIndex - inDelaySize - *inBlockPerChan;
    inIndex -= hResampleChannel->inSampleIndex;

  } else {
    if (tmp_inIndex != inIndex) {
      hResampleChannel->inSampleIndex = inIndex - tmp_inIndex - 1;
      inIndex = tmp_inIndex + 1;
    } else {
      hResampleChannel->inSampleIndex = 0;
    }
  }

  hResampleChannel->filtIndex = filtIndex;
  *inBlockPerChan = inIndex - inDelaySize;
  *outBlockPerChan = outIndex;
}

HANDLE_ERROR_INFO
CreateFirstOrderInterpolator(HANDLE_FIRST_RESAMPLER* hResampleChannel,
                             int L, int M, int fakeFirstFlag, int inOutAdjust,
                             int firstUpsamplingFactor) {
  int Fin, Fout;

  *hResampleChannel = (HANDLE_FIRST_RESAMPLER)iisCalloc(sizeof(struct FIRST_RESAMPLER), 1);
  if (!*hResampleChannel)
    return iisUtil_ERROR(CDI, "out of memory");

  (*hResampleChannel)->upsamplingFactor = firstUpsamplingFactor;

  Fin = M * firstUpsamplingFactor;
  Fout = L;
  L = Fout;
  M = Fin;

  (*hResampleChannel)->quotient = ((int)(M / L)) % firstUpsamplingFactor;
  (*hResampleChannel)->remainder = M % L;
  (*hResampleChannel)->inQuotient = (int)(((int)(M / L)) / firstUpsamplingFactor);

  (*hResampleChannel)->fakeFirstFlag = fakeFirstFlag;

  (*hResampleChannel)->inOutAdjust = inOutAdjust;

  (*hResampleChannel)->L = L;
  (*hResampleChannel)->M = M;
  (*hResampleChannel)->distance = 0;
  (*hResampleChannel)->filtIndex = 0;
  (*hResampleChannel)->inSampleIndex = 0;

  return noError;
}

void DeleteFirstOrderInterpolator(HANDLE_FIRST_RESAMPLER hResampleChannel) {
  if (hResampleChannel)
    iisFree(hResampleChannel);
}

void FirstOrderInterpolation(HANDLE_FIRST_RESAMPLER hFirstResampler,
                             const HANDLE_WORK_BUFFER workBuffer,
                             const HANDLE_POLYPHASE hPolyphaseChannel,
                             int* inBlockPerChan, int* outBlockPerChan,
                             int numChannels) {
  float L_inv;
  float lambdaInt;
  int inDelaySize;

  HANDLE_FIR_FILTER* hFIRFilter;

  float x1;
  float x2;

  int chan;
  int distance;
  int filtIndex;
  int outIndex;
  int inIndex;
  int inSample;
  int tmp_inIndex = 0;

  inDelaySize = hPolyphaseChannel->filtBlockSize;
  hFIRFilter = hPolyphaseChannel->hPolyFilters;

  distance = hFirstResampler->distance;
  filtIndex = hFirstResampler->filtIndex;
  inSample = hFirstResampler->inSampleIndex;

  inIndex = inDelaySize + inSample;
  outIndex = 0;

  L_inv = 1.0f / (float)(hFirstResampler->L);

  while ((inIndex < *inBlockPerChan + inDelaySize) && (outIndex < *outBlockPerChan)) {
    if (hFirstResampler->fakeFirstFlag) {
      for (chan = 0; chan < numChannels; chan++) {
        workBuffer[chan].outWorkBuffer[outIndex] = SingleFIRFilterMod(hFIRFilter[filtIndex], &workBuffer[chan].inWorkBuffer[inIndex]);
      }
    } else {
      lambdaInt = L_inv * ((float)(distance));
      if (filtIndex != 0) {
        for (chan = 0; chan < numChannels; chan++) {
          x2 = SingleFIRFilterMod(hFIRFilter[filtIndex], &workBuffer[chan].inWorkBuffer[inIndex]);
          x1 = SingleFIRFilterMod(hFIRFilter[filtIndex - 1], &workBuffer[chan].inWorkBuffer[inIndex]);
          workBuffer[chan].outWorkBuffer[outIndex] = (x2 - x1) * lambdaInt + x1;
        }

      } else {
        for (chan = 0; chan < numChannels; chan++) {
          x2 = SingleFIRFilterMod(hFIRFilter[0], &workBuffer[chan].inWorkBuffer[inIndex]);
          x1 = SingleFIRFilterMod(hFIRFilter[hFirstResampler->upsamplingFactor - 1], &workBuffer[chan].inWorkBuffer[inIndex - 1]);
          workBuffer[chan].outWorkBuffer[outIndex] = (x2 - x1) * lambdaInt + x1;
        }
      }
    }

    outIndex++;
    tmp_inIndex = inIndex;
    distance += hFirstResampler->remainder;
    filtIndex += hFirstResampler->quotient;
    inIndex += hFirstResampler->inQuotient;

    if (distance >= hFirstResampler->L) {
      distance -= hFirstResampler->L;
      filtIndex++;
    }

    if (filtIndex >= hFirstResampler->upsamplingFactor) {
      filtIndex -= hFirstResampler->upsamplingFactor;
      inIndex++;
    }
  }

  if (hFirstResampler->inOutAdjust) {
    hFirstResampler->inSampleIndex = inIndex - inDelaySize - *inBlockPerChan;

    inIndex -= hFirstResampler->inSampleIndex;

  } else {
    if (tmp_inIndex != inIndex) {
      hFirstResampler->inSampleIndex = inIndex - tmp_inIndex - 1;
      inIndex = tmp_inIndex + 1;
    } else {
      hFirstResampler->inSampleIndex = 0;
    }
  }

  hFirstResampler->filtIndex = filtIndex;
  hFirstResampler->distance = distance;

  *inBlockPerChan = inIndex - inDelaySize;
  *outBlockPerChan = outIndex;
}
