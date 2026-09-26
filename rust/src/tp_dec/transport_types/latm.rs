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
//! AAC Low Overhead Audio Transport Multiplex (LATM)

use itertools::izip;

use super::super::{
    asc::AudioSpecificConfig,
    callbacks::TpDecCallBacks,
    constants::{LATM_MAX_LAYER, LATM_MAX_PROG, MIN_LATM_HEADERLENGTH},
    error_codes::TpDecoderError,
};
use crate::common::aot::AudioObjectType;
use crate::common::flags;
use crate::common::transport_type::TransportType;
use crate::common::{bitstream::Bitstream, flags::ACFlags};
use crate::{common::crc::CrcInfo, tp_dec::ReconfigState};

flags::bitflags! {
    /// LATM flags
    #[derive(Debug, Default, Copy, Clone, PartialEq)]
    pub struct LatmFlags: u32 {
        /// Indicates if buffer is full.
        const BUFFER_FULLNESS_ACHIEVED = 0x00000001;
        /// Indicates if USAC config has changed.
        const USAC_EXPLICIT_CONFIG_CHANGED = 0x00000002;
        /// Indicates if audio data is preloaded before actual playback.
        const USAC_HAS_PREROLL = 0x00000004;
    }
}

/// Enable CRC for formats which support that. Additionally enables LATM header
/// CRC evaluation. Since the interpretation of the standard is not 100% clear,
/// it is disabled  by default.
const TPDEC_LATM_HEADER_CRC_ENABLE: bool = false;

#[repr(C)]
#[derive(Debug, Default, Clone, Copy)]
/// Layer information structure.
struct LayerInfo {
    /// Indicates, how the length of the audio frame is specified and managed within the
    /// bitstream.
    frame_length_type: u32,
    /// Indicates the state of the buffer, what helps with management of the resources.
    buffer_fullness: u32,
    /// ID of an individual stream.
    stream_id: u32,
    /// Length of a frame in bits.
    frame_length_in_bits: u32,
}

#[repr(C)]
#[derive(Debug, Default, Clone)]
/// Low Overhead Audio Transport Multiplex structure.
pub(in super::super) struct LatmDemux {
    /// Array to hold the information for each layer, such as layer length and other layer-specific
    /// data.
    layer_info: [[LayerInfo; LATM_MAX_PROG as usize]; LATM_MAX_LAYER as usize],
    /// Configuration parameter: Indicates buffer fullness.
    tara_buffer_fullness: u32,
    /// Length of additional data.
    other_data_length: u32,
    /// Length of the audio mux in bytes.
    audio_mux_length_bytes: u32,

    // Configuration parameters.
    /// Indicates if the same stream should be used.
    use_same_stream_mux: bool,
    /// Indicates the version of the audio Mux.
    audio_mux_version: u8,
    /// Additional versioning information.
    audio_mux_version_a: u8,
    all_streams_same_time_framing: bool,

    // State variables.
    /// Number of subframes in the audio stream.
    num_sub_frames: u8,
    /// Number of programs in the audio stream.
    num_program: u8,
    /// Number of layers in the audio stream.
    num_layer: [u8; LATM_MAX_PROG as usize],

    /// Indicates if additional data is present.
    other_data_present: bool,
    /// Indicates if CRC is present.
    crc_check_present: bool,
    /// LATM flags.
    flags: LatmFlags,
    /// Method used to transport audio data.
    transport_type: TransportType,
}

impl LatmDemux {
    /// Creates a new instance of `LatmDemux`.
    ///
    /// # Return
    ///
    /// - `LatmDemux`.
    pub fn new() -> Self {
        LatmDemux::default()
    }

    /// Initialises `LatmDemux` with transport type passed as an argument.
    ///
    /// # Parameters
    /// - `transport_type`: Bitstream format used to transport audio data.
    pub(in super::super) fn init(&mut self, transport_type: TransportType) {
        self.reset();
        self.transport_type = transport_type;
    }

    /// Resets `LatmDemux`.
    fn reset(&mut self) {
        for prog in &mut self.layer_info {
            for layer in prog {
                layer.frame_length_type = 0;
                layer.buffer_fullness = 0;
                layer.stream_id = 0;
                layer.frame_length_in_bits = 0;
            }
        }
        self.tara_buffer_fullness = 0;
        self.other_data_length = 0;
        self.audio_mux_length_bytes = 0;
        self.use_same_stream_mux = false;
        self.audio_mux_version = 0;
        self.audio_mux_version_a = 0;
        self.all_streams_same_time_framing = false;
        self.num_sub_frames = 0;
        self.num_program = 0;
        self.num_layer[..LATM_MAX_PROG as usize].fill(0);
        self.other_data_present = false;
        self.crc_check_present = false;
        self.flags = LatmFlags::empty();
        self.transport_type = TransportType::Mp4Raw;
    }

