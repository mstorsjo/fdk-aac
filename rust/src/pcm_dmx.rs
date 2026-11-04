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
//! PCM downmix module.
//!
//! The PCM downmix module is designed to perform downmixing or channel expansion in the PCM time
//! domain. It supports various configurations and provides options for adjusting the number of
//! channels based on user parameters and bitstream metadata.
//! The module is part of a larger audio processing framework and is intended to be used
//! in conjunction with other audio processing modules.

mod api_types;
mod consts;
mod tables;
mod types;

use crate::common::{audio_channel_type::AudioChannelType, bitstream, channel_config::ChannelCfg};
use consts::*;
use itertools::izip;
use tables::*;
use types::*;

pub use api_types::*;
pub use consts::PCM_DMX_MAX_CHANNELS_US;
pub use types::DualChannelMode;
pub use types::ProfileType;

/// Pcm downmix module.
#[derive(Default, Debug, Clone)]
#[repr(C)]
pub struct PcmDmx {
    bs_meta_data: [DmxBsMetaData; PCM_DMX_MAX_DELAY_FRAMES + 1],
    params: PcmDmxParams,
}

impl PcmDmx {
    /// Creates a new PCM downmix module.
    pub fn new() -> Self {
        PcmDmx::default()
    }

    /// Resets the PCM downmix module.
    /// The function resets the bitstream metadata to its default values.
    pub fn reset(&mut self) {
        for bs_meta_data_ref in self.bs_meta_data.iter_mut() {
            *bs_meta_data_ref = DmxBsMetaData::default();
        }
    }

    /// Updates the PCM downmix module parameters.
    ///
    /// # Parameters
    ///
    /// - `dmx_params`: Pointer to the new parameters.
    pub fn params_update(&mut self, dmx_params: &mut PcmDmxParams) {
        if dmx_params.has_changed {
            dmx_params.has_changed = false;
            self.params = dmx_params.clone();

            let num_out_channels_min = if self.params.num_out_channels_min == 0 {
                PCM_DMX_DFLT_MIN_OUT_CHANNELS
            } else {
                self.params.num_out_channels_min
            };
            let num_out_channels_max = if self.params.num_out_channels_max == 0 {
                PCM_DMX_DFLT_MAX_OUT_CHANNELS
            } else {
                self.params.num_out_channels_max
            };

            self.params.num_out_channels_min = if self.params.num_out_channels_max == 0 {
                self.params.num_out_channels_min
            } else {
                num_out_channels_min.min(num_out_channels_max)
            };

            self.params.num_out_channels_max = num_out_channels_max;
        }
    }

    #[cfg(test)]
    pub fn set_dmx_idx_lfe(&mut self, idx: u8) {
        self.bs_meta_data[0].set_dmx_idx_lfe(idx);
    }

    /// Parses the ancillary data and extract the downmixing coefficients.
    ///
    /// # Parameters
    ///
    /// - `h_bs`: Bitstream handle.
    /// - `anc_data_bits`: Number of bits in the ancillary data.
    ///
    /// # Return
    ///
    /// - Result<(), PcmDmxError>
    pub fn parse(
        &mut self,
        bs: &mut bitstream::Bitstream,
        anc_data_bits: u32,
    ) -> Result<(), PcmDmxError> {
        // Ancillary data sync byte.
        const ANC_DATA_SYNC_BYTE: u8 = 0xBC;
        const MIN_ANC_BITS: u32 = 3 * 8;

        let bs_meta_data = &mut self.bs_meta_data[0];

        let skip4_dmx = 0;
        let mut skip4_ext = 0;
        let mut found_new_data: u32 = 0;

        // sanity checks
        if anc_data_bits < MIN_ANC_BITS || bs.valid_bits() < anc_data_bits.try_into().unwrap() {
            return Err(PcmDmxError::CorruptAncData);
        }

        // check sync word
        if bs.read(8) != ANC_DATA_SYNC_BYTE.into() {
            return Err(PcmDmxError::CorruptAncData);
        }

        //  skip MPEG audio type and Dolby surround mode
        bs.push(4);
        //  DRC presentation mode
        bs.push(2);
        //  Pseudo surround
        bs.push(1);
        //  reserved bits
        bs.push(4);
        //  downmixing levels MPEGx status
        let dmx_lvl_avail = bs.read_bit() != 0;

        //  ancillary data extension status
        let ext_data_avail = bs.read_bit() != 0;

        //  audio coding and compression status
        if bs.read_bit() != 0 {
            skip4_ext += 16;
        }
        //  coarse grain timecode status
        if bs.read_bit() != 0 {
            skip4_ext += 16;
        }
        //  fine grain timecode status
        if bs.read_bit() != 0 {
            skip4_ext += 16;
        }

        //  skip the useless data to get to the DMX levels
        bs.push(skip4_dmx);

        //  downmix_levels_MPEGX
        if dmx_lvl_avail {
            if bs.read_bit() != 0 {
                //  center_mix_level_on
                bs_meta_data.c_lev_idx = bs.read(3) as u8;
                found_new_data |= BsDataType::DseClevData as u32;
            } else {
                bs.read(3);
            }
            if bs.read_bit() != 0 {
                //  surround_mix_level_on
                bs_meta_data.s_lev_idx = bs.read(3) as u8;
                found_new_data |= BsDataType::DseSlevData as u32;
            } else {
                bs.read(3);
            }
        }

        //  skip the useless data to get to the ancillary data extension
        bs.push(skip4_ext);

        //  anc data extension (MPEG-4 only)
        if ext_data_avail {
            //  reserved bit
            bs.read_bit();
            let ext_dmx_lvl_st = bs.read_bit() != 0;
            let ext_dmx_gain_st = bs.read_bit() != 0;
            let ext_dmx_lfe_st = bs.read_bit() != 0;
            //  reserved bits
            bs.read(4);
            if ext_dmx_lvl_st {
                bs_meta_data.dmix_idx_a = bs.read(3) as u8;
                bs_meta_data.dmix_idx_b = bs.read(3) as u8;
                //  reserved bits
                bs.read(2);
                found_new_data |= BsDataType::DseDmixAbData as u32;
            }
            if ext_dmx_gain_st {
                bs_meta_data.dmx_gain_idx5 = bs.read(7) as u8;
                //  reserved bit
                bs.read_bit();
                bs_meta_data.dmx_gain_idx2 = bs.read(7) as u8;
                //  reserved bit
                bs.read_bit();
                found_new_data |= BsDataType::DseDmxGainData as u32;
            }
            if ext_dmx_lfe_st {
                bs_meta_data.dmix_idx_lfe = bs.read(4) as u8;
                //  reserved bits
                bs.read(4);
                found_new_data |= BsDataType::DseDmixLfeData as u32;
            }
        }

        //  final sanity check on the amount of read data
        if bs.valid_bits() < 0 {
            return Err(PcmDmxError::CorruptAncData);
        }

        if found_new_data != 0 {
            //  announce new data
            bs_meta_data.type_flags |= found_new_data;
            //  reset expiry counter
            bs_meta_data.expiry_count = 0;
        }

        Ok(())
    }

    /// Sets the matrix mixdown information extracted from the PCE of an AAC bitstream.
    ///
    /// Note: Call only if matrix_mixdown_idx_present is true.
    ///
    /// # Parameters
    ///
    /// - `matrix_mixdown_present`: The 2 bit matrix mixdown index extracted from PCE.
    /// - `matrix_mixdown_idx`: The pseudo surround enable flag extracted from PCE.
    pub fn set_matrix_mixdown_from_pce(
        &mut self,
        matrix_mixdown_present: bool,
        matrix_mixdown_idx: u8,
    ) -> PcmDmxError {
        if matrix_mixdown_present {
            self.bs_meta_data[0].matrix_mixdown_idx = matrix_mixdown_idx & 0x03;
            self.bs_meta_data[0].type_flags |= BsDataType::PceData as u32;
            self.bs_meta_data[0].expiry_count = 0;
        }
        PcmDmxError::Ok
    }

