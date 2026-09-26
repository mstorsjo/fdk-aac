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
//! Cyclic redundancy check (CRC) module
//!
//! CRC in the decoder is used for error detection of parts of the bitstream.
use super::bitstream::{Bitstream, Mode};

const MAX_CRC_REGS: usize = 3;

// Precalculated lookup table for CRC polynomial x^16 + x^15 + x^2 + x^0
static CRC_LOOKUP_16_15_2_0: [u16; 256] = [
    0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011, 0x8033, 0x0036, 0x003c, 0x8039,
    0x0028, 0x802d, 0x8027, 0x0022, 0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
    0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041, 0x80c3, 0x00c6, 0x00cc, 0x80c9,
    0x00d8, 0x80dd, 0x80d7, 0x00d2, 0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
    0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1, 0x8093, 0x0096, 0x009c, 0x8099,
    0x0088, 0x808d, 0x8087, 0x0082, 0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
    0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1, 0x01e0, 0x81e5, 0x81ef, 0x01ea,
    0x81fb, 0x01fe, 0x01f4, 0x81f1, 0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
    0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151, 0x8173, 0x0176, 0x017c, 0x8179,
    0x0168, 0x816d, 0x8167, 0x0162, 0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
    0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101, 0x8303, 0x0306, 0x030c, 0x8309,
    0x0318, 0x831d, 0x8317, 0x0312, 0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
    0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371, 0x8353, 0x0356, 0x035c, 0x8359,
    0x0348, 0x834d, 0x8347, 0x0342, 0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
    0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2, 0x83a3, 0x03a6, 0x03ac, 0x83a9,
    0x03b8, 0x83bd, 0x83b7, 0x03b2, 0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
    0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291, 0x82b3, 0x02b6, 0x02bc, 0x82b9,
    0x02a8, 0x82ad, 0x82a7, 0x02a2, 0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
    0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1, 0x8243, 0x0246, 0x024c, 0x8249,
    0x0258, 0x825d, 0x8257, 0x0252, 0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
    0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231, 0x8213, 0x0216, 0x021c, 0x8219,
    0x0208, 0x820d, 0x8207, 0x0202,
];

static CRC_LOOKUP_32_IEEE: [u32; 256] = [
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
];

#[derive(Default, Debug)]
#[repr(C)]
/// CRC region data
///
/// Structure describing a single CRC region used for CRC calculation.
/// Substructure of CrcInfo.
struct CrcRegionData {
    is_active: bool,
    max_bits: i32,
    bitbuf_count_bits: isize,
    valid_bits: isize,
}

#[derive(Default, Debug)]
#[repr(C)]
/// CRC info structure
///
/// This is the main CRC structure.
pub struct CrcInfo {
    // Multiple CRC region descriptions
    region_data: [CrcRegionData; MAX_CRC_REGS],
    // Pointer to lookup table filled in CrcInfo::init()
    lookup_table: &'static [u16],
    // Pointer to 32bit lookup table filled in CrcInfo::init()
    lookup_table_32bit: &'static [u32],
    // CRC generator polynomial
    poly: u32,
    // CRC mask
    mask: u32,
    // CRC start value
    start_value: u32,
    // CRC length
    length: u8,
    // Start region marker for synchronization
    region_start: usize,
    // Stop region marker for synchronization
    region_stop: usize,
    // CRC value to be calculated
    value: u32,
}

impl CrcInfo {
    /// Create a CrcInfo instance
    pub fn new() -> CrcInfo {
        Default::default()
    }

