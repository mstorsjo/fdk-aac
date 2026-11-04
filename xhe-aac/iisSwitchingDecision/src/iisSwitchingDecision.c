
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
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "iisSwitchingDecision.h"
#include "iisutillib.h"
#include "mathlib.h"
#include "smpl_resampler.h"
#include "time_buffer.h"
#include "statClasslib.h"

#define MAX_DECISION_BUFFER_SIZE (10)

static int const STATCLASS_NFRAMES_LOOKAHEAD = 20;

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct iis_swdeci_observer {
  int id;

  int samplingRate;
  int frameSize;
  int offset;
  int frameIdx;
  IIS_SWDECI_RESULT frameDecision;

  struct iis_swdeci_observer *pNext;

} IIS_SWDECI_OBSERVER;

typedef struct iis_swdeci_reset_ctrl {
  int frameIdxClassReset;
  unsigned int resetInterval;
  unsigned int resetCnt;

} IIS_SWDECI_RESET_CTRL;

typedef struct iis_swdeci {
  int nSamplesNext;

  int nFramesToProcess;

  HANDLE_ERROR_INFO(*getNextDecision)
  (HANDLE_IIS_SWDECI);

  int samplingRateIn;
  int nChannelsIn;

  IIS_SWDECI_RESET_CTRL resetCtrl;

  int frameIdxClass;
  int samplingRateClass;
  int frameSizeClass;
  int offsetClass;
  int nSamplesNextClass;

  HANDLE_STATCLASS hStatClass;
  SCFLOAT *pStatClassInBuf;

  FILE *fOutClass;

  FILE *fInClass;

  int interval;

  struct tag_resamplelib *hResampler;
  int nSamplesNextResampler;
  int nMaxSamplesNextResampler;
  float *pResamplerIn;

  IIS_SWDECI_OBSERVER *observers;

  char decisionBuffer[MAX_DECISION_BUFFER_SIZE];

  HANDLE_MP4TIMEBUF inputBuf;
  int inputBufSize;
  float *dmxBuf;

  IIS_SWDECI_INFO info;

} IIS_SWDECI;

static int __WriteHeaderClass(int const sampleRateClass, int const offsetClass, int const frameSizeClass, FILE *fOut);
static int __WriteDecision(char const decision, FILE *fOut);
static IIS_SWDECI_OBSERVER *__GetObserver(HANDLE_IIS_SWDECI hSwDeci, int const id);
static int __gcd(int u, int v);

static int __GetBufferWriteOffset(HANDLE_IIS_SWDECI hSwDeci) {
  int offset = 0;

  if (hSwDeci != NULL) {
    if (hSwDeci->info.bDelayAway > 0) {
      offset = 0;
    } else {
      offset = hSwDeci->nSamplesNext - max(hSwDeci->nSamplesNextResampler, hSwDeci->nSamplesNextClass);
    }
  }

  return offset;
}

