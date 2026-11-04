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
//! USAC MPS212 transient steering decorrelator (TSD)

use crate::common::bitstream::Bitstream;
use itertools::izip;
use num_complex::Complex;
use std::f32::consts;

const MAX_TSD_TIME_SLOTS: usize = 64;
const TSD_START_BAND: usize = 7;

const SIZE_S: usize = 4;
const SIZE_C: usize = 5;

static N_BITS_TSD_CW_32_SLOTS: [u8; 32] = [
    5, 9, 13, 16, 18, 20, 22, 24, 25, 26, 27, 28, 29, 29, 30, 30, 30, 29, 29, 28, 27, 26, 25, 24,
    22, 20, 18, 16, 13, 9, 5, 0,
];

static N_BITS_TSD_CW_64_SLOTS: [u8; 64] = [
    6, 11, 16, 20, 23, 27, 30, 33, 35, 38, 40, 42, 44, 46, 48, 49, 51, 52, 53, 55, 56, 57, 58, 58,
    59, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 60, 60, 60, 59, 58, 58, 57, 56, 55, 53, 52, 51, 49,
    48, 46, 44, 42, 40, 38, 35, 33, 30, 27, 23, 20, 16, 11, 6, 0,
];

/// Phase data for the transient steering of TSD according to ISO-IEC 23003-3, 2020-06, Table 111.
static PHI_TSD: [Complex<f32>; 8] = [
    Complex { re: 1.0, im: 0.0 },
    Complex {
        re: consts::FRAC_1_SQRT_2,
        im: consts::FRAC_1_SQRT_2,
    },
    Complex { re: 0.0, im: 1.0 },
    Complex {
        re: -consts::FRAC_1_SQRT_2,
        im: consts::FRAC_1_SQRT_2,
    },
    Complex { re: -1.0, im: 0.0 },
    Complex {
        re: -consts::FRAC_1_SQRT_2,
        im: -consts::FRAC_1_SQRT_2,
    },
    Complex { re: 0.0, im: -1.0 },
    Complex {
        re: consts::FRAC_1_SQRT_2,
        im: -consts::FRAC_1_SQRT_2,
    },
];

#[repr(C)]
#[derive(Debug)]
/// Holds the data needed to apply TSD to current frame.
pub(super) struct TsdData {
    is_tsd_enabled: bool,
    ts: usize,
    /// Broadband phase difference measure.
    //  -1 => TsdSepData[ts]=0;
    // 0-7 => values of bs_tsd_tr_phase_data[ts] and TsdSepData[ts]=1
    bs_tsd_tr_phase_data: [i8; MAX_TSD_TIME_SLOTS],
}

impl TsdData {
    /// Generates d_{x,tr} signal and adds to d_{x,non_tr} signal.
    ///
    /// # Parameters
    ///
    /// - `num_hybrid_bands`: Number of hybrid bands.
    /// - `v_direct`: Direct signal.
    /// - `d_non_tr`: Non transient signal.
    pub(super) fn apply(
        &mut self,
        num_hybrid_bands: usize,
        v_direct: &[Complex<f32>],
        d_non_tr: &mut [Complex<f32>],
    ) {
        let ts = self.ts;
        if self.is_tr_slot() && num_hybrid_bands > TSD_START_BAND {
            let phi = &PHI_TSD[self.bs_tsd_tr_phase_data[ts] as usize];

            // d = d_non_tr + v_direct * exp(j * bs_tsd_tr_phase_data[ts]/4 * pi ).
            for (v_dir_elem, d_non_tr_elem) in izip!(
                v_direct[TSD_START_BAND..num_hybrid_bands].iter(),
                d_non_tr[TSD_START_BAND..num_hybrid_bands].iter_mut()
            ) {
                *d_non_tr_elem += *v_dir_elem * phi;
            }
        }

        // The modulo MAX_TSD_TIME_SLOTS operation is to avoid illegal memory accesses,
        // in case of errors.
        self.ts = (ts + 1) & (MAX_TSD_TIME_SLOTS - 1);
    }

