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
//! Advanced audio coding (AAC) decoder callbacks

use crate::{
    aac_dec::{aacdecoder::AacDecoder, config::Config},
    common::{
        aot::AudioObjectType,
        bitstream::{Bitstream, Mode},
        bs_element_id::ChannelElementId,
        enums::StereoCfgIndex,
        flags::{AACDecFlags, ACFlags},
    },
    drc_dec::CodecMode,
    sac_dec::SacDecoderError,
    tp_dec::{
        callbacks::TpDecCb, AudioSpecificConfig, ReconfigState, TpDecoderError, TransportDec,
    },
};

impl TpDecCb for AacDecoder {
    /// Decodes an AAC frame.
    ///
    /// # Parameters
    ///
    /// - `transport_decoder`: Transport decoder instance.
    ///
    /// # Return
    ///
    ///  - `Result<(), TpDecoderError>`.
    fn decode_frame(&mut self, tp_dec: &mut TransportDec) -> Result<(), TpDecoderError> {
        self.extension_data.reset();
        self.aac_core.reset(self.config.ac_flags);

        match self.aac_core.decode_frame(
            Some(tp_dec),
            &mut self.extension_data,
            &mut self.work_buffer_core,
            &mut self.map_descr,
            AACDecFlags::empty(),
            &mut None,
            &mut self.config,
            None,
        ) {
            Ok(_output_info) => Ok(()),
            Err((_error, _output_info)) => Err(TpDecoderError::UnknownError),
        }
    }

    /// Frees config dependent internal memory of `AacDecoder`.
    fn free_memory(&mut self) {
        self.aac_core.deinit();
        self.config = Config::default();

        if !self.work_buffer_core.is_empty() {
            self.work_buffer_core.clear();
            self.work_buffer_core.shrink_to_fit();
        }

        self.extension_data.deinit();
    }

    /// Updates decoder configuration and reinitializes `AacDecoder`, in case of
    /// `AudioSpecificConfig` changes.
    ///
    /// # Parameters
    ///
    /// - `asc`: Audio specific configuration to be used for updating.
    /// - `config_mode`: Reconfiguration state.
    /// - `is_config_changed`: Flag to indicate change in AAC configuration.
    ///
    /// # Return
    ///
    ///  - `Result<(), TpDecoderError>`.
    fn update_config(
        &mut self,
        asc: &AudioSpecificConfig,
        config_mode: ReconfigState,
        is_config_changed: &mut bool,
    ) -> Result<(), TpDecoderError> {
        let mut decoder_config = Config::default();
        match decoder_config.init(asc) {
            Ok(_) => (),
            Err(err) => {
                if config_mode == ReconfigState::AllocMem {
                    self.free_memory();
                }
                return if err.is_init_error() {
                    Err(TpDecoderError::UnsupportedFormat)
                } else {
                    Err(TpDecoderError::UnknownError)
                };
            }
        }

        // Detect config change.
        if config_mode == ReconfigState::DetCfgChange {
            *is_config_changed = decoder_config.is_config_change(&self.config);
        }

        // If no config change detected and implicit signalling possible keep previous mps, sbr,
        // and ps flags.
        if !(*is_config_changed)
            && !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            && decoder_config.ext_aot == AudioObjectType::AotNullObject
        {
            let mask = ACFlags::MPS_PRESENT | ACFlags::SBR_PRESENT | ACFlags::PS_PRESENT;
            decoder_config.ac_flags.remove(mask);
            decoder_config.ac_flags.insert(self.config.ac_flags & mask);
        }

        // Initialize AAC core decoder, and update decoder config.
        match self
            .aac_core
            .init(&decoder_config, asc.pce(), config_mode, *is_config_changed)
        {
            Ok(_) => (),
            Err(err) => {
                if config_mode == ReconfigState::AllocMem {
                    self.free_memory();
                }
                return if err.is_init_error() {
                    Err(TpDecoderError::UnsupportedFormat)
                } else {
                    Err(TpDecoderError::UnknownError)
                };
            }
        }

        match self
            .extension_data
            .init(asc.pce(), &decoder_config, config_mode, is_config_changed)
        {
            Ok(_) => (),
            Err(err) => {
                if config_mode == ReconfigState::AllocMem {
                    self.free_memory();
                }
                return if err.is_init_error() {
                    Err(TpDecoderError::UnsupportedFormat)
                } else {
                    Err(TpDecoderError::UnknownError)
                };
            }
        }

        if config_mode == ReconfigState::AllocMem {
            if self.work_buffer_core.is_empty() {
                let work_buffer_len = usize::from(decoder_config.num_channels)
                    * usize::from(decoder_config.frame_length);
                self.work_buffer_core = vec![0.0f32; work_buffer_len];
            }

            // Store new decoder config.
            self.config = decoder_config;
        }

        Ok(())
    }

