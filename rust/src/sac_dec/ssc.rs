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
//! Spatial Specific Config

use super::common::{DecorrConf, FixedGains, FreqResolution, QuantMode, Syntax, TsConf};
use super::constants::{MAX_PARAMETER_BANDS, MAX_TIME_SLOTS};
use super::error_codes::SacDecoderError;
use super::tables::{FREQ_RES_TABLE, FREQ_RES_TABLE_LD, SAMPLING_FREQ_TABLE};
use crate::common::aot::AudioObjectType;
use crate::common::bitstream::Bitstream;
use crate::common::enums::StereoCfgIndex;

const SPATIALDEC_MODE_212: usize = 7;
const MAX_NUM_EXT_TYPES: usize = 8;
const NUM_INPUT_CHANNELS: usize = 1;
const NUM_OUTPUT_CHANNELS: usize = 2;

/// Syntactic element that contains the SAC configuration data (header).
#[repr(C)]
#[derive(Debug, Default, PartialEq, Copy, Clone)]
pub(super) struct SpatialSpecificConfig {
    syntax: Syntax,
    sampling_freq: u32,
    n_time_slots: u8,
    freq_res: FreqResolution,
    quant_mode: QuantMode,
    is_residual_coding: bool,
    bs_fixed_gain_dmx: FixedGains,
    temp_shape_config: TsConf,
    decorr_config: DecorrConf,
    /// Derived from tree_config.
    n_input_channels: u8,
    /// Derived from tree_config.
    n_output_channels: u8,
    core_codec: AudioObjectType,
    stereo_config_index: StereoCfgIndex,
    /// Table 70 in ISO/IEC FDIS 23003-3:2011.
    core_sbr_frame_length_index: u8,
    bs_high_rate_mode: u8,
    n_residual_bands: u8,
    bs_pseudo_lr: u8,
    bs_phase_coding: u8,
    is_bs_ott_bands_phase_present: bool,
    bs_ott_bands_phase: u8,
    num_ott_bands_ipd: u8,
}

impl SpatialSpecificConfig {
    fn bail(is_usac: bool, bitstream: &mut Bitstream, sac_header_len: u32, cfg_start_pos: isize) {
        // bail function for finishing process of common mps212 config.
        if !is_usac && (sac_header_len > 0) {
            // If the config is of known length then assure that the bitbuffer is exactly at its
            // end when leaving the function.
            let valid_bits = bitstream.valid_bits();
            bitstream.push((sac_header_len * 8) as isize - (cfg_start_pos - valid_bits));
        }
    }

    fn decode_helper_info(&mut self) -> Result<(), SacDecoderError> {
        // Determine bit stream syntax.
        self.syntax.is_eld = false;
        self.syntax.is_usac = false;
        match self.core_codec {
            AudioObjectType::AotErAacEld => {
                self.syntax.is_eld = true;
                self.num_ott_bands_ipd = 0;
            }
            AudioObjectType::AotUsac => {
                self.syntax.is_usac = true;
                if self.is_bs_ott_bands_phase_present {
                    self.num_ott_bands_ipd = self.bs_ott_bands_phase;
                } else {
                    self.num_ott_bands_ipd = match self.freq_res {
                        FreqResolution::Res4 => 2,
                        FreqResolution::Res5 => 2,
                        FreqResolution::Res7 => 3,
                        FreqResolution::Res10 => 5,
                        FreqResolution::Res14 => 7,
                        FreqResolution::Res20 => 10,
                        FreqResolution::Res28 => 10,
                        _ => return Err(SacDecoderError::InvalidParameterbands),
                    };
                    if self.is_residual_coding && (self.num_ott_bands_ipd < self.n_residual_bands) {
                        self.num_ott_bands_ipd = self.n_residual_bands;
                    }
                }
            }
            _ => return Err(SacDecoderError::UnsupportedFormat),
        }

        Ok(())
    }

