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
//! Spatial Audio Coding bit decoder.

use super::{
    calc_m2::M2Data,
    common::{
        BsFramingType, BsQuantCoarseXxx, BsXxxDataMode, DataType, OpdSmoothingMode, PhaseCoding,
        QuantMode, TsConf,
    },
    conceal::ConcealmentInfo,
    constants::{MAX_PARAMETER_BANDS, MAX_PARAMETER_SETS, MAX_TIME_SLOTS},
    error_codes::SacDecoderError,
    ges::GesData,
    nlc,
    sac_map::{self, PbStride, PB_STRIDE_TABLE},
    ssc::SpatialSpecificConfig,
    stp::StpDec,
    tables::TEMP_SHAPE_CHAN_TABLE,
    tsd::TsdData,
};
use crate::common::bitstream::Bitstream;
use itertools::izip;

/// Lossless data of spatial dec parameters (i.e. CLD, ICC, IPD).
#[repr(C)]
#[derive(Default, Debug)]
pub(super) struct LosslessData {
    /// An array of `BsXxxDataMode`, which indicates whether and how new information for a
    /// parameter subset is encoded. `xxx` denotes the type of spatial parameter: `CLD`, `ICC` or
    /// `IPD.`
    bs_xxx_data_mode: [BsXxxDataMode; MAX_PARAMETER_SETS],
    /// Coarse quantized spatial parameter sets read from bitstream.
    bs_quant_coarse_xxx: [BsQuantCoarseXxx; MAX_PARAMETER_SETS],
    /// Frequency resolution stride of spatial parameter sets (i.e. indices to the
    /// `PB_STRIDE_TABLE`).
    bs_freq_res_stride_xxx: [u8; MAX_PARAMETER_SETS],
    /// Uncompressed coarse quantized spatial parameter sets.
    no_cmp_quant_coarse_xxx: [BsQuantCoarseXxx; MAX_PARAMETER_SETS],
    /// Previous coarse quantized spatial parameter set read from bitstream.
    bs_quant_coarse_xxx_prev: BsQuantCoarseXxx,
    /// Previous parsed coarse quantized spatial parameter set read from bitstream.
    bs_quant_coarse_xxx_prev_parse: BsQuantCoarseXxx,
}

impl LosslessData {
    /// Returns the frequency resolution stride of spatial parameter sets.
    pub(super) fn bs_freq_res_stride_xxx(&self, idx: usize) -> u8 {
        self.bs_freq_res_stride_xxx[idx]
    }

    /// Returns the coarse quantized spatial parameter sets read from bitstream.
    pub(super) fn bs_quant_coarse_xxx(&self, idx: usize) -> BsQuantCoarseXxx {
        self.bs_quant_coarse_xxx[idx]
    }

    /// Returns the previous parsed coarse quantized spatial parameter set read from bitstream.
    pub(super) fn bs_quant_coarse_xxx_prev_parse(&self) -> BsQuantCoarseXxx {
        self.bs_quant_coarse_xxx_prev_parse
    }

    /// Returns an immutable reference to `BsXxxDataMode` array. The array indicates whether and
    /// how new information for a parameter subset is encoded.
    pub(super) fn bs_xxx_data_mode(&self) -> &[BsXxxDataMode; MAX_PARAMETER_SETS] {
        &self.bs_xxx_data_mode
    }

    /// Returns a mutable reference to `BsXxxDataMode` array. The array indicates whether and how
    /// new information for a parameter subset is encoded.
    pub(super) fn bs_xxx_data_mode_mut(&mut self) -> &mut [BsXxxDataMode; MAX_PARAMETER_SETS] {
        &mut self.bs_xxx_data_mode
    }

    /// Performs sanity checks, after reading `num_param_sets` of `BsXxxDataMode` from `Bitstream`.
    pub(super) fn check_data_mode(
        &self,
        independcy_flag: bool,
        num_param_sets: usize,
    ) -> Result<(), SacDecoderError> {
        // This check catches bitstreams generated by older encoder that cause trouble.
        if independcy_flag
            && (self.bs_xxx_data_mode[0] == BsXxxDataMode::Keep
                || self.bs_xxx_data_mode[0] == BsXxxDataMode::Interpolate)
        {
            return Err(SacDecoderError::ParseError);
        }

        // The interpolation mode must not be active for the last parameter set.
        if self.bs_xxx_data_mode[num_param_sets - 1] == BsXxxDataMode::Interpolate {
            return Err(SacDecoderError::ParseError);
        }

        Ok(())
    }

    /// Extends the number of quantized coarse spatial parameters sets, in case of an extended
    /// frame where the transmited spatial parameter set is `DataType::IPD`.
    ///
    /// # Parameters
    ///
    /// - `data_type`: Type of spatial audio decoder parameter.
    /// - `num_param_sets`: Number of spatial audio decoder parameter sets.
    pub(super) fn extend_ipd_param_data(&mut self, data_type: DataType, num_param_sets: usize) {
        if data_type == DataType::Ipd {
            self.bs_quant_coarse_xxx[num_param_sets] = self.bs_quant_coarse_xxx[num_param_sets - 1];
        }
    }

    /// Groups the spatial parameters as data pairs.
    pub(super) fn group_params_as_data_pair(&mut self, set_idx: usize) {
        self.bs_quant_coarse_xxx[set_idx + 1] = self.bs_quant_coarse_xxx[set_idx];
        self.bs_freq_res_stride_xxx[set_idx + 1] = self.bs_freq_res_stride_xxx[set_idx];
    }

