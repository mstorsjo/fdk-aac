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
//! AAC decoder extension data handling

use crate::{
    aac_dec::{
        ancillary_data::{AncDataType, AncillaryData},
        channel_info::BlockType,
        config::Config,
        constants::{MAX_CHANNELS, MAX_ELEMENTS},
        drc::{AacDrcData, AacDrcPayloadType},
        error_codes::{AacDecoderError, Cluster},
        output_info::{MetadataInfo, OutputInfo},
        params::Params,
        signal_delay::SignalDelay,
    },
    common::{
        aot::AudioObjectType,
        audio_channel_type::AudioChannelType,
        bitstream::Bitstream,
        bs_element_id::ChannelElementId,
        channel_map_descr::ChannelMapDescriptor,
        enums::AudioChannel,
        flags::{AACDecFlags, ACFlags, ChannelFlags},
        qmf::{QmfFlags, QmfMode},
        qmf_domain::{
            QmfDomain, QmfDomainError, QmfDomainParams, QMF_DOMAIN_MAX_ANALYSIS_QMF_BANDS,
            QMF_DOMAIN_MAX_OV_TIMESLOTS, QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS,
            QMF_DOMAIN_MAX_TIMESLOTS, QMF_MAX_PROC_SUBBANDS,
        },
    },
    drc_dec::{
        DrcDecFunctionalRange, DrcDecParameters, DrcDecoder, ProcessingLocation,
        UNDEFINED_LOUDNESS_VALUE,
    },
    pcm_dmx::PcmDmx,
    sac_dec::{sac_dec_lib::MpegSurroundDecoder, SacDecParam, SacDecoderError, SacInputConfig},
    sbr_dec::{common::SbrError, params::SbrDecParam, sbr_dec_api::SbrDecoder},
    tp_dec::{
        asc::usac_config::UsacExtElementType, ProgramConfig, ReconfigState, TransportDec,
        UsacExtElementConfig,
    },
};

/// Ancillary data sync word.
const MPEG4_ANC_DATA_SYNC_WORD: u32 = 0xBC;

/// Ancillary data sync length.
const MPEG4_ANC_DATA_SYNC_LEN: u8 = 8;

/// Maximum number of ancillary data elements.
pub const MAX_ANC_ELEMENTS: usize = 8;

#[repr(C)]
#[derive(Debug, PartialEq)]
/// Extension payload types.
enum ExtPayloadType {
    Fil = 0x00,
    FillData = 0x01,
    DataElement = 0x02,
    DataLength = 0x03,
    LdsacData = 0x09,
    DynamicRange = 0x0b,
    SbrData = 0x0d,
    SbrDataCrc = 0x0e,
}

impl TryFrom<u8> for ExtPayloadType {
    type Error = AacDecoderError;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0x00 => Ok(ExtPayloadType::Fil),
            0x01 => Ok(ExtPayloadType::FillData),
            0x02 => Ok(ExtPayloadType::DataElement),
            0x03 => Ok(ExtPayloadType::DataLength),
            0x09 => Ok(ExtPayloadType::LdsacData),
            0x0b => Ok(ExtPayloadType::DynamicRange),
            0x0d => Ok(ExtPayloadType::SbrData),
            0x0e => Ok(ExtPayloadType::SbrDataCrc),
            _ => Err(AacDecoderError::ParseError),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
/// Structure that holds extension data.
pub struct ExtensionData {
    /// QMF domain data handling.
    pub qmf_domain: Box<QmfDomain>,

    /// Requested configuration parameters for the QMF domain.
    pub qmf_domain_params_requested: QmfDomainParams,

    /// MPEG Surround Decoder instance.
    pub mpeg_surround_decoder: Box<MpegSurroundDecoder>,

    /// SBR Decoder instance.
    pub sbr_decoder: Box<SbrDecoder>,

    /// Vector of ancillary data.
    pub ancillary_data: Vec<AncillaryData>,

    /// AAC DRC data.
    pub drc_data: Option<Box<AacDrcData>>,

    /// Optional Program Config Element (PCE).
    pub pce: Option<Box<ProgramConfig>>,

    /// MPEG-D (USAC) DRC Decoder.
    pub uni_drc_decoder: Box<DrcDecoder>,

    /// PCM Downmix instance.
    pub pcm_downmix: Box<PcmDmx>,

    /// Delay residual signal to compensate for eSBR delay of DMX signal
    /// in case of `StereoCfgIndex::Mps212wResMonoSbr`.
    pub usac_residual_delay: Option<Box<SignalDelay>>,

    /// Instance tag of the first channel element.
    pub first_channel_element_instance_tag: u8,

    /// Flag which indicates if SBR/PS/DRC is implicit.
    pub implicit_ac_flags: ACFlags,
}

impl Default for ExtensionData {
    fn default() -> Self {
        Self {
            qmf_domain: Box::new(QmfDomain::new()),
            mpeg_surround_decoder: Box::new(MpegSurroundDecoder::new()),
            sbr_decoder: Box::new(SbrDecoder::new()),
            ancillary_data: Vec::with_capacity(MAX_ANC_ELEMENTS),
            uni_drc_decoder: Box::new(DrcDecoder::new(DrcDecFunctionalRange::All)),
            pcm_downmix: Box::new(PcmDmx::new()),
            qmf_domain_params_requested: QmfDomainParams::new(),
            drc_data: None,
            pce: None,
            usac_residual_delay: None,
            first_channel_element_instance_tag: 0,
            implicit_ac_flags: ACFlags::empty(),
        }
    }
}

impl ExtensionData {
    /// Creates a new `ExtensionData` instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Initializes extension data.
    ///
    /// # Parameters
    ///
    /// - `pce`: `ProgramConfig` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `config_mode_flag`: Configuration mode flag (`ReconfigState`).
    /// - `config_changed`: Mutable reference to flag to indicate change in configuration.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    pub fn init(
        &mut self,
        pce: &ProgramConfig,
        decoder_config: &Config,
        config_mode: ReconfigState,
        config_changed: &mut bool,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());
        let ac_flags = decoder_config.ac_flags;

        if config_mode == ReconfigState::AllocMem {
            if *config_changed {
                self.configure_qmf_domain(decoder_config);

                // Delete mixdown metadata from the past.
                self.pcm_downmix.reset();

                if !ac_flags.intersects(ACFlags::USAC | ACFlags::ER) {
                    let mut drc_data =
                        Box::new(AacDrcData::new(usize::from(decoder_config.num_channels)));

                    // Initialize DRC data.
                    drc_data.init(
                        &decoder_config.channel_type[..usize::from(decoder_config.num_channels)],
                        pce.element_instance_tag(),
                    );

                    self.drc_data = Some(drc_data);

                    // Initialized program config element.
                    self.pce = Some(Box::new(*pce));
                }

                if !ac_flags.contains(ACFlags::USAC) {
                    self.ancillary_data = Vec::with_capacity(MAX_ANC_ELEMENTS);
                }

                if ac_flags.contains(ACFlags::SBR_PRESENT) {
                    if !ac_flags.intersects(ACFlags::USAC | ACFlags::ER) {
                        error_status = self.init_sbr_decoder(decoder_config);
                    }

                    // Configure QMF.
                    let qmf_mode = if decoder_config.ac_flags.contains(ACFlags::USAC_SCFGI3) {
                        Some(QmfMode::Analysis)
                    } else if decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT) {
                        Some(QmfMode::Synthesis)
                    } else {
                        None
                    };

                    let _ = self.sbr_decoder.set_param(SbrDecParam::SkipQmf(qmf_mode));

                    if decoder_config.ac_flags.contains(ACFlags::ELD) {
                        let is_ld_qmf_time_align =
                            decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT);
                        let _ = self
                            .sbr_decoder
                            .set_param(SbrDecParam::LdQmfTimeAlign(is_ld_qmf_time_align));
                    }

                    if ac_flags.contains(ACFlags::USAC_SCFGI2) {
                        let mut usac_residual_delay_comp_samples = 0;
                        // In this case it is necessary to follow up the DMX signal delay
                        // caused by HBE also with the residual signal (2nd core channel). The
                        // SBR overlap delay is not regarded here, this is handled by the
                        // MPS212 implementation.
                        if ac_flags.contains(ACFlags::HBE_PRESENT) {
                            usac_residual_delay_comp_samples += decoder_config.frame_length;
                        }
                        if decoder_config.sampling_frequency
                            == (decoder_config.ext_sampling_frequency >> 2)
                        {
                            // Difference between 12 SBR overlap slots from SBR and 6 slots delayed
                            // in MPS212.
                            usac_residual_delay_comp_samples += 6 * 16;
                        }

                        self.usac_residual_delay = Some(Box::new(SignalDelay::create(
                            usize::from(usac_residual_delay_comp_samples),
                        )));
                    }
                }

