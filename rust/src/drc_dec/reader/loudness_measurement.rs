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
//! Loudness measurement

use super::super::drc_error::DrcError;
use super::{
    loudness_measurement_system::{MeasurementSystem, MeasurementSystemRequestBonus},
    loudness_method_definition::MethodDefinition,
};
use crate::common::bitstream::Bitstream;

// ------------------------------------------------------------------------- //
/// Types of reliability for the loudness measurement.
#[repr(C)]
#[derive(Default, Debug, PartialEq)]
enum Reliability {
    #[default]
    Unknown = 0,
    Unverified = 1,
    Ceiling = 2,
    Accurate = 3,
}

/// `TryFrom<u8>` trait for `Reliability`.
impl TryFrom<u8> for Reliability {
    type Error = &'static str;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Reliability::Unknown),
            1 => Ok(Reliability::Unverified),
            2 => Ok(Reliability::Ceiling),
            3 => Ok(Reliability::Accurate),
            _ => Err("Invalid value"),
        }
    }
}

// 11 = num of MeasurementSystemRequestBonus entries.
// 12 = num of MeasurementSystem entries (up to MeasurementSystem::RmsE).
static METHOD_ORDERING_TAB: [[i32; 12]; 11] = [
    // default = bonus1770
    [0, 0, 8, 0, 1, 3, 0, 5, 6, 7, 4, 2],
    // bonus1770
    [0, 0, 8, 0, 1, 3, 0, 5, 6, 7, 4, 2],
    // bonusUser
    [0, 0, 1, 0, 8, 5, 0, 2, 3, 4, 6, 7],
    // bonusExpert
    [0, 0, 3, 0, 1, 8, 0, 4, 5, 6, 7, 2],
    // ResA
    [0, 0, 5, 0, 1, 3, 0, 8, 6, 7, 4, 2],
    // ResB
    [0, 0, 5, 0, 1, 3, 0, 6, 8, 7, 4, 2],
    // ResC
    [0, 0, 5, 0, 1, 3, 0, 6, 7, 8, 4, 2],
    // ResD
    [0, 0, 3, 0, 1, 7, 0, 4, 5, 6, 8, 2],
    // ResE
    [0, 0, 1, 0, 7, 5, 0, 2, 3, 4, 6, 8],
    // ProgramLoudness
    [0, 0, 1, 0, 0, 0, 0, 2, 3, 4, 0, 0],
    // PeakLoudness
    [0, 7, 0, 0, 0, 0, 6, 5, 4, 3, 2, 1],
];

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, PartialEq)]
/// Method value order structure to hold loudness value and order for methods.
pub(super) struct MethodValueOrder {
    pub(super) loudness_val: f32,
    pub(super) order: i32,
}

impl MethodValueOrder {
    pub(super) fn new() -> Self {
        Self {
            loudness_val: 0.0_f32,
            order: -1,
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, PartialEq)]
/// `LoudnessMeasurement` structure that holds parameters for loudness normalization settings.
pub(super) struct LoudnessMeasurement {
    /// Measurement definition index.
    pub(super) method_definition: MethodDefinition,
    /// Measurement method value.
    pub(super) method_value: f32,
    /// Measurement system index.
    pub(super) measurement_system: MeasurementSystem,
    /// Indicates reliability of measurement.
    reliability: Reliability,
}

impl LoudnessMeasurement {
    /// Reads and decodes `MethodDefinition` data from bitstream. Also calculates method values.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `DrcError`.
    fn decode_method_value(&mut self, bs: &mut Bitstream) {
        match self.method_definition {
            MethodDefinition::UnknownOther
            | MethodDefinition::ProgramLoudness
            | MethodDefinition::AnchorLoudness
            | MethodDefinition::MaxOfLoudnessRange
            | MethodDefinition::MomentaryLoudnessMax
            | MethodDefinition::ShortTermLoudnessMax => {
                let value = bs.read(8);
                self.method_value = -57.75_f32 + value as f32 * 0.25_f32;
            }
            MethodDefinition::LoudnessRange => {
                let tmp = bs.read(8);
                self.method_value = match tmp {
                    0 => 0.0_f32,
                    1..=128 => tmp as f32 * 0.25_f32,
                    129..=204 => tmp as f32 * 0.5_f32 - 32.0_f32,
                    _ => tmp as f32 - 134.0_f32,
                };
            }
            MethodDefinition::MixingLevel => {
                let value = bs.read(5);
                self.method_value = value as f32 + 80.0_f32;
            }
            MethodDefinition::RoomType => {
                let value = bs.read(2);
                self.method_value = value as f32;
            }
            MethodDefinition::ShortTermLoudness => {
                let value = bs.read(8);
                self.method_value = -116.0_f32 + value as f32 * 0.5_f32;
            }
        }
    }

    /// Returns method value order (`MethodValueOrder`).
    pub(super) fn method_value(
        &self,
        msrb: MeasurementSystemRequestBonus,
        mvo_in: MethodValueOrder,
    ) -> MethodValueOrder {
        let mut mvo_out = mvo_in;

        let row = usize::from(msrb);
        let col = usize::from(self.measurement_system);
        if row <= usize::from(MeasurementSystemRequestBonus::PeakLoudness)
            && col < usize::from(MeasurementSystem::Reserved12)
            && METHOD_ORDERING_TAB[row][col] > mvo_in.order
        {
            mvo_out.order = METHOD_ORDERING_TAB[row][col];
            mvo_out.loudness_val = self.method_value;
        }
        mvo_out
    }

    /// Reads `LoudnessMeasurement` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data.
    ///
    ///  Returns `DrcError`.
    pub(super) fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let method_defi = bs.read(4) as u8;

        match MethodDefinition::try_from(method_defi) {
            Ok(md) => {
                self.method_definition = md;
                self.decode_method_value(bs);
            }
            Err(_) => {
                return Err(DrcError::NotOk);
            }
        }

        self.measurement_system = MeasurementSystem::try_from(bs.read(4) as u8).unwrap();
        self.reliability = Reliability::try_from(bs.read(2) as u8).unwrap();
        Ok(())
    }
}
