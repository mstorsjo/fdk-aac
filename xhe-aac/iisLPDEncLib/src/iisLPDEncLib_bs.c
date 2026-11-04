
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

#include <stdlib.h>

#include "iisLPDComLib_constants.h"
#include "iisLPDEncLib_bs.h"
#include "iisLPDComLib_acelp_Util.h"
#include "iisLPDEncLib_vq_RE8.h"

static int unpack8bits(int nbits, unsigned char const *bitBuffer, int ptr, unsigned char *serial);

static int get_num_prm(int qn1, int qn2) {
  return 2 + ((qn1 > 0) ? 9 : 0) + ((qn2 > 0) ? 9 : 0);
}

void LPDEnc_bs_EncodeMain(
    int mod[],
    int n_param_tcx[],
    unsigned char serial[],
    Coder_State_Plus *st,
    int *total_nbbits,
    int *SumSqBits,
    int const bUsacIndependencyFlag,
    const int disableFacForAcelp,
    LPD_CHANNEL_STREAM *lpd_channel_stream) {
  int k, n, mode, sfr, nbits, sqBits;
  int ptr;
  unsigned char const *bitBuffer = NULL;
  int skip;
  int nbits_fac = 0;
  int force_arith_reset = 0;

  if (bUsacIndependencyFlag) {
    LPDEnc_arithEncWrapper_SetForceReset(st->phArith);
  }

  ptr = 0;
  *total_nbbits = 0;

  if (lpd_channel_stream->acelp_core_mode >= MIN_ACELP_COREMODE && lpd_channel_stream->acelp_core_mode <= MAX_ACELP_COREMODE) {
    LPDEnc_bs_IntToBin((lpd_channel_stream->acelp_core_mode + 6) % 8, 3, ptr, serial);
    ptr += 3;
    *total_nbbits += 3;
  }

  LPDEnc_bs_IntToBin(lpd_channel_stream->lpd_mode, 5, ptr, serial);
  ptr += 5;
  *total_nbbits += 5;
  LPDEnc_bs_IntToBin(lpd_channel_stream->bpf_control_info, 1, ptr, serial);
  ptr += 1;
  *total_nbbits += 1;

  LPDEnc_bs_IntToBin(lpd_channel_stream->core_mode_last, 1, ptr, serial);
  ptr += 1;
  *total_nbbits += 1;

  LPDEnc_bs_IntToBin(lpd_channel_stream->fac_data_present, 1, ptr, serial);
  ptr += 1;
  *total_nbbits += 1;

  if ((st->lDiv) == ((3 * L_DIV_1024) / 4)) {
    nbits = (lpdcom_NBITS_CORE_768[lpd_channel_stream->acelp_core_mode] / 4) - 2;
  } else {
    nbits = (lpdcom_NBITS_CORE_1024[lpd_channel_stream->acelp_core_mode] / 4) - 2;
  }

  k = 0;
  *SumSqBits = 0;
  while (k < st->nbDiv) {
    mode = mod[k];

    skip = 1;
    if ((((mod[k - 1] == 0) && (mod[k] > 0)) || ((mod[k - 1] > 0) && (mod[k] == 0))) && !disableFacForAcelp) {
      skip = 0;
    }

    if (!skip) {
      nbits_fac = LPDEnc_bs_EncodeFac(lpd_channel_stream->fac_data[k], ptr, serial, (st->lDiv) / 2);
      ptr += nbits_fac;
      *total_nbbits += nbits_fac;
    }

    switch (mode) {
      case 0:

        LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_mean_energy[k], 2, ptr, serial);
        ptr += 2;

        for (sfr = 0; sfr < (st->nbSubfr); sfr++) {
          n = 6;
          if ((sfr == 0) || (((st->lDiv) == 256) && (sfr == 2))) {
            n = 9;
          }

          LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_acb_index[k][sfr], n, ptr, serial);
          ptr += n;

          LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_ltp_filtering[k][sfr], 1, ptr, serial);
          ptr += 1;

          if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 16) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 1, ptr, serial);
            ptr += 1;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 5, ptr, serial);
            ptr += 5;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 17) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 2, ptr, serial);
            ptr += 2;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 5, ptr, serial);
            ptr += 5;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 20) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 5, ptr, serial);
            ptr += 5;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 28) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 5, ptr, serial);
            ptr += 5;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 5, ptr, serial);
            ptr += 5;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 36) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 9, ptr, serial);
            ptr += 9;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 44) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 13, ptr, serial);
            ptr += 13;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 13, ptr, serial);
            ptr += 13;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 9, ptr, serial);
            ptr += 9;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 9, ptr, serial);
            ptr += 9;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 52) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 13, ptr, serial);
            ptr += 13;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 13, ptr, serial);
            ptr += 13;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 13, ptr, serial);
            ptr += 13;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 13, ptr, serial);
            ptr += 13;
          } else if (lpdcom_NBITS_FIXED[lpd_channel_stream->acelp_core_mode][sfr] == 64) {
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][0], 2, ptr, serial);
            ptr += 2;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][1], 2, ptr, serial);
            ptr += 2;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][2], 2, ptr, serial);
            ptr += 2;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][3], 2, ptr, serial);
            ptr += 2;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][4], 14, ptr, serial);
            ptr += 14;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][5], 14, ptr, serial);
            ptr += 14;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][6], 14, ptr, serial);
            ptr += 14;
            LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_icb_index[k][sfr][7], 14, ptr, serial);
            ptr += 14;
          }

          LPDEnc_bs_IntToBin(lpd_channel_stream->acelp_gains[k][sfr], 7, ptr, serial);
          ptr += 7;
        }
        *total_nbbits += (nbits - NBITS_LPC);

        break;
      case 1:
      case 2:
      case 3:

        LPDEnc_bs_IntToBin(lpd_channel_stream->tcx_noise_factor[k], 3, ptr, serial);
        ptr += 3;
        LPDEnc_bs_IntToBin(lpd_channel_stream->tcx_global_gain[k], 7, ptr, serial);
        ptr += 7;
        *total_nbbits += 10;
        force_arith_reset = lpd_channel_stream->tcx_force_arith_reset[k];

        sqBits = LPDEnc_arithEncWrapper_Encode(st->phArith, &bitBuffer, &lpd_channel_stream->tcx_quant[k * st->lDiv], n_param_tcx[k], &force_arith_reset, n_param_tcx[k] * 2);

        if (st->tcx_arith_reset_flag[k]) {
          LPDEnc_bs_IntToBin(force_arith_reset, 1, ptr, serial);
          ptr += 1;
          *total_nbbits += 1;
        }
        unpack8bits(sqBits, bitBuffer, ptr, serial);
        *total_nbbits += sqBits;
        ptr += sqBits;
        *SumSqBits += sqBits;
        break;
    }

    switch (mode) {
      case 0:
      case 1:
        k += 1;
        break;
      case 2:
        k += 2;
        break;
      case 3:
        k += 4;
        break;
    }
  }

  {
    int totalLpcBits = 0;
    LPDEnc_bs_encodeLPC(st->nbDiv, mod, lpd_channel_stream->core_mode_last, lpd_channel_stream->lpc_params, total_nbbits, &totalLpcBits, &ptr, serial);
  }
  if ((lpd_channel_stream->core_mode_last == 0) && (lpd_channel_stream->fac_data_present == 1)) {
    int short_fac_flag = (mod[-1] == -2) ? 1 : 0;

    LPDEnc_bs_IntToBin(short_fac_flag, 1, ptr, serial);
    ptr += 1;
    *total_nbbits += 1;
  }

  return;
}

