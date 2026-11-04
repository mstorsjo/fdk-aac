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
//! MPEG-D DRC's selection processing component (selection data).

use itertools::izip;

use super::super::{
    common::{DrcBitstreamLocation, EffectBit},
    constants as drc_const,
    reader::{DependsOnDrcSet, DrcInstructionsUniDrc, LoudnessInfoSet, UniDrcConfig},
};
use super::{
    effect_type::DrcEffectTypeRequest, feature_request::DrcFeatureRequest,
    selec_error::SelectionProcessError,
};

/// Maximum number of selection data.
const MAX_SELECTION_DATA: u8 =
    drc_const::MAX_DRC_INSTRUCTIONS + 1 + drc_const::MAX_DOWNMIX_INSTRUCTIONS;

// ---- DrcInstructionsTrigger ---- //
/// Trigger information from DrcInstructionsUniDrc. This contains the properties of the DRC set
/// that define the conditions when the DRC set should be selected. The Target information, i.e.
/// information about application to the signal, is removed here, because it needs the most memory.
#[repr(C)]
#[derive(Default, Copy, Clone, Debug)]
pub(in super::super) struct DrcInstructionsTrigger {
    drc_instructions_type: u8,
    drc_set_id: i8,
    drc_location: DrcBitstreamLocation,
    is_drc_apply_to_downmix: bool,
    downmix_id_count: u8,
    downmix_id: [u8; drc_const::MAX_DOWNMIX_IDS as usize],
    drc_set_effect: u16,
    is_drc_set_target_loudness_present: bool,
    drc_set_target_loudness_value_upper: i8,
    drc_set_target_loudness_value_lower: i8,
    depends_on_drc_set_present: bool,
    depends_on_drc_set: DependsOnDrcSet,
    num_drc_channel_groups: u8,
    gain_set_index_for_channel_group: [i8; drc_const::DRCDEC_MAX_CHANNELS],
}

impl DrcInstructionsTrigger {
    /// Creates a new default instance of `Self`.
    pub(in super::super) fn _new() -> Self {
        Default::default()
    }

    /// Clears internal parameters.
    pub(in super::super) fn clear(&mut self) {
        self.drc_instructions_type = 0;
        self.drc_set_id = 0;
        self.drc_location = DrcBitstreamLocation::default();
        self.is_drc_apply_to_downmix = false;
        self.downmix_id_count = 0;
        self.downmix_id.fill(0);
        self.drc_set_effect = 0;
        self.is_drc_set_target_loudness_present = false;
        self.drc_set_target_loudness_value_upper = 0;
        self.drc_set_target_loudness_value_lower = 0;
        self.depends_on_drc_set_present = false;
        self.depends_on_drc_set = DependsOnDrcSet::default();
        self.num_drc_channel_groups = 0;
        self.gain_set_index_for_channel_group.fill(0);
    }

    /// Returns `DRC set identifier` of the DRC set this DRC depends on.
    pub(in super::super) fn depends_on_drc_set_id(&self) -> i8 {
        self.depends_on_drc_set.depends_on_drc_set()
    }

    /// Returns status of whether this DRC set depends on another DRC set.
    pub(in super::super) fn depends_on_drc_set_present(&self) -> bool {
        self.depends_on_drc_set_present
    }

    /// Returns the Downmix ID at the `index` position.
    pub(in super::super) fn downmix_id_at(&self, index: usize) -> u8 {
        self.downmix_id[index]
    }

    /// Returns `self.downmix_id[]`. The length of slice is limited to `self.downmix_id_count`.
    pub(in super::super) fn downmix_ids(&self) -> &[u8] {
        &self.downmix_id[..self.downmix_id_count as usize]
    }

    /// Returns `DRC set identifier`. A DRC set is defined as a set of DRC sequences that produce a
    /// desired effect if applied to the audio signal.
    pub(in super::super) fn drc_set_id(&self) -> i8 {
        self.drc_set_id
    }

