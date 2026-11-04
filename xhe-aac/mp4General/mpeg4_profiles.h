
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

#ifndef MPEG4_PROFILES_H
#define MPEG4_PROFILES_H

typedef enum {
  MAIN_AUDIO_PROFILE_L1 = 0x01,
  MAIN_AUDIO_PROFILE_L2 = 0x02,
  MAIN_AUDIO_PROFILE_L3 = 0x03,
  MAIN_AUDIO_PROFILE_L4 = 0x04,
  SCALABLE_AUDIO_PROFILE_L1 = 0x05,
  SCALABLE_AUDIO_PROFILE_L2 = 0x06,
  SCALABLE_AUDIO_PROFILE_L3 = 0x07,
  SCALABLE_AUDIO_PROFILE_L4 = 0x08,
  SPEECH_AUDIO_PROFILE_L1 = 0x09,
  SPEECH_AUDIO_PROFILE_L2 = 0x0A,
  SYNTHETIC_AUDIO_PROFILE_L1 = 0x0B,
  SYNTHETIC_AUDIO_PROFILE_L2 = 0x0C,
  SYNTHETIC_AUDIO_PROFILE_L3 = 0x0D,
  HIGH_QUALITY_AUDIO_PROFILE_L1 = 0x0E,
  HIGH_QUALITY_AUDIO_PROFILE_L2 = 0x0F,
  HIGH_QUALITY_AUDIO_PROFILE_L3 = 0x10,
  HIGH_QUALITY_AUDIO_PROFILE_L4 = 0x11,
  HIGH_QUALITY_AUDIO_PROFILE_L5 = 0x12,
  HIGH_QUALITY_AUDIO_PROFILE_L6 = 0x13,
  HIGH_QUALITY_AUDIO_PROFILE_L7 = 0x14,
  HIGH_QUALITY_AUDIO_PROFILE_L8 = 0x15,
  LOW_DELAY_AUDIO_PROFILE_L1 = 0x16,
  LOW_DELAY_AUDIO_PROFILE_L2 = 0x17,
  LOW_DELAY_AUDIO_PROFILE_L3 = 0x18,
  LOW_DELAY_AUDIO_PROFILE_L4 = 0x19,
  LOW_DELAY_AUDIO_PROFILE_L5 = 0x1A,
  LOW_DELAY_AUDIO_PROFILE_L6 = 0x1B,
  LOW_DELAY_AUDIO_PROFILE_L7 = 0x1C,
  LOW_DELAY_AUDIO_PROFILE_L8 = 0x1D,
  NATURAL_AUDIO_PROFILE_L1 = 0x1E,
  NATURAL_AUDIO_PROFILE_L2 = 0x1F,
  NATURAL_AUDIO_PROFILE_L3 = 0x20,
  NATURAL_AUDIO_PROFILE_L4 = 0x21,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L1 = 0x22,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L2 = 0x23,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L3 = 0x24,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L4 = 0x25,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L5 = 0x26,
  MOBILE_AUDIO_INTERNET_WORKING_PROFILE_L6 = 0x27,
  AAC_PROFILE_L1 = 0x28,
  AAC_PROFILE_L2 = 0x29,
  AAC_PROFILE_L4 = 0x2A,
  AAC_PROFILE_L5 = 0x2B,
  HIGH_EFFICIENCY_AAC_PROFILE_L2 = 0x2C,
  HIGH_EFFICIENCY_AAC_PROFILE_L3 = 0x2D,
  HIGH_EFFICIENCY_AAC_PROFILE_L4 = 0x2E,
  HIGH_EFFICIENCY_AAC_PROFILE_L5 = 0x2F,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L2 = 0x30,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L3 = 0x31,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L4 = 0x32,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L5 = 0x33,
  LOW_DELAY_AAC_PROFILE_L1 = 0x34,
  BASELINE_MPEG_SURROUND_PROFILE_L1 = 0x35,
  BASELINE_MPEG_SURROUND_PROFILE_L2 = 0x36,
  BASELINE_MPEG_SURROUND_PROFILE_L3 = 0x37,
  BASELINE_MPEG_SURROUND_PROFILE_L4 = 0x38,
  BASELINE_MPEG_SURROUND_PROFILE_L5 = 0x39,
  BASELINE_MPEG_SURROUND_PROFILE_L6 = 0x3A,
  HIGH_DEFINITION_AAC_PROFILE_L1 = 0x3B,
  ALS_SIMPLE_PROFILE_L1 = 0x3C,
  SAOC_BASELINE_PROFILE_L1 = 0x3D,
  SAOC_BASELINE_PROFILE_L2 = 0x3E,
  SAOC_BASELINE_PROFILE_L3 = 0x3F,
  SAOC_BASELINE_PROFILE_L4 = 0x40,
  SAOC_LD_PROFILE_L1 = 0x41,
  SAOC_LD_PROFILE_L2 = 0x42,
  SAOC_LD_PROFILE_L3 = 0x43,
  BASELINE_USAC_PROFILE_L1 = 0x44,
  BASELINE_USAC_PROFILE_L2 = 0x45,
  BASELINE_USAC_PROFILE_L3 = 0x46,
  BASELINE_USAC_PROFILE_L4 = 0x47,
  EXTENDED_HE_AAC_PROFILE_L1 = 0x48,
  EXTENDED_HE_AAC_PROFILE_L2 = 0x49,
  EXTENDED_HE_AAC_PROFILE_L3 = 0x4A,
  EXTENDED_HE_AAC_PROFILE_L4 = 0x4B,
  LOW_DELAY_AAC_V2_PROFILE_L1 = 0x4C,
  LOW_DELAY_AAC_V2_PROFILE_L2 = 0x4D,
  LOW_DELAY_AAC_V2_PROFILE_L3 = 0x4E,
  LOW_DELAY_AAC_V2_PROFILE_L4 = 0x4F,
  AAC_PROFILE_L6 = 0x50,
  AAC_PROFILE_L7 = 0x51,
  HIGH_EFFICIENCY_AAC_PROFILE_L6 = 0x52,
  HIGH_EFFICIENCY_AAC_PROFILE_L7 = 0x53,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L6 = 0x54,
  HIGH_EFFICIENCY_AAC_V2_PROFILE_L7 = 0x55,
  EXTENDED_HE_AAC_PROFILE_L6 = 0x56,
  EXTENDED_HE_AAC_PROFILE_L7 = 0x57,
  NO_AUDIO_PROFILE_SPECIFIED = 0xFE,
  NO_AUDIO_CAPABILITY_REQUIRED = 0xFF
} AUDIO_PROFILE_LEVEL;

#endif
