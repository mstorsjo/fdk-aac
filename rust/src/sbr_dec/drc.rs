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
//! Dynamic range control (DRC) decoder tool for SBR.

use super::flags::SbrDecFlags;
use super::{common::SbrError, constants::SBRDEC_MAX_SYNTHESIS_CHANNELS};
use itertools::izip;

pub const SBRDEC_MAX_DRC_CHANNELS: usize = 8;
pub const SBRDEC_MAX_DRC_BANDS: usize = 16;

/// DRC - Offset table for QMF interpolation. Shifted by one index position.
/// The table defines the (short) window borders rounded to the nearest QMF
/// timeslot. It has the size 16 because it is accessed with the
/// drcInterpolationScheme that is read from the bitstream with 4 bit.
#[rustfmt::skip]
static WIN_BORDER_TO_COL_MAPPING_TAB: [[u8; 16]; 2] = [
    // 1024 framing.
    [0, 0, 4, 8, 12, 16, 20, 24, 28, 32, 32, 32, 32, 32, 32, 32],
    // 960 framing.
    [0, 0, 4, 8, 11, 15, 19, 23, 26, 30, 30, 30, 30, 30, 30, 30],
];

/// Applies DRC with the audio object type SBR.
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct SbrDecDrcChannel {
    /// Previous DRC gain factor magnitudes.
    prev_fact_mag: [f32; SBRDEC_MAX_SYNTHESIS_CHANNELS],
    /// DRC gain factor magnitudes of current slot.
    curr_fact_mag: [f32; SBRDEC_MAX_DRC_BANDS],
    /// DRC gain factor magnitudes of next slot.
    next_fact_mag: [f32; SBRDEC_MAX_DRC_BANDS],
    /// Num QMF bands of current slot.
    num_bands_curr: u32,
    /// Num QMF bands of next slot.
    num_bands_next: u32,
    /// QMF band borders of current slot.
    band_top_curr: [u16; SBRDEC_MAX_DRC_BANDS],
    /// QMF band borders of next slot.
    band_top_next: [u16; SBRDEC_MAX_DRC_BANDS],
    /// DRC interpolation scheme used for the DRC data in the SBR QMF domain.
    drc_interpolation_scheme_curr: u16,
    /// DRC interpolation scheme used for the DRC data in the SBR QMF domain.
    drc_interpolation_scheme_next: u16,
    enable: bool,
    /// Current window sequence.
    win_sequence_curr: u8,
    /// Next window sequence.
    win_sequence_next: u8,
    /// Next next window sequence.
    win_sequence_next_next: u8,
}

impl Default for SbrDecDrcChannel {
    fn default() -> Self {
        Self {
            prev_fact_mag: [0.0; 64],
            curr_fact_mag: [0.0; SBRDEC_MAX_DRC_BANDS],
            next_fact_mag: [0.0; SBRDEC_MAX_DRC_BANDS],
            num_bands_curr: 0,
            num_bands_next: 0,
            band_top_curr: [0; SBRDEC_MAX_DRC_BANDS],
            band_top_next: [0; SBRDEC_MAX_DRC_BANDS],
            drc_interpolation_scheme_curr: 0,
            drc_interpolation_scheme_next: 0,
            enable: false,
            win_sequence_curr: 0,
            win_sequence_next: 0,
            win_sequence_next_next: 0,
        }
    }
}

impl SbrDecDrcChannel {
    pub fn new() -> Self {
        SbrDecDrcChannel::default()
    }

    /// Initializes DRC QMF factors.
    pub fn init_channel(&mut self) {
        self.prev_fact_mag.fill(1.0);

        self.curr_fact_mag.fill(1.0);
        self.next_fact_mag.fill(1.0);

        self.num_bands_curr = 1;
        self.num_bands_next = 1;

        self.win_sequence_curr = 0;
        self.win_sequence_next = 0;
        self.win_sequence_next_next = 0;

        self.drc_interpolation_scheme_curr = 0;
        self.drc_interpolation_scheme_next = 0;

        self.enable = false;
    }

