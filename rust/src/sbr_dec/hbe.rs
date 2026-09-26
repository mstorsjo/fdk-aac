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
//! High-Band Extension (HBE)
//!
//! This module contains the implementation of the High-Band Extension (HBE)
//! algorithm, which is used to extend the bandwidth of audio signals.

//Modules
mod constants;

// Imports
use super::{common::SbrError, constants::LPC_ORDER};
use crate::common::qmf::{self, QmfFilterBank, QmfFlags};
use constants::*;
use itertools::izip;
use num_complex::Complex;

// Re-exports
pub use constants::{HBE_MAX_NUM_PATCHES, HBE_MAX_STRETCH};

/// Enum representing the mode for keeping states synced.
#[derive(PartialEq, Clone, Copy, Debug)]
#[repr(C)]
pub(super) enum KeepStatesSyncedMode {
    /// Normal QMF transposer behaviour.
    Off = 0,
    /// QMF transposer called for syncing of states.
    /// The last 8/14 slots are calculated in case the next frame is HBE.
    Normal = 1,
    /// QMF transposer behaviour with output difference.
    /// The calculated slots are directly written to the overlap area of the buffer.
    /// Only used in resetSbrDec function.
    OutDiff = 2,
    /// QMF transposer is called for syncing of states only, not output is generated at all.
    /// Only used in resetSbrDec function.
    NoOut = 3,
    /// Harmonic patch generation disabled.
    NormalSkip = 4,
}

/// HBE buffer structure.
///
/// This structure holds the buffers for real and imaginary parts of the HBE
/// process.
#[derive(Default, Debug, Clone)]
pub(crate) struct HbeBuffer {
    real: Box<[Box<[f32]>]>,
    imag: Box<[Box<[f32]>]>,
    hbe_light_time_delay_buffer: Box<[f32]>,
}

