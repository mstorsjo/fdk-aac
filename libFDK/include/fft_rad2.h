/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2003)
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
   Author(s):   M. Lohwasser, M. Gayer
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _FFT_RAD2_H
#define _FFT_RAD2_H


#include "common_fix.h"

/**
 * \brief Performe an inplace  complex valued FFT of 2^n length
 *
 * \param x Input/Output data buffer. The input data must have at least 1 bit scale headroom.
 *          The values are interleaved, real/imag pairs.
 * \param ldn log2 of FFT length
 * \param trigdata Pointer to a sinetable of a length of at least (2^ldn)/2 sine values.
 * \param trigDataSize length of the sinetable "trigdata".
 */
void dit_fft(FIXP_DBL *x, const INT ldn, const FIXP_STP *trigdata, const INT trigDataSize) ;

/**
 * \brief Performe an inplace complex valued inverse FFT of 2^n length
 *
 * \param x Input/Output data buffer. The input data must have at least 1 bit scale headroom.
 *          The values are interleaved, real/imag pairs.
 * \param ldn log2 of FFT length
 * \param trigdata Pointer to a sinetable of a length of at least (2^ldn)/2 sine values.
 * \param trigDataSize length of the sinetable "trigdata".
 */
void dit_ifft(FIXP_DBL *x, const INT ldn, const FIXP_STP *trigdata, const INT trigDataSize) ;

/* Rest of the world. */

#define SCALEFACTOR2048       10
#define SCALEFACTOR1024        9
#define SCALEFACTOR512         8
#define SCALEFACTOR256         7
#define SCALEFACTOR128         6
#define SCALEFACTOR64          5
#define SCALEFACTOR32          4
#define SCALEFACTOR16          3
#define SCALEFACTOR8           2
#define SCALEFACTOR4           1
#define SCALEFACTOR2           1

#endif /* _FFT_RAD2_H */

