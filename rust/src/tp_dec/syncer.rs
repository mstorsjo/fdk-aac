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
//! MPEG transport format decoder syncer

use super::{Adts, TpDecCallBacks, TpDecData, TpDecoderError};
use crate::common::{bitstream::Bitstream, transport_type::TransportType};

// Structs
#[derive(Default, Debug)]
#[repr(C)]
/// Synchronization structure.
pub struct Syncer {
    /// Sync word to be found.
    word: u32,
    /// Length of the sync word in bits.
    length: isize,
    /// Mask for sync word.
    mask: u32,
    /// Number of bits to advance sync search.
    sync_skip: isize,
    /// Sync status.
    is_sync_ok: bool,
    /// If true the buffer fullness should be ignored.
    is_ignore_buffer_fullness: bool,
    /// If true two sync words should be checked.
    is_check_two_syncs: bool,
    /// Bitbuffer position points to last sync word.
    bs_anchor: isize,
}

impl Syncer {
    /// Returns new instance of the `Syncer`.
    pub fn _new() -> Self {
        Default::default()
    }

    /// Initialises `Syncer`.
    ///
    /// # Parameters
    ///
    /// - `transport_type`: `TransportType` format.
    pub(super) fn init(&mut self, transport_type: TransportType) {
        // Set transport specific sync parameters.
        match transport_type {
            TransportType::Mp4Adts => {
                self.word = Adts::SYNCWORD;
                self.length = Adts::SYNCLENGTH as isize;
            }
            TransportType::Mp4Loas => {
                self.word = 0x2B7;
                self.length = 11;
            }
            _ => {
                self.word = 0;
                self.length = 0;
            }
        };

        self.mask = (1 << self.length) - 1;
        // Number of bits to advance for synchronization search.
        self.sync_skip = 8;
        self.is_sync_ok = false;
        self.is_ignore_buffer_fullness = false;
        self.bs_anchor = 0;
    }

