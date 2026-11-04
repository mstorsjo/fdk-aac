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
//! Spatial Audio Coding decoder.

use super::error_codes::SacDecoderError;
use super::{
    bit_dec::BsData,
    calc_m2::M2Data,
    common::{DataType, DecorrConf, PhaseCoding, QuantMode, Syntax, TsConf},
    conceal::ConcealmentInfo,
    constants::{MAX_INPUT_CHANNELS, MAX_TIME_SLOTS},
    ges::GesData,
    process::{HybData, QmfSlot},
    ssc::SpatialSpecificConfig,
    stp::StpDec,
    tables::CLIP_GAIN_TABLE,
    tsd::TsdData,
};

use crate::common::{
    bitstream::Bitstream,
    channel_map_descr::ChannelMapDescriptor,
    decorrelator::{DecorrDec, DecorrType},
    enums::StereoCfgIndex,
    qmf_domain::{QmfDomain, QmfDomainSelect},
};
use crate::sac_dec::flags::{SacDecCtrlFlags, SacDecCtrlInitFlags};
use itertools::izip;

/// Gain to be applied to qmf data if bypass mode is active.
const BYPASS_MODE_GAIN: f32 = 1.0;
/// Gain to be applied to qmf data if bypass mode is inactive.
const ANA_QMF_PRE_GAIN: f32 = 1.0 / 32768.0;

/// Input mode for spatial decoder.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub(super) enum SpatialDecInputMode {
    /// QMF SBR mode.
    QmfSbr = 1001,
    /// Time domain data mode.
    Time = 1002,
}

/// Enum representing different M2 application modes.
#[repr(C)]
#[derive(Debug)]
enum M2Application {
    /// Apply M2 fallback implementation.
    ApplyM2 = 1,
    /// Apply M2 for 212 mode.
    ApplyM2Mode212 = 2,
    /// Apply M2 for 212 mode with residuals and phase coding.
    ApplyM2Mode212ResPhaseCoding = 3,
}

#[repr(C)]
#[derive(Default, Debug)]
pub(super) struct SpatialDec {
    /// Field to store internal errors.
    // Will be cleared at the very beginning of each process call.
    internal_error: SacDecoderError,
    /// Sampling frequency in Hz.
    sampling_freq: u32,
    /// Number of input channels. 1 - mono; 2 - stereo (L, R).
    num_input_channels: u8,
    /// Number of output channels.
    num_output_channels: u8,
    /// Quantization mode.
    quant_mode: QuantMode,
    /// Temporal shape configuration.
    temp_shape_config: TsConf,
    /// Decorrelator configuration.
    decorr_conf: DecorrConf,
    /// Global gain for upmix.
    clip_protect_gain: f32,
    /// Total number of time slots.
    num_time_slots: u8,
    /// Current time slots.
    cur_time_slot: u8,
    /// Previous time slots. It contains number of previous time slot + 1.
    prev_time_slot: u8,
    /// Index for current parameter time slot.
    cur_ps: u8,
    /// Flag to indicate delay is shared with SBR tool.
    share_delay_with_sbr: bool,
    /// Number of QMF bands.
    qmf_bands: u8,
    // Residual coding.
    /// Flag to indicate presence of residual coding.
    is_residual_coding: bool,
    /// Number of residual bands. 0 if no residual data present for this box.
    residual_bands: u8,
    // Time mapping.
    /// Flag to indicate extended SAC frame.
    is_extended_frame: bool,
    /// Stereo configuration index value.
    stereo_config_index: StereoCfgIndex,
    /// `Decorrelator` structure.
    decorr_dec: DecorrDec,
    /// `GesData` structure.
    ges_data: GesData,
    /// `StpDec` structure.
    stp_dec: StpDec,
    /// `TsdData` structure.
    tsd_data: TsdData,
    /// Spatial dec `ConcealmentInfo` structure.
    conceal_info: ConcealmentInfo,
    /// Hybrid Data structure.
    hyb_data: HybData,
    /// `M2Data` structure.
    m2_data: M2Data,
    /// Flag to indicate presence of new `BsData`.
    is_new_bs_data: bool,
    /// `BsData` structure.
    bs_data: BsData,
}

