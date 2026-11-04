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
//! Bitstream syntax module
//!
//! Returns a bitstream element list.
//! Bitstream element syntax is returned based on given input parameters like audio object type,
//! number of channels, error protection configuration and elements flags.
//!
//! The element list contains raw data block information and a reference to the next raw data block
//! in the bitstream. Each raw data block contains a list of flags and information about the
//! presence of bitstream elements.
use crate::common::{aot::AudioObjectType, flags::ChannelFlags};

/// Raw Data Block list items
#[derive(Debug, PartialEq, Eq, Copy, Clone)]
#[repr(C)]
pub enum RdbId {
    ElementInstanceTag,
    CommonWindow, // -> decision for LinkSequence
    GlobalGain,
    IcsInfo,
    Ms,
    LtpDataPresent,
    SectionData,
    ScaleFactorData,
    Pulse,
    TnsDataPresent,
    TnsData,
    GainControlDataPresent,
    GainControlData,
    Esc1Hcr,
    Esc2Rvlc,
    SpectralData,
    ScaleFactorDataUsac,
    CoreMode, // -> decision for LinkSequence
    CommonTw,
    LpdChannelStream,
    TwData,
    Noise,
    AcSpectralData,
    FacData,
    TnsActive, // introduced in MPEG-D usac CD
    TnsDataPresentUsac,
    CommonMaxSfb,
    CoupledElements,  // only for CCE parsing
    GainElementLists, // only for CCE parsing

    // Non data list items
    AdtscrcStartReg1,
    AdtscrcStartReg2,
    AdtscrcEndReg1,
    AdtscrcEndReg2,
    NextChannel,
    LinkSequence,
    EndOfSequence,
}

#[derive(Default, Debug, PartialEq)]
#[repr(C)]
pub struct ElementList<'a> {
    id: &'a [RdbId],
    next: [Option<&'a ElementList<'a>>; 2],
}

// Bitstream data lists
//

// AOT {2,5,29}
// epConfig = -1
//
static EL_AAC_SCE: [RdbId; 13] = [
    RdbId::AdtscrcStartReg1,
    RdbId::ElementInstanceTag,
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::AdtscrcEndReg1,
    RdbId::EndOfSequence,
];

static NODE_AAC_SCE: ElementList = ElementList {
    id: &EL_AAC_SCE,
    next: [None, None],
};

// CCE
static EL_AAC_CCE: [RdbId; 15] = [
    RdbId::AdtscrcStartReg1,
    RdbId::ElementInstanceTag,
    RdbId::CoupledElements, // CCE specific
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::GainElementLists, // CCE specific
    RdbId::AdtscrcEndReg1,
    RdbId::EndOfSequence,
];

static NODE_AAC_CCE: ElementList = ElementList {
    id: &EL_AAC_CCE,
    next: [None, None],
};

static EL_AAC_CPE: [RdbId; 4] = [
    RdbId::AdtscrcStartReg1,
    RdbId::ElementInstanceTag,
    RdbId::CommonWindow,
    RdbId::LinkSequence,
];

static EL_AAC_CPE0: [RdbId; 23] = [
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::NextChannel,
    RdbId::AdtscrcStartReg2,
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::AdtscrcEndReg1,
    RdbId::AdtscrcEndReg2,
    RdbId::EndOfSequence,
];

static EL_AAC_CPE1: [RdbId; 23] = [
    RdbId::IcsInfo,
    RdbId::Ms,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::NextChannel,
    RdbId::AdtscrcStartReg2,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::GainControlDataPresent,
    RdbId::SpectralData,
    RdbId::AdtscrcEndReg1,
    RdbId::AdtscrcEndReg2,
    RdbId::EndOfSequence,
];

static NODE_AAC_CPE0: ElementList = ElementList {
    id: &EL_AAC_CPE0,
    next: [None, None],
};

static NODE_AAC_CPE1: ElementList = ElementList {
    id: &EL_AAC_CPE1,
    next: [None, None],
};

static NODE_AAC_CPE: ElementList = ElementList {
    id: &EL_AAC_CPE,
    next: [Some(&NODE_AAC_CPE0), Some(&NODE_AAC_CPE1)],
};

