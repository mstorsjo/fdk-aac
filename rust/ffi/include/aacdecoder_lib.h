/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung
e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#ifndef AACDECODER_LIB_H
#define AACDECODER_LIB_H

#include "aacdec_errorcodes.h"
#include "aacdec_info.h"


/*! \enum  AAC_MD_PROFILE
 *  \brief The available metadata profiles which are mostly related to downmixing. The values define the arguments
 *         for the use with parameter ::AAC_METADATA_PROFILE.
 */
typedef enum {
  AAC_MD_PROFILE_MPEG_STANDARD =
      0, /*!< The standard profile creates a mixdown signal based on the
            advanced downmix metadata (from a DSE). The equations and default
            values are defined in ISO/IEC 14496:3 Ammendment 4. Any other
            (legacy) downmix metadata will be ignored. No other parameter will
            be modified.         */
  AAC_MD_PROFILE_MPEG_LEGACY =
      1, /*!< This profile behaves identical to the standard profile if advanced
              downmix metadata (from a DSE) is available. If not, the
            matrix_mixdown information embedded in the program configuration
            element (PCE) will be applied. If neither is the case, the module
            creates a mixdown using the default coefficients as defined in
            ISO/IEC 14496:3 AMD 4. The profile can be used to support legacy
            digital TV (e.g. DVB) streams.           */
  AAC_MD_PROFILE_MPEG_LEGACY_PRIO =
      2, /*!< Similar to the ::AAC_MD_PROFILE_MPEG_LEGACY profile but if both
            the advanced (ISO/IEC 14496:3 AMD 4) and the legacy (PCE) MPEG
            downmix metadata are available the latter will be applied.
          */
  AAC_MD_PROFILE_ARIB_JAPAN =
      3 /*!< Downmix creation as described in ABNT NBR 15602-2. But if advanced
             downmix metadata (ISO/IEC 14496:3 AMD 4) is available it will be
             preferred because of the higher resolutions. In addition the
           metadata expiry time will be set to the value defined in the ARIB
           standard (see ::AAC_METADATA_EXPIRY_TIME).
         */
} AAC_MD_PROFILE;

/*! \enum  AAC_DRC_DEFAULT_PRESENTATION_MODE_OPTIONS
 *  \brief Options for handling of DRC parameters, if presentation mode is not indicated in bitstream
 */
typedef enum {
  AAC_DRC_PARAMETER_HANDLING_DISABLED = -1, /*!< DRC parameter handling
                                               disabled, all parameters are
                                               applied as requested. */
  AAC_DRC_PARAMETER_HANDLING_ENABLED =
      0, /*!< Apply changes to requested DRC parameters to prevent clipping. */
  AAC_DRC_PRESENTATION_MODE_1_DEFAULT =
      1, /*!< Use DRC presentation mode 1 as default (e.g. for Nordig) */
  AAC_DRC_PRESENTATION_MODE_2_DEFAULT =
      2, /*!< Use DRC presentation mode 2 as default (e.g. for DTG DBook) */
  AAC_DRC_PARAMETER_HANDLING_AOSP = 3 /*!< Apply changes to requested DRC
                                         parameters for Android Open Source
                                         Project (AOSP) */
} AAC_DRC_DEFAULT_PRESENTATION_MODE_OPTIONS;

/**
 * \brief AAC decoder setting parameters
 */
