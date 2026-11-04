
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
#include <stdlib.h>
#include <assert.h>

#include "iisutillib.h"
#include "freq_sca.h"
#include "sbr_misc.h"
#include "mathlib.h"

static int getStartFreq(const int fs,
                        const int start_freq,
                        int *k0,
                        CODEC_TYPE coreCodec,
                        SR_MODE srMode);

static int getStopFreq(const int fs, const int stop_freq, int *k0, SR_MODE srMode);

static int numberOfBands(int b_p_o, int start, int stop, float warp_factor);
static void CalcBands(int *diff, int start, int stop, int num_bands);
static HANDLE_ERROR_INFO modifyBands(int max_band, int *diff, int length);
static void cumSum(int start_value, int *diff, int length, int *start_adress);

int GetSbrStartFreqRAW(int startFreq,
                       int fs,
                       CODEC_TYPE coreCodec,
                       SR_MODE srMode) {
  int result = 0;

  if (startFreq < 0 || startFreq > 15)
    return -1;

  switch (coreCodec) {
    case CODEC_SAAC:
      if (getStartFreq(min(fs, 192000), startFreq, &result, coreCodec, srMode) == -1) {
        return -1;
      }
      break;
    default:
      assert(0);
      break;
  }
  if (srMode == QUAD_RATE) {
    return (int)((result * fs / (SBR_QMF_CHANNELS)) + 0.5f);
  } else {
    return (int)((result * fs / (SBR_QMF_CHANNELS * 2)) + 0.5f);
  }
}

int GetSbrStopFreqRAW(int stopFreq,
                      int fs,
                      CODEC_TYPE coreCodec,
                      SR_MODE srMode) {
  int result = 0;

  if (stopFreq < 0 || stopFreq > 13)
    return -1;

  switch (coreCodec) {
    case CODEC_SAAC:
      if (getStopFreq(min(fs, 192000), stopFreq, &result, srMode) == -1) {
        return -1;
      }
      break;
    default:
      assert(0);
      break;
  }

  if (srMode == QUAD_RATE) {
    return (int)((result * fs / (SBR_QMF_CHANNELS)) + 0.5f);
  } else {
    return (int)((result * fs / (SBR_QMF_CHANNELS * 2)) + 0.5f);
  }
}

static int const v_offset_default[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 28, 33};

