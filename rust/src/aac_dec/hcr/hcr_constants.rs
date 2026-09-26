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
//! Constants used by the HCR module

use crate::aac_dec::constants;

pub(super) const LINES_PER_UNIT: usize  = 4;

// ----------- basic HCR configuration ------------
// (MAX_WINDOWS * MAX_SFB_SHORT) is not enough because sfbs are split in units for blocktype short
pub const MAX_SFB_HCR: usize =
    ((constants::MAX_FRAMESIZE / constants::MAX_WINDOWS) / LINES_PER_UNIT) * constants::MAX_WINDOWS;
// -----------------------------------------------

pub(super) const FROM_LEFT_TO_RIGHT: u8 = 0;
pub(super) const FROM_RIGHT_TO_LEFT: u8 = 1;

pub(super) const MAX_HCR_SETS: usize    = 14;
pub(super) const MAX_CB_PAIRS: usize    = 23;

pub(super) const MAX_CB: usize     = 32; // last used CB is cb #31 when VCB11 is used
pub(super) const MAX_CB_CHECK: u8  = 32; // support for VCB11 available -- is more general, could therefore used in both cases

pub(super) const ESCAPE_VALUE: u16      = 16;

pub const NUMBER_OF_BIT_IN_WORD: u32 = 32;

// log
pub(super) const THIRTYTWO_LOG_DIV_TWO_LOG: u32 = 5;
pub(super) const _EIGHT_LOG_DIV_TWO_LOG:    u32 = 3;
pub(super) const FOUR_LOG_DIV_TWO_LOG:      u32 = 2;

// borders
pub(super) const CPE_TOP_LENGTH:              u16 = 12288;
pub(super) const SCE_TOP_LENGTH:              u16 = 6144;
pub(super) const LEN_OF_LONGEST_CW_TOP_LENGTH: u8 = 49;

// qsc's of high level
pub(super) const Q_VALUE_INVALID: i16 = 8192; // mark a invalid line with this value (to be concealed later on)
pub(super) const _HCR_DIRAC: u16      = 500;  // a line of high level

pub(super) const HCR_FATAL_PCW_ERROR_MASK: u32 = 0x100E01FC;

// --------------------------------------------------
// -             insert HCR errors                  -
// --------------------------------------------------

// modify input lengths -- high protected
pub(super) const ERROR_LORSD: u16 = 0; // offset: error if different from zero
pub(super) const ERROR_LOLC:  u16 = 0; // offset: error if different from zero

// segments are earlier empty as expected when decoding PCWs
pub(super) const ERROR_PCW_BODY: u8            = 0; // set a positive value to trigger the error (make segments earlier appear to be empty)
pub(super) const ERROR_PCW_BODY_SIGN: u8       = 0; // set a positive value to trigger the error (make segments earlier appear to be empty)
pub(super) const ERROR_PCW_BODY_SIGN_ESC: u8   = 0; // set a positive value to trigger the error (make segments earlier appear to be empty)

// pretend there are too many bits decoded (enlarge length of codeword) at PCWs
// -- use a positive value
pub(super) const ERROR_PCW_BODY_ONLY_TOO_LONG:     u8 = 0; // set a positive value to trigger the error
pub(super) const ERROR_PCW_BODY_SIGN_TOO_LONG:     u8 = 0; // set a positive value to trigger the error
pub(super) const ERROR_PCW_BODY_SIGN_ESC_TOO_LONG: u8 = 0; // set a positive value to trigger the error


// --------------------------------------------------
// -               conceal HCR errors               -
// --------------------------------------------------

// -----------------------------------------------------------------------------------------------------------------------------------------
//      errorLog: A word of 32 bits used for logging possible errors within HCR
//                in case of distorted bitstreams. Table of all known errors:
// -----------------------------------------------------------------------------------------------------------------------------------------
//                                                                                 bit  fatal  location    meaning
// -------------------------------------------------------------------------------------+-----+-----------+-----------------------------------------
pub(super) const SEGMENT_OVERRIDE_ERR_PCW_BODY:            u32 = 0x80000000;    //  31   no    PCW-Dec      During PCW decoding it is checked after
                                                                                //                          every PCW if there are too many bits decoded
                                                                                //                          (immediate check).
pub(super) const SEGMENT_OVERRIDE_ERR_PCW_BODY_SIGN:       u32 = 0x40000000;    //  30   no    PCW-Dec      During PCW decoding it is checked after
                                                                                //                          every PCW if there are too many bits decoded
                                                                                //                          (immediate check).
pub(super) const SEGMENT_OVERRIDE_ERR_PCW_BODY_SIGN_ESC:   u32 = 0x20000000;    //  29   no    PCW-Dec      During PCW decoding it is checked after
                                                                                //                          every PCW if there are too many bits decoded
                                                                                //                          (immediate check).
pub(super) const EXTENDED_SORTED_COUNTER_OVERFLOW:         u32 = 0x10000000;    //  28   yes   Init-Dec     Error during extending sideinfo
                                                                                //                          (neither a PCW nor a nonPCW was decoded so far)
//                                                               0x08000000     //  27                      reserved
//                                                               0x04000000     //  26                      reserved
//                                                               0x02000000     //  25                      reserved
//                                                               0x01000000     //  24                      reserved
//                                                               0x00800000     //  23                      reserved
//                                                               0x00400000     //  22                      reserved
//                                                               0x00200000     //  21                      reserved
//                                                               0x00100000     //  20                      reserved

// special errors
pub(super) const TOO_MANY_PCW_BODY_BITS_DECODED:           u32 = 0x00080000;    //  19   yes   PCW-Dec      During PCW-body-decoding too many bits
                                                                                //                          have been read from bitstream
                                                                                //                          advice: skip non-PCW decoding
pub(super) const TOO_MANY_PCW_BODY_SIGN_BITS_DECODED:      u32 = 0x00040000;    //  18   yes   PCW-Dec      During PCW-body-sign-decoding too many
                                                                                //                          bits have been read from bitstream
                                                                                //                          advice: skip non-PCW decoding
pub(super) const TOO_MANY_PCW_BODY_SIGN_ESC_BITS_DECODED:  u32 = 0x00020000;    //  17   yes   PCW-Dec      During PCW-body-sign-esc-decoding too
                                                                                //                          many bits have been read from bitstream
                                                                                //                          advice: skip non-PCW decoding
//                                                               0x00010000     //  16                      reserved
pub(super) const STATE_ERROR_BODY_ONLY:                    u32 = 0x00008000;    //  15   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_BODY:               u32 = 0x00004000;    //  14   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_SIGN:               u32 = 0x00002000;    //  13   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_ESC_BODY:           u32 = 0x00001000;    //  12   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_ESC_SIGN:           u32 = 0x00000800;    //  11   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_ESC_ESC_PREFIX:     u32 = 0x00000400;    //  10   no    NonPCW-Dec   State machine returned with error
pub(super) const STATE_ERROR_BODY_SIGN_ESC_ESC_WORD:       u32 = 0x00000200;    //   9   no    NonPCW-Dec   State machine returned with error
pub(super) const HCR_SI_LENGTHS_FAILURE:                   u32 = 0x00000100;    //   8   yes   Init-Dec     LengthOfLongestCodeword must not be less than
                                                                                //                          lenght_Of_reordered_spectral_data
pub(super) const NUM_SECT_OUT_OF_RANGE_SHORT_BLOCK:        u32 = 0x00000080;    //   7   yes   Init-Dec     The number of sections is not within
                                                                                //                          the allowed range (short block)
