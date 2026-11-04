
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

#include "polyphase.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "helpfunc.h"
#include "resample.h"
#include "rsstruct.h"

#include "../share/rsl_dbgprint.h"

typedef enum {
  ONESTEP_FIXEDOUT = 2,
  ONESTEP_FIXEDIN = 3,
  TWOSTEP_FIXEDOUT = 12,
  TWOSTEP_FIXEDIN = 13
} POLYPHASE_MODE;

enum {
  MAXFILTLEN = 0,

  RATIO_SR = 5,
  DEFAULT_INTERM_SR = 50000,

  MAXFILTLEN2 = 0,

  DEFAULT_LPFREQUENCY = 0,
  DEFAULT_BANDWIDTH = 0,
  DEFAULT_FILTERTAPS = 0,
  DEFAULT_TYPE = 0x07,
  DEFAULT_MODE = 0
};

#define DEFAULT_ATTENUATION 90.0

typedef struct tag_all_polyphase {
  HANDLE_ERROR_INFO hErrorInfo;
  HANDLE_RESAMPLER hResampler;
  HANDLE_RESAMPLER hResampler2;
  struct RESAMPLER_PARAM filtPolyParam;
  POLYPHASE_MODE mode;
  int type;
  int inBlockSize;
  int midBlockSize;
  int outBlockSize;
  int delta;
  int delta2;
  float *inBuffer;
  float *outBuffer;
  float *midBuffer;
  short *inShortBuf;
  short *outShortBuf;
} ALL_POLYPHASE;

