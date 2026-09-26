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
//!  Loudness measurement system enums.

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
/// Types of measurement system for the loudness measurement
/// according to ISO/IEC 23003-4:2020, Table A.50.
pub(super) enum MeasurementSystem {
    #[default]
    UnknownOther = 0,
    EbuR128 = 1,
    Bs1770_4 = 2,
    Bs1770_4PreProcessing = 3,
    User = 4,
    ExpertPanel = 5,
    Bs1771_1 = 6,
    RmsA = 7,
    RmsB = 8,
    RmsC = 9,
    RmsD = 10,
    RmsE = 11,
    Reserved12 = 12,
    Reserved13 = 13,
    Reserved14 = 14,
    Reserved15 = 15,
}

/// `TryFrom<u8>` trait for `MeasurementSystem`.
impl TryFrom<u8> for MeasurementSystem {
    type Error = &'static str;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MeasurementSystem::UnknownOther),
            1 => Ok(MeasurementSystem::EbuR128),
            2 => Ok(MeasurementSystem::Bs1770_4),
            3 => Ok(MeasurementSystem::Bs1770_4PreProcessing),
            4 => Ok(MeasurementSystem::User),
            5 => Ok(MeasurementSystem::ExpertPanel),
            6 => Ok(MeasurementSystem::Bs1771_1),
            7 => Ok(MeasurementSystem::RmsA),
            8 => Ok(MeasurementSystem::RmsB),
            9 => Ok(MeasurementSystem::RmsC),
            10 => Ok(MeasurementSystem::RmsD),
            11 => Ok(MeasurementSystem::RmsE),
            12 => Ok(MeasurementSystem::Reserved12),
            13 => Ok(MeasurementSystem::Reserved13),
            14 => Ok(MeasurementSystem::Reserved14),
            15 => Ok(MeasurementSystem::Reserved15),
            _ => Err("Invalid value"),
        }
    }
}

impl From<MeasurementSystem> for usize {
    fn from(value: MeasurementSystem) -> usize {
        match value {
            MeasurementSystem::UnknownOther => 0,
            MeasurementSystem::EbuR128 => 1,
            MeasurementSystem::Bs1770_4 => 2,
            MeasurementSystem::Bs1770_4PreProcessing => 3,
            MeasurementSystem::User => 4,
            MeasurementSystem::ExpertPanel => 5,
            MeasurementSystem::Bs1771_1 => 6,
            MeasurementSystem::RmsA => 7,
            MeasurementSystem::RmsB => 8,
            MeasurementSystem::RmsC => 9,
            MeasurementSystem::RmsD => 10,
            MeasurementSystem::RmsE => 11,
            MeasurementSystem::Reserved12 => 12,
            MeasurementSystem::Reserved13 => 13,
            MeasurementSystem::Reserved14 => 14,
            MeasurementSystem::Reserved15 => 15,
        }
    }
}

impl From<MeasurementSystem> for i32 {
    fn from(value: MeasurementSystem) -> i32 {
        match value {
            MeasurementSystem::UnknownOther => 0,
            MeasurementSystem::EbuR128 => 1,
            MeasurementSystem::Bs1770_4 => 2,
            MeasurementSystem::Bs1770_4PreProcessing => 3,
            MeasurementSystem::User => 4,
            MeasurementSystem::ExpertPanel => 5,
            MeasurementSystem::Bs1771_1 => 6,
            MeasurementSystem::RmsA => 7,
            MeasurementSystem::RmsB => 8,
            MeasurementSystem::RmsC => 9,
            MeasurementSystem::RmsD => 10,
            MeasurementSystem::RmsE => 11,
            MeasurementSystem::Reserved12 => 12,
            MeasurementSystem::Reserved13 => 13,
            MeasurementSystem::Reserved14 => 14,
            MeasurementSystem::Reserved15 => 15,
        }
    }
}

impl From<i32> for MeasurementSystem {
    fn from(value: i32) -> MeasurementSystem {
        match value {
            0 => MeasurementSystem::UnknownOther,
            1 => MeasurementSystem::EbuR128,
            2 => MeasurementSystem::Bs1770_4,
            3 => MeasurementSystem::Bs1770_4PreProcessing,
            4 => MeasurementSystem::User,
            5 => MeasurementSystem::ExpertPanel,
            6 => MeasurementSystem::Bs1771_1,
            7 => MeasurementSystem::RmsA,
            8 => MeasurementSystem::RmsB,
            9 => MeasurementSystem::RmsC,
            10 => MeasurementSystem::RmsD,
            11 => MeasurementSystem::RmsE,
            12 => MeasurementSystem::Reserved12,
            13 => MeasurementSystem::Reserved13,
            14 => MeasurementSystem::Reserved14,
            15 => MeasurementSystem::Reserved15,
            _ => panic!("Invalid value."),
        }
    }
}

