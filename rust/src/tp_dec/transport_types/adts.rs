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
//! Audio Data Transport Stream (ADTS)
use super::super::{
    asc::AudioSpecificConfig,
    error_codes::TpDecoderError,
    pce::{PceCompareResult, ProgramConfig},
};
use crate::common::bs_element_id::ChannelElementId;
use crate::common::mpeg_id::MpegId;
use crate::common::samplerate_index::SAMPLING_RATE_TABLE;
use crate::common::{aot::AudioObjectType, bitstream::Bitstream, crc::CrcInfo};
use std::fmt::Debug;

#[repr(C)]
#[derive(Debug)]
pub(in super::super) struct Adts {
    bs: AdtsBs,
    /// mpeg2 implicit channel config state. (false: off, true: search)
    is_mpeg2_implicit_config: bool,
    is_buffer_fullnes_start_flag: bool,
    /// CRC state info.
    crc_info: CrcInfo,
    /// CRC value read from bitstream data.
    crc_value: u32,
    /// Distance between each raw data block. Not the same as found in the bitstream.
    raw_data_block_dist: [u16; 4],
}

impl Default for Adts {
    fn default() -> Adts {
        Adts {
            bs: Default::default(),
            is_mpeg2_implicit_config: false,
            is_buffer_fullnes_start_flag: true,
            crc_info: CrcInfo::default(),
            crc_value: 0,
            raw_data_block_dist: [0; 4],
        }
    }
}

impl Adts {
    pub(in super::super) const SYNCWORD: u32 = 0xfff;
    // Sync length in bits.
    pub(in super::super) const SYNCLENGTH: u16 = 12;
    // Minimum header size in bits.
    const HEADERLENGTH: i32 = 56;
    const LENGTH_ID: u8 = 1;
    const LENGTH_LAYER: u8 = 2;
    const LENGTH_PROTECTION_ABSENT: u8 = 1;
    const LENGTH_PROFILE: u8 = 2;
    const LENGTH_SAMPLING_FREQUENCY_INDEX: u8 = 4;
    const LENGTH_PRIVATE_BIT: u8 = 1;
    const LENGTH_CHANNEL_CONFIGURATION: u8 = 3;
    const LENGTH_ORIGINAL_COPY: u8 = 1;
    const LENGTH_HOME: u8 = 1;
    const LENGTH_COPYRIGHT_IDENTIFICATION_BIT: u8 = 1;
    const LENGTH_COPYRIGHT_IDENTIFICATION_START: u8 = 1;
    const LENGTH_FRAME_LENGTH: u8 = 13;
    const LENGTH_BUFFER_FULLNESS: u8 = 11;
    const LENGTH_NUMBER_OF_RAW_DATA_BLOCKS_IN_FRAME: u8 = 2;
    const LENGTH_CRC_CHECK: u8 = 16;

    pub fn new() -> Adts {
        Adts::default()
    }

    pub(in super::super) fn bs(&self) -> AdtsBs {
        self.bs
    }

    fn convert<T, U>(value: T) -> U
    where
        T: TryInto<U>,
        T::Error: Debug,
    {
        value.try_into().unwrap_or_else(|e| {
            panic!(
                "Conversion from {} to {} failed: {:?}",
                std::any::type_name::<T>(),
                std::any::type_name::<U>(),
                e
            )
        })
    }

