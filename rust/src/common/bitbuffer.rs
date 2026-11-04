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
//! Bit buffer implementation

const MAX_BUFSIZE: usize = 1 << (i32::BITS - 2 - 3);

pub static BIT_MASK: [u32; 32 + 1] = [
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff,
];

/// Represents a Bitbuffer operation mode.
/// Can be configured either as bitbuffer reader or as bitbuffer writer.
#[derive(Debug, PartialEq, Eq)]
pub enum Mode {
    Reader,
    Writer,
}

#[derive(Default, PartialEq, Debug)]
#[repr(C)]
pub struct Bitbuffer {
    valid_bits: isize,
    feed_offset: usize,
    bit_index: usize,
    buffer: Vec<u8>,
}

impl Bitbuffer {
    /// Creates a new Bitbuffer instance
    ///
    /// # Parameters
    ///
    /// - `length`: Length of Bitbuffer in bytes (must be `2^n` and greater than zero length)
    ///
    /// # Return
    /// Returns a new Bitbuffer instance
    pub fn new(length: usize) -> Self {
        debug_assert!(length <= MAX_BUFSIZE, "too large buffer size");
        debug_assert!(
            length == 1 << length.trailing_zeros(),
            "buffer len must be 2^n"
        );
        Self {
            valid_bits: 0,
            feed_offset: 0,
            bit_index: 0,
            buffer: vec![0; length],
        }
    }

    /// Initializes the Bitbuffer with given input data
    ///
    /// # Parameters
    ///
    /// - `buffer`: Input data buffer
    /// - `valid_bits`: Number of valid bits in provided input buffer
    pub fn init(&mut self, buffer: &[u8], valid_bits: usize) {
        debug_assert!(
            valid_bits <= (self.buffer.len() << 3),
            "too many valid_bits"
        );
        debug_assert!(!buffer.is_empty(), "buffer is empty");

        if !buffer.is_empty() {
            self.buffer[..buffer.len()].copy_from_slice(buffer);
        }
        self.valid_bits = valid_bits.try_into().unwrap();
        self.feed_offset = 0;
        self.bit_index = 0;
    }

    /// Destroys the dynamic allocated Bitbuffer memory
    pub fn destroy(&mut self) {
        if self.buffer.capacity() > 0 {
            self.buffer.clear();
            self.buffer.shrink_to_fit();
        }
    }

    /// Resets all relevant Bitbuffer states
    pub fn reset(&mut self) {
        self.valid_bits = 0;
        self.feed_offset = 0;
        self.bit_index = 0;
    }

    /// Returns 32 sequential bits from Bitbuffer
    pub fn get(&mut self) -> u32 {
        let offset = (self.bit_index + 7) >> 3;
        let bits = self.bit_index & 0x7;
        self.push(32, Mode::Reader);

        let cache = self.cache_from_buffer(offset);
        if bits != 0 {
            (cache >> (8 - bits)) | (u32::from(self.buffer[offset - 1]) << (24 + bits))
        } else {
            cache
        }
    }

    /// Writes a value with a certain number of bits to Bitbuffer
    ///
    /// # Parameters
    ///
    /// - `value`: A value to be written to Bitbuffer
    /// - `num_bits`: The number of bits to be written. Restricted to `[0; 32]`
    pub fn put(&mut self, value: u32, num_bits: u8) {
        if num_bits != 0 {
            let offset = self.bit_index >> 3;
            let bits = self.bit_index & 0x7;
            self.push(isize::from(num_bits), Mode::Writer);

            let mut cache = self.cache_from_buffer(offset);
            cache = (cache & !((BIT_MASK[usize::from(num_bits)] << (32 - num_bits)) >> bits))
                | ((value << (32 - num_bits)) >> bits);
            self.cache_to_buffer(cache, offset);

            if bits + usize::from(num_bits) > 32 {
                let offset = self.bit_index >> 3;
                let bits = self.bit_index & 0x7;
                self.buffer[offset] = ((u32::from(self.buffer[offset])
                    & (!(BIT_MASK[bits] << (8 - bits))))
                    | (value << (8 - bits))) as u8;
            }
        }
    }

    /// Feeds input data into Bitbuffer
    ///
    /// # Parameters
    ///
    /// - `input_buffer`: Byte buffer with input data to be taken from
    /// - `bytes_valid`: Number of bytes in `input_buffer`
    ///
    /// # Return
    /// Returns the number of ramaining valid bytes in extern input buffer
    pub fn feed(&mut self, input_buffer: &[u8], bytes_valid: usize) -> usize {
        let mut src_offset = input_buffer.len() - bytes_valid;
        let mut dst_offset = self.feed_offset;

        let bytes_total =
            self.buffer.len() - ((self.valid_bits.max(0) as usize + 7) >> 3).min(self.buffer.len());
        let bytes_total = bytes_total.min(bytes_valid);
        let mut bytes_left = bytes_total;

        while bytes_left > 0 {
            let bytes_to_read = (self.buffer.len() - dst_offset).min(bytes_left);
            self.buffer[dst_offset..dst_offset + bytes_to_read]
                .copy_from_slice(&input_buffer[src_offset..src_offset + bytes_to_read]);

            src_offset += bytes_to_read;
            dst_offset = (dst_offset + bytes_to_read) & (self.buffer.len() - 1);
            bytes_left -= bytes_to_read;
        }

        self.feed_offset = dst_offset;
        self.valid_bits += isize::try_from(bytes_total << 3).unwrap();

        bytes_valid - bytes_total
    }

