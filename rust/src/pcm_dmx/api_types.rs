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
//! PCM downmix public types used by api calls.

use super::PCM_DMX_MAX_DELAY_FRAMES;
use super::{DualChannelMode, ProfileType};

/// Error codes that can be returned by module interface functions.
#[derive(Debug, PartialEq, Eq)]
#[repr(C)]
pub enum PcmDmxError {
    /// No error.
    Ok = 0x0,
    /// The requested feature/service is unavailable. This can occur if the module was built for a
    /// wrong configuration
    Unsupported = 0x1,
    _FatalErrorStart = 0x2,
    _OutOfMemory = 0x3,
    _FatalErrorEnd = 0x4,
    /// The handle is invalid.
    InvalidHandle = 0x5,
    /// One of the parameters is invalid.
    InvalidArgument = 0x6,
    /// The given channel configuration is not supported and thus no processing was performed.
    InvalidChConfig = 0x7,
    /// The set configuration/mode is not applicable.
    InvalidMode = 0x8,
    /// The given parameter is unknown.
    _UnknownParam = 0x9,
    /// Unable to set the specific parameter. Most probably the value ist out of range.
    UnableToSetParam = 0xA,
    /// The ancillary data is corrupt.
    CorruptAncData = 0xB,
    /// The output buffer is too small.
    OutputBufferTooSmall = 0xC,
}

/// Module`s dynamic runtime parameters
#[derive(Debug)]
#[repr(C)]
pub enum PcmDmxParam {
    /// Defines which equations, coefficients and default/fallback values used for downmixing.
    DmxProfileSetting(ProfileType),
    /// The number of frames without new metadata that have to go by before the bitstream data
    /// expires. The value 0 disables expiry.
    DmxBsDataExpiryFrame(u32),
    /// The number of delay frames of the output samples compared to the bitstream data.
    DmxBsDataDelay(u8),
    /// The minimum number of output channels. For all
    /// input configurations that have less than the given
    /// channels the module will modify the output
    /// automatically to obtain the given number of output
    /// channels. Mono signals will be duplicated. If more
    /// than two output channels are desired the module
    /// just adds empty channels. The parameter value must
    /// be either -1, 0, 1, 2, 6 or 8. If the value is
    /// greater than zero and exceeds the value of
    /// parameter [PcmDmxParam::MaxNumberOfOutputChannels]
    /// latter will be set to the same value. Both values
    /// -1 and 0 disable the feature.
    MinNumberOfOutputChannels(u8),
    /// The maximum number of output channels. For all
    /// input configurations that have more than the given
    /// channels the module will apply a mixdown
    /// automatically to obtain the given number of output
    /// channels. The value must be either -1, 0, 1, 2, 6
    /// or 8. If it's greater than zero and lower or equal
    /// than the value of [PcmDmxParam::MinNumberOfOutputChannels]
    /// parameter the latter will be set to the same value.
    /// The values -1 and 0 disable the feature.
    MaxNumberOfOutputChannels(u8),
    /// Downmix mode for two channel audio data. See type
    /// DualChannelMode enum for details.
    DmxDualChannelMode(DualChannelMode),
}

impl TryFrom<i32> for PcmDmxParam {
    type Error = PcmDmxError;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0x01 => Ok(PcmDmxParam::DmxProfileSetting(value.try_into().unwrap())),
            0x10 => Ok(PcmDmxParam::DmxBsDataExpiryFrame(value.try_into().unwrap())),
            0x11 => Ok(PcmDmxParam::DmxBsDataDelay(value.try_into().unwrap())),
            0x20 => {
                if let Ok(num_ch_min) = u8::try_from(value) {
                    Ok(PcmDmxParam::MinNumberOfOutputChannels(num_ch_min))
                } else {
                    Err(PcmDmxError::InvalidChConfig)
                }
            }
            0x21 => {
                if let Ok(num_ch_max) = u8::try_from(value) {
                    Ok(PcmDmxParam::MinNumberOfOutputChannels(num_ch_max))
                } else {
                    Err(PcmDmxError::InvalidChConfig)
                }
            }
            0x30 => {
                if let Ok(dual_ch_mode) = DualChannelMode::try_from(value) {
                    Ok(PcmDmxParam::DmxDualChannelMode(dual_ch_mode))
                } else {
                    Err(PcmDmxError::InvalidChConfig)
                }
            }
            _ => Err(PcmDmxError::InvalidArgument),
        }
    }
}

