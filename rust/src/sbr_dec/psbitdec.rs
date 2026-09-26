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
//! Parametric Stereo Bitstream Decoder

use crate::common::bitstream::*;
use crate::sbr_dec::common::SbrError;
use crate::sbr_dec::huff_dec::*;

// +1 needed for VAR_BORDER
pub const MAX_NO_PS_ENV: usize = 4 + 1;

const NO_HI_RES_BINS: usize = 34;
pub const NO_MID_RES_BINS: usize = 20;
const NO_LOW_RES_BINS: usize = 10;

// 1 .. + 7
pub const NO_IID_STEPS: u8 = 7;
// 1 .. +15
pub const NO_IID_STEPS_FINE: u8 = 15;
// 0 .. + 7
pub const NO_ICC_STEPS: u8 = 8;

const EXTENSION_SIZE_BITS: u8 = 4;
const EXTENSION_ESC_COUNT_BITS: u8 = 8;

const FREQ_RES_HI: u8 = 2;

const MAX_IID_MODE_LOW: u8 = 2;
const MAX_ICC_MODE_LOW: u8 = 2;

const MAX_IID_MODE_HI: u8 = 5;
const MAX_ICC_MODE_HI: u8 = 5;

const IID_MODE_DIFF: u8 = MAX_IID_MODE_HI - MAX_IID_MODE_LOW;
const ICC_MODE_DIFF: u8 = MAX_ICC_MODE_HI - MAX_ICC_MODE_LOW;

const NO_SUB_SAMPLES_ERR: usize = 0;
const NO_SUB_SAMPLES_960: usize = 30;
const NO_SUB_SAMPLES_1024: usize = 32;

const MAX_NUM_COL: usize = 32;

/// Frequency resolution for IID/ICC
const NO_BINS: [usize; 3] = [NO_LOW_RES_BINS, NO_MID_RES_BINS, NO_HI_RES_BINS];

/// FIX_BORDER can have 0, 1, 2, 4 envelopes
const FIX_NUM_ENV_DECODE: [usize; 4] = [0, 1, 2, 4];

#[derive(Debug, Clone, Copy, PartialEq, Default)]
#[repr(C)]
enum PayloadType {
    #[default]
    None,
    Mpeg,
}

/// Represents the frequency resolution, 0=low, 1=mid or 2=high.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum FrequencyResolution {
    Low,
    Mid,
    High,
}

impl TryFrom<u8> for FrequencyResolution {
    type Error = &'static str;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(FrequencyResolution::Low),
            1 => Ok(FrequencyResolution::Mid),
            2 => Ok(FrequencyResolution::High),
            _ => Err("Invalid value for FrequencyResolution"),
        }
    }
}

impl From<FrequencyResolution> for usize {
    fn from(freq_res: FrequencyResolution) -> Self {
        match freq_res {
            FrequencyResolution::Low => 0,
            FrequencyResolution::Mid => 1,
            FrequencyResolution::High => 2,
        }
    }
}

/// Represents the previous frame's MPEG Program Stream Bitstream Data.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
struct MpegPsBsDataPrev {
    /// The IID quantization of the previous frame.
    is_fine_iid_quant: bool,

    /// Frequency resolution for IID of the previous frame.
    freq_res_iid: FrequencyResolution,

    /// Frequency resolution for ICC of the previous frame.
    freq_res_icc: FrequencyResolution,

    /// The IID index for the previous frame.
    iid_index: [i8; NO_HI_RES_BINS],

    /// The ICC index for the previous frame.
    icc_index: [i8; NO_HI_RES_BINS],
}

impl Default for MpegPsBsDataPrev {
    fn default() -> Self {
        MpegPsBsDataPrev {
            is_fine_iid_quant: false,
            freq_res_iid: FrequencyResolution::Low,
            freq_res_icc: FrequencyResolution::Low,
            iid_index: [0_i8; NO_HI_RES_BINS],
            icc_index: [0_i8; NO_HI_RES_BINS],
        }
    }
}

/// Represents MPEG Program Stream Bitstream Data.
#[derive(Debug, Clone, Copy)]
#[repr(C)]
struct MpegPsBsData {
    /// Set if new header is available from bitstream.
    header_valid: bool,

    /// One bit denoting the presence of IID parameters.
    enable_iid: bool,

    /// One bit denoting the presence of ICC parameters.
    enable_icc: bool,

