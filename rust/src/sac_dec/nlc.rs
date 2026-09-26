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
//! SAC noiseless decoding tools (NLC) used in the GES and sac_bitdec
//! modules
//!
//! Consists of Huffman Coding and Differential Coding Techniques.

use super::constants::MAX_PARAMETER_BANDS;
use super::error_codes::SacDecoderError;
use crate::common::bitstream::Bitstream;

use super::common::DataType;
use super::huff_nodes::*;

// 1 bit info describing if differential decoding applied in
// frequency or in temporal domain.
#[derive(Clone, Copy, Debug, PartialEq)]
enum DiffType {
    Freq = 0x0,
    Time = 0x1,
}

impl From<u8> for DiffType {
    fn from(value: u8) -> Self {
        match value {
            0 => DiffType::Freq,
            1 => DiffType::Time,
            _ => panic!("invalid value for DiffType: {value}"),
        }
    }
}

impl From<DiffType> for usize {
    fn from(value: DiffType) -> Self {
        match value {
            DiffType::Freq => 0,
            DiffType::Time => 1,
        }
    }
}

// 1 bit info describing if 1D or 2D Huffman coding is applied.
#[derive(Debug, PartialEq)]
enum CodingScheme {
    Huff1d = 0x0,
    Huff2d = 0x1,
}

impl From<u8> for CodingScheme {
    fn from(value: u8) -> Self {
        match value {
            0 => CodingScheme::Huff1d,
            1 => CodingScheme::Huff2d,
            _ => panic!("invalid value for CodingScheme: {value}"),
        }
    }
}

// 1 bit info describing the pairing direction of the 2D Huffman code.
#[derive(Debug, PartialEq)]
enum Pairing {
    Pair = 0x0,
    Time = 0x1,
}

impl From<u8> for Pairing {
    fn from(value: u8) -> Self {
        match value {
            0 => Pairing::Pair,
            1 => Pairing::Time,
            _ => panic!("invalid value for Pairing: {value}"),
        }
    }
}

impl From<Pairing> for usize {
    fn from(value: Pairing) -> Self {
        match value {
            Pairing::Pair => 0,
            Pairing::Time => 1,
        }
    }
}

// 1 bit info describing if differential coding in time domain is calculated
// using the predecessor or successor frame.
#[derive(Debug, PartialEq)]
enum Direction {
    Backward = 0x0,
    Forward = 0x1,
}

impl From<u8> for Direction {
    fn from(value: u8) -> Self {
        match value {
            0 => Direction::Backward,
            1 => Direction::Forward,
            _ => panic!("invalid value for Direction: {value}"),
        }
    }
}

// Enum describing the low amplitude variation.
#[derive(Debug, Clone, Copy)]
enum Lav {
    Lav1 = 1,
    Lav3 = 3,
    Lav5 = 5,
    Lav7 = 7,
    Lav9 = 9,
}

impl From<u8> for Lav {
    fn from(value: u8) -> Self {
        match value {
            1 => Lav::Lav1,
            3 => Lav::Lav3,
            5 => Lav::Lav5,
            7 => Lav::Lav7,
            9 => Lav::Lav9,
            _ => panic!("invalid value for Lav: {value}"),
        }
    }
}

impl From<Lav> for u8 {
    fn from(lav: Lav) -> Self {
        lav as u8
    }
}

