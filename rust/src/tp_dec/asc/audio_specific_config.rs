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
//! Audio specific config (ASC)

use super::super::{callbacks::TpDecCallBacks, error_codes::TpDecoderError, pce::ProgramConfig};
use super::{
    eld_specific_config::EldSpecificConfig,
    ga_specific_config::GaSpecificConfig,
    helper_functions::{get_aot, get_sample_rate},
    UsacConfig,
};
use crate::common::aot::AudioObjectType;
use crate::common::bitstream::Bitstream;
use crate::common::flags::ACFlags;

/// Extension ID for different configurations.
#[derive(PartialEq, Copy, Clone, Debug)]
enum ExtensionId {
    Unknown = -1,
    Sbr = 0x2b7,
    Ps = 0x548,
    Mps = 0x76a,
    Saoc = 0x7cb,
    Ldmps = 0x7cc,
}

impl From<u32> for ExtensionId {
    fn from(value: u32) -> Self {
        match value {
            0x2b7 => ExtensionId::Sbr,
            0x548 => ExtensionId::Ps,
            0x76a => ExtensionId::Mps,
            0x7cb => ExtensionId::Saoc,
            0x7cc => ExtensionId::Ldmps,
            _ => ExtensionId::Unknown,
        }
    }
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
/// Audio specific configuration struct, suitable for encoder and decoder configuration.
pub struct AudioSpecificConfig {
    /// Program configuration.
    progr_config_element: ProgramConfig,
    /// Audio Object Type.
    aot: AudioObjectType,
    /// Channel configuration index.
    channel_configuration: u8,
    /// Sample rate.
    sampling_frequency: u32,
    /// Sample rate index.
    sampling_frequency_index: u8,
    /// Extension audio object type.
    extension_audio_object_type: AudioObjectType,
    /// Sample rate.
    extension_sampling_frequency: u32,
    /// General audio specific configuration.
    ga_specific_config: GaSpecificConfig,
    /// ELD specific configuration.
    eld_specific_config: EldSpecificConfig,
    /// USAC specific configuration.
    usac_config: UsacConfig,
    /// Flags indicating if:
    /// - Callback shall work in memory allocation mode or in config change detection mode.
    /// - At least one aac config parameter has changed.
    /// - At least one sbr config parameter has changed.
    /// - At least one sac config parameter has changed.
    ac_flags: ACFlags,
}

/// API functions
impl AudioSpecificConfig {
    /// Creates a new Audio Specific Config (`ASC`) instance.
    pub fn new() -> Self {
        AudioSpecificConfig::default()
    }

    /// Initializes the Audio Specific Config (ASC).
    pub fn init(&mut self) {
        self.reset();
        self.aot = AudioObjectType::AotNone;
        self.sampling_frequency_index = 0xf;
        self.extension_audio_object_type = AudioObjectType::AotNullObject;
        self.ac_flags = ACFlags::empty();
        self.eld_specific_config.set_downscale_factor(1);
        self.progr_config_element.init();
    }

    /// Clears the Audio Specific Config (ASC) values.
    pub fn reset(&mut self) {
        self.channel_configuration = 0;
        self.sampling_frequency = 0;
        self.extension_sampling_frequency = 0;

        self.progr_config_element.reset();
        self.ga_specific_config.reset();
        self.eld_specific_config.reset();
        self.usac_config.reset();
    }

