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
//! MPEG-D DRC's reader component.

use super::super::{
    common::get_delta_tmin,
    constants::{
        DRCDEC_MAX_FRAME_SIZE, DRCDEC_MAX_SAMPLERATE, DRCDEC_MIN_SAMPLERATE, LOCATION_SELECTED,
    },
    drc_error::DrcError,
};
use super::{LoudnessInfoSet, UniDrcConfig, UniDrcGain};
use crate::common::bitstream::Bitstream;

#[repr(C)]
#[derive(Default, Debug)]
/// `Reader` structure that holds information for uniDRC config, loudness info set and uniDRC gain.
pub(in super::super) struct Reader {
    /// Delta time min value (default value is 1).
    delta_tmin_default: u16,
    /// DRC frame size
    frame_size: u16,
    /// DRC configuration information.
    pub(in super::super) uni_drc_config: UniDrcConfig,
    pub(in super::super) loudness_info_set: LoudnessInfoSet,
    /// uniDRC gain data.
    uni_drc_gain: UniDrcGain,
}

impl Reader {
    /// Creates and returns `Self` instance with default values.
    pub(in super::super) fn new() -> Self {
        Reader::default()
    }

    /// Reads `uniDRC gain` data from bitstream.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read_uni_drc_gain(
        &mut self,
        bs: &mut Bitstream,
    ) -> Result<(), DrcError> {
        let coeff_uni_drc = self
            .uni_drc_config
            .select_drc_coefficients(LOCATION_SELECTED);

        self.uni_drc_gain.read(
            bs,
            coeff_uni_drc,
            self.delta_tmin_default,
            self.frame_size as i16,
        )
    }

    /// Reads `DRC data` from bitstream. This includes parsing of information such as uniDRC
    /// configuration, uniDRC gain and loudness info sets.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream reader with valid internal data
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn read(&mut self, bs: &mut Bitstream) -> Result<(), DrcError> {
        let is_loudness_info_set_present = bs.read_bit() != 0;

        if is_loudness_info_set_present {
            let is_uni_drc_config_present = bs.read_bit() != 0;
            if is_uni_drc_config_present {
                self.uni_drc_config.read(bs)?;
            }

            self.loudness_info_set.read(bs)?;
        }

        self.read_uni_drc_gain(bs)?;

        Ok(())
    }

    /// Initializes `delta time min` value based on sampling rate.
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn init_delta_time_min(
        &mut self,
        sample_rate: u32,
    ) -> Result<(), DrcError> {
        if !(DRCDEC_MIN_SAMPLERATE..=DRCDEC_MAX_SAMPLERATE).contains(&sample_rate) {
            Err(DrcError::ParamOutOfRange)
        } else {
            self.delta_tmin_default = get_delta_tmin(sample_rate);
            Ok(())
        }
    }

    /// Sets frame size of DRC.
    ///
    ///  Returns `DrcError`.
    pub(in super::super) fn set_frame_size(&mut self, frame_size: u16) -> Result<(), DrcError> {
        if !(1..=DRCDEC_MAX_FRAME_SIZE).contains(&frame_size) {
            return Err(DrcError::ParamOutOfRange);
        }
        self.frame_size = frame_size;
        Ok(())
    }

    /// Returns immutable reference to `UniDrcGain`.
    pub(in super::super) fn uni_drc_gain(&self) -> &UniDrcGain {
        &self.uni_drc_gain
    }

    /// Returns mutable reference to `UniDrcGain`.
    pub(in super::super) fn uni_drc_gain_mut(&mut self) -> &mut UniDrcGain {
        &mut self.uni_drc_gain
    }

    /// Returns immutable reference to `UniDrcConfig`.
    pub(in super::super) fn uni_drc_config(&self) -> &UniDrcConfig {
        &self.uni_drc_config
    }

    /// Returns mutable reference to `UniDrcConfig`.
    pub(in super::super) fn uni_drc_config_mut(&mut self) -> &mut UniDrcConfig {
        &mut self.uni_drc_config
    }

    /// Returns immutable reference to `LoudnessInfoSet`.
    pub(in super::super) fn loudness_info_set(&self) -> &LoudnessInfoSet {
        &self.loudness_info_set
    }

    /// Returns mutable reference to `LoudnessInfoSet`.
    pub(in super::super) fn loudness_info_set_mut(&mut self) -> &mut LoudnessInfoSet {
        &mut self.loudness_info_set
    }

    /// Returns status of `uniDRC gain`.
    pub(in super::super) fn get_gain_status(&self) -> bool {
        self.uni_drc_gain.get_status()
    }
}
