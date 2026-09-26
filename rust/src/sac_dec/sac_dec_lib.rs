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
//! SAC Decoder Library Interface

use super::{
    common::{SacDecParam, SacInputConfig},
    constants::{MAX_INPUT_CHANNELS, MAX_OUTPUT_CHANNELS},
    error_codes::SacDecoderError,
    flags::SacDecCtrlInitFlags,
    spatial_dec::{SpatialDec, SpatialDecInputMode},
    ssc::SpatialSpecificConfig,
};
use crate::{
    common::{
        aot::AudioObjectType,
        bitstream::Bitstream,
        channel_map_descr::ChannelMapDescriptor,
        enums::StereoCfgIndex,
        qmf::QmfFlags,
        qmf_domain::{QmfDomain, QmfDomainParams, QMF_MAX_PROC_SUBBANDS},
    },
    sac_dec::flags::{SacDecCtrlFlags, SacDecInitFlags},
    tp_dec::ReconfigState,
};

/// MPEG surround decoder structure.
#[repr(C)]
#[derive(Default, Debug)]
pub struct MpegSurroundDecoder {
    /// SSC structure which is used during decoding.
    ssc: SpatialSpecificConfig,
    /// SSC structure which is used while parsing.
    ssc_backup: SpatialSpecificConfig,
    /// Spatial decoder structure.
    spatial_dec: Box<SpatialDec>,
    /// Flag telling that the SSC is an out-of-band configuration.
    ssc_is_global_cfg: bool,
    /// Flag telling if time interface is used.
    use_time_interface: bool,
    /// Synchronization state.
    on_sync: MpegSynState,
    /// Initialization flags used in the spatial decoder.
    init_flags: SacDecInitFlags,
}

impl MpegSurroundDecoder {
    /// Creates a new `MpegSurroundDecoder` instance.
    pub fn new() -> Self {
        Self::default()
    }

    /// Reads and parse `SpatialSpecificConfig`. This function is called by callback.
    /// Parse SSC, and if it has changed, open SAC decoder (allocates SAC decoder memory on heap)
    /// if not opened already and signalize that it has to be (re-)configured later during decoding
    /// of the frame (in `self.init()`).
    ///
    /// # Parameters
    ///
    /// - `bs`: `Bitstream` instance with valid internal data.
    /// - `core_codec`: Audio object type `ELD` or `USAC`.
    /// - `sampling_rate`: Sampling frequency.
    /// - `frame_size`: Number of samples per frame.
    /// - `num_channels`: Number of channels.
    /// - `stereo_config_index`: Stereo config index value.
    /// - `core_sbr_frame_length_index`: Used only for USAC: Core sbr frame length index.
    /// - `config_bytes`: Number of bytes for config.
    /// - `config_mode`: Config detection mode or memory allocation.
    /// - `is_config_changed`: Flag indicates whether a config change happend.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    #[expect(clippy::too_many_arguments)]
    pub fn config(
        &mut self,
        bs: &mut Bitstream,
        core_codec: AudioObjectType,
        sampling_rate: u32,
        frame_size: u16,
        num_channels: u8,
        stereo_config_index: StereoCfgIndex,
        core_sbr_frame_length_index: u8,
        config_bytes: u32,
        config_mode: ReconfigState,
        is_config_changed: bool,
    ) -> Result<(), SacDecoderError> {
        let mut ssc_local = SpatialSpecificConfig::default();
        let ssc = if config_mode == ReconfigState::DetCfgChange {
            // In config detection mode write parameters into temporary structure.
            &mut ssc_local
        } else {
            &mut self.ssc_backup
        };

        ssc.parse_common_mps212_config(
            bs,
            core_codec,
            config_bytes,
            sampling_rate,
            stereo_config_index,
            core_sbr_frame_length_index,
        )?;

        if num_channels != ssc.n_input_channels() {
            return Err(SacDecoderError::ParseError);
        }

        ssc.check_out_of_band(core_codec, sampling_rate, frame_size)?;

        if config_mode == ReconfigState::AllocMem {
            if is_config_changed {
                *self.spatial_dec = SpatialDec::new(stereo_config_index, self.ssc_backup.is_usac());
                // Set default parameters.
                self.ssc_is_global_cfg = false;
                self.use_time_interface = true;

                // Signalize spatial decoder re-initalization.
                self.update_status(SacDecInitFlags::ENFORCE_REINIT);
            }

            if self.ssc_backup != self.ssc {
                self.init_flags.insert(SacDecInitFlags::CHANGE_HEADER);
                self.spatial_dec.init_bs_parser_context();
            }

            // We got a valid out-of-band configuration, so label it accordingly.
            self.ssc_is_global_cfg = true;
        }

        Ok(())
    }