    /// The PS extension layer is enabled using the enable_ext bit. If it is
    /// set to %1 the IPD and OPD parameters are sent. If it is disabled, i.e.
    /// %0, the extension layer is skipped.
    enable_ext: bool,

    /// Use fine IID quantisation.
    is_fine_iid_quant: bool,

    /// The frame_class bit determines whether the parameter positions of the
    /// current frame are uniformly spaced across the frame or they are
    /// defined using the positions described by border_position.
    frame_class: bool,

    /// The configuration of IID parameters (number of bands and quantisation grid,
    /// iid_quant) is determined by iid_mode.
    mode_iid: u8,

    /// The configuration of Inter-channel Coherence parameters (number of bands
    /// and quantisation grid) is determined by icc_mode.
    mode_icc: u8,

    /// 0=low, 1=mid or 2=high frequency resolution for IID.
    freq_res_iid: FrequencyResolution,

    /// 0=low, 1=mid or 2=high frequency resolution for ICC.
    freq_res_icc: FrequencyResolution,

    /// The number of envelopes per frame.
    num_env: usize,

    /// In case of variable parameter spacing the
    /// parameter positions are determined by
    /// border_position.
    env_start_stop: [usize; MAX_NO_PS_ENV + 1],

    /// Deltacoding Domain::Time/freq flag for IID,
    /// 0 => freq.
    iid_deltacoding_flag: [bool; MAX_NO_PS_ENV],

    /// Deltacoding Domain::Time/freq flag for ICC,
    /// 0 => freq.
    icc_deltacoding_flag: [bool; MAX_NO_PS_ENV],

    /// The IID index for all envelopes and all
    /// IID bins.
    iid_index: [[i8; NO_HI_RES_BINS]; MAX_NO_PS_ENV],

    /// The ICC index for all envelopes and all
    /// ICC bins.
    icc_index: [[i8; NO_HI_RES_BINS]; MAX_NO_PS_ENV],
}

/// Represents the bitstream data for the PS decoder.
#[derive(Debug, Clone, Copy, Default)]
#[repr(C)]
pub struct PsDecBsData {
    /// Index of the last read slot.
    last_slot: u8,

    /// Index of the current read slot for additional delay.
    read_slot: u8,

    /// Index of the current slot for processing (needed for additional delay).
    process_slot: usize,

    /// Number of subsamples.
    num_sub_samples: usize,

    /// Set if PS has been processed in the last frame.
    ps_decoded_prev: bool,

    /// Set if new data is available from the bitstream.
    data_avail: [PayloadType; 2],

    /// Struct containing MPEG specific PS data from the previous frame.
    mpeg_prev: MpegPsBsDataPrev,

    /// Struct containing all MPEG specific PS data from the bitstream.
    mpeg: [MpegPsBsData; 2],
}

impl Default for MpegPsBsData {
    fn default() -> Self {
        MpegPsBsData {
            header_valid: false,
            enable_iid: false,
            enable_icc: false,
            enable_ext: false,
            is_fine_iid_quant: false,
            frame_class: false,
            mode_iid: 0,
            mode_icc: 0,
            freq_res_iid: FrequencyResolution::Low,
            freq_res_icc: FrequencyResolution::Low,
            num_env: 0,
            env_start_stop: [0; MAX_NO_PS_ENV + 1],
            iid_deltacoding_flag: [false; MAX_NO_PS_ENV],
            icc_deltacoding_flag: [false; MAX_NO_PS_ENV],
            iid_index: [[0; NO_HI_RES_BINS]; MAX_NO_PS_ENV],
            icc_index: [[0; NO_HI_RES_BINS]; MAX_NO_PS_ENV],
        }
    }
}

