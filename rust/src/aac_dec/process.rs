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
//! AAC decoder core processing
//!
//! This module consumes the AAC core bistream data and from this, generates the
//! time-domain output.

use crate::{
    aac_dec::{
        channel::{ChannelElement, CommonChannelData, ElState},
        channel_info::BlockType,
        conceal::ConcealmentData,
        config::Config,
        error_codes::{AacDecoderError, Cluster},
        ext_data::ExtensionData,
        output_info::OutputInfo,
        params::Params,
        sr_info::SamplingRateInfo,
    },
    common::{
        bs_element_id::ChannelElementId,
        channel_map_descr::ChannelMapDescriptor,
        flags::{AACDecFlags, ACFlags, ChannelFlags},
    },
    sbr_dec::constants::MAX_EL_CHANNELS,
    tp_dec::{ProgramConfig, ReconfigState, TransportDec},
};

/// AAC decoder initialization state.
#[derive(Copy, Clone, Default, Debug, PartialEq)]
#[repr(C)]
pub enum InitState {
    #[default]
    /// Decoder instance uninitialized.
    None = 0,

    /// Decoder instance initialized according to provided decoder configuration.
    Startup = 1,

    /// Decoder instance initialized and first access unit successfully parsed.
    Complete = 2,
}

#[derive(Default, Debug)]
#[repr(C)]
pub struct Process {
    /// Data related to concealment handling.
    pub conceal_data: ConcealmentData,

    /// Common channel data for the AAC decoder.
    pub common_channel_data: CommonChannelData,

    /// AAC Decoder Coupling Channel Element (CCE).
    pub channel_element_cce: Option<Box<ChannelElement>>,

    /// A vector of structures representing the AAC decoder channel elements
    /// with length and capacity.
    pub channel_elements: Vec<ChannelElement>,

    /// Number of core channels in the decoder.
    pub num_core_channels: u8,

    /// Indicates the current decoder startup phase.
    pub init_state: InitState,
}

impl Process {
    // API functions

