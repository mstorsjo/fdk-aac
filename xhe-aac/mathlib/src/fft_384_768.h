
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

static void fft384(float* restrict in) {
  const int table1[384] = {
      0, 256, 128, 129, 1, 257, 258, 130, 2, 3, 259, 131, 132, 4, 260, 261, 133, 5, 6, 262, 134, 135,
      7, 263, 264, 136, 8, 9, 265, 137, 138, 10, 266, 267, 139, 11, 12, 268, 140, 141, 13, 269, 270, 142,
      14, 15, 271, 143, 144, 16, 272, 273, 145, 17, 18, 274, 146, 147, 19, 275, 276, 148, 20, 21, 277, 149,
      150, 22, 278, 279, 151, 23, 24, 280, 152, 153, 25, 281, 282, 154, 26, 27, 283, 155, 156, 28, 284, 285,
      157, 29, 30, 286, 158, 159, 31, 287, 288, 160, 32, 33, 289, 161, 162, 34, 290, 291, 163, 35, 36, 292,
      164, 165, 37, 293, 294, 166, 38, 39, 295, 167, 168, 40, 296, 297, 169, 41, 42, 298, 170, 171, 43, 299,
      300, 172, 44, 45, 301, 173, 174, 46, 302, 303, 175, 47, 48, 304, 176, 177, 49, 305, 306, 178, 50, 51,
      307, 179, 180, 52, 308, 309, 181, 53, 54, 310, 182, 183, 55, 311, 312, 184, 56, 57, 313, 185, 186, 58,
      314, 315, 187, 59, 60, 316, 188, 189, 61, 317, 318, 190, 62, 63, 319, 191, 192, 64, 320, 321, 193, 65,
      66, 322, 194, 195, 67, 323, 324, 196, 68, 69, 325, 197, 198, 70, 326, 327, 199, 71, 72, 328, 200, 201,
      73, 329, 330, 202, 74, 75, 331, 203, 204, 76, 332, 333, 205, 77, 78, 334, 206, 207, 79, 335, 336, 208,
      80, 81, 337, 209, 210, 82, 338, 339, 211, 83, 84, 340, 212, 213, 85, 341, 342, 214, 86, 87, 343, 215,
      216, 88, 344, 345, 217, 89, 90, 346, 218, 219, 91, 347, 348, 220, 92, 93, 349, 221, 222, 94, 350, 351,
      223, 95, 96, 352, 224, 225, 97, 353, 354, 226, 98, 99, 355, 227, 228, 100, 356, 357, 229, 101, 102, 358,
      230, 231, 103, 359, 360, 232, 104, 105, 361, 233, 234, 106, 362, 363, 235, 107, 108, 364, 236, 237, 109, 365,
      366, 238, 110, 111, 367, 239, 240, 112, 368, 369, 241, 113, 114, 370, 242, 243, 115, 371, 372, 244, 116, 117,
      373, 245, 246, 118, 374, 375, 247, 119, 120, 376, 248, 249, 121, 377, 378, 250, 122, 123, 379, 251, 252, 124,
      380, 381, 253, 125, 126, 382, 254, 255, 127, 383};
  const int table2[384] = {
      0, 128, 256, 3, 131, 259, 6, 134, 262, 9, 137, 265, 12, 140, 268, 15, 143, 271, 18, 146, 274, 21,
      149, 277, 24, 152, 280, 27, 155, 283, 30, 158, 286, 33, 161, 289, 36, 164, 292, 39, 167, 295, 42, 170,
      298, 45, 173, 301, 48, 176, 304, 51, 179, 307, 54, 182, 310, 57, 185, 313, 60, 188, 316, 63, 191, 319,
      66, 194, 322, 69, 197, 325, 72, 200, 328, 75, 203, 331, 78, 206, 334, 81, 209, 337, 84, 212, 340, 87,
      215, 343, 90, 218, 346, 93, 221, 349, 96, 224, 352, 99, 227, 355, 102, 230, 358, 105, 233, 361, 108, 236,
      364, 111, 239, 367, 114, 242, 370, 117, 245, 373, 120, 248, 376, 123, 251, 379, 126, 254, 382, 129, 257, 1,
      132, 260, 4, 135, 263, 7, 138, 266, 10, 141, 269, 13, 144, 272, 16, 147, 275, 19, 150, 278, 22, 153,
      281, 25, 156, 284, 28, 159, 287, 31, 162, 290, 34, 165, 293, 37, 168, 296, 40, 171, 299, 43, 174, 302,
      46, 177, 305, 49, 180, 308, 52, 183, 311, 55, 186, 314, 58, 189, 317, 61, 192, 320, 64, 195, 323, 67,
      198, 326, 70, 201, 329, 73, 204, 332, 76, 207, 335, 79, 210, 338, 82, 213, 341, 85, 216, 344, 88, 219,
      347, 91, 222, 350, 94, 225, 353, 97, 228, 356, 100, 231, 359, 103, 234, 362, 106, 237, 365, 109, 240, 368,
      112, 243, 371, 115, 246, 374, 118, 249, 377, 121, 252, 380, 124, 255, 383, 127, 258, 2, 130, 261, 5, 133,
      264, 8, 136, 267, 11, 139, 270, 14, 142, 273, 17, 145, 276, 20, 148, 279, 23, 151, 282, 26, 154, 285,
      29, 157, 288, 32, 160, 291, 35, 163, 294, 38, 166, 297, 41, 169, 300, 44, 172, 303, 47, 175, 306, 50,
      178, 309, 53, 181, 312, 56, 184, 315, 59, 187, 318, 62, 190, 321, 65, 193, 324, 68, 196, 327, 71, 199,
      330, 74, 202, 333, 77, 205, 336, 80, 208, 339, 83, 211, 342, 86, 214, 345, 89, 217, 348, 92, 220, 351,
      95, 223, 354, 98, 226, 357, 101, 229, 360, 104, 232, 363, 107, 235, 366, 110, 238, 369, 113, 241, 372, 116,
      244, 375, 119, 247, 378, 122, 250, 381, 125, 253};

  const int L = 384;
  const int A = 3;
  const int B = 128;
  const int* idx1 = table1;
  const int* idx2 = table2;

  int k, l;
  ALIGN_16_BYTE float temp[256], out[768];

  for (k = 0; k < A; k++) {
    for (l = 0; l < B; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1 + 1];
      idx1 += A;
    }

    fft128(temp);
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

    fft3(temp);

    for (l = 0; l < A; l++) {
      out[2 * *idx2] = temp[2 * l];
      out[2 * *idx2++ + 1] = temp[2 * l + 1];
    }
  }

  memmove(in, out, 2 * L * sizeof(float));
}