impl PsDecBsData {
    /// Decodes delta coded IID and ICC indices.
    ///
    /// # Parameters
    ///
    /// - `frame_error`: Flag indicating that the frame had errors.
    /// - `process_slot`: Slot which will be processed.
    pub fn decode_ps_data(&mut self, frame_error: bool, process_slot: usize) -> bool {
        let header_valid = self.mpeg[process_slot].header_valid;
        let data_avail = self.data_avail[process_slot] == PayloadType::Mpeg;

        self.process_slot = process_slot;
        if !self.ps_decoded_prev && frame_error
            || !frame_error && !data_avail
            || !self.ps_decoded_prev && !header_valid
        {
            // Don't apply PS processing. Declare current PS header and bitstream data invalid.
            self.mpeg[self.process_slot].header_valid = false;
            self.data_avail[self.process_slot] = PayloadType::None;

            return false;
        }

        if frame_error || !header_valid {
            // No new PS data available (e.g. frame loss)
            // => keep latest data constant (i.e. FIX with num_env = 0)
            self.mpeg[self.process_slot].num_env = 0;
        }

        let data = &mut self.mpeg[self.process_slot];
        let data_prev = &mut self.mpeg_prev;
        let num_sub_samples = self.num_sub_samples;

        // Decode bitstream payload or prepare parameter for concealment.
        for env in 0..data.num_env {
            let num_iid_steps = if data.is_fine_iid_quant {
                NO_IID_STEPS_FINE
            } else {
                NO_IID_STEPS
            };

            let prev_index_iid = if env == 0 {
                data_prev.iid_index
            } else {
                data.iid_index[env - 1]
            };

            delta_decode_array(
                data.enable_iid,
                &mut data.iid_index[env],
                &prev_index_iid,
                data.iid_deltacoding_flag[env],
                NO_BINS[usize::from(data.freq_res_iid)].try_into().unwrap(),
                if data.freq_res_iid != FrequencyResolution::Low {
                    1
                } else {
                    2
                },
                -(i8::try_from(num_iid_steps).unwrap()),
                i8::try_from(num_iid_steps).unwrap(),
            );

            let prev_index_icc = if env == 0 {
                data_prev.icc_index
            } else {
                data.icc_index[env - 1]
            };

            delta_decode_array(
                data.enable_icc,
                &mut data.icc_index[env],
                &prev_index_icc,
                data.icc_deltacoding_flag[env],
                NO_BINS[usize::from(data.freq_res_icc)].try_into().unwrap(),
                if data.freq_res_icc != FrequencyResolution::Low {
                    1
                } else {
                    2
                },
                0,
                (NO_ICC_STEPS - 1) as i8,
            );
        }

        // Handling of FIX num_env = 0
        if data.num_env == 0 {
            // Set num_env = 1, keep last parameters or force 0 if not enabled
            data.num_env = 1;

            if data.enable_iid {
                data.is_fine_iid_quant = data_prev.is_fine_iid_quant;
                data.freq_res_iid = data_prev.freq_res_iid;
                // NO_HI_RES_BINS index values will be copied.
                data.iid_index[0].copy_from_slice(&data_prev.iid_index);
            } else {
                // NO_HI_RES_BINS index values will be cleared.
                data.iid_index[0].fill(0);
            }

            if data.enable_icc {
                data.freq_res_icc = data_prev.freq_res_icc;
                // NO_HI_RES_BINS index values will be copied.
                data.icc_index[0].copy_from_slice(&data_prev.icc_index);
            } else {
                // NO_HI_RES_BINS index values will be cleared.
                data.icc_index[0].fill(0);
            }
        }

        // Update previous frame IID quantization
        data_prev.is_fine_iid_quant = data.is_fine_iid_quant;

        // Update previous frequency resolution for IID
        data_prev.freq_res_iid = data.freq_res_iid;

        // Update previous frequency resolution for ICC
        data_prev.freq_res_icc = data.freq_res_icc;

        // Update previous frame index buffers
        data_prev
            .iid_index
            .copy_from_slice(&data.iid_index[data.num_env - 1]);
        data_prev
            .icc_index
            .copy_from_slice(&data.icc_index[data.num_env - 1]);

        // Handling of env borders for FIX & VAR
        if !data.frame_class {
            // FIX_BORDERS num_env = 0, 1, 2, 4
            // 960  (30 slots) env borders:  0, 7, 15, 22, 30
            // 1024 (32 slots) env borders:  0, 8, 16, 24, 32
            data.env_start_stop[0] = 0;
            for env in 1..data.num_env {
                data.env_start_stop[env] = (env * num_sub_samples) / data.num_env;
            }
            data.env_start_stop[data.num_env] = num_sub_samples;
        } else {
            // VAR_BORDERS num_env = 1, 2, 3, 4
            data.env_start_stop[0] = 0;

            // Handle case env_start_stop[num_env]<num_sub_samples for VAR_BORDERS
            // by duplicating last PS parameters and incrementing num_env
            if data.env_start_stop[data.num_env] < num_sub_samples {
                data.iid_index[data.num_env] = data.iid_index[data.num_env - 1];
                data.icc_index[data.num_env] = data.icc_index[data.num_env - 1];
                data.num_env += 1;
                data.env_start_stop[data.num_env] = num_sub_samples;
            }

            // Enforce strictly monotonic increasing borders
            for env in 1..data.num_env {
                let mut thr = num_sub_samples - (data.num_env - env);

                if data.env_start_stop[env] > thr {
                    data.env_start_stop[env] = thr;
                } else {
                    thr = data.env_start_stop[env - 1] + 1;
                    if data.env_start_stop[env] < thr {
                        data.env_start_stop[env] = thr;
                    }
                }
            }
        }

        // PS data from bitstream (if avail) was decoded now
        self.data_avail[self.process_slot] = PayloadType::None;

        true
    }

