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
//! Spectral Band Replication API
//!
//! This module provides a frontend to the SBR decoder. The function `SbrDecoder::new()` is used
//! for initialization. The function `SbrDecoder::apply()` is called for each frame.
//! `SbrDecoder::apply()` will call the required functions to decode the raw SBR data, to decode
//! the envelope data and noise floor levels, and to finally apply SBR to the current frame.

use super::{
    common::SbrError,
    constants::{
        SBRDEC_MAX_ELEMENTS, SBRDEC_MAX_EL_CHANNELS, SBR_DECODER_CRC_LEN, SBR_DECODER_CRC_POLY,
        SBR_DECODER_CRC_START_VALUE, SBR_DECODER_OUTPUT_DELAY_ELD, SBR_DECODER_OUTPUT_DELAY_ELD_DS,
        SBR_DECODER_OUTPUT_DELAY_ELD_MPS, SBR_DECODER_OUTPUT_DELAY_HE_AAC,
        SBR_DECODER_OUTPUT_DELAY_HE_AAC_DS, SBR_DECODER_OUTPUT_DELAY_HE_AAC_SKIP_QMF_SYN,
    },
    element::{SbrDecoderElement, SbrDecoderParams},
    flags::SbrDecFlags,
    frame_data::FrameData,
    header_data::{Status, SyncState},
    huff_dec::CouplingMode,
    params::SbrDecParam,
};
use crate::{
    common::{
        aot::AudioObjectType,
        bitstream::Bitstream,
        bs_element_id::ChannelElementId,
        channel_map_descr::ChannelMapDescriptor,
        crc::CrcInfo,
        enums::AudioChannel,
        flags::{ACFlags, ChannelFlags},
        qmf::QmfMode,
        qmf_domain::{QmfDomain, QmfDomainParams},
    },
    sbr_dec::constants::SBRDEC_MAX_DELAY_FRAMES,
    tp_dec::ReconfigState,
};
use std::cmp::Ordering;

#[derive(Debug, Default)]
pub struct SbrDecoder {
    elements: [Option<Box<SbrDecoderElement>>; SBRDEC_MAX_ELEMENTS],
    pub params: SbrDecoderParams,
}

impl SbrDecoder {
    /// Applies SBR to the given time data. SBR-processing works in-place.
    /// I.e. the calling function has to provide a time domain buffer `time_data`, which can hold
    /// the completely decoded result.
    /// Left and right channel are read and stored according to the interleaving flag, frame length
    /// and number of channels.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`:          QMF domain data
    /// - `input`:               Input data
    /// - `time_data`:           Upsampled output data (Time domain)
    /// - `frame_size`:          Frame size
    /// - `num_core_channels`:   Number of channels in the time data buffer
    /// - `sample_rate`:         Output sample rate
    /// - `map_descr`:           Channel mapping descriptor
    /// - `map_idx`:             Channel map index
    /// - `core_decoded_ok`:     Flag, indicating if the core decoder did not find any error.
    /// - `is_ps_decoded`:       Input: PS is possible, Output: PS has been rendered.
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    #[expect(clippy::too_many_arguments)]
    pub fn apply(
        &mut self,
        qmf_domain: &mut QmfDomain,
        input: Option<&[f32]>,
        time_data: &mut Option<&mut [f32]>,
        frame_size: &mut u16,
        num_core_channels: &mut u8,
        sample_rate: &mut u32,
        map_descr: &ChannelMapDescriptor,
        map_idx: usize,
        core_decoded_ok: u8,
        is_ps_decoded: &mut bool,
    ) -> Result<(), SbrError> {
        let mut num_sbr_channels = 0;

        if (input.is_none() && !self.params.flags().contains(SbrDecFlags::SKIP_QMF_ANA))
            || (time_data.is_none() && !self.params.flags().contains(SbrDecFlags::SKIP_QMF_SYN))
        {
            return Err(SbrError::InvalidArgument);
        }

        if *num_core_channels == 0 || !map_descr.is_valid() {
            return Err(SbrError::InvalidArgument);
        }
        if self.params.num_sbr_elements() == 0 {
            // Exit immediately to avoid access violations.
            return Err(SbrError::NotInitialized);
        }

        // Sanity check of allocated SBR elements.
        for el in 0..self.params.num_sbr_elements() {
            if self.elements[el as usize].is_none() {
                return Err(SbrError::NotInitialized);
            }
        }

        let mut is_ps_possible = *is_ps_decoded;

        match &self.elements[0] {
            Some(sbr_elem) => {
                if self.params.num_sbr_elements() != 1
                    || sbr_elem.element_id() != ChannelElementId::Sce
                {
                    is_ps_possible = false;
                }
            }
            None => {
                return Err(SbrError::NotInitialized);
            }
        }

        // Make sure that even if no SBR data was found/parsed, *is_ps_decoded is returned `true`,
        // if `is_ps_possible` was `false`.
        if !is_ps_possible {
            self.params.remove_flags(SbrDecFlags::PS_DECODED);
        }

        if self.params.num_sbr_channels() > qmf_domain.num_input_channels() {
            return Err(SbrError::UnsupportedConfig);
        }

        let num_flushed_frames = if self.params.flags().contains(SbrDecFlags::FLUSH) {
            // Flushing is signalized, hence increment the flush frame counter.
            self.params.flushed_frames() + 1
        } else {
            // No flushing is signalized, hence reset the flush frame counter.
            0
        };
        self.params.set_flushed_frames_num(num_flushed_frames);

        // Loop over SBR elements.
        for el in 0..usize::from(self.params.num_sbr_elements()) {
            match &mut self.elements[el] {
                Some(sbr_elem) => {
                    let mut num_elem_chan = if sbr_elem.element_id() == ChannelElementId::Cpe {
                        2_u8
                    } else {
                        1_u8
                    };

                    if is_ps_possible
                        && sbr_elem
                            .sbr_channel(usize::from(AudioChannel::Right))
                            .is_none()
                    {
                        // Disable PS and try decoding SBR mono.
                        is_ps_possible = false;
                    }

                    // If core signal is bad, then force upsampling.
                    if core_decoded_ok == 0 {
                        sbr_elem.set_frame_error_flags();
                    }

                    sbr_elem.decode(
                        &mut self.params,
                        qmf_domain,
                        input,
                        time_data.as_deref_mut(),
                        map_descr,
                        map_idx,
                        num_sbr_channels,
                        *num_core_channels,
                        &mut num_elem_chan,
                        is_ps_possible,
                    )?;

                    num_sbr_channels += num_elem_chan;

                    if num_sbr_channels >= *num_core_channels {
                        break;
                    }
                }
                None => {
                    return Err(SbrError::NotInitialized);
                }
            }
        }

        // Update *numCoreChannels and samplerate
        // Do not mess with output channels in case of USAC:
        // `num_sbr_channels !=  *num_core_channels`` for `stereo_config_index` == 2.
        if !self.params.flags().contains(SbrDecFlags::SYNTAX_USAC) {
            *num_core_channels = num_sbr_channels;
        }
        *frame_size = u16::from(qmf_domain.num_qmf_time_slots()) * qmf_domain.num_bands_synthesis();
        *sample_rate = self.params.sample_rate_out();
        *is_ps_decoded = self.params.flags().contains(SbrDecFlags::PS_DECODED);

        // Clear flush flag because everything seems to be done successfully.
        self.params.remove_flags(SbrDecFlags::FLUSH);

        Ok(())
    }

