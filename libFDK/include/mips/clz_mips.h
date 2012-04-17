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
#if defined(__mips__)

#if defined(__mips__) && (__GNUC__==2) && (mips>=32)

  #define FUNCTION_fixnormz_D
  inline INT fixnormz_D(LONG value)
  {
    INT result;
    __asm__ ("clz %0,%1" : "=d" (result) : "d" (value));

    return result;
  }

#elif defined(__mips__) && (__GNUC__==3) && (__mips>=32)

  #define FUNCTION_fixnormz_D
  INT inline fixnormz_D(LONG value)
  {
    INT result;
    __asm__ ("clz %[result], %[value]" : [result] "=r" (result) : [value] "r" (value)) ;

    return result;
  }

#endif

#endif /* __mips__ */

