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
//! Temporal noise shaping (TNS)

use crate::common::{
    bitstream::Bitstream,
    flags::ACFlags,
    lpc::{lpc_synthesis_lattice, FilterDirection},
};

use super::channel_info::IcsInfo;
use super::constants::*;
use super::error_codes::AacDecoderError;

// TNS_MAX_BANDS
// entry for each sampling rate
// 1  long window
// 2  SHORT window
static TNS_MAX_BANDS_TBL: [[u8; 2]; 13] = [
    [31, 9],  // 96000
    [31, 9],  // 88200
    [34, 10], // 64000
    [40, 14], // 48000
    [42, 14], // 44100
    [51, 14], // 32000
    [46, 14], // 24000
    [46, 14], // 22050
    [42, 14], // 16000
    [42, 14], // 12000
    [42, 14], // 11025
    [39, 14], //  8000
    [39, 14], //  7350
];

// TNS_MAX_BANDS for low delay. The array index is the sampleRateIndex
static TNS_MAX_BANDS_TBL_480: [u8; 13] = [
    31, // 96000
    31, // 88200
    31, // 64000
    31, // 48000
    32, // 44100
    37, // 32000
    30, // 24000
    30, // 22050
    30, // 16000
    30, // 12000
    30, // 11025
    30, //  8000
    30, //  7350
];

static TNS_MAX_BANDS_TBL_512: [u8; 13] = [
    31, // 96000
    31, // 88200
    31, // 64000
    31, // 48000
    32, // 44100
    37, // 32000
    31, // 24000
    31, // 22050
    31, // 16000
    31, // 12000
    31, // 11025
    31, //  8000
    31, //  7350
];

#[rustfmt::skip]
static TNS_COEFF3: [f32; 8] = [
    -0.9848077_f32,    -0.8660254_f32,    -0.6427876_f32,    -0.34202015_f32,
    0.00000000_f32,    0.43388373_f32,    0.7818315_f32,     0.9749279_f32,
];

#[rustfmt::skip]
static TNS_COEFF4: [f32; 16] = [
    -0.99573416_f32,    -0.96182567_f32,    -0.8951633_f32,    -0.79801726_f32,
    -0.6736957_f32,     -0.5264322_f32,     -0.3612417_f32,    -0.18374953_f32,
    0.00000000_f32,     0.2079117_f32,      0.40673664_f32,    0.58778524_f32,
    0.7431448_f32,      0.8660254_f32,      0.95105654_f32,    0.9945219_f32,
];

const TNS_MAXIMUM_ORDER: usize = 20;
const TNS_MAX_WINDOWS: usize = MAX_WINDOWS;
const TNS_MAXIMUM_FILTERS: usize = 3;

#[derive(Default, Debug, Clone, Copy)]
#[repr(C)]
pub struct TnsFilter {
    coeff: [i8; TNS_MAXIMUM_ORDER],
    start_band: u8,
    stop_band: u8,
    direction: FilterDirection,
    resolution: u8,
    order: u8,
}

#[derive(Default, Debug, Copy, Clone)]
#[repr(C)]
pub struct TnsData {
    filter: [[TnsFilter; TNS_MAXIMUM_FILTERS]; TNS_MAX_WINDOWS],
    num_of_filters: [u8; TNS_MAX_WINDOWS],
    is_data_present: bool,
    is_active: bool,
    tns_max_bands: [u8; 2], // 0: long window, 1: short window
}

impl TnsData {
    /// Create a TnsData instance
    ///
    /// # Parameters
    ///
    /// - `sr_index`: Sampling rate index
    /// - `ac_flags`: Audio codec flag
    pub fn new(sr_index: usize, ac_flags: ACFlags) -> TnsData {
        let mut tns_data = TnsData::default();
        tns_data.init(sr_index, ac_flags);
        tns_data
    }

