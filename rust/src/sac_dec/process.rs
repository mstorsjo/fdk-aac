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
//! SAC processing

use super::common::FreqResolution;
use crate::common::decorrelator::DecorrDec;
use crate::common::enums::StereoCfgIndex;
use crate::common::hybrid::{HybridAnalysis, HybridSynthesis};
use crate::common::qmf::QmfFilterBank;
use crate::common::qmf_domain::QmfDomain;
mod tables;
use super::calc_m2::ComplexPart;
use super::common::PhaseCoding;
use super::constants::*;
use super::process::tables::{
    KERNELS_10_TO_71, KERNELS_12_TO_64, KERNELS_14_TO_71, KERNELS_15_TO_64, KERNELS_20_TO_71,
    KERNELS_23_TO_64, KERNELS_28_TO_71, KERNELS_4_TO_64, KERNELS_4_TO_71, KERNELS_5_TO_64,
    KERNELS_5_TO_71, KERNELS_7_TO_64, KERNELS_7_TO_71, KERNELS_9_TO_64,
};
use super::tsd::TsdData;
use super::{
    calc_m2::{M2Data, Side, State},
    error_codes::SacDecoderError,
};
use itertools::izip;
use num_complex::Complex;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
/// QMF (Quadrature Mirror Filter) slot. Used to decompose a signal into different frequency bands,
/// allowing for efficient analysis, synthesis, and compression.
pub(super) struct QmfSlot {
    pub(super) bands_real: [f32; MAX_NUM_QMF_BANDS],
    pub(super) bands_imag: [f32; MAX_NUM_QMF_BANDS],
}

impl Default for QmfSlot {
    /// Returns default instance of `QmfSlot`.
    fn default() -> Self {
        Self {
            bands_real: [0.0; MAX_NUM_QMF_BANDS],
            bands_imag: [0.0; MAX_NUM_QMF_BANDS],
        }
    }
}

impl QmfSlot {
    /// Returns new instance of `QmfSlot`.
    pub fn new() -> Self {
        QmfSlot::default()
    }

    /// Calculates qmf data on downmix input time data. Delay lines will be applied if necessaray.
    ///
    /// # Parameters
    ///
    /// - `filter_bank`: Reference to `QmfFilterBank` structure.
    /// - `in_data`: Reference to downmix channel time data as input.
    /// - `qmf_slot`: Reference to downmix channel `QmfSlot` output data.
    /// - `gain`: Gain consisting of pre gain and clipping protection gain.
    /// - `qmf_bands`: Number of qmf bands.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn analysis(
        filter_bank: &mut QmfFilterBank,
        in_data: &[f32],
        gain: f32,
        qmf_bands: u8,
    ) -> QmfSlot {
        let mut qmf_slot = QmfSlot::new();
        let num_channels = filter_bank.num_subbands();
        filter_bank.analysis_filtering_slot(
            &mut qmf_slot.bands_real,
            &mut qmf_slot.bands_imag,
            in_data,
        );

        if qmf_bands > num_channels {
            qmf_slot.bands_real[usize::from(num_channels)..usize::from(qmf_bands)].fill(0.0);
            qmf_slot.bands_imag[usize::from(num_channels)..usize::from(qmf_bands)].fill(0.0);
        }

        if gain != 1.0 {
            let min = usize::from(num_channels.min(qmf_bands));
            for (real, imag) in izip!(
                qmf_slot.bands_real.iter_mut(),
                qmf_slot.bands_imag.iter_mut()
            )
            .take(min)
            {
                *real *= gain;
                *imag *= gain;
            }
        }

        qmf_slot
    }

    /// Feeds spatial decoder with external QMF data.
    ///
    /// # Parameters
    ///
    /// - `qmf_domain`: Refernce to `QmfDdomain` structure.
    /// - `qmf_slot`: Reference to downmix channel `QmfSlot` output data.
    /// - `gain`: Gain consisting of pre gain and clipping protection gain.
    /// - `ts`: Signal's time slot in input buffer to process.
    /// - `qmf_bands`: Number of QMF bands.
    /// - `bShareDelayWithSBR`: Flag indicates if delay is shared with SBR.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn feed(
        qmf_domain: &mut QmfDomain,
        gain: f32,
        ts: u8,
        qmf_bands: u8,
        share_delay_with_sbr: bool,
    ) -> QmfSlot {
        // Apply gain to the complex QMF data in input channel 0 and
        // copy the result to the address of qmf_real and qmf_imag.
        let mut qmf_slot = QmfSlot::new();

        if share_delay_with_sbr {
            qmf_domain.get_slot(
                gain,
                ts + HYBRID_FILTER_DELAY,
                0,
                MAX_QMF_BANDS_TO_HYBRID,
                &mut qmf_slot.bands_real,
                &mut qmf_slot.bands_imag,
            );

            qmf_domain.get_slot(
                gain,
                ts,
                MAX_QMF_BANDS_TO_HYBRID,
                qmf_bands,
                &mut qmf_slot.bands_real,
                &mut qmf_slot.bands_imag,
            );
        } else {
            qmf_domain.get_slot(
                gain,
                ts,
                0,
                qmf_bands,
                &mut qmf_slot.bands_real,
                &mut qmf_slot.bands_imag,
            );
        }

        // Copy complex QMF data in input channel 0 to overlap buffer.
        if ts == qmf_domain.num_qmf_time_slots() - 1 {
            qmf_domain.save_overlap(0);
        }

        qmf_slot
    }
}

