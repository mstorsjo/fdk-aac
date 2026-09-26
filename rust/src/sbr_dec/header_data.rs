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
//! SBR Header
//!
//! The header data is stored in a `SbrHeaderData` structure.
//!
//! The `SbrHeaderElement` can be transmitted with every frame; however, it
//! is typically sent every second or so. It contains fundamental information such
//! as SBR sampling frequency and frequency range, as well as control signals that
//! do not require frequent changes. It also includes the `SbrStandardElement`.
//!
//! Depending on the changes between the information in the current
//! `SbrHeaderElement` and the previous `SbrHeaderElement`, the SBR decoder might
//! need to be reset and reconfigured (e.g., new tables need to be calculated).

use crate::common::bitstream;
use crate::sbr_dec::constants::*;
use crate::sbr_dec::flags::*;
use crate::sbr_dec::freq_sca::*;
use crate::sbr_dec::huff_dec::*;
use crate::sbr_dec::pvc::Mode;
use crate::sbr_dec::{common::*, sr_mapping::*};
use crate::tp_dec::ReconfigState;

/// SBR header status.
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub enum Status {
    NotPresent,
    Error,
    Ok,
    Reset,
}

/// SBR synchronization state.
#[derive(Debug, Default, Clone, Copy, PartialEq, PartialOrd, Eq, Ord)]
#[repr(C)]
pub enum SyncState {
    #[default]
    NotInitialized,
    Upsampling,
    Header,
    Active,
}

/// SBR header data bitstream information.
#[derive(Debug, Default, Clone, Copy, PartialEq)]
#[repr(C)]
pub(super) struct BitstreamInfo {
    /// Amplitude resolution of envelope values (0: 1.5dB, 1: 3dB)
    amp_resolution: AmpRes,
    /// Start index in master_band_table[] used for dynamic crossover frequency
    crossover_band: u8,
    /// SBR prewhitening flag
    preprocessing: bool,
    /// Predictive vector coding mode
    pvc_mode: Mode,
}

impl BitstreamInfo {
    /// Getter for amplitude resolution.
    pub(super) fn amp_resolution(&self) -> AmpRes {
        self.amp_resolution
    }

    /// Getter for predictive vector coding mode.
    pub(super) fn pvc_mode(&self) -> Mode {
        self.pvc_mode
    }

    /// Getter for SBR prewhitening flag.
    pub(super) fn preprocessing(&self) -> bool {
        self.preprocessing
    }

    /// Getter for crossover band.
    pub(super) fn crossover_band(&self) -> u8 {
        self.crossover_band
    }
}

/// SBR header data bitstream.
/// Changes in these variables cause a reset of the decoder.
#[derive(Debug, Default, Clone, Copy, PartialEq)]
#[repr(C)]
pub(super) struct Bitstream {
    /// Frequency header data read from bitstream
    freq_band_header_data: FreqBandDataHeader,
    /// Index for number of limiter bands per octave
    limiter_bands: u8,
    /// Index to select gain limit
    limiter_gains: u8,
    /// Select gain calculation method (1: per QMF channel, 0: per SBR band)
    interpol_freq: u8,
    /// Smoothing of gains over time (0: on, 1: off)
    pub(super) smoothing_length: u8,
}

impl Bitstream {
    pub(super) fn smoothing_length(&self) -> u8 {
        self.smoothing_length
    }

    pub(super) fn interpol_freq(&self) -> u8 {
        self.interpol_freq
    }

    pub(super) fn limiter_gains(&self) -> u8 {
        self.limiter_gains
    }

    pub(super) fn limiter_bands(&self) -> u8 {
        self.limiter_bands
    }

    pub(super) fn freq_band_header_data(&self) -> FreqBandDataHeader {
        self.freq_band_header_data
    }
}

