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
//! Concealment of SAC decoder parameters

use super::common::BsXxxDataMode;
use super::constants::{MAX_PARAMETER_BANDS, MAX_PARAMETER_SETS};
use itertools::izip;

// Default dynamic parameter values:
const MPEGS_CONCEAL_DEFAULT_NUM_KEEP_FRAMES: u32 = 10;
const MPEGS_CONCEAL_DEFAULT_FADE_OUT_SLOPE_LENGTH: u32 = 5;
const MPEGS_CONCEAL_DEFAULT_FADE_IN_SLOPE_LENGTH: u32 = 5;

/// Enum keeping states of concealment strategy for SAC decoder.
#[repr(C)]
#[derive(PartialEq, Eq, Debug, Default)]
enum ConcealmentState {
    #[default]
    Init = 0,
    Ok,
    Keep,
    FadeToDefault,
    Default,
    FadeFromDefault,
}

/// Struct holding an internal state machine for selecting
/// the concealment strategy and various counters of frames,
/// used for SAC decoder.
#[repr(C)]
#[derive(Debug)]
pub(super) struct ConcealmentInfo {
    /// State of internal state machine (fade-in/out etc).
    conceal_state: ConcealmentState,
    /// Counter for fade-in/out handling.
    cnt_state_frames: u32,
    /// Counter for the number of consecutive good frames.
    cnt_valid_frames: u32,

    num_keep_frames: u32,
    num_fade_out_frames: u32,
    num_fade_in_frames: u32,
}

impl Default for ConcealmentInfo {
    fn default() -> Self {
        // Set default params.
        Self {
            conceal_state: Default::default(),
            cnt_state_frames: 0,
            cnt_valid_frames: 0,
            num_keep_frames: MPEGS_CONCEAL_DEFAULT_NUM_KEEP_FRAMES,
            num_fade_out_frames: MPEGS_CONCEAL_DEFAULT_FADE_OUT_SLOPE_LENGTH,
            num_fade_in_frames: MPEGS_CONCEAL_DEFAULT_FADE_IN_SLOPE_LENGTH,
        }
    }
}

impl ConcealmentInfo {
    /// Applies concealment over spatial parameters.
    ///
    /// # Parameters
    /// - `cmp_idx_data`: Indices of compact spatial parameter data.
    /// - `prev_idx`: OTT indices from previous frame.
    /// - `bs_xxx_data_mode`: Indicates how new information for a parameter subset is encoded.
    /// - `stop_band`: Last hybrid band to process.
    /// - `num_param_sets`: Number of parameter sets.
    ///
    /// # Return
    /// - `is_processing_applied: bool`
    pub(super) fn apply(
        &mut self,
        cmp_idx_data: &[[i8; MAX_PARAMETER_BANDS]],
        prev_idx: &mut [i8; MAX_PARAMETER_BANDS],
        bs_xxx_data_mode: &mut [BsXxxDataMode; MAX_PARAMETER_SETS],
        stop_band: usize,
        num_param_sets: usize,
    ) -> bool {
        let mut is_processing_applied = false;
        let data_mode: Option<BsXxxDataMode>;

        match self.conceal_state {
            ConcealmentState::Init => {
                data_mode = Some(BsXxxDataMode::Dflt);
            }
            ConcealmentState::Ok => {
                // Nothing to do.
                data_mode = None;
            }
            ConcealmentState::Keep => {
                data_mode = Some(BsXxxDataMode::Keep);
            }
            ConcealmentState::FadeToDefault => {
                // Start simple fade out.
                let fac =
                    ((self.cnt_state_frames + 1) as f32) / ((self.num_fade_out_frames + 1) as f32);
                for prev_i in prev_idx.iter_mut().take(stop_band) {
                    *prev_i = (f32::from(*prev_i) - fac * f32::from(*prev_i)) as i8;
                }

                data_mode = Some(BsXxxDataMode::Keep);
                is_processing_applied = true;
            }
            ConcealmentState::Default => {
                prev_idx[0..stop_band].fill(0);

                data_mode = Some(BsXxxDataMode::Keep);
                is_processing_applied = true;
            }
            ConcealmentState::FadeFromDefault => {
                let fac =
                    ((self.cnt_valid_frames + 1) as f32) / ((self.num_fade_in_frames + 1) as f32);
                for (prev_i, cmp_i) in
                    izip!(prev_idx.iter_mut(), cmp_idx_data[num_param_sets - 1].iter())
                        .take(stop_band)
                {
                    *prev_i = (fac * f32::from(*cmp_i)) as i8;
                }

                data_mode = Some(BsXxxDataMode::Keep);
                is_processing_applied = true;
            }
        }

        if let Some(dm) = data_mode {
            bs_xxx_data_mode[0..num_param_sets].fill(dm);
        }

        is_processing_applied
    }

