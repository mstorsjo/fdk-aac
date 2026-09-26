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
//! RVLC constants

use crate::aac_dec::constants as aac_dec_const;

pub(crate) const RVLC_MAX_SFB: usize = aac_dec_const::MAX_WINS_X_SFBS;

// Offset for correcting scf value.
pub(crate) const SF_OFFSET: i16 = 100;
// Empirical value.
pub(crate) const CONCEAL_MAX_INIT: i16 = 1311;
// Empirical value.
pub(crate) const CONCEAL_MIN_INIT: i16 = -1311;
// Max length of huffman coded RVLC escape word in bits.
pub(crate) const MAX_LEN_RVLC_ESCAPE_WORD: i16 = 20;
// Max length of a RVL codeword in bits.
pub(crate) const MAX_LEN_RVLC_CODE_WORD: i16 = 9;
// Bitstream decoding direction forward (RVL coded part).
pub(crate) const FWD: u8 = 0;
// Bitstream decoding direction backward (RVL coded part).
pub(crate) const BWD: u8 = 1;
// Max allowed index of a decoded dpcm value (offset 'TABLE_OFFSET' incl --> must be subtracted).
pub(crate) const MAX_ALLOWED_DPCM_INDEX: i16 = 14;

// dpcm offset of valid output values of rvl table decoding.
// The rvl table only returns positive values, therefore the offset.
pub(crate) const TABLE_OFFSET: i16 = 7;
// Positive RVLC escape.
pub(crate) const MAX_RVL: i16 = 7;
// Negative RVLC escape.
pub(crate) const MIN_RVL: i16 = -7;

// -------------------------------------------------------------------
//  error_log_rvlc: A word of 32 bits used for logging possible errors
//                  within RVLC in case of distorted bitstreams.
// -------------------------------------------------------------------
// ESC-Dec  During RVLC-Escape decoding, more bits than available were decoded.
pub(crate) const RVLC_ERROR_ALL_ESCAPE_WORDS_INVALID: u32 = 0x80000000;
// RVL-Dec  Negative sum-bitcounter during RVL-fwd-decoding (long+short).
pub(crate) const RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_FWD: u32 = 0x40000000;
// RVL-Dec  Negative sum-bitcounter during RVL-fwd-decoding long+short).
pub(crate) const RVLC_ERROR_RVL_SUM_BIT_COUNTER_BELOW_ZERO_BWD: u32 = 0x20000000;
// RVL-Dec  Forbidden codeword detected fwd (long+short).
pub(crate) const RVLC_ERROR_FORBIDDEN_CW_DETECTED_FWD: u32 = 0x08000000;
// RVL-Dec  Forbidden codeword detected bwd (long+short).
pub(crate) const RVLC_ERROR_FORBIDDEN_CW_DETECTED_BWD: u32 = 0x04000000;

/// The table contains the decode tree for the RVLC escape sequences.
//  Bit 23 and 11 not used, bit 22 and 10 determine end value.
//  -->  if set codeword is decoded, bit 21-12 and 9-0 (offset to next node)
//  or (index value).
//  The escape sequence is the index value.
//
//   Input:          codeword
//   Output:         index
#[rustfmt::skip]
pub(crate) static HUFF_TREE_RVLC_ESCAPE: [u32; 53] = [
    0x002001, 0x400003, 0x401004, 0x402005, 0x403007, 0x404006, 0x00a405,
    0x009008, 0x00b406, 0x00c407, 0x00d408, 0x00e409, 0x40b40a, 0x40c00f,
    0x40d010, 0x40e011, 0x40f012, 0x410013, 0x411014, 0x412015, 0x016413,
    0x414415, 0x017416, 0x417018, 0x419019, 0x01a418, 0x01b41a, 0x01c023,
    0x03201d, 0x01e020, 0x43501f, 0x41b41c, 0x021022, 0x41d41e, 0x41f420,
    0x02402b, 0x025028, 0x026027, 0x421422, 0x423424, 0x02902a, 0x425426,
    0x427428, 0x02c02f, 0x02d02e, 0x42942a, 0x42b42c, 0x030031, 0x42d42e,
    0x42f430, 0x033034, 0x431432, 0x433434];

///  The table contains the huffman decoding tree for the RVLC scale factors.
///  The table contains 15 allowed, symmetric codewords and 8 forbidden codewords,
///  which are used for error detection.
//   Usage of bits:  bit 23 and 11: not used
//                   bit 22 and 10 determine end value  -->  if set codeword is
//                   decoded bit 21-12 and 9-0 (offset to next node within the table) or (index+7).
//                   The decoded (index+7) is in the range from 0,1,..,22.
//                   If the (index+7) is in the range 15,16,..,22,
//                   then a forbidden codeword is decoded.
//
//   Input:          A single bit from an RVLC scalefactor codeword.
//   Output:         [if codeword is not completely decoded:] offset to next node within table or
//                   [if codeword is decoded:] A dpcm value i.e. (index+7) in range from 0,1,..,22.
//                   The differential scalefactor (DPCM value) named 'index' is calculated by
//                   subtracting 7 from the decoded value (index+7).
pub(crate) static HUFF_TREE_RVL_CODEWDS: [u32; 22] = [
    0x407001, 0x002009, 0x003406, 0x004405, 0x005404, 0x006403, 0x007400, 0x008402, 0x411401,
    0x00a408, 0x00c00b, 0x00e409, 0x01000d, 0x40f40a, 0x41400f, 0x01340b, 0x011015, 0x410012,
    0x41240c, 0x416014, 0x41540d, 0x41340e,
];
