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
//! Immediate playout frame module (IPF).

use crate::aac_dec::error_codes::AacDecoderError;
use crate::tp_dec::{TransportDec, TP_USAC_MAX_CONFIG_LEN};
use itertools::izip;

pub const IPFDATA_MAX_NUM_PREROLL_AU: usize = 3;
pub const IPFDATA_MAX_NUM_PREROLL_AU_USAC: u32 = 3;
/// USAC is supported for a maximum of 2 channels.
pub const IPFDATA_FLUSH_CHANNELS: usize = 2;
pub const IPFDATA_TIME_DATA_FLUSH_SIZE: usize = 128;

#[repr(C)]
#[derive(Debug)]
pub struct IpfData {
    /// Indicates flush status: on|off
    is_flush_on: bool,
    /// Indicates build up status: on|off
    is_build_up_on: bool,
    /// Indicates preRoll status: on|off
    has_audio_pre_roll: bool,
    /// Relative offset of the preroll AU end position to the AU start position in the bitstream
    preroll_au_length: [u32; IPFDATA_MAX_NUM_PREROLL_AU + 1],
    /// Offset to each preRoll AU
    preroll_au_offset: [u32; IPFDATA_MAX_NUM_PREROLL_AU],
    /// Start anchor of AU
    au_start_anchor: i32,
    access_unit: u32,
    /// Number of the actual processed preroll access_unit
    num_preroll_au: u32,
    /// Indicates whether cross-fade for seamless stream switching is applied
    is_apply_crossfade: bool,
    /// Memory for flushed time data which will be used for the crossfade in case of an USAC DASH
    /// IPF config change
    time_data_flush: [[f32; IPFDATA_TIME_DATA_FLUSH_SIZE]; IPFDATA_FLUSH_CHANNELS],
}

impl Default for IpfData {
    fn default() -> Self {
        Self {
            is_flush_on: false,
            is_build_up_on: false,
            has_audio_pre_roll: false,
            preroll_au_length: [0; IPFDATA_MAX_NUM_PREROLL_AU + 1],
            preroll_au_offset: [0; IPFDATA_MAX_NUM_PREROLL_AU],
            au_start_anchor: 0,
            access_unit: 0,
            num_preroll_au: 0,
            is_apply_crossfade: false,
            time_data_flush: [[0.0; IPFDATA_TIME_DATA_FLUSH_SIZE]; IPFDATA_FLUSH_CHANNELS],
        }
    }
}

impl IpfData {
    /// Creates a new `IpfData` instance.
    pub fn new() -> Self {
        IpfData::default()
    }

    /// Retrieves the preroll extension payload data if it is available in a bitstream.
    ///
    /// # Parameters
    ///
    /// - `tp_dec`: Reference to a `TransportDec` structure
    /// - `is_startup_phase`: Indicates decoder start up phase
    pub fn preroll_extension_payload(
        &mut self,
        tp_dec: &mut TransportDec,
        is_startup_phase: bool,
    ) -> AacDecoderError {
        let mut error_status = AacDecoderError::Ok;

        // Bit stream pointer needs to be at the beginning of a (valid) AU.
        if self.access_unit == 0 && self.has_audio_pre_roll {
            error_status = self.preroll_extension_payload_parse(tp_dec, is_startup_phase);
        }

        if error_status != AacDecoderError::NotEnoughBits {
            if self.access_unit == 0 {
                self.au_start_anchor = tp_dec.bs.valid_bits() as i32;
            }
            if self.au_start_anchor > 0 {
                let mut bs_offset: isize = tp_dec.bs.valid_bits() - self.au_start_anchor as isize;
                if self.access_unit < self.num_preroll_au {
                    bs_offset += self.preroll_au_offset[self.access_unit as usize] as isize;
                }
                tp_dec.bs.push(bs_offset);
            }
        }
        error_status
    }

