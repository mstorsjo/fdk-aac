
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

#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include "aacenc_internal.h"
#include "glob_con.h"
#include "mathlib.h"
#include "psy_configuration.h"
#include "psy_const.h"

typedef struct {
  int sfbCnt;
  int sfbWidth[MAX_SFB];
} IISAACFENC_SFB_PARAM;

typedef struct {
  int sampleRate;
  const IISAACFENC_SFB_PARAM *paramLong;
  const IISAACFENC_SFB_PARAM *paramShort;
} IISAACFENC_SFB_INFO_TAB;

static const IISAACFENC_SFB_PARAM p_8000_long_1024 = {
    40,
    {
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        12,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        20,
        20,
        20,
        20,
        24,
        24,
        24,
        28,
        28,
        32,
        36,
        36,
        40,
        44,
        48,
        52,
        56,
        60,
        64,
        80,
    }};
static const IISAACFENC_SFB_PARAM p_8000_short_128 = {
    15,
    {4, 4, 4, 4, 4, 4, 4, 8, 8, 8,
     8, 12, 16, 20, 20}};

static const IISAACFENC_SFB_PARAM p_11025_long_1024 = {
    43,
    {8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
     8, 12, 12, 12, 12, 12, 12, 12, 12, 12,
     16, 16, 16, 16, 20, 20, 20, 24, 24, 28,
     28, 32, 36, 40, 40, 44, 48, 52, 56, 60,
     64, 64, 64}};
static const IISAACFENC_SFB_PARAM p_11025_short_128 = {
    15,
    {4, 4, 4, 4, 4, 4, 4, 4, 8, 8,
     12, 12, 16, 20, 20}};

static const IISAACFENC_SFB_PARAM p_22050_long_1024 = {
    47,
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 8, 8, 8, 8, 8, 8, 8, 8, 8,
     8, 12, 12, 12, 12, 16, 16, 16, 20, 20,
     24, 24, 28, 28, 32, 36, 36, 40, 44, 48,
     52, 52, 64, 64, 64, 64, 64}};
static const IISAACFENC_SFB_PARAM p_22050_short_128 = {
    15,
    {4, 4, 4, 4, 4, 4, 4, 8, 8, 8,
     12, 12, 16, 16, 20}};

static const IISAACFENC_SFB_PARAM p_32000_long_1024 = {
    51,
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     8, 8, 8, 8, 8, 8, 8, 12, 12, 12,
     12, 16, 16, 20, 20, 24, 24, 28, 28, 32,
     32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
     32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
     32}};
static const IISAACFENC_SFB_PARAM p_32000_short_128 = {
    14,
    {4, 4, 4, 4, 4, 8, 8, 8, 12, 12,
     12, 16, 16, 16}};

static const IISAACFENC_SFB_PARAM p_44100_long_1024 = {
    49,
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     8, 8, 8, 8, 8, 8, 8, 12, 12, 12,
     12, 16, 16, 20, 20, 24, 24, 28, 28, 32,
     32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
     32, 32, 32, 32, 32, 32, 32, 32, 96}};
static const IISAACFENC_SFB_PARAM p_44100_short_128 = {
    14,
    {4, 4, 4, 4, 4, 8, 8, 8, 12, 12,
     12, 16, 16, 16}};

static const IISAACFENC_SFB_PARAM p_64000_long_1024 = {
    47,
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 8, 8, 8, 8, 12, 12,
     12, 16, 16, 16, 20, 24, 24, 28, 36, 40,
     40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
     40, 40, 40, 40, 40, 40, 40}};
static const IISAACFENC_SFB_PARAM p_64000_short_128 = {
    12,
    {4, 4, 4, 4, 4, 4, 8, 8, 8, 16,
     28, 36}};

static const IISAACFENC_SFB_PARAM p_88200_long_1024 = {
    41,
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 8, 8, 8, 8, 8, 12,
     12, 12, 12, 12, 16, 16, 24, 28, 36, 44,
     64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
     64}};
static const IISAACFENC_SFB_PARAM p_88200_short_128 = {
    12,
    {4, 4, 4, 4, 4, 4, 8, 8, 8, 16,
     28, 36}};

