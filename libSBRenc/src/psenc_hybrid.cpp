/*****************************  MPEG Audio Encoder  ***************************

                     (C) Copyright Fraunhofer IIS (2004-2005)
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
   Initial author:       M. Neuendorf, M. Multrus
   contents/description: hybrid analysis filter bank

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "psenc_hybrid.h"


/* Includes ******************************************************************/

#include "psenc_hybrid.h"
#include "sbr_ram.h"

#include "fft.h"

#include "genericStds.h"

/* Defines *******************************************************************/

#define HYBRID_SCALE                    4

/*//#define FAST_FILTER2
#define FAST_FILTER4
#define FAST_FILTER8
#define FAST_FILTER12
*/
#define HYBRID_INVERSE_ORDER            ( 0x0F000000 )
#define HYBRID_INVERSE_MASK             ( ~HYBRID_INVERSE_ORDER )

//#define REAL                            ( 0 )
//#define CPLX                            ( 1 )

#define cos0Pi   FL2FXCONST_DBL( 1.f)
#define sin0Pi   FL2FXCONST_DBL( 0.f)
#define cos1Pi   FL2FXCONST_DBL(-1.f)
#define sin1Pi   FL2FXCONST_DBL( 0.f)
#define cos1Pi_2 FL2FXCONST_DBL( 0.f)
#define sin1Pi_2 FL2FXCONST_DBL( 1.f)
#define cos1Pi_3 FL2FXCONST_DBL( 0.5f)
#define sin1Pi_3 FL2FXCONST_DBL( 0.86602540378444f)

#define cos0Pi_4  cos0Pi
#define cos1Pi_4  FL2FXCONST_DBL(0.70710678118655f)
#define cos2Pi_4  cos1Pi_2
#define cos3Pi_4  (-cos1Pi_4)
#define cos4Pi_4  (-cos0Pi_4)
#define cos5Pi_4  cos3Pi_4
#define cos6Pi_4  cos2Pi_4

#define sin0Pi_4  sin0Pi
#define sin1Pi_4  FL2FXCONST_DBL(0.70710678118655f)
#define sin2Pi_4  sin1Pi_2
#define sin3Pi_4  sin1Pi_4
#define sin4Pi_4  sin0Pi_4
#define sin5Pi_4  (-sin3Pi_4)
#define sin6Pi_4  (-sin2Pi_4)

#define cos0Pi_8  cos0Pi
#define cos1Pi_8  FL2FXCONST_DBL(0.92387953251129f)
#define cos2Pi_8  cos1Pi_4
#define cos3Pi_8  FL2FXCONST_DBL(0.38268343236509f)
#define cos4Pi_8  cos2Pi_4
#define cos5Pi_8  (-cos3Pi_8)
#define cos6Pi_8  (-cos2Pi_8)

#define sin0Pi_8 sin0Pi
#define sin1Pi_8 cos3Pi_8
#define sin2Pi_8 sin1Pi_4
#define sin3Pi_8 cos1Pi_8
#define sin4Pi_8 sin2Pi_4
#define sin5Pi_8 sin3Pi_8
#define sin6Pi_8 sin1Pi_4

#define cos0Pi_12 cos0Pi
#define cos1Pi_12 FL2FXCONST_DBL(0.96592582628906f)
#define cos2Pi_12 FL2FXCONST_DBL(0.86602540378444f)
#define cos3Pi_12 cos1Pi_4
#define cos4Pi_12 cos1Pi_3
#define cos5Pi_12 FL2FXCONST_DBL(0.25881904510252f)
#define cos6Pi_12 cos1Pi_2

#define sin0Pi_12 sin0Pi
#define sin1Pi_12 cos5Pi_12
#define sin2Pi_12 cos4Pi_12
#define sin3Pi_12 sin1Pi_4
#define sin4Pi_12 sin1Pi_3
#define sin5Pi_12 cos1Pi_12
#define sin6Pi_12 sin1Pi_2

#define FFT_IDX_R(a)  (2*a)
#define FFT_IDX_I(a)  (2*a+1)


/* Constants *****************************************************************/

/* static const UINT noQmfBandsInHybrid34 = 5; */

static const INT aHybridResolution10[] = { HYBRID_6_CPLX,
                                           HYBRID_2_REAL | HYBRID_INVERSE_ORDER,
                                           HYBRID_2_REAL };

static const INT aHybridResolution20[] = { HYBRID_6_CPLX,
                                           HYBRID_2_REAL | HYBRID_INVERSE_ORDER,
                                           HYBRID_2_REAL };

/*static const INT aHybridResolution34[] = { HYBRID_12_CPLX,
                                           HYBRID_8_CPLX,
                                           HYBRID_4_CPLX,
                                           HYBRID_4_CPLX,
                                           HYBRID_4_CPLX };*/

static const FIXP_DBL p8_13_20[HYBRID_FILTER_LENGTH] =
{
  FL2FXCONST_DBL(0.00746082949812f),  FL2FXCONST_DBL(0.02270420949825f),  FL2FXCONST_DBL(0.04546865930473f),  FL2FXCONST_DBL(0.07266113929591f),
  FL2FXCONST_DBL(0.09885108575264f),  FL2FXCONST_DBL(0.11793710567217f),  FL2FXCONST_DBL(0.125f           ),  FL2FXCONST_DBL(0.11793710567217f),
  FL2FXCONST_DBL(0.09885108575264f),  FL2FXCONST_DBL(0.07266113929591f),  FL2FXCONST_DBL(0.04546865930473f),  FL2FXCONST_DBL(0.02270420949825f),
  FL2FXCONST_DBL(0.00746082949812f)
};

