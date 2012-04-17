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
   Author(s): Arthur Tritthart
   Description: Scaling operations for ARM

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
/* prevent multiple inclusion with re-definitions */
#ifndef __INCLUDE_SCALE_ARM__
#define __INCLUDE_SCALE_ARM__

#define FUNCTION_scaleValuesWithFactor_DBL

SCALE_INLINE
void scaleValuesWithFactor(
        FIXP_DBL *vector,
        FIXP_DBL factor,
        INT len,
        INT scalefactor
        )
{
  /* This code combines the fMult with the scaling             */
  /* It performs a fMultDiv2 and increments shift by 1         */
  int shift = scalefactor + 1;
  FIXP_DBL *mySpec = vector;

  shift = fixmin_I(shift,(INT)DFRACT_BITS-1);

  if (shift >= 0)
  {
    for (int i=0; i<(len>>2); i++)
    {
      FIXP_DBL tmp0 = mySpec[0];
      FIXP_DBL tmp1 = mySpec[1];
      FIXP_DBL tmp2 = mySpec[2];
      FIXP_DBL tmp3 = mySpec[3];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp1 = fMultDiv2(tmp1, factor);
      tmp2 = fMultDiv2(tmp2, factor);
      tmp3 = fMultDiv2(tmp3, factor);
      tmp0 <<= shift;
      tmp1 <<= shift;
      tmp2 <<= shift;
      tmp3 <<= shift;
      *mySpec++ = tmp0;
      *mySpec++ = tmp1;
      *mySpec++ = tmp2;
      *mySpec++ = tmp3;
    }
    for (int i=len&3; i--;)
    {
      FIXP_DBL tmp0 = mySpec[0];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp0 <<= shift;
      *mySpec++ = tmp0;
    }
  }
  else
  {
    shift = -shift;
    for (int i=0; i<(len>>2); i++)
    {
      FIXP_DBL tmp0 = mySpec[0];
      FIXP_DBL tmp1 = mySpec[1];
      FIXP_DBL tmp2 = mySpec[2];
      FIXP_DBL tmp3 = mySpec[3];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp1 = fMultDiv2(tmp1, factor);
      tmp2 = fMultDiv2(tmp2, factor);
      tmp3 = fMultDiv2(tmp3, factor);
      tmp0 >>= shift;
      tmp1 >>= shift;
      tmp2 >>= shift;
      tmp3 >>= shift;
      *mySpec++ = tmp0;
      *mySpec++ = tmp1;
      *mySpec++ = tmp2;
      *mySpec++ = tmp3;
    }
    for (int i=len&3; i--;)
    {
      FIXP_DBL tmp0 = mySpec[0];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp0 >>= shift;
      *mySpec++ = tmp0;
    }
  }
}

#endif /* #ifndef __INCLUDE_SCALE_ARM__ */
