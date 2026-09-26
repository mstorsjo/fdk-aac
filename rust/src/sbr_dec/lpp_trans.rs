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
//! Low power profile (LPP) transposer

use super::common::{self as sbr_common, SbrError};
use super::constants::{
    LPC_ORDER, MAX_ENV_COLS, MAX_NUM_NOISE_VALUES, MAX_NUM_PATCHES, MAX_OV_COLS,
    SBRDEC_MAX_ANALYSIS_CHANNELS, SBRDEC_MAX_SUBBAND_SUBSAMPLES,
};
use super::hf_gen_preflat;
use crate::common::{
    autocorr2nd::AutocorrCoeffs,
    qmf_domain::{QMF_DOMAIN_MAX_OV_TIMESLOTS, QMF_DOMAIN_MAX_TIMESLOTS},
};
use itertools::izip;
use num_complex::{Complex, ComplexFloat};
use std::array;
use std::ops::Neg;

/// Lowest subband of source range.
const SHIFT_START_SB: i16 = 1;
/// Number of whitening factors.
const NUM_WHFACTOR_TABLE_ENTRIES: usize = 9;

#[repr(C)]
#[derive(Debug, Copy, Clone, Default, PartialEq)]
pub enum InvfMode {
    #[default]
    Off = 0,
    LowLevel,
    MidLevel,
    HighLevel,
    Switched, // Not a real choice but used here to control behaviour.
}

#[derive(Debug)]
pub struct InvfModeError {}

impl TryFrom<u32> for InvfMode {
    type Error = InvfModeError;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(InvfMode::Off),
            1 => Ok(InvfMode::LowLevel),
            2 => Ok(InvfMode::MidLevel),
            3 => Ok(InvfMode::HighLevel),
            _ => Err(InvfModeError {}),
        }
    }
}

/// Parameter set for one single patch.
#[repr(C)]
#[derive(Default, Debug)]
pub struct PatchParam {
    /// First band in lowbands where to take the samples from.
    source_start_band: u8,
    /// First band in lowbands which is not included in the patch anymore.
    source_stop_band: u8,
    /// First band in highbands to be filled with zeros in order to reduce interferences between
    /// patches.
    guard_start_band: u8,
    /// First band in highbands to be filled with whitened lowband signal.
    target_start_band: u8,
    /// Difference between 'target_start_band' and 'source_start_band'.
    target_band_offs: u8,
    /// Number of consecutive bands in this one patch.
    num_bands_in_patch: u8,
}

impl PatchParam {
    /// Returns guard start band value.
    pub(super) fn guard_start_band(&self) -> u8 {
        self.guard_start_band
    }
}

/// Whitening factors for different levels of whitening
/// need to be initialized corresponding to crossover frequency.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
struct WhiteningFactors {
    /// Bw factor for signal OFF.
    off: f32,
    /// Transition level.
    transition_level: f32,
    /// Bw factor for signal LOW_LEVEL.
    low_level: f32,
    /// Bw factor for signal MID_LEVEL.
    mid_level: f32,
    /// Bw factor for signal HIGH_LEVEL.
    high_level: f32,
}

// Coefficients for spectral whitening in the transposer.
// Assignment of whitening tuning depending on the crossover frequency.
#[rustfmt::skip]
static WHITENING_FACTORS_INDEX:[u16;NUM_WHFACTOR_TABLE_ENTRIES] = [
    0, 5000, 6000, 6500, 7000, 7500, 8000, 9000, 10000];

// Whitening levels tuning table.
// With the current tuning, NUM_WHFACTOR_TABLE_ENTRIES has been reduced to 1.
static WHITENING_FACTORS_TABLE: [WhiteningFactors; 1] = [
    // Cross over frquency band ranges. This table contains only a single entry because all band
    // ranges have the same whitening factors. Otherwise, the table would list whitening factors
    // for each band range of cross frequencies.
    // 0  < 5000, 5000 < 6000, 6000 < 6500, 6500 < 7000, 7000 < 7500, 7500 < 8000, 8000 < 9000,
    // 9000 < 10000,  > 10000.
    WhiteningFactors {
        off: 0.00,
        transition_level: 0.6,
        low_level: 0.75,
        mid_level: 0.90,
        high_level: 0.98,
    },
];