static const FIXP_DBL p2_13_20[HYBRID_FILTER_LENGTH] =
{
  FL2FXCONST_DBL(0.0f), FL2FXCONST_DBL( 0.01899487526049f),  FL2FXCONST_DBL(0.0f), FL2FXCONST_DBL(-0.07293139167538f),
  FL2FXCONST_DBL(0.0f), FL2FXCONST_DBL( 0.30596630545168f),  FL2FXCONST_DBL(0.5f), FL2FXCONST_DBL( 0.30596630545168f),
  FL2FXCONST_DBL(0.0f), FL2FXCONST_DBL(-0.07293139167538f),  FL2FXCONST_DBL(0.0f), FL2FXCONST_DBL( 0.01899487526049f),
  FL2FXCONST_DBL(0.0f)
};


/*static const float p12_13_34[HYBRID_FILTER_LENGTH] =
{
  0.04081179924692,  0.03812810994926,  0.05144908135699,  0.06399831151592,
  0.07428313801106,  0.08100347892914,  0.08333333333333,  0.08100347892914,
  0.07428313801106,  0.06399831151592,  0.05144908135699,  0.03812810994926,
  0.04081179924692
};

static const float p8_13_34[HYBRID_FILTER_LENGTH] =
{
  0.01565675600122,  0.03752716391991,  0.05417891378782,  0.08417044116767,
  0.10307344158036,  0.12222452249753,  0.12500000000000,  0.12222452249753,
  0.10307344158036,  0.08417044116767,  0.05417891378782,  0.03752716391991,
  0.01565675600122
};

static const float p4_13_34[HYBRID_FILTER_LENGTH] =
{
 -0.05908211155639, -0.04871498374946,  0.0,               0.07778723915851,
  0.16486303567403,  0.23279856662996,  0.25,              0.23279856662996,
  0.16486303567403,  0.07778723915851,  0.0,              -0.04871498374946,
 -0.05908211155639
};*/


/* Function / Class Implementation *******************************************/



/*****************************************************************************/
/* **** FILTERBANK CONFIG **** */

HANDLE_ERROR_INFO FDKsbrEnc_CreateHybridConfig(HANDLE_PS_HYBRID_CONFIG *phHybConfig,
                                               PS_BANDS mode)
{
  HANDLE_ERROR_INFO error = noError;
  HANDLE_PS_HYBRID_CONFIG h = NULL;
  UINT k = 0;

  if (error == noError) {
    h = *phHybConfig; /* Simplify your life */
    h->mode = mode;

    switch (mode) {
    case PS_BANDS_MID:
      h->noQmfBandsInHybrid = NO_QMF_BANDS_HYBRID_20;
      for (k=0; k<h->noQmfBandsInHybrid; k++) {
        h->aHybridResolution[k] = aHybridResolution20[k];
      }
      break;

    case PS_BANDS_FINE:
      /*h->noQmfBandsInHybrid = noQmfBandsInHybrid34;
      for (k=0; k<h->noQmfBandsInHybrid; k++) {
        h->aHybridResolution[k] = aHybridResolution34[k];
      }*/
      FDK_ASSERT(0); /* we don't support! */
      break;

    case PS_BANDS_COARSE:
      h->noQmfBandsInHybrid = NO_QMF_BANDS_HYBRID_10;
      for (k=0; k<h->noQmfBandsInHybrid; k++) {
        h->aHybridResolution[k] = aHybridResolution10[k];
      }
      break;

    default:
      error = ERROR(CDI, "Invalid hybrid filter bank configuration.");
      break;
    }
  }

  return error;
}

/*****************************************************************************/
/* **** FILTERBANK DATA **** */

HANDLE_ERROR_INFO FDKsbrEnc_CreateHybridData(HANDLE_PS_HYBRID_DATA *phHybData,
                                   INT ch)
{
  HANDLE_ERROR_INFO error = noError;
  int k;

  HANDLE_PS_HYBRID_DATA hHybData = GetRam_HybData(ch);
  if (hHybData==NULL) {
    error = 1;
    goto bail;
  }

  FDKmemclear(hHybData, sizeof(PS_HYBRID_DATA));

  hHybData->rHybData[0] = GetRam_PsRhyb(ch);
  hHybData->iHybData[0] = GetRam_PsIhyb(ch);
  if ( (hHybData->rHybData[0]==NULL) || (hHybData->iHybData[0]==NULL) ) {
    error = 1;
    goto bail;
  }



  for (k=1; k<(HYBRID_FRAMESIZE+HYBRID_WRITEOFFSET); k++) {
    hHybData->rHybData[k] = hHybData->rHybData[0] + (k*HYBRID_NUM_BANDS);
    hHybData->iHybData[k] = hHybData->iHybData[0] + (k*HYBRID_NUM_BANDS);
  }

bail:
  *phHybData = hHybData;
  return error;
}


