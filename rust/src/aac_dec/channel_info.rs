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
//! Individual channel stream (ICS)

use crate::aac_dec::constants::*;
use crate::aac_dec::error_codes::AacDecoderError;
use crate::aac_dec::sr_info::SamplingRateInfo;
use crate::common::bitstream::Bitstream;
use crate::common::enums::WindowShape;
use crate::common::flags::ACFlags;

/// Defines the block type
#[repr(C)]
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub enum BlockType {
    Long = 0,
    Start = 1,
    Short = 2,
    Stop = 3,
}

/// Define Long as the default BlockType
impl Default for BlockType {
    fn default() -> Self {
        BlockType::Long
    }
}

/// Channel_Info
///
/// The Channel_Info holds information of the ICS (individual
/// channel stream), i.e information about scale factors and windows.
///
/// # Examples
///
/// Get an instance of IcsInfo and fill it with information from
/// a bitstream.
///
/// ```
/// use aac::aac_dec::channel_info::IcsInfo;
/// use aac::aac_dec::sr_info::SamplingRateInfo;
/// use aac::common::{
///     bitstream::{Bitstream, Mode},
///     flags::ACFlags,
/// };
///
/// /// Create SamplingRateInfo
/// let mut sr_info = SamplingRateInfo::new();
/// sr_info.init(1024, 3, 48000);
///
/// // Create Bitstream
/// let buffer = vec![0; 8];
/// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
/// bitstream_reader.init(&buffer, 32);
///
/// // Create ICSInfo
/// let mut ics_info = IcsInfo::new();
///
/// // Read bitstream information into ICSInfo
/// ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());
/// ```
#[derive(Default, Debug, Copy, Clone)]
#[repr(C)]
pub struct IcsInfo {
    window_group_lengths: [u8; MAX_WINDOWS],
    n_window_groups: usize,
    window_shape: WindowShape,
    window_sequence: BlockType,
    max_sf_bands: usize,
    scale_factor_grouping: u8,
    scale_factor_bands: &'static [u16],
}

/// The IcsInfo can read information about windows and scale factors from the
/// ICS element in the bitstream. The information can be accessed over the
/// appropriate getter methods (see below). Moreover, the window sequence and
/// shape may be set explicitly with setter methods.
impl IcsInfo {
    /// Create an IcsInfo instance
    pub fn new() -> IcsInfo {
        Default::default()
    }

    /// Read ICS information from a given bitstream. Scale factor information
    /// will be copied from the sampling_rate_info instance (see read_max_sfb).
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream buffer to hold the current bitstream
    /// - `sampling_rate_info`: Instance of sampling_rate_info
    /// - `flags`: Flags to provide information about audio codec
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        sampling_rate_info: &SamplingRateInfo,
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        if ac_flags.contains(ACFlags::ELD) {
            self.window_sequence = BlockType::Long;
            self.window_shape = WindowShape::Sine;
        } else {
            if !ac_flags.contains(ACFlags::USAC) {
                bs.push(1);
            }
            self.window_sequence = match bs.read(2) {
                0 => BlockType::Long,
                1 => BlockType::Start,
                2 => BlockType::Short,
                _ => BlockType::Stop,
            };
            self.window_shape = (bs.read_bit() as u8).into();

            if ac_flags.contains(ACFlags::LD) && self.window_shape != WindowShape::Sine {
                self.window_shape = WindowShape::LowOverlap;
            }
        }

        // Sanity check
        if ac_flags.intersects(ACFlags::ELD | ACFlags::LD)
            && self.window_sequence != BlockType::Long
        {
            self.window_sequence = BlockType::Long;
            return AacDecoderError::ParseError;
        }

        let error_status = self.read_max_sfb(bs, sampling_rate_info);
        if error_status != AacDecoderError::Ok {
            return error_status;
        }