impl WhiteningFactors {
    /// Gets bandwidth expansion factor depending on the desired filtering
    /// level signalled in the bitstream. When switching the filtering level
    /// from LOW to OFF, an additional level is being inserted to achieve a
    /// smooth transition.
    ///
    /// # Parameters
    ///
    /// - `mode`: Current inverse filtering modes.
    /// - `prev_mode`: Previous inverse filtering modes.
    ///
    /// # Return
    ///
    ///  `Bandwidth expansion factor`
    fn map_invf_mode(&self, mode: InvfMode, prev_mode: InvfMode) -> f32 {
        match mode {
            InvfMode::LowLevel => match prev_mode {
                InvfMode::Off => self.transition_level,
                _ => self.low_level,
            },
            InvfMode::MidLevel => self.mid_level,
            InvfMode::HighLevel => self.high_level,
            InvfMode::Switched | InvfMode::Off => match prev_mode {
                InvfMode::LowLevel => self.transition_level,
                _ => self.off,
            },
        }
    }

    /// Selects `whitening_factors` table based on 'sbr_fs` and `subband_start`.
    /// And updates `self` data based on selected table values.
    ///
    /// # Parameters
    ///
    /// - `sbr_fs`: SBR sampling rate.
    /// - `subband_start`: Subband start in high band data.
    fn select_whitening_factors(&mut self, sbr_fs: u32, subband_start: u8) {
        let start_freq_hz = (u32::from(subband_start) * sbr_fs) >> 7; // Division by 2*(64).
        let _index = WHITENING_FACTORS_INDEX[1..NUM_WHFACTOR_TABLE_ENTRIES]
            .iter()
            .position(|&wf_val| start_freq_hz < wf_val.into())
            .unwrap_or(NUM_WHFACTOR_TABLE_ENTRIES - 1);

        // In case of fine tuning WHITENING_FACTORS_TABLE[] in future, this copy has to be
        // adapted and use `index` to select right value from WHITENING_FACTORS_TABLE[].
        *self = WHITENING_FACTORS_TABLE[0];
    }
}

/// The transposer settings are calculated on a header reset
/// and are shared by both channels.
#[repr(C)]
#[derive(Default, Debug)]
pub struct TransposerSettings {
    /// Number of subsamples of a codec frame.
    num_columns: u8,
    /// Number of patches.
    num_of_patches: u8,
    /// First band of lowbands that will be patched.
    lb_start_patching: u8,
    /// First band that won't be patched anymore.
    lb_stop_patching: u8,
    /// Spectral bands with different inverse filtering levels.
    bw_borders: [u8; MAX_NUM_NOISE_VALUES],
    /// New parameter set for patching.
    patch_param: [PatchParam; MAX_NUM_PATCHES + 1],
    /// The pole moving factors for certain whitening levels as indicated in the bitstream
    /// depending on the crossover frequency.
    white_factors: WhiteningFactors,
    /// Overlap size.
    overlap_size: u8,
}

impl TransposerSettings {
    /// Creates a new instance of `TransposerSettings`.
    ///
    /// # Return
    ///
    /// A new instance of `TransposerSettings` with default values.
    pub fn new() -> Self {
        Default::default()
    }

    /// Returns `number of columns` in `transposer`.
    pub(super) fn _num_columns(&self) -> u8 {
        self.num_columns
    }

    /// Returns `overlap size` of `transposer`.
    pub(super) fn overlap_size(&self) -> u8 {
        self.overlap_size
    }

    /// Returns number of patches in `transposer`.
    pub(super) fn num_of_patches(&self) -> u8 {
        self.num_of_patches
    }

    /// Returns first band of lowbands that will be patched.
    pub(super) fn lb_start_patching(&self) -> u8 {
        self.lb_start_patching
    }

    /// Returns first band that won't be patched anymore.
    pub(super) fn lb_stop_patching(&self) -> u8 {
        self.lb_stop_patching
    }

    /// Returns parameter set for patching.
    pub(super) fn patch_param(&self) -> &[PatchParam; MAX_NUM_PATCHES + 1] {
        &self.patch_param
    }

    /// Finds closest value to `goal_sb` in `v_k_master` table.
    ///
    /// # Parameters
    ///
    /// - `v_k_master`: Master table.
    /// - `goal_sb`: Target subband value.
    /// - `is_forward_direction`: Flag to indicate direction to traverse (`true` - forward, `false`
    ///   - backward) in `v_k_master` table.
    fn find_closest_entry(v_k_master: &[u8], goal_sb: u32, is_forward_direction: bool) -> u8 {
        let num_master = v_k_master.len() - 1;
        if goal_sb <= u32::from(v_k_master[0]) {
            return v_k_master[0];
        }
        if goal_sb >= u32::from(v_k_master[num_master]) {
            return v_k_master[num_master];
        }

        let goal_sb_loc = goal_sb as u8;
        let index = if is_forward_direction {
            v_k_master
                .iter()
                .position(|x| *x >= goal_sb_loc)
                .unwrap_or(0)
        } else {
            v_k_master
                .iter()
                .rposition(|x| *x <= goal_sb_loc)
                .unwrap_or(num_master)
        };

        v_k_master[index]
    }