/// Dynamic (user) params.
/// See [PcmDmxParam] for the list of available parameters.
#[derive(Debug, Clone)]
#[repr(C)]
pub struct PcmDmxParams {
    pub(super) dmx_profile: ProfileType,
    pub(super) expiry_frame: u32,
    pub(super) dual_channel_mode: DualChannelMode,
    pub(super) num_out_channels_min: u8,
    pub(super) num_out_channels_max: u8,
    pub(super) frame_delay: u8,
    pub has_changed: bool,
}

impl Default for PcmDmxParams {
    fn default() -> Self {
        PcmDmxParams {
            dmx_profile: ProfileType::Standard,
            expiry_frame: 0,
            dual_channel_mode: DualChannelMode::Stereo,
            num_out_channels_min: 0,
            num_out_channels_max: 0,
            frame_delay: 0,
            has_changed: true,
        }
    }
}

impl PcmDmxParams {
    /// Creates new `PcmDmxParams` instance.
    pub fn new() -> Self {
        PcmDmxParams::default()
    }

    /// Initializes the pre-allocated `PcmDmxParams` to default values.
    pub fn init(&mut self) {
        *self = Default::default();
    }

    pub fn set(&mut self, param: PcmDmxParam) -> Result<(), PcmDmxError> {
        match param {
            PcmDmxParam::DmxProfileSetting(value) => self.dmx_profile = value,
            PcmDmxParam::DmxBsDataExpiryFrame(value) => {
                self.expiry_frame = value;
            }
            PcmDmxParam::DmxBsDataDelay(value) => {
                if usize::from(value) > PCM_DMX_MAX_DELAY_FRAMES {
                    return Err(PcmDmxError::UnableToSetParam);
                }
                self.frame_delay = value;
            }
            PcmDmxParam::MinNumberOfOutputChannels(value) => match value {
                0 | 1 | 2 | 6 | 8 => self.num_out_channels_min = value,
                _ => return Err(PcmDmxError::UnableToSetParam),
            },
            PcmDmxParam::MaxNumberOfOutputChannels(value) => match value {
                0 | 1 | 2 | 6 | 8 => self.num_out_channels_max = value,
                _ => return Err(PcmDmxError::UnableToSetParam),
            },
            PcmDmxParam::DmxDualChannelMode(value) => self.dual_channel_mode = value,
        }

        self.has_changed = true;

        Ok(())
    }
}

/// Bitstream metadata structure.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct DmxBsMetaData {
    pub(super) type_flags: u32,
    /// From DSE
    pub(super) c_lev_idx: u8,
    pub(super) s_lev_idx: u8,
    pub(super) dmix_idx_a: u8,
    pub(super) dmix_idx_b: u8,
    pub(super) dmix_idx_lfe: u8,
    pub(super) dmx_gain_idx2: u8,
    pub(super) dmx_gain_idx5: u8,
    /// From PCE
    pub(super) matrix_mixdown_idx: u8,
    /// Counter to monitor the life time of a meta data set.
    pub(super) expiry_count: u32,
}

impl Default for DmxBsMetaData {
    fn default() -> Self {
        DmxBsMetaData {
            type_flags: 0,
            c_lev_idx: 2,
            s_lev_idx: 2,
            dmix_idx_a: 2,
            dmix_idx_b: 2,
            dmix_idx_lfe: 15,
            dmx_gain_idx2: 0,
            dmx_gain_idx5: 0,
            matrix_mixdown_idx: 0,
            expiry_count: 0,
        }
    }
}

#[cfg(test)]
impl DmxBsMetaData {
    pub fn set_dmx_idx_lfe(&mut self, idx: u8) {
        self.dmix_idx_lfe = idx;
    }
}
