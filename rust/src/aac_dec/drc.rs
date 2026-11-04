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
//! AAC dynamic range control (DRC)

use crate::aac_dec::{constants::MAX_CHANNELS, error_codes::AacDecoderError};
use crate::common::{audio_channel_type::AudioChannelType, bitstream::Bitstream};
use itertools::izip;

pub const MAX_DRC_BANDS: usize = 16; // 2^LEN_DRC_BAND_INCR (LEN_DRC_BAND_INCR = 4)
const DRC_BLOCK_LEN: u16 = 1024;
const DRC_BAND_MULT: u16 = 4;
const DRC_BLOCK_LEN_DIV_BAND_MULT: u16 = DRC_BLOCK_LEN / DRC_BAND_MULT;
const DVB_ANC_DATA_SYNC_BYTE: u32 = 0xBC; // DVB ancillary data sync byte.
const DVB_COMPRESSION_SCALE: u8 = 8; // 48,164 dB

// For parameter conversion.
const DRC_PARAMETER_BITS: u8 = 7;
const DRC_MAX_QUANT_STEPS: u16 = 1 << DRC_PARAMETER_BITS;
const DRC_MAX_QUANT_FACTOR: f32 = DRC_MAX_QUANT_STEPS as f32 - 1.0_f32;
const DRC_PARAM_QUANT_STEP: f32 = 1.0_f32 / DRC_MAX_QUANT_FACTOR;
const DRC_PARAM_SCALE: u8 = 1;
const DRC_PARAM_GAIN: f32 = (1 << DRC_PARAM_SCALE) as f32;
const DRC_SCALING_MAX: f32 = DRC_PARAM_QUANT_STEP / DRC_PARAM_GAIN * 127.0_f32;

const DRC_MAX_REFERENCE_LEVEL: i8 = 127;
const DRC_DFLT_ENC_TARGET_LEVEL: u8 = 127;
const DRC_HEAVY_THRESHOLD_DB: i32 = 10;
const DRC_DFLT_EXPIRY_FRAMES: u32 = 0; // Default DRC data expiry time in AAC frames

/// DRC metadata type.
#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub enum AacDrcPayloadType {
    #[default]
    UnknownPayload = 0,
    MpegDrcExtData = 1,
    DvbDrcAncData = 2,
}

/// MPEG-4 DRC: Default presentation mode (DRC parameter handling). Defines the handling of the DRC
/// parameters boost factor, attenuation factor and heavy compression, if no presentation mode is
/// indicated in the bitstream.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
pub enum AacDrcParameterHandling {
    #[default]
    // DRC parameter handling disabled, all parameters are applied as requested.
    Disabled = -1,
    // Apply changes to requested DRC parameters to prevent clipping.
    Enabled = 0,
    // DRC Presentation mode 1
    Mode1 = 1,
    // DRC Presentation mode 2
    Mode2 = 2,
    // Apply changes to requested DRC parameters for Android Open Source Project (AOSP).
    Aosp = 3,
}

/// DRC presentation mode. According to ETSI TS 101 154, this field indicates whether light
/// (MPEG-4 Dynamic Range Control tool) or heavy compression (DVB heavy compression) dynamic
/// range control shall take priority on the outputs. For details, see ETSI TS 101 154,
/// table C.33. Possible values are:
/// -1: No corresponding metadata found in the bitstream.
/// 0: DRC presentation mode not indicated
/// 1: DRC presentation mode 1
/// 2: DRC presentation mode 2
/// 3: Reserved
#[repr(i8)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd)]
pub enum AacDrcPresentationMode {
    #[default]
    /// No corresponding metadata found in the bitstream.
    NotPresent = -1,
    // Apply changes to requested DRC parameters to prevent clipping.
    Enabled = 0,
    // DRC Presentation mode 1
    Mode1 = 1,
    // DRC Presentation mode 2
    Mode2 = 2,
    // Apply changes to requested DRC parameters.
    Reserved = 3,
}

/// DRC data specific to a single channel.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
struct AacDrcChannelData {
    num_bands: usize,
    band_top: [u16; MAX_DRC_BANDS],
    interpolation_scheme: i16,
    value: [u8; MAX_DRC_BANDS],
    data_type: AacDrcPayloadType,
}

impl AacDrcChannelData {
    /// Create a new AacDrcChannelData instance.
    fn new() -> Self {
        AacDrcChannelData::default()
    }

    fn determine_factor(
        &mut self,
        aac_drc_params: &AacDrcParams,
        prog_ref_level: i8,
        factor: &mut [f32],
    ) {
        let mut norm = 1_f32;

        // If program reference normalization is done in the digital domain,
        // modify factor to perform normalization.
        // `prog_ref_level` can alternatively be passed to the system for modification of
        // the level in the analog domain.
        // Analog level modification avoids problems with
        // reduced DAC SNR (if signal is attenuated) or clipping (if signal is boosted).
        if aac_drc_params.target_ref_level >= 0 {
            // 0.5^((targetRefLevel - progRefLevel)/24)
            let k = -0.02888113252333105456_f32; // = ln(0.5)/24
            norm = (k * f32::from(aac_drc_params.target_ref_level - prog_ref_level)).exp();
        }

        // Calc scale factors.
        for (dcr_val_item, factor_item) in
            izip!(self.value.iter(), factor.iter_mut()).take(self.num_bands)
        {
            let drc_val = *dcr_val_item;
            let mut fact = norm;

            if aac_drc_params.apply_heavy_compression
                && self.data_type == AacDrcPayloadType::DvbDrcAncData
            {
                // dBY[val_y] = pow(0.95483867181f, val_y) * 0.99990790084f;
                let db_y_table: [f32; 16] = [
                    0.99990790084000003_f32,
                    0.95475073197039084_f32,
                    0.91163292082423331_f32,
                    0.87046236729808191_f32,
                    0.83115113065148893_f32,
                    0.79361524166464747_f32,
                    0.75777452327924422_f32,
                    0.72355241933940950_f32,
                    0.69087583106695394_f32,
                    0.65967496092160038_f32,
                    0.62988316351269447_f32,
                    0.60143680324394222_f32,
                    0.57427511838709810_f32,
                    0.54834009129426731_f32,
                    0.52357632447159241_f32,
                    0.49993092224961688_f32,
                ];
                let val_x = drc_val >> 4;
                let val_y = usize::from(drc_val & 0x0F);

                // Calculate the unscaled heavy compression factor.
                // compressionFactor = 48.164 - 6.0206*valX - 0.4014*valY dB
                // range: -48.166 dB to 48.164 dB
                if drc_val != 0x7F {
                    // -0.4014dB = 0.95483867181
                    // factor[band] = powf(0.95483867181f, (FLOAT)valY);
                    // -0.0008dB (48.164 - 6.0206*8 = -0.0008)
                    // factor[band] = 0.99990790084f * factor[band];
                    fact *= db_y_table[val_y]; // pow(0.95483867181f, valY) * 0.99990790084f;

                    // factor[band] = factor[band] * powf(2.0f, (FLOAT)(DVB_COMPRESSION_SCALE -
                    // valX));
                    fact *= 2.0_f32.powi(i32::from(DVB_COMPRESSION_SCALE) - i32::from(val_x));
                }
            } else if self.data_type == AacDrcPayloadType::MpegDrcExtData {
                // Apply the scaled dynamic range control words to factor.
                // If scaling drc_cut (or drc_boost), or control word drc_mantissa is 0,
                // then there is no dynamic range compression.
                //
                // If pDrcChData->drcSgn[band] is 1,
                // then gain is < 1 :  factor = 2^(-cut  * pDrcChData->drcMag[band] / 24)
                // if pDrcChData->drcSgn[band] is 0,
                // then gain is > 1 :  factor = 2^(boost * pDrcChData->drcMag[band] / 24)
                if (drc_val & 0x7F) > 0 {
                    let t_param_val = if drc_val & 0x80 != 0 {
                        0.0 - aac_drc_params.cut
                    } else {
                        aac_drc_params.boost
                    };
                    // factor[band] = powf(2.0f, ((1.0f / 192.0f) * tParamVal *
                    // (FLOAT)(drcVal & 0x7F)) * powf(2.0f, (FLOAT)(3 + DRC_PARAM_SCALE)));
                    let k: f32 = 3.61014156541638181988e-3; // = ln(2)/192
                    fact *= (k
                        * f32::from(1_u8 << (3 + DRC_PARAM_SCALE))
                        * t_param_val
                        * f32::from(drc_val & 0x7F))
                    .exp();
                }
            }
            *factor_item = fact;
        }
    }

