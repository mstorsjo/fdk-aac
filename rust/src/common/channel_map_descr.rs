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
//! Channel map descriptor.

use crate::common::channel_order::ChannelOrder;
use crate::common::tables::channel_map_descriptors::{MAP_INFO_TAB_CICP, MAP_INFO_TAB_DFLT};

#[repr(C)]
#[derive(PartialEq, Copy, Clone, Default, Debug)]
/// Channel map descriptor structure.
///
/// This is the main data struct. It contains the mapping for all channel configurations such as
/// administration information.
pub struct ChannelMapDescriptor {
    /// Table of channel maps.
    pub map_info_tab: Option<&'static [ChannelMapInfo]>,
    /// Defines the specified mapping that shall be applied (MPEG, WAV or CICP mapping).
    pub ch_map_order: ChannelOrder,
}

impl ChannelMapDescriptor {
    /// Creates a ChannelMapDescriptor instance.
    pub fn new() -> ChannelMapDescriptor {
        Default::default()
    }

    /// Initializes the complete channel map descriptor.
    ///
    /// # Parameters
    ///
    /// - `map_info_tab`: Optional.Table of channel maps to initizalize the descriptor with. If
    ///   None, then mapping based on ch_map_order will be used.
    /// - `ch_map_order`: Set the channel order (given by map_info_tab) to MPEG, WAV or CICP.
    ///   Default: WAV.
    ///
    /// # Examples
    ///
    /// ```
    /// use aac::common::channel_map_descr::ChannelMapDescriptor;
    /// use aac::common::channel_order::ChannelOrder;
    ///
    /// let mut channel_map_descr = ChannelMapDescriptor::new();
    /// let map_info_tab = None;
    /// let ch_map_order = ChannelOrder::Wav;
    /// channel_map_descr.init(map_info_tab, ch_map_order);
    /// ```
    pub fn init(
        &mut self,
        map_info_tab: Option<&'static [ChannelMapInfo]>,
        ch_map_order: ChannelOrder,
    ) {
        let mut use_default_tab: bool = true;
        self.ch_map_order = ch_map_order;

        if let Some(map_info_tab_unwrap) = map_info_tab {
            if !map_info_tab_unwrap.is_empty() {
                self.map_info_tab = map_info_tab;
                use_default_tab = !self.is_valid();
            }
        }

        if use_default_tab {
            match ch_map_order {
                ChannelOrder::Mpeg => self.map_info_tab = Some(MAP_INFO_TAB_DFLT),
                ChannelOrder::Wav => self.map_info_tab = Some(MAP_INFO_TAB_DFLT),
                ChannelOrder::Cicp => self.map_info_tab = Some(MAP_INFO_TAB_CICP),
            }
        }
    }

    /// Evaluates whether channel map descriptor is reasonable or not.
    pub fn is_valid(&self) -> bool {
        let mut result = false;

        if let Some(map_info_tab_unwrap) = self.map_info_tab {
            result = true;

            for i in map_info_tab_unwrap {
                if !i.ch_map_descr_is_valid_map() {
                    result = false;
                    break;
                }
            }
        }

        result
    }

    /// Evaluates whether channel map descriptor is initialized with given configuration.
    ///
    /// # Parameters
    ///
    /// - `ch_map_order`: Channel mapping order to be validated.
    ///
    /// Returns value 1 if constraint is fulfilled, otherwise 0.
    pub fn check_ch_map_order(&self, ch_map_order: ChannelOrder) -> bool {
        ch_map_order == self.ch_map_order
    }

    /// Get the mapping value for a specific channel and map index.
    ///
    /// # Parameters
    ///
    /// - `ch_idx`: Channel index.
    /// - `map_idx`: mapping index (corresponding to the channel configuration index).
    ///
    /// # Return
    ///
    /// - `map_value`: u8
    pub fn get_map_value(&self, ch_idx: u8, map_idx: usize) -> u8 {
        //MPEG by default.
        let mut map_value: u8 = ch_idx;

        if let Some(map_info_tab) = self.map_info_tab {
            if !self.check_ch_map_order(ChannelOrder::Mpeg)
                && map_info_tab.len() > map_idx
                && map_info_tab[map_idx].channel_map.is_some()
            {
                let channel_map = map_info_tab[map_idx].channel_map.unwrap();
                if (ch_idx as usize) < channel_map.len() {
                    map_value = channel_map[ch_idx as usize];
                }
            }
        }

        map_value
    }
}

#[repr(C)]
#[derive(PartialEq, Copy, Clone, Default, Debug)]
/// Channel map info structure
///
/// Contains information needed for a single channel map.
pub struct ChannelMapInfo {
    pub channel_map: Option<&'static [u8]>,
}

impl ChannelMapInfo {
    /// Creates a ChannelMapInfo instance.
    pub fn new() -> ChannelMapInfo {
        Default::default()
    }

