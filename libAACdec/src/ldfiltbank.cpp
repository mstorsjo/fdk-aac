/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (2006)
                                All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.

*
 *   filename: ldfiltbank.c
 *   project : MPEG-4 Audio Decoder
 *   contents/description: low delay filterbank
 *
 *   This software and/or program is protected by copyright law and
 *   international treaties. Any reproduction or distribution of this
 *   software and/or program, or any portion of it, may result in severe
 *   civil and criminal penalties, and will be prosecuted to the maximum
 *   extent possible under law.
 *
 * $Id$
 *
\***************************************************************************/

#include "ldfiltbank.h"


#include "aac_rom.h"
#include "dct.h"
#include "FDK_tools_rom.h"
#include "mdct.h"

#define LDFB_HEADROOM 2

static void multE2_DinvF_fdk(INT_PCM *output, FIXP_DBL* x, const FIXP_WTB* fb, FIXP_DBL* z, const int N, const int  stride)
{
  int i, scale;

  /*  scale for FIXP_DBL -> INT_PCM conversion. */
  scale = (DFRACT_BITS - SAMPLE_BITS) - LDFB_HEADROOM;

  for(i=0;i<N/4;i++)
  {
    FIXP_DBL z0, z2, tmp;

    z2 = x[N/2+i];
    z0 = z2 + ( fMultDiv2(z[N/2+i], fb[2*N + i]) >> (-WTS2-1) );

    z[N/2+i] = x[N/2-1-i] + ( fMultDiv2(z[N + i], fb[2*N + N/2 + i]) >> (-WTS2-1) );

    tmp = ( fMultDiv2(z[N/2+i], fb[N+N/2-1-i]) + fMultDiv2(z[i], fb[N+N/2+i]) ) ;

#if (SAMPLE_BITS <= 16)
    FDK_ASSERT( (-WTS1-1 + scale) >= 0);
    output[(N*3/4-1-i)*stride] = (INT_PCM)SATURATE_RIGHT_SHIFT(tmp, -WTS1-1 + scale, SAMPLE_BITS);
#else
    FDK_ASSERT( (WTS1+1 - scale) >= 0);
    output[(N*3/4-1-i)*stride] = (INT_PCM)SATURATE_LEFT_SHIFT(tmp, WTS1+1 - scale, SAMPLE_BITS);
#endif

    z[i] = z0;
    z[N + i] = z2;
  }

  for(i=N/4;i<N/2;i++)
  {
    FIXP_DBL z0, z2, tmp0, tmp1;

    z2 = x[N/2+i];
    z0 = z2 + ( fMultDiv2(z[N/2+i], fb[2*N + i]) >> (-WTS2-1) );

    z[N/2+i] = x[N/2-1-i] + ( fMultDiv2(z[N + i], fb[2*N + N/2 + i]) >> (-WTS2-1) );

    tmp0 = ( fMultDiv2(z[N/2+i], fb[N/2-1-i])   + fMultDiv2(z[i], fb[N/2+i])   ) ;
    tmp1 = ( fMultDiv2(z[N/2+i], fb[N+N/2-1-i]) + fMultDiv2(z[i], fb[N+N/2+i]) ) ;

#if (SAMPLE_BITS <= 16)
    FDK_ASSERT( (-WTS0-1 + scale) >= 0);
    output[(i-N/4)*stride]     = (INT_PCM)SATURATE_RIGHT_SHIFT(tmp0, -WTS0-1 + scale, SAMPLE_BITS);
    output[(N*3/4-1-i)*stride] = (INT_PCM)SATURATE_RIGHT_SHIFT(tmp1, -WTS1-1 + scale, SAMPLE_BITS);
#else
    FDK_ASSERT( (WTS0+1 - scale) >= 0);
    output[(i-N/4)*stride]     = (INT_PCM)SATURATE_LEFT_SHIFT(tmp0, WTS0+1 - scale, SAMPLE_BITS);
    output[(N*3/4-1-i)*stride] = (INT_PCM)SATURATE_LEFT_SHIFT(tmp1, WTS1+1 - scale, SAMPLE_BITS);
#endif
    z[i] = z0;
    z[N + i] = z2;
  }

  /* Exchange quarter parts of x to bring them in the "right" order */
  for(i=0;i<N/4;i++)
  {
    FIXP_DBL tmp0 = fMultDiv2(z[i], fb[N/2+i]);

#if (SAMPLE_BITS <= 16)
    FDK_ASSERT( (-WTS0-1 + scale) >= 0);
    output[(N*3/4 + i)*stride] = (INT_PCM)SATURATE_RIGHT_SHIFT(tmp0, -WTS0-1 + scale, SAMPLE_BITS);
#else
    FDK_ASSERT( (WTS0+1 - scale) >= 0);
    output[(N*3/4 + i)*stride] = (INT_PCM)SATURATE_LEFT_SHIFT(tmp0, WTS0+1 - scale, SAMPLE_BITS);
#endif
  }
}

int InvMdctTransformLowDelay_fdk (FIXP_DBL *mdctData, const int mdctData_e, INT_PCM *output, FIXP_DBL *fs_buffer, const int stride, const int N) {

  const FIXP_WTB *coef;
  FIXP_DBL gain = (FIXP_DBL)0;
  int scale = mdctData_e + MDCT_OUT_HEADROOM - LDFB_HEADROOM; /* The LDFB_HEADROOM is compensated inside multE2_DinvF_fdk() below */

  /* Select LD window slope */
  if (N == 512)
    coef = (FIXP_WTB*)LowDelaySynthesis512;
  else
    coef = (FIXP_WTB*)LowDelaySynthesis480;

  /*
     Apply exponent and 1/N factor.
     Note: "scale" is off by one because for LD_MDCT the window length is twice
     the window length of a regular MDCT. This is corrected inside multE2_DinvF_fdk().
     Refer to ISO/IEC 14496-3:2009 page 277, chapter 4.6.20.2 "Low Delay Window".
   */
  imdct_gain(&gain, &scale, N);

  dct_IV(mdctData, N, &scale);

  if (gain != (FIXP_DBL)0) {
    scaleValuesWithFactor(mdctData, gain, N, scale);
  } else {
    scaleValues(mdctData, N, scale);
  }

  /* Since all exponent and factors have been applied, current exponent is zero. */
  multE2_DinvF_fdk(output, mdctData, coef, fs_buffer, N, stride);

  return (1);
}




