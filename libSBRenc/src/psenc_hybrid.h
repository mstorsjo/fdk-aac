/*****************************  MPEG Audio Encoder  ***************************

                     (C) Copyright Fraunhofer IIS (2004-2005)
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
   Initial author:       M. Neuendorf, M. Multrus
   contents/description: hypbrid filter bank (prototypes)

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef __hybrid_h
#define __hybrid_h

/* Includes ******************************************************************/
#include "sbr_def.h"
#include "ps_const.h"

#include "qmf.h"

/* Data Types ****************************************************************/

typedef enum {
  HYBRID_2_REAL     = 2,
  HYBRID_4_CPLX     = 4,
  HYBRID_6_CPLX     = 6,
  HYBRID_8_CPLX     = 8,
  HYBRID_12_CPLX    = 12
} HYBRID_RES;

#define MAX_HYBRID_RES (HYBRID_12_CPLX)

/* Defines *******************************************************************/
#define MAX_QMF_BANDS_IN_HYBRID         (5)
#define MAX_IID_GROUPS                  (50) /* NO_IID_GROUPS_HI_RES */

#define HYBRID_FILTER_LENGTH            ( 13 )

#define  HYBRID_FRAMESIZE        ( QMF_MAX_TIME_SLOTS )
#define  HYBRID_WRITEOFFSET      ( 10 )
#define  HYBRID_NUM_BANDS        ( 10 )

#define  NO_QMF_BANDS_HYBRID_10  ( 3 )
#define  NO_QMF_BANDS_HYBRID_20  ( 3 )
#define  HYBRID_MAX_QMF_BANDS    ( NO_QMF_BANDS_HYBRID_20 )

#define  QMF_BUFFER_MOVE         ( HYBRID_FILTER_LENGTH - 1 )


/* Data Types ****************************************************************/
typedef struct PS_HYBRID_CONFIG_tag {
  PS_BANDS      mode;

  UINT          noQmfBandsInHybrid;
  INT   aHybridResolution[MAX_QMF_BANDS_IN_HYBRID]; /* valid entries from 0 to noQmfBandsInHybrid */

} PS_HYBRID_CONFIG, *HANDLE_PS_HYBRID_CONFIG;

typedef struct PS_HYBRID_tag
{
  PS_BANDS   mode;
  INT        nQmfBands;
  INT        frameSizeInit;
  INT        frameSize;
  INT        pResolution[HYBRID_MAX_QMF_BANDS];
  INT        qmfBufferMove;
  INT        hybridFilterDelay;

  FIXP_DBL *fft;

  FIXP_QMF *pWorkReal;         /**< Working arrays for Qmf samples. */
  FIXP_QMF *pWorkImag;

  FIXP_QMF mQmfBufferReal[HYBRID_MAX_QMF_BANDS][QMF_BUFFER_MOVE];   /**< Stores old Qmf samples. */
  FIXP_QMF mQmfBufferImag[HYBRID_MAX_QMF_BANDS][QMF_BUFFER_MOVE];
  FIXP_QMF *mTempReal[HYBRID_FRAMESIZE];            /**< Temporary matrices for filter bank output. */
  FIXP_QMF *mTempImag[HYBRID_FRAMESIZE];

} PS_HYBRID;

typedef struct PS_HYBRID_DATA_tag {
  INT  frameSize;
  INT  nHybridBands;
  INT  nHybridQmfBands;
  INT  nHybridResolution [HYBRID_MAX_QMF_BANDS];

  FIXP_QMF* rHybData [(HYBRID_FRAMESIZE + HYBRID_WRITEOFFSET)];
  FIXP_QMF* iHybData [(HYBRID_FRAMESIZE + HYBRID_WRITEOFFSET)];
  SCHAR sf_fixpHybrid;

  INT hybDataReadOffset;
  INT hybDataWriteOffset;

} PS_HYBRID_DATA;


typedef struct PS_HYBRID_DATA_tag *HANDLE_PS_HYBRID_DATA;
typedef struct PS_HYBRID_tag *HANDLE_PS_HYBRID;



/* Function Declarations *********************************************/

/*****************************************************************************/
/* **** FILTERBANK CONFIG **** */

HANDLE_ERROR_INFO FDKsbrEnc_CreateHybridConfig(HANDLE_PS_HYBRID_CONFIG *phHybConfig,
                                     PS_BANDS mode);

/*****************************************************************************/
/* **** FILTERBANK DATA **** */

HANDLE_ERROR_INFO FDKsbrEnc_CreateHybridData(HANDLE_PS_HYBRID_DATA *phHybData,
                                   INT ch);

HANDLE_ERROR_INFO FDKsbrEnc_InitHybridData(HANDLE_PS_HYBRID_DATA hHybData,
                                   HANDLE_PS_HYBRID_CONFIG  hHybConfig,
                                   INT frameSize);

HANDLE_ERROR_INFO FDKsbrEnc_DestroyHybridData(HANDLE_PS_HYBRID_DATA* phHybData);

inline INT FDKsbrEnc_GetHybridFrameSize(HANDLE_PS_HYBRID_DATA h) {
  return h->frameSize;
}

inline INT FDKsbrEnc_GetNumberHybridBands(HANDLE_PS_HYBRID_DATA h) {
  return h->nHybridBands;
}

inline INT FDKsbrEnc_GetNumberHybridQmfBands(HANDLE_PS_HYBRID_DATA h) {
  return h->nHybridQmfBands;
}

INT FDKsbrEnc_GetHybridResolution(HANDLE_PS_HYBRID_DATA h, INT qmfBand);



/*****************************************************************************/
/* **** FILTERBANK **** */

HANDLE_ERROR_INFO
FDKsbrEnc_CreateHybridFilterBank ( HANDLE_PS_HYBRID        *phHybrid,
                                   INT                      ch );

HANDLE_ERROR_INFO
FDKsbrEnc_InitHybridFilterBank ( HANDLE_PS_HYBRID         hHybrid,
                                 HANDLE_PS_HYBRID_CONFIG  hHybConfig,
                                 INT                      frameSize );

HANDLE_ERROR_INFO
FDKsbrEnc_DeleteHybridFilterBank ( HANDLE_PS_HYBRID* phHybrid );

HANDLE_ERROR_INFO
HybridAnalysis ( HANDLE_PS_HYBRID        hHybrid,
                 FIXP_QMF *const * const mQmfReal,
                 FIXP_QMF *const * const mQmfImag,
                 SCHAR sf_fixpQmf,
                 FIXP_QMF **mHybridReal,
                 FIXP_QMF **mHybridImag,
                 SCHAR *sf_fixpHybrid);


INT
FDKsbrEnc_GetHybridFilterDelay(HANDLE_PS_HYBRID hHybrid);

#endif /*__hybrid_h*/
