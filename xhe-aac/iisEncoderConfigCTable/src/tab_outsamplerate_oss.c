
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

#include "rel_outsamplerate.h"
#include "rel_insamplerate.h"

typedef struct tab_outsamplerate_oss_default_struct {
  PARAMLIST_KEYOUTSAMPLERATE keyoutsamplerate;
  PARAMLIST_KEYINSAMPLERATE keyinsamplerate;
  PARAMLIST_SAMPLERATE outsamplerate;

} TAB_OUTSAMPLERATE_OSS_DEFAULT;

static TAB_OUTSAMPLERATE_OSS_DEFAULT const tab_outsamplerate_oss_default[] = {

    {KEY_OUTSR_LST_44100_48000_D44100_D48000, KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_44100},
    {KEY_OUTSR_LST_44100_48000_D44100_D48000, KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_48000},

};

typedef struct tab_outsamplerate_oss_struct {
  PARAMLIST_KEYOUTSAMPLERATE keyoutsamplerate;
  PARAMLIST_KEYINSAMPLERATE keyinsamplerate;
  PARAMLIST_SAMPLERATE outsamplerate;

} TAB_OUTSAMPLERATE_OSS;

static TAB_OUTSAMPLERATE_OSS const tab_outsamplerate_oss[] = {

    {KEY_OUTSR_LST_44100_48000_D44100_D48000, KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_44100},
    {KEY_OUTSR_LST_44100_48000_D44100_D48000, KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_48000},

};

ENCCONFIGCTAB_RETURN_CODE tab_outsamplerate_ossCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key) {
  ENCCONFIGCTAB_RETURN_CODE retCode = ENCCONFIGCTAB_NO_ERROR;
  HANDLE_ERROR_INFO errorInfo = noError;

  PARAM_INSTANCE_HANDLE param_keyinsamplerate = NULL;
  PARAM_INSTANCE_HANDLE param_outsamplerate = NULL;

  PARAMLIST_SAMPLERATE outsamplerate = PARAMLIST_SAMPLERATE_INVALID;

  PARAMLIST_KEYINSAMPLERATE insamplerate = KEY_INSAMPLERATE_INVALID;

  int i, found = 0;
  int tableSize = sizeof(tab_outsamplerate_oss) / sizeof(TAB_OUTSAMPLERATE_OSS);
  int tableSizeDefault = sizeof(tab_outsamplerate_oss_default) / sizeof(TAB_OUTSAMPLERATE_OSS_DEFAULT);

  if (errorInfo == noError) {
    errorInfo = iisParamListGetParam(hInputParamList, PARAMLIST_PARAMETER_OUTSAMPLERATE, &param_outsamplerate);
    if (param_outsamplerate) outsamplerate = (PARAMLIST_SAMPLERATE)param_outsamplerate->paramValue._int;
  }

  if (errorInfo == noError) {
    errorInfo = iisParamListGetParam(hInputParamList, PARAMLIST_PARAMETER_INSAMPLERATE, &param_keyinsamplerate);
    if (param_keyinsamplerate) insamplerate = param_keyinsamplerate->paramValue._int;
  }

  AVOID_COMPILER_WARNING(outsamplerate);

  AVOID_COMPILER_WARNING(key);

  for (i = 0; i < tableSize; i++) {
    if (1 && ((int)(tab_outsamplerate_oss[i].keyoutsamplerate) == key) && ((int)(tab_outsamplerate_oss[i].outsamplerate) == (int)outsamplerate)

        && (tab_insamplerateIsValid(tab_outsamplerate_oss[i].keyinsamplerate, insamplerate))

    ) {
      PARAM_INSTANCE param;
      PARAM_INSTANCE_HANDLE param_advanced = NULL;

      int advanced = 0;
      AVOID_COMPILER_WARNING(param)
      AVOID_COMPILER_WARNING(param_advanced)
      AVOID_COMPILER_WARNING(advanced)

      iisParamListAddParam(hOutputParamList, param_outsamplerate, PARAMLIST_MODE_REPLACE);

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

  if ((errorInfo == noError) && (found == 0 && outsamplerate == PARAMLIST_SAMPLERATE_INVALID

                                 )) {
    found = 0;

    for (i = 0; i < tableSizeDefault; i++) {
      if (1 && ((int)(tab_outsamplerate_oss_default[i].keyoutsamplerate) == key)

          && (tab_insamplerateIsValid(tab_outsamplerate_oss_default[i].keyinsamplerate, insamplerate))

      ) {
        PARAM_INSTANCE param;

        param.paramFormat = PARAM_INT;
        param.paramLength = 1;
        param.paramTag = PARAMLIST_PARAMETER_OUTSAMPLERATE;
        param.paramValue._int = tab_outsamplerate_oss_default[i].outsamplerate;
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

int tab_outsamplerate_ossIsValid(
    int const key,
    int const value) {
  int i, retVal = 0;
  int tableSize = sizeof(tab_outsamplerate_oss) / sizeof(TAB_OUTSAMPLERATE_OSS);

  AVOID_COMPILER_WARNING(key);

  for (i = 0; i < tableSize; i++) {
    if (1 && ((int)(tab_outsamplerate_oss[i].keyoutsamplerate) == key) && ((int)(tab_outsamplerate_oss[i].outsamplerate) == value)

    ) {
      retVal = 1;
      break;
    }
  }

  return retVal;
}
