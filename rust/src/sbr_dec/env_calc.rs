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
//! Envelope calculation

// Imports
use super::{
    common::{self as sbr_common, shell_sort, SbrError},
    constants::*,
    frame_data::{
        constants::{ADD_HARMONICS_FLAGS_SIZE, HI, LO},
        FrameData, SbrPatchingMode,
    },
    frame_info::FreqRes,
    freq_sca::FreqBandData,
    hbe::HBE_MAX_STRETCH,
    header_data::HeaderData,
    lpp_trans::PatchParam,
    pvc::{Mode, PvcDecoder},
};
use crate::{
    common::{
        arith_ops::{self},
        qmf_domain::{QMF_DOMAIN_MAX_OV_TIMESLOTS, QMF_DOMAIN_MAX_TIMESLOTS},
    },
    sbr_dec::flags::SbrDecFlags,
};
use itertools::izip;
use num_complex::Complex;
use std::array;

/// Size of random number array for noise floor.
const N_NF_RND_VALS: usize = 512;
/// SBR gain limitation for concealment.
const GAIN_CONCEAL_LIMITATION: f32 = 1.0;
/// SBR boost gain for limiter band.
const BOOST_GAIN_LIMIT: f32 = 2.51188643;

/// Structure that holds data for SBR envelope calculation.
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct EnvelopeData {
    /// Previous gains (required for smoothing).
    filt_buffer: [f32; MAX_FREQ_COEFFS],
    /// Previous noise levels (required for smoothing).
    filt_buffer_noise: [f32; MAX_FREQ_COEFFS],
    /// Flag to signal initial conditions in buffers.
    is_start_up: bool,
    /// Index for `RANDOM_PHASE[]` array.
    phase_index: i32,
    /// The transient envelope of the previous frame. -1 if none.
    prev_tran_env: i8,
    /// Words with 16 flags each indicating where a sine was added in the previous frame.
    harm_flags_prev: [u32; ADD_HARMONICS_FLAGS_SIZE],
    /// Index of harmonics.
    harm_index: u8,
    /// Noise envelope (required for differential-coded values) of the previous frame.
    prev_noise_floor_level: [f32; MAX_NOISE_COEFFS],
    /// Sinusoidal position in the previous frame.
    sinusoidal_position_prev: i8,
    /// Harmonic flags of previous frame.
    harm_flags_prev_active: [u32; ADD_HARMONICS_FLAGS_SIZE],
    /// Limiter band table.
    limiter_band_table: [u8; MAX_NUM_LIMITER_BANDS + 1],
    /// Number of limiter bands.
    num_limiter_bands: u8,
}

impl Default for EnvelopeData {
    fn default() -> Self {
        EnvelopeData {
            filt_buffer: [0.0f32; MAX_FREQ_COEFFS],
            filt_buffer_noise: [0.0f32; MAX_FREQ_COEFFS],
            is_start_up: false,
            phase_index: 0,
            prev_tran_env: 0,
            harm_flags_prev: [0; ADD_HARMONICS_FLAGS_SIZE],
            harm_index: 0,
            prev_noise_floor_level: [0.0; MAX_NOISE_COEFFS],
            sinusoidal_position_prev: 0,
            harm_flags_prev_active: [0; ADD_HARMONICS_FLAGS_SIZE],
            limiter_band_table: [0; MAX_NUM_LIMITER_BANDS + 1],
            num_limiter_bands: 0,
        }
    }
}