    /// Assigns QMF domain provided QMF channels to SBR channels.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain_params_requested`: QMF domain requested data.
    fn assign_qmf_channels_2_sbr_channels(
        &mut self,
        qmf_domain_params_requested: &mut QmfDomainParams,
    ) {
        let mut abs_ch_offset = 0;

        for el in 0..self.params.num_sbr_elements() as usize {
            match &mut self.elements[el] {
                None => continue, // No SBR element allocated, nothing to do.
                Some(sbr_elem) => {
                    for (ch, sbr_ch) in sbr_elem.sbr_channel_mut().iter_mut().enumerate() {
                        sbr_ch
                            .sbr_dec
                            .set_ch_qmf_domain(u8::try_from(abs_ch_offset + ch).unwrap());
                        qmf_domain_params_requested.work_buffer_index[abs_ch_offset + ch] =
                            u8::try_from(ch).unwrap();
                    }
                    abs_ch_offset += usize::from(sbr_elem.element_channels());
                }
            }
        }
    }

    /// Cleanup before return.
    fn bail(
        sbr_elem: &mut SbrDecoderElement,
        error_status: SbrError,
        header_status: Status,
        this_hdr_slot: usize,
        last_hdr_slot: usize,
        num_delay_frames: usize,
    ) -> Result<(), SbrError> {
        if error_status == SbrError::NotInitialized {
            // If the SBR element is not initialized, then return the error.
            return Err(SbrError::NotInitialized);
        }

        let mut use_old_hdr = header_status == Status::NotPresent
            || header_status == Status::Error
            || (header_status == Status::Reset && error_status == SbrError::ParseError);

        if !use_old_hdr && (this_hdr_slot != last_hdr_slot) {
            use_old_hdr |= !sbr_elem.compare_sbr_header(last_hdr_slot);
        }

        sbr_elem.update_header_and_frame_slot(
            use_old_hdr,
            this_hdr_slot,
            last_hdr_slot,
            num_delay_frames,
        );

        if error_status == SbrError::Ok {
            Ok(())
        } else {
            Err(error_status)
        }
    }