static int const v_offset_aac_16000[] = {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
static int const v_offset_aac_22050[] = {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13};
static int const v_offset_aac_24000[] = {-5, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
static int const v_offset_aac_32000[] = {-6, -4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
static int const v_offset_aac_40000[] = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 19};
static int const v_offset_aac_48000[] = {-4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20};
static int const v_offset_aac_88200[] = {-2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24};

static int mapFreqUsac(int fs) {
  int fsMapped = fs;

  if (fs >= 0 && fs < 18783) {
    fsMapped = 16000;
  } else if (fs >= 18783 && fs < 23004) {
    fsMapped = 22050;
  } else if (fs >= 23004 && fs < 27713) {
    fsMapped = 24000;
  } else if (fs >= 27713 && fs < 35777) {
    fsMapped = 32000;
  } else if (fs >= 35777 && fs < 42000) {
    fsMapped = 40000;
  } else if (fs >= 42000 && fs < 46009) {
    fsMapped = 44100;
  } else if (fs >= 46009 && fs < 55426) {
    fsMapped = 48000;
  } else if (fs >= 55426 && fs < 75132) {
    fsMapped = 64000;
  } else if (fs >= 75132 && fs < 92017) {
    fsMapped = 88200;
  } else if (fs >= 92017) {
    fsMapped = 96000;
  } else {
    assert(0);
  }

  return fsMapped;
}

static int
getStartFreq(const int fs,
             const int start_freq,
             int *k0,
             CODEC_TYPE coreCodec,
             SR_MODE srMode) {
  int k0_min = 0;
  int const *v_offset;
  int fsMapped = fs;

  if (start_freq < 0 || start_freq > 15) {
    return -1;
  }
  if (k0 == NULL) {
    return -1;
  }
  v_offset = v_offset_default;

  switch (coreCodec) {
    case CODEC_SAAC:
      fsMapped = mapFreqUsac(fs);

      switch (fsMapped) {
        case 16000:
          v_offset = v_offset_aac_16000;
          break;
        case 19200:
          v_offset = v_offset_aac_22050;
          break;
        case 22050:
          v_offset = v_offset_aac_22050;
          break;
        case 24000:
          v_offset = v_offset_aac_24000;
          break;
        case 32000:
          v_offset = v_offset_aac_32000;
          break;
        case 40000:
          v_offset = v_offset_aac_40000;
          break;
        case 44100:
          v_offset = v_offset_aac_48000;
          break;
        case 48000:
          v_offset = v_offset_aac_48000;
          break;
        case 64000:
          v_offset = v_offset_aac_48000;
          break;
        case 88200:
        case 96000:
        case 128000:
        case 176400:
        case 192000:
          v_offset = v_offset_aac_88200;
          break;
        default:
          assert(0);
      }
      if (fsMapped < 32000) {
        if (srMode == QUAD_RATE) {
          k0_min = (int)(((float)(3000 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
        } else {
          k0_min = (int)(((float)(3000 * 2 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
        }
      } else {
        if (fsMapped < 64000) {
          if (srMode == QUAD_RATE) {
            k0_min = (int)(((float)(4000 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
          } else {
            k0_min = (int)(((float)(4000 * 2 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
          }
        } else {
          if (srMode == QUAD_RATE) {
            k0_min = (int)(((float)(5000 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
          } else {
            k0_min = (int)(((float)(5000 * 2 * SBR_QMF_CHANNELS) / fsMapped) + 0.5);
          }
        }
      }
      break;
    default:
      break;
  }
  *k0 = k0_min + v_offset[start_freq];
  return 0;
}

static int
getStopFreq(const int fs, const int stop_freq, int *k2, SR_MODE srMode) {
  int result, i;
  int v_stop_freq[14];
  int k1_min;
  int v_dstop[13];

  if (stop_freq < 0 || stop_freq > 15) {
    return -1;
  }
  if (k2 == NULL) {
    return -1;
  }

  if (fs < 32000) {
    if (srMode == QUAD_RATE) {
      k1_min = (int)(((float)(6000 * SBR_QMF_CHANNELS) / fs) + 0.5);
    } else {
      k1_min = (int)(((float)(6000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
    }
  } else {
    if (fs < 64000) {
      if (srMode == QUAD_RATE) {
        k1_min = (int)(((float)(8000 * SBR_QMF_CHANNELS) / fs) + 0.5);
      } else {
        k1_min = (int)(((float)(8000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
      }
    } else {
      if (srMode == QUAD_RATE) {
        k1_min = (int)(((float)(10000 * SBR_QMF_CHANNELS) / fs) + 0.5);
      } else {
        k1_min = (int)(((float)(10000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
      }
    }
  }

  for (i = 0; i <= 13; i++) {
    v_stop_freq[i] = (int)(k1_min * pow(64.0 / k1_min, i / 13.0) + 0.5);
  }

  for (i = 0; i <= 12; i++) {
    v_dstop[i] = v_stop_freq[i + 1] - v_stop_freq[i];
  }

  shellsortINT(v_dstop, 13);

  result = k1_min;
  for (i = 0; i < stop_freq; i++) {
    result = result + v_dstop[i];
  }
  *k2 = result;

  return 0;
}

HANDLE_ERROR_INFO
FindStartAndStopBand(const int samplingFreq,
                     const int startFreq,
                     const int stopFreq,
                     const SR_MODE sampleRateMode,
                     int *k0,
                     int *k2,
                     CODEC_TYPE coreCodec) {
  int samplingFreqM = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      samplingFreqM = samplingFreq;
      break;
    default:
      assert(0);
      break;
  }

  if (k0 == NULL || k2 == NULL) {
    return iisUtil_ERROR(CDI, "Pointer k0 or k2 is NULL");
  }

  if (sampleRateMode == QUAD_RATE) {
    if (getStartFreq(samplingFreqM / 2, startFreq, k0, coreCodec, sampleRateMode) == -1) {
      return iisUtil_ERROR(CDI, "startFreq must be between 0 and 15");
    }
  } else {
    if (getStartFreq(samplingFreqM, startFreq, k0, coreCodec, sampleRateMode) == -1) {
      return iisUtil_ERROR(CDI, "startFreq must be between 0 and 15");
    }
  }

  if ((sampleRateMode == DUAL_RATE) &&
      (samplingFreqM / (float)4.0 <
       *k0 * samplingFreqM / (float)(2.0 * SBR_QMF_CHANNELS))) {
    return iisUtil_ERROR(CDI,
                         "Please raise the cross-over frequency and/or lower the number\n"
                         "of target bands per octave (or lower the sampling frequency)!");
  }

  if ((sampleRateMode == QUAD_RATE) &&
      (samplingFreq / (float)8.0 <
       *k0 * samplingFreq / (float)(2.0 * SBR_QMF_CHANNELS))) {
    return iisUtil_ERROR(CDI,
                         "Please raise the cross-over frequency and/or lower the number\n"
                         "of target bands per octave (or lower the sampling frequency)!");
  }

  if (stopFreq < 14) {
    if (sampleRateMode == QUAD_RATE) {
      if (getStopFreq(samplingFreqM / 2, stopFreq, k2, sampleRateMode) == -1) {
        return iisUtil_ERROR(CDI, "stopFreq must be between 0 and 15");
      }
    } else {
      if (getStopFreq(samplingFreqM, stopFreq, k2, sampleRateMode) == -1) {
        return iisUtil_ERROR(CDI, "stopFreq must be between 0 and 15");
      }
    }
  } else if (stopFreq == 14) {
    *k2 = 2 * *k0;
  } else if (stopFreq == 15) {
    *k2 = 3 * *k0;
  } else {
    return iisUtil_ERROR(CDI, "stopFreq must be between 0 and 15");
  }

  if (*k2 > SBR_QMF_CHANNELS) {
    *k2 = SBR_QMF_CHANNELS;
  }

  if (sampleRateMode == QUAD_RATE) {
    if ((*k2 - *k0) > MAX_FREQ_COEFFS_SBR_RATIO_16_64)
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS");

    if ((*k2 - *k0) <= 0)
      return iisUtil_ERROR(CDI, "Number of bands is negative");

    if ((samplingFreqM == 44100) && ((*k2 - *k0) > MAX_FREQ_COEFFS_SBR_RATIO_16_64_FS44100))
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS for fs=44.1kHz");

    if ((samplingFreqM >= 48000) && ((*k2 - *k0) > MAX_FREQ_COEFFS_SBR_RATIO_16_64_FS48000))
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS for fs>=48kHz");
  } else {
    if ((*k2 - *k0) > MAX_FREQ_COEFFS)
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS");

    if ((*k2 - *k0) <= 0)
      return iisUtil_ERROR(CDI, "Number of bands is negative");

    if ((samplingFreqM == 32000) && ((*k2 - *k0) > MAX_FREQ_COEFFS_FS32000))
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS for fs=32kHz");

    if ((samplingFreqM == 44100) && ((*k2 - *k0) > MAX_FREQ_COEFFS_FS44100))
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS for fs=44.1kHz");

    if ((samplingFreqM >= 48000) && ((*k2 - *k0) > MAX_FREQ_COEFFS_FS48000))
      return iisUtil_ERROR(CDI, "Number of bands exceeds valid range of MAX_FREQ_COEFFS for fs>=48kHz");
  }

  return noError;
}

HANDLE_ERROR_INFO
UpdateFreqScale(int *v_k_master, int *h_num_bands,
                const int k0, const int k2,
                const int freqScale,
                const int alterScale,
                SR_MODE srMode) {
  HANDLE_ERROR_INFO h_error = noError;

  int b_p_o = 0;
  float warp = 0;
  int dk = 0;

  int k1 = 0, i;
  int num_bands0;
  int num_bands1;
  int diff_tot[MAX_OCTAVE + MAX_SECOND_REGION] = {0};
  int *diff0 = diff_tot;
  int *diff1 = diff_tot + MAX_OCTAVE;
  int k2_achived;
  int k2_diff;
  int incr = 0;

  if (freqScale == 1)
    b_p_o = 12;
  if (freqScale == 2)
    b_p_o = 10;
  if (freqScale == 3)
    b_p_o = 8;

  if (freqScale > 0) {
    if (alterScale == 0)
      warp = 1.0;
    else
      warp = 1.3f;

    if ((srMode == QUAD_RATE) && (k0 < b_p_o)) {
      b_p_o = ((int)(k0 / 2)) * 2;
    }

    if (((float)k2 / k0) > 2.2449) {
      k1 = 2 * k0;

      num_bands0 = numberOfBands(b_p_o, k0, k1, 1.0);
      num_bands1 = numberOfBands(b_p_o, k1, k2, warp);

      CalcBands(diff0, k0, k1, num_bands0);
      shellsortINT(diff0, num_bands0);

      if (diff0[0] == 0) {
        return iisUtil_ERROR(CDI,
                             "Please raise the cross-over frequency and/or lower the number\n"
                             "of target bands per octave (or lower the sampling frequency)!");
      }

      cumSum(k0, diff0, num_bands0, v_k_master);

      CalcBands(diff1, k1, k2, num_bands1);
      shellsortINT(diff1, num_bands1);
      if (diff0[num_bands0 - 1] > diff1[0]) {
        h_error = modifyBands(diff0[num_bands0 - 1], diff1, num_bands1);
        if (h_error != noError)
          return handBack(h_error);
      }

      cumSum(k1, diff1, num_bands1, &v_k_master[num_bands0]);
      *h_num_bands = num_bands0 + num_bands1;

    } else {
      k1 = k2;

      num_bands0 = numberOfBands(b_p_o, k0, k1, 1.0);
      CalcBands(diff0, k0, k1, num_bands0);
      shellsortINT(diff0, num_bands0);

      if (diff0[0] == 0) {
        return iisUtil_ERROR(CDI,
                             "Please raise the cross-over frequency and/or lower the number\n"
                             "of target bands per octave (or lower the sampling frequency)!");
      }

      cumSum(k0, diff0, num_bands0, v_k_master);
      *h_num_bands = num_bands0;
    }
  } else {
    if (alterScale == 0) {
      dk = 1;
      num_bands0 = 2 * (int)((float)(k2 - k0) / (dk * 2));
    } else {
      dk = 2;
      num_bands0 = 2 * (int)((float)(k2 - k0) / (dk * 2) + 0.5);
    }

    k2_achived = k0 + num_bands0 * dk;
    k2_diff = k2 - k2_achived;

    for (i = 0; i < num_bands0; i++)
      diff_tot[i] = dk;

    if (k2_diff < 0) {
      incr = 1;
      i = 0;
    }

    if (k2_diff > 0) {
      incr = -1;
      i = num_bands0 - 1;
    }

    while (k2_diff != 0) {
      diff_tot[i] = diff_tot[i] - incr;
      i = i + incr;
      k2_diff = k2_diff + incr;
    }

    cumSum(k0, diff_tot, num_bands0, v_k_master);
    *h_num_bands = num_bands0;
  }

  if (*h_num_bands < 1)
    return iisUtil_ERROR(CDI, "To small sbr area");

  return noError;
}

static int
numberOfBands(int b_p_o, int start, int stop, float warp_factor) {
  int result = 0;
  result = 2 * (int)(b_p_o * log((float)(stop) / start) / (2 * log(2.0) * warp_factor) + 0.5);

  return (result);
}

static void
CalcBands(int *diff, int start, int stop, int num_bands) {
  int i;
  int previous;
  int current;

  previous = start;
  for (i = 1; i <= num_bands; i++) {
    current = (int)((start * pow((float)stop / start, (float)i / num_bands)) + 0.5);
    diff[i - 1] = current - previous;
    previous = current;
  }
}

static void
cumSum(int start_value, int *diff, int length, int *start_adress) {
  int i;
  start_adress[0] = start_value;
  for (i = 1; i <= length; i++)
    start_adress[i] = start_adress[i - 1] + diff[i - 1];
}

static HANDLE_ERROR_INFO
modifyBands(int max_band_previous, int *diff, int length) {
  int change = max_band_previous - diff[0];

  if (change > (diff[length - 1] - diff[0]) / 2)
    change = (diff[length - 1] - diff[0]) / 2;

  diff[0] += change;
  diff[length - 1] -= change;
  shellsortINT(diff, length);

  return (noError);
}

HANDLE_ERROR_INFO
UpdateHiRes(int *h_hires, int *num_hires, int *v_k_master,
            int num_master, int *xover_band, SR_MODE drOrSr, CODEC_TYPE coreCodec) {
  int i;
  int coreStopBand;
  int max1, max2;
  int nBitsXOverBand;
  nBitsXOverBand = (coreCodec == CODEC_SAAC) ? SI_SBR_XOVER_BAND_BITS_SAAC : SI_SBR_XOVER_BAND_BITS;

  if (drOrSr == DUAL_RATE)
    coreStopBand = SBR_QMF_CHANNELS >> 1;
  else if (drOrSr == QUAD_RATE)
    coreStopBand = SBR_QMF_CHANNELS >> 2;
  else
    coreStopBand = SBR_QMF_CHANNELS;

  if ((v_k_master[*xover_band] > coreStopBand) ||
      (*xover_band > num_master) ||
      ((*xover_band >> nBitsXOverBand) > 0)) {
    WARN("xover_band error, too big for this startFreq! Will be clipped.\n");

    max1 = 0;
    max2 = num_master;
    while ((v_k_master[max1 + 1] < coreStopBand) &&
           ((max1 + 1) < max2)) {
      max1++;
    }

    *xover_band = min(max1, (1 << nBitsXOverBand) - 1);
    ;
  }

  *num_hires = num_master - *xover_band;
  for (i = *xover_band; i <= num_master; i++) {
    h_hires[i - *xover_band] = v_k_master[i];
  }

  return noError;
}

void UpdateLoRes(int *h_lores, int *num_lores, int *h_hires, int num_hires) {
  int i;

  if (num_hires % 2 == 0) {
    *num_lores = num_hires / 2;

    for (i = 0; i <= *num_lores; i++)
      h_lores[i] = h_hires[i * 2];

  } else {
    *num_lores = (num_hires + 1) / 2;

    h_lores[0] = h_hires[0];
    for (i = 1; i <= *num_lores; i++) {
      h_lores[i] = h_hires[i * 2 - 1];
    }
  }
}

int GetStartMin(const int fs, SR_MODE srMode, CODEC_TYPE coreCodec) {
  int k0_min = 0;

  int fsMapped = fs;
  switch (coreCodec) {
    case CODEC_SAAC:
      fsMapped = mapFreqUsac(fs);
      break;
    default:
      break;
  }

  if (fsMapped < 32000) {
    k0_min = 3000 * 2 * SBR_QMF_CHANNELS;
  } else if (fsMapped < 64000) {
    k0_min = 4000 * 2 * SBR_QMF_CHANNELS;
  } else {
    k0_min = 5000 * 2 * SBR_QMF_CHANNELS;
  }
  if (srMode == QUAD_RATE) {
    k0_min /= 2;
  }
  k0_min = (int)((float)k0_min / fsMapped + 0.5f);

  return k0_min;
}

int GetStopMin(const int fs, SR_MODE srMode) {
  int k2_min = 0;
  if (fs < 32000) {
    if (srMode == QUAD_RATE) {
      k2_min = (int)(((float)(6000 * SBR_QMF_CHANNELS) / fs) + 0.5);
    } else
      k2_min = (int)(((float)(6000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
  } else {
    if (fs < 64000) {
      if (srMode == QUAD_RATE) {
        k2_min = (int)(((float)(8000 * SBR_QMF_CHANNELS) / fs) + 0.5);
      } else
        k2_min = (int)(((float)(8000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
    } else {
      if (srMode == QUAD_RATE) {
        k2_min = (int)(((float)(10000 * SBR_QMF_CHANNELS) / fs) + 0.5);
      } else
        k2_min = (int)(((float)(10000 * 2 * SBR_QMF_CHANNELS) / fs) + 0.5);
    }
  }
  return k2_min;
}

int const *GetVOffset(const int fs,
                      CODEC_TYPE coreCodec) {
  int const *v_offset = v_offset_default;
  int fsMapped = 0;
  switch (coreCodec) {
    case CODEC_SAAC:
      fsMapped = mapFreqUsac(fs);
      break;
    default:
      break;
  }

  switch (fsMapped) {
    case 16000:
      v_offset = v_offset_aac_16000;
      break;
    case 19200:
      v_offset = v_offset_aac_22050;
      break;
    case 22050:
      v_offset = v_offset_aac_22050;
      break;
    case 24000:
      v_offset = v_offset_aac_24000;
      break;
    case 32000:
      v_offset = v_offset_aac_32000;
      break;
    case 40000:
      v_offset = v_offset_aac_40000;
      break;
    case 44100:
    case 48000:
    case 64000:
      v_offset = v_offset_aac_48000;
      break;
    case 88200:
    case 96000:
      v_offset = v_offset_aac_88200;
      break;
    default:
      assert(0);
      break;
  }

  return v_offset;
}

void GetStopDkSort(int k2_min, int v_dstop[13]) {
  int i = 0;
  int v_stop_freq[14] = {0};

  if (v_dstop == NULL) return;

  for (i = 0; i <= 13; i++) {
    v_stop_freq[i] = (int)(k2_min * pow(64.0 / k2_min, i / 13.0) + 0.5);
  }

  for (i = 0; i <= 12; i++) {
    v_dstop[i] = v_stop_freq[i + 1] - v_stop_freq[i];
  }
  shellsortINT(v_dstop, 13);
  return;
}
