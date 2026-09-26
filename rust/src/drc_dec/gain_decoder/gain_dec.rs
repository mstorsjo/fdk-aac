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
//! MPEG-D DRC gain decoder structure.

use super::super::{
    common::{self, DelayMode, EffectBit, ProcessingLocation, SubbandDomainMode},
    constants::{
        ACTIVE_DRC_LOCATIONS, DOWNMIX_ID_ANY_DOWNMIX, DOWNMIX_ID_BASE_LAYOUT, DRCDEC_MAX_CHANNELS,
        DRCDEC_MAX_FRAME_SIZE, DRCDEC_MAX_SAMPLERATE, DRCDEC_MIN_SAMPLERATE, LOCATION_SELECTED,
        MAX_ACTIVE_DRCS, MAX_NODES, MAX_SEQUENCES,
    },
    drc_error::DrcError,
    reader::{DrcCoefficientsUniDrc, UniDrcConfig, UniDrcGain},
};
use super::{
    active_drc::ActiveDrc, constants::NUM_LNB_FRAMES, gain_buffers::DrcGainBuffers,
    node_modification::NodeModification,
};
use itertools::izip;

/// DRC gain decoder parameters.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub(in super::super) enum DrcGainDecoderParam {
    /// Range: (1..=DRCDEC_MAX_FRAME_SIZE);
    FrameSize(u16),
    /// Range: (DRCDEC_MIN_SAMPLERATE..=DRCDEC_MAX_SAMPLERATE)
    SampleRate(u32),
}

#[repr(C)]
#[derive(Default, Debug)]
pub(in super::super) struct DrcGainDecoder {
    /// Smallest permitted DRC gain sample interval (in samples).
    delta_t_min_default: u16,
    /// DRC frame size (in samples). Each DRC frame contains DRC data to generate the DRC gain for
    /// the duration of a DRC frame.
    frame_size: u16,
    /// Loudness normalization gain (expressed in dB).
    loudness_normalization_gain_db: f32,
    /// A bit field that indicates whether the received gains are applied immediately or with a
    /// delay of one frame.
    delay_mode: DelayMode,
    /// `DrcCoefficientsUniDrc` instance that holds payloads for all available DRC gain sequences
    /// in one location.
    drc_coeff: DrcCoefficientsUniDrc,
    /// Number of active DRCs sets for each DRC processing location (see `ProcessingLocation`).
    num_active_drcs: [usize; ACTIVE_DRC_LOCATIONS],
    /// Active DRC sets used for DRC reconfiguration, for each DRC processing location (see
    /// `ProcessingLocation`).
    active_drc: [[ActiveDrc; MAX_ACTIVE_DRCS]; ACTIVE_DRC_LOCATIONS],
    /// Relates to location ProcessingLocation::Drc1 (before downmix).
    multi_band_active_drc_index: i32,
    /// Relates to location ProcessingLocation::Drc1 (before downmix).
    channel_gain_active_drc_index: i32,
    channel_gain: [f32; DRCDEC_MAX_CHANNELS],
    channel_gain_prev: [f32; DRCDEC_MAX_CHANNELS],
    /// Instance that holds interpolation nodes to be applied for the gains of each DRC sequence.
    drc_gain_buffers: DrcGainBuffers,
    /// For the first frame, the channel gains are applied from first to the last sample. For all
    /// other frames, they are interpolated from previous to current value.
    is_not_first_frame: bool,
    is_time_domain_supported: bool,
    subband_domain_supported: SubbandDomainMode,
}

