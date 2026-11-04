
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
#include <string.h>
#include <math.h>
#include <limits.h>
#include "bit_aac.h"
#include "codebook.h"
#include "iisArithEncoder.h"

#if defined __GNUC__ || defined __clang__
#define BITMUX_UNUSED __attribute__((unused))
#else
#define BITMUX_UNUSED
#endif

static const int globalGainOffset = 100;
static const int noiseOffset = 90;
static const int icsReservedBit = 0;
#define MAX_PREDICTION_COEF 30

int encodeGlobalGain(BS_SCALEFAC_DATA *scalefacData,
                     int firstSCF,
                     HANDLE_BIT_BUF hBitStream) {
  int bitCount = 0;

  bitCount += WriteBits(hBitStream, scalefacData->globalGain - SCALEFAC_SCALE * (scalefacData->scalefac[firstSCF]) + globalGainOffset, 8);
  return (bitCount);
}

int encodeNoiseLevel(unsigned int noiseLevel,
                     HANDLE_BIT_BUF hBitStream) {
  int bitCount = 0;

  bitCount += WriteBits(hBitStream, noiseLevel, 8);

  return (bitCount);
}

int encodeIcsInfo(BLOCK_TYPE blockType,
                  WIN_SHAPE_NGS mdctWindowShape,
                  int groupingMask,
                  BITMUX_UNUSED int commonWindow,
                  int maxSfb,
                  BITMUX_UNUSED BS_PREDICTION_DATA *predictionData,
                  BITMUX_UNUSED BS_LTP_DATA *bs_ltp_data1,
                  BITMUX_UNUSED BS_LTP_DATA *bs_ltp_data2,
                  BITMUX_UNUSED JS_FLAG *jsFlag1,
                  BITMUX_UNUSED JS_FLAG *jsFlag2,
                  HANDLE_BIT_BUF hBitStream,
                  AUDIO_OBJECT_TYPE aot) {
  int bitCount = 0;

  if (aot != AOT_USAC && aot != AOT_MPEGH) {
    bitCount += WriteBits(hBitStream, icsReservedBit, 1);
  }

  if (blockType == STOPSTART_WINDOW && (aot == AOT_USAC || aot == AOT_MPEGH)) {
    bitCount += WriteBits(hBitStream, START_WINDOW, 2);
  } else {
    bitCount += WriteBits(hBitStream, blockType, 2);
  }

  bitCount += WriteBits(hBitStream, (mdctWindowShape == 0) ? 0 : 1, 1);

  switch (blockType) {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
    case STOPSTART_WINDOW:
    case LOW_OVER_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 6);

      break;
    case SHORT_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 4);
      bitCount += WriteBits(hBitStream, groupingMask, TRANS_FAK - 1);
      break;

    default:
      WARN("Unknown window type");
      break;
  }
  return (bitCount);
}

int encodeIcsInfoScal(BLOCK_TYPE blockType,
                      WIN_SHAPE_NGS mdctWindowShape,
                      int groupingMask,
                      int maxSfb,
                      HANDLE_BIT_BUF hBitStream)

{
  int bitCount = 0;

  bitCount += WriteBits(hBitStream, blockType, 2);
  bitCount += WriteBits(hBitStream, mdctWindowShape, 1);

  switch (blockType) {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
    case STOPSTART_WINDOW:
    case LOW_OVER_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 6);

      break;
    case SHORT_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 4);
      bitCount += WriteBits(hBitStream, groupingMask, TRANS_FAK - 1);
      break;

    default:
      WARN("Unknown window type");
      break;
  }
  return (bitCount);
}

int encodeReducedIcsInfo(BLOCK_TYPE blockType,
                         int groupingMask,
                         int maxSfb,
                         HANDLE_BIT_BUF hBitStream) {
  int bitCount = 0;

  switch (blockType) {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
    case STOPSTART_WINDOW:
    case LOW_OVER_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 6);

      break;
    case SHORT_WINDOW:
      bitCount += WriteBits(hBitStream, maxSfb, 4);
      bitCount += WriteBits(hBitStream, groupingMask, TRANS_FAK - 1);

      break;

    default:
      WARN("Unknown window type");
      break;
  }
  return (bitCount);
}

