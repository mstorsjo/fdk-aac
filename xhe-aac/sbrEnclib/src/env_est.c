
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
#include <string.h>
#include <assert.h>

#include "iisutillib.h"
#include "mathlib.h"
#include "cpuinfo.h"
#include "sbr.h"
#include "env_est.h"
#include "tran_det.h"
#include "qmflib.h"
#include "fram_gen.h"
#include "sbr_main.h"
#include "bit_sbr.h"
#include "cmondata.h"
#include "sib_det.h"

#define SBR_EMERGENCY_MODE_THRESHOLD 700

#define LOW_BITRATE_TUNING_THRESHOLD 7000
#define QUANT_ERROR_THRES 20

#if defined __GNUC__ && (__GNUC__ > 6) && !defined __clang__
#define EXPLICIT_FALLTHROUGH __attribute__((fallthrough));
#else
#define EXPLICIT_FALLTHROUGH
#endif

static void
lowbitrateEnergyAdjustment(float **YBuffer,
                           float *noiseLevelLoweringFactor,
                           int nNoiseEnvelopes,
                           int noNoiseBands,
                           int gmf_col,
                           int gmf_row) {
  int i, k;
  int noiseEnv = 0;
  int noiseBand = 0;
  float adjust = 0;

  for (i = 1; i < gmf_col; i++) {
    if (i % (gmf_col / nNoiseEnvelopes) == 0) noiseEnv++;

    for (k = 1; k < gmf_row; k++) {
      if (k % (gmf_row / noNoiseBands) == 0) noiseBand++;
      adjust = 1.f + 1.f - noiseLevelLoweringFactor[(noiseEnv * noNoiseBands) + noiseBand];
      YBuffer[i - 1][k] = YBuffer[i - 1][k] * adjust;
    }
    noiseBand = 0;
  }
}

static SBR_STEREO_MODE
stereoModeOverride(float quantError,
                   AMP_RES ampRes,
                   int *nrgLeft,
                   int *nrgRight,
                   int noEnv,
                   int *noScfBands) {
  int i, j;
  int zeroFlagLeft = 1;
  int zeroFlagRight = 1;

  float ca = (ampRes ? 1.5f : 3.0f);
  SBR_STEREO_MODE stereoMode = SBR_SWITCH_LRC;

  if (quantError * ca > QUANT_ERROR_THRES)
    stereoMode = SBR_LEFT_RIGHT;

  for (i = 0; i < noEnv; i++) {
    for (j = 0; j < noScfBands[i]; j++) {
      if (nrgLeft[i])
        zeroFlagLeft = 0;
      if (nrgRight[i])
        zeroFlagRight = 0;
    }
  }

  if (zeroFlagRight != zeroFlagLeft)
    stereoMode = SBR_LEFT_RIGHT;

  return stereoMode;
}

static float
mapPanorama(float nrgVal,
            int ampRes,
            float *quantError) {
  int i;
  float min_val, val;
  int panTable[2][10] = {{0, 2, 4, 6, 8, 12, 16, 20, 24},
                         {0, 2, 4, 8, 12}};
  int maxIndex[2] = {9, 5};

  int panIndex;
  int sign;

  sign = nrgVal > 0 ? 1 : -1;

  nrgVal = sign * nrgVal;

  min_val = 1e8;
  panIndex = 0;
  for (i = 0; i < maxIndex[ampRes]; i++) {
    val = (float)fabs(nrgVal - panTable[ampRes][i]);

    if (val < min_val) {
      min_val = val;
      panIndex = i;
    }
  }

  *quantError = min_val;
  return (float)(panTable[ampRes][maxIndex[ampRes] - 1] + sign * panTable[ampRes][panIndex]);
}

static void
sbrNoiseFloorLevelsQuantisation(int *iNoiseLevels,
                                float *NoiseLevels,
                                int coupling) {
  int i;
  float dummy;

  for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {
    float tmp;
    tmp = NoiseLevels[i] > 30 ? 30.0f : (float)ceil(NoiseLevels[i]);

    if (coupling) {
      tmp = tmp < -30 ? -30.0f : tmp;
      tmp = mapPanorama(tmp, 1, &dummy);
    }
    iNoiseLevels[i] = (int)tmp;
  }
}

static void
coupleNoiseFloor(float *noise_level_left,
                 float *noise_level_right,
                 CODEC_TYPE coreCodec) {
  int i;
  float temp1 = 0, temp2 = 0;

  for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {
    switch (coreCodec) {
      case CODEC_SAAC:
        temp1 = (float)pow(2.0f, (-noise_level_right[i] + NOISE_FLOOR_OFFSET));
        temp2 = (float)pow(2.0f, (-noise_level_left[i] + NOISE_FLOOR_OFFSET));
        noise_level_left[i] = (float)(NOISE_FLOOR_OFFSET - log((temp1 + temp2) * 0.5) * ILOG2);
        break;

      default:
        assert(0);
    }

    noise_level_right[i] = (float)(log(temp2 / temp1) * ILOG2);
  }
}

#ifndef _NOT_AVOID_FLOAT_DENORMALS
static const ALIGN_16_BYTE float _myMin = 1.e-9f;
#endif

void GetEnergyFromCplxQmfData_NoOpt(float **energyValues,
                                    float **realValues,
                                    float **imagValues,
                                    int numberBands,
                                    int numberCols) {
  int j, k;

  for (k = 0; k < numberCols; k++) {
    for (j = 0; j < numberBands; j++) {
      energyValues[k][j] = realValues[k][j] * realValues[k][j] +
                           imagValues[k][j] * imagValues[k][j];
#ifndef _NOT_AVOID_FLOAT_DENORMALS
      if (energyValues[k][j] < _myMin)
        energyValues[k][j] = 0.f;
#endif
    }
  }
}

