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
//! USAC extension element config (ASC)

use super::super::{callbacks::TpDecCallBacks, error_codes::TpDecoderError};
use super::usac_config::UsacExtElementType;
use crate::common::aot::AudioObjectType;
use crate::common::bitstream::Bitstream;
use crate::common::bs_element_id::ChannelElementId;
use crate::common::flags::ChannelFlags;

/// USAC extension element configuration.
#[repr(C)]
#[derive(Debug, Default, Copy, Clone, PartialEq)]
pub struct UsacExtElementConfig {
    ext_element_type: UsacExtElementType,
    default_length: u32,
    is_fragmented_payload: bool,
}

impl UsacExtElementConfig {
    /// Creates USAC Extension Element Config instance.
    pub fn new() -> Self {
        UsacExtElementConfig::default()
    }

    /// Returns extension element type.
    pub fn ext_element_type(&self) -> UsacExtElementType {
        self.ext_element_type
    }

    /// Returns extension element type default length.
    pub fn default_length(&self) -> u32 {
        self.default_length
    }

    /// Returns extension element type.
    pub fn is_fragmented_payload(&self) -> bool {
        self.is_fragmented_payload
    }

    /// Subroutine for parsing extension element configuration:
    /// UsacExtElementConfig() q.v. ISO/IEC FDIS 23003-3:2011(E) Table 14.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    pub(super) fn config(
        &mut self,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());
        let mut usac_ext_element_type = bs.escaped_value(4, 8, 16);
        let usac_ext_element_config_length = bs.escaped_value(4, 8, 16);

        // usacExtElementDefaultLengthPresent
        if bs.read_bit() != 0 {
            self.default_length = bs.escaped_value(8, 16, 0) + 1;
        } else {
            self.default_length = 0;
        }

        self.is_fragmented_payload = bs.read_bit() != 0;

        let bs_anchor = bs.valid_bits();

        // Returns an error in case the bitbuffer fill level is too low.
        if bs_anchor < (usac_ext_element_config_length * 8) as isize {
            return Err(TpDecoderError::NotEnoughBits);
        }

        match usac_ext_element_type {
            0xFF | 0x00 | 0x03 => (),
            0x04 => {
                // uniDrcConfig
                error_status = cb.uni_drc_callback(Some(bs), 0, AudioObjectType::AotUsac);
                error_status?;
            }
            _ => usac_ext_element_type = 0xFF,
        };

        self.ext_element_type = UsacExtElementType::from(usac_ext_element_type);

        // Adjust bit stream position. This is required because of byte alignment and
        // unhandled extensions.
        {
            let left_bits: i32 =
                (usac_ext_element_config_length << 3) as i32 - (bs_anchor - bs.valid_bits()) as i32;
            if left_bits >= 0 {
                bs.push(left_bits as isize);
            } else {
                // Parsed too many bits.
                error_status = Err(TpDecoderError::ParseError);
            }
        }

        error_status
    }
}

/// USAC element configuration.
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct UsacElementConfig {
    element_type: ChannelElementId,
    ac_el_flags: ChannelFlags,
    pub ext_element: UsacExtElementConfig,
}

impl UsacElementConfig {
    ///  Creates USAC Element Config instance.
    pub fn new() -> Self {
        UsacElementConfig::default()
    }

    /// Returns element type.
    pub fn element_type(&self) -> ChannelElementId {
        self.element_type
    }

    /// Returns element flags.
    pub fn ac_el_flags(&self) -> ChannelFlags {
        self.ac_el_flags
    }

    /// Returns extension element.
    pub fn ext_element(&self) -> UsacExtElementConfig {
        self.ext_element
    }

    /// Sets element type.
    pub fn set_element_type(&mut self, element_type: ChannelElementId) {
        self.element_type = element_type;
    }

    /// Sets element flags.
    pub fn set_ac_el_flags(&mut self, ac_el_flags: ChannelFlags) {
        self.ac_el_flags = ac_el_flags;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_usac_extension_element_config() {
        let usac_ext_element_config = UsacExtElementConfig::new();
        assert!(usac_ext_element_config.ext_element_type == UsacExtElementType::Fill);
        assert!(usac_ext_element_config.default_length == 0);
        assert!(!usac_ext_element_config.is_fragmented_payload);
    }

    #[test]
    fn new_usac_element_config() {
        let usac_element_config = UsacElementConfig::new();
        assert!(usac_element_config.element_type == ChannelElementId::None);
        assert!(usac_element_config.ac_el_flags == ChannelFlags::empty());
    }
}