    /// Gets effect type of particular DRC set.
    pub(in super::super) fn drc_set_effect(&self) -> u16 {
        self.drc_set_effect
    }

    /// Returns the target loudness upper value for a DRC set.
    pub(in super::super) fn drc_set_target_loudness_value_upper(&self) -> i8 {
        self.drc_set_target_loudness_value_upper
    }

    /// Fills data from `drc_instr` to `self`.
    pub(in super::super) fn fill(
        &mut self,
        drc_instr: &DrcInstructionsUniDrc,
    ) -> Result<(), SelectionProcessError> {
        self.drc_set_id = drc_instr.drc_set_id();
        self.drc_location = drc_instr.drc_location();
        self.is_drc_apply_to_downmix = drc_instr.is_drc_apply_to_downmix();
        self.downmix_id_count = drc_instr.downmix_id_count();
        for (dest, src) in izip!(self.downmix_id.iter_mut(), drc_instr.downmix_ids().iter())
            .take(usize::from(self.downmix_id_count))
        {
            *dest = *src;
        }
        self.drc_set_effect = drc_instr.drc_set_effect();
        self.is_drc_set_target_loudness_present = drc_instr.is_drc_set_target_loudness_present();

        if self.is_drc_set_target_loudness_present {
            self.drc_set_target_loudness_value_upper =
                drc_instr.drc_set_target_loudness_value_upper();
            self.drc_set_target_loudness_value_lower =
                drc_instr.drc_set_target_loudness_value_lower();
        }
        self.depends_on_drc_set_present = drc_instr.depends_on_drc_set_present();
        self.depends_on_drc_set = drc_instr.depends_on_drc_set();

        self.num_drc_channel_groups = drc_instr.num_drc_channel_groups();
        for (dest, src) in izip!(
            self.gain_set_index_for_channel_group.iter_mut(),
            drc_instr.gain_set_index_for_channel_group().iter()
        )
        .take(usize::from(self.num_drc_channel_groups))
        {
            *dest = *src;
        }

        Ok(())
    }

    /// Returns `true` if `target_loudness` in range. Otherwise, `false`.
    ///
    /// # Parameters
    ///
    /// - `target_loudness`: Target loudness value.
    pub(in super::super) fn is_target_loudness_in_range(&self, target_loudness: f32) -> bool {
        let upper = f32::from(self.drc_set_target_loudness_value_upper);
        let lower = f32::from(self.drc_set_target_loudness_value_lower);

        self.is_drc_set_target_loudness_present
            && upper >= target_loudness
            && lower < target_loudness
    }

    /// Returns status of flag `self.is_drc_apply_to_downmix`.
    pub(in super::super) fn is_drc_apply_to_downmix(&self) -> bool {
        self.is_drc_apply_to_downmix
    }
}

// ---- SelectionData ---- //
#[repr(C)]
#[derive(Default, Copy, Clone, Debug)]
/// `SelectionData` structure that holds data related to single DRC set.
pub(in super::super) struct SelectionData {
    pub(in super::super) selection_flag: bool,
    pub(in super::super) downmix_id_request_index: u8,
    pub(in super::super) output_peak_level: f32,
    pub(in super::super) loudness_normalization_gain_db_adjusted: f32,
    pub(in super::super) output_loudness: f32,
    drc_instr_trigger: DrcInstructionsTrigger,
}

impl SelectionData {
    /// Clears internal parameters.
    fn clear(&mut self) {
        self.selection_flag = false;
        self.downmix_id_request_index = 0;
        self.output_peak_level = 0.0;
        self.loudness_normalization_gain_db_adjusted = 0.0;
        self.output_loudness = 0.0;
        self.drc_instr_trigger.clear();
    }

    #[cfg(test)]
    /// Creates new instance of `SelectionData`.
    pub(in super::super) fn new() -> Self {
        Default::default()
    }