int PolyphaseConstruct(HANDLE_ALL_POLYPHASE *hAllPolyphase,
                       const int sampleRateInput,
                       const int sampleRateOutput,
                       const int maxNumberOfChannels,
                       const int maxNumberOfDataInputSamples,
                       const int maxNumberOfDataOutputSamples,
                       const SRTYPE samplerateMode,
                       const SRSUBTYPE resamplerSubType,
                       PolyphaseFilterParameters *filtPolyInterfaceParam,
                       int *numberOfSamplesInput,
                       int *numberOfSamplesOutput,
                       float **dataInput,
                       float **dataOutput) {
  int sampleRateInterm;
  HANDLE_ALL_POLYPHASE h;

  h = *hAllPolyphase = (HANDLE_ALL_POLYPHASE)iisCalloc(sizeof(ALL_POLYPHASE), 1);

  h->filtPolyParam.attenuation = filtPolyInterfaceParam->attenuation;
  h->filtPolyParam.lowpassFrequency = filtPolyInterfaceParam->lowpassFrequency;
  h->filtPolyParam.bandwidth = filtPolyInterfaceParam->bandwidth;
  h->filtPolyParam.gain = filtPolyInterfaceParam->gain;
  sampleRateInterm = filtPolyInterfaceParam->sampleRateInterm;
  if (h->filtPolyParam.attenuation == 0.0f)
    h->filtPolyParam.attenuation = DEFAULT_ATTENUATION;
  if (h->filtPolyParam.lowpassFrequency == 0.0f)
    h->filtPolyParam.lowpassFrequency = DEFAULT_LPFREQUENCY;
  if (h->filtPolyParam.bandwidth == 0.0f)
    h->filtPolyParam.bandwidth = DEFAULT_BANDWIDTH;
  if (sampleRateInterm == 0)
    sampleRateInterm = DEFAULT_INTERM_SR;

  h->type = 0;
  switch (samplerateMode) {
    case POLYPHASETECHNIQUE:
      h->type = POLYPHASETECHNIQUE_FAST_ZERO_FIRST - POLYPHASETECHNIQUE * 10;
      break;
    case POLYPHASETECHNIQUE_BYPASS:
    case POLYPHASETECHNIQUE_FAST:
    case POLYPHASETECHNIQUE_ZERO:
    case POLYPHASETECHNIQUE_FAST_ZERO:
    case POLYPHASETECHNIQUE_FIRST:
    case POLYPHASETECHNIQUE_FAST_FIRST:
    case POLYPHASETECHNIQUE_ZERO_FIRST:
    case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
      h->type = samplerateMode - POLYPHASETECHNIQUE * 10;
      break;
    default:
      break;
  }
  h->mode = ONESTEP_FIXEDIN;
  switch (resamplerSubType) {
    case FIXEDINPUTBLOCKSIZE:
      h->mode = ONESTEP_FIXEDIN;
      break;
    case FIXEDOUTPUTBLOCKSIZE:
      h->mode = ONESTEP_FIXEDOUT;
      break;
    case FIXEDINPUTBLOCKSIZE_ONESTEP:
      h->mode = ONESTEP_FIXEDIN;
      break;
    case FIXEDOUTPUTBLOCKSIZE_ONESTEP:
      h->mode = ONESTEP_FIXEDOUT;
      break;
    case FIXEDINPUTBLOCKSIZE_TWOSTEPUPSAMPLING:
      h->mode = TWOSTEP_FIXEDIN;
      break;
    case FIXEDOUTPUTBLOCKSIZE_TWOSTEPDOWNSAMPLING:
      h->mode = TWOSTEP_FIXEDOUT;
      break;
    case AUTOMATIC_SELECTION:

      if (sampleRateOutput > sampleRateInterm)
        h->mode = ONESTEP_FIXEDOUT;
      if (sampleRateInput > sampleRateInterm)
        h->mode = ONESTEP_FIXEDIN;
      if ((sampleRateOutput > sampleRateInterm) && (sampleRateInput < sampleRateInterm))
        h->mode = TWOSTEP_FIXEDOUT;
      if ((sampleRateInput > sampleRateInterm) && (sampleRateOutput < sampleRateInterm))
        h->mode = TWOSTEP_FIXEDIN;
      break;
    default:
      break;
  }

  h->inBlockSize = maxNumberOfDataInputSamples;
  h->outBlockSize = maxNumberOfDataOutputSamples;
  if (h->mode == ONESTEP_FIXEDOUT)
    h->inBlockSize = 0;
  if (h->mode == ONESTEP_FIXEDIN)
    h->outBlockSize = 0;
  if ((h->mode == ONESTEP_FIXEDOUT) || (h->mode == ONESTEP_FIXEDIN)) {
    if (InitResample(&h->hResampler,
                     sampleRateInput,
                     sampleRateOutput,
                     maxNumberOfChannels,
                     h->filtPolyParam.attenuation,
                     h->filtPolyParam.lowpassFrequency,
                     h->filtPolyParam.bandwidth,
                     MAXFILTLEN,
                     &h->inBlockSize,
                     &h->outBlockSize,
                     h->type)) return (1);
    if (CreateBuffers(&h->inBuffer,
                      &h->inShortBuf,
                      &h->outBuffer,
                      &h->outShortBuf,
                      h->inBlockSize,
                      h->outBlockSize) != noError) return (1);
  }
  if (h->mode == TWOSTEP_FIXEDOUT) {
    h->inBlockSize = 0;
    h->midBlockSize = 0;

    h->outBlockSize = maxNumberOfDataOutputSamples * RATIO_SR;

    if (InitResample(&h->hResampler2,
                     sampleRateInterm,
                     sampleRateOutput,
                     maxNumberOfChannels,
                     h->filtPolyParam.attenuation,
                     .0,
                     h->filtPolyParam.bandwidth,
                     MAXFILTLEN2,
                     &h->midBlockSize,
                     &h->outBlockSize,
                     h->type)) return (1);
    if (InitResample(&h->hResampler,
                     sampleRateInput,
                     sampleRateInterm,
                     maxNumberOfChannels,
                     h->filtPolyParam.attenuation,
                     h->filtPolyParam.lowpassFrequency,
                     h->filtPolyParam.bandwidth,
                     MAXFILTLEN,
                     &h->inBlockSize,
                     &h->midBlockSize,
                     (h->type & ~FASTRES))) return (1);
    if (CreateBuffers(&h->inBuffer,
                      &h->inShortBuf,
                      &h->outBuffer,
                      &h->outShortBuf,
                      h->inBlockSize,
                      h->outBlockSize) != noError) return (1);
    h->midBuffer = (float *)iisCalloc(sizeof(float), (unsigned int)h->midBlockSize);
    if (!h->midBuffer)
      return (1);
  }
  if (h->mode == TWOSTEP_FIXEDIN) {
    h->inBlockSize = maxNumberOfDataInputSamples * RATIO_SR;
    h->midBlockSize = 0;
    h->outBlockSize = 0;

    if (InitResample(&h->hResampler2,
                     sampleRateInput,
                     sampleRateInterm,
                     maxNumberOfChannels,
                     h->filtPolyParam.attenuation,
                     .0,
                     h->filtPolyParam.bandwidth,
                     MAXFILTLEN2,
                     &h->inBlockSize,
                     &h->midBlockSize,
                     h->type)) return (1);
    if (InitResample(&h->hResampler,
                     sampleRateInterm,
                     sampleRateOutput,
                     maxNumberOfChannels,
                     h->filtPolyParam.attenuation,
                     h->filtPolyParam.lowpassFrequency,
                     h->filtPolyParam.bandwidth,
                     MAXFILTLEN,
                     &h->midBlockSize,
                     &h->outBlockSize,
                     (h->type & ~FASTRES))) return (1);
    if (CreateBuffers(&h->inBuffer,
                      &h->inShortBuf,
                      &h->outBuffer,
                      &h->outShortBuf,
                      h->inBlockSize,
                      h->outBlockSize) != noError) return (1);
    h->midBuffer = (float *)iisCalloc(sizeof(float), (unsigned int)h->midBlockSize);
    if (!h->midBuffer)
      return (1);
  }

  *numberOfSamplesInput = h->inBlockSize;
  *numberOfSamplesOutput = h->outBlockSize;
  *dataInput = h->inBuffer;
  *dataOutput = h->outBuffer;
  return (0);
}

