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
//! SBR Frame Information

// Modules
mod tables;

// Imports
use super::{
    constants::{
        ABS_BITS, CLA_BITS, ENV_BITS, MAX_ENVELOPES, MAX_ENVELOPES_USAC, MAX_ENV_COLS,
        MAX_NOISE_ENVELOPES, MAX_OV_COLS, NUM_BITS, PVC_NOISEPOSITION_BITS, PVC_NTIMESLOT,
        PVC_VAR_LEN_HF_BITS, REL_BITS, RES_BITS,
    },
    huff_dec::AmpRes,
    pvc::Mode,
};
use crate::{common::bitstream::Bitstream, sbr_dec::flags::SbrDecFlags};

// Predefined envelope position for the FixFix case.
const FRAME_INFO_1_15: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 1,
    borders: [0, 15, 0, 0, 0, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 1,
    border_noise: [0, 15, 0],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_2_15: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 2,
    borders: [0, 8, 15, 0, 0, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 15],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_4_15: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 4,
    borders: [0, 4, 8, 12, 15, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 15],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_8_15: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 8,
    borders: [0, 2, 4, 6, 8, 10, 12, 14, 15],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 15],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_1_16: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 1,
    borders: [0, 16, 0, 0, 0, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 1,
    border_noise: [0, 16, 0],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_2_16: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 2,
    borders: [0, 8, 16, 0, 0, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 16],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_4_16: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 4,
    borders: [0, 4, 8, 12, 16, 0, 0, 0, 0],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
        FreqRes::Coarse,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 16],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

const FRAME_INFO_8_16: FrameInfo = FrameInfo {
    frame_class: FrameClass::FixFix,
    num_envelopes: 8,
    borders: [0, 2, 4, 6, 8, 10, 12, 14, 16],
    freq_res: [
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
        FreqRes::Fine,
    ],
    tran_env: -1,
    num_noise_envelopes: 2,
    border_noise: [0, 8, 16],
    pvc_borders: [0, 0, 0],
    noise_position: 0,
    var_length: 0,
};

#[repr(u8)]
#[derive(Debug, Copy, Clone, PartialEq, Default)]
pub(super) enum FreqRes {
    #[default]
    Coarse = 0,
    Fine = 1,
}

#[repr(C)]
#[derive(Debug, PartialEq, Copy, Clone, Default)]
pub(super) enum FrameClass {
    #[default]
    FixFix = 0,
    FixVar = 1,
    VarFix = 2,
    VarVar = 3,
}

impl From<u32> for FrameClass {
    fn from(value: u32) -> Self {
        match value {
            0 => FrameClass::FixFix,
            1 => FrameClass::FixVar,
            2 => FrameClass::VarFix,
            3 => FrameClass::VarVar,
            _ => panic!("invalid value: {value}"),
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Default, Debug)]
/// Previous frame information structure.
pub(super) struct PrevFrameInfo {
    /// Number of envelopes.
    num_envelopes: usize,
    /// Envelope border.
    border: u8,
}

impl PrevFrameInfo {
    /// Returns new instance of `PrevFrameInfo`.
    pub(super) fn _new() -> Self {
        PrevFrameInfo::default()
    }

