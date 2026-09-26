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
use aac::aac_dec::{MetadataInfo, OutputInfo, StreamInfo};

/// Returns a default `OutputInfo` instance.
///
/// # Safety
///
/// Since the data is passed by value across the FFI boundary, it is crucial to ensure that
/// the memory layout of the Rust-defined struct matches the struct definition in C.
#[no_mangle]
pub unsafe extern "C" fn CAacDecoderOutputInfo_default() -> OutputInfo {
    OutputInfo::default()
}

/// Returns a default `StreamInfo` instance.
///
/// # Safety
///
/// Since the data is passed by value across the FFI boundary, it is crucial to ensure that
/// the memory layout of the Rust-defined struct matches the struct definition in C.
#[no_mangle]
pub unsafe extern "C" fn CAacDecoderStreamInfo_default() -> StreamInfo {
    StreamInfo::default()
}

/// Returns a default `MetadataInfo` instance.
///
/// # Safety
///
/// Since the data is passed by value across the FFI boundary, it is crucial to ensure that
/// the memory layout of the Rust-defined struct matches the struct definition in C.
#[no_mangle]
pub unsafe extern "C" fn CAacDecoderMetadataInfo_default() -> MetadataInfo {
    MetadataInfo::default()
}

// --------  Tests --------------------------------------------------------- //

// Tests of struct size and layout of the API structs shared btw. Rust and C.

// OutputInfo
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of OutputInfo"][::std::mem::size_of::<OutputInfo>() - 56usize];
    ["Alignment of OutputInfo"][::std::mem::align_of::<OutputInfo>() - 4usize];
    ["Offset of field: OutputInfo::sampling_rate"]
        [::std::mem::offset_of!(OutputInfo, sampling_rate) - 0usize];
    ["Offset of field: OutputInfo::frame_size"]
        [::std::mem::offset_of!(OutputInfo, frame_size) - 4usize];
    ["Offset of field: OutputInfo::num_channels"]
        [::std::mem::offset_of!(OutputInfo, num_channels) - 6usize];
    ["Offset of field: OutputInfo::output_delay"]
        [::std::mem::offset_of!(OutputInfo, output_delay) - 8usize];
    ["Offset of field: OutputInfo::channel_type"]
        [::std::mem::offset_of!(OutputInfo, channel_type) - 12usize];
    ["Offset of field: OutputInfo::channel_indices"]
        [::std::mem::offset_of!(OutputInfo, channel_indices) - 44usize];
    ["Offset of field: OutputInfo::output_loudness"]
        [::std::mem::offset_of!(OutputInfo, output_loudness) - 52usize];
};

// StreamInfo
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of StreamInfo"][::std::mem::size_of::<StreamInfo>() - 80usize];
    ["Alignment of StreamInfo"][::std::mem::align_of::<StreamInfo>() - 4usize];
    ["Offset of field: StreamInfo::num_elements"]
        [::std::mem::offset_of!(StreamInfo, num_elements) - 0usize];
    ["Offset of field: StreamInfo::bs_element_list"]
        [::std::mem::offset_of!(StreamInfo, bs_element_list) - 4usize];
    ["Offset of field: StreamInfo::ch_order"]
        [::std::mem::offset_of!(StreamInfo, ch_order) - 44usize];
    ["Offset of field: StreamInfo::sample_rate"]
        [::std::mem::offset_of!(StreamInfo, sample_rate) - 48usize];
    ["Offset of field: StreamInfo::aot"][::std::mem::offset_of!(StreamInfo, aot) - 52usize];
    ["Offset of field: StreamInfo::channel_config"]
        [::std::mem::offset_of!(StreamInfo, channel_config) - 56usize];
    ["Offset of field: StreamInfo::samples_per_frame"]
        [::std::mem::offset_of!(StreamInfo, samples_per_frame) - 60usize];
    ["Offset of field: StreamInfo::num_channels"]
        [::std::mem::offset_of!(StreamInfo, num_channels) - 62usize];
    ["Offset of field: StreamInfo::ext_aot"][::std::mem::offset_of!(StreamInfo, ext_aot) - 64usize];
    ["Offset of field: StreamInfo::ext_sampling_rate"]
        [::std::mem::offset_of!(StreamInfo, ext_sampling_rate) - 68usize];
    ["Offset of field: StreamInfo::flags"][::std::mem::offset_of!(StreamInfo, flags) - 72usize];
    ["Offset of field: StreamInfo::num_consumed_bytes"]
        [::std::mem::offset_of!(StreamInfo, num_consumed_bytes) - 76usize];
};

// MetadataInfo
#[allow(clippy::unnecessary_operation, clippy::identity_op)]
const _: () = {
    ["Size of MetadataInfo"][::std::mem::size_of::<MetadataInfo>() - 4usize];
    ["Alignment of MetadataInfo"][::std::mem::align_of::<MetadataInfo>() - 1usize];
    ["Offset of field: MetadataInfo::drc_presentation_mode"]
        [::std::mem::offset_of!(MetadataInfo, drc_presentation_mode) - 0usize];
    ["Offset of field: MetadataInfo::drc_program_reference_level"]
        [::std::mem::offset_of!(MetadataInfo, drc_program_reference_level) - 1usize];
    ["Offset of field: MetadataInfo::pce_matrix_mixdown_index"]
        [::std::mem::offset_of!(MetadataInfo, pce_matrix_mixdown_index) - 2usize];
    ["Offset of field: MetadataInfo::pce_pseudo_surround_enable"]
        [::std::mem::offset_of!(MetadataInfo, pce_pseudo_surround_enable) - 3usize];
};