    /// Initialize CrcInfo structure using the passed parameters
    ///
    /// # Parameters
    ///
    /// - `poly`: Polynomial data for CRC
    /// - `start_value`: Start value for CRC
    /// - `length`: length for CRC
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::crc::CrcInfo;
    ///
    /// let mut crc_info = CrcInfo::new();
    /// let poly = 0x8005;
    /// let start_value = 0xFFFF;
    /// let length = 16;
    ///
    /// crc_info.init(poly, start_value, length);
    /// ```
    pub fn init(&mut self, poly: u32, start_value: u32, length: u8) {
        // CRC polynomial example:
        // x^16 + x^15 + x^2 + x^0        (1) 1000 0000 0000 0101 -> 0x8005

        self.length = length;
        self.poly = poly;
        self.start_value = start_value;
        self.mask = if length != 0 && length <= 16 {
            1 << (length - 1)
        } else {
            0
        };
        self.reset();

        // Preset None for unknown 16-bit polynomial "poly"
        self.lookup_table = &[];

        // Preset None for unknown 32-bit polynomial "poly"
        self.lookup_table_32bit = &[];

        if length == 16 {
            // match is used as other lookup tables might be added in the future
            self.lookup_table = match poly {
                0x8005 => &CRC_LOOKUP_16_15_2_0,
                _ => &[],
            };
        } else if length == 32 {
            self.lookup_table_32bit = match poly {
                0xEDB88320 => &CRC_LOOKUP_32_IEEE,
                _ => &[],
            };
        }
    }

    /// Reset CrcInfo structure
    ///
    /// This function clears all internal states of the CrcInfo structure.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::crc::CrcInfo;
    ///
    /// let mut crc_info = CrcInfo::new();
    /// crc_info.reset();
    /// ```
    pub fn reset(&mut self) {
        self.value = self.start_value;
        for i in 0..MAX_CRC_REGS {
            self.region_data[i].is_active = false;
        }
        self.region_start = 0;
        self.region_stop = 0;
    }