static const IISAACFENC_SFB_INFO_TAB sfbInfoTab[] = {
    {9390, &p_8000_long_1024, &p_8000_short_128},
    {18782, &p_11025_long_1024, &p_11025_short_128},
    {27712, &p_22050_long_1024, &p_22050_short_128},
    {37565, &p_32000_long_1024, &p_32000_short_128},
    {55425, &p_44100_long_1024, &p_44100_short_128},
    {75131, &p_64000_long_1024, &p_64000_short_128},
    {INT_MAX, &p_88200_long_1024, &p_88200_short_128}};

static const float max_barc = 24.0f;
static const float maskLow = 30.0f;
static const float maskHigh = 15.0f;

int iisaacfenc_snapToLowSfbBorder(const int sampleRate,
                                  const int blockType,
                                  const int granuleLength,
                                  const float desiredBandwidth,
                                  const float tol,
                                  float *adjBandwidth) {
  const IISAACFENC_SFB_PARAM *sfbParam = NULL;
  int error = 0;
  int i = 0;
  int specStartOffset = 0;
  int specStopOffset = 0;
  int line = 0;
  int tol_line = 0;
  float tmpAdjBandwidth;
  const int frameLengthLong = granuleLength;
  const int frameLengthShort = granuleLength / TRANS_FAC;

  tmpAdjBandwidth = desiredBandwidth;

  if (error == 0) {
    for (i = 0; i < (int)(sizeof(sfbInfoTab) / sizeof(IISAACFENC_SFB_INFO_TAB)); i++) {
      if (sfbInfoTab[i].sampleRate >= sampleRate) {
        switch (blockType) {
          case LONG_WINDOW:
          case START_WINDOW:
          case STOP_WINDOW:
            sfbParam = sfbInfoTab[i].paramLong;
            line = (int)(2.0f * desiredBandwidth / sampleRate * frameLengthLong);
            break;
          case SHORT_WINDOW:
            sfbParam = sfbInfoTab[i].paramShort;
            line = (int)(2.0f * desiredBandwidth / sampleRate * frameLengthShort);
            break;
          default:
            assert(0);
            break;
        }
        break;
      }
    }
    if (sfbParam == NULL) {
      error = 1;
    }
  }

  if (error == 0) {
    i = 0;
    specStartOffset = 0;
    specStopOffset = sfbParam->sfbWidth[i];
    while ((specStopOffset <= line) && (i < sfbParam->sfbCnt)) {
      i++;
      specStartOffset = specStopOffset;
      specStopOffset += sfbParam->sfbWidth[i];
    }
    if (line < specStartOffset) {
      error = 1;
    }
  }

  if (error == 0) {
    tol_line = (int)(tol * sfbParam->sfbWidth[i]);
    if ((line - specStartOffset) <= tol_line) {
      tmpAdjBandwidth = 0.5f * specStartOffset * sampleRate * (blockType == SHORT_WINDOW ? TRANS_FAC : 1.0f) / frameLengthLong;
    } else {
      tmpAdjBandwidth = desiredBandwidth;
    }
  }

  if (adjBandwidth != NULL) {
    *adjBandwidth = tmpAdjBandwidth;
  } else {
    error = 1;
  }

  return error;
}

int iisaacfenc_initSfbTable(int sampleRate, int blockType, int granuleLength, int *sfbOffset, int *sfbCnt) {
  const IISAACFENC_SFB_PARAM *sfbParam = 0;
  int i, specStartOffset;
  int maxMdctLine = 0;

  if (granuleLength == 768) {
    sampleRate = (sampleRate * 4) / 3;
  }

  for (i = 0; i < (int)(sizeof(sfbInfoTab) / sizeof(IISAACFENC_SFB_INFO_TAB)); i++) {
    if (sfbInfoTab[i].sampleRate >= sampleRate) {
      switch (blockType) {
        case LONG_WINDOW:
        case START_WINDOW:
        case STOP_WINDOW:
          sfbParam = sfbInfoTab[i].paramLong;
          maxMdctLine = granuleLength;
          break;
        case SHORT_WINDOW:
          sfbParam = sfbInfoTab[i].paramShort;
          maxMdctLine = granuleLength / TRANS_FAC;
          break;
        default:
          assert(0);
          break;
      }
      break;
    }
  }
  if (sfbParam == 0)
    return (1);

  specStartOffset = 0;
  for (i = 0; i < sfbParam->sfbCnt; i++) {
    sfbOffset[i] = min(specStartOffset, maxMdctLine);
    specStartOffset += sfbParam->sfbWidth[i];
    if (sfbOffset[i] >= maxMdctLine) {
      break;
    }
  }
  *sfbCnt = i;
  sfbOffset[*sfbCnt] = min(specStartOffset, maxMdctLine);
  return (0);
}

