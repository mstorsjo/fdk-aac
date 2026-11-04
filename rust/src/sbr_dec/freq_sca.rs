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
//! Frequency scale calculation

use super::common;
use super::{common::SbrError, sr_mapping::*};
use crate::common::bitstream::Bitstream;
use crate::sbr_dec::constants::*;
use crate::sbr_dec::flags::*;
use itertools::izip;
use std::cmp::min;

/// Start and stop subbands of the highband
const START_FREQ_16: [[u8; 16]; 2] = [
    [
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    ],
    [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
];

const START_FREQ_22: [[u8; 16]; 2] = [
    [
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 28, 30,
    ],
    [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22],
];

const START_FREQ_24: [[u8; 16]; 2] = [
    [
        11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 32,
    ],
    [3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 19, 21, 24],
];

const START_FREQ_32: [[u8; 16]; 2] = [
    [
        10, 12, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 32,
    ],
    [2, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 19, 21, 24],
];

const START_FREQ_40: [[u8; 16]; 2] = [
    [
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 26, 28, 30, 32,
    ],
    [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 19, 21, 23, 25],
];

const START_FREQ_44: [[u8; 16]; 2] = [
    [
        8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 23, 25, 28, 32,
    ],
    [2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 17, 19, 22, 26],
];

const START_FREQ_48: [[u8; 16]; 2] = [
    [7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 27, 31],
    [1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 21, 25],
];

const START_FREQ_64: [[u8; 16]; 2] = [
    [6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 21, 23, 26, 30],
    [1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 21, 25],
];

const START_FREQ_88: [[u8; 16]; 2] = [
    [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 23, 27, 31],
    [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 20, 24, 28],
];

const START_FREQ_192: [u8; 16] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 19, 23, 27];

const START_FREQ_176: [u8; 16] = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 20, 24, 28];

const START_FREQ_128: [u8; 16] = [1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 21, 25];

const MAX_OCTAVE: usize = 29;
const MAX_SECOND_REGION: usize = 50;

const STOP_FREQ_BORDER_0: u32 = 13;
const STOP_FREQ_BORDER_1: u32 = 14;

const FREQ_SCALE_12_BANDS_PER_OCTAVE: u32 = 12;
const FREQ_SCALE_10_BANDS_PER_OCTAVE: u32 = 10;
const FREQ_SCALE_08_BANDS_PER_OCTAVE: u32 = 8;

/// Used to signalize an error
const FREQ_SCALE_ERROR: u8 = 255;

/// Inverse warp factor
const INV_WARP_FACTOR: f32 = 1.0 / 1.3;

/// Weighting factor
const WEIGHT_FACTOR_1K: u32 = 1000;

/// Threshold 2.245 weighted by weighting factor
const THR_MORE_REGIONS: u32 = (2.245 * WEIGHT_FACTOR_1K as f32) as u32;

const MAX_FREQ_COEFFS_FS44100: u8 = 35;
const MAX_FREQ_COEFFS_FS48000: u8 = 32;

const LO: usize = 0;
const HI: usize = 1;

#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq)]
enum SbrRate {
    Dual,
    Quad,
}

/// Structure representing Frequency Band Data Header
#[repr(C)]
#[derive(Debug, Default, Clone, Copy, PartialEq)]
pub struct FreqBandDataHeader {
    start_freq: u8,    // Index for SBR start frequency
    stop_freq: u8,     // Index for SBR highest frequency
    freq_scale: u8,    // 0: linear scale,  1-3 logarithmic scales
    alter_scale: bool, // Flag for coarser frequency resolution
    noise_bands: u8,   // Noise bands per octave, read from bitstream
}

