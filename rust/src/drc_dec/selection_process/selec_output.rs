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
//! DRC selection process output.

use super::super::{
    common::*,
    constants as drc_const,
    reader::{DrcInstructionsUniDrc, LoudnessInfoSet, UniDrcConfig},
};
use super::{
    selec_data::SelectionData, selec_error::SelectionProcessError,
    selec_input::SelectionProcessInput,
};

#[repr(C)]
#[derive(Default, Copy, Clone, Debug)]
/// Selection process output structure to hold information about selected DRC sets.
pub(in super::super) struct SelectionProcessOutput {
    /// Output peak level value in db.
    output_peak_level_db: f32,
    /// Loudness normalization gain in dB.
    loudness_normalization_gain_db: f32,
    /// Output loudness value.
    output_loudness: f32,
    /// Number of selected DRC sets. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    num_selected_drc_sets: u8,
    /// List of selected DRC set IDs. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    selected_drc_set_ids: [i8; drc_const::MAX_ACTIVE_DRCS],
    /// List of selected downmix IDs.
    selected_downmix_ids: [u8; drc_const::MAX_ACTIVE_DRCS],
    /// Active downmix ID.
    active_downmix_id: u8,
    /// Number of base channel count.
    base_channel_count: u8,
    /// Number of target channel count.
    target_channel_count: u8,
    /// Target layout config value.
    target_layout: i16,
    /// Flag to indicate presence of downmix matrix.
    is_downmix_matrix_present: bool,
    /// dowmn matrix values.
    downmix_matrix: [[f32; drc_const::DRCDEC_MAX_CHANNELS]; drc_const::DRCDEC_MAX_CHANNELS],
    /// Output boost level.
    boost: f32,
    /// Output compress level.
    compress: f32,
    /// Output mixing level.
    mixing_level: f32,
}

impl SelectionProcessOutput {
    /// Returns the number of base channel count.
    pub(in super::super) fn base_channel_count(&self) -> u8 {
        self.base_channel_count
    }

    /// Returns the output boost level.
    pub(in super::super) fn boost(&self) -> f32 {
        self.boost
    }

    /// Returns the output compression level.
    pub(in super::super) fn compress(&self) -> f32 {
        self.compress
    }

    pub(in super::super) fn downmix_matrix(
        &self,
    ) -> &[[f32; drc_const::DRCDEC_MAX_CHANNELS]; drc_const::DRCDEC_MAX_CHANNELS] {
        &self.downmix_matrix
    }

    /// Checks whether given `drc_instruction` is usable or not, based on it's DRC set ID value.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `drc_instruction`: `DrcInstructionsUniDrc` instance that holds payload / metadata of
    ///   uniDRC instructions.
    ///
    /// # Return
    ///
    ///  - Result<bool, SelectionProcessError>
    ///      - Ok(is_usable): `true` if `drc_instruction` is usable. Otherwise `false`.
    ///      - Err(SelectionProcessError)
    fn is_drc_set_usable(
        uni_drc_config: &UniDrcConfig,
        drc_instruction: &DrcInstructionsUniDrc,
    ) -> Result<bool, SelectionProcessError> {
        let coeff = uni_drc_config.select_drc_coefficients(drc_const::LOCATION_SELECTED);
        // Check if ID is unique.
        if uni_drc_config.select_drc_instructions(drc_instruction.drc_set_id())
            != Some(drc_instruction)
        {
            return Ok(false);
        }
        // Sanity check on DRC instructions.
        SelectionProcessInput::pre_selection_requirement5(drc_instruction, coeff)
    }