    /// Parse the Audio Specific Config (ASC).
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `is_explicit_backward_compatible`: Parse extensions if is `true`.
    /// - `cb`: Transport decoder callbacks.
    /// - `aot`: Audio object type.
    pub fn parse(
        &mut self,
        bs: &mut Bitstream,
        is_explicit_backward_compatible: bool,
        cb: &mut TpDecCallBacks,
        aot: AudioObjectType,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        let asc_start_anchor = bs.valid_bits();

        self.init();

        if aot != AudioObjectType::AotNullObject {
            self.aot = aot;
            if aot == AudioObjectType::AotUsac {
                self.ac_flags.insert(ACFlags::USAC);
            }
        } else {
            self.aot = get_aot(bs);

            match self.aot {
                AudioObjectType::AotAacLc => (),
                AudioObjectType::AotSbr => self.ac_flags.insert(ACFlags::SBR_PRESENT),
                AudioObjectType::AotErAacLc => self.ac_flags.insert(ACFlags::ER),
                AudioObjectType::AotErAacScal => {
                    self.ac_flags.insert(ACFlags::ER | ACFlags::SCALABLE);
                }
                AudioObjectType::AotErAacLd => {
                    self.ac_flags.insert(ACFlags::ER | ACFlags::LD);
                }
                AudioObjectType::AotPs => {
                    self.ac_flags
                        .insert(ACFlags::PS_PRESENT | ACFlags::SBR_PRESENT);
                }
                AudioObjectType::AotErAacEld => {
                    self.ac_flags.insert(ACFlags::ER | ACFlags::ELD);
                }
                AudioObjectType::AotUsac => self.ac_flags.insert(ACFlags::USAC),
                _ => error_status = Err(TpDecoderError::UnsupportedFormat),
            };

            error_status?;

            self.sampling_frequency =
                get_sample_rate(bs, Some(&mut self.sampling_frequency_index), 4);
            if self.sampling_frequency == 0 {
                return Err(TpDecoderError::ParseError);
            }

            self.channel_configuration = bs.read(4) as u8;

            if self.ac_flags.contains(ACFlags::ER) {
                if self.channel_configuration == 0 {
                    return Err(TpDecoderError::UnsupportedFormat);
                }
                if self.ac_flags.contains(ACFlags::SCALABLE) && self.channel_configuration > 2 {
                    return Err(TpDecoderError::UnsupportedFormat);
                }
            }

            if !self.ac_flags.intersects(ACFlags::ELD | ACFlags::USAC) {
                if self
                    .ac_flags
                    .intersects(ACFlags::SBR_PRESENT | ACFlags::PS_PRESENT)
                {
                    self.extension_sampling_frequency = get_sample_rate(bs, None, 4);
                    if self.extension_sampling_frequency == 0 {
                        return Err(TpDecoderError::ParseError);
                    }
                    self.aot = get_aot(bs);
                    if self.aot != AudioObjectType::AotAacLc {
                        return Err(TpDecoderError::UnsupportedFormat);
                    }
                }

                error_status = self.ga_specific_config.parse(
                    &mut self.progr_config_element,
                    self.channel_configuration,
                    &mut self.ac_flags,
                    bs,
                    asc_start_anchor,
                );

                error_status?;
            }

            if self.ac_flags.contains(ACFlags::ELD) {
                error_status = self.eld_specific_config.parse(
                    self.sampling_frequency,
                    self.channel_configuration,
                    &mut self.ac_flags,
                    bs,
                    cb,
                );
                error_status?;
                self.extension_sampling_frequency =
                    self.sampling_frequency << self.eld_specific_config.sbr_sampling_rate();
            }

            if self.ac_flags.contains(ACFlags::ER) && bs.read(2) > 0 {
                // epConfig > 0 not supported
                return Err(TpDecoderError::UnsupportedFormat);
            }

            if !self.ac_flags.intersects(ACFlags::ER | ACFlags::USAC) {
                if is_explicit_backward_compatible {
                    error_status = self.extension_parse(bs);
                    error_status?;
                }
                if self.ac_flags.contains(ACFlags::SBR_PRESENT)
                    && !self.ac_flags.contains(ACFlags::FRAME_LENGTH)
                {
                    self.ac_flags.insert(ACFlags::MPEG4_ESBR);
                }
            }
        }
        if self.ac_flags.contains(ACFlags::USAC) {
            error_status = self.usac_config.parse(&mut self.ac_flags, bs, cb);
            error_status?;
        }

        if bs.valid_bits() < 0 {
            return Err(TpDecoderError::NotEnoughBits);
        }

        error_status
    }

    /// Parse Audio Specific Config (ASC) extension.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    pub fn extension_parse(&mut self, bs: &mut Bitstream) -> Result<(), TpDecoderError> {
        let mut last_asc_ext: ExtensionId = ExtensionId::Unknown;

