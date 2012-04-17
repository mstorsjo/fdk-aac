/***************************  Fraunhofer IIS FDK Tools  **********************

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


   $Id$
   Author(s):
   Description: Scaling operations

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef SCALE_H
#define SCALE_H

#include "common_fix.h"
#include "genericStds.h"
#include "fixminmax.h"

  #define SCALE_INLINE inline


#if defined(__arm__)	/* cppp replaced: elif */
#include "arm/scale.h"

#elif defined(__mips__)	/* cppp replaced: elif */
#include "mips/scale.h"

#endif


#include "../src/scale.cpp"

#ifndef FUNCTION_scaleValue
/*!
 *
 *  \brief Multiply input by \f$ 2^{scalefactor} \f$
 *
 *  \return Scaled input
 *
 */
#define FUNCTION_scaleValue
inline
FIXP_DBL scaleValue(const FIXP_DBL value, /*!< Value */
                    INT scalefactor   /*!< Scalefactor */
                   )
{
  if(scalefactor > 0)
    return (value<<scalefactor);
  else
    return (value>>(-scalefactor));
}
#endif

#ifndef FUNCTION_scaleValueSaturate
/*!
 *
 *  \brief Multiply input by \f$ 2^{scalefactor} \f$
 *  \param value The value to be scaled.
 *  \param the shift amount
 *  \return \f$ value * 2^scalefactor \f$
 *
 */
#define FUNCTION_scaleValueSaturate
inline
FIXP_DBL scaleValueSaturate(
        const FIXP_DBL value,
        INT scalefactor
        )
{
  if(scalefactor > 0) {
    if (fNorm(value) < scalefactor && value != (FIXP_DBL)0) {
      return (FIXP_DBL)MAXVAL_DBL;
    } else {
      return (value<<scalefactor);
    }
  } else {
    return (value>>(-scalefactor));
  }
}
#endif

#ifndef FUNCTION_scaleValueInPlace
/*!
 *
 *  \brief Multiply input by \f$ 2^{scalefactor} \f$ in place
 *
 *  \return void
 *
 */
#define FUNCTION_scaleValueInPlace
inline
void scaleValueInPlace(
        FIXP_DBL *value, /*!< Value */
        INT scalefactor   /*!< Scalefactor */
        )
{
  INT newscale;
  /* Note: The assignment inside the if conditional allows combining a load with the compare to zero (on ARM and maybe others) */
  if ((newscale = (scalefactor)) >= 0) {
    *(value) <<= newscale;
  } else {
    *(value) >>= -newscale;
  }
}
#endif

/*!
 *
 *  \brief  Scale input value by 2^{scale} and saturate output to 2^{dBits-1}
 *  \return scaled and saturated value
 *
 *  This macro scales src value right or left and applies saturation to (2^dBits)-1
 *  maxima output.
 */

#ifndef SATURATE_RIGHT_SHIFT
  #define SATURATE_RIGHT_SHIFT(src, scale, dBits)                                                      \
            ( (((LONG)(src)>>(scale)) > (LONG)(((1U)<<((dBits)-1))-1))      ? (LONG)(((1U)<<((dBits)-1))-1)    \
              : (((LONG)(src)>>(scale)) < ~((LONG)(((1U)<<((dBits)-1))-1))) ? ~((LONG)(((1U)<<((dBits)-1))-1)) \
              : ((LONG)(src) >> (scale)) )
#endif

#ifndef SATURATE_LEFT_SHIFT
  #define SATURATE_LEFT_SHIFT(src, scale, dBits)                                                       \
            ( ((LONG)(src) > ((LONG)(((1U)<<((dBits)-1))-1)>>(scale)))    ? (LONG)(((1U)<<((dBits)-1))-1)      \
              : ((LONG)(src) < ~((LONG)(((1U)<<((dBits)-1))-1)>>(scale))) ? ~((LONG)(((1U)<<((dBits)-1))-1))   \
              : ((LONG)(src) << (scale)) )
#endif

#ifndef SATURATE_SHIFT
#define SATURATE_SHIFT(src, scale, dBits)               \
     ( ((scale) < 0)                                      \
      ? SATURATE_LEFT_SHIFT((src), -(scale), (dBits))   \
      : SATURATE_RIGHT_SHIFT((src), (scale), (dBits)) )
#endif

/*
 * Alternative shift and saturate left, saturates to -0.99999 instead of -1.0000
 * to avoid problems when inverting the sign of the result.
 */
#ifndef SATURATE_LEFT_SHIFT_ALT
#define SATURATE_LEFT_SHIFT_ALT(src, scale, dBits)                                                     \
            ( ((LONG)(src) > ((LONG)(((1U)<<((dBits)-1))-1)>>(scale)))    ? (LONG)(((1U)<<((dBits)-1))-1)      \
              : ((LONG)(src) < ~((LONG)(((1U)<<((dBits)-1))-2)>>(scale))) ? ~((LONG)(((1U)<<((dBits)-1))-2))   \
              : ((LONG)(src) << (scale)) )
#endif

#ifndef SATURATE_RIGHT_SHIFT_ALT
  #define SATURATE_RIGHT_SHIFT_ALT(src, scale, dBits)                                                  \
            ( (((LONG)(src)>>(scale)) > (LONG)(((1U)<<((dBits)-1))-1))      ? (LONG)(((1U)<<((dBits)-1))-1)    \
              : (((LONG)(src)>>(scale)) < ~((LONG)(((1U)<<((dBits)-1))-2))) ? ~((LONG)(((1U)<<((dBits)-1))-2)) \
              : ((LONG)(src) >> (scale)) )
#endif

#ifndef SATURATE_INT_PCM_RIGHT_SHIFT
#define SATURATE_INT_PCM_RIGHT_SHIFT(src, scale) SATURATE_RIGHT_SHIFT(src, scale, SAMPLE_BITS)
#endif

#ifndef SATURATE_INT_PCM_LEFT_SHIFT
#define SATURATE_INT_PCM_LEFT_SHIFT(src, scale) SATURATE_LEFT_SHIFT(src, scale, SAMPLE_BITS)
#endif

#endif /* #ifndef SCALE_H */