HANDLE_ERROR_INFO FDKsbrEnc_InitHybridData(HANDLE_PS_HYBRID_DATA hHybData,
                                   HANDLE_PS_HYBRID_CONFIG  hHybConfig,
                                   INT frameSize)
{
  HANDLE_ERROR_INFO error = noError;
  INT nHybridBands = 0;
  INT k = 0;
  INT noBands = 0;
  const INT *hybridRes = NULL;

  if (hHybConfig != NULL) {
    noBands = hHybConfig->noQmfBandsInHybrid;
    hybridRes = hHybConfig->aHybridResolution;
  }

  for (k=0; k<noBands; k++) {
    nHybridBands += (hybridRes[k] & HYBRID_INVERSE_MASK);
  }
  FDK_ASSERT (HYBRID_NUM_BANDS>=nHybridBands);

  hHybData->hybDataReadOffset = HYBRID_DATA_READ_OFFSET;
  hHybData->hybDataWriteOffset = HYBRID_WRITEOFFSET;

  for (k=0; k<(HYBRID_FRAMESIZE+HYBRID_WRITEOFFSET); k++) {
    FDKmemclear(hHybData->rHybData[k], sizeof(FIXP_QMF)*HYBRID_NUM_BANDS);
    FDKmemclear(hHybData->iHybData[k], sizeof(FIXP_QMF)*HYBRID_NUM_BANDS);
  }

  hHybData->frameSize       = frameSize;
  hHybData->nHybridBands    = nHybridBands;
  hHybData->nHybridQmfBands = noBands;

  /* store hybrid resoltion in hybrid data handle */
  FDK_ASSERT (HYBRID_MAX_QMF_BANDS>=hHybData->nHybridQmfBands);
  for(k = 0; k<hHybData->nHybridQmfBands; k++){
    hHybData->nHybridResolution[k] = (hybridRes[k] & HYBRID_INVERSE_MASK);
  }

  return error;
}

HANDLE_ERROR_INFO FDKsbrEnc_DestroyHybridData(HANDLE_PS_HYBRID_DATA* phHybData)
{
  HANDLE_PS_HYBRID_DATA hHybData = *phHybData;

  if (hHybData!=NULL) {
    FreeRam_PsRhyb(&hHybData->rHybData[0]);
    FreeRam_PsIhyb(&hHybData->iHybData[0]);
    FreeRam_HybData(phHybData);
  }

  return noError;
}

/*** Access functions ***/

/* return hybrid band resolution of qmf band 'qmfBand' */
INT FDKsbrEnc_GetHybridResolution(HANDLE_PS_HYBRID_DATA h, INT qmfBand){

  INT nHybridResolution = 0;

  if(h->nHybridResolution){
    nHybridResolution = h->nHybridResolution[qmfBand];
  }

  return nHybridResolution;
}

/*****************************************************************************/
/* **** FILTERBANK **** */

/*
   2 channel filter
   Filter Coefs:
   0.0,
   0.01899487526049,
   0.0,
   -0.07293139167538,
   0.0,
   0.30596630545168,
   0.5,
   0.30596630545168,
   0.0,
   -0.07293139167538,
   0.0,
   0.01899487526049,
   0.0


   Filter design:
   h[q,n] = g[n] * cos(2pi/2 * q * (n-6) );  n = 0..12,  q = 0,1;

   ->  h[0,n] = g[n] * 1;
   ->  h[1,n] = g[n] * pow(-1,n);

*/

static void dualChannelFiltering(const FIXP_QMF *RESTRICT pQmfReal,
                                 const FIXP_QMF *RESTRICT pQmfImag,
                                 FIXP_QMF **RESTRICT mHybridReal,
                                 FIXP_QMF **RESTRICT mHybridImag,
                                 INT nSamples)
{
  INT i;

  for(i = 0; i < nSamples; i++) {
    FIXP_DBL  r1, r3, r5, r6;
    FIXP_DBL  i1, i3, i5, i6;

    /* symmetric filter coefficients
       scaleValue same as used in eightChannelFiltering (HYBRID_SCALE = 4)
    */
    r1 = fMultDiv2(p2_13_20[1], (FIXP_QMF)((pQmfReal[1+i]>>1) + (pQmfReal[11+i]>>1)) ) >> 2;
    r3 = fMultDiv2(p2_13_20[3], (FIXP_QMF)((pQmfReal[3+i]>>1) + (pQmfReal[ 9+i]>>1)) ) >> 2;
    r5 = fMultDiv2(p2_13_20[5], (FIXP_QMF)((pQmfReal[5+i]>>1) + (pQmfReal[ 7+i]>>1)) ) >> 2;
    r6 = fMultDiv2(p2_13_20[6], (FIXP_QMF) (pQmfReal[6+i]>>1) ) >> 2;

    i1 = fMultDiv2(p2_13_20[1], (FIXP_QMF)((pQmfImag[1+i]>>1) + (pQmfImag[11+i]>>1)) ) >> 2;
    i3 = fMultDiv2(p2_13_20[3], (FIXP_QMF)((pQmfImag[3+i]>>1) + (pQmfImag[ 9+i]>>1)) ) >> 2;
    i5 = fMultDiv2(p2_13_20[5], (FIXP_QMF)((pQmfImag[5+i]>>1) + (pQmfImag[ 7+i]>>1)) ) >> 2;
    i6 = fMultDiv2(p2_13_20[6], (FIXP_QMF) (pQmfImag[6+i]>>1) ) >> 2;

    mHybridReal[i][0] = FX_DBL2FX_QMF(r1 + r3 + r5 + r6);
    mHybridImag[i][0] = FX_DBL2FX_QMF(i1 + i3 + i5 + i6);

    mHybridReal[i][1] = FX_DBL2FX_QMF(- r1 - r3 - r5 + r6);
    mHybridImag[i][1] = FX_DBL2FX_QMF(- i1 - i3 - i5 + i6);
  }
}

