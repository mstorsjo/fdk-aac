
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
#include "code_env.h"
#include "sbr.h"
#include "huff_tab.h"

HANDLE_ERROR_INFO
InitSbrHuffmanTables(HANDLE_SBR_ENV_DATA sbrEnvData,
                     HANDLE_SBR_CODE_ENVELOPE henv,
                     HANDLE_SBR_CODE_ENVELOPE hnoise,
                     AMP_RES amp_res,
                     CODEC_TYPE coreCodec) {
  if ((!henv) || (!hnoise) || (!sbrEnvData))
    return iisUtil_ERROR(CDI, "handles not initialized");

  sbrEnvData->init_sbr_amp_res = amp_res;

  switch (amp_res) {
    case SBR_AMP_RES_3_0:

      sbrEnvData->hufftableLevelTimeC = v_Huff_envelopeLevelC11T;
      sbrEnvData->hufftableLevelTimeL = v_Huff_envelopeLevelL11T;
      sbrEnvData->hufftableBalanceTimeC = bookSbrEnvBalanceC11T;
      sbrEnvData->hufftableBalanceTimeL = bookSbrEnvBalanceL11T;

      sbrEnvData->hufftableLevelFreqC = v_Huff_envelopeLevelC11F;
      sbrEnvData->hufftableLevelFreqL = v_Huff_envelopeLevelL11F;
      sbrEnvData->hufftableBalanceFreqC = bookSbrEnvBalanceC11F;
      sbrEnvData->hufftableBalanceFreqL = bookSbrEnvBalanceL11F;

      sbrEnvData->hufftableTimeC = v_Huff_envelopeLevelC11T;
      sbrEnvData->hufftableTimeL = v_Huff_envelopeLevelL11T;
      sbrEnvData->hufftableFreqC = v_Huff_envelopeLevelC11F;
      sbrEnvData->hufftableFreqL = v_Huff_envelopeLevelL11F;

      sbrEnvData->codeBookScfLavBalance = CODE_BOOK_SCF_LAV_BALANCE11;
      sbrEnvData->codeBookScfLav = CODE_BOOK_SCF_LAV11;

      sbrEnvData->si_sbr_start_env_bits = SI_SBR_START_ENV_BITS_AMP_RES_3_0;
      sbrEnvData->si_sbr_start_env_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0;
      break;

    case SBR_AMP_RES_1_5:

      sbrEnvData->hufftableLevelTimeC = v_Huff_envelopeLevelC10T;
      sbrEnvData->hufftableLevelTimeL = v_Huff_envelopeLevelL10T;
      sbrEnvData->hufftableBalanceTimeC = bookSbrEnvBalanceC10T;
      sbrEnvData->hufftableBalanceTimeL = bookSbrEnvBalanceL10T;

      sbrEnvData->hufftableLevelFreqC = v_Huff_envelopeLevelC10F;
      sbrEnvData->hufftableLevelFreqL = v_Huff_envelopeLevelL10F;
      sbrEnvData->hufftableBalanceFreqC = bookSbrEnvBalanceC10F;
      sbrEnvData->hufftableBalanceFreqL = bookSbrEnvBalanceL10F;

      sbrEnvData->hufftableTimeC = v_Huff_envelopeLevelC10T;
      sbrEnvData->hufftableTimeL = v_Huff_envelopeLevelL10T;
      sbrEnvData->hufftableFreqC = v_Huff_envelopeLevelC10F;
      sbrEnvData->hufftableFreqL = v_Huff_envelopeLevelL10F;

      sbrEnvData->codeBookScfLavBalance = CODE_BOOK_SCF_LAV_BALANCE10;
      sbrEnvData->codeBookScfLav = CODE_BOOK_SCF_LAV10;

      sbrEnvData->si_sbr_start_env_bits = SI_SBR_START_ENV_BITS_AMP_RES_1_5;
      sbrEnvData->si_sbr_start_env_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5;
      break;

    default:
      return iisUtil_ERROR(CDI, "undefined amp_res mode");
      break;
  }

  sbrEnvData->hufftableNoiseLevelTimeC = v_Huff_NoiseLevelC11T;
  sbrEnvData->hufftableNoiseLevelTimeL = v_Huff_NoiseLevelL11T;
  sbrEnvData->hufftableNoiseBalanceTimeC = bookSbrNoiseBalanceC11T;
  sbrEnvData->hufftableNoiseBalanceTimeL = bookSbrNoiseBalanceL11T;

  sbrEnvData->hufftableNoiseLevelFreqC = v_Huff_envelopeLevelC11F;
  sbrEnvData->hufftableNoiseLevelFreqL = v_Huff_envelopeLevelL11F;
  sbrEnvData->hufftableNoiseBalanceFreqC = bookSbrEnvBalanceC11F;
  sbrEnvData->hufftableNoiseBalanceFreqL = bookSbrEnvBalanceL11F;

  sbrEnvData->hufftableNoiseTimeC = v_Huff_NoiseLevelC11T;
  sbrEnvData->hufftableNoiseTimeL = v_Huff_NoiseLevelL11T;
  sbrEnvData->hufftableNoiseFreqC = v_Huff_envelopeLevelC11F;
  sbrEnvData->hufftableNoiseFreqL = v_Huff_envelopeLevelL11F;

  sbrEnvData->si_sbr_start_noise_bits = SI_SBR_START_NOISE_BITS_AMP_RES_3_0;
  sbrEnvData->si_sbr_start_noise_bits_balance = SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0;

  henv->codeBookScfLavBalanceTime = sbrEnvData->codeBookScfLavBalance;
  henv->codeBookScfLavBalanceFreq = sbrEnvData->codeBookScfLavBalance;
  henv->codeBookScfLavLevelTime = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavLevelFreq = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavTime = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavFreq = sbrEnvData->codeBookScfLav;

  henv->hufftableLevelTimeL = sbrEnvData->hufftableLevelTimeL;
  henv->hufftableBalanceTimeL = sbrEnvData->hufftableBalanceTimeL;
  henv->hufftableTimeL = sbrEnvData->hufftableTimeL;
  henv->hufftableLevelFreqL = sbrEnvData->hufftableLevelFreqL;
  henv->hufftableBalanceFreqL = sbrEnvData->hufftableBalanceFreqL;
  henv->hufftableFreqL = sbrEnvData->hufftableFreqL;

  henv->codeBookScfLavFreq = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavTime = sbrEnvData->codeBookScfLav;

  henv->start_bits = sbrEnvData->si_sbr_start_env_bits;
  henv->start_bits_balance = sbrEnvData->si_sbr_start_env_bits_balance;

  hnoise->codeBookScfLavBalanceTime = CODE_BOOK_SCF_LAV_BALANCE11;
  hnoise->codeBookScfLavBalanceFreq = CODE_BOOK_SCF_LAV_BALANCE11;
  hnoise->codeBookScfLavLevelTime = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavLevelFreq = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavTime = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavFreq = CODE_BOOK_SCF_LAV11;

  hnoise->hufftableLevelTimeL = sbrEnvData->hufftableNoiseLevelTimeL;
  hnoise->hufftableBalanceTimeL = sbrEnvData->hufftableNoiseBalanceTimeL;
  hnoise->hufftableTimeL = sbrEnvData->hufftableNoiseTimeL;
  hnoise->hufftableLevelFreqL = sbrEnvData->hufftableNoiseLevelFreqL;
  hnoise->hufftableBalanceFreqL = sbrEnvData->hufftableNoiseBalanceFreqL;
  hnoise->hufftableFreqL = sbrEnvData->hufftableNoiseFreqL;

  hnoise->start_bits = sbrEnvData->si_sbr_start_noise_bits;
  hnoise->start_bits_balance = sbrEnvData->si_sbr_start_noise_bits_balance;

  switch (coreCodec) {
    case CODEC_SAAC:

      henv->upDate = 0;
      hnoise->upDate = 0;
      break;
    default:
      assert(0);
  }

  return noError;
}

