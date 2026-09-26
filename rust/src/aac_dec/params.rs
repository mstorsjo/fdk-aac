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
//! AAC decoder parameters handling
//!
//! This module manages the parameters of the AAC decoder and provides mechanisms
//! for updating and signaling changes to the internal decoder configuration.
//!
//! Decoder parameters can be updated using the `set_param()` method. This method
//! modifies the relevant sub-struct within the user configuration, which holds
//! the current user-defined parameter values. At this stage, the internal decoder
//! configuration is *not* directly updated.
//!
//! Instead, when a parameter is set, the `has_changed` member of the corresponding
//! sub-struct is flagged to indicate that the internal decoder state requires an update.
//! This ensures that the decoder's internal parameters remain consistent with user
//! changes without immediate reconfiguration.
//!
//! The actual update of the internal decoder configuration occurs at the beginning
//! of the next `decode_frame()` call. During this process, the `has_changed` flag
//! is checked, and if it was set, the internal parameters are synchronized with the
//! values from the `user_params` structure.

use crate::{
    aac_dec::{
        conceal::{ConcealmentMethod, ConcealmentParams},
        config::Config,
        constants as aac_constants,
        drc::{AacDrcParameterHandling, AacDrcParams},
        error_codes::AacDecoderError,
    },
    common::{channel_order::ChannelOrder, flags::*},
    drc_dec::selection_process::DrcEffectTypeRequest,
    pcm_dmx::{DualChannelMode, PcmDmxParam, PcmDmxParams, ProfileType as PcmDmxProfileType},
    td_limiter::{TDLIMIT_MAX_ATTACK_MS, TDLIMIT_MIN_ATTACK_MS, TDLIMIT_MIN_RELEASE_MS},
};

/// AAC decoder setting parameters
#[repr(C)]
pub enum Param {
    /// Defines how the decoder processes two channel signals
    PcmDualChannelOutputMode(DualChannelMode),

    /// Output buffer channel ordering
    PcmOutputChannelMapping(ChannelOrder),

    /// Minimum number of PCM output channels
    PcmMinOutputChannels(u8),

    /// Maximum number of PCM output channels
    PcmMaxOutputChannels(u8),

    /// Enable signal level limiting
    PcmLimiterEnable(LimiterMode),

    /// Signal level limiting attack time in ms. Must be between 1 to 15 ms.
    PcmLimiterAttackTime(u8),

    /// Signal level limiting release time in ms. Must be >0ms.
    PcmLimiterReleaseTime(u8),

    /// See `AacMdProfile` for all available values
    MetadataProfile(AacMdProfile),

    /// Time in ms after which meta-data will reset to default
    MetadataExpiryTime(u16),

    /// Error concealment: Processing method
    ConcealMethod(ConcealmentMethod),

    /// MPEG-4 / MPEG-D Dynamic Range Control: Boosting gain values between 0 and 127
    DrcBoostFactor(u8),

    /// MPEG-4 / MPEG-D DRC: Attenuating gain values between 0 and 127
    DrcAttenuationFactor(u8),

    /// Target reference level / decoder target loudness. -10 to -31.75 dB, encoded as 40 to 127
    DrcReferenceLevel(i8),

    /// En-/Disable DVB specific heavy compression
    DrcHeavyCompression(bool),

    /// Default presentation mode (DRC parameter handling)
    DrcDefaultPresentationMode(AacDrcParameterHandling),

    /// Encoder target level, assumed at encoder for deriving limiting gains, for light compression
    DrcEncTargetLevel(i8),

    /// MPEG-D DRC: Request a DRC effect type for selection of a DRC set
    UnidrcSetEffect(DrcEffectTypeRequest),

    /// MPEG-D DRC: Enable album mode
    UnidrcAlbumMode(bool),

    /// Ignore buffer fullness parameter for streaming
    TpdecParamIgnoreBufferFullness(bool),

    /// Enhance synchronization robustness by requiring two valid sync words in expected positions
    /// for bitstreams
    TpdecCheckTwoSyncs(bool),

    /// Target Layout index for audio output. 0 (default), 1, 2, 6, or 7
    TargetLayoutCicp(u8),
}