/*
   8 channel filter

   Implementation using a FFT of length 8

   prototype filter coefficients:
   0.00746082949812   0.02270420949825   0.04546865930473   0.07266113929591   0.09885108575264   0.11793710567217
   0.125
   0.11793710567217   0.09885108575264   0.07266113929591   0.04546865930473   0.02270420949825   0.00746082949812

   Filter design:
   N = 13; Q = 8;
   h[q,n]       = g[n] * exp(j * 2 * pi / Q * (q + .5) * (n - 6));  n = 0..(N-1),  q = 0..(Q-1);

   Time Signal:   x[t];
   Filter Bank Output
   y[q,t] = conv(x[t],h[q,t]) = conv(h[q,t],x[t]) = sum(x[k] * h[q, t - k] ) = sum(h[q, k] * x[t - k] ); k = 0..(N-1);

   y[q,t] =   x[t - 12]*h[q, 12]  +  x[t - 11]*h[q, 11]  +  x[t - 10]*h[q, 10]  +  x[t -  9]*h[q,  9]
           +  x[t -  8]*h[q,  8]  +  x[t -  7]*h[q,  7]
           +  x[t -  6]*h[q,  6]
           +  x[t -  5]*h[q,  5]  +  x[t -  4]*h[q,  4]
           +  x[t -  3]*h[q,  3]  +  x[t -  2]*h[q,  2]  +  x[t -  1]*h[q,  1]  +  x[t -  0]*h[q,  0];

   h'[q, n] = h[q,(N-1)-n] = g[n] * exp(j * 2 * pi / Q * (q + .5) * (6 - n));  n = 0..(N-1),  q = 0..(Q-1);

   y[q,t] =   x[t - 12]*h'[q,  0]  +  x[t - 11]*h'[q,  1]  +  x[t - 10]*h'[q,  2]  +  x[t -  9]*h'[q,  3]
           +  x[t -  8]*h'[q,  4]  +  x[t -  7]*h'[q,  5]
           +  x[t -  6]*h'[q,  6]
           +  x[t -  5]*h'[q,  7]  +  x[t -  4]*h'[q,  8]
           +  x[t -  3]*h'[q,  9]  +  x[t -  2]*h'[q, 10]  +  x[t -  1]*h'[q, 11]  +  x[t -  0]*h'[q, 12];

   Try to split off FFT Modulation Term:
   FFT(x[t], q) = sum(x[t+k]*exp(-j*2*pi/N *q * k))
                                           c                                           m
   Step 1:  h'[q,n] = g[n] * ( exp(j * 2 * pi / 8 * .5 * (6 - n)) ) * ( exp (j * 2 * pi / 8 * q * (6 - n)) );

    h'[q,n] = g[n] *c[n] * m[q,n]; (see above)
    c[n]    = exp( j * 2 * pi / 8 * .5 * (6 - n) );
    m[q,n]  = exp( j * 2 * pi / 8 *  q * (6 - n) );

    y[q,t] = x[t -  0]*g[0]*c[0]*m[q,0]  +  x[t -  1]*g[1]*c[ 1]*m[q, 1]  + ...
             ...                         +  x[t - 12]*g[2]*c[12]*m[q,12];

                                                                              |
    n                   m                            *exp(-j*2*pi)            |   n'                   fft
-------------------------------------------------------------------------------------------------------------------------
    0       exp( j * 2 * pi / 8 * q * 6) ->  exp(-j * 2 * pi / 8 * q * 2)     |   2         exp(-j * 2 * pi / 8 * q * 0)
    1       exp( j * 2 * pi / 8 * q * 5) ->  exp(-j * 2 * pi / 8 * q * 3)     |   3         exp(-j * 2 * pi / 8 * q * 1)
    2       exp( j * 2 * pi / 8 * q * 4) ->  exp(-j * 2 * pi / 8 * q * 4)     |   4         exp(-j * 2 * pi / 8 * q * 2)
    3       exp( j * 2 * pi / 8 * q * 3) ->  exp(-j * 2 * pi / 8 * q * 5)     |   5         exp(-j * 2 * pi / 8 * q * 3)
    4       exp( j * 2 * pi / 8 * q * 2) ->  exp(-j * 2 * pi / 8 * q * 6)     |   6         exp(-j * 2 * pi / 8 * q * 4)
    5       exp( j * 2 * pi / 8 * q * 1) ->  exp(-j * 2 * pi / 8 * q * 7)     |   7         exp(-j * 2 * pi / 8 * q * 5)
    6       exp( j * 2 * pi / 8 * q * 0)                                      |   0         exp(-j * 2 * pi / 8 * q * 6)
    7       exp(-j * 2 * pi / 8 * q * 1)                                      |   1         exp(-j * 2 * pi / 8 * q * 7)
    8       exp(-j * 2 * pi / 8 * q * 2)                                      |   2
    9       exp(-j * 2 * pi / 8 * q * 3)                                      |   3
    10      exp(-j * 2 * pi / 8 * q * 4)                                      |   4
    11      exp(-j * 2 * pi / 8 * q * 5)                                      |   5
    12      exp(-j * 2 * pi / 8 * q * 6)                                      |   6


    now use fft modulation coefficients
    m[6]  =       = fft[0]
    m[7]  =       = fft[1]
    m[8]  = m[ 0] = fft[2]
    m[9]  = m[ 1] = fft[3]
    m[10] = m[ 2] = fft[4]
    m[11] = m[ 3] = fft[5]
    m[12] = m[ 4] = fft[6]
            m[ 5] = fft[7]

    y[q,t] = (                       x[t- 6]*g[ 6]*c[ 6] ) * fft[q,0]  +
             (                       x[t- 7]*g[ 7]*c[ 7] ) * fft[q,1]  +
             ( x[t- 0]*g[ 0]*c[ 0] + x[t- 8]*g[ 8]*c[ 8] ) * fft[q,2]  +
             ( x[t- 1]*g[ 1]*c[ 1] + x[t- 9]*g[ 9]*c[ 9] ) * fft[q,3]  +
             ( x[t- 2]*g[ 2]*c[ 2] + x[t-10]*g[10]*c[10] ) * fft[q,4]  +
             ( x[t- 3]*g[ 3]*c[ 3] + x[t-11]*g[11]*c[11] ) * fft[q,5]  +
             ( x[t- 4]*g[ 4]*c[ 4] + x[t-12]*g[12]*c[12] ) * fft[q,6]  +
             ( x[t- 5]*g[ 5]*c[ 5]                       ) * fft[q,7];

    pre twiddle factors c[n] = exp(j * 2 * pi / 8 * .5 * (6 - n));
    n                c]           |  n                c[n]         |  n                c[n]
---------------------------------------------------------------------------------------------------
    0       exp( j * 6 * pi / 8)  |  1       exp( j * 5 * pi / 8)  |  2       exp( j * 4 * pi / 8)
    3       exp( j * 3 * pi / 8)  |  4       exp( j * 2 * pi / 8)  |  5       exp( j * 1 * pi / 8)
    6       exp( j * 0 * pi / 8)  |  7       exp(-j * 1 * pi / 8)  |  8       exp(-j * 2 * pi / 8)
    9       exp(-j * 3 * pi / 8)  | 10       exp(-j * 4 * pi / 8)  | 11       exp(-j * 5 * pi / 8)
   12       exp(-j * 6 * pi / 8)  |                                |

*/

