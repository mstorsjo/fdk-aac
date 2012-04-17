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
#ifndef __LINE_PE_H
#define __LINE_PE_H


#include "common_fix.h"

#include "psy_const.h"

#define PE_CONSTPART_SHIFT  FRACT_BITS

typedef struct {
   /* calculated by FDKaacEnc_prepareSfbPe */
   INT      sfbNLines[MAX_GROUPED_SFB];             /* number of relevant lines in sfb */
   /* the rest is calculated by FDKaacEnc_calcSfbPe */
   INT      sfbPe[MAX_GROUPED_SFB];                 /* pe for each sfb */
   INT      sfbConstPart[MAX_GROUPED_SFB];          /* constant part for each sfb */
   INT      sfbNActiveLines[MAX_GROUPED_SFB];       /* number of active lines in sfb */
   INT      pe;                                     /* sum of sfbPe */
   INT      constPart;                              /* sum of sfbConstPart */
   INT      nActiveLines;                           /* sum of sfbNActiveLines */
} PE_CHANNEL_DATA;

typedef struct {
    PE_CHANNEL_DATA peChannelData[(2)];
    INT    pe;
    INT    constPart;
    INT    nActiveLines;
    INT    offset;
} PE_DATA;


void FDKaacEnc_prepareSfbPe(PE_CHANNEL_DATA   *peChanData,
                  const FIXP_DBL    *sfbEnergyLdData,
                  const FIXP_DBL    *sfbThresholdLdData,
                  const FIXP_DBL    *sfbFormFactorLdData,
                  const INT         *sfbOffset,
                  const INT         sfbCnt,
                  const INT         sfbPerGroup,
                  const INT         maxSfbPerGroup);

void FDKaacEnc_calcSfbPe(PE_CHANNEL_DATA  *RESTRICT peChanData,
               const FIXP_DBL   *RESTRICT sfbEnergyLdData,
               const FIXP_DBL   *RESTRICT sfbThresholdLdData,
               const INT        sfbCnt,
               const INT        sfbPerGroup,
               const INT        maxSfbPerGroup,
               const INT       *isBook,
               const INT       *isScale);

#endif
