/********************************  Fraunhofer IIS  ***************************

                        (C) Copyright Fraunhofer IIS (2010)
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
   Author(s):   Arthur Tritthart
   Description: (ARM optimised) Scaling of spectral data

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/


#define FUNCTION_CBlock_ScaleSpectralData_func1

/* Note: This loop is only separated for ARM in order to save cycles
         by loop unrolling. The ARM core provides by default a 5-cycle
         loop overhead per sample, that goes down to 1-cycle per sample
         with an optimal 4x-loop construct (do - 4x - while).
*/

FDK_INLINE static void CBlock_ScaleSpectralData_func1(
    FIXP_DBL *pSpectrum,
    int max_band,
    const SHORT * RESTRICT BandOffsets,
    int SpecScale_window,
    const SHORT * RESTRICT pSfbScale,
    int window)
{
  int band_offset = 0;
  for (int band=0; band < max_band; band++)
  {
    int runs = band_offset;
    band_offset = BandOffsets[band+1];
    runs = band_offset - runs;    /* is always a multiple of 4 */
    int scale = SpecScale_window-pSfbScale[window*16+band];
    if (scale)
    {
      do
      {
        FIXP_DBL tmp0, tmp1, tmp2, tmp3;
        tmp0 = pSpectrum[0];
        tmp1 = pSpectrum[1];
        tmp2 = pSpectrum[2];
        tmp3 = pSpectrum[3];
        tmp0 >>= scale;
        tmp1 >>= scale;
        tmp2 >>= scale;
        tmp3 >>= scale;
        *pSpectrum++ = tmp0;
        *pSpectrum++ = tmp1;
        *pSpectrum++ = tmp2;
        *pSpectrum++ = tmp3;
      } while ((runs = runs-4) != 0);
    }
    else
    {
      pSpectrum+= runs;
    }
  }
}
