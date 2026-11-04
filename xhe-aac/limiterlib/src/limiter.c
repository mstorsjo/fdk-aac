
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
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "iisutillib.h"

#include "limiter_buildconf.h"
#include "limiterlib.h"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define TP_FILT_LEN 20

static const float tpFilt[4][TP_FILT_LEN] = {
    {0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
     1.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f},
    {-0.003906f, 0.006828f, -0.010843f, 0.016275f, -0.023640f, 0.033866f, -0.048870f, 0.073390f, -0.123109f, 0.297714f,
     0.899517f, -0.176098f, 0.093026f, -0.059415f, 0.040580f, -0.028310f, 0.019671f, -0.013356f, 0.008682f, -0.005246f},
    {-0.006431f, 0.010917f, -0.017045f, 0.025326f, -0.036596f, 0.052406f, -0.076089f, 0.116378f, -0.205502f, 0.634362f,
     0.634362f, -0.205502f, 0.116378f, -0.076089f, 0.052406f, -0.036596f, 0.025326f, -0.017045f, 0.010917f, -0.006431f},
    {-0.005246f, 0.008682f, -0.013356f, 0.019671f, -0.028310f, 0.040580f, -0.059415f, 0.093026f, -0.176098f, 0.899517f,
     0.297714f, -0.123109f, 0.073390f, -0.048870f, 0.033866f, -0.023640f, 0.016275f, -0.010843f, 0.006828f, -0.003906f}

};

struct TDLimiter {
  unsigned int attack;
  float attackConst, releaseConst;
  float attackMs, releaseMs, maxAttackMs;
  float threshold;
  TDLSmoothType smoothType;
  unsigned int channels, maxChannels;
  unsigned int sampleRate, maxSampleRate;
  int audioIsInterleaved;
  float max, cor;
  float* maxBuf;
  float* delayBuf;
  unsigned int maxBufIdx, delayBufIdx;
  double smoothState0;
  float minGain;
  unsigned int delay;
  unsigned int tpFlag;
  float* tpBuf;
  unsigned int tpBufPos;
  int bReturnGains;
};

TDLimiterPtr createLimiter(
    float maxAttackMs,
    float releaseMs,
    float threshold,
    TDLSmoothType smoothType,
    unsigned int maxChannels,
    unsigned int maxSampleRate) {
  TDLimiterPtr limiter = NULL;
  unsigned int attack;

  switch (smoothType) {
    case TDL_EXPONENTIAL:
      break;

    default:
      return NULL;
  }

  attack = (unsigned int)(maxAttackMs * maxSampleRate / 1000);

  limiter = (TDLimiterPtr)iisCalloc(1, sizeof(struct TDLimiter));
  if (!limiter) return NULL;

  limiter->maxBuf = (float*)iisCalloc(attack + 1, sizeof(float));
  if (attack) {
    limiter->delayBuf = (float*)iisCalloc(attack * maxChannels, sizeof(float));
  }

  if ((!limiter->maxBuf) || (!limiter->delayBuf && attack)) {
    destroyLimiter(limiter);
    return NULL;
  }

  limiter->maxBufIdx = limiter->delayBufIdx = 0;

  limiter->attackMs = maxAttackMs;
  limiter->maxAttackMs = maxAttackMs;
  limiter->releaseMs = releaseMs;
  limiter->attack = attack;
  if (attack)
    limiter->attackConst = (float)pow(0.1, 1.0 / (attack + 1));
  else
    limiter->attackConst = 0.0f;
  limiter->releaseConst = (float)pow(0.1, 1.0 / (releaseMs * maxSampleRate / 1000 + 1));
  limiter->threshold = threshold;
  limiter->smoothType = smoothType;
  limiter->channels = maxChannels;
  limiter->maxChannels = maxChannels;
  limiter->sampleRate = maxSampleRate;
  limiter->maxSampleRate = maxSampleRate;
  limiter->audioIsInterleaved = 1;
  limiter->delay = attack;

  limiter->max = 0;
  limiter->cor = 1;
  limiter->smoothState0 = 1;
  limiter->minGain = 1;

  limiter->tpFlag = 0;
  limiter->tpBuf = NULL;
  limiter->tpBufPos = 0;

  limiter->bReturnGains = 0;

  return limiter;
}

int resetLimiter(TDLimiterPtr limiter) {
  if (limiter) {
    limiter->maxBufIdx = 0;
    limiter->delayBufIdx = 0;
    limiter->max = 0;
    limiter->cor = 1;
    limiter->smoothState0 = 1;
    limiter->minGain = 1;

    if (limiter->tpFlag == 0) {
      memset(limiter->maxBuf, 0, (limiter->attack + 1) * sizeof(float));
    } else {
      limiter->tpBufPos = 0;
      memset(limiter->maxBuf, 0, 4 * (limiter->attack + 1) * sizeof(float));
      memset(limiter->tpBuf, 0, TP_FILT_LEN * limiter->channels * sizeof(float));
    }

    if (limiter->delay) {
      memset(limiter->delayBuf, 0, limiter->delay * limiter->channels * sizeof(float));
    }
  }

  return TDLIMIT_OK;
}