pub(super) const NUM_SECT_OUT_OF_RANGE_LONG_BLOCK:         u32 = 0x00000040;    //   6   yes   Init-Dec     The number of sections is not within
                                                                                //                          the allowed range (long block)
pub(super) const LINE_IN_SECT_OUT_OF_RANGE_SHORT_BLOCK:    u32 = 0x00000020;    //   5   yes   Init-Dec     The number of lines per section is not
                                                                                //                          within the allowed range (short block)
pub(super) const CB_OUT_OF_RANGE_SHORT_BLOCK:              u32 = 0x00000010;    //   4   yes   Init-Dec     The codebook is not within the allowed
                                                                                //                          range (short block)
pub(super) const LINE_IN_SECT_OUT_OF_RANGE_LONG_BLOCK:     u32 = 0x00000008;    //   3   yes   Init-Dec     The number of lines per section is not
                                                                                //                          within the allowed range (long block)
pub(super) const CB_OUT_OF_RANGE_LONG_BLOCK:               u32 = 0x00000004;    //   2   yes   Init-Dec     The codebook is not within the allowed
                                                                                //                          range (long block)
pub(super) const LAV_VIOLATION:                            u32 = 0x00000002;    //   1   no    Final        The absolute value of at least one
                                                                                //                          decoded line was too high for the according codebook.
pub(super) const BIT_IN_SEGMENTATION_ERROR:                u32 = 0x00000001;    //   0   no    Final        After PCW and non-PWC-decoding at least
                                                                                //                          one segment is not zero (global check).
// -----------------------------------------------------------------------------------------------------------------------------------------

#[repr(C)]
#[derive(Debug)]
pub(super) enum PcwType { Body, BodySign, BodySignEsc,}

// ----------------------------------------------------------------------------------------------------------
//                          the states of the state machine
// ----------------------------------------------------------------------------------------------------------

/// The four different kinds of types of states are:
///
/// examples:
///
/// BODY_ONLY                Means that only the codeword body will be decoded.
///                          No sign bits will follow and no escape sequence will follow.
///
/// BODY_SIGN__BODY          Means that the codeword consists of two parts: body and sign part.
///                          The part '__BODY' after the two underscores shows that
///                          the bits which are currently decoded belong to
///                          the '__BODY' of the codeword and not to the sign part.
///
/// BODY_SIGN_ESC__ESC_PB    Means that the codeword consists of three parts:
///                             body, sign and (here: two) escape sequences;
///                                 P = Prefix = ones
///                                 W = Escape Word
///                                 A = first possible (of two) Escape sequeces
///                                 B = second possible (of two) Escape sequeces
///                          The part after the two underscores shows that
///                          the current bits which are decoded belong to
///                          the '__ESC_PB' - part of the codeword.
///                          That means the body and the sign bits are decoded completely and
///                          the bits which are decoded now belong to
///                          the escape sequence [P = prefix; B=second possible escape sequence]
///
/// different states are defined as constants        // start  middle=self  next  stop
pub(super) const STOP_THIS_STATE:           i8 = 0;  //
pub(super) const BODY_ONLY:                 i8 = 1;  //   X        X                X
pub(super) const BODY_SIGN__BODY:           i8 = 2;  //   X        X         X      X [stop if no sign]
pub(super) const BODY_SIGN__SIGN:           i8 = 3;  //            X                X [stop if sign bits decoded]
pub(super) const BODY_SIGN_ESC__BODY:       i8 = 4;  //   X        X         X      X [stop if no sign]
pub(super) const BODY_SIGN_ESC__SIGN:       i8 = 5;  //            X         X      X [stop if no escape sequence]
pub(super) const BODY_SIGN_ESC__ESC_PREFIX: i8 = 6;  //            X         X
pub(super) const BODY_SIGN_ESC__ESC_WORD:   i8 = 7;  //            X         X      X [stop if abs(second qsc) != 16]//
//                                                   //-------------------------------------------------------------

pub(super) const ESCAPE_CODEBOOK: usize = 11;
pub(super) const DIMENSION_OF_ESCAPE_CODEBOOK: usize = 2; // for cb >= 11 is dimension 2

pub(super) const MASK_ESCAPE_PREFIX_UP:   u32 = 0x000F0000;
pub(super) const MASK_ESCAPE_PREFIX_DOWN: u32 = 0x0000F000;
pub(super) const LSB_ESCAPE_PREFIX_UP:    u32 = 16;
pub(super) const LSB_ESCAPE_PREFIX_DOWN:  u32 = 12;

pub(super) const MASK_ESCAPE_WORD: u32 = 0x00000FFF;
pub(super) const MASK_FLAG_A:      u32 = 0x00200000;
pub(super) const MASK_FLAG_B:      u32 = 0x00100000;

// --------------------------------------------------------------------------------
// --- arrays for HCR_TABLE_INFO structures
//
// priorities of codebooks
#[rustfmt::skip]
pub(super) static CB_PRIORITY: [u8; MAX_CB] =  [0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,
                                                22, 0,  0,  0,  0,  6,  7,  8,  9,  10, 11,
                                                12, 13, 14, 15, 16, 17, 18, 19, 20, 21];

pub(super) static CODEBOOK_2_START_INT: [i8; 32] =
[
    STOP_THIS_STATE,      // cb  0
    BODY_ONLY,            // cb  1
    BODY_ONLY,            // cb  2
    BODY_SIGN__BODY,      // cb  3
    BODY_SIGN__BODY,      // cb  4
    BODY_ONLY,            // cb  5
    BODY_ONLY,            // cb  6
    BODY_SIGN__BODY,      // cb  7
    BODY_SIGN__BODY,      // cb  8
    BODY_SIGN__BODY,      // cb  9
    BODY_SIGN__BODY,      // cb 10
    BODY_SIGN_ESC__BODY,  // cb 11
    STOP_THIS_STATE,      // cb 12
    STOP_THIS_STATE,      // cb 13
    STOP_THIS_STATE,      // cb 14
    STOP_THIS_STATE,      // cb 15
    BODY_SIGN_ESC__BODY,  // cb 16
    BODY_SIGN_ESC__BODY,  // cb 17
    BODY_SIGN_ESC__BODY,  // cb 18
    BODY_SIGN_ESC__BODY,  // cb 19
    BODY_SIGN_ESC__BODY,  // cb 20
    BODY_SIGN_ESC__BODY,  // cb 21
    BODY_SIGN_ESC__BODY,  // cb 22
    BODY_SIGN_ESC__BODY,  // cb 23
    BODY_SIGN_ESC__BODY,  // cb 24
    BODY_SIGN_ESC__BODY,  // cb 25
    BODY_SIGN_ESC__BODY,  // cb 26
    BODY_SIGN_ESC__BODY,  // cb 27
    BODY_SIGN_ESC__BODY,  // cb 28
    BODY_SIGN_ESC__BODY,  // cb 29
    BODY_SIGN_ESC__BODY,  // cb 30
    BODY_SIGN_ESC__BODY,  // cb 31
];

#[rustfmt::skip]
//                                     CB:  0 1 2 3 4 5 6 7 8  9 10      12
// 14    16    18    20      22      24      26      28       30
pub(super) static LARGEST_ABSOLUTE_VALUE: [u16; MAX_CB] = [
                                0, 1, 1, 2, 2, 4, 4, 7, 7, 12, 12, 8191,
                                0, 0, 0, 0, 15, 31, 47, 63, 95, 127, 159,
                                191, 223, 255, 319, 383, 511, 767, 1023, 2047,
]; // lav
//                                     CB:                           11     13
// 15    17    19     21      23      25      27      39       31