    /// Resets MPEG Surround status info.
    fn update_status(&mut self, init_flags: SacDecInitFlags) {
        self.init_flags.insert(init_flags);

        if self.ssc_is_global_cfg && self.on_sync >= MpegSynState::Found {
            self.on_sync = MpegSynState::Found;
        } else {
            self.on_sync = MpegSynState::Lost;
        }
    }

    /// Configures QMF domain.
    ///
    /// # Parameters
    ///
    /// - `qd_params_requested`: Requested configuration parameters for the QMF domain.
    /// - `sac_dec_interface`: Enum `SacInputConfig`, time or QMF interface.
    /// - `core_codec`: Audio object type `ELD` or `USAC`.
    /// - `core_sampling_rate`: Core sampling frequency.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    pub fn configure_qmf_domain(
        &self,
        qd_params_requested: &mut QmfDomainParams,
        sac_dec_interface: SacInputConfig,
        core_codec: AudioObjectType,
        core_sampling_rate: u32,
    ) -> Result<(), SacDecoderError> {
        let ssc = &self.ssc_backup;
        if self.ssc_is_global_cfg {
            if sac_dec_interface == SacInputConfig::Time {
                // Note: For SacInputConfig::Qmf these parameters are set by SBR.
                // core_sampling_rate == output_sampling_rate for SacInputConfig::Time.
                qd_params_requested.num_bands_analysis = ssc.get_n_qmf_bands();
                qd_params_requested.num_bands_synthesis =
                    u16::from(qd_params_requested.num_bands_analysis);
            }
            qd_params_requested.num_input_channels = ssc.n_input_channels();
            qd_params_requested.num_output_channels = ssc.n_output_channels();
        } else {
            if sac_dec_interface == SacInputConfig::Time {
                // Note: For SacInputConfig::Qmf these parameters are set by SBR.
                // core_sampling_rate == output_sampling_rate for SacInputConfig::Time.
                qd_params_requested.num_bands_analysis =
                    SpatialSpecificConfig::get_n_qmf_bands_default(core_sampling_rate);
                qd_params_requested.num_bands_synthesis =
                    u16::from(qd_params_requested.num_bands_analysis);
            }
            qd_params_requested.num_input_channels = MAX_INPUT_CHANNELS as u8;
            qd_params_requested.num_output_channels = MAX_OUTPUT_CHANNELS as u8;
        }

        qd_params_requested.num_qmf_proc_bands = QMF_MAX_PROC_SUBBANDS;
        qd_params_requested.num_qmf_proc_channels = qd_params_requested
            .num_input_channels
            .min(MAX_INPUT_CHANNELS as u8);

        if core_codec == AudioObjectType::AotErAacEld {
            qd_params_requested.flags.insert(QmfFlags::MPSLDFB);
            qd_params_requested.flags.remove(QmfFlags::CLDFB);
        }

        Ok(())
    }

    /// Sets one single MPEG Surround decoder parameter.
    ///
    ///  Select signal input interface (`SacDecParam::Interface`) for MPEG Surround.
    ///
    /// - Switch time interface off:  0
    /// - Switch time interface on:   1
    ///
    ///
    /// # Parameters
    ///
    /// - `param`: Parameter to be set. It is type of `SacDecParam`.
    /// - `value`: Parameter value.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error
    pub fn set_params(&mut self, param: SacDecParam, value: i32) -> Result<(), SacDecoderError> {
        match param {
            SacDecParam::Interface => {
                if !(0..=1).contains(&value) {
                    return Err(SacDecoderError::InvalidParameter);
                }

                if self.use_time_interface != (value != 0) {
                    self.use_time_interface = value != 0;
                    self.init_flags
                        .insert(SacDecInitFlags::CHANGE_TIME_FREQ_INTERFACE);
                }
            }
            SacDecParam::BsInterruption => {
                if value != 0 {
                    self.update_status(SacDecInitFlags::BS_INTERRUPTION);
                }
            }
        }

        Ok(())
    }

    /// Parses MPEG Surround data with header. Header is `MpegsAncType`, `MpegsAncStartStop` (4 bits
    ///  total).
    ///
    /// # Parameters
    ///
    /// - `bs`: `Bitstream` instance with valid internal data.
    /// - `mps_payload_length`: If > 0 this value is the MPS payload length. If < 0, the MPS payload
    ///   length is unknown. If == 0, no valid MPS payload is available.
    /// - `core_codec`: Audio object type `ELD` or `USAC`.
    /// - `sample_rate`: Sampling frequency.
    /// - `frame_size`: Number of samples per frame.
    /// - `global_independency_flag`: Global independency flag of current frame.
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error.
    pub fn parse(
        &mut self,
        bs: &mut Bitstream,
        mps_payload_length: isize,
        core_codec: AudioObjectType,
        sample_rate: u32,
        frame_size: u16,
        global_independency_flag: bool,
    ) -> Result<(), SacDecoderError> {
        let mut result = Ok(());

        if mps_payload_length == 0 {
            return Err(SacDecoderError::ParseError);
        }

        let mps_start_pos = bs.valid_bits();

        // Parse `anc_type` and `anc_start_stop`.
        let (anc_type, anc_start_stop) = if core_codec.is_lowdelay_aot() {
            (
                MpegsAncType::try_from(bs.read(2) as u8)?,
                MpegsAncStartStop::from(bs.read(2) as u8),
            )
        } else {
            // Default anc_type for USAC: only payload without header info is possible.
            // Default anc_start_stop for USAC: payload is transmitted always in one chunk.
            (MpegsAncType::Frame, MpegsAncStartStop::StartStop)
        };

        match anc_start_stop {
            MpegsAncStartStop::StartStop => {
                let mut fall_through_flag = false;
                if anc_type == MpegsAncType::HeaderAndFrame {
                    fall_through_flag = true;
                    let ssc_tmp = self.ssc_backup;

                    // Parse spatial specific config.
                    result = self.ssc_backup.parse_specific_config_header(bs, core_codec);

                    // Check SSC for consistency (e.g. bit errors could cause trouble).
                    if result.is_ok() {
                        result = self.ssc_backup.check_in_band(frame_size, sample_rate);
                    }

                    match result {
                        Ok(_) => {
                            // Initiate re-initialization, if header has changed.
                            if self.ssc_backup != self.ssc {
                                self.init_flags.insert(SacDecInitFlags::CHANGE_HEADER);
                                self.spatial_dec.init_bs_parser_context();

                                // We found a valid in-band configuration. Therefore any
                                // previous config is invalid now.
                                self.ssc_is_global_cfg = false;
                            }
                        }
                        Err(_) => {
                            self.ssc_backup = ssc_tmp;
                            // Skip frame data.
                            fall_through_flag = false;
                        }
                    }
                }

                if anc_type == MpegsAncType::Frame || fall_through_flag {
                    if self.init_flags != SacDecInitFlags::empty() {
                        self.ssc = self.ssc_backup;
                        self.on_sync = MpegSynState::Found;
                    }

                    if self.on_sync >= MpegSynState::Found {
                        if bs.valid_bits() > 0 {
                            result = self.spatial_dec.parse_bs_frame_data(
                                bs,
                                &self.ssc,
                                global_independency_flag,
                            );
                        }

                        if bs.valid_bits() < 0
                            || (mps_payload_length > 0
                                && mps_payload_length < (mps_start_pos - bs.valid_bits()))
                        {
                            self.spatial_dec.reset_new_bs_data();
                            result = Err(SacDecoderError::ParseError);
                        }
                    }
                }
            }
            MpegsAncStartStop::Continue | MpegsAncStartStop::Stop | MpegsAncStartStop::Start => {
                result = Err(SacDecoderError::ParseError);
            }
        }

        match result {
            Ok(_) => Ok(()),
            Err(_) => {
                self.update_status(SacDecInitFlags::ERROR_PAYLOAD);
                Err(SacDecoderError::ParseError)
            }
        }
    }

    /// Applies MPEG Surround upmix.
    ///
    /// Process one downmix audio frame and decode one surround frame if it applies.
    /// Downmix framing can be different from surround framing, so depending on the
    /// frame size of the downmix audio data and the framing being used by the MPEG
    /// Surround decoder, it could be that only every second call, for example, of
    /// this function actually surround data was decoded. The returned value of
    /// `frame_size` will be zero, if no surround data was decoded.
    ///
    /// Decoding one MPEG Surround frame. Depending on interface configuration
    /// `set_param(self, SacDecParam, value)`, the QMF or time interface will be applied.
    /// External access to QMF buffer interface can be achieved using `QmfSlot` in SAC process
    /// module before decode frame. While using time interface, `time_data[]` buffer will be
    /// shared as input and output buffer.
    ///
    /// # Parameters
    ///
    /// - `qd`: QMF domain (`QmfDomain`) instance.
    /// - `qd_params_requested`: Requested configuration parameters for the QMF domain.
    /// - `time_data`: Parameter to be set. It is type of `SacDecParam`.
    /// - `time_data_framesize`: Frame size of input time data.
    /// - `num_channels`: Reference to memory where the amount of input channels is given and amount
    ///   of output channels is returned.
    /// - `frame_size`: Reference to memory where the amount of output samples is returned into.
    /// - `sample_rate`: Sampling frequency.
    /// - `core_codec`: Audio object type `ELD` or `USAC`.
    /// - `ch_map_descr`: Channel map descriptor for output channel mapping to be used (From MPEG
    ///   PCE ordering to whatever is required).
    ///
    /// # Return
    ///
    /// - Result<(), SacDecoderError>
    ///     - `Ok(())` if the function is successful.
    ///     - `Err(SacDecoderError)` if there is an error
    #[expect(clippy::too_many_arguments)]
    pub fn apply(
        &mut self,
        qd: &mut QmfDomain,
        qd_params_requested: &mut QmfDomainParams,
        time_data: &mut [f32],
        time_data_framesize: usize,
        num_channels: &mut u8,
        frame_size: &mut u16,
        sample_rate: u32,
        core_codec: AudioObjectType,
        ch_map_descr: &ChannelMapDescriptor,
    ) -> Result<(), SacDecoderError> {
        let mut control_flags = SacDecCtrlFlags::empty();
        let mut result = Ok(());

        // Length of time_data buffer size.
        let time_data_size = time_data.len();

        if !ch_map_descr.is_valid() {
            return Err(SacDecoderError::NotOk);
        }

        if *num_channels == 0 || *num_channels > 2 {
            return Err(SacDecoderError::NotOk);
        }

        if self.init_flags != SacDecInitFlags::empty() {
            result = self.init(qd, qd_params_requested);
        }

        match result {
            Ok(_) => {
                if self.on_sync != MpegSynState::Complete && self.spatial_dec.bs_independency_flag()
                {
                    // We got a valid header and independently decodeable frame data.
                    // -> Go to the next sync level and start processing.
                    self.on_sync = MpegSynState::Complete;
                }
            }
            Err(_) => {
                // We got a valid config header but found an error while parsing the
                // bitstream. Wait for the next independent frame and apply error
                // concealment in the meantime.
                self.on_sync = MpegSynState::Found;
                control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
            }
        }

        // Concealment:
        // - Bitstream is available, no sync found during bitstream processing.
        // - Bitstream is available, sync lost due to corrupted bitstream.
        // - Bitstream is available, sync found but no independent frame.
        if self.on_sync != MpegSynState::Complete {
            control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);
        }

        if self.init_flags != SacDecInitFlags::empty() {
            let mut start_with_dflt_cfg = false;
            // Init with a default configuration if we came here and are still not initialized.
            if self.init_flags.contains(SacDecInitFlags::ENFORCE_REINIT) {
                // Get default spatial specific config.
                let num_time_slots = u32::from(*frame_size)
                    / u32::from(SpatialSpecificConfig::get_n_qmf_bands_default(sample_rate));
                self.ssc_backup
                    .default_specific_config(core_codec, sample_rate, num_time_slots)?;

                // Initiate re-initialization, if header has changed.
                if self.ssc_backup != self.ssc {
                    self.init_flags.insert(SacDecInitFlags::CHANGE_HEADER);
                    self.spatial_dec.init_bs_parser_context();
                }

                start_with_dflt_cfg = true;
            }

            // First spatial specific config is parsed into ssc_backup,
            // second ssc_backup is copied into ssc.
            result = self.init(qd, qd_params_requested);

            if start_with_dflt_cfg {
                // Initialized with default config, but no sync found.
                // Maybe use update_status() later on.
                self.on_sync = MpegSynState::Lost;
            }
            // Since we do not have state MPEGS_SYNC_COMPLETE, apply concealment.
            control_flags.insert(SacDecCtrlFlags::MPEGS_CONCEAL);

            match result {
                Ok(_) => (),
                Err(error) => return Err(error),
            }
        }

        let init_control_flags = control_flags;

        // Check that provided output buffer is large enough. .
        if qd.num_bands_analysis() == 0 {
            return Err(SacDecoderError::UnsupportedFormat);
        }

        let mut time_data_required_size = time_data_framesize
            * usize::from(self.spatial_dec.num_output_channels())
            * usize::from(qd.num_bands_synthesis());
        time_data_required_size /= usize::from(qd.num_bands_analysis());

        if time_data_size < time_data_required_size {
            return Err(SacDecoderError::OutputBufferTooSmall);
        }

        // ----------------------------------------------------------------------------------//
        // iTime: input time signal
        // oTime: output time signal
        // iSpec: input qmf spectrum
        // oSpec: output qmf spectrum
        // iRes:  input residual signal
        // iResC: input residual signal that has been copied to
        //         the end of ch 1 of the output buffer
        //
        //      Memory management for USAC stereo_config_index 1 and
        //      core_sbr_frame_length_index 3 (sbr ratio: 2:1):
        //
        //                time_data
        //        time_in         time_out           qmf_domain_in   qmf_domain_out
        //       ch 0   ch1     ch 0   ch1           ch 0   ch1      ch 0  ch1
        //      -------------  -------------        -------------  -------------
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      -------------    -------------      -------------  -------------
        //
        //      Memory management for USAC stereo_config_index 2 and
        //      core_sbr_frame_length_index 3 (sbr ratio: 2:1):
        //
        //                time_data
        //        time_in         time_out           qmf_domain_in  qmf_domain_out
        //       ch 0   ch1     ch 0     ch1         ch 0    ch1    ch 0   ch1
        //      -------------  -------------------  -------------  -------------
        //      |     | iRes|  |oTime|oTime      |  |iSpec|     |  |     |     | lb
        //      |     | iRes|  |oTime|oTime      |  |iSpec|     |  |     |     | lb
        //      |     | iRes|  |oTime|oTime      |  |iSpec|     |  |     |     | lb
        //      |     | iRes|  |oTime|oTime      |  |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime/iResC|  |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime/iResC|  |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime/iResC|  |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime/iResC|  |iSpec|     |  |     |     | hb
        //      -------------  -------------------  -------------  -------------
        //
        //      Memory management for USAC stereo_config_index 3 and
        //      core_sbr_frame_length_index 3 (sbr ratio: 2:1):
        //
        //                time_data
        //        time_in         time_out        qmf_domain_in   qmf_domain_out
        //       ch 0   ch1     ch 0   ch1           ch 0   ch1      ch 0  ch1
        //      -------------  -------------        -------------  -------------
        //      |iTime| iRes|  |     |     |        |     |     |  |oSpec|oSpec| lb
        //      |iTime| iRes|  |     |     |        |     |     |  |oSpec|oSpec| lb
        //      |iTime| iRes|  |     |     |        |     |     |  |oSpec|oSpec| lb
        //      |iTime| iRes|  |     |     |        |     |     |  |oSpec|oSpec| lb
        //      |     |     |  |     |     |        |     |     |  |     |     | hb
        //      |     |     |  |     |     |        |     |     |  |     |     | hb
        //      |     |     |  |     |     |        |     |     |  |     |     | hb
        //      |     |     |  |     |     |        |     |     |  |     |     | hb
        //      -------------  -------------        -------------  -------------
        //
        //      Memory management for ELD with SBR:
        //
        //                time_data
        //        time_in         time_out           qmf_domain_in   qmf_domain_out
        //       ch 0   ch1     ch 0   ch1           ch 0   ch1      ch 0  ch1
        //      -------------  -------------        -------------  -------------
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | lb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      |     |     |  |oTime|oTime|        |iSpec|     |  |     |     | hb
        //      -------------  -------------        -------------  -------------
        //
        //      Memory management for ELD without SBR:
        //
        //                time_data
        //        time_in         time_out           qmf_domain_in   qmf_domain_out
        //       ch 0   ch1     ch 0   ch1           ch 0   ch1      ch 0  ch1
        //      -------------  -------------        -------------  -------------
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | lb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | lb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | lb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | lb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | hb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | hb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | hb
        //      |iTime|     |  |oTime|oTime|        |     |     |  |     |     | hb
        // ----------------------------------------------------------------------------------//

        if self.ssc.is_usac()
            && self.spatial_dec.stereo_config_index() == StereoCfgIndex::Mps212wResMonoSbr
        {
            // Available and needed memory size only differ in the number of channels.
            debug_assert!(
                (*num_channels == 2) && self.spatial_dec.num_output_channels() >= *num_channels
            );

            // Place samples comprising QMF time slots spaced at QMF output band raster
            // to allow slot wise processing.
            let time_data_framesize_out = (time_data_framesize
                * usize::from(qd.num_bands_synthesis()))
                / usize::from(qd.num_bands_analysis());

            debug_assert!(
                time_data_framesize
                    == (usize::from(qd.num_bands_analysis())
                        * usize::from(qd.num_qmf_time_slots()))
            );
            debug_assert!(
                time_data_framesize_out
                    == (usize::from(qd.num_bands_synthesis())
                        * usize::from(qd.num_qmf_time_slots()))
            );

            // Copy residual signal from ch 1 of the input buffer to the end of ch 1 of
            // the output buffer.
            let src = usize::from(*num_channels - 1) * time_data_framesize;
            let dest = usize::from(*num_channels) * time_data_framesize_out - time_data_framesize;

            let (src_slice, dest_slice) = time_data.split_at_mut(dest);
            let src_slice = &src_slice[src..src + time_data_framesize];
            let dest_slice = &mut dest_slice[..time_data_framesize];

            dest_slice.copy_from_slice(src_slice);
        }

        //
        // Process MPEG Surround Audio.
        //

        let input_mode = if self.use_time_interface {
            SpatialDecInputMode::Time
        } else {
            SpatialDecInputMode::QmfSbr
        };

        result = self.spatial_dec.apply_frame(
            qd,
            input_mode,
            ch_map_descr,
            self.ssc.syntax(),
            time_data,
            &mut control_flags,
            *frame_size,
            *num_channels,
        );

        *num_channels = self.spatial_dec.num_output_channels();

        match result {
            Ok(_) => {
                if control_flags.contains(SacDecCtrlFlags::MPEGS_CONCEAL)
                    && !init_control_flags.contains(SacDecCtrlFlags::MPEGS_CONCEAL)
                    || self.spatial_dec.has_error()
                {
                    self.update_status(SacDecInitFlags::ERROR_PAYLOAD);
                }
            }
            Err(error) => {
                // A fatal error occured. Go back to start and try again.
                self.update_status(SacDecInitFlags::ENFORCE_REINIT);

                // Declare that framework can not use the data in Time Out buffer.
                *frame_size = 0;

                return Err(error);
            }
        }
        Ok(())
    }

    /// Initializes MPEG Surround decoder.
    fn init(
        &mut self,
        qd: &mut QmfDomain,
        qd_params_requested: &mut QmfDomainParams,
    ) -> Result<(), SacDecoderError> {
        let mpegs_init_flags = self.init_flags;
        if self.ssc_backup.core_codec() != AudioObjectType::AotUsac {
            // Check if the Ssc is valid.
            self.ssc_backup.parse_check()?;
        }

        // Analyse states which have to be initialized.
        let sac_init_flags = self.calc_init_flags(mpegs_init_flags);

        self.ssc = self.ssc_backup;

        let do_init_qmf_states = sac_init_flags.contains(SacDecCtrlInitFlags::STATES_QMF_FILTER);
        Self::reinit_qmf_fb(
            qd,
            qd_params_requested,
            do_init_qmf_states,
            self.ssc.is_eld(),
        )?;

        let num_qmf_bands = self.ssc.get_n_qmf_bands();
        self.spatial_dec
            .init(&self.ssc, num_qmf_bands, sac_init_flags)?;

        // Signal that we got a header and can go on decoding.
        self.on_sync = MpegSynState::Found;

        self.init_flags = SacDecInitFlags::INIT_OK;

        Ok(())
    }

    /// Calculates SAC init flags.
    fn calc_init_flags(&self, mpegs_init_flags: SacDecInitFlags) -> SacDecCtrlInitFlags {
        let mut sac_init_flags = SacDecCtrlInitFlags::empty();

        if mpegs_init_flags.contains(SacDecInitFlags::ENFORCE_REINIT) {
            sac_init_flags.insert(SacDecCtrlInitFlags::ALL_STATES);
        } else if mpegs_init_flags.contains(SacDecInitFlags::CHANGE_HEADER) {
            if self.ssc.decorr_config() != self.ssc_backup.decorr_config() {
                sac_init_flags.insert(SacDecCtrlInitFlags::STATES_DECORRELATOR);
            }

            if self.ssc.temp_shape_config() != self.ssc_backup.temp_shape_config() {
                sac_init_flags.insert(SacDecCtrlInitFlags::STATES_GES);
            }

            sac_init_flags.insert(
                SacDecCtrlInitFlags::STATES_SMOOTHING | SacDecCtrlInitFlags::STATES_OVERWRITE_M2,
            );
        }

        sac_init_flags
    }

    /// Re-initializes QMF filter bank.
    fn reinit_qmf_fb(
        qd: &mut QmfDomain,
        qd_params_requested: &mut QmfDomainParams,
        do_init_qmf_states: bool,
        is_eld: bool,
    ) -> Result<(), SacDecoderError> {
        if do_init_qmf_states {
            qd_params_requested.flags.remove(QmfFlags::KEEP_STATES);
        } else {
            qd_params_requested.flags.insert(QmfFlags::KEEP_STATES);
        }

        if is_eld {
            qd_params_requested.flags.insert(QmfFlags::MPSLDFB);
        } else {
            qd_params_requested.flags.remove(QmfFlags::MPSLDFB);
        }

        if qd.reinit_filter_bank(qd_params_requested) {
            Ok(())
        } else {
            Err(SacDecoderError::NotOk)
        }
    }

    /// Returns the signal delay caused by the MPEG Surround decoder module.
    pub fn get_delay(&self) -> u32 {
        let core_codec = self.ssc.core_codec();
        let mut output_delay = 0;
        if core_codec > AudioObjectType::AotNullObject {
            if core_codec.is_lowdelay_aot() {
                // All low delay variants (ER-AAC-(E)LD):
                output_delay += 256;
            } else if core_codec != AudioObjectType::AotUsac {
                // By the method of elimination this is the GA (AAC-LC, HE-AAC, ...) branch.
                // cos to exp delay + QMF synthesis.
                output_delay += 320 + 257;

                if self.use_time_interface {
                    // QMF and hybrid analysis.
                    output_delay += 320 + 384;
                }
            }
        }

        output_delay
    }

    /// Returns info on whether the USAC pseudo LR feature is active.
    pub fn is_pseudo_lr(&self) -> bool {
        self.ssc.bs_pseudo_lr()
    }

    /// Deallocates internal heap allocated memories.
    pub fn free_internal_mem(&mut self) {
        self.spatial_dec.deinit();
        *self.spatial_dec = Default::default();
    }
}

