/************************  FDK PCM up/downmixing module  *********************

                        (C) Copyright Fraunhofer IIS 2008
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
   Description: Declares functions to interface with the PCM downmix processing
                module.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

*******************************************************************************/

#ifndef _PCMUTILS_LIB_H_
#define _PCMUTILS_LIB_H_

#include "machine_type.h"
#include "common_fix.h"
#include "FDK_audio.h"

/* ------------------------ *
 *     MODULE SETTINGS:     *
 * ------------------------ */
/* #define PCM_UPMIX_ENABLE */       /*!< Generally enable up mixing.                           */
#define PCM_DOWNMIX_ENABLE           /*!< Generally enable down mixing.                         */
#define DVB_MIXDOWN_ENABLE           /*!< Enable this to support DVB ancillary data for encoder
                                          assisted downmixing of MPEG-4 AAC and
                                          MPEG-1/2 layer 2 streams. PCM_DOWNMIX_ENABLE has to
                                          be enabled, too!                                      */
#define MPEG_PCE_MIXDOWN_ENABLE      /*!< Enable this to support MPEG matrix mixdown with a
                                          coefficient carried in the PCE. PCM_DOWNMIX_ENABLE
                                          has to be enabled, too!                               */
/* #define ARIB_MIXDOWN_ENABLE */    /*!< Enable modifications to the MPEG PCE mixdown method
                                          to fulfill ARIB standard. MPEG_PCE_MIXDOWN_ENABLE has
                                          to be set.                                            */

/* ------------------------ *
 *     ERROR CODES:         *
 * ------------------------ */
typedef enum
{
  PCMDMX_OK              = 0x0,    /*!< No error happened.                                     */
  PCMDMX_OUT_OF_MEMORY   = 0x2,    /*!< Not enough memory to set up an instance of the module. */
  PCMDMX_UNKNOWN         = 0x5,    /*!< Error condition is of unknown reason, or from a third
                                          party module.                                        */
  PCMDMX_INVALID_HANDLE,           /*!< The given instance handle is not valid.                */
  PCMDMX_INVALID_ARGUMENT,         /*!< One of the parameters handed over is invalid.          */
  PCMDMX_INVALID_CH_CONFIG,        /*!< The given channel configuration is not supported and
                                          thus no processing was performed.                    */
  PCMDMX_INVALID_MODE,             /*!< The set configuration/mode is not applicable.          */
  PCMDMX_UNKNOWN_PARAM,            /*!< The handed parameter is not known/supported.           */
  PCMDMX_UNABLE_TO_SET_PARAM,      /*!< Unable to set the specific parameter. Most probably
                                          the value ist out of range.                          */
  PCMDMX_CORRUPT_ANC_DATA          /*!< The read ancillary data was corrupt.                   */

} PCMDMX_ERROR;


/* ------------------------ *
 *     RUNTIME PARAMS:      *
 * ------------------------ */
typedef enum
{
  DMX_BS_DATA_EXPIRY_FRAME,          /*!< The number of frames without new metadata that have to
                                            go by before the bitstream data expires. The value 0
                                            disables expiry.                                     */
  DMX_BS_DATA_DELAY,                 /*!< The number of delay frames of the output samples
                                            compared to the bitstream data.                      */
  NUMBER_OF_OUTPUT_CHANNELS ,        /*!< The number of output channels (equals the number of
                                            channels processed by the audio output setup).       */
  DUAL_CHANNEL_DOWNMIX_MODE          /*!< Downmix mode for two channel audio data.               */

} PCMDMX_PARAM;


typedef enum
{
  STEREO_MODE = 0x0,           /*!< Leave stereo signals as they are.                            */
  CH1_MODE    = 0x1,           /*!< Create a dual mono output signal from channel 1.             */
  CH2_MODE    = 0x2,           /*!< Create a dual mono output signal from channel 2.             */
  MIXED_MODE  = 0x3            /*!< Create a dual mono output signal by mixing the two channels. */

} DUAL_CHANNEL_MODE;


/* ------------------------ *
 *     MODULES INTERFACE:   *
 * ------------------------ */