int PolyphaseDestruct(HANDLE_ALL_POLYPHASE h) {
  if (h->midBuffer)
    iisFree(h->midBuffer);
  if (h->hResampler2)
    DeleteResampler(h->hResampler2);
  DeleteBuffers(h->inBuffer,
                h->inShortBuf,
                h->outBuffer,
                h->outShortBuf);
  DeleteResampler(h->hResampler);
  iisFree(h);
  return (0);
}

int PolyphaseResample(HANDLE_ALL_POLYPHASE h,
                      int numberOfSamplesInput,
                      int *numberOfSamplesOutput) {
  int outResampled;
  int i;

  if ((h->mode == ONESTEP_FIXEDOUT) || (h->mode == ONESTEP_FIXEDIN))
    *numberOfSamplesOutput =
        Resample(h->hResampler,
                 h->inBuffer,
                 h->outBuffer,
                 numberOfSamplesInput + h->delta);
  if (h->mode == TWOSTEP_FIXEDIN) {
    outResampled =
        Resample(h->hResampler2,
                 h->inBuffer,
                 h->midBuffer,
                 numberOfSamplesInput);

    *numberOfSamplesOutput =
        Resample(h->hResampler,
                 h->midBuffer,
                 h->outBuffer,
                 outResampled);
  }
  if (h->mode == TWOSTEP_FIXEDOUT) {
    outResampled =
        Resample(h->hResampler,
                 h->inBuffer,
                 h->midBuffer,
                 numberOfSamplesInput + h->delta);
    *numberOfSamplesOutput =
        Resample(h->hResampler2,
                 h->midBuffer,
                 h->outBuffer,
                 outResampled + h->delta2);
    if (h->hResampler2->lastInResampled != outResampled + h->delta2) {
      for (i = 0; i < (outResampled + h->delta2 - h->hResampler2->lastInResampled); i++)
        h->midBuffer[i] = h->midBuffer[h->hResampler2->lastInResampled + i];
      h->delta2 = i;

      h->hResampler->outBlockPerChan = (h->midBlockSize - h->delta2) / h->hResampler->numChannels;
    } else {
      h->delta2 = 0;
      h->hResampler->outBlockPerChan = h->midBlockSize / h->hResampler->numChannels;
    }
  }
  if ((h->mode == ONESTEP_FIXEDOUT) || (h->mode == TWOSTEP_FIXEDOUT)) {
    if (h->hResampler->lastInResampled != numberOfSamplesInput + h->delta) {
      for (i = 0; i < (numberOfSamplesInput + h->delta - h->hResampler->lastInResampled); i++)
        h->inBuffer[i] = h->inBuffer[h->hResampler->lastInResampled + i];
      h->delta = i;
    } else
      h->delta = 0;
  }
  return (0);
}

