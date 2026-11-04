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
//! Sample Rate mapping

/// Mapping to std samplerate table according to 14496-3 (4.6.18.2.6)
#[repr(C)]
#[derive(Debug)]
struct SRMapping {
    fs_range_lo: u32,
    fs_mapped: u32,
}

const STD_SAMPLE_RATES_MAPPING: [SRMapping; 12] = [
    SRMapping {
        fs_range_lo: 0,
        fs_mapped: 8000,
    },
    SRMapping {
        fs_range_lo: 9391,
        fs_mapped: 11025,
    },
    SRMapping {
        fs_range_lo: 11502,
        fs_mapped: 12000,
    },
    SRMapping {
        fs_range_lo: 13856,
        fs_mapped: 16000,
    },
    SRMapping {
        fs_range_lo: 18783,
        fs_mapped: 22050,
    },
    SRMapping {
        fs_range_lo: 23004,
        fs_mapped: 24000,
    },
    SRMapping {
        fs_range_lo: 27713,
        fs_mapped: 32000,
    },
    SRMapping {
        fs_range_lo: 37566,
        fs_mapped: 44100,
    },
    SRMapping {
        fs_range_lo: 46009,
        fs_mapped: 48000,
    },
    SRMapping {
        fs_range_lo: 55426,
        fs_mapped: 64000,
    },
    SRMapping {
        fs_range_lo: 75132,
        fs_mapped: 88200,
    },
    SRMapping {
        fs_range_lo: 92017,
        fs_mapped: 96000,
    },
];

const STD_SAMPLE_RATES_MAPPING_USAC: [SRMapping; 10] = [
    SRMapping {
        fs_range_lo: 0,
        fs_mapped: 16000,
    },
    SRMapping {
        fs_range_lo: 18783,
        fs_mapped: 22050,
    },
    SRMapping {
        fs_range_lo: 23004,
        fs_mapped: 24000,
    },
    SRMapping {
        fs_range_lo: 27713,
        fs_mapped: 32000,
    },
    SRMapping {
        fs_range_lo: 35777,
        fs_mapped: 40000,
    },
    SRMapping {
        fs_range_lo: 42000,
        fs_mapped: 44100,
    },
    SRMapping {
        fs_range_lo: 46009,
        fs_mapped: 48000,
    },
    SRMapping {
        fs_range_lo: 55426,
        fs_mapped: 64000,
    },
    SRMapping {
        fs_range_lo: 75132,
        fs_mapped: 88200,
    },
    SRMapping {
        fs_range_lo: 92017,
        fs_mapped: 96000,
    },
];

/// Maps the given sample rate to a standard sample rate based on whether USAC is used.
///
/// # Return
///
/// The mapped standard sample rate.
pub(super) fn map_to_std_sample_rate(fs: u32, is_usac: bool) -> u32 {
    let mapping_table: &[SRMapping] = if !is_usac {
        &STD_SAMPLE_RATES_MAPPING
    } else {
        &STD_SAMPLE_RATES_MAPPING_USAC
    };

    let mut fs_mapped = fs;

    for i in mapping_table.iter().rev() {
        if fs >= i.fs_range_lo {
            fs_mapped = i.fs_mapped;
            break;
        }
    }

    fs_mapped
}
