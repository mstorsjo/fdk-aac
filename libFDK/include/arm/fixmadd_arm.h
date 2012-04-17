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

  /* ############################################################################# */
  #if defined(__GNUC__) && defined(__arm__) && !defined(__SYMBIAN32__)	/* cppp replaced: elif */
  /* ############################################################################# */
    /* ARM GNU GCC */

    #define FUNCTION_fixmadddiv2_DD

    #ifdef __ARM_ARCH_6__
      inline FIXP_DBL fixmadddiv2_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
        INT result;
        asm  ("smmla %0, %1, %2, %3;\n"
              : "=r" (result)
              : "r" (a), "r" (b), "r"(x) );
        return result ;
      }
      #define FUNCTION_fixmsubdiv2_DD
      inline FIXP_DBL fixmsubdiv2_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
        INT result;
        asm  ("smmls %0, %1, %2, %3;\n"
              : "=r" (result)
              : "r" (a), "r" (b), "r"(x) );
        return result ;
      }
    #else /* __ARM_ARCH_6__ */
      inline FIXP_DBL fixmadddiv2_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
        INT discard, result = x;
        asm  ("smlal %0, %1, %2, %3;\n"
              : "=r" (discard), "+r" (result)
              : "r" (a), "r" (b) );
        return result ;
      }
    #endif /* __ARM_ARCH_6__ */

    #if defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_6__)

      #define FUNCTION_fixmadddiv2_DS

      inline FIXP_DBL fixmadddiv2_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
        INT result;
        asm("smlawb %0, %1, %2, %3 "
              : "=r" (result)
              : "r" (a), "r" (b), "r" (x) );
        return result ;
      }

    #endif /* defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_6__) */

    #define FUNCTION_fixmadddiv2BitExact_DD
    #define fixmadddiv2BitExact_DD(a, b, c) fixmadddiv2_DD(a, b, c)

    #define FUNCTION_fixmsubdiv2BitExact_DD
    inline FIXP_DBL fixmsubdiv2BitExact_DD (FIXP_DBL x, const FIXP_DBL a, const FIXP_DBL b) {
      return x - fixmuldiv2BitExact_DD(a, b);
    }

    #define FUNCTION_fixmadddiv2BitExact_DS
    #define fixmadddiv2BitExact_DS(a, b, c) fixmadddiv2_DS(a, b, c)

    #define FUNCTION_fixmsubdiv2BitExact_DS
    inline FIXP_DBL fixmsubdiv2BitExact_DS (FIXP_DBL x, const FIXP_DBL a, const FIXP_SGL b) {
      return x - fixmuldiv2BitExact_DS(a, b);
    }
  /* ############################################################################# */
  #endif /* toolchain */
  /* ############################################################################# */

#endif /* __arm__ */

