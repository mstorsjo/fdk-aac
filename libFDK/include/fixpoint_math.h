/***************************  Fraunhofer IIS FDK Tools  **********************

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


   $Id$
   Author(s):   M. Gayer
   Description: Fixed point specific mathematical functions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __fixpoint_math_H
#define __fixpoint_math_H


#include "common_fix.h"


#define LD_DATA_SCALING (64.0f)
#define LD_DATA_SHIFT   6   /* pow(2, LD_DATA_SHIFT) = LD_DATA_SCALING */
/*#define SIMULATE_MIPS_DIV */  /* schur_div() in C that simulates the inline asm schur_div() on MIPS */

/**
 * \brief deprecated. Use fLog2() instead.
 */
FIXP_DBL CalcLdData(FIXP_DBL op);

void LdDataVector(FIXP_DBL *srcVector, FIXP_DBL *destVector, INT number);

FIXP_DBL CalcInvLdData(FIXP_DBL op);


void     InitLdInt();
FIXP_DBL CalcLdInt(INT i);

extern const USHORT sqrt_tab[49];

inline FIXP_DBL sqrtFixp_lookup(FIXP_DBL x)
{
  UINT y = (INT)x;
  UCHAR is_zero=(y==0);
  INT zeros=fixnormz_D(y) & 0x1e;
  y<<=zeros;
  UINT idx=(y>>26)-16;
  USHORT frac=(y>>10)&0xffff;
  USHORT nfrac=0xffff^frac;
  UINT t=nfrac*sqrt_tab[idx]+frac*sqrt_tab[idx+1];
  t=t>>(zeros>>1);
  return(is_zero ? 0 : t);
}

inline FIXP_DBL sqrtFixp_lookup(FIXP_DBL x, INT *x_e)
{
  UINT y = (INT)x;
  INT e;

  if (x == (FIXP_DBL)0) {
    return x;
  }

  /* Normalize */
  e=fixnormz_D(y);
  y<<=e;
  e  = *x_e - e + 2;

  /* Correct odd exponent. */
  if (e & 1) {
    y >>= 1;
    e ++;
  }
  /* Get square root */
  UINT idx=(y>>26)-16;
  USHORT frac=(y>>10)&0xffff;
  USHORT nfrac=0xffff^frac;
  UINT t=nfrac*sqrt_tab[idx]+frac*sqrt_tab[idx+1];

  /* Write back exponent */
  *x_e = e >> 1;
  return (FIXP_DBL)(LONG)(t>>1);
}



FIXP_DBL sqrtFixp(FIXP_DBL op);

void InitInvSqrtTab();

FIXP_DBL invSqrtNorm2(FIXP_DBL op, INT *shift);

/*****************************************************************************

    functionname: invFixp
    description:  delivers 1/(op)

*****************************************************************************/
inline FIXP_DBL invFixp(FIXP_DBL op)
{
    INT tmp_exp ;
    FIXP_DBL tmp_inv = invSqrtNorm2(op, &tmp_exp) ;
    FDK_ASSERT((31-(2*tmp_exp+1))>=0) ;
    return ( fPow2Div2( (FIXP_DBL)tmp_inv ) >> (31-(2*tmp_exp+1)) ) ;
}



#if defined(__mips__) && (__GNUC__==2)

#define FUNCTION_schur_div
inline FIXP_DBL schur_div(FIXP_DBL num,FIXP_DBL denum, INT count)
{
  INT result, tmp ;
   __asm__ ("srl %1, %2, 15\n"
            "div %3, %1\n" : "=lo" (result)
                           : "%d" (tmp), "d" (denum) ,  "d" (num)
                           : "hi" ) ;
  return result<<16 ;
}

/*###########################################################################################*/
#elif defined(__mips__) && (__GNUC__==3)

#define FUNCTION_schur_div
inline FIXP_DBL schur_div(FIXP_DBL num,FIXP_DBL denum, INT count)
{
  INT result, tmp;

   __asm__ ("srl  %[tmp], %[denum], 15\n"
            "div %[result], %[num], %[tmp]\n"
            : [tmp] "+r" (tmp), [result]"=r"(result)
            : [denum]"r"(denum), [num]"r"(num)
            : "hi", "lo");
  return result << (DFRACT_BITS-16);
}

