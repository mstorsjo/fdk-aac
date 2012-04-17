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
   Initial author:       M.Werner
   contents/description: MS stereo processing

******************************************************************************/
#include "ms_stereo.h"

#include "psy_const.h"

/* static const float scaleMinThres = 1.0f; */ /* 0.75f for 3db boost */

void FDKaacEnc_MsStereoProcessing(PSY_DATA   *RESTRICT psyData[(2)],
                        PSY_OUT_CHANNEL* psyOutChannel[2],
                        const INT  *isBook,
                        INT        *msDigest,       /* output */
                        INT        *msMask,         /* output */
                        const INT   sfbCnt,
                        const INT   sfbPerGroup,
                        const INT   maxSfbPerGroup,
                        const INT  *sfbOffset)
{
    FIXP_DBL        *sfbEnergyLeft = psyData[0]->sfbEnergy.Long;                       /* modified where msMask==1 */
    FIXP_DBL        *sfbEnergyRight = psyData[1]->sfbEnergy.Long;                      /* modified where msMask==1 */
    const FIXP_DBL  *sfbEnergyMid = psyData[0]->sfbEnergyMS.Long;
    const FIXP_DBL  *sfbEnergySide = psyData[1]->sfbEnergyMS.Long;
    FIXP_DBL        *sfbThresholdLeft = psyData[0]->sfbThreshold.Long;                 /* modified where msMask==1 */
    FIXP_DBL        *sfbThresholdRight = psyData[1]->sfbThreshold.Long;                /* modified where msMask==1 */

    FIXP_DBL        *sfbSpreadEnLeft = psyData[0]->sfbSpreadEnergy.Long;
    FIXP_DBL        *sfbSpreadEnRight = psyData[1]->sfbSpreadEnergy.Long;

    FIXP_DBL        *sfbEnergyLeftLdData = psyOutChannel[0]->sfbEnergyLdData;        /* modified where msMask==1 */
    FIXP_DBL        *sfbEnergyRightLdData = psyOutChannel[1]->sfbEnergyLdData;       /* modified where msMask==1 */
    FIXP_DBL        *sfbEnergyMidLdData = psyData[0]->sfbEnergyMSLdData;
    FIXP_DBL        *sfbEnergySideLdData = psyData[1]->sfbEnergyMSLdData;
    FIXP_DBL        *sfbThresholdLeftLdData = psyOutChannel[0]->sfbThresholdLdData;  /* modified where msMask==1 */
    FIXP_DBL        *sfbThresholdRightLdData = psyOutChannel[1]->sfbThresholdLdData; /* modified where msMask==1 */

    FIXP_DBL        *mdctSpectrumLeft = psyData[0]->mdctSpectrum;                      /* modified where msMask==1 */
    FIXP_DBL        *mdctSpectrumRight = psyData[1]->mdctSpectrum;                     /* modified where msMask==1 */

    INT             sfb,sfboffs, j;           /* loop counters         */
    FIXP_DBL        pnlrLdData, pnmsLdData;
    FIXP_DBL        minThresholdLdData;
    FIXP_DBL        minThreshold;
    INT             useMS;

    INT             msMaskTrueSomewhere = 0;  /* to determine msDigest */
    INT             numMsMaskFalse = 0;       /* number of non-intensity bands where L/R coding is used */

    for(sfb=0; sfb<sfbCnt; sfb+=sfbPerGroup) {
      for(sfboffs=0;sfboffs<maxSfbPerGroup;sfboffs++) {

        if ( (isBook==NULL) ? 1 : (isBook[sfb+sfboffs] == 0) ) {
          FIXP_DBL tmp;

/*
          minThreshold=min(sfbThresholdLeft[sfb+sfboffs], sfbThresholdRight[sfb+sfboffs])*scaleMinThres;
          pnlr = (sfbThresholdLeft[sfb+sfboffs]/
                 max(sfbEnergyLeft[sfb+sfboffs],sfbThresholdLeft[sfb+sfboffs]))*
                 (sfbThresholdRight[sfb+sfboffs]/
                 max(sfbEnergyRight[sfb+sfboffs],sfbThresholdRight[sfb+sfboffs]));
          pnms = (minThreshold/max(sfbEnergyMid[sfb+sfboffs],minThreshold))*
                 (minThreshold/max(sfbEnergySide[sfb+sfboffs],minThreshold));
          useMS = (pnms > pnlr);
*/

          /* we assume that scaleMinThres == 1.0f and we can drop it */
          minThresholdLdData = fixMin(sfbThresholdLeftLdData[sfb+sfboffs], sfbThresholdRightLdData[sfb+sfboffs]);

          /* pnlrLdData = sfbThresholdLeftLdData[sfb+sfboffs] -
                        max(sfbEnergyLeftLdData[sfb+sfboffs], sfbThresholdLeftLdData[sfb+sfboffs]) +
                        sfbThresholdRightLdData[sfb+sfboffs] -
                        max(sfbEnergyRightLdData[sfb+sfboffs], sfbThresholdRightLdData[sfb+sfboffs]); */
          tmp = fixMax(sfbEnergyLeftLdData[sfb+sfboffs], sfbThresholdLeftLdData[sfb+sfboffs]);
          pnlrLdData = (sfbThresholdLeftLdData[sfb+sfboffs]>>1) - (tmp>>1);
          pnlrLdData = pnlrLdData + (sfbThresholdRightLdData[sfb+sfboffs]>>1);
          tmp = fixMax(sfbEnergyRightLdData[sfb+sfboffs], sfbThresholdRightLdData[sfb+sfboffs]);
          pnlrLdData = pnlrLdData - (tmp>>1);

          /* pnmsLdData = minThresholdLdData - max(sfbEnergyMidLdData[sfb+sfboffs], minThresholdLdData) +
                        minThresholdLdData - max(sfbEnergySideLdData[sfb+sfboffs], minThresholdLdData); */
          tmp = fixMax(sfbEnergyMidLdData[sfb+sfboffs], minThresholdLdData);
          pnmsLdData = minThresholdLdData - (tmp>>1);
          tmp = fixMax(sfbEnergySideLdData[sfb+sfboffs], minThresholdLdData);
          pnmsLdData = pnmsLdData - (tmp>>1);
          useMS = (pnmsLdData > (pnlrLdData));


          if (useMS) {
            msMask[sfb+sfboffs] = 1;
            msMaskTrueSomewhere = 1;
            for(j=sfbOffset[sfb+sfboffs]; j<sfbOffset[sfb+sfboffs+1]; j++) {
              FIXP_DBL specL, specR;
              specL = mdctSpectrumLeft[j]>>1;
              specR = mdctSpectrumRight[j]>>1;
              mdctSpectrumLeft[j] = specL + specR;
              mdctSpectrumRight[j] = specL - specR;
            }
            minThreshold = fixMin(sfbThresholdLeft[sfb+sfboffs], sfbThresholdRight[sfb+sfboffs]);
            sfbThresholdLeft[sfb+sfboffs] = sfbThresholdRight[sfb+sfboffs] = minThreshold;
            sfbThresholdLeftLdData[sfb+sfboffs] = sfbThresholdRightLdData[sfb+sfboffs] = minThresholdLdData;
            sfbEnergyLeft[sfb+sfboffs] = sfbEnergyMid[sfb+sfboffs];
            sfbEnergyRight[sfb+sfboffs] = sfbEnergySide[sfb+sfboffs];
            sfbEnergyLeftLdData[sfb+sfboffs] = sfbEnergyMidLdData[sfb+sfboffs];
            sfbEnergyRightLdData[sfb+sfboffs] = sfbEnergySideLdData[sfb+sfboffs];

            sfbSpreadEnLeft[sfb+sfboffs] = sfbSpreadEnRight[sfb+sfboffs] =
                     fixMin( sfbSpreadEnLeft[sfb+sfboffs],
                             sfbSpreadEnRight[sfb+sfboffs] ) >> 1;

          }
          else {
            msMask[sfb+sfboffs] = 0;
            numMsMaskFalse++;
          } /* useMS */
        } /* isBook */
        else {
          /* keep mDigest from IS module */
          if (msMask[sfb+sfboffs]) {
            msMaskTrueSomewhere = 1;
          }
          /* prohibit MS_MASK_ALL in combination with IS */
          numMsMaskFalse = 9;
        } /* isBook */
      } /* sfboffs */
    } /* sfb */


    if(msMaskTrueSomewhere == 1) {
      if ((numMsMaskFalse == 0) || ((numMsMaskFalse < maxSfbPerGroup) && (numMsMaskFalse < 9))) {
        *msDigest = SI_MS_MASK_ALL;
        /* loop through M/S bands; if msMask==0, set it to 1 and apply M/S */
        for (sfb = 0; sfb < sfbCnt; sfb += sfbPerGroup) {
          for (sfboffs = 0; sfboffs < maxSfbPerGroup; sfboffs++) {
            if (( (isBook == NULL) ? 1 : (isBook[sfb+sfboffs] == 0) ) && (msMask[sfb+sfboffs] == 0)) {
              msMask[sfb+sfboffs] = 1;
              /* apply M/S coding */
              for(j=sfbOffset[sfb+sfboffs]; j<sfbOffset[sfb+sfboffs+1]; j++) {
                FIXP_DBL specL, specR;
                specL = mdctSpectrumLeft[j]>>1;
                specR = mdctSpectrumRight[j]>>1;
                mdctSpectrumLeft[j] = specL + specR;
                mdctSpectrumRight[j] = specL - specR;
              }
              minThreshold = fixMin(sfbThresholdLeft[sfb+sfboffs], sfbThresholdRight[sfb+sfboffs]);
              sfbThresholdLeft[sfb+sfboffs] = sfbThresholdRight[sfb+sfboffs] = minThreshold;
              minThresholdLdData = fixMin(sfbThresholdLeftLdData[sfb+sfboffs], sfbThresholdRightLdData[sfb+sfboffs]);
              sfbThresholdLeftLdData[sfb+sfboffs] = sfbThresholdRightLdData[sfb+sfboffs] = minThresholdLdData;
              sfbEnergyLeft[sfb+sfboffs] = sfbEnergyMid[sfb+sfboffs];
              sfbEnergyRight[sfb+sfboffs] = sfbEnergySide[sfb+sfboffs];
              sfbEnergyLeftLdData[sfb+sfboffs] = sfbEnergyMidLdData[sfb+sfboffs];
              sfbEnergyRightLdData[sfb+sfboffs] = sfbEnergySideLdData[sfb+sfboffs];

              sfbSpreadEnLeft[sfb+sfboffs] = sfbSpreadEnRight[sfb+sfboffs] =
                       fixMin( sfbSpreadEnLeft[sfb+sfboffs],
                               sfbSpreadEnRight[sfb+sfboffs] ) >> 1;
            }
          }
        }
      } else {
        *msDigest = SI_MS_MASK_SOME;
      }
    } else {
      *msDigest = SI_MS_MASK_NONE;
    }
}