static const FIXP_DBL  cr[13] =
{  cos6Pi_8, cos5Pi_8, cos4Pi_8,
   cos3Pi_8, cos2Pi_8, cos1Pi_8,
   cos0Pi_8,
   cos1Pi_8, cos2Pi_8, cos3Pi_8,
   cos4Pi_8, cos5Pi_8, cos6Pi_8
};

static const FIXP_DBL  ci[13] =
{
  sin6Pi_8, sin5Pi_8, sin4Pi_8,
  sin3Pi_8, sin2Pi_8, sin1Pi_8,
  sin0Pi_8,
  -sin1Pi_8,  -sin2Pi_8,  -sin3Pi_8,
  -sin4Pi_8,  -sin5Pi_8,  -sin6Pi_8
};


static void eightChannelFiltering(const FIXP_QMF *pQmfReal,
                                  const FIXP_QMF *pQmfImag,
                                  FIXP_DBL *fft,
                                  FIXP_QMF **mHybridReal,
                                  FIXP_QMF **mHybridImag,
                                  INT nSamples,
                                  const FIXP_DBL *p)
{
  INT i, bin;
  for(i = 0; i < nSamples; i++) {
    /* pre twiddeling
       scaling 4 =  2 (fMultDiv2) + 2 (dit_fft) scaling (HYBRID_SCALE = 4)
    */
    fft[FFT_IDX_R(0)] = fMultDiv2(p[6],  fMultSubDiv2(fMultDiv2(cr[6], pQmfReal[6+i]), ci[6], pQmfImag[6+i]));
    fft[FFT_IDX_I(0)] = fMultDiv2(p[6],  fMultAddDiv2(fMultDiv2(ci[6], pQmfReal[6+i]), cr[6], pQmfImag[6+i]));

    fft[FFT_IDX_R(1)] = fMultDiv2(p[7],  fMultSubDiv2(fMultDiv2(cr[7], pQmfReal[7+i]), ci[7], pQmfImag[7+i]));
    fft[FFT_IDX_I(1)] = fMultDiv2(p[7],  fMultAddDiv2(fMultDiv2(ci[7], pQmfReal[7+i]), cr[7], pQmfImag[7+i]));

    fft[FFT_IDX_R(2)] = ( fMultDiv2(p[ 0], fMultSubDiv2(fMultDiv2(cr[0], pQmfReal[ 0+i]), ci[0], pQmfImag[ 0+i]))+
                          fMultDiv2(p[ 8], fMultSubDiv2(fMultDiv2(cr[8], pQmfReal[ 8+i]), ci[8], pQmfImag[ 8+i])) );
    fft[FFT_IDX_I(2)] = ( fMultDiv2(p[ 0], fMultAddDiv2(fMultDiv2(ci[0], pQmfReal[ 0+i]), cr[0], pQmfImag[ 0+i]))+
                          fMultDiv2(p[ 8], fMultAddDiv2(fMultDiv2(ci[8], pQmfReal[ 8+i]), cr[8], pQmfImag[ 8+i])) );

    fft[FFT_IDX_R(3)] = ( fMultDiv2(p[ 1], fMultSubDiv2(fMultDiv2(cr[1], pQmfReal[ 1+i]), ci[1], pQmfImag[ 1+i]))+
                          fMultDiv2(p[ 9], fMultSubDiv2(fMultDiv2(cr[9], pQmfReal[ 9+i]), ci[9], pQmfImag[ 9+i])) );
    fft[FFT_IDX_I(3)] = ( fMultDiv2(p[ 1], fMultAddDiv2(fMultDiv2(ci[1], pQmfReal[ 1+i]), cr[1], pQmfImag[ 1+i]))+
                          fMultDiv2(p[ 9], fMultAddDiv2(fMultDiv2(ci[9], pQmfReal[ 9+i]), cr[9], pQmfImag[ 9+i])) );

    fft[FFT_IDX_R(4)] = ( fMultDiv2(p[ 2], fMultSubDiv2( fMultDiv2(cr[2], pQmfReal[ 2+i]), ci[2], pQmfImag[ 2+i]))+
                          fMultDiv2(p[10], fMultSubDiv2(fMultDiv2(cr[10], pQmfReal[10+i]), ci[10], pQmfImag[10+i])) );
    fft[FFT_IDX_I(4)] = ( fMultDiv2(p[ 2], fMultAddDiv2( fMultDiv2(ci[2], pQmfReal[ 2+i]), cr[2], pQmfImag[ 2+i]))+
                          fMultDiv2(p[10], fMultAddDiv2(fMultDiv2(ci[10], pQmfReal[10+i]), cr[10], pQmfImag[10+i])) );

    fft[FFT_IDX_R(5)] = ( fMultDiv2(p[ 3], fMultSubDiv2( fMultDiv2(cr[3], pQmfReal[ 3+i]), ci[3], pQmfImag[ 3+i]))+
                          fMultDiv2(p[11], fMultSubDiv2(fMultDiv2(cr[11], pQmfReal[11+i]), ci[11], pQmfImag[11+i])) );
    fft[FFT_IDX_I(5)] = ( fMultDiv2(p[ 3], fMultAddDiv2( fMultDiv2(ci[3], pQmfReal[ 3+i]), cr[3], pQmfImag[ 3+i]))+
                          fMultDiv2(p[11], fMultAddDiv2(fMultDiv2(ci[11], pQmfReal[11+i]), cr[11], pQmfImag[11+i])) );

    fft[FFT_IDX_R(6)] = ( fMultDiv2(p[ 4], fMultSubDiv2( fMultDiv2(cr[4], pQmfReal[ 4+i]), ci[4], pQmfImag[ 4+i]))+
                          fMultDiv2(p[12], fMultSubDiv2(fMultDiv2(cr[12], pQmfReal[12+i]), ci[12], pQmfImag[12+i])) );
    fft[FFT_IDX_I(6)] = ( fMultDiv2(p[ 4], fMultAddDiv2( fMultDiv2(ci[4], pQmfReal[ 4+i]), cr[4], pQmfImag[ 4+i]))+
                          fMultDiv2(p[12], fMultAddDiv2(fMultDiv2(ci[12], pQmfReal[12+i]), cr[12], pQmfImag[12+i])) );

    fft[FFT_IDX_R(7)] = fMultDiv2(p[5], fMultSubDiv2(fMultDiv2(cr[5], pQmfReal[5+i]), ci[5], pQmfImag[5+i]));
    fft[FFT_IDX_I(7)] = fMultDiv2(p[5], fMultAddDiv2(fMultDiv2(ci[5], pQmfReal[5+i]), cr[5], pQmfImag[5+i]));

    /* fft modulation */
    fft_8(fft);

    /* resort fft data INTo output array*/
    for(bin=0; bin<8;bin++ ) {
      mHybridReal[i][bin] = FX_DBL2FX_QMF(fft[FFT_IDX_R(bin)]);
      mHybridImag[i][bin] = FX_DBL2FX_QMF(fft[FFT_IDX_I(bin)]);
    }
  }
}