impl SpatialDec {
    /// Creates a new `SpatialDec` instance.
    ///
    /// # Parameters
    ///
    /// - `stereo_config_index`: Stereo config index value [0..3].
    /// - `is_usac`: Flag to indicate whether USAC is active or not.
    pub(super) fn new(stereo_config_index: StereoCfgIndex, is_usac: bool) -> SpatialDec {
        let mut spatial_dec = Self {
            ges_data: GesData::new(),
            stp_dec: StpDec::new(),
            conceal_info: ConcealmentInfo::new(),
            m2_data: M2Data::new(),
            bs_data: BsData::new(),
            ..Default::default()
        };

        if is_usac {
            spatial_dec.hyb_data = HybData::new(stereo_config_index);
        }

        spatial_dec.conceal_info.init();
        spatial_dec.is_new_bs_data = true;
        let _ = spatial_dec.bs_data.init_data();

        spatial_dec
    }

    /// Returns independency flag.
    pub(super) fn bs_independency_flag(&self) -> bool {
        self.bs_data.independency_flag()
    }

    /// Returns number of output channels.
    pub(super) fn num_output_channels(&self) -> u8 {
        self.num_output_channels
    }

    /// Returns stereo config index value.
    pub(super) fn stereo_config_index(&self) -> StereoCfgIndex {
        self.stereo_config_index
    }

    /// Returns error status.
    pub(super) fn has_error(&self) -> bool {
        self.internal_error != SacDecoderError::Ok
    }

    /// Resets `self.is_new_bs_data` flag.
    pub(super) fn reset_new_bs_data(&mut self) {
        self.is_new_bs_data = false;
    }

    /// Decodes header info of spatial decoder's sub-components.
    ///
    /// # Parameters
    ///
    /// - `ssc`: `SpatialSpecificConfig` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    pub fn decode_header(&mut self, ssc: &SpatialSpecificConfig) -> Result<(), SacDecoderError> {
        self.sampling_freq = ssc.sampling_freq();
        self.num_time_slots = ssc.n_time_slots();
        self.num_input_channels = ssc.n_input_channels();
        self.num_output_channels = ssc.n_output_channels();
        self.quant_mode = ssc.quant_mode();
        self.temp_shape_config = ssc.temp_shape_config();
        self.decorr_conf = ssc.decorr_config();
        self.is_residual_coding = false;
        self.residual_bands = 0;

        if ssc.is_usac() && ssc.is_residual_coding() {
            self.is_residual_coding = ssc.is_residual_coding();
            self.residual_bands = ssc.n_residual_bands();
        }

        self.clip_protect_gain = CLIP_GAIN_TABLE[usize::from(ssc.bs_fixed_gain_dmx())];

        self.m2_data.decode_header(
            ssc.freq_res(),
            ssc.n_output_channels(),
            ssc.num_ott_bands_ipd(),
            PhaseCoding::from(ssc.bs_phase_coding()),
        );

        let err = self
            .hyb_data
            .decode_header(ssc.get_n_qmf_bands(), ssc.freq_res(), ssc.is_eld());

