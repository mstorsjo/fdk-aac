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
//! AAC core channel concealment

use itertools::izip;

use super::conceal_constants::{
    AacDecoderRenderMode, ConcealmentExpandType, ConcealmentState, FadeDirection, TDfadingType,
};
use super::conceal_params::{ConcealmentMethod, ConcealmentParams};
use crate::aac_dec::{
    channel_info::{BlockType, IcsInfo},
    constants,
    lpd::{common::LpdMode, constants as lpd_constants, LpdData},
    sr_info::SamplingRateInfo,
};
use crate::common::flags::ACFlags;
use crate::common::{arith_ops::calc_energy, enums::WindowShape};

use crate::aac_dec::utils;

// Size of random number array for noise floor.
pub const NUM_NOISE_FLOOR_VALUES: usize = 512;

// Random sign bit used for concealment.
#[rustfmt::skip]
pub const AAC_DEC_RANDOM_SIGN: [u16; NUM_NOISE_FLOOR_VALUES / 16] = [
    // sign bits of rust_aac::sbr_dec::env_calc::RANDOM_PHASE[] entries:
    // MSB ... LSB   <- LSB ........... MSB

    0x3ce9,       // <- 1001 0111 0011 1100
    0xdee2,       // <- 0100 0111 0111 1011
    0xd738,       // <- 0001 1100 1110 1011
    0x96c8,       // <- 0001 0011 0110 1001
    0x0bca,       // <- 0101 0011 1101 0000
    0x2f88,       // <- 0001 0001 1111 0100
    0xb737,       // <- 1110 1100 1110 1101
    0x9d54,       // <- 0010 1010 1011 1001
    0x563e,       // <- 0111 1100 0110 1010
    0xa4eb,       // <- 1101 0111 0010 0101
    0x3da8,       // <- 0001 0101 1011 1100
    0xd9ea,       // <- 0101 0111 1001 1011
    0xaa2b,       // <- 1101 0100 0101 0101
    0xc291,       // <- 1000 1001 0100 0011
    0x35f3,       // <- 1100 1111 1010 1100
    0x4753,       // <- 1100 1010 1110 0010
    0x1586,       // <- 0110 0001 1010 1000
    0x3fac,       // <- 0011 0101 1111 1100
    0x8568,       // <- 0001 0110 1010 0001
    0x4eb4,       // <- 0010 1101 0111 0010
    0x925b,       // <- 1101 1010 0100 1001
    0x7093,       // <- 1100 1001 0000 1110
    0x5631,       // <- 1000 1100 0110 1010
    0xb610,       // <- 0000 1000 0110 1101
    0xdf81,       // <- 1000 0001 1111 1011
    0xe2cf,       // <- 1111 0011 0100 0111
    0x5481,       // <- 1000 0001 0010 1010
    0xf3ab,       // <- 1101 0101 1100 1111
    0x1686,       // <- 0110 0001 0110 1000
    0x63cc,       // <- 0011 0011 1100 0110
    0x6aec,       // <- 0011 0111 0101 0110
    0x458d,       // <- 1011 0001 1010 0010
];

/// Apply fade out operational modes.
#[derive(PartialEq, Copy, Clone, Debug)]
enum FadeOutOperation {
    CountFadeFrames = 0,
    RandomSignMuteSpectral,
}

/// Converts FadeOutOperation to usize.
impl From<FadeOutOperation> for usize {
    fn from(value: FadeOutOperation) -> Self {
        match value {
            FadeOutOperation::CountFadeFrames => 0,
            FadeOutOperation::RandomSignMuteSpectral => 1,
        }
    }
}

/// Concealment Info.
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub(super) struct ConcealmentInfo {
    /// Index used by `AAC_DEC_RANDOM_SIGN`.
    random_phase: i32,
    /// Used to hold status of previous 2 frames.
    pub(super) is_prev_frame_ok: [bool; 2],
    /// Counter for number of valid frames.
    cnt_valid_frames: i32,
    /// State for signal fade-in/out.
    cnt_fade_frames: i32,
    /// States for signal fade-out of frames with more than one window/subframe.
    /// [0] used by "update cnt_fade_frames mode" of apply_fade_out,
    /// [1] used by fade_out mode of concelement_state.
    win_grp_offset: [i32; 2],
    /// State for faster signal fade-out of frames with transient signal parts.
    att_grp_offset: [i32; 2],
    /// Last render mode type of AacDecoderRenderMode.
    last_render_mode: AacDecoderRenderMode,
    window_shape: WindowShape,
    window_sequence: BlockType,
    /// Last window group legth from last frame's `IcsInfo`.
    last_win_grp_len: u8,
    /// Current concealment state.
    pub(super) conceal_state: ConcealmentState,
    /// Old concealment state used by TD fading.
    conceal_state_old: ConcealmentState,
    /// Last / Previous fading factor.
    fade_old: f32,
    /// Last / Previous fading type.
    last_fading_type: TDfadingType,
    /// lsf4 coefficients.
    lsf4: [f32; lpd_constants::M_LP_FILTER_ORDER],
    /// Gain of the last frame from TCX module.
    last_tcx_gain: f32,
    /// Last window groups and their lengths from last frame's `IcsInfo`.
    last_window_groups: u8,
    last_window_group_length: [u8; constants::MAX_WINDOWS],
    /// States used by noise filtering.
    td_noise_states: [f32; 3],
    /// Flag to indicate concealment state released from muting.
    pub(super) is_mute_release: bool,
    /// Flag to indicate concealment activation.
    is_conceal_defined: bool,
}

/// Default trait.
impl Default for ConcealmentInfo {
    fn default() -> Self {
        Self {
            td_noise_states: [0.0_f32; 3],
            last_render_mode: AacDecoderRenderMode::Invalid,
            is_conceal_defined: false,
            window_shape: WindowShape::Sine,
            window_sequence: BlockType::Long,
            last_win_grp_len: 1,
            conceal_state: ConcealmentState::Ok,
            random_phase: 0,
            is_prev_frame_ok: [true; 2],
            cnt_fade_frames: 0,
            cnt_valid_frames: 0,
            fade_old: 1.0_f32,
            win_grp_offset: [0; 2],
            att_grp_offset: [0; 2],
            conceal_state_old: ConcealmentState::Ok,
            last_fading_type: TDfadingType::ToSpectralMute,
            lsf4: Default::default(),
            last_tcx_gain: Default::default(),
            last_window_groups: Default::default(),
            last_window_group_length: Default::default(),
            is_mute_release: Default::default(),
        }
    }
}

// Methods of Concealment Info struct.
impl ConcealmentInfo {
    /// Creates new ConcealmentInfo instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Initializes concealment information for a given channel with default values.
    pub(super) fn init_channel_data(&mut self) {
        *self = Self::default();
    }

