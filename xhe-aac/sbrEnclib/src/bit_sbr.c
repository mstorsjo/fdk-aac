
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
#include <assert.h>
#include <math.h>

#include "sbr_def.h"
#include "bit_buf.h"
#include "sbr_main.h"
#include "bit_sbr.h"
#include "sbr.h"
#include "code_env.h"
#include "cmondata.h"
#include "mathlib.h"

#define SI_SBR_EXTENSION_SIZE_BITS 4
#define SI_SBR_EXTENSION_ESC_COUNT_BITS 8
#define SI_SBR_EXTENSION_ID_BITS 2

#define EXTENSION_ID_PS_CODING 2
#define EXTENSION_ID_ESBR 3

static int encodeSbrData(
    HANDLE_SBR_ENCODER hEnvEncoder,
    HANDLE_COMMON_DATA cmonData,
    SBR_ELEMENT_TYPE sbrElem,
    CODEC_TYPE coreCodec, int const bUsacIndependenceFlag);

static int encodeSbrHeader(
    HANDLE_SBR_HEADER_DATA sbrHeaderData,
    HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData,
    HANDLE_COMMON_DATA cmonData,
    CODEC_TYPE coreCodec, int const bUsacIndependenceFlag);

static int encodeSbrInfoUsac(
    HANDLE_SBR_HEADER_DATA sbrHeaderData,
    HANDLE_BIT_BUF hBitStream);

static int encodeSbrSingleChannelElement(
    HANDLE_SBR_ENCODER hEnvEncoder,
    HANDLE_BIT_BUF hBitStream,
    CODEC_TYPE coreCodec, int const bUsacIndependenceFlag);

static int encodeSbrChannelPairElement(
    HANDLE_SBR_ENCODER hEnvEncoder,
    HANDLE_BIT_BUF hBitStream,
    CODEC_TYPE coreCodec, int const bUsacIndependenceFlag);

static int encodeSbrGrid(
    HANDLE_SBR_ENV_DATA sbrEnvData,
    HANDLE_BIT_BUF hBitStream,
    CODEC_TYPE coreCoder);

static int encodeSbrDtdf(
    HANDLE_SBR_ENV_DATA sbrEnvData,
    HANDLE_BIT_BUF hBitStream,
    CODEC_TYPE coreCodec, int const bUsacIndependenceFlag);

static int writeNoiseLevelData(
    HANDLE_SBR_ENV_DATA sbrEnvData,
    HANDLE_BIT_BUF hBitStream,
    int coupling);

static int writeSBREnvelopeData(
    HANDLE_SBR_ENV_DATA sbrEnvData,
    HANDLE_BIT_BUF hBitStream,
    int coupling, int bs_interTes,
    CODEC_TYPE coreCodec);
static int writeSyntheticCodingData(
    HANDLE_SBR_ENV_DATA sbrEnvData,
    HANDLE_BIT_BUF hBitStream,
    CODEC_TYPE coreCodec);

int WriteSbrSingleChannelElement(struct SBR_ENCODER *hEnvEncoder,
                                 HANDLE_COMMON_DATA cmonData,
                                 CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;

  cmonData->sbrHdrBits = 0;
  cmonData->sbrDataBits = 0;
  cmonData->sbrCrcLen = 0;

  if (hEnvEncoder->hEnvChannel[0] != NULL) {
    if (coreCodec == CODEC_SAAC) {
      payloadBits += encodeSbrHeader(hEnvEncoder->sbrHeaderData,
                                     hEnvEncoder->sbrBitstreamData,
                                     cmonData,
                                     coreCodec,
                                     bUsacIndependenceFlag);
    } else {
      payloadBits += encodeSbrHeader(hEnvEncoder->sbrHeaderData,
                                     hEnvEncoder->sbrBitstreamData,
                                     cmonData,
                                     coreCodec, bUsacIndependenceFlag);
    }

    payloadBits += encodeSbrData(hEnvEncoder,
                                 cmonData,
                                 SBR_ID_SCE,
                                 coreCodec, bUsacIndependenceFlag);
  }
  return payloadBits;
}

