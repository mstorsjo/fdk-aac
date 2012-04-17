/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2009)
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
   Author(s):
   Description: Library functions to calculate standard DCTs. This will most
                likely be replaced by hand-optimized functions for the specific
                target processor.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __dct_H
#define __dct_H



#include "common_fix.h"

/**
 * \brief Calculate DCT type II of given length. The DCT IV is
 *        calculated by a complex FFT, with some pre and post twiddeling.
 *        A factor of sqrt(2/(N-1)) is NOT applied.
 * \param pDat pointer to input/output data (in place processing).
 * \param size size of pDat.
 * \param pDat_e pointer to an integer containing the exponent of the data
 *               referenced by pDat. The exponent is updated accordingly.
 */
void dct_II(FIXP_DBL *pDat, FIXP_DBL *tmp, int size, int *pDat_e);

/**
 * \brief Calculate DCT type III of given length. The DCT IV is
 *        calculated by a complex FFT, with some pre and post twiddeling.
 *        Note that the factor 0.5 for the sum term x[0] is 1.0 instead of 0.5.
 *        A factor of sqrt(2/N) is NOT applied.
 * \param pDat pointer to input/output data (in place processing).
 * \param size size of pDat.
 * \param pDat_e pointer to an integer containing the exponent of the data
 *               referenced by pDat. The exponent is updated accordingly.
 */
void dct_III(FIXP_DBL *pDat, FIXP_DBL *tmp, int size, int *pDat_e);

/**
 * \brief Calculate DCT type IV of given length. The DCT IV is
 *        calculated by a complex FFT, with some pre and post twiddeling.
 *        A factor of sqrt(2/N) is NOT applied.
 * \param pDat pointer to input/output data (in place processing).
 * \param size size of pDat.
 * \param pDat_e pointer to an integer containing the exponent of the data
 *               referenced by pDat. The exponent is updated accordingly.
 */
void dct_IV(FIXP_DBL *pDat,int size, int *pDat_e);

/**
 * \brief Calculate DST type IV of given length. The DST IV is
 *        calculated by a complex FFT, with some pre and post twiddeling.
 *        A factor of sqrt(2/N) is NOT applied.
 * \param pDat pointer to input/output data (in place processing).
 * \param size size of pDat.
 * \param pDat_e pointer to an integer containing the exponent of the data
 *               referenced by pDat. The exponent is updated accordingly.
 */
void dst_IV(FIXP_DBL *pDat,int size, int *pDat_e);



#endif
