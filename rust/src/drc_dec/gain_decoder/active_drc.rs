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
//! Active (selected) DRC set and interpolation data used by the MPEG-D DRC gain decoder.

use super::super::{
    common::{self, DuckingModification, EffectBit, GainInterpolationType},
    constants::DRCDEC_MAX_CHANNELS,
    drc_error::DrcError,
    reader::{DrcCoefficientsUniDrc, DrcInstructionsUniDrc, UniDrcConfig},
};
use super::{constants::NUM_LNB_FRAMES, linear_node_buffer::NodeLin};
use itertools::izip;

#[repr(C)]
#[derive(Default, Debug)]
/// Struct that holds an active (selected) DRC set and interpolation data to be applied.
pub(super) struct ActiveDrc {
    offset: u8,
    pub(super) drc_instr: DrcInstructionsUniDrc,
    ducking_modification_for_channel_group: [DuckingModification; DRCDEC_MAX_CHANNELS],
    channel_group_for_channel: [i8; DRCDEC_MAX_CHANNELS],
    band_count_for_channel_group: [u8; DRCDEC_MAX_CHANNELS],
    gain_element_for_channel_group: [u8; DRCDEC_MAX_CHANNELS],
    channel_group_is_parametric_drc: [bool; DRCDEC_MAX_CHANNELS],
    /// Number of different DRC gains including all DRC bands.
    gain_element_count: u8,
    /// Linear Node Buffer index for particular DRC channel.
    pub(super) lnb_index_for_channel: [[i32; NUM_LNB_FRAMES]; DRCDEC_MAX_CHANNELS],
}

impl ActiveDrc {
    pub(super) fn band_count_for_channel_group(&self) -> &[u8; DRCDEC_MAX_CHANNELS] {
        &self.band_count_for_channel_group
    }

    pub(super) fn channel_group_is_parametric_drc(&self) -> &[bool; DRCDEC_MAX_CHANNELS] {
        &self.channel_group_is_parametric_drc
    }

    pub(super) fn ducking_modification_for_channel_group(
        &self,
    ) -> &[DuckingModification; DRCDEC_MAX_CHANNELS] {
        &self.ducking_modification_for_channel_group
    }

    pub(super) fn offset(&self) -> u8 {
        self.offset
    }

