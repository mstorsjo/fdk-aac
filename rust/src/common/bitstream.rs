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
//! Bit stream reading and writing
//!
//! High-level functions for handling `Bitbuffer`

use super::bitbuffer;

const CACHE_BITS: u8 = 32;

/// Represents a Bitstream operation mode
///
/// Can be configured either as bitstream reader or as bitstream writer.
#[derive(Debug, PartialEq, Eq, Copy, Clone)]
#[repr(C)]
pub enum Mode {
    Reader,
    Writer,
}

/// Define `Reader` as default Bitstream operation mode
impl Default for Mode {
    fn default() -> Self {
        Mode::Reader
    }
}

/// Bitstream Reader and Bitstream Writer
///
/// # Examples
///
/// ```
/// use aac::common::bitstream::{Bitstream, Mode};
///
/// let mut bitstream_writer = Bitstream::new(8, Mode::Writer);
/// bitstream_writer.write(0b1011_0001, 8);
///
/// let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
/// bitstream_reader.init(bitstream_writer.buffer(), 8);
/// let value = bitstream_reader.read(4);
/// ```
#[derive(Default, PartialEq, Debug)]
#[repr(C)]
pub struct Bitstream {
    cache: u32,
    bits_in_cache: u8,
    bit_buffer: bitbuffer::Bitbuffer,
    mode: Mode,
}

impl Bitstream {
    /// Creates a new Bitstream instance
    ///
    /// # Parameters
    ///
    /// - `length`: Length of Bitbuffer in bytes (must be `2^n` and greater than zero length)
    /// - `mode`: Indicates whether the Bitstream shall be used as reader or writer
    ///
    /// # Return
    /// Returns a new Bitstream instance
    pub fn new(length: usize, mode: Mode) -> Self {
        Self {
            cache: 0,
            bits_in_cache: 0,
            bit_buffer: bitbuffer::Bitbuffer::new(length),
            mode,
        }
    }

    /// Initializes the Bitstream with given input data
    ///
    /// # Parameters
    ///
    /// - `buffer`: Input data buffer
    /// - `valid_bits`: Number of valid bits in provided input buffer
    pub fn init(&mut self, buffer: &[u8], valid_bits: usize) {
        self.bit_buffer.init(buffer, valid_bits);
        self.cache = 0;
        self.bits_in_cache = 0;
    }

    /// Destroys the dynamic allocated Bitstream memory
    pub fn destroy(&mut self) {
        self.bit_buffer.destroy()
    }

    /// Resets all relevant Bitstream states and its underlying Bitbuffer
    pub fn reset(&mut self) {
        self.bit_buffer.reset();
        self.cache = 0;
        self.bits_in_cache = 0;
    }

    /// Returns a number of sequential bits from Bitstream
    ///
    /// # Parameters
    ///
    /// - `num_bits`: Number of bits to be retrieved. Restricted to `[0; 32]`
    pub fn read(&mut self, num_bits: u8) -> u32 {
        debug_assert!(num_bits <= 32);
        let mut value = 0u32;
        if num_bits > self.bits_in_cache {
            let missing_bits = num_bits - self.bits_in_cache;
            if missing_bits < CACHE_BITS {
                value = self.cache << missing_bits;
            }
            self.cache = self.bit_buffer.get();
            self.bits_in_cache += CACHE_BITS;
        }
        self.bits_in_cache -= num_bits;
        (value | (self.cache >> self.bits_in_cache)) & bitbuffer::BIT_MASK[usize::from(num_bits)]
    }

    /// Returns a single bit from Bitstream
    pub fn read_bit(&mut self) -> u32 {
        if self.bits_in_cache == 0 {
            self.cache = self.bit_buffer.get();
            self.bits_in_cache = CACHE_BITS;
        }
        self.bits_in_cache -= 1;
        (self.cache >> self.bits_in_cache) & 0x1
    }