/// Syntactic element that contains one or two temporally subsequent parameter subsets of a given
/// parameter in the SAC frame. Holds information related to the envelope coding process.
///
/// # Parameters
/// - `bs`: A mutable reference to a `Bitstream` instance containing valid data.
/// - `aa_out_data1`: First output audio array.
/// - `aa_out_data2`: Second output audio array.
/// - `aa_history`: Previous state information.
/// - `data_type`: Is it CLD, ICC or IPD.
/// - `data_bands`: Number of data bands.
/// - `is_pair_flag_set`: Flag activating pairing.
/// - `is_coarse_flag_set`: Flag enabling coarse quantization.
/// - `is_allow_diff_time_back_flag_set`: Allows utilizing past frame data.
/// - `is_ld_mode_set`: Low-delay mode.
///
/// # Return
/// - `Ok(())` if the decoding and reshaping is successful.
/// - `Err(SacDecoderError)` if an error occurs.
pub(super) fn ec_data_pair_dec(
    bs: &mut Bitstream,
    aa_out_data1: &mut [i8; MAX_PARAMETER_BANDS],
    aa_out_data2: &mut [i8; MAX_PARAMETER_BANDS],
    a_history: &[i8; MAX_PARAMETER_BANDS],
    data_type: DataType,
    data_bands: usize,
    (is_pair_flag_set, is_coarse_flag_set, is_allow_diff_time_back_flag_set, is_ld_mode_set): (
        bool,
        bool,
        bool,
        bool,
    ),
) -> Result<(), SacDecoderError> {
    let err;
    let do_attach_lsb;
    let is_mixed_time_pair;
    let quant_levels: u8;
    let quant_offset;

    let num_val_pcm;

    let mut aa_data_pair0 = [0_i8; MAX_PARAMETER_BANDS];
    let mut aa_data_pair1 = [0_i8; MAX_PARAMETER_BANDS];
    let mut aa_data_diff0 = [0_i8; MAX_PARAMETER_BANDS];
    let mut aa_data_diff1 = [0_i8; MAX_PARAMETER_BANDS];

    let mut a_history_msb = [0_i8; MAX_PARAMETER_BANDS];

    let mut data_vec0: Option<&mut [i8; MAX_PARAMETER_BANDS]>;
    let mut data_vec1: Option<&mut [i8; MAX_PARAMETER_BANDS]>;

    let mut diff_type = [DiffType::Freq; 2];
    let mut pairing = Pairing::Pair;
    let mut direction = Direction::Backward;

    match data_type {
        DataType::Cld => {
            if is_coarse_flag_set {
                do_attach_lsb = false;
                quant_levels = 15;
                quant_offset = 7;
            } else {
                do_attach_lsb = false;
                quant_levels = 31;
                quant_offset = 15;
            }
        }
        DataType::Icc => {
            if is_coarse_flag_set {
                do_attach_lsb = false;
                quant_levels = 4;
                quant_offset = 0;
            } else {
                do_attach_lsb = false;
                quant_levels = 8;
                quant_offset = 0;
            }
        }
        DataType::Ipd => {
            if is_coarse_flag_set {
                do_attach_lsb = false;
                quant_levels = 8;
                quant_offset = 0;
            } else {
                do_attach_lsb = true;
                quant_levels = 16;
                quant_offset = 0;
            }
        }
    }

    let do_pcm_coding = bs.read_bit() != 0;

    if do_pcm_coding {
        if is_pair_flag_set {
            data_vec0 = Some(&mut aa_data_pair0);
            data_vec1 = Some(&mut aa_data_pair1);
            num_val_pcm = 2 * data_bands;
        } else {
            data_vec0 = Some(&mut aa_data_pair0);
            data_vec1 = None;
            num_val_pcm = data_bands;
        }

        err = pcm_decode(
            bs,
            &mut data_vec0,
            &mut data_vec1,
            quant_offset,
            num_val_pcm,
            quant_levels,
        );

        match err {
            Ok(_) => {}
            Err(e) => {
                return Err(e);
            }
        }
    } else {
        // Differential/Huffman/LSB Coding
        if is_pair_flag_set {
            data_vec0 = Some(&mut aa_data_diff0);
            data_vec1 = Some(&mut aa_data_diff1);
        } else {
            data_vec0 = Some(&mut aa_data_diff0);
            data_vec1 = None;
        }

        diff_type.fill(DiffType::Freq);

        {
            if is_pair_flag_set || is_allow_diff_time_back_flag_set {
                diff_type[0] = DiffType::from(bs.read_bit() as u8);
            }

            if is_pair_flag_set
                && (diff_type[0] == DiffType::Freq || is_allow_diff_time_back_flag_set)
            {
                diff_type[1] = DiffType::from(bs.read_bit() as u8);
            }
        }

        // Huffman decoding.
        err = huff_decode(
            bs,
            &mut data_vec0,
            &mut data_vec1,
            (data_type, diff_type[0], diff_type[1]),
            data_bands,
            &mut pairing,
            is_ld_mode_set,
        );
        match err {
            Ok(_) => {}
            Err(e) => {
                return Err(e);
            }
        }

        {
            // Differential decoding.
            if (diff_type[0] == DiffType::Time) || (diff_type[1] == DiffType::Time) {
                if is_ld_mode_set {
                    direction = Direction::Backward;
                } else if is_pair_flag_set {
                    if (diff_type[0] == DiffType::Time) && !is_allow_diff_time_back_flag_set {
                        direction = Direction::Forward;
                    } else if diff_type[1] == DiffType::Time {
                        direction = Direction::Backward;
                    } else {
                        direction = Direction::from(bs.read_bit() as u8);
                    }
                } else {
                    direction = Direction::Backward;
                }
            }

            is_mixed_time_pair = (diff_type[0] != diff_type[1]) && (pairing == Pairing::Time);

            if direction == Direction::Backward {
                if diff_type[0] == DiffType::Freq {
                    diff_freq_decode(&aa_data_diff0, &mut aa_data_pair0);
                } else {
                    a_history.iter().enumerate().for_each(|(i, &value)| {
                        a_history_msb[i] = value.wrapping_add(quant_offset as i8)
                            >> (if do_attach_lsb { 1 } else { 0 });
                    });

                    diff_time_decode_backward(
                        &a_history_msb,
                        &aa_data_diff0,
                        &mut aa_data_pair0,
                        is_mixed_time_pair,
                        data_bands,
                    );
                }
                if diff_type[1] == DiffType::Freq {
                    diff_freq_decode(&aa_data_diff1, &mut aa_data_pair1);
                } else {
                    diff_time_decode_backward(
                        &aa_data_pair0,
                        &aa_data_diff1,
                        &mut aa_data_pair1,
                        is_mixed_time_pair,
                        data_bands,
                    );
                }
            } else {
                // diff_type[1] must be DiffType::Freq.
                diff_freq_decode(&aa_data_diff1, &mut aa_data_pair1);

                if diff_type[0] == DiffType::Freq {
                    diff_freq_decode(&aa_data_diff0, &mut aa_data_pair0);
                } else {
                    diff_time_decode_forwards(
                        &aa_data_pair1,
                        &aa_data_diff0,
                        &mut aa_data_pair0,
                        is_mixed_time_pair,
                        data_bands,
                    );
                }
            }
        }
        // LSB decoding.
        attach_lsb(
            bs,
            quant_offset,
            do_attach_lsb,
            data_bands,
            &mut aa_data_pair0,
        );
        if is_pair_flag_set {
            attach_lsb(
                bs,
                quant_offset,
                do_attach_lsb,
                data_bands,
                &mut aa_data_pair1,
            );
        }
    }

    // Copy data to output arrays.
    aa_out_data1[..data_bands].copy_from_slice(&aa_data_pair0[..data_bands]);
    if is_pair_flag_set {
        aa_out_data2[..data_bands].copy_from_slice(&aa_data_pair1[..data_bands]);
    }

    err
}

