
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

#include "mathlib.h"

#include "spaceEnclib_const.h"
#include "space_framewindowing.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

typedef struct T_FRAMEWINDOW {
  float *pTaperSyn;
  float *pTaperAna;

  int nTimeSlotsMax;
  int bLowDelay;
  int bFrameKeep;

} FRAMEWINDOW;

typedef enum {
  FIX_INVALID = -1,
  FIX_RECT_SMOOTH = 0,
  FIX_SMOOTH_RECT = 1,
  FIX_LARGE_SMOOTH = 2,
  FIX_RECT_TRIANG = 3
} FIX_TYPE;

typedef enum {
  VAR_INVALID = -1,
  VAR_HOLD = 0,
  VAR_ISOLATE = 1
} VAR_TYPE;

HANDLE_ERROR_INFO
FrameWindow_Create(HANDLE_FRAMEWINDOW *phFrameWindow,
                   FRAMEWINDOW_CONFIG *pFrameWindowConfig) {
  HANDLE_ERROR_INFO error = noError;

  if (pFrameWindowConfig != NULL) {
    if (error == noError) {
      if (NULL == (*phFrameWindow = (HANDLE_FRAMEWINDOW)iisCalloc(1, sizeof(FRAMEWINDOW)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc for hFrameWindow.");
      }
    }

    if (error == noError) {
      (*phFrameWindow)->bLowDelay = pFrameWindowConfig->bLowDelay;
      (*phFrameWindow)->bFrameKeep = pFrameWindowConfig->bFrameKeep;
    }

    if (error == noError) {
      (*phFrameWindow)->nTimeSlotsMax = pFrameWindowConfig->nTimeSlotsMax;
      if ((*phFrameWindow)->nTimeSlotsMax < 0) {
        error = iisUtil_ERROR(CDI, "Invalid number nTimeSlotsMax.");
      }
    }

    if (error == noError) {
      if (NULL == ((*phFrameWindow)->pTaperSyn = (float *)iisCalloc(1, ((*phFrameWindow)->nTimeSlotsMax + 1) * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
      }
    }

    if (error == noError) {
      if (NULL == ((*phFrameWindow)->pTaperAna = (float *)iisCalloc(1, (*phFrameWindow)->nTimeSlotsMax * sizeof(float)))) {
        error = iisUtil_ERROR(CDI, "Unable to calloc memory.");
      }
    }

  } else {
    error = iisUtil_ERROR(CDI, "Invalid pointer.");
  }

  return error;
}

HANDLE_ERROR_INFO
FrameWindow_Destroy(HANDLE_FRAMEWINDOW *phFrameWindow) {
  HANDLE_ERROR_INFO error = noError;

  if (*phFrameWindow) {
    if ((*phFrameWindow)->pTaperAna) {
      iisFree((*phFrameWindow)->pTaperAna);
    }
    (*phFrameWindow)->pTaperAna = NULL;

    if ((*phFrameWindow)->pTaperSyn) {
      iisFree((*phFrameWindow)->pTaperSyn);
    }
    (*phFrameWindow)->pTaperSyn = NULL;

    iisFree(*phFrameWindow);
  }
  *phFrameWindow = NULL;

  return error;
}

static void
FrameWinList_Reset(FRAMEWIN_LIST *pFrameWinList) {
  int k = 0;
  if (NULL == pFrameWinList) return;
  for (k = 0; k < MAX_NUM_PARAMS; k++) {
    pFrameWinList->dat[k].slot = -1;
    pFrameWinList->dat[k].hold = 0;
  }
  pFrameWinList->n = 0;
}

static HANDLE_ERROR_INFO
FrameWindowList_Add(FRAMEWIN_LIST *pFrameWinList, int slot, FW_SLOTTYPE hold) {
  HANDLE_ERROR_INFO error = noError;
  if (NULL == pFrameWinList) {
    error = iisUtil_ERROR(CDI, "Invalid pointer");
  }
  if (noError == error) {
    if (pFrameWinList->n >= MAX_NUM_PARAMS) {
      error = iisUtil_ERROR(CDI, "List full");
    }
  }
  if (noError == error) {
    pFrameWinList->dat[pFrameWinList->n].slot = slot;
    pFrameWinList->dat[pFrameWinList->n].hold = hold;
    pFrameWinList->n++;
  }
  return error;
}

static void
FrameWindowList_Remove(FRAMEWIN_LIST *pFrameWinList, int idx) {
  int k = 0;
  if (NULL == pFrameWinList) return;
  if (idx < 0 || idx >= MAX_NUM_PARAMS) return;
  if (pFrameWinList->n > 0) {
    if (idx == MAX_NUM_PARAMS - 1) {
      pFrameWinList->dat[idx].slot = -1;
      pFrameWinList->dat[idx].hold = 0;
    } else {
      for (k = idx; k < MAX_NUM_PARAMS - 1; k++) {
        pFrameWinList->dat[k] = pFrameWinList->dat[k + 1];
      }
    }
    pFrameWinList->n--;
  }
}

static void
FrameWindowList_Limit(FRAMEWIN_LIST *pFrameWinList, int ll, int ul) {
  int k = 0;
  if (NULL == pFrameWinList) return;
  for (k = 0; k < pFrameWinList->n; k++) {
    if (pFrameWinList->dat[k].slot < ll ||
        pFrameWinList->dat[k].slot > ul) {
      FrameWindowList_Remove(pFrameWinList, k);
      --k;
    }
  }
}

HANDLE_ERROR_INFO
FrameWindow_GetWindow(HANDLE_FRAMEWINDOW hFrameWindow,
                      int tr_pos[MAX_NUM_PARAMS],
                      int timeSlots,
                      FRAMINGINFO *pFramingInfo,
                      float *pWindowAna[MAX_NUM_PARAMS],
                      float *pWindowSyn[MAX_NUM_PARAMS],
                      FRAMEWIN_LIST *pFrameWinList,
                      int avoid_keep) {
  HANDLE_ERROR_INFO error = noError;
  FIX_TYPE fixType = FIX_RECT_TRIANG;
  VAR_TYPE varType = VAR_HOLD;
  int tranL = 4;
  int startSlope = 0;
  int stopSlope = 0;
  int startRect = 0;
  int stopRect = 0;
  int ps = 0;
  int ts = 0;
  int winCnt = 0;
  float applyRightWindowGain[MAX_NUM_PARAMS];

  if (NULL == tr_pos ||
      NULL == pFramingInfo ||
      NULL == pWindowAna ||
      NULL == pWindowSyn ||
      NULL == pFrameWinList ||
      NULL == hFrameWindow) {
    error = iisUtil_ERROR(CDI, "Invalid Pointer");
  }

  if (error == noError) {
    for (ps = 0; ps < MAX_NUM_PARAMS; ps++) {
      if ((pWindowAna[ps] == NULL) || (pWindowSyn[ps] == NULL)) {
        error = iisUtil_ERROR(CDI, "Invalid pointer.");
        break;
      }
    }
  }

  if (error == noError) {
    float *pTaperSyn = hFrameWindow->pTaperSyn;
    float *pTaperAna = hFrameWindow->pTaperAna;
    int taperSynLen = 0;
    int taperAnaLen = 0;

    if ((timeSlots > hFrameWindow->nTimeSlotsMax) ||
        (timeSlots < 0)) {
      error = iisUtil_ERROR(CDI, "Invalid timeSlots.");
    }

    if (noError == error) {
      FrameWinList_Reset(pFrameWinList);
      for (ps = 0; ps < MAX_NUM_PARAMS; ps++) {
        applyRightWindowGain[ps] = 0.0f;
      }
    }

    if (noError == error) {
      switch (fixType) {
        case FIX_RECT_TRIANG:
          if (hFrameWindow->bLowDelay == 0) {
            startSlope = 0;
            stopSlope = 2 * timeSlots - 1;
            startRect = startSlope;
            stopRect = stopSlope;
            for (ts = 0; ts <= timeSlots; ts++) {
              pTaperSyn[ts] = (float)ts / timeSlots;
            }
            taperSynLen = timeSlots;
          }

          break;
        case FIX_SMOOTH_RECT:
          startSlope = 0;
          stopSlope = 2 * timeSlots - 1;
          startRect = timeSlots;
          stopRect = timeSlots - 1;
          setFLOAT(1.0f, pTaperSyn, timeSlots + 1);
          taperSynLen = timeSlots;
          break;
        case FIX_RECT_SMOOTH:
        case FIX_LARGE_SMOOTH:
          error = iisUtil_ERROR(CDI, "Fix Type not implemented");
          break;
        case FIX_INVALID:
        default:
          error = iisUtil_ERROR(CDI, "Invalid Fix Type");
          break;
      }
    }

    if (noError == error) {
      taperAnaLen = startRect - startSlope;
      for (ts = 0; ts < taperAnaLen; ts++) {
        if (hFrameWindow->bLowDelay == 0) {
          pTaperAna[ts] = 0.5f - 0.5f * (float)cos(((float)ts + 0.5f) * PI / taperAnaLen);
        }
      }
    }

    if (noError == error) {
      if (tr_pos[0] > -1) {
        int p_l = tr_pos[0];
        winCnt = 0;

        switch (varType) {
          case VAR_HOLD:
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l - 1, FW_HOLD));
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l, FW_INTP));
            break;
          case VAR_ISOLATE:
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l - 1, FW_HOLD));
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l, FW_INTP));
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l + tranL, FW_HOLD));
            SAFECALL(error, FrameWindowList_Add(pFrameWinList, p_l + tranL + 1, FW_INTP));
            break;
          default:
            error = iisUtil_ERROR(CDI, "Invalid Var Type");
            break;
        }

        if (noError == error) {
          FrameWindowList_Limit(pFrameWinList, 0, timeSlots - 1);
        }

        SAFECALL(error, FrameWindowList_Add(pFrameWinList, timeSlots - 1, FW_HOLD));

        if (noError == error) {
          for (ps = 0; ps < pFrameWinList->n - 1; ps++) {
            if (FW_HOLD != pFrameWinList->dat[ps].hold) {
              int const start = pFrameWinList->dat[ps].slot;
              int const stop = pFrameWinList->dat[ps + 1].slot;

              setFLOAT(0.0f, pWindowAna[winCnt], start);
              setFLOAT(1.0f, &pWindowAna[winCnt][start], stop - start + 1);
              setFLOAT(0.0f, &pWindowAna[winCnt][stop + 1], timeSlots - stop - 1);

              copyFLOAT(pWindowAna[winCnt], pWindowSyn[winCnt], timeSlots);

              applyRightWindowGain[winCnt] = pWindowAna[winCnt][timeSlots - 1];

              winCnt++;
            }
          }
        }

        if (noError == error) {
          FrameWindowList_Remove(pFrameWinList, pFrameWinList->n - 1);
        }
      } else {
        winCnt = 0;

        SAFECALL(error, FrameWindowList_Add(pFrameWinList, timeSlots - 1, FW_INTP));

        if (noError == error) {
          setFLOAT(0.0f, pWindowAna[winCnt], startSlope);
          copyFLOAT(pTaperAna, &pWindowAna[winCnt][startSlope], taperAnaLen);
          setFLOAT(1.0f, &pWindowAna[winCnt][startRect], timeSlots - startRect);

          setFLOAT(0.0f, pWindowSyn[winCnt], timeSlots - taperSynLen);
          copyFLOAT(pTaperSyn, &pWindowSyn[winCnt][timeSlots - taperSynLen], taperSynLen);

          applyRightWindowGain[winCnt] = 1.0f;
          winCnt++;
        }
      }
    }

    if (noError == error) {
      int w = 0;
      for (w = 0; w < winCnt; w++) {
        if (applyRightWindowGain[w] > 0.0f) {
          if (tr_pos[1] > -1) {
            int p_r = tr_pos[1];

            setFLOAT(1.0f, &pWindowAna[w][timeSlots], p_r - timeSlots);
            setFLOAT(0.0f, &pWindowAna[w][p_r], 2 * timeSlots - p_r);

            copyFLOAT(&pWindowAna[w][timeSlots], &pWindowSyn[w][timeSlots], timeSlots);

          } else {
            setFLOAT(1.0f, &pWindowAna[w][timeSlots], stopRect - timeSlots + 1);
            copyFLOATflex(&pTaperAna[taperAnaLen - 1], -1,
                          &pWindowAna[w][stopRect], 1,
                          taperAnaLen);
            setFLOAT(0.0f, &pWindowAna[w][stopSlope + 1], 2 * timeSlots - stopSlope - 1);

            copyFLOATflex(&pTaperSyn[timeSlots], -1,
                          &pWindowSyn[w][timeSlots], 1,
                          taperSynLen);
            setFLOAT(0.0f, &pWindowSyn[w][timeSlots + taperSynLen], timeSlots - taperSynLen);
          }

          if (applyRightWindowGain[w] < 1.0f) {
            smultFLOATip(applyRightWindowGain[w],
                         &pWindowAna[w][timeSlots],
                         timeSlots);
            smultFLOATip(applyRightWindowGain[w],
                         &pWindowSyn[w][timeSlots],
                         timeSlots);
          }

        } else {
          setFLOAT(0.0f, &pWindowAna[w][timeSlots], timeSlots);
          setFLOAT(0.0f, &pWindowSyn[w][timeSlots], timeSlots);
        }
      }
    }

    if (noError == error) {
      pFramingInfo->numParamSets = pFrameWinList->n;
      pFramingInfo->bsFramingType = 1;
      for (ps = 0; ps < pFramingInfo->numParamSets; ps++) {
        pFramingInfo->bsParamSlots[ps] = pFrameWinList->dat[ps].slot;
      }

      if ((pFramingInfo->numParamSets == 1) &&
          (pFramingInfo->bsParamSlots[0] == timeSlots - 1)) {
        pFramingInfo->bsFramingType = 0;
      }
    }
  }

  if (error == noError) {
    if (hFrameWindow->bFrameKeep == 1) {
      copyFLOAT(&pWindowSyn[0][timeSlots], &pWindowSyn[0][2 * timeSlots], timeSlots);
      copyFLOAT(pWindowSyn[0], &pWindowSyn[0][timeSlots], timeSlots);
      setFLOAT(0.f, pWindowSyn[0], timeSlots);
      copyFLOAT(&pWindowAna[0][timeSlots], &pWindowAna[0][2 * timeSlots], timeSlots);
      copyFLOAT(pWindowAna[0], &pWindowAna[0][timeSlots], timeSlots);
      if (avoid_keep != 0) {
        setFLOAT(0.f, pWindowAna[0], timeSlots);
      } else {
        setFLOAT(1.f, pWindowAna[0], timeSlots);
      }
    }
  }

  return error;
}
