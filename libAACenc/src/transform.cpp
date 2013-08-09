
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/*****************************************************************************

   Description: FDKaacLdEnc_MdctTransform480:
                The module FDKaacLdEnc_MdctTransform will perform the MDCT.
                The MDCT supports the sine window and
                the zero padded window. The algorithm of the MDCT
                can be divided in  Windowing, PreModulation, Fft and
                PostModulation.

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