    /// Reads audio data encoded with LATM.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    /// - `asc`: Audio Specific Config.
    /// - `config_found`: Indicates if config has been found.
    /// - `ignore_buffer_fullness`: Flag to indicate buffer fullness to be ignored.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(in super::super) fn read(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
        asc: &mut AudioSpecificConfig,
        is_config_found: &mut bool,
        ignore_buffer_fullness: bool,
    ) -> Result<(), TpDecoderError> {
        let cnt_bits = bs.valid_bits();
        const AUDIO_MUX_LENGTH_BYTE_LAST: u32 = 0;

        if cnt_bits < MIN_LATM_HEADERLENGTH {
            return Err(TpDecoderError::NotEnoughBits);
        }

        let error_status = self.read_audio_mux_element(bs, cb, asc, is_config_found);
        error_status?;

        if !ignore_buffer_fullness {
            let cmp_buffer_fullness = 24
                + AUDIO_MUX_LENGTH_BYTE_LAST * 8
                + self.layer_info[0][0].buffer_fullness * (asc.channel_config() as u32) * 32;

            // Evaluate buffer fullness.
            if (self.layer_info[0][0].buffer_fullness != 0xFF)
                && !self.flags.contains(LatmFlags::BUFFER_FULLNESS_ACHIEVED)
            {
                if cnt_bits < cmp_buffer_fullness as isize {
                    // Condition for start of decoding is not fulfilled.
                    // Current frame will not be decoded.
                    return Err(TpDecoderError::NotEnoughBits);
                } else {
                    self.flags.insert(LatmFlags::BUFFER_FULLNESS_ACHIEVED);
                }
            }
        }

        Ok(())
    }

    /// Parses configuration information about how audio streams are multiplexed.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    /// - `asc`: Audio Specific Config.
    /// - `config_found`: Indicates if config has been found.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(in super::super) fn read_stream_mux_config(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
        asc: &mut AudioSpecificConfig,
        is_config_found: &mut bool,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        self.set_config_changed();

        let mut crc_info = CrcInfo::new();
        let mut crc_region = 0;

        if TPDEC_LATM_HEADER_CRC_ENABLE {
            crc_info.init(0x07, 0x0, 8);
            crc_region = crc_info.start_region(bs, 0);
        }

        self.audio_mux_version = bs.read_bit() as u8;

        if self.audio_mux_version == 0 {
            self.audio_mux_version_a = 0;
        } else {
            self.audio_mux_version_a = bs.read_bit() as u8;
        }

        if self.audio_mux_version_a == 0 {
            if self.audio_mux_version == 1 {
                self.tara_buffer_fullness = get_value(bs) as u32;
            }
            self.all_streams_same_time_framing = bs.read_bit() != 0;
            self.num_sub_frames = (bs.read(6) + 1) as u8;
            self.num_program = (bs.read(4) + 1) as u8;

            if self.num_program > LATM_MAX_PROG {
                self.init(self.transport_type);
                return Err(TpDecoderError::UnsupportedFormat);
            }

            let mut id_cnt = 0;
            for prog in 0..usize::from(self.num_program) {
                self.num_layer[prog] = (bs.read(3) + 1) as u8;
                if self.num_layer[prog] > LATM_MAX_LAYER {
                    self.init(self.transport_type);
                    return Err(TpDecoderError::UnsupportedFormat);
                }

                for lay in 0..usize::from(self.num_layer[prog]) {
                    let mut use_same_config = false;
                    self.layer_info[prog][lay].stream_id = id_cnt;
                    id_cnt += 1;
                    self.layer_info[prog][lay].frame_length_in_bits = 0;

                    if !((prog == 0) && (lay == 0)) {
                        use_same_config = bs.read_bit() != 0;
                    }

                    if use_same_config {
                        if lay == 0 {
                            self.init(self.transport_type);
                            return Err(TpDecoderError::ParseError);
                        }
                    } else {
                        // Bit position after ASC with explicit length.
                        let mut bs_after_asc = 0;
                        let mut dummy_asc = AudioSpecificConfig::new();
                        dummy_asc.init();

                        if self.audio_mux_version == 1 {
                            let asc_length = get_value(bs);
                            if asc_length > bs.valid_bits() || asc_length < 0 {
                                self.init(self.transport_type);
                                return Err(TpDecoderError::ParseError);
                            }
                            bs_after_asc = bs.valid_bits() - asc_length;
                        }

                        // Read ASC.
                        error_status = dummy_asc.parse(
                            bs,
                            self.audio_mux_version == 1,
                            cb,
                            AudioObjectType::AotNullObject,
                        );
                        if error_status.is_err() {
                            self.init(self.transport_type);
                            return error_status;
                        }
                        if self.audio_mux_version == 1 {
                            if bs.valid_bits() < bs_after_asc {
                                self.init(self.transport_type);
                                return Err(TpDecoderError::ParseError);
                            }
                            let num_bits = bs.valid_bits() - bs_after_asc;
                            bs.push(num_bits);
                        }

                        // Configure source decoder.
                        error_status = cb.update_config_callback(&dummy_asc);

                        if error_status.is_err() {
                            if error_status != Err(TpDecoderError::NeedToRestart) {
                                error_status = Err(TpDecoderError::SyncError);
                            }
                            *is_config_found = false;
                            self.init(self.transport_type);
                            return error_status;
                        }
                        *is_config_found = true;

                        //Store ASC.
                        if (prog == 0) && (lay == 0) {
                            *asc = dummy_asc;
                        }
                    }

                    self.layer_info[prog][lay].frame_length_type = bs.read(3);

                    let frame_length = self.layer_info[prog][lay].frame_length_type;
                    match frame_length {
                        0 => self.layer_info[prog][lay].buffer_fullness = bs.read(8),
                        1 => self.layer_info[prog][lay].frame_length_in_bits = bs.read(9),
                        _ => {
                            self.init(self.transport_type);
                            return Err(TpDecoderError::ParseError);
                        }
                    };
                } // layer loop
            } // prog loop

            self.other_data_present = bs.read_bit() != 0;
            self.other_data_length = 0;

            if self.other_data_present {
                if self.audio_mux_version == 1 {
                    self.other_data_length = get_value(bs) as u32;
                } else {
                    loop {
                        // *=256
                        self.other_data_length <<= 8;
                        let other_data_len_esc = bs.read_bit() != 0;
                        self.other_data_length += bs.read(8);
                        if !other_data_len_esc {
                            break;
                        }
                    }
                }
                if self.audio_mux_length_bytes < (self.other_data_length >> 3) {
                    self.init(self.transport_type);
                    return Err(TpDecoderError::ParseError);
                }
            }
            if TPDEC_LATM_HEADER_CRC_ENABLE {
                crc_info.end_region(bs, crc_region);
            }
            self.crc_check_present = bs.read_bit() != 0;

            if self.crc_check_present {
                if TPDEC_LATM_HEADER_CRC_ENABLE {
                    if (crc_info.value() ^ 0xFF) != bs.read(8) {
                        error_status = Err(TpDecoderError::CrcError);
                    }
                } else {
                    bs.read(8);
                }
            }
        } else {
            // audio_mux_version_a > 0 is reserved for future extensions.
            error_status = Err(TpDecoderError::UnsupportedFormat);
        }

        if bs.valid_bits() < 0 {
            error_status = Err(TpDecoderError::NotEnoughBits);
        }

        if error_status.is_err() {
            self.init(self.transport_type);
        }

        error_status
    }

    /// Parses length information of the audio payload.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(in super::super) fn read_payload_length_info(
        &mut self,
        bs: &mut Bitstream,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        let mut total_payload_bits = 0;

        if self.all_streams_same_time_framing {
            for prog in &mut self.layer_info {
                for lay in prog {
                    match lay.frame_length_type {
                        0 => {
                            let au_chunk_length_info = read_au_chunk_length_info(bs);
                            if au_chunk_length_info >= 0 {
                                lay.frame_length_in_bits = au_chunk_length_info as u32;
                                total_payload_bits += lay.frame_length_in_bits;
                            }
                        }
                        1 => (),
                        _ => return Err(TpDecoderError::ParseError),
                    }
                }
            }
        } else {
            error_status = Err(TpDecoderError::ParseError);
        }

        if (self.audio_mux_length_bytes > 0)
            && (total_payload_bits > (self.audio_mux_length_bytes * 8))
        {
            error_status = Err(TpDecoderError::ParseError);
        }
        if bs.valid_bits() < 0 {
            error_status = Err(TpDecoderError::NotEnoughBits);
        }

        error_status
    }

    /// Returns frame length in bits.
    pub(in super::super) fn get_frame_length_in_bits(&mut self) -> u32 {
        let mut frame_len_bits = 0;
        for (linfo_prog, layer) in
            izip!(self.layer_info.iter(), self.num_layer.iter()).take(usize::from(self.num_program))
        {
            for linfo in linfo_prog.iter().take(usize::from(*layer)) {
                frame_len_bits += linfo.frame_length_in_bits;
            }
        }

        frame_len_bits
    }

    /// Returns number of subframes.
    pub(in super::super) fn get_num_of_subframes(&mut self) -> u8 {
        self.num_sub_frames
    }

    /// Ensures, that the end of the current access unit (AU) aligns correctly
    /// within the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `is_last_raw_data_block`: Flag to indicate last raw data block.
    /// - `global_frame_pos`: Global frame position.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(in super::super) fn adjust_end_of_access_unit(
        &mut self,
        bs: &mut Bitstream,
        is_last_raw_data_block: bool,
        global_frame_pos: isize,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        if is_last_raw_data_block {
            // Read other data if available.
            if self.other_data_present {
                if bs.valid_bits() >= self.other_data_length as isize {
                    bs.push(self.other_data_length as isize);
                } else {
                    error_status = Err(TpDecoderError::ParseError);
                }
            }
        }

        if self.transport_type == TransportType::Mp4Loas {
            let loas_offset =
                bs.valid_bits() - (global_frame_pos) + (self.audio_mux_length_bytes * 8) as isize;

            // Check read bits after every raw data block to be able to wind back
            // in the bitstream buffer in case that too many bits have been read.
            // Doing the check only after the last raw data block is not suitable
            // because the bitstream buffer might be overwritten in the meantime.
            // For ELD and other payloads there is an unknown amount of padding,
            // so ignore unread bits, but throw an error only if too many bits
            // where read.
            if loas_offset < 0 {
                error_status = Err(TpDecoderError::ParseError);
            }

            if (is_last_raw_data_block || (error_status == Err(TpDecoderError::ParseError)))
                && self.audio_mux_length_bytes > 0
            {
                bs.push(loas_offset);
            }
        }

        // If bit buffer has not more bits then too many bits were read and
        // obviously no more RawDataBlocks can be read.
        if bs.valid_bits() < 0 {
            error_status = Ok(());
        }

        if is_last_raw_data_block || (error_status.is_err()) {
            // Do byte align at the end of AudioMuxElement.
            bs.align(global_frame_pos);
        }

        error_status
    }

    /// Parses and extracts audio multiplex elements from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    /// - `asc`: Audio Specific Config.
    /// - `config_found`: Indicates if config has been found.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    fn read_audio_mux_element(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
        asc: &mut AudioSpecificConfig,
        is_config_found: &mut bool,
    ) -> Result<(), TpDecoderError> {
        if self.transport_type != TransportType::Mp4LatmMcp0 {
            self.use_same_stream_mux = bs.read_bit() != 0;

            if !self.use_same_stream_mux {
                let bs_anchor = bs.valid_bits();
                let mut dummy_asc = AudioSpecificConfig::new();
                dummy_asc.init();
                cb.set_config_mode(ReconfigState::DetCfgChange);

                for i in 0..2 {
                    let num_bits = bs_anchor - bs.valid_bits();
                    bs.push(-num_bits);

                    let error_status =
                        self.read_stream_mux_config(bs, cb, &mut dummy_asc, is_config_found);
                    error_status?;

                    // Skip the decoder reconfiguration step when audioPreroll functionality
                    // is enabled in current and new config.
                    if self.flags.contains(LatmFlags::USAC_HAS_PREROLL)
                        && dummy_asc.ac_flags().contains(ACFlags::USAC_HAS_PREROLL)
                    {
                        break;
                    }

                    if (i == 0) && cb.is_config_changed() && cb.free_mem_callback().is_err() {
                        return Err(TpDecoderError::ParseError);
                    }
                    cb.set_config_mode(ReconfigState::AllocMem);
                }

                if self.flags.contains(LatmFlags::USAC_HAS_PREROLL) {
                    let dummy_asc_config_length = dummy_asc.usac_config().config_length();
                    let asc_usac_config = asc.usac_config_mut();
                    let asc_config = asc_usac_config.config_buffer();
                    let dummy_asc_usac_config = dummy_asc.usac_config();
                    let dummy_asc_config = dummy_asc_usac_config.config_buffer();
                    if (asc_usac_config.config_length() != dummy_asc_config_length)
                        || (asc_config[..usize::from(dummy_asc_config_length)]
                            != dummy_asc_config[..usize::from(dummy_asc_config_length)])
                    {
                        // Store new USAC config.
                        asc_usac_config.set_config_length(dummy_asc_config_length);
                        asc_usac_config
                            .config_buffer_mut()
                            .copy_from_slice(dummy_asc_config);
                        // USAC config has changed.
                        self.flags.insert(LatmFlags::USAC_EXPLICIT_CONFIG_CHANGED);
                    }
                } else {
                    // Store new ASC.
                    *asc = dummy_asc;
                    self.flags.remove(
                        LatmFlags::USAC_HAS_PREROLL | LatmFlags::USAC_EXPLICIT_CONFIG_CHANGED,
                    );
                    self.flags
                        .insert(if asc.ac_flags().contains(ACFlags::USAC_HAS_PREROLL) {
                            LatmFlags::USAC_HAS_PREROLL
                        } else {
                            LatmFlags::empty()
                        });
                }
            }
        }

        // If there was no configuration read, its not possible to parse
        // payload_length_info below.
        if !(*is_config_found) {
            return Err(TpDecoderError::SyncError);
        }

        if self.audio_mux_version_a == 0 {
            let status = self.read_payload_length_info(bs);
            // Do only once per call, because parsing and decoding is done in-line.
            if status.is_err() {
                *is_config_found = false;
                return status;
            }
        } else {
            // audioMuxVersionA > 0 is reserved for future extensions.
            *is_config_found = false;
            return Err(TpDecoderError::UnsupportedFormat);
        }

        Ok(())
    }

    /// Updates`LatmDemux` flags on USAC configuration change.
    pub(in super::super) fn set_config_changed(&mut self) {
        self.flags.remove(LatmFlags::USAC_EXPLICIT_CONFIG_CHANGED);
    }

    /// Returns `LatmDemux` flags.
    pub(in super::super) fn flags(&self) -> LatmFlags {
        self.flags
    }

    /// Removes given flags from `LatmDemux` flags.
    pub(in super::super) fn remove_flags(&mut self, flags: LatmFlags) {
        self.flags.remove(flags);
    }

    /// Returns audio multiplex length in bytes.
    pub(in super::super) fn audio_mux_length_bytes(&self) -> u32 {
        self.audio_mux_length_bytes
    }

    /// Sets audio length in bytes.
    pub(in super::super) fn set_audio_mux_length_bytes(&mut self, audio_mux_length_bytes: u32) {
        self.audio_mux_length_bytes = audio_mux_length_bytes;
    }
}

