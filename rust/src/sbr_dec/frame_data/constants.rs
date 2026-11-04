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
//! Constants for frame data

use super::super::constants::{MAX_ENVELOPES, MAX_FREQ_COEFFS};

pub(crate) const ADD_HARMONICS_FLAGS_SIZE: usize = 2;
pub(crate) const MAX_NUM_ENVELOPE_VALUES: usize = MAX_ENVELOPES * MAX_FREQ_COEFFS;
pub(crate) const LO: usize = 0;
pub(crate) const HI: usize = 1;
pub(crate) const SI_SBR_PITCH_IN_BINS_BITS: u8 = 7;

pub(crate) const SI_SBR_EXTENDED_DATA_BITS: u8 = 1;
pub(crate) const SI_SBR_EXTENSION_SIZE_BITS: u8 = 4;
pub(crate) const SI_SBR_EXTENSION_ESC_COUNT_BITS: u8 = 8;
pub(crate) const SI_SBR_EXTENSION_ID_BITS: u8 = 2;
pub(crate) const EXTENSION_ID_PS_CODING: u8 = 2;
pub(crate) const EXTENSION_ID_ESBR: u8 = 3;

pub(crate) const SI_SBR_DATA_EXTRA_BITS: u8 = 1;
pub(crate) const SI_SBR_RESERVED_BITS_DATA: u8 = 4;
pub(crate) const SI_SBR_COUPLING_BITS: u8 = 1;
pub(crate) const SI_SBR_INVF_MODE_BITS: u8 = 2;

// Flag indicating that USAC global independency flag is active.
pub(crate) const SI_SBR_DOMAIN_BITS: u8 = 1;

pub(crate) const SI_SBR_START_ENV_BITS_AMP_RES_3_0: u8 = 6;
pub(crate) const SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0: u8 = 5;
pub(crate) const SI_SBR_START_ENV_BITS_AMP_RES_1_5: u8 = 7;
pub(crate) const SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5: u8 = 6;
pub(crate) const ENV_DATA_TABLE_COMP_FACTOR: u8 = 0;
pub(crate) const ENV_DATA_TABLE_COMP_FACTOR_BAL: u8 = 1;

pub(crate) const SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0: u8 = 5;
pub(crate) const SI_SBR_START_NOISE_BITS_AMP_RES_3_0: u8 = 5;

pub(crate) const PVC_DIVMODE_BITS: u8 = 3;
pub(crate) const PVC_PVCID_BITS: u8 = 7;

pub(crate) const MAP_NS_MODE_2_NS: [[u8; 2]; 2] = [[16, 4], [12, 3]];

pub(crate) const NOISE_FLOOR_OFFSET: f32 = 6.0;
pub(crate) const SBR_ENERGY_PAN_OFFSET: f32 = 12.0;

pub(crate) const DECAY: f32 = 1.0;
pub(crate) const DECAY_COUPLING: f32 = 1.0;

pub(crate) const SBR_MAX_ENERGY: f32 = 35.0;

// SBR_NOISE_FLOOR_LOWER_LIMIT actually refers to the _highest_ noise energy
pub(crate) const SBR_NOISE_FLOOR_LOWER_LIMIT: f32 = 0.0;

// SBR_NOISE_FLOOR_UPPER_LIMIT actually refers to the _lowest_ noise energy
pub(crate) const SBR_NOISE_FLOOR_UPPER_LIMIT: f32 = 35.0;