static int limitToScfLAV(int scf) {
  if (scf > CODE_BOOK_SCF_LAV) {
    scf = CODE_BOOK_SCF_LAV;
  }
  if (scf < -CODE_BOOK_SCF_LAV) {
    scf = -CODE_BOOK_SCF_LAV;
  }
  return scf;
}

int encodeCplxPredData(const int sfbCnt,
                       const int grpSfb,
                       const int maxSfb,
                       const int *jsFlag,
                       const int *predCoefRe, const int *predCoefIm,
                       int *predCoefPrevRe, int *predCoefPrevIm,
                       const int bResetPredictors, const int nGroupsPrev,
                       const BLOCK_TYPE windowSequence, const BLOCK_TYPE windowSequencePrev,
                       const int sfbPerPredBand,
                       const int bUsePrevFrame,
                       HANDLE_BIT_BUF hBitStream,
                       const int bUsacIndependenceFlag) {
  int bDeltaTimeCoding = 0;
  int nBitsDeltaFreq = INT_MAX;
  int nBitsDeltaTime = INT_MAX;
  int deltaRe[2][MAX_GROUPED_SFB] = {{0}};
  int deltaIm[2][MAX_GROUPED_SFB] = {{0}};
  int bComplex = 0;
  int sfbOffset = 0;
  int sfb = 0;
  int bitCount = 0;

  assert(sfbPerPredBand == 2);

  if (((windowSequence == SHORT_WINDOW) && (windowSequencePrev != SHORT_WINDOW)) ||
      ((windowSequencePrev == SHORT_WINDOW) && (windowSequence != SHORT_WINDOW))) {
    setINT(0, predCoefPrevRe, MAX_GROUPED_SFB);
    setINT(0, predCoefPrevIm, MAX_GROUPED_SFB);
    assert(bResetPredictors == 1);
  }

  for (sfbOffset = 0; sfbOffset < sfbCnt; sfbOffset += grpSfb) {
    for (sfb = 0; sfb < maxSfb; sfb += sfbPerPredBand) {
      if ((abs(predCoefRe[sfbOffset + sfb]) > MAX_PREDICTION_COEF) | (abs(predCoefIm[sfbOffset + sfb]) > MAX_PREDICTION_COEF)) {
        assert(0);
        freeErrorTraceback(iisUtil_ERROR(CDI, "Complex prediction encoding out of bound"));

        if (hBitStream == NULL) {
          return MAX_GROUPED_SFB * huff_ltabscf[2 * CODE_BOOK_SCF_LAV];
        }
      }

      if (predCoefIm[sfbOffset + sfb] != 0) {
        bComplex = 1;
      }
    }
  }

  bitCount += WriteBits(hBitStream, bComplex, 1);
  if (bComplex) {
    if (bUsacIndependenceFlag == 0) {
      bitCount += WriteBits(hBitStream, bUsePrevFrame, 1);
    }
  }

  for (int codingMethod = 0; codingMethod < (bResetPredictors ? 1 : 2); codingMethod++) {
    if (codingMethod) {
      nBitsDeltaTime = 0;
    } else {
      nBitsDeltaFreq = 0;
    }

    for (sfbOffset = 0; sfbOffset < sfbCnt; sfbOffset += grpSfb) {
      for (sfb = 0; sfb < maxSfb; sfb += sfbPerPredBand) {
        if (jsFlag[sfbOffset + sfb]) {
          int predRe;
          int predIm;

          assert((sfb == (maxSfb - 1)) || (jsFlag[sfbOffset + sfb + (sfbPerPredBand - 1)] == 1));

          if (codingMethod) {
            if (sfbOffset > 0) {
              predRe = predCoefRe[sfbOffset + sfb - grpSfb];
              predIm = predCoefIm[sfbOffset + sfb - grpSfb];

            } else if (windowSequence == SHORT_WINDOW && windowSequence == windowSequencePrev) {
              assert(sfbOffset == 0);

              predRe = predCoefPrevRe[(nGroupsPrev - 1) * grpSfb + sfb];
              predIm = predCoefPrevIm[(nGroupsPrev - 1) * grpSfb + sfb];

            } else {
              predRe = predCoefPrevRe[sfbOffset + sfb];
              predIm = predCoefPrevIm[sfbOffset + sfb];
            }

          } else {
            if (sfb % grpSfb != 0) {
              predRe = predCoefRe[sfbOffset + sfb - sfbPerPredBand];
              predIm = predCoefIm[sfbOffset + sfb - sfbPerPredBand];
            } else {
              predRe = 0;
              predIm = 0;
            }
          }

          deltaRe[codingMethod][sfbOffset + sfb] = limitToScfLAV(predCoefRe[sfbOffset + sfb] - predRe);
          deltaIm[codingMethod][sfbOffset + sfb] = limitToScfLAV(predCoefIm[sfbOffset + sfb] - predIm);
          const int nHuffReBits = huff_ltabscf[deltaRe[codingMethod][sfbOffset + sfb] + CODE_BOOK_SCF_LAV];
          const int nHuffImBits = huff_ltabscf[deltaIm[codingMethod][sfbOffset + sfb] + CODE_BOOK_SCF_LAV];

          if (codingMethod) {
            nBitsDeltaTime += nHuffReBits;
            if (bComplex) {
              nBitsDeltaTime += nHuffImBits;
            }
          } else {
            nBitsDeltaFreq += nHuffReBits;
            if (bComplex) {
              nBitsDeltaFreq += nHuffImBits;
            }
          }
        }
      }
    }
  }

  if (bUsacIndependenceFlag) {
    bDeltaTimeCoding = 0;
  } else {
    if (nBitsDeltaFreq < nBitsDeltaTime) {
      bDeltaTimeCoding = 0;
    } else {
      bDeltaTimeCoding = 1;
    }
    bitCount += WriteBits(hBitStream, bDeltaTimeCoding, 1);
  }

  for (sfbOffset = 0; sfbOffset < sfbCnt; sfbOffset += grpSfb) {
    for (sfb = 0; sfb < maxSfb; sfb += sfbPerPredBand) {
      if (jsFlag[sfbOffset + sfb]) {
        const int nHuffReBits = huff_ltabscf[deltaRe[bDeltaTimeCoding][sfbOffset + sfb] + CODE_BOOK_SCF_LAV];
        const int nHuffImBits = huff_ltabscf[deltaIm[bDeltaTimeCoding][sfbOffset + sfb] + CODE_BOOK_SCF_LAV];

        bitCount += WriteBits(hBitStream, huff_ctabscf[deltaRe[bDeltaTimeCoding][sfbOffset + sfb] + CODE_BOOK_SCF_LAV], nHuffReBits);

        if (bComplex) {
          bitCount += WriteBits(hBitStream, huff_ctabscf[deltaIm[bDeltaTimeCoding][sfbOffset + sfb] + CODE_BOOK_SCF_LAV], nHuffImBits);
        }
      }
    }
  }

  return bitCount;
}

