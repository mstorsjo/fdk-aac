
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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "iisxHEAACEncLib_syncframe.h"
#include "iisxHEAACEncLib_rapOnDemand.h"

#define IISXHEAACENCLIB_MAX_SYNC_FRAME_FORCED_SYNC 20
#define IISXHEAACENCLIB_MAX_SYNC_FRAME_INTERVALS 20

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct xheaacenclib_syncinterval {
  int intervalLength;
  int startInFrames;
  int offset;
  int bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];

} XHEAACENCLIB_SYNCINTERVAL, *XHEAACENCLIB_SYNCINTERVAL_HANDLE;

struct xheaacenclib_forcedsync {
  struct xheaacenclib_forcedsync *nextForcedSync;
  int forceInFrames;
  int bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];
};

typedef struct xheaacenclib_forcedsync XHEAACENCLIB_FORCEDSYNC, *XHEAACENCLIB_FORCEDSYNC_HANDLE;

typedef struct xheaacenclib_syncframe_struct {
  int frameCounter;
  int frameCounterModulo;
  int startupPhaseLeft[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];
  int delay[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];
  int intervalAfterLastSyncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];
  int maxNumberOfIntervals;
  XHEAACENCLIB_SYNCINTERVAL_HANDLE interval;
  int nInterval;
  int maxNumberOfForcedSync;
  XHEAACENCLIB_FORCEDSYNC_HANDLE forcedSync;
  XHEAACENCLIB_FORCEDSYNC_HANDLE firstForcedSync;
  int lastSyncFrame[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS];

} XHEAACENCLIB_SYNCFRAME;

