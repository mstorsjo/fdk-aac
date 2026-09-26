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
//! AAC decoder output information
use super::ACFlags;
use crate::{
    aac_dec::drc::AacDrcPresentationMode,
    common::{
        aot::AudioObjectType, audio_channel_type::AudioChannelType,
        bs_element_id::ChannelElementId, channel_order::ChannelOrder,
    },
};

#[repr(C)]
#[derive(Clone, Copy, Default, Debug)]
pub struct OutputInfo {
    /// The sample rate in Hz of the decoded PCM audio signal.
    pub sampling_rate: u32,

    /// The frame size of the decoded PCM audio signal.
    pub frame_size: u16,

    /// The number of decoder output audio channels.
    pub num_channels: u8,

    /// The number of samples the output is additionally delayed by the decoder.
    pub output_delay: u32,

    /// Audio channel type of each output audio channel.
    pub channel_type: [AudioChannelType; 8],

    /// Audio channel index for each output audio channel.
    /// See ISO/IEC 13818-7:2005(E), 8.5.3.2 Explicit channel mapping using a
    /// program_config_element().
    pub channel_indices: [u8; 8],

    /// Audio output loudness in steps of -0.25 dB. Range: 0 (0 dBFS) to 231 (-57.75 dBFS).
    /// A value of -1 indicates that no loudness metadata is present.
    ///
    /// If loudness normalization is active, the value corresponds to the target
    /// loudness value set with `AAC_DRC_REFERENCE_LEVEL`.
    ///
    /// If loudness normalization is not active, the output loudness value
    /// corresponds to the loudness metadata given in the bitstream.
    /// Loudness metadata can originate from MPEG-4 DRC or MPEG-D DRC.
    pub output_loudness: i16,
}

