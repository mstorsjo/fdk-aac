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
//! Flags

pub use bitflags::bitflags;

bitflags! {
    /// Audio Codec flags
    #[derive(Copy, Clone, Debug, Default, PartialEq)]
    #[repr(C)]
    pub struct ACFlags: u32 {
        /// aacSectionDataResilienceFlag flag (from ASC): 1 means use virtual codebooks
        const ER_VCB11 = 0x000001;
        /// aacSpectralDataResilienceFlag flag (from ASC): 1 means use huffman codeword reordering
        const ER_RVLC = 0x000002;
        /// aacSectionDataResilienceFlag flag (from ASC): 1 means use virtual codebooks
        const ER_HCR = 0x000004;
        /// AAC Scalable
        const SCALABLE = 0x000008;
        /// AAC-ELD
        const ELD = 0x000010;
        /// AAC-LD
        const LD = 0x000020;
        /// ER syntax
        const ER = 0x000040;
        /// USAC
        const USAC = 0x000100;
        /// USAC may have preroll.
        const USAC_HAS_PREROLL = 0x000200;
        /// Harmonic SBR present flag.
        const HBE_PRESENT = 0x000400;
        /// SBR present flag (from ASC or implicit)
        const SBR_PRESENT = 0x008000;
        /// SBR CRC present flag. Only relevant for AAC-ELD for now.
        const SBRCRC = 0x010000;
        /// PS present flag (from ASC or implicit)
        const PS_PRESENT = 0x020000;
        /// MPS present flag (from ASC or implicit)
        const MPS_PRESENT = 0x040000;
        /// Independency flag
        const INDEP = 0x100000;
        /// PCE present in current raw_data_block().
        const PCE = 0x200000;
        /// ELD Downscaled playout
        const ELD_DOWNSCALE = 0x1000000;
        /// Dynamic Range Control (DRC) data found.
        const DRC_PRESENT = 0x4000000;
        /// Frame length flag (from gaSpecificConfig or eldSpecificConfig)
        const FRAME_LENGTH = 0x8000000;
        /// MPEG-4 SBR Enhancements
        const MPEG4_ESBR = 0x10000000;
        /// USAC flag: If stereoConfigIndex is 1 the flag is set.
        const USAC_SCFGI1 = 0x20000000;
        /// USAC flag: If stereoConfigIndex is 2 the flag is set.
        const USAC_SCFGI2 = 0x40000000;
        /// USAC flag: If stereoConfigIndex is 3 the flag is set.
        const USAC_SCFGI3 = 0x80000000;
    }

    /// Element specific flags
    #[derive(Copy, Clone, Debug, Default, PartialEq)]
    #[repr(C)]
    pub struct ChannelFlags: u32 {
        /// GA AAC coupling channel element (CCE)
        const GA_CCE = 0x000001;
        /// USAC noise filling is active
        const USAC_NOISE = 0x000002;
        /// USAC SBR inter-TES tool is active.
        const USAC_ITES = 0x000004;
        /// USAC SBR predictive vector coding tool is active.
        const USAC_PVC = 0x000008;
        /// USAC MPS212 tool is active.
        const USAC_MPS212 = 0x000010;
        /// USAC may use Complex Stereo Prediction
        const USAC_CP_POSSIBLE = 0x000040;
        /// USAC harmonic SBR tools are active.
        const USAC_HBE = 0x000080;
        /// GA AAC may use Parametric Stereo in this channel element */
        const PS_POSSIBLE = 0x000100;
        /// Channel element is LFE
        const LFE = 0x002000;
    }

    #[derive(Copy, Clone, Debug, Default, PartialEq)]
    #[repr(C)]
    pub struct AACDecFlags: u32 {
        /// Flag for aacDecoder_DecodeFrame(): Trigger the built-in
        /// error concealment module to generate a substitute signal
        /// for one lost frame. New input data will not be considered.
        const CONCEAL= 1;

        /// Flag for aacDecoder_DecodeFrame(): Flush all filterbanks to
        /// get all delayed audio without having new input data.
        /// Thus new input data will not be considered.
        const FLUSH = 2;
    }
}
