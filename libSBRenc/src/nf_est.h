/****************************************************************************

                       (C) copyright Fraunhofer-IIS (2004)
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

****************************************************************************/
/*!
  \file
  \brief  Noise floor estimation structs and prototypes $Revision: 36867 $
*/

#ifndef __NF_EST_H
#define __NF_EST_H

#include "sbr_encoder.h"
#include "fram_gen.h"

#define NF_SMOOTHING_LENGTH 4                   /*!< Smoothing length of the noise floors. */

typedef struct
{
  FIXP_DBL prevNoiseLevels[NF_SMOOTHING_LENGTH][MAX_NUM_NOISE_VALUES]; /*!< The previous noise levels. */
  FIXP_DBL noiseFloorOffset[MAX_NUM_NOISE_VALUES];   /*!< Noise floor offset, scaled with NOISE_FLOOR_OFFSET_SCALING */
  const FIXP_DBL *smoothFilter;                      /*!< Smoothing filter to use. */
  FIXP_DBL ana_max_level;                            /*!< Max level allowed.   */
  FIXP_DBL weightFac;                                /*!< Weightening factor for the difference between orig and sbr. */
  INT freqBandTableQmf[MAX_NUM_NOISE_VALUES + 1]; /*!< Frequncy band table for the noise floor bands.*/
  INT noNoiseBands;                               /*!< Number of noisebands. */
  INT noiseBands;                                 /*!< NoiseBands switch 4 bit.*/
  INT timeSlots;                                  /*!< Number of timeslots in a frame. */
  INVF_MODE diffThres;                            /*!< Threshold value to control the inverse filtering decision */
}
SBR_NOISE_FLOOR_ESTIMATE;

typedef SBR_NOISE_FLOOR_ESTIMATE *HANDLE_SBR_NOISE_FLOOR_ESTIMATE;

void
FDKsbrEnc_sbrNoiseFloorEstimateQmf(HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate, /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                         const SBR_FRAME_INFO *frame_info,   /*!< Time frequency grid of the current frame. */
                         FIXP_DBL *noiseLevels,              /*!< Pointer to vector to store the noise levels in.*/
                         FIXP_DBL **quotaMatrixOrig,         /*!< Matrix holding the quota values of the original. */
                         SCHAR* indexVector,                 /*!< Index vector to obtain the patched data. */
                         INT missingHarmonicsFlag,           /*!< Flag indicating if a strong tonal component will be missing. */
                         INT startIndex,                     /*!< Start index. */
                         int numberOfEstimatesPerFrame,      /*!< The number of tonality estimates per frame. */
                         INT transientFrame,                 /*!< A flag indicating if a transient is present. */
                         INVF_MODE* pInvFiltLevels,          /*!< Pointer to the vector holding the inverse filtering levels. */
                         UINT sbrSyntaxFlags
                         );

INT
FDKsbrEnc_InitSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE  h_sbrNoiseFloorEstimate,   /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                             INT ana_max_level,                       /*!< Maximum level of the adaptive noise. */
                             const UCHAR *freqBandTable,      /*!< Frequany band table. */
                             INT nSfb,                                /*!< Number of frequency bands. */
                             INT noiseBands,                          /*!< Number of noise bands per octave. */
                             INT noiseFloorOffset,                    /*!< Noise floor offset. */
                             INT timeSlots,                           /*!< Number of time slots in a frame. */
                             UINT useSpeechConfig             /*!< Flag: adapt tuning parameters according to speech */
                             );

INT
FDKsbrEnc_resetSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate, /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */
                            const UCHAR *freqBandTable,   /*!< Frequany band table. */
                            INT nSfb);                            /*!< Number of bands in the frequency band table. */

void
FDKsbrEnc_deleteSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate); /*!< Handle to SBR_NOISE_FLOOR_ESTIMATE struct */

#endif
