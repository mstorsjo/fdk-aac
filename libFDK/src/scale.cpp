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

#include "common_fix.h"

#include "genericStds.h"

/**************************************************
 * Inline definitions
 **************************************************/

#define SCALE_INLINE inline


#if defined(__mips__)	/* cppp replaced: elif */
#include "mips/scale.cpp"

#elif defined(__arm__)
#include "arm/scale_arm.cpp"

#endif

#ifndef FUNCTION_scaleValues_SGL
/*!
 *
 *  \brief  Multiply input vector by \f$ 2^{scalefactor} \f$
 *  \param len    must be larger than 4
 *  \return void
 *
 */
#define FUNCTION_scaleValues_SGL
SCALE_INLINE
void scaleValues(FIXP_SGL *vector,  /*!< Vector */
                 INT len,            /*!< Length */
                 INT scalefactor     /*!< Scalefactor */
                 )
{
  INT i;

  /* Return if scalefactor is Zero */
  if (scalefactor==0) return;

  if(scalefactor > 0){
    scalefactor = fixmin_I(scalefactor,(INT)(DFRACT_BITS-1));
    for (i = len&3; i--; )
    {
      *(vector++) <<= scalefactor;
    }
    for (i = len>>2; i--; )
    {
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
    }
  } else {
    INT negScalefactor = fixmin_I(-scalefactor,(INT)DFRACT_BITS-1);
    for (i = len&3; i--; )
    {
      *(vector++) >>= negScalefactor;
    }
    for (i = len>>2; i--; )
    {
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
    }
  }
}
#endif

#ifndef FUNCTION_scaleValues_DBL
/*!
 *
 *  \brief  Multiply input vector by \f$ 2^{scalefactor} \f$
 *  \param len must be larger than 4
 *  \return void
 *
 */
#define FUNCTION_scaleValues_DBL
SCALE_INLINE
void scaleValues(FIXP_DBL *vector,    /*!< Vector */
                 INT len,             /*!< Length */
                 INT scalefactor      /*!< Scalefactor */
                )
{
  INT i;

  /* Return if scalefactor is Zero */
  if (scalefactor==0) return;

  if(scalefactor > 0){
    scalefactor = fixmin_I(scalefactor,(INT)DFRACT_BITS-1);
    for (i = len&3; i--; )
    {
      *(vector++) <<= scalefactor;
    }
    for (i = len>>2; i--; )
    {
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
      *(vector++) <<= scalefactor;
    }
  } else {
    INT negScalefactor = fixmin_I(-scalefactor,(INT)DFRACT_BITS-1);
    for (i = len&3; i--; )
    {
      *(vector++) >>= negScalefactor;
    }
    for (i = len>>2; i--; )
    {
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
      *(vector++) >>= negScalefactor;
    }
  }
}
#endif

#ifndef FUNCTION_scaleValues_DBLDBL
/*!
 *
 *  \brief  Multiply input vector src by \f$ 2^{scalefactor} \f$
 *          and place result into dst
 *  \param dst detination buffer
 *  \param src source buffer
 *  \param len must be larger than 4
 *  \param scalefactor amount of left shifts to be applied
 *  \return void
 *
 */
#define FUNCTION_scaleValues_DBLDBL
SCALE_INLINE
void scaleValues(FIXP_DBL *dst,       /*!< dst Vector */
                 const FIXP_DBL *src, /*!< src Vector */
                 INT len,             /*!< Length */
                 INT scalefactor      /*!< Scalefactor */
                )
{
  INT i;

  /* Return if scalefactor is Zero */
  if (scalefactor==0) {
	if (dst != src)
      FDKmemmove(dst, src, len*sizeof(FIXP_DBL));
  }
  else {

    if(scalefactor > 0){
      scalefactor = fixmin_I(scalefactor,(INT)DFRACT_BITS-1);
      for (i = len&3; i--; )
      {
        *(dst++) = *(src++) << scalefactor;
      }
      for (i = len>>2; i--; )
      {
        *(dst++) = *(src++) << scalefactor;
        *(dst++) = *(src++) << scalefactor;
        *(dst++) = *(src++) << scalefactor;
        *(dst++) = *(src++) << scalefactor;
      }
    } else {
      INT negScalefactor = fixmin_I(-scalefactor,(INT)DFRACT_BITS-1);
      for (i = len&3; i--; )
      {
        *(dst++) = *(src++) >> negScalefactor;
      }
      for (i = len>>2; i--; )
      {
        *(dst++) = *(src++) >> negScalefactor;
        *(dst++) = *(src++) >> negScalefactor;
        *(dst++) = *(src++) >> negScalefactor;
        *(dst++) = *(src++) >> negScalefactor;
      }
    }
  }
}
#endif

#ifndef FUNCTION_scaleValuesWithFactor_DBL
/*!
 *
 *  \brief  Multiply input vector by \f$ 2^{scalefactor} \f$
 *  \param len must be larger than 4
 *  \return void
 *
 */