#[repr(C)]
/// Data used for decomposition and reconstruction of the signal.
#[derive(Debug)]
pub(super) struct HybData {
    /// Number of hybrid bands.
    pub hybrid_bands: usize,
    /// Parameter to hybrid sub-band mapping.
    param2hyb: [u8; MAX_PARAMETER_BANDS + 1],
    /// Kernels used for calculation of the hybrid filter outputs.
    kernels: &'static [u8],
    /// Kernels widths list. Each value in a list corresponds to one sub-band.
    kernels_width: [u8; MAX_PARAMETER_BANDS],
    /// Temporary buffer used during processing.
    temp_signal: [[Complex<f32>; MAX_HYBRID_BANDS]; MAX_V_CHANNELS],
    /// Complex hybrid filter input.
    pub hyb_input_cplx: [Complex<f32>; MAX_HYBRID_BANDS],
    /// Complex residual data.
    pub hyb_residual_cplx: Box<[Complex<f32>]>,
    /// Dry complex hybrid filter output.
    pub hyb_output_cplx_dry: [[Complex<f32>; MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS],
    /// Wet complex hybrid filter output.
    pub hyb_output_cplx_wet: [[Complex<f32>; MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS],
    /// List of HybridAnalysis objects used for signal decomposition.
    pub hybrid_analysis: Option<Vec<HybridAnalysis>>,
    /// List of HybridSynthesis object used for signal reconstruction. Maximum number of channels
    /// is 2.
    pub hybrid_synthesis: Box<[HybridSynthesis]>,
}

impl Default for HybData {
    /// Returns default `HybData`.
    fn default() -> Self {
        Self {
            hybrid_bands: Default::default(),
            param2hyb: Default::default(),
            kernels: Default::default(),
            kernels_width: Default::default(),
            temp_signal: [[Default::default(); MAX_HYBRID_BANDS]; MAX_V_CHANNELS],
            hyb_input_cplx: [Default::default(); MAX_HYBRID_BANDS],
            hyb_residual_cplx: Default::default(),
            hyb_output_cplx_dry: [[Default::default(); MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS],
            hyb_output_cplx_wet: [[Default::default(); MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS],
            hybrid_analysis: Default::default(),
            hybrid_synthesis: Default::default(),
        }
    }
}

impl HybData {
    /// Creates new instance of `HybData`.
    ///
    /// # Parameters
    ///
    /// - `stereo_config_index`: Index of stereo configuration.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub fn new(stereo_config_index: StereoCfgIndex) -> Self {
        let mut hyb_data = HybData::default();

        let hyb_residual_cplx: [Complex<f32>; MAX_HYBRID_BANDS] =
            [Complex { re: 0.0, im: 0.0 }; MAX_HYBRID_BANDS];
        hyb_data.hyb_residual_cplx = Box::new(hyb_residual_cplx);

        let hybrid_synthesis: [HybridSynthesis; MAX_OUTPUT_CHANNELS] =
            [HybridSynthesis::default(); MAX_OUTPUT_CHANNELS];
        hyb_data.hybrid_synthesis = Box::new(hybrid_synthesis);

        if hyb_data.hybrid_analysis.is_none() {
            let hybrid_analysis_len = if stereo_config_index > StereoCfgIndex::Mps212 {
                MAX_INPUT_CHANNELS + MAX_USAC_RESIDUAL_CHANNELS
            } else {
                MAX_INPUT_CHANNELS
            };

            let mut hyb_analysis_vec: Vec<HybridAnalysis> = Vec::new();

            for _i in 0..hybrid_analysis_len {
                let mut hyb_analysis = HybridAnalysis::default();
                hyb_analysis.allocate(true);
                hyb_analysis_vec.push(hyb_analysis);
            }

            hyb_data.hybrid_analysis = Some(hyb_analysis_vec);
        }

        hyb_data
    }

    /// Initialises Hybdata.
    ///
    /// #Parameters
    ///
    /// - `num_qmf_bands`: Number of QMF bands.
    /// - `do_reset_states`: Indicates whether the states buffers have to be cleared.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn init(&mut self, num_qmf_bands: u8, do_reset_states: bool) -> SacDecoderError {
        for hybrid_synthesis in self.hybrid_synthesis.as_mut() {
            hybrid_synthesis.init(usize::from(num_qmf_bands));
        }

        // USAC input signal.
        if let Some(hybrid_analysis_vec) = self.hybrid_analysis.as_deref_mut() {
            for hybrid_analysis in hybrid_analysis_vec {
                hybrid_analysis.init(num_qmf_bands, do_reset_states);
            }
        }

        SacDecoderError::Ok
    }

    /// Deinitialises Hybdata members.
    pub(super) fn deinit(&mut self) {
        self.hybrid_analysis = None;
        self.hybrid_synthesis = Default::default();
        self.hyb_residual_cplx = Default::default();
    }

    /// Performs decomposition of the signal into sub-bands using Hybrid Analysis Filterbank.
    ///
    /// # Parameters
    ///
    /// - `qmf_data`: Reference to `QmfSlot` data.
    /// - `data_residuals`: Reference to `QmfSlot` residual data.
    /// - `bShareDelayWithSBR`: Flag indicates if delay is shared with SBR.
    /// - `is_eld`: `true`, if isEld syntax is active, otherwise `false`.
    /// - `is_usac_residual`: `true`, if USAC residual coding is active, otherwise `false`.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn analysis(
        &mut self,
        qmf_data: &QmfSlot,
        data_residuals: &QmfSlot,
        share_delay_with_sbr: bool,
        is_eld: bool,
        is_usac_residual: bool,
    ) -> SacDecoderError {
        if is_eld {
            // No hybrid filtering. Just copy the QMF data.
            for (hyb, q_re, q_im) in izip!(
                self.hyb_input_cplx.iter_mut(),
                qmf_data.bands_real.iter(),
                qmf_data.bands_imag.iter()
            )
            .take(self.hybrid_bands)
            {
                hyb.re = *q_re;
                hyb.im = *q_im;
            }
        } else {
            let hybrid_analysis = self.hybrid_analysis.as_mut().unwrap();
            hybrid_analysis[USAC_INP_CH].hf_mode = if share_delay_with_sbr { 1 } else { 0 };
            hybrid_analysis[USAC_INP_CH].apply(
                &qmf_data.bands_real,
                &qmf_data.bands_imag,
                &mut self.hyb_input_cplx,
            );
        }

        if is_usac_residual {
            let hybrid_analysis = self.hybrid_analysis.as_mut().unwrap();
            hybrid_analysis[USAC_RES_CH].hf_mode = 0;
            hybrid_analysis[USAC_RES_CH].apply(
                &data_residuals.bands_real,
                &data_residuals.bands_imag,
                self.hyb_residual_cplx.as_mut(),
            );
        }

        SacDecoderError::Ok
    }

    /// Applies M1 parameters and creates wet signal.
    ///
    /// # Parameters
    ///
    /// - `tsd_data`: Reference to TSD (Transient Steering Decorrelator) data.
    /// - `decor`: Reference to decorelator instance.
    /// - `residual_bands`: Number of residuals bands.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn apply_m1_create_w_mode_212(
        &mut self,
        tsd_data: &mut TsdData,
        decor: &mut DecorrDec,
        residual_bands: u8,
    ) -> SacDecoderError {
        let start_hyb_bands = self.param2hyb[residual_bands as usize] as usize;
        // M1 does not do anything in 212 mode, so use simplified processing.
        let (sig_0, sig_1) = self.temp_signal.split_at_mut(1);
        let (tmp_sig_0, tmp_sig_1) = (&mut sig_0[0], &mut sig_1[0]);

        tmp_sig_0.copy_from_slice(&self.hyb_input_cplx);

        if tsd_data.is_tsd_enabled() {
            let mut decorr_in_tsd_cplx: [Complex<f32>; MAX_HYBRID_BANDS] =
                [Complex { re: 0.0, im: 0.0 }; MAX_HYBRID_BANDS];

            // Generate v_{x,nonTr} as input for allpass based decorrelator.
            tsd_data.generate_non_tr(self.hybrid_bands, tmp_sig_0, &mut decorr_in_tsd_cplx);

            // Decorrelate.
            if decor.apply(&decorr_in_tsd_cplx, tmp_sig_1, start_hyb_bands) != 0 {
                return SacDecoderError::NotOk;
            }

            // Generate v_{x,Tr}, apply transient decorrelator and add to allpass based
            // decorrelator output.
            tsd_data.apply(self.hybrid_bands, tmp_sig_0, tmp_sig_1);
        } else if decor.apply(tmp_sig_0, tmp_sig_1, start_hyb_bands) != 0 {
            return SacDecoderError::NotOk;
        }

        if residual_bands > 0 {
            let f_min = start_hyb_bands.min(self.hybrid_bands);

            tmp_sig_1[0..f_min].copy_from_slice(&self.hyb_residual_cplx[0..f_min]);
        }

        SacDecoderError::Ok
    }