#[rustfmt::skip]
/// maximum length of codeword in each codebook
/// codebook:                     0,1, 2,3, 4, 5, 6, 7, 8, 9,
/// 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
pub(super) static MAX_CW_LEN: [u8; MAX_CB] = [
                                0,  11, 9,  20, 16, 13, 11, 14, 12, 17, 14,
                                49, 0,  0,  0,  0,  14, 17, 21, 21, 25, 25,
                                29, 29, 29, 29, 33, 33, 33, 37, 37, 41
];

#[rustfmt::skip]
//                                                          11  13  15  17  19
// 21  23  25  27  39  31
//                           CB:  0 1 2 3 4 5 6 7 8 9 10  12  14  16  18  20
// 22  24  26  28  30
pub(super) static DIM_CB: [u8; MAX_CB] = [
    2, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]; // codebook dimension -
                                                  // zero cb got a
                                                  // dimension of 2

#[rustfmt::skip]
//                                                           11  13  15  17  19
// 21  23  25  27  39  31
//                            CB:  0 1 2 3 4 5 6 7 8 9 10  12  14  16  18  20
// 22  24  26  28  30
pub(super) static DIM_CB_SHIFT: [u8; MAX_CB] = [
                                    1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
]; // codebook dimension

#[rustfmt::skip]
//               1 -> decode sign bits
//               0 -> decode no sign bits                11  13  15  17  19  21
// 23  25  27  39  31
//                        CB:  0 1 2 3 4 5 6 7 8 9 10  12  14  16  18  20  22
// 24  26  28  30
pub(super) static  SIGN_CB: [u8; MAX_CB] = [
                                0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
                                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
];

// arrays for HCR_CB_PAIRS structures
#[rustfmt::skip]
pub(super) static  MIN_OF_CB_PAIR: [u8; MAX_CB_PAIRS] = [
                                        0,  1,  3,  5,  7,  9,  16, 17,
                                        18, 19, 20, 21, 22, 23, 24, 25,
                                        26, 27, 28, 29, 30, 31, 11
];

#[rustfmt::skip]
pub(super) static  MAX_OF_CB_PAIR: [u8; MAX_CB_PAIRS] = [
                                        0,  2,  4,  6,  8,  10, 16, 17,
                                        18, 19, 20, 21, 22, 23, 24, 25,
                                        26, 27, 28, 29, 30, 31, 11
];

// --------------------------------------------------------------------------------
// The following tables contain the quantized values. Two or four of the
// quantized values are indexed by the result of the decoding in the decoding tree

#[rustfmt::skip]
///  This table contains the quantized values for codebooks 1-2.
pub(super) static  VAL_TAB_41: [i8; 324] = [
    -1, -1, -1, -1, -1, -1, -1, 0,  -1, -1, -1, 1,  -1, -1, 0,  -1, -1, -1,
    0,  0,  -1, -1, 0,  1,  -1, -1, 1,  -1, -1, -1, 1,  0,  -1, -1, 1,  1,
    -1, 0,  -1, -1, -1, 0,  -1, 0,  -1, 0,  -1, 1,  -1, 0,  0,  -1, -1, 0,
    0,  0,  -1, 0,  0,  1,  -1, 0,  1,  -1, -1, 0,  1,  0,  -1, 0,  1,  1,
    -1, 1,  -1, -1, -1, 1,  -1, 0,  -1, 1,  -1, 1,  -1, 1,  0,  -1, -1, 1,
    0,  0,  -1, 1,  0,  1,  -1, 1,  1,  -1, -1, 1,  1,  0,  -1, 1,  1,  1,
    0,  -1, -1, -1, 0,  -1, -1, 0,  0,  -1, -1, 1,  0,  -1, 0,  -1, 0,  -1,
    0,  0,  0,  -1, 0,  1,  0,  -1, 1,  -1, 0,  -1, 1,  0,  0,  -1, 1,  1,
    0,  0,  -1, -1, 0,  0,  -1, 0,  0,  0,  -1, 1,  0,  0,  0,  -1, 0,  0,
    0,  0,  0,  0,  0,  1,  0,  0,  1,  -1, 0,  0,  1,  0,  0,  0,  1,  1,
    0,  1,  -1, -1, 0,  1,  -1, 0,  0,  1,  -1, 1,  0,  1,  0,  -1, 0,  1,
    0,  0,  0,  1,  0,  1,  0,  1,  1,  -1, 0,  1,  1,  0,  0,  1,  1,  1,
    1,  -1, -1, -1, 1,  -1, -1, 0,  1,  -1, -1, 1,  1,  -1, 0,  -1, 1,  -1,
    0,  0,  1,  -1, 0,  1,  1,  -1, 1,  -1, 1,  -1, 1,  0,  1,  -1, 1,  1,
    1,  0,  -1, -1, 1,  0,  -1, 0,  1,  0,  -1, 1,  1,  0,  0,  -1, 1,  0,
    0,  0,  1,  0,  0,  1,  1,  0,  1,  -1, 1,  0,  1,  0,  1,  0,  1,  1,
    1,  1,  -1, -1, 1,  1,  -1, 0,  1,  1,  -1, 1,  1,  1,  0,  -1, 1,  1,
    0,  0,  1,  1,  0,  1,  1,  1,  1,  -1, 1,  1,  1,  0,  1,  1,  1,  1];

#[rustfmt::skip]
/// This table contains the quantized values for codebooks 3-4.
pub(super) static VAL_TAB_42: [i8; 324] = [
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 2, 0,
    0, 2, 0, 0, 0, 2, 1, 0, 0, 2, 2, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 2, 0, 1,
    1, 0, 0, 1, 1, 1, 0, 1, 1, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 2, 0,
    0, 0, 2, 0, 1, 0, 2, 0, 2, 0, 2, 1, 0, 0, 2, 1, 1, 0, 2, 1, 2, 0, 2, 2, 0,
    0, 2, 2, 1, 0, 2, 2, 2, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 2, 1, 0, 1, 0, 1,
    0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 0, 1, 0, 2, 1, 1, 0, 2, 2, 1, 1, 0, 0, 1, 1,
    0, 1, 1, 1, 0, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 0, 1, 1, 2,
    1, 1, 1, 2, 2, 1, 2, 0, 0, 1, 2, 0, 1, 1, 2, 0, 2, 1, 2, 1, 0, 1, 2, 1, 1,
    1, 2, 1, 2, 1, 2, 2, 0, 1, 2, 2, 1, 1, 2, 2, 2, 2, 0, 0, 0, 2, 0, 0, 1, 2,
    0, 0, 2, 2, 0, 1, 0, 2, 0, 1, 1, 2, 0, 1, 2, 2, 0, 2, 0, 2, 0, 2, 1, 2, 0,
    2, 2, 2, 1, 0, 0, 2, 1, 0, 1, 2, 1, 0, 2, 2, 1, 1, 0, 2, 1, 1, 1, 2, 1, 1,
    2, 2, 1, 2, 0, 2, 1, 2, 1, 2, 1, 2, 2, 2, 2, 0, 0, 2, 2, 0, 1, 2, 2, 0, 2,
    2, 2, 1, 0, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 2, 0, 2, 2, 2, 1, 2, 2, 2, 2];