static int unpack8bits(int nbits, unsigned char const *bitBuffer, int ptr, unsigned char *serial) {
  int i;
  int temp;
  i = 0;

  while (nbits > 8) {
    temp = (int)bitBuffer[i];
    LPDEnc_bs_IntToBin(temp, 8, ptr, serial);
    ptr += 8;
    nbits -= 8;
    i++;
  }

  temp = (int)bitBuffer[i] >> (8 - nbits);
  LPDEnc_bs_IntToBin(temp, nbits, ptr, serial);
  ptr += nbits;

  for (i = nbits; i < 8; i++) {
    LPDEnc_bs_IntToBin(0, 1, ptr, serial);
    ptr++;
  }

  return (i);
}

int LPDEnc_bs_EncodeFac(int *fac_data, int ptr, unsigned char *serial, int lFac) {
  int i, iii, n, nb, qn, kv[8] = {0}, nk, nbits_fac;
  int I;

  nbits_fac = 0;

  for (i = 0; i < lFac; i += 8) {
    LPDEnc_vq_getAVQindices(&fac_data[i], &qn, &I, kv);

    nb = LPDEnc_bs_UnaryCode(qn, ptr, serial);
    ptr += nb;

    nbits_fac += nb;

    nk = 0;
    n = qn;
    if (qn > 4) {
      nk = (qn - 3) >> 1;
      n = qn - nk * 2;
    }

    LPDEnc_bs_IntToBin(I, 4 * n, ptr, serial);
    ptr += 4 * n;

    for (iii = 0; iii < 8; iii++) {
      LPDEnc_bs_IntToBin(kv[iii], nk, ptr, serial);
      ptr += nk;
    }

    nbits_fac += 4 * qn;
  }
  return nbits_fac;
}
int LPDEnc_bs_UnaryCode(int ind, int ptr, unsigned char *serial) {
  int nb_bits;

  nb_bits = 1;

  ind -= 1;
  while (ind-- > 0) {
    LPDEnc_bs_IntToBin(1, 1, ptr, serial);
    ptr++;
    nb_bits++;
  }

  LPDEnc_bs_IntToBin(0, 1, ptr, serial);

  return (nb_bits);
}

