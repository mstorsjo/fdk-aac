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
//! DRC effect types.

use super::super::constants as drc_const;
use super::SelectionProcessError;

#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
/// DRC effect type structure that holds list of requested effect types.
pub(super) struct DrcEffectType {
    pub(super) num_requests: u8,
    pub(super) num_requests_desired: u8,
    pub(super) request: [DrcEffectTypeRequest; drc_const::MAX_REQUESTS_DRC_EFFECT_TYPE],
}

#[repr(C)]
#[derive(Default, Copy, Clone, Debug, PartialEq, PartialOrd)]
/// Requestable DRC effect types.
pub enum DrcEffectTypeRequest {
    Off = -1,
    #[default]
    None = 0,
    Night = 1,
    Noisy = 2,
    Limited = 3,
    LowLevel = 4,
    Dialog = 5,
    GeneralCompr = 6,
    Expand = 7,
    Artistic = 8,
}

impl From<i8> for DrcEffectTypeRequest {
    fn from(val: i8) -> DrcEffectTypeRequest {
        match val {
            -1 => DrcEffectTypeRequest::Off,
            0 => DrcEffectTypeRequest::None,
            1 => DrcEffectTypeRequest::Night,
            2 => DrcEffectTypeRequest::Noisy,
            3 => DrcEffectTypeRequest::Limited,
            4 => DrcEffectTypeRequest::LowLevel,
            5 => DrcEffectTypeRequest::Dialog,
            6 => DrcEffectTypeRequest::GeneralCompr,
            7 => DrcEffectTypeRequest::Expand,
            8 => DrcEffectTypeRequest::Artistic,
            _ => panic!("Invalid DrcEffectTypeRequest variant."),
        }
    }
}

impl From<DrcEffectTypeRequest> for i8 {
    fn from(val: DrcEffectTypeRequest) -> i8 {
        match val {
            DrcEffectTypeRequest::Off => -1,
            DrcEffectTypeRequest::None => 0,
            DrcEffectTypeRequest::Night => 1,
            DrcEffectTypeRequest::Noisy => 2,
            DrcEffectTypeRequest::Limited => 3,
            DrcEffectTypeRequest::LowLevel => 4,
            DrcEffectTypeRequest::Dialog => 5,
            DrcEffectTypeRequest::GeneralCompr => 6,
            DrcEffectTypeRequest::Expand => 7,
            DrcEffectTypeRequest::Artistic => 8,
        }
    }
}

impl From<isize> for DrcEffectTypeRequest {
    fn from(val: isize) -> DrcEffectTypeRequest {
        match val {
            -1 => DrcEffectTypeRequest::Off,
            0 => DrcEffectTypeRequest::None,
            1 => DrcEffectTypeRequest::Night,
            2 => DrcEffectTypeRequest::Noisy,
            3 => DrcEffectTypeRequest::Limited,
            4 => DrcEffectTypeRequest::LowLevel,
            5 => DrcEffectTypeRequest::Dialog,
            6 => DrcEffectTypeRequest::GeneralCompr,
            7 => DrcEffectTypeRequest::Expand,
            8 => DrcEffectTypeRequest::Artistic,
            _ => panic!("Invalid DrcEffectTypeRequest variant."),
        }
    }
}

impl DrcEffectTypeRequest {
    /// Return a `DrcEffectTypeRequest` from the `FALLBACK_EFFECT_TYPE_REQUESTS` table.
    /// For details, see E.1 of ISO/IEC DIS 23003-4.
    /// # Parameters
    /// - `requested_val`: `DrcEffectTypeRequest` variant type.
    /// - `index`: Index to a specific fallback `DrcEffectTypeRequest`. Range [0..5).
    ///
    /// # Return
    /// - `DrcEffectTypeRequest`
    pub(super) fn fallback_effect_type_request(
        requested_val: DrcEffectTypeRequest,
        i: usize,
    ) -> Result<DrcEffectTypeRequest, SelectionProcessError> {
        match requested_val {
            DrcEffectTypeRequest::Night => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[0][i]),
            DrcEffectTypeRequest::Noisy => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[1][i]),
            DrcEffectTypeRequest::Limited => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[2][i]),
            DrcEffectTypeRequest::LowLevel => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[3][i]),
            DrcEffectTypeRequest::Dialog => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[4][i]),
            DrcEffectTypeRequest::GeneralCompr => Ok(FALLBACK_EFFECT_TYPE_REQUESTS[5][i]),
            _ => Err(SelectionProcessError::ParamInvalid),
        }
    }
}

/// Table E.1 of ISO/IEC DIS 23003-4: Recommended order of fallback effect type requests.
static FALLBACK_EFFECT_TYPE_REQUESTS: [[DrcEffectTypeRequest; 5]; 6] = [
    // Night
    [
        DrcEffectTypeRequest::GeneralCompr,
        DrcEffectTypeRequest::Noisy,
        DrcEffectTypeRequest::Limited,
        DrcEffectTypeRequest::LowLevel,
        DrcEffectTypeRequest::Dialog,
    ],
    // Noisy
    [
        DrcEffectTypeRequest::GeneralCompr,
        DrcEffectTypeRequest::Night,
        DrcEffectTypeRequest::Limited,
        DrcEffectTypeRequest::LowLevel,
        DrcEffectTypeRequest::Dialog,
    ],
    // Limited
    [
        DrcEffectTypeRequest::GeneralCompr,
        DrcEffectTypeRequest::Night,
        DrcEffectTypeRequest::Noisy,
        DrcEffectTypeRequest::LowLevel,
        DrcEffectTypeRequest::Dialog,
    ],
    // LowLevel
    [
        DrcEffectTypeRequest::GeneralCompr,
        DrcEffectTypeRequest::Noisy,
        DrcEffectTypeRequest::Night,
        DrcEffectTypeRequest::Limited,
        DrcEffectTypeRequest::Dialog,
    ],
    // Dialog
    [
        DrcEffectTypeRequest::GeneralCompr,
        DrcEffectTypeRequest::Night,
        DrcEffectTypeRequest::Noisy,
        DrcEffectTypeRequest::Limited,
        DrcEffectTypeRequest::LowLevel,
    ],
    // General
    [
        DrcEffectTypeRequest::Night,
        DrcEffectTypeRequest::Noisy,
        DrcEffectTypeRequest::Limited,
        DrcEffectTypeRequest::LowLevel,
        DrcEffectTypeRequest::Dialog,
    ],
];