typedef struct {
  int maxNumberOfIntervals;
  int defaultStartupPhase;
  int defaultIntervalAfterLastSyncFrame;
  int maxNumberOfForcedSync;

} XHEAACENCLIB_SYNCFRAME_CONFIG;

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_leastCommonMultiple(int value1, int value2, int *result);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_greatestCommonDivisor(int value1, int value2, int *result);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_Update(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, XHEAACENCLIB_SYNCFRAME_CONFIG const *const syncConfig);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetNewSyncInterval(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const startFrame, int const interval, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetStartupPhase(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setStartupLength, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetDelay(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setDelayLength, XHEAACENCLIB_SYNCFRAME_TYPE const syncType);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetIntervalAfterLastSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setIntervalAfterLastSyncFrameLength, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]);

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_leastCommonMultiple(int value1, int value2, int *result) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int greatestCommonDivisor = 0;

  if (result == NULL || value1 < 0 || value2 < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_greatestCommonDivisor(value1, value2, &greatestCommonDivisor);
  }

  if (!isError(retValue)) {
    *result = value1 * value2 / greatestCommonDivisor;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_greatestCommonDivisor(int value1, int value2, int *result) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  if (!isError(retValue)) {
    if (result == NULL || value1 < 0 || value2 < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
    }
  }

  if (!isError(retValue)) {
    if (value1 < value2) {
      retValue = iisxHEAACEncLibSyncFrame_greatestCommonDivisor(value2, value1, result);
    } else if (value2 == 0) {
      *result = value1;
    } else {
      retValue = iisxHEAACEncLibSyncFrame_greatestCommonDivisor(value2, value1 % value2, result);
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_Update(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, XHEAACENCLIB_SYNCFRAME_CONFIG const *const syncConfig) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || syncConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if (syncConfig->defaultIntervalAfterLastSyncFrame < 0 ||
        syncConfig->maxNumberOfForcedSync < 0 ||
        syncConfig->maxNumberOfIntervals < 0 ||
        syncConfig->defaultStartupPhase < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_BD_CONFIG;
    }
  }

  if (!isError(retValue)) {
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      hSyncFrame->startupPhaseLeft[i] = syncConfig->defaultStartupPhase;
      hSyncFrame->delay[i] = 0;
      hSyncFrame->intervalAfterLastSyncFrame[i] = syncConfig->defaultIntervalAfterLastSyncFrame;
    }
    hSyncFrame->nInterval = 0;
    hSyncFrame->frameCounter = -1;
    hSyncFrame->frameCounterModulo = 1;
    memset(hSyncFrame->lastSyncFrame, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
    if (syncConfig->maxNumberOfIntervals != hSyncFrame->maxNumberOfIntervals) {
      hSyncFrame->maxNumberOfIntervals = syncConfig->maxNumberOfIntervals;
      if (hSyncFrame->interval != NULL) {
        iisFree(hSyncFrame->interval);
        hSyncFrame->interval = NULL;
      }
      if (hSyncFrame->maxNumberOfIntervals > 0) {
        hSyncFrame->interval = (XHEAACENCLIB_SYNCINTERVAL_HANDLE)iisMalloc(sizeof(struct xheaacenclib_syncinterval) * hSyncFrame->maxNumberOfIntervals);
        if (hSyncFrame->interval == NULL) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY;
        }
      }
    }
  }
  if (!isError(retValue)) {
    if (hSyncFrame->interval != NULL) {
      memset(hSyncFrame->interval, 0, sizeof(struct xheaacenclib_syncinterval) * hSyncFrame->maxNumberOfIntervals);
    }
  }
  if (!isError(retValue)) {
    if (syncConfig->maxNumberOfForcedSync != hSyncFrame->maxNumberOfForcedSync) {
      hSyncFrame->maxNumberOfForcedSync = syncConfig->maxNumberOfForcedSync;
      if (hSyncFrame->forcedSync != NULL) {
        iisFree(hSyncFrame->forcedSync);
        hSyncFrame->forcedSync = NULL;
      }
      if (hSyncFrame->maxNumberOfForcedSync > 0) {
        hSyncFrame->forcedSync = (XHEAACENCLIB_FORCEDSYNC_HANDLE)iisMalloc(sizeof(struct xheaacenclib_forcedsync) * hSyncFrame->maxNumberOfForcedSync);
        if (hSyncFrame->forcedSync == NULL) {
          retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY;
        }
      }
    }
  }
  if (!isError(retValue)) {
    if (hSyncFrame->forcedSync != NULL) {
      memset(hSyncFrame->forcedSync, 0, sizeof(struct xheaacenclib_forcedsync) * hSyncFrame->maxNumberOfForcedSync);
    }
    hSyncFrame->firstForcedSync = NULL;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetNewSyncInterval(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const startFrame, int const interval, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || startFrame < 0 || interval <= 0 || bSyncType == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    for (i = 0; !isError(retValue) && i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        if (startFrame <= hSyncFrame->delay[i]) {
          int multiply = 0;
          for (multiply = 0; !isError(retValue) && startFrame + multiply * interval <= hSyncFrame->delay[i]; multiply++) {
            int isSyncFrame = 0;
            retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hSyncFrame, startFrame + multiply * interval, &isSyncFrame, bSyncType[i]);
            if (!isError(retValue) && isSyncFrame == 0) {
              retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_START_FRAME_LEN_INVALID;
            }
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (hSyncFrame->nInterval >= hSyncFrame->maxNumberOfIntervals) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_MORE_INTERVALS_SPECIFIED;
    } else {
      hSyncFrame->interval[hSyncFrame->nInterval].intervalLength = interval;
      hSyncFrame->interval[hSyncFrame->nInterval].offset = 0;
      hSyncFrame->interval[hSyncFrame->nInterval].startInFrames = startFrame;
      memcpy(hSyncFrame->interval[hSyncFrame->nInterval].bSyncType, bSyncType, sizeof(hSyncFrame->interval[hSyncFrame->nInterval].bSyncType));
      retValue = iisxHEAACEncLibSyncFrame_leastCommonMultiple(hSyncFrame->frameCounterModulo, interval, &hSyncFrame->frameCounterModulo);
      hSyncFrame->nInterval++;
    }
  }
  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetStartupPhase(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setStartupLength, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || setStartupLength < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  } else if (hSyncFrame->frameCounter >= 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_STARTUP_LEN_CHNG;
  }

  if (!isError(retValue)) {
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        hSyncFrame->startupPhaseLeft[i] = setStartupLength;
      }
    }
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetDelay(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setDelayLength, XHEAACENCLIB_SYNCFRAME_TYPE const syncType) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSyncFrame == NULL || setDelayLength < 0 || (unsigned int)syncType >= (unsigned int)XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  } else if ((unsigned int)syncType > (unsigned int)XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    hSyncFrame->delay[syncType] = setDelayLength;
  }

  return retValue;
}