int WriteSbrChannelPairElement(HANDLE_SBR_ENCODER hEnvEncoder,
                               HANDLE_COMMON_DATA hCmonData,
                               CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;
  hCmonData->sbrHdrBits = 0;
  hCmonData->sbrDataBits = 0;
  hCmonData->sbrCrcLen = 0;

  hCmonData->sbrDataBitsEnh = 0;
  hCmonData->sbrCrcLenEnh = 0;

  if ((hEnvEncoder->hEnvChannel[0] != NULL) && (hEnvEncoder->hEnvChannel[1] != NULL)) {
    if (coreCodec == CODEC_SAAC) {
      payloadBits += encodeSbrHeader(hEnvEncoder->sbrHeaderData,
                                     hEnvEncoder->sbrBitstreamData,
                                     hCmonData,
                                     coreCodec,
                                     bUsacIndependenceFlag);
    } else {
      payloadBits += encodeSbrHeader(hEnvEncoder->sbrHeaderData,
                                     hEnvEncoder->sbrBitstreamData,
                                     hCmonData,
                                     coreCodec, bUsacIndependenceFlag);
    }

    payloadBits += encodeSbrData(hEnvEncoder,
                                 hCmonData,
                                 SBR_ID_CPE,
                                 coreCodec, bUsacIndependenceFlag);
  }
  return payloadBits;
}

int CountSbrChannelPairElement(HANDLE_SBR_ENCODER hEnvEncoder,
                               HANDLE_COMMON_DATA hCmonData,
                               CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  HANDLE_BIT_BUF hbb = hCmonData->hSbrBitbuf;
  HANDLE_BIT_BUF hbbEnh = hCmonData->hSbrBitbufEnh;
  int payloadBits;

  hCmonData->hSbrBitbuf = NULL;
  hCmonData->hSbrBitbufEnh = NULL;

  payloadBits = WriteSbrChannelPairElement(hEnvEncoder,
                                           hCmonData,
                                           coreCodec, bUsacIndependenceFlag);

  hCmonData->hSbrBitbuf = hbb;
  hCmonData->hSbrBitbufEnh = hbbEnh;

  return payloadBits;
}

int CountSbrSingleChannelElement(HANDLE_SBR_ENCODER hEnvEncoder,
                                 HANDLE_COMMON_DATA hCmonData,
                                 CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  HANDLE_BIT_BUF hbb = hCmonData->hSbrBitbuf;
  HANDLE_BIT_BUF hbbEnh = hCmonData->hSbrBitbufEnh;
  int payloadBits;

  hCmonData->hSbrBitbuf = NULL;
  hCmonData->hSbrBitbufEnh = NULL;

  payloadBits = WriteSbrSingleChannelElement(hEnvEncoder,
                                             hCmonData,
                                             coreCodec, bUsacIndependenceFlag);

  hCmonData->hSbrBitbuf = hbb;
  hCmonData->hSbrBitbufEnh = hbbEnh;

  return payloadBits;
}

static int
encodeSbrHeader(HANDLE_SBR_HEADER_DATA sbrHeaderData,
                HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData,
                HANDLE_COMMON_DATA cmonData,
                CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;
  int sbrHeaderPresent = 0;
  int sbrUseDfltHeader = 1;
  int sbrInfoPresent = sbrBitstreamData->HeaderActive;

  if (sbrBitstreamData->CRCActive) {
    cmonData->sbrCrcLen = 1;
    cmonData->sbrCrcLenEnh = 1;

  } else {
    cmonData->sbrCrcLen = 0;
    cmonData->sbrCrcLenEnh = 0;
  }
  switch (coreCodec) {
    case CODEC_SAAC:

      if (bUsacIndependenceFlag) {
        sbrInfoPresent = 1;
        sbrHeaderPresent = 1;
      } else {
        payloadBits += WriteBits(cmonData->hSbrBitbuf, sbrInfoPresent, SI_SBR_HEADER_FLAG);
        if (sbrInfoPresent) {
          payloadBits += WriteBits(cmonData->hSbrBitbuf, sbrHeaderPresent, SI_SBR_HEADER_FLAG);
        }
      }
      if (sbrInfoPresent) {
        payloadBits += encodeSbrInfoUsac(sbrHeaderData,
                                         cmonData->hSbrBitbuf);
      }
      if (sbrHeaderPresent) {
        payloadBits += WriteBits(cmonData->hSbrBitbuf, sbrUseDfltHeader, SI_SBR_HEADER_FLAG);
        if (!sbrUseDfltHeader) {
          payloadBits += encodeSbrHeaderData(sbrHeaderData, cmonData->hSbrBitbuf, coreCodec);
        }
      }
      break;
    default:
      assert(0);
  }

  cmonData->sbrHdrBits = payloadBits;

  return payloadBits;
}