#[rustfmt::skip]
/// This table contains the quantized values for codebooks 5-6.
pub(super) static VAL_TAB_21: [i8; 162] = [
    -4, -4, -4, -3, -4, -2, -4, -1, -4, 0, -4, 1, -4, 2, -4, 3, -4, 4,
    -3, -4, -3, -3, -3, -2, -3, -1, -3, 0, -3, 1, -3, 2, -3, 3, -3, 4,
    -2, -4, -2, -3, -2, -2, -2, -1, -2, 0, -2, 1, -2, 2, -2, 3, -2, 4,
    -1, -4, -1, -3, -1, -2, -1, -1, -1, 0, -1, 1, -1, 2, -1, 3, -1, 4,
    0,  -4, 0,  -3, 0,  -2, 0,  -1, 0,  0, 0,  1, 0,  2, 0,  3, 0,  4,
    1,  -4, 1,  -3, 1,  -2, 1,  -1, 1,  0, 1,  1, 1,  2, 1,  3, 1,  4,
    2,  -4, 2,  -3, 2,  -2, 2,  -1, 2,  0, 2,  1, 2,  2, 2,  3, 2,  4,
    3,  -4, 3,  -3, 3,  -2, 3,  -1, 3,  0, 3,  1, 3,  2, 3,  3, 3,  4,
    4,  -4, 4,  -3, 4,  -2, 4,  -1, 4,  0, 4,  1, 4,  2, 4,  3, 4,  4];

#[rustfmt::skip]
/// This table contains the quantized values for codebooks 7-8.
pub(super) static VAL_TAB_22: [i8; 128] = [
    0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 1, 0, 1, 1, 1, 2,
    1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5,
    2, 6, 2, 7, 3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 4, 0,
    4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 5, 0, 5, 1, 5, 2, 5, 3,
    5, 4, 5, 5, 5, 6, 5, 7, 6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6,
    6, 7, 7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7];

#[rustfmt::skip]
/// This table contains the quantized values for codebooks 9-10.
pub(super) static VAL_TAB_23: [i8; 338] = [
    0,  0,  0,  1,  0,  2,  0,  3,  0,  4,  0,  5,  0,  6,  0,  7,  0,  8,  0,
    9,  0,  10, 0,  11, 0,  12, 1,  0,  1,  1,  1,  2,  1,  3,  1,  4,  1,  5,
    1,  6,  1,  7,  1,  8,  1,  9,  1,  10, 1,  11, 1,  12, 2,  0,  2,  1,  2,
    2,  2,  3,  2,  4,  2,  5,  2,  6,  2,  7,  2,  8,  2,  9,  2,  10, 2,  11,
    2,  12, 3,  0,  3,  1,  3,  2,  3,  3,  3,  4,  3,  5,  3,  6,  3,  7,  3,
    8,  3,  9,  3,  10, 3,  11, 3,  12, 4,  0,  4,  1,  4,  2,  4,  3,  4,  4,
    4,  5,  4,  6,  4,  7,  4,  8,  4,  9,  4,  10, 4,  11, 4,  12, 5,  0,  5,
    1,  5,  2,  5,  3,  5,  4,  5,  5,  5,  6,  5,  7,  5,  8,  5,  9,  5,  10,
    5,  11, 5,  12, 6,  0,  6,  1,  6,  2,  6,  3,  6,  4,  6,  5,  6,  6,  6,
    7,  6,  8,  6,  9,  6,  10, 6,  11, 6,  12, 7,  0,  7,  1,  7,  2,  7,  3,
    7,  4,  7,  5,  7,  6,  7,  7,  7,  8,  7,  9,  7,  10, 7,  11, 7,  12, 8,
    0,  8,  1,  8,  2,  8,  3,  8,  4,  8,  5,  8,  6,  8,  7,  8,  8,  8,  9,
    8,  10, 8,  11, 8,  12, 9,  0,  9,  1,  9,  2,  9,  3,  9,  4,  9,  5,  9,
    6,  9,  7,  9,  8,  9,  9,  9,  10, 9,  11, 9,  12, 10, 0,  10, 1,  10, 2,
    10, 3,  10, 4,  10, 5,  10, 6,  10, 7,  10, 8,  10, 9,  10, 10, 10, 11, 10,
    12, 11, 0,  11, 1,  11, 2,  11, 3,  11, 4,  11, 5,  11, 6,  11, 7,  11, 8,
    11, 9,  11, 10, 11, 11, 11, 12, 12, 0,  12, 1,  12, 2,  12, 3,  12, 4,  12,
    5,  12, 6,  12, 7,  12, 8,  12, 9,  12, 10, 12, 11, 12, 12];

#[rustfmt::skip]
/// This table contains the quantized values for codebooks 11.
pub(super) static VAL_TAB_24: [i8; 578] = [
    0,  0,  0,  1,  0,  2,  0,  3,  0,  4,  0,  5,  0,  6,  0,  7,  0,  8,  0,
    9,  0,  10, 0,  11, 0,  12, 0,  13, 0,  14, 0,  15, 0,  16, 1,  0,  1,  1,
    1,  2,  1,  3,  1,  4,  1,  5,  1,  6,  1,  7,  1,  8,  1,  9,  1,  10, 1,
    11, 1,  12, 1,  13, 1,  14, 1,  15, 1,  16, 2,  0,  2,  1,  2,  2,  2,  3,
    2,  4,  2,  5,  2,  6,  2,  7,  2,  8,  2,  9,  2,  10, 2,  11, 2,  12, 2,
    13, 2,  14, 2,  15, 2,  16, 3,  0,  3,  1,  3,  2,  3,  3,  3,  4,  3,  5,
    3,  6,  3,  7,  3,  8,  3,  9,  3,  10, 3,  11, 3,  12, 3,  13, 3,  14, 3,
    15, 3,  16, 4,  0,  4,  1,  4,  2,  4,  3,  4,  4,  4,  5,  4,  6,  4,  7,
    4,  8,  4,  9,  4,  10, 4,  11, 4,  12, 4,  13, 4,  14, 4,  15, 4,  16, 5,
    0,  5,  1,  5,  2,  5,  3,  5,  4,  5,  5,  5,  6,  5,  7,  5,  8,  5,  9,
    5,  10, 5,  11, 5,  12, 5,  13, 5,  14, 5,  15, 5,  16, 6,  0,  6,  1,  6,
    2,  6,  3,  6,  4,  6,  5,  6,  6,  6,  7,  6,  8,  6,  9,  6,  10, 6,  11,
    6,  12, 6,  13, 6,  14, 6,  15, 6,  16, 7,  0,  7,  1,  7,  2,  7,  3,  7,
    4,  7,  5,  7,  6,  7,  7,  7,  8,  7,  9,  7,  10, 7,  11, 7,  12, 7,  13,
    7,  14, 7,  15, 7,  16, 8,  0,  8,  1,  8,  2,  8,  3,  8,  4,  8,  5,  8,
    6,  8,  7,  8,  8,  8,  9,  8,  10, 8,  11, 8,  12, 8,  13, 8,  14, 8,  15,
    8,  16, 9,  0,  9,  1,  9,  2,  9,  3,  9,  4,  9,  5,  9,  6,  9,  7,  9,
    8,  9,  9,  9,  10, 9,  11, 9,  12, 9,  13, 9,  14, 9,  15, 9,  16, 10, 0,
    10, 1,  10, 2,  10, 3,  10, 4,  10, 5,  10, 6,  10, 7,  10, 8,  10, 9,  10,
    10, 10, 11, 10, 12, 10, 13, 10, 14, 10, 15, 10, 16, 11, 0,  11, 1,  11, 2,
    11, 3,  11, 4,  11, 5,  11, 6,  11, 7,  11, 8,  11, 9,  11, 10, 11, 11, 11,
    12, 11, 13, 11, 14, 11, 15, 11, 16, 12, 0,  12, 1,  12, 2,  12, 3,  12, 4,
    12, 5,  12, 6,  12, 7,  12, 8,  12, 9,  12, 10, 12, 11, 12, 12, 12, 13, 12,
    14, 12, 15, 12, 16, 13, 0,  13, 1,  13, 2,  13, 3,  13, 4,  13, 5,  13, 6,
    13, 7,  13, 8,  13, 9,  13, 10, 13, 11, 13, 12, 13, 13, 13, 14, 13, 15, 13,
    16, 14, 0,  14, 1,  14, 2,  14, 3,  14, 4,  14, 5,  14, 6,  14, 7,  14, 8,
    14, 9,  14, 10, 14, 11, 14, 12, 14, 13, 14, 14, 14, 15, 14, 16, 15, 0,  15,
    1,  15, 2,  15, 3,  15, 4,  15, 5,  15, 6,  15, 7,  15, 8,  15, 9,  15, 10,
    15, 11, 15, 12, 15, 13, 15, 14, 15, 15, 15, 16, 16, 0,  16, 1,  16, 2,  16,
    3,  16, 4,  16, 5,  16, 6,  16, 7,  16, 8,  16, 9,  16, 10, 16, 11, 16, 12,
    16, 13, 16, 14, 16, 15, 16, 16];