    /// Initialize DRC control data for one channel.
    fn init_channel_data(&mut self) {
        self.num_bands = 1;
        self.band_top[0] = DRC_BLOCK_LEN_DIV_BAND_MULT - 1;
        self.value[0] = 0;
        self.interpolation_scheme = 0;
        self.data_type = AacDrcPayloadType::UnknownPayload;
    }
}

#[repr(C)]
#[derive(Default, Debug, Clone)]
/// DRC data specific to all channels present in ASC (Audio Specific Config).
///
/// # Examples
/// ```
/// use aac::aac_dec::drc::{AacDrcData, AacDrcParams, AacDrcPayloadType};
/// use aac::common::{
///     audio_channel_type::AudioChannelType,
///     bitstream::{Bitstream, Mode},
/// };
/// use itertools::izip;
///
/// const NUM_CH: usize = 1;
///
/// // FFT(x[], 32) of f(t) = 1.0 * sin(2*pi*9000*t), when Fs = 48kHz.
/// let mut spec: [f32; 16] = [
///     2.4328e-16, 1.5445, 8.6711e-16, 2.0584, 5.0356e-16, 5.3433, 7.9992, 4.9244, 1.7448e-15,
///     1.5989, 1.1802e-15, 0.98458, 8.295e-16, 0.76085, 1.9004e-15, 0.67753,
/// ];
/// let dbg_spec = spec.clone();
///
/// // Byte blob simulating a bitstream with a measured PRL=0 dB. See `spec[]` above.
/// // Byte blob meaning:
/// //  0b0      : pce_tag_present
/// //  0b0      : excluded_chns_present
/// //  0b0      : drc_bands_present
/// //  0b1      : prog_ref_level_present
/// //  0b0000000: prog_ref_level   // = 0 dBFS
/// //  0b0      : prog_ref_level_reserved_bits
/// //  0b0      : dyn_rng_sgn[0]
/// //  0b0000000: dyn_rng_ctl[0]
///
/// let mut bitstream_writer = Bitstream::new(32, Mode::Writer);
/// bitstream_writer.write(0b00010000000000000000, 20);
/// bitstream_writer.sync();
/// let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
/// bitstream_reader.init(bitstream_writer.buffer(), 20);
///
/// let act: [AudioChannelType; NUM_CH] = [AudioChannelType::Front; NUM_CH];
/// let pce_instance_tag = 255;
///
/// // Set a target reference level for `spec[]`.
/// let target_ref_level_db = -18;
/// let mut drc_params: AacDrcParams = Default::default();
/// drc_params.set_target_ref_level(4 * -target_ref_level_db);
///
/// let mut drc = AacDrcData::new(NUM_CH);
/// drc.init(&act, pce_instance_tag);
/// drc.reset();
/// drc.update_params(&mut drc_params);
///
/// drc.read(&mut bitstream_reader, 20, AacDrcPayloadType::MpegDrcExtData);
/// let drc_present = drc.decode();
/// drc.apply(&mut spec, 0, false);
///
/// assert!(drc_present == true);
/// assert!(drc.prog_ref_level() == 0);
///
/// // Test if output is with `target_ref_level_db` smaller than input.
/// let test_thresh: f32 = 0.1;
/// for (out, inp) in izip!(spec.iter(), dbg_spec.iter()) {
///     let db = 20.0 * ((*out) / (*inp)).log10();
///     assert!((f32::from(target_ref_level_db) - db).abs() < test_thresh);
/// }
/// ```
pub struct AacDrcData {
    // Module parameters that can be set by user (via SetParam API function).
    params: AacDrcParams,
    // Switch that controls dynamic range processing.
    enable: bool,
    // Flag indicating the change of a user or bitstream parameter.
    update: bool,
    // The number of DRC data threads extracted from the found payload elements.
    num_threads: u16,
    // Program reference level for all channels.
    prog_ref_level: [i8; 2],
    // Program reference level found in bitstream.
    prog_ref_level_present: bool,
    expiry_count: u32,
    // Counter that can be used to monitor the life time of the program reference level.
    prl_expiry_count: u32,
    // Presentation mode as defined in ETSI TS 101 154.
    pres_mode: AacDrcPresentationMode,
    channel_mapping: [u8; MAX_CHANNELS],
    drc_ch_data: [Vec<AacDrcChannelData>; 2],
    num_channels: u32,
    thread_bs: Vec<AacDrcPayload>,
    max_drc_threads: u16,
    factor: Vec<[f32; MAX_DRC_BANDS]>,
    pce_instance_tag: u8,
}