    /// Parses `spatial specific config (SSC)` information.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `core_codec`: Audio object type of core codec.
    /// - `sampling_rate`: Sampling rate value.
    /// - `frame_size`: Frame length for core codec.
    /// - `num_channels`: Number of channels.
    /// - `stereo_config_index`: USAC stereo config index value.
    /// - `core_sbr_frame_length_index`: Core SBR frame length index value.
    /// - `config_bytes`: Configuration length in bytes.
    /// - `config_mode`: Reconfiguration state.
    /// - `is_config_changed`: Flag to indicate change in SAC configuration.
    ///
    /// # Return
    ///
    ///  - `Result<(), TpDecoderError>`.
    fn ssc_parser(
        &mut self,
        bs: &mut Bitstream,
        core_codec: AudioObjectType,
        sampling_rate: u32,
        frame_size: u16,
        num_channels: u8,
        stereo_config_index: StereoCfgIndex,
        core_sbr_frame_length_index: u8,
        config_bytes: u32,
        config_mode: ReconfigState,
        is_config_changed: bool,
    ) -> Result<(), TpDecoderError> {
        match self.extension_data.mpeg_surround_decoder.config(
            bs,
            core_codec,
            sampling_rate,
            frame_size,
            num_channels,
            stereo_config_index,
            core_sbr_frame_length_index,
            config_bytes,
            config_mode,
            is_config_changed,
        ) {
            Ok(_) => Ok(()),
            Err(SacDecoderError::UnsupportedConfig) => Err(TpDecoderError::UnsupportedFormat),
            Err(SacDecoderError::ParseError) => Err(TpDecoderError::ParseError),
            Err(_) => Err(TpDecoderError::UnknownError),
        }
    }

    /// Parses `SBR header` information.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `sampling_rate_in`: Input sampling rate value of SBR decoder.
    /// - `sampling_rate_out`: Output sampling rate value of SBR decoder.
    /// - `samples_per_frame`: Number of samples per frame.
    /// - `core_codec`: Audio object type of core codec.
    /// - `element_id`: MPEG-4 audio channel type (Element ID).
    /// - `element_index`: SBR element index value.
    /// - `is_harmonic_sbr`: Flag Signals the usage of the harmonic patching for the SBR.
    /// - `config_mode`: Reconfiguration state.
    /// - `is_config_changed`: Flag to indicate change in SBR configuration.
    /// - `downscale_factor`: ELD downscale factor value.
    ///
    /// # Return
    ///
    ///  - `Result<(), TpDecoderError>`.
    fn sbr_parser(
        &mut self,
        bs: &mut Bitstream,
        sample_rate_in: u32,
        sample_rate_out: u32,
        samples_per_frame: u16,
        core_codec: AudioObjectType,
        element_id: ChannelElementId,
        element_index: usize,
        is_harmonic_sbr: bool,
        config_mode: ReconfigState,
        is_config_changed: &mut bool,
        down_scale_factor: u8,
    ) -> Result<(), TpDecoderError> {
        let extension_data = &mut self.extension_data;
        extension_data
            .sbr_decoder
            .decode_header(
                bs,
                &mut extension_data.qmf_domain,
                &mut extension_data.qmf_domain_params_requested,
                sample_rate_in,
                sample_rate_out,
                samples_per_frame,
                core_codec,
                element_id,
                element_index,
                is_harmonic_sbr as u8,
                config_mode,
                is_config_changed,
                down_scale_factor,
            )
            .map_err(|_| TpDecoderError::ParseError)?;

        Ok(())
    }

    /// Parses `uniDrcConfig` and `LoudnessInfoSet` information.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `payload_type`: Type of payload `uniDrcConfig` (or) `LoudnessInfoSet`.
    /// - `aot`: Audio object type of core codec.
    ///
    /// # Return
    ///
    ///  - `Result<(), TpDecoderError>`.
    fn uni_drc_parser(
        &mut self,
        bs: Option<&mut Bitstream>,
        payload_type: u8,
        aot: AudioObjectType,
    ) -> Result<(), TpDecoderError> {
        let mut dummy_bs;

        let bit_stream = if let Some(bs) = bs {
            bs
        } else {
            dummy_bs = Bitstream::new(4, Mode::Reader);
            dummy_bs.init(&[0_u8; 4], 24);
            &mut dummy_bs
        };

        let drc_dec_codec_mode = if aot == AudioObjectType::AotUsac {
            CodecMode::MpegD_Usac
        } else {
            CodecMode::Undefined
        };

        self.extension_data
            .uni_drc_decoder
            .set_codec_mode(drc_dec_codec_mode)
            .map_err(|_| TpDecoderError::UnknownError)?;

        if payload_type == 0 {
            // Read uniDrcConfig.
            self.extension_data
                .uni_drc_decoder
                .read_uni_drc_config(bit_stream)
                .map_err(|_| TpDecoderError::UnknownError)?;
        } else {
            // Read loudnessInfoSet.
            self.extension_data
                .uni_drc_decoder
                .read_loudness_info_set(bit_stream)
                .map_err(|_| TpDecoderError::UnknownError)?;
        }

        Ok(())
    }
}
