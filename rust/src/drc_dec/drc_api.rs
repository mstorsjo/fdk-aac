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
//! DRC decoder according to ISO/IEC 23003-4 (MPEG-D DRC) including ISO/IEC 23003-4/AMD1 (Amend. 1).

use super::{
    common::{CodecMode, DelayMode, ProcessingLocation, SubbandDomainMode},
    constants::{DRCDEC_MAX_CHANNELS, UNDEFINED_LOUDNESS_VALUE},
    drc_error::DrcDecError,
    functional_range::DrcDecFunctionalRange,
    gain_decoder::*,
    parameters::DrcDecParameters,
    reader::Reader,
    selection_process::{SelProcUserParam, SelectionProcess, SelectionProcessOutput},
};
use crate::common::bitstream::Bitstream;
use itertools::izip;

#[repr(C)]
#[derive(PartialEq, Default, Debug)]
/// Status of the DRC payload.
enum DrcDecStatus {
    #[default]
    NotInitialized = 0,
    Initialized,
    NewGainPayload,
    InterpolationPrepared,
}

#[repr(C)]
#[derive(Default, Debug)]
pub struct DrcDecoder {
    codec_mode: CodecMode,
    functional_range: DrcDecFunctionalRange,
    status: DrcDecStatus,

    gain_dec: Option<Box<DrcGainDecoder>>,
    sel_proc: Option<Box<SelectionProcess>>,
    reader: Box<Reader>,

    sel_proc_output: SelectionProcessOutput,
    is_sel_proc_input_diff: bool,
}

impl DrcDecoder {
    /// Applies downmixing on the `real_buffer`. Downmix is only performed if downmix coefficients
    /// are provided. The channel maps are necessary for applying the downmix coefficients to the
    /// correspondent channels.
    ///
    /// # Parameters
    /// - `reverse_in_channel_map`: Reverse input channel map, containing audio channel indices
    /// - `reverse_out_channel_map`: Reverse output channel map, containing audio channel indices
    /// - `real_buffer`: The I/O de-interleaved audio signal buffer, containing audio data
    /// - `num_channels`: I/O parameter containing the number of input channels and updating to the
    ///   number of downmixed output channels.
    ///
    /// # Return
    /// - Result<(), DrcDecError>
    ///   - Ok(_) => {
    ///      - Downmix successfully applied.
    ///      - No downmix coeffients to be applied.
    ///      - The num. of output channels >= num. of input channels. },
    ///   - Err(e) => {
    ///      - Number of input channels or output channels is > than DRCDEC_MAX_CHANNELS.
    ///      - `num_channels` does not reflect the accurate number of base channels.
    ///      - Internal parameters are wrong. }
    pub fn apply_downmix(
        &mut self,
        reverse_in_channel_map: &[usize],
        reverse_out_channel_map: &[usize],
        real_buffer: &mut [f32],
        num_channels: &mut usize,
    ) -> Result<(), DrcDecError> {
        let sel_proc_out = &self.sel_proc_output;
        let base_ch_cnt = usize::from(sel_proc_out.base_channel_count());
        let target_ch_cnt = usize::from(sel_proc_out.target_channel_count());

        let mut tmp_out = [0.0_f32; DRCDEC_MAX_CHANNELS];

        if (self.functional_range & DrcDecFunctionalRange::Gain) == 0 {
            return Err(DrcDecError::NotOk);
        }

        // Only downmix is performed here, no upmix.
        // Downmix is only performed if downmix coefficients are provided.
        // All other cases of downmix and upmix are treated by pcm_dmx module.
        if !sel_proc_out.is_downmix_matrix_present() {
            // No downmix.
            return Ok(());
        }
        if target_ch_cnt >= base_ch_cnt {
            // Downmix only.
            return Ok(());
        }

        // Sanity checks.
        if base_ch_cnt > DRCDEC_MAX_CHANNELS || target_ch_cnt > DRCDEC_MAX_CHANNELS {
            return Err(DrcDecError::NotOk);
        }
        if base_ch_cnt != *num_channels {
            return Err(DrcDecError::NotOk);
        }
        let frame_size = match &self.gain_dec {
            Some(drc_gain) => usize::from(drc_gain.frame_size()),
            None => {
                return Err(DrcDecError::NotOk);
            }
        };

        let dmx_coeff = sel_proc_out.downmix_matrix();

        // In-place downmix.
        for n in 0..frame_size {
            for oc in 0..target_ch_cnt {
                tmp_out[oc] = 0.0_f32;
                for ic in 0..base_ch_cnt {
                    tmp_out[oc] += real_buffer[ic * frame_size + n]
                        * dmx_coeff[reverse_in_channel_map[ic]][reverse_out_channel_map[oc]];
                }
            }
            for oc in 0..target_ch_cnt {
                if oc >= base_ch_cnt {
                    break;
                }
                real_buffer[oc * frame_size + n] = tmp_out[oc];
            }
        }

        for oc in target_ch_cnt..base_ch_cnt {
            real_buffer[oc * frame_size..((oc + 1) * frame_size)].fill(0.0);
        }

        *num_channels = target_ch_cnt;

        Ok(())
    }