    /// Reads parametric stereo data from bitstream
    pub(super) fn read_ps_data(&mut self, bs: &mut Bitstream, num_bits_left: i32) -> u32 {
        if self.read_slot != self.last_slot {
            // Copy last header data
            self.mpeg[usize::from(self.read_slot)] = self.mpeg[usize::from(self.last_slot)];
        }

        let start_bits = bs.valid_bits();
        // assuming 0 is false
        let enable_header = bs.read_bit() != 0;
        let data = &mut self.mpeg[usize::from(self.read_slot)];

        // Read header
        if enable_header {
            data.header_valid = true;

            data.enable_iid = bs.read_bit() != 0;
            if data.enable_iid {
                data.mode_iid = u8::try_from(bs.read(3)).unwrap();
            }

            data.enable_icc = bs.read_bit() != 0;
            if data.enable_icc {
                data.mode_icc = u8::try_from(bs.read(3)).unwrap();
            }

            data.enable_ext = bs.read_bit() != 0;
        }

        data.frame_class = bs.read_bit() != 0;
        if !data.frame_class {
            // FIX_BORDERS num_env = 0, 1, 2, 4
            data.num_env = FIX_NUM_ENV_DECODE[usize::try_from(bs.read(2)).unwrap()];
        } else {
            // VAR_BORDERS num_env = 1, 2, 3, 4
            data.num_env = usize::try_from(bs.read(2)).unwrap() + 1;
            for env in 1..=data.num_env {
                data.env_start_stop[env] = usize::try_from(bs.read(5)).unwrap() + 1;
            }
        }

        // Verify that IID & ICC modes (quant grid, freq res) are supported
        if (data.mode_iid > MAX_IID_MODE_HI) || (data.mode_icc > MAX_ICC_MODE_HI) {
            // no useful PS data could be read from bitstream
            self.data_avail[usize::from(self.read_slot)] = PayloadType::None;

            // discard all remaining bits
            let mut bits_left = num_bits_left - i32::try_from(start_bits).unwrap()
                + i32::try_from(bs.valid_bits()).unwrap();
            while bits_left > 0 {
                let i = if bits_left > 8 { 8 } else { bits_left };
                bs.read(u8::try_from(i).unwrap());
                bits_left -= i;
            }
            return u32::try_from(start_bits - bs.valid_bits()).unwrap();
        }

        if data.mode_iid > MAX_IID_MODE_LOW {
            data.freq_res_iid =
                FrequencyResolution::try_from(data.mode_iid - IID_MODE_DIFF).unwrap();
            data.is_fine_iid_quant = true;
        } else {
            data.freq_res_iid = FrequencyResolution::try_from(data.mode_iid).unwrap();
            data.is_fine_iid_quant = false;
        }

        if data.mode_icc > MAX_ICC_MODE_LOW {
            data.freq_res_icc =
                FrequencyResolution::try_from(data.mode_icc - ICC_MODE_DIFF).unwrap();
        } else {
            data.freq_res_icc = FrequencyResolution::try_from(data.mode_icc).unwrap();
        }

        // Extract IID data
        if data.enable_iid {
            for env in 0..data.num_env {
                data.iid_deltacoding_flag[env] = bs.read_bit() != 0;

                decode_huffman(
                    bs,
                    &mut data.iid_index[env][..NO_BINS[usize::from(data.freq_res_iid)]],
                    ParametricStereoParam::Iid,
                    if data.iid_deltacoding_flag[env] {
                        Domain::Time
                    } else {
                        Domain::Freq
                    },
                    if data.is_fine_iid_quant {
                        QuantRes::Fine
                    } else {
                        QuantRes::Coarse
                    },
                );
            }
        }

        // Extract ICC data
        if data.enable_icc {
            for env in 0..data.num_env {
                data.icc_deltacoding_flag[env] = bs.read_bit() != 0;

                decode_huffman(
                    bs,
                    &mut data.icc_index[env][..NO_BINS[usize::from(data.freq_res_icc)]],
                    ParametricStereoParam::Icc,
                    if data.icc_deltacoding_flag[env] {
                        Domain::Time
                    } else {
                        Domain::Freq
                    },
                    // unused for ICC
                    QuantRes::Coarse,
                );
            }
        }

        if data.enable_ext {
            // Decoders that support only the baseline version of the PS tool are allowed
            // to ignore the IPD/OPD data, but according header data has to be parsed.
            // ISO/IEC 14496-3 Subpart 8 Annex 4

            let mut cnt = i32::try_from(bs.read(EXTENSION_SIZE_BITS)).unwrap();
            if cnt == (1 << EXTENSION_SIZE_BITS) - 1 {
                cnt += i32::try_from(bs.read(EXTENSION_ESC_COUNT_BITS)).unwrap();
            }
            while cnt > 0 {
                bs.read(8);
                cnt -= 1;
            }
        }

        // new PS data was read from bitstream
        self.data_avail[usize::from(self.read_slot)] = PayloadType::Mpeg;

        u32::try_from(start_bits - bs.valid_bits()).unwrap()
    }

