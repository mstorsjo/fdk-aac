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
//! Constants commonly used by DRC's components.

use super::common::DrcBitstreamLocation;

pub const DRCDEC_MAX_CHANNELS: usize = 2;

/// Maximum AAC frame size for USAC.
pub const DRCDEC_MAX_FRAME_SIZE: u16 = 4096;
/// Maximum supported sample rate.
pub const DRCDEC_MAX_SAMPLERATE: u32 = 96000;
/// Minimum supported sample rate.
pub(super) const DRCDEC_MIN_SAMPLERATE: u32 = 1000;

// Values expressed in LUFS.
pub const DRC_TARGET_LOUDNESS_MAX_RECOMMENDED_VALUE: i8 = -10;
pub const DRC_TARGET_LOUDNESS_MAX_VALUE: i8 = 0;
pub const DRC_TARGET_LOUDNESS_MIN_VALUE: i8 = -63;
pub const UNDEFINED_LOUDNESS_VALUE: f32 = 1000.0_f32;

/// Number of locations where the DRC sets are being applied to the signal. See
/// `ProcessingLocation`.
pub(super) const ACTIVE_DRC_LOCATIONS: usize = 2;

// Values expressed in LUFS.
pub(super) const DEFAULT_LOUDNESS_NORMALIZATION_GAIN_MAX: f32 = 1000.0_f32;

/// Downmix ID for DRC set to be applied to the base layout.
pub(super) const DOWNMIX_ID_BASE_LAYOUT: u8 = 0x0;
/// Downmix ID for DRC set to be applied to the base layout or any downmix. It indicates that the
/// DRC set can be applied before or after the downmix.
/// This ID is not permitted for a ducking DRC set.
pub(super) const DOWNMIX_ID_ANY_DOWNMIX: u8 = 0x7F;

/// Target input loudness of the DRC characteristic applied to generate the DRC gains in the
/// decoder.
pub(super) const DRC_INPUT_LOUDNESS_TARGET: f32 = -31.0_f32;

/// Maximum number of DRC bands for channel group.
pub(super) const DRC_MAX_BANDS: usize = 4;

/// Represents infinity for sigmoidal exponent.
pub(super) const EXP_INFINITY: f32 = 1000.0_f32;

/// The selected location where the uniDRC gain sequences can be found in the bitstream. Set to
/// location selected by system.
pub(super) const LOCATION_SELECTED: DrcBitstreamLocation = DrcBitstreamLocation::Mp4InstreamUniDrc;

/// Maximum number of DRC sets applied simultaneously.
pub(super) const MAX_ACTIVE_DRCS: usize = 3;

pub(super) const MAX_CUSTOM_CHARACTERISTICS: usize = 16;
pub(super) const MAX_CUSTOM_CHARACTERISTIC_NODES: usize = 4;

pub(super) const MAX_DOWNMIX_IDS: u8 = 8;

/// Maximum number of downmix instructions.
pub(super) const MAX_DOWNMIX_INSTRUCTIONS: u8 = 4;

/// Maximum number of DRC set extensions.
pub(super) const MAX_EXTENSIONS: usize = 8;
/// Maximum number of DRC coefficients.
pub(super) const MAX_DRC_COEFFICIENTS: usize = 2;

/// Maximum number of DRC instructions.
pub(super) const MAX_DRC_INSTRUCTIONS: u8 = 8;

/// Maximum number of loudness info in loudness info set.
pub(super) const MAX_LOUDNESS_INFOS: u8 = 8;

/// Maximum number of loudness measurements in loudness info.
pub(super) const MAX_LOUDNESS_MEASUREMENTS: u8 = 8;

/// Maximum number of interpolation nodes per gain sequence.
pub(super) const MAX_NODES: usize = 16;

pub(super) const MAX_REQUESTS_DOWNMIX_ID: usize = 15;
pub(super) const MAX_REQUESTS_DRC_EFFECT_TYPE: usize = 15;
pub(super) const MAX_REQUESTS_DRC_FEATURE: usize = 7;

/// Maximum number of DRC gain sequences.
/// A DRC sequence is a series of DRC gain values that can be applied to one or more audio chan.
pub(super) const MAX_SEQUENCES: u8 = 8;

pub(super) const UNDEFINED_LOUDNESS_NORMALIZATION_GAIN: f32 = 0.0_f32;