    /// Prepares, applies and controls crossfade an IPF config change.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time data (block based data)
    /// - `num_channels`: Number of channels
    /// - `frame_size`: Size of frame
    /// - `is_frame_ok`: Successfully decoded audio data
    pub fn process(
        &mut self,
        time_data: &mut [f32],
        num_channels: usize,
        frame_size: usize,
        is_frame_ok: bool,
    ) {
        if self.is_flush_on && is_frame_ok {
            self.prepare_crossfade(time_data, num_channels, frame_size);
        }

        if self.is_build_up_on && self.access_unit >= self.num_preroll_au {
            if self.is_apply_crossfade && is_frame_ok {
                self.apply_crossfade(time_data, num_channels, frame_size);
            }
            self.is_apply_crossfade = false;
            self.is_build_up_on = false;
        }

        if !self.is_flush_on {
            self.access_unit += 1;
        }
    }

    /// Get preroll AU length.
    pub fn preroll_au_length(&self) -> u32 {
        if self.access_unit <= self.num_preroll_au {
            let au = self.access_unit as usize;
            self.preroll_au_length[au]
        } else {
            0
        }
    }

    /// Flag if flush status is on.
    pub fn is_flush_on(&self) -> bool {
        self.is_flush_on
    }

    /// Flag if is end of access unit.
    pub fn is_end_access_unit(&self) -> bool {
        !self.is_flush_on && (self.access_unit >= self.num_preroll_au)
    }

    /// Resets IPF structure.
    ///
    /// # Parameters
    ///
    /// - `has_audio_pre_roll`: Indicates the presence of an audio preroll in the bitstream
    pub fn reset(&mut self, has_audio_pre_roll: bool) {
        *self = Default::default();
        self.has_audio_pre_roll = has_audio_pre_roll;
    }

    /// Parse PreRoll Extension Payload
    ///
    /// # Parameters
    /// - `tp_dec`: Transport decoder structure
    /// - `is_startup_phase`: Decoder start up phase
    fn preroll_extension_payload_parse(
        &mut self,
        tp_dec: &mut TransportDec,
        is_startup_phase: bool,
    ) -> AacDecoderError {
        let mut error_status = AacDecoderError::Ok;
        let au_start_anchor = tp_dec.bs.valid_bits();

        if au_start_anchor <= 0 {
            let valid_bits = tp_dec.bs.valid_bits();
            tp_dec.bs.push(valid_bits - au_start_anchor);
            return AacDecoderError::NotEnoughBits;
        }

        let usac_independency_flag = tp_dec.bs.read_bit() != 0;
        let usac_ext_element_present = tp_dec.bs.read_bit() != 0;
        let usac_ext_element_use_default_length = tp_dec.bs.read_bit() != 0;

        if usac_independency_flag
            && usac_ext_element_present
            && !usac_ext_element_use_default_length
        {
            // Read overall ext payload length.
            tp_dec.bs.escaped_value(8, 16, 0);

            // Read config size
            let config_length = tp_dec.bs.escaped_value(4, 4, 8) as usize;

            if config_length > 0 {
                // DASH IPF USAC Config Change: Read new config and compare with current
                // config. Apply reconfiguration if config's are different.
                let mut config = [0_u8; TP_USAC_MAX_CONFIG_LEN];
                for value in config.iter_mut().take(config_length) {
                    *value = tp_dec.bs.read(8) as u8;
                }
                if tp_dec
                    .in_band_config(
                        &mut config,
                        config_length,
                        &mut self.is_flush_on,
                        &mut self.is_build_up_on,
                        is_startup_phase,
                    )
                    .is_err()
                {
                    let valid_bits = tp_dec.bs.valid_bits();
                    tp_dec.bs.push(valid_bits - au_start_anchor);
                    return AacDecoderError::ParseError;
                }

                // We are interested in preroll AUs if an explicit or an implicit config
                // change is signalized. In other words if the build up status is set.
                if self.is_build_up_on {
                    self.is_apply_crossfade = tp_dec.bs.read_bit() != 0;
                    // reserved bit
                    _ = tp_dec.bs.read_bit();
                    // Read num preroll AU's
                    self.num_preroll_au = tp_dec.bs.escaped_value(2, 4, 0);
                    // check limits for USAC
                    if self.num_preroll_au > IPFDATA_MAX_NUM_PREROLL_AU_USAC {
                        error_status = AacDecoderError::ParseError;
                    } else {
                        for (preroll_au, preroll_au_length, preroll_au_offset) in izip!(
                            0..self.num_preroll_au,
                            self.preroll_au_length.iter_mut(),
                            self.preroll_au_offset.iter_mut()
                        ) {
                            // For every AU get length and offset in the bitstream
                            let read_preroll_au_length = tp_dec.bs.escaped_value(16, 16, 0);
                            if read_preroll_au_length > 0
                                && (read_preroll_au_length * 8) as isize <= tp_dec.bs.valid_bits()
                            {
                                *preroll_au_offset =
                                    u32::try_from(au_start_anchor - tp_dec.bs.valid_bits())
                                        .unwrap();
                                let independency_flag = tp_dec.bs.read_bit() != 0;
                                if preroll_au == 0 && !independency_flag {
                                    error_status = AacDecoderError::ParseError;
                                    break;
                                }
                                tp_dec.bs.push(((read_preroll_au_length * 8) - 1) as isize);
                                *preroll_au_length = read_preroll_au_length * 8;
                            } else {
                                // Something is wrong
                                error_status = AacDecoderError::ParseError;
                                break;
                            }
                        }
                    }
                    if error_status == AacDecoderError::ParseError {
                        self.num_preroll_au = 0;
                    }
                }
            }
        }
        let valid_bits = tp_dec.bs.valid_bits();
        tp_dec.bs.push(valid_bits - au_start_anchor);
        error_status
    }

