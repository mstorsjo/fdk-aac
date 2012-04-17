/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2005)
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

*******************************************************************************/


#ifndef __HYBRID_H
#define __HYBRID_H

#include "sbrdecoder.h"


#define HYBRID_FILTER_LENGTH  13
#define HYBRID_FILTER_DELAY    6


#define FAST_FILTER2
#define FAST_FILTER4
#define FAST_FILTER8
#define FAST_FILTER12

#define FFT_IDX_R(a)  (2*a)
#define FFT_IDX_I(a)  (2*a+1)

#define FIXP_FFT_IDX_R(a)   (a<<1)
#define FIXP_FFT_IDX_I(a)  ((a<<1) + 1)


typedef enum {

  HYBRID_2_REAL     = 2,
  HYBRID_4_CPLX     = 4,
  HYBRID_8_CPLX     = 8,
  HYBRID_12_CPLX    = 12

} HYBRID_RES;

typedef struct
{
  SCHAR   nQmfBands;
  SCHAR   frameSize;
  SCHAR   qmfBufferMove;

  UCHAR   pResolution[3];

  FIXP_DBL mQmfBufferRealSlot[3][HYBRID_FILTER_LENGTH];   /**< Stores old Qmf samples. */
  FIXP_DBL mQmfBufferImagSlot[3][HYBRID_FILTER_LENGTH];
  SCHAR sf_mQmfBuffer;

} HYBRID;

typedef HYBRID *HANDLE_HYBRID;

void
fillHybridDelayLine( FIXP_DBL **fixpQmfReal,
                     FIXP_DBL **fixpQmfImag,
                     FIXP_DBL   fixpHybridLeftR[12],
                     FIXP_DBL   fixpHybridLeftI[12],
                     FIXP_DBL   fixpHybridRightR[12],
                     FIXP_DBL   fixpHybridRightI[12],
                     HANDLE_HYBRID hHybrid );

void
slotBasedHybridAnalysis ( FIXP_DBL *fixpQmfReal,
                          FIXP_DBL *fixpQmfImag,

                          FIXP_DBL  *fixpHybridReal,
                          FIXP_DBL  *fixpHybridImag,

                          HANDLE_HYBRID hHybrid);


void
slotBasedHybridSynthesis ( FIXP_DBL  *fixpHybridReal,
                           FIXP_DBL  *fixpHybridImag,

                           FIXP_DBL  *fixpQmfReal,
                           FIXP_DBL  *fixpQmfImag,

                           HANDLE_HYBRID hHybrid );

SBR_ERROR InitHybridFilterBank ( HANDLE_HYBRID  hHybrid,
                                 SCHAR frameSize,
                                 SCHAR noBands,
                                 const UCHAR *pResolution );


#endif /* __HYBRID_H */
