/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2007)
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
   Description: dit_fft MIPS assembler replacements.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if defined(MIPS_DSP_LIB)

#include "dsplib_util.h"
#include "dsplib_dsp.h"

#define FUNCTION_dit_fft

#ifdef FUNCTION_dit_fft

#include "mips_fft_twiddles.cpp"

void dit_fft(FIXP_DBL *x, const INT ldn, const FIXP_STP *trigdata, const INT trigDataSize)
{
    int i;

    int32c *din = (int32c*)x;
    int32c *dout = (int32c*)x;

    int32c scratch[1024];
    int32c *twiddles;

    switch (ldn) {
            case 4:  twiddles = (int32c*)__twiddles_mips_fft32_16;   break;
            case 5:  twiddles = (int32c*)__twiddles_mips_fft32_32;   break;
            case 6:  twiddles = (int32c*)__twiddles_mips_fft32_64;   break;
            case 7:  twiddles = (int32c*)__twiddles_mips_fft32_128;  break;
            case 8:  twiddles = (int32c*)__twiddles_mips_fft32_256;  break;
            case 9:  twiddles = (int32c*)__twiddles_mips_fft32_512;  break;
            case 10: twiddles = (int32c*)__twiddles_mips_fft32_1024; break;
            //case 11: twiddles = (int32c*)__twiddles_mips_fft32_2048; break;
            default: FDK_ASSERT(0); break;
    }

    mips_fft32(dout, din, twiddles, scratch, ldn);

    for(i=0;i<(1<<ldn);i++) {
        x[2*i]   = dout[i].re<<1;
        x[2*i+1] = dout[i].im<<1;
    }
}
#endif


#endif /* defined(MIPS_DSP_LIB) */
