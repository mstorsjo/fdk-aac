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

#if defined(__GNUC__) && defined(__ARM_ARCH_5TE__)	/* cppp replaced: elif */
  /* ARM gcc*/

  #define FUNCTION_fixnormz_D
  #define FUNCTION_fixnorm_D

  inline INT fixnormz_D(LONG value)
  {
    INT result;
    asm("clz %0, %1 ": "=r"(result) : "r"(value) );
    return result;
  }

  inline INT fixnorm_D(LONG value)
  {
    INT result;
    if (value == 0) {
      return 0;
    }
    if (value < 0) {
      value = ~value;
    }
    result =  fixnormz_D(value);
    return result - 1;
  }

#endif /* arm toolchain */

#endif /* __arm__ */

