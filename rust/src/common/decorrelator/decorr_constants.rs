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
//! Constants used by the decorrelator module

pub(super) const DECORR_MAX_PARAMETER_BANDS_MPS: usize = 28;
pub(super) const DECORR_MAX_PARAMETER_BANDS_PS: usize = 20;
pub(super) const DECORR_MAX_PARAMETER_BANDS_LD: usize = 23;
pub(super) const DECORR_MAX_PARAMETER_BANDS: usize = DECORR_MAX_PARAMETER_BANDS_MPS;

pub(super) const MAX_DECORR_SEED_LD: usize = 1;
pub(super) const MAX_DECORR_SEED_USAC: usize = 1;

pub(super) const DECORR_ZERO_PADDING: usize = 0;

pub(super) const NUM_DECORR_BANDS: usize = 4;
/// Number of filter links for the all-pass filter.
pub(super) const NUM_ALLPASS_LINKS: usize = 3;
/// Different configs defined by `bs_decorr_config` bitstream field.
pub(super) const NUM_DECORR_CONFIGS: usize = 3;

pub(super) const DECORR_FILTER_ORDER_BAND_0_MPS: usize = 20;
pub(super) const DECORR_FILTER_ORDER_BAND_1_MPS: usize = 15;
pub(super) const DECORR_FILTER_ORDER_BAND_2_MPS: usize = 6;
pub(super) const DECORR_FILTER_ORDER_BAND_3_MPS: usize = 3;

pub(super) const DECORR_FILTER_ORDER_BAND_0_USAC: usize = 10;
pub(super) const DECORR_FILTER_ORDER_BAND_1_USAC: usize = 8;
pub(super) const DECORR_FILTER_ORDER_BAND_2_USAC: usize = 3;
pub(super) const DECORR_FILTER_ORDER_BAND_3_USAC: usize = 2;

//pub(super) const DECORR_FILTER_ORDER_BAND_0_LD: usize = 0;
pub(super) const DECORR_FILTER_ORDER_BAND_1_LD: usize = DECORR_FILTER_ORDER_BAND_1_MPS;
pub(super) const DECORR_FILTER_ORDER_BAND_2_LD: usize = DECORR_FILTER_ORDER_BAND_2_MPS;
pub(super) const DECORR_FILTER_ORDER_BAND_3_LD: usize = DECORR_FILTER_ORDER_BAND_3_MPS;

pub(super) const DECORR_FILTER_ORDER_PS: usize = 12;

// -- Lengths of buffers belonging to `DecorrDec`. -- //
pub(super) const DECORR_STATE_BUF_LEN_PS_HQ: usize = 360;
pub(super) const DECORR_DELAY_BUF_LEN_PS_HQ: usize = 257;

// Worst case for USAC is `bs_decorr_config` == 1
pub(super) const DECORR_STATE_BUF_LEN_USAC_1: usize = 509;
pub(super) const DECORR_DELAY_BUF_LEN_USAC_1: usize = 643;

// Worst case for MPS LD is `bs_decorr_config` == 1
pub(super) const DECORR_STATE_BUF_LEN_MPS_LD_1: usize = 825;
pub(super) const DECORR_DELAY_BUF_LEN_MPS_LD_1: usize = 373;
// ----------------------------------------------- //
