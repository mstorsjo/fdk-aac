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
//! Complex quadrature mirror filter (QMF) domain
//!
//! This is a module to efficiently handle QMF data for multiple channels and
//! to share the data between e.g. SBR and MPS.

use itertools::izip;
use num_complex::Complex;
use std::array;

use super::qmf::{QmfFilterBank, QmfFlags, QmfMode, QMF_MAX_SYNTHESIS_SUBBANDS};

const CMPLX_MOD: u16 = 2;

pub const QMF_DOMAIN_MAX_ANALYSIS_QMF_BANDS: u16 = 64;
pub const QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS: usize = QMF_MAX_SYNTHESIS_SUBBANDS;
pub const QMF_DOMAIN_MAX_TIMESLOTS: u8 = 64;
pub const QMF_DOMAIN_MAX_OV_TIMESLOTS: u8 = 12;
pub const QMF_MAX_PROC_SUBBANDS: u8 = 64;

const QMF_DOMAIN_ANALYSIS_QMF_BANDS_8: u8 = 8;
const QMF_DOMAIN_ANALYSIS_QMF_BANDS_16: u8 = 16;
const QMF_DOMAIN_ANALYSIS_QMF_BANDS_24: u8 = 24;
pub const QMF_DOMAIN_ANALYSIS_QMF_BANDS_32: u8 = 32;

/// Maximum number of workbuffer in qmf domain.
const QMF_DOMAIN_MAX_WB: usize = 2;
/// Index of workbuffer 0 in qmf domain.
const QMF_DOMAIN_WB_0: usize = 0;
/// Index of workbuffer 1 in qmf domain.
const QMF_DOMAIN_WB_1: usize = 1;

#[repr(C)]
#[derive(Debug)]
/// Defines errors of `QmfDomain`.
pub enum QmfDomainError {
    Ok = 0,        // No error occurred.
    InitError = 2, // An error during filterbank-setup occurred.
}

#[repr(C)]
#[derive(PartialEq)]
/// Defines QMF domain channel selection.
pub enum QmfDomainSelect {
    QmfDomainIn = 0x0,  // Select input channel QMF domain.
    QmfDomainOut = 0x1, // Select output channel QMF domain.
}

/// Structure to hold the configuration parameters of the QMF domain.
#[repr(C)]
#[derive(Default, PartialEq, Debug)]
pub struct QmfDomainParams {
    /// Flags to be set on all QMF analysis/synthesis filter instances.
    pub flags: QmfFlags,
    /// Number of QMF input channels.
    pub num_input_channels: u8,
    /// Number of QMF output channels.
    pub num_output_channels: u8,
    /// Number of QMF analysis bands for all input channels.
    pub num_bands_analysis: u8,
    /// Number of QMF time slots (stored in work buffer memory).
    pub num_qmf_time_slots: u8,
    /// Number of QMF overlap/delay time slots (stored in
    /// persistent memory).
    pub num_qmf_ov_time_slots: u8,
    /// Number of QMF bands which are processed by the
    /// decoder. Typically this is equal to num_bands_synthesis
    /// but it may differ if the QMF based resampler is being used.
    pub num_qmf_proc_bands: u8,
    /// Number of complete QMF channels which need to
    /// coexist in memory at the same time. For most cases
    /// this is 1 which means the work buffer can be shared
    /// between audio channels.
    pub num_qmf_proc_channels: u8,
    /// Number of QMF synthesis bands for all output channels.
    pub num_bands_synthesis: u16,
    /// Number of work buffers.
    pub num_work_buffer: u8,
    /// Array contains the workbuffer index for all channels.
    pub work_buffer_index: [u8; 8],
}

