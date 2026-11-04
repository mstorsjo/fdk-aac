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
//! Guided envelope shaping (GES)
//!
//! GES is a tool that specifically address the preservation of the temporal structure
//! in the output signal, by restoring its broadband envelope.

use itertools::izip;
use num_complex::Complex;

use super::process::HybData;
use super::{constants::*, error_codes::SacDecoderError, nlc};
use crate::common::{arith_ops, bitstream::Bitstream};

/// Enum which describes the possible variants of GES processing.
#[repr(C)]
#[derive(PartialEq, Eq, Debug)]
enum InpSignalSelector {
    DryWet = 0,
    Dmx = 1,
}

/// Start band for part energy and frame energy calculation.
const BB_ENV_START: usize = 0;
/// End band for part energy and frame energy calculation.
const BB_ENV_END: usize = 9;
/// Total bands for part energy and frame energy calculation.
const BB_ENV_SIZE: usize = BB_ENV_END - BB_ENV_START;

/// Minimum number of hybrid bands considered in guided envelope shaping.
const BB_ENV_MIN_HYB_BANDS: usize = 23;
/// Maximum number of hybrid bands considered in guided envelope shaping.
const BB_ENV_MAX_HYB_BANDS: usize = 42;

/// Start band for energy calculation.
const BB_ENV_NRG_START: usize = 12;
/// Start band for envelope shaping.
const BB_ENV_SHAPE_START: usize = 6;

/// Band threshold that needs to be considerd in GesData::calc_part_nrg_hq().
const BB_ENV_THR_BANDS: usize = BB_ENV_MIN_HYB_BANDS - BB_ENV_NRG_START;

/// Maximum number of input channels for extracting the downmix envelope.
const BB_ENV_INPUT_CHANNELS: usize = 1;

const BB_ENV_ALPHA: f32 = 0.99637845575;
const BB_ENV_BETA: f32 = 0.96436909488;
const BB_ENV_ONE_MINUS_ALPHA: f32 = 1.0 - BB_ENV_ALPHA;
const BB_ENV_ONE_MINUS_BETA: f32 = 1.0 - BB_ENV_BETA;

const ABS_THR_FL: f32 = 1e-9f32;
const LAMBDA_4: f32 = 4.0;

static ENV_SHAPE_DATA_TABLE: [f32; 5] = [
    0.500000000000000,
    0.707106781186548,
    1.000000000000000,
    1.414213562373100,
    2.000000000000000,
];

