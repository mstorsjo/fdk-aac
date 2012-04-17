/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2004)
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
/*!
  \file
  \brief  FDK Fixed Point Arithmetic Library Interface $Revision: 36841 $
*/

/*!
  \mainpage Fixed Point Arithmetic Library Documentation

  Information in this SDK is subject to change without notice. Companies,
  names, and data used in examples herein are fictitious unless otherwise
  noted.

  Product and corporate names may be trademarks or registered trademarks
  of other companies. They are used for explanation only, with no intent
  to infringe.

  No part of this publication may be reproduced or utilized in any form or
  by any means, electronic or mechanical, including photocopying and
  microfilm, without permission in writing from the publisher.
*/

#ifndef __TRANSCENDENT_H
#define __TRANSCENDENT_H

#include "sbrdecoder.h"
#include "sbr_rom.h"

/************************************************************************/
/*!
  \brief   Get number of octaves between frequencies a and b

  The Result is scaled with 1/8.
  The valid range for a and b is 1 to LOG_DUALIS_TABLE_SIZE.

  \return   ld(a/b) / 8
*/
/************************************************************************/
static inline FIXP_SGL FDK_getNumOctavesDiv8(INT a, /*!< lower band */
                                             INT b) /*!< upper band */
{
  return ( (SHORT)((LONG)(CalcLdInt(b) - CalcLdInt(a))>>(FRACT_BITS-3)) );
}


/************************************************************************/
/*!
  \brief   Add two values given by mantissa and exponent.

  Mantissas are in fract format with values between 0 and 1. <br>
  The base for exponents is 2.  Example:  \f$  a = a\_m * 2^{a\_e}  \f$<br>
*/
/************************************************************************/
inline void FDK_add_MantExp(FIXP_SGL a_m, /*!< Mantissa of 1st operand a */
                     SCHAR     a_e,       /*!< Exponent of 1st operand a */
                     FIXP_SGL  b_m,       /*!< Mantissa of 2nd operand b */
                     SCHAR     b_e,       /*!< Exponent of 2nd operand b */
                     FIXP_SGL *ptrSum_m,  /*!< Mantissa of result */
                     SCHAR    *ptrSum_e)  /*!< Exponent of result */
{
  FIXP_DBL accu;
  int   shift;
  int   shiftAbs;

  FIXP_DBL shiftedMantissa;
  FIXP_DBL otherMantissa;

  /* Equalize exponents of the summands.
     For the smaller summand, the exponent is adapted and
     for compensation, the mantissa is shifted right. */

  shift = (int)(a_e - b_e);

  shiftAbs = (shift>0)? shift : -shift;
  shiftAbs = (shiftAbs < DFRACT_BITS-1)? shiftAbs : DFRACT_BITS-1;
  shiftedMantissa = (shift>0)? (FX_SGL2FX_DBL(b_m) >> shiftAbs) : (FX_SGL2FX_DBL(a_m) >> shiftAbs);
  otherMantissa = (shift>0)? FX_SGL2FX_DBL(a_m) : FX_SGL2FX_DBL(b_m);
  *ptrSum_e = (shift>0)? a_e : b_e;

  accu = (shiftedMantissa >> 1) + (otherMantissa >> 1);
  /* shift by 1 bit to avoid overflow */

  if ( (accu >= (FL2FXCONST_DBL(0.5f) - (FIXP_DBL)1)) || (accu <= FL2FXCONST_DBL(-0.5f)) )
    *ptrSum_e += 1;
  else
    accu = (shiftedMantissa + otherMantissa);

  *ptrSum_m = FX_DBL2FX_SGL(accu);

}

inline void FDK_add_MantExp(FIXP_DBL a,   /*!< Mantissa of 1st operand a */
                     SCHAR     a_e,       /*!< Exponent of 1st operand a */
                     FIXP_DBL  b,         /*!< Mantissa of 2nd operand b */
                     SCHAR     b_e,       /*!< Exponent of 2nd operand b */
                     FIXP_DBL *ptrSum,    /*!< Mantissa of result */
                     SCHAR    *ptrSum_e)  /*!< Exponent of result */
{
  FIXP_DBL accu;
  int   shift;
  int   shiftAbs;

  FIXP_DBL shiftedMantissa;
  FIXP_DBL otherMantissa;

  /* Equalize exponents of the summands.
     For the smaller summand, the exponent is adapted and
     for compensation, the mantissa is shifted right. */

  shift = (int)(a_e - b_e);

  shiftAbs = (shift>0)? shift : -shift;
  shiftAbs = (shiftAbs < DFRACT_BITS-1)? shiftAbs : DFRACT_BITS-1;
  shiftedMantissa = (shift>0)? (b >> shiftAbs) : (a >> shiftAbs);
  otherMantissa = (shift>0)? a : b;
  *ptrSum_e = (shift>0)? a_e : b_e;

  accu = (shiftedMantissa >> 1) + (otherMantissa >> 1);
  /* shift by 1 bit to avoid overflow */

  if ( (accu >= (FL2FXCONST_DBL(0.5f) - (FIXP_DBL)1)) || (accu <= FL2FXCONST_DBL(-0.5f)) )
    *ptrSum_e += 1;
  else
    accu = (shiftedMantissa + otherMantissa);

  *ptrSum = accu;

}

