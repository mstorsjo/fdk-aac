
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
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "resamplelib.h"
#include "share/rsl_dbgprint.h"

#include "polyphase/polyphase.h"
#include "iisutillib.h"
#include "mathlib.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct tag_resamplelib {
  int sampleRateInput;
  int sampleRateOutput;
  int maxNumberOfChannels;
  int maxNumberOfDataInputSamples;
  int maxNumberOfDataOutputSamples;
  int numberOfChannels;
  int numberOfSamplesInput;
  int numberOfSamplesOutput;
  int numberOfSamplesInputWanted;
  int numberOfSamplesInputRecomm;
  int endCondition;
  SRTYPE samplerateMode;
  SRSUBTYPE resamplerSubType;
  float* dataInput;
  float* dataOutput;

  HANDLE_ALL_POLYPHASE hAllPolyphase;
  PolyphaseFilterParameters filtPolyInterfaceParam;
} T_RESAMPLELIB;

void ResamplerVerbosity(int verboseLevelLocal) {
}

int ResamplerConstruct(
    HANDLE_RESAMPLELIB* hSrConversion,
    unsigned int sampleRateInput,
    unsigned int sampleRateOutput,
    unsigned int maxNumberOfChannels,
    unsigned int maxNumberOfDataInputSamples,
    unsigned int maxNumberOfDataOutputSamples,
    SRTYPE samplerateMode,
    SRSUBTYPE resamplerSubType,
    GeneralType* generalData) {
  int returnValue = 0;
  HANDLE_RESAMPLELIB h;

  if (hSrConversion == NULL) {
    RSL_PRINT_MSG(stderr, "Invalid handle hSrConversion.\n");
    return (2);
  }

  h = *hSrConversion = (HANDLE_RESAMPLELIB)iisCalloc(sizeof(*h), 1);
  if (h == NULL) {
    RSL_PRINT_MSG(stderr, "Out of memory (HANDLE_RESAMPLELIB) \n");
    return (2);
  }

  if ((maxNumberOfDataInputSamples % maxNumberOfChannels != 0) ||
      (maxNumberOfDataOutputSamples % maxNumberOfChannels != 0)) {
    RSL_PRINT_MSG(stderr, "In multichannel applications the maximum number of in/output samples\n has to be a multiple of the number of channels.\n");
    return (1);
  }

  h->sampleRateInput = sampleRateInput;
  h->sampleRateOutput = sampleRateOutput;
  h->maxNumberOfChannels = maxNumberOfChannels;
  h->numberOfChannels = maxNumberOfChannels;
  h->numberOfSamplesInput = 0;
  h->numberOfSamplesOutput = 0;
  h->numberOfSamplesInputWanted = (int)(((float)sampleRateInput / (float)sampleRateOutput) * (maxNumberOfDataOutputSamples / maxNumberOfChannels)) * maxNumberOfChannels;
  if (h->numberOfSamplesInputWanted >= 0) {
    h->numberOfSamplesInputWanted = min((unsigned int)h->numberOfSamplesInputWanted, maxNumberOfDataInputSamples);
    h->numberOfSamplesInputRecomm = h->numberOfSamplesInputWanted;
    if (resamplerSubType == FIXEDINPUTBLOCKSIZE) {
      h->maxNumberOfDataInputSamples = maxNumberOfDataInputSamples;
    } else {
      h->maxNumberOfDataInputSamples = h->numberOfSamplesInputWanted + maxNumberOfChannels;
    }
  } else {
    RSL_PRINT_MSG(stderr, "Invalid number of input samples wanted.\n");
    return (1);
  }

  h->maxNumberOfDataOutputSamples = maxNumberOfDataOutputSamples;
  h->endCondition = 0;
  h->samplerateMode = samplerateMode;
  h->resamplerSubType = resamplerSubType;
  h->dataInput = NULL;
  h->dataOutput = NULL;
  h->hAllPolyphase = NULL;
  switch (h->samplerateMode) {
    case BYPASSDATA: {
      int nSamplesinBuffer = min(h->maxNumberOfDataInputSamples, h->maxNumberOfDataOutputSamples);

      if (h->sampleRateInput != h->sampleRateOutput) {
        RSL_PRINT_MSG(stderr, "ResamplerConstruct: WARNING: Bypassing data of differing samplerates: %d != %d \n", h->sampleRateInput, h->sampleRateOutput);
      }

      h->dataInput = (float*)iisCalloc(nSamplesinBuffer, sizeof(float));
      if (!h->dataInput)
        RSL_PRINT_MSG(stderr, "ResamplerConstruct: Could not allocate internal buffer.\n");

      h->dataOutput = h->dataInput;
      h->numberOfSamplesInput = nSamplesinBuffer;
      h->numberOfSamplesOutput = nSamplesinBuffer;
      break;
    }
    case POLYPHASETECHNIQUE:
    case POLYPHASETECHNIQUE_BYPASS:
    case POLYPHASETECHNIQUE_FAST:
    case POLYPHASETECHNIQUE_ZERO:
    case POLYPHASETECHNIQUE_FAST_ZERO:
    case POLYPHASETECHNIQUE_FIRST:
    case POLYPHASETECHNIQUE_FAST_FIRST:
    case POLYPHASETECHNIQUE_ZERO_FIRST:
    case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
      switch (resamplerSubType) {
        case AUTOMATIC_SELECTION:
        case FIXEDINPUTBLOCKSIZE:
        case FIXEDOUTPUTBLOCKSIZE:
        case FIXEDINPUTBLOCKSIZE_TWOSTEPUPSAMPLING:
        case FIXEDINPUTBLOCKSIZE_ONESTEP:
        case FIXEDOUTPUTBLOCKSIZE_TWOSTEPDOWNSAMPLING:
        case FIXEDOUTPUTBLOCKSIZE_ONESTEP:
          break;
        default:
          RSL_PRINT_MSG(stderr, "ResamplerConstruct: resamplerSubType %i not supported.\n", resamplerSubType);
          returnValue = 1;
      }
      if (returnValue != 0) break;
      if (generalData)
        memcpy(&h->filtPolyInterfaceParam, &generalData->polyphaseFilterVariables, sizeof(h->filtPolyInterfaceParam));
      else {
        memset(&h->filtPolyInterfaceParam, 0, sizeof(h->filtPolyInterfaceParam));
        h->filtPolyInterfaceParam.gain = 1.0;
      }
      if (PolyphaseConstruct(&h->hAllPolyphase,
                             h->sampleRateInput,
                             h->sampleRateOutput,
                             h->maxNumberOfChannels,
                             h->maxNumberOfDataInputSamples,
                             h->maxNumberOfDataOutputSamples,
                             h->samplerateMode,
                             h->resamplerSubType,
                             &h->filtPolyInterfaceParam,
                             &h->numberOfSamplesInput,
                             &h->numberOfSamplesOutput,
                             &h->dataInput,
                             &h->dataOutput) != 0) {
        RSL_PRINT_MSG(stderr, "PolyphaseConstruct: resampling tool initialization error.\nPolyphaseConstruct: inacceptable samplerate/mode configuration?\n");
        returnValue = 1;
      }
      return (returnValue);
    case MAX_SRTYPE:
      break;
    default:
      RSL_PRINT_MSG(stderr, "ResamplerConstruct: samplerateMode %i not supported.\n", h->samplerateMode);
      returnValue = 1;
  }
  return (returnValue);
}

