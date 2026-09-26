
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

#ifndef INCLUDED_FROM_IISFFT_C
#error "this file must not be included"
#endif

static void fft240(float* in) {
  const int table1[240] = {
      0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 225, 1, 17, 33, 49, 65, 81,
      97, 113, 129, 145, 161, 177, 193, 209, 210, 226, 2, 18, 34, 50, 66, 82, 98, 114, 130, 146, 162, 178,
      194, 195, 211, 227, 3, 19, 35, 51, 67, 83, 99, 115, 131, 147, 163, 179, 180, 196, 212, 228, 4, 20,
      36, 52, 68, 84, 100, 116, 132, 148, 164, 165, 181, 197, 213, 229, 5, 21, 37, 53, 69, 85, 101, 117,
      133, 149, 150, 166, 182, 198, 214, 230, 6, 22, 38, 54, 70, 86, 102, 118, 134, 135, 151, 167, 183, 199,
      215, 231, 7, 23, 39, 55, 71, 87, 103, 119, 120, 136, 152, 168, 184, 200, 216, 232, 8, 24, 40, 56,
      72, 88, 104, 105, 121, 137, 153, 169, 185, 201, 217, 233, 9, 25, 41, 57, 73, 89, 90, 106, 122, 138,
      154, 170, 186, 202, 218, 234, 10, 26, 42, 58, 74, 75, 91, 107, 123, 139, 155, 171, 187, 203, 219, 235,
      11, 27, 43, 59, 60, 76, 92, 108, 124, 140, 156, 172, 188, 204, 220, 236, 12, 28, 44, 45, 61, 77,
      93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 13, 29, 30, 46, 62, 78, 94, 110, 126, 142, 158, 174,
      190, 206, 222, 238, 14, 15, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 207, 223, 239};
  const int table2[240] = {
      0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 15, 31, 47, 63, 79, 95, 111,
      127, 143, 159, 175, 191, 207, 223, 239, 30, 46, 62, 78, 94, 110, 126, 142, 158, 174, 190, 206, 222, 238,
      14, 45, 61, 77, 93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 13, 29, 60, 76, 92, 108, 124, 140,
      156, 172, 188, 204, 220, 236, 12, 28, 44, 75, 91, 107, 123, 139, 155, 171, 187, 203, 219, 235, 11, 27,
      43, 59, 90, 106, 122, 138, 154, 170, 186, 202, 218, 234, 10, 26, 42, 58, 74, 105, 121, 137, 153, 169,
      185, 201, 217, 233, 9, 25, 41, 57, 73, 89, 120, 136, 152, 168, 184, 200, 216, 232, 8, 24, 40, 56,
      72, 88, 104, 135, 151, 167, 183, 199, 215, 231, 7, 23, 39, 55, 71, 87, 103, 119, 150, 166, 182, 198,
      214, 230, 6, 22, 38, 54, 70, 86, 102, 118, 134, 165, 181, 197, 213, 229, 5, 21, 37, 53, 69, 85,
      101, 117, 133, 149, 180, 196, 212, 228, 4, 20, 36, 52, 68, 84, 100, 116, 132, 148, 164, 195, 211, 227,
      3, 19, 35, 51, 67, 83, 99, 115, 131, 147, 163, 179, 210, 226, 2, 18, 34, 50, 66, 82, 98, 114,
      130, 146, 162, 178, 194, 225, 1, 17, 33, 49, 65, 81, 97, 113, 129, 145, 161, 177, 193, 209};

  const int L = 240;
  const int A = 15;
  const int B = 16;
  const int* idx1 = table1;
  const int* idx2 = table2;

  int k, l;
  ALIGN_16_BYTE float temp[32], out[480];

  for (k = 0; k < A; k++) {
    for (l = 0; l < B; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1 + 1];
      idx1 += A;
    }

    fft16(temp);
    idx1 -= L;

    for (l = 0; l < B; l++) {
      in[2 * *idx1] = temp[2 * l];
      in[2 * *idx1 + 1] = temp[2 * l + 1];
      idx1 += A;
    }

    idx1 -= L - 1;
  }

  idx1 -= A;

  for (k = 0; k < B; k++) {
    for (l = 0; l < A; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1++ + 1];
    }

    fft15(temp);

    for (l = 0; l < A; l++) {
      out[2 * *idx2] = temp[2 * l];
      out[2 * *idx2++ + 1] = temp[2 * l + 1];
    }
  }

  memmove(in, out, 2 * L * sizeof(float));
}

