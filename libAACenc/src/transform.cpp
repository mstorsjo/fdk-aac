/*****************************************************************************

                       (C) copyright Fraunhofer IIS (2001)
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
   Description: FDKaacLdEnc_MdctTransform480:
                The module FDKaacLdEnc_MdctTransform will perform the MDCT.
                The MDCT supports the sine window and

                the zero padded window. The algorithm of the MDCT
                can be divided in  Windowing, PreModulation, Fft and
                PostModulation.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "transform.h"

#include "dct.h"
#include "psy_const.h"
#include "aacEnc_rom.h"
#include "FDK_tools_rom.h"

INT FDKaacEnc_Transform_Real (const INT_PCM * pTimeData,
                              FIXP_DBL *RESTRICT mdctData,
                              const INT blockType,
                              const INT windowShape,
                              INT *prevWindowShape,
                              const INT frameLength,
                              INT *mdctData_e,
                              INT filterType
                             ,FIXP_DBL * RESTRICT overlapAddBuffer
                             )
{
  const INT_PCM * RESTRICT timeData;

  INT i;
  /* tl: transform length
     fl: left window slope length
     nl: left window slope offset
     fr: right window slope length
     nr: right window slope offset
     See FDK_tools/doc/intern/mdct.tex for more detail. */
  int tl, fl, nl, fr, nr;

  const FIXP_WTP * RESTRICT pLeftWindowPart;
  const FIXP_WTP * RESTRICT pRightWindowPart;

  /*
   * MDCT scale:
   * + 1: fMultDiv2() in windowing.
   * + 1: Because of factor 1/2 in Princen-Bradley compliant windowed TDAC.
   */
  *mdctData_e = 1+1;

  tl = frameLength;
  timeData = pTimeData;

  switch( blockType ) {
    case LONG_WINDOW:
      {
        int offset = (windowShape == LOL_WINDOW) ? ((frameLength * 3)>>2) : 0;
        fl = frameLength - offset;
        fr = frameLength - offset;
      }
      break;
    case STOP_WINDOW:
      fl = frameLength >> 3;
      fr = frameLength;
      break;
    case START_WINDOW: /* or StopStartSequence */
      fl = frameLength;
      fr = frameLength >> 3;
      break;
    case SHORT_WINDOW:
      fl = fr = frameLength >> 3;
      tl >>= 3;
      timeData = pTimeData + 3*fl + (fl/2);
      break;
    default:
      FDK_ASSERT(0);
      return -1;
      break;
  }

  /* Taken from FDK_tools/src/mdct.cpp Derive NR and NL */
  nr = (tl - fr)>>1;
  nl = (tl - fl)>>1;

  pLeftWindowPart = FDKgetWindowSlope(fl, *prevWindowShape);
  pRightWindowPart = FDKgetWindowSlope(fr, windowShape);

  /* windowing */
  if (filterType != FB_ELD)
  {
    /* Left window slope offset */
    for (i=0; i<nl ; i++)
    {
#if SAMPLE_BITS == DFRACT_BITS   /* SPC_BITS and DFRACT_BITS should be equal. */
      mdctData[(tl/2)+i] = - (FIXP_DBL) timeData[tl-i-1] >> ( 1 );
#else
      mdctData[(tl/2)+i] = - (FIXP_DBL) timeData[tl-i-1] << (DFRACT_BITS - SAMPLE_BITS - 1);
#endif
    }
    /* Left window slope */
    for (i=0; i<fl/2; i++)
    {
      FIXP_DBL tmp0;
      tmp0 = fMultDiv2((FIXP_PCM)timeData[i+nl], pLeftWindowPart[i].v.im);
      mdctData[(tl/2)+i+nl] = fMultSubDiv2(tmp0, (FIXP_PCM)timeData[tl-nl-i-1], pLeftWindowPart[i].v.re);
    }

    /* Right window slope offset */
    for(i=0; i<nr; i++)
    {
#if SAMPLE_BITS == DFRACT_BITS    /* This should be SPC_BITS instead of DFRACT_BITS. */
      mdctData[(tl/2)-1-i] = - (FIXP_DBL) timeData[tl+i] >> (1);
#else
      mdctData[(tl/2)-1-i] = - (FIXP_DBL) timeData[tl+i] << (DFRACT_BITS - SAMPLE_BITS - 1);
#endif
    }
    /* Right window slope */
    for (i=0; i<fr/2; i++)
    {
      FIXP_DBL tmp1;
      tmp1 = fMultDiv2((FIXP_PCM)timeData[tl+nr+i], pRightWindowPart[i].v.re);
      mdctData[(tl/2)-nr-i-1] = -fMultAddDiv2(tmp1, (FIXP_PCM)timeData[(tl*2)-nr-i-1], pRightWindowPart[i].v.im);
    }
  }

  if (filterType == FB_ELD)
  {
    const FIXP_WTB *pWindowELD=NULL;
    int i, N = frameLength, L = frameLength;

    if (frameLength == 512) {
      pWindowELD = ELDAnalysis512;
    } else {
      pWindowELD = ELDAnalysis480;
    }

    for(i=0;i<N/4;i++)
    {
      FIXP_DBL z0, outval;

      z0 = (fMult((FIXP_PCM)timeData[L+N*3/4-1-i], pWindowELD[N/2-1-i])<< (WTS0-1)) + (fMult((FIXP_PCM)timeData[L+N*3/4+i], pWindowELD[N/2+i])<< (WTS0-1));

      outval  = (fMultDiv2((FIXP_PCM)timeData[L+N*3/4-1-i], pWindowELD[N+N/2-1-i]) >> (-WTS1));
      outval += (fMultDiv2((FIXP_PCM)timeData[L+N*3/4+i], pWindowELD[N+N/2+i]) >> (-WTS1) );
      outval += (fMultDiv2(overlapAddBuffer[N/2+i], pWindowELD[2*N+i])>> (-WTS2-1));

      overlapAddBuffer[N/2+i] = overlapAddBuffer[i];

      overlapAddBuffer[i] = z0;
      mdctData[i] = overlapAddBuffer[N/2+i] + (fMultDiv2(overlapAddBuffer[N+N/2-1-i], pWindowELD[2*N+N/2+i]) >> (-WTS2-1));

      mdctData[N-1-i] = outval;
      overlapAddBuffer[N+N/2-1-i] = outval;
    }

    for(i=N/4;i<N/2;i++)
    {
      FIXP_DBL z0, outval;

      z0 = fMult((FIXP_PCM)timeData[L+N*3/4-1-i], pWindowELD[N/2-1-i]) << (WTS0-1);

      outval  = (fMultDiv2((FIXP_PCM)timeData[L+N*3/4-1-i], pWindowELD[N+N/2-1-i]) >> (-WTS1)) ;
      outval += (fMultDiv2(overlapAddBuffer[N/2+i], pWindowELD[2*N+i]) >> (-WTS2-1));

      overlapAddBuffer[N/2+i] = overlapAddBuffer[i] + (fMult((FIXP_PCM)timeData[L-N/4+i], pWindowELD[N/2+i])<< (WTS0-1) );

      overlapAddBuffer[i] = z0;
      mdctData[i] = overlapAddBuffer[N/2+i] + (fMultDiv2(overlapAddBuffer[N+N/2-1-i], pWindowELD[2*N+N/2+i]) >> (-WTS2-1));

      mdctData[N-1-i] = outval;
      overlapAddBuffer[N+N/2-1-i] = outval;
    }
  }

  dct_IV(mdctData, tl, mdctData_e);

  *prevWindowShape = windowShape;

  return 0;
}