    /// Applies the downmixing or channel expansion to the input PCM samples.
    /// The function will apply the downmixing or channel expansion to the input PCM samples
    /// depending on the module settings and the respective given input configuration.
    ///
    /// # Parameters
    ///
    /// - `pcm_buf`: Pointer to time buffer with PCM samples.
    /// - `pcm_tmp_buf`: Pointer to a work buffer, which will help moving the PCM samples from
    ///   pPcmBuf.
    /// - `frame_size`: The I/O block size which is the number of samples per channel.
    /// - `n_channels`: Pointer to buffer that holds the number of input channels and where the
    ///   amount of output channels is written to.
    /// - `channel_type`: Array where the corresponding channel type for each output audio channel
    ///   is stored into.
    /// - `channel_indices`: Array where the corresponding channel type index for each output audio
    ///   channel is stored into.
    /// - `map_descr`: Pointer to a channel mapping descriptor that contains the channel mapping to
    ///   be used.
    #[expect(clippy::too_many_arguments)]
    pub fn apply_frame(
        &mut self,
        pcm_buf: &mut [f32],
        pcm_tmp_buf: &mut [f32],
        frame_size: u16,
        n_channels: &mut u8,
        channel_type: &mut [AudioChannelType],
        channel_indices: &mut [u8],
        map_descr: &mut crate::common::channel_map_descr::ChannelMapDescriptor,
    ) -> Result<(), PcmDmxError> {
        let frame_size_u16 = frame_size;
        let frame_size_us = usize::from(frame_size_u16);
        let pcm_buf_size = pcm_buf.len();

        // Perform some input sanity checks and return an error status if necessary.
        if pcm_buf.is_empty()
            || pcm_tmp_buf.is_empty()
            || frame_size_us == 0
            || *n_channels == 0
            || !map_descr.is_valid()
        {
            return Err(PcmDmxError::InvalidArgument);
        }
        if *n_channels > PCM_DMX_MAX_CHANNELS_U8 {
            return Err(PcmDmxError::InvalidChConfig);
        }

        if *n_channels == 2 && self.params.dual_channel_mode != DualChannelMode::Stereo {
            Self::apply_dual_channel(
                self.params.dual_channel_mode,
                pcm_buf,
                frame_size_us,
                n_channels,
            );
        }
        let num_in_channels: u8 = *n_channels;

        // Check on misconfiguration.
        if !(self.params.num_out_channels_max == 0
            || self.params.num_out_channels_max >= self.params.num_out_channels_min)
        {
            debug_assert!(false);
            return Err(PcmDmxError::InvalidChConfig);
        }

        // Determine if the module has to do processing.
        if (self.params.num_out_channels_max == 0
            || self.params.num_out_channels_max >= num_in_channels)
            && self.params.num_out_channels_min <= num_in_channels
        {
            // Nothing to do.
            return Ok(());
        }

        // Determine the number of output channels.
        let num_out_channels: u8 = if self.params.num_out_channels_max > 0
            && self.params.num_out_channels_max < num_in_channels
        {
            self.params.num_out_channels_max
        } else if self.params.num_out_channels_min > num_in_channels {
            self.params.num_out_channels_min
        } else {
            num_in_channels
        };

        // Check I/O buffer size.
        if pcm_buf_size < usize::from(num_out_channels) * frame_size_us {
            return Err(PcmDmxError::OutputBufferTooSmall);
        }

        let mut in_offset_table = [0; PCM_DMX_MAX_CHANNELS_US];
        let mut in_ch_mode = ChannelMode::Undefined;

        // Analyse input channel configuration and get channel offset
        // table that can be accessed with the fixed channel labels.
        let mut error_status = Self::get_channel_mode(
            num_in_channels,
            channel_type,
            channel_indices,
            &mut in_offset_table,
            &mut in_ch_mode,
        );

        if in_ch_mode == ChannelMode::Undefined {
            // We don't need to restore because the channel
            // configuration has not been changed. Just exit.
            return Err(PcmDmxError::InvalidChConfig);
        }

        let in_stride = 1;
        //  Channel specific offset factor
        let offset: u16 = frame_size_u16;

        //  Reset downmix meta data if necessary
        if self.params.expiry_frame > 0 {
            self.bs_meta_data[0].expiry_count += 1;
            if self.bs_meta_data[0].expiry_count > self.params.expiry_frame {
                self.reset();
            }
        }
        let bs_meta_data = self.bs_meta_data[usize::from(self.params.frame_delay)];

        //  Maintain delay line
        self.bs_meta_data
            .copy_within(0..usize::from(self.params.frame_delay), 1);

        match num_in_channels.cmp(&num_out_channels) {
            //  Apply downmix
            std::cmp::Ordering::Greater => {
                let mut in_pcm_offset = [u16::MAX; PCM_DMX_MAX_CHANNELS_US];
                let mut tmp_out_pcm_offset = [u16::MAX; PCM_DMX_MAX_CHANNELS_US];
                let mut mix_factors = [[0.0f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US];

                let mut out_offset_table = [0; PCM_DMX_MAX_CHANNELS_US];
                let mut in_ch_cfg = ChannelCfg::Cfg0;

                //  check tmp buffer size: tmp buffer will hold the output signal
                if pcm_tmp_buf.len() < usize::from(num_out_channels) * frame_size_us {
                    return Err(PcmDmxError::InvalidMode);
                }

                if num_in_channels > ChannelCount::Six as u8 {
                    //  Check if the input configuration is one defined in the standard.
                    in_ch_cfg = match in_ch_mode {
                        ChannelMode::Mode_5_0_2_1 => {
                            let mut multi_purpose_ch_type = [AudioChannelType::None; 2];
                            //  Get the type of the multipurpose channels
                            multi_purpose_ch_type[0] = channel_type
                                [usize::from(in_offset_table[Channel::LeftMultiprps as usize])];
                            multi_purpose_ch_type[1] = channel_type
                                [usize::from(in_offset_table[Channel::RightMultiprps as usize])];

                            if multi_purpose_ch_type[0] == AudioChannelType::FrontTop
                                && multi_purpose_ch_type[1] == AudioChannelType::FrontTop
                            {
                                ChannelCfg::Cfg14
                            } else {
                                ChannelCfg::Cfg7
                            }
                        }
                        ChannelMode::Mode_3_0_3_1 => ChannelCfg::Cfg11,
                        ChannelMode::Mode_3_0_4_1 => ChannelCfg::Cfg12,
                        _ => ChannelCfg::Cfg0,
                    };
                }

                let out_ch_mode = OUT_CH_MODE_TABLE[usize::from(num_out_channels)];
                debug_assert_ne!(out_ch_mode, ChannelMode::Undefined);
                if out_ch_mode == ChannelMode::Undefined {
                    return Err(PcmDmxError::InvalidChConfig);
                }

                // Get channel description and channel mapping for the desired output configuration.
                Self::get_channel_description(
                    out_ch_mode,
                    map_descr,
                    channel_type,
                    channel_indices,
                    &mut out_offset_table,
                )?;

                //  Now there is no way back because we modified the channel configuration!

                //  Create the DMX matrix
                error_status = Self::get_mix_factors(
                    in_ch_cfg,
                    in_ch_mode,
                    out_ch_mode,
                    &self.params,
                    &bs_meta_data,
                    &mut mix_factors,
                );
                // No fatal errors can occur here. The function is designed to always return
                // a valid matrix. The error code is used to signal configurations and
                // matrices that are not conform to any standard.
                let mut map: [usize; PCM_DMX_MAX_CHANNELS_US] = [0; PCM_DMX_MAX_CHANNELS_US];
                let mut ch: usize = 0;
                for (in_ch, in_offset_table_in_ch) in in_offset_table
                    .iter()
                    .enumerate()
                    .take(PCM_DMX_MAX_CHANNELS_US)
                {
                    if num_in_channels > *in_offset_table_in_ch {
                        in_pcm_offset[ch] = u16::from(*in_offset_table_in_ch) * offset;
                        map[ch] = in_ch;
                        ch += 1;
                    }
                }

                for (nr, map_ch) in map[ch..]
                    .iter_mut()
                    .enumerate()
                    .take(PCM_DMX_MAX_CHANNELS_US)
                {
                    *map_ch = ch + nr;
                }

                // Remove unused cols from factor matrix
                for (in_ch, map_in_ch) in map.iter().enumerate().take(num_in_channels.into()) {
                    if in_ch != *map_in_ch {
                        for mix_factors_out_ch in mix_factors.iter_mut() {
                            mix_factors_out_ch[in_ch] = mix_factors_out_ch[*map_in_ch];
                        }
                    }
                }

                let mut ch = 0;
                for (out_ch, out_offset_table_out_ch) in out_offset_table
                    .iter()
                    .enumerate()
                    .take(PCM_DMX_MAX_CHANNELS_US)
                {
                    if num_out_channels > *out_offset_table_out_ch {
                        tmp_out_pcm_offset[ch] = u16::from(*out_offset_table_out_ch) * offset;
                        map[ch] = out_ch;
                        ch += 1;
                    }
                }
                for (nr, map_ch) in map[ch..]
                    .iter_mut()
                    .enumerate()
                    .take(PCM_DMX_MAX_CHANNELS_US)
                {
                    *map_ch = ch + nr;
                }
                //  Remove unused rows from factor matrix
                for dst_ch in 0..num_out_channels.into() {
                    let src_ch = map[dst_ch];
                    if dst_ch != src_ch {
                        let (mix_factors_lo, mix_factors_hi) =
                            mix_factors.split_at_mut(src_ch.max(dst_ch));
                        let (mix_factors_src, mix_factors_dst) = if src_ch < dst_ch {
                            (&mix_factors_lo[src_ch], &mut mix_factors_hi[0])
                        } else {
                            (&mix_factors_hi[0], &mut mix_factors_lo[dst_ch])
                        };
                        mix_factors_dst.copy_from_slice(mix_factors_src);
                    }
                }

                //  Sample processing loop
                pcm_tmp_buf[0..frame_size_us * usize::from(num_out_channels)].fill(0.0);
                for in_ch in 0..usize::from(num_in_channels) {
                    if in_pcm_offset[in_ch] == u16::MAX {
                        // Skip processing for un-allocated input channels.
                        // Output channels are already zero-ed.
                        continue;
                    }

                    for out_ch in 0..usize::from(num_out_channels) {
                        let factor = mix_factors[out_ch][in_ch];
                        if factor != 0.0 {
                            //  set the correct offset for output.
                            debug_assert!(tmp_out_pcm_offset[out_ch] != u16::MAX);
                            debug_assert!(
                                usize::from(tmp_out_pcm_offset[out_ch]) < pcm_tmp_buf.len()
                            );

                            let in_buf = &pcm_buf[usize::from(in_pcm_offset[in_ch])
                                ..usize::from(in_pcm_offset[in_ch]) + frame_size_us];
                            let out_buf = &mut pcm_tmp_buf[usize::from(tmp_out_pcm_offset[out_ch])
                                ..usize::from(tmp_out_pcm_offset[out_ch]) + frame_size_us];

                            izip!(out_buf, in_buf).for_each(|(out, inp)| *out += inp * factor);
                        }
                    }
                }
                //  pPcmBuf is I/O buffer, so put data back to it.
                pcm_buf[0..frame_size_us * usize::from(num_out_channels)].copy_from_slice(
                    pcm_tmp_buf[0..frame_size_us * usize::from(num_out_channels)].as_ref(),
                );

                //  Update the number of output channels
                *n_channels = num_out_channels;
            }
            //  Apply rudimentary upmix
            std::cmp::Ordering::Less => {
                let mut out_offset_table = [0; PCM_DMX_MAX_CHANNELS_US];

                //  upmix to stereo only if it's explicitly set.
                if num_in_channels == ChannelCount::One as u8
                    && num_out_channels == ChannelCount::Two as u8
                {
                    let out_stride = 1;
                    let out_ch_mode = OUT_CH_MODE_TABLE[ChannelCount::Two as usize];

                    // Get channel description and channel mapping for this
                    // stages number of output channels (always STEREO).
                    Self::get_channel_description(
                        out_ch_mode,
                        map_descr,
                        channel_type,
                        channel_indices,
                        &mut out_offset_table,
                    )?;

                    // Now there is no way back because we modified the channel configuration!
                    //
                    // Set input channel pointer. The first channel is always at index 0.

                    // Considering input mapping
                    // could lead to a invalid pointer
                    // here if the channel is not
                    // declared to be a front channel.
                    let mut off_in_c = (frame_size_us - 1) * in_stride;
                    let mut off_out_lf: usize =
                        usize::from(out_offset_table[Channel::LeftFront as usize])
                            * usize::from(offset)
                            + (frame_size_us - 1) * out_stride;
                    let mut off_out_rf =
                        usize::from(out_offset_table[Channel::RightFront as usize])
                            * usize::from(offset)
                            + (frame_size_us - 1) * out_stride;
                    //  1/0 input:
                    for x in 0..frame_size_us {
                        //  L' = C;  R' = C;
                        pcm_buf[off_out_lf] = pcm_buf[off_in_c];
                        pcm_buf[off_out_rf] = pcm_buf[off_in_c];
                        if x < frame_size_us - 1 {
                            off_in_c -= in_stride;
                            off_out_lf -= out_stride;
                            off_out_rf -= out_stride;
                        }
                    }

                    // constant 1: in_stride = out_stride;
                    // Prepare for next stage:
                    in_ch_mode = out_ch_mode;
                    in_offset_table.copy_from_slice(&out_offset_table);
                }
                // SECOND STAGE
                // Extend with zero channels to achieved the desired number of output
                // channels.
                if num_out_channels > ChannelCount::Two as u8 {
                    let mut in_ch_types = [AudioChannelType::None; PCM_DMX_MAX_CHANNELS_US];
                    let mut in_ch_indices = [0; PCM_DMX_MAX_CHANNELS_US];
                    let mut num_ch_per_grp = [[0; PCM_DMX_MAX_CHANNEL_GROUPS as usize]; 2];
                    //  Number of channels with content
                    let mut n_content_ch = 0;
                    //  Number of channels with content
                    let mut n_empty_ch = 0;
                    let mut is_compatible = true;

                    // Do not change the signalling which is the channel types and indices.
                    // Just reorder and add channels. So first save the input signalling.
                    in_ch_types[..usize::from(num_in_channels)]
                        .copy_from_slice(&channel_type[..usize::from(num_in_channels)]);
                    in_ch_indices[..usize::from(num_in_channels)]
                        .copy_from_slice(&channel_indices[..usize::from(num_in_channels)]);

                    let out_stride = 1;
                    let out_ch_mode = OUT_CH_MODE_TABLE[usize::from(num_out_channels)];
                    debug_assert_ne!(out_ch_mode, ChannelMode::Undefined);
                    if out_ch_mode == ChannelMode::Undefined {
                        return Err(PcmDmxError::InvalidChConfig);
                    }

                    // Check if input channel config can be easily mapped to the desired
                    // output config.
                    for ch_grp in 0..PCM_DMX_MAX_CHANNEL_GROUPS {
                        num_ch_per_grp[NCPG_IN][usize::from(ch_grp)] =
                            (in_ch_mode as u32 >> (u32::from(ch_grp) * 4)) & 0xF;
                        num_ch_per_grp[NCPG_OUT][usize::from(ch_grp)] =
                            (out_ch_mode as u32 >> (u32::from(ch_grp) * 4)) & 0xF;

                        if num_ch_per_grp[NCPG_IN][usize::from(ch_grp)]
                            > num_ch_per_grp[NCPG_OUT][usize::from(ch_grp)]
                        {
                            is_compatible = false;
                            break;
                        }
                    }

                    if is_compatible {
                        // Get new channel description and channel
                        // mapping for the desired output channel mode.
                        Self::get_channel_description(
                            out_ch_mode,
                            map_descr,
                            channel_type,
                            channel_indices,
                            &mut out_offset_table,
                        )?;

                        // If the input config has a back center channel but the output
                        // config has not, copy it to left and right (if available).
                        if num_ch_per_grp[NCPG_IN][ChGroup::Rear as usize] % 2 != 0
                            && num_ch_per_grp[NCPG_OUT][ChGroup::Rear as usize] % 2 == 0
                        {
                            if num_ch_per_grp[NCPG_IN][ChGroup::Rear as usize] == 1 {
                                in_offset_table[Channel::RightRear as usize] =
                                    in_offset_table[Channel::LeftRear as usize];
                            } else if num_ch_per_grp[NCPG_IN][ChGroup::Rear as usize] == 3 {
                                in_offset_table[Channel::RightMultiprps as usize] =
                                    in_offset_table[Channel::LeftMultiprps as usize];
                            }
                        }
                    } else {
                        //  Just copy and extend the original config
                        out_offset_table.copy_from_slice(&in_offset_table);
                    }

                    // Set I/O channel pointer.
                    // Note: The following assignment algorithm clears the channel offset
                    // tables. Thus they can not be used afterwards.
                    let mut off_in = [0; PCM_DMX_MAX_CHANNELS_US];
                    let mut off_out = [0; PCM_DMX_MAX_CHANNELS_US];
                    izip!(&mut in_offset_table, &mut out_offset_table)
                        .filter(|(in_ch, out_ch)| **in_ch < 255 && **out_ch < 255)
                        .for_each(|(in_ch, out_ch)| {
                            off_in[n_content_ch] = usize::from(*in_ch) * usize::from(offset)
                                + (frame_size_us - 1) * in_stride;
                            off_out[n_content_ch] = usize::from(*out_ch) * usize::from(offset)
                                + (frame_size_us - 1) * out_stride;

                            //  Update signalling
                            channel_type[usize::from(*out_ch)] = in_ch_types[usize::from(*in_ch)];
                            channel_indices[usize::from(*out_ch)] =
                                in_ch_indices[usize::from(*in_ch)];

                            *in_ch = 255;
                            *out_ch = 255;
                            n_content_ch += 1;
                        });
                    if is_compatible {
                        // Assign the remaining input channels.
                        // This is just a safety appliance. We should never need it.
                        for (in_offset_table_ch, out_offset_table_ch) in
                            izip!(&mut in_offset_table, &mut out_offset_table)
                        {
                            if *in_offset_table_ch < 255 {
                                let mut out_ch = 0;
                                while out_ch < PCM_DMX_MAX_CHANNELS_U8 {
                                    if *out_offset_table_ch < 255 {
                                        break;
                                    }
                                    out_ch += 1;
                                }
                                if out_ch >= PCM_DMX_MAX_CHANNELS_U8 {
                                    break;
                                }

                                //  Set I/O pointer:
                                off_in[n_content_ch] = usize::from(*in_offset_table_ch)
                                    * usize::from(offset)
                                    + (frame_size_us - 1) * in_stride;
                                off_out[n_content_ch] = usize::from(*out_offset_table_ch)
                                    * usize::from(offset)
                                    + (frame_size_us - 1) * out_stride;

                                //  Update signalling
                                debug_assert!(*in_offset_table_ch < num_in_channels);
                                debug_assert!(*out_offset_table_ch < num_out_channels);
                                channel_type[usize::from(*out_offset_table_ch)] =
                                    in_ch_types[usize::from(*in_offset_table_ch)];
                                channel_indices[usize::from(*out_offset_table_ch)] =
                                    in_ch_indices[usize::from(*in_offset_table_ch)];

                                *in_offset_table_ch = 255;
                                *out_offset_table_ch = 255;
                                n_content_ch += 1;
                            }
                        }

                        //  Set the remaining output channel pointer
                        out_offset_table
                            .iter_mut()
                            .filter(|out_offset_table_ch| **out_offset_table_ch < 255)
                            .for_each(|out_offset_table_ch| {
                                off_out[n_content_ch + usize::from(n_empty_ch)] =
                                    usize::from(*out_offset_table_ch) * usize::from(offset)
                                        + (frame_size_us - 1) * out_stride;

                                //  Expand output signalling
                                channel_type[usize::from(*out_offset_table_ch)] =
                                    AudioChannelType::None;
                                channel_indices[usize::from(*out_offset_table_ch)] = n_empty_ch;

                                *out_offset_table_ch = 255;
                                n_empty_ch += 1;
                            });
                    } else {
                        //  Set the remaining output channel pointer
                        for ch in n_content_ch..usize::from(num_out_channels) {
                            off_out[ch] =
                                ch * usize::from(offset) + (frame_size_us - 1) * out_stride;
                            //  Expand output signalling
                            channel_type[ch] = AudioChannelType::None;
                            channel_indices[ch] = n_empty_ch;
                            n_empty_ch += 1;
                        }
                    }

                    {
                        let mut t_in = [0.0; PCM_DMX_MAX_CHANNELS_US];
                        //  First copy the channels that have signal
                        for _ in 0..frame_size_us - 1 {
                            // Read all channel samples but last.
                            for ch in 0..n_content_ch {
                                t_in[ch] = pcm_buf[off_in[ch]];
                                off_in[ch] -= in_stride;
                            }
                            // Write all channel samples but last.
                            for ch in 0..n_content_ch {
                                pcm_buf[off_out[ch]] = t_in[ch];
                                off_out[ch] -= out_stride;
                            }
                        }
                        // Read last channel sample. (do not underflow off_in[ch])
                        for ch in 0..n_content_ch {
                            t_in[ch] = pcm_buf[off_in[ch]];
                        }
                        // Write last channel sample. (do not underflow off_out[ch])
                        for ch in 0..n_content_ch {
                            pcm_buf[off_out[ch]] = t_in[ch];
                        }
                    }
                    {
                        //  Clear all the other channels
                        for _ in 0..frame_size_us - 1 {
                            for ch in n_content_ch..usize::from(num_out_channels) {
                                pcm_buf[off_out[ch]] = 0.0;
                                off_out[ch] -= out_stride;
                            }
                        }
                        for ch in n_content_ch..usize::from(num_out_channels) {
                            pcm_buf[off_out[ch]] = 0.0;
                        }
                    }
                }
                //  update the number of output channels
                *n_channels = num_out_channels;
            }
            std::cmp::Ordering::Equal => {}
        }

        error_status
    }

    fn get_speaker_distance(pos_a: &SpeakerPosition, pos_b: &SpeakerPosition) -> u16 {
        let dx = i16::from(pos_a.x) - i16::from(pos_b.x);
        let dy = i16::from(pos_a.y) - i16::from(pos_b.y);
        let dz = i16::from(pos_a.z) - i16::from(pos_b.z);

        (dx * dx + dy * dy + dz * dz).try_into().unwrap()
    }

    fn get_speaker_pos(
        ch_type: AudioChannelType,
        ch_index: u8,
        num_ch_in_grp: u8,
    ) -> SpeakerPosition {
        let mut spkr_pos = SpeakerPosition { x: 0, y: 0, z: 0 };
        let ch_grp = ch_type as u8 & 0x0F;
        let f_has_center = num_ch_in_grp & 0x1;
        let ch_grp_width = num_ch_in_grp >> 1;
        let mut f_is_center = false;
        let f_is_lfe = ch_type == AudioChannelType::Lfe;
        let mut offset = 0;

        debug_assert!(ch_index < num_ch_in_grp);
        let mut ch_index = ch_index;

        if ch_grp == AudioChannelType::Front as u8 && f_has_center != 0 {
            if ch_index == 0 {
                f_is_center = true;
            }
            ch_index = if ch_index > 0 { ch_index - 1 } else { 0 };
        } else if f_has_center != 0 && ch_index == num_ch_in_grp - 1 {
            f_is_center = true;
        }

        //  now all even indices are left (-)
        if !f_is_center {
            offset = i32::from(ch_index) >> 1;
            if ch_grp > AudioChannelType::Front as u8
                && ch_type != AudioChannelType::Side
                && !f_is_lfe
            {
                //  the higher the index the lower the distance to the center position
                offset = i32::from(ch_grp_width) - i32::from(f_has_center) - offset;
            }
            if ch_index & 0x1 == 0 {
                //  even
                offset = -(offset + 1);
            } else {
                offset += 1;
            }
        }

        //  apply the offset
        if ch_type == AudioChannelType::Side {
            spkr_pos.x = if offset < 0 {
                -PCMDMX_SPKR_POS_X_MAX_WIDTH
            } else {
                PCMDMX_SPKR_POS_X_MAX_WIDTH
            };
            spkr_pos.y = PCMDMX_SPKR_POS_Y_SPREAD + i8::try_from(offset.abs() - 1).unwrap();
            spkr_pos.z = 0;
        } else {
            let spread = if ch_grp_width == 1 && !f_is_lfe {
                PCMDMX_SPKR_POS_X_MAX_WIDTH - 1
            } else {
                1
            };
            spkr_pos.x = i8::try_from(offset).unwrap() * spread;
            if f_is_lfe {
                spkr_pos.y = 0;
                spkr_pos.z = SpZ::Lfe as i8;
            } else {
                spkr_pos.y = (i8::try_from(ch_grp).unwrap() - 1).max(0) * PCMDMX_SPKR_POS_Y_SPREAD;
                spkr_pos.z = ch_type as i8 >> 4;
                //  ACT_BOTTOM
                if spkr_pos.z == 2 {
                    spkr_pos.z = -1;
                }
                spkr_pos.z *= PCMDMX_SPKR_POS_Z_SPREAD;
            }
        }

        spkr_pos
    }

    /// Return the channel mode of a given horizontal channel plain (normal, top, bottom) for a
    /// given channel configuration.
    ///
    /// # Parameters
    ///
    /// - `plain_index`: Index of the requested channel plain.
    /// - `tot_ch_mode`: The packed channel mode for the complete channel configuration (all
    ///   plains).
    /// - `ch_cfg`: The MPEG-4 channel configuration index which is necessary in cases where the
    ///   (packed) channel mode is ambiguous.
    fn get_ch_mode4_plain(
        plain_index: ChPlain,
        tot_ch_mode: ChannelMode,
        ch_cfg: ChannelCfg,
    ) -> ChannelMode {
        let mut plain_ch_mode = tot_ch_mode;

        if plain_ch_mode == ChannelMode::Mode_5_0_2_1 && ch_cfg == ChannelCfg::Cfg14 {
            match plain_index {
                ChPlain::Bottom => plain_ch_mode = ChannelMode::Undefined,
                ChPlain::Top => plain_ch_mode = ChannelMode::Mode_2_0_0_0,
                ChPlain::Normal => plain_ch_mode = ChannelMode::Mode_3_0_2_1,
            }
        }

        plain_ch_mode
    }

    /// Validates the channel indices of all channels present in the bitstream.
    /// The channel indices have to be consecutive and unique for each audio channel type.
    ///
    /// # Parameters
    ///
    /// - `num_channels`: The total number of channels of the given configuration.
    /// - `num_channels_plane_and_grp`: The total number of channels of the current audio channel
    ///   type of the given configuration.
    /// - `a_ch_type`: Audio channel type to be examined.
    /// - `channel_type`: Array holding the corresponding channel types for each channel.
    /// - `channel_indices`: Array holding the corresponding channel type indices for each channel.
    ///
    /// # Return
    ///
    /// Returns true on success, returns false on error.
    fn validate_indices(
        num_channels: usize,
        num_channels_plane_and_grp: usize,
        a_ch_type: AudioChannelType,
        channel_type: &[AudioChannelType],
        channel_indices: &[u8],
    ) -> bool {
        let num_channels_plane_and_grp = u8::try_from(num_channels_plane_and_grp).unwrap();
        for req_value in 0..num_channels_plane_and_grp {
            let mut found = false;
            for _ in izip!(channel_type, channel_indices)
                .take(num_channels)
                .filter(|(ch_type, ch_idx)| **ch_type == a_ch_type && **ch_idx == req_value)
            {
                if found {
                    return false;
                } else {
                    found = true;
                }
            }
            if !found {
                return false;
            }
        }
        true
    }

    /// Gets the channel mode and the corresponding offset table for a given channel configuration.
    ///
    /// The function evaluates a given channel configuration and extracts a packed channel mode.
    /// In addition, the function generates a channel offset table for the mapping to the internal
    /// representation.
    ///
    /// # Parameters
    ///
    /// - `num_channels`: The total number of channels of the given configuration.
    /// - `channel_type`: Array holding the corresponding channel types for each channel.
    /// - `channel_indices`: Array holding the corresponding channel type indices for each channel.
    /// - `offset_table`: Array where the buffer offsets for each channel are stored into.
    /// - `ch_mode`: The generated packed channel mode that represents the given input
    ///   configuration.
    ///
    /// # Return
    ///
    /// Returns an error code.
    fn get_channel_mode(
        num_channels: u8,
        channel_type: &[AudioChannelType],
        channel_indices: &mut [u8],
        offset_table: &mut [u8; PCM_DMX_MAX_CHANNELS_US],
        ch_mode: &mut ChannelMode,
    ) -> Result<(), PcmDmxError> {
        let mut num_ch =
            [[0u8; PCM_DMX_MAX_CHANNEL_GROUPS as usize]; PCM_DMX_MAX_CHANNEL_PLANES as usize];
        let mut mapped = [0; PCM_DMX_MAX_CHANNELS_US];
        let mut spkr_pos = [SpeakerPosition::default(); PCM_DMX_MAX_CHANNELS_US];
        let mut err = Ok(());
        let mut num_mapped_in_chs = 0;
        let stop_slot = Channel::LowFrequency as u16;

        offset_table.fill(255);

        channel_type
            .iter()
            .take(usize::from(num_channels))
            .for_each(|ch_type| {
                let ch_grp = (*ch_type as u8 & 0x0F).saturating_sub(1);
                num_ch[*ch_type as usize >> 4][usize::from(ch_grp)] += 1;
            });

        for ch_grp in 0..PCM_DMX_MAX_CHANNEL_GROUPS {
            for (plane, num_ch_plane) in num_ch
                .iter()
                .take(PCM_DMX_MAX_CHANNEL_PLANES as usize)
                .enumerate()
            {
                let plane: u8 = plane as u8;
                if num_ch_plane[usize::from(ch_grp)] == 0 {
                    continue;
                }
                let a_ch_type =
                    AudioChannelType::from(u32::from((plane << 4) | ((ch_grp + 1) & 0xF)));
                if !Self::validate_indices(
                    num_channels.into(),
                    num_ch_plane[usize::from(ch_grp)].into(),
                    a_ch_type,
                    channel_type,
                    channel_indices,
                ) {
                    let mut idx_cnt = 0;
                    for ch in 0..usize::from(num_channels) {
                        if channel_type[ch] == a_ch_type {
                            channel_indices[ch] = idx_cnt;
                            idx_cnt += 1;
                        }
                    }
                    err = Err(PcmDmxError::InvalidChConfig);
                }
            }
        }

        // Mapping HEAT 1:
        // Determine the speaker position of each input channel and map it to an
        // internal slot if it matches exactly (with zero distance).
        for ch in 0..num_channels {
            let mut map_dist = u16::MAX;
            let mut map_pos = u16::MAX;
            let ch_grp = (channel_type[usize::from(ch)] as u8 & 0x0F).saturating_sub(1);
            spkr_pos[usize::from(ch)] = Self::get_speaker_pos(
                channel_type[usize::from(ch)],
                channel_indices[usize::from(ch)],
                num_ch[channel_type[usize::from(ch)] as usize >> 4][usize::from(ch_grp)],
            );

            for (map_ch, offset_table_map_ch) in offset_table
                .iter()
                .enumerate()
                .take(usize::from(stop_slot) + 1)
            {
                let map_ch_u16 = map_ch as u16;
                if *offset_table_map_ch == 255 {
                    let dist = Self::get_speaker_distance(
                        &spkr_pos[usize::from(ch)],
                        &SPKR_SLOT_POS[map_ch],
                    );
                    if dist < map_dist {
                        map_pos = map_ch_u16;
                        map_dist = dist;
                        if dist == 0 {
                            break;
                        }
                    }
                }
            }

            if map_dist <= ThresholdMap::Heat1 as u16 {
                offset_table[usize::from(map_pos)] = ch;
                mapped[usize::from(ch)] = 1;
                num_mapped_in_chs += 1;
            }
        }

        let start_slot = if (num_ch[ChPlain::Normal as usize][ChGroup::Front as usize] & 0x1) != 0
            || num_channels >= PCM_DMX_MAX_CHANNELS_U8
        {
            0
        } else {
            1
        };

        // Mapping HEAT 2:
        // Go through the unmapped input channels and assign them to the internal
        // slots that matches best (least distance). But assign center channels to
        // center slots only.
        for (ch_us, mapped_ch) in mapped
            .iter_mut()
            .enumerate()
            .take(usize::from(num_channels))
            .filter(|(_, m)| **m == 0)
        {
            let ch = u8::try_from(ch_us).unwrap();
            let mut map_dist = u16::MAX;
            let mut map_pos = u16::MAX;

            for map_ch in start_slot..=stop_slot {
                if offset_table[usize::from(map_ch)] == 255 {
                    let dist = Self::get_speaker_distance(
                        &spkr_pos[ch_us],
                        &SPKR_SLOT_POS[usize::from(map_ch)],
                    );
                    if dist < map_dist {
                        map_pos = map_ch;
                        map_dist = dist;
                    }
                }
            }
            if map_pos <= stop_slot
                && map_dist < ThresholdMap::Heat2 as u16
                && ((spkr_pos[ch_us].x != 0 && SPKR_SLOT_POS[usize::from(map_pos)].x != 0)
                    || (spkr_pos[ch_us].x == 0 && SPKR_SLOT_POS[usize::from(map_pos)].x == 0))
            {
                offset_table[usize::from(map_pos)] = ch;
                *mapped_ch = 1;
                num_mapped_in_chs += 1;
            }
        }

        // Mapping HEAT 3:
        // Assign the rest by searching for the nearest input channel for each
        // internal slot.
        let mut ch = start_slot;
        let pcm_dmx_max_channels_u16: u16 = PCM_DMX_MAX_CHANNELS_U8.into();
        while ch < pcm_dmx_max_channels_u16 {
            if num_mapped_in_chs >= num_channels {
                break;
            }
            if offset_table[usize::from(ch)] == 255 {
                let mut map_dist = u16::MAX;
                let mut map_pos = u8::MAX;

                for (map_ch_us, _) in mapped
                    .iter()
                    .enumerate()
                    .take(usize::from(num_channels))
                    .filter(|(_, m)| **m == 0)
                {
                    let map_ch = u8::try_from(map_ch_us).unwrap();
                    let dist = Self::get_speaker_distance(
                        &spkr_pos[map_ch_us],
                        &SPKR_SLOT_POS[usize::from(ch)],
                    );
                    if dist < map_dist {
                        map_pos = map_ch;
                        map_dist = dist;
                    }
                }
                if map_dist < ThresholdMap::Heat3 as u16 {
                    offset_table[usize::from(ch)] = map_pos;
                    mapped[usize::from(map_pos)] = 1;
                    num_mapped_in_chs += 1;
                    if spkr_pos[usize::from(map_pos)].x == 0
                        && SPKR_SLOT_POS[usize::from(ch)].x != 0
                        && num_channels < PCM_DMX_MAX_CHANNELS_U8
                    {
                        ch += 1;
                    }
                }
            }
            ch += 1;
        }

        // Finally compose the channel mode
        let mut ch_mode_int = ChannelMode::Undefined as i32;
        for ch_grp in 0..PCM_DMX_MAX_CHANNEL_GROUPS as usize {
            let mut num_ch_in_grp = 0;
            for num_ch_plane in num_ch.iter().take(PCM_DMX_MAX_CHANNEL_PLANES as usize) {
                num_ch_in_grp += u16::from(num_ch_plane[ch_grp]);
            }
            ch_mode_int |= i32::from(num_ch_in_grp << (ch_grp * 4));
        }

        *ch_mode = ChannelMode::from(ch_mode_int);

        err
    }

    /// Generates a channel offset table and complete channel description for a given
    /// (packed) channel mode. This function is the inverse to the getChannelMode() routine but
    /// does not support weird channel configurations.
    ///
    /// # Parameters
    ///
    /// - `ch_mode`: The packed channel mode of the configuration to be processed.
    /// - `map_descr`: Array containing the channel mapping to be used (From MPEG PCE ordering to
    ///   whatever is required).
    /// - `channel_type`: Array where corresponding channel types for each channels are stored into.
    /// - `channel_indices`: Array where corresponding channel type indices for each output channel
    ///   are stored into.
    /// - `offset_table`: Array where the buffer offsets for each channel are stored into.
    fn get_channel_description(
        ch_mode: ChannelMode,
        map_descr: &crate::common::channel_map_descr::ChannelMapDescriptor,
        channel_type: &mut [AudioChannelType],
        channel_indices: &mut [u8],
        offset_table: &mut [u8; PCM_DMX_MAX_CHANNELS_US],
    ) -> Result<(), PcmDmxError> {
        let mut ch: u8 = 0;

        channel_type.fill(AudioChannelType::None);
        channel_indices.fill(0);
        offset_table.fill(255);

        let ch_cfg = match ch_mode {
            ChannelMode::Mode_1_0_0_0 => ChannelCfg::Cfg1,
            ChannelMode::Mode_2_0_0_0 => ChannelCfg::Cfg2,
            ChannelMode::Mode_3_0_0_0 => ChannelCfg::Cfg3,
            ChannelMode::Mode_3_0_1_0 => ChannelCfg::Cfg4,
            ChannelMode::Mode_3_0_2_0 => ChannelCfg::Cfg5,
            ChannelMode::Mode_3_0_2_1 => ChannelCfg::Cfg6,
            ChannelMode::Mode_3_0_3_1 => ChannelCfg::Cfg11,
            ChannelMode::Mode_3_0_4_1 => ChannelCfg::Cfg12,
            ChannelMode::Mode_5_0_2_1 => ChannelCfg::Cfg7,
            // Fallback channel config.
            _ => ChannelCfg::Cfg0,
        };

        // Currently support only ChPlain::Normal.
        {
            let plain = ChPlain::Normal;
            let plain_ch_mode = Self::get_ch_mode4_plain(plain, ch_mode, ch_cfg);

            let mut num_ch_in_grp = [0u8; PCM_DMX_MAX_CHANNEL_GROUPS as usize];

            num_ch_in_grp[ChGroup::Front as usize] = (plain_ch_mode as usize & 0xF) as u8;
            num_ch_in_grp[ChGroup::Side as usize] = ((plain_ch_mode as usize >> 4) & 0xF) as u8;
            num_ch_in_grp[ChGroup::Rear as usize] = ((plain_ch_mode as usize >> 8) & 0xF) as u8;
            num_ch_in_grp[ChGroup::Lfe as usize] = ((plain_ch_mode as usize >> 12) & 0xF) as u8;

            if (num_ch_in_grp[ChGroup::Front as usize] & 0x1) != 0 && plain == ChPlain::Normal {
                let mapped_idx = map_descr.get_map_value(ch, ch_cfg as usize);
                offset_table[Channel::CenterFront as usize] = mapped_idx;
                let mapped_idx = usize::from(mapped_idx);
                channel_type[mapped_idx] = AudioChannelType::Front;
                channel_indices[mapped_idx] = 0;
                ch += 1;
            }

            for (grp_idx, num_ch_in_grp_grp_idx) in num_ch_in_grp
                .iter()
                .enumerate()
                .take(PCM_DMX_MAX_CHANNEL_GROUPS as usize)
            {
                let mut ac_type = AudioChannelType::None;
                let mut ch_map_pos = 0;
                let mut max_channels = 0;
                let mut ch_idx = 0;

                match grp_idx.into() {
                    ChGroup::Front => {
                        ac_type = AudioChannelType::from(
                            ((plain as u32) << 4) | AudioChannelType::Front as u32,
                        );
                        ch_map_pos = Channel::LeftFront as usize;
                        ch_idx = num_ch_in_grp_grp_idx & 0x1;
                        max_channels = 3;
                    }
                    ChGroup::Side => {
                        ac_type = AudioChannelType::from(
                            ((plain as u32) << 4) | AudioChannelType::Side as u32,
                        );
                        ch_map_pos = Channel::LeftMultiprps as usize;
                        max_channels = 2;
                    }
                    ChGroup::Rear => {
                        ac_type = AudioChannelType::from(
                            ((plain as u32) << 4) | AudioChannelType::Back as u32,
                        );
                        ch_map_pos = Channel::LeftRear as usize;
                        max_channels = 2;
                    }
                    ChGroup::Lfe => {
                        if plain == ChPlain::Normal {
                            ac_type = AudioChannelType::Lfe;
                            ch_map_pos = Channel::LowFrequency as usize;
                            max_channels = 1;
                        }
                    }
                }

                for ch_idx in ch_idx..num_ch_in_grp[grp_idx] {
                    let mapped_idx = map_descr.get_map_value(ch, ch_cfg as usize);
                    if (ch_idx == max_channels) || (offset_table[ch_map_pos] < 255) {
                        if offset_table[Channel::LeftMultiprps as usize] != 255 {
                            debug_assert!(false);
                        };
                        ch_map_pos = Channel::LeftMultiprps as usize;
                    }
                    offset_table[ch_map_pos] = mapped_idx;
                    let mapped_idx = usize::from(mapped_idx);
                    channel_type[mapped_idx] = ac_type;
                    channel_indices[mapped_idx] = ch_idx;
                    ch_map_pos += 1;
                    ch += 1;
                }
            }
        }
        Ok(())
    }

    /// Helper function for downmix matrix manipulation that initializes
    /// one row in a given downmix matrix (corresponding to one output channel).
    ///
    /// # Parameters
    ///
    /// - `mixFactors`: Pointer to floating-point parts of the downmix matrix.
    /// - `outCh`: Index of channel (row) to be initialized.
    fn init_channel(
        mix_factors: &mut [[f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US],
        out_ch: usize,
    ) {
        mix_factors[out_ch].fill(0.0);
        mix_factors[out_ch][out_ch] = 1.0;
    }

    /// Helper function for downmix matrix manipulation that does a reset
    /// of one row in a given downmix matrix (corresponding to one output channel).
    ///
    /// # Parameters
    ///
    /// - `mixFactors`: Pointer to floating-point parts of the downmix matrix.
    /// - `outCh`: Index of channel (row) to be cleared/reset.
    fn clear_channel(
        mix_factors: &mut [[f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US],
        out_ch: usize,
    ) {
        mix_factors[out_ch].fill(0.0);
    }

    /// Helper function for downmix matrix manipulation that applies a
    /// source channel (row) scaled by a given mix factor to a destination channel
    /// (row) in a given downmix matrix. Existing mix factors of the destination
    /// channel (row) will get overwritten.
    ///
    /// # Parameters
    ///
    /// - `mix_factors`: Downmix matrix.
    /// - `dst_ch`: Index of destination channel (row).
    /// - `src_ch`: Index of source channel (row).
    /// - `factor`: Floating-point part of mix factor to be applied.
    fn set_channel(
        mix_factors: &mut [[f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US],
        dst_ch: usize,
        src_ch: usize,
        factor: f32,
    ) {
        if src_ch != dst_ch {
            let (mix_factors_lo, mix_factors_hi) = mix_factors.split_at_mut(src_ch.max(dst_ch));
            let (mix_factors_src, mix_factors_dst) = if src_ch < dst_ch {
                (&mix_factors_lo[src_ch], &mut mix_factors_hi[0])
            } else {
                (&mix_factors_hi[0], &mut mix_factors_lo[dst_ch])
            };
            izip!(mix_factors_dst.iter_mut(), mix_factors_src.iter())
                .take(PCM_DMX_MAX_CHANNELS_US)
                .for_each(|(dst, src)| {
                    if *src != 0.0 {
                        *dst = *src * factor;
                    }
                });
        } else if factor != 1.0f32 {
            for mix in &mut mix_factors[src_ch][..PCM_DMX_MAX_CHANNELS_US] {
                *mix *= factor;
            }
        }
    }

    /// Helper function for downmix matrix manipulation that adds a source
    /// channel (row) scaled by a given mix factor to a destination channel (row) in a
    /// given downmix matrix.
    ///
    /// # Parameters
    ///
    /// - `mixFactors`: Pointer to floating-point parts of the downmix matrix.
    /// - `dstCh`: Index of destination channel (row).
    /// - `srcCh`: Index of source channel (row).
    /// - `factor`: Floating-point part of mix factor to be applied.
    fn add_channel(
        mix_factors: &mut [[f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US],
        dst_ch: usize,
        src_ch: usize,
        factor: f32,
    ) {
        if factor == 0.0f32 {
            return;
        }

        if src_ch != dst_ch {
            let (mix_factors_lo, mix_factors_hi) = mix_factors.split_at_mut(src_ch.max(dst_ch));
            let (mix_factors_src, mix_factors_dst) = if src_ch < dst_ch {
                (&mix_factors_lo[src_ch], &mut mix_factors_hi[0])
            } else {
                (&mix_factors_hi[0], &mut mix_factors_lo[dst_ch])
            };

            izip!(mix_factors_dst.iter_mut(), mix_factors_src.iter())
                .take(PCM_DMX_MAX_CHANNELS_US)
                .for_each(|(dst, src)| {
                    *dst += *src * factor;
                });
        } else {
            for mix in &mut mix_factors[src_ch][..PCM_DMX_MAX_CHANNELS_US] {
                *mix += *mix * factor;
            }
        }
    }

    /// Creates a downmix factor matrix depending on the input
    /// and output configuration, the user parameters as well as the given metadata.
    /// This function is the modules brain and hold all downmix algorithms.
    ///
    /// # Parameters
    ///
    /// - `in_ch_cfg`: Dependent on the inModeIsCfg flag this field hands in a (packed) channel mode
    ///   or the corresponding MPEG-4 channel configuration index of the input configuration.
    /// - `in_ch_mode`: The (packed) channel mode of the input configuration.
    /// - `out_ch_mode`: The (packed) channel mode of the output configuration.
    /// - `params`: Pointer to structure holding all current user parameter.
    /// - `meta_data`: Pointer to field holding all current meta data.
    /// - `mix_factors`: Pointer to floating-point parts of the downmix matrix. Normalized to one
    ///   scale factor.
    ///
    /// # Return
    ///
    /// An error code
    ///   The function is designed to always return a valid matrix. The error code is used to
    ///   signal configurations and matrices that are not conform to any standard.
    fn get_mix_factors(
        in_ch_cfg: ChannelCfg,
        in_ch_mode: ChannelMode,
        out_ch_mode: ChannelMode,
        params: &PcmDmxParams,
        meta_data: &DmxBsMetaData,
        mix_factors: &mut [[f32; PCM_DMX_MAX_CHANNELS_US]; PCM_DMX_MAX_CHANNELS_US],
    ) -> Result<(), PcmDmxError> {
        let mut err = Ok(());
        let mut valid = [0; PCM_DMX_MAX_CHANNELS_US];

        // Check on a supported output configuration.
        // Add new one only after extensive testing!
        match out_ch_mode {
            ChannelMode::Mode_1_0_0_0
            | ChannelMode::Mode_2_0_0_0
            | ChannelMode::Mode_3_0_2_1
            | ChannelMode::Mode_3_0_4_1
            | ChannelMode::Mode_5_0_2_1 => {}
            _ => {
                debug_assert!(false);
            }
        }

        //  Convert channel config to channel mode:
        let mut in_ch_mode = match in_ch_cfg {
            ChannelCfg::Cfg0 => in_ch_mode,
            ChannelCfg::Cfg1
            | ChannelCfg::Cfg2
            | ChannelCfg::Cfg3
            | ChannelCfg::Cfg4
            | ChannelCfg::Cfg5
            | ChannelCfg::Cfg6 => OUT_CH_MODE_TABLE[in_ch_cfg as usize],
            ChannelCfg::Cfg7 => ChannelMode::Mode_5_0_2_1,
            ChannelCfg::Cfg11 => ChannelMode::Mode_3_0_3_1,
            ChannelCfg::Cfg12 => ChannelMode::Mode_3_0_4_1,
            ChannelCfg::Cfg14 => ChannelMode::Mode_5_0_2_1,
        };

        //  Extract the total number of input channels
        let num_in_channel = (in_ch_mode as i32 & 0xF)
            + ((in_ch_mode as i32 >> 4) & 0xF)
            + ((in_ch_mode as i32 >> 8) & 0xF)
            + ((in_ch_mode as i32 >> 12) & 0xF);
        //  Extract the total number of output channels
        let num_out_channel = (out_ch_mode as i32 & 0xF)
            + ((out_ch_mode as i32 >> 4) & 0xF)
            + ((out_ch_mode as i32 >> 8) & 0xF)
            + ((out_ch_mode as i32 >> 12) & 0xF);

        // MPEG ammendment 4 aka ETSI metadata and fallback mode:
        //  Create identity DMX matrix:
        for out_ch in 0..PCM_DMX_MAX_CHANNELS_US {
            Self::init_channel(mix_factors, out_ch);
        }
        if (in_ch_mode as i32 >> 12) & 0xF == 0 {
            //  Clear empty or wrongly mapped input channel
            Self::clear_channel(mix_factors, Channel::LowFrequency as usize);
        }

        //  FIRST STAGE:
        // Always use MPEG equations either with
        // meta data or with default values.
        if num_in_channel > ChannelCount::Six as i32 {
            let mut is_valid_cfg = true;

            //  Get factors from meta data
            let d_mix_fact_a = AB_MIX_LVL_VALUE_TAB[usize::from(meta_data.dmix_idx_a)];
            let d_mix_fact_b = AB_MIX_LVL_VALUE_TAB[usize::from(meta_data.dmix_idx_b)];

            //  Check if input is in the list of supported configurations
            match in_ch_mode {
                // ChannelMode::Mode_3_2_1_1: chCfg 11 but with side channels
                // ChannelMode::Mode_3_0_3_1: chCfg 11
                // 6.1ch:  C' = C;  L' = L;  R' = R;  LFE' = LFE
                //           Ls' = Ls*dmix_a_idx + Cs*dmix_b_idx
                //           Rs' = Rs*dmix_a_idx + Cs*dmix_b_idx
                // The index provided by `LEFT_MULTIPRPS_CHANNEL` simulates the `Cs`
                // channel in the below implementation of setting the mixFactors.
                ChannelMode::Mode_3_2_1_1
                | ChannelMode::Mode_3_2_1_0
                | ChannelMode::Mode_3_0_3_1 => {
                    match in_ch_mode {
                        ChannelMode::Mode_3_2_1_1 | ChannelMode::Mode_3_2_1_0 => {
                            is_valid_cfg = false;
                            err = Err(PcmDmxError::InvalidMode);
                        }
                        _ => {}
                    }

                    Self::clear_channel(mix_factors, Channel::RightMultiprps as usize);
                    Self::set_channel(
                        mix_factors,
                        Channel::LeftRear as usize,
                        Channel::LeftRear as usize,
                        d_mix_fact_a,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::LeftRear as usize,
                        Channel::LeftMultiprps as usize,
                        d_mix_fact_b,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::RightRear as usize,
                        Channel::RightRear as usize,
                        d_mix_fact_a,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::RightRear as usize,
                        Channel::LeftMultiprps as usize,
                        d_mix_fact_b,
                    );
                }
                // ChannelMode::Mode_3_0_4_1: chCfg 12:
                // 7.1ch Surround Back:  C' = C;  L' = L;  R' = R;  LFE' = LFE;
                // Ls' = Ls*dmix_a_idx + Lsr*dmix_b_idx;
                // Rs' = Rs*dmix_a_idx + Rsr*dmix_b_idx;
                ChannelMode::Mode_3_0_4_1 => {
                    Self::set_channel(
                        mix_factors,
                        Channel::LeftRear as usize,
                        Channel::LeftRear as usize,
                        d_mix_fact_a,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::LeftRear as usize,
                        Channel::LeftMultiprps as usize,
                        d_mix_fact_b,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::RightRear as usize,
                        Channel::RightRear as usize,
                        d_mix_fact_a,
                    );
                    Self::set_channel(
                        mix_factors,
                        Channel::RightRear as usize,
                        Channel::RightMultiprps as usize,
                        d_mix_fact_b,
                    );
                }
                // ChannelMode::Mode_5_0_2_1: chCfg 7 || 14
                // The index provided by `LEFT_REAR_CHANNEL` simulates the `Cs` channel in
                // the below implementation of setting the mixFactors. This channel is
                // copied on `LEFT_REAR_CHANNEL` and `RIGHT_REAR_CHANNEL` to form a
                // `CH_MODE_5_0_2_1` configuration.
                ChannelMode::Mode_5_0_1_0
                | ChannelMode::Mode_5_0_1_1
                | ChannelMode::Mode_5_2_1_0
                | ChannelMode::Mode_5_0_2_1 => {
                    match in_ch_mode {
                        ChannelMode::Mode_5_0_1_0 | ChannelMode::Mode_5_0_1_1 => {
                            Self::clear_channel(mix_factors, Channel::RightRear as usize);
                            Self::set_channel(
                                mix_factors,
                                Channel::RightRear as usize,
                                Channel::LeftRear as usize,
                                1.0,
                            );
                            Self::set_channel(
                                mix_factors,
                                Channel::LeftRear as usize,
                                Channel::LeftRear as usize,
                                1.0,
                            );
                            is_valid_cfg = false;
                            err = Err(PcmDmxError::InvalidMode);
                        }
                        ChannelMode::Mode_5_2_1_0 => {
                            is_valid_cfg = false;
                            err = Err(PcmDmxError::InvalidMode);
                        }
                        _ => {}
                    }
                    if in_ch_cfg == ChannelCfg::Cfg14 {
                        // 7.1ch Front Height:  C' = C;  Ls' = Ls;  Rs' = Rs;  LFE' = LFE;
                        // L' = L*dmix_a_idx + Lv*dmix_b_idx;
                        // R' = R*dmix_a_idx + Rv*dmix_b_idx;
                        Self::set_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftFront as usize,
                            d_mix_fact_a,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftMultiprps as usize,
                            d_mix_fact_b,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightFront as usize,
                            d_mix_fact_a,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightMultiprps as usize,
                            d_mix_fact_b,
                        );
                    } else {
                        // 7.1ch Front:  Ls' = Ls;  Rs' = Rs;  LFE' = LFE;
                        // C' = C + (Lc+Rc)*dmix_a_idx;
                        // L' = L + Lc*dmix_b_idx;
                        // R' = R + Rc*dmix_b_idx;
                        Self::set_channel(
                            mix_factors,
                            Channel::CenterFront as usize,
                            Channel::LeftMultiprps as usize,
                            d_mix_fact_a,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::CenterFront as usize,
                            Channel::RightMultiprps as usize,
                            d_mix_fact_a,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftMultiprps as usize,
                            d_mix_fact_b,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftFront as usize,
                            1.0,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightMultiprps as usize,
                            d_mix_fact_b,
                        );
                        Self::set_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightFront as usize,
                            1.0,
                        );
                    }
                }
                _ => {
                    //  Nothing to do. Just use the identity matrix.
                    is_valid_cfg = false;
                    err = Err(PcmDmxError::InvalidMode);
                }
            }

            //  Add additional DMX gain
            if is_valid_cfg && meta_data.dmx_gain_idx5 != 0 {
                let sign = if meta_data.dmx_gain_idx5 & 0x40 != 0 {
                    -1.0
                } else {
                    1.0
                };
                let val = meta_data.dmx_gain_idx5 & 0x3F;

                //  10^(dmx_gain_5/80)
                let dmx_gain = 10.0f32.powf(sign * f32::from(val) * 0.0125);

                Self::set_channel(
                    mix_factors,
                    Channel::CenterFront as usize,
                    Channel::CenterFront as usize,
                    dmx_gain,
                );
                Self::set_channel(
                    mix_factors,
                    Channel::LeftFront as usize,
                    Channel::LeftFront as usize,
                    dmx_gain,
                );
                Self::set_channel(
                    mix_factors,
                    Channel::RightFront as usize,
                    Channel::RightFront as usize,
                    dmx_gain,
                );
                Self::set_channel(
                    mix_factors,
                    Channel::LeftRear as usize,
                    Channel::LeftRear as usize,
                    dmx_gain,
                );
                Self::set_channel(
                    mix_factors,
                    Channel::RightRear as usize,
                    Channel::RightRear as usize,
                    dmx_gain,
                );
                Self::set_channel(
                    mix_factors,
                    Channel::LowFrequency as usize,
                    Channel::LowFrequency as usize,
                    dmx_gain,
                );
            }

            //  Mark the output channels
            valid[Channel::CenterFront as usize] = 1;
            valid[Channel::LeftFront as usize] = 1;
            valid[Channel::RightFront as usize] = 1;
            valid[Channel::LeftRear as usize] = 1;
            valid[Channel::RightRear as usize] = 1;
            valid[Channel::LowFrequency as usize] = 1;

            //  Update channel mode for the next stage
            in_ch_mode = ChannelMode::Mode_3_0_2_1;
        }

        // For the X (> 6) to 6 channel downmix we had no choice.
        // To mix from 6 to 2 (or 1) channel(s) we have several possibilities (MPEG
        // DSE | MPEG PCE | ITU | ARIB | DLB). Use profile and the metadata
        // available flags to determine which equation to use:
        // default
        let mut dmx_method = MetadataProfile::MpegAmd4;

        if params.dmx_profile == ProfileType::ForceMatrixMix
            && (meta_data.type_flags & BsDataType::PceData as u32 != 0)
        {
            dmx_method = MetadataProfile::MpegLegacy;
        } else if (meta_data.type_flags
            & (BsDataType::DseClevData as u32 | BsDataType::DseSlevData as u32))
            == 0
        {
            match params.dmx_profile {
                ProfileType::Standard => {
                    //  dmxMethod = DMX_METHOD_MPEG_AMD4
                }
                ProfileType::MatrixMix | ProfileType::ForceMatrixMix => {
                    if meta_data.type_flags & BsDataType::PceData as u32 != 0 {
                        dmx_method = MetadataProfile::MpegLegacy;
                    }
                }
                ProfileType::AribJapan => dmx_method = MetadataProfile::AribJapan,
            }
        }

        //  SECOND STAGE:
        if num_out_channel <= ChannelCount::Two as i32 {
            //  Create DMX matrix according to input configuration
            match in_ch_mode {
                ChannelMode::Mode_2_0_0_0 => {}
                ChannelMode::Mode_2_0_1_0 => {
                    //  L' = L + 0.707*S;  R' = R + 0.707*S;
                    let s_mix_lvl: f32 = 0.707;
                    Self::add_channel(
                        mix_factors,
                        Channel::LeftFront as usize,
                        // The index provided by `LEFT_REAR_CHANNEL` simulates the `S`
                        // channel in the below implementation of setting the mixFactors.
                        Channel::LeftRear as usize,
                        s_mix_lvl,
                    );
                    Self::add_channel(
                        mix_factors,
                        Channel::RightFront as usize,
                        // The index provided by `LEFT_REAR_CHANNEL` simulates the `S`
                        // channel in the below implementation of setting the mixFactors.
                        Channel::LeftRear as usize,
                        s_mix_lvl,
                    );
                }
                ChannelMode::Mode_3_0_0_0 => {
                    //  L' = L + 0.707*C;  R' = R + 0.707*C;
                    let c_mix_lvl: f32 = 0.707;
                    Self::add_channel(
                        mix_factors,
                        Channel::LeftFront as usize,
                        Channel::CenterFront as usize,
                        c_mix_lvl,
                    );
                    Self::add_channel(
                        mix_factors,
                        Channel::RightFront as usize,
                        Channel::CenterFront as usize,
                        c_mix_lvl,
                    );
                }
                ChannelMode::Mode_2_0_2_0 => {
                    //  ARIB 2/2
                    let s_mix_lvl: f32 = 0.707;
                    if dmx_method == MetadataProfile::AribJapan {
                        //  L' = L + 0.707*Ls;  R' = R + 0.707*Rs;
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftRear as usize,
                            s_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightRear as usize,
                            s_mix_lvl,
                        );
                    }
                }
                ChannelMode::Mode_3_0_1_0 => {
                    // L' = L + 0.707*C + 0.707*S
                    // R' = R + 0.707*C + 0.707*S
                    let cs_mix_lvl: f32 = 0.707;
                    // The index provided by `LeftRear` simulates the `S`
                    // channel in the below implementation of setting the mixFactors.
                    Self::add_channel(
                        mix_factors,
                        Channel::LeftFront as usize,
                        Channel::CenterFront as usize,
                        cs_mix_lvl,
                    );
                    Self::add_channel(
                        mix_factors,
                        Channel::LeftFront as usize,
                        Channel::LeftRear as usize,
                        cs_mix_lvl,
                    );
                    Self::add_channel(
                        mix_factors,
                        Channel::RightFront as usize,
                        Channel::CenterFront as usize,
                        cs_mix_lvl,
                    );
                    Self::add_channel(
                        mix_factors,
                        Channel::RightFront as usize,
                        Channel::LeftRear as usize,
                        cs_mix_lvl,
                    );
                }
                ChannelMode::Mode_3_0_2_0 | ChannelMode::Mode_3_0_2_1 => match dmx_method {
                    MetadataProfile::MpegAmd4 => {
                        //  Get factors from meta data

                        let c_mix_lvl: f32 = AB_MIX_LVL_VALUE_TAB[usize::from(meta_data.c_lev_idx)];
                        let s_mix_lvl: f32 = AB_MIX_LVL_VALUE_TAB[usize::from(meta_data.s_lev_idx)];
                        let l_mix_lvl: f32 =
                            LFE_MIX_LVL_VALUE_TAB[usize::from(meta_data.dmix_idx_lfe)];
                        //  Setup the DMX matrix
                        // L' = L + C*clev + Ls*slev + LFE*llev
                        // R' = R + C*clev + Rs*slev + LFE*llev
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::CenterFront as usize,
                            c_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftRear as usize,
                            s_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LowFrequency as usize,
                            l_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::CenterFront as usize,
                            c_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightRear as usize,
                            s_mix_lvl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::LowFrequency as usize,
                            l_mix_lvl,
                        );

                        //  Add additional DMX gain
                        if meta_data.dmx_gain_idx2 != 0 {
                            //  Apply DMX gain 2
                            let sign = if meta_data.dmx_gain_idx2 & 0x40 != 0 {
                                -1.0
                            } else {
                                1.0
                            };
                            let val = meta_data.dmx_gain_idx2 & 0x3F;

                            //  10^(dmx_gain_2/80)
                            let dmx_gain = 10.0f32.powf(sign * f32::from(val) * 0.0125);

                            Self::set_channel(
                                mix_factors,
                                Channel::LeftFront as usize,
                                Channel::LeftFront as usize,
                                dmx_gain,
                            );
                            Self::set_channel(
                                mix_factors,
                                Channel::RightFront as usize,
                                Channel::RightFront as usize,
                                dmx_gain,
                            );
                        }
                    }
                    MetadataProfile::MpegLegacy => {
                        let mtrx_mix_dwn_coef: f32 =
                            MPEG_MIX_DOWN_IDX2_COEF[usize::from(meta_data.matrix_mixdown_idx)];

                        // 3/2 input: L' = (1.707+A)^-1 * [L+0.707*C+A*Ls]
                        // R' = (1.707+A)^-1 * [R+0.707*C+A*Rs]
                        let flev = MPEG_MIX_DOWN_IDX2_PRE_FACT[0]
                            [usize::from(meta_data.matrix_mixdown_idx)];
                        let slev_ll = flev * mtrx_mix_dwn_coef;
                        let slev_rr = slev_ll;
                        let slev_rl = 0.0;
                        let slev_lr = slev_rl;

                        //  common factor
                        //  0.707
                        let clev = flev * MPEG_MIX_DOWN_IDX2_COEF[0];

                        Self::set_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftFront as usize,
                            flev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::CenterFront as usize,
                            clev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftRear as usize,
                            slev_ll,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::RightRear as usize,
                            slev_lr,
                        );

                        Self::set_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightFront as usize,
                            flev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::CenterFront as usize,
                            clev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::LeftRear as usize,
                            slev_rl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightRear as usize,
                            slev_rr,
                        );
                    }
                    MetadataProfile::AribJapan => {
                        let mtrx_mix_dwn_coef: f32 =
                            MPEG_MIX_DOWN_IDX2_COEF[usize::from(meta_data.matrix_mixdown_idx)];

                        // 3/2 input:
                        // L' = L+0.707*C+k*Ls
                        // R' = R+0.707*C+k*Rs
                        let slev_ll = mtrx_mix_dwn_coef;
                        let slev_rr = slev_ll;
                        let slev_rl = 0.0;
                        let slev_lr = slev_rl;

                        //  common factor
                        //  0.707
                        let clev = MPEG_MIX_DOWN_IDX2_COEF[0];

                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::CenterFront as usize,
                            clev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::LeftRear as usize,
                            slev_ll,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::LeftFront as usize,
                            Channel::RightRear as usize,
                            slev_lr,
                        );

                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::CenterFront as usize,
                            clev,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::LeftRear as usize,
                            slev_rl,
                        );
                        Self::add_channel(
                            mix_factors,
                            Channel::RightFront as usize,
                            Channel::RightRear as usize,
                            slev_rr,
                        );
                    }
                    _ => {}
                },
                _ => {
                    //  This configuration does not fit to any known downmix equation!
                    err = Err(PcmDmxError::InvalidMode);
                }
            }

            //  Mark the output channels
            valid.fill(0);
            valid[Channel::LeftFront as usize] = 1;
            valid[Channel::RightFront as usize] = 1;
        }

        if num_out_channel == ChannelCount::One as i32 {
            let mut mono_mix_level: f32;

            //  C is not in the mix
            Self::clear_channel(mix_factors, Channel::CenterFront as usize);
            if dmx_method == MetadataProfile::MpegLegacy {
                mono_mix_level =
                    MPEG_MIX_DOWN_IDX2_PRE_FACT[2][usize::from(meta_data.matrix_mixdown_idx)];

                mix_factors[Channel::CenterFront as usize][Channel::CenterFront as usize] =
                    mono_mix_level;
                mix_factors[Channel::CenterFront as usize][Channel::LeftFront as usize] =
                    mono_mix_level;
                mix_factors[Channel::CenterFront as usize][Channel::RightFront as usize] =
                    mono_mix_level;
                mono_mix_level *=
                    MPEG_MIX_DOWN_IDX2_COEF[usize::from(meta_data.matrix_mixdown_idx)];
                mix_factors[Channel::CenterFront as usize][Channel::LeftRear as usize] =
                    mono_mix_level;
                mix_factors[Channel::CenterFront as usize][Channel::RightRear as usize] =
                    mono_mix_level;
            } else {
                match dmx_method {
                    MetadataProfile::MpegAmd4 => {
                        //  C' = L + R;
                        mono_mix_level = 1.0;
                    }
                    _ => {
                        //  C' = 0.5*L + 0.5*R;
                        mono_mix_level = 0.5;
                    }
                }
                Self::set_channel(
                    mix_factors,
                    Channel::CenterFront as usize,
                    Channel::LeftFront as usize,
                    mono_mix_level,
                );
                Self::add_channel(
                    mix_factors,
                    Channel::CenterFront as usize,
                    Channel::RightFront as usize,
                    mono_mix_level,
                );
            }

            //  Mark the output channel
            valid[Channel::CenterFront as usize] = 1;
            valid[1..].fill(0);
        }

        err
    }

    /// Applies the selected dual channel mode to the given PCM buffer.
    ///
    /// # Parameters
    ///
    /// - `dual_channel_mode`: The selected dual channel mode.
    /// - `pcm_buf`: Pointer to time buffer with PCM samples.
    /// - `frame_size`: The I/O block size which is the number of samples per channel.
    /// - `n_channels`: Pointer to buffer that holds the number of input channels and where the
    ///   amount of output channels is explicitly set to 1 channel, in case of processing.
    ///
    /// # Return
    ///
    /// An error code
    fn apply_dual_channel(
        dual_channel_mode: DualChannelMode,
        pcm_buf: &mut [f32],
        frame_size: usize,
        n_channels: &mut u8,
    ) -> PcmDmxError {
        let pcm_buf_size: usize = pcm_buf.len();

        let num_in_channels = *n_channels;
        //  One channel to the output, if the selected dualChannelMode is supported.
        let num_out_channels = ChannelMode::Mode_1_0_0_0 as u8;

        if pcm_buf.is_empty() {
            return PcmDmxError::InvalidArgument;
        }
        if frame_size == 0 {
            return PcmDmxError::InvalidArgument;
        }
        if num_in_channels == 0 {
            return PcmDmxError::InvalidArgument;
        }
        // dual channel should not be able to handle signals with more than 2 input
        // channels.
        if num_in_channels > 2 {
            return PcmDmxError::InvalidChConfig;
        }

        //  Check I/O buffer size
        if pcm_buf_size < 2 * frame_size {
            return PcmDmxError::OutputBufferTooSmall;
        }

        //  2/0 input:
        match dual_channel_mode {
            DualChannelMode::Ch1 => {
                //  Nothing to do, left channel already in place.
            }
            DualChannelMode::Ch2 => {
                let (left, right) = pcm_buf.split_at_mut(frame_size);
                left.copy_from_slice(right[0..frame_size].as_ref());
            }
            DualChannelMode::Mixed => {
                let (left, right) = pcm_buf.split_at_mut(frame_size);
                izip!(left.iter_mut(), right.iter()).for_each(|(l, r)| *l = (*l + *r) * 0.5);
            }
            DualChannelMode::Stereo => {
                //  nothing to do
            }
        }

        if dual_channel_mode == DualChannelMode::Ch1
            || dual_channel_mode == DualChannelMode::Ch2
            || dual_channel_mode == DualChannelMode::Mixed
        {
            *n_channels = num_out_channels;
        }

        PcmDmxError::Ok
    }

    /// Gets the DMX Profile Setting.
    pub fn dmx_profile_setting(&self) -> ProfileType {
        self.params.dmx_profile
    }

    /// Gets the DMX Bitstream Data Expiry Frame.
    pub fn get_dmx_bs_data_expiry_frame(&self) -> u32 {
        self.params.expiry_frame
    }

    /// Gets the DMX Bitstream Data Delay.
    pub fn get_dmx_bs_data_delay(&self) -> u8 {
        self.params.frame_delay
    }

    /// Gets the minimum number of output channels.
    pub fn get_min_number_of_output_channels(&self) -> u8 {
        self.params.num_out_channels_min
    }

    /// Gets the maximum number of output channels.
    pub fn get_max_number_of_output_channels(&self) -> u8 {
        self.params.num_out_channels_max
    }

    /// Gets the DMX Dual Channel Mode.
    pub fn get_dmx_dual_channel_mode(&self) -> DualChannelMode {
        self.params.dual_channel_mode
    }
}

