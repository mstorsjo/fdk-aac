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
//! Constants used by AAC core concealment

pub const MAX_COMF_NOISE_LEVEL: f32 = 2147483647.0; //i32 = 0x7FFFFFFF;
pub const DFLT_COMF_NOISE_LEVEL_FL: f32 = 1048576.0; //i32 = 0x100000;

// Default settings.
pub const DFLT_FADEOUT_FRAMES: i32 = 6;
pub const DFLT_FADEIN_FRAMES: i32 = 5;
pub const DFLT_MUTE_RELEASE_FRAMES: i32 = 0;

// For parameter conversion.
pub const PARAMETER_BITS: i16 = 8;
pub const MAX_QUANT_FACTOR: i16 = (1 << PARAMETER_BITS) - 1;
pub const MIN_ATTENUATION_FACTOR_025_FL: f32 = 0.971627951577106174;
// pub const MIN_ATTENUATION_FACTOR_050_FL: f32 = 0.944060876285923380;

pub const MAX_NUM_FADE_FACTORS: usize = 32;

const CONCEAL_AU_SIZE: usize = 16;
/// Byte sequence for concealment AU indicating that frame should be concealed.
#[rustfmt::skip]
pub(crate) static A_CONCEAL_AU: [u8; CONCEAL_AU_SIZE] = [
    0x44, 0xB3, 0x8D, 0x5A, 0x36, 0xAA, 0x4E, 0xD5,
    0xBA, 0xDE, 0xCF, 0xA6, 0xD7, 0xE2, 0x88, 0x15];

/// Output rendering mode.
#[repr(C)]
#[derive(Default, Debug, PartialEq, Copy, Clone)]
pub enum AacDecoderRenderMode {
    #[default]
    Invalid = 0,
    Imdct,
    EldFb,
    Lpd,
}

// Converts i8 to AacDecoderRenderMode.
impl From<i8> for AacDecoderRenderMode {
    fn from(value: i8) -> Self {
        match value {
            0 => AacDecoderRenderMode::Invalid,
            1 => AacDecoderRenderMode::Imdct,
            2 => AacDecoderRenderMode::EldFb,
            3 => AacDecoderRenderMode::Lpd,
            _ => AacDecoderRenderMode::Invalid,
        }
    }
}

// Converts AacDecoderRenderMode to i8.
impl TryFrom<AacDecoderRenderMode> for i8 {
    type Error = &'static str;

    fn try_from(value: AacDecoderRenderMode) -> Result<Self, Self::Error> {
        match value {
            AacDecoderRenderMode::Invalid => Ok(0),
            AacDecoderRenderMode::Imdct => Ok(1),
            AacDecoderRenderMode::EldFb => Ok(2),
            AacDecoderRenderMode::Lpd => Ok(3),
            // _ => Err("invalid AacDecoderRenderMode value."),
        }
    }
}

/// Concealment states.
#[repr(C)]
#[derive(Default, Debug, PartialEq, PartialOrd, Copy, Clone)]
pub enum ConcealmentState {
    #[default]
    Ok,
    Single,
    FadeIn,
    Mute,
    FadeOut,
}

/// TD fading types.
#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
pub enum TDfadingType {
    #[default]
    /// Fades time domain to spectral mute.
    ToSpectralMute = 1,
    /// Fades time domain from spectral mute.
    FromSpectralMute,
    /// Fades time domain.
    FadeTimeDomain,
}

/// Concealment expand types.
#[repr(C)]
#[derive(Debug, PartialEq)]
pub enum ConcealmentExpandType {
    NoExpand,
    Expand,
    _Compress,
}

/// Fading directions
#[derive(Debug, PartialEq)]
pub enum FadeDirection {
    /// Fade Out -> Fade In.
    OutToIn = 0,
    /// Fade In -> Fade Out.
    InToOut,
}