/// Metadata profiles
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum AacMdProfile {
    /// The standard profile creates a mixdown signal based on the
    /// advanced downmix metadata (from a DSE). The equations and default
    /// values are defined in ISO/IEC 14496:3 Ammendment 4. Any other
    /// (legacy) downmix metadata will be ignored. No other parameter will
    /// be modified.
    MpegStandard,
    /// This profile behaves identical to the standard profile if advanced
    /// downmix metadata (from a DSE) is available. If not, the
    /// matrix_mixdown information embedded in the program configuration
    /// element (PCE) will be applied. If neither is the case, the module
    /// creates a mixdown using the default coefficients as defined in
    /// ISO/IEC 14496:3 AMD 4. The profile can be used to support legacy
    /// digital TV (e.g. DVB) streams.
    MpegLegacy,
    /// Similar to the `MpegLegacy` profile but if both
    /// the advanced (ISO/IEC 14496:3 AMD 4) and the legacy (PCE) MPEG
    /// downmix metadata are available the latter will be applied.
    MpegLegacyPrio,
    /// Downmix creation as described in ABNT NBR 15602-2. But if advanced
    /// downmix metadata (ISO/IEC 14496:3 AMD 4) is available it will be
    /// preferred because of the higher resolutions. In addition the
    /// metadata expiry time will be set to the value defined in the ARIB
    /// standard (see `MdExpiryParams::time`).
    AribJapan,
}

impl TryFrom<i32> for AacMdProfile {
    type Error = AacDecoderError;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::MpegStandard),
            1 => Ok(Self::MpegLegacy),
            2 => Ok(Self::MpegLegacyPrio),
            3 => Ok(Self::AribJapan),
            _ => Err(AacDecoderError::SetParamFail),
        }
    }
}
/// UniDRC parameters
#[repr(C)]
#[derive(Debug)]
pub(super) struct UniDrcParams {
    pub(super) target_loudness: f32,
    pub(super) compress_factor: f32,
    pub(super) boost_factor: f32,
    pub(super) effect_type: DrcEffectTypeRequest,
    pub(super) is_album_mode: bool,
    // Signals that internal decoder config needs to be updated.
    pub(super) has_changed: bool,
}

impl Default for UniDrcParams {
    fn default() -> Self {
        UniDrcParams {
            // -24 dBFS for TV sets or equivalent devices.
            target_loudness: -24.0,
            // Fully apply compress factors.
            compress_factor: 1.0,
            // Fully apply boost factors.
            boost_factor: 1.0,
            // Disable MPEG-D DRC, but automatically enable DRC if necessary
            // to prevent clipping.
            effect_type: DrcEffectTypeRequest::None,
            // Album mode is disabled by default.
            is_album_mode: false,
            has_changed: true,
        }
    }
}

// Metadata expiry time
#[repr(C)]
#[derive(Debug)]
struct MdExpiryParams {
    // Metadata expiry time in milli-seconds.
    time: u16,
    // Signals that internal decoder config needs to be updated.
    has_changed: bool,
}

impl Default for MdExpiryParams {
    fn default() -> Self {
        MdExpiryParams {
            time: 0,
            has_changed: true,
        }
    }
}

/// Defines the modes for the signal level limiter.
///
/// The limiter mode controls whether the signal limiter is automatically
/// applied, explicitly enabled, or explicitly disabled.
#[repr(i8)]
#[derive(Debug, PartialEq)]
pub enum LimiterMode {
    Auto = -1,
    Off = 0,
    On = 1,
}

impl TryFrom<i32> for LimiterMode {
    type Error = AacDecoderError;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            -1 => Ok(LimiterMode::Auto),
            0 => Ok(LimiterMode::Off),
            1 => Ok(LimiterMode::On),
            _ => Err(AacDecoderError::SetParamFail),
        }
    }
}

/// Limiter parameters
#[repr(C)]
#[derive(Debug)]
pub(super) struct LimiterParams {
    mode: LimiterMode,
    attack_time: u32,
    release_time: u32,
    has_changed: bool,
}

impl Default for LimiterParams {
    fn default() -> Self {
        LimiterParams {
            mode: LimiterMode::Auto,
            attack_time: 0,
            release_time: 0,
            has_changed: true,
        }
    }
}

impl LimiterParams {
    /// Gets attack time (in ms).
    pub(super) fn attack_time(&self) -> u32 {
        self.attack_time
    }

    /// Gets release time (in ms).
    pub(super) fn release_time(&self) -> u32 {
        self.release_time
    }

    pub(super) fn has_changed(&self) -> bool {
        self.has_changed
    }

    /// Set the status of the `LimiterParams`.
    pub(super) fn do_change(&mut self, val: bool) {
        self.has_changed = val;
    }
}

