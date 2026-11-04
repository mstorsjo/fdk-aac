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
//! Advanced audio coding (AAC) decoder

use crate::{
    aac_dec::{
        config::Config,
        constants::MAX_FRAMESIZE,
        error_codes::AacDecoderError,
        ext_data::ExtensionData,
        interleaver,
        ipf::IpfData,
        output_info::{MetadataInfo, OutputInfo, StreamInfo},
        params::Params,
        process::{InitState, Process},
    },
    common::{
        channel_map_descr::ChannelMapDescriptor,
        channel_order::ChannelOrder,
        flags::{AACDecFlags, ACFlags, ChannelFlags},
    },
    td_limiter::{
        PcmLimiter, TDLIMIT_ATTACK_DEFAULT_MS, TDLIMIT_MAX_SAMPLERATE,
        TD_PEAK_LIMIT_ATTACK_DEFAULT_MS, TD_PEAK_LIMIT_THRESHOLD_DEFAULT,
    },
    tp_dec::TransportDec,
};
use itertools::izip;
use std::{
    cell::RefCell,
    ops::{Deref, DerefMut},
    rc::Rc,
};

/// AAC decoder.
#[repr(C)]
#[derive(Debug)]
pub struct AacDecoder {
    /// Configuration for the decoder.
    pub(super) config: Config,
    /// Describes the output channel mapping.
    pub(super) map_descr: ChannelMapDescriptor,
    /// AAC decoder instance.
    pub(super) aac_core: Process,
    /// Extension data.
    pub(super) extension_data: ExtensionData,
    /// Handle of time domain limiter.
    limiter: Option<Box<PcmLimiter>>,
    /// Maximum number of limiter channels.
    max_limiter_channels: u8,
    /// Work buffer for core processing.
    pub(super) work_buffer_core: Vec<f32>,

    /// Work buffer for complete output.
    work_buffer_output: Vec<f32>,
}

impl Default for AacDecoder {
    fn default() -> Self {
        Self {
            config: Config::default(),
            map_descr: ChannelMapDescriptor::new(),
            aac_core: Process::new(),
            extension_data: ExtensionData::new(),
            limiter: None,
            max_limiter_channels: 0,
            work_buffer_core: Default::default(),
            work_buffer_output: Default::default(),
        }
    }
}

impl AacDecoder {
    /// Creates a new `AacDecoder` instance.
    pub fn new() -> AacDecoder {
        let mut aac_dec = Self::default();

        aac_dec.map_descr.init(None, ChannelOrder::Mpeg);

        aac_dec
    }

    /// De-initializes the internal memory and sub-components.
    pub(super) fn close(&mut self) {
        self.limiter = None;

        if !self.work_buffer_output.is_empty() {
            self.work_buffer_output.clear();
            self.work_buffer_output.shrink_to_fit();
        }

        if !self.work_buffer_core.is_empty() {
            self.work_buffer_core.clear();
            self.work_buffer_core.shrink_to_fit();
        }

        self.aac_core.deinit();
        self.extension_data.destroy();
    }

    /// Returns whether `IPF` is possible.
    pub(super) fn is_ipf_possible(&self) -> bool {
        self.config.ac_flags.contains(ACFlags::USAC)
    }

    /// Applies dynamic range limiter on the given input samples. The numbers of the output audio
    /// channels and the output sample rate is extracted from `output_info`. The delay introduced
    /// by the limiter is then added to `output_info`.
    ///
    /// # Parameters
    ///
    /// - `time_data`: The time domain audio samples to process.
    /// - `output_info`: Information about the output format.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`
    pub(super) fn apply_limiter(
        &mut self,
        time_data: &mut [f32],
        output_info: &mut OutputInfo,
    ) -> Result<(), AacDecoderError> {
        if let Some(pcm_limiter) = &mut self.limiter {
            // Set actual signal parameters.
            pcm_limiter.set_n_channels(usize::from(output_info.num_channels))?;
            pcm_limiter.set_sample_rate(output_info.sampling_rate)?;
            // Apply pcm limiter on time data.
            let buff_len =
                usize::from(output_info.frame_size) * usize::from(output_info.num_channels);
            pcm_limiter.apply(
                &mut time_data[..buff_len],
                &mut self.work_buffer_output[..buff_len],
            )?;
            // Announce the additional limiter output delay
            output_info.output_delay += pcm_limiter.get_delay();
        };
        Ok(())
    }

