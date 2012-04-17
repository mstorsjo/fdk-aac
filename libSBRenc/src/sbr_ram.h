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
\brief Memory layout
$Revision: 36847 $
*/
#ifndef __SBR_RAM_H
#define __SBR_RAM_H

#include "sbr_def.h"
#include "env_est.h"
#include "sbr_encoder.h"
#include "sbr.h"



#include "ps_main.h"
#include "ps_encode.h"


#define ENV_TRANSIENTS_BYTE  ( (sizeof(FIXP_DBL)*(MAX_NUM_CHANNELS*3*QMF_MAX_TIME_SLOTS)) )

#define ENV_R_BUFF_BYTE      ( (sizeof(FIXP_DBL)*((QMF_MAX_TIME_SLOTS) * QMF_CHANNELS)) )
#define ENV_I_BUFF_BYTE      ( (sizeof(FIXP_DBL)*((QMF_MAX_TIME_SLOTS) * QMF_CHANNELS)) )

#define Y_BUF_CH_BYTE        ( (sizeof(FIXP_DBL)*QMF_MAX_TIME_SLOTS * QMF_CHANNELS) )

#define ENV_R_BUF_PS_BYTE    ( (sizeof(FIXP_DBL)*QMF_MAX_TIME_SLOTS * QMF_CHANNELS / 2) )
#define ENV_I_BUF_PS_BYTE    ( (sizeof(FIXP_DBL)*QMF_MAX_TIME_SLOTS * QMF_CHANNELS / 2) )

#define TON_BUF_CH_BYTE      ( (sizeof(FIXP_DBL)*(MAX_NO_OF_ESTIMATES*MAX_FREQ_COEFFS)) )

#define Y_2_BUF_BYTE         ( Y_BUF_CH_BYTE>>1 )


/* Workbuffer RAM - Allocation */
/*
 ++++++++++++++++++++++++++++++++++++++++++++++++++++
 |        OFFSET_NRG      |        OFFSET_QMF       |
 ++++++++++++++++++++++++++++++++++++++++++++++++++++
  --------------------------------------------------
 |         0.5 *          |                         |
 | sbr_envYBuffer_size    |     sbr_envRBuffer      |
 |                        |     sbr_envIBuffer      |
  --------------------------------------------------

*/
  #define BUF_NRG_SIZE   ( (MAX_NUM_CHANNELS * Y_2_BUF_BYTE) )

  #define OFFSET_NRG         ( 0 )
  #define OFFSET_QMF         ( OFFSET_NRG + BUF_NRG_SIZE )

  /* if common dynamic memory used in AAC-core and SBR, find out maximal size of
     SCR buffer (see XX in figure above) */
  /* Only PS required holding 2 channel QMF data. AAC_WK_BUF_SIZE_0 must fit into this buffer. */
  #define BUF_QMF_SIZE  ( 2*(ENV_R_BUFF_BYTE + ENV_I_BUFF_BYTE) )

  /* Size of the shareable memory region than can be reused */
  #define SBR_ENC_DYN_RAM_SIZE  ( BUF_NRG_SIZE + BUF_QMF_SIZE )