void LPDEnc_bs_IntToBin(
    int value,
    int no_of_bits,
    int position,
    unsigned char *bitstream) {
  int positionBit, positionByte;
  int bitsLeft, bitsAvail;

  positionByte = (position + no_of_bits - 1) >> 3;
  positionBit = (position + no_of_bits - 1) & 7;

  bitsLeft = no_of_bits;
  bitsAvail = positionBit + 1;

  while (bitsLeft) {
    unsigned char nb, mask;
    int bitstoWrite = bitsAvail < bitsLeft ? bitsAvail : bitsLeft;

    mask = (0xff >> (8 - bitstoWrite)) << (8 - bitsAvail);
    nb = ((value << (8 - bitsAvail)) & mask) | (bitstream[positionByte] & (~mask));
    bitstream[positionByte--] = nb;
    value >>= bitstoWrite;
    bitsLeft -= bitstoWrite;
    bitsAvail = 8;
  }
}

HANDLE_ERROR_INFO LPDEnc_bs_encodeLPC(
    const short nbDiv,
    const int mod[],
    const int core_mode_last,
    const int lpc_params[],
    int *total_nbbits,
    int *totalLpcBits,
    int *position,
    unsigned char *serial) {
  int j = 0;
  int k;
  int q_type;
  int nb_ind = 0;
  int skip_avq = 0;
  int totalBitsBeforeLpc = *total_nbbits;
  int skip;
  int nb;
  int ptr = 0;
  int st1 = 0;
  int qn1 = 0;
  int qn2 = 0;
  HANDLE_ERROR_INFO error = noError;

  if (serial == NULL || position == NULL) {
    error = iisUtil_ERROR(CDI, "Invalid bitstream pointer");
  }

  if (error == noError) {
    ptr = *position;
  }

  if (error == noError) {
    for (k = 0; k < nbDiv + 1; k++) {
      if ((k == 1) && core_mode_last) k++;

      if (k == 0)
        q_type = 0;
      else {
        q_type = lpc_params[j];
        j++;
      }

      nb_ind = 0;
      skip_avq = 0;
      if ((k == 3) && (q_type == 1)) {
        skip_avq = 1;
      }

      if (!skip_avq) {
        if (q_type == 0) {
          st1 = lpc_params[j];
          j++;
        }
        qn1 = lpc_params[j];
        j++;
        qn2 = lpc_params[j];
        j++;
        nb_ind = get_num_prm(qn1, qn2) - 2;
      }

      skip = 1;
      if (k < 2) {
        skip = 0;
      }
      if ((k == 2) && (mod[0] < 3)) {
        skip = 0;
      }
      if ((k == 3) && (mod[0] < 2)) {
        skip = 0;
      }
      if ((k == 4) && (mod[2] < 2)) {
        skip = 0;
      }

      if (skip) {
        j += nb_ind;
      } else {
        if (k == 0) {
          nb = 0;
        } else if (k == 3) {
          if (q_type == 0) {
            nb = 2;
            LPDEnc_bs_IntToBin(2, nb, ptr, serial);
          } else if (q_type == 1) {
            nb = 2;
            LPDEnc_bs_IntToBin(3, nb, ptr, serial);
          } else {
            nb = 1;
            LPDEnc_bs_IntToBin(0, nb, ptr, serial);
          }
        } else if (k == 4) {
          if (q_type == 0) {
            nb = 2;
            LPDEnc_bs_IntToBin(2, nb, ptr, serial);
          } else if (q_type == 1) {
            nb = 1;
            LPDEnc_bs_IntToBin(0, nb, ptr, serial);
          } else if (q_type == 2) {
            nb = 3;
            LPDEnc_bs_IntToBin(6, nb, ptr, serial);
          } else {
            nb = 3;
            LPDEnc_bs_IntToBin(7, nb, ptr, serial);
          }
        } else {
          nb = 1;
          LPDEnc_bs_IntToBin(q_type, nb, ptr, serial);
        }

        ptr += nb;
        *total_nbbits += nb;

        if (!skip_avq) {
          if (q_type == 0) {
            LPDEnc_bs_IntToBin(st1, 8, ptr, serial);
            ptr += 8;
            *total_nbbits += 8;
          }

          if ((q_type == 1) && ((k == 3) || (k == 4))) {
            nb = LPDEnc_bs_UnaryCode(qn1, ptr, serial);
            ptr += nb;
            *total_nbbits += nb;

            nb = LPDEnc_bs_UnaryCode(qn2, ptr, serial);
            ptr += nb;
            *total_nbbits += nb;
          } else {
            int i;

            *total_nbbits += 4;

            i = qn1 - 2;
            if ((i < 0) || (i > 3)) i = 3;
            LPDEnc_bs_IntToBin(i, 2, ptr, serial);
            ptr += 2;

            i = qn2 - 2;
            if ((i < 0) || (i > 3)) i = 3;
            LPDEnc_bs_IntToBin(i, 2, ptr, serial);
            ptr += 2;
            if ((q_type > 1) && ((k == 3) || (k == 4))) {
              nb = qn1;
              if (nb > 4)
                nb -= 3;
              else if (nb == 0)
                nb = 1;
              else
                nb = 0;

              if (nb > 0) LPDEnc_bs_UnaryCode(nb, ptr, serial);
              ptr += nb;
              *total_nbbits += nb;

              nb = qn2;
              if (nb > 4)
                nb -= 3;
              else if (nb == 0)
                nb = 1;
              else
                nb = 0;

              if (nb > 0) {
                LPDEnc_bs_UnaryCode(nb, ptr, serial);
              }
              ptr += nb;
              *total_nbbits += nb;
            } else {
              nb = qn1;
              if (nb > 6)
                nb -= 3;
              else if (nb > 4)
                nb -= 4;
              else if (nb == 0)
                nb = 3;
              else
                nb = 0;

              if (nb > 0) LPDEnc_bs_UnaryCode(nb, ptr, serial);
              ptr += nb;
              *total_nbbits += nb;

              nb = qn2;
              if (nb > 6)
                nb -= 3;
              else if (nb > 4)
                nb -= 4;
              else if (nb == 0)
                nb = 3;
              else
                nb = 0;

              if (nb > 0) LPDEnc_bs_UnaryCode(nb, ptr, serial);
              ptr += nb;
              *total_nbbits += nb;
            }
          }

          {
            if (qn1 > 0) {
              int n;
              int nk;
              int i;

              nk = 0;
              n = qn1;
              if (qn1 > 4) {
                nk = (qn1 - 3) >> 1;
                n = qn1 - nk * 2;
              }

              LPDEnc_bs_IntToBin(lpc_params[j++], 4 * n, ptr, serial);
              ptr += 4 * n;

              for (i = 0; i < 8; i++) {
                LPDEnc_bs_IntToBin(lpc_params[j++], nk, ptr, serial);
                ptr += nk;
              }
              *total_nbbits += 4 * n + 8 * nk;
            }

            if (qn2 > 0) {
              int n;
              int nk;
              int i;
              nk = 0;
              n = qn2;
              if (qn2 > 4) {
                nk = (qn2 - 3) >> 1;
                n = qn2 - nk * 2;
              }

              LPDEnc_bs_IntToBin(lpc_params[j++], 4 * n, ptr, serial);
              ptr += 4 * n;

              for (i = 0; i < 8; i++) {
                LPDEnc_bs_IntToBin(lpc_params[j++], nk, ptr, serial);
                ptr += nk;
              }
              *total_nbbits += 4 * n + 8 * nk;
            }
          }
        }
      }
    }
    *totalLpcBits = *total_nbbits - totalBitsBeforeLpc;
    *position = ptr;
  }

  return error;
}