int encodeSbrHeaderData(HANDLE_SBR_HEADER_DATA sbrHeaderData,
                        HANDLE_BIT_BUF hBitStream,
                        CODEC_TYPE coreCodec) {
  int payloadBits = 0;

  if (sbrHeaderData != NULL) {
    switch (coreCodec) {
      case CODEC_SAAC:

        payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_start_frequency,
                                 SI_SBR_START_FREQ_BITS);
        payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_stop_frequency,
                                 SI_SBR_STOP_FREQ_BITS);
        payloadBits += WriteBits(hBitStream, sbrHeaderData->header_extra_1,
                                 SI_SBR_HEADER_EXTRA_1_BITS);
        payloadBits += WriteBits(hBitStream, sbrHeaderData->header_extra_2,
                                 SI_SBR_HEADER_EXTRA_2_BITS);

        break;
      default:
        assert(0);
    }

    if (sbrHeaderData->header_extra_1) {
      payloadBits += WriteBits(hBitStream, sbrHeaderData->freqScale,
                               SI_SBR_FREQ_SCALE_BITS);
      payloadBits += WriteBits(hBitStream, sbrHeaderData->alterScale,
                               SI_SBR_ALTER_SCALE_BITS);
      payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_noise_bands,
                               SI_SBR_NOISE_BANDS_BITS);
    }

    if (sbrHeaderData->header_extra_2) {
      payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_limiter_bands,
                               SI_SBR_LIMITER_BANDS_BITS);
      payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_limiter_gains,
                               SI_SBR_LIMITER_GAINS_BITS);
      payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_interpol_freq,
                               SI_SBR_INTERPOL_FREQ_BITS);
      payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_smoothing_length,
                               SI_SBR_SMOOTHING_LENGTH_BITS);
    }
  }

  return payloadBits;
}

static int
encodeSbrInfoUsac(HANDLE_SBR_HEADER_DATA sbrHeaderData,
                  HANDLE_BIT_BUF hBitStream) {
  int payloadBits = 0;
  if (sbrHeaderData != NULL) {
    payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_amp_res,
                             SI_SBR_AMP_RES_BITS);
    payloadBits += WriteBits(hBitStream, sbrHeaderData->sbr_xover_band,
                             SI_SBR_XOVER_BAND_BITS_SAAC);
    payloadBits += WriteBits(hBitStream, 1,
                             SI_SBR_PRE_FLAT_BITS);
  }

  return payloadBits;
}

static int
encodeSbrData(HANDLE_SBR_ENCODER hEnvEncoder,
              HANDLE_COMMON_DATA cmonData,
              SBR_ELEMENT_TYPE sbrElem,
              CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;
  int payloadBitsEnh = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      break;

    default:
      assert(0);
  }

  switch (sbrElem) {
    case SBR_ID_SCE:
      payloadBits += encodeSbrSingleChannelElement(hEnvEncoder, cmonData->hSbrBitbuf, coreCodec, bUsacIndependenceFlag);
      break;
    case SBR_ID_CPE:
      payloadBits += encodeSbrChannelPairElement(hEnvEncoder, cmonData->hSbrBitbuf, coreCodec, bUsacIndependenceFlag);
      break;
    default:

      assert(0);
  }

  cmonData->sbrDataBits = payloadBits;
  cmonData->sbrDataBitsEnh = payloadBitsEnh;

  return payloadBits + payloadBitsEnh;
}

