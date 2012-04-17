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
   contents/description: pns.h

******************************************************************************/

#ifndef __PNS_H
#define __PNS_H

#include "common_fix.h"

#include "pnsparam.h"

#define NO_NOISE_PNS FDK_INT_MIN

typedef struct{
  NOISEPARAMS np;
  FIXP_DBL minCorrelationEnergy;
  FIXP_DBL noiseCorrelationThresh;
  INT      usePns;
} PNS_CONFIG;

typedef struct{
  FIXP_SGL noiseFuzzyMeasure[MAX_GROUPED_SFB];
  FIXP_DBL noiseEnergyCorrelation[MAX_GROUPED_SFB];
  INT      pnsFlag[MAX_GROUPED_SFB];
} PNS_DATA;

#endif
