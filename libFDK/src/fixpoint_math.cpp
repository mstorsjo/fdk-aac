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

#include "fixpoint_math.h"


#define MAX_LD_PRECISION 10
#define LD_PRECISION     10

/* Taylor series coeffcients for ln(1-x), centered at 0 (MacLaurin polinomial). */
#ifndef LDCOEFF_16BIT
LNK_SECTION_CONSTDATA_L1
static const FIXP_DBL ldCoeff[MAX_LD_PRECISION] = {
    FL2FXCONST_DBL(-1.0),
    FL2FXCONST_DBL(-1.0/2.0),
    FL2FXCONST_DBL(-1.0/3.0),
    FL2FXCONST_DBL(-1.0/4.0),
    FL2FXCONST_DBL(-1.0/5.0),
    FL2FXCONST_DBL(-1.0/6.0),
    FL2FXCONST_DBL(-1.0/7.0),
    FL2FXCONST_DBL(-1.0/8.0),
    FL2FXCONST_DBL(-1.0/9.0),
    FL2FXCONST_DBL(-1.0/10.0)
};
#else
LNK_SECTION_CONSTDATA_L1
static const FIXP_SGL ldCoeff[MAX_LD_PRECISION] = {
    FL2FXCONST_SGL(-1.0),
    FL2FXCONST_SGL(-1.0/2.0),
    FL2FXCONST_SGL(-1.0/3.0),
    FL2FXCONST_SGL(-1.0/4.0),
    FL2FXCONST_SGL(-1.0/5.0),
    FL2FXCONST_SGL(-1.0/6.0),
    FL2FXCONST_SGL(-1.0/7.0),
    FL2FXCONST_SGL(-1.0/8.0),
    FL2FXCONST_SGL(-1.0/9.0),
    FL2FXCONST_SGL(-1.0/10.0)
};
#endif

/*****************************************************************************

  functionname: CalcLdData
  description:  Delivers the Logarithm Dualis ld(op)/LD_DATA_SCALING with polynomial approximation.
  input:        Input op is assumed to be double precision fractional 0 < op < 1.0
                This function does not accept negative values.
  output:       For op == 0, the result is saturated to -1.0
                This function does not return positive values since input values are treated as fractional values.
                It does not make sense to input an integer value into this function (and expect a positive output value)
                since input values are treated as fractional values.

*****************************************************************************/

LNK_SECTION_CODE_L1
FIXP_DBL CalcLdData(FIXP_DBL op)
{
  return fLog2(op, 0);
}


/*****************************************************************************
  functionname: LdDataVector
*****************************************************************************/
LNK_SECTION_CODE_L1
void LdDataVector(  FIXP_DBL    *srcVector,
                    FIXP_DBL    *destVector,
                    INT         n)
{
    INT i;
    for ( i=0; i<n; i++) {
        destVector[i] = CalcLdData(srcVector[i]);
    }
}



#define MAX_POW2_PRECISION 8
#ifndef SINETABLE_16BIT
	#define POW2_PRECISION MAX_POW2_PRECISION
#else
	#define POW2_PRECISION 5
#endif

/*
  Taylor series coefficients of the function x^2. The first coefficient is
  ommited (equal to 1.0).

  pow2Coeff[i-1] = (1/i!) d^i(2^x)/dx^i, i=1..MAX_POW2_PRECISION
  To evaluate the taylor series around x = 0, the coefficients are: 1/!i * ln(2)^i
 */
#ifndef POW2COEFF_16BIT
LNK_SECTION_CONSTDATA_L1
static const FIXP_DBL pow2Coeff[MAX_POW2_PRECISION] = {
    FL2FXCONST_DBL(0.693147180559945309417232121458177),    /* ln(2)^1 /1! */
    FL2FXCONST_DBL(0.240226506959100712333551263163332),    /* ln(2)^2 /2! */
    FL2FXCONST_DBL(0.0555041086648215799531422637686218),   /* ln(2)^3 /3! */
    FL2FXCONST_DBL(0.00961812910762847716197907157365887),  /* ln(2)^4 /4! */
    FL2FXCONST_DBL(0.00133335581464284434234122219879962),  /* ln(2)^5 /5! */
    FL2FXCONST_DBL(1.54035303933816099544370973327423e-4),  /* ln(2)^6 /6! */
    FL2FXCONST_DBL(1.52527338040598402800254390120096e-5),  /* ln(2)^7 /7! */
    FL2FXCONST_DBL(1.32154867901443094884037582282884e-6)   /* ln(2)^8 /8! */
};
#else
LNK_SECTION_CONSTDATA_L1
static const FIXP_SGL pow2Coeff[MAX_POW2_PRECISION] = {
    FL2FXCONST_SGL(0.693147180559945309417232121458177),    /* ln(2)^1 /1! */
    FL2FXCONST_SGL(0.240226506959100712333551263163332),    /* ln(2)^2 /2! */
    FL2FXCONST_SGL(0.0555041086648215799531422637686218),   /* ln(2)^3 /3! */
    FL2FXCONST_SGL(0.00961812910762847716197907157365887),  /* ln(2)^4 /4! */
    FL2FXCONST_SGL(0.00133335581464284434234122219879962),  /* ln(2)^5 /5! */
    FL2FXCONST_SGL(1.54035303933816099544370973327423e-4),  /* ln(2)^6 /6! */
    FL2FXCONST_SGL(1.52527338040598402800254390120096e-5),  /* ln(2)^7 /7! */
    FL2FXCONST_SGL(1.32154867901443094884037582282884e-6)   /* ln(2)^8 /8! */
};
#endif



/*****************************************************************************

    functionname: mul_dbl_sgl_rnd
    description:  Multiply with round.
*****************************************************************************/

/* for rounding a dfract to fract */
/* static LONG accu_r = (int64)((INT64(1)<<(DFRACT_BITS-1))>>FRACT_BITS); */
//LNK_SECTION_CONSTDATA
//static const LONG accu_r = 0x00008000;
#define ACCU_R (LONG) 0x00008000

LNK_SECTION_CODE_L1
FIXP_DBL mul_dbl_sgl_rnd (const FIXP_DBL op1, const FIXP_SGL op2)
{
  FIXP_DBL prod;
  LONG v = (LONG)(op1);
  SHORT u = (SHORT)(op2);

  LONG low = u*(v&SGL_MASK);
  low = (low+(ACCU_R>>1)) >> (FRACT_BITS-1);  /* round */
  LONG high = u * ((v>>FRACT_BITS)<<1);

  prod = (LONG)(high+low);

  return((FIXP_DBL)prod);
}