    /// Evaluates whether single channel map is reasonable or not.
    ///
    /// # Return
    ///
    /// - `result`: : {true, false}
    fn ch_map_descr_is_valid_map(&self) -> bool {
        let mut result: bool = true;

        if self.channel_map.is_some() {
            let channel_map = self.channel_map.unwrap();
            let num_channels: usize = channel_map.len();

            if num_channels < 32 {
                let mut mapped_ch_mask: u32 = 0x0;

                for ch_map_i in channel_map.iter() {
                    mapped_ch_mask |= 1 << *ch_map_i;
                }

                if mapped_ch_mask != ((1 << num_channels) - 1) {
                    result = false;
                }
            } else {
                for (idx, ch_map_i) in channel_map.iter().enumerate() {
                    if (num_channels - 1) < (*ch_map_i as usize) {
                        result = false;
                    } else {
                        for (jdx, ch_map_j) in channel_map.iter().enumerate().rev() {
                            if (jdx > idx) && ch_map_i == ch_map_j {
                                result = false;
                            }
                        }
                    }
                }
            }
        } else {
            result = false;
        }
        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::tables::channel_map_descriptors::{
        TEST_INVALID_MAP_EXCEEDED_IDX, TEST_VALID_MAP, TEST_VALID_MAP_WITH_NONE,
    };

    #[test]
    fn test_ch_map_descr_new() {
        let channel_map_descr: ChannelMapDescriptor = ChannelMapDescriptor::new();
        assert!(channel_map_descr.ch_map_order == ChannelOrder::Mpeg);
        assert!(channel_map_descr.map_info_tab.is_none());
    }

    #[test]
    fn test_ch_map_descr_init_none() {
        let mut channel_map_table_is_none: ChannelMapDescriptor = Default::default();
        channel_map_table_is_none.init(None, ChannelOrder::Wav);

        assert!(channel_map_table_is_none.map_info_tab == Some(MAP_INFO_TAB_DFLT));
        assert!(channel_map_table_is_none.ch_map_order == ChannelOrder::Wav);
    }

    #[test]
    fn test_ch_map_descr_init_not_none() {
        let mut channel_map_not_none: ChannelMapDescriptor = Default::default();
        channel_map_not_none.init(Some(TEST_VALID_MAP), ChannelOrder::Mpeg);

        assert!(channel_map_not_none.map_info_tab.is_some());
        assert!(channel_map_not_none.ch_map_order == ChannelOrder::Mpeg)
    }

    #[test]
    fn test_ch_map_descr_init_not_valid_map() {
        let mut channel_map_invalid: ChannelMapDescriptor = Default::default();
        channel_map_invalid.init(Some(TEST_INVALID_MAP_EXCEEDED_IDX), ChannelOrder::Mpeg);

        assert!(channel_map_invalid.map_info_tab == Some(MAP_INFO_TAB_DFLT));
        assert!(channel_map_invalid.ch_map_order == ChannelOrder::Mpeg);
    }

    #[test]
    fn test_ch_map_descr_is_valid_none_map() {
        let channel_map: ChannelMapDescriptor = ChannelMapDescriptor::new();
        assert!(!channel_map.is_valid());
    }

    #[test]
    fn test_ch_map_descr_is_valid_invalid_map() {
        let mut channel_map_invalid_map: ChannelMapDescriptor = ChannelMapDescriptor::new();
        channel_map_invalid_map.ch_map_order = ChannelOrder::Mpeg;
        channel_map_invalid_map.map_info_tab = Some(TEST_INVALID_MAP_EXCEEDED_IDX);

        assert!(!channel_map_invalid_map.is_valid());
    }

    #[test]
    fn test_ch_map_descr_is_valid() {
        let mut channel_map_valid_map: ChannelMapDescriptor = ChannelMapDescriptor::new();
        channel_map_valid_map.ch_map_order = ChannelOrder::Mpeg;
        channel_map_valid_map.map_info_tab = Some(TEST_VALID_MAP);
        assert!(channel_map_valid_map.is_valid());
    }

    #[test]
    fn test_check_ch_map_descr_order_mpeg() {
        let channel_map: ChannelMapDescriptor = Default::default();

        let result_mpeg: bool = channel_map.check_ch_map_order(ChannelOrder::Mpeg);
        assert!(result_mpeg);
    }

    #[test]
    fn test_check_ch_map_descr_order_cicp() {
        let channel_map: ChannelMapDescriptor = Default::default();

        let result_cicp: bool = channel_map.check_ch_map_order(ChannelOrder::Cicp);
        assert!(!result_cicp);
    }

    #[test]
    fn test_check_ch_map_descr_order_wav() {
        let channel_map: ChannelMapDescriptor = Default::default();

        let result_wav: bool = channel_map.check_ch_map_order(ChannelOrder::Wav);
        assert!(!result_wav);
    }

    #[test]
    fn test_get_ch_map_descr_value_none_table() {
        let channel_map_table_none: ChannelMapDescriptor = Default::default();
        assert!(channel_map_table_none.get_map_value(1, 0) == 1);
    }

    #[test]
    fn test_get_ch_map_descr_value_mpeg() {
        let mut channel_map_table_mpeg = ChannelMapDescriptor::new();
        channel_map_table_mpeg.ch_map_order = ChannelOrder::Mpeg;
        channel_map_table_mpeg.map_info_tab = Some(TEST_VALID_MAP);
        assert!(channel_map_table_mpeg.get_map_value(5, 0) == 5);
    }

    #[test]
    fn test_get_ch_map_descr_value_map_idx_exceeded() {
        let mut channel_map_table_map_idx_exceeded = ChannelMapDescriptor::new();
        channel_map_table_map_idx_exceeded.ch_map_order = ChannelOrder::Wav;
        channel_map_table_map_idx_exceeded.map_info_tab = Some(TEST_VALID_MAP);
        assert!(channel_map_table_map_idx_exceeded.get_map_value(1, 3) == 1);
    }

    #[test]
    fn test_get_ch_map_descr_value_channel_idx_exceeded() {
        let mut channel_map_table_channel_idx_exceeded = ChannelMapDescriptor::new();
        channel_map_table_channel_idx_exceeded.ch_map_order = ChannelOrder::Wav;
        channel_map_table_channel_idx_exceeded.map_info_tab = Some(TEST_VALID_MAP);
        assert!(channel_map_table_channel_idx_exceeded.get_map_value(25, 0) == 25);
    }

    #[test]
    fn test_get_ch_map_descr_value_map_info_is_none() {
        let mut channel_map_info_is_none = ChannelMapDescriptor::new();
        channel_map_info_is_none.ch_map_order = ChannelOrder::Wav;
        channel_map_info_is_none.map_info_tab = Some(TEST_VALID_MAP_WITH_NONE);
        assert!(channel_map_info_is_none.get_map_value(0, 2) == 0);
    }

    #[test]
    fn test_get_ch_map_descr_value_wav_with_valid_map() {
        let mut channel_map_table_wav = ChannelMapDescriptor::new();
        channel_map_table_wav.ch_map_order = ChannelOrder::Wav;
        channel_map_table_wav.map_info_tab = Some(TEST_VALID_MAP);

        assert!(channel_map_table_wav.get_map_value(8, 0) == 10);
    }

    #[test]
    fn test_channel_map_info_default() {
        let channel_map_info: ChannelMapInfo = Default::default();
        assert!(channel_map_info.channel_map.is_none());
    }

    #[test]
    fn test_channel_map_info_new_none() {
        let channel_map_info_new: ChannelMapInfo = ChannelMapInfo::new();
        assert!(channel_map_info_new.channel_map.is_none());
    }

    #[test]
    fn test_channel_map_info_not_none() {
        let mut channel_map_info_not_none: ChannelMapInfo = ChannelMapInfo::new();
        channel_map_info_not_none.channel_map = Some(&[0, 1]);
        assert!(channel_map_info_not_none.channel_map.is_some());
    }

    #[test]
    fn test_ch_map_descr_is_valid_map_none() {
        let mut channel_map_info_none: ChannelMapInfo = ChannelMapInfo::new();
        channel_map_info_none.channel_map = None;
        assert!(!channel_map_info_none.ch_map_descr_is_valid_map());
    }

    #[test]
    fn test_ch_map_descr_is_valid_map_idx_exceeded() {
        static INVALID_MAP_IDX_EXCEEDED: &[u8] = &[
            25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        ];
        let invalid_map_info: Option<&'static [u8]> = Some(INVALID_MAP_IDX_EXCEEDED);
        let mut channel_map_info: ChannelMapInfo = ChannelMapInfo::new();
        channel_map_info.channel_map = invalid_map_info;
        assert!(!channel_map_info.ch_map_descr_is_valid_map());
    }

    #[test]
    fn test_ch_map_descr_is_valid_map_repetitions() {
        static INVALID_MAP_REPETITIONS: &[u8] = &[1, 2, 3, 4, 5, 6, 7, 8, 9, 1];
        let invalid_map_info_repeat: Option<&'static [u8]> = Some(INVALID_MAP_REPETITIONS);
        let mut channel_map_info_repeat: ChannelMapInfo = ChannelMapInfo::new();
        channel_map_info_repeat.channel_map = invalid_map_info_repeat;
        assert!(!channel_map_info_repeat.ch_map_descr_is_valid_map());
    }

    #[test]
    fn test_ch_map_descr_is_valid_map() {
        static VALID_MAP: &[u8] = &[2, 0, 1];
        let valid_map: Option<&'static [u8]> = Some(VALID_MAP);
        let mut valid_channel_map_info: ChannelMapInfo = ChannelMapInfo::new();
        valid_channel_map_info.channel_map = valid_map;
        assert!(valid_channel_map_info.ch_map_descr_is_valid_map());
    }
}