    /// Applies M2 parameter for 212 mode with residual coding and phase coding.
    ///
    /// # Parameters
    ///
    /// - `m2_data`: Reference to M2 data.
    /// - `alpha`: Smoothing factor between current and previous parameter band. Rangeability
    ///   between 0.f and 1.f.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn apply_m2_mode_212_residuals_plus_phase_coding(
        &mut self,
        m2_data: &M2Data,
        alpha: f32,
    ) -> SacDecoderError {
        let pb_max = usize::from(self.kernels[self.hybrid_bands - 1] + 1);

        for row in 0..usize::from(m2_data.num_m2_rows()) {
            let m_real_0 = m2_data.m2_row(ComplexPart::Real, State::Curr, row).unwrap()[0];
            let m_imag_0 = m2_data.m2_row(ComplexPart::Imag, State::Curr, row).unwrap()[0];
            let m_real_1 = m2_data.m2_row(ComplexPart::Real, State::Curr, row).unwrap()[1];
            let m_real_prev_0 = m2_data.m2_row(ComplexPart::Real, State::Prev, row).unwrap()[0];
            let m_imag_prev_0 = m2_data.m2_row(ComplexPart::Imag, State::Prev, row).unwrap()[0];
            let m_real_prev_1 = m2_data.m2_row(ComplexPart::Real, State::Prev, row).unwrap()[1];

            let mut bnds = 0;
            let mut qs = 3;

            let mut pb = 0;
            while pb < 2 {
                let real_0 = interpolate_parameter(alpha, m_real_0[pb], m_real_prev_0[pb]);
                let imag_0 = -interpolate_parameter(alpha, m_imag_0[pb], m_imag_prev_0[pb]);
                let real_1 = interpolate_parameter(alpha, m_real_1[pb], m_real_prev_1[pb]);

                let mut m_cplx = Complex {
                    re: real_0,
                    im: imag_0,
                };

                for _i in 0..self.kernels_width[pb] {
                    let cplx = (self.temp_signal[0][bnds] * m_cplx)
                        + self.temp_signal[1][bnds].scale(real_1);
                    self.hyb_output_cplx_dry[row][bnds] = cplx;

                    if qs > 0 {
                        m_cplx.im = -m_cplx.im;
                        qs -= 1;
                    }
                    bnds += 1;
                }
                pb += 1;
            }

            while pb < pb_max {
                let real_0 = interpolate_parameter(alpha, m_real_0[pb], m_real_prev_0[pb]);
                let imag_0 = interpolate_parameter(alpha, m_imag_0[pb], m_imag_prev_0[pb]);
                let real_1 = interpolate_parameter(alpha, m_real_1[pb], m_real_prev_1[pb]);

                let m_cplx_0 = Complex {
                    re: real_0,
                    im: imag_0,
                };

                for _i in 0..self.kernels_width[pb] {
                    let cplx = (self.temp_signal[0][bnds] * m_cplx_0)
                        + self.temp_signal[1][bnds].scale(real_1);
                    self.hyb_output_cplx_dry[row][bnds] = cplx;

                    bnds += 1;
                }

                pb += 1;
            }
        }

        SacDecoderError::Ok
    }

    /// Applies M2 parameters.
    ///
    /// # Parameters
    ///
    /// - `m2_data`: Reference to M2 data.
    /// - `residual_bands`: Number of residuals bands.
    /// - `alpha`: Smoothing factor between current and previous parameter band. Rangeability
    ///   between 0.f and 1.f.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn apply_m2(
        &mut self,
        m2_data: &M2Data,
        residual_bands: u8,
        alpha: f32,
    ) -> SacDecoderError {
        let num_parameter_bands = m2_data.num_parameter_bands();
        let res_hyb_index = usize::from(self.param2hyb[usize::from(residual_bands)]);

        let mut kernel: [f32; MAX_HYBRID_BANDS] = [0.0; MAX_HYBRID_BANDS];

        let kernels_width = &self.kernels_width;

        // Clear memory.
        self.hyb_output_cplx_dry = [[Default::default(); MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS];
        self.hyb_output_cplx_wet = [[Default::default(); MAX_HYBRID_BANDS]; MAX_OUTPUT_CHANNELS];

        for row in 0..usize::from(m2_data.num_m2_rows()) {
            let m2_real = m2_data.m2_row(ComplexPart::Real, State::Curr, row).unwrap();
            let m2_imag = m2_data.m2_row(ComplexPart::Imag, State::Curr, row).unwrap();
            let m2_real_prev = m2_data.m2_row(ComplexPart::Real, State::Prev, row).unwrap();
            let m2_imag_prev = m2_data.m2_row(ComplexPart::Imag, State::Prev, row).unwrap();

            m2_param_to_kernel_mult(
                &mut kernel,
                &m2_real[0],
                &m2_real_prev[0],
                kernels_width,
                alpha,
                num_parameter_bands,
            );

            for (cplx_dry, tmp_sig, ker) in izip!(
                self.hyb_output_cplx_dry[row].iter_mut(),
                self.temp_signal[0].iter(),
                kernel.iter()
            )
            .take(self.hybrid_bands)
            {
                *cplx_dry += tmp_sig.scale(*ker);
            }

            if m2_data.phase_coding() == PhaseCoding::IpdPrediction {
                m2_param_to_kernel_mult(
                    &mut kernel,
                    &m2_imag[0],
                    &m2_imag_prev[0],
                    kernels_width,
                    alpha,
                    num_parameter_bands,
                );

                // Direct signals sign is -1 for qs = 0,2.
                self.hyb_output_cplx_dry[row][0].re += self.temp_signal[0][0].im * kernel[0];
                self.hyb_output_cplx_dry[row][0].im -= self.temp_signal[0][0].re * kernel[0];

                self.hyb_output_cplx_dry[row][2].re += self.temp_signal[0][2].im * kernel[2];
                self.hyb_output_cplx_dry[row][2].im -= self.temp_signal[0][2].re * kernel[2];

                // Direct signals sign is +1 for qs = 1,3,4,5,...,complexHybBands.
                self.hyb_output_cplx_dry[row][1].re -= self.temp_signal[0][1].im * kernel[1];
                self.hyb_output_cplx_dry[row][1].im += self.temp_signal[0][1].re * kernel[1];

                for (cplx_dry, tmp_sig, ker) in izip!(
                    self.hyb_output_cplx_dry[row].iter_mut(),
                    self.temp_signal[0].iter(),
                    kernel.iter()
                )
                .take(self.hybrid_bands)
                .skip(3)
                {
                    cplx_dry.re -= tmp_sig.im * *ker;
                    cplx_dry.im += tmp_sig.re * *ker;
                }
            }

            m2_param_to_kernel_mult(
                &mut kernel,
                &m2_real[1],
                &m2_real_prev[1],
                kernels_width,
                alpha,
                num_parameter_bands,
            );

            // Dry - Residual signals.
            for (cplx_dry, tmp_sig, ker) in izip!(
                self.hyb_output_cplx_dry[row].iter_mut(),
                self.temp_signal[1].iter(),
                kernel.iter()
            )
            .take(res_hyb_index)
            {
                cplx_dry.re += tmp_sig.re * *ker;
                cplx_dry.im += tmp_sig.im * *ker;
            }

            // Wet.
            if self.hybrid_bands > res_hyb_index {
                // Decor signals.
                for (cplx_wet, tmp_sig, ker) in izip!(
                    self.hyb_output_cplx_wet[row][res_hyb_index..self.hybrid_bands].iter_mut(),
                    self.temp_signal[1][res_hyb_index..self.hybrid_bands].iter(),
                    kernel[res_hyb_index..self.hybrid_bands].iter()
                ) {
                    cplx_wet.re += tmp_sig.re * *ker;
                    cplx_wet.im += tmp_sig.im * *ker;
                }
            }

            if m2_data.phase_coding() == PhaseCoding::IpdPrediction {
                m2_param_to_kernel_mult(
                    &mut kernel,
                    &m2_imag[1],
                    &m2_imag_prev[1],
                    kernels_width,
                    alpha,
                    num_parameter_bands,
                );

                let mut hyb_out_cplx = &mut self.hyb_output_cplx_dry[row];

                // Direct signals sign is -1 for qs = 0,2.
                // Direct signals sign is +1 for qs = 1,3...
                if res_hyb_index == 0 {
                    hyb_out_cplx = &mut self.hyb_output_cplx_wet[row];
                }
                hyb_out_cplx[0].re += self.temp_signal[1][0].im * kernel[0];
                hyb_out_cplx[0].im -= self.temp_signal[1][0].re * kernel[0];

                if res_hyb_index == 1 {
                    hyb_out_cplx = &mut self.hyb_output_cplx_wet[row];
                }
                hyb_out_cplx[1].re -= self.temp_signal[1][1].im * kernel[1];
                hyb_out_cplx[1].im += self.temp_signal[1][1].re * kernel[1];

                if res_hyb_index == 2 {
                    hyb_out_cplx = &mut self.hyb_output_cplx_wet[row];
                }
                hyb_out_cplx[2].re += self.temp_signal[1][2].im * kernel[2];
                hyb_out_cplx[2].re -= self.temp_signal[1][2].re * kernel[2];

                // Dry.
                if 3 < res_hyb_index {
                    // Residual signals.
                    for (cplx_dry, tmp_sig, ker) in izip!(
                        self.hyb_output_cplx_dry[row][3..res_hyb_index].iter_mut(),
                        self.temp_signal[1][3..res_hyb_index].iter(),
                        kernel[3..res_hyb_index].iter()
                    ) {
                        cplx_dry.re -= tmp_sig.im * *ker;
                        cplx_dry.im += tmp_sig.re * *ker;
                    }
                }

                // Wet.
                if 3 < res_hyb_index && res_hyb_index < self.hybrid_bands {
                    // Decor signals.
                    for (cplx_wet, tmp_sig, ker) in izip!(
                        self.hyb_output_cplx_wet[row][res_hyb_index..self.hybrid_bands].iter_mut(),
                        self.temp_signal[1][res_hyb_index..self.hybrid_bands].iter(),
                        kernel[res_hyb_index..self.hybrid_bands].iter()
                    ) {
                        cplx_wet.re -= tmp_sig.im * *ker;
                        cplx_wet.im += tmp_sig.re * *ker;
                    }
                }
            }
        }

        SacDecoderError::Ok
    }

    /// Applies phase.
    ///
    /// # Parameters
    ///
    /// - `m2_data`: Reference to M2 data.
    /// - `alpha`: Smoothing factor between current and previous parameter band. Rangeability
    ///   between 0.f and 1.f.
    pub(super) fn apply_phase(&mut self, m2_data: &M2Data, alpha: f32) {
        let mut ppb: [f32; MAX_PARAMETER_BANDS * 4] = [0.0; MAX_PARAMETER_BANDS * 4];

        for (pb, ppb_data) in ppb
            .chunks_exact_mut(4)
            .enumerate()
            .take(usize::from(m2_data.num_parameter_bands()))
        {
            let pl = interp_angle(
                m2_data.phase_for_param_band(Side::Left, State::Prev, pb),
                m2_data.phase_for_param_band(Side::Left, State::Curr, pb),
                alpha,
                std::f32::consts::TAU,
            );

            let pr = interp_angle(
                m2_data.phase_for_param_band(Side::Right, State::Prev, pb),
                m2_data.phase_for_param_band(Side::Right, State::Curr, pb),
                alpha,
                std::f32::consts::TAU,
            );

            ppb_data[0] = pl.cos();
            ppb_data[1] = pl.sin();
            ppb_data[2] = pr.cos();
            ppb_data[3] = pr.sin();
        }

        let (cplx_dry_0, cplx_dry_1) = self.hyb_output_cplx_dry.split_at_mut(1);
        let (dry_0, dry_1) = (&mut cplx_dry_0[0], &mut cplx_dry_1[0]);

        let mut idx = usize::from(self.kernels[0]) * 4;
        let mut fac_0 = Complex {
            re: ppb[idx],
            im: -ppb[1 + idx],
        };
        let mut fac_1 = Complex {
            re: ppb[2 + idx],
            im: -ppb[3 + idx],
        };
        dry_0[0] *= fac_0;
        dry_1[0] *= fac_1;

        idx = usize::from(self.kernels[1]) * 4;
        if ppb.len() > idx + 3 {
            fac_0.re = ppb[idx];
            fac_0.im = ppb[1 + idx];
            fac_1.re = ppb[2 + idx];
            fac_1.im = ppb[3 + idx];
            dry_0[1] *= fac_0;
            dry_1[1] *= fac_1;
        }

        idx = usize::from(self.kernels[2]) * 4;
        if ppb.len() > idx + 3 {
            fac_0.re = ppb[idx];
            fac_0.im = -ppb[1 + idx];
            fac_1.re = ppb[2 + idx];
            fac_1.im = -ppb[3 + idx];
            dry_0[2] *= fac_0;
            dry_1[2] *= fac_1;
        }

        for (out_0, out_1, ker_val) in izip!(
            dry_0[3..].iter_mut(),
            dry_1[3..].iter_mut(),
            self.kernels[3..].iter()
        )
        .take(self.hybrid_bands - 3)
        {
            let idx = usize::from(*ker_val) * 4;

            if ppb.len() > (idx + 3) {
                fac_0.re = ppb[idx];
                fac_0.im = ppb[1 + idx];
                fac_1.re = ppb[2 + idx];
                fac_1.im = ppb[3 + idx];

                *out_0 *= fac_0;
                *out_1 *= fac_1;
            }
        }
    }

    /// Mapping of parameter bands to hybrid bands.
    ///
    /// # Parameters
    ///
    /// - `qmf_bands`: Number of QMF bands.
    /// - `freq_res`: Frequency resolution.
    /// - `is_eld`: `true`, if eld syntax is active, otherwise `false`.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn decode_header(
        &mut self,
        qmf_bands: u8,
        freq_res: FreqResolution,
        is_eld: bool,
    ) -> SacDecoderError {
        if is_eld {
            self.hybrid_bands = qmf_bands as usize;
            match freq_res {
                FreqResolution::Res23 => self.kernels = &KERNELS_23_TO_64,
                FreqResolution::Res15 => self.kernels = &KERNELS_15_TO_64,
                FreqResolution::Res12 => self.kernels = &KERNELS_12_TO_64,
                FreqResolution::Res9 => self.kernels = &KERNELS_9_TO_64,
                FreqResolution::Res7 => self.kernels = &KERNELS_7_TO_64,
                FreqResolution::Res5 => self.kernels = &KERNELS_5_TO_64,
                FreqResolution::Res4 => self.kernels = &KERNELS_4_TO_64,
                _ => return SacDecoderError::InvalidParameterbands,
            };
        } else {
            self.hybrid_bands = hybrid_subbands(qmf_bands);
            match freq_res {
                FreqResolution::Res28 => self.kernels = &KERNELS_28_TO_71,
                FreqResolution::Res20 => self.kernels = &KERNELS_20_TO_71,
                FreqResolution::Res14 => self.kernels = &KERNELS_14_TO_71,
                FreqResolution::Res10 => self.kernels = &KERNELS_10_TO_71,
                FreqResolution::Res7 => self.kernels = &KERNELS_7_TO_71,
                FreqResolution::Res5 => self.kernels = &KERNELS_5_TO_71,
                FreqResolution::Res4 => self.kernels = &KERNELS_4_TO_71,
                _ => return SacDecoderError::InvalidParameterbands,
            }
        }

        // Clear memory.
        self.param2hyb = [0; MAX_PARAMETER_BANDS + 1];

        let mut i = 0;
        while i < self.hybrid_bands {
            self.param2hyb[usize::from(self.kernels[i] + 1)] = (i + 1) as u8;
            i += 1;
        }

        let mut pb = usize::from(self.kernels[i - 1] + 2);
        while pb < (MAX_PARAMETER_BANDS + 1) {
            self.param2hyb[pb] = i as u8;
            pb += 1;
        }

        for p in 0..MAX_PARAMETER_BANDS {
            self.kernels_width[p] = self.param2hyb[p + 1] - self.param2hyb[p];
        }

        SacDecoderError::Ok
    }

    /// Applies M2 parameter for 212 mode, upmix from mono to stereo.
    ///
    /// # Parameters
    ///
    /// - `m2_data`: Reference to M2 data.
    /// - `alpha`: Smoothing factor between current and previous parameter band. Rangeability
    ///   between 0.f and 1.f.
    ///
    /// # Return
    ///
    /// - `SacDecoderError`.
    pub(super) fn apply_m2_mode_212(
        &mut self,
        m2_data: &mut M2Data,
        alpha: f32,
    ) -> SacDecoderError {
        // For stereoConfigIndex == 3 case hybridBands is < 71.
        let pb_max = usize::from(self.kernels[self.hybrid_bands - 1] + 1);

        for (row, hyb_output_cplx_dry) in self
            .hyb_output_cplx_dry
            .iter_mut()
            .enumerate()
            .take(usize::from(m2_data.num_m2_rows()))
        {
            let m_param: Option<&[[f32; 28]; 2]> =
                m2_data.m2_row(ComplexPart::Real, State::Curr, row);

            let (m_param_0, m_param_1) = match m_param {
                Some(param_row) => (param_row[0], param_row[1]),
                None => return SacDecoderError::ParseError,
            };

            let m_param_prev: Option<&[[f32; 28]; 2]> =
                m2_data.m2_row(ComplexPart::Real, State::Prev, row);

            let (m_param_prev_0, m_param_prev_1) = match m_param_prev {
                Some(prev_param_row) => (prev_param_row[0], prev_param_row[1]),
                None => return SacDecoderError::ParseError,
            };

            let mut bands = 0;
            let temp_sig_0 = &self.temp_signal[0];
            let temp_sig_1 = &self.temp_signal[1];

            for (p0, pp0, p1, pp1, kw) in izip!(
                m_param_0.iter(),
                m_param_prev_0.iter(),
                m_param_1.iter(),
                m_param_prev_1.iter(),
                self.kernels_width.iter()
            )
            .take(pb_max)
            {
                let tmp_0 = interpolate_parameter(alpha, *p0, *pp0);
                let tmp_1 = interpolate_parameter(alpha, *p1, *pp1);
                let t_bands = usize::from(*kw) + bands;
                for (ts0, ts1, cplx_dry) in izip!(
                    temp_sig_0[bands..t_bands].iter(),
                    temp_sig_1[bands..t_bands].iter(),
                    hyb_output_cplx_dry[bands..t_bands].iter_mut(),
                ) {
                    let cplx_0 = ts0.scale(tmp_0);
                    let cplx_1 = ts1.scale(tmp_1);
                    *cplx_dry = cplx_0 + cplx_1;
                }
                bands = t_bands;
            }
        }

        SacDecoderError::Ok
    }

    /// Adds dry and wet signals.
    ///
    /// # Parameters
    ///
    /// - `num_out_channels`:Number of output channels.
    pub(super) fn add_dry_wet(&mut self, num_out_channels: u8) {
        for channel in 0..usize::from(num_out_channels) {
            for (dry, wet) in izip!(
                self.hyb_output_cplx_dry[channel].iter_mut(),
                self.hyb_output_cplx_wet[channel].iter()
            )
            .take(self.hybrid_bands)
            {
                *dry += *wet;
            }
        }
    }

    /// Copies surround bypass buffer to the output.
    pub(super) fn apply_bypass(&mut self) {
        // Determine output channel indices for tree config == TREE_212.
        // Left front.
        let lf = 0;
        // Right front.
        let rf = 1;

        self.hyb_output_cplx_dry[lf].copy_from_slice(&self.hyb_input_cplx);
        self.hyb_output_cplx_dry[rf].copy_from_slice(&self.hyb_input_cplx);
    }

    /// Number of bands.
    pub(super) fn hyb_analysis_num_bands(&self, idx: usize) -> u8 {
        if let Some(hybri_analysis_vec) = self.hybrid_analysis.as_deref() {
            return hybri_analysis_vec[idx].num_bands;
        }
        0
    }

    pub(super) fn hybrid_synthesis(&self, channel: usize) -> &HybridSynthesis {
        &self.hybrid_synthesis[channel]
    }
}