    /// Passes out of band SBR header to SBR decoder.
    ///
    /// # Parameters
    ///
    ///  - `bs`:                          Bitstream data to read from
    ///  - `qmf_domain`:                  QMF domain data
    ///  - `qmf_domain_params_requested`: QMF domain requested data
    ///  - `sample_rate_in`:              Input samplerate of the SBR decoder
    ///  - `sample_rate_out`:             Output samplerate of the SBR decoder
    ///  - `samples_per_frame`:           Core codec frame size
    ///  - `core_codec`:                  Audio Object Type (AOT) of the core codec
    ///  - `element_id`:                  MPEG-4 SBR element ID
    ///  - `elemend_index`:               Index of the SBR element
    ///  - `harmonic_sbr`:                Harmonic SBR status
    ///  - `config_mode`:                 Config Mode
    ///  - `is_config_changed`:           Did the config changed?
    ///  - `downscale_factor`:            ELD downscale factor
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    #[expect(clippy::too_many_arguments)]
    pub fn decode_header(
        &mut self,
        bs: &mut Bitstream,
        qmf_domain: &mut QmfDomain,
        qmf_domain_params_requested: &mut QmfDomainParams,
        sample_rate_in: u32,
        sample_rate_out: u32,
        samples_per_frame: u16,
        core_codec: AudioObjectType,
        element_id: ChannelElementId,
        element_index: usize,
        harmonic_sbr: u8,
        config_mode: ReconfigState,
        is_config_changed: &mut bool,
        downscale_factor: u8,
    ) -> Result<(), SbrError> {
        let mut sbr_error = SbrError::Ok;

        // Flags should not be changed in SbrDecFlags::DetCfgChange - mode after parsing.
        let flags_saved = if config_mode == ReconfigState::DetCfgChange {
            self.params.flags()
        } else {
            SbrDecFlags::empty()
        };

        let mut params_requested = SbrDecoderParams::default();

        params_requested.set_requested_element_params(
            sample_rate_in,
            sample_rate_out,
            samples_per_frame,
            core_codec,
            harmonic_sbr,
            downscale_factor,
            false,
            element_id,
            element_index,
        );

        if params_requested.element_index() >= SBRDEC_MAX_ELEMENTS {
            return Err(SbrError::UnsupportedConfig);
        }

        let is_new_elem = match self.elements[params_requested.element_index()] {
            Some(_) => {
                // The element already exists.
                false
            }
            None => {
                self.elements[params_requested.element_index()] =
                    Some(Box::new(SbrDecoderElement::new()));
                true
            }
        };

        match self.elements[params_requested.element_index()]
            .as_mut()
            .unwrap()
            .init(
                &mut self.params,
                &params_requested,
                qmf_domain,
                qmf_domain_params_requested,
                is_config_changed,
                config_mode,
                is_new_elem,
            ) {
            Ok(keep) => {
                if keep {
                    // Keep the SBR element if:
                    // - is a new SBR element in the list and it successful init().
                    // - is an existing SBR element in the list, but returned early from init().
                    // - is an existing element in the list and it successful init().

                    // Make sure each SBR channel has one QMF channel assigned even if
                    // `num_sbr_channels` or element set-up has changed.
                    self.assign_qmf_channels_2_sbr_channels(qmf_domain_params_requested);
                } else {
                    // Don't keep the SBR element.
                    let _ = self.elements[params_requested.element_index()]
                        .as_mut()
                        .unwrap()
                        .destroy(&mut self.params.num_sbr_channels);
                    self.elements[params_requested.element_index()] = None;
                }
            }
            Err((err, free_mem)) => {
                if free_mem {
                    // Don't keep the SBR element if:
                    // - is a newly created SBR element, but init() detects that the allocation of
                    //   its internals failed.
                    // - is an existing SBR element, but init() detects that the allocation of its
                    //   internals failed.
                    // The error return must be evaluated, nevertheless.
                    let _ = self.elements[params_requested.element_index()]
                        .as_mut()
                        .unwrap()
                        .destroy(&mut self.params.num_sbr_channels);
                    self.elements[params_requested.element_index()] = None;
                } else {
                    // Keep the newly created SBR element. The error return must be evaluated.
                }
                sbr_error = err;
            }
        }
        match sbr_error {
            SbrError::Ok => {
                if element_id == ChannelElementId::Lfe {
                    if config_mode == ReconfigState::DetCfgChange {
                        self.params.set_flags(flags_saved);
                    }
                    return Ok(());
                }
            }
            _ => {
                if config_mode == ReconfigState::DetCfgChange {
                    self.params.set_flags(flags_saved);
                }
                return Err(sbr_error);
            }
        }

        let header_status = match &mut self.elements[element_index] {
            None => {
                if config_mode == ReconfigState::DetCfgChange {
                    super::header_data::HeaderData::read_header_bits(bs, self.params.flags())
                } else {
                    Status::Ok
                }
            }
            Some(sbr_elem) => sbr_elem.sbr_header_data_mut().get_header_data(
                bs,
                self.params.flags(),
                false,
                config_mode,
            ),
        };

        if core_codec == AudioObjectType::AotUsac {
            if config_mode == ReconfigState::DetCfgChange {
                self.params.set_flags(flags_saved);
            }
            if sbr_error == SbrError::Ok {
                return Ok(());
            } else {
                return Err(sbr_error);
            }
        }

        if config_mode == ReconfigState::AllocMem {
            if let Some(sbr_elem) = &mut self.elements[element_index] {
                let elem_channels = sbr_elem.element_channels();

                if (element_id == ChannelElementId::Cpe && elem_channels != 2)
                    || (element_id != ChannelElementId::Cpe && elem_channels != 1)
                {
                    return Err(SbrError::UnsupportedConfig);
                }

                if header_status == Status::Reset {
                    let sbr_header = sbr_elem.sbr_header_data_mut();
                    sbr_error = sbr_header.header_update(header_status, self.params.flags());

                    if sbr_error == SbrError::Ok {
                        sbr_header.set_sync_state(SyncState::Header);
                        sbr_header.set_header_updated(true)?;
                    } else {
                        sbr_header.set_sync_state(SyncState::NotInitialized);
                    }
                }
            }
        }

        if config_mode == ReconfigState::DetCfgChange {
            self.params.set_flags(flags_saved);
        }

        Ok(())
    }

