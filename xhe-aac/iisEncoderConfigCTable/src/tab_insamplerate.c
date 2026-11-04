
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

#include "rel_insamplerate.h"

typedef struct tab_insamplerate_default_struct {
  PARAMLIST_KEYINSAMPLERATE keyinsamplerate;
  PARAMLIST_SAMPLERATE insamplerate;

} TAB_INSAMPLERATE_DEFAULT;

typedef struct tab_insamplerate_struct {
  PARAMLIST_KEYINSAMPLERATE keyinsamplerate;
  PARAMLIST_SAMPLERATE insamplerate;

} TAB_INSAMPLERATE;

static TAB_INSAMPLERATE const tab_insamplerate[] = {

    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_7350},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_8820},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_11025},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_11760},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_14700},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_17640},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_22050},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_29400},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_35280},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_44100},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_58800},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_70560},
    {KEY_INSAMPLERATE_44100_LINE, PARAMLIST_SAMPLERATE_88200},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_8000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_9600},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_12000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_12800},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_16000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_19200},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_24000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_32000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_38400},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_48000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_64000},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_76800},
    {KEY_INSAMPLERATE_48000_LINE, PARAMLIST_SAMPLERATE_96000},
    {KEY_INSAMPLERATE_88200, PARAMLIST_SAMPLERATE_88200},
    {KEY_INSAMPLERATE_96000, PARAMLIST_SAMPLERATE_96000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_7350},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_8000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_8820},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_9600},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_11025},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_11760},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_12000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_12800},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_14700},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_16000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_17640},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_19200},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_22050},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_24000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_29400},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_32000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_35280},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_38400},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_44100},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_48000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_58800},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_64000},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_70560},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_76800},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_88200},
    {KEY_INSAMPLERATE_ALL, PARAMLIST_SAMPLERATE_96000},
    {KEY_INSAMPLERATE_INVALID, PARAMLIST_SAMPLERATE_INVALID},

};

int tab_insamplerateIsValid(
    int const key,
    int const value) {
  int i, retVal = 0;
  int tableSize = sizeof(tab_insamplerate) / sizeof(TAB_INSAMPLERATE);

  AVOID_COMPILER_WARNING(key);

  for (i = 0; i < tableSize; i++) {
    if (1 && ((int)(tab_insamplerate[i].keyinsamplerate) == key) && ((int)(tab_insamplerate[i].insamplerate) == value)

    ) {
      retVal = 1;
      break;
    }
  }

  return retVal;
}