    /// Initializes the `DrcDecoder` instance. The function returns early, without initializing,
    /// when invalid arguments are passed.
    ///
    /// # Parameters
    ///
    /// - `frame_size`: Length of DRC frame (i.e. number of samples per frame ( expect value > 0 ))
    /// - `sample_rate`: Sampling frequency ( expect value > 0 )
    /// - `base_channel_count`: Number of base channels in channel layout ( expect value > 0 )
    ///
    /// # Return
    ///
    /// - Result<(), DrcDecError>
    pub fn init(
        &mut self,
        frame_size: u16,
        sample_rate: u32,
        base_channel_count: u8,
    ) -> Result<(), DrcDecError> {
        if frame_size == 0 || sample_rate == 0 || base_channel_count == 0 {
            // Return without doing anything.
            return Ok(());
        }

        self.reader.set_frame_size(frame_size)?;
        self.reader.init_delta_time_min(sample_rate)?;

        if (self.functional_range & DrcDecFunctionalRange::Selection) != 0 {
            if let Some(sel_proc) = &mut self.sel_proc {
                sel_proc.set_param(
                    SelProcUserParam::BaseChannelCount(base_channel_count),
                    Some(&mut self.is_sel_proc_input_diff),
                )?;
                sel_proc.set_param(
                    SelProcUserParam::SampleRate(sample_rate),
                    Some(&mut self.is_sel_proc_input_diff),
                )?;
            }
        }
        if (self.functional_range & DrcDecFunctionalRange::Gain) != 0 {
            if let Some(gain_dec) = &mut self.gain_dec {
                gain_dec.set_param(DrcGainDecoderParam::FrameSize(frame_size))?;
                gain_dec.set_param(DrcGainDecoderParam::SampleRate(sample_rate))?;
                gain_dec.init()?;
            }
        }

        self.status = DrcDecStatus::Initialized;

        self.start_selection_process()?;

        Ok(())
    }

    /// Returns `true` if the data from `old_sel_proc_output` is different than the current one.
    fn is_reset_needed(&mut self, old_sel_proc_output: &SelectionProcessOutput) -> bool {
        let mut is_reset_needed = false;

        let curr_sel_proc_output = &self.sel_proc_output;

        if curr_sel_proc_output.output_peak_level_db() != old_sel_proc_output.output_peak_level_db()
        {
            is_reset_needed = true;
        }
        if curr_sel_proc_output.loudness_normalization_gain_db()
            != old_sel_proc_output.loudness_normalization_gain_db()
        {
            is_reset_needed = true;
        }
        if curr_sel_proc_output.output_loudness() != old_sel_proc_output.output_loudness() {
            is_reset_needed = true;
        }
        if curr_sel_proc_output.num_selected_drc_sets()
            != old_sel_proc_output.num_selected_drc_sets()
        {
            is_reset_needed = true;
        } else {
            let num_drc_sets = usize::from(curr_sel_proc_output.num_selected_drc_sets());
            for (this_drc_set_id, this_drc_dmx_id, old_drc_set_id, old_drc_dmx_id) in izip!(
                curr_sel_proc_output.selected_drc_set_ids().iter(),
                curr_sel_proc_output.selected_downmix_ids().iter(),
                old_sel_proc_output.selected_drc_set_ids().iter(),
                old_sel_proc_output.selected_downmix_ids().iter(),
            )
            .take(num_drc_sets)
            {
                if *this_drc_set_id != *old_drc_set_id {
                    is_reset_needed = true;
                }
                if *this_drc_dmx_id != *old_drc_dmx_id {
                    is_reset_needed = true;
                }
            }
        }
        if curr_sel_proc_output.boost() != old_sel_proc_output.boost() {
            is_reset_needed = true;
        }
        if curr_sel_proc_output.compress() != old_sel_proc_output.compress() {
            is_reset_needed = true;
        }
        // Note: Changes in downmix matrix are not caught, as they don't affect the DRC gain
        // decoder.
        is_reset_needed
    }

