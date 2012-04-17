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

 $Header$

*******************************************************************************/
/*!
  \file
  \brief  General tonality correction detector module.
*/
#ifndef _TON_CORR_EST_H
#define _TON_CORR_EST_H

#include "sbr_encoder.h"
#include "mh_det.h"
#include "nf_est.h"
#include "invf_est.h"


#define MAX_NUM_PATCHES 6
#define SCALE_NRGVEC 4

/** parameter set for one single patch */
typedef struct {
  INT    sourceStartBand;         /*!< first band in lowbands where to take the samples from */
  INT    sourceStopBand;          /*!< first band in lowbands which is not included in the patch anymore */
  INT    guardStartBand;          /*!< first band in highbands to be filled with zeros in order to
                                         reduce interferences between patches */
  INT    targetStartBand;         /*!< first band in highbands to be filled with whitened lowband signal */
  INT    targetBandOffs;          /*!< difference between 'startTargetBand' and 'startSourceBand' */
  INT    numBandsInPatch;         /*!< number of consecutive bands in this one patch */
} PATCH_PARAM;




typedef struct
{
  INT switchInverseFilt;          /*!< Flag to enable dynamic adaption of invf. detection */
  INT noQmfChannels;
  INT bufferLength;               /*!< Length of the r and i buffers. */
  INT stepSize;                   /*!< Stride for the lpc estimate. */
  INT numberOfEstimates;          /*!< The total number of estiamtes, available in the quotaMatrix.*/
  INT numberOfEstimatesPerFrame;  /*!< The number of estimates per frame available in the quotaMatrix.*/
  INT lpcLength[2];               /*!< Segment length used for second order LPC analysis.*/
  INT nextSample;                 /*!< Where to start the LPC analysis of the current frame.*/
  INT move;                       /*!< How many estimates to move in the quotaMatrix, when buffering. */
  INT frameStartIndex;            /*!< The start index for the current frame in the r and i buffers. */
  INT startIndexMatrix;           /*!< The start index for the current frame in the quotaMatrix. */
  INT frameStartIndexInvfEst;     /*!< The start index of the inverse filtering, not the same as the others,
                                       dependent on what decoder is used (buffer opt, or no buffer opt). */
  INT prevTransientFlag;          /*!< The transisent flag (from the transient detector) for the previous frame. */
  INT transientNextFrame;         /*!< Flag to indicate that the transient will show up in the next frame. */
  INT transientPosOffset;         /*!< An offset value to match the transient pos as calculated by the transient detector
                                       with the actual position in the frame.*/

  INT *signMatrix[MAX_NO_OF_ESTIMATES];      /*!< Matrix holding the sign of each channe, i.e. indicating in what
                                                  part of a QMF channel a possible sine is. */

  FIXP_DBL *quotaMatrix[MAX_NO_OF_ESTIMATES];/*!< Matrix holding the quota values for all estimates, all channels. */

  FIXP_DBL nrgVector[MAX_NO_OF_ESTIMATES];   /*!< Vector holding the averaged energies for every QMF band. */
  FIXP_DBL nrgVectorFreq[QMF_CHANNELS];      /*!< Vector holding the averaged energies for every QMF channel */

  SCHAR indexVector[QMF_CHANNELS];           /*!< Index vector poINTing to the correct lowband channel,
                                                  when indexing a highband channel, -1 represents a guard band */
  PATCH_PARAM  patchParam[MAX_NUM_PATCHES];  /*!< new parameter set for patching */
  INT    guard;                              /*!< number of guardbands between every patch */
  INT    shiftStartSb;                       /*!< lowest subband of source range to be included in the patches */
  INT    noOfPatches;                        /*!< number of patches */

  SBR_MISSING_HARMONICS_DETECTOR sbrMissingHarmonicsDetector;  /*!< SBR_MISSING_HARMONICS_DETECTOR struct. */
  SBR_NOISE_FLOOR_ESTIMATE sbrNoiseFloorEstimate;              /*!< SBR_NOISE_FLOOR_ESTIMATE struct. */
  SBR_INV_FILT_EST sbrInvFilt;                                 /*!< SBR_INV_FILT_EST struct. */
}
SBR_TON_CORR_EST;

typedef SBR_TON_CORR_EST *HANDLE_SBR_TON_CORR_EST;

void
FDKsbrEnc_TonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,   /*!< Handle to SBR_TON_CORR struct. */
                 INVF_MODE* infVec,                  /*!< Vector where the inverse filtering levels will be stored. */
                 FIXP_DBL * noiseLevels,             /*!< Vector where the noise levels will be stored. */
                 INT* missingHarmonicFlag,           /*!< Flag set to one or zero, dependent on if any strong sines are missing.*/
                 UCHAR* missingHarmonicsIndex,       /*!< Vector indicating where sines are missing. */
                 UCHAR* envelopeCompensation,        /*!< Vector to store compensation values for the energies in. */
                 const SBR_FRAME_INFO *frameInfo,    /*!< Frame info struct, contains the time and frequency grid of the current frame.*/
                 UCHAR* transientInfo,               /*!< Transient info.*/
                 UCHAR * freqBandTable,              /*!< Frequency band tables for high-res.*/
                 INT nSfb,                           /*!< Number of scalefactor bands for high-res. */
                 XPOS_MODE xposType,                 /*!< Type of transposer used in the decoder.*/
                 UINT sbrSyntaxFlags
                 );

INT
FDKsbrEnc_CreateTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr, /*!< Pointer to handle to SBR_TON_CORR struct. */
                                 INT                     chan);    /*!< Channel index, needed for mem allocation */

INT
FDKsbrEnc_InitTonCorrParamExtr(INT frameSize,                      /*!< Current SBR frame size. */
                               HANDLE_SBR_TON_CORR_EST hTonCorr,   /*!< Pointer to handle to SBR_TON_CORR struct. */
                               HANDLE_SBR_CONFIG_DATA sbrCfg,      /*!< Pointer to SBR configuration parameters. */
                               INT timeSlots,                      /*!< Number of time-slots per frame */
                               INT xposCtrl,                       /*!< Different patch modes. */
                               INT ana_max_level,                  /*!< Maximum level of the adaptive noise. */
                               INT noiseBands,                     /*!< Number of noise bands per octave. */
                               INT noiseFloorOffset,               /*!< Noise floor offset. */
                               UINT useSpeechConfig                /*!< Speech or music tuning. */
                              );

void
FDKsbrEnc_DeleteTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr); /*!< Handle to SBR_TON_CORR struct. */


void
FDKsbrEnc_CalculateTonalityQuotas(HANDLE_SBR_TON_CORR_EST hTonCorr,
                        FIXP_DBL **sourceBufferReal,
                        FIXP_DBL **sourceBufferImag,
                        INT usb,
                        INT qmfScale                    /*!< sclefactor of QMF subsamples */
                       );

INT
FDKsbrEnc_ResetTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr, /*!< Handle to SBR_TON_CORR struct. */
                      INT xposctrl,                     /*!< Different patch modes. */
                      INT highBandStartSb,              /*!< Start band of the SBR range. */
                      UCHAR *v_k_master,        /*!< Master frequency table from which all other table are derived.*/
                      INT numMaster,                    /*!< Number of elements in the master table. */
                      INT fs,                           /*!< Sampling frequency (of the SBR part). */
                      UCHAR** freqBandTable,    /*!< Frequency band table for low-res and high-res. */
                      INT* nSfb,                        /*!< Number of frequency bands (hig-res and low-res). */
                      INT noQmfChannels                 /*!< Number of QMF channels. */
                      );
#endif

