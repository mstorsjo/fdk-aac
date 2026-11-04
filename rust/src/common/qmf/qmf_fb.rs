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
//! QMF filterbank
//!
//! The QMF is used by the MPEG Surround and SBR decoder.
use super::super::dct;
use super::qmf_tables;
pub use bitflags::bitflags;
use itertools::izip;

pub const QMF_NO_POLY: usize = 5;
/// Maximum number of QMF analysis subbands
pub const QMF_MAX_ANALYSIS_SUBBANDS: usize = 64;
/// Maximum number of QMF synthesis subbands
pub const QMF_MAX_SYNTHESIS_SUBBANDS: usize = 64;
/// Number of synthesis filter states
const NUM_SYNTH_FILTER_STATES: usize = 9;

bitflags! {

    #[derive(Copy, Clone, Debug, Default, PartialEq)]
    #[repr(C)]
    /// QMF filter bank flags.
    pub struct QmfFlags: u32 {
        /// Low Power mode flag
        const LP = 1;
        /// Filter is not symmetric. This flag is set internally in the QMF initialization as
        /// required.
        // DO NOT PASS THIS FLAG TO `init_analysis_filterbank()` or `synthesis_filterbank()`.
        const NONSYMMETRIC = 2;
        /// Complex Low Delay Filter Bank (or std symmetric filter bank).
        const CLDFB = 4;
        /// Flag indicating that the states should be kept.
        const KEEP_STATES = 8;
        /// Complex Low Delay Filter Bank used in MPEG Surround Encoder.
        const MPSLDFB = 16;
        /// Complex Low Delay Filter Bank used in MPEG Surround Encoder allows an
        /// optimized calculation of the modulation in `forward_modulation_hq()`.
        const MPSLDFB_OPTIMIZE_MODULATION = 32;
        /// Flag to indicate HE-AAC down-sampled SBR mode (decoder) -> adapt analysis post
        /// twiddling.
        const DOWNSAMPLED = 64;
    }
}

/// QMF operating modes
#[derive(PartialEq, Copy, Clone, Debug)]
pub enum QmfMode {
    Analysis = 0,
    Synthesis,
}

#[derive(Debug)]
pub struct QmfError;

impl TryFrom<i32> for QmfMode {
    type Error = QmfError;
    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(QmfMode::Analysis),
            1 => Ok(QmfMode::Synthesis),
            _ => Err(QmfError {}),
        }
    }
}

#[derive(Default, Clone, Debug)]
#[repr(C)]
/// QMF filter bank structure
pub struct QmfFilterBank {
    /// Reference to filter coefficients.
    filter_coeff: Option<&'static [f32]>,
    /// Reference to buffer of filter states.
    pub filter_states: Box<[f32]>,
    /// Cosine modulation table.
    cos_table: Option<&'static [f32]>,
    /// Sine modulation table.
    sin_table: Option<&'static [f32]>,
    /// Size of prototype filter.
    filter_size: u16,
    /// Total number of QMF subbands.
    num_subbands: u8,
    /// Number of time slots per frame.
    num_time_slots: u8,
    /// Number of low freq. QMF subbands.
    num_lf_subbands: u8,
    /// Number of high freq. QMF subbands.
    num_hf_subbands: u8,
    /// QMF filter bank flags.
    flags: QmfFlags,
    /// Analysis factor.
    ana_factor: f32,
    /// Gain output data (syn only) (init with 1.0 to ignore).
    out_gain: f32,
    /// stride factor of polyphase filters.
    stride_poly: u8,
    /// Holds filt. state slot indices (Analysis only).
    state_slots: [u16; 2 * QMF_NO_POLY],
}

impl QmfFilterBank {
    /// Creates a new `QmfFilterBank`.
    ///
    /// # Parameters
    /// - `len_filter_states`: length of `filter_states[]` internal buffer. The lenght should be big
    ///   enough to store multiple slots of `self.num_subbands()` data.
    ///
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_ANALYSIS_SUBBANDS * (2 * QMF_NO_POLY);
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// ```
    pub fn new(len_filter_states: usize) -> QmfFilterBank {
        QmfFilterBank {
            filter_states: vec![0.0f32; len_filter_states].into_boxed_slice(),
            ..Default::default()
        }
    }

    /// Returns immutable reference to filter states of `QmfFilterBank`.
    pub fn filter_states(&self) -> &[f32] {
        &self.filter_states
    }

    /// Returns mutable reference to filter states of `QmfFilterBank`.
    pub fn filter_states_mut(&mut self) -> &mut [f32] {
        &mut self.filter_states
    }

    /// Returns frame size of `QmfFilterBank`.
    pub fn filter_bank_frame_size(&self) -> usize {
        usize::from(self.num_time_slots) * usize::from(self.num_subbands)
    }

    /// Returns number of lower subbands of `QmfFilterBank`.
    pub fn num_lf_subbands(&self) -> u8 {
        self.num_lf_subbands
    }

    /// Returns number of upper subbands of `QmfFilterBank`.
    pub fn num_hf_subbands(&self) -> u8 {
        self.num_hf_subbands
    }

    /// Returns number of columns in `QmfFilterBank`.
    pub fn num_time_slots(&self) -> u8 {
        self.num_time_slots
    }

    /// Returns number of subbands in `QmfFilterBank`.
    pub fn num_subbands(&self) -> u8 {
        self.num_subbands
    }

    /// Returns output gain of synthesis `QmfFilterBank`.
    pub fn out_gain(&self) -> f32 {
        self.out_gain
    }

    /// Sets filter states of `QmfFilterBank`.
    pub fn set_filter_states(&mut self, states: Box<[f32]>) {
        self.filter_states = states;
    }

    /// Sets number of lower subbands of `QmfFilterBank`.
    pub fn set_num_lf_subbands(&mut self, num_lf_subbands: u8) {
        self.num_lf_subbands = num_lf_subbands;
    }

    /// Sets number of upper subbands of `QmfFilterBank`.
    pub fn set_num_hf_subbands(&mut self, num_hf_subbands: u8) {
        self.num_hf_subbands = num_hf_subbands;
    }

    /// Sets number of columns of `QmfFilterBank`.
    pub fn set_num_time_slots(&mut self, num_time_slots: u8) {
        self.num_time_slots = num_time_slots;
    }

    /// Sets cosine table of `QmfFilterBank`.
    pub fn set_cos_table(&mut self, cos_table: Option<&'static [f32]>) {
        self.cos_table = cos_table;
    }

    /// Sets sine table of `QmfFilterBank`.
    pub fn set_sin_table(&mut self, sin_table: Option<&'static [f32]>) {
        self.sin_table = sin_table;
    }

    /// Changes output gain of synthesis `QmfFilterBank`.
    ///
    /// # Parameters
    /// - `gain`: Gain value of qmf filter output
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_SYNTHESIS_SUBBANDS * (2 * QMF_NO_POLY);
    /// let gain = 0.5_f32;
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.set_output_gain(gain);
    /// ```
    pub fn set_output_gain(&mut self, gain: f32) {
        self.out_gain = gain;
    }

    /// Copies the filter states from src to self.
    pub fn cp_filter_states(&mut self, src: &Self) {
        self.filter_states.copy_from_slice(&src.filter_states);
    }

    /// Resets all qmf filter bank structure elements with the exception of `filter_states[]` and
    /// `state_slots[]`.
    fn reset(&mut self) {
        self.filter_coeff = None;
        self.cos_table = None;
        self.sin_table = None;
        self.filter_size = 0;
        self.num_subbands = 0;
        self.num_time_slots = 0;
        self.num_lf_subbands = 0;
        self.num_hf_subbands = 0;
        self.flags = QmfFlags::empty();
        self.stride_poly = 0;
        self.ana_factor = 0.0_f32;
        self.out_gain = 0.0_f32;
    }

