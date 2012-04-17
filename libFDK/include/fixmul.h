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
   Author(s):   Stefan Gewinner
   Description: fixed point multiplication

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#if !defined(__FIXMUL_H__)
#define __FIXMUL_H__

#include "FDK_archdef.h"
#include "machine_type.h"


#if defined(__arm__)
#include "arm/fixmul_arm.h"

#elif defined(__mips__)	/* cppp replaced: elif */
#include "mips/fixmul_mips.h"

#elif defined(__x86__)	/* cppp replaced: elif */
#include "x86/fixmul_x86.h"

#endif /* all cores */

/*************************************************************************
 *************************************************************************
    Software fallbacks for missing functions
**************************************************************************
**************************************************************************/

#if !defined(FUNCTION_fixmuldiv2_DD)
#define FUNCTION_fixmuldiv2_DD
#if defined(_MSC_VER) || defined(__CC_ARM) || defined(__ANALOG_EXTENSIONS__) || defined(__TI_COMPILER_VERSION__)
#pragma message ("Extremely slow implementation of fixmuldiv2_DD !!")
#else
#warning Extremely slow implementation of fixmuldiv2_DD !!
#endif
inline LONG fixmuldiv2_DD (const LONG a, const LONG b)
{
  return (LONG) ((((INT64)a) * b) >> 32) ;
}
#endif

#if !defined(FUNCTION_fixmuldiv2BitExact_DD)
#define FUNCTION_fixmuldiv2BitExact_DD
#if defined(_MSC_VER) || defined(__CC_ARM) || defined(__ANALOG_EXTENSIONS__) || defined(__TI_COMPILER_VERSION__)
#pragma message ("Extremely slow implementation of fixmuldiv2BitExact_DD !!")
#else
#warning Extremely slow implementation of fixmuldiv2BitExact_DD !!
#endif
inline LONG fixmuldiv2BitExact_DD (const LONG a, const LONG b)
{
  return (LONG) ((((INT64)a) * b) >> 32) ;
}
#endif

#if !defined(FUNCTION_fixmul_DD)
#define FUNCTION_fixmul_DD
inline LONG fixmul_DD (const LONG a, const LONG b)
  { return fixmuldiv2_DD (a, b) << 1 ; }
#endif

#if !defined(FUNCTION_fixmulBitExact_DD)
#define FUNCTION_fixmulBitExact_DD
#if defined(_MSC_VER) || defined(__CC_ARM) || defined(__ANALOG_EXTENSIONS__) || defined(__TI_COMPILER_VERSION__) || defined(__XTENSA__)
#pragma message ("Extremely slow implementation of fixmulBitExact_DD !!")
#else
#warning Extremely slow implementation of fixmulBitExact_DD !!
#endif
inline LONG fixmulBitExact_DD (const LONG a, const LONG b)
{
  return ( (LONG) ((((INT64)a) * b) >> 32) ) << 1;
}
#endif

#if !defined(FUNCTION_fixmuldiv2_SS)
#define FUNCTION_fixmuldiv2_SS
inline LONG fixmuldiv2_SS (const SHORT a, const SHORT b)
  { return ((LONG)a*b); }
#endif

#if !defined(FUNCTION_fixmul_SS)
#define FUNCTION_fixmul_SS
inline LONG fixmul_SS (const SHORT a, const SHORT b)
  { return (a*b) <<1; }
#endif

#if !defined(FUNCTION_fixmuldiv2_SD)
#define FUNCTION_fixmuldiv2_SD
inline LONG fixmuldiv2_SD (const SHORT a, const LONG b)
#ifdef FUNCTION_fixmuldiv2_DS
  { return fixmuldiv2_DS(b, a); }
#else
  { return fixmuldiv2_DD(FX_SGL2FX_DBL(a), b); }
#endif
#endif