    /// Returns whether MPEG-D DRC payload is present. `MPEG-D` DRC overrides `MPEG-4` DRC, if
    /// `uni_drc` payload is present: (`loudness_info_set` and/or `uni_drc_config`).
    pub fn is_drc_active(&self) -> bool {
        let loudness_info_set = self.reader.loudness_info_set();
        let uni_drc_config = self.reader.uni_drc_config();

        (loudness_info_set.li_count() > 0)
            || (loudness_info_set.li_album_count() > 0)
            || (uni_drc_config.drc_instructions_uni_drc_count() > 0)
            || (uni_drc_config.downmix_instructions_count() > 0)
    }

    /// Returns the output MPEG-D DRC boost level.
    pub fn boost_level(&self) -> f32 {
        self.sel_proc_output.boost()
    }

    /// Returns the output MPEG-D DRC compression level.
    pub fn compress_level(&self) -> f32 {
        self.sel_proc_output.compress()
    }

    /// Returns the output loudness value (in dB). The value is `UNDEFINED_LOUDNESS_VALUE`, if
    /// no loudness is contained in the bitstream.
    pub fn out_loudness_level(&self) -> f32 {
        self.sel_proc_output.output_loudness()
    }

    fn do_selection_process(
        &mut self,
        uni_drc_config_has_changed: &mut bool,
    ) -> Result<(), DrcDecError> {
        if (self.functional_range & DrcDecFunctionalRange::Selection) != 0 {
            let tmp_reader = &mut self.reader;

            *uni_drc_config_has_changed = tmp_reader.uni_drc_config.is_diff();
            let loudness_info_has_changed = tmp_reader.loudness_info_set.is_diff();

            if *uni_drc_config_has_changed
                || loudness_info_has_changed
                || self.is_sel_proc_input_diff
            {
                // In case of an error, signal that selection process was not successful.
                self.sel_proc_output.set_num_selected_drc_sets(0);

                match &mut self.sel_proc {
                    Some(drc_sel_proc) => {
                        drc_sel_proc.process(
                            &mut tmp_reader.uni_drc_config,
                            &tmp_reader.loudness_info_set,
                            &mut self.sel_proc_output,
                        )?;
                    }
                    None => {
                        return Err(DrcDecError::NotOk);
                    }
                }

                self.is_sel_proc_input_diff = false;
                tmp_reader.uni_drc_config.set_diff(false);
                tmp_reader.loudness_info_set.set_diff(false);
            }
        }

        Ok(())
    }

    fn do_gain_config(
        &mut self,
        uni_drc_config_has_changed: &bool,
        old_sel_proc_output: &SelectionProcessOutput,
    ) -> Result<(), DrcDecError> {
        if (self.functional_range & DrcDecFunctionalRange::Gain) != 0
            && (self.is_reset_needed(old_sel_proc_output) || *uni_drc_config_has_changed)
        {
            match &mut self.gain_dec {
                Some(gain_decoder) => {
                    gain_decoder.config(
                        &self.reader.uni_drc_config,
                        self.sel_proc_output.num_selected_drc_sets(),
                        self.sel_proc_output.selected_drc_set_ids(),
                    )?;
                }
                None => {
                    return Err(DrcDecError::NotOk);
                }
            }
        }
        Ok(())
    }