// ------------------------------------------------------------------------- //

/// Loudness measurement system indices for the loudness normalization settings
/// according to ISO/IEC 23003-4:2020, Table 48.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
pub(in super::super) enum MeasurementSystemRequest {
    _Default = 0,
    /// ITU-R BS.1770-4. Default value according to the ISO/IEC 23003-4:2020, Table A.102.
    _Bs1770_4 = 1,
    _User = 2,
    // Default value changed to "ExpertPanel". Value is intended to take precedence over
    // "ITU-R BS.1770" measurement system.
    #[default]
    ExpertPanel = 3,
    _ReservedA = 4,
    _ReservedB = 5,
    _ReservedC = 6,
    _ReservedD = 7,
    _ReservedE = 8,
}

impl From<MeasurementSystemRequest> for usize {
    fn from(value: MeasurementSystemRequest) -> usize {
        match value {
            MeasurementSystemRequest::_Default => 0,
            MeasurementSystemRequest::_Bs1770_4 => 1,
            MeasurementSystemRequest::_User => 2,
            MeasurementSystemRequest::ExpertPanel => 3,
            MeasurementSystemRequest::_ReservedA => 4,
            MeasurementSystemRequest::_ReservedB => 5,
            MeasurementSystemRequest::_ReservedC => 6,
            MeasurementSystemRequest::_ReservedD => 7,
            MeasurementSystemRequest::_ReservedE => 8,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
pub(super) enum MeasurementSystemRequestBonus {
    Default = 0,
    Bs1770_4 = 1,
    User = 2,
    #[default]
    ExpertPanel = 3,
    ReservedA = 4,
    ReservedB = 5,
    ReservedC = 6,
    ReservedD = 7,
    ReservedE = 8,
    ProgramLoudness = 9,
    PeakLoudness = 10,
}

impl From<MeasurementSystemRequestBonus> for usize {
    fn from(value: MeasurementSystemRequestBonus) -> usize {
        match value {
            MeasurementSystemRequestBonus::Default => 0,
            MeasurementSystemRequestBonus::Bs1770_4 => 1,
            MeasurementSystemRequestBonus::User => 2,
            MeasurementSystemRequestBonus::ExpertPanel => 3,
            MeasurementSystemRequestBonus::ReservedA => 4,
            MeasurementSystemRequestBonus::ReservedB => 5,
            MeasurementSystemRequestBonus::ReservedC => 6,
            MeasurementSystemRequestBonus::ReservedD => 7,
            MeasurementSystemRequestBonus::ReservedE => 8,
            MeasurementSystemRequestBonus::ProgramLoudness => 9,
            MeasurementSystemRequestBonus::PeakLoudness => 10,
        }
    }
}

impl From<i32> for MeasurementSystemRequestBonus {
    fn from(value: i32) -> MeasurementSystemRequestBonus {
        match value {
            0 => MeasurementSystemRequestBonus::Default,
            1 => MeasurementSystemRequestBonus::Bs1770_4,
            2 => MeasurementSystemRequestBonus::User,
            3 => MeasurementSystemRequestBonus::ExpertPanel,
            4 => MeasurementSystemRequestBonus::ReservedA,
            5 => MeasurementSystemRequestBonus::ReservedB,
            6 => MeasurementSystemRequestBonus::ReservedC,
            7 => MeasurementSystemRequestBonus::ReservedD,
            8 => MeasurementSystemRequestBonus::ReservedE,
            9 => MeasurementSystemRequestBonus::ProgramLoudness,
            10 => MeasurementSystemRequestBonus::PeakLoudness,
            _ => panic!("Invalid value."),
        }
    }
}

impl From<MeasurementSystemRequest> for MeasurementSystemRequestBonus {
    fn from(value: MeasurementSystemRequest) -> MeasurementSystemRequestBonus {
        match value {
            MeasurementSystemRequest::_Default => MeasurementSystemRequestBonus::Default,
            MeasurementSystemRequest::_Bs1770_4 => MeasurementSystemRequestBonus::Bs1770_4,
            MeasurementSystemRequest::_User => MeasurementSystemRequestBonus::User,
            MeasurementSystemRequest::ExpertPanel => MeasurementSystemRequestBonus::ExpertPanel,
            MeasurementSystemRequest::_ReservedA => MeasurementSystemRequestBonus::ReservedA,
            MeasurementSystemRequest::_ReservedB => MeasurementSystemRequestBonus::ReservedB,
            MeasurementSystemRequest::_ReservedC => MeasurementSystemRequestBonus::ReservedC,
            MeasurementSystemRequest::_ReservedD => MeasurementSystemRequestBonus::ReservedD,
            MeasurementSystemRequest::_ReservedE => MeasurementSystemRequestBonus::ReservedE,
        }
    }
}