static int
encodeSbrSingleChannelElement(HANDLE_SBR_ENCODER hEnvEncoder,
                              HANDLE_BIT_BUF hBitStream,
                              CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;
  HANDLE_SBR_ENV_DATA sbrEnvData = &hEnvEncoder->hEnvChannel[0]->encEnvData;
  int bs_interTes = hEnvEncoder->sbrConfigData->bs_interTes;

  switch (coreCodec) {
    case CODEC_SAAC:

      break;
    default:
      assert(0);
  }

  if (hEnvEncoder->sbrConfigData->bMonoStereoScal) {
    payloadBits += WriteBits(hBitStream, 1, SI_SBR_COUPLING_BITS);
  }

  switch (coreCodec) {
    case CODEC_SAAC:
      payloadBits += encodeSbrGrid(sbrEnvData, hBitStream, coreCodec);
      break;
    default:
      break;
  }

  payloadBits += encodeSbrDtdf(sbrEnvData, hBitStream, coreCodec, bUsacIndependenceFlag);

  switch (coreCodec) {
    case CODEC_SAAC: {
      int i;
      for (i = 0; i < sbrEnvData->noOfnoisebands; i++) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->sbr_invf_mode_vec[i], SI_SBR_INVF_MODE_BITS);
      }
    } break;
    default:
      assert(0);
  }

  payloadBits += writeSBREnvelopeData(sbrEnvData, hBitStream, 0, bs_interTes, coreCodec);
  payloadBits += writeNoiseLevelData(sbrEnvData, hBitStream, 0);

  switch (coreCodec) {
    case CODEC_SAAC:
      payloadBits += writeSyntheticCodingData(sbrEnvData, hBitStream, coreCodec);
      break;

    default:
      assert(0);
  }

  switch (coreCodec) {
    case CODEC_SAAC:
      break;
    default:
      assert(0);
  }

  return payloadBits;
}

static int
encodeSbrChannelPairElement(HANDLE_SBR_ENCODER hEnvEncoder,
                            HANDLE_BIT_BUF hBitStream,
                            CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int payloadBits = 0;

  HANDLE_SBR_ENV_DATA sbrEnvDataLeft = &hEnvEncoder->hEnvChannel[0]->encEnvData;
  HANDLE_SBR_ENV_DATA sbrEnvDataRight = &hEnvEncoder->hEnvChannel[1]->encEnvData;
  int bs_interTes = hEnvEncoder->sbrConfigData->bs_interTes;

  int i = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      break;
    default:
      break;
  }

  payloadBits += WriteBits(hBitStream, hEnvEncoder->sbrHeaderData->coupling,
                           SI_SBR_COUPLING_BITS);

  if (hEnvEncoder->sbrHeaderData->coupling) {
    switch (coreCodec) {
      case CODEC_SAAC:
        payloadBits += encodeSbrGrid(sbrEnvDataLeft, hBitStream, coreCodec);
        break;
      default:
        break;
    }

    payloadBits += encodeSbrDtdf(sbrEnvDataLeft, hBitStream, coreCodec, bUsacIndependenceFlag);
    payloadBits += encodeSbrDtdf(sbrEnvDataRight, hBitStream, coreCodec, bUsacIndependenceFlag);

    switch (coreCodec) {
      case CODEC_SAAC:
        for (i = 0; i < sbrEnvDataLeft->noOfnoisebands; i++) {
          payloadBits += WriteBits(hBitStream, sbrEnvDataLeft->sbr_invf_mode_vec[i], SI_SBR_INVF_MODE_BITS);
        }
        break;
      default:
        assert(0);
    }

    payloadBits += writeSBREnvelopeData(sbrEnvDataLeft, hBitStream, 1, bs_interTes, coreCodec);
    payloadBits += writeNoiseLevelData(sbrEnvDataLeft, hBitStream, 1);

    payloadBits += writeSBREnvelopeData(sbrEnvDataRight, hBitStream, 1, bs_interTes, coreCodec);
    payloadBits += writeNoiseLevelData(sbrEnvDataRight, hBitStream, 1);

  } else {
    switch (coreCodec) {
      case CODEC_SAAC:
        payloadBits += encodeSbrGrid(sbrEnvDataLeft, hBitStream, coreCodec);
        payloadBits += encodeSbrGrid(sbrEnvDataRight, hBitStream, coreCodec);
        break;
      default:
        break;
    }

    payloadBits += encodeSbrDtdf(sbrEnvDataLeft, hBitStream, coreCodec, bUsacIndependenceFlag);
    payloadBits += encodeSbrDtdf(sbrEnvDataRight, hBitStream, coreCodec, bUsacIndependenceFlag);

    switch (coreCodec) {
      case CODEC_SAAC:
        for (i = 0; i < sbrEnvDataLeft->noOfnoisebands; i++) {
          payloadBits += WriteBits(hBitStream, sbrEnvDataLeft->sbr_invf_mode_vec[i],
                                   SI_SBR_INVF_MODE_BITS);
        }
        for (i = 0; i < sbrEnvDataRight->noOfnoisebands; i++) {
          payloadBits += WriteBits(hBitStream, sbrEnvDataRight->sbr_invf_mode_vec[i],
                                   SI_SBR_INVF_MODE_BITS);
        }
        break;
      default:
        assert(0);
    }

    payloadBits += writeSBREnvelopeData(sbrEnvDataLeft, hBitStream, 0, bs_interTes, coreCodec);
    payloadBits += writeSBREnvelopeData(sbrEnvDataRight, hBitStream, 0, bs_interTes, coreCodec);
    payloadBits += writeNoiseLevelData(sbrEnvDataLeft, hBitStream, 0);
    payloadBits += writeNoiseLevelData(sbrEnvDataRight, hBitStream, 0);
  }

  switch (coreCodec) {
    case CODEC_SAAC:
      payloadBits += writeSyntheticCodingData(sbrEnvDataLeft, hBitStream, coreCodec);
      payloadBits += writeSyntheticCodingData(sbrEnvDataRight, hBitStream, coreCodec

      );

      break;

    default:
      break;
  }

  switch (coreCodec) {
    case CODEC_SAAC:
      break;
    default:
      assert(0);
      break;
  }

  return payloadBits;
}

