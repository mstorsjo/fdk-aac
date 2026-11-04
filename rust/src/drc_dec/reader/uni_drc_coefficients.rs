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
//! MPEG-D DRC's uniDRC coefficients.

use super::super::{
    common::{DrcBitstreamLocation, GainInterpolationType},
    constants::*,
    drc_error::DrcError,
};
use super::{
    gain_node::GainNode,
    uni_drc_characteristic::{
        CharacteristicFormat, CharacteristicSide, CustomDrcChar, DrcCharacteristic,
    },
};
use crate::common::bitstream::Bitstream;
use itertools::izip;

type Huffman = &'static [[i8; 2]];

#[rustfmt::skip]
/// Huffman codeword table for gain coding profile is [0,1].
static DELTA_GAIN_CODING_PROFILE_0_1_HUFFMAN: [[i8; 2]; 24] = [
    [1, 2],    [3, 4],     [-63, -65], [5, -66],   [-64, 6],   [-80, 7],
    [8, 9],    [-68, 10],  [11, 12],   [-56, -67], [-61, 13],  [-62, -69],
    [14, 15],  [16, -72],  [-71, 17],  [-70, -60], [18, -59],  [19, 20],
    [21, -79], [-57, -73], [22, -58],  [-76, 23],  [-75, -74], [-78, -77],
];

#[rustfmt::skip]
/// Huffman codeword table for gain coding profile is 2.
static DELTA_GAIN_CODING_PROFILE_2_HUFFMAN: [[i8; 2]; 48] = [
    [1, 2],     [3, 4],     [5, 6],     [7, 8],     [9, 10],    [11, 12],
    [13, -65],  [14, -64],  [15, -66],  [16, -67],  [17, 18],   [19, -68],
    [20, -63],  [-69, 21],  [-59, 22],  [-61, -62], [-60, 23],  [24, -58],
    [-70, -57], [-56, -71], [25, 26],   [27, -55],  [-72, 28],  [-54, 29],
    [-53, 30],  [-73, -52], [31, -74],  [32, 33],   [-75, 34],  [-76, 35],
    [-51, 36],  [-78, 37],  [-77, 38],  [-96, 39],  [-48, 40],  [-50, -79],
    [41, 42],   [-80, -81], [-82, 43],  [44, -49],  [45, -84],  [-83, -89],
    [-86, 46],  [-90, -85], [-91, -93], [-92, 47],  [-88, -87], [-95, -94],
];

#[rustfmt::skip]
/// Huffman codeword table for slope steepness.
static SLOPE_STEEPNESS_HUFFMAN: [[i8; 2]; 14] = [
    [1, -57],  [-58, 2],   [3, 4],    [5, 6],    [7, -56],
    [8, -60],  [-61, -55], [9, -59],  [10, -54], [-64, 11],
    [-51, 12], [-62, -50], [-63, 13], [-52, -53],
];

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// The types of DRC gain coding profile.
enum GainCodingProfile {
    #[default]
    /// Regular gain coding.
    Regular = 0,
    /// Fading gain coding.
    Fading,
    /// Clipping prevention and ducking gain coding.
    ClippingDucking,
    /// Constant gain (no gain sequence is transmitted).
    Constant,
}

