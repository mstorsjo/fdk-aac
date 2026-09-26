
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

#ifndef CODEBOOK_H
#define CODEBOOK_H

#include "bit_buf.h"

enum codeBookNo {
  CODE_BOOK_ZERO_NO = 0,
  CODE_BOOK_1_NO = 1,
  CODE_BOOK_2_NO = 2,
  CODE_BOOK_3_NO = 3,
  CODE_BOOK_4_NO = 4,
  CODE_BOOK_5_NO = 5,
  CODE_BOOK_6_NO = 6,
  CODE_BOOK_7_NO = 7,
  CODE_BOOK_8_NO = 8,
  CODE_BOOK_9_NO = 9,
  CODE_BOOK_10_NO = 10,
  CODE_BOOK_ESC_NO = 11,
  CODE_BOOK_RES_NO = 12,
  CODE_BOOK_PNS_NO = 13,
  CODE_BOOK_IS_OUT_OF_PHASE_NO = 14,
  CODE_BOOK_IS_IN_PHASE_NO = 15
};

enum codeBookNdx {
  CODE_BOOK_ZERO_NDX,
  CODE_BOOK_1_NDX,
  CODE_BOOK_2_NDX,
  CODE_BOOK_3_NDX,
  CODE_BOOK_4_NDX,
  CODE_BOOK_5_NDX,
  CODE_BOOK_6_NDX,
  CODE_BOOK_7_NDX,
  CODE_BOOK_8_NDX,
  CODE_BOOK_9_NDX,
  CODE_BOOK_10_NDX,
  CODE_BOOK_ESC_NDX,
  CODE_BOOK_PNS_NDX,
  CODE_BOOK_IS_OUT_OF_PHASE_NDX,
  CODE_BOOK_IS_IN_PHASE_NDX,
  NUMBER_OF_CODE_BOOKS
};

enum codeBookLav {
  CODE_BOOK_ZERO_LAV = 0,
  CODE_BOOK_1_LAV = 1,
  CODE_BOOK_2_LAV = 1,
  CODE_BOOK_3_LAV = 2,
  CODE_BOOK_4_LAV = 2,
  CODE_BOOK_5_LAV = 4,
  CODE_BOOK_6_LAV = 4,
  CODE_BOOK_7_LAV = 7,
  CODE_BOOK_8_LAV = 7,
  CODE_BOOK_9_LAV = 12,
  CODE_BOOK_10_LAV = 12,
  CODE_BOOK_ESC_LAV = 16,
  CODE_BOOK_PNS_LAV = 60,
  CODE_BOOK_IS_OUT_OF_PHASE_LAV = 0,
  CODE_BOOK_IS_IN_PHASE_LAV = 0,
  CODE_BOOK_SCF_LAV = 60,
  CODE_BOOK_RVLC_LAV = 7
};

extern const int huff_ltab1[3][3][3][3];
extern const int huff_ltab2[3][3][3][3];
extern const int huff_ltab3[3][3][3][3];
extern const int huff_ltab4[3][3][3][3];
extern const int huff_ltab5[9][9];
extern const int huff_ltab6[9][9];
extern const int huff_ltab7[8][8];
extern const int huff_ltab8[8][8];
extern const int huff_ltab9[13][13];
extern const int huff_ltab10[13][13];
extern const int huff_ltab11[17][17];
extern const int huff_ltabscf[121];

extern const int huff_ltabrvlc[15];
extern const int huff_ltabesc[54];

extern const int huff_ctab1[3][3][3][3];
extern const int huff_ctab2[3][3][3][3];
extern const int huff_ctab3[3][3][3][3];
extern const int huff_ctab4[3][3][3][3];
extern const int huff_ctab5[9][9];
extern const int huff_ctab6[9][9];
extern const int huff_ctab7[8][8];
extern const int huff_ctab8[8][8];
extern const int huff_ctab9[13][13];
extern const int huff_ctab10[13][13];
extern const int huff_ctab11[17][17];
extern const int huff_ctabscf[121];

extern const int huff_ctabrvlc[15];
extern const int huff_ctabesc[54];

#endif