/// Reads value from the bitstream.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data.
///
/// # Return
///
/// - `isize`.
fn get_value(bs: &mut Bitstream) -> isize {
    let bytes_for_value = bs.read(2);
    let mut value = 0;

    for _i in 0..(bytes_for_value + 1) {
        let tmp = bs.read(8);
        value <<= 8;
        value += tmp;
    }

    value as isize
}

/// Reads audio unit chunk length.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data.
///
/// # Return
///
/// - `i32`.
fn read_au_chunk_length_info(bs: &mut Bitstream) -> i32 {
    let mut len = 0;
    let mut tmp = 255;
    let mut valid_bytes = bs.valid_bits() >> 3;

    while (tmp == 255) && (valid_bytes > 0) {
        tmp = bs.read(8);
        len += tmp;
        valid_bytes -= 1;
    }

    if tmp == 255 {
        -1
    } else {
        (len << 3) as i32
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new() {
        let latm = LatmDemux::new();
        assert!(latm.transport_type == TransportType::Unknown);
        assert!(latm.flags == LatmFlags::empty());
        assert!(!latm.crc_check_present);
        assert!(!latm.other_data_present);
        assert!(latm.num_layer.len() == 1);
        assert!(latm.num_program == 0);
        assert!(latm.num_sub_frames == 0);
        assert!(!latm.all_streams_same_time_framing);
        assert!(latm.audio_mux_version_a == 0);
        assert!(latm.audio_mux_version == 0);
        assert!(!latm.use_same_stream_mux);
        assert!(latm.audio_mux_length_bytes == 0);
        assert!(latm.other_data_length == 0);
        assert!(latm.tara_buffer_fullness == 0);
        assert!(latm.layer_info.len() == 1);
        assert!(latm.layer_info[0].len() == 1);
    }

    #[test]
    fn init() {
        let mut latm = LatmDemux::new();
        latm.init(TransportType::Mp4Adif);
        assert!(latm.transport_type == TransportType::Mp4Adif);
    }
}