impl AacDrcData {
    /// Applies DRC metadata on `spectrum` data of a channel.
    ///
    /// # Parameters
    /// - `spectrum`: in/out buffer (slice) containing the spectrum data of the respective channel.
    /// - `channel`: current channel.
    /// - `is_sbr_enabled`: is Spectral Band Replication (SBR) enabled.
    pub fn apply(&mut self, spectrum: &mut [f32], channel: usize, is_sbr_enabled: bool) {
        if self.enable {
            let factor_ch = &mut self.factor[channel][..];
            self.drc_ch_data[0][channel].determine_factor(
                &self.params,
                self.prog_ref_level[0],
                factor_ch,
            );

            if !is_sbr_enabled {
                // Apply factor to spectral lines.
                // Short blocks must take care that bands fall on block boundaries!
                let mut bottom = 0;
                let spectrum_len = spectrum.len();
                for (band, fact_item) in factor_ch
                    .iter()
                    .enumerate()
                    .take(self.drc_ch_data[0][channel].num_bands)
                {
                    let top = spectrum_len
                        .min(usize::from(self.drc_ch_data[0][channel].band_top[band] + 1) << 2);
                    if *fact_item != 1.0f32 && top > bottom {
                        spectrum[bottom..top].iter_mut().for_each(|x| {
                            *x *= *fact_item;
                        });
                    }
                    bottom = top;
                }
            }
        }
    }

    /// Updates DRC parameters based on presentation mode.
    fn apply_pres_mode_on_params(&mut self) {
        // Derive downmix property.
        // aac_num_channels: number of channels in aac stream,
        // num_out_channels: number of output channels.
        let is_downmix =
            self.params.num_out_channels > 0 && self.num_channels > self.params.num_out_channels;
        let is_mono_downmix = is_downmix && (self.params.num_out_channels == 1);
        let is_stereo_downmix = is_downmix && (self.params.num_out_channels == 2);

        // Sanity check:
        // DRC presentation mode 1 and 2 is possible in default presentation mode.
        // DRC_PARAMETER_HANDLING_AOSP
        // - for a maximum of 6 channels (5.1)
        // - if encoder target level is DRC_DFLT_ENC_TARGET_LEVEL
        let pres_mode_param_handling = if (self.pres_mode == AacDrcPresentationMode::Mode1
            || self.pres_mode == AacDrcPresentationMode::Mode2)
            && !((self.params.default_presentation_mode == AacDrcParameterHandling::Aosp)
                && self.params.encoder_target_level != DRC_DFLT_ENC_TARGET_LEVEL)
            && !((self.params.default_presentation_mode == AacDrcParameterHandling::Aosp)
                && self.num_channels > 6)
        {
            AacDrcParameterHandling::from(self.pres_mode)
        } else {
            // No presentation mode -> use parameter handling specified by
            // AAC_DRC_DEFAULT_PRESENTATION_MODE.
            self.params.default_presentation_mode
        };

        match pres_mode_param_handling {
            AacDrcParameterHandling::Disabled => {
                // Use DRC parameters as requested.
            }
            AacDrcParameterHandling::Enabled => {
                //  dec_dmx: estimated headroom reduction due to downmix, format: -1/4*dB
                //  dec_dmx = floor(-4*20*log10(aac_num_channels/num_out_channels))
                let dec_dmx = if is_downmix {
                    f32::floor(
                        -4.0_f32
                            * 20.0_f32
                            * f32::log10(
                                self.num_channels as f32 / self.params.num_out_channels as f32,
                            ),
                    )
                } else {
                    0.0
                } as i32;

                // dec_hr: Full estimated (decoder) headroom reduction due to loudness
                // normalisation (DTL - PRL) and downmix. Format: -1/4*dB.
                let dec_hr = if self.params.target_ref_level >= 0 {
                    // If target level is provided
                    i32::from(self.params.target_ref_level) + dec_dmx
                        - i32::from(self.prog_ref_level[usize::from(self.params.bs_delay)])
                } else {
                    dec_dmx
                };

                if dec_hr < 0 {
                    // If headroom is reduced, use compression, but as little as possible.
                    // enc_hr: Headroom provided by encoder, format: -1/4 dB.
                    let enc_hr = (i32::from(self.params.encoder_target_level)
                        - i32::from(self.prog_ref_level[usize::from(self.params.bs_delay)]))
                    .min(0);
                    if enc_hr < dec_hr {
                        // Encoder provides more headroom than decoder needs.
                        // Derive scaling of light DRC.

                        // 0.0 < calcFactor_norm < 1.0
                        let mut calc_factor_norm = dec_hr as f32 / enc_hr as f32;

                        // Quantize to 128 steps.
                        // Convert to float value between 0 and 127.
                        calc_factor_norm =
                            f32::round(calc_factor_norm * DRC_MAX_REFERENCE_LEVEL as f32);
                        calc_factor_norm *= DRC_PARAM_QUANT_STEP / DRC_PARAM_GAIN;
                        // Use calc_factor_norm as lower limit.
                        self.params.cut = calc_factor_norm.max(self.params.cut);
                    } else {
                        // Encoder provides equal or less headroom than decoder needs.
                        // The time domain limiter must always be active in this case. It is
                        // assumed that the framework activates it by default.
                        self.params.cut = DRC_SCALING_MAX;
                        if (dec_hr - enc_hr) <= (-4 * DRC_HEAVY_THRESHOLD_DB) {
                            // Use heavy compression if headroom deficit is equal or
                            // higher than DRC_HEAVY_THRESHOLD_DB.
                            self.params.apply_heavy_compression = true;
                        }
                    }
                } else { // dec_hr >= 0
                     // No restrictions required, as headroom is not reduced.
                }
            }

            // Presentation mode 1 and 2 according to ETSI TS 101 154:
            // Digital Video Broadcasting (DVB); Specification for the use of Video
            // and Audio Coding in Broadcasting Applications based on the MPEG-2
            // Transport Stream, section C.5.4., "Decoding", and Table C.33. Also
            // according to amendment 4 to ISO/IEC 14496-3, section 4.5.2.14.2.4, and
            // Table AMD4.11.
            // ISO DRC            -> apply_heavy_compression = OFF (Use light compression,
            // MPEG-style) Compression_value  -> apply_heavy_compression = ON  (Use
            // heavy compression, DVB-style) scaling restricted -> cut = DRC_SCALING_MAX
            AacDrcParameterHandling::Mode1 => {
                // Presentation mode 1, Light:-31/Heavy:-23
                if self.params.target_ref_level >= 0 && self.params.target_ref_level < 124 {
                    // If target level is provided and > -31 dB, then playback up to -23 dB.
                    self.params.apply_heavy_compression = true;
                } else {
                    // If target level <= -31 dB or not provided, then playback -31 dB.
                    if is_mono_downmix || is_stereo_downmix {
                        // Stereo or mono downmixing.
                        self.params.cut = DRC_SCALING_MAX;
                    }
                }
            }
            AacDrcParameterHandling::Mode2 => {
                // Presentation mode 2, Light:-23/Heavy:-23
                self.params.apply_heavy_compression =
                    if self.params.target_ref_level >= 0 && self.params.target_ref_level < 124 {
                        // If target level is provided and > -31 dB, playback up to -23 dB.
                        if is_mono_downmix {
                            true
                        } else {
                            self.params.cut = DRC_SCALING_MAX;
                            false
                        }
                    } else {
                        // If target level <= -31 dB or not provided, then playback -31 dB.
                        if is_mono_downmix || is_stereo_downmix {
                            // Stereo or mono downmixing.
                            self.params.cut = DRC_SCALING_MAX;
                        }
                        false
                    };
            }
            AacDrcParameterHandling::Aosp => {
                if self.params.encoder_target_level != DRC_DFLT_ENC_TARGET_LEVEL {
                    // If target level > -31 dB
                    if self.params.target_ref_level < 124 {
                        // No stereo or mono downmixing. Calculated scaling of light DRC.
                        if !is_stereo_downmix && !is_mono_downmix {
                            // Use as little compression as possible.
                            self.params.cut = 0.0;
                            self.params.boost = 0.0;
                            if self.params.target_ref_level
                                < self.prog_ref_level[usize::from(self.params.bs_delay)]
                            {
                                let encoder_target_level =
                                    i8::try_from(self.params.encoder_target_level).unwrap();
                                if encoder_target_level < self.params.target_ref_level {
                                    //  ETL > target level > PRL
                                    let mut calc_factor_norm = f32::from(
                                        self.params.target_ref_level
                                            - self.prog_ref_level
                                                [usize::from(self.params.bs_delay)],
                                    ) / f32::from(
                                        encoder_target_level
                                            - self.prog_ref_level
                                                [usize::from(self.params.bs_delay)],
                                    );
                                    // 0 <= calcFactor < 0.5
                                    calc_factor_norm = (calc_factor_norm
                                        * f32::from(DRC_MAX_REFERENCE_LEVEL))
                                    .trunc()
                                        * DRC_PARAM_QUANT_STEP
                                        / DRC_PARAM_GAIN;
                                    // calcFactor is the lower limit
                                    self.params.cut = self.params.cut.max(calc_factor_norm);
                                    self.params.boost = self.params.cut;
                                } else {
                                    // target level > ETL > PRL
                                    self.params.cut = DRC_SCALING_MAX;
                                    self.params.boost = DRC_SCALING_MAX;
                                }
                            } else { // target level <= PRL
                                 // No restrictions required.
                            }
                        } else {
                            // Downmixing:
                            // If target level > -23 dB or mono downmix,
                            if self.params.target_ref_level < 92 || is_mono_downmix {
                                self.params.apply_heavy_compression = true;
                            } else {
                                // we perform a downmix, so we need at least full light DRC.
                                self.params.cut = DRC_SCALING_MAX;
                            }
                        }
                    } else {
                        // target level <= -31 dB
                        // playback - 31 dB: light DRC only needed if we perform downmixing.
                        if is_downmix {
                            // We do downmixing.
                            self.params.cut = DRC_SCALING_MAX;
                        }
                    }
                } else {
                    // If target level > -31 dB
                    if self.params.target_ref_level < 124 {
                        // No stereo or mono downmixing.
                        if !is_stereo_downmix && !is_mono_downmix {
                            // If target level > PRL
                            if self.params.target_ref_level
                                < self.prog_ref_level[usize::from(self.params.bs_delay)]
                            {
                                // at least, use light compression
                                self.params.cut = DRC_SCALING_MAX;
                            } else { // target level <= PRL
                                 // No restrictions required.
                            }
                        } else {
                            // Downmixing.
                            // If target level > -23 dB or mono downmix,
                            if self.params.target_ref_level < 92 || is_mono_downmix {
                                self.params.apply_heavy_compression = true;
                            } else {
                                // we perform a downmix, so we need at least full light DRC.
                                self.params.cut = DRC_SCALING_MAX;
                            }
                        }
                    } else {
                        // target level <= -31 dB
                        // We do downmixing.
                        if is_downmix {
                            self.params.cut = DRC_SCALING_MAX;
                        }
                    }
                }
            }
        } // match pres_mode_param_handling

        // With heavy compression, there is no scaling.
        // Scaling factors are set for notification only.
        if self.params.apply_heavy_compression {
            self.params.boost = DRC_SCALING_MAX;
            self.params.cut = DRC_SCALING_MAX;
        }
    }

