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
//! AAC decoder interface
use crate::td_limiter::limiter::TdLimiterError;

use crate::{pcm_dmx::PcmDmxError, tp_dec::TpDecoderError};

#[repr(C)]
#[derive(PartialEq, Clone, Copy, Debug, PartialOrd)]
pub enum AacDecoderError {
    /// No error occurred. Output buffer is valid and error free.
    Ok = 0x0000,
    /// No error occurred. Output samples are not available yet.
    IntermediateOk = 0x0001,
    /// Heap returned NULL pointer. Output buffer is invalid.
    OutOfMemory = 0x0002,
    // Error condition is of unknown reason, or from a another module. Output buffer is invalid.
    Unknown = 0x0005,

    /// Synchronization errors. Output buffer is invalid.
    ErrorStart = 0x1000,
    /// The transport decoder had synchronization problems. Do not exit decoding. Just feed new
    /// bitstream data.
    TransportSyncError = 0x1001,
    /// The input buffer ran out of bits.
    NotEnoughBits = 0x1002,
    SyncErrorEnd = 0x1FFF,

    /// Initialization errors. Output buffer is invalid.
    InitErrorStart = 0x2000,
    /// The handle passed to the function call was invalid (NULL).
    InvalidHandle = 0x2001,
    /// The AOT found in the configuration is not supported.
    UnsupportedAot = 0x2002,
    /// The bitstream format is not supported.
    UnsupportedFormat = 0x2003,
    /// The channel configuration (either number or arrangement) is not supported.
    UnsupportedChannelconfig = 0x2007,
    /// The sample rate specified in the configuration is not supported.
    UnsupportedSamplingrate = 0x2008,
    /// The SBR configuration is not supported.
    InvalidSbrConfig = 0x2009,
    /// The parameter could not be set. Either the value was out of range or the parameter does
    /// not exist.
    SetParamFail = 0x200A,
    /// The decoder needs to be restarted, since the required configuration change cannot be
    /// performed.
    NeedToRestart = 0x200B,
    /// The provided output buffer is too small.
    OutputBufferTooSmall = 0x200C,
    InitErrorEnd = 0x2FFF,

    /// Decode errors. Output buffer is valid but concealed.
    DecodeErrorStart = 0x4000,
    /// Error while parsing the bitstream. Most probably it is corrupted, or the system crashed.
    ParseError = 0x4002,
    /// The parsed bitstream value is out of range. Most probably the bitstream is corrupt, or the
    /// system crashed.
    DecodeFrameError = 0x4004,
    /// The embedded CRC did not match.
    CrcError = 0x4005,
    /// An invalid codebook was signaled. Most probably the bitstream is corrupt, or the system
    /// crashed.
    InvalidCodeBook = 0x4006,
    /// Predictor found, but not supported in the AAC Low Complexity profile. Most probably the
    /// bitstream is corrupt, or has a wrong format.
    UnsupportedPrediction = 0x4007,
    /// Gain control data found but not supported. Most probably the bitstream is corrupt, or has a
    /// wrong format.
    UnsupportedGainControlData = 0x400A,
    /// Error while reading TNS data. Most  probably the bitstream is corrupt or the  system
    /// crashed.
    TnsReadError = 0x400C,
    DecodeErrorEnd = 0x4FFF,

    /// Ancillary data errors. Output buffer is valid.
    AncDataErrorStart = 0x8000,
    /// The ancillary data could not be parsed completely.
    AncDataError = 0x8001,
    AncDataErrorEnd = 0x8FFF,
}

impl AacDecoderError {
    pub fn is_decode_error(&self) -> bool {
        *self >= AacDecoderError::DecodeErrorStart && *self <= AacDecoderError::DecodeErrorEnd
    }

    pub fn is_init_error(&self) -> bool {
        *self >= AacDecoderError::InitErrorStart && *self <= AacDecoderError::InitErrorEnd
    }

    pub fn is_output_valid(&self) -> bool {
        *self == AacDecoderError::Ok || self.is_decode_error()
    }
}

impl From<TdLimiterError> for AacDecoderError {
    fn from(error: TdLimiterError) -> Self {
        match error {
            TdLimiterError::Ok => AacDecoderError::Ok,
            TdLimiterError::Unknown => AacDecoderError::Unknown,
            TdLimiterError::InvalidHandle => AacDecoderError::InvalidHandle,
            TdLimiterError::InvalidParameter => AacDecoderError::SetParamFail,
        }
    }
}

pub trait Cluster {
    fn is_output_valid(&self) -> bool;
}

impl Cluster for Result<(), AacDecoderError> {
    fn is_output_valid(&self) -> bool {
        match self {
            Ok(()) => true,
            Err(error) => error.is_output_valid(),
        }
    }
}

/// Error mapping from `PcmDmxError` to `AacDecoderError`
impl From<PcmDmxError> for AacDecoderError {
    fn from(pcm_dmx_error: PcmDmxError) -> Self {
        match pcm_dmx_error {
            PcmDmxError::Ok => Self::Ok,
            PcmDmxError::OutputBufferTooSmall => Self::OutputBufferTooSmall,
            PcmDmxError::_OutOfMemory => Self::OutOfMemory,
            PcmDmxError::InvalidHandle => Self::InvalidHandle,
            PcmDmxError::InvalidArgument => Self::DecodeFrameError,
            PcmDmxError::InvalidChConfig => Self::DecodeFrameError,
            PcmDmxError::InvalidMode => Self::DecodeFrameError,
            PcmDmxError::_UnknownParam => Self::Unknown,
            PcmDmxError::UnableToSetParam => Self::SetParamFail,
            PcmDmxError::CorruptAncData => Self::AncDataError,
            PcmDmxError::Unsupported => Self::DecodeFrameError,
            PcmDmxError::_FatalErrorStart | PcmDmxError::_FatalErrorEnd => {
                panic!("Error invalid !")
            }
        }
    }
}

/// Error mapping from `TpDecoderError` to `AacDecoderError`.
impl From<TpDecoderError> for AacDecoderError {
    fn from(error: TpDecoderError) -> Self {
        match error {
            TpDecoderError::Ok => AacDecoderError::Ok,
            TpDecoderError::NotEnoughBits => AacDecoderError::NotEnoughBits,
            TpDecoderError::SyncError => AacDecoderError::TransportSyncError,
            TpDecoderError::UnknownError => AacDecoderError::Unknown,
            TpDecoderError::InvalidParameter => AacDecoderError::SetParamFail,
            TpDecoderError::NeedToRestart => AacDecoderError::NeedToRestart,
            TpDecoderError::TooManyBits => AacDecoderError::Unknown,
            TpDecoderError::ParseError => AacDecoderError::ParseError,
            TpDecoderError::UnsupportedFormat => AacDecoderError::UnsupportedFormat,
            TpDecoderError::CrcError => AacDecoderError::CrcError,
            TpDecoderError::FatalErrorStart
            | TpDecoderError::FatalErrorEnd
            | TpDecoderError::SyncErrorStart
            | TpDecoderError::SyncErrorEnd
            | TpDecoderError::DecodeErrorStart
            | TpDecoderError::DecodeErrorEnd => {
                panic!(
                    "This error case is only used as start/stop index.
                It cannot be converted into a meaningful error."
                )
            }
        }
    }
}