/*****************************************************************************

  functionname: CalcInvLdData
  description:  Delivers the inverse of function CalcLdData().
                Delivers 2^(op*LD_DATA_SCALING)
  input:        Input op is assumed to be fractional -1.0 < op < 1.0
  output:       For op == 0, the result is MAXVAL_DBL (almost 1.0).
                For negative input values the output should be treated as a positive fractional value.
                For positive input values the output should be treated as a positive integer value.
                This function does not output negative values.

*****************************************************************************/
LNK_SECTION_CODE_L1
FIXP_DBL CalcInvLdData(FIXP_DBL op)
{
  FIXP_DBL result_m;

  if ( op == FL2FXCONST_DBL(0.0f) ) {
    result_m = (FIXP_DBL)MAXVAL_DBL;
  }
  else if ( op < FL2FXCONST_DBL(0.0f) ) {
    result_m = f2Pow(op, LD_DATA_SHIFT);
  }
  else {
    int result_e;

    result_m = f2Pow(op, LD_DATA_SHIFT, &result_e);
    result_e = fixMin(fixMax(result_e+1-(DFRACT_BITS-1), -(DFRACT_BITS-1)), (DFRACT_BITS-1)); /* rounding and saturation */

    if ( (result_e>0) && ( result_m > (((FIXP_DBL)MAXVAL_DBL)>>result_e) ) ) {
      result_m = (FIXP_DBL)MAXVAL_DBL; /* saturate to max representable value */
    }
    else {
      result_m = (scaleValue(result_m, result_e)+(FIXP_DBL)1)>>1; /* descale result + rounding */
    }
  }
  return result_m;
}





/*****************************************************************************
    functionname: InitLdInt and CalcLdInt
    description:  Create and access table with integer LdData (0 to 193)
*****************************************************************************/


    LNK_SECTION_CONSTDATA_L1
    static const FIXP_DBL ldIntCoeff[] = {
      0x80000001, 0x00000000, 0x02000000, 0x032b8034, 0x04000000, 0x04a4d3c2, 0x052b8034, 0x059d5da0,
      0x06000000, 0x06570069, 0x06a4d3c2, 0x06eb3a9f, 0x072b8034, 0x0766a009, 0x079d5da0, 0x07d053f7,
      0x08000000, 0x082cc7ee, 0x08570069, 0x087ef05b, 0x08a4d3c2, 0x08c8ddd4, 0x08eb3a9f, 0x090c1050,
      0x092b8034, 0x0949a785, 0x0966a009, 0x0982809d, 0x099d5da0, 0x09b74949, 0x09d053f7, 0x09e88c6b,
      0x0a000000, 0x0a16bad3, 0x0a2cc7ee, 0x0a423162, 0x0a570069, 0x0a6b3d79, 0x0a7ef05b, 0x0a92203d,
      0x0aa4d3c2, 0x0ab7110e, 0x0ac8ddd4, 0x0ada3f60, 0x0aeb3a9f, 0x0afbd42b, 0x0b0c1050, 0x0b1bf312,
      0x0b2b8034, 0x0b3abb40, 0x0b49a785, 0x0b584822, 0x0b66a009, 0x0b74b1fd, 0x0b82809d, 0x0b900e61,
      0x0b9d5da0, 0x0baa708f, 0x0bb74949, 0x0bc3e9ca, 0x0bd053f7, 0x0bdc899b, 0x0be88c6b, 0x0bf45e09,
      0x0c000000, 0x0c0b73cb, 0x0c16bad3, 0x0c21d671, 0x0c2cc7ee, 0x0c379085, 0x0c423162, 0x0c4caba8,
      0x0c570069, 0x0c6130af, 0x0c6b3d79, 0x0c7527b9, 0x0c7ef05b, 0x0c88983f, 0x0c92203d, 0x0c9b8926,
      0x0ca4d3c2, 0x0cae00d2, 0x0cb7110e, 0x0cc0052b, 0x0cc8ddd4, 0x0cd19bb0, 0x0cda3f60, 0x0ce2c97d,
      0x0ceb3a9f, 0x0cf39355, 0x0cfbd42b, 0x0d03fda9, 0x0d0c1050, 0x0d140ca0, 0x0d1bf312, 0x0d23c41d,
      0x0d2b8034, 0x0d3327c7, 0x0d3abb40, 0x0d423b08, 0x0d49a785, 0x0d510118, 0x0d584822, 0x0d5f7cff,
      0x0d66a009, 0x0d6db197, 0x0d74b1fd, 0x0d7ba190, 0x0d82809d, 0x0d894f75, 0x0d900e61, 0x0d96bdad,
      0x0d9d5da0, 0x0da3ee7f, 0x0daa708f, 0x0db0e412, 0x0db74949, 0x0dbda072, 0x0dc3e9ca, 0x0dca258e,
      0x0dd053f7, 0x0dd6753e, 0x0ddc899b, 0x0de29143, 0x0de88c6b, 0x0dee7b47, 0x0df45e09, 0x0dfa34e1,
      0x0e000000, 0x0e05bf94, 0x0e0b73cb, 0x0e111cd2, 0x0e16bad3, 0x0e1c4dfb, 0x0e21d671, 0x0e275460,
      0x0e2cc7ee, 0x0e323143, 0x0e379085, 0x0e3ce5d8, 0x0e423162, 0x0e477346, 0x0e4caba8, 0x0e51daa8,
      0x0e570069, 0x0e5c1d0b, 0x0e6130af, 0x0e663b74, 0x0e6b3d79, 0x0e7036db, 0x0e7527b9, 0x0e7a1030,
      0x0e7ef05b, 0x0e83c857, 0x0e88983f, 0x0e8d602e, 0x0e92203d, 0x0e96d888, 0x0e9b8926, 0x0ea03232,
      0x0ea4d3c2, 0x0ea96df0, 0x0eae00d2, 0x0eb28c7f, 0x0eb7110e, 0x0ebb8e96, 0x0ec0052b, 0x0ec474e4,
      0x0ec8ddd4, 0x0ecd4012, 0x0ed19bb0, 0x0ed5f0c4, 0x0eda3f60, 0x0ede8797, 0x0ee2c97d, 0x0ee70525,
      0x0eeb3a9f, 0x0eef69ff, 0x0ef39355, 0x0ef7b6b4, 0x0efbd42b, 0x0effebcd, 0x0f03fda9, 0x0f0809cf,
      0x0f0c1050, 0x0f10113b, 0x0f140ca0, 0x0f18028d, 0x0f1bf312, 0x0f1fde3d, 0x0f23c41d, 0x0f27a4c0,
      0x0f2b8034
    };


  LNK_SECTION_INITCODE
  void InitLdInt()
  {
    /* nothing to do! Use preinitialized logarithm table */
  }



