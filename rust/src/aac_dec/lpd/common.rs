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
//! Enums and functions used commonly by LPD's components

/// Defines the LPD mode.
#[repr(u8)]
#[derive(PartialEq, Eq, Clone, Copy, Debug, Default)]
pub enum LpdMode {
    Acelp = 0,
    Tcx20 = 1,
    Tcx40 = 2,
    Tcx80 = 3,
    TcxTdConceal = 4,
    #[default]
    Undef = 8,
    // Not LPD means in fact: FD
    NotLpd = 255,
}

/// Convert local data types to speech codec mode.
macro_rules! impl_to_lpd_mode {
    ($original_type:ty, $lpd_mode_type:ty) => {
        impl From<$original_type> for $lpd_mode_type {
            fn from(value: $original_type) -> Self {
                match value {
                    0 => LpdMode::Acelp,
                    1 => LpdMode::Tcx20,
                    2 => LpdMode::Tcx40,
                    3 => LpdMode::Tcx80,
                    4 => LpdMode::TcxTdConceal,
                    8 => LpdMode::Undef,
                    255 => LpdMode::NotLpd,
                    _ => panic!("invalid value: {value}"),
                }
            }
        }
    };
}

impl_to_lpd_mode!(u8, LpdMode);
impl_to_lpd_mode!(u16, LpdMode);
impl_to_lpd_mode!(u32, LpdMode);
impl_to_lpd_mode!(u64, LpdMode);
impl_to_lpd_mode!(usize, LpdMode);
impl_to_lpd_mode!(i16, LpdMode);
impl_to_lpd_mode!(i32, LpdMode);
impl_to_lpd_mode!(i64, LpdMode);
impl_to_lpd_mode!(isize, LpdMode);

/// Convert speech codec mode to local data types.
macro_rules! impl_from_lpd_mode {
    ($lpd_mode_type:ty, $convert_type:ty) => {
        impl From<$lpd_mode_type> for $convert_type {
            fn from(value: $lpd_mode_type) -> Self {
                match value {
                    LpdMode::Acelp => 0u8.into(),
                    LpdMode::Tcx20 => 1u8.into(),
                    LpdMode::Tcx40 => 2u8.into(),
                    LpdMode::Tcx80 => 3u8.into(),
                    LpdMode::TcxTdConceal => 4u8.into(),
                    LpdMode::Undef => 8u8.into(),
                    LpdMode::NotLpd => 255u8.into(),
                }
            }
        }
    };
}

impl_from_lpd_mode!(LpdMode, u8);
impl_from_lpd_mode!(LpdMode, u16);
impl_from_lpd_mode!(LpdMode, u32);
impl_from_lpd_mode!(LpdMode, u64);
impl_from_lpd_mode!(LpdMode, usize);
impl_from_lpd_mode!(LpdMode, i16);
impl_from_lpd_mode!(LpdMode, i32);
impl_from_lpd_mode!(LpdMode, i64);
impl_from_lpd_mode!(LpdMode, isize);

impl LpdMode {
    /// Indicates ACELP.
    pub fn is_acelp(&self) -> bool {
        *self == LpdMode::Acelp
    }

    /// Indicates TCX.
    pub fn is_tcx(&self) -> bool {
        *self == LpdMode::Tcx20 || *self == LpdMode::Tcx40 || *self == LpdMode::Tcx80
    }

    /// Indicates Time domain coding.
    pub fn is_time_domain(&self) -> bool {
        *self == LpdMode::Acelp || *self == LpdMode::TcxTdConceal
    }

    /// Determines the number of ACELP or TCX subframes per frame (1-4).
    pub fn num_subframes(&self) -> usize {
        if self.is_tcx() {
            1 << (*self as u8 - 1)
        } else if self.is_time_domain() {
            1
        } else {
            // LpdMode::NotLpd
            4
        }
    }

    /// Get subframe length in samples.
    pub fn subframe_length(&self, frame_length: usize) -> usize {
        if self.is_tcx() {
            frame_length >> (3 - (*self as u8))
        } else if self.is_time_domain() {
            frame_length >> 2
        } else {
            0
        }
    }
}

/// Decodes the 7 bit binary word from the bitstream representing the gain.
pub fn decode_gain(gain_code: u8) -> f32 {
    // gain * 2^(gain_e) = 10^(gain_code/28)
    // 0.08223518189264448871 = log(10.)/28.
    f32::exp(0.08223518189264448871_f32 * gain_code as f32)
}