    /// Generates and updates parameters of selection process output.
    ///
    /// # Parameters
    ///
    /// - `selc_proc_input`: `SelectionProcessInput` instance with valid data.
    /// - `selc_data`: `SelectionData` instance with valid data.
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `loudness_infoset`: `LoudnessInfoSet` instance that holds loudness info albums and set
    ///   extension information.
    ///
    /// # Return
    ///
    ///  - Result<(), SelectionProcessError>
    ///      - `Ok(())` if the function is successful.
    ///      - `Err(SelectionProcessError)` if there is an error.
    pub(super) fn generate_output_info(
        &mut self,
        selc_proc_input: &SelectionProcessInput,
        selc_data: &SelectionData,
        uni_drc_config: &UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
    ) -> Result<(), SelectionProcessError> {
        self.num_selected_drc_sets = 1;

        if let Some(drc_inst) = selc_data.drc_instructions_trigger() {
            self.selected_drc_set_ids[0] = drc_inst.drc_set_id();
            self.selected_downmix_ids[0] = if drc_inst.is_drc_apply_to_downmix() {
                drc_inst.downmix_id_at(0)
            } else {
                0
            };
        }

        self.loudness_normalization_gain_db = selc_data.loudness_normalization_gain_db_adjusted
            + selc_proc_input.loudness_normalization_gain_modification_db;
        self.output_peak_level_db = selc_data.output_peak_level;
        self.output_loudness = selc_data.output_loudness;
        self.boost = selc_proc_input.boost;
        self.compress = selc_proc_input.compress;
        self.base_channel_count = uni_drc_config.channel_layout().base_channel_count();
        self.target_channel_count = self.base_channel_count;
        self.active_downmix_id =
            selc_proc_input.downmix_id_requested[usize::from(selc_data.downmix_id_request_index)];

        let downmix_id_requested = selc_proc_input.downmix_id_requested[0];
        let is_album_mode = selc_proc_input.is_album_mode;
        let drc_set_id = self.selected_drc_set_ids[0];

        let mixing_level_res =
            loudness_infoset.get_mixing_level(downmix_id_requested, drc_set_id, is_album_mode);
        match mixing_level_res {
            Ok(mix_level) => {
                self.mixing_level = mix_level;
            }
            Err(_err) => {
                return Err(SelectionProcessError::NotOk);
            }
        }

        // Dependent.
        let mut has_dependent = 0;
        if let Some(drc_inst) = selc_data.drc_instructions_trigger() {
            if drc_inst.depends_on_drc_set_present() {
                let mut selc_drc_set_ids_iterator =
                    self.selected_drc_set_ids[usize::from(self.num_selected_drc_sets)..].iter_mut();
                let mut selc_dmx_ids_iterator =
                    self.selected_downmix_ids[usize::from(self.num_selected_drc_sets)..].iter_mut();

                let depends_on_drc_set_id = drc_inst.depends_on_drc_set_id();
                let drc_instructions_uni_drc = uni_drc_config.drc_instructions_uni_drc();
                let num_instructions =
                    usize::from(uni_drc_config.drc_instructions_count_incl_virtual());

                'dependent_iterator: for instruction in
                    drc_instructions_uni_drc.iter().take(num_instructions)
                {
                    if !(Self::is_drc_set_usable(uni_drc_config, instruction)?) {
                        continue;
                    }

                    let drc_set_id = instruction.drc_set_id();

                    if drc_set_id == depends_on_drc_set_id {
                        let downmix_id = instruction.downmix_id_at(0);
                        if let (Some(dsi), Some(dmxi)) = (
                            selc_drc_set_ids_iterator.next(),
                            selc_dmx_ids_iterator.next(),
                        ) {
                            *dsi = drc_set_id;
                            *dmxi = if instruction.is_drc_apply_to_downmix() {
                                downmix_id
                            } else {
                                0
                            };

                            self.num_selected_drc_sets += 1;
                            has_dependent = 1;
                            break 'dependent_iterator;
                        }
                    }
                }
            }
        }

        let drc_instructions_uni_drc = uni_drc_config.drc_instructions_uni_drc();
        let num_instructions = usize::from(uni_drc_config.drc_instructions_uni_drc_count());

        // Fading.
        if !is_album_mode {
            for instruction in drc_instructions_uni_drc.iter().take(num_instructions) {
                if !(Self::is_drc_set_usable(uni_drc_config, instruction)?) {
                    continue;
                }

                if instruction.drc_set_effect() & u16::from(EffectBit::Fade) != 0 {
                    let downmix_id = instruction.downmix_id_at(0);
                    if downmix_id == drc_const::DOWNMIX_ID_ANY_DOWNMIX {
                        self.num_selected_drc_sets = has_dependent + 1;
                        self.selected_drc_set_ids[usize::from(self.num_selected_drc_sets)] =
                            instruction.drc_set_id();
                        self.selected_downmix_ids[usize::from(self.num_selected_drc_sets)] =
                            if instruction.is_drc_apply_to_downmix() {
                                downmix_id
                            } else {
                                0
                            };
                        self.num_selected_drc_sets += 1;
                    } else {
                        return Err(SelectionProcessError::NotOk);
                    }
                }
            }
        }

        // Ducking.
        // Check for ducking, then repeat process for DOWNMIX_ID_BASE_LAYOUT if no ducking found.
        let mut has_ducking = false;
        let mut use_this_dmx_id = self.active_downmix_id;
        // CASE 0: Ducking. CASE 1: DOWNMIX_ID_BASE_LAYOUT if no ducking.
        for _case in 0..2 {
            if !has_ducking {
                for instruction in drc_instructions_uni_drc.iter().take(num_instructions) {
                    if !(Self::is_drc_set_usable(uni_drc_config, instruction)?) {
                        continue;
                    }

                    if instruction.drc_set_effect() & (EffectBit::DuckOther | EffectBit::DuckSelf)
                        != 0
                    {
                        let downmix_ids_list = instruction.downmix_ids();
                        for downmix_id in downmix_ids_list.iter() {
                            if *downmix_id == use_this_dmx_id {
                                // Ducking overrides fading.
                                self.num_selected_drc_sets = has_dependent + 1;
                                self.selected_drc_set_ids
                                    [usize::from(self.num_selected_drc_sets)] =
                                    instruction.drc_set_id();
                                // Force ducking DRC set to be processed on base layout.
                                self.selected_downmix_ids
                                    [usize::from(self.num_selected_drc_sets)] = 0;
                                self.num_selected_drc_sets += 1;
                                has_ducking = true;
                            }
                        }
                    }
                }
            }
            // Change downmix_id for CASE 1.
            use_this_dmx_id = drc_const::DOWNMIX_ID_BASE_LAYOUT;
        }

        if self.num_selected_drc_sets > 3 {
            // Maximum permitted number of applied DRC sets is 3,
            // see section 6.3.5 of ISO/IEC 23003-4.
            self.num_selected_drc_sets = 0;
            return Err(SelectionProcessError::NotOk);
        }

        // Sorting: Ducking/Fading -> Dependent -> Selected.
        if self.num_selected_drc_sets == 3 {
            let selected_drc_set_id = self.selected_drc_set_ids[0];
            let selected_downmix_id = self.selected_downmix_ids[0];
            self.selected_drc_set_ids[0] = self.selected_drc_set_ids[2];
            self.selected_downmix_ids[0] = self.selected_downmix_ids[2];
            self.selected_drc_set_ids[2] = selected_drc_set_id;
            self.selected_downmix_ids[2] = selected_downmix_id;
        } else if self.num_selected_drc_sets == 2 {
            let selected_drc_set_id = self.selected_drc_set_ids[0];
            let selected_downmix_id = self.selected_downmix_ids[0];
            self.selected_drc_set_ids[0] = self.selected_drc_set_ids[1];
            self.selected_downmix_ids[0] = self.selected_downmix_ids[1];
            self.selected_drc_set_ids[1] = selected_drc_set_id;
            self.selected_downmix_ids[1] = selected_downmix_id;
        }

        Ok(())
    }