#define FUNCTION_scaleValuesWithFactor_DBL
SCALE_INLINE
void scaleValuesWithFactor(
        FIXP_DBL *vector,
        FIXP_DBL factor,
        INT len,
        INT scalefactor
        )
{
  INT i;

  /* Compensate fMultDiv2 */
  scalefactor++;

  if(scalefactor > 0){
    scalefactor = fixmin_I(scalefactor,(INT)DFRACT_BITS-1);
    for (i = len&3; i--; )
    {
      *vector = fMultDiv2(*vector, factor) << scalefactor;
      vector++;
    }
    for (i = len>>2; i--; )
    {
      *vector = fMultDiv2(*vector, factor) << scalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) << scalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) << scalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) << scalefactor; vector++;
    }
  } else {
    INT negScalefactor = fixmin_I(-scalefactor,(INT)DFRACT_BITS-1);
    for (i = len&3; i--; )
    {
      *vector = fMultDiv2(*vector, factor) >> negScalefactor;
      vector++;
    }
    for (i = len>>2; i--; )
    {
      *vector = fMultDiv2(*vector, factor) >> negScalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) >> negScalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) >> negScalefactor; vector++;
      *vector = fMultDiv2(*vector, factor) >> negScalefactor; vector++;
    }
  }
}
#endif /* FUNCTION_scaleValuesWithFactor_DBL */


/*******************************************

IMPORTANT NOTE for usage of getScalefactor()

If the input array contains negative values too, then these functions may sometimes return
the actual maximum value minus 1, due to the nature of the applied algorithm.
So be careful with possible fractional -1 values that may lead to overflows when being fPow2()'ed.

********************************************/



#ifndef FUNCTION_getScalefactorShort
/*!
 *
 *  \brief Calculate max possible scale factor for input vector of shorts
 *
 *  \return Maximum scale factor / possible left shift
 *
 */
#define FUNCTION_getScalefactorShort
SCALE_INLINE
INT getScalefactorShort(const SHORT *vector, /*!< Pointer to input vector */
                        INT len              /*!< Length of input vector */
                       )
{
  INT i;
  SHORT temp, maxVal = 0;

  for(i=len;i!=0;i--){
    temp = (SHORT)(*vector++);
    maxVal |= (temp^(temp>>(SHORT_BITS-1)));
  }

  return fixmax_I((INT)0,(INT)(fixnormz_D((INT)maxVal) - (INT)1 - (INT)(DFRACT_BITS - SHORT_BITS)));
}
#endif

#ifndef FUNCTION_getScalefactorPCM
/*!
 *
 *  \brief Calculate max possible scale factor for input vector of shorts
 *
 *  \return Maximum scale factor
 *
 */
#define FUNCTION_getScalefactorPCM
SCALE_INLINE
INT getScalefactorPCM(const INT_PCM *vector, /*!< Pointer to input vector */
                      INT len,               /*!< Length of input vector */
                      INT stride
                      )
{
  INT i;
  INT_PCM temp, maxVal = 0;

  for(i=len;i!=0;i--){
    temp = (INT_PCM)(*vector); vector+=stride;
    maxVal |= (temp^(temp>>((sizeof(INT_PCM)*8)-1)));
  }
  return fixmax_I((INT)0,(INT)(fixnormz_D((INT)maxVal) - (INT)1 - (INT)(DFRACT_BITS - SAMPLE_BITS)));
}
#endif

#ifndef FUNCTION_getScalefactorShort
/*!
 *
 *  \brief Calculate max possible scale factor for input vector of shorts
 *  \param stride, item increment between verctor members.
 *  \return Maximum scale factor
 *
 */
#define FUNCTION_getScalefactorShort
SCALE_INLINE
INT getScalefactorShort(const SHORT *vector, /*!< Pointer to input vector */
                        INT len,             /*!< Length of input vector */
                        INT stride
                       )
{
  INT i;
  SHORT temp, maxVal = 0;

  for(i=len;i!=0;i--){
    temp = (SHORT)(*vector); vector+=stride;
    maxVal |= (temp^(temp>>(SHORT_BITS-1)));
  }

  return fixmax_I((INT)0,(INT)(fixnormz_D((INT)maxVal) - (INT)1 - (INT)(DFRACT_BITS - SHORT_BITS)));
}
#endif

#ifndef FUNCTION_getScalefactor_DBL
/*!
 *
 *  \brief Calculate max possible scale factor for input vector
 *
 *  \return Maximum scale factor
 *
 *  This function can constitute a significant amount of computational complexity - very much depending on the
 *  bitrate. Since it is a rather small function, effective assembler optimization might be possible.
 *
 */
#define FUNCTION_getScalefactor_DBL
SCALE_INLINE
INT getScalefactor(const FIXP_DBL *vector, /*!< Pointer to input vector */
                   INT len)                /*!< Length of input vector */
{
  INT i;
  FIXP_DBL temp, maxVal = (FIXP_DBL)0;

  for(i=len;i!=0;i--){
    temp = (LONG)(*vector++);
    maxVal |= (FIXP_DBL)((LONG)temp^(LONG)(temp>>(DFRACT_BITS-1)));
  }

  return fixmax_I((INT)0,(INT)(fixnormz_D(maxVal) - 1));
}
#endif

#ifndef FUNCTION_getScalefactor_SGL
#define FUNCTION_getScalefactor_SGL
SCALE_INLINE
INT getScalefactor(const FIXP_SGL *vector, /*!< Pointer to input vector */
                   INT len)                /*!< Length of input vector */
{
  INT i;
  SHORT temp, maxVal = (FIXP_SGL)0;

  for(i=len;i!=0;i--){
    temp = (SHORT)(*vector++);
    maxVal |= (temp^(temp>>(FRACT_BITS-1)));
  }

  return fixmax_I((INT)0,(INT)(fixnormz_D(FX_SGL2FX_DBL((FIXP_SGL)maxVal)) - 1));
}
#endif

