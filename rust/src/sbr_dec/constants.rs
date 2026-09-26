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
//! SBR decoder constants

/// Max amount of input (analysis) QMF frequency bands.
pub const SBRDEC_MAX_ANALYSIS_CHANNELS: usize = 32;
/// Max amount of output (synthesis) QMF frequency bands.
pub const SBRDEC_MAX_SYNTHESIS_CHANNELS: usize = 64;
/// Maximum number of subband subsamples.
pub const SBRDEC_MAX_SUBBAND_SUBSAMPLES: usize = 64;

/// Maximum number of overlapping QMF timeslots of two consecutive frames.
pub const MAX_RATE: usize = 4;
pub const MAX_OV_COLS: usize = 3 * MAX_RATE;

/// Filter order for LPC filtering.
pub const LPC_ORDER: usize = 2;

/// Maximum number of frequency coefficients.
pub const MAX_FREQ_COEFFS_DUAL_RATE: u8 = 48;
pub const MAX_FREQ_COEFFS_QUAD_RATE: u8 = 56;
pub const MAX_FREQ_COEFFS: usize = MAX_FREQ_COEFFS_QUAD_RATE as usize;

/// Maximum number of patches.
pub const MAX_NUM_PATCHES: usize = 6;
/// Maximum number of noise values
pub const MAX_NOISE_COEFFS: usize = 5;

/// Maximum supported frame size of input time signal.
pub const SBRDEC_MAX_FRAMESIZE: usize = 1024;
/// Maximum number of QMF timeslots for one frame.
pub const MAX_COLS: usize = SBRDEC_MAX_FRAMESIZE / SBRDEC_MAX_ANALYSIS_CHANNELS * MAX_RATE / 2;
/// Maximum number of QMF timeslots spanned by the envelopes of one frame.
pub const MAX_ENV_COLS: usize = MAX_COLS + MAX_OV_COLS;

/// Maximum number of SBR element channels.
pub const MAX_EL_CHANNELS: usize = 2;
/// Maximum number of limiter bands
pub const MAX_NUM_LIMITER_BANDS: usize = 12;

pub const FREQ_SCALE_DEFAULT: u8 = 2;
pub const ALTER_SCALE_DEFAULT: bool = true;
pub const NOISE_BANDS_DEFAULT: u8 = 2;

// header
pub const SI_SBR_START_FREQ_BITS: u8 = 4;
pub const SI_SBR_STOP_FREQ_BITS: u8 = 4;
pub const SI_SBR_FREQ_SCALE_BITS: u8 = 2;
pub const SI_SBR_ALTER_SCALE_BITS: u8 = 1;
pub const SI_SBR_NOISE_BANDS_BITS: u8 = 2;

// Frame Information
pub(crate) const MAX_ENVELOPES_USAC: usize = 8;
pub(crate) const MAX_ENVELOPES: usize = MAX_ENVELOPES_USAC;
pub(crate) const MAX_NOISE_ENVELOPES: usize = 2;

/// Maximum number of noise values.
pub(crate) const MAX_NUM_NOISE_VALUES: usize = MAX_NOISE_ENVELOPES * MAX_NOISE_COEFFS;

pub(crate) const PVC_NOISEPOSITION_BITS: u8 = 4;
pub(crate) const PVC_VAR_LEN_HF_BITS: u8 = 2;
pub(crate) const PVC_NTIMESLOT: u8 = 16;

pub(crate) const CLA_BITS: u8 = 2;
pub(crate) const ABS_BITS: u8 = 2;
pub(crate) const RES_BITS: u8 = 1;
pub(crate) const REL_BITS: u8 = 2;
pub(crate) const ENV_BITS: u8 = 2;
pub(crate) const NUM_BITS: u8 = 2;

pub const SI_SBR_AMP_RES_BITS: u8 = 1;
pub const SI_SBR_XOVER_BAND_BITS: u8 = 3;
pub const SI_SBR_RESERVED_BITS_HDR: u8 = 2;
pub const SI_SBR_LIMITER_BANDS_BITS: u8 = 2;
pub const SI_SBR_LIMITER_GAINS_BITS: u8 = 2;
pub const SI_SBR_INTERPOL_FREQ_BITS: u8 = 1;
pub const SI_SBR_SMOOTHING_LENGTH_BITS: u8 = 1;
pub const SBR_LIMITER_BANDS_DEFAULT: u8 = 2;
pub const SBR_LIMITER_GAINS_DEFAULT: u8 = 2;
pub const SBR_INTERPOL_FREQ_DEFAULT: u8 = 1;
pub const SBR_SMOOTHING_LENGTH_DEFAULT: u8 = 1;

pub const SBRDEC_MAX_DELAY_FRAMES: usize = 1;
pub const SBRDEC_MAX_EL_CHANNELS: usize = 2;
pub const SBRDEC_LD_MPS_QMF: u32 = 512;

// Overlap size = 3*RATE.
pub const SBR_DUAL_RATE_OVERLAP_SIZE: u8 = 3 * 2;
pub const SBR_QUAD_RATE_OVERLAP_SIZE: u8 = 3 * 4;

pub const SBRDEC_MAX_SAMPLE_RATE_IN: u32 = 96000;
pub const SBRDEC_MAX_SAMPLE_RATE_OUT: u32 = 96000;
pub const SBRDEC_MAX_ELEMENTS: usize = 8;

// Defines CRC parameters.
pub(super) const SBR_DECODER_CRC_LEN: u8 = 10;
pub(super) const SBR_DECODER_CRC_POLY: u32 = 0x0633;
pub(super) const SBR_DECODER_CRC_START_VALUE: u32 = 0x0000;

// Defines output delay for ELD format.
pub(super) const SBR_DECODER_OUTPUT_DELAY_ELD: u32 = 64;
pub(super) const SBR_DECODER_OUTPUT_DELAY_ELD_DS: u32 = SBR_DECODER_OUTPUT_DELAY_ELD / 2;
pub(super) const SBR_DECODER_OUTPUT_DELAY_ELD_MPS: u32 = 32;

// Defines output delay for HE-AAC format.
pub(super) const SBR_DECODER_OUTPUT_DELAY_HE_AAC: u32 = 962;
pub(super) const SBR_DECODER_OUTPUT_DELAY_HE_AAC_DS: u32 = SBR_DECODER_OUTPUT_DELAY_HE_AAC / 2;
pub(super) const SBR_DECODER_OUTPUT_DELAY_HE_AAC_SKIP_QMF_SYN: u32 = 257;
