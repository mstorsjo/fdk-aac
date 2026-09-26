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
//! MPEG-D DRC interpolation buffers used for `DrcGainDecoder`.

use super::super::{common::GainInterpolationType, constants::MAX_SEQUENCES};
use super::linear_node_buffer::LinearNodeBuffer;

#[repr(C)]
#[derive(Default, Debug)]
/// Struct that holds interpolation nodes to be applied for the gains of each DRC sequence.
pub(super) struct DrcGainBuffers {
    /// Index of the most recent node buffer.
    lnb_index: usize,
    /// Holds interpolation nodes to be applied for the gains of each DRC sequence.
    linear_node_buffer: [LinearNodeBuffer; MAX_SEQUENCES as usize],
    /// Dummy Linear Node Buffer. Used for "no DRC processing".
    dummy_lnb: LinearNodeBuffer,
}

impl DrcGainBuffers {
    /// Initializes the start and stop interpolation nodes for all DRC sequences.
    pub(super) fn init(&mut self, frame_size: u16) {
        // Prepare MAX_SEQUENCES instances of node buffers.
        for lnb_elem in self.linear_node_buffer.iter_mut() {
            lnb_elem.init_segment_start(
                GainInterpolationType::Linear,
                1.0,
                (frame_size - 1) as i16,
            );

            // Initialize last node with startup node.
            for nl in lnb_elem.linear_nodes_mut().iter_mut().take(1) {
                nl[0].init(1.0, 0);
            }
        }

        // Prepare dummy_lnb, a linear_node_buffer containing a constant gain of 0 dB, for the
        // "no DRC processing" case.
        self.dummy_lnb.init_segment_start(
            GainInterpolationType::Linear,
            1.0,
            (frame_size - 1) as i16,
        );

        self.lnb_index = 0;
    }

    /// Increment the index of the linear node buffer.
    pub(super) fn inc_lnb_index(&mut self) {
        self.lnb_index += 1;
    }

    /// Gets an immutable reference to a `LinearNodeBuffer` instance, depending on `index`. The max
    /// `index` value is `MAX_SEQUENCES-1`.
    pub(super) fn linear_node_buffer(&self, index: usize) -> &LinearNodeBuffer {
        &self.linear_node_buffer[index]
    }

    /// Gets a mutable reference to a `LinearNodeBuffer` instance, depending on `index`. The max
    /// `index` value is `MAX_SEQUENCES-1`.
    pub(super) fn linear_node_buffer_mut(&mut self, index: usize) -> &mut LinearNodeBuffer {
        &mut self.linear_node_buffer[index]
    }

    /// Gets index of linear node buffer.
    pub(super) fn lnb_index(&self) -> usize {
        self.lnb_index
    }

    /// Gets an immutable reference to a `LinearNodeBuffer` instance. The `dummy_lnb` is used for
    /// "no DRC processing".
    pub(super) fn dummy_lnb(&self) -> &LinearNodeBuffer {
        &self.dummy_lnb
    }

    /// Sets index of linear node buffer.
    pub(super) fn set_lnb_index(&mut self, value: usize) {
        self.lnb_index = value;
    }
}