    /// Copies current frame information to previous frame information.
    ///
    /// # Parameters
    ///
    /// - `prev_frame_info`: Reference to previous frame information.
    pub(super) fn new_from_frame_info(frame_info: &FrameInfo) -> Self {
        let border = if frame_info.num_envelopes > 0 {
            frame_info.borders[frame_info.num_envelopes]
        } else {
            0
        };

        PrevFrameInfo {
            num_envelopes: frame_info.num_envelopes,
            border,
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone, Default)]
/// Frame information structure.
pub(super) struct FrameInfo {
    /// Selects grid type.
    frame_class: FrameClass,
    /// Number of envelopes.
    num_envelopes: usize,
    /// Envelope borders (in SBR-timeslots).
    borders: [u8; MAX_ENVELOPES + 1],
    /// Frequency resolution for each envelope (0=low, 1=high).
    freq_res: [FreqRes; MAX_ENVELOPES],
    /// Transient envelope, -1 if none.
    tran_env: i8,
    /// Number of noise envelopes.
    num_noise_envelopes: usize,
    /// Borders of noise envelopes.
    border_noise: [u8; MAX_NOISE_ENVELOPES + 1],
    /// PVC borders.
    pvc_borders: [u8; MAX_NOISE_ENVELOPES + 1],
    /// Position of the noise in the signal.
    noise_position: u8,
    /// Frame variable length.
    var_length: u8,
}

impl FrameInfo {
    /// Extracts frame information from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `amp_resolution`: Reference to amplification resolution value.
    /// - `number_time_slots`: Number of time slots.
    /// - `flags`: SBR decoder flags.
    ///
    /// # Return
    ///
    /// - `bool`: `true` if extraction is successfull, `false` otherwise.
    pub(super) fn extract(
        &mut self,
        bs: &mut Bitstream,
        amp_resolution: &mut AmpRes,
        number_time_slots: u8,
        flags: SbrDecFlags,
    ) -> bool {
        let frame_class;
        let mut temp = 0;
        let mut num_env = 0;
        let mut b: usize = 0;
        let mut n = 0;
        let mut border: i16;
        let mut i;
        let pointer_bits;
        let p;

        if flags.contains(SbrDecFlags::ELD_GRID) {
            // CODEC_AACLD (LD+SBR) only uses the normal FixFix grid for non-transient
            // frames and the low delay grid for transient frames.
            frame_class = FrameClass::from(bs.read_bit());

            // If frameClass == 1, extract low delay SBR grid, otherwise extract normal
            // SBR-Grid for FixFix.
            if frame_class == FrameClass::FixVar {
                self.frame_class = frame_class;
                return self.extract_low_delay_grid(bs, number_time_slots);
            }
        } else {
            frame_class = FrameClass::from(bs.read(CLA_BITS));
        }

        match frame_class {
            FrameClass::FixFix => {
                temp = bs.read(ENV_BITS);
                num_env = 1 << temp;

                if flags.contains(SbrDecFlags::ELD_GRID) && (num_env == 1) {
                    // New ELD Syntax 07-11-09.
                    *amp_resolution = AmpRes::from(bs.read_bit());
                }

                if flags.contains(SbrDecFlags::SYNTAX_USAC) {
                    if num_env > MAX_ENVELOPES_USAC {
                        return false;
                    }
                } else {
                    b = num_env + 1;
                }

                match num_env {
                    1 => {
                        match number_time_slots {
                            15 => *self = FRAME_INFO_1_15,
                            16 => *self = FRAME_INFO_1_16,
                            _ => (),
                        };
                    }
                    2 => {
                        match number_time_slots {
                            15 => *self = FRAME_INFO_2_15,
                            16 => *self = FRAME_INFO_2_16,
                            _ => (),
                        };
                    }
                    4 => {
                        match number_time_slots {
                            15 => *self = FRAME_INFO_4_15,
                            16 => *self = FRAME_INFO_4_16,
                            _ => (),
                        };
                    }
                    8 => {
                        if MAX_ENVELOPES >= 8 {
                            match number_time_slots {
                                15 => *self = FRAME_INFO_8_15,
                                16 => *self = FRAME_INFO_8_16,
                                _ => (),
                            };
                        } else {
                            return false;
                        }
                    }
                    _ => (),
                };

                // Apply correct frequency resolution (High is default).
                if bs.read(RES_BITS) == 0 {
                    // Clear memory of freq_res array.
                    self.freq_res = [FreqRes::Coarse; MAX_ENVELOPES];
                }
            }
            FrameClass::FixVar | FrameClass::VarFix => {
                temp = bs.read(ABS_BITS);
                n = bs.read(NUM_BITS);

                // Envelopes.
                num_env = (n + 1) as usize;
                // Borders.
                b = num_env + 1;
            }
            _ => (),
        };

        match frame_class {
            FrameClass::FixVar => {
                // Decode borders.
                // First border.
                self.borders[0] = 0;
                border = (temp as u8 + number_time_slots) as i16;

                if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                    return false;
                }

                // Frame info index for last border.
                i = b - 1;
                // Last border.
                self.borders[i] = border as u8;

                for _k in 0..n {
                    i -= 1;
                    temp = bs.read(REL_BITS);
                    border -= 2 * temp as i16 + 2;

                    if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                        return false;
                    }

                    self.borders[i] = border as u8;
                }

                // Decode pointer.
                pointer_bits = u32::BITS - (n + 1).leading_zeros();
                p = bs.read(pointer_bits as u8);

                if p > (n + 1) {
                    return false;
                }

                self.tran_env = if p != 0 { (n + 2 - p) as i8 } else { -1 };

                // Decode freq_res.
                let mut k = n as i32;
                while k >= 0 {
                    self.freq_res[k as usize] = if bs.read(RES_BITS) == 0 {
                        FreqRes::Coarse
                    } else {
                        FreqRes::Fine
                    };
                    k -= 1;
                }

                // Calculate noise floor middle border.
                if p == 0 || p == 1 {
                    self.border_noise[1] = self.borders[n as usize];
                } else {
                    self.border_noise[1] = self.borders[self.tran_env as usize];
                }
            }
            FrameClass::VarFix => {
                // Decode borders.
                border = temp as i16;

                if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                    return false;
                }

