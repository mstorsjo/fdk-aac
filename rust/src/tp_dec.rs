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
//! MPEG transport format decoder

// Modules
pub mod asc;
pub mod callbacks;
pub mod constants;
pub mod error_codes;
pub mod info;
pub mod pce;
pub mod syncer;
mod tables;
mod transport_types;

// Re-exports
pub use {
    asc::{AudioSpecificConfig, UsacConfig, UsacElementConfig, UsacExtElementConfig},
    callbacks::TpDecCallBacks,
    constants::{
        MAX_CONF_SIZE, TP_USAC_MAX_CONFIG_LEN, TP_USAC_MAX_ELEMENTS, TRANSPORTDEC_INBUF_SIZE,
    },
    error_codes::TpDecoderError,
    pce::{PceCompareResult, ProgramConfig},
};

// Imports
use crate::{
    common::{
        aot::AudioObjectType,
        bitstream::{Bitstream, Mode},
        flags::ACFlags,
        samplerate_index::SAMPLING_RATE_TABLE,
        transport_type::TransportType,
    },
    tp_dec::transport_types::latm::LatmFlags,
};
use transport_types::{Adif, Adts, LatmDemux};
use {constants::ADTS_SYNCLENGTH, info::TpDecInfo, syncer::Syncer};

// Enums
/// Reconfiguration states
#[repr(u8)]
#[derive(Debug, PartialEq, Copy, Clone)]
pub enum ReconfigState {
    /// No config mode set at all.
    None = 0x00,
    /// Config mode signalizes the callback to work in config change detection mode.
    DetCfgChange = 0x01,
    /// Config mode signalizes the callback to work in memory allocation mode.
    AllocMem = 0x02,
}

impl From<ReconfigState> for u8 {
    fn from(state: ReconfigState) -> Self {
        match state {
            ReconfigState::None => 0x00,
            ReconfigState::DetCfgChange => 0x01,
            ReconfigState::AllocMem => 0x02,
        }
    }
}

impl TryFrom<u8> for ReconfigState {
    type Error = &'static str;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0x00 => Ok(ReconfigState::None),
            0x01 => Ok(ReconfigState::DetCfgChange),
            0x02 => Ok(ReconfigState::AllocMem),
            _ => Err("invalid u8 value to convert to ReconfigState."),
        }
    }
}

impl ReconfigState {
    fn all() -> [ReconfigState; 2] {
        [ReconfigState::DetCfgChange, ReconfigState::AllocMem]
    }
}

#[repr(C)]
#[derive(PartialEq, Default, Debug)]
/// Configuration Change Status.
enum CfgChangeStatus {
    #[default]
    /// Not specified.
    Undefined,
    /// System should clear buffered data.
    FlushOn,
    /// Build data up in the buffer until certain condition is met.
    BuildUp,
}

#[repr(C)]
#[derive(PartialEq)]
/// Transport Decoder parameter.
pub enum TpDecParam {
    /// Ignore buffer fullness.
    IgnoreBufferFullness,
    /// Reset data.
    Reset,
    /// Check for two valid sync words
    CheckTwoSyncs,
}

// Structs
#[derive(Default, Debug)]
#[repr(C)]
/// Transport Decoder Data structure.
pub struct TpDecData {
    /// MPEG4 transport decoder type.
    transport_type: TransportType,
    /// Audio Data Transport Stream (ADTS).
    adts: Option<Box<Adts>>,
    /// Audio Data Interchange Format (ADIF).
    adif: Option<Box<Adif>>,
    /// Low Overhead Audio Transport Multiplex (LATM).
    latm: Option<Box<LatmDemux>>,
    /// Audio specific config from the last config found.
    asc: AudioSpecificConfig,
    /// Transport decoder frame info.
    info: TpDecInfo,
    /// Global transport frame reference bit position.
    global_frame_position: i32,
    /// Current number of raw data blocks contained, remaining from the current transport frame.
    num_raw_data_blocks: u8,
    /// Indicates valid config and successful decoder initialisation.
    is_config_found: bool,
}

impl TpDecData {
    /// Returns new instance of `TpDecData`.
    ///
    /// # Parameters
    ///
    /// - `transport_type`: TransportType` format.
    pub fn new(transport_type: TransportType) -> TpDecData {
        let mut tpdec_data = TpDecData::default();

        match transport_type {
            TransportType::Unknown | TransportType::Mp4Raw => (),
            TransportType::Mp4Adif => tpdec_data.adif = Some(Box::default()),
            TransportType::Mp4Adts => {
                let adts = Adts::new();
                tpdec_data.adts = Some(Box::new(adts));
            }
            TransportType::Mp4LatmMcp1 | TransportType::Mp4LatmMcp0 | TransportType::Mp4Loas => {
                let latm = LatmDemux::new();
                tpdec_data.latm = Some(Box::new(latm));
            }
        };

        tpdec_data
    }