static int indexLow2High(int offset, int index, FREQ_RES res) {
  if (res == FREQ_RES_LOW) {
    if (offset >= 0) {
      if (index < offset)
        return (index);
      else
        return (2 * index - offset);
    } else {
      offset = -offset;
      if (index < offset)
        return (2 * index + index);
      else
        return (2 * index + offset);
    }
  } else
    return (index);
}

static void mapLowResEnergyVal(int currVal, int *prevData, int offset, int index, FREQ_RES res) {
  if (res == FREQ_RES_LOW) {
    if (offset >= 0) {
      if (index < offset)
        prevData[index] = currVal;
      else {
        prevData[2 * index - offset] = currVal;
        prevData[2 * index + 1 - offset] = currVal;
      }
    } else {
      offset = -offset;
      if (index < offset) {
        prevData[3 * index] = currVal;
        prevData[3 * index + 1] = currVal;
        prevData[3 * index + 2] = currVal;
      } else {
        prevData[2 * index + offset] = currVal;
        prevData[2 * index + 1 + offset] = currVal;
      }
    }
  } else
    prevData[index] = currVal;
}

static int
computeBits(int *delta,
            int codeBookScfLavLevel,
            int codeBookScfLavBalance,
            const int *hufftableLevel,
            const int *hufftableBalance, int scalable, int channel) {
  int index;
  int delta_bits = 0;

  if (scalable) {
    if (channel == 1) {
      index =
          (*delta < 0) ? max(*delta, -codeBookScfLavBalance) : min(*delta, codeBookScfLavBalance);
      if (index != *delta) {
        *delta = index;
        return (10000);
      }

      delta_bits = hufftableBalance[index + codeBookScfLavBalance];
    } else {
      index =
          (*delta < 0) ? max(*delta, -codeBookScfLavLevel) : min(*delta, codeBookScfLavLevel);
      if (index != *delta) {
        *delta = index;
        return (10000);
      }

      delta_bits = hufftableLevel[index + codeBookScfLavLevel];
    }
  } else {
    index =
        (*delta < 0) ? max(*delta, -codeBookScfLavLevel) : min(*delta, codeBookScfLavLevel);
    if (index != *delta) {
      *delta = index;
      return (10000);
    }
    delta_bits = hufftableLevel[index + codeBookScfLavLevel];
  }

  return (delta_bits);
}

