/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2007)
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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#if defined(__mips_dsp)

#ifndef FUNCTION_getScalefactor_DBL
#define FUNCTION_getScalefactor_DBL
/*!
 *
 *  \brief Calculate max possible scale factor for input vector
 *
 *  \return Maximum scale factor
 *
 *  This function can constitute a significant amount of computational complexity - very much depending on the
 *  bitrate. Since it is a rather small function, effective assembler optimization might be possible.
 *
 */
inline INT getScalefactor(const FIXP_DBL *vector, /*!< Pointer to input vector */
                          INT len)                /*!< Length of input vector */
{
  INT i;
  FIXP_DBL maxVal = FL2FX_DBL(0.0f);

  for(i=len;i!=0;i--){
    maxVal |= __builtin_mips_absq_s_w(*vector++);
  }

  return fixMax((INT)0,(CntLeadingZeros(maxVal) - 1));
}
#endif

#endif /*__mips_dsp */