    /// Creates a new instance.
    ///
    /// Returns a default-initialized AAC deocder core instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Initializes the AAC decoder core.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: AAC decoder configuration.
    /// - `program_config_element`: PCE structure.
    /// - `config_mode`: Reconfiguration state.
    /// - `config_changed`: Indictaes whetehr teh configuration has changed.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn init(
        &mut self,
        decoder_config: &Config,
        program_config_element: &ProgramConfig,
        config_mode: ReconfigState,
        config_changed: bool,
    ) -> Result<(), AacDecoderError> {
        // Check if samplerate changed.
        let mut sampling_rate_info = SamplingRateInfo::default();

        sampling_rate_info.init(
            decoder_config.frame_length,
            decoder_config.sampling_frequency_index.into(),
            decoder_config.sampling_frequency * u32::from(decoder_config.ds_factor),
        )?;

        if config_mode == ReconfigState::AllocMem {
            if config_changed {
                // Allocate all memory structures for each channel.
                let mut ch = 0;
                self.channel_elements =
                    Vec::with_capacity(decoder_config.num_channel_elements.into());

                for el_cfg in decoder_config
                    .element_config
                    .iter()
                    .take(decoder_config.num_elements.into())
                {
                    if el_cfg.element_type.is_channel_element() {
                        self.channel_elements.push(ChannelElement::new());
                        let current_ch_element = self.channel_elements.len() - 1;

                        let instance_tag: u8 = if decoder_config.channel_config != 0 {
                            255
                        } else {
                            program_config_element.get_element_associated_instance_tag(
                                current_ch_element,
                                el_cfg.element_type,
                            )
                        };
                        self.channel_elements[current_ch_element].init(
                            &sampling_rate_info,
                            el_cfg.element_type,
                            instance_tag,
                            decoder_config.frame_length.into(),
                            el_cfg.el_flags,
                            decoder_config.ac_flags,
                        );

                        ch += self.channel_elements[current_ch_element].num_channels();
                    }
                }
                if ch != decoder_config.num_channels {
                    return self.bail_from_init();
                }

                self.common_channel_data.init(
                    &sampling_rate_info,
                    decoder_config.num_channel_elements == ch,
                    decoder_config.ac_flags,
                );

                self.conceal_data.init(decoder_config.num_channels.into());

                if !decoder_config
                    .ac_flags
                    .intersects(ACFlags::USAC | ACFlags::ER)
                    && decoder_config.num_channels > 2
                {
                    self.channel_element_cce = Some(Box::new(ChannelElement::new()));

                    self.channel_element_cce.as_mut().unwrap().init(
                        &sampling_rate_info,
                        ChannelElementId::Cce,
                        255,
                        decoder_config.frame_length.into(),
                        ChannelFlags::GA_CCE,
                        decoder_config.ac_flags,
                    );
                }

                self.num_core_channels = decoder_config.num_channels;
                self.init_state = InitState::Startup;
            } // config_changed

            self.accept_flags(decoder_config);
        } // config_mode == ReconfigState::AllocMem

        Ok(())
    }

    /// De-initializes the AAC decoder core.
    pub fn deinit(&mut self) {
        self.channel_element_cce = None;

        self.conceal_data.deinit_conceal_info();

        self.common_channel_data.er_hcr_data = None;
        self.common_channel_data.pulse_data = None;

        self.channel_elements.clear();
        self.channel_elements.shrink_to_fit();

        *self = Process::default();
    }

    /// Resets AAC core decoder.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio codec flags.
    pub fn reset(&mut self, ac_flags: ACFlags) {
        for el in &mut self.channel_elements {
            el.reset(ac_flags);
        }
    }

    /// Decodes an AAC frame.
    ///
    /// The processing is done in three steps:
    /// * Read the bitstream.
    /// * Decode the bitstream data.
    /// * Render the time signal.
    ///
    /// # Parameters
    ///
    /// - `tp_dec_option`: Optional transport decoder.
    /// - `extension_data`: Bitstream extension data.
    /// - `work_buffer_core`: Work buffer for spectral data.
    /// - `map_descr`: Channel map descriptors.
    /// - `flags`: AAC decoder flags.
    /// - `time_data`: Time domain output buffer.
    /// - `decoder_config`: AAC decoder configuration.
    /// - `preroll_au_length`: Length of the preroll AU. `None` for non-USAC bitstreams.
    ///
    /// # Returns
    /// - `OutputInfo` containing information about the decoded output, and in case of an error,
    ///   also the error code.
    #[expect(clippy::too_many_arguments)]
    pub fn decode_frame(
        &mut self,
        tp_dec_option: Option<&mut TransportDec>,
        extension_data: &mut ExtensionData,
        work_buffer_core: &mut [f32],
        map_descr: &mut ChannelMapDescriptor,
        flags: AACDecFlags,
        time_data: &mut Option<&mut [f32]>,
        decoder_config: &mut Config,
        preroll_au_length: Option<u32>,
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        if let Some(time_data) = time_data.as_ref() {
            if time_data.len()
                < decoder_config.samples_per_frame as usize * decoder_config.num_channels as usize
            {
                return self.create_output_info(
                    Err(AacDecoderError::OutputBufferTooSmall),
                    decoder_config,
                    map_descr,
                    extension_data,
                );
            }
        }

        if work_buffer_core.len()
            < decoder_config.frame_length as usize * decoder_config.num_channels as usize
        {
            return self.create_output_info(
                Err(AacDecoderError::Unknown),
                decoder_config,
                map_descr,
                extension_data,
            );
        }

        // Decoder must at least be initialized.
        if self.init_state == InitState::None {
            return self.create_output_info(
                Err(AacDecoderError::Unknown),
                decoder_config,
                map_descr,
                extension_data,
            );
        }

        let mut decode_state = Ok(());
        // Read bitstream.
        if !flags.intersects(AACDecFlags::CONCEAL | AACDecFlags::FLUSH) {
            decode_state = self.read(
                tp_dec_option.unwrap(),
                extension_data,
                work_buffer_core,
                decoder_config,
                preroll_au_length,
            );

            // Early termination. Parse and validate access unit only since no output
            // buffer is given.
            if time_data.is_none() {
                return self.create_output_info(
                    decode_state,
                    decoder_config,
                    map_descr,
                    extension_data,
                );
            }

            if decode_state == Ok(()) {
                // Decode bitstream.
                self.decode(work_buffer_core, decoder_config);
            }
        } else if flags.contains(AACDecFlags::FLUSH) {
            // Clear scratch buffer used for spectral data.
            work_buffer_core.fill(0.0);
        }

        // Decoder startup phase must be completed.
        if self.init_state != InitState::Complete {
            decode_state = Err(AacDecoderError::Unknown);
        }

        // If there is no valid data to transform into time domain, return.
        if !decode_state.is_output_valid() {
            return self.create_output_info(
                decode_state,
                decoder_config,
                map_descr,
                extension_data,
            );
        }

        if let Some(time_data) = time_data.as_mut() {
            // Render time signal.
            if self
                .render(
                    extension_data,
                    time_data,
                    work_buffer_core,
                    map_descr,
                    decoder_config,
                    flags
                        | if decode_state != Ok(()) {
                            AACDecFlags::CONCEAL
                        } else {
                            AACDecFlags::empty()
                        },
                )
                .is_err()
            {
                decode_state = Err(AacDecoderError::Unknown);
            }
        } else {
            decode_state = Err(AacDecoderError::Unknown);
        }

        // Update decoder output info and return.
        self.create_output_info(decode_state, decoder_config, map_descr, extension_data)
    }

    /// Signals a bit stream interruption to the decoder.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio codec flags.
    pub fn signal_interruption(&mut self, ac_flags: ACFlags) {
        for el in &mut self.channel_elements {
            el.interrupt(ac_flags);
        }
    }

    /// Update parameters if something has changed.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio codec flags.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn param_update(
        &mut self,
        params: &mut Params,
        ac_flags: ACFlags,
    ) -> Result<(), AacDecoderError> {
        if params.conceal.has_changed {
            self.conceal_data
                .params_update(&mut params.conceal, ac_flags);
        }
        Ok(())
    }

    /// Gets the current delay in frames.
    pub fn frame_delay(&self) -> u8 {
        self.conceal_data.delay()
    }

    /// Gets decoder initialization state.
    pub fn init_state(&self) -> InitState {
        self.init_state
    }

    // Processing - internal sub-functions

    /// Processing pt. I: Reads the bitstream data.
    fn read(
        &mut self,
        tp_dec: &mut TransportDec,
        extension_data: &mut ExtensionData,
        scratch_buffer: &mut [f32],
        decoder_config: &mut Config,
        preroll_au_length: Option<u32>,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        let au_start_anchor = tp_dec.bs.valid_bits();

        // Current element type.
        let mut el_type = ChannelElementId::None;
        // Index of channel element being processed. 255 means: Not an audio channel element.
        let mut channel_element_index = 255;

        // Element counter used for bistream syntax using explicit elements list.
        let mut element_count = 0;
        // Channel element counter.
        let mut channel_element_count: usize = 0;
        // Channel counter for channels found in the bitstream
        let mut aac_channels = 0;

        if decoder_config.ac_flags.contains(ACFlags::USAC) {
            decoder_config.ac_flags.remove(ACFlags::INDEP);
            if tp_dec.bs.valid_bits() <= 0 {
                error_status = Err(AacDecoderError::DecodeFrameError);
            } else {
                if tp_dec.bs.read_bit() != 0 {
                    decoder_config.ac_flags.insert(ACFlags::INDEP);
                } else if self.init_state == InitState::Startup {
                    // Decoding cannot start on a frame which is no independent frame.
                    error_status = Err(AacDecoderError::DecodeFrameError);
                }

                // If the last frame was broken and this frame is no independent frame
                // correct decoding is impossible - trigger concealment.
                if !self.conceal_data.was_last_frame_ok()
                    && !decoder_config.ac_flags.contains(ACFlags::INDEP)
                {
                    error_status = Err(AacDecoderError::DecodeFrameError);
                }
            }
        }

        while (error_status == Ok(())) && (el_type != ChannelElementId::End) {
            let mut element_instance_tag = 255;

            if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                el_type = ChannelElementId::from(tp_dec.bs.read(3));
                element_instance_tag = tp_dec.bs.read(4) as u8;
                tp_dec.bs.push(-4);

                if el_type.is_channel_element() {
                    if channel_element_count >= decoder_config.num_channel_elements.into() {
                        error_status = Err(AacDecoderError::ParseError);
                        break;
                    }
                    if self.init_state == InitState::Startup {
                        // Exceptions because of not standard conform encoder implementations.
                        if el_type == ChannelElementId::Sce {
                            match self.channel_elements[channel_element_count].element_type() {
                                ChannelElementId::Cpe => {
                                    // PS streams using CPE instead of SCE when setting up the
                                    // decoder.
                                    if decoder_config.num_channel_elements == 1 {
                                        self.channel_elements[channel_element_count]
                                            .set_element_type(ChannelElementId::Sce);
                                        self.channel_elements[channel_element_count]
                                            .set_num_channels(1);
                                        self.num_core_channels = 1;

                                        decoder_config.element_config[0]
                                            .el_flags
                                            .insert(ChannelFlags::PS_POSSIBLE);
                                        // Element type has changed, reinit SBR if it is enabled.
                                        if decoder_config.ac_flags.contains(ACFlags::SBR_PRESENT) {
                                            extension_data.interrupt(decoder_config, false);
                                        }
                                    }
                                }
                                ChannelElementId::Lfe => {
                                    // Multichannel streams using SCE instead of LFE.
                                    self.channel_elements[channel_element_count]
                                        .set_element_type(ChannelElementId::Sce);
                                    self.channel_elements[channel_element_count]
                                        .remove_flags(ChannelFlags::LFE);
                                }
                                _ => {}
                            }
                        }
                        // Very first audio channel alement instance tag may be necessary to
                        // parse extension data.
                        if el_type.is_channel_element() && channel_element_count == 0 {
                            let instance_tag =
                                self.channel_elements[channel_element_count].element_instance_tag();
                            extension_data.set_first_channel_element_instance_tag(
                                if instance_tag == 255 {
                                    element_instance_tag
                                } else {
                                    instance_tag
                                },
                            );
                        }
                    } // is_startup_phase
                    if self.channel_elements[channel_element_count].element_type() != el_type {
                        error_status = Err(AacDecoderError::ParseError);
                        break;
                    }
                } // is_channel_element
            } else {
                el_type = decoder_config.element_config[element_count].element_type;
            }

            if tp_dec.bs.valid_bits() < 0 {
                error_status = Err(AacDecoderError::DecodeFrameError);
                break;
            }

            if el_type.is_channel_element() {
                let el_channels = self.channel_elements[channel_element_count].num_channels();

                let mut ch_offset = aac_channels;
                channel_element_index = channel_element_count;

                if !decoder_config
                    .ac_flags
                    .intersects(ACFlags::USAC | ACFlags::ER)
                    && (decoder_config.channel_config == 0
                        || self.init_state == InitState::Complete)
                {
                    channel_element_index = self.get_element_index(element_instance_tag, el_type);
                    if channel_element_index == 255 {
                        error_status = Err(AacDecoderError::ParseError);
                        break;
                    }
                    ch_offset = self.get_channel_offset(channel_element_index);
                }

                // Note: `Some(scratch_buffer)` always works here - `None` is for a CCE only.
                error_status = self.channel_elements[channel_element_index].read(
                    &mut self.common_channel_data,
                    tp_dec,
                    Some(
                        &mut scratch_buffer
                            [usize::from(ch_offset) * usize::from(decoder_config.frame_length)..],
                    ),
                    usize::from(el_channels) * usize::from(decoder_config.frame_length),
                    decoder_config.aot,
                    decoder_config.ac_flags,
                );

                if error_status != Ok(()) {
                    break;
                }

                channel_element_count += 1;
                aac_channels += el_channels;
            } else if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
                && (el_type == ChannelElementId::Cce)
            {
                if decoder_config.channel_config == 0 {
                    // Check whether the element belongs to current program.
                    if let Some(pce) = extension_data.pce.as_ref() {
                        if !pce.validate_non_audio_element_instance_tag(
                            element_instance_tag,
                            ChannelElementId::Cce,
                        ) {
                            error_status = Err(AacDecoderError::DecodeFrameError);
                            break;
                        }
                    }
                }

                if let Some(cce) = self.channel_element_cce.as_mut() {
                    // Parse CCE.
                    error_status = cce.read(
                        &mut self.common_channel_data,
                        tp_dec,
                        None,
                        decoder_config.frame_length.into(),
                        decoder_config.aot,
                        decoder_config.ac_flags,
                    );
                } else {
                    error_status = Err(AacDecoderError::DecodeFrameError);
                }

                if error_status != Ok(()) {
                    break;
                }
            } else if el_type == ChannelElementId::End {
                error_status = self.end_raw_data_block(
                    tp_dec,
                    decoder_config,
                    channel_element_count as u8,
                    aac_channels,
                    preroll_au_length,
                    au_start_anchor,
                );
                if error_status != Ok(()) {
                    break;
                }
            }

            if !(el_type.is_mp4_channel_element() || (el_type == ChannelElementId::Cce)) {
                error_status = extension_data.read(
                    tp_dec,
                    decoder_config,
                    el_type,
                    channel_element_index as u8,
                    channel_element_count as u8,
                    element_count as u8,
                    au_start_anchor,
                );
                if error_status != Ok(()) {
                    break;
                }
            }

            if !(el_type).is_channel_element() {
                channel_element_index = 255;
            }
            if decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                element_count += 1;
                if element_count >= decoder_config.num_elements.into() {
                    break;
                }
            }
        } // while ( el_type != ChannelElementId::End ... )

        // Check whether ID_END is successfully completed.
        if el_type != ChannelElementId::End && error_status == Ok(()) {
            error_status = Err(AacDecoderError::ParseError);
        }

        if error_status != Ok(()) {
            // Push the bitbuffer to the end of the raw_data_block().
            let trailing_bits =
                tp_dec.trailing_bits(au_start_anchor, preroll_au_length, decoder_config.ac_flags);
            tp_dec.bs.push(trailing_bits);
        }

        if error_status == Ok(()) && self.init_state == InitState::Startup {
            self.init_state = InitState::Complete;
        }

        error_status
    }

    /// Processing pt. II: Decodes bitstream data.
    fn decode(&mut self, scratch_buffer: &mut [f32], decoder_config: &Config) {
        let mut aac_channels: usize = 0;
        for el in &mut self.channel_elements {
            let el_channels = usize::from(el.num_channels());

            let el_start = aac_channels * usize::from(decoder_config.frame_length);
            let el_len = el_channels * usize::from(decoder_config.frame_length);
            let this_scratch = &mut scratch_buffer[el_start..el_start + el_len];

            el.decode(
                &mut self.common_channel_data,
                this_scratch,
                decoder_config.ac_flags,
            );

            aac_channels += el_channels;
        }
    }

    /// Processing pt. III: Renders decoded bitstream datat to time-domain output.
    fn render(
        &mut self,
        extension_data: &mut ExtensionData,
        time_data: &mut [f32],
        scratch_buffer: &mut [f32],
        map_descr: &mut ChannelMapDescriptor,
        decoder_config: &Config,
        flags: AACDecFlags,
    ) -> Result<(), AacDecoderError> {
        let is_frame_ok: bool = !(flags.contains(AACDecFlags::CONCEAL)
            || (flags.contains(AACDecFlags::FLUSH) && !self.conceal_data.was_last_frame_ok()));

        let mut aac_channels: usize = 0;
        for (channel_element_count, el) in self.channel_elements.iter_mut().enumerate() {
            let el_channels = usize::from(el.num_channels());
            let mapped_channel = usize::from(
                map_descr
                    .get_map_value(aac_channels as u8, decoder_config.channel_map_index.into()),
            );

            let el_start_spec = aac_channels * usize::from(decoder_config.frame_length);
            let el_len_spec = el_channels * usize::from(decoder_config.frame_length);
            let el_start_time = mapped_channel * usize::from(decoder_config.samples_per_frame);
            let el_len_time = el_channels * usize::from(decoder_config.samples_per_frame);
            let this_time_data = &mut time_data[el_start_time..el_start_time + el_len_time];
            let this_spectral_data =
                &mut scratch_buffer[el_start_spec..el_start_spec + el_len_spec];

            let is_pseudo_lr = extension_data.is_pseudo_lr(decoder_config);

            if AacDecoderError::Ok
                != el.render(
                    &mut self.common_channel_data,
                    &mut self.conceal_data,
                    &mut extension_data.drc_data.as_deref_mut(),
                    this_spectral_data,
                    this_time_data,
                    (decoder_config.ac_flags | extension_data.implicit_ac_flags)
                        .contains(ACFlags::SBR_PRESENT),
                    is_pseudo_lr,
                    is_frame_ok,
                    aac_channels,
                    flags.contains(AACDecFlags::FLUSH),
                    decoder_config.ac_flags,
                )
            {
                return Err(AacDecoderError::Unknown);
            }

            if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                let mut window_sequence = [BlockType::Long; MAX_EL_CHANNELS];

                for (ch, ch_el_info) in (el.get_element_channel_infos()).iter().enumerate() {
                    window_sequence[ch] = ch_el_info.ics_info.window_sequence();
                }

                if extension_data
                    .feed_drc_element(
                        decoder_config,
                        &window_sequence[..el_channels],
                        aac_channels,
                        channel_element_count,
                    )
                    .is_err()
                {
                    return Err(AacDecoderError::Unknown);
                }
            }

            aac_channels += el_channels;
        }

        Ok(())
    }

    // Internal helper functions

    /// Take over flags from decoder config.
    fn accept_flags(&mut self, decoder_config: &Config) {
        let mut channel_element_count = 0;
        for el_cfg in decoder_config
            .element_config
            .iter()
            .take(decoder_config.num_elements.into())
        {
            if el_cfg.element_type.is_channel_element() {
                self.channel_elements[channel_element_count].clear_flags();
                self.channel_elements[channel_element_count].insert_flags(el_cfg.el_flags);
                channel_element_count += 1;
            }
        }
    }

    /// States whether all channel elements have been read.
    fn all_channel_elements_read(&self) -> bool {
        for el in &self.channel_elements {
            if !el.state_flags().contains(ElState::READ) {
                return false;
            }
        }
        true
    }

    /// Deinitialize again and return with an error if something went wrong during initialization.
    fn bail_from_init(&mut self) -> Result<(), AacDecoderError> {
        self.deinit();
        Err(AacDecoderError::OutOfMemory)
    }

    /// Creates output info structure - to be provided via the AAC API.
    pub(crate) fn create_output_info(
        &self,
        ret_val: Result<(), AacDecoderError>,
        decoder_config: &Config,
        map_descr: &ChannelMapDescriptor,
        extension_data: &ExtensionData,
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        let mut output_info = OutputInfo::default();

        if ret_val.is_output_valid() {
            output_info.sampling_rate = decoder_config.sampling_frequency;
            output_info.frame_size = decoder_config.samples_per_frame;
            output_info.num_channels = self.num_core_channels;
            output_info.output_delay =
                self.frame_delay() as u32 * decoder_config.samples_per_frame as u32;

            for c in 0..output_info.num_channels {
                let mapped_channel = usize::from(
                    map_descr.get_map_value(c, decoder_config.channel_map_index as usize),
                );
                output_info.channel_type[mapped_channel] = decoder_config.channel_type[c as usize];
                output_info.channel_indices[mapped_channel] =
                    decoder_config.channel_indices[c as usize];
            }

            output_info.output_loudness = if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                extension_data
                    .drc_data
                    .as_ref()
                    .unwrap()
                    .output_loudness()
                    .into()
            } else {
                -1
            };
        }

        if let Err(e) = ret_val {
            Err((e, output_info))
        } else {
            Ok(output_info)
        }
    }

    /// Sanity checks and alignment when having finished reading a raw data block.
    fn end_raw_data_block(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
        channel_element_count: u8,
        aac_channels: u8,
        preroll_au_length: Option<u32>,
        au_start_anchor: isize,
    ) -> Result<(), AacDecoderError> {
        // Check if whole number of channels and elements have been read.
        if decoder_config.num_channel_elements != channel_element_count
            || self.num_core_channels != aac_channels
        {
            return Err(AacDecoderError::ParseError);
        }

        // Check if all channel elements have been read.
        if !self.all_channel_elements_read() {
            return Err(AacDecoderError::ParseError);
        }

        // Byte alignment with respect to the first bit of the `raw_data_block()`.
        if !decoder_config.ac_flags.contains(ACFlags::USAC) {
            tp_dec.bs.align(au_start_anchor);
        }

        // Check if all bits of the `raw_data_block()` have been read.
        let trailing_bits =
            tp_dec.trailing_bits(au_start_anchor, preroll_au_length, decoder_config.ac_flags);

        if (tp_dec.remaining_au_bits() < trailing_bits)
            || (if decoder_config.ac_flags.contains(ACFlags::USAC) {
                (trailing_bits & !0x7) != 0
            } else {
                trailing_bits != 0
            })
        {
            return Err(AacDecoderError::ParseError);
        }

        // Put the bitbuffer at the end of the `raw_data_block()`.
        tp_dec.bs.push(trailing_bits);

        tp_dec.crc_check().map_err(|_| AacDecoderError::CrcError)?;

        Ok(())
    }

    /// Gets the element index for a given bitstream elemement.
    fn get_element_index(&self, tag: u8, id: ChannelElementId) -> usize {
        for (cnt, el) in self.channel_elements.iter().enumerate() {
            if tag == el.element_instance_tag() && id == el.element_type() {
                return cnt;
            }
        }
        255
    }

    /// Gets the channel offset for a given bitstream elemement.
    fn get_channel_offset(&self, el_index: usize) -> u8 {
        let mut ch_offset = 0;
        for (cnt, el) in self.channel_elements.iter().enumerate() {
            if cnt == el_index {
                break;
            };
            ch_offset += el.num_channels();
        }
        ch_offset
    }
}