/// Structure representing Frequency Band Data
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FreqBandData {
    num_freq_bands_sbr: [u8; 2], // Number of SBR-bands for low and high freq-resolution
    num_freq_bands_noise: u8,    // Actual number of noise bands to read from the bitstream
    pub(super) num_master: u8,   // Number of SBR-bands in master_band_table
    pub start_subband: u8,       // QMF-band where SBR frequency range starts
    pub end_subband: u8,         // QMF-band where SBR frequency range ends
    pub(super) old_end_subband: u8, /* if headerchange applies this value holds the old highband
                                 value -> highband value of overlap area; required for
                                 overlap in usac when headerchange occurs between XVAR and
                                 VARX frame */
    freq_band_table_low: [u8; MAX_FREQ_COEFFS / 2 + 1], /* Mapping of SBR bands to QMF bands
                                                         * for low frequency resolution */
    freq_band_table_high: [u8; MAX_FREQ_COEFFS + 1], /* Mapping of SBR bands to QMF bands for
                                                      * high frequency resolution */
    freq_band_table_noise: [u8; MAX_NOISE_COEFFS + 1], // Mapping of SBR noise bands to QMF bands
    master_band_table: [u8; MAX_FREQ_COEFFS + 1],      /* Master Band Table from which
                                                        * freq_band_table is derived */
}

impl Default for FreqBandData {
    fn default() -> Self {
        FreqBandData {
            num_freq_bands_sbr: [0; 2],
            num_freq_bands_noise: 0,
            num_master: 0,
            start_subband: 0,
            end_subband: 0,
            old_end_subband: 0,
            freq_band_table_low: [0; MAX_FREQ_COEFFS / 2 + 1],
            freq_band_table_high: [0; MAX_FREQ_COEFFS + 1],
            freq_band_table_noise: [0; MAX_NOISE_COEFFS + 1],
            master_band_table: [0; MAX_FREQ_COEFFS + 1],
        }
    }
}

impl FreqBandData {
    /// Returns the number of frequency bands for SBR at the given index.
    ///
    /// # Arguments
    ///
    /// * `index` - The index of the frequency band.
    ///
    /// # Returns
    ///
    /// * `u8` - The number of frequency bands for SBR at the specified index.
    pub(super) fn num_freq_bands_sbr(&self, index: usize) -> u8 {
        self.num_freq_bands_sbr[index]
    }

    /// Returns the number of frequency bands for noise.
    ///
    /// # Returns
    ///
    /// * `u8` - The number of frequency bands for noise.
    pub(super) fn num_freq_bands_noise(&self) -> u8 {
        self.num_freq_bands_noise
    }

    pub(super) fn freq_band_table_low(&self) -> &[u8] {
        &self.freq_band_table_low
    }

    pub(super) fn freq_band_table_high(&self) -> &[u8] {
        &self.freq_band_table_high
    }

    pub(super) fn freq_band_table_noise(&self) -> &[u8] {
        &self.freq_band_table_noise
    }

    pub(super) fn master_band_table(&self) -> &[u8] {
        &self.master_band_table
    }

    pub(super) fn freq_band_table_low_mut(&mut self) -> &mut [u8] {
        &mut self.freq_band_table_low
    }

    pub(super) fn freq_band_table_high_mut(&mut self) -> &mut [u8] {
        &mut self.freq_band_table_high
    }

    pub(super) fn freq_band_table_noise_mut(&mut self) -> &mut [u8] {
        &mut self.freq_band_table_noise
    }

