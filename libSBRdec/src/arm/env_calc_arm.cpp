/********************************  Fraunhofer IIS  ***************************

                        (C) Copyright Fraunhofer IIS (2010)
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
   Author(s):   Arthur Tritthart
   Description: (ARM optimised) SBR domain coding

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef INCLUSION_GUARD_CALC_ENV_ARM
#define INCLUSION_GUARD_CALC_ENV_ARM


/*!
  \brief   Compute maximal value of a complex array (re/im) of a given width
           Negative values are temporarily logically or'ed with 0xFFFFFFFF
           instead of negating the value, if the sign bit is set.
  \param   maxVal  Preset maximal value
  \param   reTmp   real input signal
  \param   imTmp   imaginary input signal
  \return  new maximal value
*/

#ifdef FUNCTION_FDK_get_maxval
__asm FIXP_DBL FDK_get_maxval (FIXP_DBL maxVal, FIXP_DBL *reTmp, FIXP_DBL *imTmp, int width )
{

  /* Register map:
     r0    maxVal
     r1    reTmp
     r2    imTmp
     r3    width
     r4    real
     r5    imag
  */
    PUSH    {r4-r5}

    MOVS    r3, r3, ASR #1
    ADC     r3, r3, #0
    BCS     FDK_get_maxval_loop_2nd_part
    BEQ     FDK_get_maxval_loop_end

FDK_get_maxval_loop
    LDR     r4, [r1], #4
    LDR     r5, [r2], #4
    EOR     r4, r4, r4, ASR #31
    EOR     r5, r5, r5, ASR #31
    ORR     r0, r0, r4
    ORR     r0, r0, r5

FDK_get_maxval_loop_2nd_part
    LDR     r4, [r1], #4
    LDR     r5, [r2], #4
    EOR     r4, r4, r4, ASR #31
    EOR     r5, r5, r5, ASR #31
    ORR     r0, r0, r4
    ORR     r0, r0, r5

    SUBS    r3, r3, #1
    BNE     FDK_get_maxval_loop

FDK_get_maxval_loop_end
    POP     {r4-r5}
    BX      lr
}
#endif /* FUNCTION_FDK_get_maxval */

#endif /* INCLUSION_GUARD_CALC_ENV_ARM */