#[rustfmt::skip]
/// cb    quant. val table
pub(super) static QUANT_TABLE: [&[i8]; 32] = [
    &VAL_TAB_41,                   // 0             -  // use quant. val table 1 as dummy here
    &VAL_TAB_41,                   // 1             1
    &VAL_TAB_41,                   // 2             1
    &VAL_TAB_42,                   // 3             2
    &VAL_TAB_42,                   // 4             2
    &VAL_TAB_21,                   // 5             3
    &VAL_TAB_21,                   // 6             3
    &VAL_TAB_22,                   // 7             4
    &VAL_TAB_22,                   // 8             4
    &VAL_TAB_23,                   // 9             5
    &VAL_TAB_23,                   // 10            5
    &VAL_TAB_24,                   // 11            6
    &VAL_TAB_41,                   // 12            -  // use quant. val table 1 as dummy here
    &VAL_TAB_41,                   // 13            -  // use quant. val table 1 as dummy here
    &VAL_TAB_41,                   // 14            -  // use quant. val table 1 as dummy here
    &VAL_TAB_41,                   // 15            -  // use quant. val table 1 as dummy here
    &VAL_TAB_24,                   // 16            6
    &VAL_TAB_24,                   // 17            6
    &VAL_TAB_24,                   // 18            6
    &VAL_TAB_24,                   // 19            6
    &VAL_TAB_24,                   // 20            6
    &VAL_TAB_24,                   // 21            6
    &VAL_TAB_24,                   // 22            6
    &VAL_TAB_24,                   // 23            6
    &VAL_TAB_24,                   // 24            6
    &VAL_TAB_24,                   // 25            6
    &VAL_TAB_24,                   // 26            6
    &VAL_TAB_24,                   // 27            6
    &VAL_TAB_24,                   // 28            6
    &VAL_TAB_24,                   // 29            6
    &VAL_TAB_24,                   // 30            6
    &VAL_TAB_24];                  // 31            6


// --------------------------------------------------------------------------------
// Huffman tree tables
#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 1).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 4)
///
///  input:    codeword
///  output:   index * 4
///
pub(super) static HUFF_TREE41: [u32; 80] = [
    0x4a0001, 0x026002, 0x013003, 0x021004, 0x01c005, 0x00b006, 0x010007,
    0x019008, 0x00900e, 0x00a03a, 0x400528, 0x00c037, 0x00d03b, 0x454404,
    0x00f04c, 0x448408, 0x017011, 0x01202e, 0x42c40c, 0x034014, 0x01502c,
    0x016049, 0x410470, 0x01804e, 0x414424, 0x03201a, 0x02001b, 0x520418,
    0x02f01d, 0x02a01e, 0x01f04d, 0x41c474, 0x540420, 0x022024, 0x04a023,
    0x428510, 0x025029, 0x430508, 0x02703c, 0x028047, 0x50c434, 0x438478,
    0x04802b, 0x46443c, 0x02d03e, 0x4404b0, 0x44451c, 0x03003f, 0x03104b,
    0x52444c, 0x033039, 0x4f0450, 0x035041, 0x036046, 0x4e8458, 0x04f038,
    0x45c53c, 0x4604e0, 0x4f8468, 0x46c4d4, 0x04503d, 0x4ac47c, 0x518480,
    0x043040, 0x4844dc, 0x042044, 0x4884a8, 0x4bc48c, 0x530490, 0x4a4494,
    0x4984b8, 0x49c4c4, 0x5044b4, 0x5004c0, 0x4d04c8, 0x4f44cc, 0x4d8538,
    0x4ec4e4, 0x52c4fc, 0x514534];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 2).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 4)
///
///  input:     codeword
///  output:    index * 4
///
pub(super) static HUFF_TREE42: [u32; 80] = [
    0x026001, 0x014002, 0x009003, 0x010004, 0x01d005, 0x00600d, 0x007018,
    0x450008, 0x4e0400, 0x02e00a, 0x03900b, 0x03d00c, 0x43c404, 0x01b00e,
    0x00f04f, 0x4d8408, 0x023011, 0x01203b, 0x01a013, 0x41440c, 0x015020,
    0x016040, 0x025017, 0x500410, 0x038019, 0x540418, 0x41c444, 0x02d01c,
    0x420520, 0x01e042, 0x03701f, 0x4244cc, 0x02a021, 0x02204c, 0x478428,
    0x024031, 0x42c4dc, 0x4304e8, 0x027033, 0x4a0028, 0x50c029, 0x4344a4,
    0x02c02b, 0x470438, 0x4404c8, 0x4f8448, 0x04902f, 0x04b030, 0x44c484,
    0x524032, 0x4ec454, 0x03e034, 0x035046, 0x4c4036, 0x488458, 0x4d445c,
    0x460468, 0x04e03a, 0x51c464, 0x03c04a, 0x46c514, 0x47453c, 0x04503f,
    0x47c4ac, 0x044041, 0x510480, 0x04304d, 0x4e448c, 0x490518, 0x49449c,
    0x048047, 0x4c0498, 0x4b84a8, 0x4b0508, 0x4fc4b4, 0x4bc504, 0x5304d0,
    0x5344f0, 0x4f452c, 0x528538];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 3).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 4)