// AOT C- {17,23}
// epConfig = 0
//
static EL_AAC_SCE_EPC0: [RdbId; 14] = [
    RdbId::ElementInstanceTag,
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::GainControlDataPresent,
    RdbId::GainControlData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::TnsData,
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_AAC_SCE_EPC0: ElementList = ElementList {
    id: &EL_AAC_SCE_EPC0,
    next: [None, None],
};

static EL_AAC_CPE_EPC0: [RdbId; 3] = [
    RdbId::ElementInstanceTag,
    RdbId::CommonWindow,
    RdbId::LinkSequence,
];

static EL_AAC_CPE0_EPC0: [RdbId; 24] = [
    // ESC 1:
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::GainControlDataPresent,
    RdbId::Esc1Hcr,
    // ESC 2:
    RdbId::Esc2Rvlc,
    // ESC 3:
    RdbId::TnsData,
    // ESC 4:
    RdbId::SpectralData,
    RdbId::NextChannel,
    // ESC 1:
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::GainControlDataPresent,
    RdbId::Esc1Hcr,
    // ESC 2:
    RdbId::Esc2Rvlc,
    // ESC 3:
    RdbId::TnsData,
    // ESC 4:
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static EL_AAC_CPE1_EPC0: [RdbId; 24] = [
    // ESC 0:
    RdbId::IcsInfo,
    RdbId::Ms,
    // ESC 1:
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::GainControlDataPresent,
    RdbId::Esc1Hcr,
    // ESC 2:
    RdbId::Esc2Rvlc,
    // ESC 3:
    RdbId::TnsData,
    // ESC 4:
    RdbId::SpectralData,
    RdbId::NextChannel,
    // ESC 1:
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Pulse,
    RdbId::TnsDataPresent,
    RdbId::GainControlDataPresent,
    RdbId::Esc1Hcr,
    // ESC 2:
    RdbId::Esc2Rvlc,
    // ESC 3:
    RdbId::TnsData,
    // ESC 4:
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_AAC_CPE0_EPC0: ElementList = ElementList {
    id: &EL_AAC_CPE0_EPC0,
    next: [None, None],
};

static NODE_AAC_CPE1_EPC0: ElementList = ElementList {
    id: &EL_AAC_CPE1_EPC0,
    next: [None, None],
};

static NODE_AAC_CPE_EPC0: ElementList = ElementList {
    id: &EL_AAC_CPE_EPC0,
    next: [Some(&NODE_AAC_CPE0_EPC0), Some(&NODE_AAC_CPE1_EPC0)],
};

// AOT = 39
// epConfig = 0
//
static EL_ELD_SCE_EPC0: [RdbId; 10] = [
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_ELD_SCE_EPC0: ElementList = ElementList {
    id: &EL_ELD_SCE_EPC0,
    next: [None, None],
};

static EL_ELD_CPE_EPC0: [RdbId; 20] = [
    RdbId::IcsInfo,
    RdbId::Ms,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::SpectralData,
    RdbId::NextChannel,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::TnsDataPresent,
    RdbId::TnsData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_ELD_CPE_EPC0: ElementList = ElementList {
    id: &EL_ELD_CPE_EPC0,
    next: [None, None],
};

// AOT = 42
// epConfig = 0
//

static EL_USAC_COREMODE: [RdbId; 3] = [RdbId::CoreMode, RdbId::NextChannel, RdbId::LinkSequence];

static EL_USAC_SCE0_EPC0: [RdbId; 10] = [
    RdbId::TnsDataPresent,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static EL_USAC_LFE_EPC0: [RdbId; 6] = [
    RdbId::GlobalGain,
    RdbId::IcsInfo,
    RdbId::ScaleFactorDataUsac,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static EL_USAC_LPD_EPC0: [RdbId; 2] = [RdbId::LpdChannelStream, RdbId::EndOfSequence];

static NODE_USAC_SCE0_EPC0: ElementList = ElementList {
    id: &EL_USAC_SCE0_EPC0,
    next: [None, None],
};

static NODE_USAC_SCE1_EPC0: ElementList = ElementList {
    id: &EL_USAC_LPD_EPC0,
    next: [None, None],
};

static NODE_USAC_SCE_EPC0: ElementList = ElementList {
    id: &EL_USAC_COREMODE,
    next: [Some(&NODE_USAC_SCE0_EPC0), Some(&NODE_USAC_SCE1_EPC0)],
};

static LIST_USAC_CPE00_EPC0: [RdbId; 3] =
    [RdbId::TnsActive, RdbId::CommonWindow, RdbId::LinkSequence];

static EL_USAC_COMMON_TW: [RdbId; 2] = [RdbId::CommonTw, RdbId::LinkSequence];

static LIST_USAC_CPE0000_EPC0: [RdbId; 19] = [
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 0 , common_tw = 0
    RdbId::TnsDataPresentUsac,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::NextChannel,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE0001_EPC0: [RdbId; 18] = [
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 0 , common_tw = 1
    RdbId::TwData,
    RdbId::TnsDataPresentUsac,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::NextChannel,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE001_EPC0: [RdbId; 5] = [
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1
    RdbId::IcsInfo,
    RdbId::CommonMaxSfb,
    RdbId::Ms,
    RdbId::CommonTw,
    RdbId::LinkSequence,
];

static LIST_USAC_CPE0010_EPC0: [RdbId; 17] = [
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1 , common_tw = 0
    RdbId::TnsDataPresentUsac,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::NextChannel,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE0011_EPC0: [RdbId; 16] = [
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1 , common_tw = 1
    RdbId::TwData,
    RdbId::TnsDataPresentUsac,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::NextChannel,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE10_EPC0: [RdbId; 12] = [
    // core_mode0 = 1 , core_mode1 = 0
    RdbId::LpdChannelStream,
    RdbId::NextChannel,
    RdbId::TnsDataPresent,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE01_EPC0: [RdbId; 12] = [
    // core_mode0 = 0 , core_mode1 = 1
    RdbId::TnsDataPresent,
    RdbId::GlobalGain,
    RdbId::Noise,
    RdbId::IcsInfo,
    RdbId::TwData,
    RdbId::ScaleFactorDataUsac,
    RdbId::TnsData,
    RdbId::AcSpectralData,
    RdbId::FacData,
    RdbId::NextChannel,
    RdbId::LpdChannelStream,
    RdbId::EndOfSequence,
];

static LIST_USAC_CPE11_EPC0: [RdbId; 4] = [
    // core_mode0 = 1 , core_mode1 = 1
    RdbId::LpdChannelStream,
    RdbId::NextChannel,
    RdbId::LpdChannelStream,
    RdbId::EndOfSequence,
];

static NODE_USAC_CPE0000_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 0 , common_tw = 0
    id: &LIST_USAC_CPE0000_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE0010_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1 , common_tw = 0
    id: &LIST_USAC_CPE0010_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE0001_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 0 , common_tw = 1
    id: &LIST_USAC_CPE0001_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE0011_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1 , common_tw = 1
    id: &LIST_USAC_CPE0011_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE000_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 0
    id: &EL_USAC_COMMON_TW,
    next: [Some(&NODE_USAC_CPE0000_EPC0), Some(&NODE_USAC_CPE0001_EPC0)],
};

static NODE_USAC_CPE001_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0 , common_window = 1
    id: &LIST_USAC_CPE001_EPC0,
    next: [Some(&NODE_USAC_CPE0010_EPC0), Some(&NODE_USAC_CPE0011_EPC0)],
};

static NODE_USAC_CPE00_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 0
    id: &LIST_USAC_CPE00_EPC0,
    next: [Some(&NODE_USAC_CPE000_EPC0), Some(&NODE_USAC_CPE001_EPC0)],
};

static NODE_USAC_CPE10_EPC0: ElementList = ElementList {
    // core_mode0 = 1 , core_mode1 = 0
    id: &LIST_USAC_CPE10_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE01_EPC0: ElementList = ElementList {
    // core_mode0 = 0 , core_mode1 = 1
    id: &LIST_USAC_CPE01_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE11_EPC0: ElementList = ElementList {
    // core_mode0 = 1 , core_mode1 = 1
    id: &LIST_USAC_CPE11_EPC0,
    next: [None, None],
};

static NODE_USAC_CPE0_EPC0: ElementList = ElementList {
    // core_mode0 = 0
    id: &EL_USAC_COREMODE,
    next: [Some(&NODE_USAC_CPE00_EPC0), Some(&NODE_USAC_CPE01_EPC0)],
};

static NODE_USAC_CPE1_EPC0: ElementList = ElementList {
    // core_mode0 = 1
    id: &EL_USAC_COREMODE,
    next: [Some(&NODE_USAC_CPE10_EPC0), Some(&NODE_USAC_CPE11_EPC0)],
};

static NODE_USAC_CPE_EPC0: ElementList = ElementList {
    id: &EL_USAC_COREMODE,
    next: [Some(&NODE_USAC_CPE0_EPC0), Some(&NODE_USAC_CPE1_EPC0)],
};

static NODE_USAC_LFE_EPC0: ElementList = ElementList {
    id: &EL_USAC_LFE_EPC0,
    next: [None, None],
};

// AOT = 20
// epConfig = 0
static EL_SCAL_SCE_EPC0: [RdbId; 11] = [
    RdbId::IcsInfo,
    RdbId::TnsDataPresent,
    RdbId::LtpDataPresent,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::TnsData,
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_SCAL_SCE_EPC0: ElementList = ElementList {
    id: &EL_SCAL_SCE_EPC0,
    next: [None, None],
};

static EL_SCAL_CPE_EPC0: [RdbId; 22] = [
    RdbId::IcsInfo,
    RdbId::Ms,
    RdbId::TnsDataPresent,
    RdbId::LtpDataPresent,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::TnsData,
    RdbId::SpectralData,
    RdbId::NextChannel,
    RdbId::TnsDataPresent,
    RdbId::LtpDataPresent,
    RdbId::GlobalGain,
    RdbId::SectionData,
    RdbId::ScaleFactorData,
    RdbId::Esc1Hcr,
    RdbId::Esc2Rvlc,
    RdbId::TnsData,
    RdbId::SpectralData,
    RdbId::EndOfSequence,
];

static NODE_SCAL_CPE_EPC0: ElementList = ElementList {
    id: &EL_SCAL_CPE_EPC0,
    next: [None, None],
};

impl<'a> ElementList<'a> {
    pub fn new() -> Self {
        ElementList::default()
    }

    /// Get bitstream element list for given input parameters
    ///
    /// # Parameters
    ///
    /// - `aot`: Audio object type
    /// - `num_channels`: Number of channels contained in the current element
    /// - `el_flags`: Element specific flags
    ///
    ///  Returns ElementList parser guidance structure
    ///
    /// # Examples
    /// ```
    /// use aac::common::{
    ///     aot::AudioObjectType,
    ///     bs_syntax::ElementList,
    ///     flags::{self, ChannelFlags},
    /// };
    ///
    /// let aot = AudioObjectType::AotAacLc;
    /// let num_channels = 2;
    /// let el_flags = ChannelFlags::GA_CCE;
    /// let element_list = ElementList::get_bitstream_element_list(aot, num_channels, el_flags);
    /// ```
    pub fn get_bitstream_element_list(
        aot: AudioObjectType,
        num_channels: u8,
        el_flags: ChannelFlags,
    ) -> Option<&'a ElementList<'a>> {
        match aot {
            AudioObjectType::AotAacLc | AudioObjectType::AotSbr | AudioObjectType::AotPs => {
                if el_flags.contains(ChannelFlags::GA_CCE) {
                    Some(&NODE_AAC_CCE)
                } else if num_channels == 1 {
                    Some(&NODE_AAC_SCE)
                } else {
                    Some(&NODE_AAC_CPE)
                }
            }
            AudioObjectType::AotErAacLc | AudioObjectType::AotErAacLd => {
                if num_channels == 1 {
                    Some(&NODE_AAC_SCE_EPC0)
                } else {
                    Some(&NODE_AAC_CPE_EPC0)
                }
            }
            AudioObjectType::AotUsac => {
                if el_flags.contains(ChannelFlags::LFE) {
                    assert!(num_channels == 1);
                    return Some(&NODE_USAC_LFE_EPC0);
                }
                if num_channels == 1 {
                    Some(&NODE_USAC_SCE_EPC0)
                } else {
                    Some(&NODE_USAC_CPE_EPC0)
                }
            }
            AudioObjectType::AotErAacScal => {
                if num_channels == 1 {
                    Some(&NODE_SCAL_SCE_EPC0)
                } else {
                    Some(&NODE_SCAL_CPE_EPC0)
                }
            }
            AudioObjectType::AotErAacEld => {
                if num_channels == 1 {
                    Some(&NODE_ELD_SCE_EPC0)
                } else {
                    Some(&NODE_ELD_CPE_EPC0)
                }
            }
            AudioObjectType::AotNone
            | AudioObjectType::AotNullObject
            | AudioObjectType::AotEscape
            | AudioObjectType::Undefined => None,
        }
    }

    /// Get next element from given ElementList
    ///
    /// # Parameters
    ///
    /// - `decision_index`: Index value to select next element, value should be 0 or 1
    ///
    ///  Returns next ElementList from list if available, else None
    ///
    /// # Examples
    /// ```
    /// use aac::common::{
    ///     aot::AudioObjectType,
    ///     bs_syntax::ElementList,
    ///     flags::{self, ChannelFlags},
    /// };
    ///
    /// let aot = AudioObjectType::AotAacLc;
    /// let num_channels = 2;
    /// let el_flags = ChannelFlags::GA_CCE;
    /// let element_list = ElementList::get_bitstream_element_list(aot, num_channels, el_flags);
    ///
    /// let next_lement_list = element_list.unwrap().next(0);
    /// ```
    pub fn next(&self, decision_index: usize) -> Option<&'a ElementList<'a>> {
        self.next[decision_index]
    }

    /// Get raw data block id list of ElementList
    ///
    ///  Returns slice of raw data block id list
    ///
    /// # Examples
    /// ```
    /// use aac::common::{
    ///     aot::AudioObjectType,
    ///     bs_syntax::ElementList,
    ///     flags::{self, ChannelFlags},
    /// };
    ///
    /// let aot = AudioObjectType::AotAacLc;
    /// let num_channels = 2;
    /// let el_flags = ChannelFlags::GA_CCE;
    /// let element_list = ElementList::get_bitstream_element_list(aot, num_channels, el_flags);
    ///
    /// let id = element_list.unwrap().id();
    /// ```
    pub fn id(&self) -> &'a [RdbId] {
        self.id
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn new() {
        let el = ElementList::new();
        assert_eq!(el.id, []);
        assert_eq!(el.next, [None, None]);
    }

    #[test]
    fn bitstream_element_list_aac_lc_sbr_ps() {
        let aot_val = AudioObjectType::AotAacLc;
        let num_channels = 2;

        // case 1
        let el_flags = ChannelFlags::GA_CCE;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_CCE));

        // case 2
        let aot_val = AudioObjectType::AotAacLc;
        let el_flags = ChannelFlags::empty();
        let num_channels = 1;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_SCE));

        // case 3
        let aot_val = AudioObjectType::AotSbr;
        let el_flags = ChannelFlags::empty();
        let num_channels = 2;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_CPE));
    }

    #[test]
    fn bitstream_element_list_eraac_lc_ld() {
        let aot_val = AudioObjectType::AotErAacLc;
        let el_flags = ChannelFlags::GA_CCE;
        let num_channels = 1;

        // case 1
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_SCE_EPC0));

        // case 2
        let aot_val = AudioObjectType::AotErAacLc;
        let num_channels = 2;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_CPE_EPC0));
    }

    #[test]
    fn bitstream_element_list_usac() {
        let aot_val = AudioObjectType::AotUsac;
        let num_channels = 1;

        // case 1
        let el_flags = ChannelFlags::LFE;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_USAC_LFE_EPC0));

        // case 2
        let el_flags = ChannelFlags::empty();
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_USAC_SCE_EPC0));

        // case 3
        let num_channels = 2;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_USAC_CPE_EPC0));
    }

    #[test]
    fn bitstream_element_list_eraac_eld() {
        let aot_val = AudioObjectType::AotErAacEld;
        let el_flags = ChannelFlags::empty();
        let num_channels = 1;

        // case 1
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_ELD_SCE_EPC0));

        // case 2
        let num_channels = 2;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_ELD_CPE_EPC0));
    }

    #[test]
    fn bitstream_element_list_unsupported_aot() {
        let aot_val = AudioObjectType::AotNullObject;
        let el_flags = ChannelFlags::empty();
        let num_channels = 1;

        // case 1
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, None);
    }

    #[test]
    fn element_list_id_next() {
        let aot_val = AudioObjectType::AotSbr;
        let el_flags = ChannelFlags::empty();
        let num_channels = 2;
        let el = ElementList::get_bitstream_element_list(aot_val, num_channels, el_flags);
        assert_eq!(el, Some(&NODE_AAC_CPE));

        // case 1
        let id = el.unwrap().id();
        assert_eq!(id, &EL_AAC_CPE);

        // case 2
        let next_el_0 = el.unwrap().next(0);
        let next_el_1 = el.unwrap().next(1);
        assert_eq!(next_el_0, Some(&NODE_AAC_CPE0));
        assert_eq!(next_el_1, Some(&NODE_AAC_CPE1));
    }
}
