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
//! MPEG-D DRC's uniDRC gain.

use super::super::{constants::*, drc_error::DrcError};
use super::{gain_node::GainNode, DrcCoefficientsUniDrc};
use crate::common::bitstream::Bitstream;
use itertools::izip;

/// uniDRC gain extension type for termination tag.
const UNIDRCGAINEXT_TERM: u8 = 0x0;

#[repr(C)]
#[derive(Default, Debug)]
/// `UniDrcGainExtension` structure that holds extension type and bit size.
struct UniDrcGainExtension {
    /// Gain extension type.
    uni_drc_gain_ext_type: [u8; MAX_EXTENSIONS],
    /// Extension bit size.
    ext_bit_size: [u32; MAX_EXTENSIONS - 1],
}

impl UniDrcGainExtension {
    /// Reads `UniDrcGainExtension` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let mut is_early_break = false;

        'read_loop: for (ext_bit_size, ext_type) in izip!(
            self.ext_bit_size.iter_mut(),
            self.uni_drc_gain_ext_type.iter_mut()
        )
        .take(MAX_EXTENSIONS - 1)
        {
            *ext_type = bs.read(4) as u8;
            if *ext_type == UNIDRCGAINEXT_TERM {
                is_early_break = true;
                break 'read_loop;
            }
            let bit_size_len = bs.read(3) as u8;
            let ext_size_bits = bit_size_len + 4;

            let bit_size = bs.read(ext_size_bits);
            *ext_bit_size = bit_size + 1;

            // Add future extensions here.
            bs.push(*ext_bit_size as isize);
        }

        if !is_early_break {
            // Read last item of uni_drc_gain_ext_type (i.e [MAX_EXTENSIONS-1]) from
            // bitstream.
            self.uni_drc_gain_ext_type[MAX_EXTENSIONS - 1] = bs.read(4) as u8;
            if self.uni_drc_gain_ext_type[MAX_EXTENSIONS - 1] != UNIDRCGAINEXT_TERM {
                return Err(DrcError::MemoryError);
            }
        }

        Ok(())
    }
}

#[repr(C)]
#[derive(Default, Debug)]
/// `UniDrcGain` structure that holds `DRC gain sequence` and `DRC gain extension`. Where each
/// `DRC gain sequence` contains corresponding set of gain nodes.
pub(in super::super) struct UniDrcGain {
    /// Buffer to store number of gain values (GainNode) encoded for each DRC gain sequence.
    num_nodes: [u8; MAX_SEQUENCES as usize],
    /// Buffer to store `GainNodes` for each DRC gain sequence.
    gain_nodes: [[GainNode; MAX_NODES]; MAX_SEQUENCES as usize],
    /// Flag to indicate presence of gain extension.
    is_uni_drc_gain_ext_present: bool,
    /// Gain extension.
    uni_drc_gain_extension: UniDrcGainExtension,
    /// Flag to indicate `gain sequence` data has been read from bitstream or not.
    is_status: bool,
}

impl UniDrcGain {
    /// Creates gain sequence out of gain sequences of last frame for concealment and flushing.
    /// # Parameters
    /// - `gain_seq_count`: Gain sequence count. Range: [1..MAX_SEQUENCES).
    /// - `time_offset`: Time offset (in samples) for the first `GainNode`.
    ///
    /// # Return
    /// - `Result<(), DrcError>`
    pub(in super::super) fn conceal(
        &mut self,
        gain_seq_count: usize,
        time_offset: i16,
    ) -> Result<(), DrcError> {
        for (g_node_seq, node_seq_idx) in
            izip!(self.gain_nodes.iter_mut(), self.num_nodes.iter_mut(),).take(gain_seq_count)
        {
            let last_node_idx = i16::from(*node_seq_idx) - 1;
            let last_gain_db = if (0..MAX_NODES as i16).contains(&last_node_idx) {
                g_node_seq[last_node_idx as usize].gain_db()
            } else {
                0.0_f32
            };

            let gain_coeff = if last_gain_db > 0.0 { 0.90 } else { 0.98 };

            *node_seq_idx = 1;
            g_node_seq[0].set_gain_db(gain_coeff * last_gain_db);
            g_node_seq[0].set_time(time_offset - 1);
        }
        Ok(())
    }
    /// Sets value of status.
    pub(in super::super) fn set_status(&mut self, status: bool) {
        self.is_status = status;
    }