    /// Sets output loudness and peak level parameters of `SelectionData`.
    ///
    /// # Parameters
    ///
    /// - `loudness`: Loudness value.
    /// - `loudness_normalization_gain_db`: Gain value expressed in dB.
    /// - `loudness_normalization_gain_db_max`: Maximum gain value expressed in dB.
    /// - `loudness_deviation_max`: Maximum loudness deviation value.
    /// - `signal_peak_level`: Signal peak level value.
    /// - `output_peak_level_max`: Output peak level value.
    /// - `is_apply_adjustment`: Flag to adjust loudness normalization gain.
    #[expect(clippy::too_many_arguments)]
    pub(in super::super) fn set_selection_data_info(
        &mut self,
        loudness: f32,
        loudness_normalization_gain_db: f32,
        loudness_normalization_gain_db_max: f32,
        loudness_deviation_max: f32,
        signal_peak_level: f32,
        output_peak_level_max: f32,
        is_apply_adjustment: bool,
    ) {
        let adjustment = if is_apply_adjustment {
            let adjust = (signal_peak_level + loudness_normalization_gain_db
                - output_peak_level_max)
                .max(0.0_f32);
            adjust.min(loudness_deviation_max.max(0.0_f32))
        } else {
            0.0_f32
        };

        self.loudness_normalization_gain_db_adjusted =
            loudness_normalization_gain_db_max.min(loudness_normalization_gain_db - adjustment);
        self.output_loudness = loudness + self.loudness_normalization_gain_db_adjusted;
        self.output_peak_level = signal_peak_level + self.loudness_normalization_gain_db_adjusted;
    }

    /// Copies the fields from 'drc_instr'.
    pub(in super::super) fn fill_drc_instructions(&mut self, drc_instr: &DrcInstructionsUniDrc) {
        let _ = self.drc_instr_trigger.fill(drc_instr);
    }

    /// Returns an optional immutable reference to an `DrcInstructionsUniDrc` instance.
    pub(in super::super) fn drc_instructions_trigger(&self) -> Option<&DrcInstructionsTrigger> {
        Some(&self.drc_instr_trigger)
    }
}

// ---- Selection ----//
#[repr(C)]
#[derive(Default, Debug)]
/// `Selection` structure that holds list of `SelectionData`. Each `SelectionData` has
/// information related to single DRC set.
pub(in super::super) struct Selection {
    num_data: u8,
    data: [SelectionData; MAX_SELECTION_DATA as usize],
}

impl Selection {
    #[cfg(test)]
    pub(in super::super) fn new() -> Self {
        Selection {
            num_data: 0,
            data: [SelectionData::new(); MAX_SELECTION_DATA as usize],
        }
    }

