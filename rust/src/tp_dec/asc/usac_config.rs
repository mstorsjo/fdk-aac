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
//! USAC config (ASC)

use super::super::{
    callbacks::TpDecCallBacks,
    constants::{TP_USAC_MAX_CONFIG_LEN, TP_USAC_MAX_ELEMENTS},
    error_codes::TpDecoderError,
    tables::{SBR_RATIO_INDEX, USAC_FRAME_LENGTH},
};
use super::{
    helper_functions::{get_sample_rate, skip_sbr_header},
    usac_ext_element_config::UsacElementConfig,
};
use crate::common::aot::AudioObjectType;
use crate::common::bs_element_id::{ChannelElementId, USAC_ID_BIT};
use crate::common::samplerate_index::get_sample_rate_idx;
use crate::common::{
    bitstream::{Bitstream, Mode},
    flags::ACFlags,
};
use crate::common::{enums::StereoCfgIndex, flags::ChannelFlags};
use std::mem;

/// USAC configuration extension types.
// usacConfigExtType q.v. ISO/IEC 23003-3:2020(E) Table 79.
#[derive(PartialEq, Copy, Clone, Debug)]
enum ConfigExtId {
    Fill = 0,
    LoudnessInfo = 2,
}

impl From<u32> for ConfigExtId {
    fn from(value: u32) -> Self {
        match value {
            0 => ConfigExtId::Fill,
            2 => ConfigExtId::LoudnessInfo,
            _ => panic!("invalid value: {value}"),
        }
    }
}

/// USAC extension element types.
// usacExtElementType q.v. ISO/IEC 23003-3:2020(E) Table 86.
#[repr(C)]
#[derive(PartialEq, Copy, Clone, Debug, Default)]
pub enum UsacExtElementType {
    #[default]
    Fill = 0x00,
    Audiopreroll = 0x03,
    UniDrc = 0x04,
    Unknown = 0xFF,
}

impl From<u32> for UsacExtElementType {
    fn from(value: u32) -> Self {
        match value {
            0x00 => UsacExtElementType::Fill,
            0x03 => UsacExtElementType::Audiopreroll,
            0x04 => UsacExtElementType::UniDrc,
            0xFF => UsacExtElementType::Unknown,
            _ => panic!("invalid value: {value}"),
        }
    }
}

/// USAC configuration.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct UsacConfig {
    core_sbr_frame_length_index: u8,
    sbr_ratio_index: u8,
    channel_configuration_index: u8,
    usac_num_elements: u32,
    element: [UsacElementConfig; TP_USAC_MAX_ELEMENTS],
    sampling_frequency: u32,
    sampling_frequency_index: u8,
    extension_sampling_frequency: u32,
    samples_per_frame: u16,
    config_buffer: [u8; TP_USAC_MAX_CONFIG_LEN],
    config_length: u16,
}

impl UsacConfig {
    /// Creates a new USAC Config instance.
    pub fn new() -> Self {
        UsacConfig::default()
    }

    /// Returns channel configuration index.
    pub fn channel_configuration_index(&self) -> u8 {
        self.channel_configuration_index
    }

    /// Returns sampling frequnecy.
    pub fn sampling_frequency(&self) -> u32 {
        self.sampling_frequency
    }

    /// Returns sampling frequency index.
    pub fn sampling_frequency_index(&self) -> u8 {
        self.sampling_frequency_index
    }

    /// Returns extension sampling frequnecy.
    pub fn extension_sampling_frequency(&self) -> u32 {
        self.extension_sampling_frequency
    }

    /// Returns the number of samples per frame of a particular audio channel.
    pub fn samples_per_frame(&self) -> u16 {
        self.samples_per_frame
    }

    /// Returns config length.
    pub fn config_length(&self) -> u16 {
        self.config_length
    }

    /// Sets config length.
    pub fn set_config_length(&mut self, config_length: u16) {
        self.config_length = config_length;
    }

    /// Returns reference to mutable config buffer.
    pub fn config_buffer_mut(&mut self) -> &mut [u8; TP_USAC_MAX_CONFIG_LEN] {
        &mut self.config_buffer
    }

    /// Returns config buffer.
    pub fn config_buffer(&self) -> &[u8; TP_USAC_MAX_CONFIG_LEN] {
        &self.config_buffer
    }