    /// Clears TSD specific data.
    pub(super) fn clear(&mut self) {
        self.is_tsd_enabled = false;
    }

    /// Clears TSD specific data (time slot counter).
    pub(super) fn clear_ts(&mut self) {
        self.ts = 0;
    }

    /// Performs transient separation (v_{x,non_tr} signal).
    /// # Parameters
    /// - `num_hybrid_bands`: Number of hybrid bands.
    /// - `v_direct_cplx`: Direct signal.
    /// - `v_non_tr_cplx`: Non transient signal.
    pub(super) fn generate_non_tr(
        &mut self,
        num_hybrid_bands: usize,
        v_direct: &[Complex<f32>],
        v_non_tr: &mut [Complex<f32>],
    ) {
        if !self.is_tr_slot() {
            // Let allpass based decorrelator read from direct input.
            v_non_tr[0..num_hybrid_bands].copy_from_slice(&v_direct[0..num_hybrid_bands]);
        } else {
            // Generate nonTr input signal for allpass based decorrelator.
            v_non_tr[0..TSD_START_BAND].copy_from_slice(&v_direct[0..TSD_START_BAND]);
            v_non_tr[TSD_START_BAND..num_hybrid_bands].fill(Complex { re: 0.0, im: 0.0 });
        }
    }

    /// Returns whether the TSD is enabled or not.
    /// This data is read from bitstream, in `TsdData::read()`.
    ///
    /// # Return
    /// - true: TSD is enabled.
    /// - false: TSD is disabled.
    pub(super) fn is_tsd_enabled(&self) -> bool {
        self.is_tsd_enabled
    }

    #[inline(always)]
    /// Returns whether the phase data for the `ts` time slot is non-negative.
    ///
    /// # Return
    /// - true: phase data for `ts` is `>= 0`.
    /// - false: phase data for `ts` is `< 0`.
    fn is_tr_slot(&self) -> bool {
        self.bs_tsd_tr_phase_data[self.ts] >= 0
    }

    /// Creates `TsdData` instance.
    pub fn new() -> TsdData {
        TsdData {
            is_tsd_enabled: false,
            ts: 0,
            bs_tsd_tr_phase_data: [0; MAX_TSD_TIME_SLOTS],
        }
    }

