/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (1997)
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
   Initial author:       M. Werner
   contents/description: Chaos measure calculation

******************************************************************************/

#include "chaosmeasure.h"

/*****************************************************************************
    functionname: FDKaacEnc_FDKaacEnc_CalculateChaosMeasurePeakFast
    description:  Eberlein method of chaos measure calculation by high-pass
                  filtering amplitude spectrum
                  A special case of FDKaacEnc_CalculateChaosMeasureTonalGeneric --
                  highly optimized
*****************************************************************************/
static void
FDKaacEnc_FDKaacEnc_CalculateChaosMeasurePeakFast( FIXP_DBL  *RESTRICT paMDCTDataNM0,
                               INT        numberOfLines,
                               FIXP_DBL  *RESTRICT chaosMeasure )
{
  INT i, j;

  /* calculate chaos measure by "peak filter" */
  for (i=0; i<2; i++) {
    /* make even and odd pass through data */
    FIXP_DBL left,center; /* left, center tap of filter */

    left   = (FIXP_DBL)((LONG)paMDCTDataNM0[i]^((LONG)paMDCTDataNM0[i]>>(DFRACT_BITS-1)));
    center = (FIXP_DBL)((LONG)paMDCTDataNM0[i+2]^((LONG)paMDCTDataNM0[i+2]>>(DFRACT_BITS-1)));

    for (j = i+2; j < numberOfLines - 2; j+=2) {
      FIXP_DBL right = (FIXP_DBL)((LONG)paMDCTDataNM0[j+2]^((LONG)paMDCTDataNM0[j+2]>>(DFRACT_BITS-1)));
      FIXP_DBL tmp = (left>>1)+(right>>1);

      if (tmp < center ) {
         INT leadingBits = CntLeadingZeros(center)-1;
         tmp = schur_div(tmp<<leadingBits, center<<leadingBits, 8);
         chaosMeasure[j] = fMult(tmp,tmp);
      }
      else {
         chaosMeasure[j] = (FIXP_DBL)MAXVAL_DBL;
      }

      left   = center;
      center = right;
    }
  }

  /* provide chaos measure for first few lines */
  chaosMeasure[0] = chaosMeasure[2];
  chaosMeasure[1] = chaosMeasure[2];

  /* provide chaos measure for last few lines */
  for (i = (numberOfLines-3); i < numberOfLines; i++)
    chaosMeasure[i] = FL2FXCONST_DBL(0.5);
}


/*****************************************************************************
    functionname: FDKaacEnc_CalculateChaosMeasure
    description:  calculates a chaosmeasure for every line, different methods
                  are available. 0 means tonal, 1 means noiselike
    returns:
    input:        MDCT data, number of lines
    output:       chaosMeasure
*****************************************************************************/
void
FDKaacEnc_CalculateChaosMeasure( FIXP_DBL    *paMDCTDataNM0,
                       INT          numberOfLines,
                       FIXP_DBL    *chaosMeasure )

{
    FDKaacEnc_FDKaacEnc_CalculateChaosMeasurePeakFast( paMDCTDataNM0,
                                   numberOfLines,
                                   chaosMeasure );
}

