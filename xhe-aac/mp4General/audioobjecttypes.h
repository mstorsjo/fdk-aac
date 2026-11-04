
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

#ifndef AUDIOOBJECTTYPES_H__
#define AUDIOOBJECTTYPES_H__

typedef enum _AUDIO_OBJECT_TYPE {
  AOT_NULL_OBJECT = 0,
  AOT_AAC_MAIN = 1,
  AOT_AAC_LC = 2,
  AOT_AAC_SSR = 3,
  AOT_AAC_LTP = 4,
  AOT_SBR = 5,
  AOT_AAC_SCAL = 6,
  AOT_TWIN_VQ = 7,
  AOT_CELP = 8,
  AOT_HVXC = 9,
  AOT_RSVD_10 = 10,
  AOT_RSVD_11 = 11,
  AOT_TTSI = 12,
  AOT_MAIN_SYNTH = 13,
  AOT_WAV_TAB_SYNTH = 14,
  AOT_GEN_MIDI = 15,
  AOT_ALG_SYNTH_AUD_FX = 16,
  AOT_ER_AAC_LC = 17,
  AOT_RSVD_18 = 18,
  AOT_ER_AAC_LTP = 19,
  AOT_ER_AAC_SCAL = 20,
  AOT_ER_TWIN_VQ = 21,
  AOT_ER_BSAC = 22,
  AOT_ER_AAC_LD = 23,
  AOT_ER_CELP = 24,
  AOT_ER_HVXC = 25,
  AOT_ER_HILN = 26,
  AOT_ER_PARA = 27,
  AOT_RSVD_28 = 28,
  AOT_PS = 29,
  AOT_MPEGS = 30,

  AOT_ESCAPE = 31,

  AOT_MP3ONMP4_L1 = 32,
  AOT_MP3ONMP4_L2 = 33,
  AOT_MP3ONMP4_L3 = 34,
  AOT_RSVD_35 = 35,
  AOT_RSVD_36 = 36,
  AOT_SLS = 37,
  AOT_SLS_NC = 38,
  AOT_ER_AAC_ELD = 39,

  AOT_RSVD_40 = 40,
  AOT_RSVD_41 = 41,
  AOT_USAC = 42,

  AOT_SAOC = 43,
  AOT_LD_MPEGS = 44,
  AOT_SAOC_DE = 45,
  AOT_AUDIO_SYNC = 46,
  AOT_MP2_AAC_MAIN = 128,
  AOT_MP2_AAC_LC = 129,
  AOT_MP2_AAC_SSR = 130,

  AOT_MPEGH = 136,
  AOT_INVALID = 555,
  AOT_DUMMY = 556

} AUDIO_OBJECT_TYPE;

#endif
