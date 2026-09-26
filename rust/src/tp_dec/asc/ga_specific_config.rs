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
//! GA specific config

use super::super::{error_codes::TpDecoderError, pce::ProgramConfig};
use crate::common::bitstream::Bitstream;
use crate::common::flags::ACFlags;

/// General Audio Specific Configuration.
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub(super) struct GaSpecificConfig {
    depends_on_core_coder: u32,
    core_coder_delay: u32,
    extension_flag: u32,
    extension_flag3: u32,
    layer: u32,
}

impl GaSpecificConfig {
    ///  Creates General Audio Specific Config instance.
    pub fn _new() -> Self {
        GaSpecificConfig::default()
    }

    /// Clears the General Audio Specific Config values.
    pub(super) fn reset(&mut self) {
        self.depends_on_core_coder = 0;
        self.core_coder_delay = 0;
        self.extension_flag = 0;
        self.extension_flag3 = 0;
        self.layer = 0;
    }

    /// Parse the General Audio Specific Config.
    ///
    /// # Parameters
    ///
    /// - `pce`: Program config element.
    /// - `channel_config`: MPEG-4 channel configuration.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `asc_start_anchor`: Bit position to be considered as origin for byte alignment.
    pub(super) fn parse(
        &mut self,
        pce: &mut ProgramConfig,
        channel_config: u8,
        ac_flags: &mut ACFlags,
        bs: &mut Bitstream,
        asc_start_anchor: isize,
    ) -> Result<(), TpDecoderError> {
        ac_flags.insert(if bs.read_bit() != 0 {
            ACFlags::FRAME_LENGTH
        } else {
            ACFlags::empty()
        });

        self.depends_on_core_coder = bs.read_bit();

        if self.depends_on_core_coder != 0 {
            self.core_coder_delay = bs.read(14);
        }

        self.extension_flag = bs.read_bit();

        if channel_config == 0 {
            pce.read(bs, asc_start_anchor);
        }

        if ac_flags.contains(ACFlags::SCALABLE) {
            self.layer = bs.read(3);
            // Only first AAC layer supported.
            if self.layer > 0 {
                return Err(TpDecoderError::UnknownError);
            }
        }

        if self.extension_flag != 0 {
            if ac_flags.contains(ACFlags::ER) {
                ac_flags.insert(if bs.read_bit() != 0 {
                    ACFlags::ER_VCB11
                } else {
                    ACFlags::empty()
                });

                ac_flags.insert(if bs.read_bit() != 0 {
                    ACFlags::ER_RVLC
                } else {
                    ACFlags::empty()
                });

                ac_flags.insert(if bs.read_bit() != 0 {
                    ACFlags::ER_HCR
                } else {
                    ACFlags::empty()
                });
            }

            self.extension_flag3 = bs.read_bit();
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new() {
        let gasc = GaSpecificConfig::_new();
        assert!(gasc.depends_on_core_coder == 0);
        assert!(gasc.core_coder_delay == 0);
        assert!(gasc.extension_flag == 0);
        assert!(gasc.extension_flag3 == 0);
        assert!(gasc.layer == 0);
    }

    #[test]
    fn reset() {
        let mut gasc = GaSpecificConfig {
            depends_on_core_coder: 1,
            core_coder_delay: 1,
            extension_flag: 1,
            extension_flag3: 1,
            layer: 1,
        };
        gasc.reset();

        assert!(gasc.depends_on_core_coder == 0);
        assert!(gasc.core_coder_delay == 0);
        assert!(gasc.extension_flag == 0);
        assert!(gasc.extension_flag3 == 0);
        assert!(gasc.layer == 0);
    }
}