///
///  input:    codeword
///  output:   index * 4
///
pub(super) static HUFF_TREE43: [u32; 80] = [
    0x400001, 0x002004, 0x00300a, 0x46c404, 0x00b005, 0x00600d, 0x034007,
    0x037008, 0x494009, 0x4d8408, 0x42440c, 0x00c01b, 0x490410, 0x00e016,
    0x00f011, 0x010014, 0x4144fc, 0x01201d, 0x020013, 0x508418, 0x4c0015,
    0x41c440, 0x022017, 0x018026, 0x019035, 0x03801a, 0x420444, 0x01c01f,
    0x430428, 0x02101e, 0x44842c, 0x478434, 0x4b4438, 0x45443c, 0x02c023,
    0x039024, 0x02503f, 0x48844c, 0x030027, 0x02e028, 0x032029, 0x02a041,
    0x4d402b, 0x4504f0, 0x04302d, 0x4584a8, 0x02f03b, 0x46045c, 0x03103d,
    0x464046, 0x033044, 0x46853c, 0x47049c, 0x045036, 0x4744dc, 0x4a047c,
    0x500480, 0x4ac03a, 0x4b8484, 0x03c04e, 0x48c524, 0x03e040, 0x4984e8,
    0x50c4a4, 0x4b0530, 0x042047, 0x4bc04b, 0x4e44c4, 0x5184c8, 0x52c4cc,
    0x5204d0, 0x04d048, 0x04a049, 0x4e004c, 0x51c4ec, 0x4f4510, 0x5284f8,
    0x50404f, 0x514538, 0x540534];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 4).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 4)
///
///  input:    codeword
///  output:   index * 4
///
pub(super) static HUFF_TREE44: [u32; 80] = [
    0x001004, 0x020002, 0x036003, 0x490400, 0x005008, 0x010006, 0x01f007,
    0x404428, 0x00e009, 0x01100a, 0x00b018, 0x01600c, 0x03700d, 0x408015,
    0x00f03e, 0x40c424, 0x410478, 0x022012, 0x038013, 0x01e014, 0x454414,
    0x448418, 0x025017, 0x47441c, 0x030019, 0x02601a, 0x02d01b, 0x01c034,
    0x01d029, 0x4204f0, 0x4dc42c, 0x470430, 0x02103c, 0x4a0434, 0x02302a,
    0x440024, 0x4384a8, 0x43c44c, 0x02703a, 0x02802c, 0x444524, 0x4504e0,
    0x02b03d, 0x458480, 0x45c4f4, 0x04b02e, 0x04f02f, 0x460520, 0x042031,
    0x048032, 0x049033, 0x514464, 0x03504c, 0x540468, 0x47c46c, 0x4844d8,
    0x039044, 0x4884fc, 0x03b045, 0x48c53c, 0x49449c, 0x4b8498, 0x03f046,
    0x041040, 0x4c44a4, 0x50c4ac, 0x04a043, 0x5184b0, 0x4e44b4, 0x4bc4ec,
    0x04e047, 0x4c04e8, 0x4c8510, 0x4cc52c, 0x4d0530, 0x5044d4, 0x53804d,
    0x5284f8, 0x508500, 0x51c534];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 5).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:    codeword
///  output:   index * 2
///
pub(super) static HUFF_TREE21: [u32; 80] = [
    0x450001, 0x044002, 0x042003, 0x035004, 0x026005, 0x022006, 0x013007,
    0x010008, 0x00d009, 0x01c00a, 0x01f00b, 0x01e00c, 0x4a0400, 0x01b00e,
    0x03200f, 0x47e402, 0x020011, 0x01204d, 0x40449c, 0x017014, 0x015019,
    0x01603f, 0x406458, 0x01804f, 0x448408, 0x04901a, 0x40a45a, 0x48c40c,
    0x01d031, 0x40e48e, 0x490410, 0x492412, 0x021030, 0x480414, 0x033023,
    0x02402e, 0x02503e, 0x416482, 0x02a027, 0x02802c, 0x029040, 0x418468,
    0x02b04a, 0x41a486, 0x02d048, 0x41c484, 0x04e02f, 0x41e426, 0x420434,
    0x42249e, 0x424494, 0x03d034, 0x428470, 0x039036, 0x03703b, 0x038041,
    0x42a476, 0x03a04b, 0x42c454, 0x03c047, 0x42e472, 0x430478, 0x43246e,
    0x496436, 0x488438, 0x43a466, 0x046043, 0x43c464, 0x04504c, 0x43e462,
    0x460440, 0x44245e, 0x45c444, 0x46a446, 0x44a456, 0x47444c, 0x45244e,
    0x46c47c, 0x48a47a, 0x49a498];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 6).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:    codeword
///  output:   index * 2
///
pub(super) static HUFF_TREE22: [u32; 80] = [
    0x03c001, 0x02f002, 0x020003, 0x01c004, 0x00f005, 0x00c006, 0x016007,
    0x04d008, 0x00b009, 0x01500a, 0x400490, 0x40e402, 0x00d013, 0x00e02a,
    0x40c404, 0x019010, 0x011041, 0x038012, 0x40a406, 0x014037, 0x40849c,
    0x4a0410, 0x04a017, 0x458018, 0x412422, 0x02801a, 0x01b029, 0x480414,
    0x02401d, 0x01e02b, 0x48a01f, 0x416432, 0x02d021, 0x026022, 0x023039,
    0x418468, 0x025043, 0x48641a, 0x027040, 0x41c488, 0x41e48c, 0x42045a,
    0x47c424, 0x04c02c, 0x46e426, 0x03602e, 0x428478, 0x030033, 0x43c031,
    0x04b032, 0x42e42a, 0x03403a, 0x035048, 0x42c442, 0x470430, 0x494434,
    0x43649a, 0x45c438, 0x04403b, 0x43a454, 0x04503d, 0x03e03f, 0x43e464,
    0x440460, 0x484444, 0x049042, 0x446448, 0x44a456, 0x46644c, 0x047046,
    0x44e452, 0x450462, 0x47445e, 0x46a496, 0x49846c, 0x472476, 0x47a482,
    0x04e04f, 0x47e492, 0x48e49e];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 7).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:    codeword
///  output:   index * 2
///
pub(super) static HUFF_TREE23: [u32; 63] = [
    0x400001, 0x002003, 0x410402, 0x004007, 0x412005, 0x01c006, 0x420404,
    0x00800b, 0x01d009, 0x00a01f, 0x406026, 0x00c012, 0x00d00f, 0x02700e,
    0x408440, 0x010022, 0x028011, 0x45440a, 0x013017, 0x029014, 0x024015,
    0x01602f, 0x43c40c, 0x02b018, 0x019033, 0x03201a, 0x43e01b, 0x47040e,
    0x422414, 0x01e025, 0x432416, 0x020021, 0x418442, 0x41a452, 0x036023,
    0x41c446, 0x46441e, 0x424430, 0x426434, 0x436428, 0x44442a, 0x02e02a,
    0x45642c, 0x03002c, 0x02d03b, 0x46642e, 0x43a438, 0x460448, 0x031037,
    0x47244a, 0x45a44c, 0x034039, 0x038035, 0x47844e, 0x462450, 0x474458,
    0x46a45c, 0x03a03c, 0x45e47a, 0x476468, 0x03d03e, 0x47c46c, 0x46e47e];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 8).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:          codeword