typedef enum {
  AAC_PCM_DUAL_CHANNEL_OUTPUT_MODE = 0x0002, /*!< Defines how the decoder
                                                processes two channel signals:
                                                  0: Leave both signals as they
                                                are (default). 1: Create a mono
                                                output signal from channel 1. 2:
                                                Create a mono output signal from
                                                channel 2. 3: Create a mono
                                                output signal by mixing both
                                                channels (L' = R' = 0.5*Ch1 +
                                                0.5*Ch2). */
  AAC_PCM_OUTPUT_CHANNEL_MAPPING =
      0x0003, /*!< Output buffer channel ordering. 0: MPEG PCE style order, 1:
                 WAV file channel order (default), 2: CICP channel order. */
  AAC_PCM_MIN_OUTPUT_CHANNELS =
      0x0005, /*!< Minimum number of PCM output channels. If higher than the
                 number of encoded audio channels, a simple channel extension is
                 applied (see note 3 for exceptions). \n -1, 0: Disable channel
                 extension feature. The decoder output contains the same number
                 of channels as the encoded bitstream. \n 1:    This value is
                 currently needed only together with the mix-down feature. See
                          ::AAC_PCM_MAX_OUTPUT_CHANNELS and note 2 below. \n
                    2:    Encoded mono signals will be duplicated to achieve a
                 2/0/0.0 channel output configuration. \n 6:    The decoder
                 tries to reorder encoded signals with less than six channels to
                 achieve a 3/0/2.1 channel output signal. Missing channels will
                 be filled with a zero signal. If reordering is not possible the
                 empty channels will simply be appended. Only available if
                 instance is configured to support multichannel output. \n 8:
                 The decoder tries to reorder encoded signals with less than
                 eight channels to achieve a 3/0/4.1 channel output signal.
                 Missing channels will be filled with a zero signal. If
                 reordering is not possible the empty channels will simply be
                          appended. Only available if instance is configured to
                 support multichannel output.\n NOTE: \n
                     1. The channel signaling (CStreamInfo::pChannelType and
                 CStreamInfo::pChannelIndices) will not be modified. Added empty
                 channels will be signaled with channel type
                        AUDIO_CHANNEL_TYPE::ACT_NONE. \n
                     2. If the parameter value is greater than that of
                 ::AAC_PCM_MAX_OUTPUT_CHANNELS both will be set to the same
                 value. \n
                     3. This parameter will be ignored if the number of encoded
                 audio channels is greater than 8. */
  AAC_PCM_MAX_OUTPUT_CHANNELS =
      0x0006, /*!< Maximum number of PCM output channels. If lower than the
                 number of encoded audio channels, downmixing is applied
                 accordingly (see note 3 for exceptions). If dedicated metadata
                 is available in the stream it will be used to achieve better
                 mixing results. \n -1, 0: Disable downmixing feature. The
                 decoder output contains the same number of channels as the
                 encoded bitstream. \n 1:    All encoded audio configurations
                 with more than one channel will be mixed down to one mono
                 output signal. \n 2:    The decoder performs a stereo mix-down
                 if the number encoded audio channels is greater than two. \n 6:
                 If the number of encoded audio channels is greater than six the
                 decoder performs a mix-down to meet the target output
                 configuration of 3/0/2.1 channels. Only available if instance
                 is configured to support multichannel output. \n 8:    This
                 value is currently needed only together with the channel
                 extension feature. See ::AAC_PCM_MIN_OUTPUT_CHANNELS and note 2
                 below. Only available if instance is configured to support
                 multichannel output. \n NOTE: \n
                     1. Down-mixing of any seven or eight channel configuration
                 not defined in ISO/IEC 14496-3 PDAM 4 is not supported by this
                 software version. \n
                     2. If the parameter value is greater than zero but smaller
                 than ::AAC_PCM_MIN_OUTPUT_CHANNELS both will be set to same
                 value. \n
                     3. This parameter will be ignored if the number of encoded
                 audio channels is greater than 8. */
  AAC_PCM_LIMITER_ENABLE = 0x0010,      /*!< Enable signal level limiting. \n
                                             -1: Auto-config. Enable limiter for all
                                           non-lowdelay configurations by default. \n
                                              0: Disable limiter in general. \n
                                              1: Enable limiter always. */
  AAC_PCM_LIMITER_ATTACK_TIME = 0x0011, /*!< Signal level limiting attack time
                                           in ms. Default configuration is 15
                                           ms. Adjustable range from 1 ms to 15
                                           ms. */
  AAC_PCM_LIMITER_RELEAS_TIME = 0x0012, /*!< Signal level limiting release time
                                           in ms. Default configuration is 50
                                           ms. Adjustable time must be larger
                                           than 0 ms. */

  AAC_METADATA_PROFILE =
      0x0020, /*!< See ::AAC_MD_PROFILE for all available values. */
  AAC_METADATA_EXPIRY_TIME = 0x0021, /*!< Defines the time in ms after which all
                                        the bitstream associated meta-data (DRC,
                                        downmix coefficients, ...) will be reset
                                        to default if no update has been
                                        received. Negative values disable the
                                        feature. */

  AAC_CONCEAL_METHOD = 0x0100, /*!< Error concealment: Processing method. \n
                                    0: Spectral muting. \n
                                    1: Noise substitution (see ::CONCEAL_NOISE).
                                  \n 2: Energy interpolation (adds additional
                                  signal delay of one frame, see
                                  ::CONCEAL_INTER. only some AOTs are
                                  supported). \n */

  AAC_DRC_BOOST_FACTOR =
      0x0200, /*!< MPEG-4 / MPEG-D Dynamic Range Control (DRC): Scaling factor
                 for boosting gain values. Defines how the boosting DRC factors
                 (conveyed in the bitstream) will be applied to the decoded
                 signal. The valid values range from 0 (don't apply boost
                 factors) to 127 (fully apply boost factors). Default value is 0
                 for MPEG-4 DRC and 127 for MPEG-D DRC. */
  AAC_DRC_ATTENUATION_FACTOR = 0x0201, /*!< MPEG-4 / MPEG-D DRC: Scaling factor
                                          for attenuating gain values. Same as
                                            ::AAC_DRC_BOOST_FACTOR but for
                                          attenuating DRC factors. */
  AAC_DRC_REFERENCE_LEVEL =
      0x0202, /*!< MPEG-4 / MPEG-D DRC: Target reference level / decoder target
                 loudness.\n Defines the level below full-scale (quantized in
                 steps of 0.25dB) to which the output audio signal will be
                 normalized to by the DRC module.\n The parameter controls
                 loudness normalization for both MPEG-4 DRC and MPEG-D DRC. The
                 valid values range from 40 (-10 dBFS) to 127 (-31.75 dBFS).\n
                   Example values:\n
                   124 (-31 dBFS) for audio/video receivers (AVR) or other
                 devices allowing audio playback with high dynamic range,\n 96
                 (-24 dBFS) for TV sets or equivalent devices (default),\n 64
                 (-16 dBFS) for mobile devices where the dynamic range of audio
                 playback is restricted.\n Any value smaller than 0 switches off
                 loudness normalization and MPEG-4 DRC. */
  AAC_DRC_HEAVY_COMPRESSION =
      0x0203, /*!< MPEG-4 DRC: En-/Disable DVB specific heavy compression (aka
                 RF mode). If set to 1, the decoder will apply the compression
                 values from the DVB specific ancillary data field. At the same
                 time the MPEG-4 Dynamic Range Control tool will be disabled. By
                   default, heavy compression is disabled. */
  AAC_DRC_DEFAULT_PRESENTATION_MODE =
      0x0204, /*!< MPEG-4 DRC: Default presentation mode (DRC parameter
                 handling). \n Defines the handling of the DRC parameters boost
                 factor, attenuation factor and heavy compression, if no
                 presentation mode is indicated in the bitstream.\n For options,
                 see ::AAC_DRC_DEFAULT_PRESENTATION_MODE_OPTIONS.\n Default:
                 ::AAC_DRC_PARAMETER_HANDLING_DISABLED */
  AAC_DRC_ENC_TARGET_LEVEL =
      0x0205, /*!< MPEG-4 DRC: Encoder target level for light (i.e. not heavy)
                 compression.\n If known, this declares the target reference
                 level that was assumed at the encoder for calculation of
                 limiting gains. The valid values range from 0 (full-scale) to
                 127 (31.75 dB below full-scale). This parameter is used only
                 with ::AAC_DRC_PARAMETER_HANDLING_ENABLED or
                 ::AAC_DRC_PARAMETER_HANDLING_AOSP and ignored otherwise.
                 Default: -1 (unknown).\n */
  AAC_UNIDRC_SET_EFFECT = 0x0206, /*!< MPEG-D DRC: Request a DRC effect type for
                                     selection of a DRC set.\n Supported indices
                                     are:\n -1: DRC off. Completely disables
                                     MPEG-D DRC.\n 0: None (default). Disables
                                     MPEG-D DRC, but automatically enables DRC
                                     if necessary to prevent clipping.\n 1: Late
                                     night\n 2: Noisy environment\n 3: Limited
                                     playback range\n 4: Low playback level\n 5:
                                     Dialog enhancement\n 6: General
                                     compression. Used for generally enabling
                                     MPEG-D DRC without particular request.\n */
  AAC_UNIDRC_ALBUM_MODE =
      0x0207, /*!<  MPEG-D DRC: Enable album mode. 0: Disabled (default), 1:
                 Enabled.\n Disabled album mode leads to application of gain
                 sequences for fading in and out, if provided in the
                 bitstream.\n Enabled album mode makes use of dedicated album
                 loudness information, if provided in the bitstream.\n */
  AAC_TPDEC_PARAM_IGNORE_BUFFERFULLNESS =
      0x0601, /*!< Applicable to LOAS/LATM or ADTS streams. It allows to ignore
                 the buffer fullness parameter that is part of the header
                 there.\n For streaming, the decoder makes use the buffer
                 fullness parameter to make sure that the input buffer never
                 runs empty. The decoder will only start decoding after it has
                 received enough bytes to ensure that the stream is never
                 interrupted due to a buffer underrun.\n For packet-based
                 scenarios, this mechanism is not necessary. Additionally, it
                 may be unwanted, since it may cause an additional startup
                 delay, i.e. the decoder may return
                   ::AAC_DEC_NOT_ENOUGH_BITS for the first frames. To disable
                 this behavior,
                   ::AAC_TPDEC_PARAM_IGNORE_BUFFERFULLNESS has to be set to 1.
                 Therefore, the decoder will start decoding immediately. */
  AAC_TPDEC_PARAM_CHECK_TWO_SYNCS =
      0x0605, /*!< Enhance synchronization robustness by requiring two valid
                 sync words in expected positions for bitstreams, reducing false
                 positives caused by accidental sync word matches. Turned off by
                 default. */

  AAC_TARGET_LAYOUT_CICP =
      0x0900 /*!< Target Layout index for audio output based on table 95 of
                ISO/IEC 23008-3. This parameter must be set before any
                aacDecoder_ConfigRaw or aacDecoder_DecodeFrame call.\n See \ref
                AAC_TARGET_LAYOUT_CICP_values for the possible values.
                  */

} AACDEC_PARAM;

