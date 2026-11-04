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
//! MPEG-D DRC's selection processing input component.

use super::super::{
    common::{self, CodecMode, EffectBit},
    constants as drc_const,
    reader::{
        DrcCoefficientsUniDrc, DrcInstructionsUniDrc, LoudnessInfoSet, LoudnessProcessError,
        MeasurementSystemRequest, MethodDefinitionRequest, UniDrcConfig,
    },
};
use super::{
    effect_type::DrcEffectTypeRequest,
    feature_request::{
        DrcFeatureRequest, DrcFeatureRequestType, LoudnessPreprocessingRequest,
        TargetConfigRequestType,
    },
    selec_data::{self, Selection},
    selec_error::SelectionProcessError,
    selec_user_params::SelProcUserParam,
};
use itertools::izip;

const DEFAULT_LOUDNESS_DEVIATION_MAX: u8 = 63;

/// Input parameters necessary for DRC selection process.
#[repr(C)]
#[derive(Default, Debug)]
pub(super) struct SelectionProcessInput {
    // System parameters.
    pub(super) base_channel_count: u8,
    /// Not supported.
    base_layout: i8,
    pub(super) target_config_request_type: TargetConfigRequestType,
    pub(super) num_downmix_id_requests: u8,
    pub(super) downmix_id_requested: [u8; drc_const::MAX_REQUESTS_DOWNMIX_ID],
    target_layout_requested: u8,
    target_channel_count_requested: u8,
    /// Sample rate. Needed for complexity estimation, currently not supported.
    audio_sample_rate: u32,

    // Loudness normalization parameters.
    is_loudness_normalization_on: bool,
    pub(super) target_loudness: f32,
    pub(super) is_album_mode: bool,
    is_peak_limiter_present: bool,
    /// Maximum loudness deviation value. Resolution: 1dB.
    /// Default value: `DEFAULT_LOUDNESS_DEVIATION_MAX`.
    pub(super) loudness_deviation_max: u8,
    loudness_measurement_method: MethodDefinitionRequest,
    loudness_measurement_system: MeasurementSystemRequest,
    /// Not supported.
    loudness_measurement_pre_proc: LoudnessPreprocessingRequest,
    /// Not supported.
    device_cut_off_frequency: i32,
    loudness_normalization_gain_db_max: f32,
    pub(super) loudness_normalization_gain_modification_db: f32,
    pub(super) output_peak_level_max: f32,

    // Dynamic Range Control parameters.
    is_dynamic_range_control_on: bool,
    pub(super) num_drc_feature_requests: u8,
    pub(super) drc_feature_request_type:
        [DrcFeatureRequestType; drc_const::MAX_REQUESTS_DRC_FEATURE],
    pub(super) drc_feature_request: [DrcFeatureRequest; drc_const::MAX_REQUESTS_DRC_FEATURE],

    // Other parameters.
    pub(super) boost: f32,
    pub(super) compress: f32,
    /// Not supported.
    drc_characteristic_target: u8,
}

impl SelectionProcessInput {
    /// Returns if dynamic range control is on or off.
    pub(super) fn dynamic_range_control_on(&self) -> bool {
        self.is_dynamic_range_control_on
    }

