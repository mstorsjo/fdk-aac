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
//! Enums used by the decorrelator module

use super::decorr_constants::{
    DECORR_MAX_PARAMETER_BANDS_LD, DECORR_MAX_PARAMETER_BANDS_MPS, DECORR_MAX_PARAMETER_BANDS_PS,
};

/// Decorrelator types.
#[derive(Debug, PartialEq, Eq, Copy, Clone)]
#[repr(C)]
pub enum DecorrType {
    Ps,   // Decorrelator type used by HEAACv2 and MPS LP.
    Usac, // Decorrelator type used by USAC
    Ld,   // Decorrelator type used by MPS Low Delay
}

/// Maximum parameter bands, depending on type of decorrelator.
#[derive(Debug, PartialEq, Eq, Copy, Clone, Default)]
#[repr(C)]
pub(super) enum DecorrMaxParameterBands {
    None = 0,
    Ps = DECORR_MAX_PARAMETER_BANDS_PS as isize,
    Ld = DECORR_MAX_PARAMETER_BANDS_LD as isize,
    #[default]
    Mps = DECORR_MAX_PARAMETER_BANDS_MPS as isize,
}

// Convert usize to DecorrMaxParameterBands.
impl From<usize> for DecorrMaxParameterBands {
    fn from(value: usize) -> Self {
        match value {
            0 => DecorrMaxParameterBands::None,
            DECORR_MAX_PARAMETER_BANDS_PS => DecorrMaxParameterBands::Ps,
            DECORR_MAX_PARAMETER_BANDS_LD => DecorrMaxParameterBands::Ld,
            DECORR_MAX_PARAMETER_BANDS_MPS => DecorrMaxParameterBands::Mps,
            _ => panic!("invalid value: {value}"),
        }
    }
}

// Convert DecorrMaxParameterBands to usize.
impl From<DecorrMaxParameterBands> for usize {
    fn from(value: DecorrMaxParameterBands) -> Self {
        match value {
            DecorrMaxParameterBands::None => 0,
            DecorrMaxParameterBands::Ps => DECORR_MAX_PARAMETER_BANDS_PS,
            DecorrMaxParameterBands::Ld => DECORR_MAX_PARAMETER_BANDS_LD,
            DecorrMaxParameterBands::Mps => DECORR_MAX_PARAMETER_BANDS_MPS,
        }
    }
}

/// Reverb band types.
#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
#[repr(C)]
pub(super) enum ReverbBandFiltType {
    #[default]
    NotExist, // Mark reverb band as non-existing (number of bands = 0).
    Delay,       // Reverb bands just contains delay elements and no allpass filters.
    CommonReal,  // Real filter coeffs, common filter coeffs within one reverb band.
    IndepCplxPs, // PS optimized implementation of general IndepCplx type.
}