    /// Start a CRC region with maximum number of bits
    ///
    /// This function marks position in bitstream to be used as start point for CRC
    /// calculation. Bitstream range for CRC calculation can be limited or kept
    /// dynamic depending on `max_bits` parameter. The CRC region has to be terminated
    /// with `end_region()` in each case.
    ///                    - `max_bits` > 0: Zero padding will be used for CRC calculation, if there
    ///                      are less than `max_bits` bits available.
    ///                    - `max_bits` < 0: No zero padding is done.
    ///                    - `max_bits` = 0: The number of bits used in CRC calculation is dynamic,
    ///                      depending on bitstream position between `start_region()` and
    ///                      `end_region()` call
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `max_bits`: Maximum number of bits for CRC start region data
    ///
    /// Returns initialized region value
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::bitstream::{Bitstream, Mode};
    /// use aac::common::crc::CrcInfo;
    ///
    /// // Precondition: There has to be a valid Bitstream instance.
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 50);
    ///
    /// let mut crc_info = CrcInfo::new();
    /// let max_bits = 50;
    ///
    /// let region = crc_info.start_region(&mut bitstream_reader, max_bits);
    /// ```
    pub fn start_region(&mut self, bs: &mut Bitstream, max_bits: i32) -> usize {
        let region = self.region_start;
        debug_assert!(!self.region_data[region].is_active);
        self.region_data[region].is_active = true;
        self.region_data[region].max_bits = max_bits;
        self.region_data[region].valid_bits = bs.valid_bits();
        self.region_data[region].bitbuf_count_bits = 0;
        self.region_start = (self.region_start + 1) % MAX_CRC_REGS;
        region
    }

    /// Calculates CRC for specified `region` ID and ends it
    ///
    /// This function terminates a CRC region initialized by `start_region()`.
    /// The number of bits in a CRC region depends on the `max_bits` parameter
    /// of `start_region()`.
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `region`: Region ID in CRC region data (0 to 2)
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::bitstream::{Bitstream, Mode};
    /// use aac::common::crc::CrcInfo;
    ///
    /// // Precondition, should have a valid Bitstream instance.
    /// let buffer = vec![0; 64];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 510);
    ///
    /// let mut crc_info = CrcInfo::new();
    /// let max_bits = 510;
    /// // Set region to active mode by calling start_region()
    /// let region = crc_info.start_region(&mut bitstream_reader, max_bits);
    /// let error = crc_info.end_region(&mut bitstream_reader, region);
    /// ```
    pub fn end_region(&mut self, bs: &mut Bitstream, region: usize) {
        debug_assert!(region == self.region_stop && self.region_data[region].is_active);

        self.region_data[region].bitbuf_count_bits = if bs.mode() == Mode::Writer {
            bs.valid_bits() - self.region_data[region].valid_bits
        } else {
            self.region_data[region].valid_bits - bs.valid_bits()
        };

        if self.region_data[region].max_bits == 0 {
            self.region_data[region].max_bits = self.region_data[region].bitbuf_count_bits as i32;
            // "as" cast is possible without overflowing due to common::bitbuffer::MAX_BUFSIZE.
        }

        self.process(bs, region);
        self.region_data[region].is_active = false;
        self.region_stop = (self.region_stop + 1) % MAX_CRC_REGS;
    }

    /// Calculate CRC based on configured number of maximum bits in CrcInfo
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `region`: CRC region ID in CrcInfo
    fn process(&mut self, bs: &mut Bitstream, region: usize) {
        let reg_data = &self.region_data[region];

        let bs_mode = bs.mode();
        let valid_bits = bs.valid_bits();
        if bs_mode == Mode::Writer {
            let mut bs_reader_instance = Bitstream::new(bs.buffer().len(), Mode::Reader);
            bs_reader_instance.init(bs.buffer(), valid_bits.try_into().unwrap());
            bs_reader_instance.push(reg_data.valid_bits);
            self.calc(&mut bs_reader_instance, region);
            bs_reader_instance.destroy();
        } else {
            bs.push(-(reg_data.valid_bits - valid_bits));
            self.calc(bs, region);
            let curr_valid_bits = bs.valid_bits();
            // Push remaining bits back to bitstream to adjust bit_index to be correct.
            bs.push(curr_valid_bits - valid_bits);
        }
    }

    /// Calculate CRC based on configured number of maximum bits in CrcInfo
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `region`: CRC region ID in CrcInfo
    fn calc(&mut self, bs: &mut Bitstream, region: usize) {
        let reg_data = &self.region_data[region];

        let mut remaining_bits = if reg_data.max_bits >= 0 {
            reg_data.max_bits
        } else {
            -reg_data.max_bits
        } as isize;

        let bits =
            if reg_data.max_bits > 0 && ((reg_data.bitbuf_count_bits >> 3 << 3) < remaining_bits) {
                reg_data.bitbuf_count_bits
            } else {
                remaining_bits
            };

        let words = bits >> 3; // Processing bytes
        let mbits = bits & 0x07; // Module bits

        if !self.lookup_table.is_empty() {
            remaining_bits -= self.calc_bytes(bs, words, false) << 3;
        } else if !self.lookup_table_32bit.is_empty() {
            remaining_bits -= self.calc_bytes_32bit(bs, words) << 3;
        } else {
            remaining_bits -= self.calc_bits(bs, words << 3, false);
        }

        // Remaining valid bits
        if mbits != 0 {
            remaining_bits -= self.calc_bits(bs, mbits, false);
        }
        if remaining_bits != 0 {
            // Zero bytes
            if !self.lookup_table.is_empty() && remaining_bits > 8 {
                remaining_bits -= self.calc_bytes(bs, remaining_bits >> 3, true) << 3;
            }

            // Remaining zero bits
            if remaining_bits != 0 {
                self.calc_bits(bs, remaining_bits, true);
            }
        }
    }

    /// Calculate CRC starting at current bitstream position over num_bytes.
    /// Works only with 16 bit CRCs.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `num_bytes`: Number of processing bytes
    /// - `is_zero_padding`: Flag to indicate zero padding of input data
    ///
    /// Returns number of processed bytes
    fn calc_bytes(&mut self, bs: &mut Bitstream, num_bytes: isize, is_zero_padding: bool) -> isize {
        let mut crc = self.value as u16; // Cuts away upper 16bits - this is intended.

        if is_zero_padding {
            for _ in 0..num_bytes {
                crc = (crc << 8) ^ self.lookup_table[usize::from((crc >> 8) & 0xFF)];
            }
        } else {
            for _ in 0..(num_bytes >> 2) {
                let data = bs.read(32).to_be_bytes();
                crc = (crc << 8) ^ self.lookup_table[usize::from((crc >> 8) as u8 ^ data[0])];
                crc = (crc << 8) ^ self.lookup_table[usize::from((crc >> 8) as u8 ^ data[1])];
                crc = (crc << 8) ^ self.lookup_table[usize::from((crc >> 8) as u8 ^ data[2])];
                crc = (crc << 8) ^ self.lookup_table[usize::from((crc >> 8) as u8 ^ data[3])];
            }
            let mut bits = (num_bytes & 3) << 3; // 0, 8, 16, 24
            if bits > 0 {
                let data = bs.read(bits.try_into().unwrap());
                bits -= 8;
                while bits >= 0 {
                    crc = (crc << 8)
                        ^ self.lookup_table
                            [usize::from(((crc >> 8) ^ (data >> bits) as u16) & 0xFF)];
                    bits -= 8;
                }
            }
        }
        // Update CRC value
        self.value = crc as u32;
        num_bytes
    }

    /// Calculate CRC starting at current bitstream position over num_bytes.
    /// Works only with 32 bit CRCs.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `num_bytes`: Number of processing bytes
    ///
    /// Returns number of processed bytes
    fn calc_bytes_32bit(&mut self, bs: &mut Bitstream, num_bytes: isize) -> isize {
        let mut crc = self.value;
        let mut data: u8;

        for _ in 0..(num_bytes >> 2) {
            data = bs.read(8) as u8;
            crc = (crc >> 8) ^ self.lookup_table_32bit[usize::from(crc as u8 ^ data) & 0xff];
            data = bs.read(8) as u8;
            crc = (crc >> 8) ^ self.lookup_table_32bit[usize::from(crc as u8 ^ data) & 0xff];
            data = bs.read(8) as u8;
            crc = (crc >> 8) ^ self.lookup_table_32bit[usize::from(crc as u8 ^ data) & 0xff];
            data = bs.read(8) as u8;
            crc = (crc >> 8) ^ self.lookup_table_32bit[usize::from(crc as u8 ^ data) & 0xff];
        }
        let mut bits = (num_bytes & 3) << 3;
        if bits > 0 {
            while bits >= 8 {
                data = bs.read(8) as u8;
                crc = (crc >> 8) ^ self.lookup_table_32bit[usize::from(crc as u8 ^ data) & 0xff];
                bits -= 8;
            }
        }
        // Update CRC value
        self.value = crc;
        num_bytes
    }

    /// Calculate CRC starting at current bitstream position over num_bits
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    /// - `num_bits`: Number of processing bits
    /// - `is_zero_padding`: Flag to indicate zero padding of input data
    ///
    /// Returns number of processed bits
    fn calc_bits(&mut self, bs: &mut Bitstream, num_bits: isize, is_zero_padding: bool) -> isize {
        let mut crc = self.value;
        let crc_out_mask = ((1u64 << self.length as usize) - 1) as u32;

        if is_zero_padding {
            for _ in 0..num_bits {
                let tmp = if crc & self.mask != 0 { self.poly } else { 0 };
                crc <<= 1;
                crc ^= tmp;
            }
        } else {
            for _ in 0..num_bits {
                let tmp = if crc & self.mask != 0 { 1_u32 } else { 0 };
                crc = if tmp ^ (bs.read_bit()) != 0 {
                    crc <<= 1;
                    crc ^ self.poly
                } else {
                    crc << 1
                };
            }
        }
        // Update CRC value
        self.value = crc & crc_out_mask;
        num_bits
    }

    /// Returns masked CRC value from CrcInfo
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::crc::CrcInfo;
    ///
    /// let mut crc_info = CrcInfo::new();
    /// let poly = 0x8005;
    /// let start_value = 0xFFFF;
    /// let length = 16;
    ///
    /// crc_info.init(poly, start_value, length);
    /// let crc = crc_info.value();
    /// ```
    pub fn value(&self) -> u32 {
        if self.mask != 0 {
            self.value & (((self.mask - 1) << 1) + 1)
        } else {
            self.value
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new() {
        let crc_info = CrcInfo::new();

        for reg in 0..MAX_CRC_REGS {
            assert!(!crc_info.region_data[reg].is_active);
            assert_eq!(crc_info.region_data[reg].max_bits, 0);
            assert_eq!(crc_info.region_data[reg].bitbuf_count_bits, 0);
            assert_eq!(crc_info.region_data[reg].valid_bits, 0);
        }

        assert_eq!(crc_info.lookup_table, &[] as &[u16]);
        assert_eq!(crc_info.poly, 0);
        assert_eq!(crc_info.mask, 0);
        assert_eq!(crc_info.start_value, 0);
        assert_eq!(crc_info.length, 0);
        assert_eq!(crc_info.region_start, 0);
        assert_eq!(crc_info.region_stop, 0);
        assert_eq!(crc_info.value, 0);
    }

    #[test]
    fn reset() {
        let mut crc_info = CrcInfo::new();

        assert_eq!(crc_info.start_value, 0);
        assert!(!crc_info.region_data[0].is_active);
        assert!(!crc_info.region_data[1].is_active);
        assert!(!crc_info.region_data[2].is_active);
        assert_eq!(crc_info.region_start, 0);
        assert_eq!(crc_info.region_stop, 0);

        // Assign dummy values for testing
        crc_info.start_value = 0x1234;
        crc_info.region_start = 0x5678;
        crc_info.region_stop = 0x91011;
        crc_info.region_data[0].is_active = true;
        crc_info.region_data[1].is_active = true;
        crc_info.region_data[2].is_active = true;

        crc_info.reset();

        assert_eq!(crc_info.start_value, 0x1234);
        assert!(!crc_info.region_data[0].is_active);
        assert!(!crc_info.region_data[1].is_active);
        assert!(!crc_info.region_data[2].is_active);
        assert_eq!(crc_info.region_start, 0);
        assert_eq!(crc_info.region_stop, 0);
    }

    #[test]
    fn crc_value() {
        let mut crc_info = CrcInfo::new();
        crc_info.value = 0x1234;
        crc_info.mask = 1 << (16 - 1);
        // let ref_crc = 0x1234_u16 & (((1 << 15) - 1 << 1) + 1);
        let ref_crc = 0x1234_u32;
        let calc_crc = crc_info.value();
        assert_eq!(ref_crc, calc_crc);
    }

    #[test]
    fn start_region() {
        let buffer = vec![0x12, 0x34, 0x56, 0x78];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 30);

        let mut crc_info = CrcInfo::new();
        let max_bits = 35;
        crc_info.region_start = 1;
        let region = crc_info.start_region(&mut bitstream_reader, max_bits);

        assert_eq!(region, 1);
        assert!(crc_info.region_data[region].is_active);
        assert_eq!(crc_info.region_data[region].max_bits, max_bits);
        assert_eq!(crc_info.region_data[region].valid_bits, 30);
        assert_eq!(crc_info.region_data[region].bitbuf_count_bits, 0);
        assert_eq!(crc_info.region_start, 2);
    }

    #[test]
    fn init() {
        let mut crc_info = CrcInfo::new();
        let length = 16;
        let start_val = 0x1234;

        // CASE 1: polynomial - lookup table available
        let poly_valid = 0x8005;
        crc_info.init(poly_valid, start_val, length);

        assert_eq!(crc_info.length, length);
        assert_eq!(crc_info.poly, poly_valid);
        assert_eq!(crc_info.start_value, start_val);
        assert_eq!(crc_info.mask, 32768);
        assert_eq!(crc_info.lookup_table, &CRC_LOOKUP_16_15_2_0);

        // CASE 2: polynomial - lookup table unavailable
        let poly_valid = 0x8000;
        crc_info.init(poly_valid, start_val, length);
        assert_eq!(crc_info.lookup_table, &[] as &[u16]);

        // CASE 3: length != 16,
        let length = 15;
        crc_info.init(poly_valid, start_val, length);
        assert_eq!(crc_info.lookup_table, &[] as &[u16]);
    }

    #[test]
    pub fn decode() {
        // Test case: Decode CRC over 56 bits (e.g. ADTS header) with/without available lookup table
        {
            // poly - 0x8005 lookup table available
            // poly - 0x1234 lookup table not available
            let crc = [0x6dce, 0x7288];
            let poly = [0x8005, 0x1234];
            let max_bits = 0;
            let length: u8 = 16;
            let start_val = 0xFFFF;

            for (crc, poly_valid) in crc.iter().zip(poly.iter()) {
                let mut crc_info = CrcInfo::new();

                crc_info.init(*poly_valid, start_val, length);

                let buffer = vec![0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00];
                let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
                bitstream_reader.init(&buffer, 56);

                crc_info.region_start = 0;
                let region = crc_info.start_region(&mut bitstream_reader, max_bits);
                bitstream_reader.push(56);

                crc_info.end_region(&mut bitstream_reader, region);
                assert!(crc_info.value() == *crc);
            }
        }

        // Test case: Decode CRC over 192 bits (e.g. SCE, CPE, ...) with available lookup table and
        // max_bits set
        {
            let mut crc_info = CrcInfo::new();
            let length: u8 = 16;
            let start_val = 0xFFFF_u32;

            let poly_valid = 0x1021;
            crc_info.init(poly_valid, start_val, length);

            let buffer: Vec<u8> = vec![
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14,
                0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
            ];
            let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
            bitstream_reader.init(&buffer, 256);

            let max_bits = 192;
            crc_info.region_start = 0;
            let region = crc_info.start_region(&mut bitstream_reader, max_bits);
            bitstream_reader.push(192);

            crc_info.end_region(&mut bitstream_reader, region);
            assert!(crc_info.value() == 0x96BB);
        }

        // Test case: Decode CRC over 192 bits (e.g. SCE, CPE, ...) with/without available lookup
        // table and with zero padding
        {
            // poly - 0x8005 lookup table available
            // poly - 0x5610 lookup table not available
            let crc = [0x8A99, 0x5F30];
            let poly = [0x8005, 0x5610];
            let length: u8 = 16;

            for (crc, poly_valid) in crc.iter().zip(poly.iter()) {
                let (start_val, max_bits, valid_bits, buffer) = if *poly_valid == 0x8005 {
                    (
                        0xFFFF_u32,
                        128_i32,
                        64_usize,
                        vec![0xFF_u8, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF0, 0xF9],
                    )
                } else {
                    (
                        0x0000_u32,
                        192_i32,
                        79_usize,
                        vec![
                            0x01_u8, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11,
                            0x12, 0x13, 0x14, 0xFF, 0xFF,
                        ],
                    )
                };

                let mut crc_info = CrcInfo::new();
                let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
                bitstream_reader.init(&buffer, valid_bits);

                crc_info.init(*poly_valid, start_val, length);
                crc_info.region_start = 0;

                let region = crc_info.start_region(&mut bitstream_reader, max_bits);
                bitstream_reader.push(valid_bits.try_into().unwrap());

                crc_info.end_region(&mut bitstream_reader, region);
                assert!(crc_info.value() == *crc);
            }
        }
    }

    #[test]
    pub fn encode() {
        // Test case: Encode CRC over 56 bits (e.g. ADTS header) with available lookup table
        {
            let mut crc_info = CrcInfo::new();
            let length = 16;
            let start_val = 0xFFFF;

            let poly_valid = 0x8005;
            crc_info.init(poly_valid, start_val, length);

            let mut bitstream_writer = Bitstream::new(128, Mode::Writer);

            let max_bits = 0;
            crc_info.region_start = 0;

            let region = crc_info.start_region(&mut bitstream_writer, max_bits);

            bitstream_writer.write(0xfff05080, 32);
            bitstream_writer.write(0x2e6244, 24);

            crc_info.end_region(&mut bitstream_writer, region);
            assert!(crc_info.value() == 0xFE76);
        }
    }

    #[test]
    pub fn decode_overlapping_region() {
        // Test case: Decode CRC over 2 overlapping region ( region 0 without zero padding, region 1
        // with zero padding )
        {
            // poly - 0x8005 lookup table available
            let reg0_crc = 0xFFB3;
            let reg1_crc = 0x5EAD;
            let poly = 0x8005;
            let max_bits_reg0 = 40;
            let max_bits_reg1 = 56;
            let length: u8 = 16;
            let start_val = 0xFFFF;

            let mut crc_info = CrcInfo::new();

            crc_info.init(poly, start_val, length);

            let buffer = vec![0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x00];
            let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
            bitstream_reader.init(&buffer, 64);

            crc_info.region_start = 0;
            let reg0 = crc_info.start_region(&mut bitstream_reader, max_bits_reg0);
            bitstream_reader.push(24);

            let reg1 = crc_info.start_region(&mut bitstream_reader, max_bits_reg1);
            bitstream_reader.push(16);
            let bs_reader = &mut bitstream_reader;
            crc_info.end_region(bs_reader, reg0);
            assert!(crc_info.value() == reg0_crc);

            bitstream_reader.push(8);
            crc_info.end_region(&mut bitstream_reader, reg1);

            assert!(crc_info.value() == reg1_crc);
        }

        // Test case: Decode CRC over 2 overlapping region ( region 0 with zero padding, region 1
        // with zero padding )
        {
            // poly - 0x8005 lookup table available
            let reg0_crc = 0x92CC;
            let reg1_crc = 0x088C;
            let poly = 0x1234;
            let max_bits_reg0 = 48;
            let max_bits_reg1 = 32;
            let length: u8 = 16;
            let start_val = 0xFFFF;
            let mut crc_info = CrcInfo::new();

            crc_info.init(poly, start_val, length);

            let buffer = vec![0x31, 0x32, 0x33, 0x34];
            let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
            bitstream_reader.init(&buffer, 32);

            crc_info.region_start = 0;
            let reg0 = crc_info.start_region(&mut bitstream_reader, max_bits_reg0);
            bitstream_reader.push(24);

            let reg1 = crc_info.start_region(&mut bitstream_reader, max_bits_reg1);
            bitstream_reader.push(8);
            let bs_reader = &mut bitstream_reader;
            crc_info.end_region(bs_reader, reg0);
            assert!(crc_info.value() == reg0_crc);

            crc_info.end_region(&mut bitstream_reader, reg1);

            assert!(crc_info.value() == reg1_crc);
        }

        // Test case: Decode CRC over 5 overlapping region ( region 0  to 3 without zero padding,
        // but overlapping, region 4 with zero padding and overlapping )
        {
            // poly - 0x8005 lookup table available
            let reg0_crc = 0xB1B6;
            let reg1_crc = 0x7AB1;
            let reg2_crc = 0xF4E1;
            let reg3_crc = 0x88DA;
            let reg4_crc = 0xCEFE;

            let poly = 0x8005;
            let max_bits_reg0 = 48;
            let max_bits_reg1 = 32;
            let max_bits_reg2 = 32;
            let max_bits_reg3 = 16;
            let max_bits_reg4 = 32;

            let length: u8 = 16;
            let start_val = 0xFFFF;
            let mut crc_info = CrcInfo::new();

            crc_info.init(poly, start_val, length);

            let buffer = vec![0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38];
            let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
            bitstream_reader.init(&buffer, 64);

            crc_info.region_start = 0;
            let bs_reader = &mut bitstream_reader;
            // region 0
            let reg0 = crc_info.start_region(bs_reader, max_bits_reg0);
            bs_reader.push(24);

            // region 1
            let reg1 = crc_info.start_region(bs_reader, max_bits_reg1);
            bs_reader.push(8);

            // region 2
            let reg2 = crc_info.start_region(bs_reader, max_bits_reg2);
            bs_reader.push(16);

            crc_info.end_region(bs_reader, reg0);
            assert!(crc_info.value() == reg0_crc);

            // region 3
            let reg3 = crc_info.start_region(bs_reader, max_bits_reg3);
            bs_reader.push(8);

            crc_info.end_region(bs_reader, reg1);
            assert!(crc_info.value() == reg1_crc);

            // region 4
            let reg4 = crc_info.start_region(bs_reader, max_bits_reg4);
            bs_reader.push(8);

            crc_info.end_region(bs_reader, reg2);
            assert!(crc_info.value() == reg2_crc);

            crc_info.end_region(bs_reader, reg3);
            assert!(crc_info.value() == reg3_crc);

            crc_info.end_region(bs_reader, reg4);
            assert!(crc_info.value() == reg4_crc);
        }
    }
}