    /// Get reference to DRC band_top[], specific to a channel.
    pub fn band_top(&self, delay_index: usize, channel: usize) -> &[u16] {
        &self.drc_ch_data[delay_index][channel].band_top
    }

    /// Decodes DRC metadata and maps it to a channel.
    ///
    /// # Return
    ///
    /// - `drc_present`: {true, false}
    pub fn decode(&mut self) -> bool {
        let mut drc_present = false;
        //  Keep previous prog_ref_level and pres_mode for update flag in
        //  drc_parameter_handling.
        let bs_delay = usize::from(self.params.bs_delay);
        let prev_prl = self.prog_ref_level[bs_delay];
        let prev_pm = self.pres_mode;

        let drc_payload_order: [AacDrcPayloadType; 2] = [
            AacDrcPayloadType::MpegDrcExtData,
            AacDrcPayloadType::DvbDrcAncData,
        ];

        for payload_order in drc_payload_order.iter() {
            'thread_loop: for thread in self
                .thread_bs
                .iter_mut()
                .take(usize::from(self.num_threads))
            {
                let mut num_excl_chns = 0;
                let payload_type = thread.channel_data.data_type;

                // Check for valid threads.
                if payload_type != *payload_order {
                    continue 'thread_loop;
                }

                // If PCE tag present
                if thread.pce_instance_tag != 255
                    && thread.pce_instance_tag != self.pce_instance_tag
                {
                    continue 'thread_loop; // don't accept
                }

                // Calculate number of excluded channels.
                if thread.excluded_chns_mask > 0 {
                    let mut excl_mask = thread.excluded_chns_mask;
                    for _ch in 0..self.num_channels {
                        num_excl_chns += excl_mask & 0x1;
                        excl_mask >>= 1;
                    }
                }

                // Map DRC bitstream information onto DRC channel information.
                if num_excl_chns < self.num_channels {
                    // last prog_ref_level transmitted is the one that is used.
                    // But it should really only be transmitted once per block!
                    if thread.prog_ref_level >= 0 {
                        self.prog_ref_level[bs_delay] = thread.prog_ref_level;
                        self.prog_ref_level_present = true;
                        self.prl_expiry_count = 0; // Got a new value -> Reset counter
                    }

                    if payload_type == AacDrcPayloadType::DvbDrcAncData {
                        // Announce the presentation mode of this valid thread.
                        self.pres_mode = thread.pres_mode;
                    }

                    if (self.params.expiry_frame == 0)
                        || (self.expiry_count < self.params.expiry_frame)
                    {
                        // SCE, CPE and LFE
                        'channel_mapping: for (ch, ch_map) in self
                            .channel_mapping
                            .iter()
                            .enumerate()
                            .take(self.num_channels as usize)
                        {
                            if (u32::from(*ch_map) >= self.num_channels)
                                || (thread.excluded_chns_mask & (1_u32 << *ch_map) != 0)
                            {
                                continue 'channel_mapping;
                            }

                            if payload_type != AacDrcPayloadType::DvbDrcAncData
                                || self.params.apply_heavy_compression
                            {
                                // Copy thread to channel.
                                self.drc_ch_data[bs_delay][ch] = thread.channel_data;
                                drc_present = true;
                            }
                        } // channel_mapping loop
                    }
                } // num_excl_chns < self.num_channels
            } // thread_loop
        } // drc_payload_order loop

        // Increment and check expiry counter for the program reference level.
        if self.params.expiry_frame > 0 {
            let is_expired = self.prl_expiry_count > self.params.expiry_frame;
            self.prl_expiry_count += 1;
            if is_expired {
                // The program reference level is too old, so set it back to the target level.
                self.prog_ref_level_present = false;
                self.prog_ref_level[bs_delay] = self.params.target_ref_level;
                self.prl_expiry_count = 0;
            }
        }

        // Increment and check expiry counter.
        if self.params.expiry_frame > 0 {
            let is_expired = self.prl_expiry_count >= self.params.expiry_frame;
            self.prl_expiry_count += 1;
            if is_expired {
                for ch_data in self.drc_ch_data[bs_delay]
                    .iter_mut()
                    .take(self.num_channels as usize)
                {
                    ch_data.init_channel_data();
                }
                self.expiry_count = 0;
            }
        }

        if self.prog_ref_level[bs_delay] != prev_prl || self.pres_mode != prev_pm {
            self.params.has_changed = true;
        }

        drc_present
    }