impl DrcGainDecoder {
    /// Adds a virtual DRC set to the `active_drcs`, if there is none present yet. This removes
    /// clicks if `num_active_drcs` changes during runtime due to an user interaction.
    fn add_virtual_to_active_drc(&mut self) {
        let mut is_virtual_drc_active;

        for (l, (n_active_drcs_in_loc, active_drc_arr)) in
            izip!(self.num_active_drcs.iter_mut(), self.active_drc.iter_mut()).enumerate()
        {
            // Cross-check if adding a virtual DRC set is possible.
            if *n_active_drcs_in_loc >= MAX_ACTIVE_DRCS {
                continue;
            }

            // Don't add virtual DRC for subband processing, because clicks don't occur there.
            if l == 0 && self.multi_band_active_drc_index >= 0 {
                continue;
            }

            is_virtual_drc_active = false;

            // Cross-check if there is already a virtual DRC set active.
            let cnt = *n_active_drcs_in_loc;
            for active_drc_elem in active_drc_arr.iter_mut().take(cnt) {
                if active_drc_elem.drc_instr.drc_set_id() < 0 {
                    is_virtual_drc_active = true;
                }
            }

            if !is_virtual_drc_active {
                // Add a virtual DRC set.
                let drc_instr = &mut active_drc_arr[cnt].drc_instr;

                if l == 0 {
                    drc_instr.generate_virtual_drc_set(DOWNMIX_ID_BASE_LAYOUT, -1);
                } else if l == 1 {
                    drc_instr.generate_virtual_drc_set(DOWNMIX_ID_ANY_DOWNMIX, -1);
                    drc_instr.set_drc_apply_to_downmix(true);
                } else {
                    drc_instr.clear();
                    drc_instr.set_drc_set_id(-1);
                    continue;
                }

                *n_active_drcs_in_loc += 1;
            }
        }

        // There should now always be a virtual DRC set on location 0 (before downmix). Pick
        // the first one to apply channel_gains.
        if self.num_active_drcs[0] > 0 && self.channel_gain_active_drc_index == -1 {
            self.channel_gain_active_drc_index = 0;
        }
    }