void codeEnvelope(int *sfb_nrg,
                  const FREQ_RES *freq_res,
                  SBR_CODE_ENVELOPE *h_sbrCodeEnvelope,
                  int *directionVec,
                  int coupling,
                  int nEnvelopes,
                  int channel,
                  int headerActive) {
  int i, no_of_bands, band, last_nrg, curr_nrg;
  int *ptr_nrg;

  int codeBookScfLavLevelTime;
  int codeBookScfLavLevelFreq;
  int codeBookScfLavBalanceTime;
  int codeBookScfLavBalanceFreq;
  const int *hufftableLevelTimeL;
  const int *hufftableBalanceTimeL;
  const int *hufftableLevelFreqL;
  const int *hufftableBalanceFreqL;

  int offset = h_sbrCodeEnvelope->offset;
  int envDataTableCompFactor;

  int delta_F_bits = 0, delta_T_bits = 0;
  int use_dT;

  int delta_F[MAX_FREQ_COEFFS];
  int delta_T[MAX_FREQ_COEFFS];

  float dF_edge_1stEnv;

  dF_edge_1stEnv = h_sbrCodeEnvelope->dF_edge_1stEnv +
                   h_sbrCodeEnvelope->dF_edge_incr * h_sbrCodeEnvelope->dF_edge_incr_fac;

  if (coupling) {
    codeBookScfLavLevelTime = h_sbrCodeEnvelope->codeBookScfLavLevelTime;
    codeBookScfLavLevelFreq = h_sbrCodeEnvelope->codeBookScfLavLevelFreq;
    codeBookScfLavBalanceTime = h_sbrCodeEnvelope->codeBookScfLavBalanceTime;
    codeBookScfLavBalanceFreq = h_sbrCodeEnvelope->codeBookScfLavBalanceFreq;
    hufftableLevelTimeL = h_sbrCodeEnvelope->hufftableLevelTimeL;
    hufftableBalanceTimeL = h_sbrCodeEnvelope->hufftableBalanceTimeL;
    hufftableLevelFreqL = h_sbrCodeEnvelope->hufftableLevelFreqL;
    hufftableBalanceFreqL = h_sbrCodeEnvelope->hufftableBalanceFreqL;
  } else {
    codeBookScfLavLevelTime = h_sbrCodeEnvelope->codeBookScfLavTime;
    codeBookScfLavLevelFreq = h_sbrCodeEnvelope->codeBookScfLavFreq;
    codeBookScfLavBalanceTime = h_sbrCodeEnvelope->codeBookScfLavTime;
    codeBookScfLavBalanceFreq = h_sbrCodeEnvelope->codeBookScfLavFreq;
    hufftableLevelTimeL = h_sbrCodeEnvelope->hufftableTimeL;
    hufftableBalanceTimeL = h_sbrCodeEnvelope->hufftableTimeL;
    hufftableLevelFreqL = h_sbrCodeEnvelope->hufftableFreqL;
    hufftableBalanceFreqL = h_sbrCodeEnvelope->hufftableFreqL;
  }

  if (coupling == 1 && channel == 1)
    envDataTableCompFactor = 1;
  else
    envDataTableCompFactor = 0;

  if (h_sbrCodeEnvelope->deltaTAcrossFrames == 0)
    h_sbrCodeEnvelope->upDate = 0;

  if (headerActive)
    h_sbrCodeEnvelope->upDate = 0;

  for (i = 0; i < nEnvelopes; i++) {
    if (freq_res[i] == FREQ_RES_HIGH)
      no_of_bands = h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH];
    else
      no_of_bands = h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW];

    assert(no_of_bands == h_sbrCodeEnvelope->nSfb[freq_res[i]]);

    ptr_nrg = sfb_nrg;
    curr_nrg = *ptr_nrg;

    delta_F[0] = curr_nrg >> envDataTableCompFactor;

    if (coupling && channel == 1)
      delta_F_bits = h_sbrCodeEnvelope->start_bits_balance;
    else
      delta_F_bits = h_sbrCodeEnvelope->start_bits;

    if (h_sbrCodeEnvelope->upDate != 0) {
      delta_T[0] = (curr_nrg - h_sbrCodeEnvelope->sfb_nrg_prev[0]) >> envDataTableCompFactor;

      delta_T_bits = computeBits(&delta_T[0],
                                 codeBookScfLavLevelTime,
                                 codeBookScfLavBalanceTime,
                                 hufftableLevelTimeL,
                                 hufftableBalanceTimeL, coupling, channel);
    }

    mapLowResEnergyVal(curr_nrg, h_sbrCodeEnvelope->sfb_nrg_prev, offset, 0, freq_res[i]);

    for (band = 1; band < no_of_bands; band++) {
      last_nrg = (*ptr_nrg);
      ptr_nrg++;
      curr_nrg = (*ptr_nrg);

      delta_F[band] = (curr_nrg - last_nrg) >> envDataTableCompFactor;

      delta_F_bits += computeBits(&delta_F[band],
                                  codeBookScfLavLevelFreq,
                                  codeBookScfLavBalanceFreq,
                                  hufftableLevelFreqL,
                                  hufftableBalanceFreqL, coupling, channel);

      if (h_sbrCodeEnvelope->upDate != 0) {
        delta_T[band] = curr_nrg - h_sbrCodeEnvelope->sfb_nrg_prev[indexLow2High(offset, band, freq_res[i])];
        delta_T[band] = delta_T[band] >> envDataTableCompFactor;
      }

      mapLowResEnergyVal(curr_nrg, h_sbrCodeEnvelope->sfb_nrg_prev, offset, band, freq_res[i]);

      if (h_sbrCodeEnvelope->upDate != 0) {
        delta_T_bits += computeBits(&delta_T[band],
                                    codeBookScfLavLevelTime,
                                    codeBookScfLavBalanceTime,
                                    hufftableLevelTimeL,
                                    hufftableBalanceTimeL, coupling, channel);
      }
    }

    if (i == 0) {
      use_dT = (h_sbrCodeEnvelope->upDate != 0 && (delta_F_bits > delta_T_bits * (1 + dF_edge_1stEnv)));
    } else {
      use_dT = (delta_T_bits < delta_F_bits && h_sbrCodeEnvelope->upDate != 0);
    }

    if (use_dT) {
      directionVec[i] = TIME;
      memcpy(sfb_nrg, delta_T, no_of_bands * sizeof(int));
    } else {
      h_sbrCodeEnvelope->upDate = 0;
      directionVec[i] = FREQ;
      memcpy(sfb_nrg, delta_F, no_of_bands * sizeof(int));
    }

    sfb_nrg += no_of_bands;
    h_sbrCodeEnvelope->upDate = 1;
  }
}