/// MPEG Surround data segment indication.
#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
enum MpegsAncStartStop {
    /// Indicates if data segment continues a data block.
    #[default]
    Continue = 0,
    /// Indicates if data segment ends a data block.
    Stop = 1,
    /// Indicates if data segment begins a data block.
    Start = 2,
    /// Indicates if data segment begins and ends a data block.
    StartStop = 3,
}

/// MPEG Surround synchronization state.
// CAUTION: Changing the enumeration values can break the sync mechanism
// because it is based on comparing the state values.
#[repr(C)]
#[derive(Default, Debug, PartialEq, PartialOrd)]
enum MpegSynState {
    /// Indicates lost sync because of current discontinuity.
    #[default]
    Lost = 0,
    /// Parsed a valid header and (re)initialization was successfully completed.
    Found = 1,
    /// In sync and continuous. Found an independent frame in addition to MpegSynState::Found.
    /// Precondition: MpegSynState::Found.
    Complete = 2,
}

/// MPEG Surround data indication.
#[repr(C)]
#[derive(Debug, PartialEq, Copy, Clone)]
enum MpegsAncType {
    /// MPEG Surround frame, see ISO/IEC 23003-1.
    Frame,
    /// MPEG Surround header and MPEG Surround frame, see ISO/IEC 23003-1.
    HeaderAndFrame,
}

impl From<u8> for MpegsAncStartStop {
    fn from(value: u8) -> Self {
        match value {
            0 => MpegsAncStartStop::Continue,
            1 => MpegsAncStartStop::Stop,
            2 => MpegsAncStartStop::Start,
            3 => MpegsAncStartStop::StartStop,
            _ => panic!("MpegsAncStartStop is 2 bit wide information read from bitstream."),
        }
    }
}

impl TryFrom<u8> for MpegsAncType {
    type Error = SacDecoderError;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MpegsAncType::Frame),
            1 => Ok(MpegsAncType::HeaderAndFrame),
            _ => Err(SacDecoderError::ParseError),
        }
    }
}