LNK_SECTION_CODE_L1
FIXP_DBL CalcLdInt(INT i)
{
  /* calculates ld(op)/LD_DATA_SCALING */
  /* op is assumed to be an integer value between 1 and 193 */

  FDK_ASSERT((193>0) && ((FIXP_DBL)ldIntCoeff[0]==(FIXP_DBL)0x80000001)); /* tab has to be initialized */

  if ((i>0)&&(i<193))
    return ldIntCoeff[i];
  else
  {
    return (0);
  }
}


/*****************************************************************************

    functionname: invSqrtNorm2
    description:  delivers 1/sqrt(op) normalized to .5...1 and the shift value of the OUTPUT

*****************************************************************************/
#define SQRT_BITS           7
#define SQRT_VALUES       128
#define SQRT_BITS_MASK   0x7f

LNK_SECTION_CONSTDATA_L1
static const FIXP_DBL invSqrtTab[SQRT_VALUES] = {
  0x5a827999, 0x5a287e03, 0x59cf8cbb, 0x5977a0ab, 0x5920b4de, 0x58cac480, 0x5875cade, 0x5821c364,
  0x57cea99c, 0x577c792f, 0x572b2ddf, 0x56dac38d, 0x568b3631, 0x563c81df, 0x55eea2c3, 0x55a19521,
  0x55555555, 0x5509dfd0, 0x54bf311a, 0x547545d0, 0x542c1aa3, 0x53e3ac5a, 0x539bf7cc, 0x5354f9e6,
  0x530eafa4, 0x52c91617, 0x52842a5e, 0x523fe9ab, 0x51fc513f, 0x51b95e6b, 0x51770e8e, 0x51355f19,
  0x50f44d88, 0x50b3d768, 0x5073fa4f, 0x5034b3e6, 0x4ff601df, 0x4fb7e1f9, 0x4f7a5201, 0x4f3d4fce,
  0x4f00d943, 0x4ec4ec4e, 0x4e8986e9, 0x4e4ea718, 0x4e144ae8, 0x4dda7072, 0x4da115d9, 0x4d683948,
  0x4d2fd8f4, 0x4cf7f31b, 0x4cc08604, 0x4c898fff, 0x4c530f64, 0x4c1d0293, 0x4be767f5, 0x4bb23df9,
  0x4b7d8317, 0x4b4935ce, 0x4b1554a6, 0x4ae1de2a, 0x4aaed0f0, 0x4a7c2b92, 0x4a49ecb3, 0x4a1812fa,
  0x49e69d16, 0x49b589bb, 0x4984d7a4, 0x49548591, 0x49249249, 0x48f4fc96, 0x48c5c34a, 0x4896e53c,
  0x48686147, 0x483a364c, 0x480c6331, 0x47dee6e0, 0x47b1c049, 0x4784ee5f, 0x4758701c, 0x472c447c,
  0x47006a80, 0x46d4e130, 0x46a9a793, 0x467ebcb9, 0x46541fb3, 0x4629cf98, 0x45ffcb80, 0x45d61289,
  0x45aca3d5, 0x45837e88, 0x455aa1ca, 0x45320cc8, 0x4509beb0, 0x44e1b6b4, 0x44b9f40b, 0x449275ec,
  0x446b3b95, 0x44444444, 0x441d8f3b, 0x43f71bbe, 0x43d0e917, 0x43aaf68e, 0x43854373, 0x435fcf14,
  0x433a98c5, 0x43159fdb, 0x42f0e3ae, 0x42cc6397, 0x42a81ef5, 0x42841527, 0x4260458d, 0x423caf8c,
  0x4219528b, 0x41f62df1, 0x41d3412a, 0x41b08ba1, 0x418e0cc7, 0x416bc40d, 0x4149b0e4, 0x4127d2c3,
  0x41062920, 0x40e4b374, 0x40c3713a, 0x40a261ef, 0x40818511, 0x4060da21, 0x404060a1, 0x40201814
};

LNK_SECTION_INITCODE
void InitInvSqrtTab()
{
  /* nothing to do !
     use preinitialized square root table
  */
}



#if !defined(FUNCTION_invSqrtNorm2)
/*****************************************************************************
  delivers 1/sqrt(op) normalized to .5...1 and the shift value of the OUTPUT,
  i.e. the denormalized result is 1/sqrt(op) = invSqrtNorm(op) * 2^(shift)
  uses Newton-iteration for approximation
      Q(n+1) = Q(n) + Q(n) * (0.5 - 2 * V * Q(n)^2)
      with Q = 0.5* V ^-0.5; 0.5 <= V < 1.0
*****************************************************************************/
FIXP_DBL invSqrtNorm2(FIXP_DBL op, INT *shift)
{

  FIXP_DBL val = op ;
  FIXP_DBL reg1, reg2, regtmp ;

  if (val == FL2FXCONST_DBL(0.0)) {
    *shift = 1 ;
    return((LONG)1);  /* minimum positive value */
  }


  /* normalize input, calculate shift value */
  FDK_ASSERT(val > FL2FXCONST_DBL(0.0));
  *shift = fNormz(val) - 1;  /* CountLeadingBits() is not necessary here since test value is always > 0 */
  val <<=*shift ;  /* normalized input V */
  *shift+=2 ;      /* bias for exponent */

  /* Newton iteration of 1/sqrt(V) */
  reg1 = invSqrtTab[ (INT)(val>>(DFRACT_BITS-1-(SQRT_BITS+1))) & SQRT_BITS_MASK ];
  reg2 = FL2FXCONST_DBL(0.0625f);   /* 0.5 >> 3 */

  regtmp= fPow2Div2(reg1);              /* a = Q^2 */
  regtmp= reg2 - fMultDiv2(regtmp, val);      /* b = 0.5 - 2 * V * Q^2 */
  reg1 += (fMultDiv2(regtmp, reg1)<<4);       /* Q = Q + Q*b */

  /* calculate the output exponent = input exp/2 */
  if (*shift & 0x00000001) { /* odd shift values ? */
    reg2 = FL2FXCONST_DBL(0.707106781186547524400844362104849f); /* 1/sqrt(2); */
    reg1 = fMultDiv2(reg1, reg2) << 2;
  }

  *shift = *shift>>1;

  return(reg1);
}
#endif /* !defined(FUNCTION_invSqrtNorm2) */

/*****************************************************************************

    functionname: sqrtFixp
    description:  delivers sqrt(op)

*****************************************************************************/
FIXP_DBL sqrtFixp(FIXP_DBL op)
{
  INT tmp_exp = 0;
  FIXP_DBL tmp_inv = invSqrtNorm2(op, &tmp_exp);

  FDK_ASSERT(tmp_exp > 0) ;
  return( (FIXP_DBL) ( fMultDiv2( (op<<(tmp_exp-1)), tmp_inv ) << 2 ));
}


#if !defined(FUNCTION_schur_div)
/*****************************************************************************

    functionname: schur_div
    description:  delivers op1/op2 with op3-bit accuracy

*****************************************************************************/


FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT count)
{
    INT L_num   = (LONG)num>>1;
    INT L_denum = (LONG)denum>>1;
    INT div     = 0;
    INT k       = count;

    FDK_ASSERT (num>=(FIXP_DBL)0);
    FDK_ASSERT (denum>(FIXP_DBL)0);
    FDK_ASSERT (num <= denum);

    if (L_num != 0)
        while (--k)
        {
            div   <<= 1;
            L_num <<= 1;
            if (L_num >= L_denum)
            {
                L_num -= L_denum;
                div++;
            }
        }
    return (FIXP_DBL)(div << (DFRACT_BITS - count));
}


#endif /* !defined(FUNCTION_schur_div) */


#ifndef FUNCTION_fMultNorm
FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2, INT *result_e)
{
    INT    product = 0;
    INT    norm_f1, norm_f2;

    if (  (f1 == (FIXP_DBL)0) || (f2 == (FIXP_DBL)0) ) {
        *result_e = 0;
        return (FIXP_DBL)0;
    }
    norm_f1 = CountLeadingBits(f1);
    f1 = f1 << norm_f1;
    norm_f2 = CountLeadingBits(f2);
    f2 = f2 << norm_f2;

    product = fMult(f1, f2);
    *result_e  = - (norm_f1 + norm_f2);

    return (FIXP_DBL)product;
}
#endif

#ifndef FUNCTION_fDivNorm
FIXP_DBL fDivNorm(FIXP_DBL L_num, FIXP_DBL L_denum, INT *result_e)
{
    FIXP_DBL div;
    INT norm_num, norm_den;

    FDK_ASSERT (L_num   >= (FIXP_DBL)0);
    FDK_ASSERT (L_denum >  (FIXP_DBL)0);

    if(L_num == (FIXP_DBL)0)
    {
        *result_e = 0;
        return ((FIXP_DBL)0);
    }

    norm_num = CountLeadingBits(L_num);
    L_num = L_num << norm_num;
    L_num = L_num >> 1;
    *result_e = - norm_num + 1;

    norm_den = CountLeadingBits(L_denum);
    L_denum = L_denum << norm_den;
    *result_e -= - norm_den;

    div = schur_div(L_num, L_denum, FRACT_BITS);

    return div;
}
#endif /* !FUNCTION_fDivNorm */

#ifndef FUNCTION_fDivNorm
FIXP_DBL fDivNorm(FIXP_DBL num, FIXP_DBL denom)
{
    INT e;
    FIXP_DBL res;

    FDK_ASSERT (denom >= num);

    res = fDivNorm(num, denom, &e);

    /* Avoid overflow since we must output a value with exponent 0
       there is no other choice than saturating to almost 1.0f */
    if(res == (FIXP_DBL)(1<<(DFRACT_BITS-2)) && e == 1)
    {
        res = (FIXP_DBL)MAXVAL_DBL;
    }
    else
    {
        res = scaleValue(res, e);
    }

    return res;
}
#endif /* !FUNCTION_fDivNorm */

#ifndef FUNCTION_fDivNormHighPrec
FIXP_DBL fDivNormHighPrec(FIXP_DBL num, FIXP_DBL denom, INT *result_e)
{
    FIXP_DBL div;
    INT norm_num, norm_den;

    FDK_ASSERT (num   >= (FIXP_DBL)0);
    FDK_ASSERT (denom >  (FIXP_DBL)0);

    if(num == (FIXP_DBL)0)
    {
        *result_e = 0;
        return ((FIXP_DBL)0);
    }

    norm_num = CountLeadingBits(num);
    num = num << norm_num;
    num = num >> 1;
    *result_e = - norm_num + 1;

    norm_den = CountLeadingBits(denom);
    denom = denom << norm_den;
    *result_e -=  - norm_den;

    div = schur_div(num, denom, 31);
    return div;
}
#endif /* !FUNCTION_fDivNormHighPrec */



FIXP_DBL CalcLog2(FIXP_DBL base_m, INT base_e, INT *result_e)
{
  return fLog2(base_m, base_e, result_e);
}

FIXP_DBL f2Pow(
        const FIXP_DBL exp_m, const INT exp_e,
        INT *result_e
        )
{
  FIXP_DBL frac_part, result_m;
  INT int_part;

  if (exp_e > 0)
  {
      INT exp_bits = DFRACT_BITS-1 - exp_e;
      int_part = exp_m >> exp_bits;
      frac_part = exp_m - (FIXP_DBL)(int_part << exp_bits);
      frac_part = frac_part << exp_e;
  }
  else
  {
      int_part = 0;
      frac_part = exp_m >> -exp_e;
  }

  /* Best accuracy is around 0, so try to get there with the fractional part. */
  if( frac_part > FL2FXCONST_DBL(0.5f) )
  {
      int_part = int_part + 1;
      frac_part = frac_part + FL2FXCONST_DBL(-1.0f);
  }
  if( frac_part < FL2FXCONST_DBL(-0.5f) )
  {
      int_part = int_part - 1;
      frac_part = -(FL2FXCONST_DBL(-1.0f) - frac_part);
  }

  /* Evaluate taylor polynomial which approximates 2^x */
  {
    FIXP_DBL p;

    /* result_m ~= 2^frac_part */
    p = frac_part;
    /* First taylor series coefficient a_0 = 1.0, scaled by 0.5 due to fMultDiv2(). */
    result_m = FL2FXCONST_DBL(1.0f/2.0f);
    for (INT i = 0; i < POW2_PRECISION; i++) {
      /* next taylor series term: a_i * x^i, x=0 */
      result_m = fMultAddDiv2(result_m, pow2Coeff[i], p);
      p  = fMult(p, frac_part);
    }
  }

  /* "+ 1" compensates fMultAddDiv2() of the polynomial evaluation above. */
  *result_e = int_part + 1;

  return result_m;
}

FIXP_DBL f2Pow(
        const FIXP_DBL exp_m, const INT exp_e
        )
{
  FIXP_DBL result_m;
  INT result_e;

  result_m = f2Pow(exp_m, exp_e, &result_e);
  result_e = fixMin(DFRACT_BITS-1,fixMax(-(DFRACT_BITS-1),result_e));

  return scaleValue(result_m, result_e);
}

FIXP_DBL fPow(
        FIXP_DBL base_m, INT base_e,
        FIXP_DBL exp_m, INT exp_e,
        INT *result_e
        )
{
    INT ans_lg2_e, baselg2_e;
    FIXP_DBL base_lg2, ans_lg2, result;

    /* Calc log2 of base */
    base_lg2 = fLog2(base_m, base_e, &baselg2_e);

    /* Prepare exp */
    {
      INT leadingBits;

      leadingBits = CountLeadingBits(fAbs(exp_m));
      exp_m = exp_m << leadingBits;
      exp_e -= leadingBits;
    }

    /* Calc base pow exp */
    ans_lg2 = fMult(base_lg2, exp_m);
    ans_lg2_e = exp_e + baselg2_e;

    /* Calc antilog */
    result = f2Pow(ans_lg2, ans_lg2_e, result_e);

    return result;
}