        while bs.valid_bits() >= 11 {
            let asc_ex_id: u32 = bs.read(11);

            match ExtensionId::from(asc_ex_id) {
                // 0x2b7
                ExtensionId::Sbr => {
                    if self.extension_audio_object_type != AudioObjectType::AotSbr {
                        self.extension_audio_object_type = get_aot(bs);

                        // Get SBR extension configuration.
                        if self.extension_audio_object_type == AudioObjectType::AotSbr {
                            self.ac_flags.insert(if bs.read_bit() != 0 {
                                ACFlags::SBR_PRESENT
                            } else {
                                ACFlags::empty()
                            });

                            if self.ac_flags.contains(ACFlags::SBR_PRESENT) {
                                self.extension_sampling_frequency = get_sample_rate(bs, None, 4);
                                if self.extension_sampling_frequency == 0 {
                                    return Err(TpDecoderError::ParseError);
                                }
                            }
                        }
                    }
                }
                // 0x548
                ExtensionId::Ps => {
                    // Get PS extension configuration.
                    if last_asc_ext == ExtensionId::Sbr
                        && self.extension_audio_object_type == AudioObjectType::AotSbr
                    {
                        self.ac_flags.insert(if bs.read_bit() != 0 {
                            ACFlags::PS_PRESENT
                        } else {
                            ACFlags::empty()
                        })
                    }
                }
                _ => {
                    // Ignore everything.
                    bs.push(-11);
                    return Ok(());
                }
            };
            last_asc_ext = ExtensionId::from(asc_ex_id);
        }
        Ok(())
    }

    /// Returns audio object type.
    pub fn aot(&self) -> AudioObjectType {
        self.aot
    }

    /// Sets audio object type.
    pub fn set_aot(&mut self, aot: AudioObjectType) {
        self.aot = aot;
    }

    /// Returns extension audio object type.
    pub fn extension_aot(&self) -> AudioObjectType {
        self.extension_audio_object_type
    }

    /// Returns channel configuration.
    pub fn channel_config(&self) -> u8 {
        if self.ac_flags.contains(ACFlags::USAC) {
            self.usac_config.channel_configuration_index()
        } else {
            self.channel_configuration
        }
    }

    /// Sets channel configuration.
    pub fn set_channel_config(&mut self, chanel_config: u8) {
        self.channel_configuration = chanel_config;
    }

    /// Returns sampling frequency.
    pub fn sampling_frequency(&self) -> u32 {
        if self.ac_flags.contains(ACFlags::USAC) {
            self.usac_config.sampling_frequency()
        } else {
            self.sampling_frequency
        }
    }

    /// Sets sampling frequency.
    pub fn set_sampling_frequency(&mut self, sampling_frequency: u32) {
        self.sampling_frequency = sampling_frequency;
    }

    /// Returns sampling frequency index.
    pub fn sampling_frequency_index(&self) -> u8 {
        if self.ac_flags.contains(ACFlags::USAC) {
            self.usac_config.sampling_frequency_index()
        } else {
            self.sampling_frequency_index
        }
    }

    /// Sets sampling frequency index.
    pub fn set_sampling_frequency_index(&mut self, sampling_frequency_index: u8) {
        self.sampling_frequency_index = sampling_frequency_index;
    }

    /// Returns extension sampling frequency.
    pub fn extension_sampling_frequency(&self) -> u32 {
        if self.ac_flags.contains(ACFlags::USAC) {
            self.usac_config.extension_sampling_frequency()
        } else {
            self.extension_sampling_frequency
        }
    }

    /// Returns samples per frame depending on audio codec flags.
    pub fn samples_per_frame(&self) -> u16 {
        if self.ac_flags.contains(ACFlags::USAC) {
            // USAC samples per frame value.
            self.usac_config.samples_per_frame()
        } else if self.ac_flags.intersects(ACFlags::LD | ACFlags::ELD) {
            //  AC_LD (Low Delay) and AC_ELD (Enhanced Low Delay) number of samples per frame.
            if self.ac_flags.contains(ACFlags::FRAME_LENGTH) {
                480
            } else {
                512
            }
        } else if self.ac_flags.contains(ACFlags::FRAME_LENGTH) {
            960
        } else {
            // Default value
            1024
        }
    }