    /// Swaps DRC QMF scaling factors after they have been applied.
    ///
    /// # Parameters
    ///
    /// - `flags`: SBR decoder flags.
    pub(super) fn update_channel(&mut self, flags: SbrDecFlags) {
        if !(self.enable) {
            return;
        }
        // Swap previous data.
        self.curr_fact_mag.copy_from_slice(&self.next_fact_mag);

        self.num_bands_curr = self.num_bands_next;

        self.band_top_curr.copy_from_slice(&self.band_top_next);

        self.drc_interpolation_scheme_curr = self.drc_interpolation_scheme_next;

        self.win_sequence_curr = self.win_sequence_next;

        if !flags.contains(SbrDecFlags::SYNTAX_USAC)
            && flags.contains(SbrDecFlags::USAC_HARMONICSBR)
        {
            self.win_sequence_next = self.win_sequence_next_next;
        }
    }

    pub fn feed(
        &mut self,
        num_bands: usize,
        next_fact_mag: &[f32],
        drc_interpolation_scheme: u16,
        win_sequence: u8,
        band_top: &[u16],
        sbr_flags: SbrDecFlags,
    ) -> Result<(), SbrError> {
        self.enable = true;
        self.num_bands_next = num_bands as u32;

        // In case of MPEG4 eSbr window sequence needs to be delayed by one frame.
        if !sbr_flags.contains(SbrDecFlags::SYNTAX_USAC)
            && sbr_flags.contains(SbrDecFlags::USAC_HARMONICSBR)
        {
            self.win_sequence_next_next = win_sequence;
        } else {
            self.win_sequence_next = win_sequence;
        }
        self.drc_interpolation_scheme_next = drc_interpolation_scheme;

        for (inp_band_top_next, out_band_top_next, inp_fact_mag_next, out_fact_mag_next) in izip!(
            band_top.iter(),
            self.band_top_next.iter_mut(),
            next_fact_mag.iter(),
            self.next_fact_mag.iter_mut(),
        )
        .take(num_bands)
        {
            *out_band_top_next = *inp_band_top_next;
            *out_fact_mag_next = *inp_fact_mag_next;
        }

        Ok(())
    }