/// Decodes the temporal envelope data of Guided Envelope Shaping (GES).
///
/// # Parameters
///
/// - `bs`: A mutable reference to a `Bitstream` instance containing valid data.
/// - `out_data`: A mutable slice to store the decoded output data.
/// - `num_timeslots`: The total number of time slots to process.
///
/// # Returns
///
/// - `Ok(())` if the decoding and reshaping is successful.
/// - `Err(SacDecoderError)` if an error occurs, data exceeds available time slots.
pub(super) fn huff_dec_reshape(
    bs: &mut Bitstream,
    out_data: &mut [u8],
    num_timeslots: usize,
) -> Result<(), SacDecoderError> {
    let mut val_received = 0;
    let mut rl_data = [0_i8; 2];

    while val_received < num_timeslots as u8 {
        // Escape value is not needed in this case, therefore providing false.
        huff_read_2d(bs, &HUFF_RESHAPE_NODES.node_tab, &mut rl_data, &mut false);
        let val = rl_data[0] as u8;
        let len = rl_data[1] as u8 + 1;

        if val_received + len > num_timeslots as u8 {
            return Err(SacDecoderError::ParseError);
        }

        let start_index = val_received as usize;
        let end_index = start_index + len as usize;
        out_data[start_index..end_index].fill(val);

        val_received += len;
    }

    Ok(())
}