    /// Initializes `self` with default concealment values.
    pub(super) fn init(&mut self) {
        *self = Default::default();
    }

    /// Creates a new `SpatialDecConcealmentInfo` instance, filled with `Default` values.
    pub(super) fn new() -> Self {
        Default::default()
    }

    /// Resets internal's state machine of `self`.
    pub(super) fn reset(&mut self) {
        self.conceal_state = ConcealmentState::Init;
    }

    /// Updates internal states of `self`, according to `SpatialDecConcealmentState` states.
    ///
    /// # Parameters
    ///
    /// - `is_frame_ok`: Is current frame OK.
    pub(super) fn update_state(&mut self, is_frame_ok: bool) {
        if is_frame_ok {
            self.cnt_valid_frames += 1;
        } else {
            self.cnt_valid_frames = 0;
        }

        match self.conceal_state {
            ConcealmentState::Init => {
                if is_frame_ok {
                    // Next state: Ok.
                    self.conceal_state = ConcealmentState::Ok;
                    self.cnt_state_frames = 0;
                }
            }
            ConcealmentState::Ok => {
                if !is_frame_ok {
                    // Next state: Keep.
                    self.conceal_state = ConcealmentState::Keep;
                    self.cnt_state_frames = 0;
                }
            }
            ConcealmentState::Keep => {
                self.cnt_state_frames += 1;
                if is_frame_ok {
                    // Next state: Ok.
                    self.conceal_state = ConcealmentState::Ok;
                } else if self.cnt_state_frames >= self.num_keep_frames {
                    if self.num_fade_out_frames == 0 {
                        // Next state: Default.
                        self.conceal_state = ConcealmentState::Default;
                    } else {
                        // Next state: Fade to default.
                        self.conceal_state = ConcealmentState::FadeToDefault;
                        self.cnt_state_frames = 0;
                    }
                }
            }
            ConcealmentState::FadeToDefault => {
                self.cnt_state_frames += 1;
                if self.cnt_valid_frames > 0 {
                    // Next state: Fade in from default.
                    self.conceal_state = ConcealmentState::FadeFromDefault;
                    self.cnt_state_frames = 0;
                } else if self.cnt_state_frames >= self.num_fade_out_frames {
                    // Next state: Default.
                    self.conceal_state = ConcealmentState::Default;
                }
            }
            ConcealmentState::Default => {
                if self.cnt_valid_frames > 0 {
                    if self.num_fade_in_frames == 0 {
                        // Next state: Ok.
                        self.conceal_state = ConcealmentState::Ok;
                    } else {
                        // Next state: Fade in from default.
                        self.conceal_state = ConcealmentState::FadeFromDefault;
                        self.cnt_valid_frames = 0;
                    }
                }
            }
            ConcealmentState::FadeFromDefault => {
                self.cnt_valid_frames += 1;
                if is_frame_ok {
                    if self.cnt_valid_frames >= self.num_fade_in_frames {
                        // Next state: Ok.
                        self.conceal_state = ConcealmentState::Ok;
                    }
                } else {
                    // Next state: Fade to default.
                    self.conceal_state = ConcealmentState::FadeToDefault;
                    self.cnt_state_frames = 0;
                }
            }
        }
    }
}
