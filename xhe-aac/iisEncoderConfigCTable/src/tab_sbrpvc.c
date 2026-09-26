
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

#include "iisEncoderConfigCTable.h"

#define AVOID_COMPILER_WARNING(expr) \
  do {                               \
    (void)(expr);                    \
  } while (0);

#include "rel_sbrpvc.h"

typedef struct tab_sbrpvc_default_struct {
  PARAMLIST_KEYSBRPVC keysbrpvc;
  PARAMLIST_SBRPVC sbrpvc;

} TAB_SBRPVC_DEFAULT;

static TAB_SBRPVC_DEFAULT const tab_sbrpvc_default[] = {

    {KEY_SBRPVC_OFF, PARAMLIST_SBRPVC_OFF},
    {KEY_SBRPVC_ON, PARAMLIST_SBRPVC_ON},
    {KEY_SBRPVC_ONOFF_OFF, PARAMLIST_SBRPVC_OFF},
    {KEY_SBRPVC_ONOFF_ON, PARAMLIST_SBRPVC_ON},

};

typedef struct tab_sbrpvc_struct {
  PARAMLIST_KEYSBRPVC keysbrpvc;
  PARAMLIST_SBRPVC sbrpvc;

} TAB_SBRPVC;

static TAB_SBRPVC const tab_sbrpvc[] = {

    {KEY_SBRPVC_OFF, PARAMLIST_SBRPVC_OFF},
    {KEY_SBRPVC_ON, PARAMLIST_SBRPVC_ON},
    {KEY_SBRPVC_ONOFF_OFF, PARAMLIST_SBRPVC_ON},
    {KEY_SBRPVC_ONOFF_OFF, PARAMLIST_SBRPVC_OFF},
    {KEY_SBRPVC_ONOFF_ON, PARAMLIST_SBRPVC_ON},
    {KEY_SBRPVC_ONOFF_ON, PARAMLIST_SBRPVC_OFF},

};

ENCCONFIGCTAB_RETURN_CODE tab_sbrpvcCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key) {
  ENCCONFIGCTAB_RETURN_CODE retCode = ENCCONFIGCTAB_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  PARAM_INSTANCE_HANDLE param_sbrpvc = NULL;

  PARAMLIST_SBRPVC sbrpvc = PARAMLIST_SBRPVC_INVALID;

  int i, found = 0;
  int tableSize = sizeof(tab_sbrpvc) / sizeof(TAB_SBRPVC);
  int tableSizeDefault = sizeof(tab_sbrpvc_default) / sizeof(TAB_SBRPVC_DEFAULT);

  if (errorInfo == noError) {
    errorInfo = iisParamListGetParam(hInputParamList, PARAMLIST_PARAMETER_SBRPVC, &param_sbrpvc);
    if (param_sbrpvc) sbrpvc = (PARAMLIST_SBRPVC)param_sbrpvc->paramValue._int;
  }

  AVOID_COMPILER_WARNING(sbrpvc);

  AVOID_COMPILER_WARNING(key);

  for (i = 0; i < tableSize; i++) {
    if (1 && ((int)(tab_sbrpvc[i].keysbrpvc) == key) && ((int)(tab_sbrpvc[i].sbrpvc) == (int)sbrpvc)

    ) {
      PARAM_INSTANCE param;
      PARAM_INSTANCE_HANDLE param_advanced = NULL;

      int advanced = 0;
      AVOID_COMPILER_WARNING(param)
      AVOID_COMPILER_WARNING(param_advanced)
      AVOID_COMPILER_WARNING(advanced)

      iisParamListAddParam(hOutputParamList, param_sbrpvc, PARAMLIST_MODE_REPLACE);

      if (retCode >= ENCCONFIGCTAB_ERROR_FIRST) {
        break;
      }

      if (errorInfo == noError) {
        errorInfo = iisParamListGetParam(hInputParamList, PARAMLIST_PARAMETER_ADVANCED, &param_advanced);
        if (param_advanced) advanced = param_advanced->paramValue._int;

        if (advanced > 0) {
          PARAM_INSTANCE_HANDLE paramAdd = NULL;
          int add = 0;
          AVOID_COMPILER_WARNING(paramAdd)
          AVOID_COMPILER_WARNING(add)
        }
      }

      if (errorInfo != noError) break;

      found++;
    }
  }

  if ((errorInfo == noError) && (found == 0 && sbrpvc == PARAMLIST_SBRPVC_INVALID

                                 )) {
    found = 0;

    for (i = 0; i < tableSizeDefault; i++) {
      if (1 && ((int)(tab_sbrpvc_default[i].keysbrpvc) == key)

      ) {
        PARAM_INSTANCE param;

        param.paramFormat = PARAM_INT;
        param.paramLength = 1;
        param.paramTag = PARAMLIST_PARAMETER_SBRPVC;
        param.paramValue._int = tab_sbrpvc_default[i].sbrpvc;
        iisParamListAddParam(hOutputParamList, &param, PARAMLIST_MODE_REPLACE);

        found++;
        break;
      }
    }
  }

  if (retCode < ENCCONFIGCTAB_ERROR_FIRST) {
    if (errorInfo == noError) {
      if (found == 0) {
        retCode = ENCCONFIGCTAB_ERROR_NO_CONFIG_FOUND;
      } else if (found > 1) {
        retCode = ENCCONFIGCTAB_ERROR_UNKOWN;
      } else {
        retCode = ENCCONFIGCTAB_INFO_CONFIG_FOUND;
      }
    } else {
      retCode = ENCCONFIGCTAB_ERROR_UNKOWN;
    }
  }

  return retCode;
}

int tab_sbrpvcIsValid(
    int const key,
    int const value) {
  int i, retVal = 0;
  int tableSize = sizeof(tab_sbrpvc) / sizeof(TAB_SBRPVC);

  AVOID_COMPILER_WARNING(key);

  for (i = 0; i < tableSize; i++) {
    if (1 && ((int)(tab_sbrpvc[i].keysbrpvc) == key) && ((int)(tab_sbrpvc[i].sbrpvc) == value)

    ) {
      retVal = 1;
      break;
    }
  }

  return retVal;
}