    /// Returns flag value of switch that controls dynamic range processing.
    pub(super) fn is_enabled(&self) -> bool {
        self.enable
    }
    /// Get factor, specific to a channel.
    pub fn factor(&self, channel: usize) -> &[f32] {
        &self.factor[channel]
    }

    /// Initializes an AacDrcData instance.
    ///
    /// # Parameters
    ///
    /// - `act`: speaker description tag for a channel element
    /// - `pce_instance_tag`: program config element (PCE) instance tag (read from bitstream)
    pub fn init(&mut self, act: &[AudioChannelType], pce_instance_tag: u8) {
        let num_channels = self.num_channels as usize;
        self.pce_instance_tag = pce_instance_tag;

        for ch in 0..num_channels {
            self.drc_ch_data[0][ch].init_channel_data();
            self.drc_ch_data[1][ch].init_channel_data();
            self.thread_bs[ch].init_payload();
        }
        // thread_bs should be initialized to `num_channels` (ideally last item in thread_bs
        // vector).
        self.thread_bs[num_channels].init_payload();
        self.params.init(); // Init params

        if pce_instance_tag != 255 {
            // ISO/IEC 14496-3 says:
            //    If a PCE is present, the exclude_mask bits correspond to the audio
            //    channels in the SCE, CPE, CCE and LFE syntax elements in the order of
            //    their appearance in the PCE. In the case of a CPE, the first transmitted
            //    mask bit corresponds to the first channel in the CPE, the second
            //    transmitted mask bit to the second channel. In the case of a CCE, a mask
            //    bit is transmitted only if the coupling channel is specified to be an
            //    independently switched coupling channel.
            //    Thus we have to convert the internal channel mapping from "canonical"
            //    MPEG to PCE order:
            let channel_type: [AudioChannelType; 4] = [
                AudioChannelType::Front,
                AudioChannelType::Side,
                AudioChannelType::Back,
                AudioChannelType::Lfe,
            ];
            let mut ch = 0;
            for ch_type in channel_type.iter() {
                for (act_item, ch_map) in
                    izip!(act.iter(), self.channel_mapping.iter_mut()).take(num_channels)
                {
                    if u32::from(*act_item) & 0x7_u32 == u32::from(*ch_type) {
                        *ch_map = ch;
                        ch += 1;
                    }
                }
            }
        } else {
            for (ch, ch_map) in self
                .channel_mapping
                .iter_mut()
                .enumerate()
                .take(num_channels)
            {
                *ch_map = ch as u8;
            }
        }

        self.prog_ref_level_present = false;
        self.prog_ref_level[1] = self.params.target_ref_level;
        self.prog_ref_level[0] = self.prog_ref_level[1];
        self.num_threads = 0;
        self.expiry_count = 0;
        self.pres_mode = AacDrcPresentationMode::default();
        self.enable = false;
        self.update = true;
    }

    /// Get DRC interpolation scheme used for a channel.
    pub fn interpolation_scheme(&self, delay_index: usize, channel: usize) -> i16 {
        self.drc_ch_data[delay_index][channel].interpolation_scheme
    }

    /// Creates a new AacDrcData instance.
    ///
    /// # Parameters
    ///
    /// - `num_channels`: number of AAC channels
    pub fn new(num_channels: usize) -> Self {
        let mut drc = AacDrcData::default();

        // Dynamic memory allocation for members of AacDrcData.
        drc.drc_ch_data[0] = vec![AacDrcChannelData::new(); num_channels];
        drc.drc_ch_data[1] = vec![AacDrcChannelData::new(); num_channels];
        drc.thread_bs = vec![AacDrcPayload::new(); num_channels + 1];
        drc.factor = vec![[0.0_f32; MAX_DRC_BANDS]; num_channels];

        // Initialize the members of AacDrcData.
        drc.num_channels = num_channels as u32;
        drc.max_drc_threads = num_channels as u16 + 1;

        drc
    }

    /// Get the number of scf bands for a channel.
    pub fn num_bands(&self, delay_index: usize, channel: usize) -> usize {
        self.drc_ch_data[delay_index][channel].num_bands
    }

    /// Returns DRC presentation mode (either set by parsing the bitstream or by the user).
    pub fn presentation_mode(&self) -> AacDrcPresentationMode {
        self.pres_mode
    }

    /// Returns program reference level for loudness normalization.
    /// The information is read from the bitstream, if `prog_ref_level_present` flag is `true`.
    /// If the program reference level is not present, it returns `-1`.
    pub fn prog_ref_level(&self) -> i8 {
        if self.prog_ref_level_present {
            self.prog_ref_level[self.params.bs_delay as usize]
        } else {
            -1
        }
    }