    /// Disables SBR DRC for a certain channel.
    ///
    /// # Parameters
    /// - `element_index`: SBR element index. Max value: `SBRDEC_MAX_ELEMENTS`.
    /// - `channel`: The `AudioChannel` that has to be disabled.
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    pub fn drc_disable(
        &mut self,
        element_index: usize,
        channel: AudioChannel,
    ) -> Result<(), SbrError> {
        if element_index >= SBRDEC_MAX_ELEMENTS {
            return Err(SbrError::InvalidArgument);
        }

        match &mut self.elements[element_index] {
            None => {}
            Some(sbr_elem) => {
                if let Some(sbr_drc_channel) = sbr_elem.sbr_drc_channel_mut(usize::from(channel)) {
                    sbr_drc_channel.init_channel();
                }
            }
        }

        Ok(())
    }

    /// Feeds DRC channel data into an SBR decoder runtime instance.
    ///
    /// # Parameters
    ///
    ///  - `element_index`:             SBR element index.
    ///  - `channel`:                   Channel number to which the DRC data is associated to.
    ///  - `num_bands`:                 Number of DRC bands.
    ///  - `next_fact_mag`:             Pointer to a table with the DRC factor magnitudes.
    ///  - `drc_interpolation_scheme`:  DRC interpolation scheme.
    ///  - `win_sequence`:              Window sequence from core coder (8 short or 1 long window).
    ///  - `band_top`:                  Pointer to a table with the top borders for all DRC bands.
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    #[expect(clippy::too_many_arguments)]
    pub fn drc_feed_channel(
        &mut self,
        element_index: usize,
        channel: AudioChannel,
        num_bands: usize,
        next_fact_mag: &[f32],
        drc_interpolation_scheme: u16,
        win_sequence: u8,
        band_top: &[u16],
    ) -> Result<(), SbrError> {
        if element_index >= SBRDEC_MAX_ELEMENTS {
            return Err(SbrError::NotInitialized);
        }

        match &mut self.elements[element_index] {
            None => return Err(SbrError::NotInitialized),
            Some(sbr_elem) => match sbr_elem.sbr_drc_channel_mut(usize::from(channel)) {
                None => return Err(SbrError::NotInitialized),
                Some(sbr_drc_channel) => {
                    sbr_drc_channel.feed(
                        num_bands,
                        next_fact_mag,
                        drc_interpolation_scheme,
                        win_sequence,
                        band_top,
                        self.params.flags(),
                    )?;
                }
            },
        }

        Ok(())
    }

    /// Frees config dependent SBR memory.
    pub fn free_mem(&mut self) {
        for el in 0..SBRDEC_MAX_ELEMENTS {
            match &mut self.elements[el] {
                Some(sbr_elem) => {
                    if sbr_elem.destroy(&mut self.params.num_sbr_channels).is_ok() {
                        self.elements[el] = None;
                        self.params
                            .set_num_sbr_elements(self.params.num_sbr_elements() - 1);
                    }
                }
                None => {
                    // No SBR element allocated, nothing to do.
                }
            }
        }
    }

