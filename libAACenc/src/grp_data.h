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
#ifndef __GRP_DATA_H__
#define __GRP_DATA_H__

#include "common_fix.h"

#include "psy_data.h"


void
FDKaacEnc_groupShortData(FIXP_DBL      *mdctSpectrum,         /* in-out */
               SFB_THRESHOLD *sfbThreshold,         /* in-out */
               SFB_ENERGY    *sfbEnergy,            /* in-out */
               SFB_ENERGY    *sfbEnergyMS,          /* in-out */
               SFB_ENERGY    *sfbSpreadEnergy,
               const INT      sfbCnt,
               const INT      sfbActive,
               const INT     *sfbOffset,
               const FIXP_DBL *sfbMinSnrLdData,
               INT           *groupedSfbOffset,     /* out */
               INT           *maxSfbPerGroup,
               FIXP_DBL      *groupedSfbMinSnrLdData,
               const INT      noOfGroups,
               const INT     *groupLen,
               const INT      granuleLength);

#endif /* _INTERFACE_H */