    /// Applies DRC factors slot based.
    ///
    /// # Parameters
    /// - `qmf_real_slot`: Pointer to real valued QMF data of one time slot.
    /// - `qmf_imag_slot`: Pointer to the imaginary QMF data of one time slot.
    /// - `col`: Number of the time slot.
    /// - `num_qmf_slots`: Total number of time slots for one frame.
    pub(super) fn apply_slot(
        &mut self,
        qmf_real_slot: &mut [f32],
        qmf_imag_slot: &mut [f32],
        mut col: u32,
        num_qmf_slots: u32,
    ) {
        let num_qmf_slots_div_2 = num_qmf_slots >> 1;
        // l_border.
        let indx = num_qmf_slots - num_qmf_slots_div_2 - 10;

        let mut is_short_window = false;
        let is_short_frame = num_qmf_slots == 30;
        let frame_size: u32 = if is_short_frame { 960 } else { 1024 };

        let mut alpha_value: f32 = 0.0;
        let mut alpha_value_2: f32;

        if !(self.enable) {
            return;
        }

        let win_border_to_col_map = WIN_BORDER_TO_COL_MAPPING_TAB[usize::from(is_short_frame)];

        col += indx;

        // Gets respective data and calc interpolation factor.
        let (fact_mag, num_bands, band_top) = if col < num_qmf_slots_div_2 {
            // First half of current frame.
            if self.win_sequence_curr != 2 {
                // Long window.
                let j = col + num_qmf_slots_div_2;
                if j < win_border_to_col_map[15].into() {
                    if self.drc_interpolation_scheme_curr == 0 {
                        let k: f32 = if is_short_frame {
                            1.0 / 30.0
                        } else {
                            1.0 / 32.0
                        };
                        alpha_value = (j as f32) * k;
                    } else if j
                        >= win_border_to_col_map[self.drc_interpolation_scheme_curr as usize] as u32
                    {
                        alpha_value = 1.0;
                    }
                } else {
                    alpha_value = 1.0;
                }
            } else {
                // Short windows.
                is_short_window = true;
            }
            (
                &self.curr_fact_mag,
                self.num_bands_curr as usize,
                &self.band_top_curr,
            )
        } else if col < num_qmf_slots {
            // Second half of current frame.
            if self.win_sequence_next != 2 {
                // Next: long window.
                let j = col - num_qmf_slots_div_2;
                if j < win_border_to_col_map[15].into() {
                    if self.drc_interpolation_scheme_next == 0 {
                        let k: f32 = if is_short_frame {
                            1.0 / 30.0
                        } else {
                            1.0 / 32.0
                        };
                        alpha_value = (j as f32) * k;
                    } else if j
                        >= win_border_to_col_map[self.drc_interpolation_scheme_next as usize] as u32
                    {
                        alpha_value = 1.0;
                    }
                } else {
                    alpha_value = 1.0;
                }
                (
                    &self.next_fact_mag,
                    self.num_bands_next as usize,
                    &self.band_top_next,
                )
            } else {
                // Next: short windows.
                if self.win_sequence_curr != 2 {
                    // Current: long window.
                    alpha_value = 0.0;
                    (
                        &self.next_fact_mag,
                        self.num_bands_next as usize,
                        &self.band_top_next,
                    )
                } else {
                    // Current: short windows.
                    is_short_window = true;
                    (
                        &self.curr_fact_mag,
                        self.num_bands_curr as usize,
                        &self.band_top_curr,
                    )
                }
            }
        } else {
            // First half of next frame.
            if self.win_sequence_next != 2 {
                // Long window.
                let j = col - num_qmf_slots_div_2;
                if j < win_border_to_col_map[15].into() {
                    if self.drc_interpolation_scheme_next == 0 {
                        let k: f32 = if is_short_frame {
                            1.0 / 30.0
                        } else {
                            1.0 / 32.0
                        };
                        alpha_value = (j as f32) * k;
                    } else if j
                        >= win_border_to_col_map[self.drc_interpolation_scheme_next as usize] as u32
                    {
                        alpha_value = 1.0;
                    }
                } else {
                    alpha_value = 1.0;
                }
            } else {
                // Short windows.
                is_short_window = true;
            }
            col -= num_qmf_slots;
            (
                &self.next_fact_mag,
                self.num_bands_next as usize,
                &self.band_top_next,
            )
        };

        let mut bottom_mdct: u32 = 0;
        let mut top_mdct: u32;

        for (band, (b_top, f_mag)) in izip!(band_top.iter(), fact_mag.iter())
            .enumerate()
            .take(num_bands)
        {
            let mut bottom_qmf: u32;
            let mut top_qmf: u32;
            let mut drc_fact_mag: f32;
            top_mdct = ((*b_top + 1) << 2) as u32;

            if !is_short_window {
                // Long window.
                if is_short_frame {
                    // 960 framing.
                    bottom_qmf = ((1.0 / 30.0) * (bottom_mdct as f32)).floor() as u32;
                    top_qmf = ((1.0 / 30.0) * (top_mdct as f32)).floor() as u32;
                    top_mdct = 30 * top_qmf;
                } else {
                    // 1024 framing.
                    top_mdct &= !0x1f;
                    bottom_qmf = bottom_mdct >> 5;
                    top_qmf = top_mdct >> 5;
                }
                if band == num_bands - 1 {
                    top_qmf = 64;
                }

                alpha_value_2 = 1.0 - alpha_value;
                let drc_fact_2_mag: f32 = alpha_value * *f_mag;

                if bottom_qmf < top_qmf {
                    let start = bottom_qmf as usize;
                    let end = top_qmf as usize;
                    if alpha_value_2 != 0.0 {
                        for (drc_fact_1_mag, qmf_r, qmf_i) in izip!(
                            self.prev_fact_mag[start..end].iter(),
                            qmf_real_slot[start..end].iter_mut(),
                            qmf_imag_slot[start..end].iter_mut()
                        ) {
                            // Interpolate.
                            drc_fact_mag = drc_fact_2_mag + alpha_value_2 * *drc_fact_1_mag;

                            // Apply scaling.
                            *qmf_r *= drc_fact_mag;
                            *qmf_i *= drc_fact_mag;
                        }
                    } else if drc_fact_2_mag != 1.0 {
                        for (qmf_r, qmf_i) in izip!(
                            qmf_real_slot[start..end].iter_mut(),
                            qmf_imag_slot[start..end].iter_mut()
                        ) {
                            // Apply scaling.
                            *qmf_r *= drc_fact_2_mag;
                            *qmf_i *= drc_fact_2_mag;
                        }
                    }
                }
                // Save previous factors.
                if (col == num_qmf_slots_div_2 - 1) && (bottom_qmf <= top_qmf) {
                    self.prev_fact_mag[bottom_qmf as usize..top_qmf as usize].fill(*f_mag);
                }
            } else {
                // Short window.
                let start_win_idx: u32;
                let mut stop_win_idx: u32;
                let inv_frame_size_div_8: f32 = if is_short_frame {
                    1.0 / 120.0
                } else {
                    1.0 / 128.0
                };

                // Limit top at the frame borders.
                if top_mdct >= frame_size {
                    top_mdct = frame_size - 1
                }

                if is_short_frame {
                    // 960 framing.
                    top_mdct = (0.9375f32 * (0.266666681f32 * (top_mdct as f32)).floor() * 4.0f32)
                        .floor() as u32;

                    start_win_idx =
                        ((inv_frame_size_div_8 * (bottom_mdct as f32)).floor() + 1.0) as u32;
                    stop_win_idx = (((inv_frame_size_div_8 - f32::EPSILON) * (top_mdct as f32))
                        .ceil()
                        + 1.0) as u32;
                } else {
                    // 1024 framing.
                    top_mdct &= !0x03;
                    start_win_idx =
                        ((inv_frame_size_div_8 * (bottom_mdct as f32)).floor() + 1.0) as u32;
                    stop_win_idx = ((inv_frame_size_div_8 * (top_mdct as f32)).ceil() + 1.0) as u32;
                }

                // start_col is truncated to the nearest corresponding start subsample in the QMF of
                // the short window.
                let start_col: u32 = win_border_to_col_map[start_win_idx as usize] as u32;
                // stop_col is rounded upwards to the nearest corresponding stop subsample in the
                // QMF of the short window.
                let mut stop_col: u32 = win_border_to_col_map[stop_win_idx as usize] as u32;
                bottom_qmf = (inv_frame_size_div_8
                    * ((bottom_mdct % (num_qmf_slots << 2)) << 5) as f32)
                    .floor() as u32;
                top_qmf = (inv_frame_size_div_8 * ((top_mdct % (num_qmf_slots << 2)) << 5) as f32)
                    .floor() as u32;

                // Extend last band.
                if band == num_bands - 1 {
                    stop_win_idx = 10;
                    stop_col = num_qmf_slots;
                    top_qmf = 64;
                }

                if top_qmf == 0 {
                    if is_short_frame && ((top_mdct & 0x03) != 0) {
                        stop_win_idx -= 1;
                        stop_col = win_border_to_col_map[stop_win_idx as usize] as u32;
                    }
                    top_qmf = 64;
                }

                // Saves previous factors.
                if stop_col == num_qmf_slots {
                    let mut tmp_bottom = bottom_qmf;
                    if (win_border_to_col_map[8] as u32) > start_col {
                        // Band starts in previous short window.
                        tmp_bottom = 0;
                    }

                    if tmp_bottom <= top_qmf {
                        self.prev_fact_mag[tmp_bottom as usize..top_qmf as usize].fill(*f_mag);
                    }
                }

                // Apply.
                if (col >= start_col) && (col < stop_col) {
                    if col >= (win_border_to_col_map[(start_win_idx + 1) as usize] as u32) {
                        // Band starts in previous short window.
                        bottom_qmf = 0;
                    }
                    if col < (win_border_to_col_map[(stop_win_idx - 1) as usize] as u32) {
                        // Band ends in next short window.
                        top_qmf = 64;
                    }

                    drc_fact_mag = *f_mag;

                    // Applies scaling.
                    if bottom_qmf < top_qmf {
                        let start = bottom_qmf as usize;
                        let end = top_qmf as usize;
                        for (qmf_r, qmf_i) in izip!(
                            qmf_real_slot[start..end].iter_mut(),
                            qmf_imag_slot[start..end].iter_mut()
                        ) {
                            *qmf_r *= drc_fact_mag;
                            *qmf_i *= drc_fact_mag;
                        }
                    }
                }
            }
            bottom_mdct = top_mdct;
        }
        // End of bands loop.
    }