                if ac_flags.contains(ACFlags::MPS_PRESENT) {
                    let param_value = if ac_flags.contains(ACFlags::SBR_PRESENT)
                        && !ac_flags.contains(ACFlags::USAC_SCFGI3)
                    {
                        SacInputConfig::Qmf
                    } else {
                        SacInputConfig::Time
                    } as i32;

                    let _ = self
                        .mpeg_surround_decoder
                        .set_params(SacDecParam::Interface, param_value);
                }

                if ac_flags.contains(ACFlags::USAC) {
                    let drc_dec_sample_rate = if decoder_config.ext_sampling_frequency == 0 {
                        decoder_config.sampling_frequency
                    } else {
                        decoder_config.ext_sampling_frequency
                    };

                    if self
                        .uni_drc_decoder
                        .init(
                            decoder_config.ext_samples_per_frame,
                            drc_dec_sample_rate,
                            decoder_config.num_channels,
                        )
                        .is_err()
                    {
                        error_status = Err(AacDecoderError::UnsupportedFormat);
                    }
                }

                self.first_channel_element_instance_tag = 255;
                self.implicit_ac_flags = ACFlags::empty();
            }

            // PCE present during initialization.
            if !ac_flags.intersects(ACFlags::USAC | ACFlags::ER) && pce.is_valid() {
                if let Some(pce) = self.pce.as_deref() {
                    self.pcm_downmix.set_matrix_mixdown_from_pce(
                        pce.is_matrix_mixdown_index_present(),
                        pce.matrix_mixdown_index(),
                    );
                }
            }
        }

        error_status
    }

    /// Initializes the SBR decoder instance.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: `Config` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn init_sbr_decoder(&mut self, decoder_config: &Config) -> Result<(), AacDecoderError> {
        let is_ps_possible = decoder_config.num_channel_elements == 1
            && decoder_config.element_config[0]
                .el_flags
                .contains(ChannelFlags::PS_POSSIBLE);

        let mut element_ids = [ChannelElementId::None; MAX_ELEMENTS];

        let mut ch_el_idx = 0;
        for element_cfg in decoder_config
            .element_config
            .iter()
            .take(usize::from(decoder_config.num_elements))
        {
            if element_cfg.element_type.is_channel_element() {
                element_ids[ch_el_idx] = if is_ps_possible {
                    ChannelElementId::Sce
                } else {
                    element_cfg.element_type
                };
                ch_el_idx += 1;
            }
        }

        let ext_sampling_frequency = if decoder_config.ext_sampling_frequency == 0 {
            2 * decoder_config.sampling_frequency
        } else {
            decoder_config.ext_sampling_frequency
        };

        let harmonic_sbr = if self.implicit_ac_flags.contains(ACFlags::MPEG4_ESBR) {
            1
        } else {
            2
        };

        if self
            .sbr_decoder
            .init(
                &mut self.qmf_domain,
                &mut self.qmf_domain_params_requested,
                decoder_config.sampling_frequency,
                ext_sampling_frequency,
                decoder_config.samples_per_frame,
                decoder_config.aot,
                &element_ids,
                usize::from(decoder_config.num_channel_elements),
                harmonic_sbr,
                1,
                is_ps_possible,
            )
            .is_err()
        {
            return Err(AacDecoderError::UnsupportedFormat);
        }

        if (!decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER))
            && self.qmf_domain.explicit_config() == 0
            && self.qmf_domain_params_requested.num_qmf_proc_channels != 0
        {
            self.qmf_domain_params_requested.num_input_channels =
                decoder_config.num_channels.max(2);
            self.qmf_domain_params_requested.num_output_channels =
                decoder_config.num_channels.max(2);
        }

        Ok(())
    }

    /// Configures QMF domain.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: `Config` instance with valid internal data.
    fn configure_qmf_domain(&mut self, decoder_config: &Config) {
        let ac_flags = decoder_config.ac_flags;

        // Set up QMF domain for AOTs with explicit signalling of SBR and or MPS. This is to be
        // able to play out the first frame alway with the correct frame size and sampling rate
        // even in case of concealment.
        if ac_flags.contains(ACFlags::USAC) {
            if ac_flags.contains(ACFlags::SBR_PRESENT) {
                self.qmf_domain_params_requested.num_input_channels = decoder_config.num_channels;
                self.qmf_domain_params_requested.num_output_channels =
                    if ac_flags.contains(ACFlags::USAC_SCFGI1) {
                        2
                    } else {
                        decoder_config.num_channels
                    };
                self.qmf_domain_params_requested.flags = QmfFlags::empty();
                self.qmf_domain_params_requested.num_bands_analysis =
                    ((u32::from(QMF_DOMAIN_MAX_ANALYSIS_QMF_BANDS)
                        * u32::from(decoder_config.samples_per_frame))
                        / u32::from(decoder_config.ext_samples_per_frame))
                    .try_into()
                    .unwrap();
                self.qmf_domain_params_requested.num_bands_synthesis =
                    QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS as u16;

                if decoder_config.samples_per_frame == decoder_config.ext_samples_per_frame >> 2 {
                    self.qmf_domain_params_requested.num_qmf_time_slots = QMF_DOMAIN_MAX_TIMESLOTS;
                    self.qmf_domain_params_requested.num_qmf_ov_time_slots =
                        QMF_DOMAIN_MAX_OV_TIMESLOTS;
                } else {
                    self.qmf_domain_params_requested.num_qmf_time_slots =
                        QMF_DOMAIN_MAX_TIMESLOTS >> 1;
                    self.qmf_domain_params_requested.num_qmf_ov_time_slots =
                        QMF_DOMAIN_MAX_OV_TIMESLOTS >> 1;
                }

                self.qmf_domain_params_requested.num_qmf_proc_bands = QMF_MAX_PROC_SUBBANDS;
                self.qmf_domain_params_requested.num_qmf_proc_channels = 1;
                self.qmf_domain.set_explicit_config(1);
            }
        } else if ac_flags.contains(ACFlags::ELD) {
            if ac_flags.contains(ACFlags::MPS_PRESENT) {
                let sac_interface = if ac_flags.contains(ACFlags::SBR_PRESENT) {
                    SacInputConfig::Qmf
                } else {
                    SacInputConfig::Time
                };

                let _ = self.mpeg_surround_decoder.configure_qmf_domain(
                    &mut self.qmf_domain_params_requested,
                    sac_interface,
                    AudioObjectType::AotErAacEld,
                    decoder_config.sampling_frequency * u32::from(decoder_config.ds_factor),
                );

                self.qmf_domain.set_explicit_config(1);
            } else if ac_flags.contains(ACFlags::SBR_PRESENT) {
                self.qmf_domain_params_requested.num_input_channels = decoder_config.num_channels;
                self.qmf_domain_params_requested.num_output_channels = decoder_config.num_channels;
                self.qmf_domain.set_explicit_config(1);
            }
        } else {
            // QmfDomain is initialized by SBR and MPS init functions if required.
            self.qmf_domain.set_explicit_config(0);
        }
    }

    /// Returns `MetadataInfo` instance.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: `Config` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - `MetadataInfo`
    pub fn get_metadata_info(&self, decoder_config: &Config) -> MetadataInfo {
        let mut metadata_info = MetadataInfo::default();

        if !decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER)
        {
            if let Some(pce) = self.pce.as_deref() {
                if pce.is_matrix_mixdown_index_present() {
                    metadata_info.pce_matrix_mixdown_index = pce.matrix_mixdown_index() as i8;
                    metadata_info.pce_pseudo_surround_enable = pce.is_pseudo_surround_enabled();
                }
            }

            if let Some(drc_data) = self.drc_data.as_deref() {
                // Map DRC data to MetadataInfo structure.
                metadata_info.drc_presentation_mode = drc_data.presentation_mode();
                metadata_info.drc_program_reference_level = drc_data.prog_ref_level();
            }
        }

        metadata_info
    }

    /// Parses error resilient(ER) extension data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn parse_er_ext_data(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());
        let ac_flags = decoder_config.ac_flags;

        if ac_flags.contains(ACFlags::ELD | ACFlags::SBR_PRESENT) {
            let mut ch_el_idx = 0;

            for ele_config in decoder_config
                .element_config
                .iter()
                .take(usize::from(decoder_config.num_elements))
            {
                let element_id = ele_config.element_type;

                if element_id.is_channel_element() {
                    let sbr_payload_length =
                        if (tp_dec.remaining_au_bits() > 0) && error_status.is_ok() {
                            -1
                        } else {
                            0
                        };

                    if let Err(sbr_error) = self.sbr_decoder.parse(
                        tp_dec.bs_mut(),
                        sbr_payload_length,
                        element_id,
                        ch_el_idx,
                        decoder_config.ac_flags,
                        ele_config.el_flags,
                    ) {
                        if element_id != ChannelElementId::Lfe {
                            error_status = if sbr_error == SbrError::ParseError {
                                Err(AacDecoderError::ParseError)
                            } else {
                                Err(AacDecoderError::Unknown)
                            };
                        }
                    }
                    ch_el_idx += 1;
                }
            }
        }

        if error_status.is_ok() {
            let mut bit_count = tp_dec.remaining_au_bits();
            while bit_count > 7 {
                {
                    if self
                        .parse_ext_payload(tp_dec.bs_mut(), decoder_config, bit_count, 255)
                        .is_err()
                    {
                        error_status = Err(AacDecoderError::ParseError);
                        break;
                    }
                }
                bit_count = tp_dec.remaining_au_bits();
            }
        }

        // Skip the remaining extension bytes.
        let au_bits_remaining = tp_dec.remaining_au_bits();
        tp_dec.bs_mut().push(au_bits_remaining);

        // In ER bitstream syntax the extensions payloads are at the very end of the
        // access unit. No further bitstream data need to be parsed and error code can
        // be reset.
        if error_status == Err(AacDecoderError::ParseError) {
            Ok(())
        } else {
            error_status
        }
    }

    /// Parses extension data payload from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Mutable `Bitstream` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `ext_payload_length`: Payload length of extension data in bits.
    /// - `element_index`: Index of the current channel element. It is 255, if the channel element
    ///   is not an audio channel element.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn parse_ext_payload(
        &mut self,
        bs: &mut Bitstream,
        decoder_config: &Config,
        ext_payload_length: isize,
        element_index: usize,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());
        let mut skip_trailing_bytes = false;
        let ext_payload_anchor = bs.valid_bits();

        if ext_payload_length < 4 {
            return Err(AacDecoderError::ParseError);
        } else if ext_payload_anchor < ext_payload_length {
            return Err(AacDecoderError::DecodeFrameError);
        }

        let ext_type_result = ExtPayloadType::try_from(bs.read(4) as u8);
        match ext_type_result {
            Ok(ExtPayloadType::DynamicRange) => {
                if !decoder_config
                    .ac_flags
                    .intersects(ACFlags::USAC | ACFlags::ER)
                {
                    if let Some(drc_data) = self.drc_data.as_deref_mut() {
                        let payload_length =
                            ext_payload_length - (ext_payload_anchor - bs.valid_bits());
                        drc_data.read(bs, payload_length, AacDrcPayloadType::MpegDrcExtData);
                    } else {
                        // AAC DRC instance is expected to be present.
                        error_status = Err(AacDecoderError::ParseError);
                    }
                } else {
                    skip_trailing_bytes = true;
                }
            }
            Ok(ExtPayloadType::LdsacData) => {
                if decoder_config
                    .ac_flags
                    .contains(ACFlags::ELD | ACFlags::MPS_PRESENT)
                {
                    let mps_payload_length =
                        (bs.valid_bits() - ext_payload_anchor + ext_payload_length).max(0);
                    if self
                        .mpeg_surround_decoder
                        .parse(
                            bs,
                            mps_payload_length,
                            AudioObjectType::AotErAacEld,
                            decoder_config.ext_sampling_frequency,
                            decoder_config.ext_samples_per_frame,
                            decoder_config.ac_flags.contains(ACFlags::INDEP),
                        )
                        .is_err()
                    {
                        error_status = Err(AacDecoderError::ParseError);
                    }
                }

                if error_status.is_ok() {
                    skip_trailing_bytes = true;
                }
            }
            Ok(ExtPayloadType::SbrDataCrc) | Ok(ExtPayloadType::SbrData) => {
                let mut break_flag = false;
                if (!decoder_config
                    .ac_flags
                    .intersects(ACFlags::USAC | ACFlags::ER))
                    && element_index != 255
                {
                    let ele_type = if decoder_config.num_channel_elements == 1
                        && decoder_config.element_config[0]
                            .el_flags
                            .contains(ChannelFlags::PS_POSSIBLE)
                    {
                        ChannelElementId::Sce
                    } else {
                        decoder_config.element_config[element_index].element_type
                    };

                    if ele_type.is_channel_element() && ele_type != ChannelElementId::Lfe {
                        if (!(decoder_config.ac_flags | self.implicit_ac_flags)
                            .contains(ACFlags::SBR_PRESENT))
                            && decoder_config.sampling_frequency <= 48000
                        {
                            if self.init_sbr_decoder(decoder_config).is_ok() {
                                // Enable SBR for implicit SBR signalling.
                                self.implicit_ac_flags.insert(ACFlags::SBR_PRESENT);
                            } else {
                                error_status = Err(AacDecoderError::ParseError);
                                break_flag = true;
                            }
                        }

                        if !break_flag {
                            let sbr_payload_length =
                                (bs.valid_bits() - ext_payload_anchor + ext_payload_length).max(0);
                            let ac_flags = if ext_type_result == Ok(ExtPayloadType::SbrDataCrc) {
                                decoder_config.ac_flags | ACFlags::SBRCRC
                            } else {
                                decoder_config.ac_flags
                            };

                            if let Err(sbr_error) = self.sbr_decoder.parse(
                                bs,
                                sbr_payload_length,
                                ele_type,
                                element_index,
                                ac_flags,
                                ChannelFlags::empty(),
                            ) {
                                error_status = if sbr_error == SbrError::ParseError {
                                    Err(AacDecoderError::ParseError)
                                } else {
                                    Err(AacDecoderError::Unknown)
                                };
                            }
                        }
                    }
                }

                if error_status.is_ok() && !break_flag {
                    skip_trailing_bytes = true;
                }
            }
            Ok(ExtPayloadType::FillData) => {
                // fill_nibble must be `0000`.
                if bs.read(4) == 0 {
                    let fill_data_len =
                        (bs.valid_bits() - ext_payload_anchor + ext_payload_length) >> 3;

                    for _byte in 0..fill_data_len {
                        if bs.read(8) != 0xa5 {
                            error_status = Err(AacDecoderError::ParseError);
                            break;
                        }
                    }
                } else {
                    error_status = Err(AacDecoderError::ParseError);
                }
            }
            Ok(ExtPayloadType::DataElement) => {
                // ancillary data
                if bs.read(4) == 0 {
                    let mut temp;
                    let mut data_element_length = 0;

                    'data_element_loop: loop {
                        temp = bs.read(8);
                        data_element_length += temp;

                        if (bs.valid_bits() - ext_payload_anchor + ext_payload_length)
                            < (data_element_length * 8).try_into().unwrap()
                        {
                            error_status = Err(AacDecoderError::DecodeFrameError);
                            break 'data_element_loop;
                        }

                        if temp != 255 {
                            break 'data_element_loop;
                        }
                    }

                    if error_status.is_ok() && data_element_length > 0 {
                        // Mark extension data element and push it to ancillary_data vector.
                        // If capacity exceeded, just skip the element.
                        if self.ancillary_data.len() < self.ancillary_data.capacity() {
                            let anc_data = AncillaryData::new(
                                bs.valid_bits(),
                                data_element_length as usize,
                                255,
                                AncDataType::ExtensionDataElement,
                            );
                            self.ancillary_data.push(anc_data)
                        }

                        // Move to the end of the element.
                        bs.push(data_element_length as isize * 8);
                    }
                } else {
                    skip_trailing_bytes = true;
                }
            }
            Ok(ExtPayloadType::DataLength) => {
                // This extension payload type was created to circumvent the
                // missing length in ER-Syntax.
                if decoder_config.ac_flags.contains(ACFlags::ER) {
                    let mut len = bs.read(4);
                    if len == 15 {
                        len += bs.read(8);

                        if len == (15 + 255) {
                            len += bs.read(16);
                        }
                    }

                    let ext_data_anchor = bs.valid_bits();
                    let ext_data_length = (len << 3) as isize;
                    let mut break_flag = false;

                    if (bs.valid_bits() - ext_payload_anchor + ext_payload_length) < ext_data_length
                    {
                        error_status = Err(AacDecoderError::DecodeFrameError);
                        break_flag = true;
                    }

                    if !break_flag {
                        if ExtPayloadType::try_from(bs.read(4) as u8)
                            == Ok(ExtPayloadType::DataLength)
                        {
                            // Check NOTE 2: The extension_payload() included here must
                            // not have extension_type == EXT_DATA_LENGTH
                            error_status = Err(AacDecoderError::ParseError);
                        } else {
                            // rewind and call myself again.
                            bs.push(-4);

                            error_status =
                                self.parse_ext_payload(bs, decoder_config, ext_data_length, 255);

                            // Skip the remaining extension data length bytes.
                            let valid_bits = bs.valid_bits();
                            bs.push(valid_bits - ext_data_anchor + ext_data_length);
                        }
                    }
                } else {
                    skip_trailing_bytes = true;
                }
            }
            Ok(ExtPayloadType::Fil) => {
                skip_trailing_bytes = true;
            }
            Err(_) => {
                skip_trailing_bytes = true;
            }
        }

        if skip_trailing_bytes {
            // Skip the remaining extension bytes.
            let valid_bits = bs.valid_bits();
            bs.push(valid_bits - ext_payload_anchor + ext_payload_length);
        }

        error_status
    }

    /// Parses GA fill data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `channel_element_index`: Index of the current channel element. It is 255, if the channel
    ///   element is not an audio channel element.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn parse_ga_fill_data(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
        channel_element_index: usize,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        let mut bit_count_tmp;
        {
            let bs = tp_dec.bs_mut();
            // bs_count.
            bit_count_tmp = bs.read(4);
            if bit_count_tmp == 15 {
                // bs_esc_count.
                bit_count_tmp = (bit_count_tmp - 1) + bs.read(8);
            }
        }
        let ext_payload_anchor = tp_dec.bs_mut().valid_bits();
        let ext_payload_length = (bit_count_tmp as isize) << 3;

        if ext_payload_length > tp_dec.remaining_au_bits() {
            return Err(AacDecoderError::DecodeFrameError);
        }

        {
            let mut bit_count =
                tp_dec.bs_mut().valid_bits() - ext_payload_anchor + ext_payload_length;

            while bit_count > 7 {
                error_status = self.parse_ext_payload(
                    tp_dec.bs_mut(),
                    decoder_config,
                    ext_payload_length,
                    channel_element_index,
                );

                if error_status.is_err() {
                    // Patch error code because decoding can go on.
                    if error_status == Err(AacDecoderError::ParseError) {
                        error_status = Ok(());
                    }
                    break;
                }

                bit_count = tp_dec.bs_mut().valid_bits() - ext_payload_anchor + ext_payload_length;
            }
        }

        // Skip the remaining extension bytes.
        let push_bits_count =
            tp_dec.bs_mut().valid_bits() - ext_payload_anchor + ext_payload_length;
        tp_dec.bs_mut().push(push_bits_count);

        error_status
    }

    /// Parses ancillary data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Mutable `Bitstream` instance with valid internal data.
    /// - `data_start`: Ancillary data bitstream start offset in bits.
    /// - `data_len`: Ancillary data length in bytes.
    fn parse_mpeg_ancillary_data(&mut self, bs: &mut Bitstream, data_start: isize, data_len: u32) {
        let bs_bits = bs.valid_bits();

        // Parse AAC DRC metadata thread.
        bs.push(bs_bits - data_start);
        if let Some(drc_data) = self.drc_data.as_deref_mut() {
            drc_data.read(
                bs,
                (data_len * 8) as isize,
                AacDrcPayloadType::DvbDrcAncData,
            );
        }

        // Parse DMX metadata.
        let valid_bits = bs.valid_bits();
        bs.push(valid_bits - data_start);

        if self.pcm_downmix.parse(bs, data_len * 8).is_err() {
            self.pcm_downmix.reset();
        }

        let valid_bits = bs.valid_bits();
        bs.push(valid_bits - bs_bits);
    }

    /// Reads program config element (PCE) from bitstream.
    /// According to ISO/IEC 14496-3, section 4.5.1.2. Do not evaluate the channel configuration
    /// given in a program_config_element() inside the AAC payload.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `align_anchor`: AU start bit buffer position for AU byte alignment.
    fn parse_program_config_element(&mut self, tp_dec: &mut TransportDec, align_anchor: isize) {
        // Read PCE to temporal buffer.
        let mut tmp_pce = ProgramConfig::new();
        tmp_pce.init();

        let crc_reg = tp_dec.crc_start_region(0);
        let bs = tp_dec.bs_mut();
        tmp_pce.read(bs, align_anchor);

        tp_dec.crc_end_region(crc_reg);

        if tmp_pce.is_valid() {
            // Store matrix mixdown info in static pce.
            if tmp_pce.is_matrix_mixdown_index_present() {
                if let Some(pce) = self.pce.as_deref_mut() {
                    pce.set_matrix_mixdown_index_present(tmp_pce.is_matrix_mixdown_index_present());
                    pce.set_matrix_mixdown_index(tmp_pce.matrix_mixdown_index());
                    pce.set_pseudo_surround_enabled(tmp_pce.is_pseudo_surround_enabled());
                }

                // Valid PCE in current raw_data block.
                self.implicit_ac_flags.insert(ACFlags::PCE);
            }
        }
    }

    /// Parses USAC extension element from bitstream.
    /// UsacExElement() ISO/IEC FDIS 23003-3:2011(E) Table 21.
    ///
    /// # Parameters
    ///
    /// - `bs`: Mutable `Bitstream` instance with valid internal data.
    /// - `usac_ext_element_config`: `UsacExtElementConfig` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn parse_usac_ext_element(
        &mut self,
        bs: &mut Bitstream,
        usac_ext_element_config: &UsacExtElementConfig,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        // usac_ext_element_present.
        if bs.read_bit() != 0 {
            // usac_ext_element_use_default_length.
            let usac_ext_element_payload_length = if bs.read_bit() != 0 {
                usac_ext_element_config.default_length()
            } else {
                let mut usac_ee_pl = bs.read(8);
                if usac_ee_pl == (1_u32 << 8) - 1 {
                    let value_add = bs.read(16);
                    usac_ee_pl = (usac_ee_pl - 2) + value_add;
                }
                usac_ee_pl
            };

            if usac_ext_element_payload_length > 0 {
                if usac_ext_element_config.is_fragmented_payload() {
                    // usac_ext_element_start, usac_ext_element_stop.
                    bs.push(2);
                }

                let bs_after_ext_element =
                    bs.valid_bits() - (usac_ext_element_payload_length * 8) as isize;

                if usac_ext_element_config.ext_element_type() == UsacExtElementType::UniDrc
                    && self.uni_drc_decoder.read_uni_drc_gain(bs).is_err()
                {
                    error_status = Err(AacDecoderError::ParseError);
                }

                // Skip any remaining bits of extension payload.
                if bs.valid_bits() < bs_after_ext_element {
                    error_status = Err(AacDecoderError::ParseError);
                }

                let valid_bits = bs.valid_bits();
                bs.push(valid_bits - bs_after_ext_element);
            }
        }

        error_status
    }

    /// Parses USAC MPS and SBR data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `channel_element_index`: Index of the current channel element. It is 255, if the channel
    ///   element is not an audio channel element.
    /// - `element_count`: Index value for element config buffer in `Config`.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    fn parse_usac_mps_and_sbr_data(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
        channel_element_index: usize,
        element_count: usize,
    ) -> Result<(), AacDecoderError> {
        if decoder_config.ac_flags.contains(ACFlags::SBR_PRESENT)
            && decoder_config.element_config[element_count].element_type
                != ChannelElementId::UsacLfe
        {
            let el_flags = decoder_config.element_config[element_count].el_flags;
            let el_type = if el_flags.contains(ChannelFlags::USAC_CP_POSSIBLE)
                || decoder_config.ac_flags.contains(ACFlags::USAC_SCFGI3)
            {
                ChannelElementId::Cpe
            } else {
                ChannelElementId::Sce
            };

            let sbr_payload_length = if tp_dec.remaining_au_bits() > 0 {
                -1
            } else {
                0
            };

            let bs = tp_dec.bs_mut();
            if let Err(sbr_error) = self.sbr_decoder.parse(
                bs,
                sbr_payload_length,
                el_type,
                channel_element_index,
                decoder_config.ac_flags,
                el_flags,
            ) {
                if sbr_error == SbrError::ParseError {
                    return Err(AacDecoderError::ParseError);
                } else {
                    return Err(AacDecoderError::Unknown);
                }
            }

            if decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT)
                && el_flags.contains(ChannelFlags::USAC_MPS212)
            {
                let mps_payload_length = if tp_dec.remaining_au_bits() > 0 {
                    -1_isize
                } else {
                    0
                };

                let bs = tp_dec.bs_mut();
                if self
                    .mpeg_surround_decoder
                    .parse(
                        bs,
                        mps_payload_length,
                        AudioObjectType::AotUsac,
                        0,
                        0,
                        decoder_config.ac_flags.contains(ACFlags::INDEP),
                    )
                    .is_err()
                {
                    return Err(AacDecoderError::ParseError);
                }
            }
        }
        Ok(())
    }

    /// Reads data stream element (DSE) from bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `channel_element_count`: Last processed channel element.
    /// - `align_anchor`: AU start bit buffer position for AU byte alignment.
    fn read_dse(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
        channel_element_count: u8,
        align_anchor: isize,
    ) -> Result<(), AacDecoderError> {
        let crc_reg = tp_dec.crc_start_region(0);
        let bs = tp_dec.bs_mut();

        let element_instance_tag = bs.read(4) as u8;
        let data_byte_align_flag = bs.read_bit() != 0;

        if decoder_config.channel_config == 0 {
            // Check whether the element belongs to current program.
            if let Some(pce) = self.pce.as_deref_mut() {
                if !pce.validate_non_audio_element_instance_tag(
                    element_instance_tag,
                    ChannelElementId::Dse,
                ) {
                    return Err(AacDecoderError::DecodeFrameError);
                }
            }
        }

        let mut data_len = bs.read(8);
        if data_len == 255 {
            // Esc count
            data_len += bs.read(8);
        }

        let data_len_bits = (data_len * 8) as isize;
        if bs.valid_bits() < data_len_bits {
            return Err(AacDecoderError::DecodeFrameError);
        }

        if data_byte_align_flag {
            bs.align(align_anchor);
        }

        if data_len > 0 {
            let data_start = bs.valid_bits();

            // ISO/IEC 14496-3/AMD4, section 4.5.2.1:
            // A DSE containing a MPEG4_ancillary_data element shall have the same
            // element_instance_tag as the first channel element it is associated to. It
            // should follow after the last channel or SBR element it is associated to.

            let anc_type = if channel_element_count == decoder_config.num_channel_elements
                && element_instance_tag == self.first_channel_element_instance_tag
                && bs.read(MPEG4_ANC_DATA_SYNC_LEN) == MPEG4_ANC_DATA_SYNC_WORD
            {
                // MPEG4_ancillary_data
                AncDataType::Mpeg4AncillaryData
            } else {
                AncDataType::DataStreamElement
            };

            // Mark data stream element and push it to ancillary_data vector.
            // If capacity exceeded just skip the element.
            if self.ancillary_data.len() < self.ancillary_data.capacity() {
                let anc_data = AncillaryData::new(
                    data_start,
                    data_len as usize,
                    element_instance_tag,
                    anc_type,
                );
                self.ancillary_data.push(anc_data);
            }

            // Move to the very end of the element.
            let valid_bits = bs.valid_bits();
            bs.push(valid_bits - data_start + data_len_bits);
        }

        tp_dec.crc_end_region(crc_reg);

        Ok(())
    }

    /// Applies SBR and MPS to the input PCM samples.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time domain input/ouptut data buffer.
    /// - `scratch_buffer`: Scratch buffer.
    /// - `output_info`: Mutable `OutputInfo` instance.
    /// - `channel_map_descr`: Mutable `ChannelMapDescriptor` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `is_mono_output`: Flag to indicate output channel is mono.
    /// - `flags`: AAC decoder flags.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    #[expect(clippy::too_many_arguments)]
    fn apply_sbr_and_mps(
        &mut self,
        time_data: &mut [f32],
        scratch_buffer: &mut [f32],
        output_info: &mut OutputInfo,
        channel_map_descr: &ChannelMapDescriptor,
        decoder_config: &Config,
        is_mono_output: bool,
        flags: AACDecFlags,
    ) -> Result<(), AacDecoderError> {
        let core_channels = output_info.num_channels;

        let mut error_status = Ok(());

        match self
            .qmf_domain
            .configure(&mut self.qmf_domain_params_requested)
        {
            QmfDomainError::Ok => (),
            QmfDomainError::InitError => return Err(AacDecoderError::Unknown),
        };

        if (decoder_config.ac_flags | self.implicit_ac_flags).contains(ACFlags::SBR_PRESENT) {
            let _ = self
                .sbr_decoder
                .set_param(SbrDecParam::FlushData(flags.contains(AACDecFlags::FLUSH)));

            if !decoder_config.ac_flags.contains(ACFlags::USAC_SCFGI3) {
                let mut is_ps_possible = (!decoder_config
                    .ac_flags
                    .intersects(ACFlags::USAC | ACFlags::ER | ACFlags::MPS_PRESENT))
                    && output_info.num_channels == 1
                    && !is_mono_output;

                if !decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT) {
                    let cp_length =
                        usize::from(output_info.num_channels) * usize::from(output_info.frame_size);
                    scratch_buffer[..cp_length].copy_from_slice(&time_data[..cp_length]);
                }

                let sbr_error = if decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT) {
                    self.sbr_decoder.apply(
                        &mut self.qmf_domain,
                        Some(time_data),
                        &mut None,
                        &mut output_info.frame_size,
                        &mut output_info.num_channels,
                        &mut output_info.sampling_rate,
                        channel_map_descr,
                        usize::from(decoder_config.channel_map_index),
                        (!flags.contains(AACDecFlags::CONCEAL)) as u8,
                        &mut is_ps_possible,
                    )
                } else {
                    self.sbr_decoder.apply(
                        &mut self.qmf_domain,
                        Some(scratch_buffer),
                        &mut Some(time_data),
                        &mut output_info.frame_size,
                        &mut output_info.num_channels,
                        &mut output_info.sampling_rate,
                        channel_map_descr,
                        usize::from(decoder_config.channel_map_index),
                        (!flags.contains(AACDecFlags::CONCEAL)) as u8,
                        &mut is_ps_possible,
                    )
                };

                match sbr_error {
                    Ok(()) => {
                        if !decoder_config
                            .ac_flags
                            .intersects(ACFlags::USAC | ACFlags::ER)
                            && (!decoder_config.ac_flags.contains(ACFlags::PS_PRESENT))
                            && is_ps_possible
                        {
                            self.implicit_ac_flags.insert(ACFlags::PS_PRESENT);
                        }

                        output_info.output_delay = output_info.output_delay
                            * u32::from(output_info.frame_size)
                            / u32::from(decoder_config.frame_length);

                        output_info.output_delay += self.sbr_decoder.get_delay();
                    }
                    Err(SbrError::OutputBufferTooSmall) => {
                        return Err(AacDecoderError::OutputBufferTooSmall);
                    }
                    Err(_) => (),
                }
            }
        }

        if decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT) {
            // Apply residual channel delay.
            if decoder_config.ac_flags.contains(ACFlags::USAC_SCFGI2) {
                if let Some(sig_delay) = self.usac_residual_delay.as_deref_mut() {
                    sig_delay.apply(
                        &mut time_data[usize::from(decoder_config.samples_per_frame)
                            ..2 * usize::from(decoder_config.samples_per_frame)],
                    );
                }
            }

            // Apply SAC processing.
            match self.mpeg_surround_decoder.apply(
                &mut self.qmf_domain,
                &mut self.qmf_domain_params_requested,
                time_data,
                usize::from(decoder_config.samples_per_frame),
                &mut output_info.num_channels,
                &mut output_info.frame_size,
                output_info.sampling_rate,
                decoder_config.aot,
                channel_map_descr,
            ) {
                Ok(()) => {
                    output_info.output_delay += self.mpeg_surround_decoder.get_delay();
                }
                Err(SacDecoderError::OutputBufferTooSmall) => {
                    return Err(AacDecoderError::OutputBufferTooSmall);
                }
                Err(_) => {
                    // Clear the buffer data.
                    time_data.fill(0.0);
                    error_status = Err(AacDecoderError::DecodeFrameError);
                }
            }

            if decoder_config.ac_flags.contains(ACFlags::USAC_SCFGI3) {
                let mut ps_possible = false;

                // Apply SBR processing for Unified Stereo Config Index 3.
                match self.sbr_decoder.apply(
                    &mut self.qmf_domain,
                    None,
                    &mut Some(time_data),
                    &mut output_info.frame_size,
                    &mut output_info.num_channels,
                    &mut output_info.sampling_rate,
                    channel_map_descr,
                    usize::from(decoder_config.channel_map_index),
                    (!flags.contains(AACDecFlags::CONCEAL)) as u8,
                    &mut ps_possible,
                ) {
                    Ok(()) => {
                        output_info.output_delay = output_info.output_delay
                            * u32::from(output_info.frame_size)
                            / u32::from(decoder_config.frame_length);

                        output_info.output_delay += self.sbr_decoder.get_delay();
                    }
                    Err(SbrError::OutputBufferTooSmall) => {
                        return Err(AacDecoderError::OutputBufferTooSmall);
                    }
                    Err(_) => (),
                }
            }
        }

        // Update channel type description (HE-AAC mono SBR, PS, 212).
        if (decoder_config.ac_flags | self.implicit_ac_flags)
            .intersects(ACFlags::SBR_PRESENT | ACFlags::PS_PRESENT | ACFlags::MPS_PRESENT)
            && core_channels < output_info.num_channels
        {
            for ch_idx in core_channels..output_info.num_channels {
                output_info.channel_indices[usize::from(ch_idx)] = ch_idx;
                output_info.channel_type[usize::from(ch_idx)] = AudioChannelType::Front;
            }
        }

        error_status
    }

    /// Applies DRC gains and downmix in the MPEG-D DRC decoder.
    ///
    /// # Parameters
    ///
    /// - `output_info`: Mutable `OutputInfo` instance.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `time_data`: Time domain output data.
    /// - `aacdec_flags`: AAC decoder flag.
    fn apply_uni_drc_gains_and_downmix(
        &mut self,
        output_info: &mut OutputInfo,
        decoder_config: &Config,
        time_data: &mut [f32],
        aacdec_flags: AACDecFlags,
    ) {
        if self.uni_drc_decoder.is_drc_active() {
            // If SBR and/or MPS is active, the DRC gains are aligned to the QMF
            // domain signal before the QMF synthesis. Therefore the DRC gains need
            // to be delayed by the QMF synthesis delay.
            let mut drc_delay = if decoder_config
                .ac_flags
                .intersects(ACFlags::SBR_PRESENT | ACFlags::MPS_PRESENT)
            {
                257
            } else {
                0
            };

            // Take into account concealment delay.
            drc_delay +=
                if output_info.output_delay < u32::from(decoder_config.ext_samples_per_frame) {
                    0
                } else {
                    output_info.frame_size
                };

            // Apply DRC gains.
            let mut channel_gain_db = [0.0_f32; MAX_CHANNELS];
            let _ = self.uni_drc_decoder.set_channel_gains(
                !aacdec_flags.contains(AACDecFlags::FLUSH),
                usize::from(output_info.num_channels),
                &mut channel_gain_db,
            );

            let _ = self.uni_drc_decoder.preprocess();

            // Apply DRC1 gain sequence.
            let _ = self.uni_drc_decoder.process_time(
                ProcessingLocation::Drc1,
                drc_delay,
                0,
                0,
                usize::from(output_info.num_channels),
                time_data,
                usize::from(output_info.frame_size),
            );

            // Apply downmix.
            let mut reverse_channel_map = [0_usize; MAX_CHANNELS];
            for (ch_num, ch) in reverse_channel_map.iter_mut().enumerate() {
                *ch = ch_num;
            }
            let mut num_channels = usize::from(output_info.num_channels);
            let _ = self.uni_drc_decoder.apply_downmix(
                &reverse_channel_map,
                &reverse_channel_map,
                time_data,
                &mut num_channels,
            );
            // Update number of channels.
            output_info.num_channels = num_channels as u8;

            // apply DRC2/3 gain sequence.
            let _ = self.uni_drc_decoder.process_time(
                ProcessingLocation::Drc2Drc3,
                drc_delay,
                0,
                0,
                usize::from(output_info.num_channels),
                time_data,
                usize::from(output_info.frame_size),
            );

            // Return output loudness information for MPEG-D DRC.
            let output_loudness = self.uni_drc_decoder.out_loudness_level();
            if output_loudness == UNDEFINED_LOUDNESS_VALUE {
                // No valid MPEG-D DRC loudness value contained.
                output_info.output_loudness = -1;
            } else {
                // round(-outputLoudness*4) and limit to 0 dB.
                output_info.output_loudness = ((-4.0 * output_loudness + 0.5) as i16).max(0);
            }
        }
    }

    /// Updates parameters of extension data and AAC decoder.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `aac_params`: AAC decoder parameters.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    pub fn update_params(
        &mut self,
        decoder_config: &Config,
        aac_params: &mut Params,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        self.pcm_downmix.params_update(&mut aac_params.pcm_dmx);

        if !decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER)
        {
            if let Some(drc_data) = self.drc_data.as_deref_mut() {
                drc_data.update_params(&mut aac_params.aac_drc);
            }

            // Reinitialize sbr decoder in case ESBR setting has changed.
            if aac_params.is_mpeg4_esbr_active(decoder_config)
                != self.implicit_ac_flags.contains(ACFlags::MPEG4_ESBR)
            {
                self.implicit_ac_flags ^= ACFlags::MPEG4_ESBR;
                self.sbr_decoder.free_mem();
                let _ = self.init_sbr_decoder(decoder_config);
            }
        }

        // Set MPEG-D DRC parameters.
        if aac_params.uni_drc.has_changed {
            if decoder_config.ac_flags.contains(ACFlags::USAC) {
                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::LoudnessNormalizationOn(
                        aac_params.uni_drc.target_loudness <= 0.0,
                    ))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if aac_params.uni_drc.target_loudness <= 0.0
                    && self
                        .uni_drc_decoder
                        .set_param(DrcDecParameters::TargetLoudness(
                            aac_params.uni_drc.target_loudness,
                        ))
                        .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::Compress(
                        aac_params.uni_drc.compress_factor,
                    ))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::Boost(aac_params.uni_drc.boost_factor))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::EffectType(aac_params.uni_drc.effect_type))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::AlbumMode(
                        aac_params.uni_drc.is_album_mode,
                    ))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }

                if self
                    .uni_drc_decoder
                    .set_param(DrcDecParameters::TargetChannelCountRequested(
                        aac_params.max_target_channels(decoder_config),
                    ))
                    .is_err()
                {
                    error_status = Err(AacDecoderError::SetParamFail);
                }
            }

            if error_status.is_ok() {
                aac_params.uni_drc.has_changed = false;
            }
        }

        if self
            .sbr_decoder
            .set_param(SbrDecParam::SystemBitstreamDelay(aac_params.bs_delay))
            .is_err()
        {
            error_status = Err(AacDecoderError::SetParamFail);
        }

        error_status
    }

    /// Reads extension data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Mutable `TransportDec` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `element_id`: Channel element ID.
    /// - `channel_element_index`: Index of the current channel element. It is 255, if the channel
    ///   element is not an audio channel element.
    /// - `channel_element_count`: Last processed channel element.
    /// - `element_count`: Index value for element config buffer in `Config`.
    /// - `au_start_anchor`: AU start bit buffer position for AU byte alignment.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    #[expect(clippy::too_many_arguments)]
    pub fn read(
        &mut self,
        tp_dec: &mut TransportDec,
        decoder_config: &Config,
        element_id: ChannelElementId,
        channel_element_index: u8,
        channel_element_count: u8,
        element_count: u8,
        au_start_anchor: isize,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());
        let bs = tp_dec.bs_mut();

        if element_count >= decoder_config.num_elements {
            return Err(AacDecoderError::ParseError);
        }

        if !decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER)
        {
            if element_id == ChannelElementId::Fil {
                error_status = self.parse_ga_fill_data(
                    tp_dec,
                    decoder_config,
                    usize::from(channel_element_index),
                );
            } else if element_id == ChannelElementId::Pce {
                self.parse_program_config_element(tp_dec, au_start_anchor);
            } else if element_id == ChannelElementId::Dse {
                error_status = self.read_dse(
                    tp_dec,
                    decoder_config,
                    channel_element_count,
                    au_start_anchor,
                );
            } else if element_id == ChannelElementId::End {
                // Parse MPEG4_ancillary_data() metadata if present.
                for anc_data in self.ancillary_data.iter() {
                    if anc_data.anc_type() == AncDataType::Mpeg4AncillaryData {
                        self.parse_mpeg_ancillary_data(
                            bs,
                            anc_data.data_start(),
                            anc_data.data_len() as u32,
                        );
                        break;
                    }
                }

                // Pass matrix mixdown info to DMX if PCE is present.
                if self.implicit_ac_flags.contains(ACFlags::PCE) {
                    if let Some(pce) = self.pce.as_deref() {
                        self.pcm_downmix.set_matrix_mixdown_from_pce(
                            pce.is_matrix_mixdown_index_present(),
                            pce.matrix_mixdown_index(),
                        );
                    }
                }

                // Extract DRC control data and map it to channels.
                if let Some(drc_data) = self.drc_data.as_deref_mut() {
                    if drc_data.decode() {
                        // If at least one DRC thread has been mapped to a channel there was
                        // DRC data in the bitstream.
                        self.implicit_ac_flags.insert(ACFlags::DRC_PRESENT);
                    }
                }
            } else {
                error_status = Err(AacDecoderError::Unknown);
            }
        } else if decoder_config.ac_flags.contains(ACFlags::USAC) {
            if element_id.is_channel_element() {
                if element_id != ChannelElementId::UsacLfe {
                    error_status = self.parse_usac_mps_and_sbr_data(
                        tp_dec,
                        decoder_config,
                        usize::from(channel_element_index),
                        usize::from(element_count),
                    );
                }
            } else if element_id == ChannelElementId::UsacExt {
                let usac_ext_element_config =
                    &decoder_config.element_config[usize::from(element_count)].extension;
                error_status = self.parse_usac_ext_element(bs, usac_ext_element_config);
            } else {
                error_status = Err(AacDecoderError::Unknown);
            }
        } else if decoder_config.ac_flags.contains(ACFlags::ER) {
            if element_id == ChannelElementId::Ext {
                error_status = self.parse_er_ext_data(tp_dec, decoder_config);
            } else {
                error_status = Err(AacDecoderError::Unknown);
            }
        } else {
            error_status = Err(AacDecoderError::Unknown);
        }

        if element_id == ChannelElementId::End {
            Ok(())
        } else {
            error_status
        }
    }

    /// Feeds DRC channel data into SBR decoder.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `window_sequence`: List of window sequences.
    /// - `element_channels`: Number of element channels.
    /// - `channel_offset`: Channel offset value.
    /// - `channel_element_index`: Index of the current channel element. It is 255, if the channel
    ///   element is not an audio channel element.
    /// - `channel_element_count`: Last processed channel element.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    pub fn feed_drc_element(
        &mut self,
        decoder_config: &Config,
        window_sequence: &[BlockType],
        channel_offset: usize,
        channel_element_index: usize,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());
        if !decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER)
            && (decoder_config.ac_flags | self.implicit_ac_flags).contains(ACFlags::SBR_PRESENT)
        {
            if let Some(drc_data) = self.drc_data.as_deref() {
                if drc_data.is_enabled() {
                    for (channel, win_sequence) in window_sequence.iter().enumerate() {
                        // Feed factors into SBR decoder for application in QMF domain.
                        if self
                            .sbr_decoder
                            .drc_feed_channel(
                                channel_element_index,
                                AudioChannel::from(channel),
                                drc_data.num_bands(0, channel_offset + channel),
                                drc_data.factor(channel_offset + channel),
                                drc_data
                                    .interpolation_scheme(0, channel_offset + channel)
                                    .try_into()
                                    .unwrap(),
                                *win_sequence as u8,
                                drc_data.band_top(0, channel_offset + channel),
                            )
                            .is_err()
                        {
                            error_status = Err(AacDecoderError::Unknown);
                        }
                    }
                } else {
                    for channel in 0..window_sequence.len() {
                        let _ = self
                            .sbr_decoder
                            .drc_disable(channel_element_index, AudioChannel::from(channel));
                    }
                }
            } else {
                // Note: when DRC data instance is None.
                for channel in 0..window_sequence.len() {
                    let _ = self
                        .sbr_decoder
                        .drc_disable(channel_element_index, AudioChannel::from(channel));
                }
            }
        }

        error_status
    }

    /// Signals bit stream interruption and resets DRC and downmix metadata only if
    /// `is_explicit_interruption` is `true`.
    pub fn interrupt(&mut self, decoder_config: &Config, is_explicit_interruption: bool) {
        if (decoder_config.ac_flags | self.implicit_ac_flags).contains(ACFlags::SBR_PRESENT) {
            let _ = self.sbr_decoder.set_param(SbrDecParam::BsInterruption);

            if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                let _ = self.init_sbr_decoder(decoder_config);
            }
        }

        if decoder_config.ac_flags.contains(ACFlags::MPS_PRESENT) {
            let _ = self
                .mpeg_surround_decoder
                .set_params(SacDecParam::BsInterruption, 1);
        }

        // Reset drc and downmix metadata only if explicit interruption requested.
        if is_explicit_interruption {
            // Delete data from the past (e.g. mixdown coeficients).
            self.pcm_downmix.reset();

            if !decoder_config
                .ac_flags
                .intersects(ACFlags::USAC | ACFlags::ER)
            {
                // Reinitialize drc with default settings.
                if let Some(drc_data) = self.drc_data.as_deref_mut() {
                    if let Some(pce) = self.pce.as_deref() {
                        drc_data.init(
                            &decoder_config.channel_type
                                [..usize::from(decoder_config.num_channels)],
                            pce.element_instance_tag(),
                        );
                    }
                }
            }
        }
    }

    /// Applies SBR, MPS, MPEG-D DRC gains and PCM downmixing to the input PCM samples.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time domain input/output data buffer.
    /// - `scratch_buffer`: Scratch buffer.
    /// - `output_info`: Mutable `OutputInfo` instance.
    /// - `channel_map_descr`: Mutable `ChannelMapDescriptor` instance with valid internal data.
    /// - `decoder_config`: `Config` instance with valid internal data.
    /// - `is_mono_output`: Flag to indicate whether the output is mono.
    /// - `aacdec_flags`: AAC decoder flags.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    #[expect(clippy::too_many_arguments)]
    pub fn apply(
        &mut self,
        time_data: &mut [f32],
        scratch_buffer: &mut [f32],
        output_info: &mut OutputInfo,
        channel_map_descr: &ChannelMapDescriptor,
        decoder_config: &Config,
        is_mono_output: bool,
        aacdec_flags: AACDecFlags,
    ) -> Result<(), AacDecoderError> {
        // Apply SBR and MPS.
        let error_status = self.apply_sbr_and_mps(
            time_data,
            scratch_buffer,
            output_info,
            channel_map_descr,
            decoder_config,
            is_mono_output,
            aacdec_flags,
        );

        // Apply MPEG-D DRC gains and downmix.
        if decoder_config.ac_flags.contains(ACFlags::USAC) && error_status.is_output_valid() {
            self.apply_uni_drc_gains_and_downmix(
                output_info,
                decoder_config,
                time_data,
                aacdec_flags,
            );
        }

        error_status
    }

    /// Applies the PCM downmixing or channel expansion to the input PCM samples.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time domain input/ouptut data buffer.
    /// - `time_data_len`: Length of `time_data` buffer.
    /// - `scratch_buffer`: Scratch buffer.
    /// - `scratch_buffer_len`: Length of scratch buffer.
    /// - `output_info`: Mutable `OutputInfo` instance.
    /// - `channel_map_descr`: Mutable `ChannelMapDescriptor` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), AacDecoderError>
    pub fn apply_pcm_downmix(
        &mut self,
        time_data: &mut [f32],
        scratch_buffer: &mut [f32],
        output_info: &mut OutputInfo,
        channel_map_descr: &mut ChannelMapDescriptor,
    ) -> Result<(), AacDecoderError> {
        // Apply downmix
        let result = self.pcm_downmix.apply_frame(
            time_data,
            scratch_buffer,
            output_info.frame_size,
            &mut output_info.num_channels,
            &mut output_info.channel_type,
            &mut output_info.channel_indices,
            channel_map_descr,
        );

        // In case of error, announce the framework that the current combination of channel
        // configuration and downmix settings are not know to produce a
        // predictable behavior and thus maybe produce strange output.
        match result {
            Ok(_) => Ok(()),
            Err(err) => Err(err.into()),
        }
    }

    /// Sets `first channel element instance tag` value.
    pub fn set_first_channel_element_instance_tag(&mut self, element_instance_tag: u8) {
        self.first_channel_element_instance_tag = element_instance_tag
    }

    /// Returns info on whether the USAC pseudo LR feature is active.
    pub fn is_pseudo_lr(&self, decoder_config: &Config) -> bool {
        decoder_config.ac_flags.contains(ACFlags::USAC)
            && decoder_config.num_channels == 2
            && self.mpeg_surround_decoder.is_pseudo_lr()
    }

    /// Resets audio codec flags, DRC and ancillary data.
    pub fn reset(&mut self) {
        if let Some(drc_data) = self.drc_data.as_deref_mut() {
            drc_data.reset();
        }
        self.ancillary_data.clear();
        self.implicit_ac_flags.remove(ACFlags::PCE);
    }

    /// Deinitializez its internal allocated memory.
    pub fn deinit(&mut self) {
        // SBR decoder data.
        self.sbr_decoder.free_mem();

        // MPS data.
        self.mpeg_surround_decoder.free_internal_mem();

        // Ancillary data.
        self.ancillary_data.clear();
        self.ancillary_data.shrink_to_fit();

        // Signal delay.
        if let Some(resi_delay) = self.usac_residual_delay.as_deref_mut() {
            resi_delay.destroy();
        }
        self.usac_residual_delay = None;

        // QMF domain.
        self.qmf_domain.deinit();

        // QMF domain params requested.
        self.qmf_domain_params_requested = QmfDomainParams::default();

        // AAC DRC data.
        self.drc_data = None;

        // PCE data.
        self.pce = None;
    }

    /// Deallocates heap allocated internal sub-components.
    pub fn destroy(&mut self) {
        self.deinit();
    }

    /// Returns a mutable reference to qmf domain.
    pub fn qmf_domain_mut(&mut self) -> &mut QmfDomain {
        &mut self.qmf_domain
    }

    /// Returns a mutable reference to qmf domain.
    pub fn mpeg_surround_decoder_mut(&mut self) -> &mut MpegSurroundDecoder {
        &mut self.mpeg_surround_decoder
    }

    /// Returns a mutable reference to SBR decoder.
    pub fn sbr_decoder_mut(&mut self) -> &mut SbrDecoder {
        &mut self.sbr_decoder
    }

    /// Returns a mutable reference to ancillary data.
    pub fn ancillary_data_mut(&mut self, index: usize) -> Option<&mut AncillaryData> {
        if self.ancillary_data.len() > index {
            Some(&mut self.ancillary_data[index])
        } else {
            None
        }
    }

    /// Returns a mutable reference to AAC DRC data.
    pub fn drc_data_mut(&mut self) -> Option<&mut AacDrcData> {
        self.drc_data.as_deref_mut()
    }

    /// Returns a mutable reference to PCE.
    pub fn pce(&mut self) -> Option<&mut ProgramConfig> {
        self.pce.as_deref_mut()
    }

    /// Returns a mutable reference to UniDRC decoder.
    pub fn uni_drc_decoder(&mut self) -> &mut DrcDecoder {
        &mut self.uni_drc_decoder
    }

    /// Returns a mutable reference to USAC delay.
    pub fn usac_residual_delay(&mut self) -> Option<&mut SignalDelay> {
        self.usac_residual_delay.as_deref_mut()
    }
}