    /// Reads info from the input parameters and derives additional data, in order to prepare for
    /// processing.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `drc_coeff`: `DrcCoefficientsUniDrc` instance that holds payloads for all available DRC
    ///   gain sequences in one location.
    /// - `drc_instr`: `DrcInstructionsUniDrc` instance that holds payload / metadata of `uniDRC`
    ///   instructions.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    ///   - Err(e): If derived data does not match data from inputs.
    fn generate_drc_instructions_derived_data(
        &mut self,
        uni_drc_config: &UniDrcConfig,
        drc_coeff: &DrcCoefficientsUniDrc,
        drc_instr: &DrcInstructionsUniDrc,
    ) -> Result<(), DrcError> {
        // The DRC gain set indices for channel groups are checked for negative values in
        // `derive_drc_channel_groups()`. Therefore, if `derive_drc_channel_groups()` does not
        // return with an error, it is safe to assume that `gain_set_index_for_channel_group[]`
        // contains non-negative values.
        let mut gain_set_index_for_channel_group = [0_i8; DRCDEC_MAX_CHANNELS];
        let mut num_drc_channel_groups: u8 = 0;

        let dm_for_channel =
            if (drc_instr.drc_set_effect() & (EffectBit::DuckOther | EffectBit::DuckSelf)) != 0 {
                Some(
                    &drc_instr
                        .modification_for_channel_group
                        .ducking_modification,
                )
            } else {
                None
            };

        let dm_for_channel_group =
            if (drc_instr.drc_set_effect() & (EffectBit::DuckOther | EffectBit::DuckSelf)) != 0 {
                Some(&mut self.ducking_modification_for_channel_group)
            } else {
                None
            };

        common::derive_drc_channel_groups(
            dm_for_channel,
            dm_for_channel_group,
            drc_instr.gain_set_index(),
            &mut gain_set_index_for_channel_group,
            &mut self.channel_group_for_channel,
            &mut num_drc_channel_groups,
            usize::from(drc_instr.drc_channel_count()),
            drc_instr.drc_set_effect(),
        )?;

        // Sanity check.
        if num_drc_channel_groups != drc_instr.num_drc_channel_groups() {
            return Err(DrcError::NotOk);
        }

        let num_drc_channel_groups = usize::from(drc_instr.num_drc_channel_groups());

        for (gsi, drc_instr_gsi) in izip!(
            gain_set_index_for_channel_group.iter(),
            drc_instr.gain_set_index_for_channel_group().iter()
        )
        .take(num_drc_channel_groups)
        {
            if *gsi != *drc_instr_gsi {
                return Err(DrcError::NotOk);
            }
        }

        for (seq, is_parametric_drc) in izip!(
            drc_instr.gain_set_index_for_channel_group().iter(),
            self.channel_group_is_parametric_drc.iter_mut()
        )
        .take(num_drc_channel_groups)
        {
            if *seq != -1
                && (uni_drc_config.drc_coefficients_uni_drc_count() == 0
                    || *seq as u8 >= drc_coeff.gain_set_count())
            {
                *is_parametric_drc = true;
            } else {
                *is_parametric_drc = false;
                if *seq as u8 >= drc_coeff.gain_set_count() {
                    return Err(DrcError::NotOk);
                }
            }
        }

        // gain_element_count
        if (drc_instr.drc_set_effect() & (EffectBit::DuckOther | EffectBit::DuckSelf)) != 0 {
            self.band_count_for_channel_group
                .iter_mut()
                .take(num_drc_channel_groups)
                .for_each(|b| *b = 1);
            // One gain element per channel group.
            self.gain_element_count = drc_instr.num_drc_channel_groups();
        } else {
            let mut gain_element_count = 0;
            for (drc_instr_gsi, is_parametric_drc, band_count_for_ch_grp) in izip!(
                drc_instr.gain_set_index_for_channel_group().iter(),
                self.channel_group_is_parametric_drc.iter(),
                self.band_count_for_channel_group.iter_mut(),
            )
            .take(num_drc_channel_groups)
            {
                if *is_parametric_drc {
                    gain_element_count += 1;
                    *band_count_for_ch_grp = 1;
                } else {
                    let seq = *drc_instr_gsi as usize;
                    let band_count = drc_coeff.gain_set(seq).band_count();
                    *band_count_for_ch_grp = band_count;
                    gain_element_count += band_count;
                }
            }
            self.gain_element_count = gain_element_count;
        }

        // Prepare gain_element_for_group (cumulated sum of band_count_for_channel_group).
        self.gain_element_for_channel_group[0] = 0;
        let mut sum = self.gain_element_for_channel_group[0] + self.band_count_for_channel_group[0];
        for (g_el_for_gr, bc_for_ch_gr) in izip!(
            self.gain_element_for_channel_group.iter_mut(),
            self.band_count_for_channel_group.iter()
        )
        .take(num_drc_channel_groups)
        .skip(1)
        {
            *g_el_for_gr = sum;
            sum += *bc_for_ch_gr;
        }

        Ok(())
    }

    /// Initializes an active DRC set. Additional DRC data is further derived from input data. The
    /// `drc_instr` input is copied into this instance afterwards.
    ///
    /// # Parameters
    ///
    /// - `uni_drc_config`: `UniDrcConfig` instance that holds DRC configuration information.
    /// - `drc_coeff`: `DrcCoefficientsUniDrc` instance that holds payloads for all available DRC
    ///   gain sequences in one location.
    /// - `drc_instr`: `DrcInstructionsUniDrc` instance that holds payload / metadata of `uniDRC`
    ///   instructions.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    ///   - Ok(()): init ok.
    ///   - Err(e): If DRC data could not be derived from input.
    pub(super) fn init_drc_instr(
        &mut self,
        uni_drc_config: &UniDrcConfig,
        drc_coeff: &DrcCoefficientsUniDrc,
        drc_instr: &DrcInstructionsUniDrc,
    ) -> Result<(), DrcError> {
        if drc_instr.drc_set_id() >= 0 {
            self.generate_drc_instructions_derived_data(uni_drc_config, drc_coeff, drc_instr)?;
        }

        // Keep deep copy of DRC instructions.
        self.drc_instr.clone_from(drc_instr);

        Ok(())
    }

