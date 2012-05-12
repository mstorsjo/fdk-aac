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
   contents/description: Psychoaccoustic data

******************************************************************************/
#ifndef _PSY_DATA_H
#define _PSY_DATA_H


#include "block_switch.h"

/* Be careful with MAX_SFB_LONG as length of the .Long arrays.
 * sfbEnergy.Long and sfbEnergyMS.Long and sfbThreshold.Long are used as a temporary storage for the regrouped
 * short energies and thresholds between FDKaacEnc_groupShortData() and BuildInterface() in FDKaacEnc_psyMain().
 * The space required for this is MAX_GROUPED_SFB ( = MAX_NO_OF_GROUPS*MAX_SFB_SHORT ).
 * However, this is not important if unions are used (which is not possible with pfloat). */

typedef shouldBeUnion{
  FIXP_DBL  Long[MAX_GROUPED_SFB];
  FIXP_DBL  Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_THRESHOLD;

typedef shouldBeUnion{
  FIXP_DBL  Long[MAX_GROUPED_SFB];
  FIXP_DBL  Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_ENERGY;

typedef shouldBeUnion{
  FIXP_DBL  Long[MAX_GROUPED_SFB];
  FIXP_DBL  Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_LD_ENERGY;

typedef shouldBeUnion{
  INT  Long[MAX_GROUPED_SFB];
  INT  Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_MAX_SCALE;


typedef struct{
  INT_PCM*                  psyInputBuffer;
  FIXP_DBL                  overlapAddBuffer[1024];

  BLOCK_SWITCHING_CONTROL   blockSwitchingControl;              /* block switching */
  FIXP_DBL                  sfbThresholdnm1[MAX_SFB];           /* FDKaacEnc_PreEchoControl */
  INT                       mdctScalenm1;                       /* scale of last block's mdct (FDKaacEnc_PreEchoControl) */
  INT                       calcPreEcho;
  INT                       isLFE;
}PSY_STATIC;


typedef struct{
  FIXP_DBL                 *mdctSpectrum;
  SFB_THRESHOLD             sfbThreshold;                       /* adapt                                  */
  SFB_ENERGY                sfbEnergy;                          /* sfb energies                           */
  SFB_LD_ENERGY             sfbEnergyLdData;                    /* sfb energies in ldData format          */
  SFB_MAX_SCALE             sfbMaxScaleSpec;
  SFB_ENERGY                sfbEnergyMS;                        /* mid/side sfb energies                  */
  FIXP_DBL                  sfbEnergyMSLdData[MAX_GROUPED_SFB]; /* mid/side sfb energies in ldData format */
  SFB_ENERGY                sfbSpreadEnergy;
  INT                       mdctScale;                          /* exponent of data in mdctSpectrum       */
  INT                       groupedSfbOffset[MAX_GROUPED_SFB+1];
  INT                       sfbActive;
  INT                       lowpassLine;
}PSY_DATA;


#endif /* _PSY_DATA_H */