static void fft480(float* in) {
  const int table1[480] = {
      0, 256, 32, 288, 64, 320, 96, 352, 128, 384, 160, 416, 192, 448, 224, 225, 1, 257, 33, 289, 65, 321,
      97, 353, 129, 385, 161, 417, 193, 449, 450, 226, 2, 258, 34, 290, 66, 322, 98, 354, 130, 386, 162, 418,
      194, 195, 451, 227, 3, 259, 35, 291, 67, 323, 99, 355, 131, 387, 163, 419, 420, 196, 452, 228, 4, 260,
      36, 292, 68, 324, 100, 356, 132, 388, 164, 165, 421, 197, 453, 229, 5, 261, 37, 293, 69, 325, 101, 357,
      133, 389, 390, 166, 422, 198, 454, 230, 6, 262, 38, 294, 70, 326, 102, 358, 134, 135, 391, 167, 423, 199,
      455, 231, 7, 263, 39, 295, 71, 327, 103, 359, 360, 136, 392, 168, 424, 200, 456, 232, 8, 264, 40, 296,
      72, 328, 104, 105, 361, 137, 393, 169, 425, 201, 457, 233, 9, 265, 41, 297, 73, 329, 330, 106, 362, 138,
      394, 170, 426, 202, 458, 234, 10, 266, 42, 298, 74, 75, 331, 107, 363, 139, 395, 171, 427, 203, 459, 235,
      11, 267, 43, 299, 300, 76, 332, 108, 364, 140, 396, 172, 428, 204, 460, 236, 12, 268, 44, 45, 301, 77,
      333, 109, 365, 141, 397, 173, 429, 205, 461, 237, 13, 269, 270, 46, 302, 78, 334, 110, 366, 142, 398, 174,
      430, 206, 462, 238, 14, 15, 271, 47, 303, 79, 335, 111, 367, 143, 399, 175, 431, 207, 463, 239, 240, 16,
      272, 48, 304, 80, 336, 112, 368, 144, 400, 176, 432, 208, 464, 465, 241, 17, 273, 49, 305, 81, 337, 113,
      369, 145, 401, 177, 433, 209, 210, 466, 242, 18, 274, 50, 306, 82, 338, 114, 370, 146, 402, 178, 434, 435,
      211, 467, 243, 19, 275, 51, 307, 83, 339, 115, 371, 147, 403, 179, 180, 436, 212, 468, 244, 20, 276, 52,
      308, 84, 340, 116, 372, 148, 404, 405, 181, 437, 213, 469, 245, 21, 277, 53, 309, 85, 341, 117, 373, 149,
      150, 406, 182, 438, 214, 470, 246, 22, 278, 54, 310, 86, 342, 118, 374, 375, 151, 407, 183, 439, 215, 471,
      247, 23, 279, 55, 311, 87, 343, 119, 120, 376, 152, 408, 184, 440, 216, 472, 248, 24, 280, 56, 312, 88,
      344, 345, 121, 377, 153, 409, 185, 441, 217, 473, 249, 25, 281, 57, 313, 89, 90, 346, 122, 378, 154, 410,
      186, 442, 218, 474, 250, 26, 282, 58, 314, 315, 91, 347, 123, 379, 155, 411, 187, 443, 219, 475, 251, 27,
      283, 59, 60, 316, 92, 348, 124, 380, 156, 412, 188, 444, 220, 476, 252, 28, 284, 285, 61, 317, 93, 349,
      125, 381, 157, 413, 189, 445, 221, 477, 253, 29, 30, 286, 62, 318, 94, 350, 126, 382, 158, 414, 190, 446,
      222, 478, 254, 255, 31, 287, 63, 319, 95, 351, 127, 383, 159, 415, 191, 447, 223, 479};
  const int table2[480] = {
      0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 15, 47, 79, 111, 143, 175, 207,
      239, 271, 303, 335, 367, 399, 431, 463, 30, 62, 94, 126, 158, 190, 222, 254, 286, 318, 350, 382, 414, 446,
      478, 45, 77, 109, 141, 173, 205, 237, 269, 301, 333, 365, 397, 429, 461, 13, 60, 92, 124, 156, 188, 220,
      252, 284, 316, 348, 380, 412, 444, 476, 28, 75, 107, 139, 171, 203, 235, 267, 299, 331, 363, 395, 427, 459,
      11, 43, 90, 122, 154, 186, 218, 250, 282, 314, 346, 378, 410, 442, 474, 26, 58, 105, 137, 169, 201, 233,
      265, 297, 329, 361, 393, 425, 457, 9, 41, 73, 120, 152, 184, 216, 248, 280, 312, 344, 376, 408, 440, 472,
      24, 56, 88, 135, 167, 199, 231, 263, 295, 327, 359, 391, 423, 455, 7, 39, 71, 103, 150, 182, 214, 246,
      278, 310, 342, 374, 406, 438, 470, 22, 54, 86, 118, 165, 197, 229, 261, 293, 325, 357, 389, 421, 453, 5,
      37, 69, 101, 133, 180, 212, 244, 276, 308, 340, 372, 404, 436, 468, 20, 52, 84, 116, 148, 195, 227, 259,
      291, 323, 355, 387, 419, 451, 3, 35, 67, 99, 131, 163, 210, 242, 274, 306, 338, 370, 402, 434, 466, 18,
      50, 82, 114, 146, 178, 225, 257, 289, 321, 353, 385, 417, 449, 1, 33, 65, 97, 129, 161, 193, 240, 272,
      304, 336, 368, 400, 432, 464, 16, 48, 80, 112, 144, 176, 208, 255, 287, 319, 351, 383, 415, 447, 479, 31,
      63, 95, 127, 159, 191, 223, 270, 302, 334, 366, 398, 430, 462, 14, 46, 78, 110, 142, 174, 206, 238, 285,
      317, 349, 381, 413, 445, 477, 29, 61, 93, 125, 157, 189, 221, 253, 300, 332, 364, 396, 428, 460, 12, 44,
      76, 108, 140, 172, 204, 236, 268, 315, 347, 379, 411, 443, 475, 27, 59, 91, 123, 155, 187, 219, 251, 283,
      330, 362, 394, 426, 458, 10, 42, 74, 106, 138, 170, 202, 234, 266, 298, 345, 377, 409, 441, 473, 25, 57,
      89, 121, 153, 185, 217, 249, 281, 313, 360, 392, 424, 456, 8, 40, 72, 104, 136, 168, 200, 232, 264, 296,
      328, 375, 407, 439, 471, 23, 55, 87, 119, 151, 183, 215, 247, 279, 311, 343, 390, 422, 454, 6, 38, 70,
      102, 134, 166, 198, 230, 262, 294, 326, 358, 405, 437, 469, 21, 53, 85, 117, 149, 181, 213, 245, 277, 309,
      341, 373, 420, 452, 4, 36, 68, 100, 132, 164, 196, 228, 260, 292, 324, 356, 388, 435, 467, 19, 51, 83,
      115, 147, 179, 211, 243, 275, 307, 339, 371, 403, 450, 2, 34, 66, 98, 130, 162, 194, 226, 258, 290, 322,
      354, 386, 418, 465, 17, 49, 81, 113, 145, 177, 209, 241, 273, 305, 337, 369, 401, 433};

  const int L = 480;
  const int A = 15;
  const int B = 32;
  const int* idx1 = table1;
  const int* idx2 = table2;

  int k, l;
  ALIGN_16_BYTE float temp[64], out[960];

  for (k = 0; k < A; k++) {
    for (l = 0; l < B; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1 + 1];
      idx1 += A;
    }

    fft32(temp);
    idx1 -= L;

    for (l = 0; l < B; l++) {
      in[2 * *idx1] = temp[2 * l];
      in[2 * *idx1 + 1] = temp[2 * l + 1];
      idx1 += A;
    }

    idx1 -= L - 1;
  }

  idx1 -= A;

  for (k = 0; k < B; k++) {
    for (l = 0; l < A; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1++ + 1];
    }

    fft15(temp);

    for (l = 0; l < A; l++) {
      out[2 * *idx2] = temp[2 * l];
      out[2 * *idx2++ + 1] = temp[2 * l + 1];
    }
  }

  memmove(in, out, 2 * L * sizeof(float));
}