HANDLE_ERROR_INFO
CreateSbrCodeEnvelope(HANDLE_SBR_CODE_ENVELOPE *hSbr,
                      int *nSfb,
                      int deltaTAcrossFrames,
                      float dF_edge_1stEnv,
                      float dF_edge_incr) {
  HANDLE_SBR_CODE_ENVELOPE hs;

  hs =
      (HANDLE_SBR_CODE_ENVELOPE)iisCalloc(1, sizeof(SBR_CODE_ENVELOPE));
  if (hs == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  hs->deltaTAcrossFrames = deltaTAcrossFrames;
  hs->dF_edge_1stEnv = dF_edge_1stEnv;
  hs->dF_edge_incr = dF_edge_incr;
  hs->dF_edge_incr_fac = 0;
  hs->upDate = 0;
  hs->nSfb[FREQ_RES_LOW] = nSfb[FREQ_RES_LOW];
  hs->nSfb[FREQ_RES_HIGH] = nSfb[FREQ_RES_HIGH];
  hs->offset = 2 * hs->nSfb[FREQ_RES_LOW] - hs->nSfb[FREQ_RES_HIGH];

  hs->sfb_nrg_prev = (int *)iisCalloc(MAX_FREQ_COEFFS, sizeof(int));
  if (hs->sfb_nrg_prev == NULL)
    return iisUtil_ERROR(CDI, "out of memory");

  *hSbr = hs;
  return (noError);
}

void deleteSbrCodeEnvelope(HANDLE_SBR_CODE_ENVELOPE hSbrCut) {
  if (hSbrCut) {
    iisFree(hSbrCut->sfb_nrg_prev);
    iisFree(hSbrCut);
  }
}