    /// Copying and if necessary mapping of ICC/IID parameters to 20 stereo bands.
    pub(super) fn fill_ps_data(
        &mut self,
        iid_index_mapped: &mut [[i8; NO_MID_RES_BINS]; MAX_NO_PS_ENV],
        icc_index_mapped: &mut [[i8; NO_MID_RES_BINS]; MAX_NO_PS_ENV],
    ) {
        let data = &mut self.mpeg[self.process_slot];

        // MPEG baseline PS:
        // Baseline version of PS always uses the hybrid filter structure with 20
        // stereo bands. If ICC/IID parameters for 34 stereo bands are decoded they
        // have to be mapped to 20 stereo bands.

        if data.freq_res_iid == FrequencyResolution::try_from(FREQ_RES_HI).unwrap() {
            data.iid_index
                .iter()
                .take(data.num_env)
                .enumerate()
                .for_each(|(env, iid)| {
                    map34_index_to_20(&mut iid_index_mapped[env], iid);
                });
        } else {
            data.iid_index
                .iter()
                .take(data.num_env)
                .enumerate()
                .for_each(|(env, iid)| {
                    iid_index_mapped[env][..NO_MID_RES_BINS]
                        .copy_from_slice(&iid[..NO_MID_RES_BINS]);
                });
        }

        if data.freq_res_icc == FrequencyResolution::try_from(FREQ_RES_HI).unwrap() {
            data.icc_index
                .iter()
                .take(data.num_env)
                .enumerate()
                .for_each(|(env, icc)| {
                    map34_index_to_20(&mut icc_index_mapped[env], icc);
                });
        } else {
            data.icc_index
                .iter()
                .take(data.num_env)
                .enumerate()
                .for_each(|(env, icc)| {
                    icc_index_mapped[env][..NO_MID_RES_BINS]
                        .copy_from_slice(&icc[..NO_MID_RES_BINS]);
                });
        }
    }

    /// Inits bitstream specific ps parameters
    pub(super) fn init_ps_data(&mut self, aac_samples_per_frame: u16) -> SbrError {
        self.ps_decoded_prev = false;

        self.num_sub_samples = match aac_samples_per_frame {
            1024 => NO_SUB_SAMPLES_1024,
            960 => NO_SUB_SAMPLES_960,
            _ => NO_SUB_SAMPLES_ERR,
        };

        if self.num_sub_samples > MAX_NUM_COL || self.num_sub_samples == NO_SUB_SAMPLES_ERR {
            return SbrError::CreateError;
        }

        self.data_avail = [PayloadType::None, PayloadType::None];
        self.mpeg = [MpegPsBsData::default(), MpegPsBsData::default()];

        SbrError::Ok
    }

