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
//! SBR element.

use crate::{
    common::{
        aot::AudioObjectType,
        bitstream::Bitstream,
        bs_element_id::ChannelElementId,
        channel_map_descr::ChannelMapDescriptor,
        qmf::QmfFlags,
        qmf_domain::{QmfDomain, QmfDomainParams, QmfDomainSelect},
    },
    sbr_dec::{common::SbrError, flags::SbrDecFlags},
    tp_dec::ReconfigState,
};

use super::{
    constants::{
        SBRDEC_LD_MPS_QMF, SBRDEC_MAX_DELAY_FRAMES, SBRDEC_MAX_ELEMENTS, SBRDEC_MAX_EL_CHANNELS,
        SBRDEC_MAX_SAMPLE_RATE_IN, SBRDEC_MAX_SAMPLE_RATE_OUT, SBR_DUAL_RATE_OVERLAP_SIZE,
        SBR_QUAD_RATE_OVERLAP_SIZE,
    },
    drc::SbrDecDrcChannel,
    frame_data::{FrameData, PrevFrameData, SbrPatchingMode},
    header_data::{compare_header, HeaderData, Status, SyncState},
    huff_dec::CouplingMode,
    lpp_trans::TransposerSettings,
    process::{SbrDec, SbrDecEsbrMode},
    psdec::PsDec,
    pvc::Mode,
};

const LEFT: usize = 0;
const RIGHT: usize = 1;

#[derive(Default, Debug)]
#[repr(C)]
/// SBR channel.
pub struct SbrChannel {
    /// Frame data array.
    frame_data: [FrameData; SBRDEC_MAX_DELAY_FRAMES + 1],
    /// Previous frame data.
    prev_frame_data: PrevFrameData,
    /// SBR decoder.
    pub(super) sbr_dec: SbrDec,
}