/// `From<u8>` for GainCodingProfile.
impl From<u8> for GainCodingProfile {
    fn from(value: u8) -> Self {
        match value {
            0 => GainCodingProfile::Regular,
            1 => GainCodingProfile::Fading,
            2 => GainCodingProfile::ClippingDucking,
            3 => GainCodingProfile::Constant,
            _ => panic!("invalid value: {value}"),
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
pub(in super::super) struct GainSet {
    /// DRC gain coding profile type.
    gain_coding_profile: GainCodingProfile,
    /// DRC gain interpolation type.
    gain_interpolation_type: GainInterpolationType,
    /// Flag to indicate the last node is always at end of the frame (In low delay mode).
    is_full_frame: bool,
    /// Flag to indicate whether the gain sample is aligned with the end (false) or the center
    /// (true) of the `deltaTmin` interval.
    is_time_alignment: bool,
    /// Flag to indicate the custom time (`deltaTmin`) resolution of a DRC sequence.
    is_time_delta_min_present: bool,
    /// The custom time (`deltaTmin`) resolution of a DRC sequence. `deltaTmin` in units of audio
    /// sample intervals. Range: 1...2^11.
    time_delta_min: u16,
    /// Number of DRC sub-bands (4 bit value read from bitstream).
    band_count: u8,
    /// Flag to indicate whether the `crossover_freq_index` (`true`) or the `start_sub_band_index`
    /// (`false`)  of sub-bands should be used for DRC gain.
    is_drc_band_type: bool,
    /// Gain sequence indices.
    gain_sequence_index: [u8; DRC_MAX_BANDS],
    /// DRC characteristic for gain sequences.
    drc_characteristic: [DrcCharacteristic; DRC_MAX_BANDS],
}

impl GainSet {
    /// Returns number of DRC bands.
    pub(in super::super) fn band_count(&self) -> u8 {
        self.band_count
    }

    pub(in super::super) fn drc_characteristic(&self) -> &[DrcCharacteristic; 4] {
        &self.drc_characteristic
    }

    pub(in super::super) fn gain_interpolation_type(&self) -> GainInterpolationType {
        self.gain_interpolation_type
    }

    /// Returns `gain sequence indices`.
    pub(in super::super) fn gain_sequence_index(&self) -> &[u8] {
        &self.gain_sequence_index
    }

    /// Returns `self.time_delta_min` if present, Otherwise `delta_tmin_default`.
    pub(super) fn get_time_delta_min(&self, delta_tmin_default: u16) -> u16 {
        if self.is_time_delta_min_present {
            self.time_delta_min
        } else {
            delta_tmin_default
        }
    }

    /// Decodes initial gain of node from bitstream based on gain coding profile.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `gain value`.
    fn decode_gain_initial(&self, bs: &mut Bitstream) -> f32 {
        match self.gain_coding_profile {
            GainCodingProfile::Regular => {
                let is_sign = bs.read_bit() != 0;
                let magn = bs.read(8) as f32;
                let gain_initial = magn * 0.125_f32;
                if is_sign {
                    -gain_initial
                } else {
                    gain_initial
                }
            }
            GainCodingProfile::Fading => {
                let is_sign = bs.read_bit() != 0;
                if is_sign {
                    let magn = (bs.read(10) + 1) as f32;
                    -magn * 0.125_f32
                } else {
                    0.0_f32
                }
            }
            GainCodingProfile::ClippingDucking => {
                let is_sign = bs.read_bit() != 0;
                if is_sign {
                    let magn = (bs.read(8) + 1) as f32;
                    -magn * 0.125_f32
                } else {
                    0.0_f32
                }
            }
            GainCodingProfile::Constant => 0.0_f32,
        }
    }

    /// Decodes `huffman codeword`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `huffman codeword`.
    fn decode_huffman_cw(bs: &mut Bitstream, hcw: Huffman) -> i32 {
        let mut index = 0;
        while index >= 0 {
            let bit = bs.read_bit() as usize;
            index = hcw[index as usize][bit];
        }

        // value
        i32::from(index) + 64 // add offset
    }

    /// Decodes gain value of each gain node from bitstream based on gain coding profile.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `nodes`: Buffer to store decoded gain node values.
    /// - `num_nodes`: Number of gain nodes.
    fn decode_gains(
        &self,
        bs: &mut Bitstream,
        nodes: &mut [GainNode; MAX_NODES],
        num_nodes: usize,
    ) {
        nodes[0].set_gain_db(self.decode_gain_initial(bs));

        let delta_gain_cb = if self.gain_coding_profile == GainCodingProfile::ClippingDucking {
            DELTA_GAIN_CODING_PROFILE_2_HUFFMAN.as_slice()
        } else {
            DELTA_GAIN_CODING_PROFILE_0_1_HUFFMAN.as_slice()
        };

        // Nodes => [0]
        let mut prev_again_db = nodes[0].gain_db();

        // Nodes => [1..15].
        for node in nodes[1..].iter_mut().take(num_nodes.min(MAX_NODES) - 1) {
            let delta_gain = Self::decode_huffman_cw(bs, delta_gain_cb);
            node.set_gain_db(prev_again_db + delta_gain as f32 * 0.125_f32);
            prev_again_db = node.gain_db();
        }

        // Nodes => [16..(num_nodes - 1)].
        if num_nodes > MAX_NODES {
            for _k in MAX_NODES..num_nodes {
                let _delta_gain = Self::decode_huffman_cw(bs, delta_gain_cb);
            }
        }
    }

    /// Decodes slop steepness value for each gain node from bitstream if gain interpolation type
    /// is `GainInterpolationType::Spline`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `num_nodes`: Number of gain nodes.
    fn decode_slopes(&self, bs: &mut Bitstream, num_nodes: usize) {
        if self.gain_interpolation_type == GainInterpolationType::Spline {
            // decode slope steepness
            for _k in 0..num_nodes {
                let _value = Self::decode_huffman_cw(bs, SLOPE_STEEPNESS_HUFFMAN.as_slice());
            }
        }
    }

    /// Decodes value of `number of gain nodes` from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    fn decode_n_nodes(bs: &mut Bitstream) -> usize {
        let mut num_nodes = 128;

        // Decode number of nodes.
        'count_loop: for n in 1..128 {
            if bs.read_bit() != 0 {
                num_nodes = n;
                break 'count_loop;
            }
        }
        num_nodes
    }

    /// Decodes `delta time` value from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `z`: Minimum codeword length for delta time.
    ///
    /// Returns `Delta time value`.
    fn decode_time_delta(bs: &mut Bitstream, z: u8) -> i32 {
        let prefix = bs.read(2) as u8;
        (match prefix {
            0x0 => 1,
            0x1 => {
                let mu = bs.read(2);
                mu + 2
            }
            0x2 => {
                let mu = bs.read(3);
                mu + 6
            }
            0x3 => {
                let mu = bs.read(z);
                mu + 14
            }
            _ => 0,
        })
        .try_into()
        .unwrap()
    }

    /// Decodes `time value` of each gain node from bitstream. `Time` is calculated for each node
    /// using `frame_size` , `delta_tmin` and `time_offset` values.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `nodes`: Buffer to store decoded gain node values.
    /// - `num_nodes`: Number of gain nodes.
    /// - `delta_tmin`: Delta time minimum value.
    /// - `frame_size`: Length of DRC frame.
    /// - `time_offset`: Offset value to calculate gain node`s time.
    /// - `z`: Minimum codeword length for delta time.
    #[expect(clippy::too_many_arguments)]
    fn decode_times(
        &self,
        bs: &mut Bitstream,
        nodes: &mut [GainNode; MAX_NODES],
        num_nodes: usize,
        delta_tmin: i32,
        frame_size: i16,
        time_offset: i16,
        z: u8,
    ) {
        let mut time_offs = i32::from(time_offset);
        let is_frame_end_flag = if self.is_full_frame {
            true
        } else {
            bs.read_bit() != 0
        };

        if is_frame_end_flag {
            // is_frame_end_flag == true, signals that the last node is at the end of the DRC frame.
            let nodes_len = num_nodes - 1;
            let mut is_node_res_flag = false;

            // Nodes => [0..15].
            for k in 0..nodes_len.min(MAX_NODES - 1) {
                // Decode a delta time value.
                let time_delta = Self::decode_time_delta(bs, z);
                // is_frame_end_flag == true, needs special handling for last node with node
                // reservoir.

                let node_time_tmp_val = time_offs + time_delta * delta_tmin;
                let node_time_tmp = node_time_tmp_val.min(i32::from(2 * frame_size + time_offset));
                if node_time_tmp > i32::from(frame_size + time_offset) {
                    nodes[k + 1].set_time(node_time_tmp.try_into().unwrap());
                    if !is_node_res_flag {
                        nodes[k].set_time(frame_size + time_offset);
                        is_node_res_flag = true;
                    }
                } else {
                    nodes[k].set_time(node_time_tmp.try_into().unwrap());
                }
                time_offs = node_time_tmp;
            }

            // Nodes => [16..(num_nodes - 2)].
            for _k in MAX_NODES - 1..nodes_len {
                // Decode a delta time value.
                let _time_delta = Self::decode_time_delta(bs, z);
            }

            if !is_node_res_flag {
                let k = nodes_len.min(MAX_NODES - 1);
                nodes[k].set_time(frame_size + time_offset);
            }
        } else {
            // Nodes => [0..15].
            for node in nodes.iter_mut().take(num_nodes.min(MAX_NODES)) {
                // Decode a delta time value.
                let time_delta = Self::decode_time_delta(bs, z);
                let mut node_time_tmp = time_offs + time_delta * delta_tmin;
                node_time_tmp = node_time_tmp.min(i32::from(2 * frame_size + time_offset));
                time_offs = node_time_tmp;
                node.set_time(node_time_tmp.try_into().unwrap());
            }

            // Nodes => [16..(num_nodes - 1)].
            if num_nodes > MAX_NODES {
                for _k in MAX_NODES..num_nodes {
                    // Decode a delta time value.
                    let _time_delta = Self::decode_time_delta(bs, z);
                }
            }
        }
    }

    /// Calculates minimum codeword length that is used for encoding `delta time` values.
    ///
    /// # Parameters
    /// - `num_nodes_max`: Maximum of number of gain nodes.
    ///
    /// Returns `minimum codeword length`.
    fn get_z(num_nodes_max: i32) -> i32 {
        // `z` is the minimum codeword length that is needed to encode all possible
        // time delta values.
        // `z = ceil(log2(2*num_nodes_max))`

        let mut z = 1;
        while (1 << z) < (2 * num_nodes_max) {
            z += 1;
        }
        z
    }

    /// Reads data from bitstream based on `GainSet` information and calculates
    /// `time and gain value` of each gain node.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `nodes`: Buffer to store decoded gain node values.
    /// - `time_delta_min`: Delta time minimum value.
    /// - `frame_size`: Length of DRC frame.
    ///
    /// Returns `Number of gain nodes`.
    fn read_nodes(
        &self,
        bs: &mut Bitstream,
        nodes: &mut [GainNode; MAX_NODES],
        time_delta_min: i32,
        frame_size: i16,
    ) -> usize {
        let z = Self::get_z(i32::from(frame_size) / time_delta_min);
        let time_offs = if !self.is_time_alignment {
            -1
        } else {
            -time_delta_min + (time_delta_min - 1) / 2
        };

        let time_offset: i16 = time_offs.try_into().unwrap();

        let is_drc_gain_coding_mode = bs.read_bit() != 0;

        if !is_drc_gain_coding_mode {
            // "simple" mode: only one node at the end of the frame with slope = 0.
            nodes[0].set_gain_db(self.decode_gain_initial(bs));
            nodes[0].set_time(frame_size + time_offset);

            1 // return number of nodes.
        } else {
            let num_nodes = Self::decode_n_nodes(bs);

            self.decode_slopes(bs, num_nodes);
            self.decode_times(
                bs,
                nodes,
                num_nodes,
                time_delta_min,
                frame_size,
                time_offset,
                z.try_into().unwrap(),
            );

            self.decode_gains(bs, nodes, num_nodes);
            // Return number of nodes.
            num_nodes
        }
    }

    /// Reads data from bitstream based on `GainSet` information and calculates
    /// `time and gain value` for each gain node that belongs to specific `DRC gain sequence`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data.
    /// - `nodes`: Buffer to store decoded gain node values.
    /// - `num_nodes`: Number of gain nodes.
    /// - `time_delta_min`: Delta time minimum value.
    /// - `frame_size`: Length of DRC frame.
    pub(super) fn read_drc_gain_sequence(
        &self,
        bs: &mut Bitstream,
        nodes: &mut [GainNode; MAX_NODES],
        num_nodes: &mut usize,
        time_delta_min: u16,
        frame_size: i16,
    ) {
        let mut time_buf_prev_frame = [0_i16; MAX_NODES];
        let mut time_buf_cur_frame = [0_i16; MAX_NODES];

        if self.gain_coding_profile == GainCodingProfile::Constant {
            *num_nodes = 1;
            nodes[0].set_time(frame_size - 1);
            nodes[0].set_gain_db(0.0_f32);
        } else {
            *num_nodes = self.read_nodes(bs, nodes, time_delta_min.into(), frame_size);
            // Count number of nodes in node reservoir.
            let mut n_nodes_node_res = 0;
            let mut n_nodes_cur = 0;

            // Count and buffer nodes from node reservoir.
            let mut node_count = 0;
            'count_nodes: for node in nodes.iter_mut().take((*num_nodes).min(MAX_NODES)) {
                if node.time() >= (2 * frame_size) {
                    break 'count_nodes;
                }

                if node.time() >= frame_size {
                    // Write node reservoir times into buffer.
                    time_buf_prev_frame[n_nodes_node_res] = node.time();
                    n_nodes_node_res += 1;
                } else {
                    // Times from current frame.
                    time_buf_cur_frame[n_nodes_cur] = node.time();
                    n_nodes_cur += 1;
                }
                node_count += 1;
            }

            // Restrict num_nodes to valid ones if break occured.
            *num_nodes = node_count;

            // Insert dummy node if no node is available.
            if *num_nodes == 0 {
                *num_nodes = 1;
                nodes[0].set_time(frame_size - 1);
                nodes[0].set_gain_db(0.0_f32);
            }

            // Compose right time order (bit reservoir first).
            for (node, prev_frame) in
                izip!(nodes.iter_mut(), time_buf_prev_frame.iter()).take(n_nodes_node_res)
            {
                // Subtract two time `frame_size`: one to remove node reservoir offset and
                // one to get the negative index relative to the current frame.
                node.set_time(*prev_frame - 2 * frame_size);
            }

            // Times from current frame.
            if n_nodes_node_res < MAX_NODES {
                izip!(
                    nodes[n_nodes_node_res..].iter_mut(),
                    time_buf_cur_frame.iter()
                )
                .take(n_nodes_cur)
                .for_each(|(node, curr_frame)| node.set_time(*curr_frame));
            }
        }
    }

