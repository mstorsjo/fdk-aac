
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

#define ENCCONFIGCTAB_MODULE_NAME "iisEncoderConfigCTable"

#define ENCCONFIGCTAB_MAX_VALID_CONFIGS 1024 * 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iisutillib.h"
#include "iisEncoderConfigCTable.h"

#include "iisParamList.h"

typedef struct encconfigctab_private_data_struct {
  int nSize;
} ENCCONFIGCTAB_PRIVATE_DATA, *ENCCONFIGCTAB_PRIVATE_DATA_HANDLE;

typedef HANDLE_ERROR_INFO (*ENCCONFIGCTAB_TABLE_CALLBACK)(void *ptr,
                                                          PARAMLIST_INSTANCE_HANDLE const hInputParamList,
                                                          PARAMLIST_INSTANCE_HANDLE const hOutputParamList);

static ENCCONFIGCTAB_PRIVATE_DATA_HANDLE iisEncoderConfigCTableGetPrivateDataHandle(
    ENCCONFIGCTAB_INSTANCE_HANDLE const hInstance);

static ENCCONFIGCTAB_PRIVATE_DATA_HANDLE iisEncoderConfigCTableGetPrivateDataHandle(
    ENCCONFIGCTAB_INSTANCE_HANDLE const hInstance) {
  ENCCONFIGCTAB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (ENCCONFIGCTAB_PRIVATE_DATA_HANDLE)((char *)hInstance + sizeof(struct encconfigctab_instance_struct));
  return hPrivateData;
}

HANDLE_ERROR_INFO iisEncoderConfigCTableNew(
    ENCCONFIGCTAB_INSTANCE_HANDLE *phInstance) {
  HANDLE_ERROR_INFO retError = noError;
  int nSize = 0;
  ENCCONFIGCTAB_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  nSize += sizeof(struct encconfigctab_instance_struct);
  nSize += sizeof(struct encconfigctab_private_data_struct);

  *phInstance = (ENCCONFIGCTAB_INSTANCE_HANDLE)iisMalloc(nSize);

  if (*phInstance != NULL) {
    memset(*phInstance, 0, nSize);
    hPrivateData = (ENCCONFIGCTAB_PRIVATE_DATA_HANDLE)((char *)(*phInstance) + sizeof(struct encconfigctab_instance_struct));
    hPrivateData->nSize = nSize;

    sprintf((*phInstance)->infoModuleVersion, "%s", ENCCONFIGCTAB_MODULE_NAME);
  } else {
    retError = iisUtil_ERROR(CDI, "Memory allocation in function iisEncoderConfigCTableOpen failed");
  }
  return retError;
}

ENCCONFIGCTAB_RETURN_CODE iisEncoderConfigCTableUpdate(
    ENCCONFIGCTAB_INSTANCE_HANDLE hInstance,
    PARAMLIST_INSTANCE_HANDLE hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList) {
  ENCCONFIGCTAB_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  ENCCONFIGCTAB_RETURN_CODE retCode = ENCCONFIGCTAB_NO_ERROR;
  if (hInstance) {
    hPrivateData = iisEncoderConfigCTableGetPrivateDataHandle(hInstance);
    (void)(hPrivateData);
    if (retCode != ENCCONFIGCTAB_INFO_CONFIG_FOUND) {
      retCode = main_aot_42_dash_ossCheck(hInputParamList, hOutputParamList, 0);
    }
  }

  return retCode;
}

HANDLE_ERROR_INFO iisEncoderConfigCTableDelete(
    ENCCONFIGCTAB_INSTANCE_HANDLE hInstance) {
  HANDLE_ERROR_INFO retError = noError;

  if (hInstance != NULL) {
    iisFree(hInstance);
  } else {
    retError = iisUtil_ERROR(CDI, "Deleting instance in function iisEncoderConfigCTableDelete failed");
  }
  return retError;
}