    pub(super) fn default_specific_config(
        &mut self,
        core_codec: AudioObjectType,
        sampling_freq: u32,
        n_time_slots: u32,
    ) -> Result<(), SacDecoderError> {
        if (core_codec != AudioObjectType::AotUsac && core_codec != AudioObjectType::AotErAacEld)
            || (n_time_slots == 0)
            || (n_time_slots > MAX_TIME_SLOTS as u32)
            || (sampling_freq == 0)
        {
            return Err(SacDecoderError::UnsupportedConfig);
        }

        self.core_codec = core_codec;
        self.sampling_freq = sampling_freq;
        self.n_time_slots = n_time_slots as u8;
        if self.core_codec == AudioObjectType::AotUsac {
            self.freq_res = FreqResolution::Res28;
            self.bs_fixed_gain_dmx = FixedGains::Mode0;
        } else {
            self.freq_res = FreqResolution::Res23;
            self.bs_fixed_gain_dmx = FixedGains::Rsvd2;
        }

        self.n_input_channels = NUM_INPUT_CHANNELS as u8;
        self.n_output_channels = NUM_OUTPUT_CHANNELS as u8;

        self.quant_mode = QuantMode::FineDef;
        self.is_residual_coding = false;
        self.temp_shape_config = TsConf::TpNoWhite;
        self.decorr_config = DecorrConf::Mode0;

        Ok(())
    }

