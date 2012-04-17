/**********************  Fraunhofer IIS FDK AAC Encoder lib  ******************

                        (C) Copyright Fraunhofer IIS (2011)
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
   Author(s): V. Bacigalupo
   Description: Metadata Encoder library interface functions

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef _METADATA_MAIN_H
#define _METADATA_MAIN_H


/* Includes ******************************************************************/
#include "aacenc_lib.h"
#include "aacenc.h"


/* Defines *******************************************************************/

/* Data Types ****************************************************************/

typedef enum {
    METADATA_OK                    = 0x0000,   /*!< No error happened. All fine. */
    METADATA_INVALID_HANDLE        = 0x0020,   /*!< Handle passed to function call was invalid. */
    METADATA_MEMORY_ERROR          = 0x0021,   /*!< Memory allocation failed. */
    METADATA_INIT_ERROR            = 0x0040,   /*!< General initialization error. */
    METADATA_ENCODE_ERROR          = 0x0060    /*!< The encoding process was interrupted by an unexpected error. */

} FDK_METADATA_ERROR;

/**
 *  Meta Data handle.
 */
typedef struct FDK_METADATA_ENCODER *HANDLE_FDK_METADATA_ENCODER;


/**
 * \brief  Open a Meta Data instance.
 *
 * \param phMetadataEnc         A pointer to a Meta Data handle to be allocated. Initialized on return.
 *
 * \return
 *          - METADATA_OK, on succes.
 *          - METADATA_INVALID_HANDLE, METADATA_MEMORY_ERROR, on failure.
 */
FDK_METADATA_ERROR FDK_MetadataEnc_Open(
        HANDLE_FDK_METADATA_ENCODER     *phMetadataEnc
        );


/**
 * \brief  Initialize a Meta Data instance.
 *
 * \param hMetadataEnc          Meta Data handle.
 * \param resetStates           Indication for full reset of all states.
 * \param metadataMode          Configures metat data output format (0,1,2).
 * \param audioDelay            Delay cause by the audio encoder.
 * \param frameLength           Number of samples to be processes within one frame.
 * \param sampleRate            Sampling rat in Hz of audio input signal.
 * \param nChannels             Number of audio input channels.
 * \param channelMode           Channel configuration which is used by the encoder.
 * \param channelOrder          Channel order of the input data. (WAV, MPEG)
 *
 * \return
 *          - METADATA_OK, on succes.
 *          - METADATA_INVALID_HANDLE, METADATA_INIT_ERROR, on failure.
 */
FDK_METADATA_ERROR FDK_MetadataEnc_Init(
        HANDLE_FDK_METADATA_ENCODER      hMetadataEnc,
        const INT                        resetStates,
        const INT                        metadataMode,
        const INT                        audioDelay,
        const UINT                       frameLength,
        const UINT                       sampleRate,
        const UINT                       nChannels,
        const CHANNEL_MODE               channelMode,
        const CHANNEL_ORDER              channelOrder
        );


/**
 * \brief  Calculate Meta Data processing.
 *
 * This function treats all step necessary for meta data processing.
 * - Receive new meta data and make usable.
 * - Calculate DRC compressor and extract meta data info.
 * - Make meta data available for extern use.
 * - Apply audio data and meta data delay compensation.
 *
 * \param hMetadataEnc          Meta Data handle.
 * \param pAudioSamples         Pointer to audio input data. Existing function overwrites audio data with delayed audio samples.
 * \param nAudioSamples         Number of input audio samples to be prcessed.
 * \param pMetadata             Pointer to Metat Data input.
 * \param ppMetaDataExtPayload  Pointer to extension payload array. Filled on return.
 * \param nMetaDataExtensions   Pointer to variable to describe number of available extension payloads. Filled on return.
 * \param matrix_mixdown_idx    Pointer to variable for matrix mixdown coefficient. Filled on return.
 *
 * \return
 *          - METADATA_OK, on succes.
 *          - METADATA_INVALID_HANDLE, METADATA_ENCODE_ERROR, on failure.
 */
FDK_METADATA_ERROR FDK_MetadataEnc_Process(
        HANDLE_FDK_METADATA_ENCODER      hMetadataEnc,
        INT_PCM * const                  pAudioSamples,
        const INT                        nAudioSamples,
        const AACENC_MetaData * const    pMetadata,
        AACENC_EXT_PAYLOAD **            ppMetaDataExtPayload,
        UINT *                           nMetaDataExtensions,
        INT *                            matrix_mixdown_idx
        );


/**
 * \brief  Close the Meta Data instance.
 *
 * Deallocate instance and free whole memory.
 *
 * \param phMetaData            Pointer to the Meta Data handle to be deallocated.
 *
 * \return
 *          - METADATA_OK, on succes.
 *          - METADATA_INVALID_HANDLE, on failure.
 */
FDK_METADATA_ERROR FDK_MetadataEnc_Close(
        HANDLE_FDK_METADATA_ENCODER     *phMetaData
        );


/**
 * \brief  Get Meta Data Encoder delay.
 *
 * \param hMetadataEnc          Meta Data Encoder handle.
 *
 * \return  Delay caused by Meta Data module.
 */
INT FDK_MetadataEnc_GetDelay(
        HANDLE_FDK_METADATA_ENCODER      hMetadataEnc
        );


#endif /* _METADATA_MAIN_H */

