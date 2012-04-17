/***************************  Fraunhofer IIS FDK Tools  ***********************

                        (C) Copyright Fraunhofer IIS (2009)
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
   Author(s):   M. Lohwasser
   Description: auto-correlation functions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "autocorr2nd.h"



/*  If the accumulator does not provide enough overflow bits,
    products have to be shifted down in the autocorrelation below. */
#define SHIFT_FACTOR (5)
#define SHIFT >> (SHIFT_FACTOR)


#if defined(__CC_ARM) || defined(__arm__)
#include "arm/autocorr2nd.cpp"
#endif


/*!
 *
 * \brief Calculate second order autocorrelation using 2 accumulators
 *
 */
#if !defined(FUNCTION_autoCorr2nd_real)
INT
autoCorr2nd_real (ACORR_COEFS *ac,          /*!< Pointer to autocorrelation coeffs */
                  const FIXP_DBL *reBuffer, /*!< Pointer to to real part of input samples */
                  const int len             /*!< Number input samples */
                 )
{
  int   j, autoCorrScaling, mScale;

  FIXP_DBL accu1, accu2, accu3, accu4, accu5;

  const FIXP_DBL *pReBuf;

  const FIXP_DBL *realBuf = reBuffer;

  /*
    r11r,r22r
    r01r,r12r
    r02r
  */
  pReBuf = realBuf-2;
  accu5 = ( (fMultDiv2(pReBuf[0], pReBuf[2]) +
             fMultDiv2(pReBuf[1], pReBuf[3])) SHIFT);
  pReBuf++;

  //len must be even
  accu1 = fPow2Div2(pReBuf[0]) SHIFT;
  accu3 = fMultDiv2(pReBuf[0], pReBuf[1]) SHIFT;
  pReBuf++;

  for ( j = (len - 2)>>1; j != 0; j--,pReBuf+=2 ) {

    accu1 += ( (fPow2Div2(pReBuf[0]) +
                fPow2Div2(pReBuf[1])) SHIFT);

    accu3 += ( (fMultDiv2(pReBuf[0], pReBuf[1]) +
                fMultDiv2(pReBuf[1], pReBuf[2])) SHIFT);

    accu5 += ( (fMultDiv2(pReBuf[0], pReBuf[2]) +
                fMultDiv2(pReBuf[1], pReBuf[3])) SHIFT);

  }

  accu2 = (fPow2Div2(realBuf[-2]) SHIFT);
  accu2 += accu1;

  accu1 += (fPow2Div2(realBuf[len - 2]) SHIFT);

  accu4  = (fMultDiv2(realBuf[-1],realBuf[-2]) SHIFT);
  accu4 += accu3;

  accu3 += (fMultDiv2(realBuf[len - 1],realBuf[len - 2]) SHIFT);

  mScale = CntLeadingZeros( (accu1 | accu2 | fAbs(accu3) | fAbs(accu4) | fAbs(accu5)) ) - 1;
  autoCorrScaling = mScale - 1 - SHIFT_FACTOR; /* -1 because of fMultDiv2*/

  /* Scale to common scale factor */
  ac->r11r = accu1 << mScale;
  ac->r22r = accu2 << mScale;
  ac->r01r = accu3 << mScale;
  ac->r12r = accu4 << mScale;
  ac->r02r = accu5 << mScale;

  ac->det = (fMultDiv2(ac->r11r,ac->r22r) - fMultDiv2(ac->r12r,ac->r12r)) ;
  mScale  = CountLeadingBits(fAbs(ac->det));

  ac->det     <<= mScale;
  ac->det_scale = mScale - 1;

  return autoCorrScaling;
}
#endif