/// SBR header data.
#[derive(Debug, Default, Clone, Copy)]
#[repr(C)]
pub struct HeaderData {
    /// The current initialization status of the header
    sync_state: SyncState,
    /// True if the header has been updated.
    is_header_updated: bool,
    /// Signals if the header needs to be reset.
    needs_reset: bool,
    /// AAC: 16, 15
    number_time_slots: u8,
    /// Number of QMF analysis bands
    number_of_analysis_bands: u8,
    /// Time resolution of SBR in QMF-slots
    time_step: u8,
    /// SBR processing sampling frequency (always: CoreSamplingRate * UpSamplingFactor)
    processing_sampling_rate: u32,
    /// Current SBR header.
    bitstream: Bitstream,
    /// Default SBR header.
    default: Bitstream,
    /// SBR info.
    bitstream_info: BitstreamInfo,
    /// Pointer to struct FreqBandData
    freq_band_data: FreqBandData,
    /// Pointer to last frame's FreqBandData
    freq_band_data_prev: FreqBandData,
}

impl HeaderData {
    /// Read header bits from bitstream.
    pub fn read_header_bits(bs: &mut bitstream::Bitstream, flags: SbrDecFlags) -> Status {
        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            // amp_resolution
            bs.read(SI_SBR_AMP_RES_BITS);
        }
        // start_freq, stop_freq
        bs.push(8);
        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            // crossover_band
            bs.read(SI_SBR_XOVER_BAND_BITS);
            // reserved bits
            bs.read(SI_SBR_RESERVED_BITS_HDR);
        }

        let header_extra_1: u32 = bs.read_bit();
        let header_extra_2: u32 = bs.read_bit();
        bs.push((5 * header_extra_1 + 6 * header_extra_2) as isize);

        Status::Ok
    }

    /// Reads header data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream structure.
    /// - `flags`: SBR decoder flags.
    /// - `is_sbr_data`: Flag indicates if SBR data is available.
    /// - `config_mode`: Config detection mode or memory allocation mode.
    ///
    /// # Return
    ///
    /// Status
    pub fn get_header_data(
        &mut self,
        bs: &mut bitstream::Bitstream,
        flags: SbrDecFlags,
        is_sbr_data: bool,
        config_modes: ReconfigState,
    ) -> Status {
        if config_modes == ReconfigState::DetCfgChange {
            return Self::read_header_bits(bs, flags);
        }

        // Copy SBR bit stream header to temporary header
        let last_header: Bitstream = self.bitstream;
        let last_info: BitstreamInfo = self.bitstream_info;

        // Read new header from bitstream
        let bitstream_data: &mut Bitstream =
            if flags.contains(SbrDecFlags::SYNTAX_USAC) && !is_sbr_data {
                &mut self.default
            } else {
                &mut self.bitstream
            };

        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            // SI_SBR_AMP_RES_BITS = 1, hence using read_bit()
            self.bitstream_info.amp_resolution = AmpRes::from(bs.read_bit());
        }

        bitstream_data
            .freq_band_header_data
            .read_freq_band_data_header(bs);

        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            self.bitstream_info.crossover_band = bs.read(SI_SBR_XOVER_BAND_BITS) as u8;
            bs.read(SI_SBR_RESERVED_BITS_HDR);
        }

        // SI_SBR_HEADER_EXTRA_1_BITS and SI_SBR_HEADER_EXTRA_2_BITS both equal to 1, hence
        // using read_bit()
        let header_extra_1 = bs.read_bit() != 0;
        let header_extra_2 = bs.read_bit() != 0;

        // Handle extra header information
        bitstream_data
            .freq_band_header_data
            .read_freq_band_data_header_extra(bs, header_extra_1);

        if header_extra_2 {
            bitstream_data.limiter_bands = bs.read(SI_SBR_LIMITER_BANDS_BITS) as u8;
            bitstream_data.limiter_gains = bs.read(SI_SBR_LIMITER_GAINS_BITS) as u8;
            // SI_SBR_INTERPOL_FREQ_BITS = 1, hence using read_bit()
            bitstream_data.interpol_freq = bs.read_bit() as u8;
            // SI_SBR_SMOOTHING_LENGTH_BITS = 1, hence read_bit()
            bitstream_data.smoothing_length = bs.read_bit() as u8;
        } else {
            bitstream_data.limiter_bands = SBR_LIMITER_BANDS_DEFAULT;
            bitstream_data.limiter_gains = SBR_LIMITER_GAINS_DEFAULT;
            bitstream_data.interpol_freq = SBR_INTERPOL_FREQ_DEFAULT;
            bitstream_data.smoothing_length = SBR_SMOOTHING_LENGTH_DEFAULT;
        }

        // Look for new settings. IEC 14496-3, 4.6.18.3.1
        if self.sync_state < SyncState::Header
            || last_info.crossover_band != self.bitstream_info.crossover_band
            || last_header
                .freq_band_header_data
                .cmp_freq_band_data_header(&bitstream_data.freq_band_header_data)
        {
            return Status::Reset;
        }
        Status::Ok
    }

    /// Getter for Sync State
    pub fn sync_state(&self) -> SyncState {
        self.sync_state
    }

    /// Setter for Sync State
    pub fn set_sync_state(&mut self, sync_state: SyncState) -> SbrError {
        self.sync_state = sync_state;
        SbrError::Ok
    }

    /// True if the header has been updated
    pub fn is_header_updated(&self) -> bool {
        self.is_header_updated
    }

    /// Signal that the header has been updated
    pub fn set_header_updated(&mut self, update_header: bool) -> Result<(), SbrError> {
        self.is_header_updated = update_header;
        Ok(())
    }

    /// Signals if the header needs to be reset
    pub fn header_needs_reset(&self) -> bool {
        self.needs_reset
    }

    /// Indicate that the header needs to be reset
    pub fn set_header_needs_reset(&mut self, needs_reset: bool) -> SbrError {
        self.needs_reset = needs_reset;
        SbrError::Ok
    }

    /// Getter for number_time_slots.
    pub(super) fn number_time_slots(&self) -> u8 {
        self.number_time_slots
    }

    /// Getter for time step.
    pub(super) fn time_step(&self) -> u8 {
        self.time_step
    }

    /// Getter for bitstream info.
    pub(super) fn bitstream_info(&self) -> &BitstreamInfo {
        &self.bitstream_info
    }

    /// Getter for frequency band data.
    pub fn freq_band_data_as_mut(&mut self) -> &mut FreqBandData {
        &mut self.freq_band_data
    }

    /// Getter for frequency band data.
    pub(super) fn freq_band_data(&self) -> &FreqBandData {
        &self.freq_band_data
    }

    /// Getter for frequency band data previous.
    pub(super) fn _freq_band_data_prev(&self) -> &FreqBandData {
        &self.freq_band_data_prev
    }

    /// Getter for frequency band data previous.
    pub(super) fn freq_band_data_prev_mut(&mut self) -> &mut FreqBandData {
        &mut self.freq_band_data_prev
    }

    /// Getter for current SBR header.
    pub(super) fn bitstream(&self) -> &Bitstream {
        &self.bitstream
    }

    /// Getter for analysis bands number.
    pub fn number_of_analysis_bands(&self) -> u8 {
        self.number_of_analysis_bands
    }

    /// Getter for processing sampling rate.
    pub fn processing_sampling_rate(&self) -> u32 {
        self.processing_sampling_rate
    }

    /// Initializes SBR header data.
    ///
    /// Copy default values to the header data struct and patch some entries
    /// depending on the core codec.
    ///
    /// # Parameters
    ///
    /// - `sample_rate_in`: Input samplerate.
    /// - `sample_rate_out`: Output samplerate.
    /// - `downscale_factor`: Downscalefactor.
    /// - `samples_per_frame`: AAC core frame size.
    /// - `flags`: SBR decoder flags.
    /// - `set_default_header`: Flag indicates setting of default header.
    ///
    /// # Return
    ///
    /// SbrError
    pub fn init_header_data(
        &mut self,
        sample_rate_in: u32,
        sample_rate_out: u32,
        downscale_factor: u8,
        samples_per_frame: u16,
        flags: SbrDecFlags,
        set_default_header: bool,
    ) -> SbrError {
        let processing_sampling_rate: u32 = if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            map_to_std_sample_rate(sample_rate_out * downscale_factor as u32, false)
        } else {
            sample_rate_out * downscale_factor as u32
        };

        let mut num_analysis_bands: u16;

        if sample_rate_in == sample_rate_out {
            self.processing_sampling_rate = processing_sampling_rate << 1;
            num_analysis_bands = 32;
        } else {
            self.processing_sampling_rate = processing_sampling_rate;
            if (sample_rate_out >> 1) == sample_rate_in {
                // 1:2
                num_analysis_bands = 32;
            } else if (sample_rate_out >> 2) == sample_rate_in {
                // 1:4
                num_analysis_bands = 16;
            } else if (sample_rate_out * 3) >> 3 == (sample_rate_in * 8) >> 3 {
                // 3:8, 3/4 core frame length
                num_analysis_bands = 24;
            } else {
                return SbrError::UnsupportedConfig;
            }
        }
        num_analysis_bands /= downscale_factor as u16;

        if set_default_header {
            // Fill in default values first
            self.sync_state = SyncState::NotInitialized;
            self.needs_reset = false;
            self.is_header_updated = false;

            self.bitstream_info.amp_resolution = AmpRes::Res3_0;
            self.bitstream_info.crossover_band = 0;
            self.bitstream_info.preprocessing = false;
            self.bitstream_info.pvc_mode = Mode::None;

            self.bitstream.limiter_bands = SBR_LIMITER_BANDS_DEFAULT;
            self.bitstream.limiter_gains = SBR_LIMITER_GAINS_DEFAULT;
            self.bitstream.interpol_freq = SBR_INTERPOL_FREQ_DEFAULT;
            self.bitstream.smoothing_length = SBR_SMOOTHING_LENGTH_DEFAULT;

            // Patch some entries
            self.bitstream
                .freq_band_header_data
                .set_freq_band_data_header_default(sample_rate_out, downscale_factor);
        }

        if (sample_rate_out >> 2) == sample_rate_in {
            self.time_step = 4;
        } else {
            self.time_step = if flags.contains(SbrDecFlags::ELD_GRID) {
                1
            } else {
                2
            };
        }

        // One SBR timeslot corresponds to the amount of samples equal to the amount
        // of analysis bands, divided by the timestep.
        self.number_time_slots = ((samples_per_frame / num_analysis_bands)
            >> (self.time_step as u16 - 1))
            .try_into()
            .unwrap();
        if self.number_time_slots > 16 {
            return SbrError::UnsupportedConfig;
        }

        self.number_of_analysis_bands = num_analysis_bands.try_into().unwrap();
        if (sample_rate_out >> 2) == sample_rate_in {
            self.number_time_slots <<= 1;
        }
        SbrError::Ok
    }

    /// Updates the SBR header.
    ///
    /// # Parameters
    ///
    /// - `status`: Header status value returned from SBR header parser.
    /// - `flags`: SBR decoder flags.
    ///
    /// # Return
    ///
    /// SbrError
    pub fn header_update(&mut self, status: Status, flags: SbrDecFlags) -> SbrError {
        // Change of control data, reset decoder
        let error_status = self.freq_band_data.reset_freq_band_tables(
            &self.bitstream.freq_band_header_data,
            self.bitstream_info.crossover_band,
            self.number_of_analysis_bands,
            self.processing_sampling_rate,
            flags,
        );

        if error_status == SbrError::Ok {
            if self.sync_state() == SyncState::Upsampling && status != Status::Reset {
                // As the default header would limit the frequency range,
                // start_subband and end_subband must be patched.
                self.freq_band_data.start_subband = self.number_of_analysis_bands;
                self.freq_band_data.end_subband = self.number_of_analysis_bands;
            }

            // Trigger a reset before processing this slot
            self.needs_reset = true;
        }

        error_status
    }

    pub fn bitstream_info_parse(
        &mut self,
        status: &mut Status,
        bitstream: &mut bitstream::Bitstream,
        is_pvc: bool,
        is_cpe: bool,
    ) {
        let mut new_bitstream_info = BitstreamInfo {
            amp_resolution: AmpRes::from(bitstream.read_bit()),
            crossover_band: bitstream.read(4).try_into().unwrap(),
            preprocessing: bitstream.read_bit() != 0,
            pvc_mode: Mode::None,
        };

        if is_pvc {
            new_bitstream_info.pvc_mode = Mode::from(bitstream.read(2) as usize);
            // pvc_mode: 0 -> no PVC, 1 -> PVC mode 1, 2 -> PVC mode 2, 3 -> reserved
            if new_bitstream_info.pvc_mode > Mode::from(2_usize) {
                *status = Status::Error;
            }
            if is_cpe && (new_bitstream_info.pvc_mode > Mode::from(0_usize)) {
                // pvc is always transmitted but pvc_mode is set to zero in case of
                // stereo SBR. The config might be wrong but we cannot tell for sure.
                new_bitstream_info.pvc_mode = Mode::None;
            }
        } else {
            new_bitstream_info.pvc_mode = Mode::None;
        }

        if *status != Status::Error {
            if self.bitstream_info != new_bitstream_info {
                // In case of amp_resolution and preprocessing change no full reset required
                if (self.bitstream_info.pvc_mode != new_bitstream_info.pvc_mode)
                    || (self.bitstream_info.crossover_band != new_bitstream_info.crossover_band)
                {
                    *status = Status::Reset;
                } else {
                    *status = Status::Ok;
                }

                self.bitstream_info = new_bitstream_info;
            } else {
                *status = Status::Ok;
            }
        }
    }

    /// Sets the default BS data in the SBR header if the current data differs
    /// from the default or if the synchronization state is not active.
    ///
    /// # Parameters
    ///
    /// - `status`: Header status value returned from SBR header parser.
    pub fn bitstream_set_default(&mut self, status: &mut Status) {
        if self.bitstream != self.default || self.sync_state() != SyncState::Active {
            self.bitstream = self.default;
            *status = Status::Reset;
        }
    }

    /// Resets the MPEG-4 eSBR header data
    pub(super) fn reset_esbr_header_data(&mut self) {
        self.bitstream_info.preprocessing = false;
    }

    /// Reads the extension data MPEG-4 SBR Enhancements from the bitstream
    ///
    /// # Parameters
    ///
    /// - `bitstream`: Bitstream structure.
    pub(super) fn read_esbr_header_data(&mut self, bitstream: &mut bitstream::Bitstream) -> i32 {
        self.bitstream_info.preprocessing = bitstream.read_bit() != 0;
        1
    }
}

pub fn copy_header(destination: &mut HeaderData, source: &HeaderData) {
    *destination = *source;
}

pub fn compare_header(header_1: &HeaderData, header_2: &HeaderData) -> bool {
    header_1.sync_state != header_2.sync_state
        || header_1.is_header_updated != header_2.is_header_updated
        || header_1.needs_reset != header_2.needs_reset
        || header_1.number_time_slots != header_2.number_time_slots
        || header_1.number_of_analysis_bands != header_2.number_of_analysis_bands
        || header_1.time_step != header_2.time_step
        || header_1.processing_sampling_rate != header_2.processing_sampling_rate
        || header_1.bitstream != header_2.bitstream
        || header_1.default != header_2.default
        || header_1.bitstream_info != header_2.bitstream_info
        || header_1.freq_band_data != header_2.freq_band_data
}
