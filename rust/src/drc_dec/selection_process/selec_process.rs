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
//! DRC selection process.

use super::super::{
    common::CodecMode,
    reader::{LoudnessInfoSet, UniDrcConfig},
};
use super::{
    feature_request::TargetConfigRequestType,
    selec_data::{self, Selection},
    selec_error::SelectionProcessError,
    selec_input::SelectionProcessInput,
    selec_output::SelectionProcessOutput,
    selec_user_params::SelProcUserParam,
};
#[repr(C)]
#[derive(Default, Debug)]
/// `SelectionProcess` structure that holds input and selection data for DRC selection process.
pub(in super::super) struct SelectionProcess {
    /// Codec mode for DRC selection process.
    codec_mode: CodecMode,
    /// `SelectionProcessInput` instance which holds input parameters for DRC selection process.
    selec_proc_input: SelectionProcessInput,
    /// Selection data list for selection processing.
    // 2 instances: one before and one after DRC selection.
    selections: [Selection; 2],
}

impl SelectionProcess {
    /// Creates a new `SelectionProcess` instance.
    pub(in super::super) fn new() -> SelectionProcess {
        Self::default()
    }

    /// Initializes selection process input to default value.
    pub(in super::super) fn init(&mut self) {
        self.selec_proc_input.init_default_params();
    }

    /// Sets codec mode value for DRC selection process.
    ///
    /// # Parameters
    ///
    /// - `mode`: Value of enum type `CodecMode`.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn set_codec_mode(
        &mut self,
        mode: CodecMode,
    ) -> Result<(), SelectionProcessError> {
        match mode {
            CodecMode::Mpeg4_Aac | CodecMode::MpegD_Usac => {
                self.codec_mode = mode;
            }
            CodecMode::Undefined => {
                return Err(SelectionProcessError::NotOk);
            }
        }

        self.selec_proc_input.init_code_mode_params(mode);

        Ok(())
    }

    /// Sets a specific parameter of `self`, by using `SelProcUserParam`.
    ///
    /// # Parameters
    ///
    /// - `request_type`: See enum `SelProcUserParam`.
    /// - `diff`: Checks wether a parameter value has been changed.
    ///
    /// # Return
    ///
    /// - `Ok()` - parameter has been set.
    /// - `Err()`- `SelectionProcessError::ParamOutOfRange`, in case of invalid argument.
    pub(in super::super) fn set_param(
        &mut self,
        request_type: SelProcUserParam,
        diff: Option<&mut bool>,
    ) -> Result<(), SelectionProcessError> {
        self.selec_proc_input.set_param(request_type, diff)
    }

    /// A bitstream can carry multiple DRCs for various purposes. The selection process selects DRC
    /// set that best matches the requirements for the given playback scenario. The selection
    /// process is performed in three stages. DRC sets with only a “Fade”, “Duck other” or
    /// “Duck/Level self” effect are automatically selected by the decoder without using the
    /// three-stage selection process.
    ///
    /// `Three stages`:
    ///
    ///  1. A pre-selection that discards all DRC sets that are not applicable because they do not
    ///     match a target channel configuration, do not support the decoder target loudness, or
    ///     have more clipping than requested.
    ///
    ///  2. A main selection process based on requested DRC set features.
    ///
    ///  3. A final selection that picks a single DRC set if multiple DRC sets are applicable.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `loudness_infoset`: `LoudnessInfoSet` instance that holds loudness info albums and set
    ///   extension information.
    /// - `selc_proc_output`: `SelectionProcessOutput` instance, updated after successful DRC set
    ///   selection.
    ///
    /// # Return
    ///
    /// - Result<(), SelectionProcessError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SelectionProcessError)` if there is an error.
    pub(in super::super) fn process(
        &mut self,
        uni_drc_config: &mut UniDrcConfig,
        loudness_infoset: &LoudnessInfoSet,
        selc_proc_output: &mut SelectionProcessOutput,
    ) -> Result<(), SelectionProcessError> {
        let (selected, potential) = self.selections.split_at_mut(1);
        let mut candidates_selected = &mut selected[0];
        let mut candidates_potential = &mut potential[0];

        candidates_selected.clear_num_data();
        candidates_potential.clear_num_data();

        if !uni_drc_config.generate_virtual_drc_sets() {
            return Err(SelectionProcessError::NotOk);
        }

        self.selec_proc_input.base_channel_count =
            uni_drc_config.channel_layout().base_channel_count();

        if self.selec_proc_input.target_config_request_type != TargetConfigRequestType::DownmixId
            || (self.selec_proc_input.target_config_request_type
                == TargetConfigRequestType::DownmixId
                && self.selec_proc_input.num_downmix_id_requests == 0)
        {
            match self
                .selec_proc_input
                .channel_layout_to_downmix_id_mapping(uni_drc_config)
            {
                Ok(_) => (),
                Err(err) => {
                    if err < SelectionProcessError::Warning {
                        return Err(err);
                    }
                }
            }
        }

        self.selec_proc_input.drc_set_pre_selection(
            &mut candidates_potential,
            &mut candidates_selected,
            uni_drc_config,
            loudness_infoset,
        )?;

        if self.selec_proc_input.is_album_mode {
            selec_data::swap_selection_and_clear(
                &mut candidates_potential,
                &mut candidates_selected,
            );
            candidates_selected.select_album_loudness(candidates_potential, loudness_infoset)?;

            if candidates_selected.num_data() == 0 {
                selec_data::swap_selection(&mut candidates_potential, &mut candidates_selected);
            }
        }

        selec_data::swap_selection_and_clear(&mut candidates_potential, &mut candidates_selected);

        self.selec_proc_input.drc_set_request_selection(
            uni_drc_config,
            &mut candidates_potential,
            &mut candidates_selected,
        )?;

        self.selec_proc_input
            .drc_set_final_selection(&mut candidates_potential, &mut candidates_selected)?;

        match selc_proc_output.generate_output_info(
            &self.selec_proc_input,
            candidates_selected.data(0).unwrap(),
            uni_drc_config,
            loudness_infoset,
        ) {
            Ok(_) => (),
            Err(err) => {
                if err < SelectionProcessError::Warning {
                    return Err(err);
                }
            }
        }

        selc_proc_output.select_downmix_matrix(uni_drc_config);

        Ok(())
    }
}
