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


#if defined(__arm__) && defined(__GNUC__)	/* cppp replaced: elif */

#if defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_6__)
  #define FUNCTION_cplxMultDiv2_32x16
  #define FUNCTION_cplxMultDiv2_32x16X2
  //#define FUNCTION_cplxMult_32x16
  //#define FUNCTION_cplxMult_32x16X2
#endif

#define FUNCTION_cplxMultDiv2_32x32X2
//#define FUNCTION_cplxMult_32x32X2

#ifdef FUNCTION_cplxMultDiv2_32x16
inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL a_Re,
                          const FIXP_DBL a_Im,
                          const FIXP_SPK wpk )
{
   LONG tmp1,tmp2;
   const LONG w = wpk.w;

   asm("smulwt %0, %3, %4;\n"
       "rsb %1,%0,#0;\n"
       "smlawb %0, %2, %4, %1;\n"
       "smulwt %1, %2, %4;\n"
       "smlawb %1, %3, %4, %1;\n"
       : "=&r"(tmp1), "=&r"(tmp2)
       : "r"(a_Re), "r"(a_Im), "r"(w)
       );

    *c_Re = tmp1;
    *c_Im = tmp2;
}
#endif /* FUNCTION_cplxMultDiv2_32x16 */

#ifdef FUNCTION_cplxMultDiv2_32x16X2
inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL a_Re,
                          const FIXP_DBL a_Im,
                          const FIXP_SGL b_Re,
                          const FIXP_SGL b_Im)
{
    LONG tmp1, tmp2;

    asm("smulwb %0, %3, %5;\n"     /* %7   = -a_Im * b_Im */
        "rsb %1,%0,#0;\n"
        "smlawb %0, %2, %4, %1;\n" /* tmp1 =  a_Re * b_Re - a_Im * b_Im */
        "smulwb %1, %2, %5;\n"     /* %7   =  a_Re * b_Im */
        "smlawb %1, %3, %4, %1;\n" /* tmp2 =  a_Im * b_Re + a_Re * b_Im */
        : "=&r"(tmp1), "=&r"(tmp2)
        : "r"(a_Re), "r"(a_Im), "r"(b_Re), "r"(b_Im)
        );

     *c_Re = tmp1;
     *c_Im = tmp2;
}
#endif /* FUNCTION_cplxMultDiv2_32x16X2 */

#ifdef FUNCTION_cplxMultAddDiv2_32x16X2
inline void cplxMultAddDiv2( FIXP_DBL *c_Re,
                             FIXP_DBL *c_Im,
                             const FIXP_DBL  a_Re,
                             const FIXP_DBL  a_Im,
                             const FIXP_SGL b_Re,
                             const FIXP_SGL b_Im)
{
    LONG tmp1, tmp2;

    asm("smulwb %0, %3, %5;\n"
        "rsb %1,%0,#0;\n"
        "smlawb %0, %2, %4, %1;\n"
        "smulwb %1, %2, %5;\n"
        "smlawb %1, %3, %4, %1;\n"
        : "=&r"(tmp1), "=&r"(tmp2)
        : "r"(a_Re), "r"(a_Im), "r"(b_Re), "r"(b_Im)
        );

     *c_Re += tmp1;
     *c_Im += tmp2;
}
#endif /* FUNCTION_cplxMultAddDiv2_32x16X2 */


#ifdef FUNCTION_cplxMultDiv2_32x32X2
inline void cplxMultDiv2( FIXP_DBL *c_Re,
                          FIXP_DBL *c_Im,
                          const FIXP_DBL a_Re,
                          const FIXP_DBL a_Im,
                          const FIXP_DBL b_Re,
                          const FIXP_DBL b_Im)
{
    LONG tmp1, tmp2;

#ifdef __ARM_ARCH_6__
    asm(
       "smmul %0, %2, %4;\n"     /* tmp1  = a_Re * b_Re */
       "smmls %0, %3, %5, %0;\n" /* tmp1 -= a_Im * b_Im */
       "smmul %1, %2, %5;\n"     /* tmp2  = a_Re * b_Im */
       "smmla %1, %3, %4, %1;\n" /* tmp2 += a_Im * b_Re */
       : "=&r"(tmp1), "=&r"(tmp2)
       : "r"(a_Re), "r"(a_Im), "r"(b_Re), "r"(b_Im)
       : "r0"
       );
#else
    LONG discard;
    asm(
       "smull %2, %0, %7, %6;\n" /* tmp1  = -a_Im * b_Im */
       "smlal %2, %0, %3, %5;\n" /* tmp1 +=  a_Re * b_Re */
       "smull %2, %1, %3, %6;\n" /* tmp2  =  a_Re * b_Im */
       "smlal %2, %1, %4, %5;\n" /* tmp2 +=  a_Im * b_Re */
       : "=&r"(tmp1), "=&r"(tmp2), "=&r"(discard)
       : "r"(a_Re), "r"(a_Im), "r"(b_Re), "r"(b_Im), "r"(-a_Im)
       );
 #endif
    *c_Re = tmp1;
    *c_Im = tmp2;
}
#endif /* FUNCTION_cplxMultDiv2_32x32X2 */


#endif

