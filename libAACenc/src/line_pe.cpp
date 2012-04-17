/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (1999)
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
   contents/description: Perceptual entropie module

******************************************************************************/

#include "line_pe.h"
#include "sf_estim.h"
#include "bit_cnt.h"

#include "genericStds.h"

static const FIXP_DBL C1LdData = FL2FXCONST_DBL(3.0/LD_DATA_SCALING);        /* C1 = 3.0 = log(8.0)/log(2) */
static const FIXP_DBL C2LdData = FL2FXCONST_DBL(1.3219281/LD_DATA_SCALING);  /* C2 = 1.3219281 = log(2.5)/log(2) */
static const FIXP_DBL C3LdData = FL2FXCONST_DBL(0.5593573);                  /* 1-C2/C1 */


/* constants that do not change during successive pe calculations */
void FDKaacEnc_prepareSfbPe(PE_CHANNEL_DATA *peChanData,
                  const FIXP_DBL  *sfbEnergyLdData,
                  const FIXP_DBL  *sfbThresholdLdData,
                  const FIXP_DBL  *sfbFormFactorLdData,
                  const INT       *sfbOffset,
                  const INT       sfbCnt,
                  const INT       sfbPerGroup,
                  const INT       maxSfbPerGroup)
{
   INT sfbGrp,sfb;
   INT sfbWidth;
   FIXP_DBL avgFormFactorLdData;
   const FIXP_DBL formFacScaling = FL2FXCONST_DBL((float)FORM_FAC_SHIFT/LD_DATA_SCALING);

   for (sfbGrp = 0;sfbGrp < sfbCnt;sfbGrp+=sfbPerGroup) {
    for (sfb=0; sfb<maxSfbPerGroup; sfb++) {
      if ((FIXP_DBL)sfbEnergyLdData[sfbGrp+sfb] > (FIXP_DBL)sfbThresholdLdData[sfbGrp+sfb]) {
         sfbWidth = sfbOffset[sfbGrp+sfb+1] - sfbOffset[sfbGrp+sfb];
         /* estimate number of active lines */
         avgFormFactorLdData = ((-sfbEnergyLdData[sfbGrp+sfb]>>1) + (CalcLdInt(sfbWidth)>>1))>>1;
         peChanData->sfbNLines[sfbGrp+sfb] =
           (INT)CalcInvLdData( (sfbFormFactorLdData[sfbGrp+sfb] + formFacScaling) + avgFormFactorLdData);
      }
      else {
         peChanData->sfbNLines[sfbGrp+sfb] = 0;
      }
    }
   }
}

/*
   formula for one sfb:
   pe = n * ld(en/thr),                if ld(en/thr) >= C1
   pe = n * (C2 + C3 * ld(en/thr)),    if ld(en/thr) <  C1
   n: estimated number of lines in sfb,
   ld(x) = log(x)/log(2)

   constPart is sfbPe without the threshold part n*ld(thr) or n*C3*ld(thr)
*/
void FDKaacEnc_calcSfbPe(PE_CHANNEL_DATA  *RESTRICT peChanData,
               const FIXP_DBL   *RESTRICT sfbEnergyLdData,
               const FIXP_DBL   *RESTRICT sfbThresholdLdData,
               const INT        sfbCnt,
               const INT        sfbPerGroup,
               const INT        maxSfbPerGroup,
               const INT       *isBook,
               const INT       *isScale)
{
   INT sfbGrp,sfb;
   INT nLines;
   FIXP_DBL logDataRatio;
   INT lastValIs = 0;

   peChanData->pe = 0;
   peChanData->constPart = 0;
   peChanData->nActiveLines = 0;

   for(sfbGrp = 0;sfbGrp < sfbCnt;sfbGrp+=sfbPerGroup){
    for (sfb=0; sfb<maxSfbPerGroup; sfb++) {
      if ((FIXP_DBL)sfbEnergyLdData[sfbGrp+sfb] > (FIXP_DBL)sfbThresholdLdData[sfbGrp+sfb]) {
         logDataRatio = (FIXP_DBL)(sfbEnergyLdData[sfbGrp+sfb] - sfbThresholdLdData[sfbGrp+sfb]);
         nLines = peChanData->sfbNLines[sfbGrp+sfb];
         if (logDataRatio >= C1LdData) {
            /* scale sfbPe and sfbConstPart with PE_CONSTPART_SHIFT */
            peChanData->sfbPe[sfbGrp+sfb] = fMultDiv2(logDataRatio, (FIXP_DBL)(nLines<<(LD_DATA_SHIFT+PE_CONSTPART_SHIFT+1)));
            peChanData->sfbConstPart[sfbGrp+sfb] =
                    fMultDiv2(sfbEnergyLdData[sfbGrp+sfb], (FIXP_DBL)(nLines<<(LD_DATA_SHIFT+PE_CONSTPART_SHIFT+1))); ;

         }
         else {
            /* scale sfbPe and sfbConstPart with PE_CONSTPART_SHIFT */
            peChanData->sfbPe[sfbGrp+sfb] =
                     fMultDiv2(((FIXP_DBL)C2LdData + fMult(C3LdData,logDataRatio)), (FIXP_DBL)(nLines<<(LD_DATA_SHIFT+PE_CONSTPART_SHIFT+1)));

            peChanData->sfbConstPart[sfbGrp+sfb] =
                     fMultDiv2(((FIXP_DBL)C2LdData + fMult(C3LdData,sfbEnergyLdData[sfbGrp+sfb])),
                           (FIXP_DBL)(nLines<<(LD_DATA_SHIFT+PE_CONSTPART_SHIFT+1))) ;

            nLines = fMultI(C3LdData, nLines);
         }
         peChanData->sfbNActiveLines[sfbGrp+sfb] = nLines;
      }
      else if( isBook[sfb] ) {
        /* provide for cost of scale factor for Intensity */
        INT delta = isScale[sfbGrp+sfb] - lastValIs;
        lastValIs = isScale[sfbGrp+sfb];
        peChanData->sfbPe[sfbGrp+sfb] = FDKaacEnc_bitCountScalefactorDelta(delta)<<PE_CONSTPART_SHIFT;
        peChanData->sfbConstPart[sfbGrp+sfb] = 0;
        peChanData->sfbNActiveLines[sfbGrp+sfb] = 0;
      }
      else {
         peChanData->sfbPe[sfbGrp+sfb] = 0;
         peChanData->sfbConstPart[sfbGrp+sfb] = 0;
         peChanData->sfbNActiveLines[sfbGrp+sfb] = 0;
      }
      /* sum up peChanData values */
      peChanData->pe += peChanData->sfbPe[sfbGrp+sfb];
      peChanData->constPart += peChanData->sfbConstPart[sfbGrp+sfb];
      peChanData->nActiveLines += peChanData->sfbNActiveLines[sfbGrp+sfb];
    }
   }
   /* correct scaled pe and constPart values */
   peChanData->pe>>=PE_CONSTPART_SHIFT;
   peChanData->constPart>>=PE_CONSTPART_SHIFT;
}