    fn start_selection_process(&mut self) -> Result<(), DrcDecError> {
        if self.status == DrcDecStatus::NotInitialized {
            return Ok(());
        }

        let mut uni_drc_config_has_changed = false;
        let old_sel_proc_output = self.sel_proc_output;

        self.do_selection_process(&mut uni_drc_config_has_changed)?;
        self.do_gain_config(&uni_drc_config_has_changed, &old_sel_proc_output)?;

        Ok(())
    }

    /// Creates a new instance of `DrcDecoder`, by allocating heap memory for its
    /// struct fields. The memory allocation depends on `DrcDecFunctionalRange`. For a full
    /// allocation the `DrcDecFunctionalRange::All` should be used.
    ///
    /// # Parameters
    /// - `functional_range`: Specifies the MPEG-D DRC functionality, that `self` should enable.
    pub fn new(func_range: DrcDecFunctionalRange) -> Self {
        let mut drc_dec = DrcDecoder {
            functional_range: func_range,
            status: DrcDecStatus::NotInitialized,
            codec_mode: CodecMode::Undefined,
            reader: Box::new(Reader::new()),
            ..Default::default()
        };

        if (drc_dec.functional_range & DrcDecFunctionalRange::Selection) != 0 {
            drc_dec.sel_proc = Some(Box::new(SelectionProcess::new()));
            if let Some(sel_proc) = &mut drc_dec.sel_proc {
                sel_proc.init();
            }
            drc_dec
                .sel_proc_output
                .set_output_loudness(UNDEFINED_LOUDNESS_VALUE);
            drc_dec.is_sel_proc_input_diff = true;
        }
        if (drc_dec.functional_range & DrcDecFunctionalRange::Gain) != 0 {
            drc_dec.gain_dec = Some(Box::new(DrcGainDecoder::new()));
        }

        drc_dec
    }

    /// Pre-processing function for the gain decoder. It should be called before the 1st call of
    /// `self.process_time()` of a processing frame.
    pub fn preprocess(&mut self) -> Result<(), DrcDecError> {
        if self.status == DrcDecStatus::NotInitialized {
            return Err(DrcDecError::NotReady);
        }
        if (self.functional_range & DrcDecFunctionalRange::Gain) == 0 {
            return Err(DrcDecError::NotOk);
        }

        if let Some(drc_gain_dec) = &mut self.gain_dec {
            if self.status != DrcDecStatus::NewGainPayload {
                // No new gain payload was read, e.g. during concealment or flushing.
                // Generate DRC gains based on the stored DRC gains of last frames.
                let uni_drc_gain = self.reader.uni_drc_gain_mut();
                drc_gain_dec.conceal(uni_drc_gain)?;
            }
            drc_gain_dec.preprocess(
                self.reader.uni_drc_gain(),
                self.sel_proc_output.loudness_normalization_gain_db(),
                self.sel_proc_output.boost(),
                self.sel_proc_output.compress(),
            )?;

            self.status = DrcDecStatus::InterpolationPrepared;
        }

        Ok(())
    }

