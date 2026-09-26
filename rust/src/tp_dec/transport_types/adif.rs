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
//! Audio data interchange format (ADIF) reader

use super::super::{error_codes::TpDecoderError, pce::ProgramConfig};
use crate::common::bitstream::Bitstream;

#[derive(Clone, Default, PartialEq, Debug)]
#[repr(C)]
pub(in super::super) struct Adif {
    num_program_config_elements: u8,
    bit_rate: u32,
    /// Indicates whether copyright_id is present or not.
    copyright_id_present: u8,
    /// If this bit equals '0', there is no copyright on the ISO/IEC 11 172-3 bitstream, '1' means
    /// copyright protected.
    original_copy: u8,
    /// This bit equals '0' if the bitstream is a copy,
    /// '1' if it is an original.
    home: u8,
    /// A flag indicating the type of a bitstream
    /// * '0' constant rate bitstream. This bitstream may be transmitted via a channel with
    ///   constant rate.
    /// * '1' variable rate bitstream. This bitstream is not designed for transmission via constant
    ///   rate channels.
    bitstream_type: u8,
}

const MIN_ADIF_HEADERLENGTH: isize = 63;

impl Adif {
    /// Parses an ADIF header of the given audio file and store the parsed data
    /// into the `AdifHeader` and `ProgramConfig` struct.
    ///
    /// # Parameters
    ///
    /// - `pce`: reference to a `ProgramConfig` structure where the last PCE will remain.
    /// - `bs`: reference to a `Bitstream` struct.
    pub(in super::super) fn read_decode_header(
        &mut self,
        pce: &mut ProgramConfig,
        bs: &mut Bitstream,
    ) -> Result<(), TpDecoderError> {
        let start_anchor = bs.valid_bits();

        if start_anchor < MIN_ADIF_HEADERLENGTH {
            return Err(TpDecoderError::NotEnoughBits);
        }

        if "ADIF".chars().any(|c| bs.read(8) != c as u32) {
            return Err(TpDecoderError::SyncError);
        }

        self.copyright_id_present = bs.read_bit() as u8;
        if self.copyright_id_present != 0 {
            bs.push(72);
        }

        self.original_copy = bs.read_bit() as u8;
        self.home = bs.read_bit() as u8;
        self.bitstream_type = bs.read_bit() as u8;
        self.bit_rate = bs.read(23);
        self.num_program_config_elements = bs.read(4) as u8 + 1;

        if self.bitstream_type == 0 {
            // Skip `adif_buffer_fullness` bit field.
            bs.push(20);
        }

        // Parse all PCEs but keep only the last one.
        for _ in 0..self.num_program_config_elements {
            pce.read(bs, start_anchor);
        }

        bs.align(start_anchor);

        if pce.sampling_frequency_index() >= 13 || pce.profile() != 1 {
            return Err(TpDecoderError::ParseError);
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::bitstream::Mode::Reader;

    #[test]
    fn test_with_dumped_header_01() {
        let ref_bytes = [
            65, 68, 73, 70, 0, 62, 128, 0, 5, 251, 0, 160, 128, 0, 4, 0, 0, 33, 16, 5, 0, 160, 27,
            255, 192, 0, 0, 0, 0, 0, 0, 0,
        ];
        let adif_ref = Adif {
            num_program_config_elements: 1,
            bit_rate: 128000u32,
            copyright_id_present: 0,
            original_copy: 0,
            home: 0,
            bitstream_type: 0,
        };

        let mut pce_ref = ProgramConfig::default();
        let valid_bits = ref_bytes.len() * 8;
        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);
        // skip initial 83 bits - that's where 1 PCE config is in the dumped bitstream
        let _ = bs.read(32);
        let _ = bs.read(32);
        let _ = bs.read(19);
        pce_ref.read(&mut bs, valid_bits as isize);

        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let mut adif_cut: Adif = Default::default();
        let mut pce_cut = Default::default();
        let error = adif_cut.read_decode_header(&mut pce_cut, &mut bs);

        assert_eq!(error, Ok(()));
        assert_eq!(adif_ref, adif_cut);
        assert_eq!(pce_ref, pce_cut);
    }

    #[test]
    fn test_with_dumped_header_02() {
        let ref_bytes = [
            65, 68, 73, 70, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 128, 0, 5, 251, 0, 160, 128, 0, 4,
            0, 0, 33, 16, 5, 0, 160, 27, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        let adif_ref = Adif {
            num_program_config_elements: 1,
            bit_rate: 128000u32,
            copyright_id_present: 1,
            original_copy: 0,
            home: 0,
            bitstream_type: 0,
        };

        let mut pce_ref = ProgramConfig::default();
        let valid_bits = ref_bytes.len() * 8;
        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);
        // skip initial 155 bits - that's where 1 PCE config is in the dumped bitstream
        let _ = bs.read(32);
        let _ = bs.read(32);
        let _ = bs.read(32);
        let _ = bs.read(32);
        let _ = bs.read(27);
        pce_ref.read(&mut bs, valid_bits as isize);

        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let mut adif_cut: Adif = Default::default();
        let mut pce_cut = Default::default();
        let error = adif_cut.read_decode_header(&mut pce_cut, &mut bs);

        assert_eq!(error, Ok(()));
        assert_eq!(adif_ref, adif_cut);
        assert_eq!(pce_ref, pce_cut);
    }

    #[test]
    fn test_too_short_header() {
        let ref_bytes = [65, 68];

        let valid_bits = ref_bytes.len() * 8;

        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let mut adif_cut: Adif = Default::default();
        let mut pce_cut = Default::default();
        let error = adif_cut.read_decode_header(&mut pce_cut, &mut bs);

        assert_eq!(error, Err(TpDecoderError::NotEnoughBits));
    }

    #[test]
    fn test_sync_error() {
        let ref_bytes = [
            1, 2, 3, 4, 0, 62, 128, 0, 5, 251, 0, 160, 128, 0, 4, 0, 0, 33, 16, 5, 0, 160, 27, 255,
            192, 0, 0, 0, 0, 0, 0, 0,
        ];

        let valid_bits = ref_bytes.len() * 8;

        let mut bs = Bitstream::new(ref_bytes.len(), Reader);
        bs.init(&ref_bytes, valid_bits);

        let mut adif_cut: Adif = Default::default();
        let mut pce_cut = Default::default();
        let error = adif_cut.read_decode_header(&mut pce_cut, &mut bs);

        assert_eq!(error, Err(TpDecoderError::SyncError));
    }
}