impl<'a> HbeBuffer {
    fn new(no_cols: usize, no_channels: usize) -> Self {
        let mut vec_real = Vec::<Box<[f32]>>::with_capacity(2 * no_cols);
        let mut vec_imag = Vec::<Box<[f32]>>::with_capacity(2 * no_cols);
        for _ in 0..2 * no_cols {
            let buf = vec![0.0f32; no_channels].to_vec().into_boxed_slice();
            vec_real.push(buf);

            let buf = vec![0.0f32; no_channels].to_vec().into_boxed_slice();
            vec_imag.push(buf);
        }

        Self {
            real: vec_real.into_boxed_slice(),
            imag: vec_imag.into_boxed_slice(),
            hbe_light_time_delay_buffer: vec![0.0f32; no_cols * no_channels]
                .to_vec()
                .into_boxed_slice(),
        }
    }

    pub(crate) fn real(&'a self) -> &'a [Box<[f32]>] {
        &self.real
    }

    #[allow(dead_code)]
    pub(crate) fn imag(&'a self) -> &'a [Box<[f32]>] {
        &self.imag
    }

    #[allow(clippy::type_complexity)]
    pub(super) fn buffers_mut(
        &'a mut self,
    ) -> (
        &'a mut Box<[Box<[f32]>]>,
        &'a mut Box<[Box<[f32]>]>,
        &'a Box<[f32]>,
    ) {
        (
            &mut self.real,
            &mut self.imag,
            &self.hbe_light_time_delay_buffer,
        )
    }

    pub(super) fn hbe_light_time_delay_buffer(&self) -> &[f32] {
        &self.hbe_light_time_delay_buffer
    }

    pub(super) fn hbe_light_time_delay_buffer_mut(&mut self) -> &mut [f32] {
        &mut self.hbe_light_time_delay_buffer
    }
}

/// HBE transposer structure.
///
/// This structure holds the parameters and buffers required for the HBE
/// transposition process.
#[repr(C)]
#[derive(Clone, Debug)]
pub struct HbeTransposer {
    quad_rate: bool,
    no_cols: u8,
    no_channels: u8,
    start_band: u8,
    stop_band: u8,
    kstart: u8,
    synth_size: u8,
    max_stretch: u8,
    time_domain_win_len: u16,

    qmf_in_buf_slot_index: u8,
    qmf_hbe_buf_slot_index: u8,

    x_over_qmf: [u8; HBE_MAX_NUM_PATCHES as usize],

    in_buf: [f32; 3 * HBE_MAX_QMF_SYN_BANDS as usize],

    hbe_analysis_qmf: QmfFilterBank,
    hbe_synthesis_qmf: QmfFilterBank,

    sign_pre_mod: f32,

    synthesis_qmf_pre_mod_cos: Option<&'static [f32]>,
    synthesis_qmf_pre_mod_sin: Option<&'static [f32]>,

    pub(crate) hbe_buffer: Box<HbeBuffer>,

    qmf_in_buf: [[Complex<f32>; HBE_QMF_SYNTH_CHANNELS as usize]; HBE_QMF_IN_BUF_SIZE as usize],
    qmf_hbe_buf: [[Complex<f32>; HBE_QMF_SYNTH_CHANNELS as usize]; HBE_MAX_OUT_SLOTS as usize],
}

impl HbeTransposer {
    /// Creates the HBE transposer.
    ///
    /// This function initializes the HBE transposer with the given frame size,
    /// number of columns, and SBR mode.
    ///
    /// # Parameters
    ///
    /// - `frame_size`: The size of the audio frame.
    /// - `no_cols`: The number of columns in the audio frame.
    /// - `quad_rate`: The SBR mode.
    pub(super) fn new(frame_size: u16, no_cols: u8, quad_rate: bool) -> Self {
        let no_channels = HBE_QMF_SYNTH_CHANNELS / ((if quad_rate { 1 } else { 0 } + 1) * 2);

        let hbe_buffer = Box::new(HbeBuffer::new(no_cols.into(), no_channels.into()));

        let no_cols = if frame_size == 768 {
            // 32 for 24:64

            u8::try_from((8 * frame_size / 3) / u16::from(HBE_QMF_SYNTH_CHANNELS)).unwrap()
        } else {
            // 32 for 32:64 and 64 for 16:64 ->
            // identical to sbrdec->no_cols
            u8::try_from(
                (u16::from(quad_rate) + 1) * 2 * frame_size / u16::from(HBE_QMF_SYNTH_CHANNELS),
            )
            .unwrap()
        };

        let no_channels = (frame_size / u16::from(no_cols)).try_into().unwrap();

        let time_domain_win_len = frame_size;

        Self {
            quad_rate,
            no_cols,
            no_channels,
            start_band: 0,
            stop_band: 0,
            kstart: 0,
            synth_size: 0,
            max_stretch: 0,
            time_domain_win_len,
            qmf_in_buf_slot_index: 0,
            qmf_hbe_buf_slot_index: 0,
            x_over_qmf: [0; HBE_MAX_NUM_PATCHES as usize],
            in_buf: [0.0; 3 * HBE_MAX_QMF_SYN_BANDS as usize],
            hbe_analysis_qmf: QmfFilterBank::new(HBE_QMF_FILTER_STATE_ANA_SIZE.into()),
            hbe_synthesis_qmf: QmfFilterBank::new(HBE_QMF_FILTER_STATE_SYN_SIZE.into()),
            sign_pre_mod: 0.0,
            synthesis_qmf_pre_mod_cos: None,
            synthesis_qmf_pre_mod_sin: None,
            hbe_buffer,

            qmf_in_buf: [[Complex { re: 0.0, im: 0.0 }; HBE_QMF_SYNTH_CHANNELS as usize];
                HBE_QMF_IN_BUF_SIZE as usize],
            qmf_hbe_buf: [[Complex { re: 0.0, im: 0.0 }; HBE_QMF_SYNTH_CHANNELS as usize];
                HBE_MAX_OUT_SLOTS as usize],
        }
    }

    /// Reinitializes the HBE transposer.
    ///
    /// This function reinitializes the HBE transposer with the given frequency
    /// band table and number of scalefactor bands.
    ///
    /// # Parameters
    ///
    /// - `freq_band_table`: A reference to the frequency band table.
    /// - `n_sfb`: A reference to the number of scalefactor bands.
    ///
    /// # Return
    ///
    /// Returns `Ok(())` if reinitialization is successful, otherwise returns an
    /// error if reinitialization fails.
    pub(super) fn reinit(
        &mut self,
        freq_band_table: &[&[u8]],
        n_sfb: &[u8],
    ) -> Result<(), SbrError> {
        self.start_band = freq_band_table[HBE_LO][0];
        debug_assert!(
            (!self.quad_rate && self.start_band <= 32) || (self.quad_rate && self.start_band <= 16)
        );
        self.stop_band = freq_band_table[HBE_LO][usize::from(n_sfb[HBE_LO])];

        // 8, 12, 16, 20
        self.synth_size = 4 * ((self.start_band + 4) / 8 + 1);
        self.kstart = START_SUBBAND2K_L[usize::from(self.start_band)];
        if self.quad_rate {
            if self.kstart + self.synth_size > 16 {
                self.kstart = 16 - self.synth_size;
            }
        } else if self.time_domain_win_len == 768 && self.kstart + self.synth_size > 24 {
            self.kstart = 24 - self.synth_size;
        }

        self.synthesis_qmf_pre_mod_cos = Some(&PRE_MOD_COS[usize::from(self.kstart)..]);
        self.synthesis_qmf_pre_mod_sin = Some(&PRE_MOD_SIN[usize::from(self.kstart)..]);

        self.sign_pre_mod = if PRE_MOD_COS[usize::from(self.kstart)] < 0.0 {
            1.0
        } else {
            -1.0
        };

        let qmf_err = self.hbe_synthesis_qmf.init(
            self.no_cols,
            0,
            self.synth_size,
            self.synth_size,
            QmfFlags::LP,
            qmf::QmfMode::Synthesis,
        );
        if qmf_err != 0 {
            return Err(SbrError::UnsupportedConfig);
        }

        let qmf_err = self.hbe_analysis_qmf.init(
            self.no_cols / 2,
            0,
            2 * self.synth_size,
            2 * self.synth_size,
            QmfFlags::empty(),
            qmf::QmfMode::Analysis,
        );
        if qmf_err != 0 {
            return Err(SbrError::UnsupportedConfig);
        }

        // Change analysis post twiddles
        // 8, 16, 24, 32, 40
        let l = 2 * self.synth_size;
        match l {
            8 => {
                self.hbe_analysis_qmf
                    .set_cos_table(Some(&POST_TWIDDLE_COS_8));
                self.hbe_analysis_qmf
                    .set_sin_table(Some(&POST_TWIDDLE_SIN_8));
            }
            16 => {
                self.hbe_analysis_qmf
                    .set_cos_table(Some(&POST_TWIDDLE_COS_16));
                self.hbe_analysis_qmf
                    .set_sin_table(Some(&POST_TWIDDLE_SIN_16));
            }
            24 => {
                self.hbe_analysis_qmf
                    .set_cos_table(Some(&POST_TWIDDLE_COS_24));
                self.hbe_analysis_qmf
                    .set_sin_table(Some(&POST_TWIDDLE_SIN_24));
            }
            32 => {
                self.hbe_analysis_qmf
                    .set_cos_table(Some(&POST_TWIDDLE_COS_32));
                self.hbe_analysis_qmf
                    .set_sin_table(Some(&POST_TWIDDLE_SIN_32));
            }
            40 => {
                self.hbe_analysis_qmf
                    .set_cos_table(Some(&POST_TWIDDLE_COS_40));
                self.hbe_analysis_qmf
                    .set_sin_table(Some(&POST_TWIDDLE_SIN_40));
            }
            _ => return Err(SbrError::UnsupportedConfig),
        }

        self.x_over_qmf.fill(0);

        let mut sfb: usize = 0;
        let stop_patch = if self.quad_rate {
            HBE_MAX_NUM_PATCHES
        } else {
            HBE_MAX_STRETCH
        };
        if self.quad_rate {
            self.max_stretch = HBE_MAX_STRETCH;
        }
        for patch in 1..=stop_patch {
            let patch_us = usize::from(patch);
            while sfb <= n_sfb[HBE_LO].into()
                && freq_band_table[HBE_LO][sfb] <= patch * self.start_band
            {
                sfb += 1;
            }
            if sfb <= n_sfb[HBE_LO].into() {
                // If the distance is larger than three QMF bands - try aligning to high
                // resolution frequency bands instead.
                if (patch * self.start_band - freq_band_table[HBE_LO][sfb - 1]) <= 3 {
                    self.x_over_qmf[patch_us - 1] = freq_band_table[HBE_LO][sfb - 1];
                } else {
                    let mut sfb_tmp: usize = 0;
                    while sfb_tmp <= n_sfb[HBE_HI].into()
                        && freq_band_table[HBE_HI][sfb_tmp] <= patch * self.start_band
                    {
                        sfb_tmp += 1;
                    }
                    self.x_over_qmf[patch_us - 1] = freq_band_table[HBE_HI][sfb_tmp - 1];
                }
            } else {
                self.x_over_qmf[patch_us - 1] = self.stop_band;
                self.max_stretch = patch.min(HBE_MAX_STRETCH);
                break;
            }
        }
        self.qmf_in_buf_slot_index = 0;
        self.qmf_hbe_buf_slot_index = 0;

        Ok(())
    }

    /// Retrieves the crossover band for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns a mutable reference to the crossover band array.
    pub fn x_over_band_mut(&mut self) -> &mut [u8] {
        &mut self.x_over_qmf
    }

    /// Retrieves the crossover band for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns crossover band array.
    pub(super) fn x_over_band(&self) -> &[u8; HBE_MAX_NUM_PATCHES as usize] {
        &self.x_over_qmf
    }

    /// Retrieves the 4to1 SBR flag for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns the 4to1 SBR flag.
    pub fn quad_rate(&self) -> bool {
        self.quad_rate
    }

    /// Retrieves the number of columns for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns the number of columns.
    pub(super) fn no_cols(&self) -> u8 {
        self.no_cols
    }

    /// Retrieves the number of channels for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns the number of channels.
    pub(super) fn no_channels(&self) -> u8 {
        self.no_channels
    }

    /// Retrieves the start band for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns the start band.
    pub(super) fn start_band(&self) -> u8 {
        self.start_band
    }

    /// Retrieves the stop band for the HBE transposer.
    ///
    /// # Return
    ///
    /// Returns the stop band.
    pub(super) fn stop_band(&self) -> u8 {
        self.stop_band
    }

    /// Apply the HBE transposition.
    ///
    /// This function applies the HBE transposition to the input audio signal,
    /// extending its bandwidth.
    ///
    /// # Parameters
    ///
    /// - `qmf_buffer_out_real`: A mutable reference to the output real part of the QMF buffer.
    /// - `qmf_buffer_out_imag`: A mutable reference to the output imaginary part of the QMF buffer.
    /// - `pitch_in_bins`: The pitch in bins.
    /// - `ov_len`: The overlap length.
    /// - `keep_states_synced_mode`: The mode for keeping states synced.
    pub(super) fn apply(
        &mut self,
        qmf_buffer_out_real: &mut [&mut [f32]],
        qmf_buffer_out_imag: &mut [&mut [f32]],
        pitch_in_bins: u8,
        ov_len: u8,
        mut keep_states_synced_mode: KeepStatesSyncedMode,
    ) {
        let mut slot_idx_in = self.qmf_in_buf_slot_index;
        let mut slot_idx_hbe = self.qmf_hbe_buf_slot_index;
        let qmf_vocoder_cols_in = self.no_cols / 2;
        let qmf_offset = 2 * self.kstart;
        let kstart_us: usize = self.kstart.into();
        let slot_offset_lin = 6;
        let b_pitch_in_bins = pitch_in_bins >= PMIN * (1 + if self.quad_rate { 1 } else { 0 });
        let p = pitch_in_bins as f32 / (12.0f32 * (1 + if self.quad_rate { 1 } else { 0 }) as f32);
        let mut skip = false;
        let mut offset = 0;
        let mut twid_m_new = [Complex::new(0.0f32, 0.0f32); 3];
        let start_col: usize = if self.quad_rate {
            (self.no_cols / 2).into()
        } else if keep_states_synced_mode != KeepStatesSyncedMode::NoOut {
            self.no_cols.into()
        } else {
            0
        };

        Self::calc_twid_m(&mut twid_m_new, pitch_in_bins, self.quad_rate);

        if keep_states_synced_mode == KeepStatesSyncedMode::NormalSkip {
            skip = true;
            keep_states_synced_mode = KeepStatesSyncedMode::Normal;
        }
        if keep_states_synced_mode != KeepStatesSyncedMode::Off {
            offset = self.no_cols - ov_len - LPC_ORDER as u8;
        }

        // Set out gain to 2.0f.
        self.hbe_synthesis_qmf.set_output_gain(2.0);

        let synth_size_us = usize::from(self.synth_size);

        // Time stretch.
        for j in 0..qmf_vocoder_cols_in {
            let j_us: usize = j.into();
            self.in_buf
                .copy_within((2 * synth_size_us)..(3 * synth_size_us), 0);

            // Set last two slot to zero.
            for i in 0..2 {
                let ih: usize = Self::mod_idx_hbe(slot_idx_hbe + HBE_MAX_OUT_SLOTS - 1 - i).into();
                self.qmf_hbe_buf[ih][usize::from(self.x_over_qmf[0])..]
                    .fill(Complex::new(0.0f32, 0.0f32));
            }

            // Run synthesis for two sbr slots as transposer uses half slots double bands
            // representation.
            let hbe_buffer = self.hbe_buffer.as_mut();
            let hbe_transposer_synthesis_qmf_pre_mod_cos = self.synthesis_qmf_pre_mod_cos.unwrap();
            let hbe_transposer_synthesis_qmf_pre_mod_sin = self.synthesis_qmf_pre_mod_sin.unwrap();
            for z in 0..2 {
                let mut qmf_buffer_codec_temp_slot = [0.0f32; HBE_MAX_QMF_SYN_BANDS as usize];

                for (cts, cos, sin, real, imag) in izip!(
                    qmf_buffer_codec_temp_slot.iter_mut(),
                    hbe_transposer_synthesis_qmf_pre_mod_cos.iter(),
                    hbe_transposer_synthesis_qmf_pre_mod_sin.iter(),
                    hbe_buffer.real[start_col + 2 * j_us + z][kstart_us..].iter(),
                    hbe_buffer.imag[start_col + 2 * j_us + z][kstart_us..].iter(),
                )
                .take(synth_size_us)
                {
                    *cts = self.sign_pre_mod * (*cos * *real + *sin * *imag);
                }

                self.hbe_synthesis_qmf.synthesis_filtering_slot(
                    &qmf_buffer_codec_temp_slot,
                    None,
                    &mut self.in_buf[synth_size_us * (z + 1)..],
                );
            }

            let mut qmf_in_buf_real = [0.0f32; HBE_QMF_SYNTH_CHANNELS as usize];
            let mut qmf_in_buf_imag = [0.0f32; HBE_QMF_SYNTH_CHANNELS as usize];

            self.hbe_analysis_qmf.analysis_filtering_slot(
                &mut qmf_in_buf_real,
                &mut qmf_in_buf_imag,
                &self.in_buf[1..],
            );

            let idx: usize = Self::mod_idx_in(slot_idx_in + HBE_QMF_WIN_LEN - 1).into();
            let num_subbands: usize = self.hbe_analysis_qmf.num_subbands().into();

            for (qmf, real, imag) in izip!(
                self.qmf_in_buf[idx].iter_mut(),
                qmf_in_buf_real.iter(),
                qmf_in_buf_imag.iter()
            )
            .take(num_subbands)
            {
                *qmf = Complex::new(*real, *imag);
            }

            if keep_states_synced_mode == KeepStatesSyncedMode::Normal
                && j <= qmf_vocoder_cols_in
                    - ((LPC_ORDER as u8 + ov_len + HBE_QMF_WIN_LEN - 1) >> 1)
            {
                // Update slot index to input buffer.
                slot_idx_in = Self::mod_idx_in(slot_idx_in + 1);
                continue;
            }

            if !skip {
                // Pre-calculate qmf_in_buf energies which are needed multiple times.
                let mut qmf_in_buf_energy = [0.0f32; HBE_QMF_SYNTH_CHANNELS as usize];
                // Full range needed for cross-products below.
                let sourceband_min = 0;
                let sourceband_max = 2 * synth_size_us;
                let slot_offset: usize = Self::mod_idx_in(slot_idx_in + slot_offset_lin).into();

                for (e, data_in) in izip!(
                    qmf_in_buf_energy[sourceband_min..sourceband_max].iter_mut(),
                    self.qmf_in_buf[slot_offset][sourceband_min..sourceband_max].iter()
                ) {
                    *e = Self::calc_nrg(*data_in);
                }

                qmf_in_buf_energy[sourceband_max..usize::from(HBE_QMF_SYNTH_CHANNELS)]
                    .fill(HBE_MIN_NRG);

                for stretch in 2..=4 {
                    self.stretch(
                        stretch,
                        qmf_offset,
                        &qmf_in_buf_energy,
                        slot_offset,
                        slot_offset_lin,
                        slot_idx_in,
                        slot_idx_hbe,
                        b_pitch_in_bins,
                        p,
                        &twid_m_new,
                    );
                }
            }
            debug_assert!(self.x_over_qmf[0] == self.start_band);
            if keep_states_synced_mode != KeepStatesSyncedMode::NoOut
                && 2 * j >= offset
                && self.stop_band > self.start_band
            {
                // Copy first two slots of internal buffer to output.
                let write_offset = if keep_states_synced_mode == KeepStatesSyncedMode::OutDiff {
                    -(i16::from(offset))
                } else {
                    i16::from(ov_len)
                };
                let tmp_index = 2 * i32::from(j) + i32::from(write_offset);
                let start = usize::from(self.start_band);
                let end = usize::from(self.stop_band);
                for i in 0..2 {
                    let ih: usize = Self::mod_idx_hbe(slot_idx_hbe + i as u8).into();
                    let qmf_slot = usize::try_from(tmp_index + i).unwrap();

                    for (q_re, q_im, q_hbe, cos_re, cos_im) in izip!(
                        qmf_buffer_out_real[qmf_slot][start..].iter_mut(),
                        qmf_buffer_out_imag[qmf_slot][start..].iter_mut(),
                        self.qmf_hbe_buf[ih][start..].iter(),
                        COS_64[start..end].iter(),
                        COS_64[64 - end..64 - start].iter().rev()
                    )
                    .take(end - start)
                    {
                        let c = Complex {
                            re: *cos_re,
                            im: -(*cos_im),
                        } * q_hbe;
                        *q_re = c.re;
                        *q_im = c.im;
                    }
                }
            }

            slot_idx_in = Self::mod_idx_in(slot_idx_in + 1);
            slot_idx_hbe = Self::mod_idx_hbe(slot_idx_hbe + 2);
        }

        self.qmf_in_buf_slot_index = slot_idx_in;
        self.qmf_hbe_buf_slot_index = slot_idx_hbe;
    }

    /// Stretch logic.
    ///
    /// Encapsulated stretch logic to handle different stretch values into a single function.
    #[expect(clippy::too_many_arguments)]
    fn stretch(
        &mut self,
        stretch: u8,
        qmf_offset: u8,
        qmf_in_buf_energy: &[f32],
        slot_offset: usize,
        slot_offset_lin: u8,
        slot_idx_in: u8,
        slot_idx_hbe: u8,
        b_pitch_in_bins: bool,
        p: f32,
        twid_m_new: &[Complex<f32>],
    ) {
        if stretch <= self.max_stretch {
            // slot_offset - win_length[stretch - 2] / 2;
            let start = stretch - 1;
            // slot_offset + win_length[stretch - 2] / 2;
            let stop = 13 - stretch;

            let slot_stretch: &[u8] = if stretch == 3 {
                &SLOT_STRETCH3
            } else {
                &SLOT_STRETCH4
            };

            for band in
                self.x_over_qmf[usize::from(stretch) - 2]..self.x_over_qmf[usize::from(stretch) - 1]
            {
                let band_us: usize = band.into();
                let mut gamma_center = [Complex::new(0.0f32, 0.0f32); 2];
                // Interpolation filters for 3rd order.
                let mut sourceband: usize = (2 * band / stretch - qmf_offset).into();
                debug_assert!(sourceband < HBE_QMF_SYNTH_CHANNELS.into());
                if stretch == 2 {
                    gamma_center[0] = Self::calculate_center(
                        qmf_in_buf_energy[sourceband],
                        self.qmf_in_buf[slot_offset][sourceband],
                        stretch,
                        stretch,
                    );

                    for k in start..stop {
                        let ki: usize = Self::mod_idx_in(slot_idx_in + k).into();
                        let kh: usize = Self::mod_idx_hbe(slot_idx_hbe + k).into();
                        Self::add_high_band_part(
                            &self.qmf_in_buf[ki][sourceband..sourceband + 1],
                            &gamma_center,
                            &mut self.qmf_hbe_buf[kh][band_us],
                            stretch,
                            0,
                        );
                    }
                } else {
                    let mut gamma_vec = [Complex::new(0.0f32, 0.0f32); 2];
                    let r = (5 - stretch) * band
                        - (6 - stretch) * ((5 - stretch) * band / (6 - stretch));

                    let tab_idx: usize = (sourceband % 4) * 2;

                    let rl = if r == 2 { 2 } else { 1 };
                    for (i, g) in gamma_center[0..rl].iter_mut().enumerate() {
                        *g = Self::calculate_center(
                            qmf_in_buf_energy[sourceband + i],
                            self.qmf_in_buf[slot_offset][sourceband + i],
                            stretch,
                            stretch,
                        );
                        if stretch == 4 && i == 0 {
                            if r == 0 {
                                sourceband -= 1
                            } else {
                                sourceband += 1
                            };
                            debug_assert!(sourceband + 1 < HBE_QMF_SYNTH_CHANNELS.into());
                        }
                    }
                    for k in start..stop {
                        let k_us: usize = k.into();
                        let ki: usize = Self::mod_idx_in(slot_idx_in + slot_stretch[k_us]).into();
                        let kh: usize = Self::mod_idx_hbe(slot_idx_hbe + k).into();
                        gamma_vec[0] = self.qmf_in_buf[ki][sourceband];
                        if r == 2 {
                            gamma_vec[1] = self.qmf_in_buf[ki][sourceband + 1];
                        }
                        if stretch == 3 && k % 2 != 0 {
                            let kki: usize =
                                Self::mod_idx_in(slot_idx_in + slot_stretch[k_us] + 1).into();
                            let mut tmp = self.qmf_in_buf[kki][sourceband];
                            tmp *= HINT1_CPLX[tab_idx];
                            gamma_vec[0] *= HINT1_CPLX[tab_idx + 1];
                            gamma_vec[0] += tmp;

                            if r == 2 {
                                tmp = self.qmf_in_buf[kki][sourceband + 1];
                                tmp *= HINT1_CPLX[tab_idx + 2];
                                gamma_vec[1] *= HINT1_CPLX[tab_idx + 3];
                                gamma_vec[1] += tmp;
                            }
                        }
                        Self::add_high_band_part(
                            &gamma_vec,
                            &gamma_center,
                            &mut self.qmf_hbe_buf[kh][band_us],
                            stretch,
                            r,
                        );
                    }
                }
                // pitchInBins is given with the resolution of a 768 bins FFT and we
                // need 64 QMF units, so factor 768/64 = 12.
                if b_pitch_in_bins {
                    Self::pitch_in_bins_calc(
                        band,
                        stretch,
                        p,
                        qmf_offset,
                        self.synth_size,
                        slot_idx_in + slot_offset_lin,
                        slot_idx_hbe + slot_offset_lin,
                        twid_m_new,
                        qmf_in_buf_energy,
                        &self.qmf_in_buf,
                        &mut self.qmf_hbe_buf,
                    );
                }
            }
        }
    }

    /// Calculates modulo index from linear index for hbe spectrum.
    ///
    /// # Parameters
    ///
    /// - `idx`: The linear index.
    ///
    /// # Return
    ///
    /// Returns the remainder
    fn mod_idx_in(idx: u8) -> u8 {
        idx % HBE_QMF_IN_BUF_SIZE
    }

    /// Calculates modulo index from linear index for hbe spectrum.
    ///
    /// # Parameters
    ///
    /// - `idx`: The linear index.
    ///
    /// # Return
    ///
    /// Returns the remainder
    fn mod_idx_hbe(idx: u8) -> u8 {
        idx % HBE_MAX_OUT_SLOTS
    }

    /// Fills twid_m table.
    ///
    /// # Parameters
    ///
    /// - `twid_m`: Linear index.
    /// - `pitch_in_bins`: Pitch value in bins.
    /// - `quad_rate`: Indicates quad rate mode.
    fn calc_twid_m(twid_m: &mut [Complex<f32>; 3], pitch_in_bins: u8, quad_rate: bool) {
        let stepsize: u16 = 1 + if quad_rate { 0 } else { 1 };
        for (i, tw) in twid_m[0..usize::from(HBE_MAX_STRETCH) - 1]
            .iter_mut()
            .enumerate()
        {
            let idx: u16 = ((i as u16 + 1) * (stepsize * u16::from(pitch_in_bins)))
                % u16::from(HBE_TWID_M_MOD);
            if idx < u16::from(HBE_TWID_M_MOD) / 2 {
                *tw = TWIDDLE_CPLX[usize::from(idx)];
            } else {
                *tw = TWIDDLE_CPLX[usize::from(idx - u16::from(HBE_TWID_M_MOD) / 2)] * -1.0;
            }
        }
    }

    /// Calculates the energy with a minimum energy of HBE_MIN_NRG.
    ///
    /// # Parameters
    ///
    /// - `in`: Complex spectral value.
    ///
    /// # Return
    ///
    /// Returns the energy value.
    #[inline(always)]
    fn calc_nrg(v: Complex<f32>) -> f32 {
        v.norm_sqr() + HBE_MIN_NRG
    }

    /// Calculates center.
    ///
    /// # Parameters
    ///
    /// - `nrg`: Energy value.
    /// - 'gamma_vec`: Vector complex value.
    /// - `stretch`: Stretch value.
    /// - `mult`: Weighting factor.
    ///
    /// # Return
    /// 'gamma_center`: Center complex value.
    fn calculate_center(nrg: f32, gamma_vec: Complex<f32>, stretch: u8, mult: u8) -> Complex<f32> {
        debug_assert!(stretch == 2 || stretch == 3 || stretch == 4);
        debug_assert!(mult == 2 || mult == 3 || mult == 4);

        let factor = Self::calc_factor(nrg, 1.0, stretch);

        let gc = gamma_vec * factor;
        if mult == 2 {
            gc
        } else if mult == 3 {
            gc * gc
        } else {
            gc * gc * gc
        }
    }

    /// Calculates factor divided by some root of nrg.
    ///
    /// # Parameters
    ///
    /// - `nrg`: Energy value.
    /// - `mult`: Weighting factor.
    /// - `stretch`: Stretch value.
    #[inline(always)]
    fn calc_factor(nrg: f32, mult: f32, stretch: u8) -> f32 {
        if stretch == 2 {
            mult / (nrg.sqrt()).sqrt()
        } else if stretch == 3 {
            mult * nrg.powf(-1.0 / 3.0)
        } else {
            let sqrt4 = nrg.sqrt().sqrt();
            let sqrt8 = sqrt4.sqrt();
            mult / (sqrt4 * sqrt8)
        }
    }

    /// Adds high band part.
    ///
    /// # Parameters
    ///
    /// - `g`: Complex spectral value.
    /// - `gamma_center`: Center complex value.
    /// - `qmf_hbebuf`: Output buffer.
    /// - `stretch`: Stretch value.
    /// - `r`: R value.
    fn add_high_band_part(
        g: &[Complex<f32>],
        gamma_center: &[Complex<f32>],
        qmf_hbebuf: &mut Complex<f32>,
        stretch: u8,
        r: u8,
    ) {
        let mut factor;
        let mut tmp;
        if r == 2 {
            debug_assert!(stretch == 3 || stretch == 4);
            let mult = 1.4142f32 / 6.0f32;
            factor = Self::calc_factor(Self::calc_nrg(g[0]), mult, stretch);
            tmp = g[0] * gamma_center[1];
            *qmf_hbebuf += tmp * factor;
            factor = Self::calc_factor(Self::calc_nrg(g[1]), mult, stretch);
            tmp = g[1] * gamma_center[0];
            *qmf_hbebuf += tmp * factor;
        } else {
            debug_assert!(stretch == 2 || stretch == 3 || stretch == 4);
            let mult = if stretch == 2 {
                1.0f32 / 3.0f32
            } else if stretch == 3 {
                1.4142f32 / 3.0f32
            } else {
                2.0f32 / 3.0f32
            };
            factor = Self::calc_factor(Self::calc_nrg(g[0]), mult, stretch);
            tmp = g[0] * gamma_center[0];
            *qmf_hbebuf += tmp * factor;
        }
    }

    /// Calculates pitch in bins.
    ///
    /// # Parameters
    ///
    /// - `band`: Band value.
    /// - `stretch`: Stretch value.
    /// - `p`: Pitch value.
    /// - `qmf_offset`: QMF offset.
    /// - `synth_size`: Synthesis size.
    /// - `slot_idx_in`: Input slot index.
    /// - `slot_idx_hbe`: HBE slot index.
    /// - `twid_m_new`: New twiddle values.
    /// - `qmf_in_buf_nrg`: Input buffer energy.
    /// - `qmf_in_buf`: Input buffer complex.
    /// - `qmf_hbebuf`: Output buffer complex.
    #[expect(clippy::too_many_arguments)]
    fn pitch_in_bins_calc(
        band: u8,
        stretch: u8,
        p: f32,
        qmf_offset: u8,
        synth_size: u8,
        slot_idx_in: u8,
        slot_idx_hbe: u8,
        twid_m_new: &[Complex<f32>],
        qmf_in_buf_nrg: &[f32],
        qmf_in_buf: &[[Complex<f32>; HBE_QMF_SYNTH_CHANNELS as usize]],
        qmf_hbe_buf: &mut [[Complex<f32>; HBE_QMF_SYNTH_CHANNELS as usize]],
    ) {
        let band_us: usize = band.into();
        let stretch_us: usize = stretch.into();
        let synth_size_us: usize = synth_size.into();
        let mut m_tr = 0;
        let mut ts1: usize = 0;
        let mut ts2: usize = 0;
        let mut m_val = 0.0f32;
        let mut sign = -1.0f32;
        let mut gamma_vec = [Complex::new(0.0f32, 0.0f32), Complex::new(0.0f32, 0.0f32)];
        let mut gamma_center: Complex<f32>;

        debug_assert!((2..=4).contains(&stretch));

        let table_value = TABLE_STRETCH[stretch_us - 2];

        let sourceband = 2 * band / stretch - qmf_offset;

        let sqmag0 = qmf_in_buf_nrg[usize::from(sourceband)];
        let slot_offset: usize = Self::mod_idx_in(slot_idx_in).into();

        debug_assert!(qmf_offset <= i8::MAX as u8);
        debug_assert!(synth_size <= i8::MAX as u8);
        let qmf_offset_i8 = qmf_offset as i8;
        let synth_size_i8 = synth_size as i8;

        const BETTER_MATCH_REFERENCE_SOFTWARE: bool = false;

        for tr in 1..stretch {
            let ti1: i8;
            let ti2: i8;
            if BETTER_MATCH_REFERENCE_SOFTWARE {
                let f1 = (2.0f64 * band as f64 + 1.0f64 - tr as f64 * p as f64) / stretch as f64;

                ti1 = f1 as i8 - qmf_offset_i8;
                ti2 = (f1 + p as f64) as i8 - qmf_offset_i8;
            } else {
                let f1 = (2.0f32 * band as f32 + 1.0f32 - tr as f32 * p) * table_value;

                ti1 = f1 as i8 - qmf_offset_i8;
                ti2 = (f1 + p) as i8 - qmf_offset_i8;
            }

            if ti1 >= 0 && ti2 < 2 * synth_size_i8 {
                let sqmag1 = qmf_in_buf_nrg[ti1 as usize];
                let sqmag2 = qmf_in_buf_nrg[usize::try_from(ti2).unwrap()];

                let temp = sqmag1.min(sqmag2);

                if temp > m_val {
                    m_val = temp;
                    m_tr = tr;
                    ts1 = ti1 as usize;
                    ts2 = ti2.try_into().unwrap();
                }
            }
        }

        if m_val > sqmag0 && ts2 < 2 * synth_size_us {
            let mut tcenter = stretch - m_tr;
            let mut tvec = m_tr;

            if stretch == 2 {
                gamma_center = qmf_in_buf[slot_offset][ts1];

                for (k, g) in gamma_vec.iter_mut().enumerate() {
                    let ki: usize = Self::mod_idx_in(slot_idx_in - 1 + k as u8).into();
                    *g = qmf_in_buf[ki][ts2];
                }
            } else if stretch == 3 {
                let mut idx_center = ts1;
                let mut idx_vec = ts2;
                if m_tr == 2 {
                    sign = 1.0f32;
                    tcenter = m_tr;
                    tvec = stretch - m_tr;
                    idx_center = ts2;
                    idx_vec = ts1;
                }

                let tab_idx = (idx_vec % 4) * 2;

                gamma_center = qmf_in_buf[slot_offset][idx_center];
                gamma_vec[1] = qmf_in_buf[slot_offset][idx_vec];

                let var_name: usize = Self::mod_idx_in(slot_idx_in - 2).into();
                gamma_vec[0] = qmf_in_buf[var_name][idx_vec] * HINT2_CPLX[tab_idx];

                let tmp = qmf_in_buf[usize::from(Self::mod_idx_in(slot_idx_in - 1))][idx_vec]
                    * HINT2_CPLX[tab_idx + 1];
                gamma_vec[0] += tmp;
            } else if m_tr == 1 {
                gamma_center = qmf_in_buf[slot_offset][ts1];

                for (k, g) in gamma_vec.iter_mut().enumerate() {
                    let ki: usize = Self::mod_idx_in(
                        u8::try_from(i16::from(slot_idx_in) + 2 * (k as i16 - 1)).unwrap(),
                    )
                    .into();
                    *g = qmf_in_buf[ki][ts2]
                }
            } else if m_tr == 2 {
                gamma_center = qmf_in_buf[slot_offset][ts1];
                for (k, g) in gamma_vec.iter_mut().enumerate() {
                    let ki: usize = Self::mod_idx_in(
                        u8::try_from(i16::from(slot_idx_in) + k as i16 - 1).unwrap(),
                    )
                    .into();
                    *g = qmf_in_buf[ki][ts2]
                }
            } else {
                sign = 1.0f32;
                tcenter = m_tr;
                tvec = stretch - m_tr;

                gamma_center = qmf_in_buf[slot_offset][ts2];

                for (k, g) in gamma_vec.iter_mut().enumerate() {
                    let ki: usize = Self::mod_idx_in(
                        u8::try_from(i16::from(slot_idx_in) + 2 * (k as i16 - 1)).unwrap(),
                    )
                    .into();
                    *g = qmf_in_buf[ki][ts1]
                }
            }

            // Parameter controlled phase modification parts.
            gamma_center = Self::calculate_center(
                Self::calc_nrg(gamma_center),
                gamma_center,
                stretch,
                tcenter + 2 - 1,
            );
            gamma_vec[0] = Self::calculate_center(
                Self::calc_nrg(gamma_vec[0]),
                gamma_vec[0],
                stretch,
                tvec + 2 - 1,
            );
            gamma_vec[1] = Self::calculate_center(
                Self::calc_nrg(gamma_vec[1]),
                gamma_vec[1],
                stretch,
                tvec + 2 - 1,
            );

            // Final multiplication of prepared parts.
            let mut gamma_out = [Complex::new(0.0f32, 0.0f32), Complex::new(0.0f32, 0.0f32)];
            for (k, g) in gamma_out[0..2].iter_mut().enumerate() {
                *g = gamma_vec[k] * gamma_center
            }

            let modstretch4 = (stretch == 4) && (m_tr == 2);
            let mut twid = twid_m_new[usize::from(stretch) - 2 - usize::from(modstretch4)];
            twid.im *= sign;

            gamma_out[0] *= twid;

            // OLA including window scaling.
            for (k, g) in gamma_out[0..2].iter_mut().enumerate() {
                let kh: usize = Self::mod_idx_hbe(
                    u8::try_from(i16::from(slot_idx_hbe) + k as i16 - 1).unwrap(),
                )
                .into();
                qmf_hbe_buf[kh][band_us] += *g * constants::WINGAIN[stretch_us - 2]
            }
        }
    }
}