    /// Applies DRC gain sequences on the time domain in/out signal. The audio buffer must be
    /// de-interleaved and contain a single AAC frame (see `frame_size`).
    ///
    /// # Parameters
    ///
    /// - `drc_location`: The location where the uniDRC gain sequences can be found.
    /// - `delay_samples`: Number of delay samples to be considered for DRC processing.
    /// - `channel_offset`: Start index of physical (audio) channels.
    /// - `drc_channel_offset`: Start index of DRC channels.
    /// - `num_processed_channels`: Number of processed audio channels.
    /// - `deinterleaved_audio_buffer`: Frame of de-interleaved audio samples.
    /// - `time_data_channel_offset`: Offset that specifies how many samples are in a particular.
    ///   audio channel (from `deinterleaved_audio_buffer`).
    ///
    /// # Return
    ///
    /// - Result<(), DrcDecError>
    #[expect(clippy::too_many_arguments)]
    pub fn process_time(
        &mut self,
        drc_location: ProcessingLocation,
        delay_samples: u16,
        channel_offset: usize,
        drc_channel_offset: usize,
        num_processed_channels: usize,
        deinterleaved_audio_buffer: &mut [f32],
        time_data_channel_offset: usize,
    ) -> Result<(), DrcDecError> {
        if (self.functional_range & DrcDecFunctionalRange::Gain) == 0 {
            return Err(DrcDecError::NotOk);
        }
        if self.status != DrcDecStatus::InterpolationPrepared {
            return Err(DrcDecError::NotReady);
        }
        if let Some(drc_gain) = &mut self.gain_dec {
            drc_gain.process_time_domain(
                drc_location,
                delay_samples,
                channel_offset,
                drc_channel_offset,
                num_processed_channels,
                time_data_channel_offset,
                deinterleaved_audio_buffer,
            )?;
        }
        Ok(())
    }

    /// Reads MPEG-D loudness metadata, if provided in the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream data to read from.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`:
    ///   - Ok(_) => { Reading was successful. }
    ///   - Err(e) => {
    ///       - `DrcDecError::NotOk` if codec_mode is !`CodecMode::MpegD_Usac` or the selection
    ///         process could not be started.
    ///       - `DrcDecError::ParseError` if parsing failed. }
    pub fn read_loudness_info_set(&mut self, bs: &mut Bitstream) -> Result<(), DrcDecError> {
        if self.codec_mode == CodecMode::MpegD_Usac {
            match self.reader.loudness_info_set_mut().read(bs) {
                Ok(_) => {}
                Err(_e) => {
                    return Err(DrcDecError::ParseError);
                }
            }
        } else {
            return Err(DrcDecError::NotOk);
        }

        self.start_selection_process()?;

        Ok(())
    }

    /// Reads MPEG-D DRC metadata, if provided in the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream data to read from.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`:
    ///   - Ok(_) => { Reading was successful. }
    ///   - Err(e) => {
    ///       - `DrcDecError::NotOk` if the selection process could not be started.
    ///       - `DrcDecError::ParseError` if parsing failed.
    ///       - `DrcDecError::NotReady` if `DrcDecStatus::NotInitialized` }
    pub fn read_uni_drc(&mut self, bs: &mut Bitstream) -> Result<(), DrcDecError> {
        if self.status == DrcDecStatus::NotInitialized {
            return Err(DrcDecError::NotReady);
        }
        match self.reader.read(bs) {
            Ok(_) => {}
            Err(_e) => {
                return Err(DrcDecError::ParseError);
            }
        }

        self.start_selection_process()?;

        if self.reader.get_gain_status() {
            self.status = DrcDecStatus::NewGainPayload;
        }

        Ok(())
    }

    /// Reads MPEG-D DRC config (static) metadata, if provided in the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream data to read from.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`:
    ///   - Ok(_) => { Reading was successful. }
    ///   - Err(e) => {
    ///       - `DrcDecError::NotOk` if codec_mode is !`CodecMode::MpegD_Usac` or the selection
    ///         process could not be started.
    ///       - `DrcDecError::ParseError` if parsing failed. }
    pub fn read_uni_drc_config(&mut self, bs: &mut Bitstream) -> Result<(), DrcDecError> {
        if self.codec_mode == CodecMode::MpegD_Usac {
            match self.reader.uni_drc_config_mut().read(bs) {
                Ok(_) => {}
                Err(_e) => {
                    return Err(DrcDecError::ParseError);
                }
            }
        } else {
            return Err(DrcDecError::NotOk);
        }

        self.start_selection_process()?;

        Ok(())
    }