    /// Initializes an SBR decoder runtime instance. Must be called before decoding starts.
    ///
    /// # Parameters
    ///
    ///  - `qmf_domain`:                  QMF domain data
    ///  - `qmf_domain_params_requested`: QMF domain requested data
    ///  - `sample_rate_in`:              Input samplerate of the SBR decoder
    ///  - `sample_rate_out`:             Output samplerate of the SBR decoder
    ///  - `samples_per_frame`:           Core codec frame size
    ///  - `core_codec`:                  Audio Object Type (AOT) of the core codec
    ///  - `element_id`:                  Array with MPEG-4 SBR element IDs
    ///  - `num_channel_elements`:        Number of channel elements
    ///  - `harmonic_sbr`:                Harmonic SBR status
    ///  - `downscale_factor`:            ELD downscale factor
    ///  - `is_ps_possible`:              Is true parametric stereo possible?
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    #[expect(clippy::too_many_arguments)]
    pub fn init(
        &mut self,
        qmf_domain: &mut QmfDomain,
        qmf_domain_params_requested: &mut QmfDomainParams,
        sample_rate_in: u32,
        sample_rate_out: u32,
        samples_per_frame: u16,
        core_codec: AudioObjectType,
        element_id: &[ChannelElementId],
        num_channel_elements: usize,
        harmonic_sbr: u8,
        downscale_factor: u8,
        is_ps_possible: bool,
    ) -> Result<(), SbrError> {
        if num_channel_elements > SBRDEC_MAX_ELEMENTS {
            return Err(SbrError::UnsupportedConfig);
        }

        for (el, el_id) in element_id.iter().take(num_channel_elements).enumerate() {
            let mut params_requested = SbrDecoderParams::default();
            let mut is_config_changed = false;

            params_requested.set_requested_element_params(
                sample_rate_in,
                sample_rate_out,
                samples_per_frame,
                core_codec,
                harmonic_sbr,
                downscale_factor,
                is_ps_possible,
                *el_id,
                el,
            );

            let is_new_elem = match self.elements[params_requested.element_index()] {
                Some(_) => {
                    // The element already exists.
                    false
                }
                None => {
                    self.elements[params_requested.element_index()] =
                        Some(Box::new(SbrDecoderElement::new()));
                    true
                }
            };

            match self.elements[params_requested.element_index()]
                .as_mut()
                .unwrap()
                .init(
                    &mut self.params,
                    &params_requested,
                    qmf_domain,
                    qmf_domain_params_requested,
                    &mut is_config_changed,
                    ReconfigState::AllocMem,
                    is_new_elem,
                ) {
                Ok(keep) => {
                    if keep {
                        // Keep the SBR element if:
                        // - new SBR element in the list and a successful init().
                        // - existing SBR element in the list and early return from init().
                        // - existing element in the list and successful init().

                        // Make sure each SBR channel has one QMF channel assigned even if
                        // `num_sbr_channels` or element set-up has changed.
                        self.assign_qmf_channels_2_sbr_channels(qmf_domain_params_requested);
                    } else {
                        // Don't keep the SBR element.
                        let _ = self.elements[params_requested.element_index()]
                            .as_mut()
                            .unwrap()
                            .destroy(&mut self.params.num_sbr_channels);
                        self.elements[params_requested.element_index()] = None;
                    }
                }
                Err((err, free_mem)) => {
                    if free_mem {
                        // Don't keep the SBR element if:
                        // - is a newly created SBR element, but init() detects that the allocation
                        //   of its internals failed.
                        // - is an existing SBR element, but init() detects that the allocation of
                        //   its internals failed.
                        // The error return must be evaluated, nevertheless.
                        let _ = self.elements[params_requested.element_index()]
                            .as_mut()
                            .unwrap()
                            .destroy(&mut self.params.num_sbr_channels);
                        self.elements[params_requested.element_index()] = None;
                    } else {
                        // Keep the newly created SBR element. The error return must be evaluated.
                    }
                    return Err(err);
                }
            }
        }

        Ok(())
    }