    /// Initialises `TpDecData`.
    ///
    /// # Parameters
    ///
    /// - `transport_type`: TransportType` format.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(super) fn init(&mut self, transport_type: TransportType) -> Result<(), TpDecoderError> {
        match transport_type {
            TransportType::Unknown => {
                self.deinit();
                return Err(TpDecoderError::UnknownError);
            }
            TransportType::Mp4Raw => (),
            TransportType::Mp4Adif => (),
            TransportType::Mp4Adts => {
                if let Some(adts) = self.adts.as_deref_mut() {
                    adts.init();
                }
            }
            TransportType::Mp4LatmMcp1 | TransportType::Mp4LatmMcp0 | TransportType::Mp4Loas => {
                if let Some(latm) = self.latm.as_deref_mut() {
                    latm.init(transport_type);
                }
            }
        };

        self.transport_type = transport_type;
        self.info.init();
        self.asc.init();

        Ok(())
    }

    /// Deinitialises `TpDecData`.
    pub fn deinit(&mut self) {
        self.adts = None;
        self.adif = None;
        self.latm = None;
        self.transport_type = TransportType::Unknown;
        self.asc.reset();
        self.info.reset();
        self.global_frame_position = 0;
        self.num_raw_data_blocks = 0;
        self.is_config_found = false;
    }

    /// Reads metadata information about the stream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    /// - `is_ignore_buffer_fullness`: ignore buffer fullness if `true`.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    fn read_header(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
        is_ignore_buffer_fullness: bool,
    ) -> Result<(), TpDecoderError> {
        let mut err = Ok(());

        if bs.valid_bits() <= 0 {
            err = Err(TpDecoderError::NotEnoughBits);
        } else {
            match self.transport_type {
                TransportType::Unknown => err = Err(TpDecoderError::UnsupportedFormat),
                TransportType::Mp4Raw => {
                    if !self.is_config_found {
                        // Decoder needs to be configured with out of band config.
                        err = Err(TpDecoderError::UnknownError);
                    } else {
                        // One Access Unit was filled into buffer, so get the length out of the
                        // buffer.
                        self.info.set_au_length(bs.valid_bits().try_into().unwrap());
                    }
                }
                TransportType::Mp4Adif => {
                    // Read header if not already done.
                    if !self.is_config_found {
                        let bs_start = bs.valid_bits();

                        for config_mode in ReconfigState::all() {
                            if config_mode == ReconfigState::AllocMem {
                                let num_bits = bs_start - bs.valid_bits();
                                bs.push(-num_bits);
                            }
                            cb.set_config_mode(config_mode);

                            let mut dummy_asc = AudioSpecificConfig::new();
                            dummy_asc.init();

                            if let Some(adif) = self.adif.as_deref_mut() {
                                err = adif.read_decode_header(dummy_asc.pce_as_mut(), bs);
                                // err = adif.read_decode_header(dummy_asc.pce_as_mut(), bs);
                            }

                            if bs.valid_bits() < 0 {
                                err = Err(TpDecoderError::NotEnoughBits);
                            }

                            if err.is_err() {
                                break;
                            }

                            // Map adif header to ASC.
                            let sr_index = dummy_asc.pce().sampling_frequency_index();
                            dummy_asc.set_aot(AudioObjectType::AotAacLc);
                            dummy_asc.set_sampling_frequency_index(sr_index);
                            dummy_asc
                                .set_sampling_frequency(SAMPLING_RATE_TABLE[usize::from(sr_index)]);
                            dummy_asc.set_channel_config(0);

                            // Call callback to decoder.
                            if cb.update_config_callback(&dummy_asc).is_err() {
                                err = Err(TpDecoderError::ParseError);
                                break;
                            }
                            self.asc = dummy_asc;
                            self.is_config_found = true;

                            if config_mode == ReconfigState::DetCfgChange
                                && cb.is_config_changed()
                                && cb.free_mem_callback().is_err()
                            {
                                err = Err(TpDecoderError::ParseError);
                                break;
                            }
                        }
                    }
                    // Access Unit data length is unknown.
                    self.info.set_au_length(-1);
                }
                TransportType::Mp4Adts => {
                    if self.num_raw_data_blocks == 0 {
                        self.global_frame_position = bs.valid_bits() as i32;

                        for config_mode in ReconfigState::all() {
                            if config_mode == ReconfigState::AllocMem {
                                let num_bits =
                                    self.global_frame_position as isize - bs.valid_bits();
                                bs.push(-num_bits);
                            }
                            cb.set_config_mode(config_mode);

                            let mut dummy_asc = self.asc;

                            if let Some(adts) = self.adts.as_deref_mut() {
                                err = adts.decode_header(
                                    &mut dummy_asc,
                                    bs,
                                    is_ignore_buffer_fullness,
                                );
                                // Parse ADTS header.
                                // err = adts.decode_header(
                                //     &mut dummy_asc,
                                //     bs,
                                //     is_ignore_buffer_fullness,
                                // );
                                if err.is_err() {
                                    if err != Err(TpDecoderError::NotEnoughBits) {
                                        err = Err(TpDecoderError::SyncError);
                                    }
                                    break;
                                }

                                // Check if the whole frame would fit the bitstream buffer.
                                if adts.bs().frame_length() as usize > bs.buffer().len() {
                                    err = Err(TpDecoderError::SyncError);
                                    break;
                                }

                                // Validate explicit frame length and check bitbuffer fill level.
                                let frame_data_bits = (adts.bs().frame_length() << 3) as isize
                                    - (self.global_frame_position as isize - bs.valid_bits())
                                    - ADTS_SYNCLENGTH;

                                if err.is_ok() && frame_data_bits <= 0 {
                                    err = Err(TpDecoderError::SyncError);
                                }

                                if bs.valid_bits() < frame_data_bits {
                                    err = Err(TpDecoderError::NotEnoughBits);
                                }

                                if err.is_err() {
                                    break;
                                }

                                err = cb.update_config_callback(&dummy_asc);

                                if err.is_err() {
                                    if err != Err(TpDecoderError::NeedToRestart) {
                                        err = Err(TpDecoderError::SyncError);
                                    }
                                    break;
                                }

                                self.asc = dummy_asc;
                                self.is_config_found = true;
                                self.num_raw_data_blocks = adts.bs().num_raw_blocks() + 1;

                                if config_mode == ReconfigState::DetCfgChange
                                    && cb.is_config_changed()
                                    && cb.free_mem_callback().is_err()
                                {
                                    err = Err(TpDecoderError::ParseError);
                                    break;
                                }
                            }
                        }
                    } else {
                        // Reset CRC because the next bits are the beginning of a raw_data_block().
                        if let Some(adts) = self.adts.as_deref_mut() {
                            adts.reset();
                        }
                    }

                    if err.is_ok() {
                        self.num_raw_data_blocks -= 1;
                        if let Some(adts) = self.adts.as_deref_mut() {
                            self.info.set_au_length(adts.get_raw_data_block_length(
                                adts.bs().num_raw_blocks() - self.num_raw_data_blocks,
                            ))
                        }
                    }
                }
                TransportType::Mp4LatmMcp1
                | TransportType::Mp4LatmMcp0
                | TransportType::Mp4Loas => {
                    let mut is_break = false;

                    if self.transport_type == TransportType::Mp4Loas
                        && self.num_raw_data_blocks == 0
                    {
                        if let Some(latm) = self.latm.as_deref_mut() {
                            latm.set_audio_mux_length_bytes(bs.read(13));

                            // Check if the whole frame would fit the bitstream buffer.
                            if (latm.audio_mux_length_bytes() + 3) as usize > bs.buffer().len() {
                                err = Err(TpDecoderError::SyncError);
                                is_break = true;
                            }

                            // Check if the whole frame is in the bitstream buffer.
                            if bs.valid_bits() < (latm.audio_mux_length_bytes() << 3) as isize
                                && !is_break
                            {
                                err = Err(TpDecoderError::NotEnoughBits);
                                is_break = true;
                            }
                        }
                    }

                    if !is_break {
                        if self.num_raw_data_blocks == 0 {
                            let mut frame_data_bits = 0;
                            let mut config_found = self.is_config_found;
                            self.global_frame_position = bs.valid_bits() as i32;

                            if let Some(latm) = self.latm.as_deref_mut() {
                                err = latm.read(
                                    bs,
                                    cb,
                                    &mut self.asc,
                                    &mut config_found,
                                    is_ignore_buffer_fullness,
                                );

                                if err.is_err() && err != Err(TpDecoderError::NotEnoughBits) {
                                    err = Err(TpDecoderError::SyncError);
                                }

                                if err.is_ok() && self.transport_type == TransportType::Mp4Loas {
                                    // Validate explicit frame length and check bitbuffer fill
                                    // level.
                                    frame_data_bits = (latm.audio_mux_length_bytes() << 3) as i32
                                        - (self.global_frame_position - bs.valid_bits() as i32);

                                    if frame_data_bits <= 0 {
                                        err = Err(TpDecoderError::SyncError);
                                    }
                                }

                                let bits = if self.transport_type == TransportType::Mp4Loas {
                                    frame_data_bits as isize
                                } else {
                                    0
                                };

                                if bs.valid_bits() < bits {
                                    err = Err(TpDecoderError::NotEnoughBits);
                                }

                                if err.is_ok() {
                                    self.num_raw_data_blocks = latm.get_num_of_subframes();
                                    if config_found {
                                        self.is_config_found = true;
                                    }
                                }
                            }
                        } else if let Some(latm) = self.latm.as_deref_mut() {
                            err = latm.read_payload_length_info(bs);
                            if err.is_err() {
                                err = Err(TpDecoderError::SyncError);
                            }
                        }
                    }

                    if err.is_ok() {
                        if let Some(latm) = self.latm.as_deref_mut() {
                            self.info
                                .set_au_length(latm.get_frame_length_in_bits() as i32);
                            self.num_raw_data_blocks -= 1;
                        }
                    }
                }
            };
            if err.is_ok() {
                self.info.set_access_unit_anchor(bs.valid_bits());
                // In case of known access unit data length check whether bitsream contains a
                // sufficient numer of bits
                if (self.info.au_length() > 0)
                    && (bs.valid_bits() < self.info.au_length().try_into().unwrap())
                {
                    err = Err(TpDecoderError::NotEnoughBits);
                }
            }
        }

        if err.is_err() {
            self.info.reset();
        }

        err
    }

    /// Resets the `TpDecData`.
    fn reset(&mut self) {
        self.global_frame_position = 0;
        self.num_raw_data_blocks = 0;
        self.info.reset();
    }
}

