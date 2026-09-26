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
//! Transport decoder callback functions

use super::{asc::AudioSpecificConfig, error_codes::TpDecoderError};
use crate::{
    common::{
        aot::AudioObjectType, bitstream::Bitstream, bs_element_id::ChannelElementId,
        enums::StereoCfgIndex,
    },
    tp_dec::ReconfigState,
    tp_dec::TransportDec,
};

use core::fmt::Debug;
use std::{cell::RefCell, ops::DerefMut, rc::Rc};

/// Transport decoder callback trait.
pub trait TpDecCb: Debug {
    fn decode_frame(&mut self, tp_dec: &mut TransportDec) -> Result<(), TpDecoderError>;

    fn free_memory(&mut self);

    fn update_config(
        &mut self,
        asc: &AudioSpecificConfig,
        config_mode: ReconfigState,
        is_config_changed: &mut bool,
    ) -> Result<(), TpDecoderError>;

    #[expect(clippy::too_many_arguments)]
    fn ssc_parser(
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
    ) -> Result<(), TpDecoderError>;

    #[expect(clippy::too_many_arguments)]
    fn sbr_parser(
        &mut self,
        bs: &mut Bitstream,
        sample_rate_in: u32,
        sample_rate_out: u32,
        samples_per_frame: u16,
        core_codec: AudioObjectType,
        element_id: ChannelElementId,
        element_index: usize,
        is_harmonic_sbr: bool,
        config_mode: ReconfigState,
        is_config_changed: &mut bool,
        down_scale_factor: u8,
    ) -> Result<(), TpDecoderError>;

    fn uni_drc_parser(
        &mut self,
        bs: Option<&mut Bitstream>,
        payload_type: u8,
        aot: AudioObjectType,
    ) -> Result<(), TpDecoderError>;
}

/// Transport decoder struct holding decoder callbacks and its data.
#[repr(C)]
#[derive(Clone, Debug)]
pub struct TpDecCallBacks {
    pub cb_data: Option<Rc<RefCell<dyn TpDecCb>>>,
    /// The enum indicates if the callback shall work in memory
    /// allocation mode or in config change detection mode.
    config_mode: ReconfigState,
    /// The flag will be set if at least one aac config parameter has changed
    /// that requires a memory reconfiguration, otherwise it will be cleared.
    is_aac_config_changed: bool,
    /// The flag will be set if at least one sbr config parameter has changed
    /// that requires a memory reconfiguration, otherwise it will be cleared.
    is_sbr_config_changed: bool,
    /// The flag will be set if at least one sac config parameter has changed
    /// that requires a memory reconfiguration, otherwise it will be cleared.
    is_sac_config_changed: bool,
}

impl Default for TpDecCallBacks {
    fn default() -> Self {
        TpDecCallBacks {
            cb_data: None,
            config_mode: ReconfigState::None,
            is_aac_config_changed: false,
            is_sbr_config_changed: false,
            is_sac_config_changed: false,
        }
    }
}

impl TpDecCallBacks {
    /// Creates a new instance of `TpDecCallBacks`.
    ///
    /// # Return
    ///
    /// A new instance of `TpDecCallBacks` with default values.
    pub fn new() -> Self {
        Self::default()
    }

    /// Decodes a frame by invoking a callback function.
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    pub fn decode_frame_callback(
        &mut self,
        tp_dec: &mut TransportDec,
    ) -> Result<(), TpDecoderError> {
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();
            handle.decode_frame(tp_dec)
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }

    /// Frees config dependent memory of `self.cb_data` by invoking a callback function.
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    pub fn free_mem_callback(&mut self) -> Result<(), TpDecoderError> {
        self.is_aac_config_changed = true;
        self.is_sbr_config_changed = true;
        self.is_sac_config_changed = true;
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();
            handle.free_memory();
            Ok(())
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }

    /// Checks if the configuration has changed.
    ///
    /// # Return
    ///
    /// `true` if configuration has changed, otherwise `false`.
    pub fn is_config_changed(&mut self) -> bool {
        self.is_aac_config_changed || self.is_sbr_config_changed || self.is_sac_config_changed
    }

    /// Registers address of the owner which will use the `TpDecCallBacks`.
    ///
    /// # Parameters
    ///
    /// - `data`: `TpDecCb` trait object which will use the `TpDecCallBacks`.
    pub fn register_callback_data(&mut self, data: Option<Rc<RefCell<dyn TpDecCb>>>) {
        self.cb_data = data;
    }