typedef struct AAC_DECODER_INSTANCE
    *HANDLE_AACDECODER; /*!< Pointer to a AAC decoder instance. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Get one ancillary data element.
 *
 * \param self       AAC decoder handle.
 * \param index      Index of the ancillary data element to get.
 * \param buffer     Pointer to a buffer receiving the requested ancillary data
 * element.
 * \param buffer_len Length of the provided buffer receiving the requested
 * ancillary data element.
 * \param size       Pointer to a buffer receiving the length of the requested
 * ancillary data element in bytes.
 * \return           Error code.
 */
AAC_DECODER_ERROR aacDecoder_AncDataGet(HANDLE_AACDECODER self,
                                                   const uint32_t index,
                                                   uint8_t *buffer,
                                                   const uint32_t buffer_len,
                                                   uint32_t *size);

/**
 * \brief Set one single decoder parameter.
 *
 * \param self   AAC decoder handle.
 * \param param  Parameter to be set.
 * \param value  Parameter value.
 * \return       Error code.
 */
AAC_DECODER_ERROR aacDecoder_SetParam(const HANDLE_AACDECODER self,
                                                 const AACDEC_PARAM param,
                                                 const int32_t value);

/**
 * \brief               Open an AAC decoder instance.
 * \param transportFmt  The transport type to be used.
 * \param nrOfLayers    Number of transport layers.
 * \return              AAC decoder handle.
 */
