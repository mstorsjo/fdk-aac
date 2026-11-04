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

#ifndef AACDEC_ERRORCODES_H
#define AACDEC_ERRORCODES_H

/**
 * \brief  AAC decoder error codes.
 */
typedef enum {
  AAC_DEC_OK =
      0x0000, /*!< No error occurred. Output buffer is valid and error free. */
  AAC_DEC_INTERMEDIATE_OK =
      0x0001, /*!< No error occurred. Output samples are not available yet. */
  AAC_DEC_OUT_OF_MEMORY =
      0x0002, /*!< Heap returned NULL pointer. Output buffer is invalid. */
  AAC_DEC_UNKNOWN =
      0x0005, /*!< Error condition is of unknown reason, or from a another
                 module. Output buffer is invalid. */

  /* Synchronization errors. Output buffer is invalid. */
  aac_dec_sync_error_start = 0x1000,
  AAC_DEC_TRANSPORT_SYNC_ERROR = 0x1001, /*!< The transport decoder had
                                            synchronization problems. Do not
                                            exit decoding. Just feed new
                                              bitstream data. */
  AAC_DEC_NOT_ENOUGH_BITS = 0x1002, /*!< The input buffer ran out of bits. */
  aac_dec_sync_error_end = 0x1FFF,

  /* Initialization errors. Output buffer is invalid. */
  aac_dec_init_error_start = 0x2000,
  AAC_DEC_INVALID_HANDLE =
      0x2001, /*!< The handle passed to the function call was invalid (NULL). */
  AAC_DEC_UNSUPPORTED_AOT =
      0x2002, /*!< The AOT found in the configuration is not supported. */
  AAC_DEC_UNSUPPORTED_FORMAT =
      0x2003, /*!< The bitstream format is not supported.  */
  AAC_DEC_UNSUPPORTED_CHANNELCONFIG =
      0x2007, /*!< The channel configuration (either number or arrangement) is
                 not supported. */
  AAC_DEC_UNSUPPORTED_SAMPLINGRATE = 0x2008, /*!< The sample rate specified in
                                                the configuration is not
                                                supported. */
  AAC_DEC_INVALID_SBR_CONFIG =
      0x2009, /*!< The SBR configuration is not supported. */
  AAC_DEC_SET_PARAM_FAIL = 0x200A,  /*!< The parameter could not be set. Either
                                       the value was out of range or the
                                       parameter does  not exist. */
  AAC_DEC_NEED_TO_RESTART = 0x200B, /*!< The decoder needs to be restarted,
                                       since the required configuration change
                                       cannot be performed. */
  AAC_DEC_OUTPUT_BUFFER_TOO_SMALL =
      0x200C, /*!< The provided output buffer is too small. */
  aac_dec_init_error_end = 0x2FFF,

  /* Decode errors. Output buffer is valid but concealed. */
  aac_dec_decode_error_start = 0x4000,
  AAC_DEC_PARSE_ERROR = 0x4002, /*!< Error while parsing the bitstream. Most
                                   probably it is corrupted, or the system
                                   crashed. */
  AAC_DEC_DECODE_FRAME_ERROR = 0x4004, /*!< The parsed bitstream value is out of
                                          range. Most probably the bitstream is
                                          corrupt, or the system crashed. */
  AAC_DEC_CRC_ERROR = 0x4005,          /*!< The embedded CRC did not match. */
  AAC_DEC_INVALID_CODE_BOOK = 0x4006,  /*!< An invalid codebook was signaled.
                                          Most probably the bitstream is corrupt,
                                          or the system  crashed. */
  AAC_DEC_UNSUPPORTED_PREDICTION =
      0x4007, /*!< Predictor found, but not supported in the AAC Low Complexity
                 profile. Most probably the bitstream is corrupt, or has a wrong
                 format. */
  AAC_DEC_UNSUPPORTED_GAIN_CONTROL_DATA =
      0x400A, /*!< Gain control data found but not supported. Most probably the
                 bitstream is corrupt, or has a wrong format. */
  AAC_DEC_TNS_READ_ERROR = 0x400C, /*!< Error while reading TNS data. Most
                                      probably the bitstream is corrupt or the
                                      system crashed. */
  aac_dec_decode_error_end = 0x4FFF,

  /* Ancillary data errors. Output buffer is valid. */
  aac_dec_anc_data_error_start = 0x8000,
  AAC_DEC_ANC_DATA_ERROR =
      0x8001, /*!< The ancillary data could not be parsed completely. */
  aac_dec_anc_data_error_end = 0x8FFF

} AAC_DECODER_ERROR;

/** Macro to identify initialization errors. Output buffer is invalid. */
#define IS_INIT_ERROR(err)                                                    \
  ((((err) >= aac_dec_init_error_start) && ((err) <= aac_dec_init_error_end)) \
       ? 1                                                                    \
       : 0)
/** Macro to identify decode errors. Output buffer is valid but concealed. */
#define IS_DECODE_ERROR(err)                 \
  ((((err) >= aac_dec_decode_error_start) && \
    ((err) <= aac_dec_decode_error_end))     \
       ? 1                                   \
       : 0)
/**
 * Macro to identify if the audio output buffer contains valid samples after
 * calling aacDecoder_DecodeFrame(). Output buffer is valid but can be
 * concealed.
 */
#define IS_OUTPUT_VALID(err) (((err) == AAC_DEC_OK) || IS_DECODE_ERROR(err))

#endif /* AACDEC_ERRORCODES_H */