static void
calculateSbrEnvelope(float **YBufferLeft,
                     float **YBufferRight,
                     const SBR_FRAME_INFO *frame_info,
                     int *sfb_nrgLeft,
                     int *sfb_nrgRight,
                     HANDLE_SBR_CONFIG_DATA h_con,
                     HANDLE_ENV_CHANNEL h_sbr,
                     SBR_STEREO_MODE stereoMode,
                     float *maxQuantError,
                     CODEC_TYPE coreCodec) {
  int env, band, k, l, count, m = 0;
  int no_of_bands, start_pos, stop_pos, li, ui;
  FREQ_RES freq_res;

  int ca = 2 - h_sbr->encEnvData.init_sbr_amp_res;

  int nEnvelopes = frame_info->nEnvelopes;
  int short_env = frame_info->shortEnv - 1;
  int timeStep = h_sbr->h_sbrExtractEnvelope->time_step;
  float quantError;
  int missingHarmonic = 0;

  if (stereoMode == SBR_COUPLING) {
    *maxQuantError = 0;
  }

  for (env = 0; env < nEnvelopes; env++) {
    start_pos = timeStep * frame_info->borders[env];
    stop_pos = timeStep * frame_info->borders[env + 1];
    assert((stop_pos - start_pos) > 0);
    freq_res = frame_info->freqRes[env];
    no_of_bands = h_con->nSfb[freq_res];

    if (env == short_env) {
      if (h_con->downScaleFactor > 1) {
        int stopPosRed = max(1, timeStep);
        if ((stop_pos - start_pos - stopPosRed) > 0) {
          stop_pos = stop_pos - stopPosRed;
        }
      } else {
        int stopPosRed = max(2, timeStep);
        if ((stop_pos - start_pos - stopPosRed) > 0) {
          stop_pos = stop_pos - stopPosRed;
        }
      }
    }

    for (band = 0; band < no_of_bands; band++) {
      float nrgRight = 0;
      float nrgLeft = 0;
      float temp;

      li = h_con->freqBandTable[freq_res][band];
      ui = h_con->freqBandTable[freq_res][band + 1];

      switch (h_con->coreCodec) {
        case CODEC_SAAC:

          missingHarmonic = 0;

          if (h_sbr->encEnvData.addHarmonicFlag) {
            if (freq_res == FREQ_RES_HIGH) {
              if (h_sbr->encEnvData.addHarmonic[band]) {
                missingHarmonic = 1;
              }
            } else {
              int i;
              int startBandHigh = 0;
              int stopBandHigh = 0;

              while (h_con->freqBandTable[FREQ_RES_HIGH][startBandHigh] < h_con->freqBandTable[FREQ_RES_LOW][band])
                startBandHigh++;
              while (h_con->freqBandTable[FREQ_RES_HIGH][stopBandHigh] < h_con->freqBandTable[FREQ_RES_LOW][band + 1])
                stopBandHigh++;

              for (i = startBandHigh; i < stopBandHigh; i++) {
                if (h_sbr->encEnvData.addHarmonic[i]) {
                  missingHarmonic = 1;
                }
              }
            }
          }

          if (missingHarmonic) {
            float tmpNrg = 0;

            count = (stop_pos - start_pos);

            for (l = start_pos; l < stop_pos; l++) {
              nrgLeft += YBufferLeft[l][li];
            }

            for (k = li + 1; k < ui; k++) {
              tmpNrg = 0;
              for (l = start_pos; l < stop_pos; l++) {
                tmpNrg += YBufferLeft[l][k];
              }

              if (tmpNrg > nrgLeft) {
                nrgLeft = tmpNrg;
              }
            }

            if (ui - li > 2) {
              nrgLeft = nrgLeft * 0.398107267f;
            } else {
              if (ui - li > 1) {
                nrgLeft = nrgLeft * 0.5f;
              }
            }

            if (stereoMode == SBR_COUPLING) {
              for (l = start_pos; l < stop_pos; l++) {
                nrgRight += YBufferRight[l][li];
              }

              for (k = li + 1; k < ui; k++) {
                tmpNrg = 0;

                for (l = start_pos; l < stop_pos; l++) {
                  tmpNrg += YBufferRight[l][k];
                }

                if (tmpNrg > nrgRight) {
                  nrgRight = tmpNrg;
                }
              }

              if (ui - li > 2) {
                nrgRight = nrgRight * 0.398107267f;
              } else {
                if (ui - li > 1) {
                  nrgRight = nrgRight * 0.5f;
                }
              }
            }

            if (stereoMode == SBR_COUPLING) {
              temp = nrgLeft;
              nrgLeft = (nrgRight + nrgLeft) * 0.5f;

              switch (coreCodec) {
                case CODEC_SAAC:
                  nrgRight = (temp + NORM_SBR_PCM_LEVEL_SQ) / (nrgRight * INORM_SBR_PCM_LEVEL_SQ + 1.0f);
                  break;

                default:
                  nrgRight = (temp + 1.0f) / (nrgRight + 1.0f);
                  break;
              }
            }

            break;
          }

          EXPLICIT_FALLTHROUGH
        default:

          if (freq_res == FREQ_RES_HIGH) {
            if (band == 0 && ui - li > 1) {
              li++;
            }
          } else {
            if (band == 0 && ui - li > 2) {
              li++;
            }
          }

          count = (stop_pos - start_pos) * (ui - li);

          for (k = li; k < ui; k++) {
            for (l = start_pos; l < stop_pos; l++) {
              nrgLeft += YBufferLeft[l][k];
            }
          }

          if (stereoMode == SBR_COUPLING) {
            for (k = li; k < ui; k++) {
              for (l = start_pos; l < stop_pos; l++) {
                nrgRight += YBufferRight[l][k];
              }
            }
          }

          if (stereoMode == SBR_COUPLING) {
            temp = nrgLeft;
            nrgLeft = (nrgRight + nrgLeft) * 0.5f;

            switch (coreCodec) {
              case CODEC_SAAC:
                nrgRight = (temp + NORM_SBR_PCM_LEVEL_SQ) / (nrgRight * INORM_SBR_PCM_LEVEL_SQ + 1.0f);
                break;

              default:
                nrgRight = (temp + 1.0f) / (nrgRight + 1.0f);
                break;
            }
          }
      }

      switch (coreCodec) {
        case CODEC_SAAC:
          nrgLeft = (float)((log(nrgLeft / (count * SBR_QMF_CHANNELS) + EPS_NS) * ILOG2) + 30);
          break;

        default:
          nrgLeft = (float)(log(nrgLeft / (count * SBR_QMF_CHANNELS) + EPS) * ILOG2);
          break;
      }

      nrgLeft = (nrgLeft < 0 ? 0 : nrgLeft);

      sfb_nrgLeft[m] = (int)(ca * nrgLeft + 0.5);

      if (stereoMode == SBR_COUPLING) {
        switch (coreCodec) {
          case CODEC_SAAC:
            nrgRight = (float)(log(nrgRight) * ILOG2 + 30);
            break;

          default:
            nrgRight = (float)(log(nrgRight) * ILOG2);
            break;
        }

        nrgRight = ca * nrgRight;

        nrgRight = mapPanorama(nrgRight, h_sbr->encEnvData.init_sbr_amp_res, &quantError);
        sfb_nrgRight[m] = (int)(nrgRight);

        if (quantError > *maxQuantError)
          *maxQuantError = quantError;
      }
      m++;
    }

    switch (h_con->coreCodec) {
      case CODEC_SAAC:

        if (h_con->useParametricCoding) {
          m -= no_of_bands;
          for (band = 0; band < no_of_bands; band++) {
            if (freq_res == FREQ_RES_HIGH && h_sbr->h_sbrExtractEnvelope->envelopeCompensation[band]) {
              sfb_nrgLeft[m] -= (int)(ca * abs(h_sbr->h_sbrExtractEnvelope->envelopeCompensation[band]));
            }
            if (sfb_nrgLeft[m] < 0)
              sfb_nrgLeft[m] = 0;
            m++;
          }
        }
        break;
      default:
        assert(0);
    }
  }
}

HANDLE_ERROR_INFO
ExtractSbrEnvelope(HANDLE_SBR_ENCODER hEnvEncoder,
                   SBR_STEREO_MODE stereoMode,
                   HANDLE_COMMON_DATA hCmonData, const int isSwitchingDecisionResultSpeech,
                   int const bUsacIndependenceFlag, SBR_EMERGENY_MODE emergencyMode)