int encodeMSInfo(int sfbCnt,
                 int grpSfb,
                 int maxSfb,
                 JS_FLAG *jsFlag,
                 int maxSfbLastLayer,
                 AUDIO_OBJECT_TYPE aot,
                 int bCplxPredMdct,
                 int *predCoeffRe,
                 int *predCoeffIm,
                 int *predCoeffPrevRe,
                 int *predCoeffPrevIm,
                 int bResetPredictors,
                 int nGroupsPrev,
                 int wSeq,
                 int wSeqPrev,
                 int sfbPerPredBand,
                 int bSwap,
                 int bUsePrevFrame,
                 HANDLE_BIT_BUF hBitStream,
                 int const bUsacIndependenceFlag) {
  int sfbOff, i;
  int msAllOff = 1;
  int msAllOn = 1;
  int bitCount = 0;
  int bCplxPredActive = 0;

  for (sfbOff = 0; sfbOff < sfbCnt; sfbOff += grpSfb) {
    for (i = maxSfbLastLayer; i < maxSfb; i++) {
      if (jsFlag[sfbOff + i] & MS_ON || jsFlag[sfbOff + i] & PNS_CORR_ON || jsFlag[sfbOff + i] & IS_MS_ON || jsFlag[sfbOff + i] & IS_INV_PHASE)
        msAllOff = 0;
      else
        msAllOn = 0;

      if (aot == AOT_USAC || aot == AOT_MPEGH) {
        if (jsFlag[sfbOff + i] & PNS_CORR_ON)
          msAllOff = 0;
      } else {
        if (jsFlag[sfbOff + i] & IS_INV_PHASE) {
          msAllOn = 0;
          msAllOff = 0;
        }
      }

      if (bCplxPredMdct && (predCoeffRe[sfbOff + i] || predCoeffIm[sfbOff + i])) {
        bCplxPredActive = 1;
      }
    }
  }

  if (msAllOff) {
    bitCount += WriteBits(hBitStream, SI_MS_MASK_ALL_ZERO, 2);
    return (bitCount);
  }

  if (bCplxPredActive) {
    bitCount += WriteBits(hBitStream, SI_MS_MASK_CPLX_PRED, 2);
  }

  if (msAllOn) {
    if (bCplxPredActive) {
      bitCount += WriteBits(hBitStream, 1, 1);
      bitCount += WriteBits(hBitStream, bSwap, 1);
      bitCount += encodeCplxPredData(sfbCnt, grpSfb, maxSfb, jsFlag,
                                     predCoeffRe, predCoeffIm, predCoeffPrevRe, predCoeffPrevIm,
                                     bResetPredictors, nGroupsPrev,
                                     wSeq, wSeqPrev,
                                     sfbPerPredBand,
                                     bUsePrevFrame,
                                     hBitStream,
                                     bUsacIndependenceFlag);
    } else {
      bitCount += WriteBits(hBitStream, SI_MS_MASK_ALL_ONE, 2);
    }
    return (bitCount);
  }

  if (bCplxPredActive) {
    bitCount += WriteBits(hBitStream, 0, 1);
  } else {
    bitCount += WriteBits(hBitStream, SI_MS_MASK_SOME, 2);
  }

  if (bCplxPredActive) {
    for (sfbOff = 0; sfbOff < sfbCnt; sfbOff += grpSfb) {
      for (i = maxSfbLastLayer; i < maxSfb; i += sfbPerPredBand) {
        assert((i >= (maxSfb - 1)) || (jsFlag[sfbOff + i] == jsFlag[sfbOff + i + 1]));
        if ((jsFlag[sfbOff + i] & MS_ON) ||
            (jsFlag[sfbOff + i] & PNS_CORR_ON) ||
            (jsFlag[sfbOff + i] & IS_MS_ON))
          bitCount += WriteBits(hBitStream, 1, 1);
        else
          bitCount += WriteBits(hBitStream, 0, 1);
      }
    }
  } else {
    for (sfbOff = 0; sfbOff < sfbCnt; sfbOff += grpSfb) {
      for (i = maxSfbLastLayer; i < maxSfb; i++) {
        if ((jsFlag[sfbOff + i] & MS_ON) ||
            (jsFlag[sfbOff + i] & PNS_CORR_ON) ||
            (jsFlag[sfbOff + i] & IS_MS_ON) ||
            ((jsFlag[sfbOff + i] & IS_INV_PHASE) && (aot != AOT_USAC && aot != AOT_MPEGH)))
          bitCount += WriteBits(hBitStream, 1, 1);
        else
          bitCount += WriteBits(hBitStream, 0, 1);
      }
    }
  }

  if (bCplxPredActive) {
    for (sfbOff = 0; sfbOff < sfbCnt; sfbOff += grpSfb) {
      for (i = 0; i < maxSfb; i += sfbPerPredBand) {
        if (jsFlag[sfbOff + i] == 0) {
          assert(predCoeffRe[sfbOff + i] == 0);
          assert(predCoeffIm[sfbOff + i] == 0);
        }
      }
    }
    bitCount += WriteBits(hBitStream, bSwap, 1);
    bitCount += encodeCplxPredData(sfbCnt, grpSfb, maxSfb, jsFlag,
                                   predCoeffRe, predCoeffIm, predCoeffPrevRe, predCoeffPrevIm,
                                   bResetPredictors, nGroupsPrev,
                                   wSeq, wSeqPrev,
                                   sfbPerPredBand,
                                   bUsePrevFrame,
                                   hBitStream,
                                   bUsacIndependenceFlag);
  }

  return (bitCount);
}

