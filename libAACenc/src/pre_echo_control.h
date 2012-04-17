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
   contents/description: Pre echo control

******************************************************************************/
#ifndef __PRE_ECHO_CONTROL_H
#define __PRE_ECHO_CONTROL_H

#include "common_fix.h"


void FDKaacEnc_InitPreEchoControl(FIXP_DBL    *pbThresholdnm1,
						INT         *calcPreEcho,
                        INT         numPb,
                        FIXP_DBL    *sfbPcmQuantThreshold,
                        INT         *mdctScalenm1);


void FDKaacEnc_PreEchoControl(FIXP_DBL    *pbThresholdNm1,
					INT         calcPreEcho,
                    INT         numPb,
                    INT         maxAllowedIncreaseFactor,
                    FIXP_SGL    minRemainingThresholdFactor,
                    FIXP_DBL    *pbThreshold,
                    INT         mdctScale,
                    INT         *mdctScalenm1);

#endif

