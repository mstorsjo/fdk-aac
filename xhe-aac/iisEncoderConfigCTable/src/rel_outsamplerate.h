
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

#ifndef REL_OUTSAMPLERATE_H
#define REL_OUTSAMPLERATE_H

typedef enum {

  KEY_OUTSR_11025 = 0,
  KEY_OUTSR_12000 = 1,
  KEY_OUTSR_16000 = 2,
  KEY_OUTSR_176400 = 3,
  KEY_OUTSR_192000 = 4,
  KEY_OUTSR_22050 = 5,
  KEY_OUTSR_24000 = 6,
  KEY_OUTSR_29400 = 7,
  KEY_OUTSR_32000 = 8,
  KEY_OUTSR_35280 = 9,
  KEY_OUTSR_38400 = 10,
  KEY_OUTSR_44100 = 11,
  KEY_OUTSR_48000 = 12,
  KEY_OUTSR_6000 = 13,
  KEY_OUTSR_64000 = 14,
  KEY_OUTSR_8000 = 15,
  KEY_OUTSR_88200 = 16,
  KEY_OUTSR_96000 = 17,
  KEY_OUTSR_AAC_RNG_11025_48000_D32000 = 18,
  KEY_OUTSR_AAC_RNG_11025_48000_D44100_D48000 = 19,
  KEY_OUTSR_AAC_RNG_11025_96000_D44100_D48000 = 20,
  KEY_OUTSR_AAC_RNG_12000_96000_D44100_D48000 = 21,
  KEY_OUTSR_AAC_RNG_16000_96000_D44100_D48000 = 22,
  KEY_OUTSR_AAC_RNG_22050_32000_D22050_D24000 = 23,
  KEY_OUTSR_AAC_RNG_22050_32000_D22050_D32000 = 24,
  KEY_OUTSR_AAC_RNG_22050_96000_D44100_D48000 = 25,
  KEY_OUTSR_AAC_RNG_24000_96000_D44100_D48000 = 26,
  KEY_OUTSR_AAC_RNG_32000_48000_D44100_D32000 = 27,
  KEY_OUTSR_AAC_RNG_32000_48000_D44100_D48000 = 28,
  KEY_OUTSR_AAC_RNG_32000_96000_D44100_D48000 = 29,
  KEY_OUTSR_AAC_RNG_32000_96000_D88200_D96000_D44100_D48000 = 30,
  KEY_OUTSR_AAC_RNG_44100_96000_D44100_D96000 = 31,
  KEY_OUTSR_AAC_RNG_44100_96000_D88200_D96000_D44100_D48000 = 32,
  KEY_OUTSR_AAC_RNG_48000_96000_D88200_D96000_D48000 = 33,
  KEY_OUTSR_AAC_RNG_8000_16000_D11025_D16000 = 34,
  KEY_OUTSR_AAC_RNG_8000_16000_D16000 = 35,
  KEY_OUTSR_AAC_RNG_8000_24000_D22050_D16000 = 36,
  KEY_OUTSR_AAC_RNG_8000_32000_D22050_D24000 = 37,
  KEY_OUTSR_AAC_RNG_8000_48000_D22050_D16000 = 38,
  KEY_OUTSR_AAC_RNG_8000_48000_D22050_D24000 = 39,
  KEY_OUTSR_AAC_RNG_8000_48000_D32000 = 40,
  KEY_OUTSR_ALL = 41,
  KEY_OUTSR_DRM_32000 = 42,
  KEY_OUTSR_DRM_38400 = 43,
  KEY_OUTSR_DRM_48000 = 44,
  KEY_OUTSR_DRM_RNG_32000_48000_D32000_D48000 = 45,
  KEY_OUTSR_LST_22050_24000_D22050_D24000 = 46,
  KEY_OUTSR_LST_44100_48000_D44100_D48000 = 47,
  KEY_OUTSR_LST_88200_96000_D44100_D48000 = 48,
  KEY_OUTSR_LST_D29400_D32000 = 49,
  KEY_OUTSR_LST_D44100_D38400 = 50,
  KEY_OUTSR_LST_D44100_D48000 = 51,
  KEY_OUTSR_USAC_RNG_44100_96000_D44100_D96000 = 52,

  PARAMLIST_KEYOUTSAMPLERATE_LAST_ENTRY

} PARAMLIST_KEYOUTSAMPLERATE;

#endif