    /// Creates gain sequence out of gain sequences of last frame for concealment and flushing.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_gain`: Instance that holds `DRC gain sequences` and `DRC gain extension`.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    pub(in super::super) fn conceal(
        &mut self,
        uni_drc_gain: &mut UniDrcGain,
    ) -> Result<(), DrcError> {
        let gain_seq_count =
            usize::from(self.drc_coeff.gain_sequence_count().clamp(1, MAX_SEQUENCES));

        uni_drc_gain.conceal(gain_seq_count, self.frame_size as i16)?;

        Ok(())
    }

    /// Configures `self`, depending on the provided input values.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `num_selected_drc_sets`: Number of selected `DRC sets`.
    /// - `selected_drc_set_ids`: Selected DRC set identifiers.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    /// - `OK(())`: configuration is successful
    /// - `Err(e)`: Input arguments are not correctly initialized.
    pub(in super::super) fn config(
        &mut self,
        uni_drc_config: &UniDrcConfig,
        num_selected_drc_sets: u8,
        selected_drc_set_ids: &[i8],
    ) -> Result<(), DrcError> {
        self.num_active_drcs[..].fill(0);
        self.multi_band_active_drc_index = -1;
        self.channel_gain_active_drc_index = -1;

        let drc_coeff = &mut self.drc_coeff;
        if let Some(inp_drc_coeff) = uni_drc_config.select_drc_coefficients(LOCATION_SELECTED) {
            // Keep deep copy of drc_coefficients.
            drc_coeff.clone_from(inp_drc_coeff);
        } else {
            drc_coeff.init();
        }

        if drc_coeff.is_drc_frame_size_present() && (drc_coeff.drc_frame_size() != self.frame_size)
        {
            return Err(DrcError::NotOk);
        }

        for drc_set_id in selected_drc_set_ids
            .iter()
            .take(usize::from(num_selected_drc_sets))
        {
            self.init_active_drc(uni_drc_config, *drc_set_id)?;
        }

        self.add_virtual_to_active_drc();

        self.init_active_drc_offset()?;

        Ok(())
    }

    /// Gets the DRC `frame_size` (expressed in samples).
    pub(in super::super) fn frame_size(&self) -> u16 {
        self.frame_size
    }

    /// Initializes the internal components from `self`.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    pub(in super::super) fn init(&mut self) -> Result<(), DrcError> {
        // Sanity checks.
        if self.frame_size < 1 || self.delta_t_min_default > self.frame_size {
            return Err(DrcError::NotOk);
        }

        self.init_gain_dec()?;
        self.drc_gain_buffers.init(self.frame_size);

        Ok(())
    }

    /// Initializes the interpolation node indices.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    fn init_gain_dec(&mut self) -> Result<(), DrcError> {
        for active_drc_arr in self.active_drc.iter_mut() {
            for active_drc_elem in active_drc_arr.iter_mut() {
                for lnb_index_arr in active_drc_elem.lnb_index_for_channel.iter_mut() {
                    lnb_index_arr.fill(-1);
                    // Use startup node at the beginning.
                    lnb_index_arr[0] = 0;
                }
            }
        }

        self.channel_gain.fill(1.0);
        // Startup
        self.is_not_first_frame = false;

        Ok(())
    }

    /// Initializes the active DRC sets from specific `ProcessingLocation`, depending on the input
    /// parameters.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `drc_set_id_selected`: Selected `DRC set identifier`.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    fn init_active_drc(
        &mut self,
        uni_drc_config: &UniDrcConfig,
        drc_set_id_selected: i8,
    ) -> Result<(), DrcError> {
        match uni_drc_config.select_drc_instructions(drc_set_id_selected) {
            None => Err(DrcError::NotOk),
            Some(drc_instr) => {
                // Before or after downmix.
                let processing_loc = if drc_instr.is_drc_apply_to_downmix() {
                    ProcessingLocation::Drc2Drc3
                } else {
                    ProcessingLocation::Drc1
                };
                let loc = usize::from(processing_loc);

                let index = self.num_active_drcs[loc];
                let curr_active_drc = &mut self.active_drc[loc][index];
                curr_active_drc.init_drc_instr(uni_drc_config, &self.drc_coeff, drc_instr)?;

                if processing_loc == ProcessingLocation::Drc1 {
                    let mut is_multiband = false;

                    for band_cnt_grp in curr_active_drc
                        .band_count_for_channel_group()
                        .iter()
                        .take(usize::from(drc_instr.num_drc_channel_groups()))
                    {
                        if *band_cnt_grp > 1 {
                            if self.multi_band_active_drc_index != -1 {
                                return Err(DrcError::NotOk);
                            }
                            is_multiband = true;
                        }
                    }

                    if is_multiband {
                        // Keep active_drc index of multiband DRC set.
                        self.multi_band_active_drc_index = self.num_active_drcs[loc] as i32;
                    }
                }

                self.num_active_drcs[loc] += 1;

                if self.num_active_drcs[loc] > MAX_ACTIVE_DRCS {
                    Err(DrcError::NotOk)
                } else {
                    Ok(())
                }
            }
        }
    }

    fn init_active_drc_offset(&mut self) -> Result<(), DrcError> {
        let mut acc_gain_elem_count = 0;

        for (n_active_drcs_in_loc, active_drc_arr) in
            izip!(self.num_active_drcs.iter_mut(), self.active_drc.iter_mut())
        {
            let cnt = *n_active_drcs_in_loc;
            for (a, active_drc_elem) in active_drc_arr.iter_mut().enumerate().take(cnt) {
                active_drc_elem.set_offset(acc_gain_elem_count);
                acc_gain_elem_count += active_drc_elem.gain_element_count();
                if acc_gain_elem_count > MAX_SEQUENCES {
                    *n_active_drcs_in_loc = a;
                    return Err(DrcError::NotOk);
                }
            }
        }

        Ok(())
    }

    /// Creates `DrcGainDecoder` instance.
    pub(in super::super) fn new() -> Self {
        Self {
            multi_band_active_drc_index: -1,
            channel_gain_active_drc_index: -1,
            is_time_domain_supported: true,
            ..Default::default()
        }
    }

    /// Prepares buffers containing linear nodes for each gain sequence.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_gain`: Reference to an instance that holds `DRC gain sequence` and `DRC gain
    ///   extension`.
    /// - `compress`: Compress gain (dB).
    /// - `boost`: Boost gain (dB).
    /// - `loudness_normalization_gain_db`: Gain value expressed in dB.
    /// - `active_drc_index`: Index of active DRC at `active_drc_location`. Max val:
    ///   `MAX_ACTIVE_DRCS`.
    /// - `active_drc_location`: The location where the uniDRC gain sequences can be found.
    ///
    /// # Return
    ///
    /// - `Ok(())`:
    /// - `Err(DrcError)`
    fn prepare_drc_gain(
        &mut self,
        uni_drc_gain: &UniDrcGain,
        compress: f32,
        boost: f32,
        loudness_normalization_gain_db: f32,
        active_drc_index: usize,
        active_drc_location: ProcessingLocation,
    ) -> Result<(), DrcError> {
        let active_drc = &self.active_drc[usize::from(active_drc_location)][active_drc_index];
        let active_drc_instr = &active_drc.drc_instr;

        let mut node_mod = NodeModification::new();
        node_mod.set_drc_set_effect(active_drc_instr.drc_set_effect());
        node_mod.set_compress(compress);
        node_mod.set_boost(boost);
        node_mod.set_loudness_normalization_gain_db(loudness_normalization_gain_db);
        node_mod.set_limiter_peak_target_present(active_drc_instr.is_limiter_peak_target_present());
        node_mod.set_limiter_peak_target(active_drc_instr.limiter_peak_target());

        let mut gain_element_index = 0;

        let num_drc_channel_groups = usize::from(active_drc_instr.num_drc_channel_groups());
        for (gain_set_index, gm_ch_grp, dm, is_ch_grp_parametric_drc, num_drc_bands_ch_grp) in
            izip!(
                active_drc_instr.gain_set_index_for_channel_group().iter(),
                active_drc_instr
                    .gain_modification_for_channel_group()
                    .iter(),
                active_drc.ducking_modification_for_channel_group().iter(),
                active_drc.channel_group_is_parametric_drc().iter(),
                active_drc.band_count_for_channel_group().iter(),
            )
            .take(num_drc_channel_groups)
        {
            if !(*is_ch_grp_parametric_drc) {
                let (dm_opt, gm_ch_grp_opt) = if (node_mod.drc_set_effect()
                    & (EffectBit::DuckOther | EffectBit::DuckSelf))
                    != 0
                {
                    (Some(dm), None)
                } else {
                    (None, Some(gm_ch_grp))
                };

                let num_drc_bands = usize::from(*num_drc_bands_ch_grp);

                let gain_set = self.drc_coeff.gain_set(*gain_set_index as usize);
                let gain_interpol_type = gain_set.gain_interpolation_type();
                for (b, (seq, drc_char)) in izip!(
                    gain_set.gain_sequence_index().iter(),
                    gain_set.drc_characteristic().iter(),
                )
                .enumerate()
                .take(num_drc_bands)
                {
                    // The `LinearNodeBuffer` instance contains a copy of the gain sequences
                    // (consisting of nodes) that are relevant for decoding. It also contains gain
                    // sequences of previous frames.
                    let lnb_index = self.drc_gain_buffers.lnb_index();
                    let lnb = self.drc_gain_buffers.linear_node_buffer_mut(usize::from(
                        active_drc.offset() + gain_element_index,
                    ));
                    lnb.set_gain_interpolation_type(gain_interpol_type);

                    if let Some(gm) = gm_ch_grp_opt {
                        node_mod.prepare_drc_characteristic(
                            drc_char,
                            &self.drc_coeff,
                            Some(&gm[b]),
                        )?;
                    } else {
                        node_mod.prepare_drc_characteristic(drc_char, &self.drc_coeff, None)?;
                    }

                    // Copy a node buffer and convert it from dB to linear.
                    let num_nodes =
                        uni_drc_gain.num_nodes()[usize::from(*seq)].min(MAX_NODES as u8);

                    lnb.set_num_nodes(lnb_index, num_nodes);

                    for (gnode, lin_node) in izip!(
                        uni_drc_gain.gain_nodes()[usize::from(*seq)].iter(),
                        lnb.linear_nodes_mut()[lnb_index].iter_mut()
                    )
                    .take(usize::from(num_nodes))
                    {
                        let (g, _s);
                        if let Some(gm) = gm_ch_grp_opt {
                            (g, _s) =
                                node_mod.to_linear(Some(&gm[b]), dm_opt, gnode.gain_db(), 0.0)?;
                        } else {
                            (g, _s) = node_mod.to_linear(None, dm_opt, gnode.gain_db(), 0.0)?;
                        }

                        lin_node.set_gain_lin(g);
                        lin_node.set_time(gnode.time());
                    }
                    gain_element_index += 1;
                }
            } else {
                // Parametric DRC not supported.
                gain_element_index += 1;
            }
        }
        Ok(())
    }

    /// Pre-process step for DRC gains processing.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_gain`: Instance that holds `DRC gain sequences` and `DRC gain extension`.
    /// - `loudness_normalization_gain_db`: Gain value expressed in dB.
    /// - `boost`: Boost gain (dB).
    /// - `compress`: Compress gain (dB).
    ///
    /// # Return
    /// - `Result<(), DrcError>`:
    pub(in super::super) fn preprocess(
        &mut self,
        uni_drc_gain: &UniDrcGain,
        loudness_normalization_gain_db: f32,
        boost: f32,
        compress: f32,
    ) -> Result<(), DrcError> {
        self.drc_gain_buffers.inc_lnb_index();
        if self.drc_gain_buffers.lnb_index() >= NUM_LNB_FRAMES {
            self.drc_gain_buffers.set_lnb_index(0);
        }

        for loc in 0..ACTIVE_DRC_LOCATIONS {
            for active_drc_index in 0..self.num_active_drcs[loc] {
                self.prepare_drc_gain(
                    uni_drc_gain,
                    compress,
                    boost,
                    loudness_normalization_gain_db,
                    active_drc_index,
                    ProcessingLocation::from(loc),
                )?;
            }
        }

        let drc_gain_buff = &mut self.drc_gain_buffers;
        for active_drcs_loc in self.active_drc.iter_mut() {
            for active_drc in active_drcs_loc.iter_mut() {
                for lnb_idx in active_drc.lnb_index_for_channel.iter_mut() {
                    // No DRC processing.
                    lnb_idx[drc_gain_buff.lnb_index()] = -1;
                }
            }
        }

        Ok(())
    }

    #[expect(clippy::too_many_arguments)]
    /// Applies DRC gain sequences on the time domain in/out signal. The audio buffer must be
    /// de-interleaved and contain a single AAC frame (see `frame_size`).
    ///
    /// # Parameters
    ///
    /// - `active_drc_index`: Index of active DRC at `active_drc_location`. Max val:
    ///   `MAX_ACTIVE_DRCS`.
    /// - `active_drc_location`: The location where the uniDRC gain sequences can be found.
    /// - `delay_samples`:  Number of delay samples to be considered for DRC processing.
    /// - `channel_offset`: Start index of physical (audio) channels.
    /// - `drc_channel_offset`: Start index of DRC channels.
    /// - `num_processed_channels`: Number of processed audio channels.
    /// - `time_data_channel_offset`: Offset that specifies how many samples are in a particular
    ///   audio channel (from `deinterleaved_audio_buffer`).
    /// - `deinterleaved_audio_buffer`: Frame of de-interleaved audio samples.
    fn process_drc_time(
        &mut self,
        active_drc_index: usize,
        active_drc_location: ProcessingLocation,
        delay_samples: u16,
        channel_offset: usize,
        drc_channel_offset: usize,
        num_processed_channels: usize,
        time_data_channel_offset: usize,
        deinterleaved_audio_buffer: &mut [f32],
    ) -> Result<(), DrcError> {
        let active_drc = &mut self.active_drc[usize::from(active_drc_location)][active_drc_index];
        let drc_gain_buffers = &mut self.drc_gain_buffers;
        let lnb_index = drc_gain_buffers.lnb_index();

        let offset = if self.delay_mode == DelayMode::Regular {
            self.frame_size
        } else {
            0
        };

        // If delay_samples is too big, NUM_LNB_FRAMES should be increased.
        if delay_samples + offset > (NUM_LNB_FRAMES - 2) as u16 * self.frame_size {
            return Err(DrcError::NotOk);
        }

        let mut lnb_offset;

        active_drc.prepare_lnb_index(
            channel_offset,
            drc_channel_offset,
            num_processed_channels,
            lnb_index,
        )?;

        // Signal processing loop.
        for (ch_gain_item, ch_gain_prev_item, lnb_idx_ch, time_audio_data_ch_mut) in izip!(
            self.channel_gain.iter(),
            self.channel_gain_prev.iter_mut(),
            active_drc.lnb_index_for_channel.iter(),
            deinterleaved_audio_buffer[(channel_offset * time_data_channel_offset)..]
                .chunks_exact_mut(time_data_channel_offset)
        )
        .take(channel_offset + num_processed_channels)
        .skip(channel_offset)
        {
            let mut gain_one = 1.0_f32;
            let (ch_gain, ch_gain_prev);
            // `self.channel_gain_active_drc_index` set >=0 in `self.add_virtual_to_active_drc()`.
            if active_drc_location == ProcessingLocation::Drc1
                && active_drc_index == self.channel_gain_active_drc_index as usize
            {
                ch_gain = *ch_gain_item;
                ch_gain_prev = ch_gain_prev_item;
            } else {
                ch_gain = gain_one;
                ch_gain_prev = &mut gain_one;
            }

            {
                let mut lnb;
                let mut lnb_prev;
                let mut lnb_pointer_diff;

                lnb_offset = lnb_index as i32 + 1 - NUM_LNB_FRAMES as i32;
                while lnb_offset < 0 {
                    lnb_offset += NUM_LNB_FRAMES as i32;
                }
                let mut lnb_idx = lnb_offset as usize;

                // Loop over all node buffers in linearNodeBuffer. All nodes which are not relevant
                // for the current frame are sorted out inside _processNodeSegments.
                for i in 0..(NUM_LNB_FRAMES - 1) {
                    // Prepare previous node.
                    if lnb_idx_ch[lnb_idx] >= 0 {
                        lnb_prev =
                            drc_gain_buffers.linear_node_buffer(lnb_idx_ch[lnb_idx] as usize);
                    } else {
                        lnb_prev = drc_gain_buffers.dummy_lnb();
                    }
                    let num_nodes_prev = lnb_prev.num_nodes(lnb_idx);
                    let mut node_prev =
                        lnb_prev.linear_nodes()[lnb_idx][usize::from(num_nodes_prev - 1)];
                    let tmp_node_prev_time = node_prev.time();
                    node_prev.set_time(tmp_node_prev_time - self.frame_size as i16);

                    // Prepare current linearNodeBuffer instance.
                    lnb_idx += 1;
                    if lnb_idx >= NUM_LNB_FRAMES {
                        lnb_idx = 0;
                    }

                    // If lnbIndexForChannel changes over time, use the old indices for smooth
                    // transitions.
                    if lnb_idx_ch[lnb_idx] >= 0 {
                        lnb = drc_gain_buffers.linear_node_buffer(lnb_idx_ch[lnb_idx] as usize);
                    } else {
                        lnb = drc_gain_buffers.dummy_lnb();
                    }

                    // Number of frames of offset with respect to lnbPointer.
                    lnb_pointer_diff = i as i16 - (NUM_LNB_FRAMES - 2) as i16;

                    ActiveDrc::process_node_segments(
                        self.frame_size,
                        lnb.gain_interpolation_type(),
                        lnb.num_nodes(lnb_idx),
                        &lnb.linear_nodes()[lnb_idx],
                        lnb_pointer_diff * self.frame_size as i16 + (delay_samples + offset) as i16,
                        1,
                        node_prev,
                        ch_gain,
                        ch_gain_prev,
                        time_audio_data_ch_mut,
                    )?;
                }
            }
        }

        Ok(())
    }

    /// Applies DRC gain sequences on the time domain in/out signal. The audio buffer must be
    /// de-interleaved and contain a single AAC frame (see `frame_size`).
    ///
    /// # Parameters
    ///
    /// - `active_drc_location`: The location where the uniDRC gain sequences can be found.
    /// - `delay_samples`: Number of delay samples to be considered for DRC processing.
    /// - `channel_offset`: Start index of physical (audio) channels.
    /// - `drc_channel_offset`: Start index of DRC channels.
    /// - `num_processed_channels`: Number of processed audio channels.
    /// - `time_data_channel_offset`: Offset that specifies how many samples are in a particular
    ///   audio channel (from `deinterleaved_audio_buffer`).
    /// - `deinterleaved_audio_buffer`: Frame of de-interleaved audio samples.
    ///
    /// # Return
    /// # Return
    ///
    /// `Result<(), DrcError>`:
    #[expect(clippy::too_many_arguments)]
    pub(in super::super) fn process_time_domain(
        &mut self,
        active_drc_location: ProcessingLocation,
        delay_samples: u16,
        channel_offset: usize,
        drc_channel_offset: usize,
        num_processed_channels: usize,
        time_data_channel_offset: usize,
        deinterleaved_audio_buffer: &mut [f32],
    ) -> Result<(), DrcError> {
        if !self.is_time_domain_supported {
            return Err(DrcError::NotOk);
        }

        for active_drc_index in 0..self.num_active_drcs[usize::from(active_drc_location)] {
            //  Apply DRC.
            self.process_drc_time(
                active_drc_index,
                active_drc_location,
                delay_samples,
                channel_offset,
                drc_channel_offset,
                num_processed_channels,
                time_data_channel_offset,
                deinterleaved_audio_buffer,
            )?;
        }

        Ok(())
    }

    /// Set parameters specific to the codec.
    ///
    /// `Note:` The supported processing is only in time domain. Therefore,
    /// `subband_domain_supported` must be set to `SubbandDomainMode::Off`.
    ///
    /// # Parameters
    ///
    /// - `delay_mode`: The delay mode that sets when the DRC gains are applied.
    /// - `is_time_domain_supported`: Sets whether the DRC gain processing can be applied in time
    ///   domain.
    /// - `subband_domain_supported`: Sets whether the DRC gain processing can be applied in
    ///   separate sub-bands.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`:
    pub(in super::super) fn set_codec_dependent_parameters(
        &mut self,
        delay_mode: DelayMode,
        is_time_domain_supported: bool,
        subband_domain_supported: SubbandDomainMode,
    ) -> Result<(), DrcError> {
        if subband_domain_supported != SubbandDomainMode::Off {
            return Err(DrcError::NotOk);
        }

        self.delay_mode = delay_mode;
        self.is_time_domain_supported = is_time_domain_supported;
        self.subband_domain_supported = subband_domain_supported;

        Ok(())
    }

    fn _set_channel_gains(&mut self, num_channel_gains: usize, channel_gain_db: &[f32]) {
        for (inp_ch_gain, ch_gain) in
            izip!(channel_gain_db.iter(), self.channel_gain.iter_mut()).take(num_channel_gains)
        {
            if *inp_ch_gain <= -100.0 {
                *ch_gain = 0.0;
            } else {
                // Add loudness normalization gain (dB) to channel gain (dB).
                let tmp_ch_gain_db = *inp_ch_gain + self.loudness_normalization_gain_db;
                *ch_gain = common::db_to_linear(tmp_ch_gain_db);
            }
        }
    }

    /// Set the channel gains, that will be applied in `DrcGainDecoder::process_time_domain()`.
    ///
    /// # Parameters
    ///
    /// - `num_channels`: Number of physical (audio) channels.
    /// - `channel_gain_db`: Channel gain (expressed in dB).
    pub(in super::super) fn set_channel_gains(
        &mut self,
        num_channels: usize,
        channel_gain_db: &[f32],
    ) -> Result<(), DrcError> {
        if self.channel_gain_active_drc_index >= 0 {
            // Channel gains will be applied in DrcGainDecoder::process_time_domain().
            self._set_channel_gains(num_channels, channel_gain_db);

            if !self.is_not_first_frame {
                // Overwrite previous channel gains at startup.
                for (ch_g_prev, ch_g) in
                    izip!(self.channel_gain_prev.iter_mut(), self.channel_gain.iter())
                        .take(num_channels)
                {
                    *ch_g_prev = *ch_g;
                }
                self.is_not_first_frame = true;
            }
        } else {
            self._set_channel_gains(num_channels, channel_gain_db);
            if self
                .channel_gain
                .iter()
                .take(num_channels)
                .any(|&g| g != 1.0_f32)
            {
                return Err(DrcError::NotOk);
            }
        }
        Ok(())
    }

    /// Sets the loudness normalization gain value (expressed in dB).
    ///
    /// # Parameters
    ///
    /// - `gain_db`: Loudness normalization gain value expressed in dB.
    pub(in super::super) fn set_loudness_normalization_gain_db(
        &mut self,
        gain_db: f32,
    ) -> Result<(), DrcError> {
        self.loudness_normalization_gain_db = gain_db;
        Ok(())
    }

    /// Sets internal parameters of `self`.
    ///
    /// # Parameters
    ///
    /// - `param_type`: Parametrized enum, to pass arguments for specific parameters of `self`.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`:
    pub(in super::super) fn set_param(
        &mut self,
        param_type: DrcGainDecoderParam,
    ) -> Result<(), DrcError> {
        match param_type {
            DrcGainDecoderParam::FrameSize(requested_val) => {
                if !(1..=DRCDEC_MAX_FRAME_SIZE).contains(&requested_val) {
                    return Err(DrcError::ParamOutOfRange);
                }
                self.frame_size = requested_val;
            }
            DrcGainDecoderParam::SampleRate(requested_val) => {
                if !(DRCDEC_MIN_SAMPLERATE..=DRCDEC_MAX_SAMPLERATE).contains(&requested_val) {
                    return Err(DrcError::ParamOutOfRange);
                }
                self.delta_t_min_default = common::get_delta_tmin(requested_val);
            }
        }
        Ok(())
    }
}
