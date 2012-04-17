/********************************  Fraunhofer IIS  ***************************

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


   $Id:
   Author(s):
   Description: ARM scaling operations

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if defined(__GNUC__) /* GCC Compiler */	/* cppp replaced: elif */

#if defined(__ARM_ARCH_6__)

inline static INT shiftRightSat(INT src, int scale)
{
  INT result;
  asm(
      "ssat %0,%2,%0;\n"

      : "=&r"(result)
      : "r"(src>>scale), "M"(SAMPLE_BITS)
      );

  return result;
}

  #define SATURATE_INT_PCM_RIGHT_SHIFT(src, scale) shiftRightSat(src, scale)

inline static INT shiftLeftSat(INT src, int scale)
{
  INT result;
  asm(
      "ssat %0,%2,%0;\n"

      : "=&r"(result)
      : "r"(src<<scale), "M"(SAMPLE_BITS)
      );

  return result;
}

  #define SATURATE_INT_PCM_LEFT_SHIFT(src, scale)  shiftLeftSat(src, scale)

#endif /* __ARM_ARCH_6__ */

#endif /* compiler selection */

#define FUNCTION_scaleValueInPlace
inline
void scaleValueInPlace(FIXP_DBL *value, /*!< Value */
                       INT scalefactor   /*!< Scalefactor */
                       )
{
  INT newscale;
  if ((newscale = scalefactor) >= 0)
    *value <<= newscale;
  else
    *value >>= -newscale;
}


  #define SATURATE_RIGHT_SHIFT(src, scale, dBits)                                                        \
      ( (((LONG)(src) ^ ((LONG)(src) >> (DFRACT_BITS-1)))>>(scale)) > (LONG)(((1U)<<((dBits)-1))-1))     \
          ? ((LONG)(src) >> (DFRACT_BITS-1)) ^ (LONG)(((1U)<<((dBits)-1))-1)                             \
          : ((LONG)(src) >> (scale))

  #define SATURATE_LEFT_SHIFT(src, scale, dBits)                                                         \
      ( ((LONG)(src) ^ ((LONG)(src) >> (DFRACT_BITS-1))) > ((LONG)(((1U)<<((dBits)-1))-1) >> (scale)) )  \
          ? ((LONG)(src) >> (DFRACT_BITS-1)) ^ (LONG)(((1U)<<((dBits)-1))-1)                             \
          : ((LONG)(src) << (scale))