    /// Push a certain number of bits in Bitbuffer back and forth
    ///
    /// The members `bit_index` and `valid_bits` gets updated depending on chosen operation `mode`.
    /// Modulo buffer addressing is considered for `bit_index` while `valid_bits` can become
    /// negative.
    ///
    /// # Parameters
    ///
    /// - `num_bits`: Negative sign means `push_back` and positive sign `push_for`
    /// - `mode`: Parameter specifies whether the bitbuffer operate as `reader` or `writer`
    pub fn push(&mut self, num_bits: isize, mode: Mode) {
        self.bit_index =
            (self.bit_index as isize + num_bits) as usize & ((self.buffer.len() << 3) - 1);
        self.valid_bits += if mode == Mode::Reader {
            -num_bits
        } else {
            num_bits
        };
    }

    /// Returns the number of available bits in Bitbuffer
    pub fn valid_bits(&mut self) -> isize {
        self.valid_bits
    }

    /// Returns the amount of bits that fit into Bitbuffer without overwriting unprocessed data
    pub fn free_bits(&mut self) -> usize {
        (self.buffer.len() << 3) - (self.valid_bits.max(0) as usize).min(self.buffer.len() << 3)
    }

    /// Returns a reference to buffer slice holding actual data
    pub fn buffer(&mut self) -> &[u8] {
        &self.buffer
    }

    /// Read 4 bytes from buffer and return u32 value
    fn cache_from_buffer(&mut self, offset: usize) -> u32 {
        if offset + 3 < self.buffer.len() {
            u32::from_be_bytes(self.buffer[offset..offset + 4].try_into().unwrap())
        } else {
            let mask = self.buffer.len() - 1;
            u32::from_be_bytes([
                self.buffer[offset & mask],
                self.buffer[(offset + 1) & mask],
                self.buffer[(offset + 2) & mask],
                self.buffer[(offset + 3) & mask],
            ])
        }
    }

