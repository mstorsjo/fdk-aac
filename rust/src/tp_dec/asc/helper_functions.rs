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
//! ASC helper functions

use super::super::{ProgramConfig, TpDecoderError};
use crate::common::aot::AudioObjectType;
use crate::common::bitstream::Bitstream;
use crate::common::bs_element_id::ChannelElementId;
use crate::common::samplerate_index::SAMPLING_RATE_TABLE;

/// Returns `AudioObjectType`.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data.
pub(super) fn get_aot(bs: &mut Bitstream) -> AudioObjectType {
    let mut temp = bs.read(5) as i32;
    if AudioObjectType::from(temp) == AudioObjectType::AotEscape {
        let temp_2 = bs.read(6) as i32;
        temp = 32 + temp_2;
    }
    AudioObjectType::from(temp)
}

/// Returns sampling rate.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data.
/// - `index`: Sampling rate index.
/// - `n_bits`: Number of bits.
pub(super) fn get_sample_rate(bs: &mut Bitstream, index: Option<&mut u8>, n_bits: u8) -> u32 {
    let idx = bs.read(n_bits);

    let sample_rate = if idx == ((1 << n_bits) - 1) {
        if bs.valid_bits() < 24 {
            return 0;
        }
        bs.read(24)
    } else {
        SAMPLING_RATE_TABLE[idx as usize]
    };

    if let Some(i) = index {
        *i = idx as u8
    };

    sample_rate
}

/// Skips SBR header.
///
/// # Parameters
///
/// - `bs`: Bitstream instance with valid internal data.
/// - `is_usac`: `true` if is `USAC`, Otherwise `false`.
pub(super) fn skip_sbr_header(bs: &mut Bitstream, is_usac: bool) {
    if !is_usac {
        // Amp res 1, xover freq 3, reserved 2.
        bs.push(6);
    }
    // start / stop freq.
    bs.push(8);

    // Parse SBR default header.
    let dflt_header_extra_1 = bs.read_bit();
    let dflt_header_extra_2 = bs.read_bit();
    let num_bits = ((5 * dflt_header_extra_1) + (6 * dflt_header_extra_2)) as isize;
    bs.push(num_bits);
}

/// Returns element's list.
///
/// # Parameters
///
/// - `channel_config`: MPEG-4 channel configuration.
/// - `element_list`: Buffer to store element list.
pub(super) fn get_element_list(
    channel_config: u32,
    element_list: &mut [ChannelElementId],
) -> Result<(), TpDecoderError> {
    let mut tmp_pce = ProgramConfig::new();

    if tmp_pce.get_default_mpeg_config(channel_config) {
        tmp_pce.get_element_list(element_list);
        Ok(())
    } else {
        Err(TpDecoderError::ParseError)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::bitstream::Mode;

    #[test]
    fn test_get_aot() {
        let buffer: [u8; 4] = [0, 0, 0, 0];
        let mut dummy_bs = Bitstream::new(4, Mode::Reader);
        dummy_bs.init(&buffer, 24);

        let aot = get_aot(&mut dummy_bs);

        assert_eq!(aot, AudioObjectType::AotNullObject);
    }

    #[test]
    fn test_get_sample_rate() {
        let buffer: [u8; 4] = [0, 0, 0, 0];
        let mut dummy_bs = Bitstream::new(4, Mode::Reader);
        dummy_bs.init(&buffer, 24);

        let sample_rate = get_sample_rate(&mut dummy_bs, None, 10);

        assert_eq!(sample_rate, 96000);
    }

    #[test]
    fn test_is_channel_ele() {
        assert!(ChannelElementId::Sce.is_channel_element());
        assert!(ChannelElementId::Cpe.is_channel_element());
        assert!(ChannelElementId::Lfe.is_channel_element());
        assert!(ChannelElementId::UsacSce.is_channel_element());
        assert!(ChannelElementId::UsacCpe.is_channel_element());
        assert!(ChannelElementId::UsacLfe.is_channel_element());

        assert!(!ChannelElementId::Cce.is_channel_element());
        assert!(!ChannelElementId::Dse.is_channel_element());
        assert!(!ChannelElementId::Pce.is_channel_element());
        assert!(!ChannelElementId::Fil.is_channel_element());
        assert!(!ChannelElementId::End.is_channel_element());
        assert!(!ChannelElementId::Ext.is_channel_element());
        assert!(!ChannelElementId::UsacExt.is_channel_element());
        assert!(!ChannelElementId::Last.is_channel_element());
    }
}