static int
encodeSbrGrid(HANDLE_SBR_ENV_DATA sbrEnvData,
              HANDLE_BIT_BUF hBitStream,
              CODEC_TYPE coreCoder) {
  int payloadBits = 0;
  int i, temp;
  int numberTimeSlots = sbrEnvData->hSbrBSGrid->numberTimeSlots;
  switch (coreCoder) {
    case CODEC_SAAC:
      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->frameClass, SBR_CLA_BITS);
      break;
    default:
      break;
  }

  switch (sbrEnvData->hSbrBSGrid->frameClass) {
    case FIXFIX:
      temp = ceillog2(sbrEnvData->hSbrBSGrid->bs_num_env);
      payloadBits += WriteBits(hBitStream, temp, SBR_ENV_BITS);
      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->v_f[0], SBR_RES_BITS);
      break;

    case FIXVAR:
    case VARFIX:
      if (sbrEnvData->hSbrBSGrid->frameClass == FIXVAR)
        temp = sbrEnvData->hSbrBSGrid->bs_abs_bord - numberTimeSlots;
      else
        temp = sbrEnvData->hSbrBSGrid->bs_abs_bord;

      switch (coreCoder) {
        case CODEC_SAAC:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS_AAC);
          break;
        default:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS);
          break;
      }

      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->n, SBR_NUM_BITS);

      for (i = 0; i < sbrEnvData->hSbrBSGrid->n; i++) {
        temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord[i] - 2) >> 1;
        payloadBits += WriteBits(hBitStream, temp, SBR_REL_BITS);
      }

      temp = ceillog2(sbrEnvData->hSbrBSGrid->n + 2);
      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->p, temp);

      for (i = 0; i < sbrEnvData->hSbrBSGrid->n + 1; i++) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->v_f[i],
                                 SBR_RES_BITS);
      }
      break;

    case VARVAR:
      temp = sbrEnvData->hSbrBSGrid->bs_abs_bord_0;
      switch (coreCoder) {
        case CODEC_SAAC:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS_AAC);
          break;
        default:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS);
          break;
      }

      temp = sbrEnvData->hSbrBSGrid->bs_abs_bord_1 - numberTimeSlots;
      switch (coreCoder) {
        case CODEC_SAAC:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS_AAC);
          break;
        default:
          payloadBits += WriteBits(hBitStream, temp, SBR_ABS_BITS);
          break;
      }

      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->bs_num_rel_0, SBR_NUM_BITS);
      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->bs_num_rel_1, SBR_NUM_BITS);

      for (i = 0; i < sbrEnvData->hSbrBSGrid->bs_num_rel_0; i++) {
        temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord_0[i] - 2) >> 1;
        payloadBits += WriteBits(hBitStream, temp, SBR_REL_BITS);
      }

      for (i = 0; i < sbrEnvData->hSbrBSGrid->bs_num_rel_1; i++) {
        temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord_1[i] - 2) >> 1;
        payloadBits += WriteBits(hBitStream, temp, SBR_REL_BITS);
      }

      temp = ceillog2(sbrEnvData->hSbrBSGrid->bs_num_rel_0 + sbrEnvData->hSbrBSGrid->bs_num_rel_1 + 2);
      payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->p, temp);

      temp = sbrEnvData->hSbrBSGrid->bs_num_rel_0 +
             sbrEnvData->hSbrBSGrid->bs_num_rel_1 + 1;

      for (i = 0; i < temp; i++) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->hSbrBSGrid->v_fLR[i],
                                 SBR_RES_BITS);
      }
      break;

    case SBR_FRAME_CLASS_INVALID:
    default:
      assert(0);
      break;
  }
  return payloadBits;
}

