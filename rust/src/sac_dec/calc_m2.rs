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
//! Calculate mix matrix `M2`.
// M2 is a mix matrix containing spatial parameters used for re-creating
// a multi-channel output, based on the input down-mixed signal and input spatial parameters.

use super::common::{
    BsQuantCoarseXxx, BsSmoothMode, DataType, FreqResolution, OpdSmoothingMode, PhaseCoding,
};
use super::constants::MAX_OUTPUT_CHANNELS as MAX_NUM_SAC_OUTPUT_CHANNELS;
use super::constants::{MAX_PARAMETER_BANDS, MAX_PARAMETER_SETS, MAX_V_CHANNELS, N_CLD};

use super::error_codes::SacDecoderError;
use super::sac_map::{self, PbStride, PB_STRIDE_TABLE};
use super::tables::{DEQUANT_ICC, DEQUANT_IPD, H11_NC, H12_NC, SIN_IPD_TAB, SQRT_CLD_M};
use crate::common::bitstream::Bitstream;
use itertools::izip;
use std::cmp::Ordering;
use std::f32::consts::PI;

/// Defines the time constant of the parameter smoothing filter
/// by means of time slots for 1st order IIR smoothing,
/// according to ISO/IEC 23003-1:2007, Table 64.
const SMG_TIME_TABLE: [u16; 4] = [64, 128, 256, 512];

const PI_X_2: f32 = 2.0 * PI;

/// Selects either current `M2` or previous `M2`.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum State {
    Prev,
    #[default]
    Curr,
}

/// Selects either `Left` or `Right` audio channel.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum Side {
    #[default]
    Left,
    Right,
}

/// Selects either `Real` or `Imaginary` part of a complex number.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, Eq, PartialEq)]
pub(super) enum ComplexPart {
    #[default]
    Real,
    Imag,
}

/// It defines how the direct signals and the decorrelated signals shall be
/// combined in order to form the re-created multi-channel output signal.
#[repr(C)]
#[derive(Debug)]
pub(super) struct M2Data {
    /// Number of parameter bands.
    num_parameter_bands: u8,
    /// Number of output channels for `M2` (up to `sac_dec::MAX_NUM_SAC_OUTPUT_CHANNELS`).
    num_m2_rows: u8,
    /// Controls the replacement of the previous M2 parameters with new ones.
    /// I.E.: Overwrites `M2 prev` matrix with values from `M2`.
    overwrite_m2_prev: bool,