    /// Returns `signal peak level` value which is derived from provided input parameters.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `loudness_infoset`: `LoudnessInfoSet` instance that holds loudness info album and set
    ///   extension information.
    /// - `drc_instruction`: `DrcInstructionsUniDrc` instance that holds payload / metadata of
    ///   `uniDRC` instructions.
    /// - `downmix_id_requested`: Requested `downmix ID` value.
    /// - `is_album_mode`: Flag to indicate the presence of `album mode`.
    ///
    /// # Return
    ///
    /// - `(bool, f32)`
    ///     - `bool` indicates presence of explicit peak information
    ///     - `f32` is the signal peak level value.
    fn get_signal_peak_level(
        uni_drc_config: &UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
        drc_instruction: &DrcInstructionsUniDrc,
        downmix_id_requested: u8,
        is_album_mode: bool,
    ) -> (bool, f32) {
        let mut signal_peak_level = 0.0_f32;
        let downmix_id = downmix_id_requested;
        let mut explicit_peak_information_present = true;
        let mut drc_set_id = drc_instruction.drc_set_id();

        if drc_set_id < 0 {
            drc_set_id = 0;
        }

        let mut is_match_found = loudness_infoset.true_peak_level(
            drc_set_id,
            downmix_id,
            is_album_mode,
            &mut signal_peak_level,
        );

        if !is_match_found {
            is_match_found = loudness_infoset.sample_peak_level(
                drc_set_id,
                downmix_id,
                is_album_mode,
                &mut signal_peak_level,
            );
        }

        if !is_match_found {
            is_match_found = loudness_infoset.true_peak_level(
                0x3F,
                downmix_id,
                is_album_mode,
                &mut signal_peak_level,
            );
        }

        if !is_match_found {
            is_match_found = loudness_infoset.sample_peak_level(
                0x3F,
                downmix_id,
                is_album_mode,
                &mut signal_peak_level,
            );
        }

        if !is_match_found {
            is_match_found =
                drc_instruction.get_limiter_peak_target(downmix_id, &mut signal_peak_level);
        }

        if !is_match_found && downmix_id != 0 {
            let mut downmix_instruction_index = 0;
            let mut downmix_peak_level_db = 0.0_f32;
            let mut signal_peak_level_tmp = 0.0_f32;

            explicit_peak_information_present = false;

            if uni_drc_config
                .downmix_coefficients_are_present(downmix_id, &mut downmix_instruction_index)
            {
                let downmix_instr =
                    &uni_drc_config.downmix_instructions()[downmix_instruction_index];
                let base_channel_count =
                    usize::from(uni_drc_config.channel_layout().base_channel_count());
                let dmx_offset = common::get_downmix_offset(
                    base_channel_count as u8,
                    downmix_instr.bs_downmix_offset(),
                    downmix_instr.target_channel_count(),
                );

                let mut max_sum = 0.0_f32;
                let downmix_coeffs = downmix_instr.downmix_coefficient();
                for i in 0..usize::from(downmix_instr.target_channel_count()) {
                    let mut sum = 0.0_f32;
                    for j in 0..base_channel_count {
                        sum += downmix_coeffs[j + i * base_channel_count];
                    }
                    if max_sum < sum {
                        max_sum = sum;
                    }
                }

                max_sum *= dmx_offset;

                downmix_peak_level_db = if max_sum == 1.0_f32 {
                    0.0_f32
                } else {
                    common::linear_to_db(max_sum)
                };
            }

            if !is_match_found {
                is_match_found = loudness_infoset.true_peak_level(
                    drc_set_id,
                    0,
                    is_album_mode,
                    &mut signal_peak_level_tmp,
                );
            }

            if !is_match_found {
                is_match_found = loudness_infoset.sample_peak_level(
                    drc_set_id,
                    0,
                    is_album_mode,
                    &mut signal_peak_level_tmp,
                );
            }

            if !is_match_found {
                is_match_found = loudness_infoset.true_peak_level(
                    0x3F,
                    0,
                    is_album_mode,
                    &mut signal_peak_level_tmp,
                );
            }

            if !is_match_found {
                is_match_found = loudness_infoset.sample_peak_level(
                    0x3F,
                    0,
                    is_album_mode,
                    &mut signal_peak_level_tmp,
                );
            }

            if !is_match_found {
                drc_instruction.get_limiter_peak_target(0, &mut signal_peak_level_tmp);
            }

            signal_peak_level = signal_peak_level_tmp + downmix_peak_level_db;
        } else if !is_match_found {
            // Worst case estimate.
            signal_peak_level = 0.0_f32;
            explicit_peak_information_present = false;
        }

        (explicit_peak_information_present, signal_peak_level)
    }

    /// Initializes the internal parameters based on provided `mode`(codec mode) value.
    ///
    /// # Parameters
    ///
    /// - `mode`: Value of enum type `CodecMode`.
    pub(super) fn init_code_mode_params(&mut self, mode: CodecMode) {
        match mode {
            CodecMode::Undefined => {
                self.loudness_deviation_max = DEFAULT_LOUDNESS_DEVIATION_MAX;
                self.is_peak_limiter_present = false;
            }
            CodecMode::Mpeg4_Aac | CodecMode::MpegD_Usac => {
                self.loudness_deviation_max = DEFAULT_LOUDNESS_DEVIATION_MAX;
                self.is_peak_limiter_present = true;
                // A peak limiter is present at the end of the decoder, therefore we can allow for
                // a maximum output peak level greater than full scale.
                self.output_peak_level_max = 6.0_f32;
            }
        }
    }

    /// Initializes the internal parameters with default values.
    pub(super) fn init_default_params(&mut self) {
        // System parameters.
        self.base_channel_count = 0;
        self.base_layout = -1;
        self.target_config_request_type = TargetConfigRequestType::DownmixId;
        self.num_downmix_id_requests = 0;

        // Loudness normalization parameters.
        self.is_album_mode = false;
        self.is_peak_limiter_present = false;
        self.is_loudness_normalization_on = true;
        self.target_loudness = -24.0_f32;
        self.loudness_deviation_max = DEFAULT_LOUDNESS_DEVIATION_MAX;
        self.loudness_measurement_method = MethodDefinitionRequest::AnchorLoudness;
        self.loudness_measurement_system = MeasurementSystemRequest::ExpertPanel;
        self.loudness_measurement_pre_proc = LoudnessPreprocessingRequest::Default;
        self.device_cut_off_frequency = 500;
        self.loudness_normalization_gain_db_max =
            drc_const::DEFAULT_LOUDNESS_NORMALIZATION_GAIN_MAX;
        self.loudness_normalization_gain_modification_db = 0.0_f32;
        self.output_peak_level_max = 0.0_f32;

        // Dynamic range control parameters.
        self.is_dynamic_range_control_on = true;
        self.num_drc_feature_requests = 0;

        // Other parameters.
        self.boost = 1.0_f32;
        self.compress = 1.0_f32;
        self.drc_characteristic_target = 0;
    }