impl SbrChannel {
    /// Returns new instance of `SbrChannel`.
    pub fn new() -> Self {
        SbrChannel::default()
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
/// Decoder parameters.
pub struct SbrDecoderParams {
    // Used for params + requested params.
    /// Audio object type of core codec.
    core_codec: AudioObjectType,
    /// Core codec frame size.
    codec_frame_size: u16,
    /// SBR decoder input sampling rate (might be different than
    /// the transposer input sampling rate).
    sample_rate_in: u32,
    /// SBR decoder output sampling rate.
    sample_rate_out: u32,
    /// Harmonic SBR state.
    harmonic_sbr: u8,
    /// Downsample factor.
    downscale_factor: u8,
    // Used only for params.
    /// Synthesis downsample factor.
    synthesis_downsample_factor: u8,
    /// Number of overall SBR elements.
    num_sbr_elements: u8,
    /// Number of overall SBR channels.
    pub(super) num_sbr_channels: u8,
    /// Current number of additional delay frames used for processing.
    num_delay_frames: u8,
    /// Number of frames flushed consecutively.
    num_flushed_frames: u8,
    /// SBR decoder flags.
    flags: SbrDecFlags,
    /// Indicates if PS has been applied.
    is_ps_applied: bool,
    // Used only for requested parameters.
    /// SBR element ID.
    element_id: ChannelElementId,
    /// Index of SBR element to be rendered.
    element_index: usize,
    /// Indicates if parametric stereo is possible.
    is_ps_possible: bool,
}

impl Default for SbrDecoderParams {
    fn default() -> Self {
        SbrDecoderParams {
            num_delay_frames: SBRDEC_MAX_DELAY_FRAMES as u8,
            core_codec: Default::default(),
            codec_frame_size: Default::default(),
            sample_rate_in: Default::default(),
            sample_rate_out: Default::default(),
            harmonic_sbr: Default::default(),
            downscale_factor: Default::default(),
            synthesis_downsample_factor: Default::default(),
            num_sbr_elements: Default::default(),
            num_sbr_channels: Default::default(),
            num_flushed_frames: Default::default(),
            flags: Default::default(),
            is_ps_applied: Default::default(),
            element_id: Default::default(),
            element_index: Default::default(),
            is_ps_possible: Default::default(),
        }
    }
}

impl SbrDecoderParams {
    /// Returns new instance of `SbrDecoderParams`.
    pub fn new() -> Self {
        Default::default()
    }

    /// Returns the size of codec frame.
    pub fn codec_frame_size(&self) -> u16 {
        self.codec_frame_size
    }

    /// Returns mutable reference to the number of SBR channels.
    pub fn num_sbr_channels_mut(&mut self) -> &mut u8 {
        &mut self.num_sbr_channels
    }

    pub(super) fn num_sbr_channels(&self) -> u8 {
        self.num_sbr_channels
    }

    pub(super) fn core_codec(&self) -> AudioObjectType {
        self.core_codec
    }

    pub(super) fn delay_frames_num(&self) -> u8 {
        self.num_delay_frames
    }

    pub(super) fn element_index(&self) -> usize {
        self.element_index
    }

    pub(super) fn num_sbr_elements(&self) -> u8 {
        self.num_sbr_elements
    }

    pub fn flags(&self) -> SbrDecFlags {
        self.flags
    }

    pub(super) fn flushed_frames(&self) -> u8 {
        self.num_flushed_frames
    }

    pub(super) fn harmonic_sbr(&self) -> u8 {
        self.harmonic_sbr
    }

    /// Restes the SBR decoder parameters.
    fn reset(&mut self) {
        self.core_codec = AudioObjectType::AotNullObject;
        self.codec_frame_size = 0;
        self.sample_rate_in = 0;
        self.sample_rate_out = 0;
        self.harmonic_sbr = 0;
        self.downscale_factor = 0;
        self.synthesis_downsample_factor = 0;
        self.num_sbr_elements = 0;
        self.num_sbr_channels = 0;
        self.num_delay_frames = 0;
        self.num_flushed_frames = 0;
        self.flags = SbrDecFlags::empty();
        self.is_ps_applied = false;
        self.element_id = ChannelElementId::None;
        self.element_index = 0;
        self.is_ps_possible = false;
    }

    /// Sets SBR decoder parameters.
    ///
    /// # Parameters
    ///
    /// - `sample_rate_in`: Input samplerate of the SBR decoder instance.
    /// - `sample_rate_out`: Output samplerate of the SBR decoder instance.
    /// - `codec_frame_size`: Number of samples per frame.
    /// - `core_codec`: Audio Object Type (AOT) of the core codec.
    /// - `harmonic_sbr`: Harmonic SBR status.
    /// - `downscale_factor`: ELD downscale factor.
    /// - `is_ps_possible`: `true` if parametric stereo is possible, `false` otherwise.
    /// - `element_id`: Element ID.
    /// - `element_index`: Element index.
    #[expect(clippy::too_many_arguments)]
    pub fn set_requested_element_params(
        &mut self,
        sample_rate_in: u32,
        sample_rate_out: u32,
        codec_frame_size: u16,
        core_codec: AudioObjectType,
        harmonic_sbr: u8,
        downscale_factor: u8,
        is_ps_possible: bool,
        element_id: ChannelElementId,
        element_index: usize,
    ) {
        self.reset();

        self.sample_rate_in = sample_rate_in;
        self.sample_rate_out = sample_rate_out;
        self.codec_frame_size = codec_frame_size;
        self.core_codec = core_codec;
        self.harmonic_sbr = harmonic_sbr;
        self.downscale_factor = downscale_factor;
        self.is_ps_possible = is_ps_possible;
        self.element_id = element_id;
        self.element_index = element_index;
    }

    /// Calculates sampling parameters.
    ///
    /// # Parameters
    ///
    /// - `params_requested`: Requested SBR decoder parameters.
    ///
    /// # Return
    ///
    /// - `SbrError`
    fn calc_sampling_parameters(
        &mut self,
        params_requested: &SbrDecoderParams,
    ) -> Result<(), SbrError> {
        // USAC: assuming theoretical case 8 kHz output sample rate with 4:1 SBR.
        let sbr_min_sample_rate_in = if params_requested.core_codec == AudioObjectType::AotUsac {
            2000
        } else {
            6400
        };

        // Check input samplerate.
        if params_requested.sample_rate_in < sbr_min_sample_rate_in
            || params_requested.sample_rate_in > SBRDEC_MAX_SAMPLE_RATE_IN
        {
            return Err(SbrError::UnsupportedConfig);
        }

        // Check output samplerate.
        if params_requested.sample_rate_out > SBRDEC_MAX_SAMPLE_RATE_OUT {
            return Err(SbrError::UnsupportedConfig);
        }

        // Set downsampling factor for synthesis filter bank.
        if params_requested.sample_rate_out == 0 {
            // In case of implicit signalling, assume dual rate SBR.
            self.sample_rate_out = params_requested.sample_rate_in << 1;
        } else {
            self.sample_rate_out = params_requested.sample_rate_out;
        }

        if params_requested.sample_rate_in == self.sample_rate_out {
            self.synthesis_downsample_factor = 2;
            self.flags.insert(SbrDecFlags::DOWNSAMPLE);
        } else {
            self.synthesis_downsample_factor = 1;
            self.flags.remove(SbrDecFlags::DOWNSAMPLE);
        }

        Ok(())
    }

    /// Returns number of element channels.
    fn number_of_element_channels(&self) -> u8 {
        let mut num_el_channels = 0;

        // Determine amount of channels for element.
        match self.element_id {
            ChannelElementId::None | ChannelElementId::Cpe => num_el_channels = 2,
            ChannelElementId::Sce | ChannelElementId::Lfe => num_el_channels = 1,
            _ => (),
        }

        // Consider parametric stereo case.
        if self.is_ps_possible
            && self.element_index == 0
            && self.element_id == ChannelElementId::Sce
        {
            match self.core_codec {
                AudioObjectType::AotAacLc
                | AudioObjectType::AotSbr
                | AudioObjectType::AotErAacScal
                | AudioObjectType::AotPs => num_el_channels = 2,
                _ => (),
            }
        }

        num_el_channels
    }

    pub(super) fn sample_rate_out(&self) -> u32 {
        self.sample_rate_out
    }

    pub(super) fn set_delay_frames_num(&mut self, val: u8) {
        self.num_delay_frames = val;
    }

    /// Removes given flags from `SbrDecFlags` flags.
    pub(super) fn remove_flags(&mut self, flags: SbrDecFlags) {
        self.flags.remove(flags);
    }

    pub(super) fn set_flags(&mut self, f_val: SbrDecFlags) {
        self.flags = f_val;
    }

    pub(super) fn set_flushed_frames_num(&mut self, val: u8) {
        self.num_flushed_frames = val;
    }

    pub(super) fn set_num_sbr_elements(&mut self, val: u8) {
        self.num_sbr_elements = val;
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// SBR decoder element.
pub struct SbrDecoderElement {
    /// Parametric stereo decoder.
    parametric_stereo_dec: Option<Box<PsDec>>,
    /// SBR header of each indvidual channel of the element.
    sbr_header: [HeaderData; SBRDEC_MAX_DELAY_FRAMES + 1],
    /// Individual channel of the element.
    sbr_channel: Vec<SbrChannel>,
    /// Transport settings for each channel of the element.
    transposer_settings: TransposerSettings,
    /// Element ID set during initialisation. Can be used for concealment.
    element_id: ChannelElementId,
    /// Number of elements' output channels (two in case of PS).
    num_channels: usize,
    /// Frame error status for each slot in the delay line. Copied into header
    /// at the beginning of decode_element().
    frame_error_flag: [bool; SBRDEC_MAX_DELAY_FRAMES + 1],
    /// Index of slot to be decoded/filled (used with additional delay).
    use_frame_slot: usize,
    /// Index array that provides link between header and frame data.
    use_header_slot: [usize; SBRDEC_MAX_DELAY_FRAMES + 1],
}

impl SbrDecoderElement {
    /// Returns new instance of `SbrDecoderElement`.
    pub fn new() -> Self {
        SbrDecoderElement {
            sbr_channel: Vec::<SbrChannel>::with_capacity(SBRDEC_MAX_EL_CHANNELS),
            ..Default::default()
        }
    }

    /// Resets frame error flag.
    pub fn reset_frame_error_flag(&mut self) {
        self.frame_error_flag[self.use_frame_slot] = false;
    }

    /// Sets frame error flag.
    pub fn set_frame_error_flag(&mut self) {
        self.frame_error_flag[self.use_frame_slot] = true;
    }

    /// Sets frame error flags.
    pub fn set_frame_error_flags(&mut self) {
        self.frame_error_flag.fill(true);
    }

    /// Sets coupling mode.
    pub fn set_coupling_mode(&mut self, ch: usize, mode: CouplingMode) {
        self.sbr_channel[ch].frame_data[self.use_frame_slot].set_coupling_mode(mode);
    }

    /// Determines slot of last header.
    ///
    /// # Parameters
    ///
    /// - `num_delay_frames`: number of delayed frames.
    ///
    /// # Return
    ///
    /// Index of last SBR header data.
    pub fn last_header_slot(&self, num_delay_frames: usize) -> usize {
        let slot = if self.use_frame_slot > 0 {
            self.use_frame_slot - 1
        } else {
            num_delay_frames
        };

        self.use_header_slot[slot]
    }

    /// Determines an unoccupied header slot for storing a new header.
    ///
    /// # Return
    ///
    /// Index to SBR header data.
    pub fn header_slot(&self) -> usize {
        let mut occupied = 0;
        let mut slot = self.use_header_slot[self.use_frame_slot];

        for s in 0..SBRDEC_MAX_DELAY_FRAMES + 1 {
            if self.use_header_slot[s] == slot && s != slot {
                occupied = 1;
                break;
            }
        }

        if occupied != 0 {
            occupied = 0;

            for s in 0..SBRDEC_MAX_DELAY_FRAMES + 1 {
                occupied |= 1 << self.use_header_slot[s];
            }
            for s in 0..SBRDEC_MAX_DELAY_FRAMES + 1 {
                if (occupied & 0x1) == 0 {
                    slot = s;
                    break;
                }
                occupied >>= 1;
            }
        }

        slot
    }

    /// Returns mutable reference to SBR header data.
    pub fn sbr_header_data_mut(&mut self) -> &mut HeaderData {
        let idx = self.header_slot();
        &mut self.sbr_header[idx]
    }

    /// Returns immutable reference to SBR header data.
    pub fn sbr_header_data(&self) -> &HeaderData {
        let idx = self.header_slot();
        &self.sbr_header[idx]
    }

    /// Returns reference to SBR DRC channel.
    pub fn sbr_drc_channel(&self, idx: usize) -> Option<&SbrDecDrcChannel> {
        if idx < self.sbr_channel.len() {
            Some(self.sbr_channel[idx].sbr_dec.sbr_drc_channel())
        } else {
            None
        }
    }

    /// Returns reference to SBR DRC channel.
    pub fn sbr_drc_channel_mut(&mut self, idx: usize) -> Option<&mut SbrDecDrcChannel> {
        if idx < self.sbr_channel.len() {
            Some(self.sbr_channel[idx].sbr_dec.sbr_drc_channel_mut())
        } else {
            None
        }
    }

    /// Returns SBR channel reference.
    pub fn sbr_channel(&self, idx: usize) -> Option<&SbrChannel> {
        if idx < self.sbr_channel.len() {
            Some(&self.sbr_channel[idx])
        } else {
            None
        }
    }

    /// Returns SBR channel reference.
    pub(super) fn sbr_channel_mut(&mut self) -> &mut Vec<SbrChannel> {
        &mut self.sbr_channel
    }

    /// Returns element ID.
    pub fn element_id(&self) -> ChannelElementId {
        self.element_id
    }

    /// Returns number of element channels.
    pub fn element_channels(&self) -> u8 {
        self.sbr_channel.len() as u8
    }

    /// Compares header data.
    ///
    /// # Parameters
    ///
    /// - `header_idx`: Header index.
    ///
    /// # Return
    ///
    /// - `true` if header is equal, `false` otherwise.
    pub fn compare_sbr_header(&self, header_idx: usize) -> bool {
        let mut result = false;

        let header_index_2 = self.header_slot();

        if header_idx != header_index_2 {
            result = compare_header(
                &self.sbr_header[header_idx],
                &self.sbr_header[header_index_2],
            );
        }

        result
    }

    /// Copies header data.
    ///
    /// # Parameters
    ///
    /// - `header_idx_src`: Header data source index.
    pub fn copy_sbr_header(&mut self, header_idx_src: usize) {
        let header_index_dst = self.header_slot();

        if header_index_dst != header_idx_src {
            self.sbr_header[header_index_dst] = self.sbr_header[header_idx_src];
        }
    }

    /// Restores frame data.
    ///
    /// # Parameters
    ///
    /// - `frame_data`: Source of frame data to be restored.
    /// - `idx`: Channel index.
    pub fn restore_frame_data(&mut self, frame_data: &FrameData, idx: usize) {
        self.sbr_channel[idx].frame_data[self.use_frame_slot].clone_from(frame_data);
    }

    /// Stores frame data.
    ///
    /// # Parameters
    ///
    /// - `frame_data`: Destination of the frame data to be saved.
    /// - `idx`: Channel index.
    pub fn store_frame_data(&self, frame_data: &mut FrameData, idx: usize) {
        frame_data.clone_from(&self.sbr_channel[idx].frame_data[self.use_frame_slot]);
    }

    /// Updates header and frame slot.
    ///
    /// # Parameters
    ///
    /// - `is_use_old_hdr`: Indicates if old header should be used.
    /// - `this_hdr_slot`: Header of current frame.
    /// - `last_hdr_slot`: Header of last frame.
    /// - `num_delay_frames`: Number of delayed frames.
    pub fn update_header_and_frame_slot(
        &mut self,
        is_use_old_hdr: bool,
        this_hdr_slot: usize,
        last_hdr_slot: usize,
        num_delay_frames: usize,
    ) {
        if is_use_old_hdr {
            // Use the old header for this frame.
            self.use_header_slot[self.use_frame_slot] = last_hdr_slot;
        } else {
            // Use the new header for this frame.
            self.use_header_slot[self.use_frame_slot] = this_hdr_slot;
        }

        // Move frame pointer to the next slot which is up to be decoded/applied next.
        self.use_frame_slot = (self.use_frame_slot + 1) % (num_delay_frames + 1);
    }

    /// Reads SBR element data.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `params`: SBR decoder parameters.
    /// - `is_cpe`: Flag indicating whether element is CPE.
    ///
    /// # Return
    ///
    /// - `true` if SBR data should be decoded, `false` otherwise.
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        params: &mut SbrDecoderParams,
        is_cpe: bool,
    ) -> bool {
        let num_element_channels = if is_cpe { 2 } else { 1 };
        let header_idx = self.header_slot();
        let sbr_header = &mut self.sbr_header[header_idx];

        if !is_cpe {
            // Update slot index for PS bitstream parsing.
            if let Some(parametric_stereo_decoder) = self.parametric_stereo_dec.as_mut() {
                let slot = parametric_stereo_decoder.bs_data_mut().bs_read_slot();
                parametric_stereo_decoder
                    .bs_data_mut()
                    .set_bs_last_slot(slot);

                parametric_stereo_decoder
                    .bs_data_mut()
                    .set_bs_read_slot(self.use_frame_slot as u8);
            }
        }

        let pvc_mode_last = if params.flags.contains(SbrDecFlags::SYNTAX_USAC)
            && !self.sbr_channel.is_empty()
            && self.sbr_channel[LEFT].sbr_dec.pvc_data().is_some()
        {
            self.sbr_channel[LEFT].sbr_dec.pvc_data().unwrap().mode()
        } else {
            Mode::None
        };

        let sbr_channel = &mut self.sbr_channel;
        let (sbr_ch_left, sbr_ch_right) = sbr_channel.split_at_mut(1);

        let mut sbr_frame_ok;
        let frame_data_left_prev = &mut sbr_ch_left[0].prev_frame_data;

        let mut frame_data_right = if num_element_channels == 2 {
            Some(&mut sbr_ch_right[0].frame_data[self.use_frame_slot])
        } else {
            None
        };

        let frame_data_left = &mut sbr_ch_left[0].frame_data[self.use_frame_slot];
        sbr_frame_ok = frame_data_left
            .sbr_get_channel_element(
                frame_data_left_prev,
                &mut frame_data_right,
                bs,
                sbr_header,
                pvc_mode_last,
                params.flags,
                self.transposer_settings.overlap_size(),
            )
            .is_ok();

        // Read extended data.
        if !params.flags.contains(SbrDecFlags::SYNTAX_USAC) {
            let res = frame_data_left.extract_extended_data(
                &mut frame_data_right,
                sbr_header,
                bs,
                if is_cpe {
                    None
                } else {
                    self.parametric_stereo_dec.as_deref_mut()
                },
                params.flags,
            );

            if res.is_err() {
                sbr_frame_ok = false;
            }
        }

        // After a config change the HBE processing is disabled. Once a MPEG-4 SBR
        // enhancements element is available in the bitstream the HBE processing is
        // enabled. Switching off the HBE processing is not provided, only in
        // conjunction with a config change.
        if !params.flags.contains(SbrDecFlags::SYNTAX_USAC)
            && params.flags.contains(SbrDecFlags::USAC_HARMONICSBR)
        {
            let mut hbe_patching = false;
            hbe_patching |= frame_data_left.sbr_patching_mode() == SbrPatchingMode::Hbe;

            if let Some(right_frame) = frame_data_right {
                hbe_patching |= right_frame.sbr_patching_mode() == SbrPatchingMode::Hbe;
            }

            {
                let sbr_dec_left = &mut sbr_ch_left[0].sbr_dec;
                let esbr_mode = sbr_dec_left.esbr_mode();

                if esbr_mode == SbrDecEsbrMode::Off && hbe_patching {
                    sbr_dec_left.set_esbr_mode(SbrDecEsbrMode::Start);
                } else if esbr_mode == SbrDecEsbrMode::Pause && hbe_patching {
                    sbr_dec_left.set_esbr_mode(SbrDecEsbrMode::On);
                } else if esbr_mode == SbrDecEsbrMode::On && !hbe_patching {
                    sbr_dec_left.set_esbr_mode(SbrDecEsbrMode::Pause);
                }
            }

            if is_cpe {
                let esbr_mode = sbr_ch_left[0].sbr_dec.esbr_mode();

                if num_element_channels == 2 {
                    self.sbr_channel[RIGHT].sbr_dec.set_esbr_mode(esbr_mode);
                }
            }
        }

        sbr_frame_ok
    }

    /// Destroys SBR element.
    ///
    /// # Parameters
    ///
    /// - num_sbr_channels`: Number of SBR channels.
    ///
    /// # Return
    ///
    /// - `SbrError`
    pub fn destroy(&mut self, num_sbr_channels: &mut u8) -> Result<(), SbrError> {
        if let Some(mut ps_dec) = self.parametric_stereo_dec.take() {
            ps_dec.deinit();
        }

        for sbr_channel in self.sbr_channel.iter_mut() {
            sbr_channel.sbr_dec.delete();
            *num_sbr_channels -= 1;
        }
        self.sbr_channel.clear();
        self.sbr_channel.shrink_to_fit();

        Ok(())
    }

    fn bail_from_init(
        &mut self,
        params: &mut SbrDecoderParams,
        element_index: usize,
        elements_start: u8,
        channels_start: u8,
        is_element_valid: bool,
    ) -> bool {
        let mut is_free_mem = false;
        if elements_start < params.num_sbr_elements || channels_start < params.num_sbr_channels {
            // Free the memory allocated for this element.
            if is_element_valid && self.destroy(&mut params.num_sbr_channels).is_ok() {
                // The SBR element is de-allocated. Decrement the number of SBR elements.
                params.num_sbr_elements -= 1;
                is_free_mem = true;
            }
        } else if element_index < SBRDEC_MAX_ELEMENTS && is_element_valid {
            // Set error flag to trigger concealment.
            self.set_frame_error_flag();
        }
        is_free_mem
    }

    fn reconfig(&mut self, params: &mut SbrDecoderParams, params_requested: &SbrDecoderParams) {
        let sbrdec_force_reset =
            SbrDecFlags::USAC_HBE_LP | SbrDecFlags::FLUSH | SbrDecFlags::empty();

        params.flags &= sbrdec_force_reset;

        if params_requested.downscale_factor > 1 {
            params.flags.insert(SbrDecFlags::ELD_DOWNSCALE);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };

        if params_requested.core_codec == AudioObjectType::AotErAacEld {
            params.flags.insert(SbrDecFlags::ELD_GRID);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };

        if params_requested.core_codec == AudioObjectType::AotErAacScal {
            params.flags.insert(SbrDecFlags::SYNTAX_SCAL);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };

        if params_requested.core_codec == AudioObjectType::AotUsac {
            params.flags.insert(SbrDecFlags::SYNTAX_USAC);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };

        // Robustness: Take integer division rounding into consideration. E.g. 22050
        // Hz with 4:1 SBR => 5512 Hz core sampling rate.
        if params_requested.sample_rate_in == (params_requested.sample_rate_out / 4) {
            params.flags.insert(SbrDecFlags::QUAD_RATE);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };

        if params_requested.harmonic_sbr == 1 {
            params.flags.insert(SbrDecFlags::USAC_HARMONICSBR);
        } else {
            params.flags.insert(SbrDecFlags::empty());
        };
    }

    /// Resets SBR decoder element.
    ///
    /// # Parameters
    ///
    /// - `params`: SBR decoder parameter.
    /// - `qmf_domain`: QMF domain data structure.
    /// - `qmf_domain_params_requested`: QMF domain requested data structure.
    ///
    /// # Return
    ///
    /// - `SbrError`
    fn reset(
        &mut self,
        params: &mut SbrDecoderParams,
        qmf_domain: &mut QmfDomain,
        qmf_domain_params_requested: &mut QmfDomainParams,
    ) -> Result<(), SbrError> {
        let mut err = SbrError::Ok;
        for idx in 0..(SBRDEC_MAX_DELAY_FRAMES + 1) {
            let set_dflt = self.sbr_header[idx].sync_state() == SyncState::NotInitialized;

            // Init a default header such that we can at least do upsampling later.
            err = self.sbr_header[idx].init_header_data(
                params.sample_rate_in,
                params.sample_rate_out,
                params.downscale_factor,
                params.codec_frame_size,
                params.flags,
                set_dflt,
            );

            // Set synchState to UPSAMPLING in case it already is initialized.
            if self.sbr_header[idx].sync_state() > SyncState::Upsampling {
                self.sbr_header[idx].set_sync_state(SyncState::Upsampling);
            }
        }

        if err != SbrError::Ok {
            return Err(err);
        }

        // Init requested qmf domain parameters.
        if qmf_domain.explicit_config() == 0 {
            qmf_domain_params_requested
                .flags
                .insert(qmf_flags(params.flags.bits(), params.core_codec));
            qmf_domain_params_requested.num_bands_analysis =
                self.sbr_header[0].number_of_analysis_bands();

            // May be overwritten by MPS.
            qmf_domain_params_requested.num_bands_synthesis =
                if params.synthesis_downsample_factor == 1 {
                    64
                } else {
                    32
                };
            qmf_domain_params_requested.num_bands_synthesis /= params.downscale_factor as u16;
            qmf_domain_params_requested.num_qmf_time_slots =
                self.sbr_header[0].number_time_slots() * self.sbr_header[0].time_step();
            qmf_domain_params_requested.num_qmf_ov_time_slots =
                number_of_qmf_overlap_time_slots(params.flags, params.core_codec);
            // Always 64.
            qmf_domain_params_requested.num_qmf_proc_bands = 64;
            // May be overwritten by MPS.
            qmf_domain_params_requested.num_qmf_proc_channels = 1;
        }
        qmf_domain_params_requested.num_work_buffer = qmf_domain_params_requested
            .num_work_buffer
            .max(self.num_channels as u8);

        // Init SBR channels going to be assigned to a SBR element.
        {
            let header_index = self.header_slot();

            let sbr_header = &mut self.sbr_header[header_index];

            let freq_band_header_data = &sbr_header.bitstream().freq_band_header_data();
            let crossover_band = sbr_header.bitstream_info().crossover_band();
            let number_of_analysis_bands = sbr_header.number_of_analysis_bands();
            let sampling_rate = sbr_header.processing_sampling_rate();

            // Reset frequency band tables.
            err = sbr_header.freq_band_data_as_mut().reset_freq_band_tables(
                freq_band_header_data,
                crossover_band,
                number_of_analysis_bands,
                sampling_rate,
                params.flags,
            );

            if err != SbrError::Ok {
                return Err(err);
            }

            let fbd = sbr_header.freq_band_data();
            let mtl = usize::from(fbd.num_master);
            let ntl = usize::from(fbd.num_freq_bands_noise());
            // Create lppTransposer.
            err = self.transposer_settings.init(
                &fbd.master_band_table()[..=mtl],
                &fbd.freq_band_table_noise()[..=ntl],
                sbr_header.number_time_slots() as usize,
                sbr_header.processing_sampling_rate(),
                fbd.start_subband,
                fbd.end_subband,
                sbr_header.number_time_slots() * sbr_header.time_step(),
                number_of_qmf_overlap_time_slots(params.flags, params.core_codec),
            );

            if err != SbrError::Ok {
                return Err(err);
            }

            for sbr_element_channel in self.sbr_channel.iter_mut() {
                // Create SbrDec.
                sbr_element_channel.sbr_dec.create(
                    params.flags,
                    params.codec_frame_size,
                    sbr_header.number_time_slots() * sbr_header.time_step(),
                );

                // Init frame data.
                sbr_element_channel.frame_data[0].init_sbr_frame_data(
                    &mut sbr_element_channel.prev_frame_data,
                    sbr_header.number_time_slots(),
                );
            }
        }

        // Create PsDec
        if params.num_sbr_elements == 1 {
            match params.core_codec {
                AudioObjectType::AotAacLc
                | AudioObjectType::AotSbr
                | AudioObjectType::AotErAacScal
                | AudioObjectType::AotPs => {
                    let init_error = if let Some(ps_dec) = self.parametric_stereo_dec.as_deref_mut()
                    {
                        // Re-initialize the existing PsDec instance.
                        ps_dec.init(params.codec_frame_size)
                    } else {
                        // Create a new PsDec instance and then initialize it.
                        let mut ps_dec = PsDec::new();
                        let err = ps_dec.init(params.codec_frame_size);
                        self.parametric_stereo_dec = Some(Box::new(ps_dec));
                        err
                    };

                    if init_error != SbrError::Ok {
                        // Deallocate instance if there is error.
                        self.parametric_stereo_dec = None;
                        return Err(SbrError::CreateError);
                    }
                }
                _ => (),
            };
        }

        // Init frame delay slot handling.
        self.use_frame_slot = 0;
        for i in 0..(SBRDEC_MAX_DELAY_FRAMES + 1) {
            self.use_header_slot[i] = i;
        }

        Ok(())
    }
    /// Initializes a SBR decoder element runtime instance. Must be called before decoding starts.
    ///
    /// # Parameters
    ///
    /// - `params`: SBR decoder parameters.
    /// - `params_requested`: Requested SBR decoder parameters.
    /// - `qmf_domain`: QMF domain.
    /// - `qmf_domain_params`: Requested QMF domain parameters.
    /// - `is_config_changed`: Flag, that enforces complete decoder reset.
    /// - `config_mode`: Table with MPEG-4 element Ids in canonical order.
    /// - `is_new_element`: Indicates if the SBR element to initilaise is new instance.
    ///
    /// # Return
    /// - `Result<bool, (SbrError, bool)>`
    // Returns
    // Ok(true): New or existing element is successfully initialised.
    // Ok(false): Early return case. New element is not initialised, thus not saved in memory.
    // Err(err, true): Element is invalid and should be destroyed and either not saved in memory
    // or removed from memory.
    // Err(err, false): New element is unsuccessfully initialised, thus it is not saved in memory.
    #[expect(clippy::too_many_arguments)]
    pub fn init(
        &mut self,
        params: &mut SbrDecoderParams,
        params_requested: &SbrDecoderParams,
        qmf_domain: &mut QmfDomain,
        qmf_domain_params: &mut QmfDomainParams,
        is_config_changed: &mut bool,
        config_mode: ReconfigState,
        is_new_element: bool,
    ) -> Result<bool, (SbrError, bool)> {
        let elements_start = params.num_sbr_elements;
        let channels_start = params.num_sbr_channels;

        // Check core codec AOT.
        if !is_core_codec_valid(params_requested.core_codec) {
            let mut is_free_mem = self.bail_from_init(
                params,
                params_requested.element_index,
                elements_start,
                channels_start,
                !is_new_element,
            );

            if self.num_channels == 0 {
                is_free_mem = true;
            }

            return Err((SbrError::UnsupportedConfig, is_free_mem));
        }

        if !params_requested.element_id.is_mp4_channel_element() {
            let mut is_free_mem = self.bail_from_init(
                params,
                params_requested.element_index,
                elements_start,
                channels_start,
                !is_new_element,
            );

            if self.num_channels == 0 {
                is_free_mem = true;
            }

            return Err((SbrError::UnsupportedConfig, is_free_mem));
        }

        let params_requested_sample_rate_out = if params_requested.sample_rate_out == 0 {
            true
        } else {
            params.sample_rate_out == params_requested.sample_rate_out
        };

        let params_requested_harmonic_sbr = if params_requested.harmonic_sbr == 2 {
            true
        } else {
            params.harmonic_sbr == params_requested.harmonic_sbr
        };

        let sbr_dec_reset = true;

        if params.sample_rate_in == params_requested.sample_rate_in
            && params.codec_frame_size == params_requested.codec_frame_size
            && params.core_codec == params_requested.core_codec
            && !is_new_element
            && self.element_id == params_requested.element_id
            && params_requested_sample_rate_out
            && params_requested_harmonic_sbr
            && sbr_dec_reset
        {
            // Nothing to do.
            return Ok(!is_new_element);
        } else if config_mode == ReconfigState::DetCfgChange {
            *is_config_changed = true;
        }

        // Reaching this point the SBR-decoder gets (re-)configured.
        // The flags field is used for all elements.
        self.reconfig(params, params_requested);

        if config_mode == ReconfigState::DetCfgChange {
            return Ok(!is_new_element);
        }

        params.sample_rate_in = params_requested.sample_rate_in;
        params.codec_frame_size = params_requested.codec_frame_size;
        params.core_codec = params_requested.core_codec;
        params.harmonic_sbr = params_requested.harmonic_sbr;
        params.downscale_factor = params_requested.downscale_factor;

        // Calculates the following paramters: flags, sample_rate_out, synthesis_downsample_factor.
        if let Err(err) = params.calc_sampling_parameters(params_requested) {
            let mut is_free_mem = self.bail_from_init(
                params,
                params_requested.element_index,
                elements_start,
                channels_start,
                !is_new_element,
            );

            if self.num_channels == 0 {
                is_free_mem = true;
            }

            return Err((err, is_free_mem));
        };

        // Init SBR elements.
        {
            let el_channels = params_requested.number_of_element_channels();

            if is_new_element {
                params.num_sbr_elements += 1;
            } else {
                params.num_sbr_channels -= self.num_channels as u8;
            }

            // Sanity check to avoid memory leaks.
            if el_channels < self.num_channels as u8 || (params.num_sbr_channels + el_channels) > 8
            {
                params.num_sbr_channels += self.num_channels as u8;

                let is_free_mem = self.bail_from_init(
                    params,
                    params_requested.element_index,
                    elements_start,
                    channels_start,
                    true,
                );
                return Err((SbrError::ParseError, is_free_mem));
            }

            // Save element ID for sanity checks and to have a fallback for concealment.
            self.element_id = params_requested.element_id;

            for ch in 0..usize::from(el_channels) {
                if self.sbr_channel.is_empty() || self.sbr_channel.len() - 1 < ch {
                    self.sbr_channel.push(SbrChannel::new());
                }

                self.sbr_channel[ch]
                    .sbr_dec
                    .sbr_drc_channel_mut()
                    .init_channel();

                params.num_sbr_channels += 1;
            }
            self.num_channels = self.sbr_channel.len();
        }

        params.is_ps_applied = false;

        // Clear error flags for all delay slots.
        self.frame_error_flag = [false; SBRDEC_MAX_DELAY_FRAMES + 1];

        match self.reset(params, qmf_domain, qmf_domain_params) {
            Ok(_) => Ok(true),
            Err(err) => {
                let is_free_mem = self.bail_from_init(
                    params,
                    params_requested.element_index,
                    elements_start,
                    channels_start,
                    true,
                );

                Err((err, is_free_mem))
            }
        }
    }

    /// Render one SBR element into time domain signal.
    ///
    /// # Parameters
    ///
    /// - `params`: SBR decoder parameters.
    /// - `qmf_domain`: QMF domain structure.
    /// - `input`: Time input buffer.
    /// - `time_data`: Time output buffer.
    /// - `map_descr`: Channel mapping descriptor.
    /// - `map_idx`: Mapping index.
    /// - `channel_idx`: Enumerating index of the SBR channel to be rendered.
    /// - `num_in_channels`: Number of channels from core coder.
    /// - `num_out_channels`: Number of output channels.
    /// - `is_ps_possible`: Flag indicating if PS is possible or not.
    ///
    /// # Return
    ///
    /// - `SbrError`
    #[expect(clippy::too_many_arguments)]
    pub fn decode(
        &mut self,
        params: &mut SbrDecoderParams,
        qmf_domain: &mut QmfDomain,
        input: Option<&[f32]>,
        time_data: Option<&mut [f32]>,
        map_descr: &ChannelMapDescriptor,
        map_idx: usize,
        channel_idx: u8,
        num_in_channels: u8,
        num_out_channels: &mut u8,
        is_ps_possible: bool,
    ) -> Result<(), SbrError> {
        let mut err;

        if params.flags.contains(SbrDecFlags::FLUSH) {
            if params.num_flushed_frames > params.num_delay_frames {
                // No valid SBR payload available, hence switch to upsampling (in all headers).
                for hdr_idx in 0..(SBRDEC_MAX_DELAY_FRAMES + 1) {
                    if self.sbr_header[hdr_idx].sync_state() > SyncState::Upsampling {
                        self.sbr_header[hdr_idx].set_sync_state(SyncState::Upsampling);
                    }
                }
            } else {
                // Move frame pointer to the next slot which is up to be decoded/applied next.
                self.use_frame_slot =
                    (self.use_frame_slot + 1) % (params.num_delay_frames as usize + 1);
            }
        }

        let stereo = self.element_id == ChannelElementId::Cpe;
        let frm_slot = self.use_frame_slot;
        let hdr_slot = self.use_header_slot[frm_slot];
        let is_frame_error = self.frame_error_flag[frm_slot];
        let sbr_header = &mut self.sbr_header[hdr_slot];
        let mut time_data_option = time_data;

        let time_data_size = if let Some(time_data) = time_data_option.as_deref() {
            time_data.len()
        } else {
            0
        };

        self.sbr_channel[LEFT].frame_data[frm_slot]
            .set_frame_error_flag(self.frame_error_flag[frm_slot]);

        if stereo {
            self.sbr_channel[RIGHT].frame_data[frm_slot]
                .set_frame_error_flag(self.frame_error_flag[frm_slot]);
        }

        // Prepare filterbank for upsampling if no valid bit stream data is available.
        if sbr_header.sync_state() == SyncState::NotInitialized {
            err = sbr_header.init_header_data(
                params.sample_rate_in,
                params.sample_rate_out,
                params.downscale_factor,
                params.codec_frame_size,
                params.flags,
                true,
            );

            if err != SbrError::Ok {
                return Err(err);
            }

            sbr_header.set_sync_state(SyncState::Upsampling);

            err = sbr_header.header_update(Status::NotPresent, params.flags);

            if err != SbrError::Ok {
                sbr_header.set_sync_state(SyncState::NotInitialized);
                return Err(err);
            }
        }

        // Reset.
        if sbr_header.header_needs_reset() {
            let apply_sbr_proc = (sbr_header.sync_state() == SyncState::Active)
                || (!is_frame_error && sbr_header.sync_state() == SyncState::Header);

            let sbr_pitch_in_bins = self.sbr_channel[LEFT].frame_data[0].sbr_pitch_in_bins();
            let sbr_patching_mode = self.sbr_channel[LEFT].frame_data[0].sbr_patching_mode();

            for ch in 0..self.num_channels {
                let curr_sbr_channel = &mut self.sbr_channel[ch];

                let is_channel_error = curr_sbr_channel.frame_data[0].frame_error_flag();
                let apply_sbr_proc_in_ch = sbr_header.sync_state() == SyncState::Active
                    || (!is_channel_error && sbr_header.sync_state() == SyncState::Header);

                // In case of mono, only the left channel has been initialized with the
                // HBE parameters. Use the same HBE parameters for the initialization of
                // the right channel.
                if !stereo && ch == 1 {
                    curr_sbr_channel.frame_data[0].set_sbr_pitch_in_bins(sbr_pitch_in_bins);
                    curr_sbr_channel.frame_data[0].set_sbr_patching_mode(sbr_patching_mode);
                }

                if curr_sbr_channel
                    .sbr_dec
                    .reset(
                        qmf_domain,
                        sbr_header,
                        &curr_sbr_channel.prev_frame_data,
                        curr_sbr_channel.frame_data[ch].sbr_patching_mode(),
                        self.transposer_settings.overlap_size() as usize,
                        params.flags,
                        apply_sbr_proc_in_ch,
                    )
                    .is_err()
                {
                    sbr_header.set_sync_state(SyncState::Upsampling);
                }
            }

            {
                let fbd = sbr_header.freq_band_data();
                let mtl = usize::from(fbd.num_master);
                let ntl = usize::from(fbd.num_freq_bands_noise());

                // Initialize transposer and limiter.
                if self.transposer_settings.reset(
                    &fbd.master_band_table()[..=mtl],
                    &fbd.freq_band_table_noise()[..=ntl],
                    sbr_header.processing_sampling_rate(),
                    fbd.start_subband,
                    fbd.end_subband,
                ) != SbrError::Ok
                {
                    sbr_header.set_sync_state(SyncState::Upsampling);
                }
            }

            for ch in 0..self.num_channels {
                let curr_sbr_channel = &mut self.sbr_channel[ch];
                let is_channel_error = curr_sbr_channel.frame_data[0].frame_error_flag();
                let apply_sbr_proc_in_ch = sbr_header.sync_state() == SyncState::Active
                    || (!is_channel_error && sbr_header.sync_state() == SyncState::Header);

                let x_over_qmf =
                    if params.flags.contains(SbrDecFlags::USAC_HBE_LP) || !apply_sbr_proc_in_ch {
                        None
                    } else {
                        curr_sbr_channel
                            .sbr_dec
                            .hbe()
                            .map(|hbe| *(hbe.x_over_band()))
                    };

                let is_b41sbr = match curr_sbr_channel.sbr_dec.hbe() {
                    Some(hbe) => hbe.quad_rate(),
                    None => false,
                };

                let nfb_sbr_low = usize::from(sbr_header.freq_band_data().num_freq_bands_sbr(0));

                let patching_mode = curr_sbr_channel.frame_data[0].sbr_patching_mode();
                if curr_sbr_channel
                    .sbr_dec
                    .calc_envelope_mut()
                    .reset_limiter_bands(
                        if apply_sbr_proc_in_ch {
                            Some(self.transposer_settings.patch_param())
                        } else {
                            None
                        },
                        &sbr_header.freq_band_data().freq_band_table_low()[..nfb_sbr_low + 1],
                        nfb_sbr_low,
                        self.transposer_settings.num_of_patches().into(),
                        sbr_header.bitstream().limiter_bands().into(),
                        patching_mode,
                        &x_over_qmf,
                        is_b41sbr,
                    )
                    .is_err()
                {
                    sbr_header.set_sync_state(SyncState::Upsampling);
                }
            }

            if apply_sbr_proc {
                sbr_header.set_header_needs_reset(false);
            }
        }

        // Decoding.
        if sbr_header.sync_state() == SyncState::Active
            || (sbr_header.sync_state() == SyncState::Header && !is_frame_error)
        {
            let (sbr_ch_left, sbr_ch_right) = self.sbr_channel.split_at_mut(RIGHT);
            let (data_right, prev_data_right) = if stereo {
                (
                    Some(&mut sbr_ch_right[0].frame_data[frm_slot]),
                    Some(&mut sbr_ch_right[0].prev_frame_data),
                )
            } else {
                (None, None)
            };

            sbr_ch_left[0].frame_data[frm_slot].decode_sbr_data(
                &mut sbr_ch_left[0].prev_frame_data,
                data_right,
                prev_data_right,
                sbr_header,
            );

            // Now we have a full parameter set and can do parameter based concealment instead of
            // plain upsampling.
            sbr_header.set_sync_state(SyncState::Active);
        }

        let channels_num = if is_ps_possible {
            2.max(num_in_channels)
        } else {
            num_in_channels
        };

        let tdl_calc = usize::from(sbr_header.number_time_slots())
            * usize::from(sbr_header.time_step())
            * usize::from(qmf_domain.num_bands_synthesis())
            * usize::from(channels_num);

        if !params.flags.contains(SbrDecFlags::SKIP_QMF_SYN) && time_data_size < tdl_calc {
            return Err(SbrError::OutputBufferTooSmall);
        }

        params.flags.remove(SbrDecFlags::PS_DECODED);

        // Decode PS data if available.
        if is_ps_possible && sbr_header.sync_state() == SyncState::Active {
            if let Some(parametric_stereo_dec) = self.parametric_stereo_dec.as_deref_mut() {
                let apply_ps = parametric_stereo_dec.bs_data_mut().decode_ps_data(
                    self.sbr_channel[LEFT].frame_data[frm_slot].frame_error_flag(),
                    self.use_frame_slot,
                );

                params.flags.insert(if apply_ps {
                    SbrDecFlags::PS_DECODED
                } else {
                    SbrDecFlags::empty()
                });
            }
        }

        if !params.flags.contains(SbrDecFlags::SYNTAX_USAC)
            && params.flags.contains(SbrDecFlags::USAC_HARMONICSBR)
        {
            let left_esbr_mode = self.sbr_channel[LEFT].sbr_dec.esbr_mode();

            // Both channels must have the same esbr_mode.
            if stereo {
                let right_esbr_mode = self.sbr_channel[RIGHT].sbr_dec.esbr_mode();
                debug_assert!(left_esbr_mode == right_esbr_mode);
            }

            // Adapt flags according to the eSbr mode.
            if left_esbr_mode == SbrDecEsbrMode::Off {
                params.flags.insert(SbrDecFlags::USAC_HBE_LP);
            } else {
                params.flags.remove(SbrDecFlags::USAC_HBE_LP);
            }
        }

        let mut n_ch = if stereo { 2_usize } else { 1 };

        for ch in 0..n_ch {
            let curr_sbr_channel = &mut self.sbr_channel[ch];
            let frame_data = &curr_sbr_channel.frame_data[frm_slot];

            let offset = map_descr.get_map_value(channel_idx + ch as u8, map_idx);
            let offset_block: usize = offset as usize * params.codec_frame_size as usize;

            // Apply SBR analysis.
            let time_in = if let Some(input_data) = input {
                Some(&input_data[offset_block..offset_block + params.codec_frame_size as usize])
            } else {
                None
            };

            curr_sbr_channel.sbr_dec.analysis(
                qmf_domain,
                time_in,
                frame_data.sbr_patching_mode(),
                params.flags,
            );

            // Apply SBR transposing.
            curr_sbr_channel.sbr_dec.transposition(
                qmf_domain,
                sbr_header,
                frame_data,
                &curr_sbr_channel.prev_frame_data,
                &mut self.transposer_settings,
                params.flags,
                sbr_header.sync_state() == SyncState::Active,
            );

            // Update previous SBR FrameData.
            frame_data.update_sbr_prev_frame_data(
                &mut curr_sbr_channel.prev_frame_data,
                sbr_header,
                sbr_header.sync_state() == SyncState::Active,
            );
        }

        // Parametric stereo processing.
        {
            if !params.flags.contains(SbrDecFlags::PS_DECODED) {
                if !params.flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
                    params.is_ps_applied = false;
                }
            } else {
                // PS decoding stores QMF slots of right channel in pWorkBuffer.
                if let Some(ps_dec) = self.parametric_stereo_dec.as_deref_mut() {
                    let (sbr_ch_left, sbr_ch_right) = self.sbr_channel.split_at_mut(RIGHT);

                    sbr_ch_right[0].sbr_dec.parametric_stereo(
                        &sbr_ch_left[0].sbr_dec,
                        qmf_domain,
                        ps_dec,
                        params.is_ps_applied,
                    );
                }

                n_ch = 2;
                params.is_ps_applied = true;
            }
        }

        for ch in 0..n_ch {
            let curr_sbr_channel = &mut self.sbr_channel[ch];

            // Apply DRC.
            curr_sbr_channel.sbr_dec.drc_apply(qmf_domain, params.flags);

            let offset = map_descr.get_map_value(channel_idx + ch as u8, map_idx);
            let filter_bank_frame_size = qmf_domain
                .get_filter_bank(
                    curr_sbr_channel.sbr_dec.ch_qmf_domain() as usize,
                    QmfDomainSelect::QmfDomainOut,
                )
                .unwrap()
                .filter_bank_frame_size();
            let offset_deinterleave = filter_bank_frame_size * offset as usize;

            // Apply QMF synthesis.
            let time_out = time_data_option.as_mut().map(|time_data| {
                &mut time_data[offset_deinterleave..offset_deinterleave + filter_bank_frame_size]
            });
            curr_sbr_channel.sbr_dec.synthesis(
                qmf_domain,
                time_out,
                &mut sbr_header.freq_band_data_as_mut().old_end_subband,
                params.flags,
            );
        }

        // Save PS status for next run.
        if let Some(parametric_stereo_dec) = self.parametric_stereo_dec.as_deref_mut() {
            parametric_stereo_dec
                .bs_data_mut()
                .set_decoded_prev(params.flags.contains(SbrDecFlags::PS_DECODED));
        }

        if is_ps_possible && !params.flags.contains(SbrDecFlags::SKIP_QMF_SYN) {
            if !params.flags.contains(SbrDecFlags::PS_DECODED) {
                // A decoder which is able to decode PS has to produce a stereo output
                // even if no PS data is available. Copy left channel to right channel.
                let mut copy_frame_size = usize::from(params.codec_frame_size)
                    * usize::from(
                        qmf_domain
                            .get_filter_bank(0, QmfDomainSelect::QmfDomainOut)
                            .unwrap()
                            .num_subbands(),
                    );
                copy_frame_size /= usize::from(
                    qmf_domain
                        .get_filter_bank(0, QmfDomainSelect::QmfDomainIn)
                        .unwrap()
                        .num_subbands(),
                );

                if let Some(time_data) = time_data_option {
                    time_data.copy_within(0..copy_frame_size, copy_frame_size);
                }
            }
            // Output minimum two channels when PS is enabled.
            *num_out_channels = 2;
        }

        Ok(())
    }
}

/// Determines QMF flags.
///
/// # Parameters
///
/// - `flags`: SBR decoder flags.
/// - `core_codec`: Core codec audio object type.
///
/// # Return
///
/// - `qmf_flags`: QmfFlags.
fn qmf_flags(flags: u32, core_codec: AudioObjectType) -> QmfFlags {
    let mut qmf_flags = QmfFlags::empty();

    if core_codec == AudioObjectType::AotErAacEld {
        if (flags & SBRDEC_LD_MPS_QMF) != 0 {
            qmf_flags.insert(QmfFlags::MPSLDFB);
        } else {
            qmf_flags.insert(QmfFlags::CLDFB);
        }
    }

    qmf_flags
}

/// Determines number of overlap slots.
///
/// # Parameters
///
/// - `flags`: SBR decoder flags.
/// - `core_codec`: Core codec audio object type.
///
/// # Return
///
/// Number of QMF overlap time slots.
fn number_of_qmf_overlap_time_slots(flags: SbrDecFlags, core_codec: AudioObjectType) -> u8 {
    if core_codec == AudioObjectType::AotErAacEld {
        0
    } else if flags.contains(SbrDecFlags::QUAD_RATE) {
        SBR_QUAD_RATE_OVERLAP_SIZE
    } else {
        SBR_DUAL_RATE_OVERLAP_SIZE
    }
}

/// Determines whether the given core codec AOT can be processed.
///
/// # Parameters
///
/// - `core_codec`: Core codec audio object type.
///
/// # Return
///
/// - `true` if SBR can be processed, `false` otherwise.
fn is_core_codec_valid(core_codec: AudioObjectType) -> bool {
    matches!(
        core_codec,
        AudioObjectType::AotAacLc
            | AudioObjectType::AotSbr
            | AudioObjectType::AotErAacScal
            | AudioObjectType::AotPs
            | AudioObjectType::AotErAacEld
            | AudioObjectType::AotUsac
    )
}