    /// Adds (copies) a new `SelectionData` to `self.data[]`
    ///
    /// # Parameters
    ///
    /// - `data_in`: `SelectionData` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn add(
        &mut self,
        data_in: &SelectionData,
    ) -> Result<(), SelectionProcessError> {
        if self.num_data < MAX_SELECTION_DATA {
            self.data[usize::from(self.num_data)] = *data_in;
            self.num_data += 1;
            Ok(())
        } else {
            Err(SelectionProcessError::NotOk)
        }
    }

    /// Returns mutable reference to new `SelectionData` in buffer.
    ///
    /// # Return
    ///
    /// - Result<&mut SelectionData, SelectionProcessError>
    ///     - `Ok(&mut SelectionData)` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn add_new(
        &mut self,
    ) -> Result<&mut SelectionData, SelectionProcessError> {
        if self.num_data < MAX_SELECTION_DATA {
            self.data[usize::from(self.num_data)].clear();
            self.num_data += 1;
            Ok(&mut self.data[usize::from(self.num_data - 1)])
        } else {
            Err(SelectionProcessError::NotOk)
        }
    }

    /// Resets number of selection data to zero.
    pub(in super::super) fn clear_num_data(&mut self) {
        self.num_data = 0;
    }

    /// Returns number of selection data.
    pub(in super::super) fn num_data(&self) -> u8 {
        self.num_data
    }

    /// Returns `Option<&SelectionData>` if given `idx` is valid, Otherwise `None`.
    pub(in super::super) fn data(&self, idx: usize) -> Option<&SelectionData> {
        if idx < usize::from(MAX_SELECTION_DATA) {
            Some(&self.data[idx])
        } else {
            None
        }
    }

    /// Selects all DRC sets if input target loudness is in the target loudness range of DRC set
    /// (drc instructions). Otherwise selects all DRC sets who has lowest output peak level.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `target_loudness`: Target loudness value of selection process input.
    /// - `loudness_deviation_max`: Maximum loudness deviation of selection process input.
    /// - `output_peak_level_max`: Maximum output peak level of selection process input.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_selection_add_candidates(
        &mut self,
        candidates_potential: &mut Selection,
        target_loudness: f32,
        loudness_deviation_max: f32,
        output_peak_level_max: f32,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();
        let mut was_hit = false;
        'tlri_loop: for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_instruction) = candidate.drc_instructions_trigger() {
                if drc_instruction.is_target_loudness_in_range(target_loudness) {
                    was_hit = true;
                    break 'tlri_loop;
                }
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        if was_hit {
            for candidate in candidates_potential
                .data
                .iter()
                .take(num_selc_data.min(max_data_items))
            {
                if let Some(drc_instruction) = candidate.drc_instructions_trigger() {
                    if drc_instruction.is_target_loudness_in_range(target_loudness) {
                        self.add(candidate)?;
                    }
                }
            }
        } else {
            let mut lowest_peak_level = 1000.0f32;
            let mut peak_level;
            for candidate in candidates_potential
                .data
                .iter()
                .take(num_selc_data.min(max_data_items))
            {
                peak_level = candidate.output_peak_level;

                if peak_level < lowest_peak_level {
                    lowest_peak_level = peak_level;
                }
            }

            // Add all with lowest peak level or max 1dB above.
            for candidate in candidates_potential
                .data
                .iter_mut()
                .take(num_selc_data.min(max_data_items))
            {
                peak_level = candidate.output_peak_level;

                if peak_level == lowest_peak_level || peak_level <= lowest_peak_level + 1.0f32 {
                    let mut adjustment = (peak_level - output_peak_level_max).max(0.0_f32);
                    adjustment = adjustment.min(loudness_deviation_max.max(0.0_f32));
                    candidate.loudness_normalization_gain_db_adjusted -= adjustment;
                    candidate.output_peak_level -= adjustment;
                    candidate.output_loudness -= adjustment;
                    self.add(candidate)?;
                }
            }
        }
        Ok(())
    }

    /// Select loudness for album mode from the candidates list.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `loudness_info_set`: `LoudnessInfoSet` instance with valid internal data.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn select_album_loudness(
        &mut self,
        candidates_potential: &Selection,
        loudness_info_set: &LoudnessInfoSet,
    ) -> Result<(), SelectionProcessError> {
        let num_data = usize::from(candidates_potential.num_data());
        let mut v = 0;

        for idx in 0..num_data {
            let candidate_opt = candidates_potential.data(idx);
            match candidate_opt {
                Some(candidate) => {
                    let drc_set_id_candidate = match candidate.drc_instructions_trigger() {
                        Some(drc_instr) => drc_instr.drc_set_id(),
                        None => {
                            return Err(SelectionProcessError::NotOk);
                        }
                    };

                    if drc_set_id_candidate < 0 {
                        // Keep virtual DRC sets.
                        v += 1;
                        self.add(candidate)?;
                    }

                    let li_album_cnt = usize::from(loudness_info_set.li_album_count());
                    for li_album in loudness_info_set.li_album().iter().take(li_album_cnt) {
                        let drc_set_id_li_album = li_album.drc_set_id();

                        if drc_set_id_candidate == drc_set_id_li_album {
                            self.add(candidate)?;
                        }
                    }
                }
                None => {
                    return Err(SelectionProcessError::NotOk);
                }
            }
        }

        if v == self.num_data() {
            // We have only virtual DRC sets. Dismiss the selection.
            self.clear_num_data();
        }

        Ok(())
    }

    /// Selects DRC sets whose DRC set effects are none. Ideally these DRC sets don't have any DRC
    /// set effects that shall be used for applying.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn select_drc_set_effect_none(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from(candidates_potential.num_data());

        for idx in 0..num_selc_data {
            let candidate = candidates_potential.data(idx);

            if let Some(selc_data) = candidate {
                if let Some(drc_instruction) = selc_data.drc_instructions_trigger() {
                    if drc_instruction.drc_set_effect() & 0xFF == 0 {
                        self.add(selc_data)?;
                    }
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }
        Ok(())
    }

    /// Selects all DRC sets (including dependent DRC set) that matches requested single DRC effect
    /// type.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `effect_type`:  Requested DRC effect type (`DrcEffectTypeRequest`).
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn select_single_effect_type(
        &mut self,
        candidates_potential: &Selection,
        uni_drc_config: &UniDrcConfig,
        effect_type: DrcEffectTypeRequest,
    ) -> Result<(), SelectionProcessError> {
        if effect_type == DrcEffectTypeRequest::None {
            self.select_drc_set_effect_none(candidates_potential)?;
        } else {
            let effect_bit_position = 1_u16 << (i8::from(effect_type) as u16 - 1);
            let num_selc_data = usize::from(candidates_potential.num_data());
            let max_data_items = candidates_potential.data.len();

            for candidate in candidates_potential
                .data
                .iter()
                .take(num_selc_data.min(max_data_items))
            {
                if let Some(drc_instruction) = candidate.drc_instructions_trigger() {
                    if !drc_instruction.depends_on_drc_set_present() {
                        if drc_instruction.drc_set_effect() & effect_bit_position != 0 {
                            self.add(candidate)?;
                        }
                    } else if let Some(drc_instr_dependent) = uni_drc_config
                        .get_dependent_drc_instruction(drc_instruction.depends_on_drc_set_id())
                    {
                        if drc_instruction.drc_set_effect() & effect_bit_position != 0
                            || drc_instr_dependent.drc_set_effect() & effect_bit_position != 0
                        {
                            self.add(candidate)?;
                        }
                    } else {
                        return Err(SelectionProcessError::NotOk);
                    }
                } else {
                    return Err(SelectionProcessError::NotOk);
                }
            }

            if num_selc_data > max_data_items {
                return Err(SelectionProcessError::NotOk);
            }
        }
        Ok(())
    }

    /// Selects all DRC sets that matches requested DRC feature effect types.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `drc_feature_request`: `DrcFeatureRequest` instance with valid internal data.
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn select_effect_type_feature<'a>(
        uni_drc_config: &UniDrcConfig,
        drc_feature_request: &DrcFeatureRequest,
        candidates_potential: &mut &'a mut Selection,
        candidates_selected: &mut &'a mut Selection,
    ) -> Result<(), SelectionProcessError> {
        let mut is_desired_effect_type_found = false;
        let num_requests_desired =
            usize::from(drc_feature_request.drc_effect_type.num_requests_desired);
        for request in drc_feature_request
            .drc_effect_type
            .request
            .iter()
            .take(num_requests_desired)
        {
            {
                candidates_selected.select_single_effect_type(
                    candidates_potential,
                    uni_drc_config,
                    *request,
                )?;
            }

            if (*candidates_selected).num_data() != 0 {
                is_desired_effect_type_found = true;
                swap_selection_and_clear(candidates_potential, candidates_selected);
            }
        }

        if !is_desired_effect_type_found {
            let num_requests = usize::from(drc_feature_request.drc_effect_type.num_requests);
            if num_requests_desired < num_requests {
                for request in drc_feature_request.drc_effect_type.request
                    [num_requests_desired..num_requests]
                    .iter()
                    .take(num_requests - num_requests_desired)
                {
                    candidates_selected.select_single_effect_type(
                        candidates_potential,
                        uni_drc_config,
                        *request,
                    )?;

                    if (*candidates_selected).num_data() != 0 {
                        swap_selection_and_clear(candidates_potential, candidates_selected);
                        break;
                    }
                }
            }
        }

        swap_selection(candidates_potential, candidates_selected);
        Ok(())
    }

    /// Selects all DRC sets whose output peak level is zero or less than zero. In case there is no
    /// DRC set with output peak level value is zero or less than zero, then selects DRC set with
    /// lowest output peak level.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_final_selection_peak_value0(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let mut lowest_peak_level = 10000.0f32;
        let mut index_lowest_peak_level = 0;
        let num_selc_data: usize = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();

        for (index, candidate) in candidates_potential
            .data
            .iter()
            .enumerate()
            .take(num_selc_data.min(max_data_items))
        {
            if candidate.output_peak_level <= 0.0_f32 {
                self.add(candidate)?;
            }

            if candidate.output_peak_level < lowest_peak_level {
                index_lowest_peak_level = index;
                lowest_peak_level = candidate.output_peak_level;
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        if self.num_data() == 0 {
            if let Some(candidate) = candidates_potential.data(index_lowest_peak_level) {
                self.add(candidate)?;
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }
        Ok(())
    }

    /// Selects all DRC sets who has minimum number of DRC effect types, ignoring `General` effect
    /// type (`EffectBit::GeneralCompr`).
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_final_selection_effect_types(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let mut min_num_effects = 1000;
        let mut num_effects;
        let mut effects;
        let num_selc_data: usize = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();

        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_inst) = candidate.drc_instructions_trigger() {
                effects = drc_inst.drc_set_effect();
                effects &= 0xffff ^ u16::from(EffectBit::GeneralCompr);
                num_effects = effects.count_ones();
                if num_effects < min_num_effects {
                    min_num_effects = num_effects;
                }
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        // Add all with minimum number of effects.
        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_inst) = candidate.drc_instructions_trigger() {
                effects = drc_inst.drc_set_effect();
                effects &= 0xffff ^ u16::from(EffectBit::GeneralCompr);
                num_effects = effects.count_ones();
                if num_effects == min_num_effects {
                    self.add(candidate)?;
                }
            }
        }
        Ok(())
    }

    /// Selects all DRC sets whose target loudness upper value is same as smallest target loudness
    /// upper value in the available DRC sets.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    fn select_smallest_target_loudness_value_upper(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let mut min_val = 0x7F;
        let num_selc_data: usize = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();

        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_inst) = candidate.drc_instructions_trigger() {
                let upper_value = drc_inst.drc_set_target_loudness_value_upper();

                if upper_value < min_val {
                    min_val = upper_value;
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        // Add all with same smallest `DRC set target loudness value upper`.
        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_inst) = candidate.drc_instructions_trigger() {
                let upper_value = drc_inst.drc_set_target_loudness_value_upper();

                if upper_value == min_val {
                    self.add(candidate)?;
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }

        Ok(())
    }

    /// Selects all DRC sets whose output peak level value is same as largest output peak level
    /// value in the available DRC sets.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_final_selection_peak_value_largest(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();
        let mut largest_peak_level = -10000.0f32;
        let mut peak_level;
        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            peak_level = candidate.output_peak_level;

            if peak_level > largest_peak_level {
                largest_peak_level = peak_level;
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        // Add all with same largest peak level.
        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            peak_level = candidate.output_peak_level;

            if peak_level == largest_peak_level {
                self.add(candidate)?;
            }
        }

        Ok(())
    }

    /// Selects single DRC set whose DRC set ID is largest in the available DRC sets.
    ///
    /// # Parameters
    ///
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_final_selection_drc_set_id(
        &mut self,
        candidates_potential: &Selection,
    ) -> Result<(), SelectionProcessError> {
        let mut largest_id = -1000;
        let mut id;
        let num_selc_data: usize = usize::from(candidates_potential.num_data());
        let max_data_items = candidates_potential.data.len();
        let mut candidate_selected_local = None;

        for candidate in candidates_potential
            .data
            .iter()
            .take(num_selc_data.min(max_data_items))
        {
            if let Some(drc_inst) = candidate.drc_instructions_trigger() {
                id = i16::from(drc_inst.drc_set_id());

                if id > largest_id {
                    largest_id = id;
                    candidate_selected_local = Some(candidate);
                }
            }
        }

        if num_selc_data > max_data_items {
            return Err(SelectionProcessError::NotOk);
        }

        if let Some(candidate) = candidate_selected_local {
            self.add(candidate)?;
        } else {
            return Err(SelectionProcessError::NotOk);
        }

        Ok(())
    }

    /// Selects all DRC sets whose target loudness upper value is same as smallest value in the
    /// available DRC sets and includes requested target loudness value in the loudness range.
    ///
    /// # Parameters
    ///
    /// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
    /// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
    ///   processed.
    /// - `target_loudness`: Target loudness value of selection process input.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn drc_set_final_selection_target_loudness(
        candidates_potential: &mut &mut Selection,
        candidates_selected: &mut &mut Selection,
        target_loudness: f32,
    ) -> Result<(), SelectionProcessError> {
        let num_selc_data = usize::from((*candidates_potential).num_data());
        for idx in 0..num_selc_data {
            let candidate = (*candidates_potential).data(idx);
            if let Some(selc_data) = candidate {
                if !selc_data.selection_flag {
                    (*candidates_selected).add(selc_data)?;
                }
            } else {
                return Err(SelectionProcessError::NotOk);
            }
        }

        if (*candidates_selected).num_data() == 0 {
            candidates_selected
                .select_smallest_target_loudness_value_upper(candidates_potential)?;
        }

        if (*candidates_selected).num_data() > 1 {
            // Swap references and clean number of data in candidates_selected.
            let candidates_tmp = candidates_potential;
            let candidates_potential = candidates_selected;
            let candidates_selected = candidates_tmp;
            candidates_selected.clear_num_data();

            let num_selc_data = usize::from((*candidates_potential).num_data());
            for idx in 0..num_selc_data {
                let candidate = (*candidates_potential).data(idx);
                if let Some(selc_data) = candidate {
                    if let Some(drc_instruction) = selc_data.drc_instructions_trigger() {
                        if drc_instruction.is_target_loudness_in_range(target_loudness) {
                            (*candidates_selected).add(selc_data)?;
                        }
                    }
                } else {
                    return Err(SelectionProcessError::NotOk);
                }
            }

            if (*candidates_selected).num_data() > 1 {
                // Swap references and clean number of data in candidates_selected.
                let candidates_tmp = candidates_potential;
                let candidates_potential = candidates_selected;
                let candidates_selected = candidates_tmp;
                candidates_selected.clear_num_data();

                candidates_selected
                    .select_smallest_target_loudness_value_upper(candidates_potential)?;
            }
        }

        Ok(())
    }
}