#[repr(C)]
#[derive(Debug)]
/// Transport decoder structure.
pub struct TransportDec {
    /// Bitstream.
    pub bs: Bitstream,
    /// Transport decoder callbacks.
    cb: TpDecCallBacks,
    /// Control Configuration Change.
    status_config_change: CfgChangeStatus,
    /// Synchronizer.
    syncer: Syncer,
    /// Transport Decoder Data.
    pub data: TpDecData,
}

impl Default for TransportDec {
    /// Returns default instance of `TransportDec`.
    fn default() -> Self {
        Self {
            bs: Default::default(),
            cb: Default::default(),
            status_config_change: CfgChangeStatus::Undefined,
            syncer: Default::default(),
            data: Default::default(),
        }
    }
}

impl TransportDec {
    /// Returns new instance of `TransportDec`.
    ///
    /// # Parameters
    ///
    /// - `transport_type`: `TransportType` format.
    pub fn new(transport_type: TransportType) -> Self {
        Self {
            data: TpDecData::new(transport_type),
            ..Default::default()
        }
    }

    /// Initialises `TransportDec`.
    ///
    /// # Parameters
    ///
    /// - `transport_type`: `TransportType` format.
    pub fn init(&mut self, transport_type: TransportType) {
        self.bs = Bitstream::new(TRANSPORTDEC_INBUF_SIZE, Mode::Reader);
        let _ = self.data.init(transport_type);
        self.syncer.init(transport_type);
        self.status_config_change = CfgChangeStatus::Undefined;
    }