static float iisaacfenc_BarcLineValue(int noOfLines, int fftLine, int samplingFreq) {
  float center_freq, temp, bvalFFTLine;

  center_freq = (float)fftLine * ((float)samplingFreq * 0.5f) / (float)noOfLines;
  temp = (float)atan(1.3333333e-4f * center_freq);
  bvalFFTLine = (float)(13.3f * atan(0.00076f * center_freq) + 3.5f * temp * temp);
  return (bvalFFTLine);
}

static void iisaacfenc_InitMinPCMResolution(int numPb,
                                            int *pbOffset,
                                            const float bitsPerSample,
                                            float *sfbPCMquantThreshold) {
  int i;
  float lineThreshold = 0.25f;

  if (bitsPerSample > 3.0f) {
    lineThreshold = 0.00390625f;
  }

  for (i = 0; i < numPb; i++) {
    sfbPCMquantThreshold[i] = lineThreshold *
                              (float)(pbOffset[i + 1] - pbOffset[i]) * NORM_PCM_ENERGY;
  }
}

static void iisaacfenc_initSpreading(int numPb,
                                     const float bitsPerSample,
                                     float *pbBarcValue,
                                     float *pbMaskLoFactor,
                                     float *pbMaskHiFactor) {
  int i;
  float maxMaskingFac = 1.0f;

  if (bitsPerSample > 3.0f) {
    maxMaskingFac = 0.5f;
  }

  pbMaskHiFactor[0] = pbMaskLoFactor[numPb - 1] = 0.0f;

  for (i = 1; i < numPb; i++) {
    float dbVal;
    dbVal = maskHigh * (pbBarcValue[i] - pbBarcValue[i - 1]);
    pbMaskHiFactor[i] = min(maxMaskingFac,
                            (float)pow(10.0f, -dbVal / 10.0f));

    dbVal = maskLow * (pbBarcValue[i] - pbBarcValue[i - 1]);
    pbMaskLoFactor[i - 1] = min(maxMaskingFac,
                                (float)pow(10.0f, -dbVal / 10.0f));
  }
}

static void iisaacfenc_initBarcValues(int numPb,
                                      int *pbOffset,
                                      int numLines,
                                      int samplingFrequency,
                                      float *pbBval) {
  int i;
  for (i = 0; i < numPb; i++) {
    pbBval[i] = min((iisaacfenc_BarcLineValue(numLines, pbOffset[i], samplingFrequency) +
                     iisaacfenc_BarcLineValue(numLines, pbOffset[i + 1], samplingFrequency)) *
                        0.5f,
                    max_barc);
  }
}

static void iisaacfenc_initMinSnr(const int bitrate,
                                  const int samplerate,
                                  const int numLines,
                                  const int *sfbOffset,
                                  const int sfbActive,
                                  const int blockType,
                                  const float bitsPerSample,
                                  float *sfbMinSnr) {
  int sfb;
  float barcFactor;
  float barcWidth;
  float pePerWindow, pePart;
  float snr;
  float snrLimitUp = 0.8f;
  float snrLimitLo = 0.003f;

  if (bitsPerSample > 3.0f) {
    snrLimitUp = 0.63f;
  }

  barcFactor = iisaacfenc_BarcLineValue(numLines, sfbOffset[sfbActive], samplerate) /
               (max_barc + 1.0f);

  pePerWindow = 1.18f * ((float)bitrate / samplerate * numLines);

  if (blockType == SHORT_WINDOW) pePerWindow *= 1.5f;

  for (sfb = 0; sfb < sfbActive; sfb++) {
    barcWidth = iisaacfenc_BarcLineValue(numLines, sfbOffset[sfb + 1], samplerate) -
                iisaacfenc_BarcLineValue(numLines, sfbOffset[sfb], samplerate);

    pePart = pePerWindow * 0.024f / barcFactor;

    pePart *= barcWidth;

    pePart /= (sfbOffset[sfb + 1] - sfbOffset[sfb]);
    snr = (float)pow(2.0f, pePart) - 1.5f;
    snr = 1.0f / max(snr, 1.0f);

    snr = min(snr, snrLimitUp);

    snr = max(snr, snrLimitLo);

    sfbMinSnr[sfb] = snr;
  }
}

