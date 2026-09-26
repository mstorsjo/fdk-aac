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
pub mod error_codes;
pub mod output_info;
pub mod params;

// Opaque instance for FFI
#[repr(C)]
#[allow(non_camel_case_types)]
pub struct AAC_DECODER_INSTANCE {}

// Re-exports
pub use crate::aac_dec::params::ParamBind as AACDEC_PARAM;
pub use aac::aac_dec::AacDecoderError as AAC_DECODER_ERROR;
pub use aac::aac_dec::AacDrcParameterHandling as AAC_DRC_DEFAULT_PRESENTATION_MODE_OPTIONS;
pub use aac::aac_dec::AacDrcPresentationMode;
pub use aac::aac_dec::AacMdProfile as AAC_MD_PROFILE;
pub use aac::aac_dec::AudioChannelType as AUDIO_CHANNEL_TYPE;
pub use aac::aac_dec::AudioObjectType as AUDIO_OBJECT_TYPE;
pub use aac::aac_dec::ChannelElementId as MP4_ELEMENT_ID;
pub use aac::aac_dec::ChannelOrder as CHANNEL_ORDER;
pub use aac::aac_dec::TransportType as TRANSPORT_TYPE;
pub use aac::aac_dec::{MetadataInfo, OutputInfo, StreamInfo};
pub use aac::aac_dec::{MAX_ANC_ELEMENTS, MAX_CHANNELS, MAX_CONF_SIZE, TRANSPORTDEC_INBUF_SIZE};

// Imports
use crate::aac_dec::params::ParamBind;
use aac::aac_dec::{
    AacDecoderError, AacDecoderInstance, AacDrcParameterHandling, AacMdProfile, ChannelOrder,
    ConcealmentMethod, DrcEffectTypeRequest, DualChannelMode, LimiterMode, Param, TransportType,
};

/// Creates a new `AacDecoderInstance` and returns a non-null raw pointer to the caller.
///
/// # Parameters
///
/// - `transport_fmt`: The transport format to be used by the AAC decoder.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `transport_fmt` is a valid value of `TransportType`.
/// - The caller must call `aacDecoder_Close()` with the exact raw pointer returned by this function
///   to properly deallocate the `AacDecoderInstance` when the decoding process is complete.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Open(
    transport_fmt: TransportType,
) -> *mut AAC_DECODER_INSTANCE {
    let aac_decoder_instance = Box::new(AacDecoderInstance::new(transport_fmt));
    Box::into_raw(aac_decoder_instance) as *mut AAC_DECODER_INSTANCE
}

/// Retrieves ancillary data from the decoder, if available. The caller selects
/// the specific ancillary data block using the provided index. The extracted
/// ancillary data is copied to the provided buffer, and the length of the
/// data is updated in `size` (in bytes).
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `index`: An index value that specifies which ancillary data block to retrieve. It should be in
///   the range `[0, MAX_ANC_ELEMENTS)`.
/// - `buffer`: A pointer to a contiguous memory block where the extracted ancillary data will be
///   copied.
/// - `buffer_len`: The size of the buffer in bytes, indicating how much space is available for the
///   ancillary data.
/// - `size`: A pointer to a variable where the length of the extracted ancillary data will be
///   stored.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to an `AacDecoderInstance`.
/// - `index` is within the valid range `[0, MAX_ANC_ELEMENTS)`.
/// - `buffer` is a valid, non-null pointer to a contiguous memory block of `buffer_len` bytes.
/// - `buffer_len` is greater than zero.
/// - `size` is a valid, non-null pointer to a `u32`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_AncDataGet(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    index: u32,
    buffer: *mut u8,
    buffer_len: u32,
    size: *mut u32,
) -> AacDecoderError {
    // Input param checks.
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if buffer.is_null() || size.is_null() || buffer_len == 0 {
        return AacDecoderError::OutOfMemory;
    }
    if index > MAX_ANC_ELEMENTS as u32 {
        return AacDecoderError::SetParamFail;
    }

    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };
    let ret_val: Result<usize, AacDecoderError> = aac_decoder
        .anc_data(index.try_into().unwrap(), unsafe {
            std::slice::from_raw_parts_mut(buffer, buffer_len.try_into().unwrap())
        });

    unsafe {
        let (anc_data_len, ret_val) = match ret_val {
            Err(e) => (0, e),
            Ok(anc_len) => (anc_len.try_into().unwrap(), AacDecoderError::Ok),
        };
        *size = anc_data_len;
        ret_val
    }
}