    /// Parses spatial specific config for USAC and ELD.
    ///
    /// # Parameters
    /// - `bitstream`: Bitstream instance with valid internal data.
    /// - `core_codec`: Used for ELD+USAC:  Audio object type of the core codec.
    /// - `sac_header_len`: Used only for ELD:  Length of SAC header in bytes. Max value: (2^28).
    ///   Please see `eldExtLen` from ELDSpecificConfig() (ISO/IEC 14496-3:2009, Table 4.180).
    /// - `sampling_freq`: Used only for USAC: Sampling frequency.
    /// - `stereo_config_index`: Used only for USAC: Stereo config index.
    /// - `core_sbr_frame_length_index`: Used only for USAC: Core sbr frame length index.
    pub(super) fn parse_common_mps212_config(
        &mut self,
        bitstream: &mut Bitstream,
        core_codec: AudioObjectType,
        sac_header_len: u32,
        sampling_freq: u32,
        stereo_config_index: StereoCfgIndex,
        core_sbr_frame_length_index: u8,
    ) -> Result<(), SacDecoderError> {
        let mut err: Result<(), SacDecoderError>;
        let mut cfg_start_pos = 0;
        let mut bits_available = 0;
        let mut is_usac = false;

        match core_codec {
            AudioObjectType::AotUsac => {
                is_usac = true;
            }
            AudioObjectType::AotErAacEld => {}
            _ => return Err(SacDecoderError::UnsupportedConfig),
        }

        *self = SpatialSpecificConfig::default();

        self.core_codec = core_codec;
        self.n_input_channels = NUM_INPUT_CHANNELS as u8;
        self.n_output_channels = NUM_OUTPUT_CHANNELS as u8;

        if is_usac {
            // Wrong for stereoConfigIndex == 3, but value is unused.
            self.sampling_freq = sampling_freq;
            self.n_time_slots = if core_sbr_frame_length_index == 4 {
                64
            } else {
                32
            };

            let freq_res = FREQ_RES_TABLE[bitstream.read(3) as usize];
            if freq_res == FreqResolution::ResInvalid {
                return Err(SacDecoderError::ParseError);
            }
            self.freq_res = freq_res;
        } else {
            cfg_start_pos = bitstream.valid_bits();

            // It might be that we do not know the SSC length beforehand.
            if sac_header_len == 0 {
                bits_available = cfg_start_pos;
            } else {
                bits_available = (8 * sac_header_len) as isize;
                if bits_available > cfg_start_pos {
                    SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                    return Err(SacDecoderError::ParseError);
                }
            }
            let bs_sampling_freq_index = bitstream.read(4);

            if bs_sampling_freq_index == 15 {
                self.sampling_freq = bitstream.read(24);
            } else {
                self.sampling_freq = SAMPLING_FREQ_TABLE[bs_sampling_freq_index as usize];
                if self.sampling_freq == 0 {
                    SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                    return Err(SacDecoderError::ParseError);
                }
            }

            self.n_time_slots = (bitstream.read(5) + 1) as u8;
            if self.n_time_slots < 1 || (self.n_time_slots > MAX_TIME_SLOTS as u8) {
                SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                return Err(SacDecoderError::ParseError);
            }

            let freq_res = FREQ_RES_TABLE_LD[bitstream.read(3) as usize];
            if freq_res == FreqResolution::ResInvalid {
                // Reserved value.
                return Err(SacDecoderError::ParseError);
            }
            self.freq_res = freq_res;

            if bitstream.read(4) != SPATIALDEC_MODE_212 as u32 {
                SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                return Err(SacDecoderError::UnsupportedConfig);
            }

            // QuantMode > 2 is reserved.
            self.quant_mode = QuantMode::try_from(bitstream.read(2))?;

            // Parse bArbitraryDownmix.
            bitstream.read(1);
        }

        self.bs_fixed_gain_dmx = FixedGains::from(bitstream.read(3));

        let shape_config_number = bitstream.read(2);
        if !is_usac && (shape_config_number as i32 > 2) {
            // Reserved value.
            return Err(SacDecoderError::ParseError);
        }
        self.temp_shape_config = TsConf::from(shape_config_number);

        // DecorrConf > 2 is reserved value.
        self.decorr_config = DecorrConf::try_from(bitstream.read(2))?;

        if is_usac {
            self.bs_high_rate_mode = bitstream.read(1) as u8;
            self.bs_phase_coding = bitstream.read(1) as u8;
            self.is_bs_ott_bands_phase_present = bitstream.read_bit() != 0;

            if self.is_bs_ott_bands_phase_present {
                self.bs_ott_bands_phase = bitstream.read(5) as u8;
                if MAX_PARAMETER_BANDS < self.bs_ott_bands_phase as usize {
                    return Err(SacDecoderError::UnsupportedConfig);
                }
            }

            self.stereo_config_index = stereo_config_index;
            if (2..=4).contains(&core_sbr_frame_length_index) {
                self.core_sbr_frame_length_index = core_sbr_frame_length_index;
            } else {
                return Err(SacDecoderError::UnsupportedConfig);
            }

            if stereo_config_index > StereoCfgIndex::Mps212 {
                // Do residual coding.
                self.is_residual_coding = true;
                self.n_residual_bands = bitstream.read(5) as u8;
                if (self.freq_res as u8) < self.n_residual_bands {
                    return Err(SacDecoderError::UnsupportedConfig);
                }
                self.bs_ott_bands_phase = self.bs_ott_bands_phase.max(self.n_residual_bands);
                self.bs_pseudo_lr = bitstream.read(1) as u8;

                if self.bs_phase_coding != 0 {
                    self.bs_phase_coding = 3;
                }
            }
        }

        if self.temp_shape_config == TsConf::Tes {
            // GesData::reshape() is designed for envQuantMode == 0 only.
            // Therefore, envQuantMode is parsed, but then not explicitly passed to
            // SpatialDecReshapeBBEnv().
            // envQuantMode == 1 is reserved.
            if bitstream.read(1) != 0 {
                return Err(SacDecoderError::UnsupportedConfig);
            }
        }

        if !is_usac {
            // ISO/IEC FDIS 23003-1: 5.2. ... byte alignment with respect to the beginning of the
            // syntactic element in which Bitstream::align() occurs.
            bitstream.align(cfg_start_pos);

            let num_header_bits = cfg_start_pos - bitstream.valid_bits();
            bits_available -= num_header_bits;
            if bits_available < 0 {
                SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                return Err(SacDecoderError::UnsupportedConfig);
            }

            err = self.parse_extension_config(bitstream, bits_available);

            if let Err(e) = err {
                SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
                return Err(e);
            }
            // ISO/IEC FDIS 23003-1: 5.2. ... byte alignment with respect to the beginning of the
            // syntactic element in which ByteAlign() occurs.
            bitstream.align(cfg_start_pos);
        }

        // Derive additional helper variables.
        err = self.decode_helper_info();
        SpatialSpecificConfig::bail(is_usac, bitstream, sac_header_len, cfg_start_pos);
        err
    }