int ResamplerDestruct(HANDLE_RESAMPLELIB hSrConversion) {
  int returnValue = 0;

  if (hSrConversion != NULL) {
    switch (hSrConversion->samplerateMode) {
      case BYPASSDATA:
        iisFree(hSrConversion->dataInput);
        break;
      case POLYPHASETECHNIQUE:
      case POLYPHASETECHNIQUE_BYPASS:
      case POLYPHASETECHNIQUE_FAST:
      case POLYPHASETECHNIQUE_ZERO:
      case POLYPHASETECHNIQUE_FAST_ZERO:
      case POLYPHASETECHNIQUE_FIRST:
      case POLYPHASETECHNIQUE_FAST_FIRST:
      case POLYPHASETECHNIQUE_ZERO_FIRST:
      case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
        PolyphaseDestruct(hSrConversion->hAllPolyphase);
        break;
      case MAX_SRTYPE:
        break;
      default:
        RSL_PRINT_MSG(stderr, "ResamplerDestruct: samplerateMode %i not supported.\n", hSrConversion->samplerateMode);
        returnValue = 1;
        break;
    }

    ngsFree(hSrConversion);
  }
  hSrConversion = NULL;

  return (returnValue);
}

int ResamplerMain(
    HANDLE_RESAMPLELIB hSrConversion,
    unsigned int* sampleRateInput,
    unsigned int* sampleRateOutput,
    unsigned int* numberOfChannels,
    unsigned int* numberOfSamplesInput,
    unsigned int* numberOfSamplesOutput,
    float** dataInput,
    float** dataOutput) {
  int returnValue = 0;

  (void)sampleRateInput;
  (void)sampleRateOutput;
  (void)numberOfChannels;

  if (hSrConversion != NULL) {
    switch (hSrConversion->samplerateMode) {
      case BYPASSDATA:
        if (dataOutput != NULL) {
          *dataOutput = *dataInput;
        }
        if (numberOfSamplesOutput != NULL) {
          *numberOfSamplesOutput = *numberOfSamplesInput;
        }
        break;
      case POLYPHASETECHNIQUE:
      case POLYPHASETECHNIQUE_BYPASS:
      case POLYPHASETECHNIQUE_FAST:
      case POLYPHASETECHNIQUE_ZERO:
      case POLYPHASETECHNIQUE_FAST_ZERO:
      case POLYPHASETECHNIQUE_FIRST:
      case POLYPHASETECHNIQUE_FAST_FIRST:
      case POLYPHASETECHNIQUE_ZERO_FIRST:
      case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
        hSrConversion->numberOfSamplesInput = *numberOfSamplesInput;
        PolyphaseResample(hSrConversion->hAllPolyphase,
                          hSrConversion->numberOfSamplesInput,
                          &hSrConversion->numberOfSamplesOutput);
        if (numberOfSamplesInput != NULL) {
          *numberOfSamplesInput = hSrConversion->numberOfSamplesInput;
        }
        if (numberOfSamplesOutput != NULL) {
          *numberOfSamplesOutput = hSrConversion->numberOfSamplesOutput;
        }
        if (dataInput != NULL) {
          *dataInput = hSrConversion->dataInput;
        }
        if (dataOutput != NULL) {
          *dataOutput = hSrConversion->dataOutput;
        }
        break;
      case MAX_SRTYPE:
        break;
      default:
        RSL_PRINT_MSG(stderr, "ResamplerConstruct: samplerateMode %i not supported.\n", hSrConversion->samplerateMode);
        returnValue = 1;
        break;
    }
  }

  return (returnValue);
}