    /// Write u32 value to byte buffer
    fn cache_to_buffer(&mut self, cache: u32, offset: usize) {
        if offset + 3 < self.buffer.len() {
            self.buffer[offset..offset + 4].copy_from_slice(&cache.to_be_bytes());
        } else {
            let mask = self.buffer.len() - 1;
            let cache_slice: [u8; 4] = cache.to_be_bytes();
            self.buffer[offset & mask] = cache_slice[0];
            self.buffer[(offset + 1) & mask] = cache_slice[1];
            self.buffer[(offset + 2) & mask] = cache_slice[2];
            self.buffer[(offset + 3) & mask] = cache_slice[3];
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default() {
        let bitbuffer: Bitbuffer = Default::default();

        assert_eq!(bitbuffer.valid_bits, 0);
        assert_eq!(bitbuffer.feed_offset, 0);
        assert_eq!(bitbuffer.bit_index, 0);
        assert!(bitbuffer.buffer.is_empty());
    }

    #[test]
    fn init() {
        let buffer = vec![0; 8];
        let mut bitbuffer = Bitbuffer::new(8);
        bitbuffer.init(&buffer, 48);

        assert_eq!(bitbuffer.valid_bits, 48);
        assert_eq!(bitbuffer.feed_offset, 0);
        assert_eq!(bitbuffer.bit_index, 0);
        assert_eq!(bitbuffer.buffer.len(), 8);
    }

    #[test]
    fn new() {
        let bitbuffer = Bitbuffer::new(8);

        assert_eq!(bitbuffer.valid_bits, 0);
        assert_eq!(bitbuffer.feed_offset, 0);
        assert_eq!(bitbuffer.bit_index, 0);
        assert_eq!(bitbuffer.buffer.len(), 8);
    }

    #[test]
    #[should_panic(expected = "too many valid_bits")]
    #[cfg_attr(not(debug_assertions), ignore)]
    fn init_check_valid_bits() {
        let buffer = vec![0; 16];
        let mut bitbuffer = Bitbuffer::new(8);
        bitbuffer.init(&buffer, 128);
    }

    #[test]
    #[should_panic(expected = "buffer is empty")]
    #[cfg_attr(not(debug_assertions), ignore)]
    fn init_check_buffer_size() {
        let buffer = vec![0; 0];
        let mut bitbuffer = Bitbuffer::default();
        bitbuffer.init(&buffer, 0);
    }

    #[test]
    #[should_panic(expected = "buffer len must be 2^n")]
    #[cfg_attr(not(debug_assertions), ignore)]
    fn init_check_buffer_size_power_of_two() {
        let _bitbuffer = Bitbuffer::new(7);
    }

    #[test]
    fn reset() {
        let buffer = vec![0; 8];
        let mut bitbuffer = Bitbuffer::new(8);
        bitbuffer.init(&buffer, 16);
        bitbuffer.reset();

        assert_eq!(bitbuffer.valid_bits, 0);
        assert_eq!(bitbuffer.feed_offset, 0);
        assert_eq!(bitbuffer.bit_index, 0);
    }

    #[test]
    fn get() {
        let buffer = [0, 1, 2, 3, 4, 5, 6, 7];
        let mut bitbuffer = Bitbuffer::new(8);
        bitbuffer.init(&buffer, 64);

        assert_eq!(bitbuffer.get().to_be_bytes(), [0, 1, 2, 3]);
        bitbuffer.push(16, Mode::Reader);

        assert_eq!(bitbuffer.get().to_be_bytes(), [6, 7, 0, 1]);
        assert_eq!(bitbuffer.bit_index, 16);
        assert_eq!(bitbuffer.valid_bits, -16);
    }

    #[test]
    fn put() {
        let mut bitbuffer = Bitbuffer::new(8);

        // Test case: Call put() method on different unaligned bit_index positions with
        // up to 32 num_bits. As a result, the buffer must be completely zeroed out.
        bitbuffer.put(0, 7);
        bitbuffer.put(0, 32);
        bitbuffer.put(0, 25);

        assert!(bitbuffer.buffer() == vec![0; 8]);

        // Test case: Call put() method starting with a bit_index offset. The bitbuffer
        // used as a circular buffer must correctly wrap around.
        bitbuffer.reset();
        bitbuffer.bit_index = 32;
        bitbuffer.valid_bits = 32;

        for value in 0..8 {
            bitbuffer.put(value, 3);
            bitbuffer.put(0, 5);
        }

        assert_eq!(bitbuffer.buffer(), [128, 160, 192, 224, 0, 32, 64, 96]);
    }

    #[test]
    fn feed() {
        let in_buffer = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        let mut in_len = in_buffer.len();

        let mut bitbuffer = Bitbuffer::new(4);
        let out_len = bitbuffer.buffer().len();
        in_len = bitbuffer.feed(&in_buffer, in_len);
        assert_eq!(in_len, in_buffer.len() - bitbuffer.buffer.len());
        assert_eq!(bitbuffer.get().to_be_bytes(), [0, 1, 2, 3]);
        assert_eq!(bitbuffer.feed_offset, 0);

        in_len = bitbuffer.feed(&in_buffer, in_len);
        assert_eq!(in_len, in_buffer.len() - 2 * bitbuffer.buffer.len());
        assert_eq!(bitbuffer.get().to_be_bytes(), [4, 5, 6, 7]);
        assert_eq!(bitbuffer.valid_bits, 0);
        assert_eq!(bitbuffer.feed_offset, 0);

        in_len = bitbuffer.feed(&in_buffer, in_len);
        assert_eq!(in_len, 0);
        assert_eq!(bitbuffer.valid_bits, 16);
        assert_eq!(bitbuffer.get().to_be_bytes(), [8, 9, 6, 7]);
        assert_eq!(bitbuffer.valid_bits, -16);
        assert_eq!(bitbuffer.feed_offset, 2);

        in_len = in_buffer.len();
        let mut bitbuffer = Bitbuffer::new(out_len);
        in_len = bitbuffer.feed(&in_buffer, in_len);
        bitbuffer.push(11, Mode::Reader);
        in_len = bitbuffer.feed(&in_buffer, in_len);
        assert_eq!(in_len, in_buffer.len() - out_len - 11 / 8);
        assert_eq!(bitbuffer.valid_bits, 8 * out_len as isize - 11 % 8);
    }

    #[test]
    fn push() {
        let mut bitbuffer = Bitbuffer::new(8);

        bitbuffer.push(32, Mode::Writer);
        assert_eq!(bitbuffer.bit_index, 32);
        assert_eq!(bitbuffer.valid_bits, 32);

        bitbuffer.push(-8, Mode::Reader);
        assert_eq!(bitbuffer.bit_index, 24);
        assert_eq!(bitbuffer.valid_bits, 40);
    }

    #[test]
    fn bit_states() {
        let buffer = vec![0; 8];
        let mut bitbuffer = Bitbuffer::new(8);
        bitbuffer.init(&buffer, 16);

        assert_eq!(bitbuffer.valid_bits(), 16);
        assert_eq!(bitbuffer.free_bits(), 48);
    }

    #[test]
    fn buffer() {
        let buffer = [0, 1, 2, 3];
        let mut bitbuffer = Bitbuffer::new(4);
        bitbuffer.init(&buffer, 32);

        assert_eq!(bitbuffer.buffer(), [0, 1, 2, 3]);
        assert_eq!(bitbuffer.buffer().len(), 4);
    }
}
