/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2005)
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
   Author(s):
   Description: dit_fft ARM assembler replacements.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

/* NEON optimized FFT currently builds only with RVCT toolchain */

#ifndef FUNCTION_dit_fft

/* If dit_fft was not yet defined by ARM-Cortex ... */

#if defined(SINETABLE_16BIT)

#define FUNCTION_dit_fft

/*****************************************************************************

   date:   28.07.2005   srl

   Contents/description: dit-tukey-FFT-algorithm

******************************************************************************/

#if defined(FUNCTION_dit_fft)


void dit_fft(FIXP_DBL *x, const INT ldn, const FIXP_STP *trigdata, const INT trigDataSize)
{
    const INT n=1<<ldn;
    INT i;

    scramble(x,n);
    /*
     * 1+2 stage radix 4
     */

    for (i=0;i<n*2;i+=8)
    {
      FIXP_DBL a00, a10, a20, a30;
      a00 = (x[i + 0] + x[i + 2])>>1;  /* Re A + Re B */
      a10 = (x[i + 4] + x[i + 6])>>1;  /* Re C + Re D */
      a20 = (x[i + 1] + x[i + 3])>>1;  /* Im A + Im B */
      a30 = (x[i + 5] + x[i + 7])>>1;  /* Im C + Im D */

      x[i + 0] = a00 + a10;       /* Re A' = Re A + Re B + Re C + Re D */
      x[i + 4] = a00 - a10;       /* Re C' = Re A + Re B - Re C - Re D */
      x[i + 1] = a20 + a30;       /* Im A' = Im A + Im B + Im C + Im D */
      x[i + 5] = a20 - a30;       /* Im C' = Im A + Im B - Im C - Im D */

      a00 = a00 - x[i + 2];       /* Re A - Re B */
      a10 = a10 - x[i + 6];       /* Re C - Re D */
      a20 = a20 - x[i + 3];       /* Im A - Im B */
      a30 = a30 - x[i + 7];       /* Im C - Im D */

      x[i + 2] = a00 + a30;       /* Re B' = Re A - Re B + Im C - Im D */
      x[i + 6] = a00 - a30;       /* Re D' = Re A - Re B - Im C + Im D */
      x[i + 3] = a20 - a10;       /* Im B' = Im A - Im B - Re C + Re D */
      x[i + 7] = a20 + a10;       /* Im D' = Im A - Im B + Re C - Re D */
    }

    INT mh = 1 << 1;
    INT ldm = ldn - 2;
    INT trigstep = trigDataSize;

    do
    {
        const FIXP_STP *pTrigData = trigdata;
        INT j;

        mh <<= 1;
        trigstep >>= 1;

        FDK_ASSERT(trigstep > 0);

        /* Do first iteration with c=1.0 and s=0.0 separately to avoid loosing to much precision.
           Beware: The impact on the overal FFT precision is rather large. */
        {
            FIXP_DBL *xt1 = x;
            int r = n;

            do {
                FIXP_DBL *xt2 = xt1 + (mh<<1);
                /*
                FIXP_DBL *xt1 = x+ ((r)<<1);
                FIXP_DBL *xt2 = xt1 + (mh<<1);
                */
                FIXP_DBL vr,vi,ur,ui;                 

                //cplxMultDiv2(&vi, &vr, x[t2+1], x[t2], (FIXP_SGL)1.0, (FIXP_SGL)0.0);
                vi = xt2[1]>>1;
                vr = xt2[0]>>1;

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui+vi;

                xt2[0] = ur-vr;
                xt2[1] = ui-vi;

                xt1 += mh;
                xt2 += mh;

                //cplxMultDiv2(&vr, &vi, x[t2+1], x[t2], (FIXP_SGL)1.0, (FIXP_SGL)0.0);
                vr = xt2[1]>>1;
                vi = xt2[0]>>1;

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui-vi;

                xt2[0] = ur-vr;
                xt2[1] = ui+vi;

                xt1 = xt2 + mh;
            } while ((r=r-(mh<<1)) != 0);
        }
        for(j=4; j<mh; j+=4)
        {
            FIXP_DBL *xt1 = x + (j>>1);
            FIXP_SPK cs;
            int r = n;

            pTrigData += trigstep;
            cs = *pTrigData;

            do
            {
                FIXP_DBL *xt2 = xt1 + (mh<<1);
                FIXP_DBL vr,vi,ur,ui;

                cplxMultDiv2(&vi, &vr, xt2[1], xt2[0], cs);

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui+vi;

                xt2[0] = ur-vr;
                xt2[1] = ui-vi;

                xt1 += mh;
                xt2 += mh;

                cplxMultDiv2(&vr, &vi, xt2[1], xt2[0], cs);

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui-vi;

                xt2[0] = ur-vr;
                xt2[1] = ui+vi;

                /* Same as above but for t1,t2 with j>mh/4 and thus cs swapped */
                xt1 = xt1 - (j);
                xt2 = xt1 + (mh<<1);

                cplxMultDiv2(&vi, &vr, xt2[0], xt2[1], cs);

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui-vi;

                xt2[0] = ur-vr;
                xt2[1] = ui+vi;

                xt1 += mh;
                xt2 += mh;

                cplxMultDiv2(&vr, &vi, xt2[0], xt2[1], cs);

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur-vr;
                xt1[1] = ui-vi;

                xt2[0] = ur+vr;
                xt2[1] = ui+vi;

                xt1 = xt2 + (j);
            }  while ((r=r-(mh<<1)) != 0);
        }
        {
            FIXP_DBL *xt1 = x + (mh>>1);
            int r = n;

            do
            {
                FIXP_DBL *xt2 = xt1 + (mh<<1);
                FIXP_DBL vr,vi,ur,ui;

                cplxMultDiv2(&vi, &vr, xt2[1], xt2[0], STC(0x5a82799a), STC(0x5a82799a));

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui+vi;

                xt2[0] = ur-vr;
                xt2[1] = ui-vi;

                xt1 += mh;
                xt2 += mh;

                cplxMultDiv2(&vr, &vi, xt2[1], xt2[0], STC(0x5a82799a), STC(0x5a82799a));

                ur = xt1[0]>>1;
                ui = xt1[1]>>1;

                xt1[0] = ur+vr;
                xt1[1] = ui-vi;

                xt2[0] = ur-vr;
                xt2[1] = ui+vi;

                xt1 = xt2 + mh;
            }  while ((r=r-(mh<<1)) != 0);
        }
    } while (--ldm != 0);
}

#endif /* if defined(FUNCTION_dit_fft)  */

#endif /* if defined(SINETABLE_16BIT) */

#endif /* ifndef FUNCTION_dit_fft */