    /// Initializes one `low power profile` transposer instance.
    ///
    /// # Parameters
    ///
    /// - `v_k_master`: Master frequency resolution table.
    /// - `noise_band_table`: Mapping of SBR noise bands to QMF bands.
    /// - `time_slots`: Number of SBR envelope time slots that exist within an AAC frame, 16 for a
    ///   1024 AAC frame and 15 for a 960 AAC frame.
    /// - `fs`: Sampling frequency.
    /// - `high_band_start_sb`: Highband area start subband.
    /// - `usb`: Highband area stop subband.
    /// - `num_columns`: Number of columns.
    /// - `overlap_size`: Overlap size of transposer.
    ///
    /// # Return
    ///
    /// `SbrError`
    #[expect(clippy::too_many_arguments)]
    pub fn init(
        &mut self,
        v_k_master: &[u8],
        noise_band_table: &[u8],
        time_slots: usize,
        fs: u32,
        high_band_start_sb: u8,
        usb: u8,
        num_columns: u8,
        overlap_size: u8,
    ) -> SbrError {
        self.num_columns = num_columns;
        self.overlap_size = overlap_size;

        match time_slots {
            15 | 16 => self.reset(v_k_master, noise_band_table, fs, high_band_start_sb, usb),
            _ => SbrError::UnsupportedConfig,
        }
    }

    /// Constructs patches to use in transposition, where low band consecutive subbands
    /// are patched to high band consecutive subbands.
    ///
    /// # Parameters
    ///
    /// - `v_k_master`: Master table.
    /// - `sbr_fs`: SBR sample rate.
    /// - `subband_start`: Highband area start subband.
    /// - `usb`: Highband area stop subband.
    ///
    /// # Return
    ///
    /// `SbrError`
    fn patch_construction(
        &mut self,
        v_k_master: &[u8],
        sbr_fs: u32,
        subband_start: u8,
        usb: u8,
    ) -> SbrError {
        let lsb = i16::from(v_k_master[0]);
        // Calculate distance in QMF bands between k0 and kx.
        let xover_offset = i16::from(subband_start) - lsb;

        // Initialize the patching parameter.
        // ISO/IEC 14496-3 (Figure 4.48): goalSb = round( 2.048e6 / fs ).
        let desired_border = (((2048000 * 2) / sbr_fs) + 1) >> 1;

        // Adapt region to master-table.
        let mut desired_border =
            i16::from(Self::find_closest_entry(v_k_master, desired_border, true));

        // First patch.
        let mut source_start_band = SHIFT_START_SB + xover_offset;
        // Upper band.
        let mut target_stop_band = lsb + xover_offset;

        // Even (odd) numbered channel must be patched to even (odd) numbered channel.
        let mut patch = 0;
        let mut num_bands_in_patch;
        let usb = i16::from(v_k_master[v_k_master.len() - 1].min(usb));

        while target_stop_band < usb {
            if patch > MAX_NUM_PATCHES {
                return SbrError::UnsupportedConfig;
            }

            let pp: &mut PatchParam = &mut self.patch_param[patch];
            pp.guard_start_band = target_stop_band.try_into().unwrap();
            pp.target_start_band = pp.guard_start_band;

            // Get the desired range of the patch
            num_bands_in_patch = desired_border - target_stop_band;

            if num_bands_in_patch >= (lsb - source_start_band) {
                // Desired number bands are not available -> patch whole source range.
                // Get the target_offset.
                let mut patch_distance = target_stop_band - source_start_band;
                // Rounding off odd numbers and make all even.
                patch_distance &= !1;

                // Update number of bands to be patched.
                num_bands_in_patch = lsb - (target_stop_band - patch_distance);

                // Adapt region to master-table.
                num_bands_in_patch = i16::from(Self::find_closest_entry(
                    v_k_master,
                    (num_bands_in_patch + target_stop_band).try_into().unwrap(),
                    false,
                )) - target_stop_band;
            }

            if self.num_columns == 64
                && num_bands_in_patch == 0
                && source_start_band == SHIFT_START_SB
            {
                return SbrError::UnsupportedConfig;
            }

            // Desired number bands are available -> get the minimal even patching distance.
            // Get minimal distance.
            let mut patch_distance = num_bands_in_patch + target_stop_band - lsb;
            // Rounding up odd numbers and make all even.
            patch_distance = (patch_distance + 1) & !1;

            if num_bands_in_patch > 0 {
                pp.source_start_band = (target_stop_band - patch_distance).try_into().unwrap();
                pp.target_band_offs = patch_distance.try_into().unwrap();
                pp.num_bands_in_patch = num_bands_in_patch.try_into().unwrap();
                pp.source_stop_band = (i16::from(pp.source_start_band) + num_bands_in_patch)
                    .try_into()
                    .unwrap();

                target_stop_band += i16::from(pp.num_bands_in_patch);
                patch += 1;
            }

            // All patches but first.
            source_start_band = SHIFT_START_SB;

            // Check if we are close to desired_border.
            if desired_border - target_stop_band < 3 {
                desired_border = usb;
            }
        }

        // If highest patch contains less than three subband: skip it.
        if (patch > 1) && (self.patch_param[patch - 1].num_bands_in_patch < 3) {
            patch -= 1;
            target_stop_band = (self.patch_param[patch - 1].target_start_band
                + self.patch_param[patch - 1].num_bands_in_patch)
                .into();
        }

        // Now check if we don't have one too many.
        if patch > MAX_NUM_PATCHES {
            return SbrError::UnsupportedConfig;
        }

        self.num_of_patches = patch as u8;

        // Check lowest and highest source subband.
        self.lb_start_patching = target_stop_band.try_into().unwrap();
        self.lb_stop_patching = 0;
        for pp in self
            .patch_param
            .iter()
            .take(usize::from(self.num_of_patches))
        {
            self.lb_start_patching = self.lb_start_patching.min(pp.source_start_band);
            self.lb_stop_patching = self.lb_stop_patching.max(pp.source_stop_band);
        }

        SbrError::Ok
    }