/// AAC decoder parameters handling
#[repr(C)]
#[derive(Debug)]
pub struct Params {
    pub conceal: ConcealmentParams,
    pub(super) aac_drc: AacDrcParams,
    pub(super) pcm_dmx: PcmDmxParams,

    pub(super) uni_drc: UniDrcParams,
    md_expiry: MdExpiryParams,
    pub(super) limiter: LimiterParams,

    pub(super) channel_map_order: ChannelOrder,
    min_target_channels: u8,
    max_target_channels: u8,

    pub(super) bs_delay: u8,
}

impl Default for Params {
    fn default() -> Self {
        Params {
            conceal: ConcealmentParams::default(),
            aac_drc: AacDrcParams::default(),
            pcm_dmx: PcmDmxParams::default(),

            uni_drc: UniDrcParams::default(),
            md_expiry: MdExpiryParams::default(),
            limiter: LimiterParams::default(),

            channel_map_order: ChannelOrder::Wav,
            min_target_channels: 0,
            max_target_channels: 0,

            bs_delay: 0,
        }
    }
}

impl Params {
    /// Creates new Params instance.
    pub fn new() -> Self {
        Params::default()
    }

    /// Initializes the pre-allocated Params to default values.
    pub fn init(&mut self) {
        *self = Default::default();
    }