    /// Parses `SBR header` information by invoking a callback function.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `core_codec`: Audio object type of core codec.
    /// - `element_id`: MPEG-4 audio channel type (Element ID).
    /// - `sampling_rate_in`: Input sampling rate value of SBR decoder.
    /// - `sampling_rate_out`: Output sampling rate value of SBR decoder.
    /// - `samples_per_frame`: Number of samples per frame.
    /// - `element_index`: SBR element index value.
    /// - `downscale_factor`: ELD downscale factor value.
    /// - `harmonic_sbr`: Flag Signals the usage of the harmonic patching for the SBR.
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    #[expect(clippy::too_many_arguments)]
    pub fn sbr_callback(
        &mut self,
        bs: &mut Bitstream,
        core_codec: AudioObjectType,
        element_id: ChannelElementId,
        sample_rate_in: u32,
        sample_rate_out: u32,
        samples_per_frame: u16,
        element_index: usize,
        downscale_factor: u8,
        is_harmonic_sbr: bool,
    ) -> Result<(), TpDecoderError> {
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();

            handle.sbr_parser(
                bs,
                sample_rate_in,
                sample_rate_out,
                samples_per_frame,
                core_codec,
                element_id,
                element_index,
                is_harmonic_sbr,
                self.config_mode,
                &mut self.is_sbr_config_changed,
                downscale_factor,
            )
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }

    /// Sets the configuration mode of the `TpDecCallBacks`.
    ///
    /// # Parameters
    ///
    /// - `config_mode`: The configuration mode to set.
    pub fn set_config_mode(&mut self, config_mode: ReconfigState) {
        self.config_mode = config_mode;
        if config_mode == ReconfigState::DetCfgChange {
            self.is_aac_config_changed = false;
            self.is_sbr_config_changed = false;
            self.is_sac_config_changed = false;
        }
    }

    /// Parses `spatial specific config (SSC)` by invoking a callback function.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `core_codec`: Audio object type of core codec.
    /// - `sampling_rate`: Sampling rate value.
    /// - `frame_size`: Frame length for core codec.
    /// - `num_channels`: Number of channels.
    /// - `stereo_config_index`: USAC stereo config index value.
    /// - `core_sbr_frame_length_index`: Core SBR frame length index value.
    /// - `config_bytes`: Configuration length in bytes.
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    #[expect(clippy::too_many_arguments)]
    pub fn ssc_callback(
        &mut self,
        bs: &mut Bitstream,
        core_codec: AudioObjectType,
        sampling_rate: u32,
        frame_size: u16,
        num_channels: u8,
        stereo_config_index: StereoCfgIndex,
        core_sbr_frame_length_index: u8,
        config_bytes: u32,
    ) -> Result<(), TpDecoderError> {
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();

            handle.ssc_parser(
                bs,
                core_codec,
                sampling_rate,
                frame_size,
                num_channels,
                stereo_config_index,
                core_sbr_frame_length_index,
                config_bytes,
                self.config_mode,
                self.is_sac_config_changed,
            )
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }

    /// Parses `uniDrcConfig` and `LoudnessInfoSet` information by invoking
    /// a callback function.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `aot`: Audio object type of core codec.
    /// - `payload_type`: Type of payload `uniDrcConfig` (or) `LoudnessInfoSet`.
    ///
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    pub fn uni_drc_callback(
        &mut self,
        bs: Option<&mut Bitstream>,
        payload_type: u8,
        aot: AudioObjectType,
    ) -> Result<(), TpDecoderError> {
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();

            handle.uni_drc_parser(bs, payload_type, aot)
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }

    /// Updates `self.cb_data` by invoking a callback function, in case of `AudioSpecificConfig`
    /// changes.
    ///
    /// # Parameters
    ///
    /// - `asc`: Audio specific configuration to be used for updating.
    ///
    /// # Return
    /// - Result<(), TpDecoderError>
    pub fn update_config_callback(
        &mut self,
        asc: &AudioSpecificConfig,
    ) -> Result<(), TpDecoderError> {
        if let Some(handle_rc) = &self.cb_data {
            let mut handle_refcell = handle_rc.borrow_mut();
            let handle = handle_refcell.deref_mut();

            handle.update_config(asc, self.config_mode, &mut self.is_aac_config_changed)
        } else {
            Err(TpDecoderError::UnknownError)
        }
    }
}