        if self.is_long_block() {
            if !ac_flags.intersects(ACFlags::ELD | ACFlags::SCALABLE | ACFlags::USAC)
                && bs.read_bit() != 0
            // If not ELD nor Scalable nor BSAC nor USAC syntax then ...
            {
                return AacDecoderError::UnsupportedPrediction;
            }

            self.n_window_groups = 1;
            self.window_group_lengths[0] = 1;
        } else {
            self.scale_factor_grouping = bs.read(7) as u8;

            self.n_window_groups = 0;

            for i in 0..MAX_WINDOWS - 1 {
                let mask = 1 << (6 - i);
                self.window_group_lengths[i] = 1;

                if (self.scale_factor_grouping & mask) != 0 {
                    self.window_group_lengths[self.n_window_groups] += 1;
                } else {
                    self.n_window_groups += 1;
                }
            }

            // loop runs to i < 7 only
            self.window_group_lengths[MAX_WINDOWS - 1] = 1;
            self.n_window_groups += 1;
        }

        error_status
    }

    /// Read max_sf_bands, i.e. the maximum number of scale factor bands,
    /// from the bitstream. The scale factor bands themselves are copied
    /// from the sampling_rate_info instance.
    /// # Parameters
    ///
    /// - `bs`: Bitstream buffer to hold the current bitstream
    /// - `sampling_rate_info`: Instance of sampling_rate_info
    pub fn read_max_sfb(
        &mut self,
        bs: &mut Bitstream,
        sampling_rate_info: &SamplingRateInfo,
    ) -> AacDecoderError {
        let mut error_status = AacDecoderError::Ok;

        let nbits: u8;

        if self.is_long_block() {
            nbits = 6;
            self.scale_factor_bands = sampling_rate_info.scale_factor_bands_long().unwrap();
        } else {
            nbits = 4;
            self.scale_factor_bands = sampling_rate_info.scale_factor_bands_short().unwrap();
        }
        self.max_sf_bands = bs.read(nbits) as usize;

        if self.max_sf_bands > self.scale_factor_bands.len() - 1 {
            self.max_sf_bands = self.scale_factor_bands.len() - 1;
            error_status = AacDecoderError::ParseError;
        }

        error_status
    }

    /// Tells whether this is a long block.
    /// Only false for short blocks.
    pub fn is_long_block(&self) -> bool {
        self.window_sequence != BlockType::Short
    }

    /// Get windows per frame. Is MAX_WINDOWS for
    /// short blocks and 1 for all other block types.
    pub fn windows_per_frame(&self) -> usize {
        if self.window_sequence == BlockType::Short {
            MAX_WINDOWS
        } else {
            1
        }
    }

    /// Get number of total scale factor bands.
    pub fn n_total_sf_bands(&self) -> usize {
        if self.scale_factor_bands.is_empty() {
            0
        } else {
            self.scale_factor_bands.len() - 1
        }
    }

    /// Get scale factor bands.
    pub fn scale_factor_bands(&self) -> &[u16] {
        self.scale_factor_bands
    }

    /// Get current window shape.
    pub fn window_shape(&self) -> WindowShape {
        self.window_shape
    }
    /// Set current window shape.
    ///
    /// # Parameters
    ///
    /// - `window_shape`: Window shape to set
    pub fn set_window_shape(&mut self, window_shape: WindowShape) {
        self.window_shape = window_shape;
    }

    /// Get current window sequence.
    pub fn window_sequence(&self) -> BlockType {
        self.window_sequence
    }
    /// Set current window sequence.
    ///
    /// # Parameters
    ///
    /// - `window_sequence`: Window sequence to set
    pub fn set_window_sequence(&mut self, window_sequence: BlockType) {
        self.window_sequence = window_sequence;
    }

    /// Get number of window groups.
    pub fn n_window_groups(&self) -> usize {
        self.n_window_groups
    }

    /// Get the length of the window group with the given index.
    /// # Parameters
    ///
    /// - `index`: Index of window group to get length for
    pub fn window_group_length(&self, index: usize) -> u8 {
        self.window_group_lengths[index]
    }

    /// Get all window group lengths of the current frame.
    pub fn window_group_lengths(&self) -> &[u8] {
        &self.window_group_lengths
    }

    /// Get maximum transmitted scale factor bands.
    pub fn max_sf_bands(&self) -> usize {
        self.max_sf_bands
    }

    /// Calculate NR for current window
    pub fn calc_nr(&self, frame_length: usize) -> usize {
        if self.window_shape == WindowShape::LowOverlap {
            // Low Overlap, 3/4 zeroed.
            (frame_length * 3) >> 2
        } else {
            0
        }
    }

    /// Calculate Window parameters for current window.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info data.
    /// - `frame_length`: AAC frame length.
    /// - `prev tl`: TL size of previous window.
    ///
    /// # Return
    ///
    /// - `tl`: Transform block size.
    /// - `fl`: Length of left window slope.
    /// - `fr`: Length of right window slope.
    /// - `n_spec`: Number of spectra.
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info data.
    /// - `frame_length`: AAC frame length.
    /// - `prev tl`: TL size of previous window.
    ///
    /// # Return
    ///
    /// - `tl`: Transform block size.
    /// - `fl`: Length of left window slope.
    /// - `fr`: Length of right window slope.
    /// - `n_spec`: Number of spectra.
    pub fn get_window_params(
        &self,
        frame_length: usize,
        prev_tl: usize,
    ) -> (usize, usize, usize, usize) {
        let tl;
        let fl;
        let fr;
        let n_spec;

        match self.window_sequence() {
            BlockType::Long => {
                tl = frame_length;
                fr = frame_length - self.calc_nr(frame_length);
                // New startup needs differentiation between sine shape and low overlap
                // shape. This is a special case for the LD-AAC transformation windows,
                // because the slope length can be different while using the same window
                // sequence.
                fl = if prev_tl == 0 { fr } else { frame_length };
                n_spec = 1;
            }
            BlockType::Stop => {
                tl = frame_length;
                fl = frame_length >> 3;
                fr = frame_length;
                n_spec = 1;
            }
            BlockType::Start => {
                /* or StopStartSequence */
                tl = frame_length;
                fl = frame_length;
                fr = frame_length >> 3;
                n_spec = 1;
            }
            BlockType::Short => {
                tl = frame_length >> 3;
                fl = frame_length >> 3;
                fr = frame_length >> 3;
                n_spec = 8;
            }
        }

        (tl, fl, fr, n_spec)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::bitstream::Mode;

    #[test]
    fn long_block() {
        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        {
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            let mut bitstream_writer = Bitstream::new(8, Mode::Writer);
            bitstream_writer.write(0b00011010000, 11);
            bitstream_writer.sync();

            let mut bitstream_reader =
                Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
            bitstream_reader.init(bitstream_writer.buffer(), 11);

            // Create ICSInfo
            let mut ics_info = IcsInfo::new();
            ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

            assert!(ics_info.is_long_block());
            assert_eq!(ics_info.windows_per_frame(), 1);
            assert_eq!(ics_info.window_shape(), WindowShape::KBD);
            assert_eq!(ics_info.window_sequence(), BlockType::Long);
            assert_eq!(ics_info.n_window_groups(), 1);
            assert_eq!(ics_info.window_group_length(0), 1);
            assert_eq!(ics_info.window_group_lengths()[0], 1);
            assert_eq!(ics_info.max_sf_bands(), 40);
            assert_eq!(ics_info.n_total_sf_bands(), 49); // Using SFB_48_1024
        }
    }

    #[test]
    fn short_blocks() {
        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        {
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b10     : BlockType::Short
            // 0b0      : SineWindow
            // 0b1100   : max_sf_bands = 12
            // 0b1101101: scale factor grouping: 3|3|2
            let mut bitstream_writer = Bitstream::new(8, Mode::Writer);
            bitstream_writer.write(0b010011001101101, 15);
            bitstream_writer.sync();

            let mut bitstream_reader =
                Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
            bitstream_reader.init(bitstream_writer.buffer(), 15);

            // Create ICSInfo
            let mut ics_info = IcsInfo::new();
            ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

            assert!(!ics_info.is_long_block());
            assert_eq!(ics_info.windows_per_frame(), MAX_WINDOWS);
            assert_eq!(ics_info.window_shape(), WindowShape::Sine);
            assert_eq!(ics_info.window_sequence(), BlockType::Short);
            assert_eq!(ics_info.n_window_groups(), 3);
            assert_eq!(ics_info.window_group_length(0), 3);
            assert_eq!(ics_info.window_group_length(1), 3);
            assert_eq!(ics_info.window_group_length(2), 2);
            assert_eq!(ics_info.window_group_lengths()[0..3], [3, 3, 2]);
            assert_eq!(ics_info.max_sf_bands(), 12);
            assert_eq!(ics_info.n_total_sf_bands(), 14); // Using SFB_48_128
        }
    }
}