/// Swaps the values at two mutable locations, without deinitializing either one.
///
/// # Parameters
///
/// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
///   processed.
/// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
pub(super) fn swap_selection<'a>(
    candidates_potential: &mut &'a mut Selection,
    candidates_selected: &mut &'a mut Selection,
) {
    std::mem::swap(candidates_potential, candidates_selected);
}

/// Swaps the values at two mutable locations, without deinitializing either one.
/// Also clears value of `num_data` field of `*candidates_selected` after swapping.
///
/// # Parameters
///
/// - `candidates_potential`: `Selection` instance which holds list of potential DRC sets to be
///   processed.
/// - `candidates_selected`: `Selection` instance to store/add selected DRC sets.
pub(super) fn swap_selection_and_clear<'a>(
    candidates_potential: &mut &'a mut Selection,
    candidates_selected: &mut &'a mut Selection,
) {
    std::mem::swap(candidates_potential, candidates_selected);
    (*candidates_selected).clear_num_data();
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_selection_data_new() {
        let sel_data = SelectionData::new();
        assert!(sel_data.drc_instructions_trigger().is_some());
    }

    #[test]
    fn t_selection_data_clear_drc_instr() {
        let mut sel_data = SelectionData::new();
        sel_data.clear();
        assert!(sel_data.drc_instructions_trigger().is_some());
    }

    // --------------------------------------------------------------------- //
    #[test]
    fn t_selection_new() {
        let selection = Selection::new();
        assert!(selection.data[0].drc_instructions_trigger().is_some());
    }

    // Test if there's a valid reference to a DrcInstrunctiosUniDrc instance, with a valid DRC set.
    fn stub_check_drc_set_id(selection: &Selection, sel_data_idx: usize, drc_set_id: i8) {
        match selection.data(sel_data_idx) {
            Some(data) => match data.drc_instructions_trigger() {
                Some(instr) => {
                    assert!(instr.drc_set_id() == drc_set_id);
                }
                None => panic!(),
            },
            None => panic!(),
        }
    }

    #[test]
    fn t_add_data() {
        let mut drc_instr = DrcInstructionsUniDrc::_new();
        const DRC_SET_ID: i8 = 1;
        drc_instr.set_drc_set_id(DRC_SET_ID);

        let mut sel_data = SelectionData::new();
        let mut candidate = Selection::new();

        sel_data.fill_drc_instructions(&drc_instr);

        match candidate.add(&sel_data) {
            Ok(_) => {
                stub_check_drc_set_id(&candidate, 0, DRC_SET_ID);
            }
            Err(_e) => panic!(),
        }
    }

    #[test]
    fn t_swap_selection() {
        let mut drc_instr_1 = DrcInstructionsUniDrc::_new();
        const DRC_SET_ID_1: i8 = 1;
        drc_instr_1.set_drc_set_id(DRC_SET_ID_1);

        let mut sel_data_1 = SelectionData::new();
        sel_data_1.fill_drc_instructions(&drc_instr_1);

        let mut potential = Selection::new();
        match potential.add(&sel_data_1) {
            Ok(_) => {
                stub_check_drc_set_id(&potential, 0, DRC_SET_ID_1);
            }
            Err(_e) => panic!(),
        }

        // ------------------------------------------------------------ //
        let mut drc_instr_2 = DrcInstructionsUniDrc::_new();
        const DRC_SET_ID_2: i8 = 2;
        drc_instr_2.set_drc_set_id(DRC_SET_ID_2);

        let mut sel_data_2 = SelectionData::new();
        sel_data_2.fill_drc_instructions(&drc_instr_2);

        let mut selected = Selection::new();
        match selected.add(&sel_data_2) {
            Ok(_) => {
                stub_check_drc_set_id(&selected, 0, DRC_SET_ID_2);
            }
            Err(_e) => panic!(),
        }
        // ------------------------------------------------------------ //

        let mut ref_potential = &mut potential;
        let mut ref_selected = &mut selected;

        // Test before swap selection.
        stub_check_drc_set_id(ref_potential, 0, DRC_SET_ID_1);
        stub_check_drc_set_id(ref_selected, 0, DRC_SET_ID_2);

        swap_selection(&mut ref_potential, &mut ref_selected);

        // Test swap selection.
        stub_check_drc_set_id(ref_potential, 0, DRC_SET_ID_2);
        stub_check_drc_set_id(ref_selected, 0, DRC_SET_ID_1);
    }
}
