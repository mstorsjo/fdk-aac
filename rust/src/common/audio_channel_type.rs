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
//!  Speaker description tags

///  Do not change the enumeration values unless it keeps the following segmentation:
///  - `Bit 0-4`: Horizontal position (0: none, 1: front, 2: side, 3: back, 4: lfe)
///  - `Bit 5-7`: Vertical position (0: normal, 1: top, 2: bottom)
#[repr(C)]
#[derive(Default, Debug, Clone, Copy, PartialEq)]
pub enum AudioChannelType {
    #[default]
    None = 0x00,
    Front = 0x01, // Front speaker position (at normal height)
    Side = 0x02,  // Side speaker position (at normal height)
    Back = 0x03,  // Back speaker position (at normal height)
    Lfe = 0x04,   // Low frequency effect speaker postion (front)

    Top = 0x10,      // Top speaker area (for combination with speaker positions)
    FrontTop = 0x11, // Top front speaker = (FRONT|TOP)
    SideTop = 0x12,  // Top side speaker  = (SIDE |TOP)
    BackTop = 0x13,  // Top back speaker  = (BACK |TOP)

    Bottom = 0x20,      // Bottom speaker area (for combination with speaker positions)
    FrontBottom = 0x21, // Bottom front speaker = (FRONT|BOTTOM)
    SideBottom = 0x22,  // Bottom side speaker  = (SIDE |BOTTOM)
    BackBottom = 0x23,  // Bottom back speaker  = (BACK |BOTTOM)
}

impl From<AudioChannelType> for u32 {
    fn from(act: AudioChannelType) -> u32 {
        act as u32
    }
}

impl From<u32> for AudioChannelType {
    fn from(value: u32) -> Self {
        match value {
            x if x == AudioChannelType::None as u32 => AudioChannelType::None,
            x if x == AudioChannelType::Front as u32 => AudioChannelType::Front,
            x if x == AudioChannelType::Side as u32 => AudioChannelType::Side,
            x if x == AudioChannelType::Back as u32 => AudioChannelType::Back,
            x if x == AudioChannelType::Lfe as u32 => AudioChannelType::Lfe,
            x if x == AudioChannelType::Top as u32 => AudioChannelType::Top,
            x if x == AudioChannelType::FrontTop as u32 => AudioChannelType::FrontTop,
            x if x == AudioChannelType::SideTop as u32 => AudioChannelType::SideTop,
            x if x == AudioChannelType::BackTop as u32 => AudioChannelType::BackTop,
            x if x == AudioChannelType::Bottom as u32 => AudioChannelType::Bottom,
            x if x == AudioChannelType::FrontBottom as u32 => AudioChannelType::FrontBottom,
            x if x == AudioChannelType::SideBottom as u32 => AudioChannelType::SideBottom,
            x if x == AudioChannelType::BackBottom as u32 => AudioChannelType::BackBottom,
            _ => panic!("Invalid AudioChannelType value: {}", value),
        }
    }
}