int PolyphasePreMain(HANDLE_ALL_POLYPHASE h,
                     float **dataInput,
                     int *numberOfSamplesInput) {
  int i;
  int L = 0;
  int M = 0;
  int last_l;
  int last_m;
  int fixedNumberOfSamplesOutput;
  if (h->hResampler->hPolyphaseChannel) {
    L = h->hResampler->hPolyphaseChannel->L;
    M = h->hResampler->hPolyphaseChannel->M;
  } else {
    return 1;
  }
  switch (h->mode) {
    case ONESTEP_FIXEDIN:
      *numberOfSamplesInput = h->hResampler->fixedNumberOfSamplesInput;
      if (h->hResampler->hFastResampler) {
        last_m = h->hResampler->hFastResampler[0]->last_m;
        h->hResampler->outBlockPerChan = ((*numberOfSamplesInput / h->hResampler->numChannels + last_m) / M) * L;
        h->outBlockSize = h->hResampler->outBlockPerChan * h->hResampler->numChannels;
      }
      break;
    case ONESTEP_FIXEDOUT:
      if (h->hResampler->hFastResampler) {
        fixedNumberOfSamplesOutput = h->hResampler->fixedNumberOfSamplesOutput;
        last_l = h->hResampler->hFastResampler[0]->last_l;
        last_m = h->hResampler->hFastResampler[0]->last_m;
        *numberOfSamplesInput = (fixedNumberOfSamplesOutput / h->hResampler->numChannels - (L - last_l) % L) / L;
        if ((fixedNumberOfSamplesOutput / h->hResampler->numChannels - (L - last_l)) % L)
          (*numberOfSamplesInput)++;
        (*numberOfSamplesInput) *= M;
        (*numberOfSamplesInput) -= last_m;
        for (i = 0; i < h->hResampler->numChannels; i++) {
          h->hResampler->hFastResampler[i]->inBlockSize = *numberOfSamplesInput;
        }
        h->hResampler->inBlockPerChan = *numberOfSamplesInput;
        *numberOfSamplesInput *= h->hResampler->numChannels;
      } else {
        *numberOfSamplesInput = h->inBlockSize - h->delta;
        *dataInput = h->inBuffer + h->delta;
      }
      break;
    case TWOSTEP_FIXEDOUT:

      *numberOfSamplesInput = h->inBlockSize - h->delta;
      *dataInput = h->inBuffer + h->delta;
      break;
    default:
      break;
  }
  return (0);
}

float PolyphaseGetDelayFract(HANDLE_ALL_POLYPHASE h) {
  float delay = 0;

  if (h->hResampler2) {
    delay = h->hResampler2->outputDelayFrac;

    delay = (delay * ((float)h->hResampler2->outputFrequency / (float)h->hResampler->outputFrequency));
  }

  if (h->hResampler) {
    delay += h->hResampler->outputDelayFrac;
  }

  return delay;
}

int PolyphaseGetDelay(HANDLE_ALL_POLYPHASE h) {
  int delay = 0;

  if (h->hResampler2) {
    delay = h->hResampler2->outputDelay;

    delay = (int)((float)delay * ((float)h->hResampler2->outputFrequency / (float)h->hResampler->outputFrequency));
  }

  if (h->hResampler) {
    delay += h->hResampler->outputDelay;
  }

  return delay;
}

void PolyphaseCopyOverlapBuf(HANDLE_ALL_POLYPHASE hAllPolyphaseSrc, HANDLE_ALL_POLYPHASE hAllPolyphaseDst) {
  copyFLOAT((*((*(hAllPolyphaseSrc->hResampler->hFastResampler))->hConv))->overlapBuffer,
            (*((*(hAllPolyphaseDst->hResampler->hFastResampler))->hConv))->overlapBuffer,
            (*((*(hAllPolyphaseSrc->hResampler->hFastResampler))->hConv))->overlapLength);

  hAllPolyphaseDst->hResampler->lastInResampled = hAllPolyphaseSrc->hResampler->lastInResampled;
}
