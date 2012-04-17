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
   Author(s):   Haricharan Lakshman, Manuel Jander
   Description: Trigonometric functions fixed point fractional implementation.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/



#include "common_fix.h"

#include "FDK_tools_rom.h"

/* Fixed point precision definitions */
#define Q(format)        ((FIXP_DBL)(((LONG)1) << (format)))

#ifndef M_PI
#define M_PI      (3.14159265358979323846f)
#endif

/*!
 * Inverse tangent function.
 */

// --- fixp_atan() ----
#define Q_ATANINP   (25)    // Input in q25, Output in q30
#define Q_ATANOUT   (30)
#define ATI_SF              ((DFRACT_BITS-1)-Q_ATANINP)  // 6
#define ATI_SCALE           ((float)(1<<ATI_SF))         //
#define ATO_SF              ((DFRACT_BITS-1)-Q_ATANOUT)  // 1   ] -pi/2 .. pi/2 [
#define ATO_SCALE           ((float)(1<<ATO_SF))         //
// --- fixp_atan2() ---
#define Q_ATAN2OUT  (29)
#define AT2O_SF             ((DFRACT_BITS-1)-Q_ATAN2OUT) // 2   ] -pi   .. pi   ]
#define AT2O_SCALE          ((float)(1<<AT2O_SF))        //
// --------------------

FIXP_DBL fixp_atan(FIXP_DBL x);
FIXP_DBL fixp_atan2(FIXP_DBL y, FIXP_DBL x);

FIXP_DBL fixp_cos(FIXP_DBL x, int scale);
FIXP_DBL fixp_sin(FIXP_DBL x, int scale);

#define FIXP_COS_SIN

#ifndef CORDIC_SINCOS

#include "FDK_tools_rom.h"

#define SINETAB SineTable512
#define LD 9

#endif

#ifndef FUNCTION_inline_fixp_cos_sin

#define FUNCTION_inline_fixp_cos_sin

/*
 * Calculates coarse lookup index and sign for sine.
 * Returns delta x residual.
 */
static inline FIXP_DBL fixp_sin_cos_residual_inline(FIXP_DBL x, int scale, FIXP_DBL *sine, FIXP_DBL *cosine)
{
    FIXP_DBL residual;
    int s;
    int shift = (31-scale-LD-1);
    int ssign = 1;
    int csign = 1;

    residual = fMult(x, FL2FXCONST_DBL(1.0/M_PI));
    s = ((LONG)residual) >> shift;

    residual &= ( (1<<shift) - 1 );
    residual = fMult(residual, FL2FXCONST_DBL(M_PI/4.0)) << 2;
    residual <<= scale;

    /* Sine sign symmetry */
    if (s & ((1<<LD)<<1) ) {
      ssign = -ssign;
    }
    /* Cosine sign symmetry */
    if ( (s + (1<<LD)) & ((1<<LD)<<1) ) {
      csign = -csign;
    }

    s = fAbs(s);

    s &= (((1<<LD)<<1)-1); /* Modulo PI */

    if (s > (1<<LD)) {
      s = ((1<<LD)<<1) - s;
    }

    {
      LONG sl, cl;
      /* Because of packed table */
      if (s > (1<<(LD-1))) {
        FIXP_STP tmp;
        /* Cosine/Sine simetry for angles greater than PI/4 */
        s = (1<<LD)-s;
        tmp = SINETAB[s];
        sl = (LONG)tmp.v.re;
        cl = (LONG)tmp.v.im;
      } else {
        FIXP_STP tmp;
        tmp = SINETAB[s];
        sl = (LONG)tmp.v.im;
        cl = (LONG)tmp.v.re;
      }

#ifdef SINETABLE_16BIT
      *sine   = (FIXP_DBL)((sl * ssign) << (DFRACT_BITS-FRACT_BITS));
      *cosine = (FIXP_DBL)((cl * csign) << (DFRACT_BITS-FRACT_BITS));
#else
      *sine   = (FIXP_DBL)(sl * ssign);
      *cosine = (FIXP_DBL)(cl * csign);
#endif
    }

    return residual;
}

/**
 * \brief Calculate cosine and sine value each of 2 angles different angle values.
 * \param x1 first angle value
 * \param x2 second angle value
 * \param scale exponent of x1 and x2
 * \param out pointer to 4 FIXP_DBL locations, were the values cos(x1), sin(x1), cos(x2), sin(x2)
 *            will be stored into.
 */
static inline void inline_fixp_cos_sin (FIXP_DBL x1, FIXP_DBL x2, const int scale, FIXP_DBL *out)
{
    FIXP_DBL residual, error0, error1, sine, cosine;
    residual = fixp_sin_cos_residual_inline(x1, scale, &sine, &cosine);
    error0 = fMultDiv2(sine, residual);
    error1 = fMultDiv2(cosine, residual);
    *out++  = cosine - (error0<<1);
    *out++  = sine   + (error1<<1);

    residual = fixp_sin_cos_residual_inline(x2, scale, &sine, &cosine);
    error0 = fMultDiv2(sine, residual);
    error1 = fMultDiv2(cosine, residual);
    *out++  = cosine - (error0<<1);
    *out++  = sine   + (error1<<1);
}
#endif