    /// Configures TransportDec via a binary coded AudioSpecificConfig or StreamMuxConfig.
    ///
    /// # Parameters
    ///
    /// - `conf`: u8 buffer of the binary coded config (ASC or SMC).
    ///
    /// # Return
    ///
    ///   - `TpDecoderError`.
    pub fn out_of_band_config(&mut self, conf: &[u8]) -> Result<(), TpDecoderError> {
        let mut err = Ok(());
        let mut config_found = false;

        if conf.len() > MAX_CONF_SIZE {
            return Err(TpDecoderError::UnsupportedFormat);
        }

        let mut bs = Bitstream::new(MAX_CONF_SIZE, Mode::Reader);
        bs.init(conf, 8 * conf.len());

        for config_mode in ReconfigState::all() {
            if config_mode == ReconfigState::AllocMem {
                let num_bits = (conf.len() * 8) as isize - bs.valid_bits();
                bs.push(-num_bits);
            }
            self.cb.set_config_mode(config_mode);

            // Config transport decoder.
            match self.data.transport_type {
                TransportType::Mp4LatmMcp1
                | TransportType::Mp4LatmMcp0
                | TransportType::Mp4Loas => {
                    err = if let Some(latm) = self.data.latm.as_deref_mut() {
                        latm.read_stream_mux_config(
                            &mut bs,
                            &mut self.cb,
                            &mut self.data.asc,
                            &mut config_found,
                        )
                    } else {
                        Err(TpDecoderError::UnknownError)
                    };
                }
                _ => {
                    let mut dummy_asc = AudioSpecificConfig::new();
                    dummy_asc.init();

                    err = dummy_asc.parse(
                        &mut bs,
                        true,
                        &mut self.cb,
                        AudioObjectType::AotNullObject,
                    );

                    if err.is_ok() {
                        if self.cb.update_config_callback(&dummy_asc).is_err() {
                            err = Err(TpDecoderError::ParseError);
                            break;
                        }
                        self.data.asc = dummy_asc;
                        config_found = true;
                    }
                }
            };

            if err.is_ok()
                && (config_mode == ReconfigState::DetCfgChange)
                && self.cb.is_config_changed()
                && self.cb.free_mem_callback().is_err()
            {
                err = Err(TpDecoderError::ParseError);
            }

            // If an error is detected terminate config parsing to avoid that an invalid
            // config is accepted in the second pass.
            if err.is_err() {
                break;
            }
        }

        if err.is_ok() && config_found {
            self.data.is_config_found = true;
        }

        err
    }