    /// Updates a specific decoder parameter.
    ///
    /// This method modifies the user parameters and marks the corresponding
    /// subsystem as needing an update. The internal decoder configuration will
    /// be updated at the beginning of the next `decode_frame()` call.
    ///
    /// # Parameters
    ///
    /// - `param`: The parameter to be updated, specified as a variant of the `Param` enum.
    ///
    /// # Return
    ///
    /// - `AacDecoderError`: Returns an error if the parameter value is invalid or the update
    ///   process fails.
    pub fn set_param(&mut self, param: Param) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        // Configure the subsystems
        match param {
            Param::PcmDualChannelOutputMode(mode) => {
                self.pcm_dmx.set(PcmDmxParam::DmxDualChannelMode(mode))?;
            }

            Param::PcmMinOutputChannels(min_ch) => {
                if min_ch > aac_constants::MAX_CHANNELS as u8 {
                    return Err(AacDecoderError::SetParamFail);
                }

                self.pcm_dmx
                    .set(PcmDmxParam::MinNumberOfOutputChannels(min_ch))?;

                self.min_target_channels = min_ch;
                self.uni_drc.has_changed = true;
                self.limiter.has_changed = true;
            }

            Param::PcmMaxOutputChannels(max_ch) => {
                if max_ch > aac_constants::MAX_CHANNELS as u8 {
                    return Err(AacDecoderError::SetParamFail);
                }

                self.pcm_dmx
                    .set(PcmDmxParam::MaxNumberOfOutputChannels(max_ch))?;

                error_status = self.aac_drc.set_max_output_channels(max_ch as u32);
                self.max_target_channels = max_ch;
                self.uni_drc.has_changed = true;
                self.limiter.has_changed = true;
            }

            Param::PcmLimiterEnable(limiter_mode) => {
                self.limiter.mode = limiter_mode;
                self.limiter.has_changed = true;
            }

            Param::PcmLimiterAttackTime(limiter_attack_time) => {
                if !(TDLIMIT_MIN_ATTACK_MS as u8..=TDLIMIT_MAX_ATTACK_MS as u8)
                    .contains(&limiter_attack_time)
                {
                    return Err(AacDecoderError::SetParamFail);
                }
                self.limiter.attack_time = limiter_attack_time.into();
                self.limiter.has_changed = true;
            }

            Param::PcmLimiterReleaseTime(limiter_release_time) => {
                if limiter_release_time < TDLIMIT_MIN_RELEASE_MS as u8 {
                    return Err(AacDecoderError::SetParamFail);
                }
                self.limiter.release_time = limiter_release_time.into();
                self.limiter.has_changed = true;
            }

            Param::MetadataProfile(profile) => {
                // Expiry time is in ms (0: don't change)
                let mut md_expiry = 0;

                let dmx_profile = match profile {
                    AacMdProfile::MpegStandard => PcmDmxProfileType::Standard,
                    AacMdProfile::MpegLegacy => PcmDmxProfileType::MatrixMix,
                    AacMdProfile::MpegLegacyPrio => PcmDmxProfileType::ForceMatrixMix,
                    AacMdProfile::AribJapan => {
                        md_expiry = 550; // ms
                        PcmDmxProfileType::AribJapan
                    }
                };

                if self
                    .pcm_dmx
                    .set(PcmDmxParam::DmxProfileSetting(dmx_profile))
                    .is_err()
                {
                    return Err(AacDecoderError::SetParamFail);
                }

                if md_expiry > 0 {
                    self.md_expiry.time = md_expiry;
                    self.md_expiry.has_changed = true;
                }
            }

            Param::MetadataExpiryTime(expiry_time) => {
                self.md_expiry.time = expiry_time;
                self.md_expiry.has_changed = true;
            }

            Param::PcmOutputChannelMapping(channel_order) => {
                self.channel_map_order = channel_order;
            }

            // Target layout switch
            Param::TargetLayoutCicp(target_layout) => {
                let num_channels = match target_layout {
                    0 | 1 | 2 | 6 => target_layout,
                    7 => 8,
                    _ => return Err(AacDecoderError::SetParamFail),
                };

                // Target layout refers to CICP channel order.
                self.channel_map_order = ChannelOrder::Cicp;

                if self.set_param(Param::PcmMinOutputChannels(num_channels)) != Ok(()) {
                    return Err(AacDecoderError::SetParamFail);
                }
                if self.set_param(Param::PcmMaxOutputChannels(num_channels)) != Ok(()) {
                    return Err(AacDecoderError::SetParamFail);
                }
            }

            Param::DrcAttenuationFactor(attenuation_factor) => {
                // DRC compression factor (where 0 is no and 127 is max compression)
                if attenuation_factor > 127 {
                    return Err(AacDecoderError::SetParamFail);
                }
                error_status = self.aac_drc.set_cut(attenuation_factor.into());
                self.uni_drc.compress_factor = attenuation_factor as f32 / 127.0;
                self.uni_drc.has_changed = true;
            }

            Param::DrcBoostFactor(boost_factor) => {
                // DRC boost factor (where 0 is no and 127 is max boost)
                if boost_factor > 127 {
                    return Err(AacDecoderError::SetParamFail);
                }
                error_status = self.aac_drc.set_boost(boost_factor.into());
                self.uni_drc.boost_factor = boost_factor as f32 / 127.0;
                self.uni_drc.has_changed = true;
            }

            Param::DrcReferenceLevel(ref_level) => {
                if (0..40).contains(&ref_level) {
                    // allowed range=> { -10 to -31.75 dB
                    return Err(AacDecoderError::SetParamFail);
                }
                // DRC target reference level quantized in 0.25dB steps using values
                // [40..127]. Negative values switch off loudness normalisation. Negative
                // values also switch off MPEG-4 DRC, while MPEG-D DRC can be separately
                // switched on/off with AACFlags::UNIDRC_SET_EFFECT

                error_status = self.aac_drc.set_target_ref_level(ref_level);

                // Set target loudness also for MPEG-D DRC.
                self.uni_drc.target_loudness = ref_level as f32 * -0.25;
                self.uni_drc.has_changed = true;
            }

            Param::DrcHeavyCompression(heavy_compression) => {
                // Don't need to overwrite cut/boost values
                error_status = self.aac_drc.set_apply_heavy_compression(heavy_compression);
            }

            Param::DrcDefaultPresentationMode(pres_mode) => {
                // DRC default presentation mode
                error_status = self.aac_drc.set_presentation_mode(pres_mode);
            }

            Param::DrcEncTargetLevel(enc_target_level) => {
                if !(-1..127).contains(&enc_target_level) {
                    return Err(AacDecoderError::SetParamFail);
                }
                // Encoder target level for light (i.e. not heavy) compression =>
                // Target reference level assumed at encoder for deriving limiting gains
                error_status = self
                    .aac_drc
                    .set_encoder_target_level(if enc_target_level < 0 {
                        127
                    } else {
                        enc_target_level.try_into().unwrap()
                    });
            }

            Param::UnidrcSetEffect(effect_type) => {
                self.uni_drc.effect_type = effect_type;
                self.uni_drc.has_changed = true;
            }

            Param::UnidrcAlbumMode(album_mode) => {
                self.uni_drc.is_album_mode = album_mode;
                self.uni_drc.has_changed = true;
            }

            Param::ConcealMethod(method) => error_status = self.conceal.set_conceal_method(method),

            _ => error_status = Err(AacDecoderError::SetParamFail),
        }

