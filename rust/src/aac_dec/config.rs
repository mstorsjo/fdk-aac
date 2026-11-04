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
//! AAC decoder configuration

use super::constants::*;
use super::error_codes::AacDecoderError;
use crate::common::aot::AudioObjectType;
use crate::common::audio_channel_type::AudioChannelType;
use crate::common::bs_element_id::ChannelElementId;
use crate::common::flags::*;
use crate::tp_dec::{AudioSpecificConfig, ProgramConfig, UsacExtElementConfig};
use itertools::izip;

#[repr(C)]
#[derive(Default, Debug, PartialEq)]
/// Element configuration
pub(super) struct ElementConfig {
    /// Element Id
    pub(super) element_type: ChannelElementId,
    /// USAC extension element configuration
    pub(super) extension: UsacExtElementConfig,
    /// Element specific flags
    pub(super) el_flags: ChannelFlags,
}

#[repr(C)]
#[derive(Default, Debug)]
/// AAC decoder configuration
pub struct Config {
    /// Element configuration
    pub(super) element_config: [ElementConfig; MAX_ELEMENTS],
    /// Channel types of all channels
    pub(super) channel_type: [AudioChannelType; MAX_CHANNELS],
    /// Channel indices of all channels
    pub channel_indices: [u8; MAX_CHANNELS],
    /// Index to access the channelOutputMapping table
    pub channel_map_index: u8,
    /// Number of elements
    pub(super) num_elements: u8,
    /// Number of channel elements
    pub(super) num_channel_elements: u8,
    /// Number of audio channels
    pub num_channels: u8,
    /// Channel configuration
    pub(super) channel_config: u8,
    /// Audio codec flags
    pub ac_flags: ACFlags,
    /// Audio object type
    pub(super) aot: AudioObjectType,
    /// Extension audio object type
    pub ext_aot: AudioObjectType,
    /// Core decoder frame length
    pub frame_length: u16,
    /// ELD reduced delay downscale factor
    pub(super) ds_factor: u8,

    // The following variables describe the decoder output and contain ELD reduced
    // delay downscale factor if present:
    /// Number of core decoder output samples
    pub(super) samples_per_frame: u16,
    /// Number of samples at decoder output (zero with implicit signalling)
    pub(super) ext_samples_per_frame: u16,
    /// Core decoder output sampling frequency
    pub(super) sampling_frequency: u32,
    /// Decoder output sampling frequency (zero with implicit signalling)
    pub(super) ext_sampling_frequency: u32,
    ///Core decoder sampling frequency index
    pub(super) sampling_frequency_index: u8,
}

