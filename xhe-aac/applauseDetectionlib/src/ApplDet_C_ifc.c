
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

#include "iisutillib.h"
#include "ApplDet_C_ifc.h"
#include "cSpDtct.h"

typedef struct _appl_det_ifc_c {
  cSpDtct* hSpDtct;
  APPLDET_CONFIG config;
  float* inBuffer;
} APPL_DET_IFC;

AD_ERROR_CODE APPLDET_C_IFC_Open(HANDLE_APPLDET* hApplDet,
                                 APPLDET_CONFIG const* const pConfig) {
  AD_ERROR_CODE err = AD_NO_ERROR;

  *hApplDet = (struct _appl_det_ifc_c*)iisCalloc(1, sizeof(struct _appl_det_ifc_c));
  if (*hApplDet == NULL) {
    err = AD_MEMORY_ERROR;
  } else {
    (*hApplDet)->inBuffer = NULL;
  }

  if (NULL == pConfig) {
    err = AD_INVALID_POINTER;
  }

  if (AD_NO_ERROR == err) {
    if ((*hApplDet)) {
      (*hApplDet)->config = *pConfig;
    } else {
      err = AD_INVALID_POINTER;
    }
  }
  if (AD_NO_ERROR == err) {
    (*hApplDet)->inBuffer = (float*)iisCalloc((*hApplDet)->config.frameSize, sizeof(float));
    if ((*hApplDet)->inBuffer == NULL) {
      err = AD_MEMORY_ERROR;
    }
  }

  if (AD_NO_ERROR == err) {
    if (cSpDtct_open(&(*hApplDet)->hSpDtct, (*hApplDet)->config.frameSize, (*hApplDet)->config.fs))
      err = AD_MEMORY_ERROR;
  }

  return err;
}

AD_ERROR_CODE APPLDET_C_IFC_Close(HANDLE_APPLDET* hApplDet) {
  AD_ERROR_CODE err = AD_NO_ERROR;

  if (*hApplDet == NULL) {
    err = AD_INVALID_POINTER;
  }

  if (AD_NO_ERROR == err) {
    if ((*hApplDet)->hSpDtct) {
      cSpDtct_close(&(*hApplDet)->hSpDtct);
    }

    if ((*hApplDet)->inBuffer) {
      iisFree((*hApplDet)->inBuffer);
    }
  }

  if (AD_NO_ERROR == err) {
    iisFree(*hApplDet);
    *hApplDet = NULL;
  }

  return err;
}

AD_ERROR_CODE APPLDET_C_IFC_Advance(HANDLE_APPLDET hApplDet,
                                    float const* input,
                                    unsigned short int* const verdict,
                                    float* const conf) {
  AD_ERROR_CODE err = AD_NO_ERROR;
  int overlap = hApplDet->config.frameSize / 2;

  memcpy(hApplDet->inBuffer, &hApplDet->inBuffer[overlap], overlap * sizeof(float));
  memcpy(&hApplDet->inBuffer[overlap], input, overlap * sizeof(float));

  if (hApplDet) {
    cSpDtct_main((hApplDet)->hSpDtct, hApplDet->inBuffer, verdict, conf);
  } else {
    err = AD_INVALID_POINTER;
  }
  return err;
}