    /// Initializes memory for one `low power profile` transposer instance.
    ///
    /// # Parameters
    ///
    /// - `v_k_master`: Master table.
    /// - `noise_band_table`: Mapping of SBR noise bands to QMF bands.
    /// - `fs`: SBR sample rate.
    /// - `high_band_start_sb`: Highband area start subband.
    /// - `usb`: Highband area stop subband.
    ///
    /// # Return
    ///
    /// `SbrError`
    pub fn reset(
        &mut self,
        v_k_master: &[u8],
        noise_band_table: &[u8],
        fs: u32,
        high_band_start_sb: u8,
        usb: u8,
    ) -> SbrError {
        let lsb = i16::from(v_k_master[0]);

        // Plausibility check.
        if self.num_columns == 64 {
            if lsb < 4 {
                // 4:1 SBR Requirement k0 >= 4 missed.
                return SbrError::UnsupportedConfig;
            }
        } else if lsb - SHIFT_START_SB < 4 {
            return SbrError::UnsupportedConfig;
        }

        // Construct patches.
        if SbrError::Ok != self.patch_construction(v_k_master, fs, high_band_start_sb, usb) {
            return SbrError::UnsupportedConfig;
        }

        // Copy noise bands - mapping SBR noise bands to QMF bands. Later used in transposer().
        let num_noise_bands = noise_band_table.len() - 1;
        self.bw_borders[..num_noise_bands].copy_from_slice(&noise_band_table[1..=num_noise_bands]);
        self.bw_borders[num_noise_bands..MAX_NUM_NOISE_VALUES].fill(255);

        // Choose whitening factors.
        self.white_factors
            .select_whitening_factors(fs, high_band_start_sb);

        SbrError::Ok
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct LppTransposer {
    /// Pole moving factors of past frame.
    bw_vector_old: [f32; MAX_NUM_PATCHES],
    /// Buffer to save filter states for legacy SBR.
    lpc_filter_states_leg_sbr:
        [[Complex<f32>; SBRDEC_MAX_ANALYSIS_CHANNELS]; LPC_ORDER + MAX_OV_COLS],
    /// Buffer to save filter states for real HBE.
    lpc_filter_states_real_hbe: [[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
    /// Buffer to save filter states for imaginary HBE.
    lpc_filter_states_imag_hbe: [[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
}

impl Default for LppTransposer {
    fn default() -> Self {
        LppTransposer {
            bw_vector_old: Default::default(),
            lpc_filter_states_leg_sbr: Default::default(),
            lpc_filter_states_real_hbe: [[0.0; SBRDEC_MAX_SUBBAND_SUBSAMPLES];
                LPC_ORDER + MAX_OV_COLS],
            lpc_filter_states_imag_hbe: [[0.0; SBRDEC_MAX_SUBBAND_SUBSAMPLES];
                LPC_ORDER + MAX_OV_COLS],
        }
    }
}

impl LppTransposer {
    /// Creates a new instance of `LppTransposer`.
    ///
    /// # Return
    ///
    /// A new instance of `LppTransposer` with default values.
    pub fn new() -> Self {
        Self::default()
    }

    /// Calculates linear prediction complex filter coefficients for filter order `LPC_ORDER`.
    ///
    /// # Parameters
    ///
    /// - `low_bands`: Input buffer with LPC states and QMF overlap data.
    ///
    /// # Return
    ///
    ///  LPC filter coefficients `[Complex<f32>; LPC_ORDER]`.
    fn get_prediction_filter_coeffs(
        &self,
        low_bands: &[Complex<f32>],
    ) -> [Complex<f32>; LPC_ORDER] {
        // Perform autocorrelation.
        let auto_corr = AutocorrCoeffs::from_buffer(low_bands);

        let mut alpha = [Complex::<f32> { re: 0.0, im: 0.0 }; LPC_ORDER];
        // To avoid NaN and Inf situations.
        if (auto_corr.det() > f32::MIN_POSITIVE) && (auto_corr.det() < f32::MAX) {
            let mut tmp = auto_corr.r02().scale(auto_corr.r11r().neg());
            tmp += auto_corr.r01() * auto_corr.r12();
            alpha[1] = tmp.unscale(auto_corr.det());
        }

        if auto_corr.r11r() != 0.0f32 {
            // auto_corr.r11r() is always >=0
            let tmp = auto_corr.r01() + (alpha[1] * (auto_corr.r12().conj()));
            alpha[0] = tmp.unscale(auto_corr.r11r().neg());
        }

        // Prevent for shootovers.
        if alpha[0].norm_sqr() >= 16.0f32 || alpha[1].norm_sqr() >= 16.0f32 {
            let tmp = Complex::<f32> { re: 0.0, im: 0.0 };
            alpha[0] = tmp;
            alpha[1] = tmp;
        }
        alpha
    }

    /// The HF generation copies `low_band` samples to `QMF subbands` at subsample position
    /// `target_band`, with (or without) performing LPC filter using bandwidth expansion factor
    /// `bw` and prediction filter coefficients `alpha[]`.
    ///
    /// # Parameters
    ///
    /// - `low_band`: Low frequency band data.
    /// - `alpha`: The prediction filter coefficients are obtained from the covariance method.
    /// - `qmf_buffer_real`: Real part of QMF data.
    /// - `qmf_buffer_imag`: Imaginary part of QMF data.
    /// - `bw`: Bandwidth expansion factor.
    /// - `start_sample`: Subband starting in QMF buffer.
    /// - `stop_sample`: Subband ending in QMF buffer.
    /// - `target_band`: Index of subsample in subband.
    #[expect(clippy::too_many_arguments)]
    fn hf_generation(
        &self,
        low_band: &[Complex<f32>],
        alpha: &[Complex<f32>],
        qmf_buffer_real: &mut [&mut [f32]],
        qmf_buffer_imag: &mut [&mut [f32]],
        bw: f32,
        start_sample: usize,
        stop_sample: usize,
        target_band: usize,
    ) {
        if bw <= 0.0_f32 {
            // Copying low_band samples to QMF subbands
            for (real, imag, lb) in izip!(
                qmf_buffer_real[start_sample..].iter_mut(),
                qmf_buffer_imag[start_sample..].iter_mut(),
                low_band[LPC_ORDER + start_sample..].iter()
            )
            .take(stop_sample - start_sample)
            {
                real[target_band] = lb.re();
                imag[target_band] = lb.im();
            }
        } else {
            // Applying current bandwidth expansion factor and LPC filter.
            // ISO/IEC 14496-3:2009(E) 4.6.18.6.3 HF generator
            let a0 = alpha[0].scale(bw);
            let a1 = alpha[1].scale(bw * bw);

            for (real, imag, lb) in izip!(
                qmf_buffer_real[start_sample..].iter_mut(),
                qmf_buffer_imag[start_sample..].iter_mut(),
                low_band[LPC_ORDER + start_sample - 2..].windows(3)
            )
            .take(stop_sample - start_sample)
            {
                let mut accu = lb[2];
                accu += lb[1] * a0;
                accu += lb[0] * a1;

                real[target_band] = accu.re();
                imag[target_band] = accu.im();
            }
        }
    }

    /// Performs inverse filtering to emphasize the level by retrieving the bandwidth
    /// expansion factor and applying smoothing to each filter band.
    ///
    /// # Parameters
    ///
    /// - `white_factors`: `WhiteningFactors` instance with valid data.
    /// - `sbr_invf_mode`: Current inverse filtering modes.
    /// - `sbr_invf_mode_prev`: Previous inverse filtering modes.
    /// - `bw_vector`: Resulting filtering levels.
    /// - `num_invf_bands`: Number of bands for inverse filtering.
    fn inverse_filtering_level_emphasis(
        &mut self,
        white_factors: &WhiteningFactors,
        sbr_invf_mode: &[InvfMode],
        sbr_invf_mode_prev: &[InvfMode],
        bw_vector: &mut [f32],
        num_invf_bands: usize,
    ) {
        for (mode, prev_mode, bw_vec, bw_vec_old) in izip!(
            sbr_invf_mode.iter(),
            sbr_invf_mode_prev.iter(),
            bw_vector.iter_mut(),
            self.bw_vector_old.iter()
        )
        .take(num_invf_bands)
        {
            let bw_tmp = white_factors.map_invf_mode(*mode, *prev_mode);
            let (f1, f2) = if bw_tmp < *bw_vec_old {
                (0.75_f32, 0.25_f32)
            } else {
                (0.90625_f32, 0.09375_f32)
            };

            let accu = (f1 * bw_tmp) + (f2 * *bw_vec_old);
            *bw_vec = if accu < 0.015625_f32 {
                0.0_f32
            } else {
                accu.min(0.99609375_f32)
            };
        }
    }

    /// Performs transposition by patching of subband samples.
    /// The function determines the areas for the patching process (these are the source range as
    /// well as the target range) and implements spectral whitening by means of inverse filtering.
    /// The functions of `AutocorrCoeffs` is an auxiliary function for calculating the LPC
    /// coefficients for the filtering. The actual calculation of the LPC coefficients and the
    /// implementation of the filtering are done as part of this function. In case of leagcy SBR,
    /// `hf_gen_preflat` module is used for calculating pre-whitening gains if `is_pre_whitening`
    ///  is `true`.
    ///
    /// Note that the filtering is done on all available QMF subsamples, whereas the
    /// patching is only done on those QMF subsamples that will be used in the next
    /// QMF synthesis. The filtering is also implemented before the patching includes
    /// further dependencies on parameters from the SBR data.
    ///
    /// # Parameters
    ///
    /// - `trans_settings`: `TransposerSettings` instance with valid data.
    /// - `qmf_buff_real`: Real part of QMF data.
    /// - `qmf_buff_imag`: Imaginary part of QMF data.
    /// - `sbr_invf_mode`: Current inverse filtering modes.
    /// - `sbr_invf_mode_prev`: Previous inverse filtering modes.
    /// - `start`: Low band patch starting.
    /// - `stop`: Low band patch stopping.
    /// - `num_invf_bands`: Number of bands for inverse filtering.
    /// - `v_k_master0`: Number of subsamples in low frequency subbands.
    /// - `time_step`: Time step of envelope.
    /// - `first_slot_offs`: Start position in time.
    /// - `last_slot_offs`: Number of overlap-slots into next frame.
    /// - `is_hbe`: Number of bands for inverse filtering.
    /// - `is_pre_whitening`: Number of bands for inverse filtering.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply(
        &mut self,
        trans_settings: &mut TransposerSettings,
        qmf_buff_real: &mut [&mut [f32]],
        qmf_buff_imag: &mut [&mut [f32]],
        sbr_invf_mode: &[InvfMode],
        sbr_invf_mode_prev: &[InvfMode],
        start: usize,
        stop: usize,
        num_invf_bands: usize,
        v_k_master0: usize,
        time_step: usize,
        first_slot_offs: usize,
        last_slot_offs: usize,
        is_hbe: bool,
        is_pre_whitening: bool,
    ) {
        let mut bw_vector = [0.0_f32; MAX_NUM_PATCHES];
        let mut pre_whitening_gains = [0.0_f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES / 2];

        let start_sample = first_slot_offs * time_step;
        let stop_sample = usize::from(trans_settings.num_columns) + (last_slot_offs * time_step);

        let patch_param = &mut trans_settings.patch_param;

        assert!((last_slot_offs * time_step) <= usize::from(trans_settings.overlap_size));
        self.inverse_filtering_level_emphasis(
            &trans_settings.white_factors,
            sbr_invf_mode,
            sbr_invf_mode_prev,
            &mut bw_vector,
            num_invf_bands,
        );

        if trans_settings.num_of_patches > 0 {
            let target_stop_band = usize::from(
                patch_param[usize::from(trans_settings.num_of_patches - 1)].target_start_band
                    + patch_param[usize::from(trans_settings.num_of_patches - 1)]
                        .num_bands_in_patch,
            );

            izip!(
                &mut qmf_buff_real[start_sample..stop_sample],
                &mut qmf_buff_imag[start_sample..stop_sample]
            )
            .for_each(|(qmf_real, qmf_imag)| {
                qmf_real[target_stop_band..SBRDEC_MAX_SUBBAND_SUBSAMPLES].fill(0.0);
                qmf_imag[target_stop_band..SBRDEC_MAX_SUBBAND_SUBSAMPLES].fill(0.0);
            });
        }

        if is_pre_whitening && start_sample < stop_sample {
            let mut converted_real: [&[f32];
                (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize] =
                array::from_fn(|_| [].as_slice());
            let mut converted_imag: [&[f32];
                (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize] =
                array::from_fn(|_| [].as_slice());

            sbr_common::convert_mut_to_immut_slices(
                &qmf_buff_real[start_sample..stop_sample],
                &mut converted_real,
            );
            sbr_common::convert_mut_to_immut_slices(
                &qmf_buff_imag[start_sample..stop_sample],
                &mut converted_imag,
            );

            hf_gen_preflat::calc_pre_whitening_gains(
                &converted_real[..stop_sample - start_sample],
                &converted_imag[..stop_sample - start_sample],
                &mut pre_whitening_gains,
                v_k_master0,
            );
        }

        // Outer loop over bands to do analysis only once for each band.
        let auto_corr_length =
            usize::from(trans_settings.num_columns) + usize::from(trans_settings.overlap_size);
        let mut low_band_buff = [Complex::<f32> { re: 0.0, im: 0.0 }; MAX_ENV_COLS + LPC_ORDER];

        for lo_band in start..stop {
            if is_hbe {
                for (lb, real_hbe, imag_hbe) in izip!(
                    low_band_buff.iter_mut(),
                    self.lpc_filter_states_real_hbe.iter(),
                    self.lpc_filter_states_imag_hbe.iter()
                )
                .take(LPC_ORDER + start_sample)
                {
                    lb.re = real_hbe[lo_band];
                    lb.im = imag_hbe[lo_band];
                }
            } else {
                for (lb, leg_sbr) in izip!(
                    low_band_buff.iter_mut(),
                    self.lpc_filter_states_leg_sbr.iter(),
                )
                .take(LPC_ORDER + start_sample)
                {
                    *lb = leg_sbr[lo_band];
                }
            }

            // Take old slope length qmf slot source values out of (overlap)qmf buffer.
            for (lb, real_qmf, imag_qmf) in izip!(
                low_band_buff[start_sample + LPC_ORDER..].iter_mut(),
                qmf_buff_real[start_sample..].iter(),
                qmf_buff_imag[start_sample..].iter()
            )
            .take(auto_corr_length - start_sample)
            {
                lb.re = real_qmf[lo_band];
                lb.im = imag_qmf[lo_band];
            }

            // Calculate prediction filter coefficents.
            let alpha =
                self.get_prediction_filter_coeffs(&low_band_buff[..auto_corr_length + LPC_ORDER]);

            if is_hbe {
                // Store unmodified values to buffer.
                for (real_hbe, imag_hbe, real_qmf, imag_qmf) in izip!(
                    self.lpc_filter_states_real_hbe.iter_mut(),
                    self.lpc_filter_states_imag_hbe.iter_mut(),
                    qmf_buff_real[usize::from(trans_settings.num_columns) - LPC_ORDER..].iter(),
                    qmf_buff_imag[usize::from(trans_settings.num_columns) - LPC_ORDER..].iter(),
                )
                .take(LPC_ORDER + usize::from(trans_settings.overlap_size))
                {
                    real_hbe[lo_band] = real_qmf[lo_band];
                    imag_hbe[lo_band] = imag_qmf[lo_band];
                }

                let mut bw_index = 0;
                while bw_index < MAX_NUM_PATCHES - 1
                    && (lo_band >= usize::from(trans_settings.bw_borders[bw_index]))
                {
                    bw_index += 1;
                }

                let bw = bw_vector[bw_index];
                self.hf_generation(
                    &low_band_buff,
                    &alpha,
                    qmf_buff_real,
                    qmf_buff_imag,
                    bw,
                    start_sample,
                    stop_sample,
                    lo_band,
                );
            } else {
                // Legacy SBR patching.
                let mut patch = 0;
                let mut bw_index = [0_usize; MAX_NUM_PATCHES];

                // Inner loop over every patch.
                while patch < usize::from(trans_settings.num_of_patches) {
                    let hi_band = lo_band + usize::from(patch_param[patch].target_band_offs);
                    if lo_band < usize::from(patch_param[patch].source_start_band)
                        || lo_band >= usize::from(patch_param[patch].source_stop_band)
                    {
                        // Lowband not in current patch - proceed.
                        patch += 1;
                        continue;
                    }

                    debug_assert!(hi_band < SBRDEC_MAX_SUBBAND_SUBSAMPLES);

                    // bw_index[patch] is already initialized with value from previous
                    // band inside this patch.
                    while hi_band >= usize::from(trans_settings.bw_borders[bw_index[patch]])
                        && bw_index[patch] < MAX_NUM_PATCHES - 1
                    {
                        bw_index[patch] += 1;
                    }

                    let bw = bw_vector[bw_index[patch]];
                    self.hf_generation(
                        &low_band_buff,
                        &alpha,
                        qmf_buff_real,
                        qmf_buff_imag,
                        bw,
                        start_sample,
                        stop_sample,
                        hi_band,
                    );

                    if is_pre_whitening {
                        let gain = pre_whitening_gains[lo_band];
                        for (real_qmf, imag_qmf) in izip!(
                            qmf_buff_real[start_sample..].iter_mut(),
                            qmf_buff_imag[start_sample..].iter_mut(),
                        )
                        .take(stop_sample - start_sample)
                        {
                            real_qmf[hi_band] *= gain;
                            imag_qmf[hi_band] *= gain;
                        }
                    }

                    patch += 1;
                } // Inner loop over patches.
            }
        } // Outer loop over bands (lo_band).

        for (bw_vec_old, bw_vec) in
            izip!(self.bw_vector_old.iter_mut(), bw_vector.iter()).take(num_invf_bands)
        {
            *bw_vec_old = *bw_vec;
        }
    }

    /// Saves states in legacy lpc filter states buffer.
    ///
    /// # Parameters
    ///
    /// - `real`: Real slots of qmf spectrum.
    /// - `imag`: Imaginary slots of qmf spectrum.
    /// - `num_slots`: Number of qmf slots.
    /// - `num_bands`: Number of subbands in each slot.
    pub(super) fn save_leg_lpc_filter_states(
        &mut self,
        real: &[&[f32]],
        imag: &[&[f32]],
        num_slots: usize,
        num_bands: usize,
    ) {
        for (lpc_states_slot, real_slot, imag_slot) in izip!(
            self.lpc_filter_states_leg_sbr.iter_mut(),
            real.iter(),
            imag.iter()
        )
        .take(num_slots)
        {
            for (lpc_sta, re, im) in izip!(
                lpc_states_slot.iter_mut(),
                real_slot.iter(),
                imag_slot.iter()
            )
            .take(num_bands)
            {
                lpc_sta.re = *re;
                lpc_sta.im = *im;
            }
        }
    }

    /// Saves states in hbe lpc filter states buffer.
    ///
    /// # Parameters
    ///
    /// - `real`: Real slots of qmf spectrum.
    /// - `imag`: Imaginary slots of qmf spectrum.
    /// - `num_slots`: Number of qmf slots.
    /// - `num_bands`: Number of subbands in each slot.
    pub(super) fn save_hbe_lpc_filter_states(
        &mut self,
        real: &[&[f32]],
        imag: &[&[f32]],
        num_slots: usize,
        num_bands: usize,
    ) {
        for (hbe_real_slot, hbe_imag_slot, real_slot, imag_slot) in izip!(
            self.lpc_filter_states_real_hbe.iter_mut(),
            self.lpc_filter_states_imag_hbe.iter_mut(),
            real.iter(),
            imag.iter()
        )
        .take(num_slots)
        {
            hbe_real_slot[..num_bands].copy_from_slice(&real_slot[..num_bands]);
            hbe_imag_slot[..num_bands].copy_from_slice(&imag_slot[..num_bands]);
        }
    }

    /// Returns mutable reference to legacy lpc filter states.
    pub(super) fn lpc_filter_states_leg_sbr(
        &mut self,
    ) -> &mut [[Complex<f32>; SBRDEC_MAX_ANALYSIS_CHANNELS]; LPC_ORDER + MAX_OV_COLS] {
        &mut self.lpc_filter_states_leg_sbr
    }

    /// Returns mutable reference to HBE's lpc filter states.
    pub(super) fn lpc_filter_states_hbe_mut(
        &mut self,
    ) -> (
        &mut [[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
        &mut [[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
    ) {
        (
            &mut self.lpc_filter_states_real_hbe,
            &mut self.lpc_filter_states_imag_hbe,
        )
    }

    /// Returns immutable reference to HBE's lpc filter states.
    pub(super) fn lpc_filter_states_hbe(
        &self,
    ) -> (
        &[[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
        &[[f32; SBRDEC_MAX_SUBBAND_SUBSAMPLES]; LPC_ORDER + MAX_OV_COLS],
    ) {
        (
            &self.lpc_filter_states_real_hbe,
            &self.lpc_filter_states_imag_hbe,
        )
    }
}
