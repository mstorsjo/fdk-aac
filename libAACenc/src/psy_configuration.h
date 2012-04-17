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
   contents/description: Psychoaccoustic configuration

******************************************************************************/
#ifndef _PSY_CONFIGURATION_H
#define _PSY_CONFIGURATION_H


#include "aacenc.h"
#include "common_fix.h"

#include "psy_const.h"
#include "aacenc_tns.h"
#include "aacenc_pns.h"

#define THR_SHIFTBITS         4
#define PCM_QUANT_THR_SCALE  16

#define C_RATIO (FIXP_DBL)0x02940a10 /* FL2FXCONST_DBL(0.001258925f) << THR_SHIFTBITS; */ /* pow(10.0f, -(29.0f/10.0f)) */

typedef struct{

  INT sfbCnt;                               /* number of existing sf bands */
  INT sfbActive;                            /* number of sf bands containing energy after lowpass */
  INT sfbActiveLFE;
  INT sfbOffset[MAX_SFB+1];

  INT       filterbank;                     /* LC, LD or ELD */

  FIXP_DBL  sfbPcmQuantThreshold[MAX_SFB];

  INT       maxAllowedIncreaseFactor;          /* preecho control */
  FIXP_SGL  minRemainingThresholdFactor;

  INT       lowpassLine;
  INT       lowpassLineLFE;
  FIXP_DBL  clipEnergy;                        /* for level dependend tmn */

  FIXP_DBL  sfbMaskLowFactor[MAX_SFB];
  FIXP_DBL  sfbMaskHighFactor[MAX_SFB];

  FIXP_DBL  sfbMaskLowFactorSprEn[MAX_SFB];
  FIXP_DBL  sfbMaskHighFactorSprEn[MAX_SFB];

  FIXP_DBL  sfbMinSnrLdData[MAX_SFB];                /* minimum snr (formerly known as bmax) */

  TNS_CONFIG tnsConf;
  PNS_CONFIG pnsConf;

  INT        granuleLength;
  INT        allowIS;

}PSY_CONFIGURATION;


typedef struct{
    UCHAR sfbCnt;                     /* Number of scalefactor bands */
    UCHAR sfbWidth[MAX_SFB_LONG];     /* Width of scalefactor bands for long blocks */
}SFB_PARAM_LONG;

typedef struct{
    UCHAR sfbCnt;                     /* Number of scalefactor bands */
    UCHAR sfbWidth[MAX_SFB_SHORT];    /* Width of scalefactor bands for short blocks */
}SFB_PARAM_SHORT;


AAC_ENCODER_ERROR FDKaacEnc_InitPsyConfiguration(INT  bitrate,
                                                 INT  samplerate,
                                                 INT  bandwidth,
                                                 INT  blocktype,
                                                 INT  granuleLength,
                                                 INT  useIS,
                                                 PSY_CONFIGURATION *psyConf,
                                                 FB_TYPE filterbank);

#endif /* _PSY_CONFIGURATION_H */