    /// Returns value of status.
    pub(in super::super) fn get_status(&self) -> bool {
        self.is_status
    }

    /// Returns reference to nodes buffer `[u8; MAX_SEQUENCES]`.
    pub(in super::super) fn num_nodes(&self) -> &[u8; MAX_SEQUENCES as usize] {
        &self.num_nodes
    }

    pub(in super::super) fn _num_nodes_mut(&mut self) -> &mut [u8; MAX_SEQUENCES as usize] {
        &mut self.num_nodes
    }

    /// Returns a reference to gain nodes buffer.
    /// `[[GainNode; MAX_NODES]; MAX_SEQUENCES]`
    pub(in super::super) fn gain_nodes(&self) -> &[[GainNode; MAX_NODES]; MAX_SEQUENCES as usize] {
        &self.gain_nodes
    }

    pub(in super::super) fn _gain_nodes_mut(
        &mut self,
    ) -> &mut [[GainNode; MAX_NODES]; MAX_SEQUENCES as usize] {
        &mut self.gain_nodes
    }

    /// Reads `UniDrcGain` data from bitstream if `coeff_uni_drc` contains valid gain set data
    /// for each `DRC gain sequence`.
    ///
    /// # Parameters
    /// - `bs`: Bitstream reader with valid internal data
    /// - `coeff_uni_drc`: Immutable reference to `DrcCoefficientsUniDrc` instance (or) `None`.
    /// - `delta_tmin_default`: Default delta time minimum value.
    /// - `frame_size`: Length of DRC frame.
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read(
        &mut self,
        bs: &mut Bitstream,
        coeff_uni_drc: Option<&DrcCoefficientsUniDrc>,
        delta_tmin_default: u16,
        frame_size: i16,
    ) -> Result<(), DrcError> {
        self.set_status(false);

        if let Some(coeff) = coeff_uni_drc {
            let gain_sequence_count = coeff.gain_sequence_count().min(MAX_SEQUENCES);

            for (index, nodes, gain_node) in izip!(
                coeff.gain_set_index_for_gain_sequence().iter(),
                self.num_nodes.iter_mut(),
                self.gain_nodes.iter_mut()
            )
            .take(usize::from(gain_sequence_count))
            {
                let mut tmp_nodes = [GainNode::default(); MAX_NODES];
                let mut tmp_num_nodes = 0;
                if *index >= coeff.gain_set_count() || *index >= MAX_SEQUENCES {
                    return Err(DrcError::NotOk);
                }

                let gain_set = coeff.gain_set(usize::from(*index));

                let time_delta_min = gain_set.get_time_delta_min(delta_tmin_default);
                gain_set.read_drc_gain_sequence(
                    bs,
                    &mut tmp_nodes,
                    &mut tmp_num_nodes,
                    time_delta_min,
                    frame_size,
                );

                *nodes = tmp_num_nodes as u8;
                let copy_length = tmp_num_nodes.min(MAX_NODES);
                gain_node[..copy_length].copy_from_slice(&tmp_nodes[..copy_length]);
            }
            if gain_sequence_count == coeff.gain_sequence_count() {
                // All sequences have been read.
                self.is_uni_drc_gain_ext_present = bs.read_bit() != 0;
                if self.is_uni_drc_gain_ext_present {
                    self.uni_drc_gain_extension.read(bs)?;
                }
            }

            if gain_sequence_count > 0 {
                self.set_status(true);
            }
            Ok(())
        } else {
            Ok(())
        }
    }
}
