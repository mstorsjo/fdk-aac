/******************************** MPEG Audio Encoder **************************

                     (C) Copyright Fraunhofer IIS (2001)
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

   Initial author:       M. Lohwasser
   contents/description: noisedet.h

******************************************************************************/

#ifndef __NOISEDET_H
#define __NOISEDET_H

#include "common_fix.h"

#include "pnsparam.h"
#include "psy_data.h"


void FDKaacEnc_noiseDetect( FIXP_DBL    *mdctSpectrum,
                  INT         *sfbMaxScaleSpec,
                  INT          sfbActive,
                  const INT   *sfbOffset,
                  FIXP_SGL     noiseFuzzyMeasure[],
                  NOISEPARAMS *np,
                  FIXP_SGL    *sfbtonality );

#endif