    /// Configures transport decoder via a binary coded USAC `new_config`.
    ///
    /// # Parameters
    ///
    /// - `new_config`: Buffer of the binary coded config.
    /// - `new_config_len`: Length of new config in bytes.
    /// - `is_flush_on`: Indicates flush status on return.
    /// - `is_build_up_on`: Indicates build up status on return.
    /// - `is_startup_phase`: Indicates decoder start up phase.
    ///
    /// # Return
    ///
    ///   - `TpDecoderError`.
    pub fn in_band_config(
        &mut self,
        new_config: &mut [u8],
        new_config_len: usize,
        is_flush_on: &mut bool,
        is_build_up_on: &mut bool,
        is_startup_phase: bool,
    ) -> Result<(), TpDecoderError> {
        let mut err = Ok(());
        if self.status_config_change != CfgChangeStatus::FlushOn {
            self.status_config_change = CfgChangeStatus::FlushOn;
            if usize::from(self.data.asc.usac_config().config_length()) == new_config_len
                && new_config == self.data.asc.usac_config().config_buffer()
            {
                if let Some(latm) = self.data.latm.as_deref_mut() {
                    if latm
                        .flags()
                        .contains(LatmFlags::USAC_EXPLICIT_CONFIG_CHANGED)
                    {
                        latm.remove_flags(LatmFlags::USAC_EXPLICIT_CONFIG_CHANGED);
                    } else if !is_startup_phase {
                        // Skip flush, and build_up phase
                        self.status_config_change = CfgChangeStatus::Undefined;
                    }
                } else if !is_startup_phase {
                    // Skip flush, and build_up phase
                    self.status_config_change = CfgChangeStatus::Undefined;
                }
            } else {
                // ISO/IEC 23003-3:2012/FDAM 3:2016(E) Annex F.2: explicit and implicit
                // config shall be identical.
                if let Some(latm) = self.data.latm.as_deref_mut() {
                    // Reset decoder to initial state to achieve definite behavior after
                    // error in config.
                    let _ = self.cb.free_mem_callback();
                    latm.init(self.data.transport_type);
                    self.data.reset();
                    self.status_config_change = CfgChangeStatus::Undefined;
                    err = Err(TpDecoderError::ParseError);
                }
            }

            // In startup phase skip flushing and continue with build up
            if !is_startup_phase || err.is_err() {
                *is_flush_on = self.status_config_change == CfgChangeStatus::FlushOn;
                *is_build_up_on = false;
                return err;
            }
        }

        // Decoder build up phase
        self.status_config_change = CfgChangeStatus::BuildUp;

        let mut bs = Bitstream::new(TP_USAC_MAX_CONFIG_LEN, Mode::Reader);
        bs.init(new_config, new_config_len << 3);

        for config_mode in ReconfigState::all() {
            if config_mode == ReconfigState::AllocMem {
                let num_bits = (new_config_len as isize) * 8 - bs.valid_bits();
                bs.push(-num_bits);
            }
            self.cb.set_config_mode(config_mode);

            // Config transport decoder.
            let mut dummy_asc = AudioSpecificConfig::new();
            dummy_asc.init();

            err = dummy_asc.parse(&mut bs, false, &mut self.cb, self.data.asc.aot());

            if err.is_err() {
                break;
            }

            if self.cb.update_config_callback(&dummy_asc).is_err() {
                err = Err(TpDecoderError::ParseError);
                break;
            }
            self.data.asc = dummy_asc;

            if config_mode == ReconfigState::DetCfgChange
                && self.cb.is_config_changed()
                && self.cb.free_mem_callback().is_err()
            {
                err = Err(TpDecoderError::ParseError);
                break;
            }
        }

        // Save new config.
        if err.is_ok() {
            self.data
                .asc
                .usac_config_mut()
                .set_config_length(new_config_len as u16);
            self.data.asc.usac_config_mut().config_buffer_mut()[..new_config_len]
                .copy_from_slice(&new_config[..new_config_len]);
            self.data.is_config_found = true;
        } else {
            self.data.reset();
            self.status_config_change = CfgChangeStatus::Undefined;
        }

        *is_flush_on = false;
        *is_build_up_on = self.status_config_change == CfgChangeStatus::BuildUp;

        err
    }