/*###########################################################################################*/
#elif defined(SIMULATE_MIPS_DIV)

#define FUNCTION_schur_div
inline FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT count)
{
    FDK_ASSERT (count<=DFRACT_BITS-1);
    FDK_ASSERT (num>=(FIXP_DBL)0);
    FDK_ASSERT (denum>(FIXP_DBL)0);
    FDK_ASSERT (num <= denum);

    INT tmp = denum >> (count-1);
    INT result = 0;

    while (num > tmp)
    {
        num -= tmp;
        result++;
    }

    return result << (DFRACT_BITS-count);
}

/*###########################################################################################*/
#endif /* target architecture selector */

#if !defined(FUNCTION_schur_div)
/**
 * \brief Divide two FIXP_DBL values with given precision.
 * \param num dividend
 * \param denum divisor
 * \param count amount of significant bits of the result (starting to the MSB)
 * \return num/divisor
 */
FIXP_DBL schur_div(FIXP_DBL num,FIXP_DBL denum, INT count);
#endif



FIXP_DBL mul_dbl_sgl_rnd (const FIXP_DBL op1,
                          const FIXP_SGL op2);

/**
 * \brief multiply two values with normalization, thus max precision.
 * Author: Robert Weidner
 *
 * \param f1 first factor
 * \param f2 secod factor
 * \param result_e pointer to an INT where the exponent of the result is stored into
 * \return mantissa of the product f1*f2
 */
FIXP_DBL fMultNorm(
        FIXP_DBL f1,
        FIXP_DBL f2,
        INT *result_e
        );

inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2)
{
  FIXP_DBL m;
  INT e;

  m = fMultNorm(f1, f2, &e);

  m = scaleValueSaturate(m, e);

  return m;
}

/**
 * \brief Divide 2 FIXP_DBL values with normalization of input values.
 * \param num numerator
 * \param denum denomintator
 * \return num/denum with exponent = 0
 */
FIXP_DBL fDivNorm(FIXP_DBL num, FIXP_DBL denom, INT *result_e);

/**
 * \brief Divide 2 FIXP_DBL values with normalization of input values.
 * \param num numerator
 * \param denum denomintator
 * \param result_e pointer to an INT where the exponent of the result is stored into
 * \return num/denum with exponent = *result_e
 */
FIXP_DBL fDivNorm(FIXP_DBL num, FIXP_DBL denom);

/**
 * \brief Divide 2 FIXP_DBL values with normalization of input values.
 * \param num numerator
 * \param denum denomintator
 * \return num/denum with exponent = 0
 */
FIXP_DBL fDivNormHighPrec(FIXP_DBL L_num, FIXP_DBL L_denum, INT *result_e);

/**
 * \brief Calculate log(argument)/log(2) (logarithm with base 2). deprecated. Use fLog2() instead.
 * \param arg mantissa of the argument
 * \param arg_e exponent of the argument
 * \param result_e pointer to an INT to store the exponent of the result
 * \return the mantissa of the result.
 * \param
 */
FIXP_DBL CalcLog2(FIXP_DBL arg, INT arg_e, INT *result_e);

/**
 * \brief return 2 ^ (exp * 2^exp_e)
 * \param exp_m mantissa of the exponent to 2.0f
 * \param exp_e exponent of the exponent to 2.0f
 * \param result_e pointer to a INT where the exponent of the result will be stored into
 * \return mantissa of the result
 */
FIXP_DBL f2Pow(const FIXP_DBL exp_m, const INT exp_e, INT *result_e);

/**
 * \brief return 2 ^ (exp_m * 2^exp_e). This version returns only the mantissa with implicit exponent of zero.
 * \param exp_m mantissa of the exponent to 2.0f
 * \param exp_e exponent of the exponent to 2.0f
 * \return mantissa of the result
 */
FIXP_DBL f2Pow(const FIXP_DBL exp_m, const INT exp_e);

/**
 * \brief return x ^ (exp * 2^exp_e), where log2(x) = baseLd_m * 2^(baseLd_e). This saves
 *        the need to compute log2() of constant values (when x is a constant).
 * \param ldx_m mantissa of log2() of x.
 * \param ldx_e exponent of log2() of x.
 * \param exp_m mantissa of the exponent to 2.0f
 * \param exp_e exponent of the exponent to 2.0f
 * \param result_e pointer to a INT where the exponent of the result will be stored into
 * \return mantissa of the result
 */
