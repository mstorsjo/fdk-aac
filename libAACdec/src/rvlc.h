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

****************************************************************************/
/*!
  \file
  \brief  Defines structures and prototypes for RVLC
  \author Robert Weidner
*/

#ifndef RVLC_H
#define RVLC_H



#include "aacdecoder.h"
#include "channel.h"
#include "rvlc_info.h"

/* ------------------------------------------------------------------- */
/*    errorLogRvlc: A word of 32 bits used for logging possible errors */
/*                  within RVLC in case of distorted bitstreams.       */
/* ------------------------------------------------------------------- */
#define RVLC_ERROR_ALL_ESCAPE_WORDS_INVALID              0x80000000   /*  ESC-Dec  During RVLC-Escape-decoding there have been more bits decoded as there are available */
#define RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_FWD    0x40000000   /*  RVL-Dec  negative sum-bitcounter during RVL-fwd-decoding (long+shrt) */
#define RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_BWD    0x20000000   /*  RVL-Dec  negative sum-bitcounter during RVL-fwd-decoding (long+shrt) */
#define RVLC_ERROR_FORBIDDEN_CW_DETECTED_FWD             0x08000000   /*  RVL-Dec  forbidden codeword detected fwd (long+shrt) */
#define RVLC_ERROR_FORBIDDEN_CW_DETECTED_BWD             0x04000000   /*  RVL-Dec  forbidden codeword detected bwd (long+shrt) */



void CRvlc_Read (CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                 HANDLE_FDK_BITSTREAM    bs);

void CRvlc_Decode (CAacDecoderChannelInfo  *pAacDecoderChannelInfo,
                   CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                   HANDLE_FDK_BITSTREAM     bs);

/**
 * \brief performe sanity checks to the channel data corresponding to one channel element.
 * \param pAacDecoderChannelInfo
 * \param pAacDecoderStaticChannelInfo
 * \param elChannels amount of channels of the channel element.
 */
void CRvlc_ElementCheck (
        CAacDecoderChannelInfo *pAacDecoderChannelInfo[],
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        const UINT flags,
        const INT elChannels
        );




#endif /* RVLC_H */