    /// Parses and decodes TSD data.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `num_slots`: Number of QMF slots per frame.
    ///
    /// # Return
    /// - error: False on success; True on error.
    pub(super) fn read(&mut self, bs: &mut Bitstream, num_slots: usize) -> bool {
        let mut error = false;
        let n_bits_tr_slots;
        let n_bits_tsd_cw_tab: &'static [u8];

        match num_slots {
            32 => {
                n_bits_tr_slots = 4;
                n_bits_tsd_cw_tab = &N_BITS_TSD_CW_32_SLOTS;
            }
            64 => {
                n_bits_tr_slots = 5;
                n_bits_tsd_cw_tab = &N_BITS_TSD_CW_64_SLOTS;
            }
            _ => {
                error = true;
                return error;
            }
        }

        // Read TempShapeData for bsTempShapeConfig  == 3.
        self.is_tsd_enabled = bs.read_bit() > 0;
        if !self.is_tsd_enabled {
            error = false;
            return error;
        }

        // Parse/Decode TsdData.
        let bs_tsd_num_tr_slots = bs.read(n_bits_tr_slots) as u16;

        // Decode transient slot positions.
        {
            // bsTsdCodedPos
            let mut s: [u16; SIZE_S] = [0; SIZE_S];
            let mut c: [u16; SIZE_C] = [0; SIZE_C];
            let mut remainder: u16 = 0;

            // Init with TsdSepData[k] = 0.
            self.bs_tsd_tr_phase_data[0..num_slots].fill(-1);

            let mut n_bits_tsd_cw = n_bits_tsd_cw_tab[usize::from(bs_tsd_num_tr_slots)];

            for h in (0..(SIZE_S as u8)).rev() {
                if n_bits_tsd_cw > h * 16 {
                    s[usize::from(h)] = bs.read(n_bits_tsd_cw - h * 16) as u16;
                    n_bits_tsd_cw = h * 16;
                }
            }

            // c = prod_{h=1}^{p} (k-p+h)/h
            let k = num_slots as u16 - 1;
            let mut p = bs_tsd_num_tr_slots + 1;

            c[0] = k - p + 1;
            for h in 2..=p {
                // c *= k - p + h;
                TsdData::long_mul(&mut c, k - p + h);
                // c /= h;
                TsdData::long_div(&mut c, h, &mut remainder);
                debug_assert!(remainder == 0);
            }

            // Go through all slots.
            for ki in (0..=k).rev() {
                if p > ki {
                    // Means TsdSepData[] = 1.
                    self.bs_tsd_tr_phase_data[0..=ki as usize].fill(1);
                    break;
                }
                // if (s >= c)
                if TsdData::long_compare(&s, &c) {
                    // s -= c;
                    TsdData::long_sub(&mut s, &c);

                    // Means TsdSepData[] = 1.
                    self.bs_tsd_tr_phase_data[usize::from(ki)] = 1;
                    if p == 1 {
                        break;
                    }
                    // Update c for next iteration: c_new = c_old * p / ki.
                    TsdData::long_mul(&mut c, p);
                    p -= 1;
                } else {
                    // Update c for next iteration: c_new = c_old * (ki-p) / ki.
                    TsdData::long_mul(&mut c, ki - p);
                }
                TsdData::long_div(&mut c, ki, &mut remainder);
                debug_assert!(remainder == 0);
            }

            // Read phase data.
            for phase_data in self.bs_tsd_tr_phase_data.iter_mut().take(num_slots) {
                if *phase_data == 1 {
                    *phase_data = bs.read(3) as i8;
                }
            }
        }
        error
    }

    /// Calculates `a[i] *= b`, with carry.
    fn long_mul(a: &mut [u16], b: u16) {
        let b0 = u32::from(b);
        let mut tmp = 0;

        for out in a.iter_mut() {
            tmp = (tmp >> 16) + u32::from(*out) * b0;
            *out = tmp as u16;
        }
    }

    /// Calculates `b[i] /= a`. Remainder is also passed further out.
    fn long_div(b: &mut [u16], a: u16, rem: &mut u16) {
        let a0 = u32::from(a);
        let mut r = 0;
        let mut tmp;
        for out in b.iter_mut().rev() {
            tmp = u32::from(*out) + (r << 16);
            if tmp != 0 {
                *out = (tmp / a0) as u16;
                r = tmp - u32::from(*out) * a0;
            } else {
                *out = 0;
            }
        }
        *rem = r as u16;
    }

    /// Calculates `a[i] -= b[i]`, assuming that:
    /// - `a[i] >= b[i]`
    fn long_sub(a: &mut [u16], b: &[u16]) {
        let len = a.len().min(b.len());

        let mut carry = 0;
        for (minuend, subtrahend) in izip!(a.iter_mut(), b.iter()).take(len) {
            carry += i32::from(*minuend) - i32::from(*subtrahend);
            *minuend = carry as u16;
            carry >>= 16;
        }

        if a.len() > len {
            for minuend in a[len..].iter_mut() {
                carry += i32::from(*minuend);
                *minuend = carry as u16;
                carry >>= 16;
            }
        }

        // carry != 0 indicates subtraction underflow, e.g. b > a.
        debug_assert!(carry == 0);
    }

    /// Returns `true` if `a >= b`.
    /// Comparison is done in 16 bit steps starting with the MSB.
    fn long_compare(a: &[u16], b: &[u16]) -> bool {
        let mut i = a.len().min(b.len());

        while i != 0 {
            i -= 1;
            if a[i] != b[i] {
                break;
            }
        }

        a[i] >= b[i]
    }
}

impl Default for TsdData {
    fn default() -> Self {
        Self::new()
    }
}