int encodeScaleFactorData(int *sfbBandOffset,
                          BS_SECTION_DATA *sectionData,
                          BS_SCALEFAC_DATA *scalefacData,
                          int *quantSpectrum,
                          AUDIO_OBJECT_TYPE aot,
                          int useNoiseFilling,
                          HANDLE_BIT_BUF hBitStream) {
  int i, j, k, lastValScf, deltaScf, lastValIs, deltaIs;
  int lastValPns, deltaPns;
  int noisePCMFlag = 1;
  int *scalefac, *isPosition, *noiseNrg;
  int bitCount = 0;
  int scfBits = 0;

  scalefac = scalefacData->scalefac;
  isPosition = scalefacData->isPosition;
  noiseNrg = scalefacData->noiseNrg;

  lastValScf = scalefac[sectionData->firstSCF];
  lastValIs = 0;
  lastValPns = scalefacData->globalGain - scalefacData->scalefac[sectionData->firstSCF] + globalGainOffset - noiseOffset;

  for (i = 0; i < sectionData->noOfSections; i++) {
    if ((sectionData->section[i].codeBook == CODE_BOOK_IS_OUT_OF_PHASE_NO) ||
        (sectionData->section[i].codeBook == CODE_BOOK_IS_IN_PHASE_NO)) {
      for (j = sectionData->section[i].sfbStart; j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt; j++) {
        deltaIs = isPosition[j] - lastValIs;
        lastValIs = isPosition[j];
        assert(abs(deltaIs) <= CODE_BOOK_SCF_LAV);

        if (deltaIs > CODE_BOOK_SCF_LAV) {
          lastValIs -= (deltaIs - CODE_BOOK_SCF_LAV);
          deltaIs = CODE_BOOK_SCF_LAV;
        }
        if (deltaIs < -CODE_BOOK_SCF_LAV) {
          lastValIs -= (deltaIs + CODE_BOOK_SCF_LAV);
          deltaIs = -CODE_BOOK_SCF_LAV;
        }

        bitCount += WriteBits(hBitStream, huff_ctabscf[deltaIs + CODE_BOOK_SCF_LAV],
                              huff_ltabscf[deltaIs + CODE_BOOK_SCF_LAV]);
      }
    } else if (sectionData->section[i].codeBook == CODE_BOOK_PNS_NO) {
      for (j = sectionData->section[i].sfbStart; j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt; j++) {
        deltaPns = noiseNrg[j] - lastValPns;
        lastValPns = noiseNrg[j];

        if (noisePCMFlag) {
          bitCount += WriteBits(hBitStream, deltaPns + (1 << (9 - 1)), 9);
          noisePCMFlag = 0;
        } else {
          assert(abs(deltaPns) <= CODE_BOOK_SCF_LAV);
          bitCount += WriteBits(hBitStream, huff_ctabscf[deltaPns + CODE_BOOK_SCF_LAV],
                                huff_ltabscf[deltaPns + CODE_BOOK_SCF_LAV]);
        }
      }
    }

    else if (sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO) {
      for (j = sectionData->section[i].sfbStart; j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt; j++) {
        deltaScf = 0;

        if (!useNoiseFilling) {
          for (k = sfbBandOffset[j]; k < sfbBandOffset[j + 1]; k++) {
            if (quantSpectrum[k] != 0) {
              deltaScf = -(scalefac[j] - lastValScf);
              lastValScf = scalefac[j];
              break;
            }
          }
        } else {
          deltaScf = -(scalefac[j] - lastValScf);
          lastValScf = scalefac[j];
        }

        assert(abs(deltaScf) <= CODE_BOOK_SCF_LAV);

        if (!((aot == AOT_USAC || aot == AOT_MPEGH) && (i == 0) && (j == 0))) {
          scfBits += WriteBits(hBitStream, huff_ctabscf[deltaScf + CODE_BOOK_SCF_LAV],
                               huff_ltabscf[deltaScf + CODE_BOOK_SCF_LAV]);
        }
      }
    }
  }
  bitCount += scfBits;

  return (bitCount);
}

