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


#if defined(__GNUC__) && defined(__mips__)


//#define FUNCTION_cplxMultDiv2_32x16
//#define FUNCTION_cplxMultDiv2_32x16X2
#define FUNCTION_cplxMultDiv2_32x32X2
//#define FUNCTION_cplxMult_32x16
//#define FUNCTION_cplxMult_32x16X2
#define FUNCTION_cplxMult_32x32X2

#if defined(FUNCTION_cplxMultDiv2_32x32X2)
inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          FIXP_DBL  a_Re,
                          FIXP_DBL  a_Im,
                          FIXP_DBL  b_Re,
                          FIXP_DBL  b_Im)
{
   INT result;

   __asm__ ("mult %[a_Re], %[b_Re];\n"
            "msub %[a_Im], %[b_Im];\n"
       : "=hi"(result)
       : [a_Re]"r"(a_Re), [b_Re]"r"(b_Re),  [a_Im]"r"(a_Im), [b_Im]"r"(b_Im)
       : "lo");

   *c_Re = result;

   __asm__ ("mult %[a_Re], %[b_Im];\n"
            "madd %[a_Im], %[b_Re];\n"
       : "=hi"(result)
       : [a_Re]"r"(a_Re), [b_Im]"r"(b_Im), [a_Im]"r"(a_Im), [b_Re]"r"(b_Re)
       : "lo");
   *c_Im = result;
}
#endif

#if defined(FUNCTION_cplxMult_32x32X2)
inline void cplxMult( FIXP_DBL *c_Re,
                      FIXP_DBL *c_Im,
                      FIXP_DBL  a_Re,
                      FIXP_DBL  a_Im,
                      FIXP_DBL  b_Re,
                      FIXP_DBL  b_Im)
{
   INT result;
   __asm__ ("mult %[a_Re], %[b_Re];\n"
            "msub %[a_Im], %[b_Im];\n"
        : "=hi"(result)
        : [a_Re]"r"(a_Re), [b_Re]"r"(b_Re), [a_Im]"r"(a_Im), [b_Im]"r"(b_Im)
        : "lo");
   *c_Re = result<<1;

   __asm__ ("mult %[a_Re], %[b_Im];\n"
            "madd %[a_Im], %[b_Re];\n"
        : "=hi"(result)
        : [a_Re]"r"(a_Re), [b_Im]"r"(b_Im), [a_Im]"r"(a_Im), [b_Re]"r"(b_Re)
        : "lo");
   *c_Im = result<<1;
}
#endif

#endif /* defined(__GNUC__) && defined(__mips__) */