int destroyLimiter(TDLimiterPtr limiter) {
  if (limiter) {
    iisFree(limiter->maxBuf);
    iisFree(limiter->delayBuf);

    if (limiter->tpFlag == 1) {
      iisFree(limiter->tpBuf);
    }

    iisFree(limiter);
  }
  return TDLIMIT_OK;
}

int applyLimiter(TDLimiterPtr limiter, float* samples, unsigned int nSamples) {
  unsigned int i, j, c;
  float tmp, old, gain;
  float minGain = 1;

  if (limiter == NULL) return TDLIMIT_INVALID_HANDLE;

  {
    unsigned int channels = limiter->channels;
    unsigned int attack = limiter->attack;
    float attackConst = limiter->attackConst;
    float releaseConst = limiter->releaseConst;
    float threshold = limiter->threshold;

    float max = limiter->max;
    float* maxBuf = limiter->maxBuf;
    unsigned int maxBufIdx = limiter->maxBufIdx;
    float cor = limiter->cor;
    float* delayBuf = limiter->delayBuf;
    unsigned int delayBufIdx = limiter->delayBufIdx;
    unsigned int delay = limiter->delay;

    float* tpBuf = limiter->tpBuf;
    unsigned int tpBufPos = limiter->tpBufPos;

    TDLSmoothType smoothType = limiter->smoothType;
    double smoothState0 = limiter->smoothState0;

    float* pSample = samples;
    float* pChannel;
    unsigned int incPSample = channels, incPChannel = 1;

    if (!limiter->audioIsInterleaved) {
      incPSample = 1;
      incPChannel = nSamples;
    }

    for (i = 0; i < nSamples; i++) {
      tmp = threshold;

      if (limiter->tpFlag == 1) {
        pChannel = pSample;
        for (c = 0; c < channels; c++) {
          float s0 = 0, s1 = 0, s2 = 0, s3 = 0;

          tpBuf[tpBufPos * channels + c] = *pChannel;

          for (j = 0; j < TP_FILT_LEN; j++) {
            unsigned int k = (tpBufPos + TP_FILT_LEN - j) % TP_FILT_LEN;
            float s = tpBuf[k * channels + c];

            s0 += tpFilt[0][j] * s;
            s1 += tpFilt[1][j] * s;
            s2 += tpFilt[2][j] * s;
            s3 += tpFilt[3][j] * s;
          }

          tmp = max(tmp, (float)fabs(s0));
          tmp = max(tmp, (float)fabs(s1));
          tmp = max(tmp, (float)fabs(s2));
          tmp = max(tmp, (float)fabs(s3));

          pChannel += incPChannel;
        }

        tpBufPos++;
        if (tpBufPos >= TP_FILT_LEN) tpBufPos = 0;
      } else {
        pChannel = pSample;
        for (c = 0; c < channels; c++) {
          tmp = max(tmp, (float)fabs(*pChannel));
          pChannel += incPChannel;
        }
      }

      old = maxBuf[maxBufIdx];
      maxBuf[maxBufIdx] = tmp;

      if (tmp >= max) {
        max = tmp;
      } else if (old < max) {
      } else {
        max = maxBuf[0];
        for (j = 1; j <= attack; j++) {
          if (maxBuf[j] > max) max = maxBuf[j];
        }
      }
      maxBufIdx++;
      if (maxBufIdx >= attack + 1) maxBufIdx = 0;

      if (max > threshold) {
        gain = threshold / max;
      } else {
        gain = 1;
      }

      switch (smoothType) {
        case TDL_EXPONENTIAL:

          if (gain < smoothState0) {
            cor = min(cor, (gain - 0.1f * (float)smoothState0) * 1.11111111f);
          } else {
            cor = gain;
          }

          if (cor < smoothState0) {
            smoothState0 = attackConst * (smoothState0 - cor) + cor;
            smoothState0 = max(smoothState0, gain);
          } else {
            smoothState0 = releaseConst * (smoothState0 - cor) + cor;
          }

          gain = (float)smoothState0;
          break;

        default:
          return TDLIMIT_INVALID_SMOOTHTYPE;
      }

      pChannel = pSample;
      for (j = 0; j < channels; j++) {
        if (delay) {
          tmp = delayBuf[delayBufIdx * channels + j];
          delayBuf[delayBufIdx * channels + j] = *pChannel;
        } else {
          tmp = *pChannel;
        }

        tmp *= gain;
        if (tmp > threshold) tmp = threshold;
        if (tmp < -threshold) tmp = -threshold;

        if (limiter->bReturnGains == 0) {
          *pChannel = tmp;
        } else {
          *pChannel = gain;
        }

        pChannel += incPChannel;
      }
      delayBufIdx++;
      if (delayBufIdx >= delay) delayBufIdx = 0;

      if (gain < minGain) minGain = gain;

      pSample += incPSample;
    }

    limiter->max = max;
    limiter->maxBufIdx = maxBufIdx;
    limiter->cor = cor;
    limiter->delayBufIdx = delayBufIdx;
    limiter->tpBufPos = tpBufPos;
    limiter->smoothState0 = smoothState0;

    limiter->minGain = minGain;

    return TDLIMIT_OK;
  }
}

