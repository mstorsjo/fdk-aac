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
//! Mid/side (M/S) coding

use itertools::izip;

use super::{
    channel_info::{BlockType, IcsInfo},
    constants,
    huff_dec::HuffmanDecoder,
};
use crate::common::{bitstream::Bitstream, enums::WindowShape, flags::ACFlags};

const JOINTSTEREO_MAX_GROUPS: usize = 8;
const JOINTSTEREO_MAX_BANDS: usize = 64;
const SFB_PER_PRED_BAND: usize = 2;

// MDST filter coefficients for current window
static MDST_FILT_COEF_CURR_FL: [[f32; 3]; 20] = [
    [0.000000_f32, 0.000000_f32, 0.500000_f32], // only long / eight short  l:sine  r:sine
    [0.091497_f32, 0.000000_f32, 0.581427_f32], // l:kbd  r:kbd
    [0.045748_f32, 0.057238_f32, 0.540714_f32], // l:sine r:kbd
    [0.045748_f32, -0.057238_f32, 0.540714_f32], // l:kbd  r:sine
    [0.102658_f32, 0.103791_f32, 0.567149_f32], // long start
    [0.150512_f32, 0.047969_f32, 0.608574_f32],
    [0.104763_f32, 0.105207_f32, 0.567861_f32],
    [0.148406_f32, 0.046553_f32, 0.607863_f32],
    [0.102658_f32, -0.103791_f32, 0.567149_f32], // long stop
    [0.150512_f32, -0.047969_f32, 0.608574_f32],
    [0.148406_f32, -0.046553_f32, 0.607863_f32],
    [0.104763_f32, -0.105207_f32, 0.567861_f32],
    [0.205316_f32, 0.000000_f32, 0.634298_f32], // stop start
    [0.209526_f32, 0.000000_f32, 0.635722_f32],
    [0.207421_f32, 0.001416_f32, 0.635010_f32],
    [0.207421_f32, -0.001416_f32, 0.635010_f32],
    [0.185618_f32, 0.000000_f32, 0.627371_f32], // stop start Transform Splitting
    [0.204932_f32, 0.000000_f32, 0.634159_f32],
    [0.194609_f32, 0.006202_f32, 0.630536_f32],
    [0.194609_f32, -0.006202_f32, 0.630536_f32],
];

// MDST filter coefficients for previous window
static MDST_FILT_COEF_PREV_FL: [[f32; 4]; 6] = [
    // only long / long start / eight short  l:sine
    [0.000000_f32, 0.106103_f32, 0.250000_f32, 0.318310_f32],
    // l:kbd
    [0.059509_f32, 0.123714_f32, 0.186579_f32, 0.213077_f32],
    // long stop / stop start  l:sine
    [0.038498_f32, 0.039212_f32, 0.039645_f32, 0.039790_f32],
    // l:kbd
    [0.026142_f32, 0.026413_f32, 0.026577_f32, 0.026631_f32],
    // Transform splitting l:sine
    [0.069608_f32, 0.075028_f32, 0.078423_f32, 0.079580_f32],
    // l:kbd
    [0.042172_f32, 0.043458_f32, 0.044248_f32, 0.044514_f32],
];

static INDICES_1: [u8; 6] = [2, 1, 0, 1, 2, 3];
static INDICES_2: [u8; 6] = [1, 0, 0, 2, 3, 4];
static INDICES_3: [u8; 6] = [0, 0, 1, 3, 4, 5];

static SUBTR_1: [u8; 6] = [6, 5, 4, 2, 1, 1];
static SUBTR_2: [u8; 6] = [5, 4, 3, 1, 1, 2];
static SUBTR_3: [u8; 6] = [4, 3, 2, 1, 2, 3];

/// Joint stereo data
#[repr(C)]
#[derive(Debug)]
pub struct JointStereoData {
    ms_mask_present: u8,
    ms_used: [u8; JOINTSTEREO_MAX_BANDS], // Each item contains flags for up to 8 groups.
    // And also utilized for complex stereo prediction.
    cplx_pred_flag: bool, // stereo complex prediction was signalled for this frame
    max_sfb: usize,
    complex_prediction: Option<Box<ComplexPredictionData>>,
}

impl Default for JointStereoData {
    fn default() -> Self {
        JointStereoData {
            ms_mask_present: 0,
            ms_used: [0; JOINTSTEREO_MAX_BANDS],
            cplx_pred_flag: false,
            max_sfb: 0,
            complex_prediction: None,
        }
    }
}

impl JointStereoData {
    /// Returns `JointStereoData` instance
    ///
    /// # Parameters
    ///
    /// - `is_cp_possible`: Indicates whether complex prediction tool shall be allocated
    pub fn new(is_cp_possible: bool) -> JointStereoData {
        let mut join_stereo_data = JointStereoData::default();
        if is_cp_possible {
            join_stereo_data.complex_prediction = Some(Box::new(ComplexPredictionData::new()));
        }

        join_stereo_data
    }

    /// Resets JointStereoData
    pub fn reset(&mut self) {
        self.ms_mask_present = 0;
        self.ms_used[..].fill(0);
        self.cplx_pred_flag = false;
        self.max_sfb = 0;
    }

    /// Returns ms_used[] slice from JointStereoData
    /// Each item contains flags for up to 8 groups.
    // And also utilized for complex stereo prediction
    pub fn ms_used(&self) -> &[u8] {
        &self.ms_used
    }

    /// Returns ms_used[] slice from JointStereoData
    pub fn ms_used_mut(&mut self) -> &mut [u8] {
        &mut self.ms_used
    }

    /// Returns value of ms_mask_present from JointStereoData
    pub fn ms_mask_present(&self) -> u8 {
        self.ms_mask_present
    }

    /// Sets previous window sequence in ComplexPredictionData
    pub fn set_window_sequence(&mut self, window_sequence: BlockType) {
        if let Some(complex_prediction) = &mut self.complex_prediction {
            complex_prediction.win_seq_prev = window_sequence;
        }
    }

    /// Sets previous window shape in ComplexPredictionData
    pub fn set_window_shape(&mut self, window_shape: WindowShape) {
        if let Some(complex_prediction) = &mut self.complex_prediction {
            complex_prediction.win_shape_prev = window_shape;
        }
    }

    /// Sets complex prediction interruption
    pub fn set_cp_interruption(&mut self) {
        if let Some(complex_prediction) = &mut self.complex_prediction {
            complex_prediction.set_interruption();
        }
    }