static void fft768(float* in) {
  const int table1[768] = {
      0, 256, 512, 513, 1, 257, 258, 514, 2, 3, 259, 515, 516, 4, 260, 261, 517, 5, 6, 262, 518, 519,
      7, 263, 264, 520, 8, 9, 265, 521, 522, 10, 266, 267, 523, 11, 12, 268, 524, 525, 13, 269, 270, 526,
      14, 15, 271, 527, 528, 16, 272, 273, 529, 17, 18, 274, 530, 531, 19, 275, 276, 532, 20, 21, 277, 533,
      534, 22, 278, 279, 535, 23, 24, 280, 536, 537, 25, 281, 282, 538, 26, 27, 283, 539, 540, 28, 284, 285,
      541, 29, 30, 286, 542, 543, 31, 287, 288, 544, 32, 33, 289, 545, 546, 34, 290, 291, 547, 35, 36, 292,
      548, 549, 37, 293, 294, 550, 38, 39, 295, 551, 552, 40, 296, 297, 553, 41, 42, 298, 554, 555, 43, 299,
      300, 556, 44, 45, 301, 557, 558, 46, 302, 303, 559, 47, 48, 304, 560, 561, 49, 305, 306, 562, 50, 51,
      307, 563, 564, 52, 308, 309, 565, 53, 54, 310, 566, 567, 55, 311, 312, 568, 56, 57, 313, 569, 570, 58,
      314, 315, 571, 59, 60, 316, 572, 573, 61, 317, 318, 574, 62, 63, 319, 575, 576, 64, 320, 321, 577, 65,
      66, 322, 578, 579, 67, 323, 324, 580, 68, 69, 325, 581, 582, 70, 326, 327, 583, 71, 72, 328, 584, 585,
      73, 329, 330, 586, 74, 75, 331, 587, 588, 76, 332, 333, 589, 77, 78, 334, 590, 591, 79, 335, 336, 592,
      80, 81, 337, 593, 594, 82, 338, 339, 595, 83, 84, 340, 596, 597, 85, 341, 342, 598, 86, 87, 343, 599,
      600, 88, 344, 345, 601, 89, 90, 346, 602, 603, 91, 347, 348, 604, 92, 93, 349, 605, 606, 94, 350, 351,
      607, 95, 96, 352, 608, 609, 97, 353, 354, 610, 98, 99, 355, 611, 612, 100, 356, 357, 613, 101, 102, 358,
      614, 615, 103, 359, 360, 616, 104, 105, 361, 617, 618, 106, 362, 363, 619, 107, 108, 364, 620, 621, 109, 365,
      366, 622, 110, 111, 367, 623, 624, 112, 368, 369, 625, 113, 114, 370, 626, 627, 115, 371, 372, 628, 116, 117,
      373, 629, 630, 118, 374, 375, 631, 119, 120, 376, 632, 633, 121, 377, 378, 634, 122, 123, 379, 635, 636, 124,
      380, 381, 637, 125, 126, 382, 638, 639, 127, 383, 384, 640, 128, 129, 385, 641, 642, 130, 386, 387, 643, 131,
      132, 388, 644, 645, 133, 389, 390, 646, 134, 135, 391, 647, 648, 136, 392, 393, 649, 137, 138, 394, 650, 651,
      139, 395, 396, 652, 140, 141, 397, 653, 654, 142, 398, 399, 655, 143, 144, 400, 656, 657, 145, 401, 402, 658,
      146, 147, 403, 659, 660, 148, 404, 405, 661, 149, 150, 406, 662, 663, 151, 407, 408, 664, 152, 153, 409, 665,
      666, 154, 410, 411, 667, 155, 156, 412, 668, 669, 157, 413, 414, 670, 158, 159, 415, 671, 672, 160, 416, 417,
      673, 161, 162, 418, 674, 675, 163, 419, 420, 676, 164, 165, 421, 677, 678, 166, 422, 423, 679, 167, 168, 424,
      680, 681, 169, 425, 426, 682, 170, 171, 427, 683, 684, 172, 428, 429, 685, 173, 174, 430, 686, 687, 175, 431,
      432, 688, 176, 177, 433, 689, 690, 178, 434, 435, 691, 179, 180, 436, 692, 693, 181, 437, 438, 694, 182, 183,
      439, 695, 696, 184, 440, 441, 697, 185, 186, 442, 698, 699, 187, 443, 444, 700, 188, 189, 445, 701, 702, 190,
      446, 447, 703, 191, 192, 448, 704, 705, 193, 449, 450, 706, 194, 195, 451, 707, 708, 196, 452, 453, 709, 197,
      198, 454, 710, 711, 199, 455, 456, 712, 200, 201, 457, 713, 714, 202, 458, 459, 715, 203, 204, 460, 716, 717,
      205, 461, 462, 718, 206, 207, 463, 719, 720, 208, 464, 465, 721, 209, 210, 466, 722, 723, 211, 467, 468, 724,
      212, 213, 469, 725, 726, 214, 470, 471, 727, 215, 216, 472, 728, 729, 217, 473, 474, 730, 218, 219, 475, 731,
      732, 220, 476, 477, 733, 221, 222, 478, 734, 735, 223, 479, 480, 736, 224, 225, 481, 737, 738, 226, 482, 483,
      739, 227, 228, 484, 740, 741, 229, 485, 486, 742, 230, 231, 487, 743, 744, 232, 488, 489, 745, 233, 234, 490,
      746, 747, 235, 491, 492, 748, 236, 237, 493, 749, 750, 238, 494, 495, 751, 239, 240, 496, 752, 753, 241, 497,
      498, 754, 242, 243, 499, 755, 756, 244, 500, 501, 757, 245, 246, 502, 758, 759, 247, 503, 504, 760, 248, 249,
      505, 761, 762, 250, 506, 507, 763, 251, 252, 508, 764, 765, 253, 509, 510, 766, 254, 255, 511, 767};
  const int table2[768] = {
      0, 256, 512, 3, 259, 515, 6, 262, 518, 9, 265, 521, 12, 268, 524, 15, 271, 527, 18, 274, 530, 21,
      277, 533, 24, 280, 536, 27, 283, 539, 30, 286, 542, 33, 289, 545, 36, 292, 548, 39, 295, 551, 42, 298,
      554, 45, 301, 557, 48, 304, 560, 51, 307, 563, 54, 310, 566, 57, 313, 569, 60, 316, 572, 63, 319, 575,
      66, 322, 578, 69, 325, 581, 72, 328, 584, 75, 331, 587, 78, 334, 590, 81, 337, 593, 84, 340, 596, 87,
      343, 599, 90, 346, 602, 93, 349, 605, 96, 352, 608, 99, 355, 611, 102, 358, 614, 105, 361, 617, 108, 364,
      620, 111, 367, 623, 114, 370, 626, 117, 373, 629, 120, 376, 632, 123, 379, 635, 126, 382, 638, 129, 385, 641,
      132, 388, 644, 135, 391, 647, 138, 394, 650, 141, 397, 653, 144, 400, 656, 147, 403, 659, 150, 406, 662, 153,
      409, 665, 156, 412, 668, 159, 415, 671, 162, 418, 674, 165, 421, 677, 168, 424, 680, 171, 427, 683, 174, 430,
      686, 177, 433, 689, 180, 436, 692, 183, 439, 695, 186, 442, 698, 189, 445, 701, 192, 448, 704, 195, 451, 707,
      198, 454, 710, 201, 457, 713, 204, 460, 716, 207, 463, 719, 210, 466, 722, 213, 469, 725, 216, 472, 728, 219,
      475, 731, 222, 478, 734, 225, 481, 737, 228, 484, 740, 231, 487, 743, 234, 490, 746, 237, 493, 749, 240, 496,
      752, 243, 499, 755, 246, 502, 758, 249, 505, 761, 252, 508, 764, 255, 511, 767, 258, 514, 2, 261, 517, 5,
      264, 520, 8, 267, 523, 11, 270, 526, 14, 273, 529, 17, 276, 532, 20, 279, 535, 23, 282, 538, 26, 285,
      541, 29, 288, 544, 32, 291, 547, 35, 294, 550, 38, 297, 553, 41, 300, 556, 44, 303, 559, 47, 306, 562,
      50, 309, 565, 53, 312, 568, 56, 315, 571, 59, 318, 574, 62, 321, 577, 65, 324, 580, 68, 327, 583, 71,
      330, 586, 74, 333, 589, 77, 336, 592, 80, 339, 595, 83, 342, 598, 86, 345, 601, 89, 348, 604, 92, 351,
      607, 95, 354, 610, 98, 357, 613, 101, 360, 616, 104, 363, 619, 107, 366, 622, 110, 369, 625, 113, 372, 628,
      116, 375, 631, 119, 378, 634, 122, 381, 637, 125, 384, 640, 128, 387, 643, 131, 390, 646, 134, 393, 649, 137,
      396, 652, 140, 399, 655, 143, 402, 658, 146, 405, 661, 149, 408, 664, 152, 411, 667, 155, 414, 670, 158, 417,
      673, 161, 420, 676, 164, 423, 679, 167, 426, 682, 170, 429, 685, 173, 432, 688, 176, 435, 691, 179, 438, 694,
      182, 441, 697, 185, 444, 700, 188, 447, 703, 191, 450, 706, 194, 453, 709, 197, 456, 712, 200, 459, 715, 203,
      462, 718, 206, 465, 721, 209, 468, 724, 212, 471, 727, 215, 474, 730, 218, 477, 733, 221, 480, 736, 224, 483,
      739, 227, 486, 742, 230, 489, 745, 233, 492, 748, 236, 495, 751, 239, 498, 754, 242, 501, 757, 245, 504, 760,
      248, 507, 763, 251, 510, 766, 254, 513, 1, 257, 516, 4, 260, 519, 7, 263, 522, 10, 266, 525, 13, 269,
      528, 16, 272, 531, 19, 275, 534, 22, 278, 537, 25, 281, 540, 28, 284, 543, 31, 287, 546, 34, 290, 549,
      37, 293, 552, 40, 296, 555, 43, 299, 558, 46, 302, 561, 49, 305, 564, 52, 308, 567, 55, 311, 570, 58,
      314, 573, 61, 317, 576, 64, 320, 579, 67, 323, 582, 70, 326, 585, 73, 329, 588, 76, 332, 591, 79, 335,
      594, 82, 338, 597, 85, 341, 600, 88, 344, 603, 91, 347, 606, 94, 350, 609, 97, 353, 612, 100, 356, 615,
      103, 359, 618, 106, 362, 621, 109, 365, 624, 112, 368, 627, 115, 371, 630, 118, 374, 633, 121, 377, 636, 124,
      380, 639, 127, 383, 642, 130, 386, 645, 133, 389, 648, 136, 392, 651, 139, 395, 654, 142, 398, 657, 145, 401,
      660, 148, 404, 663, 151, 407, 666, 154, 410, 669, 157, 413, 672, 160, 416, 675, 163, 419, 678, 166, 422, 681,
      169, 425, 684, 172, 428, 687, 175, 431, 690, 178, 434, 693, 181, 437, 696, 184, 440, 699, 187, 443, 702, 190,
      446, 705, 193, 449, 708, 196, 452, 711, 199, 455, 714, 202, 458, 717, 205, 461, 720, 208, 464, 723, 211, 467,
      726, 214, 470, 729, 217, 473, 732, 220, 476, 735, 223, 479, 738, 226, 482, 741, 229, 485, 744, 232, 488, 747,
      235, 491, 750, 238, 494, 753, 241, 497, 756, 244, 500, 759, 247, 503, 762, 250, 506, 765, 253, 509};

  const int L = 768;
  const int A = 3;
  const int B = 256;
  const int* idx1 = table1;
  const int* idx2 = table2;

  int k, l;
  ALIGN_16_BYTE float temp[512], out[1536];

  for (k = 0; k < A; k++) {
    for (l = 0; l < B; l++) {
      temp[2 * l] = in[2 * *idx1];
      temp[2 * l + 1] = in[2 * *idx1 + 1];
      idx1 += A;
    }

    cfft(temp, temp + 1, 256, 2, -1);
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

    fft3(temp);

    for (l = 0; l < A; l++) {
      out[2 * *idx2] = temp[2 * l];
      out[2 * *idx2++ + 1] = temp[2 * l + 1];
    }
  }

  memmove(in, out, 2 * L * sizeof(float));
}