    /// Creates output information based on the current decoder state and the given return value.
    ///
    /// # Parameters
    ///
    /// - `aac_decoder`: AAC decoder instance (`AacDecoder`).
    /// - `ret_val`: Return value of the last decoding operation.
    ///
    /// # Return
    ///
    /// - `Result<OutputInfo, (AacDecoderError, OutputInfo)>`
    fn create_output_info(
        aac_decoder: &AacDecoder,
        ret_val: Result<(), AacDecoderError>,
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        aac_decoder.aac_core.create_output_info(
            ret_val,
            &aac_decoder.config,
            &aac_decoder.map_descr,
            &aac_decoder.extension_data,
        )
    }

    /// Handles signal interruption.
    ///
    /// # Parameters
    ///
    /// - `is_explicit_interruption`: `true`, when reset of DRC and downmix metadata needed, `false`
    ///   otherwise.
    pub(super) fn signal_interruption(&mut self, is_explicit_interruption: bool) {
        if self.aac_core.init_state() != InitState::None {
            self.aac_core.signal_interruption(self.config.ac_flags);
            self.extension_data
                .interrupt(&self.config, is_explicit_interruption);
        }
    }

    /// Processes one frame of AAC data.
    ///
    /// # Parameters
    ///
    /// - `aac_decoder`: AAC decoder instance (`AacDecoder`).
    /// - `ipf_data_option`: Optional IPF data.
    /// - `tp_dec_option`: Optional transport decoder.
    /// - `time_data`: Time domain audio samples (output buffer).
    /// - `params`: Decoder parameters.
    /// - `flags`: AAC Decoder flags.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`
    pub(super) fn process(
        aac_decoder: Rc<RefCell<AacDecoder>>,
        ipf_data_option: &mut Option<&mut IpfData>,
        tp_dec_option: &mut Option<&mut TransportDec>,
        time_data: &mut [f32],
        params: &mut Params,
        flags: AACDecFlags,
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        let mut error_status = AacDecoderError::Ok;
        let mut output_info;

        let mut is_end_access_unit = false;

        let mut init_state;

        // Concealment and flushing are not possible at the same time.
        if flags.contains(AACDecFlags::CONCEAL | AACDecFlags::FLUSH) {
            return Self::create_output_info(
                aac_decoder.borrow().deref(),
                Err(AacDecoderError::Unknown),
            );
        }

        {
            let mut refcell_aac_dec = aac_decoder.borrow_mut();
            let aac_decoder = refcell_aac_dec.deref_mut();

            init_state = aac_decoder.aac_core.init_state();

            // Decoder must at least be initialized by means of asc.
            if init_state == InitState::None {
                return Self::create_output_info(aac_decoder, Err(AacDecoderError::Unknown));
            }

            if let Some(ipf) = ipf_data_option {
                ipf.reset(
                    aac_decoder
                        .config
                        .ac_flags
                        .contains(ACFlags::USAC_HAS_PREROLL),
                );
            }
        }

        // Process preroll frames and current frame.
        'preroll_frames: loop {
            // Flags in current frame.
            let mut flags_cf = flags;

            if let Some(ipf) = ipf_data_option {
                if let Some(tp_dec) = tp_dec_option.as_deref_mut() {
                    error_status =
                        ipf.preroll_extension_payload(tp_dec, init_state != InitState::Complete);
                }

                if error_status == AacDecoderError::Ok {
                    if ipf.is_flush_on() {
                        flags_cf.insert(AACDecFlags::FLUSH);
                    }
                } else {
                    // Return with an error if ipf reconfiguration failed.
                    if init_state == InitState::None {
                        error_status = AacDecoderError::Unknown;
                    }
                    if !error_status.is_decode_error() {
                        let mut refcell_aac_dec = aac_decoder.borrow_mut();
                        let aac_decoder = refcell_aac_dec.deref_mut();
                        return Self::create_output_info(aac_decoder, Err(error_status));
                    }
                    flags_cf.insert(AACDecFlags::CONCEAL);
                }
            }

            {
                let mut refcell_aac_dec = aac_decoder.borrow_mut();
                let aac_decoder = refcell_aac_dec.deref_mut();

                // Signal bitstream discontinuity in case of flush request
                if flags_cf.contains(AACDecFlags::FLUSH) {
                    aac_decoder.signal_interruption(false);
                }

                // Check and apply parameter changes.
                if let Err(e) = aac_decoder.params_update(params) {
                    return Self::create_output_info(aac_decoder, Err(e));
                }

                aac_decoder.extension_data.reset();
                aac_decoder.aac_core.reset(aac_decoder.config.ac_flags);

                let preroll_au_length = ipf_data_option.as_ref().and_then(|ipf| {
                    let length = ipf.preroll_au_length();
                    (length > 0).then_some(length)
                });

                // Process AAC-core (parse access unit and extensions).
                match aac_decoder.aac_core.decode_frame(
                    tp_dec_option.as_deref_mut(),
                    &mut aac_decoder.extension_data,
                    &mut aac_decoder.work_buffer_core,
                    &mut aac_decoder.map_descr,
                    flags_cf,
                    &mut Some(&mut aac_decoder.work_buffer_output),
                    &mut aac_decoder.config,
                    preroll_au_length,
                ) {
                    Ok(info) => {
                        output_info = info;
                    }
                    Err((e, e_info)) => {
                        output_info = e_info;
                        error_status = e;
                        if !e.is_output_valid() {
                            return Self::create_output_info(aac_decoder, Err(e));
                        }
                        flags_cf.insert(AACDecFlags::CONCEAL);
                    }
                }

                let is_ipf_end_au = tp_dec_option.is_none()
                    || ipf_data_option
                        .as_deref()
                        .map_or(true, |ipf| ipf.is_end_access_unit());

                if is_ipf_end_au || (error_status != AacDecoderError::Ok) {
                    if let Some(tp_dec) = tp_dec_option.as_deref_mut() {
                        if tp_dec.end_access_unit().is_err() {
                            flags_cf.insert(AACDecFlags::CONCEAL);
                            error_status = AacDecoderError::DecodeFrameError;
                        }
                    }
                    is_end_access_unit = true;
                }

                // Process extensions if present (SBR, MPS, and MPEG-D DRC).
                if let Err(e) = aac_decoder.extension_data.apply(
                    &mut aac_decoder.work_buffer_output,
                    &mut aac_decoder.work_buffer_core,
                    &mut output_info,
                    &aac_decoder.map_descr,
                    &aac_decoder.config,
                    params.max_target_channels(&aac_decoder.config) == 1,
                    flags_cf,
                ) {
                    if !e.is_output_valid() {
                        return Self::create_output_info(aac_decoder, Err(e));
                    }
                    flags_cf.insert(AACDecFlags::CONCEAL);
                }

                // Process IPF config change and crossfade if present.
                if let Some(ipf) = ipf_data_option.as_deref_mut() {
                    ipf.process(
                        &mut aac_decoder.work_buffer_output,
                        usize::from(output_info.num_channels),
                        usize::from(output_info.frame_size),
                        !flags_cf.contains(AACDecFlags::CONCEAL),
                    );
                }

                // Values to be used in next iteration in loop.
                init_state = aac_decoder.aac_core.init_state();
            }

            if is_end_access_unit {
                break 'preroll_frames;
            }
        }