    /// Reset frequency band tables
    ///
    /// # Return
    ///
    /// SbrError, SbrError::OK if successful
    pub fn reset_freq_band_tables(
        &mut self,
        freq_band_header_data: &FreqBandDataHeader,
        crossover_band: u8,
        number_of_analysis_bands: u8,
        proc_sample_rate: u32,
        flags: SbrDecFlags,
    ) -> SbrError {
        let mut _err = SbrError::Ok;
        let mut num_bands_low: u8 = 0;
        let mut num_bands_high: u8 = 0;

        // Calculate master frequency table
        if update_freq_scale(
            &mut self.master_band_table,
            &mut self.num_master,
            freq_band_header_data,
            proc_sample_rate,
            flags,
        ) != SbrError::Ok
            || crossover_band > self.num_master
        {
            return SbrError::UnsupportedConfig;
        }

        // Derive high resolution from master frequency table
        update_high_res(
            &mut self.freq_band_table_high,
            &mut num_bands_high,
            &mut self.master_band_table,
            self.num_master,
            crossover_band,
        );
        // Derive low resolution from high resolution
        update_low_res(
            &mut self.freq_band_table_low,
            &mut num_bands_low,
            &self.freq_band_table_high,
            num_bands_high,
        );

        // Check index to freq_band_table[LOW_RES]
        if num_bands_low == 0
            || num_bands_low
                > if number_of_analysis_bands == 16 {
                    MAX_FREQ_COEFFS_QUAD_RATE
                } else {
                    MAX_FREQ_COEFFS_DUAL_RATE
                } / 2
        {
            return SbrError::UnsupportedConfig;
        }

        self.num_freq_bands_sbr[LO] = num_bands_low;
        self.num_freq_bands_sbr[HI] = num_bands_high;

        let lower_subband = self.freq_band_table_low[0];
        let upper_subband = self.freq_band_table_low[usize::from(num_bands_low)];

        // Check for start frequency border k_x
        if (lower_subband
            > if flags.contains(SbrDecFlags::QUAD_RATE) {
                16
            } else {
                SBRDEC_MAX_ANALYSIS_CHANNELS as u8
            })
            || lower_subband >= upper_subband
        {
            return SbrError::UnsupportedConfig;
        }

        // Calculate number of noise bands
        let stop_band = self.freq_band_table_high[usize::from(num_bands_high)];
        let kx = self.freq_band_table_high[0];

        if freq_band_header_data.noise_bands == 0 {
            self.num_freq_bands_noise = 1;
        } else {
            // Calculate no of noise bands 1,2 or 3 bands/octave
            let num_octaves = ((f32::from(stop_band)).log2() - (f32::from(kx)).log2())
                * f32::from(freq_band_header_data.noise_bands);
            let int_temp: u8 = (num_octaves + 0.5) as u8;

            if int_temp > MAX_NOISE_COEFFS as u8 {
                return SbrError::UnsupportedConfig;
            }

            self.num_freq_bands_noise = int_temp.max(1);
        }

        // Get noise bands
        downsample_low_res(
            &mut self.freq_band_table_noise,
            self.num_freq_bands_noise,
            &self.freq_band_table_low,
            num_bands_low,
        );

        // Save old highband; required for overlap in usac when header change occurs at XVAR and
        // VARX frame
        self.old_end_subband = self.end_subband;

        self.start_subband = lower_subband;
        self.end_subband = upper_subband;

        SbrError::Ok
    }

    // Setters
    pub(super) fn set_num_freq_bands_noise(&mut self, bands: u8) {
        self.num_freq_bands_noise = bands;
    }

    pub(super) fn set_num_freq_bands_sbr(&mut self, sbr_bands: [u8; 2]) {
        self.num_freq_bands_sbr = sbr_bands;
    }

    pub(super) fn set_start_subband(&mut self, subband: u8) {
        self.start_subband = subband;
    }

    pub(super) fn set_end_subband(&mut self, subband: u8) {
        self.end_subband = subband;
    }

    pub(super) fn set_old_end_subband(&mut self, subband: u8) {
        self.old_end_subband = subband;
    }
}