    /// Determines the module's output signal delay, in samples.
    ///
    /// # Return
    ///
    /// - The signal delay added by the module, in samples.
    pub fn get_delay(&self) -> u32 {
        let mut output_delay = 0;

        let flags = self.params.flags();

        // See chapter 1.6.7.2 of ISO/IEC 14496-3 for the GA-SBR figures below.

        if self.params.num_sbr_channels() > 0 && self.params.num_sbr_elements() > 0 {
            // Add QMF synthesis delay.
            if flags.contains(SbrDecFlags::ELD_GRID) && self.params.core_codec().is_lowdelay_aot() {
                // Low delay SBR:
                if !flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
                    output_delay += if flags.contains(SbrDecFlags::DOWNSAMPLE) {
                        SBR_DECODER_OUTPUT_DELAY_ELD_DS
                    } else {
                        SBR_DECODER_OUTPUT_DELAY_ELD
                    };
                    if flags.contains(SbrDecFlags::LD_MPS_QMF) {
                        output_delay += SBR_DECODER_OUTPUT_DELAY_ELD_MPS;
                    }
                }
            } else if self.params.core_codec() != AudioObjectType::AotUsac {
                // By the method of elimination this is the GA (AAC-LC, HE-AAC, ...).
                output_delay += if flags.contains(SbrDecFlags::DOWNSAMPLE) {
                    SBR_DECODER_OUTPUT_DELAY_HE_AAC_DS
                } else {
                    SBR_DECODER_OUTPUT_DELAY_HE_AAC
                };

                // MPEG-4 eSBR
                if self.params.harmonic_sbr() == 1 {
                    output_delay += if flags.contains(SbrDecFlags::DOWNSAMPLE) {
                        u32::from(self.params.codec_frame_size())
                    } else {
                        2 * u32::from(self.params.codec_frame_size())
                    };
                }
                if flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
                    // QMF Synthesis
                    output_delay -= SBR_DECODER_OUTPUT_DELAY_HE_AAC_SKIP_QMF_SYN;
                }
            }
        }

        output_delay
    }

    /// Creates a new SBR decoder instance.
    pub fn new() -> Self {
        let mut init_params = SbrDecoderParams::new();
        init_params.set_delay_frames_num(SBRDEC_MAX_DELAY_FRAMES as u8);

        Self {
            params: init_params,
            ..Default::default()
        }
    }

    /// Parses one SBR element data extension data block. The bit stream position will be placed
    /// at the end of the SBR payload block. If no SBR payload length is given (`bs_pay_len` < 0),
    /// then the bit stream position on return will be random after this function call in case of
    /// errors, and any further decoding will be completely pointless.
    ///
    /// # Parameters
    ///
    /// - `bs`:              Bitstream to parse from.
    /// - `bs_pay_len`:      If > 0 this value is the SBR payload length. If < 0, the SBR payload
    ///   length is unknown. If == 0, no valid SBR payload is available.
    /// - `prev_element_id`: Previous SBR element ID.
    /// - `element_index`:   Index of the current channel element.
    /// - `ac_flags`:        Audio codec flags.
    /// - `el_flags`:        Flags specific to the current channel element.
    ///
    /// # Return
    /// - `Result<(), SbrError>`
    pub fn parse(
        &mut self,
        bs: &mut Bitstream,
        bs_pay_len: isize,
        prev_element_id: ChannelElementId,
        element_index: usize,
        ac_flags: ACFlags,
        ac_el_flags: ChannelFlags,
    ) -> Result<(), SbrError> {
        let mut error_status = SbrError::Ok;

        let mut frame_data_mem = [FrameData::new(); SBRDEC_MAX_EL_CHANNELS];
        let mut header_status = Status::NotPresent;

        let is_global_independency_flag = ac_flags.contains(ACFlags::INDEP);
        let is_bs_pvc = ac_el_flags.contains(ChannelFlags::USAC_PVC);
        let is_bs_inter_tes = ac_el_flags.contains(ChannelFlags::USAC_ITES);

        let mut crc_info = CrcInfo::new();
        let crc_poly;
        let crc_len;
        let mut crc_reg = 0;
        let mut sbr_crc = 0;
        let mut crc_start_value = 0;

        let mut do_decode_sbr_data = true;
        let mut align_bits = 0;

        // Sanity checks.
        if element_index >= SBRDEC_MAX_ELEMENTS {
            return Err(SbrError::NotInitialized);
        }

        let start_pos = bs.valid_bits();

        match &mut self.elements[element_index] {
            None => {
                return Err(SbrError::NotInitialized);
            }
            Some(sbr_elem) => {
                let curr_element_id = sbr_elem.element_id();
                let is_cpe = curr_element_id == ChannelElementId::Cpe;
                let num_elem_channels = if is_cpe { 2_usize } else { 1_usize };

                if bs_pay_len == 0 {
                    sbr_elem.set_frame_error_flag();
                    return Ok(());
                }

                let last_hdr_slot =
                    sbr_elem.last_header_slot(usize::from(self.params.delay_frames_num()));
                let this_hdr_slot = sbr_elem.header_slot();

                // Store frame_data; new parsed frame_data possibly corrupted.
                for (ch, data) in frame_data_mem
                    .iter_mut()
                    .enumerate()
                    .take(num_elem_channels)
                {
                    sbr_elem.store_frame_data(data, ch);
                }

                // Reset PS flag; will be set after PS was found.
                self.params.remove_flags(SbrDecFlags::PS_DECODED);

                if sbr_elem.sbr_header_data().is_header_updated() {
                    // Got a new header from extern (e.g. from an ASC).
                    header_status = Status::Ok;
                    sbr_elem.sbr_header_data_mut().set_header_updated(false)?;
                } else if this_hdr_slot != last_hdr_slot {
                    // Copy the last header into this slot, otherwise the header compare will
                    // trigger more Status::Reset than needed.
                    sbr_elem.copy_sbr_header(last_hdr_slot);
                }

                // Check if bit stream data is valid and matches the element context.
                if (prev_element_id != ChannelElementId::Sce
                    && prev_element_id != ChannelElementId::Cpe)
                    || prev_element_id != curr_element_id
                {
                    // There is no LFE SBR element (do upsampling, only).
                    do_decode_sbr_data = false;
                }

                if do_decode_sbr_data && bs.valid_bits() <= 0 {
                    do_decode_sbr_data = false;
                }

                // SBR CRC-check.
                if do_decode_sbr_data && ac_flags.contains(ACFlags::SBRCRC) {
                    crc_poly = SBR_DECODER_CRC_POLY;
                    crc_start_value = SBR_DECODER_CRC_START_VALUE;
                    crc_len = SBR_DECODER_CRC_LEN;
                    sbr_crc = bs.read(crc_len);

                    // Setup CRC decoder.
                    crc_info.init(crc_poly, crc_start_value, crc_len);

                    // Start CRC region.
                    crc_reg = crc_info.start_region(bs, 0);
                }

                // Read in the header data and issue a reset if change occured.
                if do_decode_sbr_data {
                    let mut is_sbr_header_present;
                    let sbr_header = sbr_elem.sbr_header_data_mut();

                    if self.params.flags().contains(SbrDecFlags::SYNTAX_USAC) {
                        let is_sbr_info_present;

                        if is_bs_inter_tes {
                            self.params
                                .set_flags(self.params.flags() | SbrDecFlags::USAC_ITES);
                        } else {
                            self.params.remove_flags(SbrDecFlags::USAC_ITES);
                        }

                        if is_global_independency_flag {
                            self.params
                                .set_flags(self.params.flags() | SbrDecFlags::USAC_INDEP);
                            is_sbr_info_present = true;
                            is_sbr_header_present = true;
                        } else {
                            self.params.remove_flags(SbrDecFlags::USAC_INDEP);
                            is_sbr_info_present = bs.read_bit() != 0;

                            is_sbr_header_present = if is_sbr_info_present {
                                bs.read_bit() != 0
                            } else {
                                false
                            };
                        }

                        if is_sbr_info_present {
                            sbr_header.bitstream_info_parse(
                                &mut header_status,
                                bs,
                                is_bs_pvc,
                                is_cpe,
                            );
                        }

                        if header_status == Status::Error {
                            // Corrupt SBR info data, do not decode and switch to UPSAMPLING.
                            match sbr_header.sync_state().cmp(&SyncState::Upsampling) {
                                Ordering::Less => (),
                                Ordering::Greater => {
                                    sbr_header.set_sync_state(SyncState::Upsampling);
                                }
                                Ordering::Equal => (),
                            }
                            do_decode_sbr_data = false;
                            is_sbr_header_present = false;
                        }

                        if is_sbr_header_present && do_decode_sbr_data {
                            let use_dflt_header = bs.read_bit() != 0;
                            if use_dflt_header {
                                is_sbr_header_present = false;
                                sbr_header.bitstream_set_default(&mut header_status);
                            }
                        }
                    } else {
                        is_sbr_header_present = bs.read_bit() != 0;
                    }

                    if is_sbr_header_present {
                        header_status = sbr_header.get_header_data(
                            bs,
                            self.params.flags(),
                            true,
                            ReconfigState::None,
                        );
                    }

                    if header_status == Status::Reset {
                        error_status = sbr_header.header_update(header_status, self.params.flags());

                        if error_status == SbrError::Ok {
                            sbr_header.set_sync_state(SyncState::Header);
                        } else {
                            header_status = Status::Error;
                            sbr_header.set_sync_state(SyncState::NotInitialized);
                        }
                    }

                    if error_status != SbrError::Ok {
                        do_decode_sbr_data = false;
                    }
                }

                // Read frame data.
                if sbr_elem.sbr_header_data().sync_state() >= SyncState::Header
                    && do_decode_sbr_data
                {
                    do_decode_sbr_data = sbr_elem.read(bs, &mut self.params, is_cpe);

                    if do_decode_sbr_data {
                        do_decode_sbr_data = check_remaining_bits(
                            bs,
                            &mut align_bits,
                            start_pos,
                            bs_pay_len,
                            self.params.core_codec(),
                        );
                    }
                } else {
                    // Return an error so that the caller can react respectively.
                    error_status = SbrError::ParseError;
                }

                if ac_flags.contains(ACFlags::SBRCRC)
                    && sbr_elem.sbr_header_data().sync_state() >= SyncState::Header
                    && do_decode_sbr_data
                {
                    bs.push(align_bits);
                    crc_info.end_region(bs, crc_reg);
                    bs.push(-align_bits);

                    // Check CRC.
                    if (crc_info.value() ^ crc_start_value) != sbr_crc {
                        do_decode_sbr_data = false;
                        if header_status != Status::NotPresent {
                            header_status = Status::Error;
                            let sbr_header_data = sbr_elem.sbr_header_data_mut();
                            sbr_header_data.set_sync_state(SyncState::NotInitialized);
                        }
                    }
                }

                if !do_decode_sbr_data {
                    // Set error flag for this slot to trigger concealment.
                    sbr_elem.set_frame_error_flag();
                    // Restore old frame_data for concealment.
                    for (ch, data) in frame_data_mem
                        .iter_mut()
                        .enumerate()
                        .take(num_elem_channels)
                    {
                        sbr_elem.store_frame_data(data, ch);
                    }
                    error_status = SbrError::ParseError;
                } else {
                    // Everything seems to be ok so clear the error flag.
                    sbr_elem.reset_frame_error_flag();
                }

                if !is_cpe {
                    // Turn coupling off explicitely to avoid access to absent right frame data
                    // that might occur with corrupt bitstreams.
                    sbr_elem.set_coupling_mode(0, CouplingMode::Off);
                }

                Self::bail(
                    sbr_elem,
                    error_status,
                    header_status,
                    this_hdr_slot,
                    last_hdr_slot,
                    usize::from(self.params.delay_frames_num()),
                )?;
            }
        }

        Ok(())
    }

    /// Sets a parameter of the SBR decoder runtime instance.
    ///
    /// # Parameters
    ///
    /// - `param`: `SbrDecParam` parameter with its associated value.
    ///
    /// # Return
    ///
    /// - `Result<(), SbrError>`
    pub fn set_param(&mut self, param: SbrDecParam) -> Result<(), SbrError> {
        match param {
            SbrDecParam::SystemBitstreamDelay(bs_delay) => {
                if bs_delay > SBRDEC_MAX_DELAY_FRAMES as u8 {
                    return Err(SbrError::SetParamFail);
                }
                self.params.set_delay_frames_num(bs_delay);
            }
            SbrDecParam::LdQmfTimeAlign(is_ld_mps) => {
                if is_ld_mps {
                    self.params
                        .set_flags(self.params.flags() | SbrDecFlags::LD_MPS_QMF);
                } else {
                    self.params.remove_flags(SbrDecFlags::LD_MPS_QMF);
                };
            }
            SbrDecParam::FlushData(do_flush) => {
                if do_flush {
                    self.params
                        .set_flags(self.params.flags() | SbrDecFlags::FLUSH);
                }
            }
            SbrDecParam::SkipQmf(qmf_mode_opt) => {
                let mut flags = self.params.flags();
                match qmf_mode_opt {
                    Some(qmf_mode) => match qmf_mode {
                        QmfMode::Analysis => {
                            flags.insert(SbrDecFlags::SKIP_QMF_ANA);
                            flags.remove(SbrDecFlags::SKIP_QMF_SYN);
                        }
                        QmfMode::Synthesis => {
                            flags.insert(SbrDecFlags::SKIP_QMF_SYN);
                            flags.remove(SbrDecFlags::SKIP_QMF_ANA);
                        }
                    },
                    None => {
                        // Reset QMF skip flags.
                        flags.remove(SbrDecFlags::SKIP_QMF_ANA);
                        flags.remove(SbrDecFlags::SKIP_QMF_SYN);
                    }
                }
                self.params.set_flags(flags);
            }
            SbrDecParam::BsInterruption => {
                for el in 0..usize::from(self.params.num_sbr_elements()) {
                    match &mut self.elements[el] {
                        Some(sbr_elem) => {
                            let sbr_header = sbr_elem.sbr_header_data_mut();
                            // Set sync state UPSAMPLING for the corresponding slot.
                            // This switches off bitstream parsing until a new header arrives.
                            if sbr_header.sync_state() != SyncState::NotInitialized {
                                sbr_header.set_sync_state(SyncState::Upsampling);
                                sbr_header.set_header_updated(true)?;
                            }
                        }
                        None => {
                            // No SBR element allocated, nothing to do.
                        }
                    }
                }
            }
        }
        Ok(())
    }
}

