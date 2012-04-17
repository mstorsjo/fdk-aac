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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef SCRAMBLE_H
#define SCRAMBLE_H

#include "common_fix.h"

#if defined(__arm__)
#include "arm/scramble.h"

#elif defined(__mips__) && defined(__mips_dsp)	/* cppp replaced: elif */
#include "mips/scramble.h"

#endif

/*****************************************************************************

    functionname: scramble
    description:  bitreversal of input data
    returns:
    input:
    output:

*****************************************************************************/
#if !defined(FUNCTION_scramble_sgl)

inline void scramble_sgl(FIXP_SGL *x, INT n)
{
  INT m,k,j;

  for (m=1,j=0; m<n-1; m++)
  {
    {for(k=n>>1; (!((j^=k)&k)); k>>=1);}

    if (j>m)
    {
      FIXP_SGL tmp;
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

#if !defined(FUNCTION_scramble)

/* default scramble functionality */
inline void scramble(FIXP_DBL *x, INT n)
{
  INT m,k,j;
  FDK_ASSERT(!(((UINT64)x)&(ALIGNMENT_DEFAULT-1)));

  for (m=1,j=0; m<n-1; m++)
  {
    {for(k=n>>1; (!((j^=k)&k)); k>>=1);}

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
#endif /* !defined(FUNCTION_scramble) */

#endif /* SCRAMBLE_H */