    /// Prepares crossfade for USAC DASH IPF config change
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time data
    /// - `num_channels`: Number of channels
    /// - `frame_size`: Size of frame
    fn prepare_crossfade(&mut self, time_data: &[f32], num_channels: usize, frame_size: usize) {
        for (time_data_chunk, time_data_flush_chunk) in izip!(
            time_data
                .chunks_exact(frame_size)
                .map(|chunk| &chunk[..IPFDATA_TIME_DATA_FLUSH_SIZE]),
            self.time_data_flush.iter_mut(),
        )
        .take(num_channels)
        {
            time_data_flush_chunk.copy_from_slice(time_data_chunk);
        }
    }

    /// Applies crossfade for USAC DASH IPF config change
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time data (block based data)
    /// - `num_channels`: Number of channels
    /// - `frame_size`: Size of frame
    fn apply_crossfade(&self, time_data: &mut [f32], num_channels: usize, frame_size: usize) {
        for (channel_data, time_data_flush_ch) in izip!(
            time_data.chunks_exact_mut(frame_size),
            self.time_data_flush.iter()
        )
        .take(num_channels)
        {
            for (i, (value, time_flush_val)) in
                izip!(channel_data.iter_mut(), time_data_flush_ch.iter())
                    .enumerate()
                    .take(IPFDATA_TIME_DATA_FLUSH_SIZE)
            {
                let alpha = i as f32 * 0.0078125;
                *value = time_flush_val - (time_flush_val * alpha) + (*value * alpha);
            }
        }
    }
}

#[cfg(test)]
mod tests {

    use super::*;
    use crate::common::bitstream::Bitstream;
    use crate::common::bitstream::Mode::Reader;

    fn check_defaults(ipf_data: &IpfData) {
        assert!(!ipf_data.is_flush_on);
        assert!(!ipf_data.is_build_up_on);
        assert!(!ipf_data.has_audio_pre_roll);
        assert_eq!(
            ipf_data.preroll_au_length.len(),
            IPFDATA_MAX_NUM_PREROLL_AU + 1
        );
        assert_eq!(ipf_data.preroll_au_offset.len(), IPFDATA_MAX_NUM_PREROLL_AU);
        assert_eq!(ipf_data.au_start_anchor, 0);
        assert_eq!(ipf_data.access_unit, 0);
        assert_eq!(ipf_data.num_preroll_au, 0);
        assert!(!ipf_data.is_apply_crossfade);
        assert_eq!(ipf_data.time_data_flush.len(), IPFDATA_FLUSH_CHANNELS);
        for time_data_flush in ipf_data.time_data_flush {
            assert_eq!(time_data_flush.len(), IPFDATA_TIME_DATA_FLUSH_SIZE);
        }
    }

