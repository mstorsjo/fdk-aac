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
//! PCM Downmix types

use crate::pcm_dmx::PcmDmxError;

/// Fixed and unique channel group indices.
#[derive(Debug)]
pub(super) enum ChGroup {
    Front = 0,
    Side = 1,
    Rear = 2,
    Lfe = 3,
}

impl From<usize> for ChGroup {
    fn from(value: usize) -> Self {
        match value {
            x if x == Self::Front as usize => Self::Front,
            x if x == Self::Side as usize => Self::Side,
            x if x == Self::Rear as usize => Self::Rear,
            x if x == Self::Lfe as usize => Self::Lfe,
            _ => panic!("Invalid ChGroup value: {}", value),
        }
    }
}

/// Fixed and unique channel plain indices.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) enum ChPlain {
    Normal = 0,
    Top = 1,
    Bottom = 2,
}

impl From<i32> for ChPlain {
    fn from(value: i32) -> Self {
        match value {
            x if x == Self::Normal as i32 => Self::Normal,
            x if x == Self::Top as i32 => Self::Top,
            x if x == Self::Bottom as i32 => Self::Bottom,
            _ => panic!("Invalid ChPlain value: {}", value),
        }
    }
}

/// The ordering of the following fixed channel labels has to be in MPEG-4 style.
/// From the center to the back with left and right channel interleaved (starting
/// with left).
#[derive(Debug)]
pub(super) enum Channel {
    CenterFront = 0,
    LeftFront = 1,
    RightFront = 2,
    LeftRear = 3,
    RightRear = 4,
    LowFrequency = 5,
    LeftMultiprps = 6,
    RightMultiprps = 7,
    LeftSide = 8,
    RightSide = 9,
    CenterRear = 10,
    CenterFrontTop = 11,
    LeftFrontTop = 12,
    RightFrontTop = 13,
    LeftSideTop = 14,
    RightSideTop = 15,
    CenterSideTop = 16,
    LeftRearTop = 17,
    RightRearTop = 18,
    CenterRearTop = 19,
    CenterFrontBottom = 20,
    LeftFrontBottom = 21,
    RightFrontBottom = 22,
    LowFrequency2 = 23,
}

impl From<usize> for Channel {
    fn from(value: usize) -> Self {
        match value {
            x if x == Self::CenterFront as usize => Self::CenterFront,
            x if x == Self::LeftFront as usize => Self::LeftFront,
            x if x == Self::RightFront as usize => Self::RightFront,
            x if x == Self::LeftRear as usize => Self::LeftRear,
            x if x == Self::RightRear as usize => Self::RightRear,
            x if x == Self::LowFrequency as usize => Self::LowFrequency,
            x if x == Self::LeftMultiprps as usize => Self::LeftMultiprps,
            x if x == Self::RightMultiprps as usize => Self::RightMultiprps,
            x if x == Self::LeftSide as usize => Self::LeftSide,
            x if x == Self::RightSide as usize => Self::RightSide,
            x if x == Self::CenterRear as usize => Self::CenterRear,
            x if x == Self::CenterFrontTop as usize => Self::CenterFrontTop,
            x if x == Self::LeftFrontTop as usize => Self::LeftFrontTop,
            x if x == Self::RightFrontTop as usize => Self::RightFrontTop,
            x if x == Self::LeftSideTop as usize => Self::LeftSideTop,
            x if x == Self::RightSideTop as usize => Self::RightSideTop,
            x if x == Self::CenterSideTop as usize => Self::CenterSideTop,
            x if x == Self::LeftRearTop as usize => Self::LeftRearTop,
            x if x == Self::RightRearTop as usize => Self::RightRearTop,
            x if x == Self::CenterRearTop as usize => Self::CenterRearTop,
            x if x == Self::CenterFrontBottom as usize => Self::CenterFrontBottom,
            x if x == Self::LeftFrontBottom as usize => Self::LeftFrontBottom,
            x if x == Self::RightFrontBottom as usize => Self::RightFrontBottom,
            x if x == Self::LowFrequency2 as usize => Self::LowFrequency2,
            _ => panic!("Invalid Channel value: {}", value),
        }
    }
}

#[derive(Debug)]
pub(super) enum ChannelCount {
    One = 1,
    Two = 2,
    Six = 6,
    Eight = 8,
}

