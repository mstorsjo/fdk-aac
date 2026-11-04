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
//! Loudness measurement method definition.
// See ISO/IEC 23003-4 for details.

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
/// Method definition indices for the loudness normalization,
/// according to ISO/IEC 23003-4:2020, Table A.49.
pub(super) enum MethodDefinition {
    #[default]
    UnknownOther = 0,
    /// ProgramLoudness as defined in ISO/IEC 23091-3.
    ProgramLoudness = 1,
    /// AnchorLoudness as defined in ISO/IEC 23091-3.
    AnchorLoudness = 2,
    /// Maximum of the range (95th percentile of the loudness distribution according to EBU R-128).
    MaxOfLoudnessRange = 3,
    /// Maximum momentary loudness, measured using a 0.4s window according to ITU-R BS.1771–1 or
    /// EBU R-128.
    MomentaryLoudnessMax = 4,
    /// Maximum short-term loudness, measured using a 3s window according to ITU-R BS.1771–1 or
    /// EBU R-128.
    ShortTermLoudnessMax = 5,
    /// Loudness range derived from EBU R-128.
    LoudnessRange = 6,
    /// Production mixing level.
    MixingLevel = 7,
    /// Production room type.
    RoomType = 8,
    /// short-term loudness, measured using a 3s window according to ITU- R BS.1771–1 or EBU R-128.
    /// The 3s window shall include the current DRC frame.
    ShortTermLoudness = 9,
}

/// `TryFrom<u8>` trait for `MethodDefinition`.
impl TryFrom<u8> for MethodDefinition {
    type Error = &'static str;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MethodDefinition::UnknownOther),
            1 => Ok(MethodDefinition::ProgramLoudness),
            2 => Ok(MethodDefinition::AnchorLoudness),
            3 => Ok(MethodDefinition::MaxOfLoudnessRange),
            4 => Ok(MethodDefinition::MomentaryLoudnessMax),
            5 => Ok(MethodDefinition::ShortTermLoudnessMax),
            6 => Ok(MethodDefinition::LoudnessRange),
            7 => Ok(MethodDefinition::MixingLevel),
            8 => Ok(MethodDefinition::RoomType),
            9 => Ok(MethodDefinition::ShortTermLoudness),
            _ => Err("Invalid value"),
        }
    }
}

/// `From<i32>` trait for `MethodDefinition`.
impl From<i32> for MethodDefinition {
    fn from(value: i32) -> MethodDefinition {
        match value {
            0 => MethodDefinition::UnknownOther,
            1 => MethodDefinition::ProgramLoudness,
            2 => MethodDefinition::AnchorLoudness,
            3 => MethodDefinition::MaxOfLoudnessRange,
            4 => MethodDefinition::MomentaryLoudnessMax,
            5 => MethodDefinition::ShortTermLoudnessMax,
            6 => MethodDefinition::LoudnessRange,
            7 => MethodDefinition::MixingLevel,
            8 => MethodDefinition::RoomType,
            9 => MethodDefinition::ShortTermLoudness,
            _ => panic!("Invalid value"),
        }
    }
}

// ------------------------------------------------------------------------- //
/// Permitted loudness measurement method definition indices for the loudness normalization
/// settings, according to ISO/IEC 23003-4:2020, Table 47.
#[repr(C)]
#[derive(Default, Debug, PartialEq, PartialOrd, Copy, Clone)]
pub enum MethodDefinitionRequest {
    Default = 0,
    // Default value according to the standard (ISO/IEC 23003-4:2020, Table A.102).
    ProgramLoudness = 1,
    // Default value changed to `AnchorLoudness`. Value is intended to take precedence over
    // `ProgramLoudness` measurement method.
    #[default]
    AnchorLoudness = 2,
}

impl From<u8> for MethodDefinitionRequest {
    fn from(val: u8) -> MethodDefinitionRequest {
        match val {
            0 => MethodDefinitionRequest::Default,
            1 => MethodDefinitionRequest::ProgramLoudness,
            2 => MethodDefinitionRequest::AnchorLoudness,
            _ => panic!("Invalid MethodDefinitionRequest variant"),
        }
    }
}

impl From<MethodDefinitionRequest> for u8 {
    fn from(val: MethodDefinitionRequest) -> u8 {
        match val {
            MethodDefinitionRequest::Default => 0,
            MethodDefinitionRequest::ProgramLoudness => 1,
            MethodDefinitionRequest::AnchorLoudness => 2,
        }
    }
}

impl From<i32> for MethodDefinitionRequest {
    fn from(val: i32) -> MethodDefinitionRequest {
        match val {
            0 => MethodDefinitionRequest::Default,
            1 => MethodDefinitionRequest::ProgramLoudness,
            2 => MethodDefinitionRequest::AnchorLoudness,
            _ => panic!("Invalid MethodDefinitionRequest variant"),
        }
    }
}

impl From<MethodDefinitionRequest> for MethodDefinition {
    fn from(value: MethodDefinitionRequest) -> MethodDefinition {
        match value {
            MethodDefinitionRequest::Default => MethodDefinition::UnknownOther,
            MethodDefinitionRequest::ProgramLoudness => MethodDefinition::ProgramLoudness,
            MethodDefinitionRequest::AnchorLoudness => MethodDefinition::AnchorLoudness,
        }
    }
}
// ------------------------------------------------------------------------- //
