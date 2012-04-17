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
/*!
  \file
  \brief  Envelope calculation prototypes $Revision: 36841 $
*/
#ifndef __ENV_CALC_H
#define __ENV_CALC_H

#include "sbrdecoder.h"
#include "env_extr.h"  /* for HANDLE_SBR_HEADER_DATA */
#include "sbr_scale.h"


typedef struct
{
  FIXP_DBL  filtBuffer[MAX_FREQ_COEFFS];          /*!< previous gains (required for smoothing) */
  FIXP_DBL  filtBufferNoise[MAX_FREQ_COEFFS];     /*!< previous noise levels (required for smoothing) */
  SCHAR     filtBuffer_e[MAX_FREQ_COEFFS];        /*!< Exponents of previous gains */
  SCHAR     filtBufferNoise_e;                    /*!< Common exponent of previous noise levels */

  int startUp;               /*!< flag to signal initial conditions in buffers */
  int phaseIndex;            /*!< Index for randomPase array */
  int prevTranEnv;           /*!< The transient envelope of the previous frame. */

  int harmFlagsPrev[(MAX_FREQ_COEFFS+15)/16];
  /*!< Words with 16 flags each indicating where a sine was added in the previous frame.*/
  UCHAR harmIndex;            /*!< Current phase of synthetic sine */

}
SBR_CALCULATE_ENVELOPE;

typedef SBR_CALCULATE_ENVELOPE *HANDLE_SBR_CALCULATE_ENVELOPE;



void
calculateSbrEnvelope (QMF_SCALE_FACTOR  *sbrScaleFactor,
                      HANDLE_SBR_CALCULATE_ENVELOPE h_sbr_cal_env,
                      HANDLE_SBR_HEADER_DATA hHeaderData,
                      HANDLE_SBR_FRAME_DATA hFrameData,
                      FIXP_DBL **analysBufferReal,
                      FIXP_DBL **analysBufferImag,                 /*!< Imag part of subband samples to be processed */
                      const int useLP,
                      FIXP_DBL *degreeAlias,                       /*!< Estimated aliasing for each QMF channel */
                      const UINT flags,
                      const int frameErrorFlag
                      );

SBR_ERROR
createSbrEnvelopeCalc (HANDLE_SBR_CALCULATE_ENVELOPE hSbrCalculateEnvelope,
                       HANDLE_SBR_HEADER_DATA hHeaderData,
                       const int chan,
                       const UINT flags);

int
deleteSbrEnvelopeCalc (HANDLE_SBR_CALCULATE_ENVELOPE hSbrCalculateEnvelope);

void
resetSbrEnvelopeCalc (HANDLE_SBR_CALCULATE_ENVELOPE hCalEnv);

SBR_ERROR
ResetLimiterBands ( UCHAR *limiterBandTable,
                    UCHAR *noLimiterBands,
                    UCHAR *freqBandTable,
                    int noFreqBands,
                    const PATCH_PARAM *patchParam,
                    int noPatches,
                    int limiterBands);

void rescaleSubbandSamples( FIXP_DBL ** re,
                            FIXP_DBL ** im,
                            int lowSubband, int noSubbands,
                            int start_pos,  int next_pos,
                            int shift);

FIXP_DBL maxSubbandSample( FIXP_DBL ** analysBufferReal_m,
                           FIXP_DBL ** analysBufferImag_m,
                           int lowSubband,
                           int highSubband,
                           int start_pos,
                           int stop_pos);

#endif // __ENV_CALC_H
