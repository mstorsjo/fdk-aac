
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
#include <limits.h>

#include "dyn_bits.h"
#include "bit_cnt.h"
#include "psy_const.h"
#include "mathlib.h"
#include "pns.h"
#include "codebook.h"
#include "glob_con.h"
#include "iisutillib.h"
#include "bit_aac.h"

static int iisaacfenc_calcSideInfoBits(
    const int sfbCnt,
    const int blockType) {
  int seg_len_bits = (blockType == SHORT_WINDOW) ? 3 : 5;
  int escape_val = (blockType == SHORT_WINDOW) ? 7 : 31;
  int sideInfoBits = 0;
  int tmp = 0;

  sideInfoBits = CODE_BOOK_BITS;

  tmp = sfbCnt;

  while (tmp >= 0) {
    sideInfoBits += seg_len_bits;
    tmp -= escape_val;
  }

  return sideInfoBits;
}

static void iisaacfenc_scfCount(
    const int* scalefacGain,
    const unsigned int* maxValueInSfb,
    SECTION_DATA* sectionData,
    const int* noiseNrg,
    const int* isScale,
    int bUsacSyntax,
    int useNoiseFilling) {
  int i = 0;
  int j = 0;
  int k = 0;
  int m = 0;
  int n = 0;

  int lastValScf = 0;
  int deltaScf = 0;
  int found = 0;
  int scfSkipCounter = 0;
  int lastValPns = 0;
  int deltaPns = 0;
  int lastValIs = 0;
  int deltaIs = 0;
  int noisePCMFlag = 1;
  sectionData->scalefacBits = 0;

  if (scalefacGain == NULL) {
    return;
  }

  lastValScf = 0;
  sectionData->firstScf = 0;

  for (i = 0; i < sectionData->noOfSections; i++) {
    if (sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO) {
      sectionData->firstScf = sectionData->section[i].sfbStart;
      lastValScf = scalefacGain[sectionData->firstScf];
      break;
    }
  }

  for (i = 0; i < sectionData->noOfSections; i++) {
    if ((sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO) &&
        (sectionData->section[i].codeBook < CODE_BOOK_PNS_NO)) {
      for (j = sectionData->section[i].sfbStart;
           j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt;
           j++) {
        if ((maxValueInSfb[j] == 0) && (!useNoiseFilling)) {
          found = 0;

          if (scfSkipCounter == 0) {
            if (j == (sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt - 1)) {
              found = 0;
            } else {
              for (k = (j + 1);
                   k < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt; k++) {
                if (maxValueInSfb[k] != 0) {
                  found = 1;

                  if ((abs(scalefacGain[k] - lastValScf)) <= CODE_BOOK_SCF_LAV) {
                    deltaScf = 0;
                  } else {
                    deltaScf = -(scalefacGain[j] - lastValScf);
                    lastValScf = scalefacGain[j];
                    scfSkipCounter = 0;
                  }
                  break;
                }

                scfSkipCounter = scfSkipCounter + 1;
              }
            }

            for (m = (i + 1); (m < sectionData->noOfSections) && (found == 0); m++) {
              if ((sectionData->section[m].codeBook != CODE_BOOK_ZERO_NO) &&
                  (sectionData->section[m].codeBook < CODE_BOOK_PNS_NO)) {
                for (n = sectionData->section[m].sfbStart;
                     n < sectionData->section[m].sfbStart + sectionData->section[m].sfbCnt;
                     n++) {
                  if (maxValueInSfb[n] != 0) {
                    found = 1;

                    if ((abs(scalefacGain[n] - lastValScf)) <= CODE_BOOK_SCF_LAV) {
                      deltaScf = 0;
                    } else {
                      deltaScf = -(scalefacGain[j] - lastValScf);
                      lastValScf = scalefacGain[j];
                      scfSkipCounter = 0;
                    }
                    break;
                  }

                  scfSkipCounter = scfSkipCounter + 1;
                }
              }
            }

            if (found == 0) {
              deltaScf = 0;
              scfSkipCounter = 0;
            }
          } else {
            deltaScf = 0;
            scfSkipCounter = scfSkipCounter - 1;
          }
        } else {
          deltaScf = -(scalefacGain[j] - lastValScf);
          lastValScf = scalefacGain[j];
        }

        assert(abs(deltaScf) <= CODE_BOOK_SCF_LAV);

        if (!(bUsacSyntax && (i == 0) && (j == 0))) {
          sectionData->scalefacBits += iisaacfenc_bitCountScalefactorDelta(deltaScf);
        }
      }
    }
  }

  sectionData->noiseNrgBits = 0;

  for (i = 0; i < sectionData->noOfSections; i++) {
    if (sectionData->section[i].codeBook == CODE_BOOK_PNS_NO) {
      for (j = sectionData->section[i].sfbStart;
           j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt;
           j++) {
        if (noisePCMFlag) {
          sectionData->noiseNrgBits += PNS_PCM_BITS;
          lastValPns = noiseNrg[j];
          noisePCMFlag = 0;
        } else {
          deltaPns = noiseNrg[j] - lastValPns;
          lastValPns = noiseNrg[j];
          sectionData->noiseNrgBits += iisaacfenc_bitCountScalefactorDelta(deltaPns);
        }
      }
    } else if ((sectionData->section[i].codeBook == CODE_BOOK_IS_OUT_OF_PHASE_NO) ||
               (sectionData->section[i].codeBook == CODE_BOOK_IS_IN_PHASE_NO)) {
      for (j = sectionData->section[i].sfbStart;
           j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt;
           j++) {
        deltaIs = isScale[j] - lastValIs;
        lastValIs = isScale[j];

        if (deltaIs > CODE_BOOK_SCF_LAV) {
          lastValIs -= (deltaIs - CODE_BOOK_SCF_LAV);
          deltaIs = CODE_BOOK_SCF_LAV;
        }

        if (deltaIs < -CODE_BOOK_SCF_LAV) {
          lastValIs -= (deltaIs + CODE_BOOK_SCF_LAV);
          deltaIs = -CODE_BOOK_SCF_LAV;
        }

        sectionData->scalefacBits += iisaacfenc_bitCountScalefactorDelta(deltaIs);
      }
    }
  }
}