int encodeTnsData(BS_TNS_DATA *tnsData,
                  AUDIO_OBJECT_TYPE aot,
                  HANDLE_BIT_BUF hBitStream) {
  int i, j, subBlockCnt, coeffCnt;
  int coeffBits;

  int bitCount = -1;

  if (aot == AOT_USAC || aot == AOT_MPEGH) {
    bitCount = 0;
  }

  if ((tnsData->tnsEnabled == 0) || (tnsData->tnsActive == 0)) {
    if (aot != AOT_USAC && aot != AOT_MPEGH) {
      bitCount += WriteBits(hBitStream, 0, 1);
    }
  } else {
    if (aot != AOT_USAC && aot != AOT_MPEGH) {
      bitCount += WriteBits(hBitStream, 1, 1);
    }

    for (subBlockCnt = 0; subBlockCnt < tnsData->numOfSubblocks; subBlockCnt++) {
      if (tnsData->numOfSubblocks == 1) {
        bitCount += WriteBits(hBitStream,
                              tnsData->subBlock[subBlockCnt].numOfFilters,
                              2);
      } else {
        bitCount += WriteBits(hBitStream,
                              tnsData->subBlock[subBlockCnt].numOfFilters,
                              1);
      }

      if (tnsData->subBlock[subBlockCnt].numOfFilters > 0) {
        if (tnsData->subBlock[subBlockCnt].coefficientResolution == 3) {
          bitCount += WriteBits(hBitStream, 0, 1);
        } else {
          bitCount += WriteBits(hBitStream, 1, 1);
        }
      }

      for (i = 0; i < tnsData->subBlock[subBlockCnt].numOfFilters; i++) {
        const TNS_FILTER *filter;
        filter = &tnsData->subBlock[subBlockCnt].filters[i];
        if (tnsData->numOfSubblocks == 1) {
          bitCount += WriteBits(hBitStream,
                                abs(filter->stopBand - filter->startBand),
                                6);
          if (aot == AOT_USAC || aot == AOT_MPEGH) {
            bitCount += WriteBits(hBitStream, filter->order, 4);
          } else {
            bitCount += WriteBits(hBitStream, filter->order, 5);
          }
        } else {
          bitCount += WriteBits(hBitStream,
                                abs(filter->stopBand - filter->startBand),
                                4);
          bitCount += WriteBits(hBitStream, filter->order, 3);
        }
        if (filter->order > 0) {
          bitCount += WriteBits(hBitStream, filter->direction, 1);
          if (tnsData->subBlock[subBlockCnt].coefficientResolution == 4) {
            coeffBits = 3;
            for (j = 0; j < tnsData->subBlock[subBlockCnt].filters[i].order; j++) {
              if (tnsData->subBlock[subBlockCnt].filters[i].coefficients[j] > 3 ||
                  tnsData->subBlock[subBlockCnt].filters[i].coefficients[j] < -4) {
                coeffBits = 4;
                break;
              }
            }
            bitCount += WriteBits(hBitStream, -(coeffBits - 4), 1);

            for (coeffCnt = 0; coeffCnt < filter->order; coeffCnt++) {
              static const int rmask[] = {0, 1, 3, 7, 15};
              bitCount += WriteBits(hBitStream,
                                    (filter->coefficients[coeffCnt] & rmask[coeffBits]),
                                    coeffBits);
            }
          } else {
            coeffBits = 2;
            for (j = 0; j < tnsData->subBlock[subBlockCnt].filters[i].order; j++) {
              if (tnsData->subBlock[subBlockCnt].filters[i].coefficients[j] > 1 ||
                  tnsData->subBlock[subBlockCnt].filters[i].coefficients[j] < -2) {
                coeffBits = 3;
                break;
              }
            }
            bitCount += WriteBits(hBitStream, -(coeffBits - 3), 1);
            for (coeffCnt = 0; coeffCnt < filter->order; coeffCnt++) {
              static const int rmask[] = {0, 1, 3, 7, 15};
              bitCount += WriteBits(hBitStream,
                                    (filter->coefficients[coeffCnt] & rmask[coeffBits]),
                                    coeffBits);
            }
          }
        }
      }
    }
  }
  return (bitCount);
}