    /// read an integer value using a varying number of bits from the bitstream
    /// q.v. ISO/IEC 23003-3:2020  Table 19
    ///
    /// # Parameters
    ///
    /// - `num_bits1`: Number of bits to read for a small integer value or escape value
    /// - `num_bits2`: Number of bits to read for a medium sized integer value or escape value
    /// - `num_bits3`: Number of bits to read for a large integer value or escape value
    pub fn escaped_value(&mut self, num_bits1: u8, num_bits2: u8, num_bits3: u8) -> u32 {
        debug_assert!(num_bits1 <= 16 && num_bits2 <= 16 && num_bits3 <= 16);
        let mut value = self.read(num_bits1);
        if value == (1 << num_bits1) - 1 {
            let value_add = self.read(num_bits2);
            value += value_add;
            if value_add == (1 << num_bits2) - 1 {
                value += self.read(num_bits3);
            }
        }
        value
    }

    /// Writes a value with a certain number of bits to Bitstream
    ///
    /// # Parameters
    ///
    /// - `value`: A value to be written to Bitstream
    /// - `num_bits`: The number of bits to be written. Restricted to `[0; 32]`
    pub fn write(&mut self, value: u32, num_bits: u8) {
        debug_assert!(num_bits <= 32);
        debug_assert!(value <= bitbuffer::BIT_MASK[usize::from(num_bits)]);
        let value = value & bitbuffer::BIT_MASK[usize::from(num_bits)];
        if self.bits_in_cache + num_bits < CACHE_BITS {
            self.cache = (self.cache << num_bits) | value;
            self.bits_in_cache += num_bits;
        } else if self.bits_in_cache == 0 {
            self.bit_buffer.put(value, CACHE_BITS);
        } else {
            let missing_bits = CACHE_BITS - self.bits_in_cache;
            let remaining_bits = num_bits - missing_bits;
            self.bit_buffer.put(
                (self.cache << missing_bits) | (value >> remaining_bits),
                CACHE_BITS,
            );
            self.cache = value;
            self.bits_in_cache = remaining_bits;
        }
    }

    /// Drains the `cache` in Bitstream and synchronizes with underlying Bitbuffer
    pub fn sync(&mut self) {
        if self.mode == Mode::Reader {
            self.bit_buffer
                .push(-isize::from(self.bits_in_cache), bitbuffer::Mode::Reader);
        } else if self.bits_in_cache != 0 {
            self.bit_buffer.put(self.cache, self.bits_in_cache);
        }
        self.cache = 0;
        self.bits_in_cache = 0;
    }

    /// Applies byte alignment
    ///
    /// # Parameters
    ///
    /// - `anchor`: A bit position to which byte alignment gets applied
    pub fn align(&mut self, anchor: isize) {
        self.sync();
        if self.mode == Mode::Reader {
            let align_bits = (self.bit_buffer.valid_bits() - anchor) & 0x7;
            self.bit_buffer.push(align_bits, bitbuffer::Mode::Reader);
        } else {
            let align_bits = ((anchor - self.bit_buffer.valid_bits()) & 0x7) as u8;
            self.bit_buffer.put(0, align_bits);
        }
    }

    /// # Parameters
    ///
    /// - `input_buffer`: Byte buffer with input data to be taken from
    /// - `bytes_valid`: Number of bytes in `input_buffer`
    ///
    /// # Return
    /// Returns the number of ramaining valid bytes in extern input buffer
    pub fn feed(&mut self, input_buffer: &[u8], bytes_valid: usize) -> usize {
        self.sync();
        self.bit_buffer.feed(input_buffer, bytes_valid)
    }

    /// Push a certain number of bits in Bitstream back and forth
    ///
    /// # Parameters
    ///
    /// - `num_bits`: Negative sign means `push_back` and positive sign `push_for`
    pub fn push(&mut self, num_bits: isize) {
        self.sync();
        self.bit_buffer.push(
            num_bits,
            if self.mode == Mode::Reader {
                bitbuffer::Mode::Reader
            } else {
                bitbuffer::Mode::Writer
            },
        );
    }

    /// Push a certain number of bits in Bitstream backwards
    ///
    /// The `num_bits` needs to be less than the number of bits read since the last cache update.
    /// It's recommended to use `push_back_cache()` directly after `read()` with a known number of
    /// bits.
    ///
    /// # Parameters
    ///
    /// - `num_bits`: Number of bits to push backwards. Restricted to `[0; 32]`
    pub fn push_back_cache(&mut self, num_bits: u8) {
        debug_assert!(self.mode == Mode::Reader);
        debug_assert!((self.bits_in_cache + num_bits) <= CACHE_BITS);
        self.bits_in_cache += num_bits;
    }