        {
            let mut refcell_aac_dec = aac_decoder.borrow_mut();
            let aac_decoder = refcell_aac_dec.deref_mut();

            // Apply PCM downmix.
            if let Err(e) = aac_decoder.extension_data.apply_pcm_downmix(
                &mut aac_decoder.work_buffer_output,
                time_data,
                &mut output_info,
                &mut aac_decoder.map_descr,
            ) {
                if !e.is_output_valid() {
                    return Self::create_output_info(aac_decoder, Err(e));
                }
            }

            // Check whether time data buffer is large enough.
            let buff_len =
                usize::from(output_info.num_channels) * usize::from(output_info.frame_size);
            if time_data.len() < buff_len {
                return Self::create_output_info(
                    aac_decoder,
                    Err(AacDecoderError::OutputBufferTooSmall),
                );
            }

            // Interleave time data and adjust its scaling.
            let _ = interleaver::interleave_time_data(
                &mut time_data[..buff_len],
                &aac_decoder.work_buffer_output[..buff_len],
                usize::from(output_info.num_channels),
                1.0_f32 / (1 << 15) as f32,
            );

            // Apply limiter on time data.
            if params.is_limiter_active(aac_decoder.config.ac_flags) {
                if let Err(e) = aac_decoder.apply_limiter(time_data, &mut output_info) {
                    if !e.is_output_valid() {
                        return Self::create_output_info(aac_decoder, Err(e));
                    }
                }
            }
        }