typedef struct ENC_SPEC_DATA_ARITH2 {
  int frameCount;
  int reset;
  int resetRate;
  ARIENC_PUBLIC_DATA_HANDLE hArithEncoder;
} ENC_SPEC_DATA_ARITH2;

ARIENC_PUBLIC_DATA_HANDLE getArithEncoder(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2) {
  return hEncSpecDataArith2->hArithEncoder;
}

int createSpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 *phEncSpecDataArith2, int resetDistance) {
  int error = 0;

  if (error == 0) {
    if (NULL == (*phEncSpecDataArith2 = iisCalloc(1, sizeof(ENC_SPEC_DATA_ARITH2)))) {
      error = -1;
    }
  }

  if (error == 0) {
    (*phEncSpecDataArith2)->resetRate = resetDistance;
  }

  iisArithEncoderOpen(&(*phEncSpecDataArith2)->hArithEncoder);

  return error;
}

int destroySpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 *phEncSpecDataArith2) {
  if (phEncSpecDataArith2) {
    iisArithEncoderClose((*phEncSpecDataArith2)->hArithEncoder);

    if (*phEncSpecDataArith2) {
      iisFree(*phEncSpecDataArith2);
    }

    *phEncSpecDataArith2 = NULL;
  }

  return 0;
}

int encodeSpectralDataArith2(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2,
                             int const *sfbBandOffset,
                             int sfbCnt,
                             int grpSfb,
                             int maxSfbPerGroup,
                             int groupingMask,
                             BLOCK_TYPE blockType,
                             int const *quantSpectrum,
                             int bUsacIndepFlag,
                             HANDLE_BIT_BUF hBitStream) {
  int sfb;
  int bitCount = 0;
  int maxNumberLines, encodeNumberLines;
  int arith_reset_flag = -1;
  int nWindowsInGroup[TRANS_FAK] = {0}, nQuadruples[TRANS_FAK] = {0};
  int windowGroup = 1;
  int sfbBandOffsetUngrouped[MAX_SFB] = {0};
  int i;
  int w;
  int nWindows = (blockType == SHORT_WINDOW) ? TRANS_FAK : 1;
  int quantSpectrumReOrdered[STD_GRANULE_LEN];
  int unalignedBits, sqBits = 0;

  unsigned char const *tempBitBuffer = NULL;
  ARIENC_ERROR retError = ARIENC_OK;

  switch (blockType) {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
    case STOPSTART_WINDOW:
      maxNumberLines = 1024;
      if (sfbBandOffset[sfbCnt] == 768) {
        encodeNumberLines = 768;
      } else {
        encodeNumberLines = 1024;
      }
      break;
    case SHORT_WINDOW:
      maxNumberLines = 128;
      if (sfbBandOffset[sfbCnt] == 768) {
        encodeNumberLines = 96;
      } else {
        encodeNumberLines = 128;
      }

      break;
    default:
      maxNumberLines = 0;
      encodeNumberLines = 0;
      break;
  }

  hEncSpecDataArith2->reset = 0;
  if ((hEncSpecDataArith2->frameCount == 0) || (bUsacIndepFlag)) {
    arith_reset_flag = 1;
    hEncSpecDataArith2->reset = 1;
  }

  if ((hBitStream != NULL)) {
    if (hEncSpecDataArith2->resetRate > -1) {
      hEncSpecDataArith2->frameCount = (hEncSpecDataArith2->frameCount + 1) % hEncSpecDataArith2->resetRate;
    } else {
      hEncSpecDataArith2->frameCount = -1;
    }
  }

  nWindowsInGroup[windowGroup - 1] = 1;
  for (w = 0; w < nWindows - 1; w++) {
    if ((groupingMask & (1 << (6 - w))) == 0) {
      windowGroup++;
      nWindowsInGroup[windowGroup - 1] = 0;
    }
    nWindowsInGroup[windowGroup - 1]++;
  }

  for (sfb = 0; sfb < grpSfb + 1; sfb++) {
    sfbBandOffsetUngrouped[sfb] = sfbBandOffset[sfb] / nWindowsInGroup[0];
  }

  iisArithEncoderRestoreContextState(hEncSpecDataArith2->hArithEncoder);

  if (hBitStream == NULL) {
    iisArithEncoderSaveContextState(hEncSpecDataArith2->hArithEncoder);
  }

  {
    int windowOffset = 0, groupOffset = 0, tmpOffset = 0;
    memset(quantSpectrumReOrdered, 0, sizeof(quantSpectrumReOrdered));
    for (sfb = 0; sfb < sfbCnt; sfb++) {
      for (w = 0; w < nWindowsInGroup[groupOffset]; w++) {
        int sfbOffsetPerWindow = (sfbBandOffset[sfb + 1] - sfbBandOffset[sfb]) / nWindowsInGroup[groupOffset];

        for (i = 0; i < sfbOffsetPerWindow; i++) {
          quantSpectrumReOrdered[(windowOffset + w) * maxNumberLines + tmpOffset + i] = quantSpectrum[sfbBandOffset[sfb] + w * sfbOffsetPerWindow + i];
        }
      }
      assert(nWindowsInGroup[groupOffset] > 0);
      tmpOffset += (sfbBandOffset[sfb + 1] - sfbBandOffset[sfb]) / nWindowsInGroup[groupOffset];
      if (sfb + 1 == (groupOffset + 1) * grpSfb) {
        for (i = windowOffset; i < windowOffset + nWindowsInGroup[groupOffset]; i++) {
          nQuadruples[i] = sfbBandOffsetUngrouped[maxSfbPerGroup];
          nQuadruples[i] /= 2;
        }
        windowOffset += nWindowsInGroup[groupOffset];
        groupOffset += 1;
        tmpOffset = 0;
      }
    }
  }

  for (w = 0; w < nWindows; w++) {
    assert(nQuadruples[w] <= maxNumberLines);

    retError = iisArithEncoderEncode(
        hEncSpecDataArith2->hArithEncoder,
        NULL,
        &sqBits,
        &quantSpectrumReOrdered[w * maxNumberLines],
        nQuadruples[w] * 2,
        &arith_reset_flag,
        encodeNumberLines * 2);

    if (w == 0) hEncSpecDataArith2->reset = arith_reset_flag;
    arith_reset_flag = 0;
  }

  if (!bUsacIndepFlag) {
    bitCount += WriteBits(hBitStream, hEncSpecDataArith2->reset, 1);
  }

  if (hBitStream != NULL) {
    int nBitsArithEnc = 0;

    iisArithEncoderGetData(hEncSpecDataArith2->hArithEncoder, &tempBitBuffer, &nBitsArithEnc);
    assert(nBitsArithEnc == sqBits);

    for (i = 0; i < sqBits - 7; i += 8) {
      bitCount += WriteBits(hBitStream, tempBitBuffer[i / 8], 8);
    }

    unalignedBits = sqBits % 8;
    bitCount += WriteBits(hBitStream, tempBitBuffer[i / 8] >> (8 - unalignedBits), unalignedBits);
  }

  if (hBitStream == NULL) {
    bitCount += sqBits;
  }

  if (retError == ARIENC_WARNING_BUFFER_FULL) {
    return -1;
  }
  return bitCount;
}