typedef int (*lookUpTable)[CODE_BOOK_ESC_NDX + 1];

int iisaacfenc_dynBitCount(
    struct BITCNTR_STATE* hBC,
    const signed int* quantSpectrum,
    const unsigned int* maxValueInSfb,
    const signed int* scalefac,
    const int blockType,
    const int sfbCnt,
    const int maxSfbPerGroup,
    const int sfbPerGroup,
    const int* sfbOffset,
    SECTION_DATA* sectionData,
    const int* noiseNrg,
    const int* isScale,
    const int groupingMask,
    const int bUsacIndepFlag,
    HANDLE_ENC_SPEC_DATA_ARITH2 hEncSpecDataArith) {
  sectionData->blockType = blockType;
  sectionData->sfbCnt = sfbCnt;
  sectionData->sfbPerGroup = sfbPerGroup;
  sectionData->noOfGroups = sfbCnt / sfbPerGroup;
  sectionData->maxSfbPerGroup = maxSfbPerGroup;

  int g = 0;
  int offset = 0;

  sectionData->huffmanBits = encodeSpectralDataArith2(
      hEncSpecDataArith,
      sfbOffset,
      sfbCnt,
      sfbPerGroup,
      maxSfbPerGroup,
      groupingMask,
      blockType,
      quantSpectrum,
      bUsacIndepFlag,
      NULL);

  sectionData->sideInfoBits = 0;

  sectionData->noOfSections = sectionData->noOfGroups;

  for (g = 0; g < sectionData->noOfGroups; g++) {
    sectionData->section[g].codeBook = CODE_BOOK_ESC_NDX;
    sectionData->section[g].sfbStart = offset;
    sectionData->section[g].sfbCnt = maxSfbPerGroup;
    offset += sfbPerGroup;
  }

  iisaacfenc_scfCount(
      scalefac,
      maxValueInSfb,
      sectionData,
      noiseNrg,
      isScale,
      hEncSpecDataArith ? 1 : 0,
      hBC->useNoiseFilling);

  if (sectionData->huffmanBits == -1) {
    return -1;
  }

  return (sectionData->huffmanBits +
          sectionData->sideInfoBits +
          sectionData->scalefacBits +
          sectionData->noiseNrgBits);
}

int iisaacfenc_BCNew(
    struct BITCNTR_STATE** phBC) {
  int error = 0;

  if ((*phBC) == NULL) {
    (*phBC) = (struct BITCNTR_STATE*)iisCalloc(1, sizeof(struct BITCNTR_STATE));

    if ((*phBC) == NULL) {
      error = 1;
    }
  }

  if (error == 0) {
    if ((*phBC)->bitLookUp == NULL) {
      (*phBC)->bitLookUp = (int*)iisCalloc(1, sizeof(int) * MAX_SFB_LONG * (CODE_BOOK_ESC_NDX + 1));

      if ((*phBC)->bitLookUp == NULL) {
        error = 1;
        iisaacfenc_BCDelete((*phBC));
        (*phBC) = NULL;
      }
    }
  }

  if (error == 0) {
    if ((*phBC)->mergeGainLookUp == NULL) {
      (*phBC)->mergeGainLookUp = (int*)iisCalloc(1, sizeof(int) * MAX_SFB_LONG);

      if ((*phBC)->mergeGainLookUp == NULL) {
        error = 1;
        iisaacfenc_BCDelete((*phBC));
        (*phBC) = NULL;
      }
    }
  }

  return error;
}

int iisaacfenc_BCDelete(
    struct BITCNTR_STATE* hBC) {
  if (hBC) {
    if (hBC->bitLookUp) {
      iisFree(hBC->bitLookUp);
    }

    if (hBC->mergeGainLookUp) {
      iisFree(hBC->mergeGainLookUp);
    }

    iisFree(hBC);
  }
  return (0);
}

int iisaacfenc_BCInit(
    struct BITCNTR_STATE* hBC,
    const int useNoiseFilling,
    int* sideInfoTabLong,
    int* sideInfoTabShort) {
  int i = 0;
  hBC->useNoiseFilling = useNoiseFilling;

  for (i = 0; i <= MAX_SFB_LONG; i++) {
    sideInfoTabLong[i] = iisaacfenc_calcSideInfoBits(i, LONG_WINDOW);
  }

  for (i = 0; i <= MAX_SFB_SHORT; i++) {
    sideInfoTabShort[i] = iisaacfenc_calcSideInfoBits(i, SHORT_WINDOW);
  }

  return (0);
}