    /// Clears the USAC Config values.
    pub(super) fn reset(&mut self) {
        self.core_sbr_frame_length_index = 0;
        self.sbr_ratio_index = 0;
        self.channel_configuration_index = 0;
        self.usac_num_elements = 0;
        self.element = [UsacElementConfig::default(); TP_USAC_MAX_ELEMENTS];
        self.sampling_frequency = 0;
        self.sampling_frequency_index = 0;
        self.extension_sampling_frequency = 0;
        self.samples_per_frame = 0;
        self.config_buffer = [0; TP_USAC_MAX_CONFIG_LEN];
        self.config_length = 0;
    }

    /// Parse the USAC Config.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    pub(super) fn parse(
        &mut self,
        ac_flags: &mut ACFlags,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        // Start bit position of USAC config.
        let n_bits = bs.valid_bits();

        self.sampling_frequency = get_sample_rate(bs, Some(&mut self.sampling_frequency_index), 5);
        if self.sampling_frequency == 0 {
            return Err(TpDecoderError::ParseError);
        }

        // Reserved values.
        self.core_sbr_frame_length_index = bs.read(3) as u8;
        if self.core_sbr_frame_length_index > 4 {
            return Err(TpDecoderError::ParseError);
        }

        if self.set_core_sbr_frame_length_index(ac_flags).is_err() {
            return Err(TpDecoderError::ParseError);
        }

        // Only channelConfigurationIndex = [1,2] are supported.
        self.channel_configuration_index = bs.read(5) as u8;
        if self.channel_configuration_index == 0 || self.channel_configuration_index > 2 {
            return Err(TpDecoderError::ParseError);
        }

        let mut error_status = self.decoder_config_parse(ac_flags, bs, cb);
        error_status?;

        if bs.read_bit() != 0 {
            // usacConfigExtensionPresent
            error_status = self.config_extension(bs, cb);
            error_status?;
        } else {
            // no loudnessInfoSet contained
            error_status = cb.uni_drc_callback(None, 1, AudioObjectType::AotUsac);
            error_status?;
        }

        // Store UsacConfig bitstream data.
        let config_bits = n_bits - bs.valid_bits();
        bs.push(-config_bits);

