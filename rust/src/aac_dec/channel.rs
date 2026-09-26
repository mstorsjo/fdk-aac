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
//! Decoding of one audio channel

use super::{
    block,
    channel_info::IcsInfo,
    conceal::{AacDecoderRenderMode, ConcealmentData},
    constants,
    drc::AacDrcData,
    error_codes::AacDecoderError,
    hcr::HcrData,
    huff_dec::HuffmanDecoder,
    intensity, inverse_quantization,
    lpd::{common::LpdMode, constants as lpd_constants, LpdData, UsacCoremode},
    ms_stereo::JointStereoData,
    noise_filling,
    pns::{PnsData, PnsInterChannelData},
    pulse_data::PulseData,
    rvlc::RvlcData,
    sr_info::SamplingRateInfo,
    tns::TnsData,
    utils::{self},
};
use crate::arith_coding::arith_dec::ArithDecoderData;
use crate::common::{
    aot::AudioObjectType,
    bs_element_id::ChannelElementId,
    bs_syntax::{ElementList, RdbId},
    enums::AudioChannel,
    flags::{ACFlags, ChannelFlags},
    ld_filter_bank::{self},
    mdct::Mdct,
};
use crate::tp_dec::TransportDec;
use itertools::izip;
use std::mem::size_of;
use zerocopy::FromBytes;

const WB_SECTION_SIZE: usize = constants::MAX_FRAMESIZE * 2 * size_of::<u32>();
const SYNTH_BUF_LENGTH: usize =
    lpd_constants::PIT_MAX_MAX + lpd_constants::SYN_DELAY + lpd_constants::L_FRAME_PLUS;

const ADTS_ERROR_CHECK_REGION1_CRC_BITS: i32 = 192;
const ADTS_ERROR_CHECK_REGION2_CRC_BITS: i32 = 128;

use bitflags::bitflags;

bitflags! {
    /// Channel element states
    #[repr(C)]
    #[derive(Copy, Clone, Default, Debug)]
    pub struct ElState: u32 {
        const READ = 1;
        const DECODED = 2;
    }
}

#[repr(C, align(4))]
#[derive(Debug)]
/// `CommonChannelData` structure.
pub struct CommonChannelData {
    // The scratch_work_buffer is re-used as a [f32] and an [i16] buffer. Therefore, always make
    // sure that the buffer is memory aligned with std::mem::align_of::<f32>() and
    // std::mem::align_of::<i16>()
    scratch_work_buffer: [u8; WB_SECTION_SIZE],
    pub pulse_data: Option<Vec<PulseData>>,
    pub er_hcr_data: Option<Box<HcrData>>,
    sr_info: SamplingRateInfo,
}

impl Default for CommonChannelData {
    // Default trait.
    fn default() -> Self {
        Self {
            scratch_work_buffer: [0_u8; WB_SECTION_SIZE],
            pulse_data: None,
            er_hcr_data: None,
            sr_info: SamplingRateInfo::default(),
        }
    }
}

impl CommonChannelData {
    /// Creates a new `CommonChannelData` instance.
    pub fn new() -> Self {
        Default::default()
    }

    /// Initializes the `CommonChannelData` instance.
    ///
    /// # Parameters
    ///
    /// - `sr_info`: `SamplingRateInfo` instance with valid internal data.
    /// - `is_sce_only`: Flag to indicates (SCE) single channel element only.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub fn init(&mut self, sr_info: &SamplingRateInfo, is_sce_only: bool, ac_flags: ACFlags) {
        let num_element_channels = if is_sce_only { 1 } else { 2 };

        if ac_flags.contains(ACFlags::ER) {
            self.er_hcr_data = Some(Box::new(HcrData::new(num_element_channels)));
        }

        if !ac_flags.intersects(ACFlags::USAC | ACFlags::ELD | ACFlags::SCALABLE) {
            self.pulse_data = Some(vec![PulseData::default(); num_element_channels]);
        }

        self.sr_info.clone_from(sr_info);
    }
}

#[repr(C)]
#[derive(Debug)]
/// `AacDecoderChannelInfo` structure.
pub struct ChannelInfo {
    pub ics_info: IcsInfo,
    // -- Common bit stream data -- //
    tns_data: TnsData,
    arco_data: Option<Box<ArithDecoderData>>,
    lpd_data: Option<Box<LpdData>>,
    pns_data: Option<Box<PnsData>>,
    rvlc_data: Option<Box<RvlcData>>,
    inverse_mdct: Option<Box<Mdct>>,
    eld_overlap_buffer: Vec<f32>,
    // Spectral scale factors for each sfb in each  window.
    scale_factor: [i16; constants::MAX_WINS_X_SFBS],
    // section data: codebook for each window and sfb.
    code_book: [u8; constants::MAX_WINS_X_SFBS],
    // Output signal rendering mode
    render_mode: AacDecoderRenderMode,
    // seed value for USAC noise filling random generator
    noise_filling_random_seed: u32,
    fd_noise_level_and_offset: u8,
    global_gain: u8,
    is_usac_tns_active: bool,
    is_usac_tns_on_lr: bool,
}

impl ChannelInfo {
    /// Creates a new `AacDecoderChannelInfo` instance.
    pub fn new() -> Self {
        ChannelInfo::default()
    }
}