    // Setters
    pub fn set_bs_read_slot(&mut self, slot: u8) {
        self.read_slot = slot;
    }

    pub fn set_bs_last_slot(&mut self, slot: u8) {
        self.last_slot = slot;
    }

    pub fn set_decoded_prev(&mut self, val: bool) {
        self.ps_decoded_prev = val;
    }

    // Getters
    pub fn bs_read_slot(&mut self) -> u8 {
        self.read_slot
    }

    pub(super) fn env_start_stop(&mut self, env: usize) -> u8 {
        self.mpeg[self.process_slot].env_start_stop[env] as u8
    }

    pub(super) fn header_valid(&mut self) -> bool {
        self.mpeg[usize::from(self.read_slot)].header_valid
    }

    pub(super) fn fine_iid_quant(&mut self) -> bool {
        self.mpeg[self.process_slot].is_fine_iid_quant
    }
}

/// Decodes delta values in-place and updates data buffers according to quantization classes.
///
/// When delta coded in frequency the first element is deltacode from zero. Index buffer is decoded
/// from delta values to actual values.
///
/// # Parameters
///
/// - `enable`: Indicates, whether delta decoding is enabled
/// - `index`: ICC/IID parameters
/// - `prev_index`: ICC/IID parameters of previous frame
/// - `deltacoding_domain`: Indicates, whether delta coded in freq (false) or time (true)
/// - `num_elements`: As conveyed in bitstream. Output array size: num_elements * stride
/// - `stride`: 1 = dflt, 2 = half freq resolution
/// - `min_index`: Minimum index
/// - `max_index`: Maximum index
#[expect(clippy::too_many_arguments)]
fn delta_decode_array(
    enable: bool,
    index: &mut [i8],
    prev_index: &[i8],
    deltacoding_domain: bool,
    num_elements: u8,
    stride: u8,
    min_index: i8,
    max_index: i8,
) {
    // Delta decode
    if enable {
        if !deltacoding_domain {
            // Delta coded in freq
            index[0] = index[0].clamp(min_index, max_index);
            let mut prev = index[0];
            for i in index[1..usize::from(num_elements)].iter_mut() {
                *i = (prev + *i).clamp(min_index, max_index);
                prev = *i;
            }
        } else {
            // Delta coded in Domain::Time
            index
                .iter_mut()
                .zip(prev_index.iter().step_by(stride as usize))
                .take(usize::from(num_elements))
                .for_each(|(elem, prev)| {
                    *elem = (*prev + *elem).clamp(min_index, max_index);
                });
        }
    } else {
        // No data is sent, set index to zero
        index[..usize::from(num_elements)].fill(0);
    }

    if stride == 2 {
        // Adjust index for stride == 2
        (1..usize::from(num_elements * stride)).rev().for_each(|i| {
            index[i] = index[i >> 1];
        });
    }
}

/// Mapping of ICC/IID parameters to 20 stereo bands.
///
/// # Parameters
///
/// - `index_mapped`: Mapped ICC/IID parameters.
/// - `index`: Decoded ICC/IID parameters.
fn map34_index_to_20(index_mapped: &mut [i8], index: &[i8]) {
    index_mapped[0] = (2 * index[0] + index[1]) / 3;
    index_mapped[1] = (index[1] + 2 * index[2]) / 3;
    index_mapped[2] = (2 * index[3] + index[4]) / 3;
    index_mapped[3] = (index[4] + 2 * index[5]) / 3;
    index_mapped[4] = (index[6] + index[7]) / 2;
    index_mapped[5] = (index[8] + index[9]) / 2;
    index_mapped[6] = index[10];
    index_mapped[7] = index[11];
    index_mapped[8] = (index[12] + index[13]) / 2;
    index_mapped[9] = (index[14] + index[15]) / 2;
    index_mapped[10] = index[16];
    index_mapped[11] = index[17];
    index_mapped[12] = index[18];
    index_mapped[13] = index[19];
    index_mapped[14] = (index[20] + index[21]) / 2;
    index_mapped[15] = (index[22] + index[23]) / 2;
    index_mapped[16] = (index[24] + index[25]) / 2;
    index_mapped[17] = (index[26] + index[27]) / 2;
    index_mapped[18] = (index[28] + index[29] + index[30] + index[31]) / 4;
    index_mapped[19] = (index[32] + index[33]) / 2;
}
