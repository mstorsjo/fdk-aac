/********************************  Fraunhofer IIS  ***************************

                        (C) Copyright Fraunhofer IIS (2011)
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
   Description: (ARM optimised) LPP transposer subroutines

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/


#if defined(__arm__)


#define FUNCTION_LPPTRANSPOSER_func1

#ifdef FUNCTION_LPPTRANSPOSER_func1

/* Note: This code requires only 43 cycles per iteration instead of 61 on ARM926EJ-S */
__attribute__ ((noinline)) static void lppTransposer_func1(
  FIXP_DBL *lowBandReal,
  FIXP_DBL *lowBandImag,
  FIXP_DBL **qmfBufferReal,
  FIXP_DBL **qmfBufferImag,
  int loops,
  int hiBand,
  int dynamicScale,
  int descale,
  FIXP_SGL a0r,
  FIXP_SGL a0i,
  FIXP_SGL a1r,
  FIXP_SGL a1i)
{

  FIXP_DBL real1, real2, imag1, imag2, accu1, accu2;

  real2 = lowBandReal[-2];
  real1 = lowBandReal[-1];
  imag2 = lowBandImag[-2];
  imag1 = lowBandImag[-1];
  for(int i=0; i < loops; i++)
  {
    accu1 = fMultDiv2(         a0r,real1);
    accu2 = fMultDiv2(         a0i,imag1);
    accu1 = fMultAddDiv2(accu1,a1r,real2);
    accu2 = fMultAddDiv2(accu2,a1i,imag2);
    real2 = fMultDiv2(         a1i,real2);
    accu1 = accu1 - accu2;
    accu1 = accu1 >> dynamicScale;

    accu2 = fMultAddDiv2(real2,a1r,imag2);
    real2 = real1;
    imag2 = imag1;
    accu2 = fMultAddDiv2(accu2,a0i,real1);
    real1 = lowBandReal[i];
    accu2 = fMultAddDiv2(accu2,a0r,imag1);
    imag1 = lowBandImag[i];
    accu2 = accu2 >> dynamicScale;

    accu1 <<= 1;
    accu2 <<= 1;

    qmfBufferReal[i][hiBand] = accu1 + (real1>>descale);
    qmfBufferImag[i][hiBand] = accu2 + (imag1>>descale);
  }
}
#endif  /* #ifdef FUNCTION_LPPTRANSPOSER_func1 */
#endif  /* __arm__ */