    /// Applies a segment of interpolated gains to a single channel `audio_ch_buffer`.
    ///
    /// # Parameters
    ///
    /// - `gain_interpol_type`: Type of interpolation. Up to now it implicitely works only for
    ///   `GainInterpolationType::Linear`.
    /// - `time_prev`: Time offset (in samples) of previous interpolation node.
    /// - `t_gain_step`: Delta value btw time offset of current interpolation node and time offset
    ///   of the previous interpolation node. 0 value means that there is nothing to interpolate.
    /// - `start`: Boundary of the region of interpolation segment.
    /// - `stop`: Boundary of the region of interpolation segment.
    /// - `gain_left`: Gain value of left channel.
    /// - `gain_right`: Gain value of right channel.
    /// - `slope_left`: Slope value of left channel.
    /// - `slope_right`: Slope value of right channel.
    /// - `audio_ch_buffer`: Audio buffer of a single channel.
    ///
    /// # Return
    /// # Return
    ///
    /// - `Result<(), DrcError>`
    #[expect(clippy::too_many_arguments)]
    fn interpolate_drc_gain(
        _gain_interpol_type: GainInterpolationType,
        time_prev: i16,
        t_gain_step: u16,
        start: u16,
        stop: u16,
        gain_left: f32,
        gain_right: f32,
        slope_left: f32,
        slope_right: f32,
        audio_ch_buffer: &mut [f32],
    ) -> Result<(), DrcError> {
        if t_gain_step == 0 {
            Ok(())
        } else {
            if gain_left == 1.0 && gain_right == 1.0 && slope_left == 0.0 && slope_right == 0.0 {
                // Gain of 1.0 throughout the section. Skip processing to keep bit-identical
                // signal.
                return Ok(());
            }

            // Sanity check.
            if start > stop {
                return Err(DrcError::NotOk);
            }

            let n_buf = start as i16 + time_prev;

            {
                // GainInterpolationType::Linear

                // runs: Number of gain applications on buffer.
                let runs = stop - start;
                if runs == 0 {
                    return Ok(());
                }

                // a_step: increment per step for linear interpolation.
                let a_step = (gain_right - gain_left) / f32::from(t_gain_step);
                // a: increment per sample for linear interpolation between gainLeft and
                // gainRight.
                let mut a;
                // n: starting position of first gain application with respect to time0.
                let n = f32::from(start);

                for (i, buff) in audio_ch_buffer[n_buf as usize..]
                    .iter_mut()
                    .enumerate()
                    .take(runs as usize)
                {
                    a = a_step * (i as f32 + n) + gain_left;
                    *buff *= a;
                }
            }
            Ok(())
        }
    }

    /// Sets the offset for the current instance. The `ActiveDrc`s are hold up in arrays of:
    /// [ActiveDrc; MAX_ACTIVE_DRCS], for a particular `ACTIVE_DRC_LOCATIONS`.
    ///
    /// # Parameters
    ///
    /// - `offset`: Offset value (expressed in samples).
    pub(super) fn set_offset(&mut self, offset: u8) {
        self.offset = offset;
    }

    /// Number of different DRC gains including all DRC bands.
    pub(super) fn gain_element_count(&self) -> u8 {
        self.gain_element_count
    }

    /// Prepares `lnb_index_for_channel[]`, a map of indices from each channel to its corresponding
    /// linear_node_buffer instance.
    ///
    /// # Parameters
    ///
    /// - `channel_offset`: Start index of physical channels.
    /// - `drc_channel_offset`: Start index of DRC channels.
    /// - `num_processed_channels`: Number of processed audio channels.
    /// - `lnb_index`: Index of the most recent node buffer.
    pub(super) fn prepare_lnb_index(
        &mut self,
        channel_offset: usize,
        drc_channel_offset: usize,
        num_processed_channels: usize,
        lnb_index: usize,
    ) -> Result<(), DrcError> {
        // Sanity checks.
        if channel_offset + drc_channel_offset + num_processed_channels > DRCDEC_MAX_CHANNELS {
            return Err(DrcError::NotOk);
        }

        let start_ch = channel_offset;
        let stop_ch = channel_offset + num_processed_channels;

        for (c, lnb_idx) in self.lnb_index_for_channel[start_ch..stop_ch]
            .iter_mut()
            .enumerate()
        {
            if self.drc_instr.drc_set_id() > 0 {
                let mut drc_channel = c + drc_channel_offset;
                // Fallback for configuration with more physical channels than DRC channels:
                // reuse DRC gain of first channel. This is necessary for HE-AAC mono with stereo
                // output.
                if drc_channel > usize::from(self.drc_instr.drc_channel_count()) {
                    drc_channel = 0;
                }
                let g = self.channel_group_for_channel[drc_channel];
                if g >= 0 && !self.channel_group_is_parametric_drc[g as usize] {
                    lnb_idx[lnb_index] =
                        i32::from(self.offset + self.gain_element_for_channel_group[g as usize]);
                }
            }
        }

        Ok(())
    }

