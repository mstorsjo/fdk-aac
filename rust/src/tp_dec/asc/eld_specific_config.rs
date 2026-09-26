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
//! Enhanced low delay (ELD) specific config

use super::super::{
    callbacks::TpDecCallBacks,
    constants::{TPDEC_CORE_SBR_FRAME_LENGTH_INDEX_NONE, TP_USAC_MAX_ELEMENTS},
    error_codes::TpDecoderError,
};
use super::helper_functions::{get_element_list, get_sample_rate, skip_sbr_header};
use crate::common::aot::AudioObjectType;
use crate::common::bs_element_id::ChannelElementId;
use crate::common::enums::StereoCfgIndex;
use crate::common::{bitstream::Bitstream, flags::ACFlags};

/// ELD extension types.
#[derive(Debug, PartialEq)]
enum EldExtType {
    /// Termination tag.
    Term = 0x0,
    /// SAOC config.
    Saoc = 0x1,
    /// LD MPEG Surround config.
    Ldsac = 0x2,
    /// ELD sample rate adaptation.
    DownscaleInfo = 0x3,
    Unknown,
}

impl From<u32> for EldExtType {
    fn from(value: u32) -> Self {
        match value {
            0x0 => EldExtType::Term,
            0x1 => EldExtType::Saoc,
            0x2 => EldExtType::Ldsac,
            0x3 => EldExtType::DownscaleInfo,
            _ => EldExtType::Unknown,
        }
    }
}

/// ELD specific configuration.
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub(super) struct EldSpecificConfig {
    /// Use LD-MPS QMF in SBR to achieve time alignment.
    use_ld_qmf_time_align: bool,
    sbr_sampling_rate: u8,
    downscaled_sampling_frequency: u32,
    downscale_factor: u8,
}

impl EldSpecificConfig {
    /// Clears the ELD Specific Config values.
    pub(super) fn reset(&mut self) {
        self.use_ld_qmf_time_align = false;
        self.sbr_sampling_rate = 0;
        self.downscaled_sampling_frequency = 0;
        self.downscale_factor = 0;
    }

