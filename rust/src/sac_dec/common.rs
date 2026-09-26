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
//! Enums and functions used commonly by SAC decoder's components

use super::error_codes::SacDecoderError;

/// 1 bit value read from bitstream. It indicates the framing type under which the parameter time
/// slots information is available.
#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq)]
pub(super) enum BsFramingType {
    /// Fixed framing (equidistant parameter time slots).
    Fixed = 0,
    /// Variable framing.
    Variable = 1,
}

impl TryFrom<u32> for BsFramingType {
    type Error = SacDecoderError;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(BsFramingType::Fixed),
            1 => Ok(BsFramingType::Variable),
            _ => Err(SacDecoderError::ParseError),
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq)]
pub(super) enum DataType {
    Cld,
    Icc,
    Ipd,
}

// Converts u8 to DataType.
impl From<u8> for DataType {
    fn from(value: u8) -> Self {
        match value {
            0 => DataType::Cld,
            1 => DataType::Icc,
            2 => DataType::Ipd,
            _ => panic!("invalid value: {value}"),
        }
    }
}

/// 2 bit info, which indicates whether and how new information for a parameter subset is encoded.
/// ISO/IEC 23003-1:2007, Table 68.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum BsXxxDataMode {
    /// Set to default paramter values.
    #[default]
    Dflt = 0,
    /// Keep previous parameter values unchanged.
    Keep = 1,
    /// Interpolate parameter values.
    Interpolate = 2,
    /// Read losslessly coded parameter values.
    LosslesslyCoded = 3,
    // An invalid arbitrary value (larger than 2 bits).
    Invalid = 4,
}

impl From<u8> for BsXxxDataMode {
    fn from(value: u8) -> Self {
        match value {
            0 => BsXxxDataMode::Dflt,
            1 => BsXxxDataMode::Keep,
            2 => BsXxxDataMode::Interpolate,
            3 => BsXxxDataMode::LosslesslyCoded,
            _ => BsXxxDataMode::Invalid,
        }
    }
}

impl From<BsXxxDataMode> for u8 {
    fn from(value: BsXxxDataMode) -> Self {
        match value {
            BsXxxDataMode::Dflt => 0,
            BsXxxDataMode::Keep => 1,
            BsXxxDataMode::Interpolate => 2,
            BsXxxDataMode::LosslesslyCoded => 3,
            BsXxxDataMode::Invalid => 4,
        }
    }
}

impl From<u32> for BsXxxDataMode {
    fn from(value: u32) -> Self {
        match value {
            0 => BsXxxDataMode::Dflt,
            1 => BsXxxDataMode::Keep,
            2 => BsXxxDataMode::Interpolate,
            3 => BsXxxDataMode::LosslesslyCoded,
            _ => BsXxxDataMode::Invalid,
        }
    }
}

impl From<BsXxxDataMode> for u32 {
    fn from(value: BsXxxDataMode) -> Self {
        match value {
            BsXxxDataMode::Dflt => 0,
            BsXxxDataMode::Keep => 1,
            BsXxxDataMode::Interpolate => 2,
            BsXxxDataMode::LosslesslyCoded => 3,
            BsXxxDataMode::Invalid => 4,
        }
    }
}

/// 1 bit bitstream information, which indicates if coarse quantization
/// is employed according to: ISO/IEC 23003-1:2007, Table 69.
#[repr(u8)]
#[derive(Debug, Default, Clone, Copy, Eq, PartialEq)]
pub(super) enum BsQuantCoarseXxx {
    #[default]
    /// Parameter values coded with full quantizer resolution (fine quantization).
    FullRes = 0,
    /// Parameter values coded with half quantizer resolution (coarse quantization).
    HalfRes,
}

impl From<BsQuantCoarseXxx> for u8 {
    fn from(value: BsQuantCoarseXxx) -> Self {
        match value {
            BsQuantCoarseXxx::FullRes => 0,
            BsQuantCoarseXxx::HalfRes => 1,
        }
    }
}

impl From<u8> for BsQuantCoarseXxx {
    fn from(value: u8) -> Self {
        match value {
            0 => BsQuantCoarseXxx::FullRes,
            1 => BsQuantCoarseXxx::HalfRes,
            _ => panic!("BsQuantCoarseXxx is 1 bit wide information read from bitstream."),
        }
    }
}

impl From<BsQuantCoarseXxx> for u32 {
    fn from(value: BsQuantCoarseXxx) -> Self {
        match value {
            BsQuantCoarseXxx::FullRes => 0,
            BsQuantCoarseXxx::HalfRes => 1,
        }
    }
}