    /// Function initializes the crc buffer and the crc lookup table.
    pub(in super::super) fn crc_init(&mut self) {
        self.crc_info.init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);
    }

    /// Starts CRC region with a maximum number of bits.
    /// If `m_bits` is positive and there are less than `m_Bits` bits available,
    /// zero padding will be used for CRC calculation.
    /// If `m_bits` is negative no zero padding is done.
    /// If `m_bits` is zero the memory for the buffer is  allocated dynamically,
    /// the number of bits is not limited.
    ///
    /// Returns ID for the created region.
    ///
    /// # Parameters
    ///
    /// - `h_bs`: bitstream handle, to which the CRC region refers.
    /// - `m_bits`: max number of bits in crc region to be considered.
    pub(in super::super) fn crc_start_reg(&mut self, h_bs: &mut Bitstream, m_bits: i32) -> usize {
        if self.bs.is_protection_absent {
            0
        } else {
            self.crc_info.start_region(h_bs, m_bits)
        }
    }

    /// Ends CRC region identified by `reg`.
    ///
    /// # Parameters
    ///
    /// - `h_bs`: bitstream handle, on which the CRC region refers to.
    /// - `reg`: CRC regions ID returned by `crc_start_reg()`.
    pub(in super::super) fn crc_end_reg(&mut self, h_bs: &mut Bitstream, reg: usize) {
        if !self.bs.is_protection_absent {
            self.crc_info.end_region(h_bs, reg);
        }
    }

    /// Checks if current CRC matches the CRC field read from bitstream.
    /// If CRCs do not match, returns a `CrcError`.
    /// If protection absent or values match, returns `Ok`.
    pub(in super::super) fn crc_check(&mut self) -> Result<(), TpDecoderError> {
        if self.bs.is_protection_absent {
            Ok(())
        } else {
            let crc = self.crc_info.value();
            if crc != self.crc_value {
                return Err(TpDecoderError::CrcError);
            }
            Ok(())
        }
    }

    fn get_number_of_effective_channels(channel_config: u8) -> i32 {
        if channel_config == 6 {
            5
        } else {
            channel_config.into()
        }
    }

    pub(in super::super) fn init(&mut self) {
        self.bs = Default::default();
        self.is_mpeg2_implicit_config = false;
        self.crc_value = 0;
        self.raw_data_block_dist = [0; 4];
        self.crc_init();
        self.is_buffer_fullnes_start_flag = true;
    }

    pub(in super::super) fn reset(&mut self) {
        self.crc_info.reset();
        self.bs.num_pce_bits = 0;
    }

    /// Checks if we have a valid ADTS frame at the current bitbuffer position.
    ///
    /// This function assumes enough bits in buffer for the current frame.
    /// It reads out the header bits to prepare the bitbuffer for the decode loop.
    /// In case the header bits show an invalid bitstream/frame, the whole frame is
    /// skipped.
    ///
    /// # Parameters
    ///
    /// - `h_bs`: handle of bitstream from which the ADTS header is read.
    pub(in super::super) fn decode_header(
        &mut self,
        asc: &mut AudioSpecificConfig,
        h_bs: &mut Bitstream,
        ignore_buffer_fullness: bool,
    ) -> Result<(), TpDecoderError> {
        let mut bs: AdtsBs = Default::default();
        // Store the old PCE temporarily.
        // Maybe we'll need it later if we have channelConfig=0 and no PCE in this frame.
        let old_pce = *asc.pce();

        self.is_mpeg2_implicit_config = false;

        let val_bits = h_bs.valid_bits() + Adts::SYNCLENGTH as isize;

        if val_bits < Adts::HEADERLENGTH as isize {
            return Err(TpDecoderError::NotEnoughBits);
        }

        // adts_fixed_header
        // Defines are no more than 13, therefore as u8 & u16 may be used.
        bs.mpeg_id = MpegId::from(h_bs.read(Adts::LENGTH_ID));
        bs.layer = Adts::convert(h_bs.read(Adts::LENGTH_LAYER));
        bs.is_protection_absent = h_bs.read(Adts::LENGTH_PROTECTION_ABSENT) == 1;
        bs.profile = Adts::convert(h_bs.read(Adts::LENGTH_PROFILE));
        bs.sample_freq_index = Adts::convert(h_bs.read(Adts::LENGTH_SAMPLING_FREQUENCY_INDEX));
        bs.private_bit = Adts::convert(h_bs.read(Adts::LENGTH_PRIVATE_BIT));
        bs.channel_config = Adts::convert(h_bs.read(Adts::LENGTH_CHANNEL_CONFIGURATION));
        bs.is_original = h_bs.read(Adts::LENGTH_ORIGINAL_COPY) == 1;
        bs.is_home = h_bs.read(Adts::LENGTH_HOME) == 1;

        // adts_variable_header
        bs.has_copyright_id = h_bs.read(Adts::LENGTH_COPYRIGHT_IDENTIFICATION_BIT) == 1;
        bs.copyright_start = Adts::convert(h_bs.read(Adts::LENGTH_COPYRIGHT_IDENTIFICATION_START));
        bs.frame_length = Adts::convert(h_bs.read(Adts::LENGTH_FRAME_LENGTH));
        bs.adts_fullness = Adts::convert(h_bs.read(Adts::LENGTH_BUFFER_FULLNESS));
        bs.num_raw_blocks =
            Adts::convert(h_bs.read(Adts::LENGTH_NUMBER_OF_RAW_DATA_BLOCKS_IN_FRAME));
        bs.num_pce_bits = 0;

        let mut adts_header_length = Adts::HEADERLENGTH as isize;

        if val_bits < (bs.frame_length as isize) * 8 {
            return Adts::decode_header_bail(h_bs, adts_header_length);
        }

        self.crc_info.reset();
        if !bs.is_protection_absent {
            // complete fixed and variable header!
            h_bs.push(-adts_header_length);
            let crc_reg = self.crc_info.start_region(h_bs, 0);
            h_bs.push(adts_header_length);

            if bs.num_raw_blocks > 0 {
                if h_bs.valid_bits() < ((bs.num_raw_blocks as isize) * 16) {
                    return Adts::decode_header_bail(h_bs, adts_header_length);
                }
                for rdb in self
                    .raw_data_block_dist
                    .iter_mut()
                    .take(usize::from(bs.num_raw_blocks))
                {
                    *rdb = Adts::convert(h_bs.read(16));
                    adts_header_length += 16;
                }
                // Change raw data blocks to delta values.
                if bs.frame_length as i32 - 7 - bs.num_raw_blocks as i32 * 2 - 2 < 0 {
                    return Adts::decode_header_bail(h_bs, adts_header_length);
                }
                self.raw_data_block_dist[bs.num_raw_blocks as usize] =
                    bs.frame_length - 7 - (u16::from(bs.num_raw_blocks)) * 2 - 2;
                for i in (1..=(bs.num_raw_blocks as usize)).rev() {
                    if self.raw_data_block_dist[i] < self.raw_data_block_dist[i - 1] {
                        return Adts::decode_header_bail(h_bs, adts_header_length);
                    }
                    self.raw_data_block_dist[i] -= self.raw_data_block_dist[i - 1];
                }
            }

            self.crc_info.end_region(h_bs, crc_reg);

            if h_bs.valid_bits() < Adts::LENGTH_CRC_CHECK.into() {
                return Adts::decode_header_bail(h_bs, adts_header_length);
            }

            // adts_error_check
            let crc_check = Adts::convert(h_bs.read(Adts::LENGTH_CRC_CHECK));
            adts_header_length += Adts::LENGTH_CRC_CHECK as isize;

            self.crc_value = crc_check;
            // Check header CRC in case of multiple raw data blocks.
            if bs.num_raw_blocks > 0 {
                if self.crc_value != self.crc_info.value() {
                    return Err(TpDecoderError::CrcError);
                }
                // Reset CRC for the upcoming `raw_data_block()`.
                self.crc_info.reset();
            }
        }

        // Check if valid header (we only support MPEG ADTS).
        if (bs.layer != 0) || (bs.sample_freq_index >= Adts::LENGTH_FRAME_LENGTH) {
            // We only support 96kHz - 7350kHz.
            // Try again one frame later.
            h_bs.push((bs.frame_length as isize) * 8);
            return Err(TpDecoderError::UnsupportedFormat);
        }

        if !ignore_buffer_fullness {
            let cmp_buffer_fullness = ((i32::from(bs.frame_length)) * 8)
                + ((i32::from(bs.adts_fullness))
                    * 32
                    * Adts::get_number_of_effective_channels(bs.channel_config));

            // Evaluate buffer fullness.
            if bs.adts_fullness != 0x7FF
                && self.is_buffer_fullnes_start_flag
                && (val_bits < cmp_buffer_fullness as isize)
            {
                // Condition for start of decoding is not fulfilled.

                // The current frame will not be decoded.
                h_bs.push(-adts_header_length);

                if (Adts::convert::<i32, isize>(cmp_buffer_fullness) + adts_header_length)
                    > ((h_bs.buffer().len() as isize) << 3) - 7
                {
                    return Err(TpDecoderError::SyncError);
                } else {
                    return Err(TpDecoderError::NotEnoughBits);
                }
            } else {
                self.is_buffer_fullnes_start_flag = false;
            }
        }

        // Get info from ADTS header.
        AudioSpecificConfig::init(asc);
        match bs.profile + 1 {
            2 => asc.set_aot(AudioObjectType::AotAacLc),
            _ => {
                h_bs.push(-adts_header_length);
                return Err(TpDecoderError::ParseError);
            }
        };
        asc.set_sampling_frequency_index(bs.sample_freq_index);
        asc.set_sampling_frequency(SAMPLING_RATE_TABLE[bs.sample_freq_index as usize]);
        asc.set_channel_config(bs.channel_config);

        if bs.channel_config == 0 {
            let align_anchor = h_bs.valid_bits();
            let mut tmp_pce = ProgramConfig::default();

            if h_bs.read(3) == ChannelElementId::Pce as u32 {
                // Got luck! Parse the PCE
                let crc_reg = self.crc_start_reg(h_bs, 0);

                tmp_pce.init();
                tmp_pce.read(h_bs, align_anchor);

                if tmp_pce.is_valid() {
                    if old_pce.is_valid() && !self.is_mpeg2_implicit_config {
                        // Compare the new and the old PCE (tags ignored).
                        match &old_pce.compare(&tmp_pce) {
                            PceCompareResult::Equal
                            | PceCompareResult::DifferentButSameChannelConfig => {
                                asc.set_pce(tmp_pce);
                            }
                            _ => {
                                // If channel configuration changed
                                asc.set_pce(old_pce);
                                h_bs.push(-adts_header_length);
                                return Err(TpDecoderError::ParseError);
                            }
                        }
                    } else {
                        asc.set_pce(tmp_pce);
                    }
                } else if old_pce.is_valid() {
                    asc.set_pce(old_pce);
                } else {
                    h_bs.push(-adts_header_length);
                    return Err(TpDecoderError::ParseError);
                }

                self.crc_end_reg(h_bs, crc_reg);
                let pce_bits = align_anchor - h_bs.valid_bits();
                adts_header_length += pce_bits;

                if pce_bits > align_anchor {
                    return Adts::decode_header_bail(h_bs, adts_header_length);
                }

                // Store the number of PCE bits.
                if let Ok(num_pce_bits) = pce_bits.try_into() {
                    bs.num_pce_bits = num_pce_bits;
                } else {
                    return Err(TpDecoderError::TooManyBits);
                }
            } else {
                // No PCE in this frame! Push back the ID tag bits.
                h_bs.push(-3);

                // Encoders do not have to write a PCE in each frame.
                // So if we already have a valid PCE we have to use it.
                // Could compare the complete fixed header (bytes) here.
                if old_pce.is_valid()
                    && (bs.sample_freq_index == self.bs.sample_freq_index)
                    && (bs.channel_config == self.bs.channel_config)
                    && (bs.mpeg_id == self.bs.mpeg_id)
                {
                    // Restore previous PCE which is still valid
                    asc.set_pce(old_pce);
                } else if bs.mpeg_id == MpegId::Mpeg4 {
                    // If not it seems that we have a implicit channel configuration.
                    // This mode is not allowed in the context of ISO/IEC 14496-3.
                    // Skip this frame and try the next one.
                    h_bs.push(((bs.frame_length as isize) << 3) - adts_header_length - 3);
                    return Err(TpDecoderError::UnsupportedFormat);
                } else {
                    // ISO/IEC 13818-7 implicit channel mapping is allowed.
                    // Assume that bitstream data was valid and only the PCE is missing.
                    // Determine configuration by trial and error.
                    self.bs = bs;
                    self.is_mpeg2_implicit_config = true;
                    return Adts::decode_header_bail(h_bs, adts_header_length);
                }
            }
        }

        // Copy bitstream data struct to persistent memory now, once all above sanity checks passed.
        self.bs = bs;

        Ok(())
    }

    fn decode_header_bail(
        h_bs: &mut Bitstream,
        adts_header_length: isize,
    ) -> Result<(), TpDecoderError> {
        h_bs.push(-adts_header_length);
        Err(TpDecoderError::NotEnoughBits)
    }

    pub(in super::super) fn adjust_end_of_access_unit(
        &mut self,
        h_bs: &mut Bitstream,
        block_num: u8,
        access_unit_anchor: isize,
        global_frame_pos: isize,
    ) {
        if !self.bs.is_protection_absent {
            // Calculate offset to end of AU.
            let mut offset =
                Adts::convert::<u16, isize>(self.raw_data_block_dist[block_num as usize] << 3);
            // CAUTION: The PCE (if available) is declared to be a part of the header!
            offset -=
                access_unit_anchor - h_bs.valid_bits() + 16 + isize::from(self.bs.num_pce_bits);
            h_bs.push(offset);

            if self.bs.num_raw_blocks > 0 {
                // Note this CRC read currently happens twice because of `transportDec_CrcCheck()`.
                self.crc_value = Adts::convert(h_bs.read(Adts::LENGTH_CRC_CHECK));
            }
            if block_num == self.bs.num_raw_blocks {
                // Check global frame length.
                offset = Adts::convert::<u16, isize>(self.bs.frame_length) * 8
                    - (Adts::SYNCLENGTH as isize)
                    + h_bs.valid_bits()
                    - global_frame_pos;
                h_bs.push(offset);
            }
        }
    }

    /// Get the raw data block length of the given block number.
    ///
    /// # Parameters
    ///
    /// - `block_num`: current raw data block index.
    ///
    /// # Return
    /// The length of the given raw data block
    pub(in super::super) fn get_raw_data_block_length(&mut self, block_num: u8) -> i32 {
        let mut length: i32;

        if self.bs.num_raw_blocks == 0 {
            // aac_frame_length subtracted by the header size (7 bytes).
            length = (i32::from(self.bs.frame_length) - 7) << 3;
            if !self.bs.is_protection_absent {
                // Substract 16 bit CRC.
                length -= i32::from(Adts::LENGTH_CRC_CHECK);
            }
        } else if self.bs.is_protection_absent || !(0..=3).contains(&block_num) {
            // Raw data block length is unknown.
            length = -1;
        } else {
            length = (i32::from(self.raw_data_block_dist[block_num as usize]) << 3)
                - i32::from(Adts::LENGTH_CRC_CHECK);
        }
        if block_num == 0 && length > 0 {
            length -= i32::from(self.bs.num_pce_bits);
        }
        length
    }

    pub(in super::super) fn is_mpeg2_implicit_config(&self) -> bool {
        self.is_mpeg2_implicit_config
    }
}

