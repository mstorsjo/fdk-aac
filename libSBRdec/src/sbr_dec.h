/****************************************************************************

                     (C) Copyright Fraunhofer IIS (2005)
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
  \brief  Sbr decoder $Revision: 36841 $
*/
#ifndef __SBR_DEC_H
#define __SBR_DEC_H

#include "sbrdecoder.h"

#include "lpp_tran.h"
#include "qmf.h"
#include "env_calc.h"
#include "FDK_audio.h"


#include "sbrdec_drc.h"

#define SACDEC_ALIGNMENT_FIX

typedef struct
{
  QMF_FILTER_BANK     AnalysiscQMF;
  QMF_FILTER_BANK     SynthesisQMF;

  SBR_CALCULATE_ENVELOPE  SbrCalculateEnvelope;
  SBR_LPP_TRANS           LppTrans;

  QMF_SCALE_FACTOR sbrScaleFactor;
  QMF_SCALE_FACTOR sbrScaleFactorRight;

  /*! Delayed spectral data needed for the dynamic framing of SBR. Not required in case of CLDFB */
  FIXP_DBL * pSbrOverlapBuffer;

  /* References to workbuffers */
  FIXP_DBL * WorkBuffer1;
  FIXP_DBL * WorkBuffer2;

  /* QMF filter states */
  FIXP_QAS   anaQmfStates[(320)];
  FIXP_QSS * pSynQmfStates;

  /* Reference pointer arrays for QMF time slots,
     mixed among overlap and current slots. */
  FIXP_DBL * QmfBufferReal[(((1024)/(32))+(6))];
  FIXP_DBL * QmfBufferImag[(((1024)/(32))+(6))];
  int useLP;

  /* QMF domain extension time slot reference pointer array */

  SBRDEC_DRC_CHANNEL  sbrDrcChannel;

} SBR_DEC;

typedef SBR_DEC *HANDLE_SBR_DEC;


typedef struct
{
  SBR_FRAME_DATA      frameData[(1)+1];
  SBR_PREV_FRAME_DATA prevFrameData;
  SBR_DEC SbrDec;
}
SBR_CHANNEL;

typedef SBR_CHANNEL *HANDLE_SBR_CHANNEL;

void
SbrDecodeAndProcess (HANDLE_SBR_DEC hSbrDec,
                     INT_PCM *timeIn,
                     HANDLE_SBR_HEADER_DATA hHeaderData,
                     HANDLE_SBR_FRAME_DATA hFrameData,
                     HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,
                     int applyProcessing,
                     int channelNr
                     , UCHAR useLP
                     );


void
SbrConstructTimeOutput (HANDLE_SBR_DEC hSbrDec,            /*!< handle to Decoder channel */
                        INT_PCM *timeOut,                    /*!< pointer to output time signal */
                        HANDLE_SBR_HEADER_DATA hHeaderData,/*!< Static control data */
                        HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,  /*!< Some control data of last frame */
                        int channelNr
                       ,UCHAR useLP
                        );


void
sbr_dec (HANDLE_SBR_DEC hSbrDec,            /*!< handle to Decoder channel */
         INT_PCM *timeIn,                   /*!< pointer to input time signal */
         INT_PCM *timeOut,                  /*!< pointer to output time signal */
         HANDLE_SBR_DEC hSbrDecRight,       /*!< handle to Decoder channel right */
         INT_PCM *timeOutRight,             /*!< pointer to output time signal */
         const int strideIn,                /*!< Time data traversal strideIn */
         const int strideOut,               /*!< Time data traversal strideOut */
         HANDLE_SBR_HEADER_DATA hHeaderData,/*!< Static control data */
         HANDLE_SBR_FRAME_DATA hFrameData,  /*!< Control data of current frame */
         HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,  /*!< Some control data of last frame */
         const int applyProcessing,         /*!< Flag for SBR operation */
         HANDLE_PS_DEC h_ps_d,
         const UINT flags
        );



SBR_ERROR
createSbrDec (SBR_CHANNEL * hSbrChannel,
              HANDLE_SBR_HEADER_DATA hHeaderData,
              TRANSPOSER_SETTINGS *pSettings,
              const int downsampleFac,
              const UINT qmfFlags,
              const UINT flags,
              const int overlap,
              int chan);

int
deleteSbrDec (SBR_CHANNEL * hSbrChannel);

SBR_ERROR
resetSbrDec (HANDLE_SBR_DEC hSbrDec,
             HANDLE_SBR_HEADER_DATA hHeaderData,
             HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,
             const int useLP,
             const int downsampleFac);

#endif