FIXP_DBL fLdPow(
        FIXP_DBL baseLd_m,
        INT baseLd_e,
        FIXP_DBL exp_m, INT exp_e,
        INT *result_e
        )
{
    INT ans_lg2_e;
    FIXP_DBL ans_lg2, result;

    /* Prepare exp */
    {
      INT leadingBits;

      leadingBits = CountLeadingBits(fAbs(exp_m));
      exp_m = exp_m << leadingBits;
      exp_e -= leadingBits;
    }

    /* Calc base pow exp */
    ans_lg2 = fMult(baseLd_m, exp_m);
    ans_lg2_e = exp_e + baseLd_e;

    /* Calc antilog */
    result = f2Pow(ans_lg2, ans_lg2_e, result_e);

    return result;
}

FIXP_DBL fLdPow(
        FIXP_DBL baseLd_m, INT baseLd_e,
        FIXP_DBL exp_m, INT exp_e
        )
{
  FIXP_DBL result_m;
  int result_e;

  result_m = fLdPow(baseLd_m, baseLd_e, exp_m, exp_e, &result_e);

  return SATURATE_SHIFT(result_m, -result_e, DFRACT_BITS);
}

FIXP_DBL fPowInt(
        FIXP_DBL base_m, INT base_e,
        INT exp,
        INT *pResult_e
        )
{
  FIXP_DBL result;

  if (exp != 0) {
    INT result_e = 0;

    if (base_m != (FIXP_DBL)0) {
      {
        INT leadingBits;
        leadingBits = CountLeadingBits( base_m );
        base_m <<= leadingBits;
        base_e -= leadingBits;
      }

      result = base_m;

      {
        int i;
        for (i = 1; i < fAbs(exp); i++) {
          result = fMult(result, base_m);
        }
      }

      if (exp < 0) {
        /* 1.0 / ans */
        result = fDivNorm( FL2FXCONST_DBL(0.5f), result, &result_e );
        result_e++;
      } else {
        int ansScale = CountLeadingBits( result );
        result <<= ansScale;
        result_e -= ansScale;
      }

      result_e += exp * base_e;

    } else {
      result = (FIXP_DBL)0;
    }
    *pResult_e = result_e;
  }
  else {
    result =  FL2FXCONST_DBL(0.5f);
    *pResult_e = 1;
  }

  return result;
}

FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e, INT *result_e)
{
  FIXP_DBL result_m;

  /* Short cut for zero and negative numbers. */
  if ( x_m <= FL2FXCONST_DBL(0.0f) ) {
    *result_e = DFRACT_BITS-1;
    return FL2FXCONST_DBL(-1.0f);
  }

  /* Calculate log2() */
  {
    FIXP_DBL px2_m, x2_m;

    /* Move input value x_m * 2^x_e toward 1.0, where the taylor approximation
       of the function log(1-x) centered at 0 is most accurate. */
    {
      INT b_norm;

      b_norm = fNormz(x_m)-1;
      x2_m = x_m << b_norm;
      x_e = x_e - b_norm;
    }

    /* map x from log(x) domain to log(1-x) domain. */
    x2_m = - (x2_m + FL2FXCONST_DBL(-1.0) );

    /* Taylor polinomial approximation of ln(1-x) */
    result_m  = FL2FXCONST_DBL(0.0);
    px2_m = x2_m;
    for (int i=0; i<LD_PRECISION; i++) {
      result_m = fMultAddDiv2(result_m, ldCoeff[i], px2_m);
      px2_m = fMult(px2_m, x2_m);
    }
    /* Multiply result with 1/ln(2) = 1.0 + 0.442695040888 (get log2(x) from ln(x) result). */
    result_m = fMultAddDiv2(result_m, result_m, FL2FXCONST_DBL(2.0*0.4426950408889634073599246810019));

    /* Add exponent part. log2(x_m * 2^x_e) = log2(x_m) + x_e */
    if (x_e != 0)
    {
      int enorm;

      enorm = DFRACT_BITS - fNorm((FIXP_DBL)x_e);
      /* The -1 in the right shift of result_m compensates the fMultDiv2() above in the taylor polinomial evaluation loop.*/
      result_m = (result_m >> (enorm-1)) + ((FIXP_DBL)x_e << (DFRACT_BITS-1-enorm));

      *result_e = enorm;
    } else {
      /* 1 compensates the fMultDiv2() above in the taylor polinomial evaluation loop.*/
      *result_e = 1;
    }
  }

  return result_m;
}

FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e)
{
  if ( x_m <= FL2FXCONST_DBL(0.0f) ) {
    x_m = FL2FXCONST_DBL(-1.0f);
  }
  else {
    INT result_e;
    x_m = fLog2(x_m, x_e, &result_e);
    x_m = scaleValue(x_m, result_e-LD_DATA_SHIFT);
  }
  return  x_m;
}




#if TEST_ROUNDING
#include <math.h>

void writeToFile( FDKFILE *fh, float v) {
    FDKfprintf(fh, "%22.16f\n", v );
}
FDKFILE* openAppend(CHAR* filNam)
{
    FDKFILE* fh = NULL;
    fh = FDKfopen(filNam, "a");
    if (!fh) {
      FDKprintf("\nError at fio_open\n");
      return NULL;
    }
    return fh;
}