impl FreqBandDataHeader {
    /// Sets the default values for the frequency band data header based
    /// on the given sample rate and downscale factor.
    pub(super) fn set_freq_band_data_header_default(
        &mut self,
        sample_rate: u32,
        downscale_factor: u8,
    ) {
        // For ELD reduced delay bitstreams sample_rates initializing of the sbr
        // decoder instance fails if freq_scale is set to FREQ_SCALE_DEFAULT
        // because no master table can be generated; In ELD reduced delay bitstreams
        // this value is always 0; gets overwritten when header is read.

        self.start_freq = 5;
        self.stop_freq = 0;
        self.freq_scale = 0;

        self.alter_scale = ALTER_SCALE_DEFAULT;
        self.noise_bands = NOISE_BANDS_DEFAULT;

        if sample_rate * u32::from(downscale_factor) >= 96000 {
            self.start_freq = 4;
            self.stop_freq = 3;
        } else if sample_rate * u32::from(downscale_factor) > 24000 {
            self.start_freq = 7;
            self.stop_freq = 3;
        }
    }

    /// Compares the current frequency band data header with another header to check for
    /// differences.
    ///
    /// # Return
    ///
    /// - `true` if there are differences between the headers.
    /// - `false` if the headers are identical.
    pub(super) fn cmp_freq_band_data_header(&self, header_2: &FreqBandDataHeader) -> bool {
        self.start_freq != header_2.start_freq
            || self.stop_freq != header_2.stop_freq
            || self.freq_scale != header_2.freq_scale
            || self.alter_scale != header_2.alter_scale
            || self.noise_bands != header_2.noise_bands
    }

    /// Reads the frequency band data header from a bitstream.
    pub(super) fn read_freq_band_data_header(&mut self, bs: &mut Bitstream) {
        self.start_freq = bs.read(SI_SBR_START_FREQ_BITS) as u8;
        self.stop_freq = bs.read(SI_SBR_STOP_FREQ_BITS) as u8;
    }

    /// Reads the extra frequency band data header from a bitstream if the header is marked as
    /// extra.
    pub(super) fn read_freq_band_data_header_extra(
        &mut self,
        bs: &mut Bitstream,
        is_header_extra: bool,
    ) {
        if is_header_extra {
            self.freq_scale = bs.read(SI_SBR_FREQ_SCALE_BITS).try_into().unwrap();
            self.alter_scale = bs.read(SI_SBR_ALTER_SCALE_BITS) != 0;
            self.noise_bands = bs.read(SI_SBR_NOISE_BANDS_BITS).try_into().unwrap();
        } else {
            self.freq_scale = FREQ_SCALE_DEFAULT;
            self.alter_scale = ALTER_SCALE_DEFAULT;
            self.noise_bands = NOISE_BANDS_DEFAULT;
        }
    }
}

/// Retrieve QMF-band where the SBR range starts
///
/// Convert start_freq which was read from the bitstream into a QMF-channel number.
///
/// # Return
///
/// Number of start band
fn get_start_band(
    fs: u32,                        //Output sampling frequency
    start_freq: u8,                 // Index to table of possible start bands
    header_data_flags: SbrDecFlags, // Info to SBR mode
) -> u8 {
    let mut fs_mapped: u32 = fs;
    let mut rate: usize = SbrRate::Dual as usize;

    if header_data_flags.contains(SbrDecFlags::SYNTAX_USAC) {
        if header_data_flags.contains(SbrDecFlags::QUAD_RATE) {
            rate = SbrRate::Quad as usize;
        }
        fs_mapped = map_to_std_sample_rate(fs, true);
    }

    debug_assert!(2 * (rate + 1) <= MAX_RATE);

    let start_freq: usize = usize::from(start_freq);

    let band: u8 = match fs_mapped {
        192000 => START_FREQ_192[start_freq],
        176400 => START_FREQ_176[start_freq],
        128000 => START_FREQ_128[start_freq],
        96000 | 88200 => START_FREQ_88[rate][start_freq],
        64000 => START_FREQ_64[rate][start_freq],
        48000 => START_FREQ_48[rate][start_freq],
        44100 => START_FREQ_44[rate][start_freq],
        40000 => START_FREQ_40[rate][start_freq],
        32000 => START_FREQ_32[rate][start_freq],
        24000 => START_FREQ_24[rate][start_freq],
        22050 => START_FREQ_22[rate][start_freq],
        16000 => START_FREQ_16[rate][start_freq],
        _ => FREQ_SCALE_ERROR,
    };

    band
}