/// Structure that holds guided envelope shaping data.
#[repr(C)]
#[derive(Debug)]
pub(super) struct GesData {
    /// Specifies for certain output channels whether GES is applied.
    temp_shape_enable_channel_ges: [bool; MAX_OUTPUT_CHANNELS],
    /// Contains information about broadband envelope ratios
    /// of an input channel vs. downmix channel.
    /// Envelope ratios have time slot granularity.
    bs_env_shape_data: [[u8; MAX_TIME_SLOTS]; MAX_OUTPUT_CHANNELS],
    norm_nrg_prev: [f32; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
    frame_nrg_prev: [f32; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
    part_nrg_prev: [[f32; BB_ENV_SIZE]; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
}

impl Default for GesData {
    fn default() -> Self {
        GesData {
            temp_shape_enable_channel_ges: [false; MAX_OUTPUT_CHANNELS],
            bs_env_shape_data: [[0; MAX_TIME_SLOTS]; MAX_OUTPUT_CHANNELS],
            norm_nrg_prev: [1.0f32; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
            frame_nrg_prev: [0.0f32; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
            part_nrg_prev: [[0.0f32; BB_ENV_SIZE]; MAX_OUTPUT_CHANNELS + MAX_INPUT_CHANNELS],
        }
    }
}

impl GesData {
    /// Calculates the energy of a time slot of the hybrid `inp_signal` into `BB_ENV_SIZE` smaller
    /// parts of energy.
    ///
    /// # Parameters
    ///
    /// - `hyb_signal`: Hybrid input signal.
    /// - `part_nrg`: `BB_ENV_SIZE` output calculated energy slots.
    /// - `num_hyb_bands`: Number of hybrid bands.
    #[inline(always)]
    fn calc_part_nrg_hq(
        hyb_signal: &[Complex<f32>],
        part_nrg: &mut [f32; BB_ENV_SIZE],
        num_hyb_bands: usize,
    ) {
        // qs = 12, 13, 14, 15.
        for (out_nrg, inp_sig) in izip!(part_nrg.iter_mut(), hyb_signal.iter()).take(4) {
            *out_nrg = inp_sig.norm_sqr();
        }

        // qs = 16, 17
        part_nrg[4] = arith_ops::sum_norm_sqr(&hyb_signal[4..6]);

        // qs = 18, 19, 20
        part_nrg[5] = arith_ops::sum_norm_sqr(&hyb_signal[6..9]);

        // qs = 21, 22
        part_nrg[6] = arith_ops::sum_norm_sqr(&hyb_signal[9..11]);

        if num_hyb_bands > BB_ENV_THR_BANDS {
            // qs = 23, 24.
            part_nrg[6] += arith_ops::sum_norm_sqr(&hyb_signal[11..13]);

            // qs = 25, 26, 29, 28, 29.
            part_nrg[7] = arith_ops::sum_norm_sqr(&hyb_signal[13..18]);

            // qs = 30 ... min(41,num_hyb_bands-1).
            part_nrg[8] = arith_ops::sum_norm_sqr(&hyb_signal[18..num_hyb_bands]);
        } else {
            part_nrg[7] = 0.0;
            part_nrg[8] = 0.0;
        }
    }

    #[inline(always)]
    /// Stores the result of `hyb_output_dry + hyb_output_wet` in `out_mix`.
    ///
    /// # Parameters
    ///
    /// - `out_mix`: Output buffer.
    /// - `hyb_output_dry`: Hybrid dry signal.
    /// - `hyb_output_wet`: Hybrid wet signal.
    fn combine_dry_wet(
        out_mix: &mut [Complex<f32>],
        hyb_output_dry: &[Complex<f32>],
        hyb_output_wet: &[Complex<f32>],
    ) {
        izip!(
            out_mix.iter_mut(),
            hyb_output_dry.iter(),
            hyb_output_wet.iter()
        )
        .for_each(|(out, dry, wet)| *out = *dry + *wet);
    }

    /// Clears `GesBsFrameData::temp_shape_enable_channel_ges[..]`.
    pub(super) fn clear_data(&mut self) {
        self.temp_shape_enable_channel_ges.fill(false);
    }

    /// Extracts the envelope of hybrid signal from `HybData` (current time slot).
    ///
    /// # Parameters
    ///
    /// - `hyb_data`: Reference to `HybData` struct, to process the hybrid data buffers for each.
    ///   `channel`.
    /// - `inp_sig_sel`: Select what input signal to process from `hyb_data`.
    /// - `prev_nrg_ch_offset`: Offset for GesData struct members.
    /// - `envelope_ch`: Extracted and calculated envelopes for each channel.
    fn extract(
        &mut self,
        hyb_data: &HybData,
        inp_sig_sel: InpSignalSelector,
        prev_nrg_ch_offset: usize,
        envelope_ch: &mut [f32],
    ) {
        let mut is_shape_active = true;
        let num_bands = hyb_data.hybrid_bands.min(BB_ENV_MAX_HYB_BANDS) - BB_ENV_NRG_START;
        let mut scratch_buf = [Complex {
            re: 0.0_f32,
            im: 0.0_f32,
        }; BB_ENV_MAX_HYB_BANDS];
        // Stores the energy of current incoming hybrid data (i.e. current time slot).
        let mut part_nrg = [0.0_f32; BB_ENV_SIZE];

        for (ch, env_data) in envelope_ch.iter_mut().enumerate() {
            let ch_offset;
            let work_buff: &[Complex<f32>];
            if inp_sig_sel == InpSignalSelector::DryWet {
                is_shape_active = self.temp_shape_enable_channel_ges[ch];
                ch_offset = ch;

                GesData::combine_dry_wet(
                    &mut scratch_buf,
                    &hyb_data.hyb_output_cplx_dry[ch]
                        [BB_ENV_NRG_START..BB_ENV_NRG_START + num_bands],
                    &hyb_data.hyb_output_cplx_wet[ch]
                        [BB_ENV_NRG_START..BB_ENV_NRG_START + num_bands],
                );
                work_buff = &scratch_buf;
            } else {
                ch_offset = ch + prev_nrg_ch_offset;
                work_buff = &hyb_data.hyb_input_cplx[BB_ENV_NRG_START..];
            }

            // Calculate the energy for the incoming hybrid data.
            GesData::calc_part_nrg_hq(work_buff, &mut part_nrg, num_bands);

            // Filter self.part_nrg_prev[ch_offset] with newly calculated part_nrg values.
            self.filter_part_nrg_prev(ch_offset, &part_nrg[..BB_ENV_END]);

            // Calculate energy for the whole frame (based on current time slot data and
            // previous time slot data).
            let mean_frame_nrg =
                part_nrg[..BB_ENV_END].iter().fold(0.0, |accu, n| accu + n) / (BB_ENV_SIZE as f32);
            let frame_nrg = BB_ENV_ONE_MINUS_ALPHA * mean_frame_nrg
                + BB_ENV_ALPHA * self.frame_nrg_prev[ch_offset];

            // Update self.frame_nrg_prev[ch_offset] with newly calculated frame_nrg value.
            self.frame_nrg_prev_mut(ch_offset, frame_nrg);

            // Calculate new envelope based on current time slot data and previous time slot data.
            let mut env_ch = 0.0_f32;
            for (curr_part_nrg, prev_part_nrg) in
                izip!(part_nrg.iter(), self.part_nrg_prev[ch_offset].iter()).take(BB_ENV_SIZE)
            {
                env_ch += *curr_part_nrg / (*prev_part_nrg + ABS_THR_FL);
            }
            env_ch *= frame_nrg;

            // Calculate norm energy for the whole frame (based on current time slot data and
            // previous time slot data).
            let norm_nrg =
                BB_ENV_ONE_MINUS_BETA * env_ch + BB_ENV_BETA * self.norm_nrg_prev[ch_offset];

            // Update self.norm_nrg_prev[ch_offset] with newly calculated norm_nrg.
            self.norm_nrg_prev_mut(ch_offset, norm_nrg);

            if is_shape_active {
                env_ch = (env_ch / (norm_nrg + ABS_THR_FL)).sqrt();
            }
            *env_data = env_ch;
        }
    }

    #[inline(always)]
    fn frame_nrg_prev_mut(&mut self, ch_offset: usize, val: f32) {
        self.frame_nrg_prev[ch_offset] = val;
    }

    /// Initializes 'GesData' structure.
    ///
    /// # Parameters
    ///
    /// - `do_init`: Flag to set the initialization.
    pub(super) fn init(&mut self, do_init: bool) {
        if do_init {
            *self = Default::default();
        }
    }

    /// Creates new 'GesData' instance.
    pub(super) fn new() -> GesData {
        GesData::default()
    }

    #[inline(always)]
    fn norm_nrg_prev_mut(&mut self, ch_offset: usize, val: f32) {
        self.norm_nrg_prev[ch_offset] = val;
    }

    #[inline(always)]
    fn filter_part_nrg_prev(&mut self, ch_offset: usize, part_nrg: &[f32]) {
        for (part_nrg_prev, part_nrg) in
            izip!(self.part_nrg_prev[ch_offset].iter_mut(), part_nrg.iter()).take(BB_ENV_SIZE)
        {
            *part_nrg_prev = BB_ENV_ONE_MINUS_ALPHA * *part_nrg + BB_ENV_ALPHA * *part_nrg_prev;
        }
    }

    /// Reads `GES` specific data from `bs`.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data (in).
    /// - `num_channels`: Number of channels.
    /// - `num_time_slots`: Number of time slots.
    ///
    /// # Return
    /// - `SacDecoderError`
    pub(super) fn read(
        &mut self,
        bs: &mut Bitstream,
        num_channels: usize,
        num_time_slots: usize,
    ) -> Result<(), SacDecoderError> {
        for is_ges_enabled in self
            .temp_shape_enable_channel_ges
            .iter_mut()
            .take(num_channels)
        {
            *is_ges_enabled = bs.read_bit() != 0;
        }

        for (ch, is_ges_enabled) in self
            .temp_shape_enable_channel_ges
            .iter()
            .take(num_channels)
            .enumerate()
        {
            if *is_ges_enabled {
                let mut env_shape_data_tmp = [0_u8; MAX_TIME_SLOTS];
                nlc::huff_dec_reshape(bs, &mut env_shape_data_tmp, num_time_slots)?;
                for (bs_env_shape_data, tmp_env_shape_data) in izip!(
                    self.bs_env_shape_data[ch].iter_mut(),
                    env_shape_data_tmp.iter()
                )
                .take(num_time_slots)
                {
                    if *tmp_env_shape_data > LAMBDA_4 as u8 {
                        return Err(SacDecoderError::ParseError);
                    }
                    *bs_env_shape_data = *tmp_env_shape_data;
                }
            }
        }
        Ok(())
    }

    /// Reshapes the dry signal of current `ts`, according to the transmitted envelope.
    ///
    /// # Parameters
    ///
    /// - `hyb_data`: Reference to `HybData` struct, to process the hybrid data buffers.
    /// - `num_output_channels`: Number of output channels.
    /// - `ts`: Time slot.
    pub fn reshape(&mut self, hyb_data: &mut HybData, num_output_channels: usize, ts: usize) {
        let mut env_mix = [0.0_f32; MAX_OUTPUT_CHANNELS];
        let mut env_dmx = [0.0_f32; BB_ENV_INPUT_CHANNELS];
        let num_bands = hyb_data.hybrid_bands - BB_ENV_SHAPE_START;

        // Extract downmix envelope(s) for current `ts`.
        self.extract(
            hyb_data,
            InpSignalSelector::Dmx,
            num_output_channels,
            &mut env_dmx,
        );
        // Extract dry and wet envelopes for current `ts`.
        self.extract(
            hyb_data,
            InpSignalSelector::DryWet,
            num_output_channels,
            &mut env_mix,
        );

        for ch in 0..num_output_channels {
            // Reshape dry signal according to transmitted envelope.
            if self.temp_shape_enable_channel_ges[ch] {
                // Calculate slot_amp_wet vs slot_amp_dry ratio.
                let slot_amp_ratio = GesData::slot_nrg_ratio(
                    &hyb_data.hyb_output_cplx_dry[ch][BB_ENV_SHAPE_START..num_bands],
                    &hyb_data.hyb_output_cplx_wet[ch][BB_ENV_SHAPE_START..num_bands],
                );

                // De-quantize GES data.
                let env_shape = ENV_SHAPE_DATA_TABLE[usize::from(self.bs_env_shape_data[ch][ts])];

                // Calculate dry coefficient.
                let mut dry_fac = (env_shape * env_dmx[0]) / (env_mix[ch] + ABS_THR_FL);

                // Limit dry_fac.
                dry_fac =
                    (dry_fac + slot_amp_ratio * (dry_fac - 1.0)).clamp(1.0 / LAMBDA_4, LAMBDA_4);

                // Reshape dry signal.
                GesData::shape(
                    &mut hyb_data.hyb_output_cplx_dry[ch]
                        [BB_ENV_SHAPE_START..BB_ENV_SHAPE_START + num_bands],
                    dry_fac,
                );
            }
        }
    }

    /// Shapes the dry signal, part of `HybData`.
    ///
    /// # Parameters
    ///
    /// - `hyb_output_dry`: Hybrid dry signal.
    /// - `dry_fac`: Coeffiecient for shaping the dry signal.
    fn shape(hyb_output_dry: &mut [Complex<f32>], dry_fac: f32) {
        hyb_output_dry
            .iter_mut()
            .for_each(|out_dry| *out_dry *= dry_fac);
    }

    /// Calculates the ratio of dry vs. wet energies of hybrid signals, belonging to a time slot.
    ///
    /// # Parameters
    ///
    /// - `hyb_output_dry`: Hybrid dry signal.
    /// - `hyb_output_wet`: Hybrid wet signal.
    #[inline(always)]
    fn slot_nrg_ratio(hyb_output_dry: &[Complex<f32>], hyb_output_wet: &[Complex<f32>]) -> f32 {
        let slot_amp_dry = arith_ops::sum_norm_sqr(hyb_output_dry);
        let slot_amp_wet = arith_ops::sum_norm_sqr(hyb_output_wet);

        // Ratio.
        (slot_amp_wet / (slot_amp_dry + ABS_THR_FL)).sqrt()
    }
}