    /// Returns audio output loudness in the range `0 (0 dBFS)` to `127 (-31.75 dBFS)`.
    /// A value of `-1` indicates that no loudness metadata is present.
    pub fn output_loudness(&self) -> i8 {
        if self.prog_ref_level() < 0 {
            // No MPEG-4 DRC loudness metadata contained
            -1
        } else if self.params.target_ref_level < 0 {
            // Loudness normalization is off
            self.prog_ref_level()
        } else {
            self.params.target_ref_level
        }
    }

    /// Reads DRC metadata from `Bitstream`.
    ///
    /// # Parameters
    ///
    ///  - `bs`: Bitstream instance with valid internal data (in)
    ///  - `payload_length`: DRC payload length in bits
    ///  - `payload_type`: MPEG/DVB/Unknown DRC metadata. See `AacDrcPayloadType`
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        payload_length: isize,
        payload_type: AacDrcPayloadType,
    ) {
        let bs_anchor = bs.valid_bits();
        let num_channels = self.num_channels;
        if self.num_threads < self.max_drc_threads {
            let payload = &mut (self.thread_bs[usize::from(self.num_threads)]);
            payload.init_payload();
            match payload_type {
                AacDrcPayloadType::UnknownPayload => (),
                AacDrcPayloadType::MpegDrcExtData => {
                    if payload.parse(bs, num_channels)
                        && ((bs_anchor - bs.valid_bits()) <= payload_length)
                    {
                        self.num_threads += 1;
                    }
                }
                AacDrcPayloadType::DvbDrcAncData => {
                    if payload.read_compression(bs)
                        && ((bs_anchor - bs.valid_bits()) <= payload_length)
                    {
                        self.num_threads += 1;
                    }
                }
            }
        }
    }

    /// Resets DRC data.
    pub fn reset(&mut self) {
        if self.update {
            self.apply_pres_mode_on_params();

            // switch on/off processing
            self.enable = (self.params.boost > 0.0_f32)
                || (self.params.cut > 0.0_f32)
                || self.params.apply_heavy_compression
                || (self.params.target_ref_level >= 0);

            self.update = false;
        }

        if self.params.bs_delay != 0 {
            self.prog_ref_level[0] = self.prog_ref_level[1];
            for ch in 0..self.num_channels as usize {
                self.drc_ch_data[0][ch] = self.drc_ch_data[1][ch];
            }
        }

        self.num_threads = 0;
    }

    /// Updates internal members of `AacDrcData` either with values passed by user
    /// or by in-band parameter changes.
    ///
    /// # Parameters
    ///
    /// - `drc_params`: reference to `AacDrcParams` set by user
    ///
    /// # Examples
    /// ```
    /// use aac::aac_dec::drc::{AacDrcData, AacDrcParams};
    /// use aac::common::audio_channel_type::AudioChannelType;
    ///
    /// const NUM_CH: usize = 1;
    /// let act: [AudioChannelType; NUM_CH] = [AudioChannelType::Front; NUM_CH];
    /// let pce_instance_tag = 255;
    ///
    /// let mut drc = AacDrcData::new(NUM_CH);
    /// let mut drc_params = AacDrcParams::default();
    ///
    /// drc.init(&act, pce_instance_tag);
    ///
    /// // User parameter change.
    /// drc_params.init();
    /// assert!(drc_params.update_flag() == true);
    ///
    /// drc.update_params(&mut drc_params); // pass params to `self` and reset the `update` flag
    /// assert!(drc_params.update_flag() == false);
    ///
    /// // In-band parameter change.
    /// let mut drc_params = AacDrcParams::default();
    /// assert!(drc_params.update_flag() == true);
    ///
    /// drc.update_params(&mut drc_params); // pass params to `self` and reset the `update` flag
    /// assert!(drc_params.update_flag() == false);
    /// ```
    pub fn update_params(&mut self, drc_params: &mut AacDrcParams) {
        if drc_params.has_changed {
            // User parameter change
            drc_params.has_changed = false;
            self.params = *drc_params;
            self.prog_ref_level[self.params.bs_delay as usize] = drc_params.target_ref_level();
            self.update = true;
        } else if self.params.has_changed {
            // Inband parameter change
            self.params.has_changed = false;
            self.params.cut = drc_params.cut();
            self.params.boost = drc_params.boost();
            self.params.apply_heavy_compression = drc_params.apply_heavy_compression();
            self.update = true;
        }
    }
}

/// DRC user modifiable parameters.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct AacDrcParams {
    /// attenuation scale factor
    cut: f32,
    /// boost scale factor
    boost: f32,
    /// heavy compression (DVB) flag
    apply_heavy_compression: bool,
    /// target reference level for loudness normalization
    target_ref_level: i8,
    /// drc data expiry time in AAC frames
    expiry_frame: u32,
    /// drc bitstream delay 0, 1
    bs_delay: u8,
    /// specifies default presentation mode.
    default_presentation_mode: AacDrcParameterHandling,
    encoder_target_level: u8,
    /// number of output channels
    num_out_channels: u32,
    /// flag indicating the change of a user parameter
    pub has_changed: bool,
}

/// Default trait.
impl Default for AacDrcParams {
    fn default() -> Self {
        Self {
            cut: 0.0_f32,
            boost: 0.0_f32,
            apply_heavy_compression: false,
            target_ref_level: 96,
            expiry_frame: DRC_DFLT_EXPIRY_FRAMES,
            bs_delay: 0,
            default_presentation_mode: AacDrcParameterHandling::default(),
            encoder_target_level: DRC_DFLT_ENC_TARGET_LEVEL,
            num_out_channels: 0,
            has_changed: true,
        }
    }
}

impl AacDrcParams {
    /// Create a new AacDrcParams instance  with default values.
    pub fn new() -> Self {
        Self::default()
    }

    /// Init AacDrcParams with default values.
    pub fn init(&mut self) {
        *self = Default::default();
    }

    /// Returns attenuation scale factor.
    pub fn cut(&self) -> f32 {
        self.cut
    }

    /// Returns boost scale factor.
    pub fn boost(&self) -> f32 {
        self.boost
    }

    /// Returns target reference level for loudness normalization.
    pub fn target_ref_level(&self) -> i8 {
        self.target_ref_level
    }

    /// Returns heavy compression (DVB) flag.
    pub fn apply_heavy_compression(&self) -> bool {
        self.apply_heavy_compression
    }

    /// Returns the default presentation mode, if there is no mode embedded in the bitstream.
    /// See `AacDrcParameterHandling`.
    pub fn default_presentation_mode(&self) -> AacDrcParameterHandling {
        self.default_presentation_mode
    }