    /// Returns the loudness normalization gain in dB.
    pub(in super::super) fn loudness_normalization_gain_db(&self) -> f32 {
        self.loudness_normalization_gain_db
    }

    /// Returns a flag to indicate presence of downmix matrix.
    pub(in super::super) fn is_downmix_matrix_present(&self) -> bool {
        self.is_downmix_matrix_present
    }

    /// Returns the number of selected DRC sets.
    pub(in super::super) fn num_selected_drc_sets(&self) -> u8 {
        self.num_selected_drc_sets
    }

    /// Returns the output loudness value.
    pub(in super::super) fn output_loudness(&self) -> f32 {
        self.output_loudness
    }

    /// Returns the output peak level value in db.
    pub(in super::super) fn output_peak_level_db(&self) -> f32 {
        self.output_peak_level_db
    }

    /// Selects matching `downmix IDs` and calculates `downmix matrix` values, based on information
    /// from `UniDrcConfig` such as `channel layout` and `target layout`. The value of `target
    /// layout` is derived from matching `downmix instructions`.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    pub(super) fn select_downmix_matrix(&mut self, uni_drc_config: &UniDrcConfig) {
        self.base_channel_count = uni_drc_config.channel_layout().base_channel_count();
        self.target_channel_count = self.base_channel_count;
        self.target_layout = -1;
        self.is_downmix_matrix_present = false;