#[cfg(test)]
mod test_pcm_dmx {
    use super::*;
    use crate::common::channel_map_descr::ChannelMapDescriptor;
    use crate::common::channel_order::ChannelOrder;

    #[derive(Clone)]
    struct PrbsF32 {
        lfsr: u64,
        u1: f32,
        u2: f32,
        toggle: bool,
    }

    impl PrbsF32 {
        fn new() -> Self {
            PrbsF32 {
                lfsr: 0x123456789abcdef0,
                u1: 0.0,
                u2: 0.0,
                toggle: false,
            }
        }
    }

    impl Iterator for PrbsF32 {
        type Item = f32;

        fn next(&mut self) -> Option<Self::Item> {
            if !self.toggle {
                self.toggle = true;
                self.lfsr = (((self.lfsr as u128) << 32) & 0xffffffffffffffff) as u64
                    | (((self.lfsr << 1) ^ (self.lfsr << 2)) >> 32);
                self.lfsr = (((self.lfsr as u128) << 32) & 0xffffffffffffffff) as u64
                    | (((self.lfsr << 1) ^ (self.lfsr << 2)) >> 32);
                self.u1 = self.lfsr as Self::Item / u64::MAX as Self::Item;
                self.lfsr = (((self.lfsr as u128) << 32) & 0xffffffffffffffff) as u64
                    | (((self.lfsr << 1) ^ (self.lfsr << 2)) >> 32);
                self.lfsr = (((self.lfsr as u128) << 32) & 0xffffffffffffffff) as u64
                    | (((self.lfsr << 1) ^ (self.lfsr << 2)) >> 32);
                self.u2 = self.lfsr as Self::Item / u64::MAX as Self::Item;
                let z1: Self::Item =
                    (self.u1.ln() * -2.0).sqrt() * (self.u2 * 2.0 * std::f32::consts::PI).cos();
                Some(z1)
            } else {
                self.toggle = false;
                let z2: Self::Item =
                    (self.u1.ln() * -2.0).sqrt() * (self.u2 * 2.0 * std::f32::consts::PI).sin();
                Some(z2)
            }
        }
    }