    /// Returns downscale factor.
    pub fn ds_factor(&self) -> u8 {
        if self.ac_flags.contains(ACFlags::ELD) {
            return self.eld_specific_config.downscale_factor();
        }
        1
    }

    /// Returns audio configuration flags.
    pub fn ac_flags(&self) -> ACFlags {
        self.ac_flags
    }

    /// Returns program configuration element.
    pub fn pce(&self) -> &ProgramConfig {
        &self.progr_config_element
    }

    /// Returns mutable program configuration element.
    pub fn pce_as_mut(&mut self) -> &mut ProgramConfig {
        &mut self.progr_config_element
    }

    /// Sets program configuration element.
    pub fn set_pce(&mut self, pce: ProgramConfig) {
        self.progr_config_element = pce;
    }

    /// Returns USAC specific configuration.
    pub fn usac_config(&self) -> &UsacConfig {
        &self.usac_config
    }

    /// Returns USAC specific configuration.
    pub fn usac_config_mut(&mut self) -> &mut UsacConfig {
        &mut self.usac_config
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new() {
        let asc = AudioSpecificConfig::new();
        assert!(asc.sampling_frequency_index == 0);
        assert!(asc.extension_audio_object_type == AudioObjectType::AotNullObject);
        assert!(asc.aot == AudioObjectType::AotNullObject);
        assert!(asc.ac_flags == ACFlags::empty());
        assert!(asc.channel_configuration == 0);
        assert!(asc.sampling_frequency == 0);
        assert!(asc.extension_sampling_frequency == 0);
    }

    #[test]
    fn init() {
        let mut asc: AudioSpecificConfig = Default::default();
        asc.init();
        println!("{}", asc.sampling_frequency_index);
        assert!(asc.sampling_frequency_index == 15);
        assert!(asc.extension_audio_object_type == AudioObjectType::AotNullObject);
        assert!(asc.aot == AudioObjectType::AotNone);
        assert!(asc.ac_flags == ACFlags::empty());
        assert!(asc.eld_specific_config.downscale_factor() == 1);
        assert!(asc.channel_configuration == 0);
        assert!(asc.sampling_frequency == 0);
        assert!(asc.extension_sampling_frequency == 0);
    }

    #[test]
    fn reset() {
        let mut asc: AudioSpecificConfig = Default::default();
        asc.reset();
        assert!(asc.channel_configuration == 0);
        assert!(asc.sampling_frequency == 0);
        assert!(asc.extension_sampling_frequency == 0);
        assert!(asc.sampling_frequency_index == 0);
        assert!(asc.extension_audio_object_type == AudioObjectType::AotNullObject);
        assert!(asc.aot == AudioObjectType::AotNullObject);
        assert!(asc.ac_flags == ACFlags::empty());
        assert!(asc.eld_specific_config.downscale_factor() == 0);
    }

    #[test]
    fn aot() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.aot() == AudioObjectType::AotNullObject);
    }

    #[test]
    fn extension_aot() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.extension_aot() == AudioObjectType::AotNullObject);
    }

    #[test]
    fn channel_config() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.channel_config() == 0);
    }

    #[test]
    fn sampling_frequency() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.sampling_frequency() == 0);
    }

    #[test]
    fn sampling_frequency_index() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.sampling_frequency_index() == 0);
    }

    #[test]
    fn extension_sampling_frequency() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.extension_sampling_frequency() == 0);
    }

    #[test]
    fn samples_per_frame() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.samples_per_frame() == 1024);
    }

    #[test]
    fn ds_factor() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.ds_factor() == 1);
    }

    #[test]
    fn ac_flags() {
        let asc: AudioSpecificConfig = Default::default();
        assert!(asc.ac_flags() == ACFlags::empty());
    }
}
