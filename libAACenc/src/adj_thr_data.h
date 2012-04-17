/*************************  Fast MPEG AAC Audio Encoder  **********************

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
   Initial author:       M. Schug / A. Groeschel
   contents/description: threshold calculations

******************************************************************************/

#ifndef __ADJ_THR_DATA_H
#define __ADJ_THR_DATA_H


#include "psy_const.h"

typedef struct {
   FIXP_DBL clipSaveLow, clipSaveHigh;
   FIXP_DBL minBitSave, maxBitSave;
   FIXP_DBL clipSpendLow, clipSpendHigh;
   FIXP_DBL minBitSpend, maxBitSpend;
} BRES_PARAM;

typedef struct {
   INT modifyMinSnr;
   INT startSfbL, startSfbS;
} AH_PARAM;

typedef struct {
  FIXP_DBL maxRed;
  FIXP_DBL startRatio;
  FIXP_DBL maxRatio;
  FIXP_DBL redRatioFac;
  FIXP_DBL redOffs;
} MINSNR_ADAPT_PARAM;

typedef struct {
  /* parameters for bitreservoir control */
  INT peMin, peMax;
  /* constant offset to pe               */
  INT    peOffset;
  /* constant PeFactor */
  FIXP_DBL bits2PeFactor_m;
  INT      bits2PeFactor_e;
  /* avoid hole parameters               */
  AH_PARAM ahParam;
  /* values for correction of pe */
  /* paramters for adaptation of minSnr */
  MINSNR_ADAPT_PARAM minSnrAdaptParam;
  INT peLast;
  INT dynBitsLast;
  FIXP_DBL peCorrectionFactor_m;
  INT      peCorrectionFactor_e;

  /* vbr encoding */
  FIXP_DBL vbrQualFactor;
  FIXP_DBL chaosMeasureOld;

  /* threshold weighting */
  FIXP_DBL chaosMeasureEnFac[(2)];
  INT      lastEnFacPatch[(2)];

} ATS_ELEMENT;

typedef struct {
  BRES_PARAM bresParamLong, bresParamShort;
  ATS_ELEMENT* adjThrStateElem[(6)];
} ADJ_THR_STATE;

#endif