static XHEAACENCLIB_RETURN iisxHEAACEncLibSyncFrame_SetIntervalAfterLastSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const setIntervalAfterLastSyncFrameLength, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || setIntervalAfterLastSyncFrameLength < 0 || bSyncType == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        hSyncFrame->intervalAfterLastSyncFrame[i] = setIntervalAfterLastSyncFrameLength;
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_New(XHEAACENCLIB_SYNCFRAME_HANDLE *const phSyncFrame) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int nSize = 0;

  if (phSyncFrame == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    nSize += sizeof(XHEAACENCLIB_SYNCFRAME);
    *phSyncFrame = (XHEAACENCLIB_SYNCFRAME_HANDLE)iisMalloc(nSize);
    if (*phSyncFrame == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_MEMORY_ALLOCATION;
    }
  }
  if (!isError(retValue)) {
    memset(*phSyncFrame, 0, nSize);
  }
  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_Delete(XHEAACENCLIB_SYNCFRAME_HANDLE *const hSyncFrame) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;

  if (hSyncFrame == NULL || (*hSyncFrame) == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    if ((*hSyncFrame)->interval != NULL) {
      iisFree((*hSyncFrame)->interval);
      (*hSyncFrame)->interval = NULL;
    }
  }
  if (!isError(retValue)) {
    if ((*hSyncFrame)->forcedSync != NULL) {
      iisFree((*hSyncFrame)->forcedSync);
      (*hSyncFrame)->forcedSync = NULL;
    }
  }
  if (!isError(retValue)) {
    if (*hSyncFrame != NULL) {
      iisFree(*hSyncFrame);
      *hSyncFrame = NULL;
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_startNextFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    hSyncFrame->frameCounter++;
    hSyncFrame->frameCounter = hSyncFrame->frameCounter % hSyncFrame->frameCounterModulo;
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (hSyncFrame->intervalAfterLastSyncFrame[i] > 0 || hSyncFrame->lastSyncFrame[i] == 0) {
        hSyncFrame->lastSyncFrame[i]++;
      }
    }
  }

  if (!isError(retValue)) {
    XHEAACENCLIB_FORCEDSYNC_HANDLE actForcedSync = hSyncFrame->firstForcedSync;
    XHEAACENCLIB_FORCEDSYNC_HANDLE prevForcedSync = NULL;
    while (actForcedSync != NULL && !isError(retValue)) {
      actForcedSync->forceInFrames--;
      if (actForcedSync->forceInFrames == 0) {
        for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
          if (actForcedSync->bSyncType[i]) {
            hSyncFrame->lastSyncFrame[i] = 0;
          }
        }
        if (prevForcedSync == NULL && actForcedSync == hSyncFrame->firstForcedSync) {
          hSyncFrame->firstForcedSync = actForcedSync->nextForcedSync;
        } else if (prevForcedSync != NULL) {
          prevForcedSync->nextForcedSync = actForcedSync->nextForcedSync;
        } else {
          retValue = XHEAACENCLIB_RETURN_ERROR_UNKNOWN;
        }
      } else if (actForcedSync->forceInFrames < 0) {
        retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_NO_FORCED_FRAMES;
      }
      if (actForcedSync->forceInFrames != 0) {
        prevForcedSync = actForcedSync;
      }
      actForcedSync = actForcedSync->nextForcedSync;
    }
  }
  if (!isError(retValue)) {
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (hSyncFrame->startupPhaseLeft[i] > 0) {
        hSyncFrame->startupPhaseLeft[i]--;
        hSyncFrame->lastSyncFrame[i] = 0;
      }
    }
  }
  if (!isError(retValue)) {
    for (i = 0; i < hSyncFrame->nInterval; i++) {
      int isSyncFrame = 0;
      if (hSyncFrame->interval[i].startInFrames > 0) {
        hSyncFrame->interval[i].startInFrames--;
        if (hSyncFrame->interval[i].startInFrames <= 0) {
          isSyncFrame = 1;
          hSyncFrame->interval[i].offset = hSyncFrame->frameCounter % hSyncFrame->interval[i].intervalLength;
        }
      } else {
        if (hSyncFrame->interval[i].offset == hSyncFrame->frameCounter % hSyncFrame->interval[i].intervalLength) {
          isSyncFrame = 1;
        }
      }
      if (isSyncFrame && !isError(retValue)) {
        int j = 0;
        for (j = 0; j < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; j++) {
          if (hSyncFrame->interval[i].bSyncType[j]) {
            hSyncFrame->lastSyncFrame[j] = 0;
          }
        }
      }
    }
  }
  if (!isError(retValue)) {
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (hSyncFrame->intervalAfterLastSyncFrame[i] > 0) {
        if (hSyncFrame->intervalAfterLastSyncFrame[i] <= hSyncFrame->lastSyncFrame[i]) {
          hSyncFrame->lastSyncFrame[i] = 0;
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_ForceSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const offset, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  XHEAACENCLIB_FORCEDSYNC_HANDLE newForcedSyncHandle = NULL;
  int i = 0;

  if (hSyncFrame == NULL || bSyncType == NULL || offset < 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  } else if (hSyncFrame->maxNumberOfForcedSync <= 0) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_NO_FORCED_FRAMES;
  } else if (hSyncFrame->forcedSync == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY;
  }

  if (!isError(retValue)) {
    for (i = 0; !isError(retValue) && i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        if (hSyncFrame->delay[i] > offset) {
          int tmp_syncFrame = 0;

          retValue = iisxHEAACEncLib_syncFrame_IsSyncFrame(hSyncFrame, offset, &tmp_syncFrame, i);
          if (!isError(retValue)) {
            if (!tmp_syncFrame) {
              retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_EARLIER_SYNC_DELAY;
            }
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    for (i = hSyncFrame->maxNumberOfForcedSync - 1; i >= 0 && newForcedSyncHandle == NULL; i--) {
      if (hSyncFrame->forcedSync[i].forceInFrames <= 0) {
        newForcedSyncHandle = &hSyncFrame->forcedSync[i];
      }
    }

    if (newForcedSyncHandle == NULL) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME_MEMORY;
    }
  }

  if (!isError(retValue)) {
    newForcedSyncHandle->forceInFrames = offset;
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        newForcedSyncHandle->bSyncType[i] = 1;
      } else {
        newForcedSyncHandle->bSyncType[i] = 0;
      }
    }
  }

  if (!isError(retValue)) {
    if (hSyncFrame->firstForcedSync == newForcedSyncHandle || hSyncFrame->firstForcedSync == NULL || hSyncFrame->firstForcedSync->forceInFrames <= 0) {
      newForcedSyncHandle->nextForcedSync = NULL;
    } else {
      newForcedSyncHandle->nextForcedSync = hSyncFrame->firstForcedSync;
    }
    hSyncFrame->firstForcedSync = newForcedSyncHandle;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_GetDelay(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int *const getDelay, int const bSyncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS]) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || getDelay == NULL || bSyncType == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    *getDelay = 0;
    for (i = 0; i < XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS; i++) {
      if (bSyncType[i]) {
        if (hSyncFrame->delay[i] > *getDelay) {
          *getDelay = hSyncFrame->delay[i];
        }
      }
    }
  }
  if (isError(retValue) && getDelay != NULL) {
    *getDelay = -1;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_GetNextSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int *const nextSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE const syncType) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || nextSyncFrame == NULL || (unsigned int)syncType >= (unsigned int)XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    *nextSyncFrame = INT_MAX;
  }

  if (!isError(retValue)) {
    if (hSyncFrame->intervalAfterLastSyncFrame[syncType] > 0) {
      *nextSyncFrame = hSyncFrame->intervalAfterLastSyncFrame[syncType] - hSyncFrame->lastSyncFrame[syncType];
    }

    if (hSyncFrame->lastSyncFrame[syncType] == 0) {
      *nextSyncFrame = 0;
    }

    {
      XHEAACENCLIB_FORCEDSYNC_HANDLE tmp_forcedSync = hSyncFrame->firstForcedSync;
      while (tmp_forcedSync != NULL) {
        if (tmp_forcedSync->bSyncType[syncType] && tmp_forcedSync->forceInFrames < *nextSyncFrame) {
          *nextSyncFrame = tmp_forcedSync->forceInFrames;
        }
        tmp_forcedSync = tmp_forcedSync->nextForcedSync;
      }
    }
  }

  if (!isError(retValue)) {
    for (i = 0; i < hSyncFrame->nInterval; i++) {
      if (hSyncFrame->interval[i].bSyncType[syncType]) {
        if (hSyncFrame->interval[i].startInFrames > 0) {
          if (hSyncFrame->interval[i].startInFrames < *nextSyncFrame) {
            *nextSyncFrame = hSyncFrame->interval[i].startInFrames;
          }
        } else {
          int tmp_nextSyncFrame = hSyncFrame->interval[i].intervalLength - ((hSyncFrame->frameCounter - hSyncFrame->interval[i].offset + hSyncFrame->interval[i].intervalLength) % hSyncFrame->interval[i].intervalLength);
          if (tmp_nextSyncFrame < *nextSyncFrame) {
            *nextSyncFrame = tmp_nextSyncFrame;
          }
        }
      }
    }
  }

  if (!isError(retValue)) {
    if (*nextSyncFrame < 0) {
      retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_FRAME;
    }
  }

  if (isError(retValue) && nextSyncFrame != NULL) {
    *nextSyncFrame = -1;
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncFrame_IsSyncFrame(XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame, int const offset, int *const isSyncFrame, XHEAACENCLIB_SYNCFRAME_TYPE const syncType) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  int i = 0;

  if (hSyncFrame == NULL || offset < 0 || isSyncFrame == NULL || (unsigned int)syncType >= (unsigned int)XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS) {
    retValue = XHEAACENCLIB_RETURN_ERROR_SYNC_INVALID_PARAMETER;
  }

  if (!isError(retValue)) {
    *isSyncFrame = 0;
  }

  if (!isError(retValue)) {
    if (offset == 0) {
      if (hSyncFrame->lastSyncFrame[syncType] == 0) {
        *isSyncFrame = 1;
      }
    } else {
      int lastSyncFrameBeforeOffset = offset + hSyncFrame->lastSyncFrame[syncType];

      XHEAACENCLIB_FORCEDSYNC_HANDLE tmp_forcedSync = hSyncFrame->firstForcedSync;
      while (tmp_forcedSync != NULL && !isError(retValue)) {
        if (tmp_forcedSync->bSyncType[syncType]) {
          if (tmp_forcedSync->forceInFrames == offset) {
            *isSyncFrame = 1;
          } else if (tmp_forcedSync->forceInFrames < offset) {
            if (offset - tmp_forcedSync->forceInFrames < lastSyncFrameBeforeOffset) {
              lastSyncFrameBeforeOffset = offset - tmp_forcedSync->forceInFrames;
            }
          }
        }
        tmp_forcedSync = tmp_forcedSync->nextForcedSync;
      }

      for (i = 0; i < hSyncFrame->nInterval; i++) {
        if (hSyncFrame->interval[i].bSyncType[syncType]) {
          if (hSyncFrame->interval[i].startInFrames > 0) {
            if (offset >= hSyncFrame->interval[i].startInFrames) {
              if ((offset >= hSyncFrame->interval[i].startInFrames) && (offset - hSyncFrame->interval[i].startInFrames) % hSyncFrame->interval[i].intervalLength == 0) {
                *isSyncFrame = 1;
              } else if ((offset - hSyncFrame->interval[i].startInFrames) % hSyncFrame->interval[i].intervalLength < lastSyncFrameBeforeOffset) {
                lastSyncFrameBeforeOffset = (offset - hSyncFrame->interval[i].startInFrames) % hSyncFrame->interval[i].intervalLength;
              }
            }
          } else {
            int tmpAfterOffsetIndep = (offset - hSyncFrame->interval[i].offset + hSyncFrame->frameCounter + hSyncFrame->interval[i].intervalLength) % hSyncFrame->interval[i].intervalLength;
            if (tmpAfterOffsetIndep == 0) {
              *isSyncFrame = 1;
            } else if (tmpAfterOffsetIndep < lastSyncFrameBeforeOffset) {
              lastSyncFrameBeforeOffset = tmpAfterOffsetIndep;
            }
          }
        }
      }

      if (hSyncFrame->startupPhaseLeft[syncType] > 0) {
        if (hSyncFrame->startupPhaseLeft[syncType] >= offset) {
          *isSyncFrame = 1;
        } else if (offset - hSyncFrame->startupPhaseLeft[syncType] < lastSyncFrameBeforeOffset) {
          lastSyncFrameBeforeOffset = offset - hSyncFrame->startupPhaseLeft[syncType];
        }
      }

      if (hSyncFrame->intervalAfterLastSyncFrame[syncType] > 0) {
        if (lastSyncFrameBeforeOffset % hSyncFrame->intervalAfterLastSyncFrame[syncType] == 0) {
          *isSyncFrame = 1;
        }
      }
    }
  }

  return retValue;
}