    /// Fills internal input buffer with bitstream data from the external input buffer. The
    /// function only copies such data as long as the decoder-internal input buffer is not full.
    /// So it grabs whatever it can from buffer and returns information (bytes_valid) so that
    /// at a subsequent call of fill_data(), the right position in buffer can be determined
    /// to grab the next data.
    ///
    /// # Parameters
    ///
    /// - `buffer`: External input buffer.
    /// - `bytes_valid`: Number of bitstream bytes in the external bitstream buffer that have not
    ///   yet been copied into the decoder's internal bitstream buffer.
    ///
    /// # Return
    ///
    /// - `Result<usize, TpDecoderError>`.
    ///   - `Ok(usize)`: Number of remaining valid bytes in the external bitstream buffer.
    ///   - `(usize, TpDecoderError)`: Number of remaining valid bytes with transport decoder error.
    pub fn fill_data(
        &mut self,
        buffer: &[u8],
        bytes_valid: usize,
    ) -> Result<usize, (usize, TpDecoderError)> {
        let tp_type = self.data.transport_type;
        let mut valid_bytes = 0;
        if tp_type == TransportType::Mp4Raw
            || tp_type == TransportType::Mp4LatmMcp0
            || tp_type == TransportType::Mp4LatmMcp1
        {
            if self.data.num_raw_data_blocks == 0 {
                self.bs.reset();
                valid_bytes = self.bs.feed(buffer, bytes_valid);
                if valid_bytes != 0 {
                    return Err((valid_bytes, TpDecoderError::TooManyBits));
                }
            }
        } else if bytes_valid == 0 {
            // Nothing to do.
            return Ok(valid_bytes);
        } else {
            valid_bytes = self.bs.feed(buffer, bytes_valid);

            if self.data.num_raw_data_blocks > 0 {
                self.data.global_frame_position += (bytes_valid as i32 - valid_bytes as i32) * 8;
            }
        }

        Ok(valid_bytes)
    }

    /// Returns reference to a callback.
    pub fn callback(&mut self) -> &mut TpDecCallBacks {
        &mut self.cb
    }

    /// Performs bitstream housekeeping. Done after parsing of the access unit.
    ///
    /// # Return
    ///
    /// `TpDecoderError`.
    pub fn end_access_unit(&mut self) -> Result<(), TpDecoderError> {
        let mut err = Ok(());

        match self.data.transport_type {
            TransportType::Mp4Adif => {
                // Make sure that bitbuffer does not get stuck.
                if self.bs.valid_bits() >= self.data.info.access_unit_anchor() {
                    self.bs.push(1);
                }

                // Make sure that raw_data_block() ends with byte alignment.
                self.bs.align(self.data.info.access_unit_anchor())
            }
            TransportType::Mp4Adts => {
                if let Some(adts) = self.data.adts.as_deref_mut() {
                    let block_num = adts.bs().num_raw_blocks() - self.data.num_raw_data_blocks;

                    adts.adjust_end_of_access_unit(
                        &mut self.bs,
                        block_num,
                        self.data.info.access_unit_anchor(),
                        self.data.global_frame_position.try_into().unwrap(),
                    );
                }
            }
            TransportType::Mp4Loas | TransportType::Mp4LatmMcp1 | TransportType::Mp4LatmMcp0 => {
                if let Some(latm) = self.data.latm.as_deref_mut() {
                    err = latm.adjust_end_of_access_unit(
                        &mut self.bs,
                        self.data.num_raw_data_blocks == 0,
                        self.data.global_frame_position as isize,
                    );

                    if err == Err(TpDecoderError::ParseError) {
                        self.data.reset();
                    }
                }
            }
            _ => (),
        }

        err
    }