{
  HANDLE_ERROR_INFO err = noError;
  HANDLE_SBR_CONFIG_DATA h_con = hEnvEncoder->sbrConfigData;
  HANDLE_SBR_HEADER_DATA sbrHeaderData = hEnvEncoder->sbrHeaderData;
  HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData = hEnvEncoder->sbrBitstreamData;
  HANDLE_ENV_CHANNEL *h_envChan = hEnvEncoder->hEnvChannel;
  int nChannelsSbr = hEnvEncoder->nChannelsSbr;

  int ch, i, j, c;
  int nEnvelopes[MAX_NUM_CHANNELS];
  int transient_info[MAX_NUM_CHANNELS][3] = {{0, 0, 0}, {0, 0, 0}};

  SBR_FRAME_INFO *frame_info[MAX_NUM_CHANNELS];

  FREQ_RES res[MAX_NUM_NOISE_VALUES];

  CODEC_TYPE coreCodec = h_con->coreCodec;

  int sfb_nrg[MAX_NUM_CHANNELS][MAX_NUM_ENVELOPE_VALUES];
  int sfb_nrg_coupling[MAX_NUM_CHANNELS][MAX_NUM_ENVELOPE_VALUES];

  float noiseFloor[MAX_NUM_CHANNELS][MAX_NUM_NOISE_VALUES];
  int noise_level[MAX_NUM_CHANNELS][MAX_NUM_NOISE_VALUES];
  int noise_level_coupling[MAX_NUM_CHANNELS][MAX_NUM_NOISE_VALUES];
  float maxQuantError = 0.0f;

  int fixfixGridGranularity = 8;

  float noiseLevelLoweringFactor[MAX_NUM_CHANNELS][MAX_NUM_NOISE_VALUES];

  int totalBits = 0;
  int prevEnergies[MAX_NUM_CHANNELS][MAX_FREQ_COEFFS];
  int prevNoiseEnergies[MAX_NUM_CHANNELS][MAX_NUM_NOISE_COEFFS];

  if (isSwitchingDecisionResultSpeech && (h_con->bitRate <= LOW_BITRATE_TUNING_THRESHOLD)) {
    fixfixGridGranularity = 4;
  }

  for (i = 0; i < MAX_NUM_NOISE_VALUES; i++)
    res[i] = FREQ_RES_HIGH;

  memset(noiseFloor, 0, sizeof(noiseFloor));

  hCmonData->sumTonalityQuotas = 0.0f;
  for (ch = 0; ch < nChannelsSbr; ch++) {
    hEnvEncoder->GetEnergyFromCplxQmfData_Ptr(
        h_envChan[ch]->h_sbrExtractEnvelope->YBuffer +
            h_envChan[ch]->h_sbrExtractEnvelope->YBufferWriteOffset,
        h_envChan[ch]->h_sbrExtractEnvelope->rBuffer +
            h_envChan[ch]->h_sbrExtractEnvelope->rBufferReadOffset,
        h_envChan[ch]->h_sbrExtractEnvelope->iBuffer +
            h_envChan[ch]->h_sbrExtractEnvelope->rBufferReadOffset,
        h_envChan[ch]->no_channels,
        h_envChan[ch]->h_sbrExtractEnvelope->no_cols);

    if (coreCodec == CODEC_SAAC) {
      h_envChan[ch]->hTonCorr->indexVector = h_envChan[ch]->hTonCorr->indexVectorDef;
      err = CalculateTonalityQuotas(&h_envChan[ch]->hTonCorr->lpcParamsPatch,
                                    h_envChan[ch]->h_sbrExtractEnvelope->rBuffer +
                                        h_envChan[ch]->h_sbrExtractEnvelope->rBufferWriteOffset,
                                    h_envChan[ch]->h_sbrExtractEnvelope->iBuffer +
                                        h_envChan[ch]->h_sbrExtractEnvelope->rBufferWriteOffset,
                                    coreCodec);
      if (err != noError) {
        return handBack(err);
      }
    }

    TransientDetect((float const *const *)(h_envChan[ch]->h_sbrExtractEnvelope->YBuffer + h_envChan[ch]->h_sbrExtractEnvelope->YBufferReadOffset),
                    h_envChan[ch]->h_sbrTransientDetector,
                    transient_info[ch],
                    h_envChan[ch]->h_sbrExtractEnvelope->time_step,
                    coreCodec, h_con->bSbr41);

    if (h_con->sibilantTuning) {
      extractSibilantBorderCandidates((const float *const *const)(h_envChan[ch]->h_sbrExtractEnvelope->YBuffer + h_envChan[ch]->h_sbrExtractEnvelope->YBufferReadOffset),
                                      h_envChan[ch]->h_sbrExtractEnvelope->no_cols,
                                      h_con->freqBandTable[FREQ_RES_HIGH][h_con->nSfb[FREQ_RES_HIGH]],
                                      &h_envChan[ch]->hSbrEnvFrame->sibBordersCnt,
                                      (int)(((float)6000 / (hEnvEncoder->sbrConfigData->sampleFreq / 2)) * SBR_QMF_CHANNELS + 0.5),
                                      &h_envChan[ch]->hSbrEnvFrame->lastTilt,
                                      h_envChan[ch]->h_sbrExtractEnvelope->YBufferWriteOffset);
    }

    switch (coreCodec) {
      case CODEC_SAAC:
        FrameSplitter((const float *const *const)(h_envChan[ch]->h_sbrExtractEnvelope->YBuffer + h_envChan[ch]->h_sbrExtractEnvelope->YBufferReadOffset),
                      h_envChan[ch]->h_sbrTransientDetector,
                      h_con->freqBandTable[1],
                      h_con->nSfb[1],
                      h_envChan[ch]->h_sbrExtractEnvelope->time_step,
                      h_envChan[ch]->h_sbrExtractEnvelope->no_cols,
                      transient_info[ch],
                      coreCodec,
                      isSwitchingDecisionResultSpeech,
                      &fixfixGridGranularity,
                      h_con->bitRate,
                      h_con->nChannelsInput);
        break;
      default:
        break;
    }

    err = CalculateTonalityQuotas(&h_envChan[ch]->hTonCorr->lpcParams,
                                  h_envChan[ch]->h_sbrExtractEnvelope->rBuffer +
                                      h_envChan[ch]->h_sbrExtractEnvelope->rBufferWriteOffset,
                                  h_envChan[ch]->h_sbrExtractEnvelope->iBuffer +
                                      h_envChan[ch]->h_sbrExtractEnvelope->rBufferWriteOffset,
                                  coreCodec);
    if (err != noError) {
      return handBack(err);
    }
  }

  {
    int nEstimatesTotal = 0;
    for (ch = 0; ch < nChannelsSbr; ch++)
      nEstimatesTotal += (h_envChan[ch]->hTonCorr->lpcParams.numberOfEstimatesPerFrame * h_envChan[ch]->hTonCorr->lpcParams.stopBand);
    hCmonData->sumTonalityQuotas /= nEstimatesTotal;
  }

  if (h_con->sibilantTuning) {
    for (ch = 0; ch < nChannelsSbr; ch++) {
      if ((h_envChan[ch]->hSbrEnvFrame->sibBordersCnt == 0) || (isSwitchingDecisionResultSpeech == 0)) {
        if (h_envChan[ch]->hSbrEnvFrame->sibBorderPrevFrame) {
          if ((h_con->bitRate <= LOW_BITRATE_TUNING_THRESHOLD) && isSwitchingDecisionResultSpeech) {
            fixfixGridGranularity = 4;
          } else {
            fixfixGridGranularity = 8;
          }

          transient_info[ch][0] = 1;
          transient_info[ch][1] = 0;
          h_envChan[ch]->hSbrEnvFrame->sibBorderPrevFrame = 0;
        }

      } else {
        if ((h_envChan[ch]->hSbrEnvFrame->frameClassOld == FIXFIX) || (h_envChan[0]->hSbrEnvFrame->frameClassOld == VARFIX)) {
          if ((h_con->bitRate <= LOW_BITRATE_TUNING_THRESHOLD) && isSwitchingDecisionResultSpeech) {
            fixfixGridGranularity = 4;
          } else {
            fixfixGridGranularity = 8;
          }

          transient_info[ch][0] = 1;
          transient_info[ch][1] = 0;
        }

        h_envChan[ch]->hSbrEnvFrame->sibBorderPrevFrame = 1;
      }
    }
  }

  if (stereoMode == SBR_COUPLING) {
    if (transient_info[0][1] && transient_info[1][1]) {
      transient_info[0][0] =
          min(transient_info[1][0], transient_info[0][0]);
      transient_info[1][0] = transient_info[0][0];
    } else {
      if (transient_info[0][1] && !transient_info[1][1]) {
        transient_info[1][0] = transient_info[0][0];
      } else {
        if (!transient_info[0][1] && transient_info[1][1])
          transient_info[0][0] = transient_info[1][0];
        else {
          transient_info[0][0] =
              max(transient_info[1][0], transient_info[0][0]);
          transient_info[1][0] = transient_info[0][0];
        }
      }
    }

    if (h_con->sibilantTuning) {
      if (h_envChan[1]->hSbrEnvFrame->sibBordersCnt > h_envChan[0]->hSbrEnvFrame->sibBordersCnt) {
        h_envChan[0]->hSbrEnvFrame->sibBordersCnt = h_envChan[1]->hSbrEnvFrame->sibBordersCnt;
        memcpy(h_envChan[0]->hSbrEnvFrame->sibBorders, h_envChan[1]->hSbrEnvFrame->sibBorders, sizeof(h_envChan[0]->hSbrEnvFrame->sibBorders));
      } else if (h_envChan[0]->hSbrEnvFrame->sibBordersCnt > h_envChan[1]->hSbrEnvFrame->sibBordersCnt) {
        h_envChan[1]->hSbrEnvFrame->sibBordersCnt = h_envChan[0]->hSbrEnvFrame->sibBordersCnt;
        memcpy(h_envChan[1]->hSbrEnvFrame->sibBorders, h_envChan[0]->hSbrEnvFrame->sibBorders, sizeof(h_envChan[1]->hSbrEnvFrame->sibBorders));
      }
    }
  }

  if (emergencyMode == 1) {
    fixfixGridGranularity = fixfixGridGranularity / 2;
  }

  if (h_con->sibilantTuning) {
    if ((h_envChan[0]->hSbrEnvFrame->sibBordersCnt == 0) || (isSwitchingDecisionResultSpeech == 0)) {
      frame_info[0] = FrameInfoGenerator(h_envChan[0]->hSbrEnvFrame,
                                         transient_info[0],
                                         sbrBitstreamData->rightBorderFIX,
                                         coreCodec,
                                         (h_con->bitRate <= LOW_BITRATE_TUNING_LIMIT) ? 1 : isSwitchingDecisionResultSpeech,
                                         fixfixGridGranularity);
      h_envChan[0]->encEnvData.hSbrBSGrid = h_envChan[0]->hSbrEnvFrame->hSbrGrid;

    } else {
      frame_info[0] = FrameInfoGeneratorSibilant(h_envChan[0]->hSbrEnvFrame,
                                                 transient_info[0],
                                                 coreCodec,
                                                 isSwitchingDecisionResultSpeech,
                                                 fixfixGridGranularity);

      h_envChan[0]->hSbrEnvFrame->sibBorderPrevFrame = 1;
      h_envChan[0]->encEnvData.hSbrBSGrid = h_envChan[0]->hSbrEnvFrame->hSbrGrid;
    }
  } else {
    frame_info[0] = FrameInfoGenerator(h_envChan[0]->hSbrEnvFrame,
                                       transient_info[0],

                                       sbrBitstreamData->rightBorderFIX,
                                       coreCodec,
                                       isSwitchingDecisionResultSpeech,
                                       fixfixGridGranularity);

    h_envChan[0]->encEnvData.hSbrBSGrid = h_envChan[0]->hSbrEnvFrame->hSbrGrid;
  }

  switch (stereoMode) {
    case SBR_LEFT_RIGHT:
    case SBR_SWITCH_LRC:
      if (h_con->sibilantTuning && ((h_envChan[1]->hSbrEnvFrame->sibBordersCnt > 0) && isSwitchingDecisionResultSpeech)) {
        if ((h_envChan[1]->hSbrEnvFrame->sibBordersCnt > 0) && isSwitchingDecisionResultSpeech) {
          frame_info[1] = FrameInfoGeneratorSibilant(h_envChan[1]->hSbrEnvFrame,
                                                     transient_info[1],
                                                     coreCodec,
                                                     isSwitchingDecisionResultSpeech,
                                                     fixfixGridGranularity);
        }

      } else {
        frame_info[1] = FrameInfoGenerator(h_envChan[1]->hSbrEnvFrame,
                                           transient_info[1],
                                           sbrBitstreamData->rightBorderFIX,
                                           coreCodec,
                                           isSwitchingDecisionResultSpeech,
                                           fixfixGridGranularity);
      }
      h_envChan[1]->encEnvData.hSbrBSGrid = h_envChan[1]->hSbrEnvFrame->hSbrGrid;

      if (frame_info[0]->nEnvelopes != frame_info[1]->nEnvelopes) {
        stereoMode = SBR_LEFT_RIGHT;
      } else {
        for (i = 0; i < frame_info[0]->nEnvelopes + 1; i++) {
          if (frame_info[0]->borders[i] != frame_info[1]->borders[i]) {
            stereoMode = SBR_LEFT_RIGHT;
            break;
          }
        }
        for (i = 0; i < frame_info[0]->nEnvelopes; i++) {
          if (frame_info[0]->freqRes[i] != frame_info[1]->freqRes[i]) {
            stereoMode = SBR_LEFT_RIGHT;
            break;
          }
        }
        if (frame_info[0]->shortEnv != frame_info[1]->shortEnv) {
          stereoMode = SBR_LEFT_RIGHT;
        }
      }
      if (h_envChan[0]->encEnvData.currentAmpResFF != h_envChan[1]->encEnvData.currentAmpResFF)

        stereoMode = SBR_LEFT_RIGHT;

      break;

    case SBR_COUPLING:
      frame_info[1] = frame_info[0];
      memcpy(h_envChan[1]->hSbrEnvFrame->hSbrGrid, h_envChan[0]->hSbrEnvFrame->hSbrGrid, sizeof(SBR_GRID));
      h_envChan[1]->encEnvData.hSbrBSGrid = h_envChan[1]->hSbrEnvFrame->hSbrGrid;
      h_envChan[1]->hSbrEnvFrame->hSbrFrameInfo->shortEnv = h_envChan[0]->hSbrEnvFrame->hSbrFrameInfo->shortEnv;
      h_envChan[1]->encEnvData.currentAmpResFF = h_envChan[0]->encEnvData.currentAmpResFF;

      break;
    default:

      break;
  }

  for (ch = 0; ch < nChannelsSbr; ch++) {
    h_envChan[ch]->h_sbrExtractEnvelope->pre_transient_info[0] = transient_info[ch][0];
    h_envChan[ch]->h_sbrExtractEnvelope->pre_transient_info[1] = transient_info[ch][1];
    h_envChan[ch]->encEnvData.noOfEnvelopes = nEnvelopes[ch] = frame_info[ch]->nEnvelopes;

    switch (coreCodec) {
      case CODEC_SAAC:

        if ((h_envChan[ch]->encEnvData.hSbrBSGrid->frameClass == FIXFIX) &&
            (nEnvelopes[ch] == 1)) {
          switch (coreCodec) {
            case CODEC_SAAC:
              h_envChan[ch]->encEnvData.currentAmpResFF = SBR_AMP_RES_1_5;
              break;
            default:
              break;
          }
          if (h_envChan[ch]->encEnvData.init_sbr_amp_res != h_envChan[ch]->encEnvData.currentAmpResFF) {
            InitSbrHuffmanTables(&h_envChan[ch]->encEnvData,
                                 h_envChan[ch]->h_sbrCodeEnvelope,
                                 h_envChan[ch]->h_sbrCodeNoiseFloor,
                                 h_envChan[ch]->encEnvData.currentAmpResFF,
                                 coreCodec);
          }
        } else {
          if (sbrHeaderData->sbr_amp_res != h_envChan[ch]->encEnvData.init_sbr_amp_res) {
            InitSbrHuffmanTables(&h_envChan[ch]->encEnvData,
                                 h_envChan[ch]->h_sbrCodeEnvelope,
                                 h_envChan[ch]->h_sbrCodeNoiseFloor,
                                 sbrHeaderData->sbr_amp_res,
                                 coreCodec);
          }
        }
        break;
      default:
        assert(0);
    }

    if (coreCodec == CODEC_SAAC) {
      if (h_con->sibilantTuning == 1) {
        AdvanceSbrSibilantDetector(h_envChan[ch]->h_sbrSibilantDetector,
                                   (const float *const *const)(h_envChan[ch]->h_sbrExtractEnvelope->YBuffer + h_envChan[ch]->h_sbrExtractEnvelope->YBufferReadOffset),
                                   h_con->freqBandTable[1][h_con->nSfb[1]],
                                   h_envChan[ch]->h_sbrExtractEnvelope->no_cols);
      }
    }

    if (isSwitchingDecisionResultSpeech) {
      float const minSbrNoiseFloorFactor = 0.2f;
      float sibilance = 0.0f;

      if (coreCodec == CODEC_SAAC) {
        if (h_con->sibilantTuning == 1) {
          sibilance = sibilantDetected(h_envChan[ch]->h_sbrSibilantDetector);
        }
      } else {
        sibilance = 1.f;
      }

      AdjustNoiseFloor(h_envChan[ch]->hTonCorr->h_sbrNoiseFloorEstimate,
                       minSbrNoiseFloorFactor + (1 - minSbrNoiseFloorFactor) * sibilance);

    } else {
      AdjustNoiseFloor(h_envChan[ch]->hTonCorr->h_sbrNoiseFloorEstimate, 1.0f);
    }

    CalcNoiseLevelLoweringFactors(h_envChan[ch]->hSbrEnvFrame->hSbrFrameInfo,
                                  (float const *const *const)h_envChan[ch]->h_sbrExtractEnvelope->YBuffer + h_envChan[ch]->h_sbrExtractEnvelope->YBufferReadOffset,
                                  noiseLevelLoweringFactor[ch],
                                  h_con->freqBandTable[1],
                                  h_con->nSfb[1],
                                  h_envChan[ch]->h_sbrExtractEnvelope->time_step,
                                  h_envChan[ch]->h_sbrExtractEnvelope->no_cols);

    TonCorrParamExtr(h_envChan[ch]->hTonCorr,
                     h_envChan[ch]->encEnvData.sbr_invf_mode_vec,
                     noiseFloor[ch],
                     &h_envChan[ch]->encEnvData.addHarmonicFlag,
                     h_envChan[ch]->encEnvData.addHarmonic,
                     h_envChan[ch]->h_sbrExtractEnvelope->envelopeCompensation,
                     frame_info[ch],
                     transient_info[ch],
                     h_con->freqBandTable[FREQ_RES_HIGH],
                     h_con->nSfb[FREQ_RES_HIGH],
                     coreCodec,
                     h_envChan[ch]->encEnvData.sbr_xpos_mode,

                     h_envChan[ch]->encEnvData.sbrPatchingMode,
                     (h_envChan[ch]->encEnvData.pitchInBins > 0),
                     h_con->bSbr41,
                     noiseLevelLoweringFactor[ch]);

    h_envChan[ch]->encEnvData.noOfnoisebands = h_envChan[ch]->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands;

    h_envChan[ch]->encEnvData.sbr_invf_mode =
        h_envChan[ch]->encEnvData.sbr_invf_mode_vec[0];
  }

  if (h_con->coreCodec == CODEC_SAAC && h_con->bitRate <= 12000 && h_con->bSbr41 == 1) {
    lowbitrateEnergyAdjustment(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                                   h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                               noiseLevelLoweringFactor[0],
                               frame_info[0]->nNoiseEnvelopes,
                               h_envChan[0]->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands,
                               64,
                               64);
  }

  for (ch = 0; ch < nChannelsSbr; ch++) {
    for (i = 0; i < nEnvelopes[ch]; i++) {
      h_envChan[ch]->encEnvData.noScfBands[i] =
          (frame_info[ch]->freqRes[i] == FREQ_RES_HIGH ? h_con->nSfb[FREQ_RES_HIGH] : h_con->nSfb[FREQ_RES_LOW]);
    }
  }

  switch (stereoMode) {
    case SBR_MONO:
      calculateSbrEnvelope(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                           NULL, frame_info[0], sfb_nrg[0],
                           NULL, h_con, h_envChan[0], SBR_MONO, NULL,
                           coreCodec);

      break;

    case SBR_LEFT_RIGHT:
      calculateSbrEnvelope(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                           NULL, frame_info[0], sfb_nrg[0],
                           NULL, h_con, h_envChan[0], SBR_MONO, NULL,
                           coreCodec);
      calculateSbrEnvelope(h_envChan[1]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[1]->h_sbrExtractEnvelope->YBufferReadOffset,
                           NULL, frame_info[1], sfb_nrg[1],
                           NULL, h_con, h_envChan[1], SBR_MONO, NULL,
                           coreCodec);
      break;

    case SBR_COUPLING:
      calculateSbrEnvelope(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                           h_envChan[1]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[1]->h_sbrExtractEnvelope->YBufferReadOffset,
                           frame_info[0],
                           sfb_nrg[0], sfb_nrg[1], h_con, h_envChan[0], SBR_COUPLING, &maxQuantError,
                           coreCodec);

      break;

    case SBR_SWITCH_LRC:
      calculateSbrEnvelope(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                           NULL, frame_info[0], sfb_nrg[0],
                           NULL, h_con, h_envChan[0], SBR_MONO, NULL,
                           coreCodec);
      calculateSbrEnvelope(h_envChan[1]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[1]->h_sbrExtractEnvelope->YBufferReadOffset,
                           NULL, frame_info[1], sfb_nrg[1],
                           NULL, h_con, h_envChan[1], SBR_MONO, NULL,
                           coreCodec);
      calculateSbrEnvelope(h_envChan[0]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[0]->h_sbrExtractEnvelope->YBufferReadOffset,
                           h_envChan[1]->h_sbrExtractEnvelope->YBuffer +
                               h_envChan[1]->h_sbrExtractEnvelope->YBufferReadOffset,
                           frame_info[0],
                           sfb_nrg_coupling[0], sfb_nrg_coupling[1], h_con, h_envChan[0], SBR_COUPLING, &maxQuantError,
                           coreCodec);
      break;

    default:
      assert(0);
  }

  if (stereoMode == SBR_SWITCH_LRC) {
    stereoMode = stereoModeOverride(maxQuantError,
                                    sbrHeaderData->sbr_amp_res,
                                    sfb_nrg[0],
                                    sfb_nrg[1],
                                    nEnvelopes[0],
                                    h_envChan[0]->encEnvData.noScfBands);

    if (stereoMode == SBR_LEFT_RIGHT) {
      h_envChan[0]->h_sbrCodeEnvelope->upDate = 0;
      h_envChan[0]->h_sbrCodeNoiseFloor->upDate = 0;
      h_envChan[1]->h_sbrCodeEnvelope->upDate = 0;
      h_envChan[1]->h_sbrCodeNoiseFloor->upDate = 0;
    }
  }

  for (ch = 0; ch < nChannelsSbr; ch++) {
    memcpy(prevNoiseEnergies[ch], h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev, MAX_NUM_NOISE_COEFFS * sizeof(int));
    memcpy(prevEnergies[ch], h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev, MAX_FREQ_COEFFS * sizeof(int));
  }

  switch (stereoMode) {
    case SBR_MONO:
      if (h_con->coreCodec == CODEC_SAAC && h_con->bitRate <= 12000 && h_con->bSbr41 == 1) {
        multFLOAT(noiseFloor[0], noiseLevelLoweringFactor[0], noiseFloor[0], frame_info[0]->nNoiseEnvelopes * h_envChan[0]->hTonCorr->h_sbrNoiseFloorEstimate->noNoiseBands);
      }
      sbrNoiseFloorLevelsQuantisation(noise_level[0], noiseFloor[0], 0);

      codeEnvelope(noise_level[0], res,
                   h_envChan[0]->h_sbrCodeNoiseFloor,
                   h_envChan[0]->encEnvData.domain_vec_noise, 0,
                   (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      break;
    case SBR_LEFT_RIGHT:
      sbrNoiseFloorLevelsQuantisation(noise_level[0], noiseFloor[0], 0);

      codeEnvelope(noise_level[0], res,
                   h_envChan[0]->h_sbrCodeNoiseFloor,
                   h_envChan[0]->encEnvData.domain_vec_noise, 0,
                   (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      sbrNoiseFloorLevelsQuantisation(noise_level[1], noiseFloor[1], 0);

      codeEnvelope(noise_level[1], res,
                   h_envChan[1]->h_sbrCodeNoiseFloor,
                   h_envChan[1]->encEnvData.domain_vec_noise, 0,
                   (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      break;

    case SBR_COUPLING:
      coupleNoiseFloor(noiseFloor[0], noiseFloor[1], coreCodec);

      sbrNoiseFloorLevelsQuantisation(noise_level[0], noiseFloor[0], 0);

      codeEnvelope(noise_level[0], res,
                   h_envChan[0]->h_sbrCodeNoiseFloor,
                   h_envChan[0]->encEnvData.domain_vec_noise, 1,
                   (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      sbrNoiseFloorLevelsQuantisation(noise_level[1], noiseFloor[1], 1);

      codeEnvelope(noise_level[1], res,
                   h_envChan[1]->h_sbrCodeNoiseFloor,
                   h_envChan[1]->encEnvData.domain_vec_noise, 1,
                   (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 1,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      break;
    case SBR_SWITCH_LRC:
      sbrNoiseFloorLevelsQuantisation(noise_level[0], noiseFloor[0], 0);
      sbrNoiseFloorLevelsQuantisation(noise_level[1], noiseFloor[1], 0);

      coupleNoiseFloor(noiseFloor[0], noiseFloor[1], coreCodec);
      sbrNoiseFloorLevelsQuantisation(noise_level_coupling[0], noiseFloor[0], 0);
      sbrNoiseFloorLevelsQuantisation(noise_level_coupling[1], noiseFloor[1], 1);

      break;
  }

  switch (stereoMode) {
    case SBR_MONO:
      sbrHeaderData->coupling = 0;
      h_envChan[0]->encEnvData.balance = 0;

      if (isSwitchingDecisionResultSpeech && (h_con->bitRate <= LOW_BITRATE_TUNING_THRESHOLD)) {
        int l_start;
        int l_end;
        int no_of_bands;
        int no_of_envelopes;

        no_of_envelopes = frame_info[0]->nEnvelopes;

        l_end = 0;
        for (i = 0; i < no_of_envelopes; i++) {
          if (frame_info[0]->freqRes[i] == FREQ_RES_HIGH) {
            no_of_bands = h_envChan[0]->h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH];
          } else {
            no_of_bands = h_envChan[0]->h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW];
          }
          if (no_of_bands > 2) {
            l_start = l_end;
            l_end = l_start + no_of_bands;
            for (j = l_start + 1; j < l_end - 2; j++) {
              sfb_nrg[0][j] = (int)((sfb_nrg[0][j - 1] + sfb_nrg[0][j] + sfb_nrg[0][j + 1]) / 3);
            }
            j = l_end - 1;
            sfb_nrg[0][j] = (int)((sfb_nrg[0][j - 2] + sfb_nrg[0][j - 1] + sfb_nrg[0][j]) / 3);
          }
        }
      }

      codeEnvelope(sfb_nrg[0], frame_info[0]->freqRes,
                   h_envChan[0]->h_sbrCodeEnvelope,
                   h_envChan[0]->encEnvData.domain_vec,
                   sbrHeaderData->coupling,
                   frame_info[0]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);
      break;

    case SBR_LEFT_RIGHT:
      sbrHeaderData->coupling = 0;

      h_envChan[0]->encEnvData.balance = 0;
      h_envChan[1]->encEnvData.balance = 0;

      codeEnvelope(sfb_nrg[0], frame_info[0]->freqRes,
                   h_envChan[0]->h_sbrCodeEnvelope,
                   h_envChan[0]->encEnvData.domain_vec,
                   sbrHeaderData->coupling,
                   frame_info[0]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      codeEnvelope(sfb_nrg[1], frame_info[1]->freqRes,
                   h_envChan[1]->h_sbrCodeEnvelope,
                   h_envChan[1]->encEnvData.domain_vec,
                   sbrHeaderData->coupling,
                   frame_info[1]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      break;

    case SBR_COUPLING:
      sbrHeaderData->coupling = 1;
      h_envChan[0]->encEnvData.balance = 0;
      h_envChan[1]->encEnvData.balance = 1;

      codeEnvelope(sfb_nrg[0], frame_info[0]->freqRes,
                   h_envChan[0]->h_sbrCodeEnvelope,
                   h_envChan[0]->encEnvData.domain_vec,
                   sbrHeaderData->coupling,
                   frame_info[0]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);
      codeEnvelope(sfb_nrg[1], frame_info[1]->freqRes,
                   h_envChan[1]->h_sbrCodeEnvelope,
                   h_envChan[1]->encEnvData.domain_vec,
                   sbrHeaderData->coupling,
                   frame_info[1]->nEnvelopes, 1,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);
      break;

    case SBR_SWITCH_LRC: {
      int payloadbitsLR;
      int payloadbitsCOUPLING;

      int sfbNrgPrevTemp[MAX_NUM_CHANNELS][MAX_FREQ_COEFFS];
      int noisePrevTemp[MAX_NUM_CHANNELS][MAX_NUM_NOISE_COEFFS];
      int upDateNrgTemp[MAX_NUM_CHANNELS];
      int upDateNoiseTemp[MAX_NUM_CHANNELS];
      int domainVecTemp[MAX_NUM_CHANNELS][MAX_ENVELOPES];
      int domainVecNoiseTemp[MAX_NUM_CHANNELS][MAX_ENVELOPES];

      int tempFlagRight = 0;
      int tempFlagLeft = 0;

      for (ch = 0; ch < nChannelsSbr; ch++) {
        memcpy(sfbNrgPrevTemp[ch], h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev,
               MAX_FREQ_COEFFS * sizeof(int));

        memcpy(noisePrevTemp[ch], h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev,
               MAX_NUM_NOISE_COEFFS * sizeof(int));

        upDateNrgTemp[ch] = h_envChan[ch]->h_sbrCodeEnvelope->upDate;
        upDateNoiseTemp[ch] = h_envChan[ch]->h_sbrCodeNoiseFloor->upDate;

        if (sbrHeaderData->prev_coupling) {
          h_envChan[ch]->h_sbrCodeEnvelope->upDate = 0;
          h_envChan[ch]->h_sbrCodeNoiseFloor->upDate = 0;
        }
      }

      codeEnvelope(sfb_nrg[0], frame_info[0]->freqRes,
                   h_envChan[0]->h_sbrCodeEnvelope,
                   h_envChan[0]->encEnvData.domain_vec, 0,
                   frame_info[0]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);
      codeEnvelope(sfb_nrg[1], frame_info[1]->freqRes,
                   h_envChan[1]->h_sbrCodeEnvelope,
                   h_envChan[1]->encEnvData.domain_vec, 0,
                   frame_info[1]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      c = 0;
      for (i = 0; i < nEnvelopes[0]; i++) {
        for (j = 0; j < h_envChan[0]->encEnvData.noScfBands[i]; j++) {
          h_envChan[0]->encEnvData.ienvelope[i][j] = sfb_nrg[0][c];
          h_envChan[1]->encEnvData.ienvelope[i][j] = sfb_nrg[1][c];
          c++;
        }
      }

      codeEnvelope(noise_level[0], res,
                   h_envChan[0]->h_sbrCodeNoiseFloor,
                   h_envChan[0]->encEnvData.domain_vec_noise, 0,
                   (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++)
        h_envChan[0]->encEnvData.sbr_noise_levels[i] = noise_level[0][i];

      codeEnvelope(noise_level[1], res,
                   h_envChan[1]->h_sbrCodeNoiseFloor,
                   h_envChan[1]->encEnvData.domain_vec_noise, 0,
                   (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++)
        h_envChan[1]->encEnvData.sbr_noise_levels[i] = noise_level[1][i];

      sbrHeaderData->coupling = 0;
      h_envChan[0]->encEnvData.balance = 0;
      h_envChan[1]->encEnvData.balance = 0;

      payloadbitsLR = CountSbrChannelPairElement(hEnvEncoder,
                                                 hCmonData,
                                                 coreCodec, bUsacIndependenceFlag);

      for (ch = 0; ch < nChannelsSbr; ch++) {
        int itmp;
        for (i = 0; i < MAX_FREQ_COEFFS; i++) {
          itmp = h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev[i];
          h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev[i] = sfbNrgPrevTemp[ch][i];
          sfbNrgPrevTemp[ch][i] = itmp;
        }
        for (i = 0; i < MAX_NUM_NOISE_COEFFS; i++) {
          itmp = h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev[i];
          h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev[i] = noisePrevTemp[ch][i];
          noisePrevTemp[ch][i] = itmp;
        }

        itmp = h_envChan[ch]->h_sbrCodeEnvelope->upDate;
        h_envChan[ch]->h_sbrCodeEnvelope->upDate = upDateNrgTemp[ch];
        upDateNrgTemp[ch] = itmp;

        itmp = h_envChan[ch]->h_sbrCodeNoiseFloor->upDate;
        h_envChan[ch]->h_sbrCodeNoiseFloor->upDate = upDateNoiseTemp[ch];
        upDateNoiseTemp[ch] = itmp;

        memcpy(domainVecTemp[ch], h_envChan[ch]->encEnvData.domain_vec, sizeof(int) * MAX_ENVELOPES);
        memcpy(domainVecNoiseTemp[ch], h_envChan[ch]->encEnvData.domain_vec_noise, sizeof(int) * MAX_ENVELOPES);

        if (!sbrHeaderData->prev_coupling) {
          h_envChan[ch]->h_sbrCodeEnvelope->upDate = 0;
          h_envChan[ch]->h_sbrCodeNoiseFloor->upDate = 0;
        }
      }

      codeEnvelope(sfb_nrg_coupling[0], frame_info[0]->freqRes,
                   h_envChan[0]->h_sbrCodeEnvelope,
                   h_envChan[0]->encEnvData.domain_vec, 1,
                   frame_info[0]->nEnvelopes, 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      codeEnvelope(sfb_nrg_coupling[1], frame_info[1]->freqRes,
                   h_envChan[1]->h_sbrCodeEnvelope,
                   h_envChan[1]->encEnvData.domain_vec, 1,
                   frame_info[1]->nEnvelopes, 1,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      c = 0;
      for (i = 0; i < nEnvelopes[0]; i++) {
        for (j = 0; j < h_envChan[0]->encEnvData.noScfBands[i]; j++) {
          h_envChan[0]->encEnvData.ienvelope[i][j] = sfb_nrg_coupling[0][c];
          h_envChan[1]->encEnvData.ienvelope[i][j] = sfb_nrg_coupling[1][c];
          c++;
        }
      }

      codeEnvelope(noise_level_coupling[0], res,
                   h_envChan[0]->h_sbrCodeNoiseFloor,
                   h_envChan[0]->encEnvData.domain_vec_noise, 1,
                   (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++)
        h_envChan[0]->encEnvData.sbr_noise_levels[i] = noise_level_coupling[0][i];

      codeEnvelope(noise_level_coupling[1], res,
                   h_envChan[1]->h_sbrCodeNoiseFloor,
                   h_envChan[1]->encEnvData.domain_vec_noise, 1,
                   (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 1,
                   sbrBitstreamData->HeaderActive | h_con->overrideTimeDiffCoding);

      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++)
        h_envChan[1]->encEnvData.sbr_noise_levels[i] = noise_level_coupling[1][i];

      sbrHeaderData->coupling = 1;

      h_envChan[0]->encEnvData.balance = 0;
      h_envChan[1]->encEnvData.balance = 1;

      switch (coreCodec) {
        case CODEC_SAAC:
          tempFlagLeft = h_envChan[0]->encEnvData.addHarmonicFlag;
          tempFlagRight = h_envChan[1]->encEnvData.addHarmonicFlag;
          break;
        default:
          assert(0);
      }
      payloadbitsCOUPLING = CountSbrChannelPairElement(hEnvEncoder,
                                                       hCmonData,
                                                       coreCodec, bUsacIndependenceFlag);

      switch (coreCodec) {
        case CODEC_SAAC:
          h_envChan[0]->encEnvData.addHarmonicFlag = tempFlagLeft;
          h_envChan[1]->encEnvData.addHarmonicFlag = tempFlagRight;
          break;
        default:
          assert(0);
      }

      if (payloadbitsCOUPLING < payloadbitsLR) {
        for (ch = 0; ch < nChannelsSbr; ch++) {
          memcpy(sfb_nrg[ch], sfb_nrg_coupling[ch],
                 MAX_NUM_ENVELOPE_VALUES * sizeof(int));
          memcpy(noise_level[ch], noise_level_coupling[ch],
                 MAX_NUM_NOISE_VALUES * sizeof(int));
        }

        sbrHeaderData->coupling = 1;
        h_envChan[0]->encEnvData.balance = 0;
        h_envChan[1]->encEnvData.balance = 1;
      } else {
        for (ch = 0; ch < nChannelsSbr; ch++) {
          memcpy(h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev,
                 sfbNrgPrevTemp[ch], MAX_FREQ_COEFFS * sizeof(int));

          h_envChan[ch]->h_sbrCodeEnvelope->upDate = upDateNrgTemp[ch];

          memcpy(h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev,
                 noisePrevTemp[ch], MAX_NUM_NOISE_COEFFS * sizeof(int));

          memcpy(h_envChan[ch]->encEnvData.domain_vec, domainVecTemp[ch], sizeof(int) * MAX_ENVELOPES);
          memcpy(h_envChan[ch]->encEnvData.domain_vec_noise, domainVecNoiseTemp[ch], sizeof(int) * MAX_ENVELOPES);

          h_envChan[ch]->h_sbrCodeNoiseFloor->upDate = upDateNoiseTemp[ch];
        }

        sbrHeaderData->coupling = 0;
        h_envChan[0]->encEnvData.balance = 0;
        h_envChan[1]->encEnvData.balance = 0;
      }
    } break;

    default:
      assert(0);
  }

  if (stereoMode == SBR_MONO) {
    if (h_envChan[0]->encEnvData.domain_vec[0] == TIME) {
      h_envChan[0]->h_sbrCodeEnvelope->dF_edge_incr_fac++;
    } else {
      h_envChan[0]->h_sbrCodeEnvelope->dF_edge_incr_fac = 0;
    }
  } else {
    if (h_envChan[0]->encEnvData.domain_vec[0] == TIME ||
        h_envChan[1]->encEnvData.domain_vec[0] == TIME) {
      h_envChan[0]->h_sbrCodeEnvelope->dF_edge_incr_fac++;
      h_envChan[1]->h_sbrCodeEnvelope->dF_edge_incr_fac++;
    } else {
      h_envChan[0]->h_sbrCodeEnvelope->dF_edge_incr_fac = 0;
      h_envChan[1]->h_sbrCodeEnvelope->dF_edge_incr_fac = 0;
    }
  }

  for (ch = 0; ch < nChannelsSbr; ch++) {
    c = 0;
    for (i = 0; i < nEnvelopes[ch]; i++) {
      for (j = 0; j < h_envChan[ch]->encEnvData.noScfBands[i]; j++) {
        h_envChan[ch]->encEnvData.ienvelope[i][j] = sfb_nrg[ch][c];
        c++;
      }
    }
    for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {
      h_envChan[ch]->encEnvData.sbr_noise_levels[i] = noise_level[ch][i];
    }
  }

  if (coreCodec == CODEC_SAAC) {
    for (ch = 0; ch < nChannelsSbr; ch++) {
      for (i = 0; i < h_envChan[ch]->encEnvData.noOfEnvelopes; i++) {
        h_envChan[ch]->encEnvData.bs_temp_shape[i] = 0;
      }
    }
  }

  if (stereoMode == SBR_MONO) {
    totalBits = CountSbrSingleChannelElement(hEnvEncoder, hCmonData, coreCodec, bUsacIndependenceFlag);
  } else {
    totalBits = CountSbrChannelPairElement(hEnvEncoder, hCmonData, coreCodec, bUsacIndependenceFlag);
  }
  if (totalBits > SBR_EMERGENCY_MODE_THRESHOLD && emergencyMode == 0 && coreCodec == CODEC_SAAC && fixfixGridGranularity == 8 && stereoMode == SBR_MONO) {
    for (ch = 0; ch < nChannelsSbr; ch++) {
      memcpy(h_envChan[ch]->h_sbrCodeNoiseFloor->sfb_nrg_prev, prevNoiseEnergies[ch], MAX_NUM_NOISE_COEFFS * sizeof(int));
      memcpy(h_envChan[ch]->h_sbrCodeEnvelope->sfb_nrg_prev, prevEnergies[ch], MAX_FREQ_COEFFS * sizeof(int));
    }
    return ExtractSbrEnvelope(hEnvEncoder, stereoMode, hCmonData,
                              isSwitchingDecisionResultSpeech,
                              bUsacIndependenceFlag,
                              SBR_EMERGENCY_MODE_ON);
  }

  if (nChannelsSbr == 2) {
    WriteSbrChannelPairElement(hEnvEncoder,
                               hCmonData,
                               coreCodec, bUsacIndependenceFlag);
  } else {
    WriteSbrSingleChannelElement(hEnvEncoder,
                                 hCmonData,
                                 coreCodec, bUsacIndependenceFlag);
  }

  for (ch = 0; ch < nChannelsSbr; ch++) {
    for (i = 0; i < h_envChan[ch]->h_sbrExtractEnvelope->YBufferWriteOffset; i++) {
      float *tmp;

      tmp = h_envChan[ch]->h_sbrExtractEnvelope->YBuffer[i];
      h_envChan[ch]->h_sbrExtractEnvelope->YBuffer[i] = h_envChan[ch]->h_sbrExtractEnvelope->YBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols];
      h_envChan[ch]->h_sbrExtractEnvelope->YBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols] = tmp;
    }
    h_envChan[ch]->encEnvData.sbrPatchingMode = 1;
  }

  for (ch = 0; ch < hEnvEncoder->nChannelsQmf; ch++) {
    int writeOffset;

    {
      writeOffset = h_envChan[ch]->h_sbrExtractEnvelope->rBufferWriteOffset;
    }
    for (i = 0; i < writeOffset; i++) {
      float *tmp;

      tmp = h_envChan[ch]->h_sbrExtractEnvelope->rBuffer[i];
      h_envChan[ch]->h_sbrExtractEnvelope->rBuffer[i] = h_envChan[ch]->h_sbrExtractEnvelope->rBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols];
      h_envChan[ch]->h_sbrExtractEnvelope->rBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols] = tmp;

      tmp = h_envChan[ch]->h_sbrExtractEnvelope->iBuffer[i];
      h_envChan[ch]->h_sbrExtractEnvelope->iBuffer[i] = h_envChan[ch]->h_sbrExtractEnvelope->iBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols];
      h_envChan[ch]->h_sbrExtractEnvelope->iBuffer[i + h_envChan[ch]->h_sbrExtractEnvelope->no_cols] = tmp;
    }
  }
  sbrHeaderData->prev_coupling = sbrHeaderData->coupling;

  return noError;
}

HANDLE_ERROR_INFO
CreateExtractSbrEnvelope(HANDLE_SBR_EXTRACT_ENVELOPE *hSbr,
                         int no_cols,
                         int no_rows,
                         int start_index,
                         int time_slots,
                         int time_step,
                         HANDLE_SBR_CONFIG_DATA hSbrConfigData) {
  int i;
  int YBufferLength, rBufferLength;
  HANDLE_SBR_EXTRACT_ENVELOPE hs;
  CODEC_TYPE coreCodec = hSbrConfigData->coreCodec;

  hs = (HANDLE_SBR_EXTRACT_ENVELOPE)iisCalloc(1, sizeof(SBR_EXTRACT_ENVELOPE));
  if (hs == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  switch (coreCodec) {
    case CODEC_SAAC:
      hs->time_slot_delay = 3 * time_step;
      hs->YBufferWriteOffset = no_cols;
      hs->YBufferReadOffset = 0;
      hs->rBufferWriteOffset = no_cols / 2;
      hs->rBufferReadOffset = 0;
      break;
    default:
      return iisUtil_ERROR(CDI, "Core codec type not supported.");
  }

  YBufferLength = hs->YBufferWriteOffset + no_cols;

  rBufferLength = hs->rBufferWriteOffset + no_cols;

  hSbrConfigData->sbrDelayEnc = no_rows *
                                (hs->YBufferWriteOffset - hs->YBufferReadOffset +
                                 hs->rBufferWriteOffset - hs->rBufferReadOffset);

  hSbrConfigData->sbrDelayDec = hs->time_slot_delay * no_rows;

  hs->pre_transient_info[0] = 0;
  hs->pre_transient_info[1] = 0;

  hs->no_cols = no_cols;
  hs->no_rows = no_rows;
  hs->start_index = start_index;

  hs->time_slots = time_slots;
  hs->time_step = time_step;

  hs->YBuffer = (float **)iisCalloc(YBufferLength, sizeof(float *));
  if (hs->YBuffer == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < YBufferLength; i++) {
    hs->YBuffer[i] = (float *)iisCalloc(no_rows, sizeof(float));
    if (hs->YBuffer[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->rBuffer = (float **)iisCalloc(rBufferLength, sizeof(float *));
  hs->iBuffer = (float **)iisCalloc(rBufferLength, sizeof(float *));
  if (hs->rBuffer == NULL || hs->iBuffer == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < rBufferLength; i++) {
    hs->rBuffer[i] = (float *)iisCalloc(no_rows, sizeof(float));
    hs->iBuffer[i] = (float *)iisCalloc(no_rows, sizeof(float));
    if (hs->rBuffer[i] == NULL || hs->iBuffer[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->YBufferPatch = (float **)iisCalloc(YBufferLength, sizeof(float *));
  if (hs->YBufferPatch == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < YBufferLength; i++) {
    hs->YBufferPatch[i] = (float *)iisCalloc(no_rows, sizeof(float));
    if (hs->YBufferPatch[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->rBufferPatch = (float **)iisCalloc(rBufferLength, sizeof(float *));
  hs->iBufferPatch = (float **)iisCalloc(rBufferLength, sizeof(float *));
  if (hs->rBufferPatch == NULL || hs->iBufferPatch == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  for (i = 0; i < rBufferLength; i++) {
    hs->rBufferPatch[i] = (float *)iisCalloc(no_rows, sizeof(float));
    hs->iBufferPatch[i] = (float *)iisCalloc(no_rows, sizeof(float));
    if (hs->rBufferPatch[i] == NULL || hs->iBufferPatch[i] == NULL)
      return iisUtil_ERROR(CDI, "out of memory");
  }

  hs->envelopeCompensation = (int *)iisCalloc(MAX_FREQ_COEFFS, sizeof(int));
  if (hs->envelopeCompensation == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  *hSbr = hs;
  return noError;
}

void deleteExtractSbrEnvelope(HANDLE_SBR_EXTRACT_ENVELOPE hSbrCut) {
  int i;

  if (hSbrCut) {
    for (i = 0; i < hSbrCut->YBufferWriteOffset + hSbrCut->no_cols; i++) {
      iisFree(hSbrCut->YBuffer[i]);
    }
    for (i = 0; i < hSbrCut->rBufferWriteOffset + hSbrCut->no_cols; i++) {
      iisFree(hSbrCut->rBuffer[i]);
      iisFree(hSbrCut->iBuffer[i]);
    }
    iisFree(hSbrCut->YBuffer);
    iisFree(hSbrCut->rBuffer);
    iisFree(hSbrCut->iBuffer);

    for (i = 0; i < hSbrCut->YBufferWriteOffset + hSbrCut->no_cols; i++) {
      iisFree(hSbrCut->YBufferPatch[i]);
    }
    for (i = 0; i < hSbrCut->rBufferWriteOffset + hSbrCut->no_cols; i++) {
      iisFree(hSbrCut->rBufferPatch[i]);
      iisFree(hSbrCut->iBufferPatch[i]);
    }
    iisFree(hSbrCut->YBufferPatch);
    iisFree(hSbrCut->rBufferPatch);
    iisFree(hSbrCut->iBufferPatch);

    if (hSbrCut->envelopeCompensation)
      iisFree(hSbrCut->envelopeCompensation);

    iisFree(hSbrCut);
  }
}

float **GetQmfRealBufferRead(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->rBuffer + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferReadOffset;
}

float **GetQmfImagBufferRead(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->iBuffer + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferReadOffset;
}

float **GetQmfRealBufferWrite(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->rBuffer + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferWriteOffset;
}

float **GetQmfImagBufferWrite(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->iBuffer + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferWriteOffset;
}

HANDLE_QMFLIB_ANALYSIS GetSbrQmfHandle(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrQmf;
}

HANDLE_QMFLIB_SYNTHESIS GetSbrSynthQmfHandle(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrSynthQmf;
}

float **GetQmfRealBufferReadPV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferPatch + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferReadOffset;
}

float **GetQmfImagBufferReadPV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->iBufferPatch + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferReadOffset;
}

float **GetQmfRealBufferWritePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferPatch + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferWriteOffset;
}

float **GetQmfImagBufferWritePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrExtractEnvelope->iBufferPatch + hEnvChannel[ch]->h_sbrExtractEnvelope->rBufferWriteOffset;
}

HANDLE_QMFLIB_ANALYSIS GetSbrQmfHandlePV(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  return hEnvChannel[ch]->h_sbrPatchQmf;
}

int GetEnvEstDelay(HANDLE_SBR_EXTRACT_ENVELOPE hSbr) {
  return hSbr->no_rows * (hSbr->YBufferWriteOffset - hSbr->YBufferReadOffset +
                          hSbr->rBufferWriteOffset - hSbr->rBufferReadOffset);
}

int GetEnvDownsamplerDelay(HANDLE_ENV_CHANNEL *hEnvChannel, int ch) {
  const int downsampleFactor = hEnvChannel[ch]->no_channels / hEnvChannel[ch]->no_channels_syn;

  int delay = SBR_QMF_SUBSAMPLE_DELAY * hEnvChannel[ch]->no_channels + downsampleFactor * SBR_QMF_SUBSAMPLE_DELAY * hEnvChannel[ch]->no_channels_syn - hEnvChannel[ch]->no_channels;

  return delay / downsampleFactor;
}