// loop version, long duration, huge output data
void checkRound()
{
    #define  IN_INT     0   // all four rounding modes are bitexact for 0 and for 1

    float    inp;
    FIXP_DBL f_inp;
    float    r,  rnd;
    FIXP_DBL f_trc,f_rnd;
    float    step;

    //step=0.1f;
    step=0.001f;
    //step=0.0001f;
    //step=0.00001f;
    //step=0.0000001f;        // BEWARE output data of test might get huge!
    //step=0.00000000005f;    // BEWARE output data of test might get huge!

    double   d_floor,d_ceil;
    FIXP_DBL f_floor,f_ceil;
    INT      i,j,floorInt,ceilInt,roundInt,truncInt;

    FDKFILE *fpF_a = NULL;  FDKFILE *fpC_a = NULL;
    FDKFILE *fpF_b = NULL;  FDKFILE *fpC_b = NULL;
    FDKFILE *fpF_c = NULL;  FDKFILE *fpC_c = NULL;
    FDKFILE *fpF_d = NULL;  FDKFILE *fpC_d = NULL;
    FDKFILE *fpF_e = NULL;  FDKFILE *fpC_e = NULL;

    fpF_a = openAppend("_FLT_a.txt");  fpC_a = openAppend("_FDK_a.txt");
    fpF_b = openAppend("_FLT_b.txt");  fpC_b = openAppend("_FDK_b.txt");
    fpF_c = openAppend("_FLT_c.txt");  fpC_c = openAppend("_FDK_c.txt");
    fpF_d = openAppend("_FLT_d.txt");  fpC_d = openAppend("_FDK_d.txt");
    fpF_e = openAppend("_FLT_e.txt");  fpC_e = openAppend("_FDK_e.txt");



    #define  INPUT_SF       3  // BEWARE at SF 0 !!!  over/under-flow
    #define  INPUT_SCALE    (float)(1<<INPUT_SF)

    for (inp = -3.1f;        inp < 2.1f;        inp=inp+step)
  //for (inp = -0.9f;  inp < 0.9f;  inp=inp+step)
  //for (inp = -0.0000000001f;  inp < 0.0000000001f;  inp=inp+step)
  //for (inp = -3.1f;        inp < 2.1f;        inp=inp+step)
    {  //       #                   #
       // --- write input
                                                         writeToFile(fpF_a,(float)  inp);
       f_inp = (FIXP_DBL)(inp / INPUT_SCALE);            writeToFile(fpC_a,(float)f_inp    * (float)FDKpow(2,INPUT_SF));


       // --- floor
       d_floor   = FDKfloor(inp);                        writeToFile(fpF_b,(float)  d_floor);
       // --- floor fixedpoint
       floorInt = fixp_floorToInt(f_inp,INPUT_SF);
       f_floor  = fixp_floor     (f_inp,INPUT_SF);
      #if IN_INT
                                                         writeToFile(fpC_b,(float)  floorInt);
      #else
                                                         writeToFile(fpC_b,(float)  f_floor * (float)FDKpow(2,INPUT_SF));
      #endif


       // --- ceil
       d_ceil  = FDKceil(inp);                           writeToFile(fpF_c,(float)  d_ceil );
       // --- ceil fixedpoint
       ceilInt = fixp_ceilToInt(f_inp,INPUT_SF);
       f_ceil  = fixp_ceil     (f_inp,INPUT_SF);
      #if IN_INT
                                                         writeToFile(fpC_c,(float)  ceilInt);
      #else
                                                         writeToFile(fpC_c,(float)  f_ceil  * (float)FDKpow(2,INPUT_SF));
      #endif


       // --- truncate
       i = (INT)inp;                                     writeToFile(fpF_d,(float)  i);
       // --- truncate fixedpoint
       truncInt = fixp_truncateToInt(f_inp,INPUT_SF);
       f_trc    = fixp_truncate     (f_inp,INPUT_SF);
      #if IN_INT
                                                         writeToFile(fpC_d,(float)  truncInt);
      #else
                                                         writeToFile(fpC_d,(float)  f_trc   * (float)FDKpow(2,INPUT_SF));
      #endif


       // --- round
       r = 0.5f;
       if (inp > 0)  rnd =    inp + r;
       if (inp < 0)  rnd = -(-inp + r);  // avoid offset; you might get offset with 'rnd = inp - r'
       j = (INT)(rnd);                                   writeToFile(fpF_e,(float)  j);
       // --- round fixedpoint
       roundInt = fixp_roundToInt(f_inp,INPUT_SF);
       f_rnd    = fixp_round     (f_inp,INPUT_SF);
      #if IN_INT
                                                         writeToFile(fpC_e,(float)  roundInt);
      #else
                                                         writeToFile(fpC_e,(float)  f_rnd   * (float)FDKpow(2,INPUT_SF));
      #endif
    }

    if (fpF_a) FDKfclose(fpF_a);  if (fpC_a) FDKfclose(fpC_a);
    if (fpF_b) FDKfclose(fpF_b);  if (fpC_b) FDKfclose(fpC_b);
    if (fpF_c) FDKfclose(fpF_c);  if (fpC_c) FDKfclose(fpC_c);
    if (fpF_d) FDKfclose(fpF_d);  if (fpC_d) FDKfclose(fpC_d);
    if (fpF_e) FDKfclose(fpF_e);  if (fpC_e) FDKfclose(fpC_e);
}