/*
 *****************************************************************************************************
 */

  H_ALLOC_MEM(Ram_SbrDynamic_RAM, FIXP_DBL)

  H_ALLOC_MEM(Ram_SbrEncoder, SBR_ENCODER)
  H_ALLOC_MEM(Ram_SbrChannel, SBR_CHANNEL)
  H_ALLOC_MEM(Ram_SbrElement, SBR_ELEMENT)

  H_ALLOC_MEM(Ram_Sbr_quotaMatrix, FIXP_DBL)
  H_ALLOC_MEM(Ram_Sbr_signMatrix, INT)

  H_ALLOC_MEM(Ram_Sbr_QmfStatesAnalysis, FIXP_QAS)

  H_ALLOC_MEM(Ram_Sbr_freqBandTableLO, UCHAR)
  H_ALLOC_MEM(Ram_Sbr_freqBandTableHI, UCHAR)
  H_ALLOC_MEM(Ram_Sbr_v_k_master, UCHAR)

  H_ALLOC_MEM(Ram_Sbr_detectionVectors, UCHAR)
  H_ALLOC_MEM(Ram_Sbr_prevEnvelopeCompensation, UCHAR)
  H_ALLOC_MEM(Ram_Sbr_guideScfb, UCHAR)
  H_ALLOC_MEM(Ram_Sbr_guideVectorDetected, UCHAR)

  /* Dynamic Memory Allocation */

  H_ALLOC_MEM(Ram_Sbr_envYBuffer, FIXP_DBL)
  FIXP_DBL* GetRam_Sbr_envYBuffer (int n, UCHAR* dynamic_RAM);
  FIXP_DBL* GetRam_Sbr_envRBuffer (int n, UCHAR* dynamic_RAM);
  FIXP_DBL* GetRam_Sbr_envIBuffer (int n, UCHAR* dynamic_RAM);

  H_ALLOC_MEM(Ram_Sbr_guideVectorDiff, FIXP_DBL)
  H_ALLOC_MEM(Ram_Sbr_guideVectorOrig, FIXP_DBL)


  H_ALLOC_MEM(Ram_PsEnvRBuffer, FIXP_QMF)
  H_ALLOC_MEM(Ram_PsEnvIBuffer, FIXP_QMF)

  H_ALLOC_MEM(Ram_PsQmfStatesSynthesis, FIXP_DBL)
  H_ALLOC_MEM(Ram_PsQmfNewSamples, FIXP_DBL)

  H_ALLOC_MEM(Ram_PsEncConf, PSENC_CONFIG)
  H_ALLOC_MEM(Ram_PsEncode, PS_ENCODE)
  H_ALLOC_MEM(Ram_PsData, PS_DATA)

#define HYBRID_READ_OFFSET       ( 10 )
#define HYBRID_WRITE_OFFSET      ( ((32)>>1)-6 )

#define HYBRID_DATA_READ_OFFSET  ( 0 )
#define QMF_READ_OFFSET          ( 0 )

  H_ALLOC_MEM(Ram_PsRqmf, FIXP_DBL)
  H_ALLOC_MEM(Ram_PsIqmf, FIXP_DBL)
  FIXP_DBL* FDKsbrEnc_SliceRam_PsRqmf (FIXP_DBL* rQmfData, UCHAR* dynamic_RAM, int n, int i, int qmfSlots);
  FIXP_DBL* FDKsbrEnc_SliceRam_PsIqmf (FIXP_DBL* iQmfData, UCHAR* dynamic_RAM, int n, int i, int qmfSlots);

  H_ALLOC_MEM(Ram_PsQmfData, PS_QMF_DATA)
  H_ALLOC_MEM(Ram_PsChData, PS_CHANNEL_DATA)

  H_ALLOC_MEM(Ram_ParamStereo, PARAMETRIC_STEREO)
  H_ALLOC_MEM(Ram_PsOut, PS_OUT)

  /*  HYBRID  */
  H_ALLOC_MEM(Ram_PsHybFFT, FIXP_DBL)

  H_ALLOC_MEM(Ram_HybData, PS_HYBRID_DATA)
  H_ALLOC_MEM(Ram_PsRhyb, FIXP_DBL)
  H_ALLOC_MEM(Ram_PsIhyb, FIXP_DBL)

  H_ALLOC_MEM(Ram_PsHybConfig, PS_HYBRID_CONFIG)
  H_ALLOC_MEM(Ram_PsHybrid, PS_HYBRID)

  FIXP_DBL* psMqmfBufferRealInit (INT ch, INT i);
  FIXP_DBL* psMqmfBufferImagInit (INT ch, INT i);


  /* working buffer */
  H_ALLOC_MEM(Ram_PsHybWkReal, FIXP_DBL)
  H_ALLOC_MEM(Ram_PsHybWkImag, FIXP_DBL)

  H_ALLOC_MEM(Ram_PsMtmpReal, FIXP_DBL)
  H_ALLOC_MEM(Ram_PsMtmpImag, FIXP_DBL)



#endif