    /// The function calculates a band-wise spectral energy.
    /// This is used for frame interpolation.
    ///
    /// # Parameters
    ///
    /// - `spectrum`: Spectral coefficients of frame
    /// - `sr_info`: Sampling rate info instance with valid data
    /// - `block_type`: Block type of spectrum
    /// - `expand_type`: Type of concealment expand
    /// - `sfb_energy`: Updated with calculated energy values of scalefactor bands.
    fn calc_band_energy(
        &self,
        spectrum: &[f32],
        sr_info: &SamplingRateInfo,
        block_type: BlockType,
        expand_type: ConcealmentExpandType,
        sfb_energy: &mut [f32],
    ) {
        // In the following calculations, en_accu is initialized with LSB-value in
        // order to avoid zero energy-level.

        let mut line = 0;
        match block_type {
            BlockType::Long | BlockType::Start | BlockType::Stop => {
                if expand_type == ConcealmentExpandType::NoExpand {
                    // Standard long calculation.
                    let sfb_total = sr_info.n_scale_factor_bands_long();
                    let sfb_offset = sr_info.scale_factor_bands_long().unwrap();

                    for (offset_val, energy_val) in sfb_offset
                        .iter()
                        .skip(1)
                        .zip(sfb_energy.iter_mut())
                        .take(sfb_total)
                    {
                        *energy_val = 5e-10_f32;
                        if line < *offset_val as usize {
                            *energy_val += calc_energy(&spectrum[line..*offset_val as usize]);
                            line = *offset_val as usize;
                        }
                    }
                } else {
                    // Compress long to short.
                    let sfb_total = sr_info.n_scale_factor_bands_short();
                    let sfb_offset = sr_info.scale_factor_bands_short().unwrap();

                    for (offset_val, energy_val) in sfb_offset
                        .iter()
                        .skip(1)
                        .zip(sfb_energy.iter_mut())
                        .take(sfb_total)
                    {
                        let en_accu = 5e-10_f32;
                        if line < (*offset_val << 3) as usize {
                            *energy_val = spectrum[line..(*offset_val << 3) as usize]
                                .iter()
                                .fold(en_accu, |accu, spec| {
                                    accu + ((accu + *spec * *spec) / 8.0_f32)
                                });
                            line = (*offset_val << 3) as usize;
                        }
                    }
                }
            }
            BlockType::Short => {
                if expand_type == ConcealmentExpandType::NoExpand {
                    // Standard short calculation.
                    let sfb_total = sr_info.n_scale_factor_bands_short();
                    let sfb_offset = sr_info.scale_factor_bands_short().unwrap();

                    for (offset_val, energy_val) in sfb_offset
                        .iter()
                        .skip(1)
                        .zip(sfb_energy.iter_mut())
                        .take(sfb_total)
                    {
                        *energy_val = 5e-10_f32;
                        if line < *offset_val as usize {
                            *energy_val += calc_energy(&spectrum[line..*offset_val as usize]);
                            line = *offset_val as usize;
                        }
                    }
                } else {
                    // Expand short to long spectrum.
                    let sfb_total = sr_info.n_scale_factor_bands_long();
                    let sfb_offset = sr_info.scale_factor_bands_long().unwrap();

                    for (offset_val, energy_val) in sfb_offset
                        .iter()
                        .skip(1)
                        .zip(sfb_energy.iter_mut())
                        .take(sfb_total)
                    {
                        let en_accu = 5e-10_f32;
                        if line < *offset_val as usize {
                            *energy_val = spectrum[line >> 3..(*offset_val - 1) as usize >> 3]
                                .iter()
                                .fold(en_accu, |accu, spec| accu + (*spec * *spec * 8.0));
                            line = *offset_val as usize;
                        }
                    }
                }
            }
        }
    }

    /// The function creates the interpolated spectral data according to the
    /// energy of the last good frame and the current (good) frame.
    ///
    /// # Parameters
    ///
    /// - `spectrum`: Spectral coefficients of frame
    /// - `energy_prv`: Energy of last good frame
    /// - `energy_act`: Energy of current good frame
    /// - `sfb_cnt`: Total number of scalefactor bands
    /// - `sfb_offset`: Scalefactor band offsets
    fn interpolate_buffer(
        &self,
        spectrum: &mut [f32],
        energy_prv: &[f32],
        energy_act: &[f32],
        sfb_cnt: usize,
        sfb_offset: &[u16],
    ) {
        for (prv, act, offset) in izip!(
            energy_prv.iter(),
            energy_act.iter(),
            sfb_offset.windows(2).take(sfb_cnt)
        ) {
            let fac_gain = (act.sqrt() / prv.sqrt()).sqrt();
            for spec in spectrum[offset[0] as usize..offset[1] as usize].iter_mut() {
                *spec *= fac_gain;
            }
        }
    }

    /// The function changes sign of spectral coefficents based on random sign
    /// that is derived from AAC_DEC_RANDOM_SIGN[] table. This table value are
    /// pre-defined for concealment processing.
    ///
    /// # Parameters
    ///
    /// - `spectrum`: Spectral coefficients of frame
    fn apply_random_sign(&self, spectrum: &mut [f32]) {
        // Random table 512x16bit has been reduced to 512 packed sign bits = 32x16 bit.

        let mut random_phase = self.random_phase;
        // Read current packed sign word.
        let mut packed_sign = AAC_DEC_RANDOM_SIGN[(random_phase >> 4) as usize];
        packed_sign >>= random_phase & 0xf;

        for spec in spectrum.iter_mut() {
            if random_phase & 0xf == 0 {
                packed_sign = AAC_DEC_RANDOM_SIGN[(random_phase >> 4) as usize];
            }

            if packed_sign & 0x1 != 0 {
                *spec = -*spec;
            }

            packed_sign >>= 1;
            random_phase = (random_phase + 1) & (NUM_NOISE_FLOOR_VALUES as i32 - 1);
        }
    }

    /// Returns type of window sequence based on previous window sequence.
    ///
    /// # Parameters
    ///
    /// - `prev_win_seq`: Previous window sequence
    fn get_window_sequence(&self, prev_win_seq: BlockType) -> BlockType {
        // Try to have only long blocks.
        if prev_win_seq == BlockType::Start || prev_win_seq == BlockType::Short {
            BlockType::Stop
        } else {
            BlockType::Long
        }
    }

