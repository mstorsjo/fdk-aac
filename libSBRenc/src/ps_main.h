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
   Initial Authors:      Markus Multrus
   Contents/Description: PS Wrapper, Downmix header file

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef __INCLUDED_PS_MAIN_H
#define __INCLUDED_PS_MAIN_H

/* Includes ******************************************************************/
#include "sbr_def.h"
#include "qmf.h"
#include "ps_encode.h"
#include "FDK_bitstream.h"
#include "FDK_hybrid.h"


/* Data Types ****************************************************************/
typedef enum {
  PSENC_STEREO_BANDS_INVALID = 0,
  PSENC_STEREO_BANDS_10      = 10,
  PSENC_STEREO_BANDS_20      = 20

} PSENC_STEREO_BANDS_CONFIG;

typedef enum {
  PSENC_NENV_1               = 1,
  PSENC_NENV_2               = 2,
  PSENC_NENV_4               = 4,
  PSENC_NENV_DEFAULT         = PSENC_NENV_2,
  PSENC_NENV_MAX             = PSENC_NENV_4

} PSENC_NENV_CONFIG;

typedef struct {
  UINT                        bitrateFrom;   /* inclusive */
  UINT                        bitrateTo;     /* exclusive */
  PSENC_STEREO_BANDS_CONFIG   nStereoBands;
  PSENC_NENV_CONFIG           nEnvelopes;
  LONG                        iidQuantErrorThreshold;  /* quantization threshold to switch between coarse and fine iid quantization */

} psTuningTable_t;

/* Function / Class Declarations *********************************************/

typedef struct T_PARAMETRIC_STEREO {
  HANDLE_PS_ENCODE            hPsEncode;
  PS_OUT                      psOut[2];

  FIXP_DBL                    __staticHybridData[HYBRID_READ_OFFSET][MAX_PS_CHANNELS][2][MAX_HYBRID_BANDS];
  FIXP_DBL                   *pHybridData[HYBRID_READ_OFFSET+HYBRID_FRAMESIZE][MAX_PS_CHANNELS][2];

  FIXP_QMF                    qmfDelayLines[2][QMF_MAX_TIME_SLOTS>>1][QMF_CHANNELS];
  int                         qmfDelayScale;

  INT                         psDelay;
  UINT                        maxEnvelopes;
  UCHAR                       dynBandScale[PS_MAX_BANDS];
  FIXP_DBL                    maxBandValue[PS_MAX_BANDS];
  SCHAR                       dmxScale;
  INT                         initPS;
  INT                         noQmfSlots;
  INT                         noQmfBands;

  FIXP_DBL                    __staticHybAnaStatesLF[MAX_PS_CHANNELS][2*HYBRID_FILTER_LENGTH*HYBRID_MAX_QMF_BANDS];
  FIXP_DBL                    __staticHybAnaStatesHF[MAX_PS_CHANNELS][2*HYBRID_FILTER_DELAY*(QMF_CHANNELS-HYBRID_MAX_QMF_BANDS)];
  FDK_ANA_HYB_FILTER          fdkHybAnaFilter[MAX_PS_CHANNELS];
  FDK_SYN_HYB_FILTER          fdkHybSynFilter;

} PARAMETRIC_STEREO;


typedef struct T_PSENC_CONFIG {
  INT                         frameSize;
  INT                         qmfFilterMode;
  INT                         sbrPsDelay;
  PSENC_STEREO_BANDS_CONFIG   nStereoBands;
  PSENC_NENV_CONFIG           maxEnvelopes;
  FIXP_DBL                    iidQuantErrorThreshold;

} PSENC_CONFIG, *HANDLE_PSENC_CONFIG;

typedef struct T_PARAMETRIC_STEREO *HANDLE_PARAMETRIC_STEREO;


/**
 * \brief  Create a parametric stereo encoder instance.
 *
 * \param phParametricStereo    A pointer to a parametric stereo handle to be allocated. Initialized on return.
 *
 * \return
 *          - PSENC_OK, on succes.
 *          - PSENC_INVALID_HANDLE, PSENC_MEMORY_ERROR, on failure.
 */
FDK_PSENC_ERROR PSEnc_Create(
        HANDLE_PARAMETRIC_STEREO *phParametricStereo
        );


/**
 * \brief  Initialize a parametric stereo encoder instance.
 *
 * \param hParametricStereo     Meta Data handle.
 * \param hPsEncConfig          Filled parametric stereo configuration structure.
 * \param noQmfSlots            Number of slots within one audio frame.
 * \param noQmfBands            Number of QMF bands.
 * \param dynamic_RAM           Pointer to preallocated workbuffer.
 *
 * \return
 *          - PSENC_OK, on succes.
 *          - PSENC_INVALID_HANDLE, PSENC_INIT_ERROR, on failure.
 */
FDK_PSENC_ERROR PSEnc_Init(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        const HANDLE_PSENC_CONFIG hPsEncConfig,
        INT                       noQmfSlots,
        INT                       noQmfBands
       ,UCHAR                    *dynamic_RAM
        );


/**
 * \brief  Destroy parametric stereo encoder instance.
 *
 * Deallocate instance and free whole memory.
 *
 * \param phParametricStereo    Pointer to the parametric stereo handle to be deallocated.
 *
 * \return
 *          - PSENC_OK, on succes.
 *          - PSENC_INVALID_HANDLE, on failure.
 */
FDK_PSENC_ERROR PSEnc_Destroy(
        HANDLE_PARAMETRIC_STEREO *phParametricStereo
        );


/**
 * \brief  Apply parametric stereo processing.
 *
 * \param hParametricStereo     Meta Data handle.
 * \param samples               Pointer to 2 channel audio input signal.
 * \param timeInStride,         Stride factor of input buffer.
 * \param hQmfAnalysis,         Pointer to QMF analysis filterbanks.
 * \param downmixedRealQmfData  Pointer to real QMF buffer to be written to.
 * \param downmixedImagQmfData  Pointer to imag QMF buffer to be written to.
 * \param downsampledOutSignal  Pointer to buffer where to write downmixed timesignal.
 * \param sbrSynthQmf           Pointer to QMF synthesis filterbank.
 * \param qmfScale              Return scaling factor of the qmf data.
 * \param sendHeader            Signal whether to write header data.
 *
 * \return
 *          - PSENC_OK, on succes.
 *          - PSENC_INVALID_HANDLE, PSENC_ENCODE_ERROR, on failure.
 */
FDK_PSENC_ERROR FDKsbrEnc_PSEnc_ParametricStereoProcessing(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        INT_PCM                  *samples[2],
        UINT                      timeInStride,
        QMF_FILTER_BANK         **hQmfAnalysis,
        FIXP_QMF **RESTRICT       downmixedRealQmfData,
        FIXP_QMF **RESTRICT       downmixedImagQmfData,
        INT_PCM                  *downsampledOutSignal,
        HANDLE_QMF_FILTER_BANK    sbrSynthQmf,
        SCHAR                    *qmfScale,
        const int                 sendHeader
        );


/**
 * \brief  Write parametric stereo bitstream.
 *
 * Write ps_data() element to bitstream and return number of written bits.
 * Returns number of written bits only, if hBitstream == NULL.
 *
 * \param hParametricStereo     Meta Data handle.
 * \param hBitstream            Bitstream buffer handle.
 *
 * \return
 *          - number of written bits.
 */
INT FDKsbrEnc_PSEnc_WritePSData(
        HANDLE_PARAMETRIC_STEREO  hParametricStereo,
        HANDLE_FDK_BITSTREAM      hBitstream
        );

#endif /* __INCLUDED_PS_MAIN_H */