FIXP_DBL fLdPow(
        FIXP_DBL baseLd_m,
        INT baseLd_e,
        FIXP_DBL exp_m, INT exp_e,
        INT *result_e
        );

/**
 * \brief return x ^ (exp * 2^exp_e), where log2(x) = baseLd_m * 2^(baseLd_e). This saves
 *        the need to compute log2() of constant values (when x is a constant). This version
 *        does not return an exponent, which is implicitly 0.
 * \param ldx_m mantissa of log2() of x.
 * \param ldx_e exponent of log2() of x.
 * \param exp_m mantissa of the exponent to 2.0f
 * \param exp_e exponent of the exponent to 2.0f
 * \return mantissa of the result
 */
FIXP_DBL fLdPow(
        FIXP_DBL baseLd_m, INT baseLd_e,
        FIXP_DBL exp_m, INT exp_e
        );

/**
 * \brief return (base * 2^base_e) ^ (exp * 2^exp_e). Use fLdPow() instead whenever possible.
 * \param base_m mantissa of the base.
 * \param base_e exponent of the base.
 * \param exp_m mantissa of power to be calculated of the base.
 * \param exp_e exponent of power to be calculated of the base.
 * \param result_e pointer to a INT where the exponent of the result will be stored into.
 * \return mantissa of the result.
 */
FIXP_DBL fPow(FIXP_DBL base_m, INT base_e, FIXP_DBL exp_m, INT exp_e, INT *result_e);

/**
 * \brief return (base * 2^base_e) ^ N
 * \param base mantissa of the base
 * \param base_e exponent of the base
 * \param power to be calculated of the base
 * \param result_e pointer to a INT where the exponent of the result will be stored into
 * \return mantissa of the result
 */
FIXP_DBL fPowInt(FIXP_DBL base_m, INT base_e, INT N, INT *result_e);

/**
 * \brief calculate logarithm of base 2 of x_m * 2^(x_e)
 * \param x_m mantissa of the input value.
 * \param x_e exponent of the input value.
 * \param pointer to an INT where the exponent of the result is returned into.
 * \return mantissa of the result.
 */
FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e, INT *result_e);

/**
 * \brief calculate logarithm of base 2 of x_m * 2^(x_e)
 * \param x_m mantissa of the input value.
 * \param x_e exponent of the input value.
 * \return mantissa of the result with implicit exponent of LD_DATA_SHIFT.
 */
FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e);

/**
 * \brief Add with saturation of the result.
 * \param a first summand
 * \param b second summand
 * \return saturated sum of a and b.
 */
inline FIXP_SGL fAddSaturate(const FIXP_SGL a, const FIXP_SGL b)
{
  LONG sum;

  sum = (LONG)(SHORT)a + (LONG)(SHORT)b;
  sum = fMax(fMin((INT)sum, (INT)MAXVAL_SGL), (INT)MINVAL_SGL);
  return (FIXP_SGL)(SHORT)sum;
}

/**
 * \brief Add with saturation of the result.
 * \param a first summand
 * \param b second summand
 * \return saturated sum of a and b.
 */
inline FIXP_DBL fAddSaturate(const FIXP_DBL a, const FIXP_DBL b)
{
  LONG sum;

  sum = (LONG)(a>>1) + (LONG)(b>>1);
  sum = fMax(fMin((INT)sum, (INT)(MAXVAL_DBL>>1)), (INT)(MINVAL_DBL>>1));
  return (FIXP_DBL)(LONG)(sum<<1);
}

#define TEST_ROUNDING   0


#if TEST_ROUNDING
void checkRound();
void checkRound2();
#endif


/*****************************************************************************

 array for 1/n, n=1..50

****************************************************************************/

  extern const FIXP_DBL invCount[50];

  LNK_SECTION_INITCODE
  inline void InitInvInt(void) {}


/**
 * \brief Calculate the value of 1/i where i is a integer value. It supports
 *        input values from 1 upto 50.
 * \param intValue Integer input value.
 * \param FIXP_DBL representation of 1/intValue
 */
inline FIXP_DBL GetInvInt(int intValue)
{
  FDK_ASSERT((intValue > 0) && (intValue < 50));
  FDK_ASSERT(intValue<50);
	return invCount[intValue];
}


#endif

