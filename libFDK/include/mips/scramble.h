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

#ifndef MIPS_SCRAMBLE_H
#define MIPS_SCRAMBLE_H

#define FUNCTION_scramble

#if defined(FUNCTION_scramble)
inline void scramble(FIXP_DBL *x, INT n) {
  INT m,j;
  int ldn=1;
  do {ldn++;} while((1<<ldn)<n);

  for (m=1,j=0; m<n-1; m++)
  {
    j = __builtin_mips_bitrev(m) >> (16-ldn);

    if (j>m)
    {
      FIXP_DBL tmp;
      tmp=x[2*m];
      x[2*m]=x[2*j];
      x[2*j]=tmp;

      tmp=x[2*m+1];
      x[2*m+1]=x[2*j+1];
      x[2*j+1]=tmp;
    }
  }
}
#endif

#endif /* MIPS_SCRAMBLE_H */
