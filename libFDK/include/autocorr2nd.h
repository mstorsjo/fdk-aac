/***************************  Fraunhofer IIS FDK Tools  ***********************

                        (C) Copyright Fraunhofer IIS (2006)
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
   Author(s):   M. Lohwasser
   Description: fixed point abs definitions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _AUTOCORR_2ND_H
#define _AUTOCORR_2ND_H



#include "common_fix.h"

typedef struct {
  FIXP_DBL  r00r;
  FIXP_DBL  r11r;
  FIXP_DBL  r22r;
  FIXP_DBL  r01r;
  FIXP_DBL  r02r;
  FIXP_DBL  r12r;
  FIXP_DBL  r01i;
  FIXP_DBL  r02i;
  FIXP_DBL  r12i;
  FIXP_DBL  det;
  int det_scale;
} ACORR_COEFS;

#define LPC_ORDER 2


INT
autoCorr2nd_real (ACORR_COEFS *ac,          /*!< Pointer to autocorrelation coeffs */
                  const FIXP_DBL *reBuffer, /*!< Pointer to to real part of spectrum */
                  const int len             /*!< Number of qmf slots */
                 );
INT
autoCorr2nd_cplx (ACORR_COEFS *ac,           /*!< Pointer to autocorrelation coeffs */
                  const FIXP_DBL *reBuffer,  /*!< Pointer to to real part of spectrum */
                  const FIXP_DBL *imBuffer,  /*!< Pointer to imag part of spectrum */
                  const int len              /*!< Number of qmf slots */
                 );


#endif /* _AUTOCORR_2ND_H */
