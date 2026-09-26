
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

#include "sac_huff_tab.h"

const HUFF_IPD_TABLE huffIPDTab =
    {

        {

            {{0x0000, 0x0006, 0x001e, 0x003a, 0x003b, 0x001c, 0x001f, 0x0002},
             {1, 3, 5, 6, 6, 5, 5, 2}},

            {{0x0000, 0x0002, 0x000e, 0x003e, 0x007e, 0x007f, 0x001e, 0x0006},
             {1, 2, 4, 6, 7, 7, 5, 3}}},
        {

            {

                {

                    {

                        {{0x00000000, 0x00000007},
                         {0x00000006, 0x00000002}},
                        {{1, 3},
                         {3, 2}},
                        {0x00000007, 3}},
                    {

                        {{0x00000000, 0x000000ff, 0x000000ff, 0x000000ff},
                         {0x000000ff, 0x00000006, 0x000000ff, 0x000000fe},
                         {0x0000007c, 0x0000007e, 0x0000000e, 0x00000002},
                         {0x0000007d, 0x000000ff, 0x000000ff, 0x0000001e}},
                        {{1, 8, 8, 8},
                         {8, 3, 8, 8},
                         {7, 7, 4, 2},
                         {7, 8, 8, 5}},
                        {0x000000ff, 8}},
                    {

                        {{0x00000000, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf},
                         {0x0000006e, 0x0000001e, 0x000001bf, 0x000001bf, 0x000001bf, 0x0000002a},
                         {0x0000007e, 0x000000fe, 0x00000036, 0x000001bf, 0x00000018, 0x00000014},
                         {0x0000002b, 0x0000002e, 0x0000001a, 0x00000019, 0x0000003e, 0x000001be},
                         {0x000000ff, 0x0000000e, 0x000001bf, 0x000001bf, 0x00000016, 0x0000002f},
                         {0x000000de, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf, 0x00000004}},
                        {{1, 9, 9, 9, 9, 9},
                         {7, 5, 9, 9, 9, 6},
                         {7, 8, 6, 9, 5, 5},
                         {6, 6, 5, 5, 6, 9},
                         {8, 4, 9, 9, 5, 6},
                         {8, 9, 9, 9, 9, 3}},
                        {0x000001bf, 9}},
                    {

                        {{0x00000000, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f},
                         {0x0000000c, 0x0000001c, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000001e},
                         {0x0000007e, 0x000000ae, 0x00000056, 0x0000057f, 0x0000057f, 0x0000057f, 0x000000ee, 0x0000017e},
                         {0x000002be, 0x000000ef, 0x0000001a, 0x0000001e, 0x0000057f, 0x0000001f, 0x0000001b, 0x000000ba},
                         {0x0000015e, 0x0000003e, 0x00000014, 0x00000002, 0x00000006, 0x00000016, 0x0000005e, 0x000000be},
                         {0x0000057e, 0x000000ec, 0x0000002a, 0x0000057f, 0x0000057f, 0x0000005c, 0x000000bb, 0x0000007f},
                         {0x0000017f, 0x000000ed, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000001c, 0x0000001d},
                         {0x0000003a, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x00000004}},
                        {{2, 11, 11, 11, 11, 11, 11, 11},
                         {5, 5, 11, 11, 11, 11, 11, 6},
                         {8, 8, 7, 11, 11, 11, 8, 9},
                         {10, 8, 6, 5, 11, 5, 6, 8},
                         {9, 7, 5, 3, 3, 5, 7, 8},
                         {11, 8, 6, 11, 11, 7, 8, 8},
                         {9, 8, 11, 11, 11, 11, 6, 6},
                         {6, 11, 11, 11, 11, 11, 11, 3}},
                        {0x0000057f, 11}},
                },
                {

                    {

                        {{0x00000000, 0x00000007},
                         {0x00000006, 0x00000002}},
                        {{1, 3},
                         {3, 2}},
                        {0x00000007, 3}},
                    {

                        {{0x00000000, 0x000000ff, 0x000000ff, 0x000000ff},
                         {0x000000ff, 0x00000006, 0x000000ff, 0x000000fe},
                         {0x0000007c, 0x0000007e, 0x0000000e, 0x00000002},
                         {0x0000007d, 0x000000ff, 0x000000ff, 0x0000001e}},
                        {{1, 8, 8, 8},
                         {8, 3, 8, 8},
                         {7, 7, 4, 2},
                         {7, 8, 8, 5}},
                        {0x000000ff, 8}},
                    {

                        {{0x00000000, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf},
                         {0x0000006e, 0x0000001e, 0x000001bf, 0x000001bf, 0x000001bf, 0x0000002a},
                         {0x0000007e, 0x000000fe, 0x00000036, 0x000001bf, 0x00000018, 0x00000014},
                         {0x0000002b, 0x0000002e, 0x0000001a, 0x00000019, 0x0000003e, 0x000001be},
                         {0x000000ff, 0x0000000e, 0x000001bf, 0x000001bf, 0x00000016, 0x0000002f},
                         {0x000000de, 0x000001bf, 0x000001bf, 0x000001bf, 0x000001bf, 0x00000004}},
                        {{1, 9, 9, 9, 9, 9},
                         {7, 5, 9, 9, 9, 6},
                         {7, 8, 6, 9, 5, 5},
                         {6, 6, 5, 5, 6, 9},
                         {8, 4, 9, 9, 5, 6},
                         {8, 9, 9, 9, 9, 3}},
                        {0x000001bf, 9}},
                    {

                        {{0x00000000, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f},
                         {0x0000000c, 0x0000001c, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000001e},
                         {0x0000007e, 0x000000ae, 0x00000056, 0x0000057f, 0x0000057f, 0x0000057f, 0x000000ee, 0x0000017e},
                         {0x000002be, 0x000000ef, 0x0000001a, 0x0000001e, 0x0000057f, 0x0000001f, 0x0000001b, 0x000000ba},
                         {0x0000015e, 0x0000003e, 0x00000014, 0x00000002, 0x00000006, 0x00000016, 0x0000005e, 0x000000be},
                         {0x0000057e, 0x000000ec, 0x0000002a, 0x0000057f, 0x0000057f, 0x0000005c, 0x000000bb, 0x0000007f},
                         {0x0000017f, 0x000000ed, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000001c, 0x0000001d},
                         {0x0000003a, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x0000057f, 0x00000004}},
                        {{2, 11, 11, 11, 11, 11, 11, 11},
                         {5, 5, 11, 11, 11, 11, 11, 6},
                         {8, 8, 7, 11, 11, 11, 8, 9},
                         {10, 8, 6, 5, 11, 5, 6, 8},
                         {9, 7, 5, 3, 3, 5, 7, 8},
                         {11, 8, 6, 11, 11, 7, 8, 8},
                         {9, 8, 11, 11, 11, 11, 6, 6},
                         {6, 11, 11, 11, 11, 11, 11, 3}},
                        {0x0000057f, 11}},
                }},
            {

                {

                    {

                        {{0x00000000, 0x00000007},
                         {0x00000006, 0x00000002}},
                        {{1, 3},
                         {3, 2}},
                        {0x00000007, 3}},
                    {

                        {{0x00000000, 0x000000bf, 0x000000bf, 0x000000bf},
                         {0x00000016, 0x00000006, 0x000000bf, 0x0000005e},
                         {0x0000002e, 0x0000000e, 0x0000000a, 0x0000000f},
                         {0x000000be, 0x000000bf, 0x000000bf, 0x00000004}},
                        {{1, 8, 8, 8},
                         {5, 3, 8, 7},
                         {6, 4, 4, 4},
                         {8, 8, 8, 3}},
                        {0x000000bf, 8}},
                    {

                        {{0x00000000, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff},
                         {0x00000016, 0x0000005e, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005fe},
                         {0x000002fc, 0x000000ba, 0x0000000e, 0x000005ff, 0x0000003e, 0x0000007e},
                         {0x000002fd, 0x000002fe, 0x0000005c, 0x00000006, 0x0000000a, 0x0000017c},
                         {0x0000017d, 0x0000007f, 0x000005ff, 0x000005ff, 0x000000bb, 0x0000001e},
                         {0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x00000004}},
                        {{1, 11, 11, 11, 11, 11},
                         {5, 7, 11, 11, 11, 11},
                         {10, 8, 4, 11, 6, 7},
                         {10, 10, 7, 3, 4, 9},
                         {9, 7, 11, 11, 8, 5},
                         {11, 11, 11, 11, 11, 3}},
                        {0x000005ff, 11}},
                    {

                        {{0x00000000, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf},
                         {0x0000003e, 0x00000030, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00000032},
                         {0x0000019a, 0x000001fc, 0x000001fe, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x000001f8, 0x000001fa},
                         {0x00001fbe, 0x0000067e, 0x0000019b, 0x00000036, 0x00001fbf, 0x00000062, 0x0000033c, 0x000007fe},
                         {0x00000fde, 0x0000067f, 0x000001f9, 0x0000001a, 0x00000002, 0x00000063, 0x000007ff, 0x000007ee},
                         {0x0000033d, 0x0000033e, 0x000000cc, 0x00001fbf, 0x00001fbf, 0x000000de, 0x000003fe, 0x000003f6},
                         {0x000000df, 0x000001fd, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x000000ce, 0x0000006e},
                         {0x0000001e, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x0000000e}},
                        {{1, 13, 13, 13, 13, 13, 13, 13},
                         {6, 6, 13, 13, 13, 13, 13, 6},
                         {9, 9, 9, 13, 13, 13, 9, 9},
                         {13, 11, 9, 6, 13, 7, 10, 11},
                         {12, 11, 9, 5, 2, 7, 11, 11},
                         {10, 10, 8, 13, 13, 8, 10, 10},
                         {8, 9, 13, 13, 13, 13, 8, 7},
                         {5, 13, 13, 13, 13, 13, 13, 4}},
                        {0x00001fbf, 13}},
                },
                {

                    {

                        {{0x00000000, 0x00000007},
                         {0x00000006, 0x00000002}},
                        {{1, 3},
                         {3, 2}},
                        {0x00000007, 3}},
                    {

                        {{0x00000000, 0x000000bf, 0x000000bf, 0x000000bf},
                         {0x00000016, 0x00000006, 0x000000bf, 0x0000005e},
                         {0x0000002e, 0x0000000e, 0x0000000a, 0x0000000f},
                         {0x000000be, 0x000000bf, 0x000000bf, 0x00000004}},
                        {{1, 8, 8, 8},
                         {5, 3, 8, 7},
                         {6, 4, 4, 4},
                         {8, 8, 8, 3}},
                        {0x000000bf, 8}},
                    {

                        {{0x00000000, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff},
                         {0x00000016, 0x0000005e, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005fe},
                         {0x000002fc, 0x000000ba, 0x0000000e, 0x000005ff, 0x0000003e, 0x0000007e},
                         {0x000002fd, 0x000002fe, 0x0000005c, 0x00000006, 0x0000000a, 0x0000017c},
                         {0x0000017d, 0x0000007f, 0x000005ff, 0x000005ff, 0x000000bb, 0x0000001e},
                         {0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x000005ff, 0x00000004}},
                        {{1, 11, 11, 11, 11, 11},
                         {5, 7, 11, 11, 11, 11},
                         {10, 8, 4, 11, 6, 7},
                         {10, 10, 7, 3, 4, 9},
                         {9, 7, 11, 11, 8, 5},
                         {11, 11, 11, 11, 11, 3}},
                        {0x000005ff, 11}},
                    {

                        {{0x00000000, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf},
                         {0x0000003e, 0x00000030, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00000032},
                         {0x0000019a, 0x000001fc, 0x000001fe, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x000001f8, 0x000001fa},
                         {0x00001fbe, 0x0000067e, 0x0000019b, 0x00000036, 0x00001fbf, 0x00000062, 0x0000033c, 0x000007fe},
                         {0x00000fde, 0x0000067f, 0x000001f9, 0x0000001a, 0x00000002, 0x00000063, 0x000007ff, 0x000007ee},
                         {0x0000033d, 0x0000033e, 0x000000cc, 0x00001fbf, 0x00001fbf, 0x000000de, 0x000003fe, 0x000003f6},
                         {0x000000df, 0x000001fd, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x000000ce, 0x0000006e},
                         {0x0000001e, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x00001fbf, 0x0000000e}},
                        {{1, 13, 13, 13, 13, 13, 13, 13},
                         {6, 6, 13, 13, 13, 13, 13, 6},
                         {9, 9, 9, 13, 13, 13, 9, 9},
                         {13, 11, 9, 6, 13, 7, 10, 11},
                         {12, 11, 9, 5, 2, 7, 11, 11},
                         {10, 10, 8, 13, 13, 8, 10, 10},
                         {8, 9, 13, 13, 13, 13, 8, 7},
                         {5, 13, 13, 13, 13, 13, 13, 4}},
                        {0x00001fbf, 13}},
                }}}};
