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

   Initial author:       M. Lohwasser
   contents/description: PNS parameters depending on bitrate and bandwidth

******************************************************************************/

#ifndef __PNSPARAM_H
#define __PNSPARAM_H

#include "aacenc.h"
#include "common_fix.h"
#include "psy_const.h"

#define NUM_PNSINFOTAB    4
#define PNS_TABLE_ERROR   -1

/* detection algorithm flags */
#define USE_POWER_DISTRIBUTION           (1<<0)
#define USE_PSYCH_TONALITY               (1<<1)
#define USE_TNS_GAIN_THR                 (1<<2)
#define USE_TNS_PNS                      (1<<3)
#define JUST_LONG_WINDOW                 (1<<4)
/* additional algorithm flags */
#define IS_LOW_COMLEXITY                 (1<<5)

typedef struct
{
  /* PNS start band */
  short startSfb;

  /* detection algorithm flags */
  USHORT detectionAlgorithmFlags;

  /* Parameters for detection */
  FIXP_DBL  refPower;
  FIXP_DBL  refTonality;
  INT       tnsGainThreshold;
  INT       tnsPNSGainThreshold;
  INT       minSfbWidth;
  FIXP_SGL  powDistPSDcurve[MAX_GROUPED_SFB];
  FIXP_SGL  gapFillThr;
} NOISEPARAMS;

int FDKaacEnc_lookUpPnsUse (int bitRate, int sampleRate, int numChan, const int isLC);

/****** Definition of prototypes ******/

AAC_ENCODER_ERROR FDKaacEnc_GetPnsParam(NOISEPARAMS *np,
                                        INT         bitRate,
                                        INT         sampleRate,
                                        INT         sfbCnt,
                                        const INT   *sfbOffset,
                                        INT         *usePns,
                                        INT         numChan,
                                        const INT   isLC);

#endif