        error_status
    }

    /// Adjust parameters.
    ///
    /// This function is called for every `decode_frame()` call.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: Decoder configuration derived from Audio Specific Config.
    /// - `core_frame_delay`: AAC code delay in frames.
    ///
    /// # Return
    ///
    /// - `AacDecoderError`: Returns an error if the parameter value is invalid or the update
    ///   process fails.
    pub fn adjust_params(
        &mut self,
        decoder_config: &Config,
        core_frame_delay: u8,
    ) -> Result<(), AacDecoderError> {
        let mut error_status = Ok(());

        let bs_delay: u8 = core_frame_delay
            + if self.is_mpeg4_esbr_active(decoder_config) {
                1
            } else {
                0
            };
        if self.is_mpeg4_esbr_active(decoder_config) && (bs_delay != 1) {
            error_status = Err(AacDecoderError::SetParamFail);
        }

        if self.bs_delay != bs_delay {
            self.bs_delay = bs_delay;
            if Ok(()) != self.aac_drc.set_bs_delay(bs_delay) {
                error_status = Err(AacDecoderError::SetParamFail);
            }

            if self
                .pcm_dmx
                .set(PcmDmxParam::DmxBsDataDelay(bs_delay))
                .is_err()
            {
                error_status = Err(AacDecoderError::SetParamFail);
            }
        }

        if self.md_expiry.has_changed {
            // Determine the number of expiry frames from expiry time.
            let num_md_exp_frames: u32 = if self.md_expiry.time > 0 {
                ((decoder_config.sampling_frequency * self.md_expiry.time as u32) as f32
                    / (f32::from(decoder_config.samples_per_frame) * 1000.0))
                    .ceil() as u32
            } else {
                0 // disabled
            };

            if self
                .aac_drc
                .set_data_expiry_time(num_md_exp_frames)
                .is_err()
            {
                error_status = Err(AacDecoderError::SetParamFail);
            }

            if self
                .pcm_dmx
                .set(PcmDmxParam::DmxBsDataExpiryFrame(num_md_exp_frames))
                .is_err()
            {
                error_status = Err(AacDecoderError::SetParamFail);
            }

            self.md_expiry.has_changed = false;
        }

        error_status
    }

    /// Flags all sub-structures for a reset and signals reconfiguration.
    pub fn restore(&mut self) {
        self.conceal.has_changed = true;
        self.aac_drc.has_changed = true;
        self.pcm_dmx.has_changed = true;
        self.uni_drc.has_changed = true;
        self.md_expiry.has_changed = true;
        self.limiter.has_changed = true;
        self.bs_delay = 0;
    }

    /// Determines whether the limiter is currently active based on the configuration.
    ///
    /// # Parameters
    ///
    /// - `ac_flags`: Flags to provide information about audio codec.
    ///
    /// # Return
    ///
    /// - `true` if limiter is active, `false` if limiter is inactive.
    pub fn is_limiter_active(&self, ac_flags: ACFlags) -> bool {
        match self.limiter.mode {
            LimiterMode::Auto => !ac_flags.intersects(ACFlags::LD | ACFlags::ELD),
            LimiterMode::Off => false,
            _ => true,
        }
    }

    /// Determines whether mpeg4-esbr shall be active based on the configuration.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: Decoder configuration derived from Audio Specific Config.
    ///
    /// # Return
    ///
    /// - `true` if mpeg4-esbr is active, `false` if mpeg4-esbr is inactive.
    pub fn is_mpeg4_esbr_active(&self, decoder_config: &Config) -> bool {
        decoder_config.ac_flags.contains(ACFlags::MPEG4_ESBR)
            && ((self.conceal.conceal_method() == ConcealmentMethod::None)
                || (self.conceal.conceal_method() == ConcealmentMethod::Inter))
    }

    /// Determines the maximum number of possible target channels based on the configuration.
    ///
    /// # Parameters
    ///
    /// - `decoder_config`: Decoder configuration derived from Audio Specific Config.
    ///
    /// # Return
    ///
    /// - Maximum number of target channels determined from `decoder_config` and user parameters.
    pub fn max_target_channels(&self, decoder_config: &Config) -> u8 {
        let mut num_channels: u8 = decoder_config.num_channels;
        if (!decoder_config
            .ac_flags
            .intersects(ACFlags::USAC | ACFlags::ER))
            || decoder_config
                .ac_flags
                .intersects(ACFlags::MPS_PRESENT | ACFlags::PS_PRESENT)
        {
            num_channels = num_channels.max(2);
        }
        num_channels = num_channels.max(self.min_target_channels);
        if self.max_target_channels > 0 {
            num_channels = num_channels.min(self.max_target_channels);
        }
        num_channels
    }
}