/// Retrieve QMF-band where the SBR range stops
///
/// Convert stop_freq which was read from the bitstream into a QMF-channel number.
///
/// # Return
///
/// Number of stop band
fn get_stop_band(
    fs: u32,
    stop_freq: u32,
    header_data_flags: SbrDecFlags,
    start_freq_index: u8,
) -> u8 {
    // let mut band: u8;
    let stop_min: u32;
    let stop_min_lim: u8;

    let mut diff_0: [u8; STOP_FREQ_BORDER_0 as usize] = [0; 13];
    let mut diff_1: [u8; STOP_FREQ_BORDER_1 as usize] = [0; 14];

    let mut num: u32 = 2 * 64;
    const STOP_MAX: u32 = 64;

    let mut band = match stop_freq {
        s if s < STOP_FREQ_BORDER_1 => {
            if header_data_flags.contains(SbrDecFlags::QUAD_RATE) {
                num /= 2;
            }

            if fs < 32000 {
                stop_min = ((2 * 6000 * num) / fs).div_ceil(2);
            } else if fs < 64000 {
                stop_min = ((2 * 8000 * num) / fs).div_ceil(2);
            } else {
                stop_min = ((2 * 10000 * num) / fs).div_ceil(2);
            }

            stop_min_lim = min(stop_min, STOP_MAX) as u8;

            // Choose a stop band between k1 and 64 depending on stop_freq (0..13), based on a
            // logarithmic scale. The arrays diff0 and diff1 are used temporarily here.

            calc_bands(&mut diff_0, stop_min_lim, 64, STOP_FREQ_BORDER_0 as u8);
            common::shell_sort(&mut diff_0);
            cum_sum(
                stop_min_lim,
                &mut diff_0,
                STOP_FREQ_BORDER_0 as usize,
                &mut diff_1,
            );
            diff_1[usize::try_from(stop_freq).unwrap()]
        }
        s if s == STOP_FREQ_BORDER_1 => 2 * start_freq_index,
        _ => 3 * start_freq_index, // stopFreq == STOP_FREQ_BORDER_2
    };

    // Limit to Nyquist
    band = min(band, 64);

    // Range checks
    // 1 <= difference <= 48; 1 <= fs <= 96000
    let max_freq_coeff: u32 = if header_data_flags.contains(SbrDecFlags::QUAD_RATE) {
        MAX_FREQ_COEFFS_QUAD_RATE as u32
    } else {
        MAX_FREQ_COEFFS as u32
    };

    if (band as i16 - start_freq_index as i16 > max_freq_coeff as i16) || (band <= start_freq_index)
    {
        return FREQ_SCALE_ERROR;
    }

    if header_data_flags.contains(SbrDecFlags::QUAD_RATE) {
        return band; // skip other checks: (band - start_freq_index) must be
                     // <= MAX_FREQ_COEFFS_QUAD_RATE for all fs
    }
    if header_data_flags.contains(SbrDecFlags::SYNTAX_USAC) {
        // 1 <= difference <= 35; 42000 <= fs <= 96000
        if (fs >= 42000) && ((band - start_freq_index) > MAX_FREQ_COEFFS_FS44100) {
            return FREQ_SCALE_ERROR;
        }
        // 1 <= difference <= 32; 46009 <= fs <= 96000
        if (fs >= 46009) && ((band - start_freq_index) > MAX_FREQ_COEFFS_FS48000) {
            return FREQ_SCALE_ERROR;
        }
    } else {
        // 1 <= difference <= 35; fs == 44100
        if (fs == 44100) && ((band - start_freq_index) > MAX_FREQ_COEFFS_FS44100) {
            return FREQ_SCALE_ERROR;
        }
        // 1 <= difference <= 32; 48000 <= fs <= 96000
        if (fs >= 48000) && ((band - start_freq_index) > MAX_FREQ_COEFFS_FS48000) {
            return FREQ_SCALE_ERROR;
        }
    }

    band
}

