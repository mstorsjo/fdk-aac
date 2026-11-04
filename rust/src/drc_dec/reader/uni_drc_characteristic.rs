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
//! MPEG-D DRC's compressor characteristics.

use super::super::{constants::*, drc_error::DrcError};
use crate::common::bitstream::Bitstream;
use itertools::izip;

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Structure that holds parameters of `sigmoidal function` for customized DRC compression
/// characteristics.
pub(in super::super) struct CustomDrcCharSigmoid {
    /// Encoded gain value in bitstream.
    pub(in super::super) gain: f32,
    /// I/O ratio value.
    pub(in super::super) io_ratio: f32,
    /// Exponent value.
    pub(in super::super) exp: f32,
    /// Flag to indicate if the DRC characteristic is multiplied by -1.
    pub(in super::super) is_flip_sign: bool,
}

impl CustomDrcCharSigmoid {
    /// Creates a new instance.
    pub fn _new() -> Self {
        Default::default()
    }

    /// Calculates `out = inp / pow(1.0 +/- pow(inp/gain_db_limit, exp), 1.0/exp);`
    /// # Return
    ///   - Ok(out_gain_db);
    ///   - Err(e);
    fn compressor_io_sigmoid_common(
        inp: f32,
        gain_db_limit: f32,
        exp: f32,
        do_inverse: bool,
    ) -> Result<f32, DrcError> {
        if exp < 1.0 || gain_db_limit == 0.0 || exp == 0.0 {
            return Err(DrcError::NotOk);
        }

        let x = inp / gain_db_limit;
        if x < 0.0 {
            return Err(DrcError::NotOk);
        }

        let tmp1 = if do_inverse {
            -(x.powf(exp))
        } else {
            x.powf(exp)
        };
        let tmp2 = 1.0 + tmp1;
        if tmp2 <= 0.0 {
            return Err(DrcError::NotOk);
        }

        let inv_denom = tmp2.powf(-1.0 / exp);

        // out = inp / pow(1.0 +/- pow(x, exp), 1.0/exp);
        Ok(inp * inv_denom)
    }

    /// Calculates `out_gain_db`, based on `in_level_db`.
    pub(in super::super) fn compressor_io_sigmoid(
        &self,
        in_level_db: f32,
    ) -> Result<f32, DrcError> {
        let mut out_gain_db;

        let tmp = (DRC_INPUT_LOUDNESS_TARGET - in_level_db) * self.io_ratio;
        if self.exp < EXP_INFINITY {
            // out_gain_db = tmp / pow(1.0 + pow(tmp/gain_db_limit, exp), 1.0/exp);
            out_gain_db = Self::compressor_io_sigmoid_common(tmp, self.gain, self.exp, false)?;
        } else {
            out_gain_db = tmp;
        }
        if self.is_flip_sign {
            out_gain_db = -out_gain_db;
        }

        Ok(out_gain_db)
    }