    /// Reads MPEG-D DRC (dynamic) gain metadata, if provided in the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream data to read from.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`:
    ///   - Ok(_) => { Reading was successful or `DrcDecStatus::NotInitialized`. }
    ///   - Err(e) => {
    ///       - `DrcDecError::ParseError` if parsing failed. }
    pub fn read_uni_drc_gain(&mut self, bs: &mut Bitstream) -> Result<(), DrcDecError> {
        if self.status == DrcDecStatus::NotInitialized {
            return Ok(());
        }
        match self.reader.read_uni_drc_gain(bs) {
            Ok(_) => {}
            Err(_e) => {
                return Err(DrcDecError::ParseError);
            }
        }
        if self.reader.get_gain_status() {
            self.status = DrcDecStatus::NewGainPayload;
        }
        Ok(())
    }

    /// Sets the `CodecMode`, if it is set for the first time. Changing `CodecMode` is not allowed
    /// if has already been set.
    ///
    /// # Parameters
    ///
    /// - `codec_mode`: Enum with the codec modes supported.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`:
    ///   - `Ok(_)` => The codec is set.,
    ///   - `Err(e)` => `DrcDecError::NotOk`, if codec mode is already set.
    pub fn set_codec_mode(&mut self, codec_mode: CodecMode) -> Result<(), DrcDecError> {
        if self.codec_mode == CodecMode::Undefined {
            self.codec_mode = codec_mode;

            if (self.functional_range & DrcDecFunctionalRange::Selection) != 0 {
                if let Some(sel_proc) = &mut self.sel_proc {
                    sel_proc.set_codec_mode(codec_mode)?;
                }
                self.is_sel_proc_input_diff = true;
            }
            if (self.functional_range & DrcDecFunctionalRange::Gain) != 0 {
                let delay_mode: DelayMode = DelayMode::Regular;

                let (is_time_domain_supported, subband_domain_supported) = match self.codec_mode {
                    CodecMode::Mpeg4_Aac | CodecMode::MpegD_Usac => (true, SubbandDomainMode::Off),
                    _ => (true, SubbandDomainMode::Off),
                };

                if let Some(gain_dec) = &mut self.gain_dec {
                    gain_dec.set_codec_dependent_parameters(
                        delay_mode,
                        is_time_domain_supported,
                        subband_domain_supported,
                    )?;
                }
            }
        }
        //  Don't allow changing codec_mode if has already been set.
        if self.codec_mode != codec_mode {
            return Err(DrcDecError::NotOk);
        }
        Ok(())
    }

    /// Sets the DRC channel gains, used by processing.
    ///
    /// # Parameters
    ///
    /// - `do_update_ln_gain`: Update the loudness normalization gains.
    /// - `num_channels`: Number of physical (audio) channels.
    /// - `channel_gain_db`: Channel gain (expressed in dB).
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`
    pub fn set_channel_gains(
        &mut self,
        do_update_ln_gain: bool,
        num_channels: usize,
        channel_gain_db: &mut [f32],
    ) -> Result<(), DrcDecError> {
        match &mut self.gain_dec {
            Some(drc_gain) => {
                if do_update_ln_gain {
                    let ln_gain_db = self.sel_proc_output.loudness_normalization_gain_db();
                    drc_gain.set_loudness_normalization_gain_db(ln_gain_db)?;
                }
                drc_gain.set_channel_gains(num_channels, channel_gain_db)?;
            }
            None => {
                return Err(DrcDecError::NotOk);
            }
        }
        Ok(())
    }

    /// Sets a specific parameter of `self`, by using `DrcDecParameters`.
    ///
    /// # Parameters
    ///
    /// - `request_type`: See `DrcDecParameters`.
    ///
    /// # Return
    ///
    /// - `Result<(), DrcDecError>`
    pub fn set_param(&mut self, request_type: DrcDecParameters) -> Result<(), DrcDecError> {
        if self.functional_range == DrcDecFunctionalRange::Gain {
            return Err(DrcDecError::ParamInvalid);
        }

        match &mut self.sel_proc {
            Some(sel_proc) => {
                let sel_param = SelProcUserParam::try_from(request_type)?;
                sel_proc.set_param(sel_param, Some(&mut self.is_sel_proc_input_diff))?;
            }
            None => {
                return Err(DrcDecError::ParamInvalid);
            }
        }

        // All parameters need a new start of the selection process.
        self.start_selection_process()?;

        Ok(())
    }
}
