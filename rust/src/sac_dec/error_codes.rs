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
//! SAC decoder error codes

#[repr(C)]
#[derive(PartialEq, Eq, Debug, Default)]
pub enum SacDecoderError {
    #[default]
    Ok = 0,
    // Generic errors.
    NotOk = -1000,
    /// set_param() not successfull.
    InvalidParameter = -997,
    /// Various functions; unknown AOT or `n_channels` in filterbank.
    UnsupportedFormat = -996,
    /// Size of provided output time buffer is too small.
    OutputBufferTooSmall = -995,

    // SSC related errors.
    /// Unsupported `num_parameter_bands`.
    InvalidParameterbands = -994,
    // ec_data_dec()
    InvalidSetidx = -993,
    UnequalSsc = -992,
    UnsupportedConfig = -991,

    // Parse errors
    ParseError = -990,
    InvalidTempshape = -989,

    // Render errors
    WrongParametersets = -988,
    WrongQuantmode = -987,
}

impl From<i32> for SacDecoderError {
    fn from(value: i32) -> Self {
        match value {
            0 => SacDecoderError::Ok,
            // Generic errors.
            -1000 => SacDecoderError::NotOk,
            // set_param() not successfull.
            -997 => SacDecoderError::InvalidParameter,
            // Various functions; unknown AOT or `n_channels` in filterbank.
            -996 => SacDecoderError::UnsupportedFormat,
            // Size of provided output time buffer is too small.
            // -995 => SacDecoderError::OutputBufferTooSmall,
            // SSC related errors.
            // Unsupported `num_parameter_bands`.
            -994 => SacDecoderError::InvalidParameterbands,
            // ec_data_dec()
            -993 => SacDecoderError::InvalidSetidx,
            -992 => SacDecoderError::UnequalSsc,
            -991 => SacDecoderError::UnsupportedConfig,
            -990 => SacDecoderError::ParseError,
            -989 => SacDecoderError::InvalidTempshape,
            -988 => SacDecoderError::WrongParametersets,
            -987 => SacDecoderError::WrongQuantmode,
            _ => panic!("value {} not implemented for SacDecoderError", value),
        }
    }
}

impl From<SacDecoderError> for i32 {
    fn from(value: SacDecoderError) -> Self {
        match value {
            SacDecoderError::Ok => 0,
            // Generic errors.
            SacDecoderError::NotOk => -1000,
            // set_param() not successfull.
            SacDecoderError::InvalidParameter => -997,
            // Various functions; unknown AOT or `n_channels` in filterbank.
            SacDecoderError::UnsupportedFormat => -996,
            // Size of provided output time buffer is too small.
            SacDecoderError::OutputBufferTooSmall => -995,
            // SSC related errors.
            // Unsupported `num_parameter_bands`.
            SacDecoderError::InvalidParameterbands => -994,
            // ec_data_dec()
            SacDecoderError::InvalidSetidx => -993,
            SacDecoderError::UnequalSsc => -992,
            SacDecoderError::UnsupportedConfig => -991,
            SacDecoderError::ParseError => -990,
            SacDecoderError::InvalidTempshape => -989,
            SacDecoderError::WrongParametersets => -988,
            SacDecoderError::WrongQuantmode => -987,
        }
    }
}
