/*****************************  MPEG-4 AAC Decoder  **************************

                        (C) Copyright Fraunhofer IIS (2002)
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
   Author(s): Josef Hoepfl
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "aac_ram.h"
#include "aac_rom.h"

#define WORKBUFFER1_TAG 0
#define WORKBUFFER2_TAG 1

/*! The structure AAC_DECODER_INSTANCE is the top level structure holding all decoder configurations,
    handles and structs.
 */
C_ALLOC_MEM(AacDecoder, AAC_DECODER_INSTANCE, 1)

/*!
  \name StaticAacData

  Static memory areas, must not be overwritten in other sections of the decoder
*/
/* @{ */

/*! The structure CAacDecoderStaticChannelInfo contains the static sideinfo which is needed
    for the decoding of one aac channel. <br>
    Dimension: #AacDecoderChannels                                                      */
C_ALLOC_MEM2(AacDecoderStaticChannelInfo, CAacDecoderStaticChannelInfo, 1, (6))

/*! The structure CAacDecoderChannelInfo contains the dynamic sideinfo which is needed
    for the decoding of one aac channel. <br>
    Dimension: #AacDecoderChannels                                                      */
C_ALLOC_MEM2(AacDecoderChannelInfo, CAacDecoderChannelInfo, 1, (6))

/*! Overlap buffer */
C_ALLOC_MEM2(OverlapBuffer, FIXP_DBL, OverlapBufferSize, (6))

C_ALLOC_MEM(DrcInfo, CDrcInfo, 1)

/* @} */

/*!
  \name DynamicAacData

  Dynamic memory areas, might be reused in other algorithm sections,
  e.g. the sbr decoder
*/
C_ALLOC_MEM_OVERLAY(WorkBufferCore2, FIXP_DBL, ((6)*1024), SECT_DATA_L2, WORKBUFFER2_TAG)


C_ALLOC_MEM_OVERLAY(WorkBufferCore1, CWorkBufferCore1, 1, SECT_DATA_L1, WORKBUFFER1_TAG)

/* @{ */


/* @} */