    /// Applies `node_lin` segments of interpolated gains to `audio_ch_buffer`.
    ///
    /// # Parameters
    ///
    /// - `frame_size`: Length of DRC frame.
    /// - `gain_interpol_type`: Type of interpolation. Up to now it implicitely works only for
    ///   `GainInterpolationType::Linear`.
    /// - `num_nodes`: Number of interpolation nodes.
    /// - `node_lin`: Nodes to process.
    /// - `offset`: Offset of the interpolation nodes to be processed.
    /// - `stepsize`: Interpolation step size, expressed as a power of 2 (i.e. `stepsize = 2^exp`).
    ///   The step size is 1 for xHE-AAC, as no sub-band DRC processing is supported.
    /// - `node_previous`: The last node of the previous frame.
    /// - `channel_gain`: Gain value for the current channel.
    /// - `channel_gain_previous`: Gain value for the previous channel.
    /// - `audio_ch_buffer`: Audio buffer of a single channel.
    ///
    /// # Result
    ///
    /// - `Result<(), DrcError>`:
    #[expect(clippy::too_many_arguments)]
    pub(super) fn process_node_segments(
        frame_size: u16,
        gain_interpol_type: GainInterpolationType,
        num_nodes: u8,
        node_lin: &[NodeLin],
        offset: i16,
        _stepsize: u16,
        node_previous: NodeLin,
        channel_gain: f32,
        channel_gain_previous: &mut f32,
        audio_ch_buffer: &mut [f32],
    ) -> Result<(), DrcError> {
        // The last sample of the last interpolation segment.
        let mut time_prev = node_previous.time() + offset;

        let mut gain_lin_prev = node_previous.gain_lin();
        let slope_lin = 0.0;
        let slope_lin_prev = 0.0;

        for node in node_lin.iter().take(usize::from(num_nodes)) {
            // The last sample of the current interpolation segment.
            let time = node.time() + offset;

            let duration = time - time_prev;
            let gain_lin = node.gain_lin();

            // Skip over invalid sections with negative duration.
            if duration < 0 {
                continue;
            }

            if (time_prev >= (frame_size as i16 - 1)) || (time < 0) {
                // This segment (between previous and current node) lies outside of this audio
                // frame.
                time_prev = time;
                gain_lin_prev = gain_lin;
                continue;
            }

            // `start` and `stop` are the boundaries of the region of this segment that lie within
            // this audio frame. Their values are relative to the beginning of this segment.
            // `stop` is the first sample that isn't processed any more.
            let start = (-time_prev).max(1);
            let stop = time.min(frame_size as i16 - 1) - time_prev + 1;
            debug_assert!(start >= 0 && stop >= 0);

            let gain_lin_chan_prev = gain_lin_prev * *channel_gain_previous;

            let gain_lin_chan;
            if start == 1 {
                // This is a new segment. Apply new channel_gain.
                gain_lin_chan = gain_lin * channel_gain;
                *channel_gain_previous = channel_gain;
            } else {
                // This is a segment already processed last frame. Needs to be completed with old
                // channel_gain.
                gain_lin_chan = gain_lin * *channel_gain_previous;
            }

            // _stepsize is 1 for xHE-AAC, as no sub-band DRC processing is supported.
            debug_assert!(_stepsize == 1);

            Self::interpolate_drc_gain(
                gain_interpol_type,
                time_prev,
                duration as u16,
                start as u16,
                stop as u16,
                gain_lin_chan_prev,
                gain_lin_chan,
                slope_lin_prev,
                slope_lin,
                audio_ch_buffer,
            )?;

            time_prev = time;
            gain_lin_prev = gain_lin;
        }
        Ok(())
    }
}