///  output:         index * 2
///
pub(super) static HUFF_TREE24: [u32; 63] = [
    0x001006, 0x01d002, 0x005003, 0x424004, 0x400420, 0x414402, 0x00700a,
    0x008020, 0x00901f, 0x404432, 0x00b011, 0x00c00e, 0x00d032, 0x406446,
    0x02300f, 0x033010, 0x458408, 0x025012, 0x013016, 0x01402f, 0x015038,
    0x46840a, 0x028017, 0x01801a, 0x039019, 0x40c47a, 0x03e01b, 0x03b01c,
    0x40e47e, 0x41201e, 0x422410, 0x416434, 0x02a021, 0x02202b, 0x418444,
    0x02c024, 0x41a456, 0x02d026, 0x027034, 0x46241c, 0x029036, 0x41e45c,
    0x426031, 0x428430, 0x45242a, 0x03702e, 0x42c464, 0x03003c, 0x47442e,
    0x436442, 0x438454, 0x43a448, 0x03503a, 0x43c466, 0x43e03d, 0x44a440,
    0x44c472, 0x46044e, 0x45a450, 0x45e470, 0x46a476, 0x46c478, 0x47c46e];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 9).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:          codeword
///  output:         index * 2
///
pub(super) static HUFF_TREE25: [u32; 168] = [
    0x400001, 0x002003, 0x41a402, 0x004007, 0x41c005, 0x035006, 0x434404,
    0x008010, 0x00900c, 0x04a00a, 0x42000b, 0x44e406, 0x03600d, 0x03800e,
    0x05a00f, 0x408468, 0x01101a, 0x012016, 0x039013, 0x070014, 0x46e015,
    0x40a440, 0x03b017, 0x01804d, 0x01904f, 0x4b840c, 0x01b022, 0x01c041,
    0x03f01d, 0x01e020, 0x01f05b, 0x40e4ee, 0x02107c, 0x45c410, 0x02302c,
    0x024028, 0x053025, 0x026045, 0x02707d, 0x412522, 0x047029, 0x05e02a,
    0x02b08a, 0x526414, 0x05602d, 0x02e081, 0x02f032, 0x06e030, 0x031080,
    0x416544, 0x079033, 0x034091, 0x41852c, 0x43641e, 0x04b037, 0x42246a,
    0x43c424, 0x04c03a, 0x426456, 0x03c066, 0x03d03e, 0x482428, 0x45842a,
    0x040072, 0x42c4ba, 0x050042, 0x04305c, 0x044074, 0x42e4be, 0x06a046,
    0x4dc430, 0x075048, 0x0490a3, 0x44a432, 0x450438, 0x43a452, 0x48443e,
    0x04e068, 0x45a442, 0x4d4444, 0x051088, 0x052087, 0x44648c, 0x077054,
    0x4da055, 0x50a448, 0x057060, 0x06b058, 0x05906d, 0x44c4f6, 0x46c454,
    0x45e474, 0x06905d, 0x460520, 0x05f07e, 0x462494, 0x061063, 0x07f062,
    0x464496, 0x06408b, 0x08d065, 0x542466, 0x067071, 0x4d2470, 0x4724ec,
    0x478476, 0x53a47a, 0x09b06c, 0x47c4ac, 0x4f847e, 0x06f078, 0x510480,
    0x48649e, 0x4884a0, 0x07307b, 0x49c48a, 0x4a648e, 0x098076, 0x4904c0,
    0x4924ea, 0x4c8498, 0x07a08e, 0x51249a, 0x4a24d6, 0x5064a4, 0x4f24a8,
    0x4aa4de, 0x51e4ae, 0x4b0538, 0x082092, 0x083085, 0x08f084, 0x5464b2,
    0x096086, 0x4ce4b4, 0x4d04b6, 0x089090, 0x4bc508, 0x4c253e, 0x08c0a4,
    0x5284c4, 0x4e04c6, 0x4ca4fa, 0x5144cc, 0x4f04d8, 0x4e24fc, 0x09309c,
    0x094099, 0x095097, 0x4e4516, 0x4e652e, 0x4e84fe, 0x4f450c, 0x09a09f,
    0x500502, 0x50450e, 0x09d0a0, 0x09e0a5, 0x518530, 0x51a54a, 0x0a70a1,
    0x0a20a6, 0x51c534, 0x53c524, 0x54052a, 0x548532, 0x536550, 0x54c54e];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 10).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:          codeword
///  output:         index * 2
///
pub(super) static HUFF_TREE26: [u32; 168] = [
    0x006001, 0x002013, 0x00300f, 0x00400d, 0x03b005, 0x40046e, 0x037007,
    0x00800a, 0x009067, 0x402420, 0x05600b, 0x00c057, 0x434404, 0x06600e,
    0x406470, 0x03c010, 0x059011, 0x06f012, 0x49e408, 0x014019, 0x03f015,
    0x016044, 0x017042, 0x079018, 0x4b840a, 0x01a01f, 0x01b047, 0x07c01c,
    0x08701d, 0x06901e, 0x44640c, 0x020027, 0x04b021, 0x02204f, 0x023025,
    0x02406b, 0x40e4e0, 0x081026, 0x528410, 0x02802c, 0x06c029, 0x08f02a,
    0x02b078, 0x53a412, 0x05202d, 0x02e033, 0x02f031, 0x0300a2, 0x4144ce,
    0x0a6032, 0x416534, 0x09a034, 0x09f035, 0x0360a7, 0x54e418, 0x03a038,
    0x436039, 0x43841a, 0x41c41e, 0x42246a, 0x05803d, 0x03e068, 0x424484,
    0x04005b, 0x04107a, 0x42645a, 0x043093, 0x4d2428, 0x05e045, 0x046072,
    0x42a45e, 0x048060, 0x073049, 0x04a098, 0x42c4c4, 0x07504c, 0x09504d,
    0x04e09c, 0x51042e, 0x063050, 0x077051, 0x43053c, 0x053084, 0x065054,
    0x4e4055, 0x4fe432, 0x43a454, 0x43c46c, 0x43e486, 0x07005a, 0x4a0440,
    0x07105c, 0x05d07b, 0x45c442, 0x05f08a, 0x476444, 0x07f061, 0x06206a,
    0x448506, 0x06408e, 0x52644a, 0x54444c, 0x45644e, 0x452450, 0x488458,
    0x4604ec, 0x4624f6, 0x50e464, 0x08206d, 0x0a406e, 0x542466, 0x4a2468,
    0x48a472, 0x474089, 0x4d8478, 0x097074, 0x47a508, 0x08d076, 0x47c4b6,
    0x51247e, 0x4804fc, 0x4bc482, 0x48c4a4, 0x48e4d4, 0x07d07e, 0x4904da,
    0x49208b, 0x094080, 0x49450c, 0x4964e2, 0x09d083, 0x52a498, 0x085091,
    0x0a5086, 0x4cc49a, 0x08808c, 0x4ee49c, 0x4a64ba, 0x4a84c0, 0x4c24aa,
    0x4ac4f0, 0x4ae4d0, 0x4ca4b0, 0x0900a1, 0x4b24ea, 0x092099, 0x4b4516,
    0x4d64be, 0x4c650a, 0x522096, 0x4c8524, 0x4dc4f2, 0x4de4f4, 0x4e6548,
    0x09e09b, 0x5384e8, 0x5204f8, 0x4fa53e, 0x50051a, 0x0a30a0, 0x502536,
    0x514504, 0x51e518, 0x54a51c, 0x54052c, 0x52e546, 0x530532, 0x54c550];