#[repr(C)]
#[derive(Default, PartialEq, Debug, Copy, Clone)]
pub(in super::super) struct AdtsBs {
    /// ADTS header fields.
    mpeg_id: MpegId,
    layer: u8,
    is_protection_absent: bool,
    profile: u8,
    sample_freq_index: u8,
    private_bit: u8,
    channel_config: u8,
    is_original: bool,
    is_home: bool,
    has_copyright_id: bool,
    copyright_start: u8,
    frame_length: u16,
    adts_fullness: u16,
    num_raw_blocks: u8,
    num_pce_bits: u8,
}

impl AdtsBs {
    pub fn _new() -> AdtsBs {
        AdtsBs::default()
    }

    pub(in super::super) fn num_raw_blocks(&self) -> u8 {
        self.num_raw_blocks
    }

    pub(in super::super) fn frame_length(&self) -> u16 {
        self.frame_length
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::bitstream::Mode::Reader;

    #[test]
    fn test_crc_start_reg() {
        let mut adts_tst = Adts::new();
        adts_tst
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);
        adts_tst.bs.is_protection_absent = true;

        let mut bs = Bitstream::default();
        let m_bits = 8;
        let output = adts_tst.crc_start_reg(&mut bs, m_bits);

        assert_eq!(output, 0);
    }