impl From<u32> for BsQuantCoarseXxx {
    fn from(value: u32) -> Self {
        match value {
            0 => BsQuantCoarseXxx::FullRes,
            1 => BsQuantCoarseXxx::HalfRes,
            _ => panic!("BsQuantCoarseXxx is 1 bit wide information read from bitstream."),
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
/// Smoothing parameter modes, according to ISO/IEC 23003-1:2007, Table 63.
/// 2 bit wide information.
pub(super) enum BsSmoothMode {
    #[default]
    /// Set all smoothing flags to 0 (off).
    Off = 0,
    /// Keep previous smoothing parameters unchanged.
    KeepPrevUnchanged,
    /// Set all smoothing flags to 1 (on).
    On,
    /// Read coded smoothing flags.
    ReadCodedFlags,
}

impl From<u32> for BsSmoothMode {
    fn from(value: u32) -> Self {
        match value {
            0 => BsSmoothMode::Off,
            1 => BsSmoothMode::KeepPrevUnchanged,
            2 => BsSmoothMode::On,
            3 => BsSmoothMode::ReadCodedFlags,
            _ => panic!("BsSmoothMode is 2 bit wide information read from bitstream."),
        }
    }
}

impl From<BsSmoothMode> for u32 {
    fn from(value: BsSmoothMode) -> Self {
        match value {
            BsSmoothMode::Off => 0,
            BsSmoothMode::KeepPrevUnchanged => 1,
            BsSmoothMode::On => 2,
            BsSmoothMode::ReadCodedFlags => 3,
        }
    }
}

/// Smoothing mode for Overall Phase Difference values.
#[repr(u8)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum OpdSmoothingMode {
    #[default]
    Disabled = 0,
    Enabled = 1,
}

impl From<OpdSmoothingMode> for u8 {
    fn from(value: OpdSmoothingMode) -> Self {
        match value {
            OpdSmoothingMode::Disabled => 0,
            OpdSmoothingMode::Enabled => 1,
        }
    }
}

impl From<u8> for OpdSmoothingMode {
    fn from(value: u8) -> Self {
        match value {
            0 => OpdSmoothingMode::Disabled,
            1 => OpdSmoothingMode::Enabled,
            _ => panic!("OpdSmoothingMode is 1 bit wide data read from bitstream."),
        }
    }
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
/// Indicates whether IPD coding is applied in Mps212Config().
pub(super) enum PhaseCoding {
    #[default]
    /// No IPD data present.
    NoIpd = 0,
    /// IPD data present.
    Ipd = 1,
    /// Suggests prediction based signal upmix (USAC).
    IpdPrediction = 3,
}

impl From<PhaseCoding> for u8 {
    fn from(value: PhaseCoding) -> Self {
        match value {
            PhaseCoding::NoIpd => 0,
            PhaseCoding::Ipd => 1,
            PhaseCoding::IpdPrediction => 3,
        }
    }
}

impl From<u8> for PhaseCoding {
    fn from(value: u8) -> Self {
        match value {
            0 => PhaseCoding::NoIpd,
            1 => PhaseCoding::Ipd,
            3 => PhaseCoding::IpdPrediction,
            _ => panic!("{} is not a valid PhaseCoding enum. member.", value),
        }
    }
}

/// Defines the number of parameter bands.
// CAUTION: Do not change enum values!
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum FreqResolution {
    #[default]
    Res28 = 28,
    Res23 = 23,
    Res20 = 20,
    Res15 = 15,
    Res14 = 14,
    Res12 = 12,
    Res10 = 10,
    Res9 = 9,
    Res7 = 7,
    Res5 = 5,
    Res4 = 4,
    ResInvalid = 0,
}

impl From<FreqResolution> for u8 {
    fn from(value: FreqResolution) -> Self {
        match value {
            FreqResolution::Res28 => 28,
            FreqResolution::Res23 => 23,
            FreqResolution::Res20 => 20,
            FreqResolution::Res15 => 15,
            FreqResolution::Res14 => 14,
            FreqResolution::Res12 => 12,
            FreqResolution::Res10 => 10,
            FreqResolution::Res9 => 9,
            FreqResolution::Res7 => 7,
            FreqResolution::Res5 => 5,
            FreqResolution::Res4 => 4,
            FreqResolution::ResInvalid => 0,
        }
    }
}

impl TryFrom<u8> for FreqResolution {
    type Error = SacDecoderError;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            28 => Ok(FreqResolution::Res28),
            23 => Ok(FreqResolution::Res23),
            20 => Ok(FreqResolution::Res20),
            15 => Ok(FreqResolution::Res15),
            14 => Ok(FreqResolution::Res14),
            12 => Ok(FreqResolution::Res12),
            10 => Ok(FreqResolution::Res10),
            9 => Ok(FreqResolution::Res9),
            7 => Ok(FreqResolution::Res7),
            5 => Ok(FreqResolution::Res5),
            4 => Ok(FreqResolution::Res4),
            _ => Err(SacDecoderError::ParseError),
        }
    }
}

impl TryFrom<usize> for FreqResolution {
    type Error = SacDecoderError;
    fn try_from(value: usize) -> Result<Self, Self::Error> {
        match value {
            28 => Ok(FreqResolution::Res28),
            23 => Ok(FreqResolution::Res23),
            20 => Ok(FreqResolution::Res20),
            15 => Ok(FreqResolution::Res15),
            14 => Ok(FreqResolution::Res14),
            12 => Ok(FreqResolution::Res12),
            10 => Ok(FreqResolution::Res10),
            9 => Ok(FreqResolution::Res9),
            7 => Ok(FreqResolution::Res7),
            5 => Ok(FreqResolution::Res5),
            4 => Ok(FreqResolution::Res4),
            _ => Err(SacDecoderError::ParseError),
        }
    }
}

impl From<FreqResolution> for usize {
    fn from(value: FreqResolution) -> Self {
        match value {
            FreqResolution::Res28 => 28,
            FreqResolution::Res23 => 23,
            FreqResolution::Res20 => 20,
            FreqResolution::Res15 => 15,
            FreqResolution::Res14 => 14,
            FreqResolution::Res12 => 12,
            FreqResolution::Res10 => 10,
            FreqResolution::Res9 => 9,
            FreqResolution::Res7 => 7,
            FreqResolution::Res5 => 5,
            FreqResolution::Res4 => 4,
            FreqResolution::ResInvalid => 0,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub(super) enum QuantMode {
    #[default]
    FineDef = 0,
    Edq1 = 1,
    Edq2 = 2,
}

impl TryFrom<u32> for QuantMode {
    type Error = SacDecoderError;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(QuantMode::FineDef),
            1 => Ok(QuantMode::Edq1),
            2 => Ok(QuantMode::Edq2),
            // QuantMode > 2 is reserved.
            _ => Err(SacDecoderError::ParseError),
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub(super) enum FixedGains {
    #[default]
    Mode0 = 0,
    Rsvd1 = 1,
    Rsvd2 = 2,
    Rsvd3 = 3,
    Rsvd4 = 4,
    Rsvd5 = 5,
    Rsvd6 = 6,
    Rsvd7 = 7,
}

impl From<u32> for FixedGains {
    fn from(value: u32) -> Self {
        match value {
            0 => FixedGains::Mode0,
            1 => FixedGains::Rsvd1,
            2 => FixedGains::Rsvd2,
            3 => FixedGains::Rsvd3,
            4 => FixedGains::Rsvd4,
            5 => FixedGains::Rsvd5,
            6 => FixedGains::Rsvd6,
            7 => FixedGains::Rsvd7,
            _ => panic!("There is no FixedGains {} enum member", value),
        }
    }
}

impl From<FixedGains> for usize {
    fn from(value: FixedGains) -> Self {
        match value {
            FixedGains::Mode0 => 0,
            FixedGains::Rsvd1 => 1,
            FixedGains::Rsvd2 => 2,
            FixedGains::Rsvd3 => 3,
            FixedGains::Rsvd4 => 4,
            FixedGains::Rsvd5 => 5,
            FixedGains::Rsvd6 => 6,
            FixedGains::Rsvd7 => 7,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Clone, Copy, PartialEq)]
pub(super) enum TsConf {
    #[default]
    TpNoWhite = 0,
    TpWhite = 1,
    Tes = 2,
    Nots = 3,
}

impl From<u32> for TsConf {
    fn from(value: u32) -> Self {
        match value {
            0 => TsConf::TpNoWhite,
            1 => TsConf::TpWhite,
            2 => TsConf::Tes,
            3 => TsConf::Nots,
            _ => panic!("There is no TsConf {} enum member", value),
        }
    }
}

impl From<TsConf> for usize {
    fn from(value: TsConf) -> Self {
        match value {
            TsConf::TpNoWhite => 0,
            TsConf::TpWhite => 1,
            TsConf::Tes => 2,
            TsConf::Nots => 3,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Clone, Copy, PartialEq)]
pub(super) enum DecorrConf {
    #[default]
    Mode0 = 0,
    Mode1 = 1,
    Mode2 = 2,
}

impl From<DecorrConf> for usize {
    fn from(value: DecorrConf) -> Self {
        match value {
            DecorrConf::Mode0 => 0,
            DecorrConf::Mode1 => 1,
            DecorrConf::Mode2 => 2,
        }
    }
}

impl TryFrom<u32> for DecorrConf {
    type Error = SacDecoderError;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(DecorrConf::Mode0),
            1 => Ok(DecorrConf::Mode1),
            2 => Ok(DecorrConf::Mode2),
            // Reserved value.
            _ => Err(SacDecoderError::ParseError),
        }
    }
}

/// MPEG Surround input data interface mode.
#[repr(C)]
#[derive(Debug, PartialEq)]
pub enum SacInputConfig {
    /// Use QMF domain interface for the input downmix audio.
    Qmf,
    /// Use time domain interface for the input downmix audio.
    Time,
}

/// MPEG Surround decoder dynamic parameters.
#[repr(C)]
#[derive(Debug)]
pub enum SacDecParam {
    /// Input signal interface for MPEG surround.
    Interface = 0x0004,
    /// Bitstream interruption.
    BsInterruption = 0x0020,
}

#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub(super) struct Syntax {
    pub is_usac: bool,
    pub is_eld: bool,
}
