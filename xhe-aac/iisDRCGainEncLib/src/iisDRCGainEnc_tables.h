
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

#ifndef IISDRCGAINENC_TABLES_H
#define IISDRCGAINENC_TABLES_H

#if defined __cplusplus
extern "C" {
#endif

typedef struct {
  int sizeBits;
  int encVal;
  float value;
} SLOPE_TABLE;

typedef struct {
  int sizeBits;
  int encVal;
  float value;
} GAINDELTA_TABLE;

static const SLOPE_TABLE bsSlopeTable[] =
    {
        {6, 0x018, -3.0518f},
        {8, 0x042, -1.2207f},
        {7, 0x032, -0.4883f},
        {5, 0x00A, -0.1953f},
        {5, 0x009, -0.0781f},
        {5, 0x00D, -0.0312f},
        {2, 0x000, -0.0050f},
        {1, 0x001, 0.0000f},
        {4, 0x007, 0.0050f},
        {5, 0x00B, 0.0312f},
        {6, 0x011, 0.0781f},
        {9, 0x087, 0.1953f},
        {9, 0x086, 0.4883f},
        {7, 0x020, 1.2207f},
        {7, 0x033, 3.0518f}};

static const float bsDownmixCoefficientTable[] =
    {
        0.0f,
        -0.5f,
        -1.0f,
        -1.5f,
        -2.0f,
        -2.5f,
        -3.0f,
        -3.5f,
        -4.0f,
        -4.5f,
        -5.0f,
        -5.5f,
        -6.0f,
        -7.5f,
        -9.0f,
        -13.0f};

static const float bsDownmixCoefficientLfeTable[] =
    {
        10.0f,
        6.0f,
        4.5f,
        3.0f,
        1.5f,
        0.0f,
        -1.5f,
        -3.0f,
        -4.5f,
        -6.0f,
        -10.0f,
        -15.0f,
        -20.0f,
        -30.0f,
        -40.0f,
        -60.0f};

static const float bsDownmixCoefficientV1Table[] =
    {
        10.00f,
        6.00f,
        4.50f,
        3.00f,
        1.50f,
        0.00f,
        -0.50f,
        -1.00f,
        -1.50f,
        -2.00f,
        -2.50f,
        -3.00f,
        -3.50f,
        -4.00f,
        -4.50f,
        -5.00f,
        -5.50f,
        -6.00f,
        -6.50f,
        -7.00f,
        -7.50f,
        -8.00f,
        -9.00f,
        -10.00f,
        -11.00f,
        -12.00f,
        -15.00f,
        -20.00f,
        -25.00f,
        -30.00f,
        -40.00f,
        -60.0f};

static const GAINDELTA_TABLE bsGainDeltaGainCodingProfile0_1[] =
    {
        {4, 0x000, -2.000f},
        {9, 0x039, -1.875f},
        {11, 0x0E2, -1.750f},
        {11, 0x0E3, -1.625f},
        {10, 0x070, -1.500f},
        {10, 0x1AC, -1.375f},
        {10, 0x1AD, -1.250f},
        {9, 0x0D5, -1.125f},
        {7, 0x00F, -1.000f},
        {7, 0x034, -0.875f},
        {7, 0x036, -0.750f},
        {6, 0x019, -0.625f},
        {5, 0x002, -0.500f},
        {5, 0x00F, -0.375f},
        {3, 0x001, -0.250f},
        {2, 0x003, -0.125f},
        {3, 0x002, 0.000f},
        {2, 0x002, 0.125f},
        {6, 0x018, 0.250f},
        {6, 0x006, 0.375f},
        {7, 0x037, 0.500f},
        {8, 0x01D, 0.625f},
        {9, 0x0D7, 0.750f},
        {9, 0x0D4, 0.875f},
        {5, 0x00E, 1.000f}};

static const GAINDELTA_TABLE bsGainDeltaGainCodingProfile2[] =
    {
        {7, 0x06A, -4.000f},
        {11, 0x07A, -3.875f},
        {11, 0x07B, -3.750f},
        {9, 0x1AD, -3.625f},
        {10, 0x03C, -3.500f},
        {9, 0x1AC, -3.375f},
        {9, 0x1A6, -3.250f},
        {9, 0x0CD, -3.125f},
        {10, 0x19E, -3.000f},
        {10, 0x19F, -2.875f},
        {9, 0x0CE, -2.750f},
        {9, 0x1A7, -2.625f},
        {9, 0x01F, -2.500f},
        {9, 0x0CC, -2.375f},
        {8, 0x0D2, -2.250f},
        {8, 0x0AB, -2.125f},
        {8, 0x0AA, -2.000f},
        {8, 0x04F, -1.875f},
        {7, 0x054, -1.750f},
        {7, 0x068, -1.625f},
        {7, 0x026, -1.500f},
        {7, 0x006, -1.375f},
        {6, 0x02B, -1.250f},
        {6, 0x028, -1.125f},
        {6, 0x002, -1.000f},
        {5, 0x011, -0.875f},
        {5, 0x00E, -0.750f},
        {4, 0x00C, -0.625f},
        {4, 0x009, -0.500f},
        {4, 0x005, -0.375f},
        {4, 0x003, -0.250f},
        {3, 0x007, -0.125f},
        {4, 0x001, 0.000f},
        {4, 0x00B, 0.125f},
        {5, 0x005, 0.250f},
        {5, 0x004, 0.375f},
        {5, 0x008, 0.500f},
        {5, 0x000, 0.625f},
        {5, 0x00D, 0.750f},
        {5, 0x00F, 0.875f},
        {5, 0x010, 1.000f},
        {5, 0x01B, 1.125f},
        {6, 0x012, 1.250f},
        {6, 0x018, 1.375f},
        {6, 0x029, 1.500f},
        {7, 0x032, 1.625f},
        {8, 0x04E, 1.750f},
        {8, 0x0D7, 1.875f},
        {8, 0x00E, 2.000f}};

#if defined __cplusplus
}
#endif
#endif