typedef struct PCM_DMX_INSTANCE *HANDLE_PCM_DOWNMIX;

/* Modules reset flags */
#define PCMDMX_RESET_PARAMS   ( 1 )
#define PCMDMX_RESET_BS_DATA  ( 2 )
#define PCMDMX_RESET_FULL     ( PCMDMX_RESET_PARAMS | PCMDMX_RESET_BS_DATA )

#ifdef __cplusplus
extern "C"
{
#endif

/** Open and initialize an instance of the PCM downmix module
 * @param [out] Pointer to a buffer receiving the handle of the new instance.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Open (
    HANDLE_PCM_DOWNMIX *pSelf
  );

/** Set one parameter for one instance of the PCM downmix module.
 * @param [in] Handle of PCM downmix instance.
 * @param [in] Parameter to be set.
 * @param [in] Parameter value.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_SetParam (
    HANDLE_PCM_DOWNMIX  self,
    PCMDMX_PARAM        param,
    UINT                value
  );

/** Read the ancillary data transported in DSEs of DVB streams with MPEG-4 content
 * @param [in] Handle of PCM downmix instance.
 * @param [in] Pointer to ancillary data buffer.
 * @param [in] Size of ancillary data.
 * @param [in] Flag indicating wheter the ancillary data is from a MPEG-1/2 or an MPEG-4 stream.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_ReadDvbAncData (
    HANDLE_PCM_DOWNMIX  self,
    UCHAR *pAncDataBuf,
    UINT   ancDataBytes,
    int    isMpeg2
  );

/** Set the matrix mixdown information extracted from the PCE of an AAC bitstream.
 *  Note: Call only if matrix_mixdown_idx_present is true.
 * @param [in] Handle of PCM downmix instance.
 * @param [in] Matrix mixdown index present flag extracted from PCE.
 * @param [in] The 2 bit matrix mixdown index extracted from PCE.
 * @param [in] The pseudo surround enable flag extracted from PCE.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_SetMatrixMixdownFromPce (
    HANDLE_PCM_DOWNMIX  self,
    int                 matrixMixdownPresent,
    int                 matrixMixdownIdx,
    int                 pseudoSurroundEnable
  );

/** Reset the module.
 * @param [in] Handle of PCM downmix instance.
 * @param [in] Flags telling which parts of the module shall be reset.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Reset (
    HANDLE_PCM_DOWNMIX  self,
    UINT                flags
  );

/** Apply down or up mixing.
 *
 * \param [in]    Handle of PCM downmix module instance.
 * \param [inout] Pointer to time buffer with decoded PCM samples.
 * \param [in]    Pointer where the amount of output samples is returned into.
 * \param [inout] Pointer where the amount of output channels is returned into.
 * \param [in]    Flag which indicates if output time data are writtern interleaved or as subsequent blocks.
 * \param [inout] Array were the corresponding channel type for each output audio channel is stored into.
 * \param [inout] Array were the corresponding channel type index for each output audio channel is stored into.
 * \param [in]    Array containing the output channel mapping to be used (From MPEG PCE ordering to whatever is required).
 *
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_ApplyFrame (
    HANDLE_PCM_DOWNMIX      self,
    INT_PCM                *pPcmBuf,
    UINT                    frameSize,
    INT                    *nChannels,

    int                     fInterleaved,
    AUDIO_CHANNEL_TYPE      channelType[],
    UCHAR                   channelIndices[],
    const UCHAR             channelMapping[][8]
  );

/** Close an instance of the PCM downmix module.
 * @param [inout] Pointer to a buffer containing the handle of the instance.
 * @returns Returns an error code.
 **/
PCMDMX_ERROR pcmDmx_Close (
    HANDLE_PCM_DOWNMIX *pSelf
  );

/** Get library info for this module.
 * @param [out] Pointer to an allocated LIB_INFO structure.
 * @returns Returns an error code.
 */
PCMDMX_ERROR pcmDmx_GetLibInfo( LIB_INFO *info );


#ifdef __cplusplus
}
#endif

#endif  /* _PCMUTILS_LIB_H_ */