/// Sets one single decoder parameter.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `param`: The parameter to be set.
/// - `value`: The value to set for the specified parameter.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
/// - `param` is a valid parameter type.
/// - `value` is a valid value for the parameter specified by `param`.
///
///  # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_SetParam(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    param: ParamBind,
    value: i32,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };
    match param {
        ParamBind::PcmDualChannelOutputMode => {
            if let Ok(dual_ch_mode) = DualChannelMode::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::PcmDualChannelOutputMode(dual_ch_mode))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmMinOutputChannels => {
            if let Ok(ch_out_min) = u8::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::PcmMinOutputChannels(ch_out_min))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmMaxOutputChannels => {
            if let Ok(ch_out_max) = u8::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::PcmMaxOutputChannels(ch_out_max))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmOutputChannelMapping => {
            if let Err(result) =
                aac_decoder.set_param(Param::PcmOutputChannelMapping(ChannelOrder::from(value)))
            {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::PcmLimiterEnable => {
            if let Ok(limiter_mode) = LimiterMode::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::PcmLimiterEnable(limiter_mode)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmLimiterAttackTime => {
            if let Ok(limiter_attack_time) = u8::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::PcmLimiterAttackTime(limiter_attack_time))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmLimiterReleaseTime => {
            if let Ok(limiter_release_time) = u8::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::PcmLimiterReleaseTime(limiter_release_time))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::MetadataProfile => {
            if let Ok(metadata_profile) = AacMdProfile::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::MetadataProfile(metadata_profile))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::MetadataExpiryTime => {
            if let Ok(metadata_expiary_time) = u16::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::MetadataExpiryTime(metadata_expiary_time))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::ConcealMethod => {
            if let Err(result) =
                aac_decoder.set_param(Param::ConcealMethod(ConcealmentMethod::from(value)))
            {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::DrcBoostFactor => {
            if let Ok(drc_boost_factor) = u8::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::DrcBoostFactor(drc_boost_factor))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::DrcAttenuationFactor => {
            if let Ok(attenuation_factor) = u8::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::DrcAttenuationFactor(attenuation_factor))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::DrcReferenceLevel => {
            if let Ok(reference_level) = i8::try_from(value) {
                if let Err(result) =
                    aac_decoder.set_param(Param::DrcReferenceLevel(reference_level))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::DrcHeavyCompression => {
            if let Err(result) = aac_decoder.set_param(Param::DrcHeavyCompression(value != 0)) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::DrcDefaultPresentationMode => {
            if let Err(result) = aac_decoder.set_param(Param::DrcDefaultPresentationMode(
                AacDrcParameterHandling::from(value),
            )) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::DrcEncTargetLevel => {
            if let Ok(target_level) = i8::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::DrcEncTargetLevel(target_level)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::UnidrcSetEffect => {
            if let Err(result) = aac_decoder.set_param(Param::UnidrcSetEffect(
                DrcEffectTypeRequest::from(value as isize),
            )) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::UnidrcAlbumMode => {
            if let Err(result) = aac_decoder.set_param(Param::UnidrcAlbumMode(value != 0)) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::TpdecParamIgnoreBufferFullness => {
            if let Err(result) =
                aac_decoder.set_param(Param::TpdecParamIgnoreBufferFullness(value != 0))
            {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::TpdecCheckTwoSyncs => {
            if let Err(result) = aac_decoder.set_param(Param::TpdecCheckTwoSyncs(value != 0)) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::TargetLayoutCicp => {
            if let Ok(layout_cicp) = u8::try_from(value) {
                if let Err(result) = aac_decoder.set_param(Param::TargetLayoutCicp(layout_cicp)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        } //_ => AacDecoderError::SetParamFail,
    }
}

/// Explicitly configures the decoder by parsing an Audio Specific Config (ASC) or a
/// StreamMuxConfig (SMC) in binary format. The config lies in the `conf` buffer and
/// has `length` bytes in size.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `conf`: A pointer to a buffer containing the binary configuration data (ASC or SMC).
/// - `length`: The size of the `conf` buffer in bytes (Max value: MAX_CONF_SIZE).
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
/// - `conf` is a valid, non-null pointer to the buffer where the configuration (`ASC` or `SMC`)
///   will be stored in binary format.
/// - The `conf` buffer must have at least `length` bytes.
/// - `length` must be in the range `[1, MAX_CONF_SIZE]`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_ConfigRaw(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    conf: *mut u8,
    length: u32,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if conf.is_null() || !(1..MAX_CONF_SIZE as u32).contains(&length) {
        return AacDecoderError::OutOfMemory;
    }

    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };

    if let Err(e) = aac_decoder
        .config_raw(unsafe { std::slice::from_raw_parts_mut(conf, length.try_into().unwrap()) })
    {
        e
    } else {
        AacDecoderError::Ok
    }
}

/// Fills the AAC decoder's internal input buffer with bitstream data from the external input
/// buffer. Updates `bytes_valid` with the number of bytes remaining that have not yet been copied
/// to the internal buffer.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `p_buffer`: A pointer to a buffer containing the AAC bitstream data to be filled into the
///   decoder's internal input buffer.
/// - `buffer_size`: The size of the `p_buffer` in bytes.
/// - `bytes_valid`: A pointer to a variable that will be updated with the number of bytes remaining
///   in the `p_buffer` that have not yet been copied to the internal buffer.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
/// - `p_buffer` is a valid, non-null pointer to a contiguous memory block of at least `buffer_size`
///   bytes.
/// - `bytes_valid` is a valid, non-null pointer to a `u32`.
/// - The value of `*bytes_valid` should be less than or equal to `buffer_size`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Fill(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    p_buffer: *const u8,
    buffer_size: u32,
    bytes_valid: *mut u32,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if p_buffer.is_null() || bytes_valid.is_null() {
        return AacDecoderError::OutOfMemory;
    }
    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };

    let ret_val: Result<usize, AacDecoderError> = aac_decoder.fill(
        unsafe { std::slice::from_raw_parts(p_buffer, buffer_size.try_into().unwrap()) },
        unsafe {
            {
                *bytes_valid as usize
            }
        },
    );

    unsafe {
        let (valid_bytes, ret_val) = match ret_val {
            Err(e) => (0, e),
            Ok(bytes) => (bytes.try_into().unwrap(), AacDecoderError::Ok),
        };
        *bytes_valid = valid_bytes as u32;
        ret_val
    }
}

/// Signals an input bit stream data discontinuity.
/// Resyncs any internals as necessary. Clears all signal delay lines and history
/// buffers. This can cause discontinuities in the output signal.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Interrupt(aac_dec: *mut AacDecoderInstance) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    let aac_decoder = unsafe { &mut *aac_dec };
    if let Err(e) = aac_decoder.interrupt() {
        e
    } else {
        AacDecoderError::Ok
    }
}

/// Clears internal bit stream buffer of transport layers.
/// The decoder starts decoding at new data passed after this event
/// and any previous bit stream data is discarded.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Clear(aac_dec: *mut AacDecoderInstance) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    let aac_decoder = unsafe { &mut *aac_dec };
    if let Err(e) = aac_decoder.clear() {
        e
    } else {
        AacDecoderError::Ok
    }
}

/// Decodes an AAC frame. The decoded output signal is stored in a buffer pointed to by
/// `p_time_data`.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `p_time_data`: A pointer to a buffer where the decoded PCM audio samples will be stored.
/// - `time_data_size`: The size of the `p_time_data` buffer in number of `f32` samples.
/// - `p_output_info`: A pointer to an `OutputInfo` structure that will be filled with information
///   about the decoded output.
/// - `p_stream_info`: A pointer to a `StreamInfo` structure that will be filled with information
///   about the input stream. Can be `NULL` if not needed.
/// - `p_metadata_info`: A pointer to a `MetadataInfo` structure that will be filled with metadata
///   information. Can be `NULL` if for example, the bitstream contains no metadata or not needed.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
/// - `p_time_data` is a valid, non-null pointer to a contiguous memory block with at least `number
///   of audio channels` * `output samples per frame` float samples. The data within the block must
///   be aligned to the `f32` datatype.
/// - `p_output_info` is a valid, non-null pointer to `OutputInfo.
/// - `p_stream_info` is a valid, non-null pointer to `StreamInfo` (if used).
/// - `p_metadata_info` is a valid, non-null pointer to `MetadataInfo` (if used).
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Decode(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    p_time_data: *mut f32,
    time_data_size: u32,
    p_output_info: *mut OutputInfo,
    p_stream_info: *mut StreamInfo,
    p_metadata_info: *mut MetadataInfo,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if p_time_data.is_null() || time_data_size == 0 || p_output_info.is_null() {
        return AacDecoderError::OutOfMemory;
    }

    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };

    let error;
    match aac_decoder.decode(unsafe {
        std::slice::from_raw_parts_mut(p_time_data, time_data_size.try_into().unwrap())
    }) {
        Ok(dinfo) => {
            error = AacDecoderError::Ok;
            unsafe {
                *p_output_info = dinfo.output_info;

                if let Some(stream_info) = dinfo.bs_info.stream_info() {
                    if !p_stream_info.is_null() {
                        *p_stream_info = stream_info;
                    }
                };
                if let Some(metadata_info) = dinfo.bs_info.metadata_info() {
                    if !p_metadata_info.is_null() {
                        *p_metadata_info = metadata_info;
                    }
                };
            }
        }
        Err((e, dinfo)) => {
            error = e;
            unsafe {
                *p_output_info = dinfo.output_info;
            }
        }
    }
    error
}

/// Deallocates an `AacDecoderInstance`.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to an `AacDecoderInstance`.
/// - The caller is responsible for passing the exact raw pointer created previously by calling
///   `aacDecoder_Open()`. Failing to call this function at the end of the decoding process may
///   result in memory leaks.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Close(aac_dec: *mut AAC_DECODER_INSTANCE) {
    if !aac_dec.is_null() {
        let _ = unsafe { Box::from_raw(aac_dec as *mut AacDecoderInstance) };
    }
}

/// Triggers the built-in error concealment to generate substitute signal for one lost frame. New
/// input data will not be considered.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `p_time_data`: A pointer to a buffer where the concealed PCM audio samples will be stored.
/// - `time_data_size`: The size of the `p_time_data` buffer in number of `f32` samples.
/// - `p_output_info`: A pointer to an `OutputInfo` structure that will be filled with information
///   about the concealed output.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to `AacDecoderInstance`.
/// - `p_time_data` is a valid, non-null pointer to a contiguous memory block of at least `number of
///   audio channels * output samples per frame` float samples. The data within the block must be
///   aligned to the `f32` datatype.
/// - `p_output_info` is a valid, non-null pointer to `OutputInfo`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Conceal(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    p_time_data: *mut f32,
    time_data_size: u32,
    p_output_info: *mut OutputInfo,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if p_output_info.is_null() {
        return AacDecoderError::OutOfMemory;
    }
    if p_time_data.is_null() || time_data_size == 0 {
        return AacDecoderError::OutOfMemory;
    }
    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };

    match aac_decoder.conceal(unsafe {
        std::slice::from_raw_parts_mut(p_time_data, time_data_size.try_into().unwrap())
    }) {
        Ok(out_info) => {
            unsafe { *p_output_info = out_info };
            AacDecoderError::Ok
        }
        Err((e, out_info)) => {
            unsafe { *p_output_info = out_info };
            e
        }
    }
}

/// Flushes the decoder to retrieve all delayed audio without processing any new input data.
///
/// # Parameters
///
/// - `aac_dec`: A mutable raw pointer to a valid `AacDecoderInstance`.
/// - `p_time_data`: A pointer to a buffer where the drained PCM audio samples will be stored.
/// - `time_data_size`: The size of the `p_time_data` buffer in number of `f32` samples.
/// - `p_output_info`: A pointer to an `OutputInfo` structure that will be filled with information
///   about the drained output.
///
/// # Safety
///
/// The caller must ensure the following:
/// - `aac_dec` is a valid, non-null pointer to an `AacDecoderInstance`.
/// - `p_time_data` is a valid, non-null pointer to a contiguous memory block with at least `number
///   of audio channels * output samples per frame` float samples. The data within the block must be
///   aligned to the `f32` datatype.
/// - `p_output_info` is a valid, non-null pointer to `OutputInfo`.
///
/// # Return
///
/// - `AacDecoderError`.
#[no_mangle]
pub unsafe extern "C" fn aacDecoder_Drain(
    aac_dec: *mut AAC_DECODER_INSTANCE,
    p_time_data: *mut f32,
    time_data_size: u32,
    p_output_info: *mut OutputInfo,
) -> AacDecoderError {
    if aac_dec.is_null() {
        return AacDecoderError::InvalidHandle;
    }
    if p_output_info.is_null() {
        return AacDecoderError::OutOfMemory;
    }
    if p_time_data.is_null() || time_data_size == 0 {
        return AacDecoderError::OutOfMemory;
    }
    let aac_decoder = unsafe { &mut *(aac_dec as *mut AacDecoderInstance) };

    match aac_decoder.drain(unsafe {
        std::slice::from_raw_parts_mut(p_time_data, time_data_size.try_into().unwrap())
    }) {
        Ok(out_info) => {
            unsafe { *p_output_info = out_info };
            AacDecoderError::Ok
        }
        Err((e, out_info)) => {
            unsafe { *p_output_info = out_info };
            e
        }
    }
}

#[cfg(test)]
mod tests {
    use aac::aac_dec::{
        aacdecoder::AacDecoder, drc::AacDrcParams, params::Params, AacDecoderInstance,
    };

    #[test]
    fn size_of_drc_params_struct() {
        const SIZE_OF_DRC_PARAMS_STRUCT_C: usize = 36;

        assert_eq!(
            std::mem::size_of::<AacDrcParams>(),
            SIZE_OF_DRC_PARAMS_STRUCT_C
        );
    }

    #[test]
    fn size_of_aac_dec_params_struct() {
        const SIZE_OF_C_AAC_DEC_PARAMS_STRUCT: usize = 380;

        assert_eq!(
            std::mem::size_of::<Params>(),
            SIZE_OF_C_AAC_DEC_PARAMS_STRUCT
        );
    }

    #[test]
    fn size_of_aac_decoder_struct() {
        let size_of_c_aac_decoder_struct = if cfg!(target_pointer_width = "64") {
            9112
        } else {
            8952
        };
        assert_eq!(
            std::mem::size_of::<AacDecoder>(),
            size_of_c_aac_decoder_struct
        );
    }

    #[test]
    fn size_of_aac_decoder_instance_struct() {
        const SIZE_OF_AAC_DECODER_STRUCT_C: usize = 408;

        if cfg!(target_pointer_width = "64") {
            assert_eq!(
                std::mem::size_of::<AacDecoderInstance>(),
                SIZE_OF_AAC_DECODER_STRUCT_C
            );
        }
    }
}