                // First border.
                self.borders[0] = border as u8;

                for k in 1..=n {
                    temp = bs.read(REL_BITS);
                    border += (2 * temp + 2) as i16;

                    if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                        return false;
                    }

                    self.borders[k as usize] = border as u8;
                }
                // Last border.
                self.borders[(n + 1) as usize] = number_time_slots;

                // Decode pointer.
                pointer_bits = u32::BITS - (n + 1).leading_zeros();
                p = bs.read(pointer_bits as u8);

                if p > (n + 1) {
                    return false;
                }

                if p == 0 || p == 1 {
                    self.tran_env = -1;
                } else {
                    self.tran_env = p as i8 - 1;
                }

                // Decode freq res.
                for k in 0..=n {
                    self.freq_res[k as usize] = if bs.read(RES_BITS) == 0 {
                        FreqRes::Coarse
                    } else {
                        FreqRes::Fine
                    };
                }

                // Calculate noise floor middle border
                match p {
                    0 => self.border_noise[1] = self.borders[1],
                    1 => self.border_noise[1] = self.borders[n as usize],
                    _ => self.border_noise[1] = self.borders[self.tran_env as usize],
                };
            }
            FrameClass::VarVar => {
                // v_ctrlSignal = [frameClass, aL, aR, nL, nR, v_rL, v_rR, p, v_fLR].
                let a_l = bs.read(ABS_BITS);
                let a_r = bs.read(ABS_BITS) + number_time_slots as u32;
                let n_l = bs.read(NUM_BITS);
                let n_r = bs.read(NUM_BITS);

                // Calculate help variables.
                // General
                // Envelopes.
                num_env = (n_l + n_r + 1) as usize;

                if num_env > MAX_ENVELOPES {
                    return false;
                }

                // Borders.
                b = num_env + 1;

                // Decode envelopes.
                // L-borders.
                border = a_l as i16;

                if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                    return false;
                }

                self.borders[0] = border as u8;

                for k in 1..=n_l {
                    temp = bs.read(REL_BITS);
                    border += (2 * temp + 2) as i16;

                    if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                        return false;
                    }