impl Config {
    /// Initializes the AAC decoder configuration based on the provided Audio Specific Config (ASC).
    ///
    /// This function resets the decoder state and configures it according to the ASC. It handles
    /// both standard AAC and USAC cases, validates channel configurations, sets up channel
    /// maps, and calculates sampling frequencies. If the ASC specifies unsupported
    /// configurations, appropriate error codes will be returned.
    ///
    /// - Ensure that `asc` is properly configured before calling this function.
    /// - This function may modify the internal state of the decoder, so it should be used with
    ///   caution when multiple initializations are performed consecutively.
    pub fn init(&mut self, asc: &AudioSpecificConfig) -> Result<(), AacDecoderError> {
        *self = Default::default();

        self.num_channels = 0;
        self.num_channel_elements = 0;
        self.num_elements = 0;

        if !asc.ac_flags().contains(ACFlags::USAC) {
            // # Initialization for the AAC (non-USAC) case
            let mut pce = ProgramConfig::new();

            // Get channel config.
            if asc.channel_config() > 0 {
                if !pce.get_default_mpeg_config(asc.channel_config().into()) {
                    return Err(AacDecoderError::UnsupportedChannelconfig);
                }
            } else {
                pce = *asc.pce();
            }

            // Validate channel config.
            if !pce.is_valid()
                || (pce.num_channels() > MAX_CHANNELS as u8)
                || (pce.num_channels() == 0)
            {
                return Err(AacDecoderError::UnsupportedChannelconfig);
            }

            // Build element table.
            let mut element_list: [ChannelElementId; MAX_ELEMENTS] = Default::default();
            self.num_channel_elements = pce.get_element_list(&mut element_list).try_into().unwrap();

            // Set PS and LFE flags where applicable.
            for (element_cfg, pce_element_cfg) in
                izip!(self.element_config.iter_mut(), element_list.iter())
                    .take(self.num_channel_elements as usize)
            {
                element_cfg.element_type = *pce_element_cfg;
                element_cfg.el_flags =
                    if !asc.ac_flags().contains(ACFlags::ER) && (pce.num_channels() == 1) {
                        ChannelFlags::PS_POSSIBLE
                    } else if *pce_element_cfg == ChannelElementId::Lfe {
                        ChannelFlags::LFE
                    } else {
                        ChannelFlags::empty()
                    };

                self.num_elements += 1;
            }

            // For AAC ER syntax, set the last element's ID to `ID_EXT`.
            if asc.ac_flags().contains(ACFlags::ER) {
                self.element_config[self.num_elements as usize].element_type =
                    ChannelElementId::Ext;
                self.num_elements += 1;
            }

            // Set the last element's ID to `ID_END`.
            self.element_config[self.num_elements as usize].element_type = ChannelElementId::End;
            self.num_elements += 1;

            // Set number of channels.
            self.num_channels = pce.num_channels();

            // Set channel map index according to given channel config.
            self.channel_map_index = if asc.channel_config() > 0 {
                asc.channel_config()
            } else {
                pce.get_channel_map_index()
            };

            pce.get_channel_description(&mut self.channel_type, &mut self.channel_indices);
        } else {
            // # Initialization for the USAC case
            let usac_config = asc.usac_config();

            for (i, element_config) in self
                .element_config
                .iter_mut()
                .take(usac_config.num_elements() as usize)
                .enumerate()
            {
                let usac_el_cfg = usac_config.element_config(i);
                element_config.el_flags = usac_el_cfg.ac_el_flags();
                element_config.element_type = usac_el_cfg.element_type();
                element_config.extension = usac_el_cfg.ext_element();

                if usac_el_cfg.element_type().is_channel_element() {
                    self.num_channels += if usac_el_cfg.element_type() == ChannelElementId::UsacCpe
                        && !asc.ac_flags().contains(ACFlags::USAC_SCFGI1)
                    {
                        2
                    } else {
                        1
                    };

                    self.num_channel_elements += 1;
                }
                self.num_elements += 1;
            }

            // Validate channel config.
            if (self.num_channels > 2) || (self.num_channels == 0) {
                return Err(AacDecoderError::UnsupportedChannelconfig);
            }

            // Set the last element's ID to `ID_END`.
            self.element_config[self.num_elements as usize].element_type = ChannelElementId::End;
            self.num_elements += 1;

            self.channel_map_index = asc.channel_config();

            // Set channel description (type and index)
            for (ch_index, (channel_type, channel_indices)) in izip!(
                self.channel_type.iter_mut(),
                self.channel_indices.iter_mut(),
            )
            .take(self.num_channels.into())
            .enumerate()
            {
                *channel_type = AudioChannelType::Front;
                *channel_indices = ch_index as u8;
            }
        }

        self.channel_config = asc.channel_config();
        self.ac_flags = asc.ac_flags();

        self.aot = asc.aot();
        self.ext_aot = asc.extension_aot();
        self.frame_length = asc.samples_per_frame();
        self.ds_factor = asc.ds_factor();

        // Describe the decoder output including ELD reduced delay downscale factor.
        self.samples_per_frame = asc.samples_per_frame() / asc.ds_factor() as u16;

        if (asc.extension_sampling_frequency() == 0)
            && !asc.ac_flags().intersects(ACFlags::USAC | ACFlags::ER)
        {
            self.ext_samples_per_frame = 0;
        } else if asc.extension_sampling_frequency() > 0 {
            self.ext_samples_per_frame = 0;
            // SBR ratio for AAC is either 1 or 2.
            // For USAC, it is 1, 2, 8/3 or 4.
            if asc.ac_flags().contains(ACFlags::USAC) && asc.samples_per_frame() == 768 {
                self.ext_samples_per_frame = asc.samples_per_frame() * 8 / 3;
            } else {
                let upper_limit = if asc.ac_flags().contains(ACFlags::USAC) {
                    3
                } else {
                    2
                };

                if let Some(i) = (0..upper_limit).find(|&i| {
                    asc.sampling_frequency() == (asc.extension_sampling_frequency() >> i)
                }) {
                    self.ext_samples_per_frame =
                        asc.samples_per_frame() * (1 << i) / asc.ds_factor() as u16;
                }
            }

            if self.ext_samples_per_frame == 0 {
                return Err(AacDecoderError::UnsupportedSamplingrate);
            }
        } else {
            self.ext_samples_per_frame = asc.samples_per_frame();
        }

        self.sampling_frequency = asc.sampling_frequency() / asc.ds_factor() as u32;
        self.ext_sampling_frequency = asc.extension_sampling_frequency() / asc.ds_factor() as u32;
        self.sampling_frequency_index = asc.sampling_frequency_index();

        // MPEG4 SBR Enhancements are supported only for mono and stereo, and for aac sample rate
        // equal to or smaller than 24 kHz.
        if self.ac_flags.contains(ACFlags::MPEG4_ESBR)
            && ((self.num_channels > 2) || (self.sampling_frequency > 24000))
        {
            self.ac_flags.remove(ACFlags::MPEG4_ESBR)
        }

        self.check_sampling_rate()?;

        Ok(())
    }

