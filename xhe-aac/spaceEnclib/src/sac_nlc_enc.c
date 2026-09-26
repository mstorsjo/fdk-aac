
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

#include <math.h>
#include <string.h>

#include "bit_buf.h"

#include "spaceEnclib_const.h"
#include "sac_nlc_enc.h"
#include "sac_huff_tab.h"
#include "sac_types.h"
#include "space_paramextract.h"

#define PBC_MIN_BANDS 5

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

extern const HUFF_CLD_TABLE huffCLDTab;
extern const HUFF_ICC_TABLE huffICCTab;
extern const HUFF_IPD_TABLE huffIPDTab;
extern const HUFF_PT0_TABLE huffPart0Tab;
extern const HUFF_RES_TABLE huffReshapeTab;

static const short lavHuffVal[4] = {0, 2, 6, 7};
static const short lavHuffLen[4] = {1, 2, 3, 3};
static const short lavMapIPD[4] = {7, 1, 3, 5};

static void split_lsb(short* in_data,
                      short offset,
                      short num_val,
                      short* out_data_lsb,
                      short* out_data_msb) {
  short i = 0, val = 0, lsb = 0, msb = 0;

  for (i = 0; i < num_val; i++) {
    val = in_data[i] + offset;

    lsb = val & 0x0001;
    msb = val >> 1;

    if (out_data_lsb != NULL) out_data_lsb[i] = lsb;
    if (out_data_msb != NULL) out_data_msb[i] = msb;
  }
}

static void apply_lsb_coding(HANDLE_BIT_BUF strm,
                             short* in_data_lsb,
                             short num_lsb,
                             short num_val) {
  short i = 0;

  for (i = 0; i < num_val; i++) {
    WriteBits(strm, in_data_lsb[i], num_lsb);
  }
}

static void calc_diff_freq(short* in_data,
                           short* out_data,
                           short num_val) {
  short i = 0;

  out_data[0] = in_data[0];

  for (i = 1; i < num_val; i++) {
    out_data[i] = in_data[i] - in_data[i - 1];
  }
}

static void calc_diff_time(short* in_data,
                           short* prev_data,
                           short* out_data,
                           short num_val) {
  short i = 0;

  out_data[0] = in_data[0];
  out_data[1] = prev_data[0];

  for (i = 0; i < num_val; i++) {
    out_data[i + 2] = in_data[i] - prev_data[i];
  }
}

static void swap(short* a) {
  short tmp = a[0];

  a[0] = a[1];
  a[1] = tmp;
}

static void wrapIPDs(short* data, short num_val) {
  short i = 0;

  for (i = 0; i < num_val; i++) {
    data[i] = (data[i] + 8) % 8;
  }
}

static short sym_check(short data[2],
                       short lav,
                       short* sym_bits,
                       short bSigned) {
  short sum_val = data[0] + data[1];
  short diff_val = data[0] - data[1];

  short sum_neg = 0, diff_neg = 0;
  short num_sbits = 0;

  if (sym_bits != NULL) {
    *sym_bits = 0;
  }

  if (bSigned) {
    if (sum_val != 0) {
      sum_neg = (sum_val < 0);
      if (sum_neg) {
        sum_val = -sum_val;
        diff_val = -diff_val;
      }
      if (sym_bits != NULL) {
        *sym_bits = (*sym_bits << 1) | sum_neg;
      }
      num_sbits++;
    }
  }

  if (diff_val != 0) {
    diff_neg = (diff_val < 0);
    if (diff_neg) {
      diff_val = -diff_val;
    }
    if (sym_bits != NULL) {
      *sym_bits = (*sym_bits << 1) | diff_neg;
    }
    num_sbits++;
  }

  if (sum_val % 2) {
    data[0] = lav - sum_val / 2;
    data[1] = lav - diff_val / 2;
  } else {
    data[0] = sum_val / 2;
    data[1] = diff_val / 2;
  }

  return num_sbits;
}

static int ilog2(unsigned int i) {
  int l = 0;

  if (i) i--;
  while (i > 0) {
    i >>= 1;
    l++;
  }

  return l;
}

static short calc_pcm_bits(short num_val,
                           short num_levels) {
  short num_complete_chunks = 0, rest_chunk_size = 0;
  short max_grp_len = 0, bits_pcm = 0;
  int chunk_levels, i;

  switch (num_levels) {
    case 3:
      max_grp_len = 5;
      break;
    case 6:
      max_grp_len = 5;
      break;
    case 7:
      max_grp_len = 6;
      break;
    case 11:
      max_grp_len = 2;
      break;
    case 13:
      max_grp_len = 4;
      break;
    case 19:
      max_grp_len = 4;
      break;
    case 25:
      max_grp_len = 3;
      break;
    case 51:
      max_grp_len = 4;
      break;
    default:
      max_grp_len = 1;
  }

  num_complete_chunks = num_val / max_grp_len;
  rest_chunk_size = num_val % max_grp_len;

  chunk_levels = 1;
  for (i = 1; i <= max_grp_len; i++) {
    chunk_levels *= num_levels;
  }

  bits_pcm = (short)(ilog2(chunk_levels) * num_complete_chunks);
  bits_pcm += (short)(ilog2(num_levels) * rest_chunk_size);

  return bits_pcm;
}

static void apply_pcm_coding(HANDLE_BIT_BUF strm,
                             short* in_data_1,
                             short* in_data_2,
                             short offset,
                             short num_val,
                             short num_levels) {
  short i = 0, j = 0, idx = 0;
  short max_grp_len = 0, grp_len = 0, next_val = 0;
  int grp_val = 0, chunk_levels = 0;

  short pcm_chunk_size[7] = {0};

  switch (num_levels) {
    case 3:
      max_grp_len = 5;
      break;
    case 5:
      max_grp_len = 3;
      break;
    case 6:
      max_grp_len = 5;
      break;
    case 7:
      max_grp_len = 6;
      break;
    case 9:
      max_grp_len = 5;
      break;
    case 11:
      max_grp_len = 2;
      break;
    case 13:
      max_grp_len = 4;
      break;
    case 19:
      max_grp_len = 4;
      break;
    case 25:
      max_grp_len = 3;
      break;
    case 51:
      max_grp_len = 4;
      break;
    default:
      max_grp_len = 1;
  }

  chunk_levels = 1;
  for (i = 1; i <= max_grp_len; i++) {
    chunk_levels *= num_levels;
    pcm_chunk_size[i] = ilog2(chunk_levels);
  }

  for (i = 0; i < num_val; i += max_grp_len) {
    grp_len = min(max_grp_len, num_val - i);
    grp_val = 0;
    for (j = 0; j < grp_len; j++) {
      idx = i + j;
      if (in_data_2 == NULL) {
        next_val = in_data_1[idx];
      } else if (in_data_1 == NULL) {
        next_val = in_data_2[idx];
      } else {
        next_val = ((idx % 2) ? in_data_2[idx / 2] : in_data_1[idx / 2]);
      }
      next_val += offset;
      grp_val = grp_val * num_levels + next_val;
    }

    WriteBits(strm, grp_val, pcm_chunk_size[grp_len]);
  }
}

static short count_huff_cld_1D(const HUFF_CLD_TAB_1D* huffTab,
                               short* in_data,
                               short num_val,
                               short p0_flag) {
  short i = 0, id = 0;
  short huffBits = 0;
  short offset = 0;

  if (p0_flag) {
    huffBits += huffPart0Tab.cld.length[in_data[0]];
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];

    if (id != 0) {
      if (id < 0) {
        id = -id;
      }
      huffBits += 1;
    }

    huffBits += huffTab->length[id];
  }

  return huffBits;
}