/**************************************************************************//**
HybridAnalysis
******************************************************************************/

HANDLE_ERROR_INFO
HybridAnalysis ( HANDLE_PS_HYBRID       hHybrid,       /*!< Handle to HYBRID struct. */
                 FIXP_QMF *const *const mQmfReal,      /*!< The real part of the QMF-matrix.  */
                 FIXP_QMF *const *const mQmfImag,      /*!< The imaginary part of the QMF-matrix. */
                 SCHAR                  sf_fixpQmf,    /*!< Qmf scale factor */
                 FIXP_QMF             **mHybridReal,   /*!< The real part of the hybrid-matrix.  */
                 FIXP_QMF             **mHybridImag,   /*!< The imaginary part of the hybrid-matrix.  */
                 SCHAR                 *sf_fixpHybrid) /*!< Hybrid scale factor */
{
  HANDLE_ERROR_INFO error = noError;
  INT         n, band;
  INT         hybridRes;
  INT         chOffset = 0;
  /* INT         usedStereoBands   = hHybrid->mode;  */   /*!< indicates which 8 band filter to use */
  INT         frameSize         = hHybrid->frameSize;
  INT         hybridFilterDelay = hHybrid->hybridFilterDelay;

  for(band = 0; band < hHybrid->nQmfBands; band++) { /* loop all qmf bands */

    if(error == noError){
      hybridRes = hHybrid->pResolution[band];

      /* Create working buffer. */
      /* Copy stored samples to working buffer. */
      FDKmemcpy(hHybrid->pWorkReal, hHybrid->mQmfBufferReal[band],
                 hHybrid->qmfBufferMove * sizeof(FIXP_QMF));
      FDKmemcpy(hHybrid->pWorkImag, hHybrid->mQmfBufferImag[band],
                 hHybrid->qmfBufferMove * sizeof(FIXP_QMF));

      /* Append new samples to working buffer. */
      for(n = 0; n < frameSize; n++) {
        hHybrid->pWorkReal [hHybrid->qmfBufferMove + n] = mQmfReal [n + hybridFilterDelay] [band];
        hHybrid->pWorkImag [hHybrid->qmfBufferMove + n] = mQmfImag [n + hybridFilterDelay] [band];
      }

      /* Store samples for next frame. */
      FDKmemcpy(hHybrid->mQmfBufferReal[band], hHybrid->pWorkReal + frameSize,
                 hHybrid->qmfBufferMove * sizeof(FIXP_QMF));
      FDKmemcpy(hHybrid->mQmfBufferImag[band], hHybrid->pWorkImag + frameSize,
                 hHybrid->qmfBufferMove * sizeof(FIXP_QMF));


      switch(hybridRes) {
      case HYBRID_2_REAL:
        dualChannelFiltering( hHybrid->pWorkReal,
                              hHybrid->pWorkImag,
                              hHybrid->mTempReal,
                              hHybrid->mTempImag,
                              frameSize);

        /* copy data to output buffer */
        for(n = 0; n < frameSize; n++) {
          FDKmemcpy(&mHybridReal[n][chOffset], hHybrid->mTempReal[n],
                     (INT)(hybridRes & HYBRID_INVERSE_MASK)*sizeof(FIXP_QMF));
          FDKmemcpy(&mHybridImag[n][chOffset], hHybrid->mTempImag[n],
                     (INT)(hybridRes & HYBRID_INVERSE_MASK)*sizeof(FIXP_QMF));
        }
        break;

      case HYBRID_2_REAL | HYBRID_INVERSE_ORDER:
        dualChannelFiltering( hHybrid->pWorkReal,
                              hHybrid->pWorkImag,
                              hHybrid->mTempReal,
                              hHybrid->mTempImag,
                              frameSize);

        /* copy and resort data */
        for ( n = 0; n < frameSize; n++ )
          {
            mHybridReal[n][chOffset + 0] = hHybrid->mTempReal[n][1] ;
            mHybridReal[n][chOffset + 1] = hHybrid->mTempReal[n][0] ;
            mHybridImag[n][chOffset + 0] = hHybrid->mTempImag[n][1] ;
            mHybridImag[n][chOffset + 1] = hHybrid->mTempImag[n][0] ;
          }
        break;

      case HYBRID_6_CPLX:
        eightChannelFiltering( hHybrid->pWorkReal,
                               hHybrid->pWorkImag,
                               hHybrid->fft,
                               hHybrid->mTempReal,
                               hHybrid->mTempImag,
                               frameSize,
                               /*(usedStereoBands==PS_BANDS_FINE)?p8_13_34:*/p8_13_20);

        /* do the shuffle */
        for ( n = 0; n < frameSize; n++ )
          {
            /* add data ... */
            hHybrid->mTempReal[n][2] += hHybrid->mTempReal[n][5];
            hHybrid->mTempImag[n][2] += hHybrid->mTempImag[n][5];
            hHybrid->mTempReal[n][3] += hHybrid->mTempReal[n][4];
            hHybrid->mTempImag[n][3] += hHybrid->mTempImag[n][4];

            /* shuffle and copy to output buffer */
            mHybridReal[n][chOffset + 0] = hHybrid->mTempReal[n][6] ;
            mHybridReal[n][chOffset + 1] = hHybrid->mTempReal[n][7] ;
            mHybridReal[n][chOffset + 2] = hHybrid->mTempReal[n][0] ;
            mHybridReal[n][chOffset + 3] = hHybrid->mTempReal[n][1] ;
            mHybridReal[n][chOffset + 4] = hHybrid->mTempReal[n][2] ;
            mHybridReal[n][chOffset + 5] = hHybrid->mTempReal[n][3] ;

            mHybridImag[n][chOffset + 0] = hHybrid->mTempImag[n][6] ;
            mHybridImag[n][chOffset + 1] = hHybrid->mTempImag[n][7] ;
            mHybridImag[n][chOffset + 2] = hHybrid->mTempImag[n][0] ;
            mHybridImag[n][chOffset + 3] = hHybrid->mTempImag[n][1] ;
            mHybridImag[n][chOffset + 4] = hHybrid->mTempImag[n][2] ;
            mHybridImag[n][chOffset + 5] = hHybrid->mTempImag[n][3] ;
          }
        break;

      case HYBRID_8_CPLX:
        eightChannelFiltering( hHybrid->pWorkReal,
                               hHybrid->pWorkImag,
                               hHybrid->fft,
                               hHybrid->mTempReal,
                               hHybrid->mTempImag,
                               frameSize,
                               /*(usedStereoBands==PS_BANDS_FINE)?p8_13_34:*/p8_13_20);

        /* copy data to output buffer */
        for(n = 0; n < frameSize; n++) {
          FDKmemcpy(&mHybridReal[n][chOffset], hHybrid->mTempReal[n],
                     (INT)(hybridRes & HYBRID_INVERSE_MASK)*sizeof(FIXP_QMF));
          FDKmemcpy(&mHybridImag[n][chOffset], hHybrid->mTempImag[n],
                     (INT)(hybridRes & HYBRID_INVERSE_MASK)*sizeof(FIXP_QMF));
        }
        break;

      default:
        error = ERROR(CDI, "Invalid filter bank configuration.");
        break;
      }
      /* prepare next run by incresing chOffset */
      chOffset += hybridRes & HYBRID_INVERSE_MASK;
    }
  }

  *sf_fixpHybrid = sf_fixpQmf + HYBRID_SCALE;

  return error;
}