    /// Returns the number of available bits in Bitstream
    pub fn valid_bits(&mut self) -> isize {
        self.sync();
        self.bit_buffer.valid_bits()
    }

    /// Returns the amount of bits that fit into Bitbuffer without overwriting unprocessed data
    pub fn free_bits(&mut self) -> usize {
        self.sync();
        self.bit_buffer.free_bits()
    }

    /// Returns a reference to buffer slice holding actual data
    pub fn buffer(&mut self) -> &[u8] {
        self.bit_buffer.buffer()
    }

    /// Returns the Bitstream operating mode
    pub fn mode(&mut self) -> Mode {
        self.mode
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default() {
        let mut bitstream: Bitstream = Default::default();

        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.mode, Mode::Reader);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 0);
    }

    #[test]
    fn init() {
        let mut bitstream = Bitstream::new(8, Mode::Writer);

        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.mode, Mode::Writer);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 0);
    }

    #[test]
    fn new() {
        let buffer = vec![0; 8];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 64);

        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.mode, Mode::Reader);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 64);
    }

    #[test]
    fn reset() {
        let buffer = vec![0; 8];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Writer);
        bitstream.init(&buffer, 48);
        bitstream.reset();

        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.mode, Mode::Writer);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 0);
    }

    #[test]
    fn read() {
        #[rustfmt::skip]
        let buffer = vec![
            0b0000_0000, 0b0100_1100, 0b0111_0000, 0b1111_0000, 0b0111_1100, 0b0000_1111, 0b1100_0000, 0b0111_1111,
        ];

        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 64);

        bitstream.cache = bitstream.bit_buffer.get();
        bitstream.bits_in_cache = 32 - 13;
        bitstream.sync();

        assert_eq!(bitstream.read_bit(), 0b1);
        assert_eq!(bitstream.read(7), 0b000_1110);
        assert_eq!(
            bitstream.read(32),
            u32::from_be_bytes([0b00011110, 0b00001111, 0b10000001, 0b11111000])
        );
    }

    #[test]
    fn write() {
        #[rustfmt::skip]
        let buffer_reference = [
            0b1000_0111, 0b1111_0000, 0b0000_0011, 0b1111_1111, 0b1110_0000, 0b0000_0000, 0b0001_1111, 0b1111_1111,
            0b1111_1100, 0b0000_0000, 0b0000_0000, 0b0000_1111, 0b1111_1111, 0b1111_1111, 0b1111_1000, 0b0000_0000,
            0b0000_0000, 0b0000_0000, 0b0111_1111, 0b1111_1111, 0b1111_1111, 0b1111_1111, 0b0000_0000, 0b0000_0000,
            0b0000_0000, 0b0000_0000, 0b0000_0000, 0b0000_0000, 0b0000_0000, 0b0000_0000, 0b0000_0000, 0b0000_0000,
        ];

        let mut bitstream = Bitstream::new(32, Mode::Writer);

        for num_bits in (1..=32).step_by(3) {
            let value = if num_bits & 1 != 0 {
                bitbuffer::BIT_MASK[usize::from(num_bits)]
            } else {
                0
            };
            bitstream.write(value, num_bits);
        }
        bitstream.sync();

        assert_eq!(bitstream.buffer(), buffer_reference);
    }

    #[test]
    #[should_panic]
    fn read_too_many_bits() {
        let buffer = vec![0; 4];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 32);
        bitstream.read(33);
    }

    #[test]
    fn sync_reader() {
        let buffer = [0, 1, 2, 3, 4, 5, 6, 7];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 64);

        bitstream.cache = bitstream.bit_buffer.get();
        bitstream.bits_in_cache = 16;
        assert_eq!(bitstream.cache, 0x00010203);

        bitstream.sync();
        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.bit_buffer.get().to_be_bytes(), [2, 3, 4, 5]);
        assert_eq!(bitstream.bit_buffer.get().to_be_bytes(), [6, 7, 0, 1]);
    }

    #[test]
    fn sync_writer() {
        let mut bitstream = Bitstream::new(4, Mode::Writer);

        bitstream.cache = 0xFFFF0203;
        bitstream.bits_in_cache = 16;
        bitstream.sync();

        assert_eq!(bitstream.cache, 0);
        assert_eq!(bitstream.bits_in_cache, 0);
        assert_eq!(bitstream.buffer(), [2, 3, 0, 0]);
    }

    #[test]
    fn align_read() {
        let buffer = vec![0; 4];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 32);

        bitstream.read(3);
        let anchor = bitstream.valid_bits();
        bitstream.align(anchor);
        assert_eq!(bitstream.valid_bits(), anchor);

        bitstream.read(2);
        bitstream.align(anchor);
        assert_eq!((bitstream.valid_bits() - anchor) % 8, 0);
    }

    #[test]
    fn align_write() {
        let mut bitstream = Bitstream::new(4, Mode::Writer);

        bitstream.write(0, 3);
        let anchor = bitstream.valid_bits();
        bitstream.align(anchor);
        assert_eq!(bitstream.valid_bits(), anchor);

        bitstream.write(0, 2);
        bitstream.align(anchor);
        assert_eq!((bitstream.valid_bits() - anchor) % 8, 0);
    }

    #[test]
    fn feed() {
        let in_buffer1 = [0, 1, 2, 3, 4, 5];
        let in_buffer2 = [6, 7];
        let mut bitstream = Bitstream::new(8, Mode::Reader);

        let mut in_len = in_buffer1.len();
        in_len = bitstream.feed(&in_buffer1, in_len);

        assert_eq!(in_len, 0);
        assert_eq!(bitstream.bit_buffer.get().to_be_bytes(), [0, 1, 2, 3]);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 16);

        bitstream.bits_in_cache = 32;
        let mut in_len = in_buffer2.len();
        in_len = bitstream.feed(&in_buffer2, in_len);

        assert_eq!(in_len, 0);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 64);
        assert_eq!(bitstream.bit_buffer.get().to_be_bytes(), [0, 1, 2, 3]);
        assert_eq!(bitstream.bit_buffer.get().to_be_bytes(), [4, 5, 6, 7]);
        assert_eq!(bitstream.bit_buffer.valid_bits(), 0);
    }

    #[test]
    fn push_reader() {
        let buffer = vec![0; 4];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 32);

        bitstream.read(31);
        assert_eq!(bitstream.valid_bits(), 1);
        bitstream.push(-29);
        assert_eq!(bitstream.valid_bits(), 30);
        bitstream.push(20);
        assert_eq!(bitstream.valid_bits(), 10);
    }

    #[test]
    fn push_writer() {
        let mut bitstream = Bitstream::new(4, Mode::Writer);

        bitstream.write(0, 31);
        assert_eq!(bitstream.valid_bits(), 31);
        bitstream.push(-29);
        assert_eq!(bitstream.valid_bits(), 2);
        bitstream.push(20);
        assert_eq!(bitstream.valid_bits(), 22);
    }

    #[test]
    fn push_back_cache() {
        #[rustfmt::skip]
        let buffer = vec![
            0b1010_1010, 0b1111_1111, 0b0000_0000, 0b1100_1100,
        ];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 32);
        assert_eq!(bitstream.read(8), 0b1010_1010);
        bitstream.push_back_cache(4);
        assert_eq!(bitstream.read(8), 0b1010_1111);
    }

    #[test]
    #[should_panic]
    #[cfg_attr(not(debug_assertions), ignore)]
    fn push_back_cache_too_many_bits() {
        let buffer = vec![0; 4];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 32);
        bitstream.read(1);
        bitstream.push_back_cache(2);
    }

    #[test]
    fn bit_states() {
        let buffer = vec![0; 4];
        let mut bitstream = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream.init(&buffer, 8);

        assert_eq!(bitstream.valid_bits(), 8);
        assert_eq!(bitstream.free_bits(), 24);
    }

    #[test]
    fn buffer() {
        let buffer: [u8; 4] = [0, 1, 2, 3];
        let mut bitstream = Bitstream::new(4, Mode::Reader);
        bitstream.init(&buffer, 32);

        assert_eq!(bitstream.buffer(), [0, 1, 2, 3]);
        assert_eq!(bitstream.buffer().len(), 4);
    }

    #[test]
    fn mode() {
        assert_eq!(Bitstream::new(4, Mode::Reader).mode(), Mode::Reader);
        assert_eq!(Bitstream::new(4, Mode::Writer).mode(), Mode::Writer);
    }
}