    fn parse_extension_config(
        &mut self,
        bitstream: &mut Bitstream,
        bits_available: isize,
    ) -> Result<(), SacDecoderError> {
        let mut sac_ext_cnt = 0;
        let mut ba = bitstream.valid_bits().min(bits_available);
        while ba >= 8 && sac_ext_cnt < MAX_NUM_EXT_TYPES {
            let mut sac_ext_len;

            // Read sacExtType.
            bitstream.read(4);
            ba -= 4;

            // Read sacExtLen.
            sac_ext_len = bitstream.read(4);
            ba -= 4;

            if sac_ext_len == 15 {
                sac_ext_len += bitstream.read(8);
                ba -= 8;
                if sac_ext_len == 15 + 255 {
                    sac_ext_len += bitstream.read(16);
                    ba -= 16;
                }
            }

            // Extension config payload start anchor.
            let tmp = bitstream.valid_bits();
            if tmp <= 0 || tmp < (sac_ext_len * 8) as isize || ba < (sac_ext_len * 8) as isize {
                return Err(SacDecoderError::UnsupportedConfig);
            }

            // Skip fill bits or an unknown extension.
            bitstream.push(8 * sac_ext_len as isize);

            ba -= (8 * sac_ext_len) as isize;
            sac_ext_cnt += 1;
        }

        Ok(())
    }

    pub(super) fn parse_specific_config_header(
        &mut self,
        bitstream: &mut Bitstream,
        core_codec: AudioObjectType,
    ) -> Result<(), SacDecoderError> {
        let mut err: Result<(), SacDecoderError>;
        let sac_time_align_flag = bitstream.read(1) != 0;
        let mut sac_header_len = bitstream.read(7);

        if sac_header_len == 127 {
            sac_header_len += bitstream.read(16);
        }

        let mut num_fill_bits = bitstream.valid_bits();

        err = self.parse_common_mps212_config(
            bitstream,
            core_codec,
            sac_header_len,
            0,
            StereoCfgIndex::RegularCpe,
            0,
        );
        // The number of read bits (tmpBits).
        num_fill_bits -= bitstream.valid_bits();
        num_fill_bits = (8 * sac_header_len as isize) - num_fill_bits;
        if err == Ok(()) && num_fill_bits < 0 {
            // Parsing went wrong.
            err = Err(SacDecoderError::UnsupportedConfig);
        }
        bitstream.push(num_fill_bits);

        if err == Ok(()) && sac_time_align_flag {
            // Not supported.
            bitstream.read(16);
            err = Err(SacDecoderError::UnsupportedConfig);
        }
        err
    }

    pub(super) fn stereo_config_index(&self) -> StereoCfgIndex {
        self.stereo_config_index
    }

    pub(super) fn sampling_freq(&self) -> u32 {
        self.sampling_freq
    }

    pub(super) fn quant_mode(&self) -> QuantMode {
        self.quant_mode
    }

    pub(super) fn decorr_config(&self) -> DecorrConf {
        self.decorr_config
    }

    pub(super) fn is_residual_coding(&self) -> bool {
        self.is_residual_coding
    }

    pub(super) fn n_residual_bands(&self) -> u8 {
        self.n_residual_bands
    }

    pub(super) fn bs_fixed_gain_dmx(&self) -> FixedGains {
        self.bs_fixed_gain_dmx
    }

    pub(super) fn bs_high_rate_mode(&self) -> u8 {
        self.bs_high_rate_mode
    }

    pub(super) fn bs_phase_coding(&self) -> u8 {
        self.bs_phase_coding
    }

    pub(super) fn freq_res(&self) -> FreqResolution {
        self.freq_res
    }

    pub(super) fn is_eld(&self) -> bool {
        self.syntax.is_eld
    }

    pub(super) fn is_usac(&self) -> bool {
        self.syntax.is_usac
    }

    pub(super) fn n_input_channels(&self) -> u8 {
        self.n_input_channels
    }

    pub(super) fn n_output_channels(&self) -> u8 {
        self.n_output_channels
    }

    pub(super) fn n_time_slots(&self) -> u8 {
        self.n_time_slots
    }

    pub(super) fn num_ott_bands_ipd(&self) -> u8 {
        self.num_ott_bands_ipd
    }

    pub(super) fn temp_shape_config(&self) -> TsConf {
        self.temp_shape_config
    }

    pub(super) fn core_codec(&self) -> AudioObjectType {
        self.core_codec
    }

    pub(super) fn bs_pseudo_lr(&self) -> bool {
        self.bs_pseudo_lr != 0
    }

    pub(super) fn syntax(&self) -> Syntax {
        self.syntax
    }

    pub(super) fn parse_check(&self) -> Result<(), SacDecoderError> {
        if (self.sampling_freq > 96000) || (self.sampling_freq < 8000) {
            return Err(SacDecoderError::ParseError);
        }

        Ok(())
    }

