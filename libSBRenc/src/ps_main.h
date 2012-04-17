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
#include "psenc_hybrid.h"
#include "ps_encode.h"
#include "FDK_bitstream.h"

/* Data Types ****************************************************************/
typedef enum {
  PSENC_STEREO_BANDS_INVALID = 0,
  PSENC_STEREO_BANDS_10 = 10,
  PSENC_STEREO_BANDS_20 = 20,
  PSENC_STEREO_BANDS_34 = 34
} PSENC_STEREO_BANDS_CONFIG;

typedef enum {
  PSENC_NENV_1 = 1,
  PSENC_NENV_2 = 2,
  PSENC_NENV_4 = 4,
  PSENC_NENV_DEFAULT = PSENC_NENV_2,
  PSENC_NENV_MAX = PSENC_NENV_4
} PSENC_NENV_CONFIG;

#define MAX_PS_CHANNELS          (  2 )
#define PSENC_QMF_BUFFER_LENGTH  ( 48 )

typedef struct {

  UINT              bitrateFrom;   /* inclusive */
  UINT              bitrateTo;     /* exclusive */
  PSENC_STEREO_BANDS_CONFIG nStereoBands;
  PSENC_NENV_CONFIG         nEnvelopes;
  LONG                      iidQuantErrorThreshold;  /* quantization threshold to switch between coarse and fine iid quantization */

} psTuningTable_t;

/* Function / Class Declarations *********************************************/

typedef struct T_PS_QMF_DATA
{
  FIXP_QMF *rQmfData[PSENC_QMF_BUFFER_LENGTH];
  FIXP_QMF *iQmfData[PSENC_QMF_BUFFER_LENGTH];
  INT     nCols;
  INT     nRows;
  INT     bufferReadOffset;
  INT     bufferReadOffsetHybrid;
  INT     bufferWriteOffset;
  INT     bufferLength;
} PS_QMF_DATA, *HANDLE_PS_QMF_DATA;

typedef struct T_PS_CHANNEL_DATA {
  HANDLE_PS_QMF_DATA         hPsQmfData;

  int                        psQmfScale;
  HANDLE_PS_HYBRID_DATA      hHybData;
  HANDLE_PS_HYBRID           hHybAna;
  INT                        psChannelDelay; /* delay in samples */
} PS_CHANNEL_DATA, *HANDLE_PS_CHANNEL_DATA;

typedef struct T_PARAMETRIC_STEREO {

  HANDLE_PS_HYBRID_CONFIG     hHybridConfig;
  HANDLE_PS_CHANNEL_DATA      hPsChannelData[MAX_PS_CHANNELS];
  HANDLE_PS_ENCODE            hPsEncode;
  HANDLE_PS_OUT               hPsOut[2];

  FIXP_QMF  *qmfDelayReal[QMF_MAX_TIME_SLOTS>>1];
  FIXP_QMF  *qmfDelayImag[QMF_MAX_TIME_SLOTS>>1];
  FIXP_QMF  *qmfDelayRealRef;
  FIXP_QMF  *qmfDelayImagRef;
  int        qmfDelayScale;

  INT                         psDelay;
  UINT                        maxEnvelopes;
  UCHAR                       dynBandScale[PS_MAX_BANDS];
  FIXP_DBL                    maxBandValue[PS_MAX_BANDS];
  SCHAR                       dmxScale;
  INT                         initPS;
  INT                         noQmfSlots;
  INT                         noQmfBands;

} PARAMETRIC_STEREO;



typedef struct T_PSENC_CONFIG {

  INT                       frameSize;
  INT                       qmfFilterMode;
  INT                       sbrPsDelay;
  PSENC_STEREO_BANDS_CONFIG nStereoBands;
  PSENC_NENV_CONFIG         maxEnvelopes;
  FIXP_DBL                  iidQuantErrorThreshold;

} PSENC_CONFIG, *HANDLE_PSENC_CONFIG;

typedef struct T_PARAMETRIC_STEREO *HANDLE_PARAMETRIC_STEREO;


HANDLE_ERROR_INFO
PSEnc_Create(HANDLE_PARAMETRIC_STEREO *phParametricStereo);

HANDLE_ERROR_INFO
PSEnc_Init(HANDLE_PARAMETRIC_STEREO    hParametricStereo,
             HANDLE_PSENC_CONFIG       hPsEncConfig,
             INT                       noQmfSlots,
             INT                       noQmfBands
            ,UCHAR                   *dynamic_RAM
            );

HANDLE_ERROR_INFO
UpdatePSQmfData_second(HANDLE_PARAMETRIC_STEREO hParametricStereo);

HANDLE_ERROR_INFO
PSEnc_Destroy(HANDLE_PARAMETRIC_STEREO *hParametricStereo);


HANDLE_ERROR_INFO
FDKsbrEnc_PSEnc_ParametricStereoProcessing(HANDLE_PARAMETRIC_STEREO hParametricStereo,
                                           FIXP_QMF **RESTRICT qmfRealData,
                                           FIXP_QMF **RESTRICT qmfImagData,
                                           INT                 qmfOffset,
                                           INT_PCM            *downsampledOutSignal,
                                           HANDLE_QMF_FILTER_BANK sbrSynthQmf,
                                           SCHAR              *qmfScale,
                                           const int           sendHeader);

INT
FDKsbrEnc_PSEnc_WritePSData(HANDLE_PARAMETRIC_STEREO hParametricStereo, HANDLE_FDK_BITSTREAM hBitstream);

#endif /* __INCLUDED_PS_MAIN_H */