static short count_huff_icc_1D(const HUFF_ICC_TAB_1D* huffTab,
                               short* in_data,
                               short num_val,
                               short p0_flag) {
  short i = 0, id = 0;
  short huffBits = 0;
  short offset = 0;

  if (p0_flag) {
    huffBits += huffPart0Tab.icc.length[in_data[0]];
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];

    if (id != 0) {
      if (id < 0) {
        id = -id;
      }
      huffBits += 1;
    }

    huffBits += huffTab->length[id];
  }

  return huffBits;
}

static short count_huff_ipd_1D(const HUFF_IPD_TAB_1D* huffTab,
                               short* in_data,
                               short num_val,
                               short p0_flag) {
  short i = 0, id = 0;
  short huffBits = 0;
  short offset = 0;

  if (p0_flag) {
    huffBits += huffPart0Tab.ipd.length[in_data[0]];
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];
    huffBits += huffTab->length[id];
  }

  return huffBits;
}

static short count_huff_cld_2D(const HUFF_CLD_TAB_2D* huffTab,
                               short lav_idx,
                               short in_data[][2],
                               short num_val,
                               short stride,
                               short* p0_data[2]) {
  short i = 0, lav = 0, huffBits = 0, symBits = 0;
  short escapeFlag = 0, escIdx = 0;

  lav = 2 * lav_idx + 3;

  if (p0_data[0] != NULL) {
    huffBits += huffPart0Tab.cld.length[*p0_data[0]];
  }
  if (p0_data[1] != NULL) {
    huffBits += huffPart0Tab.cld.length[*p0_data[1]];
  }

  for (i = 0; i < num_val; i += stride) {
    symBits = sym_check(in_data[i], lav, NULL, 1);

    escapeFlag = 0;

    switch (lav) {
      case 3:
        huffBits += huffTab->lav3.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        huffBits += huffTab->lav5.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        huffBits += huffTab->lav7.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 9:
        huffBits += huffTab->lav9.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav9.value[in_data[i][0]][in_data[i][1]] == huffTab->lav9.escape[0]) &&
            (huffTab->lav9.length[in_data[i][0]][in_data[i][1]] == huffTab->lav9.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      huffBits += symBits;
    }
  }

  if (escIdx > 0) {
    huffBits += calc_pcm_bits(2 * escIdx, (2 * lav + 1));
  }

  return huffBits;
}