#[rustfmt::skip]
///  This table contains the decode tree for spectral data (Codebook 11).
///  bit 23 and 11 not used
///  bit 22 and 10 determine end value
///  bit 21-12 and 9-0 (offset to next node) or (index value * 2)
///
///  input:          codeword
///  output:         index * 2
///
pub(super) static HUFF_TREE27: [u32; 288] = [
    0x00100d, 0x002006, 0x003004, 0x400424, 0x047005, 0x402446, 0x048007,
    0x00800a, 0x00904c, 0x44a404, 0x07400b, 0x00c0bb, 0x466406, 0x00e014,
    0x00f054, 0x04e010, 0x051011, 0x0a9012, 0x0130bc, 0x408464, 0x01501f,
    0x01601a, 0x017059, 0x0af018, 0x0ca019, 0x40a0e4, 0x01b05e, 0x01c084,
    0x0bf01d, 0x05d01e, 0x55a40c, 0x020026, 0x021066, 0x043022, 0x023062,
    0x02408d, 0x025108, 0x40e480, 0x027030, 0x02802c, 0x02906b, 0x02a0da,
    0x06502b, 0x4105c8, 0x0a402d, 0x0ec02e, 0x0dd02f, 0x532412, 0x06e031,
    0x032036, 0x03303e, 0x0fd034, 0x0fc035, 0x4145b0, 0x03703a, 0x038117,
    0x10d039, 0x5ba416, 0x10f03b, 0x03c041, 0x5fa03d, 0x41c418, 0x10403f,
    0x04011d, 0x41a5f4, 0x11c042, 0x41e61c, 0x087044, 0x0f5045, 0x0d9046,
    0x4204a2, 0x640422, 0x04904a, 0x426448, 0x04b073, 0x428468, 0x46c04d,
    0x48a42a, 0x04f077, 0x076050, 0x42c4b0, 0x0520a7, 0x096053, 0x42e4a8,
    0x05507d, 0x07a056, 0x0d4057, 0x0df058, 0x442430, 0x05a081, 0x05b09b,
    0x05c0e2, 0x5b8432, 0x4fe434, 0x05f09e, 0x0e6060, 0x0610d6, 0x57c436,
    0x0cc063, 0x112064, 0x4384a0, 0x43a5ca, 0x067089, 0x0680b7, 0x0690a2,
    0x0a106a, 0x43c59c, 0x09206c, 0x06d0ba, 0x60643e, 0x0d106f, 0x0700ee,
    0x0de071, 0x10b072, 0x44056c, 0x46a444, 0x075094, 0x48c44c, 0x44e490,
    0x095078, 0x0ab079, 0x4504ce, 0x07b097, 0x11e07c, 0x630452, 0x0ac07e,
    0x07f099, 0x080106, 0x4544b8, 0x0820b1, 0x0830e5, 0x4fc456, 0x0b3085,
    0x08609d, 0x45853e, 0x0880c2, 0x5c045a, 0x08a08f, 0x08b0ce, 0x08c0f7,
    0x58645c, 0x11108e, 0x45e5c4, 0x0c4090, 0x10a091, 0x4604e4, 0x0d0093,
    0x462608, 0x48e46e, 0x4704b2, 0x4d2472, 0x0980bd, 0x4f2474, 0x0e309a,
    0x4764aa, 0x0be09c, 0x47851a, 0x47a4de, 0x09f0b5, 0x0a00c1, 0x50047c,
    0x57847e, 0x0a30c3, 0x504482, 0x0e90a5, 0x0a6100, 0x4c8484, 0x0a811f,
    0x48662a, 0x0c70aa, 0x488494, 0x4924d0, 0x0ad0c8, 0x0ae0d8, 0x496636,
    0x10e0b0, 0x4f8498, 0x0f30b2, 0x49a4dc, 0x0f20b4, 0x53c49c, 0x0b60cb,
    0x49e57a, 0x0b80e0, 0x0b9109, 0x5e44a4, 0x5484a6, 0x4ac4ae, 0x4b44ca,
    0x4d64b6, 0x4ba5da, 0x0c60c0, 0x4bc51e, 0x4be556, 0x6204c0, 0x4c24c4,
    0x0f80c5, 0x5664c6, 0x4cc53a, 0x4d462c, 0x0f10c9, 0x4d8552, 0x4da4fa,
    0x5be4e0, 0x0cd0ff, 0x5244e2, 0x0cf0e8, 0x4e6568, 0x59a4e8, 0x0f90d2,
    0x1010d3, 0x5ac4ea, 0x0d50d7, 0x4ec634, 0x4ee560, 0x4f44f0, 0x4f6638,
    0x502522, 0x0db0dc, 0x5065a6, 0x508604, 0x60050a, 0x50c0fb, 0x63250e,
    0x1130e1, 0x5a4510, 0x5125fc, 0x516514, 0x51863e, 0x51c536, 0x0e70f4,
    0x55c520, 0x602526, 0x0eb0ea, 0x5cc528, 0x5ea52a, 0x1140ed, 0x60c52c,
    0x1020ef, 0x0f0119, 0x58e52e, 0x530622, 0x558534, 0x53861e, 0x55e540,
    0x5800f6, 0x57e542, 0x5445e6, 0x5465e8, 0x0fa115, 0x54c54a, 0x54e60e,
    0x5ae550, 0x1160fe, 0x5f0554, 0x564562, 0x56a58a, 0x56e5ee, 0x10310c,
    0x5705d0, 0x107105, 0x5725d4, 0x57463a, 0x5765b4, 0x5825bc, 0x5845e2,
    0x5885de, 0x58c592, 0x5ce590, 0x5945f6, 0x63c596, 0x11b110, 0x5d8598,
    0x5c259e, 0x5e05a0, 0x5a25c6, 0x5a860a, 0x5aa5ec, 0x5b2610, 0x11a118,
    0x6185b6, 0x5f25d2, 0x5d6616, 0x5dc5f8, 0x61a5fe, 0x612614, 0x62e624,
    0x626628];

#[rustfmt::skip]
/// get starting addresses of huffman tables into an array [convert codebook into starting address]
/// cb    tree
pub(super) static HUFF_TABLE: [&[u32]; MAX_CB] = [
    &HUFF_TREE41,     // 0      -   // use tree 1 as dummy here
    &HUFF_TREE41,     // 1      1
    &HUFF_TREE42,     // 2      2
    &HUFF_TREE43,     // 3      3
    &HUFF_TREE44,     // 4      4
    &HUFF_TREE21,     // 5      5
    &HUFF_TREE22,     // 6      6
    &HUFF_TREE23,     // 7      7
    &HUFF_TREE24,     // 8      8
    &HUFF_TREE25,     // 9      9
    &HUFF_TREE26,     // 10     10
    &HUFF_TREE27,     // 11     11
    &HUFF_TREE41,     // 12     -   // use tree 1 as dummy here
    &HUFF_TREE41,     // 13     -   // use tree 1 as dummy here
    &HUFF_TREE41,     // 14     -   // use tree 1 as dummy here
    &HUFF_TREE41,     // 15     -   // use tree 1 as dummy here
    &HUFF_TREE27,     // 16     11
    &HUFF_TREE27,     // 17     11
    &HUFF_TREE27,     // 18     11
    &HUFF_TREE27,     // 19     11
    &HUFF_TREE27,     // 20     11
    &HUFF_TREE27,     // 21     11
    &HUFF_TREE27,     // 22     11
    &HUFF_TREE27,     // 23     11
    &HUFF_TREE27,     // 24     11
    &HUFF_TREE27,     // 25     11
    &HUFF_TREE27,     // 26     11
    &HUFF_TREE27,     // 27     11
    &HUFF_TREE27,     // 28     11
    &HUFF_TREE27,     // 29     11
    &HUFF_TREE27,     // 30     11
    &HUFF_TREE27];    // 31     11
