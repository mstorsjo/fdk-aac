/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2001)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   $Id$
   Author(s):   Josef Hoepfl, DSP Solutions
   Description: Fix point FFT

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __FFT_H__
#define __FFT_H__

#include "common_fix.h"

/**
 * \brief Perform an inplace complex valued FFT of length 2^n
 *
 * \param length Length of the FFT to be calculated.
 * \param pInput Input/Output data buffer. The input data must have at least 1 bit scale headroom.
 *          The values are interleaved, real/imag pairs.
 * \param scalefactor Pointer to an INT, which contains the current scale of the input data,
 *                    which is updated according to the FFT scale.
 */
void fft(int length, FIXP_DBL *pInput, INT *scalefactor);

/**
 * \brief Perform an inplace complex valued IFFT of length 2^n
 *
 * \param length Length of the FFT to be calculated.
 * \param pInput Input/Output data buffer. The input data must have at least 1 bit scale headroom.
 *          The values are interleaved, real/imag pairs.
 * \param scalefactor Pointer to an INT, which contains the current scale of the input data,
 *                    which is updated according to the IFFT scale.
 */
void ifft(int length, FIXP_DBL *pInput, INT *scalefactor);


/*
 * Frequently used and fixed short length FFTs.
 */

LNK_SECTION_CODE_L1
static void FORCEINLINE fft_4(FIXP_DBL *x)
{
    FIXP_DBL a00, a10, a20, a30, tmp0, tmp1;

    a00 = (x[0] + x[4])>>1;  /* Re A + Re B */
    a10 = (x[2] + x[6])>>1;  /* Re C + Re D */
    a20 = (x[1] + x[5])>>1;  /* Im A + Im B */
    a30 = (x[3] + x[7])>>1;  /* Im C + Im D */

    x[0] = a00 + a10;       /* Re A' = Re A + Re B + Re C + Re D */
    x[1] = a20 + a30;       /* Im A' = Im A + Im B + Im C + Im D */

    tmp0 = a00 - x[4];       /* Re A - Re B */
    tmp1 = a20 - x[5];       /* Im A - Im B */

    x[4] = a00 - a10;       /* Re C' = Re A + Re B - Re C - Re D */
    x[5] = a20 - a30;       /* Im C' = Im A + Im B - Im C - Im D */

    a10 = a10 - x[6];       /* Re C - Re D */
    a30 = a30 - x[7];       /* Im C - Im D */

    x[2] = tmp0 + a30;       /* Re B' = Re A - Re B + Im C - Im D */
    x[6] = tmp0 - a30;       /* Re D' = Re A - Re B - Im C + Im D */
    x[3] = tmp1 - a10;       /* Im B' = Im A - Im B - Re C + Re D */
    x[7] = tmp1 + a10;       /* Im D' = Im A - Im B + Re C - Re D */
}

LNK_SECTION_CODE_L1
static void FORCEINLINE fft_8(FIXP_DBL *x)
{
      #define W_PiFOURTH STC(0x5a82799a)

      FIXP_DBL a00, a10, a20, a30;
      FIXP_DBL y[16];

      a00 = (x[0] + x[8])>>1;
      a10 =  x[4] + x[12];
      a20 = (x[1] + x[9])>>1;
      a30 =  x[5] + x[13];

      y[0] = a00 + (a10>>1);
      y[4] = a00 - (a10>>1);
      y[1] = a20 + (a30>>1);
      y[5] = a20 - (a30>>1);

      a00 = a00      - x[8];
      a10 = (a10>>1) - x[12];
      a20 = a20      - x[9];
      a30 = (a30>>1) - x[13];

      y[2] = a00 + a30;
      y[6] = a00 - a30;
      y[3] = a20 - a10;
      y[7] = a20 + a10;

      a00 = (x[2] + x[10])>>1;
      a10 =  x[6] + x[14];
      a20 = (x[3] + x[11])>>1;
      a30 =  x[7] + x[15];

      y[8]  = a00 + (a10>>1);
      y[12] = a00 - (a10>>1);
      y[9]  = a20 + (a30>>1);
      y[13] = a20 - (a30>>1);

      a00 = a00      - x[10];
      a10 = (a10>>1) - x[14];
      a20 = a20      - x[11];
      a30 = (a30>>1) - x[15];

      y[10] = a00 + a30;
      y[14] = a00 - a30;
      y[11] = a20 - a10;
      y[15] = a20 + a10;

      FIXP_DBL vr, vi, ur, ui;

      ur = y[0]>>1;
      ui = y[1]>>1;
      vr = y[8];
      vi = y[9];
      x[0] = ur + (vr>>1);
      x[1] = ui + (vi>>1);
      x[8] = ur - (vr>>1);
      x[9] = ui - (vi>>1);

      ur = y[4]>>1;
      ui = y[5]>>1;
      vi = y[12];
      vr = y[13];
      x[4]  = ur + (vr>>1);
      x[5]  = ui - (vi>>1);
      x[12] = ur - (vr>>1);
      x[13] = ui + (vi>>1);

      ur = y[10];
      ui = y[11];
      vr = fMultDiv2(ui+ur,W_PiFOURTH);
      vi = fMultDiv2(ui-ur,W_PiFOURTH);
      ur = y[2];
      ui = y[3];
      x[2]  = (ur>>1) + vr;
      x[3]  = (ui>>1) + vi;
      x[10] = (ur>>1) - vr;
      x[11] = (ui>>1) - vi;

      ur = y[14];
      ui = y[15];
      vr = fMultDiv2(ui-ur,W_PiFOURTH);
      vi = fMultDiv2(ui+ur,W_PiFOURTH);
      ur = y[6];
      ui = y[7];
      x[6]  = (ur>>1) + vr;
      x[7]  = (ui>>1) - vi;
      x[14] = (ur>>1) - vr;
      x[15] = (ui>>1) + vi;
}

/**
 * \brief FFT of fixed length 16
 */
inline void fft_16(FIXP_DBL *x);

/**
 * \brief FFT of fixed length 32
 */
inline void fft_32(FIXP_DBL *x);


#endif