impl From<usize> for ChannelCount {
    fn from(value: usize) -> Self {
        match value {
            x if x == Self::One as usize => Self::One,
            x if x == Self::Two as usize => Self::Two,
            x if x == Self::Six as usize => Self::Six,
            x if x == Self::Eight as usize => Self::Eight,
            _ => panic!("Invalid ChannelCount value: {}", value),
        }
    }
}

#[derive(Debug)]
pub(super) enum ThresholdMap {
    Heat1 = 0,
    Heat2 = 20,
    Heat3 = 256,
}

#[derive(Debug)]
pub(super) enum SpZ {
    Nrm = 0,
    Top = 2,
    Bot = -2,
    Lfe = -18,
    Mul = 8,
}

#[derive(Debug)]
pub(super) enum BsDataType {
    None = 0x00,
    PceData = 0x01,
    DseClevData = 0x02,
    DseSlevData = 0x04,
    DseDmixAbData = 0x08,
    DseDmixLfeData = 0x10,
    DseDmxGainData = 0x20,
    DseDmxCglData = 0x40,
    DseData = 0x7E,
}

impl From<u8> for BsDataType {
    fn from(value: u8) -> Self {
        match value {
            x if x == Self::None as u8 => Self::None,
            x if x == Self::PceData as u8 => Self::PceData,
            x if x == Self::DseClevData as u8 => Self::DseClevData,
            x if x == Self::DseSlevData as u8 => Self::DseSlevData,
            x if x == Self::DseDmixAbData as u8 => Self::DseDmixAbData,
            x if x == Self::DseDmixLfeData as u8 => Self::DseDmixLfeData,
            x if x == Self::DseDmxGainData as u8 => Self::DseDmxGainData,
            x if x == Self::DseDmxCglData as u8 => Self::DseDmxCglData,
            x if x == Self::DseData as u8 => Self::DseData,
            _ => panic!("Invalid PcmDmxDataType value: {}", value),
        }
    }
}

#[derive(Default, Debug, Clone, Copy)]
#[repr(C)]
pub(super) struct SpeakerPosition {
    /// horizontal position:  center (0), left (-), right (+)
    pub(super) x: i8,
    /// depth position:      front, side, back, position
    pub(super) y: i8,
    /// height positions:     normal, top, bottom, lfe
    pub(super) z: i8,
}

/// List of packed channel modes
/// ChMode<numFrontCh>_<numSideCh>_<numBackCh>_<numLfCh>
#[expect(non_camel_case_types)]
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) enum ChannelMode {
    Undefined = 0x0000,
    // 1 channel
    // ch_cfg: 1
    Mode_1_0_0_0 = 0x0001,
    // 2 channels
    // ch_cfg: 2
    Mode_2_0_0_0 = 0x0002,
    // 3 channels
    // ch_cfg: 3
    Mode_3_0_0_0 = 0x0003,
    Mode_2_0_1_0 = 0x0102,
    Mode_2_0_0_1 = 0x1002,
    // 4 channels
    // ch_cfg: 4
    Mode_3_0_1_0 = 0x0103,
    Mode_2_0_2_0 = 0x0202,
    Mode_2_0_1_1 = 0x1102,
    Mode_4_0_0_0 = 0x0004,
    // 5 channels
    // ch_cfg: 5
    Mode_3_0_2_0 = 0x0203,
    Mode_2_0_2_1 = 0x1202,
    Mode_3_0_1_1 = 0x1103,
    Mode_3_2_0_0 = 0x0023,
    Mode_5_0_0_0 = 0x0005,
    // 6 channels
    // ch_cfg: 6
    Mode_3_0_2_1 = 0x1203,
    Mode_3_2_0_1 = 0x1023,
    Mode_3_2_1_0 = 0x0123,
    Mode_5_0_1_0 = 0x0105,
    Mode_6_0_0_0 = 0x0006,
    // 7 channels
    // ch_cfg: 11
    Mode_2_2_2_1 = 0x1222,
    Mode_3_0_3_1 = 0x1303,
    Mode_3_2_1_1 = 0x1123,
    Mode_3_2_2_0 = 0x0223,
    Mode_3_0_2_2 = 0x2203,
    Mode_5_0_2_0 = 0x0205,
    Mode_5_0_1_1 = 0x1105,
    Mode_7_0_0_0 = 0x0007,
    // 8 channels
    Mode_3_2_2_1 = 0x1223,
    // ch_cfg: 12
    Mode_3_0_4_1 = 0x1403,
    // ch_cfg: 7 + 14
    Mode_5_0_2_1 = 0x1205,
    Mode_5_2_1_0 = 0x0125,
    Mode_3_2_1_2 = 0x2123,
    Mode_2_2_2_2 = 0x2222,
    Mode_3_0_3_2 = 0x2303,
    Mode_8_0_0_0 = 0x0008,
}