/**************************************************************************//**
  FDKsbrEnc_CreateHybridFilterBank
******************************************************************************/
HANDLE_ERROR_INFO
FDKsbrEnc_CreateHybridFilterBank ( HANDLE_PS_HYBRID        *phHybrid,     /*!< Pointer to handle to HYBRID struct.  */
                                   INT                      ch)           /*!< Current channel */
{
  HANDLE_ERROR_INFO  error = noError;
  INT i;
  HANDLE_PS_HYBRID hs = GetRam_PsHybrid(ch); /* allocate memory */
  if (hs==NULL) {
    error = 1;
    goto bail;
  }

  hs->fft = GetRam_PsHybFFT();

  /* alloc working memory */
  hs->pWorkReal = GetRam_PsHybWkReal();
  hs->pWorkImag = GetRam_PsHybWkImag();

  if ( (hs->fft==NULL) || (hs->pWorkReal==NULL) || (hs->pWorkImag==NULL) ) {
    error = 1;
    goto bail;
  }

  /* Allocate buffers */
  for (i = 0; i < HYBRID_FRAMESIZE; i++) {
    hs->mTempReal[i] = GetRam_PsMtmpReal(i);
    hs->mTempImag[i] = GetRam_PsMtmpImag(i);
    if ( (hs->mTempReal[i]==NULL) || (hs->mTempImag[i]==NULL) ) {
      error = 1;
      goto bail;
    }
  }

bail:
  *phHybrid = hs;
  return error;
}