    #[test]
    fn new() {
        let ipf_data: IpfData = Default::default();
        check_defaults(&ipf_data);
    }

    #[test]
    fn new_modify_reset() {
        let mut ipf_data: IpfData = Default::default();
        check_defaults(&ipf_data);
        ipf_data.access_unit = 4;
        ipf_data.is_apply_crossfade = true;
        ipf_data.preroll_au_offset[0] = 99;
        ipf_data.reset(false);
        check_defaults(&ipf_data);
    }

    #[test]
    fn init() {
        let mut tp_dec = TransportDec::default();
        let ref_bytes = [
            65, 68, 73, 70, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 128, 0, 5, 251, 0, 160, 128, 0, 4,
            0, 0, 33, 16, 5, 0, 160, 27, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        let valid_bits = ref_bytes.len() * 8;
        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let nbits = bs.valid_bits();
        tp_dec.bs = bs;
        assert_eq!(nbits, valid_bits as isize);

        let mut ipf_data: IpfData = Default::default();
        let is_startup_phase: bool = true;
        assert_eq!(
            AacDecoderError::Ok,
            ipf_data.preroll_extension_payload(&mut tp_dec, is_startup_phase)
        );
        assert!(ipf_data.is_end_access_unit());
    }

    #[test]
    fn process() {
        const NUM_CHANNELS: usize = 2;
        const FRAME_SIZE: usize = 1024;

        let mut tp_dec = TransportDec::default();
        let ref_bytes = [
            65, 68, 73, 70, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 128, 0, 5, 251, 0, 160, 128, 0, 4,
            0, 0, 33, 16, 5, 0, 160, 27, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        let valid_bits = ref_bytes.len() * 8;
        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let nbits = bs.valid_bits();
        tp_dec.bs = bs;
        assert_eq!(nbits, valid_bits as isize);

        let mut ipf_data: IpfData = Default::default();
        let is_startup_phase: bool = true;
        assert_eq!(
            AacDecoderError::Ok,
            ipf_data.preroll_extension_payload(&mut tp_dec, is_startup_phase)
        );

        let is_frame_ok: bool = true;
        let mut time_data = [0.0_f32; NUM_CHANNELS * FRAME_SIZE];
        ipf_data.process(&mut time_data, NUM_CHANNELS, FRAME_SIZE, is_frame_ok);
        assert!(ipf_data.is_end_access_unit());
    }

    #[test]
    fn test_prepare_crossfade() {
        const NUM_CHANNELS: usize = 2;
        const FRAME_SIZE: usize = 1024;

        let mut ipf_data: IpfData = Default::default();

        let mut time_data = [0.0_f32; 2048];
        for (index, value) in time_data.iter_mut().enumerate().take(2048) {
            *value = (index + 1) as f32;
        }

        ipf_data.prepare_crossfade(&time_data, NUM_CHANNELS, FRAME_SIZE);

        for j in 0..NUM_CHANNELS {
            for i in 0..IPFDATA_TIME_DATA_FLUSH_SIZE {
                assert_eq!(
                    ipf_data.time_data_flush[j][i],
                    time_data[j * FRAME_SIZE + i]
                );
            }
        }
    }

    #[test]
    fn test_apply_crossfade() {
        const NUM_CHANNELS: usize = 2;
        const FRAME_SIZE: usize = 1024;

        let total_samples = NUM_CHANNELS * FRAME_SIZE;
        let preparation_time_data: [f32; 38] = [
            -4425.6953, -4300.906, -4257.0376, -4298.7256, -4381.433, -4439.4404, -4424.2324,
            -4330.1563, -4192.6978, -4062.175, -3970.851, -3914.9216, -3862.3499, -3780.2324,
            -3662.4202, -3538.0437, -3454.3474, -3444.5115, -3501.61, -3567.919, -3637.712,
            -3750.8765, -3880.7686, -3991.784, -4057.3574, -4071.4417, -4047.0378, -4003.5432,
            -3951.7336, -3886.5085, -3792.3347, -3657.8545, -3489.697, -3315.4302, -3172.015,
            -3085.3552, -3052.792, -3039.7222,
        ];
        let application_time_data: [f32; 38] = [
            -4249.715, -4215.9844, -4421.494, -4546.3765, -4512.7593, -4468.7153, -4476.213,
            -4437.8877, -4294.841, -4146.0503, -4110.3955, -4140.176, -4087.4487, -3930.111,
            -3774.1765, -3649.339, -3511.6448, -3424.33, -3471.114, -3891.345, -3769.3396,
            -3906.5972, -4215.5996, -4320.4185, -4174.3193, -4095.086, -4128.3687, -3990.1875,
            -3698.334, -3591.3882, -3679.117, -3636.1155, -3427.6914, -3313.7485, -3315.526,
            -3254.664, -3168.0347, -3176.5815,
        ];
        let expected_output_time_data: [f32; 38] = [
            -4425.6953, -4300.2427, -4259.6074, -4304.53, -4385.537, -4440.584, -4426.669,
            -4336.048, -4199.0815, -4068.0728, -3981.753, -3934.2795, -3883.453, -3795.4543,
            -3674.6436, -3551.0862, -3461.5095, -3441.831, -3497.3218, -3567.919, -3638.7402,
            -3753.3096, -3888.6162, -4002.054, -4061.9263, -4072.5498, -4051.4856, -4002.7085,
            -3933.9165, -3863.4521, -3782.605, -3655.8164, -3483.3997, -3315.246, -3188.8325,
            -3106.5188, -3068.0977, -3058.968,
        ];

        let time_data_per_channel = preparation_time_data.len() / NUM_CHANNELS;
        let mut time_data_for_preparation: Vec<f32> = vec![0.0f32; total_samples];
        for (index, value) in preparation_time_data.iter().enumerate() {
            let populated_channel_offset = index / time_data_per_channel * FRAME_SIZE;
            let sample_number = index - (index / time_data_per_channel) * time_data_per_channel;
            time_data_for_preparation[populated_channel_offset + sample_number] = *value;
        }

        let mut time_data_for_input: Vec<f32> = vec![0.0f32; total_samples];
        for (index, value) in application_time_data.iter().enumerate() {
            let populated_channel_offset = index / time_data_per_channel * FRAME_SIZE;
            let sample_number = index - (index / time_data_per_channel) * time_data_per_channel;
            time_data_for_input[populated_channel_offset + sample_number] = *value;
        }

        let mut ipf_data: IpfData = Default::default();

        ipf_data.prepare_crossfade(&time_data_for_preparation, NUM_CHANNELS, FRAME_SIZE);

        for j in 0..NUM_CHANNELS {
            for i in 0..IPFDATA_TIME_DATA_FLUSH_SIZE {
                assert_eq!(
                    ipf_data.time_data_flush[j][i],
                    time_data_for_preparation[j * FRAME_SIZE + i]
                );
            }
        }

        ipf_data.apply_crossfade(&mut time_data_for_input, NUM_CHANNELS, FRAME_SIZE);

        for j in 0..NUM_CHANNELS {
            for i in 0..IPFDATA_TIME_DATA_FLUSH_SIZE {
                if i < application_time_data.len() / 2 {
                    assert_eq!(
                        time_data_for_input[j * FRAME_SIZE + i],
                        expected_output_time_data[j * time_data_per_channel + i]
                    );
                }
            }
        }
    }
}
