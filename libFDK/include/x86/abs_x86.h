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
#if defined(__x86__)

  #if defined(__x86_64__)

    inline INT fixabs_D(INT x)      { return ((x) > (INT)(0))      ? (x) : -(x) ; }
    inline INT fixabs_S(INT x)      { return ((x) > (INT)(0))      ? (x) : -(x) ; }

    #define fixabs_I(x) fixabs_D(x)

    #define FUNCTION_fixabs_S
    #define FUNCTION_fixabs_D
    #define FUNCTION_fixabs_I

  #endif /* __x86_64__ */

#endif /*__x86__ */