impl From<i32> for ChannelMode {
    fn from(value: i32) -> Self {
        match value {
            x if x == ChannelMode::Undefined as i32 => ChannelMode::Undefined,
            x if x == ChannelMode::Mode_1_0_0_0 as i32 => ChannelMode::Mode_1_0_0_0,
            x if x == ChannelMode::Mode_2_0_0_0 as i32 => ChannelMode::Mode_2_0_0_0,
            x if x == ChannelMode::Mode_3_0_0_0 as i32 => ChannelMode::Mode_3_0_0_0,
            x if x == ChannelMode::Mode_2_0_1_0 as i32 => ChannelMode::Mode_2_0_1_0,
            x if x == ChannelMode::Mode_2_0_0_1 as i32 => ChannelMode::Mode_2_0_0_1,
            x if x == ChannelMode::Mode_3_0_1_0 as i32 => ChannelMode::Mode_3_0_1_0,
            x if x == ChannelMode::Mode_2_0_2_0 as i32 => ChannelMode::Mode_2_0_2_0,
            x if x == ChannelMode::Mode_2_0_1_1 as i32 => ChannelMode::Mode_2_0_1_1,
            x if x == ChannelMode::Mode_4_0_0_0 as i32 => ChannelMode::Mode_4_0_0_0,
            x if x == ChannelMode::Mode_3_0_2_0 as i32 => ChannelMode::Mode_3_0_2_0,
            x if x == ChannelMode::Mode_2_0_2_1 as i32 => ChannelMode::Mode_2_0_2_1,
            x if x == ChannelMode::Mode_3_0_1_1 as i32 => ChannelMode::Mode_3_0_1_1,
            x if x == ChannelMode::Mode_3_2_0_0 as i32 => ChannelMode::Mode_3_2_0_0,
            x if x == ChannelMode::Mode_5_0_0_0 as i32 => ChannelMode::Mode_5_0_0_0,
            x if x == ChannelMode::Mode_3_0_2_1 as i32 => ChannelMode::Mode_3_0_2_1,
            x if x == ChannelMode::Mode_3_2_0_1 as i32 => ChannelMode::Mode_3_2_0_1,
            x if x == ChannelMode::Mode_3_2_1_0 as i32 => ChannelMode::Mode_3_2_1_0,
            x if x == ChannelMode::Mode_5_0_1_0 as i32 => ChannelMode::Mode_5_0_1_0,
            x if x == ChannelMode::Mode_6_0_0_0 as i32 => ChannelMode::Mode_6_0_0_0,
            x if x == ChannelMode::Mode_2_2_2_1 as i32 => ChannelMode::Mode_2_2_2_1,
            x if x == ChannelMode::Mode_3_0_3_1 as i32 => ChannelMode::Mode_3_0_3_1,
            x if x == ChannelMode::Mode_3_2_1_1 as i32 => ChannelMode::Mode_3_2_1_1,
            x if x == ChannelMode::Mode_3_2_2_0 as i32 => ChannelMode::Mode_3_2_2_0,
            x if x == ChannelMode::Mode_3_0_2_2 as i32 => ChannelMode::Mode_3_0_2_2,
            x if x == ChannelMode::Mode_5_0_2_0 as i32 => ChannelMode::Mode_5_0_2_0,
            x if x == ChannelMode::Mode_5_0_1_1 as i32 => ChannelMode::Mode_5_0_1_1,
            x if x == ChannelMode::Mode_7_0_0_0 as i32 => ChannelMode::Mode_7_0_0_0,
            x if x == ChannelMode::Mode_3_2_2_1 as i32 => ChannelMode::Mode_3_2_2_1,
            x if x == ChannelMode::Mode_3_0_4_1 as i32 => ChannelMode::Mode_3_0_4_1,
            x if x == ChannelMode::Mode_5_0_2_1 as i32 => ChannelMode::Mode_5_0_2_1,
            x if x == ChannelMode::Mode_5_2_1_0 as i32 => ChannelMode::Mode_5_2_1_0,
            x if x == ChannelMode::Mode_3_2_1_2 as i32 => ChannelMode::Mode_3_2_1_2,
            x if x == ChannelMode::Mode_2_2_2_2 as i32 => ChannelMode::Mode_2_2_2_2,
            x if x == ChannelMode::Mode_3_0_3_2 as i32 => ChannelMode::Mode_3_0_3_2,
            x if x == ChannelMode::Mode_8_0_0_0 as i32 => ChannelMode::Mode_8_0_0_0,
            _ => panic!("Invalid PCM_DMX_CHANNEL_MODE value: {}", value),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) enum MetadataProfile {
    MpegAmd4 = 1,
    MpegLegacy = 2,
    AribJapan = 4,
    ItuRecom = 8,
    Custom = 16,
    SearchStartVal = -7,
}

impl From<i32> for MetadataProfile {
    fn from(value: i32) -> Self {
        match value {
            x if x == MetadataProfile::MpegAmd4 as i32 => MetadataProfile::MpegAmd4,
            x if x == MetadataProfile::MpegLegacy as i32 => MetadataProfile::MpegLegacy,
            x if x == MetadataProfile::AribJapan as i32 => MetadataProfile::AribJapan,
            x if x == MetadataProfile::ItuRecom as i32 => MetadataProfile::ItuRecom,
            x if x == MetadataProfile::Custom as i32 => MetadataProfile::Custom,
            x if x == MetadataProfile::SearchStartVal as i32 => MetadataProfile::SearchStartVal,
            _ => panic!("Invalid DmxMethod value: {}", value),
        }
    }
}

/// Profile type
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum ProfileType {
    /// The standard profile creates mixdown signals based on the
    /// advanced downmix metadata (from a DSE), equations
    /// and default values defined in ISO/IEC 14496:3 Ammendment 4.
    /// Any other (legacy) downmix metadata will be ignored.
    Standard = 0x0,
    /// This profile behaves just as the standard profile if
    /// advanced downmix metadata (from a DSE) is available. If
    /// not, the matrix_mixdown information embedded in the
    /// program configuration element (PCE) will be applied. If
    /// neither is the case the module creates a mixdown using
    /// the default coefficients defined in MPEG-4 Ammendment 4.
    /// The profile can be used e.g. to support legacy digital
    /// TV (e.g. DVB) streams.
    MatrixMix = 0x1,
    /// Similar to the ::DMX_PRFL_MATRIX_MIX profile but if both
    /// the advanced (DSE) and the legacy (PCE) MPEG downmix
    /// metadata are available the latter will be applied.
    ForceMatrixMix = 0x2,
    /// Downmix creation as described in ABNT NBR 15602-2. But
    /// if advanced downmix metadata is available it will be
    /// prefered.
    AribJapan = 0x3,
}

/// Converts ProfileType to i32.
impl From<ProfileType> for i32 {
    fn from(value: ProfileType) -> Self {
        match value {
            ProfileType::Standard => 0,
            ProfileType::MatrixMix => 1,
            ProfileType::ForceMatrixMix => 2,
            ProfileType::AribJapan => 3,
        }
    }
}

impl TryFrom<i32> for ProfileType {
    type Error = PcmDmxError;
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(ProfileType::Standard),
            1 => Ok(ProfileType::MatrixMix),
            2 => Ok(ProfileType::ForceMatrixMix),
            3 => Ok(ProfileType::AribJapan),
            _ => Err(PcmDmxError::UnableToSetParam),
        }
    }
}

/// Dual channel mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum DualChannelMode {
    /// Leave stereo signals as they are.
    Stereo = 0x0,
    /// Create a dual channel output signal from channel 1.
    Ch1 = 0x1,
    /// Create a dual channel output signal from channel 2.
    Ch2 = 0x2,
    /// Create a dual channel output signal by mixing the two
    /// channels.
    Mixed = 0x3,
}

/// Converts DualChannelMode to i32.
impl From<DualChannelMode> for i32 {
    fn from(value: DualChannelMode) -> Self {
        match value {
            DualChannelMode::Stereo => 0,
            DualChannelMode::Ch1 => 1,
            DualChannelMode::Ch2 => 2,
            DualChannelMode::Mixed => 3,
        }
    }
}

/// Converts i32 to DualChannelMode.
impl TryFrom<i32> for DualChannelMode {
    type Error = &'static str;
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(DualChannelMode::Stereo),
            1 => Ok(DualChannelMode::Ch1),
            2 => Ok(DualChannelMode::Ch2),
            3 => Ok(DualChannelMode::Mixed),
            _ => Err("Invalid value"),
        }
    }
}
