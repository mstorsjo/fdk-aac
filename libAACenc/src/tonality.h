/******************************** MPEG Audio Encoder **************************

                (C) copyright Fraunhofer - IIS (1996)
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
   author:   M. Lohwasser
   contents/description: Calculate tonality index

******************************************************************************/

#ifndef __TONALITY_H
#define __TONALITY_H

#include "common_fix.h"


#include "chaosmeasure.h"


void FDKaacEnc_CalculateFullTonality( FIXP_DBL      *RESTRICT spectrum,
                            INT           *RESTRICT sfbMaxScaleSpec,
                            FIXP_DBL      *RESTRICT sfbEnergyLD64,
                            FIXP_SGL      *RESTRICT sfbTonality,
                            INT           sfbCnt,
                            const INT     *sfbOffset,
                            INT           usePns);

#endif