XHEAACENCLIB_RETURN iisxHEAACEncLib_syncframe_setup(
    XHEAACENCLIB_SYNCFRAME_HANDLE const hSyncFrame,
    XHEAACENCLIB_CONFIG_HANDLE const hConfig,
    XHEAACENCLIB_HANDLE_SBRENCODER const hSbrEnc,
    AUDIOPREROLLLIB_INSTANCE_HANDLE const hAudioPreRoll,
    MPEG4_DELAY const *const mpeg4DelayParameter,
    HANDLE_STREAM_FORMAT const hLoaswriter,
    int const nTrashAUs,
    int *const rapFrameInAdvance,
    int const usacIndepDelay) {
  XHEAACENCLIB_RETURN retValue = XHEAACENCLIB_RETURN_NO_ERROR;
  AUDIOPREROLLLIB_RETURN retValueApr = AUDIOPREROLLLIB_NO_ERROR;
  XHEAACENCLIB_SYNCFRAME_CONFIG independent_config = {0};
  XHEAACENCLIB_RAP_SYNC_INFO rap_sync_info;
  int syncType[XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS] = {0};
  int sendSbrHeaderDelay = 0;
  int i = 0;
  int nAUPreRoll = 0;
  int ipfDelay = 0;
  int sbrStartup = 0;

  if (hSyncFrame == NULL || hConfig == NULL) {
    retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_HANDLE;
  }

  if (!isError(retValue)) {
    independent_config.maxNumberOfForcedSync = IISXHEAACENCLIB_MAX_SYNC_FRAME_FORCED_SYNC;
    independent_config.maxNumberOfIntervals = IISXHEAACENCLIB_MAX_SYNC_FRAME_INTERVALS;
    independent_config.defaultStartupPhase = 0;
    independent_config.defaultIntervalAfterLastSyncFrame = 0;
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_Update(hSyncFrame, &independent_config);
  }

  if (!isError(retValue)) {
    if (hSbrEnc != NULL) {
      retValue = iisxHEAACEncLibSbrEncSendHeaderDelay(hSbrEnc, &sendSbrHeaderDelay);
      sendSbrHeaderDelay = max(0, sendSbrHeaderDelay);
    }
  }

  if (!isError(retValue)) {
    if (hAudioPreRoll != NULL) {
      unsigned int tmp_nAUPreRoll = 0;
      unsigned int tmp_ipfDelay = 0;
      retValueApr = iisAudioPreRollLibGetDelays(hAudioPreRoll, &tmp_ipfDelay);
      if (retValueApr != AUDIOPREROLLLIB_NO_ERROR) {
        retValue = XHEAACENCLIB_RETURN_ERROR_AUDIO_PREROLL_DELAY;
      }

      if (!isError(retValue)) {
        tmp_nAUPreRoll = iisAudioPreRollLibGetnPreRollAu(hAudioPreRoll);
      }

      if (!isError(retValue)) {
        nAUPreRoll = (int)tmp_nAUPreRoll;
        ipfDelay = (int)tmp_ipfDelay;
      }
    } else {
      nAUPreRoll = 0;
      ipfDelay = INT_MAX;
    }

    if (hConfig->drcMode != XHEAACENCLIB_DRCMODE_OFF) {
      if (ipfDelay < 2) {
        ipfDelay = 2;
      }
    }
  }

  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC) {
      memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
      syncType[XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP] = 1;
      if (!isError(retValue)) {
        int usacIndepFlagIntervalFrames = hConfig->usacIndepFlagIntervalSamples / hConfig->nFrameSamples;
        retValue = iisxHEAACEncLibSyncFrame_SetIntervalAfterLastSyncFrame(hSyncFrame, usacIndepFlagIntervalFrames, syncType);
      }
      if (!isError(retValue)) {
        int indepStartup = 0;
        if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE) {
          indepStartup = nTrashAUs + 1;
        } else if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_STDDELAY) {
          indepStartup = nTrashAUs + ((mpeg4DelayParameter->mpeg4StandDelay + hConfig->nFrameSamples - 1) / hConfig->nFrameSamples);
        } else {
          indepStartup = (mpeg4DelayParameter->mpeg4Delay / hConfig->nFrameSamples) + 1;
        }
        retValue = iisxHEAACEncLibSyncFrame_SetStartupPhase(hSyncFrame, indepStartup, syncType);
      }
    }
  }

  if (!isError(retValue)) {
    if (hSbrEnc != NULL) {
      if (hConfig->aot != AUD_OBJ_TYP_USAC) {
        sbrStartup = max((mpeg4DelayParameter->mpeg4Delay + (int)hConfig->nFrameSamples) / (int)hConfig->nFrameSamples, nTrashAUs + 1);
        memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
        syncType[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER] = 1;
        retValue = iisxHEAACEncLibSyncFrame_SetStartupPhase(hSyncFrame, sbrStartup, syncType);
      }
    }
  }

  if (!isError(retValue)) {
    memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
    retValue = iisxHEAACEncLib_rapOnDemand_SetupRapSyncInfo(hConfig, hLoaswriter, hAudioPreRoll, &rap_sync_info);
  }

  if (!isError(retValue)) {
    if (hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_CONSTANT_INTERVAL) {
      if (hConfig->randomAccessIntervalInFrames <= 0) {
        retValue = XHEAACENCLIB_RETURN_ERROR_INVALID_RAP_INTERVAL;
      }
      for (i = 0; i < rap_sync_info.nSyncFrameType && !isError(retValue); i++) {
        int tmp_offset = nTrashAUs + 1 + rap_sync_info.frame_type[i].diffToSyncFrame;
        int tmp_delay = 0;
        retValue = iisxHEAACEncLib_syncFrame_GetDelay(hSyncFrame, &tmp_delay, rap_sync_info.frame_type[i].syncFrame);
        if (tmp_delay > tmp_offset) {
          tmp_offset += hConfig->randomAccessIntervalInFrames;
        }
        if (!isError(retValue)) {
          retValue = iisxHEAACEncLibSyncFrame_SetNewSyncInterval(hSyncFrame, tmp_offset, hConfig->randomAccessIntervalInFrames, rap_sync_info.frame_type[i].syncFrame);
        }
      }
    }
  }
  if (!isError(retValue)) {
    if (hLoaswriter != NULL) {
      memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
      syncType[XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC] = 1;
      syncType[XHEAACENCLIB_SYNCFRAME_TYPE_IS_RAP] = 1;
      if (hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE || hConfig->primingMode == XHEAACENC_PRIMINGMODE_STDDELAY) {
        retValue = iisxHEAACEncLib_syncFrame_ForceSyncFrame(hSyncFrame, nTrashAUs + 1, syncType);
      } else {
        retValue = iisxHEAACEncLib_syncFrame_ForceSyncFrame(hSyncFrame, 1, syncType);
      }
    }
  }
  if (!isError(retValue)) {
    if (hConfig->aot == AUD_OBJ_TYP_USAC &&
        hConfig->primingMode == XHEAACENC_PRIMINGMODE_NONE) {
      if (!isError(retValue)) {
        memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
        syncType[XHEAACENCLIB_SYNCFRAME_TYPE_IPF] = 1;
        retValue = iisxHEAACEncLib_syncFrame_ForceSyncFrame(hSyncFrame, nTrashAUs + 1, syncType);
      }

      if (!isError(retValue)) {
        memset(syncType, 0, sizeof(int) * XHEAACENCLIB_SYNCFRAME_TYPE_NUMBERS);
        syncType[XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER] = 1;
        for (i = 0; i <= nAUPreRoll; i++) {
          retValue = iisxHEAACEncLib_syncFrame_ForceSyncFrame(hSyncFrame, nTrashAUs + 1 - i, syncType);
        }
      }
    }
  }

  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, 1 + sendSbrHeaderDelay, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_SET_FIX_BORDER);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, sendSbrHeaderDelay, XHEAACENCLIB_SYNCFRAME_TYPE_SBR_HEADER);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, usacIndepDelay, XHEAACENCLIB_SYNCFRAME_TYPE_USAC_INDEP);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, 2, XHEAACENCLIB_SYNCFRAME_TYPE_CORE_LOW_OVERLAP);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, 1, XHEAACENCLIB_SYNCFRAME_TYPE_CORE_HIGH_BW);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, ipfDelay, XHEAACENCLIB_SYNCFRAME_TYPE_IPF);
  }
  if (!isError(retValue)) {
    retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, 0, XHEAACENCLIB_SYNCFRAME_TYPE_IS_RAP);
  }
  if (!isError(retValue)) {
    if (hLoaswriter != NULL && hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) {
      retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, 0, XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC);
    } else {
      retValue = iisxHEAACEncLibSyncFrame_SetDelay(hSyncFrame, INT_MAX, XHEAACENCLIB_SYNCFRAME_TYPE_LOAS_SMC);
    }
  }

  if (!isError(retValue)) {
    int tmp = 0;
    *rapFrameInAdvance = -1;
    if (hConfig->rapOccurrence == XHEAACENCLIB_RAP_OCCURRENCE_ON_DEMAND) {
      for (i = 0; i < rap_sync_info.nSyncFrameType; i++) {
        retValue = iisxHEAACEncLib_syncFrame_GetDelay(hSyncFrame, &tmp, rap_sync_info.frame_type[i].syncFrame);
        if (tmp - rap_sync_info.frame_type[i].diffToSyncFrame > *rapFrameInAdvance) {
          *rapFrameInAdvance = tmp - rap_sync_info.frame_type[i].diffToSyncFrame;
        }
      }
    }
  }
  return retValue;
}