        if err == SacDecoderError::Ok {
            Ok(())
        } else {
            Err(err)
        }
    }

    /// Initializes the spatial decoder with the specified configuration, including setting up
    /// QMF bands, error concealment, M1 and M2 parameters, subband temporal processing, GES,
    /// hybrid filter, and decorrelator, based on the provided SSC and initialization flags.
    /// The initializaton is triggered after the first parsing.
    ///
    /// # Parameters
    ///
    /// - `ssc`: `SpatialSpecificConfig` instance with valid internal data.
    /// - `qmf_bands`: Number of QMF bands to be used.
    /// - `init_flags`: Flags indicating which components to be initialized.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    pub(super) fn init(
        &mut self,
        ssc: &SpatialSpecificConfig,
        qmf_bands: u8,
        init_flags: SacDecCtrlInitFlags,
    ) -> Result<(), SacDecoderError> {
        self.qmf_bands = qmf_bands;
        self.share_delay_with_sbr = false;
        self.stereo_config_index = ssc.stereo_config_index();
        self.cur_ps = 0;
        self.prev_time_slot = 0;
        // Initialize with a invalid value to trigger concealment if first frame has no valid data.
        self.cur_time_slot = MAX_TIME_SLOTS as u8;

        // Initialization of header parameter.
        self.decode_header(ssc)?;

        // Initialization of concealment.
        if init_flags.contains(SacDecCtrlInitFlags::STATES_ERROR_CONCEALMENT) {
            self.conceal_info.reset();
        }

        // Initialization of M2 parameter.
        self.m2_data.init(
            usize::from(self.num_output_channels),
            init_flags.contains(SacDecCtrlInitFlags::STATES_M2),
            init_flags.contains(SacDecCtrlInitFlags::STATES_SMOOTHING),
            init_flags.contains(SacDecCtrlInitFlags::STATES_OVERWRITE_M2),
        );

        // Initialization of STP.
        self.stp_dec.init();

        // Initialization of GES.
        self.ges_data
            .init(init_flags.contains(SacDecCtrlInitFlags::STATES_GES));

        if ssc.is_usac() {
            let err = self.hyb_data.init(
                self.qmf_bands,
                init_flags.contains(SacDecCtrlInitFlags::STATES_ANA_HYB_FILTER),
            );
            if err != SacDecoderError::Ok {
                return Err(SacDecoderError::NotOk);
            }
        }

        // Initialization of decorrelator.
        let decorr_type = if ssc.is_eld() {
            DecorrType::Ld
        } else {
            DecorrType::Usac
        };

        let err = self.decorr_dec.init(
            self.hyb_data.hybrid_bands,
            decorr_type,
            usize::from(self.decorr_conf),
            init_flags.contains(SacDecCtrlInitFlags::STATES_DECORRELATOR),
        );
        if err != 0 {
            return Err(SacDecoderError::NotOk);
        }

        Ok(())
    }

    /// Initializes the OTT data of `BsData`.
    pub(super) fn init_bs_parser_context(&mut self) {
        self.bs_data.init_parser_context();
    }

    /// Parses SAC frame data.
    ///
    /// # Parameters
    ///
    /// - `bs`: `Bitstream` instance with valid internal data.
    /// - `ssc`: `SpatialSpecificConfig` instance with valid internal data.
    /// - `global_independency_flag`: Independency flag to parse frame data.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    pub(super) fn parse_bs_frame_data(
        &mut self,
        bs: &mut Bitstream,
        ssc: &SpatialSpecificConfig,
        global_independency_flag: bool,
    ) -> Result<(), SacDecoderError> {
        self.bs_data.parse_frame_data(
            &mut self.m2_data,
            &mut self.tsd_data,
            &mut self.stp_dec,
            &mut self.ges_data,
            bs,
            ssc,
            global_independency_flag,
        )?;

        self.is_new_bs_data = true;
        Ok(())
    }

    /// Clears frame data to avoid misconfiguration and allow proper error concealment.
    fn clear_frame_data(&mut self) {
        self.stp_dec.clear_data();
        self.ges_data.clear_data();
        self.tsd_data.clear();
        self.bs_data.clear_frame_data(self.num_time_slots);
        self.m2_data.init_parameter_smoothing();
    }

    /// Performs decomposition of the input/QMF signal into sub-bands using analysis filterbank.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`: `QmfDomain` instance with valid internal data.
    /// - `input_data`: Time domain input data.
    /// - `input_mode`: Input mode(`SpatialdecInputMode`) to be used (time or QMF interface).
    /// - `syntax`: `Syntax` instance which indicates used bitstream format.
    /// - `num_ts`: Time slot to be processed in spatial frame.
    /// - `is_bypass_mode`: Indicates if bypass mode is enabled.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    fn analysis(
        &mut self,
        qmf_domain: &mut QmfDomain,
        input_data: &[f32],
        input_mode: SpatialDecInputMode,
        syntax: Syntax,
        num_ts: usize,
        is_bypass_mode: bool,
    ) -> Result<(), SacDecoderError> {
        let mut qmf_slot_residuals = QmfSlot::new();

        let gain = if is_bypass_mode {
            BYPASS_MODE_GAIN
        } else {
            self.clip_protect_gain * ANA_QMF_PRE_GAIN
        };

        let qmf_slot = match input_mode {
            SpatialDecInputMode::QmfSbr => {
                self.share_delay_with_sbr = !syntax.is_eld;

                QmfSlot::feed(
                    qmf_domain,
                    gain,
                    num_ts.try_into().unwrap(),
                    self.qmf_bands,
                    self.share_delay_with_sbr,
                )
            }
            SpatialDecInputMode::Time => {
                self.share_delay_with_sbr = false;
                let input_start = usize::from(qmf_domain.num_bands_analysis()) * num_ts;
                let qmf_analysis_fb = qmf_domain
                    .get_filter_bank_mut(0, QmfDomainSelect::QmfDomainIn)
                    .unwrap();
                let input_length = usize::from(qmf_analysis_fb.num_subbands());
                QmfSlot::analysis(
                    qmf_analysis_fb,
                    &input_data[input_start..input_start + input_length],
                    gain,
                    self.qmf_bands,
                )
            }
        };

        if self.is_residual_coding {
            let offset = if self.stereo_config_index == StereoCfgIndex::Mps212wResMonoSbr {
                // Offset for USAC stereo config index 2: residual signal is located at
                // the end of ch 1 of the output buffer.
                usize::from(qmf_domain.num_qmf_time_slots())
                    * (2 * usize::from(qmf_domain.num_bands_synthesis())
                        - usize::from(qmf_domain.num_bands_analysis()))
            } else {
                // Offset for USAC stereo config index 3: residual signal is located
                // in channel 1 of the input buffer.
                usize::from(qmf_domain.num_bands_analysis())
                    * usize::from(qmf_domain.num_qmf_time_slots())
            };

            let input_start = usize::from(qmf_domain.num_bands_analysis()) * num_ts + offset;
            let qmf_analysis_fb = qmf_domain
                .get_filter_bank_mut(1, QmfDomainSelect::QmfDomainIn)
                .unwrap();
            let input_length = usize::from(qmf_analysis_fb.num_subbands());
            qmf_slot_residuals = QmfSlot::analysis(
                qmf_analysis_fb,
                &input_data[input_start..input_start + input_length],
                gain,
                self.qmf_bands,
            );
        }

        debug_assert!(
            !(syntax.is_usac
                && self.stereo_config_index == StereoCfgIndex::Mps212wResStereoSbr
                && self.share_delay_with_sbr)
        );
        debug_assert!(
            !syntax.is_usac || (self.qmf_bands == self.hyb_data.hyb_analysis_num_bands(0))
        );

        let err = self.hyb_data.analysis(
            &qmf_slot,
            &qmf_slot_residuals,
            self.share_delay_with_sbr,
            syntax.is_eld,
            self.is_residual_coding,
        );

        if err != SacDecoderError::Ok {
            Err(err)
        } else {
            Ok(())
        }
    }

    /// Performs synthesis filtering on a given input QMF sub-bands.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`: `QmfDomain` instance with valid internal data.
    /// - `map_descr`: `ChannelMapDescriptor` instance with valid internal data.
    /// - `time_out`: Input/output buffer.
    /// - `syntax`: `Syntax` instance which indicates used bitstream format.
    /// - `num_ts`: Time slot to be processed in spatial frame.
    /// - `num_samples`: Number of samples per output frame.
    /// - `is_bypass_mode`: Indicates if bypass mode is enabled.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    #[expect(clippy::too_many_arguments)]
    fn synthesis(
        &mut self,
        qmf_domain: &mut QmfDomain,
        map_descr: &ChannelMapDescriptor,
        time_out: &mut [f32],
        syntax: Syntax,
        num_ts: usize,
        num_samples: u16,
        is_bypass_mode: bool,
    ) -> Result<(), SacDecoderError> {
        let num_bands_synthesis = usize::from(qmf_domain.num_bands_synthesis());
        let offset_tmp =
            (num_samples / u16::from(self.qmf_bands)).min(u16::from(self.num_time_slots));
        let offset = num_bands_synthesis * usize::from(offset_tmp);

        let mut qmf_slot = QmfSlot::new();
        for ch in 0..self.num_output_channels as usize {
            if syntax.is_eld {
                // No hybrid filtering. Just copy the QMF data.
                for (cplx_dry, band_real, band_imag) in izip!(
                    self.hyb_data.hyb_output_cplx_dry[ch].iter(),
                    qmf_slot.bands_real.iter_mut(),
                    qmf_slot.bands_imag.iter_mut(),
                )
                .take(self.hyb_data.hybrid_bands)
                {
                    *band_real = cplx_dry.re;
                    *band_imag = cplx_dry.im;
                }
            } else {
                let hybrid_synthesis = self.hyb_data.hybrid_synthesis(ch);
                let hyb_output_cplx_dry = &self.hyb_data.hyb_output_cplx_dry[ch];
                hybrid_synthesis.apply(
                    hyb_output_cplx_dry,
                    &mut qmf_slot.bands_real,
                    &mut qmf_slot.bands_imag,
                );
            }

            // Map channel indices from MPEG Surround -> PCE style -> channelMapping[].
            let out_ch =
                map_descr.get_map_value(ch as u8, usize::from(self.num_output_channels)) as usize;

            if self.stereo_config_index == StereoCfgIndex::Mps212wResStereoSbr {
                // MPS -> SBR
                let scale_post = 32768.0_f32;
                let num_qmf_bands = usize::from(self.qmf_bands);

                qmf_domain.set_slot(
                    &qmf_slot.bands_real[..num_qmf_bands],
                    &qmf_slot.bands_imag[..num_qmf_bands],
                    scale_post,
                    out_ch,
                    num_ts,
                );
            } else {
                let qmf_synthesis_option =
                    qmf_domain.get_filter_bank_mut(out_ch, QmfDomainSelect::QmfDomainOut);

                if let Some(qmf_synthesis) = qmf_synthesis_option {
                    if !is_bypass_mode {
                        let scale_post = 32768.0_f32;
                        qmf_synthesis.set_output_gain(scale_post);
                    }

                    // Call the QMF synthesis for dry.
                    let out_length = num_ts * num_bands_synthesis + offset * out_ch;
                    let qmf_syn_bands = qmf_synthesis.num_subbands() as usize;

                    qmf_synthesis.synthesis_filtering_slot(
                        &qmf_slot.bands_real,
                        Some(&qmf_slot.bands_imag),
                        &mut time_out[out_length..out_length + qmf_syn_bands],
                    );
                } else {
                    return Err(SacDecoderError::NotOk);
                }
            }
        }
        Ok(())
    }

    /// Applies parameter sets for each time slot.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`: `QmfDomain` instance with valid internal data.
    /// - `mode`: Input mode(`SpatialdecInputMode`) to be used (time or QMF interface).
    /// - `map_descr`: `ChannelMapDescriptor` instance with valid internal data.
    /// - `syntax`: `Syntax` instance which indicates used bitstream format.
    /// - `time_data`: Input/output buffer.
    /// - `ctrl_flags`: Control flags.
    /// - `num_samples`: Number of audio samples per channel of down mix input data (frame length).
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    #[expect(clippy::too_many_arguments)]
    fn apply_parameter_sets(
        &mut self,
        qmf_domain: &mut QmfDomain,
        mode: SpatialDecInputMode,
        map_descr: &ChannelMapDescriptor,
        syntax: Syntax,
        time_data: &mut [f32],
        ctrl_flags: SacDecCtrlFlags,
        num_samples: u16,
    ) -> Result<(), SacDecoderError> {
        let mut ps = usize::from(self.cur_ps);
        let mut is_bypass_mode = ctrl_flags.contains(SacDecCtrlFlags::MPEGS_BYPASSMODE);
        let mut prev_slot = self.prev_time_slot;
        let ts_start = self.cur_time_slot;
        let ts_stop = (self.cur_time_slot + (num_samples / u16::from(self.qmf_bands)) as u8)
            .min(self.num_time_slots);

        for (ts_io, ts) in (ts_start..ts_stop).enumerate() {
            let cur_slot = self.bs_data.param_time_slot(ps);

            if cur_slot < ts {
                is_bypass_mode = true;
                if self.internal_error == SacDecoderError::Ok {
                    // Store internal error before it gets overwritten.
                    self.internal_error = SacDecoderError::WrongParametersets;
                }
            }

            self.analysis(qmf_domain, time_data, mode, syntax, ts_io, is_bypass_mode)?;

            if is_bypass_mode {
                self.hyb_data.apply_bypass();
            } else {
                if ts == prev_slot {
                    let param_time_slots = self.bs_data.param_time_slots();
                    let quant_coarse = self
                        .bs_data
                        .bs_quant_coarse_for_param_slot(DataType::Ipd, ps);
                    let is_phase_mode = self.bs_data.phase_mode();
                    self.m2_data.calculate_m2(
                        param_time_slots,
                        ps,
                        self.num_time_slots,
                        quant_coarse,
                        is_phase_mode,
                        self.is_residual_coding,
                        usize::from(self.residual_bands),
                    );
                }

                let alpha = (1 + ts - prev_slot) as f32 / (1 + cur_slot - prev_slot) as f32;

                let err = self.hyb_data.apply_m1_create_w_mode_212(
                    &mut self.tsd_data,
                    &mut self.decorr_dec,
                    self.residual_bands,
                );
                if err != SacDecoderError::Ok {
                    return Err(err);
                }

                let mut apply_m2_config = M2Application::ApplyM2;
                if self.temp_shape_config != TsConf::TpWhite
                    && self.temp_shape_config != TsConf::Tes
                {
                    apply_m2_config = if syntax.is_usac
                        && self.m2_data.phase_coding() == PhaseCoding::IpdPrediction
                    {
                        M2Application::ApplyM2Mode212ResPhaseCoding
                    } else {
                        M2Application::ApplyM2Mode212
                    };
                }

                let err = match apply_m2_config {
                    M2Application::ApplyM2 => {
                        self.hyb_data
                            .apply_m2(&self.m2_data, self.residual_bands, alpha)
                    }
                    M2Application::ApplyM2Mode212 => {
                        self.hyb_data.apply_m2_mode_212(&mut self.m2_data, alpha)
                    }
                    M2Application::ApplyM2Mode212ResPhaseCoding => self
                        .hyb_data
                        .apply_m2_mode_212_residuals_plus_phase_coding(&self.m2_data, alpha),
                };
                if err != SacDecoderError::Ok {
                    return Err(err);
                }

                if self.temp_shape_config == TsConf::Tes {
                    self.ges_data.reshape(
                        &mut self.hyb_data,
                        usize::from(self.num_output_channels),
                        usize::from(ts),
                    );

                    self.hyb_data.add_dry_wet(self.num_output_channels);
                }

                if self.temp_shape_config == TsConf::TpWhite {
                    self.stp_dec.apply(&mut self.hyb_data);
                }

                if self.m2_data.phase_coding() == PhaseCoding::Ipd {
                    self.hyb_data.apply_phase(&self.m2_data, alpha);
                }
            }

            self.synthesis(
                qmf_domain,
                map_descr,
                time_data,
                syntax,
                ts_io,
                num_samples,
                is_bypass_mode,
            )?;

            if ts == cur_slot {
                self.m2_data.buffer_matrices();
                prev_slot = cur_slot + 1;
                ps += 1;
            }
        }

        self.prev_time_slot = prev_slot;
        self.cur_time_slot = ts_stop;
        self.cur_ps = ps as u8;
        Ok(())
    }

    /// Applies decoded MPEG Surround parameters to time domain or QMF down mix data.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`: `QmfDomain` instance with valid internal data.
    /// - `input_mode`: Input mode(`SpatialdecInputMode`) to be used (time or QMF interface).
    /// - `map_descr`: `ChannelMapDescriptor` instance with valid internal data.
    /// - `syntax`: `Syntax` instance which indicates used bitstream format.
    /// - `time_data`: Input/output buffer.
    /// - `ctrl_flags`: Mutable reference to control flags.
    /// - `num_samples`: Number of audio samples per channel of down mix input data (frame length).
    /// - `num_input_channels`: Number of down mix input channels, Useful for internal sanity checks
    ///   and bypass mode.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply_frame(
        &mut self,
        qmf_domain: &mut QmfDomain,
        input_mode: SpatialDecInputMode,
        map_descr: &ChannelMapDescriptor,
        syntax: Syntax,
        time_data: &mut [f32],
        ctrl_flags: &mut SacDecCtrlFlags,
        num_samples: u16,
        num_input_channels: u8,
    ) -> Result<(), SacDecoderError> {
        let mut num_input_ch = num_input_channels;
        // Init internal error.
        self.internal_error = SacDecoderError::Ok;

        let mut control_flags = *ctrl_flags;

        if syntax.is_usac && self.stereo_config_index > StereoCfgIndex::Mps212 {
            // Do not count residual channel as input channel. It is handled separately.
            num_input_ch = 1;
        }

        // Check if amount of input channels is consistent.
        if num_input_ch != self.num_input_channels {
            control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
            if num_input_ch > MAX_INPUT_CHANNELS as u8 {
                return Err(SacDecoderError::InvalidParameter);
            }
        }

        // Determine local function control flags.
        let mut do_dec_and_map_frame_data = self.is_new_bs_data;

        // Assures that conceal flag will not be set for blind mode.
        if ((!do_dec_and_map_frame_data)
            && ((u16::from(self.cur_time_slot) + (num_samples / u16::from(self.qmf_bands)))
                > u16::from(self.num_time_slots)))
            || self.bs_data.num_parameter_sets() == 0
        {
            // New input samples but missing side info.
            do_dec_and_map_frame_data = true;
            control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
        }

        let num_parameter_sets = usize::from(self.bs_data.num_parameter_sets()).saturating_sub(1);

        if !do_dec_and_map_frame_data
            && (self.bs_data.param_time_slot(num_parameter_sets) != (self.num_time_slots - 1)
                || self.cur_time_slot > self.bs_data.param_time_slot(usize::from(self.cur_ps)))
        {
            // Detected faulty parameter slot data.
            do_dec_and_map_frame_data = true;
            control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
        }

        // Update concealment state machine.
        // convert from conceal flag to frame ok flag.
        let is_frame_ok = !control_flags.contains(SacDecCtrlFlags::MPEGS_CONCEAL);
        self.conceal_info.update_state(is_frame_ok);

        let mut tmp_flags = SacDecCtrlFlags::empty();

        if do_dec_and_map_frame_data {
            // Reset spatial framing control variables.
            self.is_new_bs_data = false;
            self.prev_time_slot = 0;
            self.cur_time_slot = 0;
            self.cur_ps = 0;

            if !is_frame_ok {
                // Reset frame data to avoid misconfiguration.
                self.clear_frame_data();
            }

            self.is_extended_frame = self.bs_data.is_extended_frame(self.num_time_slots);

            let err = self.bs_data.decode_frame(
                &mut self.m2_data,
                &mut self.tsd_data,
                &mut self.conceal_info,
                self.quant_mode,
                self.num_time_slots,
                self.is_extended_frame,
            );

            match err {
                Ok(_) => (),
                Err(err) => {
                    // Rescue strategy is to apply bypass mode in order
                    // to keep at least the downmix channels continuous.
                    control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
                    tmp_flags.insert(SacDecCtrlFlags::MPEGS_BYPASSMODE);

                    if self.internal_error == SacDecoderError::Ok {
                        // Store internal error before it gets overwritten.
                        self.internal_error = err;
                    }
                }
            }
        }

        let err = self.apply_parameter_sets(
            qmf_domain,
            input_mode,
            map_descr,
            syntax,
            time_data,
            control_flags | tmp_flags,
            num_samples,
        );

        *ctrl_flags = control_flags;
        err
    }

    /// Deallocates internal dynamic memories.
    pub(super) fn deinit(&mut self) {
        self.decorr_dec.deinit();
        self.hyb_data.deinit();
    }
}