    /// Reads second synch word from bitstream.
    ///
    /// # Parameters
    ///
    /// `bs`: Bitstream instance with valid internal data.
    /// - `transport_type`: `TransportType` format.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    fn read_second_sync_word(
        &mut self,
        bs: &mut Bitstream,
        transport_type: TransportType,
    ) -> Result<(), TpDecoderError> {
        let mut err = Ok(());
        let bs_anchor = bs.valid_bits();
        let frame_data_bits = (8 * match transport_type {
            TransportType::Mp4Adts => {
                let num_bits = bs.valid_bits() - self.bs_anchor + 30;
                bs.push(num_bits);
                bs.read(13)
            }
            TransportType::Mp4Loas => {
                let num_bits = bs.valid_bits() - self.bs_anchor + 11;
                bs.push(num_bits);
                bs.read(13) + 3
            }
            _ => 0,
        }) as isize;

        if self.bs_anchor < (frame_data_bits + self.length) {
            err = Err(TpDecoderError::NotEnoughBits)
        } else {
            let num_bits = bs.valid_bits() - self.bs_anchor + frame_data_bits;
            bs.push(num_bits);
            if bs.read(self.length.try_into().unwrap()) != self.word {
                err = Err(TpDecoderError::SyncError);
            }
        }

        let num_bits = bs.valid_bits() - bs_anchor;
        bs.push(num_bits);

        err
    }

    /// Reads synch word from bitstream.
    ///
    /// # Parameters
    ///
    /// `bs`: Bitstream instance with valid internal data.
    /// `data`: Transport decoder data.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    fn read(&mut self, bs: &mut Bitstream, data: &TpDecData) -> Result<(), TpDecoderError> {
        self.bs_anchor = bs.valid_bits();

        if data.num_raw_data_blocks == 0 {
            debug_assert!((bs.valid_bits() % self.sync_skip) == 0);
            if (bs.valid_bits() - self.length) < self.sync_skip {
                return Err(TpDecoderError::NotEnoughBits);
            } else {
                // Current sync word read from bitstream.
                let mut synch = bs.read(self.length as u8);
                if !self.is_sync_ok {
                    while (synch != self.word) && (bs.valid_bits() >= self.sync_skip) {
                        synch =
                            ((synch << self.sync_skip) & self.mask) | bs.read(self.sync_skip as u8);
                    }
                }
                self.bs_anchor = bs.valid_bits() + self.length;
                if synch != self.word {
                    return Err(TpDecoderError::SyncError);
                }

                // Check next syncword.
                if self.is_check_two_syncs {
                    return self.read_second_sync_word(bs, data.transport_type);
                }
            }
        }
        Ok(())
    }

    /// Rewinds and skips sync_skip bits, in order to search for next sync word.
    /// Ensures that the bit amount lands at a multiple of sync_skip.
    ///
    /// # Parameters
    ///
    /// `bs`: Bitstream instance with valid internal data.
    pub(super) fn skip(&self, bs: &mut Bitstream) {
        let num_bits =
            (bs.valid_bits() - self.bs_anchor) + self.sync_skip + (self.bs_anchor % self.sync_skip);
        bs.push(num_bits);
    }

    /// Moves the decoding position back to a previous point in the transport stream.
    ///
    /// # Parameters
    ///
    /// `bs`: Bitstream instance with valid internal data.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(super) fn rewind(&mut self, bs: &mut Bitstream) -> Result<(), TpDecoderError> {
        let mut err = Ok(());
        // Detect pointless TpDecoderError::NotEnoughBits error case, where the
        // bit buffer is already full, or no new burst packet fits. Recover by
        // advancing the bit buffer.
        if self.bs_anchor > ((bs.buffer().len() * 8) - 8).try_into().unwrap() {
            err = Err(TpDecoderError::SyncError);
        }

        let num_bits = bs.valid_bits() - self.bs_anchor;
        bs.push(num_bits);

        err
    }

    /// Performs synchronisation.
    ///
    /// # Parameters
    ///
    /// `bs`: Bitstream instance with valid internal data.
    /// `data`: Transport decoder data.
    /// `cb`: Transport decoder callbacks.
    ///
    /// # Return
    ///
    /// - `TpDecoderError`.
    pub(super) fn synchronise(
        &mut self,
        bs: &mut Bitstream,
        data: &mut TpDecData,
        cb: &mut TpDecCallBacks,
    ) -> Result<(), TpDecoderError> {
        let mut err;

        loop {
            err = self.read(bs, data);

            // Parse transport header (raw data block granularity).
            if err.is_ok() {
                err = data.read_header(bs, cb, self.is_ignore_buffer_fullness || self.is_sync_ok);

                if data.transport_type == TransportType::Mp4Adts
                    && data.adts.as_ref().unwrap().is_mpeg2_implicit_config()
                    && !self.is_sync_ok
                {
                    err = Ok(());
                    break;
                }

                if err.is_ok() {
                    self.is_sync_ok = true;
                    break;
                }
            }

            if err == Err(TpDecoderError::NotEnoughBits) {
                err = self.rewind(bs);
                if err.is_ok() {
                    err = Err(TpDecoderError::NotEnoughBits);
                    break;
                }
            }

            self.skip(bs);

            // Enforce re-sync of transport headers.
            data.reset();
            self.is_sync_ok = false;

            // Condition
            if err != Err(TpDecoderError::SyncError) || self.is_sync_ok {
                break;
            }
        }

        err
    }

    /// Sets the value of `is_ignore_buffer_fullness`.
    pub fn set_is_ignore_buffer_fullness(&mut self, value: bool) {
        self.is_ignore_buffer_fullness = value;
    }

    /// Sets the value of `is_check_two_syncs`.
    pub fn set_is_check_two_syncs(&mut self, value: bool) {
        self.is_check_two_syncs = value;
    }

    /// Gets the value of `is_sync_ok`.
    pub fn is_sync_ok(&self) -> bool {
        self.is_sync_ok
    }

    /// Sets the value of `is_sync_ok`.
    pub fn set_is_sync_ok(&mut self, value: bool) {
        self.is_sync_ok = value
    }
}