void resetSpectralDataArithFrameCount2(HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith) {
  if (hEncSpecDataArith != NULL) {
    hEncSpecDataArith->frameCount = 0;
  }
}

int encodePitchData(BS_PITCH_DATA *bs_pitch_data, HANDLE_BIT_BUF hBitStream) {
  int bitCount = 0;

  bitCount += WriteBits(hBitStream, bs_pitch_data->bActive, 1);

  if (bs_pitch_data->bActive == 1) {
    int i;
    for (i = 0; i < bs_pitch_data->nPitches; i++) {
      bitCount += WriteBits(hBitStream, bs_pitch_data->pCodedPitchIdx[i], bs_pitch_data->nBits);
    }
  }

  return (bitCount);
}

int getSpectralDataArith2(
    HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith2,
    HANDLE_BIT_BUF hBitBuf,
    unsigned int const bUsacIndepFlag) {
  unsigned char const *pData = NULL;
  int nBits = 0;
  int totBitCount = 0;

  if (hEncSpecDataArith2 != NULL) {
    iisArithEncoderGetData(hEncSpecDataArith2->hArithEncoder, &pData, &nBits);
    iisArithEncoderSetLastContextState(hEncSpecDataArith2->hArithEncoder);

    if (pData != NULL) {
      int nBytesArithEnc = nBits >> 3;
      int remBits = nBits % 8;
      nBytesArithEnc += (remBits > 0);
      if (hBitBuf != NULL) {
        if (hEncSpecDataArith2->resetRate > -1) {
          hEncSpecDataArith2->frameCount = (hEncSpecDataArith2->frameCount + 1) % hEncSpecDataArith2->resetRate;
        } else {
          hEncSpecDataArith2->frameCount = -1;
        }
      }
      if (!bUsacIndepFlag) {
        totBitCount += WriteBits(hBitBuf, hEncSpecDataArith2->reset, 1);
      }
      WriteBytes(hBitBuf, pData, nBytesArithEnc);
      if (remBits > 0) {
        SetWritePointer(hBitBuf, -(8 - remBits));
      }
      totBitCount += nBits;
    }
  }

  return totBitCount;
}