/// Checks remaining bits.
///
/// # Parameters
///
/// - `bs`:         Bitstream data to read from.
/// - `align_bits`: Align bits.
/// - `start_pos`:  Bitstream start position.
/// - `bs_pay_len`: Bitstream payload length.
/// - `core_codec`: Audio Object Type (AOT) of the core codec.
///
/// # Return
///
/// - Flag, indicating whether sbr data shall be decoded.
pub fn check_remaining_bits(
    bs: &mut Bitstream,
    align_bits: &mut isize,
    start_pos: isize,
    bs_pay_len: isize,
    core_codec: AudioObjectType,
) -> bool {
    let mut do_decode_sbr_data = true;

    let valid_bits = if bs_pay_len > 0 {
        bs_pay_len - (start_pos - bs.valid_bits())
    } else {
        bs.valid_bits()
    };

    if valid_bits < 0 {
        do_decode_sbr_data = false;
    } else {
        match core_codec {
            AudioObjectType::AotSbr | AudioObjectType::AotPs | AudioObjectType::AotAacLc => {
                // This sanity check is only meaningful with General Audio bitstreams.
                *align_bits = valid_bits & 0x7;

                if valid_bits > *align_bits {
                    do_decode_sbr_data = false;
                }
            }
            _ => {
                // No sanity check available.
            }
        }
    }

    do_decode_sbr_data
}
