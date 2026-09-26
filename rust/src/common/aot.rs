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
//! Audio object type (AOT) definitions

// Enable only supported AOTs from enum list
#[derive(Copy, Clone, Default, PartialEq, PartialOrd, Debug)]
#[repr(C)]
pub enum AudioObjectType {
    AotNone = -1,
    #[default]
    AotNullObject = 0,
    // /// Main profile
    // AotAacMain = 1,
    /// Low Complexity object
    AotAacLc = 2,
    // AotAacSsr = 3,
    // /// AAC + SSR
    // /// AAC LTP object
    // AotAacLtp = 4,
    /// SBR
    AotSbr = 5,
    // /// AAC scalable object
    // AotAacScal = 6,
    // /// TwinVQ
    // AotTwinVq = 7,
    /// CELP
    // AotCelp = 8,
    // /// HVXC
    // AotHvxc = 9,
    // /// reserved
    // AotRsvd10 = 10,
    // /// reserved
    // AotRsvd11 = 11,
    // /// TTSI Object
    // AotTtsi = 12,
    // /// Main Synthetic object
    // AotMainSynth = 13,
    // /// Wavetable Synthesis object
    // AotWavTabSynth = 14,
    // /// General MIDI object
    // AotGenMidi = 15,
    // /// Algorithmic Synthesis and Audio FX object
    // AotAlgSynthAudFx = 16,
    /// Error Resilient(ER) AAC Low Complexity
    AotErAacLc = 17,
    // /// reserved
    // AotRsvd18 = 18,
    // /// Error Resilient(ER) AAC LTP object
    // AotErAacLtp = 19,
    /// Error Resilient(ER) AAC Scalable object
    AotErAacScal = 20,
    // /// Error Resilient(ER) TwinVQ object
    // AotErTwinVq = 21,
    // /// Error Resilient(ER) BSAC object
    // AotErBsac = 22,
    /// Error Resilient(ER) AAC LowDelay object
    AotErAacLd = 23,
    // /// Error Resilient(ER) CELP object
    // AotErCelp = 24,
    // /// Error Resilient(ER) HVXC object
    // AotErHvxc = 25,
    // /// Error Resilient(ER) HILN object
    // AotErHiln = 26,
    // /// Error Resilient(ER) Parametric object
    // AotErPara = 27,
    // /// might become SSC
    // AotRsvd28 = 28,
    /// PS, Parametric Stereo (includes SBR)
    AotPs = 29,
    // /// MPEG Surround
    // AotMpegs = 30,
    /// Signal AOT uses more than 5 bits
    AotEscape = 31,
    // /// MPEG-Layer1 in mp4
    // AotMp3onmp4L1 = 32,
    // /// MPEG-Layer2 in mp4
    // AotMp3onmp4L2 = 33,
    // /// MPEG-Layer3 in mp4
    // AotMp3onmp4L3 = 34,
    // /// might become DST
    // AotRsvd35 = 35,
    // /// might become ALS
    // AotRsvd36 = 36,
    // /// AAC + SLS
    // AotAacSls = 37,
    // /// SLS
    // AotSls = 38,
    /// AAC Enhanced Low Delay
    AotErAacEld = 39,
    /// USAC
    AotUsac = 42,
    // /// SAOC
    // AotSaoc = 43,
    // /// Low Delay MPEG Surround
    // AotLdMpegs = 44,
    // /// Interim AOT for Rsvd50
    // AotRsvd50 = 50,
    // // Pseudo AOTs
    // /// Virtual AOT MP2 Main profile
    // AotMp2AacMain = 128,
    // /// Virtual AOT MP2 Low Complexity profile
    // AotMp2AacLc = 129,
    // /// Virtual AOT MP2 Scalable Sampling Rate profile
    // AotMp2AacSsr = 130,
    // /// Virtual AOT MP2 Low Complexity Profile with SBR
    // AotMp2Sbr = 132,
    // /// Virtual AOT for DAB (Layer2 with scalefactor CRC)
    // AotDab = 134,
    // /// Virtual AOT for DAB plus AAC-LC
    // AotDabplusAacLc = 135,
    // /// Virtual AOT for DAB plus HE-AAC
    // AotDabplusSbr = 136,
    // /// Virtual AOT for DAB plus HE-AAC v2
    // AotDabplusPs = 137,
    // /// Virtual AOT for plain mp1
    // AotPlainMp1 = 140,
    // /// Virtual AOT for plain mp2
    // AotPlainMp2 = 141,
    // /// Virtual AOT for plain mp3
    // AotPlainMp3 = 142,
    // /// Virtual AOT for DRM (ER-AAC-SCAL without SBR)
    // AotDrmAac = 143,
    // /// Virtual AOT for DRM (ER-AAC-SCAL with SBR)
    // AotDrmSbr = 144,
    // /// Virtual AOT for DRM (ER-AAC-SCAL with SBR and MPEG-PS)
    // AotDrmMpegPs = 145,
    // /// Virtual AOT for DRM Surround (ER-AAC-SCAL (+SBR) +MPS)
    // AotDrmSurround = 146,
    // /// Virtual AOT for DRM with USAC
    // AotDrmUsac = 147,
    // /// Virtual AOT for MPEG-H 3D audio
    // AotMpegh3da = 150,
    // /// Virtual AOT for MPEG-D residuals
    // AotMpegdResiduals = 256,
    Undefined,
}

impl From<i32> for AudioObjectType {
    fn from(value: i32) -> Self {
        match value {
            -1 => AudioObjectType::AotNone,
            0 => AudioObjectType::AotNullObject,
            2 => AudioObjectType::AotAacLc,
            5 => AudioObjectType::AotSbr,
            17 => AudioObjectType::AotErAacLc,
            20 => AudioObjectType::AotErAacScal,
            23 => AudioObjectType::AotErAacLd,
            29 => AudioObjectType::AotPs,
            31 => AudioObjectType::AotEscape,
            39 => AudioObjectType::AotErAacEld,
            42 => AudioObjectType::AotUsac,
            _ => AudioObjectType::Undefined,
        }
    }
}

impl AudioObjectType {
    pub fn is_lowdelay_aot(&self) -> bool {
        *self == AudioObjectType::AotErAacLd || *self == AudioObjectType::AotErAacEld
    }
}