#if !defined(FUNCTION_fixmuldiv2_DS)
#define FUNCTION_fixmuldiv2_DS
inline LONG fixmuldiv2_DS (const LONG a, const SHORT b)
#ifdef FUNCTION_fixmuldiv2_SD
  { return fixmuldiv2_SD(b, a); }
#else
  { return fixmuldiv2_DD(a, FX_SGL2FX_DBL(b)); }
#endif
#endif

#if !defined(FUNCTION_fixmuldiv2BitExact_SD)
#define FUNCTION_fixmuldiv2BitExact_SD
inline LONG fixmuldiv2BitExact_SD (const SHORT a, const LONG b)
#ifdef FUNCTION_fixmuldiv2BitExact_DS
  { return fixmuldiv2BitExact_DS(b, a); }
#else
  { return (LONG) ((((INT64)a) * b) >> 16) ; }
#endif
#endif

#if !defined(FUNCTION_fixmuldiv2BitExact_DS)
#define FUNCTION_fixmuldiv2BitExact_DS
inline LONG fixmuldiv2BitExact_DS (const LONG a, const SHORT b)
#ifdef FUNCTION_fixmuldiv2BitExact_SD
  { return fixmuldiv2BitExact_SD(b, a); }
#else
  { return (LONG) ((((INT64)a) * b) >> 16) ; }
#endif
#endif

#if !defined(FUNCTION_fixmul_SD)
#define FUNCTION_fixmul_SD
inline LONG fixmul_SD (const SHORT a, const LONG b) {
#ifdef FUNCTION_fixmul_DS
  return fixmul_SD(b, a);
#else
  return fixmuldiv2_SD (a, b) << 1 ;
#endif
}
#endif

#if !defined(FUNCTION_fixmul_DS)
#define FUNCTION_fixmul_DS
inline LONG fixmul_DS (const LONG a, const SHORT b) {
#ifdef FUNCTION_fixmul_SD
  return fixmul_SD(b, a);
#else
  return fixmuldiv2_DS(a, b) << 1 ;
#endif
}
#endif

#if !defined(FUNCTION_fixmulBitExact_SD)
#define FUNCTION_fixmulBitExact_SD
inline LONG fixmulBitExact_SD (const SHORT a, const LONG b)
#ifdef FUNCTION_fixmulBitExact_DS
  { return fixmulBitExact_DS(b, a); }
#else
  { return (LONG) (((((INT64)a) * b) >> 16) << 1); }
#endif
#endif

#if !defined(FUNCTION_fixmulBitExact_DS)
#define FUNCTION_fixmulBitExact_DS
inline LONG fixmulBitExact_DS (const LONG a, const SHORT b)
#ifdef FUNCTION_fixmulBitExact_SD
  { return fixmulBitExact_SD(b, a); }
#else
  { return (LONG) (((((INT64)a) * b) >> 16) << 1); }
#endif
#endif


#if !defined(FUNCTION_fixpow2div2_D)
#ifdef ARCH_WA_16BITMULT
#error Fallback for fixpow2div2_D is not 16 bit safe !
#endif
#define FUNCTION_fixpow2div2_D
inline LONG fixpow2div2_D (const LONG a)
  { return fixmuldiv2_DD(a, a); }
#endif

#if !defined(FUNCTION_fixpow2_D)
#ifdef ARCH_WA_16BITMULT
#error Fallback for fixpow2_D is not 16 bit safe !
#endif
#define FUNCTION_fixpow2_D
inline LONG fixpow2_D (const LONG a)
  { return fixpow2div2_D(a)<<1; }
#endif

#if !defined(FUNCTION_fixpow2div2_S)
#define FUNCTION_fixpow2div2_S
inline LONG fixpow2div2_S (const SHORT a)
  { return fixmuldiv2_SS(a, a); }
#endif

#if !defined(FUNCTION_fixpow2_S)
#define FUNCTION_fixpow2_S
inline LONG fixpow2_S (const SHORT a)
  { return fixpow2div2_S(a)<<1; }
#endif


#endif /* __FIXMUL_H__ */