HANDLE_AACDECODER aacDecoder_Open(TRANSPORT_TYPE transportFmt);

/**
 * \brief Explicitly configure the decoder by passing a raw AudioSpecificConfig
 * (ASC) or a StreamMuxConfig (SMC), contained in a binary buffer. This is
 * required for MPEG-4 and Raw Packets file format bitstreams as well as for
 * LATM bitstreams with no in-band SMC. If the transport format is LATM with or
 * without LOAS, configuration is assumed to be an SMC, for all other file
 * formats an ASC.
 *
 * \param self    AAC decoder handle.
 * \param conf    Pointer to an unsigned char buffer containing the binary
 * configuration buffer (either ASC or SMC).
 * \param length  Length of the configuration buffer in bytes.
 * \return        Error code.
 */
AAC_DECODER_ERROR aacDecoder_ConfigRaw(HANDLE_AACDECODER self,
                                                  uint8_t *conf,
                                                  const uint32_t length);

/**
 * \brief Fill AAC decoder's internal input buffer with bitstream data from the
 * external input buffer. The function only copies such data as long as the
 * decoder-internal input buffer is not full. So it grabs whatever it can from
 * pBuffer and returns information (bytesValid) so that at a subsequent call of
 * %aacDecoder_Fill(), the right position in pBuffer can be determined to grab
 * the next data.
 *
 * \param self        AAC decoder handle.
 * \param pBuffer     Pointer to external input buffer.
 * \param bufferSize  Size of external input buffer. This argument is required
 * because decoder-internally we need the information to calculate the offset to
 * pBuffer, where the next available data is, which is then
 * fed into the decoder-internal buffer (as much as
 * possible). Our example framework implementation fills the
 * buffer at pBuffer again, once it contains no available valid bytes anymore
 * (meaning bytesValid equal 0).
 * \param bytesValid  Number of bitstream bytes in the external bitstream buffer
 * that have not yet been copied into the decoder's internal bitstream buffer by
 * calling this function. The value is updated according to
 * the amount of newly copied bytes.
 * \return            Error code.
 */