    // Gets number of QMF bands according to sampling frequency and SSC parameters.
    // See FDIS 23003-1:2006, chapter 6.3.3.
    pub(super) fn get_n_qmf_bands(&self) -> u8 {
        if self.core_codec == AudioObjectType::AotUsac {
            if self.stereo_config_index == StereoCfgIndex::Mps212wResStereoSbr {
                match self.core_sbr_frame_length_index {
                    2 => 24,
                    3 => 32,
                    4 => 16,
                    _ => unreachable!("core_sbr_frame_length_index should be between 2 and 4"),
                }
            } else {
                64
            }
        } else if self.sampling_freq < 27713 {
            32
        } else if self.sampling_freq > 55426 {
            128
        } else {
            64
        }
    }

    // Gets default number of QMF bands according to sampling frequency when no
    // SSC parameters are available.
    // See FDIS 23003-1:2006, chapter 6.3.3.
    pub(super) fn get_n_qmf_bands_default(sampling_frequency: u32) -> u8 {
        // Number of QMF bands depend on sampling frequency, see FDIS 23003-1:2006
        // Chapter 6.3.3
        if sampling_frequency < 27713 {
            32
        } else if sampling_frequency > 55426 {
            128
        } else {
            64
        }
    }

    pub(super) fn check_in_band(
        &self,
        frame_length: u16,
        sample_rate: u32,
    ) -> Result<(), SacDecoderError> {
        // Check SSC for parse errors.
        if self.parse_check() != Ok(()) {
            return Err(SacDecoderError::ParseError);
        }

        // In-band SSC only possible for LD/ELD.
        if self.core_codec == AudioObjectType::AotUsac {
            return Err(SacDecoderError::ParseError);
        }

        // Core sampling frequency must match MPS sampling frequency.
        if self.sampling_freq != sample_rate {
            return Err(SacDecoderError::ParseError);
        }

        if self.check_time_slots_eld(frame_length) != Ok(()) {
            return Err(SacDecoderError::ParseError);
        }

        Ok(())
    }

    pub(super) fn check_out_of_band(
        &self,
        core_codec: AudioObjectType,
        sample_rate: u32,
        frame_size: u16,
    ) -> Result<(), SacDecoderError> {
        // Check SSC for parse errors.
        if self.parse_check() != Ok(()) {
            return Err(SacDecoderError::ParseError);
        }

        match core_codec {
            AudioObjectType::AotUsac => {
                // ISO/IEC 23003-1:2007(E), Chapter 6.3.3, Support for lower and
                // higher sampling frequencies.
                if self.sampling_freq >= 55426 {
                    return Err(SacDecoderError::ParseError);
                }
            }
            AudioObjectType::AotErAacEld => {
                // Core sampling frequency must match MPS sampling frequency.
                if self.sampling_freq != sample_rate {
                    return Err(SacDecoderError::ParseError);
                }

                // ISO/IEC 14496-3:2009 FDAM 3: Chapter 1.5.2.3, Levels for the Low Delay
                // AAC v2 profile.
                if self.sampling_freq > 48000 {
                    return Err(SacDecoderError::ParseError);
                }

                if self.check_time_slots_eld(frame_size) != Ok(()) {
                    return Err(SacDecoderError::ParseError);
                }
            }
            _ => {
                return Err(SacDecoderError::ParseError);
            }
        }

        Ok(())
    }

    fn check_time_slots_eld(&self, frame_size: u16) -> Result<(), SacDecoderError> {
        let qmf_bands: u8 = self.get_n_qmf_bands();

        match frame_size {
            480 => {
                if !((qmf_bands == 32) && (self.n_time_slots == 15)) {
                    return Err(SacDecoderError::ParseError);
                }
            }
            960 => {
                if !((qmf_bands == 64) && (self.n_time_slots == 15)) {
                    return Err(SacDecoderError::ParseError);
                }
            }
            512 => {
                if !(((qmf_bands == 32) && (self.n_time_slots == 16))
                    || ((qmf_bands == 64) && (self.n_time_slots == 8)))
                {
                    return Err(SacDecoderError::ParseError);
                }
            }
            1024 => {
                if !((qmf_bands == 64) && (self.n_time_slots == 16)) {
                    return Err(SacDecoderError::ParseError);
                }
            }
            _ => {
                return Err(SacDecoderError::ParseError);
            }
        }

        Ok(())
    }
}