unsigned int getLimiterDelay(TDLimiterPtr limiter) {
  return limiter->delay;
}

unsigned int calculateLimiterDelay(float attackMs, unsigned int sampleRate, unsigned int truePeakLimiting) {
  unsigned int delay = (unsigned int)(attackMs * sampleRate / 1000);
  if (truePeakLimiting)
    delay += TP_FILT_LEN / 2;
  return delay;
}

float getLimiterMaxGainReduction(TDLimiterPtr limiter) {
  return -20 * (float)log10(limiter->minGain);
}

int setLimiterNChannels(TDLimiterPtr limiter, unsigned int nChannels) {
  if (nChannels > limiter->maxChannels) return TDLIMIT_INVALID_PARAMETER;

  limiter->channels = nChannels;
  resetLimiter(limiter);

  return TDLIMIT_OK;
}

int setLimiterSampleRate(TDLimiterPtr limiter, unsigned int sampleRate) {
  unsigned int attack;

  if (sampleRate > limiter->maxSampleRate) return TDLIMIT_INVALID_PARAMETER;

  attack = (unsigned int)(limiter->attackMs * sampleRate / 1000);

  limiter->attack = attack;
  if (attack)
    limiter->attackConst = (float)pow(0.1, 1.0 / (attack + 1));
  else
    limiter->attackConst = 0.0f;
  limiter->releaseConst = (float)pow(0.1, 1.0 / (limiter->releaseMs * sampleRate / 1000 + 1));
  limiter->sampleRate = sampleRate;

  if (limiter->tpFlag) {
    limiter->delay = attack + TP_FILT_LEN / 2;
  } else {
    limiter->delay = attack;
  }

  resetLimiter(limiter);

  return TDLIMIT_OK;
}

int setLimiterAttack(TDLimiterPtr limiter, float attackMs) {
  if (attackMs > limiter->maxAttackMs) return TDLIMIT_INVALID_PARAMETER;

  limiter->attack = (unsigned int)(attackMs * limiter->sampleRate / 1000);

  if (limiter->attack)
    limiter->attackConst = (float)pow(0.1, 1.0 / (limiter->attack + 1));
  else
    limiter->attackConst = 0.0f;
  limiter->attackMs = attackMs;

  if (limiter->tpFlag) {
    limiter->delay = limiter->attack + TP_FILT_LEN / 2;
  } else {
    limiter->delay = limiter->attack;
  }

  resetLimiter(limiter);

  return TDLIMIT_OK;
}

int setLimiterRelease(TDLimiterPtr limiter, float releaseMs) {
  limiter->releaseConst = (float)pow(0.1, 1.0 / (releaseMs * limiter->sampleRate / 1000 + 1));

  limiter->releaseMs = releaseMs;

  return TDLIMIT_OK;
}

int setLimiterThreshold(TDLimiterPtr limiter, float threshold) {
  limiter->threshold = threshold;

  return TDLIMIT_OK;
}

int setLimiterInterleaved(TDLimiterPtr limiter, TDLIsInterleaved audioIsInterleaved) {
  limiter->audioIsInterleaved = audioIsInterleaved;

  return TDLIMIT_OK;
}

int setTruePeak(TDLimiterPtr limiter) {
  limiter->tpFlag = 1;

  iisFree(limiter->delayBuf);
  limiter->delayBuf = NULL;
  limiter->delayBuf = (float*)iisCalloc((limiter->attack + TP_FILT_LEN / 2) * limiter->maxChannels, sizeof(float));
  limiter->tpBuf = (float*)iisCalloc(TP_FILT_LEN * limiter->maxChannels, sizeof(float));
  iisFree(limiter->maxBuf);
  limiter->maxBuf = NULL;
  limiter->maxBuf = (float*)iisCalloc(4 * (limiter->attack + 1), sizeof(float));

  if ((!limiter->tpBuf) || (!limiter->delayBuf) || (!limiter->maxBuf)) {
    destroyLimiter(limiter);
    return TDLIMIT_MEM_ERROR;
  }

  limiter->delay = limiter->attack + TP_FILT_LEN / 2;

  limiter->tpBufPos = 0;

  return TDLIMIT_OK;
}

int setReturnGains(TDLimiterPtr limiter, int value) {
  if (limiter == NULL) {
    return TDLIMIT_INVALID_HANDLE;
  }

  limiter->bReturnGains = value;

  return TDLIMIT_OK;
}
