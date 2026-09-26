
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
#include <string.h>
#include "statClass_lpc.h"
#include "statClasslib.h"
#include "iisutillib.h"
#include "statClass_utils.h"

#define LPC_ORDER 16
#define WINDOW_LEN 480
#define WINDOW_L1 256
#define WINDOW_L2 224

#define FRAME_SIZE 256
#define SUBFRAME_SIZE 64
#define NBR_OF_SUBFRAMES (FRAME_SIZE / SUBFRAME_SIZE)

#define LPAST 112

typedef struct _statclass_lpc {
  SCFLOAT* autoCorrBuf;
  SCFLOAT* windowBuf;

  SCFLOAT* aIntLevinson;

  SCFLOAT* isp;
  SCFLOAT* memIsp;
  SCFLOAT memWsp;

} STATCLASS_LPC;

STATCLASS_ERROR_CODE STATCLASS_Lpc_Open(HANDLE_STATCLASS_LPC* hLpc) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (hLpc == NULL) {
    error = STATCLASS_INVALID_POINTER;
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == (*hLpc = (STATCLASS_LPC*)iisCalloc(1, sizeof(STATCLASS_LPC)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hLpc)->windowBuf = (SCFLOAT*)iisCalloc(WINDOW_LEN, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    STATCLASS_cos_window((*hLpc)->windowBuf, WINDOW_L1, WINDOW_L2);
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hLpc)->autoCorrBuf = (SCFLOAT*)iisCalloc(LPC_ORDER + 1, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hLpc)->memIsp = (SCFLOAT*)iisCalloc(LPC_ORDER, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    } else {
      memset((*hLpc)->memIsp, 0, LPC_ORDER);
    }
  }
  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hLpc)->isp = (SCFLOAT*)iisCalloc(LPC_ORDER, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  if (error == STATCLASS_NO_ERROR) {
    if (NULL == ((*hLpc)->aIntLevinson = (SCFLOAT*)iisCalloc((LPC_ORDER + 1) * NBR_OF_SUBFRAMES, sizeof(SCFLOAT)))) {
      error = STATCLASS_MEMORY_ALLOC_ERROR;
    }
  }

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Lpc_Advance(HANDLE_STATCLASS_LPC hLpc,
                                           SCFLOAT* pTotalSignal,
                                           SCFLOAT* pWsp,
                                           SCFLOAT* pLevinson,
                                           SCFLOAT* exc) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;
  int subFrameNbr;

  STATCLASS_autocorrPlus(pTotalSignal,
                         hLpc->autoCorrBuf,
                         LPC_ORDER,
                         WINDOW_LEN,
                         hLpc->windowBuf);

  STATCLASS_lag_wind(hLpc->autoCorrBuf,
                     LPC_ORDER);

  STATCLASS_lev_dur(pLevinson,
                    hLpc->autoCorrBuf,
                    LPC_ORDER);

  STATCLASS_a_isp_conversion(pLevinson,
                             hLpc->isp,
                             hLpc->memIsp,
                             LPC_ORDER);

  STATCLASS_int_lpc_np1(hLpc->memIsp,
                        hLpc->isp,
                        hLpc->aIntLevinson,
                        NBR_OF_SUBFRAMES,
                        LPC_ORDER);

  memcpy(hLpc->memIsp, hLpc->isp, LPC_ORDER * sizeof(SCFLOAT));

  for (subFrameNbr = 0; subFrameNbr < 4; subFrameNbr++) {
    STATCLASS_find_wsp(&hLpc->aIntLevinson[(LPC_ORDER + 1) * subFrameNbr],
                       &pTotalSignal[LPAST - LPC_ORDER + subFrameNbr * SUBFRAME_SIZE],
                       &pWsp[subFrameNbr * SUBFRAME_SIZE],
                       &hLpc->memWsp,
                       SUBFRAME_SIZE);

    STATCLASS_residu(&hLpc->aIntLevinson[(LPC_ORDER + 1) * subFrameNbr],
                     &pTotalSignal[LPAST - LPC_ORDER + subFrameNbr * SUBFRAME_SIZE],
                     &exc[subFrameNbr * SUBFRAME_SIZE],
                     SUBFRAME_SIZE);
  }

  STATCLASS_find_wsp(hLpc->aIntLevinson,
                     &pTotalSignal[LPAST - LPC_ORDER + subFrameNbr * SUBFRAME_SIZE],
                     &pWsp[NBR_OF_SUBFRAMES * SUBFRAME_SIZE],
                     &hLpc->memWsp,
                     8 * LPC_ORDER);

  return error;
}

STATCLASS_ERROR_CODE STATCLASS_Lpc_Close(HANDLE_STATCLASS_LPC* hLpc) {
  STATCLASS_ERROR_CODE error = STATCLASS_NO_ERROR;

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == ((*hLpc)->windowBuf)) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hLpc)->windowBuf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hLpc)->autoCorrBuf) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hLpc)->autoCorrBuf);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hLpc)->memIsp) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hLpc)->memIsp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hLpc)->isp) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hLpc)->isp);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == (*hLpc)->aIntLevinson) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree((*hLpc)->aIntLevinson);
    }
  }

  if (STATCLASS_NO_ERROR == error) {
    if (NULL == *hLpc) {
      error = STATCLASS_INVALID_POINTER;
    } else {
      iisFree(*hLpc);
    }
  }

  return error;
}
