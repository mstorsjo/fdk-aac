/****************************************************************************

                       (C) copyright Fraunhofer IIS (2004)
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

***************************************************************************/



#ifdef FUNCTION_dct_IV_func1

/*
   Note: This assembler routine is here, because the ARM926 compiler does
         not encode the inline assembler with optimal speed.
         With this version, we save 2 cycles per loop iteration.
*/

__asm  void dct_IV_func1(
    int i,
    const FIXP_SPK *twiddle,
    FIXP_DBL *RESTRICT pDat_0,
    FIXP_DBL *RESTRICT pDat_1)
{
    /* Register map:
       r0   i
       r1   twiddle
       r2   pDat_0
       r3   pDat_1
       r4   accu1
       r5   accu2
       r6   accu3
       r7   accu4
       r8   val_tw
       r9   accuX
    */
    PUSH    {r4-r9}

     /* 44 cycles for 2 iterations = 22 cycles/iteration */
dct_IV_loop1_start
/*  First iteration */
    LDR     r8, [r1], #4    // val_tw = *twiddle++;
    LDR     r5, [r2, #0]    // accu2 = pDat_0[0]
    LDR     r4, [r3, #0]    // accu1 = pDat_1[0]

    SMULWT  r9, r5, r8      // accuX = accu2*val_tw.l
    SMULWB  r5, r5, r8      // accu2 = accu2*val_tw.h
    RSB     r9, r9, #0      // accuX =-accu2*val_tw.l
    SMLAWT  r5, r4, r8, r5  // accu2 = accu2*val_tw.h + accu1*val_tw.l
    SMLAWB  r4, r4, r8, r9  // accu1 = accu1*val_tw.h - accu2*val_tw.l

    LDR     r8, [r1], #4    // val_tw = *twiddle++;
    LDR     r7, [r3, #-4]   // accu4 = pDat_1[-1]
    LDR     r6, [r2, #4]    // accu3 = pDat_0[1]

    SMULWB  r9, r7, r8      // accuX = accu4*val_tw.h
    SMULWT  r7, r7, r8      // accu4 = accu4*val_tw.l
    RSB     r9, r9, #0      // accuX =-accu4*val_tw.h
    SMLAWB  r7, r6, r8, r7  // accu4 = accu4*val_tw.l+accu3*val_tw.h
    SMLAWT  r6, r6, r8, r9  // accu3 = accu3*val_tw.l-accu4*val_tw.h

    STR     r5, [r2], #4    // *pDat_0++ = accu2
    STR     r4, [r2], #4    // *pDat_0++ = accu1
    STR     r6, [r3], #-4   // *pDat_1-- = accu3
    STR     r7, [r3], #-4   // *pDat_1-- = accu4

/*  Second iteration */
    LDR     r8, [r1], #4    // val_tw = *twiddle++;
    LDR     r5, [r2, #0]    // accu2 = pDat_0[0]
    LDR     r4, [r3, #0]    // accu1 = pDat_1[0]

    SMULWT  r9, r5, r8      // accuX = accu2*val_tw.l
    SMULWB  r5, r5, r8      // accu2 = accu2*val_tw.h
    RSB     r9, r9, #0      // accuX =-accu2*val_tw.l
    SMLAWT  r5, r4, r8, r5  // accu2 = accu2*val_tw.h + accu1*val_tw.l
    SMLAWB  r4, r4, r8, r9  // accu1 = accu1*val_tw.h - accu2*val_tw.l

    LDR     r8, [r1], #4    // val_tw = *twiddle++;
    LDR     r7, [r3, #-4]   // accu4 = pDat_1[-1]
    LDR     r6, [r2, #4]    // accu3 = pDat_0[1]

    SMULWB  r9, r7, r8      // accuX = accu4*val_tw.h
    SMULWT  r7, r7, r8      // accu4 = accu4*val_tw.l
    RSB     r9, r9, #0      // accuX =-accu4*val_tw.h
    SMLAWB  r7, r6, r8, r7  // accu4 = accu4*val_tw.l+accu3*val_tw.h
    SMLAWT  r6, r6, r8, r9  // accu3 = accu3*val_tw.l-accu4*val_tw.h

    STR     r5, [r2], #4    // *pDat_0++ = accu2
    STR     r4, [r2], #4    // *pDat_0++ = accu1
    STR     r6, [r3], #-4   // *pDat_1-- = accu3
    STR     r7, [r3], #-4   // *pDat_1-- = accu4

    SUBS    r0, r0, #1
    BNE     dct_IV_loop1_start

    POP     {r4-r9}

    BX      lr
}

#endif /* FUNCTION_dct_IV_func1 */


#ifdef FUNCTION_dct_IV_func2

FDK_INLINE
/* __attribute__((noinline)) */
static void dct_IV_func2(
    int i,
    const FIXP_SPK *twiddle,
    FIXP_DBL *pDat_0,
    FIXP_DBL *pDat_1,
    int inc)
{
  FIXP_DBL accu1, accu2, accu3, accu4, accuX;
  LONG val_tw;

  accu1 = pDat_1[-2];
  accu2 = pDat_1[-1];

  *--pDat_1 = -(pDat_0[1]>>1);
  *pDat_0++ = (pDat_0[0]>>1);

  twiddle += inc;

__asm
  {
    LDR     val_tw, [twiddle], inc, LSL #2    // val_tw = *twiddle; twiddle += inc
    B       dct_IV_loop2_2nd_part

    /* 42 cycles for 2 iterations = 21 cycles/iteration */
dct_IV_loop2:
    SMULWT  accuX, accu2, val_tw
    SMULWB  accu2, accu2, val_tw
    RSB     accuX, accuX, #0
    SMLAWB  accuX, accu1, val_tw, accuX
    SMLAWT  accu2, accu1, val_tw, accu2
    STR     accuX, [pDat_0], #4
    STR     accu2, [pDat_1, #-4] !

    LDR     accu4, [pDat_0, #4]
    LDR     accu3, [pDat_0]
    SMULWB  accuX, accu4, val_tw
    SMULWT  accu4, accu4, val_tw
    RSB     accuX, accuX, #0
    SMLAWT  accuX, accu3, val_tw, accuX
    SMLAWB  accu4, accu3, val_tw, accu4

    LDR     accu1, [pDat_1, #-8]
    LDR     accu2, [pDat_1, #-4]

    LDR     val_tw, [twiddle], inc, LSL #2    // val_tw = *twiddle; twiddle += inc

    STR     accuX, [pDat_1, #-4] !
    STR     accu4, [pDat_0], #4

dct_IV_loop2_2nd_part:
    SMULWT  accuX, accu2, val_tw
    SMULWB  accu2, accu2, val_tw
    RSB     accuX, accuX, #0
    SMLAWB  accuX, accu1, val_tw, accuX
    SMLAWT  accu2, accu1, val_tw, accu2
    STR     accuX, [pDat_0], #4
    STR     accu2, [pDat_1, #-4] !

    LDR     accu4, [pDat_0, #4]
    LDR     accu3, [pDat_0]
    SMULWB  accuX, accu4, val_tw
    SMULWT  accu4, accu4, val_tw
    RSB     accuX, accuX, #0
    SMLAWT  accuX, accu3, val_tw, accuX
    SMLAWB  accu4, accu3, val_tw, accu4

    LDR     accu1, [pDat_1, #-8]
    LDR     accu2, [pDat_1, #-4]

    STR     accuX, [pDat_1, #-4] !
    STR     accu4, [pDat_0], #4

    LDR     val_tw, [twiddle], inc, LSL #2    // val_tw = *twiddle; twiddle += inc

    SUBS    i, i, #1
    BNE     dct_IV_loop2
  }

  /* Last Sin and Cos value pair are the same */
  accu1 = fMultDiv2(accu1, WTC(0x5a82799a));
  accu2 = fMultDiv2(accu2, WTC(0x5a82799a));

  *--pDat_1 = accu1 + accu2;
  *pDat_0++ = accu1 - accu2;
}
#endif /* FUNCTION_dct_IV_func2 */


#ifdef FUNCTION_dst_IV_func1

__asm void dst_IV_func1(
    int i,
    const FIXP_SPK *twiddle, 
    FIXP_DBL *pDat_0, 
    FIXP_DBL *pDat_1)
{
    /* Register map:
       r0   i
       r1   twiddle
       r2   pDat_0
       r3   pDat_1
       r4   accu1
       r5   accu2
       r6   accu3
       r7   accu4
       r8   val_tw
       r9   accuX
    */
    PUSH    {r4-r9}

dst_IV_loop1
    LDR     r8, [r1], #4               // val_tw = *twiddle++
    LDR     r5, [r2]                   // accu2 = pDat_0[0]
    LDR     r6, [r2, #4]               // accu3 = pDat_0[1]
    RSB     r5, r5, #0                 // accu2 = -accu2
    SMULWT  r9, r5, r8                 // accuX = (-accu2)*val_tw.l
    LDR     r4, [r3, #-4]              // accu1 = pDat_1[-1] 
    RSB     r9, r9, #0                 // accuX = -(-accu2)*val_tw.l
    SMLAWB  r9, r4, r8, r9             // accuX = accu1*val_tw.h-(-accu2)*val_tw.l
    SMULWT  r4, r4, r8                 // accu1 = accu1*val_tw.l
    LDR     r7, [r3, #-8]              // accu4 = pDat_1[-2]
    SMLAWB  r5, r5, r8, r4             // accu2 = (-accu2)*val_tw.t+accu1*val_tw.l
    LDR     r8, [r1], #4               // val_tw = *twiddle++
    STR     r5, [r2], #4               // *pDat_0++ = accu2
    STR     r9, [r2], #4               // *pDat_0++ = accu1 (accuX)
    RSB     r7, r7, #0                 // accu4 = -accu4
    SMULWB  r5, r7, r8                 // accu2 = (-accu4)*val_tw.h
    SMULWB  r4, r6, r8                 // accu1 = (-accu4)*val_tw.l
    RSB     r5, r5, #0                 // accu2 = -(-accu4)*val_tw.h
    SMLAWT  r6, r6, r8, r5             // accu3 = (-accu4)*val_tw.l-(-accu3)*val_tw.h
    SMLAWT  r7, r7, r8, r4             // accu4 = (-accu3)*val_tw.l+(-accu4)*val_tw.h
    STR     r6, [r3, #-4] !            // *--pDat_1 = accu3
    STR     r7, [r3, #-4] !            // *--pDat_1 = accu4

    LDR     r8, [r1], #4               // val_tw = *twiddle++
    LDR     r5, [r2]                   // accu2 = pDat_0[0]
    LDR     r6, [r2, #4]               // accu3 = pDat_0[1]
    RSB     r5, r5, #0                 // accu2 = -accu2
    SMULWT  r9, r5, r8                 // accuX = (-accu2)*val_tw.l
    LDR     r4, [r3, #-4]              // accu1 = pDat_1[-1] 
    RSB     r9, r9, #0                 // accuX = -(-accu2)*val_tw.l
    SMLAWB  r9, r4, r8, r9             // accuX = accu1*val_tw.h-(-accu2)*val_tw.l
    SMULWT  r4, r4, r8                 // accu1 = accu1*val_tw.l
    LDR     r7, [r3, #-8]              // accu4 = pDat_1[-2]
    SMLAWB  r5, r5, r8, r4             // accu2 = (-accu2)*val_tw.t+accu1*val_tw.l
    LDR     r8, [r1], #4               // val_tw = *twiddle++
    STR     r5, [r2], #4               // *pDat_0++ = accu2
    STR     r9, [r2], #4               // *pDat_0++ = accu1 (accuX)
    RSB     r7, r7, #0                 // accu4 = -accu4
    SMULWB  r5, r7, r8                 // accu2 = (-accu4)*val_tw.h
    SMULWB  r4, r6, r8                 // accu1 = (-accu4)*val_tw.l
    RSB     r5, r5, #0                 // accu2 = -(-accu4)*val_tw.h
    SMLAWT  r6, r6, r8, r5             // accu3 = (-accu4)*val_tw.l-(-accu3)*val_tw.h
    SMLAWT  r7, r7, r8, r4             // accu4 = (-accu3)*val_tw.l+(-accu4)*val_tw.h
    STR     r6, [r3, #-4] !            // *--pDat_1 = accu3
    STR     r7, [r3, #-4] !            // *--pDat_1 = accu4

    SUBS    r0, r0, #4                 // i-= 4
    BNE     dst_IV_loop1

    POP     {r4-r9}
    BX      lr
}
#endif /* FUNCTION_dst_IV_func1 */

#ifdef FUNCTION_dst_IV_func2

FDK_INLINE 
/* __attribute__((noinline)) */
static void dst_IV_func2(
    int i,
    const FIXP_SPK *twiddle,
    FIXP_DBL *RESTRICT pDat_0, 
    FIXP_DBL *RESTRICT pDat_1,
    int inc)
{
  FIXP_DBL accu1,accu2,accu3,accu4;
  LONG val_tw;
  
  accu4 = pDat_0[0];
  accu3 = pDat_0[1];
  accu4 >>= 1;
  accu3 >>= 1;
  accu4 = -accu4;

  accu1 = pDat_1[-1];
  accu2 = pDat_1[0];

  *pDat_0++ = accu3;
  *pDat_1-- = accu4;

  
  __asm
  {
    B       dst_IV_loop2_2nd_part
    
    /* 50 cycles for 2 iterations = 25 cycles/iteration */

dst_IV_loop2:

    LDR     val_tw, [twiddle], inc, LSL #2    // val_tw = *twiddle; twiddle += inc
    
    RSB     accu2, accu2, #0                  // accu2 = -accu2
    RSB     accu1, accu1, #0                  // accu1 = -accu1
    SMULWT  accu3, accu2, val_tw              // accu3 = (-accu2)*val_tw.l
    SMULWT  accu4, accu1, val_tw              // accu4 = (-accu1)*val_tw.l
    RSB     accu3, accu3, #0                  // accu3 = -accu2*val_tw.l
    SMLAWB  accu1, accu1, val_tw, accu3       // accu1 = -accu1*val_tw.h-(-accu2)*val_tw.l
    SMLAWB  accu2, accu2, val_tw, accu4       // accu2 = (-accu1)*val_tw.l+(-accu2)*val_tw.h
    STR     accu1, [pDat_1], #-4              // *pDat_1-- = accu1
  	STR     accu2, [pDat_0], #4               // *pDat_0++ = accu2
  	
  	LDR     accu4, [pDat_0]                   // accu4 = pDat_0[0]
  	LDR     accu3, [pDat_0, #4]               // accu3 = pDat_0[1]
  	
    RSB     accu4, accu4, #0                  // accu4 = -accu4
    RSB     accu3, accu3, #0                  // accu3 = -accu3

    SMULWB  accu1, accu3, val_tw              // accu1 = (-accu3)*val_tw.h
    SMULWT  accu2, accu3, val_tw              // accu2 = (-accu3)*val_tw.l
    RSB     accu1, accu1, #0                  // accu1 = -(-accu3)*val_tw.h
    SMLAWT  accu3, accu4, val_tw, accu1       // accu3 = (-accu4)*val_tw.l-(-accu3)*val_tw.h
    SMLAWB  accu4, accu4, val_tw, accu2       // accu4 = (-accu3)*val_tw.l+(-accu4)*val_tw.h

    LDR     accu1, [pDat_1, #-4]              // accu1 = pDat_1[-1]
    LDR     accu2, [pDat_1]                   // accu2 = pDat_1[0]
    
    STR     accu3, [pDat_0], #4               // *pDat_0++ = accu3
    STR     accu4, [pDat_1], #-4              // *pDat_1-- = accu4

dst_IV_loop2_2nd_part:

    LDR     val_tw, [twiddle], inc, LSL #2    // val_tw = *twiddle; twiddle += inc
    
    RSB     accu2, accu2, #0                  // accu2 = -accu2
    RSB     accu1, accu1, #0                  // accu1 = -accu1
    SMULWT  accu3, accu2, val_tw              // accu3 = (-accu2)*val_tw.l
    SMULWT  accu4, accu1, val_tw              // accu4 = (-accu1)*val_tw.l
    RSB     accu3, accu3, #0                  // accu3 = -accu2*val_tw.l
    SMLAWB  accu1, accu1, val_tw, accu3       // accu1 = -accu1*val_tw.h-(-accu2)*val_tw.l
    SMLAWB  accu2, accu2, val_tw, accu4       // accu2 = (-accu1)*val_tw.l+(-accu2)*val_tw.h
    STR     accu1, [pDat_1], #-4              // *pDat_1-- = accu1
  	STR     accu2, [pDat_0], #4               // *pDat_0++ = accu2
  	
  	LDR     accu4, [pDat_0]                   // accu4 = pDat_0[0]
  	LDR     accu3, [pDat_0, #4]               // accu3 = pDat_0[1]
  	
    RSB     accu4, accu4, #0                  // accu4 = -accu4
    RSB     accu3, accu3, #0                  // accu3 = -accu3

    SMULWB  accu1, accu3, val_tw              // accu1 = (-accu3)*val_tw.h
    SMULWT  accu2, accu3, val_tw              // accu2 = (-accu3)*val_tw.l
    RSB     accu1, accu1, #0                  // accu1 = -(-accu3)*val_tw.h
    SMLAWT  accu3, accu4, val_tw, accu1       // accu3 = (-accu4)*val_tw.l-(-accu3)*val_tw.h
    SMLAWB  accu4, accu4, val_tw, accu2       // accu4 = (-accu3)*val_tw.l+(-accu4)*val_tw.h

    LDR     accu1, [pDat_1, #-4]              // accu1 = pDat_1[-1]
    LDR     accu2, [pDat_1]                   // accu2 = pDat_1[0]
    
    STR     accu3, [pDat_0], #4               // *pDat_0++ = accu3
    STR     accu4, [pDat_1], #-4              // *pDat_1-- = accu4

    SUBS    i, i, #1
    BNE     dst_IV_loop2
  }
  
  /* Last Sin and Cos value pair are the same */
  accu1 = fMultDiv2(-accu1, WTC(0x5a82799a));
  accu2 = fMultDiv2(-accu2, WTC(0x5a82799a));

  *pDat_0 = accu1 + accu2;
  *pDat_1 = accu1 - accu2;
}
#endif /* FUNCTION_dst_IV_func2 */
