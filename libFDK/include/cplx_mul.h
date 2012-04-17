/***************************  Fraunhofer IIS FDK Tools  **********************

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
   Author(s):
   Description: fixed point intrinsics

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "common_fix.h"

#if !defined(__CPLX_Mult_H__)
#define __CPLX_Mult_H__

#if defined(__CC_ARM) || defined(__arm__) || defined(_M_ARM)	/* cppp replaced: elif */
#include "arm/cplx_mul.h"

#elif defined(__GNUC__) && defined(__mips__)	/* cppp replaced: elif */
#include "mips/cplx_mul.h"

#endif /* #if defined all cores: bfin, arm, etc. */

/* ############################################################################# */

/* Fallback generic implementations */

#if !defined(FUNCTION_cplxMultDiv2_32x16X2)
#define FUNCTION_cplxMultDiv2_32x16X2

inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL  a_Re,
                          const FIXP_DBL  a_Im,
                          const FIXP_SGL  b_Re,
                          const FIXP_SGL  b_Im)
{
  *c_Re = fMultDiv2(a_Re,b_Re) - fMultDiv2(a_Im,b_Im);
  *c_Im = fMultDiv2(a_Re,b_Im) + fMultDiv2(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMultDiv2_32x16)
#define FUNCTION_cplxMultDiv2_32x16

inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL a_Re,
                          const FIXP_DBL a_Im,
                          const FIXP_SPK w )
{
  cplxMultDiv2(c_Re, c_Im, a_Re, a_Im, w.v.re, w.v.im);
}
#endif

#if !defined(FUNCTION_cplxMultAddDiv2_32x16X2)
#define FUNCTION_cplxMultAddDiv2_32x16X2

inline void cplxMultAddDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL  a_Re,
                          const FIXP_DBL  a_Im,
                          const FIXP_SGL  b_Re,
                          const FIXP_SGL  b_Im)
{
  *c_Re += fMultDiv2(a_Re,b_Re) - fMultDiv2(a_Im,b_Im);
  *c_Im += fMultDiv2(a_Re,b_Im) + fMultDiv2(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMultSubDiv2_32x16X2)
#define FUNCTION_cplxMultSubDiv2_32x16X2

inline void cplxMultSubDiv2( FIXP_DBL *c_Re,
                             FIXP_DBL *c_Im,
                             const FIXP_DBL  a_Re,
                             const FIXP_DBL  a_Im,
                             const FIXP_SGL  b_Re,
                             const FIXP_SGL  b_Im)
{
  *c_Re -= fMultDiv2(a_Re,b_Re) - fMultDiv2(a_Im,b_Im);
  *c_Im -= fMultDiv2(a_Re,b_Im) + fMultDiv2(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMultDiv2_32x32X2)
#define FUNCTION_cplxMultDiv2_32x32X2

inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL  a_Re,
                          const FIXP_DBL  a_Im,
                          const FIXP_DBL  b_Re,
                          const FIXP_DBL  b_Im)
{
  *c_Re = fMultDiv2(a_Re,b_Re) - fMultDiv2(a_Im,b_Im);
  *c_Im = fMultDiv2(a_Re,b_Im) + fMultDiv2(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMultDiv2_32x32)
#define FUNCTION_cplxMultDiv2_32x32

inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL  a_Re,
                          const FIXP_DBL  a_Im,
                          const FIXP_DPK  w)
{
  cplxMultDiv2(c_Re, c_Im, a_Re, a_Im, w.v.re, w.v.im);
}
#endif

#if !defined(FUNCTION_cplxMultSubDiv2_32x32X2)
#define FUNCTION_cplxMultSubDiv2_32x32X2

inline void cplxMultSubDiv2( FIXP_DBL *c_Re,
                             FIXP_DBL *c_Im,
                             const FIXP_DBL  a_Re,
                             const FIXP_DBL  a_Im,
                             const FIXP_DBL  b_Re,
                             const FIXP_DBL  b_Im)
{
  *c_Re -= fMultDiv2(a_Re,b_Re) - fMultDiv2(a_Im,b_Im);
  *c_Im -= fMultDiv2(a_Re,b_Im) + fMultDiv2(a_Im,b_Re);
}
#endif

/* ############################################################################# */

#if !defined(FUNCTION_cplxMult_32x16X2)
#define FUNCTION_cplxMult_32x16X2

inline void cplxMult( FIXP_DBL *c_Re,
                      FIXP_DBL *c_Im,
                      const FIXP_DBL  a_Re,
                      const FIXP_DBL  a_Im,
                      const FIXP_SGL  b_Re,
                      const FIXP_SGL  b_Im)
{
  *c_Re = fMult(a_Re,b_Re) - fMult(a_Im,b_Im);
  *c_Im = fMult(a_Re,b_Im) + fMult(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMult_32x16)
#define FUNCTION_cplxMult_32x16

inline void cplxMult( FIXP_DBL *c_Re,
                      FIXP_DBL *c_Im,
                      const FIXP_DBL a_Re,
                      const FIXP_DBL a_Im,
                      const FIXP_SPK w )
{
  cplxMult(c_Re, c_Im, a_Re, a_Im, w.v.re, w.v.im);
}
#endif

#if !defined(FUNCTION_cplxMult_32x32X2)
#define FUNCTION_cplxMult_32x32X2

inline void cplxMult( FIXP_DBL *c_Re,
                      FIXP_DBL *c_Im,
                      const FIXP_DBL  a_Re,
                      const FIXP_DBL  a_Im,
                      const FIXP_DBL  b_Re,
                      const FIXP_DBL  b_Im)
{
  *c_Re = fMult(a_Re,b_Re) - fMult(a_Im,b_Im);
  *c_Im = fMult(a_Re,b_Im) + fMult(a_Im,b_Re);
}
#endif

#if !defined(FUNCTION_cplxMult_32x32)
#define FUNCTION_cplxMult_32x32
inline void cplxMult( FIXP_DBL *c_Re,
                      FIXP_DBL *c_Im,
                      const FIXP_DBL  a_Re,
                      const FIXP_DBL  a_Im,
                      const FIXP_DPK  w)
{
  cplxMult(c_Re, c_Im, a_Re, a_Im, w.v.re, w.v.im);
}
#endif

/* ############################################################################# */

#endif /* __CPLX_Mult_H__ */

