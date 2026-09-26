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
use aac::aac_dec::config::Config;
use aac::aac_dec::drc::AacDrcParameterHandling;
use aac::aac_dec::error_codes::AacDecoderError;
use aac::aac_dec::params::LimiterMode;
use aac::aac_dec::params::Param;
use aac::aac_dec::params::Params;
use aac::common::flags::ACFlags;
use aac::pcm_dmx::DualChannelMode;

/// AAC decoder setting parameters
#[repr(C)]
pub enum ParamBind {
    /// Defines how the decoder processes two channel signals
    PcmDualChannelOutputMode = 0x0002,

    /// Output buffer channel ordering
    PcmOutputChannelMapping = 0x0003,

    /// Minimum number of PCM output channels
    PcmMinOutputChannels = 0x0005,

    /// Maximum number of PCM output channels
    PcmMaxOutputChannels = 0x0006,

    /// Enable signal level limiting
    PcmLimiterEnable = 0x0010,

    /// Signal level limiting attack time in ms
    PcmLimiterAttackTime = 0x0011,

    /// Signal level limiting release time in ms
    PcmLimiterReleaseTime = 0x0012,

    /// See AAC_MD_PROFILE for all available values
    MetadataProfile = 0x0020,

    /// Time in ms after which meta-data will reset to default
    MetadataExpiryTime = 0x0021,

    /// Error concealment: Processing method
    ConcealMethod = 0x0100,

    /// MPEG-4 / MPEG-D Dynamic Range Control: Boosting gain values
    DrcBoostFactor = 0x0200,

    /// MPEG-4 / MPEG-D DRC: Attenuating gain values
    DrcAttenuationFactor = 0x0201,

    /// Target reference level / decoder target loudness
    DrcReferenceLevel = 0x0202,

    /// En-/Disable DVB specific heavy compression
    DrcHeavyCompression = 0x0203,

    /// Default presentation mode (DRC parameter handling)
    DrcDefaultPresentationMode = 0x0204,

    /// Encoder target level for light compression
    DrcEncTargetLevel = 0x0205,

    /// MPEG-D DRC: Request a DRC effect type for selection of a DRC set
    UnidrcSetEffect = 0x0206,

    /// MPEG-D DRC: Enable album mode
    UnidrcAlbumMode = 0x0207,

    /// Ignore buffer fullness parameter for streaming
    TpdecParamIgnoreBufferFullness = 0x0601,

    /// Enhance synchronization robustness by requiring two valid sync words in expected positions
    /// for bitstreams
    TpdecCheckTwoSyncs = 0x0605,

    /// Target Layout index for audio output
    TargetLayoutCicp = 0x0900,
}

// Converts ParamBind to i16
impl TryFrom<ParamBind> for i16 {
    type Error = &'static str;

    fn try_from(value: ParamBind) -> Result<Self, Self::Error> {
        match value {
            ParamBind::PcmDualChannelOutputMode => Ok(0x0002),
            ParamBind::PcmOutputChannelMapping => Ok(0x0003),
            ParamBind::PcmMinOutputChannels => Ok(0x0005),
            ParamBind::PcmMaxOutputChannels => Ok(0x0006),
            ParamBind::PcmLimiterEnable => Ok(0x0010),
            ParamBind::PcmLimiterAttackTime => Ok(0x0011),
            ParamBind::PcmLimiterReleaseTime => Ok(0x0012),
            ParamBind::MetadataProfile => Ok(0x0020),
            ParamBind::MetadataExpiryTime => Ok(0x0021),
            ParamBind::ConcealMethod => Ok(0x0100),
            ParamBind::DrcBoostFactor => Ok(0x0200),
            ParamBind::DrcAttenuationFactor => Ok(0x0201),
            ParamBind::DrcReferenceLevel => Ok(0x0202),
            ParamBind::DrcHeavyCompression => Ok(0x0203),
            ParamBind::DrcDefaultPresentationMode => Ok(0x0204),
            ParamBind::DrcEncTargetLevel => Ok(0x0205),
            ParamBind::UnidrcSetEffect => Ok(0x0206),
            ParamBind::UnidrcAlbumMode => Ok(0x0207),
            ParamBind::TpdecParamIgnoreBufferFullness => Ok(0x0601),
            ParamBind::TpdecCheckTwoSyncs => Ok(0x0605),
            ParamBind::TargetLayoutCicp => Ok(0x0900),
        }
    }
}