impl QmfDomainParams {
    /// Creates a `QmfDomainParams` instance.
    pub fn new() -> Self {
        Self::default()
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// Structure to hold the global configuration parameters of the QMF domain.
struct QmfDomainGc {
    /// Flag to signal that QMF domain is set explicitly instead of SBR
    /// and MPS init routines.
    qmf_domain_explicit_config: u8,
    /// Configuration parameters for the QMF domain.
    qmf_domain_params: QmfDomainParams,
    /// Work buffer memory.
    work_buffer: [Vec<f32>; QMF_DOMAIN_MAX_WB],
}

impl QmfDomainGc {
    /// Allocates persistent (work buffer) memory.
    fn init(&mut self) {
        let params = &self.qmf_domain_params;
        let wb_size = usize::from(CMPLX_MOD)
            * usize::from(params.num_qmf_time_slots)
            * usize::from(params.num_qmf_proc_bands)
            * usize::from(params.num_qmf_proc_channels);

        // Allocates work buffer on heap memory.
        for work_buffer in self.work_buffer.iter_mut() {
            if work_buffer.is_empty() {
                *work_buffer = vec![0.0_f32; wb_size];
            }
        }
    }

    /// De-allocates heap allocated memory and resets.
    fn deinit(&mut self) {
        for work_buffer in self.work_buffer.iter_mut().take(QMF_DOMAIN_MAX_WB) {
            work_buffer.clear();
            // Deallocate memory by shrinking capacity to zero.
            work_buffer.shrink_to_fit();
        }
        // Clean the memory - reset to default values.
        *self = Self::default();
    }
}

#[repr(C)]
#[derive(Default, Clone, Debug)]
/// QMF domain input channel structure.
//
// Structure representing one QMF input channel. This includes the QMF analysis
// and the QMF domain data representation needed by the codec. Work buffer data
// may be shared between channels if the codec processes all QMF channels in a
// consecutive order.
struct QmfDomainIn {
    /// QMF (analysis) filter bank structure.
    analysis_fb: QmfFilterBank,
    /// Overlap buffer QMF analysis states (persistent memory).
    overlap_buffer: Vec<f32>,
    /// QMF real data time slot offsets.
    qmf_slots_real_offsets: Vec<u16>,
    /// QMF imaginary data time slot offsets.
    qmf_slots_imag_offsets: Vec<u16>,
}

impl QmfDomainIn {
    /// Creates a `QmfDomainIn` instance.
    fn new() -> Self {
        Self::default()
    }

    /// Initializes `QmfDomainIn` and allocates persistent memory.
    ///
    /// # Parameters
    ///
    /// - `params`: Qmf domain configuration parameters.
    fn init(&mut self, params: &QmfDomainParams) {
        let len_filtes_states = usize::from(params.num_bands_analysis) * 10;
        let len_qmf_offsets = usize::from(params.num_qmf_ov_time_slots + params.num_qmf_time_slots);
        let len_overlap_buffer = usize::from(CMPLX_MOD)
            * usize::from(params.num_qmf_ov_time_slots)
            * usize::from(params.num_qmf_proc_bands);

        self.analysis_fb = QmfFilterBank::new(len_filtes_states);
        self.overlap_buffer = vec![0.0_f32; len_overlap_buffer];
        self.qmf_slots_real_offsets = vec![0_u16; len_qmf_offsets];
        self.qmf_slots_imag_offsets = vec![0_u16; len_qmf_offsets];
    }
}

#[repr(C)]
#[derive(Default, Clone, Debug)]
/// QMF domain output channel structure.
// Structure representing one QMF output channel.
struct QmfDomainOut {
    /// QMF (synthesis) filter bank structure.
    synthesis_fb: QmfFilterBank,
}

impl QmfDomainOut {
    /// Creates a `QmfDomainOut` instance.
    fn new() -> Self {
        Self::default()
    }

    /// Initializes `QmfDomainOut` and allocates persistent memory.
    ///
    /// # Parameters
    ///
    /// - `len_filter_states`: Length of filter states in filter bank instance.
    fn init(&mut self, len_filter_states: usize) {
        self.synthesis_fb = QmfFilterBank::new(len_filter_states);
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// QMF domain structure.
// Structure representing the QMF domain for multiple channels.
pub struct QmfDomain {
    /// Global configuration structure.
    global_conf: QmfDomainGc,
    /// QMF domain input structures.
    input: Vec<QmfDomainIn>,
    /// QMF domain output structures.
    output: Vec<QmfDomainOut>,
}

impl QmfDomain {
    /// Creates a `QmfDomain` instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Initializes `QmfDomain` and allocates input and output channels.
    fn init(&mut self) {
        let gc = &mut self.global_conf;
        let params = &gc.qmf_domain_params;

        if self.input.is_empty() {
            // Allocate QMF domain input channels, buffers and analysis filter bank.
            self.input = Vec::new();

            for _channel in 0..usize::from(params.num_input_channels) {
                let mut input_channel = QmfDomainIn::new();
                input_channel.init(params);
                self.input.push(input_channel);
            }
        }

        if self.output.is_empty() {
            // Allocate QMF domain output channels and synthesis filter bank.
            let len_synthesis_filter_states = usize::from(params.num_bands_synthesis * 9);
            self.output = Vec::new();

            for _channel in 0..usize::from(params.num_output_channels) {
                let mut output_channel = QmfDomainOut::new();
                output_channel.init(len_synthesis_filter_states);
                self.output.push(output_channel);
            }
        }

        // Allocate work buffer.
        gc.init();
    }

    /// Initializes analysis and synthesis QMF filter banks of `QmfDomain`.
    /// # Return
    ///    - `true` on success, othewise `false`.
    fn init_filter_bank(&mut self) -> bool {
        let gc = &self.global_conf;
        let params = &self.global_conf.qmf_domain_params;
        let num_time_slots = usize::from(params.num_qmf_time_slots);
        let num_ov_ts = usize::from(params.num_qmf_ov_time_slots);
        let lfs = params.num_bands_analysis;
        let hfs = params
            .num_bands_synthesis
            .min(QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS as u16) as u8;
        let num_proc_bands = u16::from(params.num_qmf_proc_bands);
        let mut error = 0;

        for work_buffer in gc
            .work_buffer
            .iter()
            .take(usize::from(params.num_work_buffer))
        {
            // Work buffer holds one full frame of QMF data.
            if work_buffer.is_empty() && num_time_slots != 0 {
                return false;
            }

            if usize::from(CMPLX_MOD * u16::from(params.num_qmf_time_slots) * num_proc_bands)
                > work_buffer.len()
                && num_time_slots != 0
            {
                return false;
            }
        }

        // QMF domain input filter bank initialization.
        for domain_in in self
            .input
            .iter_mut()
            .take(usize::from(params.num_input_channels))
        {
            // Check overlap buffer.
            if domain_in.overlap_buffer.is_empty() && num_ov_ts != 0 {
                return false;
            }

            // Determine offset of qmf slots in overlap buffer.
            let mut ts = 0;
            for (r_offset, i_offset) in izip!(
                domain_in.qmf_slots_real_offsets.iter_mut(),
                domain_in.qmf_slots_imag_offsets.iter_mut()
            )
            .take(num_ov_ts)
            {
                *r_offset = (CMPLX_MOD * ts) * num_proc_bands;
                *i_offset = (CMPLX_MOD * ts + 1) * num_proc_bands;
                ts += 1;
            }

            // Determine offset of qmf slots in workbuffer.
            ts = 0;
            for (r_offset, i_offset) in izip!(
                domain_in.qmf_slots_real_offsets[num_ov_ts..].iter_mut(),
                domain_in.qmf_slots_imag_offsets[num_ov_ts..].iter_mut()
            )
            .take(num_time_slots)
            {
                *r_offset = (CMPLX_MOD * ts) * num_proc_bands;
                *i_offset = (CMPLX_MOD * ts + 1) * num_proc_bands;
                ts += 1;
            }

            // Init analysis filter bank.
            let mut fb_lfs = domain_in.analysis_fb.num_lf_subbands();
            let mut fb_hfs = domain_in.analysis_fb.num_hf_subbands();
            if fb_lfs == 0 {
                fb_lfs = lfs;
            }

            if fb_hfs == 0 {
                fb_hfs = hfs;
            }

            error |= domain_in.analysis_fb.init(
                params.num_qmf_time_slots,
                fb_lfs,
                fb_hfs,
                params.num_bands_analysis,
                params.flags,
                QmfMode::Analysis,
            )
        }

        // QMF domain output filter bank initialization.
        for domain_out in self
            .output
            .iter_mut()
            .take(usize::from(params.num_output_channels))
        {
            let out_gain = domain_out.synthesis_fb.out_gain();
            // Init analysis filter bank
            let mut fb_lfs = domain_out.synthesis_fb.num_lf_subbands();
            let mut fb_hfs = domain_out.synthesis_fb.num_hf_subbands();
            if fb_lfs == 0 {
                fb_lfs = lfs;
            }

            if fb_hfs == 0 {
                fb_hfs = hfs;
            }

            error |= domain_out.synthesis_fb.init(
                params.num_qmf_time_slots,
                fb_lfs,
                fb_hfs,
                params.num_bands_synthesis.try_into().unwrap(),
                params.flags,
                QmfMode::Synthesis,
            );

            if out_gain != 0.0_f32 {
                domain_out.synthesis_fb.set_output_gain(out_gain);
            }
        }
        error == 0
    }

    /// Checks for parameter-change requests in global config and (re-)configure QMF
    /// domain accordingly.
    ///
    /// # Parameters
    ///
    /// - `params_requested`: Requested configuration parameters for the QMF domain.
    pub fn configure(&mut self, params_requested: &mut QmfDomainParams) -> QmfDomainError {
        let params = &mut self.global_conf.qmf_domain_params;

        // Checking whether the internal parameters are the same as the requested
        // parameters.
        if *params != *params_requested {
            if params_requested.num_qmf_proc_channels > 0
                && params_requested.num_qmf_proc_bands != QMF_MAX_PROC_SUBBANDS
            {
                return QmfDomainError::InitError;
            }

            if params_requested.num_bands_analysis > params_requested.num_qmf_proc_bands {
                // In general the output of the qmf analysis is written to QMF memory
                // slots (which has a size defined by num_qmf_proc_bands). num_bands_synthesis
                // may be larger than num_qmf_proc_bands. This is e.g. the case if the QMF based
                // resampler is used.
                return QmfDomainError::InitError;
            }

            if params_requested.num_input_channels > 8 || params_requested.num_output_channels > 8 {
                return QmfDomainError::InitError;
            }

            if params_requested.num_qmf_time_slots > QMF_DOMAIN_MAX_TIMESLOTS {
                return QmfDomainError::InitError;
            }

            if params_requested.num_qmf_ov_time_slots > QMF_DOMAIN_MAX_OV_TIMESLOTS {
                return QmfDomainError::InitError;
            }

            let ana_bands = params_requested.num_bands_analysis;
            if ana_bands > 0
                && !((ana_bands == QMF_DOMAIN_ANALYSIS_QMF_BANDS_8)
                    || (ana_bands == QMF_DOMAIN_ANALYSIS_QMF_BANDS_16)
                    || (ana_bands == QMF_DOMAIN_ANALYSIS_QMF_BANDS_24)
                    || (ana_bands == QMF_DOMAIN_ANALYSIS_QMF_BANDS_32)
                    || (u16::from(ana_bands) == QMF_DOMAIN_MAX_ANALYSIS_QMF_BANDS))
            {
                return QmfDomainError::InitError;
            }

            if usize::from(params_requested.num_bands_synthesis)
                > QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS
            {
                return QmfDomainError::InitError;
            }

            // - Allocate persistent memory if necessary (analysis state-buffers,
            // timeslot-offset-arrays, overlap-buffers, synthesis state-buffers,
            // workbuffer, parkbuffer, filterbank structures).
            // - If only the flags are different, only a reinitialization of the
            // filterbanks is required.

            if params.num_qmf_proc_channels != params_requested.num_qmf_proc_channels
                || params.num_qmf_proc_bands != params_requested.num_qmf_proc_bands
                || params.num_input_channels != params_requested.num_input_channels
                || params.num_bands_analysis != params_requested.num_bands_analysis
                || params.num_qmf_time_slots != params_requested.num_qmf_time_slots
                || params.num_qmf_ov_time_slots != params_requested.num_qmf_ov_time_slots
                || params.num_output_channels != params_requested.num_output_channels
                || params.num_bands_synthesis != params_requested.num_bands_synthesis
                || params.num_work_buffer != params_requested.num_work_buffer
            {
                // Quick check whether config change has been performed in advance.
                debug_assert!(
                    self.input.is_empty()
                        && self.output.is_empty()
                        && self.global_conf.work_buffer[QMF_DOMAIN_WB_0].is_empty()
                        && self.global_conf.work_buffer[QMF_DOMAIN_WB_1].is_empty()
                );

                params.num_input_channels = params_requested.num_input_channels;
                params.num_bands_analysis = params_requested.num_bands_analysis;
                params.num_qmf_time_slots = params_requested.num_qmf_time_slots;
                params.num_qmf_ov_time_slots = params_requested.num_qmf_ov_time_slots;
                params.num_output_channels = params_requested.num_output_channels;
                params.num_bands_synthesis = params_requested.num_bands_synthesis;
                params.num_work_buffer = params_requested.num_work_buffer;
                params.num_qmf_proc_bands = params_requested.num_qmf_proc_bands;
                params.num_qmf_proc_channels = params_requested.num_qmf_proc_channels;
                params
                    .work_buffer_index
                    .copy_from_slice(&params_requested.work_buffer_index);

                self.init();

                let params = &mut self.global_conf.qmf_domain_params;

                // Set request-flag for downsampled SBR.
                if params.num_bands_analysis == 32
                    && params.num_bands_synthesis == 32
                    && !params.flags.intersects(QmfFlags::CLDFB | QmfFlags::MPSLDFB)
                {
                    params_requested.flags.insert(QmfFlags::DOWNSAMPLED);
                }

                // Set request-flags.
                if params.flags != params_requested.flags {
                    if params_requested.flags.contains(QmfFlags::MPSLDFB)
                        && params_requested.flags.contains(QmfFlags::CLDFB)
                    {
                        params_requested.flags.remove(QmfFlags::CLDFB);
                    }
                    params.flags = params_requested.flags;
                }

                if !self.init_filter_bank() {
                    return QmfDomainError::InitError;
                }
            } else if !self.reinit_filter_bank(params_requested) {
                return QmfDomainError::InitError;
            }
        }
        QmfDomainError::Ok
    }

    /// De-allocates heap allocated QMF domain memory.
    pub fn deinit(&mut self) {
        // De-allocate QMF domain input channels.
        self.input.clear();
        self.input.shrink_to_fit();

        // De-allocate QMF domain output channels.
        self.output.clear();
        self.output.shrink_to_fit();

        // De-allocate global config (work buffer) memory.
        self.global_conf.deinit();
    }

    /// Returns status of explicit config of `QmfDomain`.
    pub fn explicit_config(&self) -> u8 {
        self.global_conf.qmf_domain_explicit_config
    }

    /// Returns `Option` of mutable reference to specific `analysis` (input channel) or
    /// `synthesis` (output channel) filter bank of `QmfDomain`, otherwise `None`.
    ///
    /// # Parameters
    ///
    /// - `channel`: Channel number to be returned.
    /// - `qmf_select`: Value of type `QmfDomainSelect`.
    pub fn get_filter_bank_mut(
        &mut self,
        channel: usize,
        qmf_select: QmfDomainSelect,
    ) -> Option<&mut QmfFilterBank> {
        match qmf_select {
            QmfDomainSelect::QmfDomainIn => {
                if !self.input.is_empty() {
                    Some(&mut self.input[channel].analysis_fb)
                } else {
                    None
                }
            }
            QmfDomainSelect::QmfDomainOut => {
                if !self.output.is_empty() {
                    Some(&mut self.output[channel].synthesis_fb)
                } else {
                    None
                }
            }
        }
    }

    /// Returns `Option` of reference to specific `analysis` (input channel) or
    /// `synthesis` (output channel) filter bank of `QmfDomain`, otherwise `None`.
    ///
    /// # Parameters
    ///
    /// - `channel`: Channel number to be returned.
    /// - `qmf_select`: Value of type `QmfDomainSelect`.
    pub fn get_filter_bank(
        &self,
        channel: usize,
        qmf_select: QmfDomainSelect,
    ) -> Option<&QmfFilterBank> {
        match qmf_select {
            QmfDomainSelect::QmfDomainIn => {
                if !self.input.is_empty() {
                    Some(&self.input[channel].analysis_fb)
                } else {
                    None
                }
            }
            QmfDomainSelect::QmfDomainOut => {
                if !self.output.is_empty() {
                    Some(&self.output[channel].synthesis_fb)
                } else {
                    None
                }
            }
        }
    }

    /// Gets one slot of QMF data and adapts the scaling.
    ///
    /// # Parameters
    ///
    /// - `gain`: Gain consisting of pre gain and clipping protection gain.
    /// - `num_ts`: Time slot number to be obtained.
    /// - `start_band`: Start index of QMF bands to be obtained.
    /// - `stop_band`: Stop index of QMF bands to be obtained.
    /// - `qmf_out_real`: Output buffer (real QMF data).
    /// - `qmf_out_imag`: Output buffer (imag QMF data).
    pub fn get_slot(
        &mut self,
        gain: f32,
        num_ts: u8,
        start_band: u8,
        stop_band: u8,
        qmf_out_real: &mut [f32],
        qmf_out_imag: &mut [f32],
    ) {
        let gc = &self.global_conf;
        let qd_ch = &self.input[0];
        let params = &gc.qmf_domain_params;

        let wb_index = usize::from(params.work_buffer_index[0]);
        let work_buffer = &gc.work_buffer[wb_index];
        debug_assert!(!work_buffer.is_empty());

        let slots_real_offset = usize::from(qd_ch.qmf_slots_real_offsets[usize::from(num_ts)]);
        let slots_imag_offset = usize::from(qd_ch.qmf_slots_imag_offsets[usize::from(num_ts)]);
        let (real_buff, imag_buff) = if num_ts < params.num_qmf_ov_time_slots {
            (
                &qd_ch.overlap_buffer[slots_real_offset..],
                &qd_ch.overlap_buffer[slots_imag_offset..],
            )
        } else {
            (
                &work_buffer[slots_real_offset..],
                &work_buffer[slots_imag_offset..],
            )
        };

        let lower_subbands = qd_ch.analysis_fb.num_lf_subbands();
        let upper_subbands = qd_ch.analysis_fb.num_hf_subbands();

        debug_assert!(num_ts < (params.num_qmf_time_slots + params.num_qmf_ov_time_slots));
        debug_assert!(stop_band <= params.num_qmf_proc_bands);

        let mut start = usize::from(start_band);
        let mut end = usize::from(lower_subbands.min(stop_band));
        if start < end {
            for (out_real, out_imag, in_real, in_imag) in izip!(
                qmf_out_real[start..end].iter_mut(),
                qmf_out_imag[start..end].iter_mut(),
                real_buff[start..end].iter(),
                imag_buff[start..end].iter(),
            )
            .take(end - start)
            {
                *out_real = *in_real * gain;
                *out_imag = *in_imag * gain;
            }
        }

        start = end;
        end = usize::from(upper_subbands.min(stop_band));
        if start < end {
            for (out_real, out_imag, in_real, in_imag) in izip!(
                qmf_out_real[start..end].iter_mut(),
                qmf_out_imag[start..end].iter_mut(),
                real_buff[start..end].iter(),
                imag_buff[start..end].iter(),
            )
            .take(end - start)
            {
                *out_real = *in_real * gain;
                *out_imag = *in_imag * gain;
            }
        }

        start = end;
        end = usize::from(stop_band);
        if start < end {
            // Fill the rest of buffer with zeros.
            qmf_out_real[start..end].fill(0.0_f32);
            qmf_out_imag[start..end].fill(0.0_f32);
        }
    }

    /// Returns number of analysis bands of `QmfDomain`.
    pub fn num_bands_analysis(&self) -> u8 {
        self.global_conf.qmf_domain_params.num_bands_analysis
    }

    /// Returns number of synthesis bands of `QmfDomain`.
    pub fn num_bands_synthesis(&self) -> u16 {
        self.global_conf.qmf_domain_params.num_bands_synthesis
    }

    /// Returns number of input channels of `QmfDomain`.
    pub fn num_input_channels(&self) -> u8 {
        self.global_conf.qmf_domain_params.num_input_channels
    }

    /// Returns number of qmf time slots of `QmfDomain`.
    pub fn num_qmf_time_slots(&self) -> u8 {
        self.global_conf.qmf_domain_params.num_qmf_time_slots
    }

    /// Returns a mutable reference to overlap buffer of `QmfDomain` input channel.
    pub fn overlap_buffer_mut(&mut self, channel: usize) -> &mut [f32] {
        &mut self.input[channel].overlap_buffer
    }

    /// Copy QMF data to HBE.
    /// For the case of stereoCfgIndex3 with HBE the HBE buffer is copied into
    /// the processing channel work buffer and the processing channel work buffer is
    /// copied into the HBE buffer.
    ///
    /// # Parameters
    ///
    /// - `hbe_buffer_real`: HBE QMF real data time slots.
    /// - `hbe_buffer_imag`: HBE QMF imaginary data time slots.
    /// - `channel`: Channel number to be processed.
    pub fn qmf_data_to_hbe(
        &mut self,
        hbe_buffer_real: &mut [&mut [f32]],
        hbe_buffer_imag: &mut [&mut [f32]],
        channel: usize,
    ) {
        let gc = &mut self.global_conf;
        let qd_ch = &mut self.input[channel];
        let params = &gc.qmf_domain_params;

        let wb_index: usize = usize::from(params.work_buffer_index[channel]);
        let qmf_buffer = &mut gc.work_buffer[wb_index];
        debug_assert!(!qmf_buffer.is_empty());

        let bands = usize::from(params.num_bands_analysis);
        let num_ts = usize::from(params.num_qmf_time_slots);

        debug_assert!(bands <= QMF_DOMAIN_MAX_ANALYSIS_QMF_BANDS as usize);
        let num_ov_ts = usize::from(params.num_qmf_ov_time_slots);

        // Copy current data of processing channel.
        for (real_offset, imag_offset, hbe_real_slot, hbe_imag_slot) in izip!(
            &qd_ch.qmf_slots_real_offsets[num_ov_ts..num_ov_ts + num_ts],
            &qd_ch.qmf_slots_imag_offsets[num_ov_ts..num_ov_ts + num_ts],
            hbe_buffer_real.iter_mut(),
            hbe_buffer_imag.iter_mut()
        )
        .take(num_ts)
        {
            // swap data between HBE QMF buffer and QMF domain work buffer.
            // Real data slot.
            qmf_buffer[usize::from(*real_offset)..usize::from(*real_offset) + bands]
                .swap_with_slice(&mut hbe_real_slot[..bands]);

            // Imag data slot.
            qmf_buffer[usize::from(*imag_offset)..usize::from(*imag_offset) + bands]
                .swap_with_slice(&mut hbe_imag_slot[..bands]);
        }
    }

    /// Returns a mutable reference to `qmf slots imag offsets` of `QmfDomain` input channel.
    pub fn qmf_slots_imag_offsets_mut(&mut self, channel: usize) -> &mut [u16] {
        &mut self.input[channel].qmf_slots_imag_offsets
    }

    /// Returns a mutable reference to `qmf slots real offsets` of `QmfDomain` input channel.
    pub fn qmf_slots_real_offsets_mut(&mut self, channel: usize) -> &mut [u16] {
        &mut self.input[channel].qmf_slots_real_offsets
    }

    /// Re-initialize analysis and synthesis QMF filter banks and set up QMF data representation.
    ///
    /// # Parameters
    ///
    /// - `params_requested`: Requested configuration parameters for the QMF domain. Returns `true`
    ///   in case of success, otherwise `false`.
    pub fn reinit_filter_bank(&mut self, params_requested: &mut QmfDomainParams) -> bool {
        let params = &mut self.global_conf.qmf_domain_params;
        // In case flags have changed the filterbank needs to be re-initialized.
        if params.flags != params_requested.flags {
            if params_requested.flags.contains(QmfFlags::MPSLDFB)
                && params_requested.flags.contains(QmfFlags::CLDFB)
            {
                params_requested.flags.remove(QmfFlags::CLDFB);
            }
            params.flags = params_requested.flags;

            return self.init_filter_bank();
        }
        true
    }

    /// When QMF processing of one channel is finished, copy the overlap/delay
    /// part into the persistent memory to be used in the next frame.
    ///
    /// # Parameters
    ///
    /// - `channel`: Channel number to be processed.
    pub fn save_overlap(&mut self, channel: usize) {
        let gc: &mut QmfDomainGc = &mut self.global_conf;
        let qd_ch = &mut self.input[channel];
        let params = &mut gc.qmf_domain_params;

        let num_time_slots = usize::from(params.num_qmf_time_slots);
        let num_proc_bands = usize::from(params.num_qmf_proc_bands);

        let qmf_buffer_ov = &mut qd_ch.overlap_buffer;
        let qmf_buffer = &gc.work_buffer[usize::from(params.work_buffer_index[channel])];

        let real_offsets = &mut qd_ch.qmf_slots_real_offsets;
        let imag_offsets = &mut qd_ch.qmf_slots_imag_offsets;

        // For high part it would be enough to save only used part of overlap area.
        for ts in 0..usize::from(params.num_qmf_ov_time_slots) {
            // Real part of signal.
            let r_offset_ov_ts = usize::from(real_offsets[ts]);
            let r_offset_ts = usize::from(real_offsets[ts + num_time_slots]);
            qmf_buffer_ov[r_offset_ov_ts..r_offset_ov_ts + num_proc_bands]
                .copy_from_slice(&qmf_buffer[r_offset_ts..r_offset_ts + num_proc_bands]);

            // Imaginary part of signal.
            let i_offset_ov_ts = usize::from(imag_offsets[ts]);
            let i_offset_ts = usize::from(imag_offsets[ts + num_time_slots]);
            qmf_buffer_ov[i_offset_ov_ts..i_offset_ov_ts + num_proc_bands]
                .copy_from_slice(&qmf_buffer[i_offset_ts..i_offset_ts + num_proc_bands]);
        }
    }

    /// Sets explicit config of `QmfDomain`.
    pub fn set_explicit_config(&mut self, config_value: u8) {
        self.global_conf.qmf_domain_explicit_config = config_value;
    }

    /// Sets one slot of QMF data.
    ///
    /// # Parameters
    ///
    /// - `qmf_real`: Input buffer (real QMF data).
    /// - `qmf_imag`: Input buffer (imag QMF data).
    /// - `scale_post`: Scale factor to be applied to QMF data.
    /// - `channel`: Channel number to be processed.
    /// - `num_ts`: Time slot number to be processed.
    pub fn set_slot(
        &mut self,
        qmf_real: &[f32],
        qmf_imag: &[f32],
        scale_post: f32,
        channel: usize,
        num_ts: usize,
    ) {
        let gc = &mut self.global_conf;
        // let qd_ch = &mut self.input[channel];
        let params = &gc.qmf_domain_params;

        let wb_index = usize::from(params.work_buffer_index[channel]);
        let qmf_work_buffer = &mut gc.work_buffer[wb_index];
        debug_assert!(!qmf_work_buffer.is_empty());

        // Derive num_qmf_bands from qmf_real/qmf_imag length.
        let num_qmf_bands = qmf_real.len();
        let bands = usize::from(params.num_qmf_proc_bands);

        debug_assert!(num_qmf_bands <= bands);
        debug_assert!(num_ts < params.num_qmf_time_slots.into());
        debug_assert!(((usize::from(CMPLX_MOD) * num_ts + 1) * bands) <= qmf_work_buffer.len());

        let wb_start = num_ts * usize::from(CMPLX_MOD) * bands;
        // real_slot  -> wb_start  ; imag_slot -> wb_start + bands.
        // QMF work buffer: time slots in grid of num_qmf_proc_bands.
        let (real, imag) = qmf_work_buffer[wb_start..].split_at_mut(bands);

        for (re, im, qmf_re, qmf_im) in izip!(
            real.iter_mut(),
            imag.iter_mut(),
            qmf_real.iter(),
            qmf_imag.iter()
        )
        .take(num_qmf_bands)
        {
            *re = *qmf_re * scale_post;
            *im = *qmf_im * scale_post;
        }
    }

    /// Returns a mutable reference to work buffer of `QmfDomain`.
    pub fn work_buffer_mut(&mut self) -> &mut [Vec<f32>; QMF_DOMAIN_MAX_WB] {
        &mut self.global_conf.work_buffer
    }

    /// Creates array of mutable slices (real and imag) out of overlap buffer.
    /// Also updates `real_slots` and `imag_slots`.
    ///
    /// # Parameters
    ///
    /// - `overlap_buffer`: Input overlap buffer to be sliced.
    /// - `real_slots`: Reference to array of slices for qmf real data slots.
    /// - `imag_slots`: Reference to array of slices for qmf imaginary data slots.
    /// - `num_ov_slots`: Number of overlap slots.
    /// - `num_proc_bands`: Number of processing bands per overlap slot.
    fn set_pointer_array_for_overlap_buffer_area_mut<'l>(
        overlap_buffer: &'l mut [f32],
        real_slots: &mut [&'l mut [f32]],
        imag_slots: &mut [&'l mut [f32]],
        num_ov_slots: usize,
        num_proc_bands: usize,
    ) {
        for (ri_pair, real_slot, imag_slot) in izip!(
            overlap_buffer.chunks_exact_mut(num_proc_bands * CMPLX_MOD as usize),
            real_slots.iter_mut(),
            imag_slots.iter_mut()
        )
        .take(num_ov_slots)
        {
            (*real_slot, *imag_slot) = ri_pair.split_at_mut(num_proc_bands);
        }
    }

    /// Creates array of immutable slices (real and imag) out of overlap buffer for given channel.
    /// Also updates `real_slots` and `imag_slots`.
    ///
    /// # Parameters
    ///
    /// - `global_conf`: Qmf domain global configuration.
    /// - `overlap_buffer`: Overlap buffer to create qmf slots.
    /// - `real_slots`: Reference to array of slices for qmf real data slots.
    /// - `imag_slots`: Reference to array of slices for qmf imaginary data slots.
    fn set_pointer_array_for_overlap_buffer_area<'l, 's>(
        global_conf: &'l QmfDomainGc,
        overlap_buffer: &'s [f32],
        real_slots: &mut [&'s [f32]],
        imag_slots: &mut [&'s [f32]],
    ) where
        'l: 's,
    {
        let num_ov_slots = usize::from(global_conf.qmf_domain_params.num_qmf_ov_time_slots);
        let num_proc_bands = usize::from(global_conf.qmf_domain_params.num_qmf_proc_bands);

        for (ri_pair, real_slot, imag_slot) in izip!(
            overlap_buffer.chunks_exact(num_proc_bands * CMPLX_MOD as usize),
            real_slots.iter_mut(),
            imag_slots.iter_mut()
        )
        .take(num_ov_slots)
        {
            (*real_slot, *imag_slot) = ri_pair.split_at(num_proc_bands);
        }
    }

    /// Creates array of mutable slices (real and imag) out of work buffer.
    /// Also updates `real_slots` and `imag_slots`.
    ///
    /// # Parameters
    ///
    /// - `work_buffer`: Input work buffer to be sliced.
    /// - `real_slots`: Reference to array of slices for qmf real data slots.
    /// - `imag_slots`: Reference to array of slices for qmf imaginary data slots.
    /// - `num_time_slots`: Number of qmf timeslots.
    /// - `num_proc_bands`: Number of processing bands per timeslot.
    fn set_pointer_array_for_work_buffer_area_mut<'l, 's>(
        work_buffer: &'l mut [f32],
        real_slots: &mut [&'s mut [f32]],
        imag_slots: &mut [&'s mut [f32]],
        num_time_slots: usize,
        num_proc_bands: usize,
    ) where
        'l: 's,
    {
        for (ri_pair, real_slot, imag_slot) in izip!(
            work_buffer.chunks_exact_mut(num_proc_bands * CMPLX_MOD as usize),
            real_slots.iter_mut(),
            imag_slots.iter_mut()
        )
        .take(num_time_slots)
        {
            (*real_slot, *imag_slot) = ri_pair.split_at_mut(num_proc_bands);
        }
    }

    /// Creates array of immutable slices (real and imag) out of work buffer for given channel.
    /// Also updates `real_slots` and `imag_slots`.
    ///
    /// # Parameters
    ///
    /// - `global_conf`: Qmf domain global configuration.
    /// - `real_slots`: Reference to array of slices for qmf real data slots.
    /// - `imag_slots`: Reference to array of slices for qmf imaginary data slots.
    /// - `channel`: Channel number to be processed.
    fn set_pointer_array_for_work_buffer_area<'l, 's>(
        global_conf: &'l QmfDomainGc,
        real_slots: &mut [&'s [f32]],
        imag_slots: &mut [&'s [f32]],
        channel: usize,
    ) where
        'l: 's,
    {
        let gc = global_conf;
        let num_time_slots = usize::from(gc.qmf_domain_params.num_qmf_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);

        let wb_index = usize::from(gc.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &gc.work_buffer[wb_index];
        debug_assert!(!work_buffer.is_empty());

        for (ri_pair, real_slot, imag_slot) in izip!(
            work_buffer.chunks_exact(num_proc_bands * CMPLX_MOD as usize),
            real_slots.iter_mut(),
            imag_slots.iter_mut()
        )
        .take(num_time_slots)
        {
            (*real_slot, *imag_slot) = ri_pair.split_at(num_proc_bands);
        }
    }

    /// Creates array of mutable slices (real and imag) out of overlap buffer for given channel.
    /// Updates `ov_qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `ov_qmf_slots`: `QmfDomainOvQmfSlotsMut` instance with default values.
    /// - `channel`: Channel number to be processed.
    pub fn get_pointer_array_for_ov_qmf_slots_mut<'a>(
        &'a mut self,
        ov_qmf_slots: &mut QmfDomainOvQmfSlotsMut<'a>,
        channel: usize,
    ) {
        let gc = &self.global_conf;
        let overlap_buffer = &mut self.input[channel].overlap_buffer;
        let num_ov_slots = usize::from(gc.qmf_domain_params.num_qmf_ov_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);

        QmfDomain::set_pointer_array_for_overlap_buffer_area_mut(
            overlap_buffer,
            &mut ov_qmf_slots.real,
            &mut ov_qmf_slots.imag,
            num_ov_slots,
            num_proc_bands,
        );
    }

    /// Creates array of immutable slices (real and imag) out of overlap buffer for given channel.
    /// Updates `ov_qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `ov_qmf_slots`: `QmfDomainOvQmfSlots` instance with default values.
    /// - `channel`: Channel number to be processed.
    pub fn get_pointer_array_for_ov_qmf_slots<'a>(
        &'a self,
        ov_qmf_slots: &mut QmfDomainOvQmfSlots<'a>,
        channel: usize,
    ) {
        let gc = &self.global_conf;
        let overlap_buffer = &self.input[channel].overlap_buffer;
        QmfDomain::set_pointer_array_for_overlap_buffer_area(
            gc,
            overlap_buffer,
            &mut ov_qmf_slots.real,
            &mut ov_qmf_slots.imag,
        );
    }

    /// Creates array of immutable slices (real and imag) out of overlap buffer and work buffer for
    /// given channel. Updates `qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `qmf_slots`: `QmfDomainQmfSlots` instance with default values.
    /// - `channel`: Channel number to be processed.
    pub fn get_pointer_array_for_qmf_slots<'l, 'c>(
        &'l self,
        qmf_slots: &mut QmfDomainQmfSlots<'c>,
        channel: usize,
    ) where
        'l: 'c,
    {
        let overlap_buffer = &self.input[channel].overlap_buffer;
        let gc = &self.global_conf;
        QmfDomain::create_pointer_array_for_qmf_slots(gc, overlap_buffer, qmf_slots, channel);
    }

    /// Creates array of mutable slices (real and imag) out of overlap buffer and work buffer.
    /// Updates `qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `qmf_slots`: `QmfDomainQmfSlots` instance with default values.
    /// - `channel`: Channel number to be processed.
    pub fn get_pointer_array_for_qmf_slots_mut<'l, 'c>(
        &'l mut self,
        qmf_slots: &mut QmfDomainQmfSlotsMut<'c>,
        channel: usize,
    ) where
        'l: 'c,
    {
        let gc = &mut self.global_conf;
        let num_time_slots = usize::from(gc.qmf_domain_params.num_qmf_time_slots);
        let num_ov_slots = usize::from(gc.qmf_domain_params.num_qmf_ov_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);
        let wb_index_0 = usize::from(gc.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &mut gc.work_buffer[wb_index_0];
        let overlap_buffer = &mut self.input[channel].overlap_buffer;

        QmfDomain::create_pointer_array_for_qmf_slots_mut(
            overlap_buffer,
            work_buffer,
            qmf_slots,
            num_time_slots,
            num_proc_bands,
            num_ov_slots,
        );
    }

    /// Creates array of immutable slices (real and imag) out of overlap buffer and work buffer for
    /// given channel. Updates `qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `qmf_slots`: `QmfDomainQmfSlots` instances with default values.
    /// - `channel_0`: Channel number to be processed.
    /// - `channel_1`: Channel number to be processed.
    pub fn get_pointer_array_for_qmf_slots_two_channels<'l, 'c>(
        &'l self,
        qmf_slots: &mut [QmfDomainQmfSlots<'c>; 2],
        channel_0: usize,
        channel_1: usize,
    ) where
        'l: 'c,
    {
        self.get_pointer_array_for_qmf_slots(&mut qmf_slots[0], channel_0);
        self.get_pointer_array_for_qmf_slots(&mut qmf_slots[1], channel_1);
    }

    /// Creates array of mutable slices (real and imag) out of overlap buffer and work buffer for
    /// given channels. Updates `qmf_slots` with created slots.
    ///
    /// # Parameters
    ///
    /// - `qmf_slots`: `QmfDomainQmfSlotsMut` instances with default values.
    /// - `channel_0`: Channel number to be processed.
    /// - `channel_1`: Channel number to be processed.
    pub fn get_pointer_array_for_qmf_slots_two_channels_mut<'l, 'c>(
        &'l mut self,
        qmf_slots: &mut [QmfDomainQmfSlotsMut<'c>; 2],
        channel_0: usize,
        channel_1: usize,
    ) where
        'l: 'c,
    {
        let gc = &mut self.global_conf;
        let num_time_slots = usize::from(gc.qmf_domain_params.num_qmf_time_slots);
        let num_ov_slots = usize::from(gc.qmf_domain_params.num_qmf_ov_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);

        let qmf_slot_0 = &mut qmf_slots[0];

        let wb_index_0 = usize::from(gc.qmf_domain_params.work_buffer_index[channel_0]);
        let wb_index_1 = usize::from(gc.qmf_domain_params.work_buffer_index[channel_1]);

        let (wb_left, wb_right) = gc.work_buffer.split_at_mut(1);

        // Maximum number of channels is 2 in case of SBR parametric stereo.
        let (input_in_use, _input_not_in_use) = self.input.split_at_mut(2);
        let (input_left, input_right) = input_in_use.split_at_mut(1);

        let (work_buffer_0, work_buffer_1) = match (wb_index_0, wb_index_1) {
            (0, 1) => (&mut wb_left[0], &mut wb_right[0]),
            (1, 0) => (&mut wb_right[0], &mut wb_left[0]),
            _ => panic!("Both qmf channels can't use same workbuffer simultaneously"),
        };

        let (input_channel_0, input_channel_1) = match (channel_0, channel_1) {
            (0, 1) => (&mut input_left[0], &mut input_right[0]),
            (1, 0) => (&mut input_right[0], &mut input_left[0]),
            _ => panic!("Both qmf input channels can't be same"),
        };

        QmfDomain::create_pointer_array_for_qmf_slots_mut(
            &mut input_channel_0.overlap_buffer,
            work_buffer_0,
            qmf_slot_0,
            num_time_slots,
            num_proc_bands,
            num_ov_slots,
        );

        QmfDomain::create_pointer_array_for_qmf_slots_mut(
            &mut input_channel_1.overlap_buffer,
            work_buffer_1,
            &mut qmf_slots[1],
            num_time_slots,
            num_proc_bands,
            num_ov_slots,
        );
    }

    /// Creates array of immutable slices (real and imag) out of overlap buffer for given channel.
    /// Also updates `qmf_slots`.
    ///
    /// # Parameters
    ///
    /// - `global_conf`: Qmf domain global configuration.
    /// - `overlap_buffer`: Overlap buffer to create qmf slots.
    /// - `qmf_slots`: `QmfDomainQmfSlots` instance with default values.
    /// - `channel`: Channel number to be processed.
    fn create_pointer_array_for_qmf_slots<'l, 'c>(
        global_conf: &'l QmfDomainGc,
        overlap_buffer: &'c [f32],
        qmf_slots: &mut QmfDomainQmfSlots<'c>,
        channel: usize,
    ) where
        'l: 'c,
    {
        let start_of_wb_slots = usize::from(global_conf.qmf_domain_params.num_qmf_ov_time_slots);

        QmfDomain::set_pointer_array_for_overlap_buffer_area(
            global_conf,
            overlap_buffer,
            &mut qmf_slots.real,
            &mut qmf_slots.imag,
        );

        QmfDomain::set_pointer_array_for_work_buffer_area(
            global_conf,
            &mut qmf_slots.real[start_of_wb_slots..],
            &mut qmf_slots.imag[start_of_wb_slots..],
            channel,
        );
    }

    /// Creates array of mutable slices (real and imag) out of overlap buffer and work buffer.
    /// Also updates `qmf_slots`.
    ///
    /// # Parameters
    ///
    /// - `global_conf`: Qmf domain global configuration.
    /// - `overlap_buffer`: Overlap buffer to create qmf slots.
    /// - `work_buffer`: Work buffer to create qmf slots.
    /// - `qmf_slots`: `QmfDomainQmfSlotsMut` instance with default values.
    /// - `num_time_slots`: Number of qmf timeslots.
    /// - `num_proc_bands`: Number of processing bands per timeslot.
    /// - `start_of_wb_slots`: Number of overlap timeslot.
    fn create_pointer_array_for_qmf_slots_mut<'l, 'c>(
        overlap_buffer: &'l mut [f32],
        work_buffer: &'l mut [f32],
        qmf_slots: &mut QmfDomainQmfSlotsMut<'c>,
        num_time_slots: usize,
        num_proc_bands: usize,
        start_of_wb_slots: usize,
    ) where
        'l: 'c,
    {
        QmfDomain::set_pointer_array_for_overlap_buffer_area_mut(
            overlap_buffer,
            &mut qmf_slots.real,
            &mut qmf_slots.imag,
            start_of_wb_slots,
            num_proc_bands,
        );

        QmfDomain::set_pointer_array_for_work_buffer_area_mut(
            work_buffer,
            &mut qmf_slots.real[start_of_wb_slots..],
            &mut qmf_slots.imag[start_of_wb_slots..],
            num_time_slots,
            num_proc_bands,
        );
    }

    /// Copies filter states from ch_src to ch_dst.
    pub fn cp_filter_states(&mut self, ch_dst: usize, ch_src: usize) {
        let filt_states_src = self.output[ch_src].synthesis_fb.filter_states().into();

        self.output[ch_dst]
            .synthesis_fb
            .set_filter_states(filt_states_src);
    }

    /// Performs analysis filtering on `time_in` input samples (time domain).
    /// Also updates `real_slots` and `imag_slots`.
    ///
    /// # Parameters
    ///
    /// - `real_slots`: Slices of real part of output data slot.
    /// - `imag_slots`: Slices of imaginary part of output data slot.
    /// - `time_in`: Input time-domain signal for qmf analysis filtering.
    /// - `channel`: Channel number to be processed.
    pub fn qmf_analysis_filtering_cplx<'l>(
        &mut self,
        real_slots: &mut [&'l mut [f32]],
        imag_slots: &mut [&'l mut [f32]],
        time_in: &[f32],
        channel: usize,
    ) {
        let analysis_fb = &mut self.input[channel].analysis_fb;

        analysis_fb.analysis_filtering(real_slots, imag_slots, time_in);
    }

    /// Performs analysis filtering on `time_in` input samples (time domain).
    ///
    /// # Parameters
    ///
    /// - `time_in`: Input time-domain signal for qmf analysis filtering.
    /// - `channel`: Channel number to be processed.
    pub fn qmf_analysis_filtering(&mut self, time_in: &[f32], channel: usize) {
        let mut curr_qmf_slots = QmfDomainCurQmfSlots::default();
        let num_time_slots = usize::from(self.global_conf.qmf_domain_params.num_qmf_time_slots);

        let analysis_fb = &mut self.input[channel].analysis_fb;
        let gc = &mut self.global_conf;

        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);
        let wb_index = usize::from(gc.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &mut gc.work_buffer[wb_index];

        QmfDomain::set_pointer_array_for_work_buffer_area_mut(
            work_buffer,
            &mut curr_qmf_slots.real,
            &mut curr_qmf_slots.imag,
            num_time_slots,
            num_proc_bands,
        );

        analysis_fb.analysis_filtering(
            &mut curr_qmf_slots.real[..num_time_slots],
            &mut curr_qmf_slots.imag[..num_time_slots],
            time_in,
        );
    }

    /// Performs synthesis filtering on QMF input data.
    ///
    /// # Parameters
    ///
    /// - `time_out`: Output samples in time domain.
    /// - `ov_usb`: Number of Upper side bands in qmf filter.
    /// - `channel`: Channel number to be processed.
    pub fn qmf_synthesis_filtering(
        &mut self,
        time_out: &mut [f32],
        ov_usb: &mut u8,
        channel: usize,
    ) {
        let usb_hdr = *ov_usb;
        let usb_fb = self.output[channel].synthesis_fb.num_hf_subbands();

        if usb_fb < usb_hdr {
            let usb_value = usb_hdr.min(self.output[channel].synthesis_fb.num_subbands());
            self.output[channel]
                .synthesis_fb
                .set_num_hf_subbands(usb_value);
        }

        let overlap_buffer = &self.input[channel].overlap_buffer;
        let gc = &self.global_conf;
        let num_qmf_time_slots = usize::from(gc.qmf_domain_params.num_qmf_time_slots);

        let mut qmf_slots = QmfDomainQmfSlots::default();

        QmfDomain::create_pointer_array_for_qmf_slots(gc, overlap_buffer, &mut qmf_slots, channel);

        let synthesis_fb = &mut self.output[channel].synthesis_fb;
        synthesis_fb.synthesis_filtering(
            &qmf_slots.real[..num_qmf_time_slots],
            &qmf_slots.imag[..num_qmf_time_slots],
            time_out,
        );

        // Restore saved value.
        synthesis_fb.set_num_hf_subbands(usb_fb);
        *ov_usb = usb_fb;
    }

    /// Copies `HBE` data to `QMF` for given channel.
    ///
    /// # Parameters
    ///
    /// - `real_slots`: Reference to array of slices for qmf real data slots.
    /// - `imag_slots`: Reference to array of slices for qmf imaginary data slots.
    /// - `channel`: Channel number to be processed.
    pub fn hbe_to_qmf_data(
        &mut self,
        real_slots: &[&[f32]],
        imag_slots: &[&[f32]],
        channel: usize,
    ) {
        let mut curr_qmf_slots = QmfDomainCurQmfSlots::default();

        let bands: usize = self.num_bands_analysis().into();
        let gc = &mut self.global_conf;
        let num_qmf_time_slots: usize = usize::from(gc.qmf_domain_params.num_qmf_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);

        let wb_index = usize::from(gc.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &mut gc.work_buffer[wb_index];

        QmfDomain::set_pointer_array_for_work_buffer_area_mut(
            work_buffer,
            &mut curr_qmf_slots.real,
            &mut curr_qmf_slots.imag,
            num_qmf_time_slots,
            num_proc_bands,
        );

        for (curr_real, curr_imag, real, imag) in izip!(
            curr_qmf_slots.real.iter_mut(),
            curr_qmf_slots.imag.iter_mut(),
            real_slots.iter(),
            imag_slots.iter()
        )
        .take(num_qmf_time_slots)
        {
            curr_real[..bands].copy_from_slice(&real[..bands]);
            curr_imag[..bands].copy_from_slice(&imag[..bands]);
        }
    }

    /// Clears subbands above given `bands` value in each qmf slot.
    ///
    /// # Parameters
    ///
    /// - `channel`: Channel number to be processed.
    pub fn clear_hbe_qmf_data(&mut self, channel: usize) {
        let mut curr_qmf_slots = QmfDomainCurQmfSlots::default();

        let bands: usize = self.num_bands_analysis().into();
        let gc = &mut self.global_conf;
        let num_qmf_time_slots: usize = usize::from(gc.qmf_domain_params.num_qmf_time_slots);
        let num_proc_bands = usize::from(gc.qmf_domain_params.num_qmf_proc_bands);

        let wb_index = usize::from(gc.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &mut gc.work_buffer[wb_index];

        QmfDomain::set_pointer_array_for_work_buffer_area_mut(
            work_buffer,
            &mut curr_qmf_slots.real,
            &mut curr_qmf_slots.imag,
            num_qmf_time_slots,
            num_proc_bands,
        );

        for (curr_real, curr_imag) in izip!(
            curr_qmf_slots.real.iter_mut(),
            curr_qmf_slots.imag.iter_mut(),
        )
        .take(num_qmf_time_slots)
        {
            curr_real[bands..QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS].fill(0.0);
            curr_imag[bands..QMF_DOMAIN_MAX_SYNTHESIS_QMF_BANDS].fill(0.0);
        }
    }

    /// Copies `QMF` data to `lPC states` for given channel.
    ///
    /// # Parameters
    ///
    /// - `cplx_output`: LPC states buffer.
    /// - `is_usac`: Flag to indicate `USAC` case.
    /// - `lpc_order`: Order of the LPC filter.
    /// - `channel`: Channel number to be processed.
    pub fn qmf_data_to_lpc_states(
        &self,
        cplx_output: &mut [[Complex<f32>; QMF_DOMAIN_ANALYSIS_QMF_BANDS_32 as usize];
                 QMF_DOMAIN_MAX_LPC_ORDER + QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
        is_usac: bool,
        lpc_order: usize,
        channel: usize,
    ) {
        let num_qmf_time_slots = usize::from(self.global_conf.qmf_domain_params.num_qmf_time_slots);
        let num_qmf_ov_time_slots =
            usize::from(self.global_conf.qmf_domain_params.num_qmf_ov_time_slots);

        let len = if is_usac {
            self.input[channel].analysis_fb.num_subbands()
        } else {
            self.input[channel].analysis_fb.num_lf_subbands()
        } as usize;

        let wb_index = usize::from(self.global_conf.qmf_domain_params.work_buffer_index[channel]);
        let work_buffer = &self.global_conf.work_buffer[wb_index];
        debug_assert!(!work_buffer.is_empty());

        let input = &self.input[channel];

        for (cplx_row, re_start, im_start) in izip!(
            cplx_output.iter_mut(),
            input.qmf_slots_real_offsets[(num_qmf_time_slots - lpc_order)..].iter(),
            input.qmf_slots_imag_offsets[(num_qmf_time_slots - lpc_order)..].iter()
        )
        .take(num_qmf_ov_time_slots + lpc_order)
        {
            let wb_real = &work_buffer[usize::from(*re_start)..usize::from(*re_start) + len];
            let wb_imag = &work_buffer[usize::from(*im_start)..usize::from(*im_start) + len];

            for (cplx_data, re, im) in
                izip!(cplx_row.iter_mut(), wb_real.iter(), wb_imag.iter()).take(len)
            {
                cplx_data.re = *re;
                cplx_data.im = *im;
            }
        }
    }
}

pub const QMF_DOMAIN_MAX_LPC_ORDER: usize = 2;
const EMPTY_ARRAY: &[f32] = &[];

#[repr(C)]
#[derive(Debug)]
pub struct QmfDomainOvQmfSlotsMut<'a> {
    pub real: [&'a mut [f32]; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
    pub imag: [&'a mut [f32]; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
}

impl Default for QmfDomainOvQmfSlotsMut<'_> {
    fn default() -> Self {
        Self {
            real: array::from_fn(|_| [].as_mut_slice()),
            imag: array::from_fn(|_| [].as_mut_slice()),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct QmfDomainOvQmfSlots<'a> {
    real: [&'a [f32]; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
    imag: [&'a [f32]; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
}

impl Default for QmfDomainOvQmfSlots<'_> {
    fn default() -> Self {
        Self {
            real: [EMPTY_ARRAY; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
            imag: [EMPTY_ARRAY; QMF_DOMAIN_MAX_OV_TIMESLOTS as usize],
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct QmfDomainCurQmfSlots<'b> {
    real: [&'b mut [f32]; QMF_DOMAIN_MAX_TIMESLOTS as usize],
    imag: [&'b mut [f32]; QMF_DOMAIN_MAX_TIMESLOTS as usize],
}

impl Default for QmfDomainCurQmfSlots<'_> {
    fn default() -> Self {
        Self {
            real: array::from_fn(|_| [].as_mut_slice()),
            imag: array::from_fn(|_| [].as_mut_slice()),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct QmfDomainQmfSlotsMut<'c> {
    pub real: [&'c mut [f32]; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
    pub imag: [&'c mut [f32]; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
}

impl Default for QmfDomainQmfSlotsMut<'_> {
    fn default() -> Self {
        Self {
            real: array::from_fn(|_| [].as_mut_slice()),
            imag: array::from_fn(|_| [].as_mut_slice()),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct QmfDomainQmfSlots<'c> {
    pub real: [&'c [f32]; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
    pub imag: [&'c [f32]; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
}

impl Default for QmfDomainQmfSlots<'_> {
    fn default() -> Self {
        Self {
            real: [EMPTY_ARRAY; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
            imag: [EMPTY_ARRAY; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
        }
    }
}