/// Generates master frequency tables
///
/// Frequency tables are calculated according to the selected domain
/// (linear/logarithmic) and granularity.
/// IEC 14496-3 4.6.18.3.2.1
///
/// # Return
///
/// SbrError, SbrError::OK if successful
fn update_freq_scale(
    master_band_table: &mut [u8],               // Master table to be created
    num_master: &mut u8,                        // Number of entries in master table
    freq_band_header_data: &FreqBandDataHeader, // SBR frequency band header data from bitstream
    fs: u32,                                    // SBR working sampling rate
    flags: SbrDecFlags,                         // SBR decoder flags
) -> SbrError {
    let mut sample_rate: u32 = fs;

    if flags.contains(SbrDecFlags::QUAD_RATE) {
        sample_rate /= 2;
    }

    // Determine start band
    let start_band = get_start_band(sample_rate, freq_band_header_data.start_freq, flags);
    if start_band == FREQ_SCALE_ERROR {
        return SbrError::UnsupportedConfig;
    }

    // Determine stop band
    let stop_band = get_stop_band(
        sample_rate,
        freq_band_header_data.stop_freq.into(),
        flags,
        start_band,
    );
    if stop_band == FREQ_SCALE_ERROR {
        return SbrError::UnsupportedConfig;
    }

    if freq_band_header_data.freq_scale > 0 {
        // Bark
        let mut bands_per_octave: u8;
        let mut diff0: [u8; MAX_OCTAVE] = [0; MAX_OCTAVE];
        let mut diff1: [u8; MAX_SECOND_REGION] = [0; MAX_SECOND_REGION];
        let more_regions =
            WEIGHT_FACTOR_1K * u32::from(stop_band) > THR_MORE_REGIONS * u32::from(start_band);

        bands_per_octave = match freq_band_header_data.freq_scale {
            1 => FREQ_SCALE_12_BANDS_PER_OCTAVE as u8,
            2 => FREQ_SCALE_10_BANDS_PER_OCTAVE as u8,
            _ => FREQ_SCALE_08_BANDS_PER_OCTAVE as u8,
        };

        // Ref: ISO/IEC 23003-3, Figure 12 - Flowchart calculation of fMaster for 4:1 system when
        // bs_freq_scale > 0
        if flags.contains(SbrDecFlags::QUAD_RATE) && start_band < bands_per_octave {
            bands_per_octave = 2 * (start_band / 2);
        }

        let k1: u8 = if more_regions {
            2 * start_band
        } else {
            stop_band
        };

        let num_bands0 = number_of_bands(bands_per_octave, start_band, k1, false);
        debug_assert!(num_bands0 <= MAX_OCTAVE as u8);

        if num_bands0 < 1 {
            return SbrError::UnsupportedConfig;
        }

        calc_bands(&mut diff0, start_band, k1, num_bands0);
        common::shell_sort(&mut diff0[0..num_bands0.into()]);

        if diff0[0] == 0 {
            return SbrError::UnsupportedConfig;
        }

        cum_sum(
            start_band,
            &mut diff0,
            usize::from(num_bands0),
            master_band_table,
        );
        *num_master = num_bands0;

        if more_regions {
            let num_bands1 = number_of_bands(
                bands_per_octave,
                k1,
                stop_band,
                freq_band_header_data.alter_scale,
            );
            debug_assert!(num_bands1 <= MAX_SECOND_REGION as u8);

            if num_bands1 < 1 {
                return SbrError::UnsupportedConfig;
            }

            calc_bands(&mut diff1, k1, stop_band, num_bands1);
            common::shell_sort(&mut diff1[0..num_bands1.into()]);

            if diff0[usize::from(num_bands0) - 1] > diff1[0] {
                modify_bands(diff0[usize::from(num_bands0) - 1], &mut diff1, num_bands1);
            }

            // Add 2nd region
            cum_sum(
                k1,
                &mut diff1,
                usize::from(num_bands1),
                &mut master_band_table[num_bands0.into()..],
            );
            *num_master += num_bands1;
        }
    } else {
        // Linear mode
        let mut i: usize = 0;
        let rnd: f32;
        let dk: u8;
        let mut stop_band_diff: i8;
        let mut incr: i8 = 0;
        let mut diff: [u8; MAX_OCTAVE + MAX_SECOND_REGION] = [0; MAX_OCTAVE + MAX_SECOND_REGION];

        if !freq_band_header_data.alter_scale {
            dk = 1;
            rnd = 0.0; // FLOOR to get to few number of bands (next lower even number)
        } else {
            dk = 2;
            rnd = 0.5; // ROUND to the closest fit
        }
        let num_bands: u8 = 2 * (((stop_band - start_band) as f32 / (dk as f32 * 2.0) + rnd) as u8);

        if num_bands < 1 {
            return SbrError::UnsupportedConfig;
            // We must return already here because 'i' can become negative below.
        }

        stop_band_diff = stop_band as i8 - (start_band + num_bands * dk) as i8;

        diff[..usize::from(num_bands)].fill(dk);

        // If linear scale wasn't achieved
        // and we got too wide SBR area
        if stop_band_diff < 0 {
            incr = 1;
            i = 0;
        }

        // If linear scale wasn't achieved
        // and we got too small SBR area
        if stop_band_diff > 0 {
            incr = -1;
            i = usize::from(num_bands) - 1;
        }

        // Adjust diff vector to get spec. SBR range
        while stop_band_diff != 0 {
            diff[i] = u8::try_from(diff[i] as i8 - incr).unwrap();
            stop_band_diff += incr;
            i = usize::try_from(i as i8 + incr).unwrap();
        }

        cum_sum(start_band, &mut diff, num_bands.into(), master_band_table);
        *num_master = num_bands;
    }

    if *num_master < 1 {
        return SbrError::UnsupportedConfig;
    }

    // Ref: ISO/IEC 23003-3 Cor.3, "In 7.5.5.2, add to the requirements:"
    if flags.contains(SbrDecFlags::QUAD_RATE) {
        for j in 1..*num_master {
            if (master_band_table[usize::from(j)] - master_band_table[usize::from(j) - 1]) as i32
                > start_band as i32 - 2
            {
                return SbrError::UnsupportedConfig;
            }
        }
    }

    SbrError::Ok
}

