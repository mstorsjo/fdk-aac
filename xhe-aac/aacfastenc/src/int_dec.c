
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mathlib.h"
#include "imdct.h"
#include "quantize.h"
#include "ms_stereo.h"
#include "grp_data.h"
#include "win_coef.h"
#include "iis_fft.h"
#include "tns_func.h"

#include "int_dec.h"

#define INTERLEAVE_FAC 1

static void window(HANDLE_TFDEC hTfDec, int blockType, int windowShape, int prevWindowShape, int useACENext, int useACEPrev, float* data);

int iisaacfenc_CreateIntDec(HANDLE_TFDEC* hTfDEC, unsigned int nChannels, unsigned int nSamplingRate, int granuleLength) {
  int i, error = 0;

  if (*hTfDEC == NULL) {
    *hTfDEC = (HANDLE_TFDEC)iisCalloc(1, sizeof(TFDEC));
    if (*hTfDEC == NULL) {
      return -1;
    }
  }
  (*hTfDEC)->nChannels = nChannels;
  (*hTfDEC)->overlap_buffer = (float**)iisCalloc(nChannels, sizeof(float*));
  for (i = 0; i < (int)nChannels; i++) {
    (*hTfDEC)->overlap_buffer[i] = (float*)iisCalloc(FRAME_LEN_LONG, sizeof(float));
  }

  (void)nSamplingRate;

  (*hTfDEC)->nfSeed = 12345;
  (*hTfDEC)->granuleLength = granuleLength;
  if (error == 0) {
    if ((*hTfDEC)->pShortWindowSineLpdStart != NULL) {
      iisFree((*hTfDEC)->pShortWindowSineLpdStart);
    }
    (*hTfDEC)->pShortWindowSineLpdStart = (float*)iisCalloc((*hTfDEC)->granuleLength / (TRANS_FAC / 2), sizeof(float));
    if ((*hTfDEC)->pShortWindowSineLpdStart == NULL) {
      error = 1;
    }
  }

  if (error == 0) {
    if ((*hTfDEC)->pShortWindowKBDLpdStart != NULL) {
      iisFree((*hTfDEC)->pShortWindowKBDLpdStart);
    }
    (*hTfDEC)->pShortWindowKBDLpdStart = (float*)iisCalloc((*hTfDEC)->granuleLength / (TRANS_FAC / 2), sizeof(float));
    if ((*hTfDEC)->pShortWindowKBDLpdStart == NULL) {
      error = 1;
    }
  }

  if (error == 0) {
    int offset = (*hTfDEC)->granuleLength / (2 * TRANS_FAC);
    switch (granuleLength) {
      case 1024:
        (*hTfDEC)->pLongWindowSine = LongWindowSine1024;
        (*hTfDEC)->pLongWindowKBD = LongWindowKBD1024;
        (*hTfDEC)->pShortWindowSine = ShortWindowSine128;
        (*hTfDEC)->pShortWindowKBD = ShortWindowKBD128;
        offset = 0;
        setFLOAT(0.0, (*hTfDEC)->pShortWindowSineLpdStart, (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowSine256, &(*hTfDEC)->pShortWindowSineLpdStart[offset], (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        setFLOAT(0.0, (*hTfDEC)->pShortWindowKBDLpdStart, (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowKBD256, &(*hTfDEC)->pShortWindowKBDLpdStart[offset], (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        break;

        break;

      case 768:
        (*hTfDEC)->pLongWindowSine = LongWindowSine768;
        (*hTfDEC)->pLongWindowKBD = LongWindowKBD768;
        (*hTfDEC)->pShortWindowSine = ShortWindowSine96;
        (*hTfDEC)->pShortWindowKBD = ShortWindowKBD96;
        offset = 0;
        setFLOAT(0.0, (*hTfDEC)->pShortWindowSineLpdStart, (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowSine192, &(*hTfDEC)->pShortWindowSineLpdStart[offset], (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        setFLOAT(0.0, (*hTfDEC)->pShortWindowKBDLpdStart, (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        copyFLOAT(WindowKBD192, &(*hTfDEC)->pShortWindowKBDLpdStart[offset], (*hTfDEC)->granuleLength / (TRANS_FAC / 2));
        break;
      default:
        error = 1;
        break;
    }
  }

  if (error == 0) {
    if (IIS_FFT_NO_ERROR != IIS_CFFT_Create(&(*hTfDEC)->hFftLong, (*hTfDEC)->granuleLength / 2, IIS_FFT_BWD)) {
      error = -1;
    }
  }

  if (error == 0) {
    if (IIS_FFT_NO_ERROR != IIS_CFFT_Create(&(*hTfDEC)->hFftShort, (*hTfDEC)->granuleLength / TRANS_FAC / 2, IIS_FFT_BWD)) {
      error = -1;
    }
  }

  return error;
}

int iisaacfenc_DeleteIntDec(HANDLE_TFDEC htfDec) {
  int i;

  if (htfDec == NULL) {
    return (0);
  }

  if (htfDec->pShortWindowSineLpdStart) {
    iisFree(htfDec->pShortWindowSineLpdStart);
  }
  if (htfDec->pShortWindowKBDLpdStart) {
    iisFree(htfDec->pShortWindowKBDLpdStart);
  }

  if (htfDec->hFftLong != NULL) {
    IIS_CFFT_Destroy(&htfDec->hFftLong);
  }

  if (htfDec->hFftShort != NULL) {
    IIS_CFFT_Destroy(&htfDec->hFftShort);
  }

  for (i = 0; i < htfDec->nChannels; i++) {
    if (htfDec->overlap_buffer[i] != NULL) {
      iisFree(htfDec->overlap_buffer[i]);
    }
  }
  if (htfDec->overlap_buffer != NULL) {
    iisFree(htfDec->overlap_buffer);
  }
  iisFree(htfDec);
  htfDec = NULL;

  return (0);
}

int iisaacfenc_AdvanceIntDecSaac(HANDLE_TFDEC htfDec,
                                 PSY_OUT_CHANNEL* psyOutChannel[SIGMAP_MAX_SIGNALS],
                                 QC_OUT_CHANNEL* qcOutChannel[SIGMAP_MAX_SIGNALS],
                                 PSY_CONFIGURATION* psyConf,
                                 int channels,
                                 int nNumberElements,
                                 int* coreModeNext,
                                 int* coreModePrev,
                                 ELEMENT_INFO elemInfo[SIGMAP_MAX_ELEMENTS],
                                 PSY_OUT_ELEMENT* psyOutElement[SIGMAP_MAX_ELEMENTS]) {
  int ele, chIdx;
  int useAcelpNext = 0, useAcelpPrev = 0;

  float(*invQuantSpec)[FRAME_LEN_LONG] = htfDec->invQuantSpec;
  float* degroupedSpec = htfDec->degroupedSpec;
  float* timeSig = htfDec->timeSig;

  (void)channels;

  for (chIdx = 0; chIdx < SIGMAP_MAX_SIGNALS_PER_ELEMENT; chIdx++) {
    setFLOAT(0, invQuantSpec[chIdx], FRAME_LEN_LONG);
  }
  setFLOAT(0, timeSig, 2 * (FRAME_LEN_LONG));
  setFLOAT(0, degroupedSpec, FRAME_LEN_LONG);

  for (ele = 0; ele < nNumberElements; ele++) {
    int chInEle = elemInfo[ele].nChannelsInEl;

    for (chIdx = 0; chIdx < chInEle; chIdx++) {
      int ch = elemInfo[ele].ChannelIndex[chIdx];

      iisaacfenc_InvQuantizeSpectrum(psyOutChannel[ch]->sfbCnt,
                                     psyOutChannel[ch]->maxSfbPerGroup,
                                     psyOutChannel[ch]->sfbPerGroup,
                                     psyOutChannel[ch]->sfbOffsets,
                                     qcOutChannel[ch]->quantSpec,
                                     qcOutChannel[ch]->globalGain,
                                     qcOutChannel[ch]->scf,
                                     invQuantSpec[chIdx]);
    }

    if ((elemInfo[ele].elType == ID_CPE) && (elemInfo[ele].nChannelsInEl == 2)) {
      if (psyOutElement[ele]->commonWindow == 1) {
        int confIdx = (psyOutChannel[elemInfo[ele].ChannelIndex[0]]->windowSequence == SHORT_WINDOW) ? 1 : 0;
        iisaacfenc_Dematrix(psyOutChannel[elemInfo[ele].ChannelIndex[0]]->noOfGroups,
                            psyConf[confIdx].sfbCnt,
                            psyOutChannel[elemInfo[ele].ChannelIndex[0]]->groupLen,
                            psyConf[confIdx].sfbOffset,
                            psyOutElement[ele]->toolsInfo.msMask,
                            psyOutElement[ele]->toolsInfo.msDigest,
                            invQuantSpec[0],
                            invQuantSpec[1]);
      }
    }

    for (chIdx = 0; chIdx < chInEle; chIdx++) {
      int ch = elemInfo[ele].ChannelIndex[chIdx];

      useAcelpNext = (coreModeNext[ch] == 1);
      useAcelpPrev = (coreModePrev[ch] == 1);

      if (psyOutChannel[ch]->windowSequence == SHORT_WINDOW) {
        int wnd;

        iisaacfenc_DegroupSpectrum(invQuantSpec[chIdx],
                                   psyConf[1].sfbCnt,
                                   psyOutChannel[ch]->noOfGroups,
                                   psyOutChannel[ch]->groupLen,
                                   psyConf[1].sfbOffset,
                                   degroupedSpec,
                                   htfDec->granuleLength);

        for (wnd = 0; wnd < TRANS_FAC; wnd++) {
          int offset = wnd * htfDec->granuleLength / TRANS_FAC;

          iisaacfenc_TnsDecode(&psyOutChannel[ch]->tnsInfo,
                               psyConf[1].sfbCnt,
                               psyConf[1].tnsConf,
                               psyConf[1].sfbOffset,
                               degroupedSpec + offset,
                               wnd);
        }
      } else {
        iisaacfenc_TnsDecode(&psyOutChannel[ch]->tnsInfo,
                             psyConf[0].sfbCnt,
                             psyConf[0].tnsConf,
                             psyConf[0].sfbOffset,
                             invQuantSpec[chIdx], 0);
        copyFLOAT(invQuantSpec[chIdx], degroupedSpec, htfDec->granuleLength);
      }

      {
        if (psyOutChannel[ch]->windowSequence == SHORT_WINDOW) {
          iisaacfenc_IMDCT_noOA(degroupedSpec,
                                timeSig,
                                psyOutChannel[ch]->windowSequence,
                                htfDec->granuleLength,
                                htfDec->granuleLength / TRANS_FAC,
                                TRANS_FAC,
                                htfDec->granuleLength / TRANS_FAC,
                                htfDec->hFftShort);
        } else {
          iisaacfenc_IMDCT_noOA(invQuantSpec[chIdx],
                                timeSig,
                                psyOutChannel[ch]->windowSequence,
                                htfDec->granuleLength,
                                htfDec->granuleLength / TRANS_FAC,
                                TRANS_FAC,
                                htfDec->granuleLength / TRANS_FAC,
                                htfDec->hFftLong);
          window(htfDec,
                 psyOutChannel[ch]->windowSequence,
                 psyOutChannel[ch]->windowShape,
                 psyOutChannel[ch]->prevWindowShape,
                 useAcelpNext,
                 useAcelpPrev,
                 timeSig);
        }
      }

      copyFLOAT(timeSig, qcOutChannel[ch]->synthTime, 2 * htfDec->granuleLength);
    }
  }

  return (0);
}

static void window(HANDLE_TFDEC hTfDec, int blockType, int windowShape, int prevWindowShape, int useACENext, int useACEPrev, float* data) {
  int i;
  const float *leftWindowPart, *rightWindowPart;
  int granuleLength = hTfDec->granuleLength;
  int lsTrans = (granuleLength - (granuleLength / TRANS_FAC)) / 2;
  int lfac = granuleLength / TRANS_FAC;

  switch (blockType) {
    case LONG_WINDOW:
      if (prevWindowShape == SINE_WINDOW) {
        leftWindowPart = &hTfDec->pLongWindowSine[0];
      } else {
        leftWindowPart = &hTfDec->pLongWindowKBD[0];
      }

      if (windowShape == SINE_WINDOW) {
        rightWindowPart = &hTfDec->pLongWindowSine[0];
      } else {
        rightWindowPart = &hTfDec->pLongWindowKBD[0];
      }

      for (i = 0; i < granuleLength; i++) {
        data[i] *= leftWindowPart[i];
        data[granuleLength + i] *= rightWindowPart[granuleLength - i - 1];
      }

      break;

    case START_WINDOW:

      if (prevWindowShape == SINE_WINDOW) {
        leftWindowPart = &hTfDec->pLongWindowSine[0];
      } else {
        leftWindowPart = &hTfDec->pLongWindowKBD[0];
      }

      if (!useACENext) {
        if (windowShape == KBD_WINDOW) {
          rightWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          rightWindowPart = &hTfDec->pShortWindowSine[0];
        }
      } else {
        if (windowShape == KBD_WINDOW) {
          rightWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
        } else {
          rightWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
        }
      }

      for (i = 0; i < granuleLength / 2; i++) {
        data[i] *= leftWindowPart[i];
      }

      if (useACENext) {
        int foldingPoint = granuleLength / 2;
        for (i = 0; i < foldingPoint - lfac; i++) {
          data[(2 * granuleLength - 1 - i) * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < 2 * lfac; i++) {
          data[(granuleLength + foldingPoint - lfac + i) * INTERLEAVE_FAC] *= rightWindowPart[2 * lfac - i - 1];
        }
      } else {
        for (i = 0; i < lsTrans; i++) {
          data[(2 * granuleLength - 1 - i) * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < granuleLength / (TRANS_FAC); i++) {
          data[(granuleLength + i + lsTrans) * INTERLEAVE_FAC] *= rightWindowPart[granuleLength / TRANS_FAC - i - 1];
        }
      }

      break;

    case STOP_WINDOW:

      if (useACEPrev) {
        if (prevWindowShape == KBD_WINDOW) {
          leftWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
        }
      } else {
        if (prevWindowShape == KBD_WINDOW) {
          leftWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSine[0];
        }
      }

      if (windowShape == SINE_WINDOW) {
        rightWindowPart = &hTfDec->pLongWindowSine[0];
      } else {
        rightWindowPart = &hTfDec->pLongWindowKBD[0];
      }

      if (useACEPrev) {
        int foldingPoint = granuleLength / 2;
        for (i = 0; i < foldingPoint - lfac; i++) {
          data[i * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < 2 * lfac; i++) {
          data[(i + foldingPoint - lfac) * INTERLEAVE_FAC] *= leftWindowPart[i];
        }
      } else {
        for (i = 0; i < lsTrans; i++) {
          data[i * INTERLEAVE_FAC] = 0.0f;
        }
        for (i = 0; i < granuleLength / TRANS_FAC; i++) {
          data[(i + lsTrans) * INTERLEAVE_FAC] *= leftWindowPart[i];
        }
      }

      for (i = 0; i < granuleLength; i++) {
        data[(granuleLength + i) * INTERLEAVE_FAC] *= rightWindowPart[granuleLength - i - 1];
      }

      break;

    case STOPSTART_WINDOW:

      if (useACEPrev) {
        if (prevWindowShape == KBD_WINDOW) {
          leftWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
        }
      } else {
        if (prevWindowShape == KBD_WINDOW) {
          leftWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          leftWindowPart = &hTfDec->pShortWindowSine[0];
        }
      }

      if (!useACENext) {
        if (windowShape == KBD_WINDOW) {
          rightWindowPart = &hTfDec->pShortWindowKBD[0];
        } else {
          rightWindowPart = &hTfDec->pShortWindowSine[0];
        }
      } else {
        if (windowShape == KBD_WINDOW) {
          rightWindowPart = &hTfDec->pShortWindowKBDLpdStart[0];
        } else {
          rightWindowPart = &hTfDec->pShortWindowSineLpdStart[0];
        }
      }

      if (useACEPrev) {
        int foldingPoint = granuleLength / 2;
        for (i = 0; i < foldingPoint - lfac; i++) {
          data[i * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < 2 * lfac; i++) {
          data[(i + foldingPoint - lfac) * INTERLEAVE_FAC] *= leftWindowPart[i];
        }
      } else {
        for (i = 0; i < lsTrans; i++) {
          data[i * INTERLEAVE_FAC] = 0.0f;
        }
        for (i = 0; i < granuleLength / TRANS_FAC; i++) {
          data[(i + lsTrans) * INTERLEAVE_FAC] *= leftWindowPart[i];
        }
      }

      if (useACENext) {
        int foldingPoint = granuleLength / 2;
        for (i = 0; i < foldingPoint - lfac; i++) {
          data[(2 * granuleLength - 1 - i) * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < 2 * lfac; i++) {
          data[(granuleLength + foldingPoint - lfac + i) * INTERLEAVE_FAC] *= rightWindowPart[2 * lfac - i - 1];
        }
      } else {
        for (i = 0; i < lsTrans; i++) {
          data[(2 * granuleLength - 1 - i) * INTERLEAVE_FAC] = 0.0f;
        }

        for (i = 0; i < granuleLength / (TRANS_FAC); i++) {
          data[(granuleLength + i + lsTrans) * INTERLEAVE_FAC] *= rightWindowPart[granuleLength / TRANS_FAC - i - 1];
        }
      }

      break;
    case SHORT_WINDOW:
    default:
      break;
  }
}
