/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2005)
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
   Description: bitreversal of input data

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/



#if defined(FUNCTION_scramble)
#if defined(__GNUC__)	/* cppp replaced: elif */

#define FUNCTION_scramble

#if defined(__ARM_ARCH_5TE__)
#define USE_LDRD_STRD   /* LDRD requires 8 byte data alignment. */
#endif

inline void scramble(FIXP_DBL x [], INT n) {
  FDK_ASSERT(!(((INT)x)&(ALIGNMENT_DEFAULT-1)));
  asm("mov     r2, #1;\n"               /* r2(m) = 1;           */
      "sub     r3, %1, #1;\n"           /* r3 = n-1;            */
      "mov     r4, #0;\n"               /* r4(j) = 0;           */

"scramble_m_loop%=:\n"                  /* {                    */
      "mov     r5, %1;\n"               /*  r5(k) = 1;          */

"scramble_k_loop%=:\n"                  /*  {                   */
      "mov     r5, r5, lsr #1;\n"       /*   k >>= 1;           */
      "eor     r4, r4, r5;\n"           /*   j ^=k;             */
      "ands    r10, r4, r5;\n"           /*   r10 = r4 & r5;      */
      "beq     scramble_k_loop%=;\n"      /*  } while (r10 == 0);  */

      "cmp     r4, r2;\n"               /*   if (r4 < r2) break;        */
      "bcc     scramble_m_loop_end%=;\n"

#ifdef USE_LDRD_STRD
      "mov     r5, r2, lsl #3;\n"       /* m(r5) = r2*4*2               */
      "ldrd    r10, [%0, r5];\n"         /* r10 = x[r5], x7 = x[r5+1]     */
      "mov     r6, r4, lsl #3;\n"      /* j(r6) = r4*4*2              */
      "ldrd    r8, [%0, r6];\n"        /* r8 = x[r6], r9 = x[r6+1];  */
      "strd    r10, [%0, r6];\n"        /* x[r6,r6+1] = r10,r11;        */
      "strd    r8, [%0, r5];\n"         /* x[r5,r5+1] = r8,r9;          */
#else
      "mov      r5, r2, lsl #3;\n"       /* m(r5) = r2*4*2               */
      "ldr	r10, [%0, r5];\n"
      "mov      r6, r4, lsl #3;\n"      /* j(r6) = r4*4*2              */
      "ldr	r11, [%0, r6];\n"

      "str	r10, [%0, r6];\n"
      "str      r11, [%0, r5];\n"

      "add	r5, r5, #4;"
      "ldr	r10, [%0, r5];\n"
      "add	r6, r6, #4;"
      "ldr      r11, [%0, r6];\n"
      "str      r10, [%0, r6];\n"
      "str      r11, [%0, r5];\n"
#endif
"scramble_m_loop_end%=:\n"
      "add     r2, r2, #1;\n"           /* r2++;                        */
      "cmp     r2, r3;\n"
      "bcc     scramble_m_loop%=;\n"      /* } while (r2(m) < r3(n-1));   */
       :
       : "r"(x), "r"(n)
#ifdef USE_LDRD_STRD
       : "r2","r3", "r4","r5", "r10","r11", "r8","r9", "r6" );
#else
       : "r2","r3", "r4","r5", "r10","r11", "r6" );
#endif
}
#else
/* Force C implementation if no assembler version available. */
#undef FUNCTION_scramble
#endif	/* Toolchain selection. */

#endif	/* defined(FUNCTION_scramble) */
