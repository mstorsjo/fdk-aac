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
//! Efficient x^y implementations for commonly used values

// Table: pow (2.0, 0.25 * i)
#[rustfmt::skip]
static EXPONENT_TABLE: [f32; 128] = [
    1.0,          1.1892071,      1.4142135,       1.6817929,
    2.0,          2.3784142,      2.8284271,       3.3635857,
    4.0,          4.7568283,      5.6568542,       6.7271714,
    8.0,          9.5136566,      11.3137083,      13.4543428,
    16.0,         19.0273132,     22.6274166,      26.9086857,
    32.0,         38.0546265,     45.2548332,      53.8173714,
    64.0,         76.1092529,     90.5096664,      107.6347427,
    128.0,        152.2185059,    181.0193329,     215.2694855,
    256.0,        304.4370117,    362.0386658,     430.5389709,
    512.0,        608.8740234,    724.0773315,     861.0779419,
    1024.0,       1217.7480469,   1448.1546631,    1722.1558838,
    2048.0,       2435.4960938,   2896.3093262,    3444.3117676,
    4096.0,       4870.9921875,   5792.6186523,    6888.6235352,
    8192.0,       9741.9843750,   11585.2373047,   13777.2470703,
    16384.0,      19483.9687500,  23170.4746094,   27554.4941406,
    32768.0,      38967.9375000,  46340.9492188,   55108.9882812,
    65536.0,      77935.8750000,  92681.8984375,   110217.9765625,
    131072.0,     155871.7500000, 185363.7968750,  220435.9531250,
    262144.0,     311743.5000000, 370727.5937500,  440871.9062500,
    524288.0,     623487.0,       741455.1875000,  881743.8125000,
    1048576.0,    1246974.0,      1482910.3750000, 1763487.6250000,
    2097152.0,    2493948.0,      2965820.7500000, 3526975.2500000,
    4194304.0,    4987896.0,      5931641.5000000, 7053950.5000000,
    8388608.0,    9975792.0,      11863283.0,      14107901.0,
    16777216.0,   19951584.0,     23726566.0,      28215802.0,
    33554432.0,   39903168.0,     47453132.0,      56431604.0,
    67108864.0,   79806336.0,     94906264.0,      112863208.0,
    134217728.0,  159612672.0,    189812528.0,     225726416.0,
    268435456.0,  319225344.0,    379625056.0,     451452832.0,
    536870912.0,  638450688.0,    759250112.0,     902905664.0,
    1073741824.0, 1276901376.0,   1518500224.0,    1805811328.0,
    2147483648.0, 2553802752.0,   3037000448.0,    3611622656.0,
];

#[inline]
/// Calculate 2^(scf/4) using a lookup table for -127 <= scf <= 127.
pub fn scale_factor_2_exp(scf: i16) -> f32 {
    if scf.is_negative() {
        let abs_scf = usize::from(scf.unsigned_abs());
        if abs_scf < EXPONENT_TABLE.len() {
            EXPONENT_TABLE[abs_scf].recip()
        } else {
            f32::powf(2.0f32, -0.25f32 * abs_scf as f32)
        }
    } else if (scf as usize) < EXPONENT_TABLE.len() {
        EXPONENT_TABLE[scf as usize]
    } else {
        f32::powf(2.0f32, 0.25f32 * scf as f32)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_scale_factor_2_exp() {
        assert!(0.25 == scale_factor_2_exp(-8));
        assert!(2.328_306_4E-10 == scale_factor_2_exp(-128));
        assert!(4f32 == scale_factor_2_exp(8));
        assert!(4.294_967_3E9 == scale_factor_2_exp(128));
        assert!(scale_factor_2_exp(i16::MIN) == 0.0_f32);
    }
}
