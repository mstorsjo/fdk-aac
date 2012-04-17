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
   contents/description: Short block grouping

******************************************************************************/
#include "psy_const.h"
#include "interface.h"

/*
* this routine does not work in-place
*/

void
FDKaacEnc_groupShortData(FIXP_DBL      *mdctSpectrum,     /* in-out                           */
               SFB_THRESHOLD *sfbThreshold,     /* in-out                           */
               SFB_ENERGY    *sfbEnergy,        /* in-out                           */
               SFB_ENERGY    *sfbEnergyMS,      /* in-out                           */
               SFB_ENERGY    *sfbSpreadEnergy,
               const INT      sfbCnt,
               const INT      sfbActive,
               const INT     *sfbOffset,
               const FIXP_DBL *sfbMinSnrLdData,
               INT           *groupedSfbOffset,       /* out */
               INT           *maxSfbPerGroup,         /* out */
               FIXP_DBL      *groupedSfbMinSnrLdData,
               const INT      noOfGroups,
               const INT     *groupLen,
               const INT      granuleLength)
{
  INT i,j;
  INT line;       /* counts through lines              */
  INT sfb;        /* counts through scalefactor bands  */
  INT grp;        /* counts through groups             */
  INT wnd;        /* counts through windows in a group */
  INT offset;     /* needed in sfbOffset grouping      */
  INT highestSfb;

  INT granuleLength_short = granuleLength/TRANS_FAC;

  /* for short blocks: regroup spectrum and */
  /* group energies and thresholds according to grouping */
  C_ALLOC_SCRATCH_START(tmpSpectrum, FIXP_DBL, (1024));

  /* calculate maxSfbPerGroup */
  highestSfb = 0;
  for (wnd = 0; wnd < TRANS_FAC; wnd++)
  {
    for (sfb = sfbActive-1; sfb >= highestSfb; sfb--)
    {
      for (line = sfbOffset[sfb+1]-1; line >= sfbOffset[sfb]; line--)
      {
        if ( mdctSpectrum[wnd*granuleLength_short+line] != FL2FXCONST_SPC(0.0) ) break; /* this band is not completely zero */
      }
      if (line >= sfbOffset[sfb]) break;                                      /* this band was not completely zero */
    }
    highestSfb = fixMax(highestSfb, sfb);
  }
  highestSfb = highestSfb > 0 ? highestSfb : 0;
  *maxSfbPerGroup = highestSfb+1;

  /* calculate groupedSfbOffset */
  i = 0;
  offset = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
      for (sfb = 0; sfb < sfbActive+1; sfb++)
      {
          groupedSfbOffset[i++] = offset + sfbOffset[sfb] * groupLen[grp];
      }
      i +=  sfbCnt-sfb;
      offset += groupLen[grp] * granuleLength_short;
  }
  groupedSfbOffset[i++] = granuleLength;

  /* calculate groupedSfbMinSnr */
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      groupedSfbMinSnrLdData[i++] = sfbMinSnrLdData[sfb];
    }
    i +=  sfbCnt-sfb;
  }

  /* sum up sfbThresholds */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      FIXP_DBL thresh = sfbThreshold->Short[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++)
      {
        thresh += sfbThreshold->Short[wnd+j][sfb];
      }
      sfbThreshold->Long[i++] = thresh;
    }
    i +=  sfbCnt-sfb;
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies left/right */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      FIXP_DBL energy = sfbEnergy->Short[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++)
      {
        energy += sfbEnergy->Short[wnd+j][sfb];
      }
      sfbEnergy->Long[i++] = energy;
    }
    i +=  sfbCnt-sfb;
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies mid/side */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      FIXP_DBL energy = sfbEnergyMS->Short[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++)
      {
        energy += sfbEnergyMS->Short[wnd+j][sfb];
      }
      sfbEnergyMS->Long[i++] = energy;
    }
    i +=  sfbCnt-sfb;
    wnd += groupLen[grp];
  }

  /* sum up sfbSpreadEnergies */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      FIXP_DBL energy = sfbSpreadEnergy->Short[wnd][sfb];
      for (j=1; j<groupLen[grp]; j++)
      {
         energy += sfbSpreadEnergy->Short[wnd+j][sfb];
      }
      sfbSpreadEnergy->Long[i++] = energy;
    }
    i +=  sfbCnt-sfb;
    wnd += groupLen[grp];
  }

  /* re-group spectrum */
  wnd = 0;
  i = 0;
  for (grp = 0; grp < noOfGroups; grp++)
  {
    for (sfb = 0; sfb < sfbActive; sfb++)
    {
      int width = sfbOffset[sfb+1]-sfbOffset[sfb];
      FIXP_DBL *pMdctSpectrum = &mdctSpectrum[sfbOffset[sfb]] + wnd*granuleLength_short;
      for (j = 0; j < groupLen[grp]; j++)
      {
        FIXP_DBL *pTmp = pMdctSpectrum;
        for (line = width; line > 0; line--)
        {
          tmpSpectrum[i++] = *pTmp++;
        }
        pMdctSpectrum += granuleLength_short;
      }
    }
    i +=  (groupLen[grp]*(sfbOffset[sfbCnt]-sfbOffset[sfb]));
    wnd += groupLen[grp];
  }

  FDKmemcpy(mdctSpectrum, tmpSpectrum, granuleLength*sizeof(FIXP_DBL));

  C_ALLOC_SCRATCH_END(tmpSpectrum, FIXP_DBL, (1024))
}