        if error_status == AacDecoderError::Ok {
            Ok(output_info)
        } else {
            Err((error_status, output_info))
        }
    }

    /// Checks and applies parameter changes, if necessary. This function might re-allocate heap
    /// memory.
    ///
    /// # Parameters
    ///
    /// - `params`: Decoder parameters.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`
    pub(super) fn params_update(&mut self, params: &mut Params) -> Result<(), AacDecoderError> {
        // In case of a config change restore all parameters.
        if self.aac_core.init_state() == InitState::Startup {
            params.restore();
        }

        self.aac_core.param_update(params, self.config.ac_flags)?;
        params.adjust_params(&self.config, self.aac_core.frame_delay())?;
        self.extension_data.update_params(&self.config, params)?;

        // Reallocate target channels dependent components if necessary.
        self.reinit(params)?;

        if params.is_limiter_active(self.config.ac_flags) && params.limiter.has_changed() {
            if let Some(pcm_limiter) = &mut self.limiter {
                if params.limiter.attack_time() > 0 {
                    pcm_limiter.set_attack_ms(params.limiter.attack_time())?;
                } else if self.config.ac_flags.contains(ACFlags::USAC) {
                    // Default attack time in ms for peak limiter.
                    pcm_limiter.set_attack_ms(TD_PEAK_LIMIT_ATTACK_DEFAULT_MS)?;
                }

                if params.limiter.release_time() > 0 {
                    pcm_limiter.set_release_ms(params.limiter.release_time())?;
                }

                if self.config.ac_flags.contains(ACFlags::USAC) {
                    // Default threshold (-1 dBFS) for peak limiter.
                    pcm_limiter.set_threshold(TD_PEAK_LIMIT_THRESHOLD_DEFAULT)?;
                }
            }
            params.limiter.do_change(false);
        }

        if !self.map_descr.check_ch_map_order(params.channel_map_order) {
            self.map_descr.init(None, params.channel_map_order);
        }
        if !self.map_descr.is_valid() {
            return Err(AacDecoderError::UnsupportedChannelconfig);
        }

        Ok(())
    }

    /// Re-initializes the output data workbuffer and the PCM limiter, if necessary. This function
    /// might re-allocate heap memory.
    ///
    /// # Parameters
    ///
    /// - `params`: Decoder parameters.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`
    pub(super) fn reinit(&mut self, params: &mut Params) -> Result<(), AacDecoderError> {
        let max_output_frame_length = if self.config.ac_flags.contains(ACFlags::USAC)
            || params.is_mpeg4_esbr_active(&self.config)
        {
            4 * MAX_FRAMESIZE
        } else if self.config.ext_sampling_frequency == 0 {
            usize::from(self.config.samples_per_frame * 2)
        } else {
            usize::from(self.config.ext_samples_per_frame)
        };

        let target_channels = usize::from(params.max_target_channels(&self.config));
        let tmp_ch = if !self.config.ac_flags.intersects(ACFlags::USAC | ACFlags::ER)
            || self
                .config
                .ac_flags
                .intersects(ACFlags::MPS_PRESENT | ACFlags::PS_PRESENT)
        {
            2
        } else {
            0
        };
        let max_output_channels =
            target_channels.max(usize::from(self.config.num_channels.max(tmp_ch)));

        // Reallocate output data workbuffer if necessary.
        if self.work_buffer_output.len() < (max_output_frame_length * max_output_channels) {
            self.work_buffer_output = vec![0_f32; max_output_frame_length * max_output_channels];
        }

        // Reallocate limiter if necessary.
        if usize::from(self.max_limiter_channels) < target_channels
            && params.is_limiter_active(self.config.ac_flags)
        {
            if let Some(pcm_limiter) = &self.limiter {
                let _ = pcm_limiter;
                self.limiter = None;
            }

            match PcmLimiter::new(
                TDLIMIT_ATTACK_DEFAULT_MS,
                target_channels,
                TDLIMIT_MAX_SAMPLERATE,
            ) {
                Ok(limiter) => {
                    self.limiter = Some(Box::new(limiter));
                    params.limiter.do_change(true);
                    self.max_limiter_channels = u8::try_from(target_channels).unwrap();
                }
                Err(_) => return Err(AacDecoderError::OutOfMemory),
            }
        }

        Ok(())
    }

    /// Get metadata information, if available.
    ///
    /// # Return
    ///
    /// - `MetadataInfo`: Metadata information if available, otherwise `None`.
    pub(super) fn metadata_info(&mut self) -> Option<MetadataInfo> {
        if self.aac_core.init_state() == InitState::Complete {
            Some(self.extension_data.get_metadata_info(&self.config))
        } else {
            None
        }
    }

    /// Returns `StreamInfo` which gives information about the currently decoded audio data.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: `TransportDec` instance with valid data.
    /// - `bs_anchor`: Bitstream anchor value at start of AAC decoding.
    /// - `is_frame_ok`: Flag indicates whether current frame is Ok (or) defective.
    ///
    /// # Return
    ///
    /// - `Option<StreamInfo>`
    pub(super) fn stream_info(
        &mut self,
        tp_dec: &mut TransportDec,
        bs_anchor: isize,
        is_frame_ok: bool,
    ) -> Option<StreamInfo> {
        // Update `StreamInfo`.
        if self.aac_core.init_state() == InitState::Complete {
            // Create `StreamInfo` with default values.
            let mut si = StreamInfo::new();

            si.aot = self.config.aot;
            si.ext_aot = self.config.aot;
            si.channel_config = i32::from(self.config.channel_config);
            si.ext_sampling_rate = self.config.ext_sampling_frequency;
            si.sample_rate = self.config.sampling_frequency;
            si.samples_per_frame = self.config.samples_per_frame;
            si.flags = self.config.ac_flags | self.extension_data.implicit_ac_flags;

            // Adapt independent frame flag.
            if !is_frame_ok {
                si.flags.remove(ACFlags::INDEP);
            } else if !self.config.ac_flags.contains(ACFlags::USAC) {
                si.flags.insert(ACFlags::INDEP);
            }

            if !self.config.ac_flags.contains(ACFlags::USAC) {
                for (si_el, ele_config) in izip!(
                    si.bs_element_list.iter_mut(),
                    self.config.element_config.iter(),
                )
                .take(usize::from(self.config.num_elements))
                {
                    *si_el = ele_config.element_type;
                }
                si.num_elements = self.config.num_elements;
            }

            si.num_channels = if self.config.element_config[0]
                .el_flags
                .contains(ChannelFlags::PS_POSSIBLE)
            {
                1
            } else {
                self.config.num_channels
            };

            si.ch_order = self.map_descr.ch_map_order;
            si.num_consumed_bytes =
                u32::try_from(bs_anchor - tp_dec.bs_mut().valid_bits()).unwrap() / 8;

            Some(si)
        } else {
            None
        }
    }
}