static short count_huff_icc_2D(const HUFF_ICC_TAB_2D* huffTab,
                               short lav_idx,
                               short in_data[][2],
                               short num_val,
                               short stride,
                               short* p0_data[2]) {
  short i = 0, lav = 0, huffBits = 0, symBits = 0;
  short escapeFlag = 0, escIdx = 0;

  lav = 2 * lav_idx + 1;

  if (p0_data[0] != NULL) {
    huffBits += huffPart0Tab.icc.length[*p0_data[0]];
  }
  if (p0_data[1] != NULL) {
    huffBits += huffPart0Tab.icc.length[*p0_data[1]];
  }

  for (i = 0; i < num_val; i += stride) {
    symBits = sym_check(in_data[i], lav, NULL, 1);

    escapeFlag = 0;

    switch (lav) {
      case 1:
        huffBits += huffTab->lav1.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav1.value[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[0]) &&
            (huffTab->lav1.length[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 3:
        huffBits += huffTab->lav3.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        huffBits += huffTab->lav5.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        huffBits += huffTab->lav7.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      huffBits += symBits;
    }
  }

  if (escIdx > 0) {
    huffBits += calc_pcm_bits(2 * escIdx, (2 * lav + 1));
  }

  return huffBits;
}

static short count_huff_ipd_2D(const HUFF_IPD_TAB_2D* huffTab,
                               short lav_idx,
                               short in_data[][2],
                               short num_val,
                               short stride,
                               short* p0_data[2]) {
  short i = 0, lav = 0, huffBits = 0, symBits = 0;
  short escapeFlag = 0, escIdx = 0;

  lav = lavMapIPD[lav_idx];

  if (p0_data[0] != NULL) {
    huffBits += huffPart0Tab.ipd.length[*p0_data[0]];
  }
  if (p0_data[1] != NULL) {
    huffBits += huffPart0Tab.ipd.length[*p0_data[1]];
  }

  for (i = 0; i < num_val; i += stride) {
    symBits = sym_check(in_data[i], lav, NULL, 0);

    escapeFlag = 0;

    switch (lav) {
      case 1:
        huffBits += huffTab->lav1.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav1.value[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[0]) &&
            (huffTab->lav1.length[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 3:
        huffBits += huffTab->lav3.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        huffBits += huffTab->lav5.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        huffBits += huffTab->lav7.length[in_data[i][0]][in_data[i][1]];
        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      huffBits += symBits;
    }
  }

  if (escIdx > 0) {
    huffBits += calc_pcm_bits(2 * escIdx, (2 * lav + 1));
  }

  return huffBits;
}

static void huff_enc_cld_1D(HANDLE_BIT_BUF strm,
                            const HUFF_CLD_TAB_1D* huffTab,
                            short* in_data,
                            short num_val,
                            short p0_flag) {
  short i = 0, id = 0, id_sign = 0;
  short offset = 0;

  if (p0_flag) {
    WriteBits(strm, huffPart0Tab.cld.value[in_data[0]],
              huffPart0Tab.cld.length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];

    if (id != 0) {
      id_sign = 0;
      if (id < 0) {
        id = -id;
        id_sign = 1;
      }
    }

    WriteBits(strm, huffTab->value[id], huffTab->length[id]);

    if (id != 0) {
      WriteBits(strm, id_sign, 1);
    }
  }
}

static void huff_enc_icc_1D(HANDLE_BIT_BUF strm,
                            const HUFF_ICC_TAB_1D* huffTab,
                            short* in_data,
                            short num_val,
                            short p0_flag) {
  short i = 0, id = 0, id_sign = 0;
  short offset = 0;

  if (p0_flag) {
    WriteBits(strm, huffPart0Tab.icc.value[in_data[0]],
              huffPart0Tab.icc.length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];

    if (id != 0) {
      id_sign = 0;
      if (id < 0) {
        id = -id;
        id_sign = 1;
      }
    }

    WriteBits(strm, huffTab->value[id], huffTab->length[id]);

    if (id != 0) {
      WriteBits(strm, id_sign, 1);
    }
  }
}

static void huff_enc_ipd_1D(HANDLE_BIT_BUF strm,
                            const HUFF_IPD_TAB_1D* huffTab,
                            short* in_data,
                            short num_val,
                            short p0_flag) {
  short i = 0, id = 0;
  short offset = 0;

  if (p0_flag) {
    WriteBits(strm, huffPart0Tab.ipd.value[in_data[0]],
              huffPart0Tab.ipd.length[in_data[0]]);
    offset = 1;
  }

  for (i = offset; i < num_val; i++) {
    id = in_data[i];
    WriteBits(strm, huffTab->value[id], huffTab->length[id]);
  }
}

static void huff_enc_cld_2D(HANDLE_BIT_BUF strm,
                            const HUFF_CLD_TAB_2D* huffTab,
                            short lav_idx,
                            short in_data[][2],
                            short num_val,
                            short stride,
                            short* p0_data[2]) {
  short i = 0, lav = 0, num_sbits = 0, sym_bits = 0, escIdx = 0, escapeFlag = 0;
  short esc_data[2][MAXBANDS] = {{0}};

  lav = lav_idx * 2 + 3;

  if (p0_data[0] != NULL) {
    WriteBits(strm, huffPart0Tab.cld.value[*p0_data[0]],
              huffPart0Tab.cld.length[*p0_data[0]]);
  }
  if (p0_data[1] != NULL) {
    WriteBits(strm, huffPart0Tab.cld.value[*p0_data[1]],
              huffPart0Tab.cld.length[*p0_data[1]]);
  }

  for (i = 0; i < num_val; i += stride) {
    esc_data[0][escIdx] = in_data[i][0] + lav;
    esc_data[1][escIdx] = in_data[i][1] + lav;

    num_sbits = sym_check(in_data[i], lav, &sym_bits, 1);

    escapeFlag = 0;

    switch (lav) {
      case 3:
        WriteBits(strm, huffTab->lav3.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav3.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        WriteBits(strm, huffTab->lav5.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav5.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        WriteBits(strm, huffTab->lav7.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav7.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 9:
        WriteBits(strm, huffTab->lav9.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav9.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav9.value[in_data[i][0]][in_data[i][1]] == huffTab->lav9.escape[0]) &&
            (huffTab->lav9.length[in_data[i][0]][in_data[i][1]] == huffTab->lav9.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      WriteBits(strm, sym_bits, num_sbits);
    }
  }

  if (escIdx > 0) {
    apply_pcm_coding(strm, esc_data[0], esc_data[1], 0, 2 * escIdx, (2 * lav + 1));
  }
}

static void huff_enc_icc_2D(HANDLE_BIT_BUF strm,
                            const HUFF_ICC_TAB_2D* huffTab,
                            short lav_idx,
                            short in_data[][2],
                            short num_val,
                            short stride,
                            short* p0_data[2]) {
  short i = 0, lav = 0, num_sbits = 0, sym_bits = 0, escIdx = 0, escapeFlag = 0;
  short esc_data[2][MAXBANDS] = {{0}};

  lav = lav_idx * 2 + 1;

  if (p0_data[0] != NULL) {
    WriteBits(strm, huffPart0Tab.icc.value[*p0_data[0]],
              huffPart0Tab.icc.length[*p0_data[0]]);
  }
  if (p0_data[1] != NULL) {
    WriteBits(strm, huffPart0Tab.icc.value[*p0_data[1]],
              huffPart0Tab.icc.length[*p0_data[1]]);
  }

  for (i = 0; i < num_val; i += stride) {
    esc_data[0][escIdx] = in_data[i][0] + lav;
    esc_data[1][escIdx] = in_data[i][1] + lav;

    num_sbits = sym_check(in_data[i], lav, &sym_bits, 1);

    escapeFlag = 0;

    switch (lav) {
      case 1:
        WriteBits(strm, huffTab->lav1.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav1.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav1.value[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[0]) &&
            (huffTab->lav1.length[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 3:
        WriteBits(strm, huffTab->lav3.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav3.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        WriteBits(strm, huffTab->lav5.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav5.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        WriteBits(strm, huffTab->lav7.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav7.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      WriteBits(strm, sym_bits, num_sbits);
    }
  }

  if (escIdx > 0) {
    apply_pcm_coding(strm, esc_data[0], esc_data[1], 0, 2 * escIdx, (2 * lav + 1));
  }
}

static void huff_enc_ipd_2D(HANDLE_BIT_BUF strm,
                            const HUFF_IPD_TAB_2D* huffTab,
                            short lav_idx,
                            short in_data[][2],
                            short num_val,
                            short stride,
                            short* p0_data[2]) {
  short i = 0, lav = 0, num_sbits = 0, sym_bits = 0, escIdx = 0, escapeFlag = 0;
  short esc_data[2][MAXBANDS] = {{0}};

  lav = lavMapIPD[lav_idx];

  if (p0_data[0] != NULL) {
    WriteBits(strm, huffPart0Tab.ipd.value[*p0_data[0]],
              huffPart0Tab.ipd.length[*p0_data[0]]);
  }
  if (p0_data[1] != NULL) {
    WriteBits(strm, huffPart0Tab.ipd.value[*p0_data[1]],
              huffPart0Tab.ipd.length[*p0_data[1]]);
  }

  for (i = 0; i < num_val; i += stride) {
    esc_data[0][escIdx] = in_data[i][0] + lav;
    esc_data[1][escIdx] = in_data[i][1] + lav;

    num_sbits = sym_check(in_data[i], lav, &sym_bits, 0);

    escapeFlag = 0;

    switch (lav) {
      case 1:
        WriteBits(strm, huffTab->lav1.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav1.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav1.value[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[0]) &&
            (huffTab->lav1.length[in_data[i][0]][in_data[i][1]] == huffTab->lav1.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 3:
        WriteBits(strm, huffTab->lav3.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav3.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav3.value[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[0]) &&
            (huffTab->lav3.length[in_data[i][0]][in_data[i][1]] == huffTab->lav3.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 5:
        WriteBits(strm, huffTab->lav5.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav5.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav5.value[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[0]) &&
            (huffTab->lav5.length[in_data[i][0]][in_data[i][1]] == huffTab->lav5.escape[1])) {
          escapeFlag = 1;
        }
        break;

      case 7:
        WriteBits(strm, huffTab->lav7.value[in_data[i][0]][in_data[i][1]],
                  huffTab->lav7.length[in_data[i][0]][in_data[i][1]]);

        if ((huffTab->lav7.value[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[0]) &&
            (huffTab->lav7.length[in_data[i][0]][in_data[i][1]] == huffTab->lav7.escape[1])) {
          escapeFlag = 1;
        }
        break;

      default:

        assert(0);
    }

    if (escapeFlag) {
      escIdx++;
    } else {
      WriteBits(strm, sym_bits, num_sbits);
    }
  }

  if (escIdx > 0) {
    apply_pcm_coding(strm, esc_data[0], esc_data[1], 0, 2 * escIdx, (2 * lav + 1));
  }
}

static short get_next_lav_step(short lav,
                               DATA_TYPE data_type) {
  short lav_step_CLD[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3};
  short lav_step_ICC[] = {0, 0, 1, 1, 2, 2, 3, 3};
  short lav_step_IPD[] = {1, 1, 2, 2, 3, 3, 0, 0};

  short lav_step = 0;

  switch (data_type) {
    case t_CLD:
      lav_step = (lav > 9) ? -1 : lav_step_CLD[lav];
      break;
    case t_ICC:
      lav_step = (lav > 7) ? -1 : lav_step_ICC[lav];
      break;
    case t_IPD:
      lav_step = (lav > 7) ? -1 : lav_step_IPD[lav];
      break;
    default:

      assert(0);
      break;
  }

  return lav_step;
}

static short calc_huff_bits(short* in_data_1,
                            short* in_data_2,
                            DATA_TYPE data_type,
                            DIFF_TYPE diff_type_1,
                            DIFF_TYPE diff_type_2,
                            short num_val,
                            short* lav_idx,
                            short* cdg_scheme,
                            MPS_MODE mps_mode) {
  short tab_idx_2D[2][2] = {{0}};
  short tab_idx_1D[2] = {0};
  short df_rest_flag[2] = {0};
  short p0_flag[2] = {0};

  short pair_vec[MAXBANDS][2] = {{0}};

  short* p0_data_1[2] = {NULL};
  short* p0_data_2[2] = {NULL};

  short i = 0;
  short lav_fp[2] = {0};
  short lav_tp = 0;

  short bit_count_1D = 0;
  short bit_count_2D_freq = 0;
  short bit_count_2D_time = 0;
  short bit_count_min = 0;

  short num_val_1_short = 0;
  short num_val_2_short = 0;

  short* in_data_1_short = NULL;
  short* in_data_2_short = NULL;

  bit_count_1D = 1;

  num_val_1_short = num_val;
  num_val_2_short = num_val;

  if (in_data_1 != NULL) {
    switch (diff_type_1) {
      case DIFF_FREQ:
        in_data_1_short = in_data_1;
        break;
      case DIFF_PILOT:
        in_data_1_short = in_data_1 + 1;
        break;
      case DIFF_TIME:
        in_data_1_short = in_data_1 + 2;
        break;
    }
  }

  if (in_data_2 != NULL) {
    switch (diff_type_2) {
      case DIFF_FREQ:
        in_data_2_short = in_data_2;
        break;
      case DIFF_PILOT:
        in_data_2_short = in_data_2 + 1;
        break;
      case DIFF_TIME:
        in_data_2_short = in_data_2 + 2;
        break;
    }
  }

  p0_flag[0] = (diff_type_1 == DIFF_FREQ);
  p0_flag[1] = (diff_type_2 == DIFF_FREQ);

  tab_idx_1D[0] = (diff_type_1 == DIFF_FREQ) ? 0 : 1;
  tab_idx_1D[1] = (diff_type_2 == DIFF_FREQ) ? 0 : 1;

  switch (data_type) {
    case t_CLD:
      if (in_data_1 != NULL) bit_count_1D += count_huff_cld_1D(&huffCLDTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
      if (in_data_2 != NULL) bit_count_1D += count_huff_cld_1D(&huffCLDTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
      break;

    case t_ICC:
      if (in_data_1 != NULL) bit_count_1D += count_huff_icc_1D(&huffICCTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
      if (in_data_2 != NULL) bit_count_1D += count_huff_icc_1D(&huffICCTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
      break;

    case t_IPD:
      if (in_data_1 != NULL) bit_count_1D += count_huff_ipd_1D(&huffIPDTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
      if (in_data_2 != NULL) bit_count_1D += count_huff_ipd_1D(&huffIPDTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
      break;

    default:
      break;
  }

  bit_count_min = bit_count_1D;
  *cdg_scheme = CODING_SCHEME_HUFF_1D << PAIR_SHIFT;
  lav_idx[0] = lav_idx[1] = -1;

  bit_count_2D_freq = 1;

  if (mps_mode != MPS_MODE_LD) {
    if ((in_data_1 != NULL) && (in_data_2 != NULL)) {
      bit_count_2D_freq += 1;
    }
  }

  num_val_1_short = num_val;
  num_val_2_short = num_val;

  if (in_data_1 != NULL) {
    switch (diff_type_1) {
      case DIFF_FREQ:
        in_data_1_short = in_data_1;
        break;
      case DIFF_PILOT:
        in_data_1_short = in_data_1 + 1;
        break;
      case DIFF_TIME:
        in_data_1_short = in_data_1 + 2;
        break;
    }
  }

  if (in_data_2 != NULL) {
    switch (diff_type_2) {
      case DIFF_FREQ:
        in_data_2_short = in_data_2;
        break;
      case DIFF_PILOT:
        in_data_2_short = in_data_2 + 1;
        break;
      case DIFF_TIME:
        in_data_2_short = in_data_2 + 2;
        break;
    }
  }

  lav_fp[0] = lav_fp[1] = 0;

  p0_data_1[0] = NULL;
  p0_data_1[1] = NULL;
  p0_data_2[0] = NULL;
  p0_data_2[1] = NULL;

  if (in_data_1 != NULL) {
    if (diff_type_1 == DIFF_FREQ) {
      p0_data_1[0] = &in_data_1[0];
      p0_data_1[1] = NULL;

      num_val_1_short -= 1;
      in_data_1_short += 1;
    }

    df_rest_flag[0] = num_val_1_short % 2;

    if (df_rest_flag[0]) num_val_1_short -= 1;

    for (i = 0; i < num_val_1_short - 1; i += 2) {
      pair_vec[i][0] = in_data_1_short[i];
      pair_vec[i][1] = in_data_1_short[i + 1];

      lav_fp[0] = max(lav_fp[0], abs(pair_vec[i][0]));
      lav_fp[0] = max(lav_fp[0], abs(pair_vec[i][1]));
    }

    tab_idx_2D[0][0] = (diff_type_1 == DIFF_TIME) ? 1 : 0;
    tab_idx_2D[0][1] = (diff_type_1 == DIFF_PILOT) ? 1 : 0;

    tab_idx_1D[0] = (diff_type_1 == DIFF_FREQ) ? 0 : 1;

    lav_fp[0] = get_next_lav_step(lav_fp[0], data_type);

    if (lav_fp[0] != -1) bit_count_2D_freq += lavHuffLen[lav_fp[0]];
  }

  if (in_data_2 != NULL) {
    if (diff_type_2 == DIFF_FREQ) {
      p0_data_2[0] = NULL;
      p0_data_2[1] = &in_data_2[0];

      num_val_2_short -= 1;
      in_data_2_short += 1;
    }

    df_rest_flag[1] = num_val_2_short % 2;

    if (df_rest_flag[1]) num_val_2_short -= 1;

    for (i = 0; i < num_val_2_short - 1; i += 2) {
      pair_vec[i + 1][0] = in_data_2_short[i];
      pair_vec[i + 1][1] = in_data_2_short[i + 1];

      lav_fp[1] = max(lav_fp[1], abs(pair_vec[i + 1][0]));
      lav_fp[1] = max(lav_fp[1], abs(pair_vec[i + 1][1]));
    }

    tab_idx_2D[1][0] = (diff_type_2 == DIFF_TIME) ? 1 : 0;
    tab_idx_2D[1][1] = (diff_type_2 == DIFF_PILOT) ? 1 : 0;

    tab_idx_1D[1] = (diff_type_2 == DIFF_FREQ) ? 0 : 1;

    lav_fp[1] = get_next_lav_step(lav_fp[1], data_type);

    if (lav_fp[1] != -1) bit_count_2D_freq += lavHuffLen[lav_fp[1]];
  }

  if ((lav_fp[0] != -1) && (lav_fp[1] != -1)) {
    switch (data_type) {
      case t_CLD:
        if (in_data_1 != NULL) {
          bit_count_2D_freq += count_huff_cld_2D(&huffCLDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_fp[0], pair_vec, num_val_1_short, 2, p0_data_1);
          if (df_rest_flag[0]) bit_count_2D_freq += count_huff_cld_1D(&huffCLDTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
        }
        if (in_data_2 != NULL) {
          bit_count_2D_freq += count_huff_cld_2D(&huffCLDTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_fp[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
          if (df_rest_flag[1]) bit_count_2D_freq += count_huff_cld_1D(&huffCLDTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
        }
        break;

      case t_ICC:
        if (in_data_1 != NULL) {
          bit_count_2D_freq += count_huff_icc_2D(&huffICCTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_fp[0], pair_vec, num_val_1_short, 2, p0_data_1);
          if (df_rest_flag[0]) bit_count_2D_freq += count_huff_icc_1D(&huffICCTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
        }
        if (in_data_2 != NULL) {
          bit_count_2D_freq += count_huff_icc_2D(&huffICCTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_fp[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
          if (df_rest_flag[1]) bit_count_2D_freq += count_huff_icc_1D(&huffICCTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
        }
        break;

      case t_IPD:
        if (in_data_1 != NULL) {
          bit_count_2D_freq += count_huff_ipd_2D(&huffIPDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_fp[0], pair_vec, num_val_1_short, 2, p0_data_1);
          if (df_rest_flag[0]) bit_count_2D_freq += count_huff_ipd_1D(&huffIPDTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
        }
        if (in_data_2 != NULL) {
          bit_count_2D_freq += count_huff_ipd_2D(&huffIPDTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_fp[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
          if (df_rest_flag[1]) bit_count_2D_freq += count_huff_ipd_1D(&huffIPDTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
        }
        break;

      default:
        break;
    }

    if (bit_count_2D_freq < bit_count_min) {
      bit_count_min = bit_count_2D_freq;
      *cdg_scheme = CODING_SCHEME_HUFF_2D << PAIR_SHIFT | FREQ_PAIR;
      lav_idx[0] = lav_fp[0];
      lav_idx[1] = lav_fp[1];
    }
  }

  if (mps_mode != MPS_MODE_LD) {
    if ((in_data_1 != NULL) && (in_data_2 != NULL)) {
      bit_count_2D_time = 2;

      num_val_1_short = num_val;

      switch (diff_type_1) {
        case DIFF_FREQ:
          in_data_1_short = in_data_1;
          break;
        case DIFF_PILOT:
          in_data_1_short = in_data_1 + 1;
          break;
        case DIFF_TIME:
          in_data_1_short = in_data_1 + 2;
          break;
      }

      switch (diff_type_2) {
        case DIFF_FREQ:
          in_data_2_short = in_data_2;
          break;
        case DIFF_PILOT:
          in_data_2_short = in_data_2 + 1;
          break;
        case DIFF_TIME:
          in_data_2_short = in_data_2 + 2;
          break;
      }

      lav_tp = 0;

      p0_data_1[0] = NULL;
      p0_data_1[1] = NULL;

      if ((diff_type_1 == DIFF_FREQ) || (diff_type_2 == DIFF_FREQ)) {
        p0_data_1[0] = &in_data_1[0];
        p0_data_1[1] = &in_data_2[0];

        in_data_1_short += 1;
        in_data_2_short += 1;

        num_val_1_short -= 1;
      }

      for (i = 0; i < num_val_1_short; i++) {
        pair_vec[i][0] = in_data_1_short[i];
        pair_vec[i][1] = in_data_2_short[i];

        lav_tp = max(lav_tp, abs(pair_vec[i][0]));
        lav_tp = max(lav_tp, abs(pair_vec[i][1]));
      }

      tab_idx_2D[0][0] = ((diff_type_1 == DIFF_TIME) || (diff_type_2 == DIFF_TIME)) ? 1 : 0;
      tab_idx_2D[0][1] = 1;

      lav_tp = get_next_lav_step(lav_tp, data_type);

      if (lav_tp != -1) {
        bit_count_2D_time += lavHuffLen[lav_tp];

        switch (data_type) {
          case t_CLD:
            bit_count_2D_time += count_huff_cld_2D(&huffCLDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_tp, pair_vec, num_val_1_short, 1, p0_data_1);
            break;

          case t_ICC:
            bit_count_2D_time += count_huff_icc_2D(&huffICCTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_tp, pair_vec, num_val_1_short, 1, p0_data_1);
            break;

          case t_IPD:
            bit_count_2D_time += count_huff_ipd_2D(&huffIPDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_tp, pair_vec, num_val_1_short, 1, p0_data_1);
            break;

          default:
            break;
        }

        if (bit_count_2D_time < bit_count_min) {
          bit_count_min = bit_count_2D_time;
          *cdg_scheme = CODING_SCHEME_HUFF_2D << PAIR_SHIFT | TIME_PAIR;
          lav_idx[0] = lav_idx[1] = lav_tp;
        }
      }
    }
  }

  return bit_count_min;
}

static void apply_huff_coding(HANDLE_BIT_BUF strm,
                              short* in_data_1,
                              short* in_data_2,
                              DATA_TYPE data_type,
                              DIFF_TYPE diff_type_1,
                              DIFF_TYPE diff_type_2,
                              short num_val,
                              short* lav_idx,
                              short cdg_scheme,
                              MPS_MODE mps_mode) {
  short tab_idx_2D[2][2] = {{0}};
  short tab_idx_1D[2] = {0};
  short df_rest_flag[2] = {0};
  short p0_flag[2] = {0};

  short pair_vec[MAXBANDS][2] = {{0}};

  short* p0_data_1[2] = {NULL};
  short* p0_data_2[2] = {NULL};

  short i = 0;

  short num_val_1_short = num_val;
  short num_val_2_short = num_val;

  short* in_data_1_short = NULL;
  short* in_data_2_short = NULL;

  if (in_data_1 != NULL) {
    switch (diff_type_1) {
      case DIFF_FREQ:
        in_data_1_short = in_data_1;
        break;
      case DIFF_PILOT:
        in_data_1_short = in_data_1 + 1;
        break;
      case DIFF_TIME:
        in_data_1_short = in_data_1 + 2;
        break;
    }
  }

  if (in_data_2 != NULL) {
    switch (diff_type_2) {
      case DIFF_FREQ:
        in_data_2_short = in_data_2;
        break;
      case DIFF_PILOT:
        in_data_2_short = in_data_2 + 1;
        break;
      case DIFF_TIME:
        in_data_2_short = in_data_2 + 2;
        break;
    }
  }

  WriteBits(strm, cdg_scheme >> PAIR_SHIFT, 1);
  if (mps_mode != MPS_MODE_LD) {
    if ((cdg_scheme >> PAIR_SHIFT == CODING_SCHEME_HUFF_2D) && (in_data_1 != NULL) && (in_data_2 != NULL)) {
      WriteBits(strm, cdg_scheme & PAIR_MASK, 1);
    }
  }

  switch (cdg_scheme >> PAIR_SHIFT) {
    case CODING_SCHEME_HUFF_1D:

      p0_flag[0] = (diff_type_1 == DIFF_FREQ);
      p0_flag[1] = (diff_type_2 == DIFF_FREQ);

      tab_idx_1D[0] = (diff_type_1 == DIFF_FREQ) ? 0 : 1;
      tab_idx_1D[1] = (diff_type_2 == DIFF_FREQ) ? 0 : 1;

      switch (data_type) {
        case t_CLD:
          if (in_data_1 != NULL) huff_enc_cld_1D(strm, &huffCLDTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
          if (in_data_2 != NULL) huff_enc_cld_1D(strm, &huffCLDTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
          break;

        case t_ICC:
          if (in_data_1 != NULL) huff_enc_icc_1D(strm, &huffICCTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
          if (in_data_2 != NULL) huff_enc_icc_1D(strm, &huffICCTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
          break;

        case t_IPD:
          if (in_data_1 != NULL) huff_enc_ipd_1D(strm, &huffIPDTab.h1D[tab_idx_1D[0]], in_data_1_short, num_val_1_short, p0_flag[0]);
          if (in_data_2 != NULL) huff_enc_ipd_1D(strm, &huffIPDTab.h1D[tab_idx_1D[1]], in_data_2_short, num_val_2_short, p0_flag[1]);
          break;

        default:

          assert(0);
          break;
      }

      break;

    case CODING_SCHEME_HUFF_2D:

      switch (cdg_scheme & PAIR_MASK) {
        case FREQ_PAIR:

          if (in_data_1 != NULL) {
            if (diff_type_1 == DIFF_FREQ) {
              p0_data_1[0] = &in_data_1[0];
              p0_data_1[1] = NULL;

              num_val_1_short -= 1;
              in_data_1_short += 1;
            }

            df_rest_flag[0] = num_val_1_short % 2;

            if (df_rest_flag[0]) num_val_1_short -= 1;

            for (i = 0; i < num_val_1_short - 1; i += 2) {
              pair_vec[i][0] = in_data_1_short[i];
              pair_vec[i][1] = in_data_1_short[i + 1];
            }

            tab_idx_2D[0][0] = (diff_type_1 == DIFF_TIME) ? 1 : 0;
            tab_idx_2D[0][1] = (diff_type_1 == DIFF_PILOT) ? 1 : 0;

            tab_idx_1D[0] = (diff_type_1 == DIFF_FREQ) ? 0 : 1;
          }

          if (in_data_2 != NULL) {
            if (diff_type_2 == DIFF_FREQ) {
              p0_data_2[0] = NULL;
              p0_data_2[1] = &in_data_2[0];

              num_val_2_short -= 1;
              in_data_2_short += 1;
            }

            df_rest_flag[1] = num_val_2_short % 2;

            if (df_rest_flag[1]) num_val_2_short -= 1;

            for (i = 0; i < num_val_2_short - 1; i += 2) {
              pair_vec[i + 1][0] = in_data_2_short[i];
              pair_vec[i + 1][1] = in_data_2_short[i + 1];
            }

            tab_idx_2D[1][0] = (diff_type_2 == DIFF_TIME) ? 1 : 0;
            tab_idx_2D[1][1] = (diff_type_2 == DIFF_PILOT) ? 1 : 0;

            tab_idx_1D[1] = (diff_type_2 == DIFF_FREQ) ? 0 : 1;
          }

          switch (data_type) {
            case t_CLD:
              if (in_data_1 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[0]], lavHuffLen[lav_idx[0]]);
                huff_enc_cld_2D(strm, &huffCLDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 2, p0_data_1);
                if (df_rest_flag[0]) huff_enc_cld_1D(strm, &huffCLDTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
              }
              if (in_data_2 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[1]], lavHuffLen[lav_idx[1]]);
                huff_enc_cld_2D(strm, &huffCLDTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_idx[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
                if (df_rest_flag[1]) huff_enc_cld_1D(strm, &huffCLDTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
              }
              break;

            case t_ICC:
              if (in_data_1 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[0]], lavHuffLen[lav_idx[0]]);
                huff_enc_icc_2D(strm, &huffICCTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 2, p0_data_1);
                if (df_rest_flag[0]) huff_enc_icc_1D(strm, &huffICCTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
              }
              if (in_data_2 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[1]], lavHuffLen[lav_idx[1]]);
                huff_enc_icc_2D(strm, &huffICCTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_idx[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
                if (df_rest_flag[1]) huff_enc_icc_1D(strm, &huffICCTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
              }
              break;

            case t_IPD:
              if (in_data_1 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[0]], lavHuffLen[lav_idx[0]]);
                huff_enc_ipd_2D(strm, &huffIPDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 2, p0_data_1);
                if (df_rest_flag[0]) huff_enc_ipd_1D(strm, &huffIPDTab.h1D[tab_idx_1D[0]], in_data_1_short + num_val_1_short, 1, 0);
              }
              if (in_data_2 != NULL) {
                WriteBits(strm, lavHuffVal[lav_idx[1]], lavHuffLen[lav_idx[1]]);
                huff_enc_ipd_2D(strm, &huffIPDTab.h2D[tab_idx_2D[1][0]][tab_idx_2D[1][1]], lav_idx[1], pair_vec + 1, num_val_2_short, 2, p0_data_2);
                if (df_rest_flag[1]) huff_enc_ipd_1D(strm, &huffIPDTab.h1D[tab_idx_1D[1]], in_data_2_short + num_val_2_short, 1, 0);
              }
              break;

            default:
              break;
          }

          break;

        case TIME_PAIR:

          if ((diff_type_1 == DIFF_FREQ) || (diff_type_2 == DIFF_FREQ)) {
            p0_data_1[0] = &in_data_1[0];
            p0_data_1[1] = &in_data_2[0];

            in_data_1_short += 1;
            in_data_2_short += 1;

            num_val_1_short -= 1;
          }

          for (i = 0; i < num_val_1_short; i++) {
            pair_vec[i][0] = in_data_1_short[i];
            pair_vec[i][1] = in_data_2_short[i];
          }

          tab_idx_2D[0][0] = ((diff_type_1 == DIFF_TIME) || (diff_type_2 == DIFF_TIME)) ? 1 : 0;
          tab_idx_2D[0][1] = 1;

          WriteBits(strm, lavHuffVal[lav_idx[0]], lavHuffLen[lav_idx[0]]);

          switch (data_type) {
            case t_CLD:
              huff_enc_cld_2D(strm, &huffCLDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 1, p0_data_1);
              break;

            case t_ICC:
              huff_enc_icc_2D(strm, &huffICCTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 1, p0_data_1);
              break;

            case t_IPD:
              huff_enc_ipd_2D(strm, &huffIPDTab.h2D[tab_idx_2D[0][0]][tab_idx_2D[0][1]], lav_idx[0], pair_vec, num_val_1_short, 1, p0_data_1);
              break;

            default:
              break;
          }

          break;
      }

      break;

    default:
      break;
  }
}

int EcDataPairEnc(HANDLE_BIT_BUF strm,
                  short aaInData[][MAXBANDS],
                  short aHistory[MAXBANDS],
                  DATA_TYPE data_type,
                  int setIdx,
                  int startBand,
                  int dataBands,
                  int coarse_flag,
                  int independency_flag,
                  MPS_MODE mps_mode) {
  short reset = 0, pb = 0;
  short quant_levels = 0, quant_offset = 0, num_pcm_val = 0;

  short splitLsb_flag = 0;
  short pcmCoding_flag = 0;
  short pilotCoding_flag = 0;

  short allowDiffTimeBack_flag = !independency_flag || (setIdx > 0);
  short allowPilotBasedCdg_flag = 0;
  short pbc_applicable = 0;

  short num_lsb_bits = -1;
  short num_pcm_bits = -1;
  short num_pilot_bits = -1;

  short quant_data_lsb[2][MAXBANDS];
  short quant_data_msb[2][MAXBANDS];

  short quant_data_hist_lsb[MAXBANDS];
  short quant_data_hist_msb[MAXBANDS];

  short data_diff_freq[2][MAXBANDS];
  short data_diff_time[2][MAXBANDS + 2];

  short* p_quant_data_msb[2];
  short* p_quant_data_hist_msb = NULL;

  short min_bits_all = 0;
  short min_found = 0;

  short min_bits_df_df = -1;
  short min_bits_df_dt = -1;
  short min_bits_dtbw_df = -1;
  short min_bits_dtfw_df = -1;
  short min_bits_dt_dt = -1;

  short lav_df_df[2] = {-1, -1};
  short lav_df_dt[2] = {-1, -1};
  short lav_dtbw_df[2] = {-1, -1};
  short lav_dtfw_df[2] = {-1, -1};
  short lav_dt_dt[2] = {-1, -1};

  short coding_scheme_df_df = 0;
  short coding_scheme_df_dt = 0;
  short coding_scheme_dtbw_df = 0;
  short coding_scheme_dtfw_df = 0;
  short coding_scheme_dt_dt = 0;

  switch (data_type) {
    case t_CLD:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 15;
        quant_offset = 7;
      } else {
        splitLsb_flag = 0;
        quant_levels = 31;
        quant_offset = 15;
      }

      break;

    case t_ICC:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 4;
        quant_offset = 0;
      } else {
        splitLsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      }

      break;

    case t_IPD:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      } else {
        splitLsb_flag = 1;
        quant_levels = 16;
        quant_offset = 0;
      }

      break;

    default:

      assert(0);
      return 0;
  }

  if (splitLsb_flag) {
    split_lsb(aaInData[setIdx] + startBand,
              quant_offset,
              dataBands,
              quant_data_lsb[0],
              quant_data_msb[0]);

    split_lsb(aaInData[setIdx + 1] + startBand,
              quant_offset,
              dataBands,
              quant_data_lsb[1],
              quant_data_msb[1]);

    p_quant_data_msb[0] = quant_data_msb[0];
    p_quant_data_msb[1] = quant_data_msb[1];

    num_lsb_bits = 2 * dataBands;
  } else if (quant_offset != 0) {
    for (pb = 0; pb < dataBands; pb++) {
      quant_data_msb[0][pb] = aaInData[setIdx][startBand + pb] + quant_offset;
      quant_data_msb[1][pb] = aaInData[setIdx + 1][startBand + pb] + quant_offset;
    }

    p_quant_data_msb[0] = quant_data_msb[0];
    p_quant_data_msb[1] = quant_data_msb[1];

    num_lsb_bits = 0;
  } else {
    p_quant_data_msb[0] = aaInData[setIdx] + startBand;
    p_quant_data_msb[1] = aaInData[setIdx + 1] + startBand;

    num_lsb_bits = 0;
  }

  if (allowDiffTimeBack_flag) {
    if (splitLsb_flag) {
      split_lsb(aHistory + startBand,
                quant_offset,
                dataBands,
                quant_data_hist_lsb,
                quant_data_hist_msb);

      p_quant_data_hist_msb = quant_data_hist_msb;
    } else if (quant_offset != 0) {
      for (pb = 0; pb < dataBands; pb++) {
        quant_data_hist_msb[pb] = aHistory[startBand + pb] + quant_offset;
      }
      p_quant_data_hist_msb = quant_data_hist_msb;
    } else {
      p_quant_data_hist_msb = aHistory + startBand;
    }
  }

  calc_diff_freq(p_quant_data_msb[0],
                 data_diff_freq[0],
                 dataBands);

  calc_diff_freq(p_quant_data_msb[1],
                 data_diff_freq[1],
                 dataBands);

  if (data_type == t_IPD) {
    wrapIPDs(data_diff_freq[0], dataBands);
    wrapIPDs(data_diff_freq[1], dataBands);
  }

  if (allowDiffTimeBack_flag) {
    calc_diff_time(p_quant_data_msb[0],
                   p_quant_data_hist_msb,
                   data_diff_time[0],
                   dataBands);
  }

  calc_diff_time(p_quant_data_msb[1],
                 p_quant_data_msb[0],
                 data_diff_time[1],
                 dataBands);

  if (data_type == t_IPD) {
    wrapIPDs(data_diff_time[0] + 2, dataBands);
    wrapIPDs(data_diff_time[1] + 2, dataBands);
  }

  num_pcm_bits = calc_pcm_bits(2 * dataBands, quant_levels);
  num_pcm_val = 2 * dataBands;

  min_bits_all = num_pcm_bits;

  {
    min_bits_df_df = calc_huff_bits(data_diff_freq[0],
                                    data_diff_freq[1],
                                    data_type,
                                    DIFF_FREQ, DIFF_FREQ,
                                    dataBands,
                                    lav_df_df,
                                    &coding_scheme_df_df,
                                    mps_mode);

    min_bits_df_df += 2;

    min_bits_df_df += num_lsb_bits;

    if (min_bits_df_df < min_bits_all) {
      min_bits_all = min_bits_df_df;
    }
  }

  {
    min_bits_df_dt = calc_huff_bits(data_diff_freq[0],
                                    data_diff_time[1],
                                    data_type,
                                    DIFF_FREQ, DIFF_TIME,
                                    dataBands,
                                    lav_df_dt,
                                    &coding_scheme_df_dt,
                                    mps_mode);

    min_bits_df_dt += 2;

    min_bits_df_dt += num_lsb_bits;

    if (min_bits_df_dt < min_bits_all) {
      min_bits_all = min_bits_df_dt;
    }
  }

  if (mps_mode != MPS_MODE_LD) {
    {
      swap(data_diff_time[1]);
      min_bits_dtfw_df = calc_huff_bits(data_diff_time[1],
                                        data_diff_freq[1],
                                        data_type,
                                        DIFF_TIME, DIFF_FREQ,
                                        dataBands,
                                        lav_dtfw_df,
                                        &coding_scheme_dtfw_df,
                                        mps_mode);
      swap(data_diff_time[1]);

      if (allowDiffTimeBack_flag) {
        min_bits_dtfw_df += 3;
      } else {
        min_bits_dtfw_df += 1;
      }

      min_bits_dtfw_df += num_lsb_bits;

      if (min_bits_dtfw_df < min_bits_all) {
        min_bits_all = min_bits_dtfw_df;
      }
    }
  }

  if (allowDiffTimeBack_flag) {
    {
      min_bits_dtbw_df = calc_huff_bits(data_diff_time[0],
                                        data_diff_freq[1],
                                        data_type,
                                        DIFF_TIME, DIFF_FREQ,
                                        dataBands,
                                        lav_dtbw_df,
                                        &coding_scheme_dtbw_df,
                                        mps_mode);

      if (mps_mode != MPS_MODE_LD) {
        min_bits_dtbw_df += 3;
      } else {
        min_bits_dtbw_df += 2;
      }

      min_bits_dtbw_df += num_lsb_bits;

      if (min_bits_dtbw_df < min_bits_all) {
        min_bits_all = min_bits_dtbw_df;
      }
    }

    {
      min_bits_dt_dt = calc_huff_bits(data_diff_time[0],
                                      data_diff_time[1],
                                      data_type,
                                      DIFF_TIME, DIFF_TIME,
                                      dataBands,
                                      lav_dt_dt,
                                      &coding_scheme_dt_dt,
                                      mps_mode);

      min_bits_dt_dt += 2;

      min_bits_dt_dt += num_lsb_bits;

      if (min_bits_dt_dt < min_bits_all) {
        min_bits_all = min_bits_dt_dt;
      }
    }
  }

  if (!pbc_applicable) {
    pcmCoding_flag = (min_bits_all == num_pcm_bits);
  } else {
    pcmCoding_flag = (min_bits_all == min(num_pcm_bits, num_pilot_bits));
  }

  WriteBits(strm, pcmCoding_flag, 1);

  if (pcmCoding_flag) {
    if (!allowPilotBasedCdg_flag) {
      pilotCoding_flag = 0;
    } else {
      pilotCoding_flag = pbc_applicable ? (num_pilot_bits < num_pcm_bits) : 0;
      WriteBits(strm, pilotCoding_flag, 1);
    }

    if (!pilotCoding_flag) {
      apply_pcm_coding(strm,
                       aaInData[setIdx] + startBand,
                       aaInData[setIdx + 1] + startBand,
                       quant_offset,
                       num_pcm_val,
                       quant_levels);
    }

  } else {
    min_found = 0;

    if (min_bits_all == min_bits_df_df) {
      WriteBits(strm, DIFF_FREQ, 1);
      WriteBits(strm, DIFF_FREQ, 1);

      apply_huff_coding(strm,
                        data_diff_freq[0],
                        data_diff_freq[1],
                        data_type,
                        DIFF_FREQ, DIFF_FREQ,
                        dataBands,
                        lav_df_df,
                        coding_scheme_df_df,
                        mps_mode);

      min_found = 1;
    }

    if (!min_found && (min_bits_all == min_bits_df_dt)) {
      WriteBits(strm, DIFF_FREQ, 1);
      WriteBits(strm, DIFF_TIME, 1);

      apply_huff_coding(strm,
                        data_diff_freq[0],
                        data_diff_time[1],
                        data_type,
                        DIFF_FREQ, DIFF_TIME,
                        dataBands,
                        lav_df_dt,
                        coding_scheme_df_dt,
                        mps_mode);

      min_found = 1;
    }

    if (mps_mode != MPS_MODE_LD) {
      if (!min_found && (min_bits_all == min_bits_dtfw_df)) {
        WriteBits(strm, DIFF_TIME, 1);
        if (allowDiffTimeBack_flag) {
          WriteBits(strm, DIFF_FREQ, 1);
        }

        swap(data_diff_time[1]);
        apply_huff_coding(strm,
                          data_diff_time[1],
                          data_diff_freq[1],
                          data_type,
                          DIFF_TIME, DIFF_FREQ,
                          dataBands,
                          lav_dtfw_df,
                          coding_scheme_dtfw_df,
                          mps_mode);

        if (allowDiffTimeBack_flag) {
          WriteBits(strm, FORWARDS, 1);
        }

        min_found = 1;
      }
    }

    if (allowDiffTimeBack_flag) {
      if (!min_found && (min_bits_all == min_bits_dtbw_df)) {
        WriteBits(strm, DIFF_TIME, 1);
        WriteBits(strm, DIFF_FREQ, 1);

        apply_huff_coding(strm,
                          data_diff_time[0],
                          data_diff_freq[1],
                          data_type,
                          DIFF_TIME, DIFF_FREQ,
                          dataBands,
                          lav_dtbw_df,
                          coding_scheme_dtbw_df,
                          mps_mode);

        if (mps_mode != MPS_MODE_LD) {
          WriteBits(strm, BACKWARDS, 1);
        }

        min_found = 1;
      }

      if (!min_found && (min_bits_all == min_bits_dt_dt)) {
        WriteBits(strm, DIFF_TIME, 1);
        WriteBits(strm, DIFF_TIME, 1);

        apply_huff_coding(strm,
                          data_diff_time[0],
                          data_diff_time[1],
                          data_type,
                          DIFF_TIME, DIFF_TIME,
                          dataBands,
                          lav_dt_dt,
                          coding_scheme_dt_dt,
                          mps_mode);
      }
    }

    if (splitLsb_flag) {
      apply_lsb_coding(strm,
                       quant_data_lsb[0],
                       1,
                       dataBands);

      apply_lsb_coding(strm,
                       quant_data_lsb[1],
                       1,
                       dataBands);
    }
  }

  return reset;
}

int EcDataSingleEnc(HANDLE_BIT_BUF strm,
                    short aaInData[][MAXBANDS],
                    short aHistory[MAXBANDS],
                    DATA_TYPE data_type,
                    int setIdx,
                    int startBand,
                    int dataBands,
                    int coarse_flag,
                    int independency_flag,
                    MPS_MODE mps_mode) {
  short reset = 0, pb = 0;
  short quant_levels = 0, quant_offset = 0, num_pcm_val = 0;

  short splitLsb_flag = 0;
  short pcmCoding_flag = 0;
  short pilotCoding_flag = 0;

  short allowDiffTimeBack_flag = !independency_flag || (setIdx > 0);
  short allowPilotBasedCdg_flag = 0;
  short pbc_applicable = 0;

  short num_lsb_bits = -1;
  short num_pcm_bits = -1;
  short num_pilot_bits = -1;

  short quant_data_lsb[MAXBANDS];
  short quant_data_msb[MAXBANDS];

  short quant_data_hist_lsb[MAXBANDS];
  short quant_data_hist_msb[MAXBANDS];

  short data_diff_freq[MAXBANDS];
  short data_diff_time[MAXBANDS + 2];

  short* p_quant_data_msb = NULL;
  short* p_quant_data_hist_msb = NULL;

  short min_bits_all = 0;
  short min_found = 0;

  short min_bits_df = -1;
  short min_bits_dt = -1;

  short lav_df[2] = {-1, -1};
  short lav_dt[2] = {-1, -1};

  short coding_scheme_df = 0;
  short coding_scheme_dt = 0;

  switch (data_type) {
    case t_CLD:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 15;
        quant_offset = 7;
      } else {
        splitLsb_flag = 0;
        quant_levels = 31;
        quant_offset = 15;
      }

      break;

    case t_ICC:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 4;
        quant_offset = 0;
      } else {
        splitLsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      }

      break;

    case t_IPD:
      if (coarse_flag) {
        splitLsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      } else {
        splitLsb_flag = 1;
        quant_levels = 16;
        quant_offset = 0;
      }

      break;

    default:

      assert(0);
      return 0;
  }

  if (splitLsb_flag) {
    split_lsb(aaInData[setIdx] + startBand,
              quant_offset,
              dataBands,
              quant_data_lsb,
              quant_data_msb);

    p_quant_data_msb = quant_data_msb;
    num_lsb_bits = dataBands;
  } else if (quant_offset != 0) {
    for (pb = 0; pb < dataBands; pb++) {
      quant_data_msb[pb] = aaInData[setIdx][startBand + pb] + quant_offset;
    }

    p_quant_data_msb = quant_data_msb;
    num_lsb_bits = 0;
  } else {
    p_quant_data_msb = aaInData[setIdx] + startBand;
    num_lsb_bits = 0;
  }

  if (allowDiffTimeBack_flag) {
    if (splitLsb_flag) {
      split_lsb(aHistory + startBand,
                quant_offset,
                dataBands,
                quant_data_hist_lsb,
                quant_data_hist_msb);

      p_quant_data_hist_msb = quant_data_hist_msb;
    } else if (quant_offset != 0) {
      for (pb = 0; pb < dataBands; pb++) {
        quant_data_hist_msb[pb] = aHistory[startBand + pb] + quant_offset;
      }
      p_quant_data_hist_msb = quant_data_hist_msb;
    } else {
      p_quant_data_hist_msb = aHistory + startBand;
    }
  }

  calc_diff_freq(p_quant_data_msb,
                 data_diff_freq,
                 dataBands);

  if (data_type == t_IPD) {
    wrapIPDs(data_diff_freq, dataBands);
  }

  if (allowDiffTimeBack_flag) {
    calc_diff_time(p_quant_data_msb,
                   p_quant_data_hist_msb,
                   data_diff_time,
                   dataBands);
    if (data_type == t_IPD) {
      wrapIPDs(data_diff_time + 2, dataBands);
    }
  }

  num_pcm_bits = calc_pcm_bits(dataBands, quant_levels);
  num_pcm_val = dataBands;

  min_bits_all = num_pcm_bits;

  {
    min_bits_df = calc_huff_bits(data_diff_freq,
                                 NULL,
                                 data_type,
                                 DIFF_FREQ, DIFF_FREQ,
                                 dataBands,
                                 lav_df,
                                 &coding_scheme_df,
                                 mps_mode);

    if (allowDiffTimeBack_flag) min_bits_df += 1;

    min_bits_df += num_lsb_bits;

    if (min_bits_df < min_bits_all) {
      min_bits_all = min_bits_df;
    }
  }

  if (allowDiffTimeBack_flag) {
    min_bits_dt = calc_huff_bits(data_diff_time,
                                 NULL,
                                 data_type,
                                 DIFF_TIME, DIFF_TIME,
                                 dataBands,
                                 lav_dt,
                                 &coding_scheme_dt,
                                 mps_mode);

    min_bits_dt += 1;
    min_bits_dt += num_lsb_bits;

    if (min_bits_dt < min_bits_all) {
      min_bits_all = min_bits_dt;
    }
  }

  if (!pbc_applicable) {
    pcmCoding_flag = (min_bits_all == num_pcm_bits);
  } else {
    pcmCoding_flag = (min_bits_all == min(num_pcm_bits, num_pilot_bits));
  }

  WriteBits(strm, pcmCoding_flag, 1);

  if (pcmCoding_flag) {
    if (!allowPilotBasedCdg_flag) {
      pilotCoding_flag = 0;
    }

    if (!pilotCoding_flag) {
      apply_pcm_coding(strm,
                       aaInData[setIdx] + startBand,
                       NULL,
                       quant_offset,
                       num_pcm_val,
                       quant_levels);
    }

  } else {
    min_found = 0;

    if (min_bits_all == min_bits_df) {
      if (allowDiffTimeBack_flag) {
        WriteBits(strm, DIFF_FREQ, 1);
      }

      apply_huff_coding(strm,
                        data_diff_freq,
                        NULL,
                        data_type,
                        DIFF_FREQ, DIFF_FREQ,
                        dataBands,
                        lav_df,
                        coding_scheme_df,
                        mps_mode);

      min_found = 1;
    }

    if (allowDiffTimeBack_flag) {
      if (!min_found && (min_bits_all == min_bits_dt)) {
        WriteBits(strm, DIFF_TIME, 1);

        apply_huff_coding(strm,
                          data_diff_time,
                          NULL,
                          data_type,
                          DIFF_TIME, DIFF_TIME,
                          dataBands,
                          lav_dt,
                          coding_scheme_dt,
                          mps_mode);
      }
    }

    if (splitLsb_flag) {
      apply_lsb_coding(strm,
                       quant_data_lsb,
                       1,
                       dataBands);
    }
  }

  return reset;
}
