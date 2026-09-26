
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
#include <limits.h>

#include "iisutillib.h"
#include "options.h"
#include "iisLPDEncLib_arithEncWrapper.h"

#define PRIVATE_DATA_OFFSET_BYTES (sizeof(void *) - sizeof(struct LPDEnc_arithEncWrapper_public_data_struct) % sizeof(void *))

typedef struct LPDEnc_arithEncWrapper_private_data_struct {
  int size;
  ARIENC_PUBLIC_DATA_HANDLE hArithmeticEncoder;
} LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA;

static LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE getPrivateDataHandle(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData);

static LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE getPrivateDataHandle(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  hPrivateData = hPublicData->hPrivateData;

  return hPrivateData;
}

HANDLE_ERROR_INFO LPDEnc_arithEncWrapper_Open(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE *hPublicData) {
  int size = 0;

  size += sizeof(struct LPDEnc_arithEncWrapper_private_data_struct);
  size += sizeof(struct LPDEnc_arithEncWrapper_public_data_struct);

  *hPublicData = (LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE)iisCalloc(1, sizeof(LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA));

  if (*hPublicData == NULL) {
    return iisUtil_ERROR(CDI, "Memory allocation in function LPDEnc_arithEncWrapper_Open failed");
  }

  (*hPublicData)->hPrivateData = (LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE)iisCalloc(1, sizeof(LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA));
  if ((*hPublicData)->hPrivateData == NULL) {
    iisFree(*hPublicData);
    *hPublicData = NULL;
    return iisUtil_ERROR(CDI, "Memory allocation in function LPDEnc_arithEncWrapper_Open failed");
  }
  (*hPublicData)->hPrivateData->size = size;

  return noError;
}

LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE LPDEnc_arithEncWrapper_Close(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData) {
  if (hPublicData) {
    LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = getPrivateDataHandle(hPublicData);
    if (hPrivateData) {
      iisFree(hPrivateData);
    }
    iisFree(hPublicData);
  }
  return NULL;
}

void LPDEnc_arithEncWrapper_SetForceReset(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  hPrivateData = getPrivateDataHandle(hPublicData);

  hPrivateData->hArithmeticEncoder->forceArithResetOnNextCall = 1;
}

int LPDEnc_arithEncWrapper_GetForceReset(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  hPrivateData = getPrivateDataHandle(hPublicData);

  return hPrivateData->hArithmeticEncoder->forceArithResetOnNextCall;
}

int LPDEnc_arithEncWrapper_Encode(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData,
    unsigned char const **bitBuffer,
    int *quantSpectrum,
    int lg,
    int *arith_reset_flag,
    int N) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  int bitCount = 0;
  ARIENC_ERROR retError = ARIENC_OK;

  hPrivateData = getPrivateDataHandle(hPublicData);

  if (hPrivateData->hArithmeticEncoder->forceArithResetOnNextCall) {
    if (*arith_reset_flag == -1) {
      *arith_reset_flag = 1;
    } else if (*arith_reset_flag == 0) {
    }
  }

  retError = iisArithEncoderEncode(
      hPrivateData->hArithmeticEncoder,
      NULL,
      &bitCount,
      quantSpectrum,
      lg,
      arith_reset_flag,
      N);

  hPrivateData->hArithmeticEncoder->forceArithResetOnNextCall = 0;

  (void)retError;

  (void)retError;

  iisArithEncoderGetData(hPrivateData->hArithmeticEncoder,
                         bitBuffer, &bitCount);

  return bitCount;
}

void LPDEnc_arithEncWrapper_SetArithEncoderInstance(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData,
    ARIENC_PUBLIC_DATA_HANDLE const hArithmeticEncoder) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  hPrivateData = getPrivateDataHandle(hPublicData);
  hPrivateData->hArithmeticEncoder = hArithmeticEncoder;

  if (hArithmeticEncoder) {
    hPublicData->info_ArithEncoderIsLinked = 1;
  }
}

HANDLE_ERROR_INFO LPDEnc_arithEncWrapper_Copy(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData_src,
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE *hPublicData_dest) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData_src = NULL;
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData_dest = NULL;
  HANDLE_ERROR_INFO err = noError;
  if (hPublicData_src == NULL) {
    if (*hPublicData_dest != NULL) {
      *hPublicData_dest = LPDEnc_arithEncWrapper_Close(*hPublicData_dest);
    }
    return err;
  }

  hPrivateData_src = getPrivateDataHandle(hPublicData_src);

  if (*hPublicData_dest == NULL) {
    SAFECALL(err, LPDEnc_arithEncWrapper_Open(hPublicData_dest));
    if (err != noError) {
      return err;
    }
  }

  hPrivateData_dest = getPrivateDataHandle(*hPublicData_dest);

  (*hPublicData_dest)->info_ArithEncoderIsLinked = hPublicData_src->info_ArithEncoderIsLinked;

  SAFECALL(err, iisArithEncoderCopy(hPrivateData_src->hArithmeticEncoder, &(hPrivateData_dest->hArithmeticEncoder)));

  return err;
}

void LPDEnc_arithEncWrapper_CloseArithEnc(
    LPDENC_ARITH_ENC_WRAPPER_PUBLIC_DATA_HANDLE const hPublicData) {
  LPDENC_ARITH_ENC_WRAPPER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  if (hPublicData == NULL) {
    return;
  }

  hPrivateData = getPrivateDataHandle(hPublicData);

  hPrivateData->hArithmeticEncoder = iisArithEncoderClose(hPrivateData->hArithmeticEncoder);
}
