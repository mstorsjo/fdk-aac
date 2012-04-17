/*****************************  MPEG-4 AAC Encoder  **************************

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
   Author(s): Manuel Jander
   Description: Audio Specific Config writer

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef TPENC_ASC_H
#define TPENC_ASC_H

/**
 * \brief Get channel config from channel mode.
 *
 * \param channel_mode channel mode
 *
 * \return chanel config
 */
int getChannelConfig( CHANNEL_MODE channel_mode );

/**
 * \brief Write a Program Config Element.
 *
 * \param hBs bitstream handle into which the PCE is appended
 * \param channelMode the channel mode to be used
 * \param sampleRate the sample rate
 * \param instanceTagPCE the instance tag of the Program Config Element
 * \param profile the MPEG Audio profile to be used
 * \param matrix mixdown gain
 * \param pseudo surround indication
 * \param reference bitstream position for alignment
 * \return zero on success, non-zero on failure.
 */
int transportEnc_writePCE(
        HANDLE_FDK_BITSTREAM hBs,
        CHANNEL_MODE         channelMode,
        INT                  sampleRate,
        int                  instanceTagPCE,
        int                  profile,
        int                  matrixMixdownA,
        int                  pseudoSurroundEnable,
        UINT                 alignAnchor
        );

/**
 * \brief Get the bit count required by a Program Config Element
 *
 * \param channelMode the channel mode to be used
 * \param matrix mixdown gain
 * \param bit offset at which the PCE would start
 * \return the amount of bits required for the PCE including the given bit offset.
 */
int transportEnc_GetPCEBits(
        CHANNEL_MODE channelMode,
        int          matrixMixdownA,
        int          bits
        );

#endif /* TPENC_ASC_H */

