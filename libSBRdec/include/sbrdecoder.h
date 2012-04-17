/************************  Fraunhofer IIS SBR decoder library ******************

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
   Author(s):
   Description: SBR decoder front-end prototypes and definitions.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#ifndef __SBRDECODER_H
#define __SBRDECODER_H

#include "common_fix.h"

#include "FDK_bitstream.h"
#include "FDK_audio.h"


#define SBR_DEBUG_EXTHLP "\
--- SBR ---\n\
    0x00000010 Ancillary data and SBR-Header\n\
    0x00000020 SBR-Side info\n\
    0x00000040 Decoded SBR-bitstream data, e.g. envelope data\n\
    0x00000080 SBR-Bitstream statistics\n\
    0x00000100 Miscellaneous SBR-messages\n\
    0x00000200 SBR-Energies and gains in the adjustor\n\
    0x00000400 Fatal SBR errors\n\
    0x00000800 Transposer coefficients for inverse filtering\n\
"

/* Capability flags */
#define CAPF_SBR_LP          0x00000001   /*!< Flag indicating library's capability of Low Power mode.            */
#define CAPF_SBR_HQ          0x00000002   /*!< Flag indicating library's capability of High Quality mode.         */
#define CAPF_SBR_DRM_BS      0x00000004   /*!< Flag indicating library's capability to decode DRM SBR data.       */
#define CAPF_SBR_CONCEALMENT 0x00000008   /*!< Flag indicating library's capability to conceal erroneous frames.  */
#define CAPF_SBR_DRC         0x00000010   /*!< Flag indicating library's capability for Dynamic Range Control.    */
#define CAPF_SBR_PS_MPEG     0x00000020   /*!< Flag indicating library's capability to do MPEG Parametric Stereo. */
#define CAPF_SBR_PS_DRM      0x00000040   /*!< Flag indicating library's capability to do DRM Parametric Stereo.  */

typedef enum
{
  SBRDEC_OK = 0,                       /*!< All fine.                                                       */
  /* SBRDEC_CONCEAL,                                                                                        */
  /* SBRDEC_NOSYNCH,                                                                                        */
  /* SBRDEC_ILLEGAL_PROGRAM,                                                                                */
  /* SBRDEC_ILLEGAL_TAG,                                                                                    */
  /* SBRDEC_ILLEGAL_CHN_CONFIG,                                                                             */
  /* SBRDEC_ILLEGAL_SECTION,                                                                                */
  /* SBRDEC_ILLEGAL_SCFACTORS,                                                                              */
  /* SBRDEC_ILLEGAL_PULSE_DATA,                                                                             */
  /* SBRDEC_MAIN_PROFILE_NOT_IMPLEMENTED,                                                                   */
  /* SBRDEC_GC_NOT_IMPLEMENTED,                                                                             */
  /* SBRDEC_ILLEGAL_PLUS_ELE_ID,                                                                            */
  SBRDEC_CREATE_ERROR,                 /*!<                                                                 */
  SBRDEC_NOT_INITIALIZED,              /*!<                                                                 */
  SBRDEC_MEM_ALLOC_FAILED,             /*!< Memory allocation failed. Probably not enough memory available. */
  SBRDEC_PARSE_ERROR,                  /*!<                                                                 */
  SBRDEC_UNSUPPORTED_CONFIG,           /*!<                                                                 */
  SBRDEC_SET_PARAM_FAIL                /*!<                                                                 */
} SBR_ERROR;

typedef enum
{
  SBR_SYSTEM_BITSTREAM_DELAY,          /*!< System: Switch to enable an additional SBR bitstream delay of one frame. */
  SBR_QMF_MODE,                        /*!< Set QMF mode, either complex or low power. */
  SBR_LD_QMF_TIME_ALIGN,               /*!< Set QMF type, either LD-MPS or CLDFB. Relevant for ELD streams only. */
  SBR_BS_INTERRUPTION                  /*!< Signal bit stream interruption. Value is ignored. */
} SBRDEC_PARAM;

typedef struct SBR_DECODER_INSTANCE *HANDLE_SBRDECODER;


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * \brief        Allocates and initializes one SBR decoder instance.
 * \param pSelf  Pointer to where a SBR decoder handle is copied into.
 * \return       Error code.
 */
SBR_ERROR sbrDecoder_Open ( HANDLE_SBRDECODER *pSelf );

/**
 * \brief  Initialize a SBR decoder runtime instance. Must be called before decoding starts.
 *
 * \param self             Handle to a SBR decoder instance.
 * \param sampleRateIn     Input samplerate of the SBR decoder instance.
 * \param sampleRateOut    Output samplerate of the SBR decoder instance.
 * \param samplesPerFrame  Number of samples per frames.
 * \param coreCodec        Audio Object Type (AOT) of the core codec.
 * \param elementID        Table with MPEG-4 element Ids in canonical order.
 * \param forceReset       Flag that enforces a complete decoder reset.
 *
 * \return  Error code.
 */
SBR_ERROR sbrDecoder_InitElement (
        HANDLE_SBRDECODER       self,
        const int               sampleRateIn,
        const int               sampleRateOut,
        const int               samplesPerFrame,
        const AUDIO_OBJECT_TYPE coreCodec,
        const MP4_ELEMENT_ID    elementID,
        const int               elementIndex
        );

/**
 * \brief pass out of band SBR header to SBR decoder
 *
 * \param self         Handle to a SBR decoder instance.
 * \param hBs          bit stream handle data source.
 * \param elementID    SBR element ID.
 * \param elementIndex SBR element index.
 *
 * \return  Error code.
 */