impl EnvelopeData {
    /// Calculates and applies spectral envelope to subband samples.
    ///
    /// The control data in `FrameData` contains envelope data which is unpacked within this
    /// function. Sine flags from bitstream mapped to QMF bands. For each envelope, the following
    /// steps are performed.
    ///
    ///  - Energy calculation: Calculates energy in the signal to be adjusted. Depending on the
    ///    value of interpolation mode, this is either done separately for each QMF-subband or for
    ///    each SBR-band. The resulting energies are stored in the buffer.
    ///  - Level calculation: Calculates gain, noise level and sine levels. The resulting values are
    ///    stored in the buffers.
    ///  - Noise limiting: The gain for each subband is limited both absolutely and relatively
    ///    compared to the total gain over all subbands.
    ///  - Boost gain: Calculates and applies boost factor for each limiter band in order to
    ///    compensate for the energy loss imposed by the limiting.
    ///  - Apply gains and add noise: The gains and noise levels are applied to all timeslots of the
    ///    current envelope. A short FIR-filter (length 4 QMF-timeslots) can be used to smooth the
    ///    sudden change at the envelope borders. Each complex subband sample of the current
    ///    timeslot is multiplied by the smoothed gain, then random noise with the calculated level
    ///    is added.
    ///
    /// # Parameters
    ///
    /// - `header_data`: SBR header data instance.
    /// - `frame_data`: SBR frame data instance.
    /// - `pvc_data`: PVC decoder instance.
    /// - `ana_buffer_real`: Real part of subband samples to be processed.
    /// - `ana_buffer_imag`: Imag part of subband samples to be processed.
    /// - `flags`: SBR decoder flags.
    /// - `frame_error_flag`: Flag to indicate valid frame data.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn apply(
        &mut self,
        header_data: &mut HeaderData,
        frame_data: &FrameData,
        mut pvc_data: Option<&mut PvcDecoder>,
        ana_buffer_real: &mut [&mut [f32];
                 (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
        ana_buffer_imag: &mut [&mut [f32];
                 (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize],
        flags: SbrDecFlags,
        frame_error_flag: bool,
    ) {
        let is_ites_enable = frame_data.inter_tes_active();
        let borders_pvc = frame_data.frame_info.pvc_borders();
        let borders = frame_data.frame_info.borders();
        let border_noise = frame_data.frame_info.border_noise();
        let tran_env = frame_data.frame_info.tran_env();
        let num_envelopes = frame_data.frame_info.num_envelopes();
        let i_envelope = frame_data.i_envelope();
        let mut i_envelope_idx = 0;

        let limiter_gain_index = usize::from(header_data.bitstream().limiter_gains());
        let smooth_len = header_data.bitstream().smoothing_length();
        let is_interpol_freq = header_data.bitstream().interpol_freq() != 0;
        let fbd: FreqBandData = *header_data.freq_band_data();
        let time_step = usize::from(header_data.time_step());
        let fbd_prev = header_data.freq_band_data_prev_mut();

        let mut sine_mapped = [0_i8; MAX_FREQ_COEFFS];
        let prev_noise_floor_level = self.prev_noise_floor_level;

        let pvc_data_mode = if let Some(pvc) = &pvc_data {
            pvc.mode()
        } else {
            Mode::None
        };

        let is_usac_syntax_with_pvc =
            flags.contains(SbrDecFlags::SYNTAX_USAC) && pvc_data_mode > Mode::None;

        let mut env_noise = 0;
        let mut noise_levels = frame_data.sbr_noise_floor_level();
        let mut noise_levels_idx = 0_usize;
        let mut num_noise_bands = fbd.num_freq_bands_noise();
        let mut num_subframe_bands = [fbd.num_freq_bands_sbr(LO), fbd.num_freq_bands_sbr(HI)];
        let mut low_subband = fbd.start_subband;
        let mut high_subband = fbd.end_subband;
        let mut num_subbands = high_subband - low_subband;

        // If values differ we had a headerchange. If the old highband is bigger
        // than the new one, we need to patch the overlap-highband-scaling for
        // this frame (see use of `ov_high_subband`), as overlap contains higher
        // frequency components which would get lost.
        let mut ov_high_subband = high_subband.max(fbd.old_end_subband);

        // Set to false when not necessary to copy back previous data.
        let mut do_copy_prev_data = true;

        {
            let mut freq_band_table_low = fbd.freq_band_table_low();
            let mut freq_band_table_high = fbd.freq_band_table_high();
            let mut freq_band_table_noise = fbd.freq_band_table_noise();

            if is_usac_syntax_with_pvc && border_noise[0] > borders_pvc[0] {
                // Noise envelope of previous frame is trailing into current PVC frame.
                env_noise = -1_i16;
                noise_levels = &prev_noise_floor_level;
                noise_levels_idx = 0_usize;
                num_noise_bands = fbd_prev.num_freq_bands_noise();
                num_subframe_bands = [
                    fbd_prev.num_freq_bands_sbr(LO),
                    fbd_prev.num_freq_bands_sbr(HI),
                ];
                low_subband = fbd_prev.start_subband;
                high_subband = fbd_prev.end_subband;
                num_subbands = high_subband - low_subband;

                ov_high_subband = if high_subband < fbd_prev.old_end_subband {
                    fbd_prev.old_end_subband
                } else {
                    high_subband
                };

                freq_band_table_low = fbd_prev.freq_band_table_low();
                freq_band_table_high = fbd_prev.freq_band_table_high();
                freq_band_table_noise = fbd_prev.freq_band_table_noise();

                // We're actually using the previous data, therefore it is not necessary to copy
                // it back again later.
                do_copy_prev_data = false;
            }

            // Extract sine flags for all QMF bands.
            self.extract_sine_flags(
                frame_data,
                freq_band_table_high,
                &mut sine_mapped,
                num_subframe_bands[HI],
                is_usac_syntax_with_pvc,
            );

            // Calculate adjustment factors and apply them for every envelope.
            let (ts_start, ts_stop) = if is_usac_syntax_with_pvc {
                // iterate over SBR time slots starting with borders_pvc[num_envelopes].
                // usually 0; can be >0 if switching from legacy SBR to PVC.

                debug_assert!(borders_pvc[num_envelopes] == PVC_NTIMESLOT);
                (usize::from(borders_pvc[0]), PVC_NTIMESLOT as usize)
            } else {
                // iterate over SBR envelopes starting with 0.
                (0, num_envelopes)
            };

            for i in ts_start..ts_stop {
                // Helper variables.
                let (start_pos, stop_pos, freq_res) = if is_usac_syntax_with_pvc {
                    // Start-position in time (subband sample) for current envelope.
                    let start_position = time_step * i;
                    // Stop-position in time (subband sample) for current envelope.
                    let stop_position = time_step * (i + 1);
                    // Frequency resolution for current envelope.
                    let freq_resolution = frame_data.frame_info.freq_res()[0];

                    debug_assert!(
                        freq_resolution == frame_data.frame_info.freq_res()[num_envelopes - 1]
                    );

                    (start_position, stop_position, freq_resolution)
                } else {
                    // Start-position in time (subband sample) for current envelope.
                    let start_position = time_step * usize::from(borders[i]);
                    // Stop-position in time (subband sample) for current envelope.
                    let stop_position = time_step * usize::from(borders[i + 1]);
                    // Frequency resolution for current envelope.
                    let freq_resolution = frame_data.frame_info.freq_res()[i];

                    (start_position, stop_position, freq_resolution)
                };
                // Always fully initialize the temporary energy table. This prevents
                // negative energies and extreme gain factors in cases where the number of
                // limiter bands exceeds the number of subbands. The latter can be caused by
                // undetected bit errors and is tested by some streams from the
                // certification set.
                let mut env_calc_nrgs = EnvCalcNrgs::default();

                if is_usac_syntax_with_pvc {
                    // Get predicted energy values from PVC module.
                    pvc_data
                        .as_mut()
                        .unwrap()
                        .expand_pred_esg(i, &mut env_calc_nrgs.nrg_ref);

                    if i == usize::from(borders[0]) {
                        self.map_sine_flags(
                            freq_band_table_high,
                            frame_data.add_harmonics(),
                            &mut sine_mapped,
                            usize::from(num_subframe_bands[HI]),
                            frame_data.sinusoidal_position().try_into().unwrap(),
                        );
                    }

                    if i >= border_noise[(env_noise + 1) as usize] as usize {
                        if env_noise >= 0 {
                            // The noise floor data is stored in a row [noiseFloor1 noiseFloor2...].
                            noise_levels_idx += usize::from(num_noise_bands);
                        } else {
                            // Leave trailing noise envelope of past frame.
                            num_noise_bands = fbd.num_freq_bands_noise();
                            num_subframe_bands =
                                [fbd.num_freq_bands_sbr(LO), fbd.num_freq_bands_sbr(HI)];
                            noise_levels = frame_data.sbr_noise_floor_level();
                            noise_levels_idx = 0;
                            low_subband = fbd.start_subband;
                            high_subband = fbd.end_subband;
                            num_subbands = high_subband - low_subband;
                            ov_high_subband = high_subband;

                            if high_subband < fbd.old_end_subband {
                                ov_high_subband = fbd.old_end_subband;
                            }

                            freq_band_table_low = fbd.freq_band_table_low();
                            freq_band_table_high = fbd.freq_band_table_high();
                            freq_band_table_noise = fbd.freq_band_table_noise();

                            // We're using current data, therefore we must copy back the previous
                            // data later.
                            do_copy_prev_data = true;
                        }
                        env_noise += 1;
                    }
                } else {
                    // If the start-pos of the current envelope equals the stop pos of the
                    // current noise envelope, increase the pointer (i.e. choose the next
                    // noise-floor).
                    if borders[i] == border_noise[(env_noise + 1) as usize] {
                        // The noise floor data is stored in a row [noiseFloor1 noiseFloor2...].
                        noise_levels_idx += usize::from(num_noise_bands);
                        env_noise += 1;
                    }
                }

                // Attack.
                let (no_noise_flag, smooth_length) =
                    if i as i16 == tran_env as i16 || i as i16 == self.prev_tran_env as i16 {
                        // No smoothing on attacks!.
                        (true, 0)
                    } else {
                        // can become either 0 or 4.
                        (false, (1 - smooth_len) << 2)
                    };

                let freq_table = if freq_res == FreqRes::Coarse {
                    freq_band_table_low
                } else {
                    freq_band_table_high
                };

                // Energy estimation in transposed highband.
                {
                    let mut converted_real: [&[f32];
                        (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize] =
                        array::from_fn(|_| [].as_slice());
                    let mut converted_imag: [&[f32];
                        (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize] =
                        array::from_fn(|_| [].as_slice());

                    sbr_common::convert_mut_to_immut_slices(
                        &ana_buffer_real[start_pos..stop_pos],
                        &mut converted_real,
                    );
                    sbr_common::convert_mut_to_immut_slices(
                        &ana_buffer_imag[start_pos..stop_pos],
                        &mut converted_imag,
                    );

                    if is_interpol_freq {
                        Self::calc_nrg_per_subband(
                            &converted_real[..stop_pos - start_pos],
                            &converted_imag[..stop_pos - start_pos],
                            &mut env_calc_nrgs.nrg_est,
                            usize::from(low_subband),
                            usize::from(high_subband),
                        );
                    } else {
                        Self::calc_nrg_per_sfb(
                            &converted_real[..stop_pos - start_pos],
                            &converted_imag[..stop_pos - start_pos],
                            &mut env_calc_nrgs.nrg_est,
                            freq_table,
                            usize::from(num_subframe_bands[freq_res as usize]),
                        );
                    }
                }

                // Calculate subband gains.
                {
                    let mut upper_noise_idx = 1;
                    let mut tmp_noise_levels_idx = noise_levels_idx;
                    let mut tmp_noise = noise_levels[tmp_noise_levels_idx];
                    tmp_noise_levels_idx += 1;

                    let mut cc = 0;
                    let mut c = 0;
                    if is_usac_syntax_with_pvc {
                        for table in freq_table
                            .windows(2)
                            .take(usize::from(num_subframe_bands[freq_res as usize]))
                        {
                            let mut sine_present_flag = false;
                            let iter_len = usize::from(table[1] - table[0]);

                            for &sc in sine_mapped[cc..].iter().take(iter_len) {
                                sine_present_flag |= i as i16 >= sc as i16;
                            }
                            cc += iter_len;

                            for k in table[0]..table[1] {
                                debug_assert!(k >= low_subband);
                                let ref_nrg = env_calc_nrgs.nrg_ref[(k - low_subband) as usize];

                                if k >= freq_band_table_noise[upper_noise_idx] {
                                    tmp_noise = noise_levels[tmp_noise_levels_idx];
                                    tmp_noise_levels_idx += 1;
                                    upper_noise_idx += 1;
                                }

                                env_calc_nrgs.nrg_sine[c] = 0.0f32;

                                env_calc_nrgs.calc_subband_gain(
                                    ref_nrg,
                                    c,
                                    tmp_noise,
                                    sine_present_flag,
                                    i as i16 >= sine_mapped[c] as i16,
                                    no_noise_flag,
                                );

                                c += 1;
                            }
                        }
                    } else {
                        for table in freq_table
                            .windows(2)
                            .take(usize::from(num_subframe_bands[freq_res as usize]))
                        {
                            let ref_nrg = i_envelope[i_envelope_idx];
                            let mut sine_present_flag = false;
                            let iter_len = usize::from(table[1] - table[0]);

                            for &sc in sine_mapped[cc..].iter().take(iter_len) {
                                sine_present_flag |= i as i16 >= sc as i16;
                            }
                            cc += iter_len;

                            for k in table[0]..table[1] {
                                if k >= freq_band_table_noise[upper_noise_idx] {
                                    tmp_noise = noise_levels[tmp_noise_levels_idx];
                                    tmp_noise_levels_idx += 1;
                                    upper_noise_idx += 1;
                                }

                                debug_assert!(k >= low_subband);

                                env_calc_nrgs.nrg_sine[c] = 0.0f32;

                                env_calc_nrgs.calc_subband_gain(
                                    ref_nrg,
                                    c,
                                    tmp_noise,
                                    sine_present_flag,
                                    i as i16 >= sine_mapped[c] as i16,
                                    no_noise_flag,
                                );

                                env_calc_nrgs.nrg_ref[c] = ref_nrg;

                                c += 1;
                            }
                            i_envelope_idx += 1;
                        }
                    }
                }

                // Noise limiting and boosting gain.
                self.compress_signal(
                    &mut env_calc_nrgs,
                    limiter_gain_index,
                    frame_error_flag,
                    no_noise_flag,
                );

                // Convert energies to amplitude levels.
                env_calc_nrgs.convert_nrgs_to_amp_level(usize::from(num_subbands));

                // Apply calculated gains and adaptive noise.
                {
                    let gamma_index = if is_ites_enable {
                        Some(frame_data.inter_temp_shape_mode()[i])
                    } else {
                        // gamma_index is only used if is_ites_enable is true
                        None
                    };
                    self.apply_gains_and_adaptive_noise(
                        &env_calc_nrgs,
                        &mut ana_buffer_real[start_pos..stop_pos],
                        &mut ana_buffer_imag[start_pos..stop_pos],
                        low_subband,
                        num_subbands,
                        gamma_index,
                        smooth_length,
                        is_ites_enable,
                        no_noise_flag,
                    );
                }
            }
        }

        // We need to remember in the next frame that the transient
        // will occur in the first envelope (if tranEnv == nEnvelopes).
        self.prev_tran_env = if tran_env == num_envelopes as i8 {
            0
        } else {
            -1
        };

        if is_usac_syntax_with_pvc {
            let num_noise_envelopes = frame_data.frame_info.num_noise_envelopes();
            debug_assert!(border_noise[num_noise_envelopes - 1] < PVC_NTIMESLOT);
            if border_noise[num_noise_envelopes] > PVC_NTIMESLOT {
                fbd_prev.set_num_freq_bands_noise(num_noise_bands);
                fbd_prev.set_num_freq_bands_sbr(num_subframe_bands);
                fbd_prev.set_start_subband(low_subband);
                fbd_prev.set_end_subband(high_subband);
                fbd_prev.set_old_end_subband(ov_high_subband);

                if do_copy_prev_data {
                    let freq_band_table_low = fbd.freq_band_table_low();
                    let freq_band_table_high = fbd.freq_band_table_high();
                    let freq_band_table_noise = fbd.freq_band_table_noise();

                    fbd_prev.freq_band_table_low_mut()[..(num_subframe_bands[LO] + 1) as usize]
                        .copy_from_slice(
                            &freq_band_table_low[..(num_subframe_bands[LO] + 1) as usize],
                        );
                    fbd_prev.freq_band_table_high_mut()[..(num_subframe_bands[HI] + 1) as usize]
                        .copy_from_slice(
                            &freq_band_table_high[..(num_subframe_bands[HI] + 1) as usize],
                        );
                    fbd_prev
                        .freq_band_table_noise_mut()
                        .copy_from_slice(freq_band_table_noise);
                }

                self.prev_noise_floor_level.copy_from_slice(
                    &noise_levels[noise_levels_idx..noise_levels_idx + MAX_NOISE_COEFFS],
                );
            }
        }
    }

    /// Initializes `EnvelopeData` instance.
    pub(super) fn init(&mut self) {
        // Clear previous missing harmonics flags.
        self.harm_flags_prev.fill(0);
        self.harm_flags_prev_active.fill(0);

        self.harm_index = 0;

        self.prev_noise_floor_level.fill(0.0f32);

        self.prev_tran_env = -1;

        self.reset();
    }

    /// Resets envelope instance. This function must be called for each channel on a change of
    /// configuration.
    pub(super) fn reset(&mut self) {
        self.phase_index = 0;
        self.is_start_up = true;
    }

    /// Resets limiter bands. Builds frequency band table for the gain limiter dependent on the
    /// previously generated transposer patch areas.
    ///
    /// # Parameters
    ///
    /// - `patch_param`: Patch parameters.
    /// - `freq_band_table`: Frequency band table from 'HeaderData`.
    /// - `num_freq_bands`: Number of frequency bands.
    /// - `num_patches`: Number of patches.
    /// - `limiter_bands`: Index for number of limiter bands per octave
    ///   ('LIMITER_BANDS_PER_OCTAVE[]`).
    /// - `patch_mode`: Transposer patching mode, type of `SbrPatchingMode`.
    /// - `x_over_qmf`: The qmf crossover bands for the HBE transposer.
    /// - `is_b41sbr`: The flag to indicate usage of 4to1 SBR in HBE transposer.
    ///
    /// # Return
    ///
    /// - Result<(), SbrError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SbrError)` if there is an error.
    #[expect(clippy::too_many_arguments)]
    pub fn reset_limiter_bands(
        &mut self,
        patch_param: Option<&[PatchParam]>,
        freq_band_table: &[u8],
        num_freq_bands: usize,
        num_patches: usize,
        limiter_bands: usize,
        patch_mode: SbrPatchingMode,
        x_over_qmf: &Option<[u8; MAX_NUM_PATCHES]>,
        is_b41sbr: bool,
    ) -> Result<(), SbrError> {
        let low_subband = freq_band_table[0];
        let high_subband = freq_band_table[num_freq_bands];

        // 1 limiter band.
        if limiter_bands == 0 {
            self.limiter_band_table[0] = 0;
            self.limiter_band_table[1] = high_subband - low_subband;
            self.num_limiter_bands = 1;
        } else {
            let mut work_limiter_band_table = [0_u8; MAX_FREQ_COEFFS / 2 + MAX_NUM_PATCHES + 1];
            let mut patch_borders = [0_u8; MAX_NUM_PATCHES + 1];
            let mut num_patches_loc = 0;

            if patch_mode == SbrPatchingMode::Hbe {
                if let Some(x_over_qmf) = x_over_qmf {
                    let range_end = if is_b41sbr {
                        MAX_NUM_PATCHES
                    } else {
                        HBE_MAX_STRETCH as usize
                    };
                    let slice = &x_over_qmf[1..range_end];
                    num_patches_loc = slice.iter().filter(|xoq| **xoq != 0).count();
                    for (pb, xoq) in
                        izip!(patch_borders.iter_mut(), x_over_qmf.iter()).take(num_patches_loc)
                    {
                        *pb = *xoq - low_subband;
                    }
                }
            } else if let Some(patch_param) = patch_param {
                num_patches_loc = num_patches;
                for (pb, pp) in
                    izip!(patch_borders.iter_mut(), patch_param.iter()).take(num_patches_loc)
                {
                    *pb = pp.guard_start_band() - low_subband;
                }
            }

            patch_borders[num_patches_loc] = high_subband - low_subband;

            // 1.2, 2, or 3 limiter bands/octave plus bandborders at patch_borders.
            for (w_fbt, fbt) in izip!(work_limiter_band_table.iter_mut(), freq_band_table.iter())
                .take(num_freq_bands + 1)
            {
                *w_fbt = *fbt - low_subband;
            }

            if num_patches_loc > 0 {
                work_limiter_band_table[num_freq_bands + 1..num_freq_bands + num_patches_loc]
                    .copy_from_slice(&patch_borders[1..num_patches_loc]);
            }

            let mut num_bands = num_freq_bands + num_patches_loc - 1;
            let temp_no_lim = num_bands;

            shell_sort(&mut work_limiter_band_table[..=temp_no_lim]);

            let mut low_lim_index = 0;
            let mut high_lim_index = 1;

            'lim_band_loop: loop {
                if high_lim_index <= temp_no_lim {
                    let k2 = work_limiter_band_table[high_lim_index] + low_subband;
                    let kx = work_limiter_band_table[low_lim_index] + low_subband;

                    // Calculate number of octaves.
                    let oct = f32::log2(k2 as f32 / kx as f32);

                    // Multiply with limiterbands per octave
                    // values 1, 1.2, 2, 3

                    let temp = oct * LIMITER_BANDS_PER_OCTAVE[limiter_bands];

                    if temp < 0.49f32 {
                        if work_limiter_band_table[high_lim_index]
                            == work_limiter_band_table[low_lim_index]
                        {
                            work_limiter_band_table[high_lim_index] = high_subband;
                            num_bands -= 1;
                            high_lim_index += 1;
                            continue;
                        }
                        let mut is_patch_border = [false; 2];
                        let mut w_lbt_value = work_limiter_band_table[high_lim_index];

                        for pb in patch_borders.iter().take(num_patches_loc + 1) {
                            if w_lbt_value == *pb {
                                is_patch_border[1] = true;
                                break;
                            }
                        }

                        if !is_patch_border[1] {
                            work_limiter_band_table[high_lim_index] = high_subband;
                            num_bands -= 1;
                            high_lim_index += 1;
                            continue;
                        }

                        w_lbt_value = work_limiter_band_table[low_lim_index];
                        for pb in patch_borders.iter().take(num_patches_loc + 1) {
                            if w_lbt_value == *pb {
                                is_patch_border[0] = true;
                                break;
                            }
                        }

                        if !is_patch_border[0] {
                            work_limiter_band_table[low_lim_index] = high_subband;
                            num_bands -= 1;
                        }
                    }

                    low_lim_index = high_lim_index;
                    high_lim_index += 1;
                } else {
                    break 'lim_band_loop;
                }
            }

            shell_sort(&mut work_limiter_band_table[..=temp_no_lim]);

            // Test if algorithm exceeded maximum allowed limiter bands.
            if num_bands > MAX_NUM_LIMITER_BANDS || num_bands == 0 {
                return Err(SbrError::UnsupportedConfig);
            }

            // Restrict maximum value of limiter band table.
            if work_limiter_band_table[temp_no_lim] > high_subband {
                return Err(SbrError::UnsupportedConfig);
            }

            // Copy limiterbands from working buffer into final destination.
            self.limiter_band_table[..=num_bands]
                .copy_from_slice(&work_limiter_band_table[..=num_bands]);
            self.num_limiter_bands = num_bands as u8;
        }

        Ok(())
    }

    /// Adjusting HQ time slots: amplifying subband signal by smoothing gain factor and adding noise
    /// floor to signal if `noise flags` is active.
    #[expect(clippy::too_many_arguments)]
    fn adjust_time_slot_hq(
        &mut self,
        env_calc_nrgs: &EnvCalcNrgs,
        subband_real: &mut [f32],
        subband_imag: &mut [f32],
        low_subband: u8,
        num_subbands: usize,
        smooth_ratio: f32,
        no_noise_flag: bool,
    ) {
        let mut phase_index = self.phase_index;
        let harm_index = self.harm_index;
        let mut freq_inv_flag = low_subband & 1;
        let direct_ratio = 1.0 - smooth_ratio;

        self.phase_index = (phase_index + num_subbands as i32) & (N_NF_RND_VALS as i32 - 1);
        self.harm_index = (harm_index + 1) & 3;

        if smooth_ratio > 0.0f32 {
            for (&filt, &filt_noise, &gain, &noise_level, &sine_level, real, imag) in izip!(
                self.filt_buffer.iter(),
                self.filt_buffer_noise.iter(),
                env_calc_nrgs.nrg_gain.iter(),
                env_calc_nrgs.noise_level.iter(),
                env_calc_nrgs.nrg_sine.iter(),
                subband_real.iter_mut(),
                subband_imag.iter_mut()
            )
            .take(num_subbands)
            {
                // Smoothing: The old envelope has been bufferd and a certain ratio
                // of the old gains and noise levels is used.
                let smoothed_gain = smooth_ratio * filt + direct_ratio * gain;
                let smoothed_noise = smooth_ratio * filt_noise + direct_ratio * noise_level;
                let sig_real = *real * smoothed_gain;
                let sig_imag = *imag * smoothed_gain;

                phase_index += 1;

                if sine_level != 0.0f32 {
                    match harm_index {
                        0 => {
                            *real = sig_real + sine_level;
                            *imag = sig_imag;
                        }
                        1 => {
                            *real = sig_real;
                            *imag = if freq_inv_flag != 0 {
                                sig_imag - sine_level
                            } else {
                                sig_imag + sine_level
                            };
                        }
                        2 => {
                            *real = sig_real - sine_level;
                            *imag = sig_imag;
                        }
                        3 => {
                            *real = sig_real;
                            *imag = if freq_inv_flag != 0 {
                                sig_imag + sine_level
                            } else {
                                sig_imag - sine_level
                            };
                        }
                        _ => {
                            // Do nothing here.!
                        }
                    }
                } else if no_noise_flag {
                    // Just the amplified signal is saved .
                    *real = sig_real;
                    *imag = sig_imag;
                } else {
                    // Add noisefloor to the amplified signal.
                    phase_index &= N_NF_RND_VALS as i32 - 1;
                    let noise_real = RANDOM_PHASE[phase_index as usize].re * smoothed_noise;
                    let noise_imag = RANDOM_PHASE[phase_index as usize].im * smoothed_noise;
                    *real = sig_real + noise_real;
                    *imag = sig_imag + noise_imag;
                }
                freq_inv_flag ^= 1;
            }
        } else {
            for (&gain, &noise_level, &sine_level, real, imag) in izip!(
                env_calc_nrgs.nrg_gain.iter(),
                env_calc_nrgs.noise_level.iter(),
                env_calc_nrgs.nrg_sine.iter(),
                subband_real.iter_mut(),
                subband_imag.iter_mut()
            )
            .take(num_subbands)
            {
                let smoothed_gain = gain;
                let mut sig_real = *real * smoothed_gain;
                let mut sig_imag = *imag * smoothed_gain;

                phase_index += 1;

                if sine_level != 0.0f32 {
                    match harm_index {
                        0 => {
                            sig_real += sine_level;
                        }
                        1 => {
                            if freq_inv_flag != 0 {
                                sig_imag -= sine_level;
                            } else {
                                sig_imag += sine_level;
                            }
                        }
                        2 => {
                            sig_real -= sine_level;
                        }
                        3 => {
                            if freq_inv_flag != 0 {
                                sig_imag += sine_level;
                            } else {
                                sig_imag -= sine_level;
                            }
                        }
                        _ => {
                            // Do nothing here.
                        }
                    }
                } else if !no_noise_flag {
                    // Add noisefloor to the amplified signal.
                    let smoothed_noise = noise_level;
                    phase_index &= N_NF_RND_VALS as i32 - 1;
                    let noise_real = RANDOM_PHASE[phase_index as usize].re * smoothed_noise;
                    let noise_imag = RANDOM_PHASE[phase_index as usize].im * smoothed_noise;
                    sig_real += noise_real;
                    sig_imag += noise_imag;
                }

                *real = sig_real;
                *imag = sig_imag;

                freq_inv_flag ^= 1;
            }
        }
    }

    /// Variant of `adjust_time_slot_hq()` which only adds the additional harmonics.
    fn adjust_time_slot_hq_add_harmonics(
        &mut self,
        env_calc_nrgs: &EnvCalcNrgs,
        subband_real: &mut [f32],
        subband_imag: &mut [f32],
        low_subband: u8,
    ) {
        let harm_index = self.harm_index;
        self.harm_index = (harm_index + 1) & 3;

        let mut freq_inv_flag = low_subband & 1;

        for (sig_real, sig_imag, nrg_sine) in izip!(
            subband_real.iter_mut(),
            subband_imag.iter_mut(),
            env_calc_nrgs.nrg_sine.iter()
        ) {
            let mut sine_level = *nrg_sine;
            freq_inv_flag ^= 1;

            if sine_level != 0.0f32 {
                if harm_index & 2 != 0 {
                    // case 2,3.
                    sine_level = -sine_level;
                }
                if harm_index & 1 == 0 {
                    // case 0,2
                    *sig_real += sine_level;
                } else {
                    // case 1,3
                    if freq_inv_flag == 0 {
                        sine_level = -sine_level;
                    }

                    *sig_imag += sine_level;
                }
            }
        }
    }

    /// Variant of `adjust_time_slot_hq()` which only regards gain and noise but no additional
    /// harmonics.
    fn adjust_time_slot_hq_gain_and_noise(
        &mut self,
        env_calc_nrgs: &EnvCalcNrgs,
        subband_real: &mut [f32],
        subband_imag: &mut [f32],
        num_subbands: usize,
        smooth_ratio: f32,
        no_noise_flag: bool,
    ) {
        let mut phase_index = self.phase_index as usize;

        self.phase_index = ((phase_index + num_subbands) & (N_NF_RND_VALS - 1)) as i32;

        let direct_ratio = 1.0 - smooth_ratio;
        if smooth_ratio > 0.0f32 {
            for (&filt, &filt_noise, &gain, &noise_level, &sine_level, real, imag) in izip!(
                self.filt_buffer.iter(),
                self.filt_buffer_noise.iter(),
                env_calc_nrgs.nrg_gain.iter(),
                env_calc_nrgs.noise_level.iter(),
                env_calc_nrgs.nrg_sine.iter(),
                subband_real.iter_mut(),
                subband_imag.iter_mut()
            )
            .take(num_subbands)
            {
                // Smoothing: The old envelope has been buffered and a certain ratio
                // of the old gains and noise levels is used.
                let smoothed_gain = smooth_ratio * filt + direct_ratio * gain;
                let smoothed_noise = smooth_ratio * filt_noise + direct_ratio * noise_level;
                let sig_real = *real * smoothed_gain;
                let sig_imag = *imag * smoothed_gain;

                phase_index += 1;

                if sine_level != 0.0f32 || no_noise_flag {
                    // Just the amplified signal is saved.
                    *real = sig_real;
                    *imag = sig_imag;
                } else {
                    // Add noisefloor to the amplified signal.
                    phase_index &= N_NF_RND_VALS - 1;
                    let noise_real = RANDOM_PHASE[phase_index].re * smoothed_noise;
                    let noise_imag = RANDOM_PHASE[phase_index].im * smoothed_noise;
                    *real = sig_real + noise_real;
                    *imag = sig_imag + noise_imag;
                }
            }
        } else {
            for (&gain, &noise_level, &sine_level, real, imag) in izip!(
                env_calc_nrgs.nrg_gain.iter(),
                env_calc_nrgs.noise_level.iter(),
                env_calc_nrgs.nrg_sine.iter(),
                subband_real.iter_mut(),
                subband_imag.iter_mut()
            )
            .take(num_subbands)
            {
                let smoothed_gain = gain;
                *real *= smoothed_gain;
                *imag *= smoothed_gain;

                phase_index += 1;

                if sine_level == 0.0f32 && !no_noise_flag {
                    // Add noisefloor to the amplified signal.
                    let smoothed_noise = noise_level;
                    phase_index &= N_NF_RND_VALS - 1;
                    let noise_real = RANDOM_PHASE[phase_index].re * smoothed_noise;
                    let noise_imag = RANDOM_PHASE[phase_index].im * smoothed_noise;
                    *real += noise_real;
                    *imag += noise_imag;
                }
            }
        }
    }

    fn apply_inter_tes(
        qmf_real: &mut [&mut [f32]],
        qmf_imag: &mut [&mut [f32]],
        low_subband: usize,
        num_subbands: usize,
        gamma_index: u8,
    ) {
        // gamma[gamma_idx] = {0.0f, 1.0f, 2.0f, 4.0f}.
        let gamma: f32 = if gamma_index == 0 {
            0.0f32
        } else {
            (1 << (gamma_index - 1)) as f32
        };

        let num_subsample = qmf_real.len();
        let high_subband = low_subband + num_subbands;
        let mut total_power_low = 0.0f32;
        let mut total_power_high = 0.0f32;

        let mut ites = ItesTmp::default();

        if gamma_index > 0 {
            for (real, imag, power_low, power_high) in izip!(
                qmf_real.iter(),
                qmf_imag.iter(),
                ites.subsample_power_low.iter_mut(),
                ites.subsample_power_high.iter_mut()
            )
            .take(num_subsample)
            {
                let mut is_all_zero = true;
                // Loop range is [0..low_subband]
                'zero_check_first: for (&re, &im) in
                    izip!(real.iter(), imag.iter()).take(low_subband)
                {
                    if re != 0.0f32 || im != 0.0f32 {
                        is_all_zero = false;
                        break 'zero_check_first;
                    }
                }

                *power_low = 0.0f32;

                if !is_all_zero {
                    *power_low += arith_ops::calc_energy(&real[..low_subband]);
                    *power_low += arith_ops::calc_energy(&imag[..low_subband]);
                }

                total_power_low += *power_low;

                is_all_zero = true;

                // Loop range is [low_subband..high_subband]
                'zero_check_second: for (&re, &im) in
                    izip!(real[low_subband..].iter(), imag[low_subband..].iter()).take(num_subbands)
                {
                    if re != 0.0f32 || im != 0.0f32 {
                        is_all_zero = false;
                        break 'zero_check_second;
                    }
                }
                *power_high = 0.0f32;

                if !is_all_zero {
                    *power_high += arith_ops::calc_energy(&real[low_subband..high_subband]);
                    *power_high += arith_ops::calc_energy(&imag[low_subband..high_subband]);
                }
                total_power_high += *power_high;
            }

            if total_power_low != 0.0f32 {
                let inv_taotal_power_low = 1.0f32 / total_power_low;

                for (gain, &power_low) in
                    izip!(ites.gain.iter_mut(), ites.subsample_power_low.iter()).take(num_subsample)
                {
                    let mult_factor = power_low * num_subsample as f32;
                    *gain = f32::sqrt(mult_factor * inv_taotal_power_low);
                }
            } else {
                ites.gain[..num_subsample].fill(0.0f32);
            }

            let mut total_power_high_after = 0.0f32;

            for (gain, power_high) in
                izip!(ites.gain.iter_mut(), ites.subsample_power_high.iter_mut())
                    .take(num_subsample)
            {
                *gain = 1.0f32 + gamma * (*gain - 1.0f32);
                *gain = (*gain).max(0.2f32);

                *power_high = *power_high * *gain * *gain;
                total_power_high_after += *power_high;
            }

            let mut gain_adj_2 = 0.5f32;
            if total_power_high != 0.0f32 && total_power_high_after != 0.0f32 {
                gain_adj_2 = total_power_high / total_power_high_after;
            }
            let sqrt_gain_adj_2 = gain_adj_2.sqrt();

            for (real, imag, gain) in izip!(
                qmf_real.iter_mut(),
                qmf_imag.iter_mut(),
                ites.gain.iter_mut(),
            )
            .take(num_subsample)
            {
                *gain *= sqrt_gain_adj_2;

                for (re, im) in izip!(
                    real[low_subband..high_subband].iter_mut(),
                    imag[low_subband..high_subband].iter_mut()
                ) {
                    *re *= *gain;
                    *im *= *gain;
                }
            }
        }
    }

    /// Applies gains and adaptive noise to the current envelope.
    #[expect(clippy::too_many_arguments)]
    fn apply_gains_and_adaptive_noise(
        &mut self,
        env_calc_nrgs: &EnvCalcNrgs,
        ana_buffer_real: &mut [&mut [f32]],
        ana_buffer_imag: &mut [&mut [f32]],
        low_subband: u8,
        num_subbands: u8,
        gamma_index: Option<u8>,
        smooth_length: u8,
        is_ites_enable: bool,
        no_noise_flag: bool,
    ) {
        let low_subband_usize = usize::from(low_subband);
        let num_subbands_usize = usize::from(num_subbands);
        let smooth_length_usize = usize::from(smooth_length);

        // Initialize smoothing buffers with the first valid value.
        if self.is_start_up {
            self.filt_buffer_noise[..num_subbands_usize]
                .copy_from_slice(&env_calc_nrgs.noise_level[..num_subbands_usize]);
            self.filt_buffer[..num_subbands_usize]
                .copy_from_slice(&env_calc_nrgs.nrg_gain[..num_subbands_usize]);
            self.is_start_up = false;
        }

        for (pos, (buff_real, buff_imag)) in
            izip!(ana_buffer_real.iter_mut(), ana_buffer_imag.iter_mut(),).enumerate()
        {
            // Prevent the smoothing filter from running on constant levels.
            let smooth_ratio = if pos < smooth_length_usize {
                SMOOTH_FILTER[pos]
            } else {
                0.0f32
            };

            if is_ites_enable {
                self.adjust_time_slot_hq_gain_and_noise(
                    env_calc_nrgs,
                    &mut buff_real[low_subband_usize..],
                    &mut buff_imag[low_subband_usize..],
                    num_subbands_usize,
                    smooth_ratio,
                    no_noise_flag,
                );
            } else {
                self.adjust_time_slot_hq(
                    env_calc_nrgs,
                    &mut buff_real[low_subband_usize..],
                    &mut buff_imag[low_subband_usize..],
                    low_subband,
                    num_subbands_usize,
                    smooth_ratio,
                    no_noise_flag,
                );
            }
        }

        if is_ites_enable {
            if let Some(gi) = gamma_index {
                Self::apply_inter_tes(
                    ana_buffer_real,
                    ana_buffer_imag,
                    low_subband_usize,
                    num_subbands_usize,
                    gi,
                );

                // Add additional harmonics.
                for (buff_real, buff_imag) in
                    izip!(ana_buffer_real.iter_mut(), ana_buffer_imag.iter_mut(),)
                {
                    self.adjust_time_slot_hq_add_harmonics(
                        env_calc_nrgs,
                        &mut buff_real[low_subband_usize..low_subband_usize + num_subbands_usize],
                        &mut buff_imag[low_subband_usize..low_subband_usize + num_subbands_usize],
                        low_subband,
                    );
                }
            }
        }

        // Update time-smoothing-buffers for gains and noise levels
        // The gains and the noise values of the current envelope are copied into
        // the buffer. This has to be done at the end of each envelope as the
        // values are required for a smooth transition to the next envelope.
        self.filt_buffer_noise[..num_subbands_usize]
            .copy_from_slice(&env_calc_nrgs.noise_level[..num_subbands_usize]);
        self.filt_buffer[..num_subbands_usize]
            .copy_from_slice(&env_calc_nrgs.nrg_gain[..num_subbands_usize]);
    }

    /// Estimates the mean energy of each Scale factor band for the duration of the current
    /// envelope.
    fn calc_nrg_per_sfb(
        ana_buffer_real: &[&[f32]],
        ana_buffer_imag: &[&[f32]],
        nrg_est: &mut [f32],
        freq_band_table: &[u8],
        num_sfb: usize,
    ) {
        let width = ana_buffer_real.len() as f32;
        let mut nrg_est_idx = 0;

        for indices in freq_band_table.windows(2).take(num_sfb) {
            let li = usize::from(indices[0]);
            let ui = usize::from(indices[1]);
            let mut sum_all = 0.0f32;
            for (real_slot, imag_slot) in izip!(ana_buffer_real.iter(), ana_buffer_imag.iter(),) {
                let mut sum_col = 0.0f32;
                for (&real, &imag) in
                    izip!(real_slot[li..].iter(), imag_slot[li..].iter()).take(ui - li)
                {
                    sum_col += real * real;
                    sum_col += imag * imag;
                }
                sum_all += sum_col;
            }

            // Divide by width of envelope.
            let mut sum = sum_all / width;

            // Divide by width of scale factor band.
            sum /= (ui - li) as f32;

            nrg_est[nrg_est_idx..(nrg_est_idx + ui - li)].fill(sum);
            nrg_est_idx += ui - li;
        }
    }

    /// Estimates the mean energy of each subband for the duration of the current envelope.
    fn calc_nrg_per_subband(
        ana_buffer_real: &[&[f32]],
        ana_buffer_imag: &[&[f32]],
        nrg_est: &mut [f32],
        low_subband: usize,
        high_subband: usize,
    ) {
        let mut acc_buff =
            [0.0_f64; (QMF_DOMAIN_MAX_OV_TIMESLOTS + QMF_DOMAIN_MAX_TIMESLOTS) as usize];
        let width = ana_buffer_real.len() as f32;

        for (real_slot, imag_slot) in izip!(ana_buffer_real.iter(), ana_buffer_imag.iter(),) {
            for (acc, &real, &imag) in izip!(
                acc_buff.iter_mut(),
                real_slot[low_subband..].iter(),
                imag_slot[low_subband..].iter()
            )
            .take(high_subband - low_subband)
            {
                *acc += (real * real + imag * imag) as f64;
            }
        }

        for (nrg_val, acc_val) in
            izip!(nrg_est.iter_mut(), acc_buff.iter()).take(high_subband - low_subband)
        {
            *nrg_val = *acc_val as f32 / width;
        }
    }

    /// Limits noise level for each limiter band. Calculates and applies boost factor for each
    /// limiter band.
    fn compress_signal(
        &self,
        env_calc_nrgs: &mut EnvCalcNrgs,
        limiter_gain_index: usize,
        frame_error_flag: bool,
        no_noise_flag: bool,
    ) {
        for limit_band_table in self
            .limiter_band_table
            .windows(2)
            .take(usize::from(self.num_limiter_bands))
        {
            let low_subband = usize::from(limit_band_table[0]);
            let high_subband = usize::from(limit_band_table[1]);
            let mut accu = 0.0f32;

            let (sum_ref, avg_gain) = env_calc_nrgs.calc_avg_gain(low_subband, high_subband);

            let gain_limit = if frame_error_flag {
                GAIN_CONCEAL_LIMITATION
            } else {
                1.0e10f32
            };

            let max_gain = if avg_gain > (gain_limit / LIM_GAINS[limiter_gain_index]) {
                gain_limit
            } else {
                avg_gain * LIM_GAINS[limiter_gain_index]
            };

            // Noise limiting.
            for (nl, gain) in izip!(
                env_calc_nrgs.noise_level[low_subband..high_subband].iter_mut(),
                env_calc_nrgs.nrg_gain[low_subband..high_subband].iter_mut()
            ) {
                if *gain > max_gain {
                    *nl = *nl * max_gain / *gain;
                    *gain = max_gain;
                }
            }

            // -- Boost gain
            // Calculate and apply boost factor for each limiter band:
            // 1. Check how much energy would be present when using the limited gain.
            // 2. Calculate boost factor by comparison with reference energy.
            // 3. Apply boost factor to compensate for the energy loss due to limiting.

            for (&est, &gain, &sine, &nl) in izip!(
                env_calc_nrgs.nrg_est[low_subband..high_subband].iter(),
                env_calc_nrgs.nrg_gain[low_subband..high_subband].iter(),
                env_calc_nrgs.nrg_sine[low_subband..high_subband].iter(),
                env_calc_nrgs.noise_level[low_subband..high_subband].iter()
            ) {
                // 1.a  Add energy of adjusted signal (using preliminary gain).
                let tmp = est * gain;
                accu += tmp;

                // 1.b  Add sine energy (if present).
                if sine != 0.0f32 {
                    accu += sine;
                } else {
                    // 1.c  Add noise energy (if present).
                    if !no_noise_flag {
                        accu += nl;
                    }
                }
            }

            // 2.a  Calculate ratio of wanted energy and accumulated energy.
            let boost_gain = if accu == 0.0f32 {
                BOOST_GAIN_LIMIT
            } else {
                (sum_ref / accu).min(BOOST_GAIN_LIMIT)
            };

            // 3. Multiply all signal components with the boost factor.
            for (gain, sine, nl) in izip!(
                env_calc_nrgs.nrg_gain[low_subband..high_subband].iter_mut(),
                env_calc_nrgs.nrg_sine[low_subband..high_subband].iter_mut(),
                env_calc_nrgs.noise_level[low_subband..high_subband].iter_mut()
            ) {
                *gain *= boost_gain;
                *sine *= boost_gain;
                *nl *= boost_gain;
            }
        }
    }

    /// Extracts sine flags for all QMF bands.
    fn extract_sine_flags(
        &mut self,
        frame_data: &FrameData,
        freq_band_table_high: &[u8],
        sine_mapped: &mut [i8],
        num_sfb: u8,
        is_usac_syntax_with_pvc: bool,
    ) {
        let fi = &frame_data.frame_info;
        let num_subbands = usize::from(num_sfb);
        if is_usac_syntax_with_pvc {
            let is_trailing_sbr_frame = fi.borders()[0] > fi.pvc_borders()[0];
            self.map_sine_flags_pvc(
                usize::from(freq_band_table_high[0]),
                usize::from(freq_band_table_high[num_subbands]),
                sine_mapped,
                frame_data.sinusoidal_position(),
                is_trailing_sbr_frame,
            );
        } else {
            self.map_sine_flags(
                freq_band_table_high,
                &frame_data.add_harmonics()[..],
                sine_mapped,
                num_subbands,
                fi.tran_env(),
            );
        }
    }

    /// Maps sine flags from bitstream to QMF bands.
    ///
    /// The bitstream carries only 1 sine flag per band (sfb) and frame.This function maps every
    /// sine flag from the bitstream to a specific QMF subband and to a specific envelope where the
    /// sine shall start. The result is stored in the buffer `sine_mapped[]` which contains one
    /// entry per QMF subband. The value of an entry specifies the envelope where a sine shall
    /// start. A value of 32 indicates that no sine is present in the subband. The missing
    /// harmonics flags from the previous frame (`self.harm_flags_prev[]`) determine if a sine
    /// starts at the beginning of the frame or at the transient position. Additionally, the
    /// flags in `self.harm_flags_prev[]` are being updated by this function for the next frame.
    fn map_sine_flags(
        &mut self,
        freq_band_table: &[u8],
        add_harmonics: &[u32],
        sine_mapped: &mut [i8],
        num_sfb: usize,
        tran_env: i8,
    ) {
        let mut harm_flags_qmf_bands = [0_u32; ADD_HARMONICS_FLAGS_SIZE];
        let mut bit_count = 31_u32;
        let mut cur_flags = add_harmonics[0];
        // Format of add_harmonics (aligned to MSB):
        //
        // Up to MAX_FREQ_COEFFS sfb bands can be flagged for a sign.
        // first word  = flags for lowest 32 sfb bands in use
        // second word = flags for higest 32 sfb bands (if present)
        //
        // Format of harm_flags_prev (aligned to LSB):
        //
        // Index is absolute (not relative to lsb) so it is correct even if lsb
        // changes first word  = flags for lowest 32 qmf bands (0...31) second word =
        // flags for next higher 32 qmf bands (32...63)

        // Reset the output vector first. `32` means "no sine".
        sine_mapped.fill(32);
        self.harm_flags_prev_active.fill(0);

        // start of sbr range.
        let lsb = usize::from(freq_band_table[0]);

        for fbt in freq_band_table.windows(2).take(num_sfb) {
            // Mask to extract the add_harmonics flag of the  current sfb.
            let mask_sfb = 1 << bit_count;

            // There is a sine in this band.
            if cur_flags & mask_sfb != 0 {
                // qmf band to which sine should be added.
                let qmf_band = usize::from(fbt[0] + fbt[1]) >> 1;
                let qmf_band_div32 = qmf_band >> 5;

                // mask to extract harmonic flag from prev_flags.
                let mask_qmf_band = 1 << (qmf_band & 31) as u32;

                // mapping of sfb with sine to a certain qmf band -> for harm_flags_prev.
                harm_flags_qmf_bands[qmf_band_div32] |= mask_qmf_band;

                // If there was a sine in the last frame, let it continue from the first
                // envelope on else start at the transient position. Indexing of sine_mapped
                // starts relative to lsb.
                sine_mapped[qmf_band - lsb] =
                    if self.harm_flags_prev[qmf_band_div32] & mask_qmf_band != 0 {
                        0
                    } else {
                        tran_env
                    };

                if sine_mapped[qmf_band - lsb] < PVC_NTIMESLOT as i8 {
                    self.harm_flags_prev_active[qmf_band_div32] |= mask_qmf_band;
                }
            }

            if bit_count == 0 {
                bit_count = 31;
                cur_flags = add_harmonics[1];
            } else {
                bit_count -= 1;
            }
        }

        self.harm_flags_prev.copy_from_slice(&harm_flags_qmf_bands);
    }

    /// Restores `sine_mapped[]` of previous frame.
    ///
    /// For PVC it might happen that the PVC framing (always FixFix) is out of sync with the SBR
    /// framing. The adding of additional harmonics is done based on the SBR framing. If the SBR
    /// framing is trailing the PVC framing the sine mapping of the previous SBR frame needs to be
    /// used for the overlapping time slots.
    fn map_sine_flags_pvc(
        &mut self,
        low_subband: usize,
        high_subband: usize,
        sine_mapped: &mut [i8],
        sinusoidal_pos: u8,
        is_trailing_sbr_frame: bool,
    ) {
        // Reset the output vector first. `32` means "no sine".
        sine_mapped.fill(32);
        if is_trailing_sbr_frame {
            let sinusoidal_pos_prev = self.sinusoidal_position_prev;
            for (band, sine_map) in izip!(low_subband..high_subband, sine_mapped.iter_mut())
                .take(high_subband - low_subband)
            {
                let qmf_band_div32 = band >> 5;
                // mask to extract harmonic flag from prev_flags.
                let mask_qmf_band = 1 << (band & 31);

                // Two cases need to be distinguished...
                if self.harm_flags_prev_active[qmf_band_div32] & mask_qmf_band != 0 {
                    // The sine mapping already started last PVC frame -> seamlessly continue.
                    *sine_map = 0;
                } else if self.harm_flags_prev[qmf_band_div32] & mask_qmf_band != 0 {
                    // sinusoidal_pos of prev PVC frame was >= PVC_NTIMESLOT -> sine starts in this
                    // frame. we are 16 sbr time slots ahead of last frame now.
                    *sine_map = sinusoidal_pos_prev - PVC_NTIMESLOT as i8;
                }
            }
        }
        // Note: Max value of sinusoidal_pos can be  31 (5 bits) from bitstream data.
        self.sinusoidal_position_prev = sinusoidal_pos as i8;
    }
}

/// Strcuture that holds buffers for inter-TES processing.
#[repr(C)]
#[derive(Debug)]
struct ItesTmp {
    subsample_power_low: [f32; MAX_ENV_COLS],
    subsample_power_high: [f32; MAX_ENV_COLS],
    gain: [f32; MAX_ENV_COLS],
}

impl Default for ItesTmp {
    fn default() -> Self {
        ItesTmp {
            subsample_power_low: [0.0f32; MAX_ENV_COLS],
            subsample_power_high: [0.0f32; MAX_ENV_COLS],
            gain: [0.0f32; MAX_ENV_COLS],
        }
    }
}

/// Structure that holds buffers for envelope energy calculation.
#[repr(C)]
#[derive(Debug)]
struct EnvCalcNrgs {
    nrg_ref: [f32; MAX_FREQ_COEFFS],
    nrg_est: [f32; MAX_FREQ_COEFFS],
    nrg_gain: [f32; MAX_FREQ_COEFFS],
    noise_level: [f32; MAX_FREQ_COEFFS],
    nrg_sine: [f32; MAX_FREQ_COEFFS],
}

impl Default for EnvCalcNrgs {
    fn default() -> Self {
        EnvCalcNrgs {
            nrg_ref: [0.0f32; MAX_FREQ_COEFFS],
            nrg_est: [0.0f32; MAX_FREQ_COEFFS],
            nrg_gain: [0.0f32; MAX_FREQ_COEFFS],
            noise_level: [0.0f32; MAX_FREQ_COEFFS],
            nrg_sine: [0.0f32; MAX_FREQ_COEFFS],
        }
    }
}

impl EnvCalcNrgs {
    /// Calculates gain, noise, and additional sine level for one subband.
    fn calc_subband_gain(
        &mut self,
        nrg_ref: f32,
        freq_coeff_index: usize,
        tmp_noise: f32,
        sine_present_flag: bool,
        sine_mapped_flag: bool,
        no_noise_flag: bool,
    ) {
        let nrg_est = self.nrg_est[freq_coeff_index] + 1.0f32;
        let a = nrg_ref * tmp_noise;
        let b = 1.0f32 + tmp_noise;

        self.noise_level[freq_coeff_index] = a / b;

        self.nrg_gain[freq_coeff_index] = if sine_present_flag {
            if sine_mapped_flag {
                self.nrg_sine[freq_coeff_index] = nrg_ref / b;
            }
            a / (b * nrg_est)
        } else if no_noise_flag {
            nrg_ref / nrg_est
        } else {
            nrg_ref / (b * nrg_est)
        };
    }

    /// Calculates "average gain" for the specified subband range.
    ///
    /// This is rather a gain of the average magnitude than the average of gains!. The result is
    /// used as a relative limit for all gains within the current "limiter band" (a certain
    /// frequency range).
    fn calc_avg_gain(&self, low_subband: usize, high_subband: usize) -> (f32, f32) {
        let mut sum_ref = 0.0f32;
        let mut sum_est = 0.0f32;

        for (&nrg_ref, &nrg_est) in izip!(
            self.nrg_ref[low_subband..high_subband].iter(),
            self.nrg_est[low_subband..high_subband].iter(),
        ) {
            sum_ref += nrg_ref;
            sum_est += nrg_est;
        }

        let avg_gain = sum_ref / (sum_est + f32::EPSILON);

        (sum_ref, avg_gain)
    }

    /// Converts energy values to amplitude levels.
    fn convert_nrgs_to_amp_level(&mut self, num_subbands: usize) {
        // Convert energies to amplitude levels.
        for (gain, sine, nl) in izip!(
            self.nrg_gain.iter_mut(),
            self.nrg_sine.iter_mut(),
            self.noise_level.iter_mut()
        )
        .take(num_subbands)
        {
            *gain = (*gain).sqrt();
            *sine = (*sine).sqrt();
            *nl = (*nl).sqrt();
        }
    }
}

// Constants for calculating the number of limiter bands.
static LIMITER_BANDS_PER_OCTAVE: [f32; 4] = [1.0, 1.2, 2.0, 3.0];

// Ratio of old gains and noise levels for the first 4 timeslots of an envelope.
static SMOOTH_FILTER: [f32; 4] = [
    0.66666666666666,
    0.36516383427084,
    0.14699433520835,
    0.03183050093751,
];

static LIM_GAINS: [f32; 4] = [
    0.5011932025, /* -3 dB. Gain limit when limiterGains in frameData is 0 */
    1.0,          /* 0 dB.  Gain limit when limiterGains in frameData is 1 */
    1.9952692516, /* +3 dB. Gain limit when limiterGains in frameData is 2 */
    1.0e20,       /* Inf.   Gain limit when limiterGains in frameData is 3 */
];

#[rustfmt::skip]
static RANDOM_PHASE: [Complex<f32>; N_NF_RND_VALS] = [
    Complex { re: -0.99948153278296, im: -0.59483417516607 }, Complex { re: 0.97113454393991, im: -0.67528515225647  },
    Complex { re: 0.14130051758487, im: -0.95090983575689  }, Complex { re: -0.47005496701697, im: -0.37340549728647 },
    Complex { re: 0.80705063769351, im: 0.29653668284408   }, Complex { re: -0.38981478896926, im: 0.89572605717087  },
    Complex { re: -0.01053049862020, im: -0.66959058036166 }, Complex { re: -0.91266367957293, im: -0.11522938140034 },
    Complex { re: 0.54840422910309, im: 0.75221367176302   }, Complex { re: 0.40009252867955, im: -0.98929400334421  },
    Complex { re: -0.99867974711855, im: -0.88147068645358 }, Complex { re: -0.95531076805040, im: 0.90908757154593  },
    Complex { re: -0.45725933317144, im: -0.56716323646760 }, Complex { re: -0.72929675029275, im: -0.98008272727324 },
    Complex { re: 0.75622801399036, im: 0.20950329995549   }, Complex { re: 0.07069442601050, im: -0.78247898470706  },
    Complex { re: 0.74496252926055, im: -0.91169004445807  }, Complex { re: -0.96440182703856, im: -0.94739918296622 },
    Complex { re: 0.30424629369539, im: -0.49438267012479  }, Complex { re: 0.66565033746925, im: 0.64652935542491   },
    Complex { re: 0.91697008020594, im: 0.17514097332009   }, Complex { re: -0.70774918760427, im: 0.52548653416543  },
    Complex { re: -0.70051415345560, im: -0.45340028808763 }, Complex { re: -0.99496513054797, im: -0.90071908066973 },
    Complex { re: 0.98164490790123, im: -0.77463155528697  }, Complex { re: -0.54671580548181, im: -0.02570928536004 },
    Complex { re: -0.01689629065389, im: 0.00287506445732  }, Complex { re: -0.86110349531986, im: 0.42548583726477  },
    Complex { re: -0.98892980586032, im: -0.87881132267556 }, Complex { re: 0.51756627678691, im: 0.66926784710139   },
    Complex { re: -0.99635026409640, im: -0.58107730574765 }, Complex { re: -0.99969370862163, im: 0.98369989360250  },
    Complex { re: 0.55266258627194, im: 0.59449057465591   }, Complex { re: 0.34581177741673, im: 0.94879421061866   },
    Complex { re: 0.62664209577999, im: -0.74402970906471  }, Complex { re: -0.77149701404973, im: -0.33883658042801 },
    Complex { re: -0.91592244254432, im: 0.03687901376713  }, Complex { re: -0.76285492357887, im: -0.91371867919124 },
    Complex { re: 0.79788337195331, im: -0.93180971199849  }, Complex { re: 0.54473080610200, im: -0.11919206037186  },
    Complex { re: -0.85639281671058, im: 0.42429854760451  }, Complex { re: -0.92882402971423, im: 0.27871809078609  },
    Complex { re: -0.11708371046774, im: -0.99800843444966 }, Complex { re: 0.21356749817493, im: -0.90716295627033  },
    Complex { re: -0.76191692573909, im: 0.99768118356265  }, Complex { re: 0.98111043100884, im: -0.95854459734407  },
    Complex { re: -0.85913269895572, im: 0.95766566168880  }, Complex { re: -0.93307242253692, im: 0.49431757696466  },
    Complex { re: 0.30485754879632, im: -0.70540034357529  }, Complex { re: 0.85289650925190, im: 0.46766131791044   },
    Complex { re: 0.91328082618125, im: -0.99839597361769  }, Complex { re: -0.05890199924154, im: 0.70741827819497  },
    Complex { re: 0.28398686150148, im: 0.34633555702188   }, Complex { re: 0.95258164539612, im: -0.54893416026939  },
    Complex { re: -0.78566324168507, im: -0.75568541079691 }, Complex { re: -0.95789495447877, im: -0.20423194696966 },
    Complex { re: 0.82411158711197, im: 0.96654618432562   }, Complex { re: -0.65185446735885, im: -0.88734990773289 },
    Complex { re: -0.93643603134666, im: 0.99870790442385  }, Complex { re: 0.91427159529618, im: -0.98290505544444  },
    Complex { re: -0.70395684036886, im: 0.58796798221039  }, Complex { re: 0.00563771969365, im: 0.61768196727244   },
    Complex { re: 0.89065051931895, im: 0.52783352697585   }, Complex { re: -0.68683707712762, im: 0.80806944710339  },
    Complex { re: 0.72165342518718, im: -0.69259857349564  }, Complex { re: -0.62928247730667, im: 0.13627037407335  },
    Complex { re: 0.29938434065514, im: -0.46051329682246  }, Complex { re: -0.91781958879280, im: -0.74012716684186 },
    Complex { re: 0.99298717043688, im: 0.40816610075661   }, Complex { re: 0.82368298622748, im: -0.74036047190173  },
    Complex { re: -0.98512833386833, im: -0.99972330709594 }, Complex { re: -0.95915368242257, im: -0.99237800466040 },
    Complex { re: -0.21411126572790, im: -0.93424819052545 }, Complex { re: -0.68821476106884, im: -0.26892306315457 },
    Complex { re: 0.91851997982317, im: 0.09358228901785   }, Complex { re: -0.96062769559127, im: 0.36099095133739  },
    Complex { re: 0.51646184922287, im: -0.71373332873917  }, Complex { re: 0.61130721139669, im: 0.46950141175917   },
    Complex { re: 0.47336129371299, im: -0.27333178296162  }, Complex { re: 0.90998308703519, im: 0.96715662938132   },
    Complex { re: 0.44844799194357, im: 0.99211574628306   }, Complex { re: 0.66614891079092, im: 0.96590176169121   },
    Complex { re: 0.74922239129237, im: -0.89879858826087  }, Complex { re: -0.99571588506485, im: 0.52785521494349  },
    Complex { re: 0.97401082477563, im: -0.16855870075190  }, Complex { re: 0.72683747733879, im: -0.48060774432251  },
    Complex { re: 0.95432193457128, im: 0.68849603408441   }, Complex { re: -0.72962208425191, im: -0.76608443420917 },
    Complex { re: -0.85359479233537, im: 0.88738125901579  }, Complex { re: -0.81412430338535, im: -0.97480768049637 },
    Complex { re: -0.87930772356786, im: 0.74748307690436  }, Complex { re: -0.71573331064977, im: -0.98570608178923 },
    Complex { re: 0.83524300028228, im: 0.83702537075163   }, Complex { re: -0.48086065601423, im: -0.98848504923531 },
    Complex { re: 0.97139128574778, im: 0.80093621198236   }, Complex { re: 0.51992825347895, im: 0.80247631400510   },
    Complex { re: -0.00848591195325, im: -0.76670128000486 }, Complex { re: -0.70294374303036, im: 0.55359910445577  },
    Complex { re: -0.95894428168140, im: -0.43265504344783 }, Complex { re: 0.97079252950321, im: 0.09325857238682   },
    Complex { re: -0.92404293670797, im: 0.85507704027855  }, Complex { re: -0.69506469500450, im: 0.98633412625459  },
    Complex { re: 0.26559203620024, im: 0.73314307966524   }, Complex { re: 0.28038443336943, im: 0.14537913654427   },
    Complex { re: -0.74138124825523, im: 0.99310339807762  }, Complex { re: -0.01752795995444, im: -0.82616635284178 },
    Complex { re: -0.55126773094930, im: -0.98898543862153 }, Complex { re: 0.97960898850996, im: -0.94021446752851  },
    Complex { re: -0.99196309146936, im: 0.67019017358456  }, Complex { re: -0.67684928085260, im: 0.12631491649378  },
    Complex { re: 0.09140039465500, im: -0.20537731453108  }, Complex { re: -0.71658965751996, im: -0.97788200391224 },
    Complex { re: 0.81014640078925, im: 0.53722648362443   }, Complex { re: 0.40616991671205, im: -0.26469008598449  },
    Complex { re: -0.67680188682972, im: 0.94502052337695  }, Complex { re: 0.86849774348749, im: -0.18333598647899  },
    Complex { re: -0.99500381284851, im: -0.02634122068550 }, Complex { re: 0.84329189340667, im: 0.10406957462213   },
    Complex { re: -0.09215968531446, im: 0.69540012101253  }, Complex { re: 0.99956173327206, im: -0.12358542001404  },
    Complex { re: -0.79732779473535, im: -0.91582524736159 }, Complex { re: 0.96349973642406, im: 0.96640458041000   },
    Complex { re: -0.79942778496547, im: 0.64323902822857  }, Complex { re: -0.11566039853896, im: 0.28587846253726  },
    Complex { re: -0.39922954514662, im: 0.94129601616966  }, Complex { re: 0.99089197565987, im: -0.92062625581587  },
    Complex { re: 0.28631285179909, im: -0.91035047143603  }, Complex { re: -0.83302725605608, im: -0.67330410892084 },
    Complex { re: 0.95404443402072, im: 0.49162765398743   }, Complex { re: -0.06449863579434, im: 0.03250560813135  },
    Complex { re: -0.99575054486311, im: 0.42389784469507  }, Complex { re: -0.65501142790847, im: 0.82546114655624  },
    Complex { re: -0.81254441908887, im: -0.51627234660629 }, Complex { re: -0.99646369485481, im: 0.84490533520752  },
    Complex { re: 0.00287840603348, im: 0.64768261158166   }, Complex { re: 0.70176989408455, im: -0.20453028573322  },
    Complex { re: 0.96361882270190, im: 0.40706967140989   }, Complex { re: -0.68883758192426, im: 0.91338958840772  },
    Complex { re: -0.34875585502238, im: 0.71472290693300  }, Complex { re: 0.91980081243087, im: 0.66507455644919   },
    Complex { re: -0.99009048343881, im: 0.85868021604848  }, Complex { re: 0.68865791458395, im: 0.55660316809678   },
    Complex { re: -0.99484402129368, im: -0.20052559254934 }, Complex { re: 0.94214511408023, im: -0.99696425367461  },
    Complex { re: -0.67414626793544, im: 0.49548221180078  }, Complex { re: -0.47339353684664, im: -0.85904328834047 },
    Complex { re: 0.14323651387360, im: -0.94145598222488  }, Complex { re: -0.29268293575672, im: 0.05759224927952  },
    Complex { re: 0.43793861458754, im: -0.78904969892724  }, Complex { re: -0.36345126374441, im: 0.64874435357162  },
    Complex { re: -0.08750604656825, im: 0.97686944362527  }, Complex { re: -0.96495267812511, im: -0.53960305946511 },
    Complex { re: 0.55526940659947, im: 0.78891523734774   }, Complex { re: 0.73538215752630, im: 0.96452072373404   },
    Complex { re: -0.30889773919437, im: -0.80664389776860 }, Complex { re: 0.03574995626194, im: -0.97325616900959  },
    Complex { re: 0.98720684660488, im: 0.48409133691962   }, Complex { re: -0.81689296271203, im: -0.90827703628298 },
    Complex { re: 0.67866860118215, im: 0.81284503870856   }, Complex { re: -0.15808569732583, im: 0.85279555024382  },
    Complex { re: 0.80723395114371, im: -0.24717418514605  }, Complex { re: 0.47788757329038, im: -0.46333147839295  },
    Complex { re: 0.96367554763201, im: 0.38486749303242   }, Complex { re: -0.99143875716818, im: -0.24945277239809 },
    Complex { re: 0.83081876925833, im: -0.94780851414763  }, Complex { re: -0.58753191905341, im: 0.01290772389163  },
    Complex { re: 0.95538108220960, im: -0.85557052096538  }, Complex { re: -0.96490920476211, im: -0.64020970923102 },
    Complex { re: -0.97327101028521, im: 0.12378128133110  }, Complex { re: 0.91400366022124, im: 0.57972471346930   },
    Complex { re: -0.99925837363824, im: 0.71084847864067  }, Complex { re: -0.86875903507313, im: -0.20291699203564 },
    Complex { re: -0.26240034795124, im: -0.68264554369108 }, Complex { re: -0.24664412953388, im: -0.87642273115183 },
    Complex { re: 0.02416275806869, im: 0.27192914288905   }, Complex { re: 0.82068619590515, im: -0.85087787994476  },
    Complex { re: 0.88547373760759, im: -0.89636802901469  }, Complex { re: -0.18173078152226, im: -0.26152145156800 },
    Complex { re: 0.09355476558534, im: 0.54845123045604   }, Complex { re: -0.54668414224090, im: 0.95980774020221  },
    Complex { re: 0.37050990604091, im: -0.59910140383171  }, Complex { re: -0.70373594262891, im: 0.91227665827081  },
    Complex { re: -0.34600785879594, im: -0.99441426144200 }, Complex { re: -0.68774481731008, im: -0.30238837956299 },
    Complex { re: -0.26843291251234, im: 0.83115668004362  }, Complex { re: 0.49072334613242, im: -0.45359708737775  },
    Complex { re: 0.38975993093975, im: 0.95515358099121   }, Complex { re: -0.97757125224150, im: 0.05305894580606  },
    Complex { re: -0.17325552859616, im: -0.92770672250494 }, Complex { re: 0.99948035025744, im: 0.58285545563426   },
    Complex { re: -0.64946246527458, im: 0.68645507104960  }, Complex { re: -0.12016920576437, im: -0.57147322153312 },
    Complex { re: -0.58947456517751, im: -0.34847132454388 }, Complex { re: -0.41815140454465, im: 0.16276422358861  },
    Complex { re: 0.99885650204884, im: 0.11136095490444   }, Complex { re: -0.56649614128386, im: -0.90494866361587 },
    Complex { re: 0.94138021032330, im: 0.35281916733018   }, Complex { re: -0.75725076534641, im: 0.53650549640587  },
    Complex { re: 0.20541973692630, im: -0.94435144369918  }, Complex { re: 0.99980371023351, im: 0.79835913565599   },
    Complex { re: 0.29078277605775, im: 0.35393777921520   }, Complex { re: -0.62858772103030, im: 0.38765693387102  },
    Complex { re: 0.43440904467688, im: -0.98546330463232  }, Complex { re: -0.98298583762390, im: 0.21021524625209  },
    Complex { re: 0.19513029146934, im: -0.94239832251867  }, Complex { re: -0.95476662400101, im: 0.98364554179143  },
    Complex { re: 0.93379635304810, im: -0.70881994583682  }, Complex { re: -0.85235410573336, im: -0.08342347966410 },
    Complex { re: -0.86425093011245, im: -0.45795025029466 }, Complex { re: 0.38879779059045, im: 0.97274429344593   },
    Complex { re: 0.92045124735495, im: -0.62433652524220  }, Complex { re: 0.89162532251878, im: 0.54950955570563   },
    Complex { re: -0.36834336949252, im: 0.96458298020975  }, Complex { re: 0.93891760988045, im: -0.89968353740388  },
    Complex { re: 0.99267657565094, im: -0.03757034316958  }, Complex { re: -0.94063471614176, im: 0.41332338538963  },
    Complex { re: 0.99740224117019, im: -0.16830494996370  }, Complex { re: -0.35899413170555, im: -0.46633226649613 },
    Complex { re: 0.05237237274947, im: -0.25640361602661  }, Complex { re: 0.36703583957424, im: -0.38653265641875  },
    Complex { re: 0.91653180367913, im: -0.30587628726597  }, Complex { re: 0.69000803499316, im: 0.90952171386132   },
    Complex { re: -0.38658751133527, im: 0.99501571208985  }, Complex { re: -0.29250814029851, im: 0.37444994344615  },
    Complex { re: -0.60182204677608, im: 0.86779651036123  }, Complex { re: -0.97418588163217, im: 0.96468523666475  },
    Complex { re: 0.88461574003963, im: 0.57508405276414   }, Complex { re: 0.05198933055162, im: 0.21269661669964   },
    Complex { re: -0.53499621979720, im: 0.97241553731237  }, Complex { re: -0.49429560226497, im: 0.98183865291903  },
    Complex { re: -0.98935142339139, im: -0.40249159006933 }, Complex { re: -0.98081380091130, im: -0.72856895534041 },
    Complex { re: -0.27338148835532, im: 0.99950922447209  }, Complex { re: 0.06310802338302, im: -0.54539587529618  },
    Complex { re: -0.20461677199539, im: -0.14209977628489 }, Complex { re: 0.66223843141647, im: 0.72528579940326   },
    Complex { re: -0.84764345483665, im: 0.02372316801261  }, Complex { re: -0.89039863483811, im: 0.88866581484602  },
    Complex { re: 0.95903308477986, im: 0.76744927173873   }, Complex { re: 0.73504123909879, im: -0.03747203173192  },
    Complex { re: -0.31744434966056, im: -0.36834111883652 }, Complex { re: -0.34110827591623, im: 0.40211222807691  },
    Complex { re: 0.47803883714199, im: -0.39423219786288  }, Complex { re: 0.98299195879514, im: 0.01989791390047   },
    Complex { re: -0.30963073129751, im: -0.18076720599336 }, Complex { re: 0.99992588229018, im: -0.26281872094289  },
    Complex { re: -0.93149731080767, im: -0.98313162570490 }, Complex { re: 0.99923472302773, im: -0.80142993767554  },
    Complex { re: -0.26024169633417, im: -0.75999759855752 }, Complex { re: -0.35712514743563, im: 0.19298963768574  },
    Complex { re: -0.99899084509530, im: 0.74645156992493  }, Complex { re: 0.86557171579452, im: 0.55593866696299   },
    Complex { re: 0.33408042438752, im: 0.86185953874709   }, Complex { re: 0.99010736374716, im: 0.04602397576623   },
    Complex { re: -0.66694269691195, im: -0.91643611810148 }, Complex { re: 0.64016792079480, im: 0.15649530836856   },
    Complex { re: 0.99570534804836, im: 0.45844586038111   }, Complex { re: -0.63431466947340, im: 0.21079116459234  },
    Complex { re: -0.07706847005931, im: -0.89581437101329 }, Complex { re: 0.98590090577724, im: 0.88241721133981   },
    Complex { re: 0.80099335254678, im: -0.36851896710853  }, Complex { re: 0.78368131392666, im: 0.45506999802597   },
    Complex { re: 0.08707806671691, im: 0.80938994918745   }, Complex { re: -0.86811883080712, im: 0.39347308654705  },
    Complex { re: -0.39466529740375, im: -0.66809432114456 }, Complex { re: 0.97875325649683, im: -0.72467840967746  },
    Complex { re: -0.95038560288864, im: 0.89563219587625  }, Complex { re: 0.17005239424212, im: 0.54683053962658   },
    Complex { re: -0.76910792026848, im: -0.96226617549298 }, Complex { re: 0.99743281016846, im: 0.42697157037567   },
    Complex { re: 0.95437383549973, im: 0.97002324109952   }, Complex { re: 0.99578905365569, im: -0.54106826257356  },
    Complex { re: 0.28058259829990, im: -0.85361420634036  }, Complex { re: 0.85256524470573, im: -0.64567607735589  },
    Complex { re: -0.50608540105128, im: -0.65846015480300 }, Complex { re: -0.97210735183243, im: -0.23095213067791 },
    Complex { re: 0.95424048234441, im: -0.99240147091219  }, Complex { re: -0.96926570524023, im: 0.73775654896574  },
    Complex { re: 0.30872163214726, im: 0.41514960556126   }, Complex { re: -0.24523839572639, im: 0.63206633394807  },
    Complex { re: -0.33813265086024, im: -0.38661779441897 }, Complex { re: -0.05826828420146, im: -0.06940774188029 },
    Complex { re: -0.22898461455054, im: 0.97054853316316  }, Complex { re: -0.18509915019881, im: 0.47565762892084  },
    Complex { re: -0.10488238045009, im: -0.87769947402394 }, Complex { re: -0.71886586182037, im: 0.78030982480538  },
    Complex { re: 0.99793873738654, im: 0.90041310491497   }, Complex { re: 0.57563307626120, im: -0.91034337352097  },
    Complex { re: 0.28909646383717, im: 0.96307783970534   }, Complex { re: 0.42188998312520, im: 0.48148651230437   },
    Complex { re: 0.93335049681047, im: -0.43537023883588  }, Complex { re: -0.97087374418267, im: 0.86636445711364  },
    Complex { re: 0.36722871286923, im: 0.65291654172961   }, Complex { re: -0.81093025665696, im: 0.08778370229363  },
    Complex { re: -0.26240603062237, im: -0.92774095379098 }, Complex { re: 0.83996497984604, im: 0.55839849139647   },
    Complex { re: -0.99909615720225, im: -0.96024605713970 }, Complex { re: 0.74649464155061, im: 0.12144893606462   },
    Complex { re: -0.74774595569805, im: -0.26898062008959 }, Complex { re: 0.95781667469567, im: -0.79047927052628  },
    Complex { re: 0.95472308713099, im: -0.08588776019550  }, Complex { re: 0.48708332746299, im: 0.99999041579432   },
    Complex { re: 0.46332038247497, im: 0.10964126185063   }, Complex { re: -0.76497004940162, im: 0.89210929242238  },
    Complex { re: 0.57397389364339, im: 0.35289703373760   }, Complex { re: 0.75374316974495, im: 0.96705214651335   },
    Complex { re: -0.59174397685714, im: -0.89405370422752 }, Complex { re: 0.75087906691890, im: -0.29612672982396  },
    Complex { re: -0.98607857336230, im: 0.25034911730023  }, Complex { re: -0.40761056640505, im: -0.90045573444695 },
    Complex { re: 0.66929266740477, im: 0.98629493401748   }, Complex { re: -0.97463695257310, im: -0.00190223301301 },
    Complex { re: 0.90145509409859, im: 0.99781390365446   }, Complex { re: -0.87259289048043, im: 0.99233587353666  },
    Complex { re: -0.91529461447692, im: -0.15698707534206 }, Complex { re: -0.03305738840705, im: -0.37205262859764 },
    Complex { re: 0.07223051368337, im: -0.88805001733626  }, Complex { re: 0.99498012188353, im: 0.97094358113387   },
    Complex { re: -0.74904939500519, im: 0.99985483641521  }, Complex { re: 0.04585228574211, im: 0.99812337444082   },
    Complex { re: -0.89054954257993, im: -0.31791913188064 }, Complex { re: -0.83782144651251, im: 0.97637632547466  },
    Complex { re: 0.33454804933804, im: -0.86231516800408  }, Complex { re: -0.99707579362824, im: 0.93237990079441  },
    Complex { re: -0.22827527843994, im: 0.18874759397997  }, Complex { re: 0.67248046289143, im: -0.03646211390569  },
    Complex { re: -0.05146538187944, im: -0.92599700120679 }, Complex { re: 0.99947295749905, im: 0.93625229707912   },
    Complex { re: 0.66951124390363, im: 0.98905825623893   }, Complex { re: -0.99602956559179, im: -0.44654715757688 },
    Complex { re: 0.82104905483590, im: 0.99540741724928   }, Complex { re: 0.99186510988782, im: 0.72023001312947   },
    Complex { re: -0.65284592392918, im: 0.52186723253637  }, Complex { re: 0.93885443798188, im: -0.74895312615259  },
    Complex { re: 0.96735248738388, im: 0.90891816978629   }, Complex { re: -0.22225968841114, im: 0.57124029781228  },
    Complex { re: -0.44132783753414, im: -0.92688840659280 }, Complex { re: -0.85694974219574, im: 0.88844532719844  },
    Complex { re: 0.91783042091762, im: -0.46356892383970  }, Complex { re: 0.72556974415690, im: -0.99899555770747  },
    Complex { re: -0.99711581834508, im: 0.58211560180426  }, Complex { re: 0.77638976371966, im: 0.94321834873819   },
    Complex { re: 0.07717324253925, im: 0.58638399856595   }, Complex { re: -0.56049829194163, im: 0.82522301569036  },
    Complex { re: 0.98398893639988, im: 0.39467440420569   }, Complex { re: 0.47546946844938, im: 0.68613044836811   },
    Complex { re: 0.65675089314631, im: 0.18331637134880   }, Complex { re: 0.03273375457980, im: -0.74933109564108  },
    Complex { re: -0.38684144784738, im: 0.51337349030406  }, Complex { re: -0.97346267944545, im: -0.96549364384098 },
    Complex { re: -0.53282156061942, im: -0.91423265091354 }, Complex { re: 0.99817310731176, im: 0.61133572482148   },
    Complex { re: -0.50254500772635, im: -0.88829338134294 }, Complex { re: 0.01995873238855, im: 0.85223515096765   },
    Complex { re: 0.99930381973804, im: 0.94578896296649   }, Complex { re: 0.82907767600783, im: -0.06323442598128  },
    Complex { re: -0.58660709669728, im: 0.96840773806582  }, Complex { re: -0.17573736667267, im: -0.48166920859485 },
    Complex { re: 0.83434292401346, im: -0.13023450646997  }, Complex { re: 0.05946491307025, im: 0.20511047074866   },
    Complex { re: 0.81505484574602, im: -0.94685947861369  }, Complex { re: -0.44976380954860, im: 0.40894572671545  },
    Complex { re: -0.89746474625671, im: 0.99846578838537  }, Complex { re: 0.39677256130792, im: -0.74854668609359  },
    Complex { re: -0.07588948563079, im: 0.74096214084170  }, Complex { re: 0.76343198951445, im: 0.41746629422634   },
    Complex { re: -0.74490104699626, im: 0.94725911744610  }, Complex { re: 0.64880119792759, im: 0.41336660830571   },
    Complex { re: 0.62319537462542, im: -0.93098313552599  }, Complex { re: 0.42215817594807, im: -0.07712787385208  },
    Complex { re: 0.02704554141885, im: -0.05417518053666  }, Complex { re: 0.80001773566818, im: 0.91542195141039   },
    Complex { re: -0.79351832348816, im: -0.36208897989136 }, Complex { re: 0.63872359151636, im: 0.08128252493444   },
    Complex { re: 0.52890520960295, im: 0.60048872455592   }, Complex { re: 0.74238552914587, im: 0.04491915291044   },
    Complex { re: 0.99096131449250, im: -0.19451182854402  }, Complex { re: -0.80412329643109, im: -0.88513818199457 },
    Complex { re: -0.64612616129736, im: 0.72198674804544  }, Complex { re: 0.11657770663191, im: -0.83662833815041  },
    Complex { re: -0.95053182488101, im: -0.96939905138082 }, Complex { re: -0.62228872928622, im: 0.82767262846661  },
    Complex { re: 0.03004475787316, im: -0.99738896333384  }, Complex { re: -0.97987214341034, im: 0.36526129686425  },
    Complex { re: -0.99986980746200, im: -0.36021610299715 }, Complex { re: 0.89110648599879, im: -0.97894250343044  },
    Complex { re: 0.10407960510582, im: 0.77357793811619   }, Complex { re: 0.95964737821728, im: -0.35435818285502  },
    Complex { re: 0.50843233159162, im: 0.96107691266205   }, Complex { re: 0.17006334670615, im: -0.76854025314829  },
    Complex { re: 0.25872675063360, im: 0.99893303933816   }, Complex { re: -0.01115998681937, im: 0.98496019742444  },
    Complex { re: -0.79598702973261, im: 0.97138411318894  }, Complex { re: -0.99264708948101, im: -0.99542822402536 },
    Complex { re: -0.99829663752818, im: 0.01877138824311  }, Complex { re: -0.70801016548184, im: 0.33680685948117  },
    Complex { re: -0.70467057786826, im: 0.93272777501857  }, Complex { re: 0.99846021905254, im: -0.98725746254433  },
    Complex { re: -0.63364968534650, im: -0.16473594423746 }, Complex { re: -0.16258217500792, im: -0.95939125400802 },
    Complex { re: -0.43645594360633, im: -0.94805030113284 }, Complex { re: -0.99848471702976, im: 0.96245166923809  },
    Complex { re: -0.16796458968998, im: -0.98987511890470 }, Complex { re: -0.87979225745213, im: -0.71725725041680 },
    Complex { re: 0.44183099021786, im: -0.93568974498761  }, Complex { re: 0.93310180125532, im: -0.99913308068246  },
    Complex { re: -0.93941931782002, im: -0.56409379640356 }, Complex { re: -0.88590003188677, im: 0.47624600491382  },
    Complex { re: 0.99971463703691, im: -0.83889954253462  }, Complex { re: -0.75376385639978, im: 0.00814643438625  },
    Complex { re: 0.93887685615875, im: -0.11284528204636  }, Complex { re: 0.85126435782309, im: 0.52349251543547   },
    Complex { re: 0.39701421446381, im: 0.81779634174316   }, Complex { re: -0.37024464187437, im: -0.87071656222959 },
    Complex { re: -0.36024828242896, im: 0.34655735648287  }, Complex { re: -0.93388812549209, im: -0.84476541096429 },
    Complex { re: -0.65298804552119, im: -0.18439575450921 }, Complex { re: 0.11960319006843, im: 0.99899346780168   },
    Complex { re: 0.94292565553160, im: 0.83163906518293   }, Complex { re: 0.75081145286948, im: -0.35533223142265  },
    Complex { re: 0.56721979748394, im: -0.24076836414499  }, Complex { re: 0.46857766746029, im: -0.30140233457198  },
    Complex { re: 0.97312313923635, im: -0.99548191630031  }, Complex { re: -0.38299976567017, im: 0.98516909715427  },
    Complex { re: 0.41025800019463, im: 0.02116736935734   }, Complex { re: 0.09638062008048, im: 0.04411984381457   },
    Complex { re: -0.85283249275397, im: 0.91475563922421  }, Complex { re: 0.88866808958124, im: -0.99735267083226  },
    Complex { re: -0.48202429536989, im: -0.96805608884164 }, Complex { re: 0.27572582416567, im: 0.58634753335832   },
    Complex { re: -0.65889129659168, im: 0.58835634138583  }, Complex { re: 0.98838086953732, im: 0.99994349600236   },
    Complex { re: -0.20651349620689, im: 0.54593044066355  }, Complex { re: -0.62126416356920, im: -0.59893681700392 },
    Complex { re: 0.20320105410437, im: -0.86879180355289  }, Complex { re: -0.97790548600584, im: 0.96290806999242  },
    Complex { re: 0.11112534735126, im: 0.21484763313301   }, Complex { re: -0.41368337314182, im: 0.28216837680365  },
    Complex { re: 0.24133038992960, im: 0.51294362630238   }, Complex { re: -0.66393410674885, im: -0.08249679629081 },
    Complex { re: -0.53697829178752, im: -0.97649903936228 }, Complex { re: -0.97224737889348, im: 0.22081333579837  },
    Complex { re: 0.87392477144549, im: -0.12796173740361  }, Complex { re: 0.19050361015753, im: 0.01602615387195   },
    Complex { re: -0.46353441212724, im: -0.95249041539006 }, Complex { re: -0.07064096339021, im: -0.94479803205886 },
    Complex { re: -0.92444085484466, im: -0.10457590187436 }, Complex { re: -0.83822593578728, im: -0.01695043208885 },
    Complex { re: 0.75214681811150, im: -0.99955681042665  }, Complex { re: -0.42102998829339, im: 0.99720941999394  },
    Complex { re: -0.72094786237696, im: -0.35008961934255 }, Complex { re: 0.78843311019251, im: 0.52851398958271   },
    Complex { re: 0.97394027897442, im: -0.26695944086561  }, Complex { re: 0.99206463477946, im: -0.57010120849429  },
    Complex { re: 0.76789609461795, im: -0.76519356730966  }, Complex { re: -0.82002421836409, im: -0.73530179553767 },
    Complex { re: 0.81924990025724, im: 0.99698425250579   }, Complex { re: -0.26719850873357, im: 0.68903369776193  },
    Complex { re: -0.43311260380975, im: 0.85321815947490  }, Complex { re: 0.99194979673836, im: 0.91876249766422   },
    Complex { re: -0.80692001248487, im: -0.32627540663214 }, Complex { re: 0.43080003649976, im: -0.21919095636638  },
    Complex { re: 0.67709491937357, im: -0.95478075822906  }, Complex { re: 0.56151770568316, im: -0.70693811747778  },
    Complex { re: 0.10831862810749, im: -0.08628837174592  }, Complex { re: 0.91229417540436, im: -0.65987351408410  },
    Complex { re: -0.48972893932274, im: 0.56289246362686  }, Complex { re: -0.89033658689697, im: -0.71656563987082 },
    Complex { re: 0.65269447475094, im: 0.65916004833932   }, Complex { re: 0.67439478141121, im: -0.81684380846796  },
    Complex { re: -0.47770832416973, im: -0.16789556203025 }, Complex { re: -0.99715979260878, im: -0.93565784007648 },
    Complex { re: -0.90889593602546, im: 0.62034397054380  }, Complex { re: -0.06618622548177, im: -0.23812217221359 },
    Complex { re: 0.99430266919728, im: 0.18812555317553   }, Complex { re: 0.97686402381843, im: -0.28664534366620  },
    Complex { re: 0.94813650221268, im: -0.97506640027128  }, Complex { re: -0.95434497492853, im: -0.79607978501983 },
    Complex { re: -0.49104783137150, im: 0.32895214359663  }, Complex { re: 0.99881175120751, im: 0.88993983831354   },
    Complex { re: 0.50449166760303, im: -0.85995072408434  }, Complex { re: 0.47162891065108, im: -0.18680204049569  },
    Complex { re: -0.62081581361840, im: 0.75000676218956  }, Complex { re: -0.43867015250812, im: 0.99998069244322  },
    Complex { re: 0.98630563232075, im: -0.53578899600662  }, Complex { re: -0.61510362277374, im: -0.89515019899997 },
    Complex { re: -0.03841517601843, im: -0.69888815681179 }, Complex { re: -0.30102157304644, im: -0.07667808922205 },
    Complex { re: 0.41881284182683, im: 0.02188098922282   }, Complex { re: -0.86135454941237, im: 0.98947480909359  },
    Complex { re: 0.67226861393788, im: -0.13494389011014  }, Complex { re: -0.70737398842068, im: -0.76547349325992 },
    Complex { re: 0.94044946687963, im: 0.09026201157416   }, Complex { re: -0.82386352534327, im: 0.08924768823676  },
    Complex { re: -0.32070666698656, im: 0.50143421908753  }, Complex { re: 0.57593163224487, im: -0.98966422921509  },
    Complex { re: -0.36326018419965, im: 0.07440243123228  }, Complex { re: 0.99979044674350, im: -0.14130287347405  },
    Complex { re: -0.92366023326932, im: -0.97979298068180 }, Complex { re: -0.44607178518598, im: -0.54233252016394 },
    Complex { re: 0.44226800932956, im: 0.71326756742752   }, Complex { re: 0.03671907158312, im: 0.63606389366675   },
    Complex { re: 0.52175424682195, im: -0.85396826735705  }, Complex { re: -0.94701139690956, im: -0.01826348194255 },
    Complex { re: -0.98759606946049, im: 0.82288714303073  }, Complex { re: 0.87434794743625, im: 0.89399495655433   },
    Complex { re: -0.93412041758744, im: 0.41374052024363  }, Complex { re: 0.96063943315511, im: 0.93116709541280   },
    Complex { re: 0.97534253457837, im: 0.86150930812689   }, Complex { re: 0.99642466504163, im: 0.70190043427512   },
    Complex { re: -0.94705089665984, im: -0.29580042814306 }, Complex { re: 0.91599807087376, im: -0.98147830385781  }
    ];