    /// Maps channel layout to requested downmix IDs. In case of target config type is requested
    /// `target layout` / `target channel count`, downmix ID values are derived from matching
    /// `downmix instruction` from `UniDrcConfig`.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(super) fn channel_layout_to_downmix_id_mapping(
        &mut self,
        uni_drc_config: &UniDrcConfig,
    ) -> Result<(), SelectionProcessError> {
        let mut err = SelectionProcessError::NoError;

        self.num_downmix_id_requests = 0;
        match self.target_config_request_type {
            TargetConfigRequestType::DownmixId => {
                self.downmix_id_requested[0] = 0;
                self.num_downmix_id_requests = 1;
            }
            TargetConfigRequestType::TargetLayout => {
                if i16::from(self.target_layout_requested) == i16::from(self.base_layout) {
                    self.downmix_id_requested[0] = 0;
                    self.num_downmix_id_requests = 1;
                }
                if self.num_downmix_id_requests == 0 {
                    if let (Some(downmix_ids), length) = uni_drc_config
                        .downmix_ids_based_on_target_layout(self.target_layout_requested)
                    {
                        self.downmix_id_requested[..length].copy_from_slice(&downmix_ids[..length]);
                        self.num_downmix_id_requests += length as u8;
                    }
                }

                if self.base_layout == -1 {
                    err = SelectionProcessError::Warning;
                }

                if self.num_downmix_id_requests == 0 {
                    self.downmix_id_requested[0] = 0;
                    self.num_downmix_id_requests = 1;
                    err = SelectionProcessError::Warning;
                }
            }
            TargetConfigRequestType::TargetChannelCount => {
                if self.target_channel_count_requested == self.base_channel_count {
                    self.downmix_id_requested[0] = 0;
                    self.num_downmix_id_requests = 1;
                }

                if self.num_downmix_id_requests == 0 {
                    if let (Some(downmix_ids), length) = uni_drc_config
                        .downmix_ids_based_on_target_channel_count(
                            self.target_channel_count_requested,
                        )
                    {
                        self.downmix_id_requested[..length].copy_from_slice(&downmix_ids[..length]);
                        self.num_downmix_id_requests += length as u8;
                    }
                }

                if self.base_channel_count == 0 {
                    err = SelectionProcessError::Warning;
                }

                if self.num_downmix_id_requests == 0 {
                    self.downmix_id_requested[0] = 0;
                    self.num_downmix_id_requests = 1;
                    err = SelectionProcessError::Warning;
                }
            }
        }
        match err {
            SelectionProcessError::NoError => Ok(()),
            _ => Err(err),
        }
    }

    // Note: Numbering of DRC pre-selection steps according to MPEG-D Part-4 DRC Amd1.

    /// Preselection requirements:
    /// - #1: `downmix_id` of DRC set matches the requested `downmix_id`.
    /// - #2: Output channel layout of `DRC` set matches the requested layout.
    /// - #3: Channel count of DRC set matches the requested channel count.
    ///
    /// # Return
    ///
    /// - Result<bool, SelectionProcessError>
    ///     - 'true` if `downmix_id` is present, otherwise `false`.
    fn pre_selection_requirement123(
        drc_instruction: &DrcInstructionsUniDrc,
        downmix_id_requested: u8,
    ) -> Result<bool, SelectionProcessError> {
        let is_valid_drc_set_id = drc_instruction.drc_set_id() > 0;
        let downmix_ids = drc_instruction.downmix_ids();
        let is_match_found = downmix_ids.iter().any(|&dmx_id| {
            dmx_id == downmix_id_requested
                || dmx_id == drc_const::DOWNMIX_ID_ANY_DOWNMIX
                || (dmx_id == drc_const::DOWNMIX_ID_BASE_LAYOUT && is_valid_drc_set_id)
        });
        Ok(is_match_found)
    }

    /// Preselection requirement:
    /// - #4: The DRC set is not a "Fade-" or "Ducking-" only DRC set.
    ///
    /// # Return
    ///
    /// - Result<bool, SelectionProcessError>
    ///     - 'true` if requirement is fulfilled, otherwise `false`.
    fn pre_selection_requirement4(
        drc_instruction: &DrcInstructionsUniDrc,
        is_dynamic_range_control_on: bool,
    ) -> Result<bool, SelectionProcessError> {
        let is_match_found = if is_dynamic_range_control_on {
            let is_valid_here = drc_instruction.drc_set_id() < 0;
            let drc_set_effect = drc_instruction.drc_set_effect();
            drc_set_effect != u16::from(EffectBit::Fade)
                && drc_set_effect != u16::from(EffectBit::DuckOther)
                && drc_set_effect != u16::from(EffectBit::DuckSelf)
                && (drc_set_effect != 0 || is_valid_here)
        } else {
            true
        };
        Ok(is_match_found)
    }

    /// Preselection requirement:
    /// #5: The number of DRC bands is supported. Moreover, `gain_set_index` and
    /// `gain_sequence_index` are within the allowed range.
    ///
    /// # Return
    ///
    /// - Result<bool, SelectionProcessError>
    ///     - 'true` if requirement is fulfilled, otherwise `false`.
    pub(super) fn pre_selection_requirement5(
        drc_instruction: &DrcInstructionsUniDrc,
        drc_coeffs: Option<&DrcCoefficientsUniDrc>,
    ) -> Result<bool, SelectionProcessError> {
        // Virtual DRC sets are okay.
        if drc_instruction.drc_set_id() < 0 {
            return Ok(true);
        }

        // Check for parametric DRC.
        if let Some(coeff) = drc_coeffs {
            if coeff.drc_location() != drc_instruction.drc_location() {
                // DRC location must be LOCATION_SELECTED.
                return Ok(false);
            }

            let num_ch_grp = usize::from(drc_instruction.num_drc_channel_groups());
            let gain_indices_ch_grp = drc_instruction.gain_set_index_for_channel_group();
            let gain_sequence_count = coeff.gain_sequence_count();

            for gsi in gain_indices_ch_grp.iter().take(num_ch_grp) {
                let index_drc_coeff = *gsi as usize;
                if index_drc_coeff >= drc_const::MAX_SEQUENCES as usize {
                    return Ok(false);
                }

                // Check for parametric DRC.
                if (*gsi + 1) as u8 > coeff.gain_set_count() {
                    continue;
                }

                let gain_set = coeff.gain_set(index_drc_coeff);
                let band_count = usize::from(gain_set.band_count());

                if band_count > drc_const::DRC_MAX_BANDS {
                    return Ok(false);
                }

                let gain_seq_index = gain_set.gain_sequence_index();

                for b in gain_seq_index.iter().take(band_count) {
                    if *b >= drc_const::MAX_SEQUENCES || *b >= gain_sequence_count {
                        return Ok(false);
                    }
                }
            }
        } else {
            // Parametric DRC not supported.
            return Ok(false);
        }
        Ok(true)
    }

    /// Preselection requirement:
    /// - #6: Independent use of DRC set is permitted.
    ///
    /// # Return
    ///
    /// - Result<bool, SelectionProcessError>
    ///     - 'true` if requirement is fulfilled, otherwise `false`.
    fn pre_selection_requirement6(
        drc_instruction: &DrcInstructionsUniDrc,
    ) -> Result<bool, SelectionProcessError> {
        let dep_drc_set = drc_instruction.depends_on_drc_set();

        let is_match_found = (!drc_instruction.depends_on_drc_set_present()
            && !dep_drc_set.is_no_independent_use())
            || drc_instruction.depends_on_drc_set_present();
        Ok(is_match_found)
    }

    /// Preselection requirement:
    /// - #7: DRC sets that require EQ are only permitted if EQ is supported.
    ///
    /// # Return
    ///
    /// - Result<bool, SelectionProcessError>
    ///     - 'true` if requirement is fulfilled, otherwise `false`.
    fn pre_selection_requirement7(
        drc_instruction: &DrcInstructionsUniDrc,
    ) -> Result<bool, SelectionProcessError> {
        // EQ is not supported.
        Ok(!drc_instruction.is_eq_required())
    }

    /// Preselection requirement:
    /// - #8: The range of the target loudness specified for a DRC set has to include the requested
    ///   decoder target loudness.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if requirement is fulfilled.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn pre_selection_requirement8(
        &self,
        uni_drc_config: &UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
        drc_instruction: &DrcInstructionsUniDrc,
        candidates_potential: &mut Selection,
        candidates_selected: &mut Selection,
        downmix_id_index: usize,
    ) -> Result<(), SelectionProcessError> {
        let mut loudness_normalization_gain_db;
        let mut add_to_candidate = false;
        let loudness_deviation_max = f32::from(self.loudness_deviation_max);
        let loudness = loudness_infoset.get_loudness(
            self.is_album_mode,
            self.loudness_measurement_method,
            self.loudness_measurement_system,
            drc_instruction.drc_set_id(),
            self.downmix_id_requested[downmix_id_index],
        );

        let mut loudness_value = 0.0_f32;
        match loudness {
            Ok(content_loudness) => {
                loudness_normalization_gain_db = LoudnessInfoSet::calc_loudness_normalization_gain(
                    self.target_loudness,
                    content_loudness,
                );
                loudness_value = content_loudness;
            }
            Err(err) => {
                if err == LoudnessProcessError::UndefinedValue {
                    loudness_normalization_gain_db =
                        drc_const::UNDEFINED_LOUDNESS_NORMALIZATION_GAIN;
                } else {
                    return Err(SelectionProcessError::NotOk);
                }
            }
        }

        if !self.is_loudness_normalization_on {
            loudness_normalization_gain_db = 0.0_f32;
        }

        let (explicit_peak_information_present, signal_peak_level) = Self::get_signal_peak_level(
            uni_drc_config,
            loudness_infoset,
            drc_instruction,
            self.downmix_id_requested[downmix_id_index],
            self.is_album_mode,
        );

        if self.is_dynamic_range_control_on {
            if !explicit_peak_information_present {
                if (drc_instruction.is_target_loudness_in_range(self.target_loudness)
                    || !self.is_loudness_normalization_on)
                    && drc_instruction.is_drc_set_target_loudness_present()
                {
                    match candidates_selected.add_new() {
                        Ok(selc_data) => {
                            selc_data.set_selection_data_info(
                                loudness_value,
                                loudness_normalization_gain_db,
                                self.loudness_normalization_gain_db_max,
                                loudness_deviation_max,
                                signal_peak_level,
                                self.output_peak_level_max,
                                false,
                            );
                            selc_data.downmix_id_request_index = downmix_id_index as u8;
                            //  Signal pre-selection step dealing with drc_set_target_loudness.
                            selc_data.selection_flag = true;

                            selc_data.output_peak_level = if self.is_loudness_normalization_on {
                                self.target_loudness
                                    - f32::from(
                                        drc_instruction.drc_set_target_loudness_value_upper(),
                                    )
                            } else {
                                0.0_f32
                            };
                            selc_data.fill_drc_instructions(drc_instruction);
                        }
                        Err(e) => {
                            return Err(e);
                        }
                    }
                } else if !self.is_loudness_normalization_on
                    || !drc_instruction.is_drc_set_target_loudness_present()
                    || drc_instruction.is_target_loudness_in_range(self.target_loudness)
                {
                    add_to_candidate = true;
                }
            } else {
                add_to_candidate = true;
            }

            if add_to_candidate {
                match candidates_potential.add_new() {
                    Ok(selc_data) => {
                        selc_data.set_selection_data_info(
                            loudness_value,
                            loudness_normalization_gain_db,
                            self.loudness_normalization_gain_db_max,
                            loudness_deviation_max,
                            signal_peak_level,
                            self.output_peak_level_max,
                            false,
                        );
                        selc_data.downmix_id_request_index = downmix_id_index as u8;
                        selc_data.selection_flag = false;
                        selc_data.fill_drc_instructions(drc_instruction);
                    }
                    Err(e) => {
                        return Err(e);
                    }
                }
            }
        } else if drc_instruction.drc_set_id() < 0 {
            match candidates_potential.add_new() {
                Ok(selc_data) => {
                    selc_data.set_selection_data_info(
                        loudness_value,
                        loudness_normalization_gain_db,
                        self.loudness_normalization_gain_db_max,
                        loudness_deviation_max,
                        signal_peak_level,
                        self.output_peak_level_max,
                        true,
                    );
                    selc_data.downmix_id_request_index = downmix_id_index as u8;
                    selc_data.selection_flag = false;
                    selc_data.fill_drc_instructions(drc_instruction);
                }
                Err(e) => {
                    return Err(e);
                }
            }
        }
        Ok(())
    }

    /// Preselection requirement:
    /// - #9: Clipping is minimized.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if requirement is fulfilled.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn pre_selection_requirement9(
        &self,
        candidates_potential: &Selection,
        candidates_selected: &mut Selection,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from(candidates_potential.num_data());
        for idx in 0..num_selc_data {
            let candidate = candidates_potential.data(idx);
            if let Some(selc_data) = candidate {
                if selc_data.output_peak_level <= self.output_peak_level_max {
                    candidates_selected.add(selc_data)?;
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }

        Ok(())
    }

    /// Checks pre-selection requirements for given `drc_instruction`.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `loudness_infoset`: `LoudnessInfoSet` instance that holds loudness info albums and set
    ///   extension information.
    /// - `drc_instruction`: `DrcInstructionsUniDrc` instance that holds payload / metadata of
    ///   uniDRC instructions.
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    /// - `downmix_id_index`: Index value for downmix ID.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn drc_set_pre_selection_single_instruction(
        &self,
        uni_drc_config: &UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
        drc_instruction: &DrcInstructionsUniDrc,
        candidates_potential: &mut Selection,
        candidates_selected: &mut Selection,
        downmix_id_index: usize,
    ) -> Result<(), SelectionProcessError> {
        let drc_coeffs = uni_drc_config.select_drc_coefficients(drc_const::LOCATION_SELECTED);

        let mut is_match_found = Self::pre_selection_requirement123(
            drc_instruction,
            self.downmix_id_requested[downmix_id_index],
        )?;

        is_match_found = is_match_found
            && Self::pre_selection_requirement4(drc_instruction, self.is_dynamic_range_control_on)?;
        is_match_found =
            is_match_found && Self::pre_selection_requirement5(drc_instruction, drc_coeffs)?;
        is_match_found = is_match_found && Self::pre_selection_requirement6(drc_instruction)?;
        is_match_found = is_match_found && Self::pre_selection_requirement7(drc_instruction)?;

        if is_match_found {
            self.pre_selection_requirement8(
                uni_drc_config,
                loudness_infoset,
                drc_instruction,
                candidates_potential,
                candidates_selected,
                downmix_id_index,
            )
        } else {
            Ok(())
        }
    }

    /// Selects all DRC sets(DRC instructions) that hav unique DRC set ID and satisfy the
    /// pre-selection requirements.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `loudness_infoset`: `LoudnessInfoSet` instance that holds loudness info albums and set
    ///   extension information.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(super) fn drc_set_pre_selection(
        &self,
        candidates_potential: &mut &mut Selection,
        candidates_selected: &mut &mut Selection,
        uni_drc_config: &UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
    ) -> Result<(), SelectionProcessError> {
        let num_downmix_id_requests = usize::from(self.num_downmix_id_requests);
        let instr_count_incl_virtual =
            usize::from(uni_drc_config.drc_instructions_count_incl_virtual());
        let drc_instructions = uni_drc_config.drc_instructions_uni_drc();

        for downmix_id_requested in 0..num_downmix_id_requests {
            for instruction in drc_instructions.iter().take(instr_count_incl_virtual) {
                // Check if ID is unique.
                if uni_drc_config.select_drc_instructions(instruction.drc_set_id())
                    != Some(instruction)
                {
                    continue;
                }

                self.drc_set_pre_selection_single_instruction(
                    uni_drc_config,
                    loudness_infoset,
                    instruction,
                    candidates_potential,
                    candidates_selected,
                    downmix_id_requested,
                )?;
            }
        }

        self.pre_selection_requirement9(candidates_potential, candidates_selected)?;

        if (*candidates_selected).num_data() == 0 {
            candidates_selected.drc_set_selection_add_candidates(
                candidates_potential,
                self.target_loudness,
                self.loudness_deviation_max as f32,
                self.output_peak_level_max,
            )?;
        }

        Ok(())
    }

    /// Selects all DRC sets (DRC instructions) that matches `input requested downmix IDs`.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    ///
    /// # Return
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn drc_set_final_selection_downmix_id<'a>(
        &self,
        candidates_potential: &mut &'a mut Selection,
        candidates_selected: &mut &'a mut Selection,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from(candidates_potential.num_data());
        for idx in 0..num_selc_data {
            let candidate = candidates_potential.data(idx);
            if let Some(selc_data) = candidate {
                if let Some(drc_instruction) = selc_data.drc_instructions_trigger() {
                    let input_dmx_id =
                        self.downmix_id_requested[usize::from(selc_data.downmix_id_request_index)];
                    for dmx_id in drc_instruction.downmix_ids().iter() {
                        if drc_const::DOWNMIX_ID_BASE_LAYOUT != *dmx_id
                            && drc_const::DOWNMIX_ID_ANY_DOWNMIX != *dmx_id
                            && input_dmx_id == *dmx_id
                        {
                            candidates_selected.add(selc_data)?;
                        }
                    }
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }

        if (*candidates_selected).num_data() == 0 {
            selec_data::swap_selection(candidates_potential, candidates_selected);
        }

        Ok(())
    }

    /// Selects single DRC set if multiple DRC sets are applicable. This selection is done step by
    /// step based on different criteria such as lowest output peak value, matching downmix ID,
    /// effect types, target loudness, largest peak value and largest DRC set ID value. A DRC set
    /// is defined as a set of DRC sequences that produce a desired effect if applied to the audio
    /// signal.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    ///
    /// # Return
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(super) fn drc_set_final_selection<'a>(
        &self,
        candidates_potential: &mut &'a mut Selection,
        candidates_selected: &mut &'a mut Selection,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from((*candidates_potential).num_data());

        if num_selc_data == 0 {
            return Err(SelectionProcessError::NotOk);
        } else if num_selc_data == 1 {
            selec_data::swap_selection(candidates_potential, candidates_selected);
        } else {
            candidates_selected.drc_set_final_selection_peak_value0(candidates_potential)?;

            if (*candidates_selected).num_data() > 1 {
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
                self.drc_set_final_selection_downmix_id(candidates_potential, candidates_selected)?;
            }

            if (*candidates_selected).num_data() > 1 {
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
                candidates_selected.drc_set_final_selection_effect_types(candidates_potential)?;
            }

            if (*candidates_selected).num_data() > 1 {
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
                Selection::drc_set_final_selection_target_loudness(
                    candidates_potential,
                    candidates_selected,
                    self.target_loudness,
                )?;
            }

            if (*candidates_selected).num_data() > 1 {
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
                candidates_selected
                    .drc_set_final_selection_peak_value_largest(candidates_potential)?;
            }

            if (*candidates_selected).num_data() > 1 {
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
                candidates_selected.drc_set_final_selection_drc_set_id(candidates_potential)?;
            }
        }

        if (*candidates_selected).num_data() == 0 {
            return Err(SelectionProcessError::NotOk);
        }

        Ok(())
    }

    /// Selects DRC sets depending on number of requested DRC feature effect types. A DRC set is
    /// defined as a set of DRC sequences that produce a desired effect if applied to the audio
    /// signal.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    ///
    /// # Return
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(super) fn drc_set_request_selection<'a>(
        &self,
        uni_drc_config: &UniDrcConfig,
        candidates_potential: &mut &'a mut Selection,
        candidates_selected: &mut &'a mut Selection,
    ) -> Result<(), SelectionProcessError> {
        if (*candidates_potential).num_data() == 0 {
            return Err(SelectionProcessError::NotOk);
        }

        if self.dynamic_range_control_on() {
            if self.num_drc_feature_requests == 0 {
                candidates_selected.select_drc_set_effect_none(candidates_potential)?;

                if (*candidates_selected).num_data() == 0 {
                    let mut fallback_request = DrcFeatureRequest::default();

                    fallback_request.drc_effect_type.num_requests = 5;
                    fallback_request.drc_effect_type.num_requests_desired = 5;
                    fallback_request.drc_effect_type.request[0] =
                        DrcEffectTypeRequest::GeneralCompr;
                    fallback_request.drc_effect_type.request[1] = DrcEffectTypeRequest::Night;
                    fallback_request.drc_effect_type.request[2] = DrcEffectTypeRequest::Noisy;
                    fallback_request.drc_effect_type.request[3] = DrcEffectTypeRequest::Limited;
                    fallback_request.drc_effect_type.request[4] = DrcEffectTypeRequest::LowLevel;

                    Selection::select_effect_type_feature(
                        uni_drc_config,
                        &fallback_request,
                        candidates_potential,
                        candidates_selected,
                    )?;
                }
                selec_data::swap_selection_and_clear(candidates_potential, candidates_selected);
            } else {
                let num_feature_requests = usize::from(self.num_drc_feature_requests);

                for (dfr_type, feature_request) in izip!(
                    self.drc_feature_request_type.iter(),
                    self.drc_feature_request.iter()
                )
                .take(num_feature_requests)
                {
                    if *dfr_type == DrcFeatureRequestType::EffectType {
                        let result = Selection::select_effect_type_feature(
                            uni_drc_config,
                            feature_request,
                            candidates_potential,
                            candidates_selected,
                        );

                        selec_data::swap_selection_and_clear(
                            candidates_potential,
                            candidates_selected,
                        );

                        match result {
                            Ok(_) => (),
                            Err(err) => return Err(err),
                        }
                    }
                }
            }
        }
        Ok(())
    }

    /// Set a specific parameter of `self`, by using `SelProcUserParam`.
    ///
    /// # Parameters
    ///
    /// - `request_type`: See enum `SelProcUserParam`.
    /// - `diff`: Checks wether a parameter value has been changed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok()` - parameter has been set
    ///     - `Err()`- `SelectionProcessError::ParamOutOfRange`, in case of invalid argument.
    pub(super) fn set_param(
        &mut self,
        request_type: SelProcUserParam,
        diff: Option<&mut bool>,
    ) -> Result<(), SelectionProcessError> {
        let mut is_diff = false;
        match request_type {
            SelProcUserParam::LoudnessNormalizationOn(requested_val) => {
                is_diff |=
                    common::comp_assign(&mut self.is_loudness_normalization_on, requested_val);
            }
            SelProcUserParam::TargetLoudness(requested_val) => {
                if !(f32::from(drc_const::DRC_TARGET_LOUDNESS_MIN_VALUE)
                    ..=f32::from(drc_const::DRC_TARGET_LOUDNESS_MAX_RECOMMENDED_VALUE))
                    .contains(&requested_val)
                {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }
                is_diff |= common::comp_assign(&mut self.target_loudness, requested_val);
            }
            SelProcUserParam::EffectType(requested_val) => match requested_val {
                DrcEffectTypeRequest::Off => {
                    // Caution! This overrides all drcFeatureRequests requested so far.
                    is_diff |= common::comp_assign(&mut self.is_dynamic_range_control_on, false);
                }
                DrcEffectTypeRequest::None => {
                    is_diff |= common::comp_assign(&mut self.is_dynamic_range_control_on, true);
                    is_diff |= common::comp_assign(&mut self.num_drc_feature_requests, 0);
                }
                DrcEffectTypeRequest::Night
                | DrcEffectTypeRequest::Noisy
                | DrcEffectTypeRequest::Limited
                | DrcEffectTypeRequest::LowLevel
                | DrcEffectTypeRequest::Dialog
                | DrcEffectTypeRequest::GeneralCompr => {
                    is_diff |= common::comp_assign(&mut self.is_dynamic_range_control_on, true);
                    is_diff |= common::comp_assign(&mut self.num_drc_feature_requests, 1);
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request_type[0],
                        DrcFeatureRequestType::EffectType,
                    );
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0]
                            .drc_effect_type
                            .num_requests_desired,
                        1,
                    );
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0].drc_effect_type.request[0],
                        requested_val,
                    );

                    // Use fallback effect type requests.
                    for (i, req) in self.drc_feature_request[0]
                        .drc_effect_type
                        .request
                        .iter_mut()
                        .enumerate()
                        .take(6)
                        .skip(1)
                    {
                        let fallback_eff_type_req =
                            DrcEffectTypeRequest::fallback_effect_type_request(
                                requested_val,
                                i - 1,
                            )?;
                        is_diff |= common::comp_assign(req, fallback_eff_type_req);
                    }
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0].drc_effect_type.num_requests,
                        6,
                    );
                }
                DrcEffectTypeRequest::Expand | DrcEffectTypeRequest::Artistic => {
                    // Same core code as above, except the fallback effect type requests and number
                    // of requests.
                    is_diff |= common::comp_assign(&mut self.is_dynamic_range_control_on, true);
                    is_diff |= common::comp_assign(&mut self.num_drc_feature_requests, 1);
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request_type[0],
                        DrcFeatureRequestType::EffectType,
                    );
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0]
                            .drc_effect_type
                            .num_requests_desired,
                        1,
                    );
                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0].drc_effect_type.request[0],
                        requested_val,
                    );

                    is_diff |= common::comp_assign(
                        &mut self.drc_feature_request[0].drc_effect_type.num_requests,
                        1,
                    );
                }
            },
            SelProcUserParam::LoudnessMeasurementMethod(requested_val) => {
                is_diff |=
                    common::comp_assign(&mut self.loudness_measurement_method, requested_val);
            }
            SelProcUserParam::AlbumMode(requested_val) => {
                is_diff |= common::comp_assign(&mut self.is_album_mode, requested_val);
            }
            SelProcUserParam::DownmixId(requested_val) => {
                is_diff |= common::comp_assign(
                    &mut self.target_config_request_type,
                    TargetConfigRequestType::DownmixId,
                );
                if requested_val < 0 {
                    // Negative requests signal no DownmixId.
                    is_diff = common::comp_assign(&mut self.num_downmix_id_requests, 0);
                } else {
                    is_diff |= common::comp_assign(&mut self.num_downmix_id_requests, 1);
                    is_diff |=
                        common::comp_assign(&mut self.downmix_id_requested[0], requested_val as u8);
                }
            }
            SelProcUserParam::TargetLayout(requested_val) => {
                // Request target layout according to ChConfiguration in ISO/IEC 23001-8 (CICP).
                if !(1..=63).contains(&requested_val) {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }
                is_diff |= common::comp_assign(
                    &mut self.target_config_request_type,
                    TargetConfigRequestType::TargetLayout,
                );
                is_diff |= common::comp_assign(&mut self.target_layout_requested, requested_val);
            }
            SelProcUserParam::TargetChannelCount(requested_val) => {
                if requested_val < 1 {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }

                is_diff |= common::comp_assign(
                    &mut self.target_config_request_type,
                    TargetConfigRequestType::TargetChannelCount,
                );
                is_diff |=
                    common::comp_assign(&mut self.target_channel_count_requested, requested_val);
            }
            SelProcUserParam::BaseChannelCount(requested_val) => {
                if requested_val < 1 {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }
                is_diff |= common::comp_assign(&mut self.base_channel_count, requested_val);
            }
            SelProcUserParam::SampleRate(requested_val) => {
                if !(drc_const::DRCDEC_MIN_SAMPLERATE..=drc_const::DRCDEC_MAX_SAMPLERATE)
                    .contains(&requested_val)
                {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }
                is_diff |= common::comp_assign(&mut self.audio_sample_rate, requested_val);
            }
            SelProcUserParam::Boost(requested_val) => {
                if !(0.0..=1.0).contains(&requested_val) {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }

                is_diff |= common::comp_assign(&mut self.boost, requested_val);
            }
            SelProcUserParam::Compress(requested_val) => {
                if !(0.0..=1.0).contains(&requested_val) {
                    return Err(SelectionProcessError::ParamOutOfRange);
                }

                is_diff |= common::comp_assign(&mut self.compress, requested_val);
            }
        }

        if let Some(d) = diff {
            *d |= is_diff;
        }
        Ok(())
    }
}
