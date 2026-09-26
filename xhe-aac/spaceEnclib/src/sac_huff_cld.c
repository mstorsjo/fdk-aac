
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

#include <assert.h>

#include "spaceEnclib_const.h"
#include "sac_huff_tab.h"

const HUFF_CLD_TABLE huffCLDTab =
    {
        {{{0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x00fe, 0x01fe, 0x03fe, 0x07fe, 0x0ffe, 0x1ffe, 0x7ffe, 0x7ffc, 0xfffe, 0xfffa, 0x1fffe, 0x1fff6, 0x3fffe, 0x3ffff, 0x7ffde, 0x3ffee, 0xfffbe, 0x1fff7e, 0xfffbfc, 0xfffbfd, 0xfffbfe, 0xfffbff, 0x7ffdfc, 0x7ffdfd},
          {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 15, 16, 16, 17, 17, 18, 18, 19, 18, 20, 21, 24, 24, 24, 24, 23, 23}},
         {{0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x01fe, 0x01fc, 0x03fe, 0x03fa, 0x07fe, 0x07f6, 0x0ffe, 0x0fee, 0x1ffe, 0x1fde, 0x3ffe, 0x3fbe, 0x3fbf, 0x7ffe, 0xfffe, 0x1fffe, 0x7fffe, 0x7fffc, 0xffffa, 0x1ffffc, 0x1ffffd, 0x1ffffe, 0x1fffff, 0xffffb},
          {1, 2, 3, 4, 5, 6, 7, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 14, 15, 16, 17, 19, 19, 20, 21, 21, 21, 21, 20}}},
        {{{{{{0x00000002, 0x00000002, 0x00000004, 0x0000003e},
             {0x00000006, 0x00000007, 0x0000000e, 0x000000fe},
             {0x0000007e, 0x0000001e, 0x0000000c, 0x00000005},
             {0x000000ff, 0x0000000d, 0x00000000, 0x00000003}},
            {{2, 3, 5, 8},
             {4, 4, 6, 10},
             {9, 7, 6, 5},
             {10, 6, 3, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x00000003, 0x00000010, 0x0000007c, 0x000000d6, 0x000003ee},
             {0x0000000a, 0x0000000c, 0x00000016, 0x00000034, 0x000000fe, 0x00001f7e},
             {0x0000007e, 0x00000036, 0x00000026, 0x00000046, 0x0000011e, 0x000001f6},
             {0x0000011f, 0x000000d7, 0x0000008e, 0x000000ff, 0x0000006a, 0x0000004e},
             {0x00000fbe, 0x000007de, 0x0000004f, 0x00000037, 0x00000017, 0x0000001e},
             {0x00001f7f, 0x000000fa, 0x00000022, 0x00000012, 0x0000000e, 0x00000000}},
            {{3, 3, 5, 7, 8, 10},
             {4, 4, 5, 6, 8, 13},
             {7, 6, 6, 7, 9, 9},
             {9, 8, 8, 8, 7, 7},
             {12, 11, 7, 6, 5, 5},
             {13, 8, 6, 5, 4, 2}},
            {0x00000000, 0}},
           {{{0x0000000e, 0x0000000a, 0x0000000a, 0x0000007c, 0x000000be, 0x0000017a, 0x000000ee, 0x000007b6},
             {0x00000006, 0x0000000c, 0x00000016, 0x00000026, 0x0000003e, 0x0000002e, 0x000001ec, 0x000047ce},
             {0x00000016, 0x0000003c, 0x00000022, 0x0000004e, 0x0000003f, 0x0000005e, 0x000008fa, 0x000008fb},
             {0x0000005f, 0x000000fa, 0x000000bf, 0x0000003a, 0x000001f6, 0x000001de, 0x000003da, 0x000007b7},
             {0x000001df, 0x000003ee, 0x0000017b, 0x000003ef, 0x000001ee, 0x0000008e, 0x000001ef, 0x000001fe},
             {0x000008f8, 0x0000047e, 0x0000047f, 0x00000076, 0x0000003c, 0x00000046, 0x0000007a, 0x0000007e},
             {0x000023e6, 0x000011f2, 0x000001ff, 0x0000003d, 0x0000004f, 0x0000002e, 0x00000012, 0x00000004},
             {0x000047cf, 0x0000011e, 0x000000bc, 0x000000fe, 0x0000001c, 0x00000010, 0x0000000d, 0x00000000}},
            {{4, 4, 5, 7, 8, 9, 9, 11},
             {4, 4, 5, 6, 7, 7, 9, 15},
             {6, 6, 6, 7, 7, 8, 12, 12},
             {8, 8, 8, 7, 9, 10, 10, 11},
             {10, 10, 9, 10, 9, 8, 9, 9},
             {12, 11, 11, 8, 7, 7, 7, 7},
             {14, 13, 9, 7, 7, 6, 5, 4},
             {15, 9, 8, 8, 6, 5, 4, 2}},
            {0x00000000, 0}},
           {{{0x00000006, 0x00000007, 0x00000006, 0x0000007e, 0x0000000a, 0x0000001e, 0x0000008a, 0x0000004e, 0x00000276, 0x000002e2},
             {0x00000000, 0x0000000a, 0x00000016, 0x00000026, 0x00000076, 0x000000f2, 0x00000012, 0x0000005e, 0x0000008b, 0x00002e76},
             {0x00000012, 0x00000007, 0x00000038, 0x0000007c, 0x00000008, 0x00000046, 0x000000f6, 0x000001ca, 0x0000173a, 0x00001738},
             {0x0000009e, 0x0000004a, 0x00000026, 0x0000000c, 0x0000004e, 0x000000f7, 0x0000013a, 0x0000009e, 0x000009fe, 0x0000013e},
             {0x00000026, 0x0000001a, 0x000001e6, 0x000001e2, 0x000000ee, 0x000001ce, 0x00000277, 0x000003ce, 0x000002e6, 0x000004fc},
             {0x000002e3, 0x00000170, 0x00000172, 0x000000ba, 0x0000003e, 0x000001e3, 0x0000001b, 0x0000003f, 0x0000009e, 0x0000009f},
             {0x00000b9e, 0x000009ff, 0x000004fd, 0x000004fe, 0x000001cf, 0x000000ef, 0x00000044, 0x0000005f, 0x000000e4, 0x000000f0},
             {0x00002e72, 0x0000013f, 0x00000b9f, 0x0000013e, 0x000000fe, 0x00000047, 0x0000000e, 0x0000007d, 0x00000010, 0x00000024},
             {0x00002e77, 0x00005ce6, 0x000000bb, 0x000000e6, 0x00000016, 0x000000ff, 0x0000007a, 0x0000003a, 0x00000017, 0x00000002},
             {0x00005ce7, 0x000003cf, 0x00000017, 0x000001cb, 0x0000009c, 0x0000004b, 0x00000016, 0x0000000a, 0x00000008, 0x00000006}},
            {{4, 4, 5, 7, 7, 8, 9, 10, 10, 11},
             {4, 4, 5, 6, 7, 8, 8, 8, 9, 15},
             {6, 5, 6, 7, 7, 8, 8, 9, 14, 14},
             {8, 7, 7, 7, 8, 8, 9, 11, 12, 12},
             {9, 8, 9, 9, 8, 9, 10, 10, 11, 11},
             {11, 10, 10, 9, 9, 9, 8, 9, 9, 9},
             {13, 12, 11, 11, 9, 8, 8, 8, 8, 8},
             {15, 12, 13, 9, 8, 8, 7, 7, 6, 6},
             {15, 16, 9, 8, 8, 8, 7, 6, 5, 4},
             {16, 10, 8, 9, 8, 7, 6, 5, 4, 3}},
            {0x00000000, 0}}},
          {{{{0x00000000, 0x0000003e, 0x0000076e, 0x00000ede},
             {0x00000006, 0x0000003f, 0x000003b6, 0x0000003a},
             {0x0000001c, 0x000000ee, 0x000001da, 0x0000001e},
             {0x000000ef, 0x00000edf, 0x000000ec, 0x00000002}},
            {{1, 6, 11, 12},
             {3, 6, 10, 6},
             {5, 8, 9, 5},
             {8, 12, 8, 2}},
            {0x00000000, 0}},
           {{{0x00000006, 0x0000001c, 0x0000007e, 0x00000efc, 0x0000effe, 0x0001dffe},
             {0x00000004, 0x0000000a, 0x0000003e, 0x00000efe, 0x000077fe, 0x00000076},
             {0x00000006, 0x00000016, 0x000000be, 0x00000efd, 0x000000ee, 0x0000000e},
             {0x0000003e, 0x0000002e, 0x000001de, 0x000003be, 0x0000007e, 0x0000001e},
             {0x0000007f, 0x0000005e, 0x00003bfe, 0x000000fe, 0x0000001e, 0x00000002},
             {0x000000bf, 0x0001dfff, 0x00001dfe, 0x000000ff, 0x0000003a, 0x00000000}},
            {{3, 5, 8, 12, 16, 17},
             {3, 4, 7, 12, 15, 7},
             {4, 5, 8, 12, 8, 5},
             {6, 6, 9, 10, 7, 5},
             {7, 7, 14, 9, 6, 3},
             {8, 17, 13, 9, 6, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x0000001c, 0x000000bc, 0x000005fc, 0x00005ffe, 0x0002ffde, 0x000bff7e, 0x0017feff},
             {0x00000004, 0x0000000a, 0x0000000e, 0x000002fa, 0x000001fe, 0x0000bff2, 0x0005ffbe, 0x000000ee},
             {0x00000002, 0x00000016, 0x000000f6, 0x000005fe, 0x000001ff, 0x0000bff6, 0x000001de, 0x0000007e},
             {0x00000000, 0x0000003c, 0x0000000e, 0x0000003e, 0x00002ffe, 0x000002fb, 0x000000f7, 0x0000002e},
             {0x00000006, 0x0000007a, 0x0000000a, 0x0000007e, 0x000000fe, 0x00000016, 0x00000006, 0x00000002},
             {0x0000000f, 0x00000076, 0x00000017, 0x00005ff8, 0x00000bfe, 0x0000001e, 0x0000007f, 0x00000003},
             {0x00000004, 0x000000bd, 0x0000bff3, 0x00005fff, 0x00000bfa, 0x0000017c, 0x0000003a, 0x00000003},
             {0x0000017e, 0x0017fefe, 0x00017fee, 0x00005ffa, 0x00000bfb, 0x000001df, 0x0000003e, 0x00000006}},
            {{3, 5, 8, 11, 15, 18, 20, 21},
             {3, 4, 7, 10, 13, 16, 19, 8},
             {4, 5, 8, 11, 13, 16, 9, 7},
             {5, 6, 8, 10, 14, 10, 8, 6},
             {6, 7, 8, 11, 12, 9, 7, 5},
             {7, 7, 9, 15, 12, 9, 7, 4},
             {7, 8, 16, 15, 12, 9, 6, 3},
             {9, 21, 17, 15, 12, 9, 6, 3}},
            {0x0017feff, 21}},
           {{{0x0000000e, 0x00000014, 0x0000008e, 0x000004fe, 0x000023fe, 0x00008ffe, 0x0005ffbc, 0x0017fef7, 0x0017fef7, 0x0017fef7},
             {0x00000002, 0x00000002, 0x00000044, 0x0000027e, 0x000017fc, 0x0000bff6, 0x0005ffbe, 0x00011ff8, 0x000bff7a, 0x000000bc},
             {0x00000006, 0x00000016, 0x0000001a, 0x000000fe, 0x000011f6, 0x0000bffe, 0x00011ff9, 0x0017fef6, 0x0000011e, 0x00000056},
             {0x00000010, 0x0000003e, 0x0000009e, 0x000007fe, 0x000011f7, 0x00005ff8, 0x00017fee, 0x000007ff, 0x000000ae, 0x0000001e},
             {0x00000026, 0x0000000e, 0x000001ee, 0x0000047e, 0x00000bfc, 0x0000bfff, 0x000008fa, 0x0000006e, 0x000001ef, 0x0000007e},
             {0x0000007a, 0x0000004e, 0x0000007e, 0x000000de, 0x000011fe, 0x00002ffe, 0x000004ff, 0x000000ff, 0x000000bd, 0x0000002e},
             {0x000000fe, 0x000000af, 0x000001ec, 0x000001be, 0x00011ffe, 0x00002ffa, 0x000008fe, 0x000003fe, 0x00000046, 0x00000012},
             {0x0000003e, 0x00000045, 0x000002fe, 0x000bff7e, 0x00005ff9, 0x00005ffa, 0x00000bfd, 0x0000013e, 0x0000000c, 0x00000007},
             {0x000000be, 0x00000036, 0x000bff7f, 0x00023ffe, 0x00011ffa, 0x00005ffe, 0x000001bf, 0x000001ed, 0x0000002a, 0x00000000},
             {0x0000017e, 0x0017fef7, 0x00047ffe, 0x00047fff, 0x00011ffb, 0x00002ffb, 0x0000047c, 0x000001fe, 0x0000003c, 0x00000006}},
            {{4, 5, 8, 11, 14, 16, 19, 21, 21, 21},
             {3, 4, 7, 10, 13, 16, 19, 17, 20, 8},
             {4, 5, 7, 10, 13, 16, 17, 21, 9, 7},
             {5, 6, 8, 11, 13, 15, 17, 11, 8, 7},
             {6, 6, 9, 11, 12, 16, 12, 9, 9, 7},
             {7, 7, 9, 10, 13, 14, 11, 10, 8, 6},
             {8, 8, 9, 11, 17, 14, 12, 10, 7, 5},
             {8, 7, 10, 20, 15, 15, 12, 9, 6, 4},
             {8, 8, 20, 18, 17, 15, 11, 9, 6, 3},
             {9, 21, 19, 19, 17, 14, 11, 9, 6, 3}},
            {0x0017fef7, 21}}}},
         {{{{{0x00000000, 0x0000001e, 0x000003be, 0x00000efe},
             {0x00000006, 0x0000001c, 0x000001de, 0x000000ea},
             {0x00000074, 0x000000ee, 0x000000eb, 0x0000001f},
             {0x0000077e, 0x00000eff, 0x00000076, 0x00000002}},
            {{1, 5, 10, 12},
             {3, 5, 9, 8},
             {7, 8, 8, 5},
             {11, 12, 7, 2}},
            {0x00000000, 0}},
           {{{0x00000000, 0x00000006, 0x00000024, 0x0000025e, 0x00003cfe, 0x000079fe},
             {0x00000006, 0x00000007, 0x00000078, 0x000003ce, 0x00001e7e, 0x000000be},
             {0x00000008, 0x0000003e, 0x00000026, 0x0000012e, 0x000000bf, 0x0000002e},
             {0x00000027, 0x0000007a, 0x000001e4, 0x00000096, 0x0000007b, 0x0000003f},
             {0x000001e6, 0x000001e5, 0x00000f3e, 0x0000005e, 0x00000016, 0x0000000e},
             {0x0000079e, 0x000079ff, 0x0000025f, 0x0000004a, 0x0000000a, 0x00000002}},
            {{2, 4, 7, 11, 14, 15},
             {3, 4, 7, 10, 13, 9},
             {5, 6, 7, 10, 9, 7},
             {7, 7, 9, 9, 7, 6},
             {9, 9, 12, 8, 6, 4},
             {11, 15, 11, 8, 5, 2}},
            {0x00000000, 0}},
           {{{0x00000000, 0x00000006, 0x000000de, 0x0000069e, 0x000034fe, 0x0001a7fe, 0x00069ff6, 0x00069ff7},
             {0x00000002, 0x0000000c, 0x0000006a, 0x0000034e, 0x00001fde, 0x000069fe, 0x0001a7fc, 0x00000372},
             {0x0000003e, 0x0000003c, 0x000000df, 0x000001ee, 0x00000dde, 0x000069fa, 0x00000373, 0x0000007a},
             {0x0000003e, 0x00000068, 0x000001ba, 0x000003f6, 0x00000d3e, 0x0000034c, 0x000001fa, 0x000000d2},
             {0x0000007e, 0x0000007f, 0x000001f8, 0x000006ee, 0x000003de, 0x000001b8, 0x000001fc, 0x0000006b},
             {0x000000f6, 0x000001fe, 0x0000034d, 0x00003fbe, 0x000007f6, 0x000003fa, 0x0000003c, 0x0000003d},
             {0x000003f7, 0x00000376, 0x0001a7ff, 0x00003fbf, 0x00000ddf, 0x000001f9, 0x00000036, 0x0000000e},
             {0x000003df, 0x00034ffa, 0x000069fb, 0x000034fc, 0x00000fee, 0x000001ff, 0x0000000e, 0x00000002}},
            {{2, 4, 8, 11, 14, 17, 19, 19},
             {3, 4, 7, 10, 13, 15, 17, 10},
             {6, 6, 8, 10, 12, 15, 10, 8},
             {7, 7, 9, 10, 12, 10, 9, 8},
             {8, 8, 9, 11, 11, 9, 9, 7},
             {9, 9, 10, 14, 11, 10, 7, 6},
             {10, 10, 17, 14, 12, 9, 6, 4},
             {11, 18, 15, 14, 12, 9, 5, 2}},
            {0x00000000, 0}},
           {{{0x00000006, 0x00000004, 0x00000012, 0x000007fe, 0x00001f7e, 0x0000fbfe, 0x0001f7fe, 0x000b7dfe, 0x000b7dff, 0x000b7dff},
             {0x00000000, 0x00000006, 0x0000007c, 0x00000046, 0x000007d0, 0x00001f4e, 0x0000b7fe, 0x00005bee, 0x00016fbe, 0x000003ee},
             {0x00000006, 0x0000000a, 0x0000002e, 0x000003fe, 0x000007d2, 0x00001f4f, 0x00002dfe, 0x0000b7de, 0x000001fe, 0x0000002e},
             {0x0000007a, 0x0000007e, 0x0000007a, 0x000001fa, 0x000007fe, 0x00001f7c, 0x000016fa, 0x0000009e, 0x00000020, 0x00000021},
             {0x000000fe, 0x00000016, 0x000000fe, 0x0000016e, 0x0000009f, 0x00000b7c, 0x000003de, 0x000000b6, 0x000000be, 0x0000007c},
             {0x0000005a, 0x00000078, 0x00000047, 0x00000044, 0x000007ff, 0x000007d1, 0x000001f6, 0x000001f7, 0x0000002f, 0x0000002c},
             {0x000000fc, 0x000001f6, 0x000000f6, 0x000007ff, 0x000016fe, 0x000002de, 0x000003ea, 0x000000bf, 0x000000fa, 0x0000000a},
             {0x0000004e, 0x00000026, 0x000001ee, 0x00005bfe, 0x00003efe, 0x00000b7e, 0x000003eb, 0x000001fe, 0x0000007b, 0x00000007},
             {0x000001fb, 0x00000045, 0x00016ffe, 0x0001f7ff, 0x00002df6, 0x00001f7d, 0x000003fe, 0x0000005e, 0x0000003c, 0x0000000e},
             {0x000003df, 0x0005befe, 0x0002df7e, 0x00016fff, 0x00007dfe, 0x00000fa6, 0x000007de, 0x00000079, 0x0000000e, 0x00000002}},
            {{3, 4, 7, 11, 13, 16, 17, 21, 21, 21},
             {3, 4, 7, 9, 12, 14, 17, 16, 18, 10},
             {5, 5, 7, 10, 12, 14, 15, 17, 10, 8},
             {7, 7, 8, 10, 12, 13, 14, 10, 8, 8},
             {8, 7, 9, 10, 10, 13, 11, 9, 9, 8},
             {8, 8, 9, 9, 12, 12, 10, 10, 8, 7},
             {9, 9, 9, 11, 14, 11, 11, 9, 8, 6},
             {9, 8, 10, 16, 14, 13, 11, 9, 7, 5},
             {10, 9, 18, 17, 15, 13, 11, 8, 6, 4},
             {11, 20, 19, 18, 15, 13, 11, 8, 5, 2}},
            {0x000b7dff, 21}}},
          {{{{0x00000000, 0x0000000e, 0x000000fa, 0x000007de},
             {0x0000000c, 0x0000001e, 0x000000fe, 0x000001f6},
             {0x000000ff, 0x0000007c, 0x0000007e, 0x0000001a},
             {0x000007df, 0x000003ee, 0x0000001b, 0x00000002}},
            {{1, 4, 8, 11},
             {4, 5, 8, 9},
             {8, 7, 7, 5},
             {11, 10, 5, 2}},
            {0x00000000, 0}},
           {{{0x00000006, 0x0000000e, 0x0000007c, 0x000003fe, 0x00000fbe, 0x00003efe},
             {0x00000000, 0x00000001, 0x0000003c, 0x0000005e, 0x000007de, 0x000007be},
             {0x0000001e, 0x0000000a, 0x0000001f, 0x0000005f, 0x000001ee, 0x000001f6},
             {0x000001fe, 0x000000fe, 0x000000f6, 0x000000fa, 0x0000007e, 0x00000016},
             {0x000007bf, 0x000003de, 0x000003ee, 0x0000007a, 0x0000000e, 0x00000006},
             {0x00003eff, 0x00001f7e, 0x000003ff, 0x0000002e, 0x00000004, 0x00000002}},
            {{3, 4, 7, 10, 12, 14},
             {3, 3, 6, 8, 11, 11},
             {6, 5, 6, 8, 9, 9},
             {9, 8, 8, 8, 7, 6},
             {11, 10, 10, 7, 5, 4},
             {14, 13, 10, 7, 4, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x0000000a, 0x0000001a, 0x000001be, 0x000006e6, 0x0000067a, 0x00000cf2, 0x000033de},
             {0x0000000c, 0x0000000e, 0x0000000e, 0x000000de, 0x00000372, 0x000003d6, 0x00000678, 0x00000cf6},
             {0x00000036, 0x00000012, 0x0000003e, 0x0000003c, 0x000001b8, 0x000003d4, 0x0000033e, 0x0000033f},
             {0x0000007e, 0x0000006a, 0x0000004e, 0x0000007e, 0x000001ba, 0x000000ce, 0x000000f6, 0x000001ee},
             {0x000001ef, 0x0000013e, 0x0000007f, 0x00000066, 0x000000d6, 0x0000003e, 0x000000d7, 0x0000009e},
             {0x000007ae, 0x000001e8, 0x000001e9, 0x0000027e, 0x00000032, 0x00000018, 0x00000026, 0x00000034},
             {0x00000cf3, 0x000007aa, 0x000007ab, 0x0000027f, 0x000001bf, 0x0000001b, 0x0000001e, 0x0000000b},
             {0x000033df, 0x000019ee, 0x000007af, 0x000006e7, 0x000001bb, 0x0000007f, 0x00000008, 0x00000000}},
            {{3, 4, 6, 9, 11, 12, 13, 15},
             {4, 4, 5, 8, 10, 11, 12, 13},
             {6, 5, 6, 7, 9, 11, 11, 11},
             {8, 7, 7, 7, 9, 9, 9, 10},
             {10, 9, 8, 8, 8, 7, 8, 8},
             {12, 10, 10, 10, 7, 6, 6, 6},
             {13, 12, 12, 10, 9, 6, 5, 4},
             {15, 14, 12, 11, 9, 7, 4, 2}},
            {0x00000000, 0}},
           {{{0x0000000e, 0x00000008, 0x0000007e, 0x000001fe, 0x000001ba, 0x00000dbe, 0x00000d7e, 0x00001af6, 0x00007fec, 0x0001ffb6},
             {0x0000000a, 0x0000000c, 0x0000000c, 0x00000036, 0x000000de, 0x000005fe, 0x000006be, 0x00001b7e, 0x00007fee, 0x00006dfe},
             {0x0000001e, 0x0000000e, 0x0000000a, 0x0000006a, 0x000001ae, 0x000006fe, 0x00000376, 0x00000dfe, 0x00000dff, 0x00000d7f},
             {0x000000b6, 0x0000005e, 0x0000007c, 0x0000006e, 0x0000006a, 0x0000016a, 0x00000ffe, 0x00000dfe, 0x00000ffc, 0x00001bfe},
             {0x0000035e, 0x000001b6, 0x0000005e, 0x000000b4, 0x0000006c, 0x0000017e, 0x0000036e, 0x000003ee, 0x0000037e, 0x00000377},
             {0x00000fff, 0x000001ae, 0x000001be, 0x000001f6, 0x000001be, 0x000000da, 0x000000fe, 0x0000016b, 0x000000d6, 0x0000037e},
             {0x000017fe, 0x00000bfe, 0x000007de, 0x000006de, 0x000001b8, 0x000000d6, 0x0000002e, 0x00000034, 0x000000de, 0x000000be},
             {0x00007fef, 0x000006bc, 0x00001bff, 0x00001ffa, 0x000001b9, 0x000003fe, 0x000000fa, 0x0000002e, 0x00000034, 0x0000001f},
             {0x00006dff, 0x00001af7, 0x000036fe, 0x000006fe, 0x00000fbe, 0x0000035f, 0x000000b7, 0x0000002c, 0x0000001e, 0x00000009},
             {0x0001ffb7, 0x0000ffda, 0x00000d7a, 0x000017ff, 0x00000fbf, 0x000002fe, 0x0000005f, 0x00000016, 0x00000004, 0x00000000}},
            {{4, 4, 7, 9, 10, 12, 13, 14, 15, 17},
             {4, 4, 5, 7, 9, 11, 12, 13, 15, 15},
             {6, 5, 5, 7, 9, 11, 11, 13, 13, 13},
             {8, 7, 7, 7, 8, 9, 12, 12, 12, 13},
             {10, 9, 8, 8, 7, 9, 10, 10, 11, 11},
             {12, 10, 10, 9, 9, 8, 8, 9, 9, 10},
             {13, 12, 11, 11, 10, 8, 7, 7, 8, 8},
             {15, 12, 13, 13, 10, 10, 8, 6, 6, 6},
             {15, 14, 14, 12, 12, 10, 8, 6, 5, 4},
             {17, 16, 13, 13, 12, 10, 8, 6, 4, 2}},
            {0x00000000, 0}}}}}};