    /// Reads one access unit from the `TransportDec` medium.
    ///
    /// # Return
    ///
    /// `TpDecoderError`.
    pub fn read_access_unit(&mut self) -> Result<(), TpDecoderError> {
        let mut err;

        if self.bs.valid_bits() <= 0 {
            self.data.reset();
            return Err(TpDecoderError::NotEnoughBits);
        }

        if self.data.transport_type == TransportType::Mp4Raw
            || self.data.transport_type == TransportType::Mp4LatmMcp0
            || self.data.transport_type == TransportType::Mp4LatmMcp1
        {
            err = self.data.read_header(&mut self.bs, &mut self.cb, true);
            if err == Err(TpDecoderError::NotEnoughBits) {
                err = Err(TpDecoderError::SyncError);
            }
            if err.is_ok() {
                self.data.reset();
            }
        } else {
            err = self
                .syncer
                .synchronise(&mut self.bs, &mut self.data, &mut self.cb);

            if self.data.transport_type == TransportType::Mp4Adts
                && self
                    .data
                    .adts
                    .as_ref()
                    .is_some_and(|adts| adts.is_mpeg2_implicit_config())
                && !self.syncer.is_sync_ok()
                && err.is_ok()
            {
                err = self.determine_implicit_channel_config();
            }
        }

        err
    }

    /// Returns the remaining amount of bits of the current access unit. The result can be below
    /// zero, meaning that too many bits have been read.
    pub fn remaining_au_bits(&mut self) -> isize {
        let valid_bits = self.bs.valid_bits();

        if self.data.info.access_unit_anchor() > 0
            && self.data.info.au_length() > 0
            && valid_bits >= 0
        {
            return isize::try_from(self.data.info.au_length()).unwrap()
                - (self.data.info.access_unit_anchor() - valid_bits);
        }

        valid_bits
    }

    /// Returns the total amount of bits of the current access unit.
    pub fn total_au_bits(&self) -> i32 {
        self.data.info.au_length()
    }

    /// Returns a mutable reference to `Bitstream`.
    pub fn bs_mut(&mut self) -> &mut Bitstream {
        &mut self.bs
    }

    /// Sets parameter.
    ///
    /// # Parameters
    ///
    /// - `param`: Identifier of the parameter to be changed.
    /// - `value`: Value for the parameter to be changed.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub fn set_param(&mut self, param: TpDecParam, value: bool) -> Result<(), TpDecoderError> {
        match param {
            TpDecParam::IgnoreBufferFullness => {
                self.syncer.set_is_ignore_buffer_fullness(value);
            }
            TpDecParam::Reset => {
                if value {
                    self.bs.reset();
                    self.data.reset();
                    self.syncer.set_is_sync_ok(false);
                }
            }
            TpDecParam::CheckTwoSyncs => {
                self.syncer.set_is_check_two_syncs(value);
            }
        };