                    self.borders[k as usize] = border as u8;
                }

                // R-borders.
                border = a_r as i16;

                if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                    return false;
                }

                i = num_env;

                self.borders[i] = border as u8;

                for _k in 0..n_r {
                    i -= 1;
                    temp = bs.read(REL_BITS);
                    border -= (2 * temp + 2) as i16;

                    if !(0..=MAX_ENV_COLS as i16).contains(&border) {
                        return false;
                    }

                    self.borders[i] = border as u8;
                }

                // Decode pointer.
                pointer_bits = u32::BITS - (n_l + n_r + 1).leading_zeros();
                p = bs.read(pointer_bits as u8);

                if p > (n_l + n_r + 1) {
                    return false;
                }

                self.tran_env = if p != 0 { b as i8 - p as i8 } else { -1 };

                // Decode freq res.
                for k in 0..num_env {
                    self.freq_res[k] = if bs.read(RES_BITS) == 0 {
                        FreqRes::Coarse
                    } else {
                        FreqRes::Fine
                    };
                }

                // Decode noise floors
                self.border_noise[0] = a_l as u8;

                if num_env == 1 {
                    // 1 noise floor envelope.
                    self.border_noise[1] = a_r as u8;
                } else {
                    // 2 noise floor envelopes.
                    if p == 0 || p == 1 {
                        self.border_noise[1] = self.borders[num_env - 1];
                    } else {
                        self.border_noise[1] = self.borders[self.tran_env as usize];
                    }
                    self.border_noise[2] = a_r as u8;
                }
            }
            _ => (),
        };

        // Store number of envelopes, noise floor envelopes and frame class.
        self.num_envelopes = num_env;

        if num_env == 1 {
            self.num_noise_envelopes = 1;
        } else {
            self.num_noise_envelopes = 2;
        }

        self.frame_class = frame_class;

        if self.frame_class == FrameClass::VarFix || self.frame_class == FrameClass::FixVar {
            // Calculate noise floor first and last borders.
            self.border_noise[0] = self.borders[0];
            self.border_noise[self.num_noise_envelopes] = self.borders[num_env];
        }

        true
    }

    /// Extracts low delay SBR control data from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `number_time_slots`: Number of time slots.
    ///
    /// # Return
    ///
    /// - `bool`: `true` if extraction is successfull, `false` otherwise.
    fn extract_low_delay_grid(&mut self, bs: &mut Bitstream, number_time_slots: u8) -> bool {
        // FixFix only framing case.
        self.frame_class = FrameClass::FixFix;

        // Get the transient position from the bitstream.
        let trans_pos = match number_time_slots {
            // 3bit transient position (temp={0;..;7}).
            8 => bs.read(3) as usize,
            // 4bit transient position (temp={0;..;15}).
            16 | 15 => bs.read(4) as usize,
            _ => return false,
        };

        // For "case 15" only.
        if trans_pos >= usize::from(number_time_slots) {
            return false;
        }

        // Calculate borders according to the transient position.
        if !self.generate_fix_fix_only(trans_pos, number_time_slots) {
            return false;
        }

        // Decode freq res.
        for k in 0..self.num_envelopes {
            self.freq_res[k] = if bs.read(RES_BITS) == 0 {
                FreqRes::Coarse
            } else {
                FreqRes::Fine
            };
        }
        true
    }

    /// Generates frame info for FIXFIX frame class used for low delay version.
    ///
    /// # Parameters
    ///
    /// - `tran_pos_internal`: Transient position.
    /// - `number_time_slots`: Number of time slots.
    ///
    /// # Return
    ///
    /// - `bool`: `true` if generation is successfull, `false` otherwise.
    fn generate_fix_fix_only(&mut self, tran_pos_internal: usize, number_time_slots: u8) -> bool {
        let table = match number_time_slots {
            8 => &tables::ENVELOPE_TABLE_8[tran_pos_internal],
            15 => &tables::ENVELOPE_TABLE_15[tran_pos_internal],
            16 => &tables::ENVELOPE_TABLE_16[tran_pos_internal],
            _ => return false,
        };

        // Looks up envelope distribution in table.
        for i in 1..table.num_env {
            self.borders[i] = table.borders[i - 1];
        }

        // Open and close frame border.
        self.borders[0] = 0;
        self.borders[table.num_env] = number_time_slots;
        self.num_envelopes = table.num_env;

        self.tran_env = table.tran_idx as i8;

        // Transient idx.
        let tran_idx = if self.tran_env != 0 {
            usize::from(table.tran_idx)
        } else {
            1
        };

        // Add noise floors.
        self.border_noise[0] = 0;
        self.border_noise[1] = self.borders[tran_idx];
        self.border_noise[2] = number_time_slots;

        // num_env is always > 1, so num_noise_envelopes is always 2 (IEC 14496-3 4.6.19.3.2).
        self.num_noise_envelopes = 2;

        true
    }

    /// Extracts the PVC frame information from the bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid data.
    /// - `frame_info_prev`: Previous frame information.
    /// - `pvc_mode`: PVC mode of current frame.
    /// - `pvc_mode_last`: PVC mode of the previous frame.
    ///
    /// # Return
    ///
    /// - `bool`: `true` if extraction is successfull, `false` otherwise.
    pub(super) fn extract_pvc(
        &mut self,
        bs: &mut Bitstream,
        frame_info_prev: &PrevFrameInfo,
        pvc_mode: Mode,
        pvc_mode_last: Mode,
    ) -> bool {
        self.tran_env = -1;

        // Init for bs noise_position == 0 in case a parse error is found below.
        self.num_envelopes = 1;
        self.num_noise_envelopes = 1;
        self.freq_res[0] = FreqRes::Coarse;

        self.noise_position = bs.read(PVC_NOISEPOSITION_BITS) as u8;

        // bs_var_len_hf: 1 or 3 bits.
        if bs.read_bit() == 1 {
            self.var_length = (bs.read(PVC_VAR_LEN_HF_BITS) + 1) as u8;

            if self.var_length > 3 {
                // Error case: assume bs_var_len_hf == 0.
                self.var_length = 0;
                // Reserved value => set parse error.
                return false;
            }
        } else {
            self.var_length = 0;
        }

        if self.noise_position != 0 {
            self.num_envelopes = 2;
            self.num_noise_envelopes = 2;
            // Clear memory of freq_res array.
            self.freq_res = [FreqRes::Coarse; MAX_ENVELOPES];
        }

        // Frame border calculation
        if pvc_mode > Mode::None {
            // Left timeborder-offset: use the timeborder of prev SBR frame.
            if frame_info_prev.num_envelopes > 0 {
                self.borders[0] = frame_info_prev.border - PVC_NTIMESLOT;
            } else {
                self.borders[0] = 0;
            }

            // Right timeborder-offset.
            self.borders[self.num_envelopes] = 16 + self.var_length;

            if self.num_envelopes == 2 {
                self.borders[1] = self.noise_position;
            }

            // Calculation of PVC time borders t_EPVC.
            if pvc_mode_last == Mode::None {
                // There was a legacy SBR frame before this frame => use bs_var_len' for
                // first PVC timeslot.
                self.pvc_borders[0] = self.borders[0];
            } else {
                self.pvc_borders[0] = 0;
            }

            if self.num_envelopes == 2 {
                self.pvc_borders[1] = self.borders[1];
            }
            self.pvc_borders[self.num_envelopes] = 16;

            // Calculation of SBR noise-floor time-border vector.
            for i in 0..=self.num_noise_envelopes {
                self.border_noise[i] = self.borders[i];
            }
        }

        true
    }

    /// Checks if frame information values are valid.
    ///
    /// # Parameters
    ///
    /// - `num_time_slots`: Number of QMF time slots per frame.
    /// - `overlap`: Amount of the QMF time slots overlap.
    /// - `time_step`: QMF slots to SBR slots step factor.
    ///
    /// # Return
    ///
    /// - `bool`: `true` if valid and `false` otherwise.
    pub(super) fn check(&self, num_time_slots: u8, overlap: u8, time_step: u8) -> bool {
        let max_pos = num_time_slots + (overlap / time_step);
        let start_pos = self.borders[0];
        let stop_pos = self.borders[self.num_envelopes];

        if self.num_envelopes < 1 || self.num_envelopes > MAX_ENVELOPES {
            return false;
        }

        if self.num_noise_envelopes > MAX_NOISE_ENVELOPES {
            return false;
        }

        // Skipped overlap > 0, because u8 can not be negative.
        if usize::from(overlap) > MAX_OV_COLS {
            return false;
        }

        if !(1..=4).contains(&time_step) {
            return false;
        }

        // Check that the start and stop positions of the frame are reasonable values.
        // Skipped self.borders[0] < 0, because u8 can not be negative.
        if start_pos >= stop_pos {
            return false;
        }

        // First envelope must start in or directly after the overlap buffer.
        if start_pos > max_pos - num_time_slots {
            return false;
        }

        // One complete frame must be ready for output after processing.
        if stop_pos < num_time_slots {
            return false;
        }

        if stop_pos > max_pos {
            return false;
        }

        // Check that the start border for every envelope is strictly later in time.
        for i in 0..self.num_envelopes {
            if self.borders[i] >= self.borders[i + 1] {
                return false;
            }
        }

        // Check that the envelope to be shortened is actually among the envelopes.
        if self.tran_env > self.num_envelopes as i8 {
            return false;
        }

        // Check the noise borders.
        if self.num_envelopes == 1 && self.num_noise_envelopes > 1 {
            return false;
        }

        if start_pos != self.border_noise[0]
            || stop_pos != self.border_noise[self.num_noise_envelopes]
        {
            return false;
        }

        // Check that the start border for every noise-envelope is strictly later in time.
        for i in 0..self.num_noise_envelopes {
            if self.border_noise[i] >= self.border_noise[i + 1] {
                return false;
            }
        }

        // Check that every noise border is the same as an envelope border.
        for i in 0..self.num_noise_envelopes {
            let mut j = 0;
            while j < self.num_envelopes {
                if self.borders[j] == self.border_noise[i] {
                    break;
                }
                j += 1;
            }
            if j == self.num_envelopes {
                return false;
            }
        }

        true
    }

    /// Sets parameters suitable for (lean) SBR concealment.
    ///
    /// # Parameters
    ///
    /// - `start_pos`: New start position of first envelope.
    /// - `stop_pos`: New stop position of first envelope.
    pub(super) fn conceal(&mut self, start_pos: u8, stop_pos: u8) {
        self.num_envelopes = 1;
        self.borders[0] = start_pos;
        self.borders[1] = stop_pos;
        self.freq_res[0] = FreqRes::Fine;
        self.tran_env = -1;
        self.num_noise_envelopes = 1;
        self.border_noise[0] = start_pos;
        self.border_noise[1] = stop_pos;
    }

    /// Sets first envelope's start position.
    ///
    /// # Parameters
    ///
    /// - `start_pos`: New start position of first envelope.
    pub(super) fn set_start_pos(&mut self, start_pos: u8) {
        self.borders[0] = start_pos;
    }

    /// Sets first noise envelope's start position.
    ///
    /// # Parameters
    ///
    /// - `start_pos`: New start position of first envelope.
    pub(super) fn set_start_pos_noise(&mut self, start_pos: u8) {
        self.border_noise[0] = start_pos;
    }

    /// Returns the frame class.
    pub(super) fn frame_class(&self) -> FrameClass {
        self.frame_class
    }

    /// Returns the number of envelopes.
    pub(super) fn num_envelopes(&self) -> usize {
        self.num_envelopes
    }

    /// Returns the borders of the envelopes.
    pub(super) fn borders(&self) -> &[u8; MAX_ENVELOPES + 1] {
        &self.borders
    }

    /// Returns the borders of noise envelopes.
    pub(super) fn border_noise(&self) -> &[u8; MAX_NOISE_ENVELOPES + 1] {
        &self.border_noise
    }

    /// Returns the PVC borders of the envelopes.
    pub(super) fn pvc_borders(&self) -> &[u8; MAX_NOISE_ENVELOPES + 1] {
        &self.pvc_borders
    }

    /// Returns the frequency resolution for each envelope.
    pub(super) fn freq_res(&self) -> &[FreqRes; MAX_ENVELOPES] {
        &self.freq_res
    }

    /// Returns the number of noise envelopes.
    pub(super) fn num_noise_envelopes(&self) -> usize {
        self.num_noise_envelopes
    }

    /// Returns transient envelope, -1 if none.
    pub(super) fn tran_env(&self) -> i8 {
        self.tran_env
    }
}
