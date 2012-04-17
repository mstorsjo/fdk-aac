/******************************** MPEG Audio Encoder **************************

                       (c) copyright Fraunhofer IIS (1996)
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
   author:   M. Werner
   contents/description: Convert chaos measure to the tonality index

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "tonality.h"
#include "chaosmeasure.h"

/*static const FIXP_SGL maxtone  =  FL2FXCONST_SGL(0.5);*/
/*static const FIXP_SGL mintone  =  FL2FXCONST_SGL(0.05);*/
/*static const float convtone =  2.0f;*/           /* 1.0/mintone               */

/* -1.0/log(maxtone/mintone) */
static const FIXP_DBL normlog = (FIXP_DBL)0xd977d949; /*FL2FXCONST_DBL(-0.4342944819f * FDKlog(2.0)/FDKlog(2.7182818)); */

static void FDKaacEnc_CalcSfbTonality(FIXP_DBL      *RESTRICT spectrum,
							INT           *RESTRICT sfbMaxScaleSpec,
                            FIXP_DBL      *RESTRICT chaosMeasure,
                            FIXP_SGL      *RESTRICT sfbTonality,
                            INT           sfbCnt,
                            const INT     *RESTRICT sfbOffset,
                            FIXP_DBL      *RESTRICT sfbEnergyLD64 );


void FDKaacEnc_CalculateFullTonality(FIXP_DBL      *RESTRICT spectrum,
						             INT           *RESTRICT sfbMaxScaleSpec,
                                     FIXP_DBL      *RESTRICT sfbEnergyLD64,
                                     FIXP_SGL      *RESTRICT sfbTonality,
                                     INT            sfbCnt,
                                     const INT     *sfbOffset,
                                     INT            usePns)
{
  INT j;
#if defined(ARCH_PREFER_MULT_32x16)
  FIXP_SGL alpha_0 = FL2FXCONST_SGL(0.25f);       /* used in smooth ChaosMeasure */
  FIXP_SGL alpha_1 = FL2FXCONST_SGL(1.0f-0.25f);  /* used in smooth ChaosMeasure */
#else
  FIXP_DBL alpha_0 = FL2FXCONST_DBL(0.25f);       /* used in smooth ChaosMeasure */
  FIXP_DBL alpha_1 = FL2FXCONST_DBL(1.0f-0.25f);  /* used in smooth ChaosMeasure */
#endif
  INT numberOfLines = sfbOffset[sfbCnt];

  if (!usePns)
    return;

  C_ALLOC_SCRATCH_START(chaosMeasurePerLine, FIXP_DBL, (1024));
  /* calculate chaos measure */
  FDKaacEnc_CalculateChaosMeasure(spectrum,
                        numberOfLines,
                        chaosMeasurePerLine);

  /* smooth ChaosMeasure */
  for (j=1;j<numberOfLines;j++) {
    FIXP_DBL tmp = fMultDiv2(alpha_1, chaosMeasurePerLine[j]);
    chaosMeasurePerLine[j] = fMultAdd(tmp, alpha_0, chaosMeasurePerLine[j-1]);
  }

  FDKaacEnc_CalcSfbTonality(spectrum,
                  sfbMaxScaleSpec,
                  chaosMeasurePerLine,
                  sfbTonality,
                  sfbCnt,
                  sfbOffset,
                  sfbEnergyLD64);

  C_ALLOC_SCRATCH_END(chaosMeasurePerLine, FIXP_DBL, (1024));
}


/*****************************************************************************

    functionname: CalculateTonalityIndex
    description:  computes tonality values out of unpredictability values
                  limits range and computes log()
    returns:
    input:        ptr to energies, ptr to chaos measure values,
                  number of sfb
    output:       sfb wise tonality values

*****************************************************************************/
static void FDKaacEnc_CalcSfbTonality(FIXP_DBL      *RESTRICT spectrum,
                            INT           *RESTRICT sfbMaxScaleSpec,
                            FIXP_DBL      *RESTRICT chaosMeasure,
                            FIXP_SGL      *RESTRICT sfbTonality,
                            INT           sfbCnt,
                            const INT     *RESTRICT sfbOffset,
                            FIXP_DBL      *RESTRICT sfbEnergyLD64 )
{
    INT i, j;

    for (i=0; i<sfbCnt; i++) {
      FIXP_DBL chaosMeasureSfbLD64;
      INT shiftBits = fixMax(0,sfbMaxScaleSpec[i] - 4);  /* max sfbWidth = 96 ; 2^7=128 => 7/2 = 4 (spc*spc) */

      FIXP_DBL chaosMeasureSfb = FL2FXCONST_DBL(0.0);

      /* calc chaosMeasurePerSfb */
      for (j=(sfbOffset[i+1]-sfbOffset[i])-1; j>=0; j--) {
        FIXP_DBL tmp = (*spectrum++)<<shiftBits;
        FIXP_DBL lineNrg = fMultDiv2(tmp, tmp);
        chaosMeasureSfb = fMultAddDiv2(chaosMeasureSfb, lineNrg, *chaosMeasure++);
      }

      /* calc tonalityPerSfb */
      if (chaosMeasureSfb != FL2FXCONST_DBL(0.0))
      {
        /* add ld(convtone)/64 and 2/64 bec.fMultDiv2 */
        chaosMeasureSfbLD64 = CalcLdData((chaosMeasureSfb)) - sfbEnergyLD64[i];
        chaosMeasureSfbLD64 += FL2FXCONST_DBL(3.0f/64) - ((FIXP_DBL)(shiftBits)<<(DFRACT_BITS-6));

        if (chaosMeasureSfbLD64 > FL2FXCONST_DBL(-0.0519051) )     /* > ld(0.05)+ld(2) */
        {
          if (chaosMeasureSfbLD64 <= FL2FXCONST_DBL(0.0) )
		    sfbTonality[i] = FX_DBL2FX_SGL(fMultDiv2( chaosMeasureSfbLD64 , normlog ) << 7);
          else
            sfbTonality[i] = FL2FXCONST_SGL(0.0);
        }
        else
          sfbTonality[i] = (FIXP_SGL)MAXVAL_SGL;
      }
      else
        sfbTonality[i] = (FIXP_SGL)MAXVAL_SGL;
    }
}