        Ok(())
    }

    /// Sets current bitstream position as start of a new data region for CRC calculation.
    ///
    /// # Parameters
    ///
    /// - `m_bits`: Size in bits of the data region. Set to 0 if it should not be of a fixed size.
    ///
    /// # Return
    ///
    ///   - Data region ID, which should be used when calling `crc_end_region()`.
    pub fn crc_start_region(&mut self, m_bits: i32) -> i32 {
        if self.data.transport_type == TransportType::Mp4Adts {
            if let Some(adts) = self.data.adts.as_deref_mut() {
                return adts.crc_start_reg(&mut self.bs, m_bits) as i32;
            }
        }

        -1
    }

    /// Sets current bitstream position as end of a data region for CRC calculation.
    ///
    /// # Parameters
    ///
    /// - `region`: Data region ID, obtained from `crc_start_region()`.
    pub fn crc_end_region(&mut self, region: i32) {
        if self.data.transport_type == TransportType::Mp4Adts {
            if let Some(adts) = self.data.adts.as_deref_mut() {
                adts.crc_end_reg(&mut self.bs, region as usize);
            }
        }
    }

    /// Calculates ADTS CRC and checks if it is correct. The ADTS checksum is
    /// held internally.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub fn crc_check(&mut self) -> Result<(), TpDecoderError> {
        if self.data.transport_type == TransportType::Mp4Adts {
            if let Some(adts) = self.data.adts.as_deref_mut() {
                if adts.bs().num_raw_blocks() > 0 {
                    self.end_access_unit()
                } else {
                    adts.crc_check()
                }
            } else {
                Err(TpDecoderError::UnknownError)
            }
        } else {
            Ok(())
        }
    }

    /// Returns the trailing bits of the current access unit.
    ///
    /// # Parameters
    ///
    /// - `au_start_anchor`: Start anchor of AU.
    /// - `preroll_au_length`: Length of the preroll AU. Should be `None` for non-USAC bitstreams.
    /// - `ac_flags`: Audio codec flags.
    pub fn trailing_bits(
        &mut self,
        au_start_anchor: isize,
        preroll_au_length: Option<u32>,
        ac_flags: ACFlags,
    ) -> isize {
        let valid_bits = self.bs.valid_bits();

        if ac_flags.contains(ACFlags::USAC) && preroll_au_length.is_some() {
            // For pre-roll frames preroll bound has to be met
            if let Some(preroll_au_length) = preroll_au_length {
                valid_bits - au_start_anchor + isize::try_from(preroll_au_length).unwrap()
            } else {
                unreachable!()
            }
        } else if self.total_au_bits() > 0 {
            self.remaining_au_bits()
        } else if !ac_flags.contains(ACFlags::USAC) {
            (valid_bits - au_start_anchor) & 7
        } else {
            0
        }
    }

    /// Determines implicit channel configuration.
    /// In MPEG2 ADTS it is allowed to omit explicit channel configurations. The decoder
    /// has to use the channel configuration transmitted in access unit as bitstream audio
    /// element list. The configuration will be detected by trial and error.
    ///
    /// # Parameters
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(super) fn determine_implicit_channel_config(&mut self) -> Result<(), TpDecoderError> {
        let mut err = Err(TpDecoderError::SyncError);

        // 16 (dual-mono), 21 (2/2 ARIB), 30 (2/1 ARIB), + mpeg channel configs 1-7.
        static CH_CFG_TABLE: [u32; 10] = [16, 21, 30, 1, 2, 3, 4, 5, 6, 7];

        // Iterate with different channel configurations until decoder is able to
        // decode first access unit.
        for channel_config in CH_CFG_TABLE {
            // Get new PCE and call read_access_unit() again.
            self.data
                .asc
                .pce_as_mut()
                .get_default_config(channel_config);

            if self.data.asc.pce().is_valid() {
                // Jump back to synch word and read header.
                let num_bits = self.data.global_frame_position as isize - self.bs.valid_bits();
                self.bs.push(-num_bits);
                self.data.reset();

                if self
                    .data
                    .read_header(&mut self.bs, &mut self.cb, true)
                    .is_ok()
                {
                    let mut cb_clone = self.cb.clone();
                    let cb_err = cb_clone.decode_frame_callback(self);
                    self.cb = cb_clone;
                    if cb_err.is_ok() {
                        // Re-initialize decoder instance after successful decoder setup.
                        self.cb.set_config_mode(ReconfigState::AllocMem);

                        if self.cb.free_mem_callback().is_err()
                            || self.cb.update_config_callback(&self.data.asc).is_err()
                        {
                            err = Err(TpDecoderError::NeedToRestart);
                        } else {
                            // Jump back to access unit.
                            let num_bits =
                                self.data.info.access_unit_anchor() - self.bs.valid_bits();
                            self.bs.push(-num_bits);
                            err = Ok(())
                        }
                        break;
                    }
                }
            }
        }

        if err.is_err() {
            // Invalidate program config element.
            self.data.asc.pce_as_mut().init();
            if err == Err(TpDecoderError::NotEnoughBits) && self.syncer.rewind(&mut self.bs).is_ok()
            {
                return Err(TpDecoderError::NotEnoughBits);
            }
            self.syncer.skip(&mut self.bs);
            // Enforce re-sync of transport headers.
            self.data.reset();
            self.syncer.set_is_sync_ok(false);
        } else {
            self.syncer.set_is_sync_ok(true);
        }

        err
    }
}