    /// Performs initialization of qmf filter bank instance.
    /// The `init()` function initializes an instance of qmf filter bank.
    /// For synthesis filter bank, `qmf_mode` should be 'QmfMode::Synthesis'
    /// and for analysis it should be 'QmfMode::Analysis'.
    ///
    /// Note: `QmfFilterBank::num_lf_subbands()` can be greater than `QmfFilterBank::num_subbands`
    /// in case of implicit resampling (USAC with reduced 3/4 core frame length).
    ///
    /// # Parameters
    /// - `num_time_slots`: Number of timeslots per frame
    /// - `num_lf_subbands`: Number of lower side bands in qmf filter
    /// - `num_hf_subbands`: Number of Upper side bands in qmf filter
    /// - `num_subbands`: Number of qmf bands/subbands
    /// - `flags`: Flag switch to set operational mode of qmf filter
    /// - `qmf_mode`: Enum to enable init for synthesis mode (or) analysis mode
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_ANALYSIS_SUBBANDS * (2 * QMF_NO_POLY);
    /// let num_subbands = u8::try_from(QMF_MAX_ANALYSIS_SUBBANDS / 2).unwrap();
    /// let num_lf_subbands = num_subbands / 2;
    /// let num_hf_subbands = num_subbands / 2;
    /// let num_time_slots = 2;
    /// let flags = QmfFlags::CLDFB | QmfFlags::KEEP_STATES;
    /// let qmf_mode = QmfMode::Analysis;
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.init(
    ///     num_time_slots,
    ///     num_lf_subbands,
    ///     num_hf_subbands,
    ///     num_subbands,
    ///     flags,
    ///     qmf_mode,
    /// );
    /// ```
    pub fn init(
        &mut self,
        num_time_slots: u8,  // Number of timeslots per frame
        num_lf_subbands: u8, // Lower end of QMF frequency range
        num_hf_subbands: u8, // Upper end of QMF frequency range
        num_subbands: u8,    // Number of subbands (bands)
        flags: QmfFlags,     // flags
        qmf_mode: QmfMode,   // qmf operating mode
    ) -> i32 {
        self.reset();

        self.ana_factor = 1.0_f32;
        let mut flags = flags;

        if !flags.contains(QmfFlags::MPSLDFB) {
            if flags.contains(QmfFlags::CLDFB) {
                flags.insert(QmfFlags::NONSYMMETRIC);

                self.stride_poly = 1;
                match num_subbands {
                    64 => {
                        self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS64_CLDFB);
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN64_CLDFB);
                        self.filter_coeff = Some(&qmf_tables::QMF_CLDFB_640);
                        self.ana_factor = 1.0_f32;
                        self.filter_size = 640;
                    }
                    32 => {
                        self.cos_table = {
                            if qmf_mode == QmfMode::Synthesis {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS32_CLDFB_SYN)
                            } else {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS32_CLDFB_ANA)
                            }
                        };
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN32_CLDFB);
                        self.filter_coeff = Some(&qmf_tables::QMF_CLDFB_320);
                        self.ana_factor = 2.0_f32;
                        self.filter_size = 320;
                    }
                    16 => {
                        self.cos_table = {
                            if qmf_mode == QmfMode::Synthesis {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS16_CLDFB_SYN)
                            } else {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS16_CLDFB_ANA)
                            }
                        };
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN16_CLDFB);
                        self.filter_coeff = Some(&qmf_tables::QMF_CLDFB_160);
                        self.ana_factor = 4.0_f32;
                        self.filter_size = 160;
                    }
                    8 => {
                        self.cos_table = {
                            if qmf_mode == QmfMode::Synthesis {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS8_CLDFB_SYN)
                            } else {
                                Some(&qmf_tables::QMF_PHASESHIFT_COS8_CLDFB_ANA)
                            }
                        };
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN8_CLDFB);
                        self.filter_coeff = Some(&qmf_tables::QMF_CLDFB_80);
                        self.ana_factor = 8.0_f32;
                        self.filter_size = 80;
                    }
                    _ => {
                        return -1;
                    }
                }
            } else {
                match num_subbands {
                    64 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT640);
                        self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS64);
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN64);
                        self.stride_poly = 1;
                        self.filter_size = 640;
                        self.ana_factor = 1.0_f32;
                    }
                    40 => {
                        if qmf_mode == QmfMode::Analysis {
                            self.filter_coeff = Some(&qmf_tables::QMF_PFILT400);
                            self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS40);
                            self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN40);

                            self.stride_poly = 1;
                            self.filter_size = u16::from(num_subbands) * 10;
                            self.ana_factor = 2.0_f32;
                        }
                    }
                    32 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT640);
                        if flags.contains(QmfFlags::DOWNSAMPLED) {
                            self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS_DOWNSAMP32);
                            self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN_DOWNSAMP32);
                        } else {
                            self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS32);
                            self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN32);
                        }
                        self.stride_poly = 2;
                        self.filter_size = 640;
                        self.ana_factor = 2.0_f32;
                    }
                    24 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT240);
                        self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS24);
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN24);
                        self.stride_poly = 1;
                        self.filter_size = 240;
                        self.ana_factor = 4.0_f32;
                    }
                    20 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT200);
                        self.stride_poly = 1;
                        self.filter_size = 200;
                        self.ana_factor = 4.0_f32;
                    }
                    16 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT640);
                        self.cos_table = Some(&qmf_tables::QMF_PHASESHIFT_COS16);
                        self.sin_table = Some(&qmf_tables::QMF_PHASESHIFT_SIN16);
                        self.stride_poly = 4;
                        self.filter_size = 640;
                        self.ana_factor = 4.0_f32;
                    }
                    12 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT120);
                        self.stride_poly = 1;
                        self.filter_size = 120;
                        self.ana_factor = 8.0_f32;
                    }
                    8 => {
                        self.filter_coeff = Some(&qmf_tables::QMF_PFILT640);
                        self.stride_poly = 8;
                        self.filter_size = 640;
                        self.ana_factor = 8.0_f32;
                    }
                    _ => {
                        return -1;
                    }
                } // end of match
            }
        } else {
            flags.insert(QmfFlags::NONSYMMETRIC);
            flags.insert(QmfFlags::MPSLDFB_OPTIMIZE_MODULATION);

            self.cos_table = None;
            self.sin_table = None;
            self.stride_poly = 1;

            match num_subbands {
                64 => {
                    self.filter_coeff = Some(&qmf_tables::QMF_MPSLDFB_640);
                    self.ana_factor = 1.0_f32;
                    self.filter_size = 640;
                }
                32 => {
                    self.filter_coeff = Some(&qmf_tables::QMF_MPSLDFB_320);
                    self.ana_factor = 2.0_f32;
                    self.filter_size = 320;
                }
                _ => {
                    return -1;
                }
            }
        }

        self.flags = flags;

        self.num_subbands = num_subbands;
        self.num_time_slots = num_time_slots;

        self.num_lf_subbands = num_lf_subbands.min(self.num_subbands);
        self.num_hf_subbands = {
            if qmf_mode == QmfMode::Synthesis {
                num_hf_subbands.min(self.num_subbands)
            } else {
                num_hf_subbands
            }
        };

        self.out_gain = 1.0_f32;

        if qmf_mode == QmfMode::Analysis {
            if !flags.contains(QmfFlags::KEEP_STATES) && (!self.filter_states.is_empty()) {
                // memclear the filter_states
                let len = (2 * QMF_NO_POLY - 1) * usize::from(self.num_subbands);
                self.filter_states[..len].fill(0.0_f32);
                self.init_analysis_state_slots();
            }
        } else if !flags.contains(QmfFlags::KEEP_STATES) && (!self.filter_states.is_empty()) {
            self.filter_states.fill(0.0_f32);
        }

        0 // Returning zero on success.
    }

    /// Initializes the analysis state slot values.
    // Only re-init it if state buffer memory is cleared.
    #[inline(always)]
    fn init_analysis_state_slots(&mut self) {
        for (slot_index, slot) in self.state_slots.iter_mut().enumerate() {
            *slot = slot_index as u16 * u16::from(self.num_subbands)
        }
    }

    /// Performs analysis filtering on `time_in` input samples (time domain).
    /// It takes `self.num_time_slots()` input slots.
    /// The length of each slot is equal to `self.num_subbands()`.
    /// It provides `self.num_time_slots()` QMF data slots to the output.
    /// Each output slot is containing `self.num_subbands()` complex data.
    /// The output slots are split into separated real and imaginary data buffers.
    ///
    /// # Parameters
    /// - `qmf_buffer_real`: Slices of real part of output data slot
    /// - `qmf_buffer_imag`: Slices of imaginary part of output data slot
    /// - `time_in`: Input samples in time domain
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_ANALYSIS_SUBBANDS * (2 * QMF_NO_POLY);
    /// let num_subbands = QMF_MAX_ANALYSIS_SUBBANDS / 2;
    /// let num_lf_subbands = num_subbands / 2;
    /// let num_hf_subbands = num_subbands / 2;
    /// let num_time_slots = 2;
    /// let flags = QmfFlags::CLDFB;
    /// let qmf_mode = QmfMode::Analysis;
    ///
    /// let mut time_in = vec![0.0f32; num_time_slots * num_subbands];
    ///
    /// let mut real_out_qmf_slot = vec![0.0f32; num_subbands];
    /// let mut imag_out_qmf_slot = vec![0.0f32; num_subbands];
    /// let mut real_out_qmf: Vec<&mut [f32]> = Vec::new();
    /// let mut imag_out_qmf: Vec<&mut [f32]> = Vec::new();
    ///
    /// for (re_slot, im_slot) in real_out_qmf_slot
    ///     .chunks_exact_mut(num_subbands)
    ///     .zip(imag_out_qmf_slot.chunks_exact_mut(num_subbands))
    ///     .take(num_time_slots)
    /// {
    ///     real_out_qmf.push(re_slot);
    ///     imag_out_qmf.push(im_slot);
    /// }
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.init(
    ///     u8::try_from(num_time_slots).unwrap(),
    ///     u8::try_from(num_lf_subbands).unwrap(),
    ///     u8::try_from(num_hf_subbands).unwrap(),
    ///     u8::try_from(num_subbands).unwrap(),
    ///     flags,
    ///     qmf_mode,
    /// );
    /// qmf.analysis_filtering(&mut real_out_qmf, &mut imag_out_qmf, &time_in[..]);
    /// ```
    pub fn analysis_filtering(
        &mut self,
        qmf_buffer_real: &mut [&mut [f32]],
        qmf_buffer_imag: &mut [&mut [f32]],
        time_in: &[f32],
    ) {
        let num_timeslots = usize::from(self.num_time_slots);

        debug_assert!(
            time_in.len() >= num_timeslots * usize::from(self.num_subbands),
            "Analysis input buffer size is too small."
        );

        // Loop over number of time slots.
        for (time_in_slot, real_slot, imag_slot) in izip!(
            time_in.chunks_exact(usize::from(self.num_subbands)),
            qmf_buffer_real.iter_mut(),
            qmf_buffer_imag.iter_mut()
        )
        .take(num_timeslots)
        {
            self.analysis_filtering_slot(real_slot, imag_slot, time_in_slot);
        }
    }

    /// Performs one QMF slot analysis on a single `time_in` slot.
    /// A slot has `self.num_subbands()` samples.
    /// It provides the real part of the subband samples in `real_slot`
    /// and the imaginary part in `imag_slot`.
    ///
    /// # Parameters
    /// - `real_slot`: Real part of output samples
    /// - `imag_slot`: Imaginary part of output samples
    /// - `time_in`: Input samples in time domain
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_ANALYSIS_SUBBANDS * (2 * QMF_NO_POLY);
    /// let num_subbands = QMF_MAX_ANALYSIS_SUBBANDS / 2;
    /// let num_lf_subbands = num_subbands / 2;
    /// let num_hf_subbands = num_subbands / 2;
    /// let num_time_slots = 1;
    /// let flags = QmfFlags::CLDFB;
    /// let qmf_mode = QmfMode::Analysis;
    ///
    /// let time_in = &vec![0.0_f32; num_time_slots * num_subbands];
    /// let out_qmf_real_slot = &mut vec![0.0_f32; num_subbands];
    /// let out_qmf_imag_slot = &mut vec![0.0_f32; num_subbands];
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.init(
    ///     u8::try_from(num_time_slots).unwrap(),
    ///     u8::try_from(num_lf_subbands).unwrap(),
    ///     u8::try_from(num_hf_subbands).unwrap(),
    ///     u8::try_from(num_subbands).unwrap(),
    ///     flags,
    ///     qmf_mode,
    /// );
    /// qmf.analysis_filtering_slot(out_qmf_real_slot, out_qmf_imag_slot, time_in);
    /// ```
    pub fn analysis_filtering_slot(
        &mut self,
        real_slot: &mut [f32],
        imag_slot: &mut [f32],
        time_in: &[f32],
    ) {
        let num_subbands = usize::from(self.num_subbands);

        // Create temp work buffer for qmf analysis filtering.
        let mut work_buffer = [0.0_f32; 2 * QMF_MAX_ANALYSIS_SUBBANDS];

        // Feed time signal into oldest state_slots.
        {
            let copy_offset = self.state_slots[QMF_NO_POLY * 2 - 1] as usize;
            self.filter_states[copy_offset..(num_subbands + copy_offset)]
                .copy_from_slice(&time_in[..num_subbands]);
        }

        if self.flags.contains(QmfFlags::NONSYMMETRIC) {
            self.analysis_prototype_firslot_nonsymmetric(&mut work_buffer);
        } else {
            self.analysis_prototype_firslot_symmetric(&mut work_buffer);
        }

        self.forward_modulation_hq(&work_buffer, real_slot, imag_slot);

        // Update analysis state slots.
        self.update_analysis_state_slots();
    }

    /// Updates state buffer slot.
    #[inline(always)]
    fn update_analysis_state_slots(&mut self) {
        let slot_backup = self.state_slots[0];
        self.state_slots.copy_within(1..2 * QMF_NO_POLY, 0);
        self.state_slots[2 * QMF_NO_POLY - 1] = slot_backup;
    }

    /// Returns `filter states` slot (sub-slice) from `self.filter_states[]` based on given
    /// slot index `num_slot`. The value of 'num_slot' should be in range 0 to 9.
    #[inline(always)]
    fn get_filter_states_slot(&self, num_slot: usize) -> &[f32] {
        &self.filter_states[usize::from(self.state_slots[num_slot])
            ..usize::from(self.state_slots[num_slot]) + usize::from(self.num_subbands)]
    }

    /// Performs analysis prototype symmetric filtering on a single slot of data from
    /// `self.filter_states[]`.
    /// The filter processes `self.num_subbands()` of time domain data internally
    /// and generates 2 filtered copies of `self.num_subbands()` time domain output samples.
    ///
    /// # Parameters
    /// - `output`: Output samples. Buffer size is `>= 2 * self.num_subbands()`.
    fn analysis_prototype_firslot_symmetric(&mut self, output: &mut [f32]) {
        let num_subbands = usize::from(self.num_subbands);

        let s0 = self.get_filter_states_slot(0);
        let s1 = self.get_filter_states_slot(1);
        let s2 = self.get_filter_states_slot(2);
        let s3 = self.get_filter_states_slot(3);
        let s4 = self.get_filter_states_slot(4);
        let s5 = self.get_filter_states_slot(5);
        let s6 = self.get_filter_states_slot(6);
        let s7 = self.get_filter_states_slot(7);
        let s8 = self.get_filter_states_slot(8);
        let s9 = self.get_filter_states_slot(9);

        let (left_out, right_out) = output[..(num_subbands << 1)].split_at_mut(num_subbands);
        // 127
        // Read first 5 coeffs.
        let flt = &self.filter_coeff.unwrap()[0..5];
        let tmp = flt[4] * s1[s1.len() - 1];
        let mut accu = flt[0] * s9[s9.len() - 1];
        accu += flt[1] * s7[s7.len() - 1];
        accu += flt[2] * s5[s5.len() - 1];
        accu += flt[3] * s3[s3.len() - 1];
        left_out[0] = accu + tmp;

        let filter_offset: usize = usize::from(self.stride_poly) * QMF_NO_POLY;
        let filter_coeff = &self.filter_coeff.unwrap()[filter_offset..];
        for (
            flt,
            s0_val,
            s2_val,
            s4_val,
            s6_val,
            s8_val,
            s1_val,
            s3_val,
            s5_val,
            s7_val,
            s9_val,
            lo,
            ro,
        ) in izip!(
            filter_coeff.chunks_exact(filter_offset),
            s0.iter(),
            s2.iter(),
            s4.iter(),
            s6.iter(),
            s8.iter(),
            s1.iter().rev().skip(1),
            s3.iter().rev().skip(1),
            s5.iter().rev().skip(1),
            s7.iter().rev().skip(1),
            s9.iter().rev().skip(1),
            left_out[1..].iter_mut(),        // left[1..num_subbands]
            right_out[1..].iter_mut().rev()  // right[0..num_subbands-1]
        )
        .take(num_subbands - 1)
        {
            // 0..62
            let tmp1 = flt[4] * *s8_val;
            let mut accu = flt[0] * *s0_val;
            accu += flt[1] * *s2_val;
            accu += flt[2] * *s4_val;
            accu += flt[3] * *s6_val;
            *ro = accu + tmp1;

            // 126..64
            let tmp2 = flt[4] * *s1_val;
            let mut acc = flt[0] * *s9_val;
            acc += flt[1] * *s7_val;
            acc += flt[2] * *s5_val;
            acc += flt[3] * *s3_val;
            *lo = acc + tmp2;
        }
        // 63
        let flt = &self.filter_coeff.unwrap()[num_subbands * filter_offset..];
        let tmp = flt[4] * s8[s8.len() - 1];
        let mut accu = flt[0] * s0[s0.len() - 1];
        accu += flt[1] * s2[s2.len() - 1];
        accu += flt[2] * s4[s4.len() - 1];
        accu += flt[3] * s6[s6.len() - 1];
        right_out[0] = accu + tmp;
    }

    /// Performs analysis prototype non-symmetric filtering on a single slot of data from
    /// `self.filter_states[]`.
    /// The filter processes `self.num_subbands()` of time domain data internally
    /// and generates 2 filtered copies of `self.num_subbands()` time domain output samples.
    ///
    /// # Parameters
    /// - `output`: Output samples. Buffer size is `>= 2 * self.num_subbands()`.
    fn analysis_prototype_firslot_nonsymmetric(&mut self, output: &mut [f32]) {
        let filter = self.filter_coeff.unwrap();
        let num_subbands = usize::from(self.num_subbands);
        let filt_offset = usize::from(self.stride_poly - 1) * QMF_NO_POLY;

        let s0 = self.get_filter_states_slot(0);
        let s2 = self.get_filter_states_slot(2);
        let s4 = self.get_filter_states_slot(4);
        let s6 = self.get_filter_states_slot(6);
        let s8 = self.get_filter_states_slot(8);

        let (out_left, out_right) = output[..num_subbands << 1].split_at_mut(num_subbands);
        let filt_chunk_size = filt_offset + QMF_NO_POLY;
        let filt_left = &filter[filt_offset..filt_offset + (filt_chunk_size * num_subbands)];
        let filt_right = &filter[filt_offset + (filt_chunk_size * num_subbands)..];

        for (flt, s0_val, s2_val, s4_val, s6_val, s8_val, out) in izip!(
            filt_left.chunks_exact(filt_chunk_size),
            s0.iter(),
            s2.iter(),
            s4.iter(),
            s6.iter(),
            s8.iter(),
            out_right.iter_mut().rev()
        )
        .take(num_subbands)
        {
            // Perform FIR-Filter
            let tmp = flt[4] * *s8_val;
            let mut accu = flt[0] * *s0_val;
            accu += flt[1] * *s2_val;
            accu += flt[2] * *s4_val;
            accu += flt[3] * *s6_val;
            *out = accu + tmp;
        }

        let s1 = self.get_filter_states_slot(1);
        let s3 = self.get_filter_states_slot(3);
        let s5 = self.get_filter_states_slot(5);
        let s7 = self.get_filter_states_slot(7);
        let s9 = self.get_filter_states_slot(9);
        for (flt, s1_val, s3_val, s5_val, s7_val, s9_val, out) in izip!(
            filt_right.chunks_exact(filt_chunk_size),
            s1.iter(),
            s3.iter(),
            s5.iter(),
            s7.iter(),
            s9.iter(),
            out_left.iter_mut().rev()
        )
        .take(num_subbands - 1)
        {
            // Perform FIR-Filter
            let tmp = flt[4] * *s9_val;
            let mut accu = flt[0] * *s1_val;
            accu += flt[1] * *s3_val;
            accu += flt[2] * *s5_val;
            accu += flt[3] * *s7_val;
            *out = accu + tmp;
        }

        // Perform FIR-Filter
        let flt = &filt_right[filt_chunk_size * (num_subbands - 1)..];
        let tmp = flt[4] * s9[s9.len() - 1];
        let mut accu = flt[0] * s1[s1.len() - 1];
        accu += flt[1] * s3[s3.len() - 1];
        accu += flt[2] * s5[s5.len() - 1];
        accu += flt[3] * s7[s7.len() - 1];
        output[0] = accu + tmp;
    }

    /// Performs complex-valued forward modulation on a single slot of filtered input data.
    /// The forward modulation takes `2 * self.num_subbands()` `time_in` data and after modulation,
    /// stores the `self.num_subbands()` complex subband samples split in
    /// `r_subband` (for the real part) and `i_subband` (for the imaginary part in).
    ///
    /// # Parameters
    /// - `time_in`: Input data in time domain. Length is `2 * self.num_subbands()`.
    /// - `r_subband`: Real part of Output samples. Length is `self.num_subbands()`.
    /// - `i_subband`: Imaginary part of Output samples. Length is `self.num_subbands()`.
    fn forward_modulation_hq(
        &mut self,
        time_in: &[f32],
        r_subband: &mut [f32],
        i_subband: &mut [f32],
    ) {
        let num_subbands = usize::from(self.num_subbands);
        let factor = self.ana_factor;
        // Time advance by one sample, which is equivalent to the complex
        // rotation at the end of the analysis. Works only for STD mode.
        if (num_subbands == QMF_MAX_ANALYSIS_SUBBANDS)
            && !self.flags.intersects(QmfFlags::CLDFB | QmfFlags::MPSLDFB)
        {
            let x = time_in[1];
            let y = time_in[0];
            r_subband[0] = x + y;
            i_subband[0] = x - y;

            // Formula for implementation in case of complex rotation
            // rSubband[n] = u[n+1] - u[2*num_subbands-n], n=1,...,num_subbands-1
            // iSubband[n] = u[n+1] + u[2*num_subbands-n], n=1,...,num_subbands-1
            izip!(
                time_in[..num_subbands + 1].iter().skip(2),
                time_in[num_subbands..(2 * num_subbands)]
                    .iter()
                    .skip(1)
                    .rev(),
                r_subband.iter_mut().skip(1),
                i_subband.iter_mut().skip(1),
            )
            .take(num_subbands - 1)
            .for_each(|(x, y, re, im)| {
                *re = *x - *y;
                *im = *x + *y;
            });
        } else {
            // rSubband[n] = u[n] - u[2*num_subbands-n-1], n=0,...,num_subbands-1
            // iSubband[n] = u[n] + u[2*num_subbands-n-1], n=0,...,num_subbands-1
            let (left, right) = time_in[..num_subbands << 1].split_at(num_subbands);
            izip!(
                left.iter(),
                right.iter().rev(),
                r_subband.iter_mut(),
                i_subband.iter_mut(),
            )
            .take(num_subbands)
            .for_each(|(x, y, re, im)| {
                *re = *x - *y;
                *im = *x + *y;
            });
        };

        // Apply dct & dst transform type 4.
        dct::dctiv(&mut r_subband[0..num_subbands]);
        dct::dstiv(&mut i_subband[0..num_subbands]);

        // Do the complex rotation except for the case of 64 bands (in STD mode).
        if (num_subbands != QMF_MAX_ANALYSIS_SUBBANDS)
            || self.flags.intersects(QmfFlags::CLDFB | QmfFlags::MPSLDFB)
        {
            if self.flags.contains(QmfFlags::MPSLDFB_OPTIMIZE_MODULATION) {
                // Note: num_subbands is always even, but self.num_lf_subbands could be odd, also.
                let mut len = num_subbands.min(usize::from(self.num_lf_subbands));
                if len & 0x1 == 0x1 {
                    // Check case of length being an odd number.
                    len += 1;
                }
                for (re, im) in izip!(r_subband.chunks_exact_mut(2), i_subband.chunks_exact_mut(2))
                    .take(len >> 1)
                {
                    let re_tmp2 = -re[1];
                    re[1] = im[1] * factor;
                    im[1] = re_tmp2 * factor;

                    let re_tmp1 = re[0];
                    re[0] = -im[0] * factor;
                    im[0] = re_tmp1 * factor;
                }
            } else {
                for (cos, sin, r_in, i_in) in izip!(
                    self.cos_table.unwrap().iter(),
                    self.sin_table.unwrap().iter(),
                    r_subband.iter_mut(),
                    i_subband.iter_mut()
                )
                .take(num_subbands)
                {
                    let re_mul = *r_in * *cos + *i_in * *sin;
                    let im_mul = *i_in * *cos - *r_in * *sin;
                    *r_in = re_mul * factor;
                    *i_in = im_mul * factor;
                }
            }
        }
    }

    /// Performs synthesis filtering on QMF input data.
    /// It takes `self.num_time_slots()` input slots.
    /// The length of each slot is equal to `self.num_subbands()`.
    /// Each input slot contains `self.num_subbands()` complex QMF input data.
    /// The input slots are split in 2 input buffers (real and imaginary).
    /// It provides `self.num_time_slots()` time domain samples to the output.
    /// The length of each output slot is equal to `self.num_subbands()` samples.
    ///
    /// # Parameters
    /// - `qmf_buffer_real`: Slices of Real part of input data slot
    /// - `qmf_buffer_imag`: Slices of Imaginary part of input data slot
    /// - `time_out`: Output samples in time domain
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_SYNTHESIS_SUBBANDS * (2 * QMF_NO_POLY - 1);
    /// let num_lf_subbands = u8::try_from(QMF_MAX_SYNTHESIS_SUBBANDS / 2).unwrap();
    /// let num_hf_subbands = u8::try_from(QMF_MAX_SYNTHESIS_SUBBANDS / 2).unwrap();
    /// let num_subbands = QMF_MAX_SYNTHESIS_SUBBANDS;
    /// let num_time_slots = 2;
    /// let flags = QmfFlags::MPSLDFB;
    /// let qmf_mode = QmfMode::Synthesis;
    ///
    /// let mut real_qmf_in_data_slot = vec![0.0f32; QMF_MAX_SYNTHESIS_SUBBANDS];
    /// let mut imag_qmf_in_data_slot = vec![0.0f32; QMF_MAX_SYNTHESIS_SUBBANDS];
    /// let mut real_in_qmf: Vec<&[f32]> = Vec::new();
    /// let mut imag_in_qmf: Vec<&[f32]> = Vec::new();
    ///
    /// let mut time_out = vec![0.0f32; usize::from(num_time_slots) * QMF_MAX_SYNTHESIS_SUBBANDS];
    ///
    /// for (re_qmf_slot, im_qmf_slot) in real_qmf_in_data_slot
    ///     .chunks_exact_mut(num_subbands)
    ///     .zip(imag_qmf_in_data_slot.chunks_exact_mut(num_subbands))
    ///     .take(usize::from(num_time_slots))
    /// {
    ///     real_in_qmf.push(re_qmf_slot);
    ///     imag_in_qmf.push(im_qmf_slot);
    /// }
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.init(
    ///     num_time_slots,
    ///     num_lf_subbands,
    ///     num_hf_subbands,
    ///     u8::try_from(num_subbands).unwrap(),
    ///     flags,
    ///     qmf_mode,
    /// );
    /// qmf.synthesis_filtering(&real_in_qmf, &imag_in_qmf, &mut time_out[..]);
    /// ```
    pub fn synthesis_filtering(
        &mut self,
        qmf_buffer_real: &[&[f32]],
        qmf_buffer_imag: &[&[f32]],
        time_out: &mut [f32],
    ) {
        let num_timeslots = usize::from(self.num_time_slots);

        debug_assert!(
            time_out.len() >= usize::from(self.num_subbands) * num_timeslots,
            "Synthesis output buffer size is too small."
        );

        // Loop over number of time slots.
        for (time_out_slot, real_slot, imag_slot) in izip!(
            time_out.chunks_exact_mut(usize::from(self.num_subbands)),
            qmf_buffer_real.iter(),
            qmf_buffer_imag.iter()
        )
        .take(num_timeslots)
        {
            self.synthesis_filtering_slot(real_slot, Some(imag_slot), time_out_slot);
        }
    }

    /// Performs QMF synthesis filtering on a single QMF input data slot.
    /// A slot contains `self.num_subbands()` complex QMF input data.
    /// The input slot is split in 2 input buffers (real and imaginary).
    /// It provides `self.num_subbands()` time domain samples to the output.
    ///
    /// # Parameters
    /// - `real_slot`: Real part of input data
    /// - `imag_slot`: Imaginary part of input data
    /// - `time_out`: Output samples in time domain
    ///
    /// # Examples
    /// ```
    /// use aac::common::qmf::*;
    ///
    /// let len_filter_states = QMF_MAX_SYNTHESIS_SUBBANDS * (2 * QMF_NO_POLY - 1);
    /// let num_lf_subbands = u8::try_from(QMF_MAX_SYNTHESIS_SUBBANDS / 2).unwrap();
    /// let num_hf_subbands = u8::try_from(QMF_MAX_SYNTHESIS_SUBBANDS / 2).unwrap();
    /// let num_subbands = u8::try_from(QMF_MAX_SYNTHESIS_SUBBANDS).unwrap();
    /// let num_time_slots = 1;
    /// let flags = QmfFlags::MPSLDFB;
    /// let qmf_mode = QmfMode::Synthesis;
    ///
    /// let real_in_slot = &vec![0.0_f32; QMF_MAX_SYNTHESIS_SUBBANDS];
    /// let imag_in_slot = vec![0.0_f32; QMF_MAX_SYNTHESIS_SUBBANDS];
    /// let imag_in_slot_option = Some(imag_in_slot.as_slice());
    ///
    /// let time_out = &mut vec![0.0_f32; usize::from(num_time_slots) * QMF_MAX_SYNTHESIS_SUBBANDS];
    ///
    /// let mut qmf = QmfFilterBank::new(len_filter_states);
    /// qmf.init(
    ///     num_time_slots,
    ///     num_lf_subbands,
    ///     num_hf_subbands,
    ///     num_subbands,
    ///     flags,
    ///     qmf_mode,
    /// );
    /// qmf.synthesis_filtering_slot(real_in_slot, imag_in_slot_option, time_out);
    /// ```
    pub fn synthesis_filtering_slot(
        &mut self,
        real_slot: &[f32],
        imag_slot: Option<&[f32]>,
        time_out: &mut [f32],
    ) {
        let num_subbands = usize::from(self.num_subbands);
        // Create temp work buffer for qmf synthesis filtering.
        let mut work_buffer = [0.0_f32; 2 * QMF_MAX_SYNTHESIS_SUBBANDS];

        debug_assert!(
            time_out.len() >= num_subbands,
            "Synthesis output buffer size is too small."
        );

        if !self.flags.contains(QmfFlags::LP) {
            self.inverse_modulation_hq(real_slot, imag_slot, &mut work_buffer);
        } else {
            self.inverse_modulation_lowpower_even(real_slot, &mut work_buffer);
        }

        if self.flags.contains(QmfFlags::NONSYMMETRIC) {
            self.synthesis_prototype_firslot_nonsymmetric(
                &work_buffer[0..num_subbands],
                &work_buffer[num_subbands..],
                time_out,
            );
        } else {
            self.synthesis_prototype_firslot_symmetric(
                &work_buffer[0..num_subbands],
                &work_buffer[num_subbands..],
                time_out,
            );
        }
    }

    /// Performs high quality inverse modulation on a single slot of QMF input data.
    /// The filter takes `self.num_subbands()` complex input data and
    /// stores the `self.num_subbands()` complex subband samples in the output buffer.
    /// The complex data output is split in 2 parts (first half - real, second half - imaginary).
    ///
    /// # Parameters
    /// - `qmf_real`: Real part of synthesis input data
    /// - `qmf_imag_option`: Imaginary part of synthesis input data and it can be `None` in case of
    ///   HBE.
    /// - `output`: Output samples
    #[inline]
    fn inverse_modulation_hq(
        &mut self,
        qmf_real: &[f32],
        qmf_imag_option: Option<&[f32]>,
        output: &mut [f32],
    ) {
        let syn_factor = 0.5_f32 / 64.0_f32;
        let num_subbands = usize::from(self.num_subbands);
        let m = num_subbands >> 1;
        let lsb_val = usize::from(self.num_lf_subbands);
        let usb_val = usize::from(self.num_hf_subbands);

        let (real_out, imag_out) = output[..2 * num_subbands].split_at_mut(num_subbands);

        if self.flags.contains(QmfFlags::CLDFB) {
            let bands = lsb_val.max(usb_val);

            if let Some(qmf_imag) = qmf_imag_option {
                for (cos, sin, i_in, r_in, i_out, r_out) in izip!(
                    self.cos_table.unwrap().iter(),
                    self.sin_table.unwrap().iter(),
                    qmf_imag.iter(),
                    qmf_real.iter(),
                    imag_out.iter_mut(),
                    real_out.iter_mut(),
                )
                .take(bands)
                {
                    // out  = (r_in + ii_in) * (cos + isin).conj()
                    let re_mul = *r_in * *cos + *i_in * *sin;
                    let im_mul = *i_in * *cos - *r_in * *sin;
                    *r_out = re_mul;
                    *i_out = im_mul;
                }
            } else {
                for (cos, sin, r_in, i_out, r_out) in izip!(
                    self.cos_table.unwrap().iter(),
                    self.sin_table.unwrap().iter(),
                    qmf_real.iter(),
                    imag_out.iter_mut(),
                    real_out.iter_mut(),
                )
                .take(bands)
                {
                    // out  = r_in * (cos + isin).conj()
                    let re_mul = *r_in * *cos;
                    let im_mul = -*r_in * *sin;
                    *r_out = re_mul;
                    *i_out = im_mul;
                }
            }
        } else {
            real_out[..usb_val].copy_from_slice(&qmf_real[..usb_val]);

            if let Some(qmf_imag) = qmf_imag_option {
                imag_out[..usb_val].copy_from_slice(&qmf_imag[..usb_val]);
            }
        }

        real_out[usb_val..num_subbands].fill(0.0_f32);
        imag_out[usb_val..num_subbands].fill(0.0_f32);

        dct::dctiv(real_out);
        dct::dstiv(imag_out);

        let (real_left, real_right) = real_out.split_at_mut(m);
        let (imag_left, imag_right) = imag_out.split_at_mut(m);
        let iter_slots = izip!(
            real_left.iter_mut(),
            imag_left.iter_mut(),
            real_right.iter_mut().rev(),
            imag_right.iter_mut().rev()
        )
        .take(m);

        if self.flags.contains(QmfFlags::CLDFB) {
            for (re_1, im_1, re_2, im_2) in iter_slots {
                let r1 = *re_1;
                let i2 = *im_2;
                let r2 = *re_2;
                let i1 = *im_1;

                *re_1 = (r1 - i1) * syn_factor;
                *im_2 = -(r1 + i1) * syn_factor;
                *re_2 = (r2 - i2) * syn_factor;
                *im_1 = -(r2 + i2) * syn_factor;
            }
        } else {
            for (re_1, im_1, re_2, im_2) in iter_slots {
                let r1 = -*re_1;
                let i2 = -*im_2;
                let r2 = -*re_2;
                let i1 = -*im_1;

                *re_1 = (r1 - i1) * syn_factor;
                *im_2 = -(r1 + i1) * syn_factor;
                *re_2 = (r2 - i2) * syn_factor;
                *im_1 = -(r2 + i2) * syn_factor;
            }
        }
    }

    /// Performs low-power inverse modulation on a single slot of QMF input data.
    /// The filter takes `self.num_subbands()` real input data
    /// stores the `self.num_subbands()` complex subband samples in the output buffer.
    /// The complex data output is split in 2 parts (first half - real, second half - imaginary).
    ///
    /// # Parameters
    /// - `qmf_real`: Real part of synthesis input data
    /// - `time_out`: Output samples
    #[inline]
    fn inverse_modulation_lowpower_even(&mut self, qmf_real: &[f32], time_out: &mut [f32]) {
        let syn_factor: f32 = 0.5_f32 / 32.0_f32;
        let num_subbands = usize::from(self.num_subbands);
        let usb_len = usize::from(self.num_hf_subbands);
        let m = num_subbands >> 1;

        let (real, imag) = time_out.split_at_mut(num_subbands);

        real[0..usb_len].copy_from_slice(&qmf_real[0..usb_len]);
        if num_subbands > usb_len {
            real[usb_len..(num_subbands - usb_len)].fill(0.0_f32);
        }

        // DCT type-2 transform
        dct::dctii(real);

        // Expand output and replace inplace the output buffers.
        imag[0] = real[m] * syn_factor;
        imag[m] = 0.0_f32;
        let mut tmp = real[0] * syn_factor;
        real[0] = real[m] * syn_factor;
        real[m] = tmp;

        for i in 1..m / 2 {
            // Imag slot
            tmp = real[num_subbands - i] * syn_factor;
            imag[m - i] = tmp;
            imag[m + i] = -tmp;

            tmp = real[m + i] * syn_factor;
            imag[i] = tmp;
            imag[num_subbands - i] = -tmp;

            // Real slot
            real[m + i] = real[i] * syn_factor;
            real[num_subbands - i] = real[m - i] * syn_factor;
            tmp = real[i] * syn_factor;
            real[i] = real[m - i] * syn_factor;
            real[m - i] = tmp;
        }

        // Remaining odd terms
        tmp = real[m + m / 2] * syn_factor;
        imag[m / 2] = tmp;
        imag[(m / 2) + m] = -tmp;

        real[m / 2] *= syn_factor;
        real[m + m / 2] = real[m / 2];
    }

    /// Performs synthesis prototype symmetric filtering on a single slot of QMF input data.
    /// The filter takes `self.num_subbands()` synthesis complex input data and
    /// generates `self.num_subbands()` time domain output samples.
    ///
    /// # Parameters
    /// - `real_slot`: Real part of input data
    /// - `imag_slot`: Imaginary part of input data
    /// - `time_out`: Output samples in time domain
    fn synthesis_prototype_firslot_symmetric(
        &mut self,
        real_slot: &[f32],
        imag_slot: &[f32],
        time_out: &mut [f32],
    ) {
        let num_subbands = usize::from(self.num_subbands);
        let filt_offset = usize::from(self.stride_poly) * QMF_NO_POLY;
        let coeffs = self.filter_coeff.unwrap();

        let flt_slice: &[f32] = &coeffs[filt_offset..(num_subbands * filt_offset) + QMF_NO_POLY];
        let fltm_slice = &coeffs[0..usize::from(self.filter_size / 2)];
        for (flt, fltm, sta, re_in, im_in, out) in izip!(
            flt_slice.chunks_exact(filt_offset),
            fltm_slice.rchunks_exact(filt_offset),
            self.filter_states.chunks_exact_mut(NUM_SYNTH_FILTER_STATES),
            real_slot[..num_subbands].iter().rev(),
            imag_slot[..num_subbands].iter().rev(),
            time_out[..num_subbands].iter_mut().rev()
        )
        .take(num_subbands - 1)
        {
            let imag = *im_in;
            let real = *re_in;
            {
                let a_re = sta[0] + fltm[0] * real;
                *out = a_re * self.out_gain * 2.0_f32;
            }
            let tmp = sta[8] + fltm[4] * real;
            sta[0] = sta[1] + flt[4] * imag;
            sta[1] = sta[2] + fltm[1] * real;
            sta[2] = sta[3] + flt[3] * imag;
            sta[3] = sta[4] + fltm[2] * real;
            sta[4] = sta[5] + flt[2] * imag;
            sta[5] = sta[6] + fltm[3] * real;
            sta[6] = sta[7] + flt[1] * imag;
            sta[7] = tmp;
            sta[8] = flt[0] * imag;
        }

        {
            let flt =
                &coeffs[(num_subbands * filt_offset)..(num_subbands * filt_offset) + QMF_NO_POLY];
            let fltm = &coeffs[..QMF_NO_POLY];
            let imag = imag_slot[0];
            let real = real_slot[0];
            let sta = &mut self.filter_states[(num_subbands - 1) * NUM_SYNTH_FILTER_STATES..];
            {
                let a_re = sta[0] + fltm[0] * real;
                time_out[0] = a_re * self.out_gain * 2.0_f32;
            }
            let tmp = sta[8] + fltm[4] * real;
            sta[0] = sta[1] + flt[4] * imag;
            sta[1] = sta[2] + fltm[1] * real;
            sta[2] = sta[3] + flt[3] * imag;
            sta[3] = sta[4] + fltm[2] * real;
            sta[4] = sta[5] + flt[2] * imag;
            sta[5] = sta[6] + fltm[3] * real;
            sta[6] = sta[7] + flt[1] * imag;
            sta[7] = tmp;
            sta[8] = flt[0] * imag;
        }
    }

    /// Performs synthesis prototype non-symmetric filtering on a single slot of QMF input data.
    /// The filter takes `self.num_subbands()` synthesis complex input data and
    /// generates `self.num_subbands()` time domain output samples.
    ///
    /// # Parameters
    /// - `real_slot`: Real part of input data
    /// - `imag_slot`: Imaginary part of input data
    /// - `time_out`: Output samples in time domain
    fn synthesis_prototype_firslot_nonsymmetric(
        &mut self,
        real_slot: &[f32],
        imag_slot: &[f32],
        time_out: &mut [f32],
    ) {
        let num_subbands = usize::from(self.num_subbands);
        let filt_offset = usize::from(self.stride_poly) * QMF_NO_POLY;
        let coeffs = self.filter_coeff.unwrap();

        let flt_slice = &coeffs[0..(num_subbands * filt_offset)];
        let fltm_slice = &coeffs[usize::from(self.filter_size / 2)
            ..(usize::from(self.filter_size / 2) + (num_subbands * filt_offset))];

        for (flt, fltm, sta, re_in, im_in, out) in izip!(
            flt_slice.chunks_exact(filt_offset),
            fltm_slice.chunks_exact(filt_offset),
            self.filter_states.chunks_exact_mut(NUM_SYNTH_FILTER_STATES),
            real_slot[..num_subbands].iter().rev(),
            imag_slot[..num_subbands].iter().rev(),
            time_out[..num_subbands].iter_mut().rev()
        )
        .take(num_subbands)
        {
            let imag = *im_in;
            let real = *re_in;
            {
                let a_re = sta[0] + fltm[4] * real;
                *out = a_re * self.out_gain * 2.0_f32;
            }

            sta[0] = sta[1] + flt[4] * imag;
            sta[1] = sta[2] + fltm[3] * real;
            sta[2] = sta[3] + flt[3] * imag;
            sta[3] = sta[4] + fltm[2] * real;
            sta[4] = sta[5] + flt[2] * imag;
            sta[5] = sta[6] + fltm[1] * real;
            sta[6] = sta[7] + flt[1] * imag;
            sta[7] = sta[8] + fltm[0] * real;
            sta[8] = flt[0] * imag;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_qmf_instance() {
        let num_states = 0;
        let qmf = QmfFilterBank::new(num_states);

        assert!(qmf.filter_coeff.is_none());
        assert!(qmf.filter_states.is_empty());
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 0);
        assert_eq!(qmf.num_subbands, 0);
        assert_eq!(qmf.num_time_slots, 0);
        assert_eq!(qmf.num_lf_subbands, 0);
        assert_eq!(qmf.num_hf_subbands, 0);
        assert_eq!(qmf.flags, QmfFlags::empty());
        assert_eq!(qmf.stride_poly, 0);
        assert_eq!(qmf.ana_factor, 0.0_f32);
        assert_eq!(qmf.out_gain, 0.0_f32);
    }

    #[test]
    fn init_mpsldfb_num_subbands_64() {
        // No.of subbands is 64
        let num_states = 64 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 64_u8;
        let flags = QmfFlags::MPSLDFB;
        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );
        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_MPSLDFB_640[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(
            qmf.flags,
            (QmfFlags::MPSLDFB | QmfFlags::NONSYMMETRIC | QmfFlags::MPSLDFB_OPTIMIZE_MODULATION)
        );
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_mpsldfb_num_channels_32() {
        // No.of channels is 32
        let num_states = 32 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::MPSLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_MPSLDFB_320[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 320);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(
            qmf.flags,
            (QmfFlags::MPSLDFB | QmfFlags::NONSYMMETRIC | QmfFlags::MPSLDFB_OPTIMIZE_MODULATION)
        );
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_mpsldfb_num_channels_invalid() {
        //No.of channels can be anything except 32, 64.
        let num_states = 15 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 15_u8;
        let flags = QmfFlags::MPSLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, -1); // Invalid Error status
        assert_eq!(qmf.flags, QmfFlags::empty());
        assert_eq!(qmf.num_subbands, 0);
        assert_eq!(qmf.num_time_slots, 0);
        assert_eq!(qmf.num_lf_subbands, 0);
        assert_eq!(qmf.num_hf_subbands, 0);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.filter_size, 0);
        assert_eq!(qmf.out_gain, 0.0_f32);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert!(qmf.filter_coeff.is_none());
        assert!(!qmf.filter_states.is_empty());
    }

    #[test]
    fn init_cldfb_num_channels_64() {
        // No.of channels is 64
        let num_states = 64 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 64_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_640[..]));
        // assert_eq!(qmf.filter_states.len(), fs_len);
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS64_CLDFB[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN64_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_32() {
        // No.of channels is 32
        let num_states = 32 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_320[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS32_CLDFB_ANA[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN32_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 320);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_32_synmode() {
        // No.of channels is 32 , synthesis mode is active.
        let num_states = 32 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Synthesis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_320[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS32_CLDFB_SYN[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN32_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 320);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_16() {
        // No.of channels is 16
        let num_states = 16 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 16_u8;
        let num_subbands = 16_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_160[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS16_CLDFB_ANA[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN16_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 160);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 4.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_16_synmode() {
        // No.of channels is 16, synthesis mode is active.
        let num_states = 16 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 16_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Synthesis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_160[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS16_CLDFB_SYN[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN16_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 160);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_subbands);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 4.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_8() {
        // No.of channels is 8.
        let num_states = 8 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 16_u8;
        let num_hf_subbands = 8_u8;
        let num_subbands = 8_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_80[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS8_CLDFB_ANA[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN8_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 80);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 8.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_8_synmode() {
        // No.of channels is 8, synthesis mode is active.
        let num_states = 8 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 4_u8;
        let num_hf_subbands = 16_u8;
        let num_subbands = 8_u8;
        let flags = QmfFlags::CLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Synthesis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_CLDFB_80[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS8_CLDFB_SYN[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN8_CLDFB[..])
        );
        assert_eq!(qmf.filter_size, 80);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.flags, (QmfFlags::CLDFB | QmfFlags::NONSYMMETRIC));
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 8.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_cldfb_num_channels_invalid() {
        // No.of channels can be anything except previous cases.
        let num_states = 15 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 15_u8;
        let flags = QmfFlags::MPSLDFB;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, -1); // Invalid Error status
        assert_eq!(qmf.flags, QmfFlags::empty());
        assert_eq!(qmf.num_subbands, 0);
        assert_eq!(qmf.num_time_slots, 0);
        assert_eq!(qmf.num_lf_subbands, 0);
        assert_eq!(qmf.num_hf_subbands, 0);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.filter_size, 0);
        assert_eq!(qmf.out_gain, 0.0_f32);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert!(qmf.filter_coeff.is_none());
        assert!(!qmf.filter_states.is_empty());
    }

    #[test]
    fn init_num_channels_64() {
        // No.of channels is 64.
        let num_states = 64 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 64_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT640[..]));
        assert_eq!(qmf.cos_table, Some(&qmf_tables::QMF_PHASESHIFT_COS64[..]));
        assert_eq!(qmf.sin_table, Some(&qmf_tables::QMF_PHASESHIFT_SIN64[..]));
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_40() {
        // No.of channels is 40.
        let num_states = 40 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 40_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT400[..]));
        assert_eq!(qmf.cos_table, Some(&qmf_tables::QMF_PHASESHIFT_COS40[..]));
        assert_eq!(qmf.sin_table, Some(&qmf_tables::QMF_PHASESHIFT_SIN40[..]));
        assert_eq!(qmf.filter_size, u16::from(num_subbands) * 10);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_32_downsampled() {
        // No.of channels is 32, QmfFlags::DOWNSAMPLED is active.
        let num_states = 32 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::DOWNSAMPLED;

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT640[..]));
        assert_eq!(
            qmf.cos_table,
            Some(&qmf_tables::QMF_PHASESHIFT_COS_DOWNSAMP32[..])
        );
        assert_eq!(
            qmf.sin_table,
            Some(&qmf_tables::QMF_PHASESHIFT_SIN_DOWNSAMP32[..])
        );
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, QmfFlags::DOWNSAMPLED);
        assert_eq!(qmf.stride_poly, 2);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_32() {
        // No.of channels is 32, QmfFlags::DOWNSAMPLED is inactive.
        let num_states = 32 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT640[..]));
        assert_eq!(qmf.cos_table, Some(&qmf_tables::QMF_PHASESHIFT_COS32[..]));
        assert_eq!(qmf.sin_table, Some(&qmf_tables::QMF_PHASESHIFT_SIN32[..]));
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 2);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_24() {
        // No.of channels is 24.
        let num_states = 24 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 24_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT240[..]));
        assert_eq!(qmf.cos_table, Some(&qmf_tables::QMF_PHASESHIFT_COS24[..]));
        assert_eq!(qmf.sin_table, Some(&qmf_tables::QMF_PHASESHIFT_SIN24[..]));
        assert_eq!(qmf.filter_size, 240);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 4.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_16() {
        // No.of channels is 16.
        let num_states = 16 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 16_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT640[..]));
        assert_eq!(qmf.cos_table, Some(&qmf_tables::QMF_PHASESHIFT_COS16[..]));
        assert_eq!(qmf.sin_table, Some(&qmf_tables::QMF_PHASESHIFT_SIN16[..]));
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 4);
        assert_eq!(qmf.ana_factor, 4.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_20() {
        // No.of channels is 20.
        let num_states = 20 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 20_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT200[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 200);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 4.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_12() {
        // No.of channels is 12.
        let num_states = 12 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 12_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT120[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 120);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 8.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_8() {
        // No.of channels is 8.
        let num_states = 8 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 8_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_PFILT640[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 640);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(qmf.flags, flags);
        assert_eq!(qmf.stride_poly, 8);
        assert_eq!(qmf.ana_factor, 8.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_num_channels_invalid() {
        // No.of channels can be anything except previous cases.
        let num_states = 18 * 9;
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 5_u8;
        let num_lf_subbands = 32_u8;
        let num_hf_subbands = 32_u8;
        let num_subbands = 18_u8;
        let flags = QmfFlags::empty();

        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        assert_eq!(err, -1); // Invalid Error status
        assert_eq!(qmf.flags, QmfFlags::empty());
        assert_eq!(qmf.num_subbands, 0);
        assert_eq!(qmf.num_time_slots, 0);
        assert_eq!(qmf.num_lf_subbands, 0);
        assert_eq!(qmf.num_hf_subbands, 0);
        assert_eq!(qmf.stride_poly, 0);
        assert_eq!(qmf.filter_size, 0);
        assert_eq!(qmf.out_gain, 0.0_f32);
        assert_eq!(qmf.ana_factor, 1.0_f32);
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert!(qmf.filter_coeff.is_none());
        assert!(!qmf.filter_states.is_empty());
    }

    #[test]
    fn init_analysis_fb_clear_state() {
        let num_states = 32 * 9 + 10; // It is just dummy state length for testing.
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 10_u8;
        let num_lf_subbands = 16_u8;
        let num_hf_subbands = 16_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::MPSLDFB;
        // (2 * QMF_NO_POLY - 1) * num_subbands

        qmf.filter_states[..9 * usize::from(num_subbands) + 10].fill(1.0_f32);
        // Clear filter state buffer values.
        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        for i in 0..(9 * usize::from(num_subbands)) {
            assert_eq!(qmf.filter_states[i], 0.0_f32);
        }
        for j in 0..10 {
            assert_eq!(
                qmf.filter_states[j + (9 * usize::from(num_subbands))],
                1.0_f32
            );
        }
        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_MPSLDFB_320[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 320);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(
            qmf.flags,
            (QmfFlags::MPSLDFB | QmfFlags::NONSYMMETRIC | QmfFlags::MPSLDFB_OPTIMIZE_MODULATION)
        );
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }

    #[test]
    fn init_analysis_fb_keep_state() {
        let num_states = 32 * 9 + 10; // It is just dummy state length for testing.
        let mut qmf = QmfFilterBank::new(num_states);
        let num_time_slots = 10_u8;
        let num_lf_subbands = 16_u8;
        let num_hf_subbands = 16_u8;
        let num_subbands = 32_u8;
        let flags = QmfFlags::MPSLDFB | QmfFlags::KEEP_STATES;

        qmf.filter_states[..9 * usize::from(num_subbands) + 10].fill(1.0_f32);

        // Keep filter state buffer values.
        let err = qmf.init(
            num_time_slots,
            num_lf_subbands,
            num_hf_subbands,
            num_subbands,
            flags,
            QmfMode::Analysis,
        );

        for i in 0..(9 * usize::from(num_subbands) + 10) {
            assert_eq!(qmf.filter_states[i], 1.0_f32);
        }

        assert_eq!(err, 0);
        assert_eq!(qmf.filter_coeff, Some(&qmf_tables::QMF_MPSLDFB_320[..]));
        assert!(qmf.cos_table.is_none());
        assert!(qmf.sin_table.is_none());
        assert_eq!(qmf.filter_size, 320);
        assert_eq!(qmf.num_subbands, num_subbands);
        assert_eq!(qmf.num_time_slots, num_time_slots);
        assert_eq!(qmf.num_lf_subbands, num_lf_subbands);
        assert_eq!(qmf.num_hf_subbands, num_hf_subbands);
        assert_eq!(
            qmf.flags,
            (QmfFlags::MPSLDFB
                | QmfFlags::NONSYMMETRIC
                | QmfFlags::MPSLDFB_OPTIMIZE_MODULATION
                | QmfFlags::KEEP_STATES)
        );
        assert_eq!(qmf.stride_poly, 1);
        assert_eq!(qmf.ana_factor, 2.0_f32);
        assert_eq!(qmf.out_gain, 1.0_f32);
    }
}