    /// Reads joint stereo data from bitstream
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `max_sfb`: Maximum number of scale factor bands transmitted
    /// - `ac_flags`: Audio codec flags
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{channel_info::IcsInfo, ms_stereo::JointStereoData};
    /// use aac::common::{
    ///     bitstream::Bitstream,
    ///     bitstream::Mode,
    ///     flags::{self, ACFlags},
    /// };
    /// let mut jsd = JointStereoData::new(true);
    ///
    /// // Precondition, should have a valid Bitstream and IcsInfo instance
    /// // Refer respective components for instance creation, read/write methods
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 40);
    ///
    /// let ics_info = IcsInfo::new();
    /// let max_sfb = 64;
    /// let ac_flags = ACFlags::USAC;
    ///
    /// jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);
    /// ```
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        max_sfb: usize,
        ac_flags: ACFlags,
    ) -> i32 {
        let win_groups = ics_info.n_window_groups();
        self.max_sfb = max_sfb;
        self.ms_used[..max_sfb].fill(0_u8);
        self.ms_mask_present = bs.read(2) as u8;

        self.cplx_pred_flag = false;

        match self.ms_mask_present {
            // read ms_used
            1 => {
                for group in 0..win_groups {
                    self.ms_used
                        .iter_mut()
                        .take(max_sfb)
                        .for_each(|ms| *ms |= (bs.read_bit() << group) as u8);
                }
            }

            // full spectrum M/S, set all flags to 1
            2 => {
                self.ms_used[..max_sfb].fill(255_u8);
            }

            // complex stereo prediction
            3 => {
                if ac_flags.contains(ACFlags::USAC) && self.complex_prediction.is_some() {
                    if self.complex_prediction.as_mut().unwrap().read(
                        bs,
                        ics_info,
                        &mut self.ms_used,
                        max_sfb,
                        ac_flags,
                    ) != 0
                    {
                        return -1;
                    }
                    // M/S coding is disabled, complex stereo prediction is enabled
                    self.cplx_pred_flag = true;
                }
            }
            _ => (),
        } // match end

        if !self.cplx_pred_flag && self.complex_prediction.is_some() {
            self.complex_prediction.as_mut().unwrap().set_interruption();
        }

        0 // return 0 on success
    }

    /// Apply ms stereo to the given left and right channel spectrum
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectrum`: Spectrum coefficients of both left and right channels
    /// - `spectrum_prev`: Previous spectrum coefficients of both left and right channels
    /// - `scratch_buffer`: Scratch buffer with length of 2*MAX_FRAMESIZE
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{
    ///     channel_info::IcsInfo, constants::MAX_FRAMESIZE, ms_stereo::JointStereoData,
    /// };
    /// let mut jsd = JointStereoData::new(true);
    ///
    /// // Precondition, should have a valid IcsInfo instance
    /// // Refer respective components for instance creation, read/write methods
    /// let ics_info = IcsInfo::new();
    ///
    /// let mut spectrum = vec![0.0_f32; MAX_FRAMESIZE * 2];
    /// let spectrum_prev = vec![0.0_f32; MAX_FRAMESIZE * 2];
    /// let mut scratch_buffer = vec![0.0_f32; MAX_FRAMESIZE * 2];
    ///
    /// jsd.apply(
    ///     &ics_info,
    ///     &mut spectrum,
    ///     &spectrum_prev,
    ///     &mut scratch_buffer,
    /// );
    /// ```
    pub fn apply(
        &mut self,
        ics_info: &IcsInfo,
        spectrum: &mut [f32],
        spectrum_prev: &[f32],
        scratch_buffer: &mut [f32],
    ) {
        let sfb_offsets = ics_info.scale_factor_bands();
        let win_group_lengths = ics_info.window_group_lengths();

        let (spectrum_left, spectrum_right) = spectrum.split_at_mut(spectrum.len() / 2);
        let win_length = spectrum_right.len() / ics_info.windows_per_frame();

        if self.cplx_pred_flag {
            // complex stereo prediction
            if let Some(complex_prediction) = &mut self.complex_prediction {
                complex_prediction.apply(
                    ics_info,
                    &self.ms_used,
                    spectrum,
                    spectrum_prev,
                    self.max_sfb,
                    scratch_buffer,
                );
            }
        } else {
            // MS stereo
            let mut group_offset = 0;
            for (group, windows_per_group) in win_group_lengths
                .iter()
                .enumerate()
                .take(ics_info.n_window_groups())
            {
                let g_mask = 1 << group;
                for (left_spec, right_spec) in izip!(
                    spectrum_left[group_offset * win_length..].chunks_exact_mut(win_length),
                    spectrum_right[group_offset * win_length..]
                        .chunks_exact_mut(win_length)
                        .take(usize::from(*windows_per_group))
                ) {
                    for (ms, band_offsets) in self
                        .ms_used
                        .iter()
                        .zip(sfb_offsets.windows(2))
                        .take(self.max_sfb)
                    {
                        if *ms & g_mask != 0 {
                            let offset_currband = usize::from(band_offsets[0]);
                            let offset_nextband = usize::from(band_offsets[1]);

                            self.generate_ms_output(
                                &mut left_spec[offset_currband..],
                                &mut right_spec[offset_currband..],
                                offset_nextband - offset_currband,
                            );
                        }
                    }
                } // window - iterator
                group_offset += usize::from(*windows_per_group);
            } // group - iteartor

            // Reset ms_used flags if no explicit signaling was transmitted.
            if self.ms_mask_present == 2 {
                self.ms_used[..].fill(0);
            }
        }
    }

    /// Generate ms output channels from left and right spectrum
    ///
    /// # Parameters
    ///
    /// - `spec_left_currband`: left side spectrum bands
    /// - `spec_right_currband`: Right side spectrum bands
    /// - `n_sfb_bands`: Number of scale factor bands
    fn generate_ms_output(
        &self,
        spec_left_currband: &mut [f32],
        spec_right_currband: &mut [f32],
        n_sfb_bands: usize,
    ) {
        for (l, r) in spec_left_currband
            .chunks_exact_mut(4)
            .zip(spec_right_currband.chunks_exact_mut(4))
            .take(n_sfb_bands >> 2)
        {
            //  t = *l
            // *l += *r
            // *r = t - *r
            let mut tmp = l[0];
            l[0] += r[0];
            r[0] = tmp - r[0];

            tmp = l[1];
            l[1] += r[1];
            r[1] = tmp - r[1];

            tmp = l[2];
            l[2] += r[2];
            r[2] = tmp - r[2];

            tmp = l[3];
            l[3] += r[3];
            r[3] = tmp - r[3];
        }
    }
}