    /// Apply concealment interpolation.
    ///
    /// The function swaps the data from the current and the previous frame. If an
    /// error has occured, frame interpolation is performed to restore the missing
    /// frame. In case of multiple faulty frames, fade-in and fade-out is applied.
    ///
    /// # Parameters
    ///
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `sr_info`: Sampling rate info instance with valid data
    /// - `is_frame_ok`: Flag indicates whether current frame is Ok (or) defective
    fn apply_interpolation(
        &mut self,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &[f32],
        ics_info: &mut IcsInfo,
        sr_info: &SamplingRateInfo,
        is_frame_ok: bool,
    ) {
        // Create buffer with zero filled/initialized memory.
        let mut sfb_energy_prev = [0.0_f32; constants::MAX_SFB_LONG];
        let mut sfb_energy_act = [0.0_f32; constants::MAX_SFB_LONG];

        let samples_per_frame = spectral_coefficient.len();

        if !is_frame_ok || self.is_mute_release {
            // Restore spectral data.
            let mut num_windows = 1_usize;
            let mut window_len = samples_per_frame;
            let mut src_grp_stop = num_windows;
            let mut src_grp_start = 0_usize;

            let last_window_group_length = &self.last_window_group_length[..];
            let last_window_groups = usize::from(self.last_window_groups);
            let last_grp_len = last_window_group_length[last_window_groups - 1] as usize;

            // Restore last frame from concealment buffer.
            ics_info.set_window_shape(self.window_shape);
            ics_info.set_window_sequence(self.window_sequence);

            // Set old window parameters.
            if self.window_sequence == BlockType::Short {
                // short block handling
                num_windows = 8_usize;
                src_grp_stop = num_windows;
                window_len = samples_per_frame >> 3;
                // Intelligent `src_grp_start`/`src_grp_stop`:
                // Repeat window groups with no transients if possible.
                // If previous frame was ok, repeat whole frame, because this is a
                // completely good one (b/c interpolation concealment has 1 frame delay).

                if !self.is_prev_frame_ok[1] {
                    src_grp_start = num_windows - last_grp_len;
                    if last_grp_len < 3
                        || (last_window_groups > 1
                            && last_window_group_length[last_window_groups - 2] == 1)
                    {
                        let mut loop_iter = last_window_groups;
                        'window_groups: while loop_iter > 0 {
                            // Criterion 1: last window group length is greater than 2 and
                            // previous group was greater than 1.
                            // Criterion 2: criterion 1 doesnt match for the whole window but at
                            // least the first group was greater than 1.
                            loop_iter -= 1;
                            let crit1 = last_window_group_length[loop_iter] > 2
                                && loop_iter > 0
                                && last_window_group_length[loop_iter - 1] != 1;
                            let crit2 = loop_iter == 0 && last_window_group_length[0] > 1;

                            if crit1 || crit2 {
                                let mut tmp = 0;
                                src_grp_stop = last_window_group_length[loop_iter] as usize;
                                for len in
                                    last_window_group_length[loop_iter..last_window_groups].iter()
                                {
                                    tmp += *len;
                                }

                                src_grp_start = num_windows - usize::from(tmp);
                                src_grp_stop += src_grp_start;
                                break 'window_groups;
                            }
                        }
                    }
                }
            }

            let mut src_win = src_grp_start + self.win_grp_offset[1] as usize;

            assert!((src_grp_start + 1) * window_len <= samples_per_frame);
            assert!((src_win + 1) * window_len <= samples_per_frame);

            for spec in spectral_coefficient
                .chunks_exact_mut(window_len)
                .take(num_windows)
            {
                // Restore frequency coefficients from buffer.
                spec.copy_from_slice(
                    &spectral_coefficient_prev[src_win * window_len..(src_win + 1) * window_len],
                );
                src_win += 1;
                if src_win >= src_grp_stop {
                    // End of sequence -> rewind to first window of group.
                    src_win = src_grp_start;
                }
            }

            self.win_grp_offset[1] = (src_win - src_grp_start).try_into().unwrap();
            assert!(
                (self.win_grp_offset[1] >= 0)
                    && (self.win_grp_offset[1] < constants::MAX_WINDOWS as i32)
            );
        }

        // If previous frame was not ok.
        if !self.is_prev_frame_ok[1] || self.is_mute_release {
            // If current frame (f_n) is ok and the last but one frame (f_(n-2))
            // was ok, too, then interpolate both frames in order to generate
            // the current output frame (f_(n-1)). Otherwise, use the last stored
            // frame (f_(n-2) or f_(n-3) or ...).

            if is_frame_ok && self.is_prev_frame_ok[0] && !self.is_mute_release {
                // Interpolate both frames in order to generate the current output frame
                // (f_(n-1)).

                if !ics_info.is_long_block() {
                    // f_(n-2) == BLOCK_SHORT
                    // short--??????--short, short--??????--long interpolation
                    // short--short---short, short---long---long interpolation

                    if self.window_sequence == BlockType::Short {
                        // f_n == BLOCK_SHORT
                        // short--short---short interpolation
                        let sfb_total = sr_info.n_scale_factor_bands_short();
                        let sfb_offset = sr_info.scale_factor_bands_short().unwrap();
                        ics_info.set_window_shape(if samples_per_frame <= 512 {
                            WindowShape::LowOverlap
                        } else {
                            WindowShape::KBD
                        });
                        ics_info.set_window_sequence(BlockType::Short);

                        let win_size = samples_per_frame / constants::MAX_WINDOWS;
                        for (spec, spec_prev) in izip!(
                            spectral_coefficient.chunks_exact_mut(win_size),
                            spectral_coefficient_prev.chunks_exact(win_size)
                        )
                        .take(constants::MAX_WINDOWS)
                        {
                            self.calc_band_energy(
                                spec, // spec_(n-2)
                                sr_info,
                                BlockType::Short,
                                ConcealmentExpandType::NoExpand,
                                &mut sfb_energy_prev,
                            );

                            self.calc_band_energy(
                                spec_prev, // spec_n
                                sr_info,
                                BlockType::Short,
                                ConcealmentExpandType::NoExpand,
                                &mut sfb_energy_act,
                            );

                            self.interpolate_buffer(
                                spec,
                                &sfb_energy_prev,
                                &sfb_energy_act,
                                sfb_total,
                                sfb_offset,
                            );
                        }
                    } else {
                        // f_n != BLOCK_SHORT
                        // short---long---long interpolation
                        let sfb_total = sr_info.n_scale_factor_bands_long();
                        let sfb_offset = sr_info.scale_factor_bands_long().unwrap();

                        let win_size = samples_per_frame / constants::MAX_WINDOWS;

                        self.calc_band_energy(
                            &spectral_coefficient[samples_per_frame - win_size..], // spec_(n-2)
                            sr_info,
                            BlockType::Short,
                            ConcealmentExpandType::Expand,
                            &mut sfb_energy_act,
                        );

                        self.calc_band_energy(
                            spectral_coefficient_prev, // spec_n
                            sr_info,
                            BlockType::Long,
                            ConcealmentExpandType::NoExpand,
                            &mut sfb_energy_prev,
                        );

                        ics_info.set_window_shape(WindowShape::Sine);
                        ics_info.set_window_sequence(BlockType::Stop);

                        // spec_n
                        spectral_coefficient
                            .copy_from_slice(&spectral_coefficient_prev[..samples_per_frame]);

                        self.interpolate_buffer(
                            spectral_coefficient, // spec_(n-1)
                            &sfb_energy_prev,
                            &sfb_energy_act,
                            sfb_total,
                            sfb_offset,
                        );
                    } // window_sequence
                } else {
                    // long--??????--short, long--??????--long interpolation
                    // long---long---short, long---long---long interpolation
                    let sfb_total = sr_info.n_scale_factor_bands_long();
                    let sfb_offset = sr_info.scale_factor_bands_long().unwrap();

                    self.calc_band_energy(
                        spectral_coefficient, // spec_(n-2)
                        sr_info,
                        BlockType::Long,
                        ConcealmentExpandType::NoExpand,
                        &mut sfb_energy_prev,
                    );

                    if self.window_sequence == BlockType::Short {
                        // f_n == BLOCK_SHORT
                        // long---long---short interpolation

                        ics_info.set_window_shape(if samples_per_frame <= 512 {
                            WindowShape::LowOverlap
                        } else {
                            WindowShape::KBD
                        });
                        ics_info.set_window_sequence(BlockType::Start);

                        // Expand first short spectrum
                        self.calc_band_energy(
                            spectral_coefficient_prev, // spec_n
                            sr_info,
                            BlockType::Short,
                            ConcealmentExpandType::Expand,
                            &mut sfb_energy_act,
                        );
                    } else {
                        // long---long---long interpolation

                        ics_info.set_window_shape(WindowShape::Sine);
                        ics_info.set_window_sequence(BlockType::Long);

                        self.calc_band_energy(
                            spectral_coefficient_prev, // spec_n
                            sr_info,
                            BlockType::Long,
                            ConcealmentExpandType::NoExpand,
                            &mut sfb_energy_act,
                        );
                    }

                    self.interpolate_buffer(
                        spectral_coefficient, // spec_(n-1)
                        &sfb_energy_prev,
                        &sfb_energy_act,
                        sfb_total,
                        sfb_offset,
                    );
                }
            } // if scope -> is_frame_ok && self.is_prev_frame_ok[0] && !self.is_mute_release

            // Noise substitution of sign of the output spectral coefficients.
            self.apply_random_sign(spectral_coefficient);

            // Increment random phase index to avoid repetition artifacts.
            self.random_phase = (self.random_phase + 1) & (NUM_NOISE_FLOOR_VALUES as i32 - 1);
        }

