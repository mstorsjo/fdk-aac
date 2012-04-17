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

#if defined(_MSC_VER) && defined(_M_IX86)
/* Intel x86 */

#define FUNCTION_fixmul_DD
#define FUNCTION_fixmuldiv2_DD
#define FUNCTION_fixmuldiv2BitExact_DD
#define fixmuldiv2BitExact_DD(a,b) fixmuldiv2_DD(a,b)
#define FUNCTION_fixmulBitExact_DD
#define fixmulBitExact_DD(a,b) fixmul_DD(a,b)

#define FUNCTION_fixmuldiv2BitExact_DS
#define fixmuldiv2BitExact_DS(a,b) fixmuldiv2_DS(a,b)

#define FUNCTION_fixmulBitExact_DS
#define fixmulBitExact_DS(a,b) fixmul_DS(a,b)

inline INT fixmul_DD (INT a, const INT b)
{
  __asm
  {
    mov eax, a
    imul b
    shl edx, 1
    mov a, edx
  }
  return a ;
}


inline INT fixmuldiv2_DD (INT a, const INT b)
{
  __asm
  {
    mov eax, a
    imul b
    mov a, edx
  }
  return a ;
}

/* ############################################################################# */
#elif (defined(__GNUC__)||defined(__gnu_linux__)) && defined(__x86__)

#define FUNCTION_fixmul_DD
#define FUNCTION_fixmuldiv2_DD

#define FUNCTION_fixmuldiv2BitExact_DD
#define fixmuldiv2BitExact_DD(a,b) fixmuldiv2_DD(a,b)

#define FUNCTION_fixmulBitExact_DD
#define fixmulBitExact_DD(a,b) fixmul_DD(a,b)

#define FUNCTION_fixmuldiv2BitExact_DS
#define fixmuldiv2BitExact_DS(a,b) fixmuldiv2_DS(a,b)

#define FUNCTION_fixmulBitExact_DS
#define fixmulBitExact_DS(a,b) fixmul_DS(a,b)

inline INT fixmul_DD (INT a, const INT b)
{
  INT result;

  asm( "imul %2;\n"
       "shl $1, %0;\n"
            : "=d"(result), "+a"(a)
            : "r"(b) );

  return result;
}


inline INT fixmuldiv2_DD (INT a, const INT b)
{
  INT result;

  asm ( "imul %2;"
             : "=d"(result), "+a"(a)
             : "r"(b) );

  return result;
}

#endif /* (defined(__GNUC__)||defined(__gnu_linux__)) && defined(__x86__) */

#endif /* __x86__ */