#[inline]
fn interpolate_parameter(alpha: f32, a: f32, b: f32) -> f32 {
    b - alpha * b + alpha * a
}

fn interp_angle(mut angle_1: f32, mut angle_2: f32, alpha: f32, pi_x2: f32) -> f32 {
    if (angle_2 - angle_1) > std::f32::consts::PI {
        angle_2 -= pi_x2;
    }

    if (angle_1 - angle_2) > std::f32::consts::PI {
        angle_1 -= pi_x2;
    }

    interpolate_parameter(alpha, angle_2, angle_1)
}

fn m2_param_to_kernel_mult(
    kernel: &mut [f32],
    m_param: &[f32],
    m_param_prev: &[f32],
    width: &[u8],
    alpha: f32,
    num_bands: u8,
) {
    let mut bands = 0;
    for (&a, &b, &w) in
        izip!(m_param.iter(), m_param_prev.iter(), width.iter()).take(usize::from(num_bands))
    {
        let tmp = interpolate_parameter(alpha, a, b);
        let t_bands = bands + usize::from(w);
        kernel[bands..t_bands].fill(tmp);
        bands = t_bands;
    }
}

fn hybrid_subbands(qmf_subbands: u8) -> usize {
    qmf_subbands as usize + (HYB_FB_BANDS_HYB_OUT - HYB_FB_BANDS_QMF_IN)
}