#ifndef LOW_POWER_SBR_ONLY
#if !defined(FUNCTION_autoCorr2nd_cplx)
INT
autoCorr2nd_cplx (ACORR_COEFS *ac,           /*!< Pointer to autocorrelation coeffs */
                  const FIXP_DBL *reBuffer,  /*!< Pointer to real part of input samples */
                  const FIXP_DBL *imBuffer,  /*!< Pointer to imag part of input samples */
                  const int len              /*!< Number of input samples */
                 )
{

  int   j, autoCorrScaling, mScale, len_scale;

  FIXP_DBL accu0, accu1,accu2, accu3, accu4, accu5, accu6, accu7, accu8;

  const FIXP_DBL *pReBuf, *pImBuf;

  const FIXP_DBL *realBuf = reBuffer;
  const FIXP_DBL *imagBuf = imBuffer;

  (len>64) ? (len_scale = 6) : (len_scale = 5);
  /*
    r00r,
    r11r,r22r
    r01r,r12r
    r01i,r12i
    r02r,r02i
  */
  accu1 = accu3 = accu5 = accu7 = accu8 = FL2FXCONST_DBL(0.0f);

  pReBuf  = realBuf-2, pImBuf  = imagBuf-2;
  accu7 += ( (fMultDiv2(pReBuf[2], pReBuf[0]) + fMultDiv2(pImBuf[2], pImBuf[0])) >> len_scale);
  accu8 += ( (fMultDiv2(pImBuf[2], pReBuf[0]) - fMultDiv2(pReBuf[2], pImBuf[0])) >> len_scale);

  pReBuf = realBuf-1, pImBuf = imagBuf-1;
  for ( j = (len - 1); j != 0; j--,pReBuf++,pImBuf++ ){
    accu1 += ( (fPow2Div2(pReBuf[0]           ) + fPow2Div2(pImBuf[0]           )) >> len_scale);
    accu3 += ( (fMultDiv2(pReBuf[0], pReBuf[1]) + fMultDiv2(pImBuf[0], pImBuf[1])) >> len_scale);
    accu5 += ( (fMultDiv2(pImBuf[1], pReBuf[0]) - fMultDiv2(pReBuf[1], pImBuf[0])) >> len_scale);
    accu7 += ( (fMultDiv2(pReBuf[2], pReBuf[0]) + fMultDiv2(pImBuf[2], pImBuf[0])) >> len_scale);
    accu8 += ( (fMultDiv2(pImBuf[2], pReBuf[0]) - fMultDiv2(pReBuf[2], pImBuf[0])) >> len_scale);
  }

  accu2 = ( (fPow2Div2(realBuf[-2]) + fPow2Div2(imagBuf[-2])) >> len_scale);
  accu2 += accu1;

  accu1 += ( (fPow2Div2(realBuf[len-2]) +
              fPow2Div2(imagBuf[len-2])) >> len_scale);
  accu0 = ( (fPow2Div2(realBuf[len-1]) +
             fPow2Div2(imagBuf[len-1])) >> len_scale) -
          ( (fPow2Div2(realBuf[-1]) +
             fPow2Div2(imagBuf[-1])) >> len_scale);
  accu0 += accu1;

  accu4 = ( (fMultDiv2(realBuf[-1], realBuf[-2]) +
             fMultDiv2(imagBuf[-1], imagBuf[-2])) >> len_scale);
  accu4 += accu3;

  accu3 += ( (fMultDiv2(realBuf[len-1], realBuf[len-2]) +
              fMultDiv2(imagBuf[len-1], imagBuf[len-2])) >> len_scale);

  accu6 = ( (fMultDiv2(imagBuf[-1], realBuf[-2]) -
             fMultDiv2(realBuf[-1], imagBuf[-2])) >> len_scale);
  accu6 += accu5;

  accu5 += ( (fMultDiv2(imagBuf[len - 1], realBuf[len - 2]) -
              fMultDiv2(realBuf[len - 1], imagBuf[len - 2])) >> len_scale);

  mScale = CntLeadingZeros( (accu0 | accu1 | accu2 | fAbs(accu3) | fAbs(accu4) | fAbs(accu5) |
                             fAbs(accu6) | fAbs(accu7) | fAbs(accu8)) ) - 1;
  autoCorrScaling = mScale - 1 - len_scale; /* -1 because of fMultDiv2*/

  /* Scale to common scale factor */
  ac->r00r = (FIXP_DBL)accu0 << mScale;
  ac->r11r = (FIXP_DBL)accu1 << mScale;
  ac->r22r = (FIXP_DBL)accu2 << mScale;
  ac->r01r = (FIXP_DBL)accu3 << mScale;
  ac->r12r = (FIXP_DBL)accu4 << mScale;
  ac->r01i = (FIXP_DBL)accu5 << mScale;
  ac->r12i = (FIXP_DBL)accu6 << mScale;
  ac->r02r = (FIXP_DBL)accu7 << mScale;
  ac->r02i = (FIXP_DBL)accu8 << mScale;

  ac->det = ( fMultDiv2(ac->r11r,ac->r22r) >> 1 ) -
            ( (fMultDiv2(ac->r12r,ac->r12r) + fMultDiv2(ac->r12i,ac->r12i)) >> 1 );
  mScale = CountLeadingBits(fAbs(ac->det));

  ac->det <<= mScale;
  ac->det_scale = mScale - 2;

  return autoCorrScaling;
}
#endif /* FUNCTION_autoCorr2nd_cplx */
#endif /* LOW_POWER_SBR_ONLY */