    /// Reads `GainSet` data from bitstream and updates `gain_sequence_index`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `gain_sequence_index`: Gain sequence index
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    ///
    ///  Returns `DrcError`.
    fn read(
        &mut self,
        bs: &mut Bitstream,
        gain_sequence_index: &mut u8,
        version: u8,
    ) -> Result<(), DrcError> {
        self.gain_coding_profile = GainCodingProfile::from(bs.read(2) as u8);
        self.gain_interpolation_type = GainInterpolationType::from(bs.read_bit() != 0);
        self.is_full_frame = bs.read_bit() != 0;
        self.is_time_alignment = bs.read_bit() != 0;
        self.is_time_delta_min_present = bs.read_bit() != 0;

        if self.is_time_delta_min_present {
            self.time_delta_min = bs.read(11) as u16 + 1;
        }

        match self.gain_coding_profile {
            GainCodingProfile::Regular
            | GainCodingProfile::Fading
            | GainCodingProfile::ClippingDucking => {
                self.band_count = bs.read(4) as u8;
                if self.band_count > DRC_MAX_BANDS as u8 {
                    return Err(DrcError::MemoryError);
                }
                if self.band_count > 1 {
                    self.is_drc_band_type = bs.read_bit() != 0;
                }

                for (gsi, drc_char) in izip!(
                    self.gain_sequence_index.iter_mut(),
                    self.drc_characteristic.iter_mut()
                )
                .take(usize::from(self.band_count))
                {
                    if version != 0 {
                        let is_index_present = bs.read_bit() != 0;
                        if is_index_present {
                            *gain_sequence_index = bs.read(6) as u8;
                        }
                    }

                    *gsi = *gain_sequence_index;
                    *gain_sequence_index += 1;
                    drc_char.read(bs, version);
                }

                if self.band_count > 1 {
                    self.read_band_border(bs);
                }
            }
            GainCodingProfile::Constant => {
                self.band_count = 1;
                self.gain_sequence_index[0] = *gain_sequence_index;
                *gain_sequence_index += 1;
            }
        }

        Ok(())
    }