        // Scale spectrum according to concealment state.
        match self.conceal_state {
            ConcealmentState::Ok => {}
            ConcealmentState::Single => {}
            ConcealmentState::FadeIn => {}
            ConcealmentState::FadeOut => {}
            ConcealmentState::Mute => {
                // Set dummy window parameters.
                // Prevent an invalid `window_shape` (required for F/T transform).
                ics_info.set_window_shape(self.window_shape);
                ics_info.set_window_sequence(self.get_window_sequence(self.window_sequence));
                self.win_grp_offset[1] = 0;

                // mute spectral data
                spectral_coefficient.fill(0.0f32);
            }
        }
    }

    /// Based on input parameter 'mode', this function either updates cnt_fade_frames
    /// (where mode == FadeOutOperation::CountFadeFrames) or apply random sign and mute spectral
    /// coefficients (where mode == FadeOutOperation::RandomSignMuteSpectral) if necessary.
    ///
    /// # Parameters
    ///
    /// - `mode`: Mode to indicate operation to be performed inside this function
    /// - `num_fade_out_frames`: Number of fade out frames
    /// - `last_lpd_mode`: Last lpd mode from LpdData (or) 0 in case of LpdData is not available
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    fn apply_fade_out(
        &mut self,
        mode: FadeOutOperation,
        num_fade_out_frames: usize,
        last_lpd_mode: LpdMode,
        ics_info: &mut IcsInfo,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &mut [f32],
    ) {
        // FadeOutOperation::RandomSignMuteSpectral = apply random_sign and mute spectral
        // coefficients if necessary,
        // FadeOutOperation::CountFadeFrames = Update cnt_fade_frames

        // restore frequency coefficients from buffer with a specific muting
        let samples_per_frame = spectral_coefficient.len();
        let mut num_windows = 1_usize;
        let mut window_len = samples_per_frame;
        let mut src_grp_start = 0_usize;

        // Set old window parameters.
        if self.last_render_mode == AacDecoderRenderMode::Lpd {
            (num_windows, src_grp_start, window_len) = match last_lpd_mode {
                LpdMode::Tcx20 => (4, 3, samples_per_frame >> 2),
                LpdMode::Tcx40 => (2, 1, samples_per_frame >> 1),
                LpdMode::Tcx80 => (1, 0, samples_per_frame),
                _ => (1, 0, samples_per_frame), // Equal to default init values.
            };
            self.last_win_grp_len = 1;
        } else {
            ics_info.set_window_shape(self.window_shape);
            ics_info.set_window_sequence(self.window_sequence);

            if self.window_sequence == BlockType::Short {
                // Short block handling.
                num_windows = 8;
                window_len = samples_per_frame >> 3;
                src_grp_start = num_windows - usize::from(self.last_win_grp_len);
            }
        }

        let att_idx_stride = (num_windows as i32 / (self.last_win_grp_len + 1) as i32).max(1);

        // Load last state.
        let mode2idx = usize::from(mode);
        let mut att_idx = self.cnt_fade_frames as usize;
        let mut num_win_grp_per_fac = self.att_grp_offset[mode2idx];
        let mut src_win = src_grp_start + usize::try_from(self.win_grp_offset[mode2idx]).unwrap();

        assert!((src_grp_start + 1) * window_len <= samples_per_frame);
        assert!((src_win + 1) * window_len <= samples_per_frame);

        let (spec_prev_left, spec_prev_right) =
            spectral_coefficient_prev.split_at_mut(src_win * window_len);

        // Note:- This iterator acts like cycle() for spectral_coefficient_prev data.
        for (spec, spec_prev) in izip!(
            spectral_coefficient
                .chunks_exact_mut(window_len)
                .take(num_windows),
            spec_prev_right
                .chunks_exact_mut(window_len)
                .chain(spec_prev_left.chunks_exact_mut(window_len))
        ) {
            if mode == FadeOutOperation::RandomSignMuteSpectral {
                // Mute if att_idx gets large enough.
                if att_idx > num_fade_out_frames {
                    spec_prev.fill(0.0f32);
                }

                // Restore frequency coefficients from buffer - attenuation is done later.
                spec.copy_from_slice(spec_prev);

                // Apply random change of sign for spectral coefficients.
                self.apply_random_sign(spec);

                // Increment random phase index to avoid repetition artifacts.
                self.random_phase = (self.random_phase + 1) & (NUM_NOISE_FLOOR_VALUES as i32 - 1);
            }

            src_win += 1;

            if src_win >= num_windows {
                // End of sequence -> rewind to first window of group.
                src_win = src_grp_start;
                num_win_grp_per_fac += 1;
                if num_win_grp_per_fac >= att_idx_stride {
                    num_win_grp_per_fac = 0;
                    att_idx += 1;
                }
            }
        }

        // Store current state.
        self.win_grp_offset[mode2idx] = (src_win - src_grp_start) as i32;
        assert!(
            (self.win_grp_offset[mode2idx] >= 0)
                && (self.win_grp_offset[mode2idx] < constants::MAX_WINDOWS as i32)
        );
        self.att_grp_offset[mode2idx] = num_win_grp_per_fac;
        assert!(
            (self.att_grp_offset[mode2idx] >= 0)
                && (self.att_grp_offset[mode2idx] < att_idx_stride)
        );

        if mode == FadeOutOperation::CountFadeFrames {
            self.cnt_fade_frames = att_idx as i32;
        }
    }

    /// Apply concealment noise substitution. In case of frame loss this
    /// function produces a noisy frame based on the energy
    /// values of the previous frame.
    ///
    /// # Parameters
    ///
    /// - `num_fade_out_frames`: Number of fade out frames
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    /// - `last_lpd_mode`: Last lpd mode from LpdData (or) 0 in case of LpdData is not available
    fn apply_noise(
        &mut self,
        num_fade_out_frames: usize,
        ics_info: &mut IcsInfo,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &mut [f32],
        last_lpd_mode: LpdMode,
    ) {
        let samples_per_frame = spectral_coefficient.len();
        assert!((120..=constants::MAX_FRAMESIZE).contains(&samples_per_frame));

        match self.conceal_state {
            ConcealmentState::Ok => {} // Nothing to do here!
            // Produce noisy frame based on previous spectral coefficients.
            ConcealmentState::Single | ConcealmentState::FadeOut => self.apply_fade_out(
                FadeOutOperation::RandomSignMuteSpectral,
                num_fade_out_frames,
                last_lpd_mode,
                ics_info,
                spectral_coefficient,
                spectral_coefficient_prev,
            ),
            ConcealmentState::Mute => {
                // Set dummy window parameters.
                // Prevent an invalid `window_shape`(required for F/T transform).
                ics_info.set_window_shape(self.window_shape);
                ics_info.set_window_sequence(self.get_window_sequence(self.window_sequence));

                // Store for next frame (spectrum in concealment buffer can't be used at all).
                self.window_sequence = ics_info.window_sequence();
                self.win_grp_offset[0] = 0;
                self.win_grp_offset[1] = 0;

                // Mute spectral data - clear buffer.
                spectral_coefficient.fill(0.0f32);
                spectral_coefficient_prev.fill(0.0f32);
            }
            ConcealmentState::FadeIn => {}
        }
    }

    /// Store data for concealment techniques applied later.
    /// Interface function to store data for different concealment strategies.
    ///
    /// # Parameters
    ///
    /// - `conceal_method`: Type of concealment methods/techniques
    /// - `render_mode`: Type of AAC decoder's render mode (AacDecoderRenderMode)
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    /// - `lpc4_lsf`: lpc4_lsf coefficients values from LpdData if available
    /// - `tcx_gain`: last(previous) frame gain from TCX
    /// - `lpd_frame_mode`: Mode of specific (constants::NB_DIV - 1) Lpd frame
    #[expect(clippy::too_many_arguments)]
    fn store(
        &mut self,
        conceal_method: ConcealmentMethod,
        render_mode: AacDecoderRenderMode,
        ics_info: &mut IcsInfo,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &mut [f32],
        lpc4_lsf: &[f32],
        tcx_gain: f32,
        lpd_frame_mode: LpdMode,
    ) {
        if !(render_mode == AacDecoderRenderMode::Lpd) || !lpd_frame_mode.is_acelp() {
            if conceal_method < ConcealmentMethod::Inter {
                // Store new spectral bins.
                spectral_coefficient_prev.copy_from_slice(spectral_coefficient);
            } else {
                // Swap spectral data.
                spectral_coefficient.swap_with_slice(spectral_coefficient_prev);
            }
        }

        if render_mode != AacDecoderRenderMode::Lpd {
            // Store old window infos for swapping.
            let window_sequence = self.window_sequence;
            let window_shape = self.window_shape;

            // Store new window infos.
            self.window_sequence = ics_info.window_sequence();
            self.window_shape = ics_info.window_shape();
            self.last_window_groups = ics_info.n_window_groups() as u8;

            self.last_window_group_length
                .copy_from_slice(ics_info.window_group_lengths());
            self.last_win_grp_len = ics_info.window_group_length(ics_info.n_window_groups() - 1);

            if conceal_method >= ConcealmentMethod::Inter {
                // Complete swapping of window infos.
                ics_info.set_window_sequence(window_sequence);
                ics_info.set_window_shape(window_shape);
            }
        } else {
            // Set window info.
            self.window_shape = WindowShape::Sine;
            self.window_sequence = BlockType::Long; // default type

            // Store LSF4.
            self.lsf4.copy_from_slice(lpc4_lsf);
            // Store TCX gain.
            self.last_tcx_gain = tcx_gain;
        }
    }

    /// The function updates the state of the concealment state-machine.
    /// The states are: mute, fade-in, fade-out, interpolate, frame-ok
    /// and single-frame loss.
    ///
    /// # Parameters
    ///
    /// - `conceal_params`: Concealment params instance with valid data
    /// - `last_lpd_mode`: Last lpd mode from LpdData (or) 0 in case of LpdData is not available
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    /// - `is_frame_ok`: Flag indicates whether current frame is Ok (or) defective
    fn update_state(
        &mut self,
        conceal_params: &ConcealmentParams,
        last_lpd_mode: LpdMode,
        ics_info: &mut IcsInfo,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &mut [f32],
        is_frame_ok: bool,
    ) {
        match conceal_params.method {
            ConcealmentMethod::Noise => {
                if self.conceal_state != ConcealmentState::Ok {
                    // Count the valid frames during concealment process.
                    if is_frame_ok {
                        self.cnt_valid_frames += 1;
                    } else {
                        self.cnt_valid_frames = 0;
                    }
                }

                // -- STATE MACHINE for Noise Substitution --
                match self.conceal_state {
                    ConcealmentState::Ok => {
                        if !is_frame_ok {
                            self.cnt_fade_frames = 0;
                            self.cnt_valid_frames = 0;
                            self.att_grp_offset[0] = 0;
                            self.att_grp_offset[1] = 0;
                            self.win_grp_offset[0] = 0;
                            self.win_grp_offset[1] = 0;
                            if conceal_params.num_fade_out_frames > 0 {
                                // Change to state SINGLE-FRAME-LOSS.
                                self.conceal_state = ConcealmentState::Single;

                                // Mode 0 just updates the Fading counter.
                                self.apply_fade_out(
                                    FadeOutOperation::CountFadeFrames,
                                    conceal_params.num_fade_out_frames.try_into().unwrap(),
                                    last_lpd_mode,
                                    ics_info,
                                    spectral_coefficient,
                                    spectral_coefficient_prev,
                                );
                            } else {
                                // Change to state MUTE.
                                self.conceal_state = ConcealmentState::Mute;
                            }
                        }
                    }
                    ConcealmentState::Single => {
                        // Just a pre-stage before fade-out begins.
                        // Stay here only one frame!
                        if is_frame_ok {
                            // Change to state OK.
                            self.conceal_state = ConcealmentState::Ok;
                        } else if self.cnt_fade_frames >= conceal_params.num_fade_out_frames {
                            // Change to state MUTE.
                            self.conceal_state = ConcealmentState::Mute;
                        } else {
                            // Change to state FADE-OUT.
                            self.conceal_state = ConcealmentState::FadeOut;

                            // Mode 0 just updates the Fading counter.
                            self.apply_fade_out(
                                FadeOutOperation::CountFadeFrames,
                                conceal_params.num_fade_out_frames.try_into().unwrap(),
                                last_lpd_mode,
                                ics_info,
                                spectral_coefficient,
                                spectral_coefficient_prev,
                            );
                        }
                    }
                    ConcealmentState::FadeOut => {
                        if self.cnt_valid_frames > conceal_params.num_mute_release_frames {
                            if conceal_params.num_fade_in_frames > 0 {
                                // Change to state FADE-IN.
                                self.conceal_state = ConcealmentState::FadeIn;

                                // FadeOut -> FadeIn.
                                self.cnt_fade_frames = conceal_params.find_equi_fade_frame(
                                    self.cnt_fade_frames,
                                    FadeDirection::OutToIn,
                                );
                            } else {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        } else {
                            if is_frame_ok {
                                // We have good frame information but stay fully in concealment -
                                // reset win_grp_offset/att_grp_offset.
                                self.win_grp_offset[0] = 0;
                                self.win_grp_offset[1] = 0;
                                self.att_grp_offset[0] = 0;
                                self.att_grp_offset[1] = 0;
                            }
                            if self.cnt_fade_frames >= conceal_params.num_fade_out_frames {
                                // Change to state MUTE.
                                self.conceal_state = ConcealmentState::Mute;
                            } else {
                                // Stay in FADE-OUT.
                                // Mode 0 just updates the Fading counter.
                                self.apply_fade_out(
                                    FadeOutOperation::CountFadeFrames,
                                    conceal_params.num_fade_out_frames.try_into().unwrap(),
                                    last_lpd_mode,
                                    ics_info,
                                    spectral_coefficient,
                                    spectral_coefficient_prev,
                                );
                            }
                        }
                    }
                    ConcealmentState::Mute => {
                        if self.cnt_valid_frames > conceal_params.num_mute_release_frames {
                            if conceal_params.num_fade_in_frames > 0 {
                                // Change to state FADE-IN.
                                self.conceal_state = ConcealmentState::FadeIn;
                                self.cnt_fade_frames = conceal_params.num_fade_in_frames - 1;
                            } else {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        } else if is_frame_ok {
                            // We have good frame information but stay fully in concealment -
                            // reset win_grp_offset/att_grp_offset.
                            self.win_grp_offset[0] = 0;
                            self.win_grp_offset[1] = 0;
                            self.att_grp_offset[0] = 0;
                            self.att_grp_offset[1] = 0;
                        }
                    }
                    ConcealmentState::FadeIn => {
                        self.cnt_fade_frames -= 1;
                        if is_frame_ok {
                            if self.cnt_fade_frames < 0 {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        } else if conceal_params.num_fade_out_frames > 0 {
                            // Change to state FADE-OUT.
                            self.conceal_state = ConcealmentState::FadeOut;

                            // FadeIn -> FadeOut.
                            self.cnt_fade_frames = conceal_params.find_equi_fade_frame(
                                self.cnt_fade_frames + 1,
                                FadeDirection::InToOut,
                            );
                            self.win_grp_offset[0] = 0;
                            self.win_grp_offset[1] = 0;
                            self.att_grp_offset[0] = 0;
                            self.att_grp_offset[1] = 0;
                            // Decrease because apply_fade_out() will increase, accordingly.
                            self.cnt_fade_frames -= 1;

                            // Mode 0 just updates the Fading counter.
                            self.apply_fade_out(
                                FadeOutOperation::CountFadeFrames,
                                conceal_params.num_fade_out_frames.try_into().unwrap(),
                                last_lpd_mode,
                                ics_info,
                                spectral_coefficient,
                                spectral_coefficient_prev,
                            );
                        } else {
                            // Change to state MUTE.
                            self.conceal_state = ConcealmentState::Mute;
                        }
                    }
                }
            }
            ConcealmentMethod::Inter => {
                if self.conceal_state != ConcealmentState::Ok {
                    // Count the valid frames during concealment process.
                    if self.is_prev_frame_ok[1] || self.is_prev_frame_ok[0] && is_frame_ok {
                        // The frame is OK even if it can be estimated by the energy
                        // interpolation algorithm.
                        self.cnt_valid_frames += 1;
                    } else {
                        self.cnt_valid_frames = 0;
                    }
                }

                // -- STATE MACHINE for energy interpolation --
                match self.conceal_state {
                    ConcealmentState::Ok => {
                        if !(self.is_prev_frame_ok[1] || self.is_prev_frame_ok[0] && is_frame_ok) {
                            if conceal_params.num_fade_out_frames > 0 {
                                // Fade out only if the energy interpolation algorithm can not be
                                // applied!.
                                self.conceal_state = ConcealmentState::FadeOut;
                                // self.win_grp_offset[0] is not used in interpolation concealment.
                                self.win_grp_offset[1] = 0;
                            } else {
                                // Change to state MUTE.
                                self.conceal_state = ConcealmentState::Mute;
                                // self.win_grp_offset[0] is not used in interpolation concealment.
                                self.win_grp_offset[1] = 0;
                            }
                            self.cnt_fade_frames = 0;
                            self.cnt_valid_frames = 0;
                        }
                    }
                    ConcealmentState::Single => {
                        self.conceal_state = ConcealmentState::Ok;
                    }
                    ConcealmentState::FadeOut => {
                        self.cnt_fade_frames += 1;

                        if self.cnt_valid_frames > conceal_params.num_mute_release_frames {
                            self.win_grp_offset[1] = 0;
                            if conceal_params.num_fade_in_frames > 0 {
                                // Change to state FADE-IN.
                                self.conceal_state = ConcealmentState::FadeIn;
                                // FadeOut -> FadeIn.
                                self.cnt_fade_frames = conceal_params.find_equi_fade_frame(
                                    self.cnt_fade_frames - 1,
                                    FadeDirection::OutToIn,
                                );
                            } else {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        } else if self.cnt_fade_frames >= conceal_params.num_fade_out_frames {
                            // Change to state MUTE.
                            self.conceal_state = ConcealmentState::Mute;
                        }
                    }
                    ConcealmentState::Mute => {
                        if self.cnt_valid_frames > conceal_params.num_mute_release_frames {
                            self.win_grp_offset[1] = 0;
                            if conceal_params.num_fade_in_frames > 0 {
                                // Change to state FADE-IN.
                                self.conceal_state = ConcealmentState::FadeIn;
                                self.cnt_fade_frames = conceal_params.num_fade_in_frames - 1;
                            } else {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        }
                    }
                    ConcealmentState::FadeIn => {
                        self.cnt_fade_frames -= 1; // Used to address the fade-in factors.

                        if is_frame_ok || self.is_prev_frame_ok[1] {
                            if self.cnt_fade_frames < 0 {
                                // Change to state OK.
                                self.conceal_state = ConcealmentState::Ok;
                            }
                        } else if conceal_params.num_fade_out_frames > 0 {
                            // Change to state FADE-OUT.
                            self.conceal_state = ConcealmentState::FadeOut;

                            // FadeIn -> FadeOut.
                            self.cnt_fade_frames = conceal_params.find_equi_fade_frame(
                                self.cnt_fade_frames + 1,
                                FadeDirection::InToOut,
                            );
                        } else {
                            // Change to state MUTE.
                            self.conceal_state = ConcealmentState::Mute;
                        }
                    }
                }
            }
            ConcealmentMethod::Mute | ConcealmentMethod::None => {
                // Don't need a state machine for other concealment methods.
            }
        }
    }

    /// Apply concealment for given channel.
    /// Interface function to different concealment techniquies.
    ///
    /// # Parameters
    ///
    /// - `conceal_params`: Concealment params instance with valid data(from ConcealmentData)
    /// - `lpd_data_option`: LpdData instance with valid internal data if available
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `sr_info`: Sampling rate info instance with valid data
    /// - `spectral_coefficient`: Spectral coefficients of current frame
    /// - `spectral_coefficient_prev`: Spectral coefficients of previous frame
    /// - `render_mode`: Type of AAC decoder's render mode (AacDecoderRenderMode)
    /// - `ac_flags`: Scalefactors for each band in each window
    /// - `is_frame_ok`: Flag indicates whether current frame is Ok (or) defective
    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply(
        &mut self,
        conceal_params: &ConcealmentParams,
        lpd_data_option: Option<&mut LpdData>,
        ics_info: &mut IcsInfo,
        sr_info: &SamplingRateInfo,
        spectral_coefficient: &mut [f32],
        spectral_coefficient_prev: &mut [f32],
        render_mode: &mut AacDecoderRenderMode,
        ac_flags: ACFlags,
        is_frame_ok: bool,
    ) {
        self.is_mute_release = is_frame_ok
            && (self.conceal_state >= ConcealmentState::Mute)
            && ((self.cnt_valid_frames + 1) <= conceal_params.num_mute_release_frames);

        if !self.is_conceal_defined {
            if *render_mode == AacDecoderRenderMode::Lpd {
                self.window_shape = WindowShape::Sine; //Sine window
            } else {
                // Initialize window_shape with same value as in the current (parsed)
                // frame. Because section 4.6.11.3.2 (Windowing and block switching) of
                // ISO/IEC 14496-3:2009 says: For the first raw_data_block() to be decoded
                // the window_shape of the left and right half of the window are identical.
                self.window_shape = ics_info.window_shape();
            }

            self.is_conceal_defined = true; // Conceal window_shape has been updated.
        }

        if is_frame_ok && !self.is_mute_release {
            // Update render mode if frameOk except for ongoing mute release state.
            self.last_render_mode = *render_mode;
            {
                if let Some(lpd_data) = &lpd_data_option {
                    let lpc4_lsf = lpd_data.lpc4_lsf();
                    // Get mode of specific LPD frame.
                    let lpd_frame_mode = lpd_data.lpd_frame_mode(lpd_constants::NB_DIV - 1);
                    // Rescue current data for concealment in future frames.
                    self.store(
                        conceal_params.method,
                        *render_mode,
                        ics_info,
                        spectral_coefficient,
                        spectral_coefficient_prev,
                        lpc4_lsf,
                        lpd_data.last_tcx_gain(),
                        lpd_frame_mode,
                    );
                } else {
                    let lpc4_lsf = [0.0f32; 0];
                    let lpd_frame_mode = LpdMode::Acelp;
                    // Rescue current data for concealment in future frames.
                    self.store(
                        conceal_params.method,
                        *render_mode,
                        ics_info,
                        spectral_coefficient,
                        spectral_coefficient_prev,
                        &lpc4_lsf[..],
                        0.0_f32,
                        lpd_frame_mode,
                    );
                }
            }
            // Reset index to random sign vector to make sign calculation frame agnostic
            // (only depends on number of subsequently concealed spectral blocks).
            self.random_phase = 0;
        } else {
            if self.last_render_mode == AacDecoderRenderMode::Invalid {
                self.last_render_mode = if ac_flags.contains(ACFlags::ELD) {
                    AacDecoderRenderMode::EldFb
                } else {
                    AacDecoderRenderMode::Imdct
                };
            }
            *render_mode = self.last_render_mode;
        }

        let last_lpd_mode = if let Some(lpd_data) = lpd_data_option {
            let last_lpd_mode = lpd_data.last_lpd_mode;
            // Hand current frame status to the state machine.
            self.update_state(
                conceal_params,
                last_lpd_mode,
                ics_info,
                spectral_coefficient,
                spectral_coefficient_prev,
                is_frame_ok,
            );

            if !is_frame_ok
                && *render_mode == AacDecoderRenderMode::Imdct
                && ac_flags.contains(ACFlags::USAC)
            {
                // LPC extrapolation.
                lpd_data.lpc_extrapolation(self.last_render_mode == AacDecoderRenderMode::Imdct);
                self.lsf4.copy_from_slice(lpd_data.lpc4_lsf());
            }

            // Create data for signal rendering according to the selected concealment
            // method and decoder operating mode.
            if (!is_frame_ok || self.is_mute_release) && (*render_mode == AacDecoderRenderMode::Lpd)
            {
                // Restore old LSF4.
                lpd_data.lpc4_lsf_mut().copy_from_slice(&self.lsf4);
                // Restore old TCX gain.
                lpd_data.set_last_tcx_gain(self.last_tcx_gain);
            }

            if ac_flags.contains(ACFlags::USAC) {
                lpd_data.set_was_last_last_frame_ok(lpd_data.was_last_frame_ok());
                lpd_data.set_was_last_frame_ok(is_frame_ok);
            }
            last_lpd_mode // Return value for later usage.
        } else {
            // Hand current frame status to the state machine.
            self.update_state(
                conceal_params,
                LpdMode::Acelp, // Note: It is default case in apply_fade_out.
                ics_info,
                spectral_coefficient,
                spectral_coefficient_prev,
                is_frame_ok,
            );
            LpdMode::Acelp // last_lpd_mode - Return value for later usage.
        };

        {
            // Get mode of specific LPD frame.
            if !(*render_mode == AacDecoderRenderMode::Lpd) || !last_lpd_mode.is_acelp() {
                match conceal_params.method {
                    ConcealmentMethod::None | ConcealmentMethod::Mute => {
                        if !is_frame_ok {
                            // Mute spectral data in case of errors.
                            spectral_coefficient.fill(0.0f32);
                            // Set last window shape.
                            ics_info.set_window_shape(self.window_shape);
                        }
                    }
                    ConcealmentMethod::Noise => {
                        // Noise substitution error concealment technique.
                        self.apply_noise(
                            conceal_params.num_fade_out_frames.try_into().unwrap(),
                            ics_info,
                            spectral_coefficient,
                            spectral_coefficient_prev,
                            last_lpd_mode,
                        );
                    }
                    ConcealmentMethod::Inter => {
                        // Energy interpolation concealment based on 3GPP.
                        self.apply_interpolation(
                            spectral_coefficient,
                            spectral_coefficient_prev,
                            ics_info,
                            sr_info,
                            is_frame_ok,
                        );
                    }
                }
            } else if !is_frame_ok || self.is_mute_release {
                // Simply restore the buffer.

                // Restore window infos.
                ics_info.set_window_sequence(self.window_sequence);
                ics_info.set_window_shape(self.window_shape);

                if self.conceal_state != ConcealmentState::Mute {
                    // Restore spectral bins.
                    spectral_coefficient.copy_from_slice(spectral_coefficient_prev);
                } else {
                    // Clear buffer.
                    spectral_coefficient.fill(0.0f32);
                }
            }
        }

        // Update history.
        self.is_prev_frame_ok[0] = self.is_prev_frame_ok[1];
        self.is_prev_frame_ok[1] = is_frame_ok;
    }

    /// Generates and updates random noise based on input seed.
    pub(super) fn timedomain_noise_random(seed: &mut u32) -> f32 {
        {
            // Note: Don't use return value to decide sign of noise.
            let _ = utils::random_noise_generator(seed);
        }
        // Note: Direct conversion to f32 (*seed as f32) leads to wrong value.
        (*seed as i32) as f32
    }

    /// Apply time domain noise to given PCM data.
    /// This function creates filtered noise based on random
    /// noise generator and filter processing. Also normalize the noise level
    /// based on comfort_noise_level and applies it on given PCM samples.
    ///
    /// # Parameters
    ///
    /// - `noise_seed`: Input noise seed to random noise generation
    /// - `comfort_noise_level`: Noise level to be applied on noise value
    /// - `pcm_data`: Input/Output PCM data on which noise to be applied
    fn apply_timedomain_noise(
        &mut self,
        noise_seed: u32,
        comfort_noise_level: f32,
        pcm_data: &mut [f32],
    ) {
        if (self.conceal_state != ConcealmentState::Ok
            || self.conceal_state_old != ConcealmentState::Ok)
            && comfort_noise_level != 0.0_f32
        {
            let coef: [f32; 3] = [0.05, 0.5, 0.45];
            let states = &mut self.td_noise_states;
            let mut seed = noise_seed;

            for pcm in pcm_data.iter_mut() {
                // Create filtered noise.
                states[2] = states[1];
                states[1] = states[0];

                states[0] = Self::timedomain_noise_random(&mut seed) / 65536.0_f32;
                let mut noise_val = states[0] * coef[0] + states[1] * coef[1] + states[2] * coef[2];
                noise_val *= comfort_noise_level;

                // Add filtered noise - check for clipping, before.
                if ((*pcm > (32767.0_f32 - noise_val)) && (noise_val > 0.0_f32))
                    || ((*pcm < (-32768.0_f32 - noise_val)) && (noise_val < 0.0_f32))
                {
                    noise_val = -noise_val;
                }
                *pcm += noise_val;
            }
        }
    }

    /// Attenuates pcm_data in time domain fading process.
    /// Step by step fading applied to PCM samples.
    ///
    /// # Parameters
    ///
    /// - `fade_start`: Gain value at start of fading
    /// - `fade_stop`: Gain value at end of fading
    /// - `pcm_data`: PCM data to be attenuated
    fn timedomain_fade_pcm_attenuate(fade_start: f32, fade_stop: f32, pcm_data: &mut [f32]) {
        let length = pcm_data.len();

        // set start energy
        let mut gain = fade_start;
        // Determine energy steps from sample to sample.
        let step = (fade_start - fade_stop) / length as f32;

        for pcm in pcm_data.iter_mut().take(length) {
            gain -= step;
            // Prevent gain from getting negative due to possible inaccuracies.
            gain = gain.max(0.0_f32);
            // Finally, attenuate samples.
            *pcm *= gain;
        }
    }

    /// Fill liner fading steps value (i.e set step value to 1 for all steps).
    fn timedomain_do_linear_fading_steps(fading_steps: &mut [i32]) {
        fading_steps.fill(1_i32);
    }

    // Fill fadingstations.
    // The fadingstations are the attenuation factors, being applied to its dedicated
    // portions of pcm data. They are calculated using the fadingsteps. One fadingstep
    // is the weighted contribution to the fading slope within its dedicated portion of
    // pcm data.
    //
    // Fadingsteps  :      0  0  0  1  0  1  2  0
    //
    //                   |<-  1 Frame pcm data ->|
    //       fadeStart-->|__________             |
    //                   ^  ^  ^  ^ \____        |
    //  Attenuation  :   |  |  |  |  ^  ^\__     |
    //                   |  |  |  |  |  |  ^\    |
    //                   |  |  |  |  |  |  | \___|<-- fadeStop
    //                   |  |  |  |  |  |  |  ^  ^
    //                   |  |  |  |  |  |  |  |  |
    // Fadingstations:  [0][1][2][3][4][5][6][7][8]
    //
    // (Fadingstations "[0]" is "[8] from previous frame", therefore its not meaningful
    // to be edited)

    fn timedomain_fill_fading_stations(
        fading_stations: &mut [f32],
        fading_steps: &[i32],
        fade_stop: f32,
        fade_start: f32,
    ) {
        let fading_steps_sum = fading_steps[..8].iter().sum::<i32>();

        let fade_diff = (fade_stop - fade_start) / (1.0_f32.max(fading_steps_sum as f32));
        fading_stations[0] = fade_start;

        for i in 1..8 {
            fading_stations[i] = fading_stations[i - 1] + fade_diff * (fading_steps[i - 1] as f32);
        }
        fading_stations[8] = fade_stop;
    }

    ///  Do Time domain fading (TDFading) in concealment case.
    ///
    ///   Workflow:
    ///   1.) Determine Fading behavior and end-of-frame target fading level, based on
    /// `ConcealmentState` (determined by update_state()) and the core mode.
    ///
    /// By default, the target fading level is determined by `fade_out_factor[cnt_fade_frames]`
    /// in case of fadeOut, or `fadeInFactor[cnt_fade_frames]` in case of fadeIn.
    /// Fading type is `FadeTimedomain` in this case. Target fading level
    /// is determined by fading index cnt_fade_frames.
    ///
    /// If `ConcealmentState` is signalling a _MUTED SIGNAL_,
    /// TDFading decays to 0 within 1/8th of a frame if `num_fade_out_frames == 0`.
    /// Fading type is `Tospectralmute` in this case.
    ///
    /// If `ConcealmentState`  is signalling the _END OF MUTING_,
    /// TDFading fades to target fading level within 1/8th of a frame if
    /// `num_fade_in_frames == 0`.
    /// Fading type is `Fromspectralmute` in this case.
    ///
    /// Target fading level is determined by fading index `cnt_fade_frames`.
    ///
    ///   2.) Render fading levels within current frame and do the final fading.
    ///       Map Fading slopes to fading levels and apply to time domain signal.
    ///
    /// # Parameters
    ///
    /// - `params`: ConcealmentParams instance with valid data
    /// - `noise_seed`: Noise seed to genearte time domain noise
    /// - `pcm_data`: PCM data to be faded
    pub(super) fn timedomain_fading(
        &mut self,
        params: &ConcealmentParams,
        noise_seed: u32,
        pcm_data: &mut [f32],
    ) {
        let mut fading_steps = [0_i32; 8];
        let mut fading_stations = [0.0_f32; 9];
        let mut index = 0;
        let mut fading_type; // TDfadingType::FadeTimedomainTospectralmute; // Default case
        let td_fade_in_stop_before_full_level = 1;
        let td_fade_out_stop_before_mute = 1;
        let mut fade_factor = &params.fade_out_factor; // Default

        // 1.) Determine Fading behaviour (end-of-frame attenuation and fading type).
        match self.conceal_state {
            ConcealmentState::Single | ConcealmentState::Mute | ConcealmentState::FadeOut => {
                index = if params.method == ConcealmentMethod::Noise {
                    self.cnt_fade_frames - 1
                } else {
                    self.cnt_fade_frames
                };
                fading_type = TDfadingType::FadeTimeDomain;

                if self.conceal_state == ConcealmentState::Mute
                    || (self.cnt_fade_frames + td_fade_out_stop_before_mute)
                        > params.num_fade_out_frames
                {
                    fading_type = TDfadingType::ToSpectralMute;
                }
            }
            ConcealmentState::Ok | ConcealmentState::FadeIn => {
                if self.conceal_state == ConcealmentState::FadeIn {
                    index = self.cnt_fade_frames;
                    index -= td_fade_in_stop_before_full_level;
                }
                fade_factor = &params.fade_in_factor;
                index = if self.conceal_state == ConcealmentState::Ok {
                    -1
                } else {
                    index
                };
                fading_type = if self.conceal_state_old == ConcealmentState::Mute {
                    TDfadingType::FromSpectralMute
                } else {
                    TDfadingType::FadeTimeDomain
                };
            }
        }

        let fade_stop; //= 1.0_f32;
        let fade_start = self.fade_old;
        let att_mute = 0.0_f32;
        // Determine Target end-of-frame fading level and fading slope.
        match fading_type {
            TDfadingType::FromSpectralMute => {
                fade_stop = if index < 0 {
                    1.0_f32
                } else {
                    fade_factor[index as usize]
                };
                if params.num_fade_in_frames == 0 {
                    // Do step as fast as possible.
                    fading_steps[0] = 1;
                } else {
                    Self::timedomain_do_linear_fading_steps(&mut fading_steps);
                }
            }
            TDfadingType::FadeTimeDomain => {
                fade_stop = if index < 0 {
                    1.0_f32
                } else {
                    fade_factor[index as usize]
                };
                Self::timedomain_do_linear_fading_steps(&mut fading_steps);
            }
            TDfadingType::ToSpectralMute => {
                fade_stop = att_mute;
                if params.num_fade_out_frames == 0 {
                    // Do step as fast as possible.
                    fading_steps[0] = 1;
                } else {
                    Self::timedomain_do_linear_fading_steps(&mut fading_steps);
                }
            }
        }

        // 2.) Render fading levels within current frame and do the final fading.
        Self::timedomain_fill_fading_stations(
            &mut fading_stations,
            &fading_steps,
            fade_stop,
            fade_start,
        );

        if fading_stations.iter().any(|&x| x != 1.0_f32) {
            let len: usize = pcm_data.len() >> 3;
            let mut start = 0;
            let mut end = len;
            for fs in fading_stations.windows(2).take(8) {
                Self::timedomain_fade_pcm_attenuate(fs[0], fs[1], &mut pcm_data[start..end]);
                start += len;
                end = start + len;
            }
        }

        self.apply_timedomain_noise(noise_seed, params.comfort_noise_level, pcm_data);

        // Save end-of-frame attenuation and fading type.
        self.last_fading_type = fading_type;
        self.fade_old = fade_stop;
        self.conceal_state_old = self.conceal_state;
    }
}
