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
   Initial author:       Alex Groeschel
   contents/description: Temporal noise shaping

******************************************************************************/
#ifndef _TNS_H
#define _TNS_H

#include "common_fix.h"

#include "psy_const.h"


#ifndef PI
#define PI            3.1415926535897931f
#endif

/**
 * TNS_ENABLE_MASK
 * This bitfield defines which TNS features are enabled
 *   The TNS mask is composed of 4 bits.
 *   tnsMask |= 0x1; activate TNS short blocks
 *   tnsMask |= 0x2; activate TNS for long blocks
 *   tnsMask |= 0x4; activate TNS PEAK tool for short blocks
 *   tnsMask |= 0x8; activate TNS PEAK tool for long blocks
 */
#define TNS_ENABLE_MASK 0xf

/* TNS max filter order for Low Complexity MPEG4 profile */
#define TNS_MAX_ORDER 12


#define MAX_NUM_OF_FILTERS 2


typedef struct{ /*stuff that is tabulated dependent on bitrate etc. */
  INT     filterEnabled[MAX_NUM_OF_FILTERS];
  INT     threshOn[MAX_NUM_OF_FILTERS];                /* min. prediction gain for using tns TABUL*/
  INT     tnsLimitOrder[MAX_NUM_OF_FILTERS];           /* Limit for TNS order TABUL*/
  INT     tnsFilterDirection[MAX_NUM_OF_FILTERS];      /* Filtering direction, 0=up, 1=down TABUL */
  INT     acfSplit[MAX_NUM_OF_FILTERS];
  INT     seperateFiltersAllowed;

}TNS_CONFIG_TABULATED;



typedef struct {   /*assigned at InitTime*/
  TNS_CONFIG_TABULATED confTab;
  INT tnsActive;
  INT maxOrder;                /* max. order of tns filter */
  INT coefRes;
  FIXP_DBL acfWindow[MAX_NUM_OF_FILTERS][TNS_MAX_ORDER+3+1];
  /* now some things that only probably can be done at Init time;
     could be they have to be split up for each individual (short) window or
     even filter.  */
  INT lpcStartBand[MAX_NUM_OF_FILTERS];
  INT lpcStartLine[MAX_NUM_OF_FILTERS];
  INT lpcStopBand;
  INT lpcStopLine;

}TNS_CONFIG;


typedef struct {
  INT   tnsActive;
  INT   predictionGain;
} TNS_SUBBLOCK_INFO;

typedef struct{   /*changed at runTime*/
  TNS_SUBBLOCK_INFO subBlockInfo[TRANS_FAC];
  FIXP_DBL ratioMultTable[TRANS_FAC][MAX_SFB_SHORT];
} TNS_DATA_SHORT;

typedef struct{   /*changed at runTime*/
  TNS_SUBBLOCK_INFO subBlockInfo;
  FIXP_DBL ratioMultTable[MAX_SFB_LONG];
} TNS_DATA_LONG;

/* can be implemented as union */
typedef shouldBeUnion{
  TNS_DATA_LONG Long;
  TNS_DATA_SHORT Short;
}TNS_DATA_RAW;

typedef struct{
  INT numOfSubblocks;
  TNS_DATA_RAW dataRaw;
  INT tnsMaxScaleSpec;
  INT filtersMerged;
}TNS_DATA;

typedef struct{
  INT numOfFilters[TRANS_FAC];
  INT coefRes[TRANS_FAC];
  INT length[TRANS_FAC][MAX_NUM_OF_FILTERS];
  INT order[TRANS_FAC][MAX_NUM_OF_FILTERS];
  INT direction[TRANS_FAC][MAX_NUM_OF_FILTERS];
  INT coefCompress[TRANS_FAC][MAX_NUM_OF_FILTERS];
    /* for Long: length TNS_MAX_ORDER (12 for LC) is required -> 12 */
    /* for Short: length TRANS_FAC*TNS_MAX_ORDER (only 5 for short LC) is required -> 8*5=40 */
    /* Currently TRANS_FAC*TNS_MAX_ORDER = 8*12 = 96 (for LC) is used (per channel)! Memory could be saved here! */
  INT coef[TRANS_FAC][MAX_NUM_OF_FILTERS][TNS_MAX_ORDER];
}TNS_INFO;

INT FDKaacEnc_FreqToBandWithRounding(
        const INT freq,
        const INT fs,
        const INT numOfBands,
        const INT *bandStartOffset
        );

#endif /* _TNS_H */