// round only a few selected values (faster)
void checkRound2()
{
  // set point
  #define BLOD               24  // left  bits (of dot): number of bits _left_  of decimal point  ==>  Q 24.8 format (incl. sign bit)
  #define BROD               8   // right bits (of dot): number of bits _right_ of decimal point  ==>  Q 24.8 format
  FDK_ASSERT((BROD+BLOD)==DFRACT_BITS);

  // scale factors
  #define FL_SF              BLOD
  #define FL_SCALE                  (1<<FL_SF)

  #define FR_SF              BROD
  #define FR_SCALE                  (1<<FR_SF)

  #define INL_SF             7                      // bits at INput Left of dot
  #define INL_SCALE          (float)(1<<INL_SF)

  #define INR_SF             (DFRACT_BITS-1-INL_SF) // bits at INput Right of dot    32-1-7 = 24
  #define INR_SCALE          (float)(1<<INR_SF)


  // testdata
  #define X_MIN                          -128.0000f   //
  #define X0                             -127.0000f   //

  #define X1                               -5.0000f   //
//#define X1                               -4.4999f   // round
//#define X1                                4.4999f   // round

  #define X2                               -4.9999f   //
  #define X3                               -4.5000f   //
  #define X4                               -0.1234f   //
  #define X_NULL                            0.0f      //
  #define X5                                0.1234f   //
  #define X6                                4.5000f   //
  #define X7                                4.9999f   //
  #define X8                                5.0000f   //
  // subtract one LSB from 128.0f [this is needed AFTER hex values have been dumped --> this is needed to get a valid float reference for floor and trunc ]
  #define X_MAX ((-0.0000000004656613f) + 128.0000f)


  FIXP_DBL f_reg0, f_reg1, f_reg2, f_reg3, f_reg4, f_reg5, f_reg6, f_reg7, f_reg8, f_reg_min, f_reg_max, f_reg_null;
  INT        res0,   res1,   res2,   res3,   res4,   res5,   res6,   res7,   res8;
  FIXP_DBL f_res0, f_res1, f_res2, f_res3, f_res4, f_res5, f_res6, f_res7, f_res8;

  f_reg_min  = (LONG)0x80000000 ; // data taken from above dump; cast to LONG needed because of
  f_reg0     = (LONG)0x81000000 ; // fract-class needs a sign;  0x######## is of type unsigned int.
  f_reg1     = (LONG)0xfb000000 ;
  f_reg2     = (LONG)0xfb000690 ;
  f_reg3     = (LONG)0xfb800000 ;
  f_reg4     = (LONG)0xffe068dc ;
  f_reg_null = (LONG)0x00000000 ;
  f_reg5     = (LONG)0x001f9724 ;
  f_reg6     = (LONG)0x04800000 ;
  f_reg7     = (LONG)0x04fff970 ;
  f_reg8     = (LONG)0x05000000 ;
  f_reg_max  = (LONG)0x7fffffff ;


  FDKprintf("---- input values ----\n");
  FDKprintf("%f %f %f %f %f %f %f %f %f\n", X0
                                          , X1
                                          , X2
                                          , X3
                                          , X4
                                          , X5
                                          , X6
                                          , X7
                                          , X8
           );
  FDKprintf("%f %f %f %f %f %f %f %f %f\n", (float)f_reg0     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg1     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg2     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg3     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg4     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg5     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg6     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg7     * (float)FDKpow(2,INL_SF)
                                          , (float)f_reg8     * (float)FDKpow(2,INL_SF)
           );
  FDKprintf("---- min/max input values ----\n");
  FDKprintf("%f %f %f\n", X_MIN
                        , X_NULL
                        , X_MAX
           );
  FDKprintf("%f %f %f\n", (float)f_reg_min  * (float)FDKpow(2,INL_SF)
                        , (float)f_reg_null * (float)FDKpow(2,INL_SF)
                        , (float)f_reg_max  * (float)FDKpow(2,INL_SF)
           );
  FDKprintf("\n");

  FDKprintf("\n---- floor ----\n");
  res0 = fixp_floorToInt(f_reg0,     INL_SF);    f_res0 = fixp_floor(f_reg0,     INL_SF);
  res1 = fixp_floorToInt(f_reg1,     INL_SF);    f_res1 = fixp_floor(f_reg1,     INL_SF);
  res2 = fixp_floorToInt(f_reg2,     INL_SF);    f_res2 = fixp_floor(f_reg2,     INL_SF);
  res3 = fixp_floorToInt(f_reg3,     INL_SF);    f_res3 = fixp_floor(f_reg3,     INL_SF);
  res4 = fixp_floorToInt(f_reg4,     INL_SF);    f_res4 = fixp_floor(f_reg4,     INL_SF);
  res5 = fixp_floorToInt(f_reg5,     INL_SF);    f_res5 = fixp_floor(f_reg5,     INL_SF);
  res6 = fixp_floorToInt(f_reg6,     INL_SF);    f_res6 = fixp_floor(f_reg6,     INL_SF);
  res7 = fixp_floorToInt(f_reg7,     INL_SF);    f_res7 = fixp_floor(f_reg7,     INL_SF);
  res8 = fixp_floorToInt(f_reg8,     INL_SF);    f_res8 = fixp_floor(f_reg8,     INL_SF);
  FDKprintf("reference          %i %i %i %i %i %i %i %i %i\n", (int)floor(X0), (int)floor(X1), (int)floor(X2), (int)floor(X3), (int)floor(X4), (int)floor(X5), (int)floor(X6), (int)floor(X7), (int)floor(X8));
  FDKprintf("fixp_floorToInt    %i %i %i %i %i %i %i %i %i\n", res0, res1, res2, res3, res4, res5, res6, res7, res8);
  FDKprintf("fixp_floor         %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f\n", (float)f_res0*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res1*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res2*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res3*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res4*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res5*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res6*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res7*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res8*(float)FDKpow(2,INL_SF));

  FDKprintf("\n---- min/max floor ----\n");
  res1 = fixp_floorToInt(f_reg_min,  INL_SF);    f_res1 = fixp_floor(f_reg_min,  INL_SF);
  res2 = fixp_floorToInt(f_reg_null, INL_SF);    f_res2 = fixp_floor(f_reg_null, INL_SF);
  res3 = fixp_floorToInt(f_reg_max,  INL_SF);    f_res3 = fixp_floor(f_reg_max,  INL_SF);
  FDKprintf("reference          %i %i %i\n", (int)floor(X_MIN), (int)floor(X_NULL), (int)floor(X_MAX));
  FDKprintf("fixp_floorToInt    %i %i %i\n", res1, res2, res3);
  FDKprintf("fixp_floor         %10.7f %10.7f %10.7f\n", (float)f_res1*(float)FDKpow(2,INL_SF),
                                                         (float)f_res2*(float)FDKpow(2,INL_SF),
                                                         (float)f_res3*(float)FDKpow(2,INL_SF));
  FDKprintf("\n\n\n");


  FDKprintf("---- ceil ----\n");
  res0 = fixp_ceilToInt(f_reg0,      INL_SF);    f_res0 = fixp_ceil(f_reg0,      INL_SF);
  res1 = fixp_ceilToInt(f_reg1,      INL_SF);    f_res1 = fixp_ceil(f_reg1,      INL_SF);
  res2 = fixp_ceilToInt(f_reg2,      INL_SF);    f_res2 = fixp_ceil(f_reg2,      INL_SF);
  res3 = fixp_ceilToInt(f_reg3,      INL_SF);    f_res3 = fixp_ceil(f_reg3,      INL_SF);
  res4 = fixp_ceilToInt(f_reg4,      INL_SF);    f_res4 = fixp_ceil(f_reg4,      INL_SF);
  res5 = fixp_ceilToInt(f_reg5,      INL_SF);    f_res5 = fixp_ceil(f_reg5,      INL_SF);
  res6 = fixp_ceilToInt(f_reg6,      INL_SF);    f_res6 = fixp_ceil(f_reg6,      INL_SF);
  res7 = fixp_ceilToInt(f_reg7,      INL_SF);    f_res7 = fixp_ceil(f_reg7,      INL_SF);
  res8 = fixp_ceilToInt(f_reg8,      INL_SF);    f_res8 = fixp_ceil(f_reg8,      INL_SF);
  FDKprintf("reference          %i %i %i %i %i %i %i %i %i\n", (int)ceil(X0), (int)ceil(X1), (int)ceil(X2), (int)ceil(X3), (int)ceil(X4), (int)ceil(X5), (int)ceil(X6), (int)ceil(X7), (int)ceil(X8));
  FDKprintf("fixp_ceilToInt     %i %i %i %i %i %i %i %i %i\n", res0, res1, res2, res3, res4, res5, res6, res7, res8);
  FDKprintf("fixp_ceil          %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f\n", (float)f_res0*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res1*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res2*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res3*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res4*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res5*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res6*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res7*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res8*(float)FDKpow(2,INL_SF));

  FDKprintf("\n---- min/max ceil ----\n");
  res1 = fixp_ceilToInt(f_reg_min,   INL_SF);
  res2 = fixp_ceilToInt(f_reg_null,  INL_SF);
  res3 = fixp_ceilToInt(f_reg_max,   INL_SF);

  f_res1 = fixp_ceil(f_reg_min,   INL_SF);
  f_res2 = fixp_ceil(f_reg_null,  INL_SF);
  f_res3 = fixp_ceil(f_reg_max,   INL_SF);

  FDKprintf("reference          %i %i %i\n", (int)ceil(X_MIN), (int)ceil(X_NULL), (int)ceil(X_MAX));
  FDKprintf("fixp_ceilToInt     %i %i %i\n", res1, res2, res3);
  FDKprintf("fixp_ceil          %10.7f %10.7f %10.7f\n", (float)f_res1*(float)FDKpow(2,INL_SF),
                                                         (float)f_res2*(float)FDKpow(2,INL_SF),
                                                         (float)f_res3*(float)FDKpow(2,INL_SF));
  FDKprintf("\n\n\n");


  FDKprintf("---- trunc ----\n");
  res0 = fixp_truncateToInt(f_reg0,  INL_SF);    f_res0 = fixp_truncate(f_reg0,  INL_SF);
  res1 = fixp_truncateToInt(f_reg1,  INL_SF);    f_res1 = fixp_truncate(f_reg1,  INL_SF);
  res2 = fixp_truncateToInt(f_reg2,  INL_SF);    f_res2 = fixp_truncate(f_reg2,  INL_SF);
  res3 = fixp_truncateToInt(f_reg3,  INL_SF);    f_res3 = fixp_truncate(f_reg3,  INL_SF);
  res4 = fixp_truncateToInt(f_reg4,  INL_SF);    f_res4 = fixp_truncate(f_reg4,  INL_SF);
  res5 = fixp_truncateToInt(f_reg5,  INL_SF);    f_res5 = fixp_truncate(f_reg5,  INL_SF);
  res6 = fixp_truncateToInt(f_reg6,  INL_SF);    f_res6 = fixp_truncate(f_reg6,  INL_SF);
  res7 = fixp_truncateToInt(f_reg7,  INL_SF);    f_res7 = fixp_truncate(f_reg7,  INL_SF);
  res8 = fixp_truncateToInt(f_reg8,  INL_SF);    f_res8 = fixp_truncate(f_reg8,  INL_SF);
  FDKprintf("reference          %i %i %i %i %i %i %i %i %i\n", (int)(X0), (int)(X1), (int)(X2), (int)(X3), (int)(X4), (int)(X5), (int)(X6), (int)(X7), (int)(X8));
  FDKprintf("fixp_truncateToInt %i %i %i %i %i %i %i %i %i\n", res0, res1, res2, res3, res4, res5, res6, res7, res8);
  FDKprintf("fixp_truncate      %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f\n", (float)f_res0*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res1*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res2*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res3*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res4*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res5*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res6*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res7*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res8*(float)FDKpow(2,INL_SF));

  FDKprintf("\n---- min/max trunc ----\n");
  res1 = fixp_truncateToInt(f_reg_min, INL_SF);  f_res1 = fixp_truncate(f_reg_min, INL_SF);
  res2 = fixp_truncateToInt(f_reg_null,INL_SF);  f_res2 = fixp_truncate(f_reg_null,INL_SF);
  res3 = fixp_truncateToInt(f_reg_max, INL_SF);  f_res3 = fixp_truncate(f_reg_max, INL_SF);
  FDKprintf("reference          %i %i %i\n", (int)(X_MIN), (int)(X_NULL), (int)(X_MAX));
  FDKprintf("fixp_truncateToInt %i %i %i\n", res1, res2, res3);
  FDKprintf("fixp_truncate      %10.7f %10.7f %10.7f\n", (float)f_res1*(float)FDKpow(2,INL_SF),
                                                         (float)f_res2*(float)FDKpow(2,INL_SF),
                                                         (float)f_res3*(float)FDKpow(2,INL_SF));
  FDKprintf("\n\n\n");


  FDKprintf("---- round ----\n");
  res0 = fixp_roundToInt(f_reg0,  INL_SF);    f_res0 = fixp_round(f_reg0,  INL_SF);
  res1 = fixp_roundToInt(f_reg1,  INL_SF);    f_res1 = fixp_round(f_reg1,  INL_SF);
  res2 = fixp_roundToInt(f_reg2,  INL_SF);    f_res2 = fixp_round(f_reg2,  INL_SF);
  res3 = fixp_roundToInt(f_reg3,  INL_SF);    f_res3 = fixp_round(f_reg3,  INL_SF);
  res4 = fixp_roundToInt(f_reg4,  INL_SF);    f_res4 = fixp_round(f_reg4,  INL_SF);
  res5 = fixp_roundToInt(f_reg5,  INL_SF);    f_res5 = fixp_round(f_reg5,  INL_SF);
  res6 = fixp_roundToInt(f_reg6,  INL_SF);    f_res6 = fixp_round(f_reg6,  INL_SF);
  res7 = fixp_roundToInt(f_reg7,  INL_SF);    f_res7 = fixp_round(f_reg7,  INL_SF);
  res8 = fixp_roundToInt(f_reg8,  INL_SF);    f_res8 = fixp_round(f_reg8,  INL_SF);
  FDKprintf("reference          %i %i %i %i %i %i %i %i %i\n", roundRef(X0),
                                                               roundRef(X1),
                                                               roundRef(X2),
                                                               roundRef(X3),
                                                               roundRef(X4),
                                                               roundRef(X5),
                                                               roundRef(X6),
                                                               roundRef(X7),
                                                               roundRef(X8));
  FDKprintf("fixp_roundToInt    %i %i %i %i %i %i %i %i %i\n", res0, res1, res2, res3, res4, res5, res6, res7, res8);
  FDKprintf("fixp_round         %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f\n", (float)f_res0*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res1*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res2*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res3*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res4*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res5*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res6*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res7*(float)FDKpow(2,INL_SF),
                                                                                                   (float)f_res8*(float)FDKpow(2,INL_SF));

  FDKprintf("\n---- min/max round ----\n");
  res1 = fixp_roundToInt(f_reg_min, INL_SF);  f_res1 = fixp_round(f_reg_min, INL_SF);
  res2 = fixp_roundToInt(f_reg_null,INL_SF);  f_res2 = fixp_round(f_reg_null,INL_SF);
  res3 = fixp_roundToInt(f_reg_max, INL_SF);  f_res3 = fixp_round(f_reg_max, INL_SF);

  FDKprintf("reference          %i %i %i\n", roundRef(X_MIN),
                                             roundRef(X_NULL),
                                             roundRef(X_MAX));
  FDKprintf("fixp_roundToInt    %i %i %i\n", res1, res2, res3);
  FDKprintf("fixp_round         %10.7f %10.7f %10.7f\n", (float)f_res1*(float)FDKpow(2,INL_SF),
                                                         (float)f_res2*(float)FDKpow(2,INL_SF),
                                                         (float)f_res3*(float)FDKpow(2,INL_SF));
  FDKprintf("\n\n\n");

}
#endif
