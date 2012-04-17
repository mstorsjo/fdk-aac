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
  \brief  Low Power Profile Transposer, $Revision: 36841 $
*/

#ifndef _LPP_TRANS_H
#define _LPP_TRANS_H

#include "sbrdecoder.h"
#include "qmf.h"

/*
  Common
*/
#define QMF_OUT_SCALE            8

/*
  Env-Adjust
*/
#define MAX_NOISE_ENVELOPES      2
#define MAX_NOISE_COEFFS         5
#define MAX_NUM_NOISE_VALUES     (MAX_NOISE_ENVELOPES * MAX_NOISE_COEFFS)
#define MAX_NUM_LIMITERS         12

/* Set MAX_ENVELOPES to the largest value of all supported BSFORMATs
   by overriding MAX_ENVELOPES in the correct order: */
#define MAX_ENVELOPES_HEAAC      5
#define MAX_ENVELOPES            MAX_ENVELOPES_HEAAC

#define MAX_FREQ_COEFFS          48
#define MAX_FREQ_COEFFS_FS44100  35
#define MAX_FREQ_COEFFS_FS48000  32


#define MAX_NUM_ENVELOPE_VALUES  (MAX_ENVELOPES * MAX_FREQ_COEFFS)

#define MAX_GAIN_EXP             34
/* Maximum gain will be sqrt(0.5 * 2^MAX_GAIN_EXP)
   example: 34=99dB   */
#define MAX_GAIN_CONCEAL_EXP     1
/* Maximum gain will be sqrt(0.5 * 2^MAX_GAIN_CONCEAL_EXP) in concealment case (0dB) */

/*
  LPP Transposer
*/
#define LPC_ORDER                2

#define MAX_INVF_BANDS           MAX_NOISE_COEFFS

#define MAX_NUM_PATCHES          6
#define SHIFT_START_SB           1              /*!< lowest subband of source range */

typedef enum
{
  INVF_OFF = 0,
  INVF_LOW_LEVEL,
  INVF_MID_LEVEL,
  INVF_HIGH_LEVEL,
  INVF_SWITCHED /* not a real choice but used here to control behaviour */
}
INVF_MODE;


/** parameter set for one single patch */
typedef struct {
  UCHAR  sourceStartBand;   /*!< first band in lowbands where to take the samples from */
  UCHAR  sourceStopBand;    /*!< first band in lowbands which is not included in the patch anymore */
  UCHAR  guardStartBand;    /*!< first band in highbands to be filled with zeros in order to
                                         reduce interferences between patches */
  UCHAR  targetStartBand;   /*!< first band in highbands to be filled with whitened lowband signal */
  UCHAR  targetBandOffs;    /*!< difference between 'startTargetBand' and 'startSourceBand' */
  UCHAR  numBandsInPatch;   /*!< number of consecutive bands in this one patch */
} PATCH_PARAM;


/** whitening factors for different levels of whitening
    need to be initialized corresponding to crossover frequency */
typedef struct {
  FIXP_DBL  off;                       /*!< bw factor for signal OFF */
  FIXP_DBL  transitionLevel;
  FIXP_DBL  lowLevel;                  /*!< bw factor for signal LOW_LEVEL */
  FIXP_DBL  midLevel;                  /*!< bw factor for signal MID_LEVEL */
  FIXP_DBL  highLevel;                 /*!< bw factor for signal HIGH_LEVEL */
} WHITENING_FACTORS;


/*! The transposer settings are calculated on a header reset and are shared by both channels. */
typedef struct {
  UCHAR  nCols;                       /*!< number subsamples of a codec frame */
  UCHAR  noOfPatches;                 /*!< number of patches */
  UCHAR  lbStartPatching;             /*!< first band of lowbands that will be patched */
  UCHAR  lbStopPatching;              /*!< first band that won't be patched anymore*/
  UCHAR  bwBorders[MAX_NUM_NOISE_VALUES]; /*!< spectral bands with different inverse filtering levels */

  PATCH_PARAM    patchParam[MAX_NUM_PATCHES]; /*!< new parameter set for patching */
  WHITENING_FACTORS whFactors;                /*!< the pole moving factors for certain whitening levels as indicated
                                                   in the bitstream depending on the crossover frequency */
  UCHAR  overlap;                             /*!< Overlap size */
} TRANSPOSER_SETTINGS;


typedef struct
{
  TRANSPOSER_SETTINGS *pSettings;                /*!< Common settings for both channels */
  FIXP_DBL  bwVectorOld[MAX_NUM_PATCHES];        /*!< pole moving factors of past frame */
  FIXP_DBL  lpcFilterStatesReal[LPC_ORDER][(32)];     /*!< pointer array to save filter states */
  FIXP_DBL  lpcFilterStatesImag[LPC_ORDER][(32)];     /*!< pointer array to save filter states */
}
SBR_LPP_TRANS;

typedef SBR_LPP_TRANS *HANDLE_SBR_LPP_TRANS;


void lppTransposer (HANDLE_SBR_LPP_TRANS hLppTrans,
                    QMF_SCALE_FACTOR  *sbrScaleFactor,
                    FIXP_DBL **qmfBufferReal,

                    FIXP_DBL *degreeAlias,
                    FIXP_DBL **qmfBufferImag,
                    const int useLP,
                    const int timeStep,
                    const int firstSlotOffset,
                    const int lastSlotOffset,
                    const int nInvfBands,
                    INVF_MODE *sbr_invf_mode,
                    INVF_MODE *sbr_invf_mode_prev
                    );


SBR_ERROR
createLppTransposer (HANDLE_SBR_LPP_TRANS hLppTrans,
                     TRANSPOSER_SETTINGS *pSettings,
                     const int  highBandStartSb,
                     UCHAR *v_k_master,
                     const int  numMaster,
                     const int  usb,
                     const int  timeSlots,
                     const int  nCols,
                     UCHAR *noiseBandTable,
                     const int  noNoiseBands,
                     UINT   fs,
                     const int  chan,
                     const int overlap);


SBR_ERROR
resetLppTransposer (HANDLE_SBR_LPP_TRANS hLppTrans,
                    UCHAR  highBandStartSb,
                    UCHAR *v_k_master,
                    UCHAR  numMaster,
                    UCHAR *noiseBandTable,
                    UCHAR  noNoiseBands,
                    UCHAR  usb,
                    UINT   fs);



#endif /* _LPP_TRANS_H */

