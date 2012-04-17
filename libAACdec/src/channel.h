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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef CHANNEL_H
#define CHANNEL_H

#include "common_fix.h"

#include "FDK_bitstream.h"
#include "channelinfo.h"
#include "tpdec_lib.h"

/**
 * \brief Init codeBook SFB indices (section data) with HCB_ESC. Useful for bitstreams
 * which do not have any section data, but still SFB's (scale factor bands). This has
 * the effect that upto the amount of transmitted SFB are treated as non-zero.
 * \param pAacDecoderChannelInfo channel info structure containing a valid icsInfo struct.
 */
void CChannel_CodebookTableInit(CAacDecoderChannelInfo *pAacDecoderChannelInfo);

/**
 * \brief decode a channel element. To be called after CChannelElement_Read()
 * \param pAacDecoderChannelInfo pointer to channel data struct. Depending on el_channels either one or two.
 * \param pSamplingRateInfo pointer to sample rate information structure
 * \param el_channels amount of channels of the element to be decoded.
 * \param output pointer to time domain output buffer (ACELP)
 * \param stride factor for accessing output
 */
void CChannelElement_Decode ( CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
                              CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[2],
                              SamplingRateInfo *pSamplingRateInfo,
                              UINT flags,
                              int el_channels );


/**
 * \brief Read channel element of given type from bitstream.
 * \param hBs bitstream handle to access bitstream data.
 * \param pAacDecoderChannelInfo pointer array to store channel information.
 * \param aot Audio Object Type
 * \param pSamplingRateInfo sampling rate info table.
 * \param flags parser guidance flags
 * \param numberOfChannels amoun of channels contained in the object to be parsed.
 * \param epConfig the current epConfig value obtained from the Audio Specific Config.
 * \param pTp transport decoder handle required for ADTS CRC checking.
 * ...
 * \return an AAC_DECODER_ERROR error code.
 */
AAC_DECODER_ERROR CChannelElement_Read(HANDLE_FDK_BITSTREAM hBs,
                                       CAacDecoderChannelInfo *pAacDecoderChannelInfo[],
                                       CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
                                       const AUDIO_OBJECT_TYPE aot,
                                       const SamplingRateInfo *pSamplingRateInfo,
                                       const UINT  flags,
                                       const UINT  frame_length,
                                       const UCHAR numberOfChannels,
                                       const SCHAR epConfig,
                                       HANDLE_TRANSPORTDEC pTpDec
                                       );

#endif /* #ifndef CHANNEL_H */