    /// Parse the ELD specific config.
    ///
    /// # Parameters
    ///
    /// - `sampling_frequency`: Sampling frequency.
    /// - `channel_config`: MPEG-4 channel configuration.
    /// - `ac_flags`: Audio coding flags (see common/flags.rs).
    /// - `bs`: Bitstream instance with valid internal data.
    /// - `cb`: Transport decoder callbacks.
    pub(super) fn parse(
        &mut self,
        sampling_frequency: u32,
        channel_config: u8,
        ac_flags: &mut ACFlags,
        bs: &mut Bitstream,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        let mut error_status = Ok(());

        let mut elements = [ChannelElementId::None; TP_USAC_MAX_ELEMENTS];

        if get_element_list(channel_config as u32, &mut elements[..]).is_err() {
            return Err(TpDecoderError::ParseError);
        }

        self.reset();

        let eld_specific_config_anchor = bs.valid_bits();

        ac_flags.insert(if bs.read_bit() != 0 {
            ACFlags::FRAME_LENGTH
        } else {
            ACFlags::empty()
        });

        let sample_per_frame = if ac_flags.contains(ACFlags::FRAME_LENGTH) {
            480
        } else {
            512
        };

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

        if bs.read_bit() != 0 {
            ac_flags.insert(ACFlags::SBR_PRESENT);
            // 0: single rate, 1: dual rate
            self.sbr_sampling_rate = bs.read_bit() as u8;

            ac_flags.insert(if bs.read_bit() != 0 {
                ACFlags::SBRCRC
            } else {
                ACFlags::empty()
            });

            // ELD reduced delay mode: LD-SBR initialization has to know the downscale
            // information. Postpone LD-SBR initialization and read ELD extension
            // information first.
            for ele in elements.iter() {
                if (*ele == ChannelElementId::Sce) || (*ele == ChannelElementId::Cpe) {
                    skip_sbr_header(bs, false);
                }
            }
        }
        self.use_ld_qmf_time_align = false;
        // New ELD syntax.
        self.downscaled_sampling_frequency = sampling_frequency;

        let mut eld_ext_cnt = 0;
        let mut eld_ext_type = bs.read(4);

        // Parse ExtTypeConfigData.
        while EldExtType::from(eld_ext_type) != EldExtType::Term
            && bs.valid_bits() >= 0
            && eld_ext_cnt < 15
        {
            eld_ext_cnt += 1;

            let mut eld_ext_len = bs.read(4);
            let mut len = eld_ext_len;

            if len == 0xf {
                len = bs.read(8);
                eld_ext_len += len;

                if len == 0xff {
                    len = bs.read(16);
                    eld_ext_len += len;
                }
            }

            match EldExtType::from(eld_ext_type) {
                EldExtType::Ldsac => {
                    self.use_ld_qmf_time_align = true;

                    error_status = cb.ssc_callback(
                        bs,
                        AudioObjectType::AotErAacEld,
                        sampling_frequency << self.sbr_sampling_rate,
                        sample_per_frame << self.sbr_sampling_rate,
                        channel_config,
                        StereoCfgIndex::Mps212,
                        TPDEC_CORE_SBR_FRAME_LENGTH_INDEX_NONE,
                        eld_ext_len,
                    );

                    if error_status.is_ok() {
                        ac_flags.insert(ACFlags::MPS_PRESENT);
                    }

                    if error_status == Err(TpDecoderError::UnsupportedFormat) {
                        ac_flags.remove(ACFlags::MPS_PRESENT);
                        error_status = Ok(());
                    }

                    if error_status.is_err() {
                        return Err(TpDecoderError::ParseError);
                    }

                    // ELDv2 w/ ELD downscaled mode not allowed.
                    if self.downscaled_sampling_frequency != sampling_frequency {
                        return Err(TpDecoderError::UnsupportedFormat);
                    }
                }
                EldExtType::DownscaleInfo => {
                    self.downscaled_sampling_frequency = get_sample_rate(bs, None, 4);
                    if self.downscaled_sampling_frequency == 0 {
                        return Err(TpDecoderError::ParseError);
                    }

                    if bs.read(4) != 0x0 {
                        return Err(TpDecoderError::ParseError);
                    }

                    // ELDv2 w/ ELD downscaled mode not allowed.
                    if self.use_ld_qmf_time_align {
                        return Err(TpDecoderError::UnsupportedFormat);
                    }
                }
                _ => bs.push((eld_ext_len * 8) as isize),
            };

            eld_ext_type = bs.read(4);
        }
        if EldExtType::from(eld_ext_type) != EldExtType::Term {
            return Err(TpDecoderError::ParseError);
        }

        self.downscale_factor = 1;

        if self.downscaled_sampling_frequency != 0
            && (sampling_frequency % self.downscaled_sampling_frequency) == 0
        {
            let ds_factor = sampling_frequency / self.downscaled_sampling_frequency;

            // frameSize/dsf must be an integer number.
            if (u32::from(sample_per_frame) % ds_factor) != 0 {
                return Err(TpDecoderError::UnsupportedFormat);
            }

            match ds_factor {
                1 | 2 | 4 => self.downscale_factor = ds_factor as u8,
                3 => {
                    if !(ac_flags.contains(ACFlags::SBR_PRESENT)
                        && ac_flags.contains(ACFlags::FRAME_LENGTH))
                    {
                        self.downscale_factor = ds_factor as u8;
                    }
                }
                _ => (),
            };

            ac_flags.insert(if self.downscale_factor > 1 {
                ACFlags::ELD_DOWNSCALE
            } else {
                ACFlags::empty()
            });
        }

        if ac_flags.contains(ACFlags::SBR_PRESENT) {
            let bs_anchor = bs.valid_bits();
            bs.push(-(eld_specific_config_anchor - 7 - bs_anchor));

            for (ele_idx, ele) in elements.iter().enumerate() {
                if ele.is_channel_element()
                    && cb
                        .sbr_callback(
                            bs,
                            AudioObjectType::AotErAacEld,
                            *ele,
                            sampling_frequency / self.downscale_factor as u32,
                            (sampling_frequency << self.sbr_sampling_rate)
                                / self.downscale_factor as u32,
                            sample_per_frame / self.downscale_factor as u16,
                            ele_idx,
                            self.downscale_factor,
                            false,
                        )
                        .is_err()
                {
                    return Err(TpDecoderError::ParseError);
                }
            }

            let num_bits = bs.valid_bits() - bs_anchor;
            bs.push(num_bits);
        }
        error_status
    }

    /// Returns SBR sampling rate.
    pub(super) fn sbr_sampling_rate(&self) -> u8 {
        self.sbr_sampling_rate
    }

    /// Returns downscale factor.
    pub(super) fn downscale_factor(&self) -> u8 {
        self.downscale_factor
    }

    /// Sets downscale factor.
    pub(super) fn set_downscale_factor(&mut self, downscale_factor: u8) {
        self.downscale_factor = downscale_factor;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reset() {
        let mut eld = EldSpecificConfig {
            use_ld_qmf_time_align: true,
            sbr_sampling_rate: 1,
            downscaled_sampling_frequency: 1,
            downscale_factor: 1,
        };
        eld.reset();

        assert!(!eld.use_ld_qmf_time_align);
        assert!(eld.sbr_sampling_rate == 0);
        assert!(eld.downscaled_sampling_frequency == 0);
        assert!(eld.downscale_factor == 0);
    }
}