fn pcm_decode(
    bs: &mut Bitstream,
    out_data_1: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
    out_data_2: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
    offset: u8,
    num_val: usize,
    num_levels: u8,
) -> Result<(), SacDecoderError> {
    let mut pcm_chunk_size = [0_u8; 7];

    let max_grp_len: u8 = match num_levels {
        3 => 5,
        7 => 6,
        11 => 2,
        13 => 4,
        19 => 4,
        25 => 3,
        51 => 4,
        4 | 8 | 15 | 16 | 26 | 31 => 1,
        _ => return Err(SacDecoderError::NotOk),
    };

    let mut tmp: u32 = 1;

    for pcm_chunk_size_element in pcm_chunk_size
        .iter_mut()
        .take(usize::from(max_grp_len) + 1)
        .skip(1)
    {
        tmp *= num_levels as u32;
        *pcm_chunk_size_element = ilog2(tmp);
    }

    for i in (0..num_val).step_by(usize::from(max_grp_len)) {
        let grp_len = usize::min(usize::from(max_grp_len), num_val - i);
        let data = bs.read(pcm_chunk_size[grp_len]);
        let mut grp_val = data;

        for j in 0..grp_len {
            let idx = i + (grp_len - j - 1);
            let next_val = grp_val % num_levels as u32;

            if out_data_2.is_none() {
                let out_data_1_unopt: &mut [i8; MAX_PARAMETER_BANDS] =
                    out_data_1.as_deref_mut().unwrap();
                out_data_1_unopt[idx] = (next_val as i32 - offset as i32) as i8;
            } else if out_data_1.is_none() {
                let out_data_2_unopt = out_data_2.as_deref_mut().unwrap();
                out_data_2_unopt[idx] = (next_val as i32 - offset as i32) as i8;
            } else {
                let out_data_1_unopt = out_data_1.as_deref_mut().unwrap();
                let out_data_2_unopt = out_data_2.as_deref_mut().unwrap();
                if (idx % 2) != 0 {
                    out_data_2_unopt[idx / 2] = (next_val as i32 - offset as i32) as i8;
                } else {
                    out_data_1_unopt[idx / 2] = (next_val as i32 - offset as i32) as i8;
                }
            }
            grp_val = (grp_val - next_val) / num_levels as u32;
        }
    }
    Ok(())
}

fn ilog2(i: u32) -> u8 {
    if i == 0 {
        return 0;
    }

    let i = i - 1;
    // Output data range 0-31.
    (u32::BITS - i.leading_zeros()) as u8
}