    /// Interpolates the SAC data passed as an argument `output_idx_data`, if
    /// `BsXxxDataMode::Interpolate` is signaled for a SAC parameter set.
    pub(super) fn interpolate_index_data(
        &self,
        output_idx_data: &mut [[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        param_slots: &[u8; MAX_PARAMETER_SETS],
        stop_band: usize,
        num_param_sets: usize,
    ) -> Result<(), SacDecoderError> {
        let mut i1 = 0;
        // Stores a local copy of interpolated output for a particular parameter set. The values
        // are further copied to output_idx_data[ps], when BsXxxDataMode::Interpolate.
        let mut yi = [0_i16; MAX_PARAMETER_BANDS];

        for (ps, data_mode) in self
            .bs_xxx_data_mode
            .iter()
            .enumerate()
            .take(num_param_sets)
        {
            if *data_mode != BsXxxDataMode::Interpolate {
                i1 = ps;
            } else {
                let i2 = num_param_sets - 1;
                let x1 = param_slots[i1] as i16;
                let xi = param_slots[ps] as i16;
                let x2 = param_slots[i2] as i16;

                let idx_i1 = &output_idx_data[i1];
                let idx_i2 = &output_idx_data[i2];

                if x1 != x2 {
                    for (y1, y2, yi) in
                        izip!(idx_i1.iter(), idx_i2.iter(), yi.iter_mut()).take(stop_band)
                    {
                        *yi = i16::from(*y1)
                            + (xi - x1) * (i16::from(*y2) - i16::from(*y1)) / (x2 - x1);
                    }
                } else {
                    for (yi, xi) in izip!(yi.iter_mut(), idx_i1.iter()).take(stop_band) {
                        *yi = i16::from(*xi);
                    }
                }
                // Sanity check. The results should be in i8 range.
                if yi.iter().any(|&y| y < i8::MIN.into() || y > i8::MAX.into()) {
                    return Err(SacDecoderError::NotOk);
                }

                // Copy to output.
                for (out, yi) in izip!(output_idx_data[ps].iter_mut(), yi.iter()).take(stop_band) {
                    *out = *yi as i8;
                }
            }
        }

        Ok(())
    }

    /// Maps all coarse data to fine.
    pub(super) fn map_coarse_data_to_full_res(&mut self, num_param_sets: usize) {
        self.no_cmp_quant_coarse_xxx[..num_param_sets].fill(BsQuantCoarseXxx::FullRes);
    }

    /// Returns the uncompressed coarse quantized spatial parameter sets.
    pub(super) fn no_cmp_quant_coarse_xxx(&self) -> &[BsQuantCoarseXxx; MAX_PARAMETER_SETS] {
        &self.no_cmp_quant_coarse_xxx
    }

    /// Prepares the SAC data.
    pub(super) fn prepare_index_data(
        &mut self,
        num_param_sets: usize,
    ) -> Result<(), SacDecoderError> {
        let mut set_idx = 0;
        for (bs_mode, uncompressed_quant_coarse) in izip!(
            self.bs_xxx_data_mode.iter(),
            self.no_cmp_quant_coarse_xxx.iter_mut(),
        )
        .take(num_param_sets)
        {
            match bs_mode {
                BsXxxDataMode::Dflt => {
                    *uncompressed_quant_coarse = BsQuantCoarseXxx::FullRes;
                    self.bs_quant_coarse_xxx_prev = BsQuantCoarseXxx::FullRes;
                }
                BsXxxDataMode::Keep | BsXxxDataMode::Interpolate => {
                    *uncompressed_quant_coarse = self.bs_quant_coarse_xxx_prev;
                }
                BsXxxDataMode::LosslesslyCoded => {
                    let bs_quant_coarse = self.bs_quant_coarse_xxx[set_idx];
                    *uncompressed_quant_coarse = bs_quant_coarse;
                    self.bs_quant_coarse_xxx_prev = bs_quant_coarse;
                    set_idx += 1;
                }
                BsXxxDataMode::Invalid => {
                    return Err(SacDecoderError::WrongParametersets);
                }
            }
        }
        Ok(())
    }

    /// Read `BsXxxDataMode` data from `Bitstream`, for the SAC `num_param_sets`.
    pub(super) fn read_bs_mode(&mut self, bs: &mut Bitstream, num_param_sets: usize) {
        for bs_mode in self.bs_xxx_data_mode.iter_mut().take(num_param_sets) {
            *bs_mode = BsXxxDataMode::from(bs.read(2));
        }
    }

    /// Sets the frequency resolution stride of spatial parameter sets.
    pub(super) fn set_bs_freq_res_stride_xxx(&mut self, idx: usize, val: u8) {
        self.bs_freq_res_stride_xxx[idx] = val;
    }

    /// Sets the coarse quantized spatial parameter sets read from bitstream.
    pub(super) fn set_bs_quant_coarse_xxx(&mut self, idx: usize, val: BsQuantCoarseXxx) {
        self.bs_quant_coarse_xxx[idx] = val;
    }

    /// Sets the previous parsed coarse quantized spatial parameter set read from bitstream.
    pub(super) fn set_bs_quant_coarse_xxx_prev_parse(&mut self, bs_quant_prev: BsQuantCoarseXxx) {
        self.bs_quant_coarse_xxx_prev_parse = bs_quant_prev;
    }
}

/// Structure that holds spatial parameter data read from bitstream.
#[repr(C)]
#[derive(Default, Debug)]
pub(super) struct BsData {
    /// Indicates if lossless coding of current SAC frame is done independently of previous SAC
    /// frame.
    independency_flag: bool,
    /// Indicates whether the IPD parameters are available for the current Mps212Data frame. If the
    /// value is set to `false`, the IPD parameters are set to zero. Otherwise, the quantized IPD
    /// indices are losslessly decoded from the bitstream.
    phase_mode: bool,
    /// Number of spatial parameter sets (groups). Range: `[1..MAX_PARAMETER_SETS]`. A value of `0`
    /// indicates corrupted signal. A value of `MAX_PARAMETER_SETS` indicates an extended frame.
    num_parameter_sets: u8,
    /// Defines the time slot to which each parameter set applies.
    param_time_slot: [u8; MAX_PARAMETER_SETS],
    /// Sets of compressed One-to-Two Box: Channel Level Difference indices.
    cmp_ott_cld_idx: [[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
    /// Sets of compressed One-to-Two Box: Inter Channel Correlation indices.
    cmp_ott_icc_idx: [[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
    /// Sets of compressed One-to-Two Box: Inter Channel Phase Difference indices.
    cmp_ott_ipd_idx: [[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],

    /// Previous set of One-to-Two Box: Channel Level Difference indices.
    ott_cld_idx_prev: [i8; MAX_PARAMETER_BANDS],
    /// Previous set of One-to-Two Box: Inter Channel Correlation indices.
    ott_icc_idx_prev: [i8; MAX_PARAMETER_BANDS],
    /// Previous set of One-to-Two Box: Inter Channel Phase Difference indices.
    ott_ipd_idx_prev: [i8; MAX_PARAMETER_BANDS],

    /// Previous set of compressed One-to-Two Box: Channel Level Difference indices.
    cmp_ott_cld_idx_prev: [i8; MAX_PARAMETER_BANDS],
    /// Previous set of compressed One-to-Two Box: Inter Channel Correlation indices.
    cmp_ott_icc_idx_prev: [i8; MAX_PARAMETER_BANDS],
    /// Previous set of compressed One-to-Two Box: Inter Channel Phase Difference indices.
    cmp_ott_ipd_idx_prev: [i8; MAX_PARAMETER_BANDS],

    /// Channel Level Difference lossless data.
    cld_lossless_data: LosslessData,
    /// Inter Channel Correlation lossless data.
    icc_lossless_data: LosslessData,
    /// Inter Channel Phase Difference lossless data.
    ipd_lossless_data: LosslessData,
}

impl BsData {
    /// Suggests corrupt signal.
    fn bail(&mut self, e: SacDecoderError) -> Result<(), SacDecoderError> {
        self.num_parameter_sets = 0;
        Err(e)
    }

    /// Returns a coarsed quantized spatial parameter, belonging to a parameter set.
    ///
    /// # Parameters
    ///
    /// - `data_type`: The spatial parameter data type.
    /// - `idx`: Parameter set index (up to `MAX_PARAMETER_SETS-1`).
    ///
    /// # Return
    ///
    /// - `BsQuantCoarseXxx`
    pub(super) fn bs_quant_coarse_for_param_slot(
        &self,
        data_type: DataType,
        idx: usize,
    ) -> BsQuantCoarseXxx {
        match data_type {
            DataType::Cld => self.cld_lossless_data.bs_quant_coarse_xxx(idx),
            DataType::Icc => self.icc_lossless_data.bs_quant_coarse_xxx(idx),
            DataType::Ipd => self.ipd_lossless_data.bs_quant_coarse_xxx(idx),
        }
    }

    /// Clears frame data.
    ///
    /// # Parameters
    ///
    /// - `num_time_slots`: Number of time slots in a frame (Range: `[1..MAX_TIME_SLOTS]`).
    pub(super) fn clear_frame_data(&mut self, num_time_slots: u8) {
        self.num_parameter_sets = 1;
        self.param_time_slot[0] = num_time_slots - 1;
    }

    /// Parameter index mapping from coarse to fine quantization.
    fn coarse2fine(sac_data: &mut [i8], data_type: DataType) {
        for d in sac_data.iter_mut() {
            *d <<= 1;
        }

        if data_type == DataType::Cld {
            for d in sac_data.iter_mut() {
                if *d == -14 {
                    *d = -15;
                } else if *d == 14 {
                    *d = 15;
                }
            }
        }
    }

    /// Decodes a SAC frame.
    ///
    /// # Parameters
    ///
    /// - `m2_data`: Mix matrix `M2`, for re-creating a multi-channel output.
    /// - `tsd_data`: Transient steering decorrelator.
    /// - `conceal_info`: Concealment of SAC decoder parameters.
    /// - `quant_mode`: Quantization mode.
    /// - `num_time_slots`: Number of time slots in a frame (Range: `[1..MAX_TIME_SLOTS]`).
    /// - `is_extended_frame`: Perform SAC processing for an extended frame.
    ///
    /// # Return
    ///
    /// - `Result<(), SacDecoderError>`
    pub(super) fn decode_frame(
        &mut self,
        m2_data: &mut M2Data,
        tsd_data: &mut TsdData,
        conceal_info: &mut ConcealmentInfo,
        quant_mode: QuantMode,
        num_time_slots: u8,
        is_extended_frame: bool,
    ) -> Result<(), SacDecoderError> {
        tsd_data.clear_ts();

        // ****** DTDF and MAP DATA ********
        self.decode_and_map_frame_ott(m2_data, conceal_info, quant_mode)?;

        if is_extended_frame {
            self.extend_frame(m2_data, num_time_slots)?;
        }

        Ok(())
    }

    /// Does delta decoding and dequantization.
    fn decode_and_map_frame_ott(
        &mut self,
        m2_data: &mut M2Data,
        conceal_info: &mut ConcealmentInfo,
        quant_mode: QuantMode,
    ) -> Result<(), SacDecoderError> {
        let num_param_bands = m2_data.num_parameter_bands();
        if quant_mode != QuantMode::FineDef {
            return Err(SacDecoderError::WrongQuantmode);
        }

        // CLD
        let _ = conceal_info.apply(
            &self.cmp_ott_cld_idx,
            &mut self.ott_cld_idx_prev,
            self.cld_lossless_data.bs_xxx_data_mode_mut(),
            usize::from(num_param_bands),
            usize::from(self.num_parameter_sets),
        );

        self.map_index_data(m2_data.ott_cld_mut(), DataType::Cld, num_param_bands)?;

        // ICC
        let _ = conceal_info.apply(
            &self.cmp_ott_icc_idx,
            &mut self.ott_icc_idx_prev,
            self.icc_lossless_data.bs_xxx_data_mode_mut(),
            usize::from(num_param_bands),
            usize::from(self.num_parameter_sets),
        );

        self.map_index_data(m2_data.ott_icc_mut(), DataType::Icc, num_param_bands)?;

        if m2_data.phase_coding() != PhaseCoding::NoIpd {
            let num_ott_bands_ipd = m2_data.num_ott_bands_ipd();

            if !self.phase_mode {
                self.ott_ipd_idx_prev[..usize::from(num_ott_bands_ipd)].fill(0);
            }

            // IPD
            let _ = conceal_info.apply(
                &self.cmp_ott_ipd_idx,
                &mut self.ott_ipd_idx_prev,
                self.ipd_lossless_data.bs_xxx_data_mode_mut(),
                usize::from(num_ott_bands_ipd),
                usize::from(self.num_parameter_sets),
            );

            self.map_index_data(m2_data.ott_ipd_mut(), DataType::Ipd, num_ott_bands_ipd)?;
        }

        Ok(())
    }

    /// Dequantizes an index value of type `DataType`.
    fn deq_idx(value: i8, param_type: DataType) -> u8 {
        let mut idx = 0;
        match param_type {
            DataType::Cld => {
                let tmp_cld = i16::from(value) + 15;
                if (0..31).contains(&tmp_cld) {
                    idx = tmp_cld as u8;
                }
            }
            DataType::Icc => {
                if (0..8).contains(&value) {
                    idx = value as u8;
                }
            }
            DataType::Ipd => {
                idx = (value & 0xf) as u8;
            }
        }

        idx
    }

    /// Dequantizes the index values for every `stop_band` parameter bands and `num_param_sets`
    /// parameter sets.
    fn dequantize_index_data(
        output_idx_data: &mut [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        input_idx_data: &[[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        data_type: DataType,
        stop_band: usize,
        num_param_sets: usize,
    ) {
        for (deq_ps_data, quant_ps_data) in
            izip!(output_idx_data.iter_mut(), input_idx_data.iter()).take(num_param_sets)
        {
            for (deq_b, q_b) in izip!(deq_ps_data.iter_mut(), quant_ps_data.iter()).take(stop_band)
            {
                *deq_b = Self::deq_idx(*q_b, data_type);
            }
        }
    }

    fn ec_data_dec(
        &mut self,
        bs: &mut Bitstream,
        data_type: DataType,
        stop_band: u8,
        is_eld: bool,
    ) -> Result<(), SacDecoderError> {
        let num_bands = usize::from(stop_band);
        let num_param_sets = usize::from(self.num_parameter_sets);

        let (lossless_data, cmp_idx_data, cmp_idx_data_prev) = match data_type {
            DataType::Cld => (
                &mut self.cld_lossless_data,
                &mut self.cmp_ott_cld_idx,
                &mut self.cmp_ott_cld_idx_prev,
            ),
            DataType::Icc => (
                &mut self.icc_lossless_data,
                &mut self.cmp_ott_icc_idx,
                &mut self.cmp_ott_icc_idx_prev,
            ),
            DataType::Ipd => (
                &mut self.ipd_lossless_data,
                &mut self.cmp_ott_ipd_idx,
                &mut self.cmp_ott_ipd_idx_prev,
            ),
        };

        lossless_data.read_bs_mode(bs, num_param_sets);
        lossless_data.check_data_mode(self.independency_flag, num_param_sets)?;

        let mut a_strides = [0_u8; MAX_PARAMETER_BANDS + 1];
        let mut set_idx = 0;
        let mut bs_data_pair = false;
        let mut old_quant_coarse_xxx = lossless_data.bs_quant_coarse_xxx_prev_parse();

        for ps in 0..num_param_sets {
            if lossless_data.bs_xxx_data_mode[ps] == BsXxxDataMode::Dflt {
                old_quant_coarse_xxx = BsQuantCoarseXxx::FullRes;
                cmp_idx_data_prev[..num_bands].fill(0);
            }

            if lossless_data.bs_xxx_data_mode[ps] == BsXxxDataMode::LosslesslyCoded {
                if bs_data_pair {
                    bs_data_pair = false;
                } else {
                    bs_data_pair = bs.read_bit() != 0;
                    lossless_data
                        .set_bs_quant_coarse_xxx(set_idx, BsQuantCoarseXxx::from(bs.read_bit()));
                    lossless_data.set_bs_freq_res_stride_xxx(set_idx, bs.read(2) as u8);

                    let bs_quant_coarse_xxx = lossless_data.bs_quant_coarse_xxx(set_idx);
                    if bs_quant_coarse_xxx != old_quant_coarse_xxx {
                        if old_quant_coarse_xxx != BsQuantCoarseXxx::FullRes {
                            Self::coarse2fine(&mut cmp_idx_data_prev[..num_bands], data_type);
                        } else {
                            Self::fine2coarse(&mut cmp_idx_data_prev[..num_bands], data_type);
                        }
                    }

                    let data_bands = Self::get_stride_map(
                        usize::from(lossless_data.bs_freq_res_stride_xxx(set_idx)),
                        stop_band,
                        &mut a_strides,
                    );

                    for pb in 0..data_bands {
                        cmp_idx_data_prev[pb] = cmp_idx_data_prev[usize::from(a_strides[pb])];
                    }

                    if set_idx + usize::from(bs_data_pair) > MAX_PARAMETER_SETS {
                        return Err(SacDecoderError::InvalidSetidx);
                    }

                    // -------------------------------------------------------------------- //
                    let (data1, data2) = cmp_idx_data.split_at_mut(set_idx + 1);

                    let tmp_err = nlc::ec_data_pair_dec(
                        bs,
                        data1.last_mut().unwrap(),
                        data2.first_mut().unwrap(),
                        cmp_idx_data_prev,
                        data_type,
                        data_bands,
                        (
                            bs_data_pair,
                            bs_quant_coarse_xxx != BsQuantCoarseXxx::FullRes,
                            !(self.independency_flag && (ps == 0)) || (set_idx > 0),
                            is_eld,
                        ),
                    );

                    tmp_err?;
                    // -------------------------------------------------------------------- //

                    let mask = if data_type == DataType::Ipd {
                        if bs_quant_coarse_xxx != BsQuantCoarseXxx::FullRes {
                            0x07
                        } else {
                            0x0F
                        }
                    } else {
                        //0xFF as i8
                        -1i8
                    };

                    for pb in 0..data_bands {
                        for j in a_strides[pb]..a_strides[pb + 1] {
                            cmp_idx_data_prev[usize::from(j)] =
                                cmp_idx_data[set_idx + usize::from(bs_data_pair)][pb] & mask;
                        }
                    }

                    old_quant_coarse_xxx = bs_quant_coarse_xxx;

                    if bs_data_pair {
                        lossless_data.group_params_as_data_pair(set_idx);
                    }
                    set_idx += usize::from(bs_data_pair) + 1;
                }
            }
        }
        lossless_data.set_bs_quant_coarse_xxx_prev_parse(old_quant_coarse_xxx);

        Ok(())
    }

    /// Extends spatial parameters.
    fn extend_frame(
        &mut self,
        m2_data: &mut M2Data,
        num_time_slots: u8,
    ) -> Result<(), SacDecoderError> {
        let mut err = Ok(());

        let num_param_sets = usize::from(self.num_parameter_sets);
        let stop_band = usize::from(m2_data.num_parameter_bands());

        m2_data.extend_frame(DataType::Cld, num_param_sets, stop_band)?;
        m2_data.extend_frame(DataType::Icc, num_param_sets, stop_band)?;
        if m2_data.phase_coding() != PhaseCoding::NoIpd {
            self.ipd_lossless_data
                .extend_ipd_param_data(DataType::Ipd, num_param_sets);
            m2_data.extend_frame(DataType::Ipd, num_param_sets, stop_band)?;
        }

        // Increment the number of parameter set by 1, up to the maximum possible value.
        self.extend_num_parameter_sets();
        let num_param_sets = usize::from(self.num_parameter_sets);
        self.param_time_slot[num_param_sets - 1] = num_time_slots - 1;

        for ps in self.param_time_slot.iter_mut().take(num_param_sets) {
            if *ps > num_time_slots - 1 {
                *ps = num_time_slots - 1;
                err = Err(SacDecoderError::ParseError);
            }
        }

        err
    }

    /// Extends the number of parameters sets by 1 (up to `MAX_PARAMETER_SETS`).
    #[inline(always)]
    fn extend_num_parameter_sets(&mut self) {
        self.num_parameter_sets = (self.num_parameter_sets + 1).min(MAX_PARAMETER_SETS as u8);
    }

    /// Parameter index mapping from fine to coarse quantization.
    fn fine2coarse(sac_data: &mut [i8], data_type: DataType) {
        // Note: CLD can also have negative values.
        if data_type == DataType::Cld {
            for d in sac_data.iter_mut() {
                *d /= 2;
            }
        } else {
            for d in sac_data.iter_mut() {
                *d >>= 1;
            }
        }
    }

    /// Index Mapping accroding to PbStrides.
    fn get_stride_map(
        freq_res_stride: usize,
        stop_band: u8,
        a_strides: &mut [u8; MAX_PARAMETER_BANDS + 1],
    ) -> usize {
        let pb_stride = PB_STRIDE_TABLE[freq_res_stride];

        let stride_i16 = i16::from(pb_stride);
        let in_bands = i16::from(stop_band);
        // The result is always >= 0.
        let tmp = ((in_bands - 1) / stride_i16) + 1;
        let data_bands = (tmp as usize).min(MAX_PARAMETER_BANDS);

        a_strides[0] = 0;
        let mut prev_out = a_strides[0];
        for out in a_strides.iter_mut().skip(1).take(data_bands) {
            *out = prev_out + pb_stride;
            prev_out = *out;
        }

        let mut str_offset = 0;
        while a_strides[data_bands] > stop_band {
            if str_offset < data_bands {
                str_offset += 1;
            }
            for a in a_strides
                .iter_mut()
                .skip(str_offset)
                .take(data_bands - str_offset + 1)
            {
                *a -= 1;
            }
        }

        data_bands
    }

    /// Indicates if lossless coding of current SAC frame is done independently of previous SAC
    /// frame.
    pub(super) fn independency_flag(&self) -> bool {
        self.independency_flag
    }

    /// Initializes `BsData`.
    pub(super) fn init_data(&mut self) -> Result<(), SacDecoderError> {
        self.num_parameter_sets = 1;
        Ok(())
    }

    /// Initializes the OTT data.
    pub(super) fn init_parser_context(&mut self) {
        self.ott_cld_idx_prev.fill(0);
        self.ott_icc_idx_prev.fill(0);
        self.cmp_ott_cld_idx_prev.fill(0);
        self.cmp_ott_icc_idx_prev.fill(0);
    }

    /// Checks if the SAC frame is extended frame.
    pub(super) fn is_extended_frame(&self, num_time_slots: u8) -> bool {
        self.param_time_slot[usize::from(self.num_parameter_sets) - 1] != num_time_slots - 1
    }

    /// Maps SAC data, by preparing and dequantizing it.
    fn map_index_data(
        &mut self,
        output_data_idx: &mut [[u8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        data_type: DataType,
        stop_band: u8,
    ) -> Result<(), SacDecoderError> {
        let (lossless_data, cmp_idx_data, idx_prev) = match data_type {
            DataType::Cld => (
                &mut self.cld_lossless_data,
                &self.cmp_ott_cld_idx,
                &mut self.ott_cld_idx_prev,
            ),
            DataType::Icc => (
                &mut self.icc_lossless_data,
                &self.cmp_ott_icc_idx,
                &mut self.ott_icc_idx_prev,
            ),
            DataType::Ipd => (
                &mut self.ipd_lossless_data,
                &self.cmp_ott_ipd_idx,
                &mut self.ott_ipd_idx_prev,
            ),
        };

        let num_param_sets = usize::from(self.num_parameter_sets);
        if num_param_sets > (MAX_PARAMETER_SETS - 1) {
            return Err(SacDecoderError::WrongParametersets);
        }

        let mut tmp_output_idx_data = [[0_i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS];

        Self::prepare_index_data(
            &mut tmp_output_idx_data,
            lossless_data,
            cmp_idx_data,
            idx_prev,
            data_type,
            stop_band,
            num_param_sets,
        )?;

        lossless_data.interpolate_index_data(
            &mut tmp_output_idx_data,
            &self.param_time_slot,
            usize::from(stop_band),
            num_param_sets,
        )?;

        Self::dequantize_index_data(
            output_data_idx,
            &tmp_output_idx_data,
            data_type,
            usize::from(stop_band),
            num_param_sets,
        );

        Ok(())
    }

    /// Creates a new instance of 'BsData'.
    pub(super) fn new() -> Self {
        Default::default()
    }

    /// Returns the number of bits in which `num_time_slots` is represented. Returns with
    /// `SacDecoderError::ParseError`, if `num_time_slots > MAX_TIME_SLOTS`.
    fn num_bits_param_slot(num_time_slots: u8) -> Result<u8, SacDecoderError> {
        let mut bits_param_slot = if num_time_slots > 1 {
            num_time_slots.ilog2() as u8
        } else {
            0
        };

        if 1 << bits_param_slot < num_time_slots {
            bits_param_slot += 1;
        }

        if (1 << bits_param_slot) > MAX_TIME_SLOTS {
            return Err(SacDecoderError::ParseError);
        }

        Ok(bits_param_slot)
    }

    /// Returns the number of SAC parameter sets.
    pub(super) fn num_parameter_sets(&self) -> u8 {
        self.num_parameter_sets
    }

    /// Returns the time slot to which each parameter set applies.
    pub(super) fn param_time_slots(&self) -> &[u8; MAX_PARAMETER_SETS] {
        &self.param_time_slot
    }

    /// Returns the time slot to which the `idx` param set applies to?
    pub(super) fn param_time_slot(&self, idx: usize) -> u8 {
        self.param_time_slot[idx]
    }

    /// Parses SAC frame data.
    #[expect(clippy::too_many_arguments)]
    pub(super) fn parse_frame_data(
        &mut self,
        m2_data: &mut M2Data,
        tsd_data: &mut TsdData,
        stp: &mut StpDec,
        ges: &mut GesData,
        bs: &mut Bitstream,
        ssc: &SpatialSpecificConfig,
        global_independency_flag: bool,
    ) -> Result<(), SacDecoderError> {
        // Anchor for byte_align() function.
        let align_anchor = bs.valid_bits();

        let is_eld = ssc.is_eld();
        let num_time_slots = ssc.n_time_slots();

        let bs_framing_type = Self::parse_bs_framing_type(bs, ssc)?;

        self.parse_num_parameter_sets(bs, ssc)?;

        // Basic config check.
        if ssc.n_input_channels() == 0 || ssc.n_output_channels() == 0 {
            self.bail(SacDecoderError::UnsupportedConfig)?;
        }

        self.parse_param_time_slots(bs, bs_framing_type, num_time_slots)?;

        self.parse_independency_flag(bs, is_eld, global_independency_flag)?;

        // OTT Data
        let err = self.ec_data_dec(bs, DataType::Cld, u8::from(ssc.freq_res()), is_eld);
        match err {
            Ok(_) => {}
            Err(e) => {
                self.bail(e)?;
            }
        }

        let err = self.ec_data_dec(bs, DataType::Icc, u8::from(ssc.freq_res()), is_eld);
        match err {
            Ok(_) => {}
            Err(e) => {
                self.bail(e)?;
            }
        }

        let num_param_sets = usize::from(self.num_parameter_sets);

        let mut opd_smoothing_mode = OpdSmoothingMode::Disabled;

        if ssc.bs_phase_coding() != 0 {
            let num_ott_bands_ipd = usize::from(ssc.num_ott_bands_ipd());

            self.phase_mode = bs.read_bit() != 0;

            if !self.phase_mode {
                self.cmp_ott_ipd_idx_prev[..num_ott_bands_ipd].fill(0);

                for ps in self.cmp_ott_ipd_idx.iter_mut().take(num_param_sets) {
                    ps[..num_ott_bands_ipd].fill(0);
                }
            } else {
                opd_smoothing_mode = OpdSmoothingMode::from(bs.read_bit() as u8);

                let err = self.ec_data_dec(bs, DataType::Ipd, ssc.num_ott_bands_ipd(), is_eld);
                match err {
                    Ok(_) => {}
                    Err(e) => {
                        self.bail(e)?;
                    }
                }
            }
        }

        let is_extended_frame = self.is_extended_frame(num_time_slots);

        m2_data.read_smg_data(
            bs,
            ssc.freq_res(),
            ssc.bs_high_rate_mode() != 0,
            num_param_sets,
            opd_smoothing_mode,
            is_extended_frame,
            is_eld,
        );

        let temp_shape_config = ssc.temp_shape_config();

        if !is_eld && (temp_shape_config == TsConf::Nots) {
            let err = tsd_data.read(bs, usize::from(ssc.n_time_slots()));
            if err {
                self.bail(SacDecoderError::ParseError)?;
            }
        } else {
            tsd_data.clear();
        }

        stp.clear_data();
        ges.clear_data();

        if temp_shape_config == TsConf::TpWhite || temp_shape_config == TsConf::Tes {
            let bs_temp_shape_enable = bs.read_bit() != 0;

            if bs_temp_shape_enable {
                let num_temp_shape_chan =
                    usize::from(TEMP_SHAPE_CHAN_TABLE[usize::from(temp_shape_config) - 1]);

                match temp_shape_config {
                    TsConf::TpWhite => {
                        stp.read(bs, num_temp_shape_chan);
                    }
                    TsConf::Tes => {
                        let err = ges.read(bs, num_temp_shape_chan, usize::from(num_time_slots));
                        match err {
                            Ok(_) => {}
                            Err(e) => {
                                self.bail(e)?;
                            }
                        }
                    }
                    _ => {
                        self.bail(SacDecoderError::InvalidTempshape)?;
                    }
                }
            }
        }

        if is_eld {
            // ISO/IEC 23003-1:2007. Ch. 5.2. ... byte alignment with respect to the beginning of
            // the syntactic element in which ByteAlign() occurs.
            bs.align(align_anchor);
        }
        Ok(())
    }

    /// Parses bitstream framing type bit.
    fn parse_bs_framing_type(
        bs: &mut Bitstream,
        ssc: &SpatialSpecificConfig,
    ) -> Result<BsFramingType, SacDecoderError> {
        let bs_framing_type = if ssc.is_eld() || ssc.bs_high_rate_mode() != 0 {
            BsFramingType::try_from(bs.read_bit())?
        } else {
            BsFramingType::Fixed
        };

        Ok(bs_framing_type)
    }

    /// Parses the frame independency flag.
    fn parse_independency_flag(
        &mut self,
        bs: &mut Bitstream,
        is_eld: bool,
        global_independency_flag: bool,
    ) -> Result<(), SacDecoderError> {
        self.independency_flag = if !is_eld && global_independency_flag {
            true
        } else {
            bs.read_bit() != 0
        };
        Ok(())
    }

    /// Parses the `param_time_slot[]` array.
    fn parse_param_time_slots(
        &mut self,
        bs: &mut Bitstream,
        bs_framing_type: BsFramingType,
        num_time_slots: u8,
    ) -> Result<(), SacDecoderError> {
        match bs_framing_type {
            BsFramingType::Variable => {
                self.read_param_time_slots(
                    bs,
                    num_time_slots,
                    usize::from(self.num_parameter_sets),
                )?;
            }
            BsFramingType::Fixed => {
                let n_time_slots = u16::from(num_time_slots);
                let n_param_sets = u16::from(self.num_parameter_sets);

                for (i, ps) in self
                    .param_time_slot
                    .iter_mut()
                    .enumerate()
                    .take(usize::from(self.num_parameter_sets))
                {
                    // It is safe to convert back to u8, as the result cannot overflow.
                    *ps = (((n_time_slots * (i as u16 + 1)) / n_param_sets) - 1) as u8;
                }
            }
        }

        Ok(())
    }

    /// Parses the number of parameter sets. `num_parameter_sets = bs_num_param_sets + 1`.
    fn parse_num_parameter_sets(
        &mut self,
        bs: &mut Bitstream,
        ssc: &SpatialSpecificConfig,
    ) -> Result<(), SacDecoderError> {
        if ssc.is_eld() {
            self.num_parameter_sets = 1 + bs.read_bit() as u8;
        } else if ssc.bs_high_rate_mode() != 0 {
            self.num_parameter_sets = 1 + bs.read(3) as u8;
        } else {
            self.num_parameter_sets = 1;
        }

        if self.num_parameter_sets >= MAX_PARAMETER_SETS as u8 {
            self.bail(SacDecoderError::ParseError)?;
        }

        Ok(())
    }

    /// Returns the phase mode.
    pub(super) fn phase_mode(&self) -> bool {
        self.phase_mode
    }

    /// Prepares the SAC data.
    fn prepare_index_data(
        output_idx_data: &mut [[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        lossless_data: &mut LosslessData,
        cmp_idx_data: &[[i8; MAX_PARAMETER_BANDS]; MAX_PARAMETER_SETS],
        idx_prev: &mut [i8; MAX_PARAMETER_BANDS],
        data_type: DataType,
        stop_band: u8,
        num_param_sets: usize,
    ) -> Result<(), SacDecoderError> {
        let mut set_idx = 0;
        let num_bands = usize::from(stop_band);

        for (bs_mode, out_idx_data) in izip!(
            lossless_data.bs_xxx_data_mode().iter(),
            output_idx_data.iter_mut(),
        )
        .take(num_param_sets)
        {
            match bs_mode {
                BsXxxDataMode::Dflt => {
                    out_idx_data[..num_bands].fill(0);
                    idx_prev[..num_bands].fill(0);
                }
                BsXxxDataMode::Keep | BsXxxDataMode::Interpolate => {
                    out_idx_data.copy_from_slice(idx_prev);
                }
                BsXxxDataMode::LosslesslyCoded => {
                    let stride =
                        PB_STRIDE_TABLE[usize::from(lossless_data.bs_freq_res_stride_xxx(set_idx))];

                    let stride_i16 = i16::from(stride);
                    let in_bands = i16::from(stop_band);
                    // The result is always >= 0.
                    let tmp = ((in_bands - 1) / stride_i16) + 1;
                    let data_bands = (tmp as usize).min(MAX_PARAMETER_BANDS);

                    let mut a_map = sac_map::create_mapping(stop_band, PbStride::from(stride));
                    sac_map::map_frequency(
                        &(cmp_idx_data[set_idx]),
                        out_idx_data,
                        &mut a_map,
                        data_bands,
                    );
                    idx_prev.copy_from_slice(out_idx_data);
                    set_idx += 1;
                }
                BsXxxDataMode::Invalid => {
                    return Err(SacDecoderError::WrongParametersets);
                }
            }
        }

        lossless_data.prepare_index_data(num_param_sets)?;

        // Map all coarse data to fine.
        for (uncompressed_quant_coarse, out_idx_data) in izip!(
            lossless_data.no_cmp_quant_coarse_xxx().iter(),
            output_idx_data.iter_mut(),
        )
        .take(num_param_sets)
        {
            if *uncompressed_quant_coarse == BsQuantCoarseXxx::HalfRes {
                Self::coarse2fine(&mut out_idx_data[..num_bands], data_type);
            }
        }
        lossless_data.map_coarse_data_to_full_res(num_param_sets);

        Ok(())
    }

    /// Reads the time slot to which each parameter set applies from bitstream.
    fn read_param_time_slots(
        &mut self,
        bs: &mut Bitstream,
        num_time_slots: u8,
        num_param_sets: usize,
    ) -> Result<(), SacDecoderError> {
        let mut prev_param_slot = -1;
        // Can read max 6 bits.
        let bits_param_slot = Self::num_bits_param_slot(num_time_slots)?;

        for ps in self.param_time_slot.iter_mut().take(num_param_sets) {
            *ps = bs.read(bits_param_slot) as u8;

            if (*ps as i8) <= prev_param_slot || *ps >= num_time_slots {
                self.num_parameter_sets = 0;
                return Err(SacDecoderError::ParseError);
            }
            prev_param_slot = *ps as i8;
        }

        Ok(())
    }
}

// Proves that the result operations of / vs >> is different for neg values.
#[cfg(test)]
mod tests {
    use super::*;

    fn generate_array() -> [i8; MAX_PARAMETER_BANDS] {
        let mut sac_data = [0_i8; MAX_PARAMETER_BANDS];
        let mut val = -14;
        for d in sac_data.iter_mut() {
            *d = val;
            val += 1;
        }
        sac_data
    }

    #[test]
    fn test_fine2coarse() {
        let mut sac_data_cld = generate_array();
        BsData::fine2coarse(&mut sac_data_cld, DataType::Cld);

        let mut sac_data_icc = generate_array();
        BsData::fine2coarse(&mut sac_data_icc, DataType::Icc);

        assert_ne!(sac_data_cld, sac_data_icc);
    }
}