        if self.store_bs_data(bs, config_bits).is_err() {
            return Err(TpDecoderError::ParseError);
        }
        error_status
    }

    /// Mapping of coreSbrFrameLengthIndex defined by Table 70 in ISO/IEC 23003-3.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub(super) fn set_core_sbr_frame_length_index(
        &mut self,
        ac_flags: &mut ACFlags,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());

        self.samples_per_frame = USAC_FRAME_LENGTH[self.core_sbr_frame_length_index as usize];
        self.sbr_ratio_index = SBR_RATIO_INDEX[self.core_sbr_frame_length_index as usize];

        if self.sbr_ratio_index > 0 {
            ac_flags.insert(ACFlags::SBR_PRESENT);
            self.extension_sampling_frequency = self.sampling_frequency;

            match self.sbr_ratio_index {
                // sbrRatio = 4:1
                1 => {
                    self.sampling_frequency >>= 2;
                    self.samples_per_frame >>= 2;
                }
                // sbrRatio = 8:3
                2 => {
                    self.sampling_frequency = (self.sampling_frequency * 3) / 8;
                    self.samples_per_frame = (self.samples_per_frame * 3) / 8;
                }
                // sbrRatio = 2:1
                3 => {
                    self.sampling_frequency >>= 1;
                    self.samples_per_frame >>= 1;
                }
                _ => {
                    error_status = Err(TpDecoderError::ParseError);
                }
            }
            error_status?;
            self.sampling_frequency_index = get_sample_rate_idx(self.sampling_frequency, 4);
        }
        error_status
    }

    /// Stores USAC config bitstream data.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `config_bits`: Configuration bits.
    pub(super) fn store_bs_data(
        &mut self,
        bs: &mut Bitstream,
        config_bits: isize,
    ) -> Result<(), TpDecoderError> {
        if config_bits as usize > (mem::size_of_val(&self.config_buffer) << 3) {
            Err(TpDecoderError::ParseError)
        } else {
            let mut bs_writer = Bitstream::new(TP_USAC_MAX_CONFIG_LEN, Mode::Writer);

            for _j in 0..(config_bits / 32) {
                bs_writer.write(bs.read(32), 32);
            }
            let num_bits: u8 = (config_bits & 0x1f) as u8;
            bs_writer.write(bs.read(num_bits), num_bits);
            bs_writer.align(0);
            self.config_length = (bs_writer.valid_bits() >> 3) as u16;
            self.config_buffer.copy_from_slice(bs_writer.buffer());

            Ok(())
        }
    }

    /// Parse decoder configuration.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    pub(super) fn decoder_config_parse(
        &mut self,
        ac_flags: &mut ACFlags,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        // index for elements which contain audio channels (sce, cpe, lfe)
        let mut channel_element_idx = 0;
        // index of uniDrc extension element. -1 if not contained.
        let mut uni_drc_element: i32 = -1;

        self.usac_num_elements = bs.escaped_value(4, 8, 16) + 1;
        if self.usac_num_elements as usize > TP_USAC_MAX_ELEMENTS {
            return Err(TpDecoderError::UnsupportedFormat);
        }

        for i in 0..self.usac_num_elements {
            let mut ac_el_flags = ChannelFlags::empty();

            // Set `USAC_ID_BIT` to map `usacElementType` to `MP4_ELEMENT_ID` enum.
            let usac_element_type: ChannelElementId =
                ChannelElementId::from(bs.read(2) | USAC_ID_BIT);
            self.element[i as usize].set_element_type(usac_element_type);

            match usac_element_type {
                ChannelElementId::UsacLfe => {
                    // supported channel configurations don't contain lfe
                    return Err(TpDecoderError::ParseError);
                }
                ChannelElementId::UsacExt => {
                    error_status = self.element[i as usize].ext_element.config(bs, cb);
                    if self.element[i as usize].ext_element.ext_element_type()
                        == UsacExtElementType::UniDrc
                    {
                        uni_drc_element = i as i32;
                    }
                    if self.element[i as usize].ext_element.ext_element_type()
                        == UsacExtElementType::Audiopreroll
                        && i == 0
                    {
                        ac_flags.insert(ACFlags::USAC_HAS_PREROLL);
                    }
                    error_status?;
                }
                // UsacCoreConfig() ISO/IEC FDIS 23003-3  Table 10
                ChannelElementId::UsacSce => {
                    if bs.read_bit() != 0 {
                        return Err(TpDecoderError::UnsupportedFormat);
                    }

                    ac_el_flags.insert(if bs.read_bit() != 0 {
                        ChannelFlags::USAC_NOISE
                    } else {
                        ChannelFlags::empty()
                    });
                    // end of UsacCoreConfig()

                    if ac_flags.contains(ACFlags::SBR_PRESENT) {
                        //SbrConfig() ISO/IEC FDIS 23003-3  Table 11
                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_HBE
                        } else {
                            ChannelFlags::empty()
                        });

                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_ITES
                        } else {
                            ChannelFlags::empty()
                        });

                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_PVC
                        } else {
                            ChannelFlags::empty()
                        });

                        error_status = cb.sbr_callback(
                            bs,
                            AudioObjectType::AotUsac,
                            ChannelElementId::Sce,
                            self.sampling_frequency,
                            self.extension_sampling_frequency,
                            self.samples_per_frame,
                            channel_element_idx,
                            1,
                            ac_el_flags.contains(ChannelFlags::USAC_HBE),
                        );

                        if error_status.is_err() {
                            if error_status == Err(TpDecoderError::UnknownError) {
                                return Err(TpDecoderError::UnknownError);
                            }
                            return Err(TpDecoderError::ParseError);
                        }
                        // end of SbrConfig()
                        ac_flags.insert(if ac_el_flags.contains(ChannelFlags::USAC_HBE) {
                            ACFlags::HBE_PRESENT
                        } else {
                            ACFlags::empty()
                        });
                    }
                    channel_element_idx += 1;
                }
                ChannelElementId::UsacCpe => {
                    // UsacCoreConfig() ISO/IEC FDIS 23003-3  Table 10
                    if bs.read_bit() != 0 {
                        return Err(TpDecoderError::UnsupportedFormat);
                    }
                    ac_el_flags.insert(if bs.read_bit() != 0 {
                        ChannelFlags::USAC_NOISE
                    } else {
                        ChannelFlags::empty()
                    });
                    ac_el_flags.insert(ChannelFlags::USAC_CP_POSSIBLE);
                    // end of UsacCoreConfig()

                    if ac_flags.contains(ACFlags::SBR_PRESENT) {
                        // SbrConfig() ISO/IEC FDIS 23003-3
                        let sbr_config_anchor = bs.valid_bits();

                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_HBE
                        } else {
                            ChannelFlags::empty()
                        });

                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_ITES
                        } else {
                            ChannelFlags::empty()
                        });

                        ac_el_flags.insert(if bs.read_bit() != 0 {
                            ChannelFlags::USAC_PVC
                        } else {
                            ChannelFlags::empty()
                        });

                        skip_sbr_header(bs, true);

                        // read stereoConfigIndex
                        let stereo_config_index = StereoCfgIndex::from(bs.read(2) as u8);

                        // rewind to sbr header
                        let num_bits = sbr_config_anchor - 3 - bs.valid_bits();
                        bs.push(-num_bits);

                        let el_type = if stereo_config_index == StereoCfgIndex::Mps212
                            || stereo_config_index == StereoCfgIndex::Mps212wResMonoSbr
                        {
                            ChannelElementId::Sce
                        } else {
                            ChannelElementId::Cpe
                        };

                        error_status = cb.sbr_callback(
                            bs,
                            AudioObjectType::AotUsac,
                            el_type,
                            self.sampling_frequency,
                            self.extension_sampling_frequency,
                            self.samples_per_frame,
                            channel_element_idx,
                            1,
                            ac_el_flags.contains(ChannelFlags::USAC_HBE),
                        );

                        if error_status.is_err() {
                            if error_status == Err(TpDecoderError::UnknownError) {
                                return error_status;
                            } else {
                                return Err(TpDecoderError::ParseError);
                            }
                        }
                        // end of SbrConfig()

                        ac_flags.insert(if ac_el_flags.contains(ChannelFlags::USAC_HBE) {
                            ACFlags::HBE_PRESENT
                        } else {
                            ACFlags::empty()
                        });

                        // skip stereo_config_index
                        bs.push(2);

                        if stereo_config_index > StereoCfgIndex::RegularCpe {
                            ac_el_flags.insert(ChannelFlags::USAC_MPS212);
                            ac_el_flags.remove(ChannelFlags::USAC_CP_POSSIBLE);

                            // Mps212Config() ISO/IEC FDIS 23003-3
                            // only downmix channels (residual channels are not counted
                            // don't know the length.
                            error_status = cb.ssc_callback(
                                bs,
                                AudioObjectType::AotUsac,
                                self.extension_sampling_frequency,
                                USAC_FRAME_LENGTH[self.core_sbr_frame_length_index as usize],
                                1,
                                stereo_config_index,
                                self.core_sbr_frame_length_index,
                                0,
                            );

                            if error_status.is_err() {
                                if error_status == Err(TpDecoderError::UnknownError) {
                                    return error_status;
                                } else {
                                    return Err(TpDecoderError::ParseError);
                                }
                            }

                            ac_flags.insert(ACFlags::MPS_PRESENT);
                            // end of Mps212Config()

                            ac_flags.insert(if stereo_config_index == StereoCfgIndex::Mps212 {
                                ACFlags::USAC_SCFGI1
                            } else {
                                ACFlags::empty()
                            });

                            ac_flags.insert(
                                if stereo_config_index == StereoCfgIndex::Mps212wResMonoSbr {
                                    ACFlags::USAC_SCFGI2
                                } else {
                                    ACFlags::empty()
                                },
                            );

                            ac_flags.insert(
                                if stereo_config_index == StereoCfgIndex::Mps212wResStereoSbr {
                                    ACFlags::USAC_SCFGI3
                                } else {
                                    ACFlags::empty()
                                },
                            );
                        }
                        channel_element_idx += 1;
                    }
                }
                // non USAC-element encountered
                _ => return Err(TpDecoderError::ParseError),
            };
            self.element[i as usize].set_ac_el_flags(ac_el_flags);
        }

        if channel_element_idx > 1 {
            return Err(TpDecoderError::ParseError);
        }

        if uni_drc_element == -1 {
            // no uniDrcConfig contained
            error_status = cb.uni_drc_callback(None, 0, AudioObjectType::AotUsac);
            error_status?;
        }
        error_status
    }

    /// Config extension.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    pub(super) fn config_extension(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());

        let num_config_extension = bs.escaped_value(2, 4, 8) + 1;
        let mut usac_config_ext_type: u32;
        let mut usac_config_ext_length: isize;
        // Index of loudnessInfoSet config extension. -1 if not contained.
        let mut loudness_info_set_index: i32 = -1;
        // Bit field to detect unallowed multiple extension configs.
        let mut ext_is_present = 0;

        for conf_ext_idx in 0..num_config_extension {
            usac_config_ext_type = bs.escaped_value(4, 8, 16);
            usac_config_ext_length = bs.escaped_value(4, 8, 16) as isize;

            // Start bit position of config extension.
            let n_bits = bs.valid_bits();

            // Return an error in case the bitbuffer fill level is too low.
            if n_bits < (usac_config_ext_length * 8) {
                return Err(TpDecoderError::NotEnoughBits);
            }

            match usac_config_ext_type {
                0 => {
                    for _i in 0..usac_config_ext_length {
                        if bs.read(8) != 0xa5 {
                            return Err(TpDecoderError::ParseError);
                        }
                    }
                }
                2 => {
                    if (ext_is_present & (1 << (ConfigExtId::LoudnessInfo as u32))) != 0 {
                        return Err(TpDecoderError::ParseError);
                    }
                    ext_is_present |= 1 << (ConfigExtId::LoudnessInfo as u32);
                    {
                        // loudnessInfoSet
                        error_status = cb.uni_drc_callback(Some(bs), 1, AudioObjectType::AotUsac);
                        error_status?;
                        loudness_info_set_index = conf_ext_idx as i32;
                    }
                }
                _ => (),
            };

            // Skip remaining bits. If too many bits were parsed, assume error.
            usac_config_ext_length = (8 * usac_config_ext_length) - (n_bits - bs.valid_bits());
            if usac_config_ext_length < 0 {
                return Err(TpDecoderError::ParseError);
            }
            bs.push(usac_config_ext_length);
        }

        if loudness_info_set_index == -1 {
            // no loudnessInfoSet contained
            error_status = cb.uni_drc_callback(None, 1, AudioObjectType::AotUsac);
            error_status?;
        }

        error_status
    }

    /// Returns number of elements.
    pub fn num_elements(&self) -> u32 {
        self.usac_num_elements
    }

    /// Returns config of element with given index.
    ///
    /// # Parameters
    ///
    /// - `index`: Element index.
    pub fn element_config(&self, index: usize) -> &UsacElementConfig {
        &self.element[index]
    }
}