        if self.active_downmix_id != 0 {
            let downmix_instructions = uni_drc_config.downmix_instructions();
            let num_dmx_instructions = uni_drc_config.downmix_instructions_count() as usize;

            'dmx_loop: for downmix in downmix_instructions.iter().take(num_dmx_instructions) {
                let downmix_tcc = downmix.target_channel_count();
                if downmix_tcc > drc_const::DRCDEC_MAX_CHANNELS as u8 {
                    continue;
                }

                if self.active_downmix_id == downmix.downmix_id() {
                    self.target_channel_count = downmix_tcc;
                    self.target_layout = i16::from(downmix.target_layout());

                    if downmix.is_downmix_coefficients_present() {
                        let downmix_offset = get_downmix_offset(
                            self.base_channel_count,
                            downmix.bs_downmix_offset(),
                            downmix_tcc,
                        );

                        let downmix_coeffs = downmix.downmix_coefficient();
                        for (j, matrix_col) in self
                            .downmix_matrix
                            .iter_mut()
                            .enumerate()
                            .take(usize::from(self.base_channel_count))
                        {
                            for (k, data) in matrix_col
                                .iter_mut()
                                .enumerate()
                                .take(usize::from(self.target_channel_count))
                            {
                                *data = downmix_offset
                                    * downmix_coeffs[j + k * usize::from(self.base_channel_count)];
                            }
                        }

                        self.is_downmix_matrix_present = true;
                    }
                    break 'dmx_loop;
                }
            }
        }
    }

    /// Returns the list of selected downmix IDs.
    pub(in super::super) fn selected_downmix_ids(&self) -> &[u8; drc_const::MAX_ACTIVE_DRCS] {
        &self.selected_downmix_ids
    }

    /// Returns the list of selected DRC set IDs.
    pub(in super::super) fn selected_drc_set_ids(&self) -> &[i8; drc_const::MAX_ACTIVE_DRCS] {
        &self.selected_drc_set_ids
    }

    pub(in super::super) fn set_num_selected_drc_sets(&mut self, drc_sets: u8) {
        self.num_selected_drc_sets = drc_sets;
    }

    /// Sets the output loudness value.
    pub(in super::super) fn set_output_loudness(&mut self, loudness_val: f32) {
        self.output_loudness = loudness_val;
    }

    /// Returns the number of target channel count.
    pub(in super::super) fn target_channel_count(&self) -> u8 {
        self.target_channel_count
    }
}
