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
//! Envelope extraction and decoding.
//!
//! The functions provided by this module are mostly called by apply_sbr(). After
//! it is determined that there is valid SBR data, sbr_get_header_data() might be
//! called if the current SBR data contains an SBR_HEADER_ELEMENT as opposed
//! to a SBR_STANDARD_ELEMENT. This function may return various error codes
//! as defined in SBR_HEADER_STATUS. Most importantly, it returns HEADER_RESET
//! when decoder settings need to be recalculated according to the SBR
//! specifications. In that case, apply_sbr() will initiate the required
//! re-configuration.
//!
//! The header data is stored in a SBR_HEADER_DATA structure.
//!
//! The actual SBR data for the current frame is decoded into SBR_FRAME_DATA
//! structures by sbr_get_channel_pair_element() [for stereo streams] and
//! sbr_get_single_channel_element() [for mono streams]. There is no fractional
//! arithmetic involved.
//!
//! Once the information is extracted, the data needs to be further prepared
//! before the actual decoding process. This is done in decode_sbr_data().
//!
//! Description of buffer management in apply_sbr(). documentationOverview
//!
//! About the SBR data format:
//!
//! Each frame includes SBR data (side chain information) and can be either the
//! SBR_HEADER_ELEMENT or the SBR_STANDARD_ELEMENT. Parts of the data can be
//! protected by a CRC checksum.
//!
//! The SBR_HEADER_ELEMENT can be transmitted with every frame; however, it
//! typically is sent every second or so. It contains fundamental information such
//! as SBR sampling frequency and frequency range, as well as control signals that
//! do not require frequent changes. It also includes the SBR_STANDARD_ELEMENT.
//!
//! Depending on the changes between the information in a current
//! SBR_HEADER_ELEMENT and the previous SBR_HEADER_ELEMENT, the SBR decoder might
//! need to be reset and reconfigured (e.g., new tables need to be calculated).
//!
//! The SBR_STANDARD_ELEMENT can be subdivided into "side info" and "raw data",
//! where side info is defined as signals needed to decode the raw data and some
//! decoder tuning signals. Raw data is referred to as PCM and Huffman-coded
//! envelope and noise floor estimates. The side info also includes information
//! about the time-frequency grid for the current frame.

pub(crate) mod constants;

use super::{
    constants::{
        MAX_ENVELOPES, MAX_FREQ_COEFFS, MAX_NOISE_COEFFS, MAX_NOISE_ENVELOPES,
        MAX_NUM_NOISE_VALUES, PVC_NTIMESLOT,
    },
    frame_info::{FrameClass, FrameInfo, FreqRes, PrevFrameInfo},
    header_data::HeaderData,
    huff_dec::{self, AmpRes, CouplingMode, Domain, EnvParam},
    lpp_trans::{self, InvfMode},
    psdec::PsDec,
    pvc,
};
use crate::{common::bitstream::Bitstream, sbr_dec::flags::SbrDecFlags};
use constants::*;
use itertools::izip;
use MAX_NOISE_COEFFS as MAX_INVF_BANDS;

#[derive(Debug)]
pub enum Error {
    Energy,
    NoBitsLeft,
    Frame,
    FrameInfo,
    Pvc,
    Envelope,
    InvfModeError,
}
pub type Result<T, E = Error> = core::result::Result<T, E>;

impl From<lpp_trans::InvfModeError> for Error {
    fn from(_: lpp_trans::InvfModeError) -> Self {
        Error::InvfModeError
    }
}

#[repr(C)]
#[derive(Debug, PartialEq, Clone, Copy, Default)]
pub enum SbrPatchingMode {
    #[default]
    Hbe = 0,
    Lpp = 1,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
/// Structure that holds previous frame data of SBR.
pub struct PrevFrameData {
    /// Frame status of the previous frame.
    pub(crate) frame_error_flag: bool,

    /// Amplitude resolution (0: 1.5dB, 1: 3dB) of the previous frame.
    pub(crate) amp_resolution: AmpRes,

    /// SBR pitch value of the previous frame.
    pub(crate) sbr_pitch_in_bins: u8,

    /// Stereo mode of the previous frame.
    pub(crate) coupling: CouplingMode,

    /// Noise envelope (required for differential-coded values) of the previous frame.
    pub(crate) sbr_noise_floor_level: [f32; MAX_NOISE_COEFFS],

    /// Strength of filtering in the transposer of the previous frame.
    pub(crate) sbr_invf_mode: [lpp_trans::InvfMode; MAX_INVF_BANDS],

    /// Frame info (nEnvelopes and borders[]) of the previous frame.
    pub(super) prev_frame_info: PrevFrameInfo,

    /// PVC ID of the previous frame.
    pub(crate) pvc_id: u8,

    /// Position in time where the last envelope ended of the previous frame.
    pub(crate) stop_pos: u8,

