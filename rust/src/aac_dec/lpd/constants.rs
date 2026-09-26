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
//! USAC constants

/// LP filter order.
pub const M_LP_FILTER_ORDER: usize = 16;

/// Interpolation filter length.
pub const L_INTERPOL: usize = 17;

/// Number of division (20ms) per 80ms frame.
pub const NB_DIV: usize = 4;
/// Subframe size (5ms).
pub const L_SUBFR: usize = 64;

/// Bass postfilter delay (subframe).
pub const BPF_SFD: usize = 1;
// Bass postfilter delay (samples).
pub const BPF_DELAY: usize = BPF_SFD * L_SUBFR;

/// Number of mods: ACELP, TCX20, TCX40, TCX80.
pub const N_MODS: usize = 4;

/// Delay of up-sampling filter (bass post-filter)
pub const L_FILT: usize = 12;

/// Minimum pitch lag with resolution 1/4.
pub const PIT_MIN_12K8: u16 = 34;
/// Maximum pitch lag for fs=12.8kHz.
pub const PIT_MAX_12K8: u16 = 231;
/// Maximum pitch lag (= 411 for fs_max = 24000).
pub const PIT_MAX_MAX: usize = 411;

/// Limit for LP (linear prediction) synthesis filtering.
pub const LPD_SYN_FILT_LIMIT: f32 = (1 << 20) as f32;

// Definitions for coreCoderFrameLength = 1024.
/// Length of one 80ms superframe.
pub const L_FRAME_PLUS_1024: usize = 1024;
/// Length of one ACELP or TCX20 frame.
pub const L_DIV_1024: usize = L_FRAME_PLUS_1024 / NB_DIV;
/// Number of 5ms subframes per division.
pub const NB_SUBFR_1024: usize = L_DIV_1024 / L_SUBFR;
/// Number of 5ms subframes per 80ms frame.
pub const NB_SUBFR_SUPERFR_1024: usize = L_FRAME_PLUS_1024 / L_SUBFR;
/// AAC delay (subframe).
pub const AAC_SFD_1024: usize = NB_SUBFR_SUPERFR_1024 / 2;
/// Synthesis delay (subframe).
pub const SYN_SFD_1024: usize = AAC_SFD_1024 - BPF_SFD;
/// Synthesis delay (samples).
pub const SYN_DELAY_1024: usize = SYN_SFD_1024 * L_SUBFR;
/// Forward aliasing cancellation (FAC) frame length.
pub const LFAC_1024: usize = L_DIV_1024 / 2;
/// FAC frame length for transitions of EIGHT_SHORT FD <-> LPD.
pub const LFAC_SHORT_1024: usize = L_DIV_1024 / 4;

// Maximum values (used for memory allocation).
pub const L_FRAME_PLUS: usize = L_FRAME_PLUS_1024;
pub const L_DIV: usize = L_DIV_1024;
pub const NB_SUBFR_SUPERFR: usize = NB_SUBFR_SUPERFR_1024;

/// Length of FAC.
pub const LFAC: usize = LFAC_1024;
/// Length of FAC for transitions of EIGHT_SHORT FD<->LPD.
pub const LFAC_SHORT: usize = LFAC_SHORT_1024;

/// Number of 5ms subframes per division.
pub const NB_SUBFR: usize = NB_SUBFR_1024;

pub const SYN_SFD: usize = SYN_SFD_1024;
pub const SYN_DELAY: usize = SYN_DELAY_1024;

// USAC _END