INT sbrDecoder_Header (
        HANDLE_SBRDECODER       self,
        HANDLE_FDK_BITSTREAM    hBs,
        const INT sampleRateIn,
        const INT sampleRateOut,
        const INT samplesPerFrame,
        const AUDIO_OBJECT_TYPE coreCodec,
        const MP4_ELEMENT_ID    elementID,
        const INT               elementIndex
        );

/**
 * \brief        Set a parameter of the SBR decoder runtime instance.
 * \param self   SBR decoder handle.
 * \param param  Parameter which will be set if successfull.
 * \param value  New parameter value.
 * \return       Error code.
 */
SBR_ERROR sbrDecoder_SetParam ( HANDLE_SBRDECODER   self,
                                const SBRDEC_PARAM  param,
                                const INT           value );

/**
 * \brief  Feed DRC channel data into a SBR decoder runtime instance.
 *
 * \param self                    SBR decoder handle.
 * \param ch                      Channel number to which the DRC data is associated to.
 * \param numBands                Number of DRC bands.
 * \param pNextFact_mag           Pointer to a table with the DRC factor magnitudes.
 * \param nextFact_exp            Exponent for all DRC factors.
 * \param drcInterpolationScheme  DRC interpolation scheme.
 * \param winSequence             Window sequence from core coder (eight short or one long window).
 * \param pBandTop                Pointer to a table with the top borders for all DRC bands.
 *
 * \return  Error code.
 */
SBR_ERROR sbrDecoder_drcFeedChannel ( HANDLE_SBRDECODER  self,
                                      INT                ch,
                                      UINT               numBands,
                                      FIXP_DBL          *pNextFact_mag,
                                      INT                nextFact_exp,
                                      SHORT              drcInterpolationScheme,
                                      UCHAR              winSequence,
                                      USHORT            *pBandTop );

/**
 * \brief  Disable SBR DRC for a certain channel.
 *
 * \param hSbrDecoder  SBR decoder handle.
 * \param ch           Number of the channel that has to be disabled.
 *
 * \return  None.
 */
void sbrDecoder_drcDisable ( HANDLE_SBRDECODER  self,
                             INT                ch );


/**
 * \brief  Parse one SBR element data extension data block. The bit stream position will
 *         be placed at the end of the SBR payload block. The remaining bits will be returned
 *         into *count if a payload length is given (byPayLen > 0). If no SBR payload length is
 *         given (bsPayLen < 0) then the bit stream position on return will be random after this
 *         function call in case of errors, and any further decoding will be completely pointless.
 *
 * \param self           SBR decoder handle.
 * \param hBs            Bit stream handle as data source.
 * \param count          Pointer to an integer where the amount of parsed SBR payload bits is stored into.
 * \param bsPayLen       If > 0 this value is the SBR payload length. If < 0, the SBR payload length is unknown.
 * \param flags          CRC flag (0: EXT_SBR_DATA; 1: EXT_SBR_DATA_CRC)
 * \param prev_element   Previous MPEG-4 element ID.
 * \param element_index  Index of the current element.
 *
 * \return  Error code.
 */
SBR_ERROR sbrDecoder_Parse (
        HANDLE_SBRDECODER  self,
        HANDLE_FDK_BITSTREAM hBs,
        int *count,
        int  bsPayLen,
        int  crcFlag,
        MP4_ELEMENT_ID prev_element,
        int element_index,
        int fGlobalIndependencyFlag
        );

/**
 * \brief  This function decodes the given SBR bitstreams and applies SBR to the given time data.
 * 
 * SBR-processing works InPlace. I.e. the calling function has to provide
 * a time domain buffer timeData which can hold the completely decoded
 * result.
 *
 * Left and right channel are read and stored according to the
 * interleaving flag, frame length and number of channels.
 *
 * \param self            Handle of an open SBR decoder instance.
 * \param hSbrBs          SBR Bitstream handle.
 * \param timeData        Pointer to input and finally upsampled output data.
 * \param numChannels     Pointer to a buffer holding the number of channels in time data buffer.
 * \param sampleRate      Output samplerate.
 * \param channelMapping  Channel mapping indices.
 * \param interleaved     Flag indicating if time data is stored interleaved (1: Interleaved time data, 0: non-interleaved timedata).
 * \param coreDecodedOk   Flag indicating if the core decoder did not find any error (0: core decoder found errors, 1: no errors).
 * \param psDecoded       Pointer to a buffer holding a flag. Input: PS is possible, Output: PS has been rendered.
 *
 * \return  Error code.
 */
SBR_ERROR sbrDecoder_Apply ( HANDLE_SBRDECODER    self,
                             INT_PCM             *timeData,
                             int                 *numChannels,
                             int                 *sampleRate,
                             const UCHAR          channelMapping[(6)],
                             const int            interleaved,
                             const int            coreDecodedOk,
                             UCHAR               *psDecoded );


/**
 * \brief       Close SBR decoder instance and free memory.
 * \param self  SBR decoder handle.
 * \return      Error Code.
 */
SBR_ERROR sbrDecoder_Close ( HANDLE_SBRDECODER *self );


/**
 * \brief       Get SBR decoder library information.
 * \param info  Pointer to a LIB_INFO struct, where library information is written to.
 * \return      0 on success, -1 if invalid handle or if no free element is available to write information to.
 */
INT sbrDecoder_GetLibInfo( LIB_INFO *info );    


#ifdef __cplusplus
}
#endif

#endif