HANDLE_ERROR_INFO
FDKsbrEnc_InitHybridFilterBank ( HANDLE_PS_HYBRID         hs,           /*!< Handle to HYBRID struct.  */
                                 HANDLE_PS_HYBRID_CONFIG  hHybConfig,   /*!< Configuration hanlde for filter bank */
                                 INT                      frameSize)    /*!< Number of QMF slots */
{
  HANDLE_ERROR_INFO  error = noError;
  INT i;
  INT maxNoChannels = HYBRID_12_CPLX, noBands;
  PS_BANDS mode;
  const INT *RESTRICT pResolution;

  /* filter bank configuration */
  mode    = hHybConfig->mode;
  noBands = hHybConfig->noQmfBandsInHybrid;
  pResolution = hHybConfig->aHybridResolution;

  /* assign resolution, check for valid values */
  for (i = 0; i < noBands; i++) {
    if(error == noError){
      if( pResolution[i] != HYBRID_12_CPLX &&
          pResolution[i] != HYBRID_8_CPLX &&
          pResolution[i] != HYBRID_6_CPLX &&
          pResolution[i] != HYBRID_2_REAL &&
          pResolution[i] != (HYBRID_2_REAL | HYBRID_INVERSE_ORDER) &&
          pResolution[i] != HYBRID_4_CPLX ){
        error = ERROR(CDI, "Invalid filter bank resolution");
      }
    }
    hs->pResolution[i] = pResolution[i];
    if((pResolution[i] & HYBRID_INVERSE_MASK) > maxNoChannels){
      maxNoChannels = pResolution[i] & HYBRID_INVERSE_MASK;
    }
  }
  FDK_ASSERT (MAX_HYBRID_RES>=maxNoChannels); /* check size of mTempReal/Imag */

  /* assign parameters */
  hs->mode              = mode;
  hs->nQmfBands         = noBands;
  hs->frameSize         = frameSize;
  hs->frameSizeInit     = frameSize;
  hs->qmfBufferMove     = HYBRID_FILTER_LENGTH - 1;
  hs->hybridFilterDelay = HYBRID_FILTER_LENGTH/2;

  FDK_ASSERT (HYBRID_FRAMESIZE>=hs->frameSize);
  FDK_ASSERT (QMF_BUFFER_MOVE>=hs->qmfBufferMove);

  return error;
}


/**************************************************************************//**
   FDKsbrEnc_DeleteHybridFilterBank
******************************************************************************/

HANDLE_ERROR_INFO
FDKsbrEnc_DeleteHybridFilterBank ( HANDLE_PS_HYBRID* phHybrid ) /*!< Pointer to handle to HYBRID struct. */
{
  int i;
  HANDLE_PS_HYBRID hHybrid = *phHybrid;

  if (hHybrid!=NULL) {
    if (hHybrid->fft)
      FreeRam_PsHybFFT(&hHybrid->fft);
    if (hHybrid->pWorkReal)
      FreeRam_PsHybWkReal(&hHybrid->pWorkReal);
    if (hHybrid->pWorkImag)
      FreeRam_PsHybWkImag(&hHybrid->pWorkImag);

    for (i = 0; i < HYBRID_FRAMESIZE; i++) {
      if (hHybrid->mTempReal[i])
        FreeRam_PsMtmpReal(&hHybrid->mTempReal[i]);
      if (hHybrid->mTempImag[i])
      FreeRam_PsMtmpImag(&hHybrid->mTempImag[i]);
    }

    FreeRam_PsHybrid(phHybrid);
  }

  return noError;
}

/*** Access functions ***/
INT FDKsbrEnc_GetHybridFilterDelay(HANDLE_PS_HYBRID hHybrid){

  return hHybrid->hybridFilterDelay;
}

