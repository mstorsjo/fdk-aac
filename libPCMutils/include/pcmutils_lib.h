
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2012 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/************************  FDK PCM up/downmixing module  *********************

   Author(s):   Christian Griebel
   Description: Declares functions to interface with the PCM downmix processing
                module.

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
