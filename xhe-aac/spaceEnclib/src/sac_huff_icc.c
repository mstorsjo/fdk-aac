
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

#include "spaceEnclib_const.h"
#include "sac_huff_tab.h"

const HUFF_ICC_TABLE huffICCTab =
    {
        {{{0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x007f},
          {1, 2, 3, 4, 5, 6, 7, 7}},
         {{0x0000, 0x0002, 0x0006, 0x000e, 0x001e, 0x003e, 0x007e, 0x007f},
          {1, 2, 3, 4, 5, 6, 7, 7}}},
        {{{{{{0x00000000, 0x00000006},
             {0x00000007, 0x00000002}},
            {{1, 3},
             {3, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x00000000, 0x0000000a, 0x0000007e},
             {0x0000000e, 0x00000004, 0x00000016, 0x000003fe},
             {0x000001fe, 0x000000fe, 0x0000003e, 0x0000001e},
             {0x000003ff, 0x00000017, 0x00000006, 0x00000003}},
            {{2, 2, 5, 8},
             {5, 4, 6, 11},
             {10, 9, 7, 6},
             {11, 6, 4, 2}},
            {0x00000000, 0}},
           {{{0x00000000, 0x00000002, 0x0000000c, 0x0000006a, 0x000000dc, 0x000006ee},
             {0x0000001e, 0x0000000c, 0x0000000d, 0x0000001e, 0x000001ae, 0x0000ddff},
             {0x000000de, 0x0000007e, 0x0000001f, 0x000001be, 0x00006efe, 0x0000ddfe},
             {0x0000377e, 0x00001bbe, 0x00000dde, 0x000001bf, 0x000000d6, 0x00000376},
             {0x0000ddff, 0x0000ddff, 0x000001ba, 0x00000034, 0x0000003e, 0x0000000e},
             {0x0000ddff, 0x000001af, 0x0000007f, 0x00000036, 0x0000000e, 0x00000002}},
            {{2, 3, 5, 7, 8, 11},
             {5, 4, 5, 6, 9, 16},
             {8, 7, 6, 9, 15, 16},
             {14, 13, 12, 9, 8, 10},
             {16, 16, 9, 6, 6, 5},
             {16, 9, 7, 6, 4, 2}},
            {0x0000ddff, 16}},
           {{{0x00000000, 0x0000000c, 0x0000002e, 0x00000044, 0x00000086, 0x0000069e, 0x0000043e, 0x0000087a},
             {0x0000001e, 0x0000000e, 0x0000002a, 0x00000046, 0x0000015e, 0x00000047, 0x0000034a, 0x0000087b},
             {0x000000d6, 0x00000026, 0x0000002f, 0x000000d7, 0x0000006a, 0x0000034e, 0x0000087b, 0x0000087b},
             {0x000002be, 0x000001a6, 0x000001be, 0x00000012, 0x000001bf, 0x0000087b, 0x0000087b, 0x0000087b},
             {0x0000087b, 0x0000087b, 0x0000087b, 0x0000087b, 0x00000036, 0x000000d0, 0x0000043c, 0x0000043f},
             {0x0000087b, 0x0000087b, 0x0000087b, 0x0000034b, 0x00000027, 0x00000020, 0x00000042, 0x000000d1},
             {0x0000087b, 0x0000087b, 0x000002bf, 0x000000de, 0x000000ae, 0x00000056, 0x00000016, 0x00000014},
             {0x0000087b, 0x0000069f, 0x000001a4, 0x0000010e, 0x00000045, 0x0000006e, 0x0000001f, 0x00000001}},
            {{2, 4, 6, 7, 8, 11, 11, 12},
             {5, 4, 6, 7, 9, 7, 10, 12},
             {8, 6, 6, 8, 7, 10, 12, 12},
             {10, 9, 9, 5, 9, 12, 12, 12},
             {12, 12, 12, 12, 6, 8, 11, 11},
             {12, 12, 12, 10, 6, 6, 7, 8},
             {12, 12, 10, 8, 8, 7, 5, 5},
             {12, 11, 9, 9, 7, 7, 5, 2}},
            {0x0000087b, 12}}},
          {{{{0x00000000, 0x00000006},
             {0x00000007, 0x00000002}},
            {{1, 3},
             {3, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x00000004, 0x0000017e, 0x000002fe},
             {0x00000000, 0x0000000e, 0x000000be, 0x00000016},
             {0x0000000f, 0x00000014, 0x0000005e, 0x00000006},
             {0x0000002e, 0x000002ff, 0x00000015, 0x00000003}},
            {{2, 4, 10, 11},
             {2, 5, 9, 6},
             {5, 6, 8, 4},
             {7, 11, 6, 2}},
            {0x00000000, 0}},
           {{{0x00000000, 0x0000001e, 0x000003fc, 0x0000fffa, 0x000fff9e, 0x000fff9f},
             {0x00000006, 0x00000004, 0x000000be, 0x00007ffe, 0x0007ffce, 0x000000fe},
             {0x00000006, 0x0000001e, 0x000003fd, 0x0000fffb, 0x00000ffe, 0x0000003e},
             {0x0000000a, 0x0000007e, 0x00001ffe, 0x00007fff, 0x0000005e, 0x0000000e},
             {0x0000001f, 0x000003fe, 0x0001fff2, 0x00000ffc, 0x0000002e, 0x0000000e},
             {0x000000bf, 0x0003ffe6, 0x0000fff8, 0x00000ffd, 0x00000016, 0x00000002}},
            {{2, 5, 10, 16, 20, 20},
             {3, 4, 9, 15, 19, 8},
             {4, 6, 10, 16, 12, 6},
             {5, 7, 13, 15, 8, 5},
             {6, 10, 17, 12, 7, 4},
             {9, 18, 16, 12, 6, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x0000001e, 0x00000ffe, 0x0000ffff, 0x0000fffe, 0x0000ffff, 0x0000ffff, 0x0000ffff},
             {0x00000006, 0x00000008, 0x000007fe, 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000005a},
             {0x00000006, 0x0000007a, 0x00000164, 0x00007ffa, 0x0000ffff, 0x0000ffff, 0x00001fee, 0x0000003c},
             {0x0000000e, 0x000000fe, 0x000002ce, 0x000002cf, 0x00007ffb, 0x00001fec, 0x000000b0, 0x0000002e},
             {0x0000003e, 0x000003fe, 0x00000165, 0x00007ffc, 0x00001fef, 0x000007fa, 0x000007f8, 0x0000001f},
             {0x0000002f, 0x000000f6, 0x00001fed, 0x0000ffff, 0x00007ffd, 0x00000ff2, 0x000000b1, 0x0000000a},
             {0x00000009, 0x00000166, 0x0000ffff, 0x0000ffff, 0x00007ffe, 0x00003ffc, 0x0000005b, 0x0000000e},
             {0x0000007e, 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x00000ff3, 0x000000f7, 0x00000000}},
            {{2, 6, 12, 16, 16, 16, 16, 16},
             {3, 5, 11, 16, 16, 16, 16, 8},
             {4, 7, 10, 15, 16, 16, 13, 6},
             {5, 8, 11, 11, 15, 13, 9, 7},
             {6, 10, 10, 15, 13, 11, 11, 6},
             {7, 8, 13, 16, 15, 12, 9, 5},
             {5, 10, 16, 16, 15, 14, 8, 4},
             {7, 16, 16, 16, 16, 12, 8, 2}},
            {0x0000ffff, 16}}}},
         {{{{{0x00000000, 0x00000006},
             {0x00000007, 0x00000002}},
            {{1, 3},
             {3, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x0000000e, 0x0000037e, 0x00000dfe},
             {0x0000000f, 0x0000000c, 0x000001ba, 0x000001bb},
             {0x000000de, 0x000000dc, 0x000001be, 0x0000001a},
             {0x000006fe, 0x00000dff, 0x00000036, 0x00000000}},
            {{2, 4, 10, 12},
             {4, 4, 9, 9},
             {8, 8, 9, 5},
             {11, 12, 6, 1}},
            {0x00000000, 0}},
           {{{0x00000000, 0x0000000c, 0x000001b6, 0x00001b7c, 0x0000dbfe, 0x00036fff},
             {0x0000000e, 0x0000001e, 0x000001be, 0x00000dfe, 0x00036ffe, 0x0000036e},
             {0x0000006e, 0x000000fe, 0x000000d8, 0x000036fe, 0x000006de, 0x000000de},
             {0x000001fa, 0x000000da, 0x00000dff, 0x00001b7e, 0x000000d9, 0x000000ff},
             {0x000003f6, 0x000006fe, 0x00006dfe, 0x0000037e, 0x000000fc, 0x0000001a},
             {0x000007ee, 0x0001b7fe, 0x00001b7d, 0x000007ef, 0x0000003e, 0x00000002}},
            {{1, 4, 9, 13, 16, 18},
             {4, 5, 9, 12, 18, 10},
             {7, 8, 8, 14, 11, 8},
             {9, 8, 12, 13, 8, 8},
             {10, 11, 15, 10, 8, 5},
             {11, 17, 13, 11, 6, 2}},
            {0x00036fff, 18}},
           {{{0x00000000, 0x0000000c, 0x000007ee, 0x00001e7e, 0x00003cfe, 0x000079ff, 0x000079ff, 0x000079ff},
             {0x0000000e, 0x0000001a, 0x000001e6, 0x00001fbe, 0x000079fe, 0x000079ff, 0x000079ff, 0x000006fc},
             {0x0000006c, 0x000000f6, 0x000001ba, 0x00000dfc, 0x00000dfd, 0x000079ff, 0x00000f3e, 0x000001bb},
             {0x000000dc, 0x000001fe, 0x0000036e, 0x000003fe, 0x000079ff, 0x00000fde, 0x000001ee, 0x000000f2},
             {0x000001fa, 0x000003f6, 0x000001be, 0x000079ff, 0x00001fbf, 0x000003ce, 0x000003ff, 0x000000de},
             {0x00000078, 0x000000da, 0x000079ff, 0x000079ff, 0x000006fd, 0x0000036c, 0x000001ef, 0x000000fe},
             {0x0000036f, 0x00000dfe, 0x000079ff, 0x000079ff, 0x000079ff, 0x0000036d, 0x000000fc, 0x0000003e},
             {0x00000dff, 0x000079ff, 0x000079ff, 0x000079ff, 0x000079ff, 0x0000079e, 0x0000007a, 0x00000002}},
            {{1, 4, 11, 13, 14, 15, 15, 15},
             {4, 5, 9, 13, 15, 15, 15, 11},
             {7, 8, 9, 12, 12, 15, 12, 9},
             {8, 9, 10, 10, 15, 12, 9, 8},
             {9, 10, 9, 15, 13, 10, 10, 8},
             {7, 8, 15, 15, 11, 10, 9, 8},
             {10, 12, 15, 15, 15, 10, 8, 6},
             {12, 15, 15, 15, 15, 11, 7, 2}},
            {0x000079ff, 15}}},
          {{{{0x00000000, 0x00000006},
             {0x00000007, 0x00000002}},
            {{1, 3},
             {3, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x0000000e, 0x000000fc, 0x00000fde},
             {0x0000000c, 0x0000000d, 0x000001fe, 0x000007ee},
             {0x000001fa, 0x000001ff, 0x000000fe, 0x0000003e},
             {0x00000fdf, 0x000003f6, 0x0000001e, 0x00000000}},
            {{2, 4, 8, 12},
             {4, 4, 9, 11},
             {9, 9, 8, 6},
             {12, 10, 5, 1}},
            {0x00000000, 0}},
           {{{0x00000000, 0x0000000e, 0x0000003a, 0x00000676, 0x000019fe, 0x0000cebe},
             {0x0000000f, 0x00000002, 0x0000001e, 0x000000fe, 0x000019d6, 0x0000675e},
             {0x0000003e, 0x00000032, 0x00000018, 0x0000033e, 0x00000cfe, 0x00000677},
             {0x00000674, 0x0000019c, 0x000000ff, 0x0000003b, 0x0000001c, 0x0000007e},
             {0x000033fe, 0x000033ff, 0x00000cea, 0x00000066, 0x0000001a, 0x00000006},
             {0x0000cebf, 0x000033ae, 0x0000067e, 0x0000019e, 0x0000001b, 0x00000002}},
            {{2, 4, 7, 11, 13, 16},
             {4, 3, 6, 9, 13, 15},
             {7, 6, 5, 10, 12, 11},
             {11, 9, 9, 7, 6, 8},
             {14, 14, 12, 7, 5, 4},
             {16, 14, 11, 9, 5, 2}},
            {0x00000000, 0}},
           {{{0x00000002, 0x00000002, 0x000000fe, 0x000007be, 0x00000ffc, 0x00000ffd, 0x00001efe, 0x00003dfe},
             {0x00000004, 0x00000000, 0x0000003c, 0x000000f6, 0x000001da, 0x000003fe, 0x00003dfe, 0x00003dff},
             {0x0000003c, 0x0000003e, 0x0000000a, 0x0000003a, 0x000003de, 0x000007be, 0x00000f7e, 0x00001efe},
             {0x000001de, 0x000000ec, 0x0000007e, 0x0000000c, 0x000001ee, 0x00000f7e, 0x000007fc, 0x00003dff},
             {0x00007ffe, 0x000003be, 0x000000fe, 0x000001fe, 0x0000001a, 0x0000001c, 0x000007fd, 0x00000ffe},
             {0x00003dff, 0x000003bf, 0x00001ffe, 0x000003ff, 0x0000003e, 0x0000001b, 0x0000007e, 0x000000f6},
             {0x00007fff, 0x00003dff, 0x00003ffe, 0x000001db, 0x000000ee, 0x0000007a, 0x0000000e, 0x0000000b},
             {0x00003dff, 0x00003dff, 0x000003de, 0x000001fe, 0x000001ee, 0x0000007a, 0x00000006, 0x00000003}},
            {{2, 4, 9, 12, 13, 13, 15, 16},
             {4, 3, 7, 10, 11, 12, 15, 16},
             {8, 7, 5, 8, 11, 13, 14, 14},
             {11, 10, 9, 5, 10, 13, 12, 15},
             {16, 12, 10, 10, 6, 7, 12, 13},
             {16, 12, 14, 12, 8, 6, 8, 9},
             {16, 16, 15, 11, 10, 8, 5, 5},
             {16, 16, 12, 11, 11, 9, 5, 2}},
            {0x00003dff, 16}}}}}};
