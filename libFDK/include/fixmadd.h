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
   Description: fixed point intrinsics

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if !defined(__FIXMADD_H__)
#define __FIXMADD_H__

#include "FDK_archdef.h"
#include "machine_type.h"
#include "fixmul.h"

#if defined(__arm__)
#include "arm/fixmadd_arm.h"

#elif defined(__mips__)	/* cppp replaced: elif */
#include "mips/fixmadd_mips.h"

#endif /* all cores */

/*************************************************************************
 *************************************************************************
    Software fallbacks for missing functions.
**************************************************************************
**************************************************************************/

/* Divide by two versions. */

#if !defined(FUNCTION_fixmadddiv2_DD)
inline FIXP_DBL fixmadddiv2_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b)
            { return (x + fMultDiv2 (a, b)); }
#endif

#if !defined(FUNCTION_fixmadddiv2_SD)
inline FIXP_DBL fixmadddiv2_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmadddiv2_DS
  return fixmadddiv2_DS(x, b, a);
#else
  return fixmadddiv2_DD(x, FX_SGL2FX_DBL(a), b);
#endif
}
#endif

#if !defined(FUNCTION_fixmadddiv2_DS)
inline FIXP_DBL fixmadddiv2_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmadddiv2_SD
  return fixmadddiv2_SD(x, b, a);
#else
  return fixmadddiv2_DD(x, a, FX_SGL2FX_DBL(b));
#endif
}
#endif

#if !defined(FUNCTION_fixmadddiv2_SS)
inline FIXP_DBL fixmadddiv2_SS (FIXP_DBL x, const FIXP_SGL a, const FIXP_SGL b)
            { return x + fMultDiv2(a,b); }
#endif

#if !defined(FUNCTION_fixmsubdiv2_DD)
inline FIXP_DBL fixmsubdiv2_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b)
            { return (x - fMultDiv2 (a, b)); }
#endif

#if !defined(FUNCTION_fixmsubdiv2_SD)
inline FIXP_DBL fixmsubdiv2_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmsubdiv2_DS
  return fixmsubdiv2_DS(x, b, a);
#else
  return fixmsubdiv2_DD(x, FX_SGL2FX_DBL(a), b);
#endif
}
#endif

#if !defined(FUNCTION_fixmsubdiv2_DS)
inline FIXP_DBL fixmsubdiv2_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmsubdiv2_SD
  return fixmsubdiv2_SD(x, b, a);
#else
  return fixmsubdiv2_DD(x, a, FX_SGL2FX_DBL(b));
#endif
}
#endif

#if !defined(FUNCTION_fixmsubdiv2_SS)
inline FIXP_DBL fixmsubdiv2_SS (FIXP_DBL x, const FIXP_SGL a, const FIXP_SGL b)
            { return x - fMultDiv2(a,b); }
#endif


#if !defined(FUNCTION_fixmadddiv2BitExact_DD)
#define FUNCTION_fixmadddiv2BitExact_DD
inline FIXP_DBL fixmadddiv2BitExact_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
  return x + fMultDiv2BitExact(a, b);
}
#endif
#if !defined(FUNCTION_fixmadddiv2BitExact_SD)
#define FUNCTION_fixmadddiv2BitExact_SD
inline FIXP_DBL fixmadddiv2BitExact_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmadddiv2BitExact_DS
  return fixmadddiv2BitExact_DS(x, b, a);
#else
  return x + fMultDiv2BitExact(a, b);
#endif
}
#endif
#if !defined(FUNCTION_fixmadddiv2BitExact_DS)
#define FUNCTION_fixmadddiv2BitExact_DS
inline FIXP_DBL fixmadddiv2BitExact_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmadddiv2BitExact_SD
  return fixmadddiv2BitExact_SD(x, b, a);
#else
  return x + fMultDiv2BitExact(a, b);
#endif
}
#endif