impl OutputInfo {
    /// Creates a new instance of `OutputInfo`, initialized with default values.
    pub fn new() -> Self {
        Self::default()
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct MetadataInfo {
    /// DRC presentation mode. According to ETSI TS 101 154, this field indicates whether light
    /// (MPEG-4 Dynamic Range Control tool) or heavy compression (DVB heavy compression) dynamic
    /// range control shall take priority on the outputs.
    pub drc_presentation_mode: AacDrcPresentationMode,

    /// DRC program reference level. Defines the reference level below full-scale. It is
    /// quantized in steps of 0.25dB. The valid values range from 0 (0 dBFS) to 127 (-31.75 dBFS).
    /// It is used to reflect the average loudness of the audio in LKFS according to ITU-R BS
    /// 1770. If no level has been found in the bitstream the value is -1.
    pub drc_program_reference_level: i8,

    /// The 2 bit matrix mixdown index extracted from PCE.
    pub pce_matrix_mixdown_index: i8,

    /// Pseudo Surround flag extracted from PCE.
    pub pce_pseudo_surround_enable: bool,
}

impl Default for MetadataInfo {
    fn default() -> Self {
        Self {
            // Set program reference level to not indicated.
            drc_program_reference_level: -1,
            // Presentation mode not present.
            drc_presentation_mode: AacDrcPresentationMode::default(),
            // Matrix mixdown index from PCE not indicated.
            pce_matrix_mixdown_index: -1,
            // Pseudo surround flag from PCE not indicated.
            pce_pseudo_surround_enable: false,
        }
    }
}

impl MetadataInfo {
    /// Creates a default instance of `MetadataInfo`.
    pub fn new() -> Self {
        Default::default()
    }
}

/// This structure gives information about the currently decoded audio data.
/// All fields are read-only.
#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct StreamInfo {
    /// The number of bitstream elements.
    pub num_elements: u8,

    /// Bitstream element list.
    pub bs_element_list: [ChannelElementId; 10],

    /// Audio channel order (MPEG, WAV, CICP) provided at the decoder output.
    /// In case of MPEG-H, the CICP order is used always.
    pub ch_order: ChannelOrder,

    /// Sampling rate in Hz without SBR (from configuration info) divided by a (ELD) downscale
    /// factor if present.
    pub sample_rate: u32,

    /// Audio Object Type (from ASC): is set to the appropriate value for MPEG bitstreams (e.g. 2
    /// for AAC-LC).
    pub aot: AudioObjectType,

    /// Channel configuration (0: PCE-defined, 1: mono, 2: stereo, ...).
    pub channel_config: i32,

    /// Samples per frame for the AAC core (from ASC) divided by a (ELD) downscale factor if
    /// present. Typically this is (with a downscale factor of 1):
    /// 1024 or 960 for AAC-LC
    /// 512 or 480 for AAC-LD and AAC-ELD
    pub samples_per_frame: u16,

    /// The number of audio channels after AAC core processing (before PS or MPS processing).
    /// CAUTION: This is not necessarily the final number of output channels!
    pub num_channels: u8,

    /// Extension Audio Object Type (from ASC).
    pub ext_aot: AudioObjectType,

    /// Extension sampling rate in Hz (from ASC) divided by a (ELD) downscale factor if present.
    pub ext_sampling_rate: u32,

    /// Copy of internal flags. Only to be written by the decoder, and only to be read externally.
    pub flags: ACFlags,

    /// This is the number of total bytes that have passed through the decoder
    /// within the present function call.
    pub num_consumed_bytes: u32,
}

impl Default for StreamInfo {
    fn default() -> Self {
        Self {
            num_elements: 0,
            bs_element_list: Default::default(),
            ch_order: Default::default(),
            sample_rate: 0,
            aot: AudioObjectType::AotNone,
            channel_config: -1,
            samples_per_frame: 0,
            num_channels: 0,
            ext_aot: AudioObjectType::AotNone,
            ext_sampling_rate: 0,
            flags: ACFlags::empty(),
            num_consumed_bytes: 0,
        }
    }
}

impl StreamInfo {
    /// Returns new instance of `StreamInfo`.
    pub fn new() -> StreamInfo {
        StreamInfo::default()
    }
}

/// Gives information collected from the bitstream.
#[repr(C)]
#[derive(Default, Copy, Clone, Debug)]
pub struct BitstreamInfo {
    /// Gives information about the currently decoded audio data.
    pub stream_info: Option<StreamInfo>,
    /// Gives information about the metadata present for the currently decoded audio data.
    pub metadata_info: Option<MetadataInfo>,
}

impl BitstreamInfo {
    /// Creates a new instance of `BitstreamInfo`, initialized with default values.
    pub fn new() -> Self {
        Default::default()
    }

    /// Gets the stream information, if available.
    pub fn stream_info(&self) -> Option<StreamInfo> {
        self.stream_info
    }

    /// Gets the metadata information, if available.
    pub fn metadata_info(&self) -> Option<MetadataInfo> {
        self.metadata_info
    }

    /// Sets the stream information.
    pub fn set_stream_info(&mut self, si: Option<StreamInfo>) {
        match si {
            Some(s) => {
                self.stream_info = Some(s);
            }
            None => {
                self.stream_info = None;
            }
        }
    }

    /// Sets the metadata information.
    pub fn set_metadata_info(&mut self, mi: Option<MetadataInfo>) {
        match mi {
            Some(m) => {
                self.metadata_info = Some(m);
            }
            None => {
                self.metadata_info = None;
            }
        }
    }
}

/// Describes the information of the current AAC frame.
#[repr(C)]
#[derive(Default, Copy, Clone, Debug)]
pub struct DecoderInfo {
    /// Describes the audio output information of the current AAC frame.
    pub output_info: OutputInfo,
    /// Describes the bitstream information of the current AAC frame.
    pub bs_info: BitstreamInfo,
}

impl DecoderInfo {
    /// Creates a new instance of `DecoderInfo`, initialized with default values.
    pub fn new() -> Self {
        Self {
            output_info: OutputInfo::new(),
            bs_info: BitstreamInfo::new(),
        }
    }
}