    pub(in super::super) fn compressor_io_sigmoid_inverse(
        &self,
        gain_db: f32,
    ) -> Result<f32, DrcError> {
        let mut tmp = if self.is_flip_sign { -gain_db } else { gain_db };
        if self.exp < EXP_INFINITY {
            // tmp = tmp / pow(1.0 - pow(tmp/gain_db_limit, exp), 1.0/exp);
            tmp = Self::compressor_io_sigmoid_common(tmp, self.gain, self.exp, true)?;
        }

        if self.io_ratio == 0.0 {
            Err(DrcError::NotOk)
        } else {
            let in_lev = DRC_INPUT_LOUDNESS_TARGET - (tmp / self.io_ratio);
            Ok(in_lev)
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Structure that holds nodes for customized DRC compression characteristics.
pub(in super::super) struct CustomDrcCharNodes {
    /// Number of nodes.
    pub(in super::super) characteristic_node_count: u8,
    /// Node level value.
    pub(in super::super) node_level: [f32; MAX_CUSTOM_CHARACTERISTIC_NODES + 1],
    /// Node gain value.
    pub(in super::super) node_gain: [f32; MAX_CUSTOM_CHARACTERISTIC_NODES + 1],
}

impl CustomDrcCharNodes {
    /// Creates a new instance.
    pub fn _new() -> Self {
        Default::default()
    }

    pub(in super::super) fn compressor_io_nodes(&self, in_level_db: f32) -> Result<f32, DrcError> {
        let node_count = usize::from(self.characteristic_node_count);
        let is_out_interpol;
        let mut out_gain_db;

        if in_level_db < DRC_INPUT_LOUDNESS_TARGET {
            (out_gain_db, is_out_interpol) = Self::compressor_io_nodes_left(
                in_level_db,
                &self.node_gain,
                &self.node_level,
                node_count,
            );
            if is_out_interpol {
                return Ok(out_gain_db);
            }
        } else {
            (out_gain_db, is_out_interpol) = Self::compressor_io_nodes_right(
                in_level_db,
                &self.node_gain,
                &self.node_level,
                node_count,
            );
            if is_out_interpol {
                return Ok(out_gain_db);
            }
        }
        out_gain_db = self.node_gain[node_count];
        Ok(out_gain_db)
    }

    pub(in super::super) fn compressor_io_nodes_inverse(
        &self,
        gain_db: f32,
    ) -> Result<f32, DrcError> {
        let mut in_lev;

        let node_count = usize::from(self.characteristic_node_count);
        let gain_is_negative = self
            .node_gain
            .iter()
            .skip(1)
            .take(node_count)
            .any(|&x| x < 0.0);

        if gain_is_negative {
            if gain_db <= self.node_gain[node_count] {
                in_lev = self.node_level[node_count];
            } else if gain_db >= 0.0 {
                in_lev = DRC_INPUT_LOUDNESS_TARGET;
            } else {
                let is_out_interpol;
                (in_lev, is_out_interpol) = Self::compressor_io_nodes_left(
                    gain_db,
                    &self.node_level,
                    &self.node_gain,
                    node_count,
                );
                if is_out_interpol {
                    return Ok(in_lev);
                }
                in_lev = self.node_level[node_count];
            }
        } else if gain_db >= self.node_gain[node_count] {
            in_lev = self.node_level[node_count];
        } else if gain_db <= 0.0 {
            in_lev = DRC_INPUT_LOUDNESS_TARGET;
        } else {
            let is_out_interpol;
            (in_lev, is_out_interpol) = Self::compressor_io_nodes_right(
                gain_db,
                &self.node_level,
                &self.node_gain,
                node_count,
            );
            if is_out_interpol {
                return Ok(in_lev);
            }
            in_lev = self.node_level[node_count];
        }
        Ok(in_lev)
    }

    fn compressor_io_nodes_left(
        inp: f32,
        y_axys: &[f32; 5],
        x_axys: &[f32; 5],
        node_count: usize,
    ) -> (f32, bool) {
        let mut is_out_interpolated = false;

        let w;
        let mut out = 0.0;
        for (a_win, b_win) in izip!(y_axys.windows(2), x_axys.windows(2)).take(node_count) {
            let curr_node_a = a_win[0];
            let next_node_a = a_win[1];

            let curr_node_b = b_win[0];
            let next_node_b = b_win[1];

            if curr_node_b >= inp && inp > next_node_b {
                is_out_interpolated = true;
                let delta = curr_node_b - next_node_b;
                if delta == 0.0 {
                    out = curr_node_a;
                    return (out, is_out_interpolated);
                }
                w = (inp - next_node_b) / delta;
                out = Self::lerp(w, curr_node_a, next_node_a);
                return (out, is_out_interpolated);
            }
        }
        // is_out_interpolated
        (out, is_out_interpolated)
    }

    fn compressor_io_nodes_right(
        inp: f32,
        y_axys: &[f32; 5],
        x_axys: &[f32; 5],
        node_count: usize,
    ) -> (f32, bool) {
        let mut is_out_interpolated = false;

        let w;
        let mut out = 0.0;
        for (a_win, b_win) in izip!(y_axys.windows(2), x_axys.windows(2)).take(node_count) {
            let curr_node_a = a_win[0];
            let next_node_a = a_win[1];

            let curr_node_b = b_win[0];
            let next_node_b = b_win[1];

            if curr_node_b <= inp && inp < next_node_b {
                is_out_interpolated = true;
                let delta = next_node_b - curr_node_b;
                if delta == 0.0 {
                    out = curr_node_a;
                    return (out, is_out_interpolated);
                }
                w = (next_node_b - inp) / delta;
                out = Self::lerp(w, curr_node_a, next_node_a);
                return (out, is_out_interpolated);
            }
        }
        // is_out_interpolated
        (out, is_out_interpolated)
    }

    #[inline(always)]
    fn lerp(w: f32, a: f32, b: f32) -> f32 {
        w * a + (1.0 - w) * b
    }
}

#[repr(C)]
#[derive(Default, Copy, Clone, Debug, PartialEq)]
/// Customized compression(DRC) characteristics. It shall be sigmoidal function or nodes.
pub(in super::super) struct CustomDrcChar {
    /// Sigmoidal function.
    pub(in super::super) sigmoid: CustomDrcCharSigmoid,
    /// Nodes (linear interpolation).
    pub(in super::super) nodes: CustomDrcCharNodes,
}

#[repr(C)]
#[derive(Debug)]
/// Parts of characteristic side.
pub(in super::super) enum CharacteristicSide {
    /// Left part of characteristic.
    Left = 0,
    /// Right part of characteristic.
    Right,
}

impl From<CharacteristicSide> for usize {
    fn from(value: CharacteristicSide) -> usize {
        match value {
            CharacteristicSide::Left => 0,
            CharacteristicSide::Right => 1,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Representation of characteristic format.
pub(in super::super) enum CharacteristicFormat {
    #[default]
    /// Sigmoidal function.
    Sigmoid = 0,
    /// Segment-wise (Nodes b/w linear interpolation).
    Nodes = 1,
}

impl From<bool> for CharacteristicFormat {
    fn from(value: bool) -> Self {
        match value {
            false => CharacteristicFormat::Sigmoid,
            true => CharacteristicFormat::Nodes,
        }
    }
}

impl CustomDrcChar {
    /// Reads `CustomDrcChar` data from bitstream and updates `char_format`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `char_format`: Valid `CharacteristicFormat` instance.
    /// - `char_side`: Value of enum `CharacteristicSide`.
    ///
    ///  Returns `DrcError`.
    pub(super) fn read(
        &mut self,
        bs: &mut Bitstream,
        char_format: &mut CharacteristicFormat,
        char_side: CharacteristicSide,
    ) -> Result<(), DrcError> {
        *char_format = CharacteristicFormat::from(bs.read_bit() != 0);

        match *char_format {
            CharacteristicFormat::Sigmoid => {
                let bs_gain = bs.read(6) as f32;
                self.sigmoid.gain = match char_side {
                    CharacteristicSide::Left => bs_gain,
                    CharacteristicSide::Right => -bs_gain,
                };

                let bs_io_ratio = bs.read(4) as f32;
                self.sigmoid.io_ratio = 0.05_f32 + bs_io_ratio * 0.15_f32;

                let bs_exp = bs.read(4) as u8;
                self.sigmoid.exp = if bs_exp < 15 {
                    (1 + 2 * bs_exp) as f32
                } else {
                    EXP_INFINITY
                };

                self.sigmoid.is_flip_sign = bs.read_bit() != 0;
            }
            CharacteristicFormat::Nodes => {
                self.nodes.characteristic_node_count = bs.read(2) as u8 + 1;

                self.nodes.node_level[0] = DRC_INPUT_LOUDNESS_TARGET;
                self.nodes.node_gain[0] = 0.0_f32;

                let mut prev_node_level = self.nodes.node_level[0];

                for (level, gain) in izip!(
                    self.nodes.node_level[1..].iter_mut(),
                    self.nodes.node_gain[1..].iter_mut()
                )
                .take(usize::from(self.nodes.characteristic_node_count))
                {
                    let bs_node_level_delta = (bs.read(5) + 1) as f32;
                    *level = match char_side {
                        CharacteristicSide::Left => prev_node_level - bs_node_level_delta,
                        CharacteristicSide::Right => prev_node_level + bs_node_level_delta,
                    };
                    prev_node_level = *level;

                    let bs_node_gain = bs.read(8) as f32;
                    *gain = bs_node_gain * 0.5_f32 - 64.0_f32;
                }
            }
        }
        Ok(())
    }
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone, PartialEq)]
/// Customized compression(DRC) characteristics. It shall be sigmoidal function or nodes.
pub(in super::super) struct CustomDrcCharRef<'s> {
    /// Sigmoidal function.
    pub(in super::super) sigmoid: Option<&'s CustomDrcCharSigmoid>,
    /// Nodes (linear interpolation).
    pub(in super::super) nodes: Option<&'s CustomDrcCharNodes>,
}

impl CustomDrcCharRef<'_> {
    pub(in super::super) fn _new() -> Self {
        Default::default()
    }

    /// Returns the sign of the `inp`.
    fn signum(inp: f32) -> i32 {
        if inp < 0.0 {
            -1
        } else if inp > 0.0 {
            1
        } else {
            0
        }
    }

    pub(in super::super) fn get_slope_sign(
        &self,
        char_format: CharacteristicFormat,
    ) -> Result<i32, DrcError> {
        let mut slope_sign = 0;

        match char_format {
            CharacteristicFormat::Sigmoid => {
                if let Some(sigmoid) = self.sigmoid {
                    slope_sign = if sigmoid.is_flip_sign { 1 } else { -1 };
                }
            }
            CharacteristicFormat::Nodes => {
                let mut tmp_slope_sign;
                slope_sign = 0;
                if let Some(nodes) = self.nodes {
                    for (ng_win, nl_win) in
                        izip!(nodes.node_gain.windows(2), nodes.node_level.windows(2),)
                            .take(usize::from(nodes.characteristic_node_count))
                    {
                        let curr_node_g = ng_win[0];
                        let next_node_g = ng_win[1];

                        let curr_node_l = nl_win[0];
                        let next_node_l = nl_win[1];

                        tmp_slope_sign = if next_node_l > curr_node_l {
                            Self::signum(next_node_g - curr_node_g)
                        } else {
                            -Self::signum(next_node_g - curr_node_g)
                        };
                        if (slope_sign != 0 || tmp_slope_sign != 0)
                            && (slope_sign == -tmp_slope_sign)
                        {
                            // DRC characteristic is not invertible
                            return Err(DrcError::NotOk);
                        } else {
                            slope_sign = tmp_slope_sign;
                        }
                    }
                }
            }
        }
        Ok(slope_sign)
    }
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Index of source DRC characteristic of the gain sequence.
struct CustomIndex {
    left: u8,
    right: u8,
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// DRC characteristic.
pub(in super::super) struct DrcCharacteristic {
    /// Flag indicating whether a source characteristic for the gain sequence is presentor not.
    is_present: bool,
    /// Flag indicating whether the index of the characteristic is based on ISO/IEC 23091-3 or not.
    is_cicp: bool,
    /// CICP index value.
    cicp_index: u8,
    /// Index of source DRC characteristic.
    custom_index: CustomIndex,
}

impl DrcCharacteristic {
    pub(in super::super) fn is_present(&self) -> bool {
        self.is_present
    }

    pub(in super::super) fn is_cicp(&self) -> bool {
        self.is_cicp
    }

    pub(in super::super) fn cicp_index(&self) -> u8 {
        self.cicp_index
    }

    pub(in super::super) fn custom_index(&self, drc_char_side: CharacteristicSide) -> u8 {
        match drc_char_side {
            CharacteristicSide::Left => self.custom_index.left,
            CharacteristicSide::Right => self.custom_index.right,
        }
    }

    /// Reads `DrcCharacteristic` data from bitstream.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `version`: Parameter to control which syntax version to use, V0 or v1.
    pub(super) fn read(&mut self, bs: &mut Bitstream, version: u8) {
        if version == 0 {
            self.cicp_index = bs.read(7) as u8;
            if self.cicp_index > 0 {
                self.is_present = true;
                self.is_cicp = true;
            } else {
                self.is_present = false;
            }
        } else {
            self.is_present = bs.read_bit() != 0;
            if self.is_present {
                self.is_cicp = bs.read_bit() != 0;
                if self.is_cicp {
                    self.cicp_index = bs.read(7) as u8;
                } else {
                    self.custom_index.left = bs.read(4) as u8;
                    self.custom_index.right = bs.read(4) as u8;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::CustomDrcCharNodes;

    fn t_loop(node_a: &[f32; 5], node_b: &[f32; 5]) {
        println!("node_a = {:?}, node_b = {:?}", node_a, node_b);
        for i in 0..10 {
            let inp = 0.2 + i as f32;
            let (out, is_interpol) =
                CustomDrcCharNodes::compressor_io_nodes_left(inp, node_a, node_b, 4);
            println!(
                "inp = {}, out = {}, is_interpol = {}",
                inp, out, is_interpol
            );
        }
    }

    #[test]
    fn t_run_interpol_a() {
        let node_a = [8.0, 6.0, 3.0, 2.0, 1.0];
        let node_b = [80.0, 60.0, 30.0, 9.0, 8.0];

        t_loop(&node_a, &node_b);
        println!();

        t_loop(&node_b, &node_a);
        println!();

        let node_a = [0.0, 2.0, 3.0, 6.0, 8.0];
        let node_b = [0.0, 20.0, 30.0, 60.0, 80.0];

        t_loop(&node_a, &node_b);
        println!();

        t_loop(&node_b, &node_a);
        println!();
    }
}
