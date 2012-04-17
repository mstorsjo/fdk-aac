/*****************************  MPEG-4 AAC Decoder  **************************

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


   $Id$
   Author(s):   Josef Hoepfl
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef AAC_RAM_H
#define AAC_RAM_H

#include "common_fix.h"

#include "aacdecoder.h"

#include "channel.h"

#include "aacdec_hcr_types.h"
#include "aacdec_hcr.h"

/* End of formal fix.h */

#define MAX_SYNCHS        10
#define SAMPL_FREQS       12

H_ALLOC_MEM(AacDecoder, AAC_DECODER_INSTANCE)

H_ALLOC_MEM(DrcInfo, CDrcInfo)

H_ALLOC_MEM(AacDecoderStaticChannelInfo, CAacDecoderStaticChannelInfo)
H_ALLOC_MEM(AacDecoderChannelInfo, CAacDecoderChannelInfo)
H_ALLOC_MEM(OverlapBuffer, FIXP_DBL)

H_ALLOC_MEM_OVERLAY(WorkBufferCore1, CWorkBufferCore1)
H_ALLOC_MEM_OVERLAY(WorkBufferCore2, FIXP_DBL)


#endif /* #ifndef AAC_RAM_H */