static HANDLE_ERROR_INFO __OpenInputBuffer(HANDLE_IIS_SWDECI hSwDeci) {
  HANDLE_ERROR_INFO err = noError;
  int dmxBufSizeMax = 0;
  int bufferWriteOffset = 0;

  if (hSwDeci != NULL) {
    iisSwitchingDecisionGetNbrSamplesNext(hSwDeci);
    if (hSwDeci->inputBuf == NULL) {
      IIS_SWDECI_OBSERVER *observer = hSwDeci->observers;
      int alignmentDelay = INT_MIN;

      hSwDeci->inputBufSize = hSwDeci->nSamplesNext * ((hSwDeci->info.bLowDelay == 1) ? 2 : 1);
      bufferWriteOffset = __GetBufferWriteOffset(hSwDeci);

      while (observer != NULL) {
        int tmp_alignmentDelay = 0;
        int gcdTmp = 0;
        int timebase = 0;
        int scale4Obs = 0;
        int scale4Sw = 0;
        int scale4Input = 0;
        int SwBuffer = 0;
        int preBuffer = 0;
        int frameSizeObs = 0;
        int frameSizeSw = 0;
        int gcdFrSiz = 0;

        gcdTmp = __gcd(observer->samplingRate, hSwDeci->samplingRateClass);
        timebase = observer->samplingRate * (hSwDeci->samplingRateClass / gcdTmp);
        gcdTmp = __gcd(timebase, hSwDeci->samplingRateIn);
        timebase *= hSwDeci->samplingRateIn / gcdTmp;
        scale4Obs = timebase / observer->samplingRate;
        scale4Sw = timebase / hSwDeci->samplingRateClass;
        scale4Input = timebase / hSwDeci->samplingRateIn;
        SwBuffer = (bufferWriteOffset * scale4Input) - (observer->offset * scale4Obs - hSwDeci->offsetClass * scale4Sw);

        frameSizeObs = observer->frameSize * scale4Obs;
        frameSizeSw = hSwDeci->frameSizeClass * scale4Sw;
        gcdFrSiz = __gcd(frameSizeObs, frameSizeSw);
        preBuffer = (frameSizeSw - gcdFrSiz);

        if (SwBuffer > 0) {
          preBuffer -= SwBuffer / gcdFrSiz * gcdFrSiz;
        } else {
          preBuffer -= ((SwBuffer - gcdFrSiz + 1) / gcdFrSiz) * gcdFrSiz;
        }
        tmp_alignmentDelay = preBuffer;

        if (tmp_alignmentDelay < 0) {
          tmp_alignmentDelay -= scale4Obs - 1;
        } else {
          tmp_alignmentDelay += scale4Obs - 1;
        }
        tmp_alignmentDelay /= scale4Obs;

        tmp_alignmentDelay *= hSwDeci->samplingRateIn;
        if (tmp_alignmentDelay < 0) {
          tmp_alignmentDelay -= (observer->samplingRate - 1);
        } else {
          tmp_alignmentDelay += (observer->samplingRate - 1);
        }
        tmp_alignmentDelay /= observer->samplingRate;
        if (tmp_alignmentDelay > alignmentDelay) {
          alignmentDelay = tmp_alignmentDelay;
        }
        observer = observer->pNext;
      }

      bufferWriteOffset += alignmentDelay;
      if (alignmentDelay > 0) {
        hSwDeci->inputBufSize += alignmentDelay;
      }

      err = MP4TIMEBUF_Create(&hSwDeci->inputBuf,
                              hSwDeci->inputBufSize,
                              bufferWriteOffset,
                              hSwDeci->nChannelsIn,
                              bufferWriteOffset);
    }

    if (err == noError) {
      hSwDeci->nSamplesNext -= (bufferWriteOffset);
      hSwDeci->info.bufferWriteOffset = bufferWriteOffset;
      hSwDeci->info.bufferSize = hSwDeci->inputBufSize;
    }

    if (err == noError) {
      if (hSwDeci->nChannelsIn == 2) {
        dmxBufSizeMax = (hSwDeci->hResampler != NULL) ? hSwDeci->nMaxSamplesNextResampler : hSwDeci->nSamplesNextClass;

        if (NULL == (hSwDeci->dmxBuf = (float *)iisCalloc(dmxBufSizeMax, sizeof(float)))) {
          err = iisUtil_ERROR(CDI, "Memory allocation failed.");
        }
      }
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO __OpenResampler(HANDLE_IIS_SWDECI hSwDeci) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    if (hSwDeci->samplingRateClass != hSwDeci->samplingRateIn) {
      smpl_filterParams filtParams;

      filtParams.lowpassFrequency = ((hSwDeci->samplingRateIn / 2.f) * (1.0f - 0.5f * 0.05f));
      filtParams.transitionBandwidth = ((hSwDeci->samplingRateIn / 2.f) * 0.05f);
      filtParams.stopBandAttenuation = 60;

      if (0 != smpl_resampler_fo_construct(&hSwDeci->hResampler,
                                           &filtParams,
                                           hSwDeci->samplingRateIn,
                                           hSwDeci->samplingRateClass,
                                           1,
                                           hSwDeci->frameSizeClass,
                                           &hSwDeci->pResamplerIn,
                                           (unsigned int *)&hSwDeci->nSamplesNextResampler)) {
        err = iisUtil_ERROR(CDI, "Failed to open resampler.");
      } else {
        hSwDeci->offsetClass -= smpl_resampler_get_delay(hSwDeci->hResampler);
        hSwDeci->nMaxSamplesNextResampler = hSwDeci->nSamplesNextResampler;
      }
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO __AdvanceResampler(HANDLE_IIS_SWDECI hSwDeci,
                                            float **pSamplesOut) {
  HANDLE_ERROR_INFO err = noError;
  unsigned int nSamplesResamplerOut = 0;

  if (hSwDeci != NULL) {
    if (hSwDeci->hResampler != NULL) {
      if (0 != smpl_resampler_advance(hSwDeci->hResampler,
                                      hSwDeci->pResamplerIn,
                                      hSwDeci->nSamplesNextResampler,
                                      pSamplesOut,
                                      &nSamplesResamplerOut,
                                      &hSwDeci->pResamplerIn,
                                      (unsigned int *)&hSwDeci->nSamplesNextResampler)) {
        err = iisUtil_ERROR(CDI, "Resampling failed.");
      }

      if (hSwDeci->nSamplesNextResampler > hSwDeci->nMaxSamplesNextResampler) {
        err = iisUtil_ERROR(CDI, "Resampling is asking for a higher number of samples, than the maximum(first Frame), this should never happen.");
      }

      if ((int)nSamplesResamplerOut != hSwDeci->nSamplesNextClass) {
        err = iisUtil_ERROR(CDI, "Resampling failed.");
      }
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static void __UpdateDecisionBuffer(HANDLE_IIS_SWDECI hSwDeci) {
  if (hSwDeci != NULL) {
    memmove(&hSwDeci->decisionBuffer[1], &hSwDeci->decisionBuffer[0], sizeof(unsigned char) * (MAX_DECISION_BUFFER_SIZE - 1));
  }
  return;
}

static void __ConfigureStatClass(STATCLASS_CONFIG *pStatClassConfig,
                                 int const bitRate,
                                 int const samplingRateIn,
                                 int const bLowDelay,
                                 int const granularity) {
  pStatClassConfig->granularity = granularity;
  pStatClassConfig->codecType = (granularity == 512) ? STATCLASS_CODEC_MPEGH : STATCLASS_CODEC_XHEAAC;
  assert((pStatClassConfig->granularity == 1024 || pStatClassConfig->granularity == 512) && "Configuration of statClass: granularity must be either 1024 or 512.");
  switch (samplingRateIn) {
    case 44100:
    case 29400:
    case 22050:
    case 11025:
      pStatClassConfig->samplingRate = 14700;
      break;

    default:
      pStatClassConfig->samplingRate = 16000;
      break;
  }

  pStatClassConfig->nFramesLookAhead = STATCLASS_NFRAMES_LOOKAHEAD;
  if (bLowDelay > 0) {
    pStatClassConfig->nFramesLookAhead = bLowDelay - 1;
    if (pStatClassConfig->nFramesLookAhead > STATCLASS_NFRAMES_LOOKAHEAD) {
      pStatClassConfig->nFramesLookAhead = STATCLASS_NFRAMES_LOOKAHEAD;
    }
  }

  if (pStatClassConfig->granularity == 1024) {
    if (bitRate >= 15000 && bitRate <= 17999) {
      if (pStatClassConfig->samplingRate == 16000) {
        pStatClassConfig->stability = 0.55f;
        pStatClassConfig->speechBias = 0.5f;
      }
      if (pStatClassConfig->samplingRate == 14700) {
        pStatClassConfig->stability = 0.65f;
        pStatClassConfig->speechBias = 0.5f;
      }
    }

    else if (bitRate >= 18000 && bitRate <= 39999) {
      if (pStatClassConfig->samplingRate == 16000) {
        pStatClassConfig->stability = 0.1f;
        pStatClassConfig->speechBias = 0.9f;
      } else if (pStatClassConfig->samplingRate == 14700) {
        pStatClassConfig->stability = 0.5f;
        pStatClassConfig->speechBias = 0.5f;
      }
    }

    else {
      if (pStatClassConfig->samplingRate == 16000) {
        pStatClassConfig->stability = 0.5f;
        pStatClassConfig->speechBias = 0.0f;
      } else if (pStatClassConfig->samplingRate == 14700) {
        pStatClassConfig->stability = 0.6f;
        pStatClassConfig->speechBias = 0.0f;
      }
    }

  } else {
    pStatClassConfig->stability = 0.6f;
    pStatClassConfig->speechBias = 0.122f;
  }

  return;
}

static HANDLE_ERROR_INFO __OpenStatClass(HANDLE_IIS_SWDECI hSwDeci,
                                         int const bitRate,
                                         int const samplingRateIn,
                                         int const bLowDelay,
                                         int const granularity) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    STATCLASS_ERROR_CODE statError = STATCLASS_NO_ERROR;
    STATCLASS_CONFIG statConfig = {0};
    STATCLASS_INFO statClassInfo = {0};

    __ConfigureStatClass(&statConfig,
                         bitRate,
                         samplingRateIn,
                         bLowDelay,
                         granularity);

    statError = STATCLASS_Open(&hSwDeci->hStatClass,
                               &statConfig);

    if (statError == STATCLASS_NO_ERROR) {
      statError = STATCLASS_GetInfo(hSwDeci->hStatClass, &statClassInfo);
    }

    if (statError == STATCLASS_NO_ERROR) {
      hSwDeci->samplingRateClass = statClassInfo.samplingRate;
      hSwDeci->frameSizeClass = statClassInfo.granularity;
      hSwDeci->offsetClass = -statClassInfo.nSamplesDelay;
      hSwDeci->nSamplesNextClass = statClassInfo.granularity;
    }

    if (statError != STATCLASS_NO_ERROR) {
      err = iisUtil_ERROR(CDI, "Failed to open statClass.");
    }

    if (err == noError) {
      err = __OpenResampler(hSwDeci);
    }

    if (err == noError) {
      if (NULL == (hSwDeci->pStatClassInBuf = (SCFLOAT *)iisCalloc(statClassInfo.granularity, sizeof(SCFLOAT)))) {
        err = iisUtil_ERROR(CDI, "Memory allocation failed.");
      }
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO __PrepareBufferStatClass(HANDLE_IIS_SWDECI hSwDeci) {
  HANDLE_ERROR_INFO err = noError;
  int i = 0;
  float *pSamples = NULL;
  int nSamplesToRead = 0;

  if (hSwDeci != NULL) {
    if (hSwDeci->inputBuf != NULL) {
      pSamples = MP4TIMEBUF_AccessBuffer(hSwDeci->inputBuf, 0, 0);

      if (pSamples == NULL) {
        err = iisUtil_ERROR(CDI, "Failed to access input buffer.");
      }

    } else {
      err = iisUtil_ERROR(CDI, "Input buffer not initialized.");
    }

    if (err == noError) {
      nSamplesToRead = (hSwDeci->hResampler != NULL) ? hSwDeci->nSamplesNextResampler : hSwDeci->nSamplesNextClass;

      if (hSwDeci->nChannelsIn == 2) {
        for (i = 0; i < nSamplesToRead; i++) {
          hSwDeci->dmxBuf[i] = pSamples[2 * i] * 0.5f + pSamples[2 * i + 1] * 0.5f;
        }
        pSamples = hSwDeci->dmxBuf;
      }

      if (hSwDeci->hResampler != NULL) {
        copyFLOAT(pSamples, hSwDeci->pResamplerIn, nSamplesToRead);

        err = __AdvanceResampler(hSwDeci, &pSamples);
      }
    }

    if (err == noError) {
      for (i = 0; i < hSwDeci->nSamplesNextClass; i++) {
        hSwDeci->pStatClassInBuf[i] = (SCFLOAT)pSamples[i] * 32768.f;
      }

      err = MP4TIMEBUF_InvalidateBuffer(hSwDeci->inputBuf, nSamplesToRead);
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO __GetNextDecisionStatClass(HANDLE_IIS_SWDECI hSwDeci) {
  HANDLE_ERROR_INFO err = noError;
  STATCLASS_DECISION dec = STATCLASS_MUSIC;

  int nSamplesUsedStatClass = 0;

  if (hSwDeci != NULL) {
    err = __PrepareBufferStatClass(hSwDeci);

    if (err == noError) {
      if (STATCLASS_NO_ERROR != STATCLASS_Advance(hSwDeci->hStatClass,
                                                  hSwDeci->pStatClassInBuf,
                                                  hSwDeci->nSamplesNextClass,
                                                  &hSwDeci->nSamplesNextClass,
                                                  &nSamplesUsedStatClass,
                                                  &dec)) {
        err = iisUtil_ERROR(CDI, "statClass failed.");
      }
    }

    __UpdateDecisionBuffer(hSwDeci);
    hSwDeci->decisionBuffer[0] = (err == noError) ? (unsigned char)dec : -1;

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static int __WriteHeaderClass(int const sampleRateClass, int const offsetClass, int const frameSizeClass, FILE *fOut) {
  unsigned int const _null = 0;
  int nBytesWritten = 0;

  if (fOut != NULL) {
    nBytesWritten += (int)fwrite(&sampleRateClass, sizeof(int), 1, fOut);
    nBytesWritten += (int)fwrite(&offsetClass, sizeof(int), 1, fOut);
    nBytesWritten += (int)fwrite(&frameSizeClass, sizeof(int), 1, fOut);
    nBytesWritten += (int)fwrite(&_null, sizeof(int), 1, fOut);
  }

  return nBytesWritten * sizeof(int);
}

static int __WriteDecision(char const decision, FILE *fOut) {
  return (fOut != NULL) ? (int)fwrite(&decision, sizeof(char), 1, fOut) : 0;
}

static IIS_SWDECI_OBSERVER *__GetObserver(HANDLE_IIS_SWDECI hSwDeci,
                                          int const id) {
  IIS_SWDECI_OBSERVER *pObserver = NULL;

  if (hSwDeci != NULL) {
    if (hSwDeci->observers != NULL) {
      pObserver = hSwDeci->observers;

      while ((pObserver != NULL) && (pObserver->id != id)) {
        pObserver = pObserver->pNext;
      }
    }
  }

  return pObserver;
}

static HANDLE_ERROR_INFO __GetNbrFramesToProcessNext(HANDLE_IIS_SWDECI hSwDeci,
                                                     IIS_SWDECI_OBSERVER *pObserver,
                                                     int *nFrames) {
  HANDLE_ERROR_INFO err = noError;
  int scaleFrameIdx = 0;
  int scaleSampleIdx = 0;
  int frameDiff = 0;

  if (hSwDeci != NULL) {
    if (pObserver != NULL) {
      int const commonSampleRate = (hSwDeci->samplingRateClass * pObserver->samplingRate) / __gcd(hSwDeci->samplingRateClass, pObserver->samplingRate);
      int const scale4Switching = commonSampleRate / hSwDeci->samplingRateClass;
      int const scale4Observer = commonSampleRate / pObserver->samplingRate;

      int const offset = pObserver->offset * scale4Observer;
      int const startIdx = (pObserver->frameIdx + 1) * (pObserver->frameSize * scale4Observer);

      scaleSampleIdx =
          offset -
          (hSwDeci->offsetClass * scale4Switching) +
          (startIdx + pObserver->frameSize * scale4Observer - 1);

      scaleFrameIdx = (int)(scaleSampleIdx / (hSwDeci->frameSizeClass * scale4Switching));

      frameDiff = scaleFrameIdx - hSwDeci->frameIdxClass;

      if (nFrames != NULL) {
        *nFrames = frameDiff;
      }

    } else {
      err = iisUtil_ERROR(CDI, "No observer found.");
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

static HANDLE_ERROR_INFO __MapDecision(HANDLE_IIS_SWDECI hSwDeci,
                                       int const id,
                                       IIS_SWDECI_RESULT *frameDecision) {
  IIS_SWDECI_RESULT result = IIS_SWDECI_RESULT_INVALID;
  IIS_SWDECI_OBSERVER *pObserver = NULL;
  HANDLE_ERROR_INFO err = noError;

  int s = 0;
  int total = 0;
  unsigned char decision = '\0';

  if (hSwDeci != NULL) {
    pObserver = __GetObserver(hSwDeci, id);

    if (pObserver != NULL) {
      float const scale = hSwDeci->samplingRateClass / (float)pObserver->samplingRate;
      float const offset = pObserver->offset * scale;
      int const startIdx = ++pObserver->frameIdx * pObserver->frameSize;

      for (s = 0; s < pObserver->frameSize; s++) {
        float const idx = offset - hSwDeci->offsetClass + (startIdx + s) * scale;

        if (idx >= 0) {
          int const frameIdx = (int)idx / hSwDeci->frameSizeClass;
          int const frameDiff = hSwDeci->frameIdxClass - frameIdx;

          if ((frameDiff >= MAX_DECISION_BUFFER_SIZE) || (frameDiff < 0)) {
            err = iisUtil_ERROR(CDI, "Attempt to access out of the decision memory.");

          } else if (frameDiff >= 0) {
            decision = hSwDeci->decisionBuffer[frameDiff];
          }

          if (err != noError) {
            break;
          }

          total += ((int)decision > 0);

          if ((total >= pObserver->frameSize / 2) || (s + 1 - total >= pObserver->frameSize / 2)) {
            break;
          }
        }
      }

      if (err == noError) {
        result = ((int)((total / (float)pObserver->frameSize) + 0.5f) == 0) ? IIS_SWDECI_RESULT_MUSIC : IIS_SWDECI_RESULT_SPEECH;
      }

      pObserver->frameDecision = result;
      if (frameDecision != NULL) {
        *frameDecision = result;
      }

    } else {
      err = iisUtil_ERROR(CDI, "Observer not found.");
    }
  }

  return err;
}

static int __gcd(int u, int v) {
  return (v != 0) ? __gcd(v, u % v) : u;
}

static void __InitReset(HANDLE_IIS_SWDECI hSwDeci) {
  IIS_SWDECI_OBSERVER *pObserver = NULL;
  int resetIndex = 1;

  if (hSwDeci != NULL) {
    pObserver = hSwDeci->observers;

    while (pObserver != NULL) {
      if (pObserver->id == 0) {
        resetIndex = pObserver->samplingRate * hSwDeci->frameSizeClass / __gcd(hSwDeci->samplingRateClass * pObserver->frameSize, pObserver->samplingRate * hSwDeci->frameSizeClass);
      }
      pObserver = pObserver->pNext;
    }

    hSwDeci->resetCtrl.frameIdxClassReset = hSwDeci->nFramesToProcess - 1;
    hSwDeci->resetCtrl.resetInterval = resetIndex;
    hSwDeci->resetCtrl.resetCnt = hSwDeci->resetCtrl.resetInterval;
  }

  return;
}

static void __ResetMapping(HANDLE_IIS_SWDECI hSwDeci) {
  IIS_SWDECI_OBSERVER *pObserver = NULL;

  if (hSwDeci != NULL) {
    pObserver = hSwDeci->observers;

    while (pObserver != NULL) {
      pObserver->frameIdx = -1;
      pObserver = pObserver->pNext;
    }

    hSwDeci->frameIdxClass = hSwDeci->resetCtrl.frameIdxClassReset;
    hSwDeci->resetCtrl.resetCnt = hSwDeci->resetCtrl.resetInterval;
  }

  return;
}

struct ERROR_INFO *iisSwitchingDecisionOpen(
    HANDLE_IIS_SWDECI *phSwdeci,
    IIS_SWDECI_SETUP const *pSetup) {
  HANDLE_ERROR_INFO err = noError;
  HANDLE_IIS_SWDECI hSwDeci = NULL;

  if ((phSwdeci != NULL) && (pSetup != NULL)) {
    if (NULL == (hSwDeci = (HANDLE_IIS_SWDECI)iisCalloc(1, sizeof(IIS_SWDECI)))) {
      err = iisUtil_ERROR(CDI, "Memory allocation failed!");
    }

    if (err == noError) {
      memset(hSwDeci, 0, sizeof(IIS_SWDECI));
      hSwDeci->frameIdxClass = -1;
      hSwDeci->nChannelsIn = pSetup->nChannelsIn;
      hSwDeci->samplingRateIn = pSetup->samplingRateIn;
      hSwDeci->interval = pSetup->interval;

      hSwDeci->fOutClass = pSetup->fOutClass;
      hSwDeci->fInClass = pSetup->fInClass;
      hSwDeci->info.bDelayAway = pSetup->bDelayAway;
      hSwDeci->info.bLowDelay = pSetup->bLowDelay;

      if ((pSetup->nChannelsIn > 2) || (pSetup->nChannelsIn < 1)) {
        err = iisUtil_ERROR(CDI, "Only one or two input channels supported.");
      }
    }

    if (err == noError) {
      err = __OpenStatClass(hSwDeci,
                            pSetup->bitRate,
                            pSetup->samplingRateIn,
                            pSetup->bLowDelay,
                            pSetup->granularity);

      hSwDeci->getNextDecision = (err == noError) ? &__GetNextDecisionStatClass : NULL;
    }

    if (err != noError) {
      iisSwitchingDecisionClose(&hSwDeci);
    }

    if (err == noError) {
      *phSwdeci = hSwDeci;
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid pointer!");
  }

  return err;
}

void iisSwitchingDecisionClose(
    HANDLE_IIS_SWDECI *phSwdeci) {
  HANDLE_IIS_SWDECI hSwDeci = NULL;

  if (phSwdeci != NULL) {
    hSwDeci = *phSwdeci;

    if (hSwDeci != NULL) {
      if (hSwDeci->hStatClass != NULL) {
        STATCLASS_Close(&hSwDeci->hStatClass);
        hSwDeci->hStatClass = NULL;
      }

      if (hSwDeci->hResampler != NULL) {
        smpl_resampler_destruct(&hSwDeci->hResampler);
      }

      if (hSwDeci->pStatClassInBuf != NULL) {
        iisFree(hSwDeci->pStatClassInBuf);
        hSwDeci->pStatClassInBuf = NULL;
      }

      if (hSwDeci->observers != NULL) {
        IIS_SWDECI_OBSERVER *observer = hSwDeci->observers;

        while (observer->pNext != NULL) {
          while (observer->pNext->pNext != NULL) {
            observer = observer->pNext;
          }
          iisFree(observer->pNext);
          observer->pNext = NULL;
          observer = hSwDeci->observers;
        }
        iisFree(observer);
      }

      if (hSwDeci->inputBuf != NULL) {
        MP4TIMEBUF_Delete(&hSwDeci->inputBuf);
      }

      if (hSwDeci->dmxBuf != NULL) {
        iisFree(hSwDeci->dmxBuf);
        hSwDeci->dmxBuf = NULL;
      }

      iisFree(hSwDeci);
      *phSwdeci = NULL;
    }
  }

  return;
}

struct ERROR_INFO *iisSwitchingDecisionAttach(
    HANDLE_IIS_SWDECI hSwDeci,
    int const id,
    int const samplingRate,
    int const frameSize,
    int const delay) {
  HANDLE_ERROR_INFO err = noError;
  IIS_SWDECI_OBSERVER *observer = NULL;

  if (hSwDeci != NULL) {
    if (hSwDeci->observers != NULL) {
      observer = hSwDeci->observers;
      while (observer->pNext != NULL) {
        observer = observer->pNext;
      }

      if (NULL == (observer->pNext = (IIS_SWDECI_OBSERVER *)iisCalloc(1, sizeof(IIS_SWDECI_OBSERVER)))) {
        err = iisUtil_ERROR(CDI, "Memory allocation failed.");
      } else {
        observer = observer->pNext;
      }

    } else {
      if (NULL == (hSwDeci->observers = (IIS_SWDECI_OBSERVER *)iisCalloc(1, sizeof(IIS_SWDECI_OBSERVER)))) {
        err = iisUtil_ERROR(CDI, "Memory allocation failed.");
      } else {
        observer = hSwDeci->observers;
      }
    }

    if (observer != NULL) {
      observer->id = id;
      observer->samplingRate = samplingRate;
      observer->frameSize = frameSize;
      observer->offset = -delay;
      observer->frameIdx = -1;
      observer->frameDecision = IIS_SWDECI_RESULT_INVALID;
    }

    if (observer != NULL) {
      int gcdSR = __gcd(hSwDeci->observers->samplingRate, observer->samplingRate);
      int frameSizeRef = hSwDeci->observers->frameSize * observer->samplingRate / gcdSR;
      int frameSizeObs = observer->frameSize * hSwDeci->observers->samplingRate / gcdSR;
      if (frameSizeRef != frameSizeObs) {
        err = iisUtil_ERROR(CDI, "Observers with different framing are not supported.");
      }
    }
  }

  return err;
}

int iisSwitchingDecisionGetNbrSamplesNext(
    HANDLE_IIS_SWDECI hSwDeci) {
  int nSamplesNext = 0;
  int nFramesNext = 0;
  int nFramesNextTmp = 0;
  IIS_SWDECI_OBSERVER *pObserver = NULL;
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    if (hSwDeci->observers != NULL) {
      pObserver = hSwDeci->observers;

      while ((pObserver != NULL) && (err == noError)) {
        err = __GetNbrFramesToProcessNext(hSwDeci, pObserver, &nFramesNextTmp);
        nFramesNext = max(nFramesNext, nFramesNextTmp);
        pObserver = pObserver->pNext;
      }
    }
  }

  if (err == noError) {
    nSamplesNext = nFramesNext * ((hSwDeci->hResampler != NULL) ? hSwDeci->nSamplesNextResampler : hSwDeci->nSamplesNextClass);

    hSwDeci->nFramesToProcess = nFramesNext;

    if (hSwDeci->inputBuf) {
      if (nFramesNext > 1) {
        if ((hSwDeci->nMaxSamplesNextResampler > hSwDeci->nSamplesNextResampler) && (hSwDeci->hResampler != NULL)) {
          nSamplesNext += (nFramesNext - 1) * (hSwDeci->nMaxSamplesNextResampler - hSwDeci->nSamplesNextResampler);
        }
      }
    }
    if (hSwDeci->inputBuf) {
      int validSamples = 0;
      err = MP4TIMEBUF_getValidSamples(hSwDeci->inputBuf, &validSamples);
      nSamplesNext -= validSamples;
    }
    hSwDeci->nSamplesNext = nSamplesNext;
  }

  return (err == noError) ? max(0, hSwDeci->nSamplesNext) : 0;
}

struct ERROR_INFO *iisSwitchingDecisionFeedSamples(
    HANDLE_IIS_SWDECI hSwDeci,
    float const *pSamples,
    int *nSamples,
    int *nSamplesNext) {
  HANDLE_ERROR_INFO err = noError;
  int nSamplesToFill = 0;

  if (hSwDeci != NULL) {
    if ((nSamples != NULL) && (nSamplesNext != NULL)) {
      if (hSwDeci->inputBuf != NULL) {
        nSamplesToFill = *nSamples / hSwDeci->nChannelsIn;
        hSwDeci->nSamplesNext -= nSamplesToFill;
        err = MP4TIMEBUF_FeedBufferMulti(hSwDeci->inputBuf,
                                         pSamples,
                                         &nSamplesToFill);
        if (err == noError) {
          hSwDeci->nSamplesNext += nSamplesToFill;
          *nSamples %= hSwDeci->nChannelsIn;
          *nSamples += nSamplesToFill * hSwDeci->nChannelsIn;
          *nSamplesNext = hSwDeci->nSamplesNext;
        }
      }

    } else {
      err = iisUtil_ERROR(CDI, "Invalid pointer.");
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

struct ERROR_INFO *iisSwitchingDecisionGetNextDecision(
    HANDLE_IIS_SWDECI hSwDeci,
    int const id,
    IIS_SWDECI_RESULT *dec) {
  HANDLE_ERROR_INFO err = noError;
  IIS_SWDECI_RESULT result = IIS_SWDECI_RESULT_INVALID;

  if (hSwDeci != NULL) {
    while ((hSwDeci->nFramesToProcess > 0) && (err == noError)) {
      err = hSwDeci->getNextDecision(hSwDeci);

      if (hSwDeci->fOutClass != NULL) {
        __WriteDecision(hSwDeci->decisionBuffer[0], hSwDeci->fOutClass);
      }

      hSwDeci->frameIdxClass++;
      hSwDeci->nFramesToProcess--;
    }

    if (id == hSwDeci->observers->id) {
      if (hSwDeci->resetCtrl.resetCnt == 0) {
        __ResetMapping(hSwDeci);
      }
      hSwDeci->resetCtrl.resetCnt--;
    }

    if (err == noError) {
      err = __MapDecision(hSwDeci, id, &result);
    }

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  if (dec != NULL) {
    *dec = result;
  }

  return err;
}

IIS_SWDECI_RESULT iisSwitchingDecisionGetFrameDecision(
    HANDLE_IIS_SWDECI hSwDeci,
    int const id) {
  IIS_SWDECI_OBSERVER *pObserver = NULL;

  pObserver = __GetObserver(hSwDeci, id);

  return (pObserver != NULL) ? pObserver->frameDecision : IIS_SWDECI_RESULT_INVALID;
}

struct ERROR_INFO *iisSwitchingDecisionInit(
    HANDLE_IIS_SWDECI hSwDeci,
    int *nSamplesFirst) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    err = __OpenInputBuffer(hSwDeci);

  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  if (err == noError) {
    if (nSamplesFirst != NULL) {
      *nSamplesFirst = hSwDeci->nSamplesNext;
    } else {
      err = iisUtil_ERROR(CDI, "Invalid pointer.");
    }
  }

  if (err == noError) {
    __InitReset(hSwDeci);
  }

  if (err == noError) {
    if (hSwDeci->fOutClass) {
      __WriteHeaderClass(hSwDeci->samplingRateClass,
                         hSwDeci->offsetClass,
                         hSwDeci->frameSizeClass,
                         hSwDeci->fOutClass);
    }
  }

  return err;
}

struct ERROR_INFO *iisSwitchingDecisionGetInfo(
    HANDLE_IIS_SWDECI hSwDeci,
    IIS_SWDECI_INFO *pInfo) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    if (pInfo != NULL) {
      *pInfo = hSwDeci->info;
    }
  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

struct ERROR_INFO *iisSwitchingDecisionGetFreeSamplesInBuffer(
    HANDLE_IIS_SWDECI hSwDeci,
    int *nFreeSamples) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    MP4TIMEBUF_getFreeSamples(hSwDeci->inputBuf, nFreeSamples);
  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

struct ERROR_INFO *iisSwitchingDecisionGetFrameLength(
    HANDLE_IIS_SWDECI hSwDeci,
    int *frameLength) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    if (frameLength != NULL) {
      *frameLength = hSwDeci->frameSizeClass;
    }
  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}

struct ERROR_INFO *iisSwitchingDecisionGetSampleRate(
    HANDLE_IIS_SWDECI hSwDeci,
    int *sampleRate) {
  HANDLE_ERROR_INFO err = noError;

  if (hSwDeci != NULL) {
    if (sampleRate != NULL) {
      *sampleRate = hSwDeci->samplingRateClass;
    }
  } else {
    err = iisUtil_ERROR(CDI, "Invalid handle.");
  }

  return err;
}