    #[test]
    fn get_param() {
        const N_OUT: u8 = 2;
        let mut dmx = PcmDmx::new();

        let mut dmx_params = PcmDmxParams::default();
        assert!(dmx_params
            .set(PcmDmxParam::DmxProfileSetting(ProfileType::Standard))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MinNumberOfOutputChannels(N_OUT))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MaxNumberOfOutputChannels(N_OUT))
            .is_ok());

        dmx.params_update(&mut dmx_params);

        assert_eq!(ProfileType::Standard, dmx.dmx_profile_setting());

        assert_eq!(0, dmx.get_dmx_bs_data_expiry_frame());

        assert_eq!(0, dmx.get_dmx_bs_data_delay());

        assert_eq!(N_OUT, dmx.get_min_number_of_output_channels());

        assert_eq!(N_OUT, dmx.get_max_number_of_output_channels());

        assert_eq!(DualChannelMode::Stereo, dmx.get_dmx_dual_channel_mode());
    }

    #[test]
    fn downmix_5_1_to_2() {
        let frame_size: u16 = 4096;
        const N_IN: u8 = 6;
        const N_OUT: u8 = 2;
        const N_MAX: u8 = 6;
        let mut dmx = PcmDmx::new();

        let mut dmx_params = PcmDmxParams::default();
        assert!(dmx_params
            .set(PcmDmxParam::DmxProfileSetting(ProfileType::Standard))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MinNumberOfOutputChannels(N_OUT))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MaxNumberOfOutputChannels(N_OUT))
            .is_ok());

        dmx.params_update(&mut dmx_params);

        let mut map_descr = ChannelMapDescriptor::default();
        map_descr.init(None, ChannelOrder::Mpeg);
        assert!(map_descr.is_valid());

        let in_channel_type = [
            AudioChannelType::Front,
            AudioChannelType::Front,
            AudioChannelType::Front,
            AudioChannelType::Back,
            AudioChannelType::Back,
            AudioChannelType::Lfe,
        ];
        let in_channel_indices = [0, 1, 2, 3, 4, 5];

        dmx.set_dmx_idx_lfe(5);

        let mut out_channel_type = [AudioChannelType::default(); N_OUT as usize];
        let mut out_channel_indices = [0; N_OUT as usize];

        let mut out_energy_matrix = Vec::new();

        let mut rnd = PrbsF32::new();
        for in_ch in 0..N_MAX as usize {
            let mut pcm_buf = Vec::with_capacity(N_MAX as usize * usize::from(frame_size));
            for _ in 0..N_MAX as u16 * frame_size {
                pcm_buf.push(rnd.next().unwrap());
            }

            for ch in 0..N_MAX as usize {
                let ch_pcm =
                    &mut pcm_buf[ch * usize::from(frame_size)..(ch + 1) * usize::from(frame_size)];
                if ch != in_ch {
                    ch_pcm.fill(0.0);
                }
            }

            let mut pcm_tmp_buf = vec![0.0; N_MAX as usize * usize::from(frame_size)];

            let mut n_channels = N_IN;
            let mut channel_type = [AudioChannelType::default(); N_MAX as usize];
            channel_type[0..N_IN as usize].copy_from_slice(&in_channel_type[0..N_IN as usize]);
            let mut channel_indices = [0; N_MAX as usize];
            channel_indices[0..N_IN as usize]
                .copy_from_slice(&in_channel_indices[0..N_IN as usize]);
            assert!(dmx
                .apply_frame(
                    &mut pcm_buf,
                    &mut pcm_tmp_buf,
                    frame_size,
                    &mut n_channels,
                    &mut channel_type,
                    &mut channel_indices,
                    &mut map_descr,
                )
                .is_ok());
            assert!(n_channels == N_OUT);

            out_channel_type.copy_from_slice(&channel_type[0..N_OUT as usize]);
            out_channel_indices.copy_from_slice(&channel_indices[0..N_OUT as usize]);

            let mut out_energy_row = Vec::new();
            for ch in 0..N_MAX as usize {
                let ch_pcm =
                    &pcm_buf[ch * usize::from(frame_size)..(ch + 1) * usize::from(frame_size)];
                let energy = (ch_pcm.iter().map(|x| x * x).sum::<f32>() / frame_size as f32).sqrt();
                out_energy_row.push(energy);
            }
            out_energy_matrix.push(out_energy_row);
        }

        let expected: Vec<Vec<f32>> = vec![
            vec![0.707, 0.707],
            vec![1.0, 0.0],
            vec![0.0, 1.0],
            vec![0.707, 0.0],
            vec![0.0, 0.707],
            vec![1.0, 1.0],
        ];
        for in_ch in 0..N_MAX {
            for out_ch in 0..N_MAX {
                if in_ch < N_IN && out_ch < N_OUT {
                    assert!(
                        (out_energy_matrix[usize::from(in_ch)][usize::from(out_ch)]
                            - expected[usize::from(in_ch)][usize::from(out_ch)])
                        .abs()
                            < 0.05
                    );
                }
            }
        }
    }

    #[test]
    fn upmix_2_to_5_1() {
        let frame_size: u16 = 4096;
        const N_IN: u8 = 2;
        const N_OUT: u8 = 6;
        const N_MAX: u8 = 6;
        let mut dmx = PcmDmx::new();

        let mut dmx_params = PcmDmxParams::default();
        assert!(dmx_params
            .set(PcmDmxParam::DmxProfileSetting(ProfileType::Standard))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MinNumberOfOutputChannels(N_OUT))
            .is_ok());
        assert!(dmx_params
            .set(PcmDmxParam::MaxNumberOfOutputChannels(N_OUT))
            .is_ok());
        // dmx_params.set(
        //     PcmdmxParam::DmxDualChannelMode,
        //     DualChannelMode::StereoMode as i32,
        // );

        dmx.params_update(&mut dmx_params);

        let mut map_descr = ChannelMapDescriptor::default();
        map_descr.init(None, ChannelOrder::Mpeg);
        assert!(map_descr.is_valid());

        let in_channel_type = [AudioChannelType::Front, AudioChannelType::Front];
        let in_channel_indices = [0, 1];

        dmx.set_dmx_idx_lfe(5);

        let mut out_channel_type = [AudioChannelType::default(); N_MAX as usize];
        let mut out_channel_indices = [0; N_MAX as usize];

        let mut out_energy_matrix = Vec::new();

        let mut rnd = PrbsF32::new();
        for in_ch in 0..N_MAX as usize {
            let mut pcm_buf = Vec::with_capacity(N_MAX as usize * usize::from(frame_size));
            for _ in 0..u16::from(N_MAX) * frame_size {
                pcm_buf.push(rnd.next().unwrap());
            }

            for ch in 0..N_MAX as usize {
                let ch_pcm =
                    &mut pcm_buf[ch * usize::from(frame_size)..(ch + 1) * usize::from(frame_size)];
                if ch != in_ch {
                    ch_pcm.fill(0.0);
                }
            }

            let mut pcm_tmp_buf = vec![0.0; N_MAX as usize * usize::from(frame_size)];

            let mut n_channels = N_IN;
            let mut channel_type = [AudioChannelType::default(); N_MAX as usize];
            channel_type[0..N_IN as usize].copy_from_slice(&in_channel_type[0..N_IN as usize]);
            let mut channel_indices = [0; N_MAX as usize];
            channel_indices[0..N_IN as usize]
                .copy_from_slice(&in_channel_indices[0..N_IN as usize]);
            assert!(dmx
                .apply_frame(
                    &mut pcm_buf,
                    &mut pcm_tmp_buf,
                    frame_size,
                    &mut n_channels,
                    &mut channel_type,
                    &mut channel_indices,
                    &mut map_descr,
                )
                .is_ok());
            assert!(n_channels == N_OUT);

            out_channel_type.copy_from_slice(&channel_type[0..N_MAX as usize]);
            out_channel_indices.copy_from_slice(&channel_indices[0..N_MAX as usize]);

            let mut out_energy_row = Vec::new();
            for ch in 0..N_MAX as usize {
                let ch_pcm =
                    &pcm_buf[ch * usize::from(frame_size)..(ch + 1) * usize::from(frame_size)];
                let energy = (ch_pcm.iter().map(|x| x * x).sum::<f32>() / frame_size as f32).sqrt();
                out_energy_row.push(energy);
            }
            out_energy_matrix.push(out_energy_row);
        }

        let expected: Vec<Vec<f32>> = vec![
            vec![0.0, 1.0, 0.0, 0.0, 0.0, 0.0],
            vec![0.0, 0.0, 1.0, 0.0, 0.0, 0.0],
        ];
        for in_ch in 0..N_MAX {
            for out_ch in 0..N_MAX {
                if in_ch < N_IN && out_ch < N_OUT {
                    assert!(
                        (out_energy_matrix[usize::from(in_ch)][usize::from(out_ch)]
                            - expected[usize::from(in_ch)][usize::from(out_ch)])
                        .abs()
                            < 0.05
                    );
                }
            }
        }
    }
}