    fn read_band_border(&self, bs: &mut Bitstream) {
        // Simply push bits forward in bitstream, since multi-band is not supported.
        if self.is_drc_band_type {
            // Read crossover_freq_index
            let num_bits = 4 * isize::from(self.band_count - 1);
            bs.push(num_bits);
        } else {
            // Read start_sub_band_index
            let num_bits = 10 * isize::from(self.band_count - 1);
            bs.push(num_bits);
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// `DrcCoefficientsUniDrc` structure holds payloads for all available DRC gain sequences in one
/// location.
pub(in super::super) struct DrcCoefficientsUniDrc {
    /// The DRC location describes where DRC gain sequences can be found in the bitstream.
    drc_location: DrcBitstreamLocation,
    /// Flag to indicate presence of DRC frame size.
    is_drc_frame_size_present: bool,
    /// DRC frame size in units of audio sample intervals. Range: 1...2^15.
    drc_frame_size: u16,
    /// Number of left characteristic.
    characteristic_left_count: u8,
    /// Left characteristic formats.
    characteristic_left_format: [CharacteristicFormat; MAX_CUSTOM_CHARACTERISTICS],
    /// Left custom DRC characteristic.
    custom_characteristic_left: [CustomDrcChar; MAX_CUSTOM_CHARACTERISTICS],
    /// Number of right characteristic.
    characteristic_right_count: u8,
    /// Right characteristic formats.
    characteristic_right_format: [CharacteristicFormat; MAX_CUSTOM_CHARACTERISTICS],
    /// Right custom DRC characteristic.
    custom_characteristic_right: [CustomDrcChar; MAX_CUSTOM_CHARACTERISTICS],
    /// Number of DRC gain sequences. A sequence is a series of DRC gain values that can be applied
    /// to one or more audio channels.
    gain_sequence_count: u8,
    /// Number of `GainSet` is used.
    gain_set_count: u8,
    /// `GainSet` of gain sequences.
    gain_set: [GainSet; MAX_SEQUENCES as usize],
    /// Gain set index for gain sequences.
    gain_set_index_for_gain_sequence: [u8; MAX_SEQUENCES as usize],
}

impl DrcCoefficientsUniDrc {
    /// Returns `index`-th value of DRC compression characteristic fomart, belonging to a
    /// `CharacteristicSide`.
    pub(in super::super) fn characteristic_format(
        &self,
        drc_char_side: CharacteristicSide,
        index: usize,
    ) -> CharacteristicFormat {
        match drc_char_side {
            CharacteristicSide::Left => self.characteristic_left_format[index],
            CharacteristicSide::Right => self.characteristic_right_format[index],
        }
    }

    pub(in super::super) fn custom_characteristic(
        &self,
        drc_char_side: CharacteristicSide,
        index: usize,
    ) -> &CustomDrcChar {
        match drc_char_side {
            CharacteristicSide::Left => &self.custom_characteristic_left[index],
            CharacteristicSide::Right => &self.custom_characteristic_right[index],
        }
    }

    /// Returns DRC location in bitstream.
    pub(in super::super) fn drc_location(&self) -> DrcBitstreamLocation {
        self.drc_location
    }

    pub(in super::super) fn drc_frame_size(&self) -> u16 {
        self.drc_frame_size
    }

    /// Returns immutable reference to `GainSet`.
    pub(in super::super) fn gain_set(&self, index: usize) -> &GainSet {
        &self.gain_set[index]
    }

    /// Returns number of gain sets.
    pub(in super::super) fn gain_set_count(&self) -> u8 {
        self.gain_set_count
    }

    /// Returns number of gain sequences.
    pub(in super::super) fn gain_sequence_count(&self) -> u8 {
        self.gain_sequence_count
    }

    /// Returns immutable reference to `[u8; MAX_SEQUENCES]`.
    pub(in super::super) fn gain_set_index_for_gain_sequence(
        &self,
    ) -> &[u8; MAX_SEQUENCES as usize] {
        &self.gain_set_index_for_gain_sequence
    }

    pub(in super::super) fn init(&mut self) {
        self.drc_location = DrcBitstreamLocation::Undefined;
        self.is_drc_frame_size_present = false;
        self.drc_frame_size = 0;
        self.characteristic_left_count = 0;
        self.characteristic_left_format = [CharacteristicFormat::default(); 16];
        self.custom_characteristic_left = [CustomDrcChar::default(); 16];
        self.characteristic_right_count = 0;
        self.characteristic_right_format = [CharacteristicFormat::default(); 16];
        self.custom_characteristic_right = [CustomDrcChar::default(); 16];
        self.gain_sequence_count = 0;
        self.gain_set_count = 0;
        self.gain_set = [GainSet::default(); MAX_SEQUENCES as usize];
        self.gain_set_index_for_gain_sequence = [0; MAX_SEQUENCES as usize];
    }

    pub(in super::super) fn is_drc_frame_size_present(&self) -> bool {
        self.is_drc_frame_size_present
    }

    /// Reads `DrcCoefficientsUniDrc` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    ///
    ///  Returns `DrcError`.
    pub(super) fn read(&mut self, bs: &mut Bitstream, version: u8) -> Result<(), DrcError> {
        let mut gain_sequence_index = 0_u8;
        self.drc_location = DrcBitstreamLocation::from(bs.read(4) as u8);
        self.is_drc_frame_size_present = bs.read_bit() != 0;

        if self.is_drc_frame_size_present {
            // Read DRC frame size
            self.drc_frame_size = bs.read(15) as u16 + 1;
        }

        if version == 0 {
            let mut gain_sequence_count = 0;
            self.characteristic_left_count = 0;
            self.characteristic_right_count = 0;
            let gain_set_count = bs.read(6) as u8;
            self.gain_set_count = gain_set_count.min(MAX_SEQUENCES);

            for gain_set in self
                .gain_set
                .iter_mut()
                .take(usize::from(self.gain_set_count))
            {
                let mut tmp_gset = GainSet::default();
                tmp_gset.read(bs, &mut gain_sequence_index, version)?;

                gain_sequence_count += tmp_gset.band_count;
                *gain_set = tmp_gset;
            }

            if gain_set_count > MAX_SEQUENCES {
                // Read additional GainSet from Bitstream if available.
                for _i in MAX_SEQUENCES..gain_set_count {
                    let mut tmp_gset = GainSet::default();
                    tmp_gset.read(bs, &mut gain_sequence_index, version)?;

                    gain_sequence_count += tmp_gset.band_count;
                }
            }

            self.gain_sequence_count = gain_sequence_count
        } else {
            // version == 1.
            let is_drc_characteristic_left_present = bs.read_bit() != 0;
            if is_drc_characteristic_left_present {
                self.characteristic_left_count = bs.read(4) as u8;

                if (self.characteristic_left_count + 1) > MAX_CUSTOM_CHARACTERISTICS as u8 {
                    return Err(DrcError::MemoryError);
                }

                for (left_fmt, custom_char) in izip!(
                    self.characteristic_left_format.iter_mut().skip(1),
                    self.custom_characteristic_left.iter_mut().skip(1)
                )
                .take(usize::from(self.characteristic_left_count))
                {
                    custom_char.read(bs, left_fmt, CharacteristicSide::Left)?;
                }
            }

            let is_drc_characteristic_right_present = bs.read_bit() != 0;
            if is_drc_characteristic_right_present {
                self.characteristic_right_count = bs.read(4) as u8;

                if (self.characteristic_right_count + 1) > MAX_CUSTOM_CHARACTERISTICS as u8 {
                    return Err(DrcError::MemoryError);
                }

                for (right_fmt, custom_char) in izip!(
                    self.characteristic_right_format.iter_mut().skip(1),
                    self.custom_characteristic_right.iter_mut().skip(1)
                )
                .take(usize::from(self.characteristic_right_count))
                {
                    custom_char.read(bs, right_fmt, CharacteristicSide::Right)?;
                }
            }

            let is_shape_filters_present = bs.read_bit() != 0;
            if is_shape_filters_present {
                let shape_filter_count = bs.read(4) as u8;
                for _count in 0..shape_filter_count {
                    let mut is_tmp_present = bs.read_bit() != 0;
                    if is_tmp_present {
                        // lf_cut_params
                        bs.push(5);
                    }

                    is_tmp_present = bs.read_bit() != 0;
                    if is_tmp_present {
                        // lf_boost_params
                        bs.push(5);
                    }

                    is_tmp_present = bs.read_bit() != 0;
                    if is_tmp_present {
                        // hf_cut_params
                        bs.push(5);
                    }

                    is_tmp_present = bs.read_bit() != 0;
                    if is_tmp_present {
                        // hf_boost_params
                        bs.push(5);
                    }
                }
            }

            self.gain_sequence_count = bs.read(6) as u8;
            let gain_set_count = bs.read(6) as u8;
            self.gain_set_count = gain_set_count.min(MAX_SEQUENCES);

            for gain_set in self
                .gain_set
                .iter_mut()
                .take(usize::from(self.gain_set_count))
            {
                let mut tmp_gset = GainSet::default();
                tmp_gset.read(bs, &mut gain_sequence_index, version)?;

                *gain_set = tmp_gset;
            }

            if gain_set_count > MAX_SEQUENCES {
                // Read additional GainSet from Bitstream if available.
                for _i in MAX_SEQUENCES..gain_set_count {
                    let mut tmp_gset = GainSet::default();
                    tmp_gset.read(bs, &mut gain_sequence_index, version)?;
                }
            }
        }

        self.gain_set_index_for_gain_sequence.fill(255);

        let mut count = 0;
        for gset in self
            .gain_set
            .iter_mut()
            .take(usize::from(self.gain_set_count))
        {
            for seq_index in gset
                .gain_sequence_index
                .iter()
                .take(usize::from(gset.band_count))
            {
                if *seq_index >= MAX_SEQUENCES {
                    continue;
                }
                self.gain_set_index_for_gain_sequence[usize::from(*seq_index)] = count;
            }
            count += 1;
        }
        Ok(())
    }
}
