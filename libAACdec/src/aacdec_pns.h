/*****************************  MPEG-4 AAC Decoder  **************************

                        (C) Copyright Fraunhofer IIS (2004)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   $Id$
   Author(s):   Josef Hoepfl
   Description: perceptual noise substitution tool

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef PNS_H
#define PNS_H

#include "common_fix.h"


#define NO_OFBANDS ((8*16))

typedef struct {
  UCHAR correlated[NO_OFBANDS];
} CPnsInterChannelData;

typedef struct {
  CPnsInterChannelData *pPnsInterChannelData;
  UCHAR  pnsUsed[NO_OFBANDS];
  int    CurrentEnergy;
  UCHAR  PnsActive;
  INT   *currentSeed;
  INT   *randomSeed;
} CPnsData;

void CPns_InitPns ( CPnsData *pPnsData,
                    CPnsInterChannelData *pPnsInterChannelData,
                    INT* currentSeed,
                    INT* randomSeed );

void CPns_ResetData ( CPnsData *pPnsData, CPnsInterChannelData *pPnsInterChannelData );


#endif /* #ifndef PNS_H */