/************************************************************************/
/*!
  \brief   Divide two values given by mantissa and exponent.

  Mantissas are in fract format with values between 0 and 1. <br>
  The base for exponents is 2.  Example:  \f$  a = a\_m * 2^{a\_e}  \f$<br>

  For performance reasons, the division is based on a table lookup
  which limits accuracy.
*/
/************************************************************************/
static inline void FDK_divide_MantExp(FIXP_SGL a_m,           /*!< Mantissa of dividend a */
                                      SCHAR     a_e,          /*!< Exponent of dividend a */
                                      FIXP_SGL  b_m,          /*!< Mantissa of divisor b */
                                      SCHAR     b_e,          /*!< Exponent of divisor b */
                                      FIXP_SGL *ptrResult_m,  /*!< Mantissa of quotient a/b */
                                      SCHAR    *ptrResult_e)  /*!< Exponent of quotient a/b */

{
  int preShift, postShift, index, shift;
  FIXP_DBL ratio_m;
  FIXP_SGL  bInv_m = FL2FXCONST_SGL(0.0f);

  preShift = CntLeadingZeros(FX_SGL2FX_DBL(b_m));

  /*
    Shift b into the range from 0..INV_TABLE_SIZE-1,

    E.g. 10 bits must be skipped for INV_TABLE_BITS 8:
    - leave 8 bits as index for table
    - skip sign bit,
    - skip first bit of mantissa, because this is always the same (>0.5)

    We are dealing with energies, so we need not care
    about negative numbers
  */

  /*
    The first interval has half width so the lowest bit of the index is
    needed for a doubled resolution.
  */
  shift = (FRACT_BITS - 2 - INV_TABLE_BITS - preShift);

  index = (shift<0)? (LONG)b_m << (-shift) : (LONG)b_m >> shift;


  /* The index has INV_TABLE_BITS +1 valid bits here. Clear the other bits. */
  index &= (1 << (INV_TABLE_BITS+1)) - 1;

    /* Remove offset of half an interval */
  index--;

    /* Now the lowest bit is shifted out */
  index = index >> 1;

    /* Fetch inversed mantissa from table: */
  bInv_m = (index<0)? bInv_m : FDK_sbrDecoder_invTable[index];

    /* Multiply a with the inverse of b: */
  ratio_m = (index<0)? FX_SGL2FX_DBL(a_m >> 1) : fMultDiv2(bInv_m,a_m);

  postShift = CntLeadingZeros(ratio_m)-1;

  *ptrResult_m = FX_DBL2FX_SGL(ratio_m << postShift);
  *ptrResult_e = a_e - b_e + 1 + preShift - postShift;
}

static inline void FDK_divide_MantExp(FIXP_DBL a_m,           /*!< Mantissa of dividend a */
                                      SCHAR     a_e,          /*!< Exponent of dividend a */
                                      FIXP_DBL  b_m,          /*!< Mantissa of divisor b */
                                      SCHAR     b_e,          /*!< Exponent of divisor b */
                                      FIXP_DBL *ptrResult_m,  /*!< Mantissa of quotient a/b */
                                      SCHAR    *ptrResult_e)  /*!< Exponent of quotient a/b */

{
  int preShift, postShift, index, shift;
  FIXP_DBL ratio_m;
  FIXP_SGL  bInv_m = FL2FXCONST_SGL(0.0f);

  preShift = CntLeadingZeros(b_m);

  /*
    Shift b into the range from 0..INV_TABLE_SIZE-1,

    E.g. 10 bits must be skipped for INV_TABLE_BITS 8:
    - leave 8 bits as index for table
    - skip sign bit,
    - skip first bit of mantissa, because this is always the same (>0.5)

    We are dealing with energies, so we need not care
    about negative numbers
  */

  /*
    The first interval has half width so the lowest bit of the index is
    needed for a doubled resolution.
  */
  shift = (DFRACT_BITS - 2 - INV_TABLE_BITS - preShift);

  index = (shift<0)? (LONG)b_m << (-shift) : (LONG)b_m >> shift;


  /* The index has INV_TABLE_BITS +1 valid bits here. Clear the other bits. */
  index &= (1 << (INV_TABLE_BITS+1)) - 1;

    /* Remove offset of half an interval */
  index--;

    /* Now the lowest bit is shifted out */
  index = index >> 1;

    /* Fetch inversed mantissa from table: */
  bInv_m = (index<0)? bInv_m : FDK_sbrDecoder_invTable[index];

    /* Multiply a with the inverse of b: */
  ratio_m = (index<0)? (a_m >> 1) : fMultDiv2(bInv_m,a_m);

  postShift = CntLeadingZeros(ratio_m)-1;

  *ptrResult_m = ratio_m << postShift;
  *ptrResult_e = a_e - b_e + 1 + preShift - postShift;
}

/*!
  \brief   Calculate the squareroot of a number given by mantissa and exponent

  Mantissa is in fract format with values between 0 and 1. <br>
  The base for the exponent is 2.  Example:  \f$  a = a\_m * 2^{a\_e}  \f$<br>
  The operand is addressed via pointers and will be overwritten with the result.

  For performance reasons, the square root is based on a table lookup
  which limits accuracy.
*/
static inline void FDK_sqrt_MantExp(FIXP_DBL *mantissa,    /*!< Pointer to mantissa */
                                    SCHAR    *exponent,
                                    const SCHAR *destScale)
{
  FIXP_DBL input_m = *mantissa;
  int   input_e = (int) *exponent;
  FIXP_DBL result = FL2FXCONST_DBL(0.0f);
  int    result_e = -FRACT_BITS;

  /* Call lookup square root, which does internally normalization. */
  result   = sqrtFixp_lookup(input_m, &input_e);
  result_e = input_e;

  /* Write result */
  if (exponent==destScale) {
    *mantissa = result;
    *exponent = result_e;
  } else {
    int shift = result_e - *destScale;
    *mantissa = (shift>=0) ? result << (INT)fixMin(DFRACT_BITS-1,shift)
                           : result >> (INT)fixMin(DFRACT_BITS-1,-shift);
    *exponent = *destScale;
  }
}


#endif