static int getSfb(
    const int spectralLine,
    const int *sfb_offset,
    const int len) {
  int sfb;
  int retSfb = len;

  for (sfb = 0; sfb < len; sfb++) {
    if (sfb_offset[sfb] >= spectralLine) {
      retSfb = sfb;
      break;
    }
  }
  return retSfb;
}

int iisaacfenc_InitPsyConfiguration(int bitrate,
                                    int samplerate,
                                    float bandwidth,
                                    AACENC_CODEC_TYPE codecType,
                                    int blocktype,
                                    int granuleLength,
                                    const int useIS,
                                    PSY_CONFIGURATION *psyConf) {
  int sfb;
  const int frameLength = (blocktype != SHORT_WINDOW) ? granuleLength : granuleLength / TRANS_FAC;
  float sfbBarcVal[MAX_PB];
  float bitsPerSample = (float)bitrate / (bandwidth * 2.f);
  float ratio = (bitsPerSample > 3) ? 11.f + 6.f * bitsPerSample : 29.0f;

  if (psyConf == NULL) {
    return 1;
  }

  psyConf->granuleLength = granuleLength;
  psyConf->bitsPerLine = bitsPerSample;
  psyConf->allowIS = (useIS) && (bitsPerSample < 2.5f);
  psyConf->codecType = codecType;

  if (iisaacfenc_initSfbTable(samplerate, blocktype, granuleLength, psyConf->sfbOffset, &psyConf->sfbCnt))
    return (1);

  iisaacfenc_initBarcValues(psyConf->sfbCnt,
                            psyConf->sfbOffset,
                            psyConf->sfbOffset[psyConf->sfbCnt],
                            samplerate,
                            sfbBarcVal);

  iisaacfenc_InitMinPCMResolution(psyConf->sfbCnt,
                                  psyConf->sfbOffset,
                                  bitsPerSample,
                                  psyConf->sfbPcmQuantThreshold);

  iisaacfenc_initSpreading(psyConf->sfbCnt,
                           bitsPerSample,
                           sfbBarcVal,
                           psyConf->sfbMaskLowFactor,
                           psyConf->sfbMaskHighFactor);

  psyConf->ratio = (float)pow(10.0f, -(ratio / 10.0f));
  psyConf->maxAllowedIncreaseFactor = 2.0f;
  psyConf->minRemainingThresholdFactor = 0.01f;
  psyConf->clipEnergy = 1073741824.0f * NORM_PCM_ENERGY;

  if (samplerate >= 48000) {
    psyConf->sfbStartELW = (blocktype != SHORT_WINDOW) ? 38 : 10;
  } else if (samplerate >= 44100) {
    psyConf->sfbStartELW = (blocktype != SHORT_WINDOW) ? 40 : 11;
  } else {
    psyConf->sfbStartELW = 666;
  }

  psyConf->lowpassLine = (int)(2.0f * bandwidth / samplerate * frameLength);

  if (blocktype != SHORT_WINDOW) {
    psyConf->lowpassLFE = LFE_LOWPASS_LINE;
    psyConf->sfbActiveLFE = getSfb(psyConf->lowpassLFE, psyConf->sfbOffset, psyConf->sfbCnt);
  } else {
    psyConf->clipEnergy /= (TRANS_FAC * TRANS_FAC);
  }

  psyConf->sfbActive = getSfb(psyConf->lowpassLine, psyConf->sfbOffset, psyConf->sfbCnt);

  iisaacfenc_initMinSnr(bitrate,
                        samplerate,
                        psyConf->sfbOffset[psyConf->sfbCnt],
                        psyConf->sfbOffset,
                        psyConf->sfbActive,
                        blocktype,
                        bitsPerSample,
                        psyConf->sfbMinSnr);
  for (sfb = psyConf->sfbActive; sfb < psyConf->sfbCnt; sfb++) {
    psyConf->sfbMinSnr[sfb] = 0.8f;
  }

  psyConf->msMaskMerge = 1;

  return (0);
}