/// Complex prediction data
#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct ComplexPredictionData {
    cp_info: ComplexPredictionInfo,
    alpha_q_re_prev: [[i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
    alpha_q_im_prev: [[i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
    win_seq_prev: BlockType,
    win_shape_prev: WindowShape,
    win_groups_prev: u8,
    is_interrupted: bool,
}

impl Default for ComplexPredictionData {
    fn default() -> Self {
        ComplexPredictionData {
            cp_info: ComplexPredictionInfo::new(),
            alpha_q_re_prev: [[0; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
            alpha_q_im_prev: [[0; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
            win_seq_prev: BlockType::Long,
            win_shape_prev: WindowShape::Sine,
            win_groups_prev: 0,
            is_interrupted: false,
        }
    }
}

// Calculate downmix MDCT of current or previous frame
macro_rules! calc_downmix_mdct {
    (addition => $l:expr, $r:expr, $o:expr) => {
        $l.iter()
            .zip($r.iter())
            .zip($o.iter_mut())
            .for_each(|((&left, &right), out)| *out = (left + right) * 0.5_f32);
    };
    (subtraction => $l:expr, $r:expr, $o:expr) => {
        $l.iter()
            .zip($r.iter())
            .zip($o.iter_mut())
            .for_each(|((&left, &right), out)| *out = (left - right) * 0.5_f32);
    };
}

impl ComplexPredictionData {
    /// Returns `ComplexPredictionData` instance
    fn new() -> Self {
        ComplexPredictionData::default()
    }

    /// Sets complex prediction interruption
    fn set_interruption(&mut self) {
        self.is_interrupted = true;
        self.cp_info.reset();
    }

    /// Reads complex prediction data from bitstream
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `ms_used`: M/S stereo flags read from bitstream
    /// - `max_sfb`: Maximum number of scale factor bands transmitted
    /// - `ac_flags`: Audio codec flags
    fn read(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        ms_used: &mut [u8],
        max_sfb: usize,
        ac_flags: ACFlags,
    ) -> i32 {
        let win_groups = ics_info.n_window_groups();
        let win_sequence = ics_info.window_sequence();

        // cplx_pred_data()  cp. ISO/IEC FDIS 23003-3:2011(E)  Table 26
        if bs.read_bit() != 0 {
            for group in 0..win_groups {
                let g_mask = 1_u8 << group;
                ms_used
                    .iter_mut()
                    .take(max_sfb)
                    .for_each(|ms| *ms |= g_mask);
            }
        } else {
            for group in 0..win_groups {
                let g_mask = 1_u8 << group;
                // take (max_sfb + 1) to cover final element if max_sfb is odd.
                ms_used
                    .chunks_exact_mut(SFB_PER_PRED_BAND)
                    .take((max_sfb + 1) >> 1)
                    .for_each(|ms| {
                        ms[0] |= (bs.read_bit() << group) as u8;
                        ms[1] |= ms[0] & g_mask;
                    });
            }

            // correct the (last+1) element ms_used[max_sfb]
            // after iterator usage if max_sfb was odd, since the "for_each ||" overwrites one
            // additional index location.
            if max_sfb & 0x1 != 0 {
                ms_used[max_sfb] = 0;
            }
        }

        // get HUFFMAN_CODE_BOOK_SCL
        let huff_codebook = HuffmanDecoder::new(12);

        if (win_sequence == BlockType::Short && self.win_seq_prev != BlockType::Short)
            || (self.win_seq_prev == BlockType::Short && win_sequence != BlockType::Short)
            || self.is_interrupted
        {
            // memclear the previous complex prediction data
            self.alpha_q_re_prev[..].fill([0_i16; JOINTSTEREO_MAX_BANDS]);
            self.alpha_q_im_prev[..].fill([0_i16; JOINTSTEREO_MAX_BANDS]);
            self.is_interrupted = false;
        }

        // memclear the complex prediction data
        self.cp_info.alpha_q_re[..].fill([0_i16; JOINTSTEREO_MAX_BANDS]);
        self.cp_info.alpha_q_im[..].fill([0_i16; JOINTSTEREO_MAX_BANDS]);

        // 0 = mid->side prediction, 1 = side->mid prediction
        self.cp_info.is_side_to_mid_pred = bs.read_bit() != 0;
        self.cp_info.is_complex_coeff_transmitted = bs.read_bit() != 0;

        self.cp_info.is_use_prev_frame =
            if self.cp_info.is_complex_coeff_transmitted && !ac_flags.contains(ACFlags::INDEP) {
                bs.read_bit() != 0
            } else {
                false
            };

        let delta_code_time = if ac_flags.contains(ACFlags::INDEP) {
            false
        } else {
            bs.read_bit() != 0
        };

        let mut last_alpha_q_re;
        let mut last_alpha_q_im;

        for group in 0..win_groups {
            for band in (0..max_sfb).step_by(SFB_PER_PRED_BAND) {
                if delta_code_time {
                    if group > 0 {
                        last_alpha_q_re = self.cp_info.alpha_q_re[group - 1][band];
                        last_alpha_q_im = self.cp_info.alpha_q_im[group - 1][band];
                    } else if (win_sequence == BlockType::Short)
                        && (self.win_seq_prev == BlockType::Short)
                    {
                        // Included for error-robustness
                        if self.win_groups_prev == 0 {
                            return -1;
                        }
                        let win_group_prev_minus1 = usize::from(self.win_groups_prev - 1);
                        last_alpha_q_re = self.alpha_q_re_prev[win_group_prev_minus1][band];
                        last_alpha_q_im = self.alpha_q_im_prev[win_group_prev_minus1][band];
                    } else {
                        // this case covers first group only (i.e group is 0)
                        last_alpha_q_re = self.alpha_q_re_prev[group][band];
                        last_alpha_q_im = self.alpha_q_im_prev[group][band];
                    }
                } else {
                    // delta_code_time
                    if band > 0 {
                        last_alpha_q_re = self.cp_info.alpha_q_re[group][band - 1];
                        last_alpha_q_im = self.cp_info.alpha_q_im[group][band - 1];
                    } else {
                        last_alpha_q_re = 0;
                        last_alpha_q_im = 0;
                    }
                } // if delta_code_time

                if ms_used[band] & (1_u8 << group) != 0 {
                    let dpcm_alpha_re = -(huff_codebook.read_word(bs) - 60_i16);
                    self.cp_info.alpha_q_re[group][band] = dpcm_alpha_re + last_alpha_q_re;

                    if self.cp_info.is_complex_coeff_transmitted {
                        let dpcm_alpha_im = -(huff_codebook.read_word(bs) - 60_i16);
                        self.cp_info.alpha_q_im[group][band] = dpcm_alpha_im + last_alpha_q_im;
                    } else {
                        self.cp_info.alpha_q_im[group][band] = 0_i16;
                    }
                } else {
                    self.cp_info.alpha_q_re[group][band] = 0_i16;
                    self.cp_info.alpha_q_im[group][band] = 0_i16;
                } // if ms_used[band] & (1_u8 << group) != 0

                // cp. ISO_IEC_FDIS_23003-0(E)
                //  7.7.2.3.2 Decoding of prediction coefficients
                if (band + 1) <= max_sfb {
                    self.cp_info.alpha_q_re[group][band + 1] = self.cp_info.alpha_q_re[group][band];
                    self.cp_info.alpha_q_im[group][band + 1] = self.cp_info.alpha_q_im[group][band];
                }

                self.alpha_q_re_prev[group][band] = self.cp_info.alpha_q_re[group][band];
                self.alpha_q_im_prev[group][band] = self.cp_info.alpha_q_im[group][band];
            } // band loop

            // reset remaining band in the group
            self.cp_info.alpha_q_re[group][max_sfb..JOINTSTEREO_MAX_BANDS].fill(0);
            self.cp_info.alpha_q_im[group][max_sfb..JOINTSTEREO_MAX_BANDS].fill(0);
            self.alpha_q_re_prev[group][max_sfb..JOINTSTEREO_MAX_BANDS].fill(0);
            self.alpha_q_im_prev[group][max_sfb..JOINTSTEREO_MAX_BANDS].fill(0);
        } // group loop

        self.win_groups_prev = win_groups as u8;
        0 // return 0 on success
    }

    /// Apply complex prediction to the given left and right channel spectrum
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `ms_used`: Indicates the use of M/S stereo per scale factor band
    /// - `spectrum`: Spectrum coefficients of both left and right channels
    /// - `spectrum_prev`: Previous spectrum coefficients of both left and right channels
    /// - `max_sfb`: Maximum number of scale factor bands transmitted
    /// - `scratch_buffer`: Scratch buffer with length of 2*MAX_FRAMESIZE
    fn apply(
        &mut self,
        ics_info: &IcsInfo,
        ms_used: &[u8],
        spectrum: &mut [f32],
        spectrum_prev: &[f32],
        max_sfb: usize,
        scratch_buffer: &mut [f32],
    ) {
        const POINT_ONE: f32 = 0.1_f32;
        let sfb_offsets = ics_info.scale_factor_bands();
        let win_group_lengths = ics_info.window_group_lengths();
        let win_sequence = ics_info.window_sequence();

        let (spectrum_left, spectrum_right) = spectrum.split_at_mut(spectrum.len() / 2);
        let win_length = spectrum_right.len() / ics_info.windows_per_frame();

        let prev_shape = self.win_shape_prev as usize;
        let curr_shape = ics_info.window_shape();

        //  set pointer to filter-coefficients for MDST excitation including previous frame portions
        //  cp. ISO/IEC FDIS 23003-3:2011(E) table 125
        let mut coeff_prev = if self.cp_info.is_complex_coeff_transmitted {
            match win_sequence {
                BlockType::Short | BlockType::Long => &MDST_FILT_COEF_PREV_FL[prev_shape],
                BlockType::Start => {
                    if (self.win_seq_prev == BlockType::Short)
                        || (self.win_seq_prev == BlockType::Start)
                    {
                        // a stop-start-sequence can only follow on an eight-short-sequence or a
                        // start-sequence
                        &MDST_FILT_COEF_PREV_FL[2 + prev_shape]
                    } else {
                        &MDST_FILT_COEF_PREV_FL[prev_shape]
                    }
                }
                BlockType::Stop => &MDST_FILT_COEF_PREV_FL[2 + prev_shape],
            }
        } else {
            // default-position of coeff_prev
            &MDST_FILT_COEF_PREV_FL[prev_shape]
        };

        // define offset of pointer to filter-coefficients for MDST exitation
        // employing only the current frame
        let coeff_table_offset = match (self.win_shape_prev, curr_shape) {
            (WindowShape::Sine, WindowShape::Sine) => 0_usize,
            (WindowShape::Sine, WindowShape::KBD) => 2_usize,
            (WindowShape::KBD, WindowShape::KBD) => 1_usize,
            _ => 3_usize,
        };

        // set pointer to filter-coefficient table cp. ISO/IEC FDIS 23003-3:2011(E) table 124
        let mut coeff_curr = match ics_info.window_sequence() {
            BlockType::Short | BlockType::Long => &MDST_FILT_COEF_CURR_FL[coeff_table_offset],
            BlockType::Start => {
                if (self.win_seq_prev == BlockType::Short)
                    || (self.win_seq_prev == BlockType::Start)
                {
                    // a stop-start-sequence can only follow on an eight-short-sequence or a
                    // start-sequence
                    &MDST_FILT_COEF_CURR_FL[12 + coeff_table_offset]
                } else {
                    &MDST_FILT_COEF_CURR_FL[4 + coeff_table_offset]
                }
            }
            BlockType::Stop => &MDST_FILT_COEF_CURR_FL[8 + coeff_table_offset],
        };

        let mut dmx_real_chunk = scratch_buffer.chunks_exact_mut(constants::MAX_FRAMESIZE);
        let dmx_real = dmx_real_chunk.next().unwrap();
        let dmx_re_prev = dmx_real_chunk.next().unwrap();

        // initialize the MDST with zeros
        let mut dmx_im = [0.0_f32; constants::MAX_FRAMESIZE];

        let mut group_offset = 0;
        let mut is_first_window = true;

        let (prev_spectrum_left, prev_spectrum_right) =
            spectrum_prev.split_at(spectrum_prev.len() / 2);

        // process on window-basis (i.e. iterate over all groups and corresponding windows)
        for ((group, windows_per_group), alpha_q_re, alpha_q_im) in izip!(
            win_group_lengths
                .iter()
                .enumerate()
                .take(ics_info.n_window_groups()),
            self.cp_info.alpha_q_re.iter(),
            self.cp_info.alpha_q_im.iter()
        ) {
            let g_mask = 1_u8 << group;
            let group_windows_offset = group_offset * win_length;

            for ((window, dmx_imag), spectrum_l, spectrum_r) in izip!(
                dmx_im[group_windows_offset..]
                    .chunks_exact_mut(win_length)
                    .enumerate()
                    .take(usize::from(*windows_per_group)),
                spectrum_left[group_windows_offset..].chunks_exact_mut(win_length),
                spectrum_right[group_windows_offset..].chunks_exact_mut(win_length)
            ) {
                // get corresponding window in the group
                let win_start_offset = group_windows_offset + (win_length * window);

                // 1. calculate the previous downmix MDCT. We do this once just for the Main band.
                if self.cp_info.is_complex_coeff_transmitted && self.cp_info.is_use_prev_frame {
                    // if this is a long-block or the first window of a short-block
                    // calculate the downmix MDCT of the previous frame.
                    // is_use_prev_frame is assumed not to change during a frame!

                    // first determine shiftfactors to scale left and right channel
                    if ics_info.is_long_block() || is_first_window {
                        // Use the last window of the previous frame for MDCT
                        // calculation if this is a short-block
                        let index_offset = if ics_info.is_long_block() {
                            0
                        } else {
                            win_length * 7
                        };

                        // Now scale channels and determine downmix MDCT of previous frame
                        {
                            if !self.cp_info.is_side_to_mid_pred {
                                calc_downmix_mdct!(
                                    addition =>
                                    &prev_spectrum_left[index_offset..index_offset + win_length],
                                    &prev_spectrum_right[index_offset..index_offset + win_length],
                                    &mut dmx_re_prev[..win_length]
                                );
                            } else {
                                calc_downmix_mdct!(
                                    subtraction =>
                                    &prev_spectrum_left[index_offset..index_offset + win_length],
                                    &prev_spectrum_right[index_offset..index_offset + win_length],
                                    &mut dmx_re_prev[..win_length]
                                );
                            }
                        }
                    } // if is_long_block() || is_first_window
                } // if is_complex_coeff_transmitted != 0 && is_use_prev_frame != 0

                // 2. calculate downmix MDCT of current frame
                for (ms, sfb_offset) in ms_used.iter().take(max_sfb).zip(sfb_offsets.windows(2)) {
                    let start_offset = usize::from(sfb_offset[0]);
                    let end_offset = usize::from(sfb_offset[1]);

                    if *ms & g_mask != 0 {
                        dmx_real[win_start_offset + start_offset..win_start_offset + end_offset]
                            .copy_from_slice(&spectrum_l[start_offset..end_offset]);
                    } else if !self.cp_info.is_side_to_mid_pred {
                        calc_downmix_mdct!(
                            addition =>
                            &spectrum_l[start_offset..end_offset],
                            &spectrum_r[start_offset..end_offset],
                            &mut dmx_real
                                [win_start_offset + start_offset..win_start_offset + end_offset]
                        );
                    } else {
                        calc_downmix_mdct!(
                            subtraction =>
                            &spectrum_l[start_offset..end_offset],
                            &spectrum_r[start_offset..end_offset],
                            &mut dmx_real
                                [win_start_offset + start_offset..win_start_offset + end_offset]
                        );
                    }
                } // calculate downmix MDCT - iterators

                // clean until the end
                let sfb_tx_bandoffset = usize::from(sfb_offsets[max_sfb]);
                if sfb_tx_bandoffset < win_length {
                    dmx_real[win_start_offset + sfb_tx_bandoffset..win_start_offset + win_length]
                        .fill(0.0_f32);
                }

                // 3. calculate MDST-portion corresponding to the current frame
                if self.cp_info.is_complex_coeff_transmitted {
                    // The length of the filter processing must be extended because of filter
                    // boundary problems
                    let extended_band = win_length.min(sfb_tx_bandoffset + 7);

                    // 3.1 move pointer in filter-coefficient table in case of short window sequence
                    // (other coefficients are utilized for the last 7 short windows)
                    if !ics_info.is_long_block() && !is_first_window {
                        let current_shape = curr_shape as usize;
                        coeff_curr = &MDST_FILT_COEF_CURR_FL[current_shape];
                        coeff_prev = &MDST_FILT_COEF_PREV_FL[current_shape];

                        // 3.2 estimate downmix MDST from current frame downmix MDCT
                        self.filter_and_add(
                            &dmx_real[win_start_offset..win_start_offset + extended_band],
                            coeff_curr,
                            &mut dmx_imag[..extended_band],
                            true,
                        );
                        self.filter_and_add(
                            &dmx_real[win_start_offset - win_length
                                ..(win_start_offset - win_length) + extended_band],
                            coeff_prev,
                            &mut dmx_imag[..extended_band],
                            false,
                        );
                    } else {
                        self.filter_and_add(
                            &dmx_real[win_start_offset..win_start_offset + extended_band],
                            coeff_curr,
                            &mut dmx_imag[..extended_band],
                            true,
                        );
                        if self.cp_info.is_use_prev_frame {
                            self.filter_and_add(
                                &dmx_re_prev[..extended_band],
                                coeff_prev,
                                &mut dmx_imag[..extended_band],
                                false,
                            );
                        }
                    }
                } // if cplx_prediction_data.is_complex_coeff_transmitted == 1

                // 4. upmix process
                let pred_dir = if self.cp_info.is_side_to_mid_pred {
                    -1.0_f32
                } else {
                    1.0_f32
                };

                for ((ms, sfb_offset), re_alpha_q, im_alpha_q) in izip!(
                    ms_used.iter().take(max_sfb).zip(sfb_offsets.windows(2)),
                    alpha_q_re.iter(),
                    alpha_q_im.iter()
                ) {
                    if *ms & g_mask != 0 {
                        let start = usize::from(sfb_offset[0]);
                        let end = usize::from(sfb_offset[1]);

                        // inverse quantization of prediction coefficients
                        let alpha_re = f32::from(*re_alpha_q) * POINT_ONE;
                        let alpha_im = f32::from(*im_alpha_q) * POINT_ONE;

                        // calculating helper term:
                        // side = specR[i] - alpha_re[i] * dmx_re[i] - alpha_im[i] * dmx_im[i];
                        //
                        // Here "dmx_re" may be the same as "specL" or alternatively keep the
                        // downmix. "dmx_re" and "specL" are two different pointers pointing
                        // to separate arrays, which may or may not contain the same data
                        // (with different scaling).

                        for (coeff_l, coeff_r, dmx_re, dmx_im) in izip!(
                            spectrum_l[start..].iter_mut(),
                            spectrum_r[start..].iter_mut(),
                            dmx_real[win_start_offset + start..].iter(),
                            dmx_imag[start..].iter().take(end - start)
                        ) {
                            // we calculate the left and right output by using the helper function
                            // specR[i] = -/+ (specL[i] - side);
                            let side = *coeff_r - ((alpha_re * *dmx_re) + (alpha_im * *dmx_im));
                            *coeff_r = (*coeff_l - side) * pred_dir;
                            *coeff_l += side;
                        }
                    }
                } // ms_used[band] - iterator

                is_first_window = false; // reset flag after processing first window.
            } // windows_per_group - iterator

            group_offset += usize::from(*windows_per_group);
        } // group iterator
    }

    /// Apply filtering process on given input data
    ///
    /// # Parameters
    ///
    /// - `data_in`: Input samples to filter
    /// - `coeff`: Filter cofficients
    /// - `out`: Output buffer
    /// - `is_current`: Flag to indicate window is current (or) previous
    fn filter_and_add(&self, data_in: &[f32], coeff: &[f32], out: &mut [f32], is_current: bool) {
        let len = data_in.len();
        // output values with even index get a positve addon (=true) or a negative addon (=false)
        if is_current {
            for i in 0..3 {
                out[0] += coeff[i]
                    * (data_in[INDICES_1[5 - i] as usize] - data_in[INDICES_1[i] as usize]);
                out[1] += coeff[i]
                    * (data_in[INDICES_2[5 - i] as usize] - data_in[INDICES_2[i] as usize]);
                out[2] += coeff[i]
                    * (data_in[INDICES_3[5 - i] as usize] - data_in[INDICES_3[i] as usize]);
            }

            for i in 3..(len - 3) {
                let res = (data_in[i + 1..i + 4]) // (i+1+3 = i+4)f
                    .iter()
                    .rev()
                    .zip(&data_in[i - 3..i])
                    .zip(&coeff[..3])
                    .map(|((s1, s2), c)| (s1 - s2) * c)
                    .sum::<f32>();

                out[i] += res;
            }

            for i in 0..3 {
                out[len - 3] += coeff[i]
                    * (data_in[len - SUBTR_1[5 - i] as usize] - data_in[len - SUBTR_1[i] as usize]);
                out[len - 2] += coeff[i]
                    * (data_in[len - SUBTR_2[5 - i] as usize] - data_in[len - SUBTR_2[i] as usize]);
                out[len - 1] += coeff[i]
                    * (data_in[len - SUBTR_3[5 - i] as usize] - data_in[len - SUBTR_3[i] as usize]);
            }
        } else {
            for i in 0..3 {
                out[0] -= coeff[i]
                    * (data_in[INDICES_1[i] as usize] + data_in[INDICES_1[5 - i] as usize]);
                out[1] += coeff[i]
                    * (data_in[INDICES_2[i] as usize] + data_in[INDICES_2[5 - i] as usize]);
                out[2] -= coeff[i]
                    * (data_in[INDICES_3[i] as usize] + data_in[INDICES_3[5 - i] as usize]);
            }
            out[0] -= coeff[3] * data_in[0];
            out[1] += coeff[3] * data_in[1];
            out[2] -= coeff[3] * data_in[2];

            for i in 3..(len - 3) {
                let res = (data_in[i + 1..i + 4]) // (i+1+3 = i+4)
                    .iter()
                    .rev()
                    .zip(&data_in[i - 3..i])
                    .zip(&coeff[..3])
                    .map(|((s1, s2), c)| (s1 + s2) * c)
                    .sum::<f32>();

                out[i] = if let 1 = i & 0x1 {
                    out[i] + (coeff[3] * data_in[i]) + res
                } else {
                    out[i] - (coeff[3] * data_in[i]) - res
                };
            }

            for i in 0..3 {
                out[len - 3] += coeff[i]
                    * (data_in[len - SUBTR_1[i] as usize] + data_in[len - SUBTR_1[5 - i] as usize]);
                out[len - 2] -= coeff[i]
                    * (data_in[len - SUBTR_2[i] as usize] + data_in[len - SUBTR_2[5 - i] as usize]);
                out[len - 1] += coeff[i]
                    * (data_in[len - SUBTR_3[i] as usize] + data_in[len - SUBTR_3[5 - i] as usize]);
            }
            out[len - 3] += coeff[3] * data_in[len - 3];
            out[len - 2] -= coeff[3] * data_in[len - 2];
            out[len - 1] += coeff[3] * data_in[len - 1];
        }
    }
}

/// Complex prediction data
#[repr(C)]
#[derive(Copy, Clone, Debug)]
struct ComplexPredictionInfo {
    is_side_to_mid_pred: bool, // false = prediction from mid to side channel, true = vice versa
    is_complex_coeff_transmitted: bool, // false = alpha_q_im[x] is 0 for all prediction bands,
    //  true = alpha_q_im[x] is transmitted via bitstream
    is_use_prev_frame: bool, // false = use current frame for MDST estimation,
    //  true = use current and previous frame
    alpha_q_re: [[i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
    alpha_q_im: [[i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
}

impl Default for ComplexPredictionInfo {
    fn default() -> Self {
        ComplexPredictionInfo {
            is_side_to_mid_pred: false,
            is_complex_coeff_transmitted: false,
            is_use_prev_frame: false,
            alpha_q_re: [[0; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
            alpha_q_im: [[0; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS],
        }
    }
}

impl ComplexPredictionInfo {
    /// Returns `ComplexPredictionInfo` instance
    fn new() -> Self {
        ComplexPredictionInfo::default()
    }

    /// Resets complex prediction info
    fn reset(&mut self) {
        self.is_side_to_mid_pred = false;
        self.is_use_prev_frame = false;
        self.is_complex_coeff_transmitted = false;
    }
}

#[cfg(test)]
mod tests {
    use crate::{aac_dec::sr_info::SamplingRateInfo, common::bitstream::Mode};

    use super::*;

    #[test]
    fn new_complex_prediction_data() {
        let jspd = ComplexPredictionData::new();
        assert_eq!(
            &jspd.alpha_q_re_prev[..],
            &[[0_i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS][..]
        );
        assert_eq!(
            &jspd.alpha_q_im_prev[..],
            &[[0_i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS][..]
        );
        assert_eq!(jspd.win_seq_prev, BlockType::Long);
        assert_eq!(jspd.win_shape_prev, WindowShape::Sine);
        assert_eq!(jspd.win_groups_prev, 0);
        assert!(!jspd.is_interrupted);
    }

    #[test]
    fn new_jointstereo_data() {
        let jsd = JointStereoData::new(false);
        assert_eq!(jsd.ms_mask_present, 0);
        assert_eq!(jsd.ms_used, [0_u8; JOINTSTEREO_MAX_BANDS]);
        assert!(!jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, 0);
    }

    #[test]
    fn new_cplx_prediction_info() {
        let cpd = ComplexPredictionInfo::new();
        assert!(!cpd.is_side_to_mid_pred);
        assert!(!cpd.is_complex_coeff_transmitted);
        assert!(!cpd.is_use_prev_frame);
        assert_eq!(
            &cpd.alpha_q_re[..],
            &[[0_i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS][..]
        );
        assert_eq!(
            &cpd.alpha_q_im[..],
            &[[0_i16; JOINTSTEREO_MAX_BANDS]; JOINTSTEREO_MAX_GROUPS][..]
        );
    }

    #[test]
    fn generate_ms_output_samples() {
        let jsd = JointStereoData::new(false);
        let mut spec_left = [
            1.0_f32, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0,
            16.0,
        ];
        let mut spec_right = [
            2.0_f32, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
            17.0,
        ];
        let spec_left_ref = [
            3.0_f32, 5.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0, 19.0, 21.0, 23.0, 25.0, 27.0, 29.0,
            31.0, 33.0,
        ];
        let spec_right_ref = [-1.0_f32; 16];
        let n_max_sfb = 16;
        jsd.generate_ms_output(&mut spec_left, &mut spec_right, n_max_sfb);

        assert_eq!(spec_left_ref[..], spec_left[..]);
        assert_eq!(spec_right_ref[..], spec_right[..]);
    }

    #[test]
    fn read_ms_mask_present_1() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b001000 : max_sf_bands = 8
            // 0b0      : no prediction
            bitstream_writer.write(0b00010010000, 11);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b01     - ms_mask_present
            // 0b01010101 - ms_used group flags
            bitstream_writer.write(0b0101010101, 10);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 21);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(true);
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::empty();

        // Dummy init
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.cp_info.is_use_prev_frame = true;
            cp_data.cp_info.is_complex_coeff_transmitted = true;
        }
        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);

        assert_eq!(ret, 0);
        assert!(!jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 1);
        assert_eq!(jsd.ms_used[0..max_sfb], [0, 1, 0, 1, 0, 1, 0, 1]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(!cp_data.cp_info.is_use_prev_frame);
            assert!(!cp_data.cp_info.is_complex_coeff_transmitted);
        }
    }

    #[test]
    fn read_ms_mask_present_2() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b10     - ms_mask_present
            // 0b01010101 - ms_used group flags
            bitstream_writer.write(0b10, 2);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 13);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(false);
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::empty();
        // Dummy init
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.cp_info.is_use_prev_frame = true;
            cp_data.cp_info.is_complex_coeff_transmitted = true;
        }

        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);

        assert_eq!(ret, 0);
        assert!(!jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 2);
        assert_eq!(jsd.ms_used[0..max_sfb], [255; 40]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(cp_data.cp_info.is_use_prev_frame);
            assert!(cp_data.cp_info.is_complex_coeff_transmitted);
        }
    }

    #[test]
    fn read_ms_mask_present_3_long_block() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(16, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b000100 : max_sf_bands = 4
            // 0b0      : no prediction
            bitstream_writer.write(0b00010001000, 11);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b11     - ms_mask_present
            // 0b0 - cmplx_pred_data
            // 0b1010 - ms_used group flags
            // 0b1 - is_side_to_mid_pred
            // 0b1 - is_complex_coeff_transmitted
            // 0b1 - use previous frame
            // 0b1 - delta_code_time
            // 0b1111111111111111 0b1111111100000000 - some random bits to read huff code book
            bitstream_writer.write(0b110101111, 9);
            bitstream_writer.write(0b1111111111111111, 16);
            bitstream_writer.write(0b1111111100000000, 16);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 62);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(true);
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::USAC;

        // dummy value to cover calling sequence - assuming '1' (long block case)
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_groups_prev = 1;
        }

        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);

        assert_eq!(ret, 0);
        assert!(jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 3);
        assert_eq!(jsd.ms_used[0..max_sfb], [1, 1, 0, 0]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(cp_data.cp_info.is_side_to_mid_pred);
            assert!(cp_data.cp_info.is_complex_coeff_transmitted);
            assert!(cp_data.cp_info.is_use_prev_frame);

            assert!(cp_data.cp_info.alpha_q_re[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_re[0][1] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][1] != 0);
        }
    }

    #[test]
    fn read_ms_mask_present_3_short_block() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(64, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b10     : BlockType::Short
            // 0b0      : SineWindow
            // 0b0100   : max_sf_bands = 4
            // 0b1101101: scale factor grouping: 3|3|2
            bitstream_writer.write(0b010001001101101, 15);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b11     - ms_mask_present
            // 0b0 - cmplx_pred_data
            // 0b1010 10 - ms_used group flags
            // 0b1 - is_side_to_mid_pred
            // 0b1 - is_complex_coeff_transmitted
            // 0b1 - use previous frame
            // 0b1 - delta_code_time
            // 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00
            // 0xFF0000FF 0xFF0000FF 0xFF0000FF 0xFF0000FF
            bitstream_writer.write(0b110, 3);
            bitstream_writer.write(0b101010, 6);
            bitstream_writer.write(0xF, 4);
            // dummy bits to read code book
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 284);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(true);
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::USAC;

        // dummy value to cover calling sequence - assuming '1' (long block case)
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_groups_prev = 1;
            cp_data.win_seq_prev = BlockType::Short;
        }

        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);

        assert_eq!(ret, 0);
        assert!(jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 3);
        assert_eq!(jsd.ms_used[0..max_sfb], [7, 7, 0, 0]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(cp_data.cp_info.is_side_to_mid_pred);
            assert!(cp_data.cp_info.is_complex_coeff_transmitted);
            assert!(cp_data.cp_info.is_use_prev_frame);

            assert!(cp_data.cp_info.alpha_q_re[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_re[0][1] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][1] != 0);
        }
    }

    #[test]
    fn read_without_delta_code_time_with_cplx_pred_data() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(64, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b10     : BlockType::Short
            // 0b0      : SineWindow
            // 0b0100   : max_sf_bands = 4
            // 0b1101101: scale factor grouping: 3|3|2
            bitstream_writer.write(0b010001001101101, 15);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b11     - ms_mask_present
            // 0b1 - cmplx_pred_data
            // 0b1010 10 - ms_used group flags
            // 0b1 - is_side_to_mid_pred
            // 0b0 - is_complex_coeff_transmitted
            // 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00
            // 0xFF0000FF 0xFF0000FF 0xFF0000FF 0xFF0000FF
            bitstream_writer.write(0b111, 3);
            // bitstream_writer.write(0b101010, 6);
            bitstream_writer.write(0x2, 2);
            // dummy bits to read code book
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 276);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(true);
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::USAC | ACFlags::INDEP;

        // dummy value to cover calling sequence - assuming '1' (long block case)
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_groups_prev = 1;
        }

        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);

        assert_eq!(ret, 0);
        assert!(jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 3);
        assert_eq!(jsd.ms_used[0..max_sfb], [7, 7, 7, 7]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(cp_data.cp_info.is_side_to_mid_pred);
            assert!(!cp_data.cp_info.is_complex_coeff_transmitted);
            assert!(!cp_data.cp_info.is_use_prev_frame);

            assert!(cp_data.cp_info.alpha_q_re[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_re[0][1] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][0] == 0);
            assert!(cp_data.cp_info.alpha_q_im[0][1] == 0);
        }
    }

    #[test]
    fn apply_ms_stereo() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(64, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b10     : BlockType::Short
            // 0b0      : SineWindow
            // 0b0100   : max_sf_bands = 4
            // 0b1101101: scale factor grouping: 3|3|2
            bitstream_writer.write(0b010001001101101, 15);

            // Joint stereo data from bitstream
            // Byte blob meaning:
            // 0b11     - ms_mask_present
            // 0b0 - cmplx_pred_data
            // 0b1010 10 - ms_used group flags
            // 0b1 - is_side_to_mid_pred
            // 0b1 - is_complex_coeff_transmitted
            // 0b1 - use previous frame
            // 0b1 - delta_code_time
            // 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00 0xFFFFFF00
            // 0xFF0000FF 0xFF0000FF 0xFF0000FF 0xFF0000FF
            bitstream_writer.write(0b110, 3);
            bitstream_writer.write(0b101010, 6);
            bitstream_writer.write(0xF, 4);
            // dummy bits to read code book
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFFFFFF00, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);
            bitstream_writer.write(0xFF0000FF, 32);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 284);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // create Joint stereo instances
        let mut jsd = JointStereoData::new(true);
        const FRAME_LENGTH: usize = constants::MAX_FRAMESIZE;

        let mut spectrum = vec![0.0_f32; FRAME_LENGTH * 2];
        spectrum[..FRAME_LENGTH].fill(0.05_f32); // Left channel
        spectrum[FRAME_LENGTH..].fill(0.025_f32); // Right channel

        let mut scratch_buffer = vec![0.0_f32; FRAME_LENGTH * 2];
        let max_sfb = ics_info.max_sf_bands();
        let ac_flags = ACFlags::USAC;
        let spectral_coeffs_prev = &[0.025_f32; FRAME_LENGTH * 2];
        // dummy value to cover calling sequence - assuming '1' (long block case)
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_groups_prev = 1;
        }

        let ret = jsd.read(&mut bitstream_reader, &ics_info, max_sfb, ac_flags);
        assert_eq!(ret, 0);
        assert!(jsd.cplx_pred_flag);
        assert_eq!(jsd.max_sfb, ics_info.max_sf_bands());
        assert_eq!(jsd.ms_mask_present, 3);
        assert_eq!(jsd.ms_used[0..max_sfb], [7, 7, 0, 0]);
        if let Some(cp_data) = &mut jsd.complex_prediction {
            assert!(cp_data.cp_info.is_side_to_mid_pred);
            assert!(cp_data.cp_info.is_complex_coeff_transmitted);
            assert!(cp_data.cp_info.is_use_prev_frame);

            assert!(cp_data.cp_info.alpha_q_re[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_re[0][1] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][0] != 0);
            assert!(cp_data.cp_info.alpha_q_im[0][1] != 0);
        }

        // CASE 1
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_seq_prev = BlockType::Short;
        }
        jsd.apply(
            &ics_info,
            &mut spectrum,
            &spectral_coeffs_prev[..],
            &mut scratch_buffer,
        );

        // swf_offset values based on ms_used[] 0 4 8 12 16 ... And ms_used is [7, 7, 0, 0, ...]
        // window_length is [3, 3, 2, ...]
        const BAND_OFFSET: usize = 8;
        for i in 0..2 {
            let (buffer, value, scb_value) = if i == 0 {
                (&mut spectrum[..FRAME_LENGTH], 0.05_f32, 0.05_f32)
            } else {
                (
                    &mut spectrum[FRAME_LENGTH..],
                    0.025_f32,
                    (0.05_f32 - 0.025_f32) * 0.5_f32,
                )
            };
            // group 0
            assert_ne!(&buffer[0..BAND_OFFSET], &[value; BAND_OFFSET]);
            assert_ne!(&buffer[128..128 + BAND_OFFSET], &[value; BAND_OFFSET]);
            assert_ne!(&buffer[256..256 + BAND_OFFSET], &[value; BAND_OFFSET]);
            // group 1
            assert_ne!(&buffer[384..384 + BAND_OFFSET], &[value; BAND_OFFSET]);
            assert_ne!(&buffer[512..512 + BAND_OFFSET], &[value; BAND_OFFSET]);
            assert_ne!(&buffer[640..640 + BAND_OFFSET], &[value; BAND_OFFSET]);
            // group 2
            assert_ne!(&buffer[768..768 + BAND_OFFSET], &[value; BAND_OFFSET]);
            assert_ne!(&buffer[896..896 + BAND_OFFSET], &[value; BAND_OFFSET]);

            // Assert scratch_buffer values
            // group 0
            let scb_offset = BAND_OFFSET + (BAND_OFFSET * i);
            assert_eq!(
                &scratch_buffer[BAND_OFFSET * i..scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[128 + (BAND_OFFSET * i)..128 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[256 + (BAND_OFFSET * i)..256 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            // group 1
            assert_eq!(
                &scratch_buffer[384 + (BAND_OFFSET * i)..384 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[512 + (BAND_OFFSET * i)..512 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[640 + (BAND_OFFSET * i)..640 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            // group 2
            assert_eq!(
                &scratch_buffer[768 + (BAND_OFFSET * i)..768 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[896 + (BAND_OFFSET * i)..896 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
        }

        // CASE 2 - calling sequence to cover conditional statements
        spectrum[..FRAME_LENGTH].fill(0.075_f32); // Left channel
        spectrum[FRAME_LENGTH..].fill(0.05_f32); // Right channel
        if let Some(cp_data) = &mut jsd.complex_prediction {
            cp_data.win_seq_prev = BlockType::Long;
            cp_data.cp_info.is_side_to_mid_pred = false; // Force value to zero, in order to cover
                                                         // test condition
        }

        jsd.apply(
            &ics_info,
            &mut spectrum,
            &spectral_coeffs_prev[..],
            &mut scratch_buffer,
        );

        for i in 0..2 {
            let scb_value = if i == 0 {
                0.075_f32
            } else {
                (0.05_f32 + 0.075_f32) * 0.5_f32
            };

            // Assert scratch_buffer values
            // group 0
            let scb_offset = BAND_OFFSET + (BAND_OFFSET * i);
            assert_eq!(
                &scratch_buffer[BAND_OFFSET * i..scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[128 + (BAND_OFFSET * i)..128 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[256 + (BAND_OFFSET * i)..256 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            // group 1
            assert_eq!(
                &scratch_buffer[384 + (BAND_OFFSET * i)..384 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[512 + (BAND_OFFSET * i)..512 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[640 + (BAND_OFFSET * i)..640 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            // group 2
            assert_eq!(
                &scratch_buffer[768 + (BAND_OFFSET * i)..768 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
            assert_eq!(
                &scratch_buffer[896 + (BAND_OFFSET * i)..896 + scb_offset],
                &[scb_value; BAND_OFFSET]
            );
        }
    }
}