    /// One-to-Two Box: Channel Level Difference indices.
    ott_cld: [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
    /// One-to-Two Box: Inter Channel Correlation indices.
    ott_icc: [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
    /// One-to-Two Box: Inter Channel Phase Difference indices.
    ott_ipd: [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],

    /// Real part of 3D Complex M2 mix matrix.
    m2_real: [[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
    /// Imaginary part of 3D Complex M2 mix matrix.
    m2_imag: [[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
    /// Real part of previous 3D Complex M2 mix matrix.
    m2_real_prev: [[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
    /// Imaginary part of previous 3D Complex M2 mix matrix.
    m2_imag_prev: [[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
    /// Indicates whether IPD coding is applied in Mps212Config.
    phase_coding: PhaseCoding,
    /// Number of One-to-Two parameter bands for Inter-Channel Phase Difference.
    num_ott_bands_ipd: u8,
    /// Overall Phase Difference values of left output channel,
    /// estimated based on spatial parameters (CLD, IPD, ICC).
    phase_left: [f32; MAX_PARAMETER_BANDS],
    /// Overall Phase Difference values of right output channel,
    /// estimated based on spatial parameters (CLD, IPD, ICC).
    phase_right: [f32; MAX_PARAMETER_BANDS],
    /// Previous OPD values of left output channel, based on spatial parameters (CLD, IPD, ICC).
    phase_prev_left: [f32; MAX_PARAMETER_BANDS],
    /// Previous OPD values of right output channel, based on spatial parameters (CLD, IPD, ICC).
    phase_prev_right: [f32; MAX_PARAMETER_BANDS],
    /// Used for smoothing the spatial parameters.
    smoothing: Smoothing,
}

impl M2Data {
    /// Copies `M2` matrix to `M2_PREV` matrix.
    pub(super) fn buffer_matrices(&mut self) {
        let num_parameter_bands = usize::from(self.num_parameter_bands);
        let num_m2_rows = usize::from(self.num_m2_rows);

        // Copy m2_real to m2_real_prev.
        Self::copy_curr_to_prev(
            &mut self.m2_real_prev,
            &self.m2_real,
            num_m2_rows,
            num_parameter_bands,
        );

        if self.phase_coding == PhaseCoding::IpdPrediction {
            // Copy m2_imag to m2_imag_prev.
            Self::copy_curr_to_prev(
                &mut self.m2_imag_prev,
                &self.m2_imag,
                num_m2_rows,
                num_parameter_bands,
            );
        }

        // Buffer phase.
        self.phase_prev_left[..num_parameter_bands]
            .copy_from_slice(&self.phase_left[..num_parameter_bands]);
        self.phase_prev_right[..num_parameter_bands]
            .copy_from_slice(&self.phase_right[..num_parameter_bands]);
    }

    /// Copies current matrix to previous matrix.
    fn copy_curr_to_prev(
        prev_matrix: &mut [[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS];
                 MAX_NUM_SAC_OUTPUT_CHANNELS],
        curr_matrix: &[[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
        num_rows: usize,
        num_parameter_bands: usize,
    ) {
        for (row_prev_matrix, row_curr_matrix) in
            izip!(prev_matrix.iter_mut(), curr_matrix.iter()).take(num_rows)
        {
            for (col_prev_matrix, col_curr_matrix) in
                izip!(row_prev_matrix.iter_mut(), row_curr_matrix.iter()).take(MAX_V_CHANNELS)
            {
                col_prev_matrix[..num_parameter_bands]
                    .copy_from_slice(&col_curr_matrix[..num_parameter_bands]);
            }
        }
    }

    /// Extends spatial parameters by 1 (parameter set).
    pub(super) fn extend_frame(
        &mut self,
        data_type: DataType,
        num_param_sets: usize,
        num_param_bands: usize,
    ) -> Result<(), SacDecoderError> {
        if num_param_sets > (MAX_PARAMETER_SETS - 1) {
            return Err(SacDecoderError::WrongParametersets);
        }
        match data_type {
            DataType::Cld => {
                let peek_cld = &mut self.ott_cld[(num_param_sets - 1)..].iter_mut().peekable();
                if let Some(curr_set) = peek_cld.next() {
                    if let Some(next_set) = peek_cld.peek_mut() {
                        next_set[..num_param_bands].copy_from_slice(&curr_set[..num_param_bands]);
                    }
                }
            }
            DataType::Icc => {
                let peek_icc = &mut self.ott_icc[(num_param_sets - 1)..].iter_mut().peekable();
                if let Some(curr_set) = peek_icc.next() {
                    if let Some(next_set) = peek_icc.peek_mut() {
                        next_set[..num_param_bands].copy_from_slice(&curr_set[..num_param_bands]);
                    }
                }
            }
            DataType::Ipd => {
                let peek_ipd = &mut self.ott_ipd[(num_param_sets - 1)..].iter_mut().peekable();
                if let Some(curr_set) = peek_ipd.next() {
                    if let Some(next_set) = peek_ipd.peek_mut() {
                        next_set[..num_param_bands].copy_from_slice(&curr_set[..num_param_bands]);
                    }
                }
            }
        }

        Ok(())
    }

    /// Calculates M2's (mix matrix) spatial parameters.
    ///
    /// # Parameters
    ///
    /// - `param_time_slots`: The specific parameter time slots (i.e. time slot borders) for which
    ///   the parameters are defined.
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    /// - `num_time_slots`: Number of time slots.
    /// - `quant_coarse`: Full or half quantizer resolution.
    /// - `are_ipd_params_avail`: Indicates whether the IPD parameters are available for the current
    ///   frame. I.e. `phase_mode`.
    /// - `do_residual_coding`: Is residual coding used.
    /// - `num_residual_bands`: Number of residual bands.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn calculate_m2(
        &mut self,
        param_time_slots: &[u8],
        param_set_idx: usize,
        num_time_slots: u8,
        quant_coarse: BsQuantCoarseXxx,
        are_ipd_params_avail: bool,
        do_residual_coding: bool,
        num_residual_bands: usize,
    ) {
        if do_residual_coding {
            self.param_2_umx_prediction(num_residual_bands, param_set_idx);

            // Clear h12im and h22im.
            for im_row in self.m2_imag.iter_mut() {
                for im_col2 in im_row.iter_mut().skip(1).take(1) {
                    im_col2.fill(0.0_f32);
                }
            }
        } else {
            self.param_2_umx_ps(param_set_idx);

            if self.phase_coding == PhaseCoding::Ipd {
                self.param_2_umx_ipd_opd(are_ipd_params_avail, param_set_idx);
                self.smoothing.smooth_opd(
                    quant_coarse,
                    param_time_slots,
                    param_set_idx,
                    usize::from(self.num_parameter_bands),
                    &mut self.phase_left,
                    &mut self.phase_right,
                );
            }
        }

        if param_set_idx == 0 && self.overwrite_m2_prev {
            // Copy matrix entries of M2 of the first parameter set to the previous
            // matrices (of the last frame). This avoids the interpolation of
            // incompatible values. E.g. for residual bands the coefficients are
            // calculated differently compared to non-residual bands.
            self.buffer_matrices();
            self.overwrite_m2_prev = false;
        }

        self.smooth_m2(
            if do_residual_coding {
                num_residual_bands
            } else {
                0
            },
            param_time_slots[param_set_idx],
            num_time_slots,
            param_set_idx,
        );

        if let Some(param_time_slot_border) = param_time_slots.get(param_set_idx) {
            self.smoothing.prev_param_slot = *param_time_slot_border;
        }
    }

    /// Updates `self` with the provided arguments.
    ///
    /// # Parameters
    ///
    /// - `freq_res`: Specifies the number of parameter bands to process.
    /// - `num_output_channels`: Translates to the number of M2 rows (up to
    ///   `sac_dec::MAX_NUM_SAC_OUTPUT_CHANNELS`).
    /// - `num_ott_bands_ipd`: Number of Inter Channel Phase Difference bands.
    /// - `bs_phase_coding`: Is phase coding applied.
    pub(super) fn decode_header(
        &mut self,
        freq_res: FreqResolution,
        num_output_channels: u8,
        num_ott_bands_ipd: u8,
        bs_phase_coding: PhaseCoding,
    ) {
        self.num_parameter_bands = u8::from(freq_res);
        self.num_m2_rows = num_output_channels;
        self.num_ott_bands_ipd = num_ott_bands_ipd;
        self.phase_coding = bs_phase_coding;
    }

    /// Dequantize `IPD`, `CLD` and `ICC` spatial parameters.
    ///
    /// # Parameters
    ///
    /// - `ipd_idx`: Must be in range \[0..`N_IPD`\].
    /// - `cld_idx`: Must be in range \[0..`N_CLD`\].
    /// - `icc_idx`: Must be in range \[0..`N_ICC`\].
    /// # Return
    /// - `f32` calculated phase angle based on input parameters.
    fn dequant_ipd_cld_icc_split_angle(ipd_idx: usize, cld_idx: usize, icc_idx: usize) -> f32 {
        // iid_lin = sqrt(cld);
        let iid_lin = SQRT_CLD_M[cld_idx];

        // iid_lin2 = cld;
        let iid_lin21 = (iid_lin * iid_lin) + 1.0;

        let icc = DEQUANT_ICC[icc_idx];

        let cos_ipd = cos_ipd(ipd_idx);
        let sin_ipd = sin_ipd(ipd_idx);

        let temp1 = iid_lin * icc * 2.0;
        let temp1c = temp1 * cos_ipd;

        let ratio = (iid_lin21 + temp1c) / (iid_lin21 + temp1) + f32::EPSILON;
        let w2 = f32::sqrt(f32::sqrt(ratio));
        let w1 = 2.0 - w2;

        f32::atan2(w2 * sin_ipd, w1 * iid_lin + w2 * cos_ipd)
    }

    /// Initializes `M2Data` struct members.
    ///
    /// # Parameters
    ///
    /// - `num_output_channels`: Number of output channels.
    /// - `do_init_m2_states`: Flag for re-init of `M2` and `M2_prev` state buffers.
    /// - `do_init_smoothing_states`: Flag for re-init of smoothing parameters.
    /// - `do_overwrite_m2_states`: Controls the replacement of the previous M2 parameters with new
    ///   ones.
    /// # Return
    /// - `SacDecoderError`
    pub(super) fn init(
        &mut self,
        num_output_channels: usize,
        do_init_m2_states: bool,
        do_init_smoothing_states: bool,
        do_overwrite_m2_states: bool,
    ) {
        self.overwrite_m2_prev = do_overwrite_m2_states;
        self.num_m2_rows = num_output_channels as u8;

        if do_init_m2_states {
            self.m2_real[0..num_output_channels]
                .fill([[0.0_f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]);
            self.m2_real_prev[0..num_output_channels]
                .fill([[0.0_f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]);
        }

        if do_init_smoothing_states {
            self.init_parameter_smoothing();
        }
    }

    /// Initializes smoothing parameters.
    pub(super) fn init_parameter_smoothing(&mut self) {
        self.smoothing = Smoothing::default();
    }

    /// Gets a reference to a specific row of `M2` matrix, depending on the provided arguments.
    ///
    /// # Parameters
    ///
    /// - `cplx_part`: Selects whether real or imaginary part of M2's row is returned.
    /// - `state`: Selects whether current or previous row of `M2` is returned.
    /// - `row`: Selects which row of `M2` is returned.
    ///
    /// # Return
    /// - An optional reference to a specific row from `M2` matrix.
    pub(super) fn m2_row(
        &self,
        cplx_part: ComplexPart,
        state: State,
        row: usize,
    ) -> Option<&[[f32; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]> {
        match (row).cmp(&MAX_NUM_SAC_OUTPUT_CHANNELS) {
            Ordering::Less => match cplx_part {
                ComplexPart::Real => match state {
                    State::Curr => Some(&self.m2_real[row]),
                    State::Prev => Some(&self.m2_real_prev[row]),
                },
                ComplexPart::Imag => match state {
                    State::Curr => Some(&self.m2_imag[row]),
                    State::Prev => Some(&self.m2_imag_prev[row]),
                },
            },
            Ordering::Equal => None,
            Ordering::Greater => None,
        }
    }

    /// Creates a new `M2Data` structure.
    pub(super) fn new() -> Self {
        Default::default()
    }

    /// Gets number of Inter Channel Phase Difference bands.
    pub(super) fn num_ott_bands_ipd(&self) -> u8 {
        self.num_ott_bands_ipd
    }

    /// Gets number of parameter bands. Maximum value is `MAX_PARAMETER_BANDS`.
    pub(super) fn num_parameter_bands(&self) -> u8 {
        self.num_parameter_bands
    }

    /// Gets number of rows of `M2` matrix, correspondent with number of output channels.
    /// Maximum value is `sac_dec::MAX_NUM_SAC_OUTPUT_CHANNELS`.
    pub(super) fn num_m2_rows(&self) -> u8 {
        self.num_m2_rows
    }

    /// Returns a mutable reference (setter) to `CLD` matrix struct member.
    /// Matrix contains a One-to-Two Box with `Channel Level Difference` indices.
    /// Indices must be in range: \[0..`N_CLD`\].
    pub(super) fn ott_cld_mut(&mut self) -> &mut [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS] {
        &mut self.ott_cld
    }

    /// Returns a mutable reference (setter) to `ICC` matrix struct member.
    /// Matrix contains a One-to-Two Box with `Inter Channel Correlation` indices.
    /// Indices must be in range: \[0..`N_ICC`\].
    pub(super) fn ott_icc_mut(&mut self) -> &mut [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS] {
        &mut self.ott_icc
    }

    /// Returns a mutable reference (setter) to `IPD` matrix struct member.
    /// Matrix contains a One-to-Two Box with `Inter Channel Phase Difference` indices.
    /// Indices must be in range: \[0..`N_IPD`\].
    pub(super) fn ott_ipd_mut(&mut self) -> &mut [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS] {
        &mut self.ott_ipd
    }

    /// Calculates `M2`'s upmix parameters, based on `CLD` and `ICC` parameters.
    ///
    /// # Parameters
    ///
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    fn param_2_umx_ps(&mut self, param_set_idx: usize) {
        let (m2_real_l, m2_real_r) = self.m2_real.split_at_mut(1);
        let (m2_real_ll, m2_real_lr) = m2_real_l[0].split_at_mut(1);
        let (m2_real_rl, m2_real_rr) = m2_real_r[0].split_at_mut(1);

        for (idx1, idx2, h11, h12, h21, h22) in izip!(
            self.ott_cld[param_set_idx].iter(),
            self.ott_icc[param_set_idx].iter(),
            m2_real_ll[0].iter_mut(),
            m2_real_lr[0].iter_mut(),
            m2_real_rl[0].iter_mut(),
            m2_real_rr[0].iter_mut(),
        )
        .take(usize::from(self.num_parameter_bands))
        {
            // Compute mixing variables.
            let cld_idx = usize::from(*idx1);
            let icc_idx = usize::from(*idx2);

            *h11 = H11_NC[cld_idx][icc_idx];
            *h12 = H12_NC[cld_idx][icc_idx];
            *h21 = H11_NC[(N_CLD - 1) - cld_idx][icc_idx];
            *h22 = -H12_NC[(N_CLD - 1) - cld_idx][icc_idx];
        }
    }

    /// Estimates overall phase difference parameters, based on `IPD`, `CLD` and `ICC` parameters.
    ///
    /// # Parameters
    ///
    /// - `are_ipd_params_avail`: Indicates whether the IPD parameters are available for the current
    ///   frame. I.e. `phase_mode`.
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    fn param_2_umx_ipd_opd(&mut self, are_ipd_params_avail: bool, param_set_idx: usize) {
        let num_parameter_bands = usize::from(self.num_parameter_bands);
        let num_ott_bands_ipd = if are_ipd_params_avail {
            usize::from(self.num_ott_bands_ipd)
        } else {
            0
        };

        for (i_cld, i_icc, i_ipd, phase_left, phase_right) in izip!(
            self.ott_cld[param_set_idx].iter(),
            self.ott_icc[param_set_idx].iter(),
            self.ott_ipd[param_set_idx].iter(),
            self.phase_left.iter_mut(),
            self.phase_right.iter_mut()
        )
        .take(num_ott_bands_ipd)
        {
            let cld_idx = usize::from(*i_cld);
            let icc_idx = usize::from(*i_icc);
            let ipd_idx = usize::from(*i_ipd);

            // Variable only necessary for checking against value of 0.0_f32.
            // let cld = DEQUANT_CLD[cld_idx];
            let ipd = DEQUANT_IPD[ipd_idx];

            // DEQUANT_CLD[15] == 0.0; DEQUANT_IPD[8] == PI.
            *phase_left = if cld_idx == 15 && ipd_idx == 8 {
                0.0_f32
            } else {
                Self::dequant_ipd_cld_icc_split_angle(ipd_idx, cld_idx, icc_idx)
            };
            *phase_right = *phase_left - ipd;

            // Wrap phase.
            *phase_left = wrap_phase(*phase_left);
            *phase_right = wrap_phase(*phase_right);
        }

        if num_ott_bands_ipd <= num_parameter_bands {
            self.phase_left[num_ott_bands_ipd..num_parameter_bands].fill(0.0);
            self.phase_right[num_ott_bands_ipd..num_parameter_bands].fill(0.0);
        }
    }

    /// Calculates `M2`'s upmix parameters, based on `CLD`, `ICC` and `IPD` parameters.
    ///
    /// # Parameters
    ///
    /// - `num_residual_bands`: Number of residual bands.
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    fn param_2_umx_prediction(&mut self, num_residual_bands: usize, param_set_idx: usize) {
        const MAX_WEIGHT: f32 = 1.2;
        const GAIN: f32 = 0.5 / MAX_WEIGHT;

        let num_ott_bands_ipd = usize::from(self.num_ott_bands_ipd);

        let (m2_real_l, m2_real_r) = self.m2_real.split_at_mut(1);
        let (m2_real_ll, m2_real_lr) = m2_real_l[0].split_at_mut(1);
        let (m2_real_rl, m2_real_rr) = m2_real_r[0].split_at_mut(1);

        let (m2_imag_l, m2_imag_r) = self.m2_imag.split_at_mut(1);
        let (m2_imag_ll, _m2_imag_lr) = m2_imag_l[0].split_at_mut(1);
        let (m2_imag_rl, _m2_imag_rr) = m2_imag_r[0].split_at_mut(1);

        for (band, (i_cld, i_icc, i_ipd, re11, re12, re21, re22, im11, im21)) in izip!(
            self.ott_cld[param_set_idx].iter(),
            self.ott_icc[param_set_idx].iter(),
            self.ott_ipd[param_set_idx].iter(),
            m2_real_ll[0].iter_mut(),
            m2_real_lr[0].iter_mut(),
            m2_real_rl[0].iter_mut(),
            m2_real_rr[0].iter_mut(),
            m2_imag_ll[0].iter_mut(),
            m2_imag_rl[0].iter_mut(),
        )
        .enumerate()
        .take(usize::from(self.num_parameter_bands))
        {
            let cld_idx = usize::from(*i_cld);
            let icc_idx = usize::from(*i_icc);
            let ipd_idx = usize::from(*i_ipd);

            // DEQUANT_CLD[15] == 0.0; DEQUANT_ICC[0] = 1.0; DEQUANT_IPD[8] == PI.
            if band < num_ott_bands_ipd && cld_idx == 15 && icc_idx == 0 && ipd_idx == 8 {
                *re11 = GAIN;

                if band < num_residual_bands {
                    *re21 = GAIN;
                    *re12 = GAIN;
                    *re22 = -GAIN;
                } else {
                    *re21 = -GAIN;
                    *re12 = 0.0;
                    *re22 = 0.0;
                }
                *im11 = 0.0;
                *im21 = 0.0;
            } else {
                // iid_lin = sqrt(cld);
                let iid_lin = SQRT_CLD_M[cld_idx];
                // iid_lin2 = cld;
                let iid_lin2 = iid_lin * iid_lin;
                let iid_lin21 = iid_lin2 + 1.0;

                let icc = DEQUANT_ICC[icc_idx];
                let iid_icc2 = iid_lin * icc * 2.0;

                let cos_ipd = cos_ipd(if band < num_ott_bands_ipd { ipd_idx } else { 0 });
                let sin_ipd: f32 = sin_ipd(if band < num_ott_bands_ipd { ipd_idx } else { 0 });

                let temp = iid_lin21 + iid_icc2 * cos_ipd;
                let alpha_re = (1.0 - iid_lin2) / temp;
                let mut weight = (iid_lin21 / temp).sqrt();
                weight = 0.5 / weight.min(MAX_WEIGHT);

                *re11 = weight - alpha_re * weight;
                *re21 = weight + alpha_re * weight;

                if self.phase_coding == PhaseCoding::IpdPrediction {
                    let alpha_im = -iid_icc2 * sin_ipd / temp;
                    *im11 = -alpha_im * weight;
                    *im21 = alpha_im * weight;
                } else {
                    *im11 = 0.0;
                    *im21 = 0.0;
                }

                if band < num_residual_bands {
                    *re12 = weight;
                    *re22 = -weight;
                } else {
                    let beta = 2.0 * iid_lin * (1.0 - icc * icc).sqrt() * weight / temp;
                    *re12 = beta;
                    *re22 = -beta;
                }
            }
        }
    }

    pub(super) fn phase_coding(&self) -> PhaseCoding {
        self.phase_coding
    }

    /// Returns phase value for selected arguments.
    ///
    /// # Parameters
    ///
    /// - `side`: Selects either the phase for the left or right channel is returned.
    /// - `state`: Selects either current or previous phase is returned.
    /// - `param_band`: Selects the correspondent parameter band for the phase to be returned.
    ///
    /// # Return
    /// - Phase value for selected parameter band.
    pub(super) fn phase_for_param_band(&self, side: Side, state: State, param_band: usize) -> f32 {
        if param_band < MAX_PARAMETER_BANDS {
            match side {
                Side::Left => match state {
                    State::Curr => self.phase_left[param_band],
                    State::Prev => self.phase_prev_left[param_band],
                },
                Side::Right => match state {
                    State::Curr => self.phase_right[param_band],
                    State::Prev => self.phase_prev_right[param_band],
                },
            }
        } else {
            0.0
        }
    }

    /// Reads smoothing data from `Bitstream` and stores in 'self'.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - 'freq_res': Selected frequency resolution (defines the number of parameter bands).
    /// - 'is_bs_high_rate_mode': Indicates bit rate operation mode of MPS212.
    /// - 'num_parameter_sets': Defines the number of parameter sets in a frame.
    /// - 'opd_smoothing_mode': Is smoothing is applied to OPD parameters.
    /// - 'do_extend_frame': Copy smoothing params from last parameter set for "extended frame"
    ///   mode.
    /// - 'is_eld': is Extended Low Delay
    #[expect(clippy::too_many_arguments)]
    pub(super) fn read_smg_data(
        &mut self,
        bs: &mut Bitstream,
        freq_res: FreqResolution,
        is_bs_high_rate_mode: bool,
        num_parameter_sets: usize,
        opd_smoothing_mode: OpdSmoothingMode,
        do_extend_frame: bool,
        is_eld: bool,
    ) {
        self.smoothing.read_smg_data(
            bs,
            freq_res,
            is_bs_high_rate_mode,
            num_parameter_sets,
            opd_smoothing_mode,
            do_extend_frame,
            is_eld,
        );
    }

    /// Applies smoothing on `M2`'s mix data.
    ///
    /// # Parameters
    ///
    ///  - `num_residual_bands`: Number of residual bands.
    ///  - `param_slot`: Specific parameter time slot for which the parameter is defined.
    ///  - `num_time_slots`: Number of time slots.
    ///  - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    fn smooth_m2(
        &mut self,
        num_residual_bands: usize,
        param_slot: u8,
        num_time_slots: u8,
        param_set_idx: usize,
    ) {
        let delta = self
            .smoothing
            .calc_filter_coeff(param_set_idx, param_slot, num_time_slots);
        let one_minus_delta = 1.0 - delta;
        let is_phase_coding = self.phase_coding == PhaseCoding::IpdPrediction;
        let num_m2_rows = usize::from(self.num_m2_rows);
        let num_parameter_bands = usize::from(self.num_parameter_bands);

        for (row_m2_real_prev, row_m2_real) in
            izip!(self.m2_real_prev.iter(), self.m2_real.iter_mut()).take(num_m2_rows)
        {
            for (col_m2_real_prev, col_m2_real) in
                izip!(row_m2_real_prev.iter(), row_m2_real.iter_mut()).take(MAX_V_CHANNELS)
            {
                for (is_smgdata, band_m2_real_prev, band_m2_real) in izip!(
                    self.smoothing.smg_data[param_set_idx][num_residual_bands..].iter(),
                    col_m2_real_prev[num_residual_bands..].iter(),
                    col_m2_real[num_residual_bands..].iter_mut()
                )
                .take(num_parameter_bands - num_residual_bands)
                {
                    if *is_smgdata {
                        *band_m2_real *= delta;
                        *band_m2_real += one_minus_delta * *band_m2_real_prev;
                    }
                }
            }
        }

        if is_phase_coding {
            for (row_m2_imag_prev, row_m2_imag) in
                izip!(self.m2_imag_prev.iter(), self.m2_imag.iter_mut()).take(num_m2_rows)
            {
                for (col_m2_imag_prev, col_m2_imag) in
                    izip!(row_m2_imag_prev.iter(), row_m2_imag.iter_mut()).take(MAX_V_CHANNELS)
                {
                    for (is_smgdata, band_m2_imag_prev, band_m2_imag) in izip!(
                        self.smoothing.smg_data[param_set_idx][num_residual_bands..].iter(),
                        col_m2_imag_prev[num_residual_bands..].iter(),
                        col_m2_imag[num_residual_bands..].iter_mut()
                    )
                    .take(num_parameter_bands - num_residual_bands)
                    {
                        if *is_smgdata {
                            *band_m2_imag *= delta;
                            *band_m2_imag += one_minus_delta * *band_m2_imag_prev;
                        }
                    }
                }
            }
        }
    }
}

impl Default for M2Data {
    fn default() -> Self {
        M2Data {
            num_parameter_bands: 0,
            num_m2_rows: 0,
            overwrite_m2_prev: false,

            ott_cld: [[0; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
            ott_icc: [[0; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
            ott_ipd: [[0; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],

            m2_real: [[[0.0; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
            m2_imag: [[[0.0; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS]; MAX_NUM_SAC_OUTPUT_CHANNELS],
            m2_real_prev: [[[0.0; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS];
                MAX_NUM_SAC_OUTPUT_CHANNELS],
            m2_imag_prev: [[[0.0; MAX_PARAMETER_BANDS]; MAX_V_CHANNELS];
                MAX_NUM_SAC_OUTPUT_CHANNELS],

            phase_coding: PhaseCoding::NoIpd,
            num_ott_bands_ipd: 0,
            phase_left: [0.0; MAX_PARAMETER_BANDS],
            phase_right: [0.0; MAX_PARAMETER_BANDS],
            phase_prev_left: [0.0; MAX_PARAMETER_BANDS],
            phase_prev_right: [0.0; MAX_PARAMETER_BANDS],

            smoothing: Smoothing::new(),
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
struct Smoothing {
    /// Previous specific parameter time slot for which the parameter is defined.
    prev_param_slot: u8,
    /// Smoothing time constant from the previous parameter set.
    prev_smg_time: u16,
    /// Smoothing data flags from the previous parameter set.
    prev_smg_data: [bool; MAX_PARAMETER_BANDS],

    // Values depend on `opd_smoothing_mode`.
    /// Previous Overall Phase Difference parameters for left channel.
    opd_left_state: [f32; MAX_PARAMETER_BANDS],
    /// Previous Overall Phase Difference parameters for right channel.
    opd_right_state: [f32; MAX_PARAMETER_BANDS],

    /// Contains smoothing time constants to be applied for the
    /// individual parameter sets (see `SMG_TIME_TABLE[]`).
    smg_time: [u16; MAX_PARAMETER_SETS],
    /// Contains information about the temporal smoothing to be applied
    /// to the dequantized spatial parameters:
    /// - 'false': no smoothing
    /// - 'true': smoothing according to time constant (`smg_time`).
    smg_data: [[bool; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],

    /// It indicates whether smoothing is applied to Overall Phase Differences (OPD) parameters.
    opd_smoothing_mode: OpdSmoothingMode,
}

impl Default for Smoothing {
    fn default() -> Self {
        Smoothing {
            prev_param_slot: 0,
            prev_smg_time: SMG_TIME_TABLE[2],
            prev_smg_data: [false; MAX_PARAMETER_BANDS],
            opd_left_state: [0_f32; MAX_PARAMETER_BANDS],
            opd_right_state: [0_f32; MAX_PARAMETER_BANDS],
            smg_time: [SMG_TIME_TABLE[2]; MAX_PARAMETER_SETS],
            smg_data: [[false; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
            opd_smoothing_mode: Default::default(),
        }
    }
}

impl Smoothing {
    /// Calculates smoothing coefficient, based on smoothing time constants.
    /// # Parameters
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    /// - `param_slot`: Specific parameter time slot for which the parameter is defined.
    /// - `num_time_slots`: Number of time slots.
    /// # Return
    /// - Delta coefficient used for smoothing.
    fn calc_filter_coeff(&self, param_set_idx: usize, param_slot: u8, num_time_slots: u8) -> f32 {
        let mut diff_slots = i16::from(param_slot) - i16::from(self.prev_param_slot);
        if diff_slots <= 0 {
            diff_slots += i16::from(num_time_slots);
        }

        // Delta
        f32::from(diff_slots) / f32::from(self.smg_time[param_set_idx])
    }

    /// Creates `Smoothing` instance.
    fn new() -> Self {
        Default::default()
    }

    /// Reads smoothing data from `Bitstream` and stores in 'self'.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - 'freq_res': Selected frequency resolution (defines the number of parameter bands).
    /// - 'is_bs_high_rate_mode': Indicates bit rate operation mode of MPS212.
    /// - 'num_parameter_sets': Defines the number of parameter sets in a frame.
    /// - 'opd_smoothing_mode': Is smoothing is applied to OPD parameters.
    /// - 'do_extend_frame': Copy smoothing params from last parameter set for "extended frame"
    ///   mode.
    /// - 'is_eld': is Extended Low Delay
    #[expect(clippy::too_many_arguments)]
    fn read_smg_data(
        &mut self,
        bs: &mut Bitstream,
        freq_res: FreqResolution,
        is_bs_high_rate_mode: bool,
        num_parameter_sets: usize,
        opd_smoothing_mode: OpdSmoothingMode,
        do_extend_frame: bool,
        is_eld: bool,
    ) {
        self.opd_smoothing_mode = opd_smoothing_mode;
        let num_parameter_bands = usize::from(freq_res);

        if !is_eld && !is_bs_high_rate_mode {
            for (smg_time, smg_data) in
                izip!(self.smg_time.iter_mut(), self.smg_data.iter_mut()).take(num_parameter_sets)
            {
                *smg_time = SMG_TIME_TABLE[2];
                smg_data[..num_parameter_bands].fill(false);
            }
        } else {
            let mut prev_smg_time = self.smg_time[0];
            let mut prev_smg_data = self.smg_data[0];

            for (ps, (smg_time, smg_data)) in
                izip!(self.smg_time.iter_mut(), self.smg_data.iter_mut())
                    .enumerate()
                    .take(num_parameter_sets)
            {
                match BsSmoothMode::from(bs.read(2)) {
                    BsSmoothMode::Off => {
                        *smg_time = SMG_TIME_TABLE[2];
                        prev_smg_time = *smg_time;

                        smg_data[..num_parameter_bands].fill(false);
                        prev_smg_data[..num_parameter_bands]
                            .copy_from_slice(&smg_data[..num_parameter_bands]);
                    }
                    BsSmoothMode::KeepPrevUnchanged => {
                        if ps > 0 {
                            *smg_time = prev_smg_time;
                            smg_data[..num_parameter_bands]
                                .copy_from_slice(&prev_smg_data[..num_parameter_bands]);
                        } else {
                            *smg_time = self.prev_smg_time;
                            prev_smg_time = *smg_time;

                            smg_data[..num_parameter_bands]
                                .copy_from_slice(&self.prev_smg_data[..num_parameter_bands]);
                            prev_smg_data[..num_parameter_bands]
                                .copy_from_slice(&smg_data[..num_parameter_bands]);
                        }
                    }
                    BsSmoothMode::On => {
                        let bs_smooth_time = bs.read(2) as usize;
                        *smg_time = SMG_TIME_TABLE[bs_smooth_time];
                        prev_smg_time = *smg_time;

                        smg_data[..num_parameter_bands].fill(true);
                        prev_smg_data[..num_parameter_bands]
                            .copy_from_slice(&smg_data[..num_parameter_bands]);
                    }
                    BsSmoothMode::ReadCodedFlags => {
                        let bs_smooth_time = bs.read(2) as usize;
                        *smg_time = SMG_TIME_TABLE[bs_smooth_time];
                        prev_smg_time = *smg_time;

                        let bs_freq_res_stride_smg = bs.read(2) as usize;
                        let pb_stride = PB_STRIDE_TABLE[bs_freq_res_stride_smg];
                        let data_bands = usize::from(((u8::from(freq_res) - 1) / pb_stride) + 1);
                        let a_group_to_band =
                            sac_map::create_mapping(u8::from(freq_res), PbStride::from(pb_stride));

                        for b in a_group_to_band.windows(2).take(data_bands) {
                            let tmp_smg_data = bs.read_bit() != 0;

                            let start = b[0];
                            let stop = b[1];

                            smg_data[start..stop].fill(tmp_smg_data);
                        }
                        prev_smg_data[..num_parameter_bands]
                            .copy_from_slice(&smg_data[..num_parameter_bands]);
                    }
                }
            }
        }

        self.prev_smg_time = self.smg_time[num_parameter_sets - 1];
        self.prev_smg_data[..num_parameter_bands]
            .copy_from_slice(&self.smg_data[num_parameter_sets - 1][..num_parameter_bands]);

        if do_extend_frame {
            self.smg_time[num_parameter_sets] = self.smg_time[num_parameter_sets - 1];

            let prev_smg_data = self.smg_data[num_parameter_sets - 1];
            self.smg_data[num_parameter_sets][..num_parameter_bands]
                .copy_from_slice(&prev_smg_data[..num_parameter_bands]);
        }
    }

    /// Smooths Overall Phase Difference parameters (left, right) for the provided `param_set_idx`.
    ///
    /// # Parameters
    ///
    /// - `quant_coarse`: Full/Half quantization applied.
    /// - `param_time_slots`: The specific parameter time slots (i.e. time slot borders) for which
    ///   the parameters are defined.
    /// - `param_set_idx`: The specific parameter set (group) for which the parameter is defined.
    /// - `num_parameter_bands`: Number of parameter bands.
    /// - `curr_opd_left`: Current Overall Phase Difference value (left channel).
    /// - `curr_opd_right`: Current Overall Phase Difference value (right channel).
    fn smooth_opd(
        &mut self,
        quant_coarse: BsQuantCoarseXxx,
        param_time_slots: &[u8],
        param_set_idx: usize,
        num_parameter_bands: usize,
        curr_opd_left: &mut [f32; MAX_PARAMETER_BANDS],
        curr_opd_right: &mut [f32; MAX_PARAMETER_BANDS],
    ) {
        if self.opd_smoothing_mode == OpdSmoothingMode::Disabled {
            self.opd_left_state[..num_parameter_bands]
                .copy_from_slice(&curr_opd_left[..num_parameter_bands]);
            self.opd_right_state[..num_parameter_bands]
                .copy_from_slice(&curr_opd_right[..num_parameter_bands]);
        } else {
            // Number of time slots beloging to a parameter set.
            let d_param_slots = if param_set_idx == 0 {
                param_time_slots[param_set_idx] + 1
            } else {
                param_time_slots[param_set_idx] - param_time_slots[param_set_idx - 1]
            };
            let delta = f32::from(d_param_slots) / 128.0_f32;
            let one_minus_delta = 1.0 - delta;

            for (opd_left, opd_right, prev_opd_left, prev_opd_right) in izip!(
                curr_opd_left.iter_mut(),
                curr_opd_right.iter_mut(),
                self.opd_left_state.iter_mut(),
                self.opd_right_state.iter_mut()
            )
            .take(num_parameter_bands)
            {
                let mut tmp_l = *opd_left;
                let mut tmp_r = *opd_right;

                while tmp_l > *prev_opd_left + PI {
                    tmp_l -= PI_X_2;
                }
                while tmp_l < *prev_opd_left - PI {
                    tmp_l += PI_X_2;
                }
                while tmp_r > *prev_opd_right + PI {
                    tmp_r -= PI_X_2;
                }
                while tmp_r < *prev_opd_right - PI {
                    tmp_r += PI_X_2;
                }

                *prev_opd_left = delta * tmp_l + one_minus_delta * *prev_opd_left;
                *prev_opd_right = delta * tmp_r + one_minus_delta * *prev_opd_right;

                let mut tmp = (tmp_l - tmp_r) - (*prev_opd_left - *prev_opd_right);

                while tmp > PI {
                    tmp -= PI_X_2;
                }
                while tmp < -PI {
                    tmp += PI_X_2;
                }

                let quant_coarse_val = if quant_coarse != BsQuantCoarseXxx::FullRes {
                    50.0
                } else {
                    25.0
                };
                if tmp.abs() > quant_coarse_val * PI / 180.0_f32 {
                    *prev_opd_left = tmp_l;
                    *prev_opd_right = tmp_r;
                }

                while *prev_opd_left > PI_X_2 {
                    *prev_opd_left -= PI_X_2;
                }
                while *prev_opd_left < 0.0 {
                    *prev_opd_left += PI_X_2;
                }
                while *prev_opd_right > PI_X_2 {
                    *prev_opd_right -= PI_X_2;
                }
                while *prev_opd_right < 0.0 {
                    *prev_opd_right += PI_X_2;
                }

                *opd_left = *prev_opd_left;
                *opd_right = *prev_opd_right;
            }
        }
    }
}

/// Returns an 'f32' cosine value from `SIN_IPD_TAB`.
/// Offset `i+4` returns the cosine (90 degree phase offset).
///
/// # Parameters
///
/// - `i`: index for accessing the table \[0..=`N_IPD`\].
#[inline(always)]
fn cos_ipd(i: usize) -> f32 {
    SIN_IPD_TAB[(i + 4) & 0xF]
}

/// Returns an 'f32' sine value from `SIN_IPD_TAB`.
///
/// # Parameters
///
/// - `i`: index for accessing the table \[0..=`N_IPD`\].
#[inline(always)]
fn sin_ipd(i: usize) -> f32 {
    SIN_IPD_TAB[i]
}

/// Wrapps `phase` by `2xPI`.
///
/// # Parameters
///
/// - `phase`: Phase value
///
/// # Return
///
/// - Phase value wrapped by `2xPI`.
fn wrap_phase(phase: f32) -> f32 {
    let mut wrapped_phase = phase;
    while wrapped_phase < 0.0 {
        wrapped_phase += PI_X_2;
    }
    while wrapped_phase >= PI_X_2 {
        wrapped_phase -= PI_X_2;
    }

    debug_assert!((0.0_f32..PI_X_2).contains(&wrapped_phase));

    wrapped_phase
}

#[cfg(test)]
mod tests {
    use super::*;

    // Tests that ott_cld_mut() provides a proper setter.
    #[test]
    fn t_ott_cld_mut() {
        let mut m2_data = M2Data::new();
        m2_data.init(2, true, true, false);

        let new_ott_cld = m2_data.ott_cld_mut();

        for (ps, cld) in new_ott_cld.iter_mut().enumerate() {
            cld.fill(ps as u8)
        }

        for (ps, cld) in m2_data.ott_cld.iter().enumerate() {
            let param_set_idx = [ps as u8; MAX_PARAMETER_BANDS];
            debug_assert!(param_set_idx == *cld);
        }
    }

    #[test]
    fn size_of_structs() {
        const SIZE_SMOOTHING_C_STRUCT: usize = 528;
        const SIZE_M2DATA_C_STRUCT: usize = 3532;
        assert!(std::mem::size_of::<Smoothing>() == SIZE_SMOOTHING_C_STRUCT);
        assert!(std::mem::size_of::<M2Data>() == SIZE_M2DATA_C_STRUCT);
    }
}
