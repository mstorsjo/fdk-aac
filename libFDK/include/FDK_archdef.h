/***************************  Fraunhofer IIS FDK Tools  ***********************

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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __ARCH_H__
#define __ARCH_H__

/* Performance / Quality profile selector */
  #define FDK_HIGH_PERFORMANCE

/* Unify some few toolchain specific defines to avoid having large "or" macro contraptions all over the source code. */

/* Take action against VisualStudio 2005 crosscompile problems. */

/* Use single macro (the GCC built in macro) for architecture identification independent of the particular toolchain */
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || (defined(_MSC_VER) && defined(_M_IX86)) || defined (__x86_64__)
#define __x86__
#endif

#if (defined(_M_ARM) || defined(__CC_ARM)) && !defined(__arm__) || defined(__TI_TMS470_V5__) && !defined(__arm__)
#define __arm__
#endif



/* Define __ARM_ARCH_5TE__ if armv5te features are supported  */
#if (__TARGET_ARCH_ARM == 5) || defined(__TARGET_FEATURE_DSPMUL) || (_M_ARM == 5) || defined(__ARM_ARCH_5TEJ__) || defined(__TI_TMS470_V5__) || defined(__ARM_ARCH_7EM__)
#define __ARM_ARCH_5TE__
#endif

/* Define __ARM_ARCH_6__ if the armv6 intructions are being supported. */
#if (__TARGET_ARCH_ARM == 6) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6ZK__)
#define __ARM_ARCH_5TE__
#define __ARM_ARCH_6__
#endif

/* Define __ARM_ARCH_7_A__ if the armv7 intructions are being supported. */
#if defined(__TARGET_ARCH_7_R) || defined(__ARM_ARCH_7R__)
#define __ARM_ARCH_5TE__
#define __ARM_ARCH_6__
#define __ARM_ARCH_7_R__
#endif

/* Define __ARM_ARCH_7_A__ if the armv7 intructions are being supported. */
#if defined(__TARGET_ARCH_7_A) || defined(__ARM_ARCH_7A__)
#define __ARM_ARCH_5TE__
#define __ARM_ARCH_6__
#define __ARM_ARCH_7_A__
#endif

/* Define __ARM_ARCH_7M__ if the ARMv7-M instructions are being supported, e.g. Cortex-M3. */
#if defined(__TARGET_ARCH_7_M) || defined(__ARM_ARCH_7_M__)
#define __ARM_ARCH_7M__
#endif

/* Define __ARM_ARCH_7EM__ if the ARMv7-ME instructions are being supported, e.g. Cortex-M4. */
#if defined(__TARGET_ARCH_7E_M) || defined(__ARM_ARCH_7E_M__)
#define __ARM_ARCH_7EM__
#endif

/* Detect and unify macros for neon feature. */
#if defined(__TARGET_FEATURE_NEON) && !defined(__ARM_NEON__)
#define __ARM_NEON__
#endif

#ifdef _M_ARM
#include "cmnintrin.h"
#include "armintr.h"
#endif



/* Define preferred Multiplication type */
#if defined(FDK_HIGH_PERFORMANCE) && !defined(FDK_HIGH_QUALITY) /* FDK_HIGH_PERFORMANCE */

#if defined(__mips__) || defined(__powerpc__) || defined(__sh__)
#define ARCH_PREFER_MULT_16x16
#undef SINETABLE_16BIT
#undef POW2COEFF_16BIT
#undef LDCOEFF_16BIT
#undef WINDOWTABLE_16BIT

#elif defined(__arm__) && defined(__ARM_ARCH_5TE__)	/* cppp replaced: elif */	/* cppp replaced: elif */
#define ARCH_PREFER_MULT_32x16
#define SINETABLE_16BIT
#define POW2COEFF_16BIT
#define LDCOEFF_16BIT
#define WINDOWTABLE_16BIT

#elif defined(__arm__) && defined(__ARM_ARCH_7M__)
#define ARCH_PREFER_MULT_32x16
#define SINETABLE_16BIT
#define POW2COEFF_16BIT
#define LDCOEFF_16BIT
#define WINDOWTABLE_16BIT

#elif defined(__arm__) && defined(__ARM_ARCH_7EM__)
#define ARCH_PREFER_MULT_32x32
#define ARCH_PREFER_MULT_32x16
#define SINETABLE_16BIT
#define POW2COEFF_16BIT
#define LDCOEFF_16BIT
#define WINDOWTABLE_16BIT

#elif defined(__arm__) && !defined(__ARM_ARCH_5TE__)
#define ARCH_PREFER_MULT_16x16
#undef SINETABLE_16BIT
#undef WINDOWTABLE_16BIT
#undef POW2COEFF_16BIT
#undef LDCOEFF_16BIT

#elif defined(__x86__)	/* cppp replaced: elif */
#define ARCH_PREFER_MULT_32x16
#define SINETABLE_16BIT
#define WINDOWTABLE_16BIT
#define POW2COEFF_16BIT
#define LDCOEFF_16BIT

#else

  #error  >>>> Please set architecture characterization defines for your platform (FDK_HIGH_PERFORMANCE)! <<<<

#endif /* Architecture switches */

#else /* neither FDK_HIGH_QUALITY or FDK_HIGH_PERFORMANCE */

#error Either set FDK_HIGH_QUALITY or FDK_HIGH_PERFORMANCE, but not both nor none of them.

#endif /* End of quality/complexity tradeoff */

#define FDKTOOLS_PACKED_TABLES


#ifdef SINETABLE_16BIT
  #define FIXP_STB FIXP_SGL     /* STB sinus Tab used in transformation */
  #define FIXP_STP FIXP_SPK
  #define STC(a) (FX_DBL2FXCONST_SGL(a))
#else
  #define FIXP_STB FIXP_DBL
  #define FIXP_STP FIXP_DPK
  #define STC(a) ((FIXP_DBL)(LONG)(a))
#endif /* defined(SINETABLE_16BIT) */

#define STCP(cos,sin) { { STC(cos), STC(sin) } }


#ifdef WINDOWTABLE_16BIT
  #define FIXP_WTB FIXP_SGL /* single FIXP_SGL values */
  #define FIXP_WTP FIXP_SPK /* packed FIXP_SGL values */
  #define WTC(a) FX_DBL2FXCONST_SGL(a)
#else /* SINETABLE_16BIT */
  #define FIXP_WTB FIXP_DBL
  #define FIXP_WTP FIXP_DPK
  #define WTC(a) (FIXP_DBL)(a)
#endif /* SINETABLE_16BIT */

#define WTCP(a,b) { { WTC(a), WTC(b) } }


#endif /* __ARCH_H__ */