/// Calculate width of SBR bands
///
/// Given the desired number of bands within the SBR frequency range, this function calculates the
/// width of each SBR band in QMF channels. The bands get wider from start to stop (bark scale).
fn calc_bands(
    diff: &mut [u8], // Vector of widths to be calculated
    start: u8,       // Lower end of subband range
    stop: u8,        // Upper end of subband range
    num_bands: u8,   // Desired number of bands
) {
    let mut previous = stop;
    let mut exact: f32 = stop as f32;
    let band_factor: f32 = (start as f32 / stop as f32).powf(1.0 / num_bands as f32);

    for d in diff[..num_bands as usize].iter_mut().rev() {
        exact *= band_factor;

        // It is safe to use `as` here, because the following is always true:
        // - `start <= stop`
        // - `band_factor <= 1.0`
        // - both `start` and `stop` are `u8`
        // - `exact <= stop`,
        let current = (exact + 0.5) as u8;

        // Save width of band i
        *d = previous - current;
        previous = current;
    }
}

/// Calculate cumulated sum vector from delta vector
fn cum_sum(start_value: u8, diff: &mut [u8], length: usize, start_address: &mut [u8]) {
    start_address[0] = start_value;
    izip!(diff.iter(), start_address[1..].iter_mut())
        .take(length)
        .fold(start_value, |acc, (&d, sa)| {
            *sa = acc + d;
            *sa
        });
}