    /// Returns encoder target reference level.
    pub fn encoder_target_level(&self) -> u8 {
        self.encoder_target_level
    }

    /// Returns DRC bitstream delay: {0, 1}.
    pub fn bs_delay(&self) -> u8 {
        self.bs_delay
    }

    /// Returns DRC data expiry time in AAC frames.
    pub fn data_expiry_time(&self) -> u32 {
        self.expiry_frame
    }

    /// Returns number of output channels.
    pub fn max_output_channels(&self) -> u32 {
        self.num_out_channels
    }

    /// Returns the value of the `update` flag.
    pub fn update_flag(&self) -> bool {
        self.has_changed
    }

    /// Sets attenuation scale factor.
    pub fn set_cut(&mut self, val: f32) -> Result<(), AacDecoderError> {
        if !(0.0..=DRC_MAX_QUANT_FACTOR).contains(&val) {
            return Err(AacDecoderError::SetParamFail);
        }
        let tmp_param =
            ((1.0 / DRC_MAX_QUANT_FACTOR) * 2.0_f32.powi(-i32::from(DRC_PARAM_SCALE))) * val;
        if self.cut != tmp_param {
            self.cut = tmp_param;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets boost scale factor.
    pub fn set_boost(&mut self, val: f32) -> Result<(), AacDecoderError> {
        if !(0.0..=DRC_MAX_QUANT_FACTOR).contains(&val) {
            return Err(AacDecoderError::SetParamFail);
        }
        let tmp_param =
            ((1.0 / DRC_MAX_QUANT_FACTOR) * 2.0_f32.powi(-i32::from(DRC_PARAM_SCALE))) * val;
        if self.boost != tmp_param {
            self.boost = tmp_param;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets target reference level for loudness normalization.
    pub fn set_target_ref_level(&mut self, val: i8) -> Result<(), AacDecoderError> {
        if val < -DRC_MAX_REFERENCE_LEVEL {
            return Err(AacDecoderError::SetParamFail);
        }
        if val < 0 {
            self.target_ref_level = -1;
        } else if self.target_ref_level != val {
            self.target_ref_level = val;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets heavy compression (DVB) flag.
    pub fn set_apply_heavy_compression(&mut self, val: bool) -> Result<(), AacDecoderError> {
        if self.apply_heavy_compression != val {
            self.apply_heavy_compression = val;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets default presentation mode ( see `AacDrcParameterHandling`).
    pub fn set_presentation_mode(
        &mut self,
        val: AacDrcParameterHandling,
    ) -> Result<(), AacDecoderError> {
        if self.default_presentation_mode != val {
            self.default_presentation_mode = val;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets encoder target reference level.
    pub fn set_encoder_target_level(&mut self, val: u8) -> Result<(), AacDecoderError> {
        if val > DRC_DFLT_ENC_TARGET_LEVEL {
            return Err(AacDecoderError::SetParamFail);
        }
        if self.encoder_target_level != val {
            self.encoder_target_level = val;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets DRC bitstream delay: {0, 1}.
    pub fn set_bs_delay(&mut self, val: u8) -> Result<(), AacDecoderError> {
        if val > 1 {
            return Err(AacDecoderError::SetParamFail);
        }
        if self.bs_delay != val {
            self.bs_delay = val;
            self.has_changed = true;
        }
        Ok(())
    }

    /// Sets DRC data expiry time in AAC frames.
    pub fn set_data_expiry_time(&mut self, val: u32) -> Result<(), AacDecoderError> {
        if self.expiry_frame != val {
            self.expiry_frame = val;
            self.has_changed = true;
        }
        Ok(())
    }

    ///  Sets number of output channels.
    pub fn set_max_output_channels(&mut self, val: u32) -> Result<(), AacDecoderError> {
        if self.num_out_channels != val {
            self.num_out_channels = val;
            self.has_changed = true;
        }
        Ok(())
    }
}

/// Stores the DRC related payload data (extracted from bitstream).
#[repr(C)]
#[derive(Default, Debug, Clone)]
struct AacDrcPayload {
    excluded_chns_mask: u32,
    prog_ref_level: i8,
    /// Presentation mode: -1 (not present), 0 (not indicated), 1, 2, and 3 (reserved).
    pres_mode: AacDrcPresentationMode,
    pce_instance_tag: u8,
    channel_data: AacDrcChannelData,
}

impl AacDrcPayload {
    /// Creates a new AacDrcPayload instance.
    fn new() -> Self {
        AacDrcPayload::default()
    }

    fn init_payload(&mut self) {
        *self = Self::default();
        self.channel_data.init_channel_data();
    }

    #[inline(always)]
    /// Very small, particular routine for checking if a channel has to be excluded
    /// from DRC processing.
    fn parse_excluded_channel(bs: &mut Bitstream, exclude_mask: &mut u32, mask: &mut u32) {
        if bs.read_bit() != 0 {
            *exclude_mask |= *mask;
        }
        *mask <<= 1;
    }

    /// Parses the bitstream to find if there are channels
    /// to be excluded from DRC processing.
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data (in).
    /// - `num_channels`: Number of AAC channels (in)
    /// # Return
    ///
    /// - `AacDecoderError` in case that number of channels to be excluded is `>` than
    ///   `num_channels`.
    fn parse_excluded_channels(
        &mut self,
        bs: &mut Bitstream,
        num_channels: u32,
    ) -> AacDecoderError {
        let mut error = AacDecoderError::Ok;
        let mut exclude_mask = 0;
        let mut mask = 1;
        for _i in 0..7 {
            AacDrcPayload::parse_excluded_channel(bs, &mut exclude_mask, &mut mask);
        }

        // additional_excluded_chns
        while bs.read_bit() != 0 {
            if (mask >> num_channels) > 0 {
                error = AacDecoderError::ParseError;
                break;
            }
            for _i in 0..7 {
                AacDrcPayload::parse_excluded_channel(bs, &mut exclude_mask, &mut mask);
            }
        }

        if error == AacDecoderError::Ok {
            self.excluded_chns_mask = exclude_mask;
        }

        error
    }

    /// Updates AacDrcPayload instance, by parsing DRC parameters from bitstream.
    ///
    /// # Parameters
    ///
    ///  - `bs`: Bitstream instance with valid internal data (in)
    ///  - `num_channels`: Number of AAC channels (in)
    ///
    /// # Return
    ///
    ///  - `true` if parsing is succesful; `false` if parsing is not.
    fn parse(&mut self, bs: &mut Bitstream, num_channels: u32) -> bool {
        let mut num_bands;

        // pce_tag_present
        if bs.read_bit() != 0 {
            self.pce_instance_tag = bs.read(4) as u8;
            // Only one program supported.
            bs.read(4); // drc_tag_reserved_bits
        } else {
            self.pce_instance_tag = 255; // not present
        }

        // excluded_chns_present
        if bs.read_bit() != 0 {
            // get excluded_chn_mask
            if self.parse_excluded_channels(bs, num_channels) != AacDecoderError::Ok {
                return false;
            }
        } else {
            self.excluded_chns_mask = 0;
        }

        num_bands = 1;
        // drc_bands_present
        if bs.read_bit() != 0 {
            // dcr_band_incr
            num_bands += bs.read(4) as usize;
            // drc_interpolation_scheme
            self.channel_data.interpolation_scheme = bs.read(4) as i16;
            // band_top
            for band_top in self.channel_data.band_top.iter_mut().take(num_bands) {
                *band_top = bs.read(8) as u16; // drc_band_top[i]
            }
        } else {
            //... comprising the whole spectrum.
            self.channel_data.band_top[0] = DRC_BLOCK_LEN_DIV_BAND_MULT - 1;
        }

        self.channel_data.num_bands = num_bands;

        // prog_ref_level_present
        if bs.read_bit() != 0 {
            self.prog_ref_level = bs.read(7) as i8;
            bs.read_bit(); // prog_ref_level_reserved_bits
        } else {
            self.prog_ref_level = -1;
        }

        for drc_val in self.channel_data.value.iter_mut().take(num_bands) {
            *drc_val = (bs.read_bit() << 7) as u8; // dyn_rng_sgn[i]
            *drc_val |= (bs.read(7) & 0x7F) as u8; // dyn_rng_ctl[i]
        }

        // Set DRC payload type.
        self.channel_data.data_type = AacDrcPayloadType::MpegDrcExtData;

        true
    }

    /// Parses heavy compression value transported in DSEs of DVB streams with MPEG-4 content.
    ///
    /// # Parameters
    ///  - `bs`: Bitstream instance with valid internal data (in)
    ///
    /// # Return:
    ///
    ///  - {true, false}: new DRC data has been found or not.
    fn read_compression(&mut self, bs: &mut Bitstream) -> bool {
        let mut found_drc_data = false;

        // Sanity checks.
        if bs.valid_bits() < 24 {
            return false;
        }

        // Check sync word.
        if bs.read(8) != DVB_ANC_DATA_SYNC_BYTE {
            return false;
        }

        // Evaluate bs_info field.
        // mpeg_audio_type
        if bs.read(2) != 3 {
            // No MPEG-4 audio data.
            return false;
        }

        // dolby_surround_mode
        bs.read(2);
        // presentation_mode
        // The presentation mode can be either 0 (not indicated), 1, 2, or 3 (reserved).
        self.pres_mode = AacDrcPresentationMode::from(bs.read(2) as i8);
        // stereo_downmix_mode
        bs.read_bit();
        if bs.read_bit() != 0 {
            // Reserved, set to 0.
            return false;
        }

        // Evaluate ancillary_data_status.
        if bs.read(3) != 0 {
            // Reserved, set to 0
            return false;
        }

        let dmx_level_present = bs.read_bit(); // downmixing_levels_MPEG4_status
        bs.read_bit(); // ancillary_data_extension_status
        let compression_present = bs.read_bit(); // audio_coding_mode_and_compression status

        bs.read_bit(); // coarse_grain_timecode_status
        bs.read_bit(); // fine_grain_timecode_status

        if dmx_level_present != 0 {
            bs.read(8); // downmixing_levels_MPEG4
        }

        // audio_coding_mode_and_compression_status
        if compression_present != 0 {
            // audio_coding_mode
            if bs.read(7) != 0 {
                // The reserved bits shall be set to "0".
                return false;
            }
            // compression_on
            let compression_on = bs.read_bit() != 0;
            // compression_value
            let compression_value = bs.read(8) as u8;

            if compression_on {
                // A compression value is available so store the data just like MPEG DRC data.
                // one band ...
                self.channel_data.num_bands = 1;
                // ... with one value ...
                self.channel_data.value[0] = compression_value;
                // ... comprising the whole spectrum.
                self.channel_data.band_top[0] = DRC_BLOCK_LEN_DIV_BAND_MULT - 1;
                // not present
                self.pce_instance_tag = 255;
                // not present
                self.prog_ref_level = -1;
                // Set DRC payload type to DVB.
                self.channel_data.data_type = AacDrcPayloadType::DvbDrcAncData;
                found_drc_data = true;
            }
        }

        found_drc_data // return
    }
}

impl From<i8> for AacDrcPresentationMode {
    fn from(value: i8) -> Self {
        match value {
            -1 => AacDrcPresentationMode::NotPresent,
            0 => AacDrcPresentationMode::Enabled,
            1 => AacDrcPresentationMode::Mode1,
            2 => AacDrcPresentationMode::Mode2,
            3 => AacDrcPresentationMode::Reserved,
            _ => panic!("invalid value: {value}"),
        }
    }
}

// Converts AacDrcPresentationMode to i8
impl TryFrom<AacDrcPresentationMode> for i8 {
    type Error = &'static str;

    fn try_from(value: AacDrcPresentationMode) -> Result<Self, Self::Error> {
        match value {
            AacDrcPresentationMode::NotPresent => Ok(-1),
            AacDrcPresentationMode::Enabled => Ok(0),
            AacDrcPresentationMode::Mode1 => Ok(1),
            AacDrcPresentationMode::Mode2 => Ok(2),
            AacDrcPresentationMode::Reserved => Ok(3),
            // _ => Err("invalid AacDrcPresentationMode value."),
        }
    }
}

impl From<i32> for AacDrcParameterHandling {
    fn from(value: i32) -> Self {
        match value {
            -1 => AacDrcParameterHandling::Disabled,
            0 => AacDrcParameterHandling::Enabled,
            1 => AacDrcParameterHandling::Mode1,
            2 => AacDrcParameterHandling::Mode2,
            3 => AacDrcParameterHandling::Aosp,
            _ => panic!("invalid value: {value}"),
        }
    }
}

impl From<AacDrcPresentationMode> for AacDrcParameterHandling {
    fn from(value: AacDrcPresentationMode) -> Self {
        match value {
            AacDrcPresentationMode::NotPresent => AacDrcParameterHandling::Disabled,
            AacDrcPresentationMode::Enabled => AacDrcParameterHandling::Enabled,
            AacDrcPresentationMode::Mode1 => AacDrcParameterHandling::Mode1,
            AacDrcPresentationMode::Mode2 => AacDrcParameterHandling::Mode2,
            AacDrcPresentationMode::Reserved => AacDrcParameterHandling::Aosp,
        }
    }
}
