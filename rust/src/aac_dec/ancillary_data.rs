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
//! Ancillary data.

// Imports.
use crate::{aac_dec::error_codes::AacDecoderError, common::bitstream::Bitstream};

/// Structure which holds the ancillary data.
#[repr(C)]
#[derive(Debug)]
pub struct AncillaryData {
    /// Ancillary data bitstream start offset in bits.
    data_start: isize,
    /// Ancillary data length in bytes.
    data_len: usize,
    /// Element instance tag if present, otherwise 255.
    element_instance_tag: u8,
    /// Ancillary data type.
    anc_type: AncDataType,
}

/// Structure which describes the ancillary data type.
#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum AncDataType {
    Unknown,
    ExtensionDataElement,
    DataStreamElement,
    Mpeg4AncillaryData,
}

impl Default for AncillaryData {
    fn default() -> Self {
        Self {
            data_start: 0,
            data_len: 0,
            element_instance_tag: 255,
            anc_type: AncDataType::Unknown,
        }
    }
}

impl AncillaryData {
    /// Creates an ancillary data element (`AncillaryData`) instance and stores the provided data.
    ///
    /// # Parameters
    ///
    /// - `data_start`: Ancillary data bitstream start offset in bits.
    /// - `data_len`: Ancillary data length in bytes.
    /// - `element_instance_tag`: Element instance tag if present, otherwise 255.
    /// - `anc_type`: Ancillary data type.
    ///
    /// # Return
    ///
    /// A new instance of `AncillaryData` with default values.
    pub fn new(
        data_start: isize,
        data_len: usize,
        element_instance_tag: u8,
        anc_type: AncDataType,
    ) -> Self {
        AncillaryData {
            data_start,
            data_len,
            element_instance_tag,
            anc_type,
        }
    }

    /// Read one ancillary data element into the provided buffer.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream handle to access bitstream data.
    /// - `buffer`: Buffer receiving the requested ancillary data.
    /// - `num_bytes`: Number of bytes written to buffer.
    ///
    /// # Return
    ///
    /// `AacDecoderError`
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        buffer: &mut [u8],
    ) -> Result<usize, AacDecoderError> {
        if (buffer.len() < self.data_len)
            || (self.data_start < (self.data_len * 8).try_into().unwrap())
        {
            buffer.fill(0);
            Err(AacDecoderError::AncDataError)
        } else {
            let bs_anchor = bs.valid_bits();
            bs.push(bs_anchor - self.data_start);
            for buffer_elem in buffer.iter_mut().take(self.data_len) {
                *buffer_elem = bs.read(8) as u8;
            }
            let valid_bits_tmp = bs.valid_bits();
            bs.push(valid_bits_tmp - bs_anchor);
            Ok(self.data_len)
        }
    }

    /// Returns ancillary data bitstream start offset in bits.
    pub(super) fn data_start(&self) -> isize {
        self.data_start
    }

    /// Returns ancillary data length in bytes.
    pub(super) fn data_len(&self) -> usize {
        self.data_len
    }

    /// Returns ancillary data type.
    pub(super) fn anc_type(&self) -> AncDataType {
        self.anc_type
    }
}
