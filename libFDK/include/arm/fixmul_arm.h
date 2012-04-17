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
#if defined(__arm__)

#if defined(__GNUC__) && defined(__arm__)	/* cppp replaced: elif */
/* ARM with GNU compiler */

#define FUNCTION_fixmuldiv2_DD

#define FUNCTION_fixmuldiv2BitExact_DD
#define fixmuldiv2BitExact_DD(a,b) fixmuldiv2_DD(a,b)
#define FUNCTION_fixmulBitExact_DD
#define fixmulBitExact_DD(a,b) fixmul_DD(a,b)

#define FUNCTION_fixmuldiv2BitExact_DS
#define fixmuldiv2BitExact_DS(a,b) fixmuldiv2_DS(a,b)

#define FUNCTION_fixmulBitExact_DS
#define fixmulBitExact_DS(a,b) fixmul_DS(a,b)

#if defined(__ARM_ARCH_6__) || defined(__TARGET_ARCH_7E_M)
inline INT fixmuldiv2_DD (const INT a, const INT b)
{
  INT result ;
  __asm__ ("smmul %0, %1, %2" : "=r" (result)
                              : "r" (a), "r" (b)) ;
  return result ;
}
#else
inline INT fixmuldiv2_DD (const INT a, const INT b)
{
  INT discard, result ;
  __asm__ ("smull %0, %1, %2, %3" : "=&r" (discard), "=r" (result)
                                  : "r" (a), "r" (b)) ;
  return result ;
}
#endif

#if defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_6__)
#define FUNCTION_fixmuldiv2_SD
inline INT fixmuldiv2_SD (const SHORT a, const INT b)
{
  INT result ;
  __asm__ ("smulwb %0, %1, %2"
    : "=r" (result)
    : "r" (b), "r" (a)) ;
  return result ;
}
#endif

#endif /* defined(__GNUC__) && defined(__arm__) */

#endif /* __arm__ */