fn huff_decode(
    bs: &mut Bitstream,
    out_data_1: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
    out_data_2: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
    config: (DataType, DiffType, DiffType),
    num_val: usize,
    pairing_scheme: &mut Pairing,
    is_ld_mode_set: bool,
) -> Result<(), SacDecoderError> {
    let mut err: Result<(), SacDecoderError> = Ok(());
    let diff_type: DiffType;

    let mut pair_vec: [[i8; 2]; MAX_PARAMETER_BANDS] = [[0_i8; 2]; MAX_PARAMETER_BANDS];
    let mut p0_data_1_0: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]> = &mut None;
    let mut p0_data_1_1: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]> = &mut None;
    let p0_data_2_0: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]> = &mut None;
    let mut p0_data_2_1: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]> = &mut None;

    let mut p0_flag = [false, false];

    let mut num_val_1_int = num_val;
    let mut num_val_2_int = num_val;

    let mut offset_out_data_1 = 0_usize;
    let mut offset_out_data_2 = 0_usize;

    let (mut df_rest_flag_1, mut df_rest_flag_2) = (0, 0);

    let (huf_yy1, huf_yy2, huf_yy);
    let data_type = config.0;
    let diff_type_1 = config.1;
    let diff_type_2 = config.2;
    // Coding scheme
    let coding_scheme = CodingScheme::from(bs.read_bit() as u8);

    if coding_scheme == CodingScheme::Huff2d {
        if out_data_1.is_some() && out_data_2.is_some() && !is_ld_mode_set {
            *pairing_scheme = Pairing::from(bs.read_bit() as u8);
        } else {
            *pairing_scheme = Pairing::Pair;
        }
    }

    {
        huf_yy1 = diff_type_1;
        huf_yy2 = diff_type_2;
    }

    let out_data_1_is_some = out_data_1.is_some();
    let out_data_2_is_some = out_data_2.is_some();
    match coding_scheme {
        CodingScheme::Huff1d => {
            p0_flag[0] = diff_type_1 == DiffType::Freq;
            p0_flag[1] = diff_type_2 == DiffType::Freq;

            if out_data_1_is_some {
                let out_data_1_unopt = out_data_1.as_deref_mut().unwrap();
                err = huff_dec_1d(
                    bs,
                    data_type,
                    huf_yy1,
                    out_data_1_unopt,
                    num_val_1_int,
                    p0_flag[0],
                );
                match err {
                    Ok(_) => {}
                    Err(e) => {
                        return Err(e);
                    }
                }
            }
            if out_data_2.is_some() {
                let out_data_2_unopt = out_data_2.as_deref_mut().unwrap();

                err = huff_dec_1d(
                    bs,
                    data_type,
                    huf_yy2,
                    out_data_2_unopt,
                    num_val_2_int,
                    p0_flag[1],
                );
                match err {
                    Ok(_) => {}
                    Err(e) => {
                        return Err(e);
                    }
                }
            }
        }
        CodingScheme::Huff2d => match *pairing_scheme {
            Pairing::Pair => {
                if out_data_1_is_some {
                    if diff_type_1 == DiffType::Freq {
                        p0_data_1_0 = out_data_1;

                        match num_val_1_int.checked_sub(1) {
                            Some(result) => num_val_1_int = result,
                            None => return err,
                        }
                        offset_out_data_1 += 1;
                    }
                    df_rest_flag_1 = num_val_1_int % 2;
                    if df_rest_flag_1 != 0 {
                        match num_val_1_int.checked_sub(1) {
                            Some(result) => num_val_1_int = result,
                            None => return err,
                        }
                    }
                }

                if out_data_2_is_some {
                    if diff_type_2 == DiffType::Freq {
                        p0_data_2_1 = out_data_2;

                        match num_val_2_int.checked_sub(1) {
                            Some(result) => num_val_2_int = result,
                            None => return err,
                        }
                        offset_out_data_2 += 1;
                    }
                    df_rest_flag_2 = num_val_2_int % 2;
                    if df_rest_flag_2 != 0 {
                        match num_val_2_int.checked_sub(1) {
                            Some(result) => num_val_2_int = result,
                            None => return err,
                        }
                    }
                }

                if out_data_1_is_some {
                    err = huff_dec_2d(
                        bs,
                        (data_type, huf_yy1, Pairing::Pair),
                        &mut pair_vec,
                        num_val_1_int,
                        2,
                        p0_data_1_0,
                        p0_data_1_1,
                    );
                    match err {
                        Ok(_) => {}
                        Err(e) => {
                            return Err(e);
                        }
                    }
                    if df_rest_flag_1 != 0 {
                        let out_data_1_unopt = out_data_1.as_deref_mut().unwrap();
                        err = huff_dec_1d(
                            bs,
                            data_type,
                            huf_yy1,
                            &mut out_data_1_unopt[offset_out_data_1 + num_val_1_int..],
                            1,
                            false,
                        );
                        match err {
                            Ok(_) => {}
                            Err(e) => {
                                return Err(e);
                            }
                        }
                    }
                }

                if out_data_2_is_some {
                    err = huff_dec_2d(
                        bs,
                        (data_type, huf_yy2, Pairing::Pair),
                        &mut pair_vec[1..],
                        num_val_2_int,
                        2,
                        p0_data_2_0,
                        p0_data_2_1,
                    );

                    match err {
                        Ok(_) => {}
                        Err(e) => {
                            return Err(e);
                        }
                    }
                    if df_rest_flag_2 != 0 {
                        let out_data_2_unopt = out_data_2.as_deref_mut().unwrap();
                        err = huff_dec_1d(
                            bs,
                            data_type,
                            huf_yy2,
                            &mut out_data_2_unopt[offset_out_data_2 + num_val_2_int..],
                            1,
                            false,
                        );
                        match err {
                            Ok(_) => {}
                            Err(e) => {
                                return Err(e);
                            }
                        }
                    }
                }

                if out_data_1_is_some {
                    let out_data_1_unopt = out_data_1.as_deref_mut().unwrap();

                    match num_val_1_int.checked_sub(1) {
                        Some(_) => {}
                        None => return err,
                    }

                    for i in (0..num_val_1_int - 1).step_by(2) {
                        out_data_1_unopt[offset_out_data_1 + i] = pair_vec[i][0];
                        out_data_1_unopt[offset_out_data_1 + i + 1] = pair_vec[i][1];
                    }
                }
                if out_data_2_is_some {
                    let out_data_2_unopt = out_data_2.as_deref_mut().unwrap();

                    match num_val_2_int.checked_sub(1) {
                        Some(_) => {}
                        None => return err,
                    }

                    for i in (0..num_val_2_int - 1).step_by(2) {
                        out_data_2_unopt[offset_out_data_2 + i] = pair_vec[i + 1][0];
                        out_data_2_unopt[offset_out_data_2 + i + 1] = pair_vec[i + 1][1];
                    }
                }
            }
            Pairing::Time => {
                if (diff_type_1 == DiffType::Freq) || (diff_type_2 == DiffType::Freq) {
                    p0_data_1_0 = out_data_1;
                    p0_data_1_1 = out_data_2;

                    offset_out_data_1 += 1;
                    offset_out_data_2 += 1;

                    match num_val_1_int.checked_sub(1) {
                        Some(result) => num_val_1_int = result,
                        None => return err,
                    }
                }

                if (diff_type_1 == DiffType::Time) || (diff_type_2 == DiffType::Time) {
                    diff_type = DiffType::Time;
                } else {
                    diff_type = DiffType::Freq;
                }
                {
                    huf_yy = diff_type;
                }

                err = huff_dec_2d(
                    bs,
                    (data_type, huf_yy, Pairing::Time),
                    &mut pair_vec,
                    num_val_1_int,
                    1,
                    p0_data_1_0,
                    p0_data_1_1,
                );

                match err {
                    Ok(_) => {}
                    Err(e) => {
                        return Err(e);
                    }
                }

                for (i, pair_vec_element) in pair_vec.iter().enumerate().take(num_val_1_int) {
                    (out_data_1.as_mut()).unwrap()[offset_out_data_1 + i] = pair_vec_element[0];
                    (out_data_2.as_mut()).unwrap()[offset_out_data_2 + i] = pair_vec_element[1];
                }
            }
        },
    }
    err
}

