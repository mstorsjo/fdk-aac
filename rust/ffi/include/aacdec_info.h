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

#ifndef AACDEC_INFO_H
#define AACDEC_INFO_H

#include <stdint.h> /* Needed for fixed width types */
#include "aac_api_types.h"

typedef struct {
  uint32_t sampling_rate; /*!< The sample rate in Hz of the decoded PCM audio
                         signal. */
  uint16_t frame_size;  /*!< The frame size of the decoded PCM audio signal. */
  uint8_t num_channels; /*!< The number of decoder output audio channel. */
  uint32_t output_delay;  /*!< The number of samples the output is additionally
                         delayed by the decoder. */

  AUDIO_CHANNEL_TYPE
  channel_type[8]; /*!< Audio channel type of each output audio channel. */
  uint8_t channel_indices[8]; /*!< Audio channel index for each output audio
                               channel.See ISO/IEC 13818-7:2005(E), 8.5.3.2
                               Explicit channel mapping using a
                               program_config_element() */

  int16_t
    output_loudness; /*!< Audio output loudness in steps of -0.25 dB. Range: 0
                         (0 dBFS) to 231 (-57.75 dBFS).\n A value of -1
                         indicates that no loudness metadata is present.\n If
                         loudness normalization is active, the value corresponds
                         to the target loudness value set with
                         ::AAC_DRC_REFERENCE_LEVEL.\n If loudness normalization
                         is not active, the output loudness value corresponds to
                         the loudness metadata given in the bitstream.\n
                         Loudness metadata can originate from MPEG-4 DRC or
                         MPEG-D DRC. */

} OutputInfo;

/**
 * \brief This structure gives information about the currently decoded audio
 * data. All fields are read-only.
 */
typedef struct {
  uint8_t numElements;           /*!< The number of bistream elements. */
  MP4_ELEMENT_ID elements[10]; /*!< Bitstream element list. */
  CHANNEL_ORDER
  pcmChOrder; /*!< Audio channel order (MPEG, WAV, CICP) provided at the
                 decoder output. In case of MPEG-H, the CICP order is used
                 always. */

  /* Decoder internal members. */
  uint32_t aacSampleRate; /*!< Sampling rate in Hz without SBR (from configuration
                         info) divided by a (ELD) downscale factor if present.
                       */
  AUDIO_OBJECT_TYPE
  aot; /*!< Audio Object Type (from ASC): is set to the appropriate value
          for MPEG-2 bitstreams (e. g. 2 for AAC-LC). */
  int32_t channelConfig; /*!< Channel configuration (0: PCE defined, 1: mono, 2:
                        stereo, ...                       */
  uint16_t aacSamplesPerFrame; /*!< Samples per frame for the AAC core (from ASC)
                                divided by a (ELD) downscale factor if present.
                                \n Typically this is (with a downscale factor of
                                1): \n 1024 or 960 for AAC-LC \n 512 or 480 for
                                AAC-LD and AAC-ELD         */
  uint8_t aacNumChannels;      /*!< The number of audio channels after AAC core
                                processing (before PS or MPS processing).      CAUTION:
                                This are not the final number of output channels! */
  AUDIO_OBJECT_TYPE extAot;  /*!< Extension Audio Object Type (from ASC)   */
  uint32_t extSamplingRate; /*!< Extension sampling rate in Hz (from ASC) divided by
                           a (ELD) downscale factor if present. */
  uint32_t flags; /*!< Copy of internal flags. Only to be written by the decoder,
                 and only to be read externally. */

  /* Statistics */
  uint32_t num_consumed_bytes; /*!< This is the number of bytes that have passed
                              through the decoder within the present function
                              call. */

} StreamInfo;

typedef struct {
  int8_t drc_presentation_mode; /*!< DRC presentation mode. According to ETSI TS
                                  101 154, this field indicates whether light
                                  (MPEG-4 Dynamic Range Control tool) or heavy
                                  compression (DVB heavy compression) dynamic
                                  range control shall take priority on the
                                  outputs. For details, see ETSI TS 101 154,
                                  table C.33. Possible values are: \n -1: No
                                  corresponding metadata found in the bitstream
                                  \n 0: DRC presentation mode not indicated \n
                                   1: DRC presentation mode 1 \n
                                   2: DRC presentation mode 2 \n
                                   3: Reserved */
  int8_t drc_program_reference_level; /*!< DRC program reference level. Defines
                                  the reference level below full-scale. It is
                                  quantized in steps of 0.25dB. The valid values
                                  range from 0 (0 dBFS) to 127 (-31.75 dBFS). It
                                  is used to reflect the average loudness of the
                                  audio in LKFS according to ITU-R BS 1770. If
                                  no level has been found in the bitstream the
                                  value is -1. */
  int8_t
  pce_matrix_mixdown_index; /*!< The 2 bit matrix mixdown index extracted
                               from PCE. */
  int8_t pce_pseudeo_surrouncd_enable; /*!< Pseudo Surround flag extracted from
                                         PCE */
} MetadataInfo;

#ifdef __cplusplus
extern "C" {
#endif

OutputInfo CAacDecoderOutputInfo_default();
StreamInfo CAacDecoderStreamInfo_default();
MetadataInfo CAacDecoderMetadataInfo_default();

#ifdef __cplusplus
}
#endif

#endif /* AACDEC_INFO_H */