    /// Compares the current AAC decoder configuration with another configuration to determine if
    /// any significant changes have occurred.
    ///
    /// - This function is useful for determining if reinitialization or updates to the decoder are
    ///   necessary based on configuration changes.
    pub fn is_config_change(&self, config2: &Self) -> bool {
        // Define audio codec flags to be compared.
        let mut mask: ACFlags = ACFlags::MPS_PRESENT
            | ACFlags::SBR_PRESENT
            | ACFlags::PS_PRESENT
            | ACFlags::USAC_SCFGI1
            | ACFlags::USAC_SCFGI2
            | ACFlags::USAC_SCFGI3;

        if !self.ac_flags.intersects(ACFlags::USAC | ACFlags::ER)
            && (self.ext_aot == AudioObjectType::AotNullObject)
        {
            // Implicit signaling possible.
            mask.remove(ACFlags::MPS_PRESENT | ACFlags::SBR_PRESENT | ACFlags::PS_PRESENT);
        }

        // Compare flags.
        if (self.ac_flags ^ config2.ac_flags) & mask != ACFlags::empty() {
            return true;
        }

        if self.ds_factor != config2.ds_factor {
            return true;
        }

        if self.ext_sampling_frequency != config2.ext_sampling_frequency {
            return true;
        }

        if (self.sampling_frequency != config2.sampling_frequency)
            || (self.samples_per_frame != config2.samples_per_frame)
        {
            return true;
        }

        if izip!(
            self.element_config[..config2.num_elements.into()].iter(),
            config2.element_config[..config2.num_elements.into()].iter()
        )
        .any(|(elem1, elem2)| elem1.element_type != elem2.element_type)
        {
            return true;
        }

        if self.channel_config != config2.channel_config {
            return true;
        }

        if (self.aot != config2.aot) || (self.ext_aot != config2.ext_aot) {
            return true;
        }

        // Check if amount of asc channels has changed.
        if self.num_channels != config2.num_channels {
            return true;
        }

        false
    }

    /// Performs sanity checks on read sampling rate information.
    ///
    /// This function checks a given AAC decoder config to hold valid sample rate information.
    /// For that, next to doing some basic checks, it also uses information about the AOT and
    /// the extension AOT to validate that the given sample rate is correct.
    fn check_sampling_rate(&self) -> Result<(), AacDecoderError> {
        // Check if the sampling frequency is zero, which is not allowed.
        if self.sampling_frequency == 0 {
            return Err(AacDecoderError::UnsupportedSamplingrate);
        }

        // Check if the sampling frequency exceeds the maximum allowed value.
        if (if self.ac_flags.contains(ACFlags::SBR_PRESENT) {
            self.ext_sampling_frequency
        } else {
            self.sampling_frequency
        }) > 96000
        {
            return Err(AacDecoderError::UnsupportedSamplingrate);
        }

        // AAC-only (i.e. non-USAC) checks
        if !self.ac_flags.contains(ACFlags::USAC) {
            // Verify if the sampling frequency is among the supported values.
            let supported_frequencies = [
                96000, 88200, 64000, 16000, 12000, 11025, 8000, 7350, 48000, 44100, 32000, 24000,
                22050,
            ];
            if !supported_frequencies.contains(&self.sampling_frequency) {
                return Err(AacDecoderError::UnsupportedSamplingrate);
            }

            // For AAC with SBR, the extended sampling frequency must be equal to or double the core
            // sample rate.
            if self.ac_flags.contains(ACFlags::SBR_PRESENT)
                && (self.ext_sampling_frequency != self.sampling_frequency)
                && (self.ext_sampling_frequency != (2 * self.sampling_frequency))
            {
                return Err(AacDecoderError::UnsupportedSamplingrate);
            }
        }

        Ok(())
    }
}