static int
encodeSbrDtdf(HANDLE_SBR_ENV_DATA sbrEnvData,
              HANDLE_BIT_BUF hBitStream,
              CODEC_TYPE coreCodec, int const bUsacIndependenceFlag) {
  int i, payloadBits = 0, noOfNoiseEnvelopes = (sbrEnvData->noOfEnvelopes > 1) ? 2 : 1;
  switch (coreCodec) {
    case CODEC_SAAC:
      if (!bUsacIndependenceFlag) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->domain_vec[0], SBR_DIR_BITS);
      }

      for (i = 1; i < sbrEnvData->noOfEnvelopes; ++i) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->domain_vec[i], SBR_DIR_BITS);
      }
      if (!bUsacIndependenceFlag) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->domain_vec_noise[0], SBR_DIR_BITS);
      }

      for (i = 1; i < noOfNoiseEnvelopes; ++i) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->domain_vec_noise[i], SBR_DIR_BITS);
      }
      break;
    default:
      break;
  }

  return payloadBits;
}

static int
writeNoiseLevelData(HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream, int coupling) {
  int j, i, payloadBits = 0;
  int nNoiseEnvelopes = ((sbrEnvData->noOfEnvelopes > 1) ? 2 : 1);

  for (i = 0; i < nNoiseEnvelopes; i++) {
    switch (sbrEnvData->domain_vec_noise[i]) {
      case FREQ:
        if (coupling && sbrEnvData->balance) {
          payloadBits += WriteBits(hBitStream,
                                   sbrEnvData->sbr_noise_levels[i * sbrEnvData->noOfnoisebands],
                                   sbrEnvData->si_sbr_start_noise_bits_balance);
        } else {
          payloadBits += WriteBits(hBitStream,
                                   sbrEnvData->sbr_noise_levels[i * sbrEnvData->noOfnoisebands],
                                   sbrEnvData->si_sbr_start_noise_bits);
        }

        for (j = 1 + i * sbrEnvData->noOfnoisebands; j < (sbrEnvData->noOfnoisebands * (1 + i)); j++) {
          if (coupling) {
            if (sbrEnvData->balance) {
              payloadBits += WriteBits(hBitStream,
                                       sbrEnvData->hufftableNoiseBalanceFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                              CODE_BOOK_SCF_LAV_BALANCE11],
                                       sbrEnvData->hufftableNoiseBalanceFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                              CODE_BOOK_SCF_LAV_BALANCE11]);
            } else {
              payloadBits += WriteBits(hBitStream,
                                       sbrEnvData->hufftableNoiseLevelFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV11],
                                       sbrEnvData->hufftableNoiseLevelFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV11]);
            }
          } else {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableNoiseFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                     CODE_BOOK_SCF_LAV11],
                                     sbrEnvData->hufftableNoiseFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                     CODE_BOOK_SCF_LAV11]);
          }
        }
        break;

      case TIME:
        for (j = i * sbrEnvData->noOfnoisebands; j < (sbrEnvData->noOfnoisebands * (1 + i)); j++) {
          if (coupling) {
            if (sbrEnvData->balance) {
              payloadBits += WriteBits(hBitStream,
                                       sbrEnvData->hufftableNoiseBalanceTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                              CODE_BOOK_SCF_LAV_BALANCE11],
                                       sbrEnvData->hufftableNoiseBalanceTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                              CODE_BOOK_SCF_LAV_BALANCE11]);
            } else {
              payloadBits += WriteBits(hBitStream,
                                       sbrEnvData->hufftableNoiseLevelTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV11],
                                       sbrEnvData->hufftableNoiseLevelTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV11]);
            }
          } else {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableNoiseLevelTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11],
                                     sbrEnvData->hufftableNoiseLevelTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11]);
          }
        }
        break;
    }
  }
  return payloadBits;
}