impl Default for UsacConfig {
    /// Default UsacConfig
    fn default() -> Self {
        Self {
            core_sbr_frame_length_index: Default::default(),
            sbr_ratio_index: Default::default(),
            channel_configuration_index: Default::default(),
            usac_num_elements: Default::default(),
            element: Default::default(),
            sampling_frequency: Default::default(),
            sampling_frequency_index: Default::default(),
            extension_sampling_frequency: Default::default(),
            samples_per_frame: Default::default(),
            config_buffer: [0; TP_USAC_MAX_CONFIG_LEN],
            config_length: Default::default(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new() {
        let usac = UsacConfig::new();
        assert!(usac.core_sbr_frame_length_index == 0);
        assert!(usac.sbr_ratio_index == 0);
        assert!(usac.channel_configuration_index == 0);
        assert!(usac.usac_num_elements == 0);
        assert!(usac.element.len() == 9);
        assert!(usac.sampling_frequency == 0);
        assert!(usac.sampling_frequency_index == 0);
        assert!(usac.extension_sampling_frequency == 0);
        assert!(usac.samples_per_frame == 0);
        assert!(usac.config_buffer.len() == 512);
        assert!(usac.config_length == 0);
    }

    #[test]
    fn reset() {
        let mut usac = UsacConfig {
            core_sbr_frame_length_index: 1,
            sbr_ratio_index: 1,
            channel_configuration_index: 1,
            usac_num_elements: 1,
            element: [UsacElementConfig::new(); 9],
            sampling_frequency: 1,
            sampling_frequency_index: 1,
            extension_sampling_frequency: 1,
            samples_per_frame: 1,
            config_buffer: [0; 512],
            config_length: 1,
        };

        usac.reset();

        assert!(usac.core_sbr_frame_length_index == 0);
        assert!(usac.sbr_ratio_index == 0);
        assert!(usac.channel_configuration_index == 0);
        assert!(usac.usac_num_elements == 0);
        assert!(usac.sampling_frequency == 0);
        assert!(usac.sampling_frequency_index == 0);
        assert!(usac.extension_sampling_frequency == 0);
        assert!(usac.samples_per_frame == 0);
        assert!(usac.config_length == 0);
    }

    #[test]
    fn num_elements() {
        let usac = UsacConfig::default();
        assert!(usac.num_elements() == 0);
    }

    #[test]
    fn element_config() {
        let usac = UsacConfig::default();
        let usac_ele = usac.element_config(0);
        assert!(usac_ele.element_type() == ChannelElementId::None);
        assert!(usac_ele.ac_el_flags() == ChannelFlags::empty());
        assert!(usac_ele.ext_element().ext_element_type() == UsacExtElementType::Fill);
        assert!(usac_ele.ext_element().default_length() == 0);
        assert!(!usac_ele.ext_element().is_fragmented_payload());
    }
}