int ResamplerPreMain(
    HANDLE_RESAMPLELIB hSrConversion,
    unsigned int* sampleRateInput,
    unsigned int* sampleRateOutput,
    unsigned int* numberOfChannels,
    unsigned int* numberOfSamplesInput,
    unsigned int* numberOfSamplesOutput,
    float** dataInput,
    float** dataOutput) {
  int returnValue = 0;

  (void)sampleRateInput;
  (void)sampleRateOutput;
  (void)numberOfChannels;

  if (hSrConversion != NULL) {
    switch (hSrConversion->samplerateMode) {
      case BYPASSDATA:
        if (numberOfSamplesInput != NULL) {
          *numberOfSamplesInput = hSrConversion->numberOfSamplesInput;
        }
        if (numberOfSamplesOutput != NULL) {
          *numberOfSamplesOutput = hSrConversion->numberOfSamplesOutput;
        }
        if (dataInput != NULL) {
          *dataInput = hSrConversion->dataInput;
        }
        if (dataOutput != NULL) {
          *dataOutput = hSrConversion->dataOutput;
        }
        break;
      case POLYPHASETECHNIQUE:
      case POLYPHASETECHNIQUE_BYPASS:
      case POLYPHASETECHNIQUE_FAST:
      case POLYPHASETECHNIQUE_ZERO:
      case POLYPHASETECHNIQUE_FAST_ZERO:
      case POLYPHASETECHNIQUE_FIRST:
      case POLYPHASETECHNIQUE_FAST_FIRST:
      case POLYPHASETECHNIQUE_ZERO_FIRST:
      case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
        PolyphasePreMain(hSrConversion->hAllPolyphase,
                         &hSrConversion->dataInput,
                         &hSrConversion->numberOfSamplesInput);
        if (numberOfSamplesInput != NULL) {
          *numberOfSamplesInput = hSrConversion->numberOfSamplesInput;
        }
        if (numberOfSamplesOutput != NULL) {
          *numberOfSamplesOutput = hSrConversion->numberOfSamplesOutput;
        }
        if (dataInput != NULL) {
          *dataInput = hSrConversion->dataInput;
        }
        if (dataOutput != NULL) {
          *dataOutput = hSrConversion->dataOutput;
        }
        break;
      case MAX_SRTYPE:
        break;
      default:
        RSL_PRINT_MSG(stderr, "ResamplerConstruct: samplerateMode %i not supported.\n", hSrConversion->samplerateMode);
        returnValue = 1;
    }
  }

  return (returnValue);
}