fn huff_dec_1d(
    bs: &mut Bitstream,
    data_type: DataType,
    dim1: DiffType,
    out_data: &mut [i8],
    num_val: usize,
    p0_flag: bool,
) -> Result<(), SacDecoderError> {
    let mut node = 0;
    let mut offset = 0;
    let mut od;
    let mut od_sign;
    let mut data;
    let mut bits_avail;

    let part_tab: &[[i16; 2]];
    let node_tab: &[[i16; 2]];

    let dim1_usize = usize::from(dim1);

    match data_type {
        DataType::Cld => {
            part_tab = &HUFF_PART0_NODES.cld;
            node_tab = &HUFF_CLD_NODES.h_1d[dim1_usize].node_tab;
        }
        DataType::Icc => {
            part_tab = &HUFF_PART0_NODES.icc;
            node_tab = &HUFF_ICC_NODES.h_1d[dim1_usize].node_tab;
        }
        DataType::Ipd => {
            part_tab = &HUFF_PART0_NODES.ipd;
            node_tab = &HUFF_IPD_NODES.h_1d[dim1_usize].node_tab;
        }
    }

    if p0_flag {
        huff_read(bs, part_tab, &mut node);
        out_data[0] = -(node as i8 + 1);
        offset = 1;
    }

    for out_data_element in out_data.iter_mut().take(num_val).skip(offset) {
        bits_avail = bs.valid_bits();
        if bits_avail < 1 {
            return Err(SacDecoderError::NotOk);
        }

        huff_read(bs, node_tab, &mut node);
        od = -(node as i8 + 1);

        if data_type != DataType::Ipd && od != 0 {
            bits_avail = bs.valid_bits();
            if bits_avail < 1 {
                return Err(SacDecoderError::NotOk);
            }

            data = bs.read_bit();
            od_sign = data;

            if od_sign != 0 {
                od = -od;
            }
        }
        *out_data_element = od;
    }
    Ok(())
}