#if !defined(FUNCTION_fixmsubdiv2BitExact_DD)
#define FUNCTION_fixmsubdiv2BitExact_DD
inline FIXP_DBL fixmsubdiv2BitExact_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
  return x - fMultDiv2BitExact(a, b);
}
#endif
#if !defined(FUNCTION_fixmsubdiv2BitExact_SD)
#define FUNCTION_fixmsubdiv2BitExact_SD
inline FIXP_DBL fixmsubdiv2BitExact_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmsubdiv2BitExact_DS
  return fixmsubdiv2BitExact_DS(x, b, a);
#else
  return x - fMultDiv2BitExact(a, b);
#endif
}
#endif
#if !defined(FUNCTION_fixmsubdiv2BitExact_DS)
#define FUNCTION_fixmsubdiv2BitExact_DS
inline FIXP_DBL fixmsubdiv2BitExact_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmsubdiv2BitExact_SD
  return fixmsubdiv2BitExact_SD(x, b, a);
#else
  return x - fMultDiv2BitExact(a, b);
#endif
}
#endif

/* Normal versions */

#if !defined(FUNCTION_fixmadd_DD)
inline FIXP_DBL fixmadd_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b)
            { return fixmadddiv2_DD(x,a,b)<<1; }
#endif
#if !defined(FUNCTION_fixmadd_SD)
inline FIXP_DBL fixmadd_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmadd_DS
  return fixmadd_DS(x, b, a);
#else
  return fixmadd_DD(x, FX_SGL2FX_DBL(a), b);
#endif
}
#endif
#if !defined(FUNCTION_fixmadd_DS)
inline FIXP_DBL fixmadd_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmadd_SD
  return fixmadd_SD(x, b, a);
#else
  return fixmadd_DD(x, a, FX_SGL2FX_DBL(b));
#endif
}
#endif
#if !defined(FUNCTION_fixmadd_SS)
inline FIXP_DBL fixmadd_SS (FIXP_DBL x, const FIXP_SGL a, const FIXP_SGL b)
            { return (x + fMultDiv2(a,b))<<1; }
#endif

#if !defined(FUNCTION_fixmsub_DD)
inline FIXP_DBL fixmsub_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b)
            { return fixmsubdiv2_DD(x,a,b)<<1; }
#endif
#if !defined(FUNCTION_fixmsub_SD)
inline FIXP_DBL fixmsub_SD (FIXP_DBL x, const FIXP_SGL a, const FIXP_DBL b) {
#ifdef FUNCTION_fixmsub_DS
  return fixmsub_DS(x, b, a);
#else
  return fixmsub_DD(x, FX_SGL2FX_DBL(a), b);
#endif
}
#endif
#if !defined(FUNCTION_fixmsub_DS)
inline FIXP_DBL fixmsub_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
#ifdef FUNCTION_fixmsub_SD
  return fixmsub_SD(x, b, a);
#else
  return fixmsub_DD(x, a, FX_SGL2FX_DBL(b));
#endif
}
#endif
#if !defined(FUNCTION_fixmsub_SS)
inline FIXP_DBL fixmsub_SS (FIXP_DBL x, const FIXP_SGL a, const FIXP_SGL b)
            { return (x - fMultDiv2(a,b))<<1; }
#endif

#if !defined(FUNCTION_fixpow2adddiv2_D)
inline INT fixpadddiv2_D (FIXP_DBL x, const FIXP_DBL a)
            { return (x + fPow2Div2(a)); }
#endif
#if !defined(FUNCTION_fixpow2add_D)
inline INT fixpadd_D (FIXP_DBL x, const FIXP_DBL a)
            {  return (x + fPow2(a)); }
#endif

#if !defined(FUNCTION_fixpow2adddiv2_S)
inline INT fixpadddiv2_S (FIXP_DBL x, const FIXP_SGL a)
            { return (x + fPow2Div2(a)); }
#endif
#if !defined(FUNCTION_fixpow2add_S)
inline INT fixpadd_S (FIXP_DBL x, const FIXP_SGL a)
            {  return (x + fPow2(a)); }
#endif



#endif // __FIXMADD_H__

