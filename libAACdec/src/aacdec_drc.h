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
   Author(s):   Christian Griebel
   Description: Dynamic range control (DRC) decoder tool for AAC

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef AACDEC_DRC_H
#define AACDEC_DRC_H

#include "tp_data.h"            /* for program config element support */


#include "aacdec_drc_types.h"
#include "channel.h"
#include "FDK_bitstream.h"

#define AACDEC_DRC_DEFAULT_REF_LEVEL  ( 108 )   /* -27 dB below full scale (typical for movies) */
#define AACDEC_DRC_DFLT_EXPIRY_FRAMES (  40 )   /* Default DRC data expiry time in AAC frames   */
#define MAX_SBR_SYN_CHAN              (  64 )
#define MAX_SBR_COLS                  (  32 )


/**
 * \brief DRC module setting parameters
 */
typedef enum
{
  DRC_CUT_SCALE = 0,
  DRC_BOOST_SCALE,
  TARGET_REF_LEVEL,
  DRC_BS_DELAY,
  DRC_DATA_EXPIRY_FRAME,
  APPLY_HEAVY_COMPRESSION

} AACDEC_DRC_PARAM;


/**
 * \brief DRC module interface functions
 */
void aacDecoder_drcInit (
        HANDLE_AAC_DRC  self );

void aacDecoder_drcInitChannelData (
        CDrcChannelData *pDrcChannel );

AAC_DECODER_ERROR aacDecoder_drcSetParam (
        HANDLE_AAC_DRC    self,
        AACDEC_DRC_PARAM  param,
        INT               value );

int aacDecoder_drcMarkPayload (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM  hBs,
        AACDEC_DRC_PAYLOAD_TYPE  type );

int aacDecoder_drcProlog (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM hBs,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        UCHAR  pceInstanceTag,
        UCHAR  channelMapping[],
        int    numChannels );

void aacDecoder_drcApply (
        HANDLE_AAC_DRC          self,
        void                   *pSbrDec,
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        CDrcChannelData        *pDrcDat,
        int  ch,
        int  aacFrameSize,
        int  bSbrPresent );

int aacDecoder_drcEpilog (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM hBs,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        UCHAR  pceInstanceTag,
        UCHAR  channelMapping[],
        int    validChannels );


#endif  /* AACDEC_DRC_H */