AAC_DECODER_ERROR aacDecoder_Fill(HANDLE_AACDECODER self,
                                             uint8_t *pBuffer,
                                             const uint32_t bufferSize,
                                             uint32_t *bytesValid);

/**
 * \brief Signals an input bit stream data discontinuity.
 * Resyncs any internals as necessary. Clears all signal delay lines and history
 * buffers. This can cause discontinuities in the output signal.
 *
 * \param self        AAC decoder handle.
 * \return            Error code.
 */
AAC_DECODER_ERROR aacDecoder_Interrupt(HANDLE_AACDECODER self);

/**
 * \brief Clears internal bit stream buffer of transport layers.
 * The decoder starts decoding at new data passed after this event
 * and any previous bit stream data is discarded.
 *
 * \param self        AAC decoder handle.
 * \return            Error code.
 */
AAC_DECODER_ERROR aacDecoder_Clear(HANDLE_AACDECODER self);

/**
 * \brief               Decodes one audio frame
 *
 * \param self          AAC decoder handle.
 * \param pTimeData     Pointer to external output buffer where the decoded PCM
 * samples will be stored into.
 * \param timeDataSize  Size of external output buffer.
 * \param pOutputInfo   A pointer to an `OutputInfo` structure that will be
 * filled on return.
 * \param pStreamInfo   A pointer to an `StreamInfo` structure that will be
 * filled on return.
 * \param pMetadataInfo A pointer to an `MetadataInfo` structure that will be
 * filled on return.
 * \return              Error code.
 */
AAC_DECODER_ERROR aacDecoder_Decode(
    HANDLE_AACDECODER self, float *pTimeData, const uint32_t timeDataSize,
    OutputInfo *pOutputInfo, StreamInfo *pStreamInfo,
    MetadataInfo *pMetadataInfo);


/**
 * \brief               Flushes all filterbanks to get all delayed audio without
 * having new input data. New input data will not be considered.
 *
 * \param self          AAC decoder handle.
 * \param pTimeData     Pointer to external output buffer where the decoded PCM
 * samples will be stored into.
 * \param timeDataSize  Size of external output buffer.
 * \param pOutputInfo   A pointer to an `OutputInfo` structure that will be
 * filled on return.
 * \return              Error code.
 */
AAC_DECODER_ERROR
aacDecoder_Drain(HANDLE_AACDECODER self, float *pTimeData,
                 const uint32_t timeDataSize, OutputInfo *pOutputInfo);


/**
 * \brief               Triggers the built-in error concealment to generate
 * substitute signal for one lost frame. New input data will not be considered.
 *
 * \param self          AAC decoder handle.
 * \param pTimeData     Pointer to external output buffer where the decoded PCM
 * samples will be stored into.
 * \param timeDataSize  Size of external output buffer.
 * \param pOutputInfo   A pointer to an `OutputInfo` structure that will be
 * filled on return.
 * \return              Error code.
 */
AAC_DECODER_ERROR
aacDecoder_Conceal(HANDLE_AACDECODER self, float *pTimeData,
                   const uint32_t timeDataSize, OutputInfo *pOutputInfo);

/**
 * \brief       De-allocate all resources of an AAC decoder instance.
 *
 * \param self  AAC decoder handle.
 * \return      void.
 */
void aacDecoder_Close(HANDLE_AACDECODER self);

#ifdef __cplusplus
}
#endif

#endif /* AACDECODER_LIB_H */
