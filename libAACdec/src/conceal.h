/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Josef Hoepfl
   Description: independent channel concealment

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef _CONCEAL_H_
#define _CONCEAL_H_

#include "aacdecoder_lib.h"

#include "channelinfo.h"

#define AACDEC_CONCEAL_PARAM_NOT_SPECIFIED ( 0xFFFE )

void CConcealment_InitCommonData  (CConcealParams   *pConcealCommonData);

void CConcealment_InitChannelData (CConcealmentInfo *hConcealmentInfo,
                                   CConcealParams   *pConcealCommonData,
                                   int  samplesPerFrame);

CConcealmentMethod
     CConcealment_GetMethod       (CConcealParams *pConcealCommonData);

UINT
  CConcealment_GetDelay           (CConcealParams *pConcealCommonData);

AAC_DECODER_ERROR
     CConcealment_SetParams       (CConcealParams *concealParams,
                                   int  method,
                                   int  fadeOutSlope,
                                   int  fadeInSlope,
                                   int  muteRelease,
                                   int  comfNoiseLevel);

CConcealmentState
     CConcealment_GetState        (CConcealmentInfo *hConcealmentInfo);

AAC_DECODER_ERROR
     CConcealment_SetAttenuation  (CConcealParams *concealParams,
                                   SHORT *fadeOutAttenuationVector,
                                   SHORT *fadeInAttenuationVector);

void CConcealment_Store           (CConcealmentInfo *hConcealmentInfo,
                                   CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                   CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo );

int  CConcealment_Apply           (CConcealmentInfo *hConcealmentInfo,
                                   CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                   CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                                   const SamplingRateInfo *pSamplingRateInfo,
                                   const int samplesPerFrame,
                                   const UCHAR lastLpdMode,
                                   const int FrameOk,
                                   const UINT flags);

FIXP_DBL
     CConcealment_GetFadeFactor   (CConcealmentInfo *hConcealmentInfo,
                                   const int fPreviousFactor);

int  CConcealment_GetLastFrameOk  (CConcealmentInfo *hConcealmentInfo,
                                   const int fBeforeApply);

#endif /* #ifndef _CONCEAL_H_ */
