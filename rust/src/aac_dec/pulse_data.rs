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
//! Pulse data tool
//!
//! The pulse data feature can be used by the encoder to replace single spectral
//! coefficients in decoded spectrum.

use crate::aac_dec::{channel_info::IcsInfo, error_codes::AacDecoderError};
use crate::common::bitstream::Bitstream;
use itertools::izip;

const NUM_MAX_LINES: usize = 4;

/// Pulse Data
///
/// The `PulseData` tool replaces one or several quantized spectral coefficients
/// with smaller amplitudes according to the bitstream configuration.
/// The presence and the number of individual pulses is described by means of the
/// trasnmitted bitstream and restricted to long blocks only.
///
/// # Examples
///
/// ```
/// use aac::aac_dec::{channel_info::IcsInfo, pulse_data::PulseData};
/// use aac::common::bitstream::{Bitstream, Mode};
///
/// let mut spectral_data = vec![0; 1024];
/// let bit_buffer = vec![0; 2];
/// let mut bs = Bitstream::new(bit_buffer.len(), Mode::Reader);
/// bs.init(&bit_buffer, 16);
/// let ics_info = IcsInfo::new();
///
/// let mut pulse_data = PulseData::new();
/// pulse_data.read(&mut bs, &ics_info);
/// pulse_data.apply(&mut spectral_data);
/// ```
#[derive(Default, Debug, Clone)]
#[repr(C)]
pub struct PulseData {
    is_data_present: bool,
    number_pulse: usize,
    pulse_offset: [usize; NUM_MAX_LINES],
    pulse_amp: [i16; NUM_MAX_LINES],
}

impl PulseData {
    /// Creates a `PulseData` instance
    pub fn new() -> PulseData {
        Default::default()
    }

    /// Reads `PulseData` related data from `Bitstream`
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream to read from
    /// - `ics_info`: Individual channel stream info data
    ///
    /// # Errors
    ///
    /// Returns `AacDecoderError` type
    /// - `AacDecOk` on success
    pub fn read(&mut self, bs: &mut Bitstream, ics_info: &IcsInfo) -> AacDecoderError {
        if bs.read_bit() == 0 {
            self.is_data_present = false;
        } else {
            if !ics_info.is_long_block() {
                return AacDecoderError::DecodeFrameError;
            }

            self.is_data_present = true;
            self.number_pulse = bs.read(2) as usize;
            let pulse_start_band = bs.read(6) as usize;

            if pulse_start_band >= ics_info.max_sf_bands() {
                return AacDecoderError::DecodeFrameError;
            }
            let mut offset = usize::from(ics_info.scale_factor_bands()[pulse_start_band]);

            for (pulse_offset, pulse_amp) in
                izip!(self.pulse_offset.iter_mut(), self.pulse_amp.iter_mut())
                    .take(self.number_pulse + 1)
            {
                offset += bs.read(5) as usize;
                *pulse_offset = offset;
                *pulse_amp = bs.read(4) as i16;
            }

            if offset >= usize::from(ics_info.scale_factor_bands()[ics_info.n_total_sf_bands()]) {
                return AacDecoderError::DecodeFrameError;
            }
        }

        AacDecoderError::Ok
    }

    /// Applies `PulseData` adjustment on spectral coefficients
    ///
    /// # Parameters
    ///
    /// - `spectrum`: Quantized spectral coefficients
    pub fn apply(&self, spectrum: &mut [i16]) {
        if self.is_data_present {
            for (k, pulse_amp) in
                izip!(self.pulse_offset.iter(), self.pulse_amp.iter()).take(self.number_pulse + 1)
            {
                spectrum[*k] = if spectrum[*k] > 0 {
                    spectrum[*k].saturating_add(*pulse_amp)
                } else {
                    spectrum[*k].saturating_sub(*pulse_amp)
                };
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        aac_dec::{
            channel_info::IcsInfo, error_codes::AacDecoderError, pulse_data::PulseData,
            sr_info::SamplingRateInfo,
        },
        common::{
            bitstream::{Bitstream, Mode},
            flags::ACFlags,
        },
    };

    #[test]
    fn read() {
        // get sample_rate_info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // ics_info byte blob
        let buffer: [u8; 2] = [0b00011010, 0b00000000];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 11);

        // create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // pulse_data byte blob
        let buffer: [u8; 4] = [0b10100010, 0b00000100, 0b10000110, 0b10000000];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 20);

        // read pulse_data
        let mut pulse_data = PulseData::new();
        pulse_data.read(&mut bitstream_reader, &ics_info);

        assert!(pulse_data.is_data_present);
        assert_eq!(pulse_data.number_pulse, 1);
        assert_eq!(pulse_data.pulse_offset, [17, 20, 0, 0]);
        assert_eq!(pulse_data.pulse_amp, [2, 4, 0, 0]);
    }

    #[test]
    fn read_is_long_block_check() {
        // get sample_rate_info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // ics_info byte blob (short block)
        let buffer: [u8; 2] = [0b01001100, 0b11011010];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 15);

        // create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // pulse_data byte blob
        let buffer: [u8; 4] = [0b10100010, 0b00000100, 0b10000110, 0b10000000];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 20);

        // short blocks not supported
        let mut pulse_data = PulseData::new();
        assert_eq!(
            pulse_data.read(&mut bitstream_reader, &ics_info),
            AacDecoderError::DecodeFrameError
        );
    }

    #[test]
    fn read_offset_sanity_check() {
        // get sample_rate_info
        let mut sr_info = SamplingRateInfo::new();
        let _ = sr_info.init(1024, 3, 48000);

        // ics_info byte blob (long block; max_sf_bands=49)
        let buffer: [u8; 2] = [0b00011100, 0b01000000];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 11);

        // create ICSInfo
        let mut ics_info = IcsInfo::new();
        ics_info.read(&mut bitstream_reader, &sr_info, ACFlags::empty());

        // pulse_data byte blob (pulse_start_band=49)
        let buffer: [u8; 4] = [0b10111000, 0b10000100, 0b10000110, 0b10000000];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 20);

        // pulse_offset is greater than or equal to max_sf_bands
        let mut pulse_data = PulseData::new();
        assert_eq!(
            pulse_data.read(&mut bitstream_reader, &ics_info),
            AacDecoderError::DecodeFrameError
        );

        // pulse_data byte blob (pulse_start_band=48; 5x32 pulse_offset))
        let buffer: [u8; 8] = [
            0b11111000, 0b01111100, 0b00111110, 0b00011111, 0b00001111, 0b10000000, 0b00000000,
            0b00000000,
        ];
        let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
        bitstream_reader.init(&buffer, 41);

        // pulse_offset[] is greater than or equal to frame_length
        let mut pulse_data = PulseData::new();
        assert_eq!(
            pulse_data.read(&mut bitstream_reader, &ics_info),
            AacDecoderError::DecodeFrameError
        );
    }

    #[test]
    fn apply() {
        let pulse_data = PulseData {
            is_data_present: true,
            number_pulse: 1,
            pulse_offset: [3, 6, 0, 0],
            pulse_amp: [2, 4, 0, 0],
        };

        let mut spectrum = [0, 0, 0, 0, 1, 1, 1, 1];
        pulse_data.apply(&mut spectrum);

        assert_eq!(spectrum, [0, 0, 0, -2, 1, 1, 5, 1]);
    }
}
