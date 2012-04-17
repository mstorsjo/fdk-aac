/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2004)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

 $Id$

*******************************************************************************/
#ifndef __RESAMPLER_H
#define __RESAMPLER_H
/*!
  \file
  \brief  Fixed Point Resampler Tool Box $Revision: 11752 $
*/

/*!
  \mainpage Fixed Point Resampler Library Documentation

  Information in this SDK is subject to change without notice. Companies,
  names, and data used in examples herein are fictitious unless otherwise
  noted.

  Product and corporate names may be trademarks or registered trademarks
  of other companies. They are used for explanation only, with no intent
  to infringe.

  No part of this publication may be reproduced or utilized in any form or
  by any means, electronic or mechanical, including photocopying and
  microfilm, without permission in writing from the publisher.
*/

#include "common_fix.h"


/**************************************************************************/
/*                         BIQUAD Filter Structure                           */
/**************************************************************************/

#define MAXNR_SECTIONS  (15)

#ifdef RS_BIQUAD_STATES16
typedef FIXP_SGL FIXP_BQS;
#else
typedef FIXP_DBL FIXP_BQS;
#endif

typedef struct
{
  FIXP_BQS states[MAXNR_SECTIONS+1][2];   /*! state buffer */
  const FIXP_SGL *coeffa;               /*! pointer to filter coeffs */
  FIXP_DBL gain;                        /*! overall gain factor */
  int Wc;                               /*! normalized cutoff freq * 1000 */
  int noCoeffs;                         /*! number of filter coeffs sets */
  int ptr;                              /*! index to rinbuffers */
} LP_FILTER;


/**************************************************************************/
/*                        Downsampler Structure                           */
/**************************************************************************/

typedef struct
{
  LP_FILTER downFilter;           /*! filter instance */
  int ratio;                      /*! downsampling ration */
  int delay;                      /*! downsampling delay (source fs)   */
  int pending;                    /*! number of pending output samples */
} DOWNSAMPLER;


/**
 * \brief Initialized a given downsampler structure.
 */
INT FDKaacEnc_InitDownsampler(DOWNSAMPLER *DownSampler, /*!< pointer to downsampler instance */
                              INT Wc,                   /*!< normalized cutoff freq * 1000 */
                              INT ratio);               /*!< downsampler ratio */

/**
 * \brief Downsample a set of audio samples. numInSamples must be at least equal to the
 *        downsampler ratio.
 */
INT FDKaacEnc_Downsample(DOWNSAMPLER *DownSampler,  /*!< pointer to downsampler instance */
                         INT_PCM *inSamples,        /*!< pointer to input samples */
                         INT numInSamples,          /*!< number  of input samples  */
                         INT inStride,              /*!< increment of input samples      */
                         INT_PCM *outSamples,       /*!< pointer to output samples */
                         INT *numOutSamples,        /*!< pointer tp number of output samples */
                         INT outstride);            /*!< increment of output samples */



#endif /* __RESAMPLER_H */