/// Calculate number of SBR bands between start and stop band
///
/// Given the number of bands per octave, this function calculates how many bands fit in the given
/// frequency range.  When the warp flag is set, the 'band density' is decreased by a factor of
/// 1/1.3
///
/// # Return
///
/// Number of bands
fn number_of_bands(bands_per_octave: u8, start: u8, stop: u8, warp: bool) -> u8 {
    let mut num_bands: f32 =
        ((stop as f32).log2() - (start as f32).log2()) * bands_per_octave as f32;
    if warp {
        num_bands *= INV_WARP_FACTOR;
    }

    2 * (num_bands * 0.5 + 0.5) as u8
}

/// Adapt width of frequency bands in the second region
///
/// If SBR spans more than 2 octaves, the upper part of a bark-frequency-scale is calculated
/// separately. This function tries to avoid that the second region starts with a band smaller
/// than the highest band of the first region.
fn modify_bands(max_band_previous: u8, diff: &mut [u8], length: u8) {
    let mut change: u8 = max_band_previous - diff[0];
    // Limit the change so that the last band cannot get narrower than the first one

    change = change.min((diff[usize::from(length) - 1] - diff[0]) / 2);

    diff[0] += change;
    diff[usize::from(length) - 1] -= change;
    common::shell_sort(&mut diff[0..usize::from(length)]);
}

/// Update high resolution frequency band table
fn update_high_res(
    freq_band_table_high: &mut [u8; MAX_FREQ_COEFFS + 1],
    num_high_res: &mut u8,
    master_band_table: &mut [u8; MAX_FREQ_COEFFS + 1],
    num_bands: u8,
    crossover_band: u8,
) {
    *num_high_res = num_bands - crossover_band;
    freq_band_table_high[0..usize::from(num_bands + 1 - crossover_band)].copy_from_slice(
        &master_band_table[usize::from(crossover_band)
            ..usize::from(crossover_band + (num_bands + 1 - crossover_band))],
    );
}

/// Build low resolution table out of high resolution table
fn update_low_res(
    freq_band_table_low: &mut [u8; MAX_FREQ_COEFFS / 2 + 1],
    num_low_res: &mut u8,
    freq_band_table_high: &[u8; MAX_FREQ_COEFFS + 1],
    num_high_res: u8,
) {
    let offset = if num_high_res & 1 == 0 {
        // If even number of high_res bands
        0
    } else {
        // Odd number of high_res, which means xover is odd
        1
    };
    *num_low_res = (num_high_res + offset) / 2;

    // Use low_res=high_res[0,1,3,5 ...]
    // Use every second low_res=high_res[0,2,4...]
    for i in offset..=*num_low_res {
        freq_band_table_low[0] = freq_band_table_high[0];
        freq_band_table_low[usize::from(i)] =
            freq_band_table_high[usize::from(i) * 2 - usize::from(offset)];
    }
}

/// Derive a low-resolution frequency-table from the master frequency table
fn downsample_low_res(result: &mut [u8], num_result: u8, freq_band_table_ref: &[u8], num_ref: u8) {
    let mut i: usize = 0;
    let mut step: u8;
    let mut org_length: u8;
    let mut result_length: u8;
    let mut index: [u8; MAX_FREQ_COEFFS / 2] = [0; MAX_FREQ_COEFFS / 2];

    // init
    org_length = num_ref;
    result_length = num_result;

    index[0] = 0; // Always use left border

    while org_length > 0 {
        // Create downsample vector
        i += 1;
        step = org_length / result_length;
        org_length -= step;
        result_length -= 1;
        index[i] = index[i - 1] + step;
    }

    debug_assert!(org_length == 0);

    for j in 0..=i {
        // Use downsample vector to index Low Resolution vector
        result[j] = freq_band_table_ref[usize::from(index[j])];
    }
}