    /// Initialize a TnsData instance
    ///
    /// # Parameters
    ///
    /// - `sr_index`: Sampling rate index
    /// - `ac_flags`: Audio codec flag
    pub fn init(&mut self, sr_index: usize, ac_flags: ACFlags) {
        self.reset();

        if ac_flags.intersects(ACFlags::LD | ACFlags::ELD) {
            if ac_flags.contains(ACFlags::FRAME_LENGTH) {
                self.tns_max_bands[0] = TNS_MAX_BANDS_TBL_480[sr_index];
                self.tns_max_bands[1] = 0;
            } else {
                self.tns_max_bands[0] = TNS_MAX_BANDS_TBL_512[sr_index];
                self.tns_max_bands[1] = 0;
            }
        } else {
            self.tns_max_bands[0] = TNS_MAX_BANDS_TBL[sr_index][0];
            self.tns_max_bands[1] = TNS_MAX_BANDS_TBL[sr_index][1];
            if ac_flags.contains(ACFlags::USAC) && (sr_index > 5) {
                self.tns_max_bands[0] += 1;
                self.tns_max_bands[1] += 1;
            }
        }
    }

    /// Reset TNS data
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::tns::TnsData;
    ///
    /// let mut tns_data = TnsData::default();
    /// tns_data.reset();
    /// ```
    pub fn reset(&mut self) {
        self.filter = Default::default();
        self.num_of_filters = Default::default();
        self.is_data_present = false;
        self.is_active = false;
    }

    /// Returns true if TNS data is present, otherwise false
    pub fn is_data_present(&self) -> bool {
        self.is_data_present
    }