impl Default for ChannelInfo {
    // Default trait.
    fn default() -> Self {
        Self {
            scale_factor: [0_i16; constants::MAX_WINS_X_SFBS],
            code_book: [0_u8; constants::MAX_WINS_X_SFBS],
            ics_info: Default::default(),
            render_mode: AacDecoderRenderMode::default(),
            global_gain: Default::default(),
            arco_data: None,
            lpd_data: None,
            noise_filling_random_seed: Default::default(),
            fd_noise_level_and_offset: Default::default(),
            is_usac_tns_active: false,
            is_usac_tns_on_lr: false,
            tns_data: Default::default(),
            pns_data: None,
            rvlc_data: None,
            eld_overlap_buffer: Default::default(),
            inverse_mdct: None,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// `ChannelElement` structure.
pub struct ChannelElement {
    js_data: Option<Box<JointStereoData>>,
    pns_inter_channel_data: Option<Box<PnsInterChannelData>>,
    channel_info: Vec<ChannelInfo>,
    prev_spectral_data: Vec<f32>,
    common_window: bool,
    num_channels: u8,
    element_instance_tag: u8,
    element_type: ChannelElementId,
    el_flags: ChannelFlags,
    state_flags: ElState,
}

impl ChannelElement {
    /// Creates a new `ChannelElement` instance.
    pub fn new() -> Self {
        ChannelElement::default()
    }

    /// Initializes the `ChannelElement` instance.
    ///
    /// # Parameters
    ///
    /// - `sr_info`: `SamplingRateInfo` instance with valid internal data.
    /// - `element_type`: MPEG-4 audio channel type (Element ID).
    /// - `element_instance_tag`: Tag value for element instance.
    /// - `frame_length`: Length used for allocating internal memory of `ChannelElement`.
    /// - `element_flags`: Audio codec element specific flag.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub fn init(
        &mut self,
        sr_info: &SamplingRateInfo,
        element_type: ChannelElementId,
        element_instance_tag: u8,
        frame_length: usize,
        element_flags: ChannelFlags,
        ac_flags: ACFlags,
    ) {
        let num_channels = Self::get_num_channels(element_type, ac_flags);

        if !ac_flags.contains(ACFlags::USAC) {
            self.pns_inter_channel_data = Some(Box::<PnsInterChannelData>::default());
        }

        if num_channels > 1 {
            let is_cp_possible = element_flags.contains(ChannelFlags::USAC_CP_POSSIBLE);
            self.js_data = Some(Box::new(JointStereoData::new(is_cp_possible)));
        }

        // Allocate memory for channel_info.
        self.channel_info = Vec::new();
        for _ch in 0..num_channels {
            self.channel_info.push(ChannelInfo::new());
        }

        for ch in 0..num_channels {
            let channel_info = &mut self.channel_info[ch];
            if ac_flags.contains(ACFlags::USAC) {
                channel_info.arco_data = Some(Box::new(ArithDecoderData::new()));
                channel_info.lpd_data = Some(Box::new(LpdData::new(sr_info.sampling_rate())));
            } else {
                channel_info.pns_data = Some(Box::new(PnsData::new()));
            }

            if ac_flags.contains(ACFlags::ER) {
                channel_info.rvlc_data = Some(Box::new(RvlcData::new()));
            }

            if ac_flags.contains(ACFlags::ELD) {
                channel_info.eld_overlap_buffer = vec![0.0_f32; frame_length + (frame_length / 2)];
                channel_info.render_mode = AacDecoderRenderMode::EldFb;
            } else {
                assert!(frame_length < (1 << 16));
                let overlap_max_size = frame_length / 2;
                channel_info.inverse_mdct = Some(Box::new(Mdct::new(overlap_max_size).unwrap()));
                channel_info.render_mode = AacDecoderRenderMode::Imdct;
            }
            channel_info
                .tns_data
                .init(sr_info.sampling_rate_index(), ac_flags);
            channel_info.noise_filling_random_seed = if ch == 0 { 0x3039_u32 } else { 0x10932_u32 };
        }

        self.prev_spectral_data = vec![0.0_f32; num_channels * frame_length];
        self.num_channels = num_channels as u8;
        self.element_type = element_type;
        self.element_instance_tag = element_instance_tag;
        self.el_flags = element_flags;
    }

    /// Sets MPEG-4 audio channel type (Element ID).
    pub fn set_element_type(&mut self, element_type: ChannelElementId) {
        self.element_type = element_type;
    }

    /// Clears all element flags.
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::channel::ChannelElement;
    /// use aac::common::flags::ChannelFlags;
    ///
    /// let mut ch_el = ChannelElement::new();
    /// ch_el.insert_flags(ChannelFlags::USAC_NOISE | ChannelFlags::USAC_PVC);
    /// ch_el.clear_flags();
    /// assert!(ChannelFlags::empty() == ch_el.el_flags());
    /// ```
    pub fn clear_flags(&mut self) {
        self.el_flags = ChannelFlags::empty();
    }

    /// Sets one or multiple element flags.
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::channel::ChannelElement;
    /// use aac::common::flags::ChannelFlags;
    ///
    /// let mut ch_el = ChannelElement::new();
    /// ch_el.insert_flags(ChannelFlags::USAC_NOISE | ChannelFlags::USAC_PVC);
    /// assert!((ChannelFlags::USAC_NOISE | ChannelFlags::USAC_PVC) == ch_el.el_flags());
    /// ```
    pub fn insert_flags(&mut self, flags_to_set: ChannelFlags) {
        self.el_flags.insert(flags_to_set);
    }

    /// Unsets one or multiple element flags.
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::channel::ChannelElement;
    /// use aac::common::flags::ChannelFlags;
    ///
    /// let mut ch_el = ChannelElement::new();
    /// ch_el.insert_flags(ChannelFlags::USAC_NOISE | ChannelFlags::USAC_PVC);
    /// ch_el.remove_flags(ChannelFlags::USAC_NOISE);
    /// assert!(ChannelFlags::USAC_PVC == ch_el.el_flags());
    /// ```
    pub fn remove_flags(&mut self, flags_to_unset: ChannelFlags) {
        self.el_flags.remove(flags_to_unset);
    }

    /// Sets number of channels.
    pub fn set_num_channels(&mut self, num_channels: u8) {
        self.num_channels = num_channels;
    }

    /// Returns MPEG-4 audio channel type (Element ID).
    pub fn element_type(&self) -> ChannelElementId {
        self.element_type
    }

    /// Returns audio codec element specific flags.
    pub fn el_flags(&self) -> ChannelFlags {
        self.el_flags
    }

    /// Returns number of channels.
    pub fn num_channels(&self) -> u8 {
        self.num_channels
    }

    /// Returns element instance tag value.
    pub fn element_instance_tag(&self) -> u8 {
        self.element_instance_tag
    }

    /// Returns state flags of channel element.
    pub fn state_flags(&self) -> ElState {
        self.state_flags
    }

    /// Returns element channel infos.
    pub fn get_element_channel_infos(&self) -> &Vec<ChannelInfo> {
        &self.channel_info
    }

    /// Returns number of channels for a given `element_type` and `usac_stereo_config_index`.
    pub(super) fn get_num_channels(element_type: ChannelElementId, ac_flags: ACFlags) -> usize {
        match element_type {
            ChannelElementId::Cpe => 2,
            ChannelElementId::Sce
            | ChannelElementId::Lfe
            | ChannelElementId::Cce
            | ChannelElementId::UsacSce
            | ChannelElementId::UsacLfe => 1,
            ChannelElementId::UsacCpe => {
                if ac_flags.contains(ACFlags::USAC_SCFGI1) {
                    1
                } else {
                    2
                }
            }
            ChannelElementId::Dse
            | ChannelElementId::Pce
            | ChannelElementId::Fil
            | ChannelElementId::End
            | ChannelElementId::Ext
            | ChannelElementId::UsacExt
            | ChannelElementId::Last
            | ChannelElementId::None => 0,
        }
    }

    /// Resets `ChannelElement` instance.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub fn reset(&mut self, ac_flags: ACFlags) {
        for ci in self
            .channel_info
            .iter_mut()
            .take(self.num_channels as usize)
        {
            // Reset TNS Data.
            ci.tns_data.reset();
            if !ac_flags.contains(ACFlags::USAC) {
                if let Some(pns_data) = ci.pns_data.as_deref_mut() {
                    pns_data.init();
                }
            }

            // Reset LPD Data.
            if ac_flags.contains(ACFlags::USAC) {
                if let Some(lpd_data) = ci.lpd_data.as_deref_mut() {
                    lpd_data.reset();
                }
            }

            // Reset RVLC Data.
            if ac_flags.contains(ACFlags::ER) && !ac_flags.contains(ACFlags::ER_RVLC) {
                if let Some(rvlc_data) = ci.rvlc_data.as_deref_mut() {
                    rvlc_data.reset();
                }
            }
        }

        self.common_window =
            ac_flags.intersects(ACFlags::ELD | ACFlags::SCALABLE) && (self.num_channels == 2);

        if self.num_channels > 1 {
            // Reset ms_stereo data.
            if let Some(js_data) = self.js_data.as_deref_mut() {
                js_data.reset();
            }
        }

        if !ac_flags.contains(ACFlags::USAC) {
            self.channel_info[0].is_usac_tns_active = false;
            self.channel_info[0].is_usac_tns_on_lr = false;
        }

        self.state_flags = ElState::empty();
    }

    /// Reads channel element data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `common_channel_data`: `CommonChannelData` instance with valid data.
    /// - `tp_dec`: `TransportDec` instance with valid data.
    /// - `spectral_data`: Spectral data buffer to read from bitstream.
    /// - `spectral_length`: Length of Spectral data buffer and `frame_length` in case of `CCE`
    ///   parsing.
    /// - `aot`: Type of `AudioObjectType`.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub fn read(
        &mut self,
        common_channel_data: &mut CommonChannelData,
        tp_dec: &mut TransportDec,
        spectral_data: Option<&mut [f32]>,
        spectral_length: usize,
        aot: AudioObjectType,
        ac_flags: ACFlags,
    ) -> Result<(), AacDecoderError> {
        let mut error = AacDecoderError::Ok;
        let mut render_mode = AacDecoderRenderMode::Invalid;
        let mut iterate_bs_elements = true;
        let mut ind_sw_cce_flag = false;
        let mut num_gain_element_lists = 0_usize;
        let (mut crc_reg1, mut crc_reg2) = (-1, -1);

        // Get bitstream element list.
        let element_list_option =
            ElementList::get_bitstream_element_list(aot, self.num_channels, self.el_flags);

        if let Some(mut element_list) = element_list_option {
            let mut list_id = element_list.id();
            let mut list_id_index: i32 = 0;
            let mut decision_bit = 0;
            let mut channel = 0;

            let frame_length = if spectral_data.is_some() {
                spectral_length / self.num_channels() as usize
            } else {
                spectral_length // CASE: CCE parsing.
            };

            let mut spectral_data_option = spectral_data;

            let quantized_spectrum =
                <[i16]>::mut_from_bytes(&mut common_channel_data.scratch_work_buffer)
                    .expect("Cannot interpret the scratch buffer as i16 slice");

            let mut is_bail_set = false;
            'element_iterate_loop: loop {
                let bs = &mut tp_dec.bs;
                let rdb_id = list_id[list_id_index as usize];

                match rdb_id {
                    RdbId::ElementInstanceTag => {
                        if self.element_instance_tag == 255 {
                            self.element_instance_tag = bs.read(4) as u8;
                        } else if self.element_instance_tag != bs.read(4) as u8 {
                            error = AacDecoderError::ParseError;
                        }
                    }

                    RdbId::CommonWindow => {
                        decision_bit = bs.read_bit() as usize;
                        self.common_window = decision_bit != 0;
                    }
                    RdbId::IcsInfo => {
                        // Read individual channel info.
                        error = self.channel_info[channel].ics_info.read(
                            bs,
                            &common_channel_data.sr_info,
                            ac_flags,
                        );

                        if self.el_flags.contains(ChannelFlags::LFE)
                            && !self.channel_info[channel].ics_info.is_long_block()
                        {
                            error = AacDecoderError::ParseError;
                        } else if self.num_channels == 2 && self.common_window {
                            self.channel_info[usize::from(AudioChannel::Right)].ics_info =
                                self.channel_info[usize::from(AudioChannel::Left)].ics_info;
                        }
                    }
                    RdbId::CommonMaxSfb => {
                        if bs.read_bit() == 0 {
                            error = self.channel_info[usize::from(AudioChannel::Right)]
                                .ics_info
                                .read_max_sfb(bs, &common_channel_data.sr_info);
                        }
                    }
                    RdbId::LtpDataPresent => {
                        if bs.read_bit() != 0 {
                            error = AacDecoderError::UnsupportedPrediction;
                        }
                    }
                    RdbId::Ms => {
                        if let Some(js_data) = self.js_data.as_deref_mut() {
                            let left_max_sfb = self.channel_info[usize::from(AudioChannel::Left)]
                                .ics_info
                                .max_sf_bands();
                            let right_max_sfb = self.channel_info[usize::from(AudioChannel::Right)]
                                .ics_info
                                .max_sf_bands();
                            if js_data.read(
                                bs,
                                &self.channel_info[usize::from(AudioChannel::Left)].ics_info,
                                left_max_sfb.max(right_max_sfb),
                                ac_flags,
                            ) != 0
                            {
                                error = AacDecoderError::ParseError;
                            }
                        }
                    }
                    RdbId::GlobalGain => {
                        self.channel_info[channel].global_gain = bs.read(8) as u8;
                    }
                    RdbId::SectionData => {
                        let ch_info = &mut self.channel_info[channel];
                        error = block::read_section_data(
                            bs,
                            &ch_info.ics_info,
                            if let Some(hcr_data) = common_channel_data.er_hcr_data.as_deref_mut() {
                                Some(hcr_data)
                            } else {
                                None
                            },
                            &mut ch_info.code_book,
                            channel,
                            self.common_window,
                            ac_flags,
                        );
                    }
                    RdbId::ScaleFactorDataUsac | RdbId::ScaleFactorData => {
                        let ch_info = &mut self.channel_info[channel];
                        if rdb_id == RdbId::ScaleFactorDataUsac {
                            // Set active sfb codebook indexes to HCB_ESC to make them "active".
                            block::init_code_book_data(&ch_info.ics_info, &mut ch_info.code_book);
                        }

                        if ac_flags.contains(ACFlags::ER_RVLC) {
                            // Read RVLC data from bitstream (error sens. cat. 1).
                            if let Some(rvlc_data) = ch_info.rvlc_data.as_deref_mut() {
                                rvlc_data.read(bs, &ch_info.ics_info, &ch_info.code_book);
                            }
                        } else {
                            error = block::read_scalefactor_data(
                                bs,
                                &ch_info.ics_info,
                                ch_info.global_gain.into(),
                                &ch_info.code_book,
                                &mut ch_info.scale_factor,
                                if let Some(pns_data) = ch_info.pns_data.as_deref_mut() {
                                    Some(pns_data)
                                } else {
                                    None
                                },
                                ac_flags,
                            );
                        }
                    }
                    RdbId::Pulse => {
                        if let Some(pulse_data_vec) = common_channel_data.pulse_data.as_deref_mut()
                        {
                            error = pulse_data_vec[channel]
                                .read(bs, &self.channel_info[channel].ics_info);
                        }
                    }
                    RdbId::TnsDataPresent => {
                        let tns = &mut self.channel_info[channel].tns_data;
                        tns.read_datapresent_flag(bs);
                        if self.el_flags.contains(ChannelFlags::LFE) && tns.is_data_present() {
                            error = AacDecoderError::ParseError;
                        }
                    }
                    RdbId::TnsData => {
                        let ch_info = &mut self.channel_info[channel];
                        error = ch_info.tns_data.read(bs, &ch_info.ics_info, ac_flags);
                    }
                    RdbId::GainControlData => {
                        // Do nothing.
                    }
                    RdbId::GainControlDataPresent => {
                        if bs.read_bit() != 0 {
                            error = AacDecoderError::UnsupportedGainControlData;
                        }
                    }
                    RdbId::TwData => {
                        // Do nothing.
                    }
                    RdbId::CommonTw => {
                        // Do nothing.
                    }
                    RdbId::TnsDataPresentUsac => {
                        if self.channel_info[usize::from(AudioChannel::Left)].is_usac_tns_active {
                            let (left_channel, right_channel) = self.channel_info
                                [..=usize::from(AudioChannel::Right)]
                                .split_at_mut(usize::from(AudioChannel::Right));
                            left_channel[0].tns_data.read_datapresent_usac(
                                &mut right_channel[0].tns_data,
                                bs,
                                &left_channel[0].ics_info,
                                &mut left_channel[0].is_usac_tns_on_lr,
                                ac_flags,
                                self.common_window,
                            );
                        } else {
                            self.channel_info[usize::from(AudioChannel::Left)].is_usac_tns_on_lr =
                                true;
                        }
                    }
                    RdbId::CoreMode => {
                        decision_bit = bs.read_bit() as usize;
                        if let Some(lpd_data) = self.channel_info[channel].lpd_data.as_deref_mut() {
                            // lpd_data - set core_mode
                            lpd_data.lpd_info.core_mode = decision_bit as u8;
                        }
                        if channel == 1 {
                            if let (Some(lpd_left), Some(lpd_right)) = (
                                self.channel_info[usize::from(AudioChannel::Left)]
                                    .lpd_data
                                    .as_deref(),
                                self.channel_info[usize::from(AudioChannel::Right)]
                                    .lpd_data
                                    .as_deref(),
                            ) {
                                if lpd_left.lpd_info.core_mode != 0
                                    || lpd_right.lpd_info.core_mode != 0
                                {
                                    self.common_window = false;
                                }
                            }
                        }
                    }
                    RdbId::TnsActive => {
                        self.channel_info[usize::from(AudioChannel::Left)].is_usac_tns_active =
                            bs.read_bit() != 0;
                    }
                    RdbId::Noise => {
                        if self.el_flags.contains(ChannelFlags::USAC_NOISE) {
                            // Noise level.
                            self.channel_info[channel].fd_noise_level_and_offset = bs.read(8) as u8;
                        }
                    }
                    RdbId::LpdChannelStream => {
                        let ch_info = &mut self.channel_info[channel];
                        if let (Some(lpd_data), Some(arco_data), Some(spectral_data)) = (
                            ch_info.lpd_data.as_deref_mut(),
                            ch_info.arco_data.as_deref_mut(),
                            spectral_data_option.as_mut(),
                        ) {
                            error = lpd_data.read(
                                bs,
                                arco_data,
                                &mut spectral_data
                                    [channel * frame_length..(channel + 1) * frame_length],
                                &mut quantized_spectrum[..frame_length],
                                ac_flags,
                            );
                        }
                        render_mode = AacDecoderRenderMode::Lpd;
                    }
                    RdbId::FacData => {
                        let is_fac_data_present = bs.read_bit() != 0;
                        if is_fac_data_present {
                            if self.el_flags.contains(ChannelFlags::LFE) {
                                error = AacDecoderError::ParseError;
                            } else {
                                // FAC data present, this frame is FD, so the last mode had to be
                                // ACELP.
                                let is_short = !self.channel_info[channel].ics_info.is_long_block();
                                if let (Some(lpd_data), Some(spectral_data)) = (
                                    self.channel_info[channel].lpd_data.as_deref_mut(),
                                    spectral_data_option.as_mut(),
                                ) {
                                    if lpd_data.last_core_mode != UsacCoremode::Lpd
                                        || lpd_data.last_lpd_mode != LpdMode::Acelp
                                    {
                                        lpd_data.lpd_info.core_mode_last = UsacCoremode::Lpd;
                                        lpd_data.lpd_info.lpd_mode_last = LpdMode::Acelp;
                                    }

                                    // Read FAC Data from bitstream.
                                    lpd_data.fac_data.read(
                                        bs,
                                        &mut spectral_data
                                            [channel * frame_length..(channel + 1) * frame_length],
                                        is_short,
                                        None,
                                        true,
                                        0,
                                    );
                                }
                            }
                        }
                    }
                    RdbId::Esc2Rvlc => {
                        if ac_flags.contains(ACFlags::ER_RVLC) {
                            let ch_info = &mut self.channel_info[channel];
                            if let (Some(rvlc_data), Some(pns_data)) = (
                                ch_info.rvlc_data.as_deref_mut(),
                                ch_info.pns_data.as_deref_mut(),
                            ) {
                                rvlc_data.decode(
                                    bs,
                                    &ch_info.ics_info,
                                    &ch_info.code_book,
                                    ch_info.global_gain,
                                    pns_data,
                                    &mut ch_info.scale_factor,
                                );
                            }
                        }
                    }
                    RdbId::Esc1Hcr => {
                        if ac_flags.contains(ACFlags::ER_HCR) {
                            if let Some(hcr_data) = common_channel_data.er_hcr_data.as_deref_mut() {
                                hcr_data.read(
                                    bs,
                                    channel as u32,
                                    if self.num_channels == 2 {
                                        ChannelElementId::Cpe
                                    } else {
                                        ChannelElementId::Sce
                                    },
                                );
                            }
                        }
                    }
                    RdbId::SpectralData => {
                        let ch_info = &mut self.channel_info[channel];
                        if !ac_flags.contains(ACFlags::ER_HCR) {
                            error = block::read_spectral_data(
                                bs,
                                &ch_info.ics_info,
                                &ch_info.code_book,
                                &mut quantized_spectrum[..frame_length],
                            );
                        } else if let Some(hcr_data) =
                            common_channel_data.er_hcr_data.as_deref_mut()
                        {
                            if hcr_data.decode(
                                bs,
                                &ch_info.ics_info,
                                &ch_info.code_book,
                                channel,
                                &mut quantized_spectrum[..frame_length],
                            ) != 0
                            {
                                error = AacDecoderError::ParseError;
                            }
                        }

                        if !ac_flags.intersects(ACFlags::USAC | ACFlags::ELD | ACFlags::SCALABLE)
                            && !self.el_flags.contains(ChannelFlags::LFE)
                        {
                            // Apply pulse data.
                            if let Some(pulse_data) = common_channel_data.pulse_data.as_deref_mut()
                            {
                                pulse_data[channel].apply(&mut quantized_spectrum[..frame_length]);
                            }
                        }

                        render_mode = if !self.el_flags.contains(ChannelFlags::GA_CCE) {
                            if ac_flags.contains(ACFlags::ELD) {
                                AacDecoderRenderMode::EldFb
                            } else {
                                AacDecoderRenderMode::Imdct
                            }
                        } else {
                            AacDecoderRenderMode::Invalid
                        };
                    }
                    RdbId::AcSpectralData => {
                        let ch_info = &mut self.channel_info[channel];

                        if let Some(arco_data) = ch_info.arco_data.as_deref_mut() {
                            error = block::read_ac_spectral_data(
                                bs,
                                arco_data,
                                &ch_info.ics_info,
                                &mut quantized_spectrum[..frame_length],
                                ac_flags,
                            );
                            render_mode = AacDecoderRenderMode::Imdct;
                        }
                    }
                    RdbId::CoupledElements => {
                        ind_sw_cce_flag = bs.read_bit() != 0;
                        let num_coupled_elements = bs.read(3);

                        for _c in 0..=num_coupled_elements {
                            num_gain_element_lists += 1;
                            let cc_target_is_cpe = bs.read_bit() != 0; // cc_target_is_cpe[c]
                            bs.read(4); // cc_target_tag_select[c]
                            if cc_target_is_cpe {
                                let cc_l = bs.read_bit() != 0; // cc_l[c]
                                let cc_r = bs.read_bit() != 0; // cc_r[c]
                                if cc_l && cc_r {
                                    num_gain_element_lists += 1;
                                }
                            }
                        }

                        bs.read_bit(); // cc_domain
                        bs.read_bit(); // gain_element_sign
                        bs.read(2); // gain_element_scale
                    }
                    RdbId::GainElementLists => {
                        // Get BOOKSCL book from HuffmanDecoder.
                        let hcb = HuffmanDecoder::new(utils::CBTYPE_BOOKSCL); // BOOKSCL
                        let code_book = &self.channel_info[channel].code_book[..];

                        for _c in 1..num_gain_element_lists {
                            let cge = if ind_sw_cce_flag {
                                true
                            } else {
                                // common_gain_element_present[c].
                                bs.read_bit() != 0
                            };
                            if cge {
                                // Huffman
                                hcb.read_word(bs);
                            } else {
                                let max_sfb = self.channel_info[channel].ics_info.max_sf_bands();
                                for _win_group in
                                    0..self.channel_info[channel].ics_info.n_window_groups()
                                {
                                    for cb in code_book.iter().take(max_sfb) {
                                        if *cb != utils::CBTYPE_ZERO_HCB {
                                            hcb.read_word(bs);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    RdbId::AdtscrcStartReg1 => {
                        crc_reg1 = tp_dec.crc_start_region(ADTS_ERROR_CHECK_REGION1_CRC_BITS);
                    }
                    RdbId::AdtscrcStartReg2 => {
                        crc_reg2 = tp_dec.crc_start_region(ADTS_ERROR_CHECK_REGION2_CRC_BITS);
                    }

                    RdbId::AdtscrcEndReg2 => {
                        if crc_reg1 != -1 {
                            error = AacDecoderError::DecodeFrameError;
                        } else {
                            tp_dec.crc_end_region(crc_reg2);
                            crc_reg2 = -1;
                        }
                    }
                    RdbId::AdtscrcEndReg1 => {
                        tp_dec.crc_end_region(crc_reg1);
                        crc_reg1 = -1;
                    }
                    RdbId::EndOfSequence | RdbId::NextChannel => {
                        if rdb_id == RdbId::EndOfSequence {
                            iterate_bs_elements = false;
                        }
                        let ch_info = &mut self.channel_info[channel];
                        match render_mode {
                            AacDecoderRenderMode::Imdct | AacDecoderRenderMode::EldFb => {
                                let mut band_is_noise = [true; constants::MAX_WINS_X_SFBS];
                                if let Some(spectral_data) = spectral_data_option.as_mut() {
                                    error = inverse_quantization::inverse_quantize_spectral_data(
                                        &ch_info.ics_info,
                                        &ch_info.code_book,
                                        &ch_info.scale_factor,
                                        &quantized_spectrum[..frame_length],
                                        &mut spectral_data
                                            [channel * frame_length..(channel + 1) * frame_length],
                                        &mut band_is_noise,
                                    );

                                    if error != AacDecoderError::Ok {
                                        return Err(error);
                                    }

                                    if self.el_flags.contains(ChannelFlags::USAC_NOISE) {
                                        noise_filling::noise_filling(
                                            &ch_info.ics_info,
                                            ch_info.fd_noise_level_and_offset,
                                            &mut ch_info.scale_factor,
                                            &mut spectral_data[channel * frame_length
                                                ..(channel + 1) * frame_length],
                                            &mut ch_info.noise_filling_random_seed,
                                            &band_is_noise,
                                        );
                                    }
                                }

                                // fall_through case.
                                ch_info.render_mode = render_mode;
                                render_mode = AacDecoderRenderMode::Invalid;
                            }

                            AacDecoderRenderMode::Lpd => {
                                ch_info.render_mode = render_mode;
                                render_mode = AacDecoderRenderMode::Invalid;
                            }
                            AacDecoderRenderMode::Invalid => {
                                // Do nothing.!
                            }
                        }
                        // update channel
                        channel = (channel + 1) % (self.num_channels as usize);
                    }
                    RdbId::LinkSequence => {
                        if let Some(el) = element_list.next(decision_bit) {
                            element_list = el;
                            list_id = element_list.id();
                            list_id_index = -1;
                        } else {
                            // Error -> The next element could be `None`!
                            is_bail_set = true;
                            break 'element_iterate_loop;
                        }
                    }
                } // match - end

                if error != AacDecoderError::Ok {
                    is_bail_set = true; // Error
                    break 'element_iterate_loop;
                }
                if !iterate_bs_elements {
                    break 'element_iterate_loop;
                }
                list_id_index += 1;
            } // 'element_iterate_loop - end

            if ac_flags.contains(ACFlags::ER_RVLC) && !is_bail_set {
                let is_ms_mask_present = if let Some(js_data) = self.js_data.as_deref() {
                    js_data.ms_mask_present() != 0
                } else {
                    false
                };

                if self.num_channels == 2 {
                    let (ch_left, ch_right) = self.channel_info
                        [..=usize::from(AudioChannel::Right)]
                        .split_at_mut(usize::from(AudioChannel::Right));
                    if let (Some(rvlc_left), Some(rvlc_right)) = (
                        ch_left[0].rvlc_data.as_deref_mut(),
                        ch_right[0].rvlc_data.as_deref_mut(),
                    ) {
                        rvlc_left.element_check(Some(rvlc_right), is_ms_mask_present);
                    }
                } else if let Some(rvlc_left) = self.channel_info[usize::from(AudioChannel::Left)]
                    .rvlc_data
                    .as_deref_mut()
                {
                    rvlc_left.element_check(None, is_ms_mask_present);
                }
            }
        } else {
            error = AacDecoderError::UnsupportedFormat;
        } // if let Some(mut element_list) = element_list_option

        // Bail case
        if crc_reg1 != -1 || crc_reg2 != -1 {
            if error == AacDecoderError::Ok {
                error = AacDecoderError::DecodeFrameError;
            }

            if crc_reg1 != -1 {
                tp_dec.crc_end_region(crc_reg1);
            }

            if crc_reg2 != -1 {
                tp_dec.crc_end_region(crc_reg2);
            }
        }

        if error == AacDecoderError::Ok {
            // Set state flags (read).
            self.state_flags.insert(ElState::READ);
            Ok(())
        } else {
            Err(error)
        }
    }

    /// Decodes channel element.
    ///
    /// # Parameters
    ///
    /// - `common_channel_data`: `CommonChannelData` instance with valid data.
    /// - `spectral_data`: Input/Output Spectral coefficients buffer. Length should be twice of
    ///   `frame_length`.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    pub fn decode(
        &mut self,
        common_channel_data: &mut CommonChannelData,
        spectral_data: &mut [f32],
        ac_flags: ACFlags,
    ) {
        let scratch_buffer = <[f32]>::mut_from_bytes(&mut common_channel_data.scratch_work_buffer)
            .expect("Cannot interpret the scratch buffer as f32 slice");

        if self.common_window {
            if !ac_flags.contains(ACFlags::USAC) {
                if let (
                    Some(pns_left),
                    Some(pns_right),
                    Some(pns_inter_channel_data),
                    Some(js_data),
                ) = (
                    self.channel_info[usize::from(AudioChannel::Left)]
                        .pns_data
                        .as_deref(),
                    self.channel_info[usize::from(AudioChannel::Right)]
                        .pns_data
                        .as_deref(),
                    self.pns_inter_channel_data.as_deref_mut(),
                    self.js_data.as_deref_mut(),
                ) {
                    pns_inter_channel_data.init();
                    if pns_left.is_pns_active() || pns_right.is_pns_active() {
                        pns_inter_channel_data.map_midside_mask_to_pns_correlation(
                            &self.channel_info[usize::from(AudioChannel::Left)].ics_info,
                            pns_left,
                            pns_right,
                            js_data.ms_used_mut(),
                        );
                    }
                }
            }

            if !ac_flags.contains(ACFlags::USAC)
                || self.channel_info[usize::from(AudioChannel::Left)].is_usac_tns_on_lr
            {
                if let Some(js_data) = self.js_data.as_deref_mut() {
                    js_data.apply(
                        &self.channel_info[usize::from(AudioChannel::Left)].ics_info,
                        spectral_data,
                        &self.prev_spectral_data,
                        &mut scratch_buffer[..(2 * constants::MAX_FRAMESIZE)],
                    );
                }
            }

            if !ac_flags.contains(ACFlags::USAC) {
                if let Some(js_data) = self.js_data.as_deref_mut() {
                    intensity::apply_is(
                        &self.channel_info[usize::from(AudioChannel::Left)].ics_info,
                        &self.channel_info[usize::from(AudioChannel::Right)].code_book,
                        &self.channel_info[usize::from(AudioChannel::Right)].scale_factor,
                        js_data.ms_used(),
                        spectral_data,
                    );
                }
            }
        } // self.common_window - ends

        // Apply PNS, TNS (or) LPD if available for each channel_info.
        let frame_length = spectral_data.len() / self.num_channels as usize;
        for (channel, (ch_info, spectrum)) in izip!(
            self.channel_info.iter_mut(),
            spectral_data.chunks_exact_mut(frame_length)
        )
        .take(self.num_channels as usize)
        .enumerate()
        {
            if ch_info.render_mode == AacDecoderRenderMode::Lpd {
                if let Some(lpd_data) = ch_info.lpd_data.as_deref_mut() {
                    lpd_data.decode(spectrum, &mut ch_info.noise_filling_random_seed);
                }
            } else {
                if !ac_flags.contains(ACFlags::USAC) {
                    if let (Some(pns_inter_channel_data), Some(pns_data)) = (
                        self.pns_inter_channel_data.as_deref_mut(),
                        ch_info.pns_data.as_deref_mut(),
                    ) {
                        pns_data.apply(
                            pns_inter_channel_data,
                            &ch_info.ics_info,
                            spectrum,
                            &ch_info.scale_factor,
                            channel,
                        );
                    }
                }
                ch_info.tns_data.apply(&ch_info.ics_info, spectrum);
            }
        }

        if self.common_window
            && ac_flags.contains(ACFlags::USAC)
            && !self.channel_info[usize::from(AudioChannel::Left)].is_usac_tns_on_lr
        {
            if let Some(js_data) = self.js_data.as_deref_mut() {
                js_data.apply(
                    &self.channel_info[usize::from(AudioChannel::Left)].ics_info,
                    spectral_data,
                    &self.prev_spectral_data,
                    &mut scratch_buffer[..(2 * constants::MAX_FRAMESIZE)],
                );
            }
        }

        if self.el_flags.contains(ChannelFlags::USAC_CP_POSSIBLE) {
            if let Some(js_data) = self.js_data.as_deref_mut() {
                js_data.set_window_sequence(
                    self.channel_info[usize::from(AudioChannel::Left)]
                        .ics_info
                        .window_sequence(),
                );
                js_data.set_window_shape(
                    self.channel_info[usize::from(AudioChannel::Left)]
                        .ics_info
                        .window_shape(),
                );
                if !self.common_window {
                    js_data.set_cp_interruption();
                }
            }
        }
        self.state_flags.insert(ElState::DECODED);
    }

    pub fn interrupt(&mut self, ac_flags: ACFlags) {
        for ch in 0..self.num_channels as usize {
            if ac_flags.contains(ACFlags::USAC) {
                if let Some(arco_data) = self.channel_info[ch].arco_data.as_deref_mut() {
                    arco_data.reset();
                }
            }
        }
    }

    /// Conceals defective frame and renders specific channel.
    ///
    /// # Parameters
    ///
    /// - `common_channel_data`: `CommonChannelData` instance with valid data.
    /// - `conceal_data`: `ConcealmentData` instance with valid data.
    /// - `drc_data`: `AacDrcData` instance with valid data.
    /// - `spectral_data`: Spectral data buffer to read from bitstream.
    /// - `time_data`: Time domain output buffer.
    /// - `is_sbr_enabled`: Flag to indicate whether SBR is enabled.
    /// - `is_pseudo_lr`: Flag to indicate whether core decoder output is to be changed.
    /// - `is_frame_ok`: Indicates a valid frame.
    /// - `channel`: Channel number to be rendered.
    /// - `is_flushing`: Flag to indicate whether flush mode of AAC decoder is active or not.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    #[expect(clippy::too_many_arguments)]
    pub fn render(
        &mut self,
        common_channel_data: &mut CommonChannelData,
        conceal_data: &mut ConcealmentData,
        drc_data: &mut Option<&mut AacDrcData>,
        spectral_data: &mut [f32],
        time_data: &mut [f32],
        is_sbr_enabled: bool,
        is_pseudo_lr: bool,
        is_frame_ok: bool,
        channel: usize,
        is_flushing: bool,
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        let mut num_channel = channel;
        let num_element_channels = usize::from(self.num_channels);
        let frame_size = time_data.len() / num_element_channels;

        let spec_frame_size = spectral_data.len() / num_element_channels;
        let prev_spec_frame_size = self.prev_spectral_data.len() / num_element_channels;
        let prev_spectral_data = &mut self.prev_spectral_data;

        for (spec, prev_spec, time_out, ch_info) in izip!(
            spectral_data.chunks_exact_mut(spec_frame_size),
            prev_spectral_data.chunks_exact_mut(prev_spec_frame_size),
            time_data.chunks_exact_mut(frame_size),
            self.channel_info.iter_mut()
        )
        .take(num_element_channels)
        {
            // Conceal defective spectral data.
            conceal_data.apply(
                if let Some(lpd_data) = ch_info.lpd_data.as_deref_mut() {
                    Some(lpd_data)
                } else {
                    None
                },
                &mut ch_info.ics_info,
                &common_channel_data.sr_info,
                &mut spec[..frame_size],
                &mut prev_spec[..frame_size],
                &mut ch_info.render_mode,
                num_channel,
                ac_flags,
                is_frame_ok,
            );

            let is_channel_ok =
                is_frame_ok && !(is_flushing || conceal_data.is_mute_release(num_channel));

            // AAC-DRC processing.
            if !ac_flags.intersects(ACFlags::USAC | ACFlags::ER) {
                if let Some(drc_data_unwrapped) = drc_data {
                    drc_data_unwrapped.apply(&mut spec[..frame_size], num_channel, is_sbr_enabled);
                }
            }

            let scratch_buffer =
                <[f32]>::mut_from_bytes(&mut common_channel_data.scratch_work_buffer)
                    .expect("Cannot interpret the scratch buffer as f32 slice");

            match ch_info.render_mode {
                AacDecoderRenderMode::Imdct => {
                    if let Some(mdct) = ch_info.inverse_mdct.as_deref_mut() {
                        block::frequency_to_time(
                            &ch_info.ics_info,
                            if let Some(lpd_data) = ch_info.lpd_data.as_deref_mut() {
                                Some(lpd_data)
                            } else {
                                None
                            },
                            mdct,
                            &mut spec[..frame_size],
                            time_out,
                            &mut scratch_buffer[..SYNTH_BUF_LENGTH],
                            is_channel_ok,
                            ac_flags,
                        );
                    }
                }
                AacDecoderRenderMode::EldFb => {
                    ld_filter_bank::synthesize(
                        &mut spec[..frame_size],
                        time_out,
                        &mut ch_info.eld_overlap_buffer,
                    );
                }
                AacDecoderRenderMode::Lpd => {
                    if let (Some(imdct), Some(lpd_data)) = (
                        ch_info.inverse_mdct.as_deref_mut(),
                        ch_info.lpd_data.as_deref_mut(),
                    ) {
                        lpd_data.render(
                            &ch_info.ics_info,
                            imdct,
                            &mut spec[..frame_size],
                            &mut scratch_buffer[..SYNTH_BUF_LENGTH],
                            time_out,
                            is_channel_ok,
                        );
                    }
                }
                AacDecoderRenderMode::Invalid => {
                    return AacDecoderError::Unknown;
                }
            }

            // Time domain fading.
            conceal_data.timedomain_fading(time_out, num_channel);

            num_channel += 1;
        }

        // Pseudo LR channel rotation.
        if ac_flags.contains(ACFlags::USAC) && is_pseudo_lr {
            let (time_left, time_right) = time_data.split_at_mut(time_data.len() / 2);
            block::apply_pseudo_lr(&mut time_left[..frame_size], &mut time_right[..frame_size]);
        }
        AacDecoderError::Ok
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem::{align_of, size_of};

    #[test]
    fn test_buff_alignment_w_f32() {
        let ccd = CommonChannelData::new();

        let scratch_buff =
            &ccd.scratch_work_buffer[..(2 * constants::MAX_FRAMESIZE * size_of::<f32>())];

        assert!(scratch_buff.len() % size_of::<f32>() == 0);

        let addr_of_val = std::ptr::addr_of!(scratch_buff);
        assert!(addr_of_val as usize % align_of::<f32>() == 0);
    }

    #[test]
    fn test_buff_alignment_w_i16() {
        let frame_length = 1024;
        let ccd = CommonChannelData::new();

        let scratch_buff = &ccd.scratch_work_buffer[..(frame_length * size_of::<i16>())];

        assert!(scratch_buff.len() % size_of::<i16>() == 0);

        let addr_of_val = std::ptr::addr_of!(scratch_buff);
        assert!(addr_of_val as usize % align_of::<i16>() == 0);
    }
}
