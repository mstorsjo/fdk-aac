/******************************** MPEG Audio Encoder **************************

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


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

   $Id$

   Initial author:       M. Lohwasser
   contents/description: noisedet.c
                         Routines for Noise Detection

******************************************************************************/

#include "noisedet.h"

#include "aacenc_pns.h"
#include "pnsparam.h"


/*****************************************************************************

    functionname: FDKaacEnc_fuzzyIsSmaller
    description:  Fuzzy value calculation for "testVal is smaller than refVal"
    returns:      fuzzy value
    input:        test and ref Value,
                  low and high Lim
    output:       return fuzzy value

*****************************************************************************/
static FIXP_SGL FDKaacEnc_fuzzyIsSmaller( FIXP_DBL testVal,
                                FIXP_DBL refVal,
                                FIXP_DBL loLim,
                                FIXP_DBL hiLim )
{
  if (refVal <= FL2FXCONST_DBL(0.0))
        return( FL2FXCONST_SGL(0.0f) );
  else if (testVal >= fMult((hiLim>>1)+(loLim>>1), refVal))
        return( FL2FXCONST_SGL(0.0f) );
  else  return( (FIXP_SGL)MAXVAL_SGL );
}



/*****************************************************************************

    functionname: FDKaacEnc_noiseDetect
    description:  detect tonal sfb's; two tests
                  Powerdistribution:
                    sfb splittet in four regions,
                    compare the energy in all sections
                  PsychTonality:
                    compare tonality from chaosmeasure with reftonality
    returns:
    input:        spectrum of one large mdct
                  number of sfb's
                  pointer to offset of sfb's
                  pointer to noiseFuzzyMeasure (modified)
                  noiseparams struct
                  pointer to sfb energies
                  pointer to tonality calculated in chaosmeasure
    output:       noiseFuzzy Measure

*****************************************************************************/

void FDKaacEnc_noiseDetect(FIXP_DBL    *RESTRICT mdctSpectrum,
                 INT         *RESTRICT sfbMaxScaleSpec,
                 INT          sfbActive,
                 const INT   *RESTRICT sfbOffset,
                 FIXP_SGL    *RESTRICT noiseFuzzyMeasure,
                 NOISEPARAMS *np,
                 FIXP_SGL    *RESTRICT sfbtonality )

{
  int    i, k, sfb, sfbWidth;
  FIXP_SGL fuzzy, fuzzyTotal;
  FIXP_DBL refVal, testVal;

#define ALL_SFB_TONAL 0
#define ALL_SFB_NONTONAL 0

  /***** Start detection phase *****/
  /* Start noise detection for each band based on a number of checks */
  for (sfb=0; sfb<sfbActive; sfb++) {

    fuzzyTotal = (FIXP_SGL)MAXVAL_SGL;
    sfbWidth = sfbOffset[sfb+1] - sfbOffset[sfb];

    /* Reset output for lower bands or too small bands */
    if (sfb < np->startSfb  ||  sfbWidth < np->minSfbWidth) {
      noiseFuzzyMeasure[sfb] = FL2FXCONST_SGL(0.0f);
      continue;
    }

    if ( (np->detectionAlgorithmFlags & USE_POWER_DISTRIBUTION) && (fuzzyTotal > FL2FXCONST_SGL(0.5f)) ) {
      FIXP_DBL fhelp1, fhelp2, fhelp3, fhelp4, maxVal, minVal;
      INT leadingBits = fixMax(0,(sfbMaxScaleSpec[sfb] - 3));         /* max sfbWidth = 96/4 ; 2^5=32 => 5/2 = 3 (spc*spc) */

      /*  check power distribution in four regions */
      fhelp1 = fhelp2 = fhelp3 = fhelp4 = FL2FXCONST_DBL(0.0f);
      k = sfbWidth >>2;  /* Width of a quarter band */

      for (i=sfbOffset[sfb]; i<sfbOffset[sfb]+k; i++) {
        fhelp1 = fPow2AddDiv2(fhelp1, mdctSpectrum[i]<<leadingBits);
        fhelp2 = fPow2AddDiv2(fhelp2, mdctSpectrum[i+k]<<leadingBits);
        fhelp3 = fPow2AddDiv2(fhelp3, mdctSpectrum[i+2*k]<<leadingBits);
        fhelp4 = fPow2AddDiv2(fhelp4, mdctSpectrum[i+3*k]<<leadingBits);
      }

      /* get max into fhelp: */
      maxVal = fixMax(fhelp1, fhelp2);
      maxVal = fixMax(maxVal, fhelp3);
      maxVal = fixMax(maxVal, fhelp4);

      /* get min into fhelp1: */
      minVal = fixMin(fhelp1, fhelp2);
      minVal = fixMin(minVal, fhelp3);
      minVal = fixMin(minVal, fhelp4);

      /* Normalize min and max Val */
      leadingBits = CountLeadingBits(maxVal);
      testVal = maxVal << leadingBits;
      refVal  = minVal << leadingBits;

      /* calculate fuzzy value for power distribution */
      testVal = fMultDiv2(testVal, np->powDistPSDcurve[sfb]);

      fuzzy = FDKaacEnc_fuzzyIsSmaller(testVal,           /* 1/2 * maxValue * PSDcurve */
                             refVal,            /* 1   * minValue            */
                             FL2FXCONST_DBL(0.495),  /* 1/2 * loLim  (0.99f/2)    */
                             FL2FXCONST_DBL(0.505)); /* 1/2 * hiLim  (1.01f/2)    */

      fuzzyTotal = fixMin(fuzzyTotal, fuzzy);
    }

    if ( (np->detectionAlgorithmFlags & USE_PSYCH_TONALITY) && (fuzzyTotal > FL2FXCONST_SGL(0.5f)) ) {
      /* Detection with tonality-value of psych. acoustic (here: 1 is tonal!)*/

      testVal = FX_SGL2FX_DBL(sfbtonality[sfb])>>1;          /* 1/2 * sfbTonality         */
      refVal  = np->refTonality;

      fuzzy   = FDKaacEnc_fuzzyIsSmaller(testVal,
                               refVal,
                               FL2FXCONST_DBL(0.45f),    /* 1/2 * loLim  (0.9f/2)     */
                               FL2FXCONST_DBL(0.55f));   /* 1/2 * hiLim  (1.1f/2)     */

      fuzzyTotal = fixMin(fuzzyTotal, fuzzy);
    }


    /* Output of final result */
    noiseFuzzyMeasure[sfb] = fuzzyTotal;
  }
}