    /// Envelope (required for differential-coded values) of the previous frame.
    pub(crate) sfb_nrg: [f32; MAX_FREQ_COEFFS],
}

impl Default for PrevFrameData {
    fn default() -> Self {
        Self {
            frame_error_flag: Default::default(),
            amp_resolution: Default::default(),
            sbr_pitch_in_bins: Default::default(),
            coupling: Default::default(),
            sbr_noise_floor_level: Default::default(),
            sbr_invf_mode: Default::default(),
            prev_frame_info: Default::default(),
            pvc_id: Default::default(),
            stop_pos: Default::default(),
            sfb_nrg: [0.0; MAX_FREQ_COEFFS],
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
/// Structure that holds SBR frame data.
pub struct FrameData {
    /// Frame data valid flag.
    ///
    /// **CAUTION:** This variable will be overwritten by the flag stored in the element
    /// structure. This is necessary because of the frame delay. It might happen that different
    /// slots use the same header.
    frame_error_flag: bool,

    /// Amplitude resolution of envelope values.
    ///
    /// - `0`: 1.5 dB
    /// - `1`: 3 dB
    amp_resolution: AmpRes,

    /// SBR pitch value in bins.
    sbr_pitch_in_bins: u8,

    /// Stereo coupling mode.
    coupling: CouplingMode,

    /// Noise envelope data.
    sbr_noise_floor_level: [f32; MAX_NUM_NOISE_VALUES],

    /// Strength of filtering in transposer for each inverse filter band.
    sbr_invf_mode: [lpp_trans::InvfMode; MAX_INVF_BANDS],

    /// Time grid information for the current frame.
    pub(super) frame_info: FrameInfo,

    /// One PVC (Parametric Vocal Coding) ID value for each time slot.
    pvc_id: [u8; PVC_NTIMESLOT as usize],

    /// Number of time slots for time domain smoothing.
    ns: u8,

    /// Position of the sinusoidal component.
    sinusoidal_position: u8,

    /// Flag to enable USAC inter-TES (Temporal Envelope Shaping) for all envelopes.
    i_tes_active: bool,

    /// USAC inter-TES temporary shape mode values.
    ///
    /// Corresponds to `bs_inter_temp_shape_mode[ch][env]`.
    inter_temp_shape_mode: [u8; MAX_ENVELOPES],

    /// Total number of scale factors in the frame.
    n_scale_factors: u16,

    /// Transposer patching mode.
    ///
    /// - `SBR_PATCH_MODE_HBE`
    /// - `SBR_PATCH_MODE_LPP`
    sbr_patching_mode: SbrPatchingMode,

    /// Bitfield containing the direction of delta-coding for each envelope.
    ///
    /// - `0`: Frequency domain
    /// - `1`: Time domain
    domain_vec: [Domain; MAX_ENVELOPES],

    /// Bitfield for delta-coding direction of noise envelopes.
    ///
    /// Same as `domain_vec` but specifically for noise envelopes.
    domain_vec_noise: [Domain; MAX_NOISE_ENVELOPES],

    /// Flags for synthetic sine addition, aligned to the Most Significant Bit (MSB).
    add_harmonics: [u32; ADD_HARMONICS_FLAGS_SIZE],

    /// Envelope data.
    i_envelope: [f32; MAX_NUM_ENVELOPE_VALUES],
}
impl Default for FrameData {
    fn default() -> Self {
        Self {
            frame_error_flag: Default::default(),
            amp_resolution: Default::default(),
            sbr_pitch_in_bins: Default::default(),
            coupling: Default::default(),
            sbr_noise_floor_level: Default::default(),
            sbr_invf_mode: Default::default(),
            frame_info: Default::default(),
            pvc_id: Default::default(),
            ns: Default::default(),
            sinusoidal_position: Default::default(),
            i_tes_active: Default::default(),
            inter_temp_shape_mode: Default::default(),
            n_scale_factors: Default::default(),
            sbr_patching_mode: Default::default(),
            domain_vec: Default::default(),
            domain_vec_noise: Default::default(),
            add_harmonics: Default::default(),
            i_envelope: [0.0; MAX_NUM_ENVELOPE_VALUES],
        }
    }
}

impl FrameData {
    pub(super) fn new() -> Self {
        Default::default()
    }

    pub(super) fn inter_tes_active(&self) -> bool {
        self.i_tes_active
    }

    pub(super) fn sinusoidal_position(&self) -> u8 {
        self.sinusoidal_position
    }

    pub(super) fn sbr_noise_floor_level(&self) -> &[f32] {
        &self.sbr_noise_floor_level
    }

    pub(super) fn add_harmonics(&self) -> &[u32; ADD_HARMONICS_FLAGS_SIZE] {
        &self.add_harmonics
    }

    pub(super) fn i_envelope(&self) -> &[f32; MAX_NUM_ENVELOPE_VALUES] {
        &self.i_envelope
    }

    pub(super) fn inter_temp_shape_mode(&self) -> &[u8; MAX_ENVELOPES] {
        &self.inter_temp_shape_mode
    }

    pub(super) fn pvc_id(&self) -> &[u8; PVC_NTIMESLOT as usize] {
        &self.pvc_id
    }

    pub(super) fn frame_error_flag(&self) -> bool {
        self.frame_error_flag
    }

    pub(super) fn set_frame_error_flag(&mut self, frame_error_flag: bool) {
        self.frame_error_flag = frame_error_flag;
    }

    pub(super) fn ns(&self) -> u8 {
        self.ns
    }

    pub(super) fn sbr_patching_mode(&self) -> SbrPatchingMode {
        self.sbr_patching_mode
    }

    pub(super) fn set_sbr_patching_mode(&mut self, sbr_patching_mode: SbrPatchingMode) {
        self.sbr_patching_mode = sbr_patching_mode;
    }

    pub(super) fn sbr_pitch_in_bins(&self) -> u8 {
        self.sbr_pitch_in_bins
    }

    pub(super) fn set_sbr_pitch_in_bins(&mut self, sbr_pitch_in_bins: u8) {
        self.sbr_pitch_in_bins = sbr_pitch_in_bins;
    }

    pub(super) fn sbr_invf_mode(&self) -> &[InvfMode] {
        &self.sbr_invf_mode
    }

    pub(super) fn set_coupling_mode(&mut self, mode: CouplingMode) {
        self.coupling = mode;
    }

    /// Initializes the SBR frame data.
    ///
    /// # Parameters
    ///
    /// - `frame_data_prev`: Handle to frame data of the previous frame.
    /// - `time_slots`: Number of time slots.
    pub fn init_sbr_frame_data(&mut self, frame_data_prev: &mut PrevFrameData, time_slots: u8) {
        self.frame_error_flag = false;
        // Set previous energy and noise levels to 0
        // in case decoding starts in the middle of a bitstream.
        frame_data_prev.sfb_nrg.fill(0.0);
        frame_data_prev.sbr_noise_floor_level.fill(0.0);
        frame_data_prev.sbr_invf_mode.fill(lpp_trans::InvfMode::Off);
        frame_data_prev.stop_pos = time_slots;
        frame_data_prev.coupling = CouplingMode::Off;
        frame_data_prev.amp_resolution = AmpRes::Res1_5;
        frame_data_prev.prev_frame_info = PrevFrameInfo::default();
    }

    /// Updates the previous frame data structure.
    ///
    /// # Parameters
    ///
    /// - `prev_frame_data`: Handle to frame data of the previous frame.
    /// - `header_data`: Handle to SBR header data.
    /// - `apply_processing`: Indicates whether SBR processing is applied.
    pub fn update_sbr_prev_frame_data(
        &self,
        prev_frame_data: &mut PrevFrameData,
        header_data: &HeaderData,
        apply_processing: bool,
    ) {
        if apply_processing {
            for (prev, curr) in izip!(&mut prev_frame_data.sbr_invf_mode, self.sbr_invf_mode).take(
                usize::from(header_data.freq_band_data().num_freq_bands_noise()),
            ) {
                *prev = curr;
            }

            prev_frame_data.coupling = self.coupling;
            prev_frame_data.stop_pos = self.frame_info.borders()[self.frame_info.num_envelopes()];
            prev_frame_data.amp_resolution = self.amp_resolution;
            prev_frame_data.sbr_pitch_in_bins = self.sbr_pitch_in_bins;

            prev_frame_data.prev_frame_info = PrevFrameInfo::new_from_frame_info(&self.frame_info);
        }
        prev_frame_data.frame_error_flag = self.frame_error_flag;
    }

    /// Gets missing harmonics parameters (only used for AAC+SBR).
    ///
    /// # Parameters
    ///
    /// - `bs`: Handle to bitstream structure.
    /// - `header_data`: Handle to SBR header data.
    /// - `flags`: SBR decoder flags.
    fn sbr_get_synthetic_coded_data(
        &mut self,
        bs: &mut Bitstream,
        header_data: &HeaderData,
        flags: SbrDecFlags,
    ) {
        if bs.read_bit() != 0 {
            let mut n_sfb = header_data.freq_band_data().num_freq_bands_sbr(HI);
            for add_harmonic in self.add_harmonics.iter_mut() {
                // Read a maximum of 32 bits and align them to the MSB.
                let read_bits = n_sfb.min(32);
                n_sfb -= read_bits;

                if read_bits > 0 {
                    *add_harmonic = bs.read(read_bits) << (32 - read_bits);
                } else {
                    *add_harmonic = 0;
                }
            }
            if flags.contains(SbrDecFlags::SYNTAX_USAC)
                && header_data.bitstream_info().pvc_mode() != pvc::Mode::None
            {
                let mut bs_sinusoidal_position: u8 = 31;
                // bs_sinusoidal_position_flag
                if bs.read_bit() != 0 {
                    bs_sinusoidal_position = bs.read(5) as u8;
                }
                self.sinusoidal_position = bs_sinusoidal_position;
            }
        } else {
            self.add_harmonics.fill(0);
        }
    }

    /// Reads harmonic SBR data.
    ///
    /// # Parameters
    ///
    /// - `right`: Optional right channel (if channel element == CPE).
    /// - `bs`: Handle to bitstream structure.
    fn read_harmonic_sbr_data(&mut self, right: &mut Option<&mut FrameData>, bs: &mut Bitstream) {
        {
            self.sbr_patching_mode = match bs.read_bit() {
                0 => SbrPatchingMode::Hbe,
                _ => SbrPatchingMode::Lpp,
            };
            if self.sbr_patching_mode == SbrPatchingMode::Hbe {
                // The syntax element "sbrOversamplingFlag" is only evaluated in HQ-HBE
                // (PhaseVocoder). The syntax element is not evaluated by the
                // QmfTransposer. Therefore, it can be ignored.

                // sbrOversamplingFlag
                bs.read_bit();
                // sbrPitchInBinsFlag
                if bs.read_bit() != 0 {
                    self.sbr_pitch_in_bins = bs.read(SI_SBR_PITCH_IN_BINS_BITS) as u8;
                } else {
                    self.sbr_pitch_in_bins = 0;
                }
            } else {
                self.sbr_pitch_in_bins = 0;
            }
        }
        if let Some(right) = right {
            if self.coupling != CouplingMode::Off {
                right.sbr_patching_mode = self.sbr_patching_mode;
                right.sbr_pitch_in_bins = self.sbr_pitch_in_bins;
            } else {
                right.sbr_patching_mode = match bs.read_bit() {
                    0 => SbrPatchingMode::Hbe,
                    _ => SbrPatchingMode::Lpp,
                };
                if right.sbr_patching_mode == SbrPatchingMode::Hbe {
                    // The syntax element "sbrOversamplingFlag" is only evaluated in HQ-HBE
                    // (PhaseVocoder). The syntax element is not evaluated by the
                    // QmfTransposer. Therefore, it can be ignored.

                    // sbrOversamplingFlag
                    bs.read_bit();
                    // sbrPitchInBinsFlag
                    if bs.read_bit() != 0 {
                        right.sbr_pitch_in_bins = bs.read(SI_SBR_PITCH_IN_BINS_BITS) as u8;
                    } else {
                        right.sbr_pitch_in_bins = 0;
                    }
                } else {
                    right.sbr_pitch_in_bins = 0;
                }
            }
        }
    }

    /// Resets the MPEG-4 eSBR frame data.
    ///
    /// # Parameters
    ///
    /// - `right`: Optional right channel (if channel element == CPE).
    fn reset_esbr_frame_data(&mut self, right: &mut Option<&mut FrameData>) {
        self.sbr_patching_mode = SbrPatchingMode::Lpp;
        self.sbr_pitch_in_bins = 0;

        if let Some(right) = right {
            right.sbr_patching_mode = SbrPatchingMode::Lpp;
            right.sbr_pitch_in_bins = 0;
        }
    }

    /// Reads the extension data MPEG-4 SBR Enhancements from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `right`: Optional right channel (if channel element == CPE).
    /// - `bs`: Handle to bitstream structure.
    ///
    /// # Return
    ///
    /// Returns the number of bits read from the bitstream.
    fn read_esbr_frame_data(
        &mut self,
        right: &mut Option<&mut FrameData>,
        bs: &mut Bitstream,
    ) -> i32 {
        let start_bits = bs.valid_bits();

        self.read_harmonic_sbr_data(right, bs);

        let esbr_frame_bits = start_bits - bs.valid_bits();

        if esbr_frame_bits < 5 {
            // esbr_fill_bits
            bs.read((5 - esbr_frame_bits) as u8);
            return 5;
        }

        esbr_frame_bits.try_into().unwrap()
    }

    /// Reads extended data from the bitstream.
    ///
    /// The bitstream format allows up to 4 kinds of extended data elements.
    /// Extended data may contain several elements, each identified by a
    /// 2-bit ID. So far, no extended data elements are defined; hence, the first 2
    /// parameters are unused. The data should be skipped to update
    /// the number of read bits for the consistency check in applySBR().
    ///
    /// # Parameters
    ///
    /// - `right`: Optional right channel (if channel element == CPE).
    /// - `header_data`: Handle to SBR header data.
    /// - `bs`: Handle to bitstream structure.
    /// - `parametric_stereo_dec`: Handle to parametric stereo decoder.
    /// - `flags`: SBR decoder flags.
    pub fn extract_extended_data(
        &mut self,
        right: &mut Option<&mut FrameData>,
        header_data: &mut HeaderData,
        bs: &mut Bitstream,
        mut parametric_stereo_dec: Option<&mut PsDec>,
        flags: SbrDecFlags,
    ) -> Result<()> {
        let mut frame_ok = true;

        header_data.reset_esbr_header_data();
        self.reset_esbr_frame_data(right);

        let extended_data = bs.read(SI_SBR_EXTENDED_DATA_BITS) != 0;

        if extended_data {
            let mut cnt = bs.read(SI_SBR_EXTENSION_SIZE_BITS) as u16;
            if cnt == (1 << SI_SBR_EXTENSION_SIZE_BITS) - 1 {
                cnt += bs.read(SI_SBR_EXTENSION_ESC_COUNT_BITS) as u16;
            }

            let mut n_bits_left: i32 = 8 * i32::from(cnt);

            // Sanity check for cnt.
            if n_bits_left as isize > bs.valid_bits() {
                // Limit n_bits_left.
                n_bits_left = bs.valid_bits() as i32;
                // Set frame error.
                frame_ok = false;
            }

            let mut b_ps_read = false;
            while n_bits_left > 7 {
                let extension_id: u8 = bs.read(SI_SBR_EXTENSION_ID_BITS) as u8;
                n_bits_left -= i32::from(SI_SBR_EXTENSION_ID_BITS);

                match extension_id {
                    EXTENSION_ID_PS_CODING => {
                        // Read PS data from bitstream.
                        if let Some(ps_dec) = &mut parametric_stereo_dec {
                            if !b_ps_read || ps_dec.bs_data_mut().header_valid() {
                                n_bits_left -= i32::try_from(
                                    ps_dec.bs_data_mut().read_ps_data(bs, n_bits_left),
                                )
                                .unwrap();
                                b_ps_read = true;
                            } else {
                                // Number of remaining bytes.
                                let cnt = n_bits_left >> 3;
                                for _ in 0..cnt {
                                    bs.read(8);
                                }
                                n_bits_left -= cnt * 8;
                            }
                        }
                        // Parametric stereo detected; could set channelMode accordingly here.
                        //
                        // "The usage of this parametric stereo extension to HE-AAC is
                        // signalled implicitly in the bitstream. Hence, if an sbr_extension()
                        // with bs_extension_id==EXTENSION_ID_PS is found in the SBR part of
                        // the bitstream, a decoder supporting the combination of SBR and PS
                        // shall operate the PS tool to generate a stereo output signal."
                        // Source: ISO/IEC 14496-3:2001/FDAM 2:2004(E)
                    }
                    EXTENSION_ID_ESBR => {
                        if !flags.contains(SbrDecFlags::SYNTAX_USAC)
                            && flags.contains(SbrDecFlags::USAC_HARMONICSBR)
                        {
                            let e_sbr_header_bits = header_data.read_esbr_header_data(bs);
                            let e_sbr_frame_bits = self.read_esbr_frame_data(right, bs);
                            n_bits_left -= e_sbr_header_bits + e_sbr_frame_bits;
                        }
                    }
                    _ => {
                        // Number of remaining bytes.
                        let cnt = n_bits_left >> 3;
                        for _ in 0..cnt {
                            bs.read(8);
                        }
                        n_bits_left -= cnt * 8;
                    }
                }
            }

            if n_bits_left < 0 {
                return Err(Error::NoBitsLeft);
            } else {
                // Read fill bits for byte alignment.
                bs.read(n_bits_left.try_into().unwrap());
            }
        }

        if frame_ok {
            Ok(())
        } else {
            Err(Error::Frame)
        }
    }

    /// Reads bitstream elements of an SBR channel element.
    ///
    /// # Parameters
    ///
    /// - `right`: Optional right channel (if channel element == CPE).
    /// - `bs`: Handle to bitstream structure.
    /// - `header_data`: Handle to SBR header data.
    /// - `frame_data_left_prev`: Handle to SBR header data of the previous frame.
    /// - `pvc_mode_last`: PVC mode of last frame.
    /// - `flags`: SBR decoder flags.
    /// - `overlap`: Number of QMF overlap slots.
    #[expect(clippy::too_many_arguments)]
    pub fn sbr_get_channel_element(
        &mut self,
        frame_data_left_prev: &mut PrevFrameData,
        right: &mut Option<&mut FrameData>,
        bs: &mut Bitstream,
        header_data: &HeaderData,
        pvc_mode_last: pvc::Mode,
        flags: SbrDecFlags,
        overlap: u8,
    ) -> Result<()> {
        let n_invf_bands = header_data.freq_band_data().num_freq_bands_noise();

        if !flags.contains(SbrDecFlags::SYNTAX_USAC) {
            // Reserved bits.
            if bs.read(SI_SBR_DATA_EXTRA_BITS) != 0 {
                bs.read(SI_SBR_RESERVED_BITS_DATA);
                if flags.contains(SbrDecFlags::SYNTAX_SCAL) || right.is_some() {
                    bs.read(SI_SBR_RESERVED_BITS_DATA);
                }
            }
        }

        if let Some(right) = right {
            // Read coupling flag.
            let bs_coupling = bs.read(SI_SBR_COUPLING_BITS) != 0;
            if bs_coupling {
                self.coupling = CouplingMode::Level;
                right.coupling = CouplingMode::Balance;
            } else {
                self.coupling = CouplingMode::Off;
                right.coupling = CouplingMode::Off;
            }
        } else {
            if flags.contains(SbrDecFlags::SYNTAX_SCAL) {
                // bs_coupling
                bs.read(SI_SBR_COUPLING_BITS);
            }
            self.coupling = CouplingMode::Off;
        }

        if flags.contains(SbrDecFlags::SYNTAX_USAC | SbrDecFlags::USAC_HARMONICSBR) {
            self.read_harmonic_sbr_data(right, bs);
        } else {
            self.sbr_patching_mode = SbrPatchingMode::Lpp;
            self.sbr_pitch_in_bins = 0;
            if let Some(right) = right {
                right.sbr_patching_mode = SbrPatchingMode::Lpp;
                right.sbr_pitch_in_bins = 0;
            }
        }

        // sbr_grid(): Grid control.
        if header_data.bitstream_info().pvc_mode() != pvc::Mode::None {
            // PVC not possible for CPE.
            debug_assert!(right.is_none());
            if !self.frame_info.extract_pvc(
                bs,
                &frame_data_left_prev.prev_frame_info,
                header_data.bitstream_info().pvc_mode(),
                pvc_mode_last,
            ) {
                return Err(Error::FrameInfo);
            }

            if !self.frame_info.check(
                header_data.number_time_slots(),
                overlap,
                header_data.time_step(),
            ) {
                return Err(Error::FrameInfo);
            }
        } else {
            let mut amp_resolution = self.amp_resolution;

            let extract_success = self.frame_info.extract(
                bs,
                &mut amp_resolution,
                header_data.number_time_slots(),
                flags,
            );

            self.amp_resolution = amp_resolution;

            if !extract_success {
                return Err(Error::FrameInfo);
            }

            if !self.frame_info.check(
                header_data.number_time_slots(),
                overlap,
                header_data.time_step(),
            ) {
                return Err(Error::FrameInfo);
            }
        }

        if let Some(right) = right {
            if self.coupling != CouplingMode::Off {
                right.frame_info.clone_from(&self.frame_info);
                right.amp_resolution = self.amp_resolution;
            } else {
                let extract_success = right.frame_info.extract(
                    bs,
                    &mut right.amp_resolution,
                    header_data.number_time_slots(),
                    flags,
                );

                if !extract_success {
                    return Err(Error::FrameInfo);
                }

                if !right.frame_info.check(
                    header_data.number_time_slots(),
                    overlap,
                    header_data.time_step(),
                ) {
                    return Err(Error::FrameInfo);
                }
            }
        }

        // sbr_dtdf(): Fetch domain vectors (time or frequency direction for delta-coding).
        self.sbr_get_direction_control_data(bs, flags, header_data.bitstream_info().pvc_mode());
        if let Some(right) = right {
            right.sbr_get_direction_control_data(bs, flags, pvc::Mode::None);
        }

        // sbr_invf()
        for mode in self.sbr_invf_mode.iter_mut().take(n_invf_bands as usize) {
            *mode = lpp_trans::InvfMode::try_from(bs.read(SI_SBR_INVF_MODE_BITS))?;
        }

        if let Some(right) = right {
            if self.coupling != CouplingMode::Off {
                for (right_mode, self_mode) in izip!(
                    &mut right.sbr_invf_mode[..usize::from(n_invf_bands)],
                    &self.sbr_invf_mode[..usize::from(n_invf_bands)]
                ) {
                    *right_mode = *self_mode;
                }
            } else {
                for mode in right.sbr_invf_mode.iter_mut().take(n_invf_bands as usize) {
                    *mode = lpp_trans::InvfMode::try_from(bs.read(SI_SBR_INVF_MODE_BITS))?;
                }
            }
        }

        if let Some(right) = right {
            if self.coupling != CouplingMode::Off {
                self.sbr_get_envelope(bs, header_data, flags)?;
                self.sbr_get_noise_floor_data(bs, header_data);
                right.sbr_get_envelope(bs, header_data, flags)?;
                right.sbr_get_noise_floor_data(bs, header_data);
            } else {
                // Channel element is CPE and no coupling.
                self.sbr_get_envelope(bs, header_data, flags)?;
                right.sbr_get_envelope(bs, header_data, flags)?;
                self.sbr_get_noise_floor_data(bs, header_data);
                right.sbr_get_noise_floor_data(bs, header_data);
            }
        } else {
            if header_data.bitstream_info().pvc_mode() != pvc::Mode::None {
                self.sbr_get_pvc_envelope(
                    frame_data_left_prev,
                    bs,
                    flags,
                    header_data.bitstream_info().pvc_mode(),
                )?;
            } else {
                self.sbr_get_envelope(bs, header_data, flags)?;
            }
            self.sbr_get_noise_floor_data(bs, header_data);
        }

        self.sbr_get_synthetic_coded_data(bs, header_data, flags);
        if let Some(right) = right {
            right.sbr_get_synthetic_coded_data(bs, header_data, flags);
        }

        Ok(())
    }

    /// Converts raw envelope and noise floor data to energy levels.
    ///
    /// This function is called by sbrDecoder_ParseElement() and provides two
    /// important algorithms:
    ///
    /// First, the function decodes envelopes and noise floor levels as described in
    /// requantizeEnvelopeData() and sbr_envelope_unmapping(). The function also
    /// implements concealment algorithms in case there are errors within the SBR
    /// data. For both operations, fractional arithmetic is used. Therefore, you might
    /// encounter different output values on your target system compared to the
    /// reference implementation.
    pub fn decode_sbr_data(
        &mut self,
        prev_data_left: &mut PrevFrameData,
        data_right: Option<&mut FrameData>,
        prev_data_right: Option<&mut PrevFrameData>,
        header_data: &HeaderData,
    ) {
        let mut temp_sfb_nrg_prev = [0.0; MAX_FREQ_COEFFS];

        // Save previous energy values to be able to reuse them later for concealment.
        temp_sfb_nrg_prev.copy_from_slice(&prev_data_left.sfb_nrg);
        if self.frame_error_flag || header_data.bitstream_info().pvc_mode() == pvc::Mode::None {
            self.decode_envelope(prev_data_left, header_data, prev_data_right.as_deref());
        } else {
            debug_assert!(data_right.is_none());
        }
        self.decode_noise_floor_levels(prev_data_left, header_data);

        if let Some(data_right) = data_right {
            data_right.frame_error_flag |= self.frame_error_flag;
            let prev_data_right = prev_data_right.unwrap();
            data_right.decode_envelope(prev_data_right, header_data, Some(prev_data_left));
            data_right.decode_noise_floor_levels(prev_data_right, header_data);

            if !self.frame_error_flag && data_right.frame_error_flag {
                // If an error occurs in the right channel where the left channel seemed
                // ok, we apply concealment also on the left channel. This ensures that
                // the coupling modes of both channels match and that we have the same
                // number of envelopes in coupling mode. However, as the left channel has
                // already been processed before, the resulting energy levels are not the
                // same as if the left channel had been concealed during the first call of
                // decodeEnvelope().

                // Restore previous energy values for concealment, because the values have
                // been overwritten by the first call of decodeEnvelope().
                prev_data_left.sfb_nrg.copy_from_slice(&temp_sfb_nrg_prev);
                // Do concealment.
                self.decode_envelope(prev_data_left, header_data, Some(prev_data_right));
            }

            if self.coupling != CouplingMode::Off {
                self.sbr_envelope_unmapping(data_right, header_data);
            }

            // Currently, the behavior is to conceal all element channels even if there
            // is an error only in one of both channels. Hence, sync error flags of
            // both channels here.
            self.frame_error_flag |= data_right.frame_error_flag;
            data_right.frame_error_flag = self.frame_error_flag;
        }
    }

    /// Reads direction control data from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Handle to bitstream structure.
    /// - `flags`: SBR decoder flags.
    /// - `bs_pvc_mode`: PVC mode.
    fn sbr_get_direction_control_data(
        &mut self,
        bs: &mut Bitstream,
        flags: SbrDecFlags,
        bs_pvc_mode: pvc::Mode,
    ) {
        let mut start_env = 0;
        let mut indep_flag = false;

        if flags.contains(SbrDecFlags::SYNTAX_USAC) && flags.contains(SbrDecFlags::USAC_INDEP) {
            start_env = 1;
            indep_flag = true;
        }

        if bs_pvc_mode == pvc::Mode::None {
            if indep_flag {
                self.domain_vec[0] = Domain::Freq;
            }
            for domain in self.domain_vec[start_env..self.frame_info.num_envelopes()].iter_mut() {
                *domain = bs.read(SI_SBR_DOMAIN_BITS).try_into().unwrap();
            }
        }

        if indep_flag {
            self.domain_vec_noise[0] = Domain::Freq;
        }
        for domain_noise in
            self.domain_vec_noise[start_env..self.frame_info.num_noise_envelopes()].iter_mut()
        {
            *domain_noise = bs.read(SI_SBR_DOMAIN_BITS).try_into().unwrap();
        }
    }

    /// Reads PVC envelope data.
    ///
    /// # Parameters
    ///
    /// - `prev_frame_data`: Handle to previous SBR header data.
    /// - `bs`: Handle to bitstream structure.
    /// - `flags`: SBR decoder flags.
    /// - `pvc_mode`: PVC mode.
    fn sbr_get_pvc_envelope(
        &mut self,
        prev_frame_data: &mut PrevFrameData,
        bs: &mut Bitstream,
        flags: SbrDecFlags,
        pvc_mode: pvc::Mode,
    ) -> Result<()> {
        let pvc_id = &mut self.pvc_id;
        let indep_flag = flags.contains(SbrDecFlags::USAC_INDEP);

        let mut div_mode: u32 = bs.read(PVC_DIVMODE_BITS);
        let ns_mode: usize = bs.read_bit() as usize;
        debug_assert!((pvc_mode == pvc::Mode::Mode1) || (pvc_mode == pvc::Mode::Mode2));
        self.ns = MAP_NS_MODE_2_NS[pvc_mode as usize - 1][ns_mode];

        if div_mode <= 3 {
            // Special treatment for first time slot k = 0.
            let reuse_pcv_id = if indep_flag {
                false
            } else {
                bs.read_bit() != 0
            };

            if reuse_pcv_id {
                pvc_id[0] = prev_frame_data.pvc_id;
            } else {
                pvc_id[0] = bs.read(PVC_PVCID_BITS) as u8;
            }

            // Other time slots k > 0.
            let mut k = 1;
            let mut sum_length = 0;
            for _ in 0..div_mode {
                let mut num_bits = 4;

                if sum_length >= 13 {
                    num_bits = 1;
                } else if sum_length >= 11 {
                    num_bits = 2;
                } else if sum_length >= 7 {
                    num_bits = 3;
                }

                let length: u8 = bs.read(num_bits) as u8;
                sum_length += length + 1;

                if sum_length >= PVC_NTIMESLOT {
                    // Parse error.
                    return Err(Error::Pvc);
                }

                let tmp = pvc_id[k - 1];
                pvc_id[k..k + length as usize].fill(tmp);
                k += length as usize;

                pvc_id[k] = bs.read(PVC_PVCID_BITS) as u8;

                k += 1;
            }
            let tmp = pvc_id[k - 1];
            pvc_id[k..].fill(tmp);
        } else {
            div_mode -= 4;
            let num_grid_info = 2 << div_mode;
            let fixed_length = (8 >> div_mode) as usize;
            debug_assert!(num_grid_info * fixed_length == PVC_NTIMESLOT as usize);

            let mut grid_info = if indep_flag { true } else { bs.read_bit() != 0 };

            if grid_info {
                pvc_id[0] = bs.read(PVC_PVCID_BITS) as u8;
            } else {
                pvc_id[0] = prev_frame_data.pvc_id;
            }

            let mut k = 1;
            let mut j = fixed_length - 1;
            let tmp = pvc_id[k - 1];
            pvc_id[k..k + j].fill(tmp);
            k += j;

            for _ in 0..(num_grid_info - 1) {
                j = fixed_length;
                grid_info = bs.read_bit() != 0;
                if grid_info {
                    pvc_id[k] = bs.read(PVC_PVCID_BITS) as u8;
                    k += 1;
                    j -= 1;
                }
                let tmp = pvc_id[k - 1];
                pvc_id[k..k + j].fill(tmp);
                k += j;
            }
        }

        prev_frame_data.pvc_id = pvc_id[usize::from(PVC_NTIMESLOT) - 1];

        // Usage of PVC excludes inter-TES tool.
        self.i_tes_active = false;

        Ok(())
    }

    /// Reads envelope data from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Handle to bitstream structure.
    /// - `header_data`: Handle to SBR header data.
    /// - `flags`: SBR decoder flags.
    fn sbr_get_envelope(
        &mut self,
        bs: &mut Bitstream,
        header_data: &HeaderData,
        flags: SbrDecFlags,
    ) -> Result<()> {
        let mut no_band = [0; MAX_ENVELOPES];
        let n_envelopes = self.frame_info.num_envelopes();

        // Calculate the number of values for each envelope and overall.
        self.n_scale_factors = 0;
        for (no_band, freq_res) in izip!(
            no_band.iter_mut().take(n_envelopes),
            self.frame_info.freq_res().iter().take(n_envelopes)
        ) {
            *no_band = header_data
                .freq_band_data()
                .num_freq_bands_sbr(*freq_res as usize);
            self.n_scale_factors += u16::from(*no_band);
        }

        if self.n_scale_factors > MAX_NUM_ENVELOPE_VALUES as u16 {
            return Err(Error::Envelope);
        }

        // Disable inter-TES by default.
        self.i_tes_active = false;

        let mut amp_res = header_data.bitstream_info().amp_resolution();
        if self.frame_info.frame_class() == FrameClass::FixFix && n_envelopes == 1 {
            if flags.contains(SbrDecFlags::ELD_GRID) {
                amp_res = self.amp_resolution;
            } else {
                amp_res = AmpRes::Res1_5;
            }
        }
        self.amp_resolution = amp_res;

        let start_bits;
        let start_bits_balance;
        // Set the number of bits for the first value depending on amplitude resolution.
        if amp_res == AmpRes::Res3_0 {
            start_bits = SI_SBR_START_ENV_BITS_AMP_RES_3_0;
            start_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0;
        } else {
            start_bits = SI_SBR_START_ENV_BITS_AMP_RES_1_5;
            start_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5;
        }

        // Now read raw envelope data.
        let mut offset = 0;
        for (i, (band, domain)) in izip!(no_band.iter(), self.domain_vec.iter())
            .take(n_envelopes)
            .enumerate()
        {
            if *domain == Domain::Freq {
                if self.coupling == CouplingMode::Balance {
                    self.i_envelope[offset] =
                        (bs.read(start_bits_balance) << ENV_DATA_TABLE_COMP_FACTOR_BAL) as f32;
                } else {
                    self.i_envelope[offset] = bs.read(start_bits) as f32;
                }
            }

            let idx_start = offset + 1 - (*domain as usize);
            let idx_stop = idx_start + usize::from(*band) + *domain as usize - 1;
            huff_dec::decode_huffman_float(
                bs,
                &mut self.i_envelope[idx_start..idx_stop],
                EnvParam::Env,
                *domain,
                amp_res,
                self.coupling,
                if self.coupling == CouplingMode::Balance {
                    ENV_DATA_TABLE_COMP_FACTOR_BAL
                } else {
                    ENV_DATA_TABLE_COMP_FACTOR
                },
            );

            if flags.contains(SbrDecFlags::SYNTAX_USAC) && flags.contains(SbrDecFlags::USAC_ITES) {
                let bs_temp_shape = bs.read_bit() != 0;
                debug_assert!(i < 8);
                self.i_tes_active |= bs_temp_shape;
                if bs_temp_shape {
                    self.inter_temp_shape_mode[i] = bs.read(2) as u8;
                } else {
                    self.inter_temp_shape_mode[i] = 0;
                }
            }
            offset += usize::from(*band);
        }

        Ok(())
    }

    /// Reads noise floor data from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Handle to bitstream structure.
    /// - `header_data`: Handle to SBR header data.
    fn sbr_get_noise_floor_data(&mut self, bs: &mut Bitstream, header_data: &HeaderData) {
        let env_data_table_comp_factor = if self.coupling == CouplingMode::Balance {
            1
        } else {
            0
        };
        let no_noise_bands = header_data.freq_band_data().num_freq_bands_noise();

        // Read raw noise-envelope data.
        for (&domain, noise_floor) in izip!(
            self.domain_vec_noise.iter(),
            self.sbr_noise_floor_level
                .chunks_mut(usize::from(no_noise_bands))
        )
        .take(self.frame_info.num_noise_envelopes())
        {
            let offset = if domain == Domain::Freq { 1 } else { 0 };

            if domain == Domain::Freq {
                noise_floor[0] = match self.coupling {
                    CouplingMode::Balance => {
                        (bs.read(SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0)
                            << env_data_table_comp_factor) as f32
                    }
                    _ => bs.read(SI_SBR_START_NOISE_BITS_AMP_RES_3_0) as f32,
                };
            }

            let target_slice = &mut noise_floor[offset..];

            huff_dec::decode_huffman_float(
                bs,
                target_slice,
                EnvParam::Noise,
                domain,
                AmpRes::Res1_5,
                self.coupling,
                env_data_table_comp_factor,
            );
        }
    }

    /// Converts each envelope value from logarithmic to linear domain.
    ///
    /// Energy levels are transmitted in powers of 2, i.e., only the exponent
    /// is extracted from the bitstream.
    /// Therefore, normally only integer exponents can occur. However, during
    /// fading (in case of a corrupt bitstream), a fractional part can also
    /// occur.
    ///
    /// This function calculates a mantissa corresponding to the fractional
    /// part of the exponent for each reference energy. The array `i_envelope`
    /// is converted in place to save memory. Input and output data must
    /// be interpreted differently. The data is then used in
    /// `sbr_dec::env_calc::env_calc.apply()`.
    fn requantize_envelope_data(&mut self) {
        let k = 1.0f32 / (2.0f32 - (self.amp_resolution as u8) as f32);
        let i_envelope = &mut self.i_envelope;

        for e in i_envelope.iter_mut().take(self.n_scale_factors as usize) {
            let exponent = *e;
            *e = 2.0f32.powf(exponent * k + 6.0f32);
        }
    }

    /// Converts from coupled channels to independent L/R data.
    ///
    /// # Parameters
    ///
    /// - `header_data`: Static control data.
    /// - `data_right`: Pointer to the right channel.
    fn sbr_envelope_unmapping(&mut self, data_right: &mut FrameData, header_data: &HeaderData) {
        let i_envelope_l = &mut self.i_envelope;
        let i_envelope_r = &mut data_right.i_envelope;

        // 1. Unmap (already dequantized) coupled envelope energies.
        let unmapping_scale = 3.814697265625e-6;

        for (r, l) in izip!(i_envelope_r.iter_mut(), i_envelope_l.iter_mut())
            .take(self.n_scale_factors as usize)
        {
            let temp_r = *r * unmapping_scale;
            let temp_l = 2.0 * (*l);
            let new_r = temp_l / (temp_r + 1.0);
            let new_l = temp_r * new_r;
            *r = new_r;
            *l = new_l;
        }

        let sbr_noise_floor_level_l = &mut self.sbr_noise_floor_level;
        let sbr_noise_floor_level_r = &mut data_right.sbr_noise_floor_level;

        // 2. Dequantize and unmap coupled noise floor levels.
        for (l, r) in izip!(
            sbr_noise_floor_level_l.iter_mut(),
            sbr_noise_floor_level_r.iter_mut()
        )
        .take(
            header_data.freq_band_data().num_freq_bands_noise() as usize
                * self.frame_info.num_noise_envelopes(),
        ) {
            let temp_l = NOISE_FLOOR_OFFSET - *l;
            let temp_r = *r - SBR_ENERGY_PAN_OFFSET;
            let pow2temp_r = 2.0_f32.powf(temp_r);
            let new_r = 2.0_f32.powf(temp_l + 1.0) / (pow2temp_r + 1.0);
            let new_l = new_r * pow2temp_r;
            *r = new_r;
            *l = new_l;
        }
    }

    /// Simple alternative to the real SBR concealment.
    ///
    /// If the real frame_info is not available due to a frame loss, a replacement will
    /// be constructed with 1 envelope spanning the whole frame (FIX-FIX).
    /// The delta-coded energies are set to negative values, resulting in a fade-down.
    /// In case of coupling, the balance channel will move towards the center.
    ///
    /// # Parameters
    ///
    /// - `header_data`: Static control data.
    /// - `sbr_data`: Pointer to current data.
    /// - `prev_data`: Pointer to data of last frame.
    fn lean_sbr_concealment(&mut self, prev_data: &PrevFrameData, header_data: &HeaderData) {
        // Use some settings of the previous frame.
        self.amp_resolution = prev_data.amp_resolution;
        self.coupling = prev_data.coupling;

        self.sbr_invf_mode.copy_from_slice(&prev_data.sbr_invf_mode);

        // Generate concealing control data.
        self.frame_info.conceal(
            prev_data
                .stop_pos
                .saturating_sub(header_data.number_time_slots()),
            header_data.number_time_slots(),
        );

        self.n_scale_factors = header_data.freq_band_data().num_freq_bands_sbr(HI).into();

        // Generate fake envelope data.
        self.domain_vec[0] = Domain::Time;

        // Targeted level for sfb_nrg_prev during fade-down.
        let mut target;
        // Speed of fade.
        let mut step;

        if self.coupling == CouplingMode::Balance {
            target = SBR_ENERGY_PAN_OFFSET;
            step = DECAY_COUPLING;
        } else {
            target = 0.0;
            step = DECAY;
        }
        if header_data.bitstream_info().amp_resolution() == AmpRes::Res1_5 {
            target *= 2.0;
            step *= 2.0;
        }

        for (envelope, sfb_nrg) in izip!(self.i_envelope.iter_mut(), prev_data.sfb_nrg.iter())
            .take(self.n_scale_factors as usize)
        {
            *envelope = if *sfb_nrg > target { -step } else { step };
        }

        self.domain_vec_noise[0] = Domain::Time;
        self.sbr_noise_floor_level.fill(0.0);
        self.add_harmonics.fill(0);
    }

    /// Builds reference energies and noise levels from bitstream elements.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to data of the last frame.
    /// - `header_data`: Static control data.
    /// - `other_channel`: Other channel's last frame data.
    fn decode_envelope(
        &mut self,
        prev_data: &mut PrevFrameData,
        header_data: &HeaderData,
        other_channel: Option<&PrevFrameData>,
    ) {
        let mut is_frame_error = self.frame_error_flag;
        if !is_frame_error {
            // To avoid distortions after bad frames, set the error flag if delta coding
            // in time occurs. However, SBR can take a little longer to come up again.
            if prev_data.frame_error_flag {
                if self.domain_vec[0] != Domain::Freq {
                    is_frame_error = true;
                }
            } else {
                // Check that the previous stop position and the current start position
                // match. (Could be done in checkFrameInfo(), but the previous frame data
                // is not available there.)
                if self.frame_info.borders()[0]
                    != prev_data.stop_pos - header_data.number_time_slots()
                {
                    // Both the previous as well as the current frame are flagged to be ok,
                    // but they do not match!
                    if self.domain_vec[0] == Domain::Time {
                        // Prefer concealment over delta-time coding between the mismatching
                        // frames.
                        is_frame_error = true;
                    } else {
                        // Close the gap in time by triggering timeCompensateFirstEnvelope().
                        is_frame_error = true;
                    }
                }
            }
        }
        if is_frame_error {
            self.lean_sbr_concealment(prev_data, header_data);

            // Decode the envelope data to linear PCM.
            self.delta_to_linear_pcm_envelope_decoding(prev_data, header_data);
        } else {
            // Do a temporary dummy decoding and check that the envelope values are
            // within limits.
            let mut temp_sfb_nrg_prev = [0.0; MAX_FREQ_COEFFS];
            if prev_data.frame_error_flag {
                self.time_compensate_first_envelope(prev_data, header_data);
                if self.coupling != prev_data.coupling {
                    // Coupling mode has changed during concealment.
                    // The stored energy levels need to be converted.
                    if prev_data.coupling == CouplingMode::Balance {
                        if let Some(oc) = other_channel {
                            prev_data.sfb_nrg
                                [0..header_data.freq_band_data().num_freq_bands_sbr(HI) as usize]
                                .copy_from_slice(
                                    &oc.sfb_nrg[0..header_data
                                        .freq_band_data()
                                        .num_freq_bands_sbr(HI)
                                        as usize],
                                );
                        } else {
                            prev_data.sfb_nrg
                                [0..header_data.freq_band_data().num_freq_bands_sbr(HI) as usize]
                                .fill(SBR_ENERGY_PAN_OFFSET);
                        };
                    } else if self.coupling == CouplingMode::Balance {
                        prev_data.sfb_nrg
                            [0..header_data.freq_band_data().num_freq_bands_sbr(HI) as usize]
                            .fill(SBR_ENERGY_PAN_OFFSET);
                    } else if self.coupling == CouplingMode::Level {
                        if let Some(oc) = other_channel {
                            for (dst, src) in izip!(prev_data.sfb_nrg.iter_mut(), oc.sfb_nrg.iter())
                                .take(header_data.freq_band_data().num_freq_bands_sbr(HI) as usize)
                            {
                                *dst = (*dst + *src) * 0.5;
                            }
                        }
                    }
                }
            }
            temp_sfb_nrg_prev.copy_from_slice(&prev_data.sfb_nrg);

            self.delta_to_linear_pcm_envelope_decoding(prev_data, header_data);

            is_frame_error = self.check_envelope_data(prev_data, header_data).is_err();

            if is_frame_error {
                self.frame_error_flag = true;
                prev_data.sfb_nrg.copy_from_slice(&temp_sfb_nrg_prev);
                self.decode_envelope(prev_data, header_data, other_channel);
                return;
            }
        }

        self.requantize_envelope_data();

        self.frame_error_flag = is_frame_error;
    }

    /// Verifies that envelope energies are within the allowed range.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to data of the last frame.
    /// - `header_data`: Static control data.
    fn check_envelope_data(
        &self,
        prev_data: &mut PrevFrameData,
        header_data: &HeaderData,
    ) -> Result<()> {
        let mut error_flag = false;
        let sfb_nrg_prev = &mut prev_data.sfb_nrg;
        let i_envelope = &self.i_envelope;
        let sbr_max_energy = if self.amp_resolution == AmpRes::Res3_0 {
            SBR_MAX_ENERGY
        } else {
            SBR_MAX_ENERGY * 2.0
        };

        // Range check for current energies.
        for &e in i_envelope.iter().take(self.n_scale_factors as usize) {
            if e > sbr_max_energy || e < 0.0 {
                error_flag = true;
                break;
            }
        }

        // Range check for previous energies.
        for p in sfb_nrg_prev
            .iter_mut()
            .take(header_data.freq_band_data().num_freq_bands_sbr(HI) as usize)
        {
            *p = p.clamp(0.0, sbr_max_energy);
        }
        if error_flag {
            Err(Error::Energy)
        } else {
            Ok(())
        }
    }

    /// Verifies that the noise levels are within the allowed range.
    ///
    /// The function is equivalent to check_envelope_data().
    /// When the noise levels are being decoded, it is already too late for
    /// concealment. Therefore, the noise levels are simply limited here.
    ///
    /// # Parameters
    ///
    /// - `header_data`: Static control data.
    fn limit_noise_levels(&mut self, header_data: &HeaderData) {
        let n_nfb = header_data.freq_band_data().num_freq_bands_noise();
        let sbr_noise_floor_level = &mut self.sbr_noise_floor_level;

        // Range check for current noise levels.
        for n in sbr_noise_floor_level
            .iter_mut()
            .take(self.frame_info.num_noise_envelopes() * n_nfb as usize)
        {
            *n = n.clamp(SBR_NOISE_FLOOR_LOWER_LIMIT, SBR_NOISE_FLOOR_UPPER_LIMIT);
        }
    }

    /// Compensates for the wrong timing that might occur after a frame error.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to data of the last frame.
    /// - `header_data`: Static control data.
    fn time_compensate_first_envelope(
        &mut self,
        prev_data: &PrevFrameData,
        header_data: &HeaderData,
    ) {
        let ref_len: u8;
        let new_len: u8;

        {
            let frame_info = &mut self.frame_info;
            let borders = frame_info.borders();
            let mut estimated_start_pos =
                (prev_data.stop_pos as i32 - header_data.number_time_slots() as i32).max(0) as u8;

            // Original length of first envelope according to bitstream.
            ref_len = borders[1] - borders[0];

            if borders[1] <= estimated_start_pos {
                // An envelope length of <= 0 would not work, so we don't use it.
                // May occur if the previous frame was flagged bad due to a mismatch
                // of the old and new frame infos.
                new_len = ref_len;
                estimated_start_pos = borders[0];
            } else {
                // Corrected length of first envelope (concealing can make the first
                // envelope longer).
                new_len = borders[1] - estimated_start_pos;
            }

            // Set envelope start position.
            frame_info.set_start_pos(estimated_start_pos);
            frame_info.set_start_pos_noise(estimated_start_pos);
        }

        if self.coupling != CouplingMode::Balance && ref_len != new_len {
            let n_scalefactors = if self.frame_info.freq_res()[0] != FreqRes::Coarse {
                header_data.freq_band_data().num_freq_bands_sbr(HI)
            } else {
                header_data.freq_band_data().num_freq_bands_sbr(LO)
            };
            let shift = if self.amp_resolution == AmpRes::Res1_5 {
                2.0
            } else {
                1.0
            };
            let delta_exp = (ref_len as f32).log2() - (new_len as f32).log2() * 2.0f32.powf(shift);
            for x in self.i_envelope.iter_mut().take(n_scalefactors as usize) {
                *x += delta_exp;
            }
        }
    }

    /// Builds new reference energies from old ones and delta-coded data.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to data of the last frame.
    /// - `header_data`: Static control data.
    fn delta_to_linear_pcm_envelope_decoding(
        &mut self,
        prev_data: &mut PrevFrameData,
        header_data: &HeaderData,
    ) {
        let mut ptr_nrg: &mut [f32] = &mut self.i_envelope;
        let sfb_nrg_prev = &mut prev_data.sfb_nrg;
        let offset: i32 = 2 * header_data.freq_band_data().num_freq_bands_sbr(LO) as i32
            - header_data.freq_band_data().num_freq_bands_sbr(HI) as i32;

        let freq_res = self.frame_info.freq_res();
        for (&domain, &freq_res) in
            izip!(self.domain_vec.iter(), freq_res.iter()).take(self.frame_info.num_envelopes())
        {
            let no_of_bands = header_data
                .freq_band_data()
                .num_freq_bands_sbr(freq_res as usize);
            debug_assert!(no_of_bands < 64);

            if domain == Domain::Freq {
                Self::map_low_res_energy_val(sfb_nrg_prev, ptr_nrg[0], offset, 0, freq_res);
                for band in 1..no_of_bands {
                    let band_us = usize::from(band);
                    ptr_nrg[band_us] += ptr_nrg[band_us - 1];
                    Self::map_low_res_energy_val(
                        sfb_nrg_prev,
                        ptr_nrg[band_us],
                        offset,
                        i32::from(band),
                        freq_res,
                    );
                }
            } else {
                for (band, nrg) in ptr_nrg.iter_mut().enumerate().take(no_of_bands as usize) {
                    *nrg += sfb_nrg_prev[usize::try_from(Self::index_low_2_high(
                        offset,
                        band as i32,
                        freq_res,
                    ))
                    .unwrap()];
                    Self::map_low_res_energy_val(sfb_nrg_prev, *nrg, offset, band as i32, freq_res);
                }
            }
            ptr_nrg = &mut ptr_nrg[usize::from(no_of_bands)..];
        }
    }

    /// Converts table index.
    ///
    /// # Parameters
    ///
    /// - `offset`: Mapping factor.
    /// - `index`: Index to scalefactor band.
    /// - `res`: Frequency resolution.
    fn index_low_2_high(offset: i32, index: i32, res: FreqRes) -> i32 {
        if res == FreqRes::Coarse {
            if offset >= 0 {
                if index < offset {
                    index
                } else {
                    2 * index - offset
                }
            } else if index < -offset {
                2 * index + index
            } else {
                2 * index - offset
            }
        } else {
            index
        }
    }

    /// Updates previous envelope value for delta-coding.
    ///
    /// The current envelope values need to be stored for delta-coding
    /// in the next frame. The stored envelope is always represented with
    /// the high frequency resolution. If the current envelope uses the
    /// low frequency resolution, the energy value will be mapped to the
    /// corresponding high-res bands.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to previous data vector.
    /// - `curr_val`: Current energy value.
    /// - `offset`: Mapping factor.
    /// - `index`: Index to scalefactor band.
    /// - `res`: Frequency resolution.
    fn map_low_res_energy_val(
        prev_data: &mut [f32],
        curr_val: f32,
        offset: i32,
        index: i32,
        res: FreqRes,
    ) {
        if res == FreqRes::Coarse {
            if offset >= 0 {
                if index < offset {
                    prev_data[index as usize] = curr_val;
                } else {
                    prev_data[(2 * index - offset) as usize] = curr_val;
                    prev_data[(2 * index + 1 - offset) as usize] = curr_val;
                }
            } else if index < -offset {
                prev_data[(3 * index) as usize] = curr_val;
                prev_data[(3 * index + 1) as usize] = curr_val;
                prev_data[(3 * index + 2) as usize] = curr_val;
            } else {
                prev_data[(2 * index - offset) as usize] = curr_val;
                prev_data[(2 * index + 1 - offset) as usize] = curr_val;
            }
        } else {
            prev_data[index as usize] = curr_val;
        }
    }

    /// Builds new noise levels from old ones and delta-coded data.
    ///
    /// # Parameters
    ///
    /// - `prev_data`: Pointer to data of the last frame.
    /// - `header_data`: Static control data.
    fn decode_noise_floor_levels(
        &mut self,
        prev_data: &mut PrevFrameData,
        header_data: &HeaderData,
    ) {
        let n_nfb = usize::from(header_data.freq_band_data().num_freq_bands_noise());
        let n_noise_floor_envelopes = self.frame_info.num_noise_envelopes();
        let sbr_noise_floor_level = &mut self.sbr_noise_floor_level;
        // Decode first noise envelope.

        if self.domain_vec_noise[0] == Domain::Freq {
            let mut noise_level = sbr_noise_floor_level[0];
            sbr_noise_floor_level
                .iter_mut()
                .take(n_nfb)
                .skip(1)
                .for_each(|nfl| {
                    noise_level += *nfl;
                    *nfl = noise_level;
                });
        } else {
            for (nfl, prev_nfl) in izip!(
                sbr_noise_floor_level.iter_mut(),
                prev_data.sbr_noise_floor_level.iter()
            )
            .take(n_nfb)
            {
                *nfl += *prev_nfl;
            }
        }

        // If present, decode the second noise envelope.
        // Note: n_noise_floor_envelopes can only be 1 or 2.

        if n_noise_floor_envelopes > 1 {
            if self.domain_vec_noise[1] == Domain::Freq {
                let mut noise_level = sbr_noise_floor_level[n_nfb];
                for nfl in sbr_noise_floor_level
                    .iter_mut()
                    .take(2 * n_nfb)
                    .skip(n_nfb + 1)
                {
                    noise_level += *nfl;
                    *nfl = noise_level;
                }
            } else {
                debug_assert!(
                    sbr_noise_floor_level.len() >= 2 * n_nfb,
                    "Insufficient elements in sbr_noise_floor_level"
                );

                let (first, second) = sbr_noise_floor_level.split_at_mut(n_nfb);
                for (source, target) in first.iter().zip(second.iter_mut()) {
                    *target += *source;
                }
            }
        }

        self.limit_noise_levels(header_data);

        // Update sbrNoiseFloorLevel with the last noise envelope.
        prev_data.sbr_noise_floor_level[..n_nfb].copy_from_slice(
            &self.sbr_noise_floor_level[(n_nfb * (n_noise_floor_envelopes - 1))
                ..(n_nfb * (n_noise_floor_envelopes - 1) + n_nfb)],
        );

        // Requantize the noise floor levels in COUPLING_OFF mode.
        if self.coupling == CouplingMode::Off {
            for nfl in self
                .sbr_noise_floor_level
                .iter_mut()
                .take(n_noise_floor_envelopes * n_nfb)
            {
                *nfl = f32::powf(2.0, NOISE_FLOOR_OFFSET - *nfl);
            }
        }
    }
}