#[no_mangle]
pub extern "C" fn bindings_aacDecoderParams_set_param(
    params: &mut Params,
    param: ParamBind,
    value: i32,
) -> AacDecoderError {
    /* configure the subsystems */
    match param {
        ParamBind::PcmDualChannelOutputMode => {
            if let Ok(dual_ch_mode) = DualChannelMode::try_from(value) {
                if let Err(result) = params.set_param(Param::PcmDualChannelOutputMode(dual_ch_mode))
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
                if let Err(result) = params.set_param(Param::PcmMinOutputChannels(ch_out_min)) {
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
                if let Err(result) = params.set_param(Param::PcmMaxOutputChannels(ch_out_max)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::UnsupportedChannelconfig
            }
        }
        ParamBind::PcmLimiterEnable => {
            if let Ok(limiter_mode) = LimiterMode::try_from(value) {
                if let Err(result) = params.set_param(Param::PcmLimiterEnable(limiter_mode)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::PcmLimiterAttackTime => {
            if let Ok(pcm_lim_attack_time) = u8::try_from(value) {
                if let Err(result) =
                    params.set_param(Param::PcmLimiterAttackTime(pcm_lim_attack_time))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::PcmLimiterReleaseTime => {
            if let Ok(lim_release_time) = u8::try_from(value) {
                if let Err(result) =
                    params.set_param(Param::PcmLimiterReleaseTime(lim_release_time))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::MetadataProfile => {
            if let Ok(md_profile) = value.try_into() {
                if let Err(result) = params.set_param(Param::MetadataProfile(md_profile)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::MetadataExpiryTime => {
            if let Ok(md_expiry_time) = u16::try_from(value) {
                if let Err(result) = params.set_param(Param::MetadataExpiryTime(md_expiry_time)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::PcmOutputChannelMapping => {
            if let Err(result) = params.set_param(Param::PcmOutputChannelMapping(value.into())) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::TargetLayoutCicp => {
            if let Ok(cicp_layout) = u8::try_from(value) {
                if let Err(result) = params.set_param(Param::TargetLayoutCicp(cicp_layout)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::DrcBoostFactor => {
            if let Ok(drc_boost_factor) = u8::try_from(value) {
                if let Err(result) = params.set_param(Param::DrcBoostFactor(drc_boost_factor)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::DrcAttenuationFactor => {
            if let Ok(drc_att_factor) = u8::try_from(value) {
                if let Err(result) = params.set_param(Param::DrcAttenuationFactor(drc_att_factor)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::DrcReferenceLevel => {
            if let Ok(drc_ref_level) = i8::try_from(value) {
                if let Err(result) = params.set_param(Param::DrcReferenceLevel(drc_ref_level)) {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::DrcHeavyCompression => {
            if let Err(result) = params.set_param(Param::DrcHeavyCompression(value != 0)) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::DrcDefaultPresentationMode => {
            let drc_pres_mode = AacDrcParameterHandling::from(value);
            if let Err(result) = params.set_param(Param::DrcDefaultPresentationMode(drc_pres_mode))
            {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::DrcEncTargetLevel => {
            if let Ok(drc_enc_target_lvl) = i8::try_from(value) {
                if let Err(result) = params.set_param(Param::DrcEncTargetLevel(drc_enc_target_lvl))
                {
                    result
                } else {
                    AacDecoderError::Ok
                }
            } else {
                AacDecoderError::SetParamFail
            }
        }
        ParamBind::UnidrcSetEffect => {
            if let Err(result) = params.set_param(Param::UnidrcSetEffect(
                (i8::try_from(value).unwrap()).into(),
            )) {
                result
            } else {
                AacDecoderError::Ok
            }
        }
        ParamBind::UnidrcAlbumMode => {
            if let Err(result) = params.set_param(Param::UnidrcAlbumMode(value != 0)) {
                result
            } else {
                AacDecoderError::Ok
            }
        }

        ParamBind::ConcealMethod => {
            if let Err(result) = params.set_param(Param::ConcealMethod(value.into())) {
                result
            } else {
                AacDecoderError::Ok
            }
        }

        _ => panic!("Unupported parameter type"),
    }
}

#[no_mangle]
pub extern "C" fn bindings_aacDecoderParams_adjust_params(
    params: &mut Params,
    config: &Config,
    core_frame_delay: u8,
) -> AacDecoderError {
    if let Err(result) = params.adjust_params(config, core_frame_delay) {
        result
    } else {
        AacDecoderError::Ok
    }
}

#[no_mangle]
pub extern "C" fn bindings_aacDecoderParams_init(params: &mut Params) {
    params.init();
}

#[no_mangle]
pub extern "C" fn bindings_aacDecoderParams_restore(params: &mut Params) {
    params.restore();
}

#[no_mangle]
pub extern "C" fn bindings_is_limiter_active(params: &Params, ac_flags: u32) -> bool {
    params.is_limiter_active(ACFlags::from_bits(ac_flags).unwrap())
}

#[no_mangle]
pub extern "C" fn bindings_is_mpeg4_esbr_active(params: &Params, decoder_config: &Config) -> bool {
    params.is_mpeg4_esbr_active(decoder_config)
}

#[no_mangle]
pub extern "C" fn bindings_max_target_channels(params: &Params, decoder_config: &Config) -> u8 {
    params.max_target_channels(decoder_config)
}