    /// Applies DRC factors frame based.
    ///
    /// # Parameters
    /// - `qmf_real_slot`: Pointer to real valued QMF data of the whole frame.
    /// - `qmf_imag_slot`: Pointer to the imaginary QMF data of the whole frame.
    /// - `num_qmf_slots`: Total number of time slots for one frame.
    pub(super) fn apply(
        &mut self,
        qmf_buffer_real: &mut [&mut [f32]],
        qmf_buffer_imag: &mut [&mut [f32]],
        num_qmf_slots: u32,
    ) {
        if !self.enable {
            // Avoid changing the scaleFactor even though the processing is disabled.
            return;
        }

        for (col, (qmf_real_slot, qmf_imag_slot)) in
            izip!(qmf_buffer_real.iter_mut(), qmf_buffer_imag.iter_mut())
                .enumerate()
                .take(num_qmf_slots as usize)
        {
            self.apply_slot(qmf_real_slot, qmf_imag_slot, col as u32, num_qmf_slots);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::SBRDEC_MAX_DRC_BANDS;
    use crate::sbr_dec::{drc::SbrDecDrcChannel, flags::SbrDecFlags};

    #[test]
    fn new() {
        let handle_sbr_drc_channel = SbrDecDrcChannel::new();
        assert_eq!([0f32; 64], handle_sbr_drc_channel.prev_fact_mag);
        assert_eq!(
            [0f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.curr_fact_mag
        );
        assert_eq!(
            [0f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.next_fact_mag
        );
        assert_eq!(0, handle_sbr_drc_channel.num_bands_curr);
        assert_eq!(0, handle_sbr_drc_channel.num_bands_next);
        assert_eq!(
            [0u16; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.band_top_curr
        );
        assert_eq!(
            [0u16; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.band_top_next
        );
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_curr);
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_next);
        assert!(!handle_sbr_drc_channel.enable);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_curr);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_next);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_next_next);
    }
    #[test]
    fn init_channel() {
        let mut handle_sbr_drc_channel = SbrDecDrcChannel::new();
        handle_sbr_drc_channel.init_channel();
        assert_eq!([1f32; 64], handle_sbr_drc_channel.prev_fact_mag);
        assert_eq!(
            [1f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.curr_fact_mag
        );
        assert_eq!(
            [1f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.next_fact_mag
        );
        assert_eq!(1, handle_sbr_drc_channel.num_bands_curr);
        assert_eq!(1, handle_sbr_drc_channel.num_bands_next);
        assert_eq!(
            [0u16; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.band_top_curr
        );
        assert_eq!(
            [0u16; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.band_top_next
        );
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_curr);
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_next);
        assert!(!handle_sbr_drc_channel.enable);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_curr);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_next);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_next_next);
    }
    #[test]
    fn feed() {
        let mut handle_sbr_drc_channel = SbrDecDrcChannel::new();
        handle_sbr_drc_channel.init_channel();
        let sbr_result = handle_sbr_drc_channel.feed(
            1,
            &[0f32],
            0,
            1,
            &[255u16],
            SbrDecFlags::from_bits(256).unwrap(),
        );
        assert_eq!(Ok(()), sbr_result);
        assert_eq!([1f32; 64], handle_sbr_drc_channel.prev_fact_mag);
        assert_eq!(
            [1f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.curr_fact_mag
        );
        let mut test_data_next_fact_mag_fl = [1f32; SBRDEC_MAX_DRC_BANDS];
        test_data_next_fact_mag_fl[0] = 0f32;
        assert_eq!(
            test_data_next_fact_mag_fl,
            handle_sbr_drc_channel.next_fact_mag
        );
        assert_eq!(1, handle_sbr_drc_channel.num_bands_curr);
        assert_eq!(1, handle_sbr_drc_channel.num_bands_next);
        assert_eq!(
            [0u16; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.band_top_curr
        );
        let mut test_data_band_top_next = [0u16; SBRDEC_MAX_DRC_BANDS];
        test_data_band_top_next[0] = 255u16;
        assert_eq!(
            test_data_band_top_next,
            handle_sbr_drc_channel.band_top_next
        );
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_curr);
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_next);
        assert!(handle_sbr_drc_channel.enable);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_curr);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_next);
        assert_eq!(1, handle_sbr_drc_channel.win_sequence_next_next);
    }

    #[test]
    fn update_channel() {
        let mut handle_sbr_drc_channel = SbrDecDrcChannel::new();
        handle_sbr_drc_channel.init_channel();
        let _ = handle_sbr_drc_channel.feed(
            1,
            &[0f32],
            0,
            1,
            &[255u16],
            SbrDecFlags::from_bits(256).unwrap(),
        );
        handle_sbr_drc_channel.next_fact_mag[0] = 1f32;
        let flags = SbrDecFlags::from_bits(65792).unwrap();
        handle_sbr_drc_channel.update_channel(flags);
        assert_eq!([1f32; 64], handle_sbr_drc_channel.prev_fact_mag);
        assert_eq!(
            [1f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.curr_fact_mag
        );
        assert_eq!(
            [1f32; SBRDEC_MAX_DRC_BANDS],
            handle_sbr_drc_channel.next_fact_mag
        );
        assert_eq!(1, handle_sbr_drc_channel.num_bands_curr);
        assert_eq!(1, handle_sbr_drc_channel.num_bands_next);
        let mut test_data_band_top_curr = [0u16; SBRDEC_MAX_DRC_BANDS];
        test_data_band_top_curr[0] = 255u16;
        assert_eq!(
            test_data_band_top_curr,
            handle_sbr_drc_channel.band_top_curr
        );
        let mut test_data_band_top_next = [0u16; SBRDEC_MAX_DRC_BANDS];
        test_data_band_top_next[0] = 255u16;
        assert_eq!(
            test_data_band_top_next,
            handle_sbr_drc_channel.band_top_next
        );
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_curr);
        assert_eq!(0, handle_sbr_drc_channel.drc_interpolation_scheme_next);
        assert!(handle_sbr_drc_channel.enable);
        assert_eq!(0, handle_sbr_drc_channel.win_sequence_curr);
        assert_eq!(1, handle_sbr_drc_channel.win_sequence_next);
        assert_eq!(1, handle_sbr_drc_channel.win_sequence_next_next);
    }
}