    /// Read TNS data present flag from bitstream and update flag in TNS struct
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance in reader mode with valid internal data
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::tns::TnsData;
    /// use aac::common::bitstream::{Bitstream, Mode};
    ///
    /// // Precondition, should have a valid Bitstream instance in reader mode
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 8);
    ///
    /// let mut tns_data = TnsData::default();
    /// tns_data.read_datapresent_flag(&mut bitstream_reader);
    ///
    ///  ```
    pub fn read_datapresent_flag(&mut self, bs: &mut Bitstream) {
        self.is_data_present = bs.read_bit() != 0;
    }

    /// Read TNS data from bitstream
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `ac_flags`: Audio codec flag
    ///
    ///  Returns type of AacDecoderError
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{
    ///     tns::TnsData, channel_info::IcsInfo, error_codes::AacDecoderError
    /// };
    /// use aac::common::{bitstream::Bitstream, bitstream::Mode, flags::{ACFlags, self}};
    ///
    /// // Precondition, should have a valid Bitstream and IcsInfo instance
    /// // Refer respective components for instance creation, read/write methods
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 40);
    ///
    /// let ics_info = IcsInfo::new();
    ///
    /// let mut tns_data = TnsData::default();
    /// let ac_flags = ACFlags::USAC;
    /// let error_status: AacDecoderError = tns_data.read(&mut bitstream_reader, &ics_info,
    ///     ac_flags
    /// );
    ///
    ///  ```
    pub fn read(
        &mut self,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        ac_flags: ACFlags,
    ) -> AacDecoderError {
        if !self.is_data_present {
            return AacDecoderError::Ok;
        }

        // Decide the number of bits to be read from bitstream
        let (filt_bits, band_bits, order_bits) = if ics_info.is_long_block() {
            (
                2_u8,
                6_u8,
                if ac_flags.contains(ACFlags::USAC) {
                    4_u8
                } else {
                    5_u8
                },
            )
        } else {
            (1_u8, 4_u8, 3_u8)
        };

        for (window, win_filter) in self
            .filter
            .iter_mut()
            .enumerate()
            .take(ics_info.windows_per_frame())
        {
            // Read number of filters
            let num_filt = bs.read(filt_bits) as u8;
            self.num_of_filters[window] = num_filt;

            if num_filt != 0 {
                let coeff_res = bs.read_bit() as u8;
                let mut next_stopband = ics_info.n_total_sf_bands() as u8;

                // Get TNS filter(tf) from window
                for tf in win_filter.iter_mut().take(num_filt.into()) {
                    // Read band length
                    let band_length = bs.read(band_bits) as u8;

                    tf.start_band = next_stopband - band_length.min(next_stopband);
                    tf.stop_band = next_stopband;
                    next_stopband = tf.start_band;

                    // Read TNS filter order - max(Order) = 15 (long), 7 (short)
                    let order = bs.read(order_bits) as u8;
                    tf.order = order;

                    if usize::from(order) > TNS_MAXIMUM_ORDER {
                        return AacDecoderError::TnsReadError;
                    }

                    if order != 0 {
                        const SGN_MASK: [u8; 3] = [0x2, 0x4, 0x8];
                        const NEG_MASK: [u8; 3] = [!0x3, !0x7, !0xF];
                        tf.direction = {
                            if bs.read_bit() != 0 {
                                FilterDirection::Backward
                            } else {
                                FilterDirection::Forward
                            }
                        };

                        let coeff_compress = bs.read_bit() as u8;
                        tf.resolution = coeff_res + 3;

                        let mask_idx = usize::from(coeff_res + 1 - coeff_compress);
                        let s_mask = SGN_MASK[mask_idx];
                        let n_mask = NEG_MASK[mask_idx];

                        let coeff_bits = tf.resolution - coeff_compress;

                        // for_each produces less no. of asm lines than for loop
                        tf.coeff.iter_mut().take(order.into()).for_each(|tf_coeff| {
                            // Note: coeff should be u8 before sign check (or) conversion
                            let coeff = bs.read(coeff_bits) as u8;
                            *tf_coeff = {
                                if coeff & s_mask != 0 {
                                    // Attention: using "as" since "i8::try_from" will panic if
                                    // coeff > 127
                                    (coeff | n_mask) as i8
                                } else {
                                    coeff as i8
                                }
                            };
                        });
                    }
                }
            }
        }
        self.is_active = true;
        // return error status
        AacDecoderError::Ok
    }

    /// Read USAC specific TNS data from bitstream
    ///
    /// # Parameters
    ///
    /// - `second_tns`: TNS data
    /// - `bs`: Bitstream reader with valid internal data
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `is_tns_on_lr`: flag to indicate mode of TNS filtering, updated after reading bitstream
    /// - `ac_flags`: Audio codec flag
    /// - `common_window`: signalling flag to use identical window parameters for first (self) and
    ///   second TNS filtering
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{
    ///     tns::TnsData, channel_info::IcsInfo, error_codes::AacDecoderError
    /// };
    /// use aac::common::{bitstream::Bitstream, bitstream::Mode, flags::{ACFlags, self}};
    ///
    /// // Precondition, should have a valid Bitstream and IcsInfo instance
    /// // Refer respective components for instance creation, read/write methods
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 8);
    ///
    /// let ics_info = IcsInfo::new();
    ///
    /// let mut first_tnsdata = TnsData::default();
    /// let mut second_tnsdata = TnsData::default();
    /// let mut is_tns_on_lr: bool = false;
    /// let common_window: bool = true;
    /// let ac_flags = ACFlags::USAC;
    ///
    /// first_tnsdata.read_datapresent_usac(&mut second_tnsdata, &mut bitstream_reader, &ics_info,
    ///     &mut is_tns_on_lr, ac_flags, common_window
    /// );
    ///
    ///  ```
    pub fn read_datapresent_usac(
        &mut self,
        second_tns: &mut TnsData,
        bs: &mut Bitstream,
        ics_info: &IcsInfo,
        is_tns_on_lr: &mut bool,
        ac_flags: ACFlags,
        common_window: bool,
    ) {
        let common_tns = common_window && bs.read_bit() == 1;
        *is_tns_on_lr = bs.read_bit() != 0;

        if common_tns {
            self.is_data_present = true;
            self.read(bs, ics_info, ac_flags);

            self.is_data_present = false;
            self.is_active = true;
            second_tns.clone_from(self);
        } else {
            let tns_present_both = bs.read_bit() != 0;

            if tns_present_both {
                self.is_data_present = true;
                second_tns.is_data_present = true;
            } else {
                second_tns.is_data_present = bs.read_bit() != 0;
                self.is_data_present = !second_tns.is_data_present;
            }
        }
    }

    /// Apply TNS filter to the given spectrum
    ///
    /// # Parameters
    ///
    /// - `ics_info`: Individual channel stream info with valid internal data
    /// - `spectral_coefficient`: Spectrum
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::aac_dec::{tns::TnsData, channel_info::IcsInfo};
    /// use aac::common::flags::{ACFlags, self};
    ///
    /// // Precondition, should have a valid IcsInfo instance
    /// // Refer IcsInfo component for instance creation, get methods
    /// let ics_info = IcsInfo::new();
    ///
    /// let mut spectrum = vec![0.0_f32; 512];
    /// let samplingrate_index: usize = 4;
    /// let ac_flags = ACFlags::USAC;
    ///
    /// let mut tns_data = TnsData::new(samplingrate_index, ac_flags);
    ///
    /// tns_data.apply(&ics_info, spectrum.as_mut_slice());
    ///
    ///  ```
    pub fn apply(&self, ics_info: &IcsInfo, spectral_coefficient: &mut [f32]) {
        if self.is_active {
            let mut coeff: [f32; TNS_MAXIMUM_ORDER] = Default::default();
            let wins_per_frame = ics_info.windows_per_frame();

            let nbands = (ics_info.max_sf_bands() as u8)
                .min(self.tns_max_bands[usize::from(!ics_info.is_long_block())]);
            let sf_bands = ics_info.scale_factor_bands();
            let frame_length = spectral_coefficient.len();
            let window_length = frame_length / wins_per_frame;
            for (window, spectrum) in spectral_coefficient
                .chunks_exact_mut(window_length)
                .enumerate()
                .take(wins_per_frame)
            {
                for tf in self.filter[window][..]
                    .iter()
                    .take(usize::from(self.num_of_filters[window]))
                {
                    if tf.order > 0 {
                        let (tns_coeff, offset) = if tf.resolution == 3 {
                            (&TNS_COEFF3[..], 4_i8)
                        } else {
                            (&TNS_COEFF4[..], 8_i8)
                        };

                        for (coeff_val, tf_coeff_val) in coeff
                            .iter_mut()
                            .zip(tf.coeff.iter())
                            .take(usize::from(tf.order))
                        {
                            *coeff_val =
                                tns_coeff[usize::try_from(*tf_coeff_val + offset).unwrap()];
                        }

                        let start = sf_bands[usize::from(nbands.min(tf.start_band))];
                        let stop = sf_bands[usize::from(nbands.min(tf.stop_band))];

                        if start != stop {
                            lpc_synthesis_lattice(
                                &mut spectrum[usize::from(start)..usize::from(stop)],
                                tf.direction,
                                &coeff[..usize::from(tf.order)],
                            );
                        }
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        aac_dec::sr_info::SamplingRateInfo,
        common::{bitstream::Mode, flags::ACFlags},
    };

    #[test]
    fn new() {
        let tns_data = TnsData::default();

        assert_eq!(tns_data.filter.len(), TNS_MAX_WINDOWS);
        // checking for one window alone for validation
        assert_eq!(tns_data.filter[0].len(), TNS_MAXIMUM_FILTERS);

        // checking for one filter alone for validation
        let tf = tns_data.filter[0][0];
        assert_eq!(tf.coeff.len(), TNS_MAXIMUM_ORDER);
        assert_eq!(tf.start_band, 0);
        assert_eq!(tf.stop_band, 0);
        assert_eq!(tf.resolution, 0);
        assert_eq!(tf.direction, FilterDirection::Forward);
        assert_eq!(tf.order, 0);

        assert_eq!(&tns_data.num_of_filters, &[0; TNS_MAX_WINDOWS]);
        assert!(!tns_data.is_active);
        assert!(!tns_data.is_data_present);
    }

    #[test]
    fn tns_present_flag() {
        let mut tns_data = TnsData::default();
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);
        bitstream_writer.write(0b1, 1);
        bitstream_writer.sync();

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 1);
        tns_data.read_datapresent_flag(&mut bitstream_reader);

        assert!(tns_data.is_data_present);
    }

    #[test]
    fn read_long_block_one_tnsfilter() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b01     - number of TNS filters
            // 0b1      - coefficient resolution
            // 0b001010 - band length = 10
            // 0b00010  - filter order = 2
            // 0b1      - filter direction
            // 0b1      - coefficient compress
            // 0b011010 - filter coefficients = 3,2
            bitstream_writer.write(0b10110010100001011011010, 23);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 34);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 1);
        assert!(tns_data.is_active);
        assert_eq!(tns_data.filter[0][0].start_band, 39);
        assert_eq!(tns_data.filter[0][0].stop_band, 49);
        assert_eq!(tns_data.filter[0][0].resolution, 4);
        assert_eq!(tns_data.filter[0][0].order, 2);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Backward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..2], &[3, 2]);
    }

    #[test]
    fn read_long_block_two_tnsfilter() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b10     - number of TNS filters = 2
            // 0b1      - coefficient resolution
            // 0b010100 - band length = 20
            // 0b00001  - filter order = 1
            // 0b0      - filter direction
            // 0b1      - coefficient compress
            // 0b111    - filter coefficients = 7
            // For first TNS filter
            bitstream_writer.write(0b11010101000000101111, 20);
            // Byte blob meaning:
            // 0b011110    - band length = 15
            // 0b00011     - filter order = 3
            // 0b1         - filter direction
            // 0b0         - coefficient compress
            // 0b101010100111 - filter coefficients = 10,10,7
            // for second TNS filter
            bitstream_writer.write(0b0011110001110101010100111, 25);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 56);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 2);
        assert!(tns_data.is_active);

        // First TNS filter
        assert_eq!(tns_data.filter[0][0].start_band, 29);
        assert_eq!(tns_data.filter[0][0].stop_band, 49);
        assert_eq!(tns_data.filter[0][0].resolution, 4);
        assert_eq!(tns_data.filter[0][0].order, 1);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..1], &[-1]);

        // second TNS filter
        assert_eq!(tns_data.filter[0][1].start_band, 14);
        assert_eq!(tns_data.filter[0][1].stop_band, 29);
        assert_eq!(tns_data.filter[0][1].resolution, 4);
        assert_eq!(tns_data.filter[0][1].order, 3);
        assert_eq!(tns_data.filter[0][1].direction, FilterDirection::Backward);
        assert_eq!(&tns_data.filter[0][1].coeff[0..3], &[-6, -6, 7]);
    }

    #[test]
    fn read_invalid_filter_order() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b01     - number of TNS filters
            // 0b1      - coefficient resolution
            // 0b001010 - band length = 10
            // 0b10101  - filter order = 21
            // 0b1      - filter direction
            // 0b1      - coefficient compress
            // 0b011010 - filter coefficients = 3,2
            bitstream_writer.write(0b10110010101010111011010, 23);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 34);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        assert_eq!(return_status, AacDecoderError::TnsReadError);
        assert!(tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 1);
        assert!(!tns_data.is_active);
        assert_eq!(tns_data.filter[0][0].start_band, 39);
        assert_eq!(tns_data.filter[0][0].stop_band, 49);
        assert_eq!(tns_data.filter[0][0].resolution, 0);
        assert_eq!(tns_data.filter[0][0].order, 21);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..2], &[0, 0]);
    }

    #[test]
    fn read_without_tns_data() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b0      - TNS present
            bitstream_writer.write(0b0, 1);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 12);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(!tns_data.is_data_present);
        assert!(!tns_data.is_active);
    }

    #[test]
    fn read_with_usac_flag() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b01     - number of TNS filters
            // 0b1      - coefficient resolution
            // 0b001111 - band length = 15
            // 0b0011  - filter order = 3
            // 0b0      - filter direction
            // 0b1      - coefficient compress
            // 0b011010110 - filter coefficients = 3,2,6
            bitstream_writer.write(0b1011001111001101011010110, 25);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 36);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        // Attention, Eventhough ics_info is doesn't read USAC,
        // In TNS Pass AC_USAC flag, to check the conditions and flow of bitstream reading in TNS
        // read.
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::USAC);

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 1);
        assert!(tns_data.is_active);
        assert_eq!(tns_data.filter[0][0].start_band, 34);
        assert_eq!(tns_data.filter[0][0].stop_band, 49);
        assert_eq!(tns_data.filter[0][0].resolution, 4);
        assert_eq!(tns_data.filter[0][0].order, 3);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..3], &[3, 2, -2]);
    }

    #[test]
    fn read_short_block() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(16, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b10     : BlockType::Short
            // 0b0      : SineWindow
            // 0b1100   : max_sf_bands = 12
            // 0b1101101: scale factor grouping: 3|3|2
            bitstream_writer.write(0b010011001101101, 15);
            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b1      - number of TNS filters = 1
            // 0b1      - coefficient resolution
            // 0b0100   - band length = 4
            // 0b001    - filter order = 1
            // 0b0      - filter direction
            // 0b1      - coefficient compress
            // 0b101    - filter coefficients = 5
            // For first short window, first TNS filter
            bitstream_writer.write(0b111010000101101, 15);
            // Byte blob meaning:
            // 0b1         - number of TNS filters = 1
            // 0b1         - coefficient resolution
            // 0b0101      - band length = 5
            // 0b011       - filter order = 3
            // 0b1         - filter direction
            // 0b0         - coefficient compress
            // 0b101010100111 - filter coefficients = 10,10,7
            // for second short window, first TNS filter
            bitstream_writer.write(0b11010101110101010100111, 23);

            // Byte blob meaning:
            // 0b00000         - number of TNS filters = 0, 0, 0 , 0, 0
            // for next 5 short window, let assume there is no TNS filter (i.e zero)
            bitstream_writer.write(0b00000, 5);

            // Byte blob meaning:
            // 0b1         - number of TNS filters = 1
            // 0b1         - coefficient resolution
            // 0b0011      - band length = 3
            // 0b011       - filter order = 3
            // 0b1         - filter direction
            // 0b0         - coefficient compress
            // 0b010101010010 - filter coefficients = 5,5,2
            // for last(8th) short window, first TNS filter
            bitstream_writer.write(0b11001101110010101010010, 23);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 78);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::default();
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(tns_data.is_data_present);
        assert!(tns_data.is_active);

        // for first short window, first TNS filter
        assert_eq!(tns_data.num_of_filters[0], 1);
        assert_eq!(tns_data.filter[0][0].start_band, 10);
        assert_eq!(tns_data.filter[0][0].stop_band, 14);
        assert_eq!(tns_data.filter[0][0].resolution, 4);
        assert_eq!(tns_data.filter[0][0].order, 1);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..1], &[-3]);

        // for second short window, first TNS filter
        assert_eq!(tns_data.num_of_filters[1], 1);
        assert_eq!(tns_data.filter[1][0].start_band, 9);
        assert_eq!(tns_data.filter[1][0].stop_band, 14);
        assert_eq!(tns_data.filter[1][0].resolution, 4);
        assert_eq!(tns_data.filter[1][0].order, 3);
        assert_eq!(tns_data.filter[1][0].direction, FilterDirection::Backward);
        assert_eq!(&tns_data.filter[1][0].coeff[0..3], &[-6, -6, 7]);

        // for 3 to 7 short windows
        assert_eq!(tns_data.num_of_filters[2], 0);
        assert_eq!(tns_data.num_of_filters[3], 0);
        assert_eq!(tns_data.num_of_filters[4], 0);
        assert_eq!(tns_data.num_of_filters[5], 0);
        assert_eq!(tns_data.num_of_filters[6], 0);

        // for last(8th) short window, first TNS filter
        assert_eq!(tns_data.num_of_filters[7], 1);
        assert_eq!(tns_data.filter[7][0].start_band, 11);
        assert_eq!(tns_data.filter[7][0].stop_band, 14);
        assert_eq!(tns_data.filter[7][0].resolution, 4);
        assert_eq!(tns_data.filter[7][0].order, 3);
        assert_eq!(tns_data.filter[7][0].direction, FilterDirection::Backward);
        assert_eq!(&tns_data.filter[7][0].coeff[0..3], &[5, 5, 2]);
    }

    #[test]
    fn read_datapresent_usac_with_common_tns() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - common_window
            // 0b1      - is_tns_on_lr
            // 0b01     - number of TNS filters
            // 0b1      - coefficient resolution
            // 0b001010 - band length = 10
            // 0b00010  - filter order = 2
            // 0b1      - filter direction
            // 0b1      - coefficient compress
            // 0b011010 - filter coefficients = 3,2
            bitstream_writer.write(0b110110010100001011011010, 24);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 35);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut first_tns = TnsData::default();
        let mut second_tns = TnsData::default();
        let is_tns_on_lr: &mut bool = &mut false;
        let common_window = true;

        first_tns.read_datapresent_usac(
            &mut second_tns,
            &mut bitstream_reader,
            &ics_info,
            is_tns_on_lr,
            ACFlags::empty(),
            common_window,
        );

        assert!(*is_tns_on_lr);
        assert!(!first_tns.is_data_present);
        assert_eq!(first_tns.num_of_filters[0], 1);
        assert!(first_tns.is_active);
        assert_eq!(first_tns.filter[0][0].start_band, 39);
        assert_eq!(first_tns.filter[0][0].stop_band, 49);
        assert_eq!(first_tns.filter[0][0].resolution, 4);
        assert_eq!(first_tns.filter[0][0].order, 2);
        assert_eq!(first_tns.filter[0][0].direction, FilterDirection::Backward);
        assert_eq!(&first_tns.filter[0][0].coeff[0..2], &[3, 2]);

        assert!(!second_tns.is_data_present);
        assert_eq!(second_tns.num_of_filters[0], 1);
        assert!(second_tns.is_active);
        assert_eq!(second_tns.filter[0][0].start_band, 39);
        assert_eq!(second_tns.filter[0][0].stop_band, 49);
        assert_eq!(second_tns.filter[0][0].resolution, 4);
        assert_eq!(second_tns.filter[0][0].order, 2);
        assert_eq!(second_tns.filter[0][0].direction, FilterDirection::Backward);
        assert_eq!(&second_tns.filter[0][0].coeff[0..2], &[3, 2]);
    }

    #[test]
    fn read_datapresent_usac_with_both_tns() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - is_tns_on_lr
            // 0b1      - both LR TNS present
            bitstream_writer.write(0b11, 2);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 13);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut first_tns = TnsData::default();
        let mut second_tns = TnsData::default();
        let is_tns_on_lr: &mut bool = &mut false;
        let common_window = false;

        first_tns.read_datapresent_usac(
            &mut second_tns,
            &mut bitstream_reader,
            &ics_info,
            is_tns_on_lr,
            ACFlags::empty(),
            common_window,
        );

        assert!(*is_tns_on_lr);
        assert!(first_tns.is_data_present);
        assert!(second_tns.is_data_present);
    }

    #[test]
    fn read_datapresent_usac_with_first_tns() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - is_tns_on_lr
            // 0b0      - both LR TNS present
            // 0b0      - L (or) R TNS present
            bitstream_writer.write(0b100, 3);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 14);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut first_tns = TnsData::default();
        let mut second_tns = TnsData::default();
        let is_tns_on_lr: &mut bool = &mut false;
        let common_window = false;

        first_tns.read_datapresent_usac(
            &mut second_tns,
            &mut bitstream_reader,
            &ics_info,
            is_tns_on_lr,
            ACFlags::empty(),
            common_window,
        );

        assert!(*is_tns_on_lr);
        assert!(first_tns.is_data_present);
        assert!(!second_tns.is_data_present);
    }

    #[test]
    fn read_datapresent_usac_with_second_tns() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - is_tns_on_lr
            // 0b0      - both LR TNS present
            // 0b1      - L (or) R TNS present
            bitstream_writer.write(0b101, 3);
            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 14);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut first_tns = TnsData::default();
        let mut second_tns = TnsData::default();
        let is_tns_on_lr: &mut bool = &mut false;
        let common_window = false;

        first_tns.read_datapresent_usac(
            &mut second_tns,
            &mut bitstream_reader,
            &ics_info,
            is_tns_on_lr,
            ACFlags::empty(),
            common_window,
        );

        assert!(*is_tns_on_lr);
        assert!(!first_tns.is_data_present);
        assert!(second_tns.is_data_present);
    }

    #[test]
    fn reset() {
        let mut tns_data = TnsData::new(0, ACFlags::empty());

        tns_data.is_active = true;
        tns_data.is_data_present = true;
        tns_data.num_of_filters[0] = 3;
        tns_data.num_of_filters[1] = 2;
        tns_data.num_of_filters[2] = 1;
        tns_data.filter[0][0].start_band = 39;
        tns_data.filter[0][0].stop_band = 49;
        tns_data.filter[0][0].resolution = 4;
        tns_data.filter[0][0].order = 2;
        tns_data.filter[0][0].direction = FilterDirection::Backward;

        // Reset TNS instance
        tns_data.reset();

        // After reset
        assert!(!tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 0);
        assert_eq!(tns_data.num_of_filters[1], 0);
        assert_eq!(tns_data.num_of_filters[2], 0);
        assert!(!tns_data.is_active);
        assert_eq!(tns_data.filter[0][0].start_band, 0);
        assert_eq!(tns_data.filter[0][0].stop_band, 0);
        assert_eq!(tns_data.filter[0][0].resolution, 0);
        assert_eq!(tns_data.filter[0][0].order, 0);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
    }

    #[test]
    fn apply() {
        // Create bitstream
        let mut bitstream_writer = Bitstream::new(8, Mode::Writer);

        {
            // ics_info from bitstream
            // Byte blob meaning:
            // 0b0      : reserved
            // 0b00     : BlockType::Long
            // 0b1      : KBDWindow
            // 0b101000 : max_sf_bands = 40
            // 0b0      : no prediction
            bitstream_writer.write(0b00011010000, 11);

            // TNS data from bitstream
            // Byte blob meaning:
            // 0b1      - TNS present
            // 0b10     - number of TNS filters = 2
            // 0b1      - coefficient resolution
            // 0b010100 - band length = 20
            // 0b00001  - filter order = 1
            // 0b0      - filter direction
            // 0b1      - coefficient compress
            // 0b111    - filter coefficients = 7
            // For first TNS filter
            bitstream_writer.write(0b11010101000000101111, 20);
            // Byte blob meaning:
            // 0b011110    - band length = 15
            // 0b00011     - filter order = 3
            // 0b1         - filter direction
            // 0b0         - coefficient compress
            // 0b101010100111 - filter coefficients = 10,10,7
            // for second TNS filter
            bitstream_writer.write(0b0011110001110101010100111, 25);

            bitstream_writer.sync();
        }

        let mut bitstream_reader = Bitstream::new(bitstream_writer.buffer().len(), Mode::Reader);
        bitstream_reader.init(bitstream_writer.buffer(), 56);

        // Get Sample rate info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // Create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        let mut tns_data = TnsData::new(3, ACFlags::empty());
        tns_data.read_datapresent_flag(&mut bitstream_reader);
        let return_status = tns_data.read(&mut bitstream_reader, &ics_info, ACFlags::empty());

        const DUMMY_SPECTRUM: f32 = 0.6789_f32;
        let mut spectrum = vec![DUMMY_SPECTRUM; 1024];
        tns_data.apply(&ics_info, &mut spectrum);

        assert_eq!(&spectrum[0..72], &[DUMMY_SPECTRUM; 72]);
        assert_ne!(&spectrum[72..319], &[DUMMY_SPECTRUM; 247]);
        assert_eq!(&spectrum[319..321], &[DUMMY_SPECTRUM; 2]);
        assert_ne!(&spectrum[321..672], &[DUMMY_SPECTRUM; 351]);
        assert_eq!(&spectrum[672..1024], &[DUMMY_SPECTRUM; 352]);

        assert_eq!(return_status, AacDecoderError::Ok);
        assert!(tns_data.is_data_present);
        assert_eq!(tns_data.num_of_filters[0], 2);
        assert!(tns_data.is_active);

        // First TNS filter
        assert_eq!(tns_data.filter[0][0].start_band, 29);
        assert_eq!(tns_data.filter[0][0].stop_band, 49);
        assert_eq!(tns_data.filter[0][0].resolution, 4);
        assert_eq!(tns_data.filter[0][0].order, 1);
        assert_eq!(tns_data.filter[0][0].direction, FilterDirection::Forward);
        assert_eq!(&tns_data.filter[0][0].coeff[0..1], &[-1]);

        // second TNS filter
        assert_eq!(tns_data.filter[0][1].start_band, 14);
        assert_eq!(tns_data.filter[0][1].stop_band, 29);
        assert_eq!(tns_data.filter[0][1].resolution, 4);
        assert_eq!(tns_data.filter[0][1].order, 3);
        assert_eq!(tns_data.filter[0][1].direction, FilterDirection::Backward);
        assert_eq!(&tns_data.filter[0][1].coeff[0..3], &[-6, -6, 7]);
    }
}