static int
writeSBREnvelopeData(HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream, int coupling, int bs_interTes, CODEC_TYPE coreCodec) {
  int payloadBits = 0, j, i, delta;

  for (j = 0; j < sbrEnvData->noOfEnvelopes; j++) {
    if (sbrEnvData->domain_vec[j] == FREQ) {
      if (coupling && sbrEnvData->balance) {
        payloadBits += WriteBits(hBitStream, sbrEnvData->ienvelope[j][0], sbrEnvData->si_sbr_start_env_bits_balance);
      } else {
        payloadBits += WriteBits(hBitStream, sbrEnvData->ienvelope[j][0], sbrEnvData->si_sbr_start_env_bits);
      }
    }

    for (i = 1 - sbrEnvData->domain_vec[j]; i < sbrEnvData->noScfBands[j]; i++) {
      delta = sbrEnvData->ienvelope[j][i];

      if (coupling && sbrEnvData->balance) {
        assert(abs(delta) <= sbrEnvData->codeBookScfLavBalance);
      } else {
        assert(abs(delta) <= sbrEnvData->codeBookScfLav);
      }
      if (coupling) {
        if (sbrEnvData->balance) {
          if (sbrEnvData->domain_vec[j]) {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableBalanceTimeC[delta + sbrEnvData->codeBookScfLavBalance],
                                     sbrEnvData->hufftableBalanceTimeL[delta + sbrEnvData->codeBookScfLavBalance]);
          } else {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableBalanceFreqC[delta + sbrEnvData->codeBookScfLavBalance],
                                     sbrEnvData->hufftableBalanceFreqL[delta + sbrEnvData->codeBookScfLavBalance]);
          }
        } else {
          if (sbrEnvData->domain_vec[j]) {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableLevelTimeC[delta + sbrEnvData->codeBookScfLav],
                                     sbrEnvData->hufftableLevelTimeL[delta + sbrEnvData->codeBookScfLav]);
          } else {
            payloadBits += WriteBits(hBitStream,
                                     sbrEnvData->hufftableLevelFreqC[delta + sbrEnvData->codeBookScfLav],
                                     sbrEnvData->hufftableLevelFreqL[delta + sbrEnvData->codeBookScfLav]);
          }
        }
      } else {
        if (sbrEnvData->domain_vec[j]) {
          payloadBits += WriteBits(hBitStream,
                                   sbrEnvData->hufftableTimeC[delta + sbrEnvData->codeBookScfLav],
                                   sbrEnvData->hufftableTimeL[delta + sbrEnvData->codeBookScfLav]);
        } else {
          payloadBits += WriteBits(hBitStream,
                                   sbrEnvData->hufftableFreqC[delta + sbrEnvData->codeBookScfLav],
                                   sbrEnvData->hufftableFreqL[delta + sbrEnvData->codeBookScfLav]);
        }
      }
    }
    if (coreCodec == CODEC_SAAC && bs_interTes) {
      payloadBits += WriteBits(hBitStream,
                               sbrEnvData->bs_temp_shape[j],
                               SI_SBR_TEMP_SHAPE_BITS);

      if (sbrEnvData->bs_temp_shape[j] != 0) {
        assert(sbrEnvData->bs_temp_shape_mode_bits[j] == SI_SBR_INTER_TEMP_SHAPE_MODE_BITS);
        payloadBits += WriteBits(hBitStream,
                                 sbrEnvData->bs_temp_shape_mode_value[j],
                                 SI_SBR_INTER_TEMP_SHAPE_MODE_BITS);
      }
    }
  }
  return payloadBits;
}

static int writeSyntheticCodingData(HANDLE_SBR_ENV_DATA sbrEnvData,
                                    HANDLE_BIT_BUF hBitStream,
                                    CODEC_TYPE coreCodec) {
  int i;
  int payloadBits = 0;

  switch (coreCodec) {
    case CODEC_SAAC:
      payloadBits += WriteBits(hBitStream, sbrEnvData->addHarmonicFlag, 1);
      break;
    default:
      assert(0);
  }

  if (sbrEnvData->addHarmonicFlag) {
    for (i = 0; i < sbrEnvData->noHarmonics; i++) {
      payloadBits += WriteBits(hBitStream, sbrEnvData->addHarmonic[i], 1);
    }
  }

  return payloadBits;
}
