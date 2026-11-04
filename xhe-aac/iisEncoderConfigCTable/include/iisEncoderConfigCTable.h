
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

#ifndef IIS_ENCODERCONFIGCTABLE_
#define IIS_ENCODERCONFIGCTABLE_

#include "iisParamList.h"

#define ENCCONFIGCTAB_INFO_FIRST 0x00000010
#define ENCCONFIGCTAB_WARNING_FIRST 0x00001000
#define ENCCONFIGCTAB_ERROR_FIRST 0x00100000

typedef enum {
  ENCCONFIGCTAB_NO_ERROR = 0,

  ENCCONFIGCTAB_INFO_CONFIG_FOUND = ENCCONFIGCTAB_INFO_FIRST,
  ENCCONFIGCTAB_INFO_NO_CONFIG_FOUND,
  ENCCONFIGCTAB_INFO_RESERVED_2,

  ENCCONFIGCTAB_WARNING_RESERVED_1 = ENCCONFIGCTAB_WARNING_FIRST,

  ENCCONFIGCTAB_ERROR_MEMORY = ENCCONFIGCTAB_ERROR_FIRST,
  ENCCONFIGCTAB_ERROR_NO_CONFIG_FOUND,
  ENCCONFIGCTAB_ERROR_INITIALIZATION,
  ENCCONFIGCTAB_ERROR_INVALID_HANDLE,
  ENCCONFIGCTAB_ERROR_UNKOWN
} ENCCONFIGCTAB_RETURN_CODE;

typedef struct encconfigctab_instance_struct {
  char infoModuleVersion[32];
} ENCCONFIGCTAB_INSTANCE, *ENCCONFIGCTAB_INSTANCE_HANDLE;

HANDLE_ERROR_INFO iisEncoderConfigCTableNew(
    ENCCONFIGCTAB_INSTANCE_HANDLE *phInstance);

ENCCONFIGCTAB_RETURN_CODE iisEncoderConfigCTableUpdate(
    ENCCONFIGCTAB_INSTANCE_HANDLE hInstance,
    PARAMLIST_INSTANCE_HANDLE hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList);

HANDLE_ERROR_INFO iisEncoderConfigCTableDelete(
    ENCCONFIGCTAB_INSTANCE_HANDLE hInstance);

ENCCONFIGCTAB_RETURN_CODE tab_coremodeCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_coremodeIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_hbeCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_hbeIsValid(
    int const key,
    int const value);

int tab_insamplerateIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_outsamplerate_ossCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_outsamplerate_ossIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_qualityCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_qualityIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_sbrpvcCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_sbrpvcIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_sbrratioCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_sbrratioIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_sbrsignalingCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_sbrsignalingIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_stereoconfigidxCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_stereoconfigidxIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE tab_tsdCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int tab_tsdIsValid(
    int const key,
    int const value);

ENCCONFIGCTAB_RETURN_CODE main_aot_42_dash_ossCheck(
    PARAMLIST_INSTANCE_HANDLE const hInputParamList,
    PARAMLIST_INSTANCE_HANDLE hOutputParamList,
    int const key);

int main_aot_42_dash_ossIsValid(
    int const key,
    int const value);

#endif