fn huff_dec_2d(
    bs: &mut Bitstream,
    config: (DataType, DiffType, Pairing),
    out_data: &mut [[i8; 2]],
    num_val: usize,
    stride: u8,
    p0_data0: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
    p0_data1: &mut Option<&mut [i8; MAX_PARAMETER_BANDS]>,
) -> Result<(), SacDecoderError> {
    let mut err: Result<(), SacDecoderError> = Ok(());

    let mut escape = false;
    let mut esc_cntr = 0;
    let mut node = 0;

    let mut esc_data_1 = [0_i8; MAX_PARAMETER_BANDS];
    let mut esc_data_2 = [0_i8; MAX_PARAMETER_BANDS];
    let mut esc_idx = [0_usize; MAX_PARAMETER_BANDS];

    let data_type = config.0;
    let dim1 = usize::from(config.1);
    let dim2 = usize::from(config.2);
    let mut node_tab: &[[i16; 2]];

    huff_read(bs, &HUFF_LAV_IDX_NODES.node_tab, &mut node);
    let mut data = (-(node as i8 + 1)) as u8;

    let lav;

    match data_type {
        DataType::Cld => {
            // lav: 3, 5, 7, 9
            lav = Lav::from(2 * data + 3);
            node_tab = &HUFF_PART0_NODES.cld;
        }
        DataType::Icc => {
            // lav: 1, 3, 5, 7
            lav = Lav::from(2 * data + 1);
            node_tab = &HUFF_PART0_NODES.icc;
        }
        DataType::Ipd => {
            if data == 0 {
                data = 3;
            } else {
                data -= 1;
            }
            // 1, 3, 5, 7
            lav = Lav::from(2 * data + 1);
            node_tab = &HUFF_PART0_NODES.ipd;
        }
    }

    // Partition 0.
    if p0_data0.is_some() {
        let p0_data0_unopt = p0_data0.as_deref_mut().unwrap();

        huff_read(bs, node_tab, &mut node);
        p0_data0_unopt[0] = -(node as i8 + 1);
    }
    if p0_data1.is_some() {
        let p0_data1_unopt = p0_data1.as_deref_mut().unwrap();
        huff_read(bs, node_tab, &mut node);
        p0_data1_unopt[0] = -(node as i8 + 1);
    }

    match data_type {
        DataType::Cld => match lav {
            Lav::Lav3 => node_tab = &HUFF_CLD_NODES.h_2d[dim1][dim2].lav3,
            Lav::Lav5 => node_tab = &HUFF_CLD_NODES.h_2d[dim1][dim2].lav5,
            Lav::Lav7 => node_tab = &HUFF_CLD_NODES.h_2d[dim1][dim2].lav7,
            Lav::Lav9 => node_tab = &HUFF_CLD_NODES.h_2d[dim1][dim2].lav9,
            _ => panic!("Invalid Lav value for DataType::CLD"),
        },
        DataType::Icc => match lav {
            Lav::Lav1 => node_tab = &HUFF_ICC_NODES.h_2d[dim1][dim2].lav1,
            Lav::Lav3 => node_tab = &HUFF_ICC_NODES.h_2d[dim1][dim2].lav3,
            Lav::Lav5 => node_tab = &HUFF_ICC_NODES.h_2d[dim1][dim2].lav5,
            Lav::Lav7 => node_tab = &HUFF_ICC_NODES.h_2d[dim1][dim2].lav7,
            _ => panic!("Invalid Lav value for DataType::ICC"),
        },
        DataType::Ipd => match lav {
            Lav::Lav1 => node_tab = &HUFF_IPD_NODES.h_2d[dim1][dim2].lav1,
            Lav::Lav3 => node_tab = &HUFF_IPD_NODES.h_2d[dim1][dim2].lav3,
            Lav::Lav5 => node_tab = &HUFF_IPD_NODES.h_2d[dim1][dim2].lav5,
            Lav::Lav7 => node_tab = &HUFF_IPD_NODES.h_2d[dim1][dim2].lav7,
            _ => panic!("Invalid Lav value for DataType::IPD"),
        },
    }

    for i in (0..num_val).step_by(stride as usize) {
        huff_read_2d(bs, node_tab, &mut out_data[i], &mut escape);
        if escape {
            esc_idx[esc_cntr] = i;
            esc_cntr += 1;
        } else if data_type == DataType::Ipd {
            sym_restore(bs, lav as i8, &mut out_data[i], true);
        } else {
            sym_restore(bs, lav as i8, &mut out_data[i], false);
        }
    }

    if esc_cntr > 0 {
        err = pcm_decode(
            bs,
            &mut Some(&mut esc_data_1),
            &mut Some(&mut esc_data_2),
            0,
            2 * esc_cntr,
            2 * u8::from(lav) + 1,
        );
        match err {
            Ok(_) => {}
            Err(e) => {
                return Err(e);
            }
        }

        for i in 0..esc_cntr {
            out_data[esc_idx[i]][0] = esc_data_1[i] - lav as i8;
            out_data[esc_idx[i]][1] = esc_data_2[i] - lav as i8;
        }
    }
    err
}