    #[test]
    fn test_crc_end_reg() {
        let mut adts_ref = Adts::new();
        adts_ref
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let mut adts_tst = Adts::new();
        adts_tst
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);
        adts_tst.bs.is_protection_absent = true;

        let mut bs = Bitstream::default();
        let reg = 1;
        adts_tst.crc_end_reg(&mut bs, reg);

        assert_eq!(adts_ref.crc_info.value(), adts_tst.crc_info.value());
    }

    #[test]
    fn test_crc_check_bad_value() {
        let mut adts_tst = Adts::new();
        adts_tst.crc_value = 12345;

        adts_tst
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);
        adts_tst.bs.is_protection_absent = true;

        let error = adts_tst.crc_check();
        assert_eq!(error, Ok(()));
    }

    #[test]
    fn test_crc_check_no_absent() {
        let mut adts_tst = Adts::new();
        adts_tst
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let error = adts_tst.crc_check();
        assert_eq!(error, Err(TpDecoderError::CrcError));
    }

    #[test]
    fn test_get_number_of_effective_channels_five() {
        let channel_config = 6;
        let num_channels = Adts::get_number_of_effective_channels(channel_config);
        assert_eq!(num_channels, 5);
    }

    #[test]
    fn test_get_number_of_effective_channels_any() {
        let channel_config = 42;
        let num_channels = Adts::get_number_of_effective_channels(channel_config);
        assert_eq!(num_channels, 42);
    }

    #[test]
    fn test_init() {
        let mut adts_ref = Adts::new();
        adts_ref
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let mut adts_tst = Adts {
            bs: Default::default(),
            is_mpeg2_implicit_config: true,
            is_buffer_fullnes_start_flag: false,
            crc_info: CrcInfo::new(),
            crc_value: 33,
            raw_data_block_dist: [44; 4],
        };
        adts_tst.crc_info.init(0x8009, 0xFFFA, 8);

        adts_tst.init();

        assert_eq!(adts_tst.bs, adts_ref.bs);
        assert_eq!(
            adts_tst.is_mpeg2_implicit_config,
            adts_ref.is_mpeg2_implicit_config
        );
        assert_eq!(
            adts_tst.is_buffer_fullnes_start_flag,
            adts_ref.is_buffer_fullnes_start_flag
        );
        assert_eq!(adts_tst.crc_info.value(), adts_ref.crc_info.value());
        assert_eq!(adts_tst.crc_value, adts_ref.crc_value);
        assert_eq!(adts_tst.raw_data_block_dist, adts_ref.raw_data_block_dist);
    }

    #[test]
    fn test_reset() {
        let mut adts_ref = Adts::new();
        adts_ref
            .crc_info
            .init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let mut adts_tst = Adts::new();
        adts_tst
            .crc_info
            .init(0x8009, 0xFFFA, Adts::LENGTH_CRC_CHECK);
        adts_tst.bs.num_pce_bits = 15;

        adts_tst.reset();

        assert_eq!(adts_tst.bs.num_pce_bits, 0);
    }

    #[test]
    fn test_adjust_end_of_access_unit_01() {
        let mut adts = Adts::new();
        adts.crc_info.init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let ref_bytes: [u8; 256] = std::array::from_fn(|i| i as u8);
        let test_bytes: [u8; 256] = std::array::from_fn(|i| i as u8);

        let valid_bits = ref_bytes.len() * 8;
        let mut bs_ref = Bitstream::new(ref_bytes.len(), Reader);
        bs_ref.init(&ref_bytes, valid_bits);
        let mut bs_tst = Bitstream::new(test_bytes.len(), Reader);
        bs_tst.init(&test_bytes, valid_bits);

        adts.bs.is_protection_absent = true;
        adts.raw_data_block_dist = [0; 4];

        let block_num = 1;
        let access_unit_anchor = 100;
        let global_frame_pos = 200;
        adts.bs.num_raw_blocks = 2;
        adts.bs.frame_length = 1024;

        adts.adjust_end_of_access_unit(
            &mut bs_tst,
            block_num,
            access_unit_anchor,
            global_frame_pos,
        );

        assert_eq!(bs_tst.read_bit(), bs_ref.read_bit());

        adts.bs.is_protection_absent = false;

        adts.adjust_end_of_access_unit(
            &mut bs_tst,
            block_num,
            access_unit_anchor,
            global_frame_pos,
        );

        bs_ref.push(1931);
        bs_ref.read(Adts::LENGTH_CRC_CHECK);
        assert_eq!(bs_tst, bs_ref);
    }

    #[test]
    fn test_adjust_end_of_access_unit_02() {
        let mut adts = Adts::new();
        adts.crc_info.init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let ref_bytes: [u8; 256] = std::array::from_fn(|i| i as u8);
        let test_bytes: [u8; 256] = std::array::from_fn(|i| i as u8);

        let valid_bits = ref_bytes.len() * 8;
        let mut bs_ref = Bitstream::new(ref_bytes.len(), Reader);
        bs_ref.init(&ref_bytes, valid_bits);
        let mut bs_tst = Bitstream::new(test_bytes.len(), Reader);
        bs_tst.init(&test_bytes, valid_bits);

        adts.raw_data_block_dist = [0; 4];

        let block_num = 2;
        let access_unit_anchor = 100;
        let global_frame_pos = 200;
        adts.bs.num_raw_blocks = 2;
        adts.bs.frame_length = 128;

        adts.adjust_end_of_access_unit(
            &mut bs_tst,
            block_num,
            access_unit_anchor,
            global_frame_pos,
        );

        bs_ref.push(2860);
        assert_eq!(bs_tst, bs_ref);
    }

    #[test]
    fn test_get_raw_data_block_length_01() {
        let mut adts = Adts::new();
        adts.crc_info.init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let block_num = 0;

        adts.bs.num_raw_blocks = 0;
        adts.bs.frame_length = 1024;
        adts.bs.is_protection_absent = false;
        adts.bs.num_pce_bits = 20;

        let result = adts.get_raw_data_block_length(block_num);

        assert_eq!(result, 8100);
    }

    #[test]
    fn test_get_raw_data_block_length_02() {
        let mut adts = Adts::new();
        adts.crc_info.init(0x8005, 0xFFFF, Adts::LENGTH_CRC_CHECK);

        let block_num = 0;

        adts.bs.num_raw_blocks = 1;
        adts.bs.frame_length = 1024;
        adts.bs.is_protection_absent = false;
        adts.bs.num_pce_bits = 0;

        let result = adts.get_raw_data_block_length(block_num);

        assert_eq!(result, -(Adts::LENGTH_CRC_CHECK as i32));
    }
}