float ResamplerGetDelayFract(HANDLE_RESAMPLELIB hSrConversion) {
  float delay = 0.f;

  delay = (int)ResamplerGetDelay(hSrConversion);

  if (hSrConversion != NULL) {
    if (hSrConversion->hAllPolyphase) {
      delay = PolyphaseGetDelayFract(hSrConversion->hAllPolyphase);
    }
  }
  return delay;
}

int ResamplerGetDelay(HANDLE_RESAMPLELIB hSrConversion) {
  int delay = 0;

  if (hSrConversion != NULL) {
    switch (hSrConversion->samplerateMode) {
      case BYPASSDATA:
        delay = 0;
        break;
      case POLYPHASETECHNIQUE:
      case POLYPHASETECHNIQUE_BYPASS:
      case POLYPHASETECHNIQUE_FAST:
      case POLYPHASETECHNIQUE_ZERO:
      case POLYPHASETECHNIQUE_FAST_ZERO:
      case POLYPHASETECHNIQUE_FIRST:
      case POLYPHASETECHNIQUE_FAST_FIRST:
      case POLYPHASETECHNIQUE_ZERO_FIRST:
      case POLYPHASETECHNIQUE_FAST_ZERO_FIRST:
        delay = PolyphaseGetDelay(hSrConversion->hAllPolyphase);
        break;
      default:
        delay = 0;
        break;
    }
  }

  return delay;
}

int ResamplerCopyBuffers(
    struct tag_resamplelib* srcResampler,
    struct tag_resamplelib** dstResampler,
    float* srcInBuf,
    float** dstInBuf) {
  int returnValue = 0;
  if ((srcResampler->numberOfSamplesInput == (*(dstResampler))->numberOfSamplesInput) &&
      (srcResampler->numberOfSamplesOutput == (*(dstResampler))->numberOfSamplesOutput) &&
      (srcResampler->samplerateMode == (*(dstResampler))->samplerateMode) &&
      (srcResampler->resamplerSubType == (*(dstResampler))->resamplerSubType)) {
    copyFLOAT(srcInBuf, *dstInBuf, srcResampler->numberOfSamplesInput);
    PolyphaseCopyOverlapBuf(srcResampler->hAllPolyphase, (*(dstResampler))->hAllPolyphase);
    returnValue = 0;
  } else {
    RSL_PRINT_MSG(stderr, "ResamplerCopyBuffers: Resamplers differ, nothing was copied!\n");
    returnValue = 1;
  }
  return (returnValue);
}