fn sym_restore(bs: &mut Bitstream, lav: i8, data: &mut [i8; 2], is_ipd: bool) {
    let mut sym_bit;

    let sum_val = data[0] + data[1];
    let diff_val = data[0] - data[1];

    if sum_val > lav {
        data[0] = -sum_val + (2 * lav + 1);
        data[1] = -diff_val;
    } else {
        data[0] = sum_val;
        data[1] = diff_val;
    }

    if !is_ipd && (data[0] + data[1] != 0) {
        sym_bit = bs.read_bit() != 0;
        if sym_bit {
            data[0] = -data[0];
            data[1] = -data[1];
        }
    }

    if data[0] - data[1] != 0 {
        sym_bit = bs.read_bit() != 0;
        if sym_bit {
            data.swap(0, 1);
        }
    }
}

fn diff_freq_decode(
    diff_data: &[i8; MAX_PARAMETER_BANDS],
    out_data: &mut [i8; MAX_PARAMETER_BANDS],
) {
    out_data[0] = diff_data[0];

    for (i, diff) in diff_data.iter().enumerate().skip(1) {
        out_data[i] = out_data[i - 1].wrapping_add(*diff);
    }
}

fn diff_time_decode_backward(
    prev_data: &[i8; MAX_PARAMETER_BANDS],
    diff_data: &[i8; MAX_PARAMETER_BANDS],
    out_data: &mut [i8; MAX_PARAMETER_BANDS],
    mixed_diff_type: bool,
    num_val: usize,
) {
    let x = if mixed_diff_type {
        out_data[0] = diff_data[0];
        // New start value.
        1
    } else {
        // Default start value.
        0
    };

    for i in x..num_val {
        out_data[i] = prev_data[i].wrapping_add(diff_data[i]);
    }
}

fn diff_time_decode_forwards(
    prev_data: &[i8; MAX_PARAMETER_BANDS],
    diff_data: &[i8; MAX_PARAMETER_BANDS],
    out_data: &mut [i8; MAX_PARAMETER_BANDS],
    mixed_diff_type: bool,
    num_val: usize,
) {
    let x = if mixed_diff_type {
        out_data[0] = diff_data[0];
        // New start value.
        1
    } else {
        // Default start value.
        0
    };
    for i in x..num_val {
        out_data[i] = prev_data[i].wrapping_sub(diff_data[i]);
    }
}

fn attach_lsb(
    bs: &mut Bitstream,
    offset: u8,
    do_attach_lsb: bool,
    num_val: usize,
    in_out_data: &mut [i8; MAX_PARAMETER_BANDS],
) {
    // A local i8 variant is used so that less casts have to be made when
    // working with in_out_data.
    let offset_i8 = offset as i8;

    for io_data_element in in_out_data.iter_mut().take(num_val) {
        let msb = *io_data_element;
        *io_data_element = if do_attach_lsb {
            ((msb << 1) | (bs.read_bit() as i8)) - offset_i8
        } else {
            msb.wrapping_sub(offset_i8)
        };
    }
}

fn huff_read_2d(
    bs: &mut Bitstream,
    node_tab: &[[i16; 2]],
    out_data: &mut [i8; 2],
    escape: &mut bool,
) {
    let mut huff_2d_8bit = 0;
    let mut node = 0;

    huff_read(bs, node_tab, &mut node);

    *escape = node == 0;

    if *escape {
        out_data[0] = 0;
        out_data[1] = 1;
    } else {
        huff_2d_8bit -= node + 1;
        out_data[0] = (huff_2d_8bit >> 4) as i8;
        out_data[1] = (huff_2d_8bit & 0xf) as i8;
    }
}

fn huff_read(bs: &mut Bitstream, node_tab: &[[i16; 2]], out_data: &mut i16) {
    let mut node = 0;
    loop {
        let next_bit = bs.read_bit() as usize;
        node = node_tab[node as usize][next_bit];
        if node <= 0 {
            break;
        }
    }

    *out_data = node;
}
